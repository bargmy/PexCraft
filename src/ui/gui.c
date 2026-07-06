/* Split from original monolithic main.c. Included by src/main.c unity build. */


typedef struct PexProcessMemoryCounters {
    DWORD cb;
    DWORD PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} PexProcessMemoryCounters;

typedef BOOL (WINAPI *PexGetProcessMemoryInfoFn)(HANDLE, PexProcessMemoryCounters*, DWORD);

static void get_app_memory_mb(unsigned long long *used_mb, unsigned long long *peak_mb) {
    *used_mb = 0;
    *peak_mb = 0;

    static int resolved = 0;
    static PexGetProcessMemoryInfoFn fn = NULL;
    if (!resolved) {
        HMODULE psapi = LoadLibraryA("psapi.dll");
        if (!psapi) psapi = LoadLibraryA("kernel32.dll");
        if (psapi) fn = (PexGetProcessMemoryInfoFn)GetProcAddress(psapi, "GetProcessMemoryInfo");
        resolved = 1;
    }
    if (!fn) return;

    PexProcessMemoryCounters pmc;
    memset(&pmc, 0, sizeof(pmc));
    pmc.cb = sizeof(pmc);
    if (fn(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        *used_mb = (unsigned long long)(pmc.WorkingSetSize / (1024ULL * 1024ULL));
        *peak_mb = (unsigned long long)(pmc.PeakWorkingSetSize / (1024ULL * 1024ULL));
    }
}



static void draw_save_message(void) {
    if (g_save_message_ticks <= 0) return;
    draw_text("Saving game...", 8, g_gui_h - 16, 16777215);
}

typedef struct PexJavaRandom {
    uint64_t seed;
} PexJavaRandom;

static void pex_java_random_set_seed(PexJavaRandom *rng, int64_t seed) {
    rng->seed = ((uint64_t)seed ^ 0x5DEECE66DULL) & ((1ULL << 48) - 1ULL);
}

static int pex_java_random_next(PexJavaRandom *rng, int bits) {
    rng->seed = (rng->seed * 0x5DEECE66DULL + 0xBULL) & ((1ULL << 48) - 1ULL);
    return (int)(rng->seed >> (48 - bits));
}

static int pex_java_random_next_int(PexJavaRandom *rng, int bound) {
    if (bound <= 0) return 0;
    if ((bound & -bound) == bound) {
        return (int)(((int64_t)bound * (int64_t)pex_java_random_next(rng, 31)) >> 31);
    }
    int bits, val;
    do {
        bits = pex_java_random_next(rng, 31);
        val = bits % bound;
    } while (bits - val + (bound - 1) < 0);
    return val;
}

static void draw_chunk_debug_line(int x, int *y, const char *line, int color) {
    if (!y || !line) return;
    if (*y <= g_gui_h - 10) draw_text(line, x, *y, color);
    *y += 10;
}

static int debug_stream_queue_slot(int wcx, int wcz) {
    for (int i = 0; i < g_stream_gen_queue_count; ++i) {
        if (g_stream_gen_queue_cx[i] == wcx && g_stream_gen_queue_cz[i] == wcz) return i;
    }
    return -1;
}

static void draw_chunk_debug_panel(int x, int *y) {
    char line[256];

    int bx = (int)floorf(g_player_x);
    int by = (int)floorf(g_player_y);
    int bz = (int)floorf(g_player_z);
    int head_y = (int)floorf(g_player_y + 1.62f);
    int under_y = by - 1;

    int wcx = floor_div16(bx);
    int wcz = floor_div16(bz);
    int base_cx = floor_div16(g_flat_world_origin_x);
    int base_cz = floor_div16(g_flat_world_origin_z);
    int lcx = wcx - base_cx;
    int lcz = wcz - base_cz;
    int lx = bx - wcx * FLAT_RENDER_CHUNK;
    int lz = bz - wcz * FLAT_RENDER_CHUNK;
    int sy = flat_section_y_for_world(by);

    int local_ok = flat_local_chunk_valid(lcx, lcz);
    int section_ok = flat_section_index_valid(sy);
    int generated = local_ok ? g_flat_world_chunk_generated[lcz][lcx] : 0;
    int light_ready = local_ok ? g_flat_chunk_light_ready[lcz][lcx] : 0;
    unsigned int light_version = local_ok ? g_flat_chunk_light_version[lcz][lcx] : 0;
    unsigned short mask = local_ok ? g_flat_chunk_section_non_empty_mask[lcz][lcx] : 0;
    int qslot = debug_stream_queue_slot(wcx, wcz);
    int queue_state = (qslot < 0) ? 0 : ((qslot < g_stream_gen_queue_index) ? 2 : 1);

    int nonempty_sections = 0;
    int dirty_sections = 0;
    int valid_sections = 0;
    int building_sections = 0;
    int stale_light_sections = 0;
    if (local_ok) {
        for (int s = 0; s < FLAT_RENDER_SECTIONS_Y; ++s) {
            int nonempty = (mask & (unsigned short)(1u << s)) != 0;
            if (nonempty) nonempty_sections++;
            if (g_flat_section_dirty[s][lcz][lcx]) dirty_sections++;
            if (g_flat_section_valid[s][lcz][lcx]) valid_sections++;
            if (g_flat_section_mesh_building[s][lcz][lcx]) building_sections++;
            if (nonempty && g_flat_section_mesh_light_version[s][lcz][lcx] != light_version) stale_light_sections++;
        }
    }

    int top_y = -999;
    if (generated && lx >= 0 && lx < FLAT_RENDER_CHUNK && lz >= 0 && lz < FLAT_RENDER_CHUNK) {
        for (int ty = FLAT_WORLD_Y_MAX; ty >= FLAT_WORLD_Y_MIN; --ty) {
            if (flat_get_block(bx, ty, bz) != 0) {
                top_y = ty;
                break;
            }
        }
    }

    draw_chunk_debug_line(x, y, "Chunk under player (F3+V):", 0xE0E0E0);

    snprintf(line, sizeof(line), "world chunk %d,%d  local %d,%d  block %d,%d,%d", wcx, wcz, lcx, lcz, bx, by, bz);
    draw_chunk_debug_line(x, y, line, 14737632);

    snprintf(line, sizeof(line), "local block %d,%d  section %d  in window %d  generated %d", lx, lz, sy, local_ok, generated);
    draw_chunk_debug_line(x, y, line, local_ok && generated ? 14737632 : 0xFF8080);

    if (!local_ok) {
        snprintf(line, sizeof(line), "active origin %d,%d  base chunk %d,%d", g_flat_world_origin_x, g_flat_world_origin_z, base_cx, base_cz);
        draw_chunk_debug_line(x, y, line, 0xFF8080);
        return;
    }

    snprintf(line, sizeof(line), "spawn preload %d  modified %d  lightReady %d  lightVer %u",
             g_flat_chunk_initial_preload[lcz][lcx], g_flat_world_chunk_modified[lcz][lcx],
             light_ready, light_version);
    draw_chunk_debug_line(x, y, line, light_ready ? 14737632 : 0xFFFF80);

    snprintf(line, sizeof(line), "chunk dirty %d valid %d hasLiquid %d envDue %d occ %04x",
             g_flat_world_chunk_dirty[lcz][lcx], g_flat_world_chunk_valid[lcz][lcx],
             g_flat_world_chunk_has_liquid[lcz][lcx], g_flat_chunk_environment_light_due[lcz][lcx],
             (unsigned)mask);
    draw_chunk_debug_line(x, y, line, 14737632);

    snprintf(line, sizeof(line), "sections nonempty %d dirty %d valid %d building %d staleLight %d",
             nonempty_sections, dirty_sections, valid_sections, building_sections, stale_light_sections);
    draw_chunk_debug_line(x, y, line, stale_light_sections ? 0xFF8080 : 14737632);

    if (section_ok) {
        int sec_nonempty = (mask & (unsigned short)(1u << sy)) != 0;
        int sec_stale = (sec_nonempty && g_flat_section_mesh_light_version[sy][lcz][lcx] != light_version) ? 1 : 0;
        snprintf(line, sizeof(line), "current section: nonempty %d dirty %d valid %d building %d stale %d",
                 sec_nonempty, g_flat_section_dirty[sy][lcz][lcx], g_flat_section_valid[sy][lcz][lcx],
                 g_flat_section_mesh_building[sy][lcz][lcx], sec_stale);
        draw_chunk_debug_line(x, y, line, sec_stale ? 0xFF8080 : 14737632);

        snprintf(line, sizeof(line), "meshVer %u meshLight %u skip %d/%d list %u/%u dmesh %u/%u",
                 g_flat_section_mesh_version[sy][lcz][lcx],
                 g_flat_section_mesh_light_version[sy][lcz][lcx],
                 g_flat_section_skip_pass[sy][lcz][lcx][0],
                 g_flat_section_skip_pass[sy][lcz][lcx][1],
                 (unsigned)g_flat_section_lists[sy][lcz][lcx][0],
                 (unsigned)g_flat_section_lists[sy][lcz][lcx][1],
                 g_flat_section_direct_mesh[sy][lcz][lcx][0],
                 g_flat_section_direct_mesh[sy][lcz][lcx][1]);
        draw_chunk_debug_line(x, y, line, sec_stale ? 0xFF8080 : 14737632);
    }

    snprintf(line, sizeof(line), "blocks under/feet/head %d/%d/%d  topY %d",
             flat_get_block(bx, under_y, bz), flat_get_block(bx, by, bz),
             flat_get_block(bx, head_y, bz), top_y);
    draw_chunk_debug_line(x, y, line, 14737632);

    snprintf(line, sizeof(line), "sky under/feet/head %d/%d/%d  blockLight %d/%d/%d",
             flat_get_sky_light(bx, under_y, bz), flat_get_sky_light(bx, by, bz),
             flat_get_sky_light(bx, head_y, bz), flat_get_block_light(bx, under_y, bz),
             flat_get_block_light(bx, by, bz), flat_get_block_light(bx, head_y, bz));
    draw_chunk_debug_line(x, y, line, light_ready ? 14737632 : 0xFFFF80);

    snprintf(line, sizeof(line), "queue %s slot %d/%d index %d installed %d keepInit %d",
             queue_state == 0 ? "no" : (queue_state == 1 ? "queued" : "passed"),
             qslot, g_stream_gen_queue_count, g_stream_gen_queue_index,
             g_stream_gen_queue_installed_count, g_stream_generation_keep_completed);
    draw_chunk_debug_line(x, y, line, 14737632);

    snprintf(line, sizeof(line), "async workers %d jobs %d active %d results %d streamBusy %d lightDirty %d lightBusy %d",
             g_stream_async_worker_count, g_stream_async_job_count, g_stream_async_active_count,
             g_stream_async_result_count, g_world_stream_service_busy,
             flat_lighting_pending_dirty(), g_flat_lighting_worker_busy);
    draw_chunk_debug_line(x, y, line, 14737632);

    snprintf(line, sizeof(line), "initial batch run %d %d/%d  lightSettle req/run/done %d/%d/%d p%d",
             g_stream_initial_batch_running, g_stream_initial_batch_done_units,
             g_stream_initial_batch_total_units, g_stream_initial_light_settle_requested,
             g_stream_initial_light_settle_running, g_stream_initial_light_settle_done,
             g_stream_initial_light_settle_progress);
    draw_chunk_debug_line(x, y, line, 14737632);
}

static void draw_profile_debug_panel(void) {
    char line[256];
    int fps = g_debug_fps;
    int min_fps = g_debug_min_fps ? g_debug_min_fps : fps;
    int max_fps = g_debug_max_fps ? g_debug_max_fps : fps;
    int y = g_opts.show_fps ? 22 : 12;
    int x = 2;
    int right_x = g_gui_w / 2;
    if (right_x < 430) right_x = 430;

    draw_text("Task profiler (F3+J)", x, y, 0xE0E0E0); y += 10;
    snprintf(line, sizeof(line), "FPS %d/%d/%d  Frame %.2fms avg %.2fms p95 %.2f p99 %.2f worst %.2f",
             fps, min_fps, max_fps, g_ft_last_frame_ms, g_prof_display_frame_ms,
             g_ft_p95_ms, g_ft_p99_ms, g_ft_worst_ms);
    draw_text(line, x, y, 14737632); y += 10;
    snprintf(line, sizeof(line), "Backend %s  render %.2fms  sleep %.2fms  skylightSub %d",
             renderer_backend_label(g_runtime_renderer_backend), g_render_ms_last,
             g_sleep_ms_last, g_prof_skylight_subtracted_last);
    draw_text(line, x, y, 14737632); y += 12;

    draw_text("Main/render thread tasks:", x, y, 0xE0E0E0); y += 10;
    for (int pi = 0; pi < PROF_COUNT; ++pi) {
        double ms = g_prof_display_ms[pi];
        snprintf(line, sizeof(line), "%02d %-18s %6.2fms %5.1f%%",
                 pi, g_prof_names[pi], ms, profile_percent(ms));
        draw_text(line, x, y, ms >= 1.0 ? 0xFFFF80 : 14737632);
        y += 10;
        if (y > g_gui_h - 12) break;
    }

    int ry = g_opts.show_fps ? 22 : 12;
    draw_text("Worker / queue tasks:", right_x, ry, 0xE0E0E0); ry += 10;
    snprintf(line, sizeof(line), "Async tick       last %.2fms avg %.2fms pending %d busy %d dropped %d",
             ingame_tick_async_last_ms(), ingame_tick_async_avg_ms(),
             ingame_tick_async_pending_count(), ingame_tick_async_busy(),
             ingame_tick_async_dropped_count());
    draw_text(line, right_x, ry, 14737632); ry += 10;
    snprintf(line, sizeof(line), "Stream service   last %.2fms avg %.2fms busy %d pending %d",
             g_prof_stream_worker_last_ms, g_prof_stream_worker_avg_ms,
             g_world_stream_service_busy, g_prof_stream_pending_last);
    draw_text(line, right_x, ry, g_prof_stream_worker_avg_ms >= 2.0 ? 0xFFFF80 : 14737632); ry += 10;
    snprintf(line, sizeof(line), "Lighting worker  last %.2fms avg %.2fms dirty %d busy %d",
             g_prof_light_worker_last_ms, g_prof_light_worker_avg_ms,
             flat_lighting_pending_dirty(), g_flat_lighting_worker_busy);
    draw_text(line, right_x, ry, g_prof_light_worker_avg_ms >= 2.0 ? 0xFFFF80 : 14737632); ry += 10;
    snprintf(line, sizeof(line), "Mesh worker      last %.2fms avg %.2fms jobs %d results %d",
             g_prof_mesh_worker_last_ms, g_prof_mesh_worker_avg_ms,
             g_prof_mesh_jobs_last, g_prof_mesh_results_last);
    draw_text(line, right_x, ry, g_prof_mesh_worker_avg_ms >= 2.0 ? 0xFFFF80 : 14737632); ry += 10;
    snprintf(line, sizeof(line), "Mesh upload/pre  last %.2fms avg %.2fms uploads %d",
             g_prof_mesh_upload_worker_last_ms, g_prof_mesh_upload_worker_avg_ms,
             g_prof_mesh_uploads_last);
    draw_text(line, right_x, ry, g_prof_mesh_upload_worker_avg_ms >= 2.0 ? 0xFFFF80 : 14737632); ry += 12;

    draw_text("Queues / renderer state:", right_x, ry, 0xE0E0E0); ry += 10;
    snprintf(line, sizeof(line), "Stream queue %d index %d installed %d active %d",
             g_stream_gen_queue_count, g_stream_gen_queue_index,
             g_stream_gen_queue_installed_count, stream_generation_active());
    draw_text(line, right_x, ry, 14737632); ry += 10;
    snprintf(line, sizeof(line), "Stream async workers %d jobs %d active %d results %d",
             g_stream_async_worker_count, g_stream_async_job_count,
             g_stream_async_active_count, g_stream_async_result_count);
    draw_text(line, right_x, ry, 14737632); ry += 10;
    snprintf(line, sizeof(line), "Mesh queues jobs %d uploads %d results %d",
             g_prof_mesh_jobs_last, g_prof_mesh_uploads_last, g_prof_mesh_results_last);
    draw_text(line, right_x, ry, 14737632); ry += 10;
    {
        int edit_age = g_ingame_ticks - g_flat_recent_block_mesh_dirty_tick;
        if (edit_age < 0 || edit_age > 999) edit_age = 999;
        snprintf(line, sizeof(line), "Edit mesh boost age %d  lightmap dark %.2f",
                 edit_age, world_lightmap_dark_factor());
        draw_text(line, right_x, ry, edit_age <= 12 ? 0xFFFF80 : 14737632); ry += 10;
    }
    snprintf(line, sizeof(line), "Generated preload batch %d %d/%d light settle %d/%d/%d p%d",
             g_stream_initial_batch_running, g_stream_initial_batch_done_units,
             g_stream_initial_batch_total_units, g_stream_initial_light_settle_requested,
             g_stream_initial_light_settle_running, g_stream_initial_light_settle_done,
             g_stream_initial_light_settle_progress);
    draw_text(line, right_x, ry, 14737632); ry += 10;
    snprintf(line, sizeof(line), "Falling active %d wakeups %d spawned %d",
             g_prof_falling_active_last, g_prof_falling_cells_last,
             g_prof_falling_spawns_last);
    draw_text(line, right_x, ry, 14737632);
}


static int pex_map_gui_color(unsigned char v) {
    static const int base[32] = {
        0x000000, 0x7FB238, 0xF7E9A3, 0xA7A7A7, 0xFF0000, 0xA0A0FF, 0xA7A7A7, 0x007C00,
        0xFFFFFF, 0xA4A8B8, 0x976D4D, 0x707070, 0x4040FF, 0x8F7748, 0xFFFCF5, 0xD87F33,
        0xB24CD8, 0x6699D8, 0xE5E533, 0x7FCC19, 0xF27FA5, 0x4C4C4C, 0x999999, 0x4C7F99,
        0x7F3FB2, 0x334CB2, 0x664C33, 0x667F33, 0x993333, 0x191919, 0xFAEE4D, 0x5CDBD5
    };
    int idx = (v >> 2) & 31;
    int shade = v & 3;
    int c = base[idx];
    if (idx == 0) return 0x000000;
    int mult = shade == 0 ? 180 : (shade == 1 ? 220 : (shade == 2 ? 255 : 135));
    int r = ((c >> 16) & 255) * mult / 255;
    int g = ((c >> 8) & 255) * mult / 255;
    int b = (c & 255) * mult / 255;
    return (r << 16) | (g << 8) | b;
}


static int pex_hsb_to_rgb(float h, float sat, float bri) {
    while (h < 0.0f) h += 1.0f;
    while (h >= 1.0f) h -= 1.0f;
    if (sat < 0.0f) sat = 0.0f; if (sat > 1.0f) sat = 1.0f;
    if (bri < 0.0f) bri = 0.0f; if (bri > 1.0f) bri = 1.0f;
    float r = bri, g = bri, b = bri;
    if (sat > 0.0f) {
        float hf = h * 6.0f;
        int i = (int)floorf(hf);
        float f = hf - (float)i;
        float p = bri * (1.0f - sat);
        float q = bri * (1.0f - sat * f);
        float t = bri * (1.0f - sat * (1.0f - f));
        switch (i % 6) {
            case 0: r = bri; g = t; b = p; break;
            case 1: r = q; g = bri; b = p; break;
            case 2: r = p; g = bri; b = t; break;
            case 3: r = p; g = q; b = bri; break;
            case 4: r = t; g = p; b = bri; break;
            default: r = bri; g = p; b = q; break;
        }
    }
    int ri = (int)(r * 255.0f + 0.5f), gi = (int)(g * 255.0f + 0.5f), bi = (int)(b * 255.0f + 0.5f);
    if (ri < 0) ri = 0; if (ri > 255) ri = 255;
    if (gi < 0) gi = 0; if (gi > 255) gi = 255;
    if (bi < 0) bi = 0; if (bi > 255) bi = 255;
    return (ri << 16) | (gi << 8) | bi;
}

static void draw_record_playing_overlay(void) {
    if (g_record_playing_up_for <= 0 || !g_record_playing_text[0]) return;
    float up = (float)g_record_playing_up_for - g_frame_partial;
    int alpha = (int)(up * 256.0f / 20.0f);
    if (alpha > 255) alpha = 255;
    if (alpha <= 0) return;
    int rgb = 0xFFFFFF;
    if (g_record_is_playing) rgb = pex_hsb_to_rgb(up / 50.0f, 0.7f, 0.6f);
    int color = (alpha << 24) | rgb;
    int x = (g_gui_w - text_width(g_record_playing_text)) / 2;
    int y = g_gui_h - 48 - 4;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw_text_no_shadow(g_record_playing_text, x, y, color);
}

static void draw_held_map_overlay(void) {
    if (g_third_person_view) return;
    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    if (stack_empty(held) || held->id != ITEM_MAP) return;
    PexMapData *m = pex_map_find(held->damage, 0);
    if (!m || !m->active) return;
    int x0 = g_gui_w / 2 - 68;
    int y0 = g_gui_h / 2 - 68;
    draw_rect(x0 - 4, y0 - 4, x0 + 132, y0 + 132, 0xFFB69D73);
    draw_rect(x0, y0, x0 + 128, y0 + 128, 0xFFE0D8B0);
    for (int z = 0; z < 128; ++z) {
        for (int x = 0; x < 128; ++x) {
            unsigned char v = m->colors[x + z * 128];
            if (v == 0) continue;
            draw_rect(x0 + x, y0 + z, x0 + x + 1, y0 + z + 1, 0xFF000000 | pex_map_gui_color(v));
        }
    }
    int scale = 1 << (m->scale & 7);
    int px = ((int)floorf(g_player_x) - m->x_center) / scale + 64;
    int pz = ((int)floorf(g_player_z) - m->z_center) / scale + 64;
    if (px >= 0 && px < 128 && pz >= 0 && pz < 128) {
        float yaw = g_player_yaw * (float)M_PI / 180.0f;
        int fx = (int)roundf(-sinf(yaw) * 4.0f);
        int fz = (int)roundf(cosf(yaw) * 4.0f);
        int rx = (int)roundf(cosf(yaw) * 2.0f);
        int rz = (int)roundf(sinf(yaw) * 2.0f);
        draw_rect(x0 + px - 1, y0 + pz - 1, x0 + px + 2, y0 + pz + 2, 0xFF000000);
        draw_rect(x0 + px + fx - 1, y0 + pz + fz - 1, x0 + px + fx + 2, y0 + pz + fz + 2, 0xFFFFFFFF);
        draw_rect(x0 + px + rx - 1, y0 + pz + rz - 1, x0 + px + rx + 1, y0 + pz + rz + 1, 0xFFFFFFFF);
        draw_rect(x0 + px - rx - 1, y0 + pz - rz - 1, x0 + px - rx + 1, y0 + pz - rz + 1, 0xFFFFFFFF);
    }
}

static void draw_hud(void) {
    pex_sound_tick_record_stream();
    pex_game_music_tick();
    int w = g_gui_w;
    int h = g_gui_h;
    int hotbar_x = w / 2 - 91;
    int hotbar_y = h - 22;
    draw_textured_rect_256(&tex_gui, hotbar_x, hotbar_y, 0, 0, 182, 22, 0xFFFFFF);
    draw_textured_rect_256(&tex_gui, hotbar_x - 1 + g_selected_hotbar_slot * 20, hotbar_y - 1, 0, 22, 24, 22, 0xFFFFFF);

#if defined(GL_ONE_MINUS_DST_COLOR) && defined(GL_ONE_MINUS_SRC_COLOR)
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
#endif
    if (tex_icons.id && tex_icons.w > 0 && tex_icons.h > 0) {
        glBindTexture(GL_TEXTURE_2D, tex_icons.id);
        glColor4f(1,1,1,1);
        const float us = 1.0f / 256.0f;
        const float vs = 1.0f / 256.0f;
        float u0 = 0.0f * us;
        float v0 = 0.0f * vs;
        float u1 = 16.0f * us;
        float v1 = 16.0f * vs;
        int cx = w / 2 - 7;
        int cy = h / 2 - 7;
        glBegin(GL_QUADS);
        glTexCoord2f(u0, v1); glVertex3f((float)cx, (float)(cy + 16), 0.0f);
        glTexCoord2f(u1, v1); glVertex3f((float)(cx + 16), (float)(cy + 16), 0.0f);
        glTexCoord2f(u1, v0); glVertex3f((float)(cx + 16), (float)cy, 0.0f);
        glTexCoord2f(u0, v0); glVertex3f((float)cx, (float)cy, 0.0f);
        glEnd();
    }
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int left = w / 2 - 91;
    int right = w / 2 + 91;
    int row_y = h - 39;
    int upper_y = row_y - 10;

    /* Java 1.2.5 GuiIngame gates health/armor/food/air/XP behind
       PlayerController.shouldDrawHUD(); PlayerControllerCreative returns false.
       Keep the hotbar and crosshair, but hide survival HUD in creative. */
    if (!player_is_creative()) {
        int hp = player_health_clamp(g_player_health);
        int prev_hp = player_health_clamp(g_player_prev_health);
        int flash_old = ((g_hearts_life / 3) % 2) == 1;
        if (g_hearts_life < 10) flash_old = 0;
        int armor = g_player_armor;
        if (armor < 0) armor = 0;
        if (armor > 20) armor = 20;
        PexJavaRandom heart_rng;
        pex_java_random_set_seed(&heart_rng, (int64_t)g_ingame_ticks * 312871LL);

        for (int i = 0; i < 10; i++) {
            if (armor > 0) {
                int ax = left + i * 8;
                if (i * 2 + 1 < armor) draw_textured_rect_256(&tex_icons, ax, upper_y, 34, 9, 9, 9, 0xFFFFFF);
                else if (i * 2 + 1 == armor) draw_textured_rect_256(&tex_icons, ax, upper_y, 25, 9, 9, 9, 0xFFFFFF);
                else draw_textured_rect_256(&tex_icons, ax, upper_y, 16, 9, 9, 9, 0xFFFFFF);
            }

            int x = left + i * 8;
            int y = row_y;
            if (hp <= 4 && hp > 0) {
                y += pex_java_random_next_int(&heart_rng, 2);
            }
            draw_textured_rect_256(&tex_icons, x, y, 16 + (flash_old ? 9 : 0), 0, 9, 9, 0xFFFFFF);
            if (flash_old) {
                if (i * 2 + 1 < prev_hp) draw_textured_rect_256(&tex_icons, x, y, 70, 0, 9, 9, 0xFFFFFF);
                else if (i * 2 + 1 == prev_hp) draw_textured_rect_256(&tex_icons, x, y, 79, 0, 9, 9, 0xFFFFFF);
            }
            if (i * 2 + 1 < hp) draw_textured_rect_256(&tex_icons, x, y, 52, 0, 9, 9, 0xFFFFFF);
            else if (i * 2 + 1 == hp) draw_textured_rect_256(&tex_icons, x, y, 61, 0, 9, 9, 0xFFFFFF);
        }

        if (tex_icons.id && tex_icons.w > 0 && tex_icons.h > 0) {
            int food = player_food_clamp(g_player_food_level);
            int prev_food = player_food_clamp(g_player_prev_food_level);
            int food_flash = 0; /* GuiIngame 1.2.5 leaves this false unless the later highlight path toggles it. */
            for (int i = 0; i < 10; i++) {
                int x = right - i * 8 - 9;
                int y = row_y;
                int icon_base = 16;
                int container_offset = food_flash ? 1 : 0;
                if (g_player_food_saturation <= 0.0f && food > 0 && (g_ingame_ticks % (food * 3 + 1)) == 0) {
                    y += pex_java_random_next_int(&heart_rng, 3) - 1;
                }
                draw_textured_rect_256(&tex_icons, x, y, 16 + container_offset * 9, 27, 9, 9, 0xFFFFFF);
                if (food_flash) {
                    if (i * 2 + 1 < prev_food) draw_textured_rect_256(&tex_icons, x, y, icon_base + 54, 27, 9, 9, 0xFFFFFF);
                    else if (i * 2 + 1 == prev_food) draw_textured_rect_256(&tex_icons, x, y, icon_base + 63, 27, 9, 9, 0xFFFFFF);
                }
                if (i * 2 + 1 < food) draw_textured_rect_256(&tex_icons, x, y, icon_base + 36, 27, 9, 9, 0xFFFFFF);
                else if (i * 2 + 1 == food) draw_textured_rect_256(&tex_icons, x, y, icon_base + 45, 27, 9, 9, 0xFFFFFF);
            }
        }

        if (tex_icons.id && tex_icons.w > 0 && tex_icons.h > 0 && flat_player_head_in_water()) {
            int air = g_player_air;
            if (air < 0) air = 0;
            if (air > 300) air = 300;
            int full = (int)ceil((double)(air - 2) * 10.0 / 300.0);
            int partial = (int)ceil((double)air * 10.0 / 300.0) - full;
            if (full < 0) full = 0;
            if (partial < 0) partial = 0;
            for (int i = 0; i < full + partial && i < 10; i++) {
                draw_textured_rect_256(&tex_icons, right - i * 8 - 9, upper_y, i < full ? 16 : 25, 18, 9, 9, 0xFFFFFF);
            }
        }

        int xp_cap = player_xp_bar_cap();
        if (tex_icons.id && tex_icons.w > 0 && tex_icons.h > 0 && xp_cap > 0) {
            int fill = (int)(g_player_xp_progress * 183.0f);
            if (fill < 0) fill = 0;
            if (fill > 183) fill = 183;
            int xp_y = h - 32 + 3;
            draw_textured_rect_256(&tex_icons, left, xp_y, 0, 64, 182, 5, 0xFFFFFF);
            if (fill > 0) draw_textured_rect_256(&tex_icons, left, xp_y, 0, 69, fill, 5, 0xFFFFFF);
        }
        if (g_player_xp_level > 0) {
            char lvl[16];
            snprintf(lvl, sizeof(lvl), "%d", g_player_xp_level);
            int lx = (w - text_width(lvl)) / 2;
            int ly = h - 31 - 4;
            draw_text_no_shadow(lvl, lx + 1, ly, 0);
            draw_text_no_shadow(lvl, lx - 1, ly, 0);
            draw_text_no_shadow(lvl, lx, ly + 1, 0);
            draw_text_no_shadow(lvl, lx, ly - 1, 0);
            draw_text_no_shadow(lvl, lx, ly, 0x80FF20);
        }
    }

    draw_held_map_overlay();
    draw_record_playing_overlay();

    for (int i = 0; i < 9; i++) draw_item_stack_gui_animated(&g_inventory[i], hotbar_x + 3 + i * 20, hotbar_y + 3);

    draw_text(VERSION_TEXT, 2, 2, 16777215);
    if (g_debug_menu_shown && g_debug_task_info_shown) {
        draw_profile_debug_panel();
    } else if (g_debug_menu_shown) {
        char line[160];
        int fps = g_debug_fps;
        int min_fps = g_debug_min_fps ? g_debug_min_fps : fps;
        int max_fps = g_debug_max_fps ? g_debug_max_fps : fps;

        int y = g_opts.show_fps ? 22 : 12;
        snprintf(line, sizeof(line), "FPS %d/%d/%d", fps, min_fps, max_fps);
        draw_text(line, 2, y, 14737632); y += 10;

        snprintf(line, sizeof(line), "Position %.2f %.2f %.2f", g_player_x, g_player_y, g_player_z);
        draw_text(line, 2, y, 14737632); y += 10;

        static unsigned long long used_mb = 0, peak_mb = 0;
        static double last_memory_update = 0.0;
        double memory_now = now_seconds();
        if (memory_now - last_memory_update >= 0.25 || last_memory_update <= 0.0) {
            get_app_memory_mb(&used_mb, &peak_mb);
            last_memory_update = memory_now;
        }
        snprintf(line, sizeof(line), "Memory usage %lluMB/%lluMB", used_mb, peak_mb);
        draw_text(line, 2, y, 14737632); y += 10;

        snprintf(line, sizeof(line), "Frame %.1fms avg %.1fms p50 %.1f p95 %.1f p99 %.1f",
                 g_ft_last_frame_ms, g_prof_display_frame_ms, g_ft_p50_ms, g_ft_p95_ms, g_ft_p99_ms);
        draw_text(line, 2, y, 14737632); y += 10;

        snprintf(line, sizeof(line), "Worst %.1fms  Backend %s", g_ft_worst_ms, renderer_backend_label(g_runtime_renderer_backend));
        draw_text(line, 2, y, 14737632); y += 10;

        if (g_opts.max_fps > 0) {
            snprintf(line, sizeof(line), "Target %.1fms (%dfps cap)  Sleep %.1fms",
                     1000.0 / (double)g_opts.max_fps, g_opts.max_fps, g_sleep_ms_last);
        } else {
            snprintf(line, sizeof(line), "Target uncapped  Sleep %.1fms", g_sleep_ms_last);
        }
        draw_text(line, 2, y, 14737632); y += 12;

        draw_text("Main thread average per frame:", 2, y, 0xE0E0E0); y += 10;
        for (int pi = 0; pi < PROF_COUNT; pi++) {
            double ms = g_prof_display_ms[pi];
            if (ms < 0.01 && pi != PROF_PUMP && pi != PROF_NET_POLL && pi != PROF_TICK_TOTAL && pi != PROF_RENDER_TOTAL) continue;
            snprintf(line, sizeof(line), "%-16s %5.2fms %4.0f%%", g_prof_names[pi], ms, profile_percent(ms));
            draw_text(line, 2, y, 14737632);
            y += 10;
            if (y > g_gui_h - 70) break;
        }

        int right_x = g_gui_w - (g_debug_chunk_info_shown ? 380 : 210);
        int ry = g_opts.show_fps ? 22 : 12;
        if (right_x < 220) right_x = 220;
        draw_text("Queues / async state:", right_x, ry, 0xE0E0E0); ry += 10;
        snprintf(line, sizeof(line), "Net packets %d  chunks %d  queued %d", g_prof_packets_last, g_prof_chunks_last, g_prof_rx_queue_last);
        draw_text(line, right_x, ry, 14737632); ry += 10;
        snprintf(line, sizeof(line), "Mesh jobs %d  uploads %d  results %d", g_prof_mesh_jobs_last, g_prof_mesh_uploads_last, g_prof_mesh_results_last);
        draw_text(line, right_x, ry, 14737632); ry += 10;
        snprintf(line, sizeof(line), "Falling active %d  wakeups %d  spawned %d", g_prof_falling_active_last, g_prof_falling_cells_last, g_prof_falling_spawns_last);
        draw_text(line, right_x, ry, 14737632); ry += 10;
        snprintf(line, sizeof(line), "Stream pending %d", g_prof_stream_pending_last);
        draw_text(line, right_x, ry, 14737632); ry += 10;
        snprintf(line, sizeof(line), "Async tick pending %d  busy %d  dropped %d",
                 ingame_tick_async_pending_count(), ingame_tick_async_busy(), ingame_tick_async_dropped_count());
        draw_text(line, right_x, ry, 14737632); ry += 10;
        snprintf(line, sizeof(line), "Async tick %.2fms avg %.2fms",
                 ingame_tick_async_last_ms(), ingame_tick_async_avg_ms());
        draw_text(line, right_x, ry, 14737632); ry += 12;

        if (g_debug_chunk_info_shown) {
            draw_chunk_debug_panel(right_x, &ry);
        } else {
            draw_text("F3+V: chunk info off  F3+J: task profiler", right_x, ry, 0xA0A0A0);
        }
    }
    draw_chat_lines(g_screen == SCREEN_CHAT);
    draw_save_message();
}

static void draw_ingame_screen(void) {
    draw_ingame_world_view(1);
    double prof_gui = profile_begin();
    draw_hud();
    profile_add_time(PROF_HUD_GUI, prof_gui);
}

static void draw_pause_screen(void) {
    draw_ingame_world_view(1);
    double prof_gui = profile_begin();
    draw_gradient(0, 0, g_gui_w, g_gui_h, -1072689136, -804253680);
    draw_save_message();
    draw_centered_text(tr_key_default("menu.game", "Game menu"), g_gui_w / 2, 40, 16777215);
    draw_all_buttons();
    profile_add_time(PROF_HUD_GUI, prof_gui);
}

static float g_steve_uv_w = 64.0f;
static float g_steve_uv_h = 32.0f;
static float g_steve_tint_r = 1.0f;
static float g_steve_tint_g = 1.0f;
static float g_steve_tint_b = 1.0f;

static void steve_set_texture_dims(const Texture *tex) {
    g_steve_uv_w = (tex && tex->w > 0) ? (float)tex->w : 64.0f;
    g_steve_uv_h = (tex && tex->h > 0) ? (float)tex->h : 32.0f;
}

static void steve_set_tint(float r, float g, float b) {
    g_steve_tint_r = r;
    g_steve_tint_g = g;
    g_steve_tint_b = b;
}

static void steve_color_for_normal(float nx, float ny, float nz) {
    float shade = 0.82f;
    if (ny < -0.5f) shade = 1.0f;
    else if (ny > 0.5f) shade = 0.55f;
    else if (nz > 0.5f) shade = 0.92f;
    else if (nz < -0.5f) shade = 0.70f;
    else if (nx != 0.0f) shade = 0.76f;
    glColor4f(shade * g_steve_tint_r, shade * g_steve_tint_g, shade * g_steve_tint_b, 1.0f);
}

static void steve_vertex(float x, float y, float z, float u, float v) {
    glTexCoord2f(u / g_steve_uv_w, v / g_steve_uv_h);
    glVertex3f(x * 0.0625f, y * 0.0625f, z * 0.0625f);
}

static void steve_quad(float ax, float ay, float az,
                       float bx, float by, float bz,
                       float cx, float cy, float cz,
                       float dx, float dy, float dz,
                       float u0, float v0, float u1, float v1,
                       float nx, float ny, float nz) {
    steve_color_for_normal(nx, ny, nz);
    steve_vertex(ax, ay, az, u1, v0);
    steve_vertex(bx, by, bz, u0, v0);
    steve_vertex(cx, cy, cz, u0, v1);
    steve_vertex(dx, dy, dz, u1, v1);
}

static void steve_box(int tex_x, int tex_y, float x, float y, float z,
                      int w, int h, int d, float inflate, int mirror) {
    float x0 = x - inflate;
    float y0 = y - inflate;
    float z0 = z - inflate;
    float x1 = x + (float)w + inflate;
    float y1 = y + (float)h + inflate;
    float z1 = z + (float)d + inflate;
    if (mirror) {
        float t = x1;
        x1 = x0;
        x0 = t;
    }

    /* Texture layout copied from Beta's ModelRenderer/ko.java box UV construction. */
    float u_right0 = (float)(tex_x + d + w);
    float v_right0 = (float)(tex_y + d);
    float u_right1 = (float)(tex_x + d + w + d);
    float v_right1 = (float)(tex_y + d + h);

    float u_left0 = (float)(tex_x + 0);
    float v_left0 = (float)(tex_y + d);
    float u_left1 = (float)(tex_x + d);
    float v_left1 = (float)(tex_y + d + h);

    float u_top0 = (float)(tex_x + d);
    float v_top0 = (float)(tex_y + 0);
    float u_top1 = (float)(tex_x + d + w);
    float v_top1 = (float)(tex_y + d);

    float u_bottom0 = (float)(tex_x + d + w);
    float v_bottom0 = (float)(tex_y + 0);
    float u_bottom1 = (float)(tex_x + d + w + w);
    float v_bottom1 = (float)(tex_y + d);

    float u_front0 = (float)(tex_x + d);
    float v_front0 = (float)(tex_y + d);
    float u_front1 = (float)(tex_x + d + w);
    float v_front1 = (float)(tex_y + d + h);

    float u_back0 = (float)(tex_x + d + w + d);
    float v_back0 = (float)(tex_y + d);
    float u_back1 = (float)(tex_x + d + w + d + w);
    float v_back1 = (float)(tex_y + d + h);

    glBegin(GL_QUADS);
    /* +X */
    steve_quad(x1,y0,z1, x1,y0,z0, x1,y1,z0, x1,y1,z1, u_right0,v_right0,u_right1,v_right1, 1,0,0);
    /* -X */
    steve_quad(x0,y0,z0, x0,y0,z1, x0,y1,z1, x0,y1,z0, u_left0,v_left0,u_left1,v_left1, -1,0,0);
    /* -Y / top */
    steve_quad(x1,y0,z1, x0,y0,z1, x0,y0,z0, x1,y0,z0, u_top0,v_top0,u_top1,v_top1, 0,-1,0);
    /* +Y / bottom */
    steve_quad(x1,y1,z0, x0,y1,z0, x0,y1,z1, x1,y1,z1, u_bottom0,v_bottom0,u_bottom1,v_bottom1, 0,1,0);
    /* -Z / front */
    steve_quad(x1,y0,z0, x0,y0,z0, x0,y1,z0, x1,y1,z0, u_front0,v_front0,u_front1,v_front1, 0,0,-1);
    /* +Z / back */
    steve_quad(x0,y0,z1, x1,y0,z1, x1,y1,z1, x0,y1,z1, u_back0,v_back0,u_back1,v_back1, 0,0,1);
    glEnd();
}

static void steve_part(int tex_x, int tex_y,
                       float pivot_x, float pivot_y, float pivot_z,
                       float box_x, float box_y, float box_z,
                       int box_w, int box_h, int box_d,
                       float inflate, int mirror,
                       float rot_x_deg, float rot_y_deg, float rot_z_deg) {
    glPushMatrix();
    glTranslatef(pivot_x * 0.0625f, pivot_y * 0.0625f, pivot_z * 0.0625f);
    if (rot_z_deg != 0.0f) glRotatef(rot_z_deg, 0.0f, 0.0f, 1.0f);
    if (rot_y_deg != 0.0f) glRotatef(rot_y_deg, 0.0f, 1.0f, 0.0f);
    if (rot_x_deg != 0.0f) glRotatef(rot_x_deg, 1.0f, 0.0f, 0.0f);
    steve_box(tex_x, tex_y, box_x, box_y, box_z, box_w, box_h, box_d, inflate, mirror);
    glPopMatrix();
}

static void draw_inventory_steve(int inv_x, int inv_y) {
    if (!tex_steve.id) return;

    float mx = (float)(inv_x + 51 - g_mouse_x);
    float my = (float)(inv_y + 75 - 50 - g_mouse_y);
    /* ns.java stores these directly as degrees after multiplying atan() by 20/40.
       Do not convert radians to degrees here; doing so over-rotates the model by
       ~57x and makes Steve appear twisted/sideways in the inventory. */
    float body_yaw = atanf(mx / 40.0f) * 40.0f;
    float head_yaw = atanf(mx / 40.0f) * 20.0f;
    float head_pitch = -atanf(my / 40.0f) * 20.0f;

    float idle = (float)g_ingame_ticks;
    float arm_roll = cosf(idle * 0.09f) * 2.86479f + 2.86479f;
    float arm_pitch = sinf(idle * 0.067f) * 2.86479f;

    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_steve.id);
    steve_set_texture_dims(&tex_steve);
    steve_set_tint(1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);

    /* char.png has transparent pixels in the second head layer (helmet/hat).
       Minecraft renders those through the alpha test.  The previous patch
       drew the overlay with blending disabled, so the transparent RGB=0 pixels
       became a solid black cube over Steve's head. */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);

    /* ns.java inventory transform:
       translate(x+51,y+75,50), scale(-30,30,30), rotate 180 Z,
       rotate 135 Y, setup standard item lighting, rotate -135 Y,
       pitch toward mouse, then render the player model. */
    glTranslatef((float)(inv_x + 51), (float)(inv_y + 75), 50.0f);
    glScalef(-30.0f, 30.0f, 30.0f);
    glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
    glRotatef(135.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(-135.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(head_pitch, 1.0f, 0.0f, 0.0f);

    /* RenderLiving/ei.java transform for a standing humanoid.
       Do NOT add player aY/yOffset here: ns.java adds it before calling
       RenderManager, but ch.java subtracts it again before ei.java renders.
       The previous C patch added +1.62 without the matching subtract, which
       pushed Steve above the inventory preview box. */
    glRotatef(180.0f - body_yaw, 0.0f, 1.0f, 0.0f);
    glScalef(-1.0f, -1.0f, 1.0f);
    glTranslatef(0.0f, -24.0f * 0.0625f - 0.0078125f, 0.0f);

    glColor4f(1,1,1,1);
    steve_part(16, 16, 0, 0, 0, -4, 0, -2, 8, 12, 4, 0.0f, 0, 0, 0, 0);
    steve_part(40, 16, -5, 2, 0, -3, -2, -2, 4, 12, 4, 0.0f, 0, arm_pitch, 0, arm_roll);
    steve_part(40, 16,  5, 2, 0, -1, -2, -2, 4, 12, 4, 0.0f, 1, -arm_pitch, 0, -arm_roll);
    steve_part(0, 16, -2, 12, 0, -2, 0, -2, 4, 12, 4, 0.0f, 0, 0, 0, 0);
    steve_part(0, 16,  2, 12, 0, -2, 0, -2, 4, 12, 4, 0.0f, 1, 0, 0, 0);
    steve_part(0, 0, 0, 0, 0, -4, -8, -4, 8, 8, 8, 0.0f, 0, head_pitch, head_yaw, 0);
    steve_part(32, 0, 0, 0, 0, -4, -8, -4, 8, 8, 8, 0.5f, 0, head_pitch, head_yaw, 0);
    draw_armor_model_for_slots(g_armor_inventory, 0.0f, 12.0f, 0.0f,
                               0.0f, head_pitch, head_yaw,
                               arm_pitch, 0.0f, arm_roll,
                               -arm_pitch, 0.0f, -arm_roll,
                               0.0f, 0.0f);

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glColor4f(1,1,1,1);
    glPopMatrix();
}


static void draw_hovering_text_colored(const char *lines[], int line_count, int mx, int my, int first_color) {
    if (!lines || line_count <= 0) return;
    int width = 0;
    for (int i = 0; i < line_count; ++i) {
        int w = text_width(lines[i]);
        if (w > width) width = w;
    }

    int x = mx + 12;
    int y = my - 12;
    int h = 8;
    if (line_count > 1) h += 2 + (line_count - 1) * 10;

    /* Java 1.2.5 GuiContainer tooltip: same offsets, background, purple border,
       z-order intent, and first-line/extra-line spacing.  It intentionally does
       not clamp to the screen edges in this version. */
    int bg = (int)0xF0100010u;
    draw_gradient(x - 3, y - 4, x + width + 3, y - 3, bg, bg);
    draw_gradient(x - 3, y + h + 3, x + width + 3, y + h + 4, bg, bg);
    draw_gradient(x - 3, y - 3, x + width + 3, y + h + 3, bg, bg);
    draw_gradient(x - 4, y - 3, x - 3, y + h + 3, bg, bg);
    draw_gradient(x + width + 3, y - 3, x + width + 4, y + h + 3, bg, bg);

    int border1 = 0x505000FF;
    int border2 = ((border1 & 0x00FEFEFE) >> 1) | (border1 & (int)0xFF000000u);
    draw_gradient(x - 3, y - 2, x - 2, y + h + 2, border1, border2);
    draw_gradient(x + width + 2, y - 2, x + width + 3, y + h + 2, border1, border2);
    draw_gradient(x - 3, y - 3, x + width + 3, y - 2, border1, border1);
    draw_gradient(x - 3, y + h + 2, x + width + 3, y + h + 3, border2, border2);

    for (int i = 0; i < line_count; ++i) {
        int color = (i == 0) ? first_color : 0xA0A0A0;
        draw_text(lines[i], x, y, color);
        if (i == 0) y += 2;
        y += 10;
    }
}

static void draw_hovering_text(const char *lines[], int line_count, int mx, int my) {
    draw_hovering_text_colored(lines, line_count, mx, my, 0xFFFFFF);
}

static int item_tooltip_first_line_color(const ItemStack *st) {
    if (st && item_is_record_id(st->id)) return 0x55FFFF;
    return 0xFFFFFF;
}

static void append_item_extra_tooltip_lines(const ItemStack *st, const char *lines[], int *line_count, char potion_lines[][96], int potion_cap) {
    if (!st || !lines || !line_count) return;
    if (item_is_record_id(st->id) && *line_count < 10) {
        const char *rec = record_tooltip_line_for_id(st->id);
        if (rec && rec[0]) lines[(*line_count)++] = rec;
    }
    if (st->id == ITEM_POTION) {
        int n = pex_potion_tooltip_lines(st, potion_lines, potion_cap);
        for (int i = 0; i < n && *line_count < 10; ++i) lines[(*line_count)++] = potion_lines[i];
    }
}

static void draw_item_tooltip_for_slot(int slot, int mx, int my) {
    if (!stack_empty(&g_carried_stack) || slot < 0) return;
    ItemStack *st = NULL;
    ItemStack tmp;
    if (slot == 104 || slot == 309) {
        tmp = current_crafting_output();
        if (!stack_empty(&tmp)) st = &tmp;
    } else {
        st = inventory_slot_ptr(slot);
    }
    if (!st || stack_empty(st)) return;
    const char *name = item_stack_display_name(st);
    if (!name || !name[0]) return;

    const char *lines[10];
    char potion_lines[8][96];
    int line_count = 0;
    lines[line_count++] = name;

    append_item_extra_tooltip_lines(st, lines, &line_count, potion_lines, 8);

    int max_damage = item_max_damage(st->id);
    char durability[64];
    if (max_damage > 0 && st->damage > 0 && line_count < 10) {
        int remaining = max_damage - st->damage;
        if (remaining < 0) remaining = 0;
        snprintf(durability, sizeof(durability), "Durability: %d / %d", remaining, max_damage);
        lines[line_count++] = durability;
    }

    draw_hovering_text_colored(lines, line_count, mx, my, item_tooltip_first_line_color(st));
}

static void draw_hovered_item_tooltip(void) {
    draw_item_tooltip_for_slot(inventory_slot_at(g_mouse_x, g_mouse_y), g_mouse_x, g_mouse_y);
}

static void draw_active_potion_effects_panel(int inv_x, int inv_y) {
    int count = pex_active_potion_count();
    if (count <= 0 || !tex_inventory.id) return;
    int x = inv_x - 124;
    int y = inv_y;
    int spacing = 33;
    if (count > 5) spacing = 132 / (count - 1);
    if (spacing < 26) spacing = 26;
    int drawn = 0;
    for (int id = 1; id < PEX_POTION_MAX; ++id) {
        if (!g_player_potion_effects[id].duration) continue;
        int py = y + drawn * spacing;
        draw_textured_rect_tex(&tex_inventory, x, py, 0, 166, 140, 32, 0xFFFFFF);
        int icon = pex_potion_status_icon_index(id);
        if (icon >= 0) {
            draw_textured_rect_tex(&tex_inventory, x + 6, py + 7, (icon % 8) * 18, 198 + (icon / 8) * 18, 18, 18, 0xFFFFFF);
        }
        char duration[32];
        pex_potion_duration_string(g_player_potion_effects[id].duration, duration, sizeof(duration));
        draw_text_no_shadow(pex_potion_effect_display_name(id, g_player_potion_effects[id].amplifier), x + 28, py + 6, 0xFFFFFF);
        draw_text_no_shadow(duration, x + 28, py + 16, 0x7F7F7F);
        drawn++;
    }
}

static void draw_inventory_screen(void) {
    draw_ingame_world_view(0);
    draw_gradient(0, 0, g_gui_w, g_gui_h, -1072689136, -804253680);
    int x = (g_gui_w - 176) / 2;
    int y = (g_gui_h - 166) / 2;
    draw_textured_rect_tex(&tex_inventory, x, y, 0, 0, 176, 166, 0xFFFFFF);
    draw_inventory_steve(x, y);
    for (int row = 0; row < 4; row++) {
        draw_item_stack_gui(&g_armor_inventory[3 - row], x + 8, y + 8 + row * 18);
    }
    for (int row = 0; row < 3; row++) for (int col = 0; col < 9; col++) {
        int slot = 9 + col + row * 9;
        draw_item_stack_gui(&g_inventory[slot], x + 8 + col * 18, y + 84 + row * 18);
    }
    for (int col = 0; col < 9; col++) draw_item_stack_gui(&g_inventory[col], x + 8 + col * 18, y + 142);

    /* Container labels are drawn without the HUD-style shadow, matching the
       original GUI text color and avoiding the dark/white muddy look. */
    draw_text_no_shadow(tr_key_default("container.crafting", "Crafting"), x + 86, y + 6, 0x404040);
    for (int row = 0; row < 2; row++) for (int col = 0; col < 2; col++) {
        int slot = col + row * 2;
        draw_item_stack_gui(&g_craft_grid[slot], x + 88 + col * 18, y + 26 + row * 18);
    }
    ItemStack craft_out = inventory_crafting_output();
    draw_item_stack_gui(&craft_out, x + 144, y + 36);

    draw_active_potion_effects_panel(x, y);
    draw_hovered_item_tooltip();
    draw_carried_stack();
}

static void draw_creative_tooltip(int mx, int my) {
    if (!stack_empty(&g_carried_stack)) return;
    int idx = creative_item_index_at(mx, my);
    if (idx < 0) {
        int hb = creative_hotbar_slot_at(mx, my);
        if (hb >= 0 && hb < 9 && !stack_empty(&g_inventory[hb])) {
            const char *name = item_stack_display_name(&g_inventory[hb]);
            if (name && name[0]) {
                const char *lines[10];
                char potion_lines[8][96];
                int line_count = 0;
                lines[line_count++] = name;
                append_item_extra_tooltip_lines(&g_inventory[hb], lines, &line_count, potion_lines, 8);
                draw_hovering_text_colored(lines, line_count, mx, my, item_tooltip_first_line_color(&g_inventory[hb]));
            }
        }
        return;
    }
    ItemStack st = creative_stack_for_index(idx);
    const char *name = item_stack_display_name(&st);
    if (name && name[0]) {
        const char *lines[10];
        char potion_lines[8][96];
        int line_count = 0;
        lines[line_count++] = name;
        append_item_extra_tooltip_lines(&st, lines, &line_count, potion_lines, 8);
        draw_hovering_text_colored(lines, line_count, mx, my, item_tooltip_first_line_color(&st));
    }
}

static void draw_creative_slot_bg(int x, int y) {
    draw_rect(x, y, x + 18, y + 18, 0xFF8A8A8A);
    draw_rect(x + 1, y + 1, x + 17, y + 17, 0xFFC6C6C6);
    draw_rect(x + 1, y + 1, x + 16, y + 2, 0xFFE0E0E0);
    draw_rect(x + 1, y + 1, x + 2, y + 16, 0xFFE0E0E0);
    draw_rect(x + 16, y + 2, x + 17, y + 17, 0xFF555555);
    draw_rect(x + 2, y + 16, x + 17, y + 17, 0xFF555555);
}

static void draw_creative_screen(void) {
    draw_ingame_world_view(0);
    draw_gradient(0, 0, g_gui_w, g_gui_h, -1072689136, -804253680);
    int x = (g_gui_w - CREATIVE_XSIZE) / 2;
    int y = (g_gui_h - CREATIVE_YSIZE) / 2;

    /* Java 1.2.5 GuiContainerCreative uses /gui/allitems.png, 176x208.
       If that exact texture exists in the selected pack, draw it.  The code-drawn
       fallback keeps creative usable with older beta packs that do not ship it. */
    int textured_bg = tex_allitems.id && tex_allitems.w >= 176 && tex_allitems.h >= 224;
    if (textured_bg) {
        draw_textured_rect_tex(&tex_allitems, x, y, 0, 0, CREATIVE_XSIZE, CREATIVE_YSIZE, 0xFFFFFF);
    } else {
        draw_rect(x, y, x + CREATIVE_XSIZE, y + CREATIVE_YSIZE, 0xFFC6C6C6);
        draw_rect(x, y, x + CREATIVE_XSIZE, y + 1, 0xFFFFFFFF);
        draw_rect(x, y, x + 1, y + CREATIVE_YSIZE, 0xFFFFFFFF);
        draw_rect(x + CREATIVE_XSIZE - 1, y + 1, x + CREATIVE_XSIZE, y + CREATIVE_YSIZE, 0xFF555555);
        draw_rect(x + 1, y + CREATIVE_YSIZE - 1, x + CREATIVE_XSIZE, y + CREATIVE_YSIZE, 0xFF555555);
    }

    draw_text_no_shadow(tr_key_default("container.creative", "Item Selection"), x + 8, y + 6, 0x404040);

    int start = g_creative_scroll_row * CREATIVE_COLS;
    for (int row = 0; row < CREATIVE_ROWS; row++) {
        for (int col = 0; col < CREATIVE_COLS; col++) {
            int sx = x + 8 + col * 18;
            int sy = y + 18 + row * 18;
            if (!textured_bg) draw_creative_slot_bg(sx, sy);
            int idx = start + row * CREATIVE_COLS + col;
            if (idx < creative_catalog_count()) {
                ItemStack st = creative_stack_for_index(idx);
                draw_item_stack_gui(&st, sx, sy);
            }
        }
    }

    /* Scrollbar track and thumb placement copied from GuiContainerCreative. */
    int track_x = x + 155;
    int track_y = y + 17;
    int track_h = 160 + 2;
    int thumb_y = y + 17 + (int)((float)(track_h - 17) * g_creative_scroll);
    if (textured_bg) {
        draw_textured_rect_tex(&tex_allitems, x + 154, thumb_y, 0, 208, 16, 16, 0xFFFFFF);
    } else {
        draw_rect(track_x, track_y, track_x + 14, track_y + track_h, 0xFF8A8A8A);
        draw_rect(track_x + 1, track_y + 1, track_x + 13, track_y + track_h - 1, 0xFFB0B0B0);
        draw_rect(x + 154, thumb_y, x + 170, thumb_y + 16, 0xFFC6C6C6);
        draw_rect(x + 154, thumb_y, x + 170, thumb_y + 1, 0xFFFFFFFF);
        draw_rect(x + 154, thumb_y, x + 155, thumb_y + 16, 0xFFFFFFFF);
        draw_rect(x + 169, thumb_y + 1, x + 170, thumb_y + 16, 0xFF555555);
        draw_rect(x + 155, thumb_y + 15, x + 170, thumb_y + 16, 0xFF555555);
    }

    for (int col = 0; col < 9; col++) {
        if (!textured_bg) draw_creative_slot_bg(x + 8 + col * 18, y + 184);
        draw_item_stack_gui(&g_inventory[col], x + 8 + col * 18, y + 184);
    }

    draw_creative_tooltip(g_mouse_x, g_mouse_y);
    draw_carried_stack();
}



static void draw_player_inventory_stacks_at(int x, int y, int inv_y, int hotbar_y) {
    for (int row = 0; row < 3; row++) for (int col = 0; col < 9; col++) {
        int slot = 9 + col + row * 9;
        draw_item_stack_gui(&g_inventory[slot], x + 8 + col * 18, y + inv_y + row * 18);
    }
    for (int col = 0; col < 9; col++) draw_item_stack_gui(&g_inventory[col], x + 8 + col * 18, y + hotbar_y);
}

static void draw_player_inventory_stacks(int x, int y) {
    draw_player_inventory_stacks_at(x, y, 84, 142);
}

static void draw_workbench_screen(void) {
    draw_ingame_world_view(0);
    draw_gradient(0, 0, g_gui_w, g_gui_h, -1072689136, -804253680);
    int x = (g_gui_w - 176) / 2;
    int y = (g_gui_h - 166) / 2;
    if (tex_workbench.id) draw_textured_rect_tex(&tex_workbench, x, y, 0, 0, 176, 166, 0xFFFFFF);
    else draw_rect(x, y, x + 176, y + 166, 0xFFc6c6c6);

    draw_text_no_shadow(tr_key_default("container.crafting", "Crafting"), x + 28, y + 6, 4210752);
    for (int row = 0; row < 3; row++) for (int col = 0; col < 3; col++) {
        int slot = col + row * 3;
        draw_item_stack_gui(&g_workbench_grid[slot], x + 30 + col * 18, y + 17 + row * 18);
    }

    ItemStack out = workbench_crafting_output();
    draw_item_stack_gui(&out, x + 124, y + 35);

    draw_text_no_shadow(tr_key_default("container.inventory", "Inventory"), x + 8, y + 72, 4210752);
    draw_player_inventory_stacks(x, y);
    draw_hovered_item_tooltip();
    draw_carried_stack();
}


static void draw_chest_screen(void) {
    draw_ingame_world_view(0);
    draw_gradient(0, 0, g_gui_w, g_gui_h, -1072689136, -804253680);
    int rows = (g_open_chest_rows == 6) ? 6 : 3;
    int h = (rows == 6) ? 222 : 166;
    int inv_label_y = (rows == 6) ? 128 : 72;
    int inv_slots_y = (rows == 6) ? 140 : 84;
    int hotbar_y = (rows == 6) ? 198 : 142;
    int x = (g_gui_w - 176) / 2;
    int y = (g_gui_h - h) / 2;

    if (tex_chest_gui.id) {
        if (rows == 6 && tex_chest_gui.w >= 176 && tex_chest_gui.h >= 222) {
            draw_textured_rect_tex(&tex_chest_gui, x, y, 0, 0, 176, 222, 0xFFFFFF);
        } else if (tex_chest_gui.w >= 176 && tex_chest_gui.h >= 222) {
            /* The Java container.png is a double-chest sheet. For a single chest,
               draw the title + 3 chest rows, then stitch the player inventory
               section up exactly like a 27-slot Beta chest screen. */
            draw_textured_rect_tex(&tex_chest_gui, x, y, 0, 0, 176, 71, 0xFFFFFF);
            draw_textured_rect_part_scaled(&tex_chest_gui, x, y + 71, 176, 95, 0, 127, 176, 95, 0xFFFFFF);
        } else {
            draw_textured_rect_tex(&tex_chest_gui, x, y, 0, 0, 176, h, 0xFFFFFF);
        }
    } else draw_rect(x, y, x + 176, y + h, 0xFFc6c6c6);
    draw_text_no_shadow(g_open_chest_title[0] ? g_open_chest_title : tr_key_default("container.chest", "Chest"), x + 8, y + 6, 4210752);
    draw_text_no_shadow(tr_key_default("container.inventory", "Inventory"), x + 8, y + inv_label_y, 4210752);

    for (int row = 0; row < rows; row++) for (int col = 0; col < 9; col++) {
        ItemStack *st = chest_get_open_slot_ptr(col + row * 9);
        if (st) draw_item_stack_gui(st, x + 8 + col * 18, y + 18 + row * 18);
    }
    draw_player_inventory_stacks_at(x, y, inv_slots_y, hotbar_y);
    draw_hovered_item_tooltip();

    draw_carried_stack();
}

static void draw_furnace_screen(void) {
    draw_ingame_world_view(0);
    draw_gradient(0, 0, g_gui_w, g_gui_h, -1072689136, -804253680);
    int x = (g_gui_w - 176) / 2;
    int y = (g_gui_h - 166) / 2;
    if (tex_furnace_gui.id) draw_textured_rect_tex(&tex_furnace_gui, x, y, 0, 0, 176, 166, 0xFFFFFF);
    else draw_rect(x, y, x + 176, y + 166, 0xFFc6c6c6);

    if (tex_furnace_gui.id && furnace_open_tile()) {
        int flame = furnace_burn_scaled(12);
        if (flame > 0) {
            draw_textured_rect_tex(&tex_furnace_gui, x + 56, y + 36 + 12 - flame, 176, 12 - flame, 14, flame + 2, 0xFFFFFF);
        }
        int arrow = furnace_cook_scaled(24);
        if (arrow > 0) {
            draw_textured_rect_tex(&tex_furnace_gui, x + 79, y + 34, 176, 14, arrow + 1, 16, 0xFFFFFF);
        }
    }

    draw_text_no_shadow(tr_key_default("container.furnace", "Furnace"), x + 60, y + 6, 4210752);
    draw_text_no_shadow(tr_key_default("container.inventory", "Inventory"), x + 8, y + 72, 4210752);

    ItemStack *in = furnace_get_slot_ptr(0);
    ItemStack *fuel = furnace_get_slot_ptr(1);
    ItemStack *out = furnace_get_slot_ptr(2);
    if (in) draw_item_stack_gui(in, x + 56, y + 17);
    if (fuel) draw_item_stack_gui(fuel, x + 56, y + 53);
    if (out) draw_item_stack_gui(out, x + 116, y + 35);

    draw_player_inventory_stacks(x, y);
    draw_hovered_item_tooltip();
    draw_carried_stack();
}



static void draw_death_screen(void) {
    draw_ingame_world_view(0);
    draw_gradient(0, 0, g_gui_w, g_gui_h, 1615855616, -1602211792);
    draw_centered_text(tr_key_default("deathScreen.title", "Game over!"), g_gui_w / 2, 30, 16777215);
    { char score_line[96]; snprintf(score_line, sizeof(score_line), "%s: &e0", tr_key_default("deathScreen.score", "Score")); draw_centered_text(score_line, g_gui_w / 2, 100, 16777215); }
    draw_all_buttons();
    draw_save_message();
}

static void draw_chat_screen(void) {
    draw_ingame_screen();
    char field[160];
    snprintf(field, sizeof(field), "> %s%s", g_chat_input, (g_ingame_ticks / 6 % 2 == 0) ? "_" : "");
    draw_rect(2, g_gui_h - 14, g_gui_w - 2, g_gui_h - 2, (int)0x80000000u);
    draw_text(field, 4, g_gui_h - 12, 14737632);
}


static void draw_text_field_box(const char *text, int x, int y, int w, int h, int focused) {
    draw_rect(x - 1, y - 1, x + w + 1, y + h + 1, focused ? -1 : -6250336);
    draw_rect(x, y, x + w, y + h, -16777216);
    char field[160];
    snprintf(field, sizeof(field), "%s%s", text ? text : "", (focused && (g_ticks / 6 % 2 == 0)) ? "_" : "");
    draw_text(field, x + 4, y + 6, 14737632);
}

static void draw_create_world_screen(void) {
    draw_default_bg();
    draw_centered_text(tr_key_default("selectWorld.create", "Create New World"), g_gui_w / 2, 20, 16777215);
    if (!g_create_more_options) {
        draw_text(tr_key_default("selectWorld.enterName", "World Name"), g_gui_w / 2 - 100, 47, 10526880);
        draw_text_field_box(g_pending_world_name, g_gui_w / 2 - 100, 60, 200, 20, g_create_edit_field == 0);
        char folder[160];
        const char *slash = strrchr(g_pending_world_dir, '\\');
        const char *name = slash ? slash + 1 : g_pending_world_dir;
        snprintf(folder, sizeof(folder), "%s: %s", tr_key_default("selectWorld.resultFolder", "Will be saved in"), name && name[0] ? name : tr_key_default("selectWorld.newWorld", "New World"));
        draw_text(folder, g_gui_w / 2 - 100, 85, 10526880);
        if (g_pending_game_mode) {
            draw_text(tr_key_default("selectWorld.gameMode.creative.line1", "Unlimited blocks, instant mining"), g_gui_w / 2 - 100, 122, 10526880);
            draw_text(tr_key_default("selectWorld.gameMode.creative.line2", "no health, hunger or item loss"), g_gui_w / 2 - 100, 134, 10526880);
        } else {
            draw_text(tr_key_default("selectWorld.gameMode.survival.line1", "Search for resources, crafting, gain"), g_gui_w / 2 - 100, 122, 10526880);
            draw_text(tr_key_default("selectWorld.gameMode.survival.line2", "levels, health and hunger"), g_gui_w / 2 - 100, 134, 10526880);
        }
    } else {
        draw_text(tr_key_default("selectWorld.enterSeed", "Seed for the World Generator"), g_gui_w / 2 - 100, 47, 10526880);
        draw_text_field_box(g_pending_seed_text, g_gui_w / 2 - 100, 60, 200, 20, g_create_edit_field == 1);
        draw_text(tr_key_default("selectWorld.seedInfo", "Leave blank for a random seed"), g_gui_w / 2 - 100, 85, 10526880);
        draw_text(tr_key_default("selectWorld.mapFeatures.info", "Villages, dungeons etc"), g_gui_w / 2 - 150, 122, 10526880);
    }
    draw_all_buttons();
}

static void draw_rename_world_screen(void) {
    draw_default_bg();
    draw_centered_text(tr_key_default("selectWorld.renameTitle", "Rename World"), g_gui_w / 2, 20, 16777215);
    draw_text(tr_key_default("selectWorld.enterName", "World Name"), g_gui_w / 2 - 100, 47, 10526880);
    draw_text_field_box(g_rename_world_text, g_gui_w / 2 - 100, 60, 200, 20, 1);
    draw_all_buttons();
}


static void draw_set_name_screen(void) {
    draw_default_bg();
    draw_centered_text("Choose your nickname", g_gui_w / 2, g_gui_h / 2 - 58, 16777215);
    draw_text("Nickname", g_gui_w / 2 - 100, g_gui_h / 2 - 21, 10526880);
    draw_text_field_box(g_name_edit_text, g_gui_w / 2 - 100, g_gui_h / 2 - 8, 200, 20, 1);
    draw_centered_text("This can be changed later in Settings.", g_gui_w / 2, g_gui_h / 2 + 20, 10526880);
    if (g_name_edit_text[0] && !pex_nickname_valid(g_name_edit_text)) {
        draw_centered_text("Use 3-16 letters, numbers, or underscores.", g_gui_w / 2, g_gui_h / 2 + 32, 0xFF5555);
    }
    draw_all_buttons();
}

static void draw_tv_remote_map_screen(void) {
    draw_default_bg();
    draw_centered_text("Remote Button Setup", g_gui_w / 2, 24, 16777215);
    draw_centered_text("Press each button on your TV remote.", g_gui_w / 2, 48, 10526880);
    draw_centered_text("This can be changed later in Settings.", g_gui_w / 2, 60, 10526880);

    if (g_tv_remote_map_step < 0) g_tv_remote_map_step = 0;
    if (g_tv_remote_map_step >= PEX_TV_REMOTE_ACTION_COUNT) g_tv_remote_map_step = PEX_TV_REMOTE_ACTION_COUNT - 1;

    char line[160];
    snprintf(line, sizeof(line), "%d/%d", g_tv_remote_map_step + 1, PEX_TV_REMOTE_ACTION_COUNT);
    draw_centered_text(line, g_gui_w / 2, 86, 0xFFFFA0);
    snprintf(line, sizeof(line), "Press: %s", pex_tv_remote_action_label(g_tv_remote_map_step));
    draw_centered_text(line, g_gui_w / 2, 104, 16777215);

    int x = g_gui_w / 2 - 120;
    int y = 126;
    for (int i = 0; i < PEX_TV_REMOTE_ACTION_COUNT; ++i) {
        int color = i < g_tv_remote_map_step ? 0x55FF55 : (i == g_tv_remote_map_step ? 0xFFFF55 : 0x808080);
        snprintf(line, sizeof(line), "%s%s", i < g_tv_remote_map_step ? "OK  " : (i == g_tv_remote_map_step ? "->  " : "    "), pex_tv_remote_action_label(i));
        draw_text(line, x, y, color);
        y += 9;
        if (y > g_gui_h - 62) break;
    }

    draw_all_buttons();
}


static void draw_virtual_keyboard_screen(void) {
    draw_default_bg();
    draw_centered_text("Text Input", g_gui_w / 2, 18, 16777215);
    const char *label = "";
    if (g_virtual_keyboard_return_screen == SCREEN_CHAT) label = "Chat";
    else if (g_virtual_keyboard_return_screen == SCREEN_SET_NAME) label = "Nickname";
    else if (g_virtual_keyboard_return_screen == SCREEN_CREATE_WORLD) label = g_create_more_options ? "Seed" : "World Name";
    else if (g_virtual_keyboard_return_screen == SCREEN_RENAME_WORLD) label = "Rename World";
    else if (g_virtual_keyboard_return_screen == SCREEN_MULTIPLAYER) label = pex_mp_server_edit_field_get() == 1 ? "Server Name" : "Server Address";
    draw_centered_text(label, g_gui_w / 2, 34, 10526880);
    size_t cap = 0;
    char *dst = pex_virtual_keyboard_target(&cap);
    (void)cap;
    draw_text_field_box(dst ? dst : "", g_gui_w / 2 - 130, 48, 260, 20, 1);

    static const char *rows[] = {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm_-."};
    int y = 84;
    for (int r = 0; r < 4; ++r) {
        const char *row = rows[r];
        int n = (int)strlen(row);
        int total = n * 20;
        int x = g_gui_w / 2 - total / 2;
        for (int c = 0; c < n; ++c) {
            char ch[2] = { row[c], 0 };
            if (g_virtual_keyboard_caps && ch[0] >= 'a' && ch[0] <= 'z') ch[0] = (char)(ch[0] - 'a' + 'A');
            int selected = (g_virtual_keyboard_row == r && g_virtual_keyboard_col == c);
            draw_rect(x + c * 20, y, x + c * 20 + 18, y + 18, selected ? 0xFFFFFFFF : 0xFF555555);
            draw_rect(x + c * 20 + 1, y + 1, x + c * 20 + 17, y + 17, selected ? 0xFFAAAAAA : 0xFF000000);
            draw_centered_text(ch, x + c * 20 + 9, y + 5, selected ? 0xFFFF55 : 0xFFFFFF);
        }
        y += 22;
    }
    const char *actions[] = {"Space", "Back", g_virtual_keyboard_caps ? "Caps:ON" : "Caps", "Done", "Cancel"};
    int widths[] = {52, 44, 58, 44, 52};
    int total = 0;
    for (int i = 0; i < 5; ++i) total += widths[i] + 4;
    int x = g_gui_w / 2 - total / 2;
    for (int i = 0; i < 5; ++i) {
        int selected = (g_virtual_keyboard_row == 4 && g_virtual_keyboard_col == i);
        draw_rect(x, y, x + widths[i], y + 18, selected ? 0xFFFFFFFF : 0xFF555555);
        draw_rect(x + 1, y + 1, x + widths[i] - 1, y + 17, selected ? 0xFFAAAAAA : 0xFF000000);
        draw_centered_text(actions[i], x + widths[i] / 2, y + 5, selected ? 0xFFFF55 : 0xFFFFFF);
        x += widths[i] + 4;
    }
    draw_centered_text("D-pad: move  A/OK: select  B/Back: erase  Menu: cancel", g_gui_w / 2, g_gui_h - 24, 10526880);
}

static void draw_world_type_screen(void) {
    draw_default_bg();
    draw_centered_text(tr_key_default("selectWorld.create", "Create New World"), g_gui_w / 2, 40, 16777215);
    draw_centered_text(tr_key_default("selectWorld.legacyUnused", "This legacy screen is unused"), g_gui_w / 2, 64, 10526880);
    draw_all_buttons();
}

static void draw_generating_screen(void) {
    draw_tiled_background_tint(0x404040, 0);
    int bar_w = 100;
    int bar_h = 2;
    int x = g_gui_w / 2 - bar_w / 2;
    int y = g_gui_h / 2 + 16;
    int p = g_worldgen.progress;
    if (p < 0) p = 0;
    if (p > 100) p = 100;
    draw_rect(x, y, x + bar_w, y + bar_h, 8421504);
    draw_rect(x, y, x + p, y + bar_h, 8454016);
    draw_centered_text(g_worldgen.title, g_gui_w / 2, g_gui_h / 2 - 20, 16777215);
    draw_centered_text(g_worldgen.status, g_gui_w / 2, g_gui_h / 2 + 8, 16777215);
}


static void draw_texturepack_install_screen(void) {
    draw_tiled_background_tint(0x404040, 0);
    int bar_w = 100;
    int bar_h = 2;
    int x = g_gui_w / 2 - bar_w / 2;
    int y = g_gui_h / 2 + 16;
    int p = (int)InterlockedCompareExchange(&g_classic_install_progress, 0, 0);
    char status[MAX_LABEL];
    lstrcpynA(status, g_classic_install_status[0] ? g_classic_install_status : "Downloading resources...", sizeof(status));
    if (p < 0) p = 0;
    if (p > 100) p = 100;
    draw_rect(x, y, x + bar_w, y + bar_h, 8421504);
    draw_rect(x, y, x + p, y + bar_h, 8454016);
    draw_centered_text("Downloading Release resources", g_gui_w / 2, g_gui_h / 2 - 20, 16777215);
    draw_centered_text(status, g_gui_w / 2, g_gui_h / 2 + 8, 16777215);
}

static void draw_notice(void) {
    draw_default_bg();
    draw_centered_text(g_notice_title, g_gui_w / 2, g_gui_h / 4 - 60 + 20, 16777215);
    draw_text(g_notice_line1, g_gui_w / 2 - 140, g_gui_h / 4 - 60 + 60, 10526880);
    draw_text(g_notice_line2, g_gui_w / 2 - 140, g_gui_h / 4 - 60 + 69, 10526880);
    draw_all_buttons();
}


static void draw_renderer_restart_prompt(void) {
    draw_default_bg();
    draw_centered_text("Restart Required", g_gui_w / 2, g_gui_h / 4 - 60 + 20, 16777215);
    draw_text("Changing the renderer requires restarting the game.", g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 56, 10526880);
    char line[MAX_LABEL];
    snprintf(line, sizeof(line), "New renderer: %s", renderer_backend_label(g_selected_renderer_backend));
    draw_text(line, g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 73, 16777215);
    draw_text("Restart applies it. Ignore keeps the current renderer.", g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 90, 10526880);
    draw_all_buttons();
}

static void draw_pack_download_prompt(void) {
    char size_line[MAX_LABEL];
    char summary[MAX_LABEL];
    pack_install_start_size_fetch();
    classic_resource_missing_summary(summary, sizeof(summary));
    draw_default_bg();
    draw_centered_text("Release Resources Download", g_gui_w / 2, g_gui_h / 4 - 60 + 14, 16777215);
    draw_text(summary, g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 35, 16777215);
    draw_text("Choose exactly which official Release assets to download.", g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 49, 10526880);
    draw_text("Nothing downloads until you press Download selected.", g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 60, 10526880);
#if PEX_CLASSIC_SOUND_DOWNLOAD_SUPPORTED
    draw_text("Audio uses Mojang legacy.json and is saved by category.", g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 71, 10526880);
#else
    draw_text("Sound downloads are disabled on this platform build.", g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 71, 10526880);
#endif
    format_download_size(size_line, sizeof(size_line));
    draw_text(size_line, g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 88, 10526880);
    draw_all_buttons();
}

static void draw_classic_pack_warning(void) {
    char summary[MAX_LABEL];
    classic_resource_missing_summary(summary, sizeof(summary));
    draw_default_bg();
    draw_centered_text("Resource Update Needed", g_gui_w / 2, g_gui_h / 4 - 60 + 20, 16777215);
    draw_text(summary, g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 56, 10526880);
    draw_text("Do you want to download the missing resources now?", g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 75, 16777215);
    draw_text("No keeps the current files unchanged.", g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 92, 10526880);
    draw_all_buttons();
}
