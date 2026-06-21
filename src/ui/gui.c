/* Split from original monolithic main.c. Included by src/main.c unity build. */


typedef struct PexProcessMemoryCounters {
    DWORD cb;
    DWORD PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} PexProcessMemoryCounters;

typedef BOOL (WINAPI *PexGetProcessMemoryInfoFn)(HANDLE, PexProcessMemoryCounters*, DWORD);

static void get_app_memory_mb(unsigned long long *used_mb, unsigned long long *peak_mb) {
    *used_mb = 0;
    *peak_mb = 0;

    HMODULE psapi = LoadLibraryA("psapi.dll");
    if (!psapi) psapi = LoadLibraryA("kernel32.dll");
    if (!psapi) return;

    PexGetProcessMemoryInfoFn fn =
        (PexGetProcessMemoryInfoFn)GetProcAddress(psapi, "GetProcessMemoryInfo");
    if (!fn) return;

    PexProcessMemoryCounters pmc;
    memset(&pmc, 0, sizeof(pmc));
    pmc.cb = sizeof(pmc);
    if (fn(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        *used_mb = (unsigned long long)(pmc.WorkingSetSize / (1024ULL * 1024ULL));
        *peak_mb = (unsigned long long)(pmc.PeakWorkingSetSize / (1024ULL * 1024ULL));
    }
}



static void draw_save_message(void) {
    if (g_save_message_ticks <= 0) return;
    draw_text("Saving game...", 8, g_gui_h - 16, 16777215);
}

static void draw_hud(void) {
    int w = g_gui_w;
    int h = g_gui_h;
    int hotbar_x = w / 2 - 91;
    int hotbar_y = h - 22;
    draw_textured_rect_tex(&tex_gui, hotbar_x, hotbar_y, 0, 0, 182, 22, 0xFFFFFF);
    draw_textured_rect_tex(&tex_gui, hotbar_x - 1 + g_selected_hotbar_slot * 20, hotbar_y - 1, 0, 22, 24, 22, 0xFFFFFF);
    for (int i = 0; i < 9; i++) draw_item_stack_gui(&g_inventory[i], hotbar_x + 3 + i * 20, hotbar_y + 3);
    draw_textured_rect_tex(&tex_icons, w / 2 - 7, h / 2 - 7, 0, 0, 16, 16, 0xFFFFFF);

    int hp = g_player_health;
    if (hp < 0) hp = 0;
    if (hp > 20) hp = 20;
    int prev_hp = g_player_prev_health;
    if (prev_hp < 0) prev_hp = 0;
    if (prev_hp > 20) prev_hp = 20;
    int flash_old = (g_hearts_life > g_ingame_ticks) && (((g_hearts_life - g_ingame_ticks) / 3) & 1);
    for (int i = 0; i < 10; i++) {
        int x = w / 2 - 91 + i * 8;
        int y = h - 32;
        if (hp <= 4 && hp > 0) y += rand() & 1;
        draw_textured_rect_tex(&tex_icons, x, y, 16, 0, 9, 9, 0xFFFFFF);
        if (flash_old) {
            if (i * 2 + 1 < prev_hp) draw_textured_rect_tex(&tex_icons, x, y, 70, 0, 9, 9, 0xFFFFFF);
            else if (i * 2 + 1 == prev_hp) draw_textured_rect_tex(&tex_icons, x, y, 79, 0, 9, 9, 0xFFFFFF);
        }
        if (i * 2 + 1 < hp) draw_textured_rect_tex(&tex_icons, x, y, 52, 0, 9, 9, 0xFFFFFF);
        else if (i * 2 + 1 == hp) draw_textured_rect_tex(&tex_icons, x, y, 61, 0, 9, 9, 0xFFFFFF);
    }
    if (g_player_armor > 0) {
        int armor = g_player_armor;
        if (armor > 20) armor = 20;
        for (int i = 0; i < 10; i++) {
            int x = w / 2 + 91 - i * 8 - 9;
            int y = h - 32;
            if (i * 2 + 1 < armor) draw_textured_rect_tex(&tex_icons, x, y, 34, 9, 9, 9, 0xFFFFFF);
            else if (i * 2 + 1 == armor) draw_textured_rect_tex(&tex_icons, x, y, 25, 9, 9, 9, 0xFFFFFF);
            else draw_textured_rect_tex(&tex_icons, x, y, 16, 9, 9, 9, 0xFFFFFF);
        }
    }
    draw_text(VERSION_TEXT, 2, 2, 16777215);
    if (g_debug_menu_shown) {
        char line[160];
        int fps = g_debug_fps;
        int min_fps = g_debug_min_fps ? g_debug_min_fps : fps;
        int max_fps = g_debug_max_fps ? g_debug_max_fps : fps;

        int y = g_opts.show_fps ? 22 : 12;
        snprintf(line, sizeof(line), "FPS %d/%d/%d", fps, min_fps, max_fps);
        draw_text(line, 2, y, 14737632); y += 10;

        snprintf(line, sizeof(line), "Position %.2f %.2f %.2f", g_player_x, g_player_y, g_player_z);
        draw_text(line, 2, y, 14737632); y += 10;

        unsigned long long used_mb = 0, peak_mb = 0;
        get_app_memory_mb(&used_mb, &peak_mb);
        snprintf(line, sizeof(line), "Memory usage %lluMB/%lluMB", used_mb, peak_mb);
        draw_text(line, 2, y, 14737632); y += 10;

        snprintf(line, sizeof(line), "Frame %.1fms avg %.1fms p50 %.1f p95 %.1f p99 %.1f",
                 g_ft_last_frame_ms, g_prof_display_frame_ms, g_ft_p50_ms, g_ft_p95_ms, g_ft_p99_ms);
        draw_text(line, 2, y, 14737632); y += 10;

        snprintf(line, sizeof(line), "Worst %.1fms  Backend %s", g_ft_worst_ms, renderer_backend_label(g_runtime_renderer_backend));
        draw_text(line, 2, y, 14737632); y += 10;

        if (g_opts.max_fps > 0) {
            snprintf(line, sizeof(line), "Target %.1fms (%dfps cap)  Sleep %.1fms",
                     1000.0 / (double)g_opts.max_fps, g_opts.max_fps, g_sleep_ms_last);
        } else {
            snprintf(line, sizeof(line), "Target uncapped  Sleep %.1fms", g_sleep_ms_last);
        }
        draw_text(line, 2, y, 14737632); y += 12;

        draw_text("Main thread average per frame:", 2, y, 0xE0E0E0); y += 10;
        for (int pi = 0; pi < PROF_COUNT; pi++) {
            double ms = g_prof_display_ms[pi];
            if (ms < 0.01 && pi != PROF_PUMP && pi != PROF_NET_POLL && pi != PROF_TICK_TOTAL && pi != PROF_RENDER_TOTAL) continue;
            snprintf(line, sizeof(line), "%-16s %5.2fms %4.0f%%", g_prof_names[pi], ms, pex_profile_pct(ms));
            draw_text(line, 2, y, 14737632);
            y += 10;
            if (y > g_gui_h - 70) break;
        }

        int right_x = g_gui_w - 210;
        int ry = g_opts.show_fps ? 22 : 12;
        if (right_x < 220) right_x = 220;
        draw_text("Queues / async state:", right_x, ry, 0xE0E0E0); ry += 10;
        snprintf(line, sizeof(line), "Net packets %d  chunks %d  queued %d", g_prof_packets_last, g_prof_chunks_last, g_prof_rx_queue_last);
        draw_text(line, right_x, ry, 14737632); ry += 10;
        snprintf(line, sizeof(line), "Mesh jobs %d  uploads %d  results %d", g_prof_mesh_jobs_last, g_prof_mesh_uploads_last, g_prof_mesh_results_last);
        draw_text(line, right_x, ry, 14737632); ry += 10;
        snprintf(line, sizeof(line), "Falling active %d  wakeups %d  spawned %d", g_prof_falling_active_last, g_prof_falling_cells_last, g_prof_falling_spawns_last);
        draw_text(line, right_x, ry, 14737632); ry += 10;
        snprintf(line, sizeof(line), "Stream pending %d", g_prof_stream_pending_last);
        draw_text(line, right_x, ry, 14737632);
    }
    draw_chat_lines(g_screen == SCREEN_CHAT);
    draw_save_message();
}

static void draw_ingame_screen(void) {
    draw_ingame_world_view(1);
    double prof_gui = pex_profile_begin();
    draw_hud();
    pex_profile_add(PROF_HUD_GUI, prof_gui);
}

static void draw_pause_screen(void) {
    draw_ingame_world_view(1);
    double prof_gui = pex_profile_begin();
    draw_gradient(0, 0, g_gui_w, g_gui_h, -1072689136, -804253680);
    draw_save_message();
    draw_centered_text("Game menu", g_gui_w / 2, 40, 16777215);
    draw_all_buttons();
    pex_profile_add(PROF_HUD_GUI, prof_gui);
}

static float g_steve_uv_w = 64.0f;
static float g_steve_uv_h = 32.0f;
static float g_steve_tint_r = 1.0f;
static float g_steve_tint_g = 1.0f;
static float g_steve_tint_b = 1.0f;

static void steve_set_texture_dims(const Texture *tex) {
    g_steve_uv_w = (tex && tex->w > 0) ? (float)tex->w : 64.0f;
    g_steve_uv_h = (tex && tex->h > 0) ? (float)tex->h : 32.0f;
}

static void steve_set_tint(float r, float g, float b) {
    g_steve_tint_r = r;
    g_steve_tint_g = g;
    g_steve_tint_b = b;
}

static void steve_color_for_normal(float nx, float ny, float nz) {
    float shade = 0.82f;
    if (ny < -0.5f) shade = 1.0f;
    else if (ny > 0.5f) shade = 0.55f;
    else if (nz > 0.5f) shade = 0.92f;
    else if (nz < -0.5f) shade = 0.70f;
    else if (nx != 0.0f) shade = 0.76f;
    glColor4f(shade * g_steve_tint_r, shade * g_steve_tint_g, shade * g_steve_tint_b, 1.0f);
}

static void steve_vertex(float x, float y, float z, float u, float v) {
    glTexCoord2f(u / g_steve_uv_w, v / g_steve_uv_h);
    glVertex3f(x * 0.0625f, y * 0.0625f, z * 0.0625f);
}

static void steve_quad(float ax, float ay, float az,
                       float bx, float by, float bz,
                       float cx, float cy, float cz,
                       float dx, float dy, float dz,
                       float u0, float v0, float u1, float v1,
                       float nx, float ny, float nz) {
    steve_color_for_normal(nx, ny, nz);
    steve_vertex(ax, ay, az, u1, v0);
    steve_vertex(bx, by, bz, u0, v0);
    steve_vertex(cx, cy, cz, u0, v1);
    steve_vertex(dx, dy, dz, u1, v1);
}

static void steve_box(int tex_x, int tex_y, float x, float y, float z,
                      int w, int h, int d, float inflate, int mirror) {
    float x0 = x - inflate;
    float y0 = y - inflate;
    float z0 = z - inflate;
    float x1 = x + (float)w + inflate;
    float y1 = y + (float)h + inflate;
    float z1 = z + (float)d + inflate;
    if (mirror) {
        float t = x1;
        x1 = x0;
        x0 = t;
    }

    /* Texture layout copied from Beta's ModelRenderer/ko.java box UV construction. */
    float u_right0 = (float)(tex_x + d + w);
    float v_right0 = (float)(tex_y + d);
    float u_right1 = (float)(tex_x + d + w + d);
    float v_right1 = (float)(tex_y + d + h);

    float u_left0 = (float)(tex_x + 0);
    float v_left0 = (float)(tex_y + d);
    float u_left1 = (float)(tex_x + d);
    float v_left1 = (float)(tex_y + d + h);

    float u_top0 = (float)(tex_x + d);
    float v_top0 = (float)(tex_y + 0);
    float u_top1 = (float)(tex_x + d + w);
    float v_top1 = (float)(tex_y + d);

    float u_bottom0 = (float)(tex_x + d + w);
    float v_bottom0 = (float)(tex_y + 0);
    float u_bottom1 = (float)(tex_x + d + w + w);
    float v_bottom1 = (float)(tex_y + d);

    float u_front0 = (float)(tex_x + d);
    float v_front0 = (float)(tex_y + d);
    float u_front1 = (float)(tex_x + d + w);
    float v_front1 = (float)(tex_y + d + h);

    float u_back0 = (float)(tex_x + d + w + d);
    float v_back0 = (float)(tex_y + d);
    float u_back1 = (float)(tex_x + d + w + d + w);
    float v_back1 = (float)(tex_y + d + h);

    glBegin(GL_QUADS);
    /* +X */
    steve_quad(x1,y0,z1, x1,y0,z0, x1,y1,z0, x1,y1,z1, u_right0,v_right0,u_right1,v_right1, 1,0,0);
    /* -X */
    steve_quad(x0,y0,z0, x0,y0,z1, x0,y1,z1, x0,y1,z0, u_left0,v_left0,u_left1,v_left1, -1,0,0);
    /* -Y / top */
    steve_quad(x1,y0,z1, x0,y0,z1, x0,y0,z0, x1,y0,z0, u_top0,v_top0,u_top1,v_top1, 0,-1,0);
    /* +Y / bottom */
    steve_quad(x1,y1,z0, x0,y1,z0, x0,y1,z1, x1,y1,z1, u_bottom0,v_bottom0,u_bottom1,v_bottom1, 0,1,0);
    /* -Z / front */
    steve_quad(x1,y0,z0, x0,y0,z0, x0,y1,z0, x1,y1,z0, u_front0,v_front0,u_front1,v_front1, 0,0,-1);
    /* +Z / back */
    steve_quad(x0,y0,z1, x1,y0,z1, x1,y1,z1, x0,y1,z1, u_back0,v_back0,u_back1,v_back1, 0,0,1);
    glEnd();
}

static void steve_part(int tex_x, int tex_y,
                       float pivot_x, float pivot_y, float pivot_z,
                       float box_x, float box_y, float box_z,
                       int box_w, int box_h, int box_d,
                       float inflate, int mirror,
                       float rot_x_deg, float rot_y_deg, float rot_z_deg) {
    glPushMatrix();
    glTranslatef(pivot_x * 0.0625f, pivot_y * 0.0625f, pivot_z * 0.0625f);
    if (rot_z_deg != 0.0f) glRotatef(rot_z_deg, 0.0f, 0.0f, 1.0f);
    if (rot_y_deg != 0.0f) glRotatef(rot_y_deg, 0.0f, 1.0f, 0.0f);
    if (rot_x_deg != 0.0f) glRotatef(rot_x_deg, 1.0f, 0.0f, 0.0f);
    steve_box(tex_x, tex_y, box_x, box_y, box_z, box_w, box_h, box_d, inflate, mirror);
    glPopMatrix();
}

static void draw_inventory_steve(int inv_x, int inv_y) {
    if (!tex_steve.id) return;

    float mx = (float)(inv_x + 51 - g_mouse_x);
    float my = (float)(inv_y + 75 - 50 - g_mouse_y);
    /* ns.java stores these directly as degrees after multiplying atan() by 20/40.
       Do not convert radians to degrees here; doing so over-rotates the model by
       ~57x and makes Steve appear twisted/sideways in the inventory. */
    float body_yaw = atanf(mx / 40.0f) * 40.0f;
    float head_yaw = atanf(mx / 40.0f) * 20.0f;
    float head_pitch = -atanf(my / 40.0f) * 20.0f;

    float idle = (float)g_ingame_ticks;
    float arm_roll = cosf(idle * 0.09f) * 2.86479f + 2.86479f;
    float arm_pitch = sinf(idle * 0.067f) * 2.86479f;

    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_steve.id);
    steve_set_texture_dims(&tex_steve);
    steve_set_tint(1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);

    /* char.png has transparent pixels in the second head layer (helmet/hat).
       Minecraft renders those through the alpha test.  The previous patch
       drew the overlay with blending disabled, so the transparent RGB=0 pixels
       became a solid black cube over Steve's head. */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);

    /* ns.java inventory transform:
       translate(x+51,y+75,50), scale(-30,30,30), rotate 180 Z,
       rotate 135 Y, setup standard item lighting, rotate -135 Y,
       pitch toward mouse, then render the player model. */
    glTranslatef((float)(inv_x + 51), (float)(inv_y + 75), 50.0f);
    glScalef(-30.0f, 30.0f, 30.0f);
    glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
    glRotatef(135.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(-135.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(head_pitch, 1.0f, 0.0f, 0.0f);

    /* RenderLiving/ei.java transform for a standing humanoid.
       Do NOT add player aY/yOffset here: ns.java adds it before calling
       RenderManager, but ch.java subtracts it again before ei.java renders.
       The previous C patch added +1.62 without the matching subtract, which
       pushed Steve above the inventory preview box. */
    glRotatef(180.0f - body_yaw, 0.0f, 1.0f, 0.0f);
    glScalef(-1.0f, -1.0f, 1.0f);
    glTranslatef(0.0f, -24.0f * 0.0625f - 0.0078125f, 0.0f);

    glColor4f(1,1,1,1);
    steve_part(16, 16, 0, 0, 0, -4, 0, -2, 8, 12, 4, 0.0f, 0, 0, 0, 0);
    steve_part(40, 16, -5, 2, 0, -3, -2, -2, 4, 12, 4, 0.0f, 0, arm_pitch, 0, arm_roll);
    steve_part(40, 16,  5, 2, 0, -1, -2, -2, 4, 12, 4, 0.0f, 1, -arm_pitch, 0, -arm_roll);
    steve_part(0, 16, -2, 12, 0, -2, 0, -2, 4, 12, 4, 0.0f, 0, 0, 0, 0);
    steve_part(0, 16,  2, 12, 0, -2, 0, -2, 4, 12, 4, 0.0f, 1, 0, 0, 0);
    steve_part(0, 0, 0, 0, 0, -4, -8, -4, 8, 8, 8, 0.0f, 0, head_pitch, head_yaw, 0);
    steve_part(32, 0, 0, 0, 0, -4, -8, -4, 8, 8, 8, 0.5f, 0, head_pitch, head_yaw, 0);

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glColor4f(1,1,1,1);
    glPopMatrix();
}


static void draw_item_tooltip_for_slot(int slot, int mx, int my) {
    if (!stack_empty(&g_carried_stack) || slot < 0) return;
    ItemStack *st = NULL;
    ItemStack tmp;
    if (slot == 104 || slot == 309) {
        tmp = current_crafting_output();
        if (!stack_empty(&tmp)) st = &tmp;
    } else {
        st = inventory_slot_ptr(slot);
    }
    if (!st || stack_empty(st)) return;
    const char *name = item_display_name(st->id);
    if (!name || !name[0]) return;
    int tw = text_width(name);
    int x = mx + 12;
    int y = my - 12;
    if (x + tw + 6 > g_gui_w) x = g_gui_w - tw - 6;
    if (y < 4) y = my + 12;
    draw_rect(x - 3, y - 3, x + tw + 3, y + 8 + 3, (int)0xC0000000u);
    draw_text(name, x, y, 0xFFFFFF);
}

static void draw_hovered_item_tooltip(void) {
    draw_item_tooltip_for_slot(inventory_slot_at(g_mouse_x, g_mouse_y), g_mouse_x, g_mouse_y);
}

static void draw_inventory_screen(void) {
    draw_ingame_world_view(0);
    draw_gradient(0, 0, g_gui_w, g_gui_h, -1072689136, -804253680);
    int x = (g_gui_w - 176) / 2;
    int y = (g_gui_h - 166) / 2;
    draw_textured_rect_tex(&tex_inventory, x, y, 0, 0, 176, 166, 0xFFFFFF);
    draw_inventory_steve(x, y);
    for (int row = 0; row < 3; row++) for (int col = 0; col < 9; col++) {
        int slot = 9 + col + row * 9;
        draw_item_stack_gui(&g_inventory[slot], x + 8 + col * 18, y + 84 + row * 18);
    }
    for (int col = 0; col < 9; col++) draw_item_stack_gui(&g_inventory[col], x + 8 + col * 18, y + 142);

    /* Container labels are drawn without the HUD-style shadow, matching the
       original GUI text color and avoiding the dark/white muddy look. */
    draw_text_no_shadow("Crafting", x + 86, y + 6, 0x404040);
    for (int row = 0; row < 2; row++) for (int col = 0; col < 2; col++) {
        int slot = col + row * 2;
        draw_item_stack_gui(&g_craft_grid[slot], x + 88 + col * 18, y + 26 + row * 18);
    }
    ItemStack craft_out = inventory_crafting_output();
    draw_item_stack_gui(&craft_out, x + 144, y + 36);

    draw_hovered_item_tooltip();
    draw_carried_stack();
}


static void draw_container_player_inventory_stacks_at(int x, int y, int inv_y, int hotbar_y) {
    for (int row = 0; row < 3; row++) for (int col = 0; col < 9; col++) {
        int slot = 9 + col + row * 9;
        draw_item_stack_gui(&g_inventory[slot], x + 8 + col * 18, y + inv_y + row * 18);
    }
    for (int col = 0; col < 9; col++) draw_item_stack_gui(&g_inventory[col], x + 8 + col * 18, y + hotbar_y);
}

static void draw_container_player_inventory_stacks(int x, int y) {
    draw_container_player_inventory_stacks_at(x, y, 84, 142);
}

static void draw_workbench_screen(void) {
    draw_ingame_world_view(0);
    draw_gradient(0, 0, g_gui_w, g_gui_h, -1072689136, -804253680);
    int x = (g_gui_w - 176) / 2;
    int y = (g_gui_h - 166) / 2;
    if (tex_workbench.id) draw_textured_rect_tex(&tex_workbench, x, y, 0, 0, 176, 166, 0xFFFFFF);
    else draw_rect(x, y, x + 176, y + 166, 0xFFc6c6c6);

    draw_text_no_shadow("Crafting", x + 28, y + 6, 4210752);
    for (int row = 0; row < 3; row++) for (int col = 0; col < 3; col++) {
        int slot = col + row * 3;
        draw_item_stack_gui(&g_workbench_grid[slot], x + 30 + col * 18, y + 17 + row * 18);
    }

    ItemStack out = workbench_crafting_output();
    draw_item_stack_gui(&out, x + 124, y + 35);

    draw_text_no_shadow("Inventory", x + 8, y + 72, 4210752);
    draw_container_player_inventory_stacks(x, y);
    draw_hovered_item_tooltip();
    draw_carried_stack();
}


static void draw_chest_screen(void) {
    draw_ingame_world_view(0);
    draw_gradient(0, 0, g_gui_w, g_gui_h, -1072689136, -804253680);
    int rows = (g_open_chest_rows == 6) ? 6 : 3;
    int h = (rows == 6) ? 222 : 166;
    int inv_label_y = (rows == 6) ? 128 : 72;
    int inv_slots_y = (rows == 6) ? 140 : 84;
    int hotbar_y = (rows == 6) ? 198 : 142;
    int x = (g_gui_w - 176) / 2;
    int y = (g_gui_h - h) / 2;

    if (tex_chest_gui.id) {
        if (rows == 6 && tex_chest_gui.w >= 176 && tex_chest_gui.h >= 222) {
            draw_textured_rect_tex(&tex_chest_gui, x, y, 0, 0, 176, 222, 0xFFFFFF);
        } else if (tex_chest_gui.w >= 176 && tex_chest_gui.h >= 222) {
            /* The Java container.png is a double-chest sheet. For a single chest,
               draw the title + 3 chest rows, then stitch the player inventory
               section up exactly like a 27-slot Beta chest screen. */
            draw_textured_rect_tex(&tex_chest_gui, x, y, 0, 0, 176, 71, 0xFFFFFF);
            draw_textured_rect_part_scaled(&tex_chest_gui, x, y + 71, 176, 95, 0, 127, 176, 95, 0xFFFFFF);
        } else {
            draw_textured_rect_tex(&tex_chest_gui, x, y, 0, 0, 176, h, 0xFFFFFF);
        }
    } else draw_rect(x, y, x + 176, y + h, 0xFFc6c6c6);
    draw_text_no_shadow(g_open_chest_title[0] ? g_open_chest_title : "Chest", x + 8, y + 6, 4210752);
    draw_text_no_shadow("Inventory", x + 8, y + inv_label_y, 4210752);

    for (int row = 0; row < rows; row++) for (int col = 0; col < 9; col++) {
        ItemStack *st = chest_get_open_slot_ptr(col + row * 9);
        if (st) draw_item_stack_gui(st, x + 8 + col * 18, y + 18 + row * 18);
    }
    draw_container_player_inventory_stacks_at(x, y, inv_slots_y, hotbar_y);
    draw_hovered_item_tooltip();

    draw_carried_stack();
}

static void draw_furnace_screen(void) {
    draw_ingame_world_view(0);
    draw_gradient(0, 0, g_gui_w, g_gui_h, -1072689136, -804253680);
    int x = (g_gui_w - 176) / 2;
    int y = (g_gui_h - 166) / 2;
    if (tex_furnace_gui.id) draw_textured_rect_tex(&tex_furnace_gui, x, y, 0, 0, 176, 166, 0xFFFFFF);
    else draw_rect(x, y, x + 176, y + 166, 0xFFc6c6c6);

    if (tex_furnace_gui.id && furnace_open_tile()) {
        int flame = furnace_burn_scaled(12);
        if (flame > 0) {
            draw_textured_rect_tex(&tex_furnace_gui, x + 56, y + 36 + 12 - flame, 176, 12 - flame, 14, flame + 2, 0xFFFFFF);
        }
        int arrow = furnace_cook_scaled(24);
        if (arrow > 0) {
            draw_textured_rect_tex(&tex_furnace_gui, x + 79, y + 34, 176, 14, arrow + 1, 16, 0xFFFFFF);
        }
    }

    draw_text_no_shadow("Furnace", x + 60, y + 6, 4210752);
    draw_text_no_shadow("Inventory", x + 8, y + 72, 4210752);

    ItemStack *in = furnace_get_slot_ptr(0);
    ItemStack *fuel = furnace_get_slot_ptr(1);
    ItemStack *out = furnace_get_slot_ptr(2);
    if (in) draw_item_stack_gui(in, x + 56, y + 17);
    if (fuel) draw_item_stack_gui(fuel, x + 56, y + 53);
    if (out) draw_item_stack_gui(out, x + 116, y + 35);

    draw_container_player_inventory_stacks(x, y);
    draw_hovered_item_tooltip();
    draw_carried_stack();
}



static void draw_death_screen(void) {
    draw_ingame_world_view(0);
    draw_gradient(0, 0, g_gui_w, g_gui_h, 1615855616, -1602211792);
    draw_centered_text("Game over!", g_gui_w / 2, 30, 16777215);
    draw_centered_text("Score: &e0", g_gui_w / 2, 100, 16777215);
    draw_all_buttons();
    draw_save_message();
}

static void draw_chat_screen(void) {
    draw_ingame_screen();
    char field[160];
    snprintf(field, sizeof(field), "> %s%s", g_chat_input, (g_ingame_ticks / 6 % 2 == 0) ? "_" : "");
    draw_rect(2, g_gui_h - 14, g_gui_w - 2, g_gui_h - 2, (int)0x80000000u);
    draw_text(field, 4, g_gui_h - 12, 14737632);
}


static void draw_world_type_screen(void) {
    draw_default_bg();
    draw_centered_text("Create new world", g_gui_w / 2, 40, 16777215);
    draw_centered_text("Choose terrain type", g_gui_w / 2, 64, 10526880);
    draw_all_buttons();
}

static void draw_generating_screen(void) {
    draw_tiled_background_tint(0x404040, 0);
    int bar_w = 100;
    int bar_h = 2;
    int x = g_gui_w / 2 - bar_w / 2;
    int y = g_gui_h / 2 + 16;
    int p = g_worldgen.progress;
    if (p < 0) p = 0;
    if (p > 100) p = 100;
    draw_rect(x, y, x + bar_w, y + bar_h, 8421504);
    draw_rect(x, y, x + p, y + bar_h, 8454016);
    draw_centered_text(g_worldgen.title, g_gui_w / 2, g_gui_h / 2 - 20, 16777215);
    draw_centered_text(g_worldgen.status, g_gui_w / 2, g_gui_h / 2 + 8, 16777215);
}


static void draw_texturepack_install_screen(void) {
    draw_tiled_background_tint(0x404040, 0);
    int bar_w = 100;
    int bar_h = 2;
    int x = g_gui_w / 2 - bar_w / 2;
    int y = g_gui_h / 2 + 16;
    int p = (int)InterlockedCompareExchange(&g_classic_install_progress, 0, 0);
    char status[MAX_LABEL];
    lstrcpynA(status, g_classic_install_status[0] ? g_classic_install_status : "Downloading client.jar...", sizeof(status));
    if (p < 0) p = 0;
    if (p > 100) p = 100;
    draw_rect(x, y, x + bar_w, y + bar_h, 8421504);
    draw_rect(x, y, x + p, y + bar_h, 8454016);
    draw_centered_text("Downloading texture pack", g_gui_w / 2, g_gui_h / 2 - 20, 16777215);
    draw_centered_text(status, g_gui_w / 2, g_gui_h / 2 + 8, 16777215);
}

static void draw_notice(void) {
    draw_default_bg();
    draw_centered_text(g_notice_title, g_gui_w / 2, g_gui_h / 4 - 60 + 20, 16777215);
    draw_text(g_notice_line1, g_gui_w / 2 - 140, g_gui_h / 4 - 60 + 60, 10526880);
    draw_text(g_notice_line2, g_gui_w / 2 - 140, g_gui_h / 4 - 60 + 69, 10526880);
    draw_all_buttons();
}


static void draw_renderer_restart_prompt(void) {
    draw_default_bg();
    draw_centered_text("Restart Required", g_gui_w / 2, g_gui_h / 4 - 60 + 20, 16777215);
    draw_text("Changing the renderer requires restarting the game.", g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 56, 10526880);
    char line[MAX_LABEL];
    snprintf(line, sizeof(line), "New renderer: %s", renderer_backend_label(g_selected_renderer_backend));
    draw_text(line, g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 73, 16777215);
    draw_text("Restart applies it. Ignore keeps the current renderer.", g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 90, 10526880);
    draw_all_buttons();
}

static void draw_classic_pack_download_prompt(void) {
    char size_line[MAX_LABEL];
    classic_resource_size_start_fetch();
    draw_default_bg();
    draw_centered_text("Classic Resources Recommended", g_gui_w / 2, g_gui_h / 4 - 60 + 18, 16777215);
    draw_text("The built-in default textures are heavily prototype", g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 48, 10526880);
    draw_text("and some blocks/items may look wrong or unfinished.", g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 59, 10526880);
    draw_text("I recommend downloading the Classic resources.", g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 76, 16777215);
    classic_resource_size_format(size_line, sizeof(size_line));
    draw_text(size_line, g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 93, 10526880);
    draw_all_buttons();
}

static void draw_classic_pack_warning(void) {
    draw_default_bg();
    draw_centered_text("Texture Pack Warning", g_gui_w / 2, g_gui_h / 4 - 60 + 20, 16777215);
    draw_text("The currently saved Minecraft Classic texture pack", g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 56, 10526880);
    draw_text("does not have the required Beta block/item textures.", g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 67, 10526880);
    draw_text("Do you want to re-download it now?", g_gui_w / 2 - 155, g_gui_h / 4 - 60 + 86, 16777215);
    draw_all_buttons();
}

