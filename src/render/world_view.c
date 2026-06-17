/* Split from original monolithic main.c. Included by src/main.c unity build. */

static void draw_source_item_3d_from_atlas(int tile);

static void draw_sky_only(void) {
    /* Keep the sky blue all the way down.  The extra translucent white pass made
       a huge washed-out band at the horizon, especially when looking upward
       through clouds or water. */
    glDisable(GL_BLEND);
    draw_gradient(0, 0, g_gui_w, g_gui_h, 0xFF78A7FF, 0xFF8FBEFF);
    glEnable(GL_BLEND);
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


static float current_render_distance_blocks(void) {
    int chunks = g_opts.render_distance;
    if (chunks < 2) chunks = 2;
    if (chunks > 32) chunks = 32;
    return (float)(chunks * 16);
}

static void apply_source_like_fog(void) {
    float end = current_render_distance_blocks();
    float start = end * 0.25f;
    float fog_col[4] = {0.49f, 0.65f, 1.0f, 1.0f};

    /* Visual underwater/lava fog follows the camera/head, not body collision.
       This stops shallow water from tinting the screen while the player can
       still breathe and see above the surface. */
    if (flat_player_head_in_water()) {
        fog_col[0] = 0.05f; fog_col[1] = 0.20f; fog_col[2] = 0.70f;
        start = 0.0f; end = 24.0f;
    } else if (flat_player_head_in_lava()) {
        fog_col[0] = 0.60f; fog_col[1] = 0.10f; fog_col[2] = 0.00f;
        start = 0.0f; end = 8.0f;
    }

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, fog_col);
    glFogf(GL_FOG_START, start);
    glFogf(GL_FOG_END, end);
}

static unsigned int cloud_hash(int cx, int cz) {
    unsigned int h = (unsigned int)cx * 0x8da6b343u;
    h ^= (unsigned int)cz * 0xd8163841u;
    h ^= 0x5a5a5a5au;
    h ^= h >> 13;
    h *= 0x85ebca6bu;
    h ^= h >> 16;
    return h;
}

static void draw_cloud_quad(float x0, float x1, float y, float z0, float z1, float alpha) {
    glColor4f(0.92f, 0.96f, 1.0f, alpha);
    glVertex3f(x0, y, z0);
    glVertex3f(x1, y, z0);
    glVertex3f(x1, y, z1);
    glVertex3f(x0, y, z1);
}

static void draw_source_clouds(void) {
    /* Cheap fixed cloud sheet.
       The previous version built many overlapping translucent 3-D boxes every
       frame.  Looking up through them blended layer-on-layer (the "clouds
       generating on themselves" look) and made clouds the new FPS bottleneck.

       Keep the pattern in cloud-space, move only the final world positions, and
       draw a single flat batch.  Clouds stay independent from terrain render
       distance without hundreds of glBegin/glEnd calls or alpha-stacked boxes. */
    const float cloud_y = 96.0f;
    const float tile = 32.0f;
    const float cloud_radius = 192.0f;
    const float scroll = ((float)g_ingame_ticks + g_frame_partial) * 0.02f;

    float px = g_player_prev_x + (g_player_x - g_player_prev_x) * g_frame_partial;
    float py = g_player_prev_y + (g_player_y - g_player_prev_y) * g_frame_partial;
    float pz = g_player_prev_z + (g_player_z - g_player_prev_z) * g_frame_partial;

    int min_cx = (int)floorf((px - scroll - cloud_radius) / tile) - 1;
    int max_cx = (int)floorf((px - scroll + cloud_radius) / tile) + 1;
    int min_cz = (int)floorf((pz - cloud_radius) / tile) - 1;
    int max_cz = (int)floorf((pz + cloud_radius) / tile) + 1;

    glDisable(GL_FOG);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glBegin(GL_QUADS);
    for (int cz = min_cz; cz <= max_cz; cz++) {
        for (int cx = min_cx; cx <= max_cx; cx++) {
            unsigned int h = cloud_hash(cx, cz);
            if ((h & 7u) < 3u) continue;

            float x0 = (float)cx * tile + scroll;
            float z0 = (float)cz * tile;
            float x1 = x0 + tile;
            float z1 = z0 + tile;

            float mx = (x0 + x1) * 0.5f - px;
            float mz = (z0 + z1) * 0.5f - pz;
            if (mx * mx + mz * mz > cloud_radius * cloud_radius) continue;

            /* A single sheet, not a pile: no overlapping sub-boxes, no sides
               through the player's view, and no per-cloud glBegin/glEnd. */
            float alpha = (py > cloud_y) ? 0.52f : 0.40f;
            draw_cloud_quad(x0, x1, cloud_y, z0, z1, alpha);
        }
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_FOG);
    glColor4f(1,1,1,1);
}


static void setup_world_projection(void) {
    glViewport(0, 0, g_win_w, g_win_h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (g_win_h > 0) ? ((float)g_win_w / (float)g_win_h) : 1.0f;
    {
        float far_plane = current_render_distance_blocks() + 64.0f;
        if (far_plane < 384.0f) far_plane = 384.0f; /* fixed cloud layer is independent from terrain distance */
        gluPerspective(70.0, (double)aspect, 0.05, (double)far_plane);
    }
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
    if (g_screen == SCREEN_INGAME && key_down_vk(g_opts.keys[5])) y -= 0.18f;

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


static int dig_particle_tile_for_block_face(int id, int face) {
    if (id == BLOCK_GRASS) {
        if (face == 1) return 0;
        if (face == 0) return 2;
        return 3;
    }
    if (id == BLOCK_WATER || id == BLOCK_STILL_WATER) return 14;
    if (id == BLOCK_LAVA || id == BLOCK_STILL_LAVA) return 30;
    if (id == BLOCK_WATER || id == BLOCK_STILL_WATER) return 14;
    if (id == BLOCK_LAVA || id == BLOCK_STILL_LAVA) return 30;
    if (id == BLOCK_STONE) return 1;
    if (id == BLOCK_DIRT) return 2;
    if (id == BLOCK_COBBLESTONE) return 16;
    if (id == BLOCK_BEDROCK) return 17;
    if (id == BLOCK_PLANKS) return 4;
    if (id == BLOCK_LOG) return (face == 0 || face == 1) ? 21 : 20;
    if (id == BLOCK_CRAFTING_TABLE) {
        if (face == 1) return 43;
        if (face == 0) return 4;
        if (face == 2 || face == 3) return 59;
        return 60;
    }
    if (id == BLOCK_CHEST) {
        if (face == 1) return 68;
        if (face == 3) return 70;
        return 69;
    }

    if (id == BLOCK_FURNACE) {
        if (face == 1) return 46;
        if (face == 3) return 44;
        return 45;
    }
    return 2;
}

static int dig_particle_tile_for_block(int id) {
    if (id == BLOCK_GRASS) return 3;
    if (id == BLOCK_STONE) return 1;
    if (id == BLOCK_DIRT) return 2;
    if (id == BLOCK_COBBLESTONE) return 16;
    if (id == BLOCK_BEDROCK) return 17;
    if (id == BLOCK_PLANKS) return 4;
    if (id == BLOCK_LOG) return 20;
    if (id == BLOCK_CRAFTING_TABLE) return 59;
    if (id == BLOCK_FURNACE) return 45;
    return 2;
}

static void dig_particle_color_for_block(int id, float *r, float *g, float *b) {
    /* EntityDiggingFX sets all block particles to 0.6; grass skips colorMultiplier. */
    *r = 0.6f; *g = 0.6f; *b = 0.6f;
}

static float frand01(void) {
    return (float)rand() / (float)RAND_MAX;
}

static void add_dig_particle(float x, float y, float z, float mx, float my, float mz, int tile, int block_id, float motion_scale, float size_scale) {
    DigParticle *p = &g_dig_particles[g_next_dig_particle++ % MAX_DIG_PARTICLES];
    memset(p, 0, sizeof(*p));
    p->active = 1;
    p->tile = tile;
    p->x = p->prev_x = x;
    p->y = p->prev_y = y;
    p->z = p->prev_z = z;
    p->mx = mx + (frand01() * 2.0f - 1.0f) * 0.4f;
    p->my = my + (frand01() * 2.0f - 1.0f) * 0.4f;
    p->mz = mz + (frand01() * 2.0f - 1.0f) * 0.4f;

    float mag = sqrtf(p->mx * p->mx + p->my * p->my + p->mz * p->mz);
    if (mag < 0.0001f) mag = 0.0001f;
    float speed = (frand01() + frand01() + 1.0f) * 0.15f;
    p->mx = p->mx / mag * speed * 0.4f;
    p->my = p->my / mag * speed * 0.4f + 0.1f;
    p->mz = p->mz / mag * speed * 0.4f;

    /* EntityFX.func_407_b(0.2F) for hit particles. */
    p->mx *= motion_scale;
    p->my = (p->my - 0.1f) * motion_scale + 0.1f;
    p->mz *= motion_scale;

    p->tex_u_jitter = frand01() * 3.0f;
    p->tex_v_jitter = frand01() * 3.0f;
    p->scale = ((frand01() * 0.5f + 0.5f) * 2.0f) * 0.5f * size_scale; /* digging halves EntityFX scale */
    p->gravity = 1.0f;
    p->max_age = (int)(4.0f / (frand01() * 0.9f + 0.1f));
    if (p->max_age < 1) p->max_age = 1;
    p->age = 0;
    dig_particle_color_for_block(block_id, &p->r, &p->g, &p->b);
}

static void spawn_block_destroy_particles(int bx, int by, int bz, int block_id) {
    /* Exact source structure: EffectRenderer.func_1186_a uses a 4x4x4 grid.
       pos = block + (i+0.5)/4, motion = pos - block - 0.5. */
    const int n = 4;
    int tile = dig_particle_tile_for_block(block_id);
    for (int ix = 0; ix < n; ix++) {
        for (int iy = 0; iy < n; iy++) {
            for (int iz = 0; iz < n; iz++) {
                float x = (float)bx + ((float)ix + 0.5f) / (float)n;
                float y = (float)by + ((float)iy + 0.5f) / (float)n;
                float z = (float)bz + ((float)iz + 0.5f) / (float)n;
                add_dig_particle(x, y, z,
                                 x - (float)bx - 0.5f,
                                 y - (float)by - 0.5f,
                                 z - (float)bz - 0.5f,
                                 tile, block_id, 1.0f, 1.0f);
            }
        }
    }
}

static void spawn_block_hit_particle(int bx, int by, int bz, int face, int block_id) {
    /* Port of EffectRenderer.func_1191_a with Block min/max = full cube and
       then EntityFX.func_407_b(0.2F).func_405_d(0.6F). */
    const float eps = 0.1f;
    float x = (float)bx + frand01() * (1.0f - eps * 2.0f) + eps;
    float y = (float)by + frand01() * (1.0f - eps * 2.0f) + eps;
    float z = (float)bz + frand01() * (1.0f - eps * 2.0f) + eps;
    if (face == 0) y = (float)by - eps;
    if (face == 1) y = (float)by + 1.0f + eps;
    if (face == 2) z = (float)bz - eps;
    if (face == 3) z = (float)bz + 1.0f + eps;
    if (face == 4) x = (float)bx - eps;
    if (face == 5) x = (float)bx + 1.0f + eps;
    add_dig_particle(x, y, z, 0.0f, 0.0f, 0.0f,
                     dig_particle_tile_for_block_face(block_id, face), block_id,
                     0.2f, 0.6f);
}

static void update_dig_particles(void) {
    for (int i = 0; i < MAX_DIG_PARTICLES; i++) {
        DigParticle *p = &g_dig_particles[i];
        if (!p->active) continue;
        p->prev_x = p->x;
        p->prev_y = p->y;
        p->prev_z = p->z;
        if (p->age++ >= p->max_age) {
            p->active = 0;
            continue;
        }

        p->my -= 0.04f * p->gravity;
        p->x += p->mx;
        p->y += p->my;
        p->z += p->mz;

        /* Simple ground collision to match EntityFX damping when onGround. */
        if (p->y < 0.02f) {
            p->y = 0.02f;
            p->my = 0.0f;
            p->mx *= 0.7f;
            p->mz *= 0.7f;
        }

        p->mx *= 0.98f;
        p->my *= 0.98f;
        p->mz *= 0.98f;
    }
}

static void draw_dig_particles(float partial) {
    if (!tex_terrain.id) return;

    float yaw = lerp_angle(g_player_prev_yaw, g_player_yaw, partial) * (float)M_PI / 180.0f;
    float pitch = (g_player_prev_pitch + (g_player_pitch - g_player_prev_pitch) * partial) * (float)M_PI / 180.0f;
    float cos_yaw = cosf(yaw);
    float sin_yaw = sinf(yaw);
    float x_rot = -sin_yaw * sinf(pitch);
    float z_rot = cos_yaw * sinf(pitch);
    float yz_rot = cosf(pitch);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    glDisable(GL_CULL_FACE);

    glBegin(GL_QUADS);
    for (int i = 0; i < MAX_DIG_PARTICLES; i++) {
        DigParticle *p = &g_dig_particles[i];
        if (!p->active) continue;

        float u0, v0, u1, v1;
        terrain_tile_uv(p->tile, &u0, &v0, &u1, &v1);

        /* EntityDiggingFX draws a 1/4 sub-cell of the terrain tile. */
        float tile_w = (u1 - u0);
        float tile_h = (v1 - v0);
        float su0 = u0 + tile_w * (p->tex_u_jitter / 4.0f);
        float sv0 = v0 + tile_h * (p->tex_v_jitter / 4.0f);
        float su1 = su0 + tile_w * (0.999f / 4.0f);
        float sv1 = sv0 + tile_h * (0.999f / 4.0f);

        float px = p->prev_x + (p->x - p->prev_x) * partial;
        float py = p->prev_y + (p->y - p->prev_y) * partial;
        float pz = p->prev_z + (p->z - p->prev_z) * partial;
        float q = 0.1f * p->scale;

        glColor4f(p->r, p->g, p->b, 1.0f);
        world_tex_vertex(px - cos_yaw * q - x_rot * q, py - yz_rot * q, pz - sin_yaw * q - z_rot * q, su0, sv1);
        world_tex_vertex(px - cos_yaw * q + x_rot * q, py + yz_rot * q, pz - sin_yaw * q + z_rot * q, su0, sv0);
        world_tex_vertex(px + cos_yaw * q + x_rot * q, py + yz_rot * q, pz + sin_yaw * q + z_rot * q, su1, sv0);
        world_tex_vertex(px + cos_yaw * q - x_rot * q, py - yz_rot * q, pz + sin_yaw * q - z_rot * q, su1, sv1);
    }
    glEnd();

    glColor4f(1, 1, 1, 1);
    glDisable(GL_ALPHA_TEST);
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
    draw_textured_cube_tile(x, y, z, 17); /* Bare Bones bedrock */
}

static void draw_planks_block(float x, float y, float z) {
    draw_textured_cube_tile(x, y, z, 4); /* terrain tile 4 = wooden planks */
}

static void draw_log_block(float x, float y, float z) {
    /* Classic terrain.png:
       20 = log side, 21 = log top/end. */
    float x0 = x, x1 = x + 1.0f;
    float y0 = y, y1 = y + 1.0f;
    float z0 = z, z1 = z + 1.0f;
    float u0, v0, u1, v1;

    terrain_tile_uv(21, &u0, &v0, &u1, &v1);
    world_set_shade(1.0f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y1, z1, u1, v1); world_tex_vertex(x0, y1, z1, u0, v1);
    world_tex_vertex(x0, y0, z1, u0, v0); world_tex_vertex(x1, y0, z1, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    glEnd();

    terrain_tile_uv(20, &u0, &v0, &u1, &v1);
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
}

static void draw_crafting_table_block(float x, float y, float z) {
    /* Classic terrain.png: 43=crafting table top, 59=side, 60=front. */
    float x0 = x, x1 = x + 1.0f;
    float y0 = y, y1 = y + 1.0f;
    float z0 = z, z1 = z + 1.0f;
    float u0, v0, u1, v1;

    terrain_tile_uv(43, &u0, &v0, &u1, &v1);
    world_set_shade(1.0f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y1, z1, u1, v1); world_tex_vertex(x0, y1, z1, u0, v1);
    glEnd();

    terrain_tile_uv(59, &u0, &v0, &u1, &v1);
    world_set_shade(0.80f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z1, u0, v0); world_tex_vertex(x1, y1, z1, u1, v0); world_tex_vertex(x1, y0, z1, u1, v1); world_tex_vertex(x0, y0, z1, u0, v1);
    world_tex_vertex(x1, y1, z0, u0, v0); world_tex_vertex(x0, y1, z0, u1, v0); world_tex_vertex(x0, y0, z0, u1, v1); world_tex_vertex(x1, y0, z0, u0, v1);
    glEnd();

    terrain_tile_uv(60, &u0, &v0, &u1, &v1);
    world_set_shade(0.60f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x0, y1, z1, u1, v0); world_tex_vertex(x0, y0, z1, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    world_tex_vertex(x1, y1, z1, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x1, y0, z1, u0, v1);
    glEnd();

    terrain_tile_uv(4, &u0, &v0, &u1, &v1);
    world_set_shade(0.50f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y0, z1, u0, v0); world_tex_vertex(x1, y0, z1, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    glEnd();
}


static void draw_chest_block(float x, float y, float z) {
    /* Beta-style chest is smaller than a full cube. */
    float inset = 1.0f / 16.0f;
    float x0 = x + inset, x1 = x + 1.0f - inset;
    float y0 = y, y1 = y + 14.0f / 16.0f;
    float z0 = z + inset, z1 = z + 1.0f - inset;
    float u0, v0, u1, v1;

    terrain_tile_uv(68, &u0, &v0, &u1, &v1);
    world_set_shade(1.0f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y1, z1, u1, v1); world_tex_vertex(x0, y1, z1, u0, v1);
    glEnd();

    terrain_tile_uv(69, &u0, &v0, &u1, &v1);
    world_set_shade(0.50f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y0, z1, u0, v0); world_tex_vertex(x1, y0, z1, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    glEnd();

    terrain_tile_uv(70, &u0, &v0, &u1, &v1);
    world_set_shade(0.80f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z1, u0, v0); world_tex_vertex(x1, y1, z1, u1, v0); world_tex_vertex(x1, y0, z1, u1, v1); world_tex_vertex(x0, y0, z1, u0, v1);
    glEnd();

    terrain_tile_uv(69, &u0, &v0, &u1, &v1);
    world_set_shade(0.80f);
    glBegin(GL_QUADS);
    world_tex_vertex(x1, y1, z0, u0, v0); world_tex_vertex(x0, y1, z0, u1, v0); world_tex_vertex(x0, y0, z0, u1, v1); world_tex_vertex(x1, y0, z0, u0, v1);
    glEnd();

    world_set_shade(0.60f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x0, y1, z1, u1, v0); world_tex_vertex(x0, y0, z1, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    world_tex_vertex(x1, y1, z1, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x1, y0, z1, u0, v1);
    glEnd();
}

static void draw_furnace_block(float x, float y, float z) {
    /* Prepared from Bare Bones: 44=front, 45=side, 46=top. */
    float x0 = x, x1 = x + 1.0f;
    float y0 = y, y1 = y + 1.0f;
    float z0 = z, z1 = z + 1.0f;
    float u0, v0, u1, v1;

    terrain_tile_uv(46, &u0, &v0, &u1, &v1);
    world_set_shade(1.0f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y1, z1, u1, v1); world_tex_vertex(x0, y1, z1, u0, v1);
    glEnd();

    terrain_tile_uv(45, &u0, &v0, &u1, &v1);
    world_set_shade(0.50f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y0, z1, u0, v0); world_tex_vertex(x1, y0, z1, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    glEnd();

    terrain_tile_uv(44, &u0, &v0, &u1, &v1);
    world_set_shade(0.80f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z1, u0, v0); world_tex_vertex(x1, y1, z1, u1, v0); world_tex_vertex(x1, y0, z1, u1, v1); world_tex_vertex(x0, y0, z1, u0, v1);
    glEnd();

    terrain_tile_uv(45, &u0, &v0, &u1, &v1);
    world_set_shade(0.80f);
    glBegin(GL_QUADS);
    world_tex_vertex(x1, y1, z0, u0, v0); world_tex_vertex(x0, y1, z0, u1, v0); world_tex_vertex(x0, y0, z0, u1, v1); world_tex_vertex(x1, y0, z0, u0, v1);
    glEnd();

    world_set_shade(0.60f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x0, y1, z1, u1, v0); world_tex_vertex(x0, y0, z1, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    world_tex_vertex(x1, y1, z1, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x1, y0, z1, u0, v1);
    glEnd();
}




static void draw_generic_world_block_model(int id, float x, float y, float z);

static void draw_world_block_id(int id, float x, float y, float z) {
    if (id == BLOCK_GRASS) draw_grass_block(x, y, z);
    else if (id == BLOCK_STONE) draw_textured_cube_tile(x, y, z, 1);
    else if (id == BLOCK_DIRT) draw_dirt_block(x, y, z);
    else if (id == BLOCK_COBBLESTONE) draw_textured_cube_tile(x, y, z, 16);
    else if (id == BLOCK_BEDROCK) draw_bedrock_block(x, y, z);
    else if (id == BLOCK_PLANKS) draw_planks_block(x, y, z);
    else if (id == BLOCK_LOG) draw_log_block(x, y, z);
    else if (id == BLOCK_CRAFTING_TABLE) draw_crafting_table_block(x, y, z);
    else if (id == BLOCK_FURNACE) draw_furnace_block(x, y, z);
    else if (id == BLOCK_CHEST) draw_chest_block(x, y, z);
    else if (id >= 1 && id <= 89) draw_generic_world_block_model(id, x, y, z);
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


static float dropped_item_shadow_y(float x, float y, float z) {
    int ix = (int)floorf(x);
    int iz = (int)floorf(z);
    int start = (int)floorf(y);
    if (start > FLAT_WORLD_Y_MAX) start = FLAT_WORLD_Y_MAX;
    for (int yy = start; yy >= FLAT_WORLD_Y_MIN; yy--) {
        if (flat_get_block(ix, yy, iz) != 0) return (float)yy + 1.002f;
    }
    return 0.002f;
}

static void draw_dropped_item_shadow(float x, float y, float z, float radius) {
    float sy = dropped_item_shadow_y(x, y, z);
    float dy = y - sy;
    if (dy < 0.0f || dy > 4.0f) return;
    float alpha = (1.0f - dy / 4.0f) * 0.35f;
    if (alpha <= 0.0f) return;
    float r = radius * (1.0f - dy / 8.0f);
    if (r < radius * 0.5f) r = radius * 0.5f;

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, alpha);

    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(x, sy, z);
    for (int i = 0; i <= 24; i++) {
        float a = (float)i * (float)M_PI * 2.0f / 24.0f;
        glVertex3f(x + cosf(a) * r, sy, z + sinf(a) * r);
    }
    glEnd();

    glEnable(GL_TEXTURE_2D);
    glColor4f(1,1,1,1);
}


static int render_item_as_block_id(int id) {
    return (id >= 1 && id <= 89);
}


static void item_tile_uv(int tile, float *u0, float *v0, float *u1, float *v1) {
    int tx = tile & 15;
    int ty = tile >> 4;
    float step = 1.0f / 16.0f;
    *u0 = tx * step;
    *v0 = ty * step;
    *u1 = *u0 + step;
    *v1 = *v0 + step;
}

static void draw_dropped_item_sprite(int tile) {
    if (!tex_items.id) return;
    float u0, v0, u1, v1;
    item_tile_uv(tile, &u0, &v0, &u1, &v1);

    glBindTexture(GL_TEXTURE_2D, tex_items.id);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v1); glVertex3f(-0.5f, -0.5f, 0.0f);
    glTexCoord2f(u1, v1); glVertex3f( 0.5f, -0.5f, 0.0f);
    glTexCoord2f(u1, v0); glVertex3f( 0.5f,  0.5f, 0.0f);
    glTexCoord2f(u0, v0); glVertex3f(-0.5f,  0.5f, 0.0f);
    glEnd();
    glDisable(GL_ALPHA_TEST);
}

static int dropped_item_copy_count(int count) {
    /* RenderItem.java: >1 -> 2, >5 -> 3, >20 -> 4. */
    if (count > 20) return 4;
    if (count > 5) return 3;
    if (count > 1) return 2;
    return 1;
}

static void dropped_item_copy_offset(int copy, float block_scale, float *x, float *y, float *z) {
    /* Deterministic stand-in for RenderItem's Random(187L) offsets. */
    static const float offsets[4][3] = {
        { 0.00f,  0.00f,  0.00f},
        { 0.12f,  0.05f, -0.09f},
        {-0.10f, -0.03f,  0.11f},
        { 0.04f, -0.08f,  0.06f}
    };
    *x = offsets[copy & 3][0] / block_scale;
    *y = offsets[copy & 3][1] / block_scale;
    *z = offsets[copy & 3][2] / block_scale;
}

static void draw_dropped_items(void) {
    float yaw = lerp_angle(g_player_prev_yaw, g_player_yaw, g_frame_partial);
    for (int i = 0; i < MAX_DROP_ENTITIES; i++) {
        FlatDroppedItem *e = &g_drops[i];
        if (!e->active) continue;
        float x = e->prev_x + (e->x - e->prev_x) * g_frame_partial;
        float y = e->prev_y + (e->y - e->prev_y) * g_frame_partial;
        float z = e->prev_z + (e->z - e->prev_z) * g_frame_partial;
        float bob = sinf(((float)e->age + g_frame_partial) / 10.0f + e->rot) * 0.1f + 0.1f;
        int copies = dropped_item_copy_count(e->stack.count);

        draw_dropped_item_shadow(x, y, z, 0.28f);

        glPushMatrix();
        glTranslatef(x, y + bob, z);
        glRotatef((((float)e->age + g_frame_partial) / 20.0f + e->rot) * 57.29578f, 0.0f, 1.0f, 0.0f);

        if (render_item_as_block_id(e->stack.id) && tex_terrain.id) {
            /* Source RenderItem: block drops are 0.25 scale 3D blocks. */
            const float block_scale = 0.25f;
            glScalef(block_scale, block_scale, block_scale);
            for (int c = 0; c < copies; c++) {
                glPushMatrix();
                float ox, oy, oz;
                dropped_item_copy_offset(c, block_scale, &ox, &oy, &oz);
                glTranslatef(ox * 0.45f, oy * 0.45f, oz * 0.45f);
                glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
                draw_world_block_id(e->stack.id, -0.5f, -0.5f, -0.5f);
                glPopMatrix();
            }
        } else if (tex_items.id) {
            /* Source RenderItem for normal items: billboarded icon, 0.5 scale.
               The previous extruded mesh made the scattered giant-line look. */
            glRotatef(180.0f - yaw, 0.0f, 1.0f, 0.0f);
            glScalef(0.5f, 0.5f, 0.5f);
            int tile = item_icon_tile(e->stack.id);
            for (int c = 0; c < copies; c++) {
                glPushMatrix();
                float ox, oy, oz;
                dropped_item_copy_offset(c, 0.5f, &ox, &oy, &oz);
                glTranslatef(ox * 0.25f, oy * 0.25f, 0.0f);
                draw_dropped_item_sprite(tile);
                glPopMatrix();
            }
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


static int block_occludes_render_face(int id) {
    if (id == 0 || block_is_liquid(id)) return 0;
    if (id == BLOCK_GLASS || id == BLOCK_LEAVES || id == BLOCK_SAPLING ||
        id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE ||
        id == BLOCK_BROWN_MUSHROOM || id == BLOCK_RED_MUSHROOM ||
        id == BLOCK_TORCH || id == BLOCK_SNOW_LAYER) return 0;
    return 1;
}

static int neighbor_for_face(int x, int y, int z, int face) {
    if (face == 0) return (y <= FLAT_WORLD_Y_MIN) ? BLOCK_BEDROCK : flat_get_block(x, y - 1, z);
    if (face == 1) return flat_get_block(x, y + 1, z);
    if (face == 2) return flat_get_block(x, y, z - 1);
    if (face == 3) return flat_get_block(x, y, z + 1);
    if (face == 4) return flat_get_block(x - 1, y, z);
    if (face == 5) return flat_get_block(x + 1, y, z);
    return BLOCK_BEDROCK;
}

static int flat_face_exposed(int x, int y, int z, int face) {
    if (face == 0 && y <= FLAT_WORLD_Y_MIN) return 0; /* bottom of bedrock never needs drawing */
    /* Treat water/glass/leaves as transparent occluders.  The old air-only test
       hid terrain faces adjacent to water; because water is blended, that made
       caves/empty space visible through it like an x-ray. */
    return !block_occludes_render_face(neighbor_for_face(x, y, z, face));
}

static int liquid_face_exposed(int x, int y, int z, int face) {
    int n = neighbor_for_face(x, y, z, face);
    if (block_is_liquid(n)) return 0;
    return !block_occludes_render_face(n);
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

    if (id == BLOCK_WATER || id == BLOCK_STILL_WATER) {
        *tile = 14;
        world_set_color_shade(0x3F76E4, shade);
        return;
    }

    if (id == BLOCK_LAVA || id == BLOCK_STILL_LAVA) {
        *tile = 30;
        world_set_color_shade(0xFF6A00, shade);
        return;
    }

    if (id == BLOCK_STONE) {
        *tile = 1;
        world_set_shade(shade);
        return;
    }

    if (id == BLOCK_DIRT) {
        *tile = 2;
        world_set_shade(shade);
        return;
    }

    if (id == BLOCK_COBBLESTONE) {
        *tile = 16;
        world_set_shade(shade);
        return;
    }

    if (id == BLOCK_BEDROCK) {
        *tile = 17;
        world_set_shade(shade);
        return;
    }

    if (id == BLOCK_PLANKS) {
        *tile = 4;
        world_set_shade(shade);
        return;
    }

    if (id == BLOCK_LOG) {
        /* Classic terrain layout: 20 = log side, 21 = log top/end. */
        *tile = (face == 0 || face == 1) ? 21 : 20;
        world_set_shade(shade);
        return;
    }

    if (id == BLOCK_CRAFTING_TABLE) {
        /* 43 = top, 60 = front-ish side, 59 = other sides, 4 = bottom/planks. */
        if (face == 1) *tile = 43;
        else if (face == 0) *tile = 4;
        else if (face == 2 || face == 3) *tile = 59;
        else *tile = 60;
        world_set_shade(shade);
        return;
    }

    if (id == BLOCK_CHEST) {
        if (face == 1) *tile = 68;
        else if (face == 3) *tile = 70;
        else *tile = 69;
        world_set_shade(shade);
        return;
    }

    if (id == BLOCK_FURNACE) {
        /* 44 = front, 45 = side/bottom, 46 = top. */
        if (face == 1) *tile = 46;
        else if (face == 0) *tile = 45;
        else if (face == 3) *tile = 44;
        else *tile = 45;
        world_set_shade(shade);
        return;
    }


    if (id == BLOCK_SAND) { *tile = 18; world_set_shade(shade); return; }
    if (id == BLOCK_GRAVEL) { *tile = 19; world_set_shade(shade); return; }
    if (id == BLOCK_GOLD_ORE) { *tile = 32; world_set_shade(shade); return; }
    if (id == BLOCK_IRON_ORE) { *tile = 33; world_set_shade(shade); return; }
    if (id == BLOCK_COAL_ORE) { *tile = 34; world_set_shade(shade); return; }
    if (id == BLOCK_OBSIDIAN) { *tile = 35; world_set_shade(shade); return; }
    if (id == BLOCK_DIAMOND_ORE) { *tile = 36; world_set_shade(shade); return; }
    if (id == BLOCK_REDSTONE_ORE || id == BLOCK_REDSTONE_ORE_GLOWING) { *tile = 37; world_set_shade(shade); return; }
    if (id == BLOCK_LAPIS_ORE) { *tile = 38; world_set_shade(shade); return; }
    if (id == BLOCK_LAPIS_BLOCK) { *tile = 39; world_set_shade(shade); return; }
    if (id == BLOCK_GOLD_BLOCK) { *tile = 40; world_set_shade(shade); return; }
    if (id == BLOCK_IRON_BLOCK) { *tile = 41; world_set_shade(shade); return; }
    if (id == BLOCK_DIAMOND_BLOCK) { *tile = 42; world_set_shade(shade); return; }
    if (id == BLOCK_BRICK) { *tile = 47; world_set_shade(shade); return; }
    if (id == BLOCK_TNT) { *tile = 48; world_set_shade(shade); return; }
    if (id == BLOCK_GLASS) { *tile = 49; world_set_shade(shade); return; }
    if (id == BLOCK_BOOKSHELF) { *tile = 50; world_set_shade(shade); return; }
    if (id == BLOCK_MOSSY_COBBLESTONE) { *tile = 51; world_set_shade(shade); return; }
    if (id == BLOCK_LEAVES) { *tile = 52; world_set_color_shade(0x59A83A, shade); return; }
    if (id == BLOCK_SPONGE) { *tile = 54; world_set_shade(shade); return; }
    if (id == BLOCK_WOOL) { *tile = 55; world_set_shade(shade); return; }
    if (id == BLOCK_SNOW_BLOCK) { *tile = 56; world_set_shade(shade); return; }
    if (id == BLOCK_ICE) { *tile = 57; world_set_shade(shade); return; }
    if (id == BLOCK_CLAY) { *tile = 62; world_set_shade(shade); return; }
    if (id == BLOCK_NETHERRACK) { *tile = 65; world_set_shade(shade); return; }
    if (id == BLOCK_SOUL_SAND) { *tile = 66; world_set_shade(shade); return; }
    if (id == BLOCK_GLOWSTONE) { *tile = 67; world_set_shade(shade); return; }

    *tile = 2;
    world_set_shade(shade);
}

static void emit_world_block_face_float(int id, float x, float y, float z, int face) {
    float x0 = x, x1 = x + 1.0f;
    float y0 = y, y1 = y + 1.0f;
    float z0 = z, z1 = z + 1.0f;
    float u0,v0,u1,v1;
    int tile = 2;
    world_face_style(id, face, &tile);
    terrain_tile_uv(tile, &u0, &v0, &u1, &v1);
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
}

static void emit_world_block_face(int id, int x, int y, int z, int face) {
    emit_world_block_face_float(id, (float)x, (float)y, (float)z, face);
}

static void draw_world_block_face(int id, int x, int y, int z, int face) {
    glBegin(GL_QUADS);
    emit_world_block_face(id, x, y, z, face);
    glEnd();
}

static void draw_world_block_face_float(int id, float x, float y, float z, int face) {
    glBegin(GL_QUADS);
    emit_world_block_face_float(id, x, y, z, face);
    glEnd();
}

static void draw_generic_world_block_model(int id, float x, float y, float z) {
    for (int face = 0; face < 6; face++) draw_world_block_face_float(id, x, y, z, face);
}


static void draw_liquid_quad_top(float x0, float y, float z0, float x1, float z1, float u0, float v0, float u1, float v1) {
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y, z0, u0, v0);
    world_tex_vertex(x1, y, z0, u1, v0);
    world_tex_vertex(x1, y, z1, u1, v1);
    world_tex_vertex(x0, y, z1, u0, v1);
    glEnd();
}

static void emit_liquid_block_faces(int id, int x, int y, int z) {
    int is_water = (id == BLOCK_WATER || id == BLOCK_STILL_WATER);
    int tile = is_water ? 14 : 30;
    float top = (id == BLOCK_STILL_WATER || id == BLOCK_STILL_LAVA) ? 0.94f : 0.82f;
    float x0 = (float)x, x1 = (float)x + 1.0f;
    float y0 = (float)y, y1 = (float)y + top;
    float z0 = (float)z, z1 = (float)z + 1.0f;
    float u0, v0, u1, v1;
    terrain_tile_uv(tile, &u0, &v0, &u1, &v1);

    if (is_water) glColor4f(0.45f, 0.68f, 1.0f, 0.72f);
    else glColor4f(1.0f, 0.45f, 0.08f, 0.88f);

    /* Top face only when the block above is transparent/non-liquid. */
    if (liquid_face_exposed(x, y, z, 1)) {
        world_tex_vertex(x0, y1, z0, u0, v0);
        world_tex_vertex(x1, y1, z0, u1, v0);
        world_tex_vertex(x1, y1, z1, u1, v1);
        world_tex_vertex(x0, y1, z1, u0, v1);
    }

    /* Exposed sides only.  Do not draw water faces hidden inside dirt/stone;
       the solid face is now rendered behind the transparent water instead. */
    if (liquid_face_exposed(x, y, z, 2)) {
        world_tex_vertex(x1,y1,z0,u0,v0); world_tex_vertex(x0,y1,z0,u1,v0); world_tex_vertex(x0,y0,z0,u1,v1); world_tex_vertex(x1,y0,z0,u0,v1);
    }
    if (liquid_face_exposed(x, y, z, 3)) {
        world_tex_vertex(x0,y1,z1,u0,v0); world_tex_vertex(x1,y1,z1,u1,v0); world_tex_vertex(x1,y0,z1,u1,v1); world_tex_vertex(x0,y0,z1,u0,v1);
    }
    if (liquid_face_exposed(x, y, z, 4)) {
        world_tex_vertex(x0,y1,z0,u0,v0); world_tex_vertex(x0,y1,z1,u1,v0); world_tex_vertex(x0,y0,z1,u1,v1); world_tex_vertex(x0,y0,z0,u0,v1);
    }
    if (liquid_face_exposed(x, y, z, 5)) {
        world_tex_vertex(x1,y1,z1,u0,v0); world_tex_vertex(x1,y1,z0,u1,v0); world_tex_vertex(x1,y0,z0,u1,v1); world_tex_vertex(x1,y0,z1,u0,v1);
    }
}

static void draw_liquid_block(int id, float x, float y, float z) {
    int is_water = (id == BLOCK_WATER || id == BLOCK_STILL_WATER);
    int tile = is_water ? 14 : 30;
    float top = (id == BLOCK_STILL_WATER || id == BLOCK_STILL_LAVA) ? 0.94f : 0.82f;
    float x0 = x, x1 = x + 1.0f;
    float y0 = y, y1 = y + top;
    float z0 = z, z1 = z + 1.0f;
    float u0, v0, u1, v1;
    terrain_tile_uv(tile, &u0, &v0, &u1, &v1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    if (is_water) glColor4f(0.45f, 0.68f, 1.0f, 0.72f);
    else glColor4f(1.0f, 0.45f, 0.08f, 0.88f);

    int ix = (int)x, iy = (int)y, iz = (int)z;
    if (liquid_face_exposed(ix, iy, iz, 1)) {
        draw_liquid_quad_top(x0, y1, z0, x1, z1, u0, v0, u1, v1);
    }

    glBegin(GL_QUADS);
    /* Draw sides only when exposed to transparent/non-liquid space. */
    if (liquid_face_exposed(ix, iy, iz, 2)) {
        world_tex_vertex(x1,y1,z0,u0,v0); world_tex_vertex(x0,y1,z0,u1,v0); world_tex_vertex(x0,y0,z0,u1,v1); world_tex_vertex(x1,y0,z0,u0,v1);
    }
    if (liquid_face_exposed(ix, iy, iz, 3)) {
        world_tex_vertex(x0,y1,z1,u0,v0); world_tex_vertex(x1,y1,z1,u1,v0); world_tex_vertex(x1,y0,z1,u1,v1); world_tex_vertex(x0,y0,z1,u0,v1);
    }
    if (liquid_face_exposed(ix, iy, iz, 4)) {
        world_tex_vertex(x0,y1,z0,u0,v0); world_tex_vertex(x0,y1,z1,u1,v0); world_tex_vertex(x0,y0,z1,u1,v1); world_tex_vertex(x0,y0,z0,u0,v1);
    }
    if (liquid_face_exposed(ix, iy, iz, 5)) {
        world_tex_vertex(x1,y1,z1,u0,v0); world_tex_vertex(x1,y1,z0,u1,v0); world_tex_vertex(x1,y0,z0,u1,v1); world_tex_vertex(x1,y0,z1,u0,v1);
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glColor4f(1,1,1,1);
}

static void draw_world_block_exposed(int id, int x, int y, int z) {
    if (id == 0) return;
    if (block_is_liquid(id)) { draw_liquid_block(id, (float)x, (float)y, (float)z); return; }
    if (id == BLOCK_CHEST) { draw_chest_block((float)x, (float)y, (float)z); return; }
    for (int face = 0; face < 6; face++) {
        if (flat_face_exposed(x, y, z, face)) draw_world_block_face(id, x, y, z, face);
    }
}

static void rebuild_flat_world_chunk_list(int cx, int cz) {
    if (!tex_terrain.id) return;
    if (cx < 0 || cx >= FLAT_RENDER_CHUNKS || cz < 0 || cz >= FLAT_RENDER_CHUNKS) return;

    if (g_flat_world_chunk_lists[cz][cx] == 0) {
        g_flat_world_chunk_lists[cz][cx] = glGenLists(1);
    }
    if (g_flat_world_chunk_liquid_lists[cz][cx] == 0) {
        g_flat_world_chunk_liquid_lists[cz][cx] = glGenLists(1);
    }
    if (g_flat_world_chunk_lists[cz][cx] == 0 || g_flat_world_chunk_liquid_lists[cz][cx] == 0) return;

    int x0 = g_flat_world_origin_x + cx * FLAT_RENDER_CHUNK;
    int z0 = g_flat_world_origin_z + cz * FLAT_RENDER_CHUNK;
    int x1 = x0 + FLAT_RENDER_CHUNK - 1;
    int z1 = z0 + FLAT_RENDER_CHUNK - 1;
    if (x1 >= g_flat_world_origin_x + FLAT_WORLD_SIZE) x1 = g_flat_world_origin_x + FLAT_WORLD_SIZE - 1;
    if (z1 >= g_flat_world_origin_z + FLAT_WORLD_SIZE) z1 = g_flat_world_origin_z + FLAT_WORLD_SIZE - 1;

    int chunk_has_liquid = 0;

    glNewList(g_flat_world_chunk_lists[cz][cx], GL_COMPILE);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    /* Opaque pass only.  Liquids live in a separate display list so every
       opaque chunk can write the depth buffer before transparent water draws. */
    glBegin(GL_QUADS);
    for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
        for (int z = z0; z <= z1; z++) {
            for (int x = x0; x <= x1; x++) {
                int id = flat_get_block(x, y, z);
                if (!id) continue;
                if (block_is_liquid(id)) { chunk_has_liquid = 1; continue; }
                for (int face = 0; face < 6; face++) {
                    if (flat_face_exposed(x, y, z, face)) emit_world_block_face(id, x, y, z, face);
                }
            }
        }
    }
    glEnd();
    glColor4f(1,1,1,1);
    glEndList();

    glNewList(g_flat_world_chunk_liquid_lists[cz][cx], GL_COMPILE);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    if (chunk_has_liquid) {
        glBegin(GL_QUADS);
        for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
            for (int z = z0; z <= z1; z++) {
                for (int x = x0; x <= x1; x++) {
                    int id = flat_get_block(x, y, z);
                    if (block_is_liquid(id)) emit_liquid_block_faces(id, x, y, z);
                }
            }
        }
        glEnd();
    }
    glColor4f(1,1,1,1);
    glEndList();

    g_flat_world_chunk_dirty[cz][cx] = 0;
    g_flat_world_chunk_valid[cz][cx] = 1;
    g_flat_world_chunk_has_liquid[cz][cx] = chunk_has_liquid;
}

static void rebuild_flat_world_geometry_list(void) {
    /* Kept for compatibility: mark/rebuild all chunks. */
    for (int cz = 0; cz < FLAT_RENDER_CHUNKS; cz++) {
        for (int cx = 0; cx < FLAT_RENDER_CHUNKS; cx++) {
            g_flat_world_chunk_dirty[cz][cx] = 1;
            rebuild_flat_world_chunk_list(cx, cz);
        }
    }
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



static int overlay_tile_for_block(int id) {
    if (id == BLOCK_WATER || id == BLOCK_STILL_WATER) return 14;
    if (id == BLOCK_LAVA || id == BLOCK_STILL_LAVA) return 30;
    if (id == BLOCK_GRASS) return 2;
    if (id == BLOCK_STONE) return 1;
    if (id == BLOCK_DIRT) return 2;
    if (id == BLOCK_COBBLESTONE) return 16;
    if (id == BLOCK_BEDROCK) return 17;
    if (id == BLOCK_PLANKS) return 4;
    if (id == BLOCK_LOG) return 20;
    if (id == BLOCK_SAND) return 18;
    if (id == BLOCK_GRAVEL) return 19;
    if (id == BLOCK_FURNACE) return 45;
    return 1;
}

static void draw_in_block_overlay(void) {
    int id = flat_player_suffocation_block();
    int liquid_eye = flat_player_head_block();
    if (!id && (liquid_eye == BLOCK_WATER || liquid_eye == BLOCK_STILL_WATER ||
                liquid_eye == BLOCK_LAVA || liquid_eye == BLOCK_STILL_LAVA)) {
        id = liquid_eye;
    }
    if (!id || !tex_terrain.id) return;

    float u0, v0, u1, v1;
    terrain_tile_uv(overlay_tile_for_block(id), &u0, &v0, &u1, &v1);

    glDisable(GL_FOG);
    setup_gui_projection();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (id == BLOCK_WATER || id == BLOCK_STILL_WATER) {
        glColor4f(0.35f, 0.55f, 1.0f, 0.42f);
    } else if (id == BLOCK_LAVA || id == BLOCK_STILL_LAVA) {
        glColor4f(1.0f, 0.35f, 0.05f, 0.55f);
    } else {
        glColor4f(0.55f, 0.55f, 0.55f, 0.65f);
    }

    /* Source-style "inside block" overlay: a close textured quad over the view. */
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v1); glVertex2f(0.0f, (float)g_gui_h);
    glTexCoord2f(u1, v1); glVertex2f((float)g_gui_w, (float)g_gui_h);
    glTexCoord2f(u1, v0); glVertex2f((float)g_gui_w, 0.0f);
    glTexCoord2f(u0, v0); glVertex2f(0.0f, 0.0f);
    glEnd();

    glColor4f(1, 1, 1, 1);
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
    apply_source_like_fog();
    draw_source_clouds();

    /* Chunked cached display lists:
       The old single display-list rebuild walked the entire 100x100x64 world
       on every block place/mine, causing the visible 0.1s hitch.  Now the first
       load builds all chunks once, and block edits only rebuild nearby 10x10
       chunks. */
    if (g_flat_world_geometry_dirty) {
        for (int cz = 0; cz < FLAT_RENDER_CHUNKS; cz++) {
            for (int cx = 0; cx < FLAT_RENDER_CHUNKS; cx++) {
                g_flat_world_chunk_dirty[cz][cx] = 1;
                g_flat_world_chunk_valid[cz][cx] = 0;
                g_flat_world_chunk_has_liquid[cz][cx] = 0;
            }
        }
        g_flat_world_geometry_dirty = 0;
    }

    int pcx = (flat_index((int)floorf(g_player_x))) / FLAT_RENDER_CHUNK;
    int pcz = (flat_z_index((int)floorf(g_player_z))) / FLAT_RENDER_CHUNK;
    int view_chunk_radius = g_opts.render_distance;
    if (view_chunk_radius < 2) view_chunk_radius = 2;
    if (view_chunk_radius > 32) view_chunk_radius = 32;
    int cz0 = pcz - view_chunk_radius, cz1 = pcz + view_chunk_radius;
    int cx0 = pcx - view_chunk_radius, cx1 = pcx + view_chunk_radius;
    if (cz0 < 0) cz0 = 0;
    if (cx0 < 0) cx0 = 0;
    if (cz1 >= FLAT_RENDER_CHUNKS) cz1 = FLAT_RENDER_CHUNKS - 1;
    if (cx1 >= FLAT_RENDER_CHUNKS) cx1 = FLAT_RENDER_CHUNKS - 1;

    int rebuilds_left = MAX_CHUNK_REBUILDS_PER_FRAME;
    for (int ring = 0; ring < FLAT_RENDER_CHUNKS && rebuilds_left > 0; ring++) {
        for (int dz = -ring; dz <= ring && rebuilds_left > 0; dz++) {
            for (int dx = -ring; dx <= ring && rebuilds_left > 0; dx++) {
                if (ring != 0 && abs(dx) != ring && abs(dz) != ring) continue;
                int cx = pcx + dx;
                int cz = pcz + dz;
                if (cx < cx0 || cx > cx1 || cz < cz0 || cz > cz1) continue;
                if (g_flat_world_chunk_lists[cz][cx] == 0 || g_flat_world_chunk_dirty[cz][cx]) {
                    rebuild_flat_world_chunk_list(cx, cz);
                    rebuilds_left--;
                }
            }
        }
    }

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    for (int cz = cz0; cz <= cz1; cz++) {
        for (int cx = cx0; cx <= cx1; cx++) {
            /* Dirty-but-valid chunks are safe to draw using their old mesh while
               the one-chunk-per-frame rebuild catches up.  This removes the
               black/world-hole flicker when mining or placing blocks.  New
               streamed chunks stay invalid until rebuilt so old-coordinate
               display lists are never shown in the wrong place. */
            if (g_flat_world_chunk_lists[cz][cx] != 0 && g_flat_world_chunk_valid[cz][cx]) {
                glCallList(g_flat_world_chunk_lists[cz][cx]);
            }
        }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (int cz = cz0; cz <= cz1; cz++) {
        for (int cx = cx0; cx <= cx1; cx++) {
            if (g_flat_world_chunk_liquid_lists[cz][cx] != 0 &&
                g_flat_world_chunk_valid[cz][cx] && g_flat_world_chunk_has_liquid[cz][cx]) {
                glCallList(g_flat_world_chunk_liquid_lists[cz][cx]);
            }
        }
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glColor4f(1,1,1,1);

    draw_third_person_player();
    draw_block_selection_border();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    draw_dropped_items();
    draw_dig_particles(g_frame_partial);
    if (g_breaking_block && flat_get_block(g_break_x, g_break_y, g_break_z) != 0) {
        float dmg = g_prev_break_damage + (g_break_damage - g_prev_break_damage) * g_frame_partial;
        int stage = (int)(dmg * 10.0f);
        draw_break_overlay_cube((float)g_break_x, (float)g_break_y, (float)g_break_z, stage);
    }

    glColor4f(1,1,1,1);
    glDisable(GL_FOG);
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

    if (!stack_empty(held) && render_item_as_block_id(held->id) && tex_terrain.id) {
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
    draw_in_block_overlay();
    setup_gui_projection();
}

