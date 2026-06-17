/* Split from original monolithic main.c. Included by src/main.c unity build. */

static void hud_add_chat(const char *msg) {
    if (!msg || !msg[0]) return;
    memmove(&g_chat_lines[1], &g_chat_lines[0], sizeof(ChatLine) * (MAX_CHAT_LINES - 1));
    snprintf(g_chat_lines[0].text, sizeof(g_chat_lines[0].text), "%s", msg);
    g_chat_lines[0].age = 0;
    if (g_chat_count < MAX_CHAT_LINES) g_chat_count++;
}


static void player_die(const char *reason) {
    if (g_player_dead) return;
    g_player_dead = 1;
    g_player_death_time = 0;
    g_player_health = 0;
    g_player_motion_x = g_player_motion_z = 0.0f;
    g_player_motion_y = 0.10f; /* EntityPlayer.onDeath sets small upward motion. */
    inventory_drop_all_items_on_death();

    char msg[160];
    snprintf(msg, sizeof(msg), "Player died%s%s", reason && reason[0] ? ": " : "", reason && reason[0] ? reason : "");
    hud_add_chat(msg);
    set_screen(SCREEN_DEATH);
    save_current_world_state();
}

static void player_take_damage(int amount, const char *reason) {
    if (amount <= 0 || g_player_dead) return;
    g_player_health -= amount;
    if (g_player_health <= 0) {
        player_die(reason);
    } else {
        char msg[96];
        snprintf(msg, sizeof(msg), "Ouch! -%d", amount);
        hud_add_chat(msg);
        g_save_dirty = 1;
    }
}

static void player_respawn(void) {
    g_player_dead = 0;
    g_player_death_time = 0;
    g_player_health = 20;
    g_player_prev_health = 20;
    g_player_armor = 0;
    reset_flat_player_spawn();
    g_chat_input[0] = 0;
    hud_add_chat("Respawned.");
    g_save_dirty = 1;
    set_screen(SCREEN_INGAME);
}


static void ingame_tick(void) {
    int input_active = (g_screen == SCREEN_INGAME && !g_player_dead);

    g_ingame_ticks++;
    for (int i = 0; i < g_chat_count; i++) g_chat_lines[i].age++;
    inventory_tick();
    update_dropped_items();
    update_dig_particles();
    update_falling_blocks();
    update_infinite_world_streaming();
    update_liquids();

    if (g_player_dead) {
        g_player_death_time++;
        if (g_loaded_world_dir[0] && (g_save_dirty || (g_ingame_ticks % 200) == 0) && (g_ingame_ticks % 40) == 0) {
            save_current_world_state();
        }
        return;
    }

    g_break_swing_holding = 0;
    g_prev_hand_swing = g_hand_swing;
    if (input_active) {
        update_breaking();
    } else {
        reset_breaking_state();
        g_block_hit_delay = 0;
    }

    if (g_break_swing_holding && !g_hand_swing_active) {
        g_hand_swing_active = 1;
        g_hand_swing_progress = 0.0f;
        g_hand_swing_ticks = 0;
        g_hand_swing = 0.0f;
    }

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
            /* While mining: loop forever and skip the last 10% of the swing.
               The important part is that progress wraps at 7.2 ticks, never
               reaching 0.90..1.00. */
            const float loop_end = 8.0f * 0.90f;
            g_hand_swing_progress += 1.35f;
            if (g_hand_swing_progress >= loop_end) {
                g_hand_swing_progress = fmodf(g_hand_swing_progress, loop_end);
                g_prev_hand_swing = 0.0f;
            }
            g_hand_swing_ticks = (int)floorf(g_hand_swing_progress);
            g_hand_swing = g_hand_swing_progress / 8.0f;
            if (g_hand_swing >= 0.90f) g_hand_swing = 0.0f;
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

    int in_water = flat_player_in_water();
    int in_lava = flat_player_in_lava();
    if (in_water || in_lava) {
        g_player_fall_distance = 0.0f;
        if (in_lava && (g_ingame_ticks % 20) == 0) player_take_damage(4, "tried to swim in lava");
    }

    float strafe = 0.0f;
    float forward = 0.0f;
    int jumping = 0;
    int sneaking = 0;
    if (input_active) {
        if (key_down_vk(g_opts.keys[0])) forward += 1.0f;
        if (key_down_vk(g_opts.keys[2])) forward -= 1.0f;
        if (key_down_vk(g_opts.keys[1])) strafe += 1.0f;
        if (key_down_vk(g_opts.keys[3])) strafe -= 1.0f;
        jumping = key_down_vk(g_opts.keys[4]);
        sneaking = key_down_vk(g_opts.keys[5]);
        if (sneaking) { strafe *= 0.3f; forward *= 0.3f; }
    }


    int suff_block = flat_player_suffocation_block();
    if (suff_block) {
        g_suffocation_damage_timer++;
        if (g_suffocation_damage_timer >= 20) {
            g_suffocation_damage_timer = 0;
            player_take_damage(1, "suffocated in a wall");
        }
    } else {
        g_suffocation_damage_timer = 0;
    }

    if (jumping && (in_water || in_lava)) {
        /* Stronger than the previous value so you can actually climb out at shore edges. */
        g_player_motion_y += in_water ? 0.10f : 0.04f;
    } else if (jumping && g_player_on_ground) {
        g_player_motion_y = 0.50f;
        g_player_on_ground = 0;
    }

    float input_len = sqrtf(strafe * strafe + forward * forward);
    if (input_len >= 0.01f) {
        if (input_len < 1.0f) input_len = 1.0f;
        float accel = in_water ? 0.04f : (in_lava ? 0.02f : (g_player_on_ground ? 0.1f : 0.02f));
        strafe = strafe * accel / input_len;
        forward = forward * accel / input_len;
        float yaw_rad = g_player_yaw * (float)M_PI / 180.0f;
        float sy = sinf(yaw_rad);
        float cy = cosf(yaw_rad);
        g_player_motion_x += strafe * cy - forward * sy;
        g_player_motion_z += forward * cy + strafe * sy;
    }

    float previous_motion_y = g_player_motion_y;
    g_player_motion_y -= (in_water ? 0.005f : (in_lava ? 0.01f : 0.08f));

    if (input_active && sneaking && flat_player_has_sneak_support(g_player_x, g_player_y, g_player_z)) {
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
    int was_on_ground = g_player_on_ground;
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

    /* Deobf EntityLiving.fall: damage is ceil(fallDistance - 3). */
    if (g_player_on_ground) {
        if (!was_on_ground && g_player_fall_distance > 3.0f) {
            int dmg = (int)ceilf(g_player_fall_distance - 3.0f);
            if (dmg > 0) player_take_damage(dmg, "fell from a high place");
        }
        g_player_fall_distance = 0.0f;
    } else if (previous_motion_y < 0.0f) {
        g_player_fall_distance += -previous_motion_y;
    }

    if (g_player_y < -16.0f) {
        player_die("fell out of the world");
    }

    if (in_water) {
        g_player_motion_x *= 0.86f;
        g_player_motion_y *= 0.86f;
        g_player_motion_z *= 0.86f;
    } else if (in_lava) {
        g_player_motion_x *= 0.50f;
        g_player_motion_y *= 0.50f;
        g_player_motion_z *= 0.50f;
    } else {
        g_player_motion_y *= 0.98f;
        float friction = g_player_on_ground ? 0.54600006f : 0.91f;
        g_player_motion_x *= friction;
        g_player_motion_z *= friction;
    }

    float dx = g_player_x - g_player_prev_x;
    float dz = g_player_z - g_player_prev_z;
    float horizontal = sqrtf(dx * dx + dz * dz);
    g_distance_walked += horizontal * 0.6f;

    float target_limb = horizontal * 4.0f;
    if (target_limb > 1.0f) target_limb = 1.0f;
    if (!g_player_on_ground) target_limb = 0.0f;
    g_limb_swing_amount += (target_limb - g_limb_swing_amount) * 0.4f;
    g_limb_swing += g_limb_swing_amount;

    float target_yaw = sqrtf(g_player_motion_x * g_player_motion_x + g_player_motion_z * g_player_motion_z);
    float target_pitch = atanf(-g_player_motion_y * 0.2f) * 15.0f;
    if (target_yaw > 0.1f) target_yaw = 0.1f;
    if (!g_player_on_ground) target_yaw = 0.0f;
    if (g_player_on_ground) target_pitch = 0.0f;
    g_camera_yaw += (target_yaw - g_camera_yaw) * 0.4f;
    g_camera_pitch += (target_pitch - g_camera_pitch) * 0.8f;

    if (g_loaded_world_dir[0] && (g_save_dirty || (g_ingame_ticks % 200) == 0) && (g_ingame_ticks % 40) == 0) {
        save_current_world_state();
    }
}

