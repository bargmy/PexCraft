/* Split from original monolithic main.c. Included by src/main.c unity build. */

static void draw_fps_counter(void) {
    if (!g_opts.show_fps || g_debug_menu_shown) return;
    char line[32];
    snprintf(line, sizeof(line), "FPS: %d", g_debug_fps);
    draw_text(line, 2, 12, 14737632);
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
        g_debug_frame_counter = 0;
        g_debug_fps_last_time = t;
    }
}

static void render(float partial) {
    update_debug_fps_counter();
    g_frame_partial = partial;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setup_gui_projection();
    switch (g_screen) {
        case SCREEN_TITLE: draw_title_screen(partial); break;
        case SCREEN_OPTIONS: draw_options_screen(); break;
        case SCREEN_CONTROLS: draw_controls_screen(); break;
        case SCREEN_WORLD_SELECT:
        case SCREEN_WORLD_DELETE: draw_world_screen(); break;
        case SCREEN_WORLD_TYPE: draw_world_type_screen(); break;
        case SCREEN_CONFIRM_DELETE: draw_confirm_delete(); break;
        case SCREEN_MULTIPLAYER: draw_multiplayer(); break;
        case SCREEN_TEXPACK: draw_texpack(); break;
        case SCREEN_GENERATING: draw_generating_screen(); break;
        case SCREEN_INGAME: draw_ingame_screen(); break;
        case SCREEN_PAUSE: draw_pause_screen(); break;
        case SCREEN_INVENTORY: draw_inventory_screen(); break;
        case SCREEN_WORKBENCH: draw_workbench_screen(); break;
        case SCREEN_FURNACE: draw_furnace_screen(); break;
        case SCREEN_CHEST: draw_chest_screen(); break;
        case SCREEN_DEATH: draw_death_screen(); break;
        case SCREEN_CHAT: draw_chat_screen(); break;
        case SCREEN_NOTICE: draw_notice(); break;
    }
    draw_fps_counter();
    SwapBuffers(g_hdc);
}
