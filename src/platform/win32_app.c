/* Split from original monolithic main.c. Included by src/main.c unity build. */

typedef BOOL (WINAPI *PFNWGLSWAPINTERVALEXTPROC_LOCAL)(int interval);
static PFNWGLSWAPINTERVALEXTPROC_LOCAL g_wgl_swap_interval_ext = NULL;
static int g_wgl_swap_interval_loaded = 0;
static void save_world_state_for_exit(void);

static void enable_windows_dpi_awareness(void) {
    HMODULE user32 = GetModuleHandleA("user32.dll");
    if (!user32) return;
    typedef BOOL (WINAPI *PFN_SET_PROCESS_DPI_AWARE)(void);
    typedef BOOL (WINAPI *PFN_SET_PROCESS_DPI_AWARENESS_CONTEXT)(HANDLE);
    PFN_SET_PROCESS_DPI_AWARENESS_CONTEXT set_ctx =
        (PFN_SET_PROCESS_DPI_AWARENESS_CONTEXT)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
    if (set_ctx) {
        /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2, without requiring newer SDK headers. */
        if (set_ctx((HANDLE)-4)) return;
    }
    PFN_SET_PROCESS_DPI_AWARE set_dpi_aware =
        (PFN_SET_PROCESS_DPI_AWARE)GetProcAddress(user32, "SetProcessDPIAware");
    if (set_dpi_aware) set_dpi_aware();
}

static void apply_vsync_setting(void) {
    if (g_runtime_renderer_backend == RENDERER_D3D9) {
        if (g_d3d9.active) pex_renderer_d3d9_resize(g_d3d9.width, g_d3d9.height);
        return;
    }
    if (g_runtime_renderer_backend == RENDERER_D3D11) {
        return;
    }
    if (!g_wgl_swap_interval_loaded) {
        g_wgl_swap_interval_ext = (PFNWGLSWAPINTERVALEXTPROC_LOCAL)wglGetProcAddress("wglSwapIntervalEXT");
        g_wgl_swap_interval_loaded = 1;
    }
    if (g_wgl_swap_interval_ext) {
        int interval = (g_opts.anaglyph && g_opts.max_fps > 0) ? 1 : 0;
        g_wgl_swap_interval_ext(interval);
    }
}

static void refresh_window_size_after_mode_change(void) {
    if (!g_hwnd) return;
    RECT rc;
    GetClientRect(g_hwnd, &rc);
    g_win_w = rc.right - rc.left;
    g_win_h = rc.bottom - rc.top;
    if (g_win_w < 1) g_win_w = 1;
    if (g_win_h < 1) g_win_h = 1;
    g_render_w = g_win_w;
    g_render_h = g_win_h;
    setup_scale();
    rebuild_screen();
    if (g_screen == SCREEN_INGAME) set_mouse_grabbed(1);
}

static void set_fullscreen_enabled(int enabled) {
    enabled = enabled ? 1 : 0;
    g_opts.fullscreen = enabled;

    if (!g_hwnd) {
        save_options();
        return;
    }
    if (enabled == g_fullscreen_active) {
        save_options();
        return;
    }

    set_mouse_grabbed(0);

    if (enabled) {
        MONITORINFO mi;
        memset(&mi, 0, sizeof(mi));
        mi.cbSize = sizeof(mi);
        if (!GetWindowRect(g_hwnd, &g_windowed_rect)) {
            g_windowed_rect.left = 0;
            g_windowed_rect.top = 0;
            g_windowed_rect.right = 854;
            g_windowed_rect.bottom = 480;
        }
        g_windowed_style = (DWORD)GetWindowLongA(g_hwnd, GWL_STYLE);
        g_windowed_exstyle = (DWORD)GetWindowLongA(g_hwnd, GWL_EXSTYLE);

        if (GetMonitorInfoA(MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTONEAREST), &mi)) {
            DWORD style = g_windowed_style;
            DWORD exstyle = g_windowed_exstyle;
            style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
            style |= WS_POPUP;
            exstyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
            SetWindowLongA(g_hwnd, GWL_STYLE, (LONG)style);
            SetWindowLongA(g_hwnd, GWL_EXSTYLE, (LONG)exstyle);
            SetWindowPos(g_hwnd, HWND_TOP,
                         mi.rcMonitor.left, mi.rcMonitor.top,
                         mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            g_fullscreen_active = 1;
        } else {
            g_opts.fullscreen = 0;
        }
    } else {
        SetWindowLongA(g_hwnd, GWL_STYLE, (LONG)g_windowed_style);
        SetWindowLongA(g_hwnd, GWL_EXSTYLE, (LONG)g_windowed_exstyle);
        SetWindowPos(g_hwnd, NULL,
                     g_windowed_rect.left, g_windowed_rect.top,
                     g_windowed_rect.right - g_windowed_rect.left,
                     g_windowed_rect.bottom - g_windowed_rect.top,
                     SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        g_fullscreen_active = 0;
    }

    refresh_window_size_after_mode_change();
    save_options();
}

static void toggle_fullscreen(void) {
    set_fullscreen_enabled(!g_opts.fullscreen);
}

static int init_gl(HWND hwnd) {
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;
    g_hdc = GetDC(hwnd);
    int pf = ChoosePixelFormat(g_hdc, &pfd);
    if (!pf) return 0;
    if (!SetPixelFormat(g_hdc, pf, &pfd)) return 0;
    g_glrc = wglCreateContext(g_hdc);
    if (!g_glrc) return 0;
    if (!wglMakeCurrent(g_hdc, g_glrc)) return 0;
    glClearColor(0,0,0,0);
    glDisable(GL_DITHER);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    scan_texture_packs();
    if (!load_default_textures()) return 0;
    if (g_selected_texpack > 0) apply_texture_pack_index(g_selected_texpack);
    else { if (g_opts.skin_path[0]) load_custom_skin_path(g_opts.skin_path, 0); init_font_widths(); }
    apply_vsync_setting();
    return 1;
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_SIZE: {
            g_win_w = LOWORD(lparam);
            g_win_h = HIWORD(lparam);
            if (g_win_w < 1) g_win_w = 1;
            if (g_win_h < 1) g_win_h = 1;
            g_render_w = g_win_w;
            g_render_h = g_win_h;
            setup_scale();
            pex_renderer_resize(g_win_w, g_win_h);
            if (g_mouse_grabbed) set_mouse_grabbed(1);
            rebuild_screen();
            return 0;
        }
        case WM_ACTIVATE:
            if (LOWORD(wparam) == WA_INACTIVE) set_mouse_grabbed(0);
            else if (g_screen == SCREEN_INGAME) set_mouse_grabbed(1);
            return 0;
        case WM_MOUSEMOVE: {
            int px = (short)LOWORD(lparam);
            int py = (short)HIWORD(lparam);
            if (g_mouse_grabbed && g_screen == SCREEN_INGAME) {
                handle_grabbed_mouse_move(px, py);
                g_mouse_x = g_gui_w / 2;
                g_mouse_y = g_gui_h / 2;
                return 0;
            }
            g_mouse_x = px * g_gui_w / (g_win_w ? g_win_w : 1);
            g_mouse_y = py * g_gui_h / (g_win_h ? g_win_h : 1);
            if (g_mouse_down && g_drag_slider) update_slider(g_drag_slider, g_mouse_x);
            if (g_mouse_down && g_screen == SCREEN_TEXPACK) texpack_mouse_drag(g_mouse_y);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            SetCapture(hwnd);
            int px = (short)LOWORD(lparam);
            int py = (short)HIWORD(lparam);
            g_mouse_x = px * g_gui_w / (g_win_w ? g_win_w : 1);
            g_mouse_y = py * g_gui_h / (g_win_h ? g_win_h : 1);
            mouse_down(g_mouse_x, g_mouse_y);
            return 0;
        }
        case WM_LBUTTONUP: {
            ReleaseCapture();
            int px = (short)LOWORD(lparam);
            int py = (short)HIWORD(lparam);
            g_mouse_x = px * g_gui_w / (g_win_w ? g_win_w : 1);
            g_mouse_y = py * g_gui_h / (g_win_h ? g_win_h : 1);
            mouse_up(g_mouse_x, g_mouse_y);
            return 0;
        }
        case WM_RBUTTONDOWN: {
            SetCapture(hwnd);
            int px = (short)LOWORD(lparam);
            int py = (short)HIWORD(lparam);
            g_mouse_x = px * g_gui_w / (g_win_w ? g_win_w : 1);
            g_mouse_y = py * g_gui_h / (g_win_h ? g_win_h : 1);
            mouse_right_down(g_mouse_x, g_mouse_y);
            return 0;
        }
        case WM_RBUTTONUP:
            ReleaseCapture();
            return 0;
        case WM_MOUSEWHEEL:
            if (g_screen == SCREEN_INGAME) {
                int delta = GET_WHEEL_DELTA_WPARAM(wparam);
                if (delta > 0) g_selected_hotbar_slot = (g_selected_hotbar_slot + 8) % 9;
                else if (delta < 0) g_selected_hotbar_slot = (g_selected_hotbar_slot + 1) % 9;
            }
            return 0;
        case WM_KEYDOWN:
            if (wparam != VK_F11 || ((lparam & (1L << 30)) == 0)) handle_keydown(wparam);
            return 0;
        case WM_CHAR:
            handle_char(wparam);
            return 0;
        case WM_CLOSE:
            save_world_state_for_exit();
            set_mouse_grabbed(0);
            PostQuitMessage(0);
            return 0;
        case WM_DESTROY:
            save_world_state_for_exit();
            set_mouse_grabbed(0);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}


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
    if (g_save_event != NULL) {
        CloseHandle(g_save_event);
        g_save_event = NULL;
    }
    while (g_save_queue_head) {
        PexSaveSnapshot *ss = g_save_queue_head;
        g_save_queue_head = ss->next;
        pex_save_snapshot_free(ss);
    }
    g_save_queue_tail = NULL;
    g_save_thread_shutdown = 0;
}

static void save_world_state_for_exit(void) {
    pex_join_save_thread_for_exit();
    if (g_mp_connected) {
        pex_net_disconnect();
        return;
    }
    save_current_world_state_sync();
}

static int g_timer_resolution_active = 0;

static void begin_high_res_timer(void) {
    if (!g_timer_resolution_active && timeBeginPeriod(1) == TIMERR_NOERROR) {
        g_timer_resolution_active = 1;
    }
}

static void end_high_res_timer(void) {
    if (g_timer_resolution_active) {
        timeEndPeriod(1);
        g_timer_resolution_active = 0;
    }
}

static void sleep_for_max_fps(double frame_start_time) {
    double sleep_start_time = now_seconds();
    if (g_opts.max_fps <= 0) {
        /* Unlimited must mean uncapped.  Sleep(1) often becomes a ~15.6 ms
           scheduler wait on Windows, which feels exactly like a 60 FPS/vsync
           cap even when swap interval is disabled. */
        g_sleep_ms_last = (now_seconds() - sleep_start_time) * 1000.0;
        return;
    }
    double target = 1.0 / (double)g_opts.max_fps;
    for (;;) {
        double elapsed = now_seconds() - frame_start_time;
        double remaining = target - elapsed;
        if (remaining <= 0.0) break;
        if (remaining > 0.003) {
            DWORD ms = (DWORD)(remaining * 1000.0) - 1;
            Sleep(ms > 0 ? ms : 0);
        } else {
            Sleep(0);
        }
    }
    g_sleep_ms_last = (now_seconds() - sleep_start_time) * 1000.0;
}

static void main_loop(void) {
    MSG msg;
    g_last_time = now_seconds();
    double tick_accum = 0.0;
    while (g_running) {
        pex_profile_frame_begin();
        double prof_start = pex_profile_begin();
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { g_running = 0; break; }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        pex_profile_add(PROF_PUMP, prof_start);
        pex_gamepad_update();
        if (g_mp_connected || pex_net_is_connecting()) {
            prof_start = pex_profile_begin();
            pex_net_poll();
            pex_profile_add(PROF_NET_POLL, prof_start);
        }
        double frame_start_time = now_seconds();
        double t = frame_start_time;
        double dt = t - g_last_time;
        if (dt < 0) dt = 0;
        if (dt > 0.25) dt = 0.25;
        g_last_time = t;
        tick_accum += dt * 20.0;
        int ticks_this_frame = 0;
        while (tick_accum >= 1.0 && ticks_this_frame < 3) {
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
                g_screen == SCREEN_DEATH || (g_mp_connected && g_screen == SCREEN_PAUSE)) ingame_tick();
            pex_profile_add(PROF_TICK_TOTAL, tick_start);
            tick_accum -= 1.0;
            ticks_this_frame++;
        }
        if (ticks_this_frame >= 3 && tick_accum > 1.0) tick_accum = 1.0;
        float partial = (float)tick_accum;
        if (g_mp_connected) pex_net_update_smoothing();
        render(partial);
        sleep_for_max_fps(frame_start_time);
        pex_profile_frame_end();
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrev; (void)lpCmdLine;
    enable_windows_dpi_awareness();
    g_inst = hInstance;
    begin_high_res_timer();
    init_dirs();
    load_options();
    g_runtime_renderer_backend = g_opts.renderer_backend;
    g_selected_renderer_backend = g_opts.renderer_backend;
    if (!renderer_backend_supported(g_runtime_renderer_backend)) {
        g_renderer_backend_unavailable_notice = 1;
        g_runtime_renderer_backend = RENDERER_OPENGL;
    }
    snprintf(g_multiplayer_ip, sizeof(g_multiplayer_ip), "%s", g_opts.last_server);
    snprintf(g_multiplayer_username, sizeof(g_multiplayer_username), "%s", g_opts.username[0] ? g_opts.username : "Player");
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = wndproc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "PEXCRAFTBeta10Client";
    if (!RegisterClassA(&wc)) { end_high_res_timer(); return 1; }
    int initial_w = 1280, initial_h = 720;
    RECT work_area;
    memset(&work_area, 0, sizeof(work_area));
    if (SystemParametersInfoA(SPI_GETWORKAREA, 0, &work_area, 0)) {
        int work_w = work_area.right - work_area.left;
        int work_h = work_area.bottom - work_area.top;
        if (work_w > 0 && work_h > 0) {
            /* Use the largest 16:9 client area that fits the real DPI-aware desktop.
               The old default stayed around 720p on 1080p displays, making the
               Windows build look smeared even when the monitor was native 1080p. */
            int target_w = work_w;
            int target_h = (target_w * 9) / 16;
            if (target_h > work_h) { target_h = work_h; target_w = (target_h * 16) / 9; }
            if (target_w > initial_w) { initial_w = target_w; initial_h = target_h; }
        }
    }
    RECT rc = {0, 0, initial_w, initial_h};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hwnd = CreateWindowA(wc.lpszClassName, APP_TITLE, WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
                           NULL, NULL, hInstance, NULL);
    if (!g_hwnd) { end_high_res_timer(); return 2; }
    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);
    GetClientRect(g_hwnd, &rc);
    g_win_w = rc.right - rc.left;
    g_win_h = rc.bottom - rc.top;
    g_render_w = g_win_w;
    g_render_h = g_win_h;
    setup_scale();
    if (!pex_renderer_backend_init(g_hwnd)) {
        MessageBoxA(g_hwnd, "PEXCRAFT was unable to start because it failed to initialize the selected renderer or load assets.", APP_TITLE, MB_ICONERROR);
        end_high_res_timer();
        return 3;
    }
    if (g_opts.fullscreen) set_fullscreen_enabled(1);
    if (should_show_classic_pack_download_prompt()) set_screen(SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT);
    else if (!strcmp(g_opts.skin, CLASSIC_PACK_NAME) && classic_resources_need_update()) set_screen(SCREEN_CLASSIC_PACK_WARNING);
    else if (g_renderer_backend_unavailable_notice) {
        char line[MAX_LABEL];
        snprintf(line, sizeof(line), "%s is unavailable on this system.", renderer_backend_label(g_opts.renderer_backend));
        open_notice("Renderer unavailable", line, "Falling back to OpenGL for this session.");
    }
    else set_screen(SCREEN_TITLE);
    InitializeCriticalSection(&g_save_cs);
    main_loop();
    set_mouse_grabbed(0);
    async_section_mesh_shutdown();
    free_texture(&tex_bg);
    free_texture(&tex_gui);
    free_texture(&tex_font);
    free_texture(&tex_terrain);
    free_texture(&tex_black);
    free_texture(&tex_pack);
    free_texture(&tex_default_pack_icon);
    free_texture(&tex_unknown_pack);
    free_texture(&tex_icons);
    free_texture(&tex_inventory);
    free_texture(&tex_items);
    free_texture(&tex_steve);
    free_texture(&tex_chest_entity);
    free_texture(&tex_large_chest_entity);
    free_texture_pack_icons();
    pex_gamepad_shutdown();
    pex_renderer_shutdown();
    if (g_wic_factory) { IWICImagingFactory_Release(g_wic_factory); g_wic_factory = NULL; }
    CoUninitialize();
    if (g_glrc) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(g_glrc);
    }
    if (g_hdc) ReleaseDC(g_hwnd, g_hdc);
    pex_join_save_thread_for_exit();
    DeleteCriticalSection(&g_save_cs);
    end_high_res_timer();
    return 0;
}
