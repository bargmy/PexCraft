/* LG webOS Minecraft Classic texture-pack installer.
   Downloads the official Minecraft 1.2.5 client.jar with libcurl, extracts it
   with the shared ZIP extractor, then applies the built-in Release texture pack.
   This avoids the desktop SDL2 installer helpers (python/wget/powershell), which
   are not reliable on TV targets. */

#include <curl/curl.h>

#ifndef CLASSIC_INSTALL_IDLE
#define CLASSIC_INSTALL_IDLE 0
#define CLASSIC_INSTALL_DOWNLOADING 1
#define CLASSIC_INSTALL_EXTRACTING 2
#define CLASSIC_INSTALL_DONE 3
#define CLASSIC_INSTALL_ERROR 4
#endif
#ifndef CLASSIC_SIZE_UNKNOWN
#define CLASSIC_SIZE_UNKNOWN 0
#define CLASSIC_SIZE_FETCHING 1
#define CLASSIC_SIZE_READY 2
#define CLASSIC_SIZE_ERROR 3
#endif

static volatile LONG g_classic_install_state = CLASSIC_INSTALL_IDLE;
static volatile LONG g_classic_install_progress = 0;
static HANDLE g_classic_install_thread = NULL;
static HANDLE g_classic_size_thread = NULL;
static char g_classic_install_status[MAX_LABEL] = "";
static char g_classic_install_error[MAX_LABEL] = "";
static char g_classic_downloaded_jar_path[MAX_PATHBUF] = "";
static volatile LONG g_classic_download_size_state = CLASSIC_SIZE_UNKNOWN;
static volatile LONG g_classic_download_size_bytes = 0;
static volatile LONG g_classic_install_cancel = 0;

static void pack_install_set_state(LONG state, LONG progress, const char *status) {
    if (status) lstrcpynA(g_classic_install_status, status, sizeof(g_classic_install_status));
    InterlockedExchange(&g_classic_install_progress, progress);
    InterlockedExchange(&g_classic_install_state, state);
}

static void pack_install_fail(const char *msg) {
    lstrcpynA(g_classic_install_error, msg ? msg : "Unknown error", sizeof(g_classic_install_error));
    pack_install_set_state(CLASSIC_INSTALL_ERROR, 0, "Failed");
    log_msg("LG webOS Release texture install failed: %s", g_classic_install_error);
}

static void pack_install_reset_cancel(void) {
    InterlockedExchange(&g_classic_install_cancel, 0);
}

static int pack_install_is_cancelled(void) {
    return InterlockedCompareExchange(&g_classic_install_cancel, 0, 0) != 0;
}

static size_t lgwebos_curl_write_file(void *ptr, size_t size, size_t nmemb, void *userdata) {
    FILE *f = (FILE *)userdata;
    size_t written;
    if (!f) return 0;
    written = fwrite(ptr, size, nmemb, f);
    return written * size;
}

static int lgwebos_curl_progress(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    (void)clientp; (void)ultotal; (void)ulnow;
    if (pack_install_is_cancelled()) return 1;
    if (dltotal > 0) {
        int pct = 1 + (int)((dlnow * 84) / dltotal);
        char st[MAX_LABEL];
        if (pct > 85) pct = 85;
        snprintf(st, sizeof(st), "Downloading client.jar (%d%%)", pct);
        pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, pct, st);
    } else if (dlnow > 0) {
        char st[MAX_LABEL];
        snprintf(st, sizeof(st), "Downloading client.jar (%ld bytes)", (long)dlnow);
        pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 40, st);
    }
    return 0;
}

static int lgwebos_download_file(const char *url, const char *path, char *err, size_t err_cap) {
    CURL *curl = NULL;
    FILE *f = NULL;
    CURLcode rc;
    long http_code = 0;
    if (err && err_cap) err[0] = 0;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) { snprintf(err, err_cap, "Could not initialize libcurl"); return 0; }
    f = fopen(path, "wb");
    if (!f) {
        snprintf(err, err_cap, "Could not create downloaded client.jar");
        curl_easy_cleanup(curl);
        return 0;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PEXCRAFT/1.0 LGwebOS");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, lgwebos_curl_write_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, lgwebos_curl_progress);
    rc = curl_easy_perform(curl);
    fflush(f);
    fclose(f);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    if (pack_install_is_cancelled()) {
        DeleteFileA(path);
        snprintf(err, err_cap, "Canceled");
        return 0;
    }
    if (rc != CURLE_OK) {
        DeleteFileA(path);
        snprintf(err, err_cap, "%s", curl_easy_strerror(rc));
        return 0;
    }
    if (http_code < 200 || http_code >= 300) {
        DeleteFileA(path);
        snprintf(err, err_cap, "HTTP %ld while downloading client.jar", http_code);
        return 0;
    }
    if (classic_file_size_bytes(path) < 1024ULL) {
        DeleteFileA(path);
        snprintf(err, err_cap, "Downloaded client.jar was empty");
        return 0;
    }
    return 1;
}


static int pex_platform_http_download_to_file(const char *url, const char *path, unsigned int expect_size, int legacy_mode);
#include "../../assets/legacy_asset_manager_shared.c"
static int lgwebos_legacy_curl_progress(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    (void)clientp; (void)dltotal; (void)dlnow; (void)ultotal; (void)ulnow;
    return legacy_assets_is_cancelled() ? 1 : 0;
}

static int pex_platform_http_download_to_file(const char *url, const char *path, unsigned int expect_size, int legacy_mode) {
    CURL *curl = NULL;
    FILE *f = NULL;
    CURLcode rc;
    long http_code = 0;
    unsigned long long got;
    (void)legacy_mode;
    if (legacy_assets_is_cancelled()) return 0;
    pxc_mkdirs_for_file(path);
    DeleteFileA(path);
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) return 0;
    f = fopen(path, "wb");
    if (!f) { curl_easy_cleanup(curl); return 0; }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PEXCRAFT/1.0 LGwebOS");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, lgwebos_curl_write_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, lgwebos_legacy_curl_progress);
    rc = curl_easy_perform(curl);
    fflush(f);
    fclose(f);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    if (legacy_assets_is_cancelled() || rc != CURLE_OK || http_code < 200 || http_code >= 300) {
        DeleteFileA(path);
        return 0;
    }
    got = file_size_bytes(path);
    if (got == 0 || (expect_size && got != (unsigned long long)expect_size)) {
        DeleteFileA(path);
        return 0;
    }
    return 1;
}

static int lgwebos_install_resources_blocking(void) {
    char pack_dir[MAX_PATHBUF];
    char err[MAX_LABEL];
    err[0] = 0;
    ensure_dir(g_texpack_dir);
    snprintf(g_classic_downloaded_jar_path, sizeof(g_classic_downloaded_jar_path), "%s/minecraft_classic_client.jar", g_mc_dir);
    DeleteFileA(g_classic_downloaded_jar_path);
    pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 1, "Connecting to Mojang...");
    if (!lgwebos_download_file(CLASSIC_PACK_URL, g_classic_downloaded_jar_path, err, sizeof(err))) {
        pack_install_fail(err[0] ? err : "Could not download client.jar");
        return 0;
    }
    if (pack_install_is_cancelled()) { pack_install_fail("Canceled"); return 0; }
    pack_asset_path(pack_dir, sizeof(pack_dir));
    pack_install_set_state(CLASSIC_INSTALL_EXTRACTING, 90, "Extracting textures...");
    if (!pxc_extract_zip_file(g_classic_downloaded_jar_path, pack_dir, err, sizeof(err))) {
        pack_install_fail(err[0] ? err : "Could not extract client.jar");
        return 0;
    }
    if (!pack_is_installed() || pack_missing_required_textures()) {
        pack_install_fail("Extracted pack is missing required textures");
        return 0;
    }
    DeleteFileA(g_classic_downloaded_jar_path);
    pack_install_set_state(CLASSIC_INSTALL_DONE, 100, "Done!");
    log_msg("Installed Minecraft Classic texture pack at %s", pack_dir);
    return 1;
}

static DWORD WINAPI lgwebos_pack_install_worker(LPVOID unused) {
    (void)unused;
    lgwebos_install_resources_blocking();
    return 0;
}

static DWORD WINAPI lgwebos_size_fetch_worker(LPVOID unused) {
    (void)unused;
    CURL *curl;
    curl_off_t len = 0;
    CURLcode rc;
    long http_code = 0;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) {
        InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_ERROR);
        return 0;
    }
    curl_easy_setopt(curl, CURLOPT_URL, CLASSIC_PACK_URL);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PEXCRAFT/1.0 LGwebOS");
    rc = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &len);
    curl_easy_cleanup(curl);
    if (rc == CURLE_OK && http_code >= 200 && http_code < 400 && len > 0 && len < (curl_off_t)2147483647) {
        InterlockedExchange(&g_classic_download_size_bytes, (LONG)len);
        InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_READY);
    } else {
        InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_ERROR);
    }
    return 0;
}

static void pack_install_start_size_fetch(void) {
    LONG old = InterlockedCompareExchange(&g_classic_download_size_state, CLASSIC_SIZE_FETCHING, CLASSIC_SIZE_UNKNOWN);
    if (old != CLASSIC_SIZE_UNKNOWN) return;
    if (g_classic_size_thread) return;
    g_classic_size_thread = CreateThread(NULL, 0, lgwebos_size_fetch_worker, NULL, 0, NULL);
    if (!g_classic_size_thread) InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_ERROR);
}

static void format_download_size(char *out, size_t cap) {
    LONG state = InterlockedCompareExchange(&g_classic_download_size_state, 0, 0);
    LONG bytes = InterlockedCompareExchange(&g_classic_download_size_bytes, 0, 0);
    if (state == CLASSIC_SIZE_READY && bytes > 0) snprintf(out, cap, "Download size: %.2f MB (%ld bytes)", (double)bytes / (1024.0 * 1024.0), (long)bytes);
    else if (state == CLASSIC_SIZE_FETCHING) snprintf(out, cap, "Download size: checking server...");
    else snprintf(out, cap, "Download size: unavailable");
}

static void pack_install_request_cancel(void) {
    InterlockedExchange(&g_classic_install_cancel, 1);
    if (InterlockedCompareExchange(&g_classic_install_state, 0, 0) == CLASSIC_INSTALL_DOWNLOADING) {
        pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, InterlockedCompareExchange(&g_classic_install_progress, 0, 0), "Canceling...");
    }
}

static void pack_install_start(void) {
    LONG state = InterlockedCompareExchange(&g_classic_install_state, CLASSIC_INSTALL_DOWNLOADING, CLASSIC_INSTALL_IDLE);
    if (state == CLASSIC_INSTALL_DOWNLOADING || state == CLASSIC_INSTALL_EXTRACTING) {
        set_screen(SCREEN_TEXPACK_INSTALL);
        return;
    }
    if (g_classic_install_thread) {
        WaitForSingleObject(g_classic_install_thread, 0);
        CloseHandle(g_classic_install_thread);
        g_classic_install_thread = NULL;
    }
    pack_install_reset_cancel();
    g_classic_install_error[0] = 0;
    pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Preparing download...");
    set_screen(SCREEN_TEXPACK_INSTALL);
    g_classic_install_thread = CreateThread(NULL, 0, lgwebos_pack_install_worker, NULL, 0, NULL);
    if (!g_classic_install_thread) pack_install_fail("Could not start downloader thread");
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
        if (!strcmp(g_classic_install_error, "Canceled")) set_screen(SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT);
        else open_notice("Texture Pack", "Could not install Minecraft Classic.", g_classic_install_error);
    }
}

static int pack_resources_install_blocking(void) {
    pack_install_reset_cancel();
    g_classic_install_error[0] = 0;
    return lgwebos_install_resources_blocking();
}
