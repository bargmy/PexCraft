/* Split from original monolithic main.c. Included by src/main.c unity build. */

static void center_mouse_in_window(void) {
    if (!g_hwnd) return;
    POINT p;
    p.x = g_win_w / 2;
    p.y = g_win_h / 2;
    ClientToScreen(g_hwnd, &p);
    g_recentering_mouse = 1;
    SetCursorPos(p.x, p.y);
}

static void set_mouse_grabbed(int grabbed) {
    if (!g_hwnd) return;
    if (grabbed) {
        if (!g_mouse_grabbed) {
            RECT r;
            GetClientRect(g_hwnd, &r);
            POINT tl = { r.left, r.top };
            POINT br = { r.right, r.bottom };
            ClientToScreen(g_hwnd, &tl);
            ClientToScreen(g_hwnd, &br);
            r.left = tl.x; r.top = tl.y; r.right = br.x; r.bottom = br.y;
            ClipCursor(&r);
            SetCapture(g_hwnd);
            if (!g_cursor_hidden) { ShowCursor(FALSE); g_cursor_hidden = 1; }
            g_mouse_grabbed = 1;
        }
        center_mouse_in_window();
    } else {
        if (g_mouse_grabbed) {
            ClipCursor(NULL);
            ReleaseCapture();
            if (g_cursor_hidden) { ShowCursor(TRUE); g_cursor_hidden = 0; }
            g_mouse_grabbed = 0;
            g_recentering_mouse = 0;
        }
    }
}

static float lerp_angle(float a, float b, float partial) {
    float d = b - a;
    while (d < -180.0f) d += 360.0f;
    while (d >= 180.0f) d -= 360.0f;
    return a + d * partial;
}

static void player_turn_from_mouse(int dx, int dy_java) {
    float s = g_opts.sensitivity * 0.6f + 0.2f;
    float mult = s * s * s * 8.0f;
    float mx = (float)dx * mult;
    float my = (float)dy_java * mult;
    if (g_opts.invert_mouse) my = -my;

    /* mm.java::d(float,float): yaw += x*0.15, pitch -= y*0.15, clamp to +/-90.
       Mouse look is render-rate input, not 20 Hz simulation input.  Move the
       previous angles by the same delta so partial-tick interpolation does not
       visually damp camera movement; otherwise 300 FPS can still feel like the
       camera is updating at the game tick rate. */
    float old_yaw = g_player_yaw;
    float old_pitch = g_player_pitch;
    float new_yaw = old_yaw + mx * 0.15f;
    float new_pitch = old_pitch - my * 0.15f;
    if (new_pitch < -90.0f) new_pitch = -90.0f;
    if (new_pitch > 90.0f) new_pitch = 90.0f;
    g_player_yaw = new_yaw;
    g_player_pitch = new_pitch;
    g_player_prev_yaw += new_yaw - old_yaw;
    g_player_prev_pitch += new_pitch - old_pitch;
}

static void handle_grabbed_mouse_move(int px, int py) {
    if (!g_mouse_grabbed || g_screen != SCREEN_INGAME) return;
    int cx = g_win_w / 2;
    int cy = g_win_h / 2;
    int dx = px - cx;
    int dy_java = cy - py; /* LWJGL Mouse.getDY() is positive upward. */
    if (g_recentering_mouse && dx == 0 && dy_java == 0) {
        g_recentering_mouse = 0;
        return;
    }
    if (dx != 0 || dy_java != 0) {
        player_turn_from_mouse(dx, dy_java);
        center_mouse_in_window();
    }
}

static int key_down_vk(int vk) {
    if (vk <= 0) return 0;
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

