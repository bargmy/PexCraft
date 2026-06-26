/* Sony PSP input helper layer.  Real controller state is translated through
   platform/gamepad.c so the same menu/gameplay code path is used as Xbox/SDL. */

static void center_mouse_in_window(void) { }
static void set_mouse_grabbed(int grabbed) { g_mouse_grabbed = grabbed ? 1 : 0; g_cursor_hidden = g_mouse_grabbed; }

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

static void handle_grabbed_mouse_move(int px, int py) { (void)px; (void)py; }

static int key_down_vk(int vk) {
    if (vk >= 0 && vk < (int)ARRAY_COUNT(g_gamepad_vk_state) && g_gamepad_vk_state[vk]) return 1;
    return 0;
}
