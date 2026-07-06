/* Split from original monolithic main.c. Included by src/main.c unity build. */

static void rebuild_screen(void);
static void release_title_state_enter(void);
static ScreenId pex_startup_screen(void);
static void pex_ui_text_input_end(void);
static void pex_ui_text_input_begin_gui_rect(int x, int y, int w, int h);
static void pex_ui_text_input_begin_for_current_field(void);
static void create_world_update_folder(void);
static int handle_local_chat_command(const char *text);
static void pex_virtual_keyboard_prepare(ScreenId return_screen);
static int pex_virtual_keyboard_enabled(void);

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
    if (s == SCREEN_LANGUAGE && old_screen != SCREEN_LANGUAGE) language_ensure_selected_visible();
    if (s == SCREEN_SET_NAME && old_screen != SCREEN_SET_NAME && !g_name_edit_text[0])
        snprintf(g_name_edit_text, sizeof(g_name_edit_text), "%s", g_opts.username[0] ? g_opts.username : "");
    if (s == SCREEN_CREATE_WORLD) g_create_edit_field = g_create_more_options ? 1 : 0;
    g_waiting_key = -1;
    g_gamepad_menu_index = 0;
    g_gamepad_virtual_cursor_active = 0;
    if (s == SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT) pack_install_start_size_fetch();
    set_mouse_grabbed(s == SCREEN_INGAME);
    pex_ui_text_input_end();
    clear_buttons();
    rebuild_screen();
    if (s == SCREEN_CHAT) {
#if defined(PEX_PLATFORM_XBOX_UWP)
        pex_virtual_keyboard_prepare(SCREEN_CHAT);
        g_screen = SCREEN_VIRTUAL_KEYBOARD;
        rebuild_screen();
#else
        pex_ui_text_input_begin_gui_rect(4, g_gui_h - 16, g_gui_w - 8, 14);
#endif
    }
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


static int pex_valid_nickname_char(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

static void pex_sanitize_nickname_in_place(char *s) {
    char out[32];
    int n = 0;
    if (!s) return;
    for (int i = 0; s[i] && n < 16; ++i) {
        if (pex_valid_nickname_char(s[i])) out[n++] = s[i];
    }
    out[n] = 0;
    snprintf(s, 32, "%s", out);
}

static int pex_nickname_valid(const char *s) {
    size_t n;
    if (!s) return 0;
    n = strlen(s);
    if (n < 3 || n > 16) return 0;
    if (!strcmp(s, "rcon") || !strcmp(s, "RCON") || !strcmp(s, "console") || !strcmp(s, "CONSOLE")) return 0;
    for (size_t i = 0; i < n; ++i) if (!pex_valid_nickname_char(s[i])) return 0;
    return 1;
}

static void pex_name_screen_prepare(ScreenId return_screen, int first_run) {
    g_name_return_screen = return_screen;
    g_name_screen_first_run = first_run ? 1 : 0;
    snprintf(g_name_edit_text, sizeof(g_name_edit_text), "%s", g_opts.username[0] ? g_opts.username : "Player");
    if (!g_opts.name_set && !strcmp(g_name_edit_text, "Player")) g_name_edit_text[0] = 0;
}

static int g_ui_text_input_active = 0;

static void pex_ui_text_input_end(void) {
#if defined(PEX_PLATFORM_SDL2)
    if (g_ui_text_input_active) SDL_StopTextInput();
#endif
    g_ui_text_input_active = 0;
}

static void pex_ui_text_input_begin_gui_rect(int x, int y, int w, int h) {
#if defined(PEX_PLATFORM_SDL2)
    SDL_Rect r;
    int ww = g_win_w ? g_win_w : 1;
    int wh = g_win_h ? g_win_h : 1;
    int gw = g_gui_w ? g_gui_w : 1;
    int gh = g_gui_h ? g_gui_h : 1;
    r.x = x * ww / gw;
    r.y = y * wh / gh;
    r.w = w * ww / gw;
    r.h = h * wh / gh;
    SDL_SetTextInputRect(&r);
    SDL_StartTextInput();
#endif
    g_ui_text_input_active = 1;
}

static int pex_virtual_keyboard_enabled(void) {
#if defined(PEX_PLATFORM_XBOX_UWP)
    return 1;
#else
    return 0;
#endif
}

static void pex_virtual_keyboard_prepare(ScreenId return_screen) {
    g_virtual_keyboard_return_screen = return_screen;
    g_virtual_keyboard_row = 0;
    g_virtual_keyboard_col = 0;
    g_virtual_keyboard_caps = 0;
    g_ui_text_input_active = 1;
}

static char *pex_virtual_keyboard_target(size_t *cap) {
    if (cap) *cap = 0;
    switch (g_virtual_keyboard_return_screen) {
        case SCREEN_CHAT: if (cap) *cap = sizeof(g_chat_input); return g_chat_input;
        case SCREEN_SET_NAME: if (cap) *cap = sizeof(g_name_edit_text); return g_name_edit_text;
        case SCREEN_CREATE_WORLD: if (g_create_more_options) { if (cap) *cap = sizeof(g_pending_seed_text); return g_pending_seed_text; } if (cap) *cap = sizeof(g_pending_world_name); return g_pending_world_name;
        case SCREEN_RENAME_WORLD: if (cap) *cap = sizeof(g_rename_world_text); return g_rename_world_text;
        case SCREEN_MULTIPLAYER: if (pex_mp_server_edit_field_get() == 1) { if (cap) *cap = sizeof(g_mp_server_edit_name); return g_mp_server_edit_name; } if (cap) *cap = sizeof(g_mp_server_edit_address); return g_mp_server_edit_address;
        default: return NULL;
    }
}

static int pex_virtual_keyboard_char_allowed(char ch) {
    if (g_virtual_keyboard_return_screen == SCREEN_SET_NAME) return pex_valid_nickname_char(ch);
    if (g_virtual_keyboard_return_screen == SCREEN_CREATE_WORLD && g_create_more_options) return (ch == '-' || ch == '+' || (ch >= '0' && ch <= '9'));
    return ch >= 32 && ch <= 126;
}

static void pex_virtual_keyboard_append(char ch) {
    if (!pex_virtual_keyboard_char_allowed(ch)) return;
    size_t cap = 0;
    char *dst = pex_virtual_keyboard_target(&cap);
    if (!dst || cap < 2) return;
    size_t len = strlen(dst);
    if (g_virtual_keyboard_return_screen == SCREEN_SET_NAME && len >= 16) return;
    if (len + 1 >= cap) return;
    dst[len] = ch;
    dst[len + 1] = 0;
    if (g_virtual_keyboard_return_screen == SCREEN_CREATE_WORLD && !g_create_more_options) create_world_update_folder();
    rebuild_screen();
}

static void pex_virtual_keyboard_backspace(void) {
    size_t cap = 0; (void)cap;
    char *dst = pex_virtual_keyboard_target(&cap);
    if (!dst) return;
    size_t len = strlen(dst);
    if (len > 0) dst[len - 1] = 0;
    if (g_virtual_keyboard_return_screen == SCREEN_CREATE_WORLD && !g_create_more_options) create_world_update_folder();
    rebuild_screen();
}

static void pex_virtual_keyboard_done(void) {
    ScreenId ret = g_virtual_keyboard_return_screen;
    g_ui_text_input_active = 0;
    set_screen(ret);
    if (ret == SCREEN_CHAT) {
        if (strlen(g_chat_input) > 0) {
            if (g_mp_connected) pex_net_send_chat(g_chat_input);
            else if (!handle_local_chat_command(g_chat_input)) { char line[180]; snprintf(line, sizeof(line), "<Player> %s", g_chat_input); hud_add_chat(line); }
        }
        g_chat_input[0] = 0;
        set_screen(SCREEN_INGAME);
    }
}

static void pex_virtual_keyboard_cancel(void) {
    ScreenId ret = g_virtual_keyboard_return_screen;
    g_ui_text_input_active = 0;
    set_screen(ret == SCREEN_CHAT ? SCREEN_INGAME : ret);
}

static void pex_virtual_keyboard_select(void) {
    static const char *rows[] = {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm_-."};
    if (g_virtual_keyboard_row >= 0 && g_virtual_keyboard_row < 4) {
        const char *row = rows[g_virtual_keyboard_row];
        int n = (int)strlen(row);
        if (g_virtual_keyboard_col < 0) g_virtual_keyboard_col = 0;
        if (g_virtual_keyboard_col >= n) g_virtual_keyboard_col = n - 1;
        char ch = row[g_virtual_keyboard_col];
        if (g_virtual_keyboard_caps && ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
        pex_virtual_keyboard_append(ch);
    } else {
        switch (g_virtual_keyboard_col) {
            case 0: pex_virtual_keyboard_append(' '); break;
            case 1: pex_virtual_keyboard_backspace(); break;
            case 2: g_virtual_keyboard_caps = !g_virtual_keyboard_caps; break;
            case 3: pex_virtual_keyboard_done(); break;
            case 4: pex_virtual_keyboard_cancel(); break;
        }
    }
}

static void pex_virtual_keyboard_move(int dx, int dy) {
    static const int row_lens[] = {10,10,9,10,5};
    g_virtual_keyboard_row += dy;
    if (g_virtual_keyboard_row < 0) g_virtual_keyboard_row = 0;
    if (g_virtual_keyboard_row > 4) g_virtual_keyboard_row = 4;
    g_virtual_keyboard_col += dx;
    int maxc = row_lens[g_virtual_keyboard_row] - 1;
    if (g_virtual_keyboard_col < 0) g_virtual_keyboard_col = maxc;
    if (g_virtual_keyboard_col > maxc) g_virtual_keyboard_col = 0;
}

static void pex_ui_text_input_begin_for_current_field(void) {
    if (pex_virtual_keyboard_enabled()) {
        pex_virtual_keyboard_prepare(g_screen);
        set_screen(SCREEN_VIRTUAL_KEYBOARD);
        return;
    }
    if (g_screen == SCREEN_CHAT) {
        pex_ui_text_input_begin_gui_rect(4, g_gui_h - 16, g_gui_w - 8, 14);
    } else if (g_screen == SCREEN_SET_NAME) {
        pex_ui_text_input_begin_gui_rect(g_gui_w / 2 - 100, g_gui_h / 2 - 8, 200, 20);
    } else if (g_screen == SCREEN_CREATE_WORLD || g_screen == SCREEN_RENAME_WORLD) {
        pex_ui_text_input_begin_gui_rect(g_gui_w / 2 - 100, 60, 200, 20);
    } else if (g_screen == SCREEN_MULTIPLAYER && pex_mp_server_mode_get() != 0) {
        int y = 96;
        if (pex_mp_server_mode_get() != 1 && pex_mp_server_edit_field_get() == 0) y += 44;
        pex_ui_text_input_begin_gui_rect(g_gui_w / 2 - 100, y, 200, 20);
    }
}

static int pex_tv_remote_platform_enabled(void) {
#if defined(PEX_PLATFORM_ANDROID_TV) || defined(PEX_PLATFORM_LGWEBOS) || defined(PEX_PLATFORM_XBOX_UWP)
    return 1;
#else
    return 0;
#endif
}

static void pex_tv_remote_clear_states(void) {
    memset(g_tv_remote_state, 0, sizeof(g_tv_remote_state));
}

static void pex_tv_remote_apply_defaults(void) {
#if defined(PEX_PLATFORM_ANDROID_TV)
    /* Android KeyEvent keyCode values. */
    g_opts.tv_remote_map[PEX_TV_REMOTE_UP] = 19;
    g_opts.tv_remote_map[PEX_TV_REMOTE_DOWN] = 20;
    g_opts.tv_remote_map[PEX_TV_REMOTE_LEFT] = 21;
    g_opts.tv_remote_map[PEX_TV_REMOTE_RIGHT] = 22;
    g_opts.tv_remote_map[PEX_TV_REMOTE_OK] = 23;
    g_opts.tv_remote_map[PEX_TV_REMOTE_BACK] = 4;
    g_opts.tv_remote_map[PEX_TV_REMOTE_BREAK] = 183;
    g_opts.tv_remote_map[PEX_TV_REMOTE_PLACE] = 184;
    g_opts.tv_remote_map[PEX_TV_REMOTE_INVENTORY] = 185;
    g_opts.tv_remote_map[PEX_TV_REMOTE_SNEAK] = 186;
    g_opts.tv_remote_map[PEX_TV_REMOTE_DROP] = 66;
    g_opts.tv_remote_map[PEX_TV_REMOTE_HOTBAR_PREV] = 7;
    g_opts.tv_remote_map[PEX_TV_REMOTE_HOTBAR_NEXT] = 16;
#elif defined(PEX_PLATFORM_LGWEBOS)
    g_opts.tv_remote_map[PEX_TV_REMOTE_UP] = SDLK_UP;
    g_opts.tv_remote_map[PEX_TV_REMOTE_DOWN] = SDLK_DOWN;
    g_opts.tv_remote_map[PEX_TV_REMOTE_LEFT] = SDLK_LEFT;
    g_opts.tv_remote_map[PEX_TV_REMOTE_RIGHT] = SDLK_RIGHT;
    g_opts.tv_remote_map[PEX_TV_REMOTE_OK] = SDLK_RETURN;
    g_opts.tv_remote_map[PEX_TV_REMOTE_BACK] = SDLK_ESCAPE;
    g_opts.tv_remote_map[PEX_TV_REMOTE_BREAK] = 'r';
    g_opts.tv_remote_map[PEX_TV_REMOTE_PLACE] = 'g';
    g_opts.tv_remote_map[PEX_TV_REMOTE_INVENTORY] = 'y';
    g_opts.tv_remote_map[PEX_TV_REMOTE_SNEAK] = 'b';
    g_opts.tv_remote_map[PEX_TV_REMOTE_DROP] = 'q';
    g_opts.tv_remote_map[PEX_TV_REMOTE_HOTBAR_PREV] = SDLK_PAGEUP;
    g_opts.tv_remote_map[PEX_TV_REMOTE_HOTBAR_NEXT] = SDLK_PAGEDOWN;
#else
    for (int i = 0; i < PEX_TV_REMOTE_ACTION_COUNT; ++i) g_opts.tv_remote_map[i] = 0;
#endif
    g_opts.tv_remote_mapped = 1;
    save_options();
    pex_tv_remote_clear_states();
}

static void pex_tv_remote_map_prepare(ScreenId return_screen, int first_run) {
    (void)first_run;
    g_tv_remote_return_screen = return_screen;
    g_tv_remote_map_step = 0;
    g_opts.tv_remote_mapped = 0;
    for (int i = 0; i < PEX_TV_REMOTE_ACTION_COUNT; ++i) g_opts.tv_remote_map[i] = 0;
    pex_tv_remote_clear_states();
}

static int pex_tv_remote_action_from_raw(int raw_key) {
    if (!g_opts.tv_remote_mapped) return -1;
    for (int i = 0; i < PEX_TV_REMOTE_ACTION_COUNT; ++i) {
        if (g_opts.tv_remote_map[i] == raw_key) return i;
    }
    return -1;
}

static int pex_tv_remote_action_down(int action) {
    if (action < 0 || action >= PEX_TV_REMOTE_ACTION_COUNT) return 0;
    return g_tv_remote_state[action] != 0;
}

static int pex_tv_remote_any_action_down(void) {
    for (int i = 0; i < PEX_TV_REMOTE_ACTION_COUNT; ++i) if (g_tv_remote_state[i]) return 1;
    return 0;
}

static void pex_tv_remote_finish_mapping(void) {
    g_opts.tv_remote_mapped = 1;
    pex_tv_remote_clear_states();
    save_options();
    if (g_tv_remote_return_screen != SCREEN_TITLE) set_screen(g_tv_remote_return_screen);
    else set_screen(pex_startup_screen());
}

static int pex_tv_remote_handle_raw_key(int raw_key, int down) {
    if (!pex_tv_remote_platform_enabled() || raw_key == 0) return 0;
    g_tv_remote_seen = 1;
    if (g_screen == SCREEN_TV_REMOTE_MAP) {
        if (!down) return 1;
        if (g_tv_remote_map_step >= 0 && g_tv_remote_map_step < PEX_TV_REMOTE_ACTION_COUNT) {
            g_opts.tv_remote_map[g_tv_remote_map_step++] = raw_key;
            if (g_tv_remote_map_step >= PEX_TV_REMOTE_ACTION_COUNT) pex_tv_remote_finish_mapping();
            else rebuild_screen();
        }
        return 1;
    }
    int action = pex_tv_remote_action_from_raw(raw_key);
    if (action >= 0) {
        g_tv_remote_state[action] = down ? 1 : 0;
        return 1;
    }
    return 0;
}

static ScreenId pex_startup_screen(void) {
    if (pex_tv_remote_platform_enabled() && !g_opts.tv_remote_mapped) {
        pex_tv_remote_map_prepare(SCREEN_TITLE, 1);
        return SCREEN_TV_REMOTE_MAP;
    }
    if (!g_opts.name_set) {
        pex_name_screen_prepare(SCREEN_TITLE, 1);
        return SCREEN_SET_NAME;
    }
    return SCREEN_TITLE;
}

static void pex_commit_nickname(void) {
    pex_sanitize_nickname_in_place(g_name_edit_text);
    if (!pex_nickname_valid(g_name_edit_text)) return;
    snprintf(g_opts.username, sizeof(g_opts.username), "%s", g_name_edit_text);
    snprintf(g_multiplayer_username, sizeof(g_multiplayer_username), "%s", g_opts.username);
    g_opts.name_set = 1;
    save_options();
    set_screen(g_name_return_screen);
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
            { int gt = 0; if (read_level_int_tag_for_dir(e->path, "GameType", &gt)) e->game_mode = gt; else e->game_mode = 0; }
            { int hc = 0; if (read_level_int_tag_for_dir(e->path, "hardcore", &hc)) e->hardcore = hc ? 1 : 0; else e->hardcore = 0; }
            { int conv = 0; if (read_level_int_tag_for_dir(e->path, "RequiresConversion", &conv)) e->requires_conversion = conv ? 1 : 0; else e->requires_conversion = 0; }
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

static void world_save_ensure_selected_visible(void) {
    if (g_selected_world_index < 0 || g_selected_world_index >= g_world_save_count) return;
    int visible = world_save_visible_rows();
    if (g_selected_world_index < g_world_save_scroll) g_world_save_scroll = g_selected_world_index;
    if (g_selected_world_index >= g_world_save_scroll + visible) g_world_save_scroll = g_selected_world_index - visible + 1;
    clamp_world_save_scroll();
}

static void world_save_select_index(int idx) {
    if (g_world_save_count <= 0) {
        g_selected_world_index = -1;
        return;
    }
    if (idx < 0) idx = 0;
    if (idx >= g_world_save_count) idx = g_world_save_count - 1;
    g_selected_world_index = idx;
    world_save_ensure_selected_visible();
}

static void world_save_select_relative(int delta) {
    if (g_screen != SCREEN_WORLD_SELECT && g_screen != SCREEN_WORLD_DELETE) return;
    scan_world_saves();
    if (g_world_save_count <= 0) return;
    if (g_selected_world_index < 0 || g_selected_world_index >= g_world_save_count) world_save_select_index(0);
    else world_save_select_index(g_selected_world_index + delta);
    rebuild_screen();
}

static int world_save_selected_row_center(int *out_x, int *out_y) {
    if (g_selected_world_index < 0 || g_selected_world_index >= g_world_save_count) return 0;
    world_save_ensure_selected_visible();
    int row = g_selected_world_index - g_world_save_scroll;
    if (row < 0 || row >= world_save_visible_rows()) return 0;
    if (out_x) *out_x = g_gui_w / 2;
    if (out_y) *out_y = world_save_list_top() + 4 + row * 36 + 16;
    return 1;
}

static void start_selected_world_save(void) {
    if (g_selected_world_index < 0 || g_selected_world_index >= g_world_save_count) return;
    scan_world_saves();
    if (g_selected_world_index < 0 || g_selected_world_index >= g_world_save_count) return;
    WorldSaveEntry *e = &g_world_saves[g_selected_world_index];
    g_pending_world_slot = g_selected_world_index + 1;
    g_pending_world_type = e->world_type;
    g_pending_game_mode = e->game_mode ? 1 : 0;
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

static void create_world_update_folder(void) {
    char dir_name[64];
    make_unique_world_dir(g_pending_world_name[0] ? g_pending_world_name : "New World",
                          dir_name, sizeof(dir_name), g_pending_world_dir, sizeof(g_pending_world_dir));
}

static long long create_world_random_seed(void) {
    return ((long long)time(NULL) << 32) ^ (long long)GetTickCount();
}

static void create_world_prepare_defaults(void) {
    snprintf(g_pending_world_name, sizeof(g_pending_world_name), "New World");
    g_pending_seed_text[0] = 0;
    g_pending_seed_set = 0;
    g_pending_world_seed = 0;
    g_pending_map_features = 1;
    g_pending_world_type = 1;
    g_pending_game_mode = 0;
    g_create_more_options = 0;
    g_create_edit_field = 0;
    g_pending_world_slot = g_world_save_count + 1;
    create_world_update_folder();
}

static int create_world_parse_seed(void) {
    char seed_text[MAX_LABEL];
    snprintf(seed_text, sizeof(seed_text), "%s", g_pending_seed_text);
    trim_ascii_in_place(seed_text);
    if (!seed_text[0]) {
        g_pending_seed_set = 0;
        g_pending_world_seed = create_world_random_seed();
        return 1;
    }
    char *end = NULL;
    long long parsed = strtoll(seed_text, &end, 10);
    if (end && *end == 0 && parsed != 0) {
        g_pending_seed_set = 1;
        g_pending_world_seed = parsed;
        return 1;
    }
    if (end && *end == 0 && parsed == 0) {
        g_pending_seed_set = 0;
        g_pending_world_seed = create_world_random_seed();
        return 1;
    }
    g_pending_seed_set = 1;
    g_pending_world_seed = (long long)java_string_hash_compat(seed_text);
    return 1;
}

static void create_world_start(void) {
    trim_ascii_in_place(g_pending_world_name);
    if (!g_pending_world_name[0]) return;
    create_world_update_folder();
    if (g_pending_world_slot <= 0) g_pending_world_slot = g_world_save_count + 1;
    create_world_parse_seed();
    start_world_generation_in_dir(g_pending_world_dir, g_pending_world_name, g_pending_world_slot);
}

static void rename_selected_world_save(void) {
    if (g_selected_world_index < 0 || g_selected_world_index >= g_world_save_count) return;
    trim_ascii_in_place(g_rename_world_text);
    if (!g_rename_world_text[0]) return;
    WorldSaveEntry *e = &g_world_saves[g_selected_world_index];
    long long seed = 0;
    if (!read_world_seed_for_dir(e->path, &seed)) seed = (long long)time(NULL);
    int old_type = read_world_type_for_dir(e->path);
    int old_features = 1;
    int old_game_mode = 0;
    read_level_int_tag_for_dir(e->path, "MapFeatures", &old_features);
    read_level_int_tag_for_dir(e->path, "GameType", &old_game_mode);
    int old_pending_type = g_pending_world_type;
    int old_pending_features = g_pending_map_features;
    int old_pending_game_mode = g_pending_game_mode;
    int old_world_type = g_world_type;
    int old_world_features = g_world_map_features;
    int old_active_game_mode = g_game_mode;
    g_pending_world_type = old_type;
    g_pending_map_features = old_features ? 1 : 0;
    g_pending_game_mode = old_game_mode ? 1 : 0;
    g_world_type = old_type;
    g_world_map_features = old_features ? 1 : 0;
    g_game_mode = old_game_mode ? 1 : 0;
    write_level_dat(e->path, g_rename_world_text, seed, 50, 5, 50, (long long)e->size_on_disk);
    g_pending_world_type = old_pending_type;
    g_pending_map_features = old_pending_features;
    g_pending_game_mode = old_pending_game_mode;
    g_world_type = old_world_type;
    g_world_map_features = old_world_features;
    g_game_mode = old_active_game_mode;
    set_screen(SCREEN_WORLD_SELECT);
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
        /* Keep player customization inside the regular two-column options grid.
           These used to be full-width lower action buttons, which made the
           options screen feel too tall on touch devices. */
        add_button_full(301, g_gui_w / 2 - 155, g_gui_h / 6 + 72, 150, 20, tr("Skins..."), BUTTON_NORMAL);
        add_button_full(303, g_gui_w / 2 + 5,   g_gui_h / 6 + 72, 150, 20, "Nickname...", BUTTON_NORMAL);

        {
            int nav_y = g_gui_h / 6 + 100;
            add_button_full(300, g_gui_w / 2 - 100, nav_y,      200, 20, tr("Video Settings..."), BUTTON_NORMAL);
            add_button_full(100, g_gui_w / 2 - 100, nav_y + 24, 200, 20, tr("Controls..."), BUTTON_NORMAL);
            add_button_full(302, g_gui_w / 2 - 100, nav_y + 48, 200, 20, tr("Language"), BUTTON_NORMAL);
#if defined(PEX_PLATFORM_ANDROID_TV) || defined(PEX_PLATFORM_LGWEBOS) || defined(PEX_PLATFORM_XBOX_UWP)
            add_button_full(304, g_gui_w / 2 - 100, nav_y + 72, 200, 20, "Remote...", BUTTON_NORMAL);
#endif
        }
        add_button_full(200, g_gui_w / 2 - 100, g_gui_h - 28, 200, 20, tr("Done"), BUTTON_NORMAL);
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
    } else if (g_screen == SCREEN_LANGUAGE) {
        if (!pex_language_runtime_files_available()) {
            add_button_full(300, g_gui_w / 2 - 120, g_gui_h / 2 - 10, 240, 20, "Download languages", BUTTON_NORMAL);
        } else {
            int count = pex_language_count();
            int top = pex_language_top();
            int bottom = pex_language_bottom();
            int first = pex_language_first_visible_row();
            int visible = pex_language_visible_rows();
            for (int i = 0; i < visible; ++i) {
                int idx = first + i;
                int y = pex_language_row_y(idx);
                if (idx >= count) break;
                if (y + 14 < top || y > bottom) continue;
                add_button_full(4000 + idx, g_gui_w / 2 - 110, y - 2, 220, 18, "", BUTTON_HITBOX);
            }
        }
        add_button_full(200, g_gui_w / 2 - 75, g_gui_h - 38, 150, 20, tr_key_default("gui.done", "Done"), BUTTON_NORMAL);
    } else if (g_screen == SCREEN_SET_NAME) {
        Button *done = add_button_full(10, g_gui_w / 2 - 100, g_gui_h / 2 + 50, 200, 20, tr_key_default("gui.done", "Done"), BUTTON_NORMAL);
        done->enabled = pex_nickname_valid(g_name_edit_text);
        if (!g_name_screen_first_run) add_button_full(1, g_gui_w / 2 - 100, g_gui_h / 2 + 74, 200, 20, tr_key_default("gui.cancel", "Cancel"), BUTTON_NORMAL);
    } else if (g_screen == SCREEN_TV_REMOTE_MAP) {
        add_button_full(20, g_gui_w / 2 - 100, g_gui_h - 52, 200, 20, "Use Defaults", BUTTON_NORMAL);
        add_button_full(21, g_gui_w / 2 - 100, g_gui_h - 28, 200, 20, tr_key_default("gui.cancel", "Cancel"), BUTTON_NORMAL);
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
    } else if (g_screen == SCREEN_CREATE_WORLD) {
        add_button_full(0, g_gui_w / 2 - 155, g_gui_h - 28, 150, 20, tr_key_default("selectWorld.create", "Create New World"), BUTTON_NORMAL);
        add_button_full(1, g_gui_w / 2 + 5, g_gui_h - 28, 150, 20, tr_key_default("gui.cancel", "Cancel"), BUTTON_NORMAL);
        Button *create = &g_buttons[g_button_count - 2];
        create->enabled = g_pending_world_name[0] != 0;
        if (!g_create_more_options) {
            char gm_label[96];
            snprintf(gm_label, sizeof(gm_label), "%s: %s",
                     tr_key_default("selectWorld.gameMode", "Game Mode"),
                     g_pending_game_mode ? tr_key_default("selectWorld.gameMode.creative", "Creative") : tr_key_default("selectWorld.gameMode.survival", "Survival"));
            add_button_full(2, g_gui_w / 2 - 75, 100, 150, 20, gm_label, BUTTON_NORMAL);
            add_button_full(3, g_gui_w / 2 - 75, 172, 150, 20, tr_key_default("selectWorld.moreWorldOptions", "More World Options..."), BUTTON_NORMAL);
        } else {
            char mf[64];
            snprintf(mf, sizeof(mf), "%s: %s", tr_key_default("selectWorld.mapFeatures", "Generate Structures"), g_pending_map_features ? tr_key_default("options.on", "ON") : tr_key_default("options.off", "OFF"));
            add_button_full(4, g_gui_w / 2 - 155, 100, 150, 20, mf, BUTTON_NORMAL);
            { char wt[96]; snprintf(wt, sizeof(wt), "%s: %s", tr_key_default("selectWorld.mapType", "World Type"), g_pending_world_type ? tr_key_default("generator.default", "Default") : tr_key_default("generator.flat", "Superflat")); add_button_full(5, g_gui_w / 2 + 5, 100, 150, 20, wt, BUTTON_NORMAL); }
            add_button_full(3, g_gui_w / 2 - 75, 172, 150, 20, tr_key_default("gui.done", "Done"), BUTTON_NORMAL);
        }
    } else if (g_screen == SCREEN_RENAME_WORLD) {
        Button *rename = add_button_full(0, g_gui_w / 2 - 155, g_gui_h / 6 + 96, 150, 20, tr_key_default("selectWorld.renameButton", "Rename"), BUTTON_NORMAL);
        add_button_full(1, g_gui_w / 2 + 5, g_gui_h / 6 + 96, 150, 20, tr_key_default("gui.cancel", "Cancel"), BUTTON_NORMAL);
        rename->enabled = g_rename_world_text[0] != 0;
    } else if (g_screen == SCREEN_WORLD_TYPE) {
        /* Legacy screen id kept for old callers; new worlds use SCREEN_CREATE_WORLD. */
        add_button_full(2, g_gui_w / 2 - 100, g_gui_h / 4 + 108, 200, 20, tr_key_default("gui.cancel", "Cancel"), BUTTON_NORMAL);
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
            Button *select = add_button_full(10, g_gui_w / 2 - 154, g_gui_h - 52, 150, 20, tr_key_default("selectWorld.play", "Play Selected World"), BUTTON_NORMAL);
            Button *rename = add_button_full(12, g_gui_w / 2 - 154, g_gui_h - 28, 70, 20, tr_key_default("selectWorld.renameButton", "Rename"), BUTTON_NORMAL);
            Button *delete = add_button_full(5, g_gui_w / 2 - 74, g_gui_h - 28, 70, 20, tr_key_default("selectWorld.deleteButton", "Delete"), BUTTON_NORMAL);
            add_button_full(11, g_gui_w / 2 + 4, g_gui_h - 52, 150, 20, tr_key_default("selectWorld.create", "Create New World"), BUTTON_NORMAL);
            add_button_full(6, g_gui_w / 2 + 4, g_gui_h - 28, 150, 20, tr_key_default("gui.cancel", "Cancel"), BUTTON_NORMAL);
            int has_world = g_selected_world_index >= 0 && g_selected_world_index < g_world_save_count;
            select->enabled = has_world;
            rename->enabled = has_world;
            delete->enabled = has_world;
        } else {
            Button *delete = add_button_full(10, g_gui_w / 2 - 154, g_gui_h - 52, 150, 20, tr_key_default("selectWorld.deleteButton", "Delete"), BUTTON_NORMAL);
            add_button_full(6, g_gui_w / 2 + 4, g_gui_h - 52, 150, 20, tr_key_default("gui.cancel", "Cancel"), BUTTON_NORMAL);
            delete->enabled = g_selected_world_index >= 0 && g_selected_world_index < g_world_save_count;
        }
    } else if (g_screen == SCREEN_CONFIRM_DELETE) {
        add_button_full(0, g_gui_w / 2 - 155 + 0, g_gui_h / 6 + 96, 150, 20, tr_key_default("gui.yes", "Yes"), BUTTON_NORMAL);
        add_button_full(1, g_gui_w / 2 - 155 + 160, g_gui_h / 6 + 96, 150, 20, tr_key_default("gui.no", "No"), BUTTON_NORMAL);
    } else if (g_screen == SCREEN_MULTIPLAYER) {
        pex_mp_server_list_ensure();
        if (pex_mp_server_mode_get() == 0) {
            int visible_rows = pex_mp_server_visible_rows();
            int top = 32;
            int scroll = pex_mp_server_scroll_get();
            int bottom = g_gui_h - 64;
            for (int i = 0; i < visible_rows; i++) {
                int idx = scroll + i;
                if (idx >= pex_mp_server_count_get()) break;
                int y = top + 4 + i * 36;
                int h = 32;
                if (y + h > bottom) h = bottom - y;
                if (h > 0) add_button_full(PEX_MP_SERVER_ROW_BASE + idx, g_gui_w / 2 - 110, y, 220, h, "", BUTTON_HITBOX);
            }
            int has_server = pex_mp_server_selected_get() >= 0 && pex_mp_server_selected_get() < pex_mp_server_count_get();
            Button *select = add_button_full(10, g_gui_w / 2 - 154, g_gui_h - 52, 100, 20, tr_key_default("selectServer.select", "Join Server"), BUTTON_NORMAL);
            add_button_full(4, g_gui_w / 2 - 50, g_gui_h - 52, 100, 20, tr_key_default("selectServer.direct", "Direct Connect"), BUTTON_NORMAL);
            add_button_full(3, g_gui_w / 2 + 54, g_gui_h - 52, 100, 20, tr_key_default("selectServer.add", "Add server"), BUTTON_NORMAL);
            Button *edit = add_button_full(7, g_gui_w / 2 - 154, g_gui_h - 28, 70, 20, tr_key_default("selectServer.edit", "Edit"), BUTTON_NORMAL);
            Button *del = add_button_full(2, g_gui_w / 2 - 74, g_gui_h - 28, 70, 20, tr_key_default("selectServer.delete", "Delete"), BUTTON_NORMAL);
            add_button_full(8, g_gui_w / 2 + 4, g_gui_h - 28, 70, 20, tr_key_default("selectServer.refresh", "Refresh"), BUTTON_NORMAL);
            add_button_full(1, g_gui_w / 2 + 80, g_gui_h - 28, 75, 20, tr_key_default("gui.cancel", "Cancel"), BUTTON_NORMAL);
            select->enabled = has_server;
            edit->enabled = has_server;
            del->enabled = has_server;
        } else {
            Button *select = add_button_full(10, g_gui_w / 2 - 100, g_gui_h / 4 + 108 + 12, 200, 20, tr_key_default("selectServer.select", "Select"), BUTTON_NORMAL);
            select->enabled = pex_mp_server_edit_address_get()[0] != 0;
            add_button_full(1, g_gui_w / 2 - 100, g_gui_h / 4 + 132 + 12, 200, 20, tr_key_default("gui.cancel", "Cancel"), BUTTON_NORMAL);
        }
    } else if (g_screen == SCREEN_CONNECTING) {
        add_button(0, g_gui_w / 2 - 100, g_gui_h / 4 + 120 + 12, tr_key_default("gui.cancel", "Cancel"));
    } else if (g_screen == SCREEN_TEXPACK_INSTALL) {
        add_button(1, g_gui_w / 2 - 100, g_gui_h / 2 + 34, tr_key_default("gui.cancel", "Cancel"));
    } else if (g_screen == SCREEN_TEXPACK) {
        scan_texture_packs();
        add_button_full(5, g_gui_w / 2 - 154, g_gui_h - 48, 150, 20, tr_key_default("texturePack.openFolder", "Open texture pack folder"), BUTTON_NORMAL);
        add_button_full(6, g_gui_w / 2 + 4, g_gui_h - 48, 150, 20, tr_key_default("gui.done", "Done"), BUTTON_NORMAL);
    } else if (g_screen == SCREEN_PAUSE) {
        add_button(4, g_gui_w / 2 - 100, g_gui_h / 4 + 24, tr_key_default("menu.returnToGame", "Back to game"));
        add_button(1, g_gui_w / 2 - 100, g_gui_h / 4 + 48, g_mp_connected ? tr_key_default("menu.disconnect", "Disconnect") : tr_key_default("menu.returnToMenu", "Save and quit to title"));
        add_button(0, g_gui_w / 2 - 100, g_gui_h / 4 + 96, tr_key_default("menu.options", "Options..."));
    } else if (g_screen == SCREEN_CREATIVE) {
        /* Java 1.2.5 GuiContainerCreative has no Prev/Next/Done buttons. */
    } else if (g_screen == SCREEN_DEATH) {
        add_button(1, g_gui_w / 2 - 100, g_gui_h / 4 + 72, tr_key_default("deathScreen.respawn", "Respawn"));
        add_button(2, g_gui_w / 2 - 100, g_gui_h / 4 + 96, tr_key_default("deathScreen.titleScreen", "Title menu"));
    } else if (g_screen == SCREEN_NOTICE) {
        add_button(0, g_gui_w / 2 - 100, g_gui_h / 4 + 120 + 12, tr_key_default("menu.returnToMenu", "Back to title screen"));
    } else if (g_screen == SCREEN_RENDERER_RESTART_PROMPT) {
        add_button_full(0, g_gui_w / 2 - 155, g_gui_h / 4 + 120 + 12, 150, 20, "Restart", BUTTON_NORMAL);
        add_button_full(1, g_gui_w / 2 + 5, g_gui_h / 4 + 120 + 12, 150, 20, "Ignore", BUTTON_NORMAL);
    } else if (g_screen == SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT) {
#if PEX_CLASSIC_SOUND_DOWNLOAD_SUPPORTED
        char snd_label[MAX_LABEL];
        snprintf(snd_label, sizeof(snd_label), "Sounds: %s", g_opts.download_classic_sounds ? "ON" : "OFF");
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
        else if (b->id == 5) { g_language_return_screen = SCREEN_TITLE; set_screen(SCREEN_LANGUAGE); }
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
        else if (b->id == 302) { g_language_return_screen = SCREEN_OPTIONS; set_screen(SCREEN_LANGUAGE); }
        else if (b->id == 303) { pex_name_screen_prepare(SCREEN_OPTIONS, 0); set_screen(SCREEN_SET_NAME); }
        else if (b->id == 304) { pex_tv_remote_map_prepare(SCREEN_OPTIONS, 0); set_screen(SCREEN_TV_REMOTE_MAP); }
    } else if (g_screen == SCREEN_OPTIONS_MORE) {
        if (b->id < 100) {
            if (b->kind == BUTTON_NORMAL) {
                bump_option(b->opt, 1);
                get_option_label(b->opt, b->label, sizeof(b->label));
            }
        } else if (b->id == 122) set_screen(SCREEN_SYSTEM_INFO);
        else if (b->id == 199) { save_options(); set_screen(SCREEN_OPTIONS); }
    } else if (g_screen == SCREEN_LANGUAGE) {
        if (b->id >= 4000 && b->id < 4000 + pex_language_count()) {
            const char *code = pex_language_code_at(b->id - 4000);
            snprintf(g_opts.language, sizeof(g_opts.language), "%s", code);
            pex_set_language_code(g_opts.language);
            save_options();
            rebuild_screen();
        } else if (b->id == 300) {
            if (pex_language_download_from_jar_blocking()) {
                pex_set_language_code(g_opts.language);
                rebuild_screen();
            } else {
                open_notice("Language", "Could not download languages from client.jar.", g_classic_install_error);
            }
        } else if (b->id == 200) { save_options(); set_screen(g_language_return_screen); }
    } else if (g_screen == SCREEN_SET_NAME) {
        if (b->id == 10) pex_commit_nickname();
        else if (b->id == 1 && !g_name_screen_first_run) set_screen(g_name_return_screen);
    } else if (g_screen == SCREEN_TV_REMOTE_MAP) {
        if (b->id == 20) { pex_tv_remote_apply_defaults(); if (g_tv_remote_return_screen != SCREEN_TITLE) set_screen(g_tv_remote_return_screen); else set_screen(pex_startup_screen()); }
        else if (b->id == 21) {
            if (!g_opts.tv_remote_mapped) { pex_tv_remote_apply_defaults(); if (g_tv_remote_return_screen != SCREEN_TITLE) set_screen(g_tv_remote_return_screen); else set_screen(pex_startup_screen()); }
            else set_screen(g_tv_remote_return_screen);
        }
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
            create_world_prepare_defaults();
            set_screen(SCREEN_CREATE_WORLD);
        } else if (b->id == 12 && g_selected_world_index >= 0 && g_selected_world_index < g_world_save_count) {
            WorldSaveEntry *e = &g_world_saves[g_selected_world_index];
            snprintf(g_rename_world_text, sizeof(g_rename_world_text), "%s", e->display_name[0] ? e->display_name : e->dir_name);
            set_screen(SCREEN_RENAME_WORLD);
        }
        else if (b->id == 6) set_screen(g_parent_screen);
    } else if (g_screen == SCREEN_CREATE_WORLD) {
        if (b->id == 0) create_world_start();
        else if (b->id == 1) {
            g_pending_world_dir[0] = 0;
            g_pending_world_name[0] = 0;
            set_screen(SCREEN_WORLD_SELECT);
        } else if (b->id == 2) {
            g_pending_game_mode = g_pending_game_mode ? 0 : 1;
            rebuild_screen();
        } else if (b->id == 3) {
            g_create_more_options = !g_create_more_options;
            g_create_edit_field = g_create_more_options ? 1 : 0;
            rebuild_screen();
        } else if (b->id == 4) {
            g_pending_map_features = !g_pending_map_features;
            rebuild_screen();
        } else if (b->id == 5) {
            g_pending_world_type = g_pending_world_type ? 0 : 1;
            rebuild_screen();
        }
    } else if (g_screen == SCREEN_WORLD_TYPE) {
        if (b->id == 2) set_screen(SCREEN_WORLD_SELECT);
    } else if (g_screen == SCREEN_RENAME_WORLD) {
        if (b->id == 0) rename_selected_world_save();
        else if (b->id == 1) set_screen(SCREEN_WORLD_SELECT);
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
                g_pending_world_type = 1;
                g_pending_game_mode = 0;
            }
        }
        g_confirm_world = 0;
        g_pending_world_dir[0] = 0;
        g_pending_world_name[0] = 0;
        set_screen(SCREEN_WORLD_SELECT);
    } else if (g_screen == SCREEN_MULTIPLAYER) {
        if (b->id >= PEX_MP_SERVER_ROW_BASE && b->id < PEX_MP_SERVER_ROW_BASE + PEX_MP_SERVER_LIST_MAX) {
            int idx = b->id - PEX_MP_SERVER_ROW_BASE;
            pex_mp_server_select(idx);
            rebuild_screen();
        } else if (pex_mp_server_mode_get() == 0) {
            if (b->id == 1) set_screen(g_parent_screen);
            else if (b->id == 10) {
                if (!pex_mp_server_connect_selected()) return;
                snprintf(g_opts.last_server, sizeof(g_opts.last_server), "%s", g_multiplayer_ip);
                snprintf(g_opts.username, sizeof(g_opts.username), "%s", g_multiplayer_username[0] ? g_multiplayer_username : "Player");
                save_options();
                set_screen(SCREEN_CONNECTING);
                if (!pex_net_connect_to_server(g_multiplayer_ip)) {
                    open_notice("Connection failed", g_multiplayer_status[0] ? g_multiplayer_status : "Could not connect to server.", "");
                }
            } else if (b->id == 4) {
                pex_mp_server_begin_direct();
                rebuild_screen();
            } else if (b->id == 3) {
                pex_mp_server_begin_add();
                rebuild_screen();
            } else if (b->id == 7) {
                pex_mp_server_begin_edit();
                rebuild_screen();
            } else if (b->id == 2) {
                pex_mp_server_delete_selected();
                rebuild_screen();
            } else if (b->id == 8) {
                pex_mp_server_refresh_all();
                rebuild_screen();
            }
        } else {
            if (b->id == 1) {
                pex_mp_server_cancel_edit();
                rebuild_screen();
            } else if (b->id == 10) {
                if (pex_mp_server_mode_get() == 1) {
                    snprintf(g_multiplayer_ip, sizeof(g_multiplayer_ip), "%s", pex_mp_server_edit_address_get());
                    snprintf(g_opts.last_server, sizeof(g_opts.last_server), "%s", g_multiplayer_ip);
                    snprintf(g_opts.username, sizeof(g_opts.username), "%s", g_multiplayer_username[0] ? g_multiplayer_username : "Player");
                    save_options();
                    set_screen(SCREEN_CONNECTING);
                    if (!pex_net_connect_to_server(g_multiplayer_ip)) {
                        open_notice("Connection failed", g_multiplayer_status[0] ? g_multiplayer_status : "Could not connect to server.", "");
                    }
                } else {
                    pex_mp_server_commit_edit();
                    rebuild_screen();
                }
            }
        }
    } else if (g_screen == SCREEN_CONNECTING) {
        pex_net_disconnect();
        pex_net_set_status("Canceled.");
        set_screen(SCREEN_MULTIPLAYER);
    } else if (g_screen == SCREEN_TEXPACK_INSTALL) {
        if (b->id == 1) pack_install_request_cancel();
    } else if (g_screen == SCREEN_TEXPACK) {
        if (b->id == 5) ShellExecuteA(NULL, "open", g_texpack_dir, NULL, NULL, SW_SHOWNORMAL);
        else if (b->id == 6) set_screen(g_parent_screen);
    } else if (g_screen == SCREEN_PAUSE) {
        if (b->id == 4) set_screen(SCREEN_INGAME);
        else if (b->id == 0) { g_parent_screen = SCREEN_PAUSE; set_screen(SCREEN_OPTIONS); }
        else if (b->id == 1) leave_world_to_title();
    } else if (g_screen == SCREEN_CREATIVE) {
        (void)b;
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
            else set_screen(pex_startup_screen());
        } else {
            if (!pack_is_installed() || pack_missing_required_textures()) g_opts.ignore_classic_resources_warning = 1;
            if (g_opts.download_classic_sounds && !classic_sounds_installed()) g_opts.ignore_classic_sounds_warning = 1;
            save_options();
            set_screen(pex_startup_screen());
        }
    } else if (g_screen == SCREEN_CLASSIC_PACK_WARNING) {
        if (b->id == 0) pack_install_start();
        else set_screen(pex_startup_screen());
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

    if (g_screen == SCREEN_CREATIVE) { creative_mouse_click(mx, my, 1); return; }
    if (g_screen == SCREEN_INVENTORY || g_screen == SCREEN_WORKBENCH || g_screen == SCREEN_FURNACE || g_screen == SCREEN_CHEST) { inventory_mouse_click(mx, my, 1); return; }
    if (g_screen == SCREEN_INGAME) { g_right_use_button_down = 1; set_mouse_grabbed(1); if (passive_mobs_player_interact()) return; ingame_right_click(); return; }
    (void)mx; (void)my;
}

static void mouse_right_up(int mx, int my) {
    (void)mx; (void)my;
    if (g_screen == SCREEN_INGAME) ingame_right_release();
    g_right_use_button_down = 0;
}

static void mouse_down(int mx, int my) {
    g_mouse_down = 1;
    if (g_screen == SCREEN_CREATIVE) { creative_mouse_click(mx, my, 0); return; }
    if (g_screen == SCREEN_INVENTORY || g_screen == SCREEN_WORKBENCH || g_screen == SCREEN_FURNACE || g_screen == SCREEN_CHEST) { inventory_mouse_click(mx, my, 0); return; }
    if (g_screen == SCREEN_INGAME) {
        set_mouse_grabbed(1);
        if (pex_net_try_attack_player()) return;
        if (passive_mobs_attack_from_player()) return;
        FlatRayHit hit = flat_raycast();
        if (hit.hit) restart_hand_swing();
        else start_air_swing_once();
        /* Java clickMouse() calls PlayerControllerCreative.clickBlock() immediately
           on the click edge, then holding uses the 5-tick creative repeat delay. */
        if (player_is_creative()) update_breaking();
        return;
    }
    if (g_screen == SCREEN_CREATE_WORLD) {
        int x = g_gui_w / 2 - 100;
        int y = 60;
        if (mx >= x - 1 && mx < x + 201 && my >= y - 1 && my < y + 21) {
            g_create_edit_field = g_create_more_options ? 1 : 0;
            pex_ui_text_input_begin_for_current_field();
            rebuild_screen();
        }
    }
    if (g_screen == SCREEN_RENAME_WORLD) {
        int x = g_gui_w / 2 - 100;
        int y = 60;
        if (mx >= x - 1 && mx < x + 201 && my >= y - 1 && my < y + 21) { pex_ui_text_input_begin_for_current_field(); rebuild_screen(); }
    }
    if (g_screen == SCREEN_SET_NAME) {
        int x = g_gui_w / 2 - 100;
        int y = g_gui_h / 2 - 8;
        if (mx >= x - 1 && mx < x + 201 && my >= y - 1 && my < y + 21) { pex_ui_text_input_begin_for_current_field(); rebuild_screen(); }
    }
    if (g_screen == SCREEN_MULTIPLAYER && pex_mp_server_mode_get() != 0) {
        int x = g_gui_w / 2 - 100;
        int y = 96;
        if (pex_mp_server_mode_get() != 1) {
            if (mx >= x - 1 && mx < x + 201 && my >= y - 1 && my < y + 21) {
                g_mp_server_edit_field = 1;
                pex_ui_text_input_begin_for_current_field();
                rebuild_screen();
            }
            y += 44;
        }
        if (mx >= x - 1 && mx < x + 201 && my >= y - 1 && my < y + 21) {
            g_mp_server_edit_field = 0;
            pex_ui_text_input_begin_for_current_field();
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
    if (g_screen == SCREEN_CREATIVE) g_creative_dragging_scroll = 0;
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


static int pex_chat_parse_int(const char *s, int *out) {
    if (!s || !*s || !out) return 0;
    int neg = 0; long v = 0; const char *p = s;
    if (*p == '-') { neg = 1; ++p; }
    if (!*p) return 0;
    while (*p) {
        if (*p < '0' || *p > '9') return 0;
        v = v * 10 + (*p - '0');
        if (v > 1000000L) return 0;
        ++p;
    }
    *out = neg ? -(int)v : (int)v;
    return 1;
}

static int traceplace_is_airish(int id) {
    return id == 0 || id == BLOCK_TALL_GRASS || id == BLOCK_DEAD_BUSH || id == BLOCK_SNOW_LAYER ||
           id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE || id == BLOCK_BROWN_MUSHROOM ||
           id == BLOCK_RED_MUSHROOM || id == BLOCK_TORCH || id == BLOCK_VINE || id == BLOCK_RAILS;
}

static int traceplace_feet_safe_at(int x, int feet_y, int z) {
    if (feet_y < FLAT_WORLD_Y_MIN + 1 || feet_y > FLAT_WORLD_Y_MAX - 2) return 0;
    int below = flat_get_block(x, feet_y - 1, z);
    if (!flat_block_is_solid(below) || block_is_liquid(below)) return 0;
    if (!traceplace_is_airish(flat_get_block(x, feet_y, z))) return 0;
    if (!traceplace_is_airish(flat_get_block(x, feet_y + 1, z))) return 0;
    return !flat_player_aabb_collides((float)x + 0.5f, (float)feet_y + 1.62f, (float)z + 0.5f);
}

static int traceplace_find_safe_surface_near(int tx, int tz, int *out_x, int *out_y, int *out_z) {
    for (int r = 0; r <= 12; ++r) {
        for (int dz = -r; dz <= r; ++dz) {
            for (int dx = -r; dx <= r; ++dx) {
                if (abs(dx) != r && abs(dz) != r) continue;
                int x = tx + dx, z = tz + dz;
                for (int y = FLAT_WORLD_Y_MAX - 1; y >= FLAT_WORLD_Y_MIN + 1; --y) {
                    if (traceplace_feet_safe_at(x, y, z)) {
                        *out_x = x; *out_y = y; *out_z = z;
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

static int traceplace_find_safe_underground_near(int tx, int ty, int tz, int *out_x, int *out_y, int *out_z) {
    for (int r = 0; r <= 8; ++r) {
        for (int dy = -2; dy <= 4; ++dy) {
            for (int dz = -r; dz <= r; ++dz) {
                for (int dx = -r; dx <= r; ++dx) {
                    if (abs(dx) != r && abs(dz) != r) continue;
                    int x = tx + dx, y = ty + dy, z = tz + dz;
                    if (traceplace_feet_safe_at(x, y, z)) {
                        *out_x = x; *out_y = y; *out_z = z;
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

static void traceplace_commit_teleport(int x, int feet_y, int z) {
    g_player_x = g_player_prev_x = (float)x + 0.5f;
    g_player_y = g_player_prev_y = (float)feet_y + 1.62f;
    g_player_z = g_player_prev_z = (float)z + 0.5f;
    g_player_motion_x = g_player_motion_y = g_player_motion_z = 0.0f;
    g_player_fall_distance = 0.0f;
    g_player_on_ground = 1;
    g_player_dead = 0;
    g_suffocation_damage_timer = 0;
    g_distance_walked = g_prev_distance_walked = 0.0f;
    g_limb_swing = g_prev_limb_swing = 0.0f;
    g_limb_swing_amount = g_prev_limb_swing_amount = 0.0f;
    g_camera_yaw = g_prev_camera_yaw = 0.0f;
    g_camera_pitch = g_prev_camera_pitch = 0.0f;
    g_save_dirty = 1;
}

static int handle_traceplace_command(int argc, char **argv) {
    if (argc < 2) {
        hud_add_chat("Usage: /traceplace <village|stronghold|mineshaft> [stronghold 1-3]");
        return 1;
    }
    if (g_mp_connected) {
        hud_add_chat("/traceplace is local-world debug only.");
        return 1;
    }
    if (g_world_type != 1) {
        hud_add_chat("/traceplace needs Default world generator mode.");
        return 1;
    }
    const char *kind = NULL;
    if (pex_chat_word_equals_ci(argv[1], "village")) kind = "village";
    else if (pex_chat_word_equals_ci(argv[1], "stronghold") || pex_chat_word_equals_ci(argv[1], "portalroom")) kind = "stronghold";
    else if (pex_chat_word_equals_ci(argv[1], "mineshaft") || pex_chat_word_equals_ci(argv[1], "mine")) kind = "mineshaft";
    else {
        hud_add_chat("Usage: /traceplace <village|stronghold|mineshaft> [stronghold 1-3]");
        return 1;
    }
    int index = 0;
    if (argc >= 3 && !pex_chat_parse_int(argv[2], &index)) index = 0;
    WorldgenTraceTarget target;
    if (!worldgen_trace_target(g_world_seed, kind, (int)floorf(g_player_x), (int)floorf(g_player_z), index, &target)) {
        hud_add_chat("No matching structure found nearby.");
        return 1;
    }

    if ((target.kind == 1 || target.kind == 2 || target.kind == 3) && !g_world_map_features) {
        hud_add_chat("Generate Structures is OFF for this world.");
        return 1;
    }

    /* Recenter and synchronously build a small destination neighborhood.  Do not
       call the normal spawn preload reset here: that path is queue-based, and
       marking an empty window as generated made /traceplace village land on
       terrain with no generated village blocks. */
    g_player_x = g_player_prev_x = (float)target.blockX + 0.5f;
    g_player_z = g_player_prev_z = (float)target.blockZ + 0.5f;
    g_player_y = g_player_prev_y = (float)target.footY + 1.62f;
    flat_center_origin_near(g_player_x, g_player_z);
    flat_generate_traceplace_area(target.chunkX, target.chunkZ, target.kind == 2 ? 6 : 4);

    int sx = target.blockX, sy = target.footY, sz = target.blockZ;
    int safe = target.surface ? traceplace_find_safe_surface_near(target.blockX, target.blockZ, &sx, &sy, &sz)
                              : traceplace_find_safe_underground_near(target.blockX, target.footY, target.blockZ, &sx, &sy, &sz);
    if (!safe) safe = traceplace_find_safe_surface_near(target.blockX, target.blockZ, &sx, &sy, &sz);
    if (!safe) {
        hud_add_chat("Located structure, but could not find a safe landing spot.");
        return 1;
    }
    traceplace_commit_teleport(sx, sy, sz);
    char msg[180];
    snprintf(msg, sizeof(msg), "Traceplaced to %s at chunk %d,%d (%d,%d,%d).", target.label, target.chunkX, target.chunkZ, sx, sy, sz);
    hud_add_chat(msg);
    return 1;
}

static int parse_coordinate(const char *str, float relative_to, int *out_val) {
    if (str[0] == '~') {
        int offset = 0;
        if (str[1] != '\0') {
            offset = atoi(str + 1);
        }
        *out_val = (int)floorf(relative_to) + offset;
        return 1;
    }
    char *endptr;
    long val = strtol(str, &endptr, 10);
    if (endptr == str) {
        return 0;
    }
    *out_val = (int)val;
    return 1;
}

static int parse_block_name_or_id(const char *str) {
    if (str[0] >= '0' && str[0] <= '9') {
        return atoi(str);
    }
    if (pex_chat_word_equals_ci(str, "air")) return 0;
    if (pex_chat_word_equals_ci(str, "stone")) return BLOCK_STONE;
    if (pex_chat_word_equals_ci(str, "grass")) return BLOCK_GRASS;
    if (pex_chat_word_equals_ci(str, "dirt")) return BLOCK_DIRT;
    if (pex_chat_word_equals_ci(str, "cobblestone") || pex_chat_word_equals_ci(str, "cobble")) return BLOCK_COBBLESTONE;
    if (pex_chat_word_equals_ci(str, "planks") || pex_chat_word_equals_ci(str, "wood")) return BLOCK_PLANKS;
    if (pex_chat_word_equals_ci(str, "bedrock")) return BLOCK_BEDROCK;
    if (pex_chat_word_equals_ci(str, "water")) return BLOCK_WATER;
    if (pex_chat_word_equals_ci(str, "lava")) return BLOCK_LAVA;
    if (pex_chat_word_equals_ci(str, "sand")) return BLOCK_SAND;
    if (pex_chat_word_equals_ci(str, "gravel")) return BLOCK_GRAVEL;
    if (pex_chat_word_equals_ci(str, "obsidian")) return BLOCK_OBSIDIAN;
    if (pex_chat_word_equals_ci(str, "portal") || pex_chat_word_equals_ci(str, "nether_portal")) return BLOCK_PORTAL;
    if (pex_chat_word_equals_ci(str, "end_portal")) return BLOCK_END_PORTAL;
    if (pex_chat_word_equals_ci(str, "end_portal_frame")) return BLOCK_END_PORTAL_FRAME;
    if (pex_chat_word_equals_ci(str, "fire")) return BLOCK_FIRE;
    return -1;
}

static int handle_setblock_command(int argc, char *argv[]) {
    if (argc < 2) {
        hud_add_chat("Usage: /setblock <block> [x y z]  OR  /setblock <x y z> <block>");
        return 1;
    }

    int bx = (int)floorf(g_player_x);
    int by = (int)floorf(g_player_y);
    int bz = (int)floorf(g_player_z);
    int block_id = -1;
    const char *block_str = NULL;

    if (argc == 2) {
        block_str = argv[1];
        block_id = parse_block_name_or_id(block_str);
    } else if (argc == 5) {
        int is_coords_first = (argv[1][0] == '~' || (argv[1][0] >= '0' && argv[1][0] <= '9') || argv[1][0] == '-');
        if (is_coords_first) {
            if (!parse_coordinate(argv[1], g_player_x, &bx) ||
                !parse_coordinate(argv[2], g_player_y, &by) ||
                !parse_coordinate(argv[3], g_player_z, &bz)) {
                hud_add_chat("Invalid coordinates.");
                return 1;
            }
            block_str = argv[4];
            block_id = parse_block_name_or_id(block_str);
        } else {
            block_str = argv[1];
            block_id = parse_block_name_or_id(block_str);
            if (!parse_coordinate(argv[2], g_player_x, &bx) ||
                !parse_coordinate(argv[3], g_player_y, &by) ||
                !parse_coordinate(argv[4], g_player_z, &bz)) {
                hud_add_chat("Invalid coordinates.");
                return 1;
            }
        }
    } else {
        hud_add_chat("Usage: /setblock <block> [x y z]  OR  /setblock <x y z> <block>");
        return 1;
    }

    if (block_id == -1) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Unknown block: %s", block_str);
        hud_add_chat(msg);
        return 1;
    }

    if (!flat_in_bounds(bx, by, bz)) {
        hud_add_chat("Coordinates out of bounds.");
        return 1;
    }

    flat_begin_persistent_edit();
    flat_set_block(bx, by, bz, block_id);
    flat_end_persistent_edit();
    g_save_dirty = 1;

    char msg[128];
    snprintf(msg, sizeof(msg), "Set block at %d, %d, %d to %s (%d).", bx, by, bz, block_str, block_id);
    hud_add_chat(msg);

    return 1;
}

static int handle_local_chat_command(const char *text) {
    if (dear_memories_try_trigger(text)) return 1;
    if (!text || text[0] != '/') return 0;
    /* In multiplayer, slash commands belong to the server. Do not run any
       local cheat/debug command client-side. */
    if (g_mp_connected || pex_net_is_connecting()) return 0;
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "%s", text + 1);
    char *argv[8] = {0};
    int argc = 0;
    char *p = cmd;
    while (*p && argc < 8) {
        while (*p == ' ') ++p;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ') ++p;
        if (*p) *p++ = 0;
    }
    if (argc >= 1 && pex_chat_word_equals_ci(argv[0], "traceplace")) return handle_traceplace_command(argc, argv);
    if (argc >= 1 && pex_chat_word_equals_ci(argv[0], "setblock")) return handle_setblock_command(argc, argv);
    if (argc >= 1 && (pex_chat_word_equals_ci(argv[0], "gamemode") || pex_chat_word_equals_ci(argv[0], "gm"))) {
        if (argc < 2) {
            hud_add_chat(g_game_mode ? "Game mode is Creative." : "Game mode is Survival.");
            return 1;
        }
        if (pex_chat_word_equals_ci(argv[1], "creative") || pex_chat_word_equals_ci(argv[1], "c") || !strcmp(argv[1], "1")) {
            g_game_mode = 1;
            g_pending_game_mode = 1;
            g_player_dead = 0;
            player_health_set_silent(20);
            player_food_reset();
            hud_add_chat("Set game mode to Creative.");
            g_save_dirty = 1;
            return 1;
        }
        if (pex_chat_word_equals_ci(argv[1], "survival") || pex_chat_word_equals_ci(argv[1], "s") || !strcmp(argv[1], "0")) {
            g_game_mode = 0;
            g_pending_game_mode = 0;
            g_creative_flying = 0;
            g_creative_fly_toggle_timer = 0;
            hud_add_chat("Set game mode to Survival.");
            g_save_dirty = 1;
            return 1;
        }
        hud_add_chat("Usage: /gamemode <survival|creative|0|1>");
        return 1;
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
    if (g_screen == SCREEN_VIRTUAL_KEYBOARD) {
        if (vk == VK_LEFT) { pex_virtual_keyboard_move(-1, 0); return; }
        if (vk == VK_RIGHT) { pex_virtual_keyboard_move(1, 0); return; }
        if (vk == VK_UP) { pex_virtual_keyboard_move(0, -1); return; }
        if (vk == VK_DOWN) { pex_virtual_keyboard_move(0, 1); return; }
        if (vk == VK_BACK) { pex_virtual_keyboard_backspace(); return; }
        if (vk == VK_ESCAPE) { pex_virtual_keyboard_cancel(); return; }
        if (vk == VK_RETURN || vk == ' ') { pex_virtual_keyboard_select(); return; }
        return;
    }
    if (vk == VK_F11) { toggle_fullscreen(); return; }
    if (g_screen == SCREEN_CONTROLS && g_waiting_key >= 0) { key_to_control((int)vk); return; }

    if (g_screen == SCREEN_CHAT) {
        if (vk == VK_ESCAPE) { g_chat_input[0] = 0; set_screen(SCREEN_INGAME); return; }
        if (vk == VK_RETURN) {
            if (strlen(g_chat_input) > 0) {
                if (g_mp_connected) {
                    pex_net_send_chat(g_chat_input);
                } else if (!handle_local_chat_command(g_chat_input)) {
                    char line[180];
                    snprintf(line, sizeof(line), "<Player> %s", g_chat_input);
                    hud_add_chat(line);
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
        if ((int)vk == g_opts.keys[7]) { set_screen(player_is_creative() ? SCREEN_CREATIVE : SCREEN_INVENTORY); return; }
        if ((int)vk == g_opts.keys[8]) { g_chat_input[0] = 0; g_suppress_next_chat_char = 1; set_screen(SCREEN_CHAT); return; }
        if (vk >= '1' && vk <= '9') { g_selected_hotbar_slot = (int)(vk - '1'); return; }
        return;
    }

    if (g_screen == SCREEN_LANGUAGE) {
        if (vk == VK_ESCAPE) { save_options(); set_screen(g_language_return_screen); return; }
        if (vk == VK_PRIOR) { language_scroll_by(-5); return; }
        if (vk == VK_NEXT) { language_scroll_by(5); return; }
        return;
    }

    if (g_screen == SCREEN_CREATIVE) {
        if (vk == VK_ESCAPE || (int)vk == g_opts.keys[7]) { set_screen(SCREEN_INGAME); return; }
        if (vk == VK_PRIOR) { creative_scroll_page(-1); return; }
        if (vk == VK_NEXT) { creative_scroll_page(1); return; }
        return;
    }

    if (g_screen == SCREEN_INVENTORY || g_screen == SCREEN_WORKBENCH || g_screen == SCREEN_FURNACE || g_screen == SCREEN_CHEST) {
        if (vk == VK_ESCAPE || (int)vk == g_opts.keys[7]) { set_screen(SCREEN_INGAME); return; }
        return;
    }

    if (vk == VK_ESCAPE) {
        if (g_screen == SCREEN_TITLE || g_screen == SCREEN_GENERATING || g_screen == SCREEN_TEXPACK_INSTALL) { if (g_screen == SCREEN_TEXPACK_INSTALL) pack_install_request_cancel(); return; }
        if (g_screen == SCREEN_PAUSE) set_screen(SCREEN_INGAME);
        else if (g_screen == SCREEN_OPTIONS) set_screen(g_parent_screen);
        else if (g_screen == SCREEN_OPTIONS_MORE) set_screen(SCREEN_OPTIONS);
        else if (g_screen == SCREEN_SET_NAME) { if (!g_name_screen_first_run) set_screen(g_name_return_screen); }
        else if (g_screen == SCREEN_TV_REMOTE_MAP) { if (!g_opts.tv_remote_mapped) pex_tv_remote_apply_defaults(); if (g_tv_remote_return_screen != SCREEN_TITLE) set_screen(g_tv_remote_return_screen); else set_screen(pex_startup_screen()); }
        else if (g_screen == SCREEN_VIRTUAL_KEYBOARD) pex_virtual_keyboard_cancel();
        else if (g_screen == SCREEN_SYSTEM_INFO) set_screen(SCREEN_OPTIONS_MORE);
        else if (g_screen == SCREEN_SKINS) set_screen(SCREEN_OPTIONS);
        else if (g_screen == SCREEN_CONTROLS) set_screen(SCREEN_OPTIONS);
        else if (g_screen == SCREEN_WORLD_SELECT) set_screen(g_parent_screen);
        else if (g_screen == SCREEN_CREATE_WORLD) set_screen(SCREEN_WORLD_SELECT);
        else if (g_screen == SCREEN_WORLD_TYPE) set_screen(SCREEN_WORLD_SELECT);
        else if (g_screen == SCREEN_RENAME_WORLD) set_screen(SCREEN_WORLD_SELECT);
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
    if (g_screen == SCREEN_SET_NAME) {
        if (vk == VK_RETURN && (pex_tv_remote_platform_enabled() || pex_virtual_keyboard_enabled()) && !g_ui_text_input_active) { pex_ui_text_input_begin_for_current_field(); return; }
        size_t len = strlen(g_name_edit_text);
        if (vk == VK_BACK && len > 0) { g_name_edit_text[len - 1] = 0; rebuild_screen(); }
        else if (vk == VK_RETURN && pex_nickname_valid(g_name_edit_text)) pex_commit_nickname();
        return;
    }
    if (g_screen == SCREEN_CREATE_WORLD) {
        if (vk == VK_RETURN && (pex_tv_remote_platform_enabled() || pex_virtual_keyboard_enabled()) && !g_ui_text_input_active) { pex_ui_text_input_begin_for_current_field(); return; }
        char *field = g_create_more_options ? g_pending_seed_text : g_pending_world_name;
        size_t len = strlen(field);
        if (vk == VK_BACK && len > 0) { field[len - 1] = 0; if (!g_create_more_options) create_world_update_folder(); rebuild_screen(); }
        else if (vk == VK_TAB) { g_create_more_options = !g_create_more_options; g_create_edit_field = g_create_more_options ? 1 : 0; rebuild_screen(); }
        else if (vk == VK_RETURN && g_pending_world_name[0]) create_world_start();
        return;
    }
    if (g_screen == SCREEN_RENAME_WORLD) {
        if (vk == VK_RETURN && (pex_tv_remote_platform_enabled() || pex_virtual_keyboard_enabled()) && !g_ui_text_input_active) { pex_ui_text_input_begin_for_current_field(); return; }
        size_t len = strlen(g_rename_world_text);
        if (vk == VK_BACK && len > 0) { g_rename_world_text[len - 1] = 0; rebuild_screen(); }
        else if (vk == VK_RETURN && g_rename_world_text[0]) rename_selected_world_save();
        return;
    }
    if (g_screen == SCREEN_MULTIPLAYER) {
        if (pex_mp_server_mode_get() == 0) {
            int sel = pex_mp_server_selected_get();
            if (vk == VK_UP) { pex_mp_server_select(sel > 0 ? sel - 1 : 0); rebuild_screen(); }
            else if (vk == VK_DOWN) { pex_mp_server_select(sel + 1); rebuild_screen(); }
            else if (vk == VK_PRIOR) { pex_mp_server_scroll_by(-5); rebuild_screen(); }
            else if (vk == VK_NEXT) { pex_mp_server_scroll_by(5); rebuild_screen(); }
            else if (vk == VK_RETURN && sel >= 0) {
                for (int i = 0; i < g_button_count; ++i) if (g_buttons[i].id == 10) { on_button(&g_buttons[i]); break; }
            }
        } else {
            if (vk == VK_RETURN && (pex_tv_remote_platform_enabled() || pex_virtual_keyboard_enabled()) && !g_ui_text_input_active) { pex_ui_text_input_begin_for_current_field(); return; }
            char *field = pex_mp_server_edit_field_get() == 1 ? g_mp_server_edit_name : g_mp_server_edit_address;
            size_t len = strlen(field);
            if (vk == VK_TAB) {
                g_mp_server_edit_field = 1 - g_mp_server_edit_field;
                if (pex_mp_server_mode_get() == 1) g_mp_server_edit_field = 0;
                rebuild_screen();
            } else if (vk == VK_BACK && len > 0) {
                field[len - 1] = 0;
                rebuild_screen();
            } else if (vk == VK_RETURN && g_mp_server_edit_address[0]) {
                for (int i = 0; i < g_button_count; ++i) if (g_buttons[i].id == 10) { on_button(&g_buttons[i]); break; }
            }
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
    if (g_screen == SCREEN_SET_NAME) {
        if (ch == 13 || ch == 8 || ch == 9 || ch == 27) return;
        if (pex_valid_nickname_char((char)ch) && strlen(g_name_edit_text) < 16) {
            size_t len = strlen(g_name_edit_text);
            g_name_edit_text[len] = (char)ch;
            g_name_edit_text[len + 1] = 0;
            rebuild_screen();
        }
        return;
    }
    if (g_screen == SCREEN_CREATE_WORLD) {
        if (ch == 13 || ch == 8 || ch == 9 || ch == 27) return;
        char *field = g_create_more_options ? g_pending_seed_text : g_pending_world_name;
        size_t cap = g_create_more_options ? sizeof(g_pending_seed_text) : sizeof(g_pending_world_name);
        const char *allowed = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
        if (strchr(allowed, (char)ch) && strlen(field) + 1 < cap) {
            size_t len = strlen(field); field[len] = (char)ch; field[len + 1] = 0;
            if (!g_create_more_options) create_world_update_folder();
            rebuild_screen();
        }
        return;
    }
    if (g_screen == SCREEN_RENAME_WORLD) {
        if (ch == 13 || ch == 8 || ch == 9 || ch == 27) return;
        const char *allowed = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
        if (strchr(allowed, (char)ch) && strlen(g_rename_world_text) + 1 < sizeof(g_rename_world_text)) {
            size_t len = strlen(g_rename_world_text); g_rename_world_text[len] = (char)ch; g_rename_world_text[len + 1] = 0; rebuild_screen();
        }
        return;
    }
    if (g_screen == SCREEN_MULTIPLAYER) {
        if (ch == 13 || ch == 8 || ch == 9 || pex_mp_server_mode_get() == 0) return;
        char *field = pex_mp_server_edit_field_get() == 1 ? g_mp_server_edit_name : g_mp_server_edit_address;
        size_t cap = pex_mp_server_edit_field_get() == 1 ? sizeof(g_mp_server_edit_name) : sizeof(g_mp_server_edit_address);
        const char *allowed_name = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
        const char *allowed_server = ".-_:0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        const char *allowed = pex_mp_server_edit_field_get() == 1 ? allowed_name : allowed_server;
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
