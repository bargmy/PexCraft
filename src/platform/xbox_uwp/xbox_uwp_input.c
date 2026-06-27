/* Xbox UWP input bridge for the shared engine. */
static unsigned char g_xbox_uwp_key_state[512];

static void center_mouse_in_window(void) { }
static void set_mouse_grabbed(int grabbed) { g_mouse_grabbed = grabbed ? 1 : 0; }

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
    if (vk <= 0) return 0;
    if (vk >= 0 && vk < (int)ARRAY_COUNT(g_gamepad_vk_state) && g_gamepad_vk_state[vk]) return 1;
    if (vk >= 0 && vk < (int)ARRAY_COUNT(g_xbox_uwp_key_state) && g_xbox_uwp_key_state[vk]) return 1;
    return 0;
}

static void pex_xbox_uwp_set_key_state(int vk, int down) {
    if (vk >= 0 && vk < (int)ARRAY_COUNT(g_xbox_uwp_key_state)) g_xbox_uwp_key_state[vk] = down ? 1 : 0;
}
