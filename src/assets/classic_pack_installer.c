/* Split from original monolithic main.c. Included by src/main.c unity build. */

#define CLASSIC_INSTALL_IDLE 0
#define CLASSIC_INSTALL_DOWNLOADING 1
#define CLASSIC_INSTALL_EXTRACTING 2
#define CLASSIC_INSTALL_DONE 3
#define CLASSIC_INSTALL_ERROR 4

static volatile LONG g_classic_install_state = CLASSIC_INSTALL_IDLE;
static volatile LONG g_classic_install_progress = 0;
static HANDLE g_classic_install_thread = NULL;
static char g_classic_install_status[MAX_LABEL] = "";
static char g_classic_install_error[MAX_LABEL] = "";

typedef void *PxcHInternet;
typedef WORD PxcInternetPort;
typedef PxcHInternet (WINAPI *PxcInternetOpenA)(LPCSTR, DWORD, LPCSTR, LPCSTR, DWORD);
typedef PxcHInternet (WINAPI *PxcInternetOpenUrlA)(PxcHInternet, LPCSTR, LPCSTR, DWORD, DWORD, DWORD_PTR);
typedef PxcHInternet (WINAPI *PxcInternetConnectA)(PxcHInternet, LPCSTR, PxcInternetPort, LPCSTR, LPCSTR, DWORD, DWORD, DWORD_PTR);
typedef PxcHInternet (WINAPI *PxcHttpOpenRequestA)(PxcHInternet, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPCSTR*, DWORD, DWORD_PTR);
typedef BOOL (WINAPI *PxcHttpSendRequestA)(PxcHInternet, LPCSTR, DWORD, LPVOID, DWORD);
typedef BOOL (WINAPI *PxcInternetReadFile)(PxcHInternet, LPVOID, DWORD, LPDWORD);
typedef BOOL (WINAPI *PxcInternetCloseHandle)(PxcHInternet);
typedef BOOL (WINAPI *PxcHttpQueryInfoA)(PxcHInternet, DWORD, LPVOID, LPDWORD, LPDWORD);

#ifndef INTERNET_OPEN_TYPE_PRECONFIG
#define INTERNET_OPEN_TYPE_PRECONFIG 0
#endif
#ifndef INTERNET_FLAG_RELOAD
#define INTERNET_FLAG_RELOAD 0x80000000u
#endif
#ifndef INTERNET_FLAG_NO_CACHE_WRITE
#define INTERNET_FLAG_NO_CACHE_WRITE 0x04000000u
#endif
#ifndef HTTP_QUERY_CONTENT_LENGTH
#define HTTP_QUERY_CONTENT_LENGTH 5
#endif
#ifndef HTTP_QUERY_FLAG_NUMBER
#define HTTP_QUERY_FLAG_NUMBER 0x20000000u
#endif
#ifndef INTERNET_FLAG_NO_UI
#define INTERNET_FLAG_NO_UI 0x00000200u
#endif
#ifndef INTERNET_FLAG_PRAGMA_NOCACHE
#define INTERNET_FLAG_PRAGMA_NOCACHE 0x00000100u
#endif
#ifndef INTERNET_FLAG_SECURE
#define INTERNET_FLAG_SECURE 0x00800000u
#endif
#ifndef INTERNET_SERVICE_HTTP
#define INTERNET_SERVICE_HTTP 3
#endif
#ifndef INTERNET_DEFAULT_HTTP_PORT
#define INTERNET_DEFAULT_HTTP_PORT 80
#endif
#ifndef INTERNET_DEFAULT_HTTPS_PORT
#define INTERNET_DEFAULT_HTTPS_PORT 443
#endif

#define CLASSIC_SIZE_UNKNOWN 0
#define CLASSIC_SIZE_FETCHING 1
#define CLASSIC_SIZE_READY 2
#define CLASSIC_SIZE_ERROR 3

static volatile LONG g_classic_download_size_state = CLASSIC_SIZE_UNKNOWN;
static volatile LONG g_classic_download_size_bytes = 0;

static unsigned long long parse_decimal_u64(const char *s) {
    unsigned long long v = 0;
    if (!s) return 0;
    while (*s == ' ' || *s == '\t') s++;
    while (*s >= '0' && *s <= '9') {
        unsigned long long nv = v * 10ULL + (unsigned long long)(*s - '0');
        if (nv < v) return 0;
        v = nv;
        s++;
    }
    return v;
}

static int classic_split_url(const char *url, int *secure, char *host, size_t host_cap, char *path, size_t path_cap) {
    const char *p;
    const char *h;
    const char *slash;
    size_t host_len;
    if (!url || !host || !path || host_cap == 0 || path_cap == 0) return 0;

    *secure = 0;
    p = strstr(url, "://");
    if (!p) return 0;
    if (!strncmp(url, "https", 5)) *secure = 1;
    else if (strncmp(url, "http", 4)) return 0;

    h = p + 3;
    slash = strchr(h, '/');
    host_len = slash ? (size_t)(slash - h) : strlen(h);
    if (host_len == 0 || host_len >= host_cap) return 0;

    memcpy(host, h, host_len);
    host[host_len] = 0;
    snprintf(path, path_cap, "%s", slash ? slash : "/");
    return 1;
}

static int classic_query_content_length(PxcHInternet req, PxcHttpQueryInfoA pHttpQueryInfoA, unsigned long long *out_bytes) {
    unsigned long long value = 0;
    DWORD total32 = 0;
    DWORD total_len = sizeof(total32);

    if (pHttpQueryInfoA(req, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &total32, &total_len, NULL) && total32 > 0) {
        value = (unsigned long long)total32;
    } else {
        char text[64];
        DWORD text_len = sizeof(text);
        memset(text, 0, sizeof(text));
        if (pHttpQueryInfoA(req, HTTP_QUERY_CONTENT_LENGTH, text, &text_len, NULL)) {
            text[sizeof(text) - 1] = 0;
            value = parse_decimal_u64(text);
        }
    }

    if (value == 0) return 0;
    if (out_bytes) *out_bytes = value;
    return 1;
}

static int classic_query_download_size_bytes(const char *url, unsigned long long *out_bytes) {
    if (out_bytes) *out_bytes = 0;

    HMODULE wininet = LoadLibraryA("wininet.dll");
    if (!wininet) return 0;

    PxcInternetOpenA pInternetOpenA = (PxcInternetOpenA)GetProcAddress(wininet, "InternetOpenA");
    PxcInternetOpenUrlA pInternetOpenUrlA = (PxcInternetOpenUrlA)GetProcAddress(wininet, "InternetOpenUrlA");
    PxcInternetConnectA pInternetConnectA = (PxcInternetConnectA)GetProcAddress(wininet, "InternetConnectA");
    PxcHttpOpenRequestA pHttpOpenRequestA = (PxcHttpOpenRequestA)GetProcAddress(wininet, "HttpOpenRequestA");
    PxcHttpSendRequestA pHttpSendRequestA = (PxcHttpSendRequestA)GetProcAddress(wininet, "HttpSendRequestA");
    PxcInternetCloseHandle pInternetCloseHandle = (PxcInternetCloseHandle)GetProcAddress(wininet, "InternetCloseHandle");
    PxcHttpQueryInfoA pHttpQueryInfoA = (PxcHttpQueryInfoA)GetProcAddress(wininet, "HttpQueryInfoA");

    if (!pInternetOpenA || !pInternetOpenUrlA || !pInternetCloseHandle || !pHttpQueryInfoA) {
        FreeLibrary(wininet);
        return 0;
    }

    PxcHInternet inet = pInternetOpenA("PEXCRAFT", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!inet) {
        FreeLibrary(wininet);
        return 0;
    }

    int ok = 0;
    unsigned long long value = 0;

    /* Prefer a true HEAD request so the warning uses the server's real
       Content-Length header without starting a client.jar body download. */
    if (pInternetConnectA && pHttpOpenRequestA && pHttpSendRequestA) {
        int secure = 0;
        char host[256];
        char path[MAX_PATHBUF];
        if (classic_split_url(url, &secure, host, sizeof(host), path, sizeof(path))) {
            PxcHInternet conn = pInternetConnectA(inet, host, secure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT,
                NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
            if (conn) {
                LPCSTR accept_types[2] = { "*/*", NULL };
                DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_PRAGMA_NOCACHE;
                if (secure) flags |= INTERNET_FLAG_SECURE;
                PxcHInternet req = pHttpOpenRequestA(conn, "HEAD", path, NULL, NULL, accept_types, flags, 0);
                if (req) {
                    const char *headers = "Cache-Control: no-cache\r\n";
                    if (pHttpSendRequestA(req, headers, (DWORD)strlen(headers), NULL, 0)) {
                        ok = classic_query_content_length(req, pHttpQueryInfoA, &value);
                    }
                    pInternetCloseHandle(req);
                }
                pInternetCloseHandle(conn);
            }
        }
    }

    /* Fallback for odd WinINet installs/servers: open the URL and read only
       response headers.  The file body is not consumed here. */
    if (!ok) {
        const char *headers = "Accept: */*\r\nCache-Control: no-cache\r\n";
        PxcHInternet req = pInternetOpenUrlA(inet, url, headers, (DWORD)strlen(headers),
            INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_PRAGMA_NOCACHE, 0);
        if (req) {
            ok = classic_query_content_length(req, pHttpQueryInfoA, &value);
            pInternetCloseHandle(req);
        }
    }

    pInternetCloseHandle(inet);
    FreeLibrary(wininet);

    if (!ok || value == 0) return 0;
    if (out_bytes) *out_bytes = value;
    return 1;
}

static DWORD WINAPI classic_download_size_worker(LPVOID unused) {
    (void)unused;
    unsigned long long bytes = 0;
    if (classic_query_download_size_bytes(CLASSIC_PACK_URL, &bytes) && bytes > 0 && bytes <= 0x7fffffffULL) {
        InterlockedExchange(&g_classic_download_size_bytes, (LONG)bytes);
        InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_READY);
        log_msg("Minecraft Classic resource download size: %llu bytes", bytes);
    } else {
        InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_ERROR);
        log_msg("Could not query Minecraft Classic resource Content-Length");
    }
    return 0;
}

static void classic_resource_size_start_fetch(void) {
    LONG old = InterlockedCompareExchange(&g_classic_download_size_state, CLASSIC_SIZE_FETCHING, CLASSIC_SIZE_UNKNOWN);
    if (old != CLASSIC_SIZE_UNKNOWN) return;
    HANDLE th = CreateThread(NULL, 0, classic_download_size_worker, NULL, 0, NULL);
    if (th) {
        CloseHandle(th);
    } else {
        InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_ERROR);
    }
}

static void classic_resource_size_format(char *out, size_t cap) {
    LONG state = InterlockedCompareExchange(&g_classic_download_size_state, 0, 0);
    if (state == CLASSIC_SIZE_READY) {
        LONG bytes = InterlockedCompareExchange(&g_classic_download_size_bytes, 0, 0);
        if (bytes > 0) {
            double mb = (double)bytes / (1024.0 * 1024.0);
            snprintf(out, cap, "Download size: %.2f MB (%ld bytes)", mb, bytes);
            return;
        }
    } else if (state == CLASSIC_SIZE_FETCHING) {
        snprintf(out, cap, "Download size: checking server...");
        return;
    }
    snprintf(out, cap, "Download size: unavailable");
}

static void classic_install_set_state(LONG state, LONG progress, const char *status) {
    if (status) lstrcpynA(g_classic_install_status, status, sizeof(g_classic_install_status));
    InterlockedExchange(&g_classic_install_progress, progress);
    InterlockedExchange(&g_classic_install_state, state);
}

static void classic_install_fail(const char *msg) {
    lstrcpynA(g_classic_install_error, msg ? msg : "Unknown error", sizeof(g_classic_install_error));
    classic_install_set_state(CLASSIC_INSTALL_ERROR, 0, "Failed");
    log_msg("Minecraft Classic texture pack install failed: %s", g_classic_install_error);
}

static int classic_download_client_jar(const char *url, const char *zip_path) {
    HMODULE wininet = LoadLibraryA("wininet.dll");
    if (!wininet) { classic_install_fail("wininet.dll was not found"); return 0; }

    PxcInternetOpenA pInternetOpenA = (PxcInternetOpenA)GetProcAddress(wininet, "InternetOpenA");
    PxcInternetOpenUrlA pInternetOpenUrlA = (PxcInternetOpenUrlA)GetProcAddress(wininet, "InternetOpenUrlA");
    PxcInternetReadFile pInternetReadFile = (PxcInternetReadFile)GetProcAddress(wininet, "InternetReadFile");
    PxcInternetCloseHandle pInternetCloseHandle = (PxcInternetCloseHandle)GetProcAddress(wininet, "InternetCloseHandle");
    PxcHttpQueryInfoA pHttpQueryInfoA = (PxcHttpQueryInfoA)GetProcAddress(wininet, "HttpQueryInfoA");

    if (!pInternetOpenA || !pInternetOpenUrlA || !pInternetReadFile || !pInternetCloseHandle) {
        FreeLibrary(wininet);
        classic_install_fail("WinINet entry points missing");
        return 0;
    }

    DeleteFileA(zip_path);
    FILE *out = fopen(zip_path, "wb");
    if (!out) {
        FreeLibrary(wininet);
        classic_install_fail("Could not create downloaded client file");
        return 0;
    }

    PxcHInternet inet = pInternetOpenA("PEXCRAFT", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!inet) {
        fclose(out);
        FreeLibrary(wininet);
        classic_install_fail("Could not start internet session");
        return 0;
    }

    classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 1, "Connecting to Mojang...");
    PxcHInternet req = pInternetOpenUrlA(inet, url, NULL, 0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!req) {
        fclose(out);
        pInternetCloseHandle(inet);
        FreeLibrary(wininet);
        classic_install_fail("Could not download client.jar");
        return 0;
    }

    DWORD total = 0;
    if (pHttpQueryInfoA) {
        DWORD total_len = sizeof(total);
        pHttpQueryInfoA(req, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &total, &total_len, NULL);
    }

    unsigned char buf[32768];
    DWORD got = 0;
    DWORD downloaded = 0;
    for (;;) {
        if (!pInternetReadFile(req, buf, sizeof(buf), &got)) {
            fclose(out);
            pInternetCloseHandle(req);
            pInternetCloseHandle(inet);
            FreeLibrary(wininet);
            classic_install_fail("Download failed while reading");
            return 0;
        }
        if (got == 0) break;
        if (fwrite(buf, 1, got, out) != got) {
            fclose(out);
            pInternetCloseHandle(req);
            pInternetCloseHandle(inet);
            FreeLibrary(wininet);
            classic_install_fail("Could not write downloaded file");
            return 0;
        }
        downloaded += got;
        if (total > 0) {
            int pct = 1 + (int)(((unsigned long long)downloaded * 84ULL) / (unsigned long long)total);
            if (pct > 85) pct = 85;
            char st[MAX_LABEL];
            snprintf(st, sizeof(st), "Downloading client.jar (%d%%)", pct);
            classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, pct, st);
        } else {
            int pct = 5 + (int)((downloaded / 65536) % 70);
            classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, pct, "Downloading client.jar");
        }
    }

    fclose(out);
    pInternetCloseHandle(req);
    pInternetCloseHandle(inet);
    FreeLibrary(wininet);

    if (downloaded < 1024) {
        classic_install_fail("Downloaded client.jar was empty");
        return 0;
    }
    return 1;
}

static int write_classic_extract_script(const char *script_path) {
    FILE *f = fopen(script_path, "wb");
    if (!f) return 0;
    fputs("$ErrorActionPreference = 'Stop'\r\n", f);
    fputs("$zip = $env:PEXCRAFT_CLASSIC_ZIP\r\n", f);
    fputs("$pack = $env:PEXCRAFT_CLASSIC_PACK\r\n", f);
    fputs("if (Test-Path -LiteralPath $pack) { Remove-Item -Recurse -Force -LiteralPath $pack }\r\n", f);
    fputs("New-Item -ItemType Directory -Force -Path $pack | Out-Null\r\n", f);
    fputs("Expand-Archive -LiteralPath $zip -DestinationPath $pack -Force\r\n", f);
    fputs("Remove-Item -LiteralPath (Join-Path $pack 'pack.png') -Force -ErrorAction SilentlyContinue\r\n", f);
    fputs("Remove-Item -Recurse -Force -LiteralPath (Join-Path $pack 'META-INF') -ErrorAction SilentlyContinue\r\n", f);
    fputs("Get-ChildItem -LiteralPath $pack -Filter '*.class' -File | Remove-Item -Force -ErrorAction SilentlyContinue\r\n", f);
    fputs("Set-Content -LiteralPath (Join-Path $pack 'pack.txt') -Value @('Minecraft Classic texture pack','Downloaded from client.jar') -Encoding ASCII\r\n", f);
    fputs("if (!(Test-Path -LiteralPath (Join-Path $pack 'terrain.png'))) { throw 'terrain.png missing after extraction' }\r\n", f);
    fputs("if (!(Test-Path -LiteralPath (Join-Path $pack 'gui\\gui.png'))) { throw 'gui/gui.png missing after extraction' }\r\n", f);
    fputs("if (!(Test-Path -LiteralPath (Join-Path $pack 'font\\default.png'))) { throw 'font/default.png missing after extraction' }\r\n", f);
    fputs("if (!(Test-Path -LiteralPath (Join-Path $pack 'gui\\items.png'))) { throw 'gui/items.png missing after extraction' }\r\n", f);
    fclose(f);
    return 1;
}

static int run_hidden_process_wait(const char *cmdline) {
    char mutable_cmd[4096];
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    DWORD code = 1;

    lstrcpynA(mutable_cmd, cmdline, sizeof(mutable_cmd));
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (!CreateProcessA(NULL, mutable_cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) return 0;
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return code == 0;
}

static int classic_extract_downloaded_pack(const char *zip_path, const char *pack_dir) {
    char script[MAX_PATHBUF];
    char cmd[4096];

    snprintf(script, sizeof(script), "%s\\classic_pack_extract.ps1", g_mc_dir);
    if (!write_classic_extract_script(script)) {
        classic_install_fail("Could not write extraction script");
        return 0;
    }

    SetEnvironmentVariableA("PEXCRAFT_CLASSIC_ZIP", zip_path);
    SetEnvironmentVariableA("PEXCRAFT_CLASSIC_PACK", pack_dir);
    classic_install_set_state(CLASSIC_INSTALL_EXTRACTING, 90, "Extracting textures...");

    snprintf(cmd, sizeof(cmd), "powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"%s\"", script);
    int ok = run_hidden_process_wait(cmd);

    SetEnvironmentVariableA("PEXCRAFT_CLASSIC_ZIP", NULL);
    SetEnvironmentVariableA("PEXCRAFT_CLASSIC_PACK", NULL);
    DeleteFileA(script);

    if (!ok) {
        classic_install_fail("Could not extract client.jar");
        return 0;
    }

    if (!classic_pack_installed() || classic_pack_missing_required_textures()) {
        classic_install_fail("Extracted pack is missing required textures");
        return 0;
    }
    DeleteFileA(zip_path);
    return 1;
}

static DWORD WINAPI classic_install_worker(LPVOID unused) {
    (void)unused;
    char zip_path[MAX_PATHBUF];
    char pack_dir[MAX_PATHBUF];

    ensure_dir(g_texpack_dir);
    classic_pack_path(pack_dir, sizeof(pack_dir));
    snprintf(zip_path, sizeof(zip_path), "%s\\minecraft_classic_client.zip", g_mc_dir);

    classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading client.jar...");
    if (!classic_download_client_jar(CLASSIC_PACK_URL, zip_path)) return 0;
    if (!classic_extract_downloaded_pack(zip_path, pack_dir)) return 0;

    classic_install_set_state(CLASSIC_INSTALL_DONE, 100, "Done!");
    log_msg("Installed Minecraft Classic texture pack at %s", pack_dir);
    return 0;
}

static void start_classic_pack_install(void) {
    LONG state = InterlockedCompareExchange(&g_classic_install_state, CLASSIC_INSTALL_DOWNLOADING, CLASSIC_INSTALL_IDLE);
    if (state == CLASSIC_INSTALL_DOWNLOADING || state == CLASSIC_INSTALL_EXTRACTING) {
        set_screen(SCREEN_TEXPACK_INSTALL);
        return;
    }

    if (g_classic_install_thread) {
        CloseHandle(g_classic_install_thread);
        g_classic_install_thread = NULL;
    }

    g_classic_install_error[0] = 0;
    classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading client.jar...");
    set_screen(SCREEN_TEXPACK_INSTALL);
    g_classic_install_thread = CreateThread(NULL, 0, classic_install_worker, NULL, 0, NULL);
    if (!g_classic_install_thread) {
        classic_install_fail("Could not start installer thread");
    }
}

static void classic_pack_install_tick(void) {
    LONG state = InterlockedCompareExchange(&g_classic_install_state, 0, 0);
    if (state == CLASSIC_INSTALL_DONE) {
        if (g_classic_install_thread) {
            WaitForSingleObject(g_classic_install_thread, 0);
            CloseHandle(g_classic_install_thread);
            g_classic_install_thread = NULL;
        }
        InterlockedExchange(&g_classic_install_state, CLASSIC_INSTALL_IDLE);
        scan_texture_packs();
        for (int i = 0; i < g_texpack_count; i++) {
            if (g_texpacks[i].is_builtin_classic) {
                apply_texture_pack_index(i);
                break;
            }
        }
        set_screen(SCREEN_TEXPACK);
    } else if (state == CLASSIC_INSTALL_ERROR) {
        if (g_classic_install_thread) {
            WaitForSingleObject(g_classic_install_thread, 0);
            CloseHandle(g_classic_install_thread);
            g_classic_install_thread = NULL;
        }
        InterlockedExchange(&g_classic_install_state, CLASSIC_INSTALL_IDLE);
        open_notice("Texture Pack", "Could not install Minecraft Classic.", g_classic_install_error);
    }
}
