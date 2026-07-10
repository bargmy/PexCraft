/* SDL2/Linux window, OpenGL context, renderer-present, and main loop. */

static int g_stivufine_runtime_aa = 0;

static void stivufine_configure_gl_attributes(int samples) {
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, samples > 0 ? 1 : 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples > 0 ? samples : 0);
}

static int init_gl(SDL_Window *window) {
    (void)window;
    stivufine_configure_gl_attributes(g_stivufine_runtime_aa);

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
    if (g_stivufine_runtime_aa > 0) glEnable(GL_MULTISAMPLE);
    {
        int actual_samples = 0;
        SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &actual_samples);
        pex_logf("StivuFine antialiasing requested=%d actual=%d", g_opts.sf_aa_level, actual_samples);
    }
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
    handle_text_input_utf8(txt->text);
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
            legacy_assets_tick();
            if (g_screen == SCREEN_INGAME || g_screen == SCREEN_CHAT ||
                g_screen == SCREEN_INVENTORY || g_screen == SCREEN_CREATIVE || g_screen == SCREEN_WORKBENCH ||
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


static int g_sdl2_native_resource_prompt_handled = 0;

static void sdl2_apply_release_resource_selection(int textures, int audio_mask) {
    g_opts.download_classic_textures = textures ? 1 : 0;
    g_opts.download_classic_sounds = (audio_mask & CLASSIC_AUDIO_ALL) ? 1 : 0;
    g_opts.classic_audio_mask = audio_mask & CLASSIC_AUDIO_ALL;
    if (!g_opts.download_classic_textures && !g_opts.download_classic_sounds) {
        g_opts.ignore_classic_resources_warning = 1;
        g_opts.ignore_classic_sounds_warning = 1;
    } else {
        g_opts.ignore_classic_resources_warning = 0;
        g_opts.ignore_classic_sounds_warning = 0;
    }
    save_options();
}

static int sdl2_release_textures_installed(void) {
    return pack_is_installed() && !pack_missing_required_textures();
}

static int sdl2_release_audio_installed_mask(void) {
    int mask = 0;
    if (classic_sounds_installed_mask(CLASSIC_AUDIO_MOBS)) mask |= CLASSIC_AUDIO_MOBS;
    if (classic_sounds_installed_mask(CLASSIC_AUDIO_WORLD_UI)) mask |= CLASSIC_AUDIO_WORLD_UI;
    if (classic_sounds_installed_mask(CLASSIC_AUDIO_RECORDS)) mask |= CLASSIC_AUDIO_RECORDS;
    if (classic_sounds_installed_mask(CLASSIC_AUDIO_MENU_MUSIC)) mask |= CLASSIC_AUDIO_MENU_MUSIC;
    if (classic_sounds_installed_mask(CLASSIC_AUDIO_GAME_MUSIC)) mask |= CLASSIC_AUDIO_GAME_MUSIC;
    return mask & CLASSIC_AUDIO_ALL;
}

static int sdl2_release_audio_missing_mask(void) {
    return CLASSIC_AUDIO_ALL & ~sdl2_release_audio_installed_mask();
}

static int sdl2_release_resources_missing_actual(void) {
    /* Startup only installs the classic 1.2.5 client.jar texture pack.
       legacy.json assets are managed later from Options -> Assets. */
    return !sdl2_release_textures_installed();
}

static void sdl2_release_check_line(char *out, size_t cap, const char *name, int selected, int installed) {
    snprintf(out, cap, "[%c] %-24s %s", installed ? '-' : (selected ? 'X' : ' '), name, installed ? "(installed - locked)" : "");
}

static void sdl2_release_resource_selection_text(char *out, size_t cap, int textures, int audio_mask) {
    char size_line[MAX_LABEL];
    char line_tex[96], line_mob[96], line_ui[96], line_disc[96], line_menu[96], line_game[96];
    int textures_installed = sdl2_release_textures_installed();
    int installed_audio = sdl2_release_audio_installed_mask();
    int missing_audio = CLASSIC_AUDIO_ALL & ~installed_audio;
    int old_tex = g_opts.download_classic_textures;
    int old_snd = g_opts.download_classic_sounds;
    int old_mask = g_opts.classic_audio_mask;

    textures = textures_installed ? 0 : (textures ? 1 : 0);
    audio_mask &= missing_audio;

    g_opts.download_classic_textures = textures ? 1 : 0;
    g_opts.download_classic_sounds = (audio_mask & CLASSIC_AUDIO_ALL) ? 1 : 0;
    g_opts.classic_audio_mask = audio_mask & CLASSIC_AUDIO_ALL;
    pack_install_start_size_fetch();
    format_download_size(size_line, sizeof(size_line));
    g_opts.download_classic_textures = old_tex;
    g_opts.download_classic_sounds = old_snd;
    g_opts.classic_audio_mask = old_mask;

    sdl2_release_check_line(line_tex, sizeof(line_tex), "Textures", textures, textures_installed);
    sdl2_release_check_line(line_mob, sizeof(line_mob), "Mob sounds", audio_mask & CLASSIC_AUDIO_MOBS, installed_audio & CLASSIC_AUDIO_MOBS);
    sdl2_release_check_line(line_ui, sizeof(line_ui), "UI / block / world sounds", audio_mask & CLASSIC_AUDIO_WORLD_UI, installed_audio & CLASSIC_AUDIO_WORLD_UI);
    sdl2_release_check_line(line_disc, sizeof(line_disc), "Music discs", audio_mask & CLASSIC_AUDIO_RECORDS, installed_audio & CLASSIC_AUDIO_RECORDS);
    sdl2_release_check_line(line_menu, sizeof(line_menu), "Main menu music", audio_mask & CLASSIC_AUDIO_MENU_MUSIC, installed_audio & CLASSIC_AUDIO_MENU_MUSIC);
    sdl2_release_check_line(line_game, sizeof(line_game), "In-game music", audio_mask & CLASSIC_AUDIO_GAME_MUSIC, installed_audio & CLASSIC_AUDIO_GAME_MUSIC);

    snprintf(out, cap,
        "PexCraft Release Resources\n\n"
        "This is a native launcher popup, not an in-game menu.\n"
        "Nothing downloads until you press Download selected.\n\n"
        "Already downloaded items are shown as installed/locked and cannot be toggled.\n\n"
        "Current checklist:\n"
        "%s\n"
        "%s\n"
        "%s\n"
        "%s\n"
        "%s\n"
        "%s\n\n"
        "%s\n\n"
        "Use the category buttons to toggle only missing items, Select All Missing to toggle all missing items,\n"
        "or Play without selected to start without downloading.",
        line_tex, line_mob, line_ui, line_disc, line_menu, line_game, size_line);
}

static int sdl2_native_release_resources_run_download(void) {
    SDL_Window *win = NULL;
    SDL_Renderer *ren = NULL;
    HANDLE th = NULL;
    int cancelled = 0;
    LONG state;

    pack_install_reset_cancel();
    if (g_classic_install_thread) { CloseHandle(g_classic_install_thread); g_classic_install_thread = NULL; }
    g_classic_install_error[0] = 0;
    pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Preparing Release resources...");
    th = CreateThread(NULL, 0, pack_install_worker, NULL, 0, NULL);
    if (!th) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, APP_TITLE " - Resources", "Could not start the resource download thread.", NULL);
        pack_install_fail("Could not start installer thread");
        return 0;
    }

    win = SDL_CreateWindow(APP_TITLE " - Downloading Release Resources", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 560, 180, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (win) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!win || !ren) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, APP_TITLE " - Resources", "Download is running. This SDL build could not open the progress window.", NULL);
    }

    for (;;) {
        SDL_Event e;
        state = InterlockedCompareExchange(&g_classic_install_state, 0, 0);
        if (state == CLASSIC_INSTALL_DONE || state == CLASSIC_INSTALL_ERROR) break;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                cancelled = 1;
                pack_install_request_cancel();
            } else if (e.type == SDL_WINDOWEVENT && win && e.window.windowID == SDL_GetWindowID(win)) {
                if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
                    cancelled = 1;
                    pack_install_request_cancel();
                }
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                cancelled = 1;
                pack_install_request_cancel();
            }
        }
        if (win) {
            char title[MAX_LABEL * 2];
            int p = (int)InterlockedCompareExchange(&g_classic_install_progress, 0, 0);
            if (p < 0) p = 0;
            if (p > 100) p = 100;
            snprintf(title, sizeof(title), "%s - %d%% - %s%s", APP_TITLE, p, g_classic_install_status[0] ? g_classic_install_status : "Downloading...", cancelled ? " - canceling" : "");
            SDL_SetWindowTitle(win, title);
            if (ren) {
                int w = 560, h = 180;
                SDL_GetWindowSize(win, &w, &h);
                SDL_SetRenderDrawColor(ren, 28, 28, 32, 255);
                SDL_RenderClear(ren);
                SDL_Rect outline = {40, h / 2 - 12, w - 80, 24};
                SDL_Rect fill = {42, h / 2 - 10, (w - 84) * p / 100, 20};
                SDL_SetRenderDrawColor(ren, 120, 120, 120, 255);
                SDL_RenderDrawRect(ren, &outline);
                SDL_SetRenderDrawColor(ren, cancelled ? 160 : 80, cancelled ? 80 : 170, cancelled ? 80 : 90, 255);
                SDL_RenderFillRect(ren, &fill);
                SDL_RenderPresent(ren);
            }
        }
        SDL_Delay(33);
    }

    WaitForSingleObject(th, INFINITE);
    CloseHandle(th);
    if (ren) SDL_DestroyRenderer(ren);
    if (win) SDL_DestroyWindow(win);

    state = InterlockedCompareExchange(&g_classic_install_state, 0, 0);
    if (state == CLASSIC_INSTALL_DONE) {
        InterlockedExchange(&g_classic_install_state, CLASSIC_INSTALL_IDLE);
        if (g_opts.download_classic_textures && pack_is_installed()) {
            snprintf(g_opts.skin, sizeof(g_opts.skin), "%s", CLASSIC_PACK_NAME);
            save_options();
        }
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, APP_TITLE " - Resources", "Selected Release resources were downloaded.", NULL);
        return 1;
    }

    InterlockedExchange(&g_classic_install_state, CLASSIC_INSTALL_IDLE);
    if (cancelled || !strcmp(g_classic_install_error, "Canceled")) {
        /* Cancel is not a permanent opt-out. Missing resources must keep
           showing the native selector on the next launch. */
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, APP_TITLE " - Resources", "Resource download canceled. The game will continue without the selected resources for this launch.", NULL);
    } else {
        char msg[MAX_LABEL * 3];
        snprintf(msg, sizeof(msg), "Could not install Release resources.\n\n%s\n\nThe game will continue without silently retrying in-game.", g_classic_install_error[0] ? g_classic_install_error : "Unknown error");
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, APP_TITLE " - Resources", msg, NULL);
    }
    return 0;
}

static void sdl2_native_release_resources_prompt(void) {
    int buttonid = -1;
    SDL_MessageBoxButtonData buttons[2] = {
        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 100, "Download classic textures" },
        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 101, "Play without textures" }
    };
    SDL_MessageBoxColorScheme colors = {{
        { 245, 245, 245 }, { 32, 32, 36 }, { 64, 64, 70 }, { 255, 255, 255 }, { 40, 40, 46 }
    }};
    SDL_MessageBoxData data;
    char size_line[MAX_LABEL];
    g_sdl2_native_resource_prompt_handled = 1;
    if (sdl2_release_textures_installed()) return;
    g_opts.download_classic_textures = 1;
    g_opts.download_classic_sounds = 0;
    g_opts.classic_audio_mask = 0;
    pack_install_start_size_fetch();
    format_download_size(size_line, sizeof(size_line));
    memset(&data, 0, sizeof(data));
    data.flags = SDL_MESSAGEBOX_INFORMATION;
    data.window = NULL;
    data.title = APP_TITLE " - Classic Textures";
    data.message = "PexCraft needs the Minecraft 1.2.5 client.jar textures for the Release pack.\n\n"
                   "Startup only downloads/extracts the client.jar texture assets.\n"
                   "Sounds, music, languages, and other legacy.json assets are in Options -> Assets.\n\n"
                   "Press Download classic textures to install the jar texture pack.";
    data.numbuttons = 2;
    data.buttons = buttons;
    data.colorScheme = &colors;
    if (SDL_ShowMessageBox(&data, &buttonid) != 0) {
        fprintf(stderr, "PexCraft: classic texture popup could not be displayed; refusing to continue silently.\n");
        SDL_Quit();
        exit(4);
    }
    if (buttonid == 100) {
        g_opts.download_classic_textures = 1;
        g_opts.download_classic_sounds = 0;
        g_opts.classic_audio_mask = 0;
        save_options();
        sdl2_native_release_resources_run_download();
    }
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
    /* Native resource prompt must happen before the game window/GL UI exists.
       It does not use Minecraft textures, fonts, or in-game screens, and it
       does not download anything until the player presses Download selected. */
    if (sdl2_release_resources_missing_actual()) sdl2_native_release_resources_prompt();
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

    g_stivufine_runtime_aa = stivufine_aa_level();
    stivufine_configure_gl_attributes(g_stivufine_runtime_aa);
    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    g_hwnd = SDL_CreateWindow(APP_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 854, 480, flags);
    if (!g_hwnd && g_stivufine_runtime_aa > 0) {
        pex_logf("Multisample window failed at %dx; retrying without AA", g_stivufine_runtime_aa);
        g_stivufine_runtime_aa = 0;
        stivufine_configure_gl_attributes(0);
        g_hwnd = SDL_CreateWindow(APP_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 854, 480, flags);
    }
    if (!g_hwnd) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        IMG_Quit(); SDL_Quit();
        return 2;
    }
    SDL_GetWindowSize(g_hwnd, &g_win_w, &g_win_h);
    g_render_w = g_win_w; g_render_h = g_win_h;
    setup_scale();
    if (!pex_renderer_backend_init(g_hwnd)) {
        if (g_stivufine_runtime_aa > 0) {
            pex_logf("Multisample context failed at %dx; retrying without AA", g_stivufine_runtime_aa);
            if (g_glrc) { SDL_GL_DeleteContext(g_glrc); g_glrc = NULL; }
            SDL_DestroyWindow(g_hwnd); g_hwnd = NULL;
            g_stivufine_runtime_aa = 0;
            stivufine_configure_gl_attributes(0);
            g_hwnd = SDL_CreateWindow(APP_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 854, 480, flags);
            if (g_hwnd) {
                SDL_GetWindowSize(g_hwnd, &g_win_w, &g_win_h);
                g_render_w = g_win_w; g_render_h = g_win_h; setup_scale();
            }
        }
        if (!g_hwnd || !pex_renderer_backend_init(g_hwnd)) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, APP_TITLE, "PEXCRAFT failed to initialize SDL2/OpenGL or load assets.", g_hwnd);
            if (g_hwnd) SDL_DestroyWindow(g_hwnd); IMG_Quit(); SDL_Quit();
            return 3;
        }
    }
    if (g_opts.fullscreen) set_fullscreen_enabled(1);
    if (g_loggy_enabled) {
        loggy_init();
        if (g_glrc) SDL_GL_MakeCurrent(g_hwnd, g_glrc);
    }
    /* SDL2 uses the native pre-game selector. Never fall back to the in-game
       download prompt, because that UI depends on textures/fonts that may be
       missing. */
    if (!g_sdl2_native_resource_prompt_handled && !strcmp(g_opts.skin, CLASSIC_PACK_NAME) && g_opts.download_classic_textures && (!pack_is_installed() || pack_missing_required_textures())) set_screen(SCREEN_CLASSIC_PACK_WARNING);
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
