/* Native Win32 diagnostic side-window. Enabled with --loggy on Windows.
   This is intentionally not SDL based: it uses a normal HWND + EDIT control
   so the Windows build gets a real separate diagnostics window. */

#define PEX_WIN32_LOGGY_UPDATE_SECONDS 0.10
#define PEX_WIN32_LOGGY_TEXT_CAP 196608
#define PEX_WIN32_LOGGY_EDIT_CAP 262144
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


static const char *loggy_screen_name(int screen) {
    switch (screen) {
        case SCREEN_TITLE: return "TITLE/main menu";
        case SCREEN_OPTIONS: return "OPTIONS";
        case SCREEN_OPTIONS_MORE: return "OPTIONS_MORE";
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
        case SCREEN_INGAME: return "INGAME";
        case SCREEN_PAUSE: return "PAUSE";
        case SCREEN_INVENTORY: return "INVENTORY";
        case SCREEN_WORKBENCH: return "WORKBENCH";
        case SCREEN_FURNACE: return "FURNACE";
        case SCREEN_CHEST: return "CHEST";
        case SCREEN_DEATH: return "DEATH";
        case SCREEN_CHAT: return "CHAT";
        case SCREEN_NOTICE: return "NOTICE";
        case SCREEN_RENDERER_RESTART_PROMPT: return "RENDERER_RESTART_PROMPT";
        case SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT: return "CLASSIC_PACK_DOWNLOAD_PROMPT";
        case SCREEN_CLASSIC_PACK_WARNING: return "CLASSIC_PACK_WARNING";
        default: return "UNKNOWN_SCREEN";
    }
}

static const char *loggy_focus_name(int focus) {
    switch (focus) {
        case PEX_INPUT_FOCUS_MOUSE: return "mouse";
        case PEX_INPUT_FOCUS_GAMEPAD: return "gamepad";
        case PEX_INPUT_FOCUS_TOUCH: return "touch";
        default: return "unknown";
    }
}

static double loggy_top_level_accounted_ms(void) {
    double ms = 0.0;
    ms += g_prof_display_ms[PROF_PUMP];
    ms += g_prof_display_ms[PROF_GAMEPAD_POLL];
    ms += g_prof_display_ms[PROF_NET_POLL];
    ms += g_prof_display_ms[PROF_CLOCK_DT];
    ms += g_prof_display_ms[PROF_TICK_TOTAL];
    ms += g_prof_display_ms[PROF_ASYNC_RENDER_PARTIAL];
    ms += g_prof_display_ms[PROF_NET_SMOOTHING];
    ms += g_prof_display_ms[PROF_ASYNC_TICK_PUMP];
    ms += g_prof_display_ms[PROF_RENDER_TOTAL];
    ms += g_prof_display_ms[PROF_SLEEP_LIMIT];
    ms += g_prof_display_ms[PROF_LOGGY_REFRESH];
    return ms;
}

static double loggy_sum_all_profile_ms(void) {
    double ms = 0.0;
    for (int i = 0; i < PROF_COUNT; ++i) ms += g_prof_display_ms[i];
    return ms;
}

static void loggy_append_phase(char **out, size_t *left, const char *name, PexMainThreadProfileId id) {
    loggy_appendf(out, left, "  %-24s avg=%8.3fms now=%8.3fms calls=%3d pct=%6.1f%%\n",
                  name, g_prof_display_ms[id], g_prof_frame_ms[id], g_loggy_profile_calls[id], profile_percent(g_prof_display_ms[id]));
}

static void loggy_append_hot_buckets(char **out, size_t *left) {
    int used[PROF_COUNT];
    memset(used, 0, sizeof(used));
    loggy_appendf(out, left, "\nHOTTEST PROFILE BUCKETS, SORTED BY AVERAGE:\n");
    for (int rank = 0; rank < 12; ++rank) {
        int best = -1;
        double best_ms = 0.0;
        for (int i = 0; i < PROF_COUNT; ++i) {
            if (used[i]) continue;
            if (g_prof_display_ms[i] > best_ms) { best_ms = g_prof_display_ms[i]; best = i; }
        }
        if (best < 0 || best_ms <= 0.0005) break;
        used[best] = 1;
        loggy_appendf(out, left, "  #%02d %-24s avg=%8.3fms now=%8.3fms calls=%3d pct=%6.1f%%\n",
                      rank + 1, g_prof_names[best], g_prof_display_ms[best], g_prof_frame_ms[best],
                      g_loggy_profile_calls[best], profile_percent(g_prof_display_ms[best]));
    }
}

static void loggy_append_root_hints(char **out, size_t *left, double top_accounted_ms, double unknown_ms) {
    int hints = 0;
    loggy_appendf(out, left, "\nROOT-CAUSE HINTS / WARNINGS:\n");
    if (g_opts.max_fps > 0) {
        double target = 1000.0 / (double)g_opts.max_fps;
        if (g_prof_display_frame_ms >= target * 0.85 || g_sleep_ms_last > 0.5 || g_prof_display_ms[PROF_SLEEP_LIMIT] > 0.5) {
            loggy_appendf(out, left, "  [CAP] Max FPS is %d. For 1k menu FPS, set Max FPS=Unlimited. limiter_sleep_avg=%.3fms last=%.3fms\n",
                          g_opts.max_fps, g_prof_display_ms[PROF_SLEEP_LIMIT], g_sleep_ms_last);
            hints++;
        }
    }
    if (g_opts.anaglyph && g_opts.max_fps > 0) {
        loggy_appendf(out, left, "  [VSYNC] V-Sync is ON and max_fps=%d. Present may block on monitor refresh.\n", g_opts.max_fps);
        hints++;
    }
    if (g_prof_display_ms[PROF_PRESENT] > 0.75) {
        loggy_appendf(out, left, "  [PRESENT] Present/swap is %.3fms avg. Suspect DXGI/DWM/back-buffer queue wait. flags=0x%x tearing=%d latency=%d set=%d hr=0x%08lx foreground=%d\n",
                      g_prof_display_ms[PROF_PRESENT], g_loggy_d3d11_present_flags, g_loggy_d3d11_allow_tearing,
                      g_loggy_d3d11_frame_latency, g_loggy_d3d11_frame_latency_set,
                      (unsigned long)g_loggy_d3d11_present_hr, GetForegroundWindow() == g_hwnd);
        hints++;
    }
    if (g_prof_display_ms[PROF_GAMEPAD_POLL] > 0.75) {
        loggy_appendf(out, left, "  [GAMEPAD] Gamepad poll costs %.3fms avg. XInput calls=%d ok=%d fail=%d skip=%d WinMM numdev=%d pos=%d ok=%d fail=%d caps=%d skip=%d.\n",
                      g_prof_display_ms[PROF_GAMEPAD_POLL], g_loggy_xinput_calls, g_loggy_xinput_ok,
                      g_loggy_xinput_fail, g_loggy_xinput_skipped, g_loggy_winmm_numdev_calls,
                      g_loggy_winmm_pos_calls, g_loggy_winmm_pos_ok, g_loggy_winmm_pos_fail,
                      g_loggy_winmm_caps_calls, g_loggy_winmm_slots_skipped);
        hints++;
    }
    if (unknown_ms > 1.0 && unknown_ms > top_accounted_ms * 0.25) {
        loggy_appendf(out, left, "  [UNKNOWN] %.3fms/frame is still outside top-level buckets. Add a profile wrapper around code between profile_begin_frame/profile_end_frame.\n", unknown_ms);
        hints++;
    }
    if (g_prof_display_ms[PROF_RENDER_TOTAL] < 1.0 && g_prof_display_frame_ms > 3.0) {
        loggy_appendf(out, left, "  [NOT GPU] Render total is only %.3fms while frame is %.3fms. Bottleneck is outside actual drawing.\n",
                      g_prof_display_ms[PROF_RENDER_TOTAL], g_prof_display_frame_ms);
        hints++;
    }
    if (g_prof_display_ms[PROF_DRAW_CURRENT_SCREEN] > 2.0 && g_screen == SCREEN_TITLE) {
        loggy_appendf(out, left, "  [MENU DRAW] Title/menu draw costs %.3fms avg. Check title background/panorama/buttons/text loops.\n", g_prof_display_ms[PROF_DRAW_CURRENT_SCREEN]);
        hints++;
    }
    if (g_prof_display_ms[PROF_WORLD_CLOUDS] > 1.0) {
        loggy_appendf(out, left, "  [CLOUDS] World clouds cost %.3fms avg. Fancy clouds should be batched; many tiny immediate batches will cap FPS.\n",
                      g_prof_display_ms[PROF_WORLD_CLOUDS]);
        hints++;
    }
    if (g_loggy_gui_text_calls > 1000 || g_loggy_gui_quads > 20000 || g_loggy_gui_buttons > 1000) {
        loggy_appendf(out, left, "  [GUI SPAM] This frame emits text_calls=%d quads=%d buttons=%d. If this stays high after reset, menu UI is rebuilding/drawing too much.\n",
                      g_loggy_gui_text_calls, g_loggy_gui_quads, g_loggy_gui_buttons);
        hints++;
    }
    if (g_prof_display_ms[PROF_LOGGY_REFRESH] > 1.0) {
        loggy_appendf(out, left, "  [LOGGY OVERHEAD] Loggy refresh costs %.3fms avg, last edit update %.3fms. Diagnostic window itself is affecting FPS.\n",
                      g_prof_display_ms[PROF_LOGGY_REFRESH], g_loggy_edit_refresh_last_ms);
        hints++;
    }
    if (g_prof_display_ms[PROF_MESH_SELF_HEAL] > 1.0) {
        loggy_appendf(out, left, "  [MESH SELF-HEAL] Self-heal safety scan costs %.3fms. It should be near 0 unless it is actively repairing missing meshes.\n", g_prof_display_ms[PROF_MESH_SELF_HEAL]);
        hints++;
    }
    if (g_prof_display_ms[PROF_MESH_MAIN] > 2.0) {
        loggy_appendf(out, left, "  [MESH MAIN] Mesh install/rebuild path costs %.3fms on main thread. Check mesh result install/self-heal/snapshot queues.\n", g_prof_display_ms[PROF_MESH_MAIN]);
        hints++;
    }
    if (g_screen == SCREEN_INGAME && g_prof_display_ms[PROF_WORLD_STREAM] > 1.0) {
        loggy_appendf(out, left, "  [STREAM] World streaming costs %.3fms on main thread. Check async queue/result install and chunk remap.\n", g_prof_display_ms[PROF_WORLD_STREAM]);
        hints++;
    }
    if (!hints) loggy_appendf(out, left, "  none obvious in current snapshot; use UNKNOWN and HOTTEST BUCKETS above.\n");
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
    DWORD pid = GetCurrentProcessId();
    DWORD tid = GetCurrentThreadId();
    DWORD priority = GetPriorityClass(GetCurrentProcess());
    HWND fg = GetForegroundWindow();

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

    double top_accounted_ms = loggy_top_level_accounted_ms();
    double all_bucket_sum_ms = loggy_sum_all_profile_ms();
    double unknown_ms = g_prof_display_frame_ms - top_accounted_ms;
    if (unknown_ms < 0.0) unknown_ms = 0.0;
    double budget_1000_over = g_prof_display_frame_ms - 1.0;
    if (budget_1000_over < 0.0) budget_1000_over = 0.0;

    loggy_appendf(&out, &left, "PEXCRAFT LOGGY ROOT-CAUSE SNAPSHOT\n");
    loggy_appendf(&out, &left, "NATIVE WIN32 HWND. ONE COPY SHOULD INCLUDE ENOUGH CONTEXT TO FIND THE BOTTLENECK.\n");
    loggy_appendf(&out, &left, "FRAME=%d SCREEN=%d/%s FPS=%d min=%d max=%d frame_avg=%.3fms last_wall=%.3fms 1k_budget_over=%.3fms\n",
                  g_loggy_frame_no, g_screen, loggy_screen_name(g_screen), g_debug_fps, g_debug_min_fps, g_debug_max_fps,
                  g_prof_display_frame_ms, g_loggy_last_frame_wall_ms, budget_1000_over);
    loggy_appendf(&out, &left, "ACCOUNTING: top_level=%.3fms unknown=%.3fms all_bucket_sum=%.3fms render_last=%.3fms sleep_last=%.3fms refresh_rate=%dHz\n",
                  top_accounted_ms, unknown_ms, all_bucket_sum_ms, g_render_ms_last, g_sleep_ms_last, g_system_info.display_refresh_hz);
    loggy_appendf(&out, &left, "CONFIG: renderer=%s(%d) selected=%s max_fps=%d vsync=%d fullscreen=%d fancy=%d render_distance=%d fov=%.1f scale=%d\n",
                  renderer_backend_label(g_runtime_renderer_backend), g_runtime_renderer_backend,
                  renderer_backend_label(g_selected_renderer_backend), g_opts.max_fps, g_opts.anaglyph,
                  g_opts.fullscreen, g_opts.fancy_graphics, g_opts.render_distance, g_opts.fov, g_gui_scale);
    loggy_appendf(&out, &left, "WINDOW: client=%dx%d gui=%dx%d render=%dx%d foreground=%d active_hwnd=%p loggy_hwnd=%p pid=%lu tid=%lu priority=0x%lx\n\n",
                  g_win_w, g_win_h, g_gui_w, g_gui_h, g_render_w, g_render_h,
                  (fg == g_hwnd) ? 1 : 0, (void*)g_hwnd, (void*)g_loggy.hwnd,
                  (unsigned long)pid, (unsigned long)tid, (unsigned long)priority);

    loggy_append_root_hints(&out, &left, top_accounted_ms, unknown_ms);
    loggy_append_hot_buckets(&out, &left);

    loggy_appendf(&out, &left, "\nTOP-LEVEL MAIN LOOP PHASES (do not double-count sub-buckets):\n");
    loggy_append_phase(&out, &left, "Pump/messages", PROF_PUMP);
    loggy_append_phase(&out, &left, "Gamepad poll", PROF_GAMEPAD_POLL);
    loggy_append_phase(&out, &left, "Net poll/apply", PROF_NET_POLL);
    loggy_append_phase(&out, &left, "Clock/delta", PROF_CLOCK_DT);
    loggy_append_phase(&out, &left, "Tick total", PROF_TICK_TOTAL);
    loggy_append_phase(&out, &left, "Async render partial", PROF_ASYNC_RENDER_PARTIAL);
    loggy_append_phase(&out, &left, "Net smoothing", PROF_NET_SMOOTHING);
    loggy_append_phase(&out, &left, "Async tick pump", PROF_ASYNC_TICK_PUMP);
    loggy_append_phase(&out, &left, "Render total", PROF_RENDER_TOTAL);
    loggy_append_phase(&out, &left, "FPS limiter sleep", PROF_SLEEP_LIMIT);
    loggy_append_phase(&out, &left, "Loggy refresh", PROF_LOGGY_REFRESH);
    loggy_appendf(&out, &left, "  %-24s avg=%8.3fms now=%8.3fms\n", "UNPROFILED/UNKNOWN", unknown_ms,
                  g_loggy_last_frame_wall_ms > 0.0 ? g_loggy_last_frame_wall_ms : g_ft_last_frame_ms);

    loggy_appendf(&out, &left, "\nALL PROFILE BUCKETS AVG/CALLS/CURRENT-FRAME (sub-buckets included; sum double-counts):\n");
    for (int i = 0; i < PROF_COUNT; ++i) {
        loggy_appendf(&out, &left, "  %-24s avg=%8.3fms now=%8.3fms calls=%3d pct=%6.1f%%\n",
                      g_prof_names[i], g_prof_display_ms[i], g_prof_frame_ms[i], g_loggy_profile_calls[i], profile_percent(g_prof_display_ms[i]));
    }

    loggy_appendf(&out, &left, "\nFRAME / TICK DETAILS:\n");
    loggy_appendf(&out, &left, "  dt=%.3fms tick_accum=%.3f ticks_this_frame=%d partial=%.3f total_ticks=%d world_time=%lld screen=%s\n",
                  g_loggy_dt_ms, g_loggy_tick_accum, g_loggy_ticks_this_frame, g_loggy_partial,
                  g_ticks, g_world_time, loggy_screen_name(g_screen));
    loggy_appendf(&out, &left, "  frame_history: last=%.3fms p50=%.3fms p95=%.3fms p99=%.3fms worst=%.3fms debug_counter=%d\n",
                  g_ft_last_frame_ms, g_ft_p50_ms, g_ft_p95_ms, g_ft_p99_ms, g_ft_worst_ms, g_debug_frame_counter);

    loggy_appendf(&out, &left, "\nWIN32 MESSAGE PUMP:\n");
    loggy_appendf(&out, &left, "  messages_this_frame=%d last_msg=0x%04x last_msg_time=%lu quit_seen=%d pump_avg=%.3fms pump_now=%.3fms\n",
                  g_loggy_win_msg_count, g_loggy_win_last_msg, (unsigned long)g_loggy_win_last_msg_time,
                  g_loggy_win_quit_seen, g_prof_display_ms[PROF_PUMP], g_prof_frame_ms[PROF_PUMP]);

    loggy_appendf(&out, &left, "\nGAMEPAD RAW POLL / INPUT FOCUS:\n");
    loggy_appendf(&out, &left, "  connected=%d primary=%d focus=%s poll_avg=%.3fms poll_now=%.3fms\n",
                  g_loggy_gamepad_connected, g_loggy_gamepad_primary, loggy_focus_name(g_loggy_gamepad_focus),
                  g_prof_display_ms[PROF_GAMEPAD_POLL], g_prof_frame_ms[PROF_GAMEPAD_POLL]);
    loggy_appendf(&out, &left, "  XInput: calls=%d ok=%d fail=%d skipped_empty=%d  WinMM: numdev=%d pos=%d ok=%d fail=%d caps=%d skipped_empty=%d\n",
                  g_loggy_xinput_calls, g_loggy_xinput_ok, g_loggy_xinput_fail, g_loggy_xinput_skipped,
                  g_loggy_winmm_numdev_calls, g_loggy_winmm_pos_calls, g_loggy_winmm_pos_ok,
                  g_loggy_winmm_pos_fail, g_loggy_winmm_caps_calls, g_loggy_winmm_slots_skipped);
    for (int gi = 0; gi < g_gamepad_count && gi < PEX_GAMEPAD_MAX; ++gi) {
        PexGamepadState *pad = &g_gamepads[gi];
        loggy_appendf(&out, &left, "  pad[%d] slot=%d name=%.64s kind=%.32s lx=%.2f ly=%.2f rx=%.2f ry=%.2f lt=%.2f rt=%.2f buttons=A%d B%d X%d Y%d LB%d RB%d Back%d Start%d D%d%d%d%d\n",
                      gi, pad->slot, pad->name, pad->kind, pad->lx, pad->ly, pad->rx, pad->ry, pad->lt, pad->rt,
                      pad->a, pad->b, pad->x, pad->y, pad->lb, pad->rb, pad->back, pad->start,
                      pad->dpad_up, pad->dpad_down, pad->dpad_left, pad->dpad_right);
    }

    loggy_appendf(&out, &left, "\nRENDER BREAKDOWN:\n");
    loggy_appendf(&out, &left, "  begin=%.3f clear_setup=%.3f draw_screen=%.3f cursor=%.3f fps_counter=%.3f present=%.3f total=%.3f\n",
                  g_prof_display_ms[PROF_RENDERER_BEGIN_FRAME], g_prof_display_ms[PROF_RENDER_CLEAR_SETUP],
                  g_prof_display_ms[PROF_DRAW_CURRENT_SCREEN], g_prof_display_ms[PROF_GAMEPAD_CURSOR],
                  g_prof_display_ms[PROF_FPS_COUNTER], g_prof_display_ms[PROF_PRESENT], g_prof_display_ms[PROF_RENDER_TOTAL]);
    loggy_appendf(&out, &left, "  world: cull=%.3f mesh_main=%.3f solid=%.3f entities=%.3f particles=%.3f translucent=%.3f overlays=%.3f clouds=%.3f hud_gui=%.3f\n",
                  g_prof_display_ms[PROF_CULL_SORT], g_prof_display_ms[PROF_MESH_MAIN], g_prof_display_ms[PROF_WORLD_DRAW],
                  g_prof_display_ms[PROF_WORLD_ENTITIES], g_prof_display_ms[PROF_WORLD_PARTICLES],
                  g_prof_display_ms[PROF_WORLD_TRANSLUCENT], g_prof_display_ms[PROF_WORLD_OVERLAYS],
                  g_prof_display_ms[PROF_WORLD_CLOUDS], g_prof_display_ms[PROF_HUD_GUI]);

    loggy_appendf(&out, &left, "\nD3D11 PRESENT STATE:\n");
    loggy_appendf(&out, &left, "  active=%d allow_tearing=%d swap_flags=0x%x present_flags=0x%x present_hr=0x%08lx failures=%d buffers=%d frame_latency=%d latency_set=%d stall_warn=%d\n",
                  g_loggy_d3d11_active, g_loggy_d3d11_allow_tearing, g_loggy_d3d11_swap_flags,
                  g_loggy_d3d11_present_flags, (unsigned long)g_loggy_d3d11_present_hr,
                  g_loggy_d3d11_present_failures, g_loggy_d3d11_buffer_count,
                  g_loggy_d3d11_frame_latency, g_loggy_d3d11_frame_latency_set,
                  g_loggy_d3d11_present_stall_warning);

    loggy_appendf(&out, &left, "\nGUI / TEXT / BUTTONS THIS FRAME:\n");
    loggy_appendf(&out, &left, "  text_calls=%d text_chars=%d gui_quads=%d buttons=%d debug_menu=%d chunk_info=%d task_info=%d button_count=%d mouse=%d,%d grabbed=%d hidden=%d\n",
                  g_loggy_gui_text_calls, g_loggy_gui_text_chars, g_loggy_gui_quads, g_loggy_gui_buttons,
                  g_debug_menu_shown, g_debug_chunk_info_shown, g_debug_task_info_shown,
                  g_button_count, g_mouse_x, g_mouse_y, g_mouse_grabbed, g_cursor_hidden);

    loggy_appendf(&out, &left, "\nRENDER / MESH STATE:\n");
    loggy_appendf(&out, &left, "  visible_sections=%d generated_chunks=%d fancy=%d direct_backend=%d display_lists=%d texture_terrain=%u font=%u title_logo=%u panorama0=%u\n",
                  g_loggy_visible_sections, g_loggy_visible_chunks, g_opts.fancy_graphics,
                  flat_direct_backend() ? 1 : 0, flat_display_lists_supported() ? 1 : 0,
                  tex_terrain.id, tex_font.id, tex_title_logo.id, tex_panorama[0].id);
    loggy_appendf(&out, &left, "  async_mesh_queue: jobs=%d busy=%d uploads=%d upload_busy=%d results=%d prof_jobs=%d prof_uploads=%d prof_results=%d\n",
                  mesh_jobs, mesh_busy, mesh_uploads, mesh_upload_busy, mesh_results,
                  g_prof_mesh_jobs_last, g_prof_mesh_uploads_last, g_prof_mesh_results_last);
    loggy_appendf(&out, &left, "  frame_mesh: submit_calls=%d snapshot_cells=%d installs=%d install_vtx=%d install_idx=%d stale_results=%d\n",
                  g_loggy_mesh_submit_calls, g_loggy_mesh_submit_snapshot_cells,
                  g_loggy_mesh_installs, g_loggy_mesh_install_vertices, g_loggy_mesh_install_indices,
                  g_loggy_mesh_stale_results);
    loggy_appendf(&out, &left, "  self_heal_probes=%d self_heal_missing=%d skylight_sub=%d light_dirty=%d light_ready=%d\n",
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
    loggy_appendf(&out, &left, "  player=(%.2f %.2f %.2f) prev=(%.2f %.2f %.2f) motion=(%.3f %.3f %.3f) yaw=%.2f pitch=%.2f on_ground=%d selected_slot=%d world_type=%d structures=%d\n",
                  g_player_x, g_player_y, g_player_z, g_player_prev_x, g_player_prev_y, g_player_prev_z,
                  g_player_motion_x, g_player_motion_y, g_player_motion_z, g_player_yaw, g_player_pitch,
                  g_player_on_ground, g_selected_hotbar_slot, g_world_type, g_world_map_features);
    loggy_appendf(&out, &left, "  falling active=%d spawned=%d cells=%d particles=%d drops=%d liquids_ms=%.3f world_stream_ms=%.3f\n",
                  g_prof_falling_active_last, g_prof_falling_spawns_last, g_prof_falling_cells_last,
                  active_particles, active_drops, g_prof_display_ms[PROF_LIQUIDS], g_prof_display_ms[PROF_WORLD_STREAM]);

    loggy_appendf(&out, &left, "\nSYSTEM INFO CACHE:\n");
    loggy_appendf(&out, &left, "  platform=%s display=%dx%d@%dHz %s ram=%llu/%lluMB gpu=%.96s vendor=%.64s api=%.64s net=%d\n",
                  g_system_info.platform, g_system_info.screen_w, g_system_info.screen_h, g_system_info.display_refresh_hz,
                  g_system_info.display_name, g_system_info.ram_available_mb, g_system_info.ram_total_mb,
                  g_system_info.gpu_renderer, g_system_info.gpu_vendor, g_system_info.gpu_version, g_system_info.network_available);

    loggy_appendf(&out, &left, "\nHOT QUEUE SAMPLE:\n");
    int maxq = q_remaining < 24 ? q_remaining : 24;
    for (int i = 0; i < maxq; ++i) {
        int qi = g_stream_gen_queue_index + i;
        loggy_appendf(&out, &left, "  stream_q[%02d] world_chunk=%d,%d score=%d\n", i,
                      g_stream_gen_queue_cx[qi], g_stream_gen_queue_cz[qi],
                      stream_chunk_priority_score(g_stream_gen_queue_cx[qi], g_stream_gen_queue_cz[qi]));
    }
    if (maxq <= 0) loggy_appendf(&out, &left, "  none\n");

    loggy_appendf(&out, &left, "\nCOPY / LOGGY WINDOW:\n");
    loggy_appendf(&out, &left, "  text_cap=%d edit_cap=%d line_count=%d edit_refreshes=%d edit_refresh_last=%.3fms clipboard_copies=%d update_interval=%.2fs\n",
                  PEX_WIN32_LOGGY_TEXT_CAP, PEX_WIN32_LOGGY_EDIT_CAP, g_loggy.line_count,
                  g_loggy_edit_refreshes, g_loggy_edit_refresh_last_ms, g_loggy_clipboard_copies,
                  PEX_WIN32_LOGGY_UPDATE_SECONDS);

    loggy_appendf(&out, &left, "\nNOTES:\n");
    loggy_appendf(&out, &left, "  ACCOUNTING top_level is the useful value. all_bucket_sum double-counts nested render/world buckets.\n");
    loggy_appendf(&out, &left, "  If frame is 80-100 FPS but render_total/present are tiny, the root is CPU, controller polling, frame cap, Loggy overhead, or UNKNOWN main-loop code.\n");
    loggy_appendf(&out, &left, "  This still cannot magically hook every C function; unknown>1ms means add profile wrappers around the remaining code path.\n");
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
    g_loggy_clipboard_copies++;
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
    double start_time;
    if (!g_loggy_enabled || !g_loggy.hwnd || !g_loggy.edit) return;
    start_time = now_seconds();
    loggy_build_text();
    loggy_set_edit_text_preserve_scroll();
    g_loggy.last_build_time = now_seconds();
    g_loggy_edit_refresh_last_ms = (g_loggy.last_build_time - start_time) * 1000.0;
    g_loggy_edit_refreshes++;
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
                double t_loggy_timer = profile_begin();
                loggy_refresh_now();
                profile_add_time(PROF_LOGGY_REFRESH, t_loggy_timer);
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

    /* Do not activate Loggy.  If the diagnostic window steals foreground focus,
       DWM/DXGI may throttle PexCraft's Present() and make the profiler lie. */
    g_loggy.hwnd = CreateWindowExA(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, wc.lpszClassName, "PexCraft Loggy",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, x, y, w, h, NULL, NULL, g_inst, NULL);
    if (!g_loggy.hwnd) {
        log_windows_error("CreateWindow native loggy");
        g_loggy_enabled = 0;
        return 0;
    }
    ShowWindow(g_loggy.hwnd, SW_SHOWNOACTIVATE);
    SetWindowPos(g_loggy.hwnd, HWND_TOP, x, y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    UpdateWindow(g_loggy.hwnd);
    if (g_hwnd) SetForegroundWindow(g_hwnd);
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
