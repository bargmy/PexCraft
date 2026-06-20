/* Split from original monolithic main.c. Included by src/main.c unity build. */

static int stack_empty(const ItemStack *s) { return !s || s->id <= 0 || s->count <= 0; }

static void stack_clear(ItemStack *s) { if (s) memset(s, 0, sizeof(*s)); }

static int stack_same_item(const ItemStack *a, const ItemStack *b) { return !stack_empty(a) && !stack_empty(b) && a->id == b->id && a->damage == b->damage; }

static int stack_limit_for_id(int id) {
    if (id == ITEM_WOODEN_SWORD || id == ITEM_WOODEN_SHOVEL || id == ITEM_WOODEN_PICKAXE || id == ITEM_WOODEN_AXE ||
        id == ITEM_STONE_SWORD || id == ITEM_STONE_SHOVEL || id == ITEM_STONE_PICKAXE || id == ITEM_STONE_AXE ||
        id == ITEM_IRON_SWORD || id == ITEM_IRON_SHOVEL || id == ITEM_IRON_PICKAXE || id == ITEM_IRON_AXE ||
        id == ITEM_DIAMOND_SWORD || id == ITEM_DIAMOND_SHOVEL || id == ITEM_DIAMOND_PICKAXE || id == ITEM_DIAMOND_AXE ||
        id == ITEM_GOLD_SWORD || id == ITEM_GOLD_SHOVEL || id == ITEM_GOLD_PICKAXE || id == ITEM_GOLD_AXE ||
        id == ITEM_WOODEN_HOE || id == ITEM_STONE_HOE || id == ITEM_IRON_HOE || id == ITEM_DIAMOND_HOE || id == ITEM_GOLD_HOE ||
        id == ITEM_BOW || id == ITEM_FLINT_AND_IRON ||
        id == ITEM_LEATHER_CAP || id == ITEM_LEATHER_CHESTPLATE || id == ITEM_LEATHER_LEGGINGS || id == ITEM_LEATHER_BOOTS ||
        id == ITEM_CHAIN_HELMET || id == ITEM_CHAIN_CHESTPLATE || id == ITEM_CHAIN_LEGGINGS || id == ITEM_CHAIN_BOOTS ||
        id == ITEM_IRON_HELMET || id == ITEM_IRON_CHESTPLATE || id == ITEM_IRON_LEGGINGS || id == ITEM_IRON_BOOTS ||
        id == ITEM_DIAMOND_HELMET || id == ITEM_DIAMOND_CHESTPLATE || id == ITEM_DIAMOND_LEGGINGS || id == ITEM_DIAMOND_BOOTS ||
        id == ITEM_GOLD_HELMET || id == ITEM_GOLD_CHESTPLATE || id == ITEM_GOLD_LEGGINGS || id == ITEM_GOLD_BOOTS ||
        id == ITEM_MINECART || id == ITEM_MINECART_CRATE || id == ITEM_MINECART_POWERED || id == ITEM_BOAT ||
        id == ITEM_SADDLE || id == ITEM_WOOD_DOOR || id == ITEM_IRON_DOOR || id == ITEM_SIGN ||
        id == ITEM_BUCKET_EMPTY || id == ITEM_WATER_BUCKET || id == ITEM_LAVA_BUCKET || id == ITEM_MILK ||
        id == ITEM_APPLE_RED || id == ITEM_MUSHROOM_STEW || id == ITEM_BREAD || id == ITEM_APPLE_GOLD ||
        id == ITEM_PORK_RAW || id == ITEM_PORK_COOKED || id == ITEM_FISH_RAW || id == ITEM_FISH_COOKED ||
        id == ITEM_RECORD13 || id == ITEM_RECORD_CAT) return 1;
    if (id == ITEM_SNOWBALL || id == ITEM_EGG) return 16;
    (void)id;
    return ITEM_MAX_STACK;
}

static ItemStack make_stack(int id, int count, int damage) { ItemStack s; s.id = id; s.count = count; s.damage = damage; s.pop_time = 0; return s; }

static int flat_index(int coord) { return coord - g_flat_world_origin_x; }
static int flat_z_index(int coord) { return coord - g_flat_world_origin_z; }
static int floor_div16(int v);
static void mark_flat_render_chunks_dirty_around(int x, int z);
static void mark_flat_render_sections_dirty_around_block(int x, int y, int z);
static void flat_mark_section_after_generation(int lcx, int lcz, int sy);
static void mark_flat_chunk_modified_at(int x, int z);
static int g_flat_persistent_edit_depth = 0;
static void flat_begin_persistent_edit(void) { g_flat_persistent_edit_depth++; }
static void flat_end_persistent_edit(void) { if (g_flat_persistent_edit_depth > 0) g_flat_persistent_edit_depth--; }
static int flat_persistent_edit_active(void) { return g_flat_persistent_edit_depth > 0; }
static int flat_block_intersects_player(int bx, int by, int bz);
static void spawn_item_stack(float x, float y, float z, ItemStack st, int random_spread);
static int stream_world_chunk_in_window(int wcx, int wcz, int origin_x, int origin_z);
static int stream_world_chunk_local_x(int wcx);
static int stream_world_chunk_local_z(int wcz);
static void stream_mark_local_chunk_dirty(int lcx, int lcz);
static void stream_mark_local_chunk_generated(int lcx, int lcz);
static void stream_queue_add_chunk(int wcx, int wcz);
static int stream_queue_missing_visible_chunks_near_player(void);
static void process_stream_generation_queue(void);
static int stream_generation_active(void);
static void wake_neighbor_liquids(int x, int y, int z);
static void fluid_check_for_harden(int x, int y, int z);
static void flat_place_fluid_source(int x, int y, int z, int id);

static int flat_y_index(int y) { return y - FLAT_WORLD_Y_MIN; }

static int flat_in_bounds(int x, int y, int z) {
    return y >= FLAT_WORLD_Y_MIN && y <= FLAT_WORLD_Y_MAX &&
           x >= g_flat_world_origin_x && x < g_flat_world_origin_x + FLAT_WORLD_SIZE &&
           z >= g_flat_world_origin_z && z < g_flat_world_origin_z + FLAT_WORLD_SIZE;
}

static int flat_local_chunk_x_for_world(int x) { return flat_index(x) / FLAT_RENDER_CHUNK; }
static int flat_local_chunk_z_for_world(int z) { return flat_z_index(z) / FLAT_RENDER_CHUNK; }
static int flat_section_y_for_world(int y) { return (y - FLAT_WORLD_Y_MIN) / FLAT_RENDER_SECTION; }

static int flat_local_chunk_valid(int lcx, int lcz) {
    return lcx >= 0 && lcx < FLAT_RENDER_CHUNKS && lcz >= 0 && lcz < FLAT_RENDER_CHUNKS;
}

static int flat_section_index_valid(int sy) {
    return sy >= 0 && sy < FLAT_RENDER_SECTIONS_Y;
}

static int flat_scan_section_nonempty_local(int lcx, int lcz, int sy) {
    if (!flat_local_chunk_valid(lcx, lcz) || !flat_section_index_valid(sy)) return 0;
    int x0 = lcx * FLAT_RENDER_CHUNK;
    int z0 = lcz * FLAT_RENDER_CHUNK;
    int y0 = FLAT_WORLD_Y_MIN + sy * FLAT_RENDER_SECTION;
    int y1 = y0 + FLAT_RENDER_SECTION;
    if (y1 > FLAT_WORLD_Y_MAX + 1) y1 = FLAT_WORLD_Y_MAX + 1;
    for (int y = y0; y < y1; y++) {
        int yi = flat_y_index(y);
        for (int z = z0; z < z0 + FLAT_RENDER_CHUNK; z++) {
            for (int x = x0; x < x0 + FLAT_RENDER_CHUNK; x++) {
                if (g_flat_blocks[yi][z][x] != 0) return 1;
            }
        }
    }
    return 0;
}

static int flat_refresh_section_occupancy_local(int lcx, int lcz, int sy) {
    if (!flat_local_chunk_valid(lcx, lcz) || !flat_section_index_valid(sy)) return 0;
    unsigned short bit = (unsigned short)(1u << sy);
    int nonempty = flat_scan_section_nonempty_local(lcx, lcz, sy);
    if (nonempty) g_flat_chunk_section_non_empty_mask[lcz][lcx] |= bit;
    else g_flat_chunk_section_non_empty_mask[lcz][lcx] &= (unsigned short)~bit;
    return nonempty;
}

static unsigned short flat_refresh_chunk_section_occupancy_local(int lcx, int lcz) {
    if (!flat_local_chunk_valid(lcx, lcz)) return 0;
    unsigned short mask = 0;
    for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
        if (flat_scan_section_nonempty_local(lcx, lcz, sy)) mask |= (unsigned short)(1u << sy);
    }
    g_flat_chunk_section_non_empty_mask[lcz][lcx] = mask;
    return mask;
}

static void flat_update_section_occupancy_after_block_change(int x, int y, int z, int new_id) {
    if (!flat_in_bounds(x, y, z)) return;
    int lcx = flat_local_chunk_x_for_world(x);
    int lcz = flat_local_chunk_z_for_world(z);
    int sy = flat_section_y_for_world(y);
    if (!flat_local_chunk_valid(lcx, lcz) || !flat_section_index_valid(sy)) return;
    unsigned short bit = (unsigned short)(1u << sy);
    if (new_id != 0) {
        g_flat_chunk_section_non_empty_mask[lcz][lcx] |= bit;
    } else if (g_flat_chunk_section_non_empty_mask[lcz][lcx] & bit) {
        flat_refresh_section_occupancy_local(lcx, lcz, sy);
    }
}

static int flat_get_block(int x, int y, int z) {
    if (g_async_mesh_blocks) {
        if (x < g_async_mesh_x0 || y < g_async_mesh_y0 || z < g_async_mesh_z0 ||
            x >= g_async_mesh_x0 + g_async_mesh_w ||
            y >= g_async_mesh_y0 + g_async_mesh_h ||
            z >= g_async_mesh_z0 + g_async_mesh_d) return 0;
        int ix = x - g_async_mesh_x0;
        int iy = y - g_async_mesh_y0;
        int iz = z - g_async_mesh_z0;
        return g_async_mesh_blocks[((iy * g_async_mesh_d) + iz) * g_async_mesh_w + ix];
    }
    if (!flat_in_bounds(x, y, z)) return 0;
    return g_flat_blocks[flat_y_index(y)][flat_z_index(z)][flat_index(x)];
}

static int flat_get_meta(int x, int y, int z) {
    if (g_async_mesh_meta) {
        if (x < g_async_mesh_x0 || y < g_async_mesh_y0 || z < g_async_mesh_z0 ||
            x >= g_async_mesh_x0 + g_async_mesh_w ||
            y >= g_async_mesh_y0 + g_async_mesh_h ||
            z >= g_async_mesh_z0 + g_async_mesh_d) return 0;
        int ix = x - g_async_mesh_x0;
        int iy = y - g_async_mesh_y0;
        int iz = z - g_async_mesh_z0;
        return g_async_mesh_meta[((iy * g_async_mesh_d) + iz) * g_async_mesh_w + ix];
    }
    if (!flat_in_bounds(x, y, z)) return 0;
    return g_flat_meta[flat_y_index(y)][flat_z_index(z)][flat_index(x)];
}

/* Fluid level metadata (Beta): 0 source, 1-7 flow decay, bit 0x8 = falling.
   g_flat_meta mirrors this only for liquid blocks so existing chunk-delta saves
   preserve fluid state without losing door/stairs/chest metadata. */
#define FLUID_LEVEL_MAX 7
#define FLUID_FALLING 0x8
static int block_is_liquid(int id);
static int block_is_water(int id);
static int block_is_lava(int id);
static int block_is_flowing_liquid(int id);
static int flat_get_level(int x, int y, int z) {
    if (g_async_mesh_levels) {
        if (x < g_async_mesh_x0 || y < g_async_mesh_y0 || z < g_async_mesh_z0 ||
            x >= g_async_mesh_x0 + g_async_mesh_w ||
            y >= g_async_mesh_y0 + g_async_mesh_h ||
            z >= g_async_mesh_z0 + g_async_mesh_d) return 0;
        int ix = x - g_async_mesh_x0;
        int iy = y - g_async_mesh_y0;
        int iz = z - g_async_mesh_z0;
        return g_async_mesh_levels[((iy * g_async_mesh_d) + iz) * g_async_mesh_w + ix];
    }
    if (!flat_in_bounds(x, y, z)) return 0;
    return g_flat_levels[flat_y_index(y)][flat_z_index(z)][flat_index(x)];
}

static void flat_set_level_raw(int x, int y, int z, int level) {
    if (!flat_in_bounds(x, y, z)) return;
    int yi = flat_y_index(y), zi = flat_z_index(z), xi = flat_index(x);
    g_flat_levels[yi][zi][xi] = (unsigned char)(level & 15);
    if (block_is_liquid(g_flat_blocks[yi][zi][xi])) g_flat_meta[yi][zi][xi] = (unsigned char)(level & 15);
}

static float fluid_decay_height(int level) {
    if (level >= 8) level = 0;
    if (level < 0) level = 0;
    return (float)(level + 1) / 9.0f;
}

static void flat_set_meta_raw(int x, int y, int z, int meta) {
    if (!flat_in_bounds(x, y, z)) return;
    unsigned char *cell = &g_flat_meta[flat_y_index(y)][flat_z_index(z)][flat_index(x)];
    if (*cell != (unsigned char)(meta & 255)) {
        *cell = (unsigned char)(meta & 255);
        if (block_is_liquid(flat_get_block(x, y, z))) {
            g_flat_levels[flat_y_index(y)][flat_z_index(z)][flat_index(x)] = (unsigned char)(meta & 15);
        }
        mark_flat_render_sections_dirty_around_block(x, y, z);
        if (flat_persistent_edit_active()) {
            mark_flat_chunk_modified_at(x, z);
            g_save_dirty = 1;
        }
    }
}

static int block_is_door_id(int id) { return id == BLOCK_WOOD_DOOR || id == BLOCK_IRON_DOOR; }
static int door_meta_is_upper(int meta) { return (meta & 8) != 0; }
static int door_meta_is_open(int meta) { return (meta & 4) != 0; }

static int door_lower_y_at(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (!block_is_door_id(id)) return y;
    return door_meta_is_upper(flat_get_meta(x, y, z)) ? y - 1 : y;
}


#ifndef FLAT_CHUNK_BLOCK_COUNT
#define FLAT_CHUNK_BLOCK_COUNT (FLAT_RENDER_CHUNK * FLAT_RENDER_CHUNK * FLAT_WORLD_HEIGHT)
#endif
static int flat_chunk_buf_index(int lx, int y, int lz) {
    return flat_y_index(y) * (FLAT_RENDER_CHUNK * FLAT_RENDER_CHUNK) + lz * FLAT_RENDER_CHUNK + lx;
}

static void flat_chunk_delta_path_current(int cx, int cz, char *out, size_t cap, int create_dir) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    (void)cx; (void)cz; (void)create_dir; if (cap) out[0] = 0; return;
#endif
    if (!g_loaded_world_dir[0]) {
        if (cap) out[0] = 0;
        return;
    }
    char bx[32], bz[32];
    base36_i32(cx, bx, sizeof(bx));
    base36_i32(cz, bz, sizeof(bz));
    char dir[MAX_PATHBUF];
    snprintf(dir, sizeof(dir), "%s\\chunks", g_loaded_world_dir);
    if (create_dir) make_dir_recursive(dir);
    snprintf(out, cap, "%s\\c.%s.%s.dat", dir, bx, bz);
}

static void flat_chunk_delta_path_legacy(int cx, int cz, char *out, size_t cap) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    (void)cx; (void)cz; if (cap) out[0] = 0; return;
#endif
    if (!g_loaded_world_dir[0]) {
        if (cap) out[0] = 0;
        return;
    }
    char bx[32], bz[32];
    base36_i32(cx, bx, sizeof(bx));
    base36_i32(cz, bz, sizeof(bz));
    snprintf(out, cap, "%s\\pexcraft_modified_chunks\\c.%s.%s.pcd", g_loaded_world_dir, bx, bz);
}

static void flat_chunk_delta_path(int cx, int cz, char *out, size_t cap) {
    flat_chunk_delta_path_current(cx, cz, out, cap, 1);
}

static void mark_flat_chunk_modified_at(int x, int z) {
    if (x < g_flat_world_origin_x || x >= g_flat_world_origin_x + FLAT_WORLD_SIZE ||
        z < g_flat_world_origin_z || z >= g_flat_world_origin_z + FLAT_WORLD_SIZE) return;
    int lcx = flat_index(x) / FLAT_RENDER_CHUNK;
    int lcz = flat_z_index(z) / FLAT_RENDER_CHUNK;
    if (lcx >= 0 && lcx < FLAT_RENDER_CHUNKS && lcz >= 0 && lcz < FLAT_RENDER_CHUNKS) {
        g_flat_world_chunk_modified[lcz][lcx] = 1;
    }
}

static void mark_flat_chunks_modified_all(void) {
    for (int cz = 0; cz < FLAT_RENDER_CHUNKS; cz++) {
        for (int cx = 0; cx < FLAT_RENDER_CHUNKS; cx++) {
            g_flat_world_chunk_modified[cz][cx] = 1;
        }
    }
}

static TerrainProvider *beta_stream_provider(void);

#if defined(PEX_PLATFORM_PSP)
/* PSP-safe "normal" terrain: the exact Beta generator allocates and populates a
   3x3 chunk canvas for every streamed chunk.  That is too heavy for PSP and was
   closing PPSSPP during normal-world generation.  This keeps the normal-world
   look but is deterministic, chunk-local, and cheap enough for cooperative
   streaming. */
static unsigned int psp_safe_terrain_hash(int x, int z, int salt, long long seed) {
    unsigned int h = (unsigned int)(x * 374761393u + z * 668265263u + salt * 1442695041u + (unsigned int)seed);
    h ^= (unsigned int)((unsigned long long)seed >> 32);
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static float psp_safe_noise(int x, int z, int scale, int salt, long long seed) {
    int gx = (x >= 0) ? x / scale : (x - scale + 1) / scale;
    int gz = (z >= 0) ? z / scale : (z - scale + 1) / scale;
    unsigned int h = psp_safe_terrain_hash(gx, gz, salt, seed);
    return ((float)(h & 65535u) / 65535.0f) * 2.0f - 1.0f;
}
static int psp_safe_height_at(int x, int z, long long seed) {
    float n1 = psp_safe_noise(x, z, 18, 1, seed) * 10.0f;
    float n2 = psp_safe_noise(x, z, 7, 2, seed) * 4.0f;
    float n3 = psp_safe_noise(x, z, 37, 3, seed) * 14.0f;
    int h = 32 + (int)(n1 + n2 + n3);
    if (h < 8) h = 8;
    if (h > 58) h = 58;
    return h;
}
#endif

static void generate_flat_chunk_base_to_buffer_ex(int cx, int cz, unsigned char *out, int world_type, long long seed, TerrainProvider *reuse_tp) {
    if (!out) return;
    memset(out, 0, FLAT_CHUNK_BLOCK_COUNT);

    if (world_type == 1) {
#if defined(PEX_PLATFORM_PSP)
        for (int lx = 0; lx < 16; lx++) {
            int wx = cx * 16 + lx;
            for (int lz = 0; lz < 16; lz++) {
                int wz = cz * 16 + lz;
                int h = psp_safe_height_at(wx, wz, seed);
                for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
                    int id = 0;
                    if (y == 0) id = BLOCK_BEDROCK;
                    else if (y < h - 4) {
                        unsigned int oh = psp_safe_terrain_hash(wx, wz, y * 31 + 7, seed);
                        if (y < 18 && (oh % 900u) < 4u) id = BLOCK_DIAMOND_ORE;
                        else if (y < 32 && (oh % 360u) < 4u) id = BLOCK_GOLD_ORE;
                        else if (y < 45 && (oh % 180u) < 5u) id = BLOCK_IRON_ORE;
                        else if (y < 58 && (oh % 115u) < 6u) id = BLOCK_COAL_ORE;
                        else id = BLOCK_STONE;
                    } else if (y < h) id = BLOCK_DIRT;
                    else if (y == h) id = BLOCK_GRASS;
                    else if (y <= 30 && h < 30) id = (y == 30) ? BLOCK_STILL_WATER : BLOCK_WATER;
                    out[flat_chunk_buf_index(lx, y, lz)] = (unsigned char)id;
                }
            }
        }
#else
        unsigned char beta_blocks[32768];
        memset(beta_blocks, 0, sizeof(beta_blocks));

        TerrainProvider local_tp;
        TerrainProvider *tp = reuse_tp;
        int free_local_tp = 0;
        if (!tp) {
            terrain_provider_init(&local_tp, (int64_t)seed);
            tp = &local_tp;
            free_local_tp = 1;
        }

        GenCanvas cv;
        cv.minCx = cx - 1;
        cv.minCz = cz - 1;
        cv.chunks = 3;
        cv.blocks = (unsigned char*)calloc((size_t)cv.chunks * (size_t)cv.chunks * 32768u, 1);
        if (cv.blocks) {
            for (int dz = 0; dz < cv.chunks; dz++) {
                for (int dx = 0; dx < cv.chunks; dx++) {
                    generate_canvas_chunk(tp, &cv, cv.minCx + dx, cv.minCz + dz);
                }
            }
            for (int pz = cz - 1; pz <= cz; pz++) {
                for (int px = cx - 1; px <= cx; px++) {
                    qm_populate_canvas(tp, &cv, px, pz);
                }
            }
            extract_canvas_chunk(&cv, cx, cz, beta_blocks);
            free(cv.blocks);
        }
        if (free_local_tp) terrain_provider_free(&local_tp);

        for (int lx = 0; lx < 16; lx++) {
            for (int lz = 0; lz < 16; lz++) {
                for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
                    int id = 0;
                    if (y < 128) {
                        id = get_block_local(beta_blocks, lx, y, lz);
                    }
                    out[flat_chunk_buf_index(lx, y, lz)] = (unsigned char)id;
                }
                if (out[flat_chunk_buf_index(lx, 0, lz)] == 0) {
                    out[flat_chunk_buf_index(lx, 0, lz)] = BLOCK_BEDROCK;
                }
            }
        }
#endif
    } else {
        for (int lx = 0; lx < 16; lx++) {
            for (int lz = 0; lz < 16; lz++) {
                out[flat_chunk_buf_index(lx, 0, lz)] = BLOCK_BEDROCK;
                out[flat_chunk_buf_index(lx, 1, lz)] = BLOCK_DIRT;
                out[flat_chunk_buf_index(lx, 2, lz)] = BLOCK_DIRT;
                out[flat_chunk_buf_index(lx, 3, lz)] = BLOCK_GRASS;
            }
        }
    }
}

static void generate_flat_chunk_base_to_buffer(int cx, int cz, unsigned char *out) {
    TerrainProvider *reuse = (g_world_type == 1) ? beta_stream_provider() : NULL;
    generate_flat_chunk_base_to_buffer_ex(cx, cz, out, g_world_type, g_world_seed, reuse);
}

static void copy_flat_chunk_buffer_to_world(int cx, int cz, const unsigned char *buf) {
    if (!buf) return;
    for (int lx = 0; lx < 16; lx++) {
        int wx = cx * 16 + lx;
        if (wx < g_flat_world_origin_x || wx >= g_flat_world_origin_x + FLAT_WORLD_SIZE) continue;
        int fx = flat_index(wx);
        for (int lz = 0; lz < 16; lz++) {
            int wz = cz * 16 + lz;
            if (wz < g_flat_world_origin_z || wz >= g_flat_world_origin_z + FLAT_WORLD_SIZE) continue;
            int fz = flat_z_index(wz);
            for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
                g_flat_blocks[flat_y_index(y)][fz][fx] = buf[flat_chunk_buf_index(lx, y, lz)];
                g_flat_meta[flat_y_index(y)][fz][fx] = 0;
                g_flat_levels[flat_y_index(y)][fz][fx] = 0;
            }
        }
    }
    int lcx = stream_world_chunk_local_x(cx);
    int lcz = stream_world_chunk_local_z(cz);
    if (flat_local_chunk_valid(lcx, lcz)) flat_refresh_chunk_section_occupancy_local(lcx, lcz);
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_FAST_WORLD) && PEX_PSP_FAST_WORLD
    psp_fast_surface_mark_dirty_block(cx * 16 + 8, cz * 16 + 8);
#endif
}

static void load_modified_flat_chunk_delta_into_flat(int cx, int cz) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    (void)cx; (void)cz; return;
#endif
    if (!g_loaded_world_dir[0]) return;
    char path[MAX_PATHBUF];
    flat_chunk_delta_path_current(cx, cz, path, sizeof(path), 0);
    FILE *f = fopen(path, "rb");
    if (!f) {
        flat_chunk_delta_path_legacy(cx, cz, path, sizeof(path));
        f = fopen(path, "rb");
    }
    if (!f) return;
    char magic[8];
    int rcx = 0, rcz = 0, count = 0;
    if (fread(magic, 1, 8, f) != 8 ||
        fread(&rcx, sizeof(rcx), 1, f) != 1 ||
        fread(&rcz, sizeof(rcz), 1, f) != 1 ||
        fread(&count, sizeof(count), 1, f) != 1 ||
        (memcmp(magic, "CHNKDLT1", 8) != 0 && memcmp(magic, "CHNKDLT2", 8) != 0 && memcmp(magic, "PXCDLT12", 8) != 0) ||
        rcx != cx || rcz != cz || count != FLAT_CHUNK_BLOCK_COUNT) {
        fclose(f);
        return;
    }
    unsigned char *buf = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
    unsigned char *meta = (unsigned char*)calloc(FLAT_CHUNK_BLOCK_COUNT, 1);
    if (!buf || !meta) { free(buf); free(meta); fclose(f); return; }
    int ok = fread(buf, 1, FLAT_CHUNK_BLOCK_COUNT, f) == FLAT_CHUNK_BLOCK_COUNT;
    int has_meta = (memcmp(magic, "CHNKDLT2", 8) == 0);
    if (ok && has_meta) ok = fread(meta, 1, FLAT_CHUNK_BLOCK_COUNT, f) == FLAT_CHUNK_BLOCK_COUNT;
    fclose(f);
    if (ok) {
        copy_flat_chunk_buffer_to_world(cx, cz, buf);
        if (has_meta) {
            for (int lx = 0; lx < 16; lx++) {
                int wx = cx * 16 + lx;
                if (wx < g_flat_world_origin_x || wx >= g_flat_world_origin_x + FLAT_WORLD_SIZE) continue;
                int fx = flat_index(wx);
                for (int lz = 0; lz < 16; lz++) {
                    int wz = cz * 16 + lz;
                    if (wz < g_flat_world_origin_z || wz >= g_flat_world_origin_z + FLAT_WORLD_SIZE) continue;
                    int fz = flat_z_index(wz);
                    for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
                        int bi = flat_chunk_buf_index(lx, y, lz);
                        int id = g_flat_blocks[flat_y_index(y)][fz][fx];
                        g_flat_meta[flat_y_index(y)][fz][fx] = meta[bi];
                        g_flat_levels[flat_y_index(y)][fz][fx] = block_is_liquid(id) ? (meta[bi] & 15) : 0;
                    }
                }
            }
        }
    }
    free(buf);
    free(meta);
}


static void flat_chunk_delta_path_current_for_dir(const char *world_dir, int cx, int cz, char *out, size_t cap, int create_dir) {
    if (!world_dir || !world_dir[0]) {
        if (cap) out[0] = 0;
        return;
    }
    char bx[32], bz[32];
    base36_i32(cx, bx, sizeof(bx));
    base36_i32(cz, bz, sizeof(bz));
    char dir[MAX_PATHBUF];
    snprintf(dir, sizeof(dir), "%s\\chunks", world_dir);
    if (create_dir) make_dir_recursive(dir);
    snprintf(out, cap, "%s\\c.%s.%s.dat", dir, bx, bz);
}

static void flat_chunk_delta_path_legacy_for_dir(const char *world_dir, int cx, int cz, char *out, size_t cap) {
    if (!world_dir || !world_dir[0]) {
        if (cap) out[0] = 0;
        return;
    }
    char bx[32], bz[32];
    base36_i32(cx, bx, sizeof(bx));
    base36_i32(cz, bz, sizeof(bz));
    snprintf(out, cap, "%s\\pexcraft_modified_chunks\\c.%s.%s.pcd", world_dir, bx, bz);
}

static void load_modified_flat_chunk_delta_into_buffers_for_dir(const char *world_dir, int cx, int cz,
                                                                unsigned char *buf, unsigned char *meta) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    (void)world_dir; (void)cx; (void)cz; (void)buf; (void)meta; return;
#endif
    if (!world_dir || !world_dir[0] || !buf || !meta) return;
    char path[MAX_PATHBUF];
    flat_chunk_delta_path_current_for_dir(world_dir, cx, cz, path, sizeof(path), 0);
    FILE *f = fopen(path, "rb");
    if (!f) {
        flat_chunk_delta_path_legacy_for_dir(world_dir, cx, cz, path, sizeof(path));
        f = fopen(path, "rb");
    }
    if (!f) return;
    char magic[8];
    int rcx = 0, rcz = 0, count = 0;
    if (fread(magic, 1, 8, f) != 8 ||
        fread(&rcx, sizeof(rcx), 1, f) != 1 ||
        fread(&rcz, sizeof(rcz), 1, f) != 1 ||
        fread(&count, sizeof(count), 1, f) != 1 ||
        (memcmp(magic, "CHNKDLT1", 8) != 0 && memcmp(magic, "CHNKDLT2", 8) != 0 && memcmp(magic, "PXCDLT12", 8) != 0) ||
        rcx != cx || rcz != cz || count != FLAT_CHUNK_BLOCK_COUNT) {
        fclose(f);
        return;
    }
    int ok = fread(buf, 1, FLAT_CHUNK_BLOCK_COUNT, f) == FLAT_CHUNK_BLOCK_COUNT;
    int has_meta = (memcmp(magic, "CHNKDLT2", 8) == 0);
    if (ok && has_meta) ok = fread(meta, 1, FLAT_CHUNK_BLOCK_COUNT, f) == FLAT_CHUNK_BLOCK_COUNT;
    if (ok && !has_meta) memset(meta, 0, FLAT_CHUNK_BLOCK_COUNT);
    fclose(f);
}

static void copy_flat_chunk_buffers_to_world(int cx, int cz, const unsigned char *buf, const unsigned char *meta) {
    if (!buf) return;
    for (int lx = 0; lx < 16; lx++) {
        int wx = cx * 16 + lx;
        if (wx < g_flat_world_origin_x || wx >= g_flat_world_origin_x + FLAT_WORLD_SIZE) continue;
        int fx = flat_index(wx);
        for (int lz = 0; lz < 16; lz++) {
            int wz = cz * 16 + lz;
            if (wz < g_flat_world_origin_z || wz >= g_flat_world_origin_z + FLAT_WORLD_SIZE) continue;
            int fz = flat_z_index(wz);
            for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
                int bi = flat_chunk_buf_index(lx, y, lz);
                int id = buf[bi];
                unsigned char m = meta ? meta[bi] : 0;
                g_flat_blocks[flat_y_index(y)][fz][fx] = (unsigned char)id;
                g_flat_meta[flat_y_index(y)][fz][fx] = m;
                g_flat_levels[flat_y_index(y)][fz][fx] = block_is_liquid(id) ? (m & 15) : 0;
            }
        }
    }
    int lcx = stream_world_chunk_local_x(cx);
    int lcz = stream_world_chunk_local_z(cz);
    if (flat_local_chunk_valid(lcx, lcz)) flat_refresh_chunk_section_occupancy_local(lcx, lcz);
}

static void save_one_modified_flat_chunk(int lcx, int lcz) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    if (lcz >= 0 && lcz < FLAT_RENDER_CHUNKS && lcx >= 0 && lcx < FLAT_RENDER_CHUNKS) g_flat_world_chunk_modified[lcz][lcx] = 0;
    return;
#endif
    if (!g_loaded_world_dir[0]) return;
    if (lcx < 0 || lcx >= FLAT_RENDER_CHUNKS || lcz < 0 || lcz >= FLAT_RENDER_CHUNKS) return;
    if (!g_flat_world_chunk_modified[lcz][lcx]) return;

    int cx = floor_div16(g_flat_world_origin_x) + lcx;
    int cz = floor_div16(g_flat_world_origin_z) + lcz;
    unsigned char *base = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
    unsigned char *cur = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
    unsigned char *cur_meta = (unsigned char*)calloc(FLAT_CHUNK_BLOCK_COUNT, 1);
    if (!base || !cur || !cur_meta) { free(base); free(cur); free(cur_meta); return; }

    generate_flat_chunk_base_to_buffer(cx, cz, base);
    memset(cur, 0, FLAT_CHUNK_BLOCK_COUNT);
    for (int lx = 0; lx < 16; lx++) {
        int wx = cx * 16 + lx;
        if (wx < g_flat_world_origin_x || wx >= g_flat_world_origin_x + FLAT_WORLD_SIZE) continue;
        int fx = flat_index(wx);
        for (int lz = 0; lz < 16; lz++) {
            int wz = cz * 16 + lz;
            if (wz < g_flat_world_origin_z || wz >= g_flat_world_origin_z + FLAT_WORLD_SIZE) continue;
            int fz = flat_z_index(wz);
            for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
                int bi = flat_chunk_buf_index(lx, y, lz);
                cur[bi] = (unsigned char)g_flat_blocks[flat_y_index(y)][fz][fx];
                cur_meta[bi] = (unsigned char)g_flat_meta[flat_y_index(y)][fz][fx];
            }
        }
    }

    char path[MAX_PATHBUF];
    flat_chunk_delta_path(cx, cz, path, sizeof(path));
    int meta_nonzero = 0;
    for (int i = 0; i < FLAT_CHUNK_BLOCK_COUNT; i++) { if (cur_meta[i]) { meta_nonzero = 1; break; } }
    if (memcmp(base, cur, FLAT_CHUNK_BLOCK_COUNT) == 0 && !meta_nonzero) {
        DeleteFileA(path);
    } else {
        FILE *f = fopen(path, "wb");
        if (f) {
            char magic[8] = {'C','H','N','K','D','L','T','2'};
            int count = FLAT_CHUNK_BLOCK_COUNT;
            fwrite(magic, 1, 8, f);
            fwrite(&cx, sizeof(cx), 1, f);
            fwrite(&cz, sizeof(cz), 1, f);
            fwrite(&count, sizeof(count), 1, f);
            fwrite(cur, 1, FLAT_CHUNK_BLOCK_COUNT, f);
            fwrite(cur_meta, 1, FLAT_CHUNK_BLOCK_COUNT, f);
            fclose(f);
        }
    }
    g_flat_world_chunk_modified[lcz][lcx] = 0;
    free(base);
    free(cur);
    free(cur_meta);
}

static void save_modified_flat_chunks_sync(void) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    memset(g_flat_world_chunk_modified, 0, sizeof(g_flat_world_chunk_modified));
    return;
#endif
    if (!g_loaded_world_dir[0]) return;
    for (int lcz = 0; lcz < FLAT_RENDER_CHUNKS; lcz++) {
        for (int lcx = 0; lcx < FLAT_RENDER_CHUNKS; lcx++) {
            save_one_modified_flat_chunk(lcx, lcz);
        }
    }
}

static void save_modified_flat_chunks(void) {
    pex_request_modified_chunk_save_async();
}


static void flat_note_section_mesh_changed(int sy, int cz, int cx) {
    if (sy < 0 || sy >= FLAT_RENDER_SECTIONS_Y || cz < 0 || cz >= FLAT_RENDER_CHUNKS || cx < 0 || cx >= FLAT_RENDER_CHUNKS) return;
    g_flat_section_mesh_version[sy][cz][cx]++;
    if (g_flat_section_mesh_version[sy][cz][cx] == 0) g_flat_section_mesh_version[sy][cz][cx] = 1;
    g_flat_section_mesh_building[sy][cz][cx] = 0;
}

static void mark_flat_render_chunks_dirty_all(void) {
    for (int cz = 0; cz < FLAT_RENDER_CHUNKS; cz++) {
        for (int cx = 0; cx < FLAT_RENDER_CHUNKS; cx++) {
            if (g_flat_world_chunk_generated[cz][cx]) flat_refresh_chunk_section_occupancy_local(cx, cz);
            g_flat_world_chunk_dirty[cz][cx] = 1;
            g_flat_world_chunk_valid[cz][cx] = 0;
            g_flat_world_chunk_has_liquid[cz][cx] = 0;
            for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
                flat_mark_section_after_generation(cx, cz, sy);
            }
        }
    }
    g_flat_renderer_sort_dirty = 1;
    g_flat_world_geometry_dirty = 0;
    g_flat_section_geometry_dirty = 0;
}

static void mark_flat_render_chunks_dirty_around(int x, int z) {
    if (!flat_in_bounds(x, FLAT_WORLD_Y_MIN, z)) return;
    int fx = flat_index(x);
    int fz = flat_z_index(z);
    int cx0 = fx / FLAT_RENDER_CHUNK;
    int cz0 = fz / FLAT_RENDER_CHUNK;

    for (int dz = -1; dz <= 1; dz++) {
        for (int dx = -1; dx <= 1; dx++) {
            int cx = cx0 + dx;
            int cz = cz0 + dz;
            if (cx >= 0 && cx < FLAT_RENDER_CHUNKS && cz >= 0 && cz < FLAT_RENDER_CHUNKS) {
                g_flat_world_chunk_dirty[cz][cx] = 1;
                for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
                    g_flat_section_dirty[sy][cz][cx] = 1;
                    flat_note_section_mesh_changed(sy, cz, cx);
                }
            }
        }
    }
    g_flat_renderer_sort_dirty = 1;
}

static void flat_mark_section_dirty_keep_valid(int cx, int cz, int sy) {
    if (!flat_local_chunk_valid(cx, cz) || !flat_section_index_valid(sy)) return;
    g_flat_section_dirty[sy][cz][cx] = 1;
    flat_note_section_mesh_changed(sy, cz, cx);
    g_flat_world_chunk_dirty[cz][cx] = 1;
    g_flat_renderer_sort_dirty = 1;
}

static void mark_flat_render_sections_dirty_around_block(int x, int y, int z) {
    if (!flat_in_bounds(x, y, z)) return;
    int fx = flat_index(x);
    int fz = flat_z_index(z);
    int cx0 = fx / FLAT_RENDER_CHUNK;
    int cz0 = fz / FLAT_RENDER_CHUNK;
    int sy0 = (y - FLAT_WORLD_Y_MIN) / FLAT_RENDER_SECTION;

    /* Java 1.5.2 marks the 16^3 WorldRenderer containing the changed block
       and only crosses into adjacent renderers when the block touches their
       shared boundary.  The previous implementation dirtied the full 3x3x3
       neighborhood for every edit/liquid/falling-block update, causing 27
       sections to be rebuilt for many single-block changes. */
    flat_mark_section_dirty_keep_valid(cx0, cz0, sy0);
    if ((fx & (FLAT_RENDER_CHUNK - 1)) == 0) flat_mark_section_dirty_keep_valid(cx0 - 1, cz0, sy0);
    if ((fx & (FLAT_RENDER_CHUNK - 1)) == FLAT_RENDER_CHUNK - 1) flat_mark_section_dirty_keep_valid(cx0 + 1, cz0, sy0);
    if ((fz & (FLAT_RENDER_CHUNK - 1)) == 0) flat_mark_section_dirty_keep_valid(cx0, cz0 - 1, sy0);
    if ((fz & (FLAT_RENDER_CHUNK - 1)) == FLAT_RENDER_CHUNK - 1) flat_mark_section_dirty_keep_valid(cx0, cz0 + 1, sy0);
    if (((y - FLAT_WORLD_Y_MIN) & (FLAT_RENDER_SECTION - 1)) == 0) flat_mark_section_dirty_keep_valid(cx0, cz0, sy0 - 1);
    if (((y - FLAT_WORLD_Y_MIN) & (FLAT_RENDER_SECTION - 1)) == FLAT_RENDER_SECTION - 1) flat_mark_section_dirty_keep_valid(cx0, cz0, sy0 + 1);
}

static int flat_section_has_any_block_local(int lcx, int lcz, int sy) {
    if (!flat_local_chunk_valid(lcx, lcz) || !flat_section_index_valid(sy)) return 0;
    return (g_flat_chunk_section_non_empty_mask[lcz][lcx] & (unsigned short)(1u << sy)) != 0;
}

static void flat_mark_section_after_generation(int lcx, int lcz, int sy) {
    if (!flat_local_chunk_valid(lcx, lcz) || !flat_section_index_valid(sy)) return;
    if (flat_section_has_any_block_local(lcx, lcz, sy)) {
        g_flat_section_dirty[sy][lcz][lcx] = 1;
        flat_note_section_mesh_changed(sy, lcz, lcx);
        g_flat_section_valid[sy][lcz][lcx] = 0;
        g_flat_section_skip_pass[sy][lcz][lcx][0] = 1;
        g_flat_section_skip_pass[sy][lcz][lcx][1] = 1;
        g_flat_world_chunk_dirty[lcz][lcx] = 1;
        g_flat_world_chunk_valid[lcz][lcx] = 0;
    } else {
        /* Empty 16-high sections do not need a display-list/native-mesh rebuild.
           The section-existence mask makes this O(1), like Java's empty
           ExtendedBlockStorage sections, instead of a 4096-block scan. */
        g_flat_section_dirty[sy][lcz][lcx] = 0;
        g_flat_section_mesh_building[sy][lcz][lcx] = 0;
        g_flat_section_valid[sy][lcz][lcx] = 1;
        g_flat_section_skip_pass[sy][lcz][lcx][0] = 1;
        g_flat_section_skip_pass[sy][lcz][lcx][1] = 1;
    }
}

static void stream_mark_neighbor_boundary_dirty(int cx, int cz, int sy) {
    if (!flat_local_chunk_valid(cx, cz) || !flat_section_index_valid(sy)) return;
    if (!g_flat_world_chunk_generated[cz][cx]) return;
    if (!flat_section_has_any_block_local(cx, cz, sy)) return;
    flat_mark_section_dirty_keep_valid(cx, cz, sy);
}

static void stream_mark_local_chunk_generated(int lcx, int lcz) {
    if (!flat_local_chunk_valid(lcx, lcz)) return;
    g_flat_world_chunk_generated[lcz][lcx] = 1;

    /* Recompute one chunk's 16-bit occupancy mask once, then use O(1) tests.
       This replaces the old 3x3x16 section scan performed for every installed
       streamed chunk.  Only the new chunk gets invalidated; existing neighbor
       meshes stay drawable and are dirtied only on shared borders. */
    unsigned short mask = flat_refresh_chunk_section_occupancy_local(lcx, lcz);
    g_flat_world_chunk_dirty[lcz][lcx] = 1;
    g_flat_world_chunk_valid[lcz][lcx] = 0;
    g_flat_renderer_sort_dirty = 1;
    for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
        flat_mark_section_after_generation(lcx, lcz, sy);
        if (mask & (unsigned short)(1u << sy)) {
            stream_mark_neighbor_boundary_dirty(lcx - 1, lcz, sy);
            stream_mark_neighbor_boundary_dirty(lcx + 1, lcz, sy);
            stream_mark_neighbor_boundary_dirty(lcx, lcz - 1, sy);
            stream_mark_neighbor_boundary_dirty(lcx, lcz + 1, sy);
        }
    }
}


static int flat_block_is_opaque_cube_for_snow(int id) {
    if (id == 0 || id == BLOCK_WATER || id == BLOCK_STILL_WATER ||
        id == BLOCK_LAVA || id == BLOCK_STILL_LAVA) return 0;
    if (id == BLOCK_SNOW_LAYER || id == BLOCK_ICE || id == BLOCK_GLASS) return 0;
    if (id == BLOCK_SAPLING || id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE ||
        id == BLOCK_BROWN_MUSHROOM || id == BLOCK_RED_MUSHROOM ||
        id == BLOCK_TORCH || id == BLOCK_FIRE || id == BLOCK_REDSTONE_WIRE ||
        id == BLOCK_REDSTONE_TORCH_OFF || id == BLOCK_REDSTONE_TORCH_ON ||
        id == BLOCK_REEDS || id == BLOCK_LADDER || id == BLOCK_RAILS ||
        id == BLOCK_SIGN_POST || id == BLOCK_WALL_SIGN || id == BLOCK_LEVER ||
        id == BLOCK_STONE_BUTTON || id == BLOCK_WOOD_DOOR || id == BLOCK_IRON_DOOR ||
        id == BLOCK_PORTAL) return 0;
    return 1;
}

static int flat_snow_layer_can_stay_at(int x, int y, int z) {
    if (!flat_in_bounds(x, y, z) || y <= FLAT_WORLD_Y_MIN) return 0;
    return flat_block_is_opaque_cube_for_snow(flat_get_block(x, y - 1, z));
}

static void flat_set_block_raw(int x, int y, int z, int id) {
    if (!flat_in_bounds(x, y, z)) return;
    int yi = flat_y_index(y), zi = flat_z_index(z), xi = flat_index(x);
    unsigned char *cell = &g_flat_blocks[yi][zi][xi];
    /* Plain block writes clear both general metadata and liquid metadata.
       The fluid simulation uses flat_set_fluid() to update id + level together. */
    g_flat_levels[yi][zi][xi] = 0;
    if (*cell != id) {
        *cell = (unsigned char)id;
        g_flat_meta[yi][zi][xi] = 0;
        flat_update_section_occupancy_after_block_change(x, y, z, id);
        mark_flat_render_sections_dirty_around_block(x, y, z);
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_FAST_WORLD) && PEX_PSP_FAST_WORLD
        psp_fast_surface_mark_dirty_block(x, z);
        psp_fast_surface_note_edit_block(x, y, z);
#endif
        if (flat_persistent_edit_active()) {
            mark_flat_chunk_modified_at(x, z);
            g_save_dirty = 1;
        }
    }
    if (id == BLOCK_LAVA || id == BLOCK_STILL_LAVA) fluid_check_for_harden(x, y, z);
}

static void wake_falling_blocks_around(int x, int y, int z);

static void flat_set_fluid(int x, int y, int z, int id, int level) {
    if (!flat_in_bounds(x, y, z)) return;
    int yi = flat_y_index(y), zi = flat_z_index(z), xi = flat_index(x);
    unsigned char *cell = &g_flat_blocks[yi][zi][xi];
    unsigned char *lvl = &g_flat_levels[yi][zi][xi];
    unsigned char want_level = (unsigned char)(level & 15);
    if (*cell != (unsigned char)id || *lvl != want_level || g_flat_meta[yi][zi][xi] != want_level) {
        *cell = (unsigned char)id;
        *lvl = want_level;
        g_flat_meta[yi][zi][xi] = want_level;
        flat_update_section_occupancy_after_block_change(x, y, z, id);
        mark_flat_render_sections_dirty_around_block(x, y, z);
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_FAST_WORLD) && PEX_PSP_FAST_WORLD
        psp_fast_surface_mark_dirty_block(x, z);
        psp_fast_surface_note_edit_block(x, y, z);
#endif
        if (flat_persistent_edit_active()) {
            mark_flat_chunk_modified_at(x, z);
            g_save_dirty = 1;
        }
    }
    fluid_check_for_harden(x, y, z);
    wake_falling_blocks_around(x, y, z);
}

static void flat_place_fluid_source(int x, int y, int z, int id) {
    flat_set_fluid(x, y, z, id, 0);
    wake_neighbor_liquids(x, y, z);
}

static void flat_set_block(int x, int y, int z, int id) {
    if (!flat_in_bounds(x, y, z)) return;

    flat_set_block_raw(x, y, z, id);

    /* Beta snow is a dependent layer.  If the supporting block is removed or
       replaced by something snow cannot sit on, neighbor update deletes the
       layer above.  This is not a player harvest, so no snowball is dropped. */
    if (y < FLAT_WORLD_Y_MAX && flat_get_block(x, y + 1, z) == BLOCK_SNOW_LAYER &&
        !flat_snow_layer_can_stay_at(x, y + 1, z)) {
        flat_set_block_raw(x, y + 1, z, 0);
    }

    wake_neighbor_liquids(x, y, z);
    wake_falling_blocks_around(x, y, z);
}




static int floor_div16(int v) {
    return v >= 0 ? v / 16 : -((15 - v) / 16);
}

static int stream_effective_render_chunk_radius(void) {
    int chunks = g_opts.render_distance;
    if (chunks < 2) chunks = 2;
    int max_chunks = (FLAT_RENDER_CHUNKS / 2) - 2;
    if (max_chunks < 2) max_chunks = 2;
    if (chunks > max_chunks) chunks = max_chunks;
    return chunks;
}

static int stream_window_margin_blocks(void) {
    int margin = stream_effective_render_chunk_radius() * FLAT_RENDER_CHUNK + FLAT_RENDER_CHUNK;
    int max_margin = FLAT_WORLD_SIZE / 2 - FLAT_RENDER_CHUNK;
    if (max_margin < FLAT_RENDER_CHUNK * 3) max_margin = FLAT_RENDER_CHUNK * 3;
    if (margin > max_margin) margin = max_margin;
    if (margin < FLAT_RENDER_CHUNK * 3) margin = FLAT_RENDER_CHUNK * 3;
    return margin;
}

/* Streaming terrain generation used to rebuild the full Beta TerrainProvider for
   every newly-exposed chunk.  That made walking into new chunks hammer the CPU.
   Keep one provider per seed and reuse its octave/noise buffers instead. */
static TerrainProvider g_beta_stream_tp;
static int g_beta_stream_tp_valid = 0;
static long long g_beta_stream_tp_seed = 0;

static TerrainProvider *beta_stream_provider(void) {
    if (!g_beta_stream_tp_valid || g_beta_stream_tp_seed != g_world_seed) {
        if (g_beta_stream_tp_valid) terrain_provider_free(&g_beta_stream_tp);
        terrain_provider_init(&g_beta_stream_tp, (int64_t)g_world_seed);
        g_beta_stream_tp_seed = g_world_seed;
        g_beta_stream_tp_valid = 1;
    }
    return &g_beta_stream_tp;
}

static int beta_preview_copy_chunk_to_flat(int cx, int cz) {
    unsigned char *buf = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
    if (!buf) return 0;
    generate_flat_chunk_base_to_buffer(cx, cz, buf);
    copy_flat_chunk_buffer_to_world(cx, cz, buf);
    free(buf);
    /* Overlay only gameplay edits.  Unchanged chunks live only as seed + coords. */
    load_modified_flat_chunk_delta_into_flat(cx, cz);
    return 1;
}


static void beta_preview_generate_flat_world(void) {
    memset(g_flat_blocks, 0, sizeof(g_flat_blocks));
    memset(g_flat_meta, 0, sizeof(g_flat_meta));
    memset(g_flat_levels, 0, sizeof(g_flat_levels));
    memset(g_flat_chunk_section_non_empty_mask, 0, sizeof(g_flat_chunk_section_non_empty_mask));

    int min_cx = floor_div16(g_flat_world_origin_x);
    int max_cx = floor_div16(g_flat_world_origin_x + FLAT_WORLD_SIZE - 1);
    int min_cz = floor_div16(g_flat_world_origin_z);
    int max_cz = floor_div16(g_flat_world_origin_z + FLAT_WORLD_SIZE - 1);

    /* Fast startup path: build one shared Beta canvas for the whole active
       in-memory window plus a 1-chunk population border.  The earlier per-chunk
       path recreated the terrain provider and a 3x3 canvas for every visible
       chunk, which is why opening a world paused for seconds. */
    TerrainProvider tp;
    terrain_provider_init(&tp, (int64_t)g_world_seed);

    GenCanvas cv;
    cv.minCx = min_cx - 1;
    cv.minCz = min_cz - 1;
    int chunks_x = (max_cx - min_cx + 1) + 2;
    int chunks_z = (max_cz - min_cz + 1) + 2;
    cv.chunks = chunks_x; /* active window is square, so one dimension field is enough here */
    cv.blocks = NULL;

    if (chunks_x == chunks_z) {
        cv.blocks = (unsigned char*)calloc((size_t)cv.chunks * (size_t)cv.chunks * 32768u, 1);
    }

    if (cv.blocks) {
        for (int cz = cv.minCz; cz < cv.minCz + cv.chunks; cz++) {
            for (int cx = cv.minCx; cx < cv.minCx + cv.chunks; cx++) {
                generate_canvas_chunk(&tp, &cv, cx, cz);
            }
        }

        for (int cz = min_cz - 1; cz <= max_cz; cz++) {
            for (int cx = min_cx - 1; cx <= max_cx; cx++) {
                qm_populate_canvas(&tp, &cv, cx, cz);
            }
        }

        static unsigned char beta_blocks[32768];
        for (int cz = min_cz; cz <= max_cz; cz++) {
            for (int cx = min_cx; cx <= max_cx; cx++) {
                memset(beta_blocks, 0, sizeof(beta_blocks));
                extract_canvas_chunk(&cv, cx, cz, beta_blocks);
                for (int lx = 0; lx < 16; lx++) {
                    int wx = cx * 16 + lx;
                    if (wx < g_flat_world_origin_x || wx >= g_flat_world_origin_x + FLAT_WORLD_SIZE) continue;
                    int fx = flat_index(wx);
                    for (int lz = 0; lz < 16; lz++) {
                        int wz = cz * 16 + lz;
                        if (wz < g_flat_world_origin_z || wz >= g_flat_world_origin_z + FLAT_WORLD_SIZE) continue;
                        int fz = flat_z_index(wz);
                        for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
                            int id = (y < 128) ? get_block_local(beta_blocks, lx, y, lz) : 0;
                            g_flat_blocks[flat_y_index(y)][fz][fx] = (unsigned char)id;
                        }
                        if (g_flat_blocks[flat_y_index(0)][fz][fx] == 0) g_flat_blocks[flat_y_index(0)][fz][fx] = BLOCK_BEDROCK;
                    }
                }
                load_modified_flat_chunk_delta_into_flat(cx, cz);
                int lcx = stream_world_chunk_local_x(cx);
                int lcz = stream_world_chunk_local_z(cz);
                if (flat_local_chunk_valid(lcx, lcz)) flat_refresh_chunk_section_occupancy_local(lcx, lcz);
            }
        }
        free(cv.blocks);
    } else {
        /* Low-memory fallback keeps the old safe path. */
        for (int cz = min_cz; cz <= max_cz; cz++) {
            for (int cx = min_cx; cx <= max_cx; cx++) {
                beta_preview_copy_chunk_to_flat(cx, cz);
            }
        }
    }

    terrain_provider_free(&tp);
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

static void flat_world_center_origin_near(float px, float pz) {
    int pcx = floor_div16((int)floorf(px));
    int pcz = floor_div16((int)floorf(pz));
    int base_cx = pcx - (FLAT_RENDER_CHUNKS / 2);
    int base_cz = pcz - (FLAT_RENDER_CHUNKS / 2);
    g_flat_world_origin_x = base_cx * FLAT_RENDER_CHUNK;
    g_flat_world_origin_z = base_cz * FLAT_RENDER_CHUNK;
    g_stream_last_center_chunk_x = 999999;
    g_stream_last_center_chunk_z = 999999;
}

static void flat_world_prepare_initial_generation(void) {
    memset(g_flat_blocks, 0, sizeof(g_flat_blocks));
    memset(g_flat_meta, 0, sizeof(g_flat_meta));
    memset(g_flat_levels, 0, sizeof(g_flat_levels));
    memset(g_flat_world_chunk_modified, 0, sizeof(g_flat_world_chunk_modified));
    memset(g_flat_world_chunk_generated, 0, sizeof(g_flat_world_chunk_generated));
    memset(g_flat_chunk_section_non_empty_mask, 0, sizeof(g_flat_chunk_section_non_empty_mask));
    stream_generation_queue_clear();
    for (int cz = 0; cz < FLAT_RENDER_CHUNKS; cz++) {
        for (int cx = 0; cx < FLAT_RENDER_CHUNKS; cx++) {
            g_flat_world_chunk_dirty[cz][cx] = 1;
            g_flat_world_chunk_valid[cz][cx] = 0;
            g_flat_world_chunk_has_liquid[cz][cx] = 0;
            for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
                g_flat_section_dirty[sy][cz][cx] = 0;
                g_flat_section_mesh_version[sy][cz][cx] = 1;
                g_flat_section_mesh_building[sy][cz][cx] = 0;
                g_flat_section_valid[sy][cz][cx] = 1;
                g_flat_section_skip_pass[sy][cz][cx][0] = 1;
                g_flat_section_skip_pass[sy][cz][cx][1] = 1;
            }
        }
    }
    g_flat_world_geometry_dirty = 0;
    g_flat_section_geometry_dirty = 0;
}

static int stream_initial_spawn_chunk_radius(void) {
    /* Keep first-load fast.  Render distance 12+ used to force the loader to
       generate the entire 512x512 active window before entering the world.
       A 7x7 chunk spawn island is enough to stand/walk safely; the normal
       streaming worker fills the rest after gameplay starts. */
    int r = stream_effective_render_chunk_radius();
    if (r > 3) r = 3;
    if (r < 2) r = 2;
    return r;
}

static void flat_world_begin_initial_generation(void) {
    stream_generation_queue_clear();
    g_stream_generation_keep_completed = 1;

    int base_cx = floor_div16(g_flat_world_origin_x);
    int base_cz = floor_div16(g_flat_world_origin_z);
    int player_wcx = floor_div16((int)floorf(g_player_x));
    int player_wcz = floor_div16((int)floorf(g_player_z));
    int pcx = player_wcx - base_cx;
    int pcz = player_wcz - base_cz;
    if (pcx < 0) pcx = 0;
    if (pcz < 0) pcz = 0;
    if (pcx >= FLAT_RENDER_CHUNKS) pcx = FLAT_RENDER_CHUNKS - 1;
    if (pcz >= FLAT_RENDER_CHUNKS) pcz = FLAT_RENDER_CHUNKS - 1;

    int initial_radius = stream_initial_spawn_chunk_radius();
    for (int ring = 0; ring <= initial_radius; ring++) {
        for (int dz = -ring; dz <= ring; dz++) {
            for (int dx = -ring; dx <= ring; dx++) {
                if (ring != 0 && abs(dx) != ring && abs(dz) != ring) continue;
                int lcx = pcx + dx;
                int lcz = pcz + dz;
                if (lcx < 0 || lcx >= FLAT_RENDER_CHUNKS || lcz < 0 || lcz >= FLAT_RENDER_CHUNKS) continue;
                stream_queue_add_chunk(base_cx + lcx, base_cz + lcz);
            }
        }
    }
    process_stream_generation_queue();
}

static void flat_world_continue_initial_generation(void) {
    process_stream_generation_queue();
}

static int flat_world_initial_generation_active(void) {
    return stream_generation_active();
}

static int flat_world_initial_generation_total(void) {
    return g_stream_gen_queue_count;
}

static int flat_world_initial_generation_done(void) {
    return g_stream_gen_queue_installed_count;
}

static void flat_world_finish_initial_generation(void) {
    g_stream_generation_keep_completed = 0;
    stream_generation_queue_clear();
    g_flat_world_geometry_dirty = 0;
    g_flat_section_geometry_dirty = 0;
}

static void flat_world_generate_blocks_for_current_origin(void) {
    memset(g_flat_blocks, 0, sizeof(g_flat_blocks));
    memset(g_flat_meta, 0, sizeof(g_flat_meta));
    memset(g_flat_levels, 0, sizeof(g_flat_levels));
    memset(g_flat_world_chunk_modified, 0, sizeof(g_flat_world_chunk_modified));
    memset(g_flat_world_chunk_generated, 0, sizeof(g_flat_world_chunk_generated));
    memset(g_flat_chunk_section_non_empty_mask, 0, sizeof(g_flat_chunk_section_non_empty_mask));
    stream_generation_queue_clear();

    if (g_world_type == 1) {
        /* Exact same Beta 1.0 generator as before.  The difference is that the
           result is not saved unless gameplay edits the chunk. */
        beta_preview_generate_flat_world();
        for (int lcz = 0; lcz < FLAT_RENDER_CHUNKS; lcz++)
            for (int lcx = 0; lcx < FLAT_RENDER_CHUNKS; lcx++) {
                g_flat_world_chunk_generated[lcz][lcx] = 1;
                flat_refresh_chunk_section_occupancy_local(lcx, lcz);
            }
    } else {
        int min_cx = floor_div16(g_flat_world_origin_x);
        int max_cx = floor_div16(g_flat_world_origin_x + FLAT_WORLD_SIZE - 1);
        int min_cz = floor_div16(g_flat_world_origin_z);
        int max_cz = floor_div16(g_flat_world_origin_z + FLAT_WORLD_SIZE - 1);
        unsigned char *buf = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
        if (buf) {
            for (int cz = min_cz; cz <= max_cz; cz++) {
                for (int cx = min_cx; cx <= max_cx; cx++) {
                    generate_flat_chunk_base_to_buffer(cx, cz, buf);
                    copy_flat_chunk_buffer_to_world(cx, cz, buf);
                    load_modified_flat_chunk_delta_into_flat(cx, cz);
                    int lcx = stream_world_chunk_local_x(cx);
                    int lcz = stream_world_chunk_local_z(cz);
                    if (lcx >= 0 && lcx < FLAT_RENDER_CHUNKS && lcz >= 0 && lcz < FLAT_RENDER_CHUNKS) {
                        g_flat_world_chunk_generated[lcz][lcx] = 1;
                        flat_refresh_chunk_section_occupancy_local(lcx, lcz);
                    }
                }
            }
            free(buf);
        }
    }

    mark_flat_render_chunks_dirty_all();
}

static void flat_world_reset_blocks(void) {
    flat_world_center_origin_near(g_player_x, g_player_z);
    g_stream_gen_queue_count = 0;
    g_stream_gen_queue_index = 0;

    flat_world_generate_blocks_for_current_origin();

    memset(g_flat_levels, 0, sizeof(g_flat_levels));
    memset(g_drops, 0, sizeof(g_drops));
    memset(g_falling_blocks, 0, sizeof(g_falling_blocks));
    memset(g_button_timers, 0, sizeof(g_button_timers));
    memset(g_pressure_plate_timers, 0, sizeof(g_pressure_plate_timers));
    memset(g_dig_particles, 0, sizeof(g_dig_particles));
    g_next_dig_particle = 0;
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

static int fluid_flow_id(int is_water) {
    return is_water ? BLOCK_WATER : BLOCK_LAVA;
}

static int fluid_still_id(int is_water) {
    return is_water ? BLOCK_STILL_WATER : BLOCK_STILL_LAVA;
}

static int block_material_blocks_liquid(int id) {
    if (id == 0 || block_is_liquid(id)) return 0;
    if (id == BLOCK_WOOD_DOOR || id == BLOCK_IRON_DOOR || id == BLOCK_SIGN_POST ||
        id == BLOCK_WALL_SIGN || id == BLOCK_LADDER || id == BLOCK_REEDS) return 1;
    if (id == BLOCK_SAPLING || id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE ||
        id == BLOCK_BROWN_MUSHROOM || id == BLOCK_RED_MUSHROOM || id == BLOCK_TORCH ||
        id == BLOCK_FIRE || id == BLOCK_REDSTONE_WIRE || id == BLOCK_REDSTONE_TORCH_OFF ||
        id == BLOCK_REDSTONE_TORCH_ON || id == BLOCK_SNOW_LAYER || id == BLOCK_CROPS ||
        id == BLOCK_RAILS || id == BLOCK_LEVER || id == BLOCK_STONE_BUTTON ||
        id == BLOCK_STONE_PRESSURE_PLATE || id == BLOCK_WOOD_PRESSURE_PLATE) return 0;
    return 1;
}

static void fluid_wake_at(int x, int y, int z) {
    if (!flat_in_bounds(x, y, z)) return;
    int id = flat_get_block(x, y, z);
    if (id == BLOCK_STILL_WATER || id == BLOCK_STILL_LAVA) {
        int level = flat_get_level(x, y, z);
        int yi = flat_y_index(y), zi = flat_z_index(z), xi = flat_index(x);
        g_flat_blocks[yi][zi][xi] = (unsigned char)(id == BLOCK_STILL_WATER ? BLOCK_WATER : BLOCK_LAVA);
        g_flat_levels[yi][zi][xi] = (unsigned char)(level & 15);
        g_flat_meta[yi][zi][xi] = (unsigned char)(level & 15);
        mark_flat_render_sections_dirty_around_block(x, y, z);
        if (flat_persistent_edit_active()) {
            mark_flat_chunk_modified_at(x, z);
            g_save_dirty = 1;
        }
    }
}

static void wake_neighbor_liquids(int x, int y, int z) {
    fluid_wake_at(x - 1, y, z);
    fluid_wake_at(x + 1, y, z);
    fluid_wake_at(x, y, z - 1);
    fluid_wake_at(x, y, z + 1);
    fluid_wake_at(x, y - 1, z);
    fluid_wake_at(x, y + 1, z);
}

static void fluid_check_for_harden(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (!block_is_lava(id)) return;

    int touches_water =
        block_is_water(flat_get_block(x - 1, y, z)) ||
        block_is_water(flat_get_block(x + 1, y, z)) ||
        block_is_water(flat_get_block(x, y, z - 1)) ||
        block_is_water(flat_get_block(x, y, z + 1)) ||
        block_is_water(flat_get_block(x, y + 1, z));
    if (!touches_water) return;

    int level = flat_get_level(x, y, z);
    if (level == 0) flat_set_block_raw(x, y, z, BLOCK_OBSIDIAN);
    else if (level <= 4) flat_set_block_raw(x, y, z, BLOCK_COBBLESTONE);
}

static int flat_block_is_solid_for_collision(int id) {
    return id != 0 && !block_is_liquid(id) &&
           id != BLOCK_SAPLING && id != BLOCK_YELLOW_FLOWER && id != BLOCK_RED_ROSE &&
           id != BLOCK_BROWN_MUSHROOM && id != BLOCK_RED_MUSHROOM &&
           id != BLOCK_TORCH && id != BLOCK_FIRE && id != BLOCK_REDSTONE_WIRE &&
           id != BLOCK_REDSTONE_TORCH_OFF && id != BLOCK_REDSTONE_TORCH_ON &&
           id != BLOCK_REEDS && id != BLOCK_SNOW_LAYER && id != BLOCK_LADDER &&
           id != BLOCK_RAILS && id != BLOCK_LEVER && id != BLOCK_STONE_BUTTON &&
           id != BLOCK_STONE_PRESSURE_PLATE && id != BLOCK_WOOD_PRESSURE_PLATE &&
           id != BLOCK_SIGN_POST && id != BLOCK_WALL_SIGN && !block_is_door_id(id);
}

static int door_is_open_at(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (!block_is_door_id(id)) return 0;
    int ly = door_lower_y_at(x, y, z);
    return door_meta_is_open(flat_get_meta(x, ly, z));
}

static int aabb_intersects_box(float minx, float maxx, float miny, float maxy, float minz, float maxz,
                                float x0, float y0, float z0, float x1, float y1, float z1) {
    return maxx > x0 && minx < x1 && maxy > y0 && miny < y1 && maxz > z0 && minz < z1;
}

static int door_thin_aabb_intersects(float minx, float maxx, float miny, float maxy, float minz, float maxz, int x, int y, int z) {
    if (door_is_open_at(x, y, z)) return 0;
    int ly = door_lower_y_at(x, y, z);
    int dir = flat_get_meta(x, ly, z) & 3;
    float t = 3.0f / 16.0f;
    float x0 = (float)x, x1 = (float)(x + 1);
    float z0 = (float)z, z1 = (float)(z + 1);
    if (dir == 0) z1 = z0 + t;          /* north */
    else if (dir == 2) z0 = z1 - t;     /* south */
    else if (dir == 1) x0 = x1 - t;     /* east */
    else x1 = x0 + t;                   /* west */
    return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x0, (float)y, z0, x1, (float)(y + 1), z1);
}

static int ladder_thin_aabb_intersects(float minx, float maxx, float miny, float maxy, float minz, float maxz,
                                       int x, int y, int z, float inflate) {
    int meta = flat_get_meta(x, y, z);
    if (meta < 2 || meta > 5) meta = 3;
    float t = 2.0f / 16.0f;
    float x0 = (float)x, x1 = (float)(x + 1);
    float z0 = (float)z, z1 = (float)(z + 1);
    if (meta == 2) z0 = z1 - t;
    else if (meta == 3) z1 = z0 + t;
    else if (meta == 4) x0 = x1 - t;
    else if (meta == 5) x1 = x0 + t;
    x0 -= inflate; x1 += inflate; z0 -= inflate; z1 += inflate;
    return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x0, (float)y, z0, x1, (float)(y + 1), z1);
}

static int block_custom_collision_intersects(int id, float minx, float maxx, float miny, float maxy, float minz, float maxz, int x, int y, int z) {
    if (id == BLOCK_LADDER) return ladder_thin_aabb_intersects(minx, maxx, miny, maxy, minz, maxz, x, y, z, 0.0f);
    if (id == BLOCK_SLAB) return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x, y, z, x + 1.0f, y + 0.5f, z + 1.0f);
    if (id == BLOCK_CHEST) return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x + 1.0f/16.0f, y, z + 1.0f/16.0f, x + 15.0f/16.0f, y + 14.0f/16.0f, z + 15.0f/16.0f);
    if (id == BLOCK_CACTUS) return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x + 1.0f/16.0f, y, z + 1.0f/16.0f, x + 15.0f/16.0f, y + 1.0f, z + 15.0f/16.0f);
    if (id == BLOCK_WOOD_STAIRS || id == BLOCK_COBBLE_STAIRS) {
        if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x, y, z, x + 1.0f, y + 0.5f, z + 1.0f)) return 1;
        int dir = flat_get_meta(x, y, z) & 3;
        /* Beta BlockStairs metadata collision: 0=east half tall,
           1=west half tall, 2=south half tall, 3=north half tall. */
        if (dir == 0) return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x + 0.5f, y + 0.5f, z, x + 1.0f, y + 1.0f, z + 1.0f);
        if (dir == 1) return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x, y + 0.5f, z, x + 0.5f, y + 1.0f, z + 1.0f);
        if (dir == 2) return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x, y + 0.5f, z + 0.5f, x + 1.0f, y + 1.0f, z + 1.0f);
        return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x, y + 0.5f, z, x + 1.0f, y + 1.0f, z + 0.5f);
    }
    if (id == BLOCK_FENCE) {
        if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x + 0.375f, y, z + 0.375f, x + 0.625f, y + 1.5f, z + 0.625f)) return 1;
        if (flat_block_is_solid_for_collision(flat_get_block(x - 1, y, z)) || flat_get_block(x - 1, y, z) == BLOCK_FENCE)
            if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x, y, z + 0.375f, x + 0.5f, y + 1.5f, z + 0.625f)) return 1;
        if (flat_block_is_solid_for_collision(flat_get_block(x + 1, y, z)) || flat_get_block(x + 1, y, z) == BLOCK_FENCE)
            if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x + 0.5f, y, z + 0.375f, x + 1.0f, y + 1.5f, z + 0.625f)) return 1;
        if (flat_block_is_solid_for_collision(flat_get_block(x, y, z - 1)) || flat_get_block(x, y, z - 1) == BLOCK_FENCE)
            if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x + 0.375f, y, z, x + 0.625f, y + 1.5f, z + 0.5f)) return 1;
        if (flat_block_is_solid_for_collision(flat_get_block(x, y, z + 1)) || flat_get_block(x, y, z + 1) == BLOCK_FENCE)
            if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x + 0.375f, y, z + 0.5f, x + 0.625f, y + 1.5f, z + 1.0f)) return 1;
        return 0;
    }
    return 0;
}

static int block_has_custom_collision(int id) {
    return id == BLOCK_LADDER || id == BLOCK_SLAB || id == BLOCK_CHEST || id == BLOCK_CACTUS ||
           id == BLOCK_WOOD_STAIRS || id == BLOCK_COBBLE_STAIRS || id == BLOCK_FENCE;
}

static int flat_player_in_ladder(void) {
    float feet = g_player_y - 1.62f;
    float minx = g_player_x - 0.31f, maxx = g_player_x + 0.31f;
    float miny = feet + 0.05f, maxy = feet + 1.75f;
    float minz = g_player_z - 0.31f, maxz = g_player_z + 0.31f;
    int x0 = (int)floorf(minx), x1 = (int)floorf(maxx);
    int y0 = (int)floorf(miny), y1 = (int)floorf(maxy);
    int z0 = (int)floorf(minz), z1 = (int)floorf(maxz);
    for (int y = y0; y <= y1; y++) for (int z = z0; z <= z1; z++) for (int x = x0; x <= x1; x++) {
        if (flat_get_block(x, y, z) == BLOCK_LADDER && ladder_thin_aabb_intersects(minx, maxx, miny, maxy, minz, maxz, x, y, z, 0.06f)) return 1;
    }
    return 0;
}

static int flat_solid_for_spawn(int id) {
    return flat_block_is_solid_for_collision(id);
}

static int block_drop_id(int block_id) {
    /* ph.java grass drops dirt via of.v.a(0, random). */
    if (block_id == BLOCK_GRASS) return BLOCK_DIRT;
    if (block_id == BLOCK_SNOW_LAYER) return ITEM_SNOWBALL;
    if (block_id == BLOCK_LEAVES) return 0;
    if (block_is_liquid(block_id)) return 0;
    if (block_id == BLOCK_STONE) return BLOCK_COBBLESTONE;
    if (block_id == BLOCK_COAL_ORE) return ITEM_COAL;
    if (block_id == BLOCK_DIAMOND_ORE) return ITEM_DIAMOND;
    if (block_id == BLOCK_REDSTONE_ORE || block_id == BLOCK_REDSTONE_ORE_GLOWING) return ITEM_REDSTONE;
    if (block_id == BLOCK_CLAY) return ITEM_CLAY;
    if (block_id == BLOCK_GLOWSTONE) return ITEM_GLOWSTONE_DUST;
    if (block_id == BLOCK_REEDS) return ITEM_REED;
    if (block_id == BLOCK_SIGN_POST || block_id == BLOCK_WALL_SIGN) return ITEM_SIGN;
    if (block_id == BLOCK_FURNACE_LIT) return BLOCK_FURNACE;
    if (block_id == BLOCK_BEDROCK) return 0;
    return block_id;
}

static float block_hardness(int block_id) {
    /* Values mirror the deobfuscated Beta client Block.java setHardness(...). */
    switch (block_id) {
        case BLOCK_STONE: return 1.5f;
        case BLOCK_GRASS: return 0.6f;
        case BLOCK_DIRT: return 0.5f;
        case BLOCK_COBBLESTONE: return 2.0f;
        case BLOCK_PLANKS: return 2.0f;
        case BLOCK_SAPLING: return 0.0f;
        case BLOCK_BEDROCK: return -1.0f;
        case BLOCK_WATER: case BLOCK_STILL_WATER: return 100.0f;
        case BLOCK_LAVA: return 0.0f;
        case BLOCK_STILL_LAVA: return 100.0f;
        case BLOCK_SAND: return 0.5f;
        case BLOCK_GRAVEL: return 0.6f;
        case BLOCK_GOLD_ORE: case BLOCK_IRON_ORE: case BLOCK_COAL_ORE: return 3.0f;
        case BLOCK_LOG: return 2.0f;
        case BLOCK_LEAVES: return 0.2f;
        case BLOCK_SPONGE: return 0.6f;
        case BLOCK_GLASS: return 0.3f;
        case BLOCK_LAPIS_ORE: return 3.0f;
        case BLOCK_LAPIS_BLOCK: return 3.0f;
        case BLOCK_WOOL: return 0.8f;
        case BLOCK_YELLOW_FLOWER: case BLOCK_RED_ROSE:
        case BLOCK_BROWN_MUSHROOM: case BLOCK_RED_MUSHROOM: return 0.0f;
        case BLOCK_GOLD_BLOCK: return 3.0f;
        case BLOCK_IRON_BLOCK: return 5.0f;
        case BLOCK_DOUBLE_SLAB: case BLOCK_SLAB: return 2.0f;
        case BLOCK_BRICK: return 2.0f;
        case BLOCK_TNT: return 0.0f;
        case BLOCK_BOOKSHELF: return 1.5f;
        case BLOCK_MOSSY_COBBLESTONE: return 2.0f;
        case BLOCK_OBSIDIAN: return 10.0f;
        case BLOCK_TORCH: case BLOCK_FIRE: return 0.0f;
        case BLOCK_MOB_SPAWNER: return 5.0f;
        case BLOCK_WOOD_STAIRS: return 2.0f;       /* BlockStairs copies planks hardness. */
        case BLOCK_CHEST: return 2.5f;
        case BLOCK_REDSTONE_WIRE: return 0.0f;
        case BLOCK_DIAMOND_ORE: return 3.0f;
        case BLOCK_DIAMOND_BLOCK: return 5.0f;
        case BLOCK_CRAFTING_TABLE: return 2.5f;
        case BLOCK_CROPS: return 0.0f;
        case BLOCK_FARMLAND: return 0.6f;
        case BLOCK_FURNACE: case BLOCK_FURNACE_LIT: return 3.5f;
        case BLOCK_SIGN_POST: case BLOCK_WALL_SIGN: return 1.0f;
        case BLOCK_WOOD_DOOR: return 3.0f;
        case BLOCK_LADDER: return 0.4f;
        case BLOCK_RAILS: return 0.7f;
        case BLOCK_COBBLE_STAIRS: return 2.0f;     /* BlockStairs copies cobblestone hardness. */
        case BLOCK_LEVER: return 0.5f;
        case BLOCK_STONE_PRESSURE_PLATE: return 0.5f;
        case BLOCK_IRON_DOOR: return 5.0f;
        case BLOCK_WOOD_PRESSURE_PLATE: return 0.5f;
        case BLOCK_REDSTONE_ORE: case BLOCK_REDSTONE_ORE_GLOWING: return 3.0f;
        case BLOCK_REDSTONE_TORCH_OFF: case BLOCK_REDSTONE_TORCH_ON: return 0.0f;
        case BLOCK_STONE_BUTTON: return 0.5f;
        case BLOCK_SNOW_LAYER: return 0.1f;
        case BLOCK_ICE: return 0.5f;
        case BLOCK_SNOW_BLOCK: return 0.2f;
        case BLOCK_CACTUS: return 0.4f;
        case BLOCK_CLAY: return 0.6f;
        case BLOCK_REEDS: return 0.0f;
        case BLOCK_JUKEBOX: return 2.0f;
        case BLOCK_FENCE: return 2.0f;
        case BLOCK_PUMPKIN: return 1.0f;
        case BLOCK_NETHERRACK: return 0.4f;
        case BLOCK_SOUL_SAND: return 0.5f;
        case BLOCK_GLOWSTONE: return 0.3f;
        case BLOCK_PORTAL: return -1.0f;
        case BLOCK_JACK_O_LANTERN: return 1.0f;
        default: return 1.0f;
    }
}

static int held_tool_quality(int held_id) {
    /* Java Item.java constructor quality values: wood=0, stone=1, iron=2,
       diamond=3, and gold also uses quality 0 in this Beta source. */
    switch (held_id) {
        case ITEM_WOODEN_PICKAXE: case ITEM_WOODEN_SHOVEL: case ITEM_WOODEN_AXE:
        case ITEM_PICKAXE_GOLD: case ITEM_SHOVEL_GOLD: case ITEM_AXE_GOLD:
            return 0;
        case ITEM_STONE_PICKAXE: case ITEM_STONE_SHOVEL: case ITEM_STONE_AXE:
            return 1;
        case ITEM_PICKAXE_IRON: case ITEM_SHOVEL_IRON: case ITEM_AXE_IRON:
            return 2;
        case ITEM_PICKAXE_DIAMOND: case ITEM_SHOVEL_DIAMOND: case ITEM_AXE_DIAMOND:
            return 3;
        default:
            return -1;
    }
}

static int held_is_pickaxe(int held_id) {
    return held_id == ITEM_WOODEN_PICKAXE || held_id == ITEM_STONE_PICKAXE ||
           held_id == ITEM_PICKAXE_IRON || held_id == ITEM_PICKAXE_DIAMOND ||
           held_id == ITEM_PICKAXE_GOLD;
}

static int held_is_shovel(int held_id) {
    return held_id == ITEM_WOODEN_SHOVEL || held_id == ITEM_STONE_SHOVEL ||
           held_id == ITEM_SHOVEL_IRON || held_id == ITEM_SHOVEL_DIAMOND ||
           held_id == ITEM_SHOVEL_GOLD;
}

static int held_is_axe(int held_id) {
    return held_id == ITEM_WOODEN_AXE || held_id == ITEM_STONE_AXE ||
           held_id == ITEM_AXE_IRON || held_id == ITEM_AXE_DIAMOND ||
           held_id == ITEM_AXE_GOLD;
}

static int held_is_sword(int held_id) {
    return held_id == ITEM_WOODEN_SWORD || held_id == ITEM_STONE_SWORD ||
           held_id == ITEM_SWORD_IRON || held_id == ITEM_SWORD_DIAMOND ||
           held_id == ITEM_SWORD_GOLD;
}

static int block_is_pickaxe_fast_block(int block_id) {
    /* Pickaxes are fast on rock/metal/ore blocks and the small rock-derived
       utility blocks.  This keeps wooden stairs/plates/buttons on the axe path
       and stone buttons/plates/cobble stairs on the pickaxe path instead of
       letting swords beat the correct tool. */
    switch (block_id) {
        case BLOCK_STONE:
        case BLOCK_COBBLESTONE:
        case BLOCK_DOUBLE_SLAB:
        case BLOCK_SLAB:
        case BLOCK_BRICK:
        case BLOCK_MOSSY_COBBLESTONE:
        case BLOCK_OBSIDIAN:
        case BLOCK_MOB_SPAWNER:
        case BLOCK_FURNACE:
        case BLOCK_FURNACE_LIT:
        case BLOCK_COBBLE_STAIRS:
        case BLOCK_STONE_PRESSURE_PLATE:
        case BLOCK_STONE_BUTTON:
        case BLOCK_IRON_DOOR:
        case BLOCK_IRON_ORE:
        case BLOCK_IRON_BLOCK:
        case BLOCK_COAL_ORE:
        case BLOCK_GOLD_BLOCK:
        case BLOCK_GOLD_ORE:
        case BLOCK_DIAMOND_ORE:
        case BLOCK_DIAMOND_BLOCK:
        case BLOCK_REDSTONE_ORE:
        case BLOCK_REDSTONE_ORE_GLOWING:
        case BLOCK_ICE:
        case BLOCK_NETHERRACK:
        case BLOCK_LAPIS_ORE:
        case BLOCK_LAPIS_BLOCK:
            return 1;
        default:
            return 0;
    }
}

static int block_is_shovel_fast_block(int block_id) {
    return block_id == BLOCK_GRASS || block_id == BLOCK_DIRT || block_id == BLOCK_SAND ||
           block_id == BLOCK_GRAVEL || block_id == BLOCK_SNOW_LAYER ||
           block_id == BLOCK_SNOW_BLOCK || block_id == BLOCK_CLAY;
}

static int block_is_axe_fast_block(int block_id) {
    /* The old table only included the four Java Beta ItemAxe entries, which
       made later wooden block-forms like wood stairs/plates/fences slower than
       a sword.  Treat every wooden block currently implemented by this client
       as axe-effective. */
    switch (block_id) {
        case BLOCK_PLANKS:
        case BLOCK_BOOKSHELF:
        case BLOCK_LOG:
        case BLOCK_CHEST:
        case BLOCK_CRAFTING_TABLE:
        case BLOCK_WOOD_STAIRS:
        case BLOCK_WOOD_DOOR:
        case BLOCK_LADDER:
        case BLOCK_SIGN_POST:
        case BLOCK_WALL_SIGN:
        case BLOCK_WOOD_PRESSURE_PLATE:
        case BLOCK_FENCE:
        case BLOCK_JUKEBOX:
            return 1;
        default:
            return 0;
    }
}

static int block_requires_tool_to_harvest(int block_id) {
    /* InventoryPlayer.canHarvestBlock only requires a tool for rock, iron,
       builtSnow, and snow materials.  Everything else can drop by hand. */
    switch (block_id) {
        case BLOCK_STONE:
        case BLOCK_COBBLESTONE:
        case BLOCK_GOLD_ORE:
        case BLOCK_IRON_ORE:
        case BLOCK_COAL_ORE:
        case BLOCK_GOLD_BLOCK:
        case BLOCK_IRON_BLOCK:
        case BLOCK_DOUBLE_SLAB:
        case BLOCK_SLAB:
        case BLOCK_BRICK:
        case BLOCK_MOSSY_COBBLESTONE:
        case BLOCK_OBSIDIAN:
        case BLOCK_MOB_SPAWNER:
        case BLOCK_DIAMOND_ORE:
        case BLOCK_DIAMOND_BLOCK:
        case BLOCK_FURNACE:
        case BLOCK_FURNACE_LIT:
        case BLOCK_COBBLE_STAIRS:
        case BLOCK_IRON_DOOR:
        case BLOCK_REDSTONE_ORE:
        case BLOCK_REDSTONE_ORE_GLOWING:
        case BLOCK_SNOW_LAYER:
        case BLOCK_SNOW_BLOCK:
        case BLOCK_NETHERRACK:
        case BLOCK_LAPIS_ORE:
        case BLOCK_LAPIS_BLOCK:
            return 1;
        default:
            return 0;
    }
}

static int held_pickaxe_can_harvest(int held_id, int block_id) {
    int q = held_tool_quality(held_id);
    if (!held_is_pickaxe(held_id)) return 0;
    if (block_id == BLOCK_OBSIDIAN) return q == 3;
    if (block_id == BLOCK_DIAMOND_BLOCK || block_id == BLOCK_DIAMOND_ORE) return q >= 2;
    if (block_id == BLOCK_GOLD_BLOCK || block_id == BLOCK_GOLD_ORE) return q >= 2;
    if (block_id == BLOCK_IRON_BLOCK || block_id == BLOCK_IRON_ORE) return q >= 1;
    if (block_id == BLOCK_REDSTONE_ORE || block_id == BLOCK_REDSTONE_ORE_GLOWING) return q >= 2;
    if (block_id == BLOCK_LAPIS_ORE || block_id == BLOCK_LAPIS_BLOCK) return q >= 1;
    return block_requires_tool_to_harvest(block_id) &&
           block_id != BLOCK_SNOW_LAYER && block_id != BLOCK_SNOW_BLOCK;
}

static int held_shovel_can_harvest(int held_id, int block_id) {
    if (!held_is_shovel(held_id)) return 0;
    return block_id == BLOCK_SNOW_LAYER || block_id == BLOCK_SNOW_BLOCK;
}

static int held_item_can_harvest_block_id(int held_id, int block_id) {
    if (!block_requires_tool_to_harvest(block_id)) return 1;
    if (held_pickaxe_can_harvest(held_id, block_id)) return 1;
    if (held_shovel_can_harvest(held_id, block_id)) return 1;
    return 0;
}

static float held_item_strength_vs_block(int held_id, int block_id) {
    int q = held_tool_quality(held_id);
    if (held_is_pickaxe(held_id) && block_is_pickaxe_fast_block(block_id)) return (float)((q + 1) * 2);
    if (held_is_shovel(held_id) && block_is_shovel_fast_block(block_id)) return (float)((q + 1) * 2);
    if (held_is_axe(held_id) && block_is_axe_fast_block(block_id)) return (float)((q + 1) * 2);
    if (held_is_sword(held_id)) return 1.5f;
    return 1.0f;
}

static float block_relative_strength(int block_id) {
    float h = block_hardness(block_id);
    if (h < 0.0f) return 0.0f;
    if (h <= 0.0f) return 1.0f;

    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    int held_id = stack_empty(held) ? 0 : held->id;

    if (!held_item_can_harvest_block_id(held_id, block_id)) {
        return 1.0f / h / 100.0f;
    }
    return held_item_strength_vs_block(held_id, block_id) / h / 30.0f;
}

static void restart_hand_swing(void) {
    /* A new click/use action should visibly restart the first-person arm
       animation even if the previous swing has not finished yet. */
    g_air_swing_playing = 0;
    g_air_swing_ticks = 0;
    g_hand_swing_active = 1;
    g_hand_swing_ticks = 0;
    g_hand_swing_progress = 0.0f;
    g_prev_hand_swing = 0.0f;
    g_hand_swing = 0.0f;
}

static void trigger_hand_swing(void) {
    /* Holding mine must not reset every tick, otherwise the hand freezes at
       the start of the swing.  For discrete re-click/place actions use
       restart_hand_swing(). */
    if (!g_hand_swing_active && g_hand_swing_ticks == 0) {
        restart_hand_swing();
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
    for (int y = y0 - 1; y <= y1; y++) {
        for (int z = z0; z <= z1; z++) {
            for (int x = x0; x <= x1; x++) {
                int bid = flat_get_block(x, y, z);
                if (block_is_door_id(bid)) {
                    if (door_thin_aabb_intersects(minx, maxx, miny, maxy, minz, maxz, x, y, z)) return 1;
                } else if (block_has_custom_collision(bid)) {
                    if (block_custom_collision_intersects(bid, minx, maxx, miny, maxy, minz, maxz, x, y, z)) return 1;
                } else if (flat_block_is_solid_for_collision(bid)) {
                    if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz,
                                            (float)x, (float)y, (float)z,
                                            (float)(x + 1), (float)(y + 1), (float)(z + 1))) return 1;
                }
            }
        }
    }
    return 0;
}


static int flat_player_has_water_exit_ledge(float px, float py, float pz, float dx, float dz) {
    /* Only allow the strong water-exit jump when the player is actually
       pushing into a nearby bank/ledge.  Open water should keep normal slow
       swimming, not launch the player upward forever. */
    float len = sqrtf(dx * dx + dz * dz);
    if (len < 0.001f) return 0;
    dx /= len;
    dz /= len;

    /* Too deep underwater: swim upward normally first, then mantle only when
       the head is near the surface. */
    int bx = (int)floorf(px);
    int by = (int)floorf(py);
    int bz = (int)floorf(pz);
    if (block_is_water(flat_get_block(bx, by + 1, bz))) return 0;

    /* Check a few distances in the intended movement direction.  A real exit
       ledge blocks the current body, but has clearance slightly above it. */
    for (int i = 0; i < 4; i++) {
        float dist = 0.35f + (float)i * 0.15f;
        float tx = px + dx * dist;
        float tz = pz + dz * dist;
        if (!flat_player_aabb_collides(tx, py, tz)) continue;
        if (!flat_player_aabb_collides(tx, py + 0.62f, tz)) return 1;
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
    for (int y = y0 - 1; y <= y1; y++) {
        for (int z = z0; z <= z1; z++) {
            for (int x = x0; x <= x1; x++) {
                int bid = flat_get_block(x, y, z);
                if (block_is_door_id(bid)) {
                    if (door_thin_aabb_intersects(minx, maxx, miny, maxy, minz, maxz, x, y, z)) return 1;
                } else if (block_has_custom_collision(bid)) {
                    if (block_custom_collision_intersects(bid, minx, maxx, miny, maxy, minz, maxz, x, y, z)) return 1;
                } else if (flat_block_is_solid_for_collision(bid)) {
                    if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz,
                                            (float)x, (float)y, (float)z,
                                            (float)(x + 1), (float)(y + 1), (float)(z + 1))) return 1;
                }
            }
        }
    }
    return 0;
}

static void inventory_reset(void) {
    memset(g_inventory, 0, sizeof(g_inventory));
    memset(g_craft_grid, 0, sizeof(g_craft_grid));
    memset(g_workbench_grid, 0, sizeof(g_workbench_grid));
    memset(g_chest_slots, 0, sizeof(g_chest_slots));
    g_legacy_global_chest_pending = 0;
    chest_clear_all_tiles();
    furnace_clear_all_tiles();
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


static int chest_find_tile(int x, int y, int z) {
    for (int i = 0; i < MAX_CHEST_TILES; i++) {
        if (g_chest_tiles[i].active && g_chest_tiles[i].x == x && g_chest_tiles[i].y == y && g_chest_tiles[i].z == z) return i;
    }
    return -1;
}

static int chest_slots_have_items(const ItemStack *slots, int n) {
    for (int i = 0; i < n; i++) if (!stack_empty(&slots[i])) return 1;
    return 0;
}

static int chest_alloc_tile(int x, int y, int z) {
    int existing = chest_find_tile(x, y, z);
    if (existing >= 0) return existing;

    int free_i = -1;
    for (int i = 0; i < MAX_CHEST_TILES; i++) {
        if (!g_chest_tiles[i].active) { free_i = i; break; }
    }
    if (free_i < 0) return -1;

    ChestTile *ct = &g_chest_tiles[free_i];
    memset(ct, 0, sizeof(*ct));
    ct->active = 1;
    ct->x = x; ct->y = y; ct->z = z;

    /* One-time rescue path for worlds saved by the old broken global chest
       implementation.  It had no coordinates, so the first real chest opened
       after upgrading receives that old inventory instead of silently losing it. */
    if (g_legacy_global_chest_pending && !chest_slots_have_items(ct->slots, 27)) {
        memcpy(ct->slots, g_chest_slots, sizeof(ct->slots));
        memset(g_chest_slots, 0, sizeof(g_chest_slots));
        g_legacy_global_chest_pending = 0;
        g_save_dirty = 1;
    }
    return free_i;
}

static void chest_clear_all_tiles(void) {
    memset(g_chest_tiles, 0, sizeof(g_chest_tiles));
    g_open_chest_indices[0] = g_open_chest_indices[1] = -1;
    g_open_chest_count = 0;
    g_open_chest_rows = 3;
    snprintf(g_open_chest_title, sizeof(g_open_chest_title), "Chest");
}

static void chest_close_open_inventory(void) {
    g_open_chest_indices[0] = g_open_chest_indices[1] = -1;
    g_open_chest_count = 0;
    g_open_chest_rows = 3;
    snprintf(g_open_chest_title, sizeof(g_open_chest_title), "Chest");
}

static void chest_remove_tile_index(int idx) {
    if (idx < 0 || idx >= MAX_CHEST_TILES) return;
    memset(&g_chest_tiles[idx], 0, sizeof(g_chest_tiles[idx]));
    for (int i = 0; i < 2; i++) {
        if (g_open_chest_indices[i] == idx) chest_close_open_inventory();
    }
    g_save_dirty = 1;
}

static int flat_block_is_opaque_cube_for_chest(int id) {
    return flat_block_is_opaque_cube_for_snow(id);
}

static int chest_neighbor_count_at(int x, int y, int z) {
    int n = 0;
    if (flat_get_block(x - 1, y, z) == BLOCK_CHEST) n++;
    if (flat_get_block(x + 1, y, z) == BLOCK_CHEST) n++;
    if (flat_get_block(x, y, z - 1) == BLOCK_CHEST) n++;
    if (flat_get_block(x, y, z + 1) == BLOCK_CHEST) n++;
    return n;
}

static int chest_has_neighbor_chest(int x, int y, int z) {
    if (flat_get_block(x, y, z) != BLOCK_CHEST) return 0;
    return chest_neighbor_count_at(x, y, z) > 0;
}

static int chest_can_place_at(int x, int y, int z) {
    int neighbors = chest_neighbor_count_at(x, y, z);
    if (neighbors > 1) return 0;
    if (chest_has_neighbor_chest(x - 1, y, z)) return 0;
    if (chest_has_neighbor_chest(x + 1, y, z)) return 0;
    if (chest_has_neighbor_chest(x, y, z - 1)) return 0;
    if (chest_has_neighbor_chest(x, y, z + 1)) return 0;
    return 1;
}

static void chest_on_block_placed(int x, int y, int z) {
    if (flat_get_block(x, y, z) == BLOCK_CHEST) {
        chest_alloc_tile(x, y, z);
        g_save_dirty = 1;
    }
}

static int chest_blocked_above(int x, int y, int z) {
    if (y >= FLAT_WORLD_Y_MAX) return 0;
    return flat_block_is_opaque_cube_for_chest(flat_get_block(x, y + 1, z));
}

static int chest_open_at(int x, int y, int z) {
    if (flat_get_block(x, y, z) != BLOCK_CHEST) return 0;
    if (chest_blocked_above(x, y, z)) return 1;
    if (flat_get_block(x - 1, y, z) == BLOCK_CHEST && chest_blocked_above(x - 1, y, z)) return 1;
    if (flat_get_block(x + 1, y, z) == BLOCK_CHEST && chest_blocked_above(x + 1, y, z)) return 1;
    if (flat_get_block(x, y, z - 1) == BLOCK_CHEST && chest_blocked_above(x, y, z - 1)) return 1;
    if (flat_get_block(x, y, z + 1) == BLOCK_CHEST && chest_blocked_above(x, y, z + 1)) return 1;

    int self = chest_alloc_tile(x, y, z);
    if (self < 0) return 1;
    g_open_chest_indices[0] = self;
    g_open_chest_indices[1] = -1;
    g_open_chest_count = 1;
    g_open_chest_rows = 3;
    snprintf(g_open_chest_title, sizeof(g_open_chest_title), "Chest");

    int other = -1;
    if (flat_get_block(x - 1, y, z) == BLOCK_CHEST) {
        other = chest_alloc_tile(x - 1, y, z);
        g_open_chest_indices[0] = other;
        g_open_chest_indices[1] = self;
    } else if (flat_get_block(x + 1, y, z) == BLOCK_CHEST) {
        other = chest_alloc_tile(x + 1, y, z);
        g_open_chest_indices[0] = self;
        g_open_chest_indices[1] = other;
    } else if (flat_get_block(x, y, z - 1) == BLOCK_CHEST) {
        other = chest_alloc_tile(x, y, z - 1);
        g_open_chest_indices[0] = other;
        g_open_chest_indices[1] = self;
    } else if (flat_get_block(x, y, z + 1) == BLOCK_CHEST) {
        other = chest_alloc_tile(x, y, z + 1);
        g_open_chest_indices[0] = self;
        g_open_chest_indices[1] = other;
    }

    if (other >= 0) {
        g_open_chest_count = 2;
        g_open_chest_rows = 6;
        snprintf(g_open_chest_title, sizeof(g_open_chest_title), "Large chest");
    }

    set_screen(SCREEN_CHEST);
    if (g_mp_connected) pex_net_send_chest_open();
    return 1;
}

static int chest_open_slot_count(void) {
    return g_open_chest_count * 27;
}

static ItemStack *chest_get_open_slot_ptr(int local_slot) {
    if (local_slot < 0 || local_slot >= chest_open_slot_count()) return NULL;
    int chest_part = local_slot / 27;
    int slot = local_slot % 27;
    int idx = g_open_chest_indices[chest_part];
    if (idx < 0 || idx >= MAX_CHEST_TILES || !g_chest_tiles[idx].active) return NULL;
    return &g_chest_tiles[idx].slots[slot];
}

static void chest_spawn_stack_like_java(int x, int y, int z, ItemStack st) {
    if (stack_empty(&st)) return;
    float ox = ((float)rand() / (float)RAND_MAX) * 0.8f + 0.1f;
    float oy = ((float)rand() / (float)RAND_MAX) * 0.8f + 0.1f;
    float oz = ((float)rand() / (float)RAND_MAX) * 0.8f + 0.1f;
    int left = st.count;
    while (left > 0) {
        int take = (rand() % 21) + 10;
        if (take > left) take = left;
        left -= take;
        spawn_item_stack((float)x + ox, (float)y + oy, (float)z + oz, make_stack(st.id, take, st.damage), 1);
    }
}

static void chest_drop_contents_and_remove_at(int x, int y, int z) {
    int idx = chest_find_tile(x, y, z);
    if (idx >= 0) {
        ChestTile *ct = &g_chest_tiles[idx];
        for (int i = 0; i < 27; i++) {
            if (!stack_empty(&ct->slots[i])) {
                chest_spawn_stack_like_java(x, y, z, ct->slots[i]);
                stack_clear(&ct->slots[i]);
            }
        }
        chest_remove_tile_index(idx);
    }
}


static int furnace_find_tile(int x, int y, int z) {
    for (int i = 0; i < MAX_FURNACE_TILES; i++) {
        if (g_furnace_tiles[i].active && g_furnace_tiles[i].x == x && g_furnace_tiles[i].y == y && g_furnace_tiles[i].z == z) return i;
    }
    return -1;
}

static int furnace_alloc_tile(int x, int y, int z) {
    int existing = furnace_find_tile(x, y, z);
    if (existing >= 0) return existing;

    int free_i = -1;
    for (int i = 0; i < MAX_FURNACE_TILES; i++) {
        if (!g_furnace_tiles[i].active) { free_i = i; break; }
    }
    if (free_i < 0) return -1;

    FurnaceTile *ft = &g_furnace_tiles[free_i];
    memset(ft, 0, sizeof(*ft));
    ft->active = 1;
    ft->x = x; ft->y = y; ft->z = z;
    return free_i;
}

static void furnace_clear_all_tiles(void) {
    memset(g_furnace_tiles, 0, sizeof(g_furnace_tiles));
    g_open_furnace_index = -1;
}

static void furnace_close_open_inventory(void) {
    g_open_furnace_index = -1;
}

static FurnaceTile *furnace_open_tile(void) {
    if (g_open_furnace_index < 0 || g_open_furnace_index >= MAX_FURNACE_TILES) return NULL;
    if (!g_furnace_tiles[g_open_furnace_index].active) return NULL;
    return &g_furnace_tiles[g_open_furnace_index];
}

static ItemStack *furnace_get_slot_ptr(int local_slot) {
    FurnaceTile *ft = furnace_open_tile();
    if (!ft || local_slot < 0 || local_slot >= 3) return NULL;
    return &ft->slots[local_slot];
}

static void furnace_remove_tile_index(int idx) {
    if (idx < 0 || idx >= MAX_FURNACE_TILES) return;
    memset(&g_furnace_tiles[idx], 0, sizeof(g_furnace_tiles[idx]));
    if (g_open_furnace_index == idx) furnace_close_open_inventory();
    g_save_dirty = 1;
}

static int furnace_smelting_result(int id) {
    if (id == BLOCK_IRON_ORE) return ITEM_INGOT_IRON;
    if (id == BLOCK_GOLD_ORE) return ITEM_INGOT_GOLD;
    if (id == BLOCK_DIAMOND_ORE) return ITEM_DIAMOND;
    if (id == BLOCK_SAND) return BLOCK_GLASS;
    if (id == ITEM_PORK_RAW) return ITEM_PORK_COOKED;
    if (id == ITEM_FISH_RAW) return ITEM_FISH_COOKED;
    if (id == BLOCK_COBBLESTONE) return BLOCK_STONE;
    if (id == ITEM_CLAY) return ITEM_BRICK;
    return -1;
}

static int furnace_is_wood_material_block(int id) {
    switch (id) {
        case BLOCK_PLANKS:
        case BLOCK_LOG:
        case BLOCK_BOOKSHELF:
        case BLOCK_CHEST:
        case BLOCK_CRAFTING_TABLE:
        case BLOCK_WOOD_STAIRS:
        case BLOCK_FENCE:
        case BLOCK_JUKEBOX:
        case BLOCK_SIGN_POST:
        case BLOCK_WALL_SIGN:
        case BLOCK_WOOD_DOOR:
            return 1;
        default:
            return 0;
    }
}

static int furnace_item_burn_time(const ItemStack *fuel) {
    if (stack_empty(fuel)) return 0;
    int id = fuel->id;
    if (id < 256 && furnace_is_wood_material_block(id)) return 300;
    if (id == ITEM_STICK) return 100;
    if (id == ITEM_COAL) return 1600;
    if (id == ITEM_LAVA_BUCKET) return 20000;
    return 0;
}

static int furnace_can_smelt(const FurnaceTile *ft) {
    if (!ft || stack_empty(&ft->slots[0])) return 0;
    int out_id = furnace_smelting_result(ft->slots[0].id);
    if (out_id < 0) return 0;
    const ItemStack *out = &ft->slots[2];
    if (stack_empty(out)) return 1;
    if (out->id != out_id || out->damage != 0) return 0;
    return out->count < 64 && out->count < stack_limit_for_id(out_id);
}

static void furnace_smelt_item(FurnaceTile *ft) {
    if (!furnace_can_smelt(ft)) return;
    int out_id = furnace_smelting_result(ft->slots[0].id);
    if (stack_empty(&ft->slots[2])) ft->slots[2] = make_stack(out_id, 1, 0);
    else ft->slots[2].count++;

    ft->slots[0].count--;
    if (ft->slots[0].count <= 0) stack_clear(&ft->slots[0]);
}

static void furnace_set_lit_at(FurnaceTile *ft, int lit) {
    if (!ft) return;
    int cur = flat_get_block(ft->x, ft->y, ft->z);
    int want = lit ? BLOCK_FURNACE_LIT : BLOCK_FURNACE;
    if ((cur == BLOCK_FURNACE || cur == BLOCK_FURNACE_LIT) && cur != want) {
        int meta = flat_get_meta(ft->x, ft->y, ft->z);
        flat_begin_persistent_edit();
        flat_set_block(ft->x, ft->y, ft->z, want);
        flat_set_meta_raw(ft->x, ft->y, ft->z, meta);
        flat_end_persistent_edit();
    }
}

static void furnace_tick_tile(FurnaceTile *ft) {
    if (!ft || !ft->active) return;
    int block = flat_get_block(ft->x, ft->y, ft->z);
    if (block != BLOCK_FURNACE && block != BLOCK_FURNACE_LIT) return;

    int was_burning = ft->burn_time > 0;
    int changed = 0;

    if (ft->burn_time > 0) {
        ft->burn_time--;
        changed = 1;
    }

    if (ft->burn_time == 0 && furnace_can_smelt(ft)) {
        ft->current_item_burn_time = ft->burn_time = furnace_item_burn_time(&ft->slots[1]);
        if (ft->burn_time > 0) {
            changed = 1;
            if (!stack_empty(&ft->slots[1])) {
                ft->slots[1].count--;
                if (ft->slots[1].count <= 0) stack_clear(&ft->slots[1]);
            }
        }
    }

    if (ft->burn_time > 0 && furnace_can_smelt(ft)) {
        ft->cook_time++;
        changed = 1;
        if (ft->cook_time >= 200) {
            ft->cook_time = 0;
            furnace_smelt_item(ft);
        }
    } else if (ft->cook_time != 0) {
        ft->cook_time = 0;
        changed = 1;
    }

    if (was_burning != (ft->burn_time > 0)) {
        furnace_set_lit_at(ft, ft->burn_time > 0);
        changed = 1;
    }

    if (changed) g_save_dirty = 1;
}

static void furnace_tick_all(void) {
    for (int i = 0; i < MAX_FURNACE_TILES; i++) {
        if (g_furnace_tiles[i].active) furnace_tick_tile(&g_furnace_tiles[i]);
    }
}

static int furnace_burn_scaled(int pixels) {
    FurnaceTile *ft = furnace_open_tile();
    if (!ft) return 0;
    if (ft->current_item_burn_time == 0) ft->current_item_burn_time = 200;
    return ft->burn_time * pixels / ft->current_item_burn_time;
}

static int furnace_cook_scaled(int pixels) {
    FurnaceTile *ft = furnace_open_tile();
    if (!ft) return 0;
    return ft->cook_time * pixels / 200;
}

static void furnace_on_block_placed(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (id == BLOCK_FURNACE || id == BLOCK_FURNACE_LIT) {
        furnace_alloc_tile(x, y, z);
        g_save_dirty = 1;
    }
}

static int furnace_open_at(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (id != BLOCK_FURNACE && id != BLOCK_FURNACE_LIT) return 0;
    int idx = furnace_alloc_tile(x, y, z);
    if (idx < 0) return 1;
    g_open_furnace_index = idx;
    set_screen(SCREEN_FURNACE);
    return 1;
}

static void furnace_drop_contents_and_remove_at(int x, int y, int z) {
    int idx = furnace_find_tile(x, y, z);
    if (idx >= 0) {
        FurnaceTile *ft = &g_furnace_tiles[idx];
        for (int i = 0; i < 3; i++) {
            if (!stack_empty(&ft->slots[i])) {
                chest_spawn_stack_like_java(x, y, z, ft->slots[i]);
                stack_clear(&ft->slots[i]);
            }
        }
        furnace_remove_tile_index(idx);
    }
}

static void inventory_drop_selected_one(void) {
    ItemStack *s = &g_inventory[g_selected_hotbar_slot];
    if (stack_empty(s)) return;
    ItemStack one = make_stack(s->id, 1, s->damage);
    s->count--;
    if (s->count <= 0) stack_clear(s);
    if (g_mp_connected) pex_net_send_drop_item(one);
    else {
        spawn_item_stack(g_player_x, g_player_y - 0.30f, g_player_z, one, 0);
        g_save_dirty = 1;
    }
    restart_hand_swing();
    hud_add_chat("Dropped item.");
}



static int craft_id_at(const ItemStack *grid, int w, int x, int y) {
    const ItemStack *s = &grid[y * w + x];
    return stack_empty(s) ? 0 : s->id;
}

static int craft_count_nonempty(const ItemStack *grid, int n) {
    int c = 0;
    for (int i = 0; i < n; i++) if (!stack_empty(&grid[i])) c++;
    return c;
}

static int crafting_pattern_width(const char **rows, int h) {
    int w = 0;
    for (int y = 0; y < h; y++) {
        int len = (int)strlen(rows[y]);
        if (len > w) w = len;
    }
    return w;
}

static int crafting_match_at(const ItemStack *grid, int gw, int gh,
                             const char **rows, int pw, int ph,
                             const int map[256], int ox, int oy, int mirror) {
    for (int y = 0; y < gh; y++) {
        for (int x = 0; x < gw; x++) {
            int expected = 0;
            int rx = x - ox;
            int ry = y - oy;
            if (rx >= 0 && ry >= 0 && rx < pw && ry < ph) {
                int sx = mirror ? (pw - 1 - rx) : rx;
                char ch = ' ';
                if (sx < (int)strlen(rows[ry])) ch = rows[ry][sx];
                if (ch != ' ') expected = map[(unsigned char)ch];
            }
            if (craft_id_at(grid, gw, x, y) != expected) return 0;
        }
    }
    return 1;
}

static int crafting_match_recipe(const ItemStack *grid, int gw, int gh,
                                 const char **rows, int ph,
                                 int a_ch, int a_id, int b_ch, int b_id, int c_ch, int c_id) {
    int map[256]; memset(map, 0, sizeof(map));
    if (a_ch) map[(unsigned char)a_ch] = a_id;
    if (b_ch) map[(unsigned char)b_ch] = b_id;
    if (c_ch) map[(unsigned char)c_ch] = c_id;
    int pw = crafting_pattern_width(rows, ph);
    if (pw <= 0 || ph <= 0 || pw > gw || ph > gh) return 0;
    for (int oy = 0; oy <= gh - ph; oy++) {
        for (int ox = 0; ox <= gw - pw; ox++) {
            if (crafting_match_at(grid, gw, gh, rows, pw, ph, map, ox, oy, 0)) return 1;
            if (crafting_match_at(grid, gw, gh, rows, pw, ph, map, ox, oy, 1)) return 1;
        }
    }
    return 0;
}

static ItemStack crafting_make_result_if_match(const ItemStack *grid, int gw, int gh,
                                               int out_id, int out_count,
                                               const char **rows, int ph,
                                               int a_ch, int a_id, int b_ch, int b_id, int c_ch, int c_id) {
    if (crafting_match_recipe(grid, gw, gh, rows, ph, a_ch, a_id, b_ch, b_id, c_ch, c_id)) {
        return make_stack(out_id, out_count, 0);
    }
    ItemStack empty; memset(&empty, 0, sizeof(empty)); return empty;
}

#define TRY_RECIPE(OUT,COUNT,ROWS,PH,A,AI,B,BI,C,CI) do { \
    ItemStack r = crafting_make_result_if_match(grid, gw, gh, (OUT), (COUNT), (ROWS), (PH), (A), (AI), (B), (BI), (C), (CI)); \
    if (!stack_empty(&r)) return r; \
} while(0)

static ItemStack crafting_output_for_grid(const ItemStack *grid, int gw, int gh) {
    ItemStack empty; memset(&empty, 0, sizeof(empty));

    /* Java Beta 1.0 recipe groups from the uploaded deobfuscated source:
       RecipesTools, RecipesWeapons, RecipesIngots, RecipesFood, RecipesCrafting,
       RecipesArmor, then CraftingManager's direct recipes.  This expands to all
       99 registered Java recipes.  Matching is shaped and mirrorable like
       CraftingRecipe.matchRecipe(). */
    const int mats[5] = {BLOCK_PLANKS, BLOCK_COBBLESTONE, ITEM_INGOT_IRON, ITEM_DIAMOND, ITEM_INGOT_GOLD};
    const int pick[5] = {ITEM_WOODEN_PICKAXE, ITEM_STONE_PICKAXE, ITEM_IRON_PICKAXE, ITEM_DIAMOND_PICKAXE, ITEM_GOLD_PICKAXE};
    const int shovel[5] = {ITEM_WOODEN_SHOVEL, ITEM_STONE_SHOVEL, ITEM_IRON_SHOVEL, ITEM_DIAMOND_SHOVEL, ITEM_GOLD_SHOVEL};
    const int axe[5] = {ITEM_WOODEN_AXE, ITEM_STONE_AXE, ITEM_IRON_AXE, ITEM_DIAMOND_AXE, ITEM_GOLD_AXE};
    const int hoe[5] = {ITEM_WOODEN_HOE, ITEM_STONE_HOE, ITEM_IRON_HOE, ITEM_DIAMOND_HOE, ITEM_GOLD_HOE};
    const int sword[5] = {ITEM_WOODEN_SWORD, ITEM_STONE_SWORD, ITEM_IRON_SWORD, ITEM_DIAMOND_SWORD, ITEM_GOLD_SWORD};
    const char *pick_pat[3] = {"XXX", " # ", " # "};
    const char *shovel_pat[3] = {"X", "#", "#"};
    const char *axe_pat[3] = {"XX", "X#", " #"};
    const char *hoe_pat[3] = {"XX", " #", " #"};
    const char *sword_pat[3] = {"X", "X", "#"};
    for (int i = 0; i < 5; i++) {
        TRY_RECIPE(pick[i], 1, pick_pat, 3, 'X', mats[i], '#', ITEM_STICK, 0, 0);
        TRY_RECIPE(shovel[i], 1, shovel_pat, 3, 'X', mats[i], '#', ITEM_STICK, 0, 0);
        TRY_RECIPE(axe[i], 1, axe_pat, 3, 'X', mats[i], '#', ITEM_STICK, 0, 0);
        TRY_RECIPE(hoe[i], 1, hoe_pat, 3, 'X', mats[i], '#', ITEM_STICK, 0, 0);
        TRY_RECIPE(sword[i], 1, sword_pat, 3, 'X', mats[i], '#', ITEM_STICK, 0, 0);
    }

    const char *bow_pat[3] = {" #X", "# X", " #X"};
    TRY_RECIPE(ITEM_BOW, 1, bow_pat, 3, 'X', ITEM_STRING, '#', ITEM_STICK, 0, 0);
    const char *arrow_pat[3] = {"X", "#", "Y"};
    TRY_RECIPE(ITEM_ARROW, 4, arrow_pat, 3, 'X', ITEM_FLINT, '#', ITEM_STICK, 'Y', ITEM_FEATHER);

    const int block_mats[3] = {BLOCK_GOLD_BLOCK, BLOCK_IRON_BLOCK, BLOCK_DIAMOND_BLOCK};
    const int ingots[3] = {ITEM_INGOT_GOLD, ITEM_INGOT_IRON, ITEM_DIAMOND};
    const char *block9_pat[3] = {"###", "###", "###"};
    const char *one_pat[1] = {"#"};
    for (int i = 0; i < 3; i++) {
        TRY_RECIPE(block_mats[i], 1, block9_pat, 3, '#', ingots[i], 0, 0, 0, 0);
        TRY_RECIPE(ingots[i], 9, one_pat, 1, '#', block_mats[i], 0, 0, 0, 0);
    }

    const char *soup1[3] = {"Y", "X", "#"};
    TRY_RECIPE(ITEM_MUSHROOM_STEW, 1, soup1, 3, 'X', BLOCK_BROWN_MUSHROOM, 'Y', BLOCK_RED_MUSHROOM, '#', ITEM_BOWL_EMPTY);
    TRY_RECIPE(ITEM_MUSHROOM_STEW, 1, soup1, 3, 'X', BLOCK_RED_MUSHROOM, 'Y', BLOCK_BROWN_MUSHROOM, '#', ITEM_BOWL_EMPTY);

    const char *chest_pat[3] = {"###", "# #", "###"};
    TRY_RECIPE(BLOCK_CHEST, 1, chest_pat, 3, '#', BLOCK_PLANKS, 0, 0, 0, 0);
    TRY_RECIPE(BLOCK_FURNACE, 1, chest_pat, 3, '#', BLOCK_COBBLESTONE, 0, 0, 0, 0);
    const char *workbench_pat[2] = {"##", "##"};
    TRY_RECIPE(BLOCK_CRAFTING_TABLE, 1, workbench_pat, 2, '#', BLOCK_PLANKS, 0, 0, 0, 0);

    const int armor_mats[5] = {ITEM_LEATHER, BLOCK_FIRE, ITEM_INGOT_IRON, ITEM_DIAMOND, ITEM_INGOT_GOLD};
    const int helmets[5] = {ITEM_HELMET_LEATHER, ITEM_HELMET_CHAIN, ITEM_HELMET_IRON, ITEM_HELMET_DIAMOND, ITEM_HELMET_GOLD};
    const int plates[5]  = {ITEM_PLATE_LEATHER, ITEM_PLATE_CHAIN, ITEM_PLATE_IRON, ITEM_PLATE_DIAMOND, ITEM_PLATE_GOLD};
    const int legs[5]    = {ITEM_LEGS_LEATHER, ITEM_LEGS_CHAIN, ITEM_LEGS_IRON, ITEM_LEGS_DIAMOND, ITEM_LEGS_GOLD};
    const int boots[5]   = {ITEM_BOOTS_LEATHER, ITEM_BOOTS_CHAIN, ITEM_BOOTS_IRON, ITEM_BOOTS_DIAMOND, ITEM_BOOTS_GOLD};
    const char *helmet_pat[2] = {"XXX", "X X"};
    const char *plate_pat[3] = {"X X", "XXX", "XXX"};
    const char *legs_pat[3] = {"XXX", "X X", "X X"};
    const char *boots_pat[2] = {"X X", "X X"};
    for (int i = 0; i < 5; i++) {
        TRY_RECIPE(helmets[i], 1, helmet_pat, 2, 'X', armor_mats[i], 0, 0, 0, 0);
        TRY_RECIPE(plates[i], 1, plate_pat, 3, 'X', armor_mats[i], 0, 0, 0, 0);
        TRY_RECIPE(legs[i], 1, legs_pat, 3, 'X', armor_mats[i], 0, 0, 0, 0);
        TRY_RECIPE(boots[i], 1, boots_pat, 2, 'X', armor_mats[i], 0, 0, 0, 0);
    }

    const char *paper_pat[1] = {"###"};
    TRY_RECIPE(ITEM_PAPER, 3, paper_pat, 1, '#', ITEM_REED, 0, 0, 0, 0);
    const char *book_pat[3] = {"#", "#", "#"};
    TRY_RECIPE(ITEM_BOOK, 1, book_pat, 3, '#', ITEM_PAPER, 0, 0, 0, 0);
    const char *fence_pat[2] = {"###", "###"};
    TRY_RECIPE(BLOCK_FENCE, 2, fence_pat, 2, '#', ITEM_STICK, 0, 0, 0, 0);
    const char *jukebox_pat[3] = {"###", "#X#", "###"};
    TRY_RECIPE(BLOCK_JUKEBOX, 1, jukebox_pat, 3, '#', BLOCK_PLANKS, 'X', ITEM_DIAMOND, 0, 0);
    const char *bookshelf_pat[3] = {"###", "XXX", "###"};
    TRY_RECIPE(BLOCK_BOOKSHELF, 1, bookshelf_pat, 3, '#', BLOCK_PLANKS, 'X', ITEM_BOOK, 0, 0);
    TRY_RECIPE(BLOCK_SNOW_BLOCK, 1, workbench_pat, 2, '#', ITEM_SNOWBALL, 0, 0, 0, 0);
    TRY_RECIPE(BLOCK_CLAY, 1, workbench_pat, 2, '#', ITEM_CLAY, 0, 0, 0, 0);
    TRY_RECIPE(BLOCK_BRICK, 1, workbench_pat, 2, '#', ITEM_BRICK, 0, 0, 0, 0);
    TRY_RECIPE(BLOCK_GLOWSTONE, 1, block9_pat, 3, '#', ITEM_GLOWSTONE_DUST, 0, 0, 0, 0);
    TRY_RECIPE(BLOCK_WOOL, 1, block9_pat, 3, '#', ITEM_STRING, 0, 0, 0, 0);
    const char *tnt_pat[3] = {"X#X", "#X#", "X#X"};
    TRY_RECIPE(BLOCK_TNT, 1, tnt_pat, 3, 'X', ITEM_GUNPOWDER, '#', BLOCK_SAND, 0, 0);
    TRY_RECIPE(BLOCK_SLAB, 3, paper_pat, 1, '#', BLOCK_COBBLESTONE, 0, 0, 0, 0);
    const char *ladder_pat[3] = {"# #", "###", "# #"};
    TRY_RECIPE(BLOCK_LADDER, 1, ladder_pat, 3, '#', ITEM_STICK, 0, 0, 0, 0);
    const char *door_pat[3] = {"##", "##", "##"};
    TRY_RECIPE(ITEM_DOOR_WOOD, 1, door_pat, 3, '#', BLOCK_PLANKS, 0, 0, 0, 0);
    TRY_RECIPE(ITEM_DOOR_IRON, 1, door_pat, 3, '#', ITEM_INGOT_IRON, 0, 0, 0, 0);
    const char *sign_pat[3] = {"###", "###", " X "};
    TRY_RECIPE(ITEM_SIGN, 1, sign_pat, 3, '#', BLOCK_PLANKS, 'X', ITEM_STICK, 0, 0);
    TRY_RECIPE(BLOCK_PLANKS, 4, one_pat, 1, '#', BLOCK_LOG, 0, 0, 0, 0);
    const char *stick_pat[2] = {"#", "#"};
    TRY_RECIPE(ITEM_STICK, 4, stick_pat, 2, '#', BLOCK_PLANKS, 0, 0, 0, 0);
    const char *torch_pat[2] = {"X", "#"};
    TRY_RECIPE(BLOCK_TORCH, 4, torch_pat, 2, 'X', ITEM_COAL, '#', ITEM_STICK, 0, 0);
    const char *bowl_pat[2] = {"# #", " # "};
    TRY_RECIPE(ITEM_BOWL_EMPTY, 4, bowl_pat, 2, '#', BLOCK_PLANKS, 0, 0, 0, 0);
    const char *rail_pat[3] = {"X X", "X#X", "X X"};
    TRY_RECIPE(BLOCK_RAILS, 16, rail_pat, 3, 'X', ITEM_INGOT_IRON, '#', ITEM_STICK, 0, 0);
    const char *minecart_pat[2] = {"# #", "###"};
    TRY_RECIPE(ITEM_MINECART, 1, minecart_pat, 2, '#', ITEM_INGOT_IRON, 0, 0, 0, 0);
    const char *stack2_pat[2] = {"A", "B"};
    TRY_RECIPE(BLOCK_JACK_O_LANTERN, 1, stack2_pat, 2, 'A', BLOCK_PUMPKIN, 'B', BLOCK_TORCH, 0, 0);
    TRY_RECIPE(ITEM_MINECART_CRATE, 1, stack2_pat, 2, 'A', BLOCK_CHEST, 'B', ITEM_MINECART, 0, 0);
    TRY_RECIPE(ITEM_MINECART_POWERED, 1, stack2_pat, 2, 'A', BLOCK_FURNACE, 'B', ITEM_MINECART, 0, 0);
    TRY_RECIPE(ITEM_BOAT, 1, minecart_pat, 2, '#', BLOCK_PLANKS, 0, 0, 0, 0);
    TRY_RECIPE(ITEM_BUCKET_EMPTY, 1, bowl_pat, 2, '#', ITEM_INGOT_IRON, 0, 0, 0, 0);
    const char *flintsteel_pat[2] = {"A ", " B"};
    TRY_RECIPE(ITEM_FLINT_AND_IRON, 1, flintsteel_pat, 2, 'A', ITEM_INGOT_IRON, 'B', ITEM_FLINT, 0, 0);
    TRY_RECIPE(ITEM_BREAD, 1, paper_pat, 1, '#', ITEM_WHEAT, 0, 0, 0, 0);
    const char *stairs_pat[3] = {"#  ", "## ", "###"};
    TRY_RECIPE(BLOCK_WOOD_STAIRS, 4, stairs_pat, 3, '#', BLOCK_PLANKS, 0, 0, 0, 0);
    const char *rod_pat[3] = {"  #", " #X", "# X"};
    TRY_RECIPE(ITEM_FISHING_ROD, 1, rod_pat, 3, '#', ITEM_STICK, 'X', ITEM_STRING, 0, 0);
    TRY_RECIPE(BLOCK_COBBLE_STAIRS, 4, stairs_pat, 3, '#', BLOCK_COBBLESTONE, 0, 0, 0, 0);
    const char *painting_pat[3] = {"###", "#X#", "###"};
    TRY_RECIPE(ITEM_PAINTING, 1, painting_pat, 3, '#', ITEM_STICK, 'X', BLOCK_WOOL, 0, 0);
    TRY_RECIPE(ITEM_APPLE_GOLD, 1, painting_pat, 3, '#', BLOCK_GOLD_BLOCK, 'X', ITEM_APPLE_RED, 0, 0);
    TRY_RECIPE(BLOCK_LEVER, 1, torch_pat, 2, 'X', ITEM_STICK, '#', BLOCK_COBBLESTONE, 0, 0);
    TRY_RECIPE(BLOCK_REDSTONE_TORCH_ON, 1, torch_pat, 2, 'X', ITEM_REDSTONE, '#', ITEM_STICK, 0, 0);
    const char *clock_pat[3] = {" # ", "#X#", " # "};
    TRY_RECIPE(ITEM_CLOCK, 1, clock_pat, 3, '#', ITEM_INGOT_GOLD, 'X', ITEM_REDSTONE, 0, 0);
    TRY_RECIPE(ITEM_COMPASS, 1, clock_pat, 3, '#', ITEM_INGOT_IRON, 'X', ITEM_REDSTONE, 0, 0);
    TRY_RECIPE(BLOCK_STONE_BUTTON, 1, stick_pat, 2, '#', BLOCK_STONE, 0, 0, 0, 0);
    TRY_RECIPE(BLOCK_STONE_PRESSURE_PLATE, 1, paper_pat, 1, '#', BLOCK_STONE, 0, 0, 0, 0);
    TRY_RECIPE(BLOCK_WOOD_PRESSURE_PLATE, 1, paper_pat, 1, '#', BLOCK_PLANKS, 0, 0, 0, 0);

    return empty;
}

#undef TRY_RECIPE

static ItemStack workbench_crafting_output(void) {
    return crafting_output_for_grid(g_workbench_grid, 3, 3);
}

static ItemStack inventory_crafting_output(void) {
    return crafting_output_for_grid(g_craft_grid, 2, 2);
}

static ItemStack current_crafting_output(void) {
    if (g_screen == SCREEN_WORKBENCH) return workbench_crafting_output();
    return inventory_crafting_output();
}

static int inventory_take_crafting_output(void) {
    ItemStack out = current_crafting_output();
    if (stack_empty(&out)) return 0;

    if (g_mp_connected) {
        if (!stack_empty(&g_carried_stack)) {
            hud_add_chat("Server crafting needs an empty cursor.");
            return 0;
        }
        pex_net_send_craft_request();
        if (g_screen == SCREEN_WORKBENCH) memset(g_workbench_grid, 0, sizeof(g_workbench_grid));
        else memset(g_craft_grid, 0, sizeof(g_craft_grid));
        return 1;
    }

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
    if (slot >= 200 && slot < 254) return chest_get_open_slot_ptr(slot - 200);
    if (slot >= 400 && slot < 403) return furnace_get_slot_ptr(slot - 400);
    return NULL;
}

static int inventory_slot_allows_place(int slot) {
    /* Java furnace output slot is take-only; players cannot place or swap
       arbitrary items into the smelt result slot. */
    return slot != 402;
}

static int inventory_slot_at(int mx, int my) {
    int container_h = (g_screen == SCREEN_CHEST && g_open_chest_rows == 6) ? 222 : 166;
    int inv_x = (g_gui_w - 176) / 2;
    int inv_y = (g_gui_h - container_h) / 2;
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
        int rows = (g_open_chest_rows == 6) ? 6 : 3;
        int inv_slots_y = (rows == 6) ? 140 : 84;
        int hotbar_y = (rows == 6) ? 198 : 142;
        for (int row = 0; row < rows; row++) for (int col = 0; col < 9; col++) {
            int sx = 8 + col * 18;
            int sy = 18 + row * 18;
            if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return 200 + col + row * 9;
        }
        for (int row = 0; row < 3; row++) for (int col = 0; col < 9; col++) {
            int sx = 8 + col * 18;
            int sy = inv_slots_y + row * 18;
            if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return 9 + col + row * 9;
        }
        for (int col = 0; col < 9; col++) {
            int sx = 8 + col * 18;
            int sy = hotbar_y;
            if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return col;
        }
        return -1;
    }

    if (g_screen == SCREEN_FURNACE) {
        if (rx >= 56 - 1 && rx < 56 + 17 && ry >= 17 - 1 && ry < 17 + 17) return 400;
        if (rx >= 56 - 1 && rx < 56 + 17 && ry >= 53 - 1 && ry < 53 + 17) return 401;
        if (rx >= 116 - 1 && rx < 116 + 17 && ry >= 35 - 1 && ry < 35 + 17) return 402;
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
    int sync_chest_after = (g_mp_connected && g_screen == SCREEN_CHEST);
    int container_h = (g_screen == SCREEN_CHEST && g_open_chest_rows == 6) ? 222 : 166;
    int inv_x = (g_gui_w - 176) / 2;
    int inv_y = (g_gui_h - container_h) / 2;
    int outside = (mx < inv_x || my < inv_y || mx >= inv_x + 176 || my >= inv_y + container_h);
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
            if (!inventory_slot_allows_place(slot)) return;
            *s = g_carried_stack; stack_clear(&g_carried_stack);
        } else if (stack_same_item(s, &g_carried_stack) && s->count < stack_limit_for_id(s->id)) {
            if (!inventory_slot_allows_place(slot)) {
                int room = stack_limit_for_id(g_carried_stack.id) - g_carried_stack.count;
                int move = s->count < room ? s->count : room;
                if (move <= 0) return;
                g_carried_stack.count += move;
                s->count -= move;
                if (s->count <= 0) stack_clear(s);
            } else {
                int room = stack_limit_for_id(s->id) - s->count;
                int move = g_carried_stack.count < room ? g_carried_stack.count : room;
                s->count += move;
                g_carried_stack.count -= move;
                s->pop_time = 5;
                if (g_carried_stack.count <= 0) stack_clear(&g_carried_stack);
            }
        } else {
            if (!inventory_slot_allows_place(slot)) return;
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
            if (!inventory_slot_allows_place(slot)) return;
            *s = make_stack(g_carried_stack.id, 1, g_carried_stack.damage);
            if (--g_carried_stack.count <= 0) stack_clear(&g_carried_stack);
        } else if (stack_same_item(s, &g_carried_stack) && s->count < stack_limit_for_id(s->id)) {
            if (!inventory_slot_allows_place(slot)) {
                if (g_carried_stack.count >= stack_limit_for_id(g_carried_stack.id)) return;
                g_carried_stack.count++;
                s->count--;
                if (s->count <= 0) stack_clear(s);
            } else {
                s->count++;
                s->pop_time = 5;
                if (--g_carried_stack.count <= 0) stack_clear(&g_carried_stack);
            }
        }
    }
    if (sync_chest_after) pex_net_send_chest_update();
    g_save_dirty = 1;
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
        int ray_id = flat_get_block(bx, by, bz);
        /* Water/lava should not be selectable like solid blocks.  Let the
           cursor pass through liquids so the outline/mining target can be the
           block behind or under the water. */
        if (ray_id != 0 && !block_is_liquid(ray_id)) {
            /* Snow is only a 2/16-high plate.  The old full-cell ray test made
               the cursor hit invisible snow volume beside/above the visible
               layer instead of the real block behind it. */
            if (ray_id == BLOCK_SNOW_LAYER) {
                float local_y = y - floorf(y);
                if (local_y > (2.0f / 16.0f)) {
                    prev_bx = bx; prev_by = by; prev_bz = bz;
                    continue;
                }
            } else if (ray_id == BLOCK_SLAB) {
                float local_y = y - floorf(y);
                if (local_y > 0.5f) { prev_bx = bx; prev_by = by; prev_bz = bz; continue; }
            } else if (ray_id == BLOCK_STONE_PRESSURE_PLATE || ray_id == BLOCK_WOOD_PRESSURE_PLATE || ray_id == BLOCK_RAILS || ray_id == BLOCK_REDSTONE_WIRE) {
                float local_y = y - floorf(y);
                if (local_y > 0.10f) { prev_bx = bx; prev_by = by; prev_bz = bz; continue; }
            } else if (ray_id == BLOCK_LADDER) {
                float local_x = x - floorf(x), local_z = z - floorf(z);
                int meta = flat_get_meta(bx, by, bz);
                int ok = 0;
                float lt = 2.0f / 16.0f;
                if (meta == 2) ok = local_z > 1.0f - lt;
                else if (meta == 3) ok = local_z < lt;
                else if (meta == 4) ok = local_x > 1.0f - lt;
                else if (meta == 5) ok = local_x < lt;
                if (!ok) { prev_bx = bx; prev_by = by; prev_bz = bz; continue; }
            } else if (ray_id == BLOCK_STONE_BUTTON || ray_id == BLOCK_LEVER) {
                float lx = x - floorf(x), ly = y - floorf(y), lz = z - floorf(z);
                int meta = flat_get_meta(bx, by, bz) & 7;
                int ok = 0;
                if (ray_id == BLOCK_STONE_BUTTON) {
                    float t = (flat_get_meta(bx, by, bz) & 8) ? (1.0f/16.0f) : (2.0f/16.0f);
                    if (ly >= 6.0f/16.0f && ly <= 10.0f/16.0f) {
                        if (meta == 1) ok = lx <= t && lz >= 0.5f - 3.0f/16.0f && lz <= 0.5f + 3.0f/16.0f;
                        else if (meta == 2) ok = lx >= 1.0f - t && lz >= 0.5f - 3.0f/16.0f && lz <= 0.5f + 3.0f/16.0f;
                        else if (meta == 3) ok = lz <= t && lx >= 0.5f - 3.0f/16.0f && lx <= 0.5f + 3.0f/16.0f;
                        else if (meta == 4) ok = lz >= 1.0f - t && lx >= 0.5f - 3.0f/16.0f && lx <= 0.5f + 3.0f/16.0f;
                    }
                } else {
                    ok = (ly <= 0.85f && lx >= 0.30f && lx <= 0.70f && lz >= 0.30f && lz <= 0.70f);
                }
                if (!ok) { prev_bx = bx; prev_by = by; prev_bz = bz; continue; }
            } else if (block_is_door_id(ray_id)) {
                int ly = door_lower_y_at(bx, by, bz);
                int meta = flat_get_meta(bx, ly, bz);
                int dir = meta & 3;
                if (door_meta_is_open(meta)) dir = (dir + 1) & 3;
                float local_x = x - floorf(x), local_z = z - floorf(z);
                int ok = 0;
                if (dir == 0) ok = local_z < 0.20f;
                else if (dir == 2) ok = local_z > 0.80f;
                else if (dir == 1) ok = local_x > 0.80f;
                else ok = local_x < 0.20f;
                if (!ok) {
                    prev_bx = bx; prev_by = by; prev_bz = bz;
                    continue;
                }
            }
            float block_dx = x - g_player_x;
            float block_dy = y - g_player_y;
            float block_dz = z - g_player_z;
            float block_dist = sqrtf(block_dx * block_dx + block_dy * block_dy + block_dz * block_dz);
            float player_t = 9999.0f;
            if (pex_net_player_ray_distance(block_dist, &player_t) && player_t < block_dist - 0.02f) {
                return h;
            }
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

/* Ray that can land on a liquid SOURCE block (the normal raycast passes through
   liquids). Stops at the first solid block. Only level-0 sources are pickable. */
static int flat_raycast_liquid_source(int *ox, int *oy, int *oz) {
    float dx, dy, dz;
    player_look_vector(&dx, &dy, &dz);
    for (float t = 0.0f; t <= 5.0f; t += 0.05f) {
        int bx = (int)floorf(g_player_x + dx * t);
        int by = (int)floorf(g_player_y + dy * t);
        int bz = (int)floorf(g_player_z + dz * t);
        int id = flat_get_block(bx, by, bz);
        if (id == 0) continue;
        if (block_is_liquid(id)) {
            if (flat_get_level(bx, by, bz) == 0) { *ox = bx; *oy = by; *oz = bz; return 1; }
            continue;
        }
        return 0;
    }
    return 0;
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
    g_last_sent_mine_stage = -1;
}

static int held_item_can_harvest_drop_for_block(int block_id) {
    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    int held_id = stack_empty(held) ? 0 : held->id;
    return held_item_can_harvest_block_id(held_id, block_id);
}

static int door_item_for_block(int id) {
    if (id == BLOCK_WOOD_DOOR) return ITEM_DOOR_WOOD;
    if (id == BLOCK_IRON_DOOR) return ITEM_DOOR_IRON;
    return id;
}

static int door_direction_from_yaw(void) {
    int d = (int)floorf((g_player_yaw + 180.0f) * 4.0f / 360.0f + 0.5f) & 3;
    return d;
}

static int stair_direction_from_yaw(void) {
    /* Match Beta BlockStairs.onBlockPlacedBy instead of reusing door metadata. */
    int d = (int)floorf(g_player_yaw * 4.0f / 360.0f + 0.5f) & 3;
    if (d == 0) return 2;
    if (d == 1) return 1;
    if (d == 2) return 3;
    return 0;
}

static int furnace_facing_from_yaw(void) {
    int d = (int)floorf(g_player_yaw * 4.0f / 360.0f + 0.5f) & 3;
    if (d == 0) return 2;
    if (d == 1) return 5;
    if (d == 2) return 3;
    return 4;
}

static void door_toggle_at(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (!block_is_door_id(id)) return;
    int ly = door_lower_y_at(x, y, z);
    int meta = flat_get_meta(x, ly, z);
    meta ^= 4;
    if (!g_mp_connected) flat_begin_persistent_edit();
    flat_set_meta_raw(x, ly, z, meta);
    if (flat_get_block(x, ly + 1, z) == id) flat_set_meta_raw(x, ly + 1, z, 8 | (meta & 4));
    if (!g_mp_connected) flat_end_persistent_edit();
    if (g_mp_connected) pex_net_send_block_action(PEX_BLOCK_PLACE, ly == y ? x : x, ly, z, 0, id);
    restart_hand_swing();
}

static void door_break_at(int x, int y, int z, int drop_item) {
    int id = flat_get_block(x, y, z);
    if (!block_is_door_id(id)) return;
    int ly = door_lower_y_at(x, y, z);
    spawn_block_destroy_particles(x, ly, z, id);
    if (flat_get_block(x, ly + 1, z) == id) spawn_block_destroy_particles(x, ly + 1, z, id);
    flat_begin_persistent_edit();
    flat_set_block(x, ly + 1, z, 0);
    flat_set_block(x, ly, z, 0);
    flat_end_persistent_edit();
    if (drop_item) spawn_item_stack((float)x + 0.5f, (float)ly + 0.7f, (float)z + 0.5f, make_stack(door_item_for_block(id), 1, 0), 1);
}

static int place_door_from_item(int item_id, int x, int y, int z) {
    int block_id = (item_id == ITEM_DOOR_IRON) ? BLOCK_IRON_DOOR : BLOCK_WOOD_DOOR;
    if (y < FLAT_WORLD_Y_MIN || y + 1 > FLAT_WORLD_Y_MAX) return 0;
    if (flat_get_block(x, y - 1, z) == 0 || block_is_liquid(flat_get_block(x, y - 1, z))) return 0;
    if (flat_get_block(x, y, z) != 0 || flat_get_block(x, y + 1, z) != 0) return 0;
    if (flat_block_intersects_player(x, y, z) || flat_block_intersects_player(x, y + 1, z)) return 0;
    int dir = door_direction_from_yaw();
    if (!g_mp_connected) flat_begin_persistent_edit();
    flat_set_block(x, y, z, block_id);
    flat_set_meta_raw(x, y, z, dir & 3);
    flat_set_block(x, y + 1, z, block_id);
    flat_set_meta_raw(x, y + 1, z, 8);
    if (!g_mp_connected) flat_end_persistent_edit();
    return 1;
}

static int ladder_can_attach_to_face(int face) {
    return face >= 2 && face <= 5;
}

static int block_is_pressure_plate(int id) {
    return id == BLOCK_STONE_PRESSURE_PLATE || id == BLOCK_WOOD_PRESSURE_PLATE;
}

static int block_is_button_or_lever(int id) {
    return id == BLOCK_STONE_BUTTON || id == BLOCK_LEVER;
}

static int flat_block_occludes_for_support(int id) {
    if (id == 0 || block_is_liquid(id)) return 0;
    if (id == BLOCK_SLAB || id == BLOCK_WOOD_STAIRS || id == BLOCK_COBBLE_STAIRS || id == BLOCK_FENCE ||
        id == BLOCK_CACTUS || block_is_door_id(id) || id == BLOCK_LADDER || id == BLOCK_RAILS ||
        block_is_pressure_plate(id) || id == BLOCK_STONE_BUTTON || id == BLOCK_LEVER ||
        id == BLOCK_TORCH || id == BLOCK_REDSTONE_TORCH_OFF || id == BLOCK_REDSTONE_TORCH_ON ||
        id == BLOCK_SNOW_LAYER || id == BLOCK_REDSTONE_WIRE || id == BLOCK_SIGN_POST || id == BLOCK_WALL_SIGN) return 0;
    return 1;
}

static int block_supports_attached_side(int x, int y, int z, int side_meta) {
    if (side_meta == 1) return flat_block_occludes_for_support(flat_get_block(x - 1, y, z));
    if (side_meta == 2) return flat_block_occludes_for_support(flat_get_block(x + 1, y, z));
    if (side_meta == 3) return flat_block_occludes_for_support(flat_get_block(x, y, z - 1));
    if (side_meta == 4) return flat_block_occludes_for_support(flat_get_block(x, y, z + 1));
    return 0;
}

static int side_meta_from_placement_face(int face) {
    if (face == 5) return 1; /* support is west of the new block */
    if (face == 4) return 2; /* support is east */
    if (face == 3) return 3; /* support is north */
    if (face == 2) return 4; /* support is south */
    return 0;
}

static int lever_meta_from_placement_face(int face) {
    if (face == 1) {
        int yaw4 = (int)floorf(g_player_yaw * 4.0f / 360.0f + 0.5f) & 3;
        return (yaw4 & 1) ? 5 : 6;
    }
    return side_meta_from_placement_face(face);
}

static int lever_can_stay_with_meta(int x, int y, int z, int meta) {
    int side = meta & 7;
    if (side == 5 || side == 6) return flat_block_occludes_for_support(flat_get_block(x, y - 1, z));
    return block_supports_attached_side(x, y, z, side);
}

static int redstone_active_source_at(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    int meta = flat_get_meta(x, y, z);
    if (id == BLOCK_STONE_BUTTON) return (meta & 8) ? 15 : 0;
    if (id == BLOCK_LEVER) return (meta & 8) ? 15 : 0;
    if (block_is_pressure_plate(id)) return (meta & 1) ? 15 : 0;
    if (id == BLOCK_REDSTONE_TORCH_ON) return 15;
    return 0;
}

static int attached_source_powers_block(int sx, int sy, int sz, int x, int y, int z) {
    int id = flat_get_block(sx, sy, sz);
    int meta = flat_get_meta(sx, sy, sz);
    if ((id != BLOCK_STONE_BUTTON && id != BLOCK_LEVER) || !(meta & 8)) return 0;
    int side = meta & 7;
    if (side == 1) return x == sx - 1 && y == sy && z == sz;
    if (side == 2) return x == sx + 1 && y == sy && z == sz;
    if (side == 3) return x == sx && y == sy && z == sz - 1;
    if (side == 4) return x == sx && y == sy && z == sz + 1;
    if (side == 5 || side == 6) return x == sx && y == sy - 1 && z == sz;
    return 0;
}

static int redstone_block_is_strongly_powered(int x, int y, int z) {
    if (!flat_in_bounds(x, y, z)) return 0;
    if (block_is_pressure_plate(flat_get_block(x, y + 1, z)) && (flat_get_meta(x, y + 1, z) & 1)) return 1;

    static const int d[6][3] = {{1,0,0},{-1,0,0},{0,0,1},{0,0,-1},{0,1,0},{0,-1,0}};
    for (int i = 0; i < 6; i++) {
        int sx = x + d[i][0], sy = y + d[i][1], sz = z + d[i][2];
        if (attached_source_powers_block(sx, sy, sz, x, y, z)) return 1;
        if (flat_get_block(sx, sy, sz) == BLOCK_REDSTONE_TORCH_ON) return 1;
    }
    return 0;
}

static int redstone_wire_strength_at(int x, int y, int z) {
    return flat_get_block(x, y, z) == BLOCK_REDSTONE_WIRE ? (flat_get_meta(x, y, z) & 15) : 0;
}

static int redstone_calc_wire_strength_at(int x, int y, int z) {
    int best = 0;
    static const int d[6][3] = {{1,0,0},{-1,0,0},{0,0,1},{0,0,-1},{0,1,0},{0,-1,0}};
    for (int i = 0; i < 6; i++) {
        int nx = x + d[i][0], ny = y + d[i][1], nz = z + d[i][2];
        int src = redstone_active_source_at(nx, ny, nz);
        if (src > best) best = src;
        if (redstone_block_is_strongly_powered(nx, ny, nz) && best < 15) best = 15;
        int w = redstone_wire_strength_at(nx, ny, nz);
        if (w > 0 && w - 1 > best) best = w - 1;

        /* Basic vertical wire stepping for one-block rises/drops. */
        w = redstone_wire_strength_at(nx, ny + 1, nz);
        if (w > 0 && w - 1 > best) best = w - 1;
        w = redstone_wire_strength_at(nx, ny - 1, nz);
        if (w > 0 && w - 1 > best) best = w - 1;
    }
    if (best < 0) best = 0;
    if (best > 15) best = 15;
    return best;
}

static int redstone_door_should_open(int x, int lower_y, int z) {
    static const int d[6][3] = {{1,0,0},{-1,0,0},{0,0,1},{0,0,-1},{0,1,0},{0,-1,0}};
    for (int half = 0; half < 2; half++) {
        int y = lower_y + half;
        if (redstone_block_is_strongly_powered(x, y, z)) return 1;
        for (int i = 0; i < 6; i++) {
            int nx = x + d[i][0], ny = y + d[i][1], nz = z + d[i][2];
            if (redstone_active_source_at(nx, ny, nz) > 0) return 1;
            if (redstone_wire_strength_at(nx, ny, nz) > 0) return 1;
            if (redstone_block_is_strongly_powered(nx, ny, nz)) return 1;
        }
    }
    return 0;
}

static void redstone_set_door_open_if_needed(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (!block_is_door_id(id)) return;
    int ly = door_lower_y_at(x, y, z);
    int lower_meta = flat_get_meta(x, ly, z);
    int should_open = redstone_door_should_open(x, ly, z);
    int is_open = (lower_meta & 4) ? 1 : 0;
    if (should_open == is_open) return;
    lower_meta = should_open ? (lower_meta | 4) : (lower_meta & ~4);
    flat_set_meta_raw(x, ly, z, lower_meta);
    if (flat_get_block(x, ly + 1, z) == id) flat_set_meta_raw(x, ly + 1, z, 8 | (lower_meta & 4));
}

static void redstone_update_near(int cx, int cy, int cz) {
    int radius = 16;
    int y0 = cy - 3, y1 = cy + 3;
    if (y0 < FLAT_WORLD_Y_MIN) y0 = FLAT_WORLD_Y_MIN;
    if (y1 > FLAT_WORLD_Y_MAX) y1 = FLAT_WORLD_Y_MAX;

    flat_begin_persistent_edit();
    for (int pass = 0; pass < 16; pass++) {
        int changed = 0;
        for (int y = y0; y <= y1; y++) {
            for (int z = cz - radius; z <= cz + radius; z++) {
                for (int x = cx - radius; x <= cx + radius; x++) {
                    if (!flat_in_bounds(x, y, z) || flat_get_block(x, y, z) != BLOCK_REDSTONE_WIRE) continue;
                    int old = flat_get_meta(x, y, z) & 15;
                    int now = redstone_calc_wire_strength_at(x, y, z);
                    if (old != now) { flat_set_meta_raw(x, y, z, now); changed = 1; }
                }
            }
        }
        if (!changed) break;
    }

    for (int y = y0; y <= y1; y++) {
        for (int z = cz - radius; z <= cz + radius; z++) {
            for (int x = cx - radius; x <= cx + radius; x++) {
                int id = flat_get_block(x, y, z);
                if (block_is_door_id(id) && !door_meta_is_upper(flat_get_meta(x, y, z))) {
                    redstone_set_door_open_if_needed(x, y, z);
                }
            }
        }
    }
    flat_end_persistent_edit();
}

static void add_button_timer(int x, int y, int z) {
    int slot = -1;
    for (int i = 0; i < MAX_BUTTON_TIMERS; i++) {
        if (g_button_timers[i].active && g_button_timers[i].x == x && g_button_timers[i].y == y && g_button_timers[i].z == z) {
            slot = i;
            break;
        }
        if (slot < 0 && !g_button_timers[i].active) slot = i;
    }
    if (slot < 0) slot = 0;
    g_button_timers[slot].active = 1;
    g_button_timers[slot].x = x;
    g_button_timers[slot].y = y;
    g_button_timers[slot].z = z;
    g_button_timers[slot].ticks_left = 20;
}

static void press_button_at(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (id != BLOCK_STONE_BUTTON) return;
    int meta = flat_get_meta(x, y, z);
    if (meta & 8) return;
    flat_set_meta_raw(x, y, z, (meta & 7) | 8);
    add_button_timer(x, y, z);
    redstone_update_near(x, y, z);
    if (g_mp_connected) pex_net_send_block_action(PEX_BLOCK_PLACE, x, y, z, 0, id);
    restart_hand_swing();
}

static void toggle_lever_at(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (id != BLOCK_LEVER) return;
    flat_set_meta_raw(x, y, z, flat_get_meta(x, y, z) ^ 8);
    redstone_update_near(x, y, z);
    if (g_mp_connected) pex_net_send_block_action(PEX_BLOCK_PLACE, x, y, z, 0, id);
    restart_hand_swing();
}

static void add_pressure_plate_timer(int x, int y, int z) {
    int slot = -1;
    for (int i = 0; i < MAX_PRESSURE_PLATE_TIMERS; i++) {
        if (g_pressure_plate_timers[i].active && g_pressure_plate_timers[i].x == x &&
            g_pressure_plate_timers[i].y == y && g_pressure_plate_timers[i].z == z) {
            slot = i;
            break;
        }
        if (slot < 0 && !g_pressure_plate_timers[i].active) slot = i;
    }
    if (slot < 0) slot = 0;
    g_pressure_plate_timers[slot].active = 1;
    g_pressure_plate_timers[slot].x = x;
    g_pressure_plate_timers[slot].y = y;
    g_pressure_plate_timers[slot].z = z;
    g_pressure_plate_timers[slot].ticks_left = 20;
}

static int player_over_pressure_plate_at(int x, int y, int z) {
    float feet = g_player_y - 1.62f;
    float px0 = g_player_x - 0.30f, px1 = g_player_x + 0.30f;
    float pz0 = g_player_z - 0.30f, pz1 = g_player_z + 0.30f;
    float py0 = feet, py1 = feet + 0.25f;
    float e = 2.0f / 16.0f;
    float x0 = (float)x + e, x1 = (float)(x + 1) - e;
    float z0 = (float)z + e, z1 = (float)(z + 1) - e;
    return px1 > x0 && px0 < x1 && pz1 > z0 && pz0 < z1 && py1 >= (float)y && py0 <= (float)y + 0.25f;
}

static void update_buttons_and_pressure_plates(void) {
    for (int i = 0; i < MAX_BUTTON_TIMERS; i++) {
        ButtonTimer *bt = &g_button_timers[i];
        if (!bt->active) continue;
        if (flat_get_block(bt->x, bt->y, bt->z) != BLOCK_STONE_BUTTON) { bt->active = 0; continue; }
        if (--bt->ticks_left <= 0) {
            int meta = flat_get_meta(bt->x, bt->y, bt->z);
            if (meta & 8) {
                flat_set_meta_raw(bt->x, bt->y, bt->z, meta & 7);
                redstone_update_near(bt->x, bt->y, bt->z);
                if (g_mp_connected) pex_net_send_block_action(PEX_BLOCK_PLACE, bt->x, bt->y, bt->z, 0, BLOCK_STONE_BUTTON);
            }
            bt->active = 0;
        }
    }

    int range = 8;
    int px = (int)floorf(g_player_x);
    int pz = (int)floorf(g_player_z);
    int min_x = px - range, max_x = px + range;
    int min_z = pz - range, max_z = pz + range;
    if (min_x < g_flat_world_origin_x) min_x = g_flat_world_origin_x;
    if (max_x >= g_flat_world_origin_x + FLAT_WORLD_SIZE) max_x = g_flat_world_origin_x + FLAT_WORLD_SIZE - 1;
    if (min_z < g_flat_world_origin_z) min_z = g_flat_world_origin_z;
    if (max_z >= g_flat_world_origin_z + FLAT_WORLD_SIZE) max_z = g_flat_world_origin_z + FLAT_WORLD_SIZE - 1;

    int fy = (int)floorf(g_player_y - 1.62f);
    for (int y = fy - 1; y <= fy + 1; y++) {
        if (y < FLAT_WORLD_Y_MIN || y > FLAT_WORLD_Y_MAX) continue;
        for (int z = min_z; z <= max_z; z++) for (int x = min_x; x <= max_x; x++) {
            int id = flat_get_block(x, y, z);
            if (!block_is_pressure_plate(id)) continue;
            if (player_over_pressure_plate_at(x, y, z)) {
                if ((flat_get_meta(x, y, z) & 1) == 0) {
                    flat_set_meta_raw(x, y, z, 1);
                    redstone_update_near(x, y, z);
                    if (g_mp_connected) pex_net_send_block_action(PEX_BLOCK_PLACE, x, y, z, 0, id);
                }
                add_pressure_plate_timer(x, y, z);
            }
        }
    }

    for (int i = 0; i < MAX_PRESSURE_PLATE_TIMERS; i++) {
        PressurePlateTimer *pt = &g_pressure_plate_timers[i];
        if (!pt->active) continue;
        int id = flat_get_block(pt->x, pt->y, pt->z);
        if (!block_is_pressure_plate(id)) { pt->active = 0; continue; }
        if (player_over_pressure_plate_at(pt->x, pt->y, pt->z)) {
            pt->ticks_left = 20;
            if ((flat_get_meta(pt->x, pt->y, pt->z) & 1) == 0) {
                flat_set_meta_raw(pt->x, pt->y, pt->z, 1);
                redstone_update_near(pt->x, pt->y, pt->z);
                if (g_mp_connected) pex_net_send_block_action(PEX_BLOCK_PLACE, pt->x, pt->y, pt->z, 0, id);
            }
            continue;
        }
        if (--pt->ticks_left <= 0) {
            if (flat_get_meta(pt->x, pt->y, pt->z) & 1) {
                flat_set_meta_raw(pt->x, pt->y, pt->z, 0);
                redstone_update_near(pt->x, pt->y, pt->z);
                if (g_mp_connected) pex_net_send_block_action(PEX_BLOCK_PLACE, pt->x, pt->y, pt->z, 0, id);
            }
            pt->active = 0;
        }
    }
}

static int cactus_can_stay_at(int x, int y, int z) {
    int below = flat_get_block(x, y - 1, z);
    if (below != BLOCK_SAND && below != BLOCK_CACTUS) return 0;
    if (flat_get_block(x + 1, y, z) != 0) return 0;
    if (flat_get_block(x - 1, y, z) != 0) return 0;
    if (flat_get_block(x, y, z + 1) != 0) return 0;
    if (flat_get_block(x, y, z - 1) != 0) return 0;
    return 1;
}

static int reeds_can_stay_at(int x, int y, int z) {
    int below = flat_get_block(x, y - 1, z);
    if (below == BLOCK_REEDS) return 1;
    if (below != BLOCK_GRASS && below != BLOCK_DIRT && below != BLOCK_SAND) return 0;
    return block_is_water(flat_get_block(x + 1, y - 1, z)) || block_is_water(flat_get_block(x - 1, y - 1, z)) ||
           block_is_water(flat_get_block(x, y - 1, z + 1)) || block_is_water(flat_get_block(x, y - 1, z - 1));
}

static void drop_and_clear_block_no_particles(int x, int y, int z, int drop_ok) {
    int id = flat_get_block(x, y, z);
    if (id == 0) return;
    flat_set_block(x, y, z, 0);
    int drop = block_drop_id(id);
    if (drop_ok && drop > 0) spawn_item_stack((float)x + 0.5f, (float)y + 0.7f, (float)z + 0.5f, make_stack(drop, 1, 0), 1);
}

static void unsupported_block_neighbor_cleanup(int x, int y, int z) {
    int above = flat_get_block(x, y + 1, z);
    if (block_is_door_id(above) && door_meta_is_upper(flat_get_meta(x, y + 1, z))) {
        door_break_at(x, y + 1, z, 1);
    }

    /* Vertical stacks that depend on their root/support. */
    for (int yy = y + 1; yy <= FLAT_WORLD_Y_MAX; yy++) {
        int id = flat_get_block(x, yy, z);
        if (id == BLOCK_CACTUS) {
            if (!cactus_can_stay_at(x, yy, z)) drop_and_clear_block_no_particles(x, yy, z, 1);
            else break;
        } else if (id == BLOCK_REEDS) {
            if (!reeds_can_stay_at(x, yy, z)) drop_and_clear_block_no_particles(x, yy, z, 1);
            else break;
        } else break;
    }

    const int dirs[6][3] = {{0,1,0},{0,-1,0},{1,0,0},{-1,0,0},{0,0,1},{0,0,-1}};
    for (int i = 0; i < 6; i++) {
        int nx = x + dirs[i][0], ny = y + dirs[i][1], nz = z + dirs[i][2];
        int id = flat_get_block(nx, ny, nz);
        if (id == 0) continue;

        if (id == BLOCK_SNOW_LAYER || id == BLOCK_CROPS || id == BLOCK_SAPLING ||
            id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE ||
            id == BLOCK_BROWN_MUSHROOM || id == BLOCK_RED_MUSHROOM ||
            block_is_pressure_plate(id) || id == BLOCK_RAILS || id == BLOCK_REDSTONE_WIRE) {
            int below = flat_get_block(nx, ny - 1, nz);
            int ok = below != 0 && !block_is_liquid(below);
            if (id == BLOCK_CROPS) ok = (below == BLOCK_FARMLAND);
            if (!ok) drop_and_clear_block_no_particles(nx, ny, nz, 1);
            continue;
        }
        if (id == BLOCK_TORCH || id == BLOCK_REDSTONE_TORCH_OFF || id == BLOCK_REDSTONE_TORCH_ON) {
            int below = flat_get_block(nx, ny - 1, nz);
            if (below == 0 || block_is_liquid(below)) drop_and_clear_block_no_particles(nx, ny, nz, 1);
            continue;
        }
        if (id == BLOCK_LADDER || id == BLOCK_WALL_SIGN || block_is_button_or_lever(id)) {
            int m = flat_get_meta(nx, ny, nz) & 7;
            if (id == BLOCK_LADDER || id == BLOCK_WALL_SIGN) {
                int ok = 0;
                if (m == 2) ok = flat_block_occludes_for_support(flat_get_block(nx, ny, nz + 1));
                else if (m == 3) ok = flat_block_occludes_for_support(flat_get_block(nx, ny, nz - 1));
                else if (m == 4) ok = flat_block_occludes_for_support(flat_get_block(nx + 1, ny, nz));
                else if (m == 5) ok = flat_block_occludes_for_support(flat_get_block(nx - 1, ny, nz));
                if (!ok) drop_and_clear_block_no_particles(nx, ny, nz, 1);
            } else if (id == BLOCK_LEVER) {
                if (!lever_can_stay_with_meta(nx, ny, nz, m)) drop_and_clear_block_no_particles(nx, ny, nz, 1);
            } else if (!block_supports_attached_side(nx, ny, nz, m)) {
                drop_and_clear_block_no_particles(nx, ny, nz, 1);
            }
            continue;
        }
    }
}
static void break_target_block(void) {
    int id = flat_get_block(g_break_x, g_break_y, g_break_z);
    if (id == 0 || id == BLOCK_BEDROCK) return;

    if (block_is_door_id(id)) {
        if (g_mp_connected) {
            pex_net_send_player_action(PEX_ACTION_BREAK, g_break_x, g_break_y, g_break_z, g_break_face, id);
            pex_net_send_block_action(PEX_BLOCK_BREAK, g_break_x, g_break_y, g_break_z, g_break_face, 0);
            flat_set_block(g_break_x, g_break_y, g_break_z, 0);
        } else {
            door_break_at(g_break_x, g_break_y, g_break_z, held_item_can_harvest_drop_for_block(id));
            redstone_update_near(g_break_x, g_break_y, g_break_z);
        }
        return;
    }

    /* Source EffectRenderer.func_1186_a: spawn destroy particles while the
       block still exists, then remove it. */
    spawn_block_destroy_particles(g_break_x, g_break_y, g_break_z, id);

    if (g_mp_connected) {
        pex_net_send_player_action(PEX_ACTION_BREAK, g_break_x, g_break_y, g_break_z, g_break_face, id);
        flat_set_block(g_break_x, g_break_y, g_break_z, 0);
        pex_net_send_block_action(PEX_BLOCK_BREAK, g_break_x, g_break_y, g_break_z, g_break_face, 0);
        return;
    }

    flat_begin_persistent_edit();
    if (id == BLOCK_CHEST) {
        chest_drop_contents_and_remove_at(g_break_x, g_break_y, g_break_z);
    } else if (id == BLOCK_FURNACE || id == BLOCK_FURNACE_LIT) {
        furnace_drop_contents_and_remove_at(g_break_x, g_break_y, g_break_z);
    }
    if (id == BLOCK_ICE) {
        int below = flat_get_block(g_break_x, g_break_y - 1, g_break_z);
        if (flat_block_is_solid_for_collision(below) || block_is_liquid(below)) {
            flat_place_fluid_source(g_break_x, g_break_y, g_break_z, BLOCK_WATER);
        } else {
            flat_set_block(g_break_x, g_break_y, g_break_z, 0);
        }
    } else {
        flat_set_block(g_break_x, g_break_y, g_break_z, 0);
    }
    unsupported_block_neighbor_cleanup(g_break_x, g_break_y, g_break_z);
    flat_end_persistent_edit();
    redstone_update_near(g_break_x, g_break_y, g_break_z);
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
        if (g_mp_connected) {
            int stage = (int)(g_break_damage * 10.0f);
            if (stage < 0) stage = 0;
            if (stage > 9) stage = 9;
            pex_net_send_player_action_progress(PEX_ACTION_MINE_HIT, g_break_x, g_break_y, g_break_z, g_break_face, id, stage);
            g_last_sent_mine_stage = stage;
        }
        g_last_hit_particle_x = g_break_x; g_last_hit_particle_y = g_break_y; g_last_hit_particle_z = g_break_z;
        g_last_hit_particle_face = g_break_face; g_last_hit_particle_tick = g_ingame_ticks;
    }
    float rel = block_relative_strength(id);
    /* Classic-style water penalty: only slow mining when the player's head is
       actually underwater and the target block is touching water.  This keeps
       normal shoreline mining unchanged but makes true underwater mining slower. */
    if (flat_player_head_in_water() && flat_block_is_underwater_target(g_break_x, g_break_y, g_break_z)) {
        rel *= 0.20f;
    }
    g_break_damage += rel;
    if (g_mp_connected) {
        int stage = (int)(g_break_damage * 10.0f);
        if (stage < 0) stage = 0;
        if (stage > 9) stage = 9;
        if (stage != g_last_sent_mine_stage) {
            pex_net_send_player_action_progress(PEX_ACTION_MINE_HIT, g_break_x, g_break_y, g_break_z, g_break_face, id, stage);
            g_last_sent_mine_stage = stage;
        }
    }
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
    if (id == ITEM_DOOR_WOOD || id == ITEM_DOOR_IRON || id == ITEM_REED || id == ITEM_SIGN ||
        id == ITEM_REDSTONE || id == ITEM_SEEDS) return 1;
    if (block_is_liquid(id)) return 0; /* water/lava need bucket logic, not block mining/placing */
    if (id >= 1 && id <= 91) return 1;
    return 0;
}

static int held_is_hoe_item(int id) {
    return id == ITEM_HOE_WOOD || id == ITEM_HOE_STONE || id == ITEM_HOE_IRON ||
           id == ITEM_HOE_DIAMOND || id == ITEM_HOE_GOLD;
}

static int item_food_heal_amount(int id) {
    if (id == ITEM_APPLE_RED) return 4;
    if (id == ITEM_BREAD) return 5;
    if (id == ITEM_BOWL_SOUP) return 10;
    if (id == ITEM_PORK_RAW) return 3;
    if (id == ITEM_PORK_COOKED) return 8;
    if (id == ITEM_FISH_RAW) return 2;
    if (id == ITEM_FISH_COOKED) return 5;
    if (id == ITEM_APPLE_GOLD) return 20;
    return 0;
}

static void consume_held_stack_one(ItemStack *held, int replacement_id) {
    if (!held || stack_empty(held)) return;
    if (held->count <= 1) {
        if (replacement_id > 0) *held = make_stack(replacement_id, 1, 0);
        else stack_clear(held);
    } else {
        held->count--;
        if (replacement_id > 0) inventory_add_stack(make_stack(replacement_id, 1, 0));
    }
}

static void hit_adjacent_cell(const FlatRayHit *hit, int *px, int *py, int *pz) {
    *px = hit->bx; *py = hit->by; *pz = hit->bz;
    if (hit->face == 0) (*py)--;
    else if (hit->face == 1) (*py)++;
    else if (hit->face == 2) (*pz)--;
    else if (hit->face == 3) (*pz)++;
    else if (hit->face == 4) (*px)--;
    else if (hit->face == 5) (*px)++;
}

static int try_use_nonblock_item(ItemStack *held, const FlatRayHit *hit, int target_id) {
    if (!held || stack_empty(held) || !hit || !hit->hit) return 0;

    int id = held->id;
    int px, py, pz;
    hit_adjacent_cell(hit, &px, &py, &pz);

    int heal = item_food_heal_amount(id);
    if (heal > 0) {
        if (g_player_health >= 20) return 0;
        g_player_health += heal;
        if (g_player_health > 20) g_player_health = 20;
        consume_held_stack_one(held, id == ITEM_BOWL_SOUP ? ITEM_BOWL_EMPTY : 0);
        g_save_dirty = 1;
        restart_hand_swing();
        return 1;
    }

    if (held_is_hoe_item(id)) {
        if (hit->face != 1) return 0;
        if ((target_id == BLOCK_GRASS || target_id == BLOCK_DIRT) && flat_get_block(hit->bx, hit->by + 1, hit->bz) == 0) {
            if (!g_mp_connected) flat_begin_persistent_edit();
            flat_set_block(hit->bx, hit->by, hit->bz, BLOCK_FARMLAND);
            if (!g_mp_connected) flat_end_persistent_edit();
            if (g_mp_connected) pex_net_send_block_action(PEX_BLOCK_PLACE, hit->bx, hit->by, hit->bz, hit->face, BLOCK_FARMLAND);
            else g_save_dirty = 1;
            restart_hand_swing();
            return 1;
        }
        return 0;
    }

    if (id == ITEM_FLINT_AND_IRON) {
        if (flat_in_bounds(px, py, pz) && flat_get_block(px, py, pz) == 0 && !flat_block_intersects_player(px, py, pz)) {
            if (!g_mp_connected) flat_begin_persistent_edit();
            flat_set_block(px, py, pz, BLOCK_FIRE);
            if (!g_mp_connected) flat_end_persistent_edit();
            if (g_mp_connected) pex_net_send_block_action(PEX_BLOCK_PLACE, px, py, pz, hit->face, BLOCK_FIRE);
            else g_save_dirty = 1;
            restart_hand_swing();
            return 1;
        }
        return 0;
    }

    if (id == ITEM_BUCKET_EMPTY) {
        int sx, sy, sz;
        if (flat_raycast_liquid_source(&sx, &sy, &sz)) {
            int is_water = block_is_water(flat_get_block(sx, sy, sz));
            flat_begin_persistent_edit();
            flat_set_block(sx, sy, sz, 0);
            flat_end_persistent_edit();
            consume_held_stack_one(held, is_water ? ITEM_BUCKET_WATER : ITEM_BUCKET_LAVA);
            if (g_mp_connected) {
                pex_net_send_block_action(PEX_BLOCK_BUCKET_PICKUP, sx, sy, sz, 0, is_water ? BLOCK_WATER : BLOCK_LAVA);
                pex_net_send_player_action(PEX_ACTION_SWING, sx, sy, sz, 0, is_water ? BLOCK_WATER : BLOCK_LAVA);
            } else {
                g_save_dirty = 1;
            }
            restart_hand_swing();
            return 1;
        }
        return 0;
    }

    if (id == ITEM_BUCKET_WATER || id == ITEM_BUCKET_LAVA) {
        if (!flat_in_bounds(px, py, pz)) return 0;
        int existing = flat_get_block(px, py, pz);
        if (existing == 0 || !flat_block_is_solid_for_collision(existing) || block_is_liquid(existing)) {
            int fluid = id == ITEM_BUCKET_WATER ? BLOCK_WATER : BLOCK_LAVA;
            flat_begin_persistent_edit();
            flat_place_fluid_source(px, py, pz, fluid);
            flat_end_persistent_edit();
            consume_held_stack_one(held, ITEM_BUCKET_EMPTY);
            if (g_mp_connected) {
                pex_net_send_block_action(PEX_BLOCK_BUCKET_PLACE, px, py, pz, hit->face, fluid);
                pex_net_send_player_action(PEX_ACTION_PLACE, px, py, pz, hit->face, fluid);
            } else {
                g_save_dirty = 1;
            }
            restart_hand_swing();
            return 1;
        }
        return 0;
    }

    return 0;
}

static void ingame_right_click(void) {
    if (g_screen != SCREEN_INGAME) return;

    FlatRayHit hit = flat_raycast();
    if (!hit.hit) return;

    int target_id = flat_get_block(hit.bx, hit.by, hit.bz);
    int sneaking = key_down_vk(g_opts.keys[5]);

    if (!sneaking) {
        if (target_id == BLOCK_CRAFTING_TABLE) {
            set_screen(SCREEN_WORKBENCH);
            return;
        }
        if (target_id == BLOCK_FURNACE || target_id == BLOCK_FURNACE_LIT) {
            furnace_open_at(hit.bx, hit.by, hit.bz);
            return;
        }
        if (target_id == BLOCK_CHEST) {
            chest_open_at(hit.bx, hit.by, hit.bz);
            return;
        }
        if (block_is_door_id(target_id)) {
            door_toggle_at(hit.bx, hit.by, hit.bz);
            return;
        }
        if (target_id == BLOCK_STONE_BUTTON) {
            press_button_at(hit.bx, hit.by, hit.bz);
            return;
        }
        if (target_id == BLOCK_LEVER) {
            toggle_lever_at(hit.bx, hit.by, hit.bz);
            return;
        }
    }

    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    if (stack_empty(held)) return;
    if (try_use_nonblock_item(held, &hit, target_id)) return;
    if (!item_is_placeable_block_id(held->id)) return;

    int px = hit.bx;
    int py = hit.by;
    int pz = hit.bz;

    /* Beta ItemBlock: clicking a snow layer replaces the layer instead of
       placing against a fake full cube. */
    if (target_id != BLOCK_SNOW_LAYER) {
        if (hit.face == 0) py--;
        else if (hit.face == 1) py++;
        else if (hit.face == 2) pz--;
        else if (hit.face == 3) pz++;
        else if (hit.face == 4) px--;
        else if (hit.face == 5) px++;
    }

    if (py > FLAT_WORLD_Y_MAX) {
        hud_add_chat("Height limit for building is 256 blocks.");
        return;
    }
    if (py < FLAT_WORLD_Y_MIN ||
        px < g_flat_world_origin_x || px >= g_flat_world_origin_x + FLAT_WORLD_SIZE ||
        pz < g_flat_world_origin_z || pz >= g_flat_world_origin_z + FLAT_WORLD_SIZE) return;

    if (held->id == ITEM_DOOR_WOOD || held->id == ITEM_DOOR_IRON) {
        if (!place_door_from_item(held->id, px, py, pz)) return;
        redstone_update_near(px, py, pz);
        if (g_mp_connected) {
            pex_net_send_player_action(PEX_ACTION_PLACE, px, py, pz, hit.face, held->id);
            pex_net_send_block_action(PEX_BLOCK_PLACE, px, py, pz, hit.face, held->id);
        }
        if (--held->count <= 0) stack_clear(held);
        if (!g_mp_connected) g_save_dirty = 1;
        restart_hand_swing();
        return;
    }

    int place_id = held->id;
    if (place_id == ITEM_REED) place_id = BLOCK_REEDS;
    if (place_id == ITEM_REDSTONE) place_id = BLOCK_REDSTONE_WIRE;
    if (place_id == ITEM_SEEDS) place_id = BLOCK_CROPS;
    if (place_id == ITEM_SIGN) place_id = (hit.face == 1) ? BLOCK_SIGN_POST : BLOCK_WALL_SIGN;

    int place_existing = flat_get_block(px, py, pz);
    if (place_existing != 0 && !block_is_liquid(place_existing) && place_existing != BLOCK_SNOW_LAYER) return;
    if (place_id != BLOCK_LADDER && place_id != BLOCK_SIGN_POST && place_id != BLOCK_WALL_SIGN && flat_block_intersects_player(px, py, pz)) return;
    if (place_id == BLOCK_CHEST && !chest_can_place_at(px, py, pz)) return;
    if (place_id == BLOCK_LADDER && !ladder_can_attach_to_face(hit.face)) return;
    if (place_id == BLOCK_WALL_SIGN && !ladder_can_attach_to_face(hit.face)) return;
    if (place_id == BLOCK_STONE_BUTTON) {
        int bm = side_meta_from_placement_face(hit.face);
        if (bm == 0 || !block_supports_attached_side(px, py, pz, bm)) return;
    }
    if (place_id == BLOCK_LEVER) {
        int lm = lever_meta_from_placement_face(hit.face);
        if (lm == 0 || !lever_can_stay_with_meta(px, py, pz, lm)) return;
    }
    if (block_is_pressure_plate(place_id) && !flat_block_occludes_for_support(flat_get_block(px, py - 1, pz))) return;
    if (place_id == BLOCK_CACTUS && !cactus_can_stay_at(px, py, pz)) return;
    if (place_id == BLOCK_REEDS && !reeds_can_stay_at(px, py, pz)) return;
    if (place_id == BLOCK_CROPS && flat_get_block(px, py - 1, pz) != BLOCK_FARMLAND) return;

    if (!g_mp_connected) flat_begin_persistent_edit();
    flat_set_block(px, py, pz, place_id);
    if (place_id == BLOCK_CHEST) chest_on_block_placed(px, py, pz);
    if (place_id == BLOCK_FURNACE || place_id == BLOCK_FURNACE_LIT) {
        furnace_on_block_placed(px, py, pz);
        flat_set_meta_raw(px, py, pz, furnace_facing_from_yaw());
    }
    if (place_id == BLOCK_LADDER || place_id == BLOCK_WALL_SIGN) flat_set_meta_raw(px, py, pz, hit.face);
    if (place_id == BLOCK_STONE_BUTTON) flat_set_meta_raw(px, py, pz, side_meta_from_placement_face(hit.face));
    if (place_id == BLOCK_LEVER) flat_set_meta_raw(px, py, pz, lever_meta_from_placement_face(hit.face));
    if (place_id == BLOCK_SIGN_POST) flat_set_meta_raw(px, py, pz, door_direction_from_yaw() & 3);
    if (place_id == BLOCK_WOOD_STAIRS || place_id == BLOCK_COBBLE_STAIRS) flat_set_meta_raw(px, py, pz, stair_direction_from_yaw());
    if (!g_mp_connected) flat_end_persistent_edit();
    if (place_id == BLOCK_REDSTONE_WIRE || place_id == BLOCK_STONE_BUTTON || place_id == BLOCK_LEVER ||
        block_is_pressure_plate(place_id) || block_is_door_id(place_id)) {
        redstone_update_near(px, py, pz);
    }
    if (g_mp_connected) {
        pex_net_send_player_action(PEX_ACTION_PLACE, px, py, pz, hit.face, place_id);
        pex_net_send_block_action(PEX_BLOCK_PLACE, px, py, pz, hit.face, place_id);
    }
    if (--held->count <= 0) stack_clear(held);
    if (!g_mp_connected) g_save_dirty = 1;
    restart_hand_swing();
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
            if (g_mp_connected) pex_net_send_drop_item(g_inventory[i]);
            else spawn_item_stack(g_player_x, g_player_y - 0.30f, g_player_z, g_inventory[i], 1);
            stack_clear(&g_inventory[i]);
        }
    }
    if (!stack_empty(&g_carried_stack)) {
        if (g_mp_connected) pex_net_send_drop_item(g_carried_stack);
        else spawn_item_stack(g_player_x, g_player_y - 0.30f, g_player_z, g_carried_stack, 1);
        stack_clear(&g_carried_stack);
    }
    memset(g_craft_grid, 0, sizeof(g_craft_grid));
    if (!g_mp_connected) g_save_dirty = 1;
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
        if (g_mp_connected) {
            int in_world = (g_screen == SCREEN_INGAME || g_screen == SCREEN_INVENTORY ||
                            g_screen == SCREEN_WORKBENCH || g_screen == SCREEN_FURNACE ||
                            g_screen == SCREEN_CHEST || g_screen == SCREEN_CHAT);
            if (!g_player_dead && in_world && e->pickup_delay <= 0 && dropped_item_touches_player(e)) {
                pex_net_send_pickup_drop(e->net_id);
            }
            continue;
        }
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


static int flat_spawn_has_open_sky(int x, int z, int foot_y) {
    /* Reject cave/overhang spawn candidates.  The old test only required three
       air blocks above the ground, so a cave floor under a stone roof could win.
       For new/respawn placement, prefer a column with clear sky above the
       player's body. */
    for (int y = foot_y + 3; y <= FLAT_WORLD_Y_MAX; y++) {
        int id = flat_get_block(x, y, z);
        if (id == 0 || id == BLOCK_SNOW_LAYER || id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE ||
            id == BLOCK_BROWN_MUSHROOM || id == BLOCK_RED_MUSHROOM || id == BLOCK_TORCH ||
            id == BLOCK_REDSTONE_TORCH_OFF || id == BLOCK_REDSTONE_TORCH_ON) {
            continue;
        }
        return 0;
    }
    return 1;
}

static int flat_column_spawn_y_ex(int x, int z, int require_sky) {
    if (x < g_flat_world_origin_x || x >= g_flat_world_origin_x + FLAT_WORLD_SIZE || z < g_flat_world_origin_z || z >= g_flat_world_origin_z + FLAT_WORLD_SIZE) return -9999;
    for (int y = FLAT_WORLD_Y_MAX - 3; y >= FLAT_WORLD_Y_MIN; y--) {
        int id = flat_get_block(x, y, z);
        if (!flat_solid_for_spawn(id)) continue;
        if (flat_get_block(x, y + 1, z) == 0 &&
            flat_get_block(x, y + 2, z) == 0 &&
            flat_get_block(x, y + 3, z) == 0) {
            int foot_y = y + 1;
            if (require_sky && !flat_spawn_has_open_sky(x, z, foot_y)) continue;
            return foot_y;
        }
    }
    return -9999;
}

static int flat_column_spawn_y(int x, int z) {
    return flat_column_spawn_y_ex(x, z, 1);
}

static int flat_find_safe_spawn_pass(float *out_x, float *out_y, float *out_z, int require_sky) {
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

                int sy = flat_column_spawn_y_ex(x, z, require_sky);
                if (sy == -9999) continue;

                int ground = flat_get_block(x, sy - 1, z);
                if (block_is_liquid(ground) || ground == BLOCK_LEAVES) continue;
                /* Prefer normal outdoor surface blocks; avoid ore/stone cliff ledges. */
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
    return 0;
}

static int flat_find_safe_spawn(float *out_x, float *out_y, float *out_z) {
    if (flat_find_safe_spawn_pass(out_x, out_y, out_z, 1)) return 1;

    /* Last-resort fallback keeps old saves/world types playable if the center
       area is completely roofed, but normal new worlds now choose open sky. */
    if (flat_find_safe_spawn_pass(out_x, out_y, out_z, 0)) return 1;

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

static int g_falling_wakeups_accum = 0;
static int g_falling_spawns_accum = 0;

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

static int falling_block_can_fall_through(int id) {
    return id == 0 || block_is_liquid(id) || id == BLOCK_SNOW_LAYER;
}

static int falling_block_aabb_collides(float x, float y, float z) {
    float minx = x - 0.49f, maxx = x + 0.49f;
    float miny = y - 0.49f, maxy = y + 0.49f;
    float minz = z - 0.49f, maxz = z + 0.49f;
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

static int spawn_falling_block_entity(int x, int y, int z, int block_id) {
    if (!flat_in_bounds(x, y, z) || !block_is_falling_type(block_id)) return 0;
    int slot = -1;
    for (int i = 0; i < MAX_FALLING_BLOCK_ENTITIES; i++) {
        if (!g_falling_blocks[i].active) { slot = i; break; }
    }
    if (slot < 0) return 0;
    FlatFallingBlock *fb = &g_falling_blocks[slot];
    memset(fb, 0, sizeof(*fb));
    fb->active = 1; /* reserve the slot before block removal wakes stacked sand */
    fb->net_id = slot + 1;
    fb->block_id = block_id;
    fb->x = fb->prev_x = (float)x + 0.5f;
    fb->y = fb->prev_y = (float)y + 0.5f;
    fb->z = fb->prev_z = (float)z + 0.5f;
    flat_set_block(x, y, z, 0);
    g_falling_spawns_accum++;
    return 1;
}

static void wake_falling_block_at(int x, int y, int z) {
    if (g_mp_connected) return;
    g_falling_wakeups_accum++;
    if (!flat_in_bounds(x, y, z) || y <= FLAT_WORLD_Y_MIN) return;
    int id = flat_get_block(x, y, z);
    if (!block_is_falling_type(id)) return;
    if (!falling_block_can_fall_through(flat_get_block(x, y - 1, z))) return;
    spawn_falling_block_entity(x, y, z, id);
}

static void wake_falling_blocks_around(int x, int y, int z) {
    if (g_mp_connected) return;
    /* Block updates wake gravity immediately.  There is no background scanner. */
    wake_falling_block_at(x, y, z);
    if (y < FLAT_WORLD_Y_MAX) wake_falling_block_at(x, y + 1, z);
}

static void finish_falling_block_entity(FlatFallingBlock *fb) {
    if (!fb || !fb->active) return;
    int bx = (int)floorf(fb->x);
    int by = (int)floorf(fb->y);
    int bz = (int)floorf(fb->z);
    int target = flat_get_block(bx, by, bz);
    if (flat_in_bounds(bx, by, bz) && falling_block_can_fall_through(target)) {
        flat_set_block(bx, by, bz, fb->block_id);
    } else {
        int drop = block_drop_id(fb->block_id);
        if (drop > 0) spawn_item_stack(fb->x, fb->y, fb->z, make_stack(drop, 1, 0), 1);
    }
    memset(fb, 0, sizeof(*fb));
}

static void update_falling_blocks(void) {
    /* Sand/gravel is event-driven only.  There is no background world scan here:
       placing sand/gravel or changing the block below it wakes gravity directly
       through flat_set_block()/flat_set_fluid(). */
    int active_count = 0;

    for (int i = 0; i < MAX_FALLING_BLOCK_ENTITIES; i++) {
        FlatFallingBlock *fb = &g_falling_blocks[i];
        if (!fb->active) continue;
        active_count++;
        fb->prev_x = fb->x;
        fb->prev_y = fb->y;
        fb->prev_z = fb->z;
        fb->my -= 0.04f;
        fb->x += fb->mx;
        fb->y += fb->my;
        fb->z += fb->mz;
        int on_ground = 0;
        if (falling_block_aabb_collides(fb->x, fb->y, fb->z)) {
            fb->x = fb->prev_x;
            fb->y = fb->prev_y;
            fb->z = fb->prev_z;
            if (fb->my < 0.0f) on_ground = 1;
            fb->mx *= 0.70f;
            fb->my *= -0.50f;
            fb->mz *= 0.70f;
        }
        fb->mx *= 0.98f;
        fb->my *= 0.98f;
        fb->mz *= 0.98f;
        fb->age++;
        if (on_ground) {
            finish_falling_block_entity(fb);
        } else if (fb->age > 100 || fb->y < -16.0f) {
            int drop = block_drop_id(fb->block_id);
            if (drop > 0) spawn_item_stack(fb->x, fb->y, fb->z, make_stack(drop, 1, 0), 1);
            memset(fb, 0, sizeof(*fb));
        }
    }

    g_prof_falling_active_last = active_count;
    g_prof_falling_cells_last = g_falling_wakeups_accum;
    g_prof_falling_spawns_last = g_falling_spawns_accum;
    g_falling_wakeups_accum = 0;
    g_falling_spawns_accum = 0;
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




/* ---- Beta-accurate fluid simulation (mirrors BlockFluids/BlockFlowing) ---- */
static const int FLUID_DX[4] = {-1, 1, 0, 0};
static const int FLUID_DZ[4] = {0, 0, -1, 1};
enum { FLUID_SIM_RADIUS = 16 };
enum { FLUID_TICK_MAX = (FLUID_SIM_RADIUS * 2 + 1) * (FLUID_SIM_RADIUS * 2 + 1) * FLAT_WORLD_HEIGHT };

typedef struct FluidTickCell {
    int x, y, z;
    unsigned char is_water;
} FluidTickCell;

static int fluid_same_family(int id, int is_water) {
    if (!block_is_liquid(id)) return 0;
    return block_is_water(id) ? is_water : !is_water;
}

static int fluid_cell_replaceable(int id, int is_water) {
    if (id == 0) return 1;
    if (fluid_same_family(id, is_water)) return 0;
    if (block_is_lava(id)) return 0;
    return !block_material_blocks_liquid(id);
}

static int fluid_flow_cost(int x, int y, int z, int is_water, int dist, int skip_dir) {
    int best = 1000;
    for (int d = 0; d < 4; d++) {
        if ((d == 0 && skip_dir == 1) || (d == 1 && skip_dir == 0) ||
            (d == 2 && skip_dir == 3) || (d == 3 && skip_dir == 2)) continue;
        int nx = x + FLUID_DX[d], nz = z + FLUID_DZ[d];
        int n = flat_get_block(nx, y, nz);
        if (block_material_blocks_liquid(n)) continue;
        if (fluid_same_family(n, is_water) && flat_get_level(nx, y, nz) == 0) continue;
        int below = flat_get_block(nx, y - 1, nz);
        if (!block_material_blocks_liquid(below)) return dist;
        if (dist >= 4) continue;
        int c = fluid_flow_cost(nx, y, nz, is_water, dist + 1, d);
        if (c < best) best = c;
    }
    return best;
}

static void fluid_optimal_dirs(int x, int y, int z, int is_water, int out[4]) {
    int cost[4];
    int minc = 1000000;
    for (int d = 0; d < 4; d++) {
        out[d] = 0;
        int nx = x + FLUID_DX[d], nz = z + FLUID_DZ[d];
        int n = flat_get_block(nx, y, nz);
        if (block_material_blocks_liquid(n) ||
            (fluid_same_family(n, is_water) && flat_get_level(nx, y, nz) == 0)) {
            cost[d] = -1;
            continue;
        }
        int below = flat_get_block(nx, y - 1, nz);
        if (!block_material_blocks_liquid(below)) cost[d] = 0;
        else cost[d] = fluid_flow_cost(nx, y, nz, is_water, 1, d);
        if (cost[d] < minc) minc = cost[d];
    }
    for (int d = 0; d < 4; d++) if (cost[d] >= 0 && cost[d] == minc) out[d] = 1;
}

static int fluid_neighbor_min_level(int x, int y, int z, int is_water, int current_min, int *source_count) {
    int level = flat_get_level(x, y, z);
    if (!fluid_same_family(flat_get_block(x, y, z), is_water)) return current_min;
    if (level == 0 && source_count) (*source_count)++;
    if (level >= 8) level = 0;
    return (current_min >= 0 && level >= current_min) ? current_min : level;
}

static void fluid_spread_to(int x, int y, int z, int is_water, int level) {
    if (!fluid_cell_replaceable(flat_get_block(x, y, z), is_water)) return;
    int old = flat_get_block(x, y, z);
    if (old > 0 && !block_is_liquid(old)) {
        if (is_water) {
            int drop = block_drop_id(old);
            if (drop > 0) spawn_item_stack((float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, make_stack(drop, 1, 0), 1);
        }
    }
    flat_set_fluid(x, y, z, fluid_flow_id(is_water), level);
}

static void fluid_set_still_if_stable(int x, int y, int z, int is_water, int level) {
    flat_set_fluid(x, y, z, fluid_still_id(is_water), level);
}

static int fluid_effective_level_at(int x, int y, int z, int is_water) {
    if (!fluid_same_family(flat_get_block(x, y, z), is_water)) return -1;
    int level = flat_get_level(x, y, z);
    if (level >= 8) level = 0;
    return level;
}

static int fluid_side_is_exposed_for_flow_vector(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (block_is_liquid(id)) return 0;
    if (id == BLOCK_ICE) return 0;
    return !block_material_blocks_liquid(id);
}

static void fluid_flow_vector_at(int x, int y, int z, int is_water, float *vx, float *vy, float *vz) {
    float ax = 0.0f, ay = 0.0f, az = 0.0f;
    int current = fluid_effective_level_at(x, y, z, is_water);
    if (current < 0) return;

    for (int d = 0; d < 4; d++) {
        int nx = x + FLUID_DX[d];
        int nz = z + FLUID_DZ[d];
        int nl = fluid_effective_level_at(nx, y, nz, is_water);
        if (nl < 0) {
            if (!block_material_blocks_liquid(flat_get_block(nx, y, nz))) {
                nl = fluid_effective_level_at(nx, y - 1, nz, is_water);
                if (nl >= 0) {
                    int diff = nl - (current - 8);
                    ax += (float)FLUID_DX[d] * (float)diff;
                    az += (float)FLUID_DZ[d] * (float)diff;
                }
            }
        } else {
            int diff = nl - current;
            ax += (float)FLUID_DX[d] * (float)diff;
            az += (float)FLUID_DZ[d] * (float)diff;
        }
    }

    if (flat_get_level(x, y, z) >= 8) {
        int edge =
            fluid_side_is_exposed_for_flow_vector(x, y, z - 1) ||
            fluid_side_is_exposed_for_flow_vector(x, y, z + 1) ||
            fluid_side_is_exposed_for_flow_vector(x - 1, y, z) ||
            fluid_side_is_exposed_for_flow_vector(x + 1, y, z) ||
            fluid_side_is_exposed_for_flow_vector(x, y + 1, z - 1) ||
            fluid_side_is_exposed_for_flow_vector(x, y + 1, z + 1) ||
            fluid_side_is_exposed_for_flow_vector(x - 1, y + 1, z) ||
            fluid_side_is_exposed_for_flow_vector(x + 1, y + 1, z);
        if (edge) {
            float len = sqrtf(ax * ax + ay * ay + az * az);
            if (len > 0.0001f) { ax /= len; ay /= len; az /= len; }
            ay -= 6.0f;
        }
    }

    float len = sqrtf(ax * ax + ay * ay + az * az);
    if (len > 0.0001f) {
        *vx += ax / len;
        *vy += ay / len;
        *vz += az / len;
    }
}

static void apply_player_fluid_velocity(int is_water) {
    float vx = 0.0f, vy = 0.0f, vz = 0.0f;
    float min_x = g_player_x - 0.30f;
    float max_x = g_player_x + 0.30f;
    float min_y = g_player_y - 1.62f - 0.40f;
    float max_y = g_player_y;
    float min_z = g_player_z - 0.30f;
    float max_z = g_player_z + 0.30f;

    int x0 = (int)floorf(min_x), x1 = (int)floorf(max_x + 1.0f);
    int y0 = (int)floorf(min_y), y1 = (int)floorf(max_y + 1.0f);
    int z0 = (int)floorf(min_z), z1 = (int)floorf(max_z + 1.0f);
    for (int y = y0; y < y1; y++) {
        for (int z = z0; z < z1; z++) {
            for (int x = x0; x < x1; x++) {
                if (!fluid_same_family(flat_get_block(x, y, z), is_water)) continue;
                float surface = (float)(y + 1) - fluid_decay_height(flat_get_level(x, y, z));
                if (max_y < surface) continue;
                fluid_flow_vector_at(x, y, z, is_water, &vx, &vy, &vz);
            }
        }
    }

    float len = sqrtf(vx * vx + vy * vy + vz * vz);
    if (len > 0.0001f) {
        const float push = 0.004f;
        g_player_motion_x += (vx / len) * push;
        g_player_motion_y += (vy / len) * push;
        g_player_motion_z += (vz / len) * push;
    }
}

static void update_liquids(void) {
    if (stream_generation_active()) return;

    int tick_water = (g_ingame_ticks % 5) == 0;
    int tick_lava = (g_ingame_ticks % 30) == 0;
    if (!tick_water && !tick_lava) return;

    int range = active_world_sim_radius();
    if (range > FLUID_SIM_RADIUS) range = FLUID_SIM_RADIUS;

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

    static FluidTickCell fluid_ticks[FLUID_TICK_MAX];
    int fluid_tick_count = 0;

    for (int y = FLAT_WORLD_Y_MAX - 1; y >= FLAT_WORLD_Y_MIN + 1; y--) {
        for (int z = min_z; z <= max_z; z++) {
            for (int x = min_x; x <= max_x; x++) {
                int id = flat_get_block(x, y, z);
                if (!block_is_flowing_liquid(id)) continue;
                int is_water = block_is_water(id);
                if ((is_water && !tick_water) || (!is_water && !tick_lava)) continue;
                if (fluid_tick_count < FLUID_TICK_MAX) {
                    fluid_ticks[fluid_tick_count].x = x;
                    fluid_ticks[fluid_tick_count].y = y;
                    fluid_ticks[fluid_tick_count].z = z;
                    fluid_ticks[fluid_tick_count].is_water = (unsigned char)is_water;
                    fluid_tick_count++;
                }
            }
        }
    }

    for (int i = 0; i < fluid_tick_count && changes_left > 0; i++) {
        int x = fluid_ticks[i].x;
        int y = fluid_ticks[i].y;
        int z = fluid_ticks[i].z;
        int is_water = fluid_ticks[i].is_water ? 1 : 0;
        int id = flat_get_block(x, y, z);
        if (!block_is_flowing_liquid(id) || !fluid_same_family(id, is_water)) continue;

        int decay = is_water ? 1 : 2;

        fluid_check_for_harden(x, y, z);
        id = flat_get_block(x, y, z);
        if (!block_is_flowing_liquid(id) || !fluid_same_family(id, is_water)) continue;

        int level = flat_get_level(x, y, z);
        if (level > 0) {
            int source_count = 0;
            int min_level = -100;
            min_level = fluid_neighbor_min_level(x - 1, y, z, is_water, min_level, &source_count);
            min_level = fluid_neighbor_min_level(x + 1, y, z, is_water, min_level, &source_count);
            min_level = fluid_neighbor_min_level(x, y, z - 1, is_water, min_level, &source_count);
            min_level = fluid_neighbor_min_level(x, y, z + 1, is_water, min_level, &source_count);

            int new_level = min_level + decay;
            if (new_level >= 8 || min_level < 0) new_level = -1;

            if (fluid_same_family(flat_get_block(x, y + 1, z), is_water)) {
                int above_level = flat_get_level(x, y + 1, z);
                new_level = (above_level >= 8) ? above_level : above_level + 8;
            }

            if (source_count >= 2 && is_water) {
                int source_below = flat_get_block(x, y - 1, z);
                if (block_material_blocks_liquid(source_below) ||
                    (block_is_water(source_below) && flat_get_level(x, y - 1, z) == 0)) {
                    new_level = 0;
                }
            }

            if (new_level != level) {
                if (new_level < 0) {
                    flat_set_block(x, y, z, 0);
                    moved = 1;
                    changes_left--;
                    continue;
                }
                flat_set_fluid(x, y, z, fluid_flow_id(is_water), new_level);
                wake_neighbor_liquids(x, y, z);
                level = new_level;
                moved = 1;
                changes_left--;
                if (changes_left <= 0) continue;
            } else {
                fluid_set_still_if_stable(x, y, z, is_water, level);
                moved = 1;
                changes_left--;
                if (changes_left <= 0) continue;
            }
        } else {
            fluid_set_still_if_stable(x, y, z, is_water, level);
            moved = 1;
            changes_left--;
            if (changes_left <= 0) continue;
        }

        int below = flat_get_block(x, y - 1, z);
        if (fluid_cell_replaceable(below, is_water)) {
            int fall_level = (level >= 8) ? level : level + 8;
            fluid_spread_to(x, y - 1, z, is_water, fall_level);
            moved = 1; changes_left--;
            continue;
        }

        int spread_level = level + decay;
        if (level >= 8) spread_level = 1;
        if (spread_level >= 8) continue;
        if (level != 0 && !block_material_blocks_liquid(below)) continue;

        int dirs[4];
        fluid_optimal_dirs(x, y, z, is_water, dirs);
        for (int d = 0; d < 4 && changes_left > 0; d++) {
            if (!dirs[d]) continue;
            int nx = x + FLUID_DX[d], nz = z + FLUID_DZ[d];
            if (!fluid_cell_replaceable(flat_get_block(nx, y, nz), is_water)) continue;
            fluid_spread_to(nx, y, nz, is_water, spread_level);
            moved = 1; changes_left--;
        }
    }

    (void)moved;
}


static void stream_generation_queue_clear(void) {
    g_stream_gen_queue_count = 0;
    g_stream_gen_queue_index = 0;
    g_stream_gen_queue_installed_count = 0;
    g_stream_generation_keep_completed = 0;
    g_stream_generation_epoch++;
    if (g_stream_generation_epoch <= 0) g_stream_generation_epoch = 1;
}

/* Exact seed terrain generation runs on a worker thread.  The main/game/render
   thread never performs the expensive Beta 3x3 canvas generation while walking;
   it only installs completed chunk buffers. */
static CRITICAL_SECTION g_stream_async_cs;
static int g_stream_async_initialized = 0;
static HANDLE g_stream_async_event = NULL;
static HANDLE g_stream_async_thread = NULL;
static int g_stream_async_stop = 0;
static int g_stream_async_has_job = 0;
static int g_stream_async_busy = 0;
static int g_stream_async_job_cx = 0, g_stream_async_job_cz = 0, g_stream_async_job_type = 0, g_stream_async_job_epoch = 0;
static long long g_stream_async_job_seed = 0;
static char g_stream_async_job_world_dir[MAX_PATHBUF];
static int g_stream_async_result_ready = 0;
static int g_stream_async_result_cx = 0, g_stream_async_result_cz = 0, g_stream_async_result_epoch = 0;
static unsigned char *g_stream_async_result_buf = NULL;
static unsigned char *g_stream_async_result_meta = NULL;

static DWORD WINAPI stream_async_worker_proc(LPVOID unused) {
    (void)unused;
    TerrainProvider worker_tp;
    int worker_tp_valid = 0;
    long long worker_tp_seed = 0;
    for (;;) {
        WaitForSingleObject(g_stream_async_event, INFINITE);
        int cx = 0, cz = 0, type = 0, epoch = 0, stop = 0;
        long long seed = 0;
        char world_dir[MAX_PATHBUF];
        world_dir[0] = 0;
        EnterCriticalSection(&g_stream_async_cs);
        stop = g_stream_async_stop;
        if (!stop && g_stream_async_has_job) {
            cx = g_stream_async_job_cx;
            cz = g_stream_async_job_cz;
            type = g_stream_async_job_type;
            seed = g_stream_async_job_seed;
            epoch = g_stream_async_job_epoch;
            snprintf(world_dir, sizeof(world_dir), "%s", g_stream_async_job_world_dir);
            g_stream_async_has_job = 0;
            g_stream_async_busy = 1;
        }
        LeaveCriticalSection(&g_stream_async_cs);
        if (stop) break;
        if (!g_stream_async_busy) continue;

        unsigned char *buf = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
        unsigned char *meta = (unsigned char*)calloc(FLAT_CHUNK_BLOCK_COUNT, 1);
        if (buf && meta) {
            TerrainProvider *reuse = NULL;
            if (type == 1) {
                if (!worker_tp_valid || worker_tp_seed != seed) {
                    if (worker_tp_valid) terrain_provider_free(&worker_tp);
                    terrain_provider_init(&worker_tp, (int64_t)seed);
                    worker_tp_seed = seed;
                    worker_tp_valid = 1;
                }
                reuse = &worker_tp;
            }
            generate_flat_chunk_base_to_buffer_ex(cx, cz, buf, type, seed, reuse);
            load_modified_flat_chunk_delta_into_buffers_for_dir(world_dir, cx, cz, buf, meta);
        } else {
            free(buf);
            free(meta);
            buf = NULL;
            meta = NULL;
        }

        EnterCriticalSection(&g_stream_async_cs);
        if (g_stream_async_result_buf) { free(g_stream_async_result_buf); g_stream_async_result_buf = NULL; }
        if (g_stream_async_result_meta) { free(g_stream_async_result_meta); g_stream_async_result_meta = NULL; }
        g_stream_async_result_buf = buf;
        g_stream_async_result_meta = meta;
        g_stream_async_result_cx = cx;
        g_stream_async_result_cz = cz;
        g_stream_async_result_epoch = epoch;
        g_stream_async_result_ready = (buf != NULL);
        g_stream_async_busy = 0;
        LeaveCriticalSection(&g_stream_async_cs);
    }
    if (worker_tp_valid) terrain_provider_free(&worker_tp);
    return 0;
}

static void stream_async_init(void) {
    if (g_stream_async_initialized) return;
#if defined(PEX_PLATFORM_PSP)
    /* Real PSP/PPSSPP became unstable when the PC terrain worker generated
       chunks on a second user thread.  Use cooperative async instead: one cheap
       PSP-safe chunk is generated/installed over time from the main loop. */
    g_stream_async_event = NULL;
    g_stream_async_thread = NULL;
    g_stream_async_initialized = 1;
    return;
#endif
    InitializeCriticalSection(&g_stream_async_cs);
    g_stream_async_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (g_stream_async_event) {
        g_stream_async_thread = CreateThread(NULL, 0, stream_async_worker_proc, NULL, 0, NULL);
        if (g_stream_async_thread) SetThreadPriority(g_stream_async_thread, THREAD_PRIORITY_BELOW_NORMAL);
    }
    g_stream_async_initialized = 1;
}

static int stream_async_pending(void) {
    if (!g_stream_async_initialized) return 0;
#if defined(PEX_PLATFORM_PSP)
    if (!g_stream_async_event || !g_stream_async_thread) return 0;
#endif
    int pending = 0;
    EnterCriticalSection(&g_stream_async_cs);
    pending = g_stream_async_has_job || g_stream_async_busy || g_stream_async_result_ready;
    LeaveCriticalSection(&g_stream_async_cs);
    return pending;
}

static void stream_async_submit_next(void) {
    stream_async_init();
    if (!g_stream_async_event || !g_stream_async_thread) return;
    if (g_stream_gen_queue_index >= g_stream_gen_queue_count) return;
    EnterCriticalSection(&g_stream_async_cs);
    int can_submit = !g_stream_async_has_job && !g_stream_async_busy && !g_stream_async_result_ready;
    if (can_submit) {
        g_stream_async_job_cx = g_stream_gen_queue_cx[g_stream_gen_queue_index];
        g_stream_async_job_cz = g_stream_gen_queue_cz[g_stream_gen_queue_index];
        g_stream_async_job_type = g_world_type;
        g_stream_async_job_seed = g_world_seed;
        g_stream_async_job_epoch = g_stream_generation_epoch;
        snprintf(g_stream_async_job_world_dir, sizeof(g_stream_async_job_world_dir), "%s", g_loaded_world_dir);
        g_stream_async_has_job = 1;
        g_stream_gen_queue_index++;
    }
    LeaveCriticalSection(&g_stream_async_cs);
    if (can_submit) SetEvent(g_stream_async_event);
}

static int stream_async_install_ready(int max_install) {
    int installed = 0;
    while (installed < max_install) {
        int cx = 0, cz = 0, epoch = 0;
        unsigned char *buf = NULL;
        unsigned char *meta = NULL;
        EnterCriticalSection(&g_stream_async_cs);
        if (g_stream_async_result_ready) {
            cx = g_stream_async_result_cx;
            cz = g_stream_async_result_cz;
            epoch = g_stream_async_result_epoch;
            buf = g_stream_async_result_buf;
            meta = g_stream_async_result_meta;
            g_stream_async_result_buf = NULL;
            g_stream_async_result_meta = NULL;
            g_stream_async_result_ready = 0;
        }
        LeaveCriticalSection(&g_stream_async_cs);
        if (!buf) break;

        if (epoch == g_stream_generation_epoch &&
            stream_world_chunk_in_window(cx, cz, g_flat_world_origin_x, g_flat_world_origin_z)) {
            copy_flat_chunk_buffers_to_world(cx, cz, buf, meta);
            stream_mark_local_chunk_generated(stream_world_chunk_local_x(cx), stream_world_chunk_local_z(cz));
            g_stream_gen_queue_installed_count++;
            installed++;
        }
        free(buf);
        free(meta);
    }
    return installed;
}

static int stream_generation_active(void) {
    int queued = g_stream_gen_queue_count - g_stream_gen_queue_index;
    if (queued < 0) queued = 0;
    int async = stream_async_pending() ? 1 : 0;
    g_prof_stream_pending_last = queued + async;
    return queued > 0 || async;
}

static int stream_world_chunk_in_window(int wcx, int wcz, int origin_x, int origin_z) {
    int wx0 = wcx * FLAT_RENDER_CHUNK;
    int wz0 = wcz * FLAT_RENDER_CHUNK;
    return wx0 >= origin_x && wz0 >= origin_z &&
           wx0 + FLAT_RENDER_CHUNK <= origin_x + FLAT_WORLD_SIZE &&
           wz0 + FLAT_RENDER_CHUNK <= origin_z + FLAT_WORLD_SIZE;
}

static PexRendererBackend *stream_direct_mesh_backend(void) {
    if (g_runtime_renderer_backend == RENDERER_D3D9) return renderer_d3d9_get_backend();
    if (g_runtime_renderer_backend == RENDERER_D3D11) return renderer_d3d11_get_backend();
    return NULL;
}

static void stream_destroy_direct_mesh_handle(unsigned int h) {
    PexRendererBackend *rb = stream_direct_mesh_backend();
    if (rb && h) rb->destroy_mesh((PexMeshHandle)h);
}

static void stream_remap_render_chunks_after_shift(int old_origin_x, int old_origin_z,
                                                    int new_origin_x, int new_origin_z) {
    GLuint old_lists[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    GLuint old_liquid_lists[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_dirty[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_valid[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_has_liquid[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_generated[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_modified[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    unsigned short old_section_masks[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    GLuint old_section_lists[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS][2];
    int old_section_dirty[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    unsigned int old_section_versions[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_section_valid[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_section_skip[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS][2];
    unsigned int old_section_direct_mesh[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS][2];
    GLuint reusable_section_lists[FLAT_RENDER_SECTIONS_Y * FLAT_RENDER_CHUNKS * FLAT_RENDER_CHUNKS][2];
    int reusable_section_count = 0;
    int reusable_section_index = 0;
    GLuint reusable_lists[STREAM_GEN_QUEUE_MAX];
    GLuint reusable_liquid_lists[STREAM_GEN_QUEUE_MAX];
    int reusable_count = 0;
    int reusable_index = 0;

    memcpy(old_lists, g_flat_world_chunk_lists, sizeof(old_lists));
    memcpy(old_liquid_lists, g_flat_world_chunk_liquid_lists, sizeof(old_liquid_lists));
    memcpy(old_dirty, g_flat_world_chunk_dirty, sizeof(old_dirty));
    memcpy(old_valid, g_flat_world_chunk_valid, sizeof(old_valid));
    memcpy(old_has_liquid, g_flat_world_chunk_has_liquid, sizeof(old_has_liquid));
    memcpy(old_generated, g_flat_world_chunk_generated, sizeof(old_generated));
    memcpy(old_modified, g_flat_world_chunk_modified, sizeof(old_modified));
    memcpy(old_section_masks, g_flat_chunk_section_non_empty_mask, sizeof(old_section_masks));
    memcpy(old_section_lists, g_flat_section_lists, sizeof(old_section_lists));
    memcpy(old_section_dirty, g_flat_section_dirty, sizeof(old_section_dirty));
    memcpy(old_section_versions, g_flat_section_mesh_version, sizeof(old_section_versions));
    memcpy(old_section_valid, g_flat_section_valid, sizeof(old_section_valid));
    memcpy(old_section_skip, g_flat_section_skip_pass, sizeof(old_section_skip));
    memcpy(old_section_direct_mesh, g_flat_section_direct_mesh, sizeof(old_section_direct_mesh));

    int old_base_cx = floor_div16(old_origin_x);
    int old_base_cz = floor_div16(old_origin_z);
    int new_base_cx = floor_div16(new_origin_x);
    int new_base_cz = floor_div16(new_origin_z);

    for (int ocz = 0; ocz < FLAT_RENDER_CHUNKS; ocz++) {
        for (int ocx = 0; ocx < FLAT_RENDER_CHUNKS; ocx++) {
            int wcx = old_base_cx + ocx;
            int wcz = old_base_cz + ocz;
            if (!stream_world_chunk_in_window(wcx, wcz, new_origin_x, new_origin_z)) {
                reusable_lists[reusable_count] = old_lists[ocz][ocx];
                reusable_liquid_lists[reusable_count] = old_liquid_lists[ocz][ocx];
                reusable_count++;
                for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
                    if (old_section_lists[sy][ocz][ocx][0] != 0) {
                        reusable_section_lists[reusable_section_count][0] = old_section_lists[sy][ocz][ocx][0];
                        reusable_section_lists[reusable_section_count][1] = old_section_lists[sy][ocz][ocx][1];
                        reusable_section_count++;
                    }
                    stream_destroy_direct_mesh_handle(old_section_direct_mesh[sy][ocz][ocx][0]);
                    stream_destroy_direct_mesh_handle(old_section_direct_mesh[sy][ocz][ocx][1]);
                }
            }
        }
    }

    memset(g_flat_world_chunk_lists, 0, sizeof(g_flat_world_chunk_lists));
    memset(g_flat_world_chunk_liquid_lists, 0, sizeof(g_flat_world_chunk_liquid_lists));
    memset(g_flat_world_chunk_valid, 0, sizeof(g_flat_world_chunk_valid));
    memset(g_flat_world_chunk_has_liquid, 0, sizeof(g_flat_world_chunk_has_liquid));
    memset(g_flat_world_chunk_generated, 0, sizeof(g_flat_world_chunk_generated));
    memset(g_flat_world_chunk_modified, 0, sizeof(g_flat_world_chunk_modified));
    memset(g_flat_chunk_section_non_empty_mask, 0, sizeof(g_flat_chunk_section_non_empty_mask));
    memset(g_flat_section_lists, 0, sizeof(g_flat_section_lists));
    memset(g_flat_section_direct_mesh, 0, sizeof(g_flat_section_direct_mesh));
    memset(g_flat_section_dirty, 0, sizeof(g_flat_section_dirty));
    memset(g_flat_section_mesh_building, 0, sizeof(g_flat_section_mesh_building));
    memset(g_flat_section_valid, 0, sizeof(g_flat_section_valid));
    for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
        for (int cz = 0; cz < FLAT_RENDER_CHUNKS; cz++) {
            for (int cx = 0; cx < FLAT_RENDER_CHUNKS; cx++) {
                g_flat_section_skip_pass[sy][cz][cx][0] = 1;
                g_flat_section_skip_pass[sy][cz][cx][1] = 1;
            }
        }
    }
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
                g_flat_world_chunk_generated[ncz][ncx] = old_generated[ocz][ocx];
                g_flat_world_chunk_modified[ncz][ncx] = old_modified[ocz][ocx];
                g_flat_chunk_section_non_empty_mask[ncz][ncx] = old_section_masks[ocz][ocx];
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
                g_flat_world_chunk_generated[ncz][ncx] = 0;
                g_flat_world_chunk_modified[ncz][ncx] = 0;
                g_flat_chunk_section_non_empty_mask[ncz][ncx] = 0;
            }
        }
    }
    for (int ncz = 0; ncz < FLAT_RENDER_CHUNKS; ncz++) {
        for (int ncx = 0; ncx < FLAT_RENDER_CHUNKS; ncx++) {
            int wcx = new_base_cx + ncx;
            int wcz = new_base_cz + ncz;
            if (stream_world_chunk_in_window(wcx, wcz, old_origin_x, old_origin_z)) {
                int ocx = wcx - old_base_cx;
                int ocz = wcz - old_base_cz;
                for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
                    g_flat_section_lists[sy][ncz][ncx][0] = old_section_lists[sy][ocz][ocx][0];
                    g_flat_section_lists[sy][ncz][ncx][1] = old_section_lists[sy][ocz][ocx][1];
                    g_flat_section_direct_mesh[sy][ncz][ncx][0] = old_section_direct_mesh[sy][ocz][ocx][0];
                    g_flat_section_direct_mesh[sy][ncz][ncx][1] = old_section_direct_mesh[sy][ocz][ocx][1];
                    g_flat_section_dirty[sy][ncz][ncx] = old_section_dirty[sy][ocz][ocx];
                    g_flat_section_mesh_version[sy][ncz][ncx] = old_section_versions[sy][ocz][ocx];
                    g_flat_section_mesh_building[sy][ncz][ncx] = 0;
                    g_flat_section_valid[sy][ncz][ncx] = old_section_valid[sy][ocz][ocx];
                    g_flat_section_skip_pass[sy][ncz][ncx][0] = old_section_skip[sy][ocz][ocx][0];
                    g_flat_section_skip_pass[sy][ncz][ncx][1] = old_section_skip[sy][ocz][ocx][1];
                }
            } else {
                for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
                    if (reusable_section_index < reusable_section_count) {
                        g_flat_section_lists[sy][ncz][ncx][0] = reusable_section_lists[reusable_section_index][0];
                        g_flat_section_lists[sy][ncz][ncx][1] = reusable_section_lists[reusable_section_index][1];
                        reusable_section_index++;
                    }
                    g_flat_section_dirty[sy][ncz][ncx] = 1;
                    flat_note_section_mesh_changed(sy, ncz, ncx);
                    g_flat_section_valid[sy][ncz][ncx] = 0;
                    g_flat_section_skip_pass[sy][ncz][ncx][0] = 1;
                    g_flat_section_skip_pass[sy][ncz][ncx][1] = 1;
                }
            }
        }
    }
    g_flat_renderer_sort_dirty = 1;
    g_flat_world_geometry_dirty = 0;
    g_flat_section_geometry_dirty = 0;
}

static void stream_remap_flat_block_storage_after_shift(int old_origin_x, int old_origin_z,
                                                       int new_origin_x, int new_origin_z) {
    int ox0 = old_origin_x > new_origin_x ? old_origin_x : new_origin_x;
    int oz0 = old_origin_z > new_origin_z ? old_origin_z : new_origin_z;
    int ox1 = (old_origin_x + FLAT_WORLD_SIZE) < (new_origin_x + FLAT_WORLD_SIZE) ?
              (old_origin_x + FLAT_WORLD_SIZE) : (new_origin_x + FLAT_WORLD_SIZE);
    int oz1 = (old_origin_z + FLAT_WORLD_SIZE) < (new_origin_z + FLAT_WORLD_SIZE) ?
              (old_origin_z + FLAT_WORLD_SIZE) : (new_origin_z + FLAT_WORLD_SIZE);
    int row_len = ox1 - ox0;

    if (row_len <= 0 || oz1 <= oz0) {
        memset(g_flat_blocks, 0, sizeof(g_flat_blocks));
        memset(g_flat_meta, 0, sizeof(g_flat_meta));
        memset(g_flat_levels, 0, sizeof(g_flat_levels));
        return;
    }

    int z_step = (new_origin_z < old_origin_z) ? -1 : 1;
    int z_begin = (z_step > 0) ? oz0 : (oz1 - 1);
    int z_end = (z_step > 0) ? oz1 : (oz0 - 1);
    int old_fx = ox0 - old_origin_x;
    int new_fx = ox0 - new_origin_x;

    for (int y = 0; y < FLAT_WORLD_HEIGHT; y++) {
        for (int z = z_begin; z != z_end; z += z_step) {
            int old_fz = z - old_origin_z;
            int new_fz = z - new_origin_z;
            memmove(&g_flat_blocks[y][new_fz][new_fx], &g_flat_blocks[y][old_fz][old_fx], (size_t)row_len);
            memmove(&g_flat_meta[y][new_fz][new_fx], &g_flat_meta[y][old_fz][old_fx], (size_t)row_len);
            memmove(&g_flat_levels[y][new_fz][new_fx], &g_flat_levels[y][old_fz][old_fx], (size_t)row_len);
        }

        int dz = new_origin_z - old_origin_z;
        if (dz > 0) {
            int n = dz;
            if (n > FLAT_WORLD_SIZE) n = FLAT_WORLD_SIZE;
            memset(&g_flat_blocks[y][FLAT_WORLD_SIZE - n][0], 0, (size_t)n * FLAT_WORLD_SIZE);
            memset(&g_flat_meta[y][FLAT_WORLD_SIZE - n][0], 0, (size_t)n * FLAT_WORLD_SIZE);
            memset(&g_flat_levels[y][FLAT_WORLD_SIZE - n][0], 0, (size_t)n * FLAT_WORLD_SIZE);
        } else if (dz < 0) {
            int n = -dz;
            if (n > FLAT_WORLD_SIZE) n = FLAT_WORLD_SIZE;
            memset(&g_flat_blocks[y][0][0], 0, (size_t)n * FLAT_WORLD_SIZE);
            memset(&g_flat_meta[y][0][0], 0, (size_t)n * FLAT_WORLD_SIZE);
            memset(&g_flat_levels[y][0][0], 0, (size_t)n * FLAT_WORLD_SIZE);
        }

        int dx = new_origin_x - old_origin_x;
        if (dx > 0) {
            int n = dx;
            if (n > FLAT_WORLD_SIZE) n = FLAT_WORLD_SIZE;
            int x0 = FLAT_WORLD_SIZE - n;
            for (int z = 0; z < FLAT_WORLD_SIZE; z++) {
                memset(&g_flat_blocks[y][z][x0], 0, (size_t)n);
                memset(&g_flat_meta[y][z][x0], 0, (size_t)n);
                memset(&g_flat_levels[y][z][x0], 0, (size_t)n);
            }
        } else if (dx < 0) {
            int n = -dx;
            if (n > FLAT_WORLD_SIZE) n = FLAT_WORLD_SIZE;
            for (int z = 0; z < FLAT_WORLD_SIZE; z++) {
                memset(&g_flat_blocks[y][z][0], 0, (size_t)n);
                memset(&g_flat_meta[y][z][0], 0, (size_t)n);
                memset(&g_flat_levels[y][z][0], 0, (size_t)n);
            }
        }
    }
}

static int stream_world_chunk_local_x(int wcx) {
    return (wcx * FLAT_RENDER_CHUNK - g_flat_world_origin_x) / FLAT_RENDER_CHUNK;
}

static int stream_world_chunk_local_z(int wcz) {
    return (wcz * FLAT_RENDER_CHUNK - g_flat_world_origin_z) / FLAT_RENDER_CHUNK;
}

static void stream_mark_local_chunk_dirty(int lcx, int lcz) {
    if (lcx < 0 || lcx >= FLAT_RENDER_CHUNKS || lcz < 0 || lcz >= FLAT_RENDER_CHUNKS) return;
    for (int dz = -1; dz <= 1; dz++) {
        for (int dx = -1; dx <= 1; dx++) {
            int cx = lcx + dx;
            int cz = lcz + dz;
            if (cx < 0 || cx >= FLAT_RENDER_CHUNKS || cz < 0 || cz >= FLAT_RENDER_CHUNKS) continue;
            g_flat_world_chunk_dirty[cz][cx] = 1;
            for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
                /* Do not invalidate an existing mesh here.  The rebuild path will
                   atomically replace it after the new section mesh has uploaded. */
                g_flat_section_dirty[sy][cz][cx] = 1;
                flat_note_section_mesh_changed(sy, cz, cx);
            }
        }
    }
}


static int stream_queue_contains_chunk(int wcx, int wcz) {
    for (int i = g_stream_gen_queue_index; i < g_stream_gen_queue_count; i++) {
        if (g_stream_gen_queue_cx[i] == wcx && g_stream_gen_queue_cz[i] == wcz) return 1;
    }
    return 0;
}

static void stream_queue_add_chunk(int wcx, int wcz) {
    if (g_stream_gen_queue_count >= STREAM_GEN_QUEUE_MAX) return;
    if (!stream_world_chunk_in_window(wcx, wcz, g_flat_world_origin_x, g_flat_world_origin_z)) return;
    int lcx = stream_world_chunk_local_x(wcx);
    int lcz = stream_world_chunk_local_z(wcz);
    if (lcx >= 0 && lcx < FLAT_RENDER_CHUNKS && lcz >= 0 && lcz < FLAT_RENDER_CHUNKS &&
        g_flat_world_chunk_generated[lcz][lcx]) return;
    if (stream_queue_contains_chunk(wcx, wcz)) return;
    g_stream_gen_queue_cx[g_stream_gen_queue_count] = wcx;
    g_stream_gen_queue_cz[g_stream_gen_queue_count] = wcz;
    g_stream_gen_queue_count++;
}

static int stream_queue_missing_visible_chunks_near_player(void) {
    if (stream_generation_active()) return 0;
    stream_generation_queue_clear();

    int base_cx = floor_div16(g_flat_world_origin_x);
    int base_cz = floor_div16(g_flat_world_origin_z);
    int pcx = stream_world_chunk_local_x(floor_div16((int)floorf(g_player_x)));
    int pcz = stream_world_chunk_local_z(floor_div16((int)floorf(g_player_z)));
    if (pcx < 0) pcx = 0;
    if (pcz < 0) pcz = 0;
    if (pcx >= FLAT_RENDER_CHUNKS) pcx = FLAT_RENDER_CHUNKS - 1;
    if (pcz >= FLAT_RENDER_CHUNKS) pcz = FLAT_RENDER_CHUNKS - 1;

    int radius = stream_effective_render_chunk_radius();
    for (int ring = 0; ring <= radius; ring++) {
        int before = g_stream_gen_queue_count;
        for (int dz = -ring; dz <= ring; dz++) {
            for (int dx = -ring; dx <= ring; dx++) {
                if (ring != 0 && abs(dx) != ring && abs(dz) != ring) continue;
                int lcx = pcx + dx;
                int lcz = pcz + dz;
                if (lcx < 0 || lcx >= FLAT_RENDER_CHUNKS || lcz < 0 || lcz >= FLAT_RENDER_CHUNKS) continue;
                if (g_flat_world_chunk_generated[lcz][lcx]) continue;
                stream_queue_add_chunk(base_cx + lcx, base_cz + lcz);
            }
        }
        /* Do not enqueue the entire render distance in one burst.  Filling one
           visible ring at a time keeps Stream pending small and prevents the
           background generator from cooking the CPU after join. */
        if (g_stream_gen_queue_count > before) break;
    }
    return g_stream_gen_queue_count > 0;
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
    stream_async_init();

    /* If thread creation fails, keep the world functional with the old small
       synchronous budget rather than leaving missing terrain forever. */
    if (!g_stream_async_event || !g_stream_async_thread) {
        if (g_stream_gen_queue_index < g_stream_gen_queue_count) {
            int idx = g_stream_gen_queue_index++;
            int wcx = g_stream_gen_queue_cx[idx];
            int wcz = g_stream_gen_queue_cz[idx];
            beta_preview_copy_chunk_to_flat(wcx, wcz);
            stream_mark_local_chunk_generated(stream_world_chunk_local_x(wcx), stream_world_chunk_local_z(wcz));
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_FAST_WORLD) && PEX_PSP_FAST_WORLD
            psp_fast_surface_mark_dirty_block(wcx * 16 + 8, wcz * 16 + 8);
#endif
            g_stream_gen_queue_installed_count++;
        }
        if (!stream_generation_active() && !g_stream_generation_keep_completed) stream_generation_queue_clear();
        return;
    }

    static int s_stream_install_tick = 0;
    int allow_install = ((s_stream_install_tick++ & 1) == 0);
    if (allow_install) stream_async_install_ready(1);
    stream_async_submit_next();

    if (!stream_generation_active() && !g_stream_generation_keep_completed) {
        stream_generation_queue_clear();
    }
}


static void update_infinite_world_streaming(void) {
    /* Do a tiny amount of terrain generation per tick.  This is the important
       part: new chunks are no longer built all at once on the frame that the
       streaming message appears. */
    process_stream_generation_queue();
    if (stream_generation_active()) return;

    /* After the fast spawn-area load, fill the requested render-distance area
       in the background.  This keeps the loading screen short while still
       honoring high render distance as terrain becomes available. */
    if (stream_queue_missing_visible_chunks_near_player()) {
        process_stream_generation_queue();
        return;
    }

    int pcx = floor_div16((int)floorf(g_player_x));
    int pcz = floor_div16((int)floorf(g_player_z));

    int margin = stream_window_margin_blocks();
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

    /* Save edited chunks before they leave the 256x256 active window.  Generated
       chunks are not written, only chunks with real gameplay changes. */
    save_modified_flat_chunks();

    int old_origin_x = g_flat_world_origin_x;
    int old_origin_z = g_flat_world_origin_z;

    g_flat_world_origin_x = new_origin_x;
    g_flat_world_origin_z = new_origin_z;
    stream_remap_render_chunks_after_shift(old_origin_x, old_origin_z, new_origin_x, new_origin_z);

    /* Preserve edited/generated blocks in the overlap of the old and new window.
       This used to copy the entire 3D world into two huge scratch arrays and then
       copy it back byte-by-byte, which caused the hitch during terrain streaming.
       The row-copy remap keeps only a small 2D scratch plane and leaves exposed
       strips as air for the async generator to fill. */
    stream_remap_flat_block_storage_after_shift(old_origin_x, old_origin_z, new_origin_x, new_origin_z);

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

static int flat_block_is_underwater_target(int bx, int by, int bz) {
    /* A mined block counts as underwater when water is directly above it or
       pressed against any side.  The target itself is normally solid, because
       raycast skips liquid blocks. */
    return block_is_water(flat_get_block(bx, by + 1, bz)) ||
           block_is_water(flat_get_block(bx + 1, by, bz)) ||
           block_is_water(flat_get_block(bx - 1, by, bz)) ||
           block_is_water(flat_get_block(bx, by, bz + 1)) ||
           block_is_water(flat_get_block(bx, by, bz - 1));
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
