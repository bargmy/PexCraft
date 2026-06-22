/* Nintendo Wii application/bootstrap layer. */

static int init_gl(HWND unused) {
    (void)unused;
    wii_debug_stagef("renderer init / GX");
    if (!wii_gx_init()) { wii_debug_logf("wii_gx_init FAILED"); return 0; }
    wii_debug_memoryf("after wii_gx_init");
    wii_debug_stagef("input init");
    wii_input_init();
    glClearColor(0,0,0,1);
    glDisable(GL_DITHER);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    wii_debug_stagef("scan texture packs");
    scan_texture_packs();
    wii_debug_stagef("load default textures");
    if (!load_default_textures()) { wii_debug_logf("load_default_textures FAILED"); return 0; }
    wii_debug_memoryf("after texture load");
    if (g_selected_texpack > 0) {
        wii_debug_logf("applying selected texture pack index=%d", g_selected_texpack);
        apply_texture_pack_index(g_selected_texpack);
    } else {
        if (g_opts.skin_path[0]) load_custom_skin_path(g_opts.skin_path, 0);
        init_font_widths();
    }
    wii_debug_stagef("renderer init done");
    return 1;
}

static int pex_renderer_backend_init(HWND unused) {
    g_runtime_renderer_backend = RENDERER_OPENGL;
    g_selected_renderer_backend = RENDERER_OPENGL;
    return init_gl(unused);
}
static int pex_renderer_begin_frame(void) { wii_gx_begin_frame(); return 1; }
static void pex_renderer_present(void) { wii_gx_end_frame(); }
static void pex_renderer_resize(int w, int h) { (void)w; (void)h; }
static void pex_renderer_shutdown(void) { wii_gx_shutdown(); }
static void pex_gl_suppress_immediate(int on) { (void)on; }
static void apply_vsync_setting(void) { }
static void refresh_window_size_after_mode_change(void) { }
static void set_fullscreen_enabled(int enabled) { g_opts.fullscreen = enabled ? 1 : 0; }
static void toggle_fullscreen(void) { }

static void pex_join_save_thread_for_exit(void) {
    if (g_save_thread != NULL) {
        EnterCriticalSection(&g_save_cs);
        g_save_thread_shutdown = 1;
        LeaveCriticalSection(&g_save_cs);
        if (g_save_event) SetEvent(g_save_event);
        WaitForSingleObject(g_save_thread, INFINITE);
        CloseHandle(g_save_thread);
        g_save_thread = NULL;
        g_save_thread_done = 1;
    }
    if (g_save_event != NULL) { CloseHandle(g_save_event); g_save_event = NULL; }
    while (g_save_queue_head) { PexSaveSnapshot *ss = g_save_queue_head; g_save_queue_head = ss->next; pex_save_snapshot_free(ss); }
    g_save_queue_tail = NULL;
    g_save_thread_shutdown = 0;
}

static void save_world_state_for_exit(void) {
    pex_join_save_thread_for_exit();
    if (!g_wii_fat_ready) { wii_debug_logf("save skipped: SD/FAT unavailable"); return; }
    save_current_world_state_sync();
}

static volatile int g_wii_power_requested = 0;

#if defined(HW_RVL)
static void wii_power_callback(void) {
    g_wii_power_requested = 1;
}
#endif

static void wii_reserve_embedded_asset_mem2(void) {
    /* The embedded texture pak is now linked in the normal DOL data/rodata
       area. Do NOT force it to 0x90010000: that creates a huge address gap
       and can make elf2dol emit a hundreds-of-megabytes DOL. */
}

static void sleep_for_max_fps(double frame_start_time) {
    double sleep_start_time = now_seconds();
    int fps = g_opts.max_fps > 0 ? g_opts.max_fps : 60;
    double target = 1.0 / (double)fps;
    for (;;) {
        double elapsed = now_seconds() - frame_start_time;
        double remaining = target - elapsed;
        if (remaining <= 0.0) break;
        Sleep((DWORD)(remaining * 1000.0));
    }
    g_sleep_ms_last = (now_seconds() - sleep_start_time) * 1000.0;
}

static void main_loop(void) {
    wii_debug_stagef("enter main loop");
    g_last_time = now_seconds();
    double tick_accum = 0.0;
    unsigned int wii_frame_counter = 0;
    while (g_running && SYS_MainLoop()) {
        if (SYS_ResetButtonDown() || g_wii_power_requested) g_running = 0;
        pex_profile_frame_begin();
        double prof_start = pex_profile_begin();
        pex_gamepad_update();
        pex_profile_add(PROF_PUMP, prof_start);

        double frame_start_time = now_seconds();
        double t = frame_start_time;
        double dt = t - g_last_time;
        if (dt < 0) dt = 0;
        if (dt > 0.25) dt = 0.25;
        g_last_time = t;
        tick_accum += dt * 20.0;
        int ticks_this_frame = 0;
        while (tick_accum >= 1.0 && ticks_this_frame < 2) {
            double tick_start = pex_profile_begin();
            g_ticks++;
            if (g_screen == SCREEN_TITLE) tick_title_blocks();
            if (g_screen == SCREEN_GENERATING) {
                double t_worldgen = pex_profile_begin();
                worldgen_tick();
                pex_profile_add(PROF_WORLDGEN_TICK, t_worldgen);
            }
            if (g_screen == SCREEN_TEXPACK_INSTALL) classic_pack_install_tick();
            if (g_screen == SCREEN_INGAME || g_screen == SCREEN_CHAT ||
                g_screen == SCREEN_INVENTORY || g_screen == SCREEN_WORKBENCH ||
                g_screen == SCREEN_FURNACE || g_screen == SCREEN_CHEST ||
                g_screen == SCREEN_DEATH) ingame_tick();
            pex_profile_add(PROF_TICK_TOTAL, tick_start);
            tick_accum -= 1.0;
            ticks_this_frame++;
        }
        if (ticks_this_frame >= 2 && tick_accum > 1.0) tick_accum = 1.0;
        render((float)tick_accum);
        if ((++wii_frame_counter % 60u) == 0u) {
            wii_debug_logf("heartbeat frame=%u screen=%d tex=%u gui=%ux%u tris=%u draws=%u",
                           wii_frame_counter, g_screen, (unsigned)tex_terrain.id,
                           (unsigned)g_gui_w, (unsigned)g_gui_h,
                           (unsigned)g_wii_stats.triangles, (unsigned)g_wii_stats.draw_calls);
        }
        sleep_for_max_fps(frame_start_time);
        pex_profile_frame_end();
    }
    wii_debug_logf("main loop exit running=%d sys=%d", g_running, SYS_MainLoop());
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    wii_debug_init_console();
    wii_debug_stagef("main entered");
    wii_debug_memoryf("main entry");
    wii_reserve_embedded_asset_mem2();
    init_dirs();
#if defined(HW_RVL)
    SYS_SetPowerCallback(wii_power_callback);
#endif
    wii_debug_stagef("load options");
    load_options();
    g_opts.render_distance = 4;
    g_opts.fancy_graphics = 0;
    g_opts.max_fps = 60;
    g_opts.anaglyph = 0;
    g_opts.show_fps = 1;
    g_opts.ignore_classic_resources_warning = 1;
    wii_debug_stagef("embedded classic pack setup");
    wii_install_embedded_classic_pack_if_needed();
    g_opts.renderer_backend = RENDERER_OPENGL;
    g_runtime_renderer_backend = RENDERER_OPENGL;
    g_selected_renderer_backend = RENDERER_OPENGL;
    snprintf(g_multiplayer_username, sizeof(g_multiplayer_username), "%s", g_opts.username[0] ? g_opts.username : "Player");
    g_hwnd = (HWND)1;
    g_win_w = 640; g_win_h = 480; g_render_w = 640; g_render_h = 480; g_gui_w = 640; g_gui_h = 480; g_gui_scale = 1;
    setup_scale();
    InitializeCriticalSection(&g_save_cs);
    wii_debug_stagef("pex_renderer_backend_init");
    if (!pex_renderer_backend_init(g_hwnd)) {
        wii_debug_logf("FATAL: failed to initialize GX or assets; last_stage=%s", g_wii_debug_last_stage);
        wii_debug_wait_frames(240);
        return 1;
    }
    if (g_wii_render_w > 0 && g_wii_render_h > 0) {
        g_win_w = g_wii_render_w; g_win_h = g_wii_render_h; g_render_w = g_wii_render_w; g_render_h = g_wii_render_h;
        g_gui_w = g_wii_render_w; g_gui_h = g_wii_render_h; setup_scale();
    }
    wii_debug_stagef("set title screen");
    set_screen(SCREEN_TITLE);
    wii_debug_stagef("starting frames");
    main_loop();
    set_mouse_grabbed(0);
    save_world_state_for_exit();
    async_section_mesh_shutdown();
    free_texture(&tex_bg); free_texture(&tex_gui); free_texture(&tex_font); free_texture(&tex_terrain);
    free_texture(&tex_black); free_texture(&tex_pack); free_texture(&tex_default_pack_icon); free_texture(&tex_unknown_pack);
    free_texture(&tex_icons); free_texture(&tex_inventory); free_texture(&tex_items); free_texture(&tex_steve);
    free_texture(&tex_chest_entity); free_texture(&tex_large_chest_entity); free_texture_pack_icons();
    pex_gamepad_shutdown();
    pex_renderer_shutdown();
    pex_join_save_thread_for_exit();
    DeleteCriticalSection(&g_save_cs);
    return 0;
}
