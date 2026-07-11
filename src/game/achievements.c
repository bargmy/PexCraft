/* Minecraft 1.2.5-style profile statistics and achievements.
   Included after language.c and before gameplay modules in the unity build. */

#define PEX_STAT_OBJECT_MAX 512
#define PEX_ACHIEVEMENT_COUNT 27
#define PEX_ACHIEVEMENT_QUEUE_MAX 32

typedef enum PexGeneralStat {
    PEX_STAT_START_GAME = 0,
    PEX_STAT_CREATE_WORLD,
    PEX_STAT_LOAD_WORLD,
    PEX_STAT_JOIN_MULTIPLAYER,
    PEX_STAT_LEAVE_GAME,
    PEX_STAT_PLAY_TICKS,
    PEX_STAT_WALK_CM,
    PEX_STAT_SWIM_CM,
    PEX_STAT_FALL_CM,
    PEX_STAT_CLIMB_CM,
    PEX_STAT_FLY_CM,
    PEX_STAT_DIVE_CM,
    PEX_STAT_MINECART_CM,
    PEX_STAT_BOAT_CM,
    PEX_STAT_PIG_CM,
    PEX_STAT_JUMPS,
    PEX_STAT_DROPS,
    PEX_STAT_DAMAGE_DEALT,
    PEX_STAT_DAMAGE_TAKEN,
    PEX_STAT_DEATHS,
    PEX_STAT_MOB_KILLS,
    PEX_STAT_PLAYER_KILLS,
    PEX_STAT_FISH_CAUGHT,
    PEX_GENERAL_STAT_COUNT
} PexGeneralStat;

typedef enum PexAchievementId {
    PEX_ACH_OPEN_INVENTORY = 0,
    PEX_ACH_MINE_WOOD,
    PEX_ACH_BUILD_WORKBENCH,
    PEX_ACH_BUILD_PICKAXE,
    PEX_ACH_BUILD_FURNACE,
    PEX_ACH_ACQUIRE_IRON,
    PEX_ACH_BUILD_HOE,
    PEX_ACH_MAKE_BREAD,
    PEX_ACH_BAKE_CAKE,
    PEX_ACH_BUILD_BETTER_PICKAXE,
    PEX_ACH_COOK_FISH,
    PEX_ACH_ON_A_RAIL,
    PEX_ACH_BUILD_SWORD,
    PEX_ACH_KILL_ENEMY,
    PEX_ACH_KILL_COW,
    PEX_ACH_FLY_PIG,
    PEX_ACH_SNIPE_SKELETON,
    PEX_ACH_DIAMONDS,
    PEX_ACH_PORTAL,
    PEX_ACH_GHAST,
    PEX_ACH_BLAZE_ROD,
    PEX_ACH_POTION,
    PEX_ACH_THE_END,
    PEX_ACH_THE_END_2,
    PEX_ACH_ENCHANTMENTS,
    PEX_ACH_OVERKILL,
    PEX_ACH_BOOKCASE
} PexAchievementId;

typedef struct PexGeneralStatDef {
    int legacy_id;
    const char *key;
    const char *fallback;
    int format; /* 0 integer, 1 time, 2 distance */
} PexGeneralStatDef;

typedef struct PexAchievementDef {
    const char *key;
    const char *title_fallback;
    const char *desc_fallback;
    int parent;
    int display_col;
    int display_row;
    int icon_id;
    int special;
} PexAchievementDef;

typedef struct PexProfileStats {
    long long general[PEX_GENERAL_STAT_COUNT];
    long long mined[PEX_STAT_OBJECT_MAX];
    long long crafted[PEX_STAT_OBJECT_MAX];
    long long used[PEX_STAT_OBJECT_MAX];
    long long broken[PEX_STAT_OBJECT_MAX];
    long long picked[PEX_STAT_OBJECT_MAX];
    unsigned char achievements[PEX_ACHIEVEMENT_COUNT];
} PexProfileStats;

static const PexGeneralStatDef g_pex_general_stat_defs[PEX_GENERAL_STAT_COUNT] = {
    {1000, "stat.startGame", "Games played", 0},
    {1001, "stat.createWorld", "Worlds created", 0},
    {1002, "stat.loadWorld", "Worlds loaded", 0},
    {1003, "stat.joinMultiplayer", "Multiplayer joins", 0},
    {1004, "stat.leaveGame", "Games quit", 0},
    {1100, "stat.playOneMinute", "Minutes played", 1},
    {2000, "stat.walkOneCm", "Distance walked", 2},
    {2001, "stat.swimOneCm", "Distance swum", 2},
    {2002, "stat.fallOneCm", "Distance fallen", 2},
    {2003, "stat.climbOneCm", "Distance climbed", 2},
    {2004, "stat.flyOneCm", "Distance flown", 2},
    {2005, "stat.diveOneCm", "Distance dove", 2},
    {2006, "stat.minecartOneCm", "Distance by minecart", 2},
    {2007, "stat.boatOneCm", "Distance by boat", 2},
    {2008, "stat.pigOneCm", "Distance by pig", 2},
    {2010, "stat.jump", "Jumps", 0},
    {2011, "stat.drop", "Items dropped", 0},
    {2020, "stat.damageDealt", "Damage dealt", 0},
    {2021, "stat.damageTaken", "Damage taken", 0},
    {2022, "stat.deaths", "Deaths", 0},
    {2023, "stat.mobKills", "Mob kills", 0},
    {2024, "stat.playerKills", "Player kills", 0},
    {2025, "stat.fishCaught", "Fish caught", 0}
};

static const PexAchievementDef g_pex_achievement_defs[PEX_ACHIEVEMENT_COUNT] = {
    {"openInventory", "Taking Inventory", "Press '%1$s' to open your inventory.", -1, 0, 0, ITEM_BOOK, 0},
    {"mineWood", "Getting Wood", "Attack a tree until a block of wood pops out.", 0, 2, 1, BLOCK_LOG, 0},
    {"buildWorkBench", "Benchmarking", "Craft a workbench with four blocks of planks.", 1, 4, -1, BLOCK_CRAFTING_TABLE, 0},
    {"buildPickaxe", "Time to Mine!", "Use planks and sticks to make a pickaxe.", 2, 4, 2, ITEM_WOODEN_PICKAXE, 0},
    {"buildFurnace", "Hot Topic", "Construct a furnace out of eight cobblestone blocks.", 3, 3, 4, BLOCK_FURNACE_LIT, 0},
    {"acquireIron", "Acquire Hardware", "Smelt an iron ingot.", 4, 1, 4, ITEM_INGOT_IRON, 0},
    {"buildHoe", "Time to Farm!", "Use planks and sticks to make a hoe.", 2, 2, -3, ITEM_WOODEN_HOE, 0},
    {"makeBread", "Bake Bread", "Turn wheat into bread.", 6, -1, -3, ITEM_BREAD, 0},
    {"bakeCake", "The Lie", "Wheat, sugar, milk and eggs!", 6, 0, -5, ITEM_CAKE, 0},
    {"buildBetterPickaxe", "Getting an Upgrade", "Construct a better pickaxe.", 3, 6, 2, ITEM_STONE_PICKAXE, 0},
    {"cookFish", "Delicious Fish", "Catch and cook a fish!", 4, 2, 6, ITEM_FISH_COOKED, 0},
    {"onARail", "On A Rail", "Travel by minecart at least 1 km from where you started.", 5, 2, 3, BLOCK_RAILS, 1},
    {"buildSword", "Time to Strike!", "Use planks and sticks to make a sword.", 2, 6, -1, ITEM_WOODEN_SWORD, 0},
    {"killEnemy", "Monster Hunter", "Attack and destroy a monster.", 12, 8, -1, ITEM_BONE, 0},
    {"killCow", "Cow Tipper", "Harvest some leather.", 12, 7, -3, ITEM_LEATHER, 0},
    {"flyPig", "When Pigs Fly", "Fly a pig off a cliff.", 14, 8, -4, ITEM_SADDLE, 1},
    {"snipeSkeleton", "Sniper Duel", "Kill a skeleton with an arrow from more than 50 meters.", 13, 7, 0, ITEM_BOW, 1},
    {"diamonds", "DIAMONDS!", "Acquire diamonds with your iron tools.", 5, -1, 5, ITEM_DIAMOND, 0},
    {"portal", "We Need to Go Deeper", "Build a portal to the Nether.", 17, -1, 7, BLOCK_OBSIDIAN, 0},
    {"ghast", "Return to Sender", "Destroy a Ghast with a fireball.", 18, -4, 8, ITEM_GHAST_TEAR, 1},
    {"blazeRod", "Into Fire", "Relieve a Blaze of its rod.", 18, 0, 9, ITEM_BLAZE_ROD, 0},
    {"potion", "Local Brewery", "Brew a potion.", 20, 2, 8, ITEM_POTION, 0},
    {"theEnd", "The End?", "Locate the End.", 20, 3, 10, ITEM_EYE_OF_ENDER, 1},
    {"theEnd2", "The End.", "Defeat the Ender Dragon.", 22, 4, 13, BLOCK_DRAGON_EGG, 1},
    {"enchantments", "Enchanter", "Use a book, obsidian and diamonds to construct an enchantment table.", 17, -4, 4, BLOCK_ENCHANTMENT_TABLE, 0},
    {"overkill", "Overkill", "Deal nine hearts of damage in a single hit.", 24, -4, 1, ITEM_SWORD_DIAMOND, 1},
    {"bookcase", "Librarian", "Build some bookshelves to improve your enchantment table.", 24, -3, 6, BLOCK_BOOKSHELF, 0}
};

static PexProfileStats g_pex_profile_stats;
static int g_pex_stats_loaded = 0;
static int g_pex_stats_dirty = 0;
static char g_pex_stats_loaded_user[32] = "";
static int g_pex_stats_save_cooldown = 0;
static int g_pex_achievement_queue[PEX_ACHIEVEMENT_QUEUE_MAX];
static int g_pex_achievement_queue_head = 0;
static int g_pex_achievement_queue_count = 0;
static int g_pex_achievement_toast_id = -1;
static double g_pex_achievement_toast_started = 0.0;
static int g_pex_stats_prev_position_valid = 0;
static float g_pex_stats_prev_x = 0.0f, g_pex_stats_prev_y = 0.0f, g_pex_stats_prev_z = 0.0f;

static int pex_stats_valid_object_id(int id) { return id >= 0 && id < PEX_STAT_OBJECT_MAX; }

static void pex_stats_username(char *out, size_t cap, const char *src) {
    size_t n = 0;
    if (!src || !*src) src = "player";
    while (*src && n + 1 < cap) {
        unsigned char c = (unsigned char)*src++;
        if (c >= 'A' && c <= 'Z') c = (unsigned char)(c - 'A' + 'a');
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-') out[n++] = (char)c;
        else out[n++] = '_';
    }
    if (n == 0 && cap > 1) { out[0] = 'p'; out[1] = 0; }
    else out[n] = 0;
}

static void pex_stats_path(char *out, size_t cap, const char *suffix) {
    char dir[MAX_PATHBUF], user[48], file[96];
    path_join(dir, sizeof(dir), g_mc_dir, "stats");
    ensure_dir(dir);
    pex_stats_username(user, sizeof(user), g_opts.username);
    snprintf(file, sizeof(file), "stats_%s.dat%s", user, suffix ? suffix : "");
    path_join(out, cap, dir, file);
}

static void pex_stats_reset_memory(void) {
    memset(&g_pex_profile_stats, 0, sizeof(g_pex_profile_stats));
    g_pex_achievement_queue_head = 0;
    g_pex_achievement_queue_count = 0;
    g_pex_achievement_toast_id = -1;
    g_pex_achievement_toast_started = 0.0;
    g_pex_stats_prev_position_valid = 0;
    g_pex_stats_dirty = 0;
    g_pex_stats_save_cooldown = 0;
}

static void pex_stats_load_file(const char *path) {
    FILE *f = fopen(path, "rb");
    char line[256];
    if (!f) return;
    while (fgets(line, sizeof(line), f)) {
        char kind = 0;
        int id = 0;
        long long value = 0;
        if (sscanf(line, " %c %d %lld", &kind, &id, &value) != 3) continue;
        switch (kind) {
            case 'G': if (id >= 0 && id < PEX_GENERAL_STAT_COUNT) g_pex_profile_stats.general[id] = value; break;
            case 'M': if (pex_stats_valid_object_id(id)) g_pex_profile_stats.mined[id] = value; break;
            case 'C': if (pex_stats_valid_object_id(id)) g_pex_profile_stats.crafted[id] = value; break;
            case 'U': if (pex_stats_valid_object_id(id)) g_pex_profile_stats.used[id] = value; break;
            case 'B': if (pex_stats_valid_object_id(id)) g_pex_profile_stats.broken[id] = value; break;
            case 'P': if (pex_stats_valid_object_id(id)) g_pex_profile_stats.picked[id] = value; break;
            case 'A': if (id >= 0 && id < PEX_ACHIEVEMENT_COUNT) g_pex_profile_stats.achievements[id] = value > 0 ? 1 : 0; break;
            default: break;
        }
    }
    fclose(f);
}

static void pex_stats_flush(void);

static void pex_stats_ensure_loaded(void) {
    char user[32], path[MAX_PATHBUF], old[MAX_PATHBUF];
    pex_stats_username(user, sizeof(user), g_opts.username);
    if (g_pex_stats_loaded && !strcmp(user, g_pex_stats_loaded_user)) return;
    if (g_pex_stats_loaded && g_pex_stats_dirty) pex_stats_flush();
    pex_stats_reset_memory();
    snprintf(g_pex_stats_loaded_user, sizeof(g_pex_stats_loaded_user), "%s", user);
    pex_stats_path(path, sizeof(path), "");
    pex_stats_load_file(path);
    if (!file_exists(path)) {
        pex_stats_path(old, sizeof(old), ".old");
        pex_stats_load_file(old);
    }
    g_pex_stats_loaded = 1;
}

static void pex_stats_write_nonzero(FILE *f, char kind, const long long *values, int count) {
    for (int i = 0; i < count; ++i) if (values[i] != 0) fprintf(f, "%c %d %lld\n", kind, i, values[i]);
}

static void pex_stats_flush(void) {
    char path[MAX_PATHBUF], tmp[MAX_PATHBUF], old[MAX_PATHBUF];
    FILE *f;
    if (!g_pex_stats_loaded || !g_pex_stats_dirty) return;
    pex_stats_path(path, sizeof(path), "");
    pex_stats_path(tmp, sizeof(tmp), ".tmp");
    pex_stats_path(old, sizeof(old), ".old");
    f = fopen(tmp, "wb");
    if (!f) return;
    fprintf(f, "PXCSTATS 2\n");
    pex_stats_write_nonzero(f, 'G', g_pex_profile_stats.general, PEX_GENERAL_STAT_COUNT);
    pex_stats_write_nonzero(f, 'M', g_pex_profile_stats.mined, PEX_STAT_OBJECT_MAX);
    pex_stats_write_nonzero(f, 'C', g_pex_profile_stats.crafted, PEX_STAT_OBJECT_MAX);
    pex_stats_write_nonzero(f, 'U', g_pex_profile_stats.used, PEX_STAT_OBJECT_MAX);
    pex_stats_write_nonzero(f, 'B', g_pex_profile_stats.broken, PEX_STAT_OBJECT_MAX);
    pex_stats_write_nonzero(f, 'P', g_pex_profile_stats.picked, PEX_STAT_OBJECT_MAX);
    for (int i = 0; i < PEX_ACHIEVEMENT_COUNT; ++i)
        if (g_pex_profile_stats.achievements[i]) fprintf(f, "A %d 1\n", i);
    if (fclose(f) != 0) return;
    remove(old);
    rename(path, old);
    if (rename(tmp, path) == 0) g_pex_stats_dirty = 0;
}

static void pex_stats_mark_dirty(void) {
    g_pex_stats_dirty = 1;
    if (g_pex_stats_save_cooldown <= 0) g_pex_stats_save_cooldown = 600;
}

static void pex_stats_add_general(PexGeneralStat stat, long long amount) {
    if (stat < 0 || stat >= PEX_GENERAL_STAT_COUNT || amount == 0) return;
    pex_stats_ensure_loaded();
    g_pex_profile_stats.general[stat] += amount;
    if (g_pex_profile_stats.general[stat] < 0) g_pex_profile_stats.general[stat] = 0;
    pex_stats_mark_dirty();
}

static void pex_stats_add_object(long long *array, int id, long long amount) {
    if (!pex_stats_valid_object_id(id) || amount == 0) return;
    pex_stats_ensure_loaded();
    array[id] += amount;
    if (array[id] < 0) array[id] = 0;
    pex_stats_mark_dirty();
}

static void pex_stats_add_mined(int id, int amount) { pex_stats_add_object(g_pex_profile_stats.mined, id, amount); }
static void pex_stats_add_crafted(int id, int amount) { pex_stats_add_object(g_pex_profile_stats.crafted, id, amount); }
static void pex_stats_add_used(int id, int amount) { pex_stats_add_object(g_pex_profile_stats.used, id, amount); }
static void pex_stats_add_broken(int id, int amount) { pex_stats_add_object(g_pex_profile_stats.broken, id, amount); }
static void pex_stats_add_picked(int id, int amount) { pex_stats_add_object(g_pex_profile_stats.picked, id, amount); }

static int pex_achievement_unlocked(int id) {
    pex_stats_ensure_loaded();
    return id >= 0 && id < PEX_ACHIEVEMENT_COUNT && g_pex_profile_stats.achievements[id] != 0;
}

static int pex_achievement_can_unlock(int id) {
    if (id < 0 || id >= PEX_ACHIEVEMENT_COUNT) return 0;
    int parent = g_pex_achievement_defs[id].parent;
    return parent < 0 || pex_achievement_unlocked(parent);
}

static void pex_achievement_queue_toast(int id) {
    if (id < 0 || id >= PEX_ACHIEVEMENT_COUNT) return;
    if (g_pex_achievement_queue_count >= PEX_ACHIEVEMENT_QUEUE_MAX) return;
    int tail = (g_pex_achievement_queue_head + g_pex_achievement_queue_count) % PEX_ACHIEVEMENT_QUEUE_MAX;
    g_pex_achievement_queue[tail] = id;
    ++g_pex_achievement_queue_count;
}

static int pex_achievement_unlock(int id) {
    pex_stats_ensure_loaded();
    if (id < 0 || id >= PEX_ACHIEVEMENT_COUNT) return 0;
    if (g_pex_profile_stats.achievements[id] || !pex_achievement_can_unlock(id)) return 0;
    g_pex_profile_stats.achievements[id] = 1;
    pex_stats_mark_dirty();
    pex_achievement_queue_toast(id);
    return 1;
}

static void pex_achievement_toast_tick(void) {
    double now = now_seconds();
    if (g_pex_achievement_toast_id >= 0 && now - g_pex_achievement_toast_started >= 3.0) {
        g_pex_achievement_toast_id = -1;
        g_pex_achievement_toast_started = 0.0;
    }
    if (g_pex_achievement_toast_id < 0 && g_pex_achievement_queue_count > 0) {
        g_pex_achievement_toast_id = g_pex_achievement_queue[g_pex_achievement_queue_head];
        g_pex_achievement_queue_head = (g_pex_achievement_queue_head + 1) % PEX_ACHIEVEMENT_QUEUE_MAX;
        --g_pex_achievement_queue_count;
        g_pex_achievement_toast_started = now;
    }
}

static void pex_achievement_on_inventory_open(void) { (void)pex_achievement_unlock(PEX_ACH_OPEN_INVENTORY); }

static void pex_achievement_on_crafted(int id, int amount) {
    if (amount <= 0) return;
    pex_stats_add_crafted(id, amount);
    if (id == BLOCK_CRAFTING_TABLE) (void)pex_achievement_unlock(PEX_ACH_BUILD_WORKBENCH);
    else if (id == ITEM_WOODEN_PICKAXE) (void)pex_achievement_unlock(PEX_ACH_BUILD_PICKAXE);
    else if (id == BLOCK_FURNACE) (void)pex_achievement_unlock(PEX_ACH_BUILD_FURNACE);
    else if (id == ITEM_WOODEN_HOE) (void)pex_achievement_unlock(PEX_ACH_BUILD_HOE);
    else if (id == ITEM_BREAD) (void)pex_achievement_unlock(PEX_ACH_MAKE_BREAD);
    else if (id == ITEM_CAKE) (void)pex_achievement_unlock(PEX_ACH_BAKE_CAKE);
    else if (id == ITEM_STONE_PICKAXE) (void)pex_achievement_unlock(PEX_ACH_BUILD_BETTER_PICKAXE);
    else if (id == ITEM_WOODEN_SWORD) (void)pex_achievement_unlock(PEX_ACH_BUILD_SWORD);
    else if (id == BLOCK_ENCHANTMENT_TABLE) (void)pex_achievement_unlock(PEX_ACH_ENCHANTMENTS);
    else if (id == BLOCK_BOOKSHELF) (void)pex_achievement_unlock(PEX_ACH_BOOKCASE);
}

static void pex_achievement_on_furnace_taken(int id, int amount) {
    if (amount <= 0) return;
    pex_stats_add_crafted(id, amount);
    if (id == ITEM_INGOT_IRON) (void)pex_achievement_unlock(PEX_ACH_ACQUIRE_IRON);
    if (id == ITEM_FISH_COOKED) (void)pex_achievement_unlock(PEX_ACH_COOK_FISH);
}

static void pex_achievement_on_item_picked(int id, int amount) {
    if (amount <= 0) return;
    pex_stats_add_picked(id, amount);
    if (id == BLOCK_LOG) (void)pex_achievement_unlock(PEX_ACH_MINE_WOOD);
    else if (id == ITEM_LEATHER) (void)pex_achievement_unlock(PEX_ACH_KILL_COW);
    else if (id == ITEM_DIAMOND) (void)pex_achievement_unlock(PEX_ACH_DIAMONDS);
    else if (id == ITEM_BLAZE_ROD) (void)pex_achievement_unlock(PEX_ACH_BLAZE_ROD);
}

static void pex_achievement_on_potion_taken(void) { (void)pex_achievement_unlock(PEX_ACH_POTION); }
static void pex_achievement_on_portal(void) { (void)pex_achievement_unlock(PEX_ACH_PORTAL); }
static void pex_achievement_on_end_enter(void) { (void)pex_achievement_unlock(PEX_ACH_THE_END); }
static void pex_achievement_on_dragon_killed(void) { (void)pex_achievement_unlock(PEX_ACH_THE_END_2); }
static void pex_achievement_on_pig_fall(void) { (void)pex_achievement_unlock(PEX_ACH_FLY_PIG); }
static void pex_achievement_on_ghast_reflect(void) { (void)pex_achievement_unlock(PEX_ACH_GHAST); }
static void pex_achievement_on_skeleton_snipe(void) { (void)pex_achievement_unlock(PEX_ACH_SNIPE_SKELETON); }

static void pex_achievement_on_damage_dealt(int damage) {
    if (damage <= 0) return;
    pex_stats_add_general(PEX_STAT_DAMAGE_DEALT, damage);
    if (damage >= 18) (void)pex_achievement_unlock(PEX_ACH_OVERKILL);
}

static void pex_achievement_on_mob_killed(int mob_type, int hostile, const PexDamageSource *source) {
    (void)source;
    pex_stats_add_general(PEX_STAT_MOB_KILLS, 1);
    if (hostile) (void)pex_achievement_unlock(PEX_ACH_KILL_ENEMY);
    if (mob_type == PASSIVE_MOB_ENDER_DRAGON) pex_achievement_on_dragon_killed();
}

static void pex_stats_world_entered(int multiplayer, int created) {
    pex_stats_ensure_loaded();
    pex_stats_add_general(PEX_STAT_START_GAME, 1);
    if (multiplayer) pex_stats_add_general(PEX_STAT_JOIN_MULTIPLAYER, 1);
    else if (created) pex_stats_add_general(PEX_STAT_CREATE_WORLD, 1);
    else pex_stats_add_general(PEX_STAT_LOAD_WORLD, 1);
    g_pex_stats_prev_position_valid = 0;
}

static void pex_stats_world_left(void) {
    pex_stats_add_general(PEX_STAT_LEAVE_GAME, 1);
    pex_stats_flush();
    g_pex_stats_prev_position_valid = 0;
}

static void pex_stats_tick(void) {
    pex_stats_ensure_loaded();
    pex_achievement_toast_tick();
    if (g_screen == SCREEN_INGAME || g_screen == SCREEN_PAUSE || g_screen == SCREEN_INVENTORY ||
        g_screen == SCREEN_CREATIVE || g_screen == SCREEN_WORKBENCH || g_screen == SCREEN_FURNACE ||
        g_screen == SCREEN_CHEST || g_screen == SCREEN_CHAT || g_screen == SCREEN_DEATH) {
        pex_stats_add_general(PEX_STAT_PLAY_TICKS, 1);
    }
    if (g_pex_stats_save_cooldown > 0 && --g_pex_stats_save_cooldown == 0) pex_stats_flush();
}

static void pex_stats_track_player_position(int in_water, int on_ladder, int flying, int riding_pig, int riding_minecart, int riding_boat) {
    float dx, dy, dz, horizontal, distance;
    if (!g_pex_stats_prev_position_valid) {
        g_pex_stats_prev_x = g_player_x; g_pex_stats_prev_y = g_player_y; g_pex_stats_prev_z = g_player_z;
        g_pex_stats_prev_position_valid = 1;
        return;
    }
    dx = g_player_x - g_pex_stats_prev_x;
    dy = g_player_y - g_pex_stats_prev_y;
    dz = g_player_z - g_pex_stats_prev_z;
    g_pex_stats_prev_x = g_player_x; g_pex_stats_prev_y = g_player_y; g_pex_stats_prev_z = g_player_z;
    horizontal = sqrtf(dx * dx + dz * dz);
    distance = sqrtf(dx * dx + dy * dy + dz * dz);
    if (distance > 8.0f) return; /* teleport/world load */
    int cm = (int)floorf(distance * 100.0f + 0.5f);
    if (cm <= 0) return;
    if (riding_minecart) pex_stats_add_general(PEX_STAT_MINECART_CM, cm);
    else if (riding_boat) pex_stats_add_general(PEX_STAT_BOAT_CM, cm);
    else if (riding_pig) pex_stats_add_general(PEX_STAT_PIG_CM, cm);
    else if (in_water && dy < -0.01f) pex_stats_add_general(PEX_STAT_DIVE_CM, cm);
    else if (in_water) pex_stats_add_general(PEX_STAT_SWIM_CM, cm);
    else if (on_ladder && fabsf(dy) > 0.01f) pex_stats_add_general(PEX_STAT_CLIMB_CM, (long long)(fabsf(dy) * 100.0f));
    else if (flying) pex_stats_add_general(PEX_STAT_FLY_CM, cm);
    else if (horizontal > 0.0f) pex_stats_add_general(PEX_STAT_WALK_CM, (long long)(horizontal * 100.0f));
}

static const char *pex_achievement_title(int id) {
    static char key[96];
    if (id < 0 || id >= PEX_ACHIEVEMENT_COUNT) return "";
    snprintf(key, sizeof(key), "achievement.%s", g_pex_achievement_defs[id].key);
    return tr_key_default(key, g_pex_achievement_defs[id].title_fallback);
}

static const char *pex_achievement_description(int id, char *out, size_t cap) {
    char key[112], keybuf[64];
    const char *src;
    if (!out || cap == 0) return "";
    out[0] = 0;
    if (id < 0 || id >= PEX_ACHIEVEMENT_COUNT) return out;
    snprintf(key, sizeof(key), "achievement.%s.desc", g_pex_achievement_defs[id].key);
    src = tr_key_default(key, g_pex_achievement_defs[id].desc_fallback);
    key_name(g_opts.keys[7], keybuf, sizeof(keybuf));
    const char *needle = strstr(src, "%1$s");
    size_t token_len = 4;
    if (!needle) {
        /* Some third-party language/resource packs use C-style %s rather than
           Java's positional %1$s token.  Accept both without interpreting any
           other printf sequences from pack-controlled text. */
        needle = strstr(src, "%s");
        token_len = 2;
    }
    if (!needle) { snprintf(out, cap, "%s", src); return out; }
    size_t left = (size_t)(needle - src);
    if (left >= cap) left = cap - 1;
    memcpy(out, src, left); out[left] = 0;
    snprintf(out + left, cap - left, "%s%s", keybuf, needle + token_len);
    return out;
}

static void pex_stats_format_value(PexGeneralStat stat, long long value, char *out, size_t cap) {
    int format = (stat >= 0 && stat < PEX_GENERAL_STAT_COUNT) ? g_pex_general_stat_defs[stat].format : 0;
    if (format == 1) {
        double seconds = (double)value / 20.0;
        if (seconds < 60.0) snprintf(out, cap, "%.2f s", seconds);
        else if (seconds < 3600.0) snprintf(out, cap, "%.2f min", seconds / 60.0);
        else if (seconds < 86400.0) snprintf(out, cap, "%.2f h", seconds / 3600.0);
        else snprintf(out, cap, "%.2f d", seconds / 86400.0);
    } else if (format == 2) {
        if (value < 100) snprintf(out, cap, "%lld cm", value);
        else if (value < 100000) snprintf(out, cap, "%.2f m", (double)value / 100.0);
        else snprintf(out, cap, "%.2f km", (double)value / 100000.0);
    } else snprintf(out, cap, "%lld", value);
}
