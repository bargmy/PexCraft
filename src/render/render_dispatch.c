/* Split from original monolithic main.c. Included by src/main.c unity build. */

static void render(float partial) {
    g_frame_partial = partial;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setup_gui_projection();
    switch (g_screen) {
        case SCREEN_TITLE: draw_title_screen(partial); break;
        case SCREEN_OPTIONS: draw_options_screen(); break;
        case SCREEN_CONTROLS: draw_controls_screen(); break;
        case SCREEN_WORLD_SELECT:
        case SCREEN_WORLD_DELETE: draw_world_screen(); break;
        case SCREEN_CONFIRM_DELETE: draw_confirm_delete(); break;
        case SCREEN_MULTIPLAYER: draw_multiplayer(); break;
        case SCREEN_TEXPACK: draw_texpack(); break;
        case SCREEN_GENERATING: draw_generating_screen(); break;
        case SCREEN_INGAME: draw_ingame_screen(); break;
        case SCREEN_PAUSE: draw_pause_screen(); break;
        case SCREEN_INVENTORY: draw_inventory_screen(); break;
        case SCREEN_CHAT: draw_chat_screen(); break;
        case SCREEN_NOTICE: draw_notice(); break;
    }
    SwapBuffers(g_hdc);
}
