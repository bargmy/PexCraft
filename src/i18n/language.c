/* Minecraft 1.2.5-style runtime language support.
   No official language data is linked into the executable.  The client loads
   /lang/languages.txt and /lang/*.lang extracted from Minecraft 1.2.5 client.jar
   into the installed Release texture pack directory at runtime. */

#define PEX_LANG_MAX_LANGUAGES 256
#define PEX_LANG_CODE_MAX 24
#define PEX_LANG_NAME_MAX 160
#define PEX_LANG_MAX_LINE 4096

typedef struct PexLanguageInfo {
    char code[PEX_LANG_CODE_MAX];
    char name[PEX_LANG_NAME_MAX];
} PexLanguageInfo;

typedef struct PexLangKV {
    char *key;
    char *value;
} PexLangKV;

static PexLanguageInfo g_pex_langs[PEX_LANG_MAX_LANGUAGES];
static int g_pex_lang_count = 0;
static int g_lang_index = 0;
static int g_lang_unicode = 0;
static int g_lang_bidi = 0;
static int g_lang_files_available = 0;
static int g_lang_list_loaded = 0;
static PexLangKV *g_lang_table = NULL;
static int g_lang_table_count = 0;
static int g_lang_table_cap = 0;
static int g_language_scroll = 0;
static int g_language_drag_scroll_pixels = 0;

static int pex_lang_str_eq(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) { if (*a++ != *b++) return 0; }
    return *a == 0 && *b == 0;
}

static int pex_lang_ascii_cmp(const char *a, const char *b) {
    if (!a) a = "";
    if (!b) b = "";
    while (*a && *b) {
        unsigned char ca = (unsigned char)*a++;
        unsigned char cb = (unsigned char)*b++;
        if (ca != cb) return (int)ca - (int)cb;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

static char *pex_lang_strdup(const char *s) {
    size_t n;
    char *out;
    if (!s) s = "";
    n = strlen(s);
    out = (char *)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, s, n + 1);
    return out;
}

static char *pex_lang_trim(char *s) {
    char *e;
    if (!s) return s;
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') ++s;
    e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) *--e = 0;
    return s;
}

static int pex_lang_utf8_has_java_unicode(const char *s) {
    const unsigned char *p = (const unsigned char *)s;
    while (p && *p) {
        unsigned int cp;
        if (*p < 0x80u) { ++p; continue; }
        if ((*p & 0xE0u) == 0xC0u && p[1]) {
            cp = ((*p & 0x1Fu) << 6) | (p[1] & 0x3Fu);
            p += 2;
        } else if ((*p & 0xF0u) == 0xE0u && p[1] && p[2]) {
            cp = ((*p & 0x0Fu) << 12) | ((p[1] & 0x3Fu) << 6) | (p[2] & 0x3Fu);
            p += 3;
        } else if ((*p & 0xF8u) == 0xF0u && p[1] && p[2] && p[3]) {
            cp = ((*p & 0x07u) << 18) | ((p[1] & 0x3Fu) << 12) | ((p[2] & 0x3Fu) << 6) | (p[3] & 0x3Fu);
            p += 4;
        } else {
            ++p;
            cp = 0x100u;
        }
        if (cp >= 256u) return 1;
    }
    return 0;
}

static void pex_lang_pack_root(char *out, size_t cap) {
    pack_asset_path(out, cap);
}

static void pex_lang_dir(char *out, size_t cap) {
    char root[MAX_PATHBUF];
    pex_lang_pack_root(root, sizeof(root));
    path_join(out, cap, root, "lang");
}

static void pex_lang_file_path(char *out, size_t cap, const char *name) {
    char dir[MAX_PATHBUF];
    pex_lang_dir(dir, sizeof(dir));
    path_join(out, cap, dir, name);
}

static int pex_file_exists_lang(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static int pex_language_files_installed(void) {
    char a[MAX_PATHBUF], b[MAX_PATHBUF];
    pex_lang_file_path(a, sizeof(a), "languages.txt");
    pex_lang_file_path(b, sizeof(b), "en_US.lang");
    return pex_file_exists_lang(a) && pex_file_exists_lang(b);
}

static void pex_lang_table_clear(void) {
    for (int i = 0; i < g_lang_table_count; ++i) {
        free(g_lang_table[i].key);
        free(g_lang_table[i].value);
    }
    free(g_lang_table);
    g_lang_table = NULL;
    g_lang_table_count = 0;
    g_lang_table_cap = 0;
}

static int pex_lang_table_set(const char *key, const char *value) {
    if (!key || !*key || !value) return 0;
    for (int i = 0; i < g_lang_table_count; ++i) {
        if (pex_lang_str_eq(g_lang_table[i].key, key)) {
            char *nv = pex_lang_strdup(value);
            if (!nv) return 0;
            free(g_lang_table[i].value);
            g_lang_table[i].value = nv;
            return 1;
        }
    }
    if (g_lang_table_count >= g_lang_table_cap) {
        int nc = g_lang_table_cap ? g_lang_table_cap * 2 : 512;
        PexLangKV *nt = (PexLangKV *)realloc(g_lang_table, (size_t)nc * sizeof(PexLangKV));
        if (!nt) return 0;
        g_lang_table = nt;
        g_lang_table_cap = nc;
    }
    g_lang_table[g_lang_table_count].key = pex_lang_strdup(key);
    g_lang_table[g_lang_table_count].value = pex_lang_strdup(value);
    if (!g_lang_table[g_lang_table_count].key || !g_lang_table[g_lang_table_count].value) return 0;
    ++g_lang_table_count;
    return 1;
}

static int pex_lang_info_cmp_qsort(const void *aa, const void *bb) {
    const PexLanguageInfo *a = (const PexLanguageInfo *)aa;
    const PexLanguageInfo *b = (const PexLanguageInfo *)bb;
    return pex_lang_ascii_cmp(a->code, b->code);
}

static void pex_language_add_bootstrap_en(void) {
    g_pex_lang_count = 1;
    snprintf(g_pex_langs[0].code, sizeof(g_pex_langs[0].code), "en_US");
    snprintf(g_pex_langs[0].name, sizeof(g_pex_langs[0].name), "English (US)");
    g_lang_index = 0;
}

static int pex_language_load_list_from_disk(void) {
    char path[MAX_PATHBUF];
    FILE *f;
    char line[PEX_LANG_MAX_LINE];
    g_pex_lang_count = 0;
    g_lang_files_available = pex_language_files_installed();
    pex_lang_file_path(path, sizeof(path), "languages.txt");
    f = fopen(path, "rb");
    if (!f) {
        pex_language_add_bootstrap_en();
        g_lang_list_loaded = 1;
        return 0;
    }
    while (fgets(line, sizeof(line), f)) {
        char *s = pex_lang_trim(line);
        char *eq;
        if (!*s || *s == '#') continue;
        eq = strchr(s, '=');
        if (!eq) continue;
        *eq++ = 0;
        s = pex_lang_trim(s);
        eq = pex_lang_trim(eq);
        if (!*s || !*eq) continue;
        if (g_pex_lang_count >= PEX_LANG_MAX_LANGUAGES) break;
        snprintf(g_pex_langs[g_pex_lang_count].code, sizeof(g_pex_langs[g_pex_lang_count].code), "%s", s);
        snprintf(g_pex_langs[g_pex_lang_count].name, sizeof(g_pex_langs[g_pex_lang_count].name), "%s", eq);
        ++g_pex_lang_count;
    }
    fclose(f);
    if (g_pex_lang_count <= 0) pex_language_add_bootstrap_en();
    else qsort(g_pex_langs, (size_t)g_pex_lang_count, sizeof(g_pex_langs[0]), pex_lang_info_cmp_qsort);
    g_lang_list_loaded = 1;
    return g_lang_files_available;
}

static int pex_lang_find_index(const char *code) {
    if (!g_lang_list_loaded) pex_language_load_list_from_disk();
    if (!code || !*code) code = "en_US";
    for (int i = 0; i < g_pex_lang_count; ++i) if (pex_lang_str_eq(g_pex_langs[i].code, code)) return i;
    for (int i = 0; i < g_pex_lang_count; ++i) if (pex_lang_str_eq(g_pex_langs[i].code, "en_US")) return i;
    return 0;
}

static int pex_language_load_lang_file_into_table(const char *code) {
    char filename[64];
    char path[MAX_PATHBUF];
    FILE *f;
    char line[PEX_LANG_MAX_LINE];
    if (!code || !*code) return 0;
    snprintf(filename, sizeof(filename), "%s.lang", code);
    pex_lang_file_path(path, sizeof(path), filename);
    f = fopen(path, "rb");
    if (!f) return 0;
    while (fgets(line, sizeof(line), f)) {
        char *s = pex_lang_trim(line);
        char *eq;
        if (!*s || *s == '#') continue;
        eq = strchr(s, '=');
        if (!eq) continue;
        *eq++ = 0;
        s = pex_lang_trim(s);
        eq = pex_lang_trim(eq);
        if (*s) pex_lang_table_set(s, eq);
    }
    fclose(f);
    return 1;
}

static void pex_language_refresh_flags(const char *code) {
    g_lang_unicode = 0;
    g_lang_bidi = (pex_lang_str_eq(code, "ar_SA") || pex_lang_str_eq(code, "he_IL"));
    for (int i = 0; i < g_lang_table_count; ++i) {
        if (pex_lang_utf8_has_java_unicode(g_lang_table[i].value)) { g_lang_unicode = 1; break; }
    }
}

static void pex_language_reload_current(void) {
    const char *code;
    if (!g_lang_list_loaded) pex_language_load_list_from_disk();
    code = g_pex_langs[g_lang_index].code;
    pex_lang_table_clear();
    pex_language_load_lang_file_into_table("en_US");
    if (!pex_lang_str_eq(code, "en_US")) pex_language_load_lang_file_into_table(code);
    pex_language_refresh_flags(code);
}

static void pex_language_refresh_available(void) {
    char keep[PEX_LANG_CODE_MAX];
    keep[0] = 0;
    if (g_lang_list_loaded && g_lang_index >= 0 && g_lang_index < g_pex_lang_count) snprintf(keep, sizeof(keep), "%s", g_pex_langs[g_lang_index].code);
    g_lang_list_loaded = 0;
    pex_language_load_list_from_disk();
    g_lang_index = pex_lang_find_index(keep[0] ? keep : "en_US");
    pex_language_reload_current();
}

static void pex_set_language_code(const char *code) {
    if (!g_lang_list_loaded) pex_language_load_list_from_disk();
    g_lang_index = pex_lang_find_index(code);
    pex_language_reload_current();
}

static int pex_language_count(void) {
    if (!g_lang_list_loaded) pex_language_load_list_from_disk();
    return g_pex_lang_count;
}
static const char *pex_language_code_at(int idx) {
    if (!g_lang_list_loaded) pex_language_load_list_from_disk();
    if (idx < 0 || idx >= g_pex_lang_count) idx = 0;
    return g_pex_langs[idx].code;
}
static const char *pex_language_name_at(int idx) {
    if (!g_lang_list_loaded) pex_language_load_list_from_disk();
    if (idx < 0 || idx >= g_pex_lang_count) idx = 0;
    return g_pex_langs[idx].name[0] ? g_pex_langs[idx].name : g_pex_langs[idx].code;
}
static int pex_current_language_index(void) { if (!g_lang_list_loaded) pex_language_load_list_from_disk(); return g_lang_index; }
static const char *pex_current_language_code(void) { return pex_language_code_at(g_lang_index); }
static int pex_current_language_is_unicode(void) { return g_lang_unicode; }
static int pex_current_language_is_bidi(void) { return g_lang_bidi; }
static int pex_language_runtime_files_available(void) { return pex_language_files_installed(); }

static const char *tr_key_default(const char *key, const char *fallback) {
    if (!key) return fallback ? fallback : "";
    if (!g_lang_list_loaded) pex_set_language_code(g_opts.language[0] ? g_opts.language : "en_US");
    for (int i = 0; i < g_lang_table_count; ++i) {
        if (pex_lang_str_eq(g_lang_table[i].key, key)) return g_lang_table[i].value;
    }
    return fallback ? fallback : key;
}

static const char *tr_key(const char *key) {
    return tr_key_default(key, key);
}

typedef struct PexLangAlias { const char *english; const char *key; } PexLangAlias;
static const PexLangAlias pex_lang_aliases[] = {
    {"Singleplayer", "menu.singleplayer"},
    {"Multiplayer", "menu.multiplayer"},
    {"Options...", "menu.options"},
    {"Options", "options.title"},
    {"Quit Game", "menu.quit"},
    {"Done", "gui.done"},
    {"Cancel", "gui.cancel"},
    {"Yes", "gui.yes"},
    {"No", "gui.no"},
    {"Back", "gui.back"},
    {"Video Settings", "options.videoTitle"},
    {"Video Settings...", "options.videoTitle"},
    {"Controls", "options.controls"},
    {"Controls...", "options.controls"},
    {"Language", "options.language"},
    {"Texture Packs", "texturePack.title"},
    {"Open texture pack folder", "texturePack.openFolder"},
    {"Create New World", "selectWorld.create"},
    {"Play Selected World", "selectWorld.play"},
    {"Rename", "selectWorld.rename"},
    {"Delete", "selectWorld.delete"},
    {"Connect", "selectServer.select"},
    {"Back to game", "menu.returnToGame"},
    {"Disconnect", "menu.disconnect"},
    {"Save and quit to title", "menu.returnToMenu"},
    {"Respawn", "deathScreen.respawn"},
    {"Title menu", "deathScreen.titleScreen"},
    {"Inventory", "container.inventory"},
    {"Crafting", "container.crafting"},
    {"Furnace", "container.furnace"},
    {"Chest", "container.chest"},
    {"Item Selection", "container.creative"},
    {"ON", "options.on"},
    {"OFF", "options.off"},
    {"Fancy", "options.graphics.fancy"},
    {"Fast", "options.graphics.fast"},
    {"Peaceful", "options.difficulty.peaceful"},
    {"Easy", "options.difficulty.easy"},
    {"Normal", "options.difficulty.normal"},
    {"Hard", "options.difficulty.hard"},
    {"Unlimited", "options.framerateLimit.max"}
};

static const char *tr(const char *en) {
    if (!en) return "";
    for (int i = 0; i < ARRAY_COUNT(pex_lang_aliases); ++i) {
        if (pex_lang_str_eq(en, pex_lang_aliases[i].english)) return tr_key_default(pex_lang_aliases[i].key, en);
    }
    return en;
}

static int pex_language_visible_rows(void) {
    int top = 32;
    int bottom = g_gui_h - 65 + 4;
    int rows = (bottom - top) / 18;
    return rows > 1 ? rows : 1;
}

static void language_clamp_scroll(void) {
    int max_scroll = pex_language_count() - pex_language_visible_rows();
    if (max_scroll < 0) max_scroll = 0;
    if (g_language_scroll < 0) g_language_scroll = 0;
    if (g_language_scroll > max_scroll) g_language_scroll = max_scroll;
}

static void language_scroll_by(int rows) {
    g_language_scroll += rows;
    language_clamp_scroll();
    rebuild_screen();
}

static void language_ensure_selected_visible(void) {
    int vis = pex_language_visible_rows();
    int cur = pex_current_language_index();
    if (cur < g_language_scroll) g_language_scroll = cur;
    if (cur >= g_language_scroll + vis) g_language_scroll = cur - vis + 1;
    language_clamp_scroll();
}

static void language_drag_scroll(int delta_y) {
    int rows = 0;
    g_language_drag_scroll_pixels -= delta_y;
    while (g_language_drag_scroll_pixels >= 18) { rows++; g_language_drag_scroll_pixels -= 18; }
    while (g_language_drag_scroll_pixels <= -18) { rows--; g_language_drag_scroll_pixels += 18; }
    if (rows) language_scroll_by(rows);
}

static int pex_language_download_from_jar_blocking(void) {
    char zip_path[MAX_PATHBUF];
    char pack_dir[MAX_PATHBUF];
    char err[MAX_LABEL];
    int ok = 0;
    ensure_dir(g_texpack_dir);
    pack_asset_path(pack_dir, sizeof(pack_dir));
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII) || defined(PEX_PLATFORM_ANDROID_TV) || defined(PEX_PLATFORM_LGWEBOS)
    pack_install_fail("Runtime language download from client.jar is not available on this build");
    return 0;
#elif defined(PEX_PLATFORM_ANDROID)
    pack_install_fail("Use the Release texture-pack download first; Android downloads client.jar through Java and extracts /lang with the pack");
    return 0;
#elif defined(PEX_PLATFORM_SDL2)
    snprintf(zip_path, sizeof(zip_path), "%s/minecraft_1_2_5_client.jar", g_mc_dir);
    pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading 1.2.5 client.jar...");
    if (!pack_install_download_client_jar_linux(CLASSIC_PACK_URL, zip_path)) return 0;
#else
    snprintf(zip_path, sizeof(zip_path), "%s\\minecraft_1_2_5_client.jar", g_mc_dir);
    pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading 1.2.5 client.jar...");
    if (!pack_install_download_client_jar(CLASSIC_PACK_URL, zip_path)) return 0;
#endif
    err[0] = 0;
    pack_install_set_state(CLASSIC_INSTALL_EXTRACTING, 90, "Extracting languages...");
    ok = pxc_extract_zip_lang_file(zip_path, pack_dir, err, sizeof(err));
    DeleteFileA(zip_path);
    if (!ok) {
        pack_install_fail(err[0] ? err : "Could not extract /lang from client.jar");
        return 0;
    }
    pex_language_refresh_available();
    if (!pex_language_files_installed()) {
        pack_install_fail("client.jar did not contain /lang/languages.txt and en_US.lang");
        return 0;
    }
    pack_install_set_state(CLASSIC_INSTALL_DONE, 100, "Languages installed");
    return 1;
}
