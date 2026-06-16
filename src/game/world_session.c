/* Split from original monolithic main.c. Included by src/main.c unity build. */

static void enter_world_from_job(void) {
    snprintf(g_loaded_world_dir, sizeof(g_loaded_world_dir), "%s", g_worldgen.world_dir);
    snprintf(g_loaded_world_name, sizeof(g_loaded_world_name), "%s", g_worldgen.world_name[0] ? g_worldgen.world_name : "World");
    g_worldgen.active = 0;
    g_selected_hotbar_slot = 0;
    flat_world_reset_blocks();
    inventory_reset();
    g_player_health = 20;
    g_player_prev_health = 20;
    g_player_armor = 0;
    g_ingame_ticks = 0;
    reset_flat_player_spawn();
    g_chat_input[0] = 0;
    g_chat_count = 0;
    hud_add_chat("Loaded world.");
    set_screen(SCREEN_INGAME);
}

static void leave_world_to_title(void) {
    if (g_loaded_world_dir[0]) {
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
        write_level_dat(g_worldgen.world_dir, g_worldgen.world_name, g_worldgen.seed, 50, 5, 50, 0);
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

