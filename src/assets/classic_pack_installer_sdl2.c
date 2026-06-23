/* Linux/SDL2 release resources installer.
   Uses libcurl for HTTPS downloads and the internal ZIP/JAR extractor in
   pxc_zip_extract.c. Textures come from the official Minecraft 1.2.5 client.jar;
   music comes from the official legacy.json asset index, limited to Moog City 2. */

#include <curl/curl.h>

#define CLASSIC_INSTALL_IDLE 0
#define CLASSIC_INSTALL_DOWNLOADING 1
#define CLASSIC_INSTALL_EXTRACTING 2
#define CLASSIC_INSTALL_DONE 3
#define CLASSIC_INSTALL_ERROR 4

#define CLASSIC_SIZE_UNKNOWN 0
#define CLASSIC_SIZE_FETCHING 1
#define CLASSIC_SIZE_READY 2
#define CLASSIC_SIZE_ERROR 3

#define CLASSIC_SOUND_DOWNLOAD_THREADS 16

static volatile LONG g_classic_install_state = CLASSIC_INSTALL_IDLE;
static volatile LONG g_classic_install_progress = 0;
static HANDLE g_classic_install_thread = NULL;
static char g_classic_install_status[MAX_LABEL] = "";
static char g_classic_install_error[MAX_LABEL] = "";

static volatile LONG g_classic_download_size_state = CLASSIC_SIZE_UNKNOWN;
static volatile LONG g_classic_texture_download_size_bytes = 0;
static volatile LONG g_classic_sound_download_size_bytes = 0;
static volatile LONG g_classic_sound_download_count = 0;

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

static LONG classic_atomic_inc(volatile LONG *target) {
#ifdef _WIN32
    return InterlockedIncrement(target);
#else
    return __sync_add_and_fetch(target, 1);
#endif
}

typedef struct ClassicCurlWriteFileCtx {
    FILE *f;
    unsigned long long downloaded;
} ClassicCurlWriteFileCtx;

typedef struct ClassicCurlMemoryCtx {
    char *data;
    size_t len;
    size_t cap;
    size_t max_len;
} ClassicCurlMemoryCtx;

#if LIBCURL_VERSION_NUM >= 0x072000
typedef struct ClassicCurlProgressCtx { int last_pct; } ClassicCurlProgressCtx;
#endif

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

static void classic_install_set_state(LONG state, LONG progress, const char *status) {
    if (status) lstrcpynA(g_classic_install_status, status, sizeof(g_classic_install_status));
    InterlockedExchange(&g_classic_install_progress, progress);
    InterlockedExchange(&g_classic_install_state, state);
}

static void classic_install_fail(const char *msg) {
    lstrcpynA(g_classic_install_error, msg ? msg : "Unknown error", sizeof(g_classic_install_error));
    classic_install_set_state(CLASSIC_INSTALL_ERROR, 0, "Failed");
    log_msg("PexCraft Release resources install failed: %s", g_classic_install_error);
}

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
    if (!path) return 0;
    /* Moog City 2 is the Minecraft 1.2.5 menu track stored in the legacy
       asset index.  In that index it is named sounds/music/menu/menu2.ogg. */
    return !strcmp(path, "sounds/music/menu/menu2.ogg") || !strcmp(path, "music/menu/menu2.ogg");
}

static const char *classic_sound_output_rel(const char *path) {
    if (!path) return "music/menu/menu2.ogg";
    if (!strncmp(path, "sounds/", 7)) return path + 7;
    return path;
}

static const char *classic_json_find_token(const char *p, const char *end, const char *token) {
    size_t tl = strlen(token);
    for (; p && p + tl <= end; ++p) if (!memcmp(p, token, tl)) return p;
    return NULL;
}

static int classic_parse_legacy_sounds(const char *json, size_t len, ClassicSoundAsset **out_assets, int *out_count, unsigned long long *out_size) {
    const char *p = json, *end = json + len;
    int cap = 0, count = 0;
    unsigned long long total = 0;
    ClassicSoundAsset *assets = NULL;
    if (out_assets) *out_assets = NULL;
    if (out_count) *out_count = 0;
    if (out_size) *out_size = 0;
    if (!json || len == 0) return 0;

    while (p < end) {
        const char *q = strchr(p, '"');
        const char *r, *obj_end, *hash_tok, *size_tok;
        char key[MAX_PATHBUF], hash[48];
        size_t key_len;
        unsigned int sz;
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
        sz = (unsigned int)parse_decimal_u64(size_tok + 1);
        if (!sz) { p = obj_end + 1; continue; }
        total += sz;
        if (out_assets) {
            if (count >= cap) {
                int nc = cap ? cap * 2 : 256;
                ClassicSoundAsset *na = (ClassicSoundAsset *)realloc(assets, (size_t)nc * sizeof(ClassicSoundAsset));
                if (!na) { free(assets); return 0; }
                assets = na;
                cap = nc;
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

static size_t classic_curl_write_file_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    ClassicCurlWriteFileCtx *ctx = (ClassicCurlWriteFileCtx *)userdata;
    size_t bytes = size * nmemb;
    if (!ctx || !ctx->f) return 0;
    if (bytes && fwrite(ptr, 1, bytes, ctx->f) != bytes) return 0;
    ctx->downloaded += (unsigned long long)bytes;
    return bytes;
}

static size_t classic_curl_write_memory_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    ClassicCurlMemoryCtx *ctx = (ClassicCurlMemoryCtx *)userdata;
    size_t bytes = size * nmemb;
    if (!ctx) return 0;
    if (ctx->len + bytes + 1 > ctx->max_len) return 0;
    if (ctx->len + bytes + 1 > ctx->cap) {
        size_t nc = ctx->cap ? ctx->cap * 2 : 65536;
        while (nc < ctx->len + bytes + 1) nc *= 2;
        char *nd = (char *)realloc(ctx->data, nc);
        if (!nd) return 0;
        ctx->data = nd;
        ctx->cap = nc;
    }
    if (bytes) memcpy(ctx->data + ctx->len, ptr, bytes);
    ctx->len += bytes;
    ctx->data[ctx->len] = 0;
    return bytes;
}

#if LIBCURL_VERSION_NUM >= 0x072000
static int classic_curl_progress_cb(void *userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    ClassicCurlProgressCtx *ctx = (ClassicCurlProgressCtx *)userdata;
    (void)ultotal; (void)ulnow;
    if (dltotal > 0) {
        int pct = 1 + (int)((dlnow * 84) / dltotal);
        if (pct > 85) pct = 85;
        if (!ctx || pct != ctx->last_pct) {
            char st[MAX_LABEL];
            snprintf(st, sizeof(st), "Downloading client.jar (%d%%)", pct);
            classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, pct, st);
            if (ctx) ctx->last_pct = pct;
        }
    }
    return 0;
}
#endif

static int classic_download_url_to_memory(const char *url, char **out_data, size_t *out_len, size_t max_len) {
    CURL *curl;
    CURLcode rc;
    ClassicCurlMemoryCtx ctx;
    if (out_data) *out_data = NULL;
    if (out_len) *out_len = 0;
    memset(&ctx, 0, sizeof(ctx));
    ctx.max_len = max_len;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) return 0;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PEXCRAFT/1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, classic_curl_write_memory_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);
    rc = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (rc != CURLE_OK || !ctx.data || ctx.len == 0) { free(ctx.data); return 0; }
    if (out_data) *out_data = ctx.data; else free(ctx.data);
    if (out_len) *out_len = ctx.len;
    return 1;
}

static int classic_download_url_to_file_simple(const char *url, const char *path, unsigned int expect_size) {
    CURL *curl;
    CURLcode rc;
    FILE *out;
    ClassicCurlWriteFileCtx ctx;
    pxc_mkdirs_for_file(path);
    out = fopen(path, "wb");
    if (!out) return 0;
    memset(&ctx, 0, sizeof(ctx));
    ctx.f = out;
    curl = curl_easy_init();
    if (!curl) { fclose(out); DeleteFileA(path); return 0; }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PEXCRAFT/1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, classic_curl_write_file_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);
    rc = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(out);
    if (rc != CURLE_OK || ctx.downloaded == 0 || (expect_size && ctx.downloaded != (unsigned long long)expect_size)) {
        DeleteFileA(path);
        return 0;
    }
    return 1;
}

static int classic_query_download_size_bytes(const char *url, unsigned long long *out_bytes) {
    CURL *curl;
    curl_off_t clen = -1;
    CURLcode rc;
    if (out_bytes) *out_bytes = 0;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) return 0;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PEXCRAFT/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    rc = curl_easy_perform(curl);
    if (rc == CURLE_OK) curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &clen);
    curl_easy_cleanup(curl);
    if (rc != CURLE_OK || clen <= 0) return 0;
    if (out_bytes) *out_bytes = (unsigned long long)clen;
    return 1;
}

static int classic_query_sound_index_size(unsigned long long *out_bytes, int *out_count) {
    char *json = NULL;
    size_t len = 0;
    unsigned long long total = 0;
    int count = 0;
    int ok;
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

static void classic_resource_size_format(char *out, size_t cap) {
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
        }
    } else {
        got_any = 1;
    }

    InterlockedExchange(&g_classic_download_size_state, got_any ? CLASSIC_SIZE_READY : CLASSIC_SIZE_ERROR);
    return 0;
}

static void classic_resource_size_start_fetch(void) {
    LONG old = InterlockedCompareExchange(&g_classic_download_size_state, CLASSIC_SIZE_FETCHING, CLASSIC_SIZE_UNKNOWN);
    if (old != CLASSIC_SIZE_UNKNOWN) return;
    HANDLE th = CreateThread(NULL, 0, classic_download_size_worker, NULL, 0, NULL);
    if (th) CloseHandle(th);
    else InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_ERROR);
}

static int classic_download_client_jar_linux(const char *url, const char *zip_path) {
    CURL *curl;
    CURLcode rc;
    FILE *out;
    ClassicCurlWriteFileCtx write_ctx;
#if LIBCURL_VERSION_NUM >= 0x072000
    ClassicCurlProgressCtx progress_ctx;
#endif

    DeleteFileA(zip_path);
    out = fopen(zip_path, "wb");
    if (!out) { classic_install_fail("Could not create downloaded 1.2.5 client file"); return 0; }
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) { fclose(out); classic_install_fail("Could not initialize internal downloader"); return 0; }
    memset(&write_ctx, 0, sizeof(write_ctx));
    write_ctx.f = out;
#if LIBCURL_VERSION_NUM >= 0x072000
    memset(&progress_ctx, 0, sizeof(progress_ctx));
#endif
    classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 1, "Connecting to Mojang...");
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PEXCRAFT/1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, classic_curl_write_file_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_ctx);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);
#if LIBCURL_VERSION_NUM >= 0x072000
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, classic_curl_progress_cb);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress_ctx);
#endif
    rc = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(out);
    if (rc != CURLE_OK) { DeleteFileA(zip_path); classic_install_fail(curl_easy_strerror(rc)); return 0; }
    if (write_ctx.downloaded < 1024) { DeleteFileA(zip_path); classic_install_fail("Downloaded 1.2.5 client.jar was empty"); return 0; }
    classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 85, "Downloaded 1.2.5 client.jar");
    return 1;
}

static int classic_extract_downloaded_pack_linux(const char *zip_path, const char *pack_dir) {
    char err[MAX_LABEL];
    err[0] = 0;
    classic_install_set_state(CLASSIC_INSTALL_EXTRACTING, 90, "Extracting textures...");
    if (!pxc_extract_zip_file(zip_path, pack_dir, err, sizeof(err))) {
        classic_install_fail(err[0] ? err : "Could not extract 1.2.5 client.jar internally");
        return 0;
    }
    if (!classic_pack_installed() || classic_pack_missing_required_textures()) {
        classic_install_fail("Extracted pack is missing required textures");
        return 0;
    }
    DeleteFileA(zip_path);
    return 1;
}

static DWORD WINAPI classic_sound_download_worker(LPVOID arg) {
    ClassicSoundDownloadCtx *ctx = (ClassicSoundDownloadCtx *)arg;
    if (!ctx) return 0;
    for (;;) {
        LONG idx = classic_atomic_inc(&ctx->next_index) - 1;
        ClassicSoundAsset *asset;
        char out_path[MAX_PATHBUF];
        char url[256];
        char st[MAX_LABEL];
        int done;
        int pct;
        if (idx < 0 || idx >= ctx->count) break;
        if (InterlockedCompareExchange(&ctx->failed, 0, 0)) break;
        asset = &ctx->assets[idx];
        pxc_zip_make_output_path(out_path, sizeof(out_path), ctx->root, classic_sound_output_rel(asset->path));
        if (classic_file_size_bytes(out_path) != asset->size) {
            snprintf(url, sizeof(url), "%s/%.2s/%s", CLASSIC_SOUND_OBJECT_URL_PREFIX, asset->hash, asset->hash);
            if (!classic_download_url_to_file_simple(url, out_path, asset->size)) {
                InterlockedExchange(&ctx->failed, 1);
                break;
            }
        }
        done = (int)classic_atomic_inc(&ctx->completed);
        pct = 5 + (int)(((unsigned long long)done * 90ULL) / (unsigned long long)(ctx->count > 0 ? ctx->count : 1));
        if (pct > 95) pct = 95;
        snprintf(st, sizeof(st), "Downloading Moog City 2");
        classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, pct, st);
    }
    return 0;
}

static int classic_download_legacy_sounds(void) {
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

    classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Finding Moog City 2...");
    if (!classic_download_url_to_memory(CLASSIC_SOUNDS_INDEX_URL, &json, &len, 2u * 1024u * 1024u)) {
        classic_install_fail("Could not download legacy asset index");
        return 0;
    }
    if (!classic_parse_legacy_sounds(json, len, &assets, &count, &total)) {
        free(json);
        classic_install_fail("Could not find Moog City 2 in legacy asset index");
        return 0;
    }
    free(json);

    classic_resources_path(root, sizeof(root));
    ensure_dir(root);
    curl_global_init(CURL_GLOBAL_DEFAULT);

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
        snprintf(st, sizeof(st), "Downloading Moog City 2");
        classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 5, st);
    }

    for (int i = 0; i < worker_count; ++i) {
        threads[i] = CreateThread(NULL, 0, classic_sound_download_worker, &ctx, 0, NULL);
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
        classic_install_fail("Could not download Moog City 2");
        return 0;
    }

    classic_sound_marker_path(marker, sizeof(marker));
    {
        char text[160];
        snprintf(text, sizeof(text), "PexCraft Release music\nfile:menu2.ogg\nbytes:%llu\n", total);
        ok = pxc_write_file_all(marker, (const unsigned char *)text, strlen(text));
    }
    free(assets);
    if (!ok) { classic_install_fail("Could not write sound install marker"); return 0; }
    log_msg("Installed Moog City 2 with %d threads: %d files, %llu bytes", worker_count, downloaded, total);
    pex_sound_rescan();
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
    snprintf(zip_path, sizeof(zip_path), "%s/minecraft_1_2_5_client.jar", g_mc_dir);
    if (need_textures) {
        classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading 1.2.5 client.jar...");
        if (!classic_download_client_jar_linux(CLASSIC_PACK_URL, zip_path)) return 0;
        if (!classic_extract_downloaded_pack_linux(zip_path, pack_dir)) return 0;
        log_msg("Installed Minecraft 1.2.5 release texture pack at %s", pack_dir);
    }
    if (need_sounds) {
        if (!classic_download_legacy_sounds()) return 0;
    }
    classic_install_set_state(CLASSIC_INSTALL_DONE, 100, "Done!");
    return 0;
}

static int release_resources_install_blocking(void) {
    char zip_path[MAX_PATHBUF];
    char pack_dir[MAX_PATHBUF];
    int need_textures;
    int need_music;
    ensure_dir(g_texpack_dir);
    classic_pack_path(pack_dir, sizeof(pack_dir));
    snprintf(zip_path, sizeof(zip_path), "%s/minecraft_1_2_5_client.jar", g_mc_dir);

    need_textures = !classic_pack_installed() || classic_pack_missing_required_textures();
    need_music = !classic_sounds_installed();

    if (!need_textures && !need_music) return 1;
    g_classic_install_error[0] = 0;

    if (need_textures) {
        classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading 1.2.5 client.jar...");
        if (!classic_download_client_jar_linux(CLASSIC_PACK_URL, zip_path)) return 0;
        if (!classic_extract_downloaded_pack_linux(zip_path, pack_dir)) return 0;
        log_msg("Installed Minecraft 1.2.5 release texture pack at %s", pack_dir);
    }

    if (need_music) {
        if (!classic_download_legacy_sounds()) return 0;
    }

    classic_install_set_state(CLASSIC_INSTALL_DONE, 100, "Done!");
    return 1;
}

static void start_classic_pack_install(void) {
    LONG state = InterlockedCompareExchange(&g_classic_install_state, CLASSIC_INSTALL_DOWNLOADING, CLASSIC_INSTALL_IDLE);
    if (state == CLASSIC_INSTALL_DOWNLOADING || state == CLASSIC_INSTALL_EXTRACTING) { set_screen(SCREEN_TEXPACK_INSTALL); return; }
    if (g_classic_install_thread) { CloseHandle(g_classic_install_thread); g_classic_install_thread = NULL; }
    g_classic_install_error[0] = 0;
    classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading Release resources...");
    set_screen(SCREEN_TEXPACK_INSTALL);
    g_classic_install_thread = CreateThread(NULL, 0, classic_install_worker, NULL, 0, NULL);
    if (!g_classic_install_thread) classic_install_fail("Could not start installer thread");
}

static void classic_pack_install_tick(void) {
    LONG state = InterlockedCompareExchange(&g_classic_install_state, 0, 0);
    if (state == CLASSIC_INSTALL_DONE) {
        if (g_classic_install_thread) { WaitForSingleObject(g_classic_install_thread, 0); CloseHandle(g_classic_install_thread); g_classic_install_thread = NULL; }
        InterlockedExchange(&g_classic_install_state, CLASSIC_INSTALL_IDLE);
        scan_texture_packs();
        for (int i = 0; i < g_texpack_count; i++) if (g_texpacks[i].is_builtin_classic) { apply_texture_pack_index(i); break; }
        set_screen(SCREEN_TEXPACK);
    } else if (state == CLASSIC_INSTALL_ERROR) {
        if (g_classic_install_thread) { WaitForSingleObject(g_classic_install_thread, 0); CloseHandle(g_classic_install_thread); g_classic_install_thread = NULL; }
        InterlockedExchange(&g_classic_install_state, CLASSIC_INSTALL_IDLE);
        open_notice("Texture Pack", "Could not install Release resources.", g_classic_install_error);
    }
}
