/* Split from original monolithic main.c. Included by src/main.c unity build. */

static void generate_flat_chunk_base_to_buffer_ex(int cx, int cz, unsigned char *out, int world_type, long long seed, TerrainProvider *reuse_tp);
#ifndef FLAT_CHUNK_BLOCK_COUNT
#define FLAT_CHUNK_BLOCK_COUNT (FLAT_RENDER_CHUNK * FLAT_RENDER_CHUNK * FLAT_WORLD_HEIGHT)
#endif


static void level_path_for_dir(const char *dir, char *out, size_t cap) {
    snprintf(out, cap, "%s\\level.dat", dir);
}

static void legacy_world_type_path_for_dir(const char *dir, char *out, size_t cap) {
    snprintf(out, cap, "%s\\pexcraft_world_type.txt", dir);
}

static void legacy_world_seed_path_for_dir(const char *dir, char *out, size_t cap) {
    snprintf(out, cap, "%s\\pexcraft_seed.txt", dir);
}

static void legacy_world_state_path_for_dir(const char *dir, char *out, size_t cap) {
    snprintf(out, cap, "%s\\pexcraft_flat_world.dat", dir);
}

static int read_whole_file(const char *path, unsigned char **out_data, size_t *out_len) {
    if (out_data) *out_data = NULL;
    if (out_len) *out_len = 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    long n = ftell(f);
    if (n <= 0 || n > 1024 * 1024) { fclose(f); return 0; }
    rewind(f);
    unsigned char *buf = (unsigned char*)malloc((size_t)n);
    if (!buf) { fclose(f); return 0; }
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) { free(buf); fclose(f); return 0; }
    fclose(f);
    if (out_data) *out_data = buf; else free(buf);
    if (out_len) *out_len = (size_t)n;
    return 1;
}

static int read_level_long_tag_for_dir(const char *dir, const char *tag, long long *out_value) {
    char path[MAX_PATHBUF];
    level_path_for_dir(dir, path, sizeof(path));
    unsigned char *buf = NULL;
    size_t n = 0;
    if (!read_whole_file(path, &buf, &n)) return 0;
    size_t tl = strlen(tag);
    for (size_t i = 0; i + tl + 8 <= n; i++) {
        if (memcmp(buf + i, tag, tl) != 0) continue;
        size_t vpos = i + tl;
        unsigned long long u = 0;
        for (int k = 0; k < 8; k++) u = (u << 8) | (unsigned long long)buf[vpos + k];
        if (out_value) *out_value = (long long)u;
        free(buf);
        return 1;
    }
    free(buf);
    return 0;
}

static int read_level_int_tag_for_dir(const char *dir, const char *tag, int *out_value) {
    char path[MAX_PATHBUF];
    level_path_for_dir(dir, path, sizeof(path));
    unsigned char *buf = NULL;
    size_t n = 0;
    if (!read_whole_file(path, &buf, &n)) return 0;
    size_t tl = strlen(tag);
    for (size_t i = 0; i + tl + 4 <= n; i++) {
        if (memcmp(buf + i, tag, tl) != 0) continue;
        size_t vpos = i + tl;
        unsigned int u = ((unsigned int)buf[vpos] << 24) | ((unsigned int)buf[vpos + 1] << 16) |
                         ((unsigned int)buf[vpos + 2] << 8) | (unsigned int)buf[vpos + 3];
        if (out_value) *out_value = (int)u;
        free(buf);
        return 1;
    }
    free(buf);
    return 0;
}

static int read_level_string_tag_for_dir(const char *dir, const char *tag, char *out, size_t cap) {
    if (out && cap) out[0] = 0;
    char path[MAX_PATHBUF];
    level_path_for_dir(dir, path, sizeof(path));
    unsigned char *buf = NULL;
    size_t n = 0;
    if (!read_whole_file(path, &buf, &n)) return 0;
    size_t tl = strlen(tag);
    for (size_t i = 0; i + tl + 2 <= n; i++) {
        if (memcmp(buf + i, tag, tl) != 0) continue;
        size_t vpos = i + tl;
        unsigned int sl = ((unsigned int)buf[vpos] << 8) | (unsigned int)buf[vpos + 1];
        if (vpos + 2 + sl > n) continue;
        if (out && cap) {
            size_t copy = sl < cap - 1 ? sl : cap - 1;
            memcpy(out, buf + vpos + 2, copy);
            out[copy] = 0;
        }
        free(buf);
        return 1;
    }
    free(buf);
    return 0;
}

static int read_binary_level_metadata_for_dir(const char *dir, long long *out_seed, int *out_world_type) {
    char path[MAX_PATHBUF];
    level_path_for_dir(dir, path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    char magic[8];
    int version = 0, w = 0, h = 0, y_min = 0;
    int ok = 0;
    if (fread(magic, 1, 8, f) == 8 &&
        fread(&version, sizeof(version), 1, f) == 1 &&
        fread(&w, sizeof(w), 1, f) == 1 &&
        fread(&h, sizeof(h), 1, f) == 1 &&
        fread(&y_min, sizeof(y_min), 1, f) == 1 &&
        memcmp(magic, "LEVELST1", 8) == 0 && version >= 13 &&
        w == FLAT_WORLD_SIZE && h == FLAT_WORLD_HEIGHT && y_min == FLAT_WORLD_Y_MIN) {
        long long seed = 0;
        float px = 0, py = 0, pz = 0, yaw = 0, pitch = 0, fall = 0;
        int health = 0, armor = 0, slot = 0, dead = 0, wt = 0;
        if (fread(&seed, sizeof(seed), 1, f) == 1 &&
            fread(&px, sizeof(px), 1, f) == 1 &&
            fread(&py, sizeof(py), 1, f) == 1 &&
            fread(&pz, sizeof(pz), 1, f) == 1 &&
            fread(&yaw, sizeof(yaw), 1, f) == 1 &&
            fread(&pitch, sizeof(pitch), 1, f) == 1 &&
            fread(&health, sizeof(health), 1, f) == 1 &&
            fread(&armor, sizeof(armor), 1, f) == 1 &&
            fread(&slot, sizeof(slot), 1, f) == 1 &&
            fread(&fall, sizeof(fall), 1, f) == 1 &&
            fread(&dead, sizeof(dead), 1, f) == 1 &&
            fread(&wt, sizeof(wt), 1, f) == 1) {
            if (out_seed) *out_seed = seed;
            if (out_world_type) *out_world_type = wt ? 1 : 0;
            ok = 1;
        }
    }
    fclose(f);
    return ok;
}

static int read_level_seed_for_dir(const char *dir, long long *out_seed) {
    if (read_binary_level_metadata_for_dir(dir, out_seed, NULL)) return 1;
    return read_level_long_tag_for_dir(dir, "RandomSeed", out_seed);
}

static int read_world_seed_for_dir(const char *dir, long long *out_seed) {
    if (read_level_seed_for_dir(dir, out_seed)) return 1;

    /* Legacy fallback only.  New saves keep this in level.dat. */
    char path[MAX_PATHBUF];
    legacy_world_seed_path_for_dir(dir, path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (f) {
        char buf[128] = "";
        fread(buf, 1, sizeof(buf) - 1, f);
        fclose(f);
        if (out_seed) *out_seed = strtoll(buf, NULL, 10);
        return 1;
    }
    return 0;
}

static int read_world_type_for_dir(const char *dir) {
    int type = 0;
    if (read_binary_level_metadata_for_dir(dir, NULL, &type)) return type ? 1 : 0;
    if (read_level_int_tag_for_dir(dir, "WorldType", &type)) return type ? 1 : 0;

    char gen[64];
    if (read_level_string_tag_for_dir(dir, "generatorName", gen, sizeof(gen))) {
        return (strstr(gen, "default") || strstr(gen, "normal")) ? 1 : 0;
    }

    /* Legacy fallback only.  New saves keep this in level.dat. */
    char path[MAX_PATHBUF];
    legacy_world_type_path_for_dir(dir, path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    char buf[32] = "";
    fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    return (strstr(buf, "normal") != NULL) ? 1 : 0;
}

static void write_world_type_for_dir(const char *dir, int type) {
    /* Deprecated: world type is written into level.dat. */
    (void)dir;
    (void)type;
}

static void write_world_seed_for_dir(const char *dir, long long seed) {
    /* Deprecated: seed is written into level.dat. */
    (void)dir;
    (void)seed;
}

static void world_state_path(char *out, size_t cap) {
    if (!g_loaded_world_dir[0]) {
        if (cap) out[0] = 0;
        return;
    }
    level_path_for_dir(g_loaded_world_dir, out, cap);
}

static int world_state_magic_is_known(const char *magic) {
    return memcmp(magic, "LEVELST1", 8) == 0 ||
           memcmp(magic, "PXCFLAT4", 8) == 0 ||
           memcmp(magic, "PXCFLAT6", 8) == 0 ||
           memcmp(magic, "PXCFLAT7", 8) == 0 ||
           memcmp(magic, "PXCFLAT9", 8) == 0 ||
           memcmp(magic, "PXCFLT10", 8) == 0 ||
           memcmp(magic, "PXCFLT11", 8) == 0 ||
           memcmp(magic, "PXCFLT12", 8) == 0;
}

static FILE *open_current_world_state_file(void) {
    char path[MAX_PATHBUF];
    world_state_path(path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (f) {
        char magic[8];
        if (fread(magic, 1, 8, f) == 8 && world_state_magic_is_known(magic)) {
            rewind(f);
            return f;
        }
        fclose(f);
    }

    legacy_world_state_path_for_dir(g_loaded_world_dir, path, sizeof(path));
    f = fopen(path, "rb");
    if (f) return f;
    return NULL;
}

static void cleanup_legacy_world_metadata(const char *dir) {
    char path[MAX_PATHBUF];
    legacy_world_state_path_for_dir(dir, path, sizeof(path));
    DeleteFileA(path);
    legacy_world_seed_path_for_dir(dir, path, sizeof(path));
    DeleteFileA(path);
    legacy_world_type_path_for_dir(dir, path, sizeof(path));
    DeleteFileA(path);
}

static void migrate_legacy_chunk_folder(const char *dir) {
    char old_dir[MAX_PATHBUF], new_dir[MAX_PATHBUF], pattern[MAX_PATHBUF];
    snprintf(old_dir, sizeof(old_dir), "%s\\pexcraft_modified_chunks", dir);
    if (!dir_exists(old_dir)) return;

    snprintf(new_dir, sizeof(new_dir), "%s\\chunks", dir);
    make_dir_recursive(new_dir);

    WIN32_FIND_DATAA fd;
    snprintf(pattern, sizeof(pattern), "%s\\*.pcd", old_dir);
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            char old_path[MAX_PATHBUF], new_path[MAX_PATHBUF], new_name[MAX_LABEL];
            snprintf(old_path, sizeof(old_path), "%s\\%s", old_dir, fd.cFileName);
            snprintf(new_name, sizeof(new_name), "%s", fd.cFileName);
            char *dot = strrchr(new_name, '.');
            if (dot) snprintf(dot, (size_t)(sizeof(new_name) - (dot - new_name)), ".dat");
            snprintf(new_path, sizeof(new_path), "%s\\%s", new_dir, new_name);
            if (file_exists(new_path)) DeleteFileA(old_path);
            else MoveFileA(old_path, new_path);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    RemoveDirectoryA(old_dir);
}

static int pex_save_floor_div16(int v) {
    return v >= 0 ? v / 16 : -(((-v) + 15) / 16);
}

static int pex_save_flat_y_index(int y) {
    return y - FLAT_WORLD_Y_MIN;
}

static int pex_save_chunk_buf_index(int lx, int y, int lz) {
    return pex_save_flat_y_index(y) * (FLAT_RENDER_CHUNK * FLAT_RENDER_CHUNK) + lz * FLAT_RENDER_CHUNK + lx;
}

static size_t pex_save_flat_array_index(int yi, int z, int x) {
    return ((size_t)yi * (size_t)FLAT_WORLD_SIZE + (size_t)z) * (size_t)FLAT_WORLD_SIZE + (size_t)x;
}

static void pex_save_snapshot_free(PexSaveSnapshot *ss) {
    if (!ss) return;
    for (int i = 0; i < ss->chunk_count; i++) {
        free(ss->chunks[i].blocks);
        free(ss->chunks[i].meta);
    }
    free(ss);
}

static int pex_save_snapshot_add_modified_chunk(PexSaveSnapshot *ss, int lcx, int lcz) {
    if (!ss) return 0;
    if (lcx < 0 || lcx >= FLAT_RENDER_CHUNKS || lcz < 0 || lcz >= FLAT_RENDER_CHUNKS) return 1;
    if (!g_flat_world_chunk_modified[lcz][lcx]) return 1;
    if (ss->chunk_count >= MAX_SAVE_CHUNK_SNAPSHOTS) return 0;

    PexSaveChunkSnapshot *cs = &ss->chunks[ss->chunk_count];
    memset(cs, 0, sizeof(*cs));
    cs->cx = pex_save_floor_div16(g_flat_world_origin_x) + lcx;
    cs->cz = pex_save_floor_div16(g_flat_world_origin_z) + lcz;
    cs->blocks = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
    cs->meta = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
    if (!cs->blocks || !cs->meta) {
        free(cs->blocks);
        free(cs->meta);
        memset(cs, 0, sizeof(*cs));
        return 0;
    }

    int fx0 = lcx * FLAT_RENDER_CHUNK;
    int fz0 = lcz * FLAT_RENDER_CHUNK;
    for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
        int yi = pex_save_flat_y_index(y);
        for (int lz = 0; lz < FLAT_RENDER_CHUNK; lz++) {
            int bi = pex_save_chunk_buf_index(0, y, lz);
            memcpy(&cs->blocks[bi], &g_flat_blocks[yi][fz0 + lz][fx0], FLAT_RENDER_CHUNK);
            memcpy(&cs->meta[bi], &g_flat_meta[yi][fz0 + lz][fx0], FLAT_RENDER_CHUNK);
        }
    }

    ss->chunk_count++;
    return 1;
}

static PexSaveSnapshot *pex_save_snapshot_create(int write_world_state) {
    PexSaveSnapshot *ss = (PexSaveSnapshot*)calloc(1, sizeof(PexSaveSnapshot));
    if (!ss) return NULL;

    ss->write_world_state = write_world_state ? 1 : 0;
    ss->flat_world_origin_x = g_flat_world_origin_x;
    ss->flat_world_origin_z = g_flat_world_origin_z;
    ss->world_seed = g_world_seed;
    ss->world_type = g_world_type;
    ss->player_x = g_player_x;
    ss->player_y = g_player_y;
    ss->player_z = g_player_z;
    ss->player_yaw = g_player_yaw;
    ss->player_pitch = g_player_pitch;
    ss->player_health = g_player_health;
    ss->player_armor = g_player_armor;
    ss->selected_hotbar_slot = g_selected_hotbar_slot;
    ss->player_fall_distance = g_player_fall_distance;
    ss->player_dead = g_player_dead;
    memcpy(ss->inventory, g_inventory, sizeof(g_inventory));
    memcpy(ss->chest_tiles, g_chest_tiles, sizeof(g_chest_tiles));
    memcpy(ss->furnace_tiles, g_furnace_tiles, sizeof(g_furnace_tiles));
    memcpy(ss->drops, g_drops, sizeof(g_drops));
    memcpy(ss->passive_mobs, g_passive_mobs, sizeof(g_passive_mobs));
    snprintf(ss->loaded_world_dir, sizeof(ss->loaded_world_dir), "%s", g_loaded_world_dir);
    ss->ingame_ticks = g_ingame_ticks;

    for (int lcz = 0; lcz < FLAT_RENDER_CHUNKS; lcz++) {
        for (int lcx = 0; lcx < FLAT_RENDER_CHUNKS; lcx++) {
            if (!pex_save_snapshot_add_modified_chunk(ss, lcx, lcz)) {
                pex_save_snapshot_free(ss);
                return NULL;
            }
        }
    }

    return ss;
}

static void pex_clear_snapshot_chunk_modified_flags(const PexSaveSnapshot *ss) {
    if (!ss) return;
    int base_cx = pex_save_floor_div16(g_flat_world_origin_x);
    int base_cz = pex_save_floor_div16(g_flat_world_origin_z);
    for (int i = 0; i < ss->chunk_count; i++) {
        int lcx = ss->chunks[i].cx - base_cx;
        int lcz = ss->chunks[i].cz - base_cz;
        if (lcx >= 0 && lcx < FLAT_RENDER_CHUNKS && lcz >= 0 && lcz < FLAT_RENDER_CHUNKS) {
            g_flat_world_chunk_modified[lcz][lcx] = 0;
        }
    }
}

static void pex_restore_snapshot_chunk_modified_flags(const PexSaveSnapshot *ss) {
    if (!ss) return;
    int base_cx = pex_save_floor_div16(g_flat_world_origin_x);
    int base_cz = pex_save_floor_div16(g_flat_world_origin_z);
    for (int i = 0; i < ss->chunk_count; i++) {
        int lcx = ss->chunks[i].cx - base_cx;
        int lcz = ss->chunks[i].cz - base_cz;
        if (lcx >= 0 && lcx < FLAT_RENDER_CHUNKS && lcz >= 0 && lcz < FLAT_RENDER_CHUNKS) {
            g_flat_world_chunk_modified[lcz][lcx] = 1;
        }
    }
}

static void pex_snapshot_chunk_delta_path(const PexSaveSnapshot *ss, int cx, int cz, char *out, size_t cap) {
    if (!ss || !ss->loaded_world_dir[0]) {
        if (cap) out[0] = 0;
        return;
    }
    char bx[32], bz[32];
    base36_i32(cx, bx, sizeof(bx));
    base36_i32(cz, bz, sizeof(bz));
    char dir[MAX_PATHBUF];
    snprintf(dir, sizeof(dir), "%s\\chunks", ss->loaded_world_dir);
    make_dir_recursive(dir);
    snprintf(out, cap, "%s\\c.%s.%s.dat", dir, bx, bz);
}

static void pex_save_snapshot_one_modified_flat_chunk(PexSaveSnapshot *ss, PexSaveChunkSnapshot *cs) {
    if (!ss || !cs || !ss->loaded_world_dir[0] || !cs->blocks || !cs->meta) return;

    int cx = cs->cx;
    int cz = cs->cz;
    unsigned char *base = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
    if (!base) return;

    generate_flat_chunk_base_to_buffer_ex(cx, cz, base, ss->world_type, ss->world_seed, NULL);

    char path[MAX_PATHBUF];
    pex_snapshot_chunk_delta_path(ss, cx, cz, path, sizeof(path));
    int meta_nonzero = 0;
    for (int i = 0; i < FLAT_CHUNK_BLOCK_COUNT; i++) { if (cs->meta[i]) { meta_nonzero = 1; break; } }
    if (memcmp(base, cs->blocks, FLAT_CHUNK_BLOCK_COUNT) == 0 && !meta_nonzero) {
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
            fwrite(cs->blocks, 1, FLAT_CHUNK_BLOCK_COUNT, f);
            fwrite(cs->meta, 1, FLAT_CHUNK_BLOCK_COUNT, f);
            fclose(f);
        }
    }

    free(base);
}

static void pex_save_snapshot_modified_flat_chunks(PexSaveSnapshot *ss) {
    if (!ss || !ss->loaded_world_dir[0]) return;
    for (int i = 0; i < ss->chunk_count; i++) {
        pex_save_snapshot_one_modified_flat_chunk(ss, &ss->chunks[i]);
    }
}

static void pex_save_snapshot_world_state(PexSaveSnapshot *ss) {
    if (!ss || !ss->loaded_world_dir[0]) return;

    migrate_legacy_chunk_folder(ss->loaded_world_dir);
    pex_save_snapshot_modified_flat_chunks(ss);

    if (!ss->write_world_state) return;

    char path[MAX_PATHBUF];
    level_path_for_dir(ss->loaded_world_dir, path, sizeof(path));
    FILE *f = fopen(path, "wb");
    if (!f) return;

    char magic[8] = {'L','E','V','E','L','S','T','1'};
    int version = 16;
    int w = FLAT_WORLD_SIZE;
    int h = FLAT_WORLD_HEIGHT;
    int y_min = FLAT_WORLD_Y_MIN;
    fwrite(magic, 1, 8, f);
    fwrite(&version, sizeof(version), 1, f);
    fwrite(&w, sizeof(w), 1, f);
    fwrite(&h, sizeof(h), 1, f);
    fwrite(&y_min, sizeof(y_min), 1, f);

    fwrite(&ss->world_seed, sizeof(ss->world_seed), 1, f);
    fwrite(&ss->player_x, sizeof(ss->player_x), 1, f);
    fwrite(&ss->player_y, sizeof(ss->player_y), 1, f);
    fwrite(&ss->player_z, sizeof(ss->player_z), 1, f);
    fwrite(&ss->player_yaw, sizeof(ss->player_yaw), 1, f);
    fwrite(&ss->player_pitch, sizeof(ss->player_pitch), 1, f);
    fwrite(&ss->player_health, sizeof(ss->player_health), 1, f);
    fwrite(&ss->player_armor, sizeof(ss->player_armor), 1, f);
    fwrite(&ss->selected_hotbar_slot, sizeof(ss->selected_hotbar_slot), 1, f);
    fwrite(&ss->player_fall_distance, sizeof(ss->player_fall_distance), 1, f);
    fwrite(&ss->player_dead, sizeof(ss->player_dead), 1, f);
    fwrite(&ss->world_type, sizeof(ss->world_type), 1, f);
    fwrite(&ss->flat_world_origin_x, sizeof(ss->flat_world_origin_x), 1, f);
    fwrite(&ss->flat_world_origin_z, sizeof(ss->flat_world_origin_z), 1, f);

    for (int i = 0; i < 36; i++) {
        fwrite(&ss->inventory[i].id, sizeof(int), 1, f);
        fwrite(&ss->inventory[i].count, sizeof(int), 1, f);
        fwrite(&ss->inventory[i].damage, sizeof(int), 1, f);
    }

    ItemStack empty_legacy_chest[27];
    memset(empty_legacy_chest, 0, sizeof(empty_legacy_chest));
    for (int i = 0; i < 27; i++) {
        fwrite(&empty_legacy_chest[i].id, sizeof(int), 1, f);
        fwrite(&empty_legacy_chest[i].count, sizeof(int), 1, f);
        fwrite(&empty_legacy_chest[i].damage, sizeof(int), 1, f);
    }

    int chest_count = 0;
    for (int c = 0; c < MAX_CHEST_TILES; c++) {
        if (ss->chest_tiles[c].active) chest_count++;
    }
    fwrite(&chest_count, sizeof(chest_count), 1, f);
    for (int c = 0; c < MAX_CHEST_TILES; c++) {
        ChestTile *ct = &ss->chest_tiles[c];
        if (!ct->active) continue;
        fwrite(&ct->x, sizeof(int), 1, f);
        fwrite(&ct->y, sizeof(int), 1, f);
        fwrite(&ct->z, sizeof(int), 1, f);
        for (int i = 0; i < 27; i++) {
            fwrite(&ct->slots[i].id, sizeof(int), 1, f);
            fwrite(&ct->slots[i].count, sizeof(int), 1, f);
            fwrite(&ct->slots[i].damage, sizeof(int), 1, f);
        }
    }

    int furnace_count = 0;
    for (int c = 0; c < MAX_FURNACE_TILES; c++) {
        if (ss->furnace_tiles[c].active) furnace_count++;
    }
    fwrite(&furnace_count, sizeof(furnace_count), 1, f);
    for (int c = 0; c < MAX_FURNACE_TILES; c++) {
        FurnaceTile *ft = &ss->furnace_tiles[c];
        if (!ft->active) continue;
        fwrite(&ft->x, sizeof(int), 1, f);
        fwrite(&ft->y, sizeof(int), 1, f);
        fwrite(&ft->z, sizeof(int), 1, f);
        fwrite(&ft->burn_time, sizeof(int), 1, f);
        fwrite(&ft->current_item_burn_time, sizeof(int), 1, f);
        fwrite(&ft->cook_time, sizeof(int), 1, f);
        for (int i = 0; i < 3; i++) {
            fwrite(&ft->slots[i].id, sizeof(int), 1, f);
            fwrite(&ft->slots[i].count, sizeof(int), 1, f);
            fwrite(&ft->slots[i].damage, sizeof(int), 1, f);
        }
    }

    int drop_count = 0;
    for (int i = 0; i < MAX_DROP_ENTITIES; i++) {
        if (ss->drops[i].active && ss->drops[i].stack.id > 0 && ss->drops[i].stack.count > 0) drop_count++;
    }
    fwrite(&drop_count, sizeof(drop_count), 1, f);
    for (int i = 0; i < MAX_DROP_ENTITIES; i++) {
        FlatDroppedItem *e = &ss->drops[i];
        if (!e->active || e->stack.id <= 0 || e->stack.count <= 0) continue;
        fwrite(&e->stack.id, sizeof(int), 1, f);
        fwrite(&e->stack.count, sizeof(int), 1, f);
        fwrite(&e->stack.damage, sizeof(int), 1, f);
        fwrite(&e->x, sizeof(float), 1, f);
        fwrite(&e->y, sizeof(float), 1, f);
        fwrite(&e->z, sizeof(float), 1, f);
        fwrite(&e->mx, sizeof(float), 1, f);
        fwrite(&e->my, sizeof(float), 1, f);
        fwrite(&e->mz, sizeof(float), 1, f);
        fwrite(&e->rot, sizeof(float), 1, f);
        fwrite(&e->age, sizeof(int), 1, f);
        fwrite(&e->pickup_delay, sizeof(int), 1, f);
    }

    passive_mobs_write_to_file(f, ss->passive_mobs);

    fclose(f);
    write_session_lock(ss->loaded_world_dir);
    cleanup_legacy_world_metadata(ss->loaded_world_dir);
}

static DWORD WINAPI pex_save_thread_proc(LPVOID param) {
    (void)param;
    for (;;) {
        WaitForSingleObject(g_save_event, INFINITE);
        for (;;) {
            PexSaveSnapshot *ss = NULL;
            int should_exit = 0;

            EnterCriticalSection(&g_save_cs);
            ss = g_save_queue_head;
            if (ss) {
                g_save_queue_head = ss->next;
                if (!g_save_queue_head) g_save_queue_tail = NULL;
                ss->next = NULL;
            } else {
                if (g_save_thread_shutdown) should_exit = 1;
                g_save_thread_done = 1;
            }
            LeaveCriticalSection(&g_save_cs);

            if (ss) {
                pex_save_snapshot_world_state(ss);
                pex_save_snapshot_free(ss);
                continue;
            }
            if (should_exit) return 0;
            break;
        }
    }
}

static int pex_save_worker_ensure_started(void) {
    if (!g_save_event) {
        g_save_event = CreateEventA(NULL, FALSE, FALSE, NULL);
        if (!g_save_event) return 0;
    }
    if (!g_save_thread) {
        g_save_thread_shutdown = 0;
        g_save_thread_done = 1;
        g_save_thread = CreateThread(NULL, 0, pex_save_thread_proc, NULL, 0, NULL);
        if (!g_save_thread) return 0;
        SetThreadPriority(g_save_thread, THREAD_PRIORITY_BELOW_NORMAL);
    }
    return 1;
}

static int pex_save_enqueue_snapshot(PexSaveSnapshot *ss) {
    if (!ss) return 0;
    if (!pex_save_worker_ensure_started()) return 0;

    EnterCriticalSection(&g_save_cs);
    ss->next = NULL;
    if (g_save_queue_tail) g_save_queue_tail->next = ss;
    else g_save_queue_head = ss;
    g_save_queue_tail = ss;
    g_save_thread_done = 0;
    LeaveCriticalSection(&g_save_cs);

    SetEvent(g_save_event);
    return 1;
}

static void pex_request_modified_chunk_save_async(void) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    return;
#endif
    if (!g_loaded_world_dir[0]) return;

    PexSaveSnapshot *ss = pex_save_snapshot_create(0);
    if (!ss) return;
    if (ss->chunk_count <= 0) {
        pex_save_snapshot_free(ss);
        return;
    }

    pex_clear_snapshot_chunk_modified_flags(ss);
    if (!pex_save_enqueue_snapshot(ss)) {
        pex_restore_snapshot_chunk_modified_flags(ss);
        pex_save_snapshot_free(ss);
    }
}

static void save_current_world_state_sync(void) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    g_save_dirty = 0;
    g_last_autosave_tick = g_ingame_ticks;
    g_save_message_ticks = 0;
    return;
#endif
    if (!g_loaded_world_dir[0]) return;

    /* Terrain comes from the seed + generator kind.  Only edited chunks are
       stored separately; the player/inventory/world metadata now lives in level.dat. */
    migrate_legacy_chunk_folder(g_loaded_world_dir);
    save_modified_flat_chunks_sync();

    char path[MAX_PATHBUF];
    world_state_path(path, sizeof(path));
    FILE *f = fopen(path, "wb");
    if (!f) return;

    char magic[8] = {'L','E','V','E','L','S','T','1'};
    int version = 16;
    int w = FLAT_WORLD_SIZE;
    int h = FLAT_WORLD_HEIGHT;
    int y_min = FLAT_WORLD_Y_MIN;
    fwrite(magic, 1, 8, f);
    fwrite(&version, sizeof(version), 1, f);
    fwrite(&w, sizeof(w), 1, f);
    fwrite(&h, sizeof(h), 1, f);
    fwrite(&y_min, sizeof(y_min), 1, f);

    fwrite(&g_world_seed, sizeof(g_world_seed), 1, f);
    fwrite(&g_player_x, sizeof(g_player_x), 1, f);
    fwrite(&g_player_y, sizeof(g_player_y), 1, f);
    fwrite(&g_player_z, sizeof(g_player_z), 1, f);
    fwrite(&g_player_yaw, sizeof(g_player_yaw), 1, f);
    fwrite(&g_player_pitch, sizeof(g_player_pitch), 1, f);
    fwrite(&g_player_health, sizeof(g_player_health), 1, f);
    fwrite(&g_player_armor, sizeof(g_player_armor), 1, f);
    fwrite(&g_selected_hotbar_slot, sizeof(g_selected_hotbar_slot), 1, f);
    fwrite(&g_player_fall_distance, sizeof(g_player_fall_distance), 1, f);
    fwrite(&g_player_dead, sizeof(g_player_dead), 1, f);
    fwrite(&g_world_type, sizeof(g_world_type), 1, f);
    fwrite(&g_flat_world_origin_x, sizeof(g_flat_world_origin_x), 1, f);
    fwrite(&g_flat_world_origin_z, sizeof(g_flat_world_origin_z), 1, f);

    for (int i = 0; i < 36; i++) {
        fwrite(&g_inventory[i].id, sizeof(int), 1, f);
        fwrite(&g_inventory[i].count, sizeof(int), 1, f);
        fwrite(&g_inventory[i].damage, sizeof(int), 1, f);
    }
    /* Legacy shared chest payload retained as zeroed padding for v9-v13 save
       migration. v14+ uses coordinate-bound ChestTile records below. */
    ItemStack empty_legacy_chest[27];
    memset(empty_legacy_chest, 0, sizeof(empty_legacy_chest));
    for (int i = 0; i < 27; i++) {
        fwrite(&empty_legacy_chest[i].id, sizeof(int), 1, f);
        fwrite(&empty_legacy_chest[i].count, sizeof(int), 1, f);
        fwrite(&empty_legacy_chest[i].damage, sizeof(int), 1, f);
    }

    int chest_count = 0;
    for (int c = 0; c < MAX_CHEST_TILES; c++) {
        if (g_chest_tiles[c].active) chest_count++;
    }
    fwrite(&chest_count, sizeof(chest_count), 1, f);
    for (int c = 0; c < MAX_CHEST_TILES; c++) {
        ChestTile *ct = &g_chest_tiles[c];
        if (!ct->active) continue;
        fwrite(&ct->x, sizeof(int), 1, f);
        fwrite(&ct->y, sizeof(int), 1, f);
        fwrite(&ct->z, sizeof(int), 1, f);
        for (int i = 0; i < 27; i++) {
            fwrite(&ct->slots[i].id, sizeof(int), 1, f);
            fwrite(&ct->slots[i].count, sizeof(int), 1, f);
            fwrite(&ct->slots[i].damage, sizeof(int), 1, f);
        }
    }

    int furnace_count = 0;
    for (int c = 0; c < MAX_FURNACE_TILES; c++) {
        if (g_furnace_tiles[c].active) furnace_count++;
    }
    fwrite(&furnace_count, sizeof(furnace_count), 1, f);
    for (int c = 0; c < MAX_FURNACE_TILES; c++) {
        FurnaceTile *ft = &g_furnace_tiles[c];
        if (!ft->active) continue;
        fwrite(&ft->x, sizeof(int), 1, f);
        fwrite(&ft->y, sizeof(int), 1, f);
        fwrite(&ft->z, sizeof(int), 1, f);
        fwrite(&ft->burn_time, sizeof(int), 1, f);
        fwrite(&ft->current_item_burn_time, sizeof(int), 1, f);
        fwrite(&ft->cook_time, sizeof(int), 1, f);
        for (int i = 0; i < 3; i++) {
            fwrite(&ft->slots[i].id, sizeof(int), 1, f);
            fwrite(&ft->slots[i].count, sizeof(int), 1, f);
            fwrite(&ft->slots[i].damage, sizeof(int), 1, f);
        }
    }

    int drop_count = 0;
    for (int i = 0; i < MAX_DROP_ENTITIES; i++) {
        if (g_drops[i].active && g_drops[i].stack.id > 0 && g_drops[i].stack.count > 0) drop_count++;
    }
    fwrite(&drop_count, sizeof(drop_count), 1, f);
    for (int i = 0; i < MAX_DROP_ENTITIES; i++) {
        FlatDroppedItem *e = &g_drops[i];
        if (!e->active || e->stack.id <= 0 || e->stack.count <= 0) continue;
        fwrite(&e->stack.id, sizeof(int), 1, f);
        fwrite(&e->stack.count, sizeof(int), 1, f);
        fwrite(&e->stack.damage, sizeof(int), 1, f);
        fwrite(&e->x, sizeof(float), 1, f);
        fwrite(&e->y, sizeof(float), 1, f);
        fwrite(&e->z, sizeof(float), 1, f);
        fwrite(&e->mx, sizeof(float), 1, f);
        fwrite(&e->my, sizeof(float), 1, f);
        fwrite(&e->mz, sizeof(float), 1, f);
        fwrite(&e->rot, sizeof(float), 1, f);
        fwrite(&e->age, sizeof(int), 1, f);
        fwrite(&e->pickup_delay, sizeof(int), 1, f);
    }

    passive_mobs_write_to_file(f, g_passive_mobs);

    fclose(f);
    write_session_lock(g_loaded_world_dir);
    cleanup_legacy_world_metadata(g_loaded_world_dir);
    g_save_dirty = 0;
    g_last_autosave_tick = g_ingame_ticks;
    g_save_message_ticks = SAVE_MESSAGE_TICKS;
}

static void save_current_world_state(void) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    g_save_dirty = 0;
    g_last_autosave_tick = g_ingame_ticks;
    g_save_message_ticks = 0;
    return;
#endif
    if (!g_loaded_world_dir[0]) return;

    PexSaveSnapshot *ss = pex_save_snapshot_create(1);
    if (!ss) return;

    pex_clear_snapshot_chunk_modified_flags(ss);
    g_save_dirty = 0;
    g_last_autosave_tick = g_ingame_ticks;
    g_save_message_ticks = SAVE_MESSAGE_TICKS;

    if (!pex_save_enqueue_snapshot(ss)) {
        pex_restore_snapshot_chunk_modified_flags(ss);
        g_save_dirty = 1;
        pex_save_snapshot_free(ss);
    }
}


static int load_current_world_state(void) {
    g_last_load_state_had_terrain = 0;
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    return 0;
#endif
    if (!g_loaded_world_dir[0]) return 0;

    FILE *f = open_current_world_state_file();
    if (!f) return 0;

    char magic[8];
    int version = 0, w = 0, h = 0, y_min = 0;
    if (fread(magic, 1, 8, f) != 8 ||
        fread(&version, sizeof(version), 1, f) != 1 ||
        fread(&w, sizeof(w), 1, f) != 1 ||
        fread(&h, sizeof(h), 1, f) != 1 ||
        fread(&y_min, sizeof(y_min), 1, f) != 1) {
        fclose(f);
        return 0;
    }

    int ok_magic =
        (memcmp(magic, "LEVELST1", 8) == 0 && (version == 13 || version == 14 || version == 15 || version == 16)) ||
        (memcmp(magic, "PXCFLAT4", 8) == 0 && version == 4) ||
        (memcmp(magic, "PXCFLAT6", 8) == 0 && version == 6) ||
        (memcmp(magic, "PXCFLAT7", 8) == 0 && version == 7) ||
        (memcmp(magic, "PXCFLAT9", 8) == 0 && version == 9) ||
        (memcmp(magic, "PXCFLT10", 8) == 0 && version == 10) ||
        (memcmp(magic, "PXCFLT11", 8) == 0 && version == 11) ||
        (memcmp(magic, "PXCFLT12", 8) == 0 && version == 12);
    /* v12+ level.dat no longer stores the full active terrain window; it only
       stores metadata/inventory/chests and reloads terrain from seed + chunk
       deltas.  Accept saves made with the old 256-wide active window so this
       performance hotfix does not wipe inventories/chests just because the
       in-memory cache size changed. */
    if (!ok_magic || h != FLAT_WORLD_HEIGHT || y_min != FLAT_WORLD_Y_MIN ||
        (w != FLAT_WORLD_SIZE && version < 12)) {
        fclose(f);
        return 0;
    }

    chest_clear_all_tiles();
    furnace_clear_all_tiles();
    passive_mobs_reset();
    memset(g_chest_slots, 0, sizeof(g_chest_slots));
    g_legacy_global_chest_pending = 0;

    if (version >= 12) {
        fread(&g_world_seed, sizeof(g_world_seed), 1, f);
    } else {
        long long seed = 0;
        if (read_world_seed_for_dir(g_loaded_world_dir, &seed)) g_world_seed = seed;
    }

    if (fread(&g_player_x, sizeof(g_player_x), 1, f) != 1 ||
        fread(&g_player_y, sizeof(g_player_y), 1, f) != 1 ||
        fread(&g_player_z, sizeof(g_player_z), 1, f) != 1 ||
        fread(&g_player_yaw, sizeof(g_player_yaw), 1, f) != 1 ||
        fread(&g_player_pitch, sizeof(g_player_pitch), 1, f) != 1 ||
        fread(&g_player_health, sizeof(g_player_health), 1, f) != 1 ||
        fread(&g_player_armor, sizeof(g_player_armor), 1, f) != 1 ||
        fread(&g_selected_hotbar_slot, sizeof(g_selected_hotbar_slot), 1, f) != 1) {
        fclose(f);
        return 0;
    }

    if (version >= 6) {
        fread(&g_player_fall_distance, sizeof(g_player_fall_distance), 1, f);
        fread(&g_player_dead, sizeof(g_player_dead), 1, f);
        if (version >= 7) fread(&g_world_type, sizeof(g_world_type), 1, f);
        if (version >= 11) {
            fread(&g_flat_world_origin_x, sizeof(g_flat_world_origin_x), 1, f);
            fread(&g_flat_world_origin_z, sizeof(g_flat_world_origin_z), 1, f);
        }
    } else {
        g_player_fall_distance = 0.0f;
        g_player_dead = 0;
    }

    for (int i = 0; i < 36; i++) {
        if (fread(&g_inventory[i].id, sizeof(int), 1, f) != 1 ||
            fread(&g_inventory[i].count, sizeof(int), 1, f) != 1 ||
            fread(&g_inventory[i].damage, sizeof(int), 1, f) != 1) {
            fclose(f);
            return 0;
        }
        g_inventory[i].pop_time = 0;
        if (g_inventory[i].count <= 0) memset(&g_inventory[i], 0, sizeof(g_inventory[i]));
    }
    memset(g_chest_slots, 0, sizeof(g_chest_slots));
    if (version >= 9) {
        for (int i = 0; i < 27; i++) {
            if (fread(&g_chest_slots[i].id, sizeof(int), 1, f) != 1 ||
                fread(&g_chest_slots[i].count, sizeof(int), 1, f) != 1 ||
                fread(&g_chest_slots[i].damage, sizeof(int), 1, f) != 1) {
                fclose(f);
                return 0;
            }
            g_chest_slots[i].pop_time = 0;
            if (g_chest_slots[i].count <= 0) memset(&g_chest_slots[i], 0, sizeof(g_chest_slots[i]));
        }
        if (version < 14 && chest_slots_have_items(g_chest_slots, 27)) {
            g_legacy_global_chest_pending = 1;
        }
    }

    if (version >= 14) {
        int chest_count = 0;
        if (fread(&chest_count, sizeof(chest_count), 1, f) != 1) {
            fclose(f);
            return 0;
        }
        if (chest_count < 0) chest_count = 0;
        if (chest_count > MAX_CHEST_TILES) chest_count = MAX_CHEST_TILES;
        for (int c = 0; c < chest_count; c++) {
            ChestTile *ct = &g_chest_tiles[c];
            memset(ct, 0, sizeof(*ct));
            ct->active = 1;
            if (fread(&ct->x, sizeof(int), 1, f) != 1 ||
                fread(&ct->y, sizeof(int), 1, f) != 1 ||
                fread(&ct->z, sizeof(int), 1, f) != 1) {
                fclose(f);
                return 0;
            }
            for (int i = 0; i < 27; i++) {
                if (fread(&ct->slots[i].id, sizeof(int), 1, f) != 1 ||
                    fread(&ct->slots[i].count, sizeof(int), 1, f) != 1 ||
                    fread(&ct->slots[i].damage, sizeof(int), 1, f) != 1) {
                    fclose(f);
                    return 0;
                }
                ct->slots[i].pop_time = 0;
                if (ct->slots[i].count <= 0) memset(&ct->slots[i], 0, sizeof(ct->slots[i]));
            }
        }
    }

    if (version >= 15) {
        int furnace_count = 0;
        if (fread(&furnace_count, sizeof(furnace_count), 1, f) != 1) {
            fclose(f);
            return 0;
        }
        if (furnace_count < 0) furnace_count = 0;
        if (furnace_count > MAX_FURNACE_TILES) furnace_count = MAX_FURNACE_TILES;
        for (int c = 0; c < furnace_count; c++) {
            FurnaceTile *ft = &g_furnace_tiles[c];
            memset(ft, 0, sizeof(*ft));
            ft->active = 1;
            if (fread(&ft->x, sizeof(int), 1, f) != 1 ||
                fread(&ft->y, sizeof(int), 1, f) != 1 ||
                fread(&ft->z, sizeof(int), 1, f) != 1 ||
                fread(&ft->burn_time, sizeof(int), 1, f) != 1 ||
                fread(&ft->current_item_burn_time, sizeof(int), 1, f) != 1 ||
                fread(&ft->cook_time, sizeof(int), 1, f) != 1) {
                fclose(f);
                return 0;
            }
            for (int i = 0; i < 3; i++) {
                if (fread(&ft->slots[i].id, sizeof(int), 1, f) != 1 ||
                    fread(&ft->slots[i].count, sizeof(int), 1, f) != 1 ||
                    fread(&ft->slots[i].damage, sizeof(int), 1, f) != 1) {
                    fclose(f);
                    return 0;
                }
                ft->slots[i].pop_time = 0;
                if (ft->slots[i].count <= 0) memset(&ft->slots[i], 0, sizeof(ft->slots[i]));
            }
        }
    }

    if (version >= 12) {
        /* Rebuild the active window from seed + tiny edited-chunk deltas unless
           the loading screen is going to do it incrementally with progress. */
        if (!g_load_state_skip_terrain_rebuild) {
            flat_world_generate_blocks_for_current_origin();
            g_last_load_state_had_terrain = 1;
        }
    } else {
        if (fread(g_flat_blocks, sizeof(g_flat_blocks), 1, f) != 1) {
            fclose(f);
            return 0;
        }
        /* Old saves contain a full active-window block dump.  Mark every chunk so
           the next save converts the bloated file into delta chunks. */
        mark_flat_chunks_modified_all();
        for (int lcz = 0; lcz < FLAT_RENDER_CHUNKS; lcz++)
            for (int lcx = 0; lcx < FLAT_RENDER_CHUNKS; lcx++)
                g_flat_world_chunk_generated[lcz][lcx] = 1;
        mark_flat_render_chunks_dirty_all();
        g_last_load_state_had_terrain = 1;
    }

    memset(g_drops, 0, sizeof(g_drops));
    if (version >= 6) {
        int drop_count = 0;
        if (fread(&drop_count, sizeof(drop_count), 1, f) == 1) {
            if (drop_count < 0) drop_count = 0;
            if (drop_count > MAX_DROP_ENTITIES) drop_count = MAX_DROP_ENTITIES;
            for (int i = 0; i < drop_count; i++) {
                FlatDroppedItem *e = &g_drops[i];
                memset(e, 0, sizeof(*e));
                if (fread(&e->stack.id, sizeof(int), 1, f) != 1 ||
                    fread(&e->stack.count, sizeof(int), 1, f) != 1 ||
                    fread(&e->stack.damage, sizeof(int), 1, f) != 1 ||
                    fread(&e->x, sizeof(float), 1, f) != 1 ||
                    fread(&e->y, sizeof(float), 1, f) != 1 ||
                    fread(&e->z, sizeof(float), 1, f) != 1 ||
                    fread(&e->mx, sizeof(float), 1, f) != 1 ||
                    fread(&e->my, sizeof(float), 1, f) != 1 ||
                    fread(&e->mz, sizeof(float), 1, f) != 1 ||
                    fread(&e->rot, sizeof(float), 1, f) != 1 ||
                    fread(&e->age, sizeof(int), 1, f) != 1 ||
                    fread(&e->pickup_delay, sizeof(int), 1, f) != 1) {
                    memset(e, 0, sizeof(*e));
                    break;
                }
                if (e->stack.id > 0 && e->stack.count > 0) {
                    e->active = 1;
                    e->prev_x = e->x;
                    e->prev_y = e->y;
                    e->prev_z = e->z;
                }
            }
        }
    }
    passive_mobs_read_from_file(f, version);
    g_passive_mobs_need_initial_spawn = (version < 16) ? 1 : 0;
    fclose(f);

    if (version < 11) { g_flat_world_origin_x = -(FLAT_WORLD_SIZE / 2); g_flat_world_origin_z = -(FLAT_WORLD_SIZE / 2); }
    if (g_selected_hotbar_slot < 0 || g_selected_hotbar_slot > 8) g_selected_hotbar_slot = 0;
    if (g_player_health <= 0) g_player_dead = 1;
    g_player_prev_x = g_player_x;
    g_player_prev_y = g_player_y;
    g_player_prev_z = g_player_z;
    g_player_prev_yaw = g_player_yaw;
    g_player_prev_pitch = g_player_pitch;
    g_player_motion_x = g_player_motion_y = g_player_motion_z = 0.0f;
    memset(g_craft_grid, 0, sizeof(g_craft_grid));
    memset(&g_carried_stack, 0, sizeof(g_carried_stack));
    g_flat_world_geometry_dirty = 0;
    g_flat_section_geometry_dirty = 0;
    g_save_dirty = (version < 16) ? 1 : 0;
    return 1;
}


static void finish_prepared_world_entry(int loaded_state) {
    g_worldgen.active = 0;
    if (!loaded_state) {
        reset_flat_player_spawn();
        passive_mobs_reset();
        passive_mobs_spawn_initial();
        g_passive_mobs_need_initial_spawn = 0;
    } else if (!g_player_dead &&
        (flat_player_aabb_collides(g_player_x, g_player_y, g_player_z) || flat_player_suffocation_block())) {
        reset_flat_player_spawn();
    }

    if (loaded_state && g_passive_mobs_need_initial_spawn) {
        passive_mobs_spawn_initial();
        g_passive_mobs_need_initial_spawn = 0;
    }

    g_chat_input[0] = 0;
    g_chat_count = 0;
    g_last_autosave_tick = g_ingame_ticks;
    g_save_message_ticks = 0;
    hud_add_chat(loaded_state ? "Loaded saved world." : "Loaded world.");
    if (g_player_dead || g_player_health <= 0) {
        g_player_dead = 1;
        g_player_health = 0;
        g_player_motion_x = g_player_motion_y = g_player_motion_z = 0.0f;
        set_screen(SCREEN_DEATH);
    } else {
        set_screen(SCREEN_INGAME);
    }
}

static void enter_world_from_job(void) {
    /* Fallback path kept for older callers.  Normal loading now prepares terrain
       on the progress screen and enters through finish_prepared_world_entry(). */
    snprintf(g_loaded_world_dir, sizeof(g_loaded_world_dir), "%s", g_worldgen.world_dir);
    snprintf(g_loaded_world_name, sizeof(g_loaded_world_name), "%s", g_worldgen.world_name[0] ? g_worldgen.world_name : "World");
    g_selected_hotbar_slot = 0;
    g_world_type = read_world_type_for_dir(g_loaded_world_dir);
    {
        long long seed = 0;
        if (read_world_seed_for_dir(g_loaded_world_dir, &seed)) g_world_seed = seed;
    }
    migrate_legacy_chunk_folder(g_loaded_world_dir);

    int loaded_state = load_current_world_state();
    if (!loaded_state) {
        inventory_reset();
        g_player_health = 20;
        g_player_prev_health = 20;
        g_player_armor = 0;
        g_ingame_ticks = 0;
        flat_world_center_origin_near(g_player_x, g_player_z);
        flat_world_generate_blocks_for_current_origin();
    }
    finish_prepared_world_entry(loaded_state);
}


static void leave_world_to_title(void) {
    if (g_mp_connected) {
        pex_net_disconnect();
    }
#if !(defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY)
    if (!g_mp_connected && g_loaded_world_dir[0]) {
        save_current_world_state();
        write_session_lock(g_loaded_world_dir);
    }
#endif
    g_loaded_world_dir[0] = 0;
    g_loaded_world_name[0] = 0;
    g_chat_input[0] = 0;
    set_screen(SCREEN_TITLE);
}

static void start_world_generation(int slot) {
    memset(&g_worldgen, 0, sizeof(g_worldgen));
    g_worldgen.active = 1;
    g_worldgen.slot = slot;
    g_worldgen.radius = 4;
    g_worldgen.total_chunks = (g_worldgen.radius * 2 + 1) * (g_worldgen.radius * 2 + 1);
    g_worldgen.seed = ((long long)time(NULL) << 32) ^ (long long)GetTickCount();
    g_world_seed = g_worldgen.seed;
    g_world_type = g_pending_world_type;
    snprintf(g_worldgen.world_name, sizeof(g_worldgen.world_name), "World%d", slot);
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    snprintf(g_worldgen.world_dir, sizeof(g_worldgen.world_dir), "memory:/World%d", slot);
#elif defined(PEX_PLATFORM_WII)
    /* Dolphin/homebrew can run without an SD image.  In that case do not touch
       FAT at all while creating/loading worlds; generate an in-RAM world like
       the PSP memory build. */
    if (!g_wii_fat_ready) snprintf(g_worldgen.world_dir, sizeof(g_worldgen.world_dir), "memory:/World%d", slot);
    else snprintf(g_worldgen.world_dir, sizeof(g_worldgen.world_dir), "%s\\World%d", g_save_dir, slot);
#else
    snprintf(g_worldgen.world_dir, sizeof(g_worldgen.world_dir), "%s\\World%d", g_save_dir, slot);
#endif
    snprintf(g_worldgen.title, sizeof(g_worldgen.title), "Generating level");
    snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Building terrain");
    g_worldgen.progress = 0;
    g_worldgen.phase = 0;
    g_worldgen.loaded_state = 0;
    g_worldgen.terrain_total = 0;
    g_worldgen.terrain_done = 0;

#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    /* PSP test port: no Memory Stick persistence.  Generate into RAM only. */
    g_worldgen.existing_world = 0;
    snprintf(g_worldgen.title, sizeof(g_worldgen.title), "Generating RAM world");
    snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Building terrain");
#else
#if defined(PEX_PLATFORM_WII)
    if (!g_wii_fat_ready) {
        g_worldgen.existing_world = 0;
        snprintf(g_worldgen.title, sizeof(g_worldgen.title), "Generating RAM world");
        snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Building terrain");
        wii_debug_logf("worldgen: FAT unavailable, using memory world slot=%d type=%d", slot, g_pending_world_type);
    } else
#endif
    {
        char level_path[MAX_PATHBUF];
        snprintf(level_path, sizeof(level_path), "%s\\level.dat", g_worldgen.world_dir);
        if (file_exists(level_path)) {
            g_worldgen.existing_world = 1;
            snprintf(g_worldgen.title, sizeof(g_worldgen.title), "Loading level");
            snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Loading chunks");
        } else {
            /* Create the world slot.  Terrain is generated from level.dat metadata
               at load/stream time; only chunks edited by gameplay are saved. */
            if (dir_exists(g_worldgen.world_dir)) delete_recursive(g_worldgen.world_dir);
            make_dir_recursive(g_worldgen.world_dir);
            write_session_lock(g_worldgen.world_dir);
            /* level.dat contains the seed and generator kind.  Terrain itself is
               generated from that metadata, and only gameplay-edited chunks are saved. */
            write_level_dat(g_worldgen.world_dir, g_worldgen.world_name, g_worldgen.seed, 50, 5, 50, 0);
            g_worldgen.existing_world = 1;
            snprintf(g_worldgen.title, sizeof(g_worldgen.title), "Loading level");
            snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Building terrain");
        }
    }
#endif
#if defined(PEX_PLATFORM_WII)
    wii_debug_logf("worldgen start: slot=%d type=%d dir=%s title=%s", slot, g_world_type, g_worldgen.world_dir, g_worldgen.title);
#endif
    set_screen(SCREEN_GENERATING);
}

static void worldgen_tick(void) {
    if (!g_worldgen.active) return;
#if defined(PEX_PLATFORM_WII)
    static int s_wii_last_worldgen_phase = -999;
    if (s_wii_last_worldgen_phase != g_worldgen.phase) {
        s_wii_last_worldgen_phase = g_worldgen.phase;
        wii_debug_logf("worldgen tick phase=%d progress=%d type=%d queue=%d/%d screen=%d",
                       g_worldgen.phase, g_worldgen.progress, g_world_type,
                       g_stream_gen_queue_index, g_stream_gen_queue_count, g_screen);
        wii_debug_memoryf("worldgen phase change");
    }
#endif

    if (g_worldgen.phase == 0) {
        snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Reading level");
        g_worldgen.progress = 2;

        snprintf(g_loaded_world_dir, sizeof(g_loaded_world_dir), "%s", g_worldgen.world_dir);
        snprintf(g_loaded_world_name, sizeof(g_loaded_world_name), "%s",
                 g_worldgen.world_name[0] ? g_worldgen.world_name : "World");
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
        g_world_type = g_pending_world_type;
        g_world_seed = g_worldgen.seed;
        g_worldgen.loaded_state = 0;
#else
#if defined(PEX_PLATFORM_WII)
        if (!g_wii_fat_ready || strncmp(g_loaded_world_dir, "memory:", 7) == 0) {
            g_world_type = g_pending_world_type;
            g_world_seed = g_worldgen.seed;
            g_worldgen.loaded_state = 0;
            wii_debug_logf("worldgen phase0: memory world type=%d seed=%lld", g_world_type, (long long)g_world_seed);
        } else
#endif
        {
            g_world_type = read_world_type_for_dir(g_loaded_world_dir);
            {
                long long seed = 0;
                if (read_world_seed_for_dir(g_loaded_world_dir, &seed)) g_world_seed = seed;
                else g_world_seed = g_worldgen.seed;
            }
            migrate_legacy_chunk_folder(g_loaded_world_dir);

            g_load_state_skip_terrain_rebuild = 1;
            g_worldgen.loaded_state = load_current_world_state();
            g_load_state_skip_terrain_rebuild = 0;
        }
#endif

        if (!g_worldgen.loaded_state) {
            inventory_reset();
            g_player_health = 20;
            g_player_prev_health = 20;
            g_player_armor = 0;
            g_player_dead = 0;
            g_player_fall_distance = 0.0f;
            g_ingame_ticks = 0;
            g_player_x = g_player_prev_x = 0.5f;
            g_player_y = g_player_prev_y = 5.62f;
            g_player_z = g_player_prev_z = 0.5f;
            flat_world_center_origin_near(g_player_x, g_player_z);
        } else if (g_last_load_state_had_terrain) {
            g_worldgen.phase = 2;
            g_worldgen.progress = 92;
            snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Preparing chunks");
            worldgen_mesh_prep_reset();
            return;
        }

        flat_world_prepare_initial_generation();
        flat_world_begin_initial_generation();
        g_worldgen.terrain_total = flat_world_initial_generation_total();
        g_worldgen.terrain_done = flat_world_initial_generation_done();
        g_worldgen.phase = 1;
        snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Building terrain %d/%d",
                 g_worldgen.terrain_done, g_worldgen.terrain_total);
        g_worldgen.progress = 5;
        return;
    }

    if (g_worldgen.phase == 1) {
        flat_world_continue_initial_generation();
        g_worldgen.terrain_total = flat_world_initial_generation_total();
        g_worldgen.terrain_done = flat_world_initial_generation_done();
        if (g_worldgen.terrain_total <= 0) g_worldgen.terrain_total = 1;
        int terrain_p = (g_worldgen.terrain_done * 82) / g_worldgen.terrain_total;
        g_worldgen.progress = 5 + terrain_p;
        if (g_worldgen.progress > 87) g_worldgen.progress = 87;
        snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Building terrain %d/%d",
                 g_worldgen.terrain_done, g_worldgen.terrain_total);
        if (!flat_world_initial_generation_active()) {
            flat_world_finish_initial_generation();
            if (!g_worldgen.loaded_state) reset_flat_player_spawn();
            g_worldgen.phase = 2;
            g_worldgen.progress = 88;
            snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Preparing chunks");
            worldgen_mesh_prep_reset();
        }
        return;
    }

    if (g_worldgen.phase == 2) {
        int complete = worldgen_mesh_prep_step(8);
        int total = worldgen_mesh_prep_total();
        int done = worldgen_mesh_prep_done();
        if (total <= 0) total = 1;
        int mesh_p = (done * 11) / total;
        g_worldgen.progress = 88 + mesh_p;
        if (g_worldgen.progress > 99) g_worldgen.progress = 99;
        snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Preparing chunks %d/%d", done, total);
        if (complete) {
            g_worldgen.phase = 3;
            g_worldgen.progress = 100;
            snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Entering world");
        }
        return;
    }

    if (g_worldgen.phase == 3) {
        finish_prepared_world_entry(g_worldgen.loaded_state);
    }
}

