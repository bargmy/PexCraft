/* Split from original monolithic main.c. Included by src/main.c unity build. */

#define CLASSIC_INSTALL_IDLE 0
#define CLASSIC_INSTALL_DOWNLOADING 1
#define CLASSIC_INSTALL_EXTRACTING 2
#define CLASSIC_INSTALL_DONE 3
#define CLASSIC_INSTALL_ERROR 4

static volatile LONG g_classic_install_state = CLASSIC_INSTALL_IDLE;
static volatile LONG g_classic_install_progress = 0;
static volatile LONG g_classic_install_cancel_requested = 0;
static HANDLE g_classic_install_thread = NULL;
static char g_classic_install_status[MAX_LABEL] = "";
static char g_classic_install_error[MAX_LABEL] = "";

#if defined(PEX_PLATFORM_XBOX_UWP)
extern int pex_xbox_uwp_http_download_to_memory(const char *url, char **out_data, size_t *out_len, size_t max_len, volatile LONG *cancel_flag);
extern int pex_xbox_uwp_http_download_to_file(const char *url, const char *path, unsigned int expect_size, volatile LONG *cancel_flag, volatile LONG *progress_out, volatile LONG *total_out);
#endif

static void pack_install_reset_cancel(void) {
    InterlockedExchange(&g_classic_install_cancel_requested, 0);
}

static void pack_install_request_cancel(void) {
    InterlockedExchange(&g_classic_install_cancel_requested, 1);
    log_msg("Release resource download cancel requested");
}

static int pack_install_is_cancelled(void) {
    return InterlockedCompareExchange(&g_classic_install_cancel_requested, 0, 0) != 0;
}

static void log_windows_error(const char *where) {
    DWORD err = GetLastError();
    char text[512];
    text[0] = 0;
    if (err) {
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, 0, text, sizeof(text), NULL);
        for (char *p = text; *p; ++p) if (*p == '\r' || *p == '\n') *p = ' ';
    }
    log_msg("WinINet/resource error at %s: code=%lu%s%s", where ? where : "unknown",
            (unsigned long)err, text[0] ? " message=" : "", text);
}

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
#ifndef INTERNET_FLAG_KEEP_CONNECTION
#define INTERNET_FLAG_KEEP_CONNECTION 0x00400000u
#endif
#define PEX_WININET_DOWNLOAD_FLAGS (INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_SECURE | INTERNET_FLAG_KEEP_CONNECTION)

#define CLASSIC_SIZE_UNKNOWN 0
#define CLASSIC_SIZE_FETCHING 1
#define CLASSIC_SIZE_READY 2
#define CLASSIC_SIZE_ERROR 3

#define CLASSIC_SOUND_DOWNLOAD_THREADS 16

static volatile LONG g_classic_download_size_state = CLASSIC_SIZE_UNKNOWN;
static volatile LONG g_classic_texture_download_size_bytes = 0;
static volatile LONG g_classic_sound_download_size_bytes = 0;
static volatile LONG g_classic_sound_download_count = 0;

static void pack_install_set_state(LONG state, LONG progress, const char *status);
static void pack_install_fail(const char *msg);

static unsigned long long parse_u64_decimal(const char *s) {
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

static int http_split_url(const char *url, int *secure, char *host, size_t host_cap, char *path, size_t path_cap) {
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

static int http_get_content_length(PxcHInternet req, PxcHttpQueryInfoA pHttpQueryInfoA, unsigned long long *out_bytes) {
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
            value = parse_u64_decimal(text);
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

typedef struct ClassicSoundDownloadCtx {
    ClassicSoundAsset *assets;
    int count;
    char root[MAX_PATHBUF];
    volatile LONG next_index;
    volatile LONG completed;
    volatile LONG failed;
} ClassicSoundDownloadCtx;

static LONG atomic_increment_int(volatile LONG *target) {
#ifdef _WIN32
    return InterlockedIncrement(target);
#else
    return __sync_add_and_fetch(target, 1);
#endif
}

static unsigned long long file_size_bytes(const char *path) {
    FILE *f = fopen(path, "rb");
    long n;
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    n = ftell(f);
    fclose(f);
    return n > 0 ? (unsigned long long)n : 0ULL;
}

static int legacy_sound_path_is_valid(const char *path) {
    if (!path) return 0;
    /* User-requested release port: download only Moog City 2 from the legacy
       asset index.  In that index it is named sounds/music/menu/menu2.ogg. */
    return !strcmp(path, "sounds/music/menu/menu2.ogg") || !strcmp(path, "music/menu/menu2.ogg");
}

static const char *legacy_sound_output_path(const char *path) {
    if (!path) return "";
    if (!strncmp(path, "sounds/", 7)) return path + 7;
    return path;
}

static const char *json_find_token(const char *p, const char *end, const char *token) {
    size_t tl = strlen(token);
    for (; p && p + tl <= end; ++p) {
        if (!memcmp(p, token, tl)) return p;
    }
    return NULL;
}

static int legacy_sound_parse_index(const char *json, size_t len, ClassicSoundAsset **out_assets, int *out_count, unsigned long long *out_size) {
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
        if (!legacy_sound_path_is_valid(key)) continue;
        obj_end = strchr(p, '}');
        if (!obj_end || obj_end > end) break;
        hash_tok = json_find_token(p, obj_end, "\"hash\"");
        size_tok = json_find_token(p, obj_end, "\"size\"");
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
        sz = (unsigned int)parse_u64_decimal(size_tok);
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

static int http_download_to_memory(const char *url, char **out_data, size_t *out_len, size_t max_len) {
#if defined(PEX_PLATFORM_XBOX_UWP)
    return pex_xbox_uwp_http_download_to_memory(url, out_data, out_len, max_len, &g_classic_install_cancel_requested);
#else
    HMODULE wininet;
    PxcInternetOpenA pInternetOpenA;
    PxcInternetOpenUrlA pInternetOpenUrlA;
    PxcInternetReadFile pInternetReadFile;
    PxcInternetCloseHandle pInternetCloseHandle;
    PxcHInternet inet = NULL, req = NULL;
    char *buf = NULL;
    size_t len = 0, cap = 0;
    int ok = 0;
    const char *fail_where = NULL;
    if (out_data) *out_data = NULL;
    if (out_len) *out_len = 0;
    wininet = LoadLibraryA("wininet.dll");
    if (!wininet) { log_msg("Download failed before opening %s: wininet.dll missing", url ? url : "(null)"); return 0; }
    pInternetOpenA = (PxcInternetOpenA)GetProcAddress(wininet, "InternetOpenA");
    pInternetOpenUrlA = (PxcInternetOpenUrlA)GetProcAddress(wininet, "InternetOpenUrlA");
    pInternetReadFile = (PxcInternetReadFile)GetProcAddress(wininet, "InternetReadFile");
    pInternetCloseHandle = (PxcInternetCloseHandle)GetProcAddress(wininet, "InternetCloseHandle");
    if (!pInternetOpenA || !pInternetOpenUrlA || !pInternetReadFile || !pInternetCloseHandle) { fail_where = "resolve WinINet entry points"; goto done; }
    if (pack_install_is_cancelled()) { fail_where = "cancel before open memory download"; goto done; }
    inet = pInternetOpenA("PEXCRAFT", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!inet) { fail_where = "InternetOpenA memory"; goto done; }
    req = pInternetOpenUrlA(inet, url, NULL, 0, PEX_WININET_DOWNLOAD_FLAGS, 0);
    if (!req) { fail_where = "InternetOpenUrlA memory"; goto done; }
    for (;;) {
        unsigned char tmp[32768];
        DWORD got = 0;
        if (pack_install_is_cancelled()) { fail_where = "cancel memory download"; goto done; }
        if (!pInternetReadFile(req, tmp, sizeof(tmp), &got)) { fail_where = "InternetReadFile memory"; goto done; }
        if (got == 0) break;
        if (len + got + 1 > max_len) { fail_where = "memory download too large"; goto done; }
        if (len + got + 1 > cap) {
            size_t nc = cap ? cap * 2 : 65536;
            while (nc < len + got + 1) nc *= 2;
            char *nb = (char *)realloc(buf, nc);
            if (!nb) { fail_where = "allocate memory download buffer"; goto done; }
            buf = nb; cap = nc;
        }
        memcpy(buf + len, tmp, got);
        len += got;
    }
    if (!buf) { fail_where = "empty memory download"; goto done; }
    buf[len] = 0;
    ok = 1;
done:
    if (!ok && fail_where) log_windows_error(fail_where);
    if (!ok) log_msg("Download to memory failed: url=%s bytes=%u", url ? url : "(null)", (unsigned)len);
    if (req) pInternetCloseHandle(req);
    if (inet) pInternetCloseHandle(inet);
    FreeLibrary(wininet);
    if (!ok) { free(buf); return 0; }
    if (out_data) *out_data = buf; else free(buf);
    if (out_len) *out_len = len;
    return 1;
#endif
}

static int http_download_to_file(const char *url, const char *path, unsigned int expect_size) {
#if defined(PEX_PLATFORM_XBOX_UWP)
    volatile LONG dummy_progress = 0;
    volatile LONG dummy_total = 0;
    return pex_xbox_uwp_http_download_to_file(url, path, expect_size, &g_classic_install_cancel_requested, &dummy_progress, &dummy_total);
#else
    HMODULE wininet = LoadLibraryA("wininet.dll");
    PxcInternetOpenA pInternetOpenA;
    PxcInternetOpenUrlA pInternetOpenUrlA;
    PxcInternetReadFile pInternetReadFile;
    PxcInternetCloseHandle pInternetCloseHandle;
    PxcHInternet inet = NULL, req = NULL;
    FILE *out = NULL;
    unsigned long long written = 0;
    int ok = 0;
    const char *fail_where = NULL;
    if (!wininet) { log_msg("Download failed before opening %s: wininet.dll missing", url ? url : "(null)"); return 0; }
    pInternetOpenA = (PxcInternetOpenA)GetProcAddress(wininet, "InternetOpenA");
    pInternetOpenUrlA = (PxcInternetOpenUrlA)GetProcAddress(wininet, "InternetOpenUrlA");
    pInternetReadFile = (PxcInternetReadFile)GetProcAddress(wininet, "InternetReadFile");
    pInternetCloseHandle = (PxcInternetCloseHandle)GetProcAddress(wininet, "InternetCloseHandle");
    if (!pInternetOpenA || !pInternetOpenUrlA || !pInternetReadFile || !pInternetCloseHandle) { fail_where = "resolve WinINet entry points file"; goto done; }
    if (pack_install_is_cancelled()) { fail_where = "cancel before file download"; goto done; }
    pxc_mkdirs_for_file(path);
    out = fopen(path, "wb");
    if (!out) { fail_where = "open output file"; goto done; }
    inet = pInternetOpenA("PEXCRAFT", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!inet) { fail_where = "InternetOpenA file"; goto done; }
    req = pInternetOpenUrlA(inet, url, NULL, 0, PEX_WININET_DOWNLOAD_FLAGS, 0);
    if (!req) { fail_where = "InternetOpenUrlA file"; goto done; }
    for (;;) {
        unsigned char tmp[32768];
        DWORD got = 0;
        if (pack_install_is_cancelled()) { fail_where = "cancel file download"; goto done; }
        if (!pInternetReadFile(req, tmp, sizeof(tmp), &got)) { fail_where = "InternetReadFile file"; goto done; }
        if (got == 0) break;
        if (fwrite(tmp, 1, got, out) != got) { fail_where = "write output file"; goto done; }
        written += got;
    }
    ok = (written > 0 && (!expect_size || written == (unsigned long long)expect_size));
    if (!ok) fail_where = "downloaded file size mismatch";
done:
    if (!ok && fail_where) log_windows_error(fail_where);
    if (!ok) log_msg("Download to file failed: url=%s path=%s bytes=%llu expected=%u", url ? url : "(null)", path ? path : "(null)", written, expect_size);
    if (out) fclose(out);
    if (req) pInternetCloseHandle(req);
    if (inet) pInternetCloseHandle(inet);
    FreeLibrary(wininet);
    if (!ok) DeleteFileA(path);
    return ok;
#endif
}

static int legacy_sound_index_download_size(unsigned long long *out_bytes, int *out_count) {
    char *json = NULL;
    size_t len = 0;
    unsigned long long total = 0;
    int count = 0;
    int ok = 0;
    if (out_bytes) *out_bytes = 0;
    if (out_count) *out_count = 0;
    if (!http_download_to_memory(CLASSIC_SOUNDS_INDEX_URL, &json, &len, 2u * 1024u * 1024u)) return 0;
    ok = legacy_sound_parse_index(json, len, NULL, &count, &total);
    free(json);
    if (!ok) return 0;
    if (out_bytes) *out_bytes = total;
    if (out_count) *out_count = count;
    return 1;
}

static DWORD WINAPI legacy_sound_download_worker(LPVOID arg) {
    ClassicSoundDownloadCtx *ctx = (ClassicSoundDownloadCtx *)arg;
    if (!ctx) return 0;
    for (;;) {
        LONG idx = atomic_increment_int(&ctx->next_index) - 1;
        ClassicSoundAsset *asset;
        char out_path[MAX_PATHBUF];
        char url[256];
        char st[MAX_LABEL];
        int done;
        int pct;
        if (idx < 0 || idx >= ctx->count) break;
        if (InterlockedCompareExchange(&ctx->failed, 0, 0)) break;
        if (pack_install_is_cancelled()) { InterlockedExchange(&ctx->failed, 1); break; }
        asset = &ctx->assets[idx];
        pxc_zip_make_output_path(out_path, sizeof(out_path), ctx->root, legacy_sound_output_path(asset->path));
        if (file_size_bytes(out_path) != asset->size) {
            snprintf(url, sizeof(url), "%s/%.2s/%s", CLASSIC_SOUND_OBJECT_URL_PREFIX, asset->hash, asset->hash);
            if (!http_download_to_file(url, out_path, asset->size)) {
                InterlockedExchange(&ctx->failed, 1);
                break;
            }
        }
        done = (int)atomic_increment_int(&ctx->completed);
        pct = 5 + (int)(((unsigned long long)done * 90ULL) / (unsigned long long)(ctx->count > 0 ? ctx->count : 1));
        if (pct > 95) pct = 95;
        snprintf(st, sizeof(st), "Downloading Moog City 2 %d/%d", done, ctx->count);
        pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, pct, st);
    }
    return 0;
}

static int legacy_sound_download_all(void) {
    char *json = NULL;
    size_t len = 0;
    ClassicSoundAsset *assets = NULL;
    int count = 0;
    int downloaded = 0;
    int worker_count = 0;
    int started = 0;
    unsigned long long total = 0;
    char root[MAX_PATHBUF];
    char marker[MAX_PATHBUF];
    int ok = 0;
    HANDLE threads[CLASSIC_SOUND_DOWNLOAD_THREADS];
    ClassicSoundDownloadCtx ctx;

    memset(threads, 0, sizeof(threads));
    memset(&ctx, 0, sizeof(ctx));

    pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading release music index...");
    if (!http_download_to_memory(CLASSIC_SOUNDS_INDEX_URL, &json, &len, 2u * 1024u * 1024u)) {
        pack_install_fail("Could not download release music index");
        return 0;
    }
    if (!legacy_sound_parse_index(json, len, &assets, &count, &total)) {
        free(json);
        pack_install_fail("Could not find Moog City 2 in release music index");
        return 0;
    }
    free(json);

    classic_resources_path(root, sizeof(root));
    ensure_dir(root);

    ctx.assets = assets;
    ctx.count = count;
    snprintf(ctx.root, sizeof(ctx.root), "%s", root);
    InterlockedExchange(&ctx.next_index, 0);
    InterlockedExchange(&ctx.completed, 0);
    InterlockedExchange(&ctx.failed, 0);

    worker_count = count < CLASSIC_SOUND_DOWNLOAD_THREADS ? count : CLASSIC_SOUND_DOWNLOAD_THREADS;
    if (worker_count < 1) worker_count = 1;
    {
        char st[MAX_LABEL];
        snprintf(st, sizeof(st), "Downloading Moog City 2 0/%d", count);
        pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 5, st);
    }

    for (int i = 0; i < worker_count; ++i) {
        threads[i] = CreateThread(NULL, 0, legacy_sound_download_worker, &ctx, 0, NULL);
        if (!threads[i]) {
            InterlockedExchange(&ctx.failed, 1);
            break;
        }
        started++;
    }

    for (int i = 0; i < started; ++i) {
        WaitForSingleObject(threads[i], INFINITE);
        CloseHandle(threads[i]);
    }

    downloaded = (int)InterlockedCompareExchange(&ctx.completed, 0, 0);
    if (InterlockedCompareExchange(&ctx.failed, 0, 0) || downloaded < count) {
        free(assets);
        if (pack_install_is_cancelled()) pack_install_fail("Download cancelled");
        else pack_install_fail("Could not download Moog City 2");
        return 0;
    }

    classic_sound_marker_path(marker, sizeof(marker));
    {
        char text[160];
        snprintf(text, sizeof(text), "PexCraft Release music\nfile:menu2.ogg\nbytes:%llu\n", total);
        ok = pxc_write_file_all(marker, (const unsigned char *)text, strlen(text));
    }
    free(assets);
    if (!ok) { pack_install_fail("Could not write sound install marker"); return 0; }
    log_msg("Installed Moog City 2 with %d threads: %d file, %llu bytes", worker_count, downloaded, total);
    pex_sound_rescan();
    return 1;
}


static int pack_install_query_size_bytes(const char *url, unsigned long long *out_bytes) {
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
        if (http_split_url(url, &secure, host, sizeof(host), path, sizeof(path))) {
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
                        ok = http_get_content_length(req, pHttpQueryInfoA, &value);
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
            PEX_WININET_DOWNLOAD_FLAGS, 0);
        if (req) {
            ok = http_get_content_length(req, pHttpQueryInfoA, &value);
            pInternetCloseHandle(req);
        }
    }

    pInternetCloseHandle(inet);
    FreeLibrary(wininet);

    if (!ok || value == 0) return 0;
    if (out_bytes) *out_bytes = value;
    return 1;
}

static DWORD WINAPI pack_install_size_worker(LPVOID unused) {
    (void)unused;
    int need_textures = !pack_is_installed() || pack_missing_required_textures();
    int need_sounds = classic_wants_sound_download() && !classic_sounds_installed();
    int got_any = 0;

    InterlockedExchange(&g_classic_texture_download_size_bytes, need_textures ? 0 : -1);
    InterlockedExchange(&g_classic_sound_download_size_bytes, need_sounds ? 0 : -1);
    InterlockedExchange(&g_classic_sound_download_count, 0);

    if (need_textures) {
        unsigned long long bytes = 0;
        if (pack_install_query_size_bytes(CLASSIC_PACK_URL, &bytes) && bytes > 0 && bytes <= 0x7fffffffULL) {
            InterlockedExchange(&g_classic_texture_download_size_bytes, (LONG)bytes);
            got_any = 1;
            log_msg("Release texture download size: %llu bytes", bytes);
        }
    } else {
        got_any = 1;
    }

    if (need_sounds) {
        unsigned long long sound_bytes = 0;
        int sound_count = 0;
        if (legacy_sound_index_download_size(&sound_bytes, &sound_count) && sound_bytes > 0 && sound_bytes <= 0x7fffffffULL) {
            InterlockedExchange(&g_classic_sound_download_size_bytes, (LONG)sound_bytes);
            InterlockedExchange(&g_classic_sound_download_count, (LONG)sound_count);
            got_any = 1;
            log_msg("Moog City 2 download size: %llu bytes (%d files)", sound_bytes, sound_count);
        }
    } else {
        got_any = 1;
    }

    InterlockedExchange(&g_classic_download_size_state, got_any ? CLASSIC_SIZE_READY : CLASSIC_SIZE_ERROR);
    if (!got_any) log_msg("Could not query Release resource sizes");
    return 0;
}

static void pack_install_start_size_fetch(void) {
    LONG old = InterlockedCompareExchange(&g_classic_download_size_state, CLASSIC_SIZE_FETCHING, CLASSIC_SIZE_UNKNOWN);
    if (old != CLASSIC_SIZE_UNKNOWN) return;
    HANDLE th = CreateThread(NULL, 0, pack_install_size_worker, NULL, 0, NULL);
    if (th) {
        CloseHandle(th);
    } else {
        InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_ERROR);
    }
}

static void format_download_size(char *out, size_t cap) {
    LONG state = InterlockedCompareExchange(&g_classic_download_size_state, 0, 0);
    if (state == CLASSIC_SIZE_READY) {
        LONG tex = InterlockedCompareExchange(&g_classic_texture_download_size_bytes, 0, 0);
        LONG snd = InterlockedCompareExchange(&g_classic_sound_download_size_bytes, 0, 0);
        char tex_part[64];
        char snd_part[96];
        if (tex < 0) snprintf(tex_part, sizeof(tex_part), "Textures: installed");
        else if (tex > 0) snprintf(tex_part, sizeof(tex_part), "Textures: %.2f MB", (double)tex / (1024.0 * 1024.0));
        else snprintf(tex_part, sizeof(tex_part), "Textures: unavailable");

#if PEX_CLASSIC_SOUND_DOWNLOAD_SUPPORTED
        if (classic_sounds_installed()) snprintf(snd_part, sizeof(snd_part), "Moog City 2: installed");
        else if (snd > 0) snprintf(snd_part, sizeof(snd_part), "Moog City 2: %.2f MB", (double)snd / (1024.0 * 1024.0));
        else snprintf(snd_part, sizeof(snd_part), "Moog City 2: unavailable");
#else
        snprintf(snd_part, sizeof(snd_part), "Moog City 2: unsupported");
#endif
        snprintf(out, cap, "%s | %s", tex_part, snd_part);
        return;
    } else if (state == CLASSIC_SIZE_FETCHING) {
        snprintf(out, cap, "Download size: checking Mojang...");
        return;
    }
    snprintf(out, cap, "Download size: unavailable");
}

static void pack_install_set_state(LONG state, LONG progress, const char *status) {
    if (status) lstrcpynA(g_classic_install_status, status, sizeof(g_classic_install_status));
    InterlockedExchange(&g_classic_install_progress, progress);
    InterlockedExchange(&g_classic_install_state, state);
}

static void pack_install_fail(const char *msg) {
    lstrcpynA(g_classic_install_error, msg ? msg : "Unknown error", sizeof(g_classic_install_error));
    pack_install_set_state(CLASSIC_INSTALL_ERROR, 0, "Failed");
    log_msg("Release resource install failed: %s", g_classic_install_error);
}

static int pack_install_download_client_jar(const char *url, const char *zip_path) {
#if defined(PEX_PLATFORM_XBOX_UWP)
    if (pack_install_is_cancelled()) { pack_install_fail("Download cancelled"); return 0; }
    DeleteFileA(zip_path);
    pxc_mkdirs_for_file(zip_path);
    pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 1, "Connecting to Mojang...");
    volatile LONG total = 0;
    if (!pex_xbox_uwp_http_download_to_file(url, zip_path, 0, &g_classic_install_cancel_requested, &g_classic_install_progress, &total)) {
        pack_install_fail(pack_install_is_cancelled() ? "Download cancelled" : "Could not download Release resources");
        return 0;
    }
    if (pack_install_is_cancelled()) { DeleteFileA(zip_path); pack_install_fail("Download cancelled"); return 0; }
    pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 99, "Download complete");
    return 1;
#else
    HMODULE wininet = LoadLibraryA("wininet.dll");
    PxcInternetOpenA pInternetOpenA;
    PxcInternetOpenUrlA pInternetOpenUrlA;
    PxcInternetReadFile pInternetReadFile;
    PxcInternetCloseHandle pInternetCloseHandle;
    PxcHttpQueryInfoA pHttpQueryInfoA;
    PxcHInternet inet = NULL;
    PxcHInternet req = NULL;
    FILE *out = NULL;
    DWORD total = 0;
    DWORD downloaded = 0;
    int ok = 0;
    const char *fail_where = NULL;

    if (!wininet) { pack_install_fail("wininet.dll was not found"); return 0; }

    pInternetOpenA = (PxcInternetOpenA)GetProcAddress(wininet, "InternetOpenA");
    pInternetOpenUrlA = (PxcInternetOpenUrlA)GetProcAddress(wininet, "InternetOpenUrlA");
    pInternetReadFile = (PxcInternetReadFile)GetProcAddress(wininet, "InternetReadFile");
    pInternetCloseHandle = (PxcInternetCloseHandle)GetProcAddress(wininet, "InternetCloseHandle");
    pHttpQueryInfoA = (PxcHttpQueryInfoA)GetProcAddress(wininet, "HttpQueryInfoA");

    if (!pInternetOpenA || !pInternetOpenUrlA || !pInternetReadFile || !pInternetCloseHandle) {
        FreeLibrary(wininet);
        pack_install_fail("WinINet entry points missing");
        return 0;
    }

    DeleteFileA(zip_path);
    out = fopen(zip_path, "wb");
    if (!out) {
        log_windows_error("create downloaded client.jar file");
        FreeLibrary(wininet);
        pack_install_fail("Could not create downloaded 1.2.5 client file");
        return 0;
    }

    if (pack_install_is_cancelled()) { fail_where = "cancel before client.jar download"; goto done; }
    inet = pInternetOpenA("PEXCRAFT", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!inet) { fail_where = "InternetOpenA client.jar"; goto done; }

    pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 1, "Connecting to Mojang...");
    req = pInternetOpenUrlA(inet, url, NULL, 0, PEX_WININET_DOWNLOAD_FLAGS, 0);
    if (!req) { fail_where = "InternetOpenUrlA client.jar"; goto done; }

    if (pHttpQueryInfoA) {
        DWORD total_len = sizeof(total);
        pHttpQueryInfoA(req, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &total, &total_len, NULL);
    }
    log_msg("Downloading Release textures from %s to %s; expected bytes=%lu", url ? url : "(null)", zip_path ? zip_path : "(null)", (unsigned long)total);

    for (;;) {
        unsigned char buf[32768];
        DWORD got = 0;
        if (pack_install_is_cancelled()) { fail_where = "cancel client.jar download"; goto done; }
        if (!pInternetReadFile(req, buf, sizeof(buf), &got)) { fail_where = "InternetReadFile client.jar"; goto done; }
        if (got == 0) break;
        if (fwrite(buf, 1, got, out) != got) { fail_where = "write client.jar download"; goto done; }
        downloaded += got;
        if (total > 0) {
            int pct = 1 + (int)(((unsigned long long)downloaded * 84ULL) / (unsigned long long)total);
            if (pct > 85) pct = 85;
            char st[MAX_LABEL];
            snprintf(st, sizeof(st), "Downloading 1.2.5 client.jar (%d%%)", pct);
            pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, pct, st);
        } else {
            int pct = 5 + (int)((downloaded / 65536) % 70);
            pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, pct, "Downloading 1.2.5 client.jar");
        }
    }

    if (downloaded < 1024) { fail_where = "downloaded client.jar empty"; goto done; }
    ok = 1;

done:
    if (out) { fclose(out); out = NULL; }
    if (!ok && fail_where) log_windows_error(fail_where);
    if (!ok) {
        log_msg("Release texture download failed: url=%s path=%s bytes=%lu expected=%lu", url ? url : "(null)", zip_path ? zip_path : "(null)", (unsigned long)downloaded, (unsigned long)total);
        DeleteFileA(zip_path);
    }
    if (req) pInternetCloseHandle(req);
    if (inet) pInternetCloseHandle(inet);
    FreeLibrary(wininet);

    if (!ok) {
        if (pack_install_is_cancelled()) pack_install_fail("Download cancelled");
        else pack_install_fail("Could not download 1.2.5 client.jar");
        return 0;
    }
    log_msg("Downloaded Release textures client.jar bytes=%lu", (unsigned long)downloaded);
    return 1;
}

static int pack_install_extract_archive(const char *zip_path, const char *pack_dir) {
    char err[MAX_LABEL];
    pack_install_set_state(CLASSIC_INSTALL_EXTRACTING, 90, "Extracting textures...");
    err[0] = 0;

    log_msg("Extracting Release textures from %s into %s", zip_path ? zip_path : "(null)", pack_dir ? pack_dir : "(null)");
    if (!pxc_extract_zip_file(zip_path, pack_dir, err, sizeof(err))) {
        log_msg("Release texture extraction failed: %s", err[0] ? err : "unknown extraction error");
        pack_install_fail(err[0] ? err : "Could not extract 1.2.5 client.jar internally");
        return 0;
    }

    if (!pack_is_installed() || pack_missing_required_textures()) {
        pack_install_fail("Extracted pack is missing required textures");
        return 0;
    }
    DeleteFileA(zip_path);
    return 1;
}

static DWORD WINAPI pack_install_worker(LPVOID unused) {
    (void)unused;
    char zip_path[MAX_PATHBUF];
    char pack_dir[MAX_PATHBUF];
    int need_textures = !pack_is_installed() || pack_missing_required_textures();
    int need_sounds = classic_wants_sound_download() && !classic_sounds_installed();

    pack_install_reset_cancel();
    ensure_dir(g_texpack_dir);
    pack_asset_path(pack_dir, sizeof(pack_dir));
    snprintf(zip_path, sizeof(zip_path), "%s\\minecraft_1_2_5_client.zip", g_mc_dir);

    if (!need_textures && !need_sounds) {
        pack_install_set_state(CLASSIC_INSTALL_DONE, 100, "Resources already installed");
        return 0;
    }

    if (need_textures) {
        pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading 1.2.5 client.jar...");
        if (!pack_install_download_client_jar(CLASSIC_PACK_URL, zip_path)) return 0;
        if (!pack_install_extract_archive(zip_path, pack_dir)) return 0;
        log_msg("Installed Release textures at %s", pack_dir);
    } else {
        pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Release textures already installed");
    }

    if (need_sounds) {
        if (!legacy_sound_download_all()) return 0;
    }

    pack_install_set_state(CLASSIC_INSTALL_DONE, 100, "Done!");
    return 0;
}

static int pack_resources_need_download(void) {
    int need_textures;
    int need_music;
    ensure_dir(g_texpack_dir);
    need_textures = !pack_is_installed() || pack_missing_required_textures();
    need_music = classic_wants_sound_download() && !classic_sounds_installed();
    return need_textures || need_music;
}

static int pack_resources_install_blocking(void) {
    char zip_path[MAX_PATHBUF];
    char pack_dir[MAX_PATHBUF];
    int need_textures;
    int need_music;

    ensure_dir(g_texpack_dir);
    pack_asset_path(pack_dir, sizeof(pack_dir));
    snprintf(zip_path, sizeof(zip_path), "%s\\minecraft_1_2_5_client.zip", g_mc_dir);

    need_textures = !pack_is_installed() || pack_missing_required_textures();
    need_music = classic_wants_sound_download() && !classic_sounds_installed();

    if (!need_textures && !need_music) return 1;

    g_classic_install_error[0] = 0;
    pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading Release resources...");

    if (need_textures) {
        if (!pack_install_download_client_jar(CLASSIC_PACK_URL, zip_path)) return 0;
        if (!pack_install_extract_archive(zip_path, pack_dir)) return 0;
    }
    if (need_music) {
        if (!legacy_sound_download_all()) return 0;
    }

    pack_install_set_state(CLASSIC_INSTALL_DONE, 100, "Release resources ready");
    pex_sound_rescan();
    return 1;
}

static void pack_install_start(void) {
    LONG state = InterlockedCompareExchange(&g_classic_install_state, CLASSIC_INSTALL_DOWNLOADING, CLASSIC_INSTALL_IDLE);
    if (state == CLASSIC_INSTALL_DOWNLOADING || state == CLASSIC_INSTALL_EXTRACTING) {
        set_screen(SCREEN_TEXPACK_INSTALL);
        return;
    }

    if (g_classic_install_thread) {
        CloseHandle(g_classic_install_thread);
        g_classic_install_thread = NULL;
    }

    pack_install_reset_cancel();
    g_classic_install_error[0] = 0;
    pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading resources...");
    set_screen(SCREEN_TEXPACK_INSTALL);
    g_classic_install_thread = CreateThread(NULL, 0, pack_install_worker, NULL, 0, NULL);
    if (!g_classic_install_thread) {
        pack_install_fail("Could not start installer thread");
    }
}

static void pack_install_tick(void) {
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
        open_notice("Texture Pack", "Could not install Release resources.", g_classic_install_error);
    }
}
