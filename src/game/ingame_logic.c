/* Split from original monolithic main.c. Included by src/main.c unity build. */


static void hud_add_chat(const char *msg) {
    if (!msg || !msg[0]) return;
    memmove(&g_chat_lines[1], &g_chat_lines[0], sizeof(ChatLine) * (MAX_CHAT_LINES - 1));
    snprintf(g_chat_lines[0].text, sizeof(g_chat_lines[0].text), "%s", msg);
    g_chat_lines[0].age = 0;
    if (g_chat_count < MAX_CHAT_LINES) g_chat_count++;
}

static void ingame_tick(void) {
    g_ingame_ticks++;
    for (int i = 0; i < g_chat_count; i++) g_chat_lines[i].age++;
    inventory_tick();
    update_dropped_items();
    g_prev_hand_swing = g_hand_swing;
    update_breaking();
    if (g_hand_swing_active) {
        g_hand_swing_ticks++;
        if (g_hand_swing_ticks >= 8) {
            g_hand_swing_ticks = key_down_vk(VK_LBUTTON) ? 0 : 0;
            g_hand_swing_active = key_down_vk(VK_LBUTTON) ? 1 : 0;
        }
        g_hand_swing = g_hand_swing_active ? ((float)g_hand_swing_ticks / 8.0f) : 0.0f;
    } else {
        g_hand_swing_ticks = 0;
        g_hand_swing = 0.0f;
    }

    g_player_prev_x = g_player_x;
    g_player_prev_y = g_player_y;
    g_player_prev_z = g_player_z;
    g_player_prev_yaw = g_player_yaw;
    g_player_prev_pitch = g_player_pitch;
    g_prev_distance_walked = g_distance_walked;
    g_prev_camera_yaw = g_camera_yaw;
    g_prev_camera_pitch = g_camera_pitch;

    /* ho.java movement input mapping:
       forward/back/left/right produce movementInput.moveForward/moveStrafe;
       sneak scales both axes by 0.3. */
    float strafe = 0.0f;
    float forward = 0.0f;
    if (key_down_vk(g_opts.keys[0])) forward += 1.0f;
    if (key_down_vk(g_opts.keys[2])) forward -= 1.0f;
    if (key_down_vk(g_opts.keys[1])) strafe += 1.0f;
    if (key_down_vk(g_opts.keys[3])) strafe -= 1.0f;
    int jumping = key_down_vk(g_opts.keys[4]);
    int sneaking = key_down_vk(g_opts.keys[5]);
    if (sneaking) { strafe *= 0.3f; forward *= 0.3f; }

    if (jumping && g_player_on_ground) {
        /* hp.java::I() jump impulse in this era. */
        g_player_motion_y = 0.42f;
        g_player_on_ground = 0;
    }

    /* mm.java::a(strafe, forward, speed) with grass ground friction.
       Grass slipperiness=0.6 => 0.6*0.91=0.54600006, making ground accel 0.1. */
    float input_len = sqrtf(strafe * strafe + forward * forward);
    if (input_len >= 0.01f) {
        if (input_len < 1.0f) input_len = 1.0f;
        float accel = g_player_on_ground ? 0.1f : 0.02f;
        strafe = strafe * accel / input_len;
        forward = forward * accel / input_len;
        float yaw_rad = g_player_yaw * (float)M_PI / 180.0f;
        float sy = sinf(yaw_rad);
        float cy = cosf(yaw_rad);
        g_player_motion_x += strafe * cy - forward * sy;
        g_player_motion_z += forward * cy + strafe * sy;
    }

    g_player_motion_y -= 0.08f;

    float old_x = g_player_x;
    g_player_x += g_player_motion_x;
    if (flat_player_aabb_collides(g_player_x, g_player_y, g_player_z)) {
        g_player_x = old_x;
        g_player_motion_x = 0.0f;
    }

    float old_z = g_player_z;
    g_player_z += g_player_motion_z;
    if (flat_player_aabb_collides(g_player_x, g_player_y, g_player_z)) {
        g_player_z = old_z;
        g_player_motion_z = 0.0f;
    }

    float old_y = g_player_y;
    g_player_on_ground = 0;
    g_player_y += g_player_motion_y;
    if (flat_player_aabb_collides(g_player_x, g_player_y, g_player_z)) {
        g_player_y = old_y;
        if (g_player_motion_y < 0.0f) g_player_on_ground = 1;
        g_player_motion_y = 0.0f;
    }

    if (!g_player_on_ground && flat_player_aabb_collides(g_player_x, g_player_y - 0.05f, g_player_z)) {
        g_player_on_ground = 1;
    }
    if (g_player_y < -16.0f) {
        reset_flat_player_spawn();
    }

    g_player_motion_y *= 0.98f;

    float friction = g_player_on_ground ? 0.54600006f : 0.91f;
    g_player_motion_x *= friction;
    g_player_motion_z *= friction;

    float dx = g_player_x - g_player_prev_x;
    float dz = g_player_z - g_player_prev_z;
    float horizontal = sqrtf(dx * dx + dz * dz);
    g_distance_walked += horizontal * 0.6f;

    /* eh.java cameraYaw/cameraPitch smoothing used by kq.java view bobbing. */
    float target_yaw = sqrtf(g_player_motion_x * g_player_motion_x + g_player_motion_z * g_player_motion_z);
    float target_pitch = atanf(-g_player_motion_y * 0.2f) * 15.0f;
    if (target_yaw > 0.1f) target_yaw = 0.1f;
    if (!g_player_on_ground) target_yaw = 0.0f;
    if (g_player_on_ground) target_pitch = 0.0f;
    g_camera_yaw += (target_yaw - g_camera_yaw) * 0.4f;
    g_camera_pitch += (target_pitch - g_camera_pitch) * 0.8f;
}
