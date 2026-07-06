/* SDL2/Linux window, OpenGL context, renderer-present, and main loop. */

static int init_gl(SDL_Window *window) {
    (void)window;
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    g_glrc = SDL_GL_CreateContext(g_hwnd);
    if (!g_glrc) return 0;
    SDL_GL_MakeCurrent(g_hwnd, g_glrc);
    SDL_GL_SetSwapInterval((g_opts.anaglyph && g_opts.max_fps > 0) ? 1 : 0);

    glClearColor(0,0,0,0);
    glDisable(GL_DITHER);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    scan_texture_packs();
    if (!load_default_textures()) return 0;
    if (g_selected_texpack > 0) apply_texture_pack_index(g_selected_texpack);
    else { if (g_opts.skin_path[0]) load_custom_skin_path(g_opts.skin_path, 0); init_font_widths(); }
    return 1;
}

static int pex_renderer_backend_init(SDL_Window *window) {
    g_runtime_renderer_backend = RENDERER_OPENGL;
    g_selected_renderer_backend = RENDERER_OPENGL;
    return init_gl(window);
}
static int pex_renderer_begin_frame(void) { return 1; }
static void pex_renderer_present(void) { SDL_GL_SwapWindow(g_hwnd); }
static void pex_renderer_resize(int w, int h) { (void)w; (void)h; }
static void pex_renderer_shutdown(void) { }
static void pex_gl_suppress_immediate(int on) { (void)on; }

static void apply_vsync_setting(void) {
    if (g_hwnd) SDL_GL_SetSwapInterval((g_opts.anaglyph && g_opts.max_fps > 0) ? 1 : 0);
}

static void refresh_window_size_after_mode(void) {
    if (!g_hwnd) return;
    SDL_GetWindowSize(g_hwnd, &g_win_w, &g_win_h);
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
    if (g_hwnd) {
        SDL_SetWindowFullscreen(g_hwnd, enabled ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        refresh_window_size_after_mode();
    }
    save_options();
}

static void toggle_fullscreen(void) { set_fullscreen_enabled(!g_opts.fullscreen); }

static int sdl2_vk_from_key(SDL_Keycode key) {
    if (key >= 'a' && key <= 'z') return (int)(key - 'a' + 'A');
    if (key >= '0' && key <= '9') return (int)key;
    switch (key) {
        case SDLK_SPACE: return VK_SPACE;
        case SDLK_LSHIFT: case SDLK_RSHIFT: return VK_SHIFT;
        case SDLK_LCTRL: case SDLK_RCTRL: return VK_CONTROL;
        case SDLK_LALT: case SDLK_RALT: return VK_MENU;
        case SDLK_ESCAPE: return VK_ESCAPE;
        case SDLK_RETURN: return VK_RETURN;
        case SDLK_BACKSPACE: return VK_BACK;
        case SDLK_TAB: return VK_TAB;
        case SDLK_LEFT: return VK_LEFT;
        case SDLK_RIGHT: return VK_RIGHT;
        case SDLK_UP: return VK_UP;
        case SDLK_DOWN: return VK_DOWN;
        case SDLK_F3: return VK_F3;
        case SDLK_F5: return VK_F5;
        case SDLK_F11: return VK_F11;
        default: return 0;
    }
}

static void sdl2_handle_text_input(const SDL_TextInputEvent *txt) {
    if (!txt || !txt->text[0]) return;
    const unsigned char *p = (const unsigned char*)txt->text;
    /* The existing UI is byte/ASCII-oriented. Feed printable ASCII chars. */
    while (*p) {
        if (*p >= 32 && *p < 127) handle_char((WPARAM)*p);
        ++p;
    }
}

static void sdl2_handle_event(SDL_Event *e) {
    if (loggy_handle_event(e)) return;
    if (e && e->type != SDL_QUIT && g_hwnd) {
        Uint32 main_id = SDL_GetWindowID(g_hwnd);
        Uint32 event_id = 0;
        switch (e->type) {
            case SDL_WINDOWEVENT: event_id = e->window.windowID; break;
            case SDL_MOUSEMOTION: event_id = e->motion.windowID; break;
            case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP: event_id = e->button.windowID; break;
            case SDL_MOUSEWHEEL: event_id = e->wheel.windowID; break;
            case SDL_KEYDOWN: case SDL_KEYUP: event_id = e->key.windowID; break;
            case SDL_TEXTINPUT: event_id = e->text.windowID; break;
            default: break;
        }
        if (event_id && event_id != main_id) return;
    }
    switch (e->type) {
        case SDL_QUIT:
            save_world_state_for_exit();
            set_mouse_grabbed(0);
            g_running = 0;
            break;
        case SDL_WINDOWEVENT:
            if (e->window.event == SDL_WINDOWEVENT_SIZE_CHANGED || e->window.event == SDL_WINDOWEVENT_RESIZED) {
                g_win_w = e->window.data1; g_win_h = e->window.data2;
                if (g_win_w < 1) g_win_w = 1;
                if (g_win_h < 1) g_win_h = 1;
                g_render_w = g_win_w; g_render_h = g_win_h;
                setup_scale();
                pex_renderer_resize(g_win_w, g_win_h);
                rebuild_screen();
            } else if (e->window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                set_mouse_grabbed(0);
            } else if (e->window.event == SDL_WINDOWEVENT_FOCUS_GAINED && g_screen == SCREEN_INGAME) {
                set_mouse_grabbed(1);
            }
            break;
        case SDL_MOUSEMOTION:
            if (g_mouse_grabbed && g_screen == SCREEN_INGAME) {
                handle_grabbed_mouse_move(e->motion.xrel, e->motion.yrel);
                g_mouse_x = g_gui_w / 2; g_mouse_y = g_gui_h / 2;
            } else {
                int old_mouse_y = g_mouse_y;
                g_mouse_x = e->motion.x * g_gui_w / (g_win_w ? g_win_w : 1);
                g_mouse_y = e->motion.y * g_gui_h / (g_win_h ? g_win_h : 1);
                if (g_mouse_down && g_drag_slider) update_slider(g_drag_slider, g_mouse_x);
                if (g_mouse_down && g_screen == SCREEN_TEXPACK) texpack_mouse_drag(g_mouse_y);
                if (g_mouse_down && g_screen == SCREEN_CREATIVE) creative_mouse_drag(g_mouse_y);
                if (g_mouse_down && g_screen == SCREEN_LANGUAGE) language_drag_scroll(g_mouse_y - old_mouse_y);
                if (g_mouse_down && (g_screen == SCREEN_WORLD_SELECT || g_screen == SCREEN_WORLD_DELETE)) world_save_drag_scroll(g_mouse_y - old_mouse_y);
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (e->button.button < ARRAY_COUNT(g_sdl2_mouse_buttons)) g_sdl2_mouse_buttons[e->button.button] = 1;
            g_mouse_x = e->button.x * g_gui_w / (g_win_w ? g_win_w : 1);
            g_mouse_y = e->button.y * g_gui_h / (g_win_h ? g_win_h : 1);
            if (e->button.button == SDL_BUTTON_LEFT) mouse_down(g_mouse_x, g_mouse_y);
            else if (e->button.button == SDL_BUTTON_RIGHT) mouse_right_down(g_mouse_x, g_mouse_y);
            break;
        case SDL_MOUSEBUTTONUP:
            if (e->button.button < ARRAY_COUNT(g_sdl2_mouse_buttons)) g_sdl2_mouse_buttons[e->button.button] = 0;
            g_mouse_x = e->button.x * g_gui_w / (g_win_w ? g_win_w : 1);
            g_mouse_y = e->button.y * g_gui_h / (g_win_h ? g_win_h : 1);
            if (e->button.button == SDL_BUTTON_LEFT) mouse_up(g_mouse_x, g_mouse_y);
            else if (e->button.button == SDL_BUTTON_RIGHT) mouse_right_up(g_mouse_x, g_mouse_y);
            break;
        case SDL_MOUSEWHEEL:
            if (g_screen == SCREEN_CREATIVE) {
                if (e->wheel.y > 0) creative_scroll_by(-1);
                else if (e->wheel.y < 0) creative_scroll_by(1);
            } else if (g_screen == SCREEN_LANGUAGE) {
                if (e->wheel.y > 0) language_scroll_by(-1);
                else if (e->wheel.y < 0) language_scroll_by(1);
            } else if (g_screen == SCREEN_WORLD_SELECT || g_screen == SCREEN_WORLD_DELETE) {
                if (e->wheel.y > 0) world_save_scroll_by(-1);
                else if (e->wheel.y < 0) world_save_scroll_by(1);
            } else if (g_screen == SCREEN_INGAME) {
                if (e->wheel.y > 0) g_selected_hotbar_slot = (g_selected_hotbar_slot + 8) % 9;
                else if (e->wheel.y < 0) g_selected_hotbar_slot = (g_selected_hotbar_slot + 1) % 9;
            }
            break;
        case SDL_KEYDOWN: {
            int vk = sdl2_vk_from_key(e->key.keysym.sym);
            if (vk) {
                g_sdl2_key_state[sdl2_vk_index(vk)] = 1;
                if (vk != VK_F11 || e->key.repeat == 0) handle_keydown((WPARAM)vk);
            }
            break;
        }
        case SDL_KEYUP: {
            int vk = sdl2_vk_from_key(e->key.keysym.sym);
            if (vk) g_sdl2_key_state[sdl2_vk_index(vk)] = 0;
            break;
        }
        case SDL_TEXTINPUT:
            sdl2_handle_text_input(&e->text);
            break;
    }
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
    save_world_state_sync();
}

static void sleep_for_max_fps(double frame_start_time) {
    double sleep_start_time = now_seconds();
    if (g_opts.max_fps <= 0) {
        g_sleep_ms_last = (now_seconds() - sleep_start_time) * 1000.0;
        return;
    }
    double target = 1.0 / (double)g_opts.max_fps;
    for (;;) {
        double elapsed = now_seconds() - frame_start_time;
        double remaining = target - elapsed;
        if (remaining <= 0.0) break;
        if (remaining > 0.002) SDL_Delay((Uint32)(remaining * 1000.0));
        else SDL_Delay(0);
    }
    g_sleep_ms_last = (now_seconds() - sleep_start_time) * 1000.0;
}

static void main_loop(void) {
    g_last_time = now_seconds();
    double tick_accum = 0.0;
    while (g_running) {
        profile_begin_frame();
        double prof_start = profile_begin();
        SDL_Event e;
        while (SDL_PollEvent(&e)) sdl2_handle_event(&e);
        profile_add_time(PROF_PUMP, prof_start);
        pex_gamepad_update();
        if (g_mp_connected || pex_net_is_connecting()) {
            prof_start = profile_begin();
            pex_net_poll();
            profile_add_time(PROF_NET_POLL, prof_start);
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
            double tick_start = profile_begin();
            g_ticks++;
            if (g_screen == SCREEN_TITLE) tick_title_blocks();
            if (g_screen == SCREEN_GENERATING) {
                double t_worldgen = profile_begin();
                worldgen_tick();
                profile_add_time(PROF_WORLDGEN_TICK, t_worldgen);
            }
            if (g_screen == SCREEN_TEXPACK_INSTALL) pack_install_tick();
            if (g_screen == SCREEN_INGAME || g_screen == SCREEN_CHAT ||
                g_screen == SCREEN_INVENTORY || g_screen == SCREEN_WORKBENCH ||
                g_screen == SCREEN_FURNACE || g_screen == SCREEN_CHEST ||
                g_screen == SCREEN_DEATH || (g_mp_connected && g_screen == SCREEN_PAUSE)) ingame_tick_async_queue();
            profile_add_time(PROF_TICK_TOTAL, tick_start);
            tick_accum -= 1.0;
            ticks_this_frame++;
        }
        if (ticks_this_frame >= 3 && tick_accum > 1.0) tick_accum = 1.0;
        float partial = ingame_tick_async_render_partial((float)tick_accum);
        if (g_mp_connected) pex_net_update_smoothing();
        ingame_pump_async_tick();
        render(partial);
        sleep_for_max_fps(frame_start_time);
        profile_end_frame();
        loggy_draw();
        if (g_loggy_enabled && g_glrc) SDL_GL_MakeCurrent(g_hwnd, g_glrc);
    }
    SDL_StopTextInput();
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] && (!strcmp(argv[i], "--loggy") || !strcmp(argv[i], "--logger") || !strcmp(argv[i], "--diagnostics"))) g_loggy_enabled = 1;
    }
    pex_log_init();
    pex_install_crash_handlers();
    pex_logf("app main enter: %s", APP_TITLE);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    int img_flags = IMG_INIT_PNG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags) {
        fprintf(stderr, "SDL_image PNG init failed: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    init_dirs();
    srand((unsigned int)time(NULL));
    load_options();
    /* Never download Release resources silently on SDL2.  The main window is
       created first, then SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT asks the player
       before any network download starts. */
    char sound_dep_notice[MAX_LABEL * 3];
    if (!pex_sound_record_dependency_report(sound_dep_notice, sizeof(sound_dep_notice))) {
        char msg[MAX_LABEL * 4];
        snprintf(msg, sizeof(msg),
                 "%s\n\nRecords use Java-style positional streaming. Install:\n"
                 "sudo apt install libsdl2-mixer-2.0-0 libmpg123-0",
                 sound_dep_notice[0] ? sound_dep_notice : "Missing record audio dependencies.");
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, APP_TITLE " - Audio dependencies", msg, NULL);
    }
    pex_sound_rescan();
    g_runtime_renderer_backend = RENDERER_OPENGL;
    g_selected_renderer_backend = RENDERER_OPENGL;
    snprintf(g_multiplayer_ip, sizeof(g_multiplayer_ip), "%s", g_opts.last_server);
    snprintf(g_multiplayer_username, sizeof(g_multiplayer_username), "%s", g_opts.username[0] ? g_opts.username : "Player");

    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    g_hwnd = SDL_CreateWindow(APP_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 854, 480, flags);
    if (!g_hwnd) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        IMG_Quit(); SDL_Quit();
        return 2;
    }
    SDL_GetWindowSize(g_hwnd, &g_win_w, &g_win_h);
    g_render_w = g_win_w; g_render_h = g_win_h;
    setup_scale();
    if (!pex_renderer_backend_init(g_hwnd)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, APP_TITLE, "PEXCRAFT failed to initialize SDL2/OpenGL or load assets.", g_hwnd);
        SDL_DestroyWindow(g_hwnd); IMG_Quit(); SDL_Quit();
        return 3;
    }
    if (g_opts.fullscreen) set_fullscreen_enabled(1);
    if (g_loggy_enabled) {
        loggy_init();
        if (g_glrc) SDL_GL_MakeCurrent(g_hwnd, g_glrc);
    }
    if (should_show_pack_download_prompt()) set_screen(SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT);
    else if (!strcmp(g_opts.skin, CLASSIC_PACK_NAME) && classic_resources_need_update()) set_screen(SCREEN_CLASSIC_PACK_WARNING);
    else set_screen(pex_startup_screen());
    InitializeCriticalSection(&g_save_cs);
    main_loop();
    set_mouse_grabbed(0);
    world_stream_service_shutdown();
    ingame_tick_async_shutdown();
    async_section_mesh_shutdown();
    free_texture(&tex_bg); free_texture(&tex_gui); free_texture(&tex_font); free_texture(&tex_terrain);
    free_texture(&tex_black); free_texture(&tex_pack); free_texture(&tex_default_pack_icon); free_texture(&tex_unknown_pack);
    free_texture(&tex_icons); free_texture(&tex_inventory); free_texture(&tex_items); free_texture(&tex_steve); free_texture(&tex_mob_pig); free_texture(&tex_mob_sheep); free_texture(&tex_mob_sheep_fur); free_texture(&tex_mob_cow); free_texture(&tex_mob_chicken); free_texture(&tex_mob_saddle);
    free_texture(&tex_chest_entity); free_texture(&tex_large_chest_entity); free_texture(&tex_title_logo); free_texture(&tex_mojang); for (int i = 0; i < 6; ++i) free_texture(&tex_panorama[i]); free_texture_pack_icons();
    pex_gamepad_shutdown();
    pex_sound_shutdown();
    loggy_shutdown();
    pex_renderer_shutdown();
    if (g_glrc) { SDL_GL_DeleteContext(g_glrc); g_glrc = NULL; }
    pex_join_save_thread_for_exit();
    DeleteCriticalSection(&g_save_cs);
    if (g_hwnd) SDL_DestroyWindow(g_hwnd);
    IMG_Quit();
    SDL_Quit();
    pex_logf("app main exit");
    pex_log_shutdown();
    return 0;
}
