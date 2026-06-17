/* Split from original monolithic main.c. Included by src/main.c unity build. */

static int stack_empty(const ItemStack *s) { return !s || s->id <= 0 || s->count <= 0; }

static void stack_clear(ItemStack *s) { if (s) memset(s, 0, sizeof(*s)); }

static int stack_same_item(const ItemStack *a, const ItemStack *b) { return !stack_empty(a) && !stack_empty(b) && a->id == b->id && a->damage == b->damage; }

static int stack_limit_for_id(int id) {
    if (id == ITEM_STONE_SWORD || id == ITEM_STONE_SHOVEL ||
        id == ITEM_STONE_PICKAXE || id == ITEM_STONE_AXE ||
        id == ITEM_WOODEN_SWORD || id == ITEM_WOODEN_SHOVEL ||
        id == ITEM_WOODEN_PICKAXE || id == ITEM_WOODEN_AXE) return 1;
    (void)id;
    return ITEM_MAX_STACK;
}

static ItemStack make_stack(int id, int count, int damage) { ItemStack s; s.id = id; s.count = count; s.damage = damage; s.pop_time = 0; return s; }

static int flat_index(int coord) { return coord - g_flat_world_origin_x; }
static int flat_z_index(int coord) { return coord - g_flat_world_origin_z; }

static int flat_y_index(int y) { return y - FLAT_WORLD_Y_MIN; }

static int flat_in_bounds(int x, int y, int z) {
    return y >= FLAT_WORLD_Y_MIN && y <= FLAT_WORLD_Y_MAX &&
           x >= g_flat_world_origin_x && x < g_flat_world_origin_x + FLAT_WORLD_SIZE &&
           z >= g_flat_world_origin_z && z < g_flat_world_origin_z + FLAT_WORLD_SIZE;
}

static int flat_get_block(int x, int y, int z) {
    if (!flat_in_bounds(x, y, z)) return 0;
    return g_flat_blocks[flat_y_index(y)][flat_z_index(z)][flat_index(x)];
}


static void mark_flat_render_chunks_dirty_all(void) {
    for (int cz = 0; cz < FLAT_RENDER_CHUNKS; cz++) {
        for (int cx = 0; cx < FLAT_RENDER_CHUNKS; cx++) {
            g_flat_world_chunk_dirty[cz][cx] = 1;
            g_flat_world_chunk_valid[cz][cx] = 0;
            g_flat_world_chunk_has_liquid[cz][cx] = 0;
        }
    }
    g_flat_world_geometry_dirty = 1;
}

static void mark_flat_render_chunks_dirty_around(int x, int z) {
    if (!flat_in_bounds(x, FLAT_WORLD_Y_MIN, z)) return;
    int fx = flat_index(x);
    int fz = flat_z_index(z);
    int cx0 = fx / FLAT_RENDER_CHUNK;
    int cz0 = fz / FLAT_RENDER_CHUNK;

    /* Mark the changed chunk and neighbors, because exposed faces can appear on
       adjacent chunk borders.  This keeps mining/building from rebuilding the
       whole 100x100x64 display list. */
    for (int dz = -1; dz <= 1; dz++) {
        for (int dx = -1; dx <= 1; dx++) {
            int cx = cx0 + dx;
            int cz = cz0 + dz;
            if (cx >= 0 && cx < FLAT_RENDER_CHUNKS && cz >= 0 && cz < FLAT_RENDER_CHUNKS) {
                g_flat_world_chunk_dirty[cz][cx] = 1;
            }
        }
    }
}

static void flat_set_block(int x, int y, int z, int id) {
    if (!flat_in_bounds(x, y, z)) return;
    int *cell = &g_flat_blocks[flat_y_index(y)][flat_z_index(z)][flat_index(x)];
    if (*cell != id) {
        *cell = id;
        mark_flat_render_chunks_dirty_around(x, z);
        g_save_dirty = 1;
    }
}




static int floor_div16(int v) {
    return v >= 0 ? v / 16 : -((15 - v) / 16);
}

static int beta_preview_copy_chunk_to_flat(int cx, int cz) {
    /* Reuse one chunk buffer instead of malloc/free every generated chunk.
       Streaming can generate many small chunk strips over time; avoiding heap
       churn keeps those micro jobs from becoming visible stutters. */
    static unsigned char blocks[32768];
    memset(blocks, 0, sizeof(blocks));

    TerrainProvider tp;
    terrain_provider_init(&tp, (int64_t)g_world_seed);
    jr_set_seed(&tp.rand, (int64_t)((uint64_t)(int64_t)cx * UINT64_C(341873128712) + (uint64_t)(int64_t)cz * UINT64_C(132897987541)));

    BetaBiome *biomes = biome_manager_get(&tp.biomeManager, cx * 16, cz * 16, 16, 16);
    double *temps = tp.biomeManager.temp;
    qm_generate_base(&tp, cx, cz, blocks, biomes, temps);
    qm_replace_surface(&tp, cx, cz, blocks, biomes);
    qm_generate_caves(&tp, cx, cz, blocks);
    BetaBiome centerBiome = biome_manager_get(&tp.biomeManager, cx * 16 + 16, cz * 16 + 16, 1, 1)[0];
    qm_populate_local(&tp, cx, cz, blocks, centerBiome);

    for (int lx = 0; lx < 16; lx++) {
        int wx = cx * 16 + lx;
        if (wx < g_flat_world_origin_x || wx >= g_flat_world_origin_x + FLAT_WORLD_SIZE) continue;
        int fx = flat_index(wx);
        for (int lz = 0; lz < 16; lz++) {
            int wz = cz * 16 + lz;
            if (wz < g_flat_world_origin_z || wz >= g_flat_world_origin_z + FLAT_WORLD_SIZE) continue;
            int fz = flat_z_index(wz);

            for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
                /* The current renderer is 64 high while Beta chunks are 128 high.
                   Copy the playable Beta band 32..95 down into local 0..63 so
                   sea level/surface appears in the middle instead of clipped. */
                int sy = y + 32;
                int id = get_block_local(blocks, lx, sy, lz);
                if (id == BLK_STILL_LAVA) id = BLK_LAVA;
                g_flat_blocks[flat_y_index(y)][fz][fx] = (unsigned char)id;
            }

            if (g_flat_blocks[flat_y_index(0)][fz][fx] == 0) {
                g_flat_blocks[flat_y_index(0)][fz][fx] = BLOCK_BEDROCK;
            }
        }
    }

    return 1;
}

static void beta_preview_generate_flat_world(void) {
    memset(g_flat_blocks, 0, sizeof(g_flat_blocks));
    int min_cx = floor_div16(g_flat_world_origin_x);
    int max_cx = floor_div16(g_flat_world_origin_x + FLAT_WORLD_SIZE - 1);
    int min_cz = floor_div16(g_flat_world_origin_z);
    int max_cz = floor_div16(g_flat_world_origin_z + FLAT_WORLD_SIZE - 1);

    for (int cz = min_cz; cz <= max_cz; cz++) {
        for (int cx = min_cx; cx <= max_cx; cx++) {
            beta_preview_copy_chunk_to_flat(cx, cz);
        }
    }
}


static unsigned int normal_world_hash(int x, int z, int salt) {
    unsigned int h = (unsigned int)(x * 374761393u + z * 668265263u + salt * 1442695041u + (unsigned int)g_world_seed);
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

static float normal_value_noise(int x, int z, int scale, int salt) {
    int gx = (x >= 0) ? x / scale : (x - scale + 1) / scale;
    int gz = (z >= 0) ? z / scale : (z - scale + 1) / scale;
    unsigned int h = normal_world_hash(gx, gz, salt);
    return ((float)(h & 65535) / 65535.0f) * 2.0f - 1.0f;
}

static int normal_ore_at(int x, int y, int z) {
    if (y < 5) return 0;
    unsigned int h = normal_world_hash(x, z, y * 31 + 7);
    if (y < 18 && (h % 900) < 4) return BLOCK_DIAMOND_ORE;
    if (y < 28 && (h % 420) < 5) return BLOCK_REDSTONE_ORE;
    if (y < 32 && (h % 360) < 4) return BLOCK_GOLD_ORE;
    if (y < 45 && (h % 180) < 5) return BLOCK_IRON_ORE;
    if (y < 58 && (h % 115) < 6) return BLOCK_COAL_ORE;
    return 0;
}

static void normal_place_tree(int x, int z, int ground_y) {
    int trunk_h = 4 + (int)(normal_world_hash(x, z, 99) % 3);
    if (ground_y + trunk_h + 2 > FLAT_WORLD_Y_MAX) return;
    for (int y = ground_y + 1; y <= ground_y + trunk_h; y++) {
        if (flat_get_block(x, y, z) != 0) return;
    }

    for (int y = ground_y + 1; y <= ground_y + trunk_h; y++) {
        g_flat_blocks[flat_y_index(y)][flat_z_index(z)][flat_index(x)] = BLOCK_LOG;
    }

    int top = ground_y + trunk_h;
    for (int yy = top - 2; yy <= top + 2; yy++) {
        int r = (yy >= top + 1) ? 1 : 2;
        for (int dz = -r; dz <= r; dz++) {
            for (int dx = -r; dx <= r; dx++) {
                if (abs(dx) == r && abs(dz) == r && normal_world_hash(x + dx, z + dz, yy) % 3 == 0) continue;
                int lx = x + dx, lz = z + dz;
                if (!flat_in_bounds(lx, yy, lz)) continue;
                if (flat_get_block(lx, yy, lz) == 0) {
                    g_flat_blocks[flat_y_index(yy)][flat_z_index(lz)][flat_index(lx)] = BLOCK_LEAVES;
                }
            }
        }
    }
}

static int normal_world_height_at(int x, int z) {
    /* Local b1.0-style preview: layered value noise, sea level 32, dirt/grass skin.
       Save folder still gets chunks from the existing Beta generator code. */
    float n1 = normal_value_noise(x, z, 18, 1) * 10.0f;
    float n2 = normal_value_noise(x, z, 7, 2) * 4.0f;
    float n3 = normal_value_noise(x, z, 37, 3) * 14.0f;
    int h = 32 + (int)(n1 + n2 + n3);
    if (h < 8) h = 8;
    if (h > 58) h = 58;
    return h;
}

static void flat_world_reset_blocks(void) {
    g_flat_world_origin_x = -(FLAT_WORLD_SIZE / 2);
    g_flat_world_origin_z = -(FLAT_WORLD_SIZE / 2);
    g_stream_last_center_chunk_x = 999999;
    g_stream_last_center_chunk_z = 999999;
    g_stream_gen_queue_count = 0;
    g_stream_gen_queue_index = 0;
    memset(g_flat_blocks, 0, sizeof(g_flat_blocks));

    if (g_world_type == 1) {
        /* Use the existing Beta 1.0 deobf/obf generator path for the local view:
           same base terrain, surface pass, caves, ore placement and structures. */
        beta_preview_generate_flat_world();
    } else {
        for (int z = 0; z < FLAT_WORLD_SIZE; z++) {
            for (int x = 0; x < FLAT_WORLD_SIZE; x++) {
                /* Flat: 100x100x64 with 4 solid layers + 60 air layers. */
                g_flat_blocks[flat_y_index(0)][z][x] = BLOCK_BEDROCK;
                g_flat_blocks[flat_y_index(1)][z][x] = BLOCK_DIRT;
                g_flat_blocks[flat_y_index(2)][z][x] = BLOCK_DIRT;
                g_flat_blocks[flat_y_index(3)][z][x] = BLOCK_GRASS;
            }
        }
    }

    if (g_world_type == 1) {
        /* Keep the old local-tree fallback disabled for Beta preview.  Trees now
           come from qm_populate_local/place_beta_tree above. */
    }

    memset(g_drops, 0, sizeof(g_drops));
    memset(g_dig_particles, 0, sizeof(g_dig_particles));
    g_next_dig_particle = 0;
    mark_flat_render_chunks_dirty_all();
}


static int block_is_liquid(int id) {
    return id == BLOCK_WATER || id == BLOCK_STILL_WATER ||
           id == BLOCK_LAVA || id == BLOCK_STILL_LAVA;
}

static int block_is_water(int id) {
    return id == BLOCK_WATER || id == BLOCK_STILL_WATER;
}

static int block_is_lava(int id) {
    return id == BLOCK_LAVA || id == BLOCK_STILL_LAVA;
}

static int block_is_source_liquid(int id) {
    return id == BLOCK_STILL_WATER || id == BLOCK_STILL_LAVA;
}

static int block_is_flowing_liquid(int id) {
    return id == BLOCK_WATER || id == BLOCK_LAVA;
}

static int flat_block_is_solid_for_collision(int id) {
    return id != 0 && !block_is_liquid(id) &&
           id != BLOCK_LEAVES && id != BLOCK_GLASS &&
           id != BLOCK_SAPLING && id != BLOCK_YELLOW_FLOWER && id != BLOCK_RED_ROSE &&
           id != BLOCK_BROWN_MUSHROOM && id != BLOCK_RED_MUSHROOM &&
           id != BLOCK_TORCH && id != BLOCK_SNOW_LAYER;
}

static int flat_solid_for_spawn(int id) {
    return flat_block_is_solid_for_collision(id);
}

static int block_drop_id(int block_id) {
    /* ph.java grass drops dirt via of.v.a(0, random). */
    if (block_id == BLOCK_GRASS) return BLOCK_DIRT;
    if (block_id == BLOCK_LEAVES) return 0;
    if (block_is_liquid(block_id)) return 0;
    if (block_id == BLOCK_STONE) return BLOCK_COBBLESTONE;
    if (block_id == BLOCK_BEDROCK) return 0;
    return block_id;
}

static float block_hardness(int block_id) {
    if (block_id == BLOCK_BEDROCK) return -1.0f;
    if (block_is_liquid(block_id)) return -1.0f;
    if (block_id == BLOCK_STONE) return 1.5f;
    if (block_id == BLOCK_COBBLESTONE) return 2.0f;
    if (block_id == BLOCK_GRASS) return 0.6f;
    if (block_id == BLOCK_DIRT) return 0.5f;
    if (block_id == BLOCK_SAND) return 0.5f;
    if (block_id == BLOCK_GRAVEL) return 0.6f;
    if (block_id == BLOCK_LEAVES) return 0.2f;
    if (block_id == BLOCK_PLANKS) return 2.0f;
    if (block_id == BLOCK_LOG) return 2.0f;
    if (block_id == BLOCK_CRAFTING_TABLE) return 2.5f;
    if (block_id == BLOCK_CHEST) return 2.5f;
    if (block_id == BLOCK_FURNACE) return 3.5f;
    if (block_id == BLOCK_COAL_ORE || block_id == BLOCK_IRON_ORE || block_id == BLOCK_GOLD_ORE ||
        block_id == BLOCK_DIAMOND_ORE || block_id == BLOCK_REDSTONE_ORE || block_id == BLOCK_REDSTONE_ORE_GLOWING ||
        block_id == BLOCK_LAPIS_ORE) return 3.0f;
    if (block_id == BLOCK_OBSIDIAN) return 10.0f;
    return 1.0f;
}

static int block_requires_pickaxe_to_harvest(int block_id) {
    return block_id == BLOCK_STONE || block_id == BLOCK_COBBLESTONE ||
           block_id == BLOCK_COAL_ORE || block_id == BLOCK_IRON_ORE ||
           block_id == BLOCK_GOLD_ORE || block_id == BLOCK_DIAMOND_ORE ||
           block_id == BLOCK_REDSTONE_ORE || block_id == BLOCK_REDSTONE_ORE_GLOWING ||
           block_id == BLOCK_LAPIS_ORE || block_id == BLOCK_OBSIDIAN;
}

static int block_is_wood_like_for_axe(int block_id) {
    return block_id == BLOCK_LOG || block_id == BLOCK_PLANKS ||
           block_id == BLOCK_CRAFTING_TABLE || block_id == BLOCK_CHEST;
}

static float block_relative_strength(int block_id) {
    float h = block_hardness(block_id);
    if (h < 0.0f) return 0.0f;

    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    int held_id = stack_empty(held) ? 0 : held->id;

    /* Minecraft-style timing:
       - harvestable/current-tool blocks: hardness * 1.5 seconds
       - unharvestable blocks such as stone without a pickaxe: hardness * 5 seconds
       Tool speed value is then applied as damage per tick. */
    float divisor = block_requires_pickaxe_to_harvest(block_id) ? 100.0f : 30.0f;
    float tool_speed = 1.0f;

    if (held_id == ITEM_STONE_PICKAXE &&
        (block_id == BLOCK_STONE || block_id == BLOCK_COBBLESTONE ||
         block_id == BLOCK_COAL_ORE || block_id == BLOCK_IRON_ORE || block_id == BLOCK_GOLD_ORE ||
         block_id == BLOCK_DIAMOND_ORE || block_id == BLOCK_REDSTONE_ORE || block_id == BLOCK_REDSTONE_ORE_GLOWING ||
         block_id == BLOCK_LAPIS_ORE || block_id == BLOCK_OBSIDIAN)) {
        divisor = 30.0f;
        tool_speed *= 4.0f; /* stone-tier tool speed */
    }

    if (held_id == ITEM_STONE_AXE && block_is_wood_like_for_axe(block_id)) {
        tool_speed *= 4.0f; /* stone axe: logs/planks 0.75s, table ~0.95s */
    }

    /* Source-style shovel speed. Dirt felt slow because it was only 1.6x. */
    if ((block_id == BLOCK_DIRT || block_id == BLOCK_GRASS || block_id == BLOCK_SAND || block_id == BLOCK_GRAVEL) &&
        (held_id == ITEM_STONE_SHOVEL || held_id == ITEM_WOODEN_SHOVEL)) {
        tool_speed *= (held_id == ITEM_STONE_SHOVEL) ? 4.0f : 2.0f;
    }

    return tool_speed / (h * divisor);
}

static void trigger_hand_swing(void) {
    /* EntityLiving/ItemRenderer-style swing: start one 8 tick arm swing.
       Holding mine re-triggers it; using/placing gets one clean swing instead of
       the old one-frame snap. */
    if (!g_hand_swing_active && g_hand_swing_ticks == 0) {
        g_hand_swing_active = 1;
        g_hand_swing_ticks = 0;
        g_hand_swing_progress = 0.0f;
        g_hand_swing = 0.0f;
    }
}

static int flat_player_aabb_collides(float px, float py, float pz) {
    float feet = py - 1.62f;
    float minx = px - 0.30f, maxx = px + 0.30f;
    float miny = feet + 0.001f, maxy = feet + 1.80f - 0.001f;
    float minz = pz - 0.30f, maxz = pz + 0.30f;
    int x0 = (int)floorf(minx), x1 = (int)floorf(maxx - 0.001f);
    int y0 = (int)floorf(miny), y1 = (int)floorf(maxy - 0.001f);
    int z0 = (int)floorf(minz), z1 = (int)floorf(maxz - 0.001f);
    for (int y = y0; y <= y1; y++) {
        for (int z = z0; z <= z1; z++) {
            for (int x = x0; x <= x1; x++) {
                if (flat_block_is_solid_for_collision(flat_get_block(x, y, z))) return 1;
            }
        }
    }
    return 0;
}

static int flat_player_has_sneak_support(float px, float py, float pz) {
    /* Source-style sneaking edge check from B1.0.0 mm.java:249-273.
       The source reduces X/Z motion in 0.05 steps while the player's bounding
       box offset downward by 1.0 has no collision. This checks for ANY floor
       under the current player footprint, so the invisible wall is at the real
       block edge, not too early. */
    float feet = py - 1.62f;
    int floor_y = (int)floorf(feet - 1.0f + 0.001f);

    float minx = px - 0.30f, maxx = px + 0.30f;
    float minz = pz - 0.30f, maxz = pz + 0.30f;
    int x0 = (int)floorf(minx + 0.001f);
    int x1 = (int)floorf(maxx - 0.001f);
    int z0 = (int)floorf(minz + 0.001f);
    int z1 = (int)floorf(maxz - 0.001f);

    for (int z = z0; z <= z1; z++) {
        for (int x = x0; x <= x1; x++) {
            if (flat_get_block(x, floor_y, z) != 0) return 1;
        }
    }
    return 0;
}

static int flat_block_intersects_player(int bx, int by, int bz) {
    float feet = g_player_y - 1.62f;
    float minx = g_player_x - 0.30f, maxx = g_player_x + 0.30f;
    float miny = feet + 0.001f, maxy = feet + 1.80f - 0.001f;
    float minz = g_player_z - 0.30f, maxz = g_player_z + 0.30f;
    return maxx > (float)bx && minx < (float)(bx + 1) &&
           maxy > (float)by && miny < (float)(by + 1) &&
           maxz > (float)bz && minz < (float)(bz + 1);
}

static int flat_item_aabb_collides(float px, float py, float pz) {
    float minx = px - 0.125f, maxx = px + 0.125f;
    float miny = py - 0.125f, maxy = py + 0.125f;
    float minz = pz - 0.125f, maxz = pz + 0.125f;
    int x0 = (int)floorf(minx), x1 = (int)floorf(maxx - 0.001f);
    int y0 = (int)floorf(miny), y1 = (int)floorf(maxy - 0.001f);
    int z0 = (int)floorf(minz), z1 = (int)floorf(maxz - 0.001f);
    for (int y = y0; y <= y1; y++) {
        for (int z = z0; z <= z1; z++) {
            for (int x = x0; x <= x1; x++) {
                if (flat_block_is_solid_for_collision(flat_get_block(x, y, z))) return 1;
            }
        }
    }
    return 0;
}

static void inventory_reset(void) {
    memset(g_inventory, 0, sizeof(g_inventory));
    memset(g_craft_grid, 0, sizeof(g_craft_grid));
    memset(g_chest_slots, 0, sizeof(g_chest_slots));
    stack_clear(&g_carried_stack);

    /* Start hotbar stone tool set. Only the stone shovel gets the dirt/grass
       mining boost; sword/pickaxe/axe are added as usable visible items only. */
    g_inventory[0] = make_stack(ITEM_STONE_SWORD, 1, 0);
    g_inventory[1] = make_stack(ITEM_STONE_PICKAXE, 1, 0);
    g_inventory[2] = make_stack(ITEM_STONE_AXE, 1, 0);
    g_inventory[3] = make_stack(ITEM_STONE_SHOVEL, 1, 0);
    g_inventory[4] = make_stack(BLOCK_GRASS, 64, 0);
    g_inventory[5] = make_stack(BLOCK_LOG, 64, 0);

    /* Keep the earlier requested reserve of 10 stone shovels in the main inventory. */
    for (int i = 0; i < 10; i++) {
        g_inventory[9 + i] = make_stack(ITEM_STONE_SHOVEL, 1, 0);
    }

    /* One starter stack of stone in main inventory, not hotbar. Stone drops cobblestone. */
    g_inventory[19] = make_stack(BLOCK_STONE, 64, 0);
    g_inventory[20] = make_stack(BLOCK_FURNACE, 1, 0); /* test furnace for prepared furnace UI */
    g_inventory[21] = make_stack(BLOCK_SAND, 64, 0);
    g_inventory[22] = make_stack(BLOCK_GRAVEL, 64, 0);
    g_inventory[23] = make_stack(BLOCK_CHEST, 1, 0);
}

static int inventory_add_stack(ItemStack st) {
    if (stack_empty(&st)) return 1;
    for (int i = 0; i < 36 && st.count > 0; i++) {
        if (stack_same_item(&g_inventory[i], &st) && g_inventory[i].count < stack_limit_for_id(st.id)) {
            int room = stack_limit_for_id(st.id) - g_inventory[i].count;
            int move = st.count < room ? st.count : room;
            g_inventory[i].count += move;
            g_inventory[i].pop_time = 5;
            st.count -= move;
        }
    }
    for (int i = 0; i < 36 && st.count > 0; i++) {
        if (stack_empty(&g_inventory[i])) {
            int move = st.count < stack_limit_for_id(st.id) ? st.count : stack_limit_for_id(st.id);
            g_inventory[i] = make_stack(st.id, move, st.damage);
            g_inventory[i].pop_time = 5;
            st.count -= move;
        }
    }
    return st.count <= 0;
}

static void inventory_tick(void) {
    for (int i = 0; i < 36; i++) if (g_inventory[i].pop_time > 0) g_inventory[i].pop_time--;
}

static void player_look_vector(float *dx, float *dy, float *dz) {
    float yaw = g_player_yaw * (float)M_PI / 180.0f;
    float pitch = g_player_pitch * (float)M_PI / 180.0f;
    float cp = cosf(pitch);
    *dx = -sinf(yaw) * cp;
    *dy = -sinf(pitch);
    *dz =  cosf(yaw) * cp;
}

static void spawn_item_stack(float x, float y, float z, ItemStack st, int random_spread) {
    if (stack_empty(&st)) return;
    int slot = -1;
    for (int i = 0; i < MAX_DROP_ENTITIES; i++) if (!g_drops[i].active) { slot = i; break; }
    if (slot < 0) slot = 0;
    FlatDroppedItem *e = &g_drops[slot];
    memset(e, 0, sizeof(*e));
    e->active = 1;
    e->stack = st;
    e->x = e->prev_x = x;
    e->y = e->prev_y = y;
    e->z = e->prev_z = z;
    e->rot = (float)(rand() % 6283) / 1000.0f;
    e->pickup_delay = 10;
    g_save_dirty = 1;
    if (random_spread) {
        e->mx = ((float)rand() / (float)RAND_MAX) * 0.2f - 0.1f;
        e->my = 0.2f;
        e->mz = ((float)rand() / (float)RAND_MAX) * 0.2f - 0.1f;
    } else {
        float lx, ly, lz;
        player_look_vector(&lx, &ly, &lz);
        e->mx = lx * 0.3f;
        e->my = 0.1f + ly * 0.1f;
        e->mz = lz * 0.3f;
    }
}

static void inventory_drop_selected_one(void) {
    ItemStack *s = &g_inventory[g_selected_hotbar_slot];
    if (stack_empty(s)) return;
    ItemStack one = make_stack(s->id, 1, s->damage);
    s->count--;
    if (s->count <= 0) stack_clear(s);
    spawn_item_stack(g_player_x, g_player_y - 0.30f, g_player_z, one, 0);
    g_save_dirty = 1;
    trigger_hand_swing();
    hud_add_chat("Dropped item.");
}



static int craft_id_at(const ItemStack *grid, int w, int x, int y) {
    const ItemStack *st = &grid[x + y * w];
    return stack_empty(st) ? 0 : st->id;
}

static int craft_count_nonempty(const ItemStack *grid, int n) {
    int c = 0;
    for (int i = 0; i < n; i++) if (!stack_empty(&grid[i])) c++;
    return c;
}

static ItemStack workbench_crafting_output(void) {
    ItemStack *g = g_workbench_grid;
    int nonempty = craft_count_nonempty(g, 9);
    if (nonempty == 0) return make_stack(0, 0, 0);

    /* Chest: planks around the edge, empty middle. */
    if (nonempty == 8 &&
        craft_id_at(g,3,0,0)==BLOCK_PLANKS && craft_id_at(g,3,1,0)==BLOCK_PLANKS && craft_id_at(g,3,2,0)==BLOCK_PLANKS &&
        craft_id_at(g,3,0,1)==BLOCK_PLANKS && craft_id_at(g,3,1,1)==0            && craft_id_at(g,3,2,1)==BLOCK_PLANKS &&
        craft_id_at(g,3,0,2)==BLOCK_PLANKS && craft_id_at(g,3,1,2)==BLOCK_PLANKS && craft_id_at(g,3,2,2)==BLOCK_PLANKS) {
        return make_stack(BLOCK_CHEST, 1, 0);
    }

    /* Furnace: cobblestone around the edge, empty middle. */
    if (nonempty == 8 &&
        craft_id_at(g,3,0,0)==BLOCK_COBBLESTONE && craft_id_at(g,3,1,0)==BLOCK_COBBLESTONE && craft_id_at(g,3,2,0)==BLOCK_COBBLESTONE &&
        craft_id_at(g,3,0,1)==BLOCK_COBBLESTONE && craft_id_at(g,3,1,1)==0                && craft_id_at(g,3,2,1)==BLOCK_COBBLESTONE &&
        craft_id_at(g,3,0,2)==BLOCK_COBBLESTONE && craft_id_at(g,3,1,2)==BLOCK_COBBLESTONE && craft_id_at(g,3,2,2)==BLOCK_COBBLESTONE) {
        return make_stack(BLOCK_FURNACE, 1, 0);
    }

    /* Sticks: two vertical planks in any column -> 4 sticks. */
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 2; y++) {
            if (craft_id_at(g,3,x,y)==BLOCK_PLANKS && craft_id_at(g,3,x,y+1)==BLOCK_PLANKS && nonempty == 2) {
                return make_stack(ITEM_STICK, 4, 0);
            }
        }
    }

    /* Tools.  Wooden uses planks, stone uses cobblestone, and all use sticks. */
    int head_ids[2] = { BLOCK_PLANKS, BLOCK_COBBLESTONE };
    int sword_ids[2] = { ITEM_WOODEN_SWORD, ITEM_STONE_SWORD };
    int shovel_ids[2] = { ITEM_WOODEN_SHOVEL, ITEM_STONE_SHOVEL };
    int pick_ids[2] = { ITEM_WOODEN_PICKAXE, ITEM_STONE_PICKAXE };
    int axe_ids[2] = { ITEM_WOODEN_AXE, ITEM_STONE_AXE };

    for (int t = 0; t < 2; t++) {
        int h = head_ids[t];

        /* Pickaxe: HHH / .S. / .S. */
        if (nonempty == 5 &&
            craft_id_at(g,3,0,0)==h && craft_id_at(g,3,1,0)==h && craft_id_at(g,3,2,0)==h &&
            craft_id_at(g,3,1,1)==ITEM_STICK && craft_id_at(g,3,1,2)==ITEM_STICK) {
            return make_stack(pick_ids[t], 1, 0);
        }

        /* Axe: HH. / HS. / .S. or .HH / .SH / .S. */
        if (nonempty == 5 &&
            craft_id_at(g,3,1,1)==ITEM_STICK && craft_id_at(g,3,1,2)==ITEM_STICK) {
            if (craft_id_at(g,3,0,0)==h && craft_id_at(g,3,1,0)==h && craft_id_at(g,3,0,1)==h)
                return make_stack(axe_ids[t], 1, 0);
            if (craft_id_at(g,3,1,0)==h && craft_id_at(g,3,2,0)==h && craft_id_at(g,3,2,1)==h)
                return make_stack(axe_ids[t], 1, 0);
        }

        /* Shovel: .H. / .S. / .S. */
        if (nonempty == 3 &&
            craft_id_at(g,3,1,0)==h && craft_id_at(g,3,1,1)==ITEM_STICK && craft_id_at(g,3,1,2)==ITEM_STICK) {
            return make_stack(shovel_ids[t], 1, 0);
        }

        /* Sword: .H. / .H. / .S. */
        if (nonempty == 3 &&
            craft_id_at(g,3,1,0)==h && craft_id_at(g,3,1,1)==h && craft_id_at(g,3,1,2)==ITEM_STICK) {
            return make_stack(sword_ids[t], 1, 0);
        }
    }

    return make_stack(0, 0, 0);
}


static ItemStack inventory_crafting_output(void) {
    if (g_screen == SCREEN_WORKBENCH) return workbench_crafting_output();
    /* Recipes for now:
       1 log anywhere in the 2x2 crafting grid -> 4 planks.
       4 planks filling the 2x2 crafting grid -> 1 crafting table. */
    int nonempty = 0;
    int logs = 0;
    int planks = 0;

    for (int i = 0; i < 4; i++) {
        if (stack_empty(&g_craft_grid[i])) continue;
        nonempty++;
        if (g_craft_grid[i].id == BLOCK_LOG) logs++;
        else if (g_craft_grid[i].id == BLOCK_PLANKS) planks++;
        else return make_stack(0, 0, 0);
    }

    if (nonempty == 1 && logs == 1) return make_stack(BLOCK_PLANKS, 4, 0);
    if (nonempty == 4 && planks == 4) return make_stack(BLOCK_CRAFTING_TABLE, 1, 0);
    return make_stack(0, 0, 0);
}

static int inventory_take_crafting_output(void) {
    ItemStack out = inventory_crafting_output();
    if (stack_empty(&out)) return 0;

    if (stack_empty(&g_carried_stack)) {
        g_carried_stack = out;
    } else if (stack_same_item(&g_carried_stack, &out) &&
               g_carried_stack.count + out.count <= stack_limit_for_id(out.id)) {
        g_carried_stack.count += out.count;
    } else {
        return 0;
    }

    if (g_screen == SCREEN_WORKBENCH) {
        for (int i = 0; i < 9; i++) {
            if (!stack_empty(&g_workbench_grid[i])) {
                g_workbench_grid[i].count--;
                if (g_workbench_grid[i].count <= 0) stack_clear(&g_workbench_grid[i]);
            }
        }
    } else {
        for (int i = 0; i < 4; i++) {
            if (!stack_empty(&g_craft_grid[i])) {
                g_craft_grid[i].count--;
                if (g_craft_grid[i].count <= 0) stack_clear(&g_craft_grid[i]);
            }
        }
    }
    g_save_dirty = 1;
    return 1;
}

static ItemStack *inventory_slot_ptr(int slot) {
    if (slot >= 0 && slot < 36) return &g_inventory[slot];
    if (slot >= 100 && slot < 104) return &g_craft_grid[slot - 100];
    if (slot >= 300 && slot < 309) return &g_workbench_grid[slot - 300];
    if (slot >= 200 && slot < 227) return &g_chest_slots[slot - 200];
    return NULL;
}

static int inventory_slot_at(int mx, int my) {
    int inv_x = (g_gui_w - 176) / 2;
    int inv_y = (g_gui_h - 166) / 2;
    int rx = mx - inv_x;
    int ry = my - inv_y;

    if (g_screen == SCREEN_WORKBENCH) {
        for (int row = 0; row < 3; row++) for (int col = 0; col < 3; col++) {
            int sx = 30 + col * 18;
            int sy = 17 + row * 18;
            if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return 300 + col + row * 3;
        }
        if (rx >= 124 && rx < 142 && ry >= 35 && ry < 53) return 309;
    }

    if (g_screen == SCREEN_CHEST) {
        for (int row = 0; row < 3; row++) for (int col = 0; col < 9; col++) {
            int sx = 8 + col * 18;
            int sy = 18 + row * 18;
            if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return 200 + col + row * 9;
        }
        for (int row = 0; row < 3; row++) for (int col = 0; col < 9; col++) {
            int sx = 8 + col * 18;
            int sy = 84 + row * 18;
            if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return 9 + col + row * 9;
        }
        for (int col = 0; col < 9; col++) {
            int sx = 8 + col * 18;
            int sy = 142;
            if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return col;
        }
        return -1;
    }

    /* 2x2 crafting grid, matching gui_inventory.mcrw layout. */
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 2; col++) {
            int sx = 88 + col * 18;
            int sy = 26 + row * 18;
            if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return 100 + col + row * 2;
        }
    }

    /* Crafting output slot. */
    if (rx >= 144 - 1 && rx < 144 + 17 && ry >= 36 - 1 && ry < 36 + 17) return 104;

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 9; col++) {
            int sx = 8 + col * 18;
            int sy = 84 + row * 18;
            if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return 9 + col + row * 9;
        }
    }
    for (int col = 0; col < 9; col++) {
        int sx = 8 + col * 18;
        int sy = 142;
        if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return col;
    }
    return -1;
}

static void inventory_mouse_click(int mx, int my, int button) {
    int inv_x = (g_gui_w - 176) / 2;
    int inv_y = (g_gui_h - 166) / 2;
    int outside = (mx < inv_x || my < inv_y || mx >= inv_x + 176 || my >= inv_y + 166);
    int slot = inventory_slot_at(mx, my);
    if (outside && !stack_empty(&g_carried_stack)) {
        if (button == 0) { spawn_item_stack(g_player_x, g_player_y - 0.30f, g_player_z, g_carried_stack, 0); stack_clear(&g_carried_stack); }
        else { ItemStack one = make_stack(g_carried_stack.id, 1, g_carried_stack.damage); spawn_item_stack(g_player_x, g_player_y - 0.30f, g_player_z, one, 0); if (--g_carried_stack.count <= 0) stack_clear(&g_carried_stack); }
        return;
    }
    if (slot < 0) return;

    if (slot == 104 || slot == 309) {
        inventory_take_crafting_output();
        return;
    }

    ItemStack *s = inventory_slot_ptr(slot);
    if (!s) return;

    if (button == 0) {
        if (stack_empty(&g_carried_stack)) {
            if (!stack_empty(s)) { g_carried_stack = *s; stack_clear(s); }
        } else if (stack_empty(s)) {
            *s = g_carried_stack; stack_clear(&g_carried_stack);
        } else if (stack_same_item(s, &g_carried_stack) && s->count < stack_limit_for_id(s->id)) {
            int room = stack_limit_for_id(s->id) - s->count;
            int move = g_carried_stack.count < room ? g_carried_stack.count : room;
            s->count += move;
            g_carried_stack.count -= move;
            s->pop_time = 5;
            if (g_carried_stack.count <= 0) stack_clear(&g_carried_stack);
        } else {
            ItemStack tmp = *s; *s = g_carried_stack; g_carried_stack = tmp;
        }
    } else {
        if (stack_empty(&g_carried_stack)) {
            if (!stack_empty(s)) {
                int take = (s->count + 1) / 2;
                g_carried_stack = make_stack(s->id, take, s->damage);
                s->count -= take;
                if (s->count <= 0) stack_clear(s);
            }
        } else if (stack_empty(s)) {
            *s = make_stack(g_carried_stack.id, 1, g_carried_stack.damage);
            if (--g_carried_stack.count <= 0) stack_clear(&g_carried_stack);
        } else if (stack_same_item(s, &g_carried_stack) && s->count < stack_limit_for_id(s->id)) {
            s->count++;
            s->pop_time = 5;
            if (--g_carried_stack.count <= 0) stack_clear(&g_carried_stack);
        }
    }
}

static FlatRayHit flat_raycast(void) {
    FlatRayHit h; memset(&h, 0, sizeof(h)); h.face = 1;
    float dx, dy, dz;
    player_look_vector(&dx, &dy, &dz);
    float px = g_player_x;
    float py = g_player_y;
    float pz = g_player_z;
    int prev_bx = (int)floorf(px), prev_by = (int)floorf(py), prev_bz = (int)floorf(pz);
    for (float t = 0.0f; t <= 5.0f; t += 0.05f) {
        float x = px + dx * t;
        float y = py + dy * t;
        float z = pz + dz * t;
        int bx = (int)floorf(x);
        int by = (int)floorf(y);
        int bz = (int)floorf(z);
        if (flat_get_block(bx, by, bz) != 0) {
            h.hit = 1; h.bx = bx; h.by = by; h.bz = bz; h.hx = x; h.hy = y; h.hz = z;
            if (prev_by < by) h.face = 0;
            else if (prev_by > by) h.face = 1;
            else if (prev_bz < bz) h.face = 2;
            else if (prev_bz > bz) h.face = 3;
            else if (prev_bx < bx) h.face = 4;
            else if (prev_bx > bx) h.face = 5;
            return h;
        }
        prev_bx = bx; prev_by = by; prev_bz = bz;
    }
    return h;
}

static void start_air_swing_once(void) {
    if (g_air_swing_consumed || g_air_swing_playing) return;
    g_air_swing_consumed = 1;
    g_air_swing_playing = 1;
    g_air_swing_ticks = 0;
    g_prev_hand_swing = 0.0f;
    g_hand_swing = 0.0f;
    g_hand_swing_ticks = 0;
}

static void reset_breaking_state(void) {
    g_breaking_block = 0;
    g_break_damage = 0.0f;
    g_prev_break_damage = 0.0f;
    g_break_sound_counter = 0.0f;
}

static int held_item_can_harvest_drop_for_block(int block_id) {
    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    int held_id = stack_empty(held) ? 0 : held->id;
    if (block_id == BLOCK_STONE || block_id == BLOCK_COBBLESTONE) {
        return held_id == ITEM_STONE_PICKAXE;
    }
    return 1;
}

static void break_target_block(void) {
    int id = flat_get_block(g_break_x, g_break_y, g_break_z);
    if (id == 0 || id == BLOCK_BEDROCK) return;

    /* Source EffectRenderer.func_1186_a: spawn destroy particles while the
       block still exists, then remove it. */
    spawn_block_destroy_particles(g_break_x, g_break_y, g_break_z, id);

    flat_set_block(g_break_x, g_break_y, g_break_z, 0);
    int drop = block_drop_id(id);
    if (drop > 0 && held_item_can_harvest_drop_for_block(id)) {
        spawn_item_stack((float)g_break_x + 0.5f, (float)g_break_y + 0.7f, (float)g_break_z + 0.5f, make_stack(drop, 1, 0), 1);
    }
}

static void update_breaking(void) {
    g_prev_break_damage = g_break_damage;
    if (g_block_hit_delay > 0) g_block_hit_delay--;
    if (!key_down_vk(VK_LBUTTON)) { reset_breaking_state(); return; }
    FlatRayHit hit = flat_raycast();
    if (!hit.hit) { reset_breaking_state(); return; }
    trigger_hand_swing();
    if (!g_breaking_block || hit.bx != g_break_x || hit.by != g_break_y || hit.bz != g_break_z) {
        g_breaking_block = 1;
        g_break_x = hit.bx; g_break_y = hit.by; g_break_z = hit.bz; g_break_face = hit.face;
        g_break_damage = g_prev_break_damage = 0.0f;
        g_break_sound_counter = 0.0f;
    }
    if (g_block_hit_delay > 0) return;
    int id = flat_get_block(g_break_x, g_break_y, g_break_z);
    if (id == 0 || id == BLOCK_BEDROCK || block_is_liquid(id)) { reset_breaking_state(); return; }
    g_break_swing_holding = 1; /* while-mining loop */
    if (g_ingame_ticks != g_last_hit_particle_tick || g_break_x != g_last_hit_particle_x || g_break_y != g_last_hit_particle_y || g_break_z != g_last_hit_particle_z || g_break_face != g_last_hit_particle_face) {
        spawn_block_hit_particle(g_break_x, g_break_y, g_break_z, g_break_face, id);
        g_last_hit_particle_x = g_break_x; g_last_hit_particle_y = g_break_y; g_last_hit_particle_z = g_break_z;
        g_last_hit_particle_face = g_break_face; g_last_hit_particle_tick = g_ingame_ticks;
    }
    g_break_damage += block_relative_strength(id);
    g_break_sound_counter += 1.0f;
    if (g_break_damage >= 1.0f) {
        break_target_block();
        reset_breaking_state();
        /* No repeated block-hit pause while holding; the first click/start is the only delay feel. */
        g_block_hit_delay = 0;
    }
}

static void world_left_mouse_released(void) { reset_breaking_state(); g_block_hit_delay = 0; g_break_swing_consumed = 0; g_break_swing_holding = 0; }

static int item_is_placeable_block_id(int id) {
    if (id <= 0 || id == BLOCK_BEDROCK) return 0;
    if (block_is_liquid(id)) return 0; /* water/lava need bucket logic, not block mining/placing */
    if (id >= 1 && id <= 89) return 1;
    return 0;
}

static void ingame_right_click(void) {
    if (g_screen != SCREEN_INGAME) return;

    FlatRayHit hit = flat_raycast();
    if (!hit.hit) return;

    int target_id = flat_get_block(hit.bx, hit.by, hit.bz);
    if (target_id == BLOCK_CRAFTING_TABLE) {
        set_screen(SCREEN_WORKBENCH);
        return;
    }
    if (target_id == BLOCK_FURNACE) {
        set_screen(SCREEN_FURNACE);
        return;
    }
    if (target_id == BLOCK_CHEST) {
        set_screen(SCREEN_CHEST);
        return;
    }

    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    if (stack_empty(held)) return;
    if (!item_is_placeable_block_id(held->id)) return;

    int px = hit.bx;
    int py = hit.by;
    int pz = hit.bz;

    if (hit.face == 0) py--;
    else if (hit.face == 1) py++;
    else if (hit.face == 2) pz--;
    else if (hit.face == 3) pz++;
    else if (hit.face == 4) px--;
    else if (hit.face == 5) px++;

    if (!flat_in_bounds(px, py, pz)) return;
    if (flat_get_block(px, py, pz) != 0) return;
    if (flat_block_intersects_player(px, py, pz)) return;

    flat_set_block(px, py, pz, held->id);
    if (--held->count <= 0) stack_clear(held);
    g_save_dirty = 1;
    trigger_hand_swing();
}


static int dropped_stack_can_merge(const FlatDroppedItem *a, const FlatDroppedItem *b) {
    if (!a || !b || !a->active || !b->active) return 0;
    if (!stack_same_item(&a->stack, &b->stack)) return 0;
    if (a->stack.count >= stack_limit_for_id(a->stack.id)) return 0;
    return 1;
}

static void merge_nearby_dropped_items(int idx) {
    FlatDroppedItem *a = &g_drops[idx];
    if (!a->active) return;
    for (int j = 0; j < MAX_DROP_ENTITIES; j++) {
        if (j == idx) continue;
        FlatDroppedItem *b = &g_drops[j];
        if (!dropped_stack_can_merge(a, b)) continue;

        float dx = a->x - b->x;
        float dy = a->y - b->y;
        float dz = a->z - b->z;
        if (dx*dx + dy*dy + dz*dz > 0.50f * 0.50f) continue;

        int room = stack_limit_for_id(a->stack.id) - a->stack.count;
        int move = b->stack.count < room ? b->stack.count : room;
        if (move <= 0) continue;

        a->stack.count += move;
        b->stack.count -= move;
        a->pickup_delay = (a->pickup_delay > b->pickup_delay) ? a->pickup_delay : b->pickup_delay;
        a->age = (a->age < b->age) ? a->age : b->age;

        if (b->stack.count <= 0) b->active = 0;
        g_save_dirty = 1;
        if (a->stack.count >= stack_limit_for_id(a->stack.id)) return;
    }
}

static void inventory_drop_all_items_on_death(void) {
    for (int i = 0; i < 36; i++) {
        if (!stack_empty(&g_inventory[i])) {
            spawn_item_stack(g_player_x, g_player_y - 0.30f, g_player_z, g_inventory[i], 1);
            stack_clear(&g_inventory[i]);
        }
    }
    if (!stack_empty(&g_carried_stack)) {
        spawn_item_stack(g_player_x, g_player_y - 0.30f, g_player_z, g_carried_stack, 1);
        stack_clear(&g_carried_stack);
    }
    memset(g_craft_grid, 0, sizeof(g_craft_grid));
    g_save_dirty = 1;
}



static int dropped_item_touches_player(const FlatDroppedItem *e) {
    float dx = e->x - g_player_x;
    float dy = e->y - (g_player_y - 0.90f);
    float dz = e->z - g_player_z;
    return (dx * dx + dy * dy + dz * dz) <= (1.25f * 1.25f);
}

static void update_dropped_items(void) {
    for (int i = 0; i < MAX_DROP_ENTITIES; i++) {
        FlatDroppedItem *e = &g_drops[i];
        if (!e->active) continue;
        e->prev_x = e->x; e->prev_y = e->y; e->prev_z = e->z;
        if (e->pickup_delay > 0) e->pickup_delay--;
        /* Deobf EntityItem has no magnetic pull; pickup happens on player collision. */
        e->my -= 0.04f;

        e->x += e->mx;
        if (flat_item_aabb_collides(e->x, e->y, e->z)) {
            e->x = e->prev_x;
            e->mx *= -0.5f;
        }

        e->z += e->mz;
        if (flat_item_aabb_collides(e->x, e->y, e->z)) {
            e->z = e->prev_z;
            e->mz *= -0.5f;
        }

        e->y += e->my;
        int on_ground = 0;
        if (flat_item_aabb_collides(e->x, e->y, e->z)) {
            e->y = e->prev_y;
            if (e->my < 0.0f) on_ground = 1;
            e->my *= -0.5f;
        }

        float friction = on_ground ? 0.58800006f : 0.98f;
        e->mx *= friction;
        e->my *= 0.98f;
        e->mz *= friction;
        e->age++;
        if (e->age >= 6000 || e->y < -24.0f) { e->active = 0; continue; }

        if (!g_player_dead && g_screen == SCREEN_INGAME && e->pickup_delay <= 0 && dropped_item_touches_player(e)) {
            if (inventory_add_stack(e->stack)) { e->active = 0; g_save_dirty = 1; }
        }
        if (e->active) merge_nearby_dropped_items(i);
    }
}


static int flat_column_spawn_y(int x, int z) {
    if (x < g_flat_world_origin_x || x >= g_flat_world_origin_x + FLAT_WORLD_SIZE || z < g_flat_world_origin_z || z >= g_flat_world_origin_z + FLAT_WORLD_SIZE) return -9999;
    for (int y = FLAT_WORLD_Y_MAX - 3; y >= FLAT_WORLD_Y_MIN; y--) {
        int id = flat_get_block(x, y, z);
        if (!flat_solid_for_spawn(id)) continue;
        if (flat_get_block(x, y + 1, z) == 0 &&
            flat_get_block(x, y + 2, z) == 0 &&
            flat_get_block(x, y + 3, z) == 0) {
            return y + 1;
        }
    }
    return -9999;
}

static int flat_find_safe_spawn(float *out_x, float *out_y, float *out_z) {
    const int center_x = 0;
    const int center_z = 0;

    int best_x = 0, best_y = -9999, best_z = 0;
    int best_score = 0x7fffffff;

    for (int r = 0; r <= FLAT_WORLD_SIZE / 2 - 2; r++) {
        for (int dz = -r; dz <= r; dz++) {
            for (int dx = -r; dx <= r; dx++) {
                if (abs(dx) != r && abs(dz) != r) continue;
                int x = center_x + dx;
                int z = center_z + dz;
                if (x < g_flat_world_origin_x + 2 || x >= g_flat_world_origin_x + FLAT_WORLD_SIZE - 2 ||
                    z < g_flat_world_origin_z + 2 || z >= g_flat_world_origin_z + FLAT_WORLD_SIZE - 2) continue;

                int sy = flat_column_spawn_y(x, z);
                if (sy == -9999) continue;

                int ground = flat_get_block(x, sy - 1, z);
                if (block_is_liquid(ground) || ground == BLOCK_LEAVES) continue;
                /* Prefer normal surface blocks; avoid spawning on ore cliffs if possible. */
                int surface_bonus = (ground == BLOCK_GRASS || ground == BLOCK_SAND || ground == BLOCK_DIRT) ? 0 : 2000;
                int score = dx*dx + dz*dz + surface_bonus + abs(sy - 34) * 2;

                float px = (float)x + 0.5f;
                float py = (float)sy + 1.62f;
                float pz = (float)z + 0.5f;
                if (!flat_player_aabb_collides(px, py, pz)) {
                    if (score < best_score) {
                        best_score = score;
                        best_x = x; best_y = sy; best_z = z;
                    }
                    if (r <= 8 && surface_bonus == 0) {
                        *out_x = px; *out_y = py; *out_z = pz;
                        return 1;
                    }
                }
            }
        }
        if (best_y != -9999 && r > 16) break;
    }

    if (best_y != -9999) {
        *out_x = (float)best_x + 0.5f;
        *out_y = (float)best_y + 1.62f;
        *out_z = (float)best_z + 0.5f;
        return 1;
    }

    *out_x = 0.5f;
    *out_y = (float)FLAT_WORLD_Y_MAX + 3.0f;
    *out_z = 0.5f;
    return 0;
}

static int flat_player_suffocation_block(void) {
    /* Source does an "inside opaque block" test and renders that block texture over
       the screen.  Use the eye and upper-body samples for this flat renderer. */
    float samples[4][3] = {
        {g_player_x, g_player_y, g_player_z},
        {g_player_x - 0.22f, g_player_y - 0.30f, g_player_z},
        {g_player_x + 0.22f, g_player_y - 0.30f, g_player_z},
        {g_player_x, g_player_y - 0.80f, g_player_z}
    };

    for (int i = 0; i < 4; i++) {
        int bx = (int)floorf(samples[i][0]);
        int by = (int)floorf(samples[i][1]);
        int bz = (int)floorf(samples[i][2]);
        int id = flat_get_block(bx, by, bz);
        if (flat_solid_for_spawn(id) && id != BLOCK_GLASS && id != BLOCK_LEAVES) return id;
    }
    return 0;
}

static int block_is_falling_type(int id) {
    return id == BLOCK_SAND || id == BLOCK_GRAVEL;
}

static int active_world_sim_radius(void) {
    /* Keep expensive falling/liquid simulation close to the player.  Rendering
       distance can be much larger than the area that needs block updates; tying
       these together was the main reason in-world FPS collapsed. */
    int range = g_opts.render_distance * 8 + 8;
    if (range < 24) range = 24;
    if (range > MAX_WORLD_SIM_RADIUS) range = MAX_WORLD_SIM_RADIUS;
    if (range > FLAT_WORLD_SIZE / 2) range = FLAT_WORLD_SIZE / 2;
    return range;
}

static void update_falling_blocks(void) {
    if (g_stream_gen_queue_index < g_stream_gen_queue_count) return;

    /* Only simulate the active area around the player, and do it in small
       batches.  A large cave/open terrain can contain hundreds of unsupported
       sand/gravel blocks; letting all of them fall in one tick keeps render
       chunks dirty forever and turns the frame loop into a rebuild loop. */
    if ((g_ingame_ticks & 3) != 0) return;

    int range = active_world_sim_radius();
    int px = flat_index((int)floorf(g_player_x));
    int pz = flat_z_index((int)floorf(g_player_z));
    if (px < 0) px = 0;
    if (pz < 0) pz = 0;
    if (px >= FLAT_WORLD_SIZE) px = FLAT_WORLD_SIZE - 1;
    if (pz >= FLAT_WORLD_SIZE) pz = FLAT_WORLD_SIZE - 1;

    int min_x = px - range, max_x = px + range;
    int min_z = pz - range, max_z = pz + range;
    if (min_x < 0) min_x = 0;
    if (max_x >= FLAT_WORLD_SIZE) max_x = FLAT_WORLD_SIZE - 1;
    if (min_z < 0) min_z = 0;
    if (max_z >= FLAT_WORLD_SIZE) max_z = FLAT_WORLD_SIZE - 1;

    int moved = 0;
    int changes_left = MAX_WORLD_SIM_CHANGES_PER_TICK;
    for (int y = FLAT_WORLD_Y_MIN + 1; y <= FLAT_WORLD_Y_MAX && changes_left > 0; y++) {
        int yi = flat_y_index(y);
        int byi = flat_y_index(y - 1);
        for (int z = min_z; z <= max_z && changes_left > 0; z++) {
            for (int x = min_x; x <= max_x && changes_left > 0; x++) {
                int id = g_flat_blocks[yi][z][x];
                if (!block_is_falling_type(id)) continue;
                if (g_flat_blocks[byi][z][x] != 0) continue;

                g_flat_blocks[byi][z][x] = id;
                g_flat_blocks[yi][z][x] = 0;
                mark_flat_render_chunks_dirty_around(g_flat_world_origin_x + x,
                                                     g_flat_world_origin_z + z);
                moved = 1;
                changes_left--;
            }
        }
    }

    if (moved) g_save_dirty = 1;
}


static void reset_flat_player_spawn(void) {
    float sx, sy, sz;
    flat_find_safe_spawn(&sx, &sy, &sz);

    g_player_x = g_player_prev_x = sx;
    g_player_y = g_player_prev_y = sy;
    g_player_z = g_player_prev_z = sz;
    g_player_motion_x = g_player_motion_y = g_player_motion_z = 0.0f;
    g_player_fall_distance = 0.0f;
    g_player_dead = 0;
    g_player_death_time = 0;
    g_suffocation_damage_timer = 0;
    g_player_yaw = g_player_prev_yaw = 0.0f;
    g_player_pitch = g_player_prev_pitch = 0.0f;
    g_player_on_ground = 0;
    g_distance_walked = g_prev_distance_walked = 0.0f;
    g_limb_swing = g_prev_limb_swing = 0.0f;
    g_limb_swing_amount = g_prev_limb_swing_amount = 0.0f;
    g_camera_yaw = g_prev_camera_yaw = 0.0f;
    g_camera_pitch = g_prev_camera_pitch = 0.0f;
    g_hand_swing = g_prev_hand_swing = 0.0f;
    g_hand_swing_ticks = 0;
    g_hand_swing_progress = 0.0f;
    g_hand_swing_active = 0;
    g_air_swing_playing = 0;
    g_air_swing_ticks = 0;
    g_air_swing_consumed = 0;
    g_break_swing_consumed = 0;
    g_break_swing_holding = 0;
}




static int liquid_adjacent_source_count(int x, int y, int z, int source_id) {
    int c = 0;
    if (flat_get_block(x + 1, y, z) == source_id) c++;
    if (flat_get_block(x - 1, y, z) == source_id) c++;
    if (flat_get_block(x, y, z + 1) == source_id) c++;
    if (flat_get_block(x, y, z - 1) == source_id) c++;
    return c;
}

static int liquid_near_source_l1(int x, int y, int z, int source_id, int max_dist) {
    for (int dz = -max_dist; dz <= max_dist; dz++) {
        for (int dx = -max_dist; dx <= max_dist; dx++) {
            int d = abs(dx) + abs(dz);
            if (d == 0 || d > max_dist) continue;
            if (flat_get_block(x + dx, y, z + dz) == source_id) return 1;
        }
    }
    return 0;
}

static int liquid_can_occupy(int id) {
    return id == 0 || block_is_liquid(id);
}

static void update_liquids(void) {
    if (g_stream_gen_queue_index < g_stream_gen_queue_count) return;

    /* Safer Beta-style approximation:
       - still water/lava are source blocks
       - flowing water/lava are flow blocks
       - source blocks create downward flow, but do NOT flood sideways forever
       - flowing blocks spread sideways only near a source, approximating source depth
       - 2 adjacent still water sources make an infinite still-water source
    */
    int tick_water = (g_ingame_ticks % 5) == 0;
    int tick_lava = (g_ingame_ticks % 30) == 0;
    if (!tick_water && !tick_lava) return;

    int range = active_world_sim_radius();

    int px = (int)floorf(g_player_x);
    int pz = (int)floorf(g_player_z);
    int min_x = px - range, max_x = px + range;
    int min_z = pz - range, max_z = pz + range;
    if (min_x < g_flat_world_origin_x + 1) min_x = g_flat_world_origin_x + 1;
    if (max_x >= g_flat_world_origin_x + FLAT_WORLD_SIZE - 1) max_x = g_flat_world_origin_x + FLAT_WORLD_SIZE - 2;
    if (min_z < g_flat_world_origin_z + 1) min_z = g_flat_world_origin_z + 1;
    if (max_z >= g_flat_world_origin_z + FLAT_WORLD_SIZE - 1) max_z = g_flat_world_origin_z + FLAT_WORLD_SIZE - 2;

    int moved = 0;
    int changes_left = MAX_WORLD_SIM_CHANGES_PER_TICK;

    /* Infinite water sources: air with solid below and two still-water neighbors. */
    if (tick_water) {
        for (int y = FLAT_WORLD_Y_MIN + 1; y <= FLAT_WORLD_Y_MAX - 1 && changes_left > 0; y++) {
            for (int z = min_z; z <= max_z && changes_left > 0; z++) {
                for (int x = min_x; x <= max_x && changes_left > 0; x++) {
                    if (flat_get_block(x, y, z) != 0) continue;
                    int below = flat_get_block(x, y - 1, z);
                    if (below == 0 || block_is_liquid(below)) continue;
                    if (liquid_adjacent_source_count(x, y, z, BLOCK_STILL_WATER) >= 2) {
                        flat_set_block(x, y, z, BLOCK_STILL_WATER);
                        moved = 1;
                        changes_left--;
                    }
                }
            }
        }
    }

    for (int y = FLAT_WORLD_Y_MAX - 1; y >= FLAT_WORLD_Y_MIN + 1 && changes_left > 0; y--) {
        for (int z = min_z; z <= max_z && changes_left > 0; z++) {
            for (int x = min_x; x <= max_x && changes_left > 0; x++) {
                int id = flat_get_block(x, y, z);
                int is_water = block_is_water(id);
                int is_lava = block_is_lava(id);
                if ((!is_water || !tick_water) && (!is_lava || !tick_lava)) continue;

                int source_id = is_water ? BLOCK_STILL_WATER : BLOCK_STILL_LAVA;
                int flow_id = is_water ? BLOCK_WATER : BLOCK_LAVA;
                int max_dist = is_water ? 4 : 2;

                /* Downward flow from source or moving liquid. */
                if (flat_get_block(x, y - 1, z) == 0) {
                    flat_set_block(x, y - 1, z, flow_id);
                    moved = 1;
                    changes_left--;
                    continue;
                }

                /* Source blocks do not side-spread every tick.  This prevents oceans
                   from turning the whole land surface into source/flow blocks. */
                if (block_is_source_liquid(id)) continue;

                if (!liquid_near_source_l1(x, y, z, source_id, max_dist)) continue;

                int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
                for (int i = 0; i < 4 && changes_left > 0; i++) {
                    int nx = x + dirs[i][0];
                    int nz = z + dirs[i][1];
                    if (flat_get_block(nx, y, nz) != 0) continue;
                    if (!liquid_near_source_l1(nx, y, nz, source_id, max_dist)) continue;
                    flat_set_block(nx, y, nz, flow_id);
                    moved = 1;
                    changes_left--;
                }
            }
        }
    }

    if (moved) g_save_dirty = 1;
}


static int stream_generation_active(void) {
    return g_stream_gen_queue_index < g_stream_gen_queue_count;
}

static void stream_generation_queue_clear(void) {
    g_stream_gen_queue_count = 0;
    g_stream_gen_queue_index = 0;
}

static int stream_world_chunk_in_window(int wcx, int wcz, int origin_x, int origin_z) {
    int wx0 = wcx * FLAT_RENDER_CHUNK;
    int wz0 = wcz * FLAT_RENDER_CHUNK;
    return wx0 >= origin_x && wz0 >= origin_z &&
           wx0 + FLAT_RENDER_CHUNK <= origin_x + FLAT_WORLD_SIZE &&
           wz0 + FLAT_RENDER_CHUNK <= origin_z + FLAT_WORLD_SIZE;
}

static void stream_remap_render_chunks_after_shift(int old_origin_x, int old_origin_z,
                                                    int new_origin_x, int new_origin_z) {
    GLuint old_lists[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    GLuint old_liquid_lists[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_dirty[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_valid[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_has_liquid[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    GLuint reusable_lists[STREAM_GEN_QUEUE_MAX];
    GLuint reusable_liquid_lists[STREAM_GEN_QUEUE_MAX];
    int reusable_count = 0;
    int reusable_index = 0;

    memcpy(old_lists, g_flat_world_chunk_lists, sizeof(old_lists));
    memcpy(old_liquid_lists, g_flat_world_chunk_liquid_lists, sizeof(old_liquid_lists));
    memcpy(old_dirty, g_flat_world_chunk_dirty, sizeof(old_dirty));
    memcpy(old_valid, g_flat_world_chunk_valid, sizeof(old_valid));
    memcpy(old_has_liquid, g_flat_world_chunk_has_liquid, sizeof(old_has_liquid));

    int old_base_cx = floor_div16(old_origin_x);
    int old_base_cz = floor_div16(old_origin_z);
    int new_base_cx = floor_div16(new_origin_x);
    int new_base_cz = floor_div16(new_origin_z);

    for (int ocz = 0; ocz < FLAT_RENDER_CHUNKS; ocz++) {
        for (int ocx = 0; ocx < FLAT_RENDER_CHUNKS; ocx++) {
            int wcx = old_base_cx + ocx;
            int wcz = old_base_cz + ocz;
            if (!stream_world_chunk_in_window(wcx, wcz, new_origin_x, new_origin_z) && old_lists[ocz][ocx] != 0) {
                reusable_lists[reusable_count] = old_lists[ocz][ocx];
                reusable_liquid_lists[reusable_count] = old_liquid_lists[ocz][ocx];
                reusable_count++;
            }
        }
    }

    memset(g_flat_world_chunk_lists, 0, sizeof(g_flat_world_chunk_lists));
    memset(g_flat_world_chunk_liquid_lists, 0, sizeof(g_flat_world_chunk_liquid_lists));
    memset(g_flat_world_chunk_valid, 0, sizeof(g_flat_world_chunk_valid));
    memset(g_flat_world_chunk_has_liquid, 0, sizeof(g_flat_world_chunk_has_liquid));
    for (int ncz = 0; ncz < FLAT_RENDER_CHUNKS; ncz++) {
        for (int ncx = 0; ncx < FLAT_RENDER_CHUNKS; ncx++) {
            int wcx = new_base_cx + ncx;
            int wcz = new_base_cz + ncz;
            if (stream_world_chunk_in_window(wcx, wcz, old_origin_x, old_origin_z)) {
                int ocx = wcx - old_base_cx;
                int ocz = wcz - old_base_cz;
                g_flat_world_chunk_lists[ncz][ncx] = old_lists[ocz][ocx];
                g_flat_world_chunk_liquid_lists[ncz][ncx] = old_liquid_lists[ocz][ocx];
                g_flat_world_chunk_dirty[ncz][ncx] = old_dirty[ocz][ocx];
                g_flat_world_chunk_valid[ncz][ncx] = old_valid[ocz][ocx];
                g_flat_world_chunk_has_liquid[ncz][ncx] = old_has_liquid[ocz][ocx];
            } else {
                if (reusable_index < reusable_count) {
                    g_flat_world_chunk_lists[ncz][ncx] = reusable_lists[reusable_index];
                    g_flat_world_chunk_liquid_lists[ncz][ncx] = reusable_liquid_lists[reusable_index];
                    reusable_index++;
                } else {
                    g_flat_world_chunk_lists[ncz][ncx] = 0;
                    g_flat_world_chunk_liquid_lists[ncz][ncx] = 0;
                }
                g_flat_world_chunk_dirty[ncz][ncx] = 1;
                g_flat_world_chunk_valid[ncz][ncx] = 0;
                g_flat_world_chunk_has_liquid[ncz][ncx] = 0;
            }
        }
    }
    g_flat_world_geometry_dirty = 0;
}

static int stream_world_chunk_local_x(int wcx) {
    return (wcx * FLAT_RENDER_CHUNK - g_flat_world_origin_x) / FLAT_RENDER_CHUNK;
}

static int stream_world_chunk_local_z(int wcz) {
    return (wcz * FLAT_RENDER_CHUNK - g_flat_world_origin_z) / FLAT_RENDER_CHUNK;
}

static void stream_mark_local_chunk_dirty(int lcx, int lcz) {
    if (lcx < 0 || lcx >= FLAT_RENDER_CHUNKS || lcz < 0 || lcz >= FLAT_RENDER_CHUNKS) return;
    g_flat_world_chunk_dirty[lcz][lcx] = 1;
    if (lcx > 0) g_flat_world_chunk_dirty[lcz][lcx - 1] = 1;
    if (lcx + 1 < FLAT_RENDER_CHUNKS) g_flat_world_chunk_dirty[lcz][lcx + 1] = 1;
    if (lcz > 0) g_flat_world_chunk_dirty[lcz - 1][lcx] = 1;
    if (lcz + 1 < FLAT_RENDER_CHUNKS) g_flat_world_chunk_dirty[lcz + 1][lcx] = 1;
}

static void stream_queue_add_chunk(int wcx, int wcz) {
    if (g_stream_gen_queue_count >= STREAM_GEN_QUEUE_MAX) return;
    g_stream_gen_queue_cx[g_stream_gen_queue_count] = wcx;
    g_stream_gen_queue_cz[g_stream_gen_queue_count] = wcz;
    g_stream_gen_queue_count++;
}

static void stream_queue_missing_chunks_near_player(int old_origin_x, int old_origin_z) {
    stream_generation_queue_clear();

    int pcx = stream_world_chunk_local_x(floor_div16((int)floorf(g_player_x)));
    int pcz = stream_world_chunk_local_z(floor_div16((int)floorf(g_player_z)));
    if (pcx < 0) pcx = 0;
    if (pcz < 0) pcz = 0;
    if (pcx >= FLAT_RENDER_CHUNKS) pcx = FLAT_RENDER_CHUNKS - 1;
    if (pcz >= FLAT_RENDER_CHUNKS) pcz = FLAT_RENDER_CHUNKS - 1;

    int base_cx = floor_div16(g_flat_world_origin_x);
    int base_cz = floor_div16(g_flat_world_origin_z);

    /* Queue only chunks that are newly exposed by the window shift.  The old
       version regenerated the entire 256x256 window synchronously here, which
       is exactly the hitch seen when the chat says "Generated new terrain
       chunks."  Ring order makes the nearest missing terrain appear first. */
    for (int ring = 0; ring < FLAT_RENDER_CHUNKS; ring++) {
        for (int dz = -ring; dz <= ring; dz++) {
            for (int dx = -ring; dx <= ring; dx++) {
                if (ring != 0 && abs(dx) != ring && abs(dz) != ring) continue;
                int lcx = pcx + dx;
                int lcz = pcz + dz;
                if (lcx < 0 || lcx >= FLAT_RENDER_CHUNKS || lcz < 0 || lcz >= FLAT_RENDER_CHUNKS) continue;
                int wcx = base_cx + lcx;
                int wcz = base_cz + lcz;
                if (stream_world_chunk_in_window(wcx, wcz, old_origin_x, old_origin_z)) continue;
                stream_queue_add_chunk(wcx, wcz);
            }
        }
    }
}

static void process_stream_generation_queue(void) {
    if (g_world_type != 1) {
        stream_generation_queue_clear();
        return;
    }

    int chunks_left = STREAM_CHUNKS_PER_TICK;
    while (chunks_left > 0 && stream_generation_active()) {
        int wcx = g_stream_gen_queue_cx[g_stream_gen_queue_index];
        int wcz = g_stream_gen_queue_cz[g_stream_gen_queue_index];
        g_stream_gen_queue_index++;

        beta_preview_copy_chunk_to_flat(wcx, wcz);
        stream_mark_local_chunk_dirty(stream_world_chunk_local_x(wcx), stream_world_chunk_local_z(wcz));
        chunks_left--;
    }

    if (!stream_generation_active()) {
        stream_generation_queue_clear();
    }
}

static void update_infinite_world_streaming(void) {
    if (g_world_type != 1) return;

    /* Do a tiny amount of terrain generation per tick.  This is the important
       part: new chunks are no longer built all at once on the frame that the
       streaming message appears. */
    process_stream_generation_queue();
    if (stream_generation_active()) return;

    int pcx = floor_div16((int)floorf(g_player_x));
    int pcz = floor_div16((int)floorf(g_player_z));

    int margin = 48;
    int new_origin_x = g_flat_world_origin_x;
    int new_origin_z = g_flat_world_origin_z;

    /* Slide by one chunk at a time instead of recentring the whole 256x256
       window.  Recentering could expose five or more strips at once, which then
       required hundreds of chunks to be generated. */
    if (g_player_x > g_flat_world_origin_x + FLAT_WORLD_SIZE - margin) {
        new_origin_x += FLAT_RENDER_CHUNK;
    } else if (g_player_x < g_flat_world_origin_x + margin) {
        new_origin_x -= FLAT_RENDER_CHUNK;
    }

    if (g_player_z > g_flat_world_origin_z + FLAT_WORLD_SIZE - margin) {
        new_origin_z += FLAT_RENDER_CHUNK;
    } else if (g_player_z < g_flat_world_origin_z + margin) {
        new_origin_z -= FLAT_RENDER_CHUNK;
    }

    if (new_origin_x == g_flat_world_origin_x && new_origin_z == g_flat_world_origin_z) {
        g_stream_last_center_chunk_x = pcx;
        g_stream_last_center_chunk_z = pcz;
        return;
    }

    static int old_blocks[FLAT_WORLD_HEIGHT][FLAT_WORLD_SIZE][FLAT_WORLD_SIZE];
    int old_origin_x = g_flat_world_origin_x;
    int old_origin_z = g_flat_world_origin_z;
    memcpy(old_blocks, g_flat_blocks, sizeof(g_flat_blocks));

    g_flat_world_origin_x = new_origin_x;
    g_flat_world_origin_z = new_origin_z;
    stream_remap_render_chunks_after_shift(old_origin_x, old_origin_z, new_origin_x, new_origin_z);
    memset(g_flat_blocks, 0, sizeof(g_flat_blocks));

    /* Preserve edited/generated blocks in the overlap of the old and new
       window.  Newly exposed strips stay empty for a few frames and are filled
       by process_stream_generation_queue(), closest chunks first. */
    int ox0 = old_origin_x > g_flat_world_origin_x ? old_origin_x : g_flat_world_origin_x;
    int oz0 = old_origin_z > g_flat_world_origin_z ? old_origin_z : g_flat_world_origin_z;
    int ox1 = (old_origin_x + FLAT_WORLD_SIZE) < (g_flat_world_origin_x + FLAT_WORLD_SIZE) ? (old_origin_x + FLAT_WORLD_SIZE) : (g_flat_world_origin_x + FLAT_WORLD_SIZE);
    int oz1 = (old_origin_z + FLAT_WORLD_SIZE) < (g_flat_world_origin_z + FLAT_WORLD_SIZE) ? (old_origin_z + FLAT_WORLD_SIZE) : (g_flat_world_origin_z + FLAT_WORLD_SIZE);
    for (int z = oz0; z < oz1; z++) {
        for (int x = ox0; x < ox1; x++) {
            int old_fx = x - old_origin_x;
            int old_fz = z - old_origin_z;
            int new_fx = x - g_flat_world_origin_x;
            int new_fz = z - g_flat_world_origin_z;
            for (int y = 0; y < FLAT_WORLD_HEIGHT; y++) {
                g_flat_blocks[y][new_fz][new_fx] = old_blocks[y][old_fz][old_fx];
            }
        }
    }

    stream_queue_missing_chunks_near_player(old_origin_x, old_origin_z);
    process_stream_generation_queue();

    g_stream_last_center_chunk_x = pcx;
    g_stream_last_center_chunk_z = pcz;

    if (g_stream_gen_queue_count > 0) {
        hud_add_chat("Generating terrain chunks...");
    } else {
        hud_add_chat("Generated new terrain chunks.");
    }
}


static int flat_player_head_block(void) {
    /* g_player_y is the first-person camera / eye height.  Render-only water
       effects must use this sample, not the player's feet or body, otherwise
       shallow water paints the whole view blue while the head is still above it. */
    int bx = (int)floorf(g_player_x);
    int by = (int)floorf(g_player_y);
    int bz = (int)floorf(g_player_z);
    return flat_get_block(bx, by, bz);
}

static int flat_player_head_in_water(void) {
    return block_is_water(flat_player_head_block());
}

static int flat_player_head_in_lava(void) {
    return block_is_lava(flat_player_head_block());
}

static int flat_player_in_water(void) {
    /* Physics/swimming intentionally still checks body/feet contact. */
    int bx = (int)floorf(g_player_x);
    int by = (int)floorf(g_player_y - 0.25f);
    int bz = (int)floorf(g_player_z);
    return block_is_water(flat_get_block(bx, by, bz)) || block_is_water(flat_get_block(bx, by - 1, bz));
}

static int flat_player_in_lava(void) {
    /* Physics/swimming intentionally still checks body/feet contact. */
    int bx = (int)floorf(g_player_x);
    int by = (int)floorf(g_player_y - 0.25f);
    int bz = (int)floorf(g_player_z);
    return block_is_lava(flat_get_block(bx, by, bz)) || block_is_lava(flat_get_block(bx, by - 1, bz));
}
