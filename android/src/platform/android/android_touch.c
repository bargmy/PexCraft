/* Android PE-style touchscreen controls.

   This is a mobile Minecraft-style control layer:
     - visible movement D-pad only on the lower-left
     - swipe/drag anywhere in the world view to look
     - tap the world to place/use/open from the center crosshair
     - hold the world to mine from the center crosshair
     - tap/drag inventory slots directly, with the carried stack following the finger

   World use/mining intentionally ignores the finger position.  Touches choose
   the action; the block target is always the same block selected by the + in
   the middle of the screen. */

#define PEX_ANDROID_TOUCH_NONE ((SDL_FingerID)-1)

static SDL_FingerID g_android_move_finger = PEX_ANDROID_TOUCH_NONE;
static SDL_FingerID g_android_world_finger = PEX_ANDROID_TOUCH_NONE;
static SDL_FingerID g_android_jump_finger = PEX_ANDROID_TOUCH_NONE;
static SDL_FingerID g_android_sneak_finger = PEX_ANDROID_TOUCH_NONE;
static SDL_FingerID g_android_ui_finger = PEX_ANDROID_TOUCH_NONE;

static int g_android_move_up = 0;
static int g_android_move_down = 0;
static int g_android_move_left = 0;
static int g_android_move_right = 0;
static float g_android_move_x = 0.0f;
static float g_android_move_y = 0.0f;

static int g_android_world_down_x = 0;
static int g_android_world_down_y = 0;
static int g_android_world_last_x = 0;
static int g_android_world_last_y = 0;
static int g_android_world_cur_x = 0;
static int g_android_world_cur_y = 0;
static double g_android_world_down_time = 0.0;
static int g_android_world_moved = 0;
static int g_android_world_breaking = 0;

static int g_android_touch_seen = 0;
static int g_android_jump_down = 0;
static int g_android_sneak_down = 0; /* latched toggle, not press-and-hold */
static int g_android_inv_dragging = 0;
static int g_android_inv_drag_start_slot = -1;
static int g_android_inv_drag_down_x = 0;
static int g_android_inv_drag_down_y = 0;
static int g_android_inv_drag_last_x = 0;
static int g_android_inv_drag_last_y = 0;
static double g_android_inv_drag_down_time = 0.0;
static int g_android_inv_drag_moved = 0;
static int g_android_inv_hold_split_done = 0;

static void pex_android_touch_set_focus(void) {
    g_input_focus_mode = PEX_INPUT_FOCUS_TOUCH;
    g_gamepad_virtual_cursor_active = 0;
    g_gamepad_menu_index = -1;
}

static int pex_android_touch_gui_x(float nx) {
    if (nx < 0.0f) nx = 0.0f;
    if (nx > 1.0f) nx = 1.0f;
    return (int)(nx * (float)g_gui_w + 0.5f);
}

static int pex_android_touch_gui_y(float ny) {
    if (ny < 0.0f) ny = 0.0f;
    if (ny > 1.0f) ny = 1.0f;
    return (int)(ny * (float)g_gui_h + 0.5f);
}

static int pex_android_touch_in_rect(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && y >= ry && x < rx + rw && y < ry + rh;
}

static int pex_android_touch_button_close(int x, int y) {
    return pex_android_touch_in_rect(x, y, g_gui_w - 38, 6, 32, 24);
}

static float pex_android_touch_clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static int android_touch_inventory_screen(void) {
    return g_screen == SCREEN_INVENTORY || g_screen == SCREEN_WORKBENCH ||
           g_screen == SCREEN_FURNACE || g_screen == SCREEN_CHEST;
}

static int pex_android_touch_hotbar_slot(int x, int y) {
    int hotbar_x = g_gui_w / 2 - 91;
    int hotbar_y = g_gui_h - 26;
    if (y < hotbar_y - 5 || y > g_gui_h) return -1;
    for (int i = 0; i < 9; ++i) {
        int sx = hotbar_x + i * 20;
        if (x >= sx && x < sx + 20) return i;
    }
    return -1;
}

static int pex_android_touch_button_pause(int x, int y) {
    return pex_android_touch_in_rect(x, y, g_gui_w - 50, 4, 46, 24);
}

static int android_touch_inventory_button(int x, int y) {
    return pex_android_touch_in_rect(x, y, g_gui_w - 58, 34, 54, 26);
}

static void pex_android_touch_dpad_rect(int *left, int *top, int *size);

static int pex_android_touch_button_jump(int x, int y) {
    return pex_android_touch_in_rect(x, y, g_gui_w - 62, g_gui_h - 92, 54, 54);
}

static int pex_android_touch_button_sneak(int x, int y) {
    int l, t, s;
    pex_android_touch_dpad_rect(&l, &t, &s);
    int third = s / 3;
    return pex_android_touch_in_rect(x, y, l + third, t + third, third, third);
}

static void pex_android_touch_dpad_rect(int *left, int *top, int *size) {
    int s = g_gui_h / 3;
    if (s < 78) s = 78;
    if (s > 132) s = 132;
    *size = s;
    *left = 12;
    *top = g_gui_h - s - 20;
}

static int pex_android_touch_in_dpad(int x, int y) {
    int l, t, s;
    pex_android_touch_dpad_rect(&l, &t, &s);
    return pex_android_touch_in_rect(x, y, l, t, s, s);
}

static void pex_android_touch_update_dpad(int x, int y) {
    int l, t, s;
    pex_android_touch_dpad_rect(&l, &t, &s);
    float cx = (float)l + (float)s * 0.5f;
    float cy = (float)t + (float)s * 0.5f;
    float dx = ((float)x - cx) / ((float)s * 0.38f);
    float dy = ((float)y - cy) / ((float)s * 0.38f);
    dx = pex_android_touch_clampf(dx, -1.0f, 1.0f);
    dy = pex_android_touch_clampf(dy, -1.0f, 1.0f);
    g_android_move_x = dx;
    g_android_move_y = dy;
    g_android_move_left = dx < -0.30f;
    g_android_move_right = dx > 0.30f;
    g_android_move_up = dy < -0.30f;
    g_android_move_down = dy > 0.30f;
}

static void pex_android_touch_clear_move(void) {
    g_android_move_finger = PEX_ANDROID_TOUCH_NONE;
    g_android_move_up = g_android_move_down = g_android_move_left = g_android_move_right = 0;
    g_android_move_x = g_android_move_y = 0.0f;
}

static void pex_android_touch_clear_pick_ray(void) {
    g_android_touch_pick_active = 0;
}

static void pex_android_touch_center(int *x, int *y) {
    if (x) *x = g_gui_w / 2;
    if (y) *y = g_gui_h / 2;
}

static void pex_android_touch_set_pick_ray(int x, int y) {
    float fov = g_opts.fov;
    if (fov < 30.0f) fov = 30.0f;
    if (fov > 110.0f) fov = 110.0f;
    if (flat_player_head_in_water() && fov > 60.0f) fov = 60.0f;

    float aspect = (g_gui_h > 0) ? (float)g_gui_w / (float)g_gui_h : 16.0f / 9.0f;
    float tan_half = tanf(fov * (float)M_PI / 360.0f);
    float ndc_x = (((float)x + 0.5f) / (float)(g_gui_w > 0 ? g_gui_w : 1)) * 2.0f - 1.0f;
    float ndc_y = 1.0f - (((float)y + 0.5f) / (float)(g_gui_h > 0 ? g_gui_h : 1)) * 2.0f;

    float yaw = g_player_yaw * (float)M_PI / 180.0f;
    float pitch = g_player_pitch * (float)M_PI / 180.0f;
    float cp = cosf(pitch);

    float fx = -sinf(yaw) * cp;
    float fy = -sinf(pitch);
    float fz =  cosf(yaw) * cp;

    float rx = fz;
    float ry = 0.0f;
    float rz = -fx;
    float rlen = sqrtf(rx * rx + ry * ry + rz * rz);
    if (rlen < 0.0001f) { rx = 1.0f; ry = 0.0f; rz = 0.0f; rlen = 1.0f; }
    rx /= rlen; ry /= rlen; rz /= rlen;

    float ux = fy * rz - fz * ry;
    float uy = fz * rx - fx * rz;
    float uz = fx * ry - fy * rx;

    float sx = ndc_x * aspect * tan_half;
    float sy = ndc_y * tan_half;
    float dx = fx + rx * sx + ux * sy;
    float dy = fy + ry * sx + uy * sy;
    float dz = fz + rz * sx + uz * sy;
    float len = sqrtf(dx * dx + dy * dy + dz * dz);
    if (len < 0.0001f) { dx = fx; dy = fy; dz = fz; len = 1.0f; }

    g_android_touch_ray_dx = dx / len;
    g_android_touch_ray_dy = dy / len;
    g_android_touch_ray_dz = dz / len;
    g_android_touch_pick_active = 1;
}

static void pex_android_touch_stop_break(void) {
    if (g_android_world_breaking) {
        int cx, cy;
        pex_android_touch_center(&cx, &cy);
        g_android_world_breaking = 0;
        g_sdl2_mouse_buttons[SDL_BUTTON_LEFT] = 0;
        mouse_up(cx, cy);
    }
    pex_android_touch_clear_pick_ray();
}

static void pex_android_touch_begin_break(void) {
    if (g_android_world_breaking) return;
    int cx, cy;
    pex_android_touch_center(&cx, &cy);
    g_android_world_breaking = 1;
    pex_android_touch_clear_pick_ray();
    g_mouse_x = cx;
    g_mouse_y = cy;
    /* mouse_down() starts the hit animation, but update_breaking() checks the
       SDL button state via key_down_vk(VK_LBUTTON).  Touch has to latch that
       state too, otherwise Android only shows the cracking overlay/swing and
       never advances mining damage. */
    g_sdl2_mouse_buttons[SDL_BUTTON_LEFT] = 1;
    mouse_down(g_mouse_x, g_mouse_y);
}

static void android_touch_reset_inventory(void) {
    g_android_inv_dragging = 0;
    g_android_inv_drag_start_slot = -1;
    g_android_inv_drag_down_x = 0;
    g_android_inv_drag_down_y = 0;
    g_android_inv_drag_last_x = 0;
    g_android_inv_drag_last_y = 0;
    g_android_inv_drag_down_time = 0.0;
    g_android_inv_drag_moved = 0;
    g_android_inv_hold_split_done = 0;
}

static void pex_android_touch_ui_down(SDL_FingerID id, int x, int y) {
    pex_android_touch_set_focus();
    g_android_ui_finger = id;
    g_mouse_x = x;
    g_mouse_y = y;
    if (android_touch_inventory_screen() && pex_android_touch_button_close(x, y)) {
        set_screen(SCREEN_INGAME);
        g_android_ui_finger = PEX_ANDROID_TOUCH_NONE;
        android_touch_reset_inventory();
        return;
    }
    if (android_touch_inventory_screen()) {
        /* Do not left-click immediately on Android inventory screens.  A
           touch-down may become a long-press stack split, so defer the normal
           left-click until release. */
        g_android_inv_dragging = 1;
        g_android_inv_drag_start_slot = inventory_slot_at(x, y);
        g_android_inv_drag_down_x = x;
        g_android_inv_drag_down_y = y;
        g_android_inv_drag_last_x = x;
        g_android_inv_drag_last_y = y;
        g_android_inv_drag_down_time = now_seconds();
        g_android_inv_drag_moved = 0;
        g_android_inv_hold_split_done = 0;
        return;
    }
    mouse_down(x, y);
}

static void pex_android_touch_ui_motion(SDL_FingerID id, int x, int y) {
    if (g_android_ui_finger != id) return;
    int old_y = g_mouse_y;
    g_mouse_x = x;
    g_mouse_y = y;
    if (g_drag_slider) update_slider(g_drag_slider, x);
    if (g_screen == SCREEN_TEXPACK) texpack_mouse_drag(y);
    if (g_screen == SCREEN_CREATIVE) creative_mouse_drag(y);
    if (g_screen == SCREEN_WORLD_SELECT || g_screen == SCREEN_WORLD_DELETE) {
        world_save_drag_scroll(y - old_y);
    }
    if (g_android_inv_dragging) {
        int dx = x - g_android_inv_drag_down_x;
        int dy = y - g_android_inv_drag_down_y;
        int threshold = g_gui_h / 96;
        if (threshold < 4) threshold = 4;
        if (dx * dx + dy * dy > threshold * threshold) g_android_inv_drag_moved = 1;
        g_android_inv_drag_last_x = x;
        g_android_inv_drag_last_y = y;
    }
}

static void pex_android_touch_ui_up(SDL_FingerID id, int x, int y) {
    if (g_android_ui_finger != id) return;
    g_mouse_x = x;
    g_mouse_y = y;
    if (g_android_inv_dragging) {
        int end_slot = inventory_slot_at(x, y);
        if (!g_android_inv_hold_split_done) {
            if (g_android_inv_drag_start_slot >= 0 &&
                end_slot >= 0 && end_slot != g_android_inv_drag_start_slot) {
                /* Dragging an item from one slot to another should still work
                   even though Android now defers the initial pick-up click. */
                inventory_mouse_click(g_android_inv_drag_down_x, g_android_inv_drag_down_y, 0);
                inventory_mouse_click(x, y, 0);
            } else if (g_android_inv_drag_start_slot >= 0 &&
                       end_slot < 0 && g_android_inv_drag_moved) {
                /* Match the old Android behavior for dragging a stack out of
                   the GUI: pick it up and leave it on the cursor. */
                inventory_mouse_click(g_android_inv_drag_down_x, g_android_inv_drag_down_y, 0);
            } else {
                inventory_mouse_click(x, y, 0);
            }
        }
        android_touch_reset_inventory();
    } else {
        mouse_up(x, y);
    }
    g_android_ui_finger = PEX_ANDROID_TOUCH_NONE;
}

static void pex_android_touch_ingame_down(SDL_FingerID id, int x, int y) {
    pex_android_touch_set_focus();
    int slot = pex_android_touch_hotbar_slot(x, y);
    if (slot >= 0) { g_selected_hotbar_slot = slot; return; }
    if (pex_android_touch_button_pause(x, y)) { set_screen(SCREEN_PAUSE); return; }
    if (android_touch_inventory_button(x, y)) { set_screen(SCREEN_INVENTORY); return; }
    if (pex_android_touch_button_jump(x, y)) { g_android_jump_finger = id; g_android_jump_down = 1; return; }
    if (pex_android_touch_button_sneak(x, y)) {
        g_android_sneak_finger = id;
        g_android_sneak_down = !g_android_sneak_down;
        return;
    }
    if (pex_android_touch_in_dpad(x, y) && g_android_move_finger == PEX_ANDROID_TOUCH_NONE) {
        g_android_move_finger = id;
        pex_android_touch_update_dpad(x, y);
        return;
    }

    if (g_android_world_finger == PEX_ANDROID_TOUCH_NONE) {
        g_android_world_finger = id;
        g_android_world_down_x = g_android_world_last_x = g_android_world_cur_x = x;
        g_android_world_down_y = g_android_world_last_y = g_android_world_cur_y = y;
        g_android_world_down_time = now_seconds();
        g_android_world_moved = 0;
        g_android_world_breaking = 0;
        pex_android_touch_clear_pick_ray();
    }
}

static void pex_android_touch_ingame_motion(SDL_FingerID id, int x, int y) {
    if (id == g_android_move_finger) {
        pex_android_touch_update_dpad(x, y);
        return;
    }
    if (id == g_android_world_finger) {
        int dx0 = x - g_android_world_down_x;
        int dy0 = y - g_android_world_down_y;
        int threshold = g_gui_h / 80;
        if (threshold < 5) threshold = 5;
        if (dx0 * dx0 + dy0 * dy0 > threshold * threshold) {
            g_android_world_moved = 1;
            if (g_android_world_breaking) pex_android_touch_stop_break();
        }
        if (g_android_world_moved) {
            int dx = x - g_android_world_last_x;
            int dy = y - g_android_world_last_y;
            if (dx || dy) player_turn_from_mouse((int)((float)dx * 1.35f), (int)((float)-dy * 1.35f));
        }
        g_android_world_last_x = g_android_world_cur_x = x;
        g_android_world_last_y = g_android_world_cur_y = y;
    }
}

static void pex_android_touch_ingame_up(SDL_FingerID id, int x, int y) {
    if (id == g_android_move_finger) { pex_android_touch_clear_move(); return; }
    if (id == g_android_jump_finger) { g_android_jump_finger = PEX_ANDROID_TOUCH_NONE; g_android_jump_down = 0; return; }
    if (id == g_android_sneak_finger) { g_android_sneak_finger = PEX_ANDROID_TOUCH_NONE; return; }
    if (id == g_android_world_finger) {
        int was_breaking = g_android_world_breaking;
        double held = now_seconds() - g_android_world_down_time;
        pex_android_touch_stop_break();
        if (!was_breaking && !g_android_world_moved && held < 0.55) {
            int cx, cy;
            pex_android_touch_center(&cx, &cy);
            pex_android_touch_clear_pick_ray();
            g_mouse_x = cx;
            g_mouse_y = cy;
            mouse_right_down(cx, cy);
        }
        g_android_world_finger = PEX_ANDROID_TOUCH_NONE;
        g_android_world_moved = 0;
    }
}

static void pex_android_touch_world_tick(void) {
    if (g_screen != SCREEN_INGAME) { pex_android_touch_stop_break(); return; }
    if (g_android_world_finger != PEX_ANDROID_TOUCH_NONE && !g_android_world_moved) {
        if (!g_android_world_breaking && now_seconds() - g_android_world_down_time > 0.35) {
            pex_android_touch_begin_break();
        }
        if (g_android_world_breaking) pex_android_touch_clear_pick_ray();
    } else if (!g_android_world_breaking) {
        pex_android_touch_clear_pick_ray();
    }
}

static void pex_android_touch_inventory_tick(void) {
    if (!android_touch_inventory_screen()) {
        android_touch_reset_inventory();
        return;
    }
    if (g_android_ui_finger == PEX_ANDROID_TOUCH_NONE || !g_android_inv_dragging) return;
    if (g_android_inv_hold_split_done) return;
    if (g_android_inv_drag_start_slot < 0) return;
    if (g_android_inv_drag_moved) return;

    double held = now_seconds() - g_android_inv_drag_down_time;
    if (held < 0.45) return;

    int cur_slot = inventory_slot_at(g_android_inv_drag_last_x, g_android_inv_drag_last_y);
    if (cur_slot != g_android_inv_drag_start_slot) return;

    /* Android long-press in inventory maps to the desktop right-click action:
       with an empty cursor it takes half of the stack, and with a carried
       stack it places one item. */
    g_mouse_x = g_android_inv_drag_last_x;
    g_mouse_y = g_android_inv_drag_last_y;
    inventory_mouse_click(g_mouse_x, g_mouse_y, 1);
    g_android_inv_hold_split_done = 1;
}

static int pex_android_handle_touch_event(SDL_Event *e) {
    if (!e) return 0;
    if (e->type != SDL_FINGERDOWN && e->type != SDL_FINGERUP && e->type != SDL_FINGERMOTION) return 0;
    int x = pex_android_touch_gui_x(e->tfinger.x);
    int y = pex_android_touch_gui_y(e->tfinger.y);
    SDL_FingerID id = e->tfinger.fingerId;
    g_android_touch_seen = 1;
    pex_android_touch_set_focus();

    if (e->type == SDL_FINGERDOWN) {
        if (g_screen == SCREEN_INGAME) pex_android_touch_ingame_down(id, x, y);
        else pex_android_touch_ui_down(id, x, y);
    } else if (e->type == SDL_FINGERMOTION) {
        if (g_screen == SCREEN_INGAME) pex_android_touch_ingame_motion(id, x, y);
        else pex_android_touch_ui_motion(id, x, y);
    } else if (e->type == SDL_FINGERUP) {
        if (g_screen == SCREEN_INGAME) pex_android_touch_ingame_up(id, x, y);
        else pex_android_touch_ui_up(id, x, y);
    }
    return 1;
}

static void android_touch_apply_virtual_keys(void) {
    /* Touch is not a gamepad.  Feed only the movement/action key state here so
       gameplay works without enabling controller menus/cursors. */
    if (g_input_focus_mode != PEX_INPUT_FOCUS_TOUCH) return;
    if (g_android_move_up) g_gamepad_vk_state[g_opts.keys[0] & 511] = 1;
    if (g_android_move_left) g_gamepad_vk_state[g_opts.keys[1] & 511] = 1;
    if (g_android_move_down) g_gamepad_vk_state[g_opts.keys[2] & 511] = 1;
    if (g_android_move_right) g_gamepad_vk_state[g_opts.keys[3] & 511] = 1;
    if (g_android_jump_down) g_gamepad_vk_state[g_opts.keys[4] & 511] = 1;
    if (g_android_sneak_down) g_gamepad_vk_state[g_opts.keys[5] & 511] = 1;
}

static void pex_android_touch_ingame_update(void) {
    pex_android_touch_world_tick();
    pex_android_touch_inventory_tick();
}

static void draw_android_touch_controls(void) {
    if (g_screen != SCREEN_INGAME && !android_touch_inventory_screen()) return;
    if (android_touch_inventory_screen()) {
        draw_rect(g_gui_w - 38, 6, g_gui_w - 6, 30, (int)0xAA000000u);
        draw_text("X", g_gui_w - 25, 14, 0xFFFFFF);
        draw_text("Tap/drag slots. Hold stack to split half. Tap X or Android Back to close.", 8, g_gui_h - 12, 0xE0E0E0);
        return;
    }

    int l, t, s;
    pex_android_touch_dpad_rect(&l, &t, &s);
    int third = s / 3;
    int alpha = 0x66000000u;
    int hi = 0xAAFFFFFFu;

    draw_rect(l + third, t, l + third * 2, t + third, (int)(g_android_move_up ? hi : alpha));
    draw_rect(l, t + third, l + third, t + third * 2, (int)(g_android_move_left ? hi : alpha));
    draw_rect(l + third, t + third, l + third * 2, t + third * 2, (int)(g_android_sneak_down ? hi : 0x55FFFFFFu));
    draw_rect(l + third * 2, t + third, l + s, t + third * 2, (int)(g_android_move_right ? hi : alpha));
    draw_rect(l + third, t + third * 2, l + third * 2, t + s, (int)(g_android_move_down ? hi : alpha));
    draw_text("^", l + third + third / 2 - 3, t + 5, 0xFFFFFF);
    draw_text("<", l + 6, t + third + third / 2 - 4, 0xFFFFFF);
    draw_text("S", l + third + third / 2 - 3, t + third + third / 2 - 4, g_android_sneak_down ? 0x000000 : 0xFFFFFF);
    draw_text(">", l + third * 2 + third / 2 - 3, t + third + third / 2 - 4, 0xFFFFFF);
    draw_text("v", l + third + third / 2 - 3, t + third * 2 + third / 2 - 4, 0xFFFFFF);

    draw_rect(g_gui_w - 62, g_gui_h - 92, g_gui_w - 8, g_gui_h - 38, (int)(g_android_jump_down ? 0xAAFFFFFFu : 0x55FFFFFFu));
    draw_text("JUMP", g_gui_w - 52, g_gui_h - 65, 0x000000);
    draw_rect(g_gui_w - 58, 34, g_gui_w - 4, 60, (int)0x66000000u);
    draw_text("INV", g_gui_w - 45, 43, 0xFFFFFF);
    draw_rect(g_gui_w - 50, 4, g_gui_w - 4, 28, (int)0x66000000u);
    draw_text("PAUSE", g_gui_w - 46, 12, 0xFFFFFF);

    if (g_android_world_breaking) {
        int cx, cy;
        pex_android_touch_center(&cx, &cy);
        draw_text("Breaking", cx - 22, cy + 12, 0xFFFF80);
    }
}
