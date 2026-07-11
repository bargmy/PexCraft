/* Minecraft 1.2.5-style achievement toast/tree and statistics screens.
   Included after item_render.c so the real item/block atlas renderer is available. */

static int g_pex_achievement_map_x = -82;
static int g_pex_achievement_map_y = -70;
static int g_pex_achievement_dragging = 0;
static int g_pex_achievement_drag_last_x = 0;
static int g_pex_achievement_drag_last_y = 0;
static int g_pex_statistics_tab = 0; /* 0 general, 1 blocks, 2 items */
static int g_pex_statistics_scroll = 0;
static int g_pex_statistics_sort_col[3] = {-1, -1, -1};
static int g_pex_statistics_sort_dir[3] = {0, 0, 0}; /* -1 descending, +1 ascending */
static int g_pex_statistics_dragging = 0;
static int g_pex_statistics_drag_last_y = 0;
static int g_pex_statistics_drag_pixels = 0;

typedef struct PexAchievementJavaRandom { unsigned long long seed; } PexAchievementJavaRandom;

static void pex_achievement_java_random_seed(PexAchievementJavaRandom *r, long long seed) {
    r->seed = ((unsigned long long)seed ^ 0x5DEECE66DULL) & ((1ULL << 48) - 1ULL);
}

static unsigned int pex_achievement_java_random_bits(PexAchievementJavaRandom *r, int bits) {
    r->seed = (r->seed * 0x5DEECE66DULL + 0xBULL) & ((1ULL << 48) - 1ULL);
    return (unsigned int)(r->seed >> (48 - bits));
}

static int pex_achievement_java_random_next_int(PexAchievementJavaRandom *r, int bound) {
    unsigned int bits, val;
    if (bound <= 1) return 0;
    if ((bound & -bound) == bound)
        return (int)(((unsigned long long)bound * pex_achievement_java_random_bits(r, 31)) >> 31);
    do {
        bits = pex_achievement_java_random_bits(r, 31);
        val = bits % (unsigned int)bound;
    } while ((int)(bits - val + (unsigned int)(bound - 1)) < 0);
    return (int)val;
}


static void pex_draw_achievement_terrain_tile(int tile, int x, int y, int tint) {
    float u0, v0, u1, v1;
    if (!tex_terrain.id) return;
    /* terrain.png can be HD and can also be expanded into StivuFine's CTM
       mega-atlas.  Use the terrain atlas resolver rather than the generic
       256x256 GUI-sheet mapper so the achievement background always samples
       the real base-pack block tile. */
    terrain_tile_uv(tile, &u0, &v0, &u1, &v1);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    set_color_int(tint);
    if (g_loggy_enabled) g_loggy_gui_quads++;
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v1); glVertex3f((float)x, (float)(y + 16), 0.0f);
    glTexCoord2f(u1, v1); glVertex3f((float)(x + 16), (float)(y + 16), 0.0f);
    glTexCoord2f(u1, v0); glVertex3f((float)(x + 16), (float)y, 0.0f);
    glTexCoord2f(u0, v0); glVertex3f((float)x, (float)y, 0.0f);
    glEnd();
    glColor4f(1,1,1,1);
}

static void pex_draw_achievement_terrain(int view_x, int view_y) {
    int map_x = g_pex_achievement_map_x;
    int map_y = g_pex_achievement_map_y;
    int base_x = (map_x + 288) >> 4;
    int base_y = (map_y + 288) >> 4;
    int off_x = (map_x + 288) % 16;
    int off_y = (map_y + 288) % 16;
    if (!tex_terrain.id) {
        draw_rect(view_x, view_y, view_x + 224, view_y + 155, 0xFF202020);
        return;
    }
    for (int row = 0; row * 16 - off_y < 155; ++row) {
        float brightness = 0.6f - (float)(base_y + row) / 25.0f * 0.3f;
        if (brightness < 0.0f) brightness = 0.0f;
        if (brightness > 1.0f) brightness = 1.0f;
        int shade = (int)(brightness * 255.0f + 0.5f);
        int tint = (int)(0xFF000000u | ((unsigned int)shade << 16) | ((unsigned int)shade << 8) | (unsigned int)shade);
        for (int col = 0; col * 16 - off_x < 224; ++col) {
            PexAchievementJavaRandom random;
            int depth, block_id = BLOCK_SAND;
            pex_achievement_java_random_seed(&random, 1234LL + base_x + col);
            (void)pex_achievement_java_random_bits(&random, 32);
            depth = pex_achievement_java_random_next_int(&random, 1 + base_y + row) + (base_y + row) / 2;
            if (depth <= 37 && base_y + row != 35) {
                if (depth == 22) block_id = pex_achievement_java_random_next_int(&random, 2) == 0 ? BLOCK_DIAMOND_ORE : BLOCK_REDSTONE_ORE;
                else if (depth == 10) block_id = BLOCK_IRON_ORE;
                else if (depth == 8) block_id = BLOCK_COAL_ORE;
                else if (depth > 4) block_id = BLOCK_STONE;
                else if (depth > 0) block_id = BLOCK_DIRT;
            } else block_id = BLOCK_BEDROCK;
            int tile = block_item_tile_for_id(block_id);
            pex_draw_achievement_terrain_tile(tile,
                                              view_x + col * 16 - off_x,
                                              view_y + row * 16 - off_y,
                                              tint);
        }
    }
}

static void pex_ui_draw_wrapped(const char *text, int x, int y, int max_width, int color, int max_lines) {
    char line[256];
    int line_len = 0;
    int lines = 0;
    const char *p = text ? text : "";
    while (*p && lines < max_lines) {
        while (*p == ' ') ++p;
        if (!*p) break;
        const char *word = p;
        while (*p && *p != ' ') ++p;
        int word_len = (int)(p - word);
        char trial[256];
        if (line_len > 0) snprintf(trial, sizeof(trial), "%s %.*s", line, word_len, word);
        else snprintf(trial, sizeof(trial), "%.*s", word_len, word);
        if (line_len > 0 && text_width(trial) > max_width) {
            draw_text(line, x, y + lines * 9, color);
            ++lines;
            line_len = 0;
            line[0] = 0;
            if (lines >= max_lines) break;
        }
        if (line_len > 0) {
            size_t have = strlen(line);
            if (have + 1 < sizeof(line)) line[have++] = ' ';
            int copy = word_len;
            if (have + (size_t)copy >= sizeof(line)) copy = (int)(sizeof(line) - have - 1);
            if (copy > 0) memcpy(line + have, word, (size_t)copy);
            line[have + (copy > 0 ? copy : 0)] = 0;
        } else {
            int copy = word_len < (int)sizeof(line) - 1 ? word_len : (int)sizeof(line) - 1;
            memcpy(line, word, (size_t)copy);
            line[copy] = 0;
        }
        line_len = (int)strlen(line);
    }
    if (line_len > 0 && lines < max_lines) draw_text(line, x, y + lines * 9, color);
}

static int pex_ui_wrapped_line_count(const char *text, int max_width, int max_lines) {
    char line[256];
    int line_len = 0;
    int lines = 0;
    const char *p = text ? text : "";
    line[0] = 0;
    while (*p && lines < max_lines) {
        while (*p == ' ') ++p;
        if (!*p) break;
        const char *word = p;
        while (*p && *p != ' ') ++p;
        int word_len = (int)(p - word);
        char trial[256];
        if (line_len > 0) snprintf(trial, sizeof(trial), "%s %.*s", line, word_len, word);
        else snprintf(trial, sizeof(trial), "%.*s", word_len, word);
        if (line_len > 0 && text_width(trial) > max_width) {
            ++lines;
            line_len = 0;
            line[0] = 0;
            if (lines >= max_lines) break;
        }
        if (line_len > 0) {
            size_t have = strlen(line);
            if (have + 1 < sizeof(line)) line[have++] = ' ';
            int copy = word_len;
            if (have + (size_t)copy >= sizeof(line)) copy = (int)(sizeof(line) - have - 1);
            if (copy > 0) memcpy(line + have, word, (size_t)copy);
            line[have + (copy > 0 ? copy : 0)] = 0;
        } else {
            int copy = word_len < (int)sizeof(line) - 1 ? word_len : (int)sizeof(line) - 1;
            memcpy(line, word, (size_t)copy);
            line[copy] = 0;
        }
        line_len = (int)strlen(line);
    }
    if (line_len > 0 && lines < max_lines) ++lines;
    return lines;
}

static void pex_ui_format_one(const char *fmt, const char *value, char *out, size_t cap) {
    const char *needle;
    size_t token_len = 4;
    if (!out || cap == 0) return;
    if (!fmt) fmt = "";
    if (!value) value = "";
    needle = strstr(fmt, "%1$s");
    if (!needle) { needle = strstr(fmt, "%s"); token_len = 2; }
    if (!needle) { snprintf(out, cap, "%s", fmt); return; }
    size_t left = (size_t)(needle - fmt);
    if (left >= cap) left = cap - 1;
    memcpy(out, fmt, left);
    out[left] = 0;
    snprintf(out + left, cap - left, "%s%s", value, needle + token_len);
}

static int pex_achievement_overlay_allowed(void) {
    switch (g_screen) {
        case SCREEN_INGAME: case SCREEN_PAUSE: case SCREEN_INVENTORY: case SCREEN_CREATIVE:
        case SCREEN_WORKBENCH: case SCREEN_FURNACE: case SCREEN_CHEST: case SCREEN_CHAT:
        case SCREEN_DEATH: return 1;
        default: return 0;
    }
}

static void pex_draw_achievement_panel(int id, int info_mode, int y) {
    int x = g_gui_w - 160;
    char desc[512];
    ItemStack icon;
    if (id < 0 || id >= PEX_ACHIEVEMENT_COUNT) return;
    glColor4f(1,1,1,1);
    if (tex_achievement.id && tex_achievement.w >= 256 && tex_achievement.h >= 234)
        draw_textured_rect_tex(&tex_achievement, x, y, 96, 202, 160, 32, 0xFFFFFF);
    else {
        draw_rect(x, y, x + 160, y + 32, 0xE0101010);
        draw_rect(x, y, x + 160, y + 1, 0xFFFFFFFF);
        draw_rect(x, y + 31, x + 160, y + 32, 0xFF555555);
    }
    if (info_mode) {
        pex_achievement_description(id, desc, sizeof(desc));
        pex_ui_draw_wrapped(desc, x + 30, y + 7, 120, 0xFFFFFF, 2);
    } else {
        draw_text(tr_key_default("achievement.get", "Achievement get!"), x + 30, y + 7, 0xFFFF00);
        draw_text(pex_achievement_title(id), x + 30, y + 18, 0xFFFFFF);
    }
    icon = make_stack(g_pex_achievement_defs[id].icon_id, 1, 0);
    draw_item_stack_gui(&icon, x + 8, y + 8);
}

static void pex_draw_achievement_toast(void) {
    if (!pex_achievement_overlay_allowed()) return;
    pex_stats_ensure_loaded();
    pex_achievement_toast_tick();
    if (g_pex_achievement_toast_id >= 0) {
        double t = (now_seconds() - g_pex_achievement_toast_started) / 3.0;
        double e = t * 2.0;
        if (e > 1.0) e = 2.0 - e;
        e *= 4.0;
        e = 1.0 - e;
        if (e < 0.0) e = 0.0;
        e *= e;
        e *= e;
        pex_draw_achievement_panel(g_pex_achievement_toast_id, 0, -(int)(e * 36.0));
    } else if (!pex_achievement_unlocked(PEX_ACH_OPEN_INVENTORY)) {
        /* GuiAchievement.queueAchievementInformation is refreshed every tick in
           EntityPlayerSP, leaving this tutorial panel parked at y=0. */
        pex_draw_achievement_panel(PEX_ACH_OPEN_INVENTORY, 1, 0);
    }
}

static void pex_achievement_map_clamp(void) {
    int min_col = g_pex_achievement_defs[0].display_col;
    int max_col = min_col;
    int min_row = g_pex_achievement_defs[0].display_row;
    int max_row = min_row;
    for (int id = 1; id < PEX_ACHIEVEMENT_COUNT; ++id) {
        if (g_pex_achievement_defs[id].display_col < min_col) min_col = g_pex_achievement_defs[id].display_col;
        if (g_pex_achievement_defs[id].display_col > max_col) max_col = g_pex_achievement_defs[id].display_col;
        if (g_pex_achievement_defs[id].display_row < min_row) min_row = g_pex_achievement_defs[id].display_row;
        if (g_pex_achievement_defs[id].display_row > max_row) max_row = g_pex_achievement_defs[id].display_row;
    }
    /* Exact GuiAchievements bounds: min * 24 - 112, max * 24 - 77. */
    int min_x = min_col * 24 - 112;
    int max_x = max_col * 24 - 77;
    int min_y = min_row * 24 - 112;
    int max_y = max_row * 24 - 77;
    if (g_pex_achievement_map_x < min_x) g_pex_achievement_map_x = min_x;
    if (g_pex_achievement_map_x > max_x) g_pex_achievement_map_x = max_x;
    if (g_pex_achievement_map_y < min_y) g_pex_achievement_map_y = min_y;
    if (g_pex_achievement_map_y > max_y) g_pex_achievement_map_y = max_y;
}

static void pex_achievements_mouse_down(int mx, int my) {
    int pane_x = g_gui_w / 2 - 128, pane_y = g_gui_h / 2 - 101;
    if (mx >= pane_x + 8 && mx < pane_x + 232 && my >= pane_y + 17 && my < pane_y + 172) {
        g_pex_achievement_dragging = 1;
        g_pex_achievement_drag_last_x = mx;
        g_pex_achievement_drag_last_y = my;
    }
}

static void pex_achievements_mouse_drag(int mx, int my) {
    if (!g_pex_achievement_dragging) return;
    g_pex_achievement_map_x -= mx - g_pex_achievement_drag_last_x;
    g_pex_achievement_map_y -= my - g_pex_achievement_drag_last_y;
    g_pex_achievement_drag_last_x = mx;
    g_pex_achievement_drag_last_y = my;
    pex_achievement_map_clamp();
}

static void pex_achievements_mouse_up(void) { g_pex_achievement_dragging = 0; }

static void pex_statistics_set_tab(int tab) {
    if (tab < 0) tab = 0;
    if (tab > 2) tab = 2;
    g_pex_statistics_tab = tab;
    g_pex_statistics_scroll = 0;
}

static int pex_statistics_object_present(int tab, int id) {
    if (id <= 0 || id >= PEX_STAT_OBJECT_MAX) return 0;
    if (tab == 1)
        return id < 256 && (g_pex_profile_stats.mined[id] || g_pex_profile_stats.crafted[id] ||
                            g_pex_profile_stats.used[id] || g_pex_profile_stats.picked[id]);
    if (tab == 2)
        return id >= 256 && (g_pex_profile_stats.crafted[id] || g_pex_profile_stats.used[id] ||
                             g_pex_profile_stats.broken[id] || g_pex_profile_stats.picked[id]);
    return 0;
}

static long long pex_statistics_object_sort_value(int tab, int col, int id) {
    if (tab == 1) {
        if (col == 0) return g_pex_profile_stats.crafted[id];
        if (col == 1) return g_pex_profile_stats.used[id];
        return g_pex_profile_stats.mined[id];
    }
    if (col == 0) return g_pex_profile_stats.broken[id];
    if (col == 1) return g_pex_profile_stats.crafted[id];
    return g_pex_profile_stats.used[id];
}

static int pex_statistics_collect_object_ids(int tab, int *out, int cap) {
    int count = 0;
    int first = tab == 1 ? 1 : 256;
    int last = tab == 1 ? 256 : PEX_STAT_OBJECT_MAX;
    for (int id = first; id < last && count < cap; ++id)
        if (pex_statistics_object_present(tab, id)) out[count++] = id;

    int col = g_pex_statistics_sort_col[tab];
    int dir = g_pex_statistics_sort_dir[tab];
    if (col >= 0 && dir != 0) {
        /* Stable insertion sort mirrors the tiny Java Collections.sort lists and
           keeps item ID as the deterministic tiebreaker. */
        for (int i = 1; i < count; ++i) {
            int id = out[i], j = i - 1;
            long long value = pex_statistics_object_sort_value(tab, col, id);
            while (j >= 0) {
                int other = out[j];
                long long ov = pex_statistics_object_sort_value(tab, col, other);
                int before = dir < 0 ? (value > ov) : (value < ov);
                if (value == ov) before = id < other;
                if (!before) break;
                out[j + 1] = out[j];
                --j;
            }
            out[j + 1] = id;
        }
    }
    return count;
}

static int pex_statistics_row_count(void) {
    int ids[PEX_STAT_OBJECT_MAX];
    pex_stats_ensure_loaded();
    if (g_pex_statistics_tab == 0) return PEX_GENERAL_STAT_COUNT;
    return pex_statistics_collect_object_ids(g_pex_statistics_tab, ids, PEX_STAT_OBJECT_MAX);
}

static int pex_statistics_visible_rows(void) {
    int top = g_pex_statistics_tab == 0 ? 32 : 52;
    int row_h = g_pex_statistics_tab == 0 ? 10 : 20;
    int bottom = g_gui_h - 64;
    int visible = (bottom - top) / row_h;
    return visible > 0 ? visible : 1;
}

static void pex_statistics_scroll_by(int rows) {
    int max_scroll = pex_statistics_row_count() - pex_statistics_visible_rows();
    if (max_scroll < 0) max_scroll = 0;
    g_pex_statistics_scroll += rows;
    if (g_pex_statistics_scroll < 0) g_pex_statistics_scroll = 0;
    if (g_pex_statistics_scroll > max_scroll) g_pex_statistics_scroll = max_scroll;
}

static const char *pex_statistics_header_key(int tab, int col) {
    if (tab == 1) return col == 0 ? "stat.crafted" : (col == 1 ? "stat.used" : "stat.mined");
    return col == 0 ? "stat.depleted" : (col == 1 ? "stat.crafted" : "stat.used");
}

static const char *pex_statistics_header_fallback(int tab, int col) {
    if (tab == 1) return col == 0 ? "Crafted" : (col == 1 ? "Used" : "Mined");
    return col == 0 ? "Depleted" : (col == 1 ? "Crafted" : "Used");
}

static void pex_statistics_mouse_down(int mx, int my) {
    int top = g_pex_statistics_tab == 0 ? 32 : 52;
    int bottom = g_gui_h - 64;
    if (g_pex_statistics_tab != 0) {
        int base_x = g_gui_w / 2 - 108;
        int header_y = 32;
        int col = -1;
        if (my >= header_y + 1 && my < header_y + 19) {
            if (mx >= base_x + 97 && mx < base_x + 115) col = 0;
            else if (mx >= base_x + 147 && mx < base_x + 165) col = 1;
            else if (mx >= base_x + 197 && mx < base_x + 215) col = 2;
        }
        if (col >= 0) {
            if (g_pex_statistics_sort_col[g_pex_statistics_tab] != col) {
                g_pex_statistics_sort_col[g_pex_statistics_tab] = col;
                g_pex_statistics_sort_dir[g_pex_statistics_tab] = -1;
            } else if (g_pex_statistics_sort_dir[g_pex_statistics_tab] == -1) {
                g_pex_statistics_sort_dir[g_pex_statistics_tab] = 1;
            } else {
                g_pex_statistics_sort_col[g_pex_statistics_tab] = -1;
                g_pex_statistics_sort_dir[g_pex_statistics_tab] = 0;
            }
            g_pex_statistics_scroll = 0;
            pex_sound_play("random.click", 1.0f, 1.0f);
            return;
        }
    }
    if (my >= top && my < bottom) {
        g_pex_statistics_dragging = 1;
        g_pex_statistics_drag_last_y = my;
        g_pex_statistics_drag_pixels = 0;
    }
}

static void pex_statistics_mouse_drag(int mx, int my) {
    int row_h = g_pex_statistics_tab == 0 ? 10 : 20;
    int delta;
    (void)mx;
    if (!g_pex_statistics_dragging) return;
    delta = my - g_pex_statistics_drag_last_y;
    g_pex_statistics_drag_last_y = my;
    g_pex_statistics_drag_pixels -= delta;
    while (g_pex_statistics_drag_pixels >= row_h) {
        pex_statistics_scroll_by(1);
        g_pex_statistics_drag_pixels -= row_h;
    }
    while (g_pex_statistics_drag_pixels <= -row_h) {
        pex_statistics_scroll_by(-1);
        g_pex_statistics_drag_pixels += row_h;
    }
}

static void pex_statistics_mouse_up(void) {
    g_pex_statistics_dragging = 0;
    g_pex_statistics_drag_pixels = 0;
}

static void pex_draw_achievement_connector(int child_x, int child_y, int parent_x, int parent_y, int unlocked, int can_unlock) {
    int pulse = sin(now_seconds() / 0.6 * 6.283185307179586) > 0.6 ? 255 : 130;
    int color = 0xFF000000;
    int hx0 = child_x < parent_x ? child_x : parent_x;
    int hx1 = child_x > parent_x ? child_x : parent_x;
    int vy0 = child_y < parent_y ? child_y : parent_y;
    int vy1 = child_y > parent_y ? child_y : parent_y;
    if (unlocked) color = 0xFF707070;
    else if (can_unlock) color = (int)(((unsigned int)pulse << 24) | 0x0000FF00u);
    /* GuiAchievements draws child-to-parent horizontally at the child's row,
       then vertically at the parent's column. */
    draw_rect(hx0, child_y, hx1 + 1, child_y + 1, color);
    draw_rect(parent_x, vy0, parent_x + 1, vy1 + 1, color);
}

static void draw_achievements_screen(void) {
    int pane_x = g_gui_w / 2 - 128, pane_y = g_gui_h / 2 - 101;
    int view_x = pane_x + 16, view_y = pane_y + 17;
    int hover = -1;
    pex_stats_ensure_loaded();
    draw_default_bg();
    pex_draw_achievement_terrain(view_x, view_y);

    for (int id = 0; id < PEX_ACHIEVEMENT_COUNT; ++id) {
        int parent = g_pex_achievement_defs[id].parent;
        if (parent < 0) continue;
        int x = view_x + g_pex_achievement_defs[id].display_col * 24 - g_pex_achievement_map_x + 11;
        int y = view_y + g_pex_achievement_defs[id].display_row * 24 - g_pex_achievement_map_y + 11;
        int px = view_x + g_pex_achievement_defs[parent].display_col * 24 - g_pex_achievement_map_x + 11;
        int py = view_y + g_pex_achievement_defs[parent].display_row * 24 - g_pex_achievement_map_y + 11;
        if ((x >= view_x - 24 && x <= view_x + 248) || (px >= view_x - 24 && px <= view_x + 248))
            pex_draw_achievement_connector(x, y, px, py, pex_achievement_unlocked(id), pex_achievement_can_unlock(id));
    }

    for (int id = 0; id < PEX_ACHIEVEMENT_COUNT; ++id) {
        int x = view_x + g_pex_achievement_defs[id].display_col * 24 - g_pex_achievement_map_x;
        int y = view_y + g_pex_achievement_defs[id].display_row * 24 - g_pex_achievement_map_y;
        int unlocked = pex_achievement_unlocked(id);
        int can = pex_achievement_can_unlock(id);
        if (x < view_x - 24 || y < view_y - 24 || x > view_x + 224 || y > view_y + 155) continue;
        float pulse_brightness = sin(now_seconds() / 0.6 * 6.283185307179586) < 0.6 ? 0.6f : 0.8f;
        int shade = unlocked ? 255 : (can ? (int)(pulse_brightness * 255.0f) : 77);
        int tint = (shade << 16) | (shade << 8) | shade;
        if (tex_achievement.id && tex_achievement.h >= 228)
            draw_textured_rect_tex(&tex_achievement, x - 2, y - 2, g_pex_achievement_defs[id].special ? 26 : 0, 202, 26, 26, tint);
        else {
            draw_rect(x - 2, y - 2, x + 24, y + 24, 0xFF000000 | tint);
            draw_rect(x, y, x + 22, y + 22, 0xFF303030);
        }
        ItemStack icon = make_stack(g_pex_achievement_defs[id].icon_id, 1, 0);
        draw_item_stack_gui(&icon, x + 3, y + 3);
        if (!can) draw_rect(x + 3, y + 3, x + 19, y + 19, 0xC0000000);
        if (g_mouse_x >= view_x && g_mouse_x < view_x + 224 && g_mouse_y >= view_y && g_mouse_y < view_y + 155 &&
            g_mouse_x >= x && g_mouse_x <= x + 22 && g_mouse_y >= y && g_mouse_y <= y + 22) hover = id;
    }

    if (tex_achievement.id && tex_achievement.w >= 256 && tex_achievement.h >= 202)
        draw_textured_rect_tex(&tex_achievement, pane_x, pane_y, 0, 0, 256, 202, 0xFFFFFF);
    else {
        draw_rect(pane_x, pane_y, pane_x + 256, pane_y + 17, 0xFFC6C6C6);
        draw_rect(pane_x, pane_y + 172, pane_x + 256, pane_y + 202, 0xFFC6C6C6);
        draw_rect(pane_x, pane_y, pane_x + 16, pane_y + 202, 0xFFC6C6C6);
        draw_rect(pane_x + 240, pane_y, pane_x + 256, pane_y + 202, 0xFFC6C6C6);
    }
    draw_text(tr_key_default("gui.achievements", "Achievements"), pane_x + 15, pane_y + 5, 0x404040);
    draw_all_buttons();

    if (hover >= 0) {
        char desc[512], req[512];
        int tx = g_mouse_x + 12, ty = g_mouse_y - 4;
        int width = text_width(pex_achievement_title(hover));
        int can = pex_achievement_can_unlock(hover);
        int unlocked = pex_achievement_unlocked(hover);
        int special = g_pex_achievement_defs[hover].special;
        int lines, text_height, box_bottom;
        if (width < 120) width = 120;
        pex_achievement_description(hover, desc, sizeof(desc));
        if (!can && g_pex_achievement_defs[hover].parent >= 0) {
            pex_ui_format_one(tr_key_default("achievement.requires", "Requires '%1$s'"),
                              pex_achievement_title(g_pex_achievement_defs[hover].parent), req, sizeof(req));
            snprintf(desc, sizeof(desc), "%s", req);
        }
        lines = pex_ui_wrapped_line_count(desc, width, 12);
        if (lines < 1) lines = 1;
        text_height = lines * 9;
        box_bottom = ty + text_height + 12 + 3 + (can && unlocked ? 12 : 0);
        draw_rect(tx - 3, ty - 3, tx + width + 3, box_bottom, 0xC0000000);
        draw_text(pex_achievement_title(hover), tx, ty,
                  can ? (special ? 0xFFFF80 : 0xFFFFFF) : (special ? 0x808040 : 0x808080));
        pex_ui_draw_wrapped(desc, tx, ty + 12, width, can ? 0xA0A0A0 : 0x705050, 12);
        if (can && unlocked)
            draw_text(tr_key_default("achievement.taken", "Taken!"), tx, ty + text_height + 4, 0x9090FF);
    }
}

static void pex_draw_stat_slot(int x, int y, int id) {
    if (tex_stat_slot.id) draw_textured_rect_tex(&tex_stat_slot, x, y, 0, 0, 18, 18, 0xFFFFFF);
    else {
        draw_rect(x, y, x + 18, y + 18, 0xFF555555);
        draw_rect(x + 1, y + 1, x + 17, y + 17, 0xFFC6C6C6);
    }
    ItemStack st = make_stack(id, 1, 0);
    draw_item_stack_gui(&st, x + 1, y + 1);
}

static void pex_draw_statistics_tooltip(const char *text) {
    if (!text || !text[0]) return;
    int x = g_mouse_x + 12, y = g_mouse_y - 12;
    int w = text_width(text);
    draw_rect(x - 3, y - 3, x + w + 3, y + 11, 0xC0000000);
    draw_text(text, x, y, 0xFFFFFF);
}

static void pex_draw_statistics_header(int base_x, int y, int tab, int *hover_col) {
    static const int xoff[3] = {97, 147, 197};
    static const int block_icon_u[3] = {18, 36, 54};
    static const int item_icon_u[3] = {72, 18, 36};
    for (int col = 0; col < 3; ++col) {
        int x = base_x + xoff[col];
        int hover = g_mouse_x >= x && g_mouse_x < x + 18 && g_mouse_y >= y + 1 && g_mouse_y < y + 19;
        int icon_u = tab == 1 ? block_icon_u[col] : item_icon_u[col];
        if (hover && hover_col) *hover_col = col;
        if (tex_stat_slot.id) {
            draw_textured_rect_tex(&tex_stat_slot, x, y + 1, 0, hover ? 0 : 18, 18, 18, 0xFFFFFF);
            draw_textured_rect_tex(&tex_stat_slot, x + (hover ? 1 : 0), y + 1 + (hover ? 1 : 0), icon_u, 18, 18, 18, 0xFFFFFF);
            if (g_pex_statistics_sort_col[tab] == col) {
                int arrow_u = g_pex_statistics_sort_dir[tab] == 1 ? 36 : 18;
                draw_textured_rect_tex(&tex_stat_slot, x - 18, y + 1, arrow_u, 0, 18, 18, 0xFFFFFF);
            }
        } else {
            draw_rect(x, y + 1, x + 18, y + 19, hover ? 0xFF909090 : 0xFF606060);
            draw_text(col == 0 ? "1" : (col == 1 ? "2" : "3"), x + 6, y + 6, 0xFFFFFF);
        }
    }
}

static void draw_statistics_screen(void) {
    int bottom = g_gui_h - 64;
    int base_x = g_gui_w / 2 - 108;
    int hover_id = -1, hover_col = -1;
    pex_stats_ensure_loaded();
    draw_default_bg();
    draw_centered_text(tr_key_default("gui.stats", "Statistics"), g_gui_w / 2, 20, 0xFFFFFF);

    if (g_pex_statistics_tab == 0) {
        int top = 32, row_h = 10;
        int visible = (bottom - top) / row_h;
        draw_gradient(0, top - 4, g_gui_w, top + 4, 0x00000000, 0xFF000000);
        draw_gradient(0, bottom - 4, g_gui_w, bottom + 4, 0xFF000000, 0x00000000);
        for (int row = 0; row < visible; ++row) {
            int id = g_pex_statistics_scroll + row;
            if (id >= PEX_GENERAL_STAT_COUNT) break;
            int y = top + row * row_h;
            int color = (id & 1) ? 0x909090 : 0xFFFFFF;
            char value[64];
            draw_text(tr_key_default(g_pex_general_stat_defs[id].key, g_pex_general_stat_defs[id].fallback), base_x + 2, y + 1, color);
            pex_stats_format_value((PexGeneralStat)id, g_pex_profile_stats.general[id], value, sizeof(value));
            draw_text(value, base_x + 215 - text_width(value), y + 1, color);
        }
    } else {
        int ids[PEX_STAT_OBJECT_MAX];
        int count = pex_statistics_collect_object_ids(g_pex_statistics_tab, ids, PEX_STAT_OBJECT_MAX);
        int header_y = 32, top = 52, row_h = 20;
        int visible = (bottom - top) / row_h;
        pex_draw_statistics_header(base_x, header_y, g_pex_statistics_tab, &hover_col);
        draw_gradient(0, top - 4, g_gui_w, top + 4, 0x00000000, 0xFF000000);
        draw_gradient(0, bottom - 4, g_gui_w, bottom + 4, 0xFF000000, 0x00000000);
        for (int row = 0; row < visible; ++row) {
            int list_index = g_pex_statistics_scroll + row;
            if (list_index >= count) break;
            int id = ids[list_index];
            int y = top + row * row_h;
            int color = (list_index & 1) ? 0x909090 : 0xFFFFFF;
            char a[32], b[32], c[32];
            pex_draw_stat_slot(base_x + 40, y, id);
            if (g_mouse_x >= base_x + 40 && g_mouse_x < base_x + 60 && g_mouse_y >= y && g_mouse_y < y + 20) hover_id = id;
            if (g_pex_statistics_tab == 1) {
                snprintf(a, sizeof(a), "%lld", g_pex_profile_stats.crafted[id]);
                snprintf(b, sizeof(b), "%lld", g_pex_profile_stats.used[id]);
                snprintf(c, sizeof(c), "%lld", g_pex_profile_stats.mined[id]);
            } else {
                snprintf(a, sizeof(a), "%lld", g_pex_profile_stats.broken[id]);
                snprintf(b, sizeof(b), "%lld", g_pex_profile_stats.crafted[id]);
                snprintf(c, sizeof(c), "%lld", g_pex_profile_stats.used[id]);
            }
            draw_text(a, base_x + 115 - text_width(a), y + 5, color);
            draw_text(b, base_x + 165 - text_width(b), y + 5, color);
            draw_text(c, base_x + 215 - text_width(c), y + 5, color);
        }
    }
    draw_all_buttons();
    if (hover_id >= 0) {
        ItemStack st = make_stack(hover_id, 1, 0);
        pex_draw_statistics_tooltip(item_stack_display_name(&st));
    } else if (hover_col >= 0) {
        pex_draw_statistics_tooltip(tr_key_default(pex_statistics_header_key(g_pex_statistics_tab, hover_col),
                                                    pex_statistics_header_fallback(g_pex_statistics_tab, hover_col)));
    }
}
