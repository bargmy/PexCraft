/* SDL2 Release/classic resource installer.
   Startup path installs only the Minecraft 1.2.5 client.jar texture pack.
   Legacy asset-index downloads live in Options -> Assets.

   No libcurl dependency: HTTPS is delegated to platform/userland helpers
   (python urllib, wget, PowerShell, etc.) so the SDL2 build is not tied to
   one Linux-only library. */

#define CLASSIC_INSTALL_IDLE 0
#define CLASSIC_INSTALL_DOWNLOADING 1
#define CLASSIC_INSTALL_EXTRACTING 2
#define CLASSIC_INSTALL_DONE 3
#define CLASSIC_INSTALL_ERROR 4

#define CLASSIC_SIZE_UNKNOWN 0
#define CLASSIC_SIZE_FETCHING 1
#define CLASSIC_SIZE_READY 2
#define CLASSIC_SIZE_ERROR 3

#define LEGACY_ASSET_LANG       0x0020
#define LEGACY_ASSET_OTHER      0x0040
#define LEGACY_ASSET_ALL        (CLASSIC_AUDIO_ALL | LEGACY_ASSET_LANG | LEGACY_ASSET_OTHER)
#define LEGACY_ASSET_THREADS    24
#define CLASSIC_CLIENT_JAR_SIZE  4032098u
#define CLASSIC_LEGACY_JSON_MAX (2u * 1024u * 1024u)

static volatile LONG g_classic_install_state = CLASSIC_INSTALL_IDLE;
static volatile LONG g_classic_install_progress = 0;
static volatile LONG g_classic_install_cancel_requested = 0;
static HANDLE g_classic_install_thread = NULL;
static char g_classic_install_status[MAX_LABEL] = "";
static char g_classic_install_error[MAX_LABEL] = "";

static volatile LONG g_classic_download_size_state = CLASSIC_SIZE_UNKNOWN;
static volatile LONG g_classic_texture_download_size_bytes = 0;
static volatile LONG g_classic_sound_download_size_bytes = 0;
static volatile LONG g_classic_sound_download_count = 0;

static volatile LONG g_legacy_index_state = CLASSIC_SIZE_UNKNOWN;
static HANDLE g_legacy_index_thread = NULL;
static char g_legacy_index_error[MAX_LABEL] = "";
static volatile LONG g_legacy_installed_mask = 0;
static volatile LONG g_legacy_missing_mask = LEGACY_ASSET_ALL;
static volatile LONG g_legacy_download_state = CLASSIC_INSTALL_IDLE;
static volatile LONG g_legacy_download_mask = 0;
static volatile LONG g_legacy_download_progress = 0;
static volatile LONG g_legacy_download_done_kb = 0;
static volatile LONG g_legacy_download_total_kb = 0;
static char g_legacy_download_status[MAX_LABEL] = "";
static char g_legacy_download_error[MAX_LABEL] = "";
static HANDLE g_legacy_download_thread = NULL;
static volatile LONG g_legacy_download_cancel = 0;

static unsigned long long g_legacy_total_bytes[7];
static unsigned long long g_legacy_missing_bytes[7];
static int g_legacy_total_count[7];
static int g_legacy_missing_count[7];

typedef struct ClassicAsset {
    char path[MAX_PATHBUF];       /* legacy.json object path */
    char out_path[MAX_PATHBUF];   /* absolute local output path */
    char hash[48];
    unsigned int size;
    int category;
} ClassicAsset;

typedef struct ClassicAssetDownloadCtx {
    ClassicAsset *assets;
    int count;
    int mask;
    volatile LONG next_index;
    volatile LONG completed;
    volatile LONG failed;
} ClassicAssetDownloadCtx;

static ClassicAsset *g_legacy_assets = NULL;
static int g_legacy_asset_count = 0;

static LONG atomic_increment_int(volatile LONG *target) {
#ifdef _WIN32
    return InterlockedIncrement(target);
#else
    return __sync_add_and_fetch(target, 1);
#endif
}

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

static void pack_install_set_state(LONG state, LONG progress, const char *status) {
    if (status) lstrcpynA(g_classic_install_status, status, sizeof(g_classic_install_status));
    InterlockedExchange(&g_classic_install_progress, progress);
    InterlockedExchange(&g_classic_install_state, state);
}

static void pack_install_reset_cancel(void) {
    InterlockedExchange(&g_classic_install_cancel_requested, 0);
}

static void pack_install_request_cancel(void) {
    InterlockedExchange(&g_classic_install_cancel_requested, 1);
    InterlockedExchange(&g_legacy_download_cancel, 1);
    g_opts.download_classic_sounds = 0;
    g_opts.ignore_classic_sounds_warning = 1;
    save_options();
    log_msg("Release resource download cancel requested");
}

static int pack_install_is_cancelled(void) {
    return InterlockedCompareExchange(&g_classic_install_cancel_requested, 0, 0) != 0;
}

static int legacy_assets_is_cancelled(void) {
    return InterlockedCompareExchange(&g_legacy_download_cancel, 0, 0) != 0;
}

static void pack_install_fail(const char *msg) {
    lstrcpynA(g_classic_install_error, msg ? msg : "Unknown error", sizeof(g_classic_install_error));
    pack_install_set_state(CLASSIC_INSTALL_ERROR, 0, "Failed");
    log_msg("PexCraft Release texture install failed: %s", g_classic_install_error);
}

static void legacy_download_set_state(LONG state, LONG progress, const char *status) {
    if (status) lstrcpynA(g_legacy_download_status, status, sizeof(g_legacy_download_status));
    InterlockedExchange(&g_legacy_download_progress, progress);
    InterlockedExchange(&g_legacy_download_state, state);
}

static void legacy_download_fail(const char *msg) {
    lstrcpynA(g_legacy_download_error, msg ? msg : "Unknown error", sizeof(g_legacy_download_error));
    legacy_download_set_state(CLASSIC_INSTALL_ERROR, 0, "Failed");
    log_msg("PexCraft legacy asset download failed: %s", g_legacy_download_error);
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

static void pex_format_size(unsigned long long bytes, char *out, size_t cap) {
    if (!out || cap == 0) return;
    if (bytes >= 1024ULL * 1024ULL) snprintf(out, cap, "%.2f MB", (double)bytes / (1024.0 * 1024.0));
    else if (bytes >= 1024ULL) snprintf(out, cap, "%.1f KB", (double)bytes / 1024.0);
    else snprintf(out, cap, "%llu B", bytes);
}

static void pex_shell_quote(char *out, size_t cap, const char *s) {
    size_t n = 0;
    if (!out || cap == 0) return;
    out[n++] = '\'';
    for (; s && *s && n + 5 < cap; ++s) {
        if (*s == '\'') {
            out[n++] = '\''; out[n++] = '\\'; out[n++] = '\''; out[n++] = '\'';
        } else out[n++] = *s;
    }
    if (n + 1 < cap) out[n++] = '\'';
    out[n < cap ? n : cap - 1] = 0;
}

static int pex_try_system_download(const char *cmd) {
    int rc;
    if (!cmd || !*cmd) return 0;
    rc = system(cmd);
    return rc == 0;
}

static int http_download_to_file(const char *url, const char *path, unsigned int expect_size) {
    char qurl[1024], qpath[MAX_PATHBUF * 2], tmp[MAX_PATHBUF], qtmp[MAX_PATHBUF * 2];
    char cmd[MAX_PATHBUF * 4 + 2048];
    unsigned long long got;
    pxc_mkdirs_for_file(path);
    snprintf(tmp, sizeof(tmp), "%s.part", path);
    DeleteFileA(tmp);
    pex_shell_quote(qurl, sizeof(qurl), url);
    pex_shell_quote(qtmp, sizeof(qtmp), tmp);
    pex_shell_quote(qpath, sizeof(qpath), path);

    /* Prefer Python because it is common on Linux/macOS and does TLS using the OS cert store.
       Fallbacks intentionally avoid curl. */
    snprintf(cmd, sizeof(cmd), "python3 -c 'import sys,urllib.request; urllib.request.urlretrieve(sys.argv[1], sys.argv[2])' %s %s", qurl, qtmp);
    if (!pex_try_system_download(cmd)) {
        snprintf(cmd, sizeof(cmd), "python -c 'import sys,urllib.request; urllib.request.urlretrieve(sys.argv[1], sys.argv[2])' %s %s", qurl, qtmp);
        if (!pex_try_system_download(cmd)) {
            snprintf(cmd, sizeof(cmd), "wget -q -O %s %s", qtmp, qurl);
            if (!pex_try_system_download(cmd)) {
                snprintf(cmd, sizeof(cmd), "powershell -NoProfile -ExecutionPolicy Bypass -Command \"[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -UseBasicParsing -Uri %s -OutFile %s\"", qurl, qtmp);
                if (!pex_try_system_download(cmd)) { DeleteFileA(tmp); return 0; }
            }
        }
    }

    got = file_size_bytes(tmp);
    if (got == 0 || (expect_size && got != (unsigned long long)expect_size)) { DeleteFileA(tmp); return 0; }
    DeleteFileA(path);
    if (!MoveFileA(tmp, path)) {
        /* rename can fail across odd mounts; copy as fallback. */
        if (!CopyFileA(tmp, path, FALSE)) { DeleteFileA(tmp); DeleteFileA(path); return 0; }
        DeleteFileA(tmp);
    }
    return file_size_bytes(path) == got;
}

static int pex_read_file_alloc(const char *path, char **out_data, size_t *out_len, size_t max_len) {
    FILE *f;
    long n;
    char *data;
    if (out_data) *out_data = NULL;
    if (out_len) *out_len = 0;
    f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    n = ftell(f);
    if (n <= 0 || (size_t)n > max_len) { fclose(f); return 0; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return 0; }
    data = (char *)malloc((size_t)n + 1);
    if (!data) { fclose(f); return 0; }
    if (fread(data, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(data); return 0; }
    fclose(f);
    data[n] = 0;
    if (out_data) *out_data = data; else free(data);
    if (out_len) *out_len = (size_t)n;
    return 1;
}

static int http_download_to_memory(const char *url, char **out_data, size_t *out_len, size_t max_len) {
    char tmp[MAX_PATHBUF];
    unsigned int expect = 0;
    int ok;
    if (out_data) *out_data = NULL;
    if (out_len) *out_len = 0;
    snprintf(tmp, sizeof(tmp), "%s/legacy_index_download.json", g_mc_dir[0] ? g_mc_dir : ".");
    if (strstr(url, "legacy.json")) expect = 109634u;
    ok = http_download_to_file(url, tmp, expect);
    if (!ok) return 0;
    ok = pex_read_file_alloc(tmp, out_data, out_len, max_len);
    DeleteFileA(tmp);
    return ok;
}

static int pack_install_query_size_bytes(const char *url, unsigned long long *out_bytes) {
    if (out_bytes) *out_bytes = 0;
    if (url && strstr(url, "client.jar")) { if (out_bytes) *out_bytes = CLASSIC_CLIENT_JAR_SIZE; return 1; }
    return 0;
}

static int classic_audio_category_for_legacy_path(const char *path) {
    if (!path || !*path) return 0;
    if (!strncmp(path, "sounds/music/menu/", 18) || !strncmp(path, "music/menu/", 11)) return CLASSIC_AUDIO_MENU_MUSIC;
    if (!strncmp(path, "sounds/music/game/", 18)) return CLASSIC_AUDIO_GAME_MUSIC;
    if (!strncmp(path, "music/", 6)) return CLASSIC_AUDIO_GAME_MUSIC;
    if (!strncmp(path, "sounds/records/", 15) || !strncmp(path, "records/", 8)) return CLASSIC_AUDIO_RECORDS;
    if (!strncmp(path, "sound/mob/", 10) || !strncmp(path, "sounds/mob/", 11)) return CLASSIC_AUDIO_MOBS;
    if (!strncmp(path, "sound/", 6) || !strncmp(path, "sounds/", 7)) return CLASSIC_AUDIO_WORLD_UI;
    return 0;
}

static int legacy_asset_category_for_path(const char *path) {
    int audio = classic_audio_category_for_legacy_path(path);
    if (audio) return audio;
    if (path && !strncmp(path, "lang/", 5) && strstr(path, ".lang")) return LEGACY_ASSET_LANG;
    if (path && (!strncmp(path, "icons/", 6) || !strcmp(path, "pack.mcmeta") || !strcmp(path, "sounds.json") || !strcmp(path, "READ_ME_I_AM_VERY_IMPORTANT.txt"))) return LEGACY_ASSET_OTHER;
    return 0;
}

static int legacy_category_slot(int category) {
    switch (category) {
        case LEGACY_ASSET_LANG: return 0;
        case CLASSIC_AUDIO_MOBS: return 1;
        case CLASSIC_AUDIO_WORLD_UI: return 2;
        case CLASSIC_AUDIO_RECORDS: return 3;
        case CLASSIC_AUDIO_MENU_MUSIC: return 4;
        case CLASSIC_AUDIO_GAME_MUSIC: return 5;
        case LEGACY_ASSET_OTHER: return 6;
        default: return -1;
    }
}

static const char *legacy_category_name(int category) {
    switch (category) {
        case LEGACY_ASSET_LANG: return "Languages";
        case CLASSIC_AUDIO_MOBS: return "Mob sounds";
        case CLASSIC_AUDIO_WORLD_UI: return "World sounds";
        case CLASSIC_AUDIO_RECORDS: return "Music discs";
        case CLASSIC_AUDIO_MENU_MUSIC: return "Menu music";
        case CLASSIC_AUDIO_GAME_MUSIC: return "Game music";
        case LEGACY_ASSET_OTHER: return "Other";
        default: return "Assets";
    }
}

static void legacy_sound_make_output_path(const char *path, char *out, size_t cap) {
    if (!out || cap == 0) return;
    out[0] = 0;
    if (!path || !*path) return;
    if (!strncmp(path, "sounds/music/menu/", 18)) snprintf(out, cap, "music/menu/%s", path + 18);
    else if (!strncmp(path, "sounds/music/game/", 18)) snprintf(out, cap, "music/game/%s", path + 18);
    else if (!strncmp(path, "music/menu/", 11)) snprintf(out, cap, "%s", path);
    else if (!strncmp(path, "music/", 6)) snprintf(out, cap, "music/game/%s", path + 6);
    else if (!strncmp(path, "sounds/records/", 15)) snprintf(out, cap, "streaming/%s", path + 15);
    else if (!strncmp(path, "records/", 8)) snprintf(out, cap, "streaming/%s", path + 8);
    else if (!strncmp(path, "sounds/", 7)) snprintf(out, cap, "sound/%s", path + 7);
    else snprintf(out, cap, "%s", path);
}

static void legacy_asset_make_output_path(int category, const char *legacy_path, char *out, size_t cap) {
    char root[MAX_PATHBUF];
    char rel[MAX_PATHBUF];
    out[0] = 0;
    if (category == LEGACY_ASSET_LANG) {
        char pack[MAX_PATHBUF];
        pack_asset_path(pack, sizeof(pack));
        path_join(out, cap, pack, legacy_path);
    } else if (category & CLASSIC_AUDIO_ALL) {
        classic_resources_path(root, sizeof(root));
        legacy_sound_make_output_path(legacy_path, rel, sizeof(rel));
        path_join(out, cap, root, rel);
    } else {
        classic_resources_path(root, sizeof(root));
        snprintf(rel, sizeof(rel), "legacy/%s", legacy_path ? legacy_path : "unknown");
        path_join(out, cap, root, rel);
    }
}

static const char *json_find_token(const char *p, const char *end, const char *token) {
    size_t tl = strlen(token);
    for (; p && p + tl <= end; ++p) if (!memcmp(p, token, tl)) return p;
    return NULL;
}

static int legacy_asset_already_added(ClassicAsset *assets, int count, const char *out_path) {
    if (!assets || !out_path || !*out_path) return 0;
    for (int i = 0; i < count; ++i) if (!strcmp(assets[i].out_path, out_path)) return 1;
    return 0;
}

static int legacy_parse_index(const char *json, size_t len, int category_mask, ClassicAsset **out_assets, int *out_count, unsigned long long *out_size) {
    const char *p = json, *end = json + len;
    int cap = 0, count = 0;
    unsigned long long total = 0;
    ClassicAsset *assets = NULL;
    if (out_assets) *out_assets = NULL;
    if (out_count) *out_count = 0;
    if (out_size) *out_size = 0;
    if (!json || len == 0 || (category_mask & LEGACY_ASSET_ALL) == 0) return 0;

    while (p < end) {
        const char *q = strchr(p, '"');
        const char *r, *obj_end, *hash_tok, *size_tok;
        char key[MAX_PATHBUF], hash[48], out_full[MAX_PATHBUF];
        size_t key_len;
        unsigned int sz;
        int category;
        if (!q || q >= end) break;
        r = q + 1;
        while (r < end && *r != '"') r++;
        if (r >= end) break;
        key_len = (size_t)(r - (q + 1));
        if (key_len == 0 || key_len >= sizeof(key)) { p = r + 1; continue; }
        memcpy(key, q + 1, key_len); key[key_len] = 0;
        p = r + 1;
        category = legacy_asset_category_for_path(key);
        if (!category || !(category_mask & category)) continue;
        legacy_asset_make_output_path(category, key, out_full, sizeof(out_full));
        if (!out_full[0] || legacy_asset_already_added(assets, count, out_full)) continue;
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
        sz = (unsigned int)parse_u64_decimal(size_tok + 1);
        if (!sz) { p = obj_end + 1; continue; }
        total += sz;
        if (count >= cap) {
            int nc = cap ? cap * 2 : 256;
            ClassicAsset *na = (ClassicAsset *)realloc(assets, (size_t)nc * sizeof(ClassicAsset));
            if (!na) { free(assets); return 0; }
            assets = na; cap = nc;
        }
        snprintf(assets[count].path, sizeof(assets[count].path), "%s", key);
        snprintf(assets[count].out_path, sizeof(assets[count].out_path), "%s", out_full);
        snprintf(assets[count].hash, sizeof(assets[count].hash), "%s", hash);
        assets[count].size = sz;
        assets[count].category = category;
        count++;
        p = obj_end + 1;
    }
    if (out_assets) *out_assets = assets;
    else free(assets);
    if (out_count) *out_count = count;
    if (out_size) *out_size = total;
    return count > 0;
}

static int legacy_sound_parse_index(const char *json, size_t len, int audio_mask, ClassicAsset **out_assets, int *out_count, unsigned long long *out_size) {
    return legacy_parse_index(json, len, audio_mask & CLASSIC_AUDIO_ALL, out_assets, out_count, out_size);
}

static void legacy_assets_clear_cache(void) {
    free(g_legacy_assets);
    g_legacy_assets = NULL;
    g_legacy_asset_count = 0;
    memset(g_legacy_total_bytes, 0, sizeof(g_legacy_total_bytes));
    memset(g_legacy_missing_bytes, 0, sizeof(g_legacy_missing_bytes));
    memset(g_legacy_total_count, 0, sizeof(g_legacy_total_count));
    memset(g_legacy_missing_count, 0, sizeof(g_legacy_missing_count));
}

static void pex_write_legacy_languages_txt(void) {
    /* legacy.json contains lang/*.lang objects, but no lang/languages.txt.
       Generate the Java-style list with the same user-facing native names that
       Minecraft shows in GuiLanguage.  Do not use English names here: this file
       is the language selection UI. */
    static const char *pairs[] = {
        "af_ZA=Afrikaans", "ar_SA=العربية", "bg_BG=Български", "ca_ES=Català", "cs_CZ=Čeština",
        "cy_GB=Cymraeg", "da_DK=Dansk", "de_DE=Deutsch", "el_GR=Ελληνικά", "en_AU=English (Australia)",
        "en_CA=English (Canada)", "en_GB=English (UK)", "en_PT=Pirate Speak", "en_US=English (US)",
        "eo_UY=Esperanto", "es_AR=Español (Argentina)", "es_ES=Español (España)", "es_MX=Español (México)",
        "es_UY=Español (Uruguay)", "es_VE=Español (Venezuela)", "et_EE=Eesti", "eu_ES=Euskara",
        "fa_IR=فارسی", "fi_FI=Suomi", "fil_PH=Filipino", "fr_CA=Français (Canada)", "fr_FR=Français",
        "ga_IE=Gaeilge", "gl_ES=Galego", "he_IL=עברית", "hi_IN=हिन्दी", "hr_HR=Hrvatski",
        "hu_HU=Magyar", "hy_AM=Հայերեն", "id_ID=Bahasa Indonesia", "is_IS=Íslenska", "it_IT=Italiano",
        "ja_JP=日本語", "ka_GE=ქართული", "ko_KR=한국어", "kw_GB=Kernewek", "la_LA=Latina",
        "lb_LU=Lëtzebuergesch", "lt_LT=Lietuvių", "lv_LV=Latviešu", "ms_MY=Bahasa Melayu", "mt_MT=Malti",
        "nl_NL=Nederlands", "nn_NO=Norsk nynorsk", "no_NO=Norsk bokmål", "oc_FR=Occitan", "pl_PL=Polski",
        "pt_BR=Português (Brasil)", "pt_PT=Português (Portugal)", "qya_AA=Quenya", "ro_RO=Română", "ru_RU=Русский",
        "sk_SK=Slovenčina", "sl_SI=Slovenščina", "sr_SP=Српски", "sv_SE=Svenska", "th_TH=ไทย",
        "tlh_AA=tlhIngan Hol", "tr_TR=Türkçe", "uk_UA=Українська", "vi_VN=Tiếng Việt", "zh_CN=简体中文",
        "zh_TW=繁體中文"
    };
    char pack[MAX_PATHBUF], path[MAX_PATHBUF];
    FILE *f;
    pack_asset_path(pack, sizeof(pack));
    path_join(path, sizeof(path), pack, "lang/languages.txt");
    pxc_mkdirs_for_file(path);
    f = fopen(path, "wb");
    if (!f) return;
    for (int i = 0; i < (int)ARRAY_COUNT(pairs); ++i) fprintf(f, "%s\n", pairs[i]);
    fclose(f);
    /* legacy.json does not always carry en_US.lang because English is the built-in base.
       The language loader expects the file, so create a valid empty overlay. */
    path_join(path, sizeof(path), pack, "lang/en_US.lang");
    if (file_size_bytes(path) == 0) {
        pxc_mkdirs_for_file(path);
        f = fopen(path, "ab");
        if (f) fclose(f);
    }
}

static void legacy_assets_recompute_status(void) {
    int installed = 0, missing = 0;
    memset(g_legacy_total_bytes, 0, sizeof(g_legacy_total_bytes));
    memset(g_legacy_missing_bytes, 0, sizeof(g_legacy_missing_bytes));
    memset(g_legacy_total_count, 0, sizeof(g_legacy_total_count));
    memset(g_legacy_missing_count, 0, sizeof(g_legacy_missing_count));
    for (int i = 0; i < g_legacy_asset_count; ++i) {
        ClassicAsset *a = &g_legacy_assets[i];
        int slot = legacy_category_slot(a->category);
        int miss = file_size_bytes(a->out_path) != (unsigned long long)a->size;
        if (slot >= 0 && slot < 7) {
            g_legacy_total_bytes[slot] += a->size;
            g_legacy_total_count[slot]++;
            if (miss) { g_legacy_missing_bytes[slot] += a->size; g_legacy_missing_count[slot]++; }
        }
    }
    for (int i = 0; i < 7; ++i) {
        int cat = (i == 0) ? LEGACY_ASSET_LANG : (i == 1) ? CLASSIC_AUDIO_MOBS : (i == 2) ? CLASSIC_AUDIO_WORLD_UI : (i == 3) ? CLASSIC_AUDIO_RECORDS : (i == 4) ? CLASSIC_AUDIO_MENU_MUSIC : (i == 5) ? CLASSIC_AUDIO_GAME_MUSIC : LEGACY_ASSET_OTHER;
        if (g_legacy_total_count[i] > 0 && g_legacy_missing_count[i] == 0) installed |= cat;
        if (g_legacy_missing_count[i] > 0) missing |= cat;
    }
    InterlockedExchange(&g_legacy_installed_mask, installed & LEGACY_ASSET_ALL);
    InterlockedExchange(&g_legacy_missing_mask, missing & LEGACY_ASSET_ALL);
}

static DWORD WINAPI legacy_assets_index_worker(LPVOID unused) {
    char *json = NULL;
    size_t len = 0;
    ClassicAsset *assets = NULL;
    int count = 0;
    unsigned long long total = 0;
    (void)unused;
    g_legacy_index_error[0] = 0;
    if (!http_download_to_memory(CLASSIC_SOUNDS_INDEX_URL, &json, &len, CLASSIC_LEGACY_JSON_MAX)) {
        snprintf(g_legacy_index_error, sizeof(g_legacy_index_error), "Could not fetch legacy.json");
        InterlockedExchange(&g_legacy_index_state, CLASSIC_SIZE_ERROR);
        return 0;
    }
    if (!legacy_parse_index(json, len, LEGACY_ASSET_ALL, &assets, &count, &total)) {
        free(json);
        snprintf(g_legacy_index_error, sizeof(g_legacy_index_error), "Could not parse legacy.json");
        InterlockedExchange(&g_legacy_index_state, CLASSIC_SIZE_ERROR);
        return 0;
    }
    free(json);
    legacy_assets_clear_cache();
    g_legacy_assets = assets;
    g_legacy_asset_count = count;
    legacy_assets_recompute_status();
    InterlockedExchange(&g_legacy_index_state, CLASSIC_SIZE_READY);
    log_msg("Legacy asset index ready: %d assets, %llu bytes", count, total);
    return 0;
}

static void legacy_assets_start_index_fetch(void) {
    LONG old = InterlockedCompareExchange(&g_legacy_index_state, CLASSIC_SIZE_FETCHING, CLASSIC_SIZE_UNKNOWN);
    if (old != CLASSIC_SIZE_UNKNOWN) return;
    if (g_legacy_index_thread) { CloseHandle(g_legacy_index_thread); g_legacy_index_thread = NULL; }
    g_legacy_index_thread = CreateThread(NULL, 0, legacy_assets_index_worker, NULL, 0, NULL);
    if (!g_legacy_index_thread) {
        snprintf(g_legacy_index_error, sizeof(g_legacy_index_error), "Could not start legacy.json fetch");
        InterlockedExchange(&g_legacy_index_state, CLASSIC_SIZE_ERROR);
    }
}

static int legacy_assets_index_ready(void) {
    return InterlockedCompareExchange(&g_legacy_index_state, 0, 0) == CLASSIC_SIZE_READY;
}

static int legacy_assets_index_state(void) {
    return (int)InterlockedCompareExchange(&g_legacy_index_state, 0, 0);
}

static int legacy_assets_download_mask(void) {
    return (int)InterlockedCompareExchange(&g_legacy_download_mask, 0, 0);
}

static void legacy_assets_index_error_line(char *out, size_t cap) {
    if (!out || cap == 0) return;
    snprintf(out, cap, "%s", g_legacy_index_error[0] ? g_legacy_index_error : "Network/index error");
}

static void legacy_assets_status_line(char *out, size_t cap) {
    if (!out || cap == 0) return;
    snprintf(out, cap, "%s", g_legacy_download_status[0] ? g_legacy_download_status : "Downloading legacy assets");
}

static int legacy_assets_progress_percent(void) {
    return (int)InterlockedCompareExchange(&g_legacy_download_progress, 0, 0);
}

static void legacy_assets_refresh_index(void) {
    if (g_legacy_index_thread) { WaitForSingleObject(g_legacy_index_thread, 0); CloseHandle(g_legacy_index_thread); g_legacy_index_thread = NULL; }
    legacy_assets_clear_cache();
    InterlockedExchange(&g_legacy_index_state, CLASSIC_SIZE_UNKNOWN);
    legacy_assets_start_index_fetch();
}

static int legacy_asset_group_missing(int category) {
    return (InterlockedCompareExchange(&g_legacy_missing_mask, 0, 0) & category) != 0;
}

static int legacy_assets_any_missing(void) {
    return (InterlockedCompareExchange(&g_legacy_missing_mask, 0, 0) & LEGACY_ASSET_ALL) != 0;
}

static void legacy_asset_group_summary(int category, char *out, size_t cap) {
    int slot = legacy_category_slot(category);
    char total[64], miss[64];
    if (slot < 0 || slot >= 7 || g_legacy_total_count[slot] <= 0) {
        snprintf(out, cap, "%s: unavailable", legacy_category_name(category));
        return;
    }
    pex_format_size(g_legacy_total_bytes[slot], total, sizeof(total));
    pex_format_size(g_legacy_missing_bytes[slot], miss, sizeof(miss));
    if (g_legacy_missing_count[slot] <= 0) snprintf(out, cap, "%s: Done (%d files, %s)", legacy_category_name(category), g_legacy_total_count[slot], total);
    else snprintf(out, cap, "%s: %d/%d missing, %s left / %s", legacy_category_name(category), g_legacy_missing_count[slot], g_legacy_total_count[slot], miss, total);
}

static void legacy_asset_group_current_bytes(int category, unsigned long long *out_done, unsigned long long *out_total, unsigned long long *out_left) {
    unsigned long long done = 0, total = 0, left = 0;
    for (int i = 0; i < g_legacy_asset_count; ++i) {
        ClassicAsset *a = &g_legacy_assets[i];
        if (!(a->category & category)) continue;
        total += (unsigned long long)a->size;
        if (file_size_bytes(a->out_path) == (unsigned long long)a->size) done += (unsigned long long)a->size;
        else left += (unsigned long long)a->size;
    }
    if (out_done) *out_done = done;
    if (out_total) *out_total = total;
    if (out_left) *out_left = left;
}

static int legacy_asset_group_progress_percent(int category) {
    unsigned long long done = 0, total = 0, left = 0;
    legacy_asset_group_current_bytes(category, &done, &total, &left);
    if (total == 0) return 0;
    if (left == 0) return 100;
    return (int)((done * 100ULL) / total);
}

static void legacy_asset_button_label(int category, char *out, size_t cap) {
    int state = InterlockedCompareExchange(&g_legacy_download_state, 0, 0);
    int active = InterlockedCompareExchange(&g_legacy_download_mask, 0, 0) & category;
    int slot = legacy_category_slot(category);
    char miss[64];
    if (!out || cap == 0) return;
    if ((state == CLASSIC_INSTALL_DOWNLOADING || state == CLASSIC_INSTALL_EXTRACTING) && active) {
        unsigned long long done = 0, total = 0, left = 0;
        char done_s[32], total_s[32], left_s[32];
        int p;
        legacy_asset_group_current_bytes(category, &done, &total, &left);
        p = total ? (int)((done * 100ULL) / total) : (int)InterlockedCompareExchange(&g_legacy_download_progress, 0, 0);
        pex_format_size(done, done_s, sizeof(done_s));
        pex_format_size(total, total_s, sizeof(total_s));
        pex_format_size(left, left_s, sizeof(left_s));
        /* Keep all live download information on the asset button itself. */
        snprintf(out, cap, "%s %d%% %s/%s L:%s", legacy_category_name(category), p, done_s, total_s, left_s);
    } else {
        if (slot >= 0 && slot < 7) pex_format_size(g_legacy_missing_bytes[slot], miss, sizeof(miss));
        else snprintf(miss, sizeof(miss), "?");
        snprintf(out, cap, "%s (%s)", legacy_category_name(category), miss);
    }
}

static DWORD WINAPI legacy_asset_download_worker(LPVOID arg) {
    ClassicAssetDownloadCtx *ctx = (ClassicAssetDownloadCtx *)arg;
    if (!ctx) return 0;
    for (;;) {
        LONG idx = atomic_increment_int(&ctx->next_index) - 1;
        ClassicAsset *a;
        char url[256];
        unsigned long long sz;
        LONG done_kb, total_kb;
        int pct;
        if (idx < 0 || idx >= ctx->count) break;
        if (legacy_assets_is_cancelled()) { InterlockedExchange(&ctx->failed, 2); break; }
        if (InterlockedCompareExchange(&ctx->failed, 0, 0)) break;
        a = &ctx->assets[idx];
        if (!(a->category & ctx->mask)) continue;
        sz = file_size_bytes(a->out_path);
        if (sz != (unsigned long long)a->size) {
            snprintf(url, sizeof(url), "%s/%.2s/%s", CLASSIC_SOUND_OBJECT_URL_PREFIX, a->hash, a->hash);
            if (!http_download_to_file(url, a->out_path, a->size)) {
                InterlockedExchange(&ctx->failed, legacy_assets_is_cancelled() ? 2 : 1);
                break;
            }
            done_kb = InterlockedExchange(&g_legacy_download_done_kb, InterlockedCompareExchange(&g_legacy_download_done_kb, 0, 0) + (LONG)((a->size + 1023u) / 1024u));
            (void)done_kb;
        }
        atomic_increment_int(&ctx->completed);
        done_kb = InterlockedCompareExchange(&g_legacy_download_done_kb, 0, 0);
        total_kb = InterlockedCompareExchange(&g_legacy_download_total_kb, 0, 0);
        pct = total_kb > 0 ? (int)((done_kb * 100) / total_kb) : 0;
        if (pct > 99) pct = 99;
        legacy_download_set_state(CLASSIC_INSTALL_DOWNLOADING, pct, "Downloading legacy assets");
    }
    return 0;
}

static DWORD WINAPI legacy_assets_download_master(LPVOID arg) {
    int mask = (int)(intptr_t)arg;
    ClassicAssetDownloadCtx ctx;
    HANDLE threads[LEGACY_ASSET_THREADS];
    int started = 0, worker_count = 0;
    unsigned long long total_missing = 0;
    int missing_count = 0;
    memset(threads, 0, sizeof(threads));
    memset(&ctx, 0, sizeof(ctx));
    legacy_download_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Preparing legacy assets...");
    if (!legacy_assets_index_ready()) {
        legacy_assets_refresh_index();
        while (InterlockedCompareExchange(&g_legacy_index_state, 0, 0) == CLASSIC_SIZE_FETCHING) Sleep(20);
        if (!legacy_assets_index_ready()) { legacy_download_fail(g_legacy_index_error[0] ? g_legacy_index_error : "Could not fetch legacy.json"); return 0; }
    }
    legacy_assets_recompute_status();
    mask &= InterlockedCompareExchange(&g_legacy_missing_mask, 0, 0) & LEGACY_ASSET_ALL;
    InterlockedExchange(&g_legacy_download_mask, mask);
    if (!mask) { legacy_download_set_state(CLASSIC_INSTALL_DONE, 100, "Done"); return 0; }
    for (int i = 0; i < g_legacy_asset_count; ++i) {
        ClassicAsset *a = &g_legacy_assets[i];
        if ((a->category & mask) && file_size_bytes(a->out_path) != (unsigned long long)a->size) {
            total_missing += a->size;
            missing_count++;
        }
    }
    if (missing_count <= 0) { legacy_download_set_state(CLASSIC_INSTALL_DONE, 100, "Done"); return 0; }
    InterlockedExchange(&g_legacy_download_total_kb, (LONG)((total_missing + 1023ULL) / 1024ULL));
    InterlockedExchange(&g_legacy_download_done_kb, 0);
    ctx.assets = g_legacy_assets;
    ctx.count = g_legacy_asset_count;
    ctx.mask = mask;
    InterlockedExchange(&ctx.next_index, 0);
    InterlockedExchange(&ctx.completed, 0);
    InterlockedExchange(&ctx.failed, 0);
    worker_count = missing_count < LEGACY_ASSET_THREADS ? missing_count : LEGACY_ASSET_THREADS;
    if (worker_count < 1) worker_count = 1;
    legacy_download_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading legacy assets");
    for (int i = 0; i < worker_count; ++i) {
        threads[i] = CreateThread(NULL, 0, legacy_asset_download_worker, &ctx, 0, NULL);
        if (!threads[i]) { InterlockedExchange(&ctx.failed, 1); break; }
        started++;
    }
    for (int i = 0; i < started; ++i) { WaitForSingleObject(threads[i], INFINITE); CloseHandle(threads[i]); }
    if (InterlockedCompareExchange(&ctx.failed, 0, 0)) {
        legacy_download_fail(InterlockedCompareExchange(&ctx.failed, 0, 0) == 2 ? "Canceled" : "Could not download legacy assets");
        return 0;
    }
    if (mask & LEGACY_ASSET_LANG) {
        pex_write_legacy_languages_txt();
        /* Make newly downloaded legacy languages visible immediately; otherwise
           the language list can stay cached until the next client launch. */
        pex_language_refresh_available();
        pex_set_language_code(g_opts.language);
    }
    legacy_assets_recompute_status();
    pex_sound_rescan();
    if (mask & CLASSIC_AUDIO_MENU_MUSIC) {
        /* Menu music checks files directly; kick it once so newly downloaded
           menu tracks can start without restarting the client. */
        pex_menu_music_stop();
        pex_menu_music_start_once();
    }
    legacy_download_set_state(CLASSIC_INSTALL_DONE, 100, "Done");
    return 0;
}

static int legacy_assets_is_downloading(void) {
    LONG st = InterlockedCompareExchange(&g_legacy_download_state, 0, 0);
    return st == CLASSIC_INSTALL_DOWNLOADING || st == CLASSIC_INSTALL_EXTRACTING;
}

static void legacy_assets_start_download(int mask) {
    LONG old;
    if (legacy_assets_is_downloading()) return;
    InterlockedExchange(&g_legacy_download_cancel, 0);
    InterlockedExchange(&g_legacy_download_mask, mask & LEGACY_ASSET_ALL);
    g_legacy_download_error[0] = 0;
    old = InterlockedCompareExchange(&g_legacy_download_state, CLASSIC_INSTALL_DOWNLOADING, CLASSIC_INSTALL_IDLE);
    if (old == CLASSIC_INSTALL_DONE || old == CLASSIC_INSTALL_ERROR) InterlockedExchange(&g_legacy_download_state, CLASSIC_INSTALL_DOWNLOADING);
    if (g_legacy_download_thread) { CloseHandle(g_legacy_download_thread); g_legacy_download_thread = NULL; }
    g_legacy_download_thread = CreateThread(NULL, 0, legacy_assets_download_master, (LPVOID)(intptr_t)(mask & LEGACY_ASSET_ALL), 0, NULL);
    if (!g_legacy_download_thread) legacy_download_fail("Could not start legacy asset download");
}

static void legacy_assets_request_cancel(void) {
    InterlockedExchange(&g_legacy_download_cancel, 1);
    legacy_download_set_state(CLASSIC_INSTALL_DOWNLOADING, InterlockedCompareExchange(&g_legacy_download_progress, 0, 0), "Canceling...");
}

static void legacy_assets_tick(void) {
    LONG st = InterlockedCompareExchange(&g_legacy_download_state, 0, 0);
    if (st == CLASSIC_INSTALL_DONE || st == CLASSIC_INSTALL_ERROR) {
        if (g_legacy_download_thread) { WaitForSingleObject(g_legacy_download_thread, 0); CloseHandle(g_legacy_download_thread); g_legacy_download_thread = NULL; }
        if (st == CLASSIC_INSTALL_DONE) {
            legacy_assets_recompute_status();
            InterlockedExchange(&g_legacy_download_state, CLASSIC_INSTALL_IDLE);
            InterlockedExchange(&g_legacy_download_mask, 0);
            rebuild_screen();
        } else {
            InterlockedExchange(&g_legacy_download_state, CLASSIC_INSTALL_IDLE);
            InterlockedExchange(&g_legacy_download_mask, 0);
            InterlockedExchange(&g_legacy_download_cancel, 0);
            rebuild_screen();
        }
    }
    if (InterlockedCompareExchange(&g_legacy_index_state, 0, 0) != CLASSIC_SIZE_FETCHING && g_legacy_index_thread) {
        WaitForSingleObject(g_legacy_index_thread, 0);
        CloseHandle(g_legacy_index_thread);
        g_legacy_index_thread = NULL;
    }
}

static void legacy_assets_progress_line(char *out, size_t cap) {
    LONG done = InterlockedCompareExchange(&g_legacy_download_done_kb, 0, 0);
    LONG total = InterlockedCompareExchange(&g_legacy_download_total_kb, 0, 0);
    int pct = (int)InterlockedCompareExchange(&g_legacy_download_progress, 0, 0);
    char d[64], t[64], l[64];
    unsigned long long done_b = (unsigned long long)done * 1024ULL;
    unsigned long long total_b = (unsigned long long)total * 1024ULL;
    unsigned long long left_b = total_b > done_b ? total_b - done_b : 0;
    pex_format_size(done_b, d, sizeof(d));
    pex_format_size(total_b, t, sizeof(t));
    pex_format_size(left_b, l, sizeof(l));
    snprintf(out, cap, "%d%% - %s / %s, %s left", pct, d, t, l);
}

static int legacy_sound_index_download_size(int audio_mask, unsigned long long *out_bytes, int *out_count) {
    char *json = NULL;
    size_t len = 0;
    unsigned long long total = 0;
    int count = 0;
    int ok;
    if (out_bytes) *out_bytes = 0;
    if (out_count) *out_count = 0;
    if (!http_download_to_memory(CLASSIC_SOUNDS_INDEX_URL, &json, &len, CLASSIC_LEGACY_JSON_MAX)) return 0;
    ok = legacy_sound_parse_index(json, len, audio_mask, NULL, &count, &total);
    free(json);
    if (!ok) return 0;
    if (out_bytes) *out_bytes = total;
    if (out_count) *out_count = count;
    return 1;
}

static void format_download_size(char *out, size_t cap) {
    LONG state = InterlockedCompareExchange(&g_classic_download_size_state, 0, 0);
    if (state == CLASSIC_SIZE_READY) {
        LONG tex = InterlockedCompareExchange(&g_classic_texture_download_size_bytes, 0, 0);
        if (tex < 0) snprintf(out, cap, "Textures: installed");
        else if (tex > 0) snprintf(out, cap, "Textures: %.2f MB", (double)tex / (1024.0 * 1024.0));
        else snprintf(out, cap, "Textures: unavailable");
        return;
    } else if (state == CLASSIC_SIZE_FETCHING) {
        snprintf(out, cap, "Download size: checking Mojang...");
        return;
    }
    snprintf(out, cap, "Download size: unavailable");
}

static DWORD WINAPI pack_install_size_worker(LPVOID unused) {
    (void)unused;
    int need_textures = g_opts.download_classic_textures && (!pack_is_installed() || pack_missing_required_textures());
    InterlockedExchange(&g_classic_texture_download_size_bytes, need_textures ? (LONG)CLASSIC_CLIENT_JAR_SIZE : -1);
    InterlockedExchange(&g_classic_sound_download_size_bytes, -1);
    InterlockedExchange(&g_classic_sound_download_count, 0);
    InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_READY);
    return 0;
}

static void pack_install_start_size_fetch(void) {
    LONG old = InterlockedCompareExchange(&g_classic_download_size_state, CLASSIC_SIZE_FETCHING, CLASSIC_SIZE_UNKNOWN);
    if (old != CLASSIC_SIZE_UNKNOWN) return;
    HANDLE th = CreateThread(NULL, 0, pack_install_size_worker, NULL, 0, NULL);
    if (th) CloseHandle(th);
    else InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_ERROR);
}

static int pack_install_download_client_jar_linux(const char *url, const char *zip_path) {
    unsigned int expect = CLASSIC_CLIENT_JAR_SIZE;
    pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 1, "Downloading 1.2.5 client.jar...");
    DeleteFileA(zip_path);
    if (pack_install_is_cancelled()) { pack_install_fail("Canceled"); return 0; }
    if (!http_download_to_file(url, zip_path, expect)) {
        if (pack_install_is_cancelled()) pack_install_fail("Canceled");
        else pack_install_fail("Could not download 1.2.5 client.jar. Install python3/wget or pre-copy the jar.");
        return 0;
    }
    pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 85, "Downloaded 1.2.5 client.jar");
    return 1;
}

static int pack_install_extract_archive_linux(const char *zip_path, const char *pack_dir) {
    char err[MAX_LABEL];
    err[0] = 0;
    pack_install_set_state(CLASSIC_INSTALL_EXTRACTING, 90, "Extracting textures...");
    if (!pxc_extract_zip_file(zip_path, pack_dir, err, sizeof(err))) {
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

static DWORD WINAPI legacy_sound_download_worker(LPVOID arg) {
    /* Kept for old call sites; new Options->Assets path uses generic legacy downloader. */
    return legacy_asset_download_worker(arg);
}

static int legacy_sound_download_selected(int audio_mask) {
    legacy_assets_start_download(audio_mask & CLASSIC_AUDIO_ALL);
    while (legacy_assets_is_downloading()) Sleep(50);
    return InterlockedCompareExchange(&g_legacy_download_state, 0, 0) != CLASSIC_INSTALL_ERROR;
}

static DWORD WINAPI pack_install_worker(LPVOID unused) {
    (void)unused;
    char zip_path[MAX_PATHBUF];
    char pack_dir[MAX_PATHBUF];
    int need_textures = g_opts.download_classic_textures && (!pack_is_installed() || pack_missing_required_textures());
    ensure_dir(g_texpack_dir);
    pack_asset_path(pack_dir, sizeof(pack_dir));
    snprintf(zip_path, sizeof(zip_path), "%s/minecraft_1_2_5_client.jar", g_mc_dir);
    if (need_textures) {
        pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading 1.2.5 client.jar...");
        if (!pack_install_download_client_jar_linux(CLASSIC_PACK_URL, zip_path)) return 0;
        if (!pack_install_extract_archive_linux(zip_path, pack_dir)) return 0;
        log_msg("Installed Minecraft 1.2.5 release texture pack at %s", pack_dir);
    }
    pack_install_set_state(CLASSIC_INSTALL_DONE, 100, "Done!");
    return 0;
}

static int pack_resources_install_blocking(void) {
    char zip_path[MAX_PATHBUF];
    char pack_dir[MAX_PATHBUF];
    int need_textures;
    ensure_dir(g_texpack_dir);
    pack_asset_path(pack_dir, sizeof(pack_dir));
    snprintf(zip_path, sizeof(zip_path), "%s/minecraft_1_2_5_client.jar", g_mc_dir);
    need_textures = g_opts.download_classic_textures && (!pack_is_installed() || pack_missing_required_textures());
    if (!need_textures) return 1;
    g_classic_install_error[0] = 0;
    pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading 1.2.5 client.jar...");
    if (!pack_install_download_client_jar_linux(CLASSIC_PACK_URL, zip_path)) return 0;
    if (!pack_install_extract_archive_linux(zip_path, pack_dir)) return 0;
    pack_install_set_state(CLASSIC_INSTALL_DONE, 100, "Done!");
    return 1;
}

static void pack_install_start(void) {
    LONG state;
    pack_install_reset_cancel();
    state = InterlockedCompareExchange(&g_classic_install_state, CLASSIC_INSTALL_DOWNLOADING, CLASSIC_INSTALL_IDLE);
    if (state == CLASSIC_INSTALL_DOWNLOADING || state == CLASSIC_INSTALL_EXTRACTING) { set_screen(SCREEN_TEXPACK_INSTALL); return; }
    if (g_classic_install_thread) { CloseHandle(g_classic_install_thread); g_classic_install_thread = NULL; }
    g_classic_install_error[0] = 0;
    pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading Release textures...");
    set_screen(SCREEN_TEXPACK_INSTALL);
    g_classic_install_thread = CreateThread(NULL, 0, pack_install_worker, NULL, 0, NULL);
    if (!g_classic_install_thread) pack_install_fail("Could not start installer thread");
}

static void pack_install_tick(void) {
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
        if (!strcmp(g_classic_install_error, "Canceled")) set_screen(SCREEN_TITLE);
        else open_notice("Texture Pack", "Could not install Release textures.", g_classic_install_error);
    }
}
