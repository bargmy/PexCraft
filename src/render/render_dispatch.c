/* Split from original monolithic main.c. Included by src/main.c unity build. */

static void draw_fps_counter(void) {
    if (!g_opts.show_fps || g_debug_menu_shown || g_screen == SCREEN_TITLE) return;
    char line[32];
    snprintf(line, sizeof(line), "FPS: %d", g_debug_fps);
    draw_text(line, 2, 12, 14737632);
}

static int ft_compare_double(const void *a, const void *b) {
    double da = *(const double *)a;
    double db = *(const double *)b;
    return (da > db) - (da < db);
}

static int ft_percentile_index(int count, double pct) {
    int idx = (int)((double)count * pct);
    if (idx < 0) idx = 0;
    if (idx >= count) idx = count - 1;
    return idx;
}

static void update_frame_time_percentiles(void) {
    double sorted[128];
    int count = 0;
    for (int i = 0; i < 128; i++) {
        if (g_ft_history[i] > 0.0) sorted[count++] = g_ft_history[i] * 1000.0;
    }
    if (count <= 0) {
        g_ft_p50_ms = 0.0;
        g_ft_p95_ms = 0.0;
        g_ft_p99_ms = 0.0;
        return;
    }
    qsort(sorted, (size_t)count, sizeof(sorted[0]), ft_compare_double);
    g_ft_p50_ms = sorted[ft_percentile_index(count, 0.50)];
    g_ft_p95_ms = sorted[ft_percentile_index(count, 0.95)];
    g_ft_p99_ms = sorted[ft_percentile_index(count, 0.99)];
}

static void record_frame_time_sample(double render_entry_time) {
    static double last_render_entry_time = 0.0;

    if (last_render_entry_time > 0.0) {
        double frame_seconds = render_entry_time - last_render_entry_time;
        if (frame_seconds < 0.0) frame_seconds = 0.0;
        g_ft_history[g_ft_history_head] = frame_seconds;
        g_ft_history_head = (g_ft_history_head + 1) & 127;
        g_ft_last_frame_ms = frame_seconds * 1000.0;
        if (g_ft_last_frame_ms > g_ft_worst_ms) g_ft_worst_ms = g_ft_last_frame_ms;
    }
    last_render_entry_time = render_entry_time;
}

static void update_debug_fps_counter(void) {
    double t = now_seconds();
    if (g_debug_fps_last_time <= 0.0) {
        g_debug_fps_last_time = t;
        g_debug_frame_counter = 0;
    }

    g_debug_frame_counter++;
    double span = t - g_debug_fps_last_time;
    if (span >= 1.0) {
        int fps = (int)((double)g_debug_frame_counter / span + 0.5);
        g_debug_fps = fps;
        if (g_debug_min_fps == 0 || fps < g_debug_min_fps) g_debug_min_fps = fps;
        if (fps > g_debug_max_fps) g_debug_max_fps = fps;
        if (g_debug_menu_shown) update_frame_time_percentiles();
        g_debug_frame_counter = 0;
        g_debug_fps_last_time = t;
    }
}

static void draw_current_screen(float partial) {
    switch (g_screen) {
        case SCREEN_TITLE: draw_title_screen(partial); break;
        case SCREEN_OPTIONS: draw_options_screen(); break;
        case SCREEN_OPTIONS_MORE: draw_options_more_screen(); break;
        case SCREEN_SYSTEM_INFO: draw_system_info_screen(); break;
        case SCREEN_SKINS: draw_skins_screen(); break;
        case SCREEN_CONTROLS: draw_controls_screen(); break;
        case SCREEN_WORLD_SELECT:
        case SCREEN_WORLD_DELETE: draw_world_screen(); break;
        case SCREEN_CREATE_WORLD: draw_create_world_screen(); break;
        case SCREEN_WORLD_TYPE: draw_world_type_screen(); break;
        case SCREEN_RENAME_WORLD: draw_rename_world_screen(); break;
        case SCREEN_CONFIRM_DELETE: draw_confirm_delete(); break;
        case SCREEN_MULTIPLAYER: draw_multiplayer(); break;
        case SCREEN_CONNECTING: draw_connecting_screen(); break;
        case SCREEN_TEXPACK: draw_texpack(); break;
        case SCREEN_TEXPACK_INSTALL: draw_texturepack_install_screen(); break;
        case SCREEN_GENERATING: draw_generating_screen(); break;
        case SCREEN_INGAME: draw_ingame_screen(); break;
        case SCREEN_PAUSE: draw_pause_screen(); break;
        case SCREEN_INVENTORY: draw_inventory_screen(); break;
        case SCREEN_CREATIVE: draw_creative_screen(); break;
        case SCREEN_WORKBENCH: draw_workbench_screen(); break;
        case SCREEN_FURNACE: draw_furnace_screen(); break;
        case SCREEN_CHEST: draw_chest_screen(); break;
        case SCREEN_DEATH: draw_death_screen(); break;
        case SCREEN_CHAT: draw_chat_screen(); break;
        case SCREEN_NOTICE: draw_notice(); break;
        case SCREEN_RENDERER_RESTART_PROMPT: draw_renderer_restart_prompt(); break;
        case SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT: draw_pack_download_prompt(); break;
        case SCREEN_CLASSIC_PACK_WARNING: draw_classic_pack_warning(); break;
    }
}

static void render(float partial) {
    double render_profile_time = profile_begin();
    double render_entry_time = now_seconds();
    static int last_debug_menu_shown = 0;
    if (g_debug_menu_shown && !last_debug_menu_shown) {
        g_ft_worst_ms = 0.0;
    }
    last_debug_menu_shown = g_debug_menu_shown;

    double part_start = profile_begin();
    update_debug_fps_counter();
    profile_add_time(PROF_RENDER_FPS_UPDATE, part_start);
    g_frame_partial = partial;
    part_start = profile_begin();
    player_render_begin_frame();
    profile_add_time(PROF_PLAYER_RENDER_SNAPSHOT, part_start);

    part_start = profile_begin();
    int begin_ok = pex_renderer_begin_frame();
    profile_add_time(PROF_RENDERER_BEGIN_FRAME, part_start);
    if (!begin_ok) {
        g_render_ms_last = 0.0;
        record_frame_time_sample(render_entry_time);
        profile_add_time(PROF_RENDER_TOTAL, render_profile_time);
        return;
    }
    g_render_w = g_win_w;
    g_render_h = g_win_h;
    part_start = profile_begin();
    glViewport(0, 0, g_render_w, g_render_h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setup_gui_projection();
    profile_add_time(PROF_RENDER_CLEAR_SETUP, part_start);
    double render_start_time = now_seconds();
    part_start = profile_begin();
    draw_current_screen(partial);
    profile_add_time(PROF_DRAW_CURRENT_SCREEN, part_start);
    profile_add_time(PROF_SCREEN_DRAW, part_start);
#ifdef PEX_PLATFORM_ANDROID
    draw_android_touch_controls();
#endif
    part_start = profile_begin();
    draw_gamepad_virtual_cursor();
    profile_add_time(PROF_GAMEPAD_CURSOR, part_start);
    part_start = profile_begin();
    draw_fps_counter();
    profile_add_time(PROF_FPS_COUNTER, part_start);
    part_start = profile_begin();
    pex_renderer_present();
    profile_add_time(PROF_PRESENT, part_start);
    g_render_ms_last = (now_seconds() - render_start_time) * 1000.0;
    record_frame_time_sample(render_entry_time);
    profile_add_time(PROF_RENDER_TOTAL, render_profile_time);
}
