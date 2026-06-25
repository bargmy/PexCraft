/* Native Win32 diagnostic side-window. Enabled with --loggy on Windows.
   This is intentionally not SDL based: it uses a normal HWND + EDIT control
   so the Windows build gets a real separate diagnostics window. */

#define PEX_WIN32_LOGGY_UPDATE_SECONDS 0.10
#define PEX_WIN32_LOGGY_TEXT_CAP 65536
#define PEX_WIN32_LOGGY_EDIT_CAP 131072
#define PEX_WIN32_LOGGY_TIMER 9401
#define PEX_WIN32_LOGGY_COPY_BUTTON 9402

typedef struct LoggyWindowState {
    HWND hwnd;
    HWND edit;
    HWND copy_button;
    HWND label;
    HFONT mono_font;
    char text[PEX_WIN32_LOGGY_TEXT_CAP];
    char edit_text[PEX_WIN32_LOGGY_EDIT_CAP];
    int line_count;
    double last_build_time;
    int refreshing;
} LoggyWindowState;

static LoggyWindowState g_loggy;

static int loggy_cmdline_has_flag(const char *cmdline, const char *flag) {
    const char *p = cmdline;
    size_t flag_len = flag ? strlen(flag) : 0;
    if (!p || !flag || !flag_len) return 0;
    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        const char *start = p;
        const char *end = p;
        if (*p == '"') {
            start = ++p;
            while (*p && *p != '"') p++;
            end = p;
            if (*p == '"') p++;
        } else {
            while (*p && *p != ' ' && *p != '\t') p++;
            end = p;
        }
        if ((size_t)(end - start) == flag_len && !strncmp(start, flag, flag_len)) return 1;
    }
    return 0;
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

    if (g_async_section_mesh_initialized) {
        EnterCriticalSection(&g_async_section_mesh_cs);
        mesh_jobs = g_async_section_mesh_job_count;
        mesh_busy = g_async_section_mesh_busy;
        mesh_uploads = g_async_section_mesh_upload_count;
        mesh_upload_busy = g_async_section_mesh_upload_busy;
        mesh_results = g_async_section_mesh_result_count;
        LeaveCriticalSection(&g_async_section_mesh_cs);
    }
    if (g_stream_async_initialized && g_stream_async_event) {
        EnterCriticalSection(&g_stream_async_cs);
        stream_jobs = g_stream_async_job_count;
        stream_active = g_stream_async_active_count;
        stream_results = g_stream_async_result_count;
        stream_workers = g_stream_async_worker_count;
        LeaveCriticalSection(&g_stream_async_cs);
    }
    if (g_flat_light_dirty_cs_initialized) {
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
    loggy_appendf(&out, &left, "NATIVE WIN32 HWND. COPY BUTTON TOP RIGHT. EDIT CONTROL HAS REAL SCROLLBARS.\n");
    loggy_appendf(&out, &left, "FRAME=%d SCREEN=%d FPS=%d/%d/%d FRAME=%.2fMS RENDER=%.2fMS SLEEP=%.2fMS BACKEND=%d WINDOW=%dx%d GUI=%dx%d SCALE=%d\n\n",
                  g_loggy_frame_no, g_screen, g_debug_fps, g_debug_min_fps, g_debug_max_fps,
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
    loggy_appendf(&out, &left, "  self_heal_refs=%d self_heal_missing=%d skylight_sub=%d light_dirty=%d light_ready=%d\n",
                  g_loggy_mesh_self_heal_refs, g_loggy_mesh_self_heal_missing,
                  g_prof_skylight_subtracted_last, light_dirty, g_flat_light_dirty_cs_initialized ? 1 : 0);

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
    loggy_appendf(&out, &left, "  stream_worker last=%.3f avg=%.3f samples=%d workers=%d\n",
                  g_prof_stream_worker_last_ms, g_prof_stream_worker_avg_ms, g_prof_stream_worker_samples, stream_workers);
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
    loggy_appendf(&out, &left, "  Windows --loggy is native Win32 now; no SDL window/context is involved.\n");
    loggy_appendf(&out, &left, "  This window shows instrumented hot paths, queues, worker states, and frame buckets.\n");
    loggy_appendf(&out, &left, "  It does not magically hook every C function; add LOGGY_POINT/name counters for new suspects.\n");
    *out = '\0';
    loggy_count_lines();
}

static void loggy_make_crlf_text(void) {
    char *dst = g_loggy.edit_text;
    size_t left = sizeof(g_loggy.edit_text);
    const char *src = g_loggy.text;
    char prev = 0;
    while (*src && left > 1) {
        if (*src == '\n' && prev != '\r') {
            if (left <= 2) break;
            *dst++ = '\r';
            *dst++ = '\n';
            left -= 2;
        } else {
            *dst++ = *src;
            left--;
        }
        prev = *src++;
    }
    *dst = '\0';
}

static void loggy_copy_text_to_clipboard(void) {
    SIZE_T len;
    HGLOBAL mem;
    char *dst;
    if (!g_loggy.hwnd) return;
    loggy_make_crlf_text();
    len = (SIZE_T)strlen(g_loggy.edit_text);
    mem = GlobalAlloc(GMEM_MOVEABLE, len + 1);
    if (!mem) return;
    dst = (char*)GlobalLock(mem);
    if (!dst) { GlobalFree(mem); return; }
    memcpy(dst, g_loggy.edit_text, len + 1);
    GlobalUnlock(mem);
    if (!OpenClipboard(g_loggy.hwnd)) { GlobalFree(mem); return; }
    EmptyClipboard();
    SetClipboardData(CF_TEXT, mem);
    CloseClipboard();
}

static void loggy_layout_controls(void) {
    RECT rc;
    int w, h;
    if (!g_loggy.hwnd) return;
    GetClientRect(g_loggy.hwnd, &rc);
    w = rc.right - rc.left;
    h = rc.bottom - rc.top;
    if (w < 180) w = 180;
    if (h < 120) h = 120;
    if (g_loggy.label) MoveWindow(g_loggy.label, 10, 10, w - 118, 22, TRUE);
    if (g_loggy.copy_button) MoveWindow(g_loggy.copy_button, w - 98, 8, 86, 26, TRUE);
    if (g_loggy.edit) MoveWindow(g_loggy.edit, 8, 42, w - 16, h - 50, TRUE);
}

static void loggy_set_edit_text_preserve_scroll(void) {
    LRESULT first_line;
    DWORD sel_start = 0, sel_end = 0;
    if (!g_loggy.edit || g_loggy.refreshing) return;
    g_loggy.refreshing = 1;
    loggy_make_crlf_text();
    first_line = SendMessageA(g_loggy.edit, EM_GETFIRSTVISIBLELINE, 0, 0);
    SendMessageA(g_loggy.edit, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    SendMessageA(g_loggy.edit, WM_SETREDRAW, FALSE, 0);
    SetWindowTextA(g_loggy.edit, g_loggy.edit_text);
    SendMessageA(g_loggy.edit, EM_SETSEL, (WPARAM)sel_start, (LPARAM)sel_end);
    if (first_line > 0) SendMessageA(g_loggy.edit, EM_LINESCROLL, 0, first_line);
    SendMessageA(g_loggy.edit, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_loggy.edit, NULL, TRUE);
    g_loggy.refreshing = 0;
}

static void loggy_refresh_now(void) {
    if (!g_loggy_enabled || !g_loggy.hwnd || !g_loggy.edit) return;
    loggy_build_text();
    loggy_set_edit_text_preserve_scroll();
    g_loggy.last_build_time = now_seconds();
}

static LRESULT CALLBACK loggy_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_CREATE: {
            HFONT font;
            g_loggy.hwnd = hwnd;
            font = CreateFontA(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                               FIXED_PITCH | FF_MODERN, "Consolas");
            if (!font) font = (HFONT)GetStockObject(ANSI_FIXED_FONT);
            g_loggy.mono_font = font;
            g_loggy.label = CreateWindowExA(0, "STATIC", "LOGGY LIVE - native Windows window",
                WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 10, 500, 22, hwnd, NULL, g_inst, NULL);
            g_loggy.copy_button = CreateWindowExA(0, "BUTTON", "Copy",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 600, 8, 86, 26,
                hwnd, (HMENU)(INT_PTR)PEX_WIN32_LOGGY_COPY_BUTTON, g_inst, NULL);
            g_loggy.edit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL |
                WS_VSCROLL | WS_HSCROLL, 8, 42, 740, 640, hwnd, NULL, g_inst, NULL);
            if (g_loggy.label) SendMessageA(g_loggy.label, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
            if (g_loggy.copy_button) SendMessageA(g_loggy.copy_button, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
            if (g_loggy.edit) {
                SendMessageA(g_loggy.edit, WM_SETFONT, (WPARAM)font, TRUE);
                SendMessageA(g_loggy.edit, EM_SETLIMITTEXT, (WPARAM)(PEX_WIN32_LOGGY_EDIT_CAP - 1), 0);
            }
            loggy_layout_controls();
            SetTimer(hwnd, PEX_WIN32_LOGGY_TIMER, 100, NULL);
            loggy_refresh_now();
            return 0;
        }
        case WM_SIZE:
            loggy_layout_controls();
            return 0;
        case WM_COMMAND:
            if (LOWORD(wparam) == PEX_WIN32_LOGGY_COPY_BUTTON) {
                loggy_copy_text_to_clipboard();
                return 0;
            }
            break;
        case WM_TIMER:
            if (wparam == PEX_WIN32_LOGGY_TIMER) {
                loggy_refresh_now();
                return 0;
            }
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            KillTimer(hwnd, PEX_WIN32_LOGGY_TIMER);
            g_loggy.hwnd = NULL;
            g_loggy.edit = NULL;
            g_loggy.copy_button = NULL;
            g_loggy.label = NULL;
            g_loggy_enabled = 0;
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static int loggy_init(void) {
    WNDCLASSA wc;
    RECT main_rc;
    RECT work_rc;
    int w = 780, h = 720;
    int x = CW_USEDEFAULT, y = CW_USEDEFAULT;
    if (!g_loggy_enabled || g_loggy.hwnd) return 1;

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = loggy_wndproc;
    wc.hInstance = g_inst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "PexCraftNativeLoggyWindow";
    RegisterClassA(&wc);

    if (g_hwnd && GetWindowRect(g_hwnd, &main_rc)) {
        x = main_rc.right + 12;
        y = main_rc.top;
    }
    if (SystemParametersInfoA(SPI_GETWORKAREA, 0, &work_rc, 0)) {
        if (x == CW_USEDEFAULT || x + w > work_rc.right) x = work_rc.right - w;
        if (y == CW_USEDEFAULT || y + h > work_rc.bottom) y = work_rc.bottom - h;
        if (x < work_rc.left) x = work_rc.left;
        if (y < work_rc.top) y = work_rc.top;
    }

    g_loggy.hwnd = CreateWindowExA(WS_EX_APPWINDOW, wc.lpszClassName, "PexCraft Loggy",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, x, y, w, h, NULL, NULL, g_inst, NULL);
    if (!g_loggy.hwnd) {
        log_windows_error("CreateWindow native loggy");
        g_loggy_enabled = 0;
        return 0;
    }
    ShowWindow(g_loggy.hwnd, SW_SHOW);
    UpdateWindow(g_loggy.hwnd);
    return 1;
}

static void loggy_draw(void) {
    double now;
    if (!g_loggy_enabled || !g_loggy.hwnd) return;
    now = now_seconds();
    if (now - g_loggy.last_build_time >= PEX_WIN32_LOGGY_UPDATE_SECONDS) {
        loggy_refresh_now();
    }
}

static void loggy_shutdown(void) {
    HWND hwnd = g_loggy.hwnd;
    HFONT font = g_loggy.mono_font;
    if (hwnd) DestroyWindow(hwnd);
    if (font && font != GetStockObject(ANSI_FIXED_FONT)) DeleteObject(font);
    memset(&g_loggy, 0, sizeof(g_loggy));
}
