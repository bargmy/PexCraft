/* Split from original monolithic main.c. Included by src/main.c unity build. */

static int stack_empty(const ItemStack *s) { return !s || s->id <= 0 || s->count <= 0; }

static void stack_clear(ItemStack *s) { if (s) memset(s, 0, sizeof(*s)); }

static int stack_same_item(const ItemStack *a, const ItemStack *b) { return !stack_empty(a) && !stack_empty(b) && a->id == b->id && a->damage == b->damage; }

static int stack_limit_for_id(int id) {
    if (id == ITEM_STONE_SWORD || id == ITEM_STONE_SHOVEL ||
        id == ITEM_STONE_PICKAXE || id == ITEM_STONE_AXE) return 1;
    (void)id;
    return ITEM_MAX_STACK;
}

static ItemStack make_stack(int id, int count, int damage) { ItemStack s; s.id = id; s.count = count; s.damage = damage; s.pop_time = 0; return s; }

static int flat_index(int coord) { return coord - FLAT_WORLD_MIN; }

static int flat_y_index(int y) { return y - FLAT_WORLD_Y_MIN; }

static int flat_in_bounds(int x, int y, int z) {
    return y >= FLAT_WORLD_Y_MIN && y <= FLAT_WORLD_Y_MAX &&
           x >= FLAT_WORLD_MIN && x <= FLAT_WORLD_MAX &&
           z >= FLAT_WORLD_MIN && z <= FLAT_WORLD_MAX;
}

static int flat_get_block(int x, int y, int z) {
    if (!flat_in_bounds(x, y, z)) return 0;
    return g_flat_blocks[flat_y_index(y)][flat_index(z)][flat_index(x)];
}

static void flat_set_block(int x, int y, int z, int id) {
    if (!flat_in_bounds(x, y, z)) return;
    int *cell = &g_flat_blocks[flat_y_index(y)][flat_index(z)][flat_index(x)];
    if (*cell != id) {
        *cell = id;
        g_flat_world_geometry_dirty = 1;
    }
}

static void flat_world_reset_blocks(void) {
    memset(g_flat_blocks, 0, sizeof(g_flat_blocks));
    for (int z = 0; z < FLAT_WORLD_SIZE; z++) {
        for (int x = 0; x < FLAT_WORLD_SIZE; x++) {
            /* 100x100x64 world:
               y=0 bedrock, y=1..2 dirt, y=3 grass, y=4..63 air. */
            g_flat_blocks[flat_y_index(0)][z][x] = BLOCK_BEDROCK;
            g_flat_blocks[flat_y_index(1)][z][x] = BLOCK_DIRT;
            g_flat_blocks[flat_y_index(2)][z][x] = BLOCK_DIRT;
            g_flat_blocks[flat_y_index(3)][z][x] = BLOCK_GRASS;
        }
    }
    memset(g_drops, 0, sizeof(g_drops));
    g_breaking_block = 0;
    g_break_damage = g_prev_break_damage = 0.0f;
    g_block_hit_delay = 0;
    g_hand_swing = g_prev_hand_swing = 0.0f;
    g_hand_swing_ticks = 0;
    g_hand_swing_progress = 0.0f;
    g_hand_swing_active = 0;
    g_air_swing_playing = 0;
    g_air_swing_ticks = 0;
    g_air_swing_consumed = 0;
    g_break_swing_consumed = 0;
    g_break_swing_holding = 0;
    g_shovel_combo_count = 0;
    g_flat_world_geometry_dirty = 1;
}

static int block_drop_id(int block_id) {
    /* ph.java grass drops dirt via of.v.a(0, random). */
    if (block_id == BLOCK_GRASS) return BLOCK_DIRT;
    if (block_id == BLOCK_BEDROCK) return 0;
    return block_id;
}

static float block_hardness(int block_id) {
    if (block_id == BLOCK_BEDROCK) return -1.0f;
    if (block_id == BLOCK_GRASS) return 0.6f; /* of.u.c(0.6F) */
    if (block_id == BLOCK_DIRT) return 0.5f;  /* of.v.c(0.5F) */
    return 1.0f;
}

static float block_relative_strength(int block_id) {
    float h = block_hardness(block_id);
    if (h < 0.0f) return 0.0f;
    /* of.a(eh): canHarvest grass/dirt with empty hand, strength = 1/hardness/30. */
    float strength = 1.0f / h / 30.0f;
    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    if (!stack_empty(held) && held->id == ITEM_STONE_SHOVEL &&
        (block_id == BLOCK_DIRT || block_id == BLOCK_GRASS)) {
        strength *= (g_shovel_combo_count <= 0) ? 1.50f : 1.60f; /* only shovel boosts mining */
    }
    return strength;
}

static void trigger_hand_swing(void) {
    /* EntityLiving/ItemRenderer-style swing: start one 8 tick arm swing.
       Holding mine re-triggers it; using/placing gets one clean swing instead of
       the old one-frame snap. */
    if (!g_hand_swing_active && g_hand_swing_ticks == 0) {
        g_hand_swing_active = 1;
        g_hand_swing_ticks = 0;
        g_hand_swing_progress = 0.0f;
        g_hand_swing = 0.0f;
    }
}

static int flat_player_aabb_collides(float px, float py, float pz) {
    float feet = py - 1.62f;
    float minx = px - 0.30f, maxx = px + 0.30f;
    float miny = feet + 0.001f, maxy = feet + 1.80f - 0.001f;
    float minz = pz - 0.30f, maxz = pz + 0.30f;
    int x0 = (int)floorf(minx), x1 = (int)floorf(maxx - 0.001f);
    int y0 = (int)floorf(miny), y1 = (int)floorf(maxy - 0.001f);
    int z0 = (int)floorf(minz), z1 = (int)floorf(maxz - 0.001f);
    for (int y = y0; y <= y1; y++) {
        for (int z = z0; z <= z1; z++) {
            for (int x = x0; x <= x1; x++) {
                if (flat_get_block(x, y, z) != 0) return 1;
            }
        }
    }
    return 0;
}

static int flat_player_has_sneak_support(float px, float py, float pz) {
    /* Source-style sneaking edge check from B1.0.0 mm.java:249-273.
       The source reduces X/Z motion in 0.05 steps while the player's bounding
       box offset downward by 1.0 has no collision. This checks for ANY floor
       under the current player footprint, so the invisible wall is at the real
       block edge, not too early. */
    float feet = py - 1.62f;
    int floor_y = (int)floorf(feet - 1.0f + 0.001f);

    float minx = px - 0.30f, maxx = px + 0.30f;
    float minz = pz - 0.30f, maxz = pz + 0.30f;
    int x0 = (int)floorf(minx + 0.001f);
    int x1 = (int)floorf(maxx - 0.001f);
    int z0 = (int)floorf(minz + 0.001f);
    int z1 = (int)floorf(maxz - 0.001f);

    for (int z = z0; z <= z1; z++) {
        for (int x = x0; x <= x1; x++) {
            if (flat_get_block(x, floor_y, z) != 0) return 1;
        }
    }
    return 0;
}

static int flat_block_intersects_player(int bx, int by, int bz) {
    float feet = g_player_y - 1.62f;
    float minx = g_player_x - 0.30f, maxx = g_player_x + 0.30f;
    float miny = feet + 0.001f, maxy = feet + 1.80f - 0.001f;
    float minz = g_player_z - 0.30f, maxz = g_player_z + 0.30f;
    return maxx > (float)bx && minx < (float)(bx + 1) &&
           maxy > (float)by && miny < (float)(by + 1) &&
           maxz > (float)bz && minz < (float)(bz + 1);
}

static int flat_item_aabb_collides(float px, float py, float pz) {
    float minx = px - 0.125f, maxx = px + 0.125f;
    float miny = py - 0.125f, maxy = py + 0.125f;
    float minz = pz - 0.125f, maxz = pz + 0.125f;
    int x0 = (int)floorf(minx), x1 = (int)floorf(maxx - 0.001f);
    int y0 = (int)floorf(miny), y1 = (int)floorf(maxy - 0.001f);
    int z0 = (int)floorf(minz), z1 = (int)floorf(maxz - 0.001f);
    for (int y = y0; y <= y1; y++) {
        for (int z = z0; z <= z1; z++) {
            for (int x = x0; x <= x1; x++) {
                if (flat_get_block(x, y, z) != 0) return 1;
            }
        }
    }
    return 0;
}

static void inventory_reset(void) {
    memset(g_inventory, 0, sizeof(g_inventory));
    stack_clear(&g_carried_stack);

    /* Start hotbar stone tool set. Only the stone shovel gets the dirt/grass
       mining boost; sword/pickaxe/axe are added as usable visible items only. */
    g_inventory[0] = make_stack(ITEM_STONE_SWORD, 1, 0);
    g_inventory[1] = make_stack(ITEM_STONE_PICKAXE, 1, 0);
    g_inventory[2] = make_stack(ITEM_STONE_AXE, 1, 0);
    g_inventory[3] = make_stack(ITEM_STONE_SHOVEL, 1, 0);

    /* Keep the earlier requested reserve of 10 stone shovels in the main inventory. */
    for (int i = 0; i < 10; i++) {
        g_inventory[9 + i] = make_stack(ITEM_STONE_SHOVEL, 1, 0);
    }
}

static int inventory_add_stack(ItemStack st) {
    if (stack_empty(&st)) return 1;
    for (int i = 0; i < 36 && st.count > 0; i++) {
        if (stack_same_item(&g_inventory[i], &st) && g_inventory[i].count < stack_limit_for_id(st.id)) {
            int room = stack_limit_for_id(st.id) - g_inventory[i].count;
            int move = st.count < room ? st.count : room;
            g_inventory[i].count += move;
            g_inventory[i].pop_time = 5;
            st.count -= move;
        }
    }
    for (int i = 0; i < 36 && st.count > 0; i++) {
        if (stack_empty(&g_inventory[i])) {
            int move = st.count < stack_limit_for_id(st.id) ? st.count : stack_limit_for_id(st.id);
            g_inventory[i] = make_stack(st.id, move, st.damage);
            g_inventory[i].pop_time = 5;
            st.count -= move;
        }
    }
    return st.count <= 0;
}

static void inventory_tick(void) {
    for (int i = 0; i < 36; i++) if (g_inventory[i].pop_time > 0) g_inventory[i].pop_time--;
}

static void player_look_vector(float *dx, float *dy, float *dz) {
    float yaw = g_player_yaw * (float)M_PI / 180.0f;
    float pitch = g_player_pitch * (float)M_PI / 180.0f;
    float cp = cosf(pitch);
    *dx = -sinf(yaw) * cp;
    *dy = -sinf(pitch);
    *dz =  cosf(yaw) * cp;
}

static void spawn_item_stack(float x, float y, float z, ItemStack st, int random_spread) {
    if (stack_empty(&st)) return;
    int slot = -1;
    for (int i = 0; i < MAX_DROP_ENTITIES; i++) if (!g_drops[i].active) { slot = i; break; }
    if (slot < 0) slot = 0;
    FlatDroppedItem *e = &g_drops[slot];
    memset(e, 0, sizeof(*e));
    e->active = 1;
    e->stack = st;
    e->x = e->prev_x = x;
    e->y = e->prev_y = y;
    e->z = e->prev_z = z;
    e->rot = (float)(rand() % 6283) / 1000.0f;
    e->pickup_delay = 10;
    if (random_spread) {
        e->mx = ((float)rand() / (float)RAND_MAX) * 0.2f - 0.1f;
        e->my = 0.2f;
        e->mz = ((float)rand() / (float)RAND_MAX) * 0.2f - 0.1f;
    } else {
        float lx, ly, lz;
        player_look_vector(&lx, &ly, &lz);
        e->mx = lx * 0.3f;
        e->my = 0.1f + ly * 0.1f;
        e->mz = lz * 0.3f;
    }
}

static void inventory_drop_selected_one(void) {
    ItemStack *s = &g_inventory[g_selected_hotbar_slot];
    if (stack_empty(s)) return;
    ItemStack one = make_stack(s->id, 1, s->damage);
    s->count--;
    if (s->count <= 0) stack_clear(s);
    spawn_item_stack(g_player_x, g_player_y - 0.30f, g_player_z, one, 0);
    trigger_hand_swing();
    hud_add_chat("Dropped item.");
}

static int inventory_slot_at(int mx, int my) {
    int inv_x = (g_gui_w - 176) / 2;
    int inv_y = (g_gui_h - 166) / 2;
    int rx = mx - inv_x;
    int ry = my - inv_y;
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 9; col++) {
            int sx = 8 + col * 18;
            int sy = 84 + row * 18;
            if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return 9 + col + row * 9;
        }
    }
    for (int col = 0; col < 9; col++) {
        int sx = 8 + col * 18;
        int sy = 142;
        if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return col;
    }
    return -1;
}

static void inventory_mouse_click(int mx, int my, int button) {
    int inv_x = (g_gui_w - 176) / 2;
    int inv_y = (g_gui_h - 166) / 2;
    int outside = (mx < inv_x || my < inv_y || mx >= inv_x + 176 || my >= inv_y + 166);
    int slot = inventory_slot_at(mx, my);
    if (outside && !stack_empty(&g_carried_stack)) {
        if (button == 0) { spawn_item_stack(g_player_x, g_player_y - 0.30f, g_player_z, g_carried_stack, 0); stack_clear(&g_carried_stack); }
        else { ItemStack one = make_stack(g_carried_stack.id, 1, g_carried_stack.damage); spawn_item_stack(g_player_x, g_player_y - 0.30f, g_player_z, one, 0); if (--g_carried_stack.count <= 0) stack_clear(&g_carried_stack); }
        return;
    }
    if (slot < 0) return;
    ItemStack *s = &g_inventory[slot];
    if (button == 0) {
        if (stack_empty(&g_carried_stack)) {
            if (!stack_empty(s)) { g_carried_stack = *s; stack_clear(s); }
        } else if (stack_empty(s)) {
            *s = g_carried_stack; stack_clear(&g_carried_stack);
        } else if (stack_same_item(s, &g_carried_stack) && s->count < stack_limit_for_id(s->id)) {
            int room = stack_limit_for_id(s->id) - s->count;
            int move = g_carried_stack.count < room ? g_carried_stack.count : room;
            s->count += move;
            g_carried_stack.count -= move;
            s->pop_time = 5;
            if (g_carried_stack.count <= 0) stack_clear(&g_carried_stack);
        } else {
            ItemStack tmp = *s; *s = g_carried_stack; g_carried_stack = tmp;
        }
    } else {
        if (stack_empty(&g_carried_stack)) {
            if (!stack_empty(s)) {
                int take = (s->count + 1) / 2;
                g_carried_stack = make_stack(s->id, take, s->damage);
                s->count -= take;
                if (s->count <= 0) stack_clear(s);
            }
        } else if (stack_empty(s)) {
            *s = make_stack(g_carried_stack.id, 1, g_carried_stack.damage);
            if (--g_carried_stack.count <= 0) stack_clear(&g_carried_stack);
        } else if (stack_same_item(s, &g_carried_stack) && s->count < stack_limit_for_id(s->id)) {
            s->count++;
            s->pop_time = 5;
            if (--g_carried_stack.count <= 0) stack_clear(&g_carried_stack);
        }
    }
}

static FlatRayHit flat_raycast(void) {
    FlatRayHit h; memset(&h, 0, sizeof(h)); h.face = 1;
    float dx, dy, dz;
    player_look_vector(&dx, &dy, &dz);
    float px = g_player_x;
    float py = g_player_y;
    float pz = g_player_z;
    int prev_bx = (int)floorf(px), prev_by = (int)floorf(py), prev_bz = (int)floorf(pz);
    for (float t = 0.0f; t <= 5.0f; t += 0.05f) {
        float x = px + dx * t;
        float y = py + dy * t;
        float z = pz + dz * t;
        int bx = (int)floorf(x);
        int by = (int)floorf(y);
        int bz = (int)floorf(z);
        if (flat_get_block(bx, by, bz) != 0) {
            h.hit = 1; h.bx = bx; h.by = by; h.bz = bz; h.hx = x; h.hy = y; h.hz = z;
            if (prev_by < by) h.face = 0;
            else if (prev_by > by) h.face = 1;
            else if (prev_bz < bz) h.face = 2;
            else if (prev_bz > bz) h.face = 3;
            else if (prev_bx < bx) h.face = 4;
            else if (prev_bx > bx) h.face = 5;
            return h;
        }
        prev_bx = bx; prev_by = by; prev_bz = bz;
    }
    return h;
}

static void start_air_swing_once(void) {
    if (g_air_swing_consumed || g_air_swing_playing) return;
    g_air_swing_consumed = 1;
    g_air_swing_playing = 1;
    g_air_swing_ticks = 0;
    g_prev_hand_swing = 0.0f;
    g_hand_swing = 0.0f;
    g_hand_swing_ticks = 0;
}

static void reset_breaking_state(void) {
    g_breaking_block = 0;
    g_break_damage = 0.0f;
    g_prev_break_damage = 0.0f;
    g_break_sound_counter = 0.0f;
}

static void break_target_block(void) {
    int id = flat_get_block(g_break_x, g_break_y, g_break_z);
    if (id == 0 || id == BLOCK_BEDROCK) return;
    flat_set_block(g_break_x, g_break_y, g_break_z, 0);
    int drop = block_drop_id(id);
    if (drop > 0) spawn_item_stack((float)g_break_x + 0.5f, (float)g_break_y + 0.7f, (float)g_break_z + 0.5f, make_stack(drop, 1, 0), 1);

    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    if (!stack_empty(held) && held->id == ITEM_STONE_SHOVEL &&
        (id == BLOCK_DIRT || id == BLOCK_GRASS)) {
        g_shovel_combo_count++;
    }
}

static void update_breaking(void) {
    g_prev_break_damage = g_break_damage;
    if (g_block_hit_delay > 0) g_block_hit_delay--;
    if (!key_down_vk(VK_LBUTTON)) { reset_breaking_state(); g_shovel_combo_count = 0; return; }
    FlatRayHit hit = flat_raycast();
    if (!hit.hit) { reset_breaking_state(); g_shovel_combo_count = 0; return; }
    g_break_swing_holding = 1;
    if (!g_hand_swing_active && g_hand_swing_ticks == 0) {
        trigger_hand_swing();
    }
    if (!g_breaking_block || hit.bx != g_break_x || hit.by != g_break_y || hit.bz != g_break_z) {
        g_breaking_block = 1;
        g_break_x = hit.bx; g_break_y = hit.by; g_break_z = hit.bz; g_break_face = hit.face;
        g_break_damage = g_prev_break_damage = 0.0f;
        g_break_sound_counter = 0.0f;
    }
    if (g_block_hit_delay > 0) return;
    int id = flat_get_block(g_break_x, g_break_y, g_break_z);
    if (id == 0 || id == BLOCK_BEDROCK) { reset_breaking_state(); return; }
    g_break_damage += block_relative_strength(id);
    g_break_sound_counter += 1.0f;
    if (g_break_damage >= 1.0f) {
        break_target_block();
        reset_breaking_state();
        g_block_hit_delay = 5; /* pv.java hit delay after block removal. */
    }
}

static void world_left_mouse_released(void) { reset_breaking_state(); g_break_swing_consumed = 0; g_break_swing_holding = 0; g_shovel_combo_count = 0; }

static void ingame_right_click(void) {
    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    if (stack_empty(held) || (held->id != BLOCK_GRASS && held->id != BLOCK_DIRT)) return;
    FlatRayHit hit = flat_raycast();
    if (!hit.hit) return;
    int px = hit.bx, py = hit.by, pz = hit.bz;
    if (hit.face == 0) py--; else if (hit.face == 1) py++; else if (hit.face == 2) pz--; else if (hit.face == 3) pz++; else if (hit.face == 4) px--; else if (hit.face == 5) px++;
    /* Beta-style use-on-block placement: place into the adjacent air cell. The flat
       test world remains 5x5 in X/Z, but it can now stack blocks above the built-in
       grass floor instead of only replacing holes in that floor. */
    if (!flat_in_bounds(px, py, pz) || flat_get_block(px, py, pz) != 0) return;
    if (flat_block_intersects_player(px, py, pz)) return;
    flat_set_block(px, py, pz, held->id);
    held->count--;
    if (held->count <= 0) stack_clear(held);
    trigger_hand_swing();
}

static void update_dropped_items(void) {
    for (int i = 0; i < MAX_DROP_ENTITIES; i++) {
        FlatDroppedItem *e = &g_drops[i];
        if (!e->active) continue;
        e->prev_x = e->x; e->prev_y = e->y; e->prev_z = e->z;
        if (e->pickup_delay > 0) e->pickup_delay--;

        /* Beta-like loose entity physics plus visible pickup pull.  The previous
           version teleported into the inventory as soon as it was close enough. */
        float target_x = g_player_x;
        float target_y = g_player_y - 1.20f;
        float target_z = g_player_z;
        float dx = target_x - e->x;
        float dy = target_y - e->y;
        float dz = target_z - e->z;
        float dist2 = dx*dx + dy*dy + dz*dz;
        if (e->pickup_delay <= 0 && dist2 < 6.25f && dist2 > 0.0001f) {
            float dist = sqrtf(dist2);
            float pull = (1.0f - dist / 2.5f);
            if (pull < 0.0f) pull = 0.0f;
            pull *= pull * 0.08f;
            e->mx += dx / dist * pull;
            e->my += dy / dist * pull;
            e->mz += dz / dist * pull;
        }

        e->my -= 0.04f;

        e->x += e->mx;
        if (flat_item_aabb_collides(e->x, e->y, e->z)) {
            e->x = e->prev_x;
            e->mx *= -0.5f;
        }

        e->z += e->mz;
        if (flat_item_aabb_collides(e->x, e->y, e->z)) {
            e->z = e->prev_z;
            e->mz *= -0.5f;
        }

        e->y += e->my;
        int on_ground = 0;
        if (flat_item_aabb_collides(e->x, e->y, e->z)) {
            e->y = e->prev_y;
            if (e->my < 0.0f) on_ground = 1;
            e->my *= -0.5f;
        }

        float friction = on_ground ? 0.58800006f : 0.98f;
        e->mx *= friction;
        e->my *= 0.98f;
        e->mz *= friction;
        e->age++;
        if (e->age >= 6000 || e->y < -24.0f) { e->active = 0; continue; }

        dx = target_x - e->x;
        dy = target_y - e->y;
        dz = target_z - e->z;
        dist2 = dx*dx + dy*dy + dz*dz;
        if (e->pickup_delay <= 0 && dist2 < 0.30f) {
            if (inventory_add_stack(e->stack)) e->active = 0;
        }
    }
}

static void reset_flat_player_spawn(void) {
    g_player_x = g_player_prev_x = 50.5f;
    g_player_y = g_player_prev_y = 5.62f;
    g_player_z = g_player_prev_z = 50.5f;
    g_player_motion_x = g_player_motion_y = g_player_motion_z = 0.0f;
    g_player_yaw = g_player_prev_yaw = 0.0f;
    g_player_pitch = g_player_prev_pitch = 0.0f;
    g_player_on_ground = 1;
    g_distance_walked = g_prev_distance_walked = 0.0f;
    g_camera_yaw = g_prev_camera_yaw = 0.0f;
    g_camera_pitch = g_prev_camera_pitch = 0.0f;
    g_hand_swing = g_prev_hand_swing = 0.0f;
    g_hand_swing_ticks = 0;
    g_hand_swing_progress = 0.0f;
    g_hand_swing_active = 0;
    g_air_swing_playing = 0;
    g_air_swing_ticks = 0;
    g_air_swing_consumed = 0;
    g_break_swing_consumed = 0;
    g_break_swing_holding = 0;
    g_shovel_combo_count = 0;
}

