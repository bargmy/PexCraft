/* SDL2 + WebGL browser entry.  This target deliberately has no runtime HTTP,
   no multiplayer entry point, and no pthread dependency. */

#define PEX_WASM_INITIAL_W 854
#define PEX_WASM_INITIAL_H 480

static double g_wasm_tick_accum = 0.0;
static double g_wasm_last_idb_sync = 0.0;
static int g_wasm_shutdown_done = 0;
static int g_wasm_game_initialized = 0;

static void wasm_sync_idbfs(void) {
    EM_ASM({
        if (typeof FS !== 'undefined' && !Module.pexIdbSyncBusy) {
            Module.pexIdbSyncBusy = true;
            FS.syncfs(false, function(err) {
                Module.pexIdbSyncBusy = false;
                if (err) console.error('PexCraft IDBFS sync failed:', err);
            });
        }
    });
}

static int init_gl(SDL_Window *window) {
    (void)window;
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    g_glrc = SDL_GL_CreateContext(g_hwnd);
    if (!g_glrc) return 0;
    if (SDL_GL_MakeCurrent(g_hwnd, g_glrc) != 0) return 0;
    SDL_GL_SetSwapInterval(1);

    if (!pex_lgwebos_gles2_init()) return 0;
    glClearColor(0, 0, 0, 0);
    glDisable(GL_DITHER);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    scan_texture_packs();
    if (!load_default_textures()) return 0;
    if (g_selected_texpack > 0) apply_texture_pack_index(g_selected_texpack);
    else {
        if (g_opts.skin_path[0]) load_custom_skin_path(g_opts.skin_path, 0);
        init_font_widths();
    }
    return 1;
}

static int pex_renderer_backend_init(SDL_Window *window) {
    g_runtime_renderer_backend = RENDERER_OPENGL;
    g_selected_renderer_backend = RENDERER_OPENGL;
    return init_gl(window);
}
static int pex_renderer_begin_frame(void) { return 1; }
static void pex_renderer_present(void) { if (g_hwnd) SDL_GL_SwapWindow(g_hwnd); }
static void pex_renderer_resize(int w, int h) { (void)w; (void)h; }
static void pex_renderer_shutdown(void) { pex_lgwebos_gles2_shutdown(); }
static void pex_gl_suppress_immediate(int on) { (void)on; }

static void apply_vsync_setting(void) {
    if (g_hwnd) SDL_GL_SetSwapInterval(1);
}

static void wasm_refresh_display_metrics(void) {
    if (!g_hwnd) return;
    SDL_GetWindowSize(g_hwnd, &g_win_w, &g_win_h);
    if (g_win_w < 1) g_win_w = 1;
    if (g_win_h < 1) g_win_h = 1;
    g_render_w = g_win_w;
    g_render_h = g_win_h;
    if (g_glrc) SDL_GL_GetDrawableSize(g_hwnd, &g_render_w, &g_render_h);
    if (g_render_w < 1) g_render_w = g_win_w;
    if (g_render_h < 1) g_render_h = g_win_h;
}

static void refresh_window_size_after_mode(void) {
    if (!g_hwnd) return;
    wasm_refresh_display_metrics();
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
    wasm_sync_idbfs();
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
    if (txt && txt->text[0]) handle_text_input_utf8(txt->text);
}

static void sdl2_handle_event(SDL_Event *e) {
    if (!e) return;
    switch (e->type) {
        case SDL_QUIT:
            save_world_state_for_exit();
            wasm_sync_idbfs();
            set_mouse_grabbed(0);
            g_running = 0;
            break;
        case SDL_WINDOWEVENT:
            if (e->window.event == SDL_WINDOWEVENT_SIZE_CHANGED || e->window.event == SDL_WINDOWEVENT_RESIZED) {
                wasm_refresh_display_metrics();
                setup_scale();
                pex_renderer_resize(g_render_w, g_render_h);
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
                g_mouse_x = g_gui_w / 2;
                g_mouse_y = g_gui_h / 2;
            } else {
                int old_mouse_y = g_mouse_y;
                g_mouse_x = e->motion.x * g_gui_w / (g_win_w ? g_win_w : 1);
                g_mouse_y = e->motion.y * g_gui_h / (g_win_h ? g_win_h : 1);
                if (g_mouse_down && g_drag_slider) update_slider(g_drag_slider, g_mouse_x);
                if (g_mouse_down && g_screen == SCREEN_TEXPACK) texpack_mouse_drag(g_mouse_y);
                if (g_mouse_down && g_screen == SCREEN_ACHIEVEMENTS) pex_achievements_mouse_drag(g_mouse_x, g_mouse_y);
                if (g_mouse_down && g_screen == SCREEN_STATISTICS) pex_statistics_mouse_drag(g_mouse_x, g_mouse_y);
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
                if (e->wheel.y > 0) creative_scroll_by(-1); else if (e->wheel.y < 0) creative_scroll_by(1);
            } else if (g_screen == SCREEN_LANGUAGE) {
                if (e->wheel.y > 0) language_scroll_by(-1); else if (e->wheel.y < 0) language_scroll_by(1);
            } else if (g_screen == SCREEN_WORLD_SELECT || g_screen == SCREEN_WORLD_DELETE) {
                if (e->wheel.y > 0) world_save_scroll_by(-1); else if (e->wheel.y < 0) world_save_scroll_by(1);
            } else if (g_screen == SCREEN_STATISTICS) {
                if (e->wheel.y > 0) pex_statistics_scroll_by(-1); else if (e->wheel.y < 0) pex_statistics_scroll_by(1);
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
    /* CreateThread is disabled on WASM, but clear any queued snapshots safely. */
    while (g_save_queue_head) {
        PexSaveSnapshot *ss = g_save_queue_head;
        g_save_queue_head = ss->next;
        pex_save_snapshot_free(ss);
    }
    g_save_queue_tail = NULL;
    if (g_save_event) { CloseHandle(g_save_event); g_save_event = NULL; }
    g_save_thread = NULL;
    g_save_thread_done = 1;
    g_save_thread_shutdown = 0;
}

static void save_world_state_for_exit(void) {
    pex_stats_flush();
    if (world_quit_is_active()) return;
    pex_join_save_thread_for_exit();
    if (!g_mp_connected && g_loaded_world_dir[0]) save_world_state_sync();
    wasm_sync_idbfs();
}

EMSCRIPTEN_KEEPALIVE void pex_wasm_visibility_flush(void) {
    if (g_wasm_game_initialized && !g_wasm_shutdown_done) save_world_state_for_exit();
}

static void wasm_shutdown(void) {
    if (g_wasm_shutdown_done) return;
    g_wasm_shutdown_done = 1;
    g_wasm_game_initialized = 0;
    set_mouse_grabbed(0);
    world_quit_shutdown_for_app_exit();
    ingame_tick_async_shutdown();
    world_stream_service_shutdown();
    async_section_mesh_shutdown();
    passive_render_worker_shutdown();
    pex_mp_cache_save_shutdown();
    world_stream_shared_locks_shutdown();
    free_texture(&tex_bg); free_texture(&tex_gui); free_texture(&tex_font); free_texture(&tex_terrain);
    free_texture(&tex_black); free_texture(&tex_pack); free_texture(&tex_default_pack_icon); free_texture(&tex_unknown_pack);
    free_texture(&tex_icons); free_texture(&tex_inventory); free_texture(&tex_items); free_texture(&tex_steve);
    free_texture(&tex_mob_pig); free_texture(&tex_mob_sheep); free_texture(&tex_mob_sheep_fur);
    free_texture(&tex_mob_cow); free_texture(&tex_mob_chicken); free_texture(&tex_mob_saddle);
    free_texture(&tex_chest_entity); free_texture(&tex_large_chest_entity); free_texture_pack_icons();
    pex_gamepad_shutdown();
    pex_renderer_shutdown();
    if (g_glrc) { SDL_GL_DeleteContext(g_glrc); g_glrc = NULL; }
    pex_join_save_thread_for_exit();
    DeleteCriticalSection(&g_save_cs);
    if (g_hwnd) { SDL_DestroyWindow(g_hwnd); g_hwnd = NULL; }
    IMG_Quit();
    SDL_Quit();
    pex_logf("app main exit");
    pex_log_shutdown();
    wasm_sync_idbfs();
    EM_ASM({
        var el = document.getElementById('status');
        if (el) { el.textContent = 'PexCraft stopped'; el.style.display = 'block'; }
    });
}

static void wasm_frame(void) {
    if (!g_running) {
        wasm_shutdown();
        emscripten_cancel_main_loop();
        return;
    }

    profile_begin_frame();
    double prof_start = profile_begin();
    SDL_Event e;
    while (SDL_PollEvent(&e)) sdl2_handle_event(&e);
    profile_add_time(PROF_PUMP, prof_start);
    pex_gamepad_update();

    double frame_start_time = now_seconds();
    double dt = frame_start_time - g_last_time;
    if (dt < 0.0) dt = 0.0;
    if (dt > 0.25) dt = 0.25;
    g_last_time = frame_start_time;
    g_wasm_tick_accum += dt * 20.0;

    int ticks_this_frame = 0;
    while (g_wasm_tick_accum >= 1.0 && ticks_this_frame < 3) {
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
            g_screen == SCREEN_INVENTORY || g_screen == SCREEN_CREATIVE ||
            g_screen == SCREEN_WORKBENCH || g_screen == SCREEN_FURNACE ||
            g_screen == SCREEN_CHEST || g_screen == SCREEN_DEATH) {
            ingame_tick_async_queue();
        }
        profile_add_time(PROF_TICK_TOTAL, tick_start);
        g_wasm_tick_accum -= 1.0;
        ticks_this_frame++;
    }
    if (ticks_this_frame >= 3 && g_wasm_tick_accum > 1.0) g_wasm_tick_accum = 1.0;

    float partial = ingame_tick_async_render_partial((float)g_wasm_tick_accum);
    ingame_pump_async_tick();
    render(partial);
    profile_end_frame();

    /* IDBFS is local browser storage, not a network operation. */
    if (frame_start_time - g_wasm_last_idb_sync >= 15.0) {
        g_wasm_last_idb_sync = frame_start_time;
        if (g_save_dirty) save_world_state_sync();
        wasm_sync_idbfs();
    }
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    init_dirs();
    chdir(g_mc_dir);
    pex_log_init();
    pex_install_crash_handlers();
    pex_logf("app main enter: %s (WebAssembly offline build)", APP_TITLE);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    int img_flags = IMG_INIT_PNG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags) {
        fprintf(stderr, "SDL_image PNG init failed: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    srand((unsigned int)time(NULL));
    load_options();
    /* Browser profile: resources are bundled, multiplayer/audio downloads are off,
       and conservative render distance keeps the single-threaded build responsive. */
    g_opts.download_classic_textures = 1;
    g_opts.download_classic_sounds = 0;
    g_opts.classic_audio_mask = 0;
    g_opts.ignore_classic_sounds_warning = 1;
    g_opts.ignore_classic_resources_warning = 1;
    g_opts.renderer_backend = RENDERER_OPENGL;
    g_opts.sf_aa_level = 0;
    if (g_opts.render_distance > 4) g_opts.render_distance = 4;
    if (g_opts.render_distance < 2) g_opts.render_distance = 2;
    if (!g_opts.skin[0] || !strcmp(g_opts.skin, "Default")) snprintf(g_opts.skin, sizeof(g_opts.skin), "%s", CLASSIC_PACK_NAME);
    g_runtime_renderer_backend = RENDERER_OPENGL;
    g_selected_renderer_backend = RENDERER_OPENGL;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    g_hwnd = SDL_CreateWindow(APP_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              PEX_WASM_INITIAL_W, PEX_WASM_INITIAL_H,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!g_hwnd) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        IMG_Quit(); SDL_Quit();
        return 2;
    }

    wasm_refresh_display_metrics();
    setup_scale();
    if (!pex_renderer_backend_init(g_hwnd)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, APP_TITLE,
            "PexCraft could not initialize WebGL or load the embedded Minecraft 1.2.5 resources.", g_hwnd);
        SDL_DestroyWindow(g_hwnd); IMG_Quit(); SDL_Quit();
        return 3;
    }
    if (!pack_is_installed()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, APP_TITLE,
            "The standalone HTML is missing its embedded Release textures/fonts.", g_hwnd);
        return 4;
    }

    InitializeCriticalSection(&g_save_cs);
    g_wasm_game_initialized = 1;
    set_screen(pex_startup_screen());
    g_last_time = now_seconds();
    g_wasm_last_idb_sync = g_last_time;
    EM_ASM({
        var el = document.getElementById('status');
        if (el) el.style.display = 'none';
    });
    emscripten_set_main_loop(wasm_frame, 0, 0);
    return 0;
}
