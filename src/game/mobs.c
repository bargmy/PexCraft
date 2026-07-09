/* Minecraft 1.2.5-style local mobs. Included in the unity build after
   inventory.c so it can reuse the existing world/item/collision helpers. */

#define PASSIVE_MOB_SAVE_VERSION 34

static PassiveMob *passive_mob_raycast(float max_dist, float *out_t);
static void passive_mob_assign_owner(PassiveMob *m);
static int passive_mob_is_owned_by_player(const PassiveMob *m);
static void passive_owned_wolves_target_mob_125(PassiveMob *target);
static void passive_owned_wolves_defend_player_125(PassiveMob *attacker);
static void passive_village_update_reputation_from_attack(PassiveMob *m);
static const char *passive_mob_name(int type);
static void passive_mob_set_path_goal(PassiveMob *m, float x, float y, float z);
static void passive_mob_apply_default_equipment_125(PassiveMob *m);
static int passive_player_looking_at_mob_125(const PassiveMob *m);
static int passive_mob_teleport_random_125(PassiveMob *m, float range);
static int passive_path_can_stand_at(int type, int x, int y, int z);
static void passive_mobs_tick_spawners_125(void);
static void passive_mob_spawner_remove_tile_125(int x, int y, int z);
static void passive_mobs_ensure_ender_dragon_125(void);
static void passive_dragon_tick_living_125(PassiveMob *m);
static int passive_dragon_tick_death_125(PassiveMob *m);
static void passive_dragon_crystals_tick_125(PassiveMob *dragon);
static void passive_dragon_crystals_draw_125(float partial);
static int passive_dragon_crystal_player_attack_125(float max_dist, float mob_t);
static int passive_dragon_player_part_damage_125(const PassiveMob *dragon, int base_damage);
static void passive_dragon_crystal_state_write_binary_125(FILE *f);
static void passive_dragon_crystal_state_read_binary_125(FILE *f, int save_version);
static void passive_path_clear(PassiveMob *m);
static int item_icon_tile(int id);
static int block_texture_resolve(int block_id, int meta, int face);
static void draw_dropped_item_sprite(int tile);
static void passive_draw_terrain_sprite_tile(int tile);
static void draw_item3d_from_texture(Texture *atlas, int tile);
static void draw_block_item_model(int id, float x, float y, float z);

/* Performance knobs: this port renders animal models with immediate-mode
   cube parts.  Keep active/rendered passive mobs close to b1.0 density instead
   of letting the spawn patch fill the entire active window. */
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
#define PEX_PASSIVE_TARGET_CAP 32
#define PEX_PASSIVE_INITIAL_TARGET 12
#define PEX_PASSIVE_RENDER_LIMIT 6
#define PEX_PASSIVE_RENDER_DIST 40.0f
#else
/* Match Beta b1.0 much closer: creature cap is 20 for the 17x17 eligible
   chunk area around one player.  Rendering is separately culled so this does
   not force every animal model to draw every frame. */
#define PEX_PASSIVE_TARGET_CAP 100
#define PEX_PASSIVE_INITIAL_TARGET 20
#define PEX_PASSIVE_RENDER_LIMIT 24
#define PEX_PASSIVE_RENDER_DIST 72.0f
#endif

#define PEX_PASSIVE_RENDER_WORKER 0

static float pex_rand_float01(void) { return (float)rand() / (float)RAND_MAX; }
static float pex_wrap_degrees(float a) {
    while (a < -180.0f) a += 360.0f;
    while (a >= 180.0f) a -= 360.0f;
    return a;
}
static float pex_clamp_float(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static int g_passive_dragon_killed_125 = 0;

#define PEX_DRAGON_CRYSTAL_MAX_125 16
typedef struct PexDragonCrystalRuntime125 {
    int active;
    int x, y, z;
    float px, py, pz;
    int health;
    int inner_rotation;
    int beam_dragon_index;
} PexDragonCrystalRuntime125;
static PexDragonCrystalRuntime125 g_dragon_crystals_125[PEX_DRAGON_CRYSTAL_MAX_125];
static int g_dragon_crystals_seeded_125 = 0;

typedef enum PexMobModelKind {
    PEX_MOB_MODEL_QUADRUPED = 0,
    PEX_MOB_MODEL_CHICKEN = 1,
    PEX_MOB_MODEL_BIPED = 2,
    PEX_MOB_MODEL_SPIDER = 3,
    PEX_MOB_MODEL_SLIME = 4,
    PEX_MOB_MODEL_GHAST = 5,
    PEX_MOB_MODEL_BLAZE = 6,
    PEX_MOB_MODEL_SQUID = 7,
    PEX_MOB_MODEL_IRON_GOLEM = 8,
    PEX_MOB_MODEL_DRAGON = 9,
    PEX_MOB_MODEL_SNOWMAN = 10,
    PEX_MOB_MODEL_VILLAGER = 11,
    PEX_MOB_MODEL_SILVERFISH = 12
} PexMobModelKind;

typedef struct PexMobInfo {
    int type;
    int entity_id;
    const char *name;
    float width;
    float height;
    int health;
    float move_speed;
    int attack_damage;
    int hostile;
    int water_mob;
    PexMobModelKind model;
    Texture *texture;
    const char *living_sound;
    const char *hurt_sound;
    const char *death_sound;
    float sound_volume;
    int primary_drop;
    int secondary_drop;
} PexMobInfo;

static const PexMobInfo g_pex_mob_info[] = {
    {PASSIVE_MOB_PIG,          90,  "Pig",           0.9f, 0.9f,  10, 0.25f, 0, 0, 0, PEX_MOB_MODEL_QUADRUPED,  &tex_mob_pig,              "mob.pig",          "mob.pig",             "mob.pigdeath",          1.0f, ITEM_PORK_RAW, 0},
    {PASSIVE_MOB_SHEEP,        91,  "Sheep",         0.9f, 1.3f,   8, 0.23f, 0, 0, 0, PEX_MOB_MODEL_QUADRUPED,  &tex_mob_sheep,            "mob.sheep",        "mob.sheep",           "mob.sheep",             1.0f, BLOCK_WOOL, 0},
    {PASSIVE_MOB_COW,          92,  "Cow",           0.9f, 1.3f,  10, 0.23f, 0, 0, 0, PEX_MOB_MODEL_QUADRUPED,  &tex_mob_cow,              "mob.cow",          "mob.cowhurt",         "mob.cowhurt",           0.4f, ITEM_LEATHER, ITEM_BEEF_RAW},
    {PASSIVE_MOB_CHICKEN,      93,  "Chicken",       0.3f, 0.7f,   4, 0.25f, 0, 0, 0, PEX_MOB_MODEL_CHICKEN,    &tex_mob_chicken,          "mob.chicken",      "mob.chickenhurt",     "mob.chickenhurt",       1.0f, ITEM_FEATHER, ITEM_CHICKEN_RAW},
    {PASSIVE_MOB_CREEPER,      50,  "Creeper",       0.6f, 1.8f,  20, 0.25f, 0, 1, 0, PEX_MOB_MODEL_BIPED,      &tex_mob_creeper,          NULL,               "mob.creeper",         "mob.creeperdeath",      1.0f, ITEM_GUNPOWDER, 0},
    {PASSIVE_MOB_SKELETON,     51,  "Skeleton",      0.6f, 1.8f,  20, 0.25f, 4, 1, 0, PEX_MOB_MODEL_BIPED,      &tex_mob_skeleton,         "mob.skeleton",     "mob.skeletonhurt",    "mob.skeletonhurt",      1.0f, ITEM_ARROW, ITEM_BONE},
    {PASSIVE_MOB_SPIDER,       52,  "Spider",        1.4f, 0.9f,  16, 0.80f, 2, 1, 0, PEX_MOB_MODEL_SPIDER,     &tex_mob_spider,           "mob.spider",       "mob.spider",          "mob.spiderdeath",       1.0f, ITEM_STRING, ITEM_SPIDER_EYE},
    {PASSIVE_MOB_GIANT,        53,  "Giant",         3.6f,10.8f, 100, 0.50f,50, 1, 0, PEX_MOB_MODEL_BIPED,      &tex_mob_zombie,           "mob.zombie",       "mob.zombiehurt",      "mob.zombiehurt",        1.0f, ITEM_ROTTEN_FLESH, 0},
    {PASSIVE_MOB_ZOMBIE,       54,  "Zombie",        0.6f, 1.8f,  20, 0.23f, 4, 1, 0, PEX_MOB_MODEL_BIPED,      &tex_mob_zombie,           "mob.zombie",       "mob.zombiehurt",      "mob.zombiedeath",       1.0f, ITEM_ROTTEN_FLESH, 0},
    {PASSIVE_MOB_SLIME,        55,  "Slime",         1.2f, 1.2f,  16, 0.30f, 4, 1, 0, PEX_MOB_MODEL_SLIME,      &tex_mob_slime,            NULL,               "mob.slime",           "mob.slime",             1.0f, ITEM_SLIME_BALL, 0},
    {PASSIVE_MOB_GHAST,        56,  "Ghast",         4.0f, 4.0f,  10, 0.18f, 6, 1, 0, PEX_MOB_MODEL_GHAST,      &tex_mob_ghast,            "mob.ghast.moan",   "mob.ghast.scream",    "mob.ghast.death",       10.0f, ITEM_GHAST_TEAR, ITEM_GUNPOWDER},
    {PASSIVE_MOB_PIG_ZOMBIE,   57,  "Zombie Pigman", 0.6f, 1.8f,  20, 0.50f, 5, 1, 0, PEX_MOB_MODEL_BIPED,      &tex_mob_pigzombie,        "mob.zombiepig.zpig","mob.zombiepig.zpighurt","mob.zombiepig.zpigdeath",1.0f, ITEM_ROTTEN_FLESH, ITEM_GOLD_NUGGET},
    {PASSIVE_MOB_ENDERMAN,     58,  "Enderman",      0.6f, 2.9f,  40, 0.30f, 7, 1, 0, PEX_MOB_MODEL_BIPED,      &tex_mob_enderman,         "mob.endermen.idle", "mob.endermen.hit",    "mob.endermen.death",    1.0f, ITEM_ENDER_PEARL, 0},
    {PASSIVE_MOB_CAVE_SPIDER,  59,  "Cave Spider",   0.7f, 0.5f,  12, 0.80f, 2, 1, 0, PEX_MOB_MODEL_SPIDER,     &tex_mob_cavespider,       "mob.spider",       "mob.spider",          "mob.spiderdeath",       1.0f, ITEM_STRING, ITEM_SPIDER_EYE},
    {PASSIVE_MOB_SILVERFISH,   60,  "Silverfish",    0.3f, 0.7f,   8, 0.60f, 1, 1, 0, PEX_MOB_MODEL_SILVERFISH,      &tex_mob_silverfish,       "mob.silverfish.say","mob.silverfish.hit",   "mob.silverfish.kill",   1.0f, 0, 0},
    {PASSIVE_MOB_BLAZE,        61,  "Blaze",         0.6f, 1.8f,  20, 0.23f, 6, 1, 0, PEX_MOB_MODEL_BLAZE,      &tex_mob_blaze,            "mob.blaze.breathe","mob.blaze.hit",        "mob.blaze.death",       1.0f, ITEM_BLAZE_ROD, 0},
    {PASSIVE_MOB_MAGMA_CUBE,   62,  "Magma Cube",    1.2f, 1.2f,  16, 0.30f, 4, 1, 0, PEX_MOB_MODEL_SLIME,      &tex_mob_lava,             NULL,               "mob.slime",           "mob.slime",             1.0f, ITEM_MAGMA_CREAM, 0},
    {PASSIVE_MOB_ENDER_DRAGON, 63,  "Ender Dragon", 16.0f, 8.0f, 200, 0.60f,10, 1, 0, PEX_MOB_MODEL_DRAGON,     &tex_mob_enderdragon,      "mob.enderdragon.growl","mob.enderdragon.hit", "mob.enderdragon.end",   5.0f, 0, 0},
    {PASSIVE_MOB_SQUID,        94,  "Squid",         0.95f,0.95f, 10, 0.18f, 0, 0, 1, PEX_MOB_MODEL_SQUID,      &tex_mob_squid,            NULL,               NULL,                 NULL,                  0.4f, ITEM_DYE_POWDER, 0},
    {PASSIVE_MOB_WOLF,         95,  "Wolf",          0.6f, 0.8f,   8, 0.30f, 4, 0, 0, PEX_MOB_MODEL_QUADRUPED,  &tex_mob_wolf,             "mob.wolf.bark",    "mob.wolf.hurt",       "mob.wolf.death",        0.4f, 0, 0},
    {PASSIVE_MOB_MOOSHROOM,    96,  "Mooshroom",     0.9f, 1.3f,  10, 0.23f, 0, 0, 0, PEX_MOB_MODEL_QUADRUPED,  &tex_mob_mooshroom,        "mob.cow",          "mob.cowhurt",         "mob.cowhurt",           0.4f, ITEM_LEATHER, ITEM_BEEF_RAW},
    {PASSIVE_MOB_SNOWMAN,      97,  "Snow Golem",    0.4f, 1.8f,   4, 0.25f, 0, 0, 0, PEX_MOB_MODEL_SNOWMAN,    &tex_mob_snowman,          NULL,               "mob.snowman",         "mob.snowman",           1.0f, ITEM_SNOWBALL, 0},
    {PASSIVE_MOB_OCELOT,       98,  "Ocelot",        0.6f, 0.8f,  10, 0.30f, 3, 0, 0, PEX_MOB_MODEL_QUADRUPED,  &tex_mob_ocelot,           "mob.cat.meow",     "mob.cat.hitt",        "mob.cat.hitt",          0.4f, 0, 0},
    {PASSIVE_MOB_IRON_GOLEM,   99,  "Iron Golem",    1.4f, 2.9f, 100, 0.25f,15, 0, 0, PEX_MOB_MODEL_IRON_GOLEM, &tex_mob_villager_golem,   "mob.irongolem.walk","mob.irongolem.hit",    "mob.irongolem.death",   1.0f, ITEM_INGOT_IRON, BLOCK_RED_ROSE},
    {PASSIVE_MOB_VILLAGER,    120,  "Villager",      0.6f, 1.8f,  20, 0.50f, 0, 0, 0, PEX_MOB_MODEL_VILLAGER,   &tex_mob_villager,         "mob.villager.default","mob.villager.defaulthurt","mob.villager.death",  1.0f, 0, 0}
};

static const PexMobInfo *passive_mob_info(int type) {
    for (int i = 0; i < (int)(sizeof(g_pex_mob_info) / sizeof(g_pex_mob_info[0])); ++i) {
        if (g_pex_mob_info[i].type == type) return &g_pex_mob_info[i];
    }
    return NULL;
}

static int passive_mob_type_from_egg(int entity_id) {
    for (int i = 0; i < (int)(sizeof(g_pex_mob_info) / sizeof(g_pex_mob_info[0])); ++i) {
        if (g_pex_mob_info[i].entity_id == entity_id) return g_pex_mob_info[i].type;
    }
    return PASSIVE_MOB_NONE;
}

/* Java 1.2.5 EnumCreatureType clone for local flat-world spawning. */
typedef enum PexCreatureCategory {
    PEX_CAT_MONSTER = 0,
    PEX_CAT_CREATURE = 1,
    PEX_CAT_WATER = 2,
    PEX_CAT_UTILITY = 3
} PexCreatureCategory;

typedef struct PexSpawnEntry {
    int type;
    int weight;
    int min_group;
    int max_group;
} PexSpawnEntry;

static int passive_mob_category(int type) {
    switch (type) {
        case PASSIVE_MOB_SPIDER: case PASSIVE_MOB_ZOMBIE: case PASSIVE_MOB_SKELETON:
        case PASSIVE_MOB_CREEPER: case PASSIVE_MOB_SLIME: case PASSIVE_MOB_ENDERMAN:
        case PASSIVE_MOB_CAVE_SPIDER: case PASSIVE_MOB_SILVERFISH: case PASSIVE_MOB_BLAZE:
        case PASSIVE_MOB_GHAST: case PASSIVE_MOB_PIG_ZOMBIE: case PASSIVE_MOB_MAGMA_CUBE:
        case PASSIVE_MOB_ENDER_DRAGON: case PASSIVE_MOB_GIANT:
            return PEX_CAT_MONSTER;
        case PASSIVE_MOB_SQUID:
            return PEX_CAT_WATER;
        case PASSIVE_MOB_IRON_GOLEM: case PASSIVE_MOB_SNOWMAN: case PASSIVE_MOB_VILLAGER:
            return PEX_CAT_UTILITY;
        default:
            return PEX_CAT_CREATURE;
    }
}

static int passive_mob_count_category(int cat) {
    int n = 0;
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        PassiveMob *m = &g_passive_mobs[i];
        if (m->active && passive_mob_category(m->type) == cat) ++n;
    }
    return n;
}

static int passive_spawn_cap_for_category(int cat) {
    /* SpawnerAnimals: maxNumberOfCreature * eligibleChunks.size()/256.
       With one player the 17x17 set is 289 chunks. */
    int eligible = 17 * 17;
    switch (cat) {
        case PEX_CAT_MONSTER: return (70 * eligible) / 256;
        case PEX_CAT_CREATURE: return (15 * eligible) / 256;
        case PEX_CAT_WATER: return (5 * eligible) / 256;
        default: return 12;
    }
}

static int passive_current_biome_id_at(int x, int z) {
    static BiomeManager bm;
    static long long seed = 0;
    static int ready = 0;
    if (!ready || seed != g_world_seed) {
        memset(&bm, 0, sizeof(bm));
        biome_manager_init(&bm, (int64_t)g_world_seed);
        seed = g_world_seed;
        ready = 1;
    }
    WorldBiome *b = biome_manager_get(&bm, x, z, 1, 1);
    return b ? b[0].id : BIOME_PLAINS;
}

static int passive_biome_allows_creatures(int biome) {
    switch (biome) {
        case BIOME_DESERT: case BIOME_DESERT_HILLS:
        case BIOME_OCEAN: case BIOME_FROZEN_OCEAN:
        case BIOME_RIVER: case BIOME_FROZEN_RIVER:
        case BIOME_BEACH:
            return 0;
        default: return 1;
    }
}

static int passive_biome_allows_monsters(int biome) {
    return !(biome == BIOME_MUSHROOM_ISLAND || biome == BIOME_MUSHROOM_ISLAND_SHORE || biome == BIOME_HELL || biome == BIOME_SKY);
}

static int passive_biome_weighted_spawn_entry(int cat, int biome, PexSpawnEntry *out) {
    static const PexSpawnEntry base_creatures[] = {
        {PASSIVE_MOB_SHEEP,12,4,4}, {PASSIVE_MOB_PIG,10,4,4},
        {PASSIVE_MOB_CHICKEN,10,4,4}, {PASSIVE_MOB_COW,8,4,4}
    };
    static const PexSpawnEntry base_monsters[] = {
        {PASSIVE_MOB_SPIDER,10,4,4}, {PASSIVE_MOB_ZOMBIE,10,4,4},
        {PASSIVE_MOB_SKELETON,10,4,4}, {PASSIVE_MOB_CREEPER,10,4,4},
        {PASSIVE_MOB_SLIME,10,4,4}, {PASSIVE_MOB_ENDERMAN,1,1,4}
    };
    static const PexSpawnEntry water[] = {{PASSIVE_MOB_SQUID,10,4,4}};
    PexSpawnEntry tmp[12];
    int n = 0;
    if (cat == PEX_CAT_CREATURE) {
        if (biome == BIOME_MUSHROOM_ISLAND || biome == BIOME_MUSHROOM_ISLAND_SHORE) {
            tmp[n++] = (PexSpawnEntry){PASSIVE_MOB_MOOSHROOM,8,4,8};
        } else if (passive_biome_allows_creatures(biome)) {
            for (int i=0;i<(int)(sizeof(base_creatures)/sizeof(base_creatures[0]));++i) tmp[n++] = base_creatures[i];
            if (biome == BIOME_FOREST || biome == BIOME_FOREST_HILLS) tmp[n++] = (PexSpawnEntry){PASSIVE_MOB_WOLF,5,4,4};
            if (biome == BIOME_TAIGA || biome == BIOME_TAIGA_HILLS) tmp[n++] = (PexSpawnEntry){PASSIVE_MOB_WOLF,8,4,4};
            if (biome == BIOME_JUNGLE || biome == BIOME_JUNGLE_HILLS) tmp[n++] = (PexSpawnEntry){PASSIVE_MOB_CHICKEN,10,4,4};
        }
    } else if (cat == PEX_CAT_MONSTER) {
        if (biome == BIOME_JUNGLE || biome == BIOME_JUNGLE_HILLS) tmp[n++] = (PexSpawnEntry){PASSIVE_MOB_OCELOT,2,1,1};
        if (biome == BIOME_HELL) {
            /* BiomeGenHell 1.2.5: ghast 50, pig zombie 100, magma cube 1.
               Blazes are fortress-spawner mobs, not natural Nether biome spawns. */
            tmp[n++] = (PexSpawnEntry){PASSIVE_MOB_GHAST,50,4,4};
            tmp[n++] = (PexSpawnEntry){PASSIVE_MOB_PIG_ZOMBIE,100,4,4};
            tmp[n++] = (PexSpawnEntry){PASSIVE_MOB_MAGMA_CUBE,1,4,4};
        } else if (biome == BIOME_SKY) {
            tmp[n++] = (PexSpawnEntry){PASSIVE_MOB_ENDERMAN,10,4,4};
        } else if (passive_biome_allows_monsters(biome)) {
            for (int i=0;i<(int)(sizeof(base_monsters)/sizeof(base_monsters[0]));++i) tmp[n++] = base_monsters[i];
        }
    } else if (cat == PEX_CAT_WATER) {
        tmp[n++] = water[0];
    }
    if (n <= 0) return 0;
    int total = 0; for (int i=0;i<n;++i) total += tmp[i].weight;
    int r = rand() % (total > 0 ? total : 1);
    for (int i=0;i<n;++i) { if (r < tmp[i].weight) { *out = tmp[i]; return 1; } r -= tmp[i].weight; }
    *out = tmp[n-1]; return 1;
}

typedef struct PexSpawnYCacheEntry {
    int valid;
    int type, cat, x, z;
    int y;
    int tick;
    unsigned int light_version;
} PexSpawnYCacheEntry;

#define PEX_SPAWN_Y_CACHE_SIZE 4096
static PexSpawnYCacheEntry g_passive_spawn_y_cache[PEX_SPAWN_Y_CACHE_SIZE];

/* Budget for expensive spawn-height probes.  Negative means unlimited
   (used by initial population seeding before the game loop starts). */
static int g_passive_spawn_probe_budget = -1;

static unsigned int passive_spawn_column_light_version(int x, int z) {
    if (x < g_flat_world_origin_x || x >= g_flat_world_origin_x + FLAT_WORLD_SIZE ||
        z < g_flat_world_origin_z || z >= g_flat_world_origin_z + FLAT_WORLD_SIZE) return 0;
    int lcx = flat_local_chunk_x(x);
    int lcz = flat_local_chunk_z(z);
    return flat_local_chunk_valid(lcx, lcz) ? g_flat_chunk_light_version[lcz][lcx] : 0;
}

static unsigned int passive_spawn_y_cache_hash(int type, int cat, int x, int z) {
    unsigned int h = (unsigned int)(x * 73428767u) ^ (unsigned int)(z * 91278319u) ^ (unsigned int)(type * 19349663u) ^ (unsigned int)(cat * 83492791u);
    return h & (PEX_SPAWN_Y_CACHE_SIZE - 1);
}

static int passive_spawn_y_cache_get(int type, int cat, int x, int z, int *out_y) {
    unsigned int idx = passive_spawn_y_cache_hash(type, cat, x, z);
    for (int p = 0; p < 4; ++p) {
        PexSpawnYCacheEntry *e = &g_passive_spawn_y_cache[(idx + (unsigned)p) & (PEX_SPAWN_Y_CACHE_SIZE - 1)];
        if (!e->valid) continue;
        unsigned int light_version = passive_spawn_column_light_version(x, z);
        if (e->type == type && e->cat == cat && e->x == x && e->z == z &&
            e->light_version == light_version && (g_ingame_ticks - e->tick) < 80) {
            if (out_y) *out_y = e->y;
            ++g_prof_spawn_y_cache_hits;
            return 1;
        }
    }
    ++g_prof_spawn_y_cache_misses;
    return 0;
}

static void passive_spawn_y_cache_put(int type, int cat, int x, int z, int y) {
    unsigned int idx = passive_spawn_y_cache_hash(type, cat, x, z);
    int slot = (int)idx;
    int oldest = 0x7fffffff;
    for (int p = 0; p < 4; ++p) {
        int si = (int)((idx + (unsigned)p) & (PEX_SPAWN_Y_CACHE_SIZE - 1));
        PexSpawnYCacheEntry *e = &g_passive_spawn_y_cache[si];
        if (!e->valid || (e->type == type && e->cat == cat && e->x == x && e->z == z)) { slot = si; break; }
        if (e->tick < oldest) { oldest = e->tick; slot = si; }
    }
    g_passive_spawn_y_cache[slot].valid = 1;
    g_passive_spawn_y_cache[slot].type = type;
    g_passive_spawn_y_cache[slot].cat = cat;
    g_passive_spawn_y_cache[slot].x = x;
    g_passive_spawn_y_cache[slot].z = z;
    g_passive_spawn_y_cache[slot].y = y;
    g_passive_spawn_y_cache[slot].tick = g_ingame_ticks;
    g_passive_spawn_y_cache[slot].light_version = passive_spawn_column_light_version(x, z);
}

static int passive_world_is_daytime(void) {
    int t = (int)(g_world_time % 24000LL);
    if (t < 0) t += 24000;
    return t < 12300 || t > 23850;
}

static int passive_can_see_sky(int x, int y, int z) {
    if (!flat_in_bounds(x, y, z)) return 1;
    /* Use the maintained skylight buffer for the common case.  The old loop
       scanned from the mob to world top for every burning/door/spawn test. */
    if (flat_saved_sky_light(x, y + 1, z) >= 15) return 1;
    if (flat_saved_sky_light(x, y + 1, z) <= 0) return 0;
    return flat_can_block_see_sky_slow(x, y, z);
}

static int passive_is_undead_burner(int type) {
    return type == PASSIVE_MOB_ZOMBIE || type == PASSIVE_MOB_SKELETON;
}

static int passive_mob_is_hostile_target_type(int type) {
    switch (type) {
        case PASSIVE_MOB_CREEPER: case PASSIVE_MOB_SKELETON: case PASSIVE_MOB_SPIDER:
        case PASSIVE_MOB_ZOMBIE: case PASSIVE_MOB_SLIME: case PASSIVE_MOB_ENDERMAN:
        case PASSIVE_MOB_CAVE_SPIDER: case PASSIVE_MOB_SILVERFISH: case PASSIVE_MOB_BLAZE:
        case PASSIVE_MOB_MAGMA_CUBE: case PASSIVE_MOB_GHAST: case PASSIVE_MOB_PIG_ZOMBIE:
            return 1;
        default: return 0;
    }
}

static int passive_mob_hostile(const PassiveMob *m) {
    const PexMobInfo *info = m ? passive_mob_info(m->type) : NULL;
    if (!info) return 0;
    if (m->type == PASSIVE_MOB_PIG_ZOMBIE && m->rideable <= 0) return 0; /* neutral until hit */
    if (m->type == PASSIVE_MOB_ENDERMAN && m->rideable <= 0) return 0;   /* neutral until stared at / hit */
    if (m->type == PASSIVE_MOB_WOLF && m->rideable <= 0) return 0;       /* neutral/tame flag */
    if (m->type == PASSIVE_MOB_IRON_GOLEM) return 0;
    return info->hostile;
}

static int passive_mob_is_slime_family(int type) {
    return type == PASSIVE_MOB_SLIME || type == PASSIVE_MOB_MAGMA_CUBE;
}

static int passive_mob_can_despawn_125(const PassiveMob *m) {
    if (!m) return 0;
    if (m->type == PASSIVE_MOB_VILLAGER || m->type == PASSIVE_MOB_IRON_GOLEM || m->type == PASSIVE_MOB_SNOWMAN) return 0;
    if (m->type == PASSIVE_MOB_WOLF) return m->rideable > 0 && !passive_mob_is_owned_by_player(m);
    if (m->type == PASSIVE_MOB_OCELOT) return !passive_mob_is_owned_by_player(m);
    if (passive_mob_category(m->type) == PEX_CAT_CREATURE) return 0;
    return 1;
}

static float passive_mob_model_scale_for_type(int type) {
    switch (type) {
        case PASSIVE_MOB_GIANT: return 6.0f;                 /* RenderGiantZombie scale */
        case PASSIVE_MOB_GHAST: return 4.5f;                 /* RenderGhast idle pre-scale */
        case PASSIVE_MOB_CAVE_SPIDER: return 0.7f;           /* EntityCaveSpider.spiderScaleAmount */
        default: return 1.0f;                                /* 1.2.5 models render at 1/16 scale */
    }
}

static float passive_mob_shadow_size_125(int type) {
    /* Exact RenderManager.java registrations from Minecraft 1.2.5. */
    switch (type) {
        case PASSIVE_MOB_CHICKEN: return 0.3f;       /* RenderChicken */
        case PASSIVE_MOB_OCELOT: return 0.4f;        /* RenderOcelot */
        case PASSIVE_MOB_SILVERFISH: return 0.3f;    /* RenderSilverfish */
        case PASSIVE_MOB_SPIDER:
        case PASSIVE_MOB_CAVE_SPIDER: return 1.0f;   /* RenderSpider */
        case PASSIVE_MOB_GIANT: return 3.0f;         /* RenderGiantZombie: 0.5 * 6.0 */
        case PASSIVE_MOB_SLIME:
        case PASSIVE_MOB_MAGMA_CUBE: return 0.25f;   /* RenderSlime/RenderMagmaCube */
        case PASSIVE_MOB_PIG:
        case PASSIVE_MOB_SHEEP:
        case PASSIVE_MOB_COW:
        case PASSIVE_MOB_MOOSHROOM:
        case PASSIVE_MOB_SQUID: return 0.7f;
        default: return 0.5f;                        /* RenderBiped/most RenderLiving mobs */
    }
}

static float passive_lerp_angle(float a, float b, float partial) {
    return a + pex_wrap_degrees(b - a) * partial;
}

static float passive_sheep_eat_amount_125(float sheep_timer, float partial) {
    /* EntitySheep.func_44003_c: drives ModelSheep head rotationPointY while grazing. */
    if (sheep_timer <= 0.0f) return 0.0f;
    if (sheep_timer >= 4.0f && sheep_timer <= 36.0f) return 1.0f;
    if (sheep_timer < 4.0f) return (sheep_timer - partial) / 4.0f;
    return -((sheep_timer - 40.0f) - partial) / 4.0f;
}

static float passive_sheep_head_pitch_125(float sheep_timer, float partial, float normal_pitch_rad) {
    /* EntitySheep.func_44002_d: eating head bob. */
    if (sheep_timer > 4.0f && sheep_timer <= 36.0f) {
        float t = ((sheep_timer - 4.0f) - partial) / 32.0f;
        return (float)M_PI * 0.2f + (float)M_PI * 0.07f * sinf(t * 28.7f);
    }
    return sheep_timer > 0.0f ? (float)M_PI * 0.2f : normal_pitch_rad;
}

static int passive_block_is_liquid(int id) {
    return block_is_liquid(id);
}

static int passive_block_is_water(int id) {
    return block_is_water(id);
}

static int passive_mob_aabb_has_liquid_box(float minx, float miny, float minz, float maxx, float maxy, float maxz, int water_only) {
    int x0 = (int)floorf(minx);
    int x1 = (int)floorf(maxx - 0.001f);
    int y0 = (int)floorf(miny);
    int y1 = (int)floorf(maxy - 0.001f);
    int z0 = (int)floorf(minz);
    int z1 = (int)floorf(maxz - 0.001f);
    for (int y = y0; y <= y1; ++y) {
        for (int z = z0; z <= z1; ++z) {
            for (int x = x0; x <= x1; ++x) {
                int id = flat_get_block(x, y, z);
                if (water_only ? passive_block_is_water(id) : passive_block_is_liquid(id)) return 1;
            }
        }
    }
    return 0;
}

static int passive_mob_in_water(const PassiveMob *m) {
    float hw = m->width * 0.5f;
    return passive_mob_aabb_has_liquid_box(m->x - hw, m->y - 0.40f, m->z - hw,
                                           m->x + hw, m->y + m->height, m->z + hw, 1);
}

static int passive_mob_in_liquid(const PassiveMob *m) {
    float hw = m->width * 0.5f;
    return passive_mob_aabb_has_liquid_box(m->x - hw, m->y - 0.40f, m->z - hw,
                                           m->x + hw, m->y + m->height, m->z + hw, 0);
}

static int passive_mob_spawn_near_liquid(int x, int y, int z) {
    for (int oz = -1; oz <= 1; ++oz) {
        for (int ox = -1; ox <= 1; ++ox) {
            if (passive_block_is_liquid(flat_get_block(x + ox, y - 1, z + oz)) ||
                passive_block_is_liquid(flat_get_block(x + ox, y, z + oz)) ||
                passive_block_is_liquid(flat_get_block(x + ox, y + 1, z + oz))) {
                return 1;
            }
        }
    }
    return 0;
}

static const char *passive_mob_name(int type) {
    const PexMobInfo *info = passive_mob_info(type);
    return info ? info->name : "Mob";
}

static void pex_damage_source_set_direct_mob(PexDamageSource *source, PassiveMob *mob) {
    if (!source || !mob) return;
    source->direct_kind = PEX_DAMAGE_ENTITY_MOB;
    source->direct_mob_index = (int)(mob - g_passive_mobs);
    source->direct_mob_type = mob->type;
    source->direct_x = mob->x;
    source->direct_y = mob->y;
    source->direct_z = mob->z;
}

static void pex_damage_source_set_true_mob(PexDamageSource *source, PassiveMob *mob) {
    if (!source || !mob) return;
    source->true_kind = PEX_DAMAGE_ENTITY_MOB;
    source->true_mob_index = (int)(mob - g_passive_mobs);
    source->true_mob_type = mob->type;
    source->true_x = mob->x;
    source->true_y = mob->y;
    source->true_z = mob->z;
}

static PexDamageSource pex_damage_source_mob(PassiveMob *mob) {
    PexDamageSource source = pex_damage_source_simple(PEX_DAMAGE_MOB);
    pex_damage_source_set_direct_mob(&source, mob);
    pex_damage_source_set_true_mob(&source, mob);
    return source;
}

static PassiveMob *pex_projectile_owner_mob(const FlatProjectile *projectile) {
    if (!projectile || projectile->owner_type != 1) return NULL;
    if (projectile->owner_mob_index >= 0 && projectile->owner_mob_index < MAX_PASSIVE_MOBS) {
        PassiveMob *m = &g_passive_mobs[projectile->owner_mob_index];
        if (m->active && m->type == projectile->owner_mob_type) return m;
    }
    return NULL;
}

static void pex_damage_source_attach_projectile(PexDamageSource *source, const FlatProjectile *projectile) {
    if (!source || !projectile) return;
    source->direct_kind = PEX_DAMAGE_ENTITY_PROJECTILE;
    source->direct_mob_type = projectile->owner_mob_type;
    source->direct_mob_index = projectile->owner_mob_index;
    source->direct_x = projectile->x;
    source->direct_y = projectile->y;
    source->direct_z = projectile->z;
    if (projectile->owner_type == 0) {
        source->true_kind = PEX_DAMAGE_ENTITY_PLAYER;
        source->true_x = g_player_x;
        source->true_y = g_player_y;
        source->true_z = g_player_z;
    } else {
        PassiveMob *owner = pex_projectile_owner_mob(projectile);
        if (owner) {
            pex_damage_source_set_true_mob(source, owner);
        } else {
            source->true_kind = PEX_DAMAGE_ENTITY_MOB;
            source->true_mob_index = projectile->owner_mob_index;
            source->true_mob_type = projectile->owner_mob_type;
            source->true_x = projectile->x;
            source->true_y = projectile->y;
            source->true_z = projectile->z;
        }
    }
}

static PexDamageSource pex_damage_source_arrow(const FlatProjectile *projectile) {
    PexDamageSource source = pex_damage_source_simple(PEX_DAMAGE_ARROW);
    pex_damage_source_attach_projectile(&source, projectile);
    return source;
}

static PexDamageSource pex_damage_source_fireball(const FlatProjectile *projectile) {
    PexDamageSource source = pex_damage_source_simple(PEX_DAMAGE_FIREBALL);
    pex_damage_source_attach_projectile(&source, projectile);
    return source;
}

static PexDamageSource pex_damage_source_thrown(const FlatProjectile *projectile) {
    PexDamageSource source = pex_damage_source_simple(PEX_DAMAGE_THROWN);
    pex_damage_source_attach_projectile(&source, projectile);
    return source;
}

static PexDamageSource pex_damage_source_indirect_magic(const FlatProjectile *projectile) {
    PexDamageSource source = pex_damage_source_simple(PEX_DAMAGE_INDIRECT_MAGIC);
    pex_damage_source_attach_projectile(&source, projectile);
    return source;
}

static float passive_mob_width_for_type(int type) {
    const PexMobInfo *info = passive_mob_info(type);
    return info ? info->width : 0.6f;
}
static float passive_mob_height_for_type(int type) {
    const PexMobInfo *info = passive_mob_info(type);
    return info ? info->height : 1.8f;
}

static int passive_sheep_random_fleece_color(void) {
    int r = rand() % 100;
    if (r < 5) return 15;
    if (r < 10) return 7;
    if (r < 15) return 8;
    if (r < 18) return 12;
    if ((rand() % 500) == 0) return 6;
    return 0;
}

static int passive_mob_type_valid(int type) {
    return passive_mob_info(type) != NULL;
}

static int passive_mob_health_for_type(int type) {
    const PexMobInfo *info = passive_mob_info(type);
    return info ? info->health : 1;
}

static int passive_mob_max_health_current_125(const PassiveMob *m) {
    if (!m) return 1;
    /* EntityWolf.getMaxHealth(): 20 when tamed, 8 while wild/angry. */
    if (m->type == PASSIVE_MOB_WOLF && passive_mob_is_owned_by_player(m)) return 20;
    return passive_mob_health_for_type(m->type);
}

static void passive_mob_clamp_health_current_125(PassiveMob *m) {
    if (!m) return;
    int cap = passive_mob_max_health_current_125(m);
    if (m->health > cap) m->health = cap;
    if (m->prev_health > cap) m->prev_health = cap;
}

static float passive_mob_sound_volume(int type) {
    const PexMobInfo *info = passive_mob_info(type);
    return info ? info->sound_volume : 1.0f;
}
static const char *passive_mob_living_sound(int type) {
    const PexMobInfo *info = passive_mob_info(type);
    return info ? info->living_sound : NULL;
}

static const char *passive_mob_living_sound_current_125(const PassiveMob *m) {
    if (!m) return NULL;
    if (m->type == PASSIVE_MOB_WOLF) {
        if (m->rideable > 0 && !passive_mob_is_owned_by_player(m)) return "mob.wolf.growl";
        if (passive_mob_is_owned_by_player(m) && m->health < 10) return "mob.wolf.whine";
        return (rand() % 3) == 0 ? "mob.wolf.panting" : "mob.wolf.bark";
    }
    if (m->type == PASSIVE_MOB_OCELOT) {
        if (!passive_mob_is_owned_by_player(m)) return NULL;
        if (m->love_time > 0) return "mob.cat.purr";
        return (rand() % 4) == 0 ? "mob.cat.purreow" : "mob.cat.meow";
    }
    if (m->type == PASSIVE_MOB_PIG_ZOMBIE && m->rideable > 0) {
        return "mob.zombiepig.zpigangry";
    }
    return passive_mob_living_sound(m->type);
}

static const char *passive_mob_hurt_sound(int type) {
    const PexMobInfo *info = passive_mob_info(type);
    return info ? info->hurt_sound : NULL;
}
static const char *passive_mob_death_sound(int type) {
    const PexMobInfo *info = passive_mob_info(type);
    return info ? info->death_sound : NULL;
}
static void passive_mob_drop_stack(PassiveMob *m, int item_id, int count) {
    if (!m || item_id <= 0 || count <= 0) return;
    for (int i = 0; i < count; ++i) {
        spawn_item_stack(m->x, m->y + 0.5f, m->z, make_stack(item_id, 1, 0), 1);
    }
}

static void passive_mob_drop_itemstack_125(PassiveMob *m, ItemStack st) {
    if (!m || st.id <= 0 || st.count <= 0) return;
    spawn_item_stack(m->x, m->y + 0.5f, m->z, st, 1);
}

static ItemStack passive_make_random_enchanted_equipment_125(int item_id) {
    ItemStack st = make_stack(item_id, 1, 0);
    /* Port hook for EntityLiving.dropRareDrop(enchanted=1). Java calls
       EnchantmentHelper.func_48441_a(rand, stack, 5). Full enchant tables are
       outside normal-mob stability, but this stores real ItemStack enchant
       pairs so rare drops can carry enchant NBT-shaped data. */
    if (item_id == ITEM_SWORD_GOLD || item_id == ITEM_BOW || item_id == ITEM_SWORD_IRON || item_id == ITEM_SHOVEL_IRON) {
        int lvl = 1 + (rand() % 2);
        item_stack_set_enchant_125(&st, (item_id == ITEM_BOW) ? 48 : 16, lvl); /* Power or Sharpness */
    }
    return st;
}

static void passive_mob_apply_default_equipment_125(PassiveMob *m) {
    if (!m) return;
    if (m->held_item <= 0) {
        if (m->type == PASSIVE_MOB_SKELETON) {
            /* EntitySkeleton.getHeldItem returns a bow.  Keep this as saved
               mob state instead of a renderer-only hardcode so ranged AI,
               render layers, and future equipment drops all see the same data. */
            m->held_item = ITEM_BOW;
        } else if (m->type == PASSIVE_MOB_PIG_ZOMBIE) {
            /* EntityPigZombie.getHeldItem returns a golden sword. */
            m->held_item = ITEM_SWORD_GOLD;
        }
    }
}

static int passive_mob_recently_hit_by_player_125(const PassiveMob *m, const PexDamageSource *source) {
    return (source && source->true_kind == PEX_DAMAGE_ENTITY_PLAYER) || (m && m->recently_hit > 0);
}

static int passive_mob_looting_level_125(const PassiveMob *m, const PexDamageSource *source) {
    /* Java EntityLiving.onDeath asks EnchantmentHelper.getLootingModifier when
       the true killer is the player.  The C port caches the player's held
       ItemStack Looting level into last_looting_level at hit time, matching
       the recentlyHit death-window behavior. */
    if (!passive_mob_recently_hit_by_player_125(m, source)) return 0;
    int lvl = m ? m->last_looting_level : 0;
    if (lvl < 0) lvl = 0;
    if (lvl > 3) lvl = 3;
    return lvl;
}

static int passive_drop_rand_125(int exclusive) {
    return exclusive <= 1 ? 0 : (rand() % exclusive);
}

static int passive_drop_count_0_to_base_minus_1_125(int base_exclusive, int looting) {
    int n = passive_drop_rand_125(base_exclusive);
    if (looting > 0) n += rand() % (looting + 1);
    return n;
}

static int passive_drop_count_1_plus_base_125(int base_exclusive, int looting) {
    return 1 + passive_drop_count_0_to_base_minus_1_125(base_exclusive, looting);
}


static int passive_creeper_self_explosion_death_125(const PassiveMob *m, const PexDamageSource *source) {
    return m && source && m->type == PASSIVE_MOB_CREEPER && source->type == PEX_DAMAGE_EXPLOSION &&
           source->true_kind == PEX_DAMAGE_ENTITY_MOB && source->true_mob_type == PASSIVE_MOB_CREEPER;
}

static int passive_mob_drops_cooked_125(const PassiveMob *m, const PexDamageSource *source) {
    if (!m) return 0;
    return m->burn_time > 0 || (source && source->fire_damage);
}

static void passive_mob_drop_rare_drop_125(PassiveMob *m, const PexDamageSource *source) {
    if (!m || !passive_mob_recently_hit_by_player_125(m, source)) return;
    int looting = passive_mob_looting_level_125(m, source);
    /* EntityLiving.onDeath: rand.nextInt(200) - looting < 5. */
    int rare_roll = (rand() % 200) - looting;
    if (rare_roll >= 5) return;
    int enchanted_branch = rare_roll <= 0;
    switch (m->type) {
        case PASSIVE_MOB_ZOMBIE:
            switch (rand() & 3) {
                case 0: passive_mob_drop_stack(m, ITEM_SWORD_IRON, 1); break;
                case 1: passive_mob_drop_stack(m, ITEM_HELMET_IRON, 1); break;
                case 2: passive_mob_drop_stack(m, ITEM_INGOT_IRON, 1); break;
                default: passive_mob_drop_stack(m, ITEM_SHOVEL_IRON, 1); break;
            }
            break;
        case PASSIVE_MOB_SKELETON:
            if (enchanted_branch) passive_mob_drop_itemstack_125(m, passive_make_random_enchanted_equipment_125(ITEM_BOW));
            else passive_mob_drop_stack(m, ITEM_BOW, 1);
            break;
        case PASSIVE_MOB_PIG_ZOMBIE:
            if (enchanted_branch) {
                passive_mob_drop_itemstack_125(m, passive_make_random_enchanted_equipment_125(ITEM_SWORD_GOLD));
            } else {
                switch (rand() % 3) {
                    case 0: passive_mob_drop_stack(m, ITEM_INGOT_GOLD, 1); break;
                    case 1: passive_mob_drop_stack(m, ITEM_SWORD_GOLD, 1); break;
                    default: passive_mob_drop_stack(m, ITEM_HELMET_GOLD, 1); break;
                }
            }
            break;
        default:
            break;
    }
}

static void passive_mob_drop_on_death(PassiveMob *m, const PexDamageSource *source) {
    if (!m || m->death_drops_done) return;
    m->death_drops_done = 1;
    if (m->baby_age < 0) return; /* Java EntityLiving does not drop items from child mobs. */
    if (passive_creeper_self_explosion_death_125(m, source)) return;
    int cooked = passive_mob_drops_cooked_125(m, source);
    int looting = passive_mob_looting_level_125(m, source);
    switch (m->type) {
        case PASSIVE_MOB_PIG:
            passive_mob_drop_stack(m, cooked ? ITEM_PORK_COOKED : ITEM_PORK_RAW, passive_drop_count_0_to_base_minus_1_125(3, looting));
            break;
        case PASSIVE_MOB_COW:
            passive_mob_drop_stack(m, ITEM_LEATHER, passive_drop_count_0_to_base_minus_1_125(3, looting));
            passive_mob_drop_stack(m, cooked ? ITEM_BEEF_COOKED : ITEM_BEEF_RAW, passive_drop_count_1_plus_base_125(3, looting));
            break;
        case PASSIVE_MOB_CHICKEN:
            passive_mob_drop_stack(m, ITEM_FEATHER, passive_drop_count_0_to_base_minus_1_125(3, looting));
            passive_mob_drop_stack(m, cooked ? ITEM_CHICKEN_COOKED : ITEM_CHICKEN_RAW, 1);
            break;
        case PASSIVE_MOB_SHEEP:
            if (!m->sheared) spawn_item_stack(m->x, m->y + 0.5f, m->z, make_stack(BLOCK_WOOL, 1, m->fleece_color & 15), 1);
            break;
        case PASSIVE_MOB_CREEPER:
            passive_mob_drop_stack(m, ITEM_GUNPOWDER, passive_drop_count_0_to_base_minus_1_125(3, looting));
            break;
        case PASSIVE_MOB_SKELETON:
            passive_mob_drop_stack(m, ITEM_ARROW, passive_drop_count_0_to_base_minus_1_125(3, looting));
            passive_mob_drop_stack(m, ITEM_BONE, passive_drop_count_0_to_base_minus_1_125(3, looting));
            break;
        case PASSIVE_MOB_SPIDER:
        case PASSIVE_MOB_CAVE_SPIDER:
            passive_mob_drop_stack(m, ITEM_STRING, passive_drop_count_0_to_base_minus_1_125(3, looting));
            if (passive_mob_recently_hit_by_player_125(m, source) && ((rand() % 3) == 0 || (looting > 0 && (rand() % (looting + 1)) > 0))) passive_mob_drop_stack(m, ITEM_SPIDER_EYE, 1);
            break;
        case PASSIVE_MOB_ZOMBIE:
        case PASSIVE_MOB_GIANT:
            passive_mob_drop_stack(m, ITEM_ROTTEN_FLESH, passive_drop_count_0_to_base_minus_1_125(3, looting));
            break;
        case PASSIVE_MOB_SLIME:
            /* EntitySlime only drops slimeballs from the smallest size. */
            if (m->fleece_color <= 1) passive_mob_drop_stack(m, ITEM_SLIME_BALL, passive_drop_count_0_to_base_minus_1_125(3, looting));
            break;
        case PASSIVE_MOB_GHAST:
            passive_mob_drop_stack(m, ITEM_GHAST_TEAR, passive_drop_count_0_to_base_minus_1_125(2, looting));
            passive_mob_drop_stack(m, ITEM_GUNPOWDER, passive_drop_count_0_to_base_minus_1_125(3, looting));
            break;
        case PASSIVE_MOB_PIG_ZOMBIE:
            passive_mob_drop_stack(m, ITEM_ROTTEN_FLESH, passive_drop_count_0_to_base_minus_1_125(2, looting));
            passive_mob_drop_stack(m, ITEM_GOLD_NUGGET, passive_drop_count_0_to_base_minus_1_125(2, looting));
            break;
        case PASSIVE_MOB_ENDERMAN:
            passive_mob_drop_stack(m, ITEM_ENDER_PEARL, passive_drop_count_0_to_base_minus_1_125(2, looting));
            break;
        case PASSIVE_MOB_BLAZE:
            if (passive_mob_recently_hit_by_player_125(m, source)) passive_mob_drop_stack(m, ITEM_BLAZE_ROD, passive_drop_count_0_to_base_minus_1_125(2, looting));
            break;
        case PASSIVE_MOB_MAGMA_CUBE:
            /* EntityMagmaCube: only non-small cubes can drop cream. */
            if (m->fleece_color > 1) {
                int magma_count = (rand() % 4) - 2;
                if (looting > 0) magma_count += rand() % (looting + 1);
                if (magma_count > 0) passive_mob_drop_stack(m, ITEM_MAGMA_CREAM, magma_count);
            }
            break;
        case PASSIVE_MOB_SQUID:
            passive_mob_drop_stack(m, ITEM_DYE_POWDER, passive_drop_count_1_plus_base_125(3, looting));
            break;
        case PASSIVE_MOB_MOOSHROOM:
            passive_mob_drop_stack(m, ITEM_LEATHER, passive_drop_count_0_to_base_minus_1_125(3, looting));
            passive_mob_drop_stack(m, cooked ? ITEM_BEEF_COOKED : ITEM_BEEF_RAW, passive_drop_count_1_plus_base_125(3, looting));
            break;
        case PASSIVE_MOB_SNOWMAN:
            passive_mob_drop_stack(m, ITEM_SNOWBALL, rand() % 16);
            break;
        case PASSIVE_MOB_IRON_GOLEM:
            passive_mob_drop_stack(m, ITEM_INGOT_IRON, 3 + (rand() % 3));
            passive_mob_drop_stack(m, BLOCK_RED_ROSE, rand() % 3);
            break;
        default:
            break;
    }
    /* EntityCreeper.onDeath: skeleton-killed creepers drop one music disc. */
    if (m->type == PASSIVE_MOB_CREEPER && source && source->true_kind == PEX_DAMAGE_ENTITY_MOB &&
        source->true_mob_type == PASSIVE_MOB_SKELETON) {
        passive_mob_drop_stack(m, ITEM_RECORD13 + (rand() % 10), 1);
    }
    passive_mob_drop_rare_drop_125(m, source);
}

static void passive_mobs_reset(void) {
    memset(g_passive_mobs, 0, sizeof(g_passive_mobs));
    memset(g_passive_spawn_y_cache, 0, sizeof(g_passive_spawn_y_cache));
    g_passive_dragon_killed_125 = 0;
    memset(g_dragon_crystals_125, 0, sizeof(g_dragon_crystals_125));
    g_dragon_crystals_seeded_125 = 0;
    g_prof_spawn_y_cache_hits = g_prof_spawn_y_cache_misses = 0;
    g_passive_spawn_probe_budget = -1;
    g_player_riding_passive_mob = -1;
}

static PassiveMob *passive_mob_alloc(void) {
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) if (!g_passive_mobs[i].active) return &g_passive_mobs[i];
    return NULL;
}

static int passive_mob_count(void) {
    int n = 0;
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) if (g_passive_mobs[i].active) ++n;
    return n;
}

static int passive_mob_target_cap(void);

static void passive_mobs_enforce_cap(void) {
    int cap = passive_mob_target_cap();
    int count = passive_mob_count();
    if (count <= cap) return;
    /* Cull extras from old saves / older patches that filled all 40 slots.
       Keep the ridden pig alive. Prefer removing far mobs first. */
    for (int pass = 0; pass < 2 && count > cap; ++pass) {
        for (int i = MAX_PASSIVE_MOBS - 1; i >= 0 && count > cap; --i) {
            PassiveMob *m = &g_passive_mobs[i];
            if (!m->active) continue;
            if (i == g_player_riding_passive_mob) continue;
            float dx = m->x - g_player_x;
            float dz = m->z - g_player_z;
            float d2 = dx * dx + dz * dz;
            if (pass == 0 && d2 < 48.0f * 48.0f) continue;
            memset(m, 0, sizeof(*m));
            --count;
        }
    }
}

static int passive_mob_target_cap(void) {
    return MAX_PASSIVE_MOBS < PEX_PASSIVE_TARGET_CAP ? MAX_PASSIVE_MOBS : PEX_PASSIVE_TARGET_CAP;
}

static int passive_mob_ground_target_ok(int x, int y, int z) {
    if (y <= FLAT_WORLD_Y_MIN || y + 1 > FLAT_WORLD_Y_MAX) return 0;
    int below = flat_get_block(x, y - 1, z);
    if (passive_block_is_liquid(below)) return 0;
    if (below != BLOCK_GRASS && !flat_block_is_solid(below)) return 0;
    if (flat_get_block(x, y, z) != 0 || flat_get_block(x, y + 1, z) != 0) return 0;
    /* Vanilla b1.0 does not reject grass near water.  The old extra shore/water
       guard made beach/forest worlds feel empty because most valid grass around
       lakes and coasts was thrown away before the actual spawn rules ran. */
    return 1;
}

static float passive_mob_path_weight_125(const PassiveMob *m, int x, int y, int z) {
    if (!m) return 0.0f;
    float light = flat_light_brightness(x, y, z);
    if (passive_mob_category(m->type) == PEX_CAT_MONSTER) return 0.5f - light;
    if (passive_mob_category(m->type) == PEX_CAT_CREATURE) {
        return flat_get_block(x, y - 1, z) == BLOCK_GRASS ? 10.0f : light - 0.5f;
    }
    if (m->type == PASSIVE_MOB_SQUID) {
        return passive_block_is_water(flat_get_block(x, y, z)) ? 10.0f : -99999.0f;
    }
    return 0.0f;
}

static void passive_mob_init(PassiveMob *m, int type, float x, float y, float z) {
    if (!m) return;
    memset(m, 0, sizeof(*m));
    m->active = 1;
    m->type = type;
    m->x = m->prev_x = x;
    m->y = m->prev_y = y;
    m->z = m->prev_z = z;
    m->yaw = m->prev_yaw = pex_rand_float01() * 360.0f;
    m->render_yaw = m->prev_render_yaw = m->yaw;
    m->width = passive_mob_width_for_type(type);
    m->height = passive_mob_height_for_type(type);
    m->health = passive_mob_health_for_type(type);
    m->prev_health = m->health;
    m->damage_remainder = 0;
    m->fleece_color = (type == PASSIVE_MOB_SHEEP) ? passive_sheep_random_fleece_color() : 0;
    if (type == PASSIVE_MOB_VILLAGER) m->fleece_color = rand() % 5;
    if (passive_mob_is_slime_family(type)) {
        /* EntitySlime.setSlimeSize(1 << rand.nextInt(3)): legal sizes are 1, 2, 4. */
        int sz = 1 << (rand() % 3);
        m->fleece_color = sz;
        m->width = 0.6f * (float)sz;
        m->height = 0.6f * (float)sz;
        m->health = sz * sz;
        m->prev_health = m->health;
    }
    m->living_sound_delay = -120;
    m->jump_cooldown = 0;
    m->attack_time = 0;
    m->burn_time = 0;
    m->target_mob_index = -1;
    m->baby_age = 0;
    m->sitting = 0;
    m->held_block = 0;
    m->held_item = 0;
    memset(m->equipment, 0, sizeof(m->equipment));
    passive_mob_apply_default_equipment_125(m);
    m->love_time = 0;
    m->stuck_ticks = 0;
    m->wander_cooldown = 0;
    m->owner_id = PEX_MOB_OWNER_NONE;
    m->tame_state = 0;
    m->collar_color = 14; /* Java wolf collar default: red dye */
    m->village_id = -1;
    m->door_target_x = m->door_target_y = m->door_target_z = 0;
    m->door_open_time = 0;
    m->ai_task_mask = 0;
    m->ai_age = rand() % 100;
    m->ai_repath_delay = 0;
    m->path_stuck_check_tick = 0;
    m->path_stuck_check_x = m->x;
    m->path_stuck_check_y = m->y;
    m->path_stuck_check_z = m->z;
    m->path_goal_x = m->x;
    m->path_goal_y = m->y;
    m->path_goal_z = m->z;
    m->path_len = m->path_index = m->path_recalc_cooldown = 0;
    if (type == PASSIVE_MOB_CHICKEN) {
        m->chicken_wing_speed = 1.0f;
        m->egg_timer = rand() % 6000 + 6000;
    } else {
        m->egg_timer = 0; /* reused as hostile attack/cooldown/fuse timer */
    }
    pex_logf("passive mob spawn type=%s pos=%.2f,%.2f,%.2f", passive_mob_name(type), x, y, z);
}


static void passive_silverfish_spawn_from_block_125(int x, int y, int z) {
    PassiveMob *m = passive_mob_alloc();
    if (!m) return;
    passive_mob_init(m, PASSIVE_MOB_SILVERFISH, (float)x + 0.5f, (float)y, (float)z + 0.5f);
    m->yaw = m->prev_yaw = pex_rand_float01() * 360.0f;
    m->render_yaw = m->prev_render_yaw = m->yaw;
    spawn_block_destroy_particles(x, y, z, BLOCK_SILVERFISH);
    g_save_dirty = 1;
}

static void passive_mobs_on_block_broken_125(int block_id, int meta, int x, int y, int z) {
    (void)meta;
    if (g_mp_connected) return;
    /* BlockSilverfish.onBlockDestroyedByPlayer: breaking the disguise block
       spawns one silverfish and drops nothing.  Metadata affects only the
       disguise texture/creative stack, not the spawned entity. */
    if (block_id == BLOCK_SILVERFISH) passive_silverfish_spawn_from_block_125(x, y, z);
    if (block_id == BLOCK_MOB_SPAWNER) passive_mob_spawner_remove_tile_125(x, y, z);
}

static int passive_silverfish_block_meta_for_id_125(int id) {
    if (id == BLOCK_STONE) return 0;
    if (id == BLOCK_COBBLESTONE) return 1;
    if (id == BLOCK_STONE_BRICK) return 2;
    return -1;
}


static void passive_mob_aabb(const PassiveMob *m, FlatAABB *box) {
    float hw = m->width * 0.5f;
    box->minx = m->x - hw;
    box->maxx = m->x + hw;
    box->miny = m->y;
    box->maxy = m->y + m->height;
    box->minz = m->z - hw;
    box->maxz = m->z + hw;
}

static void passive_mob_move_entity(PassiveMob *m, float dx, float dy, float dz) {
    FlatAABB box;
    passive_mob_aabb(m, &box);
    FlatAABB sweep = box;
    if (dx < 0.0f) sweep.minx += dx; else sweep.maxx += dx;
    if (dy < 0.0f) sweep.miny += dy; else sweep.maxy += dy;
    if (dz < 0.0f) sweep.minz += dz; else sweep.maxz += dz;

    FlatAABB boxes[160];
    int n = flat_get_collision_boxes(&sweep, boxes, 160);
    float odx = dx, ody = dy, odz = dz;
    for (int i = 0; i < n; ++i) dy = aabb_clip_y(&boxes[i], &box, dy);
    aabb_offset(&box, 0.0f, dy, 0.0f);
    for (int i = 0; i < n; ++i) dx = aabb_clip_x(&boxes[i], &box, dx);
    aabb_offset(&box, dx, 0.0f, 0.0f);
    for (int i = 0; i < n; ++i) dz = aabb_clip_z(&boxes[i], &box, dz);
    aabb_offset(&box, 0.0f, 0.0f, dz);

    m->x = (box.minx + box.maxx) * 0.5f;
    m->y = box.miny;
    m->z = (box.minz + box.maxz) * 0.5f;
    m->on_ground = (ody != dy && ody < 0.0f) ? 1 : 0;
    m->collided_horizontal = (odx != dx || odz != dz) ? 1 : 0;
    m->collided_vertical = (ody != dy) ? 1 : 0;
    if (odx != dx) m->mx = 0.0f;
    if (ody != dy) m->my = 0.0f;
    if (odz != dz) m->mz = 0.0f;
}

static int passive_mob_can_stand_at(int x, int y, int z) {
    if (!passive_mob_ground_target_ok(x, y, z)) return 0;
    return flat_get_block(x, y - 1, z) == BLOCK_GRASS;
}

static int passive_mob_has_spawn_light(int x, int y, int z) {
    return flat_combined_light(x, y, z) > 8;
}

static int passive_mob_has_spawn_clearance(int type, float x, float y, float z) {
    if (!passive_mob_type_valid(type)) return 0;
    float hw = passive_mob_width_for_type(type) * 0.5f;
    float h = passive_mob_height_for_type(type);
    FlatAABB b = { x - hw + 0.02f, y + 0.01f, z - hw + 0.02f,
                   x + hw - 0.02f, y + h, z + hw - 0.02f };
    if (passive_mob_aabb_has_liquid_box(b.minx, b.miny, b.minz, b.maxx, b.maxy, b.maxz, 0)) return 0;
    FlatAABB boxes[64];
    return flat_get_collision_boxes(&b, boxes, 64) <= 0;
}

static int passive_mob_far_enough_from_player(float x, float y, float z) {
    float dx = x - g_player_x;
    float dy = y - g_player_y;
    float dz = z - g_player_z;
    return dx * dx + dy * dy + dz * dz >= 24.0f * 24.0f;
}

static int passive_mob_far_enough_from_world_spawn(float x, float y, float z) {
    return x * x + y * y + z * z >= 24.0f * 24.0f;
}

static int passive_mob_air_spawn_base_ok(int x, int y, int z) {
    if (y <= FLAT_WORLD_Y_MIN || y + 1 > FLAT_WORLD_Y_MAX) return 0;
    int below = flat_get_block(x, y - 1, z);
    if (below == BLOCK_BEDROCK || !flat_block_is_solid(below) || passive_block_is_liquid(below)) return 0;
    if (flat_get_block(x, y, z) != 0 || flat_get_block(x, y + 1, z) != 0) return 0;
    if (passive_block_is_liquid(flat_get_block(x, y, z))) return 0;
    return 1;
}

static int passive_mob_valid_dark_spawn_light(int x, int y, int z) {
    int sky = flat_saved_sky_light(x, y, z);
    if (sky > (rand() & 31)) return 0;
    return flat_combined_light(x, y, z) <= (rand() & 7);
}

static int passive_slime_chunk_ok(int x, int z) {
    int cx = floor_div16(x);
    int cz = floor_div16(z);
    JavaRandom r;
    int64_t seed = (int64_t)g_world_seed +
        (int64_t)(cx * cx * 4987142) + (int64_t)(cx * 5947611) +
        (int64_t)(cz * cz) * 4392871LL + (int64_t)(cz * 389711) ^ 987234911LL;
    jr_set_seed(&r, seed);
    return jr_next_int_bound(&r, 10) == 0;
}

static int passive_mob_type_specific_can_spawn(int type, int x, int y, int z, int cat) {
    int biome = passive_current_biome_id_at(x, z);
    if (cat == PEX_CAT_MONSTER && type != PASSIVE_MOB_OCELOT && (g_opts.difficulty & 3) == 0) return 0;
    if (type == PASSIVE_MOB_GIANT || type == PASSIVE_MOB_ENDER_DRAGON || type == PASSIVE_MOB_BLAZE ||
        type == PASSIVE_MOB_CAVE_SPIDER || type == PASSIVE_MOB_SILVERFISH) return 0;

    if (cat == PEX_CAT_WATER || type == PASSIVE_MOB_SQUID) {
        if (y <= 45 || y >= 63) return 0;
        if (!passive_block_is_water(flat_get_block(x, y, z))) return 0;
        if (flat_block_is_solid(flat_get_block(x, y + 1, z))) return 0;
        return 1;
    }

    if (type == PASSIVE_MOB_OCELOT) {
        if (biome != BIOME_JUNGLE && biome != BIOME_JUNGLE_HILLS) return 0;
        if ((rand() % 3) == 0) return 0;
        if (y < 63) return 0;
        int below = flat_get_block(x, y - 1, z);
        if (below != BLOCK_GRASS && below != BLOCK_LEAVES) return 0;
        return passive_mob_air_spawn_base_ok(x, y, z) && passive_mob_has_spawn_clearance(type, (float)x+0.5f, (float)y, (float)z+0.5f);
    }

    if (biome == BIOME_HELL && cat == PEX_CAT_MONSTER) {
        if (type == PASSIVE_MOB_GHAST) {
            if ((rand() % 20) != 0) return 0; /* EntityGhast.getCanSpawnHere */
            return passive_mob_has_spawn_clearance(type, (float)x+0.5f, (float)y, (float)z+0.5f);
        }
        if (type == PASSIVE_MOB_PIG_ZOMBIE || type == PASSIVE_MOB_MAGMA_CUBE) {
            return passive_mob_has_spawn_clearance(type, (float)x+0.5f, (float)y, (float)z+0.5f);
        }
    }

    if (!passive_mob_air_spawn_base_ok(x, y, z)) return 0;

    if (cat == PEX_CAT_CREATURE) {
        if (type == PASSIVE_MOB_MOOSHROOM) {
            if (biome != BIOME_MUSHROOM_ISLAND && biome != BIOME_MUSHROOM_ISLAND_SHORE) return 0;
            if (flat_get_block(x, y - 1, z) != BLOCK_MYCELIUM) return 0;
            return passive_mob_has_spawn_light(x, y, z);
        }
        if (!passive_biome_allows_creatures(biome)) return 0;
        if (flat_get_block(x, y - 1, z) != BLOCK_GRASS) return 0;
        if (!passive_mob_has_spawn_light(x, y, z)) return 0;
        return 1;
    }

    if (cat == PEX_CAT_MONSTER) {
        if (!passive_biome_allows_monsters(biome)) return 0;
        if (type == PASSIVE_MOB_SLIME) {
            if (y >= 40) return 0;
            if ((rand() % 10) != 0) return 0;
            if (!passive_slime_chunk_ok(x, z)) return 0;
            return 1;
        }
        return passive_mob_valid_dark_spawn_light(x, y, z);
    }

    return 0;
}

static int passive_mob_can_spawn_at_category(int type, int cat, int x, int y, int z) {
    if (!passive_mob_type_valid(type)) return 0;
    if (!flat_chunk_generated_at_block(x, z)) return 0;
    float fx = (float)x + 0.5f;
    float fy = (float)y;
    float fz = (float)z + 0.5f;
    if (!passive_mob_far_enough_from_player(fx, fy, fz)) return 0;
    if (!passive_mob_far_enough_from_world_spawn(fx, fy, fz)) return 0;
    if (!passive_mob_type_specific_can_spawn(type, x, y, z, cat)) return 0;
    return passive_mob_has_spawn_clearance(type, fx, fy, fz);
}

static int passive_mob_can_spawn_at(int type, int x, int y, int z) {
    return passive_mob_can_spawn_at_category(type, passive_mob_category(type), x, y, z);
}

static int passive_spawn_probe_step(void) {
    /* g_passive_spawn_probe_budget < 0 means an intentional unbudgeted caller
       such as initial population seeding.  Natural spawning sets a positive
       budget each tick so failed surface scans cannot monopolize the frame. */
    if (g_passive_spawn_probe_budget == 0) return 0;
    if (g_passive_spawn_probe_budget > 0) --g_passive_spawn_probe_budget;
    ++g_prof_mob_spawn_calls_last;
    return 1;
}

static int passive_mob_column_spawn_y_for_category(int type, int cat, int x, int z) {
    int cached_y = -9999;
    ++g_prof_mob_spawn_columns_last;
    if (x < g_flat_world_origin_x + 1 || x >= g_flat_world_origin_x + FLAT_WORLD_SIZE - 1 ||
        z < g_flat_world_origin_z + 1 || z >= g_flat_world_origin_z + FLAT_WORLD_SIZE - 1) {
        passive_spawn_y_cache_put(type, cat, x, z, -9999);
        return -9999;
    }
    if (!flat_chunk_generated_at_block(x, z)) {
        passive_spawn_y_cache_put(type, cat, x, z, -9999);
        return -9999;
    }
    /* Natural mob spawning must never repair skylight.  That check scanned a
       chunk column grid for many spawn probes and queued full chunk relights,
       showing up as Entity passive mobs ~4ms and light_worker ~250ms while the
       player was simply placing blocks.  Spawning can tolerate stale light for
       a tick; the renderer/lighting system owns repairs. */
    (void)flat_sky_light_needs_rebuild_at_block;
    (void)flat_queue_light_repair_at_block;
    if (passive_spawn_y_cache_get(type, cat, x, z, &cached_y)) {
        ++g_prof_mob_spawn_probe_hits_last;
        return cached_y;
    }
    ++g_prof_mob_spawn_probe_misses_last;
    if (cat == PEX_CAT_WATER || type == PASSIVE_MOB_SQUID) {
        for (int y = 62; y >= 46; --y) {
            if (!passive_spawn_probe_step()) return -9998;
            if (passive_mob_can_spawn_at_category(type, cat, x, y, z)) {
                passive_spawn_y_cache_put(type, cat, x, z, y);
                return y;
            }
        }
        passive_spawn_y_cache_put(type, cat, x, z, -9999);
        return -9999;
    }
    /* Start near the surface first, then fall back to a bounded random sweep.
       This keeps Java-like material checks while avoiding a full 128-level scan
       on every failed spawn column during heavy streaming. */
    for (int y = 80; y >= FLAT_WORLD_Y_MIN + 1; --y) {
        if (!passive_spawn_probe_step()) return -9998;
        if (passive_mob_can_spawn_at_category(type, cat, x, y, z)) {
            passive_spawn_y_cache_put(type, cat, x, z, y);
            return y;
        }
        if (y < 48 && cat == PEX_CAT_CREATURE) break;
    }
    int start_y = FLAT_WORLD_Y_MIN + 1 + (rand() % (FLAT_WORLD_Y_MAX - FLAT_WORLD_Y_MIN - 2));
    for (int i = 0; i < 32; ++i) {
        int y = start_y - i;
        if (y < FLAT_WORLD_Y_MIN + 1) y += (FLAT_WORLD_Y_MAX - FLAT_WORLD_Y_MIN - 2);
        if (!passive_spawn_probe_step()) return -9998;
        if (passive_mob_can_spawn_at_category(type, cat, x, y, z)) {
            passive_spawn_y_cache_put(type, cat, x, z, y);
            return y;
        }
    }
    passive_spawn_y_cache_put(type, cat, x, z, -9999);
    return -9999;
}

static int passive_mob_column_spawn_y(int type, int x, int z) {
    return passive_mob_column_spawn_y_for_category(type, passive_mob_category(type), x, z);
}

static void passive_mobs_try_natural_spawn(void) {
    if (g_mp_connected || g_player_dead) return;
    if ((g_opts.difficulty & 3) == 0) {
        /* EntityMob.onUpdate kills hostile mobs on Peaceful. */
        for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
            PassiveMob *m = &g_passive_mobs[i];
            if (m->active && passive_mob_category(m->type) == PEX_CAT_MONSTER && m->type != PASSIVE_MOB_OCELOT) {
                m->active = 0;
                g_save_dirty = 1;
            }
        }
    }

    /* Java WorldServer calls SpawnerAnimals every tick.  This local port keeps the
       17x17 eligible-chunk math but samples a small randomized subset per tick so
       it remains playable on PexCraft's C renderer/flat-world storage. */
    int pcx = floor_div16((int)floorf(g_player_x));
    int pcz = floor_div16((int)floorf(g_player_z));
    int cats[3] = { PEX_CAT_MONSTER, PEX_CAT_CREATURE, PEX_CAT_WATER };
    int first_ci = g_ingame_ticks % 3;
    for (int ci_step = 0; ci_step < 3; ++ci_step) {
        int ci = (first_ci + ci_step) % 3;
        int cat = cats[ci];
        if (g_passive_spawn_probe_budget == 0) break;
        if (cat == PEX_CAT_MONSTER && (g_opts.difficulty & 3) == 0) continue;
        if (passive_mob_count_category(cat) > passive_spawn_cap_for_category(cat)) continue;
        int chunk_attempts = (cat == PEX_CAT_MONSTER) ? 1 : 1;
        if ((cat == PEX_CAT_CREATURE || cat == PEX_CAT_WATER) && (g_world_time % 400LL) != 0) continue;
        for (int batch = 0; batch < chunk_attempts && passive_mob_count() < passive_mob_target_cap(); ++batch) {
            int cx = pcx + (rand() % 17) - 8;
            int cz = pcz + (rand() % 17) - 8;
            if (cx == pcx - 8 || cx == pcx + 8 || cz == pcz - 8 || cz == pcz + 8) continue; /* edge chunks are eligible=false */
            int base_x = cx * 16 + (rand() & 15);
            int base_z = cz * 16 + (rand() & 15);
            int biome = passive_current_biome_id_at(base_x, base_z);
            PexSpawnEntry entry;
            if (!passive_biome_weighted_spawn_entry(cat, biome, &entry)) continue;
            int spawned_group = 0;
            int max_group = entry.min_group + (rand() % (1 + entry.max_group - entry.min_group));
            for (int group_try = 0; group_try < 3 && spawned_group < max_group; ++group_try) {
                int x = base_x;
                int z = base_z;
                for (int tries = 0; tries < 4 && spawned_group < max_group && passive_mob_count() < passive_mob_target_cap(); ++tries) {
                    x += (rand() % 6) - (rand() % 6);
                    z += (rand() % 6) - (rand() % 6);
                    int y = passive_mob_column_spawn_y_for_category(entry.type, cat, x, z);
                    if (y == -9998) return;
                    if (y == -9999) continue;
                    PassiveMob *m = passive_mob_alloc();
                    if (!m) return;
                    passive_mob_init(m, entry.type, (float)x + 0.5f, (float)y, (float)z + 0.5f);
                    if (entry.type == PASSIVE_MOB_SHEEP) m->fleece_color = passive_sheep_random_fleece_color();
                    if (entry.type == PASSIVE_MOB_OCELOT && (rand() % 7) == 0) {
                        /* SpawnerAnimals creates two kittens around 1/7 of ocelot spawns. */
                        for (int k = 0; k < 2; ++k) {
                            PassiveMob *baby = passive_mob_alloc();
                            if (!baby) break;
                            passive_mob_init(baby, PASSIVE_MOB_OCELOT, m->x + (float)(k ? 0.7f : -0.7f), m->y, m->z);
                            baby->baby_age = -24000;
                        }
                    }
                    ++spawned_group;
                    g_save_dirty = 1;
                    if (spawned_group >= 4) break; /* Java EntityLiving.getMaxSpawnedInChunk default. */
                }
            }
            if (spawned_group > 0) {
                pex_logf("natural spawn cat=%d type=%s count=%d total=%d", cat, passive_mob_name(entry.type), spawned_group, passive_mob_count());
            }
        }
    }
}

static void passive_mobs_ensure_population(int wanted, int attempts) {
    if (g_mp_connected) return;
    int start = passive_mob_count();
    int cap = passive_mob_target_cap();
    if (wanted > cap) wanted = cap;
    for (int attempt = 0; attempt < attempts && passive_mob_count() < wanted; ++attempt) {
        int x = (int)floorf(g_player_x) + (rand() % 129) - 64;
        int z = (int)floorf(g_player_z) + (rand() % 129) - 64;
        int biome = passive_current_biome_id_at(x, z);
        PexSpawnEntry entry;
        if (!passive_biome_weighted_spawn_entry(PEX_CAT_CREATURE, biome, &entry)) continue;
        int type = entry.type;
        int y = passive_mob_column_spawn_y_for_category(type, PEX_CAT_CREATURE, x, z);
        if (y == -9998) break;
        if (y == -9999) continue;
        PassiveMob *m = passive_mob_alloc();
        if (!m) break;
        passive_mob_init(m, type, (float)x + 0.5f, (float)y, (float)z + 0.5f);
        g_save_dirty = 1;
    }
    if (passive_mob_count() != start) {
        pex_logf("passive population ensure start=%d target=%d final=%d attempts=%d", start, wanted, passive_mob_count(), attempts);
    }
}

static void passive_mobs_spawn_initial(void) {
    passive_mobs_reset();
    /* 1800 synchronous column scans caused a visible stall on world entry.
       Seed a smaller population immediately; natural spawning fills toward the
       1.2.5 category caps once streaming has settled. */
    passive_mobs_ensure_population(PEX_PASSIVE_INITIAL_TARGET, 256);
}


static int passive_mob_count_type_near(int type, float x, float z, float radius) {
    int n = 0; float r2 = radius * radius;
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        PassiveMob *m = &g_passive_mobs[i];
        if (!m->active || m->death_time > 0 || m->type != type) continue;
        float dx = m->x - x, dz = m->z - z;
        if (dx*dx + dz*dz <= r2) ++n;
    }
    return n;
}

static int passive_mob_is_breedable_type(int type) {
    switch (type) {
        case PASSIVE_MOB_PIG:
        case PASSIVE_MOB_SHEEP:
        case PASSIVE_MOB_COW:
        case PASSIVE_MOB_CHICKEN:
        case PASSIVE_MOB_MOOSHROOM:
        case PASSIVE_MOB_WOLF:
        case PASSIVE_MOB_OCELOT:
            return 1;
        default:
            return 0;
    }
}

static int passive_mob_breeding_item_for_type(int type, int item_id) {
    if (type == PASSIVE_MOB_WOLF) {
        return item_id == ITEM_PORK_RAW || item_id == ITEM_PORK_COOKED ||
               item_id == ITEM_BEEF_RAW || item_id == ITEM_BEEF_COOKED ||
               item_id == ITEM_CHICKEN_RAW || item_id == ITEM_CHICKEN_COOKED ||
               item_id == ITEM_ROTTEN_FLESH;
    }
    if (type == PASSIVE_MOB_OCELOT) return item_id == ITEM_FISH_RAW;
    return item_id == ITEM_WHEAT;
}

static int passive_mob_same_breeding_species(int a, int b) {
    if (a == b) return 1;
    return (a == PASSIVE_MOB_COW && b == PASSIVE_MOB_MOOSHROOM) ||
           (a == PASSIVE_MOB_MOOSHROOM && b == PASSIVE_MOB_COW);
}

static int passive_mob_find_love_mate(PassiveMob *self, float radius) {
    if (!self || self->baby_age != 0 || self->love_time <= 0) return -1;
    float best2 = radius * radius;
    int best = -1;
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        PassiveMob *m = &g_passive_mobs[i];
        if (m == self || !m->active || m->death_time > 0 || m->baby_age != 0 || m->love_time <= 0) continue;
        if (!passive_mob_same_breeding_species(self->type, m->type)) continue;
        if (self->type == PASSIVE_MOB_WOLF && (!self->sheared || !m->sheared)) continue;
        if (self->type == PASSIVE_MOB_OCELOT && (!self->sheared || !m->sheared)) continue;
        float dx = m->x - self->x, dz = m->z - self->z;
        float d2 = dx * dx + dz * dz;
        if (d2 < best2) { best2 = d2; best = i; }
    }
    return best;
}

static void passive_mob_spawn_baby_from_parents(PassiveMob *a, PassiveMob *b) {
    if (!a || !b) return;
    PassiveMob *baby = passive_mob_alloc();
    if (!baby) return;
    int child_type = a->type;
    if (a->type == PASSIVE_MOB_MOOSHROOM || b->type == PASSIVE_MOB_MOOSHROOM) child_type = PASSIVE_MOB_MOOSHROOM;
    passive_mob_init(baby, child_type, a->x, a->y, a->z);
    baby->baby_age = -24000;
    baby->love_time = 0;
    if (child_type == PASSIVE_MOB_SHEEP) {
        baby->fleece_color = (rand() & 1) ? (a->fleece_color & 15) : (b->fleece_color & 15);
    } else if (child_type == PASSIVE_MOB_WOLF || child_type == PASSIVE_MOB_OCELOT) {
        baby->sheared = 1; /* tamed child, matching EntityTameable child inheritance. */
        baby->owner_id = (a->owner_id == PEX_MOB_OWNER_SINGLEPLAYER || b->owner_id == PEX_MOB_OWNER_SINGLEPLAYER) ? PEX_MOB_OWNER_SINGLEPLAYER : 0;
        baby->tame_state = (child_type == PASSIVE_MOB_OCELOT) ? (a->fleece_color ? a->fleece_color : b->fleece_color) : 1;
        baby->fleece_color = a->fleece_color ? a->fleece_color : b->fleece_color;
        baby->sitting = 0;
        if (child_type == PASSIVE_MOB_WOLF) {
            baby->health = baby->prev_health = passive_mob_max_health_current_125(baby);
            baby->collar_color = (a->collar_color >= 0) ? a->collar_color : b->collar_color;
        }
    }
    a->baby_age = 6000;
    b->baby_age = 6000;
    a->love_time = b->love_time = 0;
    for (int i = 0; i < 7; ++i) add_splash_particle(a->x, a->y + 0.8f, a->z,
                                                    (pex_rand_float01() - 0.5f) * 0.06f,
                                                    pex_rand_float01() * 0.08f,
                                                    (pex_rand_float01() - 0.5f) * 0.06f);
    g_save_dirty = 1;
}

static void passive_mob_try_finish_breeding(PassiveMob *m) {
    if (!m || !passive_mob_is_breedable_type(m->type)) return;
    int mate_idx = passive_mob_find_love_mate(m, 8.0f);
    if (mate_idx >= 0) passive_mob_spawn_baby_from_parents(m, &g_passive_mobs[mate_idx]);
}

static int passive_mob_feed_for_love(PassiveMob *m, ItemStack *held) {
    if (!m || !held || stack_empty(held) || !passive_mob_is_breedable_type(m->type)) return 0;
    if (!passive_mob_breeding_item_for_type(m->type, held->id)) return 0;
    if (m->baby_age != 0) return 0;
    if (m->type == PASSIVE_MOB_WOLF && !m->sheared) return 0;
    if (m->type == PASSIVE_MOB_OCELOT && !m->sheared) return 0;
    m->love_time = 600;
    consume_held_stack_one(held, 0);
    restart_hand_swing();
    passive_mob_try_finish_breeding(m);
    g_save_dirty = 1;
    return 1;
}

static int passive_mob_find_nearest_type(PassiveMob *self, int type, float range) {
    int best = -1;
    float best2 = range * range;
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        PassiveMob *m = &g_passive_mobs[i];
        if (m == self || !m->active || m->death_time > 0 || m->type != type) continue;
        float dx = m->x - self->x, dz = m->z - self->z;
        float d2 = dx * dx + dz * dz;
        if (d2 < best2) { best2 = d2; best = i; }
    }
    return best;
}

static int passive_enderman_can_carry_block(int id) {
    switch (id) {
        case BLOCK_GRASS: case BLOCK_DIRT: case BLOCK_SAND: case BLOCK_GRAVEL:
        case BLOCK_YELLOW_FLOWER: case BLOCK_RED_ROSE: case BLOCK_BROWN_MUSHROOM:
        case BLOCK_RED_MUSHROOM: case BLOCK_TNT: case BLOCK_CACTUS: case BLOCK_CLAY:
        case BLOCK_PUMPKIN: case BLOCK_MELON: case BLOCK_MYCELIUM:
            return 1;
        default:
            return 0;
    }
}

static void passive_enderman_tick_carry_block(PassiveMob *m) {
    if (!m || m->type != PASSIVE_MOB_ENDERMAN || m->death_time > 0) return;
    if (m->held_block <= 0) {
        if ((rand() % 20) != 0) return;
        for (int tries = 0; tries < 24; ++tries) {
            int x = (int)floorf(m->x) + (rand() % 5) - 2;
            int y = (int)floorf(m->y) + (rand() % 4) - 1;
            int z = (int)floorf(m->z) + (rand() % 5) - 2;
            int id = flat_get_block(x, y, z);
            if (!passive_enderman_can_carry_block(id)) continue;
            if (!flat_chunk_generated_at_block(x, z)) continue;
            m->held_block = id;
            flat_set_block(x, y, z, 0);
            g_save_dirty = 1;
            return;
        }
    } else {
        if ((rand() % 200) != 0) return;
        for (int tries = 0; tries < 24; ++tries) {
            int x = (int)floorf(m->x) + (rand() % 5) - 2;
            int y = (int)floorf(m->y) + (rand() % 4) - 1;
            int z = (int)floorf(m->z) + (rand() % 5) - 2;
            int below = flat_get_block(x, y - 1, z);
            if (!flat_chunk_generated_at_block(x, z)) continue;
            if (flat_get_block(x, y, z) != 0) continue;
            if (!flat_block_is_solid(below) || passive_block_is_liquid(below)) continue;
            flat_set_block(x, y, z, m->held_block);
            m->held_block = 0;
            g_save_dirty = 1;
            return;
        }
    }
}

static int passive_biome_is_snow_layer_ok(int biome) {
    return biome == BIOME_TAIGA || biome == BIOME_TAIGA_HILLS || biome == BIOME_ICE_PLAINS ||
           biome == BIOME_ICE_MOUNTAINS || biome == BIOME_FROZEN_RIVER || biome == BIOME_FROZEN_OCEAN;
}

static void passive_snowman_tick_snow_trail(PassiveMob *m) {
    if (!m || m->type != PASSIVE_MOB_SNOWMAN || m->death_time > 0) return;
    int x0 = (int)floorf(m->x);
    int y = (int)floorf(m->y);
    int z0 = (int)floorf(m->z);
    int biome = passive_current_biome_id_at(x0, z0);
    if (!passive_biome_is_snow_layer_ok(biome)) return;
    for (int ox = -1; ox <= 1; ++ox) {
        for (int oz = -1; oz <= 1; ++oz) {
            int x = x0 + ox, z = z0 + oz;
            if ((float)(ox * ox + oz * oz) > 2.5f) continue;
            if (flat_get_block(x, y, z) != 0) continue;
            if (flat_block_is_solid(flat_get_block(x, y - 1, z))) {
                flat_set_block(x, y, z, BLOCK_SNOW_LAYER);
                g_save_dirty = 1;
            }
        }
    }
}

static int passive_find_surface_for_village_spawn(int x, int z) {
    int cached_y = -9999;
    if (passive_spawn_y_cache_get(PASSIVE_MOB_VILLAGER, PEX_CAT_UTILITY, x, z, &cached_y)) return cached_y;
    if (!flat_chunk_generated_at_block(x, z)) {
        passive_spawn_y_cache_put(PASSIVE_MOB_VILLAGER, PEX_CAT_UTILITY, x, z, -9999);
        return -9999;
    }
    for (int y = 96; y >= FLAT_WORLD_Y_MIN + 1; --y) {
        if (passive_mob_air_spawn_base_ok(x, y, z) && passive_mob_has_spawn_clearance(PASSIVE_MOB_VILLAGER, (float)x+0.5f, (float)y, (float)z+0.5f)) {
            passive_spawn_y_cache_put(PASSIVE_MOB_VILLAGER, PEX_CAT_UTILITY, x, z, y);
            return y;
        }
        if (y < 48) break;
    }
    passive_spawn_y_cache_put(PASSIVE_MOB_VILLAGER, PEX_CAT_UTILITY, x, z, -9999);
    return -9999;
}

static void passive_mobs_try_village_spawns(void) {
    if (g_mp_connected || (g_ingame_ticks % 200) != 0) return;
    int pcx = floor_div16((int)floorf(g_player_x));
    int pcz = floor_div16((int)floorf(g_player_z));
    for (int vx = pcx - 8; vx <= pcx + 8; ++vx) {
        for (int vz = pcz - 8; vz <= pcz + 8; ++vz) {
            if (!worldgen_village_can_spawn((long long)g_world_seed, vx, vz)) continue;
            float cx = (float)(vx * 16 + 8);
            float cz = (float)(vz * 16 + 8);
            if ((cx - g_player_x)*(cx - g_player_x) + (cz - g_player_z)*(cz - g_player_z) > 96.0f*96.0f) continue;
            int villagers = passive_mob_count_type_near(PASSIVE_MOB_VILLAGER, cx, cz, 40.0f);
            int desired = 5; /* simplified village pieces in PexCraft contain house/church/blacksmith doors. */
            static const int offs[][2] = {{8,-4},{-15,6},{12,10},{-24,-12},{4,4},{-4,8}};
            for (int k = 0; villagers < desired && k < (int)(sizeof(offs)/sizeof(offs[0])); ++k) {
                int x = (int)cx + offs[k][0] + (rand()%3)-1;
                int z = (int)cz + offs[k][1] + (rand()%3)-1;
                int y = passive_find_surface_for_village_spawn(x, z);
                if (y == -9999) continue;
                PassiveMob *m = passive_mob_alloc();
                if (!m) return;
                passive_mob_init(m, PASSIVE_MOB_VILLAGER, (float)x+0.5f, (float)y, (float)z+0.5f);
                villagers++;
                g_save_dirty = 1;
            }
            if (villagers >= 5 && passive_mob_count_type_near(PASSIVE_MOB_IRON_GOLEM, cx, cz, 40.0f) <= 0 && (rand()%20)==0) {
                int x = (int)cx + (rand()%9)-4, z = (int)cz + (rand()%9)-4;
                int y = passive_find_surface_for_village_spawn(x, z);
                if (y != -9999) {
                    PassiveMob *g = passive_mob_alloc();
                    if (g) { passive_mob_init(g, PASSIVE_MOB_IRON_GOLEM, (float)x+0.5f, (float)y, (float)z+0.5f); g_save_dirty = 1; }
                }
            }
        }
    }
}

static void passive_mob_choose_target(PassiveMob *m) {
    if (!m || m->death_time > 0) return;
    int base_x = (int)floorf(m->x);
    int base_y = (int)floorf(m->y);
    int base_z = (int)floorf(m->z);
    int best_x = 0, best_y = 0, best_z = 0;
    float best_weight = -99999.0f;
    for (int i = 0; i < 10; ++i) {
        int x = base_x + (rand() % 20) - 10;
        int y = base_y + (rand() % 14) - 7;
        int z = base_z + (rand() % 20) - 10;
        if (y <= FLAT_WORLD_Y_MIN || y + 1 > FLAT_WORLD_Y_MAX) continue;
        if (!passive_path_can_stand_at(m->type, x, y, z)) continue;
        float dx = ((float)x + 0.5f) - m->x;
        float dz = ((float)z + 0.5f) - m->z;
        float dist2 = dx * dx + dz * dz;
        if (dist2 < 1.0f) continue;
        float w = passive_mob_path_weight_125(m, x, y, z);
        if (w > best_weight) { best_weight = w; best_x = x; best_y = y; best_z = z; }
    }
    if (best_weight > -9999.0f) {
        passive_mob_set_path_goal(m, (float)best_x + 0.5f, (float)best_y, (float)best_z + 0.5f);
        m->wander_cooldown = 0;
    } else {
        m->has_path_target = 0;
        m->wander_cooldown = 0;
    }
}

static int held_damage_vs_mob(void) {
    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    int damage = 1;
    if (!stack_empty(held)) {
        switch (held->id) {
            case ITEM_WOODEN_SWORD: damage = 4; break;
            case ITEM_STONE_SWORD: damage = 5; break;
            case ITEM_IRON_SWORD: damage = 6; break;
            case ITEM_DIAMOND_SWORD: damage = 7; break;
            case ITEM_GOLD_SWORD: damage = 4; break;
            case ITEM_WOODEN_AXE: damage = 3; break;
            case ITEM_STONE_AXE: damage = 4; break;
            case ITEM_IRON_AXE: damage = 5; break;
            case ITEM_DIAMOND_AXE: damage = 6; break;
            case ITEM_GOLD_AXE: damage = 3; break;
            default: damage = 1; break;
        }
    }
    if (player_has_potion(PEX_POTION_STRENGTH)) damage += 3 * (player_potion_amplifier(PEX_POTION_STRENGTH) + 1);
    if (player_has_potion(PEX_POTION_WEAKNESS)) damage -= 2 * (player_potion_amplifier(PEX_POTION_WEAKNESS) + 1);
    return damage > 0 ? damage : 0;
}


static int passive_mob_can_shear_sheep(const PassiveMob *m) {
    if (!m || !m->active || m->death_time > 0) return 0;
    if (m->type != PASSIVE_MOB_SHEEP || m->sheared || m->baby_age < 0) return 0;
#ifdef ITEM_SHEARS
    const ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    return held && !stack_empty(held) && held->id == ITEM_SHEARS;
#else
    /* TODO(passive mobs): enable sheep shearing when this Beta item table gains shears. */
    return 0;
#endif
}

static void passive_mob_shear_sheep(PassiveMob *m) {
    if (!passive_mob_can_shear_sheep(m)) return;
    m->sheared = 1;
    int count = 1 + (rand() % 3);
    for (int i = 0; i < count; ++i) {
        spawn_item_stack(m->x, m->y + 1.0f, m->z, make_stack(BLOCK_WOOL, 1, m->fleece_color & 15), 1);
    }
    damage_held_item(&g_inventory[g_selected_hotbar_slot], 1);
    restart_hand_swing();
    g_save_dirty = 1;
}

static int passive_mobs_spawn_from_egg_damage(int egg_damage, float x, float y, float z) {
    int type = passive_mob_type_from_egg(egg_damage);
    PassiveMob *m;
    if (g_mp_connected || g_player_dead) return 0;
    if (!passive_mob_type_valid(type)) return 0;
    m = passive_mob_alloc();
    if (!m) return 0;
    passive_mob_init(m, type, x, y, z);
    if (type == PASSIVE_MOB_SHEEP) m->fleece_color = passive_sheep_random_fleece_color();
    if (passive_mob_is_slime_family(type)) {
        /* EntitySlime spawn eggs also call setSlimeSize(1 << rand.nextInt(3)). */
        int sz = 1 << (rand() % 3);
        m->fleece_color = sz;
        m->width = 0.6f * (float)sz;
        m->height = 0.6f * (float)sz;
        m->health = sz * sz;
        m->prev_health = m->health;
    }
    if (type == PASSIVE_MOB_WOLF || type == PASSIVE_MOB_PIG_ZOMBIE) m->rideable = 0;
    m->yaw = m->prev_yaw = pex_rand_float01() * 360.0f;
    m->render_yaw = m->prev_render_yaw = m->yaw;
    const char *s = passive_mob_living_sound(type);
    if (s) pex_sound_play_at(s, x, y, z, passive_mob_sound_volume(type), 1.0f);
    passive_mobs_enforce_cap();
    g_save_dirty = 1;
    return 1;
}

static int passive_mobs_apply_dye_to_target(int dye_damage) {
    float mob_t = 0.0f;
    PassiveMob *m;
    int wool_meta;
    if (g_mp_connected || g_player_dead) return 0;
    if ((dye_damage & 15) == 15) return 0; /* Bone meal uses block code, not sheep dyeing. */
    m = passive_mob_raycast(5.0f, &mob_t);
    if (!m || m->type != PASSIVE_MOB_SHEEP || m->sheared) return 0;
    wool_meta = dye_to_wool_meta(dye_damage & 15);
    if ((m->fleece_color & 15) == wool_meta) return 0;
    m->fleece_color = wool_meta;
    pex_sound_play_at("mob.sheep", m->x, m->y, m->z, 1.0f, 1.0f);
    g_save_dirty = 1;
    return 1;
}

static void passive_mob_apply_player_knockback(PassiveMob *m) {
    float dx = g_player_x - m->x;
    float dz = g_player_z - m->z;
    float len = sqrtf(dx * dx + dz * dz);
    if (len < 0.001f) len = 0.001f;
    m->mx -= dx / len * 0.4f;
    m->mz -= dz / len * 0.4f;
    m->my += 0.4f;
    if (m->my > 0.4f) m->my = 0.4f;
}

static void passive_mob_apply_knockback(PassiveMob *m, const PexDamageSource *source, int damage) {
    if (!m || !source) return;
    if (source->true_kind == PEX_DAMAGE_ENTITY_PLAYER) {
        passive_mob_apply_player_knockback(m);
        return;
    }
    float src_x = source->true_x;
    float src_z = source->true_z;
    if (source->direct_kind == PEX_DAMAGE_ENTITY_PROJECTILE) {
        src_x = source->direct_x;
        src_z = source->direct_z;
    }
    float dx = src_x - m->x;
    float dz = src_z - m->z;
    float len = sqrtf(dx * dx + dz * dz);
    if (len < 0.001f) {
        dx = pex_rand_float01() - pex_rand_float01();
        dz = pex_rand_float01() - pex_rand_float01();
        len = sqrtf(dx * dx + dz * dz);
        if (len < 0.001f) len = 0.001f;
    }
    (void)damage;
    m->mx /= 2.0f;
    m->my /= 2.0f;
    m->mz /= 2.0f;
    m->mx -= dx / len * 0.4f;
    m->mz -= dz / len * 0.4f;
    m->my += 0.4f;
    if (m->my > 0.4f) m->my = 0.4f;
}

static int passive_mob_is_entity_living_source(const PexDamageSource *source) {
    return source && (source->true_kind == PEX_DAMAGE_ENTITY_PLAYER || source->true_kind == PEX_DAMAGE_ENTITY_MOB);
}

static int passive_mob_damage_has_player_attacker(const PexDamageSource *source) {
    return source && source->true_kind == PEX_DAMAGE_ENTITY_PLAYER;
}

static int passive_mob_apply_armor_calculations(PassiveMob *m, const PexDamageSource *source, int damage) {
    if (!m || !source || damage <= 0) return 0;
    if (!source->unblockable) {
        int armor = 0;
        int scaled = damage * (25 - armor) + m->damage_remainder;
        damage = scaled / 25;
        m->damage_remainder = scaled % 25;
    }
    return damage;
}

static int passive_mob_apply_potion_damage_calculations(PassiveMob *m, int damage) {
    if (!m || damage <= 0) return 0;
    if (pex_passive_has_potion(m, PEX_POTION_RESISTANCE)) {
        int amp = pex_passive_potion_amplifier(m, PEX_POTION_RESISTANCE);
        int reduce = (amp + 1) * 5;
        if (reduce > 20) reduce = 20;
        int scaled = damage * (25 - reduce) + m->damage_remainder;
        damage = scaled / 25;
        m->damage_remainder = scaled % 25;
    }
    return damage;
}

static int passive_mob_damage_entity(PassiveMob *m, const PexDamageSource *source, int damage) {
    damage = passive_mob_apply_armor_calculations(m, source, damage);
    damage = passive_mob_apply_potion_damage_calculations(m, damage);
    m->health -= damage;
    return damage;
}

static void passive_pig_zombie_become_angry_125(PassiveMob *m, int play_sound) {
    if (!m || m->type != PASSIVE_MOB_PIG_ZOMBIE || m->death_time > 0) return;
    m->rideable = 1;
    m->egg_timer = 400 + (rand() % 400); /* EntityPigZombie.angerLevel */
    if (play_sound) {
        pex_sound_play_at("mob.zombiepig.zpigangry", m->x, m->y, m->z,
                          passive_mob_sound_volume(m->type) * 2.0f,
                          ((pex_rand_float01() - pex_rand_float01()) * 0.2f + 1.0f) * 1.8f);
    }
}

static void passive_pig_zombie_alert_nearby_125(PassiveMob *self) {
    if (!self || self->type != PASSIVE_MOB_PIG_ZOMBIE) return;
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        PassiveMob *m = &g_passive_mobs[i];
        if (!m->active || m->type != PASSIVE_MOB_PIG_ZOMBIE || m->death_time > 0) continue;
        float dx = m->x - self->x, dy = m->y - self->y, dz = m->z - self->z;
        if (dx * dx + dy * dy + dz * dz > 32.0f * 32.0f) continue;
        passive_pig_zombie_become_angry_125(m, m == self);
    }
}

static void passive_mob_on_attacked_by_source(PassiveMob *m, const PexDamageSource *source) {
    if (!m || !source) return;
    if (passive_mob_is_breedable_type(m->type)) {
        m->love_time = 0;
        if (m->type != PASSIVE_MOB_WOLF && m->type != PASSIVE_MOB_OCELOT) {
            m->wander_cooldown = 0;
            m->has_path_target = 0;
        }
    }
    if (passive_mob_is_entity_living_source(source) || source->direct_kind == PEX_DAMAGE_ENTITY_PROJECTILE) {
        if (m->type == PASSIVE_MOB_SILVERFISH && m->tame_state <= 0) {
            /* EntitySilverfish.attackEntityFrom: entity damage arms the 20 tick ally summon cooldown. */
            m->tame_state = 20;
        }
        if (m->type == PASSIVE_MOB_PIG_ZOMBIE) {
            if (passive_mob_damage_has_player_attacker(source)) passive_pig_zombie_alert_nearby_125(m);
            else passive_pig_zombie_become_angry_125(m, 0);
        }
        if (m->type == PASSIVE_MOB_IRON_GOLEM || m->type == PASSIVE_MOB_ENDERMAN) m->rideable = 1;
        if (m->type == PASSIVE_MOB_WOLF) {
            m->sitting = 0;
            if (!passive_mob_is_owned_by_player(m) || !passive_mob_damage_has_player_attacker(source)) {
                m->rideable = 1;
                if (!passive_mob_is_owned_by_player(m)) m->tame_state = 2;
            }
        }
    }
    if (passive_mob_damage_has_player_attacker(source) &&
        (m->type == PASSIVE_MOB_VILLAGER || m->type == PASSIVE_MOB_IRON_GOLEM)) {
        passive_village_update_reputation_from_attack(m);
    }
}

static void passive_mob_start_death(PassiveMob *m, const PexDamageSource *source) {
    if (!m || m->death_time > 0) return;
    const char *s = passive_mob_death_sound(m->type);
    if (s) pex_sound_play_at(s, m->x, m->y, m->z, passive_mob_sound_volume(m->type),
                             (pex_rand_float01() - pex_rand_float01()) * 0.2f + 1.0f);
    m->health = 0;
    m->death_time = 1;
    m->has_path_target = 0;
    m->wander_cooldown = 0;
    if (g_player_riding_passive_mob >= 0 && g_player_riding_passive_mob < MAX_PASSIVE_MOBS &&
        &g_passive_mobs[g_player_riding_passive_mob] == m) {
        g_player_riding_passive_mob = -1;
    }
    if (passive_mob_is_slime_family(m->type) && m->fleece_color > 1) {
        int old_size = m->fleece_color;
        int new_size = old_size / 2;
        int count = 2 + (rand() % 3);
        for (int i = 0; i < count; ++i) {
            PassiveMob *child = passive_mob_alloc();
            if (!child) break;
            float ox = ((float)(i % 2) - 0.5f) * (float)new_size * 0.5f;
            float oz = ((float)(i / 2) - 0.5f) * (float)new_size * 0.5f;
            passive_mob_init(child, m->type, m->x + ox, m->y + 0.1f, m->z + oz);
            child->fleece_color = new_size;
            child->width = 0.6f * (float)new_size;
            child->height = 0.6f * (float)new_size;
            child->health = new_size * new_size;
        }
    }
    if (m->type != PASSIVE_MOB_ENDER_DRAGON && passive_mob_recently_hit_by_player_125(m, source)) player_add_experience(1 + (rand() % 3));
    passive_mob_drop_on_death(m, source);
}

static int pex_mob_attack_entity_from(PassiveMob *m, PexDamageSource source, int damage) {
    if (!m || !m->active || m->death_time > 0 || damage <= 0) return 0;
    if (m->type == PASSIVE_MOB_ENDERMAN && source.projectile) {
        /* EntityEnderman.attackEntityFrom: arrows/projectiles do not damage it; it tries to teleport instead. */
        (void)passive_mob_teleport_random_125(m, 64.0f);
        return 0;
    }
    if (source.fire_damage && pex_passive_has_potion(m, PEX_POTION_FIRE_RESISTANCE)) return 0;
    m->age = 0;
    int incoming = damage;
    int play_hurt = 1;
    if (m->damage_cooldown > PEX_HEARTS_HALVES_LIFE / 2) {
        if (incoming <= m->prev_health) return 0;
        incoming -= m->prev_health;
        m->prev_health = damage;
        play_hurt = 0;
    } else {
        m->prev_health = damage;
        m->damage_cooldown = PEX_HEARTS_HALVES_LIFE;
        m->hurt_time = 10;
    }
    if (passive_mob_damage_has_player_attacker(&source)) {
        m->recently_hit = 60;
        m->last_looting_level = 0;
        if (g_selected_hotbar_slot >= 0 && g_selected_hotbar_slot < 36) {
            m->last_looting_level = item_stack_enchant_level_125(&g_inventory[g_selected_hotbar_slot], PEX_ENCHANT_LOOTING);
            if (m->last_looting_level < 0) m->last_looting_level = 0;
            if (m->last_looting_level > 3) m->last_looting_level = 3;
        }
    }
    int applied = passive_mob_damage_entity(m, &source, incoming);
    if (applied <= 0 && incoming > 0) {
        if (play_hurt) m->hurt_time = 10;
        g_save_dirty = 1;
        return 1;
    }
    passive_mob_on_attacked_by_source(m, &source);
    if (play_hurt) {
        passive_mob_apply_knockback(m, &source, applied);
        if (m->health <= 0) {
            /* passive_mob_start_death owns the death sound and drop transition. */
        } else {
            const char *s = passive_mob_hurt_sound(m->type);
            if (s) pex_sound_play_at(s, m->x, m->y, m->z, passive_mob_sound_volume(m->type),
                                     (pex_rand_float01() - pex_rand_float01()) * 0.2f + 1.0f);
        }
    }
    if (passive_mob_damage_has_player_attacker(&source)) player_add_exhaustion(source.hunger_damage);
    if (m->health <= 0) passive_mob_start_death(m, &source);
    g_save_dirty = 1;
    return 1;
}

static int passive_explosion_breaks_block(int id) {
    if (id <= 0 || id == BLOCK_BEDROCK || id == BLOCK_END_PORTAL || id == BLOCK_END_PORTAL_FRAME) return 0;
    if (block_is_liquid(id)) return 0;
    return 1;
}

static void passive_creeper_explode(PassiveMob *self) {
    if (!self) return;
    float radius = 3.0f;
    int cx = (int)floorf(self->x), cy = (int)floorf(self->y + 0.5f), cz = (int)floorf(self->z);
    for (int y = cy - 3; y <= cy + 3; ++y) {
        for (int z = cz - 3; z <= cz + 3; ++z) {
            for (int x = cx - 3; x <= cx + 3; ++x) {
                float dx = ((float)x + 0.5f) - self->x;
                float dy = ((float)y + 0.5f) - (self->y + 0.5f);
                float dz = ((float)z + 0.5f) - self->z;
                float d = sqrtf(dx * dx + dy * dy + dz * dz);
                if (d > radius) continue;
                int id = flat_get_block(x, y, z);
                if (!passive_explosion_breaks_block(id)) continue;
                if (pex_rand_float01() > 1.0f - d / (radius + 0.001f)) {
                    spawn_block_destroy_particles(x, y, z, id);
                    flat_set_block(x, y, z, 0);
                }
            }
        }
    }
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        PassiveMob *m = &g_passive_mobs[i];
        if (!m->active || m->death_time > 0 || m == self) continue;
        float dx = m->x - self->x;
        float dy = (m->y + m->height * 0.5f) - (self->y + self->height * 0.5f);
        float dz = m->z - self->z;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);
        if (dist > radius * 2.0f) continue;
        int dmg = (int)((1.0f - dist / (radius * 2.0f)) * 49.0f + 1.0f);
        PexDamageSource source = pex_damage_source_simple(PEX_DAMAGE_EXPLOSION);
        pex_damage_source_set_true_mob(&source, self);
        source.direct_kind = PEX_DAMAGE_ENTITY_MOB;
        source.direct_mob_index = (int)(self - g_passive_mobs);
        source.direct_mob_type = self->type;
        source.direct_x = self->x;
        source.direct_y = self->y;
        source.direct_z = self->z;
        (void)pex_mob_attack_entity_from(m, source, dmg);
    }
    for (int i = 0; i < 32; ++i) add_splash_particle(self->x, self->y + 0.6f, self->z,
                                                     (pex_rand_float01() - 0.5f) * 0.5f,
                                                     pex_rand_float01() * 0.35f,
                                                     (pex_rand_float01() - 0.5f) * 0.5f);
    g_save_dirty = 1;
}

static int passive_mob_find_nearest_target_mob(PassiveMob *self, int want_villager, int want_hostile, float range) {
    int best = -1;
    float best2 = range * range;
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        PassiveMob *m = &g_passive_mobs[i];
        if (m == self || !m->active || m->death_time > 0) continue;
        if (want_villager && m->type != PASSIVE_MOB_VILLAGER) continue;
        if (want_hostile && !passive_mob_is_hostile_target_type(m->type)) continue;
        if (!want_villager && !want_hostile) continue;
        float dx = m->x - self->x, dy = m->y - self->y, dz = m->z - self->z;
        float d2 = dx*dx + dy*dy + dz*dz;
        if (d2 < best2) { best2 = d2; best = i; }
    }
    return best;
}

static int passive_ray_intersect_aabb(const FlatAABB *box, float ox, float oy, float oz, float dx, float dy, float dz, float max_t, float *out_t) {
    float tmin = 0.0f, tmax = max_t;
#define PASSIVE_RAY_AXIS(o,d,minv,maxv) do { \
    if (fabsf(d) < 1e-6f) { if ((o) < (minv) || (o) > (maxv)) return 0; } \
    else { float inv = 1.0f / (d); float t1 = ((minv) - (o)) * inv; float t2 = ((maxv) - (o)) * inv; \
           if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; } \
           if (t1 > tmin) tmin = t1; if (t2 < tmax) tmax = t2; if (tmin > tmax) return 0; } \
} while (0)
    PASSIVE_RAY_AXIS(ox, dx, box->minx, box->maxx);
    PASSIVE_RAY_AXIS(oy, dy, box->miny, box->maxy);
    PASSIVE_RAY_AXIS(oz, dz, box->minz, box->maxz);
#undef PASSIVE_RAY_AXIS
    if (out_t) *out_t = tmin;
    return tmax >= 0.0f && tmin <= max_t;
}

static PassiveMob *passive_mob_raycast(float max_dist, float *out_t) {
    float dx, dy, dz;
    pex_touch_aware_look_vector(&dx, &dy, &dz);
    float ox = g_player_x, oy = g_player_y, oz = g_player_z;
    float best_t = max_dist;
    PassiveMob *best = NULL;
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        PassiveMob *m = &g_passive_mobs[i];
        if (!m->active || m->death_time > 0) continue;
        FlatAABB b;
        passive_mob_aabb(m, &b);
        b.minx -= 0.10f; b.maxx += 0.10f;
        b.miny -= 0.10f; b.maxy += 0.10f;
        b.minz -= 0.10f; b.maxz += 0.10f;
        float t = 0.0f;
        if (!passive_ray_intersect_aabb(&b, ox, oy, oz, dx, dy, dz, max_dist, &t)) continue;
        if (t >= 0.0f && t < best_t) { best_t = t; best = m; }
    }
    if (best && out_t) *out_t = best_t;
    return best;
}


static int passive_owned_wolf_can_target_125(const PassiveMob *wolf, const PassiveMob *target) {
    if (!wolf || !target || wolf == target) return 0;
    if (!wolf->active || !target->active || wolf->death_time > 0 || target->death_time > 0) return 0;
    if (wolf->type != PASSIVE_MOB_WOLF || !passive_mob_is_owned_by_player(wolf) || wolf->sitting) return 0;
    if (target->type == PASSIVE_MOB_WOLF && passive_mob_is_owned_by_player(target)) return 0;
    if (target->type == PASSIVE_MOB_OCELOT && passive_mob_is_owned_by_player(target)) return 0;
    return 1;
}

static void passive_owned_wolves_target_mob_125(PassiveMob *target) {
    if (!target || !target->active || target->death_time > 0) return;
    int target_index = (int)(target - g_passive_mobs);
    if (target_index < 0 || target_index >= MAX_PASSIVE_MOBS) return;
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        PassiveMob *wolf = &g_passive_mobs[i];
        if (!passive_owned_wolf_can_target_125(wolf, target)) continue;
        float dx = wolf->x - g_player_x;
        float dy = wolf->y - (g_player_y - 1.62f);
        float dz = wolf->z - g_player_z;
        if (dx*dx + dy*dy + dz*dz > 32.0f * 32.0f) continue;
        wolf->target_mob_index = target_index;
        wolf->rideable = 1;
        wolf->ai_repath_delay = 0;
        passive_mob_set_path_goal(wolf, target->x, target->y, target->z);
        wolf->path_recalc_cooldown = 0;
    }
}

static void passive_owned_wolves_defend_player_125(PassiveMob *attacker) {
    passive_owned_wolves_target_mob_125(attacker);
}

static int passive_mobs_attack_from_player(void) {
    if (g_mp_connected || g_player_dead) return 0;
    float mob_t = 0.0f;
    PassiveMob *m = passive_mob_raycast(5.0f, &mob_t);
    if (passive_dragon_crystal_player_attack_125(5.0f, m ? mob_t : 5.0f)) return 1;
    if (!m) return 0;
    FlatRayHit block = flat_raycast();
    if (block.hit) {
        float bx = block.hx - g_player_x;
        float by = block.hy - g_player_y;
        float bz = block.hz - g_player_z;
        float block_dist = sqrtf(bx * bx + by * by + bz * bz);
        if (block_dist + 0.20f < mob_t) return 0;
    }
    restart_hand_swing();
    if (m->type == PASSIVE_MOB_WOLF && !passive_mob_is_owned_by_player(m)) { m->rideable = 1; m->tame_state = 2; }
    (void)pex_mob_attack_entity_from(m, pex_damage_source_player(), m->type == PASSIVE_MOB_ENDER_DRAGON ? passive_dragon_player_part_damage_125(m, held_damage_vs_mob()) : held_damage_vs_mob());
    if (m->active && m->death_time <= 0) passive_owned_wolves_target_mob_125(m);
    return 1;
}

static void passive_mobs_dismount_player(void) {
    g_player_riding_passive_mob = -1;
    g_player_motion_y = 0.12f;
    hud_add_chat("Dismounted.");
}

static int passive_mobs_player_interact(void) {
    if (g_mp_connected || g_player_dead) return 0;
    if (g_player_riding_passive_mob >= 0) {
        passive_mobs_dismount_player();
        restart_hand_swing();
        return 1;
    }
    float mob_t = 0.0f;
    PassiveMob *m = passive_mob_raycast(5.0f, &mob_t);
    if (!m) return 0;
    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    if (passive_mob_can_shear_sheep(m)) {
        passive_mob_shear_sheep(m);
        return 1;
    }
    if ((m->type == PASSIVE_MOB_COW || m->type == PASSIVE_MOB_MOOSHROOM) && held && !stack_empty(held) && held->id == ITEM_BUCKET_EMPTY) {
        consume_held_stack_one(held, ITEM_BUCKET_MILK);
        restart_hand_swing();
        g_save_dirty = 1;
        return 1;
    }
    if (m->type == PASSIVE_MOB_MOOSHROOM && held && !stack_empty(held) && held->id == ITEM_BOWL_EMPTY) {
        consume_held_stack_one(held, ITEM_BOWL_SOUP);
        restart_hand_swing();
        g_save_dirty = 1;
        return 1;
    }
    if (m->type == PASSIVE_MOB_MOOSHROOM && held && !stack_empty(held) && held->id == ITEM_SHEARS) {
        /* EntityMooshroom.interact: shearing converts it into a cow and drops five red mushrooms. */
        m->type = PASSIVE_MOB_COW;
        m->width = passive_mob_width_for_type(m->type);
        m->height = passive_mob_height_for_type(m->type);
        passive_mob_clamp_health_current_125(m);
        for (int i = 0; i < 5; ++i) spawn_item_stack(m->x, m->y + 1.0f, m->z, make_stack(BLOCK_RED_MUSHROOM, 1, 0), 1);
        damage_held_item(held, 1);
        restart_hand_swing();
        g_save_dirty = 1;
        return 1;
    }
    if (m->type == PASSIVE_MOB_WOLF && held && !stack_empty(held) && held->id == ITEM_BONE && !passive_mob_is_owned_by_player(m)) {
        if ((rand() % 3) == 0) { m->sheared = 1; m->rideable = 0; m->sitting = 1; passive_mob_assign_owner(m); m->health = m->prev_health = 20; hud_add_chat("Wolf tamed."); }
        consume_held_stack_one(held, 0);
        restart_hand_swing();
        g_save_dirty = 1;
        return 1;
    }
    if (m->type == PASSIVE_MOB_OCELOT && held && !stack_empty(held) && held->id == ITEM_FISH_RAW &&
        !passive_mob_is_owned_by_player(m) && mob_t < 3.0f) {
        if ((rand() % 3) == 0) { m->sheared = 1; m->fleece_color = 1 + (rand() % 3); passive_mob_assign_owner(m); m->tame_state = m->fleece_color; m->sitting = 1; hud_add_chat("Ocelot tamed."); }
        consume_held_stack_one(held, 0);
        restart_hand_swing();
        g_save_dirty = 1;
        return 1;
    }
    if (m->type == PASSIVE_MOB_WOLF && passive_mob_is_owned_by_player(m) && held && !stack_empty(held) && passive_mob_breeding_item_for_type(m->type, held->id)) {
        int cap = passive_mob_max_health_current_125(m);
        if (m->health < cap) {
            m->health += 3;
            if (m->health > cap) m->health = cap;
            consume_held_stack_one(held, 0);
            restart_hand_swing();
            g_save_dirty = 1;
            return 1;
        }
        return passive_mob_feed_for_love(m, held);
    }
    if (m->type == PASSIVE_MOB_OCELOT && passive_mob_is_owned_by_player(m) && held && !stack_empty(held) && passive_mob_breeding_item_for_type(m->type, held->id)) {
        return passive_mob_feed_for_love(m, held);
    }
    if ((m->type == PASSIVE_MOB_WOLF || m->type == PASSIVE_MOB_OCELOT) && passive_mob_is_owned_by_player(m)) {
        /* EntityWolf/EntityOcelot.interact toggles sitting for the owner after
           item-specific healing/breeding handling has had first chance. */
        m->sitting = !m->sitting;
        m->has_path_target = 0;
        m->target_mob_index = -1;
        passive_path_clear(m);
        restart_hand_swing();
        g_save_dirty = 1;
        return 1;
    }
    if (passive_mob_feed_for_love(m, held)) return 1;
    if (m->type == PASSIVE_MOB_PIG && held && !stack_empty(held) && held->id == ITEM_SADDLE && !m->rideable) {
        m->rideable = 1;
        consume_held_stack_one(held, 0);
        restart_hand_swing();
        g_save_dirty = 1;
        return 1;
    }
    if (m->type == PASSIVE_MOB_PIG && m->rideable) {
        g_player_riding_passive_mob = (int)(m - g_passive_mobs);
        hud_add_chat("Mounted pig.");
        restart_hand_swing();
        return 1;
    }
    return 0;
}

static void passive_mob_request_jump(PassiveMob *m, const char *why) {
    if (!m || !m->on_ground || m->jump_cooldown > 0) return;
    m->my = 0.43f;
    m->on_ground = 0;
    m->jump_cooldown = (m->type == PASSIVE_MOB_CHICKEN) ? 50 : 16;
    float yaw_rad = m->yaw * (float)M_PI / 180.0f;
    m->mx += -sinf(yaw_rad) * 0.11f;
    m->mz +=  cosf(yaw_rad) * 0.11f;
    pex_logf_trace("passive mob jump type=%s reason=%s pos=%.2f,%.2f,%.2f", passive_mob_name(m->type), why ? why : "", m->x, m->y, m->z);
}

static void passive_mob_tick_potions(PassiveMob *m) {
    if (!m || !m->active || m->death_time > 0) return;
    for (int i = 1; i < 32; ++i) {
        if (m->potion_duration[i] <= 0) continue;
        int amp = m->potion_amplifier[i];
        if (i == PEX_POTION_REGENERATION) {
            int interval = 25 >> amp;
            if (interval < 1) interval = 1;
            if ((m->potion_duration[i] % interval) == 0) {
                int cap = passive_mob_max_health_current_125(m);
                if (m->health < cap) { m->health++; g_save_dirty = 1; }
            }
        } else if (i == PEX_POTION_POISON) {
            int interval = 25 >> amp;
            if (interval < 1) interval = 1;
            if ((m->potion_duration[i] % interval) == 0 && m->health > 1) {
                m->health--;
                m->hurt_time = 10;
                g_save_dirty = 1;
            }
        }
        m->potion_duration[i]--;
        if (m->potion_duration[i] <= 0) {
            m->potion_duration[i] = 0;
            m->potion_amplifier[i] = 0;
        }
    }
}

static void passive_mob_tick_timers(PassiveMob *m) {
    m->prev_x = m->x;
    m->prev_y = m->y;
    m->prev_z = m->z;
    m->prev_yaw = m->yaw;
    m->prev_render_yaw = m->render_yaw;
    m->prev_pitch = m->pitch;
    m->prev_limb_swing = m->limb_swing;
    m->prev_limb_amount = m->limb_amount;
    if (m->hurt_time > 0) --m->hurt_time;
    if (m->damage_cooldown > 0) --m->damage_cooldown;
    if (m->recently_hit > 0) --m->recently_hit;
    if (m->jump_cooldown > 0) --m->jump_cooldown;
    if (m->wander_cooldown > 0) --m->wander_cooldown;
    if (m->type != PASSIVE_MOB_CHICKEN && m->egg_timer > 0) --m->egg_timer;
    if (m->attack_time > 0) --m->attack_time;
    if (m->burn_time > 0) --m->burn_time;
    if (m->love_time > 0) --m->love_time;
    if (m->door_open_time > 0) --m->door_open_time;
    if (m->path_recalc_cooldown > 0) --m->path_recalc_cooldown;
    if (m->ai_repath_delay > 0) --m->ai_repath_delay;
    if (m->baby_age < 0) ++m->baby_age;
    else if (m->baby_age > 0) --m->baby_age;
    ++m->age;
}

static int passive_mob_tick_death(PassiveMob *m) {
    if (m->type == PASSIVE_MOB_ENDER_DRAGON && m->death_time > 0) return passive_dragon_tick_death_125(m);
    if (m->death_time <= 0) return 0;
    ++m->death_time;
    m->has_path_target = 0;
    m->mx *= 0.80f;
    m->mz *= 0.80f;
    if (m->death_time > 20) {
        if (g_player_riding_passive_mob >= 0 && g_player_riding_passive_mob < MAX_PASSIVE_MOBS &&
            &g_passive_mobs[g_player_riding_passive_mob] == m) {
            g_player_riding_passive_mob = -1;
        }
        m->active = 0;
    }
    return 1;
}

static void passive_mob_tick_sounds(PassiveMob *m) {
    if (m->death_time > 0) return;
    if (rand() % 1000 < ++m->living_sound_delay) {
        m->living_sound_delay = -120;
        const char *s = passive_mob_living_sound_current_125(m);
        if (s) {
            float pitch = (pex_rand_float01() - pex_rand_float01()) * 0.2f + 1.0f;
            float vol = passive_mob_sound_volume(m->type);
            if (m->type == PASSIVE_MOB_PIG_ZOMBIE && m->rideable > 0) { vol *= 2.0f; pitch *= 1.8f; }
            pex_sound_play_at(s, m->x, m->y, m->z, vol, pitch);
        }
    }
}

static int passive_mob_update_liquid_state(PassiveMob *m) {
    m->was_in_water = m->in_water;
    m->in_water = passive_mob_in_water(m);
    int in_liquid = m->in_water || passive_mob_in_liquid(m);
    if (m->in_water && !m->was_in_water) {
        spawn_water_entry_particles(m->x, m->y + 0.15f, m->z, m->mx, m->mz);
        pex_sound_play_at("random.splash", m->x, m->y, m->z, 1.0f,
                          1.0f + (pex_rand_float01() - pex_rand_float01()) * 0.4f);
    }
    return in_liquid;
}

static float passive_mob_player_distance2(const PassiveMob *m) {
    float dx = g_player_x - m->x;
    float dy = (g_player_y - 1.62f) - m->y;
    float dz = g_player_z - m->z;
    return dx * dx + dy * dy + dz * dz;
}

static int passive_ray_clear_blocks_125(float x0, float y0, float z0, float x1, float y1, float z1) {
    float dx = x1 - x0, dy = y1 - y0, dz = z1 - z0;
    float len = sqrtf(dx * dx + dy * dy + dz * dz);
    if (len < 0.001f) return 1;
    int steps = (int)(len * 8.0f);
    if (steps < 1) steps = 1;
    if (steps > 512) steps = 512;
    for (int i = 1; i < steps; ++i) {
        float t = (float)i / (float)steps;
        int x = (int)floorf(x0 + dx * t);
        int y = (int)floorf(y0 + dy * t);
        int z = (int)floorf(z0 + dz * t);
        int id = flat_get_block(x, y, z);
        if (id > 0 && flat_block_is_solid(id) && !block_is_liquid(id)) return 0;
    }
    return 1;
}

static int passive_player_looking_at_mob_125(const PassiveMob *m) {
    if (!m || m->type != PASSIVE_MOB_ENDERMAN || g_player_dead || player_is_creative()) return 0;
    float lx, ly, lz;
    pex_touch_aware_look_vector(&lx, &ly, &lz);
    float tx = m->x - g_player_x;
    float ty = (m->y + m->height * 0.72f) - g_player_y;
    float tz = m->z - g_player_z;
    float dist = sqrtf(tx * tx + ty * ty + tz * tz);
    if (dist < 0.001f || dist > 64.0f) return 0;
    tx /= dist; ty /= dist; tz /= dist;
    float dot = lx * tx + ly * ty + lz * tz;
    /* EntityEnderman.shouldAttackPlayer: dot > 1 - 0.025 / distance, with line of sight. */
    if (dot <= 1.0f - 0.025f / dist) return 0;
    return passive_ray_clear_blocks_125(g_player_x, g_player_y, g_player_z, m->x, m->y + m->height * 0.72f, m->z);
}

static void passive_mob_face_point(PassiveMob *m, float x, float z, float max_turn) {
    float dx = x - m->x;
    float dz = z - m->z;
    float desired = atan2f(dz, dx) * 57.29578f - 90.0f;
    float turn = pex_wrap_degrees(desired - m->yaw);
    turn = pex_clamp_float(turn, -max_turn, max_turn);
    m->yaw += turn;
}

static void passive_mob_set_path_goal(PassiveMob *m, float x, float y, float z) {
    if (!m) return;
    m->target_x = x;
    m->target_y = y;
    m->target_z = z;
    m->path_goal_x = x;
    m->path_goal_y = y;
    m->path_goal_z = z;
    m->has_path_target = 1;
}

static int passive_mob_teleport_random_125(PassiveMob *m, float range) {
    if (!m || !m->active || m->death_time > 0) return 0;
    float ox = m->x, oy = m->y, oz = m->z;
    int hr = (int)range;
    if (hr < 8) hr = 8;
    for (int tries = 0; tries < 64; ++tries) {
        int x = (int)floorf(ox) + (rand() % (hr * 2 + 1)) - hr;
        int y = (int)floorf(oy) + (rand() % 65) - 32;
        int z = (int)floorf(oz) + (rand() % (hr * 2 + 1)) - hr;
        if (!flat_chunk_generated_at_block(x, z)) continue;
        if (y < FLAT_WORLD_Y_MIN + 1) y = FLAT_WORLD_Y_MIN + 1;
        if (y > FLAT_WORLD_Y_MAX - 2) y = FLAT_WORLD_Y_MAX - 2;
        while (y > FLAT_WORLD_Y_MIN + 1 && flat_get_block(x, y - 1, z) == 0) --y;
        if (!passive_path_can_stand_at(m->type, x, y, z)) continue;
        if (!passive_ray_clear_blocks_125(ox, oy + m->height * 0.5f, oz, (float)x + 0.5f, (float)y + m->height * 0.5f, (float)z + 0.5f)) continue;
        m->prev_x = m->x = (float)x + 0.5f;
        m->prev_y = m->y = (float)y;
        m->prev_z = m->z = (float)z + 0.5f;
        m->mx = m->my = m->mz = 0.0f;
        passive_path_clear(m);
        pex_sound_play_at("mob.endermen.portal", ox, oy, oz, 1.0f, 1.0f);
        pex_sound_play_at("mob.endermen.portal", m->x, m->y, m->z, 1.0f, 1.0f);
        g_save_dirty = 1;
        return 1;
    }
    return 0;
}

static int passive_mob_scaled_attack_damage(const PassiveMob *m, int base) {
    int d = g_opts.difficulty & 3;
    if (d == 0) return 0;
    if (m && m->type == PASSIVE_MOB_WOLF) {
        /* EntityWolf.attackEntityAsMob: 4 tamed, 2 wild/angry. */
        base = passive_mob_is_owned_by_player(m) ? 4 : 2;
    } else if (passive_mob_is_slime_family(m ? m->type : PASSIVE_MOB_NONE)) {
        base = m && m->fleece_color > 0 ? m->fleece_color : 1;
    }
    if (d == 1) return (base + 1) / 2;
    if (d == 3) return (base * 3 + 1) / 2;
    (void)m;
    return base;
}

static void passive_mob_attack_player(PassiveMob *m, int ranged) {
    const PexMobInfo *info = passive_mob_info(m->type);
    if (!info || info->attack_damage <= 0 || g_player_dead || g_player_health <= 0) return;
    if (m->attack_time > 0) return;
    int dmg = ranged ? info->attack_damage : passive_mob_scaled_attack_damage(m, info->attack_damage);
    if (dmg <= 0) return;
    if (ranged) {
        float sx = m->x;
        float sy = m->y + m->height * 0.72f;
        float sz = m->z;
        float tx = g_player_x;
        float ty = g_player_y - 0.85f;
        float tz = g_player_z;
        int owner_idx = (int)(m - g_passive_mobs);
        if (m->type == PASSIVE_MOB_SKELETON) {
            /* EntityAIArrowAttack: bow every 60 ticks, moveSpeed 0.25. */
            if (pex_spawn_projectile_from_entity(FLAT_PROJECTILE_ARROW, m->type, owner_idx, sx, sy, sz, tx, ty, tz, 1.60f, 2)) {
                pex_sound_play_at("random.bow", m->x, m->y, m->z, 1.0f, 1.0f);
                passive_owned_wolves_defend_player_125(m);
                m->attack_time = 60;
            }
            return;
        }
        if (m->type == PASSIVE_MOB_BLAZE) {
            if (pex_spawn_projectile_from_entity(FLAT_PROJECTILE_SMALL_FIREBALL, m->type, owner_idx, sx, sy, sz, tx, ty, tz, 0.75f, dmg)) {
                pex_sound_play_at("mob.blaze.hit", m->x, m->y, m->z, 1.0f, 1.0f);
                passive_owned_wolves_defend_player_125(m);
                m->attack_time = 30;
            }
            return;
        }
        if (m->type == PASSIVE_MOB_GHAST) {
            if (pex_spawn_projectile_from_entity(FLAT_PROJECTILE_LARGE_FIREBALL, m->type, owner_idx, sx, sy, sz, tx, ty, tz, 0.45f, dmg)) {
                pex_sound_play_at("mob.ghast.fireball", m->x, m->y, m->z, 10.0f, 1.0f);
                passive_owned_wolves_defend_player_125(m);
                m->attack_time = 60;
            }
            return;
        }
        if (m->type == PASSIVE_MOB_SNOWMAN) {
            if (pex_spawn_projectile_from_entity(FLAT_PROJECTILE_SNOWBALL, m->type, owner_idx, sx, sy, sz, tx, ty, tz, 1.60f, 0)) {
                pex_sound_play_at("random.bow", m->x, m->y, m->z, 1.0f, 1.2f);
                m->attack_time = 20;
            }
            return;
        }
    }
    (void)player_attack_entity_from(pex_damage_source_mob(m), dmg);
    passive_owned_wolves_defend_player_125(m);
    if (m->type == PASSIVE_MOB_CAVE_SPIDER && (g_opts.difficulty & 3) > 1) {
        player_apply_potion_effect(PEX_POTION_POISON, ((g_opts.difficulty & 3) == 3) ? 15 * 20 : 7 * 20, 0, 1.0f);
    }
    m->attack_time = 20;
    if (m->type == PASSIVE_MOB_IRON_GOLEM || m->type == PASSIVE_MOB_GIANT) m->attack_time = 30;
}

static void passive_mob_attack_other_mob(PassiveMob *m, PassiveMob *target, int ranged) {
    if (!m || !target || !target->active || target->death_time > 0 || m->attack_time > 0) return;
    const PexMobInfo *info = passive_mob_info(m->type);
    int dmg = info ? info->attack_damage : 1;
    if (m->type == PASSIVE_MOB_WOLF) dmg = passive_mob_is_owned_by_player(m) ? 4 : 2;
    if (m->type == PASSIVE_MOB_IRON_GOLEM) dmg = 7 + (rand() % 15);
    if (m->type == PASSIVE_MOB_SNOWMAN && ranged) {
        pex_spawn_projectile_from_entity(FLAT_PROJECTILE_SNOWBALL, m->type, (int)(m - g_passive_mobs), m->x, m->y + 1.4f, m->z,
                                        target->x, target->y + target->height * 0.5f, target->z, 1.60f, 0);
        m->attack_time = 20;
        return;
    }
    (void)pex_mob_attack_entity_from(target, pex_damage_source_mob(m), dmg);
    if (m->type == PASSIVE_MOB_IRON_GOLEM) {
        target->my += 0.40f;
        pex_sound_play_at("mob.irongolem.throw", m->x, m->y, m->z, 1.0f, 1.0f);
    }
    m->attack_time = 20;
}


/* -------------------------------------------------------------------------
   Java 1.2.5-style missing engine layer
   -------------------------------------------------------------------------
   This pass replaces the earlier compact approximation with ports of the
   specific 1.2.5 systems the C engine can host: PathFinder/Path/PathPoint
   semantics, PathNavigate-style point following, VillageCollection's wooden
   door sky-side classification, Village radius/center math, and serialized
   Village.dat-style data.  Ender Dragon state is intentionally untouched. */
#define PEX_AI_SWIM             0x0001
#define PEX_AI_FLEE_SUN         0x0002
#define PEX_AI_ATTACK           0x0004
#define PEX_AI_ARROW_ATTACK     0x0008
#define PEX_AI_MATE             0x0010
#define PEX_AI_TEMPT            0x0020
#define PEX_AI_FOLLOW_PARENT    0x0040
#define PEX_AI_MOVE_VILLAGE     0x0080
#define PEX_AI_WANDER           0x0100
#define PEX_AI_SIT              0x0200
#define PEX_AI_AVOID_PLAYER     0x0400
#define PEX_AI_FOLLOW_OWNER     0x0800
#define PEX_AI_BREAK_DOOR       0x1000

#define PEX_MAX_RUNTIME_VILLAGES 16
#define PEX_MAX_VILLAGE_DOORS 64

typedef struct PexVillageDoorRuntime {
    int x, y, z;
    int inside_x, inside_z;
    int last_activity;
    int detached;
    int opening_restriction_counter;
} PexVillageDoorRuntime;

typedef struct PexVillageRuntime {
    int active;
    int id;
    int chunk_x, chunk_z;
    int cx, cy, cz;
    int center_helper_x, center_helper_y, center_helper_z;
    int radius;
    int last_add_door_tick;
    int tick_counter;
    int door_count;
    int villager_count;
    int golem_count;
    int player_reputation; /* single-player stand-in for Village.playerReputation map */
    int door_scan_active;
    int door_scan_x, door_scan_y, door_scan_z;
    int door_scan_y_min, door_scan_y_max;
    int synthetic_doors_seeded;
    PexVillageDoorRuntime doors[PEX_MAX_VILLAGE_DOORS];
} PexVillageRuntime;

static PexVillageRuntime g_passive_villages[PEX_MAX_RUNTIME_VILLAGES];
static int g_passive_villages_tick = -9999;
static int g_passive_next_village_id = 1;

/* The Java-style systems below are intentionally exact in their decisions, but
   the old patch recalculated expensive A* paths every tick for any target more
   than eight blocks away.  On a flat world with hostile spawns that turns one
   player chase into dozens of full PathFinder searches per rendered frame.
   Keep the same movement/path semantics, but budget path builds like Minecraft's
   PathNavigate cooldowns do and let mobs steer toward their existing path/target
   while waiting for the next solve. */
#define PEX_PATHFIND_BUDGET_PER_TICK 2
#define PEX_PATHFIND_NODE_BUDGET_PER_SOLVE 768
#define PEX_SPAWN_PROBE_BUDGET_PER_TICK 96
#define PEX_MOB_TICK_WALL_BUDGET_MS 3.0
static int g_passive_pathfind_budget = 0;
static int g_passive_perf_last_active = 0;
static int g_passive_perf_last_path_budget_left = 0;
static int g_passive_perf_last_spawns_skipped_streaming = 0;

static int passive_path_can_stand_at(int type, int x, int y, int z) {
    if (type == PASSIVE_MOB_GHAST || type == PASSIVE_MOB_BLAZE) return flat_get_block(x, y, z) == 0;
    if (type == PASSIVE_MOB_SQUID) return passive_block_is_water(flat_get_block(x, y, z));
    return passive_mob_ground_target_ok(x, y, z);
}

static int passive_path_blocks_movement_125(int id, int x, int y, int z, int pass_closed_wood_doors) {
    if (id == 0) return 1;
    if (block_is_water(id) || id == BLOCK_TRAPDOOR) return 1;
    if (block_is_door_id(id)) {
        if (pass_closed_wood_doors) return 1;
        return door_is_open_at(x, y, z) ? 1 : 0;
    }
    if (id == BLOCK_FENCE || id == BLOCK_FENCE_GATE) return 0;
    if (block_is_liquid(id)) return 0;
    return flat_block_is_solid(id) ? 0 : 1;
}

typedef struct Pex125PathPoint {
    int x, y, z;
    int hash;
    int index;
    float total_path_distance;
    float distance_to_next;
    float distance_to_target;
    short previous;
    short next_hash;
    unsigned char is_first;
    unsigned char used;
} Pex125PathPoint;

#define PEX125_PATH_POINT_MAX 1536
#define PEX125_PATH_HEAP_MAX 1536
#define PEX125_PATH_HASH_SIZE 2048

typedef struct Pex125PathContext {
    Pex125PathPoint points[PEX125_PATH_POINT_MAX];
    int point_count;
    short heap[PEX125_PATH_HEAP_MAX];
    short hash_head[PEX125_PATH_HASH_SIZE];
    int heap_count;
    int allow_wooden_door;
    int movement_block_allowed;
    int pathing_in_water;
    int can_entity_drown;
    int entity_type;
} Pex125PathContext;

static int passive_path_point_hash_125(int x, int y, int z) {
    unsigned int ux = (unsigned int)(x & 32767);
    unsigned int uz = (unsigned int)(z & 32767);
    unsigned int h = (unsigned int)(y & 255) | (ux << 8) | (uz << 24);
    if (x < 0) h |= 0x80000000u;
    if (z < 0) h |= 0x00008000u;
    return (int)h;
}

static float passive_path_point_distance_125(const Pex125PathPoint *a, const Pex125PathPoint *b) {
    float dx = (float)(b->x - a->x), dy = (float)(b->y - a->y), dz = (float)(b->z - a->z);
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

static short passive_path_open_point_125(Pex125PathContext *ctx, int x, int y, int z) {
    int h = passive_path_point_hash_125(x,y,z);
    int bucket = ((unsigned int)h) & (PEX125_PATH_HASH_SIZE - 1);
    for (short idx = ctx->hash_head[bucket]; idx >= 0; idx = ctx->points[idx].next_hash) {
        Pex125PathPoint *p = &ctx->points[idx];
        if (p->used && p->hash == h && p->x == x && p->y == y && p->z == z) return idx;
    }
    if (ctx->point_count >= PEX125_PATH_POINT_MAX) return -1;
    int i = ctx->point_count++;
    Pex125PathPoint *p = &ctx->points[i];
    memset(p, 0, sizeof(*p));
    p->x = x; p->y = y; p->z = z; p->hash = h; p->index = -1; p->previous = -1; p->used = 1;
    p->next_hash = ctx->hash_head[bucket];
    ctx->hash_head[bucket] = (short)i;
    return (short)i;
}

static void passive_path_heap_swap_125(Pex125PathContext *ctx, int a, int b) {
    short pa = ctx->heap[a], pb = ctx->heap[b];
    ctx->heap[a] = pb; ctx->heap[b] = pa;
    ctx->points[pb].index = a;
    ctx->points[pa].index = b;
}

static void passive_path_heap_sort_back_125(Pex125PathContext *ctx, int i) {
    while (i > 0) {
        int p = (i - 1) >> 1;
        if (ctx->points[ctx->heap[i]].distance_to_target >= ctx->points[ctx->heap[p]].distance_to_target) break;
        passive_path_heap_swap_125(ctx, i, p);
        i = p;
    }
}

static void passive_path_heap_sort_forward_125(Pex125PathContext *ctx, int i) {
    for (;;) {
        int l = (i << 1) + 1;
        int r = l + 1;
        if (l >= ctx->heap_count) break;
        int best = l;
        if (r < ctx->heap_count && ctx->points[ctx->heap[r]].distance_to_target < ctx->points[ctx->heap[l]].distance_to_target) best = r;
        if (ctx->points[ctx->heap[best]].distance_to_target >= ctx->points[ctx->heap[i]].distance_to_target) break;
        passive_path_heap_swap_125(ctx, i, best);
        i = best;
    }
}

static void passive_path_heap_add_125(Pex125PathContext *ctx, short idx) {
    if (idx < 0 || ctx->heap_count >= PEX125_PATH_HEAP_MAX) return;
    ctx->heap[ctx->heap_count] = idx;
    ctx->points[idx].index = ctx->heap_count;
    passive_path_heap_sort_back_125(ctx, ctx->heap_count++);
}

static short passive_path_heap_dequeue_125(Pex125PathContext *ctx) {
    if (ctx->heap_count <= 0) return -1;
    short r = ctx->heap[0];
    ctx->points[r].index = -1;
    --ctx->heap_count;
    if (ctx->heap_count > 0) {
        ctx->heap[0] = ctx->heap[ctx->heap_count];
        ctx->points[ctx->heap[0]].index = 0;
        passive_path_heap_sort_forward_125(ctx, 0);
    }
    return r;
}

static void passive_path_heap_change_distance_125(Pex125PathContext *ctx, short idx, float d) {
    if (idx < 0) return;
    float old = ctx->points[idx].distance_to_target;
    ctx->points[idx].distance_to_target = d;
    if (ctx->points[idx].index < 0) return;
    if (d < old) passive_path_heap_sort_back_125(ctx, ctx->points[idx].index);
    else passive_path_heap_sort_forward_125(ctx, ctx->points[idx].index);
}

static int passive_path_vertical_offset_125(Pex125PathContext *ctx, const PassiveMob *m, int x, int y, int z, int sx, int sy, int sz) {
    (void)m;
    int trap_or_water = 0;
    for (int ix = x; ix < x + sx; ++ix) {
        for (int iy = y; iy < y + sy; ++iy) {
            for (int iz = z; iz < z + sz; ++iz) {
                int id = flat_get_block(ix, iy, iz);
                if (id <= 0) continue;
                if (id == BLOCK_TRAPDOOR) {
                    trap_or_water = 1;
                } else if (block_is_water(id)) {
                    if (ctx->pathing_in_water) return -1;
                    trap_or_water = 1;
                } else {
                    if (!ctx->allow_wooden_door && id == BLOCK_WOOD_DOOR) return 0;
                }
                if (!passive_path_blocks_movement_125(id, ix, iy, iz, ctx->movement_block_allowed)) {
                    if (id == BLOCK_FENCE || id == BLOCK_FENCE_GATE) return -3;
                    if (id == BLOCK_TRAPDOOR) return -4;
                    if (block_is_liquid(id) && !passive_block_is_water(id)) return -2;
                    return 0;
                }
            }
        }
    }
    return trap_or_water ? 2 : 1;
}

static short passive_path_get_safe_point_125(Pex125PathContext *ctx, const PassiveMob *m, int x, int y, int z, int sx, int sy, int sz, int step_height) {
    short result = -1;
    int vo = passive_path_vertical_offset_125(ctx, m, x, y, z, sx, sy, sz);
    if (vo == 2) return passive_path_open_point_125(ctx, x, y, z);
    if (vo == 1) result = passive_path_open_point_125(ctx, x, y, z);
    if (result < 0 && step_height > 0 && vo != -3 && vo != -4 && passive_path_vertical_offset_125(ctx, m, x, y + step_height, z, sx, sy, sz) == 1) {
        result = passive_path_open_point_125(ctx, x, y + step_height, z);
        y += step_height;
    }
    if (result >= 0) {
        int fall = 0;
        int below = 0;
        while (y > FLAT_WORLD_Y_MIN) {
            below = passive_path_vertical_offset_125(ctx, m, x, y - 1, z, sx, sy, sz);
            if (ctx->pathing_in_water && below == -1) return -1;
            if (below != 1) break;
            ++fall;
            if (fall >= 4) return -1;
            --y;
            if (y > FLAT_WORLD_Y_MIN) result = passive_path_open_point_125(ctx, x, y, z);
        }
        if (below == -2) return -1;
    }
    return result;
}

static int passive_path_find_options_125(Pex125PathContext *ctx, const PassiveMob *m, short cur_idx, short goal_idx, int sx, int sy, int sz, float max_dist, short out[4]) {
    int count = 0;
    Pex125PathPoint *cur = &ctx->points[cur_idx];
    Pex125PathPoint *goal = &ctx->points[goal_idx];
    int step = 0;
    if (passive_path_vertical_offset_125(ctx, m, cur->x, cur->y + 1, cur->z, sx, sy, sz) == 1) step = 1;
    short p[4];
    p[0] = passive_path_get_safe_point_125(ctx, m, cur->x,     cur->y, cur->z + 1, sx, sy, sz, step);
    p[1] = passive_path_get_safe_point_125(ctx, m, cur->x - 1, cur->y, cur->z,     sx, sy, sz, step);
    p[2] = passive_path_get_safe_point_125(ctx, m, cur->x + 1, cur->y, cur->z,     sx, sy, sz, step);
    p[3] = passive_path_get_safe_point_125(ctx, m, cur->x,     cur->y, cur->z - 1, sx, sy, sz, step);
    for (int i=0;i<4;++i) {
        if (p[i] >= 0 && !ctx->points[p[i]].is_first && passive_path_point_distance_125(&ctx->points[p[i]], goal) < max_dist) out[count++] = p[i];
    }
    return count;
}

static int passive_path_create_output_125(PassiveMob *m, Pex125PathContext *ctx, short start_idx, short end_idx) {
    short rev[PEX_MOB_PATH_MAX * 3];
    int n = 0;
    for (short cur = end_idx; cur >= 0 && n < (int)(sizeof(rev)/sizeof(rev[0])); cur = ctx->points[cur].previous) {
        rev[n++] = cur;
        if (cur == start_idx) break;
    }
    if (n <= 1) return 0;
    int outn = 0;
    float half = (float)((int)(m->width + 1.0f)) * 0.5f; /* PathEntity.getVectorFromIndex */
    for (int i = n - 2; i >= 0 && outn < PEX_MOB_PATH_MAX; --i) {
        Pex125PathPoint *p = &ctx->points[rev[i]];
        m->path_x[outn] = p->x;
        m->path_y[outn] = p->y;
        m->path_z[outn] = p->z;
        (void)half;
        outn++;
    }
    m->path_len = outn;
    m->path_index = 0;
    m->path_recalc_cooldown = 20 + (rand() % 20);
    m->path_stuck_check_tick = m->age;
    m->path_stuck_check_x = m->x;
    m->path_stuck_check_y = m->y;
    m->path_stuck_check_z = m->z;
    return outn > 0;
}

static int passive_path_find(PassiveMob *m, float txf, float tyf, float tzf) {
    if (!m) return 0;
    if (m->type == PASSIVE_MOB_ENDER_DRAGON) return 0; /* intentionally not touched */
    if (g_passive_pathfind_budget <= 0) {
        if (m->path_recalc_cooldown <= 0) m->path_recalc_cooldown = 4;
        return 0;
    }
    --g_passive_pathfind_budget;
    ++g_prof_mob_path_solves_last;
    if (m->type == PASSIVE_MOB_GHAST || m->type == PASSIVE_MOB_BLAZE || m->type == PASSIVE_MOB_SQUID) {
        int gx = (int)floorf(txf), gy = (int)floorf(tyf), gz = (int)floorf(tzf);
        if (!passive_path_can_stand_at(m->type, gx, gy, gz)) return 0;
        m->path_len = 1; m->path_index = 0; m->path_x[0] = gx; m->path_y[0] = gy; m->path_z[0] = gz; m->path_recalc_cooldown = 20;
        m->path_stuck_check_tick = m->age;
        m->path_stuck_check_x = m->x;
        m->path_stuck_check_y = m->y;
        m->path_stuck_check_z = m->z;
        return 1;
    }
    Pex125PathContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    for (int hi = 0; hi < PEX125_PATH_HASH_SIZE; ++hi) ctx.hash_head[hi] = -1;
    ctx.allow_wooden_door = 1;
    ctx.movement_block_allowed = (m->type == PASSIVE_MOB_ZOMBIE); /* EntityAIBreakDoor mobs may pass closed doors while breaking */
    ctx.pathing_in_water = 0;
    ctx.can_entity_drown = 1;
    ctx.entity_type = m->type;

    int sx = (int)floorf(m->width + 1.0f);
    int sy = (int)floorf(m->height + 1.0f);
    int sz = (int)floorf(m->width + 1.0f);
    if (sx < 1) sx = 1; if (sy < 1) sy = 1; if (sz < 1) sz = 1;
    int start_y;
    if (ctx.can_entity_drown && m->in_water) {
        start_y = (int)floorf(m->y);
        while (passive_block_is_water(flat_get_block((int)floorf(m->x), start_y, (int)floorf(m->z))) && start_y < FLAT_WORLD_Y_MAX) ++start_y;
        ctx.pathing_in_water = 0;
    } else {
        start_y = (int)floorf(m->y + 0.5f);
    }
    int start_x = (int)floorf(m->x - m->width * 0.5f);
    int start_z = (int)floorf(m->z - m->width * 0.5f);
    int goal_x = (int)floorf(txf - m->width * 0.5f);
    int goal_y = (int)floorf(tyf);
    int goal_z = (int)floorf(tzf - m->width * 0.5f);

    short start = passive_path_open_point_125(&ctx, start_x, start_y, start_z);
    short goal = passive_path_open_point_125(&ctx, goal_x, goal_y, goal_z);
    if (start < 0 || goal < 0) return 0;
    float search_range = 32.0f;
    if (m->type == PASSIVE_MOB_IRON_GOLEM || m->type == PASSIVE_MOB_VILLAGER) search_range = 48.0f;
    ctx.points[start].total_path_distance = 0.0f;
    ctx.points[start].distance_to_next = passive_path_point_distance_125(&ctx.points[start], &ctx.points[goal]);
    ctx.points[start].distance_to_target = ctx.points[start].distance_to_next;
    passive_path_heap_add_125(&ctx, start);
    short closest = start;
    int guard = 0;
    while (ctx.heap_count > 0 && guard++ < PEX_PATHFIND_NODE_BUDGET_PER_SOLVE) {
        ++g_prof_mob_path_nodes_last;
        short cur = passive_path_heap_dequeue_125(&ctx);
        if (cur < 0) break;
        if (ctx.points[cur].x == ctx.points[goal].x && ctx.points[cur].y == ctx.points[goal].y && ctx.points[cur].z == ctx.points[goal].z) {
            return passive_path_create_output_125(m, &ctx, start, cur);
        }
        if (passive_path_point_distance_125(&ctx.points[cur], &ctx.points[goal]) < passive_path_point_distance_125(&ctx.points[closest], &ctx.points[goal])) closest = cur;
        ctx.points[cur].is_first = 1;
        short opts[4];
        int optn = passive_path_find_options_125(&ctx, m, cur, goal, sx, sy, sz, search_range, opts);
        for (int i=0;i<optn;++i) {
            short oi = opts[i];
            float nd = ctx.points[cur].total_path_distance + passive_path_point_distance_125(&ctx.points[cur], &ctx.points[oi]);
            if (ctx.points[oi].index < 0 || nd < ctx.points[oi].total_path_distance) {
                ctx.points[oi].previous = cur;
                ctx.points[oi].total_path_distance = nd;
                ctx.points[oi].distance_to_next = passive_path_point_distance_125(&ctx.points[oi], &ctx.points[goal]);
                if (ctx.points[oi].index >= 0) passive_path_heap_change_distance_125(&ctx, oi, ctx.points[oi].total_path_distance + ctx.points[oi].distance_to_next);
                else { ctx.points[oi].distance_to_target = ctx.points[oi].total_path_distance + ctx.points[oi].distance_to_next; passive_path_heap_add_125(&ctx, oi); }
            }
        }
    }
    if (closest == start) return 0;
    return passive_path_create_output_125(m, &ctx, start, closest);
}

static int passive_random_position_away_from_player(PassiveMob *m, int h_range, int v_range,
                                                    float *out_x, float *out_y, float *out_z) {
    if (!m || !out_x || !out_y || !out_z) return 0;
    float away_x = m->x - g_player_x;
    float away_z = m->z - g_player_z;
    float away_len = sqrtf(away_x * away_x + away_z * away_z);
    if (away_len < 0.001f) {
        float a = pex_rand_float01() * (float)M_PI * 2.0f;
        away_x = cosf(a);
        away_z = sinf(a);
        away_len = 1.0f;
    }
    away_x /= away_len;
    away_z /= away_len;

    int base_x = (int)floorf(m->x);
    int base_y = (int)floorf(m->y);
    int base_z = (int)floorf(m->z);
    int best_x = 0, best_y = 0, best_z = 0;
    float best_score = -999999.0f;
    for (int i = 0; i < 10; ++i) {
        int x = base_x + (rand() % (h_range * 2 + 1)) - h_range;
        int y = base_y + (rand() % (v_range * 2 + 1)) - v_range;
        int z = base_z + (rand() % (h_range * 2 + 1)) - h_range;
        if (!passive_path_can_stand_at(m->type, x, y, z)) continue;
        float wx = (float)x + 0.5f;
        float wz = (float)z + 0.5f;
        float dx = wx - m->x;
        float dz = wz - m->z;
        float d2 = dx * dx + dz * dz;
        if (d2 < 1.0f) continue;
        float dir = (dx * away_x + dz * away_z) / sqrtf(d2);
        if (dir < 0.2f) continue;
        float pdx = wx - g_player_x;
        float pdz = wz - g_player_z;
        float score = dir * 24.0f + pdx * pdx + pdz * pdz + passive_mob_path_weight_125(m, x, y, z);
        if (score > best_score) {
            best_score = score;
            best_x = x;
            best_y = y;
            best_z = z;
        }
    }
    if (best_score <= -99999.0f) return 0;
    *out_x = (float)best_x + 0.5f;
    *out_y = (float)best_y;
    *out_z = (float)best_z + 0.5f;
    return 1;
}

static void passive_path_clear(PassiveMob *m) {
    if (!m) return;
    m->has_path_target = 0;
    m->path_len = 0;
    m->path_index = 0;
    m->path_recalc_cooldown = 0;
    m->path_stuck_check_tick = m->age;
    m->path_stuck_check_x = m->x;
    m->path_stuck_check_y = m->y;
    m->path_stuck_check_z = m->z;
}

static int passive_path_drive(PassiveMob *m) {
    if (!m || !m->has_path_target) return 0;
    if (m->path_len > 0 && m->path_index < m->path_len && m->age - m->path_stuck_check_tick > 100) {
        float dx = m->x - m->path_stuck_check_x;
        float dy = m->y - m->path_stuck_check_y;
        float dz = m->z - m->path_stuck_check_z;
        if (dx * dx + dy * dy + dz * dz < 2.25f) {
            passive_path_clear(m);
            return 0;
        }
        m->path_stuck_check_tick = m->age;
        m->path_stuck_check_x = m->x;
        m->path_stuck_check_y = m->y;
        m->path_stuck_check_z = m->z;
    }
    if (m->path_len <= 0 || m->path_index >= m->path_len || m->path_recalc_cooldown <= 0 || m->stuck_ticks > 100) {
        int solved = passive_path_find(m, m->path_goal_x, m->path_goal_y, m->path_goal_z);
        if (!solved && (m->path_len <= 0 || m->path_index >= m->path_len)) {
            m->path_len = 0;
            m->path_index = 0;
            m->path_recalc_cooldown = 20 + (rand() % 20);
            return 0;
        }
    }
    if (m->path_len <= 0 || m->path_index >= m->path_len) return 0;
    int px = m->path_x[m->path_index], py = m->path_y[m->path_index], pz = m->path_z[m->path_index];
    float path_half = (float)((int)(m->width + 1.0f)) * 0.5f;
    float wx = (float)px + path_half, wy = (float)py, wz = (float)pz + path_half;
    float ddx = wx - m->x, ddz = wz - m->z;
    float advance = m->width;
    if (advance < 0.45f) advance = 0.45f;
    if (ddx*ddx + ddz*ddz < advance * advance && fabsf(wy - m->y) < 1.25f) {
        m->path_index++;
        if (m->path_index >= m->path_len) return 0;
        px = m->path_x[m->path_index]; py = m->path_y[m->path_index]; pz = m->path_z[m->path_index];
        wx = (float)px + path_half; wy = (float)py; wz = (float)pz + path_half;
    }
    m->target_x = wx; m->target_y = wy; m->target_z = wz;
    return 1;
}

static int passive_village_door_orientation_125(int x, int y, int z) {
    int ly = door_lower_y_at(x, y, z);
    int meta = flat_get_meta(x, ly, z) & 3;
    return meta;
}

static int passive_village_door_sky_score_125(int x, int y, int z, int orientation, int *out_ix, int *out_iz) {
    int score = 0;
    if (orientation != 0 && orientation != 2) {
        for (int dz = -5; dz < 0; ++dz) if (passive_can_see_sky(x, y, z + dz)) --score;
        for (int dz = 1; dz <= 5; ++dz) if (passive_can_see_sky(x, y, z + dz)) ++score;
        if (score != 0) { if (out_ix) *out_ix = 0; if (out_iz) *out_iz = score > 0 ? -2 : 2; }
    } else {
        for (int dx = -5; dx < 0; ++dx) if (passive_can_see_sky(x + dx, y, z)) --score;
        for (int dx = 1; dx <= 5; ++dx) if (passive_can_see_sky(x + dx, y, z)) ++score;
        if (score != 0) { if (out_ix) *out_ix = score > 0 ? -2 : 2; if (out_iz) *out_iz = 0; }
    }
    return score;
}

static int passive_village_door_distance_sq(const PexVillageDoorRuntime *d, int x, int y, int z) {
    int dx = x - d->x, dy = y - d->y, dz = z - d->z;
    return dx*dx + dy*dy + dz*dz;
}

static int passive_village_door_inside_distance_sq(const PexVillageDoorRuntime *d, int x, int y, int z) {
    int dx = x - d->x - d->inside_x;
    int dy = y - d->y;
    int dz = z - d->z - d->inside_z;
    return dx*dx + dy*dy + dz*dz;
}

static int passive_village_is_block_door_125(int x, int y, int z) {
    return flat_get_block(x, y, z) == BLOCK_WOOD_DOOR;
}

static void passive_village_update_radius_and_center_125(PexVillageRuntime *v) {
    if (!v) return;
    if (v->door_count <= 0) {
        v->cx = v->chunk_x * 16 + 8; v->cy = 64; v->cz = v->chunk_z * 16 + 8; v->radius = 0;
        v->center_helper_x = v->center_helper_y = v->center_helper_z = 0;
        return;
    }
    v->cx = v->center_helper_x / v->door_count;
    v->cy = v->center_helper_y / v->door_count;
    v->cz = v->center_helper_z / v->door_count;
    int maxd = 0;
    for (int i=0;i<v->door_count;++i) {
        int d = passive_village_door_distance_sq(&v->doors[i], v->cx, v->cy, v->cz);
        if (d > maxd) maxd = d;
    }
    v->radius = (int)sqrtf((float)maxd) + 1;
    if (v->radius < 32) v->radius = 32;
}

static int passive_village_get_door_at_125(PexVillageRuntime *v, int x, int y, int z) {
    if (!v || v->door_count <= 0) return -1;
    int dx = x - v->cx, dy = y - v->cy, dz = z - v->cz;
    if (dx*dx + dy*dy + dz*dz > v->radius * v->radius) return -1;
    for (int i=0;i<v->door_count;++i) {
        PexVillageDoorRuntime *d = &v->doors[i];
        if (d->x == x && d->z == z && abs(d->y - y) <= 1) return i;
    }
    return -1;
}

static void passive_village_add_door_info_125(PexVillageRuntime *v, int x, int y, int z, int ix, int iz) {
    if (!v || v->door_count >= PEX_MAX_VILLAGE_DOORS) return;
    if (passive_village_get_door_at_125(v, x, y, z) >= 0) return;
    PexVillageDoorRuntime *d = &v->doors[v->door_count++];
    memset(d, 0, sizeof(*d));
    d->x = x; d->y = y; d->z = z;
    d->inside_x = ix; d->inside_z = iz;
    d->last_activity = g_ingame_ticks;
    d->opening_restriction_counter = 0;
    v->center_helper_x += x;
    v->center_helper_y += y;
    v->center_helper_z += z;
    v->last_add_door_tick = d->last_activity;
    passive_village_update_radius_and_center_125(v);
}

static void passive_village_add_door(PexVillageRuntime *v, int x, int y, int z) {
    if (!v || v->door_count >= PEX_MAX_VILLAGE_DOORS) return;
    int orientation = passive_village_door_orientation_125(x, y, z);
    int ix = 0, iz = 0;
    int score = passive_village_door_sky_score_125(x, y, z, orientation, &ix, &iz);
    if (score == 0) return; /* VillageCollection rejects doors with equal sky on both sides. */
    passive_village_add_door_info_125(v, x, y, z, ix, iz);
}

static void passive_village_remove_door_125(PexVillageRuntime *v, int idx) {
    if (!v || idx < 0 || idx >= v->door_count) return;
    PexVillageDoorRuntime *d = &v->doors[idx];
    v->center_helper_x -= d->x;
    v->center_helper_y -= d->y;
    v->center_helper_z -= d->z;
    d->detached = 1;
    for (int j=idx+1;j<v->door_count;++j) v->doors[j-1] = v->doors[j];
    --v->door_count;
    passive_village_update_radius_and_center_125(v);
}

static void passive_village_remove_dead_and_out_of_range_doors_125(PexVillageRuntime *v) {
    if (!v) return;
    int reset = (rand() % 50) == 0;
    for (int i=0;i<v->door_count;) {
        PexVillageDoorRuntime *d = &v->doors[i];
        if (reset) d->opening_restriction_counter = 0;
        if (passive_village_is_block_door_125(d->x, d->y, d->z) && abs(g_ingame_ticks - d->last_activity) <= 1200) { ++i; continue; }
        passive_village_remove_door_125(v, i);
    }
}

static void passive_village_seed_synthetic_doors(PexVillageRuntime *v) {
    if (!v || v->synthetic_doors_seeded) return;
    /* Fallback only: enough for gameplay while real door discovery is spread
       across later ticks.  Real discovered doors are added through the same
       Java sky-side scoring path. */
    static const int voffs[][3] = {
        {8,1,-4},{-15,1,6},{12,1,10},{-24,1,-12},{4,1,4},{-4,1,8},
        {17,1,-7},{-7,1,-16},{23,1,14},{-19,1,18},{2,1,-24},{-28,1,2}
    };
    for (int i = 0; i < (int)(sizeof(voffs)/sizeof(voffs[0])); ++i) {
        int x = v->cx + voffs[i][0], z = v->cz + voffs[i][2];
        int y = passive_find_surface_for_village_spawn(x, z);
        if (y == -9999) continue;
        int ix = (i & 1) ? 2 : -2, iz = (i & 2) ? 2 : -2;
        passive_village_add_door_info_125(v, x, y + voffs[i][1], z, ix, iz);
    }
    v->synthetic_doors_seeded = 1;
}

static void passive_village_scan_doors(PexVillageRuntime *v) {
    if (!v) return;
    const int budget = 8192;
    int scanned = 0;
    if (!v->door_scan_active) {
        memset(v->doors, 0, sizeof(v->doors));
        v->door_count = 0;
        v->center_helper_x = v->center_helper_y = v->center_helper_z = 0;
        v->synthetic_doors_seeded = 0;
        v->door_scan_y_min = v->cy - 20;
        v->door_scan_y_max = v->cy + 20;
        if (v->door_scan_y_min < FLAT_WORLD_Y_MIN + 1) v->door_scan_y_min = FLAT_WORLD_Y_MIN + 1;
        if (v->door_scan_y_max > FLAT_WORLD_Y_MAX - 2) v->door_scan_y_max = FLAT_WORLD_Y_MAX - 2;
        v->door_scan_y = v->door_scan_y_min;
        v->door_scan_z = v->cz - 48;
        v->door_scan_x = v->cx - 48;
        v->door_scan_active = 1;
        passive_village_seed_synthetic_doors(v);
    }
    while (v->door_scan_active && scanned < budget) {
        int x = v->door_scan_x;
        int y = v->door_scan_y;
        int z = v->door_scan_z;
        if (flat_chunk_generated_at_block(x, z) && passive_village_is_block_door_125(x, y, z)) passive_village_add_door(v, x, y, z);
        ++scanned;
        ++v->door_scan_x;
        if (v->door_scan_x > v->cx + 48) {
            v->door_scan_x = v->cx - 48;
            ++v->door_scan_z;
            if (v->door_scan_z > v->cz + 48) {
                v->door_scan_z = v->cz - 48;
                ++v->door_scan_y;
                if (v->door_scan_y > v->door_scan_y_max || v->door_count >= PEX_MAX_VILLAGE_DOORS) {
                    v->door_scan_active = 0;
                    break;
                }
            }
        }
    }
    g_prof_village_scan_blocks_last = scanned;
    passive_village_update_radius_and_center_125(v);
}

static void passive_villages_refresh(void) {
    if (g_passive_villages_tick == g_ingame_ticks) return;
    if (g_passive_villages_tick > 0 && g_ingame_ticks - g_passive_villages_tick < 20) return;
    g_passive_villages_tick = g_ingame_ticks;
    int old_count = 0;
    PexVillageRuntime old[PEX_MAX_RUNTIME_VILLAGES];
    memcpy(old, g_passive_villages, sizeof(old));
    for (int i=0;i<PEX_MAX_RUNTIME_VILLAGES;++i) if (old[i].active) ++old_count;
    memset(g_passive_villages, 0, sizeof(g_passive_villages));
    int pcx = floor_div16((int)floorf(g_player_x));
    int pcz = floor_div16((int)floorf(g_player_z));
    int n = 0;
    for (int vx = pcx - 8; vx <= pcx + 8 && n < PEX_MAX_RUNTIME_VILLAGES; ++vx) {
        for (int vz = pcz - 8; vz <= pcz + 8 && n < PEX_MAX_RUNTIME_VILLAGES; ++vz) {
            if (!worldgen_village_can_spawn((long long)g_world_seed, vx, vz)) continue;
            PexVillageRuntime *v = &g_passive_villages[n++];
            int reused = 0;
            for (int oi=0; oi<PEX_MAX_RUNTIME_VILLAGES; ++oi) {
                if (old[oi].active && old[oi].chunk_x == vx && old[oi].chunk_z == vz) { *v = old[oi]; reused = 1; break; }
            }
            if (!reused) {
                memset(v, 0, sizeof(*v));
                v->active = 1;
                v->id = g_passive_next_village_id++;
                if (g_passive_next_village_id < 1) g_passive_next_village_id = 1;
                v->chunk_x = vx; v->chunk_z = vz;
                v->cx = vx * 16 + 8; v->cz = vz * 16 + 8; v->cy = 64;
                v->player_reputation = 0;
                passive_village_scan_doors(v);
            } else {
                v->active = 1;
                v->villager_count = 0;
                v->golem_count = 0;
                v->tick_counter = g_ingame_ticks;
                passive_village_remove_dead_and_out_of_range_doors_125(v);
                if ((g_ingame_ticks & 127) == 0) passive_village_scan_doors(v);
            }
        }
    }
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        PassiveMob *m = &g_passive_mobs[i];
        if (!m->active) continue;
        m->village_id = -1;
        for (int vi = 0; vi < PEX_MAX_RUNTIME_VILLAGES; ++vi) {
            PexVillageRuntime *v = &g_passive_villages[vi];
            if (!v->active || v->radius <= 0) continue;
            float dx = m->x - (float)v->cx, dy = m->y - (float)v->cy, dz = m->z - (float)v->cz;
            if (dx*dx + dy*dy + dz*dz > (float)(v->radius * v->radius)) continue;
            m->village_id = v->id;
            if (m->type == PASSIVE_MOB_VILLAGER) ++v->villager_count;
            else if (m->type == PASSIVE_MOB_IRON_GOLEM) ++v->golem_count;
            break;
        }
    }
}

static PexVillageRuntime *passive_village_by_id(int id) {
    if (id < 0) return NULL;
    passive_villages_refresh();
    for (int i = 0; i < PEX_MAX_RUNTIME_VILLAGES; ++i) if (g_passive_villages[i].active && g_passive_villages[i].id == id) return &g_passive_villages[i];
    return NULL;
}

static PexVillageRuntime *passive_nearest_village(float x, float z, float range) {
    passive_villages_refresh();
    PexVillageRuntime *best = NULL;
    float best2 = range * range;
    for (int i = 0; i < PEX_MAX_RUNTIME_VILLAGES; ++i) {
        PexVillageRuntime *v = &g_passive_villages[i];
        if (!v->active) continue;
        float dx = x - (float)v->cx, dz = z - (float)v->cz;
        float d2 = dx*dx + dz*dz;
        if (d2 < best2) { best2 = d2; best = v; }
    }
    return best;
}

static int passive_villager_pick_door(PassiveMob *m, PexVillageRuntime *v) {
    if (!m || !v || v->door_count <= 0) return 0;
    int mx = (int)floorf(m->x), my = (int)floorf(m->y), mz = (int)floorf(m->z);
    int best = -1;
    int best_score = 0x7fffffff;
    /* Village.findNearestDoorUnrestricted: doors farther than 16 blocks are
       heavily penalized; nearby doors are selected by opening restriction
       count so villagers spread across houses instead of all choosing nearest. */
    for (int i = 0; i < v->door_count; ++i) {
        PexVillageDoorRuntime *d = &v->doors[i];
        int score = passive_village_door_distance_sq(d, mx, my, mz);
        if (score > 256) score *= 1000;
        else score = d->opening_restriction_counter;
        if (score < best_score) { best_score = score; best = i; }
    }
    if (best < 0) return 0;
    PexVillageDoorRuntime *d = &v->doors[best];
    d->opening_restriction_counter++;
    d->last_activity = g_ingame_ticks;
    m->door_target_x = d->x; m->door_target_y = d->y; m->door_target_z = d->z;
    m->door_open_time = 40;
    passive_mob_set_path_goal(m, (float)d->x + (float)d->inside_x + 0.5f, (float)d->y, (float)d->z + (float)d->inside_z + 0.5f);
    passive_path_find(m, m->path_goal_x, m->path_goal_y, m->path_goal_z);
    return 1;
}

static void passive_village_update_reputation_from_attack(PassiveMob *m) {
    if (!m || m->village_id < 0) return;
    PexVillageRuntime *v = passive_village_by_id(m->village_id);
    if (!v) return;
    v->player_reputation -= (m->type == PASSIVE_MOB_VILLAGER) ? 1 : 5;
    if (v->player_reputation < -30) v->player_reputation = -30;
}


static void passive_mobs_try_village_spawns_exact(void) {
    if (g_mp_connected || (g_ingame_ticks % 200) != 0) return;
    passive_villages_refresh();
    for (int vi = 0; vi < PEX_MAX_RUNTIME_VILLAGES; ++vi) {
        PexVillageRuntime *v = &g_passive_villages[vi];
        if (!v->active || v->door_count <= 0) continue;
        int desired_villagers = (int)((float)v->door_count * 0.35f);
        if (desired_villagers < 2) desired_villagers = 2;
        if (desired_villagers > 20) desired_villagers = 20;
        for (int tries = 0; v->villager_count < desired_villagers && tries < v->door_count * 2; ++tries) {
            PexVillageDoorRuntime *d = &v->doors[rand() % v->door_count];
            int x = d->x + (rand() % 9) - 4;
            int z = d->z + (rand() % 9) - 4;
            int y = passive_find_surface_for_village_spawn(x, z);
            if (y == -9999) continue;
            PassiveMob *m = passive_mob_alloc();
            if (!m) return;
            passive_mob_init(m, PASSIVE_MOB_VILLAGER, (float)x + 0.5f, (float)y, (float)z + 0.5f);
            m->village_id = v->id;
            ++v->villager_count;
            g_save_dirty = 1;
        }
        /* Village.tick: numIronGolems < numVillagers/16, door count > 20,
           and rand.nextInt(7000)==0.  Keep the same probability and the
           tryGetIronGolemSpawningLocation 10-attempt box. */
        int desired_golems = v->villager_count / 16;
        if (v->golem_count < desired_golems && v->door_count > 20 && (rand() % 7000) == 0) {
            for (int tries = 0; tries < 10; ++tries) {
                int x = v->cx + (rand() % 16) - 8;
                int y = v->cy + (rand() % 6) - 3;
                int z = v->cz + (rand() % 16) - 8;
                int dx = x - v->cx, dy = y - v->cy, dz = z - v->cz;
                if (dx*dx + dy*dy + dz*dz >= v->radius * v->radius) continue;
                if (!flat_block_is_solid(flat_get_block(x, y - 1, z))) continue;
                int blocked = 0;
                for (int xx = x - 1; xx < x + 1 && !blocked; ++xx)
                    for (int yy = y; yy < y + 4 && !blocked; ++yy)
                        for (int zz = z - 1; zz < z + 1 && !blocked; ++zz)
                            if (flat_block_is_solid(flat_get_block(xx, yy, zz))) blocked = 1;
                if (blocked) continue;
                PassiveMob *g = passive_mob_alloc();
                if (!g) return;
                passive_mob_init(g, PASSIVE_MOB_IRON_GOLEM, (float)x + 0.5f, (float)y, (float)z + 0.5f);
                g->village_id = v->id;
                ++v->golem_count;
                g_save_dirty = 1;
                break;
            }
        }
    }
}

static void passive_mob_assign_owner(PassiveMob *m) {
    if (!m) return;
    m->owner_id = PEX_MOB_OWNER_SINGLEPLAYER;
    m->tame_state = 1;
    if (m->type == PASSIVE_MOB_WOLF) m->collar_color = 14;
}

static int passive_mob_is_owned_by_player(const PassiveMob *m) {
    return m && m->owner_id == PEX_MOB_OWNER_SINGLEPLAYER;
}

static int passive_mob_teleport_near_player_125(PassiveMob *m) {
    if (!m || !passive_mob_is_owned_by_player(m)) return 0;
    int base_x = (int)floorf(g_player_x) - 2;
    int base_y = (int)floorf(g_player_y - 1.62f);
    int base_z = (int)floorf(g_player_z) - 2;
    for (int ox = 0; ox <= 4; ++ox) {
        for (int oz = 0; oz <= 4; ++oz) {
            if (!(ox < 1 || oz < 1 || ox > 3 || oz > 3)) continue;
            int x = base_x + ox;
            int z = base_z + oz;
            int y = base_y;
            if (!passive_path_can_stand_at(m->type, x, y, z)) continue;
            m->prev_x = m->x = (float)x + 0.5f;
            m->prev_y = m->y = (float)y;
            m->prev_z = m->z = (float)z + 0.5f;
            m->mx = m->my = m->mz = 0.0f;
            passive_path_clear(m);
            g_save_dirty = 1;
            return 1;
        }
    }
    return 0;
}

static int passive_tameable_follow_owner_125(PassiveMob *m) {
    if (!m || !passive_mob_is_owned_by_player(m) || m->sitting) return 0;
    if (m->type != PASSIVE_MOB_WOLF && m->type != PASSIVE_MOB_OCELOT) return 0;
    float dx = g_player_x - m->x;
    float dy = (g_player_y - 1.62f) - m->y;
    float dz = g_player_z - m->z;
    float d2 = dx * dx + dy * dy + dz * dz;
    float min_start = 10.0f;
    float max_continue = (m->type == PASSIVE_MOB_OCELOT) ? 5.0f : 2.0f;
    int already_following = m->has_path_target &&
        fabsf(m->path_goal_x - g_player_x) < 4.0f &&
        fabsf(m->path_goal_z - g_player_z) < 4.0f;
    if (d2 >= 144.0f) {
        if (passive_mob_teleport_near_player_125(m)) return 1;
    }
    if (d2 < max_continue * max_continue) {
        if (already_following) passive_path_clear(m);
        return 0;
    }
    if (d2 < min_start * min_start && !already_following) return 0;
    passive_mob_set_path_goal(m, g_player_x, g_player_y - 1.62f, g_player_z);
    if (m->ai_repath_delay <= 0) {
        m->path_recalc_cooldown = 0;
        m->ai_repath_delay = 10;
    }
    m->ai_task_mask |= PEX_AI_FOLLOW_OWNER;
    return 1;
}

static int passive_zombie_find_closed_wood_door_near_125(PassiveMob *m, int *out_x, int *out_y, int *out_z) {
    if (!m || m->type != PASSIVE_MOB_ZOMBIE) return 0;
    int mx = (int)floorf(m->x);
    int my = (int)floorf(m->y);
    int mz = (int)floorf(m->z);
    for (int y = my; y <= my + 1; ++y) {
        for (int z = mz - 1; z <= mz + 1; ++z) {
            for (int x = mx - 1; x <= mx + 1; ++x) {
                if (flat_get_block(x, y, z) != BLOCK_WOOD_DOOR) continue;
                int ly = door_lower_y_at(x, y, z);
                if (door_is_open_at(x, ly, z)) continue;
                float dx = ((float)x + 0.5f) - m->x;
                float dy = ((float)ly + 0.5f) - (m->y + 0.5f);
                float dz = ((float)z + 0.5f) - m->z;
                if (dx * dx + dy * dy + dz * dz >= 4.0f) continue;
                if (out_x) *out_x = x;
                if (out_y) *out_y = ly;
                if (out_z) *out_z = z;
                return 1;
            }
        }
    }
    return 0;
}

static int passive_zombie_tick_break_door_125(PassiveMob *m) {
    if (!m || m->type != PASSIVE_MOB_ZOMBIE || m->death_time > 0) return 0;
    int x = 0, y = 0, z = 0;
    if (!passive_zombie_find_closed_wood_door_near_125(m, &x, &y, &z)) {
        m->door_open_time = 0;
        return 0;
    }
    if (m->door_open_time <= 0 || m->door_target_x != x || m->door_target_y != y || m->door_target_z != z) {
        m->door_target_x = x; m->door_target_y = y; m->door_target_z = z;
        m->door_open_time = 240;
    }
    m->ai_task_mask |= PEX_AI_BREAK_DOOR;
    if ((rand() % 20) == 0) {
        pex_sound_play_at("random.wood_click", (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, 1.0f, 1.0f);
    }
    if (m->door_open_time <= 1) {
        if ((g_opts.difficulty & 3) == 3) {
            door_break_at(x, y, z, 0);
            pex_sound_play_at("random.break", (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, 1.0f, 1.0f);
            g_save_dirty = 1;
        }
        m->door_open_time = 0;
    }
    return 1;
}




static void passive_dragon_part_box_125(const PassiveMob *d, int part, FlatAABB *b) {
    float yaw = d ? d->yaw * 0.0174532925f : 0.0f;
    float sn = sinf(yaw), cs = cosf(yaw);
    float x = d->x, y = d->y, z = d->z;
    float w = 4.0f, h = 4.0f;
    switch (part) {
        case 0: /* head */ x += sn * 5.5f; z -= cs * 5.5f; y += 1.8f; w = h = 3.0f; break;
        case 1: /* body */ x += sn * 0.5f; z -= cs * 0.5f; y += 0.5f; w = 5.0f; h = 3.0f; break;
        case 2: /* left wing */ x += cs * 4.5f; z += sn * 4.5f; y += 2.0f; w = 4.0f; h = 2.0f; break;
        case 3: /* right wing */ x -= cs * 4.5f; z -= sn * 4.5f; y += 2.0f; w = 4.0f; h = 3.0f; break;
        case 4: x -= sn * 6.0f; z += cs * 6.0f; y += 0.5f; w = h = 2.0f; break;
        case 5: x -= sn * 10.0f; z += cs * 10.0f; y += 0.2f; w = h = 2.0f; break;
        default: x -= sn * 14.0f; z += cs * 14.0f; y += 0.0f; w = h = 2.0f; break;
    }
    b->minx = x - w * 0.5f; b->maxx = x + w * 0.5f;
    b->miny = y;            b->maxy = y + h;
    b->minz = z - w * 0.5f; b->maxz = z + w * 0.5f;
}

static int passive_dragon_player_part_damage_125(const PassiveMob *dragon, int base_damage) {
    if (!dragon || base_damage <= 0) return base_damage;
    float lx, ly, lz;
    pex_touch_aware_look_vector(&lx, &ly, &lz);
    float best_t = 5.0f;
    int best_part = 1;
    for (int part = 0; part < 7; ++part) {
        FlatAABB b;
        passive_dragon_part_box_125(dragon, part, &b);
        float t = 0.0f;
        if (!passive_ray_intersect_aabb(&b, g_player_x, g_player_y, g_player_z, lx, ly, lz, 5.0f, &t)) continue;
        if (t >= 0.0f && t < best_t) { best_t = t; best_part = part; }
    }
    /* EntityDragon.attackEntityFromPart: non-head parts do var3 / 4 + 1. */
    if (best_part != 0) return base_damage / 4 + 1;
    return base_damage;
}

/* EntityEnderCrystal / EntityDragonPart gameplay port layer.
   These are kept as C runtime objects instead of PassiveMob entries so they can
   behave like Java entities without consuming normal mob slots. */
static int passive_dragon_find_index_125(const PassiveMob *dragon) {
    if (!dragon) return -1;
    long idx = (long)(dragon - g_passive_mobs);
    return (idx >= 0 && idx < MAX_PASSIVE_MOBS) ? (int)idx : -1;
}

static int passive_dragon_crystal_top_y_125(int x, int z) {
    int best = 76;
    for (int y = FLAT_WORLD_Y_MAX - 2; y >= FLAT_WORLD_Y_MIN + 1; --y) {
        if (flat_get_block(x, y, z) == BLOCK_OBSIDIAN) { best = y + 1; break; }
    }
    if (best < FLAT_WORLD_Y_MIN + 2) best = FLAT_WORLD_Y_MIN + 2;
    if (best > FLAT_WORLD_Y_MAX - 2) best = FLAT_WORLD_Y_MAX - 2;
    return best;
}

static void passive_dragon_crystals_seed_125(void) {
    if (g_dragon_crystals_seeded_125 || g_current_dimension != PEX_DIM_END || g_passive_dragon_killed_125) return;
    static const int pos[10][2] = {
        { 43,  0}, { 30,  30}, {  0,  43}, {-30,  30}, {-43,   0},
        {-30, -30}, {  0, -43}, { 30, -30}, { 18,  54}, {-54, -18}
    };
    memset(g_dragon_crystals_125, 0, sizeof(g_dragon_crystals_125));
    for (int i = 0; i < 10 && i < PEX_DRAGON_CRYSTAL_MAX_125; ++i) {
        PexDragonCrystalRuntime125 *c = &g_dragon_crystals_125[i];
        c->active = 1;
        c->x = pos[i][0];
        c->z = pos[i][1];
        c->y = passive_dragon_crystal_top_y_125(c->x, c->z);
        c->px = (float)c->x + 0.5f;
        c->py = (float)c->y;
        c->pz = (float)c->z + 0.5f;
        c->health = 5;
        c->inner_rotation = rand() % 100000;
        c->beam_dragon_index = -1;
        if (flat_get_block(c->x, c->y, c->z) == 0) flat_set_block(c->x, c->y, c->z, BLOCK_FIRE);
    }
    g_dragon_crystals_seeded_125 = 1;
    g_save_dirty = 1;
}

static int passive_dragon_crystal_count_active_125(void) {
    int n = 0;
    for (int i = 0; i < PEX_DRAGON_CRYSTAL_MAX_125; ++i) if (g_dragon_crystals_125[i].active) ++n;
    return n;
}

static PexDragonCrystalRuntime125 *passive_dragon_nearest_crystal_125(PassiveMob *dragon, float range) {
    if (!dragon) return NULL;
    PexDragonCrystalRuntime125 *best = NULL;
    float best_d2 = range * range;
    for (int i = 0; i < PEX_DRAGON_CRYSTAL_MAX_125; ++i) {
        PexDragonCrystalRuntime125 *c = &g_dragon_crystals_125[i];
        if (!c->active || c->health <= 0) continue;
        float dx = c->px - dragon->x;
        float dy = c->py - (dragon->y + dragon->height * 0.5f);
        float dz = c->pz - dragon->z;
        float d2 = dx*dx + dy*dy + dz*dz;
        if (d2 < best_d2) { best_d2 = d2; best = c; }
    }
    return best;
}

static void passive_dragon_crystal_explosion_damage_125(float x, float y, float z, PassiveMob *linked_dragon) {
    const float radius = 6.0f;
    int cx = (int)floorf(x), cy = (int)floorf(y), cz = (int)floorf(z);
    for (int by = cy - 6; by <= cy + 6; ++by) {
        for (int bz = cz - 6; bz <= cz + 6; ++bz) {
            for (int bx = cx - 6; bx <= cx + 6; ++bx) {
                float dx = ((float)bx + 0.5f) - x;
                float dy = ((float)by + 0.5f) - y;
                float dz = ((float)bz + 0.5f) - z;
                float d = sqrtf(dx*dx + dy*dy + dz*dz);
                if (d > radius) continue;
                int id = flat_get_block(bx, by, bz);
                if (!passive_explosion_breaks_block(id)) continue;
                if (pex_rand_float01() < (1.0f - d / (radius + 0.001f))) {
                    spawn_block_destroy_particles(bx, by, bz, id);
                    flat_set_block(bx, by, bz, 0);
                }
            }
        }
    }
    if (!g_player_dead && g_player_health > 0) {
        float dx = g_player_x - x;
        float dy = (g_player_y - 1.62f) - y;
        float dz = g_player_z - z;
        float d = sqrtf(dx*dx + dy*dy + dz*dz);
        if (d <= radius * 2.0f) {
            int dmg = (int)((1.0f - d / (radius * 2.0f)) * 85.0f + 1.0f);
            if (dmg > 0) (void)player_attack_entity_from(pex_damage_source_simple(PEX_DAMAGE_EXPLOSION), dmg);
        }
    }
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        PassiveMob *m = &g_passive_mobs[i];
        if (!m->active || m->death_time > 0) continue;
        float dx = m->x - x;
        float dy = (m->y + m->height * 0.5f) - y;
        float dz = m->z - z;
        float d = sqrtf(dx*dx + dy*dy + dz*dz);
        if (d > radius * 2.0f) continue;
        int dmg = (int)((1.0f - d / (radius * 2.0f)) * 85.0f + 1.0f);
        if (dmg <= 0) continue;
        PexDamageSource src = pex_damage_source_simple(PEX_DAMAGE_EXPLOSION);
        (void)pex_mob_attack_entity_from(m, src, dmg);
    }
    if (linked_dragon && linked_dragon->active && linked_dragon->death_time <= 0) {
        PexDamageSource src = pex_damage_source_simple(PEX_DAMAGE_EXPLOSION);
        (void)pex_mob_attack_entity_from(linked_dragon, src, 10);
    }
}

static void passive_dragon_crystal_destroy_125(PexDragonCrystalRuntime125 *c, PassiveMob *linked_dragon) {
    if (!c || !c->active) return;
    c->health = 0;
    c->active = 0;
    if (flat_get_block(c->x, c->y, c->z) == BLOCK_FIRE) flat_set_block(c->x, c->y, c->z, 0);
    pex_sound_play_at("random.explode", c->px, c->py, c->pz, 6.0f, 1.0f);
    for (int i = 0; i < 32; ++i) add_splash_particle(c->px, c->py + 0.8f, c->pz,
                                                     (pex_rand_float01() - 0.5f) * 0.9f,
                                                     (pex_rand_float01() - 0.2f) * 0.9f,
                                                     (pex_rand_float01() - 0.5f) * 0.9f);
    passive_dragon_crystal_explosion_damage_125(c->px, c->py, c->pz, linked_dragon);
    g_save_dirty = 1;
}

static void passive_dragon_crystals_tick_125(PassiveMob *dragon) {
    passive_dragon_crystals_seed_125();
    if (!dragon || dragon->death_time > 0) return;
    int dragon_index = passive_dragon_find_index_125(dragon);
    for (int i = 0; i < PEX_DRAGON_CRYSTAL_MAX_125; ++i) {
        PexDragonCrystalRuntime125 *c = &g_dragon_crystals_125[i];
        if (!c->active) continue;
        ++c->inner_rotation;
        c->beam_dragon_index = -1;
        if (flat_get_block(c->x, c->y, c->z) != BLOCK_FIRE && flat_get_block(c->x, c->y, c->z) == 0) flat_set_block(c->x, c->y, c->z, BLOCK_FIRE);
        if ((g_ingame_ticks & 7) == 0) add_splash_particle(c->px, c->py + 0.6f, c->pz, 0.0f, 0.04f, 0.0f);
        (void)dragon_index;
    }
    PexDragonCrystalRuntime125 *heal = passive_dragon_nearest_crystal_125(dragon, 32.0f);
    if (heal) {
        heal->beam_dragon_index = dragon_index;
        if ((dragon->age % 10) == 0 && dragon->health < passive_mob_max_health_current_125(dragon)) {
            dragon->health += 1;
            if (dragon->health > passive_mob_max_health_current_125(dragon)) dragon->health = passive_mob_max_health_current_125(dragon);
        }
    }
}

static int passive_dragon_crystal_segment_hit_125(float x0, float y0, float z0,
                                                  float x1, float y1, float z1,
                                                  float max_t, float *out_t,
                                                  PexDragonCrystalRuntime125 **out_crystal) {
    float dx = x1 - x0, dy = y1 - y0, dz = z1 - z0;
    float best = max_t;
    PexDragonCrystalRuntime125 *best_c = NULL;
    for (int i = 0; i < PEX_DRAGON_CRYSTAL_MAX_125; ++i) {
        PexDragonCrystalRuntime125 *c = &g_dragon_crystals_125[i];
        if (!c->active || c->health <= 0) continue;
        FlatAABB b;
        b.minx = c->px - 1.0f; b.maxx = c->px + 1.0f;
        b.miny = c->py;        b.maxy = c->py + 2.0f;
        b.minz = c->pz - 1.0f; b.maxz = c->pz + 1.0f;
        float t = 0.0f;
        if (!passive_ray_intersect_aabb(&b, x0, y0, z0, dx, dy, dz, max_t, &t)) continue;
        if (t >= 0.0f && t < best) { best = t; best_c = c; }
    }
    if (!best_c) return 0;
    if (out_t) *out_t = best;
    if (out_crystal) *out_crystal = best_c;
    return 1;
}

static int passive_dragon_crystal_player_attack_125(float max_dist, float mob_t) {
    if (g_current_dimension != PEX_DIM_END || passive_dragon_crystal_count_active_125() <= 0) return 0;
    float dx, dy, dz;
    pex_touch_aware_look_vector(&dx, &dy, &dz);
    PexDragonCrystalRuntime125 *c = NULL;
    float t = 1.0f;
    if (!passive_dragon_crystal_segment_hit_125(g_player_x, g_player_y, g_player_z,
                                                g_player_x + dx * max_dist, g_player_y + dy * max_dist, g_player_z + dz * max_dist,
                                                1.0f, &t, &c)) return 0;
    float crystal_dist = t * max_dist;
    if (crystal_dist > mob_t + 0.05f) return 0;
    FlatRayHit block = flat_raycast();
    if (block.hit) {
        float bx = block.hx - g_player_x;
        float by = block.hy - g_player_y;
        float bz = block.hz - g_player_z;
        float block_dist = sqrtf(bx*bx + by*by + bz*bz);
        if (block_dist + 0.20f < crystal_dist) return 0;
    }
    PassiveMob *dragon = NULL;
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        PassiveMob *m = &g_passive_mobs[i];
        if (m->active && m->type == PASSIVE_MOB_ENDER_DRAGON && m->death_time <= 0) { dragon = m; break; }
    }
    restart_hand_swing();
    passive_dragon_crystal_destroy_125(c, dragon);
    return 1;
}

static void passive_dragon_crystals_draw_125(float partial) {
    (void)partial;
    if (g_current_dimension != PEX_DIM_END || !tex_terrain.id) return;
    for (int i = 0; i < PEX_DRAGON_CRYSTAL_MAX_125; ++i) {
        PexDragonCrystalRuntime125 *c = &g_dragon_crystals_125[i];
        if (!c->active) continue;
        float dx = c->px - g_player_x, dy = c->py - g_player_y, dz = c->pz - g_player_z;
        if (dx*dx + dy*dy + dz*dz > PEX_PASSIVE_RENDER_DIST * PEX_PASSIVE_RENDER_DIST) continue;
        glPushMatrix();
        glTranslatef(c->px, c->py + 1.0f, c->pz);
        glRotatef((float)(c->inner_rotation & 1023) * (360.0f / 1024.0f), 0.0f, 1.0f, 0.0f);
        glScalef(0.75f, 0.75f, 0.75f);
        draw_block_item_model(BLOCK_GLASS, 0.0f, 0.0f, 0.0f);
        glScalef(0.65f, 0.65f, 0.65f);
        glRotatef(60.0f, 1.0f, 0.0f, 0.0f);
        draw_block_item_model(BLOCK_GLASS, 0.0f, 0.0f, 0.0f);
        glPopMatrix();
        if (c->beam_dragon_index >= 0 && c->beam_dragon_index < MAX_PASSIVE_MOBS) {
            PassiveMob *d = &g_passive_mobs[c->beam_dragon_index];
            if (d->active && d->type == PASSIVE_MOB_ENDER_DRAGON) {
                for (int k = 0; k < 3; ++k) {
                    float t = (float)(k + 1) / 4.0f;
                    add_splash_particle(c->px + (d->x - c->px) * t,
                                        c->py + 1.0f + ((d->y + d->height * 0.5f) - (c->py + 1.0f)) * t,
                                        c->pz + (d->z - c->pz) * t,
                                        0.0f, 0.02f, 0.0f);
                }
            }
        }
    }
}

static void passive_dragon_crystal_state_write_binary_125(FILE *f) {
    char magic[4] = {'P','D','C','R'};
    int version = 1;
    fwrite(magic, 1, 4, f);
    fwrite(&version, sizeof(version), 1, f);
    fwrite(&g_dragon_crystals_seeded_125, sizeof(g_dragon_crystals_seeded_125), 1, f);
    int count = 0;
    for (int i = 0; i < PEX_DRAGON_CRYSTAL_MAX_125; ++i) if (g_dragon_crystals_125[i].active) ++count;
    fwrite(&count, sizeof(count), 1, f);
    for (int i = 0; i < PEX_DRAGON_CRYSTAL_MAX_125; ++i) {
        PexDragonCrystalRuntime125 *c = &g_dragon_crystals_125[i];
        if (!c->active) continue;
        fwrite(&c->x, sizeof(int), 1, f);
        fwrite(&c->y, sizeof(int), 1, f);
        fwrite(&c->z, sizeof(int), 1, f);
        fwrite(&c->px, sizeof(float), 1, f);
        fwrite(&c->py, sizeof(float), 1, f);
        fwrite(&c->pz, sizeof(float), 1, f);
        fwrite(&c->health, sizeof(int), 1, f);
        fwrite(&c->inner_rotation, sizeof(int), 1, f);
    }
}

static void passive_dragon_crystal_state_read_binary_125(FILE *f, int save_version) {
    memset(g_dragon_crystals_125, 0, sizeof(g_dragon_crystals_125));
    g_dragon_crystals_seeded_125 = 0;
    if (save_version < 32) return;
    char magic[4];
    int version = 0, seeded = 0, count = 0;
    if (fread(magic, 1, 4, f) != 4) return;
    if (memcmp(magic, "PDCR", 4) != 0) return;
    if (fread(&version, sizeof(version), 1, f) != 1 ||
        fread(&seeded, sizeof(seeded), 1, f) != 1 ||
        fread(&count, sizeof(count), 1, f) != 1) return;
    if (count < 0) count = 0;
    if (count > PEX_DRAGON_CRYSTAL_MAX_125) count = PEX_DRAGON_CRYSTAL_MAX_125;
    g_dragon_crystals_seeded_125 = seeded ? 1 : 0;
    for (int i = 0; i < count; ++i) {
        PexDragonCrystalRuntime125 tmp;
        memset(&tmp, 0, sizeof(tmp));
        if (fread(&tmp.x, sizeof(int), 1, f) != 1 ||
            fread(&tmp.y, sizeof(int), 1, f) != 1 ||
            fread(&tmp.z, sizeof(int), 1, f) != 1 ||
            fread(&tmp.px, sizeof(float), 1, f) != 1 ||
            fread(&tmp.py, sizeof(float), 1, f) != 1 ||
            fread(&tmp.pz, sizeof(float), 1, f) != 1 ||
            fread(&tmp.health, sizeof(int), 1, f) != 1 ||
            fread(&tmp.inner_rotation, sizeof(int), 1, f) != 1) {
            memset(g_dragon_crystals_125, 0, sizeof(g_dragon_crystals_125));
            g_dragon_crystals_seeded_125 = 0;
            return;
        }
        if (!isfinite(tmp.px) || !isfinite(tmp.py) || !isfinite(tmp.pz) || tmp.health <= 0) continue;
        tmp.active = 1;
        tmp.beam_dragon_index = -1;
        g_dragon_crystals_125[i] = tmp;
    }
    (void)version;
}

static int pex_dragon_crystal_projectile_hit_125(const FlatProjectile *p, float x0, float y0, float z0,
                                                  float x1, float y1, float z1, float max_t, float *out_t,
                                                  float *out_x, float *out_y, float *out_z) {
    (void)p;
    if (g_current_dimension != PEX_DIM_END || passive_dragon_crystal_count_active_125() <= 0) return 0;
    PexDragonCrystalRuntime125 *c = NULL;
    float t = 1.0f;
    if (!passive_dragon_crystal_segment_hit_125(x0, y0, z0, x1, y1, z1, max_t, &t, &c)) return 0;
    PassiveMob *dragon = NULL;
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        PassiveMob *m = &g_passive_mobs[i];
        if (m->active && m->type == PASSIVE_MOB_ENDER_DRAGON && m->death_time <= 0) { dragon = m; break; }
    }
    if (out_t) *out_t = t;
    if (out_x) *out_x = x0 + (x1 - x0) * t;
    if (out_y) *out_y = y0 + (y1 - y0) * t;
    if (out_z) *out_z = z0 + (z1 - z0) * t;
    passive_dragon_crystal_destroy_125(c, dragon);
    return 1;
}

/* EntityDragon / WorldProviderEnd port layer.
   This is intentionally C-native: the Java dragon uses a multipart entity,
   ring buffer path history, and end-crystal entities. PexCraft does not yet
   have those engine objects, so this ports the gameplay lifecycle and flight
   target behavior into the existing PassiveMob runtime while keeping the
   missing crystal/multipart hooks isolated. */
#define PEX_DRAGON_DEATH_TICKS_125 200

static int passive_dragon_exists_125(void) {
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        PassiveMob *m = &g_passive_mobs[i];
        if (m->active && m->type == PASSIVE_MOB_ENDER_DRAGON && m->death_time <= 0) return 1;
    }
    return 0;
}

static void passive_dragon_pick_new_target_125(PassiveMob *m) {
    if (!m) return;
    float center_x = 0.0f;
    float center_z = 0.0f;
    if (fabsf(g_player_x) < 192.0f && fabsf(g_player_z) < 192.0f) {
        center_x = g_player_x * 0.35f;
        center_z = g_player_z * 0.35f;
    }
    float angle = pex_rand_float01() * 6.2831853f;
    float radius = 24.0f + pex_rand_float01() * 48.0f;
    m->target_x = center_x + cosf(angle) * radius;
    m->target_y = 68.0f + pex_rand_float01() * 32.0f;
    m->target_z = center_z + sinf(angle) * radius;
    m->has_path_target = 1;
}

static void passive_dragon_spawn_xp_125(PassiveMob *m, int total) {
    if (!m || total <= 0) return;
    while (total > 0) {
        int v = pex_xp_orb_split_value(total);
        total -= v;
        pex_spawn_xp_orb(m->x, m->y, m->z, v);
    }
}

static void passive_dragon_create_exit_portal_125(PassiveMob *m) {
    if (!m) return;
    /* EntityDragon.createEnderPortal(floor(posX), floor(posZ)).  The Java
       method uses a fixed y=64 bedrock/end-portal disk with a central bedrock
       pillar, four torches, and the dragon egg. */
    int cx = (int)floorf(m->x);
    int cz = (int)floorf(m->z);
    const int cy = 64;
    const int r = 4;
    for (int y = cy - 1; y <= cy + 32; ++y) {
        for (int z = cz - r; z <= cz + r; ++z) {
            for (int x = cx - r; x <= cx + r; ++x) {
                double dx = (double)(x - cx);
                double dz = (double)(z - cz);
                double dist = sqrt(dx * dx + dz * dz);
                if (dist > (double)r - 0.5) continue;
                if (y < cy) {
                    if (dist <= (double)(r - 1) - 0.5) flat_set_block(x, y, z, BLOCK_BEDROCK);
                } else if (y > cy) {
                    flat_set_block(x, y, z, 0);
                } else if (dist > (double)(r - 1) - 0.5) {
                    flat_set_block(x, y, z, BLOCK_BEDROCK);
                } else {
                    flat_set_block(x, y, z, BLOCK_END_PORTAL);
                }
            }
        }
    }
    flat_set_block(cx, cy + 0, cz, BLOCK_BEDROCK);
    flat_set_block(cx, cy + 1, cz, BLOCK_BEDROCK);
    flat_set_block(cx, cy + 2, cz, BLOCK_BEDROCK);
    flat_set_block(cx - 1, cy + 2, cz, BLOCK_TORCH);
    flat_set_block(cx + 1, cy + 2, cz, BLOCK_TORCH);
    flat_set_block(cx, cy + 2, cz - 1, BLOCK_TORCH);
    flat_set_block(cx, cy + 2, cz + 1, BLOCK_TORCH);
    flat_set_block(cx, cy + 3, cz, BLOCK_BEDROCK);
    flat_set_block(cx, cy + 4, cz, BLOCK_DRAGON_EGG);
    spawn_block_destroy_particles(cx, cy, cz, BLOCK_END_PORTAL);
    g_save_dirty = 1;
}

static int passive_dragon_player_close_125(const PassiveMob *m, float range) {
    if (!m || g_player_dead || g_player_health <= 0) return 0;
    float dx = g_player_x - m->x;
    float dy = (g_player_y - 1.62f) - (m->y + m->height * 0.45f);
    float dz = g_player_z - m->z;
    return dx*dx + dy*dy + dz*dz <= range * range;
}

static void passive_dragon_tick_living_125(PassiveMob *m) {
    if (!m || !m->active || m->type != PASSIVE_MOB_ENDER_DRAGON) return;
    passive_dragon_crystals_tick_125(m);
    if (!m->has_path_target || (m->age % 80) == 0) passive_dragon_pick_new_target_125(m);
    float dx = m->target_x - m->x;
    float dy = m->target_y - m->y;
    float dz = m->target_z - m->z;
    float dist = sqrtf(dx*dx + dy*dy + dz*dz);
    if (dist < 6.0f || dist > 160.0f) {
        passive_dragon_pick_new_target_125(m);
        dx = m->target_x - m->x;
        dy = m->target_y - m->y;
        dz = m->target_z - m->z;
        dist = sqrtf(dx*dx + dy*dy + dz*dz);
    }
    if (dist > 0.001f) {
        float speed = 0.10f + pex_clamp_float(dist / 120.0f, 0.0f, 0.25f);
        m->mx += (dx / dist) * speed * 0.18f;
        m->my += (dy / dist) * speed * 0.12f;
        m->mz += (dz / dist) * speed * 0.18f;
    }
    float hspeed = sqrtf(m->mx*m->mx + m->mz*m->mz);
    if (hspeed > 0.001f) {
        float yaw = atan2f(m->mz, m->mx) * 57.29578f - 90.0f;
        m->yaw += pex_wrap_degrees(yaw - m->yaw) * 0.18f;
        m->render_yaw = m->yaw;
    }
    m->mx *= 0.91f;
    m->my *= 0.91f;
    m->mz *= 0.91f;
    if (m->mx > 1.2f) m->mx = 1.2f; if (m->mx < -1.2f) m->mx = -1.2f;
    if (m->my > 0.8f) m->my = 0.8f; if (m->my < -0.8f) m->my = -0.8f;
    if (m->mz > 1.2f) m->mz = 1.2f; if (m->mz < -1.2f) m->mz = -1.2f;
    m->x += m->mx;
    m->y += m->my;
    m->z += m->mz;
    if (m->y < 48.0f) { m->y = 48.0f; m->my = fabsf(m->my) * 0.5f; }
    if (m->y > 120.0f) { m->y = 120.0f; m->my = -fabsf(m->my) * 0.5f; }

    /* Functional port of the multipart body collision damage: the current engine
       has one broad dragon AABB, so the attack window is intentionally the Java
       body/head contact distance rather than a zombie-style reach. */
    if (passive_dragon_player_close_125(m, 8.0f) && m->attack_time <= 0) {
        (void)player_attack_entity_from(pex_damage_source_mob(m), passive_mob_scaled_attack_damage(m, 10));
        m->attack_time = 10;
    }
    if ((m->age % 20) == 0 && passive_dragon_player_close_125(m, 64.0f)) {
        pex_sound_play_at("mob.enderdragon.growl", m->x, m->y, m->z, 5.0f, 1.0f);
    }
    m->limb_swing += 0.28f;
    m->limb_amount = 1.0f;
    m->prev_render_yaw = m->render_yaw;
}

static int passive_dragon_tick_death_125(PassiveMob *m) {
    if (!m || m->type != PASSIVE_MOB_ENDER_DRAGON || m->death_time <= 0) return 0;
    ++m->death_time;
    m->mx *= 0.80f;
    m->my = 0.05f;
    m->mz *= 0.80f;
    m->y += m->my;
    m->limb_swing += 0.35f;
    if ((m->death_time % 10) == 0) {
        for (int i = 0; i < 12; ++i) add_splash_particle(m->x, m->y + m->height * 0.5f, m->z,
                                                         (pex_rand_float01() - 0.5f) * 0.8f,
                                                         (pex_rand_float01() - 0.2f) * 0.7f,
                                                         (pex_rand_float01() - 0.5f) * 0.8f);
        pex_sound_play_at("random.explode", m->x, m->y, m->z, 5.0f, 0.8f + pex_rand_float01() * 0.4f);
    }
    if (m->death_time > 150 && (m->death_time % 5) == 0 && m->death_time < PEX_DRAGON_DEATH_TICKS_125) {
        passive_dragon_spawn_xp_125(m, 1000);
    }
    if (m->death_time == PEX_DRAGON_DEATH_TICKS_125) {
        passive_dragon_spawn_xp_125(m, 10000);
        passive_dragon_create_exit_portal_125(m);
    }
    if (m->death_time > PEX_DRAGON_DEATH_TICKS_125) {
        g_passive_dragon_killed_125 = 1;
        m->active = 0;
        g_save_dirty = 1;
    }
    return 1;
}

static void passive_mobs_ensure_ender_dragon_125(void) {
    if (g_current_dimension != PEX_DIM_END || g_passive_dragon_killed_125) return;
    if (passive_dragon_exists_125()) return;
    PassiveMob *m = passive_mob_alloc();
    if (!m) return;
    passive_mob_init(m, PASSIVE_MOB_ENDER_DRAGON, 0.0f, 80.0f, 0.0f);
    passive_dragon_pick_new_target_125(m);
    pex_logf("passive mob: spawned Ender Dragon for End dimension");
    g_save_dirty = 1;
}


#define PEX_MOB_SPAWNER_CACHE_MAX 128

typedef struct PexMobSpawnerRuntime {
    int active;
    int x, y, z;
    int type;              /* Java TileEntityMobSpawner EntityId mapped to PassiveMob type. */
    int delay;             /* Java Delay tag. */
    int spawn_count;       /* Java 1.2.5 constant: 4 attempts. */
    int max_nearby;        /* Java 1.2.5 cap: 6 nearby entities. */
    int required_range;    /* Java 1.2.5 player activation range: 16. */
    int spawn_range;       /* Java 1.2.5 random horizontal range: 4. */
    int last_seen_tick;
} PexMobSpawnerRuntime;

static PexMobSpawnerRuntime g_passive_spawners[PEX_MOB_SPAWNER_CACHE_MAX];

static void passive_mob_spawner_remove_tile_125(int x, int y, int z) {
    for (int i = 0; i < PEX_MOB_SPAWNER_CACHE_MAX; ++i) {
        PexMobSpawnerRuntime *sp = &g_passive_spawners[i];
        if (sp->active && sp->x == x && sp->y == y && sp->z == z) memset(sp, 0, sizeof(*sp));
    }
}

static void passive_spawner_apply_java_defaults_125(PexMobSpawnerRuntime *sp) {
    if (!sp) return;
    if (sp->delay < -1 || sp->delay > 2000) sp->delay = 20;
    if (sp->spawn_count <= 0 || sp->spawn_count > 32) sp->spawn_count = 4;
    if (sp->max_nearby <= 0 || sp->max_nearby > 64) sp->max_nearby = 6;
    if (sp->required_range <= 0 || sp->required_range > 64) sp->required_range = 16;
    if (sp->spawn_range <= 0 || sp->spawn_range > 16) sp->spawn_range = 4;
}

static int passive_neighbor_block_within_125(int x, int y, int z, int id, int radius) {
    for (int oy = -radius; oy <= radius; ++oy) {
        for (int oz = -radius; oz <= radius; ++oz) {
            for (int ox = -radius; ox <= radius; ++ox) {
                if (flat_get_block(x + ox, y + oy, z + oz) == id) return 1;
            }
        }
    }
    return 0;
}

static int passive_spawner_type_from_context_125(int x, int y, int z) {
    if (passive_neighbor_block_within_125(x, y, z, BLOCK_NETHER_BRICK, 8)) return PASSIVE_MOB_BLAZE;
    if (passive_neighbor_block_within_125(x, y, z, BLOCK_WEB, 8) || passive_neighbor_block_within_125(x, y, z, BLOCK_RAILS, 8)) return PASSIVE_MOB_CAVE_SPIDER;
    if (passive_neighbor_block_within_125(x, y, z, BLOCK_END_PORTAL_FRAME, 6) || passive_neighbor_block_within_125(x, y, z, BLOCK_STONE_BRICK, 6)) return PASSIVE_MOB_SILVERFISH;
    if (passive_neighbor_block_within_125(x, y, z, BLOCK_COBBLESTONE, 8) || passive_neighbor_block_within_125(x, y, z, BLOCK_MOSSY_COBBLESTONE, 8)) {
        unsigned int h = (unsigned int)x * 73428767u ^ (unsigned int)y * 42317861u ^ (unsigned int)z * 91278319u ^ (unsigned int)g_world_seed;
        h ^= h >> 16; h *= 2246822519u; h ^= h >> 13;
        switch (h & 3u) {
            case 0: return PASSIVE_MOB_SKELETON;
            case 3: return PASSIVE_MOB_SPIDER;
            default: return PASSIVE_MOB_ZOMBIE;
        }
    }
    return PASSIVE_MOB_PIG; /* TileEntityMobSpawner default mobID is "Pig". */
}

static PexMobSpawnerRuntime *passive_spawner_runtime_125(int x, int y, int z) {
    PexMobSpawnerRuntime *free_slot = NULL;
    PexMobSpawnerRuntime *oldest = &g_passive_spawners[0];
    for (int i = 0; i < PEX_MOB_SPAWNER_CACHE_MAX; ++i) {
        PexMobSpawnerRuntime *sp = &g_passive_spawners[i];
        if (sp->active && sp->x == x && sp->y == y && sp->z == z) {
            sp->last_seen_tick = g_ingame_ticks;
            return sp;
        }
        if (!sp->active && !free_slot) free_slot = sp;
        if (sp->last_seen_tick < oldest->last_seen_tick) oldest = sp;
    }
    PexMobSpawnerRuntime *sp = free_slot ? free_slot : oldest;
    memset(sp, 0, sizeof(*sp));
    sp->active = 1;
    sp->x = x; sp->y = y; sp->z = z;
    /* New worlds should get this from TileEntityMobSpawner.EntityId.  Existing
       PexCraft chunks did not persist that tag into gameplay state, so this
       context probe is now only a one-time migration/default, not live logic. */
    sp->type = passive_spawner_type_from_context_125(x, y, z);
    sp->delay = 20; /* TileEntityMobSpawner ctor. */
    sp->spawn_count = 4;
    sp->max_nearby = 6;
    sp->required_range = 16;
    sp->spawn_range = 4;
    sp->last_seen_tick = g_ingame_ticks;
    passive_spawner_apply_java_defaults_125(sp);
    return sp;
}

static int passive_mob_spawner_set_entity_type_125(int x, int y, int z, int type) {
    if (!passive_mob_type_valid(type)) return 0;
    PexMobSpawnerRuntime *sp = passive_spawner_runtime_125(x, y, z);
    if (!sp) return 0;
    sp->type = type;
    passive_spawner_apply_java_defaults_125(sp);
    g_save_dirty = 1;
    return 1;
}

static void passive_spawner_update_delay_125(PexMobSpawnerRuntime *sp) {
    if (!sp) return;
    sp->delay = 200 + (rand() % 600);
}

static int passive_mob_count_type_near_box_125(int type, float cx, float cy, float cz, float rx, float ry, float rz) {
    int n = 0;
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        PassiveMob *m = &g_passive_mobs[i];
        if (!m->active || m->death_time > 0 || m->type != type) continue;
        if (fabsf(m->x - cx) <= rx && fabsf((m->y + m->height * 0.5f) - cy) <= ry && fabsf(m->z - cz) <= rz) ++n;
    }
    return n;
}

static int passive_spawner_can_spawn_mob_125(int type, float x, float y, float z) {
    if (!passive_mob_type_valid(type)) return 0;
    if (!passive_mob_has_spawn_clearance(type, x, y, z)) return 0;
    int bx = (int)floorf(x), by = (int)floorf(y), bz = (int)floorf(z);
    if (!flat_chunk_generated_at_block(bx, bz)) return 0;
    if (type == PASSIVE_MOB_SILVERFISH) {
        float dx = x - g_player_x, dy = y - (g_player_y - 1.62f), dz = z - g_player_z;
        if (dx*dx + dy*dy + dz*dz <= 5.0f * 5.0f) return 0;
        return 1;
    }
    if (type == PASSIVE_MOB_CAVE_SPIDER || type == PASSIVE_MOB_BLAZE) return 1;
    if (type == PASSIVE_MOB_PIG) {
        return passive_mob_air_spawn_base_ok(bx, by, bz) && flat_get_block(bx, by - 1, bz) == BLOCK_GRASS && passive_mob_has_spawn_light(bx, by, bz);
    }
    if (passive_mob_category(type) == PEX_CAT_MONSTER) {
        if ((g_opts.difficulty & 3) == 0) return 0;
        if (type == PASSIVE_MOB_SPIDER || type == PASSIVE_MOB_ZOMBIE || type == PASSIVE_MOB_SKELETON) return passive_mob_valid_dark_spawn_light(bx, by, bz);
    }
    return passive_mob_type_specific_can_spawn(type, bx, by, bz, passive_mob_category(type));
}

static void passive_spawner_tick_one_125(PexMobSpawnerRuntime *sp) {
    if (!sp || !sp->active) return;
    if (flat_get_block(sp->x, sp->y, sp->z) != BLOCK_MOB_SPAWNER) { sp->active = 0; return; }
    float sx0 = (float)sp->x + 0.5f;
    float sy0 = (float)sp->y + 0.5f;
    float sz0 = (float)sp->z + 0.5f;
    float pdx = sx0 - g_player_x, pdy = sy0 - (g_player_y - 1.62f), pdz = sz0 - g_player_z;
    passive_spawner_apply_java_defaults_125(sp);
    float req = (float)sp->required_range;
    if (pdx*pdx + pdy*pdy + pdz*pdz > req * req) return;
    /* TileEntityMobSpawner client branch emits smoke/flame here.  Reuse the
       existing block-hit particle pool to give visible active-spawner feedback. */
    if ((g_ingame_ticks & 7) == 0) spawn_block_hit_particle(sp->x, sp->y, sp->z, rand() % 6, BLOCK_MOB_SPAWNER);
    if (sp->delay == -1) passive_spawner_update_delay_125(sp);
    if (sp->delay > 0) { --sp->delay; return; }
    int type = sp->type;
    if (!passive_mob_type_valid(type)) { passive_spawner_update_delay_125(sp); return; }
    for (int attempt = 0; attempt < sp->spawn_count; ++attempt) {
        if (passive_mob_count_type_near_box_125(type, (float)sp->x + 0.5f, (float)sp->y + 0.5f, (float)sp->z + 0.5f, 8.0f, 4.0f, 8.0f) >= sp->max_nearby) {
            passive_spawner_update_delay_125(sp);
            return;
        }
        float sr = (float)sp->spawn_range;
        float x = (float)sp->x + 0.5f + (pex_rand_float01() - pex_rand_float01()) * sr;
        float y = (float)(sp->y + (rand() % 3) - 1);
        float z = (float)sp->z + 0.5f + (pex_rand_float01() - pex_rand_float01()) * sr;
        if (!passive_spawner_can_spawn_mob_125(type, x, y, z)) continue;
        PassiveMob *m = passive_mob_alloc();
        if (!m) { passive_spawner_update_delay_125(sp); return; }
        passive_mob_init(m, type, x, y, z);
        if (type == PASSIVE_MOB_SHEEP) m->fleece_color = passive_sheep_random_fleece_color();
        m->yaw = m->prev_yaw = pex_rand_float01() * 360.0f;
        m->render_yaw = m->prev_render_yaw = m->yaw;
        pex_sound_play_at("random.pop", (float)sp->x + 0.5f, (float)sp->y + 0.5f, (float)sp->z + 0.5f, 1.0f, 1.0f);
        spawn_block_destroy_particles((int)floorf(x), (int)floorf(y), (int)floorf(z), BLOCK_MOB_SPAWNER);
        passive_spawner_update_delay_125(sp);
        g_save_dirty = 1;
    }
}

static void passive_mobs_tick_spawners_125(void) {
    if (g_mp_connected || g_player_dead) return;
    int px = (int)floorf(g_player_x);
    int py = (int)floorf(g_player_y - 1.62f);
    int pz = (int)floorf(g_player_z);
    for (int y = py - 16; y <= py + 16; ++y) {
        if (y < FLAT_WORLD_Y_MIN || y > FLAT_WORLD_Y_MAX) continue;
        for (int z = pz - 16; z <= pz + 16; ++z) {
            for (int x = px - 16; x <= px + 16; ++x) {
                float dx = ((float)x + 0.5f) - g_player_x;
                float dy = ((float)y + 0.5f) - (g_player_y - 1.62f);
                float dz = ((float)z + 0.5f) - g_player_z;
                if (dx*dx + dy*dy + dz*dz > 16.0f * 16.0f) continue;
                if (flat_get_block(x, y, z) != BLOCK_MOB_SPAWNER) continue;
                passive_spawner_tick_one_125(passive_spawner_runtime_125(x, y, z));
            }
        }
    }
}


static void passive_spawners_write_binary_125(FILE *f) {
    char magic[4] = {'P','M','S','P'};
    fwrite(magic, 1, 4, f);
    int version = 2;
    fwrite(&version, sizeof(version), 1, f);
    int count = 0;
    for (int i = 0; i < PEX_MOB_SPAWNER_CACHE_MAX; ++i) {
        PexMobSpawnerRuntime *sp = &g_passive_spawners[i];
        if (!sp->active) continue;
        if (flat_get_block(sp->x, sp->y, sp->z) != BLOCK_MOB_SPAWNER) continue;
        if (!passive_mob_type_valid(sp->type)) continue;
        ++count;
    }
    fwrite(&count, sizeof(count), 1, f);
    for (int i = 0; i < PEX_MOB_SPAWNER_CACHE_MAX; ++i) {
        PexMobSpawnerRuntime *sp = &g_passive_spawners[i];
        if (!sp->active) continue;
        if (flat_get_block(sp->x, sp->y, sp->z) != BLOCK_MOB_SPAWNER) continue;
        if (!passive_mob_type_valid(sp->type)) continue;
        fwrite(&sp->x, sizeof(int), 1, f);
        fwrite(&sp->y, sizeof(int), 1, f);
        fwrite(&sp->z, sizeof(int), 1, f);
        fwrite(&sp->type, sizeof(int), 1, f);
        fwrite(&sp->delay, sizeof(int), 1, f);
        fwrite(&sp->spawn_count, sizeof(int), 1, f);
        fwrite(&sp->max_nearby, sizeof(int), 1, f);
        fwrite(&sp->required_range, sizeof(int), 1, f);
        fwrite(&sp->spawn_range, sizeof(int), 1, f);
        fwrite(&sp->last_seen_tick, sizeof(int), 1, f);
    }
}


static void passive_dragon_state_write_binary_125(FILE *f) {
    char magic[4] = {'P','D','R','G'};
    int version = 1;
    fwrite(magic, 1, 4, f);
    fwrite(&version, sizeof(version), 1, f);
    fwrite(&g_passive_dragon_killed_125, sizeof(g_passive_dragon_killed_125), 1, f);
}

static void passive_dragon_state_read_binary_125(FILE *f, int save_version) {
    g_passive_dragon_killed_125 = 0;
    if (save_version < 31) return;
    char magic[4];
    int version = 0;
    if (fread(magic, 1, 4, f) != 4) return;
    if (memcmp(magic, "PDRG", 4) != 0) return;
    if (fread(&version, sizeof(version), 1, f) != 1) return;
    if (fread(&g_passive_dragon_killed_125, sizeof(g_passive_dragon_killed_125), 1, f) != 1) g_passive_dragon_killed_125 = 0;
    g_passive_dragon_killed_125 = g_passive_dragon_killed_125 ? 1 : 0;
}

static void passive_spawners_read_binary_125(FILE *f, int save_version) {
    memset(g_passive_spawners, 0, sizeof(g_passive_spawners));
    if (save_version < 30) return;
    char magic[4];
    if (fread(magic, 1, 4, f) != 4) return;
    if (memcmp(magic, "PMSP", 4) != 0) return;
    int version = 0, count = 0;
    if (fread(&version, sizeof(version), 1, f) != 1 || fread(&count, sizeof(count), 1, f) != 1) {
        memset(g_passive_spawners, 0, sizeof(g_passive_spawners));
        return;
    }
    if (count < 0) count = 0;
    if (count > PEX_MOB_SPAWNER_CACHE_MAX) count = PEX_MOB_SPAWNER_CACHE_MAX;
    for (int i = 0; i < count; ++i) {
        PexMobSpawnerRuntime tmp;
        memset(&tmp, 0, sizeof(tmp));
        if (fread(&tmp.x, sizeof(int), 1, f) != 1 ||
            fread(&tmp.y, sizeof(int), 1, f) != 1 ||
            fread(&tmp.z, sizeof(int), 1, f) != 1 ||
            fread(&tmp.type, sizeof(int), 1, f) != 1 ||
            fread(&tmp.delay, sizeof(int), 1, f) != 1) {
            memset(g_passive_spawners, 0, sizeof(g_passive_spawners));
            return;
        }
        if (version >= 2) {
            if (fread(&tmp.spawn_count, sizeof(int), 1, f) != 1 ||
                fread(&tmp.max_nearby, sizeof(int), 1, f) != 1 ||
                fread(&tmp.required_range, sizeof(int), 1, f) != 1 ||
                fread(&tmp.spawn_range, sizeof(int), 1, f) != 1 ||
                fread(&tmp.last_seen_tick, sizeof(int), 1, f) != 1) {
                memset(g_passive_spawners, 0, sizeof(g_passive_spawners));
                return;
            }
        } else {
            if (fread(&tmp.last_seen_tick, sizeof(int), 1, f) != 1) {
                memset(g_passive_spawners, 0, sizeof(g_passive_spawners));
                return;
            }
        }
        if (!passive_mob_type_valid(tmp.type)) continue;
        passive_spawner_apply_java_defaults_125(&tmp);
        tmp.active = 1;
        g_passive_spawners[i] = tmp;
    }
    (void)version;
}

static void passive_mob_tick_burning(PassiveMob *m) {
    if (!m || m->death_time > 0) return;
    if (passive_is_undead_burner(m->type) && passive_world_is_daytime()) {
        int bx = (int)floorf(m->x), by = (int)floorf(m->y), bz = (int)floorf(m->z);
        float br = flat_light_brightness(bx, by, bz);
        if (br > 0.5f && passive_can_see_sky(bx, by, bz) && pex_rand_float01() * 30.0f < (br - 0.4f) * 2.0f) {
            m->burn_time = 8 * 20; /* Entity.setFire(8) */
        }
    }
    if (m->burn_time > 0 && (m->burn_time % 20) == 0) {
        (void)pex_mob_attack_entity_from(m, pex_damage_source_simple(PEX_DAMAGE_ON_FIRE), 1);
    }
}

static float passive_mob_tick_ai(PassiveMob *m, int in_liquid) {
    const PexMobInfo *info = passive_mob_info(m->type);
    float forward = 0.0f;
    int hostile = passive_mob_hostile(m);
    float player_d2 = passive_mob_player_distance2(m);
    m->ai_task_mask = 0;

    if ((g_opts.difficulty & 3) == 0 && passive_mob_category(m->type) == PEX_CAT_MONSTER && m->type != PASSIVE_MOB_OCELOT) {
        m->active = 0;
        g_save_dirty = 1;
        return 0.0f;
    }

    passive_mob_tick_burning(m);
    if (!m->active || m->death_time > 0) return 0.0f;
    if (m->type == PASSIVE_MOB_ZOMBIE) passive_zombie_tick_break_door_125(m);
    if (m->type == PASSIVE_MOB_ENDERMAN && m->rideable <= 0 && passive_player_looking_at_mob_125(m)) {
        m->rideable = 1;
        m->egg_timer = 0;
        pex_sound_play_at("mob.endermen.stare", m->x, m->y, m->z, 1.0f, 1.0f);
    }
    if (in_liquid) m->ai_task_mask |= PEX_AI_SWIM;
    if (player_d2 < 32.0f * 32.0f) {
        m->ai_age = 0;
    } else {
        if (m->ai_age < 0x3fffffff) ++m->ai_age;
        if (passive_mob_can_despawn_125(m)) {
            if (player_d2 > 128.0f * 128.0f ||
                (m->ai_age > 600 && (rand() % 800) == 0 && player_d2 > 32.0f * 32.0f)) {
                if (g_player_riding_passive_mob >= 0 && g_player_riding_passive_mob < MAX_PASSIVE_MOBS &&
                    &g_passive_mobs[g_player_riding_passive_mob] == m) {
                    g_player_riding_passive_mob = -1;
                }
                m->active = 0;
                g_save_dirty = 1;
                return 0.0f;
            }
        }
    }

    if (m->sitting && (m->type == PASSIVE_MOB_WOLF || m->type == PASSIVE_MOB_OCELOT)) {
        m->ai_task_mask |= PEX_AI_SIT;
        m->has_path_target = 0;
        passive_path_clear(m);
        m->mx *= 0.65f;
        m->mz *= 0.65f;
        return 0.0f;
    }

    int target_mob = -1;
    PassiveMob *tm = NULL;
    int handled_behavior_target = 0;
    int owned_wolf_has_defense_target =
        (m->type == PASSIVE_MOB_WOLF && passive_mob_is_owned_by_player(m) &&
         m->target_mob_index >= 0 && m->target_mob_index < MAX_PASSIVE_MOBS &&
         passive_owned_wolf_can_target_125(m, &g_passive_mobs[m->target_mob_index]));

    /* Java's wolf attack task has higher priority than follow-owner. */
    if (!owned_wolf_has_defense_target && passive_tameable_follow_owner_125(m)) {
        handled_behavior_target = 1;
    }

    if (m->type == PASSIVE_MOB_OCELOT && !passive_mob_is_owned_by_player(m) && !m->sheared &&
        player_d2 < 16.0f * 16.0f && !handled_behavior_target) {
        float ax, ay, az;
        if ((m->path_len <= 0 || m->path_index >= m->path_len || m->ai_repath_delay <= 0) &&
            passive_random_position_away_from_player(m, 16, 7, &ax, &ay, &az)) {
            passive_mob_set_path_goal(m, ax, ay, az);
            m->path_recalc_cooldown = 0;
            m->ai_repath_delay = 4 + (rand() % 7);
        }
        m->ai_task_mask |= PEX_AI_AVOID_PLAYER;
        handled_behavior_target = 1;
    }

    if (!handled_behavior_target && passive_mob_is_breedable_type(m->type)) {
        if (m->love_time > 0) {
            int mate = passive_mob_find_love_mate(m, 8.0f);
            if (mate >= 0) {
                PassiveMob *pm = &g_passive_mobs[mate];
                float dx = pm->x - m->x, dz = pm->z - m->z;
                float d2 = dx * dx + dz * dz;
                passive_mob_set_path_goal(m, pm->x, pm->y, pm->z);
                if (m->ai_repath_delay <= 0) { m->path_recalc_cooldown = 0; m->ai_repath_delay = 4 + (rand() % 7); }
                m->ai_task_mask |= PEX_AI_MATE;
                handled_behavior_target = 1;
                if (d2 < 2.25f) passive_mob_spawn_baby_from_parents(m, pm);
            }
        } else if (m->baby_age < 0) {
            int parent = -1;
            float best2 = 8.0f * 8.0f;
            for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
                PassiveMob *pm = &g_passive_mobs[i];
                if (pm == m || !pm->active || pm->death_time > 0 || pm->baby_age < 0) continue;
                if (!passive_mob_same_breeding_species(m->type, pm->type)) continue;
                float dx = pm->x - m->x, dz = pm->z - m->z;
                float d2 = dx * dx + dz * dz;
                if (d2 < best2) { best2 = d2; parent = i; }
            }
            if (parent >= 0) {
                PassiveMob *pm = &g_passive_mobs[parent];
                passive_mob_set_path_goal(m, pm->x, pm->y, pm->z);
                if (m->ai_repath_delay <= 0) { m->path_recalc_cooldown = 0; m->ai_repath_delay = 4 + (rand() % 7); }
                m->ai_task_mask |= PEX_AI_FOLLOW_PARENT;
                handled_behavior_target = 1;
            }
        } else {
            ItemStack *held = &g_inventory[g_selected_hotbar_slot];
            if (held && !stack_empty(held) && passive_mob_breeding_item_for_type(m->type, held->id) && player_d2 < 8.0f * 8.0f) {
                if ((m->type != PASSIVE_MOB_WOLF && m->type != PASSIVE_MOB_OCELOT) || m->sheared) {
                    passive_mob_set_path_goal(m, g_player_x, g_player_y - 1.62f, g_player_z);
                    if (m->ai_repath_delay <= 0) { m->path_recalc_cooldown = 0; m->ai_repath_delay = 4 + (rand() % 7); }
                    m->ai_task_mask |= PEX_AI_TEMPT;
                    handled_behavior_target = 1;
                }
            }
        }
    }

    if (handled_behavior_target) {
        target_mob = -1;
    } else if (owned_wolf_has_defense_target) {
        target_mob = m->target_mob_index;
    } else if (m->type == PASSIVE_MOB_ZOMBIE) {
        target_mob = passive_mob_find_nearest_target_mob(m, 1, 0, 16.0f);
    } else if (m->type == PASSIVE_MOB_WOLF && !m->sheared) {
        target_mob = passive_mob_find_nearest_type(m, PASSIVE_MOB_SHEEP, 16.0f);
    } else if (m->type == PASSIVE_MOB_OCELOT && !m->sheared) {
        target_mob = passive_mob_find_nearest_type(m, PASSIVE_MOB_CHICKEN, 14.0f);
    } else if (m->type == PASSIVE_MOB_IRON_GOLEM) {
        target_mob = passive_mob_find_nearest_target_mob(m, 0, 1, 16.0f);
    } else if (m->type == PASSIVE_MOB_SNOWMAN) {
        target_mob = passive_mob_find_nearest_target_mob(m, 0, 1, 10.0f);
    }
    if (target_mob >= 0) tm = &g_passive_mobs[target_mob];
    m->target_mob_index = target_mob;

    int wants_player = hostile;
    if (m->type == PASSIVE_MOB_IRON_GOLEM) {
        PexVillageRuntime *v = passive_nearest_village(m->x, m->z, 72.0f);
        if (v && v->player_reputation <= -15) wants_player = 1;
    }
    if (m->type == PASSIVE_MOB_SPIDER || m->type == PASSIVE_MOB_CAVE_SPIDER) {
        int bx = (int)floorf(m->x), by = (int)floorf(m->y), bz = (int)floorf(m->z);
        if (flat_light_brightness(bx, by, bz) >= 0.5f && (rand() % 100) == 0) wants_player = 0;
    }
    if (m->type == PASSIVE_MOB_PIG_ZOMBIE && m->rideable <= 0) wants_player = 0;
    if (m->type == PASSIVE_MOB_WOLF && m->rideable <= 0) wants_player = 0;

    if (tm) {
        passive_mob_set_path_goal(m, tm->x, tm->y, tm->z);
        if (m->ai_repath_delay <= 0) {
            m->path_recalc_cooldown = 0;
            m->ai_repath_delay = 4 + (rand() % 7);
        }
        m->ai_task_mask |= PEX_AI_ATTACK;
    } else if (wants_player && player_d2 < (m->type == PASSIVE_MOB_GHAST || m->type == PASSIVE_MOB_ENDER_DRAGON ? 64.0f * 64.0f : 16.0f * 16.0f)) {
        passive_mob_set_path_goal(m, g_player_x, g_player_y - 1.62f, g_player_z);
        if (m->ai_repath_delay <= 0) {
            m->path_recalc_cooldown = 0;
            m->ai_repath_delay = 4 + (rand() % 7);
        }
        m->ai_task_mask |= PEX_AI_ATTACK;
    } else if (!handled_behavior_target && (m->path_len <= 0 || m->path_index >= m->path_len) &&
               m->ai_age < 100 && (rand() % 120) == 0) {
        passive_mob_choose_target(m);
        m->ai_task_mask |= PEX_AI_WANDER;
    }

    /* EntityAIMoveThroughVillage / EntityAIMoveIndoors approximation: villagers
       and golems navigate by the runtime door collection instead of wandering
       randomly through the village center. */
    if (m->type == PASSIVE_MOB_VILLAGER && !handled_behavior_target && !wants_player && !tm) {
        int nighttime = !passive_world_is_daytime();
        PexVillageRuntime *v = passive_nearest_village(m->x, m->z, 72.0f);
        if (v && (nighttime || (rand() % 160) == 0)) {
            m->village_id = v->id;
            if (passive_villager_pick_door(m, v)) m->ai_task_mask |= PEX_AI_MOVE_VILLAGE;
        }
    }

    if (m->has_path_target) {
        int path_driving = passive_path_drive(m);
        int no_nav_path = (m->path_len <= 0 || m->path_index >= m->path_len);
        if (!m->has_path_target) {
            path_driving = 0;
        } else if (no_nav_path && !path_driving) {
            if (!wants_player && !tm && !handled_behavior_target) passive_path_clear(m);
        } else {
            float tx = m->target_x - m->x;
            float tz = m->target_z - m->z;
            float dist2 = tx * tx + tz * tz;
            if (dist2 < 0.55f * 0.55f && !path_driving && !wants_player && !tm) {
                passive_path_clear(m);
            } else {
                passive_mob_face_point(m, m->target_x, m->target_z, 30.0f);
                float sp = info ? info->move_speed : 0.23f;
                if (m->type == PASSIVE_MOB_SPIDER || m->type == PASSIVE_MOB_CAVE_SPIDER) sp = 0.80f;
                if (m->type == PASSIVE_MOB_ZOMBIE) sp = 0.23f;
                if (m->type == PASSIVE_MOB_PIG_ZOMBIE && m->rideable > 0) sp = 0.95f;
                if (m->type == PASSIVE_MOB_SKELETON) sp = 0.25f;
                if (wants_player && m->type == PASSIVE_MOB_ENDERMAN && player_d2 < 12.0f * 12.0f) sp = 0.65f;
                if (passive_mob_is_slime_family(m->type)) sp *= 0.55f;
                if (m->type == PASSIVE_MOB_GHAST || m->type == PASSIVE_MOB_ENDER_DRAGON) sp *= 0.35f;
                if (m->type == PASSIVE_MOB_SNOWMAN) sp = 0.25f;
                if (m->type == PASSIVE_MOB_IRON_GOLEM && tm) sp = 0.22f;
                if (m->ai_task_mask & PEX_AI_AVOID_PLAYER) sp = player_d2 < 7.0f * 7.0f ? 0.40f : 0.23f;
                forward = in_liquid ? 0.04f : sp * 0.10f;
                if (pex_passive_has_potion(m, PEX_POTION_SPEED)) forward *= 1.0f + 0.20f * (float)(pex_passive_potion_amplifier(m, PEX_POTION_SPEED) + 1);
                if (pex_passive_has_potion(m, PEX_POTION_SLOWNESS)) {
                    forward *= 1.0f - 0.15f * (float)(pex_passive_potion_amplifier(m, PEX_POTION_SLOWNESS) + 1);
                    if (forward < 0.0f) forward = 0.0f;
                }
                if (m->type != PASSIVE_MOB_CHICKEN && m->type != PASSIVE_MOB_GHAST && m->type != PASSIVE_MOB_ENDER_DRAGON &&
                    m->target_y > m->y + 0.35f && m->on_ground && m->stuck_ticks > 8) {
                    passive_mob_request_jump(m, "higher-target");
                }
            }
        }
    }

    if (tm) {
        float dx = tm->x - m->x, dz = tm->z - m->z;
        float d2 = dx*dx + dz*dz;
        if (m->type == PASSIVE_MOB_SNOWMAN && d2 < 10.0f * 10.0f) {
            passive_mob_face_point(m, tm->x, tm->z, 60.0f);
            passive_mob_attack_other_mob(m, tm, 1);
        } else if (d2 < (m->type == PASSIVE_MOB_IRON_GOLEM ? 3.0f*3.0f : 1.8f*1.8f)) {
            passive_mob_attack_other_mob(m, tm, 0);
        }
    }

    if (wants_player) {
        float reach = 1.35f + m->width * 0.5f;
        if (m->type == PASSIVE_MOB_GIANT) reach = 6.0f;
        if (m->type == PASSIVE_MOB_ENDER_DRAGON) reach = 8.0f;
        if (m->type == PASSIVE_MOB_CREEPER) {
            /* EntityAICreeperSwell: start under 3 blocks, keep swelling until >7 blocks, fuseTime=30. */
            if (player_d2 < 3.0f * 3.0f || (m->egg_timer > 0 && player_d2 < 7.0f * 7.0f)) {
                if (m->egg_timer <= 0) pex_sound_play_at("random.fuse", m->x, m->y, m->z, 1.0f, 0.5f);
                ++m->egg_timer;
                if (m->egg_timer >= 30) {
                    if (player_d2 < 5.0f * 5.0f) {
                        PexDamageSource source = pex_damage_source_simple(PEX_DAMAGE_EXPLOSION);
                        pex_damage_source_set_true_mob(&source, m);
                        (void)player_attack_entity_from(source, passive_mob_scaled_attack_damage(m, 49));
                    }
                    passive_creeper_explode(m);
                    pex_sound_play_at("random.explode", m->x, m->y, m->z, 4.0f, 1.0f);
                    PexDamageSource self_explosion = pex_damage_source_simple(PEX_DAMAGE_EXPLOSION);
                    pex_damage_source_set_true_mob(&self_explosion, m);
                    passive_mob_start_death(m, &self_explosion);
                }
            } else if (m->egg_timer > 0) {
                --m->egg_timer;
            }
        } else if (m->type == PASSIVE_MOB_SPIDER || m->type == PASSIVE_MOB_CAVE_SPIDER) {
            if (player_d2 > 2.0f*2.0f && player_d2 < 6.0f*6.0f && (rand()%10)==0 && m->on_ground) {
                float dx = g_player_x - m->x, dz = g_player_z - m->z;
                float len = sqrtf(dx*dx + dz*dz); if (len < 0.001f) len = 0.001f;
                m->mx = dx / len * 0.5f * 0.8f + m->mx * 0.2f;
                m->mz = dz / len * 0.5f * 0.8f + m->mz * 0.2f;
                m->my = 0.4f;
            } else if (player_d2 < reach * reach) {
                passive_mob_attack_player(m, 0);
            }
        } else if ((m->type == PASSIVE_MOB_SKELETON || m->type == PASSIVE_MOB_GHAST || m->type == PASSIVE_MOB_BLAZE) && player_d2 < 16.0f * 16.0f) {
            passive_mob_face_point(m, g_player_x, g_player_z, 60.0f);
            passive_mob_attack_player(m, 1);
        } else if (player_d2 < reach * reach) {
            passive_mob_attack_player(m, 0);
        }
        if (m->type == PASSIVE_MOB_ENDERMAN && (rand() % 240) == 0 && player_d2 > 12.0f * 12.0f) {
            (void)passive_mob_teleport_random_125(m, 16.0f);
        }
    }

    if ((m->type == PASSIVE_MOB_SPIDER || m->type == PASSIVE_MOB_CAVE_SPIDER) && m->collided_horizontal) {
        /* EntitySpider.isOnLadder() is true while collided horizontally. */
        if (m->my < 0.20f) m->my = 0.20f;
    }

    if ((m->type == PASSIVE_MOB_GHAST || m->type == PASSIVE_MOB_ENDER_DRAGON) && !in_liquid) {
        float desired_y = wants_player ? (g_player_y + 4.0f) : (m->target_y + 3.0f);
        m->my += (desired_y - m->y) * 0.0025f;
        m->my = pex_clamp_float(m->my, -0.12f, 0.12f);
    }

    if (in_liquid && (m->collided_horizontal || m->stuck_ticks > 10 || (rand() % 10) < 3)) {
        m->my += 0.035f;
        if (m->my > 0.22f) m->my = 0.22f;
    }
    if (!in_liquid && m->collided_horizontal && m->on_ground && forward > 0.0f) {
        if (m->type != PASSIVE_MOB_CHICKEN || m->stuck_ticks > 22) {
            passive_mob_request_jump(m, "horizontal-collision");
        } else {
            m->has_path_target = 0;
            m->wander_cooldown = 30 + (rand() % 60);
        }
    }
    return forward;
}


static void passive_silverfish_summon_allies_125(PassiveMob *m) {
    if (!m || m->type != PASSIVE_MOB_SILVERFISH) return;
    int bx = (int)floorf(m->x);
    int by = (int)floorf(m->y);
    int bz = (int)floorf(m->z);
    int stopped = 0;
    for (int oy = 0; !stopped && oy <= 5 && oy >= -5; oy = oy <= 0 ? 1 - oy : 0 - oy) {
        for (int oz = 0; !stopped && oz <= 10 && oz >= -10; oz = oz <= 0 ? 1 - oz : 0 - oz) {
            for (int ox = 0; !stopped && ox <= 10 && ox >= -10; ox = ox <= 0 ? 1 - ox : 0 - ox) {
                int x = bx + ox, y = by + oy, z = bz + oz;
                if (flat_get_block(x, y, z) != BLOCK_SILVERFISH) continue;
                spawn_block_destroy_particles(x, y, z, BLOCK_SILVERFISH);
                flat_set_block(x, y, z, 0);
                passive_silverfish_spawn_from_block_125(x, y, z);
                if (rand() & 1) stopped = 1;
            }
        }
    }
}

static void passive_silverfish_try_hide_125(PassiveMob *m) {
    if (!m || m->type != PASSIVE_MOB_SILVERFISH || m->death_time > 0) return;
    if (m->target_mob_index >= 0 || m->has_path_target) return;
    if (passive_mob_player_distance2(m) < 8.0f * 8.0f) return;
    static const int sides[6][3] = {{0,-1,0},{0,1,0},{0,0,-1},{0,0,1},{-1,0,0},{1,0,0}};
    int side = rand() % 6;
    int x = (int)floorf(m->x) + sides[side][0];
    int y = (int)floorf(m->y + 0.5f) + sides[side][1];
    int z = (int)floorf(m->z) + sides[side][2];
    int id = flat_get_block(x, y, z);
    int meta = passive_silverfish_block_meta_for_id_125(id);
    if (meta < 0) return;
    flat_set_block(x, y, z, BLOCK_SILVERFISH);
    flat_set_meta_raw(x, y, z, meta);
    spawn_block_destroy_particles(x, y, z, id);
    m->active = 0;
    g_save_dirty = 1;
}

static void passive_mob_tick_species_behavior(PassiveMob *m) {
    if (m->type == PASSIVE_MOB_CHICKEN) {
        m->chicken_prev_wing_rot = m->chicken_wing_rot;
        m->chicken_prev_dest_pos = m->chicken_dest_pos;
        m->chicken_dest_pos += (float)(m->on_ground ? -1 : 4) * 0.3f;
        m->chicken_dest_pos = pex_clamp_float(m->chicken_dest_pos, 0.0f, 1.0f);
        if (!m->on_ground && m->chicken_wing_speed < 1.0f) m->chicken_wing_speed = 1.0f;
        m->chicken_wing_speed *= 0.9f;
        if (!m->on_ground && m->my < 0.0f) m->my *= 0.6f;
        m->chicken_wing_rot += m->chicken_wing_speed * 2.0f;
        if (--m->egg_timer <= 0) {
            pex_sound_play_at("mob.chickenplop", m->x, m->y, m->z, 1.0f,
                              (pex_rand_float01() - pex_rand_float01()) * 0.2f + 1.0f);
            spawn_item_stack(m->x, m->y + 0.5f, m->z, make_stack(ITEM_EGG, 1, 0), 1);
            m->egg_timer = rand() % 6000 + 6000;
        }
    } else if (passive_mob_is_slime_family(m->type)) {
        m->chicken_prev_wing_rot = m->chicken_wing_rot; /* reused as Java slime squishAmount */
        float target_squish = m->on_ground ? 0.30f : -0.12f;
        m->chicken_wing_rot += (target_squish - m->chicken_wing_rot) * 0.35f;
        if (m->on_ground && m->jump_cooldown <= 0) {
            m->jump_cooldown = 10 + (rand() % 20);
            m->my = 0.38f + 0.06f * (float)(m->fleece_color > 0 ? m->fleece_color : 1);
            m->chicken_wing_rot = 0.55f;
        }
    } else if (m->type == PASSIVE_MOB_SQUID) {
        m->chicken_prev_wing_rot = m->chicken_wing_rot;
        m->chicken_wing_rot += 0.25f;
        if (!m->in_water) m->my -= 0.02f;
    } else if (m->type == PASSIVE_MOB_SHEEP) {
        /* EntityAIEatGrass/eatGrassBonus: sheep regrow wool and babies grow faster. */
        if ((m->sheared || m->baby_age < 0) && (rand() % 180) == 0) {
            int x = (int)floorf(m->x), y = (int)floorf(m->y), z = (int)floorf(m->z);
            if (flat_get_block(x, y - 1, z) == BLOCK_GRASS) {
                flat_set_block(x, y - 1, z, BLOCK_DIRT);
                m->sheared = 0;
                m->attack_time = 40; /* SheepTimer: render the Java head-down eating animation. */
                if (m->baby_age < 0) { m->baby_age += 1200; if (m->baby_age > 0) m->baby_age = 0; }
                g_save_dirty = 1;
            }
        }
    } else if (m->type == PASSIVE_MOB_PIG_ZOMBIE) {
        if (m->rideable > 0 && m->egg_timer <= 0) {
            m->rideable = 0;
            m->has_path_target = 0;
            passive_path_clear(m);
        }
    } else if (m->type == PASSIVE_MOB_ENDERMAN) {
        passive_enderman_tick_carry_block(m);
        if (m->in_water) {
            (void)passive_mob_teleport_random_125(m, 32.0f);
            if ((m->age % 20) == 0) (void)pex_mob_attack_entity_from(m, pex_damage_source_simple(PEX_DAMAGE_DROWN), 1);
        }
    } else if (m->type == PASSIVE_MOB_SILVERFISH) {
        if (m->tame_state > 0) {
            --m->tame_state;
            if (m->tame_state == 0) passive_silverfish_summon_allies_125(m);
        }
        if (m->active) passive_silverfish_try_hide_125(m);
    } else if (m->type == PASSIVE_MOB_SNOWMAN) {
        passive_snowman_tick_snow_trail(m);
        int biome = passive_current_biome_id_at((int)floorf(m->x), (int)floorf(m->z));
        if ((biome == BIOME_DESERT || biome == BIOME_DESERT_HILLS || biome == BIOME_HELL) && (m->age % 20) == 0) {
            (void)pex_mob_attack_entity_from(m, pex_damage_source_simple(PEX_DAMAGE_ON_FIRE), 1);
        }
        if (m->in_water && (m->age % 20) == 0) (void)pex_mob_attack_entity_from(m, pex_damage_source_simple(PEX_DAMAGE_DROWN), 1);
    }
}

static void passive_mob_apply_physics(PassiveMob *m, float forward, int in_liquid) {
    float yaw_rad = m->yaw * (float)M_PI / 180.0f;
    if (forward > 0.0f) {
        m->mx += -sinf(yaw_rad) * forward;
        m->mz +=  cosf(yaw_rad) * forward;
    }
    if (in_liquid) {
        passive_mob_move_entity(m, m->mx, m->my, m->mz);
        m->mx *= m->in_water ? 0.80f : 0.50f;
        m->my *= m->in_water ? 0.80f : 0.50f;
        m->mz *= m->in_water ? 0.80f : 0.50f;
        m->my -= 0.02f;
        if (m->collided_horizontal && m->my < 0.18f) m->my = 0.18f;
    } else {
        m->my -= 0.08f;
        if (m->my < -1.0f) m->my = -1.0f;

        passive_mob_move_entity(m, m->mx, m->my, m->mz);
        float friction = m->on_ground ? 0.546f : 0.91f;
        m->mx *= friction;
        m->mz *= friction;
        m->my *= 0.98f;
        if (m->on_ground && m->my < 0.0f) m->my = 0.0f;
    }
}

static void passive_mob_tick_animation(PassiveMob *m, float forward, int in_liquid) {
    float dx = m->x - m->prev_x;
    float dz = m->z - m->prev_z;
    float speed = sqrtf(dx * dx + dz * dz);
    if (m->has_path_target && forward > 0.0f && speed < 0.006f &&
        (m->collided_horizontal || m->on_ground)) {
        if (m->stuck_ticks < 200) ++m->stuck_ticks;
    } else if (m->stuck_ticks > 0) {
        --m->stuck_ticks;
    }
    if (m->stuck_ticks > 100) {
        m->has_path_target = 0;
        m->wander_cooldown = 0;
        m->stuck_ticks = 0;
        pex_logf_trace("passive mob dropped stuck target type=%s pos=%.2f,%.2f,%.2f",
                       passive_mob_name(m->type), m->x, m->y, m->z);
    }

    float target_amount = (speed > 0.01f && (m->on_ground || in_liquid))
        ? pex_clamp_float(speed * 4.0f, 0.0f, 1.0f) : 0.0f;
    m->limb_amount += (target_amount - m->limb_amount) * 0.4f;
    m->limb_swing += m->limb_amount;
    if (speed > 0.05f) {
        float move_yaw = atan2f(dz, dx) * 57.29578f - 90.0f;
        float d = pex_clamp_float(pex_wrap_degrees(move_yaw - m->render_yaw), -75.0f, 75.0f);
        m->render_yaw += d * 0.35f;
    } else {
        m->render_yaw += pex_wrap_degrees(m->yaw - m->render_yaw) * 0.15f;
    }
}

static void passive_mob_check_bounds(PassiveMob *m) {
    if (m->y < -32.0f || m->x < g_flat_world_origin_x - 8 ||
        m->x > g_flat_world_origin_x + FLAT_WORLD_SIZE + 8 ||
        m->z < g_flat_world_origin_z - 8 ||
        m->z > g_flat_world_origin_z + FLAT_WORLD_SIZE + 8) {
        if (g_player_riding_passive_mob >= 0 && g_player_riding_passive_mob < MAX_PASSIVE_MOBS &&
            &g_passive_mobs[g_player_riding_passive_mob] == m) {
            g_player_riding_passive_mob = -1;
        }
        m->active = 0;
    }
}

static void passive_mob_tick_living(PassiveMob *m) {
    passive_mob_tick_timers(m);
    passive_mob_tick_potions(m);
    if (passive_mob_tick_death(m)) return;
    passive_mob_tick_sounds(m);
    if (m->type == PASSIVE_MOB_ENDER_DRAGON) {
        passive_dragon_tick_living_125(m);
        passive_mob_check_bounds(m);
        return;
    }
    int in_liquid = passive_mob_update_liquid_state(m);
    float forward = passive_mob_tick_ai(m, in_liquid);
    passive_mob_tick_species_behavior(m);
    passive_mob_apply_physics(m, forward, in_liquid);
    passive_mob_tick_animation(m, forward, in_liquid);
    passive_mob_check_bounds(m);
}

static void passive_mobs_apply_riding(void) {
    if (g_mp_connected || g_player_dead) { g_player_riding_passive_mob = -1; return; }
    if (g_player_riding_passive_mob < 0 || g_player_riding_passive_mob >= MAX_PASSIVE_MOBS) return;
    PassiveMob *m = &g_passive_mobs[g_player_riding_passive_mob];
    if (!m->active || m->type != PASSIVE_MOB_PIG || !m->rideable || m->death_time > 0) {
        g_player_riding_passive_mob = -1;
        return;
    }
    float mount_eye_offset = m->height * 0.75f + 1.62f;
    g_player_x = m->x;
    g_player_y = m->y + mount_eye_offset;
    g_player_z = m->z;
    g_player_prev_x = m->prev_x;
    g_player_prev_y = m->prev_y + mount_eye_offset;
    g_player_prev_z = m->prev_z;
    g_player_motion_x = m->mx;
    g_player_motion_y = m->my;
    g_player_motion_z = m->mz;
    g_player_on_ground = m->on_ground;
}

static int passive_mob_tick_stride(const PassiveMob *m) {
    float dx = m->x - g_player_x;
    float dz = m->z - g_player_z;
    float d2 = dx * dx + dz * dz;
    if (d2 > 80.0f * 80.0f) return 8;
    if (d2 > 48.0f * 48.0f) return 4;
    return 1;
}

static void update_passive_mobs(void) {
    /* Passive mobs are disabled in multiplayer until entity spawn/move/despawn sync exists. */
    if (g_mp_connected) return;

    g_passive_pathfind_budget = PEX_PATHFIND_BUDGET_PER_TICK;
    g_passive_spawn_probe_budget = PEX_SPAWN_PROBE_BUDGET_PER_TICK;
    g_passive_perf_last_spawns_skipped_streaming = 0;
    g_prof_mob_spawn_probe_hits_last = 0;
    g_prof_mob_spawn_probe_misses_last = 0;
    g_prof_mob_spawn_calls_last = 0;
    g_prof_mob_spawn_columns_last = 0;
    g_prof_mob_spawn_ms_last = 0.0;
    g_prof_mob_living_ms_last = 0.0;
    g_prof_mob_living_ticked_last = 0;
    g_prof_mob_living_deferred_last = 0;
    g_prof_mob_path_solves_last = 0;
    g_prof_mob_path_nodes_last = 0;

    passive_mobs_enforce_cap();

    /* While the world-stream worker is still installing/generating chunks, avoid
       the natural-spawn and village scans.  Existing mobs keep ticking, but new
       spawn searches wait until chunks are actually present instead of probing
       half-streamed terrain and fighting the streaming thread. */
    double spawn_start = now_seconds();
    if (!world_stream_service_active()) {
        passive_mobs_ensure_ender_dragon_125();
        passive_mobs_try_village_spawns_exact();
        passive_mobs_tick_spawners_125();
        passive_mobs_try_natural_spawn();
    } else {
        g_passive_perf_last_spawns_skipped_streaming = 1;
    }
    g_prof_mob_spawn_ms_last = (now_seconds() - spawn_start) * 1000.0;

    int active_count = 0;
    double living_start = now_seconds();
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        PassiveMob *m = &g_passive_mobs[i];
        if (!m->active) continue;
        ++active_count;
        int stride = passive_mob_tick_stride(m);
        if (stride > 1 && ((g_ingame_ticks + i) % stride) != 0) {
            /* Keep interpolation stable while far mobs are simulated at a lower
               rate.  Near animals still tick every tick. */
            m->prev_x = m->x; m->prev_y = m->y; m->prev_z = m->z;
            m->prev_yaw = m->yaw; m->prev_render_yaw = m->render_yaw;
            m->prev_limb_swing = m->limb_swing; m->prev_limb_amount = m->limb_amount;
            ++g_prof_mob_living_deferred_last;
            continue;
        }
        /* Hard frame-protection: after a spike starts, defer non-critical mobs
           instead of letting one catch-up tick destroy the whole frame.  Near
           mobs and ridden mobs continue updating. */
        double elapsed_ms = (now_seconds() - living_start) * 1000.0;
        if (elapsed_ms > PEX_MOB_TICK_WALL_BUDGET_MS && i != g_player_riding_passive_mob) {
            float dx = m->x - g_player_x;
            float dz = m->z - g_player_z;
            if (dx*dx + dz*dz > 12.0f * 12.0f) {
                m->prev_x = m->x; m->prev_y = m->y; m->prev_z = m->z;
                m->prev_yaw = m->yaw; m->prev_render_yaw = m->render_yaw;
                m->prev_limb_swing = m->limb_swing; m->prev_limb_amount = m->limb_amount;
                ++g_prof_mob_living_deferred_last;
                continue;
            }
        }
        passive_mob_tick_living(m);
        ++g_prof_mob_living_ticked_last;
    }
    g_prof_mob_living_ms_last = (now_seconds() - living_start) * 1000.0;
    g_passive_perf_last_active = active_count;
    g_passive_perf_last_path_budget_left = g_passive_pathfind_budget;
    g_prof_mob_spawn_probe_budget_last = g_passive_spawn_probe_budget;
}

static Texture *passive_mob_texture_for_type(int type) {
    const PexMobInfo *info = passive_mob_info(type);
    if (info && info->texture && info->texture->id) return info->texture;
    return &tex_steve;
}

typedef struct PassiveMobRenderEntry {
    int active;
    int type;
    int sheared;
    int fleece_color;
    int rideable;
    int baby_age;
    int sitting;
    int held_block;
    int held_item;
    int owner_id;
    int love_time;
    int attack_time;
    int hurt;
    int detail;
    float x, y, z;
    float yaw;
    float head_yaw;
    float pitch;
    float move;
    float limb;
    float wing;
    float age;
    float sheep_eat_amount;
    float sheep_head_pitch;
    float model_scale;
    float death_time;
} PassiveMobRenderEntry;

static int passive_mob_visible_to_camera(float x, float y, float z, float radius,
                                         float camx, float camy, float camz,
                                         float yaw, float pitch) {
    float dx = x - camx;
    float dy = (y + radius) - camy;
    float dz = z - camz;
    float dist2 = dx * dx + dy * dy + dz * dz;
    if (dist2 < 1.0e-4f) return 1;

    float yaw_rad = yaw * (float)M_PI / 180.0f;
    float pitch_rad = pitch * (float)M_PI / 180.0f;
    float fx = -sinf(yaw_rad) * cosf(pitch_rad);
    float fy = -sinf(pitch_rad);
    float fz =  cosf(yaw_rad) * cosf(pitch_rad);
    float inv_dist = 1.0f / sqrtf(dist2);
    float dot = (dx * fx + dy * fy + dz * fz) * inv_dist;
    if (dot < -0.20f) return 0;
    if (dot > 0.30f) return 1;

    /* Keep very near mobs even at the edge of the view so attacks/interactions do
       not pop visually.  Farther mobs are skipped until they enter the view cone. */
    return dist2 < (8.0f + radius) * (8.0f + radius);
}

static void passive_mobs_build_render_list(const PassiveMob *src, float partial,
                                                            float camx, float camy, float camz,
                                                            float cam_yaw, float cam_pitch,
                                                            PassiveMobRenderEntry *out, int *out_count) {
    int count = 0;
    const float max_dist2 = PEX_PASSIVE_RENDER_DIST * PEX_PASSIVE_RENDER_DIST;
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        const PassiveMob *m = &src[i];
        if (!m->active || !passive_mob_type_valid(m->type)) continue;
        float x = m->prev_x + (m->x - m->prev_x) * partial;
        float y = m->prev_y + (m->y - m->prev_y) * partial;
        float z = m->prev_z + (m->z - m->prev_z) * partial;
        float dx = x - camx;
        float dy = y - camy;
        float dz = z - camz;
        if (m->death_time <= 0 && dx*dx + dy*dy + dz*dz > max_dist2) continue;
        if (m->death_time <= 0 &&
            !passive_mob_visible_to_camera(x, y, z, m->height, camx, camy, camz, cam_yaw, cam_pitch)) {
            continue;
        }
        if (count >= PEX_PASSIVE_RENDER_LIMIT) break;
        PassiveMobRenderEntry *e = &out[count++];
        memset(e, 0, sizeof(*e));
        e->active = 1;
        e->type = m->type;
        e->sheared = m->sheared;
        e->fleece_color = m->fleece_color & 15;
        e->rideable = m->rideable;
        e->baby_age = m->baby_age;
        e->sitting = m->sitting;
        e->held_block = m->held_block;
        e->held_item = m->held_item;
        e->owner_id = m->owner_id;
        e->love_time = m->love_time;
        e->attack_time = m->attack_time;
        e->hurt = (m->hurt_time > 0 || m->death_time > 0);
        e->detail = (dx*dx + dy*dy + dz*dz) < (18.0f * 18.0f) ? 1 : 0;
        e->x = x; e->y = y; e->z = z;
        e->yaw = passive_lerp_angle(m->prev_render_yaw, m->render_yaw, partial);
        e->head_yaw = passive_lerp_angle(m->prev_yaw, m->yaw, partial) - e->yaw;
        e->head_yaw = pex_clamp_float(pex_wrap_degrees(e->head_yaw), -45.0f, 45.0f);
        e->pitch = m->prev_pitch + (m->pitch - m->prev_pitch) * partial;
        e->move = m->prev_limb_amount + (m->limb_amount - m->prev_limb_amount) * partial;
        if (e->move > 1.0f) e->move = 1.0f;
        e->limb = m->prev_limb_swing + (m->limb_swing - m->prev_limb_swing) * partial;
        if (m->baby_age < 0) e->limb *= 3.0f; /* RenderLiving multiplies child limb swing by 3. */
        e->age = (float)m->age + partial;
        if (m->type == PASSIVE_MOB_SHEEP) {
            float sheep_timer = (float)m->attack_time;
            e->sheep_eat_amount = passive_sheep_eat_amount_125(sheep_timer, partial);
            e->sheep_head_pitch = passive_sheep_head_pitch_125(sheep_timer, partial, e->pitch * 0.0174532925f);
        }
        e->model_scale = passive_mob_model_scale_for_type(m->type);
        if (passive_mob_is_slime_family(m->type) && m->fleece_color > 0) e->model_scale = (float)m->fleece_color;
        e->death_time = m->death_time > 0 ? (float)m->death_time + partial : 0.0f;
        if (m->type == PASSIVE_MOB_CHICKEN) {
            float wr = m->chicken_prev_wing_rot + (m->chicken_wing_rot - m->chicken_prev_wing_rot) * partial;
            float wd = m->chicken_prev_dest_pos + (m->chicken_dest_pos - m->chicken_prev_dest_pos) * partial;
            e->wing = (sinf(wr) + 1.0f) * wd;
        } else if (passive_mob_is_slime_family(m->type)) {
            e->wing = m->chicken_prev_wing_rot + (m->chicken_wing_rot - m->chicken_prev_wing_rot) * partial;
            e->sheep_eat_amount = e->wing; /* reused by the slime/magma renderer as squish */
        }
        if (count >= PEX_PASSIVE_RENDER_LIMIT) break;
    }
    if (out_count) *out_count = count;
}

#if PEX_PASSIVE_RENDER_WORKER && !defined(PEX_PLATFORM_PSP) && !defined(PEX_PLATFORM_WII)
static CRITICAL_SECTION g_passive_render_cs;
static int g_passive_render_initialized = 0;
static int g_passive_render_stop = 0;
static int g_passive_render_has_job = 0;
static int g_passive_render_busy = 0;
static HANDLE g_passive_render_event = NULL;
static HANDLE g_passive_render_thread = NULL;
static PassiveMob g_passive_render_job_mobs[MAX_PASSIVE_MOBS];
static float g_passive_render_job_partial = 0.0f;
static float g_passive_render_job_camx = 0.0f, g_passive_render_job_camy = 0.0f, g_passive_render_job_camz = 0.0f;
static float g_passive_render_job_yaw = 0.0f, g_passive_render_job_pitch = 0.0f;
static PassiveMobRenderEntry g_passive_render_ready[MAX_PASSIVE_MOBS];
static int g_passive_render_ready_count = 0;

static DWORD WINAPI passive_render_worker_proc(LPVOID unused) {
    (void)unused;
    PassiveMob local_mobs[MAX_PASSIVE_MOBS];
    PassiveMobRenderEntry local_entries[MAX_PASSIVE_MOBS];
    for (;;) {
        WaitForSingleObject(g_passive_render_event, INFINITE);
        int stop = 0, have = 0;
        float partial = 0.0f, camx = 0.0f, camy = 0.0f, camz = 0.0f, yaw = 0.0f, pitch = 0.0f;
        EnterCriticalSection(&g_passive_render_cs);
        stop = g_passive_render_stop;
        if (!stop && g_passive_render_has_job) {
            memcpy(local_mobs, g_passive_render_job_mobs, sizeof(local_mobs));
            partial = g_passive_render_job_partial;
            camx = g_passive_render_job_camx;
            camy = g_passive_render_job_camy;
            camz = g_passive_render_job_camz;
            yaw = g_passive_render_job_yaw;
            pitch = g_passive_render_job_pitch;
            g_passive_render_has_job = 0;
            g_passive_render_busy = 1;
            have = 1;
        }
        LeaveCriticalSection(&g_passive_render_cs);
        if (stop) break;
        if (!have) continue;

        int count = 0;
        passive_mobs_build_render_list(local_mobs, partial, camx, camy, camz,
                                                        yaw, pitch, local_entries, &count);

        EnterCriticalSection(&g_passive_render_cs);
        memcpy(g_passive_render_ready, local_entries, (size_t)count * sizeof(local_entries[0]));
        g_passive_render_ready_count = count;
        g_passive_render_busy = 0;
        LeaveCriticalSection(&g_passive_render_cs);
    }
    EnterCriticalSection(&g_passive_render_cs);
    g_passive_render_busy = 0;
    LeaveCriticalSection(&g_passive_render_cs);
    return 0;
}

static void passive_render_worker_init(void) {
    if (g_passive_render_initialized) return;
    g_passive_render_initialized = 1;
    InitializeCriticalSection(&g_passive_render_cs);
    g_passive_render_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (g_passive_render_event) {
        g_passive_render_thread = CreateThread(NULL, 0x100000, passive_render_worker_proc, NULL, 0, NULL);
        if (g_passive_render_thread) SetThreadPriority(g_passive_render_thread, THREAD_PRIORITY_BELOW_NORMAL);
    }
}

static void passive_mobs_submit_render_job(float partial) {
    passive_render_worker_init();
    if (!g_passive_render_event || !g_passive_render_thread) return;
    EnterCriticalSection(&g_passive_render_cs);
    if (!g_passive_render_busy && !g_passive_render_has_job) {
        memcpy(g_passive_render_job_mobs, g_passive_mobs, sizeof(g_passive_render_job_mobs));
        g_passive_render_job_partial = partial;
        g_passive_render_job_camx = g_player_x;
        g_passive_render_job_camy = g_player_y;
        g_passive_render_job_camz = g_player_z;
        g_passive_render_job_yaw = g_player_yaw;
        g_passive_render_job_pitch = g_player_pitch;
        g_passive_render_has_job = 1;
        SetEvent(g_passive_render_event);
    }
    LeaveCriticalSection(&g_passive_render_cs);
}

static int passive_mobs_fetch_render_list(PassiveMobRenderEntry *out, int cap) {
    int count = 0;
    passive_render_worker_init();
    if (g_passive_render_event && g_passive_render_thread) {
        EnterCriticalSection(&g_passive_render_cs);
        count = g_passive_render_ready_count;
        if (count > cap) count = cap;
        if (count > 0) memcpy(out, g_passive_render_ready, (size_t)count * sizeof(out[0]));
        LeaveCriticalSection(&g_passive_render_cs);
        return count;
    }
    passive_mobs_build_render_list(g_passive_mobs, g_frame_partial, g_player_x, g_player_y, g_player_z,
                                                    g_player_yaw, g_player_pitch, out, &count);
    if (count > cap) count = cap;
    return count;
}
#else
static void passive_mobs_submit_render_job(float partial) { (void)partial; }
static int passive_mobs_fetch_render_list(PassiveMobRenderEntry *out, int cap) {
    int count = 0;
    passive_mobs_build_render_list(g_passive_mobs, g_frame_partial, g_player_x, g_player_y, g_player_z,
                                                    g_player_yaw, g_player_pitch, out, &count);
    if (count > cap) count = cap;
    return count;
}
#endif


/* Fast passive-mob model emission.
   The old path reused steve_part(), which does glPush/glRotate/glBegin/glEnd for
   every cube part.  On the D3D11 GL-compat backend every matrix change flushes
   the immediate stream, so 10 animals could turn into 70+ tiny draw batches and
   cost 3ms+.  Keep the same Beta-style cuboid UV layout, but apply each part's
   local rotation on the CPU and emit all parts for one texture in a single
   glBegin(GL_QUADS) block. */
typedef struct PassiveFastVec3 { float x, y, z; } PassiveFastVec3;

static float g_passive_fast_uv_w = 64.0f;
static float g_passive_fast_uv_h = 32.0f;
static float g_passive_fast_tint_r = 1.0f;
static float g_passive_fast_tint_g = 1.0f;
static float g_passive_fast_tint_b = 1.0f;
static float g_passive_fast_tint_a = 1.0f;
static float g_passive_fast_model_scale_x = 1.0f;

static void passive_fast_set_texture_dims(const Texture *tex) {
    g_passive_fast_uv_w = (tex && tex->w > 0) ? (float)tex->w : 64.0f;
    g_passive_fast_uv_h = (tex && tex->h > 0) ? (float)tex->h : 32.0f;
}

static void passive_fast_set_tint(float r, float g, float b) {
    g_passive_fast_tint_r = r;
    g_passive_fast_tint_g = g;
    g_passive_fast_tint_b = b;
    g_passive_fast_tint_a = 1.0f;
}

static void passive_fast_set_tint_alpha(float r, float g, float b, float a) {
    g_passive_fast_tint_r = r;
    g_passive_fast_tint_g = g;
    g_passive_fast_tint_b = b;
    g_passive_fast_tint_a = a;
}

static void passive_fast_color_for_normal(float nx, float ny, float nz) {
    float shade = 0.82f;
    if (ny < -0.5f) shade = 1.0f;
    else if (ny > 0.5f) shade = 0.55f;
    else if (nz > 0.5f) shade = 0.92f;
    else if (nz < -0.5f) shade = 0.70f;
    else if (nx != 0.0f) shade = 0.76f;
    glColor4f(shade * g_passive_fast_tint_r, shade * g_passive_fast_tint_g, shade * g_passive_fast_tint_b, g_passive_fast_tint_a);
}

static void passive_fast_rotate_xyz(float *x, float *y, float *z,
                                    float rx_deg, float ry_deg, float rz_deg) {
    if (rx_deg != 0.0f) {
        float a = rx_deg * (float)M_PI / 180.0f;
        float c = cosf(a), sn = sinf(a);
        float yy = *y * c - *z * sn;
        float zz = *y * sn + *z * c;
        *y = yy; *z = zz;
    }
    if (ry_deg != 0.0f) {
        float a = ry_deg * (float)M_PI / 180.0f;
        float c = cosf(a), sn = sinf(a);
        float xx = *x * c + *z * sn;
        float zz = -*x * sn + *z * c;
        *x = xx; *z = zz;
    }
    if (rz_deg != 0.0f) {
        float a = rz_deg * (float)M_PI / 180.0f;
        float c = cosf(a), sn = sinf(a);
        float xx = *x * c - *y * sn;
        float yy = *x * sn + *y * c;
        *x = xx; *y = yy;
    }
}

static PassiveFastVec3 passive_fast_part_vertex(float x, float y, float z,
                                                float pivot_x, float pivot_y, float pivot_z,
                                                float rx_deg, float ry_deg, float rz_deg) {
    passive_fast_rotate_xyz(&x, &y, &z, rx_deg, ry_deg, rz_deg);
    PassiveFastVec3 r;
    r.x = (x + pivot_x) * g_passive_fast_model_scale_x;
    r.y = y + pivot_y;
    r.z = z + pivot_z;
    return r;
}

static void passive_fast_vertex(const PassiveFastVec3 *p, float u, float v) {
    float uu = u / g_passive_fast_uv_w;
    float vv = v / g_passive_fast_uv_h;
    if (uu < 0.0f || uu > 1.0f) {
        uu = uu - floorf(uu);
        if (uu < 0.0f) uu += 1.0f;
    }
    if (vv < 0.0f || vv > 1.0f) {
        vv = vv - floorf(vv);
        if (vv < 0.0f) vv += 1.0f;
    }
    glTexCoord2f(uu, vv);
    glVertex3f(p->x * 0.0625f, p->y * 0.0625f, p->z * 0.0625f);
}

static void passive_fast_quad(PassiveFastVec3 a, PassiveFastVec3 b, PassiveFastVec3 c, PassiveFastVec3 d,
                              float u0, float v0, float u1, float v1,
                              float nx, float ny, float nz) {
    passive_fast_color_for_normal(nx, ny, nz);
    passive_fast_vertex(&a, u1, v0);
    passive_fast_vertex(&b, u0, v0);
    passive_fast_vertex(&c, u0, v1);
    passive_fast_vertex(&d, u1, v1);
}

static void passive_fast_box(int tex_x, int tex_y, float x, float y, float z,
                             int w, int h, int d, float inflate, int mirror,
                             float pivot_x, float pivot_y, float pivot_z,
                             float rx_deg, float ry_deg, float rz_deg) {
    float x0 = x - inflate;
    float y0 = y - inflate;
    float z0 = z - inflate;
    float x1 = x + (float)w + inflate;
    float y1 = y + (float)h + inflate;
    float z1 = z + (float)d + inflate;
    if (mirror) { float t = x1; x1 = x0; x0 = t; }

    float u_right0 = (float)(tex_x + d + w);
    float v_right0 = (float)(tex_y + d);
    float u_right1 = (float)(tex_x + d + w + d);
    float v_right1 = (float)(tex_y + d + h);
    float u_left0 = (float)(tex_x + 0);
    float v_left0 = (float)(tex_y + d);
    float u_left1 = (float)(tex_x + d);
    float v_left1 = (float)(tex_y + d + h);
    float u_top0 = (float)(tex_x + d);
    float v_top0 = (float)(tex_y + 0);
    float u_top1 = (float)(tex_x + d + w);
    float v_top1 = (float)(tex_y + d);
    float u_bottom0 = (float)(tex_x + d + w);
    float v_bottom0 = (float)(tex_y + 0);
    float u_bottom1 = (float)(tex_x + d + w + w);
    float v_bottom1 = (float)(tex_y + d);
    float u_front0 = (float)(tex_x + d);
    float v_front0 = (float)(tex_y + d);
    float u_front1 = (float)(tex_x + d + w);
    float v_front1 = (float)(tex_y + d + h);
    float u_back0 = (float)(tex_x + d + w + d);
    float v_back0 = (float)(tex_y + d);
    float u_back1 = (float)(tex_x + d + w + d + w);
    float v_back1 = (float)(tex_y + d + h);

#define PFV(A,B,C) passive_fast_part_vertex((A),(B),(C), pivot_x,pivot_y,pivot_z, rx_deg,ry_deg,rz_deg)
    passive_fast_quad(PFV(x1,y0,z1), PFV(x1,y0,z0), PFV(x1,y1,z0), PFV(x1,y1,z1), u_right0,v_right0,u_right1,v_right1, 1,0,0);
    passive_fast_quad(PFV(x0,y0,z0), PFV(x0,y0,z1), PFV(x0,y1,z1), PFV(x0,y1,z0), u_left0,v_left0,u_left1,v_left1, -1,0,0);
    passive_fast_quad(PFV(x1,y0,z1), PFV(x0,y0,z1), PFV(x0,y0,z0), PFV(x1,y0,z0), u_top0,v_top0,u_top1,v_top1, 0,-1,0);
    passive_fast_quad(PFV(x1,y1,z0), PFV(x0,y1,z0), PFV(x0,y1,z1), PFV(x1,y1,z1), u_bottom0,v_bottom0,u_bottom1,v_bottom1, 0,1,0);
    passive_fast_quad(PFV(x1,y0,z0), PFV(x0,y0,z0), PFV(x0,y1,z0), PFV(x1,y1,z0), u_front0,v_front0,u_front1,v_front1, 0,0,-1);
    passive_fast_quad(PFV(x0,y0,z1), PFV(x1,y0,z1), PFV(x1,y1,z1), PFV(x0,y1,z1), u_back0,v_back0,u_back1,v_back1, 0,0,1);
#undef PFV
}

static void passive_fast_part(int tex_x, int tex_y,
                              float pivot_x, float pivot_y, float pivot_z,
                              float box_x, float box_y, float box_z,
                              int box_w, int box_h, int box_d,
                              float inflate, int mirror,
                              float rot_x_deg, float rot_y_deg, float rot_z_deg) {
    passive_fast_box(tex_x, tex_y, box_x, box_y, box_z, box_w, box_h, box_d, inflate, mirror,
                     pivot_x, pivot_y, pivot_z, rot_x_deg, rot_y_deg, rot_z_deg);
}

#define steve_part passive_fast_part

/* Java 1.2.5 model emitter.
   These cuboids are transcribed from net.minecraft.src.Model*.java.  The
   function arguments match RenderLiving -> ModelBase.render: limb swing,
   limb swing amount, age/ticks, head yaw and head pitch.  Cuboid coordinates
   are left in Java model-space pixels and passive_fast_vertex converts them
   with the vanilla 1/16 model scale. */
static float passive_deg_from_rad(float r) { return r * 57.2957795f; }
static float passive_rad_from_deg(float d) { return d * 0.0174532925f; }
static float passive_tri_wave(float x, float period) {
    return (fabsf(fmodf(x, period) - period * 0.5f) - period * 0.25f) / (period * 0.25f);
}
typedef struct Pex125ModelRendererNode {
    int tex_x, tex_y;
    float pivot_x, pivot_y, pivot_z;
    float box_x, box_y, box_z;
    int box_w, box_h, box_d;
    float inflate;
    int mirror;
    float rot_x, rot_y, rot_z;
    int first_child;
    int child_count;
} Pex125ModelRendererNode;

static void p125_compose_modelrenderer_transform(float parent_px, float parent_py, float parent_pz,
                                                 float parent_rx, float parent_ry, float parent_rz,
                                                 float local_px, float local_py, float local_pz,
                                                 float local_rx, float local_ry, float local_rz,
                                                 float *out_px, float *out_py, float *out_pz,
                                                 float *out_rx, float *out_ry, float *out_rz) {
    /* Minimal ModelRenderer hierarchy composition: child rotationPoint is first
       rotated by the parent angles, then translated by the parent rotationPoint.
       Java 1.2.5's ModelRenderer.render does this through GL matrix nesting. */
    float x = local_px, y = local_py, z = local_pz;
    passive_fast_rotate_xyz(&x, &y, &z, passive_deg_from_rad(parent_rx), passive_deg_from_rad(parent_ry), passive_deg_from_rad(parent_rz));
    if (out_px) *out_px = parent_px + x;
    if (out_py) *out_py = parent_py + y;
    if (out_pz) *out_pz = parent_pz + z;
    if (out_rx) *out_rx = parent_rx + local_rx;
    if (out_ry) *out_ry = parent_ry + local_ry;
    if (out_rz) *out_rz = parent_rz + local_rz;
}

static void p125_modelrenderer_render_tree(const Pex125ModelRendererNode *nodes, int index, int count,
                                           float parent_px, float parent_py, float parent_pz,
                                           float parent_rx, float parent_ry, float parent_rz) {
    if (!nodes || index < 0 || index >= count) return;
    const Pex125ModelRendererNode *n = &nodes[index];
    float px, py, pz, rx, ry, rz;
    p125_compose_modelrenderer_transform(parent_px, parent_py, parent_pz, parent_rx, parent_ry, parent_rz,
                                         n->pivot_x, n->pivot_y, n->pivot_z, n->rot_x, n->rot_y, n->rot_z,
                                         &px, &py, &pz, &rx, &ry, &rz);
    steve_part(n->tex_x, n->tex_y, px, py, pz, n->box_x, n->box_y, n->box_z, n->box_w, n->box_h, n->box_d,
               n->inflate, n->mirror, passive_deg_from_rad(rx), passive_deg_from_rad(ry), passive_deg_from_rad(rz));
    for (int i=0; i<n->child_count; ++i) p125_modelrenderer_render_tree(nodes, n->first_child + i, count, px, py, pz, rx, ry, rz);
}

static void p125_part_rad(int tx, int ty, float px, float py, float pz,
                          float bx, float by, float bz, int bw, int bh, int bd,
                          float inflate, int mirror, float rx, float ry, float rz) {
    Pex125ModelRendererNode n;
    memset(&n, 0, sizeof(n));
    n.tex_x = tx; n.tex_y = ty; n.pivot_x = px; n.pivot_y = py; n.pivot_z = pz;
    n.box_x = bx; n.box_y = by; n.box_z = bz; n.box_w = bw; n.box_h = bh; n.box_d = bd;
    n.inflate = inflate; n.mirror = mirror; n.rot_x = rx; n.rot_y = ry; n.rot_z = rz;
    n.first_child = 0; n.child_count = 0;
    p125_modelrenderer_render_tree(&n, 0, 1, 0,0,0, 0,0,0);
}
static void p125_part_deg(int tx, int ty, float px, float py, float pz,
                          float bx, float by, float bz, int bw, int bh, int bd,
                          float inflate, int mirror, float rx, float ry, float rz) {
    p125_part_rad(tx, ty, px, py, pz, bx, by, bz, bw, bh, bd, inflate, mirror,
                  passive_rad_from_deg(rx), passive_rad_from_deg(ry), passive_rad_from_deg(rz));
}

static void passive_render_p125_quadruped(int type, int fur_layer, float limb, float move, float head_yaw, float head_pitch,
                                           float sheep_eat_amount, float sheep_head_pitch) {
    float leg1 = cosf(limb * 0.6662f) * 1.4f * move;
    float leg2 = cosf(limb * 0.6662f + (float)M_PI) * 1.4f * move;
    float body = (float)M_PI * 0.5f;
    float hy = passive_rad_from_deg(head_yaw);
    float hp = passive_rad_from_deg(head_pitch);

    if (type == PASSIVE_MOB_SHEEP && fur_layer) {
        float sheep_head_y = 6.0f + sheep_eat_amount * 9.0f;
        hp = sheep_head_pitch;
        p125_part_rad(0,0,   0,sheep_head_y,-8,  -3,-4,-4, 6,6,6, 0.6f,0, hp,hy,0);
        p125_part_rad(28,8,  0,5,2,   -4,-10,-7, 8,16,6, 1.75f,0, body,0,0);
        p125_part_rad(0,16, -3,12,7, -2,0,-2, 4,6,4, 0.5f,0, leg1,0,0);
        p125_part_rad(0,16,  3,12,7, -2,0,-2, 4,6,4, 0.5f,0, leg2,0,0);
        p125_part_rad(0,16, -3,12,-5,-2,0,-2, 4,6,4, 0.5f,0, leg2,0,0);
        p125_part_rad(0,16,  3,12,-5,-2,0,-2, 4,6,4, 0.5f,0, leg1,0,0);
        return;
    }

    if (type == PASSIVE_MOB_SHEEP) {
        float sheep_head_y = 6.0f + sheep_eat_amount * 9.0f;
        hp = sheep_head_pitch;
        p125_part_rad(0,0,   0,sheep_head_y,-8,  -3,-4,-6, 6,6,8, 0.0f,0, hp,hy,0);
        p125_part_rad(28,8,  0,5,2,   -4,-10,-7, 8,16,6, 0.0f,0, body,0,0);
        p125_part_rad(0,16, -3,12,7, -2,0,-2, 4,12,4, 0.0f,0, leg1,0,0);
        p125_part_rad(0,16,  3,12,7, -2,0,-2, 4,12,4, 0.0f,0, leg2,0,0);
        p125_part_rad(0,16, -3,12,-5,-2,0,-2, 4,12,4, 0.0f,0, leg2,0,0);
        p125_part_rad(0,16,  3,12,-5,-2,0,-2, 4,12,4, 0.0f,0, leg1,0,0);
        return;
    }

    if (type == PASSIVE_MOB_COW || type == PASSIVE_MOB_MOOSHROOM) {
        p125_part_rad(0,0,   0,4,-8,  -4,-4,-6, 8,8,6, 0.0f,0, hp,hy,0);
        p125_part_rad(22,0,  0,4,-8,  -5,-5,-4, 1,3,1, 0.0f,0, hp,hy,0);
        p125_part_rad(22,0,  0,4,-8,   4,-5,-4, 1,3,1, 0.0f,0, hp,hy,0);
        p125_part_rad(18,4,  0,5,2,   -6,-10,-7, 12,18,10, 0.0f,0, body,0,0);
        p125_part_rad(52,0,  0,5,2,   -2,2,-8, 4,6,1, 0.0f,0, body,0,0);
        p125_part_rad(0,16, -4,12,7, -2,0,-2, 4,12,4, 0.0f,0, leg1,0,0);
        p125_part_rad(0,16,  4,12,7, -2,0,-2, 4,12,4, 0.0f,0, leg2,0,0);
        p125_part_rad(0,16, -4,12,-6,-2,0,-2, 4,12,4, 0.0f,0, leg2,0,0);
        p125_part_rad(0,16,  4,12,-6,-2,0,-2, 4,12,4, 0.0f,0, leg1,0,0);
        return;
    }

    /* ModelPig extends ModelQuadruped(6) and adds the snout. */
    p125_part_rad(0,0,   0,12,-6, -4,-4,-8, 8,8,8, fur_layer ? 0.5f : 0.0f,0, hp,hy,0);
    if (!fur_layer) p125_part_rad(16,16,0,12,-6, -2,0,-9, 4,3,1, 0.0f,0, hp,hy,0);
    p125_part_rad(28,8,  0,11,2, -5,-10,-7, 10,16,8, 0.0f,0, body,0,0);
    p125_part_rad(0,16, -3,18,7, -2,0,-2, 4,6,4, 0.0f,0, leg1,0,0);
    p125_part_rad(0,16,  3,18,7, -2,0,-2, 4,6,4, 0.0f,0, leg2,0,0);
    p125_part_rad(0,16, -3,18,-5,-2,0,-2, 4,6,4, 0.0f,0, leg2,0,0);
    p125_part_rad(0,16,  3,18,-5,-2,0,-2, 4,6,4, 0.0f,0, leg1,0,0);
}

static void passive_render_p125_chicken(float limb, float move, float head_yaw, float head_pitch, float wing) {
    float leg1 = cosf(limb * 0.6662f) * 1.4f * move;
    float leg2 = cosf(limb * 0.6662f + (float)M_PI) * 1.4f * move;
    float hy = passive_rad_from_deg(head_yaw);
    float hp = -passive_rad_from_deg(head_pitch);
    p125_part_rad(0,0,   0,15,-4, -2,-6,-2, 4,6,3, 0,0, hp,hy,0);
    p125_part_rad(14,0,  0,15,-4, -2,-4,-4, 4,2,2, 0,0, hp,hy,0);
    p125_part_rad(14,4,  0,15,-4, -1,-2,-3, 2,2,2, 0,0, hp,hy,0);
    p125_part_rad(0,9,   0,16,0,  -3,-4,-3, 6,8,6, 0,0, (float)M_PI*0.5f,0,0);
    p125_part_rad(26,0, -2,19,1,  -1,0,-3, 3,5,3, 0,0, leg1,0,0);
    p125_part_rad(26,0,  1,19,1,  -1,0,-3, 3,5,3, 0,0, leg2,0,0);
    p125_part_rad(24,13,-4,13,0,   0,0,-3, 1,4,6, 0,0, 0,0,wing);
    p125_part_rad(24,13, 4,13,0,  -1,0,-3, 1,4,6, 0,0, 0,0,-wing);
}

static void passive_render_p125_creeper(float limb, float move, float head_yaw, float head_pitch) {
    float leg1 = cosf(limb * 0.6662f) * 1.4f * move;
    float leg2 = cosf(limb * 0.6662f + (float)M_PI) * 1.4f * move;
    float hy = passive_rad_from_deg(head_yaw), hp = passive_rad_from_deg(head_pitch);
    p125_part_rad(0,0,   0,4,0,  -4,-8,-4, 8,8,8, 0,0, hp,hy,0);
    p125_part_rad(16,16, 0,4,0,  -4,0,-2, 8,12,4, 0,0, 0,0,0);
    p125_part_rad(0,16, -2,16,4, -2,0,-2, 4,6,4, 0,0, leg1,0,0);
    p125_part_rad(0,16,  2,16,4, -2,0,-2, 4,6,4, 0,0, leg2,0,0);
    p125_part_rad(0,16, -2,16,-4,-2,0,-2, 4,6,4, 0,0, leg2,0,0);
    p125_part_rad(0,16,  2,16,-4,-2,0,-2, 4,6,4, 0,0, leg1,0,0);
}

typedef struct PexBipedPose {
    float hy, hp;
    float body_y;
    float arm_rx[2], arm_ry[2], arm_rz[2];
    float arm_px[2], arm_py[2], arm_pz[2];
    float leg_rx[2], leg_ry[2], leg_rz[2];
    float leg_px[2], leg_py[2], leg_pz[2];
} PexBipedPose;

static void passive_biped_pose_125(int type, float limb, float move, float age, float head_yaw, float head_pitch,
                                   int carrying, PexBipedPose *pose) {
    if (!pose) return;
    memset(pose, 0, sizeof(*pose));
    pose->hy = passive_rad_from_deg(head_yaw);
    pose->hp = passive_rad_from_deg(head_pitch);
    int skinny = (type == PASSIVE_MOB_SKELETON);
    int enderman = (type == PASSIVE_MOB_ENDERMAN);
    float yoff = enderman ? -14.0f : 0.0f;
    pose->arm_px[0] = enderman ? -3.0f : -5.0f;
    pose->arm_py[0] = 2.0f + yoff;
    pose->arm_pz[0] = 0.0f;
    pose->arm_px[1] = 5.0f;
    pose->arm_py[1] = 2.0f + yoff;
    pose->arm_pz[1] = 0.0f;
    pose->leg_px[0] = -2.0f;
    pose->leg_py[0] = (enderman ? 9.0f : 12.0f) + yoff;
    pose->leg_pz[0] = 0.0f;
    pose->leg_px[1] = 2.0f;
    pose->leg_py[1] = (enderman ? 9.0f : 12.0f) + yoff;
    pose->leg_pz[1] = 0.0f;

    pose->arm_rx[0] = cosf(limb * 0.6662f + (float)M_PI) * 2.0f * move * 0.5f;
    pose->arm_rx[1] = cosf(limb * 0.6662f) * 2.0f * move * 0.5f;
    pose->leg_rx[0] = cosf(limb * 0.6662f) * 1.4f * move;
    pose->leg_rx[1] = cosf(limb * 0.6662f + (float)M_PI) * 1.4f * move;
    pose->arm_rz[0] = cosf(age * 0.09f) * 0.05f + 0.05f;
    pose->arm_rz[1] = -(cosf(age * 0.09f) * 0.05f + 0.05f);
    pose->arm_rx[0] += sinf(age * 0.067f) * 0.05f;
    pose->arm_rx[1] -= sinf(age * 0.067f) * 0.05f;

    if (type == PASSIVE_MOB_ZOMBIE || type == PASSIVE_MOB_GIANT || type == PASSIVE_MOB_PIG_ZOMBIE || type == PASSIVE_MOB_SKELETON) {
        pose->arm_rz[0] = 0.0f;
        pose->arm_rz[1] = 0.0f;
        pose->arm_ry[0] = -0.1f;
        pose->arm_ry[1] = 0.1f;
        pose->arm_rx[0] = -(float)M_PI * 0.5f;
        pose->arm_rx[1] = -(float)M_PI * 0.5f;
        pose->arm_rz[0] += cosf(age * 0.09f) * 0.05f + 0.05f;
        pose->arm_rz[1] -= cosf(age * 0.09f) * 0.05f + 0.05f;
        pose->arm_rx[0] += sinf(age * 0.067f) * 0.05f;
        pose->arm_rx[1] -= sinf(age * 0.067f) * 0.05f;
        if (type == PASSIVE_MOB_SKELETON) {
            pose->arm_rz[0] = 0.0f;
            pose->arm_rz[1] = 0.0f;
            pose->arm_ry[0] = -0.1f + pose->hy;
            pose->arm_ry[1] = 0.5f + pose->hy;
            pose->arm_rx[0] = -(float)M_PI * 0.5f + pose->hp;
            pose->arm_rx[1] = -(float)M_PI * 0.5f + pose->hp;
            pose->arm_rz[0] += cosf(age * 0.09f) * 0.05f + 0.05f;
            pose->arm_rz[1] -= cosf(age * 0.09f) * 0.05f + 0.05f;
            pose->arm_rx[0] += sinf(age * 0.067f) * 0.05f;
            pose->arm_rx[1] -= sinf(age * 0.067f) * 0.05f;
        }
    }
    if (enderman) {
        pose->body_y = 0.0f;
        pose->arm_rx[0] *= 0.5f;
        pose->arm_rx[1] *= 0.5f;
        pose->leg_rx[0] *= 0.5f;
        pose->leg_rx[1] *= 0.5f;
        if (pose->arm_rx[0] > 0.4f) pose->arm_rx[0] = 0.4f; if (pose->arm_rx[0] < -0.4f) pose->arm_rx[0] = -0.4f;
        if (pose->arm_rx[1] > 0.4f) pose->arm_rx[1] = 0.4f; if (pose->arm_rx[1] < -0.4f) pose->arm_rx[1] = -0.4f;
        if (pose->leg_rx[0] > 0.4f) pose->leg_rx[0] = 0.4f; if (pose->leg_rx[0] < -0.4f) pose->leg_rx[0] = -0.4f;
        if (pose->leg_rx[1] > 0.4f) pose->leg_rx[1] = 0.4f; if (pose->leg_rx[1] < -0.4f) pose->leg_rx[1] = -0.4f;
        if (carrying) {
            pose->arm_rx[0] = -0.5f;
            pose->arm_rx[1] = -0.5f;
            pose->arm_rz[0] = 0.05f;
            pose->arm_rz[1] = -0.05f;
        }
    }
    (void)skinny;
}

static void passive_render_p125_biped(int type, float limb, float move, float age, float head_yaw, float head_pitch, int carrying) {
    PexBipedPose pose;
    passive_biped_pose_125(type, limb, move, age, head_yaw, head_pitch, carrying, &pose);

    int skinny = (type == PASSIVE_MOB_SKELETON);
    int enderman = (type == PASSIVE_MOB_ENDERMAN);
    float yoff = enderman ? -14.0f : 0.0f;
    int arm_w = skinny || enderman ? 2 : 4;
    int leg_w = skinny || enderman ? 2 : 4;
    int limb_h = enderman ? 30 : 12;
    float arm_box_x_r = skinny || enderman ? -1.0f : -3.0f;
    float arm_box_x_l = skinny || enderman ? -1.0f : -1.0f;
    float arm_box_z = enderman ? -1.0f : -2.0f;
    float leg_box_x = skinny || enderman ? -1.0f : -2.0f;
    float leg_box_z = enderman ? -1.0f : (skinny ? -1.0f : -2.0f);
    int headwear_tx = enderman ? 0 : 32;
    int headwear_ty = enderman ? 16 : 0;
    float headwear_inflate = enderman ? -0.5f : 0.5f;
    int body_tx = enderman ? 32 : 16;
    int body_ty = 16;
    int limb_tx = enderman ? 56 : 40;
    int limb_ty = enderman ? 0 : 16;
    int leg_tx = enderman ? 56 : 0;
    int leg_ty = 16;

    p125_part_rad(0,0, 0, yoff + (enderman ? 1.0f : 0.0f), 0, -4,-8,-4, 8,8,8, 0,0, pose.hp,pose.hy,0);
    p125_part_rad(headwear_tx,headwear_ty, 0, yoff + (enderman ? 1.0f : 0.0f), 0, -4,-8,-4, 8,8,8, headwear_inflate,0, pose.hp,pose.hy,0);
    p125_part_rad(body_tx,body_ty, 0,yoff,0, -4,0,-2, 8,12,4, 0,0, 0,0,0);
    p125_part_rad(limb_tx,limb_ty, pose.arm_px[0],pose.arm_py[0],pose.arm_pz[0], arm_box_x_r,-2,arm_box_z, arm_w,limb_h,arm_w, 0,0, pose.arm_rx[0],pose.arm_ry[0],pose.arm_rz[0]);
    p125_part_rad(limb_tx,limb_ty, pose.arm_px[1],pose.arm_py[1],pose.arm_pz[1], arm_box_x_l,-2,arm_box_z, arm_w,limb_h,arm_w, 0,1, pose.arm_rx[1],pose.arm_ry[1],pose.arm_rz[1]);
    p125_part_rad(leg_tx,leg_ty, pose.leg_px[0],pose.leg_py[0],pose.leg_pz[0], leg_box_x,0,leg_box_z, leg_w,limb_h,leg_w, 0,0, pose.leg_rx[0],pose.leg_ry[0],pose.leg_rz[0]);
    p125_part_rad(leg_tx,leg_ty, pose.leg_px[1],pose.leg_py[1],pose.leg_pz[1], leg_box_x,0,leg_box_z, leg_w,limb_h,leg_w, 0,1, pose.leg_rx[1],pose.leg_ry[1],pose.leg_rz[1]);
}

static void passive_render_p125_spider(float limb, float move, float head_yaw, float head_pitch) {
    float hy = passive_rad_from_deg(head_yaw), hp = passive_rad_from_deg(head_pitch);
    float q = (float)M_PI * 0.25f, e = (float)M_PI * 0.125f;
    float y[8] = {e*2,-e*2,e,-e,-e,e,-e*2,e*2};
    float z[8] = {-q,q,-q*0.74f,q*0.74f,-q*0.74f,q*0.74f,-q,q};
    float c0 = -(cosf(limb * 0.6662f * 2.0f + 0.0f) * 0.4f) * move;
    float c1 = -(cosf(limb * 0.6662f * 2.0f + (float)M_PI) * 0.4f) * move;
    float c2 = -(cosf(limb * 0.6662f * 2.0f + (float)M_PI * 0.5f) * 0.4f) * move;
    float c3 = -(cosf(limb * 0.6662f * 2.0f + (float)M_PI * 1.5f) * 0.4f) * move;
    float s0 = fabsf(sinf(limb * 0.6662f + 0.0f) * 0.4f) * move;
    float s1 = fabsf(sinf(limb * 0.6662f + (float)M_PI) * 0.4f) * move;
    float s2 = fabsf(sinf(limb * 0.6662f + (float)M_PI * 0.5f) * 0.4f) * move;
    float s3 = fabsf(sinf(limb * 0.6662f + (float)M_PI * 1.5f) * 0.4f) * move;
    y[0]+=c0; y[1]+=-c0; y[2]+=c1; y[3]+=-c1; y[4]+=c2; y[5]+=-c2; y[6]+=c3; y[7]+=-c3;
    z[0]+=s0; z[1]+=-s0; z[2]+=s1; z[3]+=-s1; z[4]+=s2; z[5]+=-s2; z[6]+=s3; z[7]+=-s3;
    p125_part_rad(32,4, 0,15,-3, -4,-4,-8, 8,8,8, 0,0, hp,hy,0);
    p125_part_rad(0,0,  0,15,0,  -3,-3,-3, 6,6,6, 0,0, 0,0,0);
    p125_part_rad(0,12, 0,15,9,  -5,-4,-6, 10,8,12, 0,0, 0,0,0);
    p125_part_rad(18,0, -4,15,2, -15,-1,-1, 16,2,2, 0,0, 0,y[0],z[0]);
    p125_part_rad(18,0,  4,15,2, -1,-1,-1, 16,2,2, 0,0, 0,y[1],z[1]);
    p125_part_rad(18,0, -4,15,1, -15,-1,-1, 16,2,2, 0,0, 0,y[2],z[2]);
    p125_part_rad(18,0,  4,15,1, -1,-1,-1, 16,2,2, 0,0, 0,y[3],z[3]);
    p125_part_rad(18,0, -4,15,0, -15,-1,-1, 16,2,2, 0,0, 0,y[4],z[4]);
    p125_part_rad(18,0,  4,15,0, -1,-1,-1, 16,2,2, 0,0, 0,y[5],z[5]);
    p125_part_rad(18,0, -4,15,-1,-15,-1,-1, 16,2,2, 0,0, 0,y[6],z[6]);
    p125_part_rad(18,0,  4,15,-1,-1,-1,-1, 16,2,2, 0,0, 0,y[7],z[7]);
}

static void passive_render_p125_slime(int magma, int layer, float age, float squish) {
    if (magma) {
        int tx[8] = {0,0,24,24,0,0,0,0};
        int ty[8] = {0,1,10,19,4,5,6,7};
        squish = pex_clamp_float(squish, -0.20f, 0.55f);
        for (int i = 0; i < 8; ++i) {
            float py = (float)(-(4 - i)) * squish * 1.7f;
            p125_part_rad(tx[i],ty[i], 0,py,0, -4,16+i,-4, 8,1,8, 0,0,0,0,0);
        }
        p125_part_rad(0,16,0,0,0, -2,18,-2,4,4,4,0,0,0,0,0);
    } else {
        if (layer == 1) {
            p125_part_rad(0,0,0,0,0, -4,16,-4,8,8,8,0,0,0,0,0);
        } else {
            p125_part_rad(0,16,0,0,0, -3,17,-3,6,6,6,0,0,0,0,0);
            p125_part_rad(32,0,0,0,0, -3.25f,18,-3.5f,2,2,2,0,0,0,0,0);
            p125_part_rad(32,4,0,0,0, 1.25f,18,-3.5f,2,2,2,0,0,0,0,0);
            p125_part_rad(32,8,0,0,0, 0,21,-3.5f,1,1,1,0,0,0,0,0);
        }
    }
}

static void passive_render_p125_ghast(float age) {
    static const int len[9] = {8,13,9,11,11,10,12,9,12};
    p125_part_rad(0,0, 0,8,0, -8,-8,-8,16,16,16,0,0,0,0,0);
    for (int i = 0; i < 9; ++i) {
        float x = ((((float)(i % 3) - (float)((i / 3) % 2) * 0.5f + 0.25f) / 2.0f * 2.0f - 1.0f) * 5.0f);
        float z = (((float)(i / 3) / 2.0f * 2.0f - 1.0f) * 5.0f);
        float rx = 0.2f * sinf(age * 0.3f + (float)i) + 0.4f;
        p125_part_rad(0,0, x,15,z, -1,0,-1,2,len[i],2,0,0,rx,0,0);
    }
}

static void passive_render_p125_blaze(float age, float head_yaw, float head_pitch) {
    float var7 = age * (float)M_PI * -0.1f;
    for (int i = 0; i < 4; ++i) {
        float y = -2.0f + cosf(((float)(i * 2) + age) * 0.25f);
        p125_part_rad(0,16, cosf(var7)*9.0f, y, sinf(var7)*9.0f, 0,0,0,2,8,2,0,0,0,0,0);
        var7 += (float)M_PI * 0.5f;
    }
    var7 = (float)M_PI * 0.25f + age * (float)M_PI * 0.03f;
    for (int i = 4; i < 8; ++i) {
        float y = 2.0f + cosf(((float)(i * 2) + age) * 0.25f);
        p125_part_rad(0,16, cosf(var7)*7.0f, y, sinf(var7)*7.0f, 0,0,0,2,8,2,0,0,0,0,0);
        var7 += (float)M_PI * 0.5f;
    }
    var7 = (float)M_PI * 0.15f + age * (float)M_PI * -0.05f;
    for (int i = 8; i < 12; ++i) {
        float y = 11.0f + cosf(((float)i * 1.5f + age) * 0.5f);
        p125_part_rad(0,16, cosf(var7)*5.0f, y, sinf(var7)*5.0f, 0,0,0,2,8,2,0,0,0,0,0);
        var7 += (float)M_PI * 0.5f;
    }
    p125_part_rad(0,0,0,0,0, -4,-4,-4,8,8,8,0,0,passive_rad_from_deg(head_pitch),passive_rad_from_deg(head_yaw),0);
}

static void passive_render_p125_squid(float age) {
    p125_part_rad(0,0,0,8,0, -6,-8,-6,12,16,12,0,0,0,0,0);
    for (int i = 0; i < 8; ++i) {
        double a = (double)i * M_PI * 2.0 / 8.0;
        float x = (float)cos(a) * 5.0f;
        float z = (float)sin(a) * 5.0f;
        float ry = (float)((double)i * M_PI * -2.0 / 8.0 + M_PI * 0.5);
        p125_part_rad(48,0,x,15,z, -1,0,-1,2,18,2,0,0,age * 0.08f,ry,0);
    }
}

static void passive_render_p125_wolf(float limb, float move, float age, float head_yaw, float head_pitch, int angry, int sitting) {
    float hy = passive_rad_from_deg(head_yaw), hp = passive_rad_from_deg(head_pitch);
    float leg1 = cosf(limb * 0.6662f) * 1.4f * move;
    float leg2 = cosf(limb * 0.6662f + (float)M_PI) * 1.4f * move;
    float tail_yaw = angry ? 0.0f : cosf(limb * 0.6662f) * 1.4f * move;
    float tail_x = age * 0.05f;
    p125_part_rad(0,0, -1,13.5f,-7, -3,-3,-2,6,6,4,0,0,hp,hy,0);
    p125_part_rad(16,14,-1,13.5f,-7, -3,-5,0,2,2,1,0,0,hp,hy,0);
    p125_part_rad(16,14,-1,13.5f,-7, 1,-5,0,2,2,1,0,0,hp,hy,0);
    p125_part_rad(0,10,-1,13.5f,-7, -1.5f,0,-5,3,3,4,0,0,hp,hy,0);
    if (sitting) {
        p125_part_rad(18,14,0,18,0, -4,-2,-3,6,9,6,0,0,(float)M_PI*0.25f,0,0);
        p125_part_rad(21,0,-1,16,-3, -4,-3,-3,8,6,7,0,0,(float)M_PI*0.4f,0,0);
        p125_part_rad(0,18,-2.5f,22,2, -1,0,-1,2,8,2,0,0,(float)M_PI*1.5f,0,0);
        p125_part_rad(0,18,0.5f,22,2, -1,0,-1,2,8,2,0,0,(float)M_PI*1.5f,0,0);
        p125_part_rad(0,18,-2.49f,17,-4,-1,0,-1,2,8,2,0,0,(float)M_PI*1.85f,0,0);
        p125_part_rad(0,18,0.51f,17,-4, -1,0,-1,2,8,2,0,0,(float)M_PI*1.85f,0,0);
        p125_part_rad(9,18,-1,21,6, -1,0,-1,2,8,2,0,0,tail_x,tail_yaw,0);
    } else {
        p125_part_rad(18,14,0,14,2, -4,-2,-3,6,9,6,0,0,(float)M_PI*0.5f,0,0);
        p125_part_rad(21,0,-1,14,-3, -4,-3,-3,8,6,7,0,0,(float)M_PI*0.5f,0,0);
        p125_part_rad(0,18,-2.5f,16,7, -1,0,-1,2,8,2,0,0,leg1,0,0);
        p125_part_rad(0,18,0.5f,16,7, -1,0,-1,2,8,2,0,0,leg2,0,0);
        p125_part_rad(0,18,-2.5f,16,-4,-1,0,-1,2,8,2,0,0,leg2,0,0);
        p125_part_rad(0,18,0.5f,16,-4, -1,0,-1,2,8,2,0,0,leg1,0,0);
        p125_part_rad(9,18,-1,12,8, -1,0,-1,2,8,2,0,0,tail_x,tail_yaw,0);
    }
}

static void passive_render_p125_ocelot(float limb, float move, float head_yaw, float head_pitch) {
    float hp = passive_rad_from_deg(head_pitch), hy = passive_rad_from_deg(head_yaw);
    float legA = cosf(limb * 0.6662f) * move;
    float legB = cosf(limb * 0.6662f + (float)M_PI) * move;
    float tail2 = (float)M_PI * 0.55f + (float)M_PI * 0.25f * cosf(limb) * move;
    p125_part_rad(0,0,  0,15,-9, -2.5f,-2,-3,5,4,5,0,0,hp,hy,0);
    p125_part_rad(0,24, 0,15,-9, -1.5f,0,-4,3,2,2,0,0,hp,hy,0);
    p125_part_rad(0,10, 0,15,-9, -2,-3,0,1,1,2,0,0,hp,hy,0);
    p125_part_rad(6,10, 0,15,-9,  1,-3,0,1,1,2,0,0,hp,hy,0);
    p125_part_rad(20,0, 0,12,-10, -2,3,-8,4,16,6,0,0,(float)M_PI*0.5f,0,0);
    p125_part_rad(0,15, 0,15,8, -0.5f,0,0,1,8,1,0,0,0.9f,0,0);
    p125_part_rad(4,15, 0,20,14, -0.5f,0,0,1,8,1,0,0,tail2,0,0);
    p125_part_rad(8,13, 1.1f,18,5, -1,0,1,2,6,2,0,0,legA,0,0);
    p125_part_rad(8,13,-1.1f,18,5, -1,0,1,2,6,2,0,0,legB,0,0);
    p125_part_rad(40,0, 1.2f,13.8f,-5, -1,0,0,2,10,2,0,0,legB,0,0);
    p125_part_rad(40,0,-1.2f,13.8f,-5, -1,0,0,2,10,2,0,0,legA,0,0);
}

static void passive_render_p125_iron_golem(float limb, float move, float head_yaw, float head_pitch) {
    float hy = passive_rad_from_deg(head_yaw), hp = passive_rad_from_deg(head_pitch);
    float legR = -1.5f * passive_tri_wave(limb, 13.0f) * move;
    float legL =  1.5f * passive_tri_wave(limb, 13.0f) * move;
    float armR = (-0.2f + 1.5f * passive_tri_wave(limb, 13.0f)) * move;
    float armL = (-0.2f - 1.5f * passive_tri_wave(limb, 13.0f)) * move;
    float yoff = -7.0f;
    p125_part_rad(0,0, 0,yoff,-2, -4,-12,-5.5f,8,10,8,0,0,hp,hy,0);
    p125_part_rad(24,0,0,yoff,-2, -1,-5,-7.5f,2,4,2,0,0,hp,hy,0);
    p125_part_rad(0,40,0,yoff,0, -9,-2,-6,18,12,11,0,0,0,0,0);
    p125_part_rad(0,70,0,yoff,0, -4.5f,10,-3,9,5,6,0.5f,0,0,0,0);
    p125_part_rad(60,21,0,-7,0, -13,-2.5f,-3,4,30,6,0,0,armR,0,0);
    p125_part_rad(60,58,0,-7,0, 9,-2.5f,-3,4,30,6,0,0,armL,0,0);
    p125_part_rad(37,0,-4,18+yoff,0, -3.5f,-3,-3,6,16,5,0,0,legR,0,0);
    p125_part_rad(60,0, 5,18+yoff,0, -3.5f,-3,-3,6,16,5,0,1,legL,0,0);
}

static void passive_render_p125_snowman(float head_yaw, float head_pitch) {
    float hp = passive_rad_from_deg(head_pitch), hy = passive_rad_from_deg(head_yaw);
    float body_y = hy * 0.25f;
    float sn = sinf(body_y), cs = cosf(body_y);
    p125_part_rad(0,16,0,13,0, -5,-10,-5,10,10,10,-0.5f,0,0,body_y,0);
    p125_part_rad(0,36,0,24,0, -6,-12,-6,12,12,12,-0.5f,0,0,0,0);
    p125_part_rad(0,0, 0,4,0,  -4,-8,-4,8,8,8,-0.5f,0,hp,hy,0);
    p125_part_rad(32,0, cs*5.0f,6,-sn*5.0f, -1,0,-1,12,2,2,-0.5f,0,0,body_y,1.0f);
    p125_part_rad(32,0,-cs*5.0f,6, sn*5.0f, -1,0,-1,12,2,2,-0.5f,0,0,(float)M_PI+body_y,-1.0f);
}

static void passive_render_p125_villager(float limb, float move, float head_yaw, float head_pitch) {
    float hp = passive_rad_from_deg(head_pitch), hy = passive_rad_from_deg(head_yaw);
    float legR = cosf(limb * 0.6662f) * 1.4f * move * 0.5f;
    float legL = cosf(limb * 0.6662f + (float)M_PI) * 1.4f * move * 0.5f;
    p125_part_rad(0,0,0,0,0, -4,-10,-4,8,10,8,0,0,hp,hy,0);
    p125_part_rad(24,0,0,0,0, -1,-3,-6,2,4,2,0,0,hp,hy,0);
    p125_part_rad(16,20,0,0,0, -4,0,-3,8,12,6,0,0,0,0,0);
    p125_part_rad(0,38,0,0,0, -4,0,-3,8,18,6,0.5f,0,0,0,0);
    p125_part_rad(44,22,0,3,-1, -8,-2,-2,4,8,4,0,0,-0.75f,0,0);
    p125_part_rad(44,22,0,3,-1, 4,-2,-2,4,8,4,0,0,-0.75f,0,0);
    p125_part_rad(40,38,0,3,-1, -4,2,-2,8,4,4,0,0,-0.75f,0,0);
    p125_part_rad(0,22,-2,12,0, -2,0,-2,4,12,4,0,0,legR,0,0);
    p125_part_rad(0,22, 2,12,0, -2,0,-2,4,12,4,0,1,legL,0,0);
}

static void passive_render_p125_silverfish(float age) {
    static const int len[7][3] = {{3,2,2},{4,3,2},{6,4,3},{3,3,3},{2,2,3},{2,1,2},{1,1,2}};
    static const int uv[7][2] = {{0,0},{0,4},{0,9},{0,16},{0,22},{11,0},{13,4}};
    float zpos[7]; float z = -3.5f;
    for (int i=0;i<7;++i) { zpos[i]=z; if (i<6) z += (float)(len[i][2]+len[i+1][2])*0.5f; }
    for (int i=0;i<7;++i) {
        float ry = cosf(age * 0.9f + (float)i * 0.15f * (float)M_PI) * (float)M_PI * 0.05f * (float)(1 + abs(i - 2));
        float px = sinf(age * 0.9f + (float)i * 0.15f * (float)M_PI) * (float)M_PI * 0.2f * (float)abs(i - 2);
        p125_part_rad(uv[i][0],uv[i][1], px, (float)(24-len[i][1]), zpos[i], -(float)len[i][0]*0.5f,0,-(float)len[i][2]*0.5f, len[i][0],len[i][1],len[i][2],0,0,0,ry,0);
    }
    float ry2 = cosf(age*0.9f + 2.0f*0.15f*(float)M_PI) * (float)M_PI*0.05f*(float)(1+abs(2-2));
    p125_part_rad(20,0, 0,16,zpos[2], -5,0,-(float)len[2][2]*0.5f,10,8,len[2][2],0,0,0,ry2,0);
    float ry4 = cosf(age*0.9f + 4.0f*0.15f*(float)M_PI) * (float)M_PI*0.05f*(float)(1+abs(4-2));
    float px4 = sinf(age*0.9f + 4.0f*0.15f*(float)M_PI) * (float)M_PI*0.2f*(float)abs(4-2);
    p125_part_rad(20,11,px4,20,zpos[4], -3,0,-(float)len[4][2]*0.5f,6,4,len[4][2],0,0,0,ry4,0);
    float ry1 = cosf(age*0.9f + 1.0f*0.15f*(float)M_PI) * (float)M_PI*0.05f*(float)(1+abs(1-2));
    float px1 = sinf(age*0.9f + 1.0f*0.15f*(float)M_PI) * (float)M_PI*0.2f*(float)abs(1-2);
    p125_part_rad(20,18,px1,19,zpos[1], -3,0,-(float)len[4][2]*0.5f,6,5,len[1][2],0,0,0,ry1,0);
}

static void passive_render_dragon_limb_tree(float root_px, float root_py, float root_pz,
                                            float root_rx, float tip_rx, float foot_rx,
                                            int root_tx, int root_ty, float root_bx, float root_by, float root_bz, int root_bw, int root_bh, int root_bd,
                                            int tip_tx, int tip_ty, float tip_px, float tip_py, float tip_pz, float tip_bx, float tip_by, float tip_bz, int tip_bw, int tip_bh, int tip_bd,
                                            int foot_tx, int foot_ty, float foot_px, float foot_py, float foot_pz, float foot_bx, float foot_by, float foot_bz, int foot_bw, int foot_bh, int foot_bd) {
    Pex125ModelRendererNode nodes[3];
    memset(nodes, 0, sizeof(nodes));
    nodes[0] = (Pex125ModelRendererNode){root_tx, root_ty, root_px, root_py, root_pz, root_bx, root_by, root_bz, root_bw, root_bh, root_bd, 0, 0, root_rx, 0, 0, 1, 1};
    nodes[1] = (Pex125ModelRendererNode){tip_tx, tip_ty, tip_px, tip_py, tip_pz, tip_bx, tip_by, tip_bz, tip_bw, tip_bh, tip_bd, 0, 0, tip_rx, 0, 0, 2, 1};
    nodes[2] = (Pex125ModelRendererNode){foot_tx, foot_ty, foot_px, foot_py, foot_pz, foot_bx, foot_by, foot_bz, foot_bw, foot_bh, foot_bd, 0, 0, foot_rx, 0, 0, 0, 0};
    p125_modelrenderer_render_tree(nodes, 0, 3, 0,0,0, 0,0,0);
}

static void passive_render_dragon_wing_tree(float mirror_x, float flap, float lift) {
    float wing_rx = 0.125f - cosf(flap) * 0.2f;
    float wing_ry = 0.25f;
    float wing_rz = (sinf(flap) + 0.125f) * 0.8f;
    float tip_rz = -((sinf(flap + 2.0f) + 0.5f) * 0.75f);
    float old_scale_x = g_passive_fast_model_scale_x;
    g_passive_fast_model_scale_x = old_scale_x * mirror_x;
    Pex125ModelRendererNode wing_nodes[4];
    memset(wing_nodes, 0, sizeof(wing_nodes));
    wing_nodes[0] = (Pex125ModelRendererNode){112, 88, -12.0f, 5.0f, 2.0f, -56.0f, -4.0f, -4.0f, 56, 8, 8, 0, 0, wing_rx, wing_ry, wing_rz, 1, 2};
    wing_nodes[1] = (Pex125ModelRendererNode){-56, 88, 0.0f, 0.0f, 0.0f, -56.0f, 0.0f, 2.0f, 56, 0, 56, 0, 0, 0, 0, 0, 0, 0};
    wing_nodes[2] = (Pex125ModelRendererNode){112, 136, -56.0f, 0.0f, 0.0f, -56.0f, -2.0f, -2.0f, 56, 4, 4, 0, 0, 0, 0, tip_rz, 3, 1};
    wing_nodes[3] = (Pex125ModelRendererNode){-56, 144, 0.0f, 0.0f, 0.0f, -56.0f, 0.0f, 2.0f, 56, 0, 56, 0, 0, 0, 0, 0, 0, 0};
    p125_modelrenderer_render_tree(wing_nodes, 0, 4, 0,0,0, 0,0,0);

    passive_render_dragon_limb_tree(-12.0f, 20.0f, 2.0f, 1.3f + lift * 0.1f, -0.5f - lift * 0.1f, 0.75f + lift * 0.1f,
                                    112,104, -4,-4,-4,8,24,8,
                                    226,138, 0,20,-1, -3,-1,-3,6,24,6,
                                    144,104, 0,23,0, -4,0,-12,8,4,16);
    passive_render_dragon_limb_tree(-16.0f, 16.0f, 42.0f, 1.0f + lift * 0.1f, 0.5f + lift * 0.1f, 0.75f + lift * 0.1f,
                                    0,0, -8,-4,-8,16,32,16,
                                    196,0, 0,32,-4, -6,-2,0,12,32,12,
                                    112,0, 0,31,4, -9,0,-20,18,6,24);
    g_passive_fast_model_scale_x = old_scale_x;
}

static void passive_render_p125_dragon(float age, float head_yaw, float head_pitch) {
    /* Geometry from ModelDragon.  The original Java model is hierarchical and
       driven by EntityDragon's 64-sample flight ring buffer.  PexCraft does not
       have that state, so this preserves the 1.2.5 cuboids/UVs and uses a local
       wing/neck/tail oscillator. */
    float flap = age * 0.15f;
    float hy = passive_rad_from_deg(head_yaw) * 0.35f;
    float hp = passive_rad_from_deg(head_pitch) * 0.35f;
    for (int i=0;i<5;++i) {
        float t=(float)i;
        p125_part_rad(192,104,0,20+sinf(flap+t*0.45f)*1.5f,-12-t*9.5f, -5,-5,-5,10,10,10,0,0,hp*0.3f+sinf(flap+t)*0.15f,hy*0.6f,0);
        p125_part_rad(48,0,0,20+sinf(flap+t*0.45f)*1.5f,-12-t*9.5f, -1,-9,-3,2,4,6,0,0,hp*0.3f+sinf(flap+t)*0.15f,hy*0.6f,0);
    }
    p125_part_rad(176,44,0,17,-62, -6,-1,-24,12,5,16,0,0,hp,hy,0);
    p125_part_rad(112,30,0,17,-62, -8,-8,-10,16,16,16,0,0,hp,hy,0);
    p125_part_rad(0,0,0,17,-62, -5,-12,-4,2,4,6,0,1,hp,hy,0);
    p125_part_rad(112,0,0,17,-62, -5,-3,-22,2,2,4,0,1,hp,hy,0);
    p125_part_rad(0,0,0,17,-62, 3,-12,-4,2,4,6,0,0,hp,hy,0);
    p125_part_rad(112,0,0,17,-62, 3,-3,-22,2,2,4,0,0,hp,hy,0);
    p125_part_rad(176,65,0,21,-70, -6,0,-16,12,4,16,0,0,0.15f+sinf(flap)*0.2f,hy,0);
    p125_part_rad(0,0,0,4,8, -12,0,-16,24,24,64,0,0,0,0,0);
    p125_part_rad(220,53,0,4,8, -1,-6,-10,2,6,12,0,0,0,0,0);
    p125_part_rad(220,53,0,4,8, -1,-6,10,2,6,12,0,0,0,0,0);
    p125_part_rad(220,53,0,4,8, -1,-6,30,2,6,12,0,0,0,0,0);
    for (int side=0; side<2; ++side) {
        passive_render_dragon_wing_tree(side ? -1.0f : 1.0f, flap, sinf(flap) + 1.0f);
    }
    for (int i=0;i<12;++i) {
        p125_part_rad(192,104,0,10+sinf(flap+i*0.45f)*1.2f,60+i*9.5f, -5,-5,-5,10,10,10,0,0,sinf(flap+i*0.45f)*0.2f,(float)M_PI+sinf(flap+i*0.25f)*0.2f,0);
        p125_part_rad(48,0,0,10+sinf(flap+i*0.45f)*1.2f,60+i*9.5f, -1,-9,-3,2,4,6,0,0,sinf(flap+i*0.45f)*0.2f,(float)M_PI,0);
    }
}

static void passive_render_quad_model(int type, int fur_layer, int detail, float limb, float move, float head_yaw, float head_pitch,
                                      float age, int sitting, int rideable, int held_block,
                                      float sheep_eat_amount, float sheep_head_pitch) {
    (void)detail;
    switch (type) {
        case PASSIVE_MOB_PIG:
        case PASSIVE_MOB_SHEEP:
        case PASSIVE_MOB_COW:
        case PASSIVE_MOB_MOOSHROOM:
            passive_render_p125_quadruped(type, fur_layer, limb, move, head_yaw, head_pitch, sheep_eat_amount, sheep_head_pitch); break;
        case PASSIVE_MOB_CREEPER:
            passive_render_p125_creeper(limb, move, head_yaw, head_pitch); break;
        case PASSIVE_MOB_SPIDER:
        case PASSIVE_MOB_CAVE_SPIDER:
            passive_render_p125_spider(limb, move, head_yaw, head_pitch); break;
        case PASSIVE_MOB_SLIME:
            passive_render_p125_slime(0, fur_layer, age, sheep_eat_amount); break;
        case PASSIVE_MOB_MAGMA_CUBE:
            passive_render_p125_slime(1, fur_layer, age, sheep_eat_amount); break;
        case PASSIVE_MOB_GHAST:
            passive_render_p125_ghast(age); break;
        case PASSIVE_MOB_BLAZE:
            passive_render_p125_blaze(age, head_yaw, head_pitch); break;
        case PASSIVE_MOB_SQUID:
            passive_render_p125_squid(age); break;
        case PASSIVE_MOB_WOLF:
            passive_render_p125_wolf(limb, move, age, head_yaw, head_pitch, rideable != 0, sitting != 0); break;
        case PASSIVE_MOB_OCELOT:
            passive_render_p125_ocelot(limb, move, head_yaw, head_pitch); break;
        case PASSIVE_MOB_IRON_GOLEM:
            passive_render_p125_iron_golem(limb, move, head_yaw, head_pitch); break;
        case PASSIVE_MOB_SNOWMAN:
            passive_render_p125_snowman(head_yaw, head_pitch); break;
        case PASSIVE_MOB_VILLAGER:
            passive_render_p125_villager(limb, move, head_yaw, head_pitch); break;
        case PASSIVE_MOB_SILVERFISH:
            passive_render_p125_silverfish(age); break;
        case PASSIVE_MOB_ENDER_DRAGON:
            passive_render_p125_dragon(age, head_yaw, head_pitch); break;
        default:
            passive_render_p125_biped(type, limb, move, age, head_yaw, head_pitch, type == PASSIVE_MOB_ENDERMAN && held_block > 0); break;
    }
}

#undef steve_part

static void passive_apply_biped_right_arm_postrender(float px, float py, float pz, float rx, float ry, float rz) {
    glTranslatef(px * 0.0625f, py * 0.0625f, pz * 0.0625f);
    if (rz != 0.0f) glRotatef(passive_deg_from_rad(rz), 0.0f, 0.0f, 1.0f);
    if (ry != 0.0f) glRotatef(passive_deg_from_rad(ry), 0.0f, 1.0f, 0.0f);
    if (rx != 0.0f) glRotatef(passive_deg_from_rad(rx), 1.0f, 0.0f, 0.0f);
}

static void passive_render_biped_held_item_125(int type, int item_id, float limb, float move, float age, float head_yaw, float head_pitch) {
    if (!tex_items.id || item_id <= 0) return;
    PexBipedPose pose;
    passive_biped_pose_125(type, limb, move, age, head_yaw, head_pitch, 0, &pose);
    glPushMatrix();
    passive_apply_biped_right_arm_postrender(pose.arm_px[0], pose.arm_py[0], pose.arm_pz[0],
                                             pose.arm_rx[0], pose.arm_ry[0], pose.arm_rz[0]);
    glTranslatef(-1.0f / 16.0f, 7.0f / 16.0f, 1.0f / 16.0f);
    if (item_id == ITEM_BOW) {
        float s = 10.0f / 16.0f;
        glTranslatef(0.0f, 2.0f / 16.0f, 5.0f / 16.0f);
        glRotatef(-20.0f, 0.0f, 1.0f, 0.0f);
        glScalef(s, -s, s);
        glRotatef(-100.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
    } else {
        float s = 10.0f / 16.0f;
        glTranslatef(0.0f, 3.0f / 16.0f, 0.0f);
        glScalef(s, -s, s);
        glRotatef(-100.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
    }
    draw_item3d_from_texture(&tex_items, item_icon_tile(item_id));
    glPopMatrix();
}

static void passive_render_skeleton_bow_item(float limb, float move, float age, float head_yaw, float head_pitch) {
    passive_render_biped_held_item_125(PASSIVE_MOB_SKELETON, ITEM_BOW, limb, move, age, head_yaw, head_pitch);
}

static void passive_render_pigzombie_sword_item(float limb, float move, float age, float head_yaw, float head_pitch) {
    passive_render_biped_held_item_125(PASSIVE_MOB_PIG_ZOMBIE, ITEM_SWORD_GOLD, limb, move, age, head_yaw, head_pitch);
}

static void passive_render_golem_rose_item(float attack_time) {
    if (attack_time > 0.0f) return;
    int tile = block_texture_resolve(BLOCK_RED_ROSE, 0, 2);
    glPushMatrix();
    glTranslatef(-0.70f, -1.05f, -0.25f);
    glRotatef(75.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(-20.0f, 0.0f, 0.0f, 1.0f);
    glScalef(0.42f, 0.42f, 0.42f);
    passive_draw_terrain_sprite_tile(tile);
    glPopMatrix();
}

static void passive_render_snowman_pumpkin_item(float head_yaw, float head_pitch) {
    if (!tex_terrain.id) return;
    glPushMatrix();
    glTranslatef(0.0f, 4.0f * 0.0625f, 0.0f);
    if (head_yaw != 0.0f) glRotatef(head_yaw, 0.0f, 1.0f, 0.0f);
    if (head_pitch != 0.0f) glRotatef(head_pitch, 1.0f, 0.0f, 0.0f);
    glTranslatef(0.0f, -(11.0f / 32.0f), 0.0f);
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
    float s = 10.0f / 16.0f;
    glScalef(s, -s, s);
    draw_block_item_model(BLOCK_PUMPKIN, -0.5f, -0.5f, -0.5f);
    glPopMatrix();
}

static void passive_terrain_tile_uv_local(int tile, float *u0, float *v0, float *u1, float *v1) {
    int tx = tile & 15;
    int ty = tile >> 4;
    float step = 1.0f / 16.0f;
    *u0 = tx * step;
    *v0 = ty * step;
    *u1 = *u0 + step;
    *v1 = *v0 + step;
}

static void passive_draw_terrain_sprite_tile(int tile) {
    if (!tex_terrain.id || tile < 0) return;
    float u0, v0, u1, v1;
    passive_terrain_tile_uv_local(tile, &u0, &v0, &u1, &v1);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v1); glVertex3f(-0.5f, -0.5f, 0.0f);
    glTexCoord2f(u1, v1); glVertex3f( 0.5f, -0.5f, 0.0f);
    glTexCoord2f(u1, v0); glVertex3f( 0.5f,  0.5f, 0.0f);
    glTexCoord2f(u0, v0); glVertex3f(-0.5f,  0.5f, 0.0f);
    glEnd();
}

static void passive_render_enderman_held_block(int block_id) {
    if (block_id <= 0) return;
    int tile = block_texture_resolve(block_id, 0, 2);
    glPushMatrix();
    glTranslatef(0.0f, -1.05f, -0.38f);
    glRotatef(20.0f, 1.0f, 0.0f, 0.0f);
    glScalef(0.52f, 0.52f, 0.52f);
    passive_draw_terrain_sprite_tile(tile);
    glPopMatrix();
}

static void passive_render_mooshroom_mushrooms(float head_yaw) {
    int tile = block_texture_resolve(BLOCK_RED_MUSHROOM, 0, 2);
    glPushMatrix();
    glTranslatef(0.20f, -1.05f, 0.18f);
    glRotatef(42.0f, 0.0f, 1.0f, 0.0f);
    glScalef(0.28f, 0.28f, 0.28f);
    passive_draw_terrain_sprite_tile(tile);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-0.10f, -1.05f, 0.28f);
    glRotatef(84.0f, 0.0f, 1.0f, 0.0f);
    glScalef(0.28f, 0.28f, 0.28f);
    passive_draw_terrain_sprite_tile(tile);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0f, -1.35f, -0.48f);
    glRotatef(head_yaw + 12.0f, 0.0f, 1.0f, 0.0f);
    glScalef(0.24f, 0.24f, 0.24f);
    passive_draw_terrain_sprite_tile(tile);
    glPopMatrix();
}

static void passive_sheep_fleece_color(int meta, float *r, float *g, float *b) {
    static const float table[16][3] = {
        {1.0f, 1.0f, 1.0f}, {0.95f, 0.7f, 0.2f}, {0.9f, 0.5f, 0.85f}, {0.6f, 0.7f, 0.95f},
        {0.9f, 0.9f, 0.2f}, {0.5f, 0.8f, 0.1f}, {0.95f, 0.7f, 0.8f}, {0.3f, 0.3f, 0.3f},
        {0.6f, 0.6f, 0.6f}, {0.3f, 0.6f, 0.7f}, {0.7f, 0.4f, 0.9f}, {0.2f, 0.4f, 0.8f},
        {0.5f, 0.4f, 0.3f}, {0.4f, 0.5f, 0.2f}, {0.8f, 0.3f, 0.3f}, {0.1f, 0.1f, 0.1f}
    };
    meta &= 15;
    *r = table[meta][0]; *g = table[meta][1]; *b = table[meta][2];
}

static void draw_passive_mobs(float partial) {
    /* Passive mobs are disabled in multiplayer until entity spawn/move/despawn sync exists. */
    if (g_mp_connected) return;
    passive_mobs_submit_render_job(partial);
    PassiveMobRenderEntry entries[PEX_PASSIVE_RENDER_LIMIT];
    int entry_count = passive_mobs_fetch_render_list(entries, PEX_PASSIVE_RENDER_LIMIT);
    if (g_loggy_enabled) g_loggy_entity_passive_entries = entry_count;
    if (entry_count <= 0) return;

    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);

    for (int i = 0; i < entry_count; ++i) {
        PassiveMobRenderEntry *e = &entries[i];
        if (!e->active || !passive_mob_type_valid(e->type)) continue;
        Texture *t = passive_mob_texture_for_type(e->type);
        if (e->type == PASSIVE_MOB_WOLF) {
            if (e->rideable && e->owner_id != PEX_MOB_OWNER_SINGLEPLAYER && tex_mob_wolf_angry.id) t = &tex_mob_wolf_angry;
            else if (e->sheared && tex_mob_wolf_tame.id) t = &tex_mob_wolf_tame;
        } else if (e->type == PASSIVE_MOB_OCELOT && e->sheared) {
            if ((e->fleece_color & 3) == 1 && tex_mob_cat_black.id) t = &tex_mob_cat_black;
            else if ((e->fleece_color & 3) == 2 && tex_mob_cat_red.id) t = &tex_mob_cat_red;
            else if ((e->fleece_color & 3) == 3 && tex_mob_cat_siamese.id) t = &tex_mob_cat_siamese;
        } else if (e->type == PASSIVE_MOB_VILLAGER) {
            int prof = e->fleece_color % 5;
            if (prof == 0 && tex_mob_villager_farmer.id) t = &tex_mob_villager_farmer;
            else if (prof == 1 && tex_mob_villager_librarian.id) t = &tex_mob_villager_librarian;
            else if (prof == 2 && tex_mob_villager_priest.id) t = &tex_mob_villager_priest;
            else if (prof == 3 && tex_mob_villager_smith.id) t = &tex_mob_villager_smith;
            else if (prof == 4 && tex_mob_villager_butcher.id) t = &tex_mob_villager_butcher;
        } else if (e->type == PASSIVE_MOB_GHAST && e->attack_time > 0 && tex_mob_ghast_fire.id) {
            t = &tex_mob_ghast_fire;
        }
        if (!t || !t->id) continue;

        float shadow_size = passive_mob_shadow_size_125(e->type);
        if (e->baby_age < 0) shadow_size *= 0.5f;
        draw_java_entity_shadow(e->x, e->y, e->z, shadow_size, 1.0f);

        glPushMatrix();
        glBindTexture(GL_TEXTURE_2D, t->id);
        passive_fast_set_texture_dims(t);
        {
            float entity_br = flat_light_brightness((int)floorf(e->x), (int)floorf(e->y + 1.0f), (int)floorf(e->z));
            if (entity_br < 0.18f) entity_br = 0.18f;
            if (entity_br > 1.0f) entity_br = 1.0f;
            if (e->hurt) passive_fast_set_tint(entity_br, entity_br * 0.35f, entity_br * 0.35f);
            else passive_fast_set_tint(entity_br, entity_br, entity_br);
        }
        glTranslatef(e->x, e->y, e->z);
        glRotatef(180.0f - e->yaw, 0.0f, 1.0f, 0.0f);
        if (e->death_time > 0.0f) {
            float d = ((e->death_time - 1.0f) / 20.0f) * 1.6f;
            if (d < 0.0f) d = 0.0f;
            d = sqrtf(d);
            if (d > 1.0f) d = 1.0f;
            glRotatef(d * 90.0f, 0.0f, 0.0f, 1.0f);
        }
        float ms = e->model_scale > 0.0f ? e->model_scale : 1.0f;
        if (e->baby_age < 0) ms *= 0.5f;
        if (e->type == PASSIVE_MOB_SLIME || e->type == PASSIVE_MOB_MAGMA_CUBE) {
            float sq = pex_clamp_float(e->wing, -0.20f, 0.55f) / (ms * 0.50f + 1.0f);
            float sxz = 1.0f / (sq + 1.0f);
            float sy = sq + 1.0f;
            glScalef(-ms * sxz, -ms * sy, ms * sxz);
        } else {
            glScalef(-ms, -ms, ms);
        }
        glTranslatef(0.0f, -24.0f * 0.0625f - 0.0078125f, 0.0f);

        if (e->type == PASSIVE_MOB_CHICKEN) {
            glBegin(GL_QUADS);
            passive_render_p125_chicken(e->limb, e->move, e->head_yaw, e->pitch, e->wing);
            glEnd();
        } else {
            glBegin(GL_QUADS);
            passive_render_quad_model(e->type, 0, e->detail, e->limb, e->move, e->head_yaw, e->pitch, e->age, e->sitting, e->rideable, e->held_block, e->sheep_eat_amount, e->sheep_head_pitch);
            glEnd();
            if (e->detail && (e->type == PASSIVE_MOB_SPIDER || e->type == PASSIVE_MOB_CAVE_SPIDER) && tex_mob_spider_eyes.id) {
                glBindTexture(GL_TEXTURE_2D, tex_mob_spider_eyes.id);
                passive_fast_set_texture_dims(&tex_mob_spider_eyes);
                passive_fast_set_tint(1.0f, 1.0f, 1.0f);
                glBlendFunc(GL_ONE, GL_ONE);
                glBegin(GL_QUADS);
                passive_render_quad_model(e->type, 0, e->detail, e->limb, e->move, e->head_yaw, e->pitch, e->age, e->sitting, e->rideable, e->held_block, e->sheep_eat_amount, e->sheep_head_pitch);
                glEnd();
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            if (e->detail && e->type == PASSIVE_MOB_ENDERMAN && tex_mob_enderman_eyes.id) {
                glBindTexture(GL_TEXTURE_2D, tex_mob_enderman_eyes.id);
                passive_fast_set_texture_dims(&tex_mob_enderman_eyes);
                passive_fast_set_tint(1.0f, 1.0f, 1.0f);
                glBlendFunc(GL_ONE, GL_ONE);
                glBegin(GL_QUADS);
                passive_render_quad_model(e->type, 0, e->detail, e->limb, e->move, e->head_yaw, e->pitch, e->age, e->sitting, e->rideable, e->held_block, e->sheep_eat_amount, e->sheep_head_pitch);
                glEnd();
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            if (e->type == PASSIVE_MOB_SLIME) {
                passive_fast_set_tint_alpha(1.0f, 1.0f, 1.0f, 0.45f);
                glDepthMask(GL_FALSE);
                glBegin(GL_QUADS);
                passive_render_quad_model(e->type, 1, e->detail, e->limb, e->move, e->head_yaw, e->pitch, e->age, e->sitting, e->rideable, e->held_block, e->sheep_eat_amount, e->sheep_head_pitch);
                glEnd();
                glDepthMask(GL_TRUE);
                passive_fast_set_tint(1.0f, 1.0f, 1.0f);
            }
            if (e->type == PASSIVE_MOB_ENDERMAN && e->held_block > 0) {
                passive_render_enderman_held_block(e->held_block);
            }
            if (e->type == PASSIVE_MOB_MOOSHROOM) passive_render_mooshroom_mushrooms(e->head_yaw);
            if (e->held_item > 0 && (e->type == PASSIVE_MOB_SKELETON || e->type == PASSIVE_MOB_PIG_ZOMBIE || e->type == PASSIVE_MOB_ZOMBIE)) {
                passive_render_biped_held_item_125(e->type, e->held_item, e->limb, e->move, e->age, e->head_yaw, e->pitch);
            }
            if (e->type == PASSIVE_MOB_IRON_GOLEM) passive_render_golem_rose_item(e->attack_time);
            if (e->type == PASSIVE_MOB_SNOWMAN) passive_render_snowman_pumpkin_item(e->head_yaw, e->pitch);
            if (e->type == PASSIVE_MOB_SHEEP && !e->sheared && tex_mob_sheep_fur.id) {
                float fr, fg, fb;
                passive_sheep_fleece_color(e->fleece_color, &fr, &fg, &fb);
                glBindTexture(GL_TEXTURE_2D, tex_mob_sheep_fur.id);
                passive_fast_set_texture_dims(&tex_mob_sheep_fur);
                passive_fast_set_tint(fr, fg, fb);
                glBegin(GL_QUADS);
                passive_render_quad_model(e->type, 1, e->detail, e->limb, e->move, e->head_yaw, e->pitch, e->age, e->sitting, e->rideable, e->held_block, e->sheep_eat_amount, e->sheep_head_pitch);
                glEnd();
                passive_fast_set_tint(1.0f, 1.0f, 1.0f);
            }
            if (e->type == PASSIVE_MOB_PIG && e->rideable && tex_mob_saddle.id) {
                glBindTexture(GL_TEXTURE_2D, tex_mob_saddle.id);
                passive_fast_set_texture_dims(&tex_mob_saddle);
                glBegin(GL_QUADS);
                passive_render_quad_model(e->type, 1, e->detail, e->limb, e->move, e->head_yaw, e->pitch, e->age, e->sitting, e->rideable, e->held_block, e->sheep_eat_amount, e->sheep_head_pitch);
                glEnd();
            }
        }
        glPopMatrix();
    }

    passive_dragon_crystals_draw_125(partial);

    glDisable(GL_ALPHA_TEST);
    passive_fast_set_tint(1.0f, 1.0f, 1.0f);
    glColor4f(1, 1, 1, 1);
    glPopMatrix();
}


static void passive_villages_write_nbt_file_125(const char *world_dir) {
    if (!world_dir || !world_dir[0] || strncmp(world_dir, "memory:", 7) == 0) return;
    passive_villages_refresh();
    BinBuf b = {0};
    bb_u8(&b, 10); nbt_name(&b, "");
    nbt_int(&b, "Tick", g_ingame_ticks);
    nbt_tag(&b, 9, "Villages"); bb_u8(&b, 10);
    int count = 0;
    for (int i=0;i<PEX_MAX_RUNTIME_VILLAGES;++i) if (g_passive_villages[i].active && g_passive_villages[i].door_count > 0) ++count;
    bb_u32be(&b, (unsigned int)count);
    for (int i=0;i<PEX_MAX_RUNTIME_VILLAGES;++i) {
        PexVillageRuntime *v = &g_passive_villages[i];
        if (!v->active || v->door_count <= 0) continue;
        nbt_int(&b, "ID", v->id);
        nbt_int(&b, "ChunkX", v->chunk_x); nbt_int(&b, "ChunkZ", v->chunk_z);
        nbt_int(&b, "CX", v->cx); nbt_int(&b, "CY", v->cy); nbt_int(&b, "CZ", v->cz);
        nbt_int(&b, "Radius", v->radius);
        nbt_int(&b, "CenterHelperX", v->center_helper_x); nbt_int(&b, "CenterHelperY", v->center_helper_y); nbt_int(&b, "CenterHelperZ", v->center_helper_z);
        nbt_int(&b, "LastAddDoorTimestamp", v->last_add_door_tick);
        nbt_int(&b, "TickCounter", v->tick_counter);
        nbt_int(&b, "NumVillagers", v->villager_count);
        nbt_int(&b, "NumIronGolems", v->golem_count);
        nbt_int(&b, "PlayerReputation", v->player_reputation);
        nbt_tag(&b, 9, "Doors"); bb_u8(&b, 10); bb_u32be(&b, (unsigned int)v->door_count);
        for (int d=0; d<v->door_count; ++d) {
            PexVillageDoorRuntime *door = &v->doors[d];
            nbt_int(&b, "X", door->x); nbt_int(&b, "Y", door->y); nbt_int(&b, "Z", door->z);
            nbt_int(&b, "IDX", door->inside_x); nbt_int(&b, "IDZ", door->inside_z);
            nbt_int(&b, "LastActivity", door->last_activity);
            nbt_int(&b, "Detached", door->detached);
            nbt_int(&b, "OpeningRestriction", door->opening_restriction_counter);
            nbt_end(&b);
        }
        nbt_end(&b);
    }
    nbt_end(&b);
    char path[MAX_PATHBUF];
    snprintf(path, sizeof(path), "%s\\villages.dat", world_dir);
    write_gzip_store(path, b.data, b.len);
    bb_free(&b);
}

static void passive_villages_write_binary_125(FILE *f) {
    passive_villages_refresh();
    char magic[4] = {'P','V','L','G'};
    fwrite(magic, 1, 4, f);
    int version = 1;
    fwrite(&version, sizeof(version), 1, f);
    fwrite(&g_passive_next_village_id, sizeof(g_passive_next_village_id), 1, f);
    int count = 0;
    for (int i=0;i<PEX_MAX_RUNTIME_VILLAGES;++i) if (g_passive_villages[i].active && g_passive_villages[i].door_count > 0) ++count;
    fwrite(&count, sizeof(count), 1, f);
    for (int i=0;i<PEX_MAX_RUNTIME_VILLAGES;++i) {
        PexVillageRuntime *v = &g_passive_villages[i];
        if (!v->active || v->door_count <= 0) continue;
        fwrite(&v->id, sizeof(int), 1, f);
        fwrite(&v->chunk_x, sizeof(int), 1, f); fwrite(&v->chunk_z, sizeof(int), 1, f);
        fwrite(&v->cx, sizeof(int), 1, f); fwrite(&v->cy, sizeof(int), 1, f); fwrite(&v->cz, sizeof(int), 1, f);
        fwrite(&v->center_helper_x, sizeof(int), 1, f); fwrite(&v->center_helper_y, sizeof(int), 1, f); fwrite(&v->center_helper_z, sizeof(int), 1, f);
        fwrite(&v->radius, sizeof(int), 1, f);
        fwrite(&v->last_add_door_tick, sizeof(int), 1, f);
        fwrite(&v->tick_counter, sizeof(int), 1, f);
        fwrite(&v->villager_count, sizeof(int), 1, f); fwrite(&v->golem_count, sizeof(int), 1, f);
        fwrite(&v->player_reputation, sizeof(int), 1, f);
        fwrite(&v->door_count, sizeof(int), 1, f);
        for (int d=0; d<v->door_count; ++d) {
            PexVillageDoorRuntime *door = &v->doors[d];
            fwrite(&door->x, sizeof(int), 1, f); fwrite(&door->y, sizeof(int), 1, f); fwrite(&door->z, sizeof(int), 1, f);
            fwrite(&door->inside_x, sizeof(int), 1, f); fwrite(&door->inside_z, sizeof(int), 1, f);
            fwrite(&door->last_activity, sizeof(int), 1, f);
            fwrite(&door->detached, sizeof(int), 1, f);
            fwrite(&door->opening_restriction_counter, sizeof(int), 1, f);
        }
    }
    if (g_loaded_world_dir[0]) passive_villages_write_nbt_file_125(g_loaded_world_dir);
}

static void passive_villages_read_binary_125(FILE *f, int save_version) {
    memset(g_passive_villages, 0, sizeof(g_passive_villages));
    g_passive_villages_tick = -9999;
    if (save_version < 27) return;
    char magic[4];
    if (fread(magic, 1, 4, f) != 4) return;
    if (memcmp(magic, "PVLG", 4) != 0) return;
    int version = 0, next_id = 1, count = 0;
    if (fread(&version, sizeof(version), 1, f) != 1 || fread(&next_id, sizeof(next_id), 1, f) != 1 || fread(&count, sizeof(count), 1, f) != 1) return;
    if (next_id > 0) g_passive_next_village_id = next_id;
    if (count < 0) count = 0;
    if (count > PEX_MAX_RUNTIME_VILLAGES) count = PEX_MAX_RUNTIME_VILLAGES;
    for (int i=0; i<count; ++i) {
        PexVillageRuntime *v = &g_passive_villages[i];
        memset(v, 0, sizeof(*v));
        v->active = 1;
        if (fread(&v->id, sizeof(int), 1, f) != 1 ||
            fread(&v->chunk_x, sizeof(int), 1, f) != 1 || fread(&v->chunk_z, sizeof(int), 1, f) != 1 ||
            fread(&v->cx, sizeof(int), 1, f) != 1 || fread(&v->cy, sizeof(int), 1, f) != 1 || fread(&v->cz, sizeof(int), 1, f) != 1 ||
            fread(&v->center_helper_x, sizeof(int), 1, f) != 1 || fread(&v->center_helper_y, sizeof(int), 1, f) != 1 || fread(&v->center_helper_z, sizeof(int), 1, f) != 1 ||
            fread(&v->radius, sizeof(int), 1, f) != 1 ||
            fread(&v->last_add_door_tick, sizeof(int), 1, f) != 1 ||
            fread(&v->tick_counter, sizeof(int), 1, f) != 1 ||
            fread(&v->villager_count, sizeof(int), 1, f) != 1 || fread(&v->golem_count, sizeof(int), 1, f) != 1 ||
            fread(&v->player_reputation, sizeof(int), 1, f) != 1 ||
            fread(&v->door_count, sizeof(int), 1, f) != 1) { memset(g_passive_villages,0,sizeof(g_passive_villages)); return; }
        if (v->door_count < 0) v->door_count = 0;
        int stored_doors = v->door_count;
        if (v->door_count > PEX_MAX_VILLAGE_DOORS) v->door_count = PEX_MAX_VILLAGE_DOORS;
        for (int d=0; d<stored_doors; ++d) {
            PexVillageDoorRuntime tmp;
            if (fread(&tmp.x, sizeof(int), 1, f) != 1 || fread(&tmp.y, sizeof(int), 1, f) != 1 || fread(&tmp.z, sizeof(int), 1, f) != 1 ||
                fread(&tmp.inside_x, sizeof(int), 1, f) != 1 || fread(&tmp.inside_z, sizeof(int), 1, f) != 1 ||
                fread(&tmp.last_activity, sizeof(int), 1, f) != 1 ||
                fread(&tmp.detached, sizeof(int), 1, f) != 1 ||
                fread(&tmp.opening_restriction_counter, sizeof(int), 1, f) != 1) { memset(g_passive_villages,0,sizeof(g_passive_villages)); return; }
            if (d < PEX_MAX_VILLAGE_DOORS) v->doors[d] = tmp;
        }
        passive_village_update_radius_and_center_125(v);
    }
    (void)version;
}

static void passive_mobs_write_to_file(FILE *f, const PassiveMob *mobs) {
    int count = 0;
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        if (mobs[i].active && passive_mob_type_valid(mobs[i].type)) ++count;
    }
    fwrite(&count, sizeof(count), 1, f);
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        const PassiveMob *m = &mobs[i];
        if (!m->active || !passive_mob_type_valid(m->type)) continue;
        fwrite(&m->type, sizeof(int), 1, f);
        fwrite(&m->x, sizeof(float), 1, f);
        fwrite(&m->y, sizeof(float), 1, f);
        fwrite(&m->z, sizeof(float), 1, f);
        fwrite(&m->mx, sizeof(float), 1, f);
        fwrite(&m->my, sizeof(float), 1, f);
        fwrite(&m->mz, sizeof(float), 1, f);
        fwrite(&m->yaw, sizeof(float), 1, f);
        fwrite(&m->render_yaw, sizeof(float), 1, f);
        fwrite(&m->health, sizeof(int), 1, f);
        fwrite(&m->prev_health, sizeof(int), 1, f);
        fwrite(&m->damage_remainder, sizeof(int), 1, f);
        fwrite(&m->hurt_time, sizeof(int), 1, f);
        fwrite(&m->death_time, sizeof(int), 1, f);
        fwrite(&m->age, sizeof(int), 1, f);
        fwrite(&m->living_sound_delay, sizeof(int), 1, f);
        fwrite(&m->limb_swing, sizeof(float), 1, f);
        fwrite(&m->limb_amount, sizeof(float), 1, f);
        fwrite(&m->sheared, sizeof(int), 1, f);
        fwrite(&m->fleece_color, sizeof(int), 1, f);
        fwrite(&m->rideable, sizeof(int), 1, f);
        fwrite(&m->chicken_wing_rot, sizeof(float), 1, f);
        fwrite(&m->chicken_dest_pos, sizeof(float), 1, f);
        fwrite(&m->chicken_wing_speed, sizeof(float), 1, f);
        fwrite(&m->egg_timer, sizeof(int), 1, f);
        fwrite(&m->attack_time, sizeof(int), 1, f);
        fwrite(&m->burn_time, sizeof(int), 1, f);
        fwrite(&m->target_mob_index, sizeof(int), 1, f);
        fwrite(&m->baby_age, sizeof(int), 1, f);
        fwrite(&m->sitting, sizeof(int), 1, f);
        fwrite(&m->held_block, sizeof(int), 1, f);
        fwrite(&m->love_time, sizeof(int), 1, f);
        fwrite(&m->owner_id, sizeof(int), 1, f);
        fwrite(&m->tame_state, sizeof(int), 1, f);
        fwrite(&m->collar_color, sizeof(int), 1, f);
        fwrite(&m->village_id, sizeof(int), 1, f);
        fwrite(&m->door_target_x, sizeof(int), 1, f);
        fwrite(&m->door_target_y, sizeof(int), 1, f);
        fwrite(&m->door_target_z, sizeof(int), 1, f);
        fwrite(&m->ai_age, sizeof(int), 1, f);
        fwrite(&m->ai_repath_delay, sizeof(int), 1, f);
        fwrite(&m->path_stuck_check_tick, sizeof(int), 1, f);
        fwrite(&m->path_stuck_check_x, sizeof(float), 1, f);
        fwrite(&m->path_stuck_check_y, sizeof(float), 1, f);
        fwrite(&m->path_stuck_check_z, sizeof(float), 1, f);
        fwrite(&m->path_goal_x, sizeof(float), 1, f);
        fwrite(&m->path_goal_y, sizeof(float), 1, f);
        fwrite(&m->path_goal_z, sizeof(float), 1, f);
        fwrite(m->potion_duration, sizeof(int), 32, f);
        fwrite(m->potion_amplifier, sizeof(int), 32, f);
        fwrite(&m->recently_hit, sizeof(int), 1, f);
        fwrite(&m->last_looting_level, sizeof(int), 1, f);
        fwrite(&m->held_item, sizeof(int), 1, f);
        fwrite(m->equipment, sizeof(int), 4, f);
    }
    passive_villages_write_binary_125(f);
    passive_spawners_write_binary_125(f);
    passive_dragon_state_write_binary_125(f);
    passive_dragon_crystal_state_write_binary_125(f);
}

static void passive_mobs_read_from_file(FILE *f, int version) {
    passive_mobs_reset();
    memset(g_passive_spawners, 0, sizeof(g_passive_spawners));
    if (version < 16) return;
    int count = 0;
    if (fread(&count, sizeof(count), 1, f) != 1) return;
    if (count < 0) count = 0;
    if (count > MAX_PASSIVE_MOBS) count = MAX_PASSIVE_MOBS;
    for (int i = 0; i < count; ++i) {
        PassiveMob *m = &g_passive_mobs[i];
        memset(m, 0, sizeof(*m));
        if (fread(&m->type, sizeof(int), 1, f) != 1 ||
            fread(&m->x, sizeof(float), 1, f) != 1 ||
            fread(&m->y, sizeof(float), 1, f) != 1 ||
            fread(&m->z, sizeof(float), 1, f) != 1 ||
            fread(&m->mx, sizeof(float), 1, f) != 1 ||
            fread(&m->my, sizeof(float), 1, f) != 1 ||
            fread(&m->mz, sizeof(float), 1, f) != 1 ||
            fread(&m->yaw, sizeof(float), 1, f) != 1 ||
            fread(&m->render_yaw, sizeof(float), 1, f) != 1 ||
            fread(&m->health, sizeof(int), 1, f) != 1 ||
            (version >= 28 && fread(&m->prev_health, sizeof(int), 1, f) != 1) ||
            (version >= 28 && fread(&m->damage_remainder, sizeof(int), 1, f) != 1) ||
            fread(&m->hurt_time, sizeof(int), 1, f) != 1 ||
            fread(&m->death_time, sizeof(int), 1, f) != 1 ||
            fread(&m->age, sizeof(int), 1, f) != 1 ||
            fread(&m->living_sound_delay, sizeof(int), 1, f) != 1 ||
            fread(&m->limb_swing, sizeof(float), 1, f) != 1 ||
            fread(&m->limb_amount, sizeof(float), 1, f) != 1 ||
            fread(&m->sheared, sizeof(int), 1, f) != 1 ||
            (version >= 17 && fread(&m->fleece_color, sizeof(int), 1, f) != 1) ||
            fread(&m->rideable, sizeof(int), 1, f) != 1 ||
            fread(&m->chicken_wing_rot, sizeof(float), 1, f) != 1 ||
            fread(&m->chicken_dest_pos, sizeof(float), 1, f) != 1 ||
            fread(&m->chicken_wing_speed, sizeof(float), 1, f) != 1 ||
            fread(&m->egg_timer, sizeof(int), 1, f) != 1) {
            passive_mobs_reset();
            return;
        }
        if (version >= 24) {
            if (fread(&m->attack_time, sizeof(int), 1, f) != 1 ||
                fread(&m->burn_time, sizeof(int), 1, f) != 1 ||
                fread(&m->target_mob_index, sizeof(int), 1, f) != 1 ||
                fread(&m->baby_age, sizeof(int), 1, f) != 1 ||
                fread(&m->sitting, sizeof(int), 1, f) != 1 ||
                fread(&m->held_block, sizeof(int), 1, f) != 1) {
                passive_mobs_reset();
                return;
            }
        } else {
            m->attack_time = m->burn_time = 0;
            m->target_mob_index = -1;
            m->baby_age = m->sitting = m->held_block = 0;
        }
        if (version >= 25) {
            if (fread(&m->love_time, sizeof(int), 1, f) != 1) {
                passive_mobs_reset();
                return;
            }
        } else {
            m->love_time = 0;
        }
        if (version >= 26) {
            if (fread(&m->owner_id, sizeof(int), 1, f) != 1 ||
                fread(&m->tame_state, sizeof(int), 1, f) != 1 ||
                fread(&m->collar_color, sizeof(int), 1, f) != 1 ||
                fread(&m->village_id, sizeof(int), 1, f) != 1 ||
                fread(&m->door_target_x, sizeof(int), 1, f) != 1 ||
                fread(&m->door_target_y, sizeof(int), 1, f) != 1 ||
                fread(&m->door_target_z, sizeof(int), 1, f) != 1) {
                passive_mobs_reset();
                return;
            }
        } else {
            m->owner_id = m->sheared && (m->type == PASSIVE_MOB_WOLF || m->type == PASSIVE_MOB_OCELOT) ? PEX_MOB_OWNER_SINGLEPLAYER : PEX_MOB_OWNER_NONE;
            m->tame_state = (m->type == PASSIVE_MOB_OCELOT && m->fleece_color > 0) ? m->fleece_color : (m->owner_id ? 1 : 0);
            m->collar_color = 14;
            m->village_id = -1;
            m->door_target_x = m->door_target_y = m->door_target_z = 0;
        }
        m->door_open_time = 0;
        m->ai_task_mask = 0;
        m->path_len = m->path_index = m->path_recalc_cooldown = 0;
        if (version >= 29) {
            if (fread(&m->ai_age, sizeof(int), 1, f) != 1 ||
                fread(&m->ai_repath_delay, sizeof(int), 1, f) != 1 ||
                fread(&m->path_stuck_check_tick, sizeof(int), 1, f) != 1 ||
                fread(&m->path_stuck_check_x, sizeof(float), 1, f) != 1 ||
                fread(&m->path_stuck_check_y, sizeof(float), 1, f) != 1 ||
                fread(&m->path_stuck_check_z, sizeof(float), 1, f) != 1 ||
                fread(&m->path_goal_x, sizeof(float), 1, f) != 1 ||
                fread(&m->path_goal_y, sizeof(float), 1, f) != 1 ||
                fread(&m->path_goal_z, sizeof(float), 1, f) != 1) {
                passive_mobs_reset();
                return;
            }
        } else {
            m->ai_age = rand() % 100;
            m->ai_repath_delay = 0;
            m->path_stuck_check_tick = m->age;
            m->path_stuck_check_x = m->x;
            m->path_stuck_check_y = m->y;
            m->path_stuck_check_z = m->z;
            m->path_goal_x = m->x;
            m->path_goal_y = m->y;
            m->path_goal_z = m->z;
        }
        if (version >= 23) {
            if (fread(m->potion_duration, sizeof(int), 32, f) != 32 ||
                fread(m->potion_amplifier, sizeof(int), 32, f) != 32) {
                passive_mobs_reset();
                return;
            }
        } else {
            memset(m->potion_duration, 0, sizeof(m->potion_duration));
            memset(m->potion_amplifier, 0, sizeof(m->potion_amplifier));
        }
        if (version >= 33) {
            if (fread(&m->recently_hit, sizeof(int), 1, f) != 1 ||
                fread(&m->last_looting_level, sizeof(int), 1, f) != 1 ||
                fread(&m->held_item, sizeof(int), 1, f) != 1 ||
                fread(m->equipment, sizeof(int), 4, f) != 4) {
                passive_mobs_reset();
                return;
            }
        } else {
            m->recently_hit = 0;
            m->last_looting_level = 0;
            m->held_item = 0;
            memset(m->equipment, 0, sizeof(m->equipment));
            passive_mob_apply_default_equipment_125(m);
        }
        if (!passive_mob_type_valid(m->type) || m->health <= 0 ||
            !isfinite(m->x) || !isfinite(m->y) || !isfinite(m->z) ||
            m->x < (float)g_flat_world_origin_x - 16.0f ||
            m->x > (float)(g_flat_world_origin_x + FLAT_WORLD_SIZE) + 16.0f ||
            m->z < (float)g_flat_world_origin_z - 16.0f ||
            m->z > (float)(g_flat_world_origin_z + FLAT_WORLD_SIZE) + 16.0f ||
            m->y < (float)FLAT_WORLD_Y_MIN - 16.0f ||
            m->y > (float)FLAT_WORLD_Y_MAX + 16.0f) {
            memset(m, 0, sizeof(*m));
            continue;
        }
        passive_mob_clamp_health_current_125(m);
        if (version < 28) {
            m->prev_health = m->health;
            m->damage_remainder = 0;
        }
        m->active = 1;
        m->prev_x = m->x; m->prev_y = m->y; m->prev_z = m->z;
        m->prev_yaw = m->yaw;
        m->prev_render_yaw = m->render_yaw;
        m->width = passive_mob_width_for_type(m->type);
        m->height = passive_mob_height_for_type(m->type);
        if (m->held_item < 0) m->held_item = 0;
        for (int ei = 0; ei < 4; ++ei) if (m->equipment[ei] < 0) m->equipment[ei] = 0;
        passive_mob_apply_default_equipment_125(m);
        if (passive_mob_is_slime_family(m->type)) {
            int sz = m->fleece_color;
            if (sz != 1 && sz != 2 && sz != 4) sz = 1;
            m->fleece_color = sz;
            m->width = 0.6f * (float)sz;
            m->height = 0.6f * (float)sz;
        }
        m->prev_limb_swing = m->limb_swing;
        m->prev_limb_amount = m->limb_amount;
        m->chicken_prev_wing_rot = m->chicken_wing_rot;
        m->chicken_prev_dest_pos = m->chicken_dest_pos;
        if (m->type == PASSIVE_MOB_CHICKEN && m->egg_timer <= 0) m->egg_timer = rand() % 6000 + 6000;
        m->damage_cooldown = 0;
        m->death_drops_done = m->death_time > 0;
        if (m->target_mob_index < -1 || m->target_mob_index >= MAX_PASSIVE_MOBS) m->target_mob_index = -1;
        m->jump_cooldown = 0;
        m->stuck_ticks = 0;
        if (m->ai_age < 0 || m->ai_age > 6000) m->ai_age = rand() % 100;
        if (m->ai_repath_delay < 0 || m->ai_repath_delay > 200) m->ai_repath_delay = 0;
        if (!isfinite(m->path_stuck_check_x) || !isfinite(m->path_stuck_check_y) || !isfinite(m->path_stuck_check_z)) {
            m->path_stuck_check_x = m->x;
            m->path_stuck_check_y = m->y;
            m->path_stuck_check_z = m->z;
        }
        if (!isfinite(m->path_goal_x) || !isfinite(m->path_goal_y) || !isfinite(m->path_goal_z)) {
            m->path_goal_x = m->x;
            m->path_goal_y = m->y;
            m->path_goal_z = m->z;
        }
        m->wander_cooldown = 0;
        if (m->owner_id != PEX_MOB_OWNER_SINGLEPLAYER) m->owner_id = PEX_MOB_OWNER_NONE;
        if (m->collar_color < 0 || m->collar_color > 15) m->collar_color = 14;
        if (m->type == PASSIVE_MOB_WOLF || m->type == PASSIVE_MOB_OCELOT) m->sheared = passive_mob_is_owned_by_player(m) ? 1 : m->sheared;
    }
    passive_villages_read_binary_125(f, version);
    passive_spawners_read_binary_125(f, version);
    passive_dragon_state_read_binary_125(f, version);
    passive_dragon_crystal_state_read_binary_125(f, version);
}
