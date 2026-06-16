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
    g_break_swing_holding = 0;
    g_prev_hand_swing = g_hand_swing;
    update_breaking();

    if (g_air_swing_playing && !g_hand_swing_active) {
        /* One air swing only. This advances for 8 ticks, then stops even if
           the left mouse button is still held. */
        g_air_swing_ticks++;
        if (g_air_swing_ticks <= 8) {
            g_hand_swing_active = 1;
            g_hand_swing_progress = (float)g_air_swing_ticks;
            g_hand_swing_ticks = g_air_swing_ticks;
            g_hand_swing = (float)g_air_swing_ticks / 8.0f;
        } else {
            g_air_swing_playing = 0;
            g_air_swing_ticks = 0;
            g_hand_swing_progress = 0.0f;
            g_hand_swing_ticks = 0;
            g_hand_swing = 0.0f;
            g_prev_hand_swing = 0.0f;
        }
    } else if (g_hand_swing_active) {
        g_air_swing_playing = 0;
        g_air_swing_ticks = 0;

        if (g_break_swing_holding) {
            /* Breaking-only: 25% faster, and while looping skip the final 10%
               of the cycle by looping at 0.90. */
            g_hand_swing_progress += 1.25f;
            const float loop_end = 8.0f * 0.90f;
            if (g_hand_swing_progress >= loop_end) {
                while (g_hand_swing_progress >= loop_end) g_hand_swing_progress -= loop_end;
                g_prev_hand_swing = g_hand_swing_progress / 8.0f;
            }
            g_hand_swing_ticks = (int)floorf(g_hand_swing_progress);
            g_hand_swing = g_hand_swing_progress / 8.0f;
        } else {
            /* Release/place/air: finish a full normal cycle, then stop cleanly. */
            g_hand_swing_progress += 1.0f;
            g_hand_swing_ticks = (int)floorf(g_hand_swing_progress);
            if (g_hand_swing_ticks <= 8) {
                g_hand_swing = g_hand_swing_progress / 8.0f;
                if (g_hand_swing > 1.0f) g_hand_swing = 1.0f;
            } else {
                g_hand_swing_progress = 0.0f;
                g_hand_swing_ticks = 0;
                g_hand_swing_active = 0;
                g_hand_swing = 0.0f;
                g_prev_hand_swing = 0.0f;
            }
        }
    } else {
        g_hand_swing_progress = 0.0f;
        g_hand_swing_ticks = 0;
        g_hand_swing = 0.0f;
    }

    g_player_prev_x = g_player_x;
    g_player_prev_y = g_player_y;
    g_player_prev_z = g_player_z;
    g_player_prev_yaw = g_player_yaw;
    g_player_prev_pitch = g_player_pitch;
    g_prev_distance_walked = g_distance_walked;
    g_prev_limb_swing = g_limb_swing;
    g_prev_limb_swing_amount = g_limb_swing_amount;
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
        g_player_motion_y = 0.50f; /* requested jump height */
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

    if (sneaking && flat_player_has_sneak_support(g_player_x, g_player_y, g_player_z)) {
        /* B1.0.0 source behavior (mm.java:249-273): while sneaking, reduce
           attempted horizontal movement in 0.05 steps until the bounding box
           still has floor one block below. This acts like a wall at the actual
           block edge and also catches jump-then-sneak while still close above
           the ground. */
        const float step = 0.05f;
        while (fabsf(g_player_motion_x) > 0.00001f &&
               !flat_player_has_sneak_support(g_player_x + g_player_motion_x, g_player_y, g_player_z)) {
            if (fabsf(g_player_motion_x) <= step) g_player_motion_x = 0.0f;
            else g_player_motion_x += (g_player_motion_x > 0.0f) ? -step : step;
        }
        while (fabsf(g_player_motion_z) > 0.00001f &&
               !flat_player_has_sneak_support(g_player_x, g_player_y, g_player_z + g_player_motion_z)) {
            if (fabsf(g_player_motion_z) <= step) g_player_motion_z = 0.0f;
            else g_player_motion_z += (g_player_motion_z > 0.0f) ? -step : step;
        }
    }

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

    /* Source-style smoother limb animation:
       EntityLiving/ModelBiped receives limb swing and limb amount as smoothed
       values, not raw one-frame position delta.  The earlier F5 model used the
       raw delta, which made walking animation jitter/chop. */
    float target_limb = horizontal * 4.0f;
    if (target_limb > 1.0f) target_limb = 1.0f;
    if (!g_player_on_ground) target_limb = 0.0f;
    g_limb_swing_amount += (target_limb - g_limb_swing_amount) * 0.4f;
    g_limb_swing += g_limb_swing_amount;

    /* eh.java cameraYaw/cameraPitch smoothing used by kq.java view bobbing. */
    float target_yaw = sqrtf(g_player_motion_x * g_player_motion_x + g_player_motion_z * g_player_motion_z);
    float target_pitch = atanf(-g_player_motion_y * 0.2f) * 15.0f;
    if (target_yaw > 0.1f) target_yaw = 0.1f;
    if (!g_player_on_ground) target_yaw = 0.0f;
    if (g_player_on_ground) target_pitch = 0.0f;
    g_camera_yaw += (target_yaw - g_camera_yaw) * 0.4f;
    g_camera_pitch += (target_pitch - g_camera_pitch) * 0.8f;
}

