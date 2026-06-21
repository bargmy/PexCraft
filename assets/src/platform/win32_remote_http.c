/* Opera 11-compatible low-latency HTTP/MJPEG remote viewer.
   This module is intentionally display-only.  It streams the OpenGL back
   buffer as multipart JPEG so very old browsers can render it as a plain
   <img> without WebRTC, WebSocket, WebAssembly, or MediaSource support. */

#if defined(PEX_REMOTE_HTTP) && !defined(PEX_PLATFORM_SDL2) && !defined(PEX_PLATFORM_PSP)

#ifndef PEX_REMOTE_PORT
#define PEX_REMOTE_PORT 8425
#endif
#ifndef PEX_REMOTE_FPS
#define PEX_REMOTE_FPS 30
#endif
#ifndef PEX_REMOTE_JPEG_QUALITY
#define PEX_REMOTE_JPEG_QUALITY 0.55f
#endif

static CRITICAL_SECTION g_remote_http_cs;
static int g_remote_http_cs_ready = 0;
static volatile LONG g_remote_http_running = 0;
static HANDLE g_remote_http_thread = NULL;
static SOCKET g_remote_http_listener = INVALID_SOCKET;
static unsigned char *g_remote_http_jpeg = NULL;
static size_t g_remote_http_jpeg_len = 0;
static LONG g_remote_http_seq = 0;
static int g_remote_http_w = 0;
static int g_remote_http_h = 0;
static double g_remote_http_last_capture = 0.0;
static int g_remote_http_wsa_started = 0;
static volatile LONG g_remote_http_client_count = 0;

static int pex_remote_send_all(SOCKET s, const void *buf, int len) {
    const char *p = (const char*)buf;
    while (len > 0) {
        int n = send(s, p, len, 0);
        if (n <= 0) return 0;
        p += n;
        len -= n;
    }
    return 1;
}

static void pex_remote_send_text(SOCKET s, const char *status, const char *content_type, const char *body) {
    char hdr[512];
    int body_len = (int)strlen(body);
    int n = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 %s\r\n"
        "Connection: close\r\n"
        "Cache-Control: no-cache, no-store, must-revalidate\r\n"
        "Pragma: no-cache\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n\r\n",
        status, content_type, body_len);
    if (n > 0) pex_remote_send_all(s, hdr, n);
    pex_remote_send_all(s, body, body_len);
}

static void pex_remote_send_index(SOCKET s) {
    static const char body[] =
        "<!doctype html>\n"
        "<html><head><meta http-equiv='X-UA-Compatible' content='IE=edge'>\n"
        "<title>PEXCRAFT Remote</title>\n"
        "<style>html,body{margin:0;padding:0;width:100%;height:100%;overflow:hidden;background:#000;}"
        "#screen{position:absolute;left:0;top:0;width:100%;height:100%;border:0;}"
        "#hud{position:absolute;left:8px;top:8px;color:#fff;background:#000;padding:4px 6px;"
        "font:12px sans-serif;opacity:.65;}</style></head>\n"
        "<body><img id='screen' src='/stream.mjpg' alt='PEXCRAFT stream'>"
        "<div id='hud'>PEXCRAFT remote stream - press F11 for browser fullscreen</div>"
        "<script type='text/javascript'>setTimeout(function(){var h=document.getElementById('hud');"
        "if(h)h.style.display='none';},3500);</script></body></html>\n";
    pex_remote_send_text(s, "200 OK", "text/html; charset=utf-8", body);
}

static void pex_remote_send_status(SOCKET s) {
    char body[256];
    LONG seq;
    int w, h;
    size_t len;
    EnterCriticalSection(&g_remote_http_cs);
    seq = g_remote_http_seq;
    w = g_remote_http_w;
    h = g_remote_http_h;
    len = g_remote_http_jpeg_len;
    LeaveCriticalSection(&g_remote_http_cs);
    snprintf(body, sizeof(body), "ok\nframes=%ld\nwidth=%d\nheight=%d\njpeg_bytes=%lu\n", seq, w, h, (unsigned long)len);
    pex_remote_send_text(s, "200 OK", "text/plain; charset=utf-8", body);
}

static void pex_remote_stream_mjpeg(SOCKET s) {
    static const char hdr[] =
        "HTTP/1.1 200 OK\r\n"
        "Connection: close\r\n"
        "Cache-Control: no-cache, no-store, must-revalidate\r\n"
        "Pragma: no-cache\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=pexframe\r\n\r\n";
    unsigned char *local = NULL;
    size_t local_cap = 0;
    LONG last_seq = 0;

    InterlockedIncrement(&g_remote_http_client_count);
    if (!pex_remote_send_all(s, hdr, (int)strlen(hdr))) goto done;

    while (InterlockedCompareExchange(&g_remote_http_running, 1, 1)) {
        LONG seq = 0;
        size_t len = 0;
        int copied = 0;

        EnterCriticalSection(&g_remote_http_cs);
        if (g_remote_http_seq != last_seq && g_remote_http_jpeg && g_remote_http_jpeg_len > 0) {
            len = g_remote_http_jpeg_len;
            if (len > local_cap) {
                unsigned char *n = (unsigned char*)realloc(local, len);
                if (n) { local = n; local_cap = len; }
            }
            if (local && len <= local_cap) {
                memcpy(local, g_remote_http_jpeg, len);
                seq = g_remote_http_seq;
                copied = 1;
            }
        }
        LeaveCriticalSection(&g_remote_http_cs);

        if (!copied) {
            Sleep(1);
            continue;
        }

        char part[160];
        int n = snprintf(part, sizeof(part),
            "--pexframe\r\nContent-Type: image/jpeg\r\nContent-Length: %lu\r\n\r\n",
            (unsigned long)len);
        if (n <= 0 || !pex_remote_send_all(s, part, n)) break;
        if (!pex_remote_send_all(s, local, (int)len)) break;
        if (!pex_remote_send_all(s, "\r\n", 2)) break;
        last_seq = seq;
    }

done:
    InterlockedDecrement(&g_remote_http_client_count);
    free(local);
}

static DWORD WINAPI pex_remote_client_thread(LPVOID param) {
    SOCKET s = (SOCKET)(uintptr_t)param;
    char req[2048];
    int got = 0;
    req[0] = 0;

    for (;;) {
        int n = recv(s, req + got, (int)sizeof(req) - 1 - got, 0);
        if (n <= 0) goto done;
        got += n;
        req[got] = 0;
        if (strstr(req, "\r\n\r\n") || got >= (int)sizeof(req) - 1) break;
    }

    if (!strncmp(req, "GET /stream.mjpg", 16) || !strncmp(req, "GET /stream", 11)) {
        pex_remote_stream_mjpeg(s);
    } else if (!strncmp(req, "GET /status", 11)) {
        pex_remote_send_status(s);
    } else if (!strncmp(req, "GET / ", 6) || !strncmp(req, "GET /HTTP", 9) || !strncmp(req, "GET /index", 10)) {
        pex_remote_send_index(s);
    } else {
        pex_remote_send_text(s, "404 Not Found", "text/plain; charset=utf-8", "not found\n");
    }

done:
    shutdown(s, SD_BOTH);
    closesocket(s);
    return 0;
}

static DWORD WINAPI pex_remote_server_thread(LPVOID param) {
    (void)param;
    SOCKET ls = INVALID_SOCKET;
    struct sockaddr_in addr;
    int yes = 1;

    ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ls == INVALID_SOCKET) return 1;
    g_remote_http_listener = ls;
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); /* 0.0.0.0 as requested. */
    addr.sin_port = htons((u_short)PEX_REMOTE_PORT);

    if (bind(ls, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) goto fail;
    if (listen(ls, 8) == SOCKET_ERROR) goto fail;

    while (InterlockedCompareExchange(&g_remote_http_running, 1, 1)) {
        SOCKET cs = accept(ls, NULL, NULL);
        if (cs == INVALID_SOCKET) {
            if (!InterlockedCompareExchange(&g_remote_http_running, 1, 1)) break;
            Sleep(5);
            continue;
        }
        DWORD tid = 0;
        HANDLE th = CreateThread(NULL, 0, pex_remote_client_thread, (LPVOID)(uintptr_t)cs, 0, &tid);
        if (th) CloseHandle(th);
        else { shutdown(cs, SD_BOTH); closesocket(cs); }
    }

fail:
    if (ls != INVALID_SOCKET) closesocket(ls);
    g_remote_http_listener = INVALID_SOCKET;
    return 0;
}

static int pex_remote_encode_jpeg_24bgr(const unsigned char *bgr, int w, int h, unsigned char **out, size_t *out_len) {
    IWICBitmapEncoder *enc = NULL;
    IWICBitmapFrameEncode *frame = NULL;
    IPropertyBag2 *props = NULL;
    IStream *stream = NULL;
    HGLOBAL hglobal = NULL;
    HRESULT hr;
    WICPixelFormatGUID fmt = GUID_WICPixelFormat24bppBGR;
    UINT stride = (UINT)w * 3U;
    UINT bytes = stride * (UINT)h;
    int ok = 0;

    *out = NULL;
    *out_len = 0;
    if (!g_wic_factory && !ensure_wic()) return 0;

    hglobal = GlobalAlloc(GMEM_MOVEABLE, 0);
    if (!hglobal) goto done;
    hr = CreateStreamOnHGlobal(hglobal, TRUE, &stream);
    if (FAILED(hr)) goto done;
    hglobal = NULL; /* stream owns it now */

    hr = IWICImagingFactory_CreateEncoder(g_wic_factory, &GUID_ContainerFormatJpeg, NULL, &enc);
    if (FAILED(hr)) goto done;
    hr = IWICBitmapEncoder_Initialize(enc, stream, WICBitmapEncoderNoCache);
    if (FAILED(hr)) goto done;
    hr = IWICBitmapEncoder_CreateNewFrame(enc, &frame, &props);
    if (FAILED(hr)) goto done;

    if (props) {
        PROPBAG2 opt;
        VARIANT var;
        memset(&opt, 0, sizeof(opt));
        VariantInit(&var);
        opt.pstrName = L"ImageQuality";
        var.vt = VT_R4;
        var.fltVal = (float)PEX_REMOTE_JPEG_QUALITY;
        IPropertyBag2_Write(props, 1, &opt, &var);
        VariantClear(&var);
    }

    hr = IWICBitmapFrameEncode_Initialize(frame, props);
    if (FAILED(hr)) goto done;
    hr = IWICBitmapFrameEncode_SetSize(frame, (UINT)w, (UINT)h);
    if (FAILED(hr)) goto done;
    hr = IWICBitmapFrameEncode_SetPixelFormat(frame, &fmt);
    if (FAILED(hr)) goto done;
    hr = IWICBitmapFrameEncode_WritePixels(frame, (UINT)h, stride, bytes, (BYTE*)bgr);
    if (FAILED(hr)) goto done;
    hr = IWICBitmapFrameEncode_Commit(frame);
    if (FAILED(hr)) goto done;
    hr = IWICBitmapEncoder_Commit(enc);
    if (FAILED(hr)) goto done;

    STATSTG st;
    memset(&st, 0, sizeof(st));
    hr = IStream_Stat(stream, &st, STATFLAG_NONAME);
    if (FAILED(hr) || st.cbSize.QuadPart <= 0 || st.cbSize.QuadPart > 16 * 1024 * 1024) goto done;

    HGLOBAL hg = NULL;
    hr = GetHGlobalFromStream(stream, &hg);
    if (FAILED(hr) || !hg) goto done;
    SIZE_T sz = (SIZE_T)st.cbSize.QuadPart;
    void *p = GlobalLock(hg);
    if (!p) goto done;
    unsigned char *copy = (unsigned char*)malloc((size_t)sz);
    if (copy) {
        memcpy(copy, p, (size_t)sz);
        *out = copy;
        *out_len = (size_t)sz;
        ok = 1;
    }
    GlobalUnlock(hg);

 done:
    if (props) IPropertyBag2_Release(props);
    if (frame) IWICBitmapFrameEncode_Release(frame);
    if (enc) IWICBitmapEncoder_Release(enc);
    if (stream) IStream_Release(stream);
    if (hglobal) GlobalFree(hglobal);
    return ok;
}

static void pex_remote_http_capture_opengl_frame(void) {
    if (!InterlockedCompareExchange(&g_remote_http_running, 1, 1)) return;
    if (InterlockedCompareExchange(&g_remote_http_client_count, 0, 0) <= 0) return;
    if (pex_using_d3d9() || pex_using_d3d11()) return;

    double t = now_seconds();
    double min_dt = 1.0 / (double)PEX_REMOTE_FPS;
    if (g_remote_http_last_capture > 0.0 && (t - g_remote_http_last_capture) < min_dt) return;
    g_remote_http_last_capture = t;

    int w = g_win_w;
    int h = g_win_h;
    if (w <= 0 || h <= 0 || w > 4096 || h > 4096) return;

    size_t rgb_len = (size_t)w * (size_t)h * 3U;
    unsigned char *rgb = (unsigned char*)malloc(rgb_len);
    unsigned char *bgr = (unsigned char*)malloc(rgb_len);
    if (!rgb || !bgr) { free(rgb); free(bgr); return; }

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, rgb);

    for (int y = 0; y < h; ++y) {
        unsigned char *dst = bgr + (size_t)y * (size_t)w * 3U;
        const unsigned char *src = rgb + (size_t)(h - 1 - y) * (size_t)w * 3U;
        for (int x = 0; x < w; ++x) {
            dst[x * 3 + 0] = src[x * 3 + 2];
            dst[x * 3 + 1] = src[x * 3 + 1];
            dst[x * 3 + 2] = src[x * 3 + 0];
        }
    }

    unsigned char *jpg = NULL;
    size_t jpg_len = 0;
    if (pex_remote_encode_jpeg_24bgr(bgr, w, h, &jpg, &jpg_len)) {
        EnterCriticalSection(&g_remote_http_cs);
        free(g_remote_http_jpeg);
        g_remote_http_jpeg = jpg;
        g_remote_http_jpeg_len = jpg_len;
        g_remote_http_w = w;
        g_remote_http_h = h;
        InterlockedIncrement(&g_remote_http_seq);
        LeaveCriticalSection(&g_remote_http_cs);
    }

    free(rgb);
    free(bgr);
}

static int pex_remote_http_start(void) {
    if (InterlockedCompareExchange(&g_remote_http_running, 1, 0)) return 1;

    if (!g_remote_http_cs_ready) {
        InitializeCriticalSection(&g_remote_http_cs);
        g_remote_http_cs_ready = 1;
    }

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        InterlockedExchange(&g_remote_http_running, 0);
        return 0;
    }
    g_remote_http_wsa_started = 1;

    DWORD tid = 0;
    g_remote_http_thread = CreateThread(NULL, 0, pex_remote_server_thread, NULL, 0, &tid);
    if (!g_remote_http_thread) {
        InterlockedExchange(&g_remote_http_running, 0);
        WSACleanup();
        g_remote_http_wsa_started = 0;
        return 0;
    }
    return 1;
}

static void pex_remote_http_stop(void) {
    InterlockedExchange(&g_remote_http_running, 0);
    if (g_remote_http_listener != INVALID_SOCKET) {
        closesocket(g_remote_http_listener);
        g_remote_http_listener = INVALID_SOCKET;
    }
    if (g_remote_http_thread) {
        WaitForSingleObject(g_remote_http_thread, 3000);
        CloseHandle(g_remote_http_thread);
        g_remote_http_thread = NULL;
    }
    if (g_remote_http_wsa_started) {
        WSACleanup();
        g_remote_http_wsa_started = 0;
    }
    if (g_remote_http_cs_ready) {
        EnterCriticalSection(&g_remote_http_cs);
        free(g_remote_http_jpeg);
        g_remote_http_jpeg = NULL;
        g_remote_http_jpeg_len = 0;
        LeaveCriticalSection(&g_remote_http_cs);
        DeleteCriticalSection(&g_remote_http_cs);
        g_remote_http_cs_ready = 0;
    }
}

#else
static int pex_remote_http_start(void) { return 1; }
static void pex_remote_http_stop(void) { }
static void pex_remote_http_capture_opengl_frame(void) { }
#endif
