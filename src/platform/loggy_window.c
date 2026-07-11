/* Optional SDL2 diagnostic side-window.  Enabled only with --loggy. */

typedef struct LoggyWindowState {
    SDL_Window *window;
    SDL_Renderer *renderer;
    Uint32 window_id;
    int w, h;
    int scroll_px;
    int dragging_scroll;
    int drag_y;
    int drag_scroll_start;
    char text[65536];
    int line_count;
    double last_build_time;
} LoggyWindowState;

static LoggyWindowState g_loggy;

static const char *loggy_screen_name(ScreenId s) {
    switch (s) {
        case SCREEN_TITLE: return "TITLE";
        case SCREEN_OPTIONS: return "OPTIONS";
        case SCREEN_OPTIONS_MORE: return "OPTIONS_MORE";
        case SCREEN_STIVUFINE: return "STIVUFINE";
        case SCREEN_STIVUFINE_DETAILS: return "STIVUFINE_DETAILS";
        case SCREEN_STIVUFINE_QUALITY: return "STIVUFINE_QUALITY";
        case SCREEN_STIVUFINE_ANIMATIONS: return "STIVUFINE_ANIMATIONS";
        case SCREEN_STIVUFINE_PERFORMANCE: return "STIVUFINE_PERFORMANCE";
        case SCREEN_STIVUFINE_OTHER: return "STIVUFINE_OTHER";
        case SCREEN_ASSETS: return "ASSETS";
        case SCREEN_LANGUAGE: return "LANGUAGE";
        case SCREEN_SET_NAME: return "SET_NAME";
        case SCREEN_TV_REMOTE_MAP: return "TV_REMOTE_MAP";
        case SCREEN_SYSTEM_INFO: return "SYSTEM_INFO";
        case SCREEN_SKINS: return "SKINS";
        case SCREEN_CONTROLS: return "CONTROLS";
        case SCREEN_WORLD_SELECT: return "WORLD_SELECT";
        case SCREEN_CREATE_WORLD: return "CREATE_WORLD";
        case SCREEN_WORLD_TYPE: return "WORLD_TYPE";
        case SCREEN_WORLD_DELETE: return "WORLD_DELETE";
        case SCREEN_RENAME_WORLD: return "RENAME_WORLD";
        case SCREEN_CONFIRM_DELETE: return "CONFIRM_DELETE";
        case SCREEN_MULTIPLAYER: return "MULTIPLAYER";
        case SCREEN_CONNECTING: return "CONNECTING";
        case SCREEN_TEXPACK: return "TEXPACK";
        case SCREEN_TEXPACK_INSTALL: return "TEXPACK_INSTALL";
        case SCREEN_GENERATING: return "GENERATING";
        case SCREEN_SAVING_QUIT: return "SAVING_QUIT";
        case SCREEN_INGAME: return "INGAME";
        case SCREEN_PAUSE: return "PAUSE";
        case SCREEN_INVENTORY: return "INVENTORY";
        case SCREEN_CREATIVE: return "CREATIVE";
        case SCREEN_WORKBENCH: return "WORKBENCH";
        case SCREEN_FURNACE: return "FURNACE";
        case SCREEN_CHEST: return "CHEST";
        case SCREEN_DEATH: return "DEATH";
        case SCREEN_CHAT: return "CHAT";
        case SCREEN_VIRTUAL_KEYBOARD: return "VIRTUAL_KEYBOARD";
        case SCREEN_NOTICE: return "NOTICE";
        case SCREEN_RENDERER_RESTART_PROMPT: return "RENDERER_RESTART";
        case SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT: return "CLASSIC_PACK_PROMPT";
        case SCREEN_CLASSIC_PACK_WARNING: return "CLASSIC_PACK_WARNING";
        default: return "UNKNOWN";
    }
}

static void loggy_appendf(char **cursor, size_t *left, const char *fmt, ...) {
    if (!cursor || !*cursor || !left || *left <= 1 || !fmt) return;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(*cursor, *left, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if ((size_t)n >= *left) {
        *cursor += *left - 1;
        *left = 1;
    } else {
        *cursor += n;
        *left -= (size_t)n;
    }
}

static const unsigned char *loggy_glyph(char ch) {
    static const unsigned char blank[7] = {0,0,0,0,0,0,0};
    static const unsigned char question[7] = {14,17,1,2,4,0,4};
    static const unsigned char glyphs[64][7] = {
        {14,17,19,21,25,17,14}, /* 0 */ {4,12,4,4,4,4,14}, {14,17,1,2,4,8,31},
        {30,1,1,14,1,1,30}, {2,6,10,18,31,2,2}, {31,16,30,1,1,17,14},
        {6,8,16,30,17,17,14}, {31,1,2,4,8,8,8}, {14,17,17,14,17,17,14},
        {14,17,17,15,1,2,12},
        {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30}, {14,17,16,16,16,17,14},
        {30,17,17,17,17,17,30}, {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17}, {14,4,4,4,4,4,14},
        {7,2,2,2,18,18,12}, {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17}, {14,17,17,17,17,17,14},
        {30,17,17,30,16,16,16}, {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4}, {17,17,17,17,17,17,14},
        {17,17,17,17,17,10,4}, {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
        {0,0,0,0,0,0,0}, {4,4,4,4,4,0,4}, {10,10,0,0,0,0,0},
        {10,31,10,10,31,10,0}, {4,15,20,14,5,30,4}, {24,25,2,4,8,19,3},
        {12,18,20,8,21,18,13}, {4,4,0,0,0,0,0}, {2,4,8,8,8,4,2},
        {8,4,2,2,2,4,8}, {0,10,4,31,4,10,0}, {0,4,4,31,4,4,0},
        {0,0,0,0,0,4,8}, {0,0,0,31,0,0,0}, {0,0,0,0,0,12,12},
        {1,2,4,8,16,0,0}, {0,4,0,31,0,4,0}, {0,0,0,0,0,0,31},
        {0,4,0,0,4,0,0}, {0,0,0,31,0,31,0}, {2,4,8,4,2,0,0},
        {8,4,2,4,8,0,0}, {0,0,0,0,0,0,0}
    };
    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
    if (ch >= '0' && ch <= '9') return glyphs[ch - '0'];
    if (ch >= 'A' && ch <= 'Z') return glyphs[10 + (ch - 'A')];
    switch (ch) {
        case ' ': return glyphs[36]; case '!': return glyphs[37]; case '"': return glyphs[38]; case '#': return glyphs[39];
        case '$': return glyphs[40]; case '%': return glyphs[41]; case '&': return glyphs[42]; case '\'': return glyphs[43];
        case '(': return glyphs[44]; case ')': return glyphs[45]; case '*': return glyphs[46]; case '+': return glyphs[47];
        case ',': return glyphs[48]; case '-': return glyphs[49]; case '.': return glyphs[50]; case '/': return glyphs[51];
        case '=': return glyphs[53]; case '_': return glyphs[54]; case ':': return glyphs[55]; case ';': return glyphs[55];
        case '[': return glyphs[44]; case ']': return glyphs[45]; case '<': return glyphs[57]; case '>': return glyphs[58];
        default: return question;
    }
    return blank;
}

static void loggy_draw_text(SDL_Renderer *r, int x, int y, const char *s, SDL_Color color, int scale) {
    if (!r || !s) return;
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    int cx = x;
    for (const unsigned char *p = (const unsigned char*)s; *p; ++p) {
        if (*p == '\t') { cx += 6 * scale * 4; continue; }
        const unsigned char *rows = loggy_glyph((char)*p);
        for (int yy = 0; yy < 7; ++yy) {
            unsigned char bits = rows[yy];
            for (int xx = 0; xx < 5; ++xx) {
                if (bits & (1u << (4 - xx))) {
                    SDL_Rect rc = { cx + xx * scale, y + yy * scale, scale, scale };
                    SDL_RenderFillRect(r, &rc);
                }
            }
        }
        cx += 6 * scale;
    }
}

static int loggy_copy_button_hit(int x, int y) {
    return x >= g_loggy.w - 92 && x <= g_loggy.w - 8 && y >= 8 && y <= 30;
}

static int loggy_content_height(void) { return 42 + g_loggy.line_count * 12 + 20; }

static void loggy_count_lines(void) {
    int lines = 1;
    for (const char *p = g_loggy.text; *p; ++p) if (*p == '\n') lines++;
    g_loggy.line_count = lines;
}

static void loggy_build_text(void) {
    char *out = g_loggy.text;
    size_t left = sizeof(g_loggy.text);
    int mesh_jobs = 0, mesh_busy = 0, mesh_uploads = 0, mesh_upload_busy = 0, mesh_results = 0;
    int stream_jobs = 0, stream_active = 0, stream_results = 0, stream_workers = 0;
    int save_queue = 0;
    int light_dirty = 0;
    int active_drops = 0;
    int active_particles = 0;

    if (!world_quit_is_active() && g_async_section_mesh_initialized) {
        EnterCriticalSection(&g_async_section_mesh_cs);
        mesh_jobs = g_async_section_mesh_job_count;
        mesh_busy = g_async_section_mesh_busy;
        mesh_uploads = g_async_section_mesh_upload_count;
        mesh_upload_busy = g_async_section_mesh_upload_busy;
        mesh_results = g_async_section_mesh_result_count;
        LeaveCriticalSection(&g_async_section_mesh_cs);
    }
    if (!world_quit_is_active() && g_stream_async_initialized && g_stream_async_event) {
        EnterCriticalSection(&g_stream_async_cs);
        stream_jobs = g_stream_async_job_count;
        stream_active = g_stream_async_active_count;
        stream_results = g_stream_async_result_count;
        stream_workers = g_stream_async_worker_count;
        LeaveCriticalSection(&g_stream_async_cs);
    }
    if (!world_quit_is_active() && g_flat_light_dirty_cs_initialized) {
        EnterCriticalSection(&g_flat_light_dirty_cs);
        light_dirty = g_flat_light_dirty;
        LeaveCriticalSection(&g_flat_light_dirty_cs);
    }
    if (g_save_queue_head) {
        for (PexSaveSnapshot *ss = g_save_queue_head; ss; ss = ss->next) save_queue++;
    }
    for (int di = 0; di < MAX_DROP_ENTITIES; ++di) if (g_drops[di].active) active_drops++;
    for (int pi = 0; pi < MAX_DIG_PARTICLES; ++pi) if (g_dig_particles[pi].active) active_particles++;

    int q_remaining = g_stream_gen_queue_count - g_stream_gen_queue_index;
    if (q_remaining < 0) q_remaining = 0;
    g_loggy_stream_queue_remaining = q_remaining;
    g_loggy_stream_queue_total = g_stream_gen_queue_count;
    g_loggy_stream_queue_index = g_stream_gen_queue_index;
    g_loggy_stream_installed_total = g_stream_gen_queue_installed_count;
    g_loggy_stream_async_jobs = stream_jobs;
    g_loggy_stream_async_active = stream_active;
    g_loggy_stream_async_results = stream_results;
    g_loggy_stream_remap_active = g_stream_remap_in_progress ? 1 : 0;

    loggy_appendf(&out, &left, "PEXCRAFT LOGGY DIAGNOSTIC WINDOW\n");
    loggy_appendf(&out, &left, "COPY BUTTON TOP RIGHT. MOUSE WHEEL OR DRAG BAR TO SCROLL.\n");
    loggy_appendf(&out, &left, "FRAME=%d SCREEN=%d(%s) FPS=%d/%d/%d FRAME=%.2fMS RENDER=%.2fMS SLEEP=%.2fMS BACKEND=%d WINDOW=%dx%d GUI=%dx%d SCALE=%d\n\n",
                  g_loggy_frame_no, g_screen, loggy_screen_name(g_screen), g_debug_fps, g_debug_min_fps, g_debug_max_fps,
                  g_prof_display_frame_ms, g_render_ms_last, g_sleep_ms_last, g_runtime_renderer_backend,
                  g_win_w, g_win_h, g_gui_w, g_gui_h, g_gui_scale);

    loggy_appendf(&out, &left, "MAIN PROFILER BUCKETS AVG/CALLS/CURRENT-FRAME:\n");
    for (int i = 0; i < PROF_COUNT; ++i) {
        loggy_appendf(&out, &left, "  %-22s avg=%7.3fms now=%7.3fms calls=%3d pct=%5.1f%%\n",
                      g_prof_names[i], g_prof_display_ms[i], g_prof_frame_ms[i], g_loggy_profile_calls[i], profile_percent(g_prof_display_ms[i]));
    }

    loggy_appendf(&out, &left, "\nGUI / TEXT / BUTTONS:\n");
    loggy_appendf(&out, &left, "  text_calls=%d text_chars=%d gui_quads=%d buttons=%d debug_menu=%d chunk_info=%d task_info=%d\n",
                  g_loggy_gui_text_calls, g_loggy_gui_text_chars, g_loggy_gui_quads, g_loggy_gui_buttons,
                  g_debug_menu_shown, g_debug_chunk_info_shown, g_debug_task_info_shown);

    loggy_appendf(&out, &left, "\nRENDER / MESH:\n");
    loggy_appendf(&out, &left, "  visible_sections=%d generated_chunks=%d fancy=%d direct_backend=%d display_lists=%d\n",
                  g_loggy_visible_sections, g_loggy_visible_chunks, g_opts.fancy_graphics,
                  flat_direct_backend() ? 1 : 0, flat_display_lists_supported() ? 1 : 0);
    loggy_appendf(&out, &left, "  queue: jobs=%d busy=%d uploads=%d upload_busy=%d results=%d prof_jobs=%d prof_uploads=%d prof_results=%d\n",
                  mesh_jobs, mesh_busy, mesh_uploads, mesh_upload_busy, mesh_results,
                  g_prof_mesh_jobs_last, g_prof_mesh_uploads_last, g_prof_mesh_results_last);
    loggy_appendf(&out, &left, "  frame: submit_calls=%d snapshot_cells=%d installs=%d install_vtx=%d install_idx=%d stale_results=%d\n",
                  g_loggy_mesh_submit_calls, g_loggy_mesh_submit_snapshot_cells,
                  g_loggy_mesh_installs, g_loggy_mesh_install_vertices, g_loggy_mesh_install_indices,
                  g_loggy_mesh_stale_results);
    loggy_appendf(&out, &left, "  self_heal_probes=%d self_heal_missing=%d skylight_sub=%d light_dirty=%d light_ready=%d\n",
                  g_loggy_mesh_self_heal_refs, g_loggy_mesh_self_heal_missing,
                  g_prof_skylight_subtracted_last, light_dirty, g_flat_light_dirty_cs_initialized ? 1 : 0);
    loggy_appendf(&out, &left, "  edit_priority queued=%d drained=%d sync=%d async=%d left=%d\n",
                  g_loggy_mesh_edit_priority_queued, g_loggy_mesh_edit_priority_drained,
                  g_loggy_mesh_edit_priority_sync, g_loggy_mesh_edit_priority_async,
                  g_loggy_mesh_edit_priority_left);

    loggy_appendf(&out, &left, "\nSTREAM / WORLD WINDOW:\n");
    loggy_appendf(&out, &left, "  origin=%d,%d player_chunk=%d,%d local_chunk=%d,%d generated_center=%d,%d remap=%d\n",
                  g_flat_world_origin_x, g_flat_world_origin_z,
                  floor_div16((int)floorf(g_player_x)), floor_div16((int)floorf(g_player_z)),
                  (int)floorf((g_player_x - (float)g_flat_world_origin_x) / 16.0f),
                  (int)floorf((g_player_z - (float)g_flat_world_origin_z) / 16.0f),
                  g_stream_last_center_chunk_x, g_stream_last_center_chunk_z, g_stream_remap_in_progress ? 1 : 0);
    loggy_appendf(&out, &left, "  gen_queue index=%d total=%d remaining=%d installed=%d keep_completed=%d epoch=%d pending_prof=%d\n",
                  g_stream_gen_queue_index, g_stream_gen_queue_count, q_remaining, g_stream_gen_queue_installed_count,
                  g_stream_generation_keep_completed, g_stream_generation_epoch, g_prof_stream_pending_last);
    loggy_appendf(&out, &left, "  async_workers=%d jobs=%d active=%d results=%d initial_batch=%d done=%d/%d light_settle req/run/done/prog=%d/%d/%d/%d\n",
                  stream_workers, stream_jobs, stream_active, stream_results,
                  g_stream_initial_batch_running, g_stream_initial_batch_done_units, g_stream_initial_batch_total_units,
                  g_stream_initial_light_settle_requested, g_stream_initial_light_settle_running,
                  g_stream_initial_light_settle_done, g_stream_initial_light_settle_progress);

    loggy_appendf(&out, &left, "\nTHREADS / WORKERS:\n");
    loggy_appendf(&out, &left, "  main role=%d async_tick last=%.3f avg=%.3f samples=%d\n",
                  g_pex_profile_thread_role, g_prof_async_tick_last_ms, g_prof_async_tick_avg_ms, g_prof_async_tick_samples);
    loggy_appendf(&out, &left, "  stream_service last=%.3f avg=%.3f samples=%d workers=%d install=%.3f n=%d one=%.3f submit=%.3f\n",
                  g_prof_stream_worker_last_ms, g_prof_stream_worker_avg_ms, g_prof_stream_worker_samples, stream_workers,
                  g_prof_stream_service_install_ms, g_prof_stream_service_installs,
                  g_prof_stream_install_one_ms, g_prof_stream_service_submit_ms);
    loggy_appendf(&out, &left, "  stream_chunk last_chunk=%d,%d terrain=%.3f delta=%.3f light=%.3f pushwait=%.3f\n",
                  g_prof_stream_worker_last_cx, g_prof_stream_worker_last_cz,
                  g_prof_stream_worker_terrain_ms, g_prof_stream_worker_delta_ms,
                  g_prof_stream_worker_light_ms, g_prof_stream_worker_push_wait_ms);
    loggy_appendf(&out, &left, "  mesh_worker last=%.3f avg=%.3f samples=%d busy=%d upload_last=%.3f upload_avg=%.3f upload_samples=%d upload_busy=%d\n",
                  g_prof_mesh_worker_last_ms, g_prof_mesh_worker_avg_ms, g_prof_mesh_worker_samples, mesh_busy,
                  g_prof_mesh_upload_worker_last_ms, g_prof_mesh_upload_worker_avg_ms, g_prof_mesh_upload_worker_samples, mesh_upload_busy);
    loggy_appendf(&out, &left, "  light_worker last=%.3f avg=%.3f samples=%d dirty=%d ready=%d\n",
                  g_prof_light_worker_last_ms, g_prof_light_worker_avg_ms, g_prof_light_worker_samples, light_dirty, g_flat_light_dirty_cs_initialized ? 1 : 0);
    loggy_appendf(&out, &left, "  save_thread=%p save_done=%d save_shutdown=%d save_queue=%d\n",
                  (void*)g_save_thread, g_save_thread_done, g_save_thread_shutdown, save_queue);

    loggy_appendf(&out, &left, "\nGAMEPLAY / SIM:\n");
    loggy_appendf(&out, &left, "  player=(%.2f %.2f %.2f) motion=(%.3f %.3f %.3f) on_ground=%d selected_slot=%d world_type=%d structures=%d\n",
                  g_player_x, g_player_y, g_player_z, g_player_motion_x, g_player_motion_y, g_player_motion_z,
                  g_player_on_ground, g_selected_hotbar_slot, g_world_type, g_world_map_features);
    loggy_appendf(&out, &left, "  falling active=%d spawned=%d cells=%d particles=%d drops=%d liquids_ms=%.3f world_stream_ms=%.3f\n",
                  g_prof_falling_active_last, g_prof_falling_spawns_last, g_prof_falling_cells_last,
                  active_particles, active_drops, g_prof_display_ms[PROF_LIQUIDS], g_prof_display_ms[PROF_WORLD_STREAM]);
    loggy_appendf(&out, &left, "  mobs active=%d ticked=%d path_solves=%d failed=%d nodes=%d peak=%d path_ms=%.3f\n",
                  g_passive_perf_last_active, g_prof_mob_living_ticked_last,
                  g_prof_mob_path_solves_last, g_prof_mob_path_failed_last,
                  g_prof_mob_path_nodes_last, g_prof_mob_path_peak_nodes_last,
                  g_prof_mob_path_ms_last);
    loggy_appendf(&out, &left, "  mob_spawn_ms=%.3f mob_living_ms=%.3f spawn_probes=%d columns=%d cache=%d/%d village_scan_blocks=%d\n",
                  g_prof_mob_spawn_ms_last, g_prof_mob_living_ms_last,
                  g_prof_mob_spawn_calls_last, g_prof_mob_spawn_columns_last,
                  g_prof_mob_spawn_probe_hits_last, g_prof_mob_spawn_probe_misses_last,
                  g_prof_village_scan_blocks_last);

    loggy_appendf(&out, &left, "\nHOT QUEUE SAMPLE:\n");
    int maxq = q_remaining < 16 ? q_remaining : 16;
    for (int i = 0; i < maxq; ++i) {
        int qi = g_stream_gen_queue_index + i;
        loggy_appendf(&out, &left, "  stream_q[%02d] world_chunk=%d,%d score=%d\n", i,
                      g_stream_gen_queue_cx[qi], g_stream_gen_queue_cz[qi],
                      stream_chunk_priority_score(g_stream_gen_queue_cx[qi], g_stream_gen_queue_cz[qi]));
    }
    if (maxq <= 0) loggy_appendf(&out, &left, "  none\n");

    loggy_appendf(&out, &left, "\nNOTES:\n");
    loggy_appendf(&out, &left, "  This window shows instrumented hot paths, queues, worker states, and frame buckets.\n");
    loggy_appendf(&out, &left, "  It does not magically hook every C function; add LOGGY_POINT/name counters for new suspects.\n");
    *out = '\0';
    loggy_count_lines();
}

static int loggy_init(void) {
    if (!g_loggy_enabled || g_loggy.window) return 1;
    int x = SDL_WINDOWPOS_CENTERED, y = SDL_WINDOWPOS_CENTERED;
    if (g_hwnd) {
        int mx = 0, my = 0, mw = 854, mh = 480;
        SDL_GetWindowPosition(g_hwnd, &mx, &my);
        SDL_GetWindowSize(g_hwnd, &mw, &mh);
        x = mx + mw + 12;
        y = my;
    }
    g_loggy.w = 760;
    g_loggy.h = 720;
    g_loggy.window = SDL_CreateWindow("PexCraft Loggy", x, y, g_loggy.w, g_loggy.h, SDL_WINDOW_RESIZABLE);
    if (!g_loggy.window) {
        fprintf(stderr, "--loggy: SDL_CreateWindow failed: %s\n", SDL_GetError());
        g_loggy_enabled = 0;
        return 0;
    }
    g_loggy.window_id = SDL_GetWindowID(g_loggy.window);
    g_loggy.renderer = SDL_CreateRenderer(g_loggy.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_loggy.renderer) g_loggy.renderer = SDL_CreateRenderer(g_loggy.window, -1, SDL_RENDERER_SOFTWARE);
    if (!g_loggy.renderer) {
        fprintf(stderr, "--loggy: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_loggy.window);
        memset(&g_loggy, 0, sizeof(g_loggy));
        g_loggy_enabled = 0;
        return 0;
    }
    SDL_SetWindowMinimumSize(g_loggy.window, 420, 260);
    loggy_build_text();
    return 1;
}

static int loggy_handle_event(SDL_Event *e) {
    if (!g_loggy_enabled || !g_loggy.window || !e) return 0;
    Uint32 id = 0;
    switch (e->type) {
        case SDL_WINDOWEVENT: id = e->window.windowID; break;
        case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP: id = e->button.windowID; break;
        case SDL_MOUSEMOTION: id = e->motion.windowID; break;
        case SDL_MOUSEWHEEL: id = e->wheel.windowID; break;
        case SDL_KEYDOWN: case SDL_KEYUP: id = e->key.windowID; break;
        default: return 0;
    }
    if (id != g_loggy.window_id) return 0;

    int content_h = loggy_content_height();
    int max_scroll = content_h > g_loggy.h ? content_h - g_loggy.h : 0;
    if (e->type == SDL_WINDOWEVENT) {
        if (e->window.event == SDL_WINDOWEVENT_CLOSE) {
            SDL_DestroyRenderer(g_loggy.renderer);
            SDL_DestroyWindow(g_loggy.window);
            memset(&g_loggy, 0, sizeof(g_loggy));
            g_loggy_enabled = 0;
        } else if (e->window.event == SDL_WINDOWEVENT_SIZE_CHANGED || e->window.event == SDL_WINDOWEVENT_RESIZED) {
            g_loggy.w = e->window.data1;
            g_loggy.h = e->window.data2;
            if (g_loggy.scroll_px > max_scroll) g_loggy.scroll_px = max_scroll;
        }
        return 1;
    }
    if (e->type == SDL_MOUSEWHEEL) {
        g_loggy.scroll_px -= e->wheel.y * 48;
        if (g_loggy.scroll_px < 0) g_loggy.scroll_px = 0;
        if (g_loggy.scroll_px > max_scroll) g_loggy.scroll_px = max_scroll;
        return 1;
    }
    if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT) {
        int x = e->button.x, y = e->button.y;
        if (loggy_copy_button_hit(x, y)) {
            SDL_SetClipboardText(g_loggy.text);
            return 1;
        }
        if (x >= g_loggy.w - 14) {
            g_loggy.dragging_scroll = 1;
            g_loggy.drag_y = y;
            g_loggy.drag_scroll_start = g_loggy.scroll_px;
            return 1;
        }
    }
    if (e->type == SDL_MOUSEBUTTONUP && e->button.button == SDL_BUTTON_LEFT) {
        g_loggy.dragging_scroll = 0;
        return 1;
    }
    if (e->type == SDL_MOUSEMOTION && g_loggy.dragging_scroll) {
        int dy = e->motion.y - g_loggy.drag_y;
        if (max_scroll > 0 && g_loggy.h > 0) {
            g_loggy.scroll_px = g_loggy.drag_scroll_start + dy * max_scroll / (g_loggy.h > 1 ? g_loggy.h : 1);
            if (g_loggy.scroll_px < 0) g_loggy.scroll_px = 0;
            if (g_loggy.scroll_px > max_scroll) g_loggy.scroll_px = max_scroll;
        }
        return 1;
    }
    return 1;
}

static void loggy_draw(void) {
    if (!g_loggy_enabled || !g_loggy.window || !g_loggy.renderer) return;
    double now = now_seconds();
    if (now - g_loggy.last_build_time > 0.10) {
        loggy_build_text();
        g_loggy.last_build_time = now;
    }
    SDL_GetWindowSize(g_loggy.window, &g_loggy.w, &g_loggy.h);
    int content_h = loggy_content_height();
    int max_scroll = content_h > g_loggy.h ? content_h - g_loggy.h : 0;
    if (g_loggy.scroll_px > max_scroll) g_loggy.scroll_px = max_scroll;
    if (g_loggy.scroll_px < 0) g_loggy.scroll_px = 0;

    SDL_Renderer *r = g_loggy.renderer;
    SDL_SetRenderDrawColor(r, 18, 18, 20, 255);
    SDL_RenderClear(r);
    SDL_Rect header = {0, 0, g_loggy.w, 38};
    SDL_SetRenderDrawColor(r, 35, 37, 44, 255);
    SDL_RenderFillRect(r, &header);
    SDL_Rect copy = {g_loggy.w - 92, 8, 84, 22};
    SDL_SetRenderDrawColor(r, 70, 82, 110, 255);
    SDL_RenderFillRect(r, &copy);
    SDL_SetRenderDrawColor(r, 130, 150, 190, 255);
    SDL_RenderDrawRect(r, &copy);
    SDL_Color white = {225, 225, 225, 255};
    SDL_Color green = {135, 235, 170, 255};
    loggy_draw_text(r, 10, 12, "LOGGY LIVE", green, 2);
    loggy_draw_text(r, g_loggy.w - 80, 15, "COPY", white, 1);

    int y = 46 - g_loggy.scroll_px;
    char line[512];
    const char *p = g_loggy.text;
    while (*p) {
        int len = 0;
        while (p[len] && p[len] != '\n' && len < (int)sizeof(line) - 1) len++;
        memcpy(line, p, (size_t)len);
        line[len] = 0;
        if (y > 32 && y < g_loggy.h - 8) {
            SDL_Color c = white;
            if (strstr(line, "PROFILER") || strstr(line, "RENDER") || strstr(line, "STREAM") || strstr(line, "THREADS") || strstr(line, "GAMEPLAY")) {
                c.r = 120; c.g = 210; c.b = 255;
            } else if (strstr(line, "avg=") || strstr(line, "FRAME=")) {
                c.r = 250; c.g = 235; c.b = 150;
            }
            loggy_draw_text(r, 10, y, line, c, 1);
        }
        y += 12;
        p += len;
        if (*p == '\n') p++;
    }

    SDL_SetRenderDrawColor(r, 45, 45, 50, 255);
    SDL_Rect bar = {g_loggy.w - 10, 42, 6, g_loggy.h - 48};
    SDL_RenderFillRect(r, &bar);
    if (max_scroll > 0) {
        int track_h = bar.h;
        int thumb_h = (g_loggy.h * track_h) / content_h;
        if (thumb_h < 28) thumb_h = 28;
        if (thumb_h > track_h) thumb_h = track_h;
        int thumb_y = bar.y + (g_loggy.scroll_px * (track_h - thumb_h)) / max_scroll;
        SDL_Rect thumb = {bar.x, thumb_y, bar.w, thumb_h};
        SDL_SetRenderDrawColor(r, 160, 170, 190, 255);
        SDL_RenderFillRect(r, &thumb);
    }
    SDL_RenderPresent(r);
}

static void loggy_shutdown(void) {
    if (g_loggy.renderer) SDL_DestroyRenderer(g_loggy.renderer);
    if (g_loggy.window) SDL_DestroyWindow(g_loggy.window);
    memset(&g_loggy, 0, sizeof(g_loggy));
}
