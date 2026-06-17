/* Split from original monolithic main.c. Included by src/main.c unity build. */



static void world_type_path_for_dir(const char *dir, char *out, size_t cap) {
    snprintf(out, cap, "%s\\pexcraft_world_type.txt", dir);
}

static int read_world_type_for_dir(const char *dir) {
    char path[MAX_PATHBUF];
    world_type_path_for_dir(dir, path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    char buf[32] = "";
    fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    return (strstr(buf, "normal") != NULL) ? 1 : 0;
}

static void write_world_type_for_dir(const char *dir, int type) {
    char path[MAX_PATHBUF];
    world_type_path_for_dir(dir, path, sizeof(path));
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fputs(type ? "normal\n" : "flat\n", f);
    fclose(f);
}

static void world_state_path(char *out, size_t cap) {
    if (!g_loaded_world_dir[0]) {
        if (cap) out[0] = 0;
        return;
    }
    snprintf(out, cap, "%s\\pexcraft_flat_world.dat", g_loaded_world_dir);
}

static void save_current_world_state(void) {
    if (!g_loaded_world_dir[0]) return;

    char path[MAX_PATHBUF];
    world_state_path(path, sizeof(path));
    FILE *f = fopen(path, "wb");
    if (!f) return;

    char magic[8] = {'P','X','C','F','L','T','1','1'};
    int version = 11;
    int w = FLAT_WORLD_SIZE;
    int h = FLAT_WORLD_HEIGHT;
    int y_min = FLAT_WORLD_Y_MIN;
    fwrite(magic, 1, 8, f);
    fwrite(&version, sizeof(version), 1, f);
    fwrite(&w, sizeof(w), 1, f);
    fwrite(&h, sizeof(h), 1, f);
    fwrite(&y_min, sizeof(y_min), 1, f);

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
    for (int i = 0; i < 27; i++) {
        fwrite(&g_chest_slots[i].id, sizeof(int), 1, f);
        fwrite(&g_chest_slots[i].count, sizeof(int), 1, f);
        fwrite(&g_chest_slots[i].damage, sizeof(int), 1, f);
    }

    fwrite(g_flat_blocks, sizeof(g_flat_blocks), 1, f);

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

    fclose(f);
    write_session_lock(g_loaded_world_dir);
    g_save_dirty = 0;
}

static int load_current_world_state(void) {
    if (!g_loaded_world_dir[0]) return 0;

    char path[MAX_PATHBUF];
    world_state_path(path, sizeof(path));
    FILE *f = fopen(path, "rb");
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
        (memcmp(magic, "PXCFLAT4", 8) == 0 && version == 4) ||
        (memcmp(magic, "PXCFLAT6", 8) == 0 && version == 6) ||
        (memcmp(magic, "PXCFLAT7", 8) == 0 && version == 7) ||
        (memcmp(magic, "PXCFLAT9", 8) == 0 && version == 9) ||
        (memcmp(magic, "PXCFLT10", 8) == 0 && version == 10) ||
        (memcmp(magic, "PXCFLT11", 8) == 0 && version == 11);
    if (!ok_magic || w != FLAT_WORLD_SIZE || h != FLAT_WORLD_HEIGHT || y_min != FLAT_WORLD_Y_MIN) {
        fclose(f);
        return 0;
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
    }

    if (fread(g_flat_blocks, sizeof(g_flat_blocks), 1, f) != 1) {
        fclose(f);
        return 0;
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
    g_flat_world_geometry_dirty = 1;
    g_save_dirty = 0;
    return 1;
}


static void enter_world_from_job(void) {
    snprintf(g_loaded_world_dir, sizeof(g_loaded_world_dir), "%s", g_worldgen.world_dir);
    snprintf(g_loaded_world_name, sizeof(g_loaded_world_name), "%s", g_worldgen.world_name[0] ? g_worldgen.world_name : "World");
    g_worldgen.active = 0;
    g_selected_hotbar_slot = 0;
    g_world_type = read_world_type_for_dir(g_loaded_world_dir);
    flat_world_reset_blocks();
    inventory_reset();
    g_player_health = 20;
    g_player_prev_health = 20;
    g_player_armor = 0;
    g_ingame_ticks = 0;
    reset_flat_player_spawn();

    int loaded_state = load_current_world_state();
    if (loaded_state && (flat_player_aabb_collides(g_player_x, g_player_y, g_player_z) || flat_player_suffocation_block())) reset_flat_player_spawn();

    g_chat_input[0] = 0;
    g_chat_count = 0;
    hud_add_chat(loaded_state ? "Loaded saved world." : "Loaded world.");
    set_screen(SCREEN_INGAME);
}

static void leave_world_to_title(void) {
    if (g_loaded_world_dir[0]) {
        save_current_world_state();
        write_session_lock(g_loaded_world_dir);
    }
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
    snprintf(g_worldgen.world_dir, sizeof(g_worldgen.world_dir), "%s\\World%d", g_save_dir, slot);
    snprintf(g_worldgen.title, sizeof(g_worldgen.title), "Generating level");
    snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Building terrain");
    g_worldgen.progress = 0;

    char level_path[MAX_PATHBUF];
    snprintf(level_path, sizeof(level_path), "%s\\level.dat", g_worldgen.world_dir);
    if (file_exists(level_path)) {
        g_worldgen.existing_world = 1;
        snprintf(g_worldgen.title, sizeof(g_worldgen.title), "Loading level");
        snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Loading chunks");
    } else {
        /* Flat test-world mode: keep the Beta loading flow, but do not run the
           heavy chunk generator.  The in-game renderer below ignores chunk files
           and displays a fixed 100x100x64 grass world using /terrain.png, while this
           still creates a save folder/session/level.dat so the world slot exists. */
        if (dir_exists(g_worldgen.world_dir)) delete_recursive(g_worldgen.world_dir);
        make_dir_recursive(g_worldgen.world_dir);
        write_session_lock(g_worldgen.world_dir);
        write_world_type_for_dir(g_worldgen.world_dir, g_world_type);
        write_level_dat(g_worldgen.world_dir, g_worldgen.world_name, g_worldgen.seed, 50, 5, 50, 0);
        if (g_world_type == 1) {
            for (int cx = -2; cx <= 2; cx++) for (int cz = -2; cz <= 2; cz++) write_chunk_file(g_worldgen.world_dir, cx, cz, g_worldgen.seed);
        }
        g_worldgen.existing_world = 1;
        snprintf(g_worldgen.title, sizeof(g_worldgen.title), "Loading level");
        snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Building terrain");
    }
    set_screen(SCREEN_GENERATING);
}

static void worldgen_tick(void) {
    if (!g_worldgen.active) return;
    if (g_worldgen.existing_world) {
        if (g_worldgen.progress < 100) {
            g_worldgen.progress += 25;
            snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Loading chunks");
        } else {
            snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Done!");
            if (++g_worldgen.done_ticks > 10) { enter_world_from_job(); }
        }
        return;
    }
    if (g_worldgen.chunks_done < g_worldgen.total_chunks) {
        int r = g_worldgen.radius;
        int n = g_worldgen.chunks_done;
        int cx = (n % (r * 2 + 1)) - r;
        int cz = (n / (r * 2 + 1)) - r;
        snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Building terrain");
        write_chunk_file(g_worldgen.world_dir, cx, cz, g_worldgen.seed);
        g_worldgen.chunks_done++;
        g_worldgen.progress = (g_worldgen.chunks_done * 90) / g_worldgen.total_chunks;
        return;
    }
    if (g_worldgen.progress < 100) {
        snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Saving level");
        unsigned long long sz = world_dir_size_quick(g_worldgen.world_dir);
        write_level_dat(g_worldgen.world_dir, g_worldgen.world_name, g_worldgen.seed, 50, 5, 50, (long long)sz);
        g_worldgen.progress = 100;
        return;
    }
    snprintf(g_worldgen.status, sizeof(g_worldgen.status), "Done!");
    if (++g_worldgen.done_ticks > 12) { enter_world_from_job(); }
}

