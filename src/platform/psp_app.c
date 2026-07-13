/* Sony PSP app/bootstrap/main loop. */

/* Leave a bigger system/stack reserve on real PSP-1000.  The previous all-but-1MB heap
   could let the client push too close to the 32MB model and hard-power-off on OOM. */
PSP_HEAP_SIZE_KB(-4096);
/* Terrain/world generation has large helper structs; 512KB avoids silent PSP-1000 stack overflow. */
PSP_MAIN_THREAD_STACK_SIZE_KB(512);
PSP_MODULE_INFO("PEXCRAFT", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#ifndef PEX_PSP_BOOT_DEBUG
#define PEX_PSP_BOOT_DEBUG 1
#endif

static int g_psp_gu_debug_screen_ready = 0;
static unsigned int g_psp_runtime_overlay_frame = 0;

#if PEX_PSP_BOOT_DEBUG
static char g_psp_boot_debug_lines[18][96];
static int g_psp_boot_debug_count = 0;
static int g_psp_boot_debug_stage = 0;

static void psp_boot_debug_redraw(const char *footer) {
    pspDebugScreenSetBackColor(0x00000000);
    pspDebugScreenSetTextColor(0xffffffff);
    pspDebugScreenClear();
    pspDebugScreenSetXY(0, 0);
    pspDebugScreenPrintf("PEXCRAFT PSP BOOT DEBUG\n");
    pspDebugScreenPrintf("-----------------------\n");
    pspDebugScreenPrintf("Stage: %d\n", g_psp_boot_debug_stage);
    pspDebugScreenPrintf("RAM-only: yes   Network: disabled\n");
    pspDebugScreenPrintf("Screen: 480x272  Renderer: PSP GU\n\n");
    int first = g_psp_boot_debug_count > 14 ? g_psp_boot_debug_count - 14 : 0;
    for (int i = first; i < g_psp_boot_debug_count; ++i) {
        pspDebugScreenPrintf("%02d: %s\n", i + 1, g_psp_boot_debug_lines[i % 18]);
    }
    if (footer && footer[0]) {
        pspDebugScreenPrintf("\n%s\n", footer);
    }
}


static void psp_redraw_gu_front(const char *footer) {
    unsigned int off = psp_gu_display_offset();
    void *base = (void *)(0x44000000u + off);
    pspDebugScreenInitEx(base, PSP_DISPLAY_PIXEL_FORMAT_8888, 0);
    psp_boot_debug_redraw(footer);
}

static void psp_show_runtime_overlay(const char *phase) {
    g_psp_runtime_overlay_frame++;
    char footer[192];
    snprintf(footer, sizeof(footer),
             "LIVE PSP DEBUG OVERLAY\nphase=%s frame=%u screen=%d ticks=%llu display_off=0x%06x\nIf this stays visible, the app is alive but game drawing is not visible.",
             phase ? phase : "?",
             g_psp_runtime_overlay_frame,
             g_screen,
             (unsigned long long)g_ticks,
             psp_gu_display_offset());
    psp_redraw_gu_front(footer);
}

static void psp_boot_debug_stagef(const char *fmt, ...) {
    if (!fmt) return;
    va_list ap;
    va_start(ap, fmt);
    char line[96];
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    PEX_PSP_LOGF("BOOT STAGE %03d: %s", g_psp_boot_debug_stage + 1, line);
    snprintf(g_psp_boot_debug_lines[g_psp_boot_debug_count % 18], 96, "%s", line);
    g_psp_boot_debug_count++;
    g_psp_boot_debug_stage++;
    if (g_psp_gu_debug_screen_ready) psp_redraw_gu_front("working... after GU takeover");
    else psp_boot_debug_redraw("working...");
    sceKernelDelayThread(g_psp_gu_debug_screen_ready ? 900000 : 350000);
}

static void psp_boot_debug_pause(const char *footer, int usec) {
    if (g_psp_gu_debug_screen_ready) psp_redraw_gu_front(footer);
    else psp_boot_debug_redraw(footer);
    if (usec > 0) sceKernelDelayThread((SceUInt)usec);
}

static void psp_boot_debug_hold(const char *footer) {
    PEX_PSP_LOGF("DEBUG HOLD: %s", footer ? footer : "PSP debug hold");
    char line[160];
    snprintf(line, sizeof(line), "%s\n\nDebug hold: START+SELECT+L+R exits.", footer ? footer : "PSP debug hold");
    for (;;) {
        if (g_psp_gu_debug_screen_ready) psp_redraw_gu_front(line);
        else psp_boot_debug_redraw(line);
        SceCtrlData pad;
        memset(&pad, 0, sizeof(pad));
        sceCtrlPeekBufferPositive(&pad, 1);
        if ((pad.Buttons & PSP_CTRL_START) &&
            (pad.Buttons & PSP_CTRL_SELECT) &&
            (pad.Buttons & PSP_CTRL_LTRIGGER) &&
            (pad.Buttons & PSP_CTRL_RTRIGGER)) {
            sceKernelExitGame();
        }
        sceKernelDelayThread(250000);
    }
}

static void psp_boot_debug_fail(const char *msg) {
    psp_boot_debug_stagef("FAIL: %s", msg ? msg : "unknown");
    psp_boot_debug_hold("Boot stopped. Check this stage.");
}
#else
static void psp_redraw_gu_front(const char *footer) { (void)footer; }
static void psp_show_runtime_overlay(const char *phase) { (void)phase; }
static void psp_boot_debug_stagef(const char *fmt, ...) { (void)fmt; }
static void psp_boot_debug_pause(const char *footer, int usec) { (void)footer; (void)usec; }
static void psp_boot_debug_hold(const char *footer) {
    (void)footer;
    for (;;) {
        SceCtrlData pad;
        memset(&pad, 0, sizeof(pad));
        sceCtrlPeekBufferPositive(&pad, 1);
        if ((pad.Buttons & PSP_CTRL_START) && (pad.Buttons & PSP_CTRL_SELECT) &&
            (pad.Buttons & PSP_CTRL_LTRIGGER) && (pad.Buttons & PSP_CTRL_RTRIGGER)) {
            sceKernelExitGame();
        }
        sceKernelDelayThread(250000);
    }
}
static void psp_boot_debug_fail(const char *msg) { (void)msg; psp_boot_debug_hold("Boot stopped."); }
#endif


static int psp_exit_callback(int arg1, int arg2, void *common) { (void)arg1; (void)arg2; (void)common; g_running = 0; return 0; }
static int psp_callback_thread(SceSize args, void *argp) { (void)args; (void)argp; int cbid = sceKernelCreateCallback("Exit Callback", psp_exit_callback, NULL); sceKernelRegisterExitCallback(cbid); sceKernelSleepThreadCB(); return 0; }
static void psp_setup_callbacks(void) { SceUID thid = sceKernelCreateThread("update_thread", psp_callback_thread, 0x11, 0xFA0, 0, 0); if (thid >= 0) sceKernelStartThread(thid, 0, 0); }

static int init_gl(HWND unused) {
    (void)unused;
    psp_boot_debug_stagef("clock/input setup");
    scePowerSetClockFrequency(333, 333, 166);
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    psp_boot_debug_stagef("initializing PSP GU");
    if (!psp_gu_init()) { psp_boot_debug_fail("psp_gu_init failed"); return 0; }
    g_psp_gu_debug_screen_ready = 1;
    psp_redraw_gu_front("GU returned OK. Front-buffer debug console is active.");
#if PEX_PSP_BOOT_DEBUG
    sceKernelDelayThread(1200000);
#endif
    psp_boot_debug_stagef("GU initialized");
    glClearColor(0,0,0,1);
    glDisable(GL_DITHER);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    psp_boot_debug_stagef("scanning embedded textures");
    scan_texture_packs();
    psp_boot_debug_stagef("loading default textures");
    if (!load_default_textures()) { psp_boot_debug_fail("default textures failed"); return 0; }
    psp_boot_debug_stagef("default textures loaded");
    if (g_selected_texpack > 0) apply_texture_pack_index(g_selected_texpack);
    else { if (g_opts.skin_path[0]) load_custom_skin_path(g_opts.skin_path, 0); init_font_widths(); }
    psp_boot_debug_stagef("font widths initialized");
    return 1;
}

static int pex_renderer_backend_init(HWND unused) {
    g_runtime_renderer_backend = RENDERER_OPENGL;
    g_selected_renderer_backend = RENDERER_OPENGL;
    return init_gl(unused);
}
static int pex_renderer_begin_frame(void) { psp_gu_begin_frame(); return 1; }
static void pex_renderer_present(void) {
    psp_gu_end_frame();
#if PEX_PSP_VERBOSE
    if (g_psp_runtime_overlay_frame < 360 || (g_psp_runtime_overlay_frame % 60u) == 0u) {
        psp_show_runtime_overlay("after-present");
    }
#endif
}
static void pex_renderer_resize(int w, int h) { (void)w; (void)h; }
static void pex_renderer_shutdown(void) { psp_gu_shutdown(); }
static void pex_gl_suppress_immediate(int on) { (void)on; }
static void apply_vsync_setting(void) { }
static void refresh_window_size_after_mode(void) { }
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
    pex_stats_flush();
    if (world_quit_is_active()) return;
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    pex_join_save_thread_for_exit();
#else
    pex_join_save_thread_for_exit();
    save_world_state_sync();
#endif
}

static void sleep_for_max_fps(double frame_start_time) {
    double sleep_start_time = now_seconds();
    int fps = g_opts.max_fps > 0 ? g_opts.max_fps : 60;
    double target = 1.0 / (double)fps;
    for (;;) {
        double elapsed = now_seconds() - frame_start_time;
        double remaining = target - elapsed;
        if (remaining <= 0.0) break;
        sceKernelDelayThread((SceUInt)(remaining * 1000000.0));
    }
    g_sleep_ms_last = (now_seconds() - sleep_start_time) * 1000.0;
}

static void main_loop(void) {
    psp_boot_debug_stagef("main loop entered");
    PEX_PSP_LOGF("MAIN_LOOP enter: screen=%d running=%d", g_screen, g_running);
    g_last_time = now_seconds();
    double tick_accum = 0.0;
    int psp_debug_first_frame = 1;
    unsigned int psp_verbose_frame = 0;
    while (g_running) {
        psp_verbose_frame++;
        if (psp_verbose_frame <= 120 || (psp_verbose_frame % 60u) == 0u) {
            PEX_PSP_LOGF("FRAME %u begin: screen=%d ticks=%llu", psp_verbose_frame, g_screen, (unsigned long long)g_ticks);
        }
        profile_begin_frame();
        double prof_start = profile_begin();
        if (psp_verbose_frame <= 20) PEX_PSP_LOGF("FRAME %u: pex_gamepad_update begin", psp_verbose_frame);
        pex_gamepad_update();
        if (psp_verbose_frame <= 20) PEX_PSP_LOGF("FRAME %u: pex_gamepad_update end", psp_verbose_frame);
        profile_add_time(PROF_PUMP, prof_start);

        double frame_start_time = now_seconds();
        double t = frame_start_time;
        double dt = t - g_last_time;
        if (dt < 0) dt = 0;
        if (dt > 0.25) dt = 0.25;
        g_last_time = t;
        tick_accum += dt * 20.0;
        int ticks_this_frame = 0;
        while (tick_accum >= 1.0 && ticks_this_frame < 2) {
            if (psp_verbose_frame <= 60) PEX_PSP_LOGF("FRAME %u: tick begin screen=%d", psp_verbose_frame, g_screen);
            double tick_start = profile_begin();
            g_ticks++;
            if (g_screen == SCREEN_TITLE) tick_title_blocks();
            if (g_screen == SCREEN_GENERATING) {
                double t_worldgen = profile_begin();
                PEX_PSP_LOGF("FRAME %u: worldgen_tick begin", psp_verbose_frame);
                worldgen_tick();
                PEX_PSP_LOGF("FRAME %u: worldgen_tick end", psp_verbose_frame);
                profile_add_time(PROF_WORLDGEN_TICK, t_worldgen);
            }
            if (g_screen == SCREEN_TEXPACK_INSTALL) pack_install_tick();
            if (g_screen == SCREEN_INGAME || g_screen == SCREEN_CHAT ||
                g_screen == SCREEN_INVENTORY || g_screen == SCREEN_CREATIVE || g_screen == SCREEN_WORKBENCH ||
                g_screen == SCREEN_FURNACE || g_screen == SCREEN_CHEST ||
                g_screen == SCREEN_DEATH) ingame_tick();
            profile_add_time(PROF_TICK_TOTAL, tick_start);
            tick_accum -= 1.0;
            ticks_this_frame++;
            if (psp_verbose_frame <= 60) PEX_PSP_LOGF("FRAME %u: tick end ticks=%llu", psp_verbose_frame, (unsigned long long)g_ticks);
        }
        if (ticks_this_frame >= 2 && tick_accum > 1.0) tick_accum = 1.0;
        if (psp_verbose_frame <= 120 || (psp_verbose_frame % 60u) == 0u) PEX_PSP_LOGF("FRAME %u: render begin alpha=%.3f screen=%d", psp_verbose_frame, (double)tick_accum, g_screen);
        render((float)tick_accum);
        if (psp_verbose_frame <= 120 || (psp_verbose_frame % 60u) == 0u) PEX_PSP_LOGF("FRAME %u: render end", psp_verbose_frame);
        if (psp_debug_first_frame) {
            psp_debug_first_frame = 0;
            psp_boot_debug_stagef("first frame rendered");
#if PEX_PSP_BOOT_DEBUG
            psp_boot_debug_pause("First frame rendered. Entering game in 5 seconds. If it exits after this, the main loop ended cleanly.", 5000000);
#endif
        }
        sleep_for_max_fps(frame_start_time);
        profile_end_frame();
        if (psp_verbose_frame <= 120 || (psp_verbose_frame % 60u) == 0u) PEX_PSP_LOGF("FRAME %u end: running=%d screen=%d", psp_verbose_frame, g_running, g_screen);
    }
    PEX_PSP_LOGF("MAIN_LOOP exit: running=%d screen=%d ticks=%llu", g_running, g_screen, (unsigned long long)g_ticks);
    psp_boot_debug_stagef("main loop exited: running=%d screen=%d ticks=%llu", g_running, g_screen, (unsigned long long)g_ticks);
    psp_boot_debug_hold("Game loop ended without a PSP exception.");
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    pex_log_init();
    pex_install_crash_handlers();
    pex_logf("app main enter: %s", APP_TITLE);
    PEX_PSP_LOGF("main() entered");
    pspDebugScreenInit();
    PEX_PSP_LOGF("pspDebugScreenInit done");
    psp_boot_debug_stagef("module loaded / main entered");
    PEX_PSP_LOGF("psp_setup_callbacks begin");
    psp_setup_callbacks();
    PEX_PSP_LOGF("psp_setup_callbacks end");
    psp_boot_debug_stagef("exit callback registered");
    PEX_PSP_LOGF("init_dirs begin");
    init_dirs();
    PEX_PSP_LOGF("init_dirs end: mc=%s save=%s texpack=%s skin=%s", g_mc_dir, g_save_dir, g_texpack_dir, g_skin_dir);
    psp_boot_debug_stagef("memory-only dirs initialized");
    PEX_PSP_LOGF("load_options begin");
    load_options();
    /* PSP stability defaults: small render distance, no fancy translucent pass,
       fixed 60 FPS, no filesystem/options persistence. */
    /* PSP gameplay default: 60 FPS gives PPSSPP/real PSP smoother pacing;
       terrain workers are now low-priority async threads, so they should use
       leftover time instead of blocking user_main. */
    g_opts.render_distance = 4;
    g_opts.fancy_graphics = 0;
    g_opts.max_fps = 60;
    g_opts.anaglyph = 0;
    g_opts.show_fps = 1;
    g_opts.renderer_backend = RENDERER_OPENGL;
    g_selected_renderer_backend = RENDERER_OPENGL;
    PEX_PSP_LOGF("load_options end/forced PSP: user=%s fps=%d renderer=%d rd=%d fancy=%d", g_opts.username, g_opts.max_fps, g_selected_renderer_backend, g_opts.render_distance, g_opts.fancy_graphics);
    psp_boot_debug_stagef("RAM options loaded; PSP-safe graphics forced");
    PEX_PSP_LOGF("embedded classic pack prepare begin");
    psp_install_embedded_pack_if_needed();
    PEX_PSP_LOGF("embedded classic pack prepare end");
    psp_boot_debug_stagef("embedded classic pack ready");
    g_runtime_renderer_backend = RENDERER_OPENGL;
    g_selected_renderer_backend = RENDERER_OPENGL;
    snprintf(g_multiplayer_username, sizeof(g_multiplayer_username), "%s", g_opts.username[0] ? g_opts.username : "Player");
    PEX_PSP_LOGF("username set: %s", g_multiplayer_username);
    g_hwnd = (HWND)1;
    g_win_w = 480; g_win_h = 272; g_render_w = 480; g_render_h = 272; g_gui_w = 480; g_gui_h = 272; g_gui_scale = 1;
    PEX_PSP_LOGF("setup_scale begin");
    setup_scale();
    PEX_PSP_LOGF("setup_scale end: gui=%dx%d scale=%d", g_gui_w, g_gui_h, g_gui_scale);
    psp_boot_debug_stagef("screen scale set");
    PEX_PSP_LOGF("InitializeCriticalSection begin");
    InitializeCriticalSection(&g_save_cs);
    PEX_PSP_LOGF("InitializeCriticalSection end");
    psp_boot_debug_stagef("save lock initialized");
    PEX_PSP_LOGF("pex_renderer_backend_init begin");
    if (!pex_renderer_backend_init(g_hwnd)) {
        PEX_PSP_LOGF("pex_renderer_backend_init FAILED");
        psp_boot_debug_hold("PEXCRAFT failed to initialize PSP GU or assets.");
        return 1;
    }
    PEX_PSP_LOGF("pex_renderer_backend_init end OK");
    psp_boot_debug_stagef("renderer/backend initialized");
    PEX_PSP_LOGF("set_screen(SCREEN_TITLE) begin");
    set_screen(pex_startup_screen());
    PEX_PSP_LOGF("set_screen(SCREEN_TITLE) end: screen=%d", g_screen);
    psp_boot_debug_stagef("title screen selected");
    psp_redraw_gu_front("Reached title screen setup. Calling main_loop next.");
#if PEX_PSP_BOOT_DEBUG
    sceKernelDelayThread(1500000);
#endif
    PEX_PSP_LOGF("calling main_loop");
    main_loop();
    PEX_PSP_LOGF("main_loop returned; cleanup begin");
    set_mouse_grabbed(0);
    world_quit_shutdown_for_app_exit();
    PEX_PSP_LOGF("save_world_state_for_exit begin");
    save_world_state_for_exit();
    PEX_PSP_LOGF("save_world_state_for_exit end");
    PEX_PSP_LOGF("async_section_mesh_shutdown begin");
    async_section_mesh_shutdown();
    passive_render_worker_shutdown();
    pex_mp_cache_save_shutdown();
    PEX_PSP_LOGF("async_section_mesh_shutdown end");
    PEX_PSP_LOGF("free textures begin");
    free_texture(&tex_bg); free_texture(&tex_gui); free_texture(&tex_font); free_texture(&tex_terrain);
    free_texture(&tex_black); free_texture(&tex_pack); free_texture(&tex_default_pack_icon); free_texture(&tex_unknown_pack);
    free_texture(&tex_icons); free_texture(&tex_xinput); free_texture(&tex_inventory); free_texture(&tex_items); free_texture(&tex_steve); free_texture(&tex_mob_pig); free_texture(&tex_mob_sheep); free_texture(&tex_mob_sheep_fur); free_texture(&tex_mob_cow); free_texture(&tex_mob_chicken); free_texture(&tex_mob_saddle);
    free_texture(&tex_chest_entity); free_texture(&tex_large_chest_entity); free_texture_pack_icons();
    PEX_PSP_LOGF("free textures end");
    pex_gamepad_shutdown();
    PEX_PSP_LOGF("pex_renderer_shutdown begin");
    pex_renderer_shutdown();
    PEX_PSP_LOGF("pex_renderer_shutdown end");
    pex_join_save_thread_for_exit();
    DeleteCriticalSection(&g_save_cs);
    PEX_PSP_LOGF("cleanup finished; entering hold");
    psp_boot_debug_hold("Cleanup finished. The game would normally call sceKernelExitGame now.");
    pex_logf("app main exit");
    pex_log_shutdown();
    return 0;
}
