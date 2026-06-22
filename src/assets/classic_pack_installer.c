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
static volatile LONG g_classic_texture_download_size_bytes = 0;
static volatile LONG g_classic_sound_download_size_bytes = 0;
static volatile LONG g_classic_sound_download_count = 0;

static void classic_install_set_state(LONG state, LONG progress, const char *status);
static void classic_install_fail(const char *msg);

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


typedef struct ClassicSoundAsset {
    char path[MAX_PATHBUF];
    char hash[48];
    unsigned int size;
} ClassicSoundAsset;

static unsigned long long classic_file_size_bytes(const char *path) {
    FILE *f = fopen(path, "rb");
    long n;
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    n = ftell(f);
    fclose(f);
    return n > 0 ? (unsigned long long)n : 0ULL;
}

static int classic_sound_asset_path_ok(const char *path) {
    size_t n;
    if (!path) return 0;
    /* b1.0 uses the legacy sound/ tree for effects.  Do not mirror songs
       from music/ or records/, and do not download the newer duplicate
       sounds/ layout from the same legacy index. */
    if (strncmp(path, "sound/", 6)) return 0;
    if (!strncmp(path, "music/", 6) || !strncmp(path, "records/", 8)) return 0;
    n = strlen(path);
    if (n < 5 || strcmp(path + n - 4, ".ogg")) return 0;
    return 1;
}

static const char *classic_json_find_token(const char *p, const char *end, const char *token) {
    size_t tl = strlen(token);
    for (; p && p + tl <= end; ++p) {
        if (!memcmp(p, token, tl)) return p;
    }
    return NULL;
}

static int classic_parse_legacy_sounds(const char *json, size_t len, ClassicSoundAsset **out_assets, int *out_count, unsigned long long *out_size) {
    const char *p = json;
    const char *end = json + len;
    int cap = 0, count = 0;
    unsigned long long total = 0;
    ClassicSoundAsset *assets = NULL;
    if (out_assets) *out_assets = NULL;
    if (out_count) *out_count = 0;
    if (out_size) *out_size = 0;
    if (!json || len == 0) return 0;

    while (p < end) {
        const char *q = strchr(p, '"');
        const char *r;
        char key[MAX_PATHBUF];
        size_t key_len;
        const char *obj_end;
        const char *hash_tok;
        const char *size_tok;
        char hash[48];
        unsigned int sz = 0;
        if (!q || q >= end) break;
        r = q + 1;
        while (r < end && *r != '"') r++;
        if (r >= end) break;
        key_len = (size_t)(r - (q + 1));
        if (key_len == 0 || key_len >= sizeof(key)) { p = r + 1; continue; }
        memcpy(key, q + 1, key_len); key[key_len] = 0;
        p = r + 1;
        if (!classic_sound_asset_path_ok(key)) continue;
        obj_end = strchr(p, '}');
        if (!obj_end || obj_end > end) break;
        hash_tok = classic_json_find_token(p, obj_end, "\"hash\"");
        size_tok = classic_json_find_token(p, obj_end, "\"size\"");
        if (!hash_tok || !size_tok) { p = obj_end + 1; continue; }
        hash_tok = strchr(hash_tok + 6, '"');
        if (!hash_tok || hash_tok >= obj_end) { p = obj_end + 1; continue; }
        hash_tok++;
        if (hash_tok + 40 > obj_end) { p = obj_end + 1; continue; }
        memcpy(hash, hash_tok, 40); hash[40] = 0;
        size_tok = strchr(size_tok + 6, ':');
        if (!size_tok || size_tok >= obj_end) { p = obj_end + 1; continue; }
        size_tok++;
        while (size_tok < obj_end && (*size_tok == ' ' || *size_tok == '\t')) size_tok++;
        sz = (unsigned int)parse_decimal_u64(size_tok);
        if (sz == 0) { p = obj_end + 1; continue; }
        total += sz;
        if (out_assets) {
            if (count >= cap) {
                int new_cap = cap ? cap * 2 : 256;
                ClassicSoundAsset *na = (ClassicSoundAsset *)realloc(assets, (size_t)new_cap * sizeof(ClassicSoundAsset));
                if (!na) { free(assets); return 0; }
                assets = na;
                cap = new_cap;
            }
            snprintf(assets[count].path, sizeof(assets[count].path), "%s", key);
            snprintf(assets[count].hash, sizeof(assets[count].hash), "%s", hash);
            assets[count].size = sz;
        }
        count++;
        p = obj_end + 1;
    }
    if (out_assets) *out_assets = assets;
    else free(assets);
    if (out_count) *out_count = count;
    if (out_size) *out_size = total;
    return count > 0;
}

static int classic_download_url_to_memory(const char *url, char **out_data, size_t *out_len, size_t max_len) {
    HMODULE wininet;
    PxcInternetOpenA pInternetOpenA;
    PxcInternetOpenUrlA pInternetOpenUrlA;
    PxcInternetReadFile pInternetReadFile;
    PxcInternetCloseHandle pInternetCloseHandle;
    PxcHInternet inet = NULL, req = NULL;
    char *buf = NULL;
    size_t len = 0, cap = 0;
    int ok = 0;
    if (out_data) *out_data = NULL;
    if (out_len) *out_len = 0;
    wininet = LoadLibraryA("wininet.dll");
    if (!wininet) return 0;
    pInternetOpenA = (PxcInternetOpenA)GetProcAddress(wininet, "InternetOpenA");
    pInternetOpenUrlA = (PxcInternetOpenUrlA)GetProcAddress(wininet, "InternetOpenUrlA");
    pInternetReadFile = (PxcInternetReadFile)GetProcAddress(wininet, "InternetReadFile");
    pInternetCloseHandle = (PxcInternetCloseHandle)GetProcAddress(wininet, "InternetCloseHandle");
    if (!pInternetOpenA || !pInternetOpenUrlA || !pInternetReadFile || !pInternetCloseHandle) goto done;
    inet = pInternetOpenA("PEXCRAFT", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!inet) goto done;
    req = pInternetOpenUrlA(inet, url, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!req) goto done;
    for (;;) {
        unsigned char tmp[32768];
        DWORD got = 0;
        if (!pInternetReadFile(req, tmp, sizeof(tmp), &got)) goto done;
        if (got == 0) break;
        if (len + got + 1 > max_len) goto done;
        if (len + got + 1 > cap) {
            size_t nc = cap ? cap * 2 : 65536;
            while (nc < len + got + 1) nc *= 2;
            char *nb = (char *)realloc(buf, nc);
            if (!nb) goto done;
            buf = nb; cap = nc;
        }
        memcpy(buf + len, tmp, got);
        len += got;
    }
    if (!buf) goto done;
    buf[len] = 0;
    ok = 1;
done:
    if (req) pInternetCloseHandle(req);
    if (inet) pInternetCloseHandle(inet);
    FreeLibrary(wininet);
    if (!ok) { free(buf); return 0; }
    if (out_data) *out_data = buf; else free(buf);
    if (out_len) *out_len = len;
    return 1;
}

static int classic_download_url_to_file_simple(const char *url, const char *path, unsigned int expect_size) {
    HMODULE wininet = LoadLibraryA("wininet.dll");
    PxcInternetOpenA pInternetOpenA;
    PxcInternetOpenUrlA pInternetOpenUrlA;
    PxcInternetReadFile pInternetReadFile;
    PxcInternetCloseHandle pInternetCloseHandle;
    PxcHInternet inet = NULL, req = NULL;
    FILE *out = NULL;
    unsigned long long written = 0;
    int ok = 0;
    if (!wininet) return 0;
    pInternetOpenA = (PxcInternetOpenA)GetProcAddress(wininet, "InternetOpenA");
    pInternetOpenUrlA = (PxcInternetOpenUrlA)GetProcAddress(wininet, "InternetOpenUrlA");
    pInternetReadFile = (PxcInternetReadFile)GetProcAddress(wininet, "InternetReadFile");
    pInternetCloseHandle = (PxcInternetCloseHandle)GetProcAddress(wininet, "InternetCloseHandle");
    if (!pInternetOpenA || !pInternetOpenUrlA || !pInternetReadFile || !pInternetCloseHandle) goto done;
    pxc_mkdirs_for_file(path);
    out = fopen(path, "wb");
    if (!out) goto done;
    inet = pInternetOpenA("PEXCRAFT", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!inet) goto done;
    req = pInternetOpenUrlA(inet, url, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!req) goto done;
    for (;;) {
        unsigned char tmp[32768];
        DWORD got = 0;
        if (!pInternetReadFile(req, tmp, sizeof(tmp), &got)) goto done;
        if (got == 0) break;
        if (fwrite(tmp, 1, got, out) != got) goto done;
        written += got;
    }
    ok = (written > 0 && (!expect_size || written == (unsigned long long)expect_size));
done:
    if (out) fclose(out);
    if (req) pInternetCloseHandle(req);
    if (inet) pInternetCloseHandle(inet);
    FreeLibrary(wininet);
    if (!ok) DeleteFileA(path);
    return ok;
}

static int classic_query_sound_index_size(unsigned long long *out_bytes, int *out_count) {
    char *json = NULL;
    size_t len = 0;
    unsigned long long total = 0;
    int count = 0;
    int ok = 0;
    if (out_bytes) *out_bytes = 0;
    if (out_count) *out_count = 0;
    if (!classic_download_url_to_memory(CLASSIC_SOUNDS_INDEX_URL, &json, &len, 2u * 1024u * 1024u)) return 0;
    ok = classic_parse_legacy_sounds(json, len, NULL, &count, &total);
    free(json);
    if (!ok) return 0;
    if (out_bytes) *out_bytes = total;
    if (out_count) *out_count = count;
    return 1;
}

static int classic_download_legacy_sounds(void) {
    char *json = NULL;
    size_t len = 0;
    ClassicSoundAsset *assets = NULL;
    int count = 0, downloaded = 0;
    unsigned long long total = 0;
    char root[MAX_PATHBUF];
    char marker[MAX_PATHBUF];
    int ok = 0;

    classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading sound index...");
    if (!classic_download_url_to_memory(CLASSIC_SOUNDS_INDEX_URL, &json, &len, 2u * 1024u * 1024u)) {
        classic_install_fail("Could not download legacy sound index");
        return 0;
    }
    if (!classic_parse_legacy_sounds(json, len, &assets, &count, &total)) {
        free(json);
        classic_install_fail("Could not parse legacy sound index");
        return 0;
    }
    free(json);

    classic_resources_path(root, sizeof(root));
    ensure_dir(root);

    for (int i = 0; i < count; ++i) {
        char out_path[MAX_PATHBUF];
        char url[256];
        int pct;
        char st[MAX_LABEL];
        pxc_zip_make_output_path(out_path, sizeof(out_path), root, assets[i].path);
        if (classic_file_size_bytes(out_path) == assets[i].size) {
            downloaded++;
        } else {
            snprintf(url, sizeof(url), "%s/%.2s/%s", CLASSIC_SOUND_OBJECT_URL_PREFIX, assets[i].hash, assets[i].hash);
            pct = 5 + (int)(((unsigned long long)i * 90ULL) / (unsigned long long)(count > 0 ? count : 1));
            snprintf(st, sizeof(st), "Downloading sounds %d/%d", i + 1, count);
            classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, pct, st);
            if (!classic_download_url_to_file_simple(url, out_path, assets[i].size)) {
                free(assets);
                classic_install_fail("Could not download a b1.0 sound");
                return 0;
            }
            downloaded++;
        }
    }

    classic_sound_marker_path(marker, sizeof(marker));
    {
        char text[160];
        snprintf(text, sizeof(text), "PexCraft b1.0 legacy sounds\nfiles:%d\nbytes:%llu\n", downloaded, total);
        ok = pxc_write_file_all(marker, (const unsigned char *)text, strlen(text));
    }
    free(assets);
    if (!ok) { classic_install_fail("Could not write sound install marker"); return 0; }
    log_msg("Installed Minecraft b1.0 legacy sounds: %d files, %llu bytes", downloaded, total);
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
    int need_textures = !classic_pack_installed() || classic_pack_missing_required_textures();
    int need_sounds = classic_wants_sound_download() && !classic_sounds_installed();
    int got_any = 0;

    InterlockedExchange(&g_classic_texture_download_size_bytes, need_textures ? 0 : -1);
    InterlockedExchange(&g_classic_sound_download_size_bytes, need_sounds ? 0 : -1);
    InterlockedExchange(&g_classic_sound_download_count, 0);

    if (need_textures) {
        unsigned long long bytes = 0;
        if (classic_query_download_size_bytes(CLASSIC_PACK_URL, &bytes) && bytes > 0 && bytes <= 0x7fffffffULL) {
            InterlockedExchange(&g_classic_texture_download_size_bytes, (LONG)bytes);
            got_any = 1;
            log_msg("Minecraft Classic texture download size: %llu bytes", bytes);
        }
    } else {
        got_any = 1;
    }

    if (need_sounds) {
        unsigned long long sound_bytes = 0;
        int sound_count = 0;
        if (classic_query_sound_index_size(&sound_bytes, &sound_count) && sound_bytes > 0 && sound_bytes <= 0x7fffffffULL) {
            InterlockedExchange(&g_classic_sound_download_size_bytes, (LONG)sound_bytes);
            InterlockedExchange(&g_classic_sound_download_count, (LONG)sound_count);
            got_any = 1;
            log_msg("Minecraft b1.0 sound download size: %llu bytes (%d files)", sound_bytes, sound_count);
        }
    } else {
        got_any = 1;
    }

    InterlockedExchange(&g_classic_download_size_state, got_any ? CLASSIC_SIZE_READY : CLASSIC_SIZE_ERROR);
    if (!got_any) log_msg("Could not query Minecraft Classic resource sizes");
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
        LONG tex = InterlockedCompareExchange(&g_classic_texture_download_size_bytes, 0, 0);
        LONG snd = InterlockedCompareExchange(&g_classic_sound_download_size_bytes, 0, 0);
        LONG cnt = InterlockedCompareExchange(&g_classic_sound_download_count, 0, 0);
        char tex_part[64];
        char snd_part[96];
        if (tex < 0) snprintf(tex_part, sizeof(tex_part), "Textures: installed");
        else if (tex > 0) snprintf(tex_part, sizeof(tex_part), "Textures: %.2f MB", (double)tex / (1024.0 * 1024.0));
        else snprintf(tex_part, sizeof(tex_part), "Textures: unavailable");

#if PEX_CLASSIC_SOUND_DOWNLOAD_SUPPORTED
        if (!g_opts.download_classic_sounds) snprintf(snd_part, sizeof(snd_part), "Sounds: off");
        else if (classic_sounds_installed()) snprintf(snd_part, sizeof(snd_part), "Sounds: installed");
        else if (g_opts.ignore_classic_sounds_warning) snprintf(snd_part, sizeof(snd_part), "Sounds: ignored");
        else if (snd > 0) snprintf(snd_part, sizeof(snd_part), "Sounds: %.2f MB (%ld files)", (double)snd / (1024.0 * 1024.0), cnt);
        else snprintf(snd_part, sizeof(snd_part), "Sounds: unavailable");
#else
        snprintf(snd_part, sizeof(snd_part), "Sounds: unsupported");
#endif
        snprintf(out, cap, "%s | %s", tex_part, snd_part);
        return;
    } else if (state == CLASSIC_SIZE_FETCHING) {
        snprintf(out, cap, "Download size: checking Mojang...");
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

static int classic_extract_downloaded_pack(const char *zip_path, const char *pack_dir) {
    char err[MAX_LABEL];
    classic_install_set_state(CLASSIC_INSTALL_EXTRACTING, 90, "Extracting textures...");
    err[0] = 0;

    if (!pxc_extract_zip_file(zip_path, pack_dir, err, sizeof(err))) {
        classic_install_fail(err[0] ? err : "Could not extract client.jar internally");
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
    int need_textures = !classic_pack_installed() || classic_pack_missing_required_textures();
    int need_sounds = classic_wants_sound_download() && !classic_sounds_installed();

    ensure_dir(g_texpack_dir);
    classic_pack_path(pack_dir, sizeof(pack_dir));
    snprintf(zip_path, sizeof(zip_path), "%s\\minecraft_classic_client.zip", g_mc_dir);

    if (!need_textures && !need_sounds) {
        classic_install_set_state(CLASSIC_INSTALL_DONE, 100, "Resources already installed");
        return 0;
    }

    if (need_textures) {
        classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading client.jar...");
        if (!classic_download_client_jar(CLASSIC_PACK_URL, zip_path)) return 0;
        if (!classic_extract_downloaded_pack(zip_path, pack_dir)) return 0;
        log_msg("Installed Minecraft Classic texture pack at %s", pack_dir);
    } else {
        classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Textures already installed");
    }

    if (need_sounds) {
        if (!classic_download_legacy_sounds()) return 0;
    }

    classic_install_set_state(CLASSIC_INSTALL_DONE, 100, "Done!");
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
    classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading resources...");
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
        open_notice("Texture Pack", "Could not install Minecraft Classic resources.", g_classic_install_error);
    }
}
