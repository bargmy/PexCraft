/* Split from original monolithic main.c. Included by src/main.c unity build. */

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
    for (int i = 0; i < 10; i++) {
        int x = w / 2 - 91 + i * 8;
        int y = h - 32;
        draw_textured_rect_tex(&tex_icons, x, y, 16, 0, 9, 9, 0xFFFFFF);
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
    if (GetAsyncKeyState(VK_F3) & 0x8000) {
        char line[128];
        snprintf(line, sizeof(line), "Flat 5x5 grass world: %s", g_loaded_world_name[0] ? g_loaded_world_name : "World");
        draw_text(line, 2, 12, 14737632);
        char posline[96];
        snprintf(posline, sizeof(posline), "x: %.2f", g_player_x);
        draw_text(posline, 2, 64, 14737632);
        snprintf(posline, sizeof(posline), "y: %.2f", g_player_y);
        draw_text(posline, 2, 72, 14737632);
        snprintf(posline, sizeof(posline), "z: %.2f", g_player_z);
        draw_text(posline, 2, 80, 14737632);
        snprintf(posline, sizeof(posline), "yaw: %.1f pitch: %.1f", g_player_yaw, g_player_pitch);
        draw_text(posline, 2, 88, 14737632);
    }
    draw_chat_lines(g_screen == SCREEN_CHAT);
}

static void draw_ingame_screen(void) {
    draw_ingame_world_view(1);
    draw_hud();
}

static void draw_pause_screen(void) {
    draw_ingame_world_view(1);
    draw_gradient(0, 0, g_gui_w, g_gui_h, -1072689136, -804253680);
    float pulse = (float)(sin(((g_ticks % 10) + 0.0f) / 10.0f * (float)M_PI * 2.0f) * 0.2f + 0.8f);
    int c = (int)(255.0f * pulse);
    int color = (c << 16) | (c << 8) | c;
    draw_text("Saving level..", 8, g_gui_h - 16, color);
    draw_centered_text("Game menu", g_gui_w / 2, 40, 16777215);
    draw_all_buttons();
}


static void steve_color_for_normal(float nx, float ny, float nz) {
    float shade = 0.82f;
    if (ny < -0.5f) shade = 1.0f;
    else if (ny > 0.5f) shade = 0.55f;
    else if (nz > 0.5f) shade = 0.92f;
    else if (nz < -0.5f) shade = 0.70f;
    else if (nx != 0.0f) shade = 0.76f;
    glColor4f(shade, shade, shade, 1.0f);
}

static void steve_vertex(float x, float y, float z, float u, float v) {
    float tw = tex_steve.w ? (float)tex_steve.w : 64.0f;
    float th = tex_steve.h ? (float)tex_steve.h : 32.0f;
    glTexCoord2f(u / tw, v / th);
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
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);

    /* char.png has transparent pixels in the second head layer (helmet/hat).
       PEXCRAFT renders those through the alpha test.  The previous patch
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
    draw_text("Crafting", x + 86, y + 16, 4210752);
    draw_carried_stack();
}

static void draw_chat_screen(void) {
    draw_ingame_screen();
    char field[160];
    snprintf(field, sizeof(field), "> %s%s", g_chat_input, (g_ingame_ticks / 6 % 2 == 0) ? "_" : "");
    draw_rect(2, g_gui_h - 14, g_gui_w - 2, g_gui_h - 2, (int)0x80000000u);
    draw_text(field, 4, g_gui_h - 12, 14737632);
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

static void draw_notice(void) {
    draw_default_bg();
    draw_centered_text(g_notice_title, g_gui_w / 2, g_gui_h / 4 - 60 + 20, 16777215);
    draw_text(g_notice_line1, g_gui_w / 2 - 140, g_gui_h / 4 - 60 + 60, 10526880);
    draw_text(g_notice_line2, g_gui_w / 2 - 140, g_gui_h / 4 - 60 + 69, 10526880);
    draw_all_buttons();
}
