/* Split from original monolithic main.c. Included by src/main.c unity build. */

static void draw_sky_only(void) {
    draw_gradient(0, 0, g_gui_w, g_gui_h, 0xFF78A7FF, 0xFFC8E6FF);
    draw_gradient(0, g_gui_h / 2, g_gui_w, g_gui_h, 0x00FFFFFF, 0x30FFFFFF);
}

static void draw_chat_lines(int force_visible) {
    glEnable(GL_BLEND);
    for (int i = 0; i < g_chat_count && i < (force_visible ? 20 : 10); i++) {
        int alpha = 255;
        if (!force_visible) {
            double fade = (double)g_chat_lines[i].age / 200.0;
            fade = 1.0 - fade;
            fade *= 10.0;
            if (fade < 0.0) fade = 0.0;
            if (fade > 1.0) fade = 1.0;
            fade *= fade;
            alpha = (int)(255.0 * fade);
        }
        if (alpha <= 0) continue;
        int y = g_gui_h - 48 - i * 9;
        draw_rect(2, y - 1, 322, y + 8, (alpha / 2) << 24);
        draw_text(g_chat_lines[i].text, 2, y, (alpha << 24) | 0xFFFFFF);
    }
}


static void setup_world_projection(void) {
    glViewport(0, 0, g_win_w, g_win_h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (g_win_h > 0) ? ((float)g_win_w / (float)g_win_h) : 1.0f;
    gluPerspective(70.0, (double)aspect, 0.05, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void apply_view_bobbing(float partial) {
    if (!g_opts.view_bobbing) return;
    float walk_delta = g_distance_walked - g_prev_distance_walked;
    float walk = g_distance_walked + walk_delta * partial;
    float cam_yaw = g_prev_camera_yaw + (g_camera_yaw - g_prev_camera_yaw) * partial;
    float cam_pitch = g_prev_camera_pitch + (g_camera_pitch - g_prev_camera_pitch) * partial;

    /* kq.java::f(float): exact Beta view-bobbing transform. */
    glTranslatef(sinf(walk * (float)M_PI) * cam_yaw * 0.5f,
                 -fabsf(cosf(walk * (float)M_PI) * cam_yaw),
                 0.0f);
    glRotatef(sinf(walk * (float)M_PI) * cam_yaw * 3.0f, 0.0f, 0.0f, 1.0f);
    glRotatef(fabsf(cosf(walk * (float)M_PI + 0.2f) * cam_yaw) * 5.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(cam_pitch, 1.0f, 0.0f, 0.0f);
}

static void apply_player_camera(float partial) {
    float x = g_player_prev_x + (g_player_x - g_player_prev_x) * partial;
    float y = g_player_prev_y + (g_player_y - g_player_prev_y) * partial;
    float z = g_player_prev_z + (g_player_z - g_player_prev_z) * partial;
    float yaw = lerp_angle(g_player_prev_yaw, g_player_yaw, partial);
    float pitch = g_player_prev_pitch + (g_player_pitch - g_player_prev_pitch) * partial;

    apply_view_bobbing(partial);
    glTranslatef(0.0f, 0.0f, -0.1f); /* first-person camera nudge from kq.java */
    glRotatef(pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(yaw + 180.0f, 0.0f, 1.0f, 0.0f);
    glTranslatef(-x, -y, -z);
}


static void terrain_tile_uv(int tile, float *u0, float *v0, float *u1, float *v1) {
    float tw = tex_terrain.w ? (float)tex_terrain.w : 256.0f;
    float th = tex_terrain.h ? (float)tex_terrain.h : 256.0f;
    int tx = (tile & 15) * 16;
    int ty = (tile >> 4) * 16;
    *u0 = ((float)tx + 0.01f) / tw;
    *v0 = ((float)ty + 0.01f) / th;
    *u1 = ((float)tx + 15.99f) / tw;
    *v1 = ((float)ty + 15.99f) / th;
}

static void world_set_shade(float shade) {
    glColor4f(shade, shade, shade, 1.0f);
}

static void world_set_color_shade(int rgb, float shade) {
    float r = ((rgb >> 16) & 255) / 255.0f;
    float g = ((rgb >> 8) & 255) / 255.0f;
    float b = (rgb & 255) / 255.0f;
    glColor4f(r * shade, g * shade, b * shade, 1.0f);
}

static void world_tex_vertex(float x, float y, float z, float u, float v) {
    glTexCoord2f(u, v);
    glVertex3f(x, y, z);
}

static void draw_grass_block(float x, float y, float z) {
    float x0 = x, x1 = x + 1.0f;
    float y0 = y, y1 = y + 1.0f;
    float z0 = z, z1 = z + 1.0f;
    float u0,v0,u1,v1;

    /* Beta terrain.png indices: 0=grass top, 2=dirt, 3=grass side. */
    terrain_tile_uv(0, &u0, &v0, &u1, &v1);
    world_set_color_shade(0x6FAD3A, 1.0f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z0, u0, v0);
    world_tex_vertex(x1, y1, z0, u1, v0);
    world_tex_vertex(x1, y1, z1, u1, v1);
    world_tex_vertex(x0, y1, z1, u0, v1);
    glEnd();

    terrain_tile_uv(3, &u0, &v0, &u1, &v1);
    world_set_shade(0.80f);
    glBegin(GL_QUADS);
    world_tex_vertex(x1, y1, z0, u0, v0);
    world_tex_vertex(x0, y1, z0, u1, v0);
    world_tex_vertex(x0, y0, z0, u1, v1);
    world_tex_vertex(x1, y0, z0, u0, v1);

    world_tex_vertex(x0, y1, z1, u0, v0);
    world_tex_vertex(x1, y1, z1, u1, v0);
    world_tex_vertex(x1, y0, z1, u1, v1);
    world_tex_vertex(x0, y0, z1, u0, v1);
    glEnd();

    world_set_shade(0.60f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z0, u0, v0);
    world_tex_vertex(x0, y1, z1, u1, v0);
    world_tex_vertex(x0, y0, z1, u1, v1);
    world_tex_vertex(x0, y0, z0, u0, v1);

    world_tex_vertex(x1, y1, z1, u0, v0);
    world_tex_vertex(x1, y1, z0, u1, v0);
    world_tex_vertex(x1, y0, z0, u1, v1);
    world_tex_vertex(x1, y0, z1, u0, v1);
    glEnd();

    terrain_tile_uv(2, &u0, &v0, &u1, &v1);
    world_set_shade(0.50f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y0, z1, u0, v0);
    world_tex_vertex(x1, y0, z1, u1, v0);
    world_tex_vertex(x1, y0, z0, u1, v1);
    world_tex_vertex(x0, y0, z0, u0, v1);
    glEnd();
}


static void draw_dirt_block(float x, float y, float z) {
    float x0 = x, x1 = x + 1.0f;
    float y0 = y, y1 = y + 1.0f;
    float z0 = z, z1 = z + 1.0f;
    float u0,v0,u1,v1;
    terrain_tile_uv(2, &u0, &v0, &u1, &v1);
    world_set_shade(1.0f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y1, z1, u1, v1); world_tex_vertex(x0, y1, z1, u0, v1);
    glEnd();
    world_set_shade(0.80f);
    glBegin(GL_QUADS);
    world_tex_vertex(x1, y1, z0, u0, v0); world_tex_vertex(x0, y1, z0, u1, v0); world_tex_vertex(x0, y0, z0, u1, v1); world_tex_vertex(x1, y0, z0, u0, v1);
    world_tex_vertex(x0, y1, z1, u0, v0); world_tex_vertex(x1, y1, z1, u1, v0); world_tex_vertex(x1, y0, z1, u1, v1); world_tex_vertex(x0, y0, z1, u0, v1);
    glEnd();
    world_set_shade(0.60f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x0, y1, z1, u1, v0); world_tex_vertex(x0, y0, z1, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    world_tex_vertex(x1, y1, z1, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x1, y0, z1, u0, v1);
    glEnd();
    world_set_shade(0.50f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y0, z1, u0, v0); world_tex_vertex(x1, y0, z1, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    glEnd();
}
static void draw_world_block_id(int id, float x, float y, float z) {
    if (id == BLOCK_GRASS) draw_grass_block(x, y, z);
    else if (id == BLOCK_DIRT) draw_dirt_block(x, y, z);
}
static void draw_break_overlay_cube(float x, float y, float z, int stage) {
    if (stage < 0) return;
    if (stage > 9) stage = 9;
    float u0,v0,u1,v1;
    terrain_tile_uv(240 + stage, &u0, &v0, &u1, &v1);
    float x0 = x - 0.002f, x1 = x + 1.002f;
    float y0 = y - 0.002f, y1 = y + 1.002f;
    float z0 = z - 0.002f, z1 = z + 1.002f;
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
    glDepthMask(GL_FALSE);
    glColor4f(1,1,1,1);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y1, z1, u1, v1); world_tex_vertex(x0, y1, z1, u0, v1);
    world_tex_vertex(x1, y1, z0, u0, v0); world_tex_vertex(x0, y1, z0, u1, v0); world_tex_vertex(x0, y0, z0, u1, v1); world_tex_vertex(x1, y0, z0, u0, v1);
    world_tex_vertex(x0, y1, z1, u0, v0); world_tex_vertex(x1, y1, z1, u1, v0); world_tex_vertex(x1, y0, z1, u1, v1); world_tex_vertex(x0, y0, z1, u0, v1);
    world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x0, y1, z1, u1, v0); world_tex_vertex(x0, y0, z1, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    world_tex_vertex(x1, y1, z1, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x1, y0, z1, u0, v1);
    world_tex_vertex(x0, y0, z1, u0, v0); world_tex_vertex(x1, y0, z1, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    glEnd();
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
static void draw_dropped_items(void) {
    if (!tex_terrain.id) return;
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    for (int i = 0; i < MAX_DROP_ENTITIES; i++) {
        FlatDroppedItem *e = &g_drops[i];
        if (!e->active) continue;
        float x = e->prev_x + (e->x - e->prev_x) * g_frame_partial;
        float y = e->prev_y + (e->y - e->prev_y) * g_frame_partial;
        float z = e->prev_z + (e->z - e->prev_z) * g_frame_partial;
        float bob = sinf(((float)e->age + g_frame_partial) / 10.0f + e->rot) * 0.1f + 0.1f;
        glPushMatrix();
        glTranslatef(x, y + bob, z);
        glRotatef((((float)e->age + g_frame_partial) / 20.0f + e->rot) * 57.29578f, 0.0f, 1.0f, 0.0f);
        glScalef(0.25f, 0.25f, 0.25f);
        draw_world_block_id(e->stack.id, -0.5f, -0.5f, -0.5f);
        glPopMatrix();
    }
}

static void draw_flat_test_world(void) {
    if (!tex_terrain.id) return;

    setup_world_projection();
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);

    apply_player_camera(g_frame_partial);

    for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
        for (int z = FLAT_WORLD_MIN; z <= FLAT_WORLD_MAX; z++) {
            for (int x = FLAT_WORLD_MIN; x <= FLAT_WORLD_MAX; x++) {
                int id = flat_get_block(x, y, z);
                if (id) draw_world_block_id(id, (float)x, (float)y, (float)z);
            }
        }
    }
    draw_dropped_items();
    if (g_breaking_block && flat_get_block(g_break_x, g_break_y, g_break_z) != 0) {
        float dmg = g_prev_break_damage + (g_break_damage - g_prev_break_damage) * g_frame_partial;
        int stage = (int)(dmg * 10.0f);
        draw_break_overlay_cube((float)g_break_x, (float)g_break_y, (float)g_break_z, stage);
    }

    glColor4f(1,1,1,1);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}

static void draw_first_person_hand(void) {
    if (!tex_steve.id) return;

    setup_world_projection();
    glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Java Beta render order: the held item/hand pass gets the same view bobbing
       matrix as the camera before ItemRenderer renders the equipped item. */
    apply_view_bobbing(g_frame_partial);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);

    float swing = g_prev_hand_swing + (g_hand_swing - g_prev_hand_swing) * g_frame_partial;
    if (swing < 0.0f) swing = 0.0f;
    if (swing > 1.0f) swing = 1.0f;
    float swing_sin = sinf(swing * (float)M_PI);
    float swing_sqrt_sin = sinf(sqrtf(swing) * (float)M_PI);
    ItemStack *held = &g_inventory[g_selected_hotbar_slot];

    if (!stack_empty(held) && (held->id == BLOCK_GRASS || held->id == BLOCK_DIRT) && tex_terrain.id) {
        /* ItemRenderer held-block branch: when a block is selected Java renders the
           3D block in-hand, not the empty player arm.  This also makes the right
           click/use swing look different from mining with an empty hand. */
        glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
        glPushMatrix();
        float s = 0.8f;
        glTranslatef(0.7f * s, -0.65f * s, -0.9f * s);
        glTranslatef(-swing_sqrt_sin * 0.4f,
                     sinf(sqrtf(swing) * (float)M_PI * 2.0f) * 0.2f,
                     -swing_sin * 0.2f);
        glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
        glRotatef(-swing_sin * 20.0f, 0.0f, 1.0f, 0.0f);
        glRotatef(-swing_sqrt_sin * 20.0f, 0.0f, 0.0f, 1.0f);
        glRotatef(-swing_sqrt_sin * 80.0f, 1.0f, 0.0f, 0.0f);
        glScalef(0.40f, 0.40f, 0.40f);
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        draw_world_block_id(held->id, -0.5f, -0.5f, -0.5f);
        glPopMatrix();
    } else {
        glBindTexture(GL_TEXTURE_2D, tex_steve.id);
        glPushMatrix();
        float s = 0.8f;
        glTranslatef(-swing_sqrt_sin * 0.3f,
                     sinf(sqrtf(swing) * (float)M_PI * 2.0f) * 0.4f,
                     -swing_sin * 0.4f);
        glTranslatef(0.8f * s, -0.75f * s, -0.9f * s);
        glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
        glRotatef(swing_sqrt_sin * 70.0f, 0.0f, 1.0f, 0.0f);
        glRotatef(-swing_sin * 20.0f, 0.0f, 0.0f, 1.0f);
        glTranslatef(-1.0f, 3.6f, 3.5f);
        glRotatef(120.0f, 0.0f, 0.0f, 1.0f);
        glRotatef(200.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(-135.0f, 0.0f, 1.0f, 0.0f);
        glTranslatef(5.6f, 0.0f, 0.0f);

        /* ch.java::b(): render right arm at model scale 1/16. */
        glColor4f(1,1,1,1);
        steve_part(40, 16, -5, 2, 0, -3, -2, -2, 4, 12, 4, 0.0f, 0, 0.0f, 0.0f, 5.729578f);
        glPopMatrix();
    }

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glColor4f(1,1,1,1);
}
static void draw_ingame_world_view(int with_hand) {
    draw_sky_only();
    draw_flat_test_world();
    if (with_hand) draw_first_person_hand();
    setup_gui_projection();
}

