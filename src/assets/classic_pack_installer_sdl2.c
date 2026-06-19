/* Linux/SDL2 Minecraft Classic texture-pack installer.
   Uses libcurl in-process for HTTPS download and the internal ZIP/JAR
   extractor in pxc_zip_extract.c. No curl/unzip/find shell commands. */

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

static volatile LONG g_classic_install_state = CLASSIC_INSTALL_IDLE;
static volatile LONG g_classic_install_progress = 0;
static HANDLE g_classic_install_thread = NULL;
static char g_classic_install_status[MAX_LABEL] = "";
static char g_classic_install_error[MAX_LABEL] = "";

static volatile LONG g_classic_download_size_state = CLASSIC_SIZE_UNKNOWN;
static volatile LONG g_classic_download_size_bytes = 0;

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

static DWORD WINAPI classic_download_size_worker(LPVOID unused) {
    (void)unused;
    CURL *curl;
    curl_off_t clen = -1;
    CURLcode rc;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) {
        InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_ERROR);
        return 0;
    }

    curl_easy_setopt(curl, CURLOPT_URL, CLASSIC_PACK_URL);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PEXCRAFT/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    rc = curl_easy_perform(curl);
    if (rc == CURLE_OK) curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &clen);
    curl_easy_cleanup(curl);

    if (rc == CURLE_OK && clen > 0 && clen <= 0x7fffffff) {
        InterlockedExchange(&g_classic_download_size_bytes, (LONG)clen);
        InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_READY);
        log_msg("Minecraft Classic resource download size: %lld bytes", (long long)clen);
    } else {
        InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_ERROR);
    }
    return 0;
}

static void classic_resource_size_start_fetch(void) {
    LONG old = InterlockedCompareExchange(&g_classic_download_size_state, CLASSIC_SIZE_FETCHING, CLASSIC_SIZE_UNKNOWN);
    if (old != CLASSIC_SIZE_UNKNOWN) return;
    HANDLE th = CreateThread(NULL, 0, classic_download_size_worker, NULL, 0, NULL);
    if (th) CloseHandle(th);
    else InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_ERROR);
}

typedef struct ClassicCurlWriteCtx {
    FILE *f;
    unsigned long long downloaded;
} ClassicCurlWriteCtx;

static size_t classic_curl_write_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    ClassicCurlWriteCtx *ctx = (ClassicCurlWriteCtx *)userdata;
    size_t bytes = size * nmemb;
    if (!ctx || !ctx->f) return 0;
    if (bytes && fwrite(ptr, 1, bytes, ctx->f) != bytes) return 0;
    ctx->downloaded += (unsigned long long)bytes;
    return bytes;
}

#if LIBCURL_VERSION_NUM >= 0x072000
typedef struct ClassicCurlProgressCtx {
    int last_pct;
} ClassicCurlProgressCtx;

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

static int classic_download_client_jar_linux(const char *url, const char *zip_path) {
    CURL *curl;
    CURLcode rc;
    FILE *out;
    ClassicCurlWriteCtx write_ctx;
#if LIBCURL_VERSION_NUM >= 0x072000
    ClassicCurlProgressCtx progress_ctx;
#endif

    DeleteFileA(zip_path);
    out = fopen(zip_path, "wb");
    if (!out) {
        classic_install_fail("Could not create downloaded client file");
        return 0;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) {
        fclose(out);
        classic_install_fail("Could not initialize internal downloader");
        return 0;
    }

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
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, classic_curl_write_cb);
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

    if (rc != CURLE_OK) {
        DeleteFileA(zip_path);
        classic_install_fail(curl_easy_strerror(rc));
        return 0;
    }
    if (write_ctx.downloaded < 1024) {
        DeleteFileA(zip_path);
        classic_install_fail("Downloaded client.jar was empty");
        return 0;
    }
    classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 85, "Downloaded client.jar");
    return 1;
}

static int classic_extract_downloaded_pack_linux(const char *zip_path, const char *pack_dir) {
    char err[MAX_LABEL];
    err[0] = 0;
    classic_install_set_state(CLASSIC_INSTALL_EXTRACTING, 90, "Extracting textures...");

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

    ensure_dir(g_texpack_dir);
    classic_pack_path(pack_dir, sizeof(pack_dir));
    snprintf(zip_path, sizeof(zip_path), "%s/minecraft_classic_client.jar", g_mc_dir);

    classic_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading client.jar...");
    if (!classic_download_client_jar_linux(CLASSIC_PACK_URL, zip_path)) return 0;
    if (!classic_extract_downloaded_pack_linux(zip_path, pack_dir)) return 0;

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
    if (!g_classic_install_thread) classic_install_fail("Could not start installer thread");
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
