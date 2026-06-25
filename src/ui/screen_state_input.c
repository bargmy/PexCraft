/* Split from original monolithic main.c. Included by src/main.c unity build. */

static void rebuild_screen(void);
static void release_title_state_enter(void);

/* GuiMainMenu owns panoramaTimer/viewportTexture per screen instance in Java.
   g_screen starts as SCREEN_TITLE before platform init calls set_screen(), so
   the first title entry must not be skipped just because old==new. */
static int g_release_title_state_initialized = 0;

#define WORLD_ROW_BUTTON_BASE 1000
static int g_world_last_click_index = -1;
static double g_world_last_click_time = 0.0;

static void set_screen(ScreenId s) {
    ScreenId old_screen = g_screen;
    if (old_screen != s) pex_logf("screen change %d -> %d", (int)old_screen, (int)s);
    if (old_screen == SCREEN_FURNACE && s != SCREEN_FURNACE) furnace_close_open_inventory();
    if (old_screen == SCREEN_CHEST && s != SCREEN_CHEST) chest_close_open_inventory();
    g_screen = s;
    if (s == SCREEN_TITLE) {
        if (!g_release_title_state_initialized || old_screen != SCREEN_TITLE) {
            release_title_state_enter();
            g_release_title_state_initialized = 1;
        }
        if (!g_boot_sequence_done && g_title_enter_time <= 0.0) g_title_enter_time = now_seconds();
        g_menu_music_started = 0;
    } else if (s == SCREEN_GENERATING || s == SCREEN_CONNECTING || s == SCREEN_INGAME) {
        /* Menu music is a title-screen-only sound.  Stop it as soon as a local
           world load, multiplayer join, or gameplay entry starts. */
        pex_menu_music_stop();
    }
    if (s == SCREEN_WORLD_SELECT || s == SCREEN_WORLD_DELETE) {
        g_selected_world_index = -1;
        g_world_save_scroll = 0;
        g_world_drag_scroll_pixels = 0;
    }
    g_waiting_key = -1;
    g_gamepad_menu_index = 0;
    g_gamepad_virtual_cursor_active = 0;
    if (s == SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT) pack_install_start_size_fetch();
    set_mouse_grabbed(s == SCREEN_INGAME);
    clear_buttons();
    rebuild_screen();
}


static int java_string_hash_compat(const char *s) {
    int32_t h = 0;
    if (!s) return 0;
    while (*s) h = (int32_t)(31 * h + (unsigned char)*s++);
    return (int)h;
}

static void trim_ascii_in_place(char *s) {
    char *a, *b;
    if (!s) return;
    a = s;
    while (*a == ' ' || *a == '\t' || *a == '\r' || *a == '\n') a++;
    if (a != s) memmove(s, a, strlen(a) + 1);
    b = s + strlen(s);
    while (b > s && (b[-1] == ' ' || b[-1] == '\t' || b[-1] == '\r' || b[-1] == '\n')) --b;
    *b = 0;
}

static void choose_release_splash(char *out, size_t cap) {
    char pack_dir[MAX_PATHBUF], path[MAX_PATHBUF];
    FILE *f;
    char line[512];
    char chosen[MAX_LABEL] = "missingno";
    int count = 0;

    if (!out || cap == 0) return;
    out[0] = 0;
    pack_asset_path(pack_dir, sizeof(pack_dir));
    snprintf(path, sizeof(path), "%s/title/splashes.txt", pack_dir);
    f = fopen(path, "rb");
    if (!f) {
        snprintf(path, sizeof(path), "%s\\title\\splashes.txt", pack_dir);
        f = fopen(path, "rb");
    }
    if (!f) { snprintf(out, cap, "%s", chosen); return; }

    while (fgets(line, sizeof(line), f)) {
        trim_ascii_in_place(line);
        if (!line[0]) continue;
        ++count;
        if ((rand() % count) == 0) snprintf(chosen, sizeof(chosen), "%s", line);
    }
    fclose(f);
    if (count > 1 && java_string_hash_compat(chosen) == 125780783) choose_release_splash(out, cap);
    else snprintf(out, cap, "%s", chosen);
}

static int is_seasonal_splash(char *out, size_t cap) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    if (!tm) return 0;
    int month = tm->tm_mon + 1;
    int day = tm->tm_mday;
    if (month == 11 && day == 9) snprintf(out, cap, "Happy birthday, ez!");
    else if (month == 6 && day == 1) snprintf(out, cap, "Happy birthday, Notch!");
    else if (month == 12 && day == 24) snprintf(out, cap, "Merry X-mas!");
    else if (month == 1 && day == 1) snprintf(out, cap, "Happy new year!");
    else return 0;
    return 1;
}

static int compare_world_saves(const WorldSaveEntry *a, const WorldSaveEntry *b) {
    if (a->last_played < b->last_played) return 1;
    if (a->last_played > b->last_played) return -1;
    return strcmp(a->dir_name, b->dir_name);
}

static int world_save_list_top(void) { return 32; }

static int world_save_list_bottom(void) {
    int bottom = g_gui_h - 64;
    int top = world_save_list_top();
    if (bottom < top + 36) bottom = top + 36;
    return bottom;
}

static int world_save_visible_rows(void) {
    int rows = (world_save_list_bottom() - world_save_list_top() - 4) / 36;
    if (rows < 1) rows = 1;
    return rows;
}

static void clamp_world_save_scroll(void) {
    int visible_rows = world_save_visible_rows();
    int max_scroll = g_world_save_count - visible_rows;
    if (max_scroll < 0) max_scroll = 0;
    if (g_world_save_scroll < 0) g_world_save_scroll = 0;
    if (g_world_save_scroll > max_scroll) g_world_save_scroll = max_scroll;
}

static void scan_world_saves(void) {
    char selected_dir[MAX_LABEL] = "";
    if (g_selected_world_index >= 0 && g_selected_world_index < g_world_save_count) {
        snprintf(selected_dir, sizeof(selected_dir), "%s", g_world_saves[g_selected_world_index].dir_name);
    }

    g_world_save_count = 0;
    WIN32_FIND_DATAA fd;
    char pattern[MAX_PATHBUF];
    snprintf(pattern, sizeof(pattern), "%s\\*", g_save_dir);
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..")) continue;
            if (g_world_save_count >= MAX_WORLD_SAVES) break;

            WorldSaveEntry *e = &g_world_saves[g_world_save_count];
            memset(e, 0, sizeof(*e));
            snprintf(e->dir_name, sizeof(e->dir_name), "%s", fd.cFileName);
            snprintf(e->path, sizeof(e->path), "%s\\%s", g_save_dir, fd.cFileName);
            char level_path[MAX_PATHBUF];
            snprintf(level_path, sizeof(level_path), "%s\\level.dat", e->path);
            if (!file_exists(level_path)) continue;

            if (!read_level_string_tag_for_dir(e->path, "LevelName", e->display_name, sizeof(e->display_name)) || !e->display_name[0]) {
                snprintf(e->display_name, sizeof(e->display_name), "%s", e->dir_name);
            }
            read_level_long_tag_for_dir(e->path, "LastPlayed", &e->last_played);
            int binary_summary = read_level_summary(e->path, NULL, &e->world_type, e->last_played > 0 ? NULL : &e->last_played);
            long long size_on_disk = 0;
            if (read_level_long_tag_for_dir(e->path, "SizeOnDisk", &size_on_disk) && size_on_disk > 0) {
                e->size_on_disk = (unsigned long long)size_on_disk;
            } else {
                e->size_on_disk = dir_size(e->path);
            }
            if (!binary_summary) {
                e->world_type = read_world_type_for_dir(e->path);
            }
            g_world_save_count++;
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }

    for (int i = 1; i < g_world_save_count; i++) {
        WorldSaveEntry key = g_world_saves[i];
        int j = i - 1;
        while (j >= 0 && compare_world_saves(&g_world_saves[j], &key) > 0) {
            g_world_saves[j + 1] = g_world_saves[j];
            j--;
        }
        g_world_saves[j + 1] = key;
    }
    g_selected_world_index = -1;
    if (selected_dir[0]) {
        for (int i = 0; i < g_world_save_count; i++) {
            if (!strcmp(g_world_saves[i].dir_name, selected_dir)) {
                g_selected_world_index = i;
                break;
            }
        }
    }
    clamp_world_save_scroll();
}

static void world_save_scroll_by(int rows) {
    if (g_screen != SCREEN_WORLD_SELECT && g_screen != SCREEN_WORLD_DELETE) return;
    g_world_save_scroll += rows;
    clamp_world_save_scroll();
    rebuild_screen();
}

static void start_selected_world_save(void) {
    if (g_selected_world_index < 0 || g_selected_world_index >= g_world_save_count) return;
    scan_world_saves();
    if (g_selected_world_index < 0 || g_selected_world_index >= g_world_save_count) return;
    WorldSaveEntry *e = &g_world_saves[g_selected_world_index];
    g_pending_world_slot = g_selected_world_index + 1;
    g_pending_world_type = e->world_type;
    snprintf(g_pending_world_dir, sizeof(g_pending_world_dir), "%s", e->path);
    snprintf(g_pending_world_name, sizeof(g_pending_world_name), "%s", e->display_name[0] ? e->display_name : e->dir_name);
    start_world_generation_in_dir(g_pending_world_dir, g_pending_world_name, g_pending_world_slot);
}

static void confirm_delete_world_save(void) {
    if (g_selected_world_index < 0 || g_selected_world_index >= g_world_save_count) return;
    scan_world_saves();
    if (g_selected_world_index < 0 || g_selected_world_index >= g_world_save_count) return;
    WorldSaveEntry *e = &g_world_saves[g_selected_world_index];
    snprintf(g_pending_world_dir, sizeof(g_pending_world_dir), "%s", e->path);
    snprintf(g_pending_world_name, sizeof(g_pending_world_name), "%s", e->display_name[0] ? e->display_name : e->dir_name);
    g_confirm_world = g_selected_world_index + 1;
    set_screen(SCREEN_CONFIRM_DELETE);
}

static void sanitize_world_dir_name(const char *name, char *out, size_t cap) {
    if (!out || cap == 0) return;
    size_t o = 0;
    const char *s = (name && name[0]) ? name : "New World";
    while (*s && o + 1 < cap) {
        unsigned char ch = (unsigned char)*s++;
        if (ch < 32 || ch == '/' || ch == '\\' || ch == '"' || ch == ':' || ch == '*' || ch == '?' || ch == '<' || ch == '>' || ch == '|') {
            ch = '_';
        }
        out[o++] = (char)ch;
    }
    while (o > 0 && (out[o - 1] == ' ' || out[o - 1] == '.')) o--;
    out[o] = 0;
    if (!out[0]) snprintf(out, cap, "World");
}

static void make_unique_world_dir(const char *display_name, char *dir_name, size_t dir_cap, char *path, size_t path_cap) {
    char base[64];
    sanitize_world_dir_name(display_name, base, sizeof(base));
    for (int suffix = 0; suffix < 10000; suffix++) {
        if (suffix == 0) snprintf(dir_name, dir_cap, "%s", base);
        else snprintf(dir_name, dir_cap, "%s-%d", base, suffix);
        snprintf(path, path_cap, "%s\\%s", g_save_dir, dir_name);
        if (!dir_exists(path)) return;
    }
    snprintf(dir_name, dir_cap, "World-%lu", (unsigned long)GetTickCount());
    snprintf(path, path_cap, "%s\\%s", g_save_dir, dir_name);
}

static void world_save_drag_scroll(int delta_y) {
    if (g_screen != SCREEN_WORLD_SELECT && g_screen != SCREEN_WORLD_DELETE) return;
    if (delta_y == 0) return;
    g_world_drag_scroll_pixels -= delta_y;
    int rows = 0;
    while (g_world_drag_scroll_pixels >= 36) {
        rows++;
        g_world_drag_scroll_pixels -= 36;
    }
    while (g_world_drag_scroll_pixels <= -36) {
        rows--;
        g_world_drag_scroll_pixels += 36;
    }
    if (rows) world_save_scroll_by(rows);
}

static void rebuild_screen(void) {
    clear_buttons();
    if (g_screen == SCREEN_TITLE) {
        char seasonal[MAX_LABEL];
        choose_release_splash(g_splash, sizeof(g_splash));
        if (is_seasonal_splash(seasonal, sizeof(seasonal))) snprintf(g_splash, sizeof(g_splash), "%s", seasonal);
        int y0 = g_gui_h / 4 + 48;
        add_button(1, g_gui_w / 2 - 100, y0, "Singleplayer");
        add_button(2, g_gui_w / 2 - 100, y0 + 24, "Multiplayer");
        add_button(3, g_gui_w / 2 - 100, y0 + 48, "Texture Packs");
        add_button_full(0, g_gui_w / 2 - 100, y0 + 72 + 12, 98, 20, "Options...", BUTTON_NORMAL);
        add_button_full(4, g_gui_w / 2 + 2, y0 + 72 + 12, 98, 20, "Quit Game", BUTTON_NORMAL);
        add_button_full(5, g_gui_w / 2 - 124, y0 + 72 + 12, 20, 20, "", BUTTON_LANGUAGE);
    } else if (g_screen == SCREEN_OPTIONS) {
        const OptionId main_options[] = {
            OPT_MUSIC, OPT_SOUND, OPT_INVERT_MOUSE, OPT_SENSITIVITY, OPT_FOV, OPT_DIFFICULTY
        };
        for (int shown = 0; shown < ARRAY_COUNT(main_options); shown++) {
            OptionId opt = main_options[shown];
            int x = g_gui_w / 2 - 155 + (shown % 2) * 160;
            int y = g_gui_h / 6 + 24 * (shown >> 1);
            char label[MAX_LABEL];
            get_option_label(opt, label, sizeof(label));
            Button *b = add_button_full((int)opt, x, y, 150, 20, label, opt_is_slider[opt] ? BUTTON_SLIDER : BUTTON_NORMAL);
            b->opt = opt;
            if (b->kind == BUTTON_SLIDER) b->slider_value = get_option_float(opt);
        }
        add_button_full(300, g_gui_w / 2 - 100, g_gui_h / 6 + 96 - 6, 200, 20, tr("Video Settings..."), BUTTON_NORMAL);
        add_button_full(100, g_gui_w / 2 - 100, g_gui_h / 6 + 120 - 6, 200, 20, tr("Controls..."), BUTTON_NORMAL);
        add_button_full(301, g_gui_w / 2 - 100, g_gui_h / 6 + 144 - 6, 200, 20, tr("Skins..."), BUTTON_NORMAL);
        add_button_full(200, g_gui_w / 2 - 100, g_gui_h / 6 + 168, 200, 20, tr("Done"), BUTTON_NORMAL);
    } else if (g_screen == SCREEN_OPTIONS_MORE) {
        const OptionId video_options[] = {
            OPT_GRAPHICS, OPT_RENDER_DISTANCE, OPT_VIEW_BOBBING, OPT_LIMIT_FRAMERATE,
            OPT_ANAGLYPH, OPT_FULLSCREEN, OPT_SHOW_FPS, OPT_RENDERER
        };
        for (int i = 0; i < ARRAY_COUNT(video_options); i++) {
            OptionId opt = video_options[i];
            int x = g_gui_w / 2 - 155 + (i % 2) * 160;
            int y = g_gui_h / 6 + 24 * (i >> 1);
            char label[MAX_LABEL];
            get_option_label(opt, label, sizeof(label));
            Button *b = add_button_full((int)opt, x, y, 150, 20, label, opt_is_slider[opt] ? BUTTON_SLIDER : BUTTON_NORMAL);
            b->opt = opt;
            if (b->kind == BUTTON_SLIDER) b->slider_value = get_option_float(opt);
#ifdef PEX_PLATFORM_PSP
            if (opt == OPT_RENDERER) b->enabled = 0;
#endif
        }
        add_button_full(122, g_gui_w / 2 - 100, g_gui_h / 6 + 120 - 6, 200, 20, "Info...", BUTTON_NORMAL);
        add_button_full(199, g_gui_w / 2 - 100, g_gui_h / 6 + 168, 200, 20, tr("Done"), BUTTON_NORMAL);
    } else if (g_screen == SCREEN_SYSTEM_INFO) {
        add_button_full(200, g_gui_w / 2 - 100, g_gui_h - 24, 200, 20, tr("Back"), BUTTON_NORMAL);
    } else if (g_screen == SCREEN_SKINS) {
        add_button(1, g_gui_w / 2 - 100, g_gui_h - 76, tr("Import Skin..."));
        add_button_full(2, g_gui_w / 2 - 100, g_gui_h - 52, 98, 20, tr("Use Default"), BUTTON_NORMAL);
        add_button_full(200, g_gui_w / 2 + 2, g_gui_h - 52, 98, 20, tr("Done"), BUTTON_NORMAL);
    } else if (g_screen == SCREEN_CONTROLS) {
        int x0 = g_gui_w / 2 - 155;
        for (int i = 0; i < 10; i++) {
            char kn[64], label[96];
            snprintf(label, sizeof(label), "%s", key_name(g_opts.keys[i], kn, sizeof(kn)));
            add_button_full(i, x0 + (i % 2) * 160, g_gui_h / 6 + 24 * (i >> 1), 70, 20, label, BUTTON_NORMAL);
        }
        add_button(200, g_gui_w / 2 - 100, g_gui_h / 6 + 168, tr("Done"));
    } else if (g_screen == SCREEN_WORLD_TYPE) {
        add_button(0, g_gui_w / 2 - 100, g_gui_h / 4 + 48, "Flat");
        add_button(1, g_gui_w / 2 - 100, g_gui_h / 4 + 72, "Normal");
        add_button(2, g_gui_w / 2 - 100, g_gui_h / 4 + 108, "Cancel");
    } else if (g_screen == SCREEN_WORLD_SELECT || g_screen == SCREEN_WORLD_DELETE) {
        scan_world_saves();
        int visible_rows = world_save_visible_rows();
        if (visible_rows > g_world_save_count - g_world_save_scroll) visible_rows = g_world_save_count - g_world_save_scroll;
        if (visible_rows < 0) visible_rows = 0;
        int bottom = world_save_list_bottom();
        for (int i = 0; i < visible_rows; i++) {
            int y = world_save_list_top() + 4 + i * 36;
            int h = 32;
            if (y + h > bottom) h = bottom - y;
            if (h > 0) add_button_full(WORLD_ROW_BUTTON_BASE + g_world_save_scroll + i, g_gui_w / 2 - 110, y, 220, h, "", BUTTON_HITBOX);
        }
        if (g_screen == SCREEN_WORLD_SELECT) {
            Button *select = add_button_full(10, g_gui_w / 2 - 154, g_gui_h - 52, 150, 20, "Play Selected World", BUTTON_NORMAL);
            Button *delete = add_button_full(5, g_gui_w / 2 - 154, g_gui_h - 28, 150, 20, "Delete", BUTTON_NORMAL);
            add_button_full(11, g_gui_w / 2 + 4, g_gui_h - 52, 150, 20, "Create New World", BUTTON_NORMAL);
            add_button_full(6, g_gui_w / 2 + 4, g_gui_h - 28, 150, 20, "Cancel", BUTTON_NORMAL);
            select->enabled = g_selected_world_index >= 0 && g_selected_world_index < g_world_save_count;
            delete->enabled = g_selected_world_index >= 0 && g_selected_world_index < g_world_save_count;
        } else {
            Button *delete = add_button_full(10, g_gui_w / 2 - 154, g_gui_h - 52, 150, 20, "Delete", BUTTON_NORMAL);
            add_button_full(6, g_gui_w / 2 + 4, g_gui_h - 52, 150, 20, "Cancel", BUTTON_NORMAL);
            delete->enabled = g_selected_world_index >= 0 && g_selected_world_index < g_world_save_count;
        }
    } else if (g_screen == SCREEN_CONFIRM_DELETE) {
        add_button_full(0, g_gui_w / 2 - 155 + 0, g_gui_h / 6 + 96, 150, 20, "Yes", BUTTON_NORMAL);
        add_button_full(1, g_gui_w / 2 - 155 + 160, g_gui_h / 6 + 96, 150, 20, "No", BUTTON_NORMAL);
    } else if (g_screen == SCREEN_MULTIPLAYER) {
        Button *connect = add_button(0, g_gui_w / 2 - 100, g_gui_h / 4 + 120 + 12, "Connect");
        connect->enabled = (int)strlen(g_multiplayer_ip) > 0;
        add_button(1, g_gui_w / 2 - 100, g_gui_h / 4 + 144 + 12, "Cancel");
    } else if (g_screen == SCREEN_CONNECTING) {
        add_button(0, g_gui_w / 2 - 100, g_gui_h / 4 + 120 + 12, "Cancel");
    } else if (g_screen == SCREEN_TEXPACK) {
        scan_texture_packs();
        add_button_full(5, g_gui_w / 2 - 154, g_gui_h - 48, 150, 20, "Open texture pack folder", BUTTON_NORMAL);
        add_button_full(6, g_gui_w / 2 + 4, g_gui_h - 48, 150, 20, "Done", BUTTON_NORMAL);
    } else if (g_screen == SCREEN_PAUSE) {
        add_button(4, g_gui_w / 2 - 100, g_gui_h / 4 + 24, "Back to game");
        add_button(1, g_gui_w / 2 - 100, g_gui_h / 4 + 48, g_mp_connected ? "Disconnect" : "Save and quit to title");
        add_button(0, g_gui_w / 2 - 100, g_gui_h / 4 + 96, "Options...");
    } else if (g_screen == SCREEN_DEATH) {
        add_button(1, g_gui_w / 2 - 100, g_gui_h / 4 + 72, "Respawn");
        add_button(2, g_gui_w / 2 - 100, g_gui_h / 4 + 96, "Title menu");
    } else if (g_screen == SCREEN_NOTICE) {
        add_button(0, g_gui_w / 2 - 100, g_gui_h / 4 + 120 + 12, "Back to title screen");
    } else if (g_screen == SCREEN_RENDERER_RESTART_PROMPT) {
        add_button_full(0, g_gui_w / 2 - 155, g_gui_h / 4 + 120 + 12, 150, 20, "Restart", BUTTON_NORMAL);
        add_button_full(1, g_gui_w / 2 + 5, g_gui_h / 4 + 120 + 12, 150, 20, "Ignore", BUTTON_NORMAL);
    } else if (g_screen == SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT) {
#if PEX_CLASSIC_SOUND_DOWNLOAD_SUPPORTED
        char snd_label[MAX_LABEL];
        snprintf(snd_label, sizeof(snd_label), "Music: Moog City 2");
        add_button_full(2, g_gui_w / 2 - 100, g_gui_h / 4 + 112, 200, 20, snd_label, BUTTON_NORMAL);
        add_button_full(0, g_gui_w / 2 - 155, g_gui_h / 4 + 140, 150, 20, "Download", BUTTON_NORMAL);
        add_button_full(1, g_gui_w / 2 + 5, g_gui_h / 4 + 140, 150, 20, "Ignore", BUTTON_NORMAL);
#else
        add_button_full(0, g_gui_w / 2 - 155, g_gui_h / 4 + 120 + 12, 150, 20, "Download", BUTTON_NORMAL);
        add_button_full(1, g_gui_w / 2 + 5, g_gui_h / 4 + 120 + 12, 150, 20, "Ignore", BUTTON_NORMAL);
#endif
    } else if (g_screen == SCREEN_CLASSIC_PACK_WARNING) {
        add_button_full(0, g_gui_w / 2 - 155, g_gui_h / 4 + 120 + 12, 150, 20, "Yes", BUTTON_NORMAL);
        add_button_full(1, g_gui_w / 2 + 5, g_gui_h / 4 + 120 + 12, 150, 20, "No", BUTTON_NORMAL);
    }
}

static void update_slider(Button *b, int mx) {
    if (!b || b->kind != BUTTON_SLIDER) return;
    float v = (float)(mx - (b->x + 4)) / (float)(b->w - 8);
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    b->slider_value = v;
    set_option_float(b->opt, v);
    get_option_label(b->opt, b->label, sizeof(b->label));
}

static void open_notice(const char *title, const char *line1, const char *line2) {
    snprintf(g_notice_title, sizeof(g_notice_title), "%s", title ? title : "");
    snprintf(g_notice_line1, sizeof(g_notice_line1), "%s", line1 ? line1 : "");
    snprintf(g_notice_line2, sizeof(g_notice_line2), "%s", line2 ? line2 : "");
    set_screen(SCREEN_NOTICE);
}


static void finish_options_to(ScreenId target) {
    if (g_selected_renderer_backend != g_opts.renderer_backend) {
        g_renderer_prompt_target_screen = target;
        set_screen(SCREEN_RENDERER_RESTART_PROMPT);
    } else {
        set_screen(target);
    }
}

static void restart_application_now(void) {
#ifdef PEX_PLATFORM_PSP
    save_current_world_state();
    g_running = 0;
    return;
#else
    char exe[MAX_PATHBUF];
    DWORD n = GetModuleFileNameA(NULL, exe, sizeof(exe));
    save_current_world_state();
    set_mouse_grabbed(0);
    if (n > 0 && n < sizeof(exe)) {
        ShellExecuteA(NULL, "open", exe, NULL, NULL, SW_SHOWNORMAL);
    }
    PostQuitMessage(0);
#endif
}

static void on_button(Button *b) {
    if (!b || !b->enabled) return;
    if (g_screen == SCREEN_TITLE) {
        if (!g_boot_sequence_done && g_title_enter_time > 0.0 && now_seconds() - g_title_enter_time < 3.0) return;
        if (b->id == 0) { g_parent_screen = SCREEN_TITLE; set_screen(SCREEN_OPTIONS); }
        else if (b->id == 1) { g_parent_screen = SCREEN_TITLE; set_screen(SCREEN_WORLD_SELECT); }
        else if (b->id == 2) { g_parent_screen = SCREEN_TITLE; set_screen(SCREEN_MULTIPLAYER); }
        else if (b->id == 3) { g_parent_screen = SCREEN_TITLE; set_screen(SCREEN_TEXPACK); }
        else if (b->id == 5) { g_parent_screen = SCREEN_TITLE; set_screen(SCREEN_OPTIONS); }
        else if (b->id == 4) {
#ifdef PEX_PLATFORM_PSP
            g_running = 0;
#else
            PostQuitMessage(0);
#endif
        }
    } else if (g_screen == SCREEN_OPTIONS) {
        if (b->id < 100) {
            if (b->kind == BUTTON_NORMAL) {
                bump_option(b->opt, 1);
                get_option_label(b->opt, b->label, sizeof(b->label));
            }
        } else if (b->id == 100) set_screen(SCREEN_CONTROLS);
        else if (b->id == 200) finish_options_to(g_parent_screen);
        else if (b->id == 300) set_screen(SCREEN_OPTIONS_MORE);
        else if (b->id == 301) set_screen(SCREEN_SKINS);
    } else if (g_screen == SCREEN_OPTIONS_MORE) {
        if (b->id < 100) {
            if (b->kind == BUTTON_NORMAL) {
                bump_option(b->opt, 1);
                get_option_label(b->opt, b->label, sizeof(b->label));
            }
        } else if (b->id == 122) set_screen(SCREEN_SYSTEM_INFO);
        else if (b->id == 199) { save_options(); set_screen(SCREEN_OPTIONS); }
    } else if (g_screen == SCREEN_SYSTEM_INFO) {
        if (b->id == 200) set_screen(SCREEN_OPTIONS_MORE);
    } else if (g_screen == SCREEN_SKINS) {
        if (b->id == 1) { if (choose_and_import_skin()) set_screen(SCREEN_SKINS); }
        else if (b->id == 2) {
            g_opts.skin_path[0] = 0;
            load_default_textures();
            apply_texture_pack_index(g_selected_texpack);
            save_options();
            set_screen(SCREEN_SKINS);
        } else if (b->id == 200) set_screen(SCREEN_OPTIONS);
    } else if (g_screen == SCREEN_CONTROLS) {
        if (b->id == 200) { save_options(); set_screen(SCREEN_OPTIONS); }
        else {
            g_waiting_key = b->id;
            for (int i = 0; i < 10; i++) {
                char kn[64];
                snprintf(g_buttons[i].label, sizeof(g_buttons[i].label), "%s", key_name(g_opts.keys[i], kn, sizeof(kn)));
            }
            char old[64];
            snprintf(b->label, sizeof(b->label), "> %s <", key_name(g_opts.keys[b->id], old, sizeof(old)));
        }
    } else if (g_screen == SCREEN_WORLD_SELECT) {
        if (b->id >= WORLD_ROW_BUTTON_BASE && b->id < WORLD_ROW_BUTTON_BASE + g_world_save_count) {
            int clicked = b->id - WORLD_ROW_BUTTON_BASE;
            double click_time = now_seconds();
            int double_click = (clicked == g_world_last_click_index && click_time - g_world_last_click_time < 0.25);
            g_selected_world_index = b->id - WORLD_ROW_BUTTON_BASE;
            g_world_last_click_index = clicked;
            g_world_last_click_time = click_time;
            if (double_click) {
                start_selected_world_save();
                return;
            }
            rebuild_screen();
        } else if (b->id == 10 && g_selected_world_index >= 0 && g_selected_world_index < g_world_save_count) {
            start_selected_world_save();
        } else if (b->id == 5) confirm_delete_world_save();
        else if (b->id == 11) {
            char dir_name[64];
            make_unique_world_dir("New World", dir_name, sizeof(dir_name), g_pending_world_dir, sizeof(g_pending_world_dir));
            snprintf(g_pending_world_name, sizeof(g_pending_world_name), "New World");
            g_pending_world_slot = g_world_save_count + 1;
            g_pending_world_type = 0;
            set_screen(SCREEN_WORLD_TYPE);
        }
        else if (b->id == 6) set_screen(g_parent_screen);
    } else if (g_screen == SCREEN_WORLD_TYPE) {
        if (b->id == 0 || b->id == 1) {
            g_pending_world_type = b->id;
            if (!g_pending_world_dir[0]) {
                char dir_name[64];
                make_unique_world_dir("New World", dir_name, sizeof(dir_name), g_pending_world_dir, sizeof(g_pending_world_dir));
                snprintf(g_pending_world_name, sizeof(g_pending_world_name), "New World");
            }
            if (g_pending_world_slot <= 0) g_pending_world_slot = g_world_save_count + 1;
            start_world_generation_in_dir(g_pending_world_dir, g_pending_world_name, g_pending_world_slot);
        } else if (b->id == 2) {
            g_pending_world_dir[0] = 0;
            g_pending_world_name[0] = 0;
            set_screen(SCREEN_WORLD_SELECT);
        }
    } else if (g_screen == SCREEN_WORLD_DELETE) {
        if (b->id >= WORLD_ROW_BUTTON_BASE && b->id < WORLD_ROW_BUTTON_BASE + g_world_save_count) {
            g_selected_world_index = b->id - WORLD_ROW_BUTTON_BASE;
            rebuild_screen();
        } else if (b->id == 10 && g_selected_world_index >= 0 && g_selected_world_index < g_world_save_count) {
            confirm_delete_world_save();
        } else if (b->id == 6) set_screen(SCREEN_WORLD_SELECT);
    } else if (g_screen == SCREEN_CONFIRM_DELETE) {
        if (b->id == 0 && g_pending_world_dir[0]) {
            if (dir_exists(g_pending_world_dir)) delete_recursive(g_pending_world_dir);
            if (!strcmp(g_pending_world_dir, g_loaded_world_dir)) {
                g_pending_world_slot = 0;
                g_pending_world_type = 0;
            }
        }
        g_confirm_world = 0;
        g_pending_world_dir[0] = 0;
        g_pending_world_name[0] = 0;
        set_screen(SCREEN_WORLD_SELECT);
    } else if (g_screen == SCREEN_MULTIPLAYER) {
        if (b->id == 1) set_screen(g_parent_screen);
        else if (b->id == 0) {
            snprintf(g_opts.last_server, sizeof(g_opts.last_server), "%s", g_multiplayer_ip);
            snprintf(g_opts.username, sizeof(g_opts.username), "%s", g_multiplayer_username[0] ? g_multiplayer_username : "Player");
            save_options();
            set_screen(SCREEN_CONNECTING);
            if (!pex_net_connect_to_server(g_multiplayer_ip)) {
                open_notice("Connection failed", g_multiplayer_status[0] ? g_multiplayer_status : "Could not connect to server.", "");
            }
        }
    } else if (g_screen == SCREEN_CONNECTING) {
        pex_net_disconnect();
        pex_net_set_status("Canceled.");
        set_screen(SCREEN_MULTIPLAYER);
    } else if (g_screen == SCREEN_TEXPACK) {
        if (b->id == 5) ShellExecuteA(NULL, "open", g_texpack_dir, NULL, NULL, SW_SHOWNORMAL);
        else if (b->id == 6) set_screen(g_parent_screen);
    } else if (g_screen == SCREEN_PAUSE) {
        if (b->id == 4) set_screen(SCREEN_INGAME);
        else if (b->id == 0) { g_parent_screen = SCREEN_PAUSE; set_screen(SCREEN_OPTIONS); }
        else if (b->id == 1) leave_world_to_title();
    } else if (g_screen == SCREEN_DEATH) {
        if (b->id == 1) player_respawn();
        else if (b->id == 2) leave_world_to_title();
    } else if (g_screen == SCREEN_NOTICE) {
        set_screen(SCREEN_TITLE);
    } else if (g_screen == SCREEN_RENDERER_RESTART_PROMPT) {
        if (b->id == 0) {
            g_opts.renderer_backend = g_selected_renderer_backend;
            save_options();
            restart_application_now();
        } else {
            g_selected_renderer_backend = g_opts.renderer_backend;
            set_screen((ScreenId)g_renderer_prompt_target_screen);
        }
    } else if (g_screen == SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT) {
        if (b->id == 0) {
            pack_install_start();
        } else if (b->id == 2) {
            g_opts.download_classic_sounds = !g_opts.download_classic_sounds;
            if (g_opts.download_classic_sounds) g_opts.ignore_classic_sounds_warning = 0;
            InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_UNKNOWN);
            save_options();
            if (classic_resources_need_update()) set_screen(SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT);
            else set_screen(SCREEN_TITLE);
        } else {
            if (!pack_is_installed() || pack_missing_required_textures()) g_opts.ignore_classic_resources_warning = 1;
            if (g_opts.download_classic_sounds && !classic_sounds_installed()) g_opts.ignore_classic_sounds_warning = 1;
            save_options();
            set_screen(SCREEN_TITLE);
        }
    } else if (g_screen == SCREEN_CLASSIC_PACK_WARNING) {
        if (b->id == 0) pack_install_start();
        else set_screen(SCREEN_TITLE);
    }
}

static void texpack_mouse_down(int mx, int my) {
    int top = 32;
    int bottom = g_gui_h - 58 + 4;
    if (my >= top && my <= bottom) {
        int left = g_gui_w / 2 - 110;
        int right = g_gui_w / 2 + 110;
        int idx = (my - top + g_texpack_scroll - 2) / 36;
        if (mx >= left && mx <= right && idx >= 0 && idx < g_texpack_count) {
            TexturePackEntry *e = &g_texpacks[idx];
            if (e->is_builtin_classic && !pack_is_installed()) {
                pack_install_start();
                return;
            } else if (e->is_builtin_classic && classic_resources_need_update()) {
                set_screen(SCREEN_CLASSIC_PACK_WARNING);
                return;
            } else if (idx != g_selected_texpack) {
                apply_texture_pack_index(idx);
            }
        }
        g_texpack_drag_anchor = my;
    } else {
        g_texpack_drag_anchor = -2;
    }
}

static void texpack_mouse_drag(int my) {
    if (g_texpack_drag_anchor >= 0) {
        g_texpack_scroll -= (my - g_texpack_drag_anchor);
        g_texpack_drag_anchor = my;
        clamp_texpack_scroll();
    }
}

static void mouse_right_down(int mx, int my) {
    if (g_screen == SCREEN_DEATH) return;

    if (g_screen == SCREEN_INVENTORY || g_screen == SCREEN_WORKBENCH || g_screen == SCREEN_FURNACE || g_screen == SCREEN_CHEST) { inventory_mouse_click(mx, my, 1); return; }
    if (g_screen == SCREEN_INGAME) { set_mouse_grabbed(1); if (passive_mobs_player_interact()) return; ingame_right_click(); return; }
    (void)mx; (void)my;
}

static void mouse_down(int mx, int my) {
    g_mouse_down = 1;
    if (g_screen == SCREEN_INVENTORY || g_screen == SCREEN_WORKBENCH || g_screen == SCREEN_FURNACE || g_screen == SCREEN_CHEST) { inventory_mouse_click(mx, my, 0); return; }
    if (g_screen == SCREEN_INGAME) {
        set_mouse_grabbed(1);
        if (pex_net_try_attack_player()) return;
        if (passive_mobs_attack_from_player()) return;
        FlatRayHit hit = flat_raycast();
        if (hit.hit) restart_hand_swing();
        else start_air_swing_once();
        return;
    }
    if (g_screen == SCREEN_MULTIPLAYER) {
        int x = g_gui_w / 2 - 100;
        int server_y = g_gui_h / 4 - 10 + 50 + 18;
        int name_y = server_y + 42;
        if (mx >= x - 1 && mx < x + 201 && my >= server_y - 1 && my < server_y + 21) {
            g_multiplayer_edit_field = 0;
            rebuild_screen();
        }
        if (mx >= x - 1 && mx < x + 201 && my >= name_y - 1 && my < name_y + 21) {
            g_multiplayer_edit_field = 1;
            rebuild_screen();
        }
    }
    int hit_button = 0;
    for (int i = 0; i < g_button_count; i++) {
        Button *b = &g_buttons[i];
        if (button_hover(b, mx, my) && b->enabled) {
            hit_button = 1;
            if (b->kind == BUTTON_SLIDER) {
                g_drag_slider = b;
                b->dragging = 1;
                update_slider(b, mx);
            }
            pex_sound_play("random.click", 1.0f, 1.0f);
            on_button(b);
            break;
        }
    }
    if (!hit_button && g_screen == SCREEN_TEXPACK) texpack_mouse_down(mx, my);
}

static void mouse_up(int mx, int my) {
    (void)mx; (void)my;
    if (g_screen == SCREEN_INGAME) {
        world_left_mouse_released();
        g_air_swing_consumed = 0;
    }
    g_mouse_down = 0;
    if (g_drag_slider) g_drag_slider->dragging = 0;
    g_drag_slider = NULL;
    if (g_screen == SCREEN_TEXPACK) g_texpack_drag_anchor = -1;
}

static void key_to_control(int vk) {
    if (g_screen == SCREEN_CONTROLS && g_waiting_key >= 0 && g_waiting_key < 10) {
        g_opts.keys[g_waiting_key] = vk;
        save_options();
        g_waiting_key = -1;
        rebuild_screen();
    }
}

static int pex_chat_word_equals_ci(const char *a, const char *b) {
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca != cb) return 0;
        ++a; ++b;
    }
    return *a == 0 && *b == 0;
}

static void pex_set_world_time_to_day_tick(long long day_tick) {
    day_tick %= 24000LL;
    if (day_tick < 0) day_tick += 24000LL;
    long long day = g_world_time / 24000LL;
    if (g_world_time < 0 && (g_world_time % 24000LL) != 0) day--;
    g_world_time = day * 24000LL + day_tick;
    if (g_world_time < 0) g_world_time = day_tick;
    g_prof_skylight_subtracted_last = flat_skylight_subtracted();
    g_save_dirty = 1;
}

static int handle_local_chat_command(const char *text) {
    if (!text || text[0] != '/') return 0;
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "%s", text + 1);
    char *argv[4] = {0,0,0,0};
    int argc = 0;
    char *p = cmd;
    while (*p && argc < 4) {
        while (*p == ' ') ++p;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ') ++p;
        if (*p) *p++ = 0;
    }
    if (argc >= 1 && pex_chat_word_equals_ci(argv[0], "time")) {
        if (argc == 1) {
            char msg[96];
            snprintf(msg, sizeof(msg), "Time is %lld (%lld day ticks).", g_world_time, ((g_world_time % 24000LL) + 24000LL) % 24000LL);
            hud_add_chat(msg);
            return 1;
        }
        if (argc >= 3 && pex_chat_word_equals_ci(argv[1], "set")) {
            long long t = 0;
            int parsed_time = 1;
            if (pex_chat_word_equals_ci(argv[2], "day")) t = 1000;
            else if (pex_chat_word_equals_ci(argv[2], "night")) t = 13000;
            else if (pex_chat_word_equals_ci(argv[2], "noon")) t = 6000;
            else if (pex_chat_word_equals_ci(argv[2], "midnight")) t = 18000;
            else if (pex_chat_word_equals_ci(argv[2], "sunrise")) t = 23000;
            else if (pex_chat_word_equals_ci(argv[2], "sunset")) t = 12000;
            else {
                char *endp = argv[2];
                long long v = 0;
                int neg = 0;
                parsed_time = 0;
                if (*endp == '-') { neg = 1; ++endp; }
                while (*endp >= '0' && *endp <= '9') { v = v * 10 + (long long)(*endp - '0'); ++endp; }
                if (*endp == 0 && endp != argv[2] + neg) { t = neg ? -v : v; parsed_time = 1; }
            }
            if (parsed_time) {
                pex_set_world_time_to_day_tick(t);
                char msg[96];
                snprintf(msg, sizeof(msg), "Set time to %lld.", ((g_world_time % 24000LL) + 24000LL) % 24000LL);
                hud_add_chat(msg);
            } else {
                hud_add_chat("Usage: /time set <day|night|noon|midnight|sunrise|sunset|ticks>");
            }
            return 1;
        }
        hud_add_chat("Usage: /time or /time set <day|night|noon|midnight|sunrise|sunset|ticks>");
        return 1;
    }
    return 0;
}

static void handle_keydown(WPARAM vk) {
    if (vk == VK_F11) { toggle_fullscreen(); return; }
    if (g_screen == SCREEN_CONTROLS && g_waiting_key >= 0) { key_to_control((int)vk); return; }

    if (g_screen == SCREEN_CHAT) {
        if (vk == VK_ESCAPE) { g_chat_input[0] = 0; set_screen(SCREEN_INGAME); return; }
        if (vk == VK_RETURN) {
            if (strlen(g_chat_input) > 0) {
                if (!handle_local_chat_command(g_chat_input)) {
                    if (g_mp_connected) pex_net_send_chat(g_chat_input);
                    else {
                        char line[180];
                        snprintf(line, sizeof(line), "<Player> %s", g_chat_input);
                        hud_add_chat(line);
                    }
                }
            }
            g_chat_input[0] = 0;
            set_screen(SCREEN_INGAME);
            return;
        }
        if (vk == VK_BACK) {
            size_t len = strlen(g_chat_input);
            if (len > 0) g_chat_input[len - 1] = 0;
            return;
        }
        return;
    }

    if (g_screen == SCREEN_INGAME) {
        if (vk == VK_ESCAPE) { set_screen(SCREEN_PAUSE); return; }
        if (vk == 'V' && key_down_vk(VK_F3)) {
            g_debug_menu_shown = 1;
            g_debug_task_info_shown = 0;
            g_debug_chunk_info_shown = !g_debug_chunk_info_shown;
            return;
        }
        if (vk == 'J' && key_down_vk(VK_F3)) {
            g_debug_menu_shown = 1;
            g_debug_chunk_info_shown = 0;
            g_debug_task_info_shown = !g_debug_task_info_shown;
            return;
        }
        if (vk == VK_F3) { g_debug_menu_shown = !g_debug_menu_shown; return; }
        if (g_debug_menu_shown && vk == 'X') { player_add_experience(key_down_vk(VK_SHIFT) ? 10 : 1); g_save_dirty = 1; return; }
        if (vk == VK_F5) { third_person_view_cycle(); return; }
        if ((int)vk == g_opts.keys[6]) { inventory_drop_selected_one(); return; }
        if ((int)vk == g_opts.keys[7]) { set_screen(SCREEN_INVENTORY); return; }
        if ((int)vk == g_opts.keys[8]) { g_chat_input[0] = 0; g_suppress_next_chat_char = 1; set_screen(SCREEN_CHAT); return; }
        if (vk >= '1' && vk <= '9') { g_selected_hotbar_slot = (int)(vk - '1'); return; }
        return;
    }

    if (g_screen == SCREEN_INVENTORY || g_screen == SCREEN_WORKBENCH || g_screen == SCREEN_FURNACE || g_screen == SCREEN_CHEST) {
        if (vk == VK_ESCAPE || (int)vk == g_opts.keys[7]) { set_screen(SCREEN_INGAME); return; }
        return;
    }

    if (vk == VK_ESCAPE) {
        if (g_screen == SCREEN_TITLE || g_screen == SCREEN_GENERATING || g_screen == SCREEN_TEXPACK_INSTALL) return;
        if (g_screen == SCREEN_PAUSE) set_screen(SCREEN_INGAME);
        else if (g_screen == SCREEN_OPTIONS) set_screen(g_parent_screen);
        else if (g_screen == SCREEN_OPTIONS_MORE) set_screen(SCREEN_OPTIONS);
        else if (g_screen == SCREEN_SYSTEM_INFO) set_screen(SCREEN_OPTIONS_MORE);
        else if (g_screen == SCREEN_SKINS) set_screen(SCREEN_OPTIONS);
        else if (g_screen == SCREEN_CONTROLS) set_screen(SCREEN_OPTIONS);
        else if (g_screen == SCREEN_WORLD_SELECT) set_screen(g_parent_screen);
        else if (g_screen == SCREEN_WORLD_TYPE) set_screen(SCREEN_WORLD_SELECT);
        else if (g_screen == SCREEN_WORLD_DELETE) set_screen(SCREEN_WORLD_SELECT);
        else if (g_screen == SCREEN_CONFIRM_DELETE) set_screen(SCREEN_WORLD_SELECT);
        else if (g_screen == SCREEN_MULTIPLAYER) set_screen(g_parent_screen);
        else if (g_screen == SCREEN_CONNECTING) { pex_net_disconnect(); set_screen(SCREEN_MULTIPLAYER); }
        else if (g_screen == SCREEN_TEXPACK) set_screen(g_parent_screen);
        else if (g_screen == SCREEN_NOTICE) set_screen(SCREEN_TITLE);
        else if (g_screen == SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT) {
            g_opts.ignore_classic_resources_warning = 1;
            save_options();
            set_screen(SCREEN_TITLE);
        }
        else if (g_screen == SCREEN_CLASSIC_PACK_WARNING) set_screen(SCREEN_TITLE);
        return;
    }
    if (g_screen == SCREEN_MULTIPLAYER) {
        char *field = g_multiplayer_edit_field == 1 ? g_multiplayer_username : g_multiplayer_ip;
        size_t len = strlen(field);
        if (vk == VK_TAB) {
            g_multiplayer_edit_field = 1 - g_multiplayer_edit_field;
            rebuild_screen();
        } else if (vk == VK_BACK && len > 0) {
            field[len - 1] = 0;
            rebuild_screen();
        } else if (vk == VK_RETURN && strlen(g_multiplayer_ip) > 0) {
            on_button(&g_buttons[0]);
        }
    }
}

static void handle_char(WPARAM ch) {
    if (g_screen == SCREEN_CHAT) {
        if (g_suppress_next_chat_char) { g_suppress_next_chat_char = 0; return; }
        if (ch == 13 || ch == 8 || ch == 27) return;
        const char *allowed = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_'abcdefghijklmnopqrstuvwxyz{|}~";
        if (strchr(allowed, (char)ch) && strlen(g_chat_input) < 100) {
            size_t len = strlen(g_chat_input);
            g_chat_input[len] = (char)ch;
            g_chat_input[len + 1] = 0;
        }
        return;
    }
    if (g_screen == SCREEN_MULTIPLAYER) {
        if (ch == 13 || ch == 8 || ch == 9) return;
        const char *server_allowed = ".-_:0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        const char *name_allowed = "_0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        char *field = g_multiplayer_edit_field == 1 ? g_multiplayer_username : g_multiplayer_ip;
        size_t cap = g_multiplayer_edit_field == 1 ? sizeof(g_multiplayer_username) : sizeof(g_multiplayer_ip);
        const char *allowed = g_multiplayer_edit_field == 1 ? name_allowed : server_allowed;
        if (strchr(allowed, (char)ch) && strlen(field) + 1 < cap) {
            size_t len = strlen(field);
            field[len] = (char)ch;
            field[len + 1] = 0;
            rebuild_screen();
        }
    }
}

static void draw_all_buttons(void) {
    for (int i = 0; i < g_button_count; i++) draw_button(&g_buttons[i]);
}
