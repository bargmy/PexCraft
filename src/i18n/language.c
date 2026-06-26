/* Minecraft 1.2.5-style language support.  Included by the unity builds before
   GUI/options code so every screen can call tr()/tr_key(). */

typedef struct PexLangEntry {
    const char *key;
    const char *value;
} PexLangEntry;

typedef struct PexBundledLanguage {
    const char *code;
    const char *name;
    const PexLangEntry *entries;
    int count;
} PexBundledLanguage;

#include "bundled_langs.inc"

#ifndef PEX_BUNDLED_LANG_COUNT
static const PexBundledLanguage pex_bundled_langs[] = { { "en_US", "English (US)", NULL, 0 } };
#define PEX_BUNDLED_LANG_COUNT 1
#endif

static int g_lang_index = 0;
static int g_lang_unicode = 0;

static int pex_lang_str_eq(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) { if (*a++ != *b++) return 0; }
    return *a == 0 && *b == 0;
}

static int pex_lang_value_has_unicode(const char *s) {
    if (!s) return 0;
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p) if (*p >= 0x80) return 1;
    return 0;
}

static int pex_lang_find_index(const char *code) {
    if (!code || !*code) return 0;
    for (int i = 0; i < PEX_BUNDLED_LANG_COUNT; ++i) {
        if (pex_lang_str_eq(pex_bundled_langs[i].code, code)) return i;
    }
    return 0;
}

static const char *pex_lang_lookup_in(int idx, const char *key) {
    if (!key || idx < 0 || idx >= PEX_BUNDLED_LANG_COUNT) return NULL;
    const PexBundledLanguage *lang = &pex_bundled_langs[idx];
    for (int i = 0; i < lang->count; ++i) {
        if (pex_lang_str_eq(lang->entries[i].key, key)) return lang->entries[i].value;
    }
    return NULL;
}

static void pex_lang_refresh_unicode_flag(void) {
    g_lang_unicode = 0;
    const PexBundledLanguage *lang = &pex_bundled_langs[g_lang_index];
    for (int i = 0; i < lang->count; ++i) {
        if (pex_lang_value_has_unicode(lang->entries[i].value)) { g_lang_unicode = 1; return; }
    }
}

static void pex_set_language_code(const char *code) {
    g_lang_index = pex_lang_find_index(code);
    pex_lang_refresh_unicode_flag();
}

static int pex_language_count(void) { return PEX_BUNDLED_LANG_COUNT; }
static const char *pex_language_code_at(int idx) {
    if (idx < 0 || idx >= PEX_BUNDLED_LANG_COUNT) idx = 0;
    return pex_bundled_langs[idx].code;
}
static const char *pex_language_name_at(int idx) {
    if (idx < 0 || idx >= PEX_BUNDLED_LANG_COUNT) idx = 0;
    return pex_bundled_langs[idx].name ? pex_bundled_langs[idx].name : pex_bundled_langs[idx].code;
}
static int pex_current_language_index(void) { return g_lang_index; }
static const char *pex_current_language_code(void) { return pex_language_code_at(g_lang_index); }
static int pex_current_language_is_unicode(void) { return g_lang_unicode; }

static const char *tr_key_default(const char *key, const char *fallback) {
    const char *v = pex_lang_lookup_in(g_lang_index, key);
    if (!v) v = pex_lang_lookup_in(0, key);
    return v ? v : (fallback ? fallback : (key ? key : ""));
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
    {"Stone", "tile.stone.name"},
    {"Grass", "tile.grass.name"},
    {"Dirt", "tile.dirt.name"},
    {"Cobblestone", "tile.stonebrick.name"},
    {"Wooden Planks", "tile.wood.name"},
    {"Lapis Lazuli Ore", "tile.oreLapis.name"},
    {"Lapis Lazuli Block", "tile.blockLapis.name"},
    {"Block of Gold", "tile.blockGold.name"},
    {"Block of Iron", "tile.blockIron.name"},
    {"Block of Diamond", "tile.blockDiamond.name"},
    {"Dispenser", "tile.dispenser.name"},
    {"Sandstone", "tile.sandStone.name"},
    {"Note Block", "tile.musicBlock.name"},
    {"Chest", "tile.chest.name"},
    {"Workbench", "tile.workbench.name"},
    {"Furnace", "tile.furnace.name"},
    {"Iron Shovel", "item.shovelIron.name"},
    {"Iron Pickaxe", "item.pickaxeIron.name"},
    {"Iron Axe", "item.hatchetIron.name"},
    {"Flint and Steel", "item.flintAndSteel.name"},
    {"Apple", "item.apple.name"},
    {"Bow", "item.bow.name"},
    {"Arrow", "item.arrow.name"},
    {"Coal", "item.coal.name"},
    {"Diamond", "item.diamond.name"},
    {"Iron Ingot", "item.ingotIron.name"},
    {"Gold Ingot", "item.ingotGold.name"},
    {"Iron Sword", "item.swordIron.name"},
};

static const char *tr(const char *en) {
    if (!en) return "";
    for (int i = 0; i < ARRAY_COUNT(pex_lang_aliases); ++i) {
        if (pex_lang_str_eq(en, pex_lang_aliases[i].english)) return tr_key_default(pex_lang_aliases[i].key, en);
    }
    return en;
}
