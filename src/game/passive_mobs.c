/* Beta 1.0 passive mobs: pig, sheep, cow, chicken. Included in the unity build
   after inventory.c so it can reuse the existing world/item/collision helpers. */

#define PASSIVE_MOB_SAVE_VERSION 16

/* Performance knobs: this port renders animal models with immediate-mode
   cube parts.  Keep active/rendered passive mobs close to b1.0 density instead
   of letting the spawn patch fill the entire active window. */
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
#define PEX_PASSIVE_TARGET_CAP 8
#define PEX_PASSIVE_INITIAL_TARGET 6
#define PEX_PASSIVE_RENDER_LIMIT 6
#define PEX_PASSIVE_RENDER_DIST 40.0f
#else
/* Match Beta b1.0 much closer: creature cap is 20 for the 17x17 eligible
   chunk area around one player.  Rendering is separately culled so this does
   not force every animal model to draw every frame. */
#define PEX_PASSIVE_TARGET_CAP 20
#define PEX_PASSIVE_INITIAL_TARGET 12
#define PEX_PASSIVE_RENDER_LIMIT 16
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

static float passive_lerp_angle(float a, float b, float partial) {
    return a + pex_wrap_degrees(b - a) * partial;
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
    switch (type) {
        case PASSIVE_MOB_PIG: return "Pig";
        case PASSIVE_MOB_SHEEP: return "Sheep";
        case PASSIVE_MOB_COW: return "Cow";
        case PASSIVE_MOB_CHICKEN: return "Chicken";
        default: return "Mob";
    }
}

static float passive_mob_width_for_type(int type) {
    return type == PASSIVE_MOB_CHICKEN ? 0.3f : 0.9f;
}
static float passive_mob_height_for_type(int type) {
    if (type == PASSIVE_MOB_CHICKEN) return 0.4f;
    if (type == PASSIVE_MOB_PIG) return 0.9f;
    return 1.3f;
}
static int passive_mob_type_valid(int type) {
    return type >= PASSIVE_MOB_PIG && type <= PASSIVE_MOB_CHICKEN;
}

static int passive_mob_health_for_type(int type) {
    switch (type) {
        case PASSIVE_MOB_PIG: return 10;
        case PASSIVE_MOB_COW: return 10;
        case PASSIVE_MOB_SHEEP: return 8;
        case PASSIVE_MOB_CHICKEN: return 4;
        default: return 1;
    }
}
static float passive_mob_sound_volume(int type) {
    return type == PASSIVE_MOB_COW ? 0.4f : 1.0f;
}
static const char *passive_mob_living_sound(int type) {
    switch (type) {
        case PASSIVE_MOB_PIG: return "mob.pig";
        case PASSIVE_MOB_SHEEP: return "mob.sheep";
        case PASSIVE_MOB_COW: return "mob.cow";
        case PASSIVE_MOB_CHICKEN: return "mob.chicken";
        default: return NULL;
    }
}
static const char *passive_mob_hurt_sound(int type) {
    switch (type) {
        case PASSIVE_MOB_PIG: return "mob.pig";
        case PASSIVE_MOB_SHEEP: return "mob.sheep";
        case PASSIVE_MOB_COW: return "mob.cowhurt";
        case PASSIVE_MOB_CHICKEN: return "mob.chickenhurt";
        default: return NULL;
    }
}
static const char *passive_mob_death_sound(int type) {
    switch (type) {
        case PASSIVE_MOB_PIG: return "mob.pigdeath";
        case PASSIVE_MOB_SHEEP: return "mob.sheep";
        case PASSIVE_MOB_COW: return "mob.cowhurt";
        case PASSIVE_MOB_CHICKEN: return "mob.chickenhurt";
        default: return NULL;
    }
}
static void passive_mob_drop_stack(PassiveMob *m, int item_id, int count) {
    if (!m || item_id <= 0 || count <= 0) return;
    for (int i = 0; i < count; ++i) {
        spawn_item_stack(m->x, m->y + 0.5f, m->z, make_stack(item_id, 1, 0), 1);
    }
}

static void passive_mob_drop_on_death(PassiveMob *m) {
    if (!m || m->death_drops_done) return;
    m->death_drops_done = 1;
    switch (m->type) {
        case PASSIVE_MOB_PIG:
            passive_mob_drop_stack(m, ITEM_PORK_RAW, rand() % 3);
            break;
        case PASSIVE_MOB_COW:
            passive_mob_drop_stack(m, ITEM_LEATHER, rand() % 3);
            /* No beef item exists in this Beta-era item table. */
            break;
        case PASSIVE_MOB_CHICKEN:
            passive_mob_drop_stack(m, ITEM_FEATHER, rand() % 3);
            /* No raw/cooked chicken item exists in this Beta-era item table. */
            break;
        case PASSIVE_MOB_SHEEP:
            if (!m->sheared) passive_mob_drop_stack(m, BLOCK_WOOL, 1);
            break;
        default:
            break;
    }
}

static void passive_mobs_reset(void) {
    memset(g_passive_mobs, 0, sizeof(g_passive_mobs));
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
    m->living_sound_delay = -120;
    m->jump_cooldown = 0;
    m->stuck_ticks = 0;
    m->wander_cooldown = 20 + (rand() % 80);
    if (type == PASSIVE_MOB_CHICKEN) {
        m->chicken_wing_speed = 1.0f;
        m->egg_timer = rand() % 6000 + 6000;
    }
    pex_logf("passive mob spawn type=%s pos=%.2f,%.2f,%.2f", passive_mob_name(type), x, y, z);
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

static int passive_mob_can_spawn_at(int type, int x, int y, int z) {
    if (!passive_mob_type_valid(type)) return 0;
    if (!flat_chunk_generated_at_block(x, z)) return 0;
    if (!passive_mob_can_stand_at(x, y, z)) return 0;
    if (!passive_mob_has_spawn_light(x, y, z)) return 0;
    float fx = (float)x + 0.5f;
    float fy = (float)y;
    float fz = (float)z + 0.5f;
    if (!passive_mob_far_enough_from_player(fx, fy, fz)) return 0;
    if (!passive_mob_far_enough_from_world_spawn(fx, fy, fz)) return 0;
    return passive_mob_has_spawn_clearance(type, fx, fy, fz);
}

static int passive_mob_column_spawn_y(int type, int x, int z) {
    if (x < g_flat_world_origin_x + 1 || x >= g_flat_world_origin_x + FLAT_WORLD_SIZE - 1 ||
        z < g_flat_world_origin_z + 1 || z >= g_flat_world_origin_z + FLAT_WORLD_SIZE - 1) return -9999;
    for (int y = FLAT_WORLD_Y_MAX - 2; y >= FLAT_WORLD_Y_MIN + 1; --y) {
        if (passive_mob_can_spawn_at(type, x, y, z)) return y;
    }
    return -9999;
}

static void passive_mobs_try_natural_spawn(void) {
    if (g_mp_connected || g_player_dead) return;
    int cap = passive_mob_target_cap();
    if (passive_mob_count() >= cap) return;
    /* Vanilla b1.0 does not keep shoving passive groups into the active area
       every few frames.  The previous 5-tick retry loop was the source of the
       huge animal piles and CPU spike. */
    if ((g_ingame_ticks % 20) != 0) return;

    int pcx = floor_div16((int)floorf(g_player_x));
    int pcz = floor_div16((int)floorf(g_player_z));
    int count_now = passive_mob_count();
    int batches = count_now < 8 ? 6 : (count_now < 16 ? 4 : 2);
    for (int batch = 0; batch < batches && passive_mob_count() < cap; ++batch) {
        /* Sample a few nearby chunks instead of scanning the whole active world. */
        if (passive_mob_count() >= cap - 2 && (rand() % 3) != 0) continue;
        int cx = pcx + (rand() % 17) - 8;
        int cz = pcz + (rand() % 17) - 8;
        int base_x = cx * 16 + (rand() & 15);
        int base_z = cz * 16 + (rand() & 15);
        int r = rand() % 4;
        int type = r == 0 ? PASSIVE_MOB_SHEEP : r == 1 ? PASSIVE_MOB_PIG : r == 2 ? PASSIVE_MOB_CHICKEN : PASSIVE_MOB_COW;

        int spawned = 0;
        for (int tries = 0; tries < 16 && spawned < 4 && passive_mob_count() < cap; ++tries) {
            int x = base_x + (rand() % 6) - (rand() % 6);
            int z = base_z + (rand() % 6) - (rand() % 6);
            int y = passive_mob_column_spawn_y(type, x, z);
            if (y == -9999) continue;
            PassiveMob *m = passive_mob_alloc();
            if (!m) return;
            passive_mob_init(m, type, (float)x + 0.5f, (float)y, (float)z + 0.5f);
            ++spawned;
            g_save_dirty = 1;
        }
        if (spawned > 0) pex_logf("passive natural spawn group type=%s count=%d total=%d", passive_mob_name(type), spawned, passive_mob_count());
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
        int r = rand() % 4;
        int type = r == 0 ? PASSIVE_MOB_SHEEP : r == 1 ? PASSIVE_MOB_PIG : r == 2 ? PASSIVE_MOB_CHICKEN : PASSIVE_MOB_COW;
        int y = passive_mob_column_spawn_y(type, x, z);
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
    passive_mobs_ensure_population(PEX_PASSIVE_INITIAL_TARGET, 1800);
}

static void passive_mob_choose_target(PassiveMob *m) {
    if (!m || m->death_time > 0) return;
    int base_y = (int)floorf(m->y + 0.01f);
    int best_x = 0, best_y = 0, best_z = 0;
    float best_weight = -99999.0f;
    for (int i = 0; i < 14; ++i) {
        int x = (int)floorf(m->x + (float)(rand() % 15) - 7.0f);
        int z = (int)floorf(m->z + (float)(rand() % 15) - 7.0f);
        /* Passive animals in b1.0 wander; they do not pathfind up cliffs.
           Prefer same-level ground and allow one block lower. Only use +1 when
           the mob has been stuck for a while so it can escape a tiny ledge. */
        int y_candidates[4];
        int n = 0;
        y_candidates[n++] = base_y;
        y_candidates[n++] = base_y - 1;
        if (m->stuck_ticks > 12) y_candidates[n++] = base_y + 1;
        y_candidates[n++] = base_y - 2;
        for (int yi = 0; yi < n; ++yi) {
            int y = y_candidates[yi];
            if (!passive_mob_ground_target_ok(x, y, z)) continue;
            float dx = ((float)x + 0.5f) - m->x;
            float dz = ((float)z + 0.5f) - m->z;
            float dist2 = dx * dx + dz * dz;
            if (dist2 < 1.0f) continue;
            int below = flat_get_block(x, y - 1, z);
            float w = below == BLOCK_GRASS ? 10.0f : (float)flat_combined_light(x, y, z) / 15.0f - 0.5f;
            w -= fabsf((float)y - m->y) * 3.0f;
            if (dist2 > 64.0f) w -= 3.0f;
            if (w > best_weight) { best_weight = w; best_x = x; best_y = y; best_z = z; }
        }
    }
    if (best_weight > -9999.0f) {
        m->target_x = (float)best_x + 0.5f;
        m->target_y = (float)best_y;
        m->target_z = (float)best_z + 0.5f;
        m->has_path_target = 1;
        m->wander_cooldown = 60 + (rand() % 120);
    } else {
        m->has_path_target = 0;
        m->wander_cooldown = 40 + (rand() % 80);
    }
}

static int held_damage_vs_mob(void) {
    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    if (stack_empty(held)) return 1;
    switch (held->id) {
        case ITEM_WOODEN_SWORD: return 4;
        case ITEM_STONE_SWORD: return 5;
        case ITEM_IRON_SWORD: return 6;
        case ITEM_DIAMOND_SWORD: return 7;
        case ITEM_GOLD_SWORD: return 4;
        case ITEM_WOODEN_AXE: return 3;
        case ITEM_STONE_AXE: return 4;
        case ITEM_IRON_AXE: return 5;
        case ITEM_DIAMOND_AXE: return 6;
        case ITEM_GOLD_AXE: return 3;
        default: return 1;
    }
}

static int passive_mob_can_shear_sheep(const PassiveMob *m) {
    if (!m || !m->active || m->death_time > 0) return 0;
    if (m->type != PASSIVE_MOB_SHEEP || m->sheared) return 0;
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
        spawn_item_stack(m->x, m->y + 1.0f, m->z, make_stack(BLOCK_WOOL, 1, 0), 1);
    }
    restart_hand_swing();
    g_save_dirty = 1;
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

static void passive_mob_start_death(PassiveMob *m) {
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
    player_add_experience(1 + (rand() % 3));
    passive_mob_drop_on_death(m);
}

static void passive_mob_take_damage(PassiveMob *m, int damage) {
    if (!m || !m->active || m->death_time > 0) return;
    if (damage <= 0) return;
    if (m->damage_cooldown > 0) return;
    if (damage < 1) damage = 1;

    m->damage_cooldown = 10;
    m->health -= damage;
    player_add_exhaustion(0.3f);
    m->hurt_time = 10;
    passive_mob_apply_player_knockback(m);

    if (m->health <= 0) {
        passive_mob_start_death(m);
    } else {
        const char *s = passive_mob_hurt_sound(m->type);
        if (s) pex_sound_play_at(s, m->x, m->y, m->z, passive_mob_sound_volume(m->type),
                                 (pex_rand_float01() - pex_rand_float01()) * 0.2f + 1.0f);
    }
    g_save_dirty = 1;
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

static int passive_mobs_attack_from_player(void) {
    if (g_mp_connected || g_player_dead) return 0;
    float mob_t = 0.0f;
    PassiveMob *m = passive_mob_raycast(5.0f, &mob_t);
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
    passive_mob_take_damage(m, held_damage_vs_mob());
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
    if (m->type == PASSIVE_MOB_COW && held && !stack_empty(held) && held->id == ITEM_BUCKET_EMPTY) {
        consume_held_stack_one(held, ITEM_BUCKET_MILK);
        restart_hand_swing();
        g_save_dirty = 1;
        return 1;
    }
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
    if (m->jump_cooldown > 0) --m->jump_cooldown;
    if (m->wander_cooldown > 0) --m->wander_cooldown;
    ++m->age;
}

static int passive_mob_tick_death(PassiveMob *m) {
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
        const char *s = passive_mob_living_sound(m->type);
        if (s) pex_sound_play_at(s, m->x, m->y, m->z, passive_mob_sound_volume(m->type),
                                 (pex_rand_float01() - pex_rand_float01()) * 0.2f + 1.0f);
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

static float passive_mob_tick_ai(PassiveMob *m, int in_liquid) {
    if (!m->has_path_target || m->wander_cooldown <= 0 || (in_liquid && (rand() % 25) == 0)) {
        passive_mob_choose_target(m);
    }

    float forward = 0.0f;
    if (m->has_path_target) {
        float tx = m->target_x - m->x;
        float tz = m->target_z - m->z;
        float dist2 = tx * tx + tz * tz;
        if (dist2 < 0.55f * 0.55f) {
            m->has_path_target = 0;
        } else {
            float desired = atan2f(tz, tx) * 57.29578f - 90.0f;
            float turn = pex_wrap_degrees(desired - m->yaw);
            turn = pex_clamp_float(turn, -30.0f, 30.0f);
            m->yaw += turn;
            forward = in_liquid ? 0.010f : 0.026f;
            if (m->type != PASSIVE_MOB_CHICKEN && m->target_y > m->y + 0.35f &&
                m->on_ground && m->stuck_ticks > 8) {
                passive_mob_request_jump(m, "higher-target");
            }
        }
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

static void passive_mob_tick_species_behavior(PassiveMob *m) {
    if (m->type != PASSIVE_MOB_CHICKEN) return;
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
    if (m->stuck_ticks > 35) {
        m->has_path_target = 0;
        m->wander_cooldown = 20 + (rand() % 40);
        m->stuck_ticks = 0;
        pex_logf_trace("passive mob dropped stuck target type=%s pos=%.2f,%.2f,%.2f",
                       passive_mob_name(m->type), m->x, m->y, m->z);
    }

    float target_amount = (speed > 0.01f && (m->on_ground || in_liquid))
        ? pex_clamp_float(speed * 7.0f, 0.0f, 1.0f) : 0.0f;
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
    if (passive_mob_tick_death(m)) return;
    passive_mob_tick_sounds(m);
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
    passive_mobs_enforce_cap();
    passive_mobs_try_natural_spawn();
    for (int i = 0; i < MAX_PASSIVE_MOBS; ++i) {
        PassiveMob *m = &g_passive_mobs[i];
        if (!m->active) continue;
        int stride = passive_mob_tick_stride(m);
        if (stride > 1 && ((g_ingame_ticks + i) % stride) != 0) {
            /* Keep interpolation stable while far mobs are simulated at a lower
               rate.  Near animals still tick every tick. */
            m->prev_x = m->x; m->prev_y = m->y; m->prev_z = m->z;
            m->prev_yaw = m->yaw; m->prev_render_yaw = m->render_yaw;
            m->prev_limb_swing = m->limb_swing; m->prev_limb_amount = m->limb_amount;
            continue;
        }
        passive_mob_tick_living(m);
    }
}

static Texture *passive_mob_texture_for_type(int type) {
    switch (type) {
        case PASSIVE_MOB_PIG: return tex_mob_pig.id ? &tex_mob_pig : &tex_steve;
        case PASSIVE_MOB_SHEEP: return tex_mob_sheep.id ? &tex_mob_sheep : &tex_steve;
        case PASSIVE_MOB_COW: return tex_mob_cow.id ? &tex_mob_cow : &tex_steve;
        case PASSIVE_MOB_CHICKEN: return tex_mob_chicken.id ? &tex_mob_chicken : &tex_steve;
        default: return &tex_steve;
    }
}

typedef struct PassiveMobRenderEntry {
    int active;
    int type;
    int sheared;
    int rideable;
    int hurt;
    int detail;
    float x, y, z;
    float yaw;
    float head_yaw;
    float pitch;
    float move;
    float limb;
    float wing;
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
        e->rideable = m->rideable;
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
        e->death_time = m->death_time > 0 ? (float)m->death_time + partial : 0.0f;
        if (m->type == PASSIVE_MOB_CHICKEN) {
            float wr = m->chicken_prev_wing_rot + (m->chicken_wing_rot - m->chicken_prev_wing_rot) * partial;
            float wd = m->chicken_prev_dest_pos + (m->chicken_dest_pos - m->chicken_prev_dest_pos) * partial;
            e->wing = (sinf(wr) + 1.0f) * wd;
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

static void passive_render_quad_model(int type, int fur_layer, int detail, float limb, float move, float head_yaw, float head_pitch, float chicken_wing) {
    float leg1 = cosf(limb * 0.6662f) * 1.4f * move * 57.29578f;
    float leg2 = cosf(limb * 0.6662f + (float)M_PI) * 1.4f * move * 57.29578f;
    float body_pitch = 90.0f;
    (void)chicken_wing;

    if (type == PASSIVE_MOB_PIG) {
        steve_part(0, 0, 0, 12, -6, -4, -4, -8, 8, 8, 8, fur_layer ? 0.5f : 0.0f, 0, head_pitch, head_yaw, 0);
        steve_part(28, 8, 0, 11, 2, -5, -10, -7, 10, 16, 8, 0.0f, 0, body_pitch, 0, 0);
        if (!detail) return;
        steve_part(0, 16, -3, 18, 7, -2, 0, -2, 4, 6, 4, 0.0f, 0, leg1, 0, 0);
        steve_part(0, 16,  3, 18, 7, -2, 0, -2, 4, 6, 4, 0.0f, 0, leg2, 0, 0);
        steve_part(0, 16, -3, 18, -5, -2, 0, -2, 4, 6, 4, 0.0f, 0, leg2, 0, 0);
        steve_part(0, 16,  3, 18, -5, -2, 0, -2, 4, 6, 4, 0.0f, 0, leg1, 0, 0);
    } else if (type == PASSIVE_MOB_SHEEP) {
        if (fur_layer) {
            steve_part(0, 0, 0, 6, -8, -3, -4, -4, 6, 6, 6, 0.6f, 0, head_pitch, head_yaw, 0);
            steve_part(28, 8, 0, 5, 2, -4, -10, -7, 8, 16, 6, 1.75f, 0, body_pitch, 0, 0);
            if (!detail) return;
            steve_part(0, 16, -3, 12, 7, -2, 0, -2, 4, 6, 4, 0.5f, 0, leg1, 0, 0);
            steve_part(0, 16,  3, 12, 7, -2, 0, -2, 4, 6, 4, 0.5f, 0, leg2, 0, 0);
            steve_part(0, 16, -3, 12, -5, -2, 0, -2, 4, 6, 4, 0.5f, 0, leg2, 0, 0);
            steve_part(0, 16,  3, 12, -5, -2, 0, -2, 4, 6, 4, 0.5f, 0, leg1, 0, 0);
        } else {
            steve_part(0, 0, 0, 6, -8, -3, -4, -6, 6, 6, 8, 0.0f, 0, head_pitch, head_yaw, 0);
            steve_part(28, 8, 0, 5, 2, -4, -10, -7, 8, 16, 6, 0.0f, 0, body_pitch, 0, 0);
            if (!detail) return;
            steve_part(0, 16, -3, 12, 7, -2, 0, -2, 4, 12, 4, 0.0f, 0, leg1, 0, 0);
            steve_part(0, 16,  3, 12, 7, -2, 0, -2, 4, 12, 4, 0.0f, 0, leg2, 0, 0);
            steve_part(0, 16, -3, 12, -5, -2, 0, -2, 4, 12, 4, 0.0f, 0, leg2, 0, 0);
            steve_part(0, 16,  3, 12, -5, -2, 0, -2, 4, 12, 4, 0.0f, 0, leg1, 0, 0);
        }
    } else if (type == PASSIVE_MOB_COW) {
        steve_part(0, 0, 0, 4, -8, -4, -4, -6, 8, 8, 6, 0.0f, 0, head_pitch, head_yaw, 0);
        steve_part(18, 4, 0, 5, 2, -6, -10, -7, 12, 18, 10, 0.0f, 0, body_pitch, 0, 0);
        if (!detail) return;
        steve_part(22, 0, 0, 3, -7, -4, -5, -4, 1, 3, 1, 0.0f, 0, head_pitch, head_yaw, 0);
        steve_part(22, 0, 0, 3, -7,  4, -5, -4, 1, 3, 1, 0.0f, 0, head_pitch, head_yaw, 0);
        steve_part(52, 0, 0, 14, 6, -2, -3, 0, 4, 6, 2, 0.0f, 0, 90.0f, 0, 0);
        steve_part(0, 16, -4, 12, 7, -2, 0, -2, 4, 12, 4, 0.0f, 0, leg1, 0, 0);
        steve_part(0, 16,  4, 12, 7, -2, 0, -2, 4, 12, 4, 0.0f, 0, leg2, 0, 0);
        steve_part(0, 16, -4, 12, -6, -2, 0, -2, 4, 12, 4, 0.0f, 0, leg2, 0, 0);
        steve_part(0, 16,  4, 12, -6, -2, 0, -2, 4, 12, 4, 0.0f, 0, leg1, 0, 0);
    }
}

static void passive_render_chicken(int detail, float limb, float move, float head_yaw, float head_pitch, float wing) {
    float leg1 = cosf(limb * 0.6662f) * 1.4f * move * 57.29578f;
    float leg2 = cosf(limb * 0.6662f + (float)M_PI) * 1.4f * move * 57.29578f;
    steve_part(0, 0, 0, 15, -4, -2, -6, -2, 4, 6, 3, 0.0f, 0, head_pitch, head_yaw, 0);
    steve_part(0, 9, 0, 16, 0, -3, -4, -3, 6, 8, 6, 0.0f, 0, 90.0f, 0, 0);
    if (!detail) return;
    steve_part(14, 0, 0, 15, -4, -2, -4, -4, 4, 2, 2, 0.0f, 0, head_pitch, head_yaw, 0);
    steve_part(14, 4, 0, 15, -4, -1, -2, -3, 2, 2, 2, 0.0f, 0, head_pitch, head_yaw, 0);
    steve_part(26, 0, -2, 19, 1, -1, 0, -3, 3, 5, 3, 0.0f, 0, leg1, 0, 0);
    steve_part(26, 0,  1, 19, 1, -1, 0, -3, 3, 5, 3, 0.0f, 0, leg2, 0, 0);
    steve_part(24, 13, -4, 13, 0, 0, 0, -3, 1, 4, 6, 0.0f, 0, 0, 0, wing * 57.29578f);
    steve_part(24, 13,  4, 13, 0, -1, 0, -3, 1, 4, 6, 0.0f, 0, 0, 0, -wing * 57.29578f);
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
        if (!t || !t->id) continue;

        float shadow_size = 0.7f;
        if (e->type == PASSIVE_MOB_CHICKEN) shadow_size = 0.3f;
        draw_java_entity_shadow(e->x, e->y, e->z, shadow_size, 1.0f);

        glPushMatrix();
        glBindTexture(GL_TEXTURE_2D, t->id);
        steve_set_texture_dims(t);
        if (e->hurt) steve_set_tint(1.0f, 0.35f, 0.35f);
        else steve_set_tint(1.0f, 1.0f, 1.0f);
        glTranslatef(e->x, e->y, e->z);
        glRotatef(180.0f - e->yaw, 0.0f, 1.0f, 0.0f);
        if (e->death_time > 0.0f) {
            float d = ((e->death_time - 1.0f) / 20.0f) * 1.6f;
            if (d < 0.0f) d = 0.0f;
            d = sqrtf(d);
            if (d > 1.0f) d = 1.0f;
            glRotatef(d * 90.0f, 0.0f, 0.0f, 1.0f);
        }
        glScalef(-1.0f, -1.0f, 1.0f);
        glTranslatef(0.0f, -24.0f * 0.0625f - 0.0078125f, 0.0f);

        if (e->type == PASSIVE_MOB_CHICKEN) {
            passive_render_chicken(e->detail, e->limb, e->move, e->head_yaw, e->pitch, e->wing);
        } else {
            passive_render_quad_model(e->type, 0, e->detail, e->limb, e->move, e->head_yaw, e->pitch, 0.0f);
            if (e->detail && e->type == PASSIVE_MOB_SHEEP && !e->sheared && tex_mob_sheep_fur.id) {
                glBindTexture(GL_TEXTURE_2D, tex_mob_sheep_fur.id);
                steve_set_texture_dims(&tex_mob_sheep_fur);
                passive_render_quad_model(e->type, 1, e->detail, e->limb, e->move, e->head_yaw, e->pitch, 0.0f);
            }
            if (e->detail && e->type == PASSIVE_MOB_PIG && e->rideable && tex_mob_saddle.id) {
                glBindTexture(GL_TEXTURE_2D, tex_mob_saddle.id);
                steve_set_texture_dims(&tex_mob_saddle);
                passive_render_quad_model(e->type, 1, e->detail, e->limb, e->move, e->head_yaw, e->pitch, 0.0f);
            }
        }
        glPopMatrix();
    }

    glDisable(GL_ALPHA_TEST);
    steve_set_tint(1.0f, 1.0f, 1.0f);
    glColor4f(1, 1, 1, 1);
    glPopMatrix();
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
        fwrite(&m->hurt_time, sizeof(int), 1, f);
        fwrite(&m->death_time, sizeof(int), 1, f);
        fwrite(&m->age, sizeof(int), 1, f);
        fwrite(&m->living_sound_delay, sizeof(int), 1, f);
        fwrite(&m->limb_swing, sizeof(float), 1, f);
        fwrite(&m->limb_amount, sizeof(float), 1, f);
        fwrite(&m->sheared, sizeof(int), 1, f);
        fwrite(&m->rideable, sizeof(int), 1, f);
        fwrite(&m->chicken_wing_rot, sizeof(float), 1, f);
        fwrite(&m->chicken_dest_pos, sizeof(float), 1, f);
        fwrite(&m->chicken_wing_speed, sizeof(float), 1, f);
        fwrite(&m->egg_timer, sizeof(int), 1, f);
    }
}

static void passive_mobs_read_from_file(FILE *f, int version) {
    passive_mobs_reset();
    if (version < PASSIVE_MOB_SAVE_VERSION) return;
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
            fread(&m->hurt_time, sizeof(int), 1, f) != 1 ||
            fread(&m->death_time, sizeof(int), 1, f) != 1 ||
            fread(&m->age, sizeof(int), 1, f) != 1 ||
            fread(&m->living_sound_delay, sizeof(int), 1, f) != 1 ||
            fread(&m->limb_swing, sizeof(float), 1, f) != 1 ||
            fread(&m->limb_amount, sizeof(float), 1, f) != 1 ||
            fread(&m->sheared, sizeof(int), 1, f) != 1 ||
            fread(&m->rideable, sizeof(int), 1, f) != 1 ||
            fread(&m->chicken_wing_rot, sizeof(float), 1, f) != 1 ||
            fread(&m->chicken_dest_pos, sizeof(float), 1, f) != 1 ||
            fread(&m->chicken_wing_speed, sizeof(float), 1, f) != 1 ||
            fread(&m->egg_timer, sizeof(int), 1, f) != 1) {
            passive_mobs_reset();
            return;
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
        int max_health = passive_mob_health_for_type(m->type);
        if (m->health > max_health) m->health = max_health;
        m->active = 1;
        m->prev_x = m->x; m->prev_y = m->y; m->prev_z = m->z;
        m->prev_yaw = m->yaw;
        m->prev_render_yaw = m->render_yaw;
        m->width = passive_mob_width_for_type(m->type);
        m->height = passive_mob_height_for_type(m->type);
        m->prev_limb_swing = m->limb_swing;
        m->prev_limb_amount = m->limb_amount;
        m->chicken_prev_wing_rot = m->chicken_wing_rot;
        m->chicken_prev_dest_pos = m->chicken_dest_pos;
        if (m->type == PASSIVE_MOB_CHICKEN && m->egg_timer <= 0) m->egg_timer = rand() % 6000 + 6000;
        m->damage_cooldown = 0;
        m->death_drops_done = m->death_time > 0;
        m->jump_cooldown = 0;
        m->stuck_ticks = 0;
        m->wander_cooldown = 20 + (rand() % 80);
    }
}
