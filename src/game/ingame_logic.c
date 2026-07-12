/* Split from original monolithic main.c. Included by src/main.c unity build. */

static void hud_push_chat_line(const char *text) {
    if (!text || !text[0]) return;
    memmove(&g_chat_lines[1], &g_chat_lines[0], sizeof(ChatLine) * (MAX_CHAT_LINES - 1));
    snprintf(g_chat_lines[0].text, sizeof(g_chat_lines[0].text), "%s", text);
    g_chat_lines[0].age = 0;
    if (g_chat_count < MAX_CHAT_LINES) g_chat_count++;
}

static int hud_chat_format_token_len(const unsigned char *p) {
    if (!p || !p[0]) return 0;
    if (p[0] == 0xC2 && p[1] == 0xA7 && p[2]) return 3;
    if (p[0] == 0xA7 && p[1]) return 2;
    return 0;
}

static int hud_chat_utf8_len(const unsigned char *p) {
    if (!p || !p[0]) return 0;
    if (p[0] < 0x80) return 1;
    if ((p[0] & 0xE0) == 0xC0 && p[1]) return 2;
    if ((p[0] & 0xF0) == 0xE0 && p[1] && p[2]) return 3;
    if ((p[0] & 0xF8) == 0xF0 && p[1] && p[2] && p[3]) return 4;
    return 1;
}

static int hud_chat_visible_width(const char *text) {
    char plain[512];
    size_t n = 0;
    const unsigned char *p = (const unsigned char *)text;
    while (*p && n + 1 < sizeof(plain)) {
        int fmt = hud_chat_format_token_len(p);
        if (fmt) { p += fmt; continue; }
        int bytes = hud_chat_utf8_len(p);
        if (bytes <= 0) break;
        if (n + (size_t)bytes >= sizeof(plain)) break;
        memcpy(plain + n, p, (size_t)bytes);
        n += (size_t)bytes;
        p += bytes;
    }
    plain[n] = 0;
    return text_width_chat_plain(plain);
}

typedef struct HudChatFormatState {
    char color;
    unsigned char obfuscated;
    unsigned char bold;
    unsigned char strikethrough;
    unsigned char underlined;
    unsigned char italic;
} HudChatFormatState;

static void hud_chat_format_state_apply(HudChatFormatState *state, char code) {
    if (!state) return;
    if (code >= 'A' && code <= 'Z') code = (char)(code - 'A' + 'a');
    if ((code >= '0' && code <= '9') || (code >= 'a' && code <= 'f')) {
        memset(state, 0, sizeof(*state));
        state->color = code;
    } else if (code == 'k') state->obfuscated = 1;
    else if (code == 'l') state->bold = 1;
    else if (code == 'm') state->strikethrough = 1;
    else if (code == 'n') state->underlined = 1;
    else if (code == 'o') state->italic = 1;
    else if (code == 'r') memset(state, 0, sizeof(*state));
}

static void hud_chat_append_format_code(char *line, size_t cap, size_t *line_len, char code) {
    if (!line || !line_len || *line_len + 3 >= cap) return;
    line[(*line_len)++] = (char)0xC2;
    line[(*line_len)++] = (char)0xA7;
    line[(*line_len)++] = code;
    line[*line_len] = 0;
}

static void hud_chat_begin_wrapped_line(char *line, size_t cap, size_t *line_len,
                                        const HudChatFormatState *state) {
    *line_len = 0;
    line[0] = 0;
    if (!state) return;
    if (state->color) hud_chat_append_format_code(line, cap, line_len, state->color);
    if (state->obfuscated) hud_chat_append_format_code(line, cap, line_len, 'k');
    if (state->bold) hud_chat_append_format_code(line, cap, line_len, 'l');
    if (state->strikethrough) hud_chat_append_format_code(line, cap, line_len, 'm');
    if (state->underlined) hud_chat_append_format_code(line, cap, line_len, 'n');
    if (state->italic) hud_chat_append_format_code(line, cap, line_len, 'o');
}

static void hud_chat_emit_wrapped_line(char *line, size_t *line_len) {
    while (*line_len > 0 && (line[*line_len - 1] == ' ' || line[*line_len - 1] == '\t')) {
        line[--(*line_len)] = 0;
    }
    if (hud_chat_visible_width(line) > 0) hud_push_chat_line(line);
}

static void hud_add_chat(const char *msg) {
    if (!msg || !msg[0]) return;
    pex_logf("chat: %s", msg);

    int max_width = g_gui_w - 8;
    if (max_width > 318) max_width = 318;
    if (max_width < 80) max_width = 80;

    char line[512];
    size_t line_len = 0;
    HudChatFormatState format_state;
    memset(&format_state, 0, sizeof(format_state));
    hud_chat_begin_wrapped_line(line, sizeof(line), &line_len, &format_state);

    const unsigned char *p = (const unsigned char *)msg;
    while (*p) {
        if (*p == '\r') { p++; continue; }
        if (*p == '\n') {
            hud_chat_emit_wrapped_line(line, &line_len);
            hud_chat_begin_wrapped_line(line, sizeof(line), &line_len, &format_state);
            p++;
            continue;
        }

        int fmt = hud_chat_format_token_len(p);
        if (fmt) {
            char code = (char)p[fmt - 1];
            if (line_len + (size_t)fmt < sizeof(line)) {
                memcpy(line + line_len, p, (size_t)fmt);
                line_len += (size_t)fmt;
                line[line_len] = 0;
            }
            hud_chat_format_state_apply(&format_state, code);
            p += fmt;
            continue;
        }

        if (*p == ' ' || *p == '\t') {
            if (hud_chat_visible_width(line) > 0 && line_len + 1 < sizeof(line)) {
                line[line_len++] = ' ';
                line[line_len] = 0;
            }
            p++;
            continue;
        }

        /* Consume a word. Formatting tokens intentionally terminate a word so
           their zero-width state can be handled by the outer loop. */
        const unsigned char *word = p;
        size_t word_len = 0;
        while (word[word_len] && word[word_len] != ' ' && word[word_len] != '\t' &&
               word[word_len] != '\r' && word[word_len] != '\n' &&
               !hud_chat_format_token_len(word + word_len)) {
            int bytes = hud_chat_utf8_len(word + word_len);
            if (bytes <= 0) break;
            word_len += (size_t)bytes;
        }

        char trial[768];
        if (line_len + word_len + 1 < sizeof(trial)) {
            memcpy(trial, line, line_len);
            memcpy(trial + line_len, word, word_len);
            trial[line_len + word_len] = 0;
        } else {
            trial[0] = 0;
        }

        if (trial[0] && hud_chat_visible_width(trial) <= max_width && line_len + word_len < sizeof(line)) {
            memcpy(line + line_len, word, word_len);
            line_len += word_len;
            line[line_len] = 0;
            p += word_len;
            continue;
        }

        if (hud_chat_visible_width(line) > 0) {
            hud_chat_emit_wrapped_line(line, &line_len);
            hud_chat_begin_wrapped_line(line, sizeof(line), &line_len, &format_state);
        }

        /* A single unbroken word can still exceed the HUD. Split it only at
           UTF-8 codepoint boundaries. */
        size_t used = 0;
        while (used < word_len) {
            int bytes = hud_chat_utf8_len(word + used);
            if (bytes <= 0 || used + (size_t)bytes > word_len) bytes = 1;
            if (line_len + (size_t)bytes >= sizeof(line)) {
                hud_chat_emit_wrapped_line(line, &line_len);
                hud_chat_begin_wrapped_line(line, sizeof(line), &line_len, &format_state);
            }
            memcpy(line + line_len, word + used, (size_t)bytes);
            line_len += (size_t)bytes;
            line[line_len] = 0;
            if (hud_chat_visible_width(line) > max_width) {
                line_len -= (size_t)bytes;
                line[line_len] = 0;
                hud_chat_emit_wrapped_line(line, &line_len);
                hud_chat_begin_wrapped_line(line, sizeof(line), &line_len, &format_state);
                memcpy(line + line_len, word + used, (size_t)bytes);
                line_len += (size_t)bytes;
                line[line_len] = 0;
            }
            used += (size_t)bytes;
        }
        p += word_len;
    }

    hud_chat_emit_wrapped_line(line, &line_len);
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

static const char *pex_damage_type_name(PexDamageType type) {
    switch (type) {
        case PEX_DAMAGE_IN_FIRE: return "inFire";
        case PEX_DAMAGE_ON_FIRE: return "onFire";
        case PEX_DAMAGE_LAVA: return "lava";
        case PEX_DAMAGE_IN_WALL: return "inWall";
        case PEX_DAMAGE_DROWN: return "drown";
        case PEX_DAMAGE_STARVE: return "starve";
        case PEX_DAMAGE_CACTUS: return "cactus";
        case PEX_DAMAGE_FALL: return "fall";
        case PEX_DAMAGE_OUT_OF_WORLD: return "outOfWorld";
        case PEX_DAMAGE_EXPLOSION: return "explosion";
        case PEX_DAMAGE_MAGIC: return "magic";
        case PEX_DAMAGE_MOB: return "mob";
        case PEX_DAMAGE_PLAYER: return "player";
        case PEX_DAMAGE_ARROW: return "arrow";
        case PEX_DAMAGE_FIREBALL: return "fireball";
        case PEX_DAMAGE_THROWN: return "thrown";
        case PEX_DAMAGE_INDIRECT_MAGIC: return "indirectMagic";
        case PEX_DAMAGE_GENERIC:
        default: return "generic";
    }
}

static PexDamageSource pex_damage_source_simple(PexDamageType type) {
    PexDamageSource s;
    memset(&s, 0, sizeof(s));
    s.type = type;
    s.damage_type = pex_damage_type_name(type);
    s.hunger_damage = 0.3f;
    switch (type) {
        case PEX_DAMAGE_ON_FIRE:
        case PEX_DAMAGE_IN_WALL:
        case PEX_DAMAGE_DROWN:
        case PEX_DAMAGE_STARVE:
        case PEX_DAMAGE_FALL:
        case PEX_DAMAGE_OUT_OF_WORLD:
        case PEX_DAMAGE_GENERIC:
        case PEX_DAMAGE_MAGIC:
        case PEX_DAMAGE_INDIRECT_MAGIC:
            s.unblockable = 1;
            s.hunger_damage = 0.0f;
            break;
        default:
            break;
    }
    if (type == PEX_DAMAGE_IN_FIRE || type == PEX_DAMAGE_ON_FIRE || type == PEX_DAMAGE_LAVA || type == PEX_DAMAGE_FIREBALL) s.fire_damage = 1;
    if (type == PEX_DAMAGE_OUT_OF_WORLD) s.creative_allowed = 1;
    if (type == PEX_DAMAGE_ARROW || type == PEX_DAMAGE_FIREBALL || type == PEX_DAMAGE_THROWN) s.projectile = 1;
    return s;
}

static PexDamageSource pex_damage_source_player(void) {
    PexDamageSource s = pex_damage_source_simple(PEX_DAMAGE_PLAYER);
    s.direct_kind = PEX_DAMAGE_ENTITY_PLAYER;
    s.true_kind = PEX_DAMAGE_ENTITY_PLAYER;
    s.direct_x = s.true_x = g_player_x;
    s.direct_y = s.true_y = g_player_y;
    s.direct_z = s.true_z = g_player_z;
    return s;
}

static const char *pex_damage_source_death_reason(const PexDamageSource *source) {
    if (!source) return "died";
    switch (source->type) {
        case PEX_DAMAGE_LAVA: return "tried to swim in lava";
        case PEX_DAMAGE_IN_WALL: return "suffocated in a wall";
        case PEX_DAMAGE_OUT_OF_WORLD: return "fell out of the world";
        case PEX_DAMAGE_FALL: return "fell from a high place";
        case PEX_DAMAGE_STARVE: return "starved to death";
        case PEX_DAMAGE_MAGIC:
        case PEX_DAMAGE_INDIRECT_MAGIC: return "magic";
        case PEX_DAMAGE_ARROW: return "was shot by Skeleton";
        case PEX_DAMAGE_FIREBALL:
        case PEX_DAMAGE_THROWN: return "was hit by projectile";
        case PEX_DAMAGE_EXPLOSION: return "Creeper";
        case PEX_DAMAGE_MOB: return source->true_mob_type ? "mob" : "mob";
        default: return source->damage_type ? source->damage_type : "died";
    }
}

static void player_die(const char *reason) {
    if (g_player_dead) return;
    g_player_dead = 1;
    pex_stats_add_general(PEX_STAT_DEATHS, 1);
    g_player_death_time = 0;
    if (g_player_health > 0) player_health_set_hearts(0);
    else g_player_health = 0;
    g_player_hurt_time = g_player_max_hurt_time;
    g_player_motion_x = g_player_motion_z = 0.0f;
    g_player_motion_y = 0.10f; /* EntityPlayer.onDeath sets small upward motion. */
    g_player_sprinting = 0;
    g_sprinting_ticks_left = 0;
    g_sprint_toggle_timer = 0;
    g_prev_sprint_forward = 0;
    passive_mobs_on_player_death_125();
    if (!g_mp_connected) inventory_drop_death_items();
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

static void player_die_from_source(const PexDamageSource *source) {
    player_die(pex_damage_source_death_reason(source));
}

static int player_apply_potion_damage_reduction(int amount, int *carryover) {
    if (player_has_potion(PEX_POTION_RESISTANCE)) {
        int amp = player_potion_amplifier(PEX_POTION_RESISTANCE);
        int reduce = (amp + 1) * 5;
        if (reduce > 20) reduce = 20;
        int scaled = amount * (25 - reduce) + (carryover ? *carryover : 0);
        amount = scaled / 25;
        if (carryover) *carryover = scaled % 25;
    }
    return amount;
}

static int player_attack_entity_from(PexDamageSource source, int amount) {
    /* The server owns multiplayer health and death.  Local collision, fire,
       projectiles, potion ticks and mob AI may still run for visuals/prediction,
       but none of them may alter health while connected. */
    if (g_mp_connected) return 0;
    if (player_damage_disabled() && !source.creative_allowed) return 0;
    if (amount <= 0 || g_player_dead) return 0;
    if (source.true_kind==PEX_DAMAGE_ENTITY_MOB && source.true_mob_index>=0 && source.true_mob_index<MAX_PASSIVE_MOBS) {
        PassiveMob *attacker=&g_passive_mobs[source.true_mob_index];
        if(attacker->active){g_player_last_hurt_by_mob_entity_id=attacker->entity_id;g_player_last_hurt_by_mob_ticks=100;}
    }
    if (source.fire_damage && player_has_potion(PEX_POTION_FIRE_RESISTANCE)) return 0;

    int incoming = amount;
    int play_hurt = 1;
    if (g_hearts_life > PEX_HEARTS_HALVES_LIFE / 2) {
        if (amount <= g_player_natural_armor_rating) return 0;
        incoming = amount - g_player_natural_armor_rating;
        g_player_natural_armor_rating = amount;
        play_hurt = 0;
    } else {
        g_player_natural_armor_rating = amount;
        g_player_prev_health = player_health_clamp(g_player_health);
        g_hearts_life = PEX_HEARTS_HALVES_LIFE;
    }

    int raw_amount = incoming;
    if (!g_mp_connected && !source.unblockable) incoming = armor_apply_damage_reduction(incoming);
    incoming = player_apply_potion_damage_reduction(incoming, &g_player_damage_remainder);
    if (incoming <= 0 && raw_amount > 0) {
        player_health_damage_hearts();
        g_player_hurt_time = g_player_max_hurt_time;
        if (play_hurt) pex_sound_play("damage.hurtflesh", 1.0f, pex_living_sound_pitch_125());
        g_save_dirty = 1;
        return 1;
    }
    if (play_hurt) g_player_hurt_time = g_player_max_hurt_time;
    g_player_attacked_at_yaw = 0.0f;
    pex_stats_add_general(PEX_STAT_DAMAGE_TAKEN, incoming);
    player_health_set_hearts(g_player_health - incoming);
    if (play_hurt) pex_sound_play("damage.hurtflesh", 1.0f, pex_living_sound_pitch_125());
    player_add_exhaustion(source.hunger_damage);
    pex_logf("player damage amount=%d source=%s health=%d", incoming, source.damage_type ? source.damage_type : "", g_player_health);
    if (g_player_health <= 0) {
        player_die_from_source(&source);
    } else {
        g_save_dirty = 1;
    }
    return 1;
}

static int player_should_heal(void) {
    return !g_player_dead && g_player_health > 0 && g_player_health < 20;
}

static void player_foodstats_update(void) {
    int difficulty = g_opts.difficulty & 3; /* 0 Peaceful, 1 Easy, 2 Normal, 3 Hard. */
    g_player_prev_food_level = player_food_clamp(g_player_food_level);

    if (g_player_food_exhaustion > 4.0f) {
        g_player_food_exhaustion -= 4.0f;
        if (g_player_food_saturation > 0.0f) {
            g_player_food_saturation -= 1.0f;
            if (g_player_food_saturation < 0.0f) g_player_food_saturation = 0.0f;
        } else if (difficulty > 0) {
            g_player_food_level = player_food_clamp(g_player_food_level - 1);
        }
        g_save_dirty = 1;
    }

    if (difficulty == 0 && player_should_heal() && (g_ingame_ticks % 20) == 0) {
        /* EntityPlayer.onLivingUpdate: peaceful heals one half-heart every 20 ticks. */
        player_health_set_hearts(g_player_health + 1);
        g_save_dirty = 1;
    }

    if (g_player_food_level >= 18 && player_should_heal()) {
        g_player_food_timer++;
        if (g_player_food_timer >= 80) {
            player_health_set_hearts(g_player_health + 1);
            g_player_food_timer = 0;
            g_save_dirty = 1;
        }
    } else if (g_player_food_level <= 0) {
        g_player_food_timer++;
        if (g_player_food_timer >= 80) {
            if (g_player_health > 10 || difficulty >= 3 || (g_player_health > 1 && difficulty >= 2)) {
                (void)player_attack_entity_from(pex_damage_source_simple(PEX_DAMAGE_STARVE), 1);
            }
            g_player_food_timer = 0;
            g_save_dirty = 1;
        }
    } else {
        g_player_food_timer = 0;
    }

    player_food_sanitize();
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
    if (g_mp_connected) {
        g_chat_input[0] = 0;
        g_mp_pending_respawn_sync = 1;
        pex_net_send_respawn();
        set_screen(SCREEN_INGAME);
        return;
    }
    g_player_dead = 0;
    g_player_death_time = 0;
    player_health_set_silent(20);
    player_food_reset();
    g_player_air = 300;
    player_xp_reset();
    memset(g_armor_inventory, 0, sizeof(g_armor_inventory));
    g_player_armor = 0;
    g_player_damage_remainder = 0;
    g_hearts_life = 0;
    g_player_hurt_time = 0;
    g_player_attacked_at_yaw = 0.0f;
    reset_flat_player_spawn();
    g_chat_input[0] = 0;
    hud_add_chat("Respawned.");
    g_save_dirty = 1;
    set_screen(SCREEN_INGAME);
}

static int flat_player_try_step_move(float nx, float base_y, float nz) {
    /* Entity stepHeight behavior for stairs/half-height obstacles. Java 1.8.8
       uses 0.6F; retain the old 0.5 block behavior for the native backends. */
    if (!g_player_on_ground) return 0;
    if (g_player_y > base_y + 0.01f) return 0;
    const float step = (g_mp_join_backend == PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE) ? 0.6f : 0.5f;
    if (flat_player_aabb_collides(nx, base_y + step, nz)) return 0;
    g_player_x = nx;
    g_player_y = base_y + step;
    g_player_z = nz;
    if (g_player_motion_y < 0.0f) g_player_motion_y = 0.0f;
    return 1;
}


static int g_java47_jump_ticks = 0;

static float java47_snap_collision_edge(float value) {
    /* The gameplay globals are floats and store eye Y, while vanilla keeps a
       double-precision feet position/AABB.  Reconstructing 2.62f - 1.62f can
       produce 0.99999988, leaving the box microscopically inside the floor.
       Vanilla collision bounds are on a 1/16 grid, so only repair values that
       are already within a tiny float-error distance of such an edge. */
    float snapped = roundf(value * 16.0f) * (1.0f / 16.0f);
    return fabsf(value - snapped) <= 0.00010f ? snapped : value;
}

static FlatAABB java47_player_box(void) {
    float minx = g_player_x - 0.30f, maxx = g_player_x + 0.30f;
    float feet = g_player_y - 1.62f;
    float minz = g_player_z - 0.30f, maxz = g_player_z + 0.30f;

    float sx0 = java47_snap_collision_edge(minx);
    float sx1 = java47_snap_collision_edge(maxx);
    if (sx0 != minx) { minx = sx0; maxx = minx + 0.60f; }
    else if (sx1 != maxx) { maxx = sx1; minx = maxx - 0.60f; }
    float sz0 = java47_snap_collision_edge(minz);
    float sz1 = java47_snap_collision_edge(maxz);
    if (sz0 != minz) { minz = sz0; maxz = minz + 0.60f; }
    else if (sz1 != maxz) { maxz = sz1; minz = maxz - 0.60f; }
    feet = java47_snap_collision_edge(feet);

    FlatAABB box = {minx, feet, minz, maxx, feet + 1.80f, maxz};
    return box;
}

static void java47_player_network_position(double *x, double *feet_y, double *z) {
    FlatAABB box = java47_player_box();
    if (x) *x = ((double)box.minx + (double)box.maxx) * 0.5;
    if (feet_y) *feet_y = (double)box.miny;
    if (z) *z = ((double)box.minz + (double)box.maxz) * 0.5;
}

static FlatAABB java47_aabb_add_coord(FlatAABB box, float dx, float dy, float dz) {
    if (dx < 0.0f) box.minx += dx; else box.maxx += dx;
    if (dy < 0.0f) box.miny += dy; else box.maxy += dy;
    if (dz < 0.0f) box.minz += dz; else box.maxz += dz;
    return box;
}

static int java47_aabb_offset_has_collision(const FlatAABB *box, float dx, float dy, float dz) {
    FlatAABB query = *box;
    aabb_offset(&query, dx, dy, dz);
    FlatAABB boxes[192];
    return flat_get_collision_boxes(&query, boxes, (int)(sizeof(boxes) / sizeof(boxes[0]))) > 0;
}


/* Resolve only tiny float-derived overlaps after the vanilla clipping pass.
   This is deliberately capped at 1/32 block: it fixes reconstruction drift but
   will not teleport a player out of a genuinely enclosing server block. */
static void java47_resolve_small_penetration(FlatAABB *box, float *fix_x, float *fix_y, float *fix_z) {
    const float max_fix = 1.0f / 32.0f;
    const float epsilon = 1.0e-5f;
    float total_x = 0.0f, total_y = 0.0f, total_z = 0.0f;
    if (!box) return;

    for (int pass = 0; pass < 8; ++pass) {
        FlatAABB query = {box->minx - 0.001f, box->miny - 0.001f, box->minz - 0.001f,
                          box->maxx + 0.001f, box->maxy + 0.001f, box->maxz + 0.001f};
        FlatAABB boxes[192];
        int count = flat_get_collision_boxes(&query, boxes, (int)(sizeof(boxes) / sizeof(boxes[0])));
        int moved = 0;
        for (int i = 0; i < count; ++i) {
            const FlatAABB *b = &boxes[i];
            float ox = fminf(box->maxx, b->maxx) - fmaxf(box->minx, b->minx);
            float oy = fminf(box->maxy, b->maxy) - fmaxf(box->miny, b->miny);
            float oz = fminf(box->maxz, b->maxz) - fmaxf(box->minz, b->minz);
            if (ox <= 0.0f || oy <= 0.0f || oz <= 0.0f) continue;

            float candidates[6] = {
                b->minx - box->maxx - epsilon, b->maxx - box->minx + epsilon,
                b->miny - box->maxy - epsilon, b->maxy - box->miny + epsilon,
                b->minz - box->maxz - epsilon, b->maxz - box->minz + epsilon
            };
            int best = -1;
            float best_abs = 999.0f;
            for (int c = 0; c < 6; ++c) {
                float a = fabsf(candidates[c]);
                if (a <= max_fix + epsilon && a < best_abs) { best_abs = a; best = c; }
            }
            if (best < 0) continue;
            float dx = 0.0f, dy = 0.0f, dz = 0.0f;
            if (best < 2) dx = candidates[best];
            else if (best < 4) dy = candidates[best];
            else dz = candidates[best];
            aabb_offset(box, dx, dy, dz);
            total_x += dx; total_y += dy; total_z += dz;
            moved = 1;
            break;
        }
        if (!moved) break;
    }
    if (fix_x) *fix_x = total_x;
    if (fix_y) *fix_y = total_y;
    if (fix_z) *fix_z = total_z;
}

static int java47_player_intersects_block(int block_id) {
    FlatAABB box = java47_player_box();
    int x0=(int)floorf(box.minx),x1=(int)floorf(box.maxx-1.0e-4f);
    int y0=(int)floorf(box.miny),y1=(int)floorf(box.maxy-1.0e-4f);
    int z0=(int)floorf(box.minz),z1=(int)floorf(box.maxz-1.0e-4f);
    for(int y=y0;y<=y1;y++)for(int z=z0;z<=z1;z++)for(int x=x0;x<=x1;x++)
        if(flat_get_block(x,y,z)==block_id)return 1;
    return 0;
}

/* Port of the collision-order and step selection in 1.8.8 Entity.moveEntity.
   The old shared controller moved X and Z independently, then implemented a
   step by jumping the complete 0.6 blocks.  That produces positions a vanilla
   client can never report around slabs, stairs, panes and ledges. */
static void java47_player_move_entity(float move_x, float move_y, float move_z,
                                      int sneaking, int was_on_ground) {
    FlatAABB box = java47_player_box();
    float x=move_x,y=move_y,z=move_z;
    float original_x=x,original_y=y,original_z=z;

    flat_world_map_enter();

    if (was_on_ground && sneaking) {
        const float edge_step=0.05f;
        while(x!=0.0f&&!java47_aabb_offset_has_collision(&box,x,-1.0f,0.0f)){
            if(x<edge_step&&x>=-edge_step)x=0.0f;else x+=x>0.0f?-edge_step:edge_step;
            original_x=x;
        }
        while(z!=0.0f&&!java47_aabb_offset_has_collision(&box,0.0f,-1.0f,z)){
            if(z<edge_step&&z>=-edge_step)z=0.0f;else z+=z>0.0f?-edge_step:edge_step;
            original_z=z;
        }
        while(x!=0.0f&&z!=0.0f&&!java47_aabb_offset_has_collision(&box,x,-1.0f,z)){
            if(x<edge_step&&x>=-edge_step)x=0.0f;else x+=x>0.0f?-edge_step:edge_step;
            if(z<edge_step&&z>=-edge_step)z=0.0f;else z+=z>0.0f?-edge_step:edge_step;
            original_x=x;original_z=z;
        }
    }

    FlatAABB start_box=box;
    FlatAABB sweep=java47_aabb_add_coord(box,x,y,z);
    FlatAABB boxes[192];
    int count=flat_get_collision_boxes(&sweep,boxes,(int)(sizeof(boxes)/sizeof(boxes[0])));

    for(int i=0;i<count;i++)y=aabb_clip_y(&boxes[i],&box,y);
    aabb_offset(&box,0.0f,y,0.0f);
    /* Do not combine the 0.6 auto-step path with an upward jump impulse.
       On full-block parkour/staircase jumps, tiny edge contacts could select
       the step candidate and report a rise larger than the 0.42 jump motion.
       The player can still jump naturally onto the next full block; ordinary
       walking/downward stepping keeps vanilla's step solver. */
    int may_step=(original_y<=0.0f)&&(was_on_ground||(original_y!=y&&original_y<0.0f));
    for(int i=0;i<count;i++)x=aabb_clip_x(&boxes[i],&box,x);
    aabb_offset(&box,x,0.0f,0.0f);
    for(int i=0;i<count;i++)z=aabb_clip_z(&boxes[i],&box,z);
    aabb_offset(&box,0.0f,0.0f,z);

    if(may_step&&(original_x!=x||original_z!=z)){
        float clipped_x=x,clipped_y=y,clipped_z=z;
        FlatAABB clipped_box=box;
        float step_y=0.6f;
        FlatAABB step_sweep=java47_aabb_add_coord(start_box,original_x,step_y,original_z);
        FlatAABB step_boxes[192];
        int step_count=flat_get_collision_boxes(&step_sweep,step_boxes,(int)(sizeof(step_boxes)/sizeof(step_boxes[0])));

        FlatAABB path_a=start_box;
        FlatAABB horizontal_a=java47_aabb_add_coord(path_a,original_x,0.0f,original_z);
        float rise_a=step_y;
        for(int i=0;i<step_count;i++)rise_a=aabb_clip_y(&step_boxes[i],&horizontal_a,rise_a);
        aabb_offset(&path_a,0.0f,rise_a,0.0f);
        float ax=original_x;
        for(int i=0;i<step_count;i++)ax=aabb_clip_x(&step_boxes[i],&path_a,ax);
        aabb_offset(&path_a,ax,0.0f,0.0f);
        float az=original_z;
        for(int i=0;i<step_count;i++)az=aabb_clip_z(&step_boxes[i],&path_a,az);
        aabb_offset(&path_a,0.0f,0.0f,az);

        FlatAABB path_b=start_box;
        float rise_b=step_y;
        for(int i=0;i<step_count;i++)rise_b=aabb_clip_y(&step_boxes[i],&path_b,rise_b);
        aabb_offset(&path_b,0.0f,rise_b,0.0f);
        float bx=original_x;
        for(int i=0;i<step_count;i++)bx=aabb_clip_x(&step_boxes[i],&path_b,bx);
        aabb_offset(&path_b,bx,0.0f,0.0f);
        float bz=original_z;
        for(int i=0;i<step_count;i++)bz=aabb_clip_z(&step_boxes[i],&path_b,bz);
        aabb_offset(&path_b,0.0f,0.0f,bz);

        if(ax*ax+az*az>bx*bx+bz*bz){box=path_a;x=ax;z=az;y=-rise_a;}
        else {box=path_b;x=bx;z=bz;y=-rise_b;}
        for(int i=0;i<step_count;i++)y=aabb_clip_y(&step_boxes[i],&box,y);
        aabb_offset(&box,0.0f,y,0.0f);

        if(clipped_x*clipped_x+clipped_z*clipped_z>=x*x+z*z){
            box=clipped_box;x=clipped_x;y=clipped_y;z=clipped_z;
        }
    }

    {
        float fix_x = 0.0f, fix_y = 0.0f, fix_z = 0.0f;
        java47_resolve_small_penetration(&box, &fix_x, &fix_y, &fix_z);
        x += fix_x; y += fix_y; z += fix_z;
        if (fix_x != 0.0f) g_player_motion_x = 0.0f;
        if (fix_y != 0.0f) g_player_motion_y = 0.0f;
        if (fix_z != 0.0f) g_player_motion_z = 0.0f;

        /* A vanilla clipping pass must never create a fresh solid overlap.
           Translation fallbacks and float reconstruction can expose unusual
           shapes, so keep the previous safe AABB rather than allowing the
           player to become wedged and requiring them to walk backward. */
        FlatAABB final_hits[64], start_hits[64];
        int final_collision = flat_get_collision_boxes(&box, final_hits, 64) > 0;
        int start_collision = flat_get_collision_boxes(&start_box, start_hits, 64) > 0;
        if (final_collision && !start_collision) {
            box = start_box;
            x = y = z = 0.0f;
            g_player_motion_x = g_player_motion_y = g_player_motion_z = 0.0f;
        }
    }

    flat_world_map_leave();

    g_player_x=(box.minx+box.maxx)*0.5f;
    g_player_y=box.miny+1.62f;
    g_player_z=(box.minz+box.maxz)*0.5f;
    g_player_collided_horiz=(original_x!=x||original_z!=z);
    g_player_on_ground=(original_y!=y&&original_y<0.0f)?1:0;
    g_player_server_on_ground=g_player_on_ground;
    if(original_x!=x)g_player_motion_x=0.0f;
    if(original_y!=y)g_player_motion_y=0.0f;
    if(original_z!=z)g_player_motion_z=0.0f;
}


/* Async tick context flags are defined before ingame_tick() because the
   synchronous function checks whether it is currently running on the tick
   worker. */
static PEX_THREAD_LOCAL int g_ingame_tick_async_worker_context = 0;
static volatile int g_ingame_tick_async_needs_main_pump = 0;

#define PEX_ASYNC_INGAME_TICK 0

#if PEX_ASYNC_INGAME_TICK
static CRITICAL_SECTION g_ingame_tick_async_cs;
static HANDLE g_ingame_tick_async_thread = NULL;
static int g_ingame_tick_async_initialized = 0;
static volatile LONG g_ingame_tick_async_stop = 0;
static int g_ingame_tick_async_busy_flag = 0;
static int g_ingame_tick_async_failed = 0;
static int g_ingame_tick_async_completed = 0;
/* Time of the simulated tick boundary represented by the newest published
   prev/current player pair.  This is intentionally NOT tick-completion time:
   completion time varies with chunk/lighting/entity work, and using it as the
   render clock makes walking hold, then burst, on every platform where ticks
   take a non-zero or variable amount of time. */
static double g_ingame_tick_async_last_completed_time = 0.0;
#endif

static void player_render_capture_current(PexPlayerRenderState *s) {
    if (!s) return;
    s->x = g_player_x;
    s->y = g_player_y;
    s->z = g_player_z;
    s->prev_x = g_player_prev_x;
    s->prev_y = g_player_prev_y;
    s->prev_z = g_player_prev_z;
    s->yaw = g_player_yaw;
    s->pitch = g_player_pitch;
    s->prev_yaw = g_player_prev_yaw;
    s->prev_pitch = g_player_prev_pitch;
    s->distance_walked = g_distance_walked;
    s->prev_distance_walked = g_prev_distance_walked;
    s->limb_swing = g_limb_swing;
    s->prev_limb_swing = g_prev_limb_swing;
    s->limb_swing_amount = g_limb_swing_amount;
    s->prev_limb_swing_amount = g_prev_limb_swing_amount;
    s->camera_yaw = g_camera_yaw;
    s->prev_camera_yaw = g_prev_camera_yaw;
    s->camera_pitch = g_camera_pitch;
    s->prev_camera_pitch = g_prev_camera_pitch;
    s->render_arm_yaw = g_render_arm_yaw;
    s->prev_render_arm_yaw = g_prev_render_arm_yaw;
    s->render_arm_pitch = g_render_arm_pitch;
    s->prev_render_arm_pitch = g_prev_render_arm_pitch;
    s->dead = g_player_dead;
    s->death_time = g_player_death_time;
    s->hurt_time = g_player_hurt_time;
    s->max_hurt_time = g_player_max_hurt_time;
    s->attacked_at_yaw = g_player_attacked_at_yaw;
    s->ingame_ticks = g_ingame_ticks;
}

static void player_render_publish_after_tick(void) {
#if PEX_ASYNC_INGAME_TICK
    /* The async worker publishes player snapshot and tick-boundary timestamp
       together after ingame_tick() returns.  Publishing the snapshot here first
       would allow one render frame to see new position with the old partial
       clock, which looks exactly like a walking micro-stutter. */
    if (g_ingame_tick_async_worker_context) return;
#endif
    PexPlayerRenderState s;
    player_render_capture_current(&s);
    g_player_render_published = s;
    g_player_render_frame = s;
    g_player_render_published_valid = 1;
}

static void player_render_overlay_live_look(PexPlayerRenderState *s) {
    if (!s) return;
    /* Mouse/touch/gamepad look is render-rate input.  Keep it live so fixing the
       worker position snapshot does not make camera rotation fall back to 20 Hz. */
    s->yaw = g_player_yaw;
    s->pitch = g_player_pitch;
    s->prev_yaw = g_player_prev_yaw;
    s->prev_pitch = g_player_prev_pitch;
}

static void player_render_begin_frame(void) {
    if (world_quit_is_active() || g_screen == SCREEN_SAVING_QUIT) return;
#if PEX_ASYNC_INGAME_TICK
    if (g_player_render_frame_from_async_partial) {
        g_player_render_frame_from_async_partial = 0;
        player_render_overlay_live_look(&g_player_render_frame);
        return;
    }
    if (g_ingame_tick_async_initialized && g_ingame_tick_async_thread &&
        !g_ingame_tick_async_failed && !g_mp_connected && !pex_net_is_connecting()) {
        int valid = 0;
        EnterCriticalSection(&g_ingame_tick_async_cs);
        valid = g_player_render_published_valid;
        if (valid) g_player_render_frame = g_player_render_published;
        LeaveCriticalSection(&g_ingame_tick_async_cs);
        if (valid) {
            player_render_overlay_live_look(&g_player_render_frame);
            return;
        }
    }
#endif
    player_render_capture_current(&g_player_render_frame);
}

static void player_set_sprinting_125(int sprinting) {
    sprinting = sprinting ? 1 : 0;
    if (g_player_sprinting == sprinting) {
        if (sprinting && g_sprinting_ticks_left <= 0) g_sprinting_ticks_left = 600;
        return;
    }
    g_player_sprinting = sprinting;
    g_sprinting_ticks_left = sprinting ? 600 : 0;
}

static int player_can_sprint_125(void) {
    /* Java 1.2.5 EntityPlayerSP uses FoodStats > 6.0F.  Creative players keep
       damage disabled and effectively have food for client sprint gating. */
    return player_is_creative() || g_player_food_level > 6;
}

static int player_is_using_bow_125(void) {
    return g_bow_item_in_use;
}

static int player_bow_use_duration_125(void) {
    return g_bow_item_in_use ? g_bow_use_ticks : 0;
}

static float player_fov_target_multiplier_125(void) {
    /* EntityPlayerSP.getFOVMultiplier(): flying widens by 1.1 and sprinting
       comes from landMovementFactor/speedOnGround = 1.3. */
    float m = 1.0f;
    if (g_creative_flying) m *= 1.1f;
    float speed_ratio = g_player_sprinting ? 1.3f : 1.0f;
    m *= (speed_ratio + 1.0f) * 0.5f;
    if (player_is_using_bow_125()) {
        float draw = (float)player_bow_use_duration_125() / 20.0f;
        if (draw > 1.0f) draw = 1.0f;
        else draw *= draw;
        m *= 1.0f - draw * 0.15f;
    }
    return m;
}

static float player_fov_multiplier_125(void) {
    /* EntityRenderer interpolates fovModifierHandPrev -> fovModifierHand. */
    float partial = g_frame_partial;
    if (partial < 0.0f) partial = 0.0f;
    if (partial > 1.0f) partial = 1.0f;
    return g_prev_fov_modifier_hand + (g_fov_modifier_hand - g_prev_fov_modifier_hand) * partial;
}

static void pex_update_time_light_bucket(void) {
    int sub = flat_skylight_subtracted();
    g_prof_skylight_subtracted_last = sub;
}

static void ingame_tick(void) {
    player_capabilities_apply_game_mode();
    double prof_ingame_start = profile_begin();
    double prof_part = 0.0;
    int input_active = (g_screen == SCREEN_INGAME && !g_player_dead);

    g_ingame_ticks++;
    if (!(g_mp_connected && pex_java47_is_playing() && !pex_java47_daylight_cycle_enabled())) g_world_time++;
    pex_stats_tick();
    prof_part = profile_begin();
    pex_update_time_light_bucket();
    profile_add_time(PROF_DAYLIGHT_MESH, prof_part);
    if (g_hearts_life > 0) g_hearts_life--;
    if (g_save_message_ticks > 0) g_save_message_ticks--;
    if (g_record_playing_up_for > 0) g_record_playing_up_for--;
    for (int i = 0; i < g_chat_count; i++) g_chat_lines[i].age++;
    prof_part = profile_begin();
    inventory_tick();
    update_equipped_item();
    profile_add_time(PROF_INVENTORY, prof_part);
    prof_part = profile_begin();
    furnace_tick_all();
    profile_add_time(PROF_FURNACE, prof_part);
    if (g_player_hurt_time > 0) g_player_hurt_time--;
    prof_part = profile_begin();
    update_dropped_items();
    update_vehicles();
    update_projectiles();
    update_xp_orbs();
    profile_add_time(PROF_DROPS, prof_part);
    prof_part = profile_begin();
    update_passive_mobs();
    profile_add_time(PROF_ENTITY_PASSIVE_MOBS, prof_part);
    prof_part = profile_begin();
    update_dig_particles();
    update_portal_ambient_effects();
    profile_add_time(PROF_PARTICLES, prof_part);
    if (!g_mp_connected) {
        prof_part = profile_begin();
        update_falling_blocks();
        profile_add_time(PROF_FALLING, prof_part);
        prof_part = profile_begin();
#if defined(PEX_PLATFORM_ANDROID) || defined(PEX_PLATFORM_ANDROID_TV) || defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
        update_infinite_world_streaming();
        flat_flush_pending_lighting();
#else
        world_stream_service_ensure();
#endif
        profile_add_time(PROF_WORLD_STREAM, prof_part);
        prof_part = profile_begin();
        update_liquids();
        profile_add_time(PROF_LIQUIDS, prof_part);
        dimension_tick_portal_collision();
    }
    prof_part = profile_begin();
    update_buttons_and_plates();
    profile_add_time(PROF_BUTTONS, prof_part);
    double prof_player_logic_start = profile_begin();

    if (g_player_dead) {
        g_player_death_time++;
        if (g_loaded_world_dir[0] && g_save_dirty &&
            (g_ingame_ticks - g_last_autosave_tick) >= AUTOSAVE_INTERVAL_TICKS) {
            if (!g_mp_connected) save_current_world_state();
        }
        if (g_mp_connected) pex_net_send_player_state();
        player_render_publish_after_tick();
        profile_add_time(PROF_PLAYER_LOGIC, prof_player_logic_start);
        profile_add_time(PROF_INGAME_TOTAL, prof_ingame_start);
        return;
    }

#if defined(PEX_PLATFORM_PSP)
    if (!psp_player_terrain_ready_or_hold()) {
        player_render_publish_after_tick();
        profile_add_time(PROF_PLAYER_LOGIC, prof_player_logic_start);
        profile_add_time(PROF_INGAME_TOTAL, prof_ingame_start);
        return;
    }
#endif

    if (g_right_click_delay_timer > 0) g_right_click_delay_timer--;
    g_break_swing_holding = 0;
    g_prev_hand_swing = g_hand_swing;
    if (input_active) {
        g_right_use_button_down = key_down_vk(VK_RBUTTON);
        update_breaking();
    } else {
        g_right_use_button_down = 0;
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

    player_potion_update_tick();
    update_held_map_item_tick();
    update_bow_item_use_tick();

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
    g_prev_render_arm_yaw = g_render_arm_yaw;
    g_prev_render_arm_pitch = g_render_arm_pitch;

    int in_water = flat_player_in_water();
    int in_lava = flat_player_in_lava();
    if (!g_mp_connected) {
        if (flat_player_head_in_water()) {
            if (player_has_potion(PEX_POTION_WATER_BREATHING)) {
                if (g_player_air < 300) g_player_air += 4;
                if (g_player_air > 300) g_player_air = 300;
            } else if (g_player_air > 0) g_player_air--;
        } else {
            g_player_air += 4;
            if (g_player_air > 300) g_player_air = 300;
        }
    }
    if (in_water && !g_player_was_in_water) {
        spawn_water_entry_particles(g_player_x, g_player_y - 1.0f, g_player_z, g_player_motion_x, g_player_motion_z);
        pex_sound_play("random.splash", 1.0f, 1.0f);
    }
    g_player_was_in_water = in_water;
    if (!g_mp_connected) {
        if (in_water && g_player_fire_ticks > 0) {
            pex_sound_play("random.fizz", 0.7f, 1.6f + (pex_rand_float01() - pex_rand_float01()) * 0.4f);
            g_player_fire_ticks = 0;
        }
        if (g_player_fire_ticks > 0) {
            if ((g_player_fire_ticks % 20) == 0)
                (void)player_attack_entity_from(pex_damage_source_simple(PEX_DAMAGE_ON_FIRE), 1);
            --g_player_fire_ticks;
        }
        if (flat_get_block((int)floorf(g_player_x), (int)floorf(g_player_y - 0.9f), (int)floorf(g_player_z)) == BLOCK_FIRE) {
            if (g_player_fire_ticks < 160) g_player_fire_ticks = 160;
            (void)player_attack_entity_from(pex_damage_source_simple(PEX_DAMAGE_IN_FIRE), 1);
        }
    }
    int in_ladder = flat_player_in_ladder();
    if (in_water || in_lava || in_ladder) {
        g_player_fall_distance = 0.0f;
        if (in_water || in_lava) apply_player_fluid_velocity(in_water ? 1 : 0);
        if (in_lava && !g_mp_connected) {
            (void)player_attack_entity_from(pex_damage_source_simple(PEX_DAMAGE_LAVA), 4);
            if (g_player_fire_ticks < 300) g_player_fire_ticks = 300;
        }
    }

    if (g_sprinting_ticks_left > 0) {
        g_sprinting_ticks_left--;
        if (g_sprinting_ticks_left == 0) player_set_sprinting_125(0);
    }
    if (g_sprint_toggle_timer > 0) g_sprint_toggle_timer--;

    /* EntityLivingBase.onLivingUpdate clears tiny residual velocity before
       reading movement input. Leaving sub-0.005 drift alive changes both the
       position threshold cadence and the horizontal deltas seen by movement
       checks. */
    if (g_mp_join_backend == PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE) {
        if (fabsf(g_player_motion_x) < 0.005f) g_player_motion_x = 0.0f;
        if (fabsf(g_player_motion_y) < 0.005f) g_player_motion_y = 0.0f;
        if (fabsf(g_player_motion_z) < 0.005f) g_player_motion_z = 0.0f;
        if (g_java47_jump_ticks > 0) g_java47_jump_ticks--;
    } else {
        g_java47_jump_ticks = 0;
    }

    float strafe = 0.0f;
    float forward = 0.0f;
    float raw_forward_input = 0.0f;
    int jumping = 0;
    int sneaking = 0;
    int forward_for_sprint = 0;
    if (input_active) {
        if (key_down_vk(g_opts.keys[0])) { forward += 1.0f; raw_forward_input += 1.0f; }
        if (key_down_vk(g_opts.keys[2])) { forward -= 1.0f; raw_forward_input -= 1.0f; }
        if (key_down_vk(g_opts.keys[1])) strafe += 1.0f;
        if (key_down_vk(g_opts.keys[3])) strafe -= 1.0f;
        jumping = key_down_vk(g_opts.keys[4]);
        sneaking = key_down_vk(g_opts.keys[5]);
        forward_for_sprint = (raw_forward_input >= 0.8f) ? 1 : 0;

        /* Java 1.2.5 EntityPlayerSP double-tap sprint:
           previous-tick forward must be false, current forward >= 0.8, on
           ground, enough food, not sneaking/using/blind.  PexCraft has no item
           use hold/blindness yet, so those gates are represented by available
           systems only. */
        if (g_player_on_ground && !g_prev_sprint_forward && forward_for_sprint &&
            !g_player_sprinting && player_can_sprint_125() && !sneaking) {
            if (g_sprint_toggle_timer == 0) {
                g_sprint_toggle_timer = 7;
            } else {
                player_set_sprinting_125(1);
                g_sprint_toggle_timer = 0;
            }
        }
        if (sneaking) {
            g_sprint_toggle_timer = 0;
            strafe *= 0.3f;
            forward *= 0.3f;
        }
        if (g_player_sprinting && (sneaking || !forward_for_sprint || g_player_collided_horiz || !player_can_sprint_125())) {
            player_set_sprinting_125(0);
        }
    } else {
        player_set_sprinting_125(0);
        g_sprint_toggle_timer = 0;
        g_prev_sprint_forward = 0;
    }

    if (!player_is_creative()) {
        g_creative_flying = 0;
        g_creative_fly_toggle_timer = 0;
    } else {
        /* Java EntityPlayerSP: double-tap jump within 7 ticks toggles
           capabilities.isFlying; while flying, sneak moves down and jump moves up. */
        if (g_creative_fly_toggle_timer > 0) g_creative_fly_toggle_timer--;
        if (!g_prev_jump_down && jumping) {
            if (g_creative_fly_toggle_timer == 0) {
                g_creative_fly_toggle_timer = 7;
            } else {
                g_creative_flying = !g_creative_flying;
                g_creative_fly_toggle_timer = 0;
                g_player_fall_distance = 0.0f;
            }
        }
        if (g_creative_flying) {
            if (sneaking) g_player_motion_y -= 0.15f;
            if (jumping)  g_player_motion_y += 0.15f;
            g_player_fall_distance = 0.0f;
        }
    }
    g_prev_jump_down = jumping;

    g_prev_fov_modifier_hand = g_fov_modifier_hand;
    g_fov_modifier_hand += (player_fov_target_multiplier_125() - g_fov_modifier_hand) * 0.5f;

    int normal_jump = jumping && !(player_is_creative() && g_creative_flying);
    if (g_mp_join_backend == PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE && !normal_jump)
        g_java47_jump_ticks = 0;

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
    int trying_water_exit = normal_jump && in_water &&
        flat_player_has_water_exit_ledge(g_player_x, g_player_y, g_player_z, water_exit_dx, water_exit_dz);

    int suff_block = g_mp_connected ? 0 : flat_player_suffocation_block();
    if (suff_block) {
        g_suffocation_damage_timer++;
        if (g_suffocation_damage_timer >= 20) {
            g_suffocation_damage_timer = 0;
            (void)player_attack_entity_from(pex_damage_source_simple(PEX_DAMAGE_IN_WALL), 1);
        }
    } else {
        g_suffocation_damage_timer = 0;
    }

    if (normal_jump && in_ladder) {
        g_player_motion_y = 0.20f;
        g_player_on_ground = 0;
    } else if (normal_jump && in_water) {
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
    } else if (normal_jump && in_lava) {
        /* Lava remains heavy/slow. */
        g_player_motion_y += 0.04f;
    } else if (normal_jump && g_player_on_ground &&
               (g_mp_join_backend != PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE || g_java47_jump_ticks == 0)) {
        pex_stats_add_general(PEX_STAT_JUMPS, 1);
        /* The old controller subtracts gravity before moving, hence its 0.50
           compensation. Protocol 47 follows EntityLivingBase exactly: move
           by 0.42 on the jump tick, then apply gravity and drag afterward. */
        g_player_motion_y = (g_mp_join_backend == PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE) ? 0.42f : 0.50f;
        if (g_mp_join_backend == PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE) g_java47_jump_ticks = 10;
        if (player_has_potion(PEX_POTION_JUMP)) g_player_motion_y += 0.10f * (float)(player_potion_amplifier(PEX_POTION_JUMP) + 1);
        if (g_player_sprinting) {
            float yaw_rad = g_player_yaw * (float)M_PI / 180.0f;
            g_player_motion_x -= sinf(yaw_rad) * 0.20f;
            g_player_motion_z += cosf(yaw_rad) * 0.20f;
        }
        /* Vanilla jump() sets motionY/isAirBorne but leaves onGround true
           until moveEntity resolves the jump.  Keeping it true here is
           important: the jump tick still receives ground acceleration and
           ground slipperiness before the vertical move makes the player
           airborne. */
        player_add_exhaustion(g_player_sprinting ? 0.8f : 0.2f);
    }

    /* Vanilla computes both movement acceleration and this tick's horizontal
       drag from the onGround state that existed before moveEntity(). The second
       slipperiness lookup in EntityLivingBase also happens before moveEntity,
       even though the multiplication is applied afterward. Using the
       post-collision state makes takeoff ticks use 0.91 air drag (far too fast)
       and landing ticks use ground drag (far too slow), which strict movement
       checks identify as an invalid jump curve. */
    float java_ground_friction_before = 0.91f;
    if (g_mp_join_backend == PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE && g_player_on_ground) {
        int below = flat_get_block((int)floorf(g_player_x),
                                   (int)floorf(g_player_y - 1.62f) - 1,
                                   (int)floorf(g_player_z));
        java_ground_friction_before = block_slipperiness_for_item(below) * 0.91f;
        if (java_ground_friction_before < 0.05f) java_ground_friction_before = 0.54600006f;
    }

    if (g_mp_join_backend == PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE) {
        /* EntityLivingBase.onLivingUpdate applies this after MovementInput has
           already handled sneak's 0.3 multiplier. */
        strafe *= 0.98f;
        forward *= 0.98f;
    }

    float input_len = sqrtf(strafe * strafe + forward * forward);
    if (input_len >= 0.01f) {
        if (input_len < 1.0f) input_len = 1.0f;
        float accel = g_creative_flying ? 0.05f : (in_water ? 0.02f : (in_lava ? 0.02f : (g_player_on_ground ? 0.1f : 0.02f)));
        if (g_mp_join_backend == PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE &&
            g_player_on_ground && !g_creative_flying && !in_water && !in_lava && !in_ladder) {
            /* EntityLivingBase.moveEntityWithHeading:
               0.16277136 / (slipperiness * 0.91)^3.  This is especially
               important on ice; using ordinary-ground acceleration there
               looks like speed movement to anti-cheat. */
            float f = java_ground_friction_before;
            accel *= 0.16277136f / (f * f * f);
        }
        if (g_player_sprinting && !g_creative_flying && !in_water && !in_lava && !in_ladder) accel *= 1.3f;
        if (!g_creative_flying && !in_water && !in_lava && !in_ladder) {
            if (g_mp_join_backend == PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE) {
                /* In 1.8.8 generic.movementSpeed and speed/slowness affect the
                   ground movement factor. Air control stays at jumpMovementFactor
                   (0.02, or 0.026 while sprinting). Applying potion multipliers
                   again in mid-air makes hop/sprint deltas fail strict checks. */
                if (g_player_on_ground) {
                    accel *= pex_java47_movement_speed_scale();
                    if (player_has_potion(PEX_POTION_SPEED)) accel *= 1.0f + 0.20f * (float)(player_potion_amplifier(PEX_POTION_SPEED) + 1);
                    if (player_has_potion(PEX_POTION_SLOWNESS)) accel *= 1.0f - 0.15f * (float)(player_potion_amplifier(PEX_POTION_SLOWNESS) + 1);
                }
            } else {
                if (player_has_potion(PEX_POTION_SPEED)) accel *= 1.0f + 0.20f * (float)(player_potion_amplifier(PEX_POTION_SPEED) + 1);
                if (player_has_potion(PEX_POTION_SLOWNESS)) accel *= 1.0f - 0.15f * (float)(player_potion_amplifier(PEX_POTION_SLOWNESS) + 1);
            }
            if (accel < 0.002f) accel = 0.002f;
        }
        strafe = strafe * accel / input_len;
        forward = forward * accel / input_len;
        float yaw_rad = g_player_yaw * (float)M_PI / 180.0f;
        float sy = sinf(yaw_rad);
        float cy = cosf(yaw_rad);
        g_player_motion_x += strafe * cy - forward * sy;
        g_player_motion_z += forward * cy + strafe * sy;
    }

    float previous_motion_y = g_player_motion_y;
    int java_delayed_gravity =
        g_mp_join_backend == PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE &&
        !g_creative_flying && !in_water && !in_lava;
    if (g_creative_flying) {
        /* PlayerControllerCreative flying skips gravity; EntityPlayer restores
           motionY from before super.moveEntityWithHeading and damps it later. */
    } else if (in_ladder) {
        if (g_player_motion_y < -0.15f) g_player_motion_y = -0.15f;
    } else if (!java_delayed_gravity) {
        g_player_motion_y -= (in_water ? 0.010f : (in_lava ? 0.02f : 0.08f));
    }

    if (g_mp_join_backend != PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE &&
        !g_creative_flying && input_active && sneaking && flat_player_has_sneak_support(g_player_x, g_player_y, g_player_z)) {
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
    int was_on_ground = g_player_on_ground;

    if (g_mp_join_backend == PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE) {
        int java_in_web = java47_player_intersects_block(BLOCK_WEB);
        if (java_in_web) {
            /* Entity.moveEntity consumes a web flag by scaling this tick's
               displacement, then zeroing motion before gravity/drag. */
            g_player_motion_x *= 0.25f;
            g_player_motion_y *= 0.05000000074505806f;
            g_player_motion_z *= 0.25f;
        }
        if (in_ladder) {
            if (g_player_motion_x < -0.15f) g_player_motion_x = -0.15f;
            if (g_player_motion_x >  0.15f) g_player_motion_x =  0.15f;
            if (g_player_motion_z < -0.15f) g_player_motion_z = -0.15f;
            if (g_player_motion_z >  0.15f) g_player_motion_z =  0.15f;
            if (g_player_motion_y < -0.15f) g_player_motion_y = -0.15f;
            if (sneaking && g_player_motion_y < 0.0f) g_player_motion_y = 0.0f;
        }
        java47_player_move_entity(g_player_motion_x, g_player_motion_y, g_player_motion_z,
                                  input_active && sneaking, was_on_ground);
        liquid_horizontal_collision = g_player_collided_horiz;
        if (in_ladder && g_player_collided_horiz) g_player_motion_y = 0.20f;
        if (java47_player_intersects_block(BLOCK_SOUL_SAND)) {
            g_player_motion_x *= 0.4f;
            g_player_motion_z *= 0.4f;
        }
        if (java_in_web) g_player_motion_x = g_player_motion_y = g_player_motion_z = 0.0f;
    } else {
        int player_collided_horiz_this_tick = 0;
        float step_base_y = g_player_y;

        float old_x = g_player_x;
        g_player_x += g_player_motion_x;
        if (flat_player_aabb_collides(g_player_x, g_player_y, g_player_z)) {
            float try_x = g_player_x;
            g_player_x = old_x;
            if (!flat_player_try_step_move(try_x, step_base_y, g_player_z)) {
                g_player_motion_x = 0.0f;
                liquid_horizontal_collision = 1;
                player_collided_horiz_this_tick = 1;
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
                player_collided_horiz_this_tick = 1;
            }
        }

        g_player_collided_horiz = player_collided_horiz_this_tick;
        if (in_ladder && (liquid_horizontal_collision || (input_active && raw_forward > 0.01f))) {
            if (g_player_motion_y < 0.20f) g_player_motion_y = 0.20f;
            g_player_on_ground = 0;
        }

        float old_y = g_player_y;
        g_player_on_ground = 0;
        g_player_server_on_ground = 0;
#if defined(PEX_PLATFORM_PSP)
        (void)old_y;
        {
            float total_dy = g_player_motion_y;
            int steps = (int)ceilf(fabsf(total_dy) / 0.25f);
            if (steps < 1) steps = 1;
            if (steps > 24) steps = 24;
            float step_dy = total_dy / (float)steps;
            for (int i = 0; i < steps; i++) {
                float before_step_y = g_player_y;
                g_player_y += step_dy;
                if (flat_player_aabb_collides(g_player_x, g_player_y, g_player_z)) {
                    g_player_y = before_step_y;
                    if (total_dy < 0.0f) {
                        g_player_on_ground = 1;
                        g_player_server_on_ground = 1;
                    }
                    g_player_motion_y = 0.0f;
                    break;
                }
            }
        }
#else
        g_player_y += g_player_motion_y;
        if (flat_player_aabb_collides(g_player_x, g_player_y, g_player_z)) {
            g_player_y = old_y;
            if (g_player_motion_y < 0.0f) {
                g_player_on_ground = 1;
                g_player_server_on_ground = 1;
            }
            g_player_motion_y = 0.0f;
        }
#endif
        if (!g_player_on_ground &&
            flat_player_aabb_collides(g_player_x, g_player_y - 0.05f, g_player_z)) {
            g_player_on_ground = 1;
            g_player_server_on_ground = 1;
        }
    }

    if (g_player_sprinting && g_player_collided_horiz) player_set_sprinting_125(0);

    if (g_mp_join_backend != PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE && normal_jump && in_water && trying_water_exit) {
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

    /* Deobf EntityLiving.fall: damage is ceil(fallDistance - 3).  In
       multiplayer, only the server's health/status packets may apply damage. */
    if (g_player_on_ground) {
        if (!g_mp_connected) {
            if (!was_on_ground && !g_creative_flying && g_player_fall_distance >= 2.0f) {
                pex_stats_add_general(PEX_STAT_FALL_CM, (long long)floorf(g_player_fall_distance * 100.0f + 0.5f));
            }
            if (!was_on_ground && g_player_fall_distance > 3.0f) {
                pex_sound_play(g_player_fall_distance > 6.0f ? "damage.fallbig" : "damage.fallsmall", 1.0f, 1.0f);
                int dmg = (int)ceilf(g_player_fall_distance - 3.0f);
                if (dmg > 0) (void)player_attack_entity_from(pex_damage_source_simple(PEX_DAMAGE_FALL), dmg);
            }
        }
        g_player_fall_distance = 0.0f;
    } else if (previous_motion_y < 0.0f) {
        g_player_fall_distance += -previous_motion_y;
    }

    if (!g_mp_connected && g_player_y < -64.0f) {
        (void)player_attack_entity_from(pex_damage_source_simple(PEX_DAMAGE_OUT_OF_WORLD), 4);
    }

    if (g_creative_flying && g_player_on_ground) {
        g_creative_flying = 0;
        g_creative_fly_toggle_timer = 0;
    }

    if (java_delayed_gravity) {
        /* Vanilla applies gravity after moveEntity(), even on a grounded tick;
           the next tick's downward collision is what keeps onGround true. */
        g_player_motion_y -= 0.08f;
    }

    if (g_creative_flying) {
        g_player_motion_y *= 0.60f;
        g_player_motion_x *= 0.91f;
        g_player_motion_z *= 0.91f;
    } else if (in_ladder && g_mp_join_backend != PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE) {
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
        if (g_mp_join_backend == PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE) {
            /* EntityLivingBase chooses f4 before moveEntity(). Therefore a
               jump tick still receives the ground block's drag, while the
               tick that lands still receives air drag. */
            friction = java_ground_friction_before;
        }
        g_player_motion_x *= friction;
        g_player_motion_z *= friction;
    }

    float dx = g_player_x - g_player_prev_x;
    float dy = g_player_y - g_player_prev_y;
    float dz = g_player_z - g_player_prev_z;
    float horizontal = sqrtf(dx * dx + dz * dz);
    if (in_water || in_lava) {
        int dist_cm = (int)floorf(sqrtf(dx * dx + dy * dy + dz * dz) * 100.0f + 0.5f);
        if (dist_cm > 0) player_add_exhaustion(0.015f * (float)dist_cm * 0.01f);
    } else if (g_player_on_ground) {
        int dist_cm = (int)floorf(horizontal * 100.0f + 0.5f);
        if (dist_cm > 0) {
            if (g_player_sprinting) player_add_exhaustion(10.0f * 0.01f * (float)dist_cm * 0.01f);
            else player_add_exhaustion(0.01f * (float)dist_cm * 0.01f);
        }
    }
    g_distance_walked += horizontal * 0.6f;
    if (g_player_on_ground && horizontal > 0.015f && g_distance_walked >= g_next_footstep_distance) {
        int bx = (int)floorf(g_player_x);
        int by = (int)floorf(g_player_y - 1.75f);
        int bz = (int)floorf(g_player_z);
        int step_block = flat_get_block(bx, by, bz);
        if (step_block > 0 && !block_is_liquid(step_block)) {
            PexBlockStepSound125 sound = pex_block_step_sound_125(step_block);
            pex_sound_play_at(sound.step_key, g_player_x, g_player_y - 1.0f, g_player_z,
                              sound.volume * 0.15f, sound.pitch);
        }
        g_next_footstep_distance = floorf(g_distance_walked) + 1.0f;
    }
    if (!g_player_on_ground) g_next_footstep_distance = g_distance_walked + 0.30f;

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
    /* Java 1.2.5 EntityPlayerSP.updateEntityActionState(): the first-person
       hand/item uses smoothed renderArmYaw/renderArmPitch, which lag half-way
       toward the real rotation and create the familiar item sway when looking
       around. */
    g_render_arm_pitch += (g_player_pitch - g_render_arm_pitch) * 0.5f;
    g_render_arm_yaw += (g_player_yaw - g_render_arm_yaw) * 0.5f;
    passive_mobs_apply_riding();
    {
        int riding_pig = 0;
        if (g_player_riding_passive_mob >= 0 && g_player_riding_passive_mob < MAX_PASSIVE_MOBS) {
            PassiveMob *mount = &g_passive_mobs[g_player_riding_passive_mob];
            riding_pig = mount->active && mount->type == PASSIVE_MOB_PIG;
        }
        pex_stats_track_player_position(in_water, in_ladder, g_creative_flying, riding_pig, 0, 0);
    }

    g_prev_sprint_forward = (input_active && !sneaking && forward_for_sprint) ? 1 : 0;

    if (!g_player_dead && !g_mp_connected && !player_is_creative()) player_foodstats_update();

    if (g_loaded_world_dir[0] && g_save_dirty &&
        (g_ingame_ticks - g_last_autosave_tick) >= AUTOSAVE_INTERVAL_TICKS) {
        if (!g_mp_connected) save_current_world_state();
    }
    if (g_mp_connected) pex_net_send_player_state();
    player_render_publish_after_tick();
    profile_add_time(PROF_PLAYER_LOGIC, prof_player_logic_start);
    profile_add_time(PROF_INGAME_TOTAL, prof_ingame_start);
}



/* -------------------------------------------------------------------------
   Desktop async ingame/simulation worker

   The render thread must keep all GL/D3D calls, window messages, and final mesh
   uploads.  Gameplay simulation and in-game world streaming are paced by this
   worker instead of by the frame loop.  The old async path still let the main
   thread enqueue/drop tick requests; when terrain work made the worker late,
   walking advanced in visible bursts.  The worker below owns the 20 Hz clock,
   keeps at most one simulation step in flight, and publishes a stable player render snapshot plus the simulated tick boundary
   time for interpolation.
   ------------------------------------------------------------------------- */

static int ingame_screen_allows_tick(void) {
    return g_screen == SCREEN_INGAME || g_screen == SCREEN_CHAT ||
           g_screen == SCREEN_INVENTORY || g_screen == SCREEN_CREATIVE || g_screen == SCREEN_WORKBENCH ||
           g_screen == SCREEN_FURNACE || g_screen == SCREEN_CHEST ||
           g_screen == SCREEN_DEATH || (g_mp_connected && g_screen == SCREEN_PAUSE);
}

#if PEX_ASYNC_INGAME_TICK
static void ingame_tick_async_wait_until(double target_time) {
    /* SDL_Delay(1)/Sleep(1) can oversleep by an entire scheduler quantum on
       Linux/Android, which makes the worker's 20 Hz movement clock uneven even
       when rendering is perfectly smooth.  Sleep only while far away, yield in
       the guard band, and spin for the final tiny slice so completed tick
       boundaries are regular. */
    for (;;) {
        double now = now_seconds();
        double wait = target_time - now;
        if (wait <= 0.0) break;
        if (wait > 0.006) {
            Sleep(1);
        } else if (wait > 0.0015) {
            Sleep(0);
        } else {
            do {
                now = now_seconds();
            } while (now < target_time);
            break;
        }
    }
}

static DWORD WINAPI ingame_tick_async_worker_proc(LPVOID unused) {
    (void)unused;
    g_pex_profile_thread_role = PEX_PROFILE_ROLE_ASYNC_TICK;
    const double tick_dt = 1.0 / 20.0;
    double next_tick_time = now_seconds() + tick_dt;

    for (;;) {
        int stop = 0;
        EnterCriticalSection(&g_ingame_tick_async_cs);
        stop = g_ingame_tick_async_stop;
        LeaveCriticalSection(&g_ingame_tick_async_cs);
        if (stop) break;

        double now = now_seconds();
        if (next_tick_time > now) {
            ingame_tick_async_wait_until(next_tick_time);
            continue;
        }

        /* Do not spiral after a breakpoint, alt-tab, shader compile, or very
           slow chunk install.  The timestamp published for interpolation must
           remain the tick boundary, not the later completion time. */
        double tick_boundary_time = next_tick_time;
        if (now - tick_boundary_time > 0.25) tick_boundary_time = now;
        next_tick_time = tick_boundary_time + tick_dt;

        if (!ingame_screen_allows_tick() || g_mp_connected || pex_net_is_connecting()) {
            Sleep(1);
            continue;
        }

        EnterCriticalSection(&g_ingame_tick_async_cs);
        g_ingame_tick_async_busy_flag = 1;
        LeaveCriticalSection(&g_ingame_tick_async_cs);

        double tick_start = now_seconds();
        g_ingame_tick_async_worker_context = 1;
        ingame_tick();
        g_ingame_tick_async_worker_context = 0;
        PexPlayerRenderState render_state;
        player_render_capture_current(&render_state);
        double tick_ms = (now_seconds() - tick_start) * 1000.0;
        if (tick_ms < 0.0) tick_ms = 0.0;

        EnterCriticalSection(&g_ingame_tick_async_cs);
        g_player_render_published = render_state;
        g_player_render_published_valid = 1;
        g_ingame_tick_async_busy_flag = 0;
        g_ingame_tick_async_completed++;
        g_ingame_tick_async_last_completed_time = tick_boundary_time;
        g_prof_async_tick_last_ms = tick_ms;
        if (g_prof_async_tick_samples <= 0) g_prof_async_tick_avg_ms = tick_ms;
        else g_prof_async_tick_avg_ms = g_prof_async_tick_avg_ms * 0.90 + tick_ms * 0.10;
        g_prof_async_tick_samples++;
        LeaveCriticalSection(&g_ingame_tick_async_cs);
    }
    return 0;
}

static void ingame_tick_async_init(void) {
    if (g_ingame_tick_async_initialized || g_ingame_tick_async_failed) return;
    /* Save-and-quit destroys world-session locks after all workers stop.  Recreate
       the map lock before a new world's simulation worker can enter world code. */
    flat_world_map_lock_ensure();
    g_ingame_tick_async_initialized = 1;
    g_ingame_tick_async_stop = 0;
    g_ingame_tick_async_busy_flag = 0;
    g_ingame_tick_async_completed = 0;
    g_ingame_tick_async_last_completed_time = now_seconds();
    g_player_render_published_valid = 0;
    g_player_render_frame_from_async_partial = 0;
    InitializeCriticalSection(&g_ingame_tick_async_cs);
    /* Give the worker enough stack for the legacy C tick path and collision
       helpers.  A tiny stack caused 0xC00000FD in previous worker attempts. */
    g_ingame_tick_async_thread = CreateThread(NULL, 0x400000, ingame_tick_async_worker_proc, NULL, 0, NULL);
    if (!g_ingame_tick_async_thread) { g_ingame_tick_async_failed = 1; return; }
    SetThreadPriority(g_ingame_tick_async_thread, THREAD_PRIORITY_ABOVE_NORMAL);
    pex_logf("async ingame simulation worker started");
}
#endif

static void ingame_tick_async_queue(void) {
#if PEX_ASYNC_INGAME_TICK
    /* Multiplayer still applies network state and sends packets from the main
       frame path until that path is explicitly double-buffered.  Single-player
       desktop simulation is owned by the worker and this call only ensures the
       worker exists; it does not enqueue or execute gameplay on the main thread. */
    if (g_mp_connected || pex_net_is_connecting()) { ingame_tick(); return; }
    ingame_tick_async_init();
    if (g_ingame_tick_async_failed || !g_ingame_tick_async_thread) {
        ingame_tick();
        return;
    }
#else
    ingame_tick();
#endif
}

static float ingame_tick_async_render_partial(float fallback_partial) {
#if PEX_ASYNC_INGAME_TICK
    if (world_quit_is_active() || g_screen == SCREEN_SAVING_QUIT) return fallback_partial;
    if (!g_ingame_tick_async_initialized || g_ingame_tick_async_failed ||
        !g_ingame_tick_async_thread || g_mp_connected || pex_net_is_connecting()) {
        g_player_render_frame_from_async_partial = 0;
        return fallback_partial;
    }
    double last = 0.0;
    int valid = 0;
    EnterCriticalSection(&g_ingame_tick_async_cs);
    last = g_ingame_tick_async_last_completed_time;
    valid = g_player_render_published_valid;
    if (valid) {
        g_player_render_frame = g_player_render_published;
        g_player_render_frame_from_async_partial = 1;
    } else {
        g_player_render_frame_from_async_partial = 0;
    }
    LeaveCriticalSection(&g_ingame_tick_async_cs);
    if (last <= 0.0 || !valid) return fallback_partial;
    float p = (float)((now_seconds() - last) * 20.0);
    if (p < 0.0f) p = 0.0f;
    if (p > 1.0f) p = 1.0f;
    return p;
#else
    return fallback_partial;
#endif
}

static void ingame_pump_async_tick(void) {
#if PEX_ASYNC_INGAME_TICK
    if (world_quit_is_active() || g_screen == SCREEN_SAVING_QUIT) return;
    if (!g_ingame_tick_async_needs_main_pump) return;
    g_ingame_tick_async_needs_main_pump = 0;
    /* No main-thread streaming pump here.  Gameplay streaming is serviced by
       the async ingame/simulation worker only. */
#endif
}

static void ingame_tick_async_request_stop(void) {
#if PEX_ASYNC_INGAME_TICK
    if (!g_ingame_tick_async_initialized) return;
    InterlockedExchange(&g_ingame_tick_async_stop, 1);
    if (g_ingame_tick_async_thread) SetThreadPriority(g_ingame_tick_async_thread, THREAD_PRIORITY_BELOW_NORMAL);
#endif
}

static void ingame_tick_async_shutdown(void) {
#if PEX_ASYNC_INGAME_TICK
    if (!g_ingame_tick_async_initialized) return;
    ingame_tick_async_request_stop();
    if (g_ingame_tick_async_thread) {
        WaitForSingleObject(g_ingame_tick_async_thread, INFINITE);
        CloseHandle(g_ingame_tick_async_thread);
        g_ingame_tick_async_thread = NULL;
    }
    DeleteCriticalSection(&g_ingame_tick_async_cs);
    g_ingame_tick_async_initialized = 0;
    g_ingame_tick_async_stop = 0;
    g_ingame_tick_async_busy_flag = 0;
    g_player_render_published_valid = 0;
    g_player_render_frame_from_async_partial = 0;
    pex_logf("async ingame simulation worker stopped; completed=%d", g_ingame_tick_async_completed);
#endif
}

static int ingame_tick_async_pending_count(void) {
#if PEX_ASYNC_INGAME_TICK
    /* The worker owns its 20 Hz clock now, so there is no main-thread tick
       request queue to backlog. */
    return 0;
#else
    return 0;
#endif
}

static int ingame_tick_async_busy(void) {
    if (world_quit_is_active()) return 0;
#if PEX_ASYNC_INGAME_TICK
    if (!g_ingame_tick_async_initialized) return 0;
    int v;
    EnterCriticalSection(&g_ingame_tick_async_cs);
    v = g_ingame_tick_async_busy_flag;
    LeaveCriticalSection(&g_ingame_tick_async_cs);
    return v;
#else
    return 0;
#endif
}

static double ingame_tick_async_last_ms(void) {
    if (world_quit_is_active()) return 0;
#if PEX_ASYNC_INGAME_TICK
    if (!g_ingame_tick_async_initialized) return 0.0;
    double v = 0.0;
    EnterCriticalSection(&g_ingame_tick_async_cs);
    v = g_prof_async_tick_last_ms;
    LeaveCriticalSection(&g_ingame_tick_async_cs);
    return v;
#else
    return 0.0;
#endif
}

static double ingame_tick_async_avg_ms(void) {
    if (world_quit_is_active()) return 0;
#if PEX_ASYNC_INGAME_TICK
    if (!g_ingame_tick_async_initialized) return 0.0;
    double v = 0.0;
    EnterCriticalSection(&g_ingame_tick_async_cs);
    v = g_prof_async_tick_avg_ms;
    LeaveCriticalSection(&g_ingame_tick_async_cs);
    return v;
#else
    return 0.0;
#endif
}

static int ingame_tick_async_dropped_count(void) {
#if PEX_ASYNC_INGAME_TICK
    return 0;
#else
    return 0;
#endif
}
