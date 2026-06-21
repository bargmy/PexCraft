/* Split from original monolithic main.c. Included by src/main.c unity build. */

static void hud_add_chat(const char *msg) {
    if (!msg || !msg[0]) return;
    memmove(&g_chat_lines[1], &g_chat_lines[0], sizeof(ChatLine) * (MAX_CHAT_LINES - 1));
    snprintf(g_chat_lines[0].text, sizeof(g_chat_lines[0].text), "%s", msg);
    g_chat_lines[0].age = 0;
    if (g_chat_count < MAX_CHAT_LINES) g_chat_count++;
}

static int death_reason_code_from_text(const char *reason) {
    if (!reason) return PEX_DEATH_GENERIC;
    if (strstr(reason, "lava")) return PEX_DEATH_LAVA;
    if (strstr(reason, "suffocated")) return PEX_DEATH_SUFFOCATION;
    if (strstr(reason, "out of the world")) return PEX_DEATH_VOID;
    if (strstr(reason, "fell") || strstr(reason, "ground")) return PEX_DEATH_FALL;
    return PEX_DEATH_GENERIC;
}

static const char *death_message_from_reason(const char *reason) {
    int code = death_reason_code_from_text(reason);
    switch (code) {
        case PEX_DEATH_FALL: return "Player hit the ground too hard";
        case PEX_DEATH_LAVA: return "Player tried to swim in lava";
        case PEX_DEATH_SUFFOCATION: return "Player suffocated in a wall";
        case PEX_DEATH_VOID: return "Player fell out of the world";
        default:
            if (reason && strstr(reason, "slain")) return "Player was slain";
            return "Player died";
    }
}

static void player_die(const char *reason) {
    if (g_player_dead) return;
    g_player_dead = 1;
    g_player_death_time = 0;
    g_player_health = 0;
    g_player_hurt_time = g_player_max_hurt_time;
    g_player_motion_x = g_player_motion_z = 0.0f;
    g_player_motion_y = 0.10f; /* EntityPlayer.onDeath sets small upward motion. */
    inventory_drop_all_items_on_death();
    if (g_mp_connected) {
        pex_net_send_player_state();
        pex_net_send_player_action(PEX_ACTION_DIED, 0, 0, 0, 0, death_reason_code_from_text(reason));
    }

    char msg[160];
    snprintf(msg, sizeof(msg), "%s", death_message_from_reason(reason));
    if (!g_mp_connected) hud_add_chat(msg);
    set_screen(SCREEN_DEATH);
    if (!g_mp_connected) save_current_world_state();
}

static void player_take_damage(int amount, const char *reason) {
    if (amount <= 0 || g_player_dead) return;
    g_player_prev_health = g_player_health;
    g_hearts_life = g_ingame_ticks + 20;
    g_player_hurt_time = g_player_max_hurt_time;
    g_player_attacked_at_yaw = 0.0f;
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


static int equipped_stack_matches_current(const ItemStack *cur) {
    if (stack_empty(cur) && stack_empty(&g_equipped_item)) return 1;
    if (g_equipped_slot != g_selected_hotbar_slot) return 0;
    return stack_same_item(cur, &g_equipped_item);
}

static void update_equipped_item(void) {
    ItemStack *cur = &g_inventory[g_selected_hotbar_slot];
    g_prev_equipped_progress = g_equipped_progress;
    float target = equipped_stack_matches_current(cur) ? 1.0f : 0.0f;
    float delta = target - g_equipped_progress;
    if (delta < -0.4f) delta = -0.4f;
    if (delta > 0.4f) delta = 0.4f;
    g_equipped_progress += delta;
    if (g_equipped_progress < 0.1f) {
        g_equipped_item = *cur;
        g_equipped_slot = g_selected_hotbar_slot;
    }
}

static void player_respawn(void) {
    g_player_dead = 0;
    g_player_death_time = 0;
    g_player_health = 20;
    g_player_prev_health = 20;
    g_player_armor = 0;
    g_hearts_life = 0;
    g_player_hurt_time = 0;
    g_player_attacked_at_yaw = 0.0f;
    reset_flat_player_spawn();
    g_chat_input[0] = 0;
    hud_add_chat("Respawned.");
    g_save_dirty = 1;
    if (g_mp_connected) {
        g_mp_pending_respawn_sync = 1;
        pex_net_send_player_state();
        pex_net_request_chunks_around_player(1);
    }
    set_screen(SCREEN_INGAME);
}

static int flat_player_try_step_move(float nx, float base_y, float nz) {
    /* Entity stepHeight behavior for stairs/half-height obstacles: keep the
       walking speed, just resolve the horizontal collision at +0.5 block. */
    if (!g_player_on_ground) return 0;
    if (g_player_y > base_y + 0.01f) return 0;
    const float step = 0.5f;
    if (flat_player_aabb_collides(nx, base_y + step, nz)) return 0;
    g_player_x = nx;
    g_player_y = base_y + step;
    g_player_z = nz;
    if (g_player_motion_y < 0.0f) g_player_motion_y = 0.0f;
    return 1;
}


static void ingame_tick(void) {
    double prof_ingame_start = pex_profile_begin();
    double prof_part = 0.0;
    int input_active = (g_screen == SCREEN_INGAME && !g_player_dead);

    g_ingame_ticks++;
    if (g_save_message_ticks > 0) g_save_message_ticks--;
    for (int i = 0; i < g_chat_count; i++) g_chat_lines[i].age++;
    prof_part = pex_profile_begin();
    inventory_tick();
    update_equipped_item();
    pex_profile_add(PROF_INVENTORY, prof_part);
    prof_part = pex_profile_begin();
    furnace_tick_all();
    pex_profile_add(PROF_FURNACE, prof_part);
    if (g_player_hurt_time > 0) g_player_hurt_time--;
    prof_part = pex_profile_begin();
    update_dropped_items();
    pex_profile_add(PROF_DROPS, prof_part);
    prof_part = pex_profile_begin();
    update_dig_particles();
    pex_profile_add(PROF_PARTICLES, prof_part);
    if (!g_mp_connected) {
        prof_part = pex_profile_begin();
        update_falling_blocks();
        pex_profile_add(PROF_FALLING, prof_part);
        /* Android/GLES and PSP read terrain/render arrays on the render thread.  Keep
           streaming/remap/light commits on this game thread there; the generic
           background service can race the renderer and corrupt the visible world
           while the active window slides. */
#if defined(PEX_PLATFORM_ANDROID) || defined(PEX_PLATFORM_ANDROID_TV) || defined(PEX_PLATFORM_PSP)
        update_infinite_world_streaming();
        flat_flush_pending_lighting();
#else
        world_stream_service_ensure();
#endif
        prof_part = pex_profile_begin();
        update_liquids();
        pex_profile_add(PROF_LIQUIDS, prof_part);
    }
    prof_part = pex_profile_begin();
    update_buttons_and_pressure_plates();
    pex_profile_add(PROF_BUTTONS, prof_part);
    double prof_player_logic_start = pex_profile_begin();

    if (g_player_dead) {
        g_player_death_time++;
        if (g_loaded_world_dir[0] && g_save_dirty &&
            (g_ingame_ticks - g_last_autosave_tick) >= AUTOSAVE_INTERVAL_TICKS) {
            if (!g_mp_connected) save_current_world_state();
        }
        if (g_mp_connected) pex_net_send_player_state();
        pex_profile_add(PROF_PLAYER_LOGIC, prof_player_logic_start);
        pex_profile_add(PROF_INGAME_TOTAL, prof_ingame_start);
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
    if (in_water && !g_player_was_in_water) {
        spawn_water_entry_particles(g_player_x, g_player_y - 1.0f, g_player_z, g_player_motion_x, g_player_motion_z);
    }
    g_player_was_in_water = in_water;
    int in_ladder = flat_player_in_ladder();
    if (in_water || in_lava || in_ladder) {
        g_player_fall_distance = 0.0f;
        if (in_water || in_lava) apply_player_fluid_velocity(in_water ? 1 : 0);
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

    float raw_strafe = strafe;
    float raw_forward = forward;
    float water_exit_dx = 0.0f;
    float water_exit_dz = 0.0f;
    if (fabsf(raw_strafe) + fabsf(raw_forward) > 0.001f) {
        float yaw_rad = g_player_yaw * (float)M_PI / 180.0f;
        float sy = sinf(yaw_rad);
        float cy = cosf(yaw_rad);
        water_exit_dx = raw_strafe * cy - raw_forward * sy;
        water_exit_dz = raw_forward * cy + raw_strafe * sy;
    }
    int trying_water_exit = jumping && in_water &&
        flat_player_has_water_exit_ledge(g_player_x, g_player_y, g_player_z, water_exit_dx, water_exit_dz);

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

    if (jumping && in_ladder) {
        g_player_motion_y = 0.20f;
        g_player_on_ground = 0;
    } else if (jumping && in_water) {
        if (trying_water_exit) {
            /* Strong lift is only for shore/ledge exit attempts. */
            g_player_motion_y += 0.08f;
            if (g_player_motion_y < 0.26f) g_player_motion_y = 0.26f;
            if (g_player_motion_y > 0.34f) g_player_motion_y = 0.34f;
        } else {
            /* Open water keeps normal slow swimming.  This prevents the old
               "Jesus mode" where holding jump at the surface let the player
               climb upward forever. */
            g_player_motion_y += 0.035f;
            if (g_player_motion_y < 0.08f) g_player_motion_y = 0.08f;
            if (g_player_motion_y > 0.16f) g_player_motion_y = 0.16f;
        }
    } else if (jumping && in_lava) {
        /* Lava remains heavy/slow. */
        g_player_motion_y += 0.04f;
    } else if (jumping && g_player_on_ground) {
        g_player_motion_y = 0.50f;
        g_player_on_ground = 0;
    }

    float input_len = sqrtf(strafe * strafe + forward * forward);
    if (input_len >= 0.01f) {
        if (input_len < 1.0f) input_len = 1.0f;
        float accel = in_water ? 0.02f : (in_lava ? 0.02f : (g_player_on_ground ? 0.1f : 0.02f));
        strafe = strafe * accel / input_len;
        forward = forward * accel / input_len;
        float yaw_rad = g_player_yaw * (float)M_PI / 180.0f;
        float sy = sinf(yaw_rad);
        float cy = cosf(yaw_rad);
        g_player_motion_x += strafe * cy - forward * sy;
        g_player_motion_z += forward * cy + strafe * sy;
    }

    float previous_motion_y = g_player_motion_y;
    if (in_ladder) {
        if (g_player_motion_y < -0.15f) g_player_motion_y = -0.15f;
    } else {
        g_player_motion_y -= (in_water ? 0.010f : (in_lava ? 0.02f : 0.08f));
    }

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

    int liquid_horizontal_collision = 0;
    float step_base_y = g_player_y;

    float old_x = g_player_x;
    g_player_x += g_player_motion_x;
    if (flat_player_aabb_collides(g_player_x, g_player_y, g_player_z)) {
        float try_x = g_player_x;
        g_player_x = old_x;
        if (!flat_player_try_step_move(try_x, step_base_y, g_player_z)) {
            g_player_motion_x = 0.0f;
            liquid_horizontal_collision = 1;
        }
    }

    float old_z = g_player_z;
    g_player_z += g_player_motion_z;
    if (flat_player_aabb_collides(g_player_x, g_player_y, g_player_z)) {
        float try_z = g_player_z;
        g_player_z = old_z;
        if (!flat_player_try_step_move(g_player_x, step_base_y, try_z)) {
            g_player_motion_z = 0.0f;
            liquid_horizontal_collision = 1;
        }
    }

    if (in_ladder && (liquid_horizontal_collision || (input_active && raw_forward > 0.01f))) {
        if (g_player_motion_y < 0.20f) g_player_motion_y = 0.20f;
        g_player_on_ground = 0;
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

    if (jumping && in_water && trying_water_exit) {
        /* Shore mantle: only run when the player is pressing into a detected
           ledge.  Open water gets no direct Y teleport/lift, so it cannot fly. */
        if (!flat_player_aabb_collides(g_player_x, g_player_y + 0.06f, g_player_z)) {
            g_player_y += 0.06f;
            if (g_player_motion_y < 0.18f) g_player_motion_y = 0.18f;
        }

        if (liquid_horizontal_collision ||
            flat_player_has_water_exit_ledge(g_player_x, g_player_y, g_player_z, water_exit_dx, water_exit_dz)) {
            int climbed = 0;
            for (int i = 1; i <= 10; i++) {
                float step_up = (float)i * 0.08f;
                if (!flat_player_aabb_collides(g_player_x, g_player_y + step_up, g_player_z)) {
                    g_player_y += step_up;
                    g_player_motion_y = 0.20f;
                    climbed = 1;
                    break;
                }
            }
            if (!climbed && !flat_player_aabb_collides(g_player_x, g_player_y + 0.55f, g_player_z) &&
                g_player_motion_y < 0.24f) {
                g_player_motion_y = 0.24f;
            }
        }
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

    if (in_ladder) {
        g_player_motion_x *= 0.50f;
        g_player_motion_z *= 0.50f;
        if (g_player_motion_y < -0.15f) g_player_motion_y = -0.15f;
        g_player_motion_y *= 0.50f;
    } else if (in_water) {
        g_player_motion_x *= 0.80f;
        g_player_motion_y *= 0.80f;
        g_player_motion_z *= 0.80f;
        if (trying_water_exit && g_player_motion_y < 0.18f) {
            g_player_motion_y = 0.18f;
        }
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

    if (g_loaded_world_dir[0] && g_save_dirty &&
        (g_ingame_ticks - g_last_autosave_tick) >= AUTOSAVE_INTERVAL_TICKS) {
        if (!g_mp_connected) save_current_world_state();
    }
    if (g_mp_connected) pex_net_send_player_state();
    pex_profile_add(PROF_PLAYER_LOGIC, prof_player_logic_start);
    pex_profile_add(PROF_INGAME_TOTAL, prof_ingame_start);
}

