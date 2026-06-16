/* Split from original monolithic main.c. Included by src/main.c unity build. */

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
    else init_font_widths();
    return 1;
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_SIZE: {
            g_win_w = LOWORD(lparam);
            g_win_h = HIWORD(lparam);
            if (g_win_w < 1) g_win_w = 1;
            if (g_win_h < 1) g_win_h = 1;
            setup_scale();
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
            if (g_screen == SCREEN_INGAME || g_screen == SCREEN_CHAT) {
                int delta = GET_WHEEL_DELTA_WPARAM(wparam);
                if (delta > 0) g_selected_hotbar_slot = (g_selected_hotbar_slot + 8) % 9;
                else if (delta < 0) g_selected_hotbar_slot = (g_selected_hotbar_slot + 1) % 9;
            }
            return 0;
        case WM_KEYDOWN:
            handle_keydown(wparam);
            return 0;
        case WM_CHAR:
            handle_char(wparam);
            return 0;
        case WM_CLOSE:
            set_mouse_grabbed(0);
            PostQuitMessage(0);
            return 0;
        case WM_DESTROY:
            set_mouse_grabbed(0);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void main_loop(void) {
    MSG msg;
    g_last_time = now_seconds();
    double tick_accum = 0.0;
    while (g_running) {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { g_running = 0; break; }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        double t = now_seconds();
        double dt = t - g_last_time;
        if (dt < 0) dt = 0;
        if (dt > 0.25) dt = 0.25;
        g_last_time = t;
        tick_accum += dt * 20.0;
        while (tick_accum >= 1.0) {
            g_ticks++;
            if (g_screen == SCREEN_TITLE) tick_title_blocks();
            if (g_screen == SCREEN_GENERATING) worldgen_tick();
            if (g_screen == SCREEN_INGAME || g_screen == SCREEN_CHAT) ingame_tick();
            tick_accum -= 1.0;
        }
        float partial = (float)tick_accum;
        render(partial);
        Sleep(g_opts.limit_framerate ? 16 : 1);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrev; (void)lpCmdLine;
    g_inst = hInstance;
    init_dirs();
    load_options();
    snprintf(g_multiplayer_ip, sizeof(g_multiplayer_ip), "%s", g_opts.last_server);
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = wndproc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "PEXCRAFTBeta10Client";
    if (!RegisterClassA(&wc)) return 1;
    RECT rc = {0, 0, 854, 480};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hwnd = CreateWindowA(wc.lpszClassName, APP_TITLE, WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
                           NULL, NULL, hInstance, NULL);
    if (!g_hwnd) return 2;
    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);
    GetClientRect(g_hwnd, &rc);
    g_win_w = rc.right - rc.left;
    g_win_h = rc.bottom - rc.top;
    setup_scale();
    if (!init_gl(g_hwnd)) {
        MessageBoxA(g_hwnd, "PEXCRAFT was unable to start because it failed to initialize OpenGL or load assets.", APP_TITLE, MB_ICONERROR);
        return 3;
    }
    set_screen(SCREEN_TITLE);
    main_loop();
    set_mouse_grabbed(0);
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
    free_texture_pack_icons();
    if (g_wic_factory) { IWICImagingFactory_Release(g_wic_factory); g_wic_factory = NULL; }
    CoUninitialize();
    if (g_glrc) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(g_glrc);
    }
    if (g_hdc) ReleaseDC(g_hwnd, g_hdc);
    return 0;
}
