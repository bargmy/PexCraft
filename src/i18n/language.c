/* Minecraft 1.2.5-style runtime language support.
   No official language data is linked into the executable.  The client loads
   lang/languages.txt and per-language .lang files extracted from Minecraft 1.2.5 client.jar
   into the installed Release texture pack directory at runtime. */

static void rebuild_screen(void);

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
static int g_language_scroll = 0; /* Java GuiSlot.amountScrolled, in pixels. */
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

static int pex_lang_split_java_equals(char *line, char **out_key, char **out_value) {
    /* StringTranslate.loadLanguage/loadLanguageList used String.split("=") and
       accepted only length == 2.  That means lines with no '=' or extra '=' are
       ignored instead of treating the first '=' as a delimiter. */
    char *eq;
    if (out_key) *out_key = NULL;
    if (out_value) *out_value = NULL;
    if (!line) return 0;
    eq = strchr(line, '=');
    if (!eq) return 0;
    if (strchr(eq + 1, '=')) return 0;
    *eq++ = 0;
    if (out_key) *out_key = pex_lang_trim(line);
    if (out_value) *out_value = pex_lang_trim(eq);
    return 1;
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

static const char *pex_language_builtin_native_name(const char *code) {
    static const PexLanguageInfo names[] = {
        {"af_ZA", "Afrikaans"}, {"ar_SA", "العربية"}, {"bg_BG", "Български"}, {"ca_ES", "Català"},
        {"cs_CZ", "Čeština"}, {"cy_GB", "Cymraeg"}, {"da_DK", "Dansk"}, {"de_DE", "Deutsch"},
        {"el_GR", "Ελληνικά"}, {"en_AU", "English (Australia)"}, {"en_CA", "English (Canada)"},
        {"en_GB", "English (UK)"}, {"en_PT", "Pirate Speak"}, {"en_US", "English (US)"},
        {"eo_UY", "Esperanto"}, {"es_AR", "Español (Argentina)"}, {"es_ES", "Español (España)"},
        {"es_MX", "Español (México)"}, {"es_UY", "Español (Uruguay)"}, {"es_VE", "Español (Venezuela)"},
        {"et_EE", "Eesti"}, {"eu_ES", "Euskara"}, {"fa_IR", "فارسی"}, {"fi_FI", "Suomi"},
        {"fil_PH", "Filipino"}, {"fr_CA", "Français (Canada)"}, {"fr_FR", "Français"}, {"ga_IE", "Gaeilge"},
        {"gl_ES", "Galego"}, {"he_IL", "עברית"}, {"hi_IN", "हिन्दी"}, {"hr_HR", "Hrvatski"},
        {"hu_HU", "Magyar"}, {"hy_AM", "Հայերեն"}, {"id_ID", "Bahasa Indonesia"}, {"is_IS", "Íslenska"},
        {"it_IT", "Italiano"}, {"ja_JP", "日本語"}, {"ka_GE", "ქართული"}, {"ko_KR", "한국어"},
        {"kw_GB", "Kernewek"}, {"la_LA", "Latina"}, {"lb_LU", "Lëtzebuergesch"}, {"lt_LT", "Lietuvių"},
        {"lv_LV", "Latviešu"}, {"ms_MY", "Bahasa Melayu"}, {"mt_MT", "Malti"}, {"nl_NL", "Nederlands"},
        {"nn_NO", "Norsk nynorsk"}, {"no_NO", "Norsk bokmål"}, {"oc_FR", "Occitan"}, {"pl_PL", "Polski"},
        {"pt_BR", "Português (Brasil)"}, {"pt_PT", "Português (Portugal)"}, {"qya_AA", "Quenya"},
        {"ro_RO", "Română"}, {"ru_RU", "Русский"}, {"sk_SK", "Slovenčina"}, {"sl_SI", "Slovenščina"},
        {"sr_SP", "Српски"}, {"sv_SE", "Svenska"}, {"th_TH", "ไทย"}, {"tlh_AA", "tlhIngan Hol"},
        {"tr_TR", "Türkçe"}, {"uk_UA", "Українська"}, {"vi_VN", "Tiếng Việt"},
        {"zh_CN", "简体中文"}, {"zh_TW", "繁體中文"}
    };
    if (!code) return NULL;
    for (int i = 0; i < (int)ARRAY_COUNT(names); ++i) {
        if (pex_lang_str_eq(names[i].code, code)) return names[i].name;
    }
    return NULL;
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
        if (!pex_lang_split_java_equals(s, &s, &eq)) continue;
        if (!*s || !*eq) continue;
        if (g_pex_lang_count >= PEX_LANG_MAX_LANGUAGES) break;
        const char *native_name = pex_language_builtin_native_name(s);
        snprintf(g_pex_langs[g_pex_lang_count].code, sizeof(g_pex_langs[g_pex_lang_count].code), "%s", s);
        snprintf(g_pex_langs[g_pex_lang_count].name, sizeof(g_pex_langs[g_pex_lang_count].name), "%s", native_name ? native_name : eq);
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
        if (!pex_lang_split_java_equals(s, &s, &eq)) continue;
        if (*s && *eq) pex_lang_table_set(s, eq);
    }
    fclose(f);
    return 1;
}

static void pex_language_refresh_flags(const char *code) {
    g_lang_unicode = 0;
    g_lang_bidi = (pex_lang_str_eq(code, "ar_SA") || pex_lang_str_eq(code, "fa_IR") || pex_lang_str_eq(code, "he_IL"));
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

static const char *pex_lang_lookup_loaded_key(const char *key) {
    if (!key || !*key) return NULL;
    for (int i = 0; i < g_lang_table_count; ++i) {
        if (pex_lang_str_eq(g_lang_table[i].key, key)) return g_lang_table[i].value;
    }
    return NULL;
}

typedef struct PexLangKeyAlias { const char *primary; const char *secondary; } PexLangKeyAlias;
static const PexLangKeyAlias pex_lang_key_aliases[] = {
    /* legacy.json language files are from the newer legacy asset index.  Many
       translations renamed texture-pack keys to resource-pack keys; the 1.2.5
       UI still asks for texturePack.*, so bridge those names instead of
       falling back to English. */
    {"texturePack.title", "resourcePack.title"},
    {"texturePack.openFolder", "resourcePack.openFolder"},
    {"texturePack.folderInfo", "resourcePack.folderInfo"},
    {"menu.game", "menu.paused"},

    /* The launcher legacy asset index carries newer language files, while this
       port still has many Java 1.2.5 internal keys.  Bridge old item/block
       keys to the newer keys when the old value is missing or still English. */
    {"item.emerald.name", "item.diamond.name"},
    {"tile.sign.name", "item.sign.name"},
    {"tile.doorWood.name", "item.doorWood.name"},
    {"tile.doorIron.name", "item.doorIron.name"},
    {"tile.reeds.name", "item.reeds.name"},
    {"tile.cake.name", "item.cake.name"},
    {"tile.bed.name", "item.bed.name"},
    {"tile.diode.name", "item.diode.name"},
    {"tile.melon.name", "item.melon.name"},
    {"tile.netherStalk.name", "item.netherStalkSeeds.name"},
    {"tile.brewingStand.name", "item.brewingStand.name"},
    {"tile.cauldron.name", "item.cauldron.name"},
    {"tile.brick.name", "item.brick.name"},
    {"tile.clay.name", "item.clay.name"}
};

static int pex_lang_value_is_bridge_english(const char *key, const char *value, const char *fallback) {
    if (!value || !*value) return 1;
    if (fallback && pex_lang_str_eq(value, fallback)) return 1;
    if (pex_lang_str_eq(key, "texturePack.title")) {
        return pex_lang_str_eq(value, "Resource Packs") ||
               pex_lang_str_eq(value, "Select Resource Pack") ||
               pex_lang_str_eq(value, "Select Texture Pack");
    }
    if (pex_lang_str_eq(key, "texturePack.openFolder")) {
        return pex_lang_str_eq(value, "Open resource pack folder") ||
               pex_lang_str_eq(value, "Open texture pack folder");
    }
    if (pex_lang_str_eq(key, "texturePack.folderInfo")) {
        return pex_lang_str_eq(value, "(Place resource pack files here)") ||
               pex_lang_str_eq(value, "(Place texture pack files here)");
    }
    if (pex_lang_str_eq(key, "menu.game")) {
        return pex_lang_str_eq(value, "Game menu") ||
               pex_lang_str_eq(value, "Game Menu") ||
               pex_lang_str_eq(value, "Paused");
    }
    if (pex_lang_str_eq(key, "item.emerald.name") && fallback && pex_lang_str_eq(fallback, "Diamond")) {
        return pex_lang_str_eq(value, "Emerald");
    }
    return 0;
}


typedef struct PexLangSupplement { const char *lang; const char *key; const char *value; } PexLangSupplement;
static const PexLangSupplement pex_lang_supplements[] = {
    /* fa_IR from the legacy asset index is incomplete and contains many English
       values.  Keep the downloaded language as the main source, but patch keys
       that the 1.2.5 UI needs or that are still raw English in that file. */
    {"fa_IR", "menu.game", "منوی بازی"},
    {"fa_IR", "menu.preparingSpawn", "در حال آماده‌سازی چانک‌ها"},
    {"fa_IR", "downloadTerrain", "در حال دریافت زمین"},
    {"fa_IR", "container.inventory", "موجودی"},
    {"fa_IR", "container.crafting", "ساخت‌وساز"},
    {"fa_IR", "container.furnace", "کوره"},
    {"fa_IR", "container.chest", "صندوقچه"},
    {"fa_IR", "container.creative", "انتخاب آیتم"},
    {"fa_IR", "gui.achievements", "دستاوردها"},
    {"fa_IR", "gui.stats", "آمار"},
    {"fa_IR", "deathScreen.title", "مُردید!"},
    {"fa_IR", "deathScreen.respawn", "تولد دوباره"},
    {"fa_IR", "deathScreen.titleScreen", "صفحه اصلی"},
    {"fa_IR", "deathScreen.score", "امتیاز"},
    {"fa_IR", "selectWorld.legacyUnused", "این صفحه قدیمی استفاده نمی‌شود"},
    {"fa_IR", "selectWorld.play", "بازی در دنیای انتخاب شده"},
    {"fa_IR", "item.dyePowder.name", "رنگ"},
    {"fa_IR", "item.fishRaw.name", "ماهی خام"},
    {"fa_IR", "item.fishCooked.name", "ماهی پخته"},
    {"fa_IR", "entity.Creeper.name", "کریپر"},
    {"fa_IR", "entity.Skeleton.name", "اسکلت"},
    {"fa_IR", "entity.Spider.name", "عنکبوت"},
    {"fa_IR", "entity.Giant.name", "غول"},
    {"fa_IR", "entity.Zombie.name", "زامبی"},
    {"fa_IR", "entity.Slime.name", "اسلایم"},
    {"fa_IR", "entity.Ghast.name", "گَست"},
    {"fa_IR", "entity.PigZombie.name", "زامبی خوکی"},
    {"fa_IR", "entity.Enderman.name", "اندرمن"},
    {"fa_IR", "entity.CaveSpider.name", "عنکبوت غاری"},
    {"fa_IR", "entity.Silverfish.name", "سیلورفیش"},
    {"fa_IR", "entity.Blaze.name", "بلیز"},
    {"fa_IR", "entity.LavaSlime.name", "مکعب ماگما"},
    {"fa_IR", "entity.EnderDragon.name", "اژدهای اندر"},
    {"fa_IR", "entity.Pig.name", "خوک"},
    {"fa_IR", "entity.Sheep.name", "گوسفند"},
    {"fa_IR", "entity.Cow.name", "گاو"},
    {"fa_IR", "entity.Chicken.name", "مرغ"},
    {"fa_IR", "entity.Squid.name", "ماهی مرکب"},
    {"fa_IR", "entity.Wolf.name", "گرگ"},
    {"fa_IR", "entity.MushroomCow.name", "موشروم"},
    {"fa_IR", "entity.SnowMan.name", "گولم برفی"},
    {"fa_IR", "entity.Ozelot.name", "اوسلوت"},
    {"fa_IR", "entity.VillagerGolem.name", "گولم آهنی"},
    {"fa_IR", "entity.Villager.name", "روستایی"},

    {"fa_IR", "tile.stone.name", "سنگ"},
    {"fa_IR", "tile.grass.name", "چمن"},
    {"fa_IR", "tile.dirt.name", "خاک"},
    {"fa_IR", "tile.stonebrick.name", "سنگفرش"},
    {"fa_IR", "tile.wood.name", "تخته چوبی"},
    {"fa_IR", "tile.sapling.name", "نهال"},
    {"fa_IR", "tile.bedrock.name", "سنگ بستر"},
    {"fa_IR", "tile.water.name", "آب"},
    {"fa_IR", "tile.lava.name", "لاوا"},
    {"fa_IR", "tile.sand.name", "شن"},
    {"fa_IR", "tile.gravel.name", "ریگ"},
    {"fa_IR", "tile.oreGold.name", "سنگ معدن طلا"},
    {"fa_IR", "tile.oreIron.name", "سنگ معدن آهن"},
    {"fa_IR", "tile.oreCoal.name", "سنگ معدن زغال"},
    {"fa_IR", "tile.log.name", "چوب"},
    {"fa_IR", "tile.leaves.name", "برگ‌ها"},
    {"fa_IR", "tile.sponge.name", "اسفنج"},
    {"fa_IR", "tile.glass.name", "شیشه"},
    {"fa_IR", "tile.cloth.name", "پشم"},
    {"fa_IR", "tile.flower.name", "گل"},
    {"fa_IR", "tile.rose.name", "رز"},
    {"fa_IR", "tile.mushroom.name", "قارچ"},
    {"fa_IR", "tile.blockGold.name", "بلوک طلا"},
    {"fa_IR", "tile.blockIron.name", "بلوک آهن"},
    {"fa_IR", "tile.stoneSlab.name", "نیم‌بلوک سنگی"},
    {"fa_IR", "tile.brick.name", "آجر"},
    {"fa_IR", "tile.tnt.name", "تی‌ان‌تی"},
    {"fa_IR", "tile.bookshelf.name", "قفسه کتاب"},
    {"fa_IR", "tile.stoneMoss.name", "سنگ خزه‌ای"},
    {"fa_IR", "tile.obsidian.name", "ابسیدین"},
    {"fa_IR", "tile.torch.name", "مشعل"},
    {"fa_IR", "tile.fire.name", "آتش"},
    {"fa_IR", "tile.mobSpawner.name", "تولیدکننده هیولا"},
    {"fa_IR", "tile.stairsWood.name", "پله چوبی"},
    {"fa_IR", "tile.chest.name", "صندوقچه"},
    {"fa_IR", "tile.redstoneDust.name", "غبار ردستون"},
    {"fa_IR", "tile.oreDiamond.name", "سنگ معدن الماس"},
    {"fa_IR", "tile.blockDiamond.name", "بلوک الماس"},
    {"fa_IR", "tile.workbench.name", "میز کار"},
    {"fa_IR", "tile.crops.name", "محصولات"},
    {"fa_IR", "tile.farmland.name", "زمین کشاورزی"},
    {"fa_IR", "tile.furnace.name", "کوره"},
    {"fa_IR", "tile.sign.name", "تابلو"},
    {"fa_IR", "tile.doorWood.name", "در چوبی"},
    {"fa_IR", "tile.ladder.name", "نردبان"},
    {"fa_IR", "tile.rail.name", "ریل"},
    {"fa_IR", "tile.stairsStone.name", "پله سنگی"},
    {"fa_IR", "tile.lever.name", "اهرم"},
    {"fa_IR", "tile.pressurePlate.name", "صفحه فشاری"},
    {"fa_IR", "tile.doorIron.name", "در آهنی"},
    {"fa_IR", "tile.oreRedstone.name", "سنگ معدن ردستون"},
    {"fa_IR", "tile.notGate.name", "مشعل ردستون"},
    {"fa_IR", "tile.button.name", "دکمه"},
    {"fa_IR", "tile.snow.name", "برف"},
    {"fa_IR", "tile.ice.name", "یخ"},
    {"fa_IR", "tile.cactus.name", "کاکتوس"},
    {"fa_IR", "tile.clay.name", "رس"},
    {"fa_IR", "tile.reeds.name", "نیشکر"},
    {"fa_IR", "tile.jukebox.name", "جعبه موسیقی"},
    {"fa_IR", "tile.fence.name", "حصار"},
    {"fa_IR", "tile.pumpkin.name", "کدو"},
    {"fa_IR", "tile.hellrock.name", "ندراک"},
    {"fa_IR", "tile.hellsand.name", "شن روح"},
    {"fa_IR", "tile.lightgem.name", "گلوستون"},
    {"fa_IR", "tile.portal.name", "پرتال"},
    {"fa_IR", "tile.litpumpkin.name", "جک اُ لنترن"},
    {"fa_IR", "tile.oreLapis.name", "سنگ معدن لاجورد"},
    {"fa_IR", "tile.blockLapis.name", "بلوک لاجورد"},
    {"fa_IR", "tile.dispenser.name", "پخش‌کننده"},
    {"fa_IR", "tile.sandStone.name", "ماسه‌سنگ"},
    {"fa_IR", "tile.musicBlock.name", "بلوک نت"},
    {"fa_IR", "tile.bed.name", "تخت خواب"},
    {"fa_IR", "tile.goldenRail.name", "ریل قدرتی"},
    {"fa_IR", "tile.detectorRail.name", "ریل آشکارساز"},
    {"fa_IR", "tile.pistonStickyBase.name", "پیستون چسبنده"},
    {"fa_IR", "tile.web.name", "تار عنکبوت"},
    {"fa_IR", "tile.tallgrass.name", "چمن بلند"},
    {"fa_IR", "tile.deadbush.name", "بوته خشک"},
    {"fa_IR", "tile.pistonBase.name", "پیستون"},
    {"fa_IR", "tile.cake.name", "کیک"},
    {"fa_IR", "tile.diode.name", "تکرارکننده ردستون"},
    {"fa_IR", "tile.trapdoor.name", "دریچه"},
    {"fa_IR", "tile.stonebricksmooth.name", "آجر سنگی"},
    {"fa_IR", "tile.fenceIron.name", "میله آهنی"},
    {"fa_IR", "tile.thinGlass.name", "شیشه باریک"},
    {"fa_IR", "tile.melon.name", "هندوانه"},
    {"fa_IR", "tile.pumpkinStem.name", "ساقه کدو"},
    {"fa_IR", "tile.melonStem.name", "ساقه هندوانه"},
    {"fa_IR", "tile.vine.name", "پیچک"},
    {"fa_IR", "tile.fenceGate.name", "دروازه حصار"},
    {"fa_IR", "tile.stairsBrick.name", "پله آجری"},
    {"fa_IR", "tile.stairsStoneBrickSmooth.name", "پله آجر سنگی"},
    {"fa_IR", "tile.mycel.name", "مایسلیوم"},
    {"fa_IR", "tile.waterlily.name", "نیلوفر آبی"},
    {"fa_IR", "tile.netherBrick.name", "آجر ندر"},
    {"fa_IR", "tile.netherFence.name", "حصار آجر ندر"},
    {"fa_IR", "tile.stairsNetherBrick.name", "پله آجر ندر"},
    {"fa_IR", "tile.netherStalk.name", "زگیل ندر"},
    {"fa_IR", "tile.enchantmentTable.name", "میز افسون"},
    {"fa_IR", "tile.brewingStand.name", "پایه معجون‌سازی"},
    {"fa_IR", "tile.cauldron.name", "دیگ"},
    {"fa_IR", "tile.endPortalFrame.name", "قاب پرتال پایان"},
    {"fa_IR", "tile.whiteStone.name", "سنگ پایان"},
    {"fa_IR", "tile.dragonEgg.name", "تخم اژدها"},
    {"fa_IR", "tile.redstoneLight.name", "چراغ ردستون"},

    {"fa_IR", "item.apple.name", "سیب"},
    {"fa_IR", "item.appleGold.name", "سیب طلایی"},
    {"fa_IR", "item.arrow.name", "تیر"},
    {"fa_IR", "item.beefCooked.name", "استیک"},
    {"fa_IR", "item.beefRaw.name", "گوشت گاو خام"},
    {"fa_IR", "item.blazePowder.name", "پودر بلیز"},
    {"fa_IR", "item.blazeRod.name", "میله بلیز"},
    {"fa_IR", "item.boat.name", "قایق"},
    {"fa_IR", "item.bone.name", "استخوان"},
    {"fa_IR", "item.book.name", "کتاب"},
    {"fa_IR", "item.bootsChain.name", "چکمه زنجیری"},
    {"fa_IR", "item.bootsCloth.name", "چکمه چرمی"},
    {"fa_IR", "item.bootsDiamond.name", "چکمه الماسی"},
    {"fa_IR", "item.bootsGold.name", "چکمه طلایی"},
    {"fa_IR", "item.bootsIron.name", "چکمه آهنی"},
    {"fa_IR", "item.bow.name", "کمان"},
    {"fa_IR", "item.bowl.name", "کاسه"},
    {"fa_IR", "item.bread.name", "نان"},
    {"fa_IR", "item.brick.name", "آجر"},
    {"fa_IR", "item.bucket.name", "سطل"},
    {"fa_IR", "item.bucketLava.name", "سطل لاوا"},
    {"fa_IR", "item.bucketWater.name", "سطل آب"},
    {"fa_IR", "item.chestplateChain.name", "زره سینه زنجیری"},
    {"fa_IR", "item.chestplateCloth.name", "تُنیک چرمی"},
    {"fa_IR", "item.chestplateDiamond.name", "زره سینه الماسی"},
    {"fa_IR", "item.chestplateGold.name", "زره سینه طلایی"},
    {"fa_IR", "item.chestplateIron.name", "زره سینه آهنی"},
    {"fa_IR", "item.chickenCooked.name", "مرغ پخته"},
    {"fa_IR", "item.chickenRaw.name", "مرغ خام"},
    {"fa_IR", "item.clay.name", "رس"},
    {"fa_IR", "item.clock.name", "ساعت"},
    {"fa_IR", "item.coal.name", "زغال"},
    {"fa_IR", "item.compass.name", "قطب‌نما"},
    {"fa_IR", "item.cookie.name", "کلوچه"},
    {"fa_IR", "item.diamond.name", "الماس"},
    {"fa_IR", "item.emerald.name", "الماس"},
    {"fa_IR", "item.diode.name", "تکرارکننده ردستون"},
    {"fa_IR", "item.doorIron.name", "در آهنی"},
    {"fa_IR", "item.doorWood.name", "در چوبی"},
    {"fa_IR", "item.egg.name", "تخم‌مرغ"},
    {"fa_IR", "item.enderPearl.name", "مروارید اندر"},
    {"fa_IR", "item.expBottle.name", "بطری افسون"},
    {"fa_IR", "item.eyeOfEnder.name", "چشم اندر"},
    {"fa_IR", "item.feather.name", "پر"},
    {"fa_IR", "item.fermentedSpiderEye.name", "چشم عنکبوت تخمیرشده"},
    {"fa_IR", "item.fireball.name", "گوی آتش"},
    {"fa_IR", "item.fishingRod.name", "چوب ماهیگیری"},
    {"fa_IR", "item.flint.name", "سنگ چخماق"},
    {"fa_IR", "item.flintAndSteel.name", "فندک"},
    {"fa_IR", "item.ghastTear.name", "اشک گَست"},
    {"fa_IR", "item.glassBottle.name", "بطری شیشه‌ای"},
    {"fa_IR", "item.goldNugget.name", "تکه طلا"},
    {"fa_IR", "item.hatchetDiamond.name", "تبر الماسی"},
    {"fa_IR", "item.hatchetGold.name", "تبر طلایی"},
    {"fa_IR", "item.hatchetIron.name", "تبر آهنی"},
    {"fa_IR", "item.hatchetStone.name", "تبر سنگی"},
    {"fa_IR", "item.hatchetWood.name", "تبر چوبی"},
    {"fa_IR", "item.helmetChain.name", "کلاهخود زنجیری"},
    {"fa_IR", "item.helmetCloth.name", "کلاه چرمی"},
    {"fa_IR", "item.helmetDiamond.name", "کلاهخود الماسی"},
    {"fa_IR", "item.helmetGold.name", "کلاهخود طلایی"},
    {"fa_IR", "item.helmetIron.name", "کلاهخود آهنی"},
    {"fa_IR", "item.hoeDiamond.name", "بیلچه الماسی"},
    {"fa_IR", "item.hoeGold.name", "بیلچه طلایی"},
    {"fa_IR", "item.hoeIron.name", "بیلچه آهنی"},
    {"fa_IR", "item.hoeStone.name", "بیلچه سنگی"},
    {"fa_IR", "item.hoeWood.name", "بیلچه چوبی"},
    {"fa_IR", "item.ingotGold.name", "شمش طلا"},
    {"fa_IR", "item.ingotIron.name", "شمش آهن"},
    {"fa_IR", "item.leather.name", "چرم"},
    {"fa_IR", "item.leggingsChain.name", "شلوار زنجیری"},
    {"fa_IR", "item.leggingsCloth.name", "شلوار چرمی"},
    {"fa_IR", "item.leggingsDiamond.name", "شلوار الماسی"},
    {"fa_IR", "item.leggingsGold.name", "شلوار طلایی"},
    {"fa_IR", "item.leggingsIron.name", "شلوار آهنی"},
    {"fa_IR", "item.magmaCream.name", "کرم ماگما"},
    {"fa_IR", "item.melon.name", "هندوانه"},
    {"fa_IR", "item.milk.name", "شیر"},
    {"fa_IR", "item.minecart.name", "واگن معدن"},
    {"fa_IR", "item.minecartChest.name", "واگن معدن با صندوقچه"},
    {"fa_IR", "item.minecartFurnace.name", "واگن معدن با کوره"},
    {"fa_IR", "item.mushroomStew.name", "سوپ قارچ"},
    {"fa_IR", "item.netherStalkSeeds.name", "زگیل ندر"},
    {"fa_IR", "item.painting.name", "نقاشی"},
    {"fa_IR", "item.paper.name", "کاغذ"},
    {"fa_IR", "item.pickaxeDiamond.name", "کلنگ الماسی"},
    {"fa_IR", "item.pickaxeGold.name", "کلنگ طلایی"},
    {"fa_IR", "item.pickaxeIron.name", "کلنگ آهنی"},
    {"fa_IR", "item.pickaxeStone.name", "کلنگ سنگی"},
    {"fa_IR", "item.pickaxeWood.name", "کلنگ چوبی"},
    {"fa_IR", "item.porkchopCooked.name", "گوشت خوک پخته"},
    {"fa_IR", "item.porkchopRaw.name", "گوشت خوک خام"},
    {"fa_IR", "item.redstone.name", "ردستون"},
    {"fa_IR", "item.reeds.name", "نیشکر"},
    {"fa_IR", "item.rottenFlesh.name", "گوشت فاسد"},
    {"fa_IR", "item.saddle.name", "زین"},
    {"fa_IR", "item.seeds.name", "دانه"},
    {"fa_IR", "item.seeds_melon.name", "دانه هندوانه"},
    {"fa_IR", "item.seeds_pumpkin.name", "دانه کدو"},
    {"fa_IR", "item.shovelDiamond.name", "بیل الماسی"},
    {"fa_IR", "item.shovelGold.name", "بیل طلایی"},
    {"fa_IR", "item.shovelIron.name", "بیل آهنی"},
    {"fa_IR", "item.shovelStone.name", "بیل سنگی"},
    {"fa_IR", "item.shovelWood.name", "بیل چوبی"},
    {"fa_IR", "item.sign.name", "تابلو"},
    {"fa_IR", "item.slimeball.name", "گوی اسلایم"},
    {"fa_IR", "item.snowball.name", "گلوله برفی"},
    {"fa_IR", "item.speckledMelon.name", "هندوانه درخشان"},
    {"fa_IR", "item.spiderEye.name", "چشم عنکبوت"},
    {"fa_IR", "item.stick.name", "چوب‌دستی"},
    {"fa_IR", "item.string.name", "نخ"},
    {"fa_IR", "item.sulphur.name", "باروت"},
    {"fa_IR", "item.swordDiamond.name", "شمشیر الماسی"},
    {"fa_IR", "item.swordGold.name", "شمشیر طلایی"},
    {"fa_IR", "item.swordIron.name", "شمشیر آهنی"},
    {"fa_IR", "item.swordStone.name", "شمشیر سنگی"},
    {"fa_IR", "item.swordWood.name", "شمشیر چوبی"},
    {"fa_IR", "item.wheat.name", "گندم"},
    {"fa_IR", "item.yellowDust.name", "غبار گلوستون"}
};


#include "hptibine_lang.inc"

static int pex_lang_current_is(const char *code) {
    const char *cur = NULL;
    if (g_lang_list_loaded && g_lang_index >= 0 && g_lang_index < g_pex_lang_count) cur = g_pex_langs[g_lang_index].code;
    if (!cur || !*cur) cur = g_opts.language;
    return cur && pex_lang_str_eq(cur, code);
}

static int pex_lang_value_ascii_only(const char *value) {
    const unsigned char *p = (const unsigned char *)value;
    if (!value || !*value) return 1;
    while (*p) {
        if (*p >= 0x80) return 0;
        ++p;
    }
    return 1;
}

static const char *pex_lang_lookup_supplemental_key(const char *key, const char *exact_value) {
    if (!key || !*key) return NULL;
    for (int i = 0; i < (int)ARRAY_COUNT(hptibine_lang_supplements); ++i) {
        if (!pex_lang_current_is(hptibine_lang_supplements[i].lang)) continue;
        if (!pex_lang_str_eq(hptibine_lang_supplements[i].key, key)) continue;
        if (!exact_value || !*exact_value || pex_lang_value_ascii_only(exact_value)) return hptibine_lang_supplements[i].value;
        return NULL;
    }
    for (int i = 0; i < (int)ARRAY_COUNT(pex_lang_supplements); ++i) {
        if (!pex_lang_current_is(pex_lang_supplements[i].lang)) continue;
        if (!pex_lang_str_eq(pex_lang_supplements[i].key, key)) continue;
        if (!exact_value || !*exact_value || pex_lang_value_ascii_only(exact_value)) return pex_lang_supplements[i].value;
        return NULL;
    }
    return NULL;
}

static const char *tr_key_default(const char *key, const char *fallback) {
    const char *exact;
    if (!key) return fallback ? fallback : "";
    if (!g_lang_list_loaded || g_lang_table_count <= 0) pex_set_language_code(g_opts.language[0] ? g_opts.language : "en_US");
    exact = pex_lang_lookup_loaded_key(key);
    if (exact && !pex_lang_value_is_bridge_english(key, exact, fallback)) {
        const char *supp = pex_lang_lookup_supplemental_key(key, exact);
        if (supp) return supp;
        return exact;
    }

    {
        const char *supp = pex_lang_lookup_supplemental_key(key, exact);
        if (supp) return supp;
    }

    /* Bridge old 1.2.5 UI/item keys to newer legacy-index language files,
       but do not let en_US/newer English mask a selected language. */
    for (int i = 0; i < (int)ARRAY_COUNT(pex_lang_key_aliases); ++i) {
        if (pex_lang_str_eq(pex_lang_key_aliases[i].primary, key)) {
            const char *v = pex_lang_lookup_loaded_key(pex_lang_key_aliases[i].secondary);
            const char *supp = pex_lang_lookup_supplemental_key(pex_lang_key_aliases[i].secondary, v);
            if (supp) return supp;
            if (v && !pex_lang_value_is_bridge_english(key, v, fallback)) return v;
        }
    }
    if (exact) return exact;
    return fallback ? fallback : key;
}

static const char *tr_key2_default(const char *primary, const char *secondary, const char *fallback) {
    const char *v;
    if (!primary) return tr_key_default(secondary, fallback);
    v = tr_key_default(primary, NULL);
    if (v && !pex_lang_str_eq(v, primary)) return v;
    return tr_key_default(secondary, fallback);
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
    {"Play Selected World", "selectWorld.select"},
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
    {"Unlimited", "options.framerateLimit.max"},
    {"Minimum", "options.fov.min"},
    {"Quake Pro", "options.fov.max"},
    {"Skins...", "options.skinCustomisation"},
    {"Skins", "options.skinCustomisation"},
    {"Use Default", "selectWorld.reset"},
    {"Import Skin...", "texturePack.openFolder"},
    {"Select World", "selectWorld.title"},
    {"Delete World", "selectWorld.deleteQuestion"},
    {"No worlds found", "selectWorld.empty"},
    {"Create New World", "selectWorld.create"},
    {"Play Selected World", "selectWorld.select"},
    {"Back to title screen", "menu.returnToMenu"},
    {"Game over!", "deathScreen.title"},
    {"Score: &e0", "deathScreen.score"},
    {"Music", "options.music"},
    {"Sound", "options.sound"},
    {"Invert Mouse", "options.invertMouse"},
    {"Sensitivity", "options.sensitivity"},
    {"Render Distance", "options.renderDistance"},
    {"View Bobbing", "options.viewBobbing"},
    {"V-Sync", "options.vsync"},
    {"Max FPS", "options.framerateLimit"},
    {"FOV", "options.fov"},
    {"Difficulty", "options.difficulty"},
    {"Graphics", "options.graphics"},
    {"Fullscreen", "options.fullscreen"},
    {"Controls", "controls.title"},
    {"Reset", "controls.reset"},
    {"Key", "controls.key"},
    {"Game menu", "menu.game"},
    {"Quit Game", "menu.quit"},
    {"Rename World", "selectWorld.renameTitle"},
    {"Stone", "tile.stone.name"},
    {"Grass", "tile.grass.name"},
    {"Dirt", "tile.dirt.name"},
    {"Cobblestone", "tile.stonebrick.name"},
    {"Wooden Planks", "tile.wood.name"},
    {"Sapling", "tile.sapling.name"},
    {"Bedrock", "tile.bedrock.name"},
    {"Water", "tile.water.name"},
    {"Lava", "tile.lava.name"},
    {"Sand", "tile.sand.name"},
    {"Gravel", "tile.gravel.name"},
    {"Gold Ore", "tile.oreGold.name"},
    {"Iron Ore", "tile.oreIron.name"},
    {"Coal Ore", "tile.oreCoal.name"},
    {"Wood", "tile.log.name"},
    {"Leaves", "tile.leaves.name"},
    {"Sponge", "tile.sponge.name"},
    {"Glass", "tile.glass.name"},
    {"Flower", "tile.flower.name"},
    {"Rose", "tile.rose.name"},
    {"Mushroom", "tile.mushroom.name"},
    {"Block of Gold", "tile.blockGold.name"},
    {"Block of Iron", "tile.blockIron.name"},
    {"Stone Slab", "tile.stoneSlab.name"},
    {"Brick", "tile.brick.name"},
    {"TNT", "tile.tnt.name"},
    {"Bookshelf", "tile.bookshelf.name"},
    {"Moss Stone", "tile.stoneMoss.name"},
    {"Obsidian", "tile.obsidian.name"},
    {"Torch", "tile.torch.name"},
    {"Fire", "tile.fire.name"},
    {"Monster Spawner", "tile.mobSpawner.name"},
    {"Wooden Stairs", "tile.stairsWood.name"},
    {"Chest", "tile.chest.name"},
    {"Redstone Dust", "tile.redstoneDust.name"},
    {"Diamond Ore", "tile.oreDiamond.name"},
    {"Block of Diamond", "tile.blockDiamond.name"},
    {"Workbench", "tile.workbench.name"},
    {"Crops", "tile.crops.name"},
    {"Farmland", "tile.farmland.name"},
    {"Furnace", "tile.furnace.name"},
    {"Sign", "tile.sign.name"},
    {"Wooden Door", "tile.doorWood.name"},
    {"Ladder", "tile.ladder.name"},
    {"Rail", "tile.rail.name"},
    {"Stone Stairs", "tile.stairsStone.name"},
    {"Lever", "tile.lever.name"},
    {"Pressure Plate", "tile.pressurePlate.name"},
    {"Iron Door", "tile.doorIron.name"},
    {"Redstone Ore", "tile.oreRedstone.name"},
    {"Redstone Torch", "tile.notGate.name"},
    {"Button", "tile.button.name"},
    {"Snow", "tile.snow.name"},
    {"Ice", "tile.ice.name"},
    {"Cactus", "tile.cactus.name"},
    {"Clay", "tile.clay.name"},
    {"Reeds", "tile.reeds.name"},
    {"Jukebox", "tile.jukebox.name"},
    {"Fence", "tile.fence.name"},
    {"Pumpkin", "tile.pumpkin.name"},
    {"Netherrack", "tile.hellrock.name"},
    {"Soul Sand", "tile.hellsand.name"},
    {"Glowstone", "tile.lightgem.name"},
    {"Portal", "tile.portal.name"},
    {"Jack o'Lantern", "tile.litpumpkin.name"},
    {"Lapis Lazuli Ore", "tile.oreLapis.name"},
    {"Lapis Lazuli Block", "tile.blockLapis.name"},
    {"Dispenser", "tile.dispenser.name"},
    {"Sandstone", "tile.sandStone.name"},
    {"Note Block", "tile.musicBlock.name"},
    {"Bed", "tile.bed.name"},
    {"Powered Rail", "tile.goldenRail.name"},
    {"Detector Rail", "tile.detectorRail.name"},
    {"Sticky Piston", "tile.pistonStickyBase.name"},
    {"Web", "tile.web.name"},
    {"Tall Grass", "tile.tallgrass.name"},
    {"Dead Bush", "tile.deadbush.name"},
    {"Piston", "tile.pistonBase.name"},
    {"Cake", "tile.cake.name"},
    {"Redstone Repeater", "tile.diode.name"},
    {"Locked Chest", "tile.lockedchest.name"},
    {"Trapdoor", "tile.trapdoor.name"},
    {"Stone Bricks", "tile.stonebricksmooth.name"},
    {"Mushroom Cap", "tile.mushroom.name"},
    {"Iron Bars", "tile.fenceIron.name"},
    {"Glass Pane", "tile.thinGlass.name"},
    {"Melon", "tile.melon.name"},
    {"Pumpkin Stem", "tile.pumpkinStem.name"},
    {"Melon Stem", "tile.melonStem.name"},
    {"Vines", "tile.vine.name"},
    {"Fence Gate", "tile.fenceGate.name"},
    {"Brick Stairs", "tile.stairsBrick.name"},
    {"Stone Brick Stairs", "tile.stairsStoneBrickSmooth.name"},
    {"Mycelium", "tile.mycel.name"},
    {"Lily Pad", "tile.waterlily.name"},
    {"Nether Brick", "tile.netherBrick.name"},
    {"Nether Brick Fence", "tile.netherFence.name"},
    {"Nether Brick Stairs", "tile.stairsNetherBrick.name"},
    {"Nether Wart", "tile.netherStalk.name"},
    {"Enchantment Table", "tile.enchantmentTable.name"},
    {"Brewing Stand", "tile.brewingStand.name"},
    {"Cauldron", "tile.cauldron.name"},
    {"End Portal Frame", "tile.endPortalFrame.name"},
    {"End Stone", "tile.whiteStone.name"},
    {"Dragon Egg", "tile.dragonEgg.name"},
    {"Redstone Lamp", "tile.redstoneLight.name"},
    {"Iron Shovel", "item.shovelIron.name"},
    {"Iron Pickaxe", "item.pickaxeIron.name"},
    {"Iron Axe", "item.hatchetIron.name"},
    {"Flint and Steel", "item.flintAndSteel.name"},
    {"Apple", "item.apple.name"},
    {"Bow", "item.bow.name"},
    {"Arrow", "item.arrow.name"},
    {"Coal", "item.coal.name"},
    {"Diamond", "item.emerald.name"},
    {"Iron Ingot", "item.ingotIron.name"},
    {"Gold Ingot", "item.ingotGold.name"},
    {"Iron Sword", "item.swordIron.name"},
    {"Wooden Sword", "item.swordWood.name"},
    {"Wooden Shovel", "item.shovelWood.name"},
    {"Wooden Pickaxe", "item.pickaxeWood.name"},
    {"Wooden Axe", "item.hatchetWood.name"},
    {"Stone Sword", "item.swordStone.name"},
    {"Stone Shovel", "item.shovelStone.name"},
    {"Stone Pickaxe", "item.pickaxeStone.name"},
    {"Stone Axe", "item.hatchetStone.name"},
    {"Diamond Sword", "item.swordDiamond.name"},
    {"Diamond Shovel", "item.shovelDiamond.name"},
    {"Diamond Pickaxe", "item.pickaxeDiamond.name"},
    {"Diamond Axe", "item.hatchetDiamond.name"},
    {"Stick", "item.stick.name"},
    {"Bowl", "item.bowl.name"},
    {"Mushroom Stew", "item.mushroomStew.name"},
    {"Golden Sword", "item.swordGold.name"},
    {"Golden Shovel", "item.shovelGold.name"},
    {"Golden Pickaxe", "item.pickaxeGold.name"},
    {"Golden Axe", "item.hatchetGold.name"},
    {"String", "item.string.name"},
    {"Feather", "item.feather.name"},
    {"Sulphur", "item.sulphur.name"},
    {"Wooden Hoe", "item.hoeWood.name"},
    {"Stone Hoe", "item.hoeStone.name"},
    {"Iron Hoe", "item.hoeIron.name"},
    {"Diamond Hoe", "item.hoeDiamond.name"},
    {"Golden Hoe", "item.hoeGold.name"},
    {"Seeds", "item.seeds.name"},
    {"Wheat", "item.wheat.name"},
    {"Bread", "item.bread.name"},
    {"Leather Cap", "item.helmetCloth.name"},
    {"Leather Tunic", "item.chestplateCloth.name"},
    {"Leather Pants", "item.leggingsCloth.name"},
    {"Leather Boots", "item.bootsCloth.name"},
    {"Chain Helmet", "item.helmetChain.name"},
    {"Chain Chestplate", "item.chestplateChain.name"},
    {"Chain Leggings", "item.leggingsChain.name"},
    {"Chain Boots", "item.bootsChain.name"},
    {"Iron Helmet", "item.helmetIron.name"},
    {"Iron Chesplate", "item.chestplateIron.name"},
    {"Iron Leggings", "item.leggingsIron.name"},
    {"Iron Boots", "item.bootsIron.name"},
    {"Diamond Helmet", "item.helmetDiamond.name"},
    {"Diamond Chestplate", "item.chestplateDiamond.name"},
    {"Diamond Leggings", "item.leggingsDiamond.name"},
    {"Diamond Boots", "item.bootsDiamond.name"},
    {"Golden Helmet", "item.helmetGold.name"},
    {"Golden Chestplate", "item.chestplateGold.name"},
    {"Golden Leggings", "item.leggingsGold.name"},
    {"Golden boots", "item.bootsGold.name"},
    {"Flint", "item.flint.name"},
    {"Raw Porkchop", "item.porkchopRaw.name"},
    {"Cooked Porkchop", "item.porkchopCooked.name"},
    {"Painting", "item.painting.name"},
    {"Golden Apple", "item.appleGold.name"},
    {"Bucket", "item.bucket.name"},
    {"Water Bucket", "item.bucketWater.name"},
    {"Lava bucket", "item.bucketLava.name"},
    {"Minecart", "item.minecart.name"},
    {"Saddle", "item.saddle.name"},
    {"Redstone", "item.redstone.name"},
    {"Snowball", "item.snowball.name"},
    {"Boat", "item.boat.name"},
    {"Leather", "item.leather.name"},
    {"Milk", "item.milk.name"},
    {"Paper", "item.paper.name"},
    {"Book", "item.book.name"},
    {"Slimeball", "item.slimeball.name"},
    {"Minecart with Chest", "item.minecartChest.name"},
    {"Minecart with Furnace", "item.minecartFurnace.name"},
    {"Egg", "item.egg.name"},
    {"Compass", "item.compass.name"},
    {"Fishing Rod", "item.fishingRod.name"},
    {"Clock", "item.clock.name"},
    {"Glowstone Dust", "item.yellowDust.name"},
    {"Raw Fish", "item.fishRaw.name"},
    {"Cooked Fish", "item.fishCooked.name"},
    {"Dye", "item.dyePowder.name"},
    {"Bone", "item.bone.name"},
    {"Sugar", "item.sugar.name"},
    {"Cookie", "item.cookie.name"},
    {"Map", "item.map.name"},
    {"Shears", "item.shears.name"},
    {"Pumpkin Seeds", "item.seeds_pumpkin.name"},
    {"Melon Seeds", "item.seeds_melon.name"},
    {"Raw Beef", "item.beefRaw.name"},
    {"Steak", "item.beefCooked.name"},
    {"Raw Chicken", "item.chickenRaw.name"},
    {"Cooked Chicken", "item.chickenCooked.name"},
    {"Rotten Flesh", "item.rottenFlesh.name"},
    {"Ender Pearl", "item.enderPearl.name"},
    {"Blaze Rod", "item.blazeRod.name"},
    {"Ghast Tear", "item.ghastTear.name"},
    {"Gold Nugget", "item.goldNugget.name"},
    {"Potion", "item.potion.name"},
    {"Glass Bottle", "item.glassBottle.name"},
    {"Spider Eye", "item.spiderEye.name"},
    {"Fermented Spider Eye", "item.fermentedSpiderEye.name"},
    {"Blaze Powder", "item.blazePowder.name"},
    {"Magma Cream", "item.magmaCream.name"},
    {"Eye of Ender", "item.eyeOfEnder.name"},
    {"Glistering Melon", "item.speckledMelon.name"},
    {"Spawn Egg", "item.monsterPlacer.name"},
    {"Bottle o' Enchanting", "item.expBottle.name"},
    {"Fire Charge", "item.fireball.name"},
    {"Music Disc", "item.record.name"},
    {"World Name", "selectWorld.enterName"},
    {"Generating level", "menu.generatingLevel"},
    {"Loading level", "menu.loadingLevel"},
    {"Building terrain", "menu.generatingTerrain"},
    {"Reading level", "menu.loadingLevel"},
    {"Loading chunks", "menu.loadingLevel"},
    {"Lighting terrain", "menu.generatingTerrain"},
    {"Preparing chunks", "menu.preparingSpawn"},
    {"Entering world", "menu.loadingLevel"},
    {"Locating server", "connect.connecting"},
    {"Downloading terrain", "downloadTerrain"},
    {"Done!", "gui.done"}
};

static const char *tr(const char *en) {
    if (!en) return "";
    for (int i = 0; i < ARRAY_COUNT(pex_lang_aliases); ++i) {
        if (pex_lang_str_eq(en, pex_lang_aliases[i].english)) return tr_key_default(pex_lang_aliases[i].key, en);
    }
    return en;
}

static int pex_language_top(void) { return 32; }
static int pex_language_bottom(void) { return g_gui_h - 65 + 4; }
static int pex_language_slot_height(void) { return 18; }
static int pex_language_content_height(void) { return pex_language_count() * pex_language_slot_height(); }
static int pex_language_view_height(void) {
    int h = pex_language_bottom() - pex_language_top();
    return h > 1 ? h : 1;
}
static int pex_language_max_scroll_pixels(void) {
    /* GuiSlot.bindAmountScrolled(): contentHeight - (bottom - top - 4). */
    int max_scroll = pex_language_content_height() - (pex_language_view_height() - 4);
    if (max_scroll < 0) max_scroll = 0;
    return max_scroll;
}
static int pex_language_visible_rows(void) {
    int rows = (pex_language_view_height() + pex_language_slot_height() - 1) / pex_language_slot_height() + 1;
    return rows > 1 ? rows : 1;
}
static int pex_language_first_visible_row(void) {
    int row = g_language_scroll / pex_language_slot_height();
    if (row < 0) row = 0;
    if (row >= pex_language_count()) row = pex_language_count() - 1;
    if (row < 0) row = 0;
    return row;
}
static int pex_language_row_y(int idx) {
    /* GuiSlot var10 = top + 4 - amountScrolled; slotY = var10 + idx * 18. */
    return pex_language_top() + 4 + idx * pex_language_slot_height() - g_language_scroll;
}

static void language_clamp_scroll(void) {
    int max_scroll = pex_language_max_scroll_pixels();
    if (g_language_scroll < 0) g_language_scroll = 0;
    if (g_language_scroll > max_scroll) g_language_scroll = max_scroll;
}

static void language_scroll_by(int rows) {
    /* Java mouse wheel moves slotHeight / 2 per notch; PageUp/PageDown callers pass larger row counts. */
    g_language_scroll += rows * (pex_language_slot_height() / 2);
    language_clamp_scroll();
    rebuild_screen();
}

static void language_ensure_selected_visible(void) {
    int cur = pex_current_language_index();
    int view = pex_language_view_height() - 4;
    int y0 = cur * pex_language_slot_height();
    int y1 = y0 + pex_language_slot_height();
    if (y0 < g_language_scroll) g_language_scroll = y0;
    if (y1 > g_language_scroll + view) g_language_scroll = y1 - view;
    language_clamp_scroll();
    g_language_drag_scroll_pixels = 0;
}

static int g_language_drag_mode = 0; /* 0=list swipe, 1=scrollbar thumb */
static int g_language_drag_anchor_y = 0;
static int g_language_drag_start_scroll = 0;

static void language_mouse_down(int mx, int my) {
    int top = pex_language_top();
    int bottom = pex_language_bottom();
    int track_x = g_gui_w / 2 + 124;
    int max_scroll = pex_language_max_scroll_pixels();
    g_language_drag_mode = 0;
    g_language_drag_anchor_y = my;
    g_language_drag_start_scroll = g_language_scroll;
    if (max_scroll > 0 && my >= top && my <= bottom && mx >= track_x - 3 && mx <= track_x + 9) {
        g_language_drag_mode = 1;
    }
}

static void language_mouse_up(void) {
    g_language_drag_mode = 0;
}

static void language_drag_scroll(int delta_y) {
    if (delta_y == 0) return;
    if (g_language_drag_mode) {
        int top = pex_language_top();
        int bottom = pex_language_bottom();
        int view_h = pex_language_view_height();
        int content_h = pex_language_content_height();
        int max_scroll = pex_language_max_scroll_pixels();
        int thumb_h = content_h > 0 ? (view_h * view_h / content_h) : view_h;
        int track = bottom - top;
        if (thumb_h < 32) thumb_h = 32;
        if (thumb_h > view_h - 8) thumb_h = view_h - 8;
        if (track - thumb_h > 0) g_language_scroll = g_language_drag_start_scroll + (g_mouse_y - g_language_drag_anchor_y) * max_scroll / (track - thumb_h);
    } else {
        /* Swiping the list itself keeps the old/natural behavior: the content
           follows the drag, so dragging downward moves toward earlier rows. */
        g_language_scroll -= delta_y;
    }
    language_clamp_scroll();
    rebuild_screen();
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
    pack_install_set_state(CLASSIC_INSTALL_EXTRACTING, 90, "Extracting languages and font glyphs...");
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
