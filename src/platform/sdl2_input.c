/* Linux/SDL2 input layer. Included only by src/main_sdl2.c. */

static int g_sdl2_key_state[512];
static int g_sdl2_mouse_buttons[8];

static int sdl2_vk_index(int vk) {
    if (vk >= 0 && vk < (int)ARRAY_COUNT(g_sdl2_key_state)) return vk;
    return 0;
}

static void center_mouse_in_window(void) {
    if (!g_hwnd) return;
    g_recentering_mouse = 1;
    SDL_WarpMouseInWindow(g_hwnd, g_win_w / 2, g_win_h / 2);
}

static void set_mouse_grabbed(int grabbed) {
    if (!g_hwnd) return;
    grabbed = grabbed ? 1 : 0;
    if (grabbed == g_mouse_grabbed) return;
    g_mouse_grabbed = grabbed;
    SDL_SetWindowGrab(g_hwnd, grabbed ? SDL_TRUE : SDL_FALSE);
    SDL_SetRelativeMouseMode(grabbed ? SDL_TRUE : SDL_FALSE);
    SDL_ShowCursor(grabbed ? SDL_DISABLE : SDL_ENABLE);
    g_cursor_hidden = grabbed;
    if (grabbed) center_mouse_in_window();
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
    player_turn_from_mouse(px, -py);
}

static int key_down_vk(int vk) {
    if (vk >= 0 && vk < (int)ARRAY_COUNT(g_gamepad_vk_state) && g_gamepad_vk_state[vk]) return 1;
    if (vk == VK_LBUTTON) return g_sdl2_mouse_buttons[1] != 0;
    int idx = sdl2_vk_index(vk);
    return idx ? g_sdl2_key_state[idx] : 0;
}
