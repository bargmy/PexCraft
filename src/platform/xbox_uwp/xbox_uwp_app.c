/* CoreWindow-driven PexCraft loop for Xbox One / UWP. */

static int g_xbox_uwp_save_cs_ready = 0;

static void pex_xbox_uwp_tick_frame(void) {
    profile_begin_frame();
    double prof_start = profile_begin();
    pex_gamepad_update();
    profile_add_time(PROF_GAMEPAD_POLL, prof_start);
    if (g_mp_connected || pex_net_is_connecting()) {
        prof_start = profile_begin();
        pex_net_poll();
        profile_add_time(PROF_NET_POLL, prof_start);
    }
    double frame_start_time = now_seconds();
    double dt = frame_start_time - g_last_time;
    if (dt < 0) dt = 0;
    if (dt > 0.25) dt = 0.25;
    g_last_time = frame_start_time;
    static double tick_accum = 0.0;
    tick_accum += dt * 20.0;
    int ticks_this_frame = 0;
    while (tick_accum >= 1.0 && ticks_this_frame < 3) {
        double tick_start = profile_begin();
        g_ticks++;
        if (g_screen == SCREEN_TITLE) tick_title_blocks();
        if (g_screen == SCREEN_GENERATING) worldgen_tick();
        if (g_screen == SCREEN_TEXPACK_INSTALL) pack_install_tick();
        if (g_screen == SCREEN_INGAME || g_screen == SCREEN_CHAT ||
            g_screen == SCREEN_INVENTORY || g_screen == SCREEN_CREATIVE || g_screen == SCREEN_WORKBENCH ||
            g_screen == SCREEN_FURNACE || g_screen == SCREEN_CHEST ||
            g_screen == SCREEN_DEATH || (g_mp_connected && g_screen == SCREEN_PAUSE)) {
            ingame_tick_async_queue();
        }
        profile_add_time(PROF_TICK_TOTAL, tick_start);
        tick_accum -= 1.0;
        ticks_this_frame++;
    }
    if (ticks_this_frame >= 3 && tick_accum > 1.0) tick_accum = 1.0;
    float partial = ingame_tick_async_render_partial((float)tick_accum);
    if (g_mp_connected) pex_net_update_smoothing();
    ingame_pump_async_tick();
    render(partial);
    profile_end_frame();
    loggy_draw();
}

int pex_xbox_uwp_engine_init(void *core_window, int width, int height) {
    pex_log_init();
    pex_install_crash_handlers();
    pex_logf("Xbox UWP engine init begin");
    init_dirs();
    srand((unsigned int)time(NULL));
    load_options();
    pex_sound_rescan();
    snprintf(g_multiplayer_ip, sizeof(g_multiplayer_ip), "%s", g_opts.last_server);
    snprintf(g_multiplayer_username, sizeof(g_multiplayer_username), "%s", g_opts.username[0] ? g_opts.username : "Player");
    g_win_w = width > 0 ? width : 1280;
    g_win_h = height > 0 ? height : 720;
    g_render_w = g_win_w;
    g_render_h = g_win_h;
    setup_scale();
    if (!pex_renderer_backend_init(core_window)) {
        pex_logf("Xbox UWP renderer/assets init failed");
        return 0;
    }
    if (g_loggy_enabled) loggy_init();
    if (should_show_pack_download_prompt()) set_screen(SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT);
    else if (!strcmp(g_opts.skin, CLASSIC_PACK_NAME) && classic_resources_need_update()) set_screen(SCREEN_CLASSIC_PACK_WARNING);
    else set_screen(pex_startup_screen());
    InitializeCriticalSection(&g_save_cs);
    g_xbox_uwp_save_cs_ready = 1;
    g_last_time = now_seconds();
    g_running = 1;
    pex_logf("Xbox UWP engine init OK");
    return 1;
}

void pex_xbox_uwp_engine_frame(void) { if (g_running) pex_xbox_uwp_tick_frame(); }

void pex_xbox_uwp_engine_resize(int width, int height) {
    if (width < 1) width = 1;
    if (height < 1) height = 1;
    g_win_w = width;
    g_win_h = height;
    g_render_w = width;
    g_render_h = height;
    setup_scale();
    pex_renderer_resize(width, height);
    rebuild_screen();
}

void pex_xbox_uwp_engine_key_down(int vk) {
    pex_xbox_uwp_set_key_state(vk, 1);
    handle_keydown((WPARAM)vk);
}
void pex_xbox_uwp_engine_key_up(int vk) { pex_xbox_uwp_set_key_state(vk, 0); }
void pex_xbox_uwp_engine_char(uint32_t ch) { if (ch >= 32 && ch < 127) handle_char((WPARAM)ch); }

void pex_xbox_uwp_engine_shutdown(void) {
    save_world_state_for_exit();
    world_stream_service_shutdown();
    ingame_tick_async_shutdown();
    async_section_mesh_shutdown();
    pex_gamepad_shutdown();
    pex_sound_shutdown();
    loggy_shutdown();
    pex_renderer_shutdown();
    pex_join_save_thread_for_exit();
    if (g_xbox_uwp_save_cs_ready) { DeleteCriticalSection(&g_save_cs); g_xbox_uwp_save_cs_ready = 0; }
    pex_logf("Xbox UWP engine shutdown");
    pex_log_shutdown();
}
