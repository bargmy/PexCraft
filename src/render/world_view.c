/* Split from original monolithic main.c. Included by src/main.c unity build. */

static void draw_source_item_3d_from_atlas(int tile);

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

    /* Sneaking lowers the player camera/eye a bit, like the earlier patch. */
    if (key_down_vk(g_opts.keys[5])) y -= 0.18f;

    if (g_third_person_view) {
        /* Source reference: B1.0.0 kq.java::g(float), toggled by F5 in
           Minecraft.java when key 63 is pressed.  The source uses a 4 block
           third-person distance behind the player's view direction. */
        float yaw_rad = yaw * (float)M_PI / 180.0f;
        float pitch_rad = pitch * (float)M_PI / 180.0f;
        float dist = 4.0f;

        float look_x = -sinf(yaw_rad) * cosf(pitch_rad);
        float look_y = -sinf(pitch_rad);
        float look_z =  cosf(yaw_rad) * cosf(pitch_rad);

        float cam_x = x - look_x * dist;
        float cam_y = y - look_y * dist;
        float cam_z = z - look_z * dist;

        glRotatef(pitch, 1.0f, 0.0f, 0.0f);
        glRotatef(yaw + 180.0f, 0.0f, 1.0f, 0.0f);
        glTranslatef(-cam_x, -cam_y, -cam_z);
    } else {
        apply_view_bobbing(partial);
        glTranslatef(0.0f, 0.0f, -0.1f); /* first-person camera nudge from kq.java */
        glRotatef(pitch, 1.0f, 0.0f, 0.0f);
        glRotatef(yaw + 180.0f, 0.0f, 1.0f, 0.0f);
        glTranslatef(-x, -y, -z);
    }
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

static void draw_textured_cube_tile(float x, float y, float z, int tile) {
    float x0 = x, x1 = x + 1.0f;
    float y0 = y, y1 = y + 1.0f;
    float z0 = z, z1 = z + 1.0f;
    float u0,v0,u1,v1;
    terrain_tile_uv(tile, &u0, &v0, &u1, &v1);
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

static void draw_bedrock_block(float x, float y, float z) {
    /* Beta terrain.png bedrock tile is index 17. */
    draw_textured_cube_tile(x, y, z, 17);
}

static void draw_world_block_id(int id, float x, float y, float z) {
    if (id == BLOCK_GRASS) draw_grass_block(x, y, z);
    else if (id == BLOCK_DIRT) draw_dirt_block(x, y, z);
    else if (id == BLOCK_BEDROCK) draw_bedrock_block(x, y, z);
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

        if ((e->stack.id == BLOCK_GRASS || e->stack.id == BLOCK_DIRT || e->stack.id == BLOCK_BEDROCK) && tex_terrain.id) {
            glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
            glScalef(0.25f, 0.25f, 0.25f);
            draw_world_block_id(e->stack.id, -0.5f, -0.5f, -0.5f);
        } else if (tex_items.id) {
            /* Source-style dropped non-block item: use the same extruded
               ItemRenderer geometry as held tools, not terrain cube rendering. */
            glScalef(0.18f, 0.18f, 0.18f);
            draw_source_item_3d_from_atlas(item_icon_tile(e->stack.id));
        }

        glPopMatrix();
    }
}

static void draw_block_selection_border(void) {
    FlatRayHit hit = flat_raycast();
    if (!hit.hit || flat_get_block(hit.bx, hit.by, hit.bz) == 0) return;

    float e = 0.002f;
    float x0 = (float)hit.bx - e;
    float y0 = (float)hit.by - e;
    float z0 = (float)hit.bz - e;
    float x1 = (float)hit.bx + 1.0f + e;
    float y1 = (float)hit.by + 1.0f + e;
    float z1 = (float)hit.bz + 1.0f + e;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);
    glLineWidth(2.0f);
    glColor4f(0.0f, 0.0f, 0.0f, 0.45f);

    glBegin(GL_LINE_STRIP);
    glVertex3f(x0,y0,z0); glVertex3f(x1,y0,z0); glVertex3f(x1,y0,z1); glVertex3f(x0,y0,z1); glVertex3f(x0,y0,z0);
    glEnd();

    glBegin(GL_LINE_STRIP);
    glVertex3f(x0,y1,z0); glVertex3f(x1,y1,z0); glVertex3f(x1,y1,z1); glVertex3f(x0,y1,z1); glVertex3f(x0,y1,z0);
    glEnd();

    glBegin(GL_LINES);
    glVertex3f(x0,y0,z0); glVertex3f(x0,y1,z0);
    glVertex3f(x1,y0,z0); glVertex3f(x1,y1,z0);
    glVertex3f(x1,y0,z1); glVertex3f(x1,y1,z1);
    glVertex3f(x0,y0,z1); glVertex3f(x0,y1,z1);
    glEnd();

    glLineWidth(1.0f);
    glDepthMask(GL_TRUE);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1,1,1,1);
}


static int flat_face_exposed(int x, int y, int z, int face) {
    if (face == 0) {
        if (y <= FLAT_WORLD_Y_MIN) return 0; /* bottom of bedrock never needs drawing */
        return flat_get_block(x, y - 1, z) == 0;
    }
    if (face == 1) return flat_get_block(x, y + 1, z) == 0;
    if (face == 2) return flat_get_block(x, y, z - 1) == 0;
    if (face == 3) return flat_get_block(x, y, z + 1) == 0;
    if (face == 4) return flat_get_block(x - 1, y, z) == 0;
    if (face == 5) return flat_get_block(x + 1, y, z) == 0;
    return 0;
}

static void world_face_style(int id, int face, int *tile) {
    float shade = 1.0f;
    if (face == 0) shade = 0.50f;
    else if (face == 2 || face == 3) shade = 0.80f;
    else if (face == 4 || face == 5) shade = 0.60f;

    if (id == BLOCK_GRASS) {
        if (face == 1) { *tile = 0; world_set_color_shade(0x6FAD3A, shade); return; }
        if (face == 0) { *tile = 2; world_set_shade(shade); return; }
        *tile = 3; world_set_shade(shade); return;
    }
    if (id == BLOCK_BEDROCK) { *tile = 17; world_set_shade(shade); return; }
    *tile = 2;
    world_set_shade(shade);
}

static void draw_world_block_face(int id, int x, int y, int z, int face) {
    float x0 = (float)x, x1 = (float)x + 1.0f;
    float y0 = (float)y, y1 = (float)y + 1.0f;
    float z0 = (float)z, z1 = (float)z + 1.0f;
    float u0,v0,u1,v1;
    int tile = 2;
    world_face_style(id, face, &tile);
    terrain_tile_uv(tile, &u0, &v0, &u1, &v1);
    glBegin(GL_QUADS);
    if (face == 1) { /* top */
        world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y1, z1, u1, v1); world_tex_vertex(x0, y1, z1, u0, v1);
    } else if (face == 0) { /* bottom */
        world_tex_vertex(x0, y0, z1, u0, v0); world_tex_vertex(x1, y0, z1, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    } else if (face == 2) { /* north z- */
        world_tex_vertex(x1, y1, z0, u0, v0); world_tex_vertex(x0, y1, z0, u1, v0); world_tex_vertex(x0, y0, z0, u1, v1); world_tex_vertex(x1, y0, z0, u0, v1);
    } else if (face == 3) { /* south z+ */
        world_tex_vertex(x0, y1, z1, u0, v0); world_tex_vertex(x1, y1, z1, u1, v0); world_tex_vertex(x1, y0, z1, u1, v1); world_tex_vertex(x0, y0, z1, u0, v1);
    } else if (face == 4) { /* west x- */
        world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x0, y1, z1, u1, v0); world_tex_vertex(x0, y0, z1, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    } else if (face == 5) { /* east x+ */
        world_tex_vertex(x1, y1, z1, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x1, y0, z1, u0, v1);
    }
    glEnd();
}

static void draw_world_block_exposed(int id, int x, int y, int z) {
    if (id == 0) return;
    for (int face = 0; face < 6; face++) {
        if (flat_face_exposed(x, y, z, face)) draw_world_block_face(id, x, y, z, face);
    }
}

static void rebuild_flat_world_geometry_list(void) {
    if (!tex_terrain.id) return;
    if (g_flat_world_display_list == 0) g_flat_world_display_list = glGenLists(1);
    if (g_flat_world_display_list == 0) return;

    glNewList(g_flat_world_display_list, GL_COMPILE);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
        for (int z = FLAT_WORLD_MIN; z <= FLAT_WORLD_MAX; z++) {
            for (int x = FLAT_WORLD_MIN; x <= FLAT_WORLD_MAX; x++) {
                int id = flat_get_block(x, y, z);
                if (id) draw_world_block_exposed(id, x, y, z);
            }
        }
    }
    glEndList();
    g_flat_world_geometry_dirty = 0;
}


static void draw_third_person_player(void) {
    if (!g_third_person_view || !tex_steve.id) return;

    float x = g_player_prev_x + (g_player_x - g_player_prev_x) * g_frame_partial;
    float eye_y = g_player_prev_y + (g_player_y - g_player_prev_y) * g_frame_partial;
    float z = g_player_prev_z + (g_player_z - g_player_prev_z) * g_frame_partial;
    float yaw = lerp_angle(g_player_prev_yaw, g_player_yaw, g_frame_partial);
    float pitch = g_player_prev_pitch + (g_player_pitch - g_player_prev_pitch) * g_frame_partial;
    float feet_y = eye_y - 1.62f;

    /* Source reference: dh.java model animation:
       arms/legs use cos(limbSwing*0.6662) with opposite phase. */
    float move = g_prev_limb_swing_amount + (g_limb_swing_amount - g_prev_limb_swing_amount) * g_frame_partial;
    if (move < 0.0f) move = 0.0f;
    if (move > 1.0f) move = 1.0f;
    float limb = g_prev_limb_swing + (g_limb_swing - g_prev_limb_swing) * g_frame_partial;
    float idle = (float)g_ingame_ticks + g_frame_partial;

    float right_arm_pitch = cosf(limb * 0.6662f + (float)M_PI) * 2.0f * move * 0.5f * 57.29578f;
    float left_arm_pitch  = cosf(limb * 0.6662f) * 2.0f * move * 0.5f * 57.29578f;
    float right_leg_pitch = cosf(limb * 0.6662f) * 1.4f * move * 57.29578f;
    float left_leg_pitch  = cosf(limb * 0.6662f + (float)M_PI) * 1.4f * move * 57.29578f;

    float right_arm_roll = (cosf(idle * 0.09f) * 0.05f + 0.05f) * 57.29578f;
    float left_arm_roll = -right_arm_roll;
    right_arm_pitch += sinf(idle * 0.067f) * 0.05f * 57.29578f;
    left_arm_pitch -= sinf(idle * 0.067f) * 0.05f * 57.29578f;

    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_steve.id);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);

    /* RenderLiving/ei.java style transform used by the inventory model too:
       translate to entity feet, rotate 180-yaw, scale model, offset by 24px. */
    glTranslatef(x, feet_y, z);
    glRotatef(180.0f - yaw, 0.0f, 1.0f, 0.0f);
    glScalef(-1.0f, -1.0f, 1.0f);
    glTranslatef(0.0f, -24.0f * 0.0625f - 0.0078125f, 0.0f);

    glColor4f(1, 1, 1, 1);
    steve_part(16, 16, 0, 0, 0, -4, 0, -2, 8, 12, 4, 0.0f, 0, 0, 0, 0);
    steve_part(40, 16, -5, 2, 0, -3, -2, -2, 4, 12, 4, 0.0f, 0, right_arm_pitch, 0, right_arm_roll);
    steve_part(40, 16,  5, 2, 0, -1, -2, -2, 4, 12, 4, 0.0f, 1, left_arm_pitch, 0, left_arm_roll);
    steve_part(0, 16, -2, 12, 0, -2, 0, -2, 4, 12, 4, 0.0f, 0, right_leg_pitch, 0, 0);
    steve_part(0, 16,  2, 12, 0, -2, 0, -2, 4, 12, 4, 0.0f, 1, left_leg_pitch, 0, 0);
    steve_part(0, 0, 0, 0, 0, -4, -8, -4, 8, 8, 8, 0.0f, 0, pitch, 0, 0);
    steve_part(32, 0, 0, 0, 0, -4, -8, -4, 8, 8, 8, 0.5f, 0, pitch, 0, 0);

    glDisable(GL_ALPHA_TEST);
    glColor4f(1, 1, 1, 1);
    glPopMatrix();
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

    /* Optimized path: the 100x100x64 world no longer walks and draws every
       solid cube every frame.  We compile only exposed faces into one legacy
       OpenGL display list and rebuild that list only when blocks change. */
    if (g_flat_world_geometry_dirty || g_flat_world_display_list == 0) {
        rebuild_flat_world_geometry_list();
    }
    if (g_flat_world_display_list != 0) {
        glCallList(g_flat_world_display_list);
    }

    draw_third_person_player();
    draw_block_selection_border();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
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


static void draw_source_item_3d_from_atlas(int tile) {
    /* Port of uploaded source lk.java lines 16-118 for non-block items.
       Source does: bind /gui/items.png, compute icon UVs, translate down,
       scale 1.5, rotate Y=50/Z=335, translate -0.9375/-0.0625, then draw
       front, back, and 16-strip side thickness with depth 1/16. */
    float tw = tex_items.w ? (float)tex_items.w : 256.0f;
    float th = tex_items.h ? (float)tex_items.h : 256.0f;
    float u0 = (float)((tile & 15) * 16 + 0.0f) / tw;
    float u1 = (float)((tile & 15) * 16 + 15.99f) / tw;
    float v0 = (float)((tile >> 4) * 16 + 0.0f) / th;
    float v1 = (float)((tile >> 4) * 16 + 15.99f) / th;
    float w = 1.0f;
    float depth = 0.0625f;

    glBindTexture(GL_TEXTURE_2D, tex_items.id);
    glPushMatrix();
    glTranslatef(-0.0f, -0.3f, 0.0f);
    glScalef(1.5f, 1.5f, 1.5f);
    glRotatef(50.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(335.0f, 0.0f, 0.0f, 1.0f);
    glTranslatef(-0.9375f, -0.0625f, 0.0f);

    glColor4f(1, 1, 1, 1);

    glBegin(GL_QUADS);
    /* front */
    glTexCoord2f(u1, v1); glVertex3f(0.0f, 0.0f, 0.0f);
    glTexCoord2f(u0, v1); glVertex3f(w,    0.0f, 0.0f);
    glTexCoord2f(u0, v0); glVertex3f(w,    1.0f, 0.0f);
    glTexCoord2f(u1, v0); glVertex3f(0.0f, 1.0f, 0.0f);

    /* back */
    glTexCoord2f(u1, v0); glVertex3f(0.0f, 1.0f, -depth);
    glTexCoord2f(u0, v0); glVertex3f(w,    1.0f, -depth);
    glTexCoord2f(u0, v1); glVertex3f(w,    0.0f, -depth);
    glTexCoord2f(u1, v1); glVertex3f(0.0f, 0.0f, -depth);
    glEnd();

    /* left/right thickness: source uses 16 strips for pixel-thick sides. */
    glBegin(GL_QUADS);
    for (int i = 0; i < 16; i++) {
        float a = (float)i / 16.0f;
        float uu = u1 + (u0 - u1) * a - 0.001953125f;
        float x = w * a;
        glTexCoord2f(uu, v1); glVertex3f(x, 0.0f, -depth);
        glTexCoord2f(uu, v1); glVertex3f(x, 0.0f, 0.0f);
        glTexCoord2f(uu, v0); glVertex3f(x, 1.0f, 0.0f);
        glTexCoord2f(uu, v0); glVertex3f(x, 1.0f, -depth);
    }
    for (int i = 0; i < 16; i++) {
        float a = (float)i / 16.0f;
        float uu = u1 + (u0 - u1) * a - 0.001953125f;
        float x = w * a + 0.0625f;
        glTexCoord2f(uu, v0); glVertex3f(x, 1.0f, -depth);
        glTexCoord2f(uu, v0); glVertex3f(x, 1.0f, 0.0f);
        glTexCoord2f(uu, v1); glVertex3f(x, 0.0f, 0.0f);
        glTexCoord2f(uu, v1); glVertex3f(x, 0.0f, -depth);
    }
    for (int i = 0; i < 16; i++) {
        float a = (float)i / 16.0f;
        float vv = v1 + (v0 - v1) * a - 0.001953125f;
        float y = w * a + 0.0625f;
        glTexCoord2f(u1, vv); glVertex3f(0.0f, y, 0.0f);
        glTexCoord2f(u0, vv); glVertex3f(w,    y, 0.0f);
        glTexCoord2f(u0, vv); glVertex3f(w,    y, -depth);
        glTexCoord2f(u1, vv); glVertex3f(0.0f, y, -depth);
    }
    for (int i = 0; i < 16; i++) {
        float a = (float)i / 16.0f;
        float vv = v1 + (v0 - v1) * a - 0.001953125f;
        float y = w * a;
        glTexCoord2f(u0, vv); glVertex3f(w,    y, 0.0f);
        glTexCoord2f(u1, vv); glVertex3f(0.0f, y, 0.0f);
        glTexCoord2f(u1, vv); glVertex3f(0.0f, y, -depth);
        glTexCoord2f(u0, vv); glVertex3f(w,    y, -depth);
    }
    glEnd();

    glPopMatrix();
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
        float swing_y = sinf(swing * swing * (float)M_PI);
        glRotatef(-swing_y * 20.0f, 0.0f, 1.0f, 0.0f);
        glRotatef(-swing_sqrt_sin * 20.0f, 0.0f, 0.0f, 1.0f);
        glRotatef(-swing_sqrt_sin * 80.0f, 1.0f, 0.0f, 0.0f);
        glScalef(0.40f, 0.40f, 0.40f);
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        draw_world_block_id(held->id, -0.5f, -0.5f, -0.5f);
        glPopMatrix();
    } else if (!stack_empty(held) && tex_items.id) {
        /* Non-block first-person item branch, using uploaded source lk.java
           lines 16-118 for the actual extruded item geometry. */
        int tile = item_icon_tile(held->id);
        glPushMatrix();
        float s = 0.8f;
        glTranslatef(-swing_sqrt_sin * 0.4f,
                     sinf(sqrtf(swing) * (float)M_PI * 2.0f) * 0.2f,
                     -swing_sin * 0.2f);
        glTranslatef(0.7f * s, -0.65f * s, -0.9f * s);
        glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
        float swing_y = sinf(swing * swing * (float)M_PI);
        glRotatef(-swing_y * 20.0f, 0.0f, 1.0f, 0.0f);
        glRotatef(-swing_sqrt_sin * 20.0f, 0.0f, 0.0f, 1.0f);
        glRotatef(-swing_sqrt_sin * 80.0f, 1.0f, 0.0f, 0.0f);
        glScalef(0.40f, 0.40f, 0.40f);
        draw_source_item_3d_from_atlas(tile);
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
    if (with_hand && !g_third_person_view) draw_first_person_hand();
    setup_gui_projection();
}

