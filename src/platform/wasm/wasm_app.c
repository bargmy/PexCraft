/* SDL2 + WebGL browser entry.  This target deliberately has no runtime HTTP,
   no multiplayer entry point, and no pthread dependency. */

#define PEX_WASM_INITIAL_W 854
#define PEX_WASM_INITIAL_H 480

static double g_wasm_tick_accum = 0.0;
static double g_wasm_last_idb_sync = 0.0;
static double g_wasm_last_world_snapshot = 0.0;
static int g_wasm_shutdown_done = 0;
static int g_wasm_game_initialized = 0;

/* Keep the CSS display size and the actual WebGL drawing buffer in sync.
   A canvas has two independent sizes in browsers. CSS can stretch an
   854x480 drawing buffer to the whole window without changing the pixels that
   WebGL renders into, which breaks viewports and makes the title panorama look
   like a flickering corner thumbnail. */
#define PEX_WASM_MAX_DRAWABLE_DIM 4096

static void wasm_sync_idbfs(void) {
    EM_ASM({
        if (Module.pexSyncPersistentStorage) Module.pexSyncPersistentStorage();
    });
}

static void pex_wasm_open_github(void) {
    EM_ASM({
        var url = 'https://github.com/bargmy/PexCraft';
        var opened = window.open(url, '_blank');
        if (opened) opened.opener = null;
        else window.location.href = url;
    });
}

static int pex_wasm_choose_skin_file(void) {
    EM_ASM({
        if (Module.pexPickSkin) Module.pexPickSkin();
    });
    return 1;
}

static int pex_wasm_choose_texture_pack_file(void) {
    EM_ASM({
        if (Module.pexPickTexturePack) Module.pexPickTexturePack();
    });
    return 1;
}

static int wasm_read_first_line(const char *path, char *out, size_t cap) {
    FILE *f;
    if (!out || cap == 0) return 0;
    out[0] = 0;
    f = fopen(path, "rb");
    if (!f) return 0;
    if (!fgets(out, (int)cap, f)) { fclose(f); return 0; }
    fclose(f);
    out[strcspn(out, "\r\n")] = 0;
    return out[0] != 0;
}

static void wasm_sanitize_pack_name(const char *input, char *out, size_t cap) {
    const char *name = input ? input : "";
    const char *slash = strrchr(name, '/');
    const char *backslash = strrchr(name, '\\');
    size_t n = 0;
    if (slash && slash + 1 > name) name = slash + 1;
    if (backslash && backslash + 1 > name) name = backslash + 1;
    if (!out || cap == 0) return;
    while (*name && n + 1 < cap) {
        unsigned char c = (unsigned char)*name++;
        if (c == '.' && (!strcasecmp(name, "zip") || !strcasecmp(name, "jar"))) break;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_' || c == '.') {
            out[n++] = (char)c;
        } else {
            out[n++] = '_';
        }
    }
    while (n > 0 && (out[n - 1] == ' ' || out[n - 1] == '.')) --n;
    out[n] = 0;
    if (!out[0]) snprintf(out, cap, "Imported Pack");
}

static int wasm_pack_has_root_assets(const char *dir) {
    static const char *const candidates[] = {
        "terrain.png", "gui/gui.png", "gui/items.png", "gui/background.png",
        "mob/char.png", "char.png", "environment/clouds.png",
        "misc/grasscolor.png", "font/default.png"
    };
    char path[MAX_PATHBUF];
    for (int i = 0; i < (int)ARRAY_COUNT(candidates); ++i) {
        path_join(path, sizeof(path), dir, candidates[i]);
        if (file_exists(path)) return 1;
    }
    return 0;
}

static int wasm_move_directory_contents(const char *src, const char *dst) {
    DIR *d = opendir(src);
    struct dirent *de;
    if (!d) return 0;
    while ((de = readdir(d)) != NULL) {
        char from[MAX_PATHBUF], to[MAX_PATHBUF];
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        path_join(from, sizeof(from), src, de->d_name);
        path_join(to, sizeof(to), dst, de->d_name);
        delete_recursive(to);
        unlink(to);
        if (rename(from, to) != 0) {
            closedir(d);
            return 0;
        }
    }
    closedir(d);
    return rmdir(src) == 0 || errno == ENOENT;
}

static int wasm_flatten_single_pack_directory(const char *root) {
    DIR *d;
    struct dirent *de;
    char only_dir[MAX_PATHBUF] = "";
    int dir_count = 0;
    if (wasm_pack_has_root_assets(root)) return 1;
    d = opendir(root);
    if (!d) return 0;
    while ((de = readdir(d)) != NULL) {
        char child[MAX_PATHBUF];
        struct stat st;
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        path_join(child, sizeof(child), root, de->d_name);
        if (!strcmp(de->d_name, "__MACOSX") || !strcmp(de->d_name, ".DS_Store")) {
            delete_recursive(child);
            unlink(child);
            continue;
        }
        if (stat(child, &st) == 0 && S_ISDIR(st.st_mode)) {
            ++dir_count;
            snprintf(only_dir, sizeof(only_dir), "%s", child);
        } else {
            /* A root file means this is not a simple one-folder wrapper. */
            closedir(d);
            return 0;
        }
    }
    closedir(d);
    if (dir_count != 1 || !wasm_pack_has_root_assets(only_dir)) return 0;
    return wasm_move_directory_contents(only_dir, root);
}

EMSCRIPTEN_KEEPALIVE void pex_wasm_finish_skin_import(void) {
    const char *path = "/persist/skins/custom.png";
    if (!load_custom_skin_path(path, 1)) {
        unlink(path);
        wasm_sync_idbfs();
        return;
    }
    wasm_sync_idbfs();
    if (g_screen == SCREEN_SKINS) rebuild_screen();
}

EMSCRIPTEN_KEEPALIVE void pex_wasm_finish_texture_pack_import(void) {
    const char *zip_path = "/persist/imports/texturepack.zip";
    const char *name_path = "/persist/imports/texturepack.name";
    char original_name[256] = "Imported Pack.zip";
    char pack_name[64];
    char dest[MAX_PATHBUF];
    char err[256] = "";
    int old_lang = pxc_zip_extract_lang_only;
    int old_pack_txt = pxc_zip_write_pack_txt_after_extract;
    int ok;

    wasm_read_first_line(name_path, original_name, sizeof(original_name));
    wasm_sanitize_pack_name(original_name, pack_name, sizeof(pack_name));
    if (!strcasecmp(pack_name, CLASSIC_PACK_NAME))
        snprintf(pack_name, sizeof(pack_name), "%s Imported", CLASSIC_PACK_NAME);
    path_join(dest, sizeof(dest), g_texpack_dir, pack_name);
    /* Re-importing a pack replaces it atomically from the user's perspective;
       stale files from an older ZIP must not remain in the directory. */
    delete_recursive(dest);

    pxc_zip_extract_lang_only = 0;
    pxc_zip_write_pack_txt_after_extract = 0;
    ok = pxc_extract_zip_file(zip_path, dest, err, sizeof(err));
    pxc_zip_extract_lang_only = old_lang;
    pxc_zip_write_pack_txt_after_extract = old_pack_txt;

    if (ok && !wasm_pack_has_root_assets(dest)) ok = wasm_flatten_single_pack_directory(dest);
    if (ok && !wasm_pack_has_root_assets(dest)) {
        snprintf(err, sizeof(err), "The ZIP does not contain a compatible classic texture pack.");
        ok = 0;
    }
    if (ok) {
        char pack_txt[MAX_PATHBUF];
        path_join(pack_txt, sizeof(pack_txt), dest, "pack.txt");
        if (!file_exists(pack_txt)) {
            char text[160];
            snprintf(text, sizeof(text), "%s\nImported in the WebAssembly client\n", pack_name);
            pxc_write_file_all(pack_txt, (const unsigned char *)text, strlen(text));
        }
    }

    unlink(zip_path);
    unlink(name_path);
    if (!ok) {
        delete_recursive(dest);
        wasm_sync_idbfs();
        open_notice("Texture Packs", "Could not import the selected ZIP.",
                    err[0] ? err : "Use a Minecraft 1.2.5-style texture pack ZIP.");
        return;
    }

    scan_texture_packs();
    int applied = 0;
    for (int i = 0; i < g_texpack_count; ++i) {
        if (!strcmp(g_texpacks[i].name, pack_name)) {
            apply_texture_pack_index(i);
            applied = 1;
            break;
        }
    }
    wasm_sync_idbfs();
    if (!applied) {
        open_notice("Texture Packs", "The ZIP was extracted, but the pack could not be selected.", pack_name);
        return;
    }
    set_screen(SCREEN_TEXPACK);
}

static void wasm_apply_first_run_graphics_defaults(void) {
    /* These are browser-only first-run defaults. Once options.txt exists, every
       player change is respected on later launches. */
    g_opts.fancy_graphics = 0;
    g_opts.render_distance = 4;
    g_opts.sf_ao_level = 0.0f;
    g_opts.sf_gamma = 1.0f;
    g_opts.sf_clouds = SF_FAST;
    g_opts.sf_trees = SF_FAST;
    g_opts.sf_grass = SF_OFF;
    g_opts.sf_particles = 2;
    g_opts.sf_random_mobs = 0;
    g_opts.sf_smooth_biomes = 0;
    g_opts.sf_mipmap_level = 0;
    g_opts.sf_mipmap_linear = 0;
    g_opts.sf_connected_textures = SF_OFF;
    g_opts.sf_natural_textures = 0;
    g_opts.sf_aa_level = 0;
    g_opts.sf_af_level = 1;
    g_opts.sf_chunk_updates = 1;
    g_opts.sf_chunk_updates_dynamic = 0;
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

static int wasm_refresh_display_metrics(void) {
    double css_w = 0.0, css_h = 0.0;
    int old_win_w = g_win_w, old_win_h = g_win_h;
    int old_render_w = g_render_w, old_render_h = g_render_h;

    /* Query the size actually occupied by the canvas in CSS pixels, then make
       its WebGL drawing buffer match the physical display resolution. This is
       deliberately owned by the WASM target instead of relying on CSS
       stretching or SDL's initial 854x480 window size. */
    if (emscripten_get_element_css_size("#canvas", &css_w, &css_h) == EMSCRIPTEN_RESULT_SUCCESS &&
        css_w >= 1.0 && css_h >= 1.0) {
        double scale = emscripten_get_device_pixel_ratio();
        int canvas_w = 0, canvas_h = 0;
        int logical_w = (int)floor(css_w + 0.5);
        int logical_h = (int)floor(css_h + 0.5);

        if (!(scale > 0.0)) scale = 1.0;
        if (scale > 2.0) scale = 2.0;
        if (scale < 1.0) scale = 1.0;
        if (css_w * scale > (double)PEX_WASM_MAX_DRAWABLE_DIM)
            scale = (double)PEX_WASM_MAX_DRAWABLE_DIM / css_w;
        if (css_h * scale > (double)PEX_WASM_MAX_DRAWABLE_DIM)
            scale = (double)PEX_WASM_MAX_DRAWABLE_DIM / css_h;

        g_win_w = logical_w > 0 ? logical_w : 1;
        g_win_h = logical_h > 0 ? logical_h : 1;
        g_render_w = (int)floor(css_w * scale + 0.5);
        g_render_h = (int)floor(css_h * scale + 0.5);
        if (g_render_w < 1) g_render_w = 1;
        if (g_render_h < 1) g_render_h = 1;

        if (emscripten_get_canvas_element_size("#canvas", &canvas_w, &canvas_h) != EMSCRIPTEN_RESULT_SUCCESS ||
            canvas_w != g_render_w || canvas_h != g_render_h) {
            emscripten_set_canvas_element_size("#canvas", g_render_w, g_render_h);
        }
        /* Use the real size accepted by the browser in case WebGL clamps the
           requested drawing buffer to the device's maximum viewport. */
        if (emscripten_get_canvas_element_size("#canvas", &canvas_w, &canvas_h) == EMSCRIPTEN_RESULT_SUCCESS &&
            canvas_w > 0 && canvas_h > 0) {
            g_render_w = canvas_w;
            g_render_h = canvas_h;
        }
    } else if (g_hwnd) {
        SDL_GetWindowSize(g_hwnd, &g_win_w, &g_win_h);
        if (g_win_w < 1) g_win_w = 1;
        if (g_win_h < 1) g_win_h = 1;
        g_render_w = g_win_w;
        g_render_h = g_win_h;
        if (g_glrc) SDL_GL_GetDrawableSize(g_hwnd, &g_render_w, &g_render_h);
        if (g_render_w < 1) g_render_w = g_win_w;
        if (g_render_h < 1) g_render_h = g_win_h;
    }

    return old_win_w != g_win_w || old_win_h != g_win_h ||
           old_render_w != g_render_w || old_render_h != g_render_h;
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
                if (g_screen == SCREEN_INGAME) set_screen(SCREEN_PAUSE);
                else set_mouse_grabbed(0);
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

EMSCRIPTEN_KEEPALIVE void pex_wasm_pointer_lock_lost(void) {
    if (g_wasm_game_initialized && !g_wasm_shutdown_done && g_screen == SCREEN_INGAME)
        set_screen(SCREEN_PAUSE);
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
}

static void wasm_frame(void) {
    if (!g_running) {
        wasm_shutdown();
        emscripten_cancel_main_loop();
        return;
    }

    profile_begin_frame();

    /* Browser viewport changes do not reliably arrive as SDL events for every
       local-file/fullscreen/DPI transition. Poll the CSS canvas size once per
       frame and rebuild projections only when its logical or drawable size
       actually changed. */
    if (wasm_refresh_display_metrics()) {
        setup_scale();
        pex_renderer_resize(g_render_w, g_render_h);
        rebuild_screen();
    }

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

    /* Browser unload handlers cannot wait for an asynchronous IndexedDB flush.
       Keep the on-disk snapshot fresh during play so a reload loses at most a
       few seconds even when the user never presses Save and Quit. */
    if (!g_mp_connected && g_loaded_world_dir[0] &&
        frame_start_time - g_wasm_last_world_snapshot >= 3.0) {
        g_wasm_last_world_snapshot = frame_start_time;
        save_world_state_sync();
        wasm_sync_idbfs();
    } else if (frame_start_time - g_wasm_last_idb_sync >= 5.0) {
        g_wasm_last_idb_sync = frame_start_time;
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
    char wasm_options_path[MAX_PATHBUF];
    path_join(wasm_options_path, sizeof(wasm_options_path), g_mc_dir, "options.txt");
    int wasm_first_run = !file_exists(wasm_options_path);
    load_options();
    /* Browser resources are bundled and network-backed audio/multiplayer paths
       stay disabled. Performance preferences are defaults only, not forced. */
    g_opts.download_classic_textures = 1;
    g_opts.download_classic_sounds = 0;
    g_opts.classic_audio_mask = 0;
    g_opts.ignore_classic_sounds_warning = 1;
    g_opts.ignore_classic_resources_warning = 1;
    g_opts.renderer_backend = RENDERER_OPENGL;
    if (wasm_first_run) wasm_apply_first_run_graphics_defaults();
    if (!g_opts.skin[0] || !strcmp(g_opts.skin, "Default"))
        snprintf(g_opts.skin, sizeof(g_opts.skin), "%s", CLASSIC_PACK_NAME);
    if (wasm_first_run) {
        save_options();
        wasm_sync_idbfs();
    }
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
    g_wasm_last_world_snapshot = g_last_time;
    emscripten_set_main_loop(wasm_frame, 0, 0);
    return 0;
}
