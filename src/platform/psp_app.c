/* Sony PSP app/bootstrap/main loop. */

PSP_MODULE_INFO("PEXCRAFT", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);

#ifndef PEX_PSP_BOOT_DEBUG
#define PEX_PSP_BOOT_DEBUG 1
#endif

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

static void psp_boot_debug_stagef(const char *fmt, ...) {
    if (!fmt) return;
    va_list ap;
    va_start(ap, fmt);
    char line[96];
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    snprintf(g_psp_boot_debug_lines[g_psp_boot_debug_count % 18], 96, "%s", line);
    g_psp_boot_debug_count++;
    g_psp_boot_debug_stage++;
    psp_boot_debug_redraw("working...");
    sceKernelDelayThread(140000);
}

static void psp_boot_debug_pause(const char *footer, int usec) {
    psp_boot_debug_redraw(footer);
    if (usec > 0) sceKernelDelayThread((SceUInt)usec);
}

static void psp_boot_debug_fail(const char *msg) {
    psp_boot_debug_stagef("FAIL: %s", msg ? msg : "unknown");
    psp_boot_debug_pause("Boot stopped. Check this stage.", 8000000);
}
#else
static void psp_boot_debug_stagef(const char *fmt, ...) { (void)fmt; }
static void psp_boot_debug_pause(const char *footer, int usec) { (void)footer; (void)usec; }
static void psp_boot_debug_fail(const char *msg) { (void)msg; }
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
static void pex_renderer_present(void) { psp_gu_end_frame(); }
static void pex_renderer_resize(int w, int h) { (void)w; (void)h; }
static void pex_renderer_shutdown(void) { psp_gu_shutdown(); }
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
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    pex_join_save_thread_for_exit();
#else
    pex_join_save_thread_for_exit();
    save_current_world_state_sync();
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
    g_last_time = now_seconds();
    double tick_accum = 0.0;
    int psp_debug_first_frame = 1;
    while (g_running) {
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
        if (psp_debug_first_frame) {
            psp_debug_first_frame = 0;
            psp_boot_debug_stagef("first frame rendered");
            psp_boot_debug_pause("Entering game. If the screen goes black after this, rendering/menu code is the next suspect.", 900000);
        }
        sleep_for_max_fps(frame_start_time);
        pex_profile_frame_end();
    }
    psp_boot_debug_stagef("main loop exited: running=%d screen=%d ticks=%llu", g_running, g_screen, (unsigned long long)g_ticks);
    psp_boot_debug_pause("Game loop ended without a PSP exception.", 5000000);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    pspDebugScreenInit();
    psp_setup_callbacks();
    init_dirs();
    load_options();
    psp_install_embedded_classic_pack_if_needed();
    g_runtime_renderer_backend = RENDERER_OPENGL;
    g_selected_renderer_backend = RENDERER_OPENGL;
    snprintf(g_multiplayer_username, sizeof(g_multiplayer_username), "%s", g_opts.username[0] ? g_opts.username : "Player");
    g_hwnd = (HWND)1;
    g_win_w = 480; g_win_h = 272; g_render_w = 480; g_render_h = 272; g_gui_w = 480; g_gui_h = 272; g_gui_scale = 1;
    setup_scale();
    InitializeCriticalSection(&g_save_cs);
    if (!pex_renderer_backend_init(g_hwnd)) {
        pspDebugScreenPrintf("PEXCRAFT failed to initialize PSP GU or assets.\n");
        sceKernelDelayThread(3000000);
        sceKernelExitGame();
        return 1;
    }
    set_screen(SCREEN_TITLE);
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
    sceKernelExitGame();
    return 0;
}
