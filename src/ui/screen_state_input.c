/* Split from original monolithic main.c. Included by src/main.c unity build. */

static void rebuild_screen(void);

static void set_screen(ScreenId s) {
    g_screen = s;
    g_waiting_key = -1;
    set_mouse_grabbed(s == SCREEN_INGAME);
    clear_buttons();
    rebuild_screen();
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

static void rebuild_screen(void) {
    clear_buttons();
    if (g_screen == SCREEN_TITLE) {
        char seasonal[MAX_LABEL];
        snprintf(g_splash, sizeof(g_splash), "Finally beta!");
        if (is_seasonal_splash(seasonal, sizeof(seasonal))) snprintf(g_splash, sizeof(g_splash), "%s", seasonal);
        int y0 = g_gui_h / 4 + 48;
        add_button(1, g_gui_w / 2 - 100, y0, "Singleplayer");
        add_button(2, g_gui_w / 2 - 100, y0 + 24, "Multiplayer");
        add_button(3, g_gui_w / 2 - 100, y0 + 48, "Mods and Texture Packs");
        add_button_full(0, g_gui_w / 2 - 100, y0 + 72 + 12, 98, 20, "Options...", BUTTON_NORMAL);
        add_button_full(4, g_gui_w / 2 + 2, y0 + 72 + 12, 98, 20, "Quit Game", BUTTON_NORMAL);
    } else if (g_screen == SCREEN_OPTIONS) {
        for (int i = 0; i < OPT_COUNT; i++) {
            int x = g_gui_w / 2 - 155 + (i % 2) * 160;
            int y = g_gui_h / 6 + 24 * (i >> 1);
            char label[MAX_LABEL];
            get_option_label((OptionId)i, label, sizeof(label));
            Button *b = add_button_full(i, x, y, 150, 20, label, opt_is_slider[i] ? BUTTON_SLIDER : BUTTON_NORMAL);
            b->opt = (OptionId)i;
            if (b->kind == BUTTON_SLIDER) b->slider_value = get_option_float((OptionId)i);
        }
        add_button(100, g_gui_w / 2 - 100, g_gui_h / 6 + 120 + 12, "Controls...");
        add_button(200, g_gui_w / 2 - 100, g_gui_h / 6 + 168, "Done");
    } else if (g_screen == SCREEN_CONTROLS) {
        int x0 = g_gui_w / 2 - 155;
        for (int i = 0; i < 10; i++) {
            char kn[64], label[96];
            snprintf(label, sizeof(label), "%s", key_name(g_opts.keys[i], kn, sizeof(kn)));
            add_button_full(i, x0 + (i % 2) * 160, g_gui_h / 6 + 24 * (i >> 1), 70, 20, label, BUTTON_NORMAL);
        }
        add_button(200, g_gui_w / 2 - 100, g_gui_h / 6 + 168, "Done");
    } else if (g_screen == SCREEN_WORLD_SELECT || g_screen == SCREEN_WORLD_DELETE) {
        for (int i = 0; i < 5; i++) {
            char dir[MAX_PATHBUF], label[MAX_LABEL];
            snprintf(dir, sizeof(dir), "%s\\World%d", g_save_dir, i + 1);
            char level_path[MAX_PATHBUF];
            snprintf(level_path, sizeof(level_path), "%s\\level.dat", dir);
            if (!file_exists(level_path)) snprintf(label, sizeof(label), "- empty -");
            else {
                unsigned long long sz = dir_size(dir);
                float mb = (float)((sz / 1024ULL) * 100ULL / 1024ULL) / 100.0f;
                snprintf(label, sizeof(label), "World %d (%.2f MB)", i + 1, mb);
            }
            add_button(i, g_gui_w / 2 - 100, g_gui_h / 6 + 24 * i, label);
        }
        if (g_screen == SCREEN_WORLD_SELECT) add_button(5, g_gui_w / 2 - 100, g_gui_h / 6 + 120 + 12, "Delete world...");
        add_button(6, g_gui_w / 2 - 100, g_gui_h / 6 + 168, "Cancel");
    } else if (g_screen == SCREEN_CONFIRM_DELETE) {
        add_button_full(0, g_gui_w / 2 - 155 + 0, g_gui_h / 6 + 96, 150, 20, "Yes", BUTTON_NORMAL);
        add_button_full(1, g_gui_w / 2 - 155 + 160, g_gui_h / 6 + 96, 150, 20, "No", BUTTON_NORMAL);
    } else if (g_screen == SCREEN_MULTIPLAYER) {
        Button *connect = add_button(0, g_gui_w / 2 - 100, g_gui_h / 4 + 96 + 12, "Connect");
        connect->enabled = (int)strlen(g_multiplayer_ip) > 0;
        add_button(1, g_gui_w / 2 - 100, g_gui_h / 4 + 120 + 12, "Cancel");
    } else if (g_screen == SCREEN_TEXPACK) {
        scan_texture_packs();
        add_button_full(5, g_gui_w / 2 - 154, g_gui_h - 48, 150, 20, "Open texture pack folder", BUTTON_NORMAL);
        add_button_full(6, g_gui_w / 2 + 4, g_gui_h - 48, 150, 20, "Done", BUTTON_NORMAL);
    } else if (g_screen == SCREEN_PAUSE) {
        add_button(4, g_gui_w / 2 - 100, g_gui_h / 4 + 24, "Back to game");
        add_button(1, g_gui_w / 2 - 100, g_gui_h / 4 + 48, "Save and quit to title");
        add_button(0, g_gui_w / 2 - 100, g_gui_h / 4 + 96, "Options...");
    } else if (g_screen == SCREEN_NOTICE) {
        add_button(0, g_gui_w / 2 - 100, g_gui_h / 4 + 120 + 12, "Back to title screen");
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

static void on_button(Button *b) {
    if (!b || !b->enabled) return;
    if (g_screen == SCREEN_TITLE) {
        if (b->id == 0) { g_parent_screen = SCREEN_TITLE; set_screen(SCREEN_OPTIONS); }
        else if (b->id == 1) { g_parent_screen = SCREEN_TITLE; set_screen(SCREEN_WORLD_SELECT); }
        else if (b->id == 2) { g_parent_screen = SCREEN_TITLE; set_screen(SCREEN_MULTIPLAYER); }
        else if (b->id == 3) { g_parent_screen = SCREEN_TITLE; set_screen(SCREEN_TEXPACK); }
        else if (b->id == 4) { PostQuitMessage(0); }
    } else if (g_screen == SCREEN_OPTIONS) {
        if (b->id < 100) {
            if (b->kind == BUTTON_NORMAL) {
                bump_option(b->opt, 1);
                get_option_label(b->opt, b->label, sizeof(b->label));
            }
        } else if (b->id == 100) set_screen(SCREEN_CONTROLS);
        else if (b->id == 200) set_screen(g_parent_screen);
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
        if (b->id < 5) {
            start_world_generation(b->id + 1);
        } else if (b->id == 5) set_screen(SCREEN_WORLD_DELETE);
        else if (b->id == 6) set_screen(g_parent_screen);
    } else if (g_screen == SCREEN_WORLD_DELETE) {
        if (b->id < 5) {
            char dir[MAX_PATHBUF];
            snprintf(dir, sizeof(dir), "%s\\World%d", g_save_dir, b->id + 1);
            if (dir_exists(dir)) { g_confirm_world = b->id + 1; set_screen(SCREEN_CONFIRM_DELETE); }
        } else if (b->id == 6) set_screen(SCREEN_WORLD_SELECT);
    } else if (g_screen == SCREEN_CONFIRM_DELETE) {
        if (b->id == 0 && g_confirm_world > 0) {
            char dir[MAX_PATHBUF];
            snprintf(dir, sizeof(dir), "%s\\World%d", g_save_dir, g_confirm_world);
            if (dir_exists(dir)) delete_recursive(dir);
        }
        g_confirm_world = 0;
        set_screen(SCREEN_WORLD_SELECT);
    } else if (g_screen == SCREEN_MULTIPLAYER) {
        if (b->id == 1) set_screen(g_parent_screen);
        else if (b->id == 0) {
            snprintf(g_opts.last_server, sizeof(g_opts.last_server), "%s", g_multiplayer_ip);
            save_options();
            open_notice("Play Multiplayer", "Minecraft Multiplayer is currently not finished, but there", "is some buggy early testing going on.");
        }
    } else if (g_screen == SCREEN_TEXPACK) {
        if (b->id == 5) ShellExecuteA(NULL, "open", g_texpack_dir, NULL, NULL, SW_SHOWNORMAL);
        else if (b->id == 6) set_screen(g_parent_screen);
    } else if (g_screen == SCREEN_PAUSE) {
        if (b->id == 4) set_screen(SCREEN_INGAME);
        else if (b->id == 0) { g_parent_screen = SCREEN_PAUSE; set_screen(SCREEN_OPTIONS); }
        else if (b->id == 1) leave_world_to_title();
    } else if (g_screen == SCREEN_NOTICE) {
        set_screen(SCREEN_TITLE);
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
            if (idx != g_selected_texpack) apply_texture_pack_index(idx);
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
    if (g_screen == SCREEN_INVENTORY) { inventory_mouse_click(mx, my, 1); return; }
    if (g_screen == SCREEN_INGAME) { set_mouse_grabbed(1); ingame_right_click(); return; }
    (void)mx; (void)my;
}

static void mouse_down(int mx, int my) {
    g_mouse_down = 1;
    if (g_screen == SCREEN_INVENTORY) { inventory_mouse_click(mx, my, 0); return; }
    if (g_screen == SCREEN_INGAME) {
        set_mouse_grabbed(1);
        FlatRayHit hit = flat_raycast();
        if (!hit.hit) start_air_swing_once();
        return;
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

static void handle_keydown(WPARAM vk) {
    if (vk == VK_F11) { toggle_fullscreen(); return; }
    if (g_screen == SCREEN_CONTROLS && g_waiting_key >= 0) { key_to_control((int)vk); return; }

    if (g_screen == SCREEN_CHAT) {
        if (vk == VK_ESCAPE) { g_chat_input[0] = 0; set_screen(SCREEN_INGAME); return; }
        if (vk == VK_RETURN) {
            char line[180];
            if (strlen(g_chat_input) > 0) {
                snprintf(line, sizeof(line), "<Player> %s", g_chat_input);
                hud_add_chat(line);
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
        if (vk == VK_F3) { g_debug_menu_shown = !g_debug_menu_shown; return; }
        if (vk == VK_F5) { g_third_person_view = !g_third_person_view; return; }
        if ((int)vk == g_opts.keys[6]) { inventory_drop_selected_one(); return; }
        if ((int)vk == g_opts.keys[7]) { set_screen(SCREEN_INVENTORY); return; }
        if ((int)vk == g_opts.keys[8]) { g_chat_input[0] = 0; set_screen(SCREEN_CHAT); return; }
        if (vk >= '1' && vk <= '9') { g_selected_hotbar_slot = (int)(vk - '1'); return; }
        return;
    }

    if (g_screen == SCREEN_INVENTORY) {
        if (vk == VK_ESCAPE || (int)vk == g_opts.keys[7]) { set_screen(SCREEN_INGAME); return; }
        return;
    }

    if (vk == VK_ESCAPE) {
        if (g_screen == SCREEN_TITLE || g_screen == SCREEN_GENERATING) return;
        if (g_screen == SCREEN_PAUSE) set_screen(SCREEN_INGAME);
        else if (g_screen == SCREEN_OPTIONS) set_screen(g_parent_screen);
        else if (g_screen == SCREEN_CONTROLS) set_screen(SCREEN_OPTIONS);
        else if (g_screen == SCREEN_WORLD_SELECT) set_screen(g_parent_screen);
        else if (g_screen == SCREEN_WORLD_DELETE) set_screen(SCREEN_WORLD_SELECT);
        else if (g_screen == SCREEN_CONFIRM_DELETE) set_screen(SCREEN_WORLD_SELECT);
        else if (g_screen == SCREEN_MULTIPLAYER) set_screen(g_parent_screen);
        else if (g_screen == SCREEN_TEXPACK) set_screen(g_parent_screen);
        else if (g_screen == SCREEN_NOTICE) set_screen(SCREEN_TITLE);
        return;
    }
    if (g_screen == SCREEN_MULTIPLAYER) {
        size_t len = strlen(g_multiplayer_ip);
        if (vk == VK_BACK && len > 0) {
            g_multiplayer_ip[len - 1] = 0;
            rebuild_screen();
        } else if (vk == VK_RETURN && len > 0) {
            on_button(&g_buttons[0]);
        }
    }
}

static void handle_char(WPARAM ch) {
    if (g_screen == SCREEN_CHAT) {
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
        if (ch == 13 || ch == 8) return;
        const char *allowed = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_'abcdefghijklmnopqrstuvwxyz{|}~";
        if (strchr(allowed, (char)ch) && strlen(g_multiplayer_ip) < 32) {
            size_t len = strlen(g_multiplayer_ip);
            g_multiplayer_ip[len] = (char)ch;
            g_multiplayer_ip[len + 1] = 0;
            rebuild_screen();
        }
    }
}

static void draw_all_buttons(void) {
    for (int i = 0; i < g_button_count; i++) draw_button(&g_buttons[i]);
}

