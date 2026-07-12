/* Split from original monolithic main.c. Included by src/main.c unity build. */

#if !defined(PEX_PLATFORM_PSP) && !defined(PEX_PLATFORM_WII)
#include <zlib.h>
#define PEX_SAVE_ZLIB_CHUNK_DELTAS 1
#else
#define PEX_SAVE_ZLIB_CHUNK_DELTAS 0
#endif

static void flat_generate_chunk_base(int cx, int cz, unsigned char *out, int world_type, long long seed, TerrainProvider *reuse_tp);
#ifndef FLAT_CHUNK_BLOCK_COUNT
#define FLAT_CHUNK_BLOCK_COUNT (FLAT_RENDER_CHUNK * FLAT_RENDER_CHUNK * FLAT_WORLD_HEIGHT)
#endif

static void pex_write_item_stack_125(FILE *f, const ItemStack *s) {
    ItemStack empty;
    if (!s) { memset(&empty, 0, sizeof(empty)); s = &empty; }
    fwrite(&s->id, sizeof(int), 1, f);
    fwrite(&s->count, sizeof(int), 1, f);
    fwrite(&s->damage, sizeof(int), 1, f);
    for (int i = 0; i < PEX_ITEMSTACK_ENCHANT_MAX; ++i) fwrite(&s->enchant_id[i], sizeof(int), 1, f);
    for (int i = 0; i < PEX_ITEMSTACK_ENCHANT_MAX; ++i) fwrite(&s->enchant_level[i], sizeof(int), 1, f);
}

static int pex_read_item_stack_125(FILE *f, ItemStack *s, int save_version) {
    if (!f || !s) return 0;
    memset(s, 0, sizeof(*s));
    if (fread(&s->id, sizeof(int), 1, f) != 1 ||
        fread(&s->count, sizeof(int), 1, f) != 1 ||
        fread(&s->damage, sizeof(int), 1, f) != 1) return 0;
    if (save_version >= 29) {
        for (int i = 0; i < PEX_ITEMSTACK_ENCHANT_MAX; ++i) if (fread(&s->enchant_id[i], sizeof(int), 1, f) != 1) return 0;
        for (int i = 0; i < PEX_ITEMSTACK_ENCHANT_MAX; ++i) if (fread(&s->enchant_level[i], sizeof(int), 1, f) != 1) return 0;
        for (int i = 0; i < PEX_ITEMSTACK_ENCHANT_MAX; ++i) {
            if (s->enchant_id[i] <= 0 || s->enchant_level[i] <= 0) { s->enchant_id[i] = 0; s->enchant_level[i] = 0; }
            if (s->enchant_level[i] > 10) s->enchant_level[i] = 10;
        }
    }
    s->pop_time = 0;
    return 1;
}



static void pex_write_projectile_125(FILE *f, const FlatProjectile *p) {
    fwrite(&p->type,sizeof(int),1,f); fwrite(&p->item_damage,sizeof(int),1,f);
    fwrite(&p->owner_type,sizeof(int),1,f); fwrite(&p->owner_mob_type,sizeof(int),1,f); fwrite(&p->owner_mob_index,sizeof(int),1,f);
    fwrite(&p->damage,sizeof(int),1,f); fwrite(&p->critical,sizeof(int),1,f);
    fwrite(&p->x,sizeof(float),1,f); fwrite(&p->y,sizeof(float),1,f); fwrite(&p->z,sizeof(float),1,f);
    fwrite(&p->prev_x,sizeof(float),1,f); fwrite(&p->prev_y,sizeof(float),1,f); fwrite(&p->prev_z,sizeof(float),1,f);
    fwrite(&p->mx,sizeof(float),1,f); fwrite(&p->my,sizeof(float),1,f); fwrite(&p->mz,sizeof(float),1,f);
    fwrite(&p->yaw,sizeof(float),1,f); fwrite(&p->pitch,sizeof(float),1,f);
    fwrite(&p->prev_yaw,sizeof(float),1,f); fwrite(&p->prev_pitch,sizeof(float),1,f);
    fwrite(&p->age,sizeof(int),1,f); fwrite(&p->fire_ticks,sizeof(int),1,f); fwrite(&p->in_ground,sizeof(int),1,f);
    fwrite(&p->tile_x,sizeof(int),1,f); fwrite(&p->tile_y,sizeof(int),1,f); fwrite(&p->tile_z,sizeof(int),1,f);
    fwrite(&p->in_tile,sizeof(int),1,f); fwrite(&p->in_data,sizeof(int),1,f);
    fwrite(&p->ticks_in_ground,sizeof(int),1,f); fwrite(&p->ticks_in_air,sizeof(int),1,f);
    fwrite(&p->arrow_shake,sizeof(int),1,f); fwrite(&p->player_arrow,sizeof(int),1,f); fwrite(&p->arrow_knockback,sizeof(int),1,f);
}

static int pex_read_projectile_125(FILE *f, FlatProjectile *p) {
    memset(p,0,sizeof(*p));
#define PEX_RD_PROJ(field,type) do{if(fread(&p->field,sizeof(type),1,f)!=1)return 0;}while(0)
    PEX_RD_PROJ(type,int); PEX_RD_PROJ(item_damage,int);
    PEX_RD_PROJ(owner_type,int); PEX_RD_PROJ(owner_mob_type,int); PEX_RD_PROJ(owner_mob_index,int);
    PEX_RD_PROJ(damage,int); PEX_RD_PROJ(critical,int);
    PEX_RD_PROJ(x,float); PEX_RD_PROJ(y,float); PEX_RD_PROJ(z,float);
    PEX_RD_PROJ(prev_x,float); PEX_RD_PROJ(prev_y,float); PEX_RD_PROJ(prev_z,float);
    PEX_RD_PROJ(mx,float); PEX_RD_PROJ(my,float); PEX_RD_PROJ(mz,float);
    PEX_RD_PROJ(yaw,float); PEX_RD_PROJ(pitch,float); PEX_RD_PROJ(prev_yaw,float); PEX_RD_PROJ(prev_pitch,float);
    PEX_RD_PROJ(age,int); PEX_RD_PROJ(fire_ticks,int); PEX_RD_PROJ(in_ground,int);
    PEX_RD_PROJ(tile_x,int); PEX_RD_PROJ(tile_y,int); PEX_RD_PROJ(tile_z,int);
    PEX_RD_PROJ(in_tile,int); PEX_RD_PROJ(in_data,int); PEX_RD_PROJ(ticks_in_ground,int); PEX_RD_PROJ(ticks_in_air,int);
    PEX_RD_PROJ(arrow_shake,int); PEX_RD_PROJ(player_arrow,int); PEX_RD_PROJ(arrow_knockback,int);
#undef PEX_RD_PROJ
    if(p->type<FLAT_PROJECTILE_POTION||p->type>FLAT_PROJECTILE_SNOWBALL)return 1;
    p->active=1;
    if(!isfinite(p->prev_x)||!isfinite(p->prev_y)||!isfinite(p->prev_z)){p->prev_x=p->x;p->prev_y=p->y;p->prev_z=p->z;}
    if(!isfinite(p->prev_yaw)||!isfinite(p->prev_pitch)){p->prev_yaw=p->yaw;p->prev_pitch=p->pitch;}
    return 1;
}

static void level_path_for_dir(const char *dir, char *out, size_t cap) {
    snprintf(out, cap, "%s\\level.dat", dir);
}

static void legacy_type_path_for_dir(const char *dir, char *out, size_t cap) {
    snprintf(out, cap, "%s\\pexcraft_world_type.txt", dir);
}

static void legacy_seed_path_for_dir(const char *dir, char *out, size_t cap) {
    snprintf(out, cap, "%s\\pexcraft_seed.txt", dir);
}

static void legacy_state_path_for_dir(const char *dir, char *out, size_t cap) {
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

static int read_level_summary(const char *dir, long long *out_seed, int *out_world_type, long long *out_last_played) {
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
        h == FLAT_WORLD_HEIGHT && y_min == FLAT_WORLD_Y_MIN) {
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
    (void)w;
    if (ok && out_last_played) {
#ifdef _WIN32
        WIN32_FIND_DATAA fd;
        HANDLE hfind = FindFirstFileA(path, &fd);
        if (hfind != INVALID_HANDLE_VALUE) {
            ULARGE_INTEGER t;
            t.LowPart = fd.ftLastWriteTime.dwLowDateTime;
            t.HighPart = fd.ftLastWriteTime.dwHighDateTime;
            *out_last_played = (long long)((t.QuadPart - 116444736000000000ULL) / 10000ULL);
            FindClose(hfind);
        }
#else
        char norm[MAX_PATHBUF];
        pex_normalize_path(norm, sizeof(norm), path);
        struct stat st;
        if (stat(norm, &st) == 0) *out_last_played = (long long)st.st_mtime * 1000LL;
#endif
    }
    return ok;
}

static int read_level_metadata(const char *dir, long long *out_seed, int *out_world_type) {
    return read_level_summary(dir, out_seed, out_world_type, NULL);
}

static int read_level_seed_for_dir(const char *dir, long long *out_seed) {
    if (read_level_metadata(dir, out_seed, NULL)) return 1;
    return read_level_long_tag_for_dir(dir, "RandomSeed", out_seed);
}

static int read_world_seed_for_dir(const char *dir, long long *out_seed) {
    if (read_level_seed_for_dir(dir, out_seed)) return 1;

    /* Legacy fallback only.  New saves keep this in level.dat. */
    char path[MAX_PATHBUF];
    legacy_seed_path_for_dir(dir, path, sizeof(path));
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
    if (read_level_metadata(dir, NULL, &type)) return type ? 1 : 0;
    if (read_level_int_tag_for_dir(dir, "WorldType", &type)) return type ? 1 : 0;

    char gen[64];
    if (read_level_string_tag_for_dir(dir, "generatorName", gen, sizeof(gen))) {
        return (strstr(gen, "default") || strstr(gen, "normal")) ? 1 : 0;
    }

    /* Legacy fallback only.  New saves keep this in level.dat. */
    char path[MAX_PATHBUF];
    legacy_type_path_for_dir(dir, path, sizeof(path));
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

static FILE *open_world_state_file(void) {
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

    legacy_state_path_for_dir(g_loaded_world_dir, path, sizeof(path));
    f = fopen(path, "rb");
    if (f) return f;
    return NULL;
}

static void cleanup_legacy_world_metadata(const char *dir) {
    char path[MAX_PATHBUF];
    legacy_state_path_for_dir(dir, path, sizeof(path));
    DeleteFileA(path);
    legacy_seed_path_for_dir(dir, path, sizeof(path));
    DeleteFileA(path);
    legacy_type_path_for_dir(dir, path, sizeof(path));
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

static int save_snapshot_add_chunk(PexSaveSnapshot *ss, int lcx, int lcz) {
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

    int world_x0 = cs->cx * FLAT_RENDER_CHUNK;
    int world_z0 = cs->cz * FLAT_RENDER_CHUNK;
    int fx0 = flat_storage_x_world(world_x0);
    for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
        int yi = pex_save_flat_y_index(y);
        for (int lz = 0; lz < FLAT_RENDER_CHUNK; lz++) {
            int bi = pex_save_chunk_buf_index(0, y, lz);
            int fz = flat_storage_z_world(world_z0 + lz);
            memcpy(&cs->blocks[bi], &g_flat_blocks[yi][fz][fx0], FLAT_RENDER_CHUNK);
            memcpy(&cs->meta[bi], &g_flat_meta[yi][fz][fx0], FLAT_RENDER_CHUNK);
        }
    }

    ss->chunk_count++;
    return 1;
}

static PexSaveSnapshot *pex_save_snapshot_create(int write_world_state) {
    player_capabilities_apply_game_mode();
    PexSaveSnapshot *ss = (PexSaveSnapshot*)calloc(1, sizeof(PexSaveSnapshot));
    if (!ss) return NULL;

    ss->write_world_state = write_world_state ? 1 : 0;
    ss->flat_world_origin_x = g_flat_world_origin_x;
    ss->flat_world_origin_z = g_flat_world_origin_z;
    ss->world_seed = g_world_seed;
    ss->world_time = g_world_time;
    ss->world_type = g_world_type;
    ss->game_mode = g_game_mode;
    ss->dimension = g_current_dimension;
    ss->player_capabilities = g_player_capabilities;
    ss->player_fire_ticks = g_player_fire_ticks;
    ss->player_x = g_player_x;
    ss->player_y = g_player_y;
    ss->player_z = g_player_z;
    ss->player_yaw = g_player_yaw;
    ss->player_pitch = g_player_pitch;
    ss->player_health = g_player_health;
    ss->player_food_level = g_player_food_level;
    ss->player_prev_food_level = g_player_prev_food_level;
    ss->player_food_saturation = g_player_food_saturation;
    ss->player_food_exhaustion = g_player_food_exhaustion;
    ss->player_food_timer = g_player_food_timer;
    ss->player_xp_level = g_player_xp_level;
    ss->player_xp_total = g_player_xp_total;
    ss->player_xp_progress = g_player_xp_progress;
    ss->player_armor = g_player_armor;
    ss->selected_hotbar_slot = g_selected_hotbar_slot;
    ss->player_fall_distance = g_player_fall_distance;
    ss->player_dead = g_player_dead;
    memcpy(ss->inventory, g_inventory, sizeof(g_inventory));
    memcpy(ss->armor_inventory, g_armor_inventory, sizeof(g_armor_inventory));
    memcpy(ss->chest_tiles, g_chest_tiles, sizeof(g_chest_tiles));
    memcpy(ss->furnace_tiles, g_furnace_tiles, sizeof(g_furnace_tiles));
    memcpy(ss->drops, g_drops, sizeof(g_drops));
    memcpy(ss->xp_orbs, g_xp_orbs, sizeof(g_xp_orbs));
    memcpy(ss->projectiles, g_projectiles, sizeof(g_projectiles));
    memcpy(ss->vehicles, g_vehicles, sizeof(g_vehicles));
    memcpy(ss->jukebox_tiles, g_jukebox_tiles, sizeof(g_jukebox_tiles));
    memcpy(ss->player_potion_effects, g_player_potion_effects, sizeof(g_player_potion_effects));
    memcpy(ss->map_data, g_map_data, sizeof(g_map_data));
    ss->next_map_id = g_next_map_id;
    memcpy(ss->passive_mobs, g_passive_mobs, sizeof(g_passive_mobs));
    snprintf(ss->loaded_world_dir, sizeof(ss->loaded_world_dir), "%s", g_loaded_world_dir);
    ss->ingame_ticks = g_ingame_ticks;

    for (int lcz = 0; lcz < FLAT_RENDER_CHUNKS; lcz++) {
        for (int lcx = 0; lcx < FLAT_RENDER_CHUNKS; lcx++) {
            if (!save_snapshot_add_chunk(ss, lcx, lcz)) {
                pex_save_snapshot_free(ss);
                return NULL;
            }
        }
    }

    return ss;
}

static void save_snapshot_clear_flags(const PexSaveSnapshot *ss) {
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

static void save_snapshot_restore_flags(const PexSaveSnapshot *ss) {
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

static void snapshot_chunk_delta_path(const PexSaveSnapshot *ss, int cx, int cz, char *out, size_t cap) {
    if (!ss || !ss->loaded_world_dir[0]) {
        if (cap) out[0] = 0;
        return;
    }
    char bx[32], bz[32];
    base36_i32(cx, bx, sizeof(bx));
    base36_i32(cz, bz, sizeof(bz));
    char dir[MAX_PATHBUF];
    if (ss->dimension == -1)
        snprintf(dir, sizeof(dir), "%s\\DIM-1\\chunks", ss->loaded_world_dir);
    else if (ss->dimension == 1)
        snprintf(dir, sizeof(dir), "%s\\DIM1\\chunks", ss->loaded_world_dir);
    else
        snprintf(dir, sizeof(dir), "%s\\chunks", ss->loaded_world_dir);
    make_dir_recursive(dir);
    snprintf(out, cap, "%s\\c.%s.%s.dat", dir, bx, bz);
}

static int chunk_delta_is_raw(const char magic[8]) {
    return memcmp(magic, "CHNKDLT1", 8) == 0 ||
           memcmp(magic, "CHNKDLT2", 8) == 0 ||
           memcmp(magic, "PXCDLT12", 8) == 0;
}

static int chunk_delta_read_payload(FILE *f, const char magic[8],
                                                       int rcx, int rcz, int count,
                                                       int cx, int cz,
                                                       unsigned char *blocks,
                                                       unsigned char *meta) {
    if (!f || !blocks || !meta) return 0;
    if (rcx != cx || rcz != cz || count != FLAT_CHUNK_BLOCK_COUNT) return 0;

    if (chunk_delta_is_raw(magic)) {
        int ok = fread(blocks, 1, FLAT_CHUNK_BLOCK_COUNT, f) == FLAT_CHUNK_BLOCK_COUNT;
        int has_meta = (memcmp(magic, "CHNKDLT2", 8) == 0);
        if (ok && has_meta) ok = fread(meta, 1, FLAT_CHUNK_BLOCK_COUNT, f) == FLAT_CHUNK_BLOCK_COUNT;
        if (ok && !has_meta) memset(meta, 0, FLAT_CHUNK_BLOCK_COUNT);
        return ok;
    }

#if PEX_SAVE_ZLIB_CHUNK_DELTAS
    if (memcmp(magic, "PXCDLTZ1", 8) == 0) {
        uint32_t blocks_len = 0;
        uint32_t meta_len = 0;
        if (fread(&blocks_len, sizeof(blocks_len), 1, f) != 1 ||
            fread(&meta_len, sizeof(meta_len), 1, f) != 1) return 0;
        if (blocks_len == 0 || meta_len == 0 ||
            blocks_len > 1024u * 1024u || meta_len > 1024u * 1024u) return 0;

        unsigned char *blocks_comp = (unsigned char*)malloc((size_t)blocks_len);
        unsigned char *meta_comp = (unsigned char*)malloc((size_t)meta_len);
        if (!blocks_comp || !meta_comp) {
            free(blocks_comp);
            free(meta_comp);
            return 0;
        }

        int ok = fread(blocks_comp, 1, (size_t)blocks_len, f) == (size_t)blocks_len &&
                 fread(meta_comp, 1, (size_t)meta_len, f) == (size_t)meta_len;
        if (ok) {
            uLongf out_blocks = (uLongf)FLAT_CHUNK_BLOCK_COUNT;
            uLongf out_meta = (uLongf)FLAT_CHUNK_BLOCK_COUNT;
            ok = uncompress(blocks, &out_blocks, blocks_comp, (uLong)blocks_len) == Z_OK &&
                 out_blocks == (uLongf)FLAT_CHUNK_BLOCK_COUNT &&
                 uncompress(meta, &out_meta, meta_comp, (uLong)meta_len) == Z_OK &&
                 out_meta == (uLongf)FLAT_CHUNK_BLOCK_COUNT;
        }

        free(blocks_comp);
        free(meta_comp);
        return ok;
    }
#endif

    return 0;
}

static int chunk_delta_write_payload(const char *path, int cx, int cz,
                                                 const unsigned char *blocks,
                                                 const unsigned char *meta) {
    if (!path || !path[0] || !blocks || !meta) return 0;

#if PEX_SAVE_ZLIB_CHUNK_DELTAS
    {
        uLongf blocks_len = compressBound((uLong)FLAT_CHUNK_BLOCK_COUNT);
        uLongf meta_len = compressBound((uLong)FLAT_CHUNK_BLOCK_COUNT);
        unsigned char *blocks_comp = (unsigned char*)malloc((size_t)blocks_len);
        unsigned char *meta_comp = (unsigned char*)malloc((size_t)meta_len);
        if (blocks_comp && meta_comp &&
            compress2(blocks_comp, &blocks_len, blocks, (uLong)FLAT_CHUNK_BLOCK_COUNT, Z_BEST_COMPRESSION) == Z_OK &&
            compress2(meta_comp, &meta_len, meta, (uLong)FLAT_CHUNK_BLOCK_COUNT, Z_BEST_COMPRESSION) == Z_OK &&
            blocks_len <= 0x7fffffffu && meta_len <= 0x7fffffffu) {
            FILE *f = fopen(path, "wb");
            if (f) {
                char magic[8] = {'P','X','C','D','L','T','Z','1'};
                int count = FLAT_CHUNK_BLOCK_COUNT;
                uint32_t blocks_len32 = (uint32_t)blocks_len;
                uint32_t meta_len32 = (uint32_t)meta_len;
                int ok = fwrite(magic, 1, 8, f) == 8 &&
                         fwrite(&cx, sizeof(cx), 1, f) == 1 &&
                         fwrite(&cz, sizeof(cz), 1, f) == 1 &&
                         fwrite(&count, sizeof(count), 1, f) == 1 &&
                         fwrite(&blocks_len32, sizeof(blocks_len32), 1, f) == 1 &&
                         fwrite(&meta_len32, sizeof(meta_len32), 1, f) == 1 &&
                         fwrite(blocks_comp, 1, (size_t)blocks_len, f) == (size_t)blocks_len &&
                         fwrite(meta_comp, 1, (size_t)meta_len, f) == (size_t)meta_len;
                fclose(f);
                free(blocks_comp);
                free(meta_comp);
                return ok ? 1 : 0;
            }
        }
        free(blocks_comp);
        free(meta_comp);
    }
#endif

    {
        FILE *f = fopen(path, "wb");
        if (!f) return 0;
        char magic[8] = {'C','H','N','K','D','L','T','2'};
        int count = FLAT_CHUNK_BLOCK_COUNT;
        int ok = fwrite(magic, 1, 8, f) == 8 &&
                 fwrite(&cx, sizeof(cx), 1, f) == 1 &&
                 fwrite(&cz, sizeof(cz), 1, f) == 1 &&
                 fwrite(&count, sizeof(count), 1, f) == 1 &&
                 fwrite(blocks, 1, FLAT_CHUNK_BLOCK_COUNT, f) == FLAT_CHUNK_BLOCK_COUNT &&
                 fwrite(meta, 1, FLAT_CHUNK_BLOCK_COUNT, f) == FLAT_CHUNK_BLOCK_COUNT;
        fclose(f);
        return ok ? 1 : 0;
    }
}

static void save_snapshot_one_chunk(PexSaveSnapshot *ss, PexSaveChunkSnapshot *cs) {
    if (!ss || !cs || !ss->loaded_world_dir[0] || !cs->blocks || !cs->meta) return;

    int cx = cs->cx;
    int cz = cs->cz;
    unsigned char *base = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
    if (!base) return;

    flat_generate_chunk_base(cx, cz, base, ss->world_type, ss->world_seed, NULL);

    char path[MAX_PATHBUF];
    snapshot_chunk_delta_path(ss, cx, cz, path, sizeof(path));
    int meta_nonzero = 0;
    for (int i = 0; i < FLAT_CHUNK_BLOCK_COUNT; i++) { if (cs->meta[i]) { meta_nonzero = 1; break; } }
    if (memcmp(base, cs->blocks, FLAT_CHUNK_BLOCK_COUNT) == 0 && !meta_nonzero) {
        DeleteFileA(path);
    } else {
        chunk_delta_write_payload(path, cx, cz, cs->blocks, cs->meta);
    }

    free(base);
}

static void save_snapshot_flat_chunks(PexSaveSnapshot *ss) {
    if (!ss || !ss->loaded_world_dir[0]) return;
    for (int i = 0; i < ss->chunk_count; i++) {
        save_snapshot_one_chunk(ss, &ss->chunks[i]);
    }
}

static void save_snapshot_world_state(PexSaveSnapshot *ss) {
    if (!ss || !ss->loaded_world_dir[0]) return;

    migrate_legacy_chunk_folder(ss->loaded_world_dir);
    save_snapshot_flat_chunks(ss);

    if (!ss->write_world_state) return;

    char path[MAX_PATHBUF];
    level_path_for_dir(ss->loaded_world_dir, path, sizeof(path));
    FILE *f = fopen(path, "wb");
    if (!f) { pex_logf("save world-state failed open path=%s", path); return; }

    char magic[8] = {'L','E','V','E','L','S','T','1'};
    int version = 37;
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
    fwrite(&ss->player_food_level, sizeof(ss->player_food_level), 1, f);
    fwrite(&ss->player_prev_food_level, sizeof(ss->player_prev_food_level), 1, f);
    fwrite(&ss->player_food_saturation, sizeof(ss->player_food_saturation), 1, f);
    fwrite(&ss->player_food_exhaustion, sizeof(ss->player_food_exhaustion), 1, f);
    fwrite(&ss->player_food_timer, sizeof(ss->player_food_timer), 1, f);
    fwrite(&ss->player_xp_level, sizeof(ss->player_xp_level), 1, f);
    fwrite(&ss->player_xp_total, sizeof(ss->player_xp_total), 1, f);
    fwrite(&ss->player_xp_progress, sizeof(ss->player_xp_progress), 1, f);
    fwrite(&ss->world_time, sizeof(ss->world_time), 1, f);
    fwrite(&ss->game_mode, sizeof(ss->game_mode), 1, f);
    fwrite(&ss->dimension, sizeof(ss->dimension), 1, f);
    fwrite(&ss->player_capabilities, sizeof(ss->player_capabilities), 1, f);
    fwrite(&ss->player_fire_ticks, sizeof(ss->player_fire_ticks), 1, f);

    for (int i = 0; i < 36; i++) {
        pex_write_item_stack_125(f, &ss->inventory[i]);
    }
    for (int i = 0; i < 4; i++) {
        pex_write_item_stack_125(f, &ss->armor_inventory[i]);
    }

    ItemStack empty_legacy_chest[27];
    memset(empty_legacy_chest, 0, sizeof(empty_legacy_chest));
    for (int i = 0; i < 27; i++) {
        pex_write_item_stack_125(f, &empty_legacy_chest[i]);
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
            pex_write_item_stack_125(f, &ct->slots[i]);
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
            pex_write_item_stack_125(f, &ft->slots[i]);
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
        pex_write_item_stack_125(f, &e->stack);
        fwrite(&e->x, sizeof(float), 1, f);
        fwrite(&e->y, sizeof(float), 1, f);
        fwrite(&e->z, sizeof(float), 1, f);
        fwrite(&e->mx, sizeof(float), 1, f);
        fwrite(&e->my, sizeof(float), 1, f);
        fwrite(&e->mz, sizeof(float), 1, f);
        fwrite(&e->rot, sizeof(float), 1, f);
        fwrite(&e->age, sizeof(int), 1, f);
        fwrite(&e->pickup_delay, sizeof(int), 1, f);
        fwrite(&e->health, sizeof(int), 1, f);
    }

    int xp_orb_count = 0;
    for (int i = 0; i < MAX_XP_ORBS; ++i) if (ss->xp_orbs[i].active && ss->xp_orbs[i].value > 0) ++xp_orb_count;
    fwrite(&xp_orb_count, sizeof(xp_orb_count), 1, f);
    for (int i = 0; i < MAX_XP_ORBS; ++i) {
        FlatXPOrb *o = &ss->xp_orbs[i];
        if (!o->active || o->value <= 0) continue;
        fwrite(&o->value, sizeof(int), 1, f);
        fwrite(&o->x, sizeof(float), 1, f); fwrite(&o->y, sizeof(float), 1, f); fwrite(&o->z, sizeof(float), 1, f);
        fwrite(&o->mx, sizeof(float), 1, f); fwrite(&o->my, sizeof(float), 1, f); fwrite(&o->mz, sizeof(float), 1, f);
        fwrite(&o->rot, sizeof(float), 1, f);
        fwrite(&o->age, sizeof(int), 1, f); fwrite(&o->color, sizeof(int), 1, f);
        fwrite(&o->pickup_delay, sizeof(int), 1, f); fwrite(&o->health, sizeof(int), 1, f);
    }

    int projectile_count = 0;
    for (int i=0;i<MAX_PROJECTILE_ENTITIES;++i) if(ss->projectiles[i].active) ++projectile_count;
    fwrite(&projectile_count,sizeof(projectile_count),1,f);
    for (int i=0;i<MAX_PROJECTILE_ENTITIES;++i) if(ss->projectiles[i].active) pex_write_projectile_125(f,&ss->projectiles[i]);

    int vehicle_count = 0;
    for (int i = 0; i < MAX_VEHICLE_ENTITIES; ++i) if (ss->vehicles[i].active) vehicle_count++;
    fwrite(&vehicle_count, sizeof(vehicle_count), 1, f);
    for (int i = 0; i < MAX_VEHICLE_ENTITIES; ++i) {
        FlatVehicle *v = &ss->vehicles[i];
        if (!v->active) continue;
        fwrite(&v->type, sizeof(int), 1, f);
        fwrite(&v->x, sizeof(float), 1, f); fwrite(&v->y, sizeof(float), 1, f); fwrite(&v->z, sizeof(float), 1, f);
        fwrite(&v->mx, sizeof(float), 1, f); fwrite(&v->my, sizeof(float), 1, f); fwrite(&v->mz, sizeof(float), 1, f);
        fwrite(&v->yaw, sizeof(float), 1, f);
        fwrite(&v->age, sizeof(int), 1, f);
    }

    int jukebox_count = 0;
    for (int i = 0; i < MAX_JUKEBOX_TILES; ++i) if (ss->jukebox_tiles[i].active && ss->jukebox_tiles[i].record_item) jukebox_count++;
    fwrite(&jukebox_count, sizeof(jukebox_count), 1, f);
    for (int i = 0; i < MAX_JUKEBOX_TILES; ++i) {
        JukeboxTile *jt = &ss->jukebox_tiles[i];
        if (!jt->active || !jt->record_item) continue;
        fwrite(&jt->x, sizeof(int), 1, f); fwrite(&jt->y, sizeof(int), 1, f); fwrite(&jt->z, sizeof(int), 1, f);
        fwrite(&jt->record_item, sizeof(int), 1, f);
    }

    passive_mobs_write_to_file(f, ss->passive_mobs);

    int potion_count = 0;
    for (int i = 1; i < PEX_POTION_MAX; ++i) if (ss->player_potion_effects[i].duration > 0) potion_count++;
    fwrite(&potion_count, sizeof(potion_count), 1, f);
    for (int i = 1; i < PEX_POTION_MAX; ++i) {
        const PexPotionEffectState *pe = &ss->player_potion_effects[i];
        if (pe->duration <= 0) continue;
        fwrite(&i, sizeof(int), 1, f);
        fwrite(&pe->duration, sizeof(int), 1, f);
        fwrite(&pe->amplifier, sizeof(int), 1, f);
    }

    fwrite(&ss->next_map_id, sizeof(ss->next_map_id), 1, f);
    int map_count = 0;
    for (int i = 0; i < MAX_MAP_DATA; ++i) if (ss->map_data[i].active) map_count++;
    fwrite(&map_count, sizeof(map_count), 1, f);
    for (int i = 0; i < MAX_MAP_DATA; ++i) {
        const PexMapData *m = &ss->map_data[i];
        if (!m->active) continue;
        fwrite(&m->id, sizeof(int), 1, f);
        fwrite(&m->x_center, sizeof(int), 1, f);
        fwrite(&m->z_center, sizeof(int), 1, f);
        fwrite(&m->dimension, sizeof(int), 1, f);
        fwrite(&m->scale, sizeof(int), 1, f);
        fwrite(&m->tick, sizeof(int), 1, f);
        fwrite(m->colors, 1, sizeof(m->colors), f);
    }

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
                save_snapshot_world_state(ss);
                pex_save_snapshot_free(ss);
                continue;
            }
            if (should_exit) return 0;
            break;
        }
    }
}

static int save_worker_ensure_started(void) {
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
#if defined(PEX_PLATFORM_WASM)
    /* Browser target has no pthreads. Preserve autosave semantics by writing the
       immutable snapshot synchronously; IDBFS is flushed by the platform loop. */
    save_snapshot_world_state(ss);
    pex_save_snapshot_free(ss);
    return 1;
#else
    if (!save_worker_ensure_started()) return 0;

    EnterCriticalSection(&g_save_cs);
    ss->next = NULL;
    if (g_save_queue_tail) g_save_queue_tail->next = ss;
    else g_save_queue_head = ss;
    g_save_queue_tail = ss;
    g_save_thread_done = 0;
    LeaveCriticalSection(&g_save_cs);

    SetEvent(g_save_event);
    return 1;
#endif
}

static void request_chunk_save_async(void) {
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

    save_snapshot_clear_flags(ss);
    if (!pex_save_enqueue_snapshot(ss)) {
        save_snapshot_restore_flags(ss);
        pex_save_snapshot_free(ss);
    }
}

static void save_world_state_sync(void) {
    player_capabilities_apply_game_mode();
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
    int version = 37;
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
    fwrite(&g_player_food_level, sizeof(g_player_food_level), 1, f);
    fwrite(&g_player_prev_food_level, sizeof(g_player_prev_food_level), 1, f);
    fwrite(&g_player_food_saturation, sizeof(g_player_food_saturation), 1, f);
    fwrite(&g_player_food_exhaustion, sizeof(g_player_food_exhaustion), 1, f);
    fwrite(&g_player_food_timer, sizeof(g_player_food_timer), 1, f);
    fwrite(&g_player_xp_level, sizeof(g_player_xp_level), 1, f);
    fwrite(&g_player_xp_total, sizeof(g_player_xp_total), 1, f);
    fwrite(&g_player_xp_progress, sizeof(g_player_xp_progress), 1, f);
    fwrite(&g_world_time, sizeof(g_world_time), 1, f);
    fwrite(&g_game_mode, sizeof(g_game_mode), 1, f);
    fwrite(&g_current_dimension, sizeof(g_current_dimension), 1, f);
    fwrite(&g_player_capabilities, sizeof(g_player_capabilities), 1, f);
    fwrite(&g_player_fire_ticks, sizeof(g_player_fire_ticks), 1, f);

    for (int i = 0; i < 36; i++) {
        pex_write_item_stack_125(f, &g_inventory[i]);
    }
    for (int i = 0; i < 4; i++) {
        pex_write_item_stack_125(f, &g_armor_inventory[i]);
    }
    /* Legacy shared chest payload retained as zeroed padding for v9-v13 save
       migration. v14+ uses coordinate-bound ChestTile records below. */
    ItemStack empty_legacy_chest[27];
    memset(empty_legacy_chest, 0, sizeof(empty_legacy_chest));
    for (int i = 0; i < 27; i++) {
        pex_write_item_stack_125(f, &empty_legacy_chest[i]);
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
            pex_write_item_stack_125(f, &ct->slots[i]);
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
            pex_write_item_stack_125(f, &ft->slots[i]);
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
        pex_write_item_stack_125(f, &e->stack);
        fwrite(&e->x, sizeof(float), 1, f);
        fwrite(&e->y, sizeof(float), 1, f);
        fwrite(&e->z, sizeof(float), 1, f);
        fwrite(&e->mx, sizeof(float), 1, f);
        fwrite(&e->my, sizeof(float), 1, f);
        fwrite(&e->mz, sizeof(float), 1, f);
        fwrite(&e->rot, sizeof(float), 1, f);
        fwrite(&e->age, sizeof(int), 1, f);
        fwrite(&e->pickup_delay, sizeof(int), 1, f);
        fwrite(&e->health, sizeof(int), 1, f);
    }

    int xp_orb_count = 0;
    for (int i = 0; i < MAX_XP_ORBS; ++i) if (g_xp_orbs[i].active && g_xp_orbs[i].value > 0) ++xp_orb_count;
    fwrite(&xp_orb_count, sizeof(xp_orb_count), 1, f);
    for (int i = 0; i < MAX_XP_ORBS; ++i) {
        FlatXPOrb *o = &g_xp_orbs[i];
        if (!o->active || o->value <= 0) continue;
        fwrite(&o->value, sizeof(int), 1, f);
        fwrite(&o->x, sizeof(float), 1, f); fwrite(&o->y, sizeof(float), 1, f); fwrite(&o->z, sizeof(float), 1, f);
        fwrite(&o->mx, sizeof(float), 1, f); fwrite(&o->my, sizeof(float), 1, f); fwrite(&o->mz, sizeof(float), 1, f);
        fwrite(&o->rot, sizeof(float), 1, f);
        fwrite(&o->age, sizeof(int), 1, f); fwrite(&o->color, sizeof(int), 1, f);
        fwrite(&o->pickup_delay, sizeof(int), 1, f); fwrite(&o->health, sizeof(int), 1, f);
    }

    int projectile_count = 0;
    for (int i=0;i<MAX_PROJECTILE_ENTITIES;++i) if(g_projectiles[i].active) ++projectile_count;
    fwrite(&projectile_count,sizeof(projectile_count),1,f);
    for (int i=0;i<MAX_PROJECTILE_ENTITIES;++i) if(g_projectiles[i].active) pex_write_projectile_125(f,&g_projectiles[i]);

    int vehicle_count = 0;
    for (int i = 0; i < MAX_VEHICLE_ENTITIES; ++i) if (g_vehicles[i].active) vehicle_count++;
    fwrite(&vehicle_count, sizeof(vehicle_count), 1, f);
    for (int i = 0; i < MAX_VEHICLE_ENTITIES; ++i) {
        FlatVehicle *v = &g_vehicles[i];
        if (!v->active) continue;
        fwrite(&v->type, sizeof(int), 1, f);
        fwrite(&v->x, sizeof(float), 1, f); fwrite(&v->y, sizeof(float), 1, f); fwrite(&v->z, sizeof(float), 1, f);
        fwrite(&v->mx, sizeof(float), 1, f); fwrite(&v->my, sizeof(float), 1, f); fwrite(&v->mz, sizeof(float), 1, f);
        fwrite(&v->yaw, sizeof(float), 1, f);
        fwrite(&v->age, sizeof(int), 1, f);
    }

    int jukebox_count = 0;
    for (int i = 0; i < MAX_JUKEBOX_TILES; ++i) if (g_jukebox_tiles[i].active && g_jukebox_tiles[i].record_item) jukebox_count++;
    fwrite(&jukebox_count, sizeof(jukebox_count), 1, f);
    for (int i = 0; i < MAX_JUKEBOX_TILES; ++i) {
        JukeboxTile *jt = &g_jukebox_tiles[i];
        if (!jt->active || !jt->record_item) continue;
        fwrite(&jt->x, sizeof(int), 1, f); fwrite(&jt->y, sizeof(int), 1, f); fwrite(&jt->z, sizeof(int), 1, f);
        fwrite(&jt->record_item, sizeof(int), 1, f);
    }

    passive_mobs_write_to_file(f, g_passive_mobs);

    fclose(f);
    write_session_lock(g_loaded_world_dir);
    cleanup_legacy_world_metadata(g_loaded_world_dir);
    g_save_dirty = 0;
    g_last_autosave_tick = g_ingame_ticks;
    g_save_message_ticks = SAVE_MESSAGE_TICKS;
    pex_logf("save sync complete path=%s health=%d dead=%d origin=%d,%d", path, g_player_health, g_player_dead, g_flat_world_origin_x, g_flat_world_origin_z);
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
    if (!ss) { pex_logf("save async snapshot create failed dir=%s", g_loaded_world_dir); return; }

    save_snapshot_clear_flags(ss);
    g_save_dirty = 0;
    g_last_autosave_tick = g_ingame_ticks;
    g_save_message_ticks = SAVE_MESSAGE_TICKS;

    if (!pex_save_enqueue_snapshot(ss)) {
        pex_logf("save async enqueue failed dir=%s", g_loaded_world_dir);
        save_snapshot_restore_flags(ss);
        g_save_dirty = 1;
        pex_save_snapshot_free(ss);
    } else {
        pex_logf("save async enqueued dir=%s health=%d dead=%d origin=%d,%d", g_loaded_world_dir, g_player_health, g_player_dead, g_flat_world_origin_x, g_flat_world_origin_z);
    }
}


static void reset_player_damage_visuals(void) {
    g_player_health = player_health_clamp(g_player_health);
    g_player_prev_health = g_player_health;
    g_hearts_life = 0;
    g_player_hurt_time = 0;
    g_player_attacked_at_yaw = 0.0f;
}

static int load_current_world_state(void) {
    g_last_load_state_had_terrain = 0;
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    return 0;
#endif
    if (!g_loaded_world_dir[0]) return 0;

    FILE *f = open_world_state_file();
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
        (memcmp(magic, "LEVELST1", 8) == 0 && version >= 13 && version <= 37) ||
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

    if (version >= 18) {
        if (fread(&g_player_food_level, sizeof(g_player_food_level), 1, f) != 1 ||
            fread(&g_player_prev_food_level, sizeof(g_player_prev_food_level), 1, f) != 1 ||
            fread(&g_player_food_saturation, sizeof(g_player_food_saturation), 1, f) != 1 ||
            fread(&g_player_food_exhaustion, sizeof(g_player_food_exhaustion), 1, f) != 1 ||
            fread(&g_player_food_timer, sizeof(g_player_food_timer), 1, f) != 1 ||
            fread(&g_player_xp_level, sizeof(g_player_xp_level), 1, f) != 1 ||
            fread(&g_player_xp_total, sizeof(g_player_xp_total), 1, f) != 1 ||
            fread(&g_player_xp_progress, sizeof(g_player_xp_progress), 1, f) != 1) {
            fclose(f);
            return 0;
        }
        if (version >= 19) {
            if (fread(&g_world_time, sizeof(g_world_time), 1, f) != 1) {
                fclose(f);
                return 0;
            }
        } else {
            g_world_time = 0;
        }
        if (version >= 20) {
            if (fread(&g_game_mode, sizeof(g_game_mode), 1, f) != 1) {
                fclose(f);
                return 0;
            }
            g_game_mode = g_game_mode ? 1 : 0;
        }
        if (version >= 28) {
            if (fread(&g_current_dimension, sizeof(g_current_dimension), 1, f) != 1) {
                fclose(f);
                return 0;
            }
        } else {
            g_current_dimension = 0;
        }
        if(version>=37){
            if(fread(&g_player_capabilities,sizeof(g_player_capabilities),1,f)!=1 ||
               fread(&g_player_fire_ticks,sizeof(g_player_fire_ticks),1,f)!=1){fclose(f);return 0;}
            g_player_capabilities.disable_damage=g_player_capabilities.disable_damage?1:0;
            g_player_capabilities.allow_flying=g_player_capabilities.allow_flying?1:0;
            g_player_capabilities.is_flying=g_player_capabilities.is_flying?1:0;
            g_player_capabilities.is_creative_mode=g_player_capabilities.is_creative_mode?1:0;
            if(g_player_fire_ticks<0)g_player_fire_ticks=0;
        }else{
            memset(&g_player_capabilities,0,sizeof(g_player_capabilities));
            player_capabilities_apply_game_mode();
            g_player_fire_ticks=0;
        }
    } else {
        player_food_reset();
        player_xp_reset();
        g_world_time = 0;
        memset(&g_player_capabilities,0,sizeof(g_player_capabilities));
        player_capabilities_apply_game_mode();
        g_player_fire_ticks=0;
    }
    player_food_sanitize();
    player_xp_sanitize();

    for (int i = 0; i < 36; i++) {
        if (!pex_read_item_stack_125(f, &g_inventory[i], version)) {
            fclose(f);
            return 0;
        }
        g_inventory[i].pop_time = 0;
        if (g_inventory[i].count <= 0) memset(&g_inventory[i], 0, sizeof(g_inventory[i]));
    }
    memset(g_armor_inventory, 0, sizeof(g_armor_inventory));
    if (version >= 17) {
        for (int i = 0; i < 4; i++) {
            if (!pex_read_item_stack_125(f, &g_armor_inventory[i], version)) {
                fclose(f);
                return 0;
            }
            g_armor_inventory[i].pop_time = 0;
            if (g_armor_inventory[i].count <= 0 || armor_stack_type(g_armor_inventory[i].id) < 0) memset(&g_armor_inventory[i], 0, sizeof(g_armor_inventory[i]));
        }
    }
    armor_sync_player_armor();
    g_player_damage_remainder = 0;

    memset(g_chest_slots, 0, sizeof(g_chest_slots));
    if (version >= 9) {
        for (int i = 0; i < 27; i++) {
            if (!pex_read_item_stack_125(f, &g_chest_slots[i], version)) {
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
                if (!pex_read_item_stack_125(f, &ct->slots[i], version)) {
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
                if (!pex_read_item_stack_125(f, &ft->slots[i], version)) {
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
            flat_generate_origin_blocks();
            g_last_load_state_had_terrain = 1;
        }
    } else {
        if (fread(g_flat_blocks, sizeof(g_flat_blocks), 1, f) != 1) {
            fclose(f);
            return 0;
        }
        /* Old saves contain a full active-window block dump.  Mark every chunk so
           the next save converts the bloated file into delta chunks. */
        flat_mark_all_chunks_modified();
        int base_cx = pex_save_floor_div16(g_flat_world_origin_x);
        int base_cz = pex_save_floor_div16(g_flat_world_origin_z);
        for (int lcz = 0; lcz < FLAT_RENDER_CHUNKS; lcz++)
            for (int lcx = 0; lcx < FLAT_RENDER_CHUNKS; lcx++) {
                g_flat_world_chunk_generated[lcz][lcx] = 1;
                g_flat_chunk_world_cx[lcz][lcx] = base_cx + lcx;
                g_flat_chunk_world_cz[lcz][lcx] = base_cz + lcz;
            }
        flat_mark_all_chunks_dirty();
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
                if (!pex_read_item_stack_125(f, &e->stack, version) ||
                    fread(&e->x, sizeof(float), 1, f) != 1 ||
                    fread(&e->y, sizeof(float), 1, f) != 1 ||
                    fread(&e->z, sizeof(float), 1, f) != 1 ||
                    fread(&e->mx, sizeof(float), 1, f) != 1 ||
                    fread(&e->my, sizeof(float), 1, f) != 1 ||
                    fread(&e->mz, sizeof(float), 1, f) != 1 ||
                    fread(&e->rot, sizeof(float), 1, f) != 1 ||
                    fread(&e->age, sizeof(int), 1, f) != 1 ||
                    fread(&e->pickup_delay, sizeof(int), 1, f) != 1 ||
                    (version >= 30 && fread(&e->health, sizeof(int), 1, f) != 1)) {
                    memset(e, 0, sizeof(*e));
                    break;
                }
                if (e->stack.id > 0 && e->stack.count > 0) {
                    e->active = 1;
                    e->prev_x = e->x;
                    e->prev_y = e->y;
                    e->prev_z = e->z;
                    e->health = 5;
                }
            }
        }
    }
    memset(g_xp_orbs, 0, sizeof(g_xp_orbs));
    if (version >= 30) {
        int orb_count = 0;
        if (fread(&orb_count, sizeof(orb_count), 1, f) != 1) { fclose(f); return 0; }
        if (orb_count < 0) orb_count = 0;
        if (orb_count > MAX_XP_ORBS) orb_count = MAX_XP_ORBS;
        for (int i = 0; i < orb_count; ++i) {
            FlatXPOrb *o = &g_xp_orbs[i];
            memset(o, 0, sizeof(*o));
            if (fread(&o->value, sizeof(int), 1, f) != 1 ||
                fread(&o->x, sizeof(float), 1, f) != 1 || fread(&o->y, sizeof(float), 1, f) != 1 || fread(&o->z, sizeof(float), 1, f) != 1 ||
                fread(&o->mx, sizeof(float), 1, f) != 1 || fread(&o->my, sizeof(float), 1, f) != 1 || fread(&o->mz, sizeof(float), 1, f) != 1 ||
                fread(&o->rot, sizeof(float), 1, f) != 1 || fread(&o->age, sizeof(int), 1, f) != 1 || fread(&o->color, sizeof(int), 1, f) != 1 ||
                fread(&o->pickup_delay, sizeof(int), 1, f) != 1 || fread(&o->health, sizeof(int), 1, f) != 1) {
                fclose(f); return 0;
            }
            if (o->value > 0 && o->age < 6000) {
                o->active = 1; o->prev_x = o->x; o->prev_y = o->y; o->prev_z = o->z;
                if (o->health <= 0) o->health = 5;
            }
        }
    }
    memset(g_projectiles,0,sizeof(g_projectiles));
    if(version>=37){
        int projectile_count=0;
        if(fread(&projectile_count,sizeof(projectile_count),1,f)!=1){fclose(f);return 0;}
        if(projectile_count<0||projectile_count>4096){fclose(f);return 0;}
        for(int i=0;i<projectile_count;++i){
            FlatProjectile temp;
            if(!pex_read_projectile_125(f,&temp)){fclose(f);return 0;}
            if(i<MAX_PROJECTILE_ENTITIES && temp.active) g_projectiles[i]=temp;
        }
    }
    memset(g_vehicles, 0, sizeof(g_vehicles));
    memset(g_jukebox_tiles, 0, sizeof(g_jukebox_tiles));
    if (version >= 21) {
        int vehicle_count = 0;
        if (fread(&vehicle_count, sizeof(vehicle_count), 1, f) == 1) {
            if (vehicle_count < 0) vehicle_count = 0;
            if (vehicle_count > MAX_VEHICLE_ENTITIES) vehicle_count = MAX_VEHICLE_ENTITIES;
            for (int i = 0; i < vehicle_count; ++i) {
                FlatVehicle *v = &g_vehicles[i];
                memset(v, 0, sizeof(*v));
                if (fread(&v->type, sizeof(int), 1, f) != 1 ||
                    fread(&v->x, sizeof(float), 1, f) != 1 || fread(&v->y, sizeof(float), 1, f) != 1 || fread(&v->z, sizeof(float), 1, f) != 1 ||
                    fread(&v->mx, sizeof(float), 1, f) != 1 || fread(&v->my, sizeof(float), 1, f) != 1 || fread(&v->mz, sizeof(float), 1, f) != 1 ||
                    fread(&v->yaw, sizeof(float), 1, f) != 1 ||
                    fread(&v->age, sizeof(int), 1, f) != 1) {
                    memset(v, 0, sizeof(*v));
                    break;
                }
                if (v->type > 0) {
                    v->active = 1;
                    v->prev_x = v->x; v->prev_y = v->y; v->prev_z = v->z; v->prev_yaw = v->yaw;
                }
            }
        }
        int jukebox_count = 0;
        if (fread(&jukebox_count, sizeof(jukebox_count), 1, f) == 1) {
            if (jukebox_count < 0) jukebox_count = 0;
            if (jukebox_count > MAX_JUKEBOX_TILES) jukebox_count = MAX_JUKEBOX_TILES;
            for (int i = 0; i < jukebox_count; ++i) {
                JukeboxTile *jt = &g_jukebox_tiles[i];
                memset(jt, 0, sizeof(*jt));
                if (fread(&jt->x, sizeof(int), 1, f) != 1 || fread(&jt->y, sizeof(int), 1, f) != 1 ||
                    fread(&jt->z, sizeof(int), 1, f) != 1 || fread(&jt->record_item, sizeof(int), 1, f) != 1) {
                    memset(jt, 0, sizeof(*jt));
                    break;
                }
                if (jt->record_item > 0) jt->active = 1;
            }
        }
    }

    passive_mobs_read_from_file(f, version);
    memset(g_player_potion_effects, 0, sizeof(g_player_potion_effects));
    memset(g_map_data, 0, sizeof(g_map_data));
    g_next_map_id = 0;
    if (version >= 22) {
        int potion_count = 0;
        if (fread(&potion_count, sizeof(potion_count), 1, f) == 1) {
            if (potion_count < 0) potion_count = 0;
            if (potion_count > PEX_POTION_MAX) potion_count = PEX_POTION_MAX;
            for (int i = 0; i < potion_count; ++i) {
                int id = 0, dur = 0, amp = 0;
                if (fread(&id, sizeof(int), 1, f) != 1 || fread(&dur, sizeof(int), 1, f) != 1 || fread(&amp, sizeof(int), 1, f) != 1) break;
                if (id > 0 && id < PEX_POTION_MAX && dur > 0) {
                    g_player_potion_effects[id].duration = dur;
                    g_player_potion_effects[id].amplifier = amp < 0 ? 0 : amp;
                }
            }
        }
        if (fread(&g_next_map_id, sizeof(g_next_map_id), 1, f) != 1) g_next_map_id = 0;
        int map_count = 0;
        if (fread(&map_count, sizeof(map_count), 1, f) == 1) {
            if (map_count < 0) map_count = 0;
            if (map_count > MAX_MAP_DATA) map_count = MAX_MAP_DATA;
            for (int i = 0; i < map_count; ++i) {
                PexMapData *m = &g_map_data[i];
                memset(m, 0, sizeof(*m));
                if (fread(&m->id, sizeof(int), 1, f) != 1 || fread(&m->x_center, sizeof(int), 1, f) != 1 ||
                    fread(&m->z_center, sizeof(int), 1, f) != 1 || fread(&m->dimension, sizeof(int), 1, f) != 1 ||
                    fread(&m->scale, sizeof(int), 1, f) != 1 || fread(&m->tick, sizeof(int), 1, f) != 1 ||
                    fread(m->colors, 1, sizeof(m->colors), f) != sizeof(m->colors)) {
                    memset(m, 0, sizeof(*m));
                    break;
                }
                if (m->scale < 0) m->scale = 0;
                if (m->scale > 4) m->scale = 4;
                m->active = 1;
                if (m->id >= g_next_map_id) g_next_map_id = m->id + 1;
            }
        }
    }
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
    reset_player_damage_visuals();
    pex_logf("world state loaded dir=%s version=%d health=%d dead=%d origin=%d,%d", g_loaded_world_dir, version, g_player_health, g_player_dead, g_flat_world_origin_x, g_flat_world_origin_z);
    g_save_dirty = (version < 37) ? 1 : 0;
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
    /* Do not top up saved passive mobs on every load; natural spawning refills slowly under cap. */

    g_chat_input[0] = 0;
    g_chat_count = 0;
    reset_player_damage_visuals();
    g_last_autosave_tick = g_ingame_ticks;
    g_save_message_ticks = 0;
    hud_add_chat(loaded_state ? "Loaded saved world." : "Loaded world.");
    if (player_is_creative()) {
        g_player_dead = 0;
        if (g_player_health <= 0) player_health_set_silent(20);
        player_food_reset();
    }
    if (g_player_dead || g_player_health <= 0) {
        g_player_dead = 1;
        player_health_set_silent(0);
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
    { int gt = 0; if (read_level_int_tag_for_dir(g_loaded_world_dir, "GameType", &gt)) g_game_mode = gt ? 1 : 0; else g_game_mode = 0; }
    { int mf = 1; if (read_level_int_tag_for_dir(g_loaded_world_dir, "MapFeatures", &mf)) g_world_map_features = mf ? 1 : 0; else g_world_map_features = 1; }
    {
        long long seed = 0;
        if (read_world_seed_for_dir(g_loaded_world_dir, &seed)) g_world_seed = seed;
    }
    migrate_legacy_chunk_folder(g_loaded_world_dir);

    int loaded_state = load_current_world_state();
    if (!loaded_state) {
        inventory_reset();
        player_health_set_silent(20);
        player_food_reset();
        player_xp_reset();
        reset_player_damage_visuals();
        memset(g_armor_inventory, 0, sizeof(g_armor_inventory));
        g_player_armor = 0;
        g_player_damage_remainder = 0;
        g_ingame_ticks = 0;
        g_world_time = 0;
        flat_center_origin_near(g_player_x, g_player_z);
        flat_generate_origin_blocks();
    }
    finish_prepared_world_entry(loaded_state);
}


typedef enum PexWorldQuitStage {
    PEX_WORLD_QUIT_IDLE = 0,
    PEX_WORLD_QUIT_STARTING,
    PEX_WORLD_QUIT_REQUESTING_STOPS,
    PEX_WORLD_QUIT_STOPPING_SIMULATION,
    PEX_WORLD_QUIT_DISCONNECTING,
    PEX_WORLD_QUIT_STOPPING_STREAMING,
    PEX_WORLD_QUIT_STOPPING_RENDER_WORKERS,
    PEX_WORLD_QUIT_STOPPING_ENTITY_RENDERER,
    PEX_WORLD_QUIT_DRAINING_SAVES,
    PEX_WORLD_QUIT_WRITING_FINAL_SAVE,
    PEX_WORLD_QUIT_RELEASING_LOCKS,
    PEX_WORLD_QUIT_BACKGROUND_DONE,
    PEX_WORLD_QUIT_RELEASING_RENDER_RESOURCES,
    PEX_WORLD_QUIT_CLEARING_WORLD_STATE,
    PEX_WORLD_QUIT_DONE,
    PEX_WORLD_QUIT_FAILED
} PexWorldQuitStage;

static volatile LONG g_world_quit_active = 0;
static volatile LONG g_world_quit_worker_done = 0;
static volatile LONG g_world_quit_stage = PEX_WORLD_QUIT_IDLE;
static volatile LONG g_world_quit_progress = 0;
static HANDLE g_world_quit_thread = NULL;
static int g_world_quit_was_multiplayer = 0;
static int g_world_quit_had_local_world = 0;
static int g_world_quit_finalize_step = 0;
static double g_world_quit_started_at = 0.0;

static void world_quit_set_stage(PexWorldQuitStage stage, int progress) {
    if (progress < 0) progress = 0;
    if (progress > 100) progress = 100;
    InterlockedExchange(&g_world_quit_progress, (LONG)progress);
    InterlockedExchange(&g_world_quit_stage, (LONG)stage);
}

static int world_quit_is_active(void) {
    return InterlockedCompareExchange(&g_world_quit_active, 0, 0) ? 1 : 0;
}

static int world_quit_progress_percent(void) {
    int p = (int)InterlockedCompareExchange(&g_world_quit_progress, 0, 0);
    if (p < 0) p = 0;
    if (p > 100) p = 100;
    return p;
}

static const char *world_quit_stage_text(void) {
    switch ((PexWorldQuitStage)InterlockedCompareExchange(&g_world_quit_stage, 0, 0)) {
        case PEX_WORLD_QUIT_STARTING: return "Preparing safe shutdown";
        case PEX_WORLD_QUIT_REQUESTING_STOPS: return "Stopping world activity";
        case PEX_WORLD_QUIT_STOPPING_SIMULATION: return "Stopping simulation";
        case PEX_WORLD_QUIT_DISCONNECTING: return g_world_quit_was_multiplayer ? "Disconnecting from server" : "Stopping world cache";
        case PEX_WORLD_QUIT_STOPPING_STREAMING: return "Stopping terrain and lighting";
        case PEX_WORLD_QUIT_STOPPING_RENDER_WORKERS: return "Cancelling mesh builders";
        case PEX_WORLD_QUIT_STOPPING_ENTITY_RENDERER: return "Stopping entity renderer";
        case PEX_WORLD_QUIT_DRAINING_SAVES: return "Finishing pending saves";
        case PEX_WORLD_QUIT_WRITING_FINAL_SAVE: return "Saving world to disk";
        case PEX_WORLD_QUIT_RELEASING_LOCKS: return "Closing world services";
        case PEX_WORLD_QUIT_BACKGROUND_DONE: return "World services stopped";
        case PEX_WORLD_QUIT_RELEASING_RENDER_RESOURCES: return "Releasing world graphics";
        case PEX_WORLD_QUIT_CLEARING_WORLD_STATE: return "Clearing world memory";
        case PEX_WORLD_QUIT_DONE: return "Done";
        case PEX_WORLD_QUIT_FAILED: return "Safe shutdown could not start";
        default: return "Saving and closing world";
    }
}

/* Request every producer to stop before waiting for any one producer.  The old
   synchronous path joined simulation first; if it was waiting on streaming or
   lighting work that had not yet received a stop request, the button callback
   could deadlock the render/message thread. */
static void world_quit_request_all_stops(void) {
    ingame_tick_async_request_stop();
    pex_net_disconnect_request_stop();
    pex_mp_cache_save_request_stop();
    world_stream_service_request_stop();
    async_section_mesh_request_stop();
    passive_render_worker_request_stop();
}

static DWORD WINAPI world_quit_worker_proc(LPVOID unused) {
    (void)unused;
    g_pex_profile_thread_role = PEX_PROFILE_ROLE_ASYNC_STREAM;

    world_quit_set_stage(PEX_WORLD_QUIT_REQUESTING_STOPS, 5);
    world_quit_request_all_stops();

    world_quit_set_stage(PEX_WORLD_QUIT_STOPPING_SIMULATION, 14);
    ingame_tick_async_shutdown();

    world_quit_set_stage(PEX_WORLD_QUIT_DISCONNECTING, 26);
    if (g_world_quit_was_multiplayer) pex_net_disconnect();
    else pex_mp_cache_save_shutdown();

    world_quit_set_stage(PEX_WORLD_QUIT_STOPPING_STREAMING, 42);
    world_stream_service_shutdown();

    world_quit_set_stage(PEX_WORLD_QUIT_STOPPING_RENDER_WORKERS, 56);
    async_section_mesh_shutdown();

    world_quit_set_stage(PEX_WORLD_QUIT_STOPPING_ENTITY_RENDERER, 66);
    passive_render_worker_shutdown();

    world_quit_set_stage(PEX_WORLD_QUIT_DRAINING_SAVES, 70);
    pex_join_save_thread_for_exit();

#if !(defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY)
    if (g_world_quit_had_local_world) {
        world_quit_set_stage(PEX_WORLD_QUIT_WRITING_FINAL_SAVE, 80);
        save_world_state_sync();
    }
#endif

    world_quit_set_stage(PEX_WORLD_QUIT_RELEASING_LOCKS, 90);
    world_stream_shared_locks_shutdown();

    world_quit_set_stage(PEX_WORLD_QUIT_BACKGROUND_DONE, 92);
    InterlockedExchange(&g_world_quit_worker_done, 1);
    return 0;
}

static void world_quit_clear_session_state(void) {
    flat_world_unload_state();
    passive_mobs_reset();

    memset(&g_worldgen, 0, sizeof(g_worldgen));
    g_loaded_world_dir[0] = 0;
    g_loaded_world_name[0] = 0;
    g_chat_input[0] = 0;
    g_chat_count = 0;
    g_save_dirty = 0;
    g_save_message_ticks = 0;
    g_player_dead = 0;
    g_player_motion_x = g_player_motion_y = g_player_motion_z = 0.0f;
    reset_player_damage_visuals();
}

/* Called once per rendered frame.  Blocking joins and disk I/O happen on the
   coordinator thread; only renderer-owned destruction and final state clearing
   happen here, one visible stage at a time. */
static void world_quit_pump(void) {
    if (!world_quit_is_active()) return;
    if (!InterlockedCompareExchange(&g_world_quit_worker_done, 0, 0)) return;

    if (g_world_quit_finalize_step == 0) {
        if (g_world_quit_thread) {
            WaitForSingleObject(g_world_quit_thread, INFINITE);
            CloseHandle(g_world_quit_thread);
            g_world_quit_thread = NULL;
        }
        pex_net_release_render_resources();
        flat_world_render_resources_release_begin();
        world_quit_set_stage(PEX_WORLD_QUIT_RELEASING_RENDER_RESOURCES, 93);
        g_world_quit_finalize_step = 1;
        return;
    }
    if (g_world_quit_finalize_step == 1) {
        int finished = flat_world_render_resources_release_step(128);
        int render_pct = flat_world_render_resources_release_progress();
        world_quit_set_stage(PEX_WORLD_QUIT_RELEASING_RENDER_RESOURCES, 93 + (render_pct * 4) / 100);
        if (!finished) return;
        world_quit_set_stage(PEX_WORLD_QUIT_CLEARING_WORLD_STATE, 98);
        g_world_quit_finalize_step = 2;
        return;
    }
    if (g_world_quit_finalize_step == 2) {
        world_quit_clear_session_state();
        world_quit_set_stage(PEX_WORLD_QUIT_DONE, 100);
        g_world_quit_finalize_step = 3;
        return;
    }

    pex_logf("world teardown complete in %.3f sec; entering title", now_seconds() - g_world_quit_started_at);
    InterlockedExchange(&g_world_quit_active, 0);
    InterlockedExchange(&g_world_quit_worker_done, 0);
    g_world_quit_finalize_step = 0;
    set_screen(SCREEN_TITLE);
}

/* If the application window is closed while the progress screen is active,
   finish the coordinator before the normal process-wide shutdown starts.  This
   prevents two shutdown paths from joining/freeing the same worker objects. */
static void world_quit_shutdown_for_app_exit(void) {
    if (!world_quit_is_active()) return;

    if (g_world_quit_thread) {
        WaitForSingleObject(g_world_quit_thread, INFINITE);
        CloseHandle(g_world_quit_thread);
        g_world_quit_thread = NULL;
    }
    if (g_world_quit_finalize_step < 1) pex_net_release_render_resources();
    if (g_world_quit_finalize_step < 2) flat_world_render_resources_release();
    if (g_world_quit_finalize_step < 3) world_quit_clear_session_state();

    g_world_quit_finalize_step = 0;
    InterlockedExchange(&g_world_quit_worker_done, 0);
    InterlockedExchange(&g_world_quit_active, 0);
}

static void leave_world_to_title(void) {
    if (world_quit_is_active()) return;
    pex_stats_world_left();

    g_world_quit_was_multiplayer = (g_mp_connected || pex_net_is_connecting()) ? 1 : 0;
    g_world_quit_had_local_world = (!g_world_quit_was_multiplayer && g_loaded_world_dir[0]) ? 1 : 0;
    g_world_quit_finalize_step = 0;
    g_world_quit_started_at = now_seconds();
    InterlockedExchange(&g_world_quit_worker_done, 0);
    InterlockedExchange(&g_world_quit_active, 1);
    world_quit_set_stage(PEX_WORLD_QUIT_STARTING, 1);

    pex_logf("world teardown queued multiplayer=%d local=%d", g_world_quit_was_multiplayer, g_world_quit_had_local_world);

    /* Make the old world inaudible and inaccessible to normal frame/tick paths
       immediately.  The coordinator then owns the remaining teardown. */
    pex_sound_stop_world_audio();
    g_record_is_playing = 0;
    g_record_playing_up_for = 0;
    g_record_playing_text[0] = 0;
    set_screen(SCREEN_SAVING_QUIT);

    /* These calls only flip stop flags, wake events, and close nonblocking
       sockets.  Issue them before creating the coordinator so the old world
       starts becoming quiescent in the same button callback. */
    world_quit_set_stage(PEX_WORLD_QUIT_REQUESTING_STOPS, 5);
    world_quit_request_all_stops();

    g_world_quit_thread = CreateThread(NULL, 0x100000, world_quit_worker_proc, NULL, 0, NULL);
    if (g_world_quit_thread) {
        SetThreadPriority(g_world_quit_thread, THREAD_PRIORITY_BELOW_NORMAL);
    } else {
        /* Thread creation failure is exceptionally rare.  Preserve correctness
           rather than returning to gameplay with a half-torn-down session. */
        pex_logf("world teardown coordinator thread creation failed; using synchronous fallback");
        world_quit_set_stage(PEX_WORLD_QUIT_FAILED, 2);
        world_quit_worker_proc(NULL);
    }
}

static void start_world_generation_in_dir(const char *world_dir, const char *world_name, int slot) {
    memset(&g_worldgen, 0, sizeof(g_worldgen));
    g_worldgen.active = 1;
    g_worldgen.slot = slot;
    g_worldgen.radius = 4;
    g_worldgen.total_chunks = (g_worldgen.radius * 2 + 1) * (g_worldgen.radius * 2 + 1);
    if (g_pending_seed_set) g_worldgen.seed = g_pending_world_seed;
    else g_worldgen.seed = ((long long)time(NULL) << 32) ^ (long long)GetTickCount();
    g_world_seed = g_worldgen.seed;
    g_world_type = g_pending_world_type;
    g_game_mode = g_pending_game_mode ? 1 : 0;
    g_world_map_features = g_pending_map_features ? 1 : 0;
    snprintf(g_worldgen.world_name, sizeof(g_worldgen.world_name), "%s", (world_name && world_name[0]) ? world_name : "World");
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    snprintf(g_worldgen.world_dir, sizeof(g_worldgen.world_dir), "memory:/%s", g_worldgen.world_name);
#elif defined(PEX_PLATFORM_WII)
    /* Dolphin/homebrew can run without an SD image.  In that case do not touch
       FAT at all while creating/loading worlds; generate an in-RAM world like
       the PSP memory build. */
    if (!g_wii_fat_ready) snprintf(g_worldgen.world_dir, sizeof(g_worldgen.world_dir), "memory:/%s", g_worldgen.world_name);
    else snprintf(g_worldgen.world_dir, sizeof(g_worldgen.world_dir), "%s", (world_dir && world_dir[0]) ? world_dir : g_save_dir);
#else
    snprintf(g_worldgen.world_dir, sizeof(g_worldgen.world_dir), "%s", (world_dir && world_dir[0]) ? world_dir : g_save_dir);
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
    g_worldgen.newly_created = 1;
    snprintf(g_worldgen.title, sizeof(g_worldgen.title), "Generating RAM world");
    snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Building terrain");
#else
#if defined(PEX_PLATFORM_WII)
    if (!g_wii_fat_ready) {
        g_worldgen.existing_world = 0;
        g_worldgen.newly_created = 1;
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
            g_worldgen.newly_created = 0;
            snprintf(g_worldgen.title, sizeof(g_worldgen.title), "Loading level");
            snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Loading chunks");
        } else {
            g_worldgen.newly_created = 1;
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

static void start_world_generation(int slot) {
    char dir[MAX_PATHBUF];
    char name[64];
    snprintf(name, sizeof(name), "World%d", slot);
    snprintf(dir, sizeof(dir), "%s\\%s", g_save_dir, name);
    start_world_generation_in_dir(dir, name, slot);
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
        g_game_mode = g_pending_game_mode ? 1 : 0;
        g_world_seed = g_worldgen.seed;
        g_worldgen.loaded_state = 0;
#else
#if defined(PEX_PLATFORM_WII)
        if (!g_wii_fat_ready || strncmp(g_loaded_world_dir, "memory:", 7) == 0) {
            g_world_type = g_pending_world_type;
            g_game_mode = g_pending_game_mode ? 1 : 0;
            g_world_seed = g_worldgen.seed;
            g_worldgen.loaded_state = 0;
            wii_debug_logf("worldgen phase0: memory world type=%d seed=%lld", g_world_type, (long long)g_world_seed);
        } else
#endif
        {
            g_world_type = read_world_type_for_dir(g_loaded_world_dir);
            { int gt = 0; if (read_level_int_tag_for_dir(g_loaded_world_dir, "GameType", &gt)) g_game_mode = gt ? 1 : 0; else g_game_mode = 0; }
            { int mf = 1; if (read_level_int_tag_for_dir(g_loaded_world_dir, "MapFeatures", &mf)) g_world_map_features = mf ? 1 : 0; else g_world_map_features = 1; }
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
            player_health_set_silent(20);
            player_food_reset();
            player_xp_reset();
            reset_player_damage_visuals();
            memset(g_armor_inventory, 0, sizeof(g_armor_inventory));
            g_player_armor = 0;
            g_player_damage_remainder = 0;
            g_player_dead = 0;
            g_player_fall_distance = 0.0f;
            g_ingame_ticks = 0;
        g_world_time = 0;
            g_player_x = g_player_prev_x = 0.5f;
            g_player_y = g_player_prev_y = 5.62f;
            g_player_z = g_player_prev_z = 0.5f;
            flat_center_origin_near(g_player_x, g_player_z);
        } else if (g_last_load_state_had_terrain) {
            /* Java 1.2.5 RenderGlobal.loadRenderers() only marks world renderers
               dirty after a world switch; it does not block the loading screen until
               every chunk mesh/display-list has been rebuilt.  The old C preload
               gate could loop forever when an async mesh result was stale/dropped,
               so enter the world after terrain/light state is ready and let the
               normal renderer queue rebuild visible sections from final light. */
            g_worldgen.phase = 3;
            g_worldgen.progress = 100;
            snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Entering world");
            return;
        }

        flat_prepare_initial_generation();
        flat_begin_initial_generation();
        g_worldgen.terrain_total = flat_initial_generation_total();
        g_worldgen.terrain_done = flat_initial_generation_done();
        g_worldgen.phase = 1;
        snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Building terrain %d/%d",
                 g_worldgen.terrain_done, g_worldgen.terrain_total);
        g_worldgen.progress = 5;
        return;
    }

    if (g_worldgen.phase == 1) {
        flat_continue_initial_generation();
        g_worldgen.terrain_total = flat_initial_generation_total();
        g_worldgen.terrain_done = flat_initial_generation_done();
        if (g_worldgen.terrain_total <= 0) g_worldgen.terrain_total = 1;
        int terrain_p = (g_worldgen.terrain_done * 82) / g_worldgen.terrain_total;
        g_worldgen.progress = 5 + terrain_p;
        if (g_worldgen.progress > 87) g_worldgen.progress = 87;
        snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Building terrain %d/%d",
                 g_worldgen.terrain_done, g_worldgen.terrain_total);
        if (!flat_initial_generation_active()) {
            flat_begin_initial_light_settle();
            if (!flat_initial_light_settle_done()) {
                int light_p = flat_initial_light_settle_progress();
                g_worldgen.progress = 87 + (light_p * 5) / 100;
                if (g_worldgen.progress > 92) g_worldgen.progress = 92;
                snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Lighting terrain");
                return;
            }

            flat_finish_initial_generation();
            if (!g_worldgen.loaded_state) reset_flat_player_spawn();
            /* Match Java 1.2.5: terrain is preloaded and lighting is drained before
               entry, but renderer rebuilds are queued/updated after the world is
               active instead of being a blocking "Preparing chunks" phase. */
            g_worldgen.phase = 3;
            g_worldgen.progress = 100;
            snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Entering world");
        }
        return;
    }

    if (g_worldgen.phase == 2) {
        int complete = worldgen_mesh_prep_step(8);
        int total = worldgen_mesh_prep_total();
        int done = worldgen_mesh_prep_done();
        if (total <= 0) total = 1;
        int mesh_p = (done * 7) / total;
        g_worldgen.progress = 92 + mesh_p;
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
