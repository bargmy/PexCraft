/* Split from original monolithic main.c. Included by src/main.c unity build. */

static void draw_item3d_from_atlas(int tile);
static void draw_item3d_from_texture(Texture *atlas, int tile);
static void setup_world_projection(void);
static void apply_view_bobbing(float partial);
static void apply_hurt_camera_effect(float partial);
static void apply_portal_camera_effect(float partial);
static void update_portal_texture_animation(void);
static void draw_portal_overlay(void);

typedef struct PexSkyJavaRandom {
    uint64_t seed;
} PexSkyJavaRandom;

typedef struct PexStarQuad {
    float v[4][3];
} PexStarQuad;

static PexStarQuad g_sky_stars[1500];
static int g_sky_star_count = 0;
static int g_sky_stars_ready = 0;

static void pex_sky_java_random_set_seed(PexSkyJavaRandom *rng, int64_t seed) {
    rng->seed = ((uint64_t)seed ^ 0x5DEECE66DULL) & ((1ULL << 48) - 1ULL);
}

static int pex_sky_java_random_next(PexSkyJavaRandom *rng, int bits) {
    rng->seed = (rng->seed * 0x5DEECE66DULL + 0xBULL) & ((1ULL << 48) - 1ULL);
    return (int)(rng->seed >> (48 - bits));
}

static float pex_sky_java_random_next_float(PexSkyJavaRandom *rng) {
    return (float)pex_sky_java_random_next(rng, 24) / 16777216.0f;
}

static double pex_sky_java_random_next_double(PexSkyJavaRandom *rng) {
    uint64_t hi = (uint64_t)pex_sky_java_random_next(rng, 26);
    uint64_t lo = (uint64_t)pex_sky_java_random_next(rng, 27);
    return (double)((hi << 27) + lo) / (double)(1ULL << 53);
}

static float world_sun_angle(float partial) {
    int t = (int)(g_world_time % 24000LL);
    if (t < 0) t += 24000;
    float a = ((float)t + partial) / 24000.0f - 0.25f;
    if (a < 0.0f) a += 1.0f;
    if (a > 1.0f) a -= 1.0f;
    float raw = a;
    a = 1.0f - (float)((cos((double)a * M_PI) + 1.0) / 2.0);
    a = raw + (a - raw) / 3.0f;
    return a;
}

static int world_moon_phase(void) {
    long long day = g_world_time / 24000LL;
    int phase = (int)(day % 8LL);
    if (phase < 0) phase += 8;
    return phase;
}

static float sky_star_brightness(float partial) {
    float a = world_sun_angle(partial);
    float f = 1.0f - (cosf(a * (float)M_PI * 2.0f) * 2.0f + 12.0f / 16.0f);
    if (f < 0.0f) f = 0.0f;
    if (f > 1.0f) f = 1.0f;
    return f * f * 0.5f;
}

static float sky_day_factor(float partial, float add) {
    float a = world_sun_angle(partial);
    float f = cosf(a * (float)M_PI * 2.0f) * 2.0f + add;
    if (f < 0.0f) f = 0.0f;
    if (f > 1.0f) f = 1.0f;
    return f;
}

static void sky_color_at_time(float partial, float *r, float *g, float *b) {
    float f = sky_day_factor(partial, 0.5f);
    /* World.getSkyColor(Entity,float) with plains sky-color fallback:
       BiomeGenBase.plains.getSkyColorByTemp(0.8F) -> 0x79A7FF.
       Rain, thunder, anaglyph, and lightningFlash are absent in this C project. */
    *r = (121.0f / 255.0f) * f;
    *g = (167.0f / 255.0f) * f;
    *b = (255.0f / 255.0f) * f;
}

static void cloud_color_at_time(float partial, float *r, float *g, float *b) {
    float f = sky_day_factor(partial, 0.5f);
    /* World.drawClouds(float), with World.cloudColour = 0xFFFFFF and no
       rain/thunder: RGB is not fixed white at night. */
    *r = f * 0.90f + 0.10f;
    *g = f * 0.90f + 0.10f;
    *b = f * 0.85f + 0.15f;
}

static void world_provider_fog_color(float partial, float *r, float *g, float *b) {
    float f = sky_day_factor(partial, 0.5f);
    /* WorldProvider.getFogColor(celestialAngle,float). */
    *r = (192.0f / 255.0f) * (f * 0.94f + 0.06f);
    *g = (216.0f / 255.0f) * (f * 0.94f + 0.06f);
    *b = 1.0f * (f * 0.91f + 0.09f);
}

static void world_fog_color(float partial, float *r, float *g, float *b) {
    if (g_current_dimension == -1) {
        *r = 0.20f; *g = 0.02f; *b = 0.02f;
        return;
    }
    if (g_current_dimension == 1) {
        *r = 0.08f; *g = 0.06f; *b = 0.10f;
        return;
    }
    float fr, fg, fb;
    world_provider_fog_color(partial, &fr, &fg, &fb);

    /* EntityRenderer.updateFogColor() blends provider fog toward sky depending
       on render-distance enum.  This C port stores render distance in chunks,
       not the Java enum, so use Java's Far equivalent (0) rather than keeping a
       constant daytime blue. */
    float sr, sg, sb;
    sky_color_at_time(partial, &sr, &sg, &sb);
    float blend = 1.0f - powf(1.0f / 4.0f, 0.25f);
    *r = fr + (sr - fr) * blend;
    *g = fg + (sg - fg) * blend;
    *b = fb + (sb - fb) * blend;
}

static void apply_world_fog_color(float partial) {
    float r, g, b;
    world_fog_color(partial, &r, &g, &b);
    float fog_col[4] = { r, g, b, 1.0f };
    glFogfv(GL_FOG_COLOR, fog_col);
    glClearColor(r, g, b, 0.0f);
}

static void sky_generate_stars_once(void) {
    if (g_sky_stars_ready) return;
    g_sky_stars_ready = 1;
    g_sky_star_count = 0;
    PexSkyJavaRandom rng;
    pex_sky_java_random_set_seed(&rng, 10842LL);

    for (int i = 0; i < 1500; ++i) {
        double x = (double)(pex_sky_java_random_next_float(&rng) * 2.0f - 1.0f);
        double y = (double)(pex_sky_java_random_next_float(&rng) * 2.0f - 1.0f);
        double z = (double)(pex_sky_java_random_next_float(&rng) * 2.0f - 1.0f);
        double s = (double)(0.25f + pex_sky_java_random_next_float(&rng) * 0.25f);
        double d = x * x + y * y + z * z;
        if (d >= 1.0 || d <= 0.01) continue;
        d = 1.0 / sqrt(d);
        x *= d; y *= d; z *= d;
        double bx = x * 100.0;
        double by = y * 100.0;
        double bz = z * 100.0;
        double yaw = atan2(x, z);
        double sy = sin(yaw);
        double cy = cos(yaw);
        double pitch = atan2(sqrt(x * x + z * z), y);
        double sp = sin(pitch);
        double cp = cos(pitch);
        double roll = pex_sky_java_random_next_double(&rng) * M_PI * 2.0;
        double sr = sin(roll);
        double cr = cos(roll);
        if (g_sky_star_count >= (int)(sizeof(g_sky_stars) / sizeof(g_sky_stars[0]))) break;
        PexStarQuad *q = &g_sky_stars[g_sky_star_count++];
        for (int v = 0; v < 4; ++v) {
            double qx = 0.0;
            double qy = (double)((v & 2) - 1) * s;
            double qz = (double)(((v + 1) & 2) - 1) * s;
            double rx = qy * cr - qz * sr;
            double rz = qz * cr + qy * sr;
            double px = rx * sp + qx * cp;
            double py = qx * sp - rx * cp;
            double vx = py * sy - rz * cy;
            double vz = rz * sy + py * cy;
            q->v[v][0] = (float)(bx + vx);
            q->v[v][1] = (float)(by + px);
            q->v[v][2] = (float)(bz + vz);
        }
    }
}

static void draw_sky_plane(float y) {
    const int step = 64;
    const int extent = 256 + step * 2;
    for (int x = -extent; x <= extent; x += step) {
        for (int z = -extent; z <= extent; z += step) {
            glBegin(GL_QUADS);
            glVertex3f((float)x, (float)y, (float)z);
            glVertex3f((float)(x + step), (float)y, (float)z);
            glVertex3f((float)(x + step), (float)y, (float)(z + step));
            glVertex3f((float)x, (float)y, (float)(z + step));
            glEnd();
        }
    }
}

static void draw_lower_sky_plane(float y) {
    const int step = 64;
    const int extent = 256 + step * 2;
    glBegin(GL_QUADS);
    for (int x = -extent; x <= extent; x += step) {
        for (int z = -extent; z <= extent; z += step) {
            /* RenderGlobal::<init>() builds glSkyList2 with the opposite
               winding from glSkyList.  Keep that here so cull state inherited
               from the world renderer cannot make the lower sky disappear. */
            glVertex3f((float)(x + step), (float)y, (float)z);
            glVertex3f((float)x, (float)y, (float)z);
            glVertex3f((float)x, (float)y, (float)(z + step));
            glVertex3f((float)(x + step), (float)y, (float)(z + step));
        }
    }
    glEnd();
}

static void sky_textured_quad(Texture *tex, float size, float y, float u0, float v0, float u1, float v1, int fallback_color) {
    if (tex && tex->id) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex->id);
        glColor4f(1, 1, 1, 1);
    } else {
        glDisable(GL_TEXTURE_2D);
        set_color_int(fallback_color);
    }
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex3f(-size, y, -size);
    glTexCoord2f(u1, v0); glVertex3f( size, y, -size);
    glTexCoord2f(u1, v1); glVertex3f( size, y,  size);
    glTexCoord2f(u0, v1); glVertex3f(-size, y,  size);
    glEnd();
    glColor4f(1,1,1,1);
    glEnable(GL_TEXTURE_2D);
}

static int sky_sunrise_sunset_colors(float partial, float out[4]) {
    float a = world_sun_angle(partial);
    float width = 0.4f;
    float c = cosf(a * (float)M_PI * 2.0f);
    if (c < -width || c > width) return 0;
    float t = c / width * 0.5f + 0.5f;
    float alpha = 1.0f - (1.0f - sinf(t * (float)M_PI)) * 0.99f;
    alpha *= alpha;
    out[0] = t * 0.3f + 0.7f;
    out[1] = t * t * 0.7f + 0.2f;
    out[2] = 0.2f;
    out[3] = alpha;
    return 1;
}

static void draw_sunrise_sunset_fan(float partial) {
    float col[4];
    if (!sky_sunrise_sunset_colors(partial, col)) return;
    glDisable(GL_TEXTURE_2D);
    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(sinf(world_sun_angle(partial) * (float)M_PI * 2.0f) < 0.0f ? 180.0f : 0.0f, 0.0f, 0.0f, 1.0f);
    glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(col[0], col[1], col[2], col[3]);
    glVertex3f(0.0f, 100.0f, 0.0f);
    glColor4f(col[0], col[1], col[2], 0.0f);
    for (int i = 0; i <= 16; ++i) {
        float a = (float)i * (float)M_PI * 2.0f / 16.0f;
        float sx = sinf(a);
        float cz = cosf(a);
        glVertex3f(sx * 120.0f, cz * 120.0f, -cz * 40.0f * col[3]);
    }
    glEnd();
    glPopMatrix();
}

static float world_sea_level(void) {
    /* Java World.getSeaLevel() is FLAT -> 0, otherwise 63.  This project still
       renders the old local Beta-style terrain preview whose documented sea
       level is 32, so the RenderGlobal below-sea void/horizon test must use the
       active terrain system's sea level until the terrain generator itself is
       upgraded to Release 1.2.5. */
    return (g_world_type == 0) ? 0.0f : 32.0f;
}

static float render_eye_y(float partial) {
    const PexPlayerRenderState *pr = &g_player_render_frame;
    return pr->prev_y + (pr->y - pr->prev_y) * partial;
}

static void draw_black_horizon_box(float eye_minus_sea) {
    const float s = 1.0f;
    float y0 = -((float)(eye_minus_sea + 65.0f));
    float y1 = -s;

    glPushMatrix();
    glTranslatef(0.0f, 12.0f, 0.0f);
    draw_lower_sky_plane(-16.0f);
    glPopMatrix();

    glBegin(GL_QUADS);
    glVertex3f(-s, y0,  s);
    glVertex3f( s, y0,  s);
    glVertex3f( s, y1,  s);
    glVertex3f(-s, y1,  s);

    glVertex3f(-s, y1, -s);
    glVertex3f( s, y1, -s);
    glVertex3f( s, y0, -s);
    glVertex3f(-s, y0, -s);

    glVertex3f( s, y1, -s);
    glVertex3f( s, y1,  s);
    glVertex3f( s, y0,  s);
    glVertex3f( s, y0, -s);

    glVertex3f(-s, y0, -s);
    glVertex3f(-s, y0,  s);
    glVertex3f(-s, y1,  s);
    glVertex3f(-s, y1, -s);

    glVertex3f(-s, y1, -s);
    glVertex3f(-s, y1,  s);
    glVertex3f( s, y1,  s);
    glVertex3f( s, y1, -s);
    glEnd();
}

static void apply_sky_camera_rotation(float partial) {
    const PexPlayerRenderState *pr = &g_player_render_frame;
    float dyaw = pr->yaw - pr->prev_yaw;
    while (dyaw < -180.0f) dyaw += 360.0f;
    while (dyaw >= 180.0f) dyaw -= 360.0f;
    float yaw = pr->prev_yaw + dyaw * partial;
    float pitch = pr->prev_pitch + (pr->pitch - pr->prev_pitch) * partial;
    if (g_third_person_view == 2) {
        yaw += 180.0f;
        pitch = -pitch;
    }
    /* Java sky is drawn after setupCameraTransform(), so it inherits the same
       hurt and bob matrix.  The sky itself must not translate with player XYZ. */
    apply_hurt_camera_effect(partial);
    apply_view_bobbing(partial);
    glRotatef(pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(yaw + 180.0f, 0.0f, 1.0f, 0.0f);
}

static void draw_sky_only(void) {
    if (g_current_dimension == -1) {
        setup_world_projection();
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_TEXTURE_2D);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glColor4f(0.10f, 0.01f, 0.01f, 1.0f);
        draw_sky_plane(16.0f);
        glColor4f(0.06f, 0.0f, 0.0f, 1.0f);
        draw_lower_sky_plane(-16.0f);
        glEnable(GL_TEXTURE_2D);
        glDepthMask(GL_TRUE);
        return;
    }
    if (g_current_dimension == 1) {
        setup_world_projection();
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_TEXTURE_2D);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        apply_sky_camera_rotation(g_frame_partial);
        glColor4f(0.032f, 0.032f, 0.032f, 1.0f);
        draw_sky_plane(16.0f);
        glColor4f(0.015f, 0.015f, 0.015f, 1.0f);
        draw_lower_sky_plane(-16.0f);
        sky_generate_stars_once();
        glDisable(GL_FOG);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(0.5f, 0.5f, 0.8f, 0.7f);
        glBegin(GL_QUADS);
        for (int i = 0; i < g_sky_star_count; ++i) {
            for (int v = 0; v < 4; ++v) {
                glVertex3f(g_sky_stars[i].v[v][0], g_sky_stars[i].v[v][1], g_sky_stars[i].v[v][2]);
            }
        }
        glEnd();
        glDisable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
        glDepthMask(GL_TRUE);
        return;
    }

    setup_world_projection();
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_FOG);
    apply_world_fog_color(g_frame_partial);
    glDisable(GL_ALPHA_TEST);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    apply_sky_camera_rotation(g_frame_partial);

    /* Java 1.2.5 RenderGlobal.renderSky(float): top glSkyList plane,
       sunrise/sunset fan, additive sun/moon-phases/stars, black below-sea
       horizon when needed, then translated glSkyList2 lower plane. */
    float sr, sg, sb;
    sky_color_at_time(g_frame_partial, &sr, &sg, &sb);

    glDisable(GL_TEXTURE_2D);
    glColor4f(sr, sg, sb, 1.0f);
    draw_sky_plane(16.0f);
    glDisable(GL_FOG);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw_sunrise_sunset_fan(g_frame_partial);

    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glPushMatrix();
    {
        float rain_alpha = 1.0f;
        glColor4f(1.0f, 1.0f, 1.0f, rain_alpha);
        glTranslatef(0.0f, 0.0f, 0.0f);
        glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
        glRotatef(world_sun_angle(g_frame_partial) * 360.0f, 1.0f, 0.0f, 0.0f);

        if (tex_sun.id) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, tex_sun.id);
            glColor4f(1.0f, 1.0f, 1.0f, rain_alpha);
        } else {
            glDisable(GL_TEXTURE_2D);
            set_color_int(0xFFFFFFFF);
        }
        {
            float s = 30.0f;
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-s, 100.0f, -s);
            glTexCoord2f(1.0f, 0.0f); glVertex3f( s, 100.0f, -s);
            glTexCoord2f(1.0f, 1.0f); glVertex3f( s, 100.0f,  s);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-s, 100.0f,  s);
            glEnd();
        }

        {
            int phase = world_moon_phase();
            int mu = phase % 4;
            int mv = (phase / 4) % 2;
            float u0 = (float)mu / 4.0f;
            float v0 = (float)mv / 2.0f;
            float u1 = (float)(mu + 1) / 4.0f;
            float v1 = (float)(mv + 1) / 2.0f;
            if (tex_moon_phases.id) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, tex_moon_phases.id);
                glColor4f(1.0f, 1.0f, 1.0f, rain_alpha);
            } else if (tex_moon.id) {
                /* Fallback only for packs/platform assets missing the Release
                   1.2.5 moon phase sheet.  Java 1.2.5 uses moon_phases.png. */
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, tex_moon.id);
                glColor4f(1.0f, 1.0f, 1.0f, rain_alpha);
                u0 = 1.0f; v0 = 0.0f; u1 = 0.0f; v1 = 1.0f;
            } else {
                glDisable(GL_TEXTURE_2D);
                set_color_int(0xFFFFFFFF);
                u0 = 1.0f; v0 = 0.0f; u1 = 0.0f; v1 = 1.0f;
            }
            {
                float s = 20.0f;
                glBegin(GL_QUADS);
                glTexCoord2f(u1, v1); glVertex3f(-s, -100.0f,  s);
                glTexCoord2f(u0, v1); glVertex3f( s, -100.0f,  s);
                glTexCoord2f(u0, v0); glVertex3f( s, -100.0f, -s);
                glTexCoord2f(u1, v0); glVertex3f(-s, -100.0f, -s);
                glEnd();
            }
        }

        glDisable(GL_TEXTURE_2D);
        {
            float star = sky_star_brightness(g_frame_partial) * rain_alpha;
            if (star > 0.0f) {
                sky_generate_stars_once();
                glColor4f(star, star, star, star);
                glBegin(GL_QUADS);
                for (int i = 0; i < g_sky_star_count; ++i) {
                    for (int v = 0; v < 4; ++v) {
                        glVertex3f(g_sky_stars[i].v[v][0], g_sky_stars[i].v[v][1], g_sky_stars[i].v[v][2]);
                    }
                }
                glEnd();
            }
        }
    }
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_FOG);
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    {
        float eye_minus_sea = render_eye_y(g_frame_partial) - world_sea_level();
        if (eye_minus_sea < 0.0f) {
            glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
            draw_black_horizon_box(eye_minus_sea);
        }

        if (1) { /* WorldProviderSurface::isSkyColored() equivalent. */
            glColor4f(sr * 0.2f + 0.04f, sg * 0.2f + 0.04f, sb * 0.6f + 0.1f, 1.0f);
        } else {
            glColor4f(sr, sg, sb, 1.0f);
        }
        glPushMatrix();
        glTranslatef(0.0f, -(eye_minus_sea - 16.0f), 0.0f);
        draw_lower_sky_plane(-16.0f);
        glPopMatrix();
    }
    glEnable(GL_TEXTURE_2D);
    glDepthMask(GL_TRUE);
}

static int mcpe_chat_color_rgb(char code) {
    if (code >= 'A' && code <= 'Z') code = (char)(code - 'A' + 'a');
    switch (code) {
        case '0': return 0x000000; case '1': return 0x0000AA;
        case '2': return 0x00AA00; case '3': return 0x00AAAA;
        case '4': return 0xAA0000; case '5': return 0xAA00AA;
        case '6': return 0xFFAA00; case '7': return 0xAAAAAA;
        case '8': return 0x555555; case '9': return 0x5555FF;
        case 'a': return 0x55FF55; case 'b': return 0x55FFFF;
        case 'c': return 0xFF5555; case 'd': return 0xFF55FF;
        case 'e': return 0xFFFF55; case 'f': return 0xFFFFFF;
        case 'r': return 0xFFFFFF;
        default: return -1;
    }
}

static void draw_text_mcpe_colored(const char *text, int x, int y, int argb) {
    if (!text) return;
    int alpha = argb & 0xFF000000;
    int rgb = argb & 0x00FFFFFF;
    int cur_x = x;
    char seg[512];
    int seg_n = 0;
    const unsigned char *p = (const unsigned char *)text;
    while (*p) {
        int section = 0;
        char code = 0;
        if (p[0] == 0xc2 && p[1] == 0xa7 && p[2]) { section = 1; code = (char)p[2]; }
        else if (p[0] == 0xa7 && p[1]) { section = 2; code = (char)p[1]; }
        if (section) {
            if (seg_n > 0) {
                seg[seg_n] = 0;
                draw_text(seg, cur_x, y, alpha | rgb);
                cur_x += text_width(seg);
                seg_n = 0;
            }
            int c = mcpe_chat_color_rgb(code);
            if (c >= 0) rgb = c;
            p += section == 1 ? 3 : 2;
            continue;
        }
        if (seg_n + 1 < (int)sizeof(seg)) seg[seg_n++] = (char)*p;
        p++;
    }
    if (seg_n > 0) {
        seg[seg_n] = 0;
        draw_text(seg, cur_x, y, alpha | rgb);
    }
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
        draw_text_mcpe_colored(g_chat_lines[i].text, 2, y, (alpha << 24) | 0xFFFFFF);
    }
}


static int generated_render_radius(void) {
    int chunks = g_opts.render_distance;
    if (chunks < 2) chunks = 2;
    /* The active world window must contain player +/- render radius plus a small
       safety ring for chunk/section edge rebuilds.  Values above this cap would
       only show outside the generated in-memory terrain window. */
    int max_chunks = (FLAT_RENDER_CHUNKS / 2) - 2;
    if (max_chunks < 2) max_chunks = 2;
    if (chunks > max_chunks) chunks = max_chunks;
    return chunks;
}

static float current_render_distance_blocks(void) {
    return (float)(generated_render_radius() * 16);
}

static void apply_source_like_fog(void) {
    float end = current_render_distance_blocks();
    float start = end * 0.25f;
    float fog_col[4];
    world_fog_color(g_frame_partial, &fog_col[0], &fog_col[1], &fog_col[2]);
    fog_col[3] = 1.0f;

    int fog_mode = GL_LINEAR;
    float fog_density = 0.0f;

    /* Java EntityRenderer.setupFog(float) switches to exponential medium fog
       when the camera is in water/lava; otherwise it uses the time-dependent
       fog color from EntityRenderer.updateFogColor(), not a fixed day blue. */
    if (flat_player_head_in_water()) {
        fog_col[0] = 0.02f; fog_col[1] = 0.02f; fog_col[2] = 0.20f;
        fog_mode = GL_EXP;
        fog_density = 0.10f;
    } else if (flat_player_head_in_lava()) {
        fog_col[0] = 0.60f; fog_col[1] = 0.10f; fog_col[2] = 0.00f;
        fog_mode = GL_EXP;
        fog_density = 2.00f;
    } else if (g_current_dimension == -1) {
        fog_col[0] = 0.20f; fog_col[1] = 0.02f; fog_col[2] = 0.02f;
        fog_mode = GL_EXP;
        fog_density = 0.035f;
    } else if (g_current_dimension == 1) {
        fog_col[0] = 0.08f; fog_col[1] = 0.06f; fog_col[2] = 0.10f;
        fog_mode = GL_EXP2;
        fog_density = 0.015f;
    }

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, fog_mode);
    glFogfv(GL_FOG_COLOR, fog_col);
    glClearColor(fog_col[0], fog_col[1], fog_col[2], 0.0f);
    if (fog_mode == GL_EXP || fog_mode == GL_EXP2) glFogf(GL_FOG_DENSITY, fog_density);
    else { glFogf(GL_FOG_START, start); glFogf(GL_FOG_END, end); }
}


static void cloud_color(float *r, float *g, float *b) {
    cloud_color_at_time(g_frame_partial, r, g, b);
}

static void cloud_tex_vertex(float x, float y, float z, float u, float v) {
    glTexCoord2f(u, v);
    glVertex3f(x, y, z);
}

static int cloud_texture_has_cutout_alpha(void) {
    static const unsigned char *cached_rgba = NULL;
    static int cached_w = 0, cached_h = 0, cached_result = 0, cached_ready = 0;
    if (!tex_clouds.rgba || tex_clouds.w <= 0 || tex_clouds.h <= 0) return 0;
    if (cached_ready && cached_rgba == tex_clouds.rgba && cached_w == tex_clouds.w && cached_h == tex_clouds.h) {
        return cached_result;
    }
    int n = tex_clouds.w * tex_clouds.h;
    cached_result = 0;
    for (int i = 0; i < n; ++i) {
        unsigned char a = tex_clouds.rgba[i * 4 + 3];
        if (a < 250) { cached_result = 1; break; }
    }
    cached_rgba = tex_clouds.rgba;
    cached_w = tex_clouds.w;
    cached_h = tex_clouds.h;
    cached_ready = 1;
    return cached_result;
}

static void draw_source_fast_clouds(float partial) {
    if (!tex_clouds.id) return;
    float cr, cg, cb;
    cloud_color(&cr, &cg, &cb);

    const PexPlayerRenderState *pr = &g_player_render_frame;
    float px = pr->prev_x + (pr->x - pr->prev_x) * partial;
    float pz = pr->prev_z + (pr->z - pr->prev_z) * partial;

    /* RenderGlobal.func_4141_b: fast clouds are one huge tiled plane.  The
       coordinates are placed in world space here, which becomes the same
       camera-relative plane after apply_player_camera() translates by -player. */
    const int step = 32;
    const int loops = 256 / step;
    const float uv_scale = 0.5f / 1024.0f;
    double scroll_x = (double)px + (double)(((float)pr->ingame_ticks + partial) * 0.03f);
    double scroll_z = (double)pz;
    int wrap_x = (int)floor(scroll_x / 2048.0);
    int wrap_z = (int)floor(scroll_z / 2048.0);
    scroll_x -= (double)(wrap_x * 2048);
    scroll_z -= (double)(wrap_z * 2048);
    float u_scroll = (float)(scroll_x * (double)uv_scale);
    float v_scroll = (float)(scroll_z * (double)uv_scale);
    const float y = 120.33f;

    glDisable(GL_CULL_FACE);
    glDisable(GL_FOG);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_clouds.id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glColor4f(cr, cg, cb, 0.8f);

    glBegin(GL_QUADS);
    for (int x = -step * loops; x < step * loops; x += step) {
        for (int z = -step * loops; z < step * loops; z += step) {
            float x0 = px + (float)x;
            float x1 = px + (float)(x + step);
            float z0 = pz + (float)z;
            float z1 = pz + (float)(z + step);
            cloud_tex_vertex(x0, y, z1, (float)x * uv_scale + u_scroll, (float)(z + step) * uv_scale + v_scroll);
            cloud_tex_vertex(x1, y, z1, (float)(x + step) * uv_scale + u_scroll, (float)(z + step) * uv_scale + v_scroll);
            cloud_tex_vertex(x1, y, z0, (float)(x + step) * uv_scale + u_scroll, (float)z * uv_scale + v_scroll);
            cloud_tex_vertex(x0, y, z0, (float)x * uv_scale + u_scroll, (float)z * uv_scale + v_scroll);
        }
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glColor4f(1,1,1,1);
}

static void draw_source_fancy_clouds(float partial) {
    if (!tex_clouds.id) return;
    /* Java RenderGlobal.renderCloudsFancy is plain fixed-function OpenGL.
       The old C guard forced real OpenGL through the flat cloud path while D3D
       used the 3D slab path, which made clouds visibly 2D only on OpenGL. */
    if (!cloud_texture_has_cutout_alpha()) { draw_source_fast_clouds(partial); return; }
    float cr, cg, cb;
    cloud_color(&cr, &cg, &cb);

    const PexPlayerRenderState *pr = &g_player_render_frame;
    float px = pr->prev_x + (pr->x - pr->prev_x) * partial;
    float py = pr->prev_y + (pr->y - pr->prev_y) * partial;
    float pz = pr->prev_z + (pr->z - pr->prev_z) * partial;

    /* RenderGlobal.func_6510_c: 12x scaled 3D cloud slabs, 8x8 cloud cells,
       4 block thickness.  Kept source-shaped, but still emitted through the C
       GL shim so PC and PSP share one path. */
    const float scale = 12.0f;
    const float thickness = 4.0f;
    const int cell = 8;
    const int radius_cells = 4;
    const float eps = 1.0f / 1024.0f;
    const float uv_scale = 1.0f / 256.0f;
    double sx = ((double)px + (double)(((float)pr->ingame_ticks + partial) * 0.03f)) / (double)scale;
    double sz = ((double)pz) / (double)scale + 0.33;
    int wrap_x = (int)floor(sx / 2048.0);
    int wrap_z = (int)floor(sz / 2048.0);
    sx -= (double)(wrap_x * 2048);
    sz -= (double)(wrap_z * 2048);
    int fsx = (int)floor(sx);
    int fsz = (int)floor(sz);
    float u_base = (float)fsx * uv_scale;
    float v_base = (float)fsz * uv_scale;
    float frac_x = (float)(sx - (double)fsx);
    float frac_z = (float)(sz - (double)fsz);
    float y0 = 108.33f;
    float y1 = y0 + thickness;

    glDisable(GL_CULL_FACE);
    glDisable(GL_FOG);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_clouds.id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    /* Java does a true depth prepass by disabling color writes only.  Depth
       writes must stay enabled, otherwise all internal 3D cloud slab faces blend
       through each other and look like separated translucent cubes.  Alpha test
       is also needed so transparent pixels in clouds.png do not write depth. */
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    glDepthMask(GL_TRUE);

    for (int pass = 0; pass < 2; ++pass) {
        glColorMask(pass != 0, pass != 0, pass != 0, pass != 0);
        float alpha = 0.8f;

        /* Keep Java's two-pass fancy-cloud shape, but do not emit one immediate
           batch per 8x8 cloud cell.  On the D3D11 compatibility backend each
           glEnd() finalizes and submits a draw batch; the old code made roughly
           128 tiny cloud draw batches per frame here.  That is why Loggy showed
           World clouds at ~3 ms while the actual terrain draw was cheap.
           One batch per depth/color pass keeps the same vertices/colors while
           cutting the D3D11 cloud submit overhead to two batches per frame. */
        glBegin(GL_QUADS);
        for (int cz = -radius_cells + 1; cz <= radius_cells; ++cz) {
            for (int cx = -radius_cells + 1; cx <= radius_cells; ++cx) {
                float gx0 = (float)(cx * cell) - frac_x;
                float gz0 = (float)(cz * cell) - frac_z;
                float gx1 = gx0 + (float)cell;
                float gz1 = gz0 + (float)cell;
                float x0 = px + gx0 * scale;
                float x1 = px + gx1 * scale;
                float z0 = pz + gz0 * scale;
                float z1 = pz + gz1 * scale;
                float u0 = ((float)(cx * cell) + 0.0f) * uv_scale + u_base;
                float u1 = ((float)(cx * cell + cell)) * uv_scale + u_base;
                float v0 = ((float)(cz * cell) + 0.0f) * uv_scale + v_base;
                float v1 = ((float)(cz * cell + cell)) * uv_scale + v_base;

                /* Java draws the bottom face when the camera is below/inside the
                   slab, and the top face when the camera is above/inside it.  The
                   earlier port had these swapped, which made fancy clouds look like
                   inside-out sheets from the ground. */
                if (py < y1 + 1.0f) {
                    glColor4f(cr * 0.7f, cg * 0.7f, cb * 0.7f, alpha);
                    cloud_tex_vertex(x0, y0, z1, u0, v1);
                    cloud_tex_vertex(x1, y0, z1, u1, v1);
                    cloud_tex_vertex(x1, y0, z0, u1, v0);
                    cloud_tex_vertex(x0, y0, z0, u0, v0);
                }
                if (py > y0 - thickness - 1.0f) {
                    glColor4f(cr, cg, cb, alpha);
                    cloud_tex_vertex(x0, y1 - eps, z1, u0, v1);
                    cloud_tex_vertex(x1, y1 - eps, z1, u1, v1);
                    cloud_tex_vertex(x1, y1 - eps, z0, u1, v0);
                    cloud_tex_vertex(x0, y1 - eps, z0, u0, v0);
                }
                glColor4f(cr * 0.9f, cg * 0.9f, cb * 0.9f, alpha);
                if (cx > -1) for (int i = 0; i < cell; ++i) {
                    float xa = px + ((float)(cx * cell + i) - frac_x) * scale;
                    float uu = ((float)(cx * cell + i) + 0.5f) * uv_scale + u_base;
                    cloud_tex_vertex(xa, y0, z1, uu, v1); cloud_tex_vertex(xa, y1, z1, uu, v1); cloud_tex_vertex(xa, y1, z0, uu, v0); cloud_tex_vertex(xa, y0, z0, uu, v0);
                }
                if (cx <= 1) for (int i = 0; i < cell; ++i) {
                    float xa = px + ((float)(cx * cell + i + 1) - frac_x - eps) * scale;
                    float uu = ((float)(cx * cell + i) + 0.5f) * uv_scale + u_base;
                    cloud_tex_vertex(xa, y0, z0, uu, v0); cloud_tex_vertex(xa, y1, z0, uu, v0); cloud_tex_vertex(xa, y1, z1, uu, v1); cloud_tex_vertex(xa, y0, z1, uu, v1);
                }
                glColor4f(cr * 0.8f, cg * 0.8f, cb * 0.8f, alpha);
                if (cz > -1) for (int i = 0; i < cell; ++i) {
                    float za = pz + ((float)(cz * cell + i) - frac_z) * scale;
                    float vv = ((float)(cz * cell + i) + 0.5f) * uv_scale + v_base;
                    cloud_tex_vertex(x0, y1, za, u0, vv); cloud_tex_vertex(x1, y1, za, u1, vv); cloud_tex_vertex(x1, y0, za, u1, vv); cloud_tex_vertex(x0, y0, za, u0, vv);
                }
                if (cz <= 1) for (int i = 0; i < cell; ++i) {
                    float za = pz + ((float)(cz * cell + i + 1) - frac_z - eps) * scale;
                    float vv = ((float)(cz * cell + i) + 0.5f) * uv_scale + v_base;
                    cloud_tex_vertex(x0, y0, za, u0, vv); cloud_tex_vertex(x1, y0, za, u1, vv); cloud_tex_vertex(x1, y1, za, u1, vv); cloud_tex_vertex(x0, y1, za, u0, vv);
                }
            }
        }
        glEnd();
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glColor4f(1,1,1,1);
}

static void draw_source_clouds(void) {
    if (g_opts.fancy_graphics) draw_source_fancy_clouds(g_frame_partial);
    else draw_source_fast_clouds(g_frame_partial);
}


static void setup_world_projection(void) {
    glViewport(0, 0, g_render_w, g_render_h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (g_render_h > 0) ? ((float)g_render_w / (float)g_render_h) : 1.0f;
    {
        float far_plane = current_render_distance_blocks() + 64.0f;
        if (far_plane < 1024.0f) far_plane = 1024.0f; /* clouds are independent from terrain distance */
        double fov = (double)g_opts.fov * (double)player_fov_multiplier_125();
        if (flat_player_head_in_water()) fov = fov * 60.0 / 70.0;
        if (fov < 30.0) fov = 30.0;
        if (fov > 170.0) fov = 170.0;
        gluPerspective(fov, (double)aspect, 0.05, (double)far_plane);
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void apply_view_bobbing(float partial) {
    if (!g_opts.view_bobbing) return;
    const PexPlayerRenderState *pr = &g_player_render_frame;
    float walk_delta = pr->distance_walked - pr->prev_distance_walked;
    float walk = -(pr->distance_walked + walk_delta * partial);
    float cam_yaw = pr->prev_camera_yaw + (pr->camera_yaw - pr->prev_camera_yaw) * partial;
    float cam_pitch = pr->prev_camera_pitch + (pr->camera_pitch - pr->prev_camera_pitch) * partial;

    /* Java 1.2.5 EntityRenderer.setupViewBobbing(float): applied before
       orientCamera(), so it affects both first person and F5 third person. */
    glTranslatef(sinf(walk * (float)M_PI) * cam_yaw * 0.5f,
                 -fabsf(cosf(walk * (float)M_PI) * cam_yaw),
                 0.0f);
    glRotatef(sinf(walk * (float)M_PI) * cam_yaw * 3.0f, 0.0f, 0.0f, 1.0f);
    glRotatef(fabsf(cosf(walk * (float)M_PI - 0.2f) * cam_yaw) * 5.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(cam_pitch, 1.0f, 0.0f, 0.0f);
}


static void apply_hurt_camera_effect(float partial) {
    const PexPlayerRenderState *pr = &g_player_render_frame;
    if (pr->dead) {
        float f = (float)pr->death_time + partial;
        glRotatef(40.0f - 8000.0f / (f + 200.0f), 0.0f, 0.0f, 1.0f);
        return;
    }
    if (pr->hurt_time <= 0 || pr->max_hurt_time <= 0) return;
    float f = (float)pr->hurt_time - partial;
    if (f <= 0.0f) return;
    f /= (float)pr->max_hurt_time;
    f = sinf(f * f * f * f * (float)M_PI);
    glRotatef(-pr->attacked_at_yaw, 0.0f, 1.0f, 0.0f);
    glRotatef(-f * 14.0f, 0.0f, 0.0f, 1.0f);
    glRotatef(pr->attacked_at_yaw, 0.0f, 1.0f, 0.0f);
}

static int third_person_camera_blocker(int bx, int by, int bz) {
    int id = flat_get_block(bx, by, bz);
    if (id == 0 || block_is_liquid(id)) return 0;
    if (id == BLOCK_SNOW_LAYER || id == BLOCK_TORCH || id == BLOCK_FIRE ||
        id == BLOCK_REDSTONE_WIRE || id == BLOCK_REEDS ||
        id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE ||
        id == BLOCK_BROWN_MUSHROOM || id == BLOCK_RED_MUSHROOM) return 0;
    return 1;
}

static int third_person_trace_blocks(float ax, float ay, float az,
                                     float bx, float by, float bz,
                                     float *out_dist) {
    float dx = bx - ax;
    float dy = by - ay;
    float dz = bz - az;
    float len = sqrtf(dx * dx + dy * dy + dz * dz);
    if (len <= 0.0001f) return 0;
    int steps = (int)(len / 0.05f) + 1;
    for (int i = 1; i <= steps; ++i) {
        float t = (float)i / (float)steps;
        float x = ax + dx * t;
        float y = ay + dy * t;
        float z = az + dz * t;
        if (third_person_camera_blocker((int)floorf(x), (int)floorf(y), (int)floorf(z))) {
            if (out_dist) *out_dist = len * t;
            return 1;
        }
    }
    return 0;
}

static float third_person_camera_distance(float x, float y, float z,
                                          float yaw, float pitch,
                                          float wanted_dist) {
    float yaw_rad = yaw * (float)M_PI / 180.0f;
    float pitch_rad = pitch * (float)M_PI / 180.0f;
    float back_x = sinf(yaw_rad) * cosf(pitch_rad);
    float back_y = sinf(pitch_rad);
    float back_z = -cosf(yaw_rad) * cosf(pitch_rad);
    float dist = wanted_dist;

    for (int i = 0; i < 8; ++i) {
        float ox = (float)(((i & 1) * 2) - 1) * 0.1f;
        float oy = (float)((((i >> 1) & 1) * 2) - 1) * 0.1f;
        float oz = (float)((((i >> 2) & 1) * 2) - 1) * 0.1f;
        float hit_dist = wanted_dist;
        if (third_person_trace_blocks(x + ox, y + oy, z + oz,
                                      x + back_x * wanted_dist + ox,
                                      y + back_y * wanted_dist + oy,
                                      z + back_z * wanted_dist + oz,
                                      &hit_dist) && hit_dist < dist) {
            dist = hit_dist;
        }
    }

    if (dist < 0.15f) dist = 0.15f;
    return dist;
}

static void apply_player_camera(float partial) {
    const PexPlayerRenderState *pr = &g_player_render_frame;
    float x = pr->prev_x + (pr->x - pr->prev_x) * partial;
    float y = pr->prev_y + (pr->y - pr->prev_y) * partial;
    float z = pr->prev_z + (pr->z - pr->prev_z) * partial;
    float yaw = lerp_angle(pr->prev_yaw, pr->yaw, partial);
    float pitch = pr->prev_pitch + (pr->pitch - pr->prev_pitch) * partial;

    /* Sneaking lowers the player camera/eye a bit, like the earlier patch. */
    if (g_screen == SCREEN_INGAME && key_down_vk(g_opts.keys[5])) y -= 0.18f;

    apply_hurt_camera_effect(partial);
    apply_view_bobbing(partial);
    apply_portal_camera_effect(partial);

    if (g_third_person_view) {
        /* Java 1.2.5 EntityRenderer.orientCamera semantics:
           0 = first person, 1 = camera behind player, 2 = camera in front
           facing back at the player.  Both third-person modes use a 4 block
           camera boom shortened by 8 small collision rays. */
        int front_view = (g_third_person_view == 2);
        float yaw_rad = yaw * (float)M_PI / 180.0f;
        float pitch_rad = pitch * (float)M_PI / 180.0f;
        float dist = 4.0f;

        float look_x = -sinf(yaw_rad) * cosf(pitch_rad);
        float look_y = -sinf(pitch_rad);
        float look_z =  cosf(yaw_rad) * cosf(pitch_rad);
        float boom_pitch = front_view ? (pitch + 180.0f) : pitch;
        dist = third_person_camera_distance(x, y, z, yaw, boom_pitch, dist);

        float cam_x = front_view ? (x + look_x * dist) : (x - look_x * dist);
        float cam_y = front_view ? (y + look_y * dist) : (y - look_y * dist);
        float cam_z = front_view ? (z + look_z * dist) : (z - look_z * dist);
        float view_yaw = front_view ? (yaw + 180.0f) : yaw;
        float view_pitch = front_view ? -pitch : pitch;

        glRotatef(view_pitch, 1.0f, 0.0f, 0.0f);
        glRotatef(view_yaw + 180.0f, 0.0f, 1.0f, 0.0f);
        glTranslatef(-cam_x, -cam_y, -cam_z);
    } else {
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

#define PEX_PORTAL_TILE 14
#define PEX_PORTAL_FRAME_COUNT 32
static unsigned char g_portal_anim_frames[PEX_PORTAL_FRAME_COUNT][16 * 16 * 4];
static int g_portal_anim_ready = 0;
static int g_portal_uploaded_frame = -1;
static unsigned char *g_portal_uploaded_rgba = NULL;
static int g_portal_uploaded_w = 0;
static int g_portal_uploaded_h = 0;
static int g_portal_anim_terrain_dirty = 0;

static int pex_portal_clamp_u8(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return v;
}

static void pex_portal_build_frames(void) {
    if (g_portal_anim_ready) return;
    PexSkyJavaRandom rng;
    pex_sky_java_random_set_seed(&rng, 100LL);
    const float pi = 3.14159265358979323846f;
    for (int frame = 0; frame < PEX_PORTAL_FRAME_COUNT; ++frame) {
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 16; ++x) {
                float accum = 0.0f;
                for (int layer = 0; layer < 2; ++layer) {
                    float cx = (float)(layer * 16) * 0.5f;
                    float cy = (float)(layer * 16) * 0.5f;
                    float dx = ((float)x - cx) / 16.0f * 2.0f;
                    float dy = ((float)y - cy) / 16.0f * 2.0f;
                    if (dx < -1.0f) dx += 2.0f;
                    if (dx >= 1.0f) dx -= 2.0f;
                    if (dy < -1.0f) dy += 2.0f;
                    if (dy >= 1.0f) dy -= 2.0f;
                    float d2 = dx * dx + dy * dy;
                    float spin = atan2f(dy, dx) + (((float)frame / 32.0f) * pi * 2.0f - d2 * 10.0f + (float)(layer * 2)) * (float)(layer * 2 - 1);
                    float wave = (sinf(spin) + 1.0f) * 0.5f;
                    accum += (wave / (d2 + 1.0f)) * 0.5f;
                }
                accum += pex_sky_java_random_next_float(&rng) * 0.1f;
                int b = (int)(accum * 100.0f + 155.0f);
                int r = (int)(accum * accum * 200.0f + 55.0f);
                int g = (int)(accum * accum * accum * accum * 255.0f);
                int a = (int)(accum * 100.0f + 155.0f);
                unsigned char *dst = &g_portal_anim_frames[frame][(y * 16 + x) * 4];
                dst[0] = (unsigned char)pex_portal_clamp_u8(r);
                dst[1] = (unsigned char)pex_portal_clamp_u8(g);
                dst[2] = (unsigned char)pex_portal_clamp_u8(b);
                dst[3] = (unsigned char)pex_portal_clamp_u8(a);
            }
        }
    }
    g_portal_anim_ready = 1;
}

static void update_portal_texture_animation(void) {
    if (!tex_terrain.id || !tex_terrain.rgba || tex_terrain.w <= 0 || tex_terrain.h <= 0) return;
    int tx = (PEX_PORTAL_TILE & 15) * 16;
    int ty = (PEX_PORTAL_TILE >> 4) * 16;
    if (tx + 16 > tex_terrain.w || ty + 16 > tex_terrain.h) return;
    pex_portal_build_frames();
    int frame = (g_ingame_ticks + 1) & (PEX_PORTAL_FRAME_COUNT - 1);
    if (frame == g_portal_uploaded_frame && g_portal_uploaded_rgba == tex_terrain.rgba &&
        g_portal_uploaded_w == tex_terrain.w && g_portal_uploaded_h == tex_terrain.h) {
        return;
    }
    const unsigned char *src = g_portal_anim_frames[frame];
    for (int y = 0; y < 16; ++y) {
        unsigned char *dst = &tex_terrain.rgba[((ty + y) * tex_terrain.w + tx) * 4];
        memcpy(dst, src + y * 16 * 4, 16u * 4u);
    }
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_terrain.w, tex_terrain.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_terrain.rgba);
    g_portal_anim_terrain_dirty = 1; /* force direct backends to rebuild their terrain texture from the updated CPU atlas */
    g_portal_uploaded_frame = frame;
    g_portal_uploaded_rgba = tex_terrain.rgba;
    g_portal_uploaded_w = tex_terrain.w;
    g_portal_uploaded_h = tex_terrain.h;
}

static float portal_interpolated_amount(float partial) {
    float a = g_prev_time_in_portal + (g_time_in_portal - g_prev_time_in_portal) * partial;
    if (a < 0.0f) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    return a;
}

static void apply_portal_camera_effect(float partial) {
    float a = portal_interpolated_amount(partial);
    if (a <= 0.0f) return;
    float warp = 5.0f / (a * a + 5.0f) - a * 0.04f;
    warp *= warp;
    float spin = ((float)g_ingame_ticks + partial) * 20.0f;
    glRotatef(spin, 0.0f, 1.0f, 1.0f);
    glScalef(1.0f / warp, 1.0f, 1.0f);
    glRotatef(-spin, 0.0f, 1.0f, 1.0f);
}

static void draw_portal_overlay(void) {
    if (!tex_terrain.id) return;
    float a = portal_interpolated_amount(g_frame_partial);
    if (a <= 0.0f) return;
    if (a < 1.0f) {
        a *= a;
        a *= a;
        a = a * 0.8f + 0.2f;
    }
    float u0, v0, u1, v1;
    terrain_tile_uv(PEX_PORTAL_TILE, &u0, &v0, &u1, &v1);
    glDisable(GL_FOG);
    setup_gui_projection();
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, a);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v1); glVertex2f(0.0f, (float)g_gui_h);
    glTexCoord2f(u1, v1); glVertex2f((float)g_gui_w, (float)g_gui_h);
    glTexCoord2f(u1, v0); glVertex2f((float)g_gui_w, 0.0f);
    glTexCoord2f(u0, v0); glVertex2f(0.0f, 0.0f);
    glEnd();
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_ALPHA_TEST);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}


static int terrain_tile_has_opaque_pixels(int tile) {
    if (!tex_terrain.rgba || tex_terrain.w <= 0 || tex_terrain.h <= 0) return 0;
    int tx = (tile & 15) * 16;
    int ty = (tile >> 4) * 16;
    if (tx + 16 > tex_terrain.w || ty + 16 > tex_terrain.h) return 0;
    int count = 0;
    for (int yy = 0; yy < 16; yy++) {
        for (int xx = 0; xx < 16; xx++) {
            unsigned char *p = &tex_terrain.rgba[((ty + yy) * tex_terrain.w + (tx + xx)) * 4];
            if (p[3] > 8) count++;
        }
    }
    return count > 32;
}

static int terrain_tile_liquid_like(int tile, int water) {
    if (!tex_terrain.rgba || tex_terrain.w <= 0 || tex_terrain.h <= 0) return 0;
    int tx = (tile & 15) * 16;
    int ty = (tile >> 4) * 16;
    if (tx + 16 > tex_terrain.w || ty + 16 > tex_terrain.h) return 0;
    int opaque = 0, match = 0, gray = 0;
    for (int yy = 0; yy < 16; yy++) {
        for (int xx = 0; xx < 16; xx++) {
            unsigned char *p = &tex_terrain.rgba[((ty + yy) * tex_terrain.w + (tx + xx)) * 4];
            if (p[3] <= 8) continue;
            int r = p[0], g = p[1], b = p[2];
            ++opaque;
            if (abs(r - g) < 12 && abs(g - b) < 12 && abs(r - b) < 12) ++gray;
            if (water) {
                if (b >= 70 && b >= r + 12 && g >= r - 8) ++match;
            } else {
                if (r >= 100 && g >= 35 && r >= b + 35 && g >= b + 5) ++match;
            }
        }
    }
    if (opaque < 32) return 0;
    if (gray * 3 > opaque) return 0;
    return match * 3 >= opaque;
}

static int terrain_liquid_tile_valid_cached(int tile, int water) {
    static GLuint cached_tex = 0;
    static unsigned char *cached_rgba = NULL;
    static int values[4] = {-1, -1, -1, -1};
    int idx = -1;
    if (tile == 205 && water) idx = 0;
    else if (tile == 206 && water) idx = 1;
    else if (tile == 237 && !water) idx = 2;
    else if (tile == 238 && !water) idx = 3;
    if (idx < 0) return terrain_tile_liquid_like(tile, water);
    if (cached_tex != tex_terrain.id || cached_rgba != tex_terrain.rgba) {
        cached_tex = tex_terrain.id;
        cached_rgba = tex_terrain.rgba;
        for (int i = 0; i < 4; ++i) values[i] = -1;
    }
    if (values[idx] < 0) values[idx] = terrain_tile_liquid_like(tile, water);
    return values[idx];
}

static int chest_top_tile(void) { return 25; }
static int chest_side_tile(void) { return 26; }
static int chest_front_tile(void) { return 27; }
/* Old terrain.png stores the double/large chest strip directly below the
   single chest top/side strip.  Do not use cropped cuboid UVs or entity
   item/chest.png here; the terrain atlas supplied by the downloaded client.jar
   is the source of truth for this renderer. */
static int large_chest_left_tile(void) { return chest_top_tile() + 16; }
static int large_chest_right_tile(void) { return chest_side_tile() + 16; }
static int furnace_front_tile(void) { return 44; }
static int furnace_front_lit_tile(void) { return 61; }
static int furnace_side_tile(void) { return 45; }
static int furnace_top_tile(void) { return 62; }

/* Java 1.2.5 BlockFluid uses 205/206 for water and 237/238 for lava.  Some
   active packs in this port contain fallback checker/generated pixels in those
   slots, so verify the color family before accepting them. */
static int water_top_tile(void)  { return terrain_liquid_tile_valid_cached(205, 1) ? 205 : 14; }
static int water_side_tile(void) { return terrain_liquid_tile_valid_cached(206, 1) ? 206 : water_top_tile(); }
static int lava_top_tile(void)   { return terrain_liquid_tile_valid_cached(237, 0) ? 237 : 30; }
static int lava_side_tile(void)  { return terrain_liquid_tile_valid_cached(238, 0) ? 238 : lava_top_tile(); }
static int liquid_top_tile_for_block(int id) { return (id == BLOCK_WATER || id == BLOCK_STILL_WATER) ? water_top_tile() : lava_top_tile(); }
static int liquid_side_tile_for_block(int id) { return (id == BLOCK_WATER || id == BLOCK_STILL_WATER) ? water_side_tile() : lava_side_tile(); }


/* Async terrain mesh path -----------------------------------------------------------
   D3D9/D3D11 upload worker-built CPU meshes into native vertex/index buffers.
   OpenGL keeps the same worker-built CPU mesh and draws it with client arrays on
   the render thread.  That removes the old glNewList/glBegin section rebuild from
   the main thread while still keeping all GL calls on the GL-owning thread. */
typedef struct FlatDirectMeshBuilder {
    PexVertex *v;
    uint32_t *i;
    uint32_t vcount, vcap;
    uint32_t icount, icap;
    int quad_cursor;
    uint32_t color;
    uint32_t light;
    int light_resolved;
} FlatDirectMeshBuilder;

typedef struct FlatGLCpuMesh {
    PexVertex *v;
    PexVertex *lit_v;
    uint32_t *i;
    uint32_t vcount, icount;
    GLuint list;
    unsigned int version;
    unsigned int lit_lightmap_version;
    int origin_x, origin_z;
    int valid;
} FlatGLCpuMesh;

static FlatGLCpuMesh g_flat_section_gl_cpu_mesh[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS][2];
static int g_flat_gl_cpu_mesh_origin_x = 0x7fffffff;
static int g_flat_gl_cpu_mesh_origin_z = 0x7fffffff;

static PEX_THREAD_LOCAL FlatDirectMeshBuilder *g_flat_direct_builder = NULL;
static PEX_THREAD_LOCAL int g_flat_direct_capture_only = 0;
static PEX_THREAD_LOCAL FlatDirectMeshBuilder *g_flat_direct_capture_out0 = NULL;
static PEX_THREAD_LOCAL FlatDirectMeshBuilder *g_flat_direct_capture_out1 = NULL;
static PEX_THREAD_LOCAL int *g_flat_direct_capture_skip0 = NULL;
static PEX_THREAD_LOCAL int *g_flat_direct_capture_skip1 = NULL;
static PEX_THREAD_LOCAL int g_flat_direct_capture_success = 0;
static PexTextureHandle g_flat_direct_terrain_texture = 0;
static unsigned char *g_flat_direct_terrain_rgba = NULL;
static int g_flat_direct_terrain_w = 0, g_flat_direct_terrain_h = 0;

static PexRendererBackend *flat_direct_backend(void) {
#if defined(PEX_PLATFORM_PSP)
    /* PSP has its own GU mesh backend.  This is important for real-world mode:
       it lets section meshing run on the worker and only uploads/adopts the
       completed mesh on the render thread instead of rebuilding GL display
       lists inside the frame. */
    return psp_gu_get_backend();
#elif defined(PEX_PLATFORM_WII)
    return wii_gx_get_backend();
#else
    if (pex_using_d3d9()) return renderer_d3d9_get_backend();
    if (pex_using_d3d11()) return renderer_d3d11_get_backend();
    return NULL;
#endif
}

static uint32_t flat_pack_current_rgba(void) {
    int r = (int)(g_d3d9.color[0] * 255.0f + 0.5f);
    int g = (int)(g_d3d9.color[1] * 255.0f + 0.5f);
    int b = (int)(g_d3d9.color[2] * 255.0f + 0.5f);
    int a = (int)(g_d3d9.color[3] * 255.0f + 0.5f);
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    if (a < 0) a = 0; if (a > 255) a = 255;
    /* PexVertex.color is memory-RGBA.  That is exactly what D3D11
       DXGI_FORMAT_R8G8B8A8_UNORM expects, and D3D9 converts it to ARGB
       at upload time. */
    return ((uint32_t)r) | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24);
}

static uint32_t flat_pack_rgba4f(float r, float g, float b, float a) {
    int ri = (int)(r * 255.0f + 0.5f);
    int gi = (int)(g * 255.0f + 0.5f);
    int bi = (int)(b * 255.0f + 0.5f);
    int ai = (int)(a * 255.0f + 0.5f);
    if (ri < 0) ri = 0; if (ri > 255) ri = 255;
    if (gi < 0) gi = 0; if (gi > 255) gi = 255;
    if (bi < 0) bi = 0; if (bi > 255) bi = 255;
    if (ai < 0) ai = 0; if (ai > 255) ai = 255;
    return ((uint32_t)ri) | ((uint32_t)gi << 8) | ((uint32_t)bi << 16) | ((uint32_t)ai << 24);
}

static uint32_t flat_fullbright_packed_light(void) {
    return (15u << 20) | (15u << 4);
}

static uint32_t flat_apply_lightmap_to_color(uint32_t rgba, uint32_t packed_light) {
    float lr, lg, lb;
    flat_lightmap_color((int)packed_light, &lr, &lg, &lb);
    int r = (int)((float)(rgba & 255u) * lr + 0.5f);
    int g = (int)((float)((rgba >> 8) & 255u) * lg + 0.5f);
    int b = (int)((float)((rgba >> 16) & 255u) * lb + 0.5f);
    int a = (int)((rgba >> 24) & 255u);
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    return ((uint32_t)r) | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24);
}

static void flat_direct_set_color4f(float r, float g, float b, float a) {
    if (g_flat_direct_builder) {
        g_flat_direct_builder->color = flat_pack_rgba4f(r, g, b, a);
    } else {
        glColor4f(r, g, b, a);
    }
}

static void flat_direct_set_color_light4f(float r, float g, float b, float a, uint32_t packed_light) {
    if (g_flat_direct_builder) {
        g_flat_direct_builder->color = flat_pack_rgba4f(r, g, b, a);
        g_flat_direct_builder->light = packed_light;
    } else {
        float lr, lg, lb;
        flat_lightmap_color((int)packed_light, &lr, &lg, &lb);
        flat_direct_set_color4f(r * lr, g * lg, b * lb, a);
    }
}

static PEX_THREAD_LOCAL int g_world_style_x = 0;
static PEX_THREAD_LOCAL int g_world_style_y = 64;
static PEX_THREAD_LOCAL int g_world_style_z = 0;
static PEX_THREAD_LOCAL int g_world_light_x = 0;
static PEX_THREAD_LOCAL int g_world_light_y = 65;
static PEX_THREAD_LOCAL int g_world_light_z = 0;
/* GUI/in-hand block item models must be lit like Java RenderItem.func_1227_a:
   fixed face shading only, not the last sampled world light cell.  Without this
   a stale zero-light world coordinate can make inventory/held 3D blocks render black. */
static PEX_THREAD_LOCAL int g_force_fullbright_item_model = 0;

static void world_style_set_pos(int x, int y, int z) {
    g_world_style_x = x;
    g_world_style_y = y;
    g_world_style_z = z;
}

static void world_light_set_pos(int x, int y, int z) {
    g_world_light_x = x;
    g_world_light_y = y;
    g_world_light_z = z;
}

static void world_light_set_pos_for_face(int x, int y, int z, int face) {
    int lx = x, ly = y, lz = z;
    if (face == 0) ly--;
    else if (face == 1) ly++;
    else if (face == 2) lz--;
    else if (face == 3) lz++;
    else if (face == 4) lx--;
    else if (face == 5) lx++;
    world_light_set_pos(lx, ly, lz);
}

/* World brightness now comes from the Java-style sky/block light helpers in
   inventory.c.  Do not add roof/canopy hacks here; propagate sky/block light
   into g_flat_*_light instead. */

static int colorizer_lookup(Texture *tex, float temp, float humid, int fallback_rgb) {
    if (!tex || !tex->rgba || tex->w <= 0 || tex->h <= 0) return fallback_rgb;
    if (temp < 0.0f) temp = 0.0f; if (temp > 1.0f) temp = 1.0f;
    if (humid < 0.0f) humid = 0.0f; if (humid > 1.0f) humid = 1.0f;
    humid *= temp;
    int ix = (int)((1.0f - temp) * (float)(tex->w - 1));
    int iy = (int)((1.0f - humid) * (float)(tex->h - 1));
    if (ix < 0) ix = 0; if (ix >= tex->w) ix = tex->w - 1;
    if (iy < 0) iy = 0; if (iy >= tex->h) iy = tex->h - 1;
    unsigned char *p = &tex->rgba[((iy * tex->w) + ix) * 4];
    return ((int)p[0] << 16) | ((int)p[1] << 8) | (int)p[2];
}

typedef struct WorldBiomeColorCache {
    BiomeManager mgr;
    int mgr_ready;
    long long seed;
    int chunk_x;
    int chunk_z;
    unsigned char valid[16 * 16];
    int grass[16 * 16];
    int foliage[16 * 16];
    float temp[16 * 16];
    float humid[16 * 16];
} WorldBiomeColorCache;

static PEX_THREAD_LOCAL WorldBiomeColorCache g_world_biome_color_cache;

static void world_biome_color_cache_reset_chunk(WorldBiomeColorCache *c, int chunk_x, int chunk_z) {
    c->chunk_x = chunk_x;
    c->chunk_z = chunk_z;
    memset(c->valid, 0, sizeof(c->valid));
}

static void world_biome_color_cache_prepare(WorldBiomeColorCache *c, int x, int z) {
    int cx = floor_div16(x);
    int cz = floor_div16(z);
    if (!c->mgr_ready || c->seed != g_world_seed) {
        if (c->mgr_ready) { free(c->mgr.temp); memset(&c->mgr, 0, sizeof(c->mgr)); }
        biome_manager_init(&c->mgr, g_world_seed);
        c->seed = g_world_seed;
        c->mgr_ready = 1;
        c->chunk_x = 0x7fffffff;
        c->chunk_z = 0x7fffffff;
        memset(c->valid, 0, sizeof(c->valid));
    }
    if (c->chunk_x != cx || c->chunk_z != cz) {
        world_biome_color_cache_reset_chunk(c, cx, cz);
    }
}

static void biome_temp_humid_at(int x, int z, float *temp, float *humid) {
    WorldBiomeColorCache *c = &g_world_biome_color_cache;
    world_biome_color_cache_prepare(c, x, z);
    int lx = x - c->chunk_x * 16;
    int lz = z - c->chunk_z * 16;
    int idx = lz * 16 + lx;
    if (!c->valid[idx]) {
        WorldBiome *biomes = biome_manager_get(&c->mgr, c->chunk_x * 16, c->chunk_z * 16, 16, 16);
        for (int i = 0; i < 16 * 16; ++i) {
            float t = biomes[i].temperature;
            float h = biomes[i].rainfall;
            c->temp[i] = t;
            c->humid[i] = h;
            c->grass[i] = colorizer_lookup(&tex_grasscolor, t, h, 0x6FAD3A);
            c->foliage[i] = colorizer_lookup(&tex_foliagecolor, t, h, 0x59A83A);
            c->valid[i] = 1;
        }
    }
    *temp = c->temp[idx];
    *humid = c->humid[idx];
}

static int java_grass_color_at(int x, int z) {
    WorldBiomeColorCache *c = &g_world_biome_color_cache;
    world_biome_color_cache_prepare(c, x, z);
    int lx = x - c->chunk_x * 16;
    int lz = z - c->chunk_z * 16;
    int idx = lz * 16 + lx;
    if (!c->valid[idx]) {
        float t, h;
        biome_temp_humid_at(x, z, &t, &h);
    }
    return c->grass[idx];
}

static int java_foliage_color_at(int x, int z) {
    WorldBiomeColorCache *c = &g_world_biome_color_cache;
    world_biome_color_cache_prepare(c, x, z);
    int lx = x - c->chunk_x * 16;
    int lz = z - c->chunk_z * 16;
    int idx = lz * 16 + lx;
    if (!c->valid[idx]) {
        float t, h;
        biome_temp_humid_at(x, z, &t, &h);
    }
    return c->foliage[idx];
}

static int java_block_normal_cube_at(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    return id != 0 && flat_light_opacity_for_id(id) >= 255 && flat_block_is_solid(id);
}

static int flat_direct_reserve(FlatDirectMeshBuilder *b, uint32_t add_v, uint32_t add_i) {
    if (!b) return 0;
    if (b->vcount + add_v > b->vcap) {
        uint32_t nc = b->vcap ? b->vcap * 2u : 4096u;
        while (nc < b->vcount + add_v) nc *= 2u;
        PexVertex *nv = (PexVertex*)realloc(b->v, (size_t)nc * sizeof(PexVertex));
        if (!nv) return 0;
        b->v = nv; b->vcap = nc;
    }
    if (b->icount + add_i > b->icap) {
        uint32_t nc = b->icap ? b->icap * 2u : 6144u;
        while (nc < b->icount + add_i) nc *= 2u;
        uint32_t *ni = (uint32_t*)realloc(b->i, (size_t)nc * sizeof(uint32_t));
        if (!ni) return 0;
        b->i = ni; b->icap = nc;
    }
    return 1;
}

static void flat_direct_begin(FlatDirectMeshBuilder *b) {
    memset(b, 0, sizeof(*b));
    b->color = flat_pack_rgba4f(1.0f, 1.0f, 1.0f, 1.0f);
    b->light = flat_fullbright_packed_light();
    g_flat_direct_builder = b;
    pex_gl_suppress_immediate(1);
}

static void flat_direct_end(void) {
    pex_gl_suppress_immediate(0);
    g_flat_direct_builder = NULL;
}

static void pex_world_gl_begin_guard(GLenum mode) {
    if (!g_flat_direct_builder) glBegin(mode);
}

static void pex_world_gl_end_guard(void) {
    if (!g_flat_direct_builder) glEnd();
}

static void pex_world_gl_enable_guard(GLenum cap) { if (!g_flat_direct_builder) glEnable(cap); }
static void pex_world_gl_disable_guard(GLenum cap) { if (!g_flat_direct_builder) glDisable(cap); }
static void pex_world_gl_bind_texture_guard(GLenum target, GLuint tex) { if (!g_flat_direct_builder) glBindTexture(target, tex); }
static void pex_world_gl_alpha_func_guard(GLenum func, GLfloat ref) { if (!g_flat_direct_builder) glAlphaFunc(func, ref); }
static void pex_world_gl_depth_mask_guard(GLboolean flag) { if (!g_flat_direct_builder) glDepthMask(flag); }
static void world_gl_color_guard(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { if (!g_flat_direct_builder) glColor4f(r, g, b, a); }

/* main_android_tv.c maps legacy GL calls to the GLES2 shim before including this
   file.  The direct-mesh guard below intentionally wraps those already-expanded
   calls, but redefining the same macros trips Android/Clang warnings.  Undefine
   any platform aliases first, then install the local suppression guard. */
#ifdef glBegin
#undef glBegin
#endif
#ifdef glEnd
#undef glEnd
#endif
#ifdef glEnable
#undef glEnable
#endif
#ifdef glDisable
#undef glDisable
#endif
#ifdef glBindTexture
#undef glBindTexture
#endif
#ifdef glAlphaFunc
#undef glAlphaFunc
#endif
#ifdef glDepthMask
#undef glDepthMask
#endif
#ifdef glColor4f
#undef glColor4f
#endif
#define glBegin(mode) pex_world_gl_begin_guard(mode)
#define glEnd() pex_world_gl_end_guard()
#define glEnable(cap) pex_world_gl_enable_guard(cap)
#define glDisable(cap) pex_world_gl_disable_guard(cap)
#define glBindTexture(target, tex) pex_world_gl_bind_texture_guard(target, tex)
#define glAlphaFunc(func, ref) pex_world_gl_alpha_func_guard(func, ref)
#define glDepthMask(flag) pex_world_gl_depth_mask_guard(flag)
#define glColor4f(r,g,b,a) world_gl_color_guard(r,g,b,a)

static void flat_direct_vertex(float x, float y, float z, float u, float v) {
    FlatDirectMeshBuilder *b = g_flat_direct_builder;
    if (!b) return;
    if (!flat_direct_reserve(b, 1, 6)) return;
    uint32_t idx = b->vcount++;
    b->v[idx].x = x; b->v[idx].y = y; b->v[idx].z = z;
    b->v[idx].u = u; b->v[idx].v = v;
    b->v[idx].color = b->color;
    b->v[idx].light = b->light;
    b->quad_cursor++;
    if (b->quad_cursor == 4) {
        uint32_t q0 = idx - 3, q1 = idx - 2, q2 = idx - 1, q3 = idx;
        b->i[b->icount++] = q0; b->i[b->icount++] = q1; b->i[b->icount++] = q2;
        b->i[b->icount++] = q0; b->i[b->icount++] = q2; b->i[b->icount++] = q3;
        b->quad_cursor = 0;
    }
}

static void flat_direct_free_builder(FlatDirectMeshBuilder *b) {
    if (!b) return;
    free(b->v); free(b->i);
    memset(b, 0, sizeof(*b));
}

static int flat_async_section_mesh_enabled(void) {
#if defined(PEX_PLATFORM_WII)
    /* libogc/LWP builds have been crashing in the worker-mesh path while
       preparing chunks: Dolphin reports DSI at flat_get_block() with a bogus
       async snapshot pointer. Keep Wii section meshing synchronous on the
       main thread until the port is stable. */
    return 0;
#elif defined(PEX_PLATFORM_PSP)
    return flat_direct_backend() != NULL;
#else
    return tex_terrain.rgba != NULL;
#endif
}

/* Fast graphics no longer has its own terrain renderer.  Both Fast and Fancy
   now use the same stable Java/Fancy terrain pipeline: separate liquid pass,
   cutout pass, and the same face-visibility rules.  Fast only disables visual
   extras through g_opts.fancy_graphics (smooth lighting, fancy clouds, shadows,
   transparent leaf textures, grass-side overlay), not through a different mesh
   topology. */
static int flat_fancy_terrain_pipeline_enabled(void) {
    return 1;
}

static int flat_separate_liquid_pass_enabled(void) {
    return flat_fancy_terrain_pipeline_enabled();
}

static int flat_fancy_cutout_terrain_enabled(void) {
    return flat_fancy_terrain_pipeline_enabled();
}

static int flat_fancy_leaf_texture_enabled(void) {
    return g_opts.fancy_graphics;
}

static int flat_fancy_grass_overlay_enabled(void) {
    return g_opts.fancy_graphics;
}

static int flat_display_lists_supported(void) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII) || defined(PEX_PLATFORM_ANDROID) || defined(PEX_PLATFORM_ANDROID_TV) || defined(PEX_PLATFORM_LGWEBOS)
    return 0;
#else
    /* Cached display lists bake glColor calls, so they cannot follow Java's
       dynamic lightmap without rebuilding.  Keep async OpenGL terrain in CPU
       vertex arrays and re-resolve packed sky/block light at draw time. */
    return 0;
#endif
}

static GLuint gl_compile_flat_mesh_list(const FlatDirectMeshBuilder *b) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII) || defined(PEX_PLATFORM_ANDROID) || defined(PEX_PLATFORM_ANDROID_TV) || defined(PEX_PLATFORM_LGWEBOS)
    (void)b;
    return 0;
#else
    if (!b || !b->v || !b->i || b->vcount == 0 || b->icount < 3 || !flat_display_lists_supported()) return 0;
    GLuint list = glGenLists(1);
    if (!list) return 0;
    glNewList(list, GL_COMPILE);
    glBegin(GL_TRIANGLES);
    for (uint32_t n = 0; n < b->icount; ++n) {
        uint32_t vi = b->i[n];
        if (vi >= b->vcount) continue;
        const PexVertex *v = &b->v[vi];
        uint32_t color = flat_apply_lightmap_to_color(v->color, v->light);
        const unsigned char *c = (const unsigned char*)&color;
        glColor4ub(c[0], c[1], c[2], c[3]);
        glTexCoord2f(v->u, v->v);
        glVertex3f(v->x, v->y, v->z);
    }
    glEnd();
    glEndList();
    return list;
#endif
}

static void flat_gl_cpu_mesh_free(FlatGLCpuMesh *m) {
    if (!m) return;
#if !defined(PEX_PLATFORM_PSP) && !defined(PEX_PLATFORM_WII) && !defined(PEX_PLATFORM_ANDROID) && !defined(PEX_PLATFORM_ANDROID_TV) && !defined(PEX_PLATFORM_LGWEBOS)
    if (m->list && flat_display_lists_supported()) glDeleteLists(m->list, 1);
#endif
    free(m->v);
    free(m->lit_v);
    free(m->i);
    memset(m, 0, sizeof(*m));
}

static void flat_direct_resolve_builder_lightmap(FlatDirectMeshBuilder *b) {
    if (!b || !b->v || b->light_resolved) return;
    for (uint32_t i = 0; i < b->vcount; ++i) {
        b->v[i].color = flat_apply_lightmap_to_color(b->v[i].color, b->v[i].light);
    }
    b->light_resolved = 1;
}

static void flat_cpu_mesh_free_all(void) {
    for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; ++sy) {
        for (int cz = 0; cz < FLAT_RENDER_CHUNKS; ++cz) {
            for (int cx = 0; cx < FLAT_RENDER_CHUNKS; ++cx) {
                flat_gl_cpu_mesh_free(&g_flat_section_gl_cpu_mesh[sy][cz][cx][0]);
                flat_gl_cpu_mesh_free(&g_flat_section_gl_cpu_mesh[sy][cz][cx][1]);
            }
        }
    }
}

static void flat_cpu_mesh_remap_shift(int old_origin_x, int old_origin_z,
                                                int new_origin_x, int new_origin_z) {
    /* Android/OpenGL CPU meshes are stored outside the normal section-handle
       arrays.  When the 256x256 active window slides, keep meshes for chunks
       that remain in the window instead of dropping every section and forcing a
       full-world rebuild on the next frame. */
    if (old_origin_x == new_origin_x && old_origin_z == new_origin_z) return;
    if (flat_direct_backend()) return;

    FlatGLCpuMesh old_mesh[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS][2];
    memcpy(old_mesh, g_flat_section_gl_cpu_mesh, sizeof(old_mesh));
    memset(g_flat_section_gl_cpu_mesh, 0, sizeof(g_flat_section_gl_cpu_mesh));

    int old_base_cx = floor_div16(old_origin_x);
    int old_base_cz = floor_div16(old_origin_z);
    int new_base_cx = floor_div16(new_origin_x);
    int new_base_cz = floor_div16(new_origin_z);

    for (int ncz = 0; ncz < FLAT_RENDER_CHUNKS; ++ncz) {
        for (int ncx = 0; ncx < FLAT_RENDER_CHUNKS; ++ncx) {
            int wcx = new_base_cx + ncx;
            int wcz = new_base_cz + ncz;
            if (!stream_world_chunk_in_window(wcx, wcz, old_origin_x, old_origin_z)) continue;
            int ocx = wcx - old_base_cx;
            int ocz = wcz - old_base_cz;
            if (ocx < 0 || ocx >= FLAT_RENDER_CHUNKS || ocz < 0 || ocz >= FLAT_RENDER_CHUNKS) continue;
            for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; ++sy) {
                for (int pass = 0; pass < 2; ++pass) {
                    g_flat_section_gl_cpu_mesh[sy][ncz][ncx][pass] = old_mesh[sy][ocz][ocx][pass];
                    g_flat_section_gl_cpu_mesh[sy][ncz][ncx][pass].origin_x = new_origin_x;
                    g_flat_section_gl_cpu_mesh[sy][ncz][ncx][pass].origin_z = new_origin_z;
                    memset(&old_mesh[sy][ocz][ocx][pass], 0, sizeof(old_mesh[sy][ocz][ocx][pass]));
                }
            }
        }
    }

    for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; ++sy) {
        for (int cz = 0; cz < FLAT_RENDER_CHUNKS; ++cz) {
            for (int cx = 0; cx < FLAT_RENDER_CHUNKS; ++cx) {
                flat_gl_cpu_mesh_free(&old_mesh[sy][cz][cx][0]);
                flat_gl_cpu_mesh_free(&old_mesh[sy][cz][cx][1]);
            }
        }
    }

    g_flat_gl_cpu_mesh_origin_x = new_origin_x;
    g_flat_gl_cpu_mesh_origin_z = new_origin_z;
}

static void flat_gl_cpu_mesh_check_origin(void) {
    if (g_flat_gl_cpu_mesh_origin_x == g_flat_world_origin_x &&
        g_flat_gl_cpu_mesh_origin_z == g_flat_world_origin_z) return;

    /* Keep the OpenGL CPU/display-list section meshes that still belong to
       chunks inside the active world window.  The old path freed every GL mesh
       on each origin slide and dirtied the whole render cache; walking across a
       stream boundary then forced a visible-world rebuild/install storm.  New
       strips are already marked invalid by stream_remap_render_chunks(), so only
       the first initialization still needs the full invalidate fallback. */
    if (!flat_direct_backend() &&
        g_flat_gl_cpu_mesh_origin_x != 0x7fffffff &&
        g_flat_gl_cpu_mesh_origin_z != 0x7fffffff) {
        flat_cpu_mesh_remap_shift(g_flat_gl_cpu_mesh_origin_x,
                                  g_flat_gl_cpu_mesh_origin_z,
                                  g_flat_world_origin_x,
                                  g_flat_world_origin_z);
        return;
    }

    flat_cpu_mesh_free_all();
    g_flat_gl_cpu_mesh_origin_x = g_flat_world_origin_x;
    g_flat_gl_cpu_mesh_origin_z = g_flat_world_origin_z;
    for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; ++sy) {
        for (int cz = 0; cz < FLAT_RENDER_CHUNKS; ++cz) {
            for (int cx = 0; cx < FLAT_RENDER_CHUNKS; ++cx) {
                if (!flat_direct_backend()) {
                    g_flat_section_valid[sy][cz][cx] = 0;
                    g_flat_section_dirty[sy][cz][cx] = 1;
                    flat_note_section_mesh_changed(sy, cz, cx);
                }
            }
        }
    }
}

static void flat_gl_cpu_mesh_install(int sy, int cz, int cx, int pass, FlatDirectMeshBuilder *b, int skip, unsigned int version, int origin_x, int origin_z) {
    FlatGLCpuMesh *m = &g_flat_section_gl_cpu_mesh[sy][cz][cx][pass];
    if (g_loggy_enabled && !skip && b) {
        g_loggy_mesh_installs++;
        g_loggy_mesh_install_vertices += (int)b->vcount;
        g_loggy_mesh_install_indices += (int)b->icount;
    }
    flat_gl_cpu_mesh_free(m);
    if (skip || !b || b->vcount == 0 || b->icount == 0) return;
    m->list = gl_compile_flat_mesh_list(b);
    if (m->list) {
        free(b->v);
        free(b->i);
        m->v = NULL;
        m->i = NULL;
    } else {
        m->v = b->v;
        m->i = b->i;
    }
    m->vcount = b->vcount;
    m->icount = b->icount;
    m->version = version;
    m->origin_x = origin_x;
    m->origin_z = origin_z;
    m->valid = 1;
    memset(b, 0, sizeof(*b));
}

static int flat_gl_cpu_mesh_drawable(int sy, int cz, int cx, int pass) {
    FlatGLCpuMesh *m = &g_flat_section_gl_cpu_mesh[sy][cz][cx][pass];
    return m->valid && ((m->list != 0) || (m->v && m->i)) && m->vcount > 0 && m->icount > 0 &&
           m->origin_x == g_flat_world_origin_x && m->origin_z == g_flat_world_origin_z;
}

static int flat_gl_cpu_mesh_ready(int sy, int cz, int cx, int pass) {
    return flat_gl_cpu_mesh_drawable(sy, cz, cx, pass) &&
           g_flat_section_gl_cpu_mesh[sy][cz][cx][pass].version == g_flat_section_mesh_version[sy][cz][cx] &&
           flat_chunk_light_ready(cx, cz) &&
           g_flat_section_mesh_light_version[sy][cz][cx] == g_flat_chunk_light_version[cz][cx];
}

static void flat_gl_cpu_mesh_refresh_lightmap(FlatGLCpuMesh *m) {
    if (!m || !m->valid || !m->v || m->vcount == 0) return;
    unsigned int version = flat_current_lightmap_version();
    if (m->lit_v && m->lit_lightmap_version == version) return;
    PexVertex *nv = (PexVertex*)realloc(m->lit_v, (size_t)m->vcount * sizeof(PexVertex));
    if (!nv) return;
    m->lit_v = nv;
    for (uint32_t i = 0; i < m->vcount; ++i) {
        m->lit_v[i] = m->v[i];
        m->lit_v[i].color = flat_apply_lightmap_to_color(m->v[i].color, m->v[i].light);
    }
    m->lit_lightmap_version = version;
}

#if defined(PEX_PLATFORM_ANDROID) || defined(PEX_PLATFORM_ANDROID_TV)
static GLushort *g_flat_android_index16_tmp = NULL;
static uint32_t g_flat_android_index16_cap = 0;

static int flat_android_ensure_index16_tmp(uint32_t count) {
    if (count <= g_flat_android_index16_cap) return 1;
    uint32_t cap = g_flat_android_index16_cap ? g_flat_android_index16_cap : 4096u;
    while (cap < count) cap *= 2u;
    GLushort *nv = (GLushort*)realloc(g_flat_android_index16_tmp, (size_t)cap * sizeof(*nv));
    if (!nv) return 0;
    g_flat_android_index16_tmp = nv;
    g_flat_android_index16_cap = cap;
    return 1;
}

static void flat_gl_draw_cpu_mesh_android16(FlatGLCpuMesh *m) {
    /* Android GLES2 cannot draw GL_UNSIGNED_INT indices natively.  The shim has
       a fast path for 16-bit indices, but falls back to allocating/duplicating
       every indexed vertex when a section references more than 65535 vertices.
       Fast graphics can still produce those big solid-pass sections, so split
       the mesh into triangle windows, rebase each window to zero, and feed the
       GLES2 fast path with GL_UNSIGNED_SHORT. */
    const uint32_t max_window_vertices = 65535u;
    const PexVertex *draw_v = m->lit_v ? m->lit_v : m->v;
    uint32_t tri = 0;
    while (tri + 2u < m->icount) {
        uint32_t first = tri;
        uint32_t min_idx = UINT32_MAX;
        uint32_t max_idx = 0;
        uint32_t out_count = 0;

        while (tri + 2u < m->icount) {
            uint32_t a = m->i[tri + 0u];
            uint32_t b = m->i[tri + 1u];
            uint32_t c = m->i[tri + 2u];
            uint32_t tmin = a < b ? (a < c ? a : c) : (b < c ? b : c);
            uint32_t tmax = a > b ? (a > c ? a : c) : (b > c ? b : c);
            uint32_t nmin = min_idx == UINT32_MAX || tmin < min_idx ? tmin : min_idx;
            uint32_t nmax = tmax > max_idx ? tmax : max_idx;

            if (out_count > 0u && (nmax - nmin) >= max_window_vertices) break;
            min_idx = nmin;
            max_idx = nmax;
            out_count += 3u;
            tri += 3u;
        }

        if (out_count == 0u || min_idx == UINT32_MAX || max_idx >= m->vcount) {
            tri = first + 3u;
            continue;
        }
        if (!flat_android_ensure_index16_tmp(out_count)) return;
        for (uint32_t i = 0; i < out_count; ++i) {
            uint32_t idx = m->i[first + i];
            g_flat_android_index16_tmp[i] = (GLushort)(idx - min_idx);
        }

        const PexVertex *base = &draw_v[min_idx];
        glVertexPointer(3, GL_FLOAT, sizeof(PexVertex), (const GLvoid*)&base[0].x);
        glTexCoordPointer(2, GL_FLOAT, sizeof(PexVertex), (const GLvoid*)&base[0].u);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(PexVertex), (const GLvoid*)&base[0].color);
        glDrawElements(GL_TRIANGLES, (GLsizei)out_count, GL_UNSIGNED_SHORT, (const GLvoid*)g_flat_android_index16_tmp);
    }
}
#endif

static void flat_gl_draw_cpu_mesh(FlatGLCpuMesh *m) {
#if defined(PEX_PLATFORM_PSP)
    (void)m;
    /* PSP uses the direct PexRendererBackend mesh path.  The OpenGL client-array
       fallback is desktop-only and pspsdk does not define GL client-array APIs. */
#else
    if (!m || !m->valid || m->icount < 3) return;
    if (m->list && flat_display_lists_supported()) { glCallList(m->list); return; }
    if (!m->v || !m->i) return;
    flat_gl_cpu_mesh_refresh_lightmap(m);
    const PexVertex *draw_v = m->lit_v ? m->lit_v : m->v;
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
#if defined(PEX_PLATFORM_ANDROID) || defined(PEX_PLATFORM_ANDROID_TV)
    flat_gl_draw_cpu_mesh_android16(m);
#else
    glVertexPointer(3, GL_FLOAT, sizeof(PexVertex), (const GLvoid*)&draw_v[0].x);
    glTexCoordPointer(2, GL_FLOAT, sizeof(PexVertex), (const GLvoid*)&draw_v[0].u);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(PexVertex), (const GLvoid*)&draw_v[0].color);
    glDrawElements(GL_TRIANGLES, (GLsizei)m->icount, GL_UNSIGNED_INT, (const GLvoid*)m->i);
#endif
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
#endif
}

static PexTextureHandle flat_direct_terrain_handle(void) {
    PexRendererBackend *rb = flat_direct_backend();
    if (!rb || !tex_terrain.rgba || tex_terrain.w <= 0 || tex_terrain.h <= 0) return 0;
    if (g_portal_anim_terrain_dirty) {
        g_flat_direct_terrain_rgba = NULL;
        g_portal_anim_terrain_dirty = 0;
    }
    if (g_flat_direct_terrain_texture && g_flat_direct_terrain_rgba == tex_terrain.rgba &&
        g_flat_direct_terrain_w == tex_terrain.w && g_flat_direct_terrain_h == tex_terrain.h) {
        return g_flat_direct_terrain_texture;
    }
    if (g_flat_direct_terrain_texture) rb->destroy_texture(g_flat_direct_terrain_texture);
    PexTextureDesc td;
    memset(&td, 0, sizeof(td));
    td.width = tex_terrain.w;
    td.height = tex_terrain.h;
    td.rgba_pixels = (const uint32_t*)tex_terrain.rgba;
    td.repeat = 1;
    td.nearest = 1;
    g_flat_direct_terrain_texture = rb->create_texture(&td);
    g_flat_direct_terrain_rgba = tex_terrain.rgba;
    g_flat_direct_terrain_w = tex_terrain.w;
    g_flat_direct_terrain_h = tex_terrain.h;
    return g_flat_direct_terrain_texture;
}

static void flat_direct_upload_builder(int sy, int cz, int cx, int pass, FlatDirectMeshBuilder *b) {
    PexRendererBackend *rb = flat_direct_backend();
    unsigned int *slot = &g_flat_section_direct_mesh[sy][cz][cx][pass];
    if (!rb || !b || b->vcount == 0 || b->icount == 0) {
        if (rb && *slot) { rb->destroy_mesh(*slot); *slot = 0; }
        return;
    }
    flat_direct_resolve_builder_lightmap(b);
    PexMesh mesh;
    memset(&mesh, 0, sizeof(mesh));
    mesh.vertices = b->v;
    mesh.indices = b->i;
    mesh.vertex_count = b->vcount;
    mesh.index_count = b->icount;
    /* Terrain sections update repeatedly while streaming.  Mark them dynamic so
       the native D3D backends can reuse buffers and map/update them instead of
       destroying and recreating GPU resources on every rebuild. */
    mesh.dynamic = 1;
    if (*slot) rb->update_mesh(*slot, &mesh);
    else *slot = rb->upload_mesh(&mesh);
}

static void flat_direct_make_state(PexRenderState *st, int pass) {
    memset(st, 0, sizeof(*st));
    st->texture = flat_direct_terrain_handle();
    st->texture_enabled = st->texture ? 1 : 0;
    st->blend_enabled = (pass == 1) ? 1 : 0;
    st->alpha_test_enabled = (pass == 0) ? 1 : 0;
    st->depth_enabled = 1;
    st->depth_write = (pass == 0) ? 1 : 0;
    st->fog_enabled = g_d3d9.fog_enabled;
    st->fog_start = g_d3d9.fog_start;
    st->fog_end = g_d3d9.fog_end;
    st->fog_color = g_d3d9.fog_color;
#if defined(PEX_PLATFORM_WII)
    glGetFloatv(GL_MODELVIEW_MATRIX, st->modelview);
    glGetFloatv(GL_PROJECTION_MATRIX, st->projection);
#else
    memcpy(st->modelview, g_d3d9.modelview.m, sizeof(st->modelview));
    memcpy(st->projection, g_d3d9.projection.m, sizeof(st->projection));
#endif
}

static void world_set_shade(float shade) {
    if (g_force_fullbright_item_model) {
        flat_direct_set_color_light4f(shade, shade, shade, 1.0f, flat_fullbright_packed_light());
        return;
    }
    flat_direct_set_color_light4f(shade, shade, shade, 1.0f,
                                  (uint32_t)flat_pack_sky_block_light(g_world_light_x, g_world_light_y, g_world_light_z, 0));
}

static void world_set_color_shade(int rgb, float shade) {
    float r = ((rgb >> 16) & 255) / 255.0f;
    float g = ((rgb >> 8) & 255) / 255.0f;
    float b = (rgb & 255) / 255.0f;
    uint32_t packed = flat_fullbright_packed_light();
    if (!g_force_fullbright_item_model) {
        packed = (uint32_t)flat_pack_sky_block_light(g_world_light_x, g_world_light_y, g_world_light_z, 0);
    }
    flat_direct_set_color_light4f(r * shade, g * shade, b * shade, 1.0f, packed);
}

static void world_tex_vertex(float x, float y, float z, float u, float v) {
    if (g_flat_direct_builder) { flat_direct_vertex(x, y, z, u, v); return; }
    glTexCoord2f(u, v);
    glVertex3f(x, y, z);
}


static int dig_particle_tile_for_block_face(int id, int face) {
    /* Particles use the same terrain tile families as the block renderer.  The
       previous fallback returned dirt for most IDs, so sand/gravel/wood/etc.
       broke with dirt particles. */
    if (id == BLOCK_GRASS) {
        /* This terrain atlas stores the grass-top tile as a grayscale mask;
           without biome tint it looks like stone in dig particles.  Use the
           already-green grass side for top-hit particles too. */
        if (face == 1) return 3;
        if (face == 0) return 2;
        return 3;
    }
    if (id == BLOCK_WATER || id == BLOCK_STILL_WATER) return liquid_top_tile_for_block(id);
    if (id == BLOCK_LAVA || id == BLOCK_STILL_LAVA) return liquid_top_tile_for_block(id);
    if (id == BLOCK_STONE) return 1;
    if (id == BLOCK_DIRT) return 2;
    if (id == BLOCK_COBBLESTONE) return 16;
    if (id == BLOCK_BEDROCK) return 17;
    if (id == BLOCK_PLANKS) return 4;
    if (id == BLOCK_SAND) return 18;
    if (id == BLOCK_GRAVEL) return 19;
    if (id == BLOCK_LOG) return (face == 0 || face == 1) ? 21 : 20;
    if (id == BLOCK_LEAVES) return flat_fancy_leaf_texture_enabled() ? 52 : 53;
    if (id == BLOCK_GLASS) return 49;
    if (id == BLOCK_GOLD_ORE) return 32;
    if (id == BLOCK_IRON_ORE) return 33;
    if (id == BLOCK_COAL_ORE) return 34;
    if (id == BLOCK_BOOKSHELF) return (face == 0 || face == 1) ? 4 : 35;
    if (id == BLOCK_MOSSY_COBBLESTONE) return 36;
    if (id == BLOCK_OBSIDIAN) return 37;
    if (id == BLOCK_GOLD_BLOCK) return 39;
    if (id == BLOCK_IRON_BLOCK) return 38;
    if (id == BLOCK_DIAMOND_BLOCK) return 40;
    if (id == BLOCK_DOUBLE_SLAB || id == BLOCK_SLAB) return (face == 0 || face == 1) ? 6 : 5;
    if (id == BLOCK_BRICK) return 7;
    if (id == BLOCK_TNT) return (face == 0 ? 10 : (face == 1 ? 9 : 8));
    if (id == BLOCK_MOB_SPAWNER) return 65;
    if (id == BLOCK_WOOD_STAIRS || id == BLOCK_WOOD_PRESSURE_PLATE || id == BLOCK_FENCE) return 4;
    if (id == BLOCK_COBBLE_STAIRS) return 16;
    if (id == BLOCK_CRAFTING_TABLE) {
        if (face == 1) return 43;
        if (face == 0) return 4;
        if (face == 2 || face == 3) return 59;
        return 60;
    }
    if (id == BLOCK_CHEST) {
        if (face == 1 || face == 0) return chest_top_tile();
        if (face == 3) return chest_front_tile();
        return chest_side_tile();
    }
    if (id == BLOCK_DIAMOND_ORE) return 50;
    if (id == BLOCK_REDSTONE_ORE || id == BLOCK_REDSTONE_ORE_GLOWING) return 51;
    if (id == BLOCK_LAPIS_ORE) return 38;
    if (id == BLOCK_LAPIS_BLOCK) return 39;
    if (id == BLOCK_FURNACE || id == BLOCK_FURNACE_LIT) {
        if (face == 1 || face == 0) return furnace_top_tile();
        if (face == 3) return id == BLOCK_FURNACE_LIT ? furnace_front_lit_tile() : furnace_front_tile();
        return furnace_side_tile();
    }
    if (id == BLOCK_SIGN_POST || id == BLOCK_WALL_SIGN) return 4;
    if (id == BLOCK_WOOD_DOOR) return 97;
    if (id == BLOCK_LADDER) return 83;
    if (id == BLOCK_RAILS) return 128;
    if (id == BLOCK_LEVER) return 96;
    if (id == BLOCK_STONE_PRESSURE_PLATE || id == BLOCK_STONE_BUTTON) return 1;
    if (id == BLOCK_IRON_DOOR) return 98;
    if (id == BLOCK_REDSTONE_WIRE) return 84;
    if (id == BLOCK_REDSTONE_TORCH_OFF) return 115;
    if (id == BLOCK_REDSTONE_TORCH_ON) return 99;
    if (id == BLOCK_YELLOW_FLOWER) return 13;
    if (id == BLOCK_RED_ROSE) return 12;
    if (id == BLOCK_BROWN_MUSHROOM) return 29;
    if (id == BLOCK_RED_MUSHROOM) return 28;
    if (id == BLOCK_WOOL) return 64;
    if (id == BLOCK_SNOW_LAYER || id == BLOCK_SNOW_BLOCK) return 66;
    if (id == BLOCK_ICE) return 67;
    if (id == BLOCK_FARMLAND) return face == 1 ? 87 : 2;
    if (id == BLOCK_CACTUS) return (face == 1 ? 69 : (face == 0 ? 71 : 70));
    if (id == BLOCK_CLAY) return 72;
    if (id == BLOCK_REEDS) return 73;
    if (id == BLOCK_JUKEBOX) return (face == 1 ? 75 : 74);
    if (id == BLOCK_PUMPKIN || id == BLOCK_JACK_O_LANTERN) return (face == 1 || face == 0) ? 102 : (face == 3 ? 119 : 118);
    if (id == BLOCK_NETHERRACK) return 103;
    if (id == BLOCK_SOUL_SAND) return 104;
    if (id == BLOCK_GLOWSTONE) return 105;
    if (id == BLOCK_PORTAL) return 14;
    return 2;
}

static int dig_particle_tile_for_block(int id) {
    /* Destroy particles in Java use the block's base texture.  Use a stable,
       visible face for special blocks but keep sand/gravel/etc. on their actual
       atlas tiles instead of falling through to dirt. */
    if (id == BLOCK_GRASS) return 3;
    if (id == BLOCK_LOG || id == BLOCK_CACTUS || id == BLOCK_PUMPKIN || id == BLOCK_JACK_O_LANTERN || id == BLOCK_FURNACE || id == BLOCK_FURNACE_LIT || id == BLOCK_CRAFTING_TABLE || id == BLOCK_CHEST || id == BLOCK_TNT || id == BLOCK_FARMLAND || id == BLOCK_JUKEBOX) {
        return dig_particle_tile_for_block_face(id, 3);
    }
    return dig_particle_tile_for_block_face(id, 1);
}

static void dig_particle_color_for_block(int id, int wx, int wz, float *r, float *g, float *b) {
    /* EntityDiggingFX darkens dig particles, then some blocks (notably foliage)
       multiply that by the block color.  Leaves should use the same biome
       foliage tint as the rendered block instead of the gray placeholder. */
    *r = 0.6f; *g = 0.6f; *b = 0.6f;
    if (id == BLOCK_LEAVES) {
        int col = java_foliage_color_at(wx, wz);
        *r *= (float)((col >> 16) & 255) / 255.0f;
        *g *= (float)((col >> 8) & 255) / 255.0f;
        *b *= (float)(col & 255) / 255.0f;
    }
}

static float frand01(void) {
    return (float)rand() / (float)RAND_MAX;
}

static void add_dig_particle(float x, float y, float z, float mx, float my, float mz,
                             int tile, int block_id, int tint_x, int tint_z,
                             float motion_scale, float size_scale) {
    DigParticle *p = &g_dig_particles[g_next_dig_particle++ % MAX_DIG_PARTICLES];
    memset(p, 0, sizeof(*p));
    p->active = 1;
    p->kind = PARTICLE_DIG;
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
    dig_particle_color_for_block(block_id, tint_x, tint_z, &p->r, &p->g, &p->b);
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
                                 tile, block_id, bx, bz, 1.0f, 1.0f);
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
                     dig_particle_tile_for_block_face(block_id, face), block_id, bx, bz,
                     0.2f, 0.6f);
}


static void add_bubble_particle(float x, float y, float z, float mx, float my, float mz) {
    DigParticle *p = &g_dig_particles[g_next_dig_particle++ % MAX_DIG_PARTICLES];
    memset(p, 0, sizeof(*p));
    p->active = 1;
    p->kind = PARTICLE_BUBBLE;
    p->tile = 32;
    p->x = p->prev_x = x;
    p->y = p->prev_y = y;
    p->z = p->prev_z = z;
    p->r = p->g = p->b = 1.0f;
    p->scale = ((frand01() * 0.5f + 0.5f) * 2.0f) * (frand01() * 0.6f + 0.2f);
    p->mx = mx * 0.2f + (frand01() * 2.0f - 1.0f) * 0.02f;
    p->my = my * 0.2f + (frand01() * 2.0f - 1.0f) * 0.02f;
    p->mz = mz * 0.2f + (frand01() * 2.0f - 1.0f) * 0.02f;
    p->gravity = -0.05f; /* bubble onUpdate adds +0.002 Y, so negative gravity in the shared updater. */
    p->max_age = (int)(8.0f / (frand01() * 0.8f + 0.2f));
    if (p->max_age < 1) p->max_age = 1;
}

static void add_splash_particle(float x, float y, float z, float mx, float my, float mz) {
    DigParticle *p = &g_dig_particles[g_next_dig_particle++ % MAX_DIG_PARTICLES];
    memset(p, 0, sizeof(*p));
    p->active = 1;
    p->kind = PARTICLE_SPLASH;
    p->tile = 20 + (rand() & 3); /* EntityRainFX 19..22 then EntitySplashFX increments. */
    p->x = p->prev_x = x;
    p->y = p->prev_y = y;
    p->z = p->prev_z = z;
    p->r = p->g = p->b = 1.0f;
    p->scale = (frand01() * 0.5f + 0.5f) * 2.0f;
    p->mx = (frand01() * 2.0f - 1.0f) * 0.4f * 0.3f;
    p->my = frand01() * 0.2f + 0.1f;
    p->mz = (frand01() * 2.0f - 1.0f) * 0.4f * 0.3f;
    if (my == 0.0f && (mx != 0.0f || mz != 0.0f)) {
        p->mx = mx;
        p->my = my + 0.1f;
        p->mz = mz;
    }
    p->gravity = 1.0f;
    p->max_age = (int)(8.0f / (frand01() * 0.8f + 0.2f));
    if (p->max_age < 1) p->max_age = 1;
}

static void add_portal_particle(float x, float y, float z, float mx, float my, float mz) {
    DigParticle *p = &g_dig_particles[g_next_dig_particle++ % MAX_DIG_PARTICLES];
    memset(p, 0, sizeof(*p));
    p->active = 1;
    p->kind = PARTICLE_PORTAL;
    p->tile = rand() & 7;
    p->x = p->prev_x = p->ox = x;
    p->y = p->prev_y = p->oy = y;
    p->z = p->prev_z = p->oz = z;
    p->mx = mx;
    p->my = my;
    p->mz = mz;
    float c = frand01() * 0.6f + 0.4f;
    p->r = c * 0.9f;
    p->g = c * 0.3f;
    p->b = c;
    p->scale = frand01() * 0.2f + 0.5f;
    p->gravity = 0.0f;
    p->max_age = (int)(frand01() * 10.0f) + 40;
    p->age = 0;
}

static void spawn_portal_block_particles(int bx, int by, int bz) {
    for (int i = 0; i < 4; ++i) {
        float x = (float)bx + frand01();
        float y = (float)by + frand01();
        float z = (float)bz + frand01();
        float mx = (frand01() - 0.5f) * 0.5f;
        float my = (frand01() - 0.5f) * 0.5f;
        float mz = (frand01() - 0.5f) * 0.5f;
        int side = (rand() & 1) * 2 - 1;
        int xneg_portal = flat_in_bounds(bx - 1, by, bz) && flat_get_block(bx - 1, by, bz) == BLOCK_PORTAL;
        int xpos_portal = flat_in_bounds(bx + 1, by, bz) && flat_get_block(bx + 1, by, bz) == BLOCK_PORTAL;
        if (!xneg_portal && !xpos_portal) {
            x = (float)bx + 0.5f + 0.25f * (float)side;
            mx = frand01() * 2.0f * (float)side;
        } else {
            z = (float)bz + 0.5f + 0.25f * (float)side;
            mz = frand01() * 2.0f * (float)side;
        }
        add_portal_particle(x, y, z, mx, my, mz);
    }
}

static void update_portal_ambient_effects(void) {
    if (g_mp_connected || g_player_dead) return;
    int pcx = (int)floorf(g_player_x);
    int pcy = (int)floorf(g_player_y - 0.6f);
    int pcz = (int)floorf(g_player_z);
    for (int attempt = 0; attempt < 18; ++attempt) {
        int x = pcx + (rand() % 33) - 16;
        int y = pcy + (rand() % 17) - 8;
        int z = pcz + (rand() % 33) - 16;
        if (!flat_in_bounds(x, y, z) || flat_get_block(x, y, z) != BLOCK_PORTAL) continue;
        if ((rand() % 100) == 0) {
            pex_sound_play_at("portal.portal", (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, 0.5f, 0.8f + frand01() * 0.4f);
        }
        spawn_portal_block_particles(x, y, z);
        break;
    }
}

static void spawn_water_entry_particles(float x, float y, float z, float mx, float mz) {
    float width = 0.6f; /* player width in Entity.java */
    int n = (int)(1.0f + width * 20.0f);
    float floor_y = floorf(y);
    for (int i = 0; i < n; ++i) {
        float ox = (frand01() * 2.0f - 1.0f) * width;
        float oz = (frand01() * 2.0f - 1.0f) * width;
        add_bubble_particle(x + ox, floor_y + 1.0f, z + oz, mx, -frand01() * 0.2f, mz);
    }
    for (int i = 0; i < n; ++i) {
        float ox = (frand01() * 2.0f - 1.0f) * width;
        float oz = (frand01() * 2.0f - 1.0f) * width;
        add_splash_particle(x + ox, floor_y + 1.0f, z + oz, mx, 0.0f, mz);
    }
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

        if (p->kind == PARTICLE_BUBBLE) {
            p->my += 0.002f;
            p->x += p->mx;
            p->y += p->my;
            p->z += p->mz;
            p->mx *= 0.85f;
            p->my *= 0.85f;
            p->mz *= 0.85f;
            if (!block_is_water(flat_get_block((int)floorf(p->x), (int)floorf(p->y), (int)floorf(p->z)))) p->active = 0;
        } else if (p->kind == PARTICLE_PORTAL) {
            float t = (float)p->age / (float)(p->max_age > 0 ? p->max_age : 1);
            float curve = -t + t * t * 2.0f;
            curve = 1.0f - curve;
            p->x = p->ox + p->mx * curve;
            p->y = p->oy + p->my * curve + (1.0f - t);
            p->z = p->oz + p->mz * curve;
        } else {
            p->my -= (p->kind == PARTICLE_SPLASH ? 0.06f : 0.04f) * p->gravity;
            p->x += p->mx;
            p->y += p->my;
            p->z += p->mz;

            if (p->y < 0.02f) {
                p->y = 0.02f;
                if (p->kind == PARTICLE_SPLASH && frand01() < 0.5f) p->active = 0;
                p->my = 0.0f;
                p->mx *= 0.7f;
                p->mz *= 0.7f;
            }

            p->mx *= 0.98f;
            p->my *= 0.98f;
            p->mz *= 0.98f;
        }
    }
}

static void draw_dig_particles(float partial) {
    if (!tex_terrain.id) return;

    const PexPlayerRenderState *pr = &g_player_render_frame;
    float yaw = lerp_angle(pr->prev_yaw, pr->yaw, partial) * (float)M_PI / 180.0f;
    float pitch = (pr->prev_pitch + (pr->pitch - pr->prev_pitch) * partial) * (float)M_PI / 180.0f;
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
        if (!p->active || p->kind != PARTICLE_DIG) continue;

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
        float br = flat_light_brightness((int)floorf(px), (int)floorf(py), (int)floorf(pz));

        glColor4f(p->r * br, p->g * br, p->b * br, 1.0f);
        world_tex_vertex(px - cos_yaw * q - x_rot * q, py - yz_rot * q, pz - sin_yaw * q - z_rot * q, su0, sv1);
        world_tex_vertex(px - cos_yaw * q + x_rot * q, py + yz_rot * q, pz - sin_yaw * q + z_rot * q, su0, sv0);
        world_tex_vertex(px + cos_yaw * q + x_rot * q, py + yz_rot * q, pz + sin_yaw * q + z_rot * q, su1, sv0);
        world_tex_vertex(px + cos_yaw * q - x_rot * q, py - yz_rot * q, pz + sin_yaw * q - z_rot * q, su1, sv1);
    }
    glEnd();

    if (tex_particles.id) {
        glBindTexture(GL_TEXTURE_2D, tex_particles.id);
        glBegin(GL_QUADS);
        for (int i = 0; i < MAX_DIG_PARTICLES; i++) {
            DigParticle *p = &g_dig_particles[i];
            if (!p->active || p->kind == PARTICLE_DIG) continue;
            float u0 = (float)(p->tile % 16) / 16.0f;
            float u1 = u0 + 0.999f / 16.0f;
            float v0 = (float)(p->tile / 16) / 16.0f;
            float v1 = v0 + 0.999f / 16.0f;
            float px = p->prev_x + (p->x - p->prev_x) * partial;
            float py = p->prev_y + (p->y - p->prev_y) * partial;
            float pz = p->prev_z + (p->z - p->prev_z) * partial;
            float q = 0.1f * p->scale;
            float br = flat_light_brightness((int)floorf(px), (int)floorf(py), (int)floorf(pz));
            if (p->kind == PARTICLE_PORTAL) {
                float life = ((float)p->age + partial) / (float)(p->max_age > 0 ? p->max_age : 1);
                if (life < 0.0f) life = 0.0f;
                if (life > 1.0f) life = 1.0f;
                float grow = 1.0f - life;
                grow = 1.0f - grow * grow;
                q = 0.1f * p->scale * grow;
                br = 1.0f;
            }
            glColor4f(p->r * br, p->g * br, p->b * br, 1.0f);
            world_tex_vertex(px - cos_yaw * q - x_rot * q, py - yz_rot * q, pz - sin_yaw * q - z_rot * q, u0, v1);
            world_tex_vertex(px - cos_yaw * q + x_rot * q, py + yz_rot * q, pz - sin_yaw * q + z_rot * q, u0, v0);
            world_tex_vertex(px + cos_yaw * q + x_rot * q, py + yz_rot * q, pz + sin_yaw * q + z_rot * q, u1, v0);
            world_tex_vertex(px + cos_yaw * q - x_rot * q, py - yz_rot * q, pz + sin_yaw * q - z_rot * q, u1, v1);
        }
        glEnd();
    }

    glColor4f(1, 1, 1, 1);
    glDisable(GL_ALPHA_TEST);
}


static void draw_grass_block(float x, float y, float z) {
    float x0 = x, x1 = x + 1.0f;
    float y0 = y, y1 = y + 1.0f;
    float z0 = z, z1 = z + 1.0f;
    float u0,v0,u1,v1;
    int bx = (int)floorf(x), by = (int)floorf(y), bz = (int)floorf(z);
    world_style_set_pos(bx, by, bz);

    /* Beta terrain.png indices: 0=grass top, 2=dirt, 3=grass side. */
    terrain_tile_uv(0, &u0, &v0, &u1, &v1);
    world_light_set_pos_for_face(bx, by, bz, 1);
    world_set_color_shade(java_grass_color_at(bx, bz), 1.0f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z0, u0, v0);
    world_tex_vertex(x1, y1, z0, u1, v0);
    world_tex_vertex(x1, y1, z1, u1, v1);
    world_tex_vertex(x0, y1, z1, u0, v1);
    glEnd();

    terrain_tile_uv(3, &u0, &v0, &u1, &v1);
    world_light_set_pos_for_face(bx, by, bz, 2);
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

    world_light_set_pos_for_face(bx, by, bz, 4);
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
    world_light_set_pos_for_face(bx, by, bz, 0);
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

    terrain_tile_uv(chest_top_tile(), &u0, &v0, &u1, &v1);
    world_set_shade(1.0f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y1, z1, u1, v1); world_tex_vertex(x0, y1, z1, u0, v1);
    glEnd();

    terrain_tile_uv(chest_top_tile(), &u0, &v0, &u1, &v1);
    world_set_shade(0.50f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y0, z1, u0, v0); world_tex_vertex(x1, y0, z1, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    glEnd();

    terrain_tile_uv(chest_front_tile(), &u0, &v0, &u1, &v1);
    world_set_shade(0.80f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z1, u0, v0); world_tex_vertex(x1, y1, z1, u1, v0); world_tex_vertex(x1, y0, z1, u1, v1); world_tex_vertex(x0, y0, z1, u0, v1);
    glEnd();

    terrain_tile_uv(chest_side_tile(), &u0, &v0, &u1, &v1);
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

static void draw_furnace_block_id(int id, float x, float y, float z) {
    /* User-supplied terrain listing, 1-based X:Y:
       front 13:3 => tile 44, sides 14:3 => tile 45,
       top/bottom 15:3 => tile 46. */
    float x0 = x, x1 = x + 1.0f;
    float y0 = y, y1 = y + 1.0f;
    float z0 = z, z1 = z + 1.0f;
    float u0, v0, u1, v1;

    terrain_tile_uv(furnace_top_tile(), &u0, &v0, &u1, &v1);
    world_set_shade(1.0f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y1, z1, u1, v1); world_tex_vertex(x0, y1, z1, u0, v1);
    glEnd();

    terrain_tile_uv(furnace_top_tile(), &u0, &v0, &u1, &v1);
    world_set_shade(0.50f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y0, z1, u0, v0); world_tex_vertex(x1, y0, z1, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    glEnd();

    terrain_tile_uv(id == BLOCK_FURNACE_LIT ? furnace_front_lit_tile() : furnace_front_tile(), &u0, &v0, &u1, &v1);
    world_set_shade(0.80f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z1, u0, v0); world_tex_vertex(x1, y1, z1, u1, v0); world_tex_vertex(x1, y0, z1, u1, v1); world_tex_vertex(x0, y0, z1, u0, v1);
    glEnd();

    terrain_tile_uv(furnace_side_tile(), &u0, &v0, &u1, &v1);
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
static int block_uses_cross_plant_model(int id);
static void draw_cross_plant_block(int id, float x, float y, float z);
static void draw_snow_layer_block(float x, float y, float z);
static int block_uses_special_model(int id);
static void draw_special_block_model(int id, int x, int y, int z);

static void draw_world_block_id(int id, float x, float y, float z) {
    if (id == BLOCK_GRASS) draw_grass_block(x, y, z);
    else if (id == BLOCK_STONE) draw_textured_cube_tile(x, y, z, 1);
    else if (id == BLOCK_DIRT) draw_dirt_block(x, y, z);
    else if (id == BLOCK_COBBLESTONE) draw_textured_cube_tile(x, y, z, 16);
    else if (id == BLOCK_BEDROCK) draw_bedrock_block(x, y, z);
    else if (id == BLOCK_PLANKS) draw_planks_block(x, y, z);
    else if (id == BLOCK_LOG) draw_log_block(x, y, z);
    else if (id == BLOCK_CRAFTING_TABLE) draw_crafting_table_block(x, y, z);
    else if (id == BLOCK_FURNACE || id == BLOCK_FURNACE_LIT) draw_furnace_block_id(id, x, y, z);
    else if (id == BLOCK_CHEST) draw_chest_block(x, y, z);
    else if (id == BLOCK_SNOW_LAYER) draw_snow_layer_block(x, y, z);
    else if (block_uses_cross_plant_model(id)) draw_cross_plant_block(id, x, y, z);
    else if (id >= 1 && id <= 91) draw_generic_world_block_model(id, x, y, z);
}

typedef struct BlockBounds {
    float x0, y0, z0, x1, y1, z1;
} BlockBounds;

static BlockBounds block_bounds_for_selection(int id, int x, int y, int z) {
    BlockBounds b = {(float)x, (float)y, (float)z, (float)x + 1.0f, (float)y + 1.0f, (float)z + 1.0f};
    int meta = flat_get_meta(x, y, z);
    if (id == BLOCK_SNOW_LAYER) b.y1 = (float)y + 2.0f / 16.0f;
    else if (id == BLOCK_SLAB) b.y1 = (float)y + 0.5f;
    else if (id == BLOCK_STONE_PRESSURE_PLATE || id == BLOCK_WOOD_PRESSURE_PLATE) {
        float e = 1.0f / 16.0f;
        b.x0 = (float)x + e; b.x1 = (float)x + 1.0f - e;
        b.z0 = (float)z + e; b.z1 = (float)z + 1.0f - e;
        b.y1 = (float)y + ((meta & 1) ? (1.0f / 32.0f) : (1.0f / 16.0f));
    } else if (id == BLOCK_RAILS || id == BLOCK_REDSTONE_WIRE) {
        b.y1 = (float)y + 0.10f;
    } else if (id == BLOCK_CACTUS) {
        b.x0 = (float)x + 0.0625f; b.x1 = (float)x + 0.9375f;
        b.z0 = (float)z + 0.0625f; b.z1 = (float)z + 0.9375f;
    } else if (id == BLOCK_CHEST) {
        b.x0 = (float)x + 0.0625f; b.x1 = (float)x + 0.9375f;
        b.z0 = (float)z + 0.0625f; b.z1 = (float)z + 0.9375f;
        b.y1 = (float)y + 14.0f / 16.0f;
    } else if (block_is_door_id(id)) {
        int ly = door_meta_is_upper(meta) ? (y - 1) : y;
        int lower_meta = flat_get_meta(x, ly, z);
        int state = ((lower_meta & 4) == 0) ? ((lower_meta - 1) & 3) : (lower_meta & 3);
        float t = 3.0f / 16.0f;
        if (state == 0) b.z1 = (float)z + t;
        else if (state == 1) b.x0 = (float)x + 1.0f - t;
        else if (state == 2) b.z0 = (float)z + 1.0f - t;
        else b.x1 = (float)x + t;
    } else if (id == BLOCK_TORCH || id == BLOCK_REDSTONE_TORCH_OFF || id == BLOCK_REDSTONE_TORCH_ON) {
        int side = meta & 7;
        float r = 0.15f;
        if (side == 1) { b.x0 = (float)x; b.x1 = (float)x + r * 2.0f; b.y0 = (float)y + 0.2f; b.y1 = (float)y + 0.8f; b.z0 = (float)z + 0.5f - r; b.z1 = (float)z + 0.5f + r; }
        else if (side == 2) { b.x0 = (float)x + 1.0f - r * 2.0f; b.x1 = (float)x + 1.0f; b.y0 = (float)y + 0.2f; b.y1 = (float)y + 0.8f; b.z0 = (float)z + 0.5f - r; b.z1 = (float)z + 0.5f + r; }
        else if (side == 3) { b.x0 = (float)x + 0.5f - r; b.x1 = (float)x + 0.5f + r; b.y0 = (float)y + 0.2f; b.y1 = (float)y + 0.8f; b.z0 = (float)z; b.z1 = (float)z + r * 2.0f; }
        else if (side == 4) { b.x0 = (float)x + 0.5f - r; b.x1 = (float)x + 0.5f + r; b.y0 = (float)y + 0.2f; b.y1 = (float)y + 0.8f; b.z0 = (float)z + 1.0f - r * 2.0f; b.z1 = (float)z + 1.0f; }
        else { r = 0.1f; b.x0 = (float)x + 0.5f - r; b.x1 = (float)x + 0.5f + r; b.y1 = (float)y + 0.6f; b.z0 = (float)z + 0.5f - r; b.z1 = (float)z + 0.5f + r; }
    } else if (id == BLOCK_STONE_BUTTON) {
        int side = meta & 7;
        float t = (meta & 8) ? (1.0f / 16.0f) : (2.0f / 16.0f);
        b.y0 = (float)y + 6.0f / 16.0f; b.y1 = (float)y + 10.0f / 16.0f;
        if (side == 1) { b.x0 = (float)x; b.x1 = (float)x + t; b.z0 = (float)z + 0.5f - 3.0f/16.0f; b.z1 = (float)z + 0.5f + 3.0f/16.0f; }
        else if (side == 2) { b.x0 = (float)x + 1.0f - t; b.x1 = (float)x + 1.0f; b.z0 = (float)z + 0.5f - 3.0f/16.0f; b.z1 = (float)z + 0.5f + 3.0f/16.0f; }
        else if (side == 3) { b.x0 = (float)x + 0.5f - 3.0f/16.0f; b.x1 = (float)x + 0.5f + 3.0f/16.0f; b.z0 = (float)z; b.z1 = (float)z + t; }
        else { b.x0 = (float)x + 0.5f - 3.0f/16.0f; b.x1 = (float)x + 0.5f + 3.0f/16.0f; b.z0 = (float)z + 1.0f - t; b.z1 = (float)z + 1.0f; }
    } else if (id == BLOCK_LADDER || id == BLOCK_WALL_SIGN) {
        int side = meta;
        float e = (id == BLOCK_LADDER) ? (2.0f / 16.0f) : 0.10f;
        if (side == 2) { b.z0 = (float)z + 1.0f - e; }
        else if (side == 3) { b.z1 = (float)z + e; }
        else if (side == 4) { b.x0 = (float)x + 1.0f - e; }
        else if (side == 5) { b.x1 = (float)x + e; }
    } else if (id == BLOCK_LEVER) {
        int side = meta & 7;
        float w = 3.0f / 16.0f;
        float l = 0.25f;
        float h = 3.0f / 16.0f;
        if (side == 5) { b.x0 = (float)x + 0.5f - w; b.x1 = (float)x + 0.5f + w; b.y1 = (float)y + h; b.z0 = (float)z + 0.5f - l; b.z1 = (float)z + 0.5f + l; }
        else if (side == 6) { b.x0 = (float)x + 0.5f - l; b.x1 = (float)x + 0.5f + l; b.y1 = (float)y + h; b.z0 = (float)z + 0.5f - w; b.z1 = (float)z + 0.5f + w; }
        else if (side == 4) { b.x0 = (float)x + 0.5f - w; b.x1 = (float)x + 0.5f + w; b.y0 = (float)y + 0.5f - l; b.y1 = (float)y + 0.5f + l; b.z0 = (float)z + 1.0f - h; }
        else if (side == 3) { b.x0 = (float)x + 0.5f - w; b.x1 = (float)x + 0.5f + w; b.y0 = (float)y + 0.5f - l; b.y1 = (float)y + 0.5f + l; b.z1 = (float)z + h; }
        else if (side == 2) { b.x0 = (float)x + 1.0f - h; b.y0 = (float)y + 0.5f - l; b.y1 = (float)y + 0.5f + l; b.z0 = (float)z + 0.5f - w; b.z1 = (float)z + 0.5f + w; }
        else { b.x1 = (float)x + h; b.y0 = (float)y + 0.5f - l; b.y1 = (float)y + 0.5f + l; b.z0 = (float)z + 0.5f - w; b.z1 = (float)z + 0.5f + w; }
    } else if (id == BLOCK_SIGN_POST) {
        b.x0 = (float)x + 0.25f; b.x1 = (float)x + 0.75f;
        b.z0 = (float)z + 0.4375f; b.z1 = (float)z + 0.5625f;
    }
    return b;
}

static void emit_overlay_bounds_textured(BlockBounds b, int stage) {
    if (stage < 0) return;
    if (stage > 9) stage = 9;
    float u0,v0,u1,v1;
    terrain_tile_uv(240 + stage, &u0, &v0, &u1, &v1);
    glBegin(GL_QUADS);
    world_tex_vertex(b.x0, b.y1, b.z0, u0, v0); world_tex_vertex(b.x1, b.y1, b.z0, u1, v0); world_tex_vertex(b.x1, b.y1, b.z1, u1, v1); world_tex_vertex(b.x0, b.y1, b.z1, u0, v1);
    world_tex_vertex(b.x1, b.y1, b.z0, u0, v0); world_tex_vertex(b.x0, b.y1, b.z0, u1, v0); world_tex_vertex(b.x0, b.y0, b.z0, u1, v1); world_tex_vertex(b.x1, b.y0, b.z0, u0, v1);
    world_tex_vertex(b.x0, b.y1, b.z1, u0, v0); world_tex_vertex(b.x1, b.y1, b.z1, u1, v0); world_tex_vertex(b.x1, b.y0, b.z1, u1, v1); world_tex_vertex(b.x0, b.y0, b.z1, u0, v1);
    world_tex_vertex(b.x0, b.y1, b.z0, u0, v0); world_tex_vertex(b.x0, b.y1, b.z1, u1, v0); world_tex_vertex(b.x0, b.y0, b.z1, u1, v1); world_tex_vertex(b.x0, b.y0, b.z0, u0, v1);
    world_tex_vertex(b.x1, b.y1, b.z1, u0, v0); world_tex_vertex(b.x1, b.y1, b.z0, u1, v0); world_tex_vertex(b.x1, b.y0, b.z0, u1, v1); world_tex_vertex(b.x1, b.y0, b.z1, u0, v1);
    world_tex_vertex(b.x0, b.y0, b.z1, u0, v0); world_tex_vertex(b.x1, b.y0, b.z1, u1, v0); world_tex_vertex(b.x1, b.y0, b.z0, u1, v1); world_tex_vertex(b.x0, b.y0, b.z0, u0, v1);
    glEnd();
}

static void draw_break_overlay_cube(float x, float y, float z, int stage) {
    if (stage < 0) return;
    int bx = (int)floorf(x + 0.01f), by = (int)floorf(y + 0.01f), bz = (int)floorf(z + 0.01f);
    int id = flat_get_block(bx, by, bz);
    BlockBounds b = block_bounds_for_selection(id, bx, by, bz);

    float e = 0.002f;
    b.x0 -= e; b.y0 -= e; b.z0 -= e; b.x1 += e; b.y1 += e; b.z1 += e;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glEnable(GL_BLEND);
#if defined(PEX_PLATFORM_PSP)
    /* PSP terrain destroy-stage tiles need alpha testing.  Without it, the
       transparent gray parts of destroy_stage show as a flat gray cube. */
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.15f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1,1,1,1.0f);
#else
    glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
    glColor4f(1,1,1,1);
#endif
    glDepthMask(GL_FALSE);
    emit_overlay_bounds_textured(b, stage);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


static void draw_java_entity_shadow(float x, float y, float z, float shadow_size, float shadow_alpha);

static void draw_falling_blocks(float partial) {
    if (!tex_terrain.id) return;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glColor4f(1, 1, 1, 1);
    for (int i = 0; i < MAX_FALLING_BLOCK_ENTITIES; i++) {
        FlatFallingBlock *fb = &g_falling_blocks[i];
        if (!fb->active || fb->block_id <= 0) continue;
        float x, y, z;
        if (g_mp_connected && fb->path_duration > 0) {
            double elapsed = now_seconds() - fb->path_start_time;
            if (elapsed < 0.0) elapsed = 0.0;
            float t = (float)(elapsed * 20.0 / (double)fb->path_duration);
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            x = fb->start_x + (fb->end_x - fb->start_x) * t;
            y = fb->start_y + (fb->end_y - fb->start_y) * t;
            z = fb->start_z + (fb->end_z - fb->start_z) * t;
        } else {
            x = fb->prev_x + (fb->x - fb->prev_x) * partial;
            y = fb->prev_y + (fb->y - fb->prev_y) * partial;
            z = fb->prev_z + (fb->z - fb->prev_z) * partial;
        }
        draw_java_entity_shadow(x, y, z, 0.50f, 1.0f);

        /* draw_java_entity_shadow binds tex_shadow.  Falling blocks need the
           terrain atlas rebound per entity, otherwise they sample the shadow
           texture and appear black until they land back in the terrain mesh. */
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
        glDisable(GL_BLEND);
        glColor4f(1, 1, 1, 1);

        glPushMatrix();
        glTranslatef(x, y, z);
        /* Falling block entities are not baked into the terrain light mesh yet.
           Render them like Java entity blocks: stable entity brightness, not the
           last sampled world face light, which made falling sand/gravel black. */
        g_force_fullbright_item_model++;
        world_style_set_pos((int)floorf(x), (int)floorf(y), (int)floorf(z));
        world_light_set_pos((int)floorf(x), (int)floorf(y), (int)floorf(z));
        draw_world_block_id(fb->block_id, -0.5f, -0.5f, -0.5f);
        g_force_fullbright_item_model--;
        glPopMatrix();
    }
}

static void draw_remote_break_overlays(void) {
    if (!g_mp_connected || !tex_terrain.id) return;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    for (int i = 0; i < PEX_NET_MAX_PLAYERS; i++) {
        PexNetRenderPlayerState *r = &g_mp_render_players[i];
        if (!r->active || r->skin_only || !r->mining_active || r->mining_time <= 0.0f) continue;
        if (flat_get_block(r->mining_x, r->mining_y, r->mining_z) == 0) continue;
        draw_break_overlay_cube((float)r->mining_x, (float)r->mining_y, (float)r->mining_z, r->mining_stage);
    }
}

static int java_shadow_block_top(int x, int y, int z, float *top_y, float *brightness) {
    for (int yy = y; yy >= y - 4 && yy >= FLAT_WORLD_Y_MIN; --yy) {
        int id = flat_get_block(x, yy, z);
        if (!id || block_is_liquid(id) || !flat_block_is_solid(id)) continue;
        *top_y = (float)yy + 1.002f;
        *brightness = flat_light_brightness(x, yy + 1, z);
        return 1;
    }
    return 0;
}

static void draw_java_entity_shadow(float x, float y, float z, float shadow_size, float shadow_alpha) {
    if (!g_opts.fancy_graphics || !tex_shadow.id || shadow_size <= 0.0f || shadow_alpha <= 0.0f) return;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_shadow.id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    /* Render.java/RenderManager shadow quads rely on texture alpha and normal
       alpha blending.  Real OpenGL still had the terrain/mob alpha-test state
       live here, so dim shadow pixels were clipped; D3D backends did not expose
       the same failure. */
    glDisable(GL_ALPHA_TEST);
    glDepthMask(GL_FALSE);
    glColor4f(1, 1, 1, 1);

    float radius = shadow_size;
    int x0 = (int)floorf(x - radius - 1.0f);
    int x1 = (int)floorf(x + radius + 1.0f);
    int z0 = (int)floorf(z - radius - 1.0f);
    int z1 = (int)floorf(z + radius + 1.0f);
    int cy = (int)floorf(y);
    glBegin(GL_QUADS);
    for (int bz = z0; bz <= z1; ++bz) {
        for (int bx = x0; bx <= x1; ++bx) {
            float sy = 0.0f, br = 1.0f;
            if (!java_shadow_block_top(bx, cy, bz, &sy, &br)) continue;
            float dy = y - sy;
            if (dy < 0.0f || dy > 4.0f) continue;
            float cx = (float)bx + 0.5f;
            float cz = (float)bz + 0.5f;
            float dx = (cx - x) / radius;
            float dz = (cz - z) / radius;
            float dist = sqrtf(dx * dx + dz * dz);
            if (dist > 1.35f) continue;
            float alpha = (shadow_alpha - dy / 2.0f) * 0.35f * (1.0f - dist / 1.35f) * br;
            if (alpha <= 0.0f) continue;
            if (alpha > 0.22f) alpha = 0.22f;
            glColor4f(1.0f, 1.0f, 1.0f, alpha);
            glTexCoord2f(0, 1); glVertex3f((float)bx,     sy, (float)bz + 1);
            glTexCoord2f(1, 1); glVertex3f((float)bx + 1, sy, (float)bz + 1);
            glTexCoord2f(1, 0); glVertex3f((float)bx + 1, sy, (float)bz);
            glTexCoord2f(0, 0); glVertex3f((float)bx,     sy, (float)bz);
        }
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    glColor4f(1,1,1,1);
}


static int render_item_as_block_id(int id) {
    if (id < 1 || id > BLOCK_REDSTONE_LAMP_ON) return 0;
    if (block_is_liquid(id)) return 0;
    if (id == BLOCK_SAPLING || id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE ||
        id == BLOCK_BROWN_MUSHROOM || id == BLOCK_RED_MUSHROOM || id == BLOCK_TORCH ||
        id == BLOCK_REDSTONE_TORCH_OFF || id == BLOCK_REDSTONE_TORCH_ON || id == BLOCK_FIRE ||
        id == BLOCK_REDSTONE_WIRE || id == BLOCK_CROPS || id == BLOCK_SIGN_POST ||
        id == BLOCK_WALL_SIGN || id == BLOCK_WOOD_DOOR || id == BLOCK_IRON_DOOR ||
        id == BLOCK_LADDER || id == BLOCK_RAILS || id == BLOCK_LEVER || id == BLOCK_REEDS ||
        id == BLOCK_PORTAL || id == BLOCK_TALL_GRASS || id == BLOCK_DEAD_BUSH ||
        id == BLOCK_POWERED_RAIL || id == BLOCK_DETECTOR_RAIL || id == BLOCK_VINE ||
        id == BLOCK_LILY_PAD || id == BLOCK_NETHER_WART || id == BLOCK_PUMPKIN_STEM ||
        id == BLOCK_MELON_STEM || id == BLOCK_REDSTONE_REPEATER_OFF ||
        id == BLOCK_REDSTONE_REPEATER_ON || id == BLOCK_END_PORTAL) return 0;
    return 1;
}

static int world_item_is_block_id(int id) {
    return id >= 1 && id <= BLOCK_REDSTONE_LAMP_ON;
}

static int world_block_item_tile(int id) {
    switch (id) {
        case BLOCK_SAPLING: return 15;
        case BLOCK_YELLOW_FLOWER: return 13;
        case BLOCK_RED_ROSE: return 12;
        case BLOCK_BROWN_MUSHROOM: return 29;
        case BLOCK_RED_MUSHROOM: return 28;
        case BLOCK_TORCH: return 80;
        case BLOCK_FIRE: return 31;
        case BLOCK_REDSTONE_WIRE: return 164;
        case BLOCK_CROPS: return 88;
        case BLOCK_SIGN_POST:
        case BLOCK_WALL_SIGN: return 4;
        case BLOCK_WOOD_DOOR: return 97;
        case BLOCK_IRON_DOOR: return 98;
        case BLOCK_LADDER: return 83;
        case BLOCK_RAILS: return 128;
        case BLOCK_POWERED_RAIL: return 179;
        case BLOCK_DETECTOR_RAIL: return 195;
        case BLOCK_LEVER: return 96;
        case BLOCK_REEDS: return 73;
        case BLOCK_REDSTONE_TORCH_OFF: return 115;
        case BLOCK_REDSTONE_TORCH_ON: return 99;
        case BLOCK_TALL_GRASS: return 39;
        case BLOCK_DEAD_BUSH: return 55;
        case BLOCK_VINE: return 143;
        case BLOCK_LILY_PAD: return 76;
        case BLOCK_NETHER_WART: return 226;
        case BLOCK_PUMPKIN_STEM:
        case BLOCK_MELON_STEM: return 19;
        case BLOCK_REDSTONE_REPEATER_OFF: return 131;
        case BLOCK_REDSTONE_REPEATER_ON: return 147;
        case BLOCK_END_PORTAL: return 15;
        default: return 1;
    }
}

static void draw_block_item_model(int id, float x, float y, float z);


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

    /* Caller sets entity/item brightness before drawing. */
    glDisable(GL_CULL_FACE);
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

static void draw_dropped_terrain_sprite(int tile) {
    if (!tex_terrain.id) return;
    float u0, v0, u1, v1;
    terrain_tile_uv(tile, &u0, &v0, &u1, &v1);

    /* Caller sets entity/item brightness before drawing. */
    glDisable(GL_CULL_FACE);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
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


static void draw_vehicle_box_tile(int tile, float x0, float y0, float z0, float x1, float y1, float z1) {
    if (!tex_terrain.id) return;
    float u0, v0, u1, v1;
    terrain_tile_uv(tile, &u0, &v0, &u1, &v1);
    world_set_shade(1.0f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y1, z1, u1, v1); world_tex_vertex(x0, y1, z1, u0, v1);
    glEnd();
    world_set_shade(0.8f);
    glBegin(GL_QUADS);
    world_tex_vertex(x1, y1, z0, u0, v0); world_tex_vertex(x0, y1, z0, u1, v0); world_tex_vertex(x0, y0, z0, u1, v1); world_tex_vertex(x1, y0, z0, u0, v1);
    world_tex_vertex(x0, y1, z1, u0, v0); world_tex_vertex(x1, y1, z1, u1, v0); world_tex_vertex(x1, y0, z1, u1, v1); world_tex_vertex(x0, y0, z1, u0, v1);
    glEnd();
    world_set_shade(0.6f);
    glBegin(GL_QUADS);
    world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x0, y1, z1, u1, v0); world_tex_vertex(x0, y0, z1, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    world_tex_vertex(x1, y1, z1, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x1, y0, z1, u0, v1);
    glEnd();
}

static void draw_vehicle_boat_model(void) {
    /* ModelBoat parity is still pending.  This keeps spawned boats visible with
       the correct wooden atlas while the item function/entity path is wired. */
    draw_vehicle_box_tile(4, -0.70f, -0.18f, -0.38f, 0.70f, 0.00f, 0.38f);  /* floor */
    draw_vehicle_box_tile(4, -0.78f, -0.15f, -0.46f, -0.62f, 0.42f, 0.46f); /* left side */
    draw_vehicle_box_tile(4,  0.62f, -0.15f, -0.46f,  0.78f, 0.42f, 0.46f); /* right side */
    draw_vehicle_box_tile(4, -0.62f, -0.15f, -0.54f,  0.62f, 0.42f,-0.38f); /* front */
    draw_vehicle_box_tile(4, -0.62f, -0.15f,  0.38f,  0.62f, 0.42f, 0.54f); /* back */
}

static void draw_vehicle_minecart_model(int type) {
    int tile = 22; /* iron block-ish atlas tile in classic terrain.png */
    draw_vehicle_box_tile(tile, -0.50f, -0.25f, -0.50f, 0.50f, -0.06f, 0.50f);
    draw_vehicle_box_tile(tile, -0.60f, -0.20f, -0.58f, -0.42f, 0.42f, 0.58f);
    draw_vehicle_box_tile(tile,  0.42f, -0.20f, -0.58f,  0.60f, 0.42f, 0.58f);
    draw_vehicle_box_tile(tile, -0.42f, -0.20f, -0.60f,  0.42f, 0.42f,-0.42f);
    draw_vehicle_box_tile(tile, -0.42f, -0.20f,  0.42f,  0.42f, 0.42f, 0.60f);
    if (type == FLAT_VEHICLE_MINECART_CHEST) {
        draw_vehicle_box_tile(25, -0.32f, 0.05f, -0.32f, 0.32f, 0.55f, 0.32f);
    } else if (type == FLAT_VEHICLE_MINECART_FURNACE) {
        draw_vehicle_box_tile(45, -0.32f, 0.05f, -0.32f, 0.32f, 0.55f, 0.32f);
    }
}

static void draw_vehicles(float partial) {
    if (!tex_terrain.id) return;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glColor4f(1,1,1,1);
    for (int i = 0; i < MAX_VEHICLE_ENTITIES; ++i) {
        FlatVehicle *v = &g_vehicles[i];
        if (!v->active) continue;
        float x = v->prev_x + (v->x - v->prev_x) * partial;
        float y = v->prev_y + (v->y - v->prev_y) * partial;
        float z = v->prev_z + (v->z - v->prev_z) * partial;
        float yaw = lerp_angle(v->prev_yaw, v->yaw, partial);
        draw_java_entity_shadow(x, y, z, v->type == FLAT_VEHICLE_BOAT ? 0.70f : 0.55f, 1.0f);
        glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(yaw, 0.0f, 1.0f, 0.0f);
        if (v->type == FLAT_VEHICLE_BOAT) draw_vehicle_boat_model();
        else draw_vehicle_minecart_model(v->type);
        glPopMatrix();
    }
}

static void entity_item_light_prepare(float x, float y, float z);

static void draw_projectiles(float partial) {
    if (!tex_items.id) return;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_items.id);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int i = 0; i < MAX_PROJECTILE_ENTITIES; ++i) {
        FlatProjectile *p = &g_projectiles[i];
        if (!p->active) continue;
        float x = p->prev_x + (p->x - p->prev_x) * partial;
        float y = p->prev_y + (p->y - p->prev_y) * partial;
        float z = p->prev_z + (p->z - p->prev_z) * partial;
        int item = ITEM_POTION;
        if (p->type == FLAT_PROJECTILE_XP_BOTTLE) item = ITEM_EXP_BOTTLE;
        else if (p->type == FLAT_PROJECTILE_ARROW) item = ITEM_ARROW;
        else if (p->type == FLAT_PROJECTILE_SNOWBALL) item = ITEM_SNOWBALL;
        else if (p->type == FLAT_PROJECTILE_SMALL_FIREBALL || p->type == FLAT_PROJECTILE_LARGE_FIREBALL) item = ITEM_FIREBALL_CHARGE;
        int tile = item_icon_tile(item);
        entity_item_light_prepare(x, y, z);
        glPushMatrix();
        glTranslatef(x, y, z);
        if (p->type == FLAT_PROJECTILE_ARROW) {
            float au0, av0, au1, av1;
            item_tile_uv(tile, &au0, &av0, &au1, &av1);
            glRotatef(p->yaw - 90.0f, 0.0f, 1.0f, 0.0f);
            glRotatef(p->pitch, 0.0f, 0.0f, 1.0f);
            glRotatef(45.0f, 1.0f, 0.0f, 0.0f);
            glScalef(0.05625f, 0.05625f, 0.05625f);
            glTranslatef(-4.0f, 0.0f, 0.0f);
            /* Caller/entity brightness stays in the current GL color. */
            glBegin(GL_QUADS);
            float u0 = au0, u1 = au0 + (au1 - au0) * 0.35f;
            float v0 = av0 + (av1 - av0) * 0.35f, v1 = av0 + (av1 - av0) * 0.70f;
            glTexCoord2f(u0, v0); glVertex3f(-7.0f, -2.0f, -2.0f);
            glTexCoord2f(u1, v0); glVertex3f(-7.0f, -2.0f,  2.0f);
            glTexCoord2f(u1, v1); glVertex3f(-7.0f,  2.0f,  2.0f);
            glTexCoord2f(u0, v1); glVertex3f(-7.0f,  2.0f, -2.0f);
            glTexCoord2f(u0, v0); glVertex3f(-7.0f,  2.0f, -2.0f);
            glTexCoord2f(u1, v0); glVertex3f(-7.0f,  2.0f,  2.0f);
            glTexCoord2f(u1, v1); glVertex3f(-7.0f, -2.0f,  2.0f);
            glTexCoord2f(u0, v1); glVertex3f(-7.0f, -2.0f, -2.0f);
            for (int f = 0; f < 4; ++f) {
                float a = (float)f * (float)M_PI * 0.5f;
                float c = cosf(a), s = sinf(a);
                float y0 = -2.0f * c, z0 = -2.0f * s;
                float y1 =  2.0f * c, z1 =  2.0f * s;
                float fu0 = au0, fu1 = au1, fv0 = av0, fv1 = av0 + (av1 - av0) * 0.35f;
                glTexCoord2f(fu0, fv0); glVertex3f(-8.0f, y0, z0);
                glTexCoord2f(fu1, fv0); glVertex3f( 8.0f, y0, z0);
                glTexCoord2f(fu1, fv1); glVertex3f( 8.0f, y1, z1);
                glTexCoord2f(fu0, fv1); glVertex3f(-8.0f, y1, z1);
            }
            glEnd();
        } else {
            glRotatef((float)p->age * 18.0f + partial * 18.0f, 0.0f, 1.0f, 0.0f);
            glRotatef(20.0f, 1.0f, 0.0f, 0.0f);
            glScalef(0.25f, 0.25f, 0.25f);
            draw_item3d_from_texture(&tex_items, tile);
        }
        glPopMatrix();
    }
    glColor4f(1,1,1,1);
}

static void draw_xp_orbs(float partial) {
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int i = 0; i < MAX_XP_ORBS; ++i) {
        FlatXPOrb *o = &g_xp_orbs[i];
        if (!o->active) continue;
        float x = o->prev_x + (o->x - o->prev_x) * partial;
        float y = o->prev_y + (o->y - o->prev_y) * partial;
        float z = o->prev_z + (o->z - o->prev_z) * partial;
        float pulse = 0.12f + 0.03f * sinf(((float)o->age + partial) * 0.35f);
        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(-g_player_yaw, 0.0f, 1.0f, 0.0f);
        glColor4f(0.35f, 1.0f, 0.05f, 0.85f);
        glBegin(GL_QUADS);
        glVertex3f(-pulse, -pulse, 0.0f); glVertex3f(pulse, -pulse, 0.0f); glVertex3f(pulse, pulse, 0.0f); glVertex3f(-pulse, pulse, 0.0f);
        glEnd();
        glColor4f(1.0f, 1.0f, 0.15f, 0.75f);
        glBegin(GL_QUADS);
        glVertex3f(-pulse * 0.5f, -pulse * 0.5f, 0.01f); glVertex3f(pulse * 0.5f, -pulse * 0.5f, 0.01f); glVertex3f(pulse * 0.5f, pulse * 0.5f, 0.01f); glVertex3f(-pulse * 0.5f, pulse * 0.5f, 0.01f);
        glEnd();
        glPopMatrix();
    }
    glColor4f(1,1,1,1);
    glEnable(GL_TEXTURE_2D);
}

static float entity_light_factor_at(float x, float y, float z) {
    float br = flat_light_brightness((int)floorf(x), (int)floorf(y), (int)floorf(z));
    if (br < 0.18f) br = 0.18f;
    if (br > 1.0f) br = 1.0f;
    return br;
}

static void entity_item_light_prepare(float x, float y, float z) {
    float br = entity_light_factor_at(x, y, z);
    glColor4f(br, br, br, 1.0f);
    int lx = (int)floorf(x);
    int ly = (int)floorf(y);
    int lz = (int)floorf(z);
    world_style_set_pos(lx, ly, lz);
    world_light_set_pos(lx, ly, lz);
}

static void draw_dropped_items(void) {
    const PexPlayerRenderState *pr = &g_player_render_frame;
    float yaw = lerp_angle(pr->prev_yaw, pr->yaw, g_frame_partial);
    float item_partial = g_frame_partial;
    for (int i = 0; i < MAX_DROP_ENTITIES; i++) {
        FlatDroppedItem *e = &g_drops[i];
        if (!e->active) continue;
        float x = e->prev_x + (e->x - e->prev_x) * item_partial;
        float y = e->prev_y + (e->y - e->prev_y) * item_partial;
        float z = e->prev_z + (e->z - e->prev_z) * item_partial;
        if (g_mp_connected && e->net_id > 0) {
            PexNetRenderDropState *r = pex_net_find_render_drop(e->net_id);
            if (r && r->active) {
                x = r->x;
                y = r->y;
                z = r->z;
            }
        }
        float bob = sinf(((float)e->age + g_frame_partial) / 10.0f + e->rot) * 0.1f + 0.1f;
        int copies = dropped_item_copy_count(e->stack.count);

        draw_java_entity_shadow(x, y, z, 0.15f, 1.0f);

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        entity_item_light_prepare(x, y + 0.5f, z);
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
                draw_block_item_model(e->stack.id, -0.5f, -0.5f, -0.5f);
                glPopMatrix();
            }
        } else if (world_item_is_block_id(e->stack.id) && tex_terrain.id) {
            /* Non-cube block drops (ladder/rail/torch/etc.) use their terrain
               tile. Do not look them up in gui/items.png, where block id 65
               aliases an armor icon. */
            glRotatef(180.0f - yaw, 0.0f, 1.0f, 0.0f);
            glScalef(0.5f, 0.5f, 0.5f);
            int tile = world_block_item_tile(e->stack.id);
            for (int c = 0; c < copies; c++) {
                glPushMatrix();
                float ox, oy, oz;
                dropped_item_copy_offset(c, 0.5f, &ox, &oy, &oz);
                glTranslatef(ox * 0.25f, oy * 0.25f, 0.0f);
                draw_dropped_terrain_sprite(tile);
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


static void draw_pickup_fx_items(void) {
    const PexPlayerRenderState *pr = &g_player_render_frame;
    float yaw = lerp_angle(pr->prev_yaw, pr->yaw, g_frame_partial);
    for (int i = 0; i < MAX_PICKUP_FX; ++i) {
        PickupFx *fx = &g_pickup_fx[i];
        if (!fx->active) continue;
        float t = ((float)fx->age + g_frame_partial) / (float)(fx->max_age > 0 ? fx->max_age : 3);
        if (t < 0.0f) t = 0.0f; if (t > 1.0f) t = 1.0f;
        t = t * t;
        float px = pr->prev_x + (pr->x - pr->prev_x) * g_frame_partial;
        float py = pr->prev_y + (pr->y - pr->prev_y) * g_frame_partial - 0.5f;
        float pz = pr->prev_z + (pr->z - pr->prev_z) * g_frame_partial;
        float x = fx->start_x + (px - fx->start_x) * t;
        float y = fx->start_y + (py - fx->start_y) * t;
        float z = fx->start_z + (pz - fx->start_z) * t;

        entity_item_light_prepare(x, y + 0.5f, z);
        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef((((float)fx->age + g_frame_partial) / 20.0f + fx->rot) * 57.29578f, 0.0f, 1.0f, 0.0f);
        if (render_item_as_block_id(fx->stack.id) && tex_terrain.id) {
            glScalef(0.25f, 0.25f, 0.25f);
            glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
            draw_block_item_model(fx->stack.id, -0.5f, -0.5f, -0.5f);
        } else if (world_item_is_block_id(fx->stack.id) && tex_terrain.id) {
            glRotatef(180.0f - yaw, 0.0f, 1.0f, 0.0f);
            glScalef(0.5f, 0.5f, 0.5f);
            draw_dropped_terrain_sprite(world_block_item_tile(fx->stack.id));
        } else if (tex_items.id) {
            glRotatef(180.0f - yaw, 0.0f, 1.0f, 0.0f);
            glScalef(0.5f, 0.5f, 0.5f);
            draw_dropped_item_sprite(item_icon_tile(fx->stack.id));
        }
        glPopMatrix();
    }
}

static void draw_block_selection_border(void) {
    FlatRayHit hit = flat_raycast();
    int id = flat_get_block(hit.bx, hit.by, hit.bz);
    if (!hit.hit || id == 0) return;

    float e = 0.002f;
    BlockBounds b = block_bounds_for_selection(id, hit.bx, hit.by, hit.bz);
    float x0 = b.x0 - e, y0 = b.y0 - e, z0 = b.z0 - e;
    float x1 = b.x1 + e, y1 = b.y1 + e, z1 = b.z1 + e;

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
    if (id == 0) return 0;
    if (block_is_liquid(id)) return flat_separate_liquid_pass_enabled() ? 0 : 1;
    if (id == BLOCK_CHEST) return 0; /* chest model is smaller than a full cube; never cull neighboring faces behind it */
    if (id == BLOCK_SLAB || id == BLOCK_WOOD_STAIRS || id == BLOCK_COBBLE_STAIRS ||
        id == BLOCK_BRICK_STAIRS || id == BLOCK_STONE_BRICK_STAIRS || id == BLOCK_NETHER_BRICK_STAIRS ||
        id == BLOCK_FENCE || id == BLOCK_NETHER_BRICK_FENCE || id == BLOCK_GLASS_PANE ||
        id == BLOCK_IRON_BARS || id == BLOCK_CACTUS || block_is_door_id(id) ||
        id == BLOCK_LADDER || id == BLOCK_RAILS || id == BLOCK_POWERED_RAIL || id == BLOCK_DETECTOR_RAIL ||
        id == BLOCK_STONE_PRESSURE_PLATE || id == BLOCK_WOOD_PRESSURE_PLATE || id == BLOCK_STONE_BUTTON ||
        id == BLOCK_LEVER || id == BLOCK_TORCH || id == BLOCK_REDSTONE_TORCH_OFF || id == BLOCK_REDSTONE_TORCH_ON ||
        id == BLOCK_WEB || id == BLOCK_LILY_PAD || id == BLOCK_END_PORTAL || id == BLOCK_END_PORTAL_FRAME || id == BLOCK_PORTAL) return 0;
    if (id == BLOCK_LEAVES) return flat_fancy_cutout_terrain_enabled() ? 0 : 1;
    if (id == BLOCK_GLASS || id == BLOCK_SAPLING ||
        id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE ||
        id == BLOCK_BROWN_MUSHROOM || id == BLOCK_RED_MUSHROOM ||
        id == BLOCK_TORCH || id == BLOCK_FIRE || id == BLOCK_REDSTONE_WIRE ||
        id == BLOCK_REDSTONE_TORCH_OFF || id == BLOCK_REDSTONE_TORCH_ON ||
        id == BLOCK_REEDS || id == BLOCK_TALL_GRASS || id == BLOCK_DEAD_BUSH ||
        id == BLOCK_VINE || id == BLOCK_NETHER_WART || id == BLOCK_SNOW_LAYER) return 0;
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

static int block_uses_cross_plant_model(int id) {
    return id == BLOCK_SAPLING || id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE ||
           id == BLOCK_BROWN_MUSHROOM || id == BLOCK_RED_MUSHROOM ||
           id == BLOCK_REEDS || id == BLOCK_TALL_GRASS || id == BLOCK_DEAD_BUSH ||
           id == BLOCK_NETHER_WART || id == BLOCK_FIRE || id == BLOCK_REDSTONE_WIRE ||
           id == BLOCK_CROPS || id == BLOCK_PUMPKIN_STEM || id == BLOCK_MELON_STEM;
}

static int cross_plant_tile_for_block_meta(int id, int meta) {
    if (id == BLOCK_YELLOW_FLOWER) return 13;
    if (id == BLOCK_RED_ROSE) return 12;
    if (id == BLOCK_SAPLING) { meta &= 3; return meta == 1 ? 63 : (meta == 2 ? 79 : (meta == 3 ? 30 : 15)); }
    if (id == BLOCK_BROWN_MUSHROOM) return 29;
    if (id == BLOCK_RED_MUSHROOM) return 28;
    if (id == BLOCK_REEDS) return 73;
    if (id == BLOCK_TALL_GRASS) return meta == 1 ? 39 : (meta == 2 ? 56 : 55);
    if (id == BLOCK_DEAD_BUSH) return 55;
    if (id == BLOCK_NETHER_WART) return meta >= 3 ? 228 : (meta > 0 ? 227 : 226);
    if (id == BLOCK_PUMPKIN_STEM || id == BLOCK_MELON_STEM) return 19;
    if (id == BLOCK_FIRE) return 31;
    if (id == BLOCK_REDSTONE_WIRE) return 84;
    if (id == BLOCK_CROPS) return 88;
    return 12;
}
static int cross_plant_tile_for_block(int id) { return cross_plant_tile_for_block_meta(id, 0); }

static void emit_cross_plant_block_float(int id, float x, float y, float z) {
    int bx = (int)floorf(x);
    int by = (int)floorf(y);
    int bz = (int)floorf(z);
    world_style_set_pos(bx, by, bz);
    g_world_light_x = bx;
    g_world_light_y = by;
    g_world_light_z = bz;

    float u0, v0, u1, v1;
    terrain_tile_uv(cross_plant_tile_for_block_meta(id, flat_get_meta(bx, by, bz)), &u0, &v0, &u1, &v1);
    if (id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE || id == BLOCK_CROPS) {
        /* The rose texture has empty pixels at the top of the tile.  Crop that
           transparent cap for the world model so flowers do not look buried
           low in the grass. */
        float th = tex_terrain.h ? (float)tex_terrain.h : 256.0f;
        v0 += 4.0f / th;
    }

    float x0 = x + 0.10f, x1 = x + 0.90f;
    float y0 = y,       y1 = y + 1.00f;
    float z0 = z + 0.10f, z1 = z + 0.90f;

    /* Grass/fern textures in terrain.png are grayscale masks in this era;
       Java tints them with the biome grass color at render time.  Without this
       pass they show up as the gray "paper" plants visible in snowy/tundra
       screenshots. */
    if (id == BLOCK_TALL_GRASS) world_set_color_shade(java_grass_color_at(bx, bz), 1.0f);
    else if (id == BLOCK_REEDS || id == BLOCK_SAPLING) world_set_color_shade(java_foliage_color_at(bx, bz), 1.0f);
    else world_set_shade(1.0f);
    world_tex_vertex(x0, y1, z0, u0, v0);
    world_tex_vertex(x1, y1, z1, u1, v0);
    world_tex_vertex(x1, y0, z1, u1, v1);
    world_tex_vertex(x0, y0, z0, u0, v1);

    world_tex_vertex(x1, y1, z0, u0, v0);
    world_tex_vertex(x0, y1, z1, u1, v0);
    world_tex_vertex(x0, y0, z1, u1, v1);
    world_tex_vertex(x1, y0, z0, u0, v1);
}

static void emit_cross_plant_block(int id, int x, int y, int z) {
    emit_cross_plant_block_float(id, (float)x, (float)y, (float)z);
}

static void draw_cross_plant_block(int id, float x, float y, float z) {
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    glBegin(GL_QUADS);
    emit_cross_plant_block_float(id, x, y, z);
    glEnd();
    glDisable(GL_ALPHA_TEST);
}

static int flat_face_exposed_for_block(int id, int x, int y, int z, int face) {
    if (face == 0 && y <= FLAT_WORLD_Y_MIN) return 0; /* bottom of bedrock never needs drawing */
    int n = neighbor_for_face(x, y, z, face);

    /* Java BlockLeavesBase:
       - Fast leaves hide faces against the same block.
       - Fancy leaves render through neighboring leaf blocks because leaves are
         not opaque.  This renderer is two-sided on the terrain path, so keep
         one canonical quad for each leaf/leaf boundary instead of two coplanar
         quads.  That preserves the Java fancy-leaf visible interior without
         z-fighting or texture overlap. */
    if (id == BLOCK_LEAVES && n == BLOCK_LEAVES) {
        if (!flat_fancy_cutout_terrain_enabled()) return 0;
        return (face == 1 || face == 3 || face == 5);
    }

    /* A snow layer sits directly on the top face of the supporting block.
       Rendering the covered grass/dirt/stone top doubles the visible-surface
       cost across snowy biomes and creates unnecessary overdraw/z pressure.
       Java batches snow through RenderBlocks' standard path; in this C port we
       explicitly skip the hidden support top to keep snow fields cheap. */
    if (face == 1 && n == BLOCK_SNOW_LAYER) return 0;

    /* Liquids use their own occlusion rule.  Even in Fast graphics, do not draw
       faces between water blocks; otherwise oceans become stacks of internal
       opaque quads and look broken from underwater. */
    if (block_is_liquid(id)) {
        if (block_is_liquid(n)) return 0;
        return !block_occludes_render_face(n);
    }

    /* Solid terrain touching water must keep its face.  In Fast graphics water
       itself is opaque, but the player can stand inside it; if the terrain face
       is culled, going underwater turns the water volume into an x-ray hole. */
    if (block_is_liquid(n)) return 1;

    return !block_occludes_render_face(n);
}

static int flat_face_exposed(int x, int y, int z, int face) {
    return flat_face_exposed_for_block(flat_get_block(x, y, z), x, y, z, face);
}

static int liquid_face_exposed(int x, int y, int z, int face) {
    int n = neighbor_for_face(x, y, z, face);
    if (block_is_liquid(n)) return 0;
    return !block_occludes_render_face(n);
}

static float world_face_base_shade(int face) {
    if (face == 0) return 0.50f;
    if (face == 2 || face == 3) return 0.80f;
    if (face == 4 || face == 5) return 0.60f;
    return 1.0f;
}

static int world_face_tint_rgb(int id, int face) {
    if (id == BLOCK_GRASS && face == 1) return java_grass_color_at(g_world_style_x, g_world_style_z);
    if (id == BLOCK_LEAVES) return java_foliage_color_at(g_world_style_x, g_world_style_z);
    return 0xFFFFFF;
}

static uint32_t world_ao_cell_mixed_brightness(int x, int y, int z) {
    return (uint32_t)flat_pack_sky_block_light(x, y, z, 0);
}

static int world_ao_cell_is_normal_cube(int x, int y, int z) {
    return block_occludes_render_face(flat_get_block(x, y, z));
}

static float world_ao_cell_value(int x, int y, int z, int *blocks_grass) {
    int id = flat_get_block(x, y, z);
    int can_block_grass = flat_block_can_block_grass_java(id);
    if (blocks_grass) *blocks_grass = !can_block_grass;
    return world_ao_cell_is_normal_cube(x, y, z) ? 0.2f : 1.0f;
}

static uint32_t world_ao_mix_brightness(uint32_t a, uint32_t b, uint32_t c, uint32_t fallback) {
    if (a == 0) a = fallback;
    if (b == 0) b = fallback;
    if (c == 0) c = fallback;
    return ((a + b + c + fallback) >> 2) & 0x00FF00FFu;
}

static int smooth_block_lighting_enabled(int id) {
    return g_opts.fancy_graphics &&
           !g_force_fullbright_item_model &&
           flat_block_light_value_for_id(id) == 0;
}

static void world_face_sample_frame(int face,
                                    int *nx, int *ny, int *nz,
                                    int *ax, int *ay, int *az,
                                    int *bx, int *by, int *bz,
                                    int corner_a[4], int corner_b[4]) {
    *nx = *ny = *nz = 0;
    *ax = *ay = *az = 0;
    *bx = *by = *bz = 0;
    if (face == 1) {
        *ny = 1; *ax = 1; *bz = 1;
        corner_a[0] = -1; corner_b[0] = -1;
        corner_a[1] =  1; corner_b[1] = -1;
        corner_a[2] =  1; corner_b[2] =  1;
        corner_a[3] = -1; corner_b[3] =  1;
    } else if (face == 0) {
        *ny = -1; *ax = 1; *bz = 1;
        corner_a[0] = -1; corner_b[0] =  1;
        corner_a[1] =  1; corner_b[1] =  1;
        corner_a[2] =  1; corner_b[2] = -1;
        corner_a[3] = -1; corner_b[3] = -1;
    } else if (face == 2) {
        *nz = -1; *ax = 1; *by = 1;
        corner_a[0] =  1; corner_b[0] =  1;
        corner_a[1] = -1; corner_b[1] =  1;
        corner_a[2] = -1; corner_b[2] = -1;
        corner_a[3] =  1; corner_b[3] = -1;
    } else if (face == 3) {
        *nz = 1; *ax = 1; *by = 1;
        corner_a[0] = -1; corner_b[0] =  1;
        corner_a[1] =  1; corner_b[1] =  1;
        corner_a[2] =  1; corner_b[2] = -1;
        corner_a[3] = -1; corner_b[3] = -1;
    } else if (face == 4) {
        *nx = -1; *az = 1; *by = 1;
        corner_a[0] = -1; corner_b[0] =  1;
        corner_a[1] =  1; corner_b[1] =  1;
        corner_a[2] =  1; corner_b[2] = -1;
        corner_a[3] = -1; corner_b[3] = -1;
    } else {
        *nx = 1; *az = 1; *by = 1;
        corner_a[0] =  1; corner_b[0] =  1;
        corner_a[1] = -1; corner_b[1] =  1;
        corner_a[2] = -1; corner_b[2] = -1;
        corner_a[3] =  1; corner_b[3] = -1;
    }
}

static void world_smooth_face_lights(int x, int y, int z, int face, float ao_out[4], uint32_t light_out[4]) {
    int nx, ny, nz, ax, ay, az, bx, by, bz;
    int corner_a[4], corner_b[4];
    float ao_samples[3][3];
    uint32_t light_samples[3][3];
    int blocks_grass[3][3];
    world_face_sample_frame(face, &nx, &ny, &nz, &ax, &ay, &az, &bx, &by, &bz, corner_a, corner_b);

    int base_x = x + nx;
    int base_y = y + ny;
    int base_z = z + nz;
    uint32_t fallback_light = world_ao_cell_mixed_brightness(base_x, base_y, base_z);
    for (int ia = 0; ia < 3; ++ia) {
        for (int ib = 0; ib < 3; ++ib) {
            int da = ia - 1;
            int db = ib - 1;
            int sx = base_x + da * ax + db * bx;
            int sy = base_y + da * ay + db * by;
            int sz = base_z + da * az + db * bz;
            ao_samples[ia][ib] = world_ao_cell_value(sx, sy, sz, &blocks_grass[ia][ib]);
            light_samples[ia][ib] = world_ao_cell_mixed_brightness(sx, sy, sz);
        }
    }

    for (int i = 0; i < 4; ++i) {
        int ia = corner_a[i] > 0 ? 2 : 0;
        int ib = corner_b[i] > 0 ? 2 : 0;
        /* RenderBlocks: only use the diagonal corner sample when at least one of
           the two side cells is passable-to-grass.  If both side cells block
           grass, reuse the side sample to avoid diagonal light leaking through
           a solid corner. */
        int use_side_diag = (blocks_grass[ia][1] && blocks_grass[1][ib]) ? 1 : 0;
        float diag_ao = use_side_diag ? ao_samples[ia][1] : ao_samples[ia][ib];
        uint32_t diag_light = use_side_diag ? light_samples[ia][1] : light_samples[ia][ib];
        ao_out[i] = (ao_samples[1][1] + ao_samples[ia][1] + ao_samples[1][ib] + diag_ao) * 0.25f;
        light_out[i] = world_ao_mix_brightness(diag_light, light_samples[ia][1], light_samples[1][ib], fallback_light);
    }
}

static void world_tex_vertex_lit(float x, float y, float z, float u, float v,
                                 int rgb, float face_shade, float ao, uint32_t packed_light) {
    float r = ((rgb >> 16) & 255) / 255.0f;
    float g = ((rgb >> 8) & 255) / 255.0f;
    float b = (rgb & 255) / 255.0f;
    float shade = face_shade * ao;
    flat_direct_set_color_light4f(r * shade, g * shade, b * shade, 1.0f, packed_light);
    world_tex_vertex(x, y, z, u, v);
}

static int block_texture_resolve(int block_id, int meta, int face) {
    meta &= 15;
    switch (block_id) {
        case BLOCK_PLANKS:
            switch (meta & 3) {
                case BLOCK_META_WOOD_SPRUCE: return 198;
                case BLOCK_META_WOOD_BIRCH: return 214;
                case BLOCK_META_WOOD_JUNGLE: return 199;
                default: return 4;
            }
        case BLOCK_LOG:
            if (face == 0 || face == 1) return 21;
            switch (meta & 3) {
                case BLOCK_META_WOOD_SPRUCE: return 116;
                case BLOCK_META_WOOD_BIRCH: return 117;
                case BLOCK_META_WOOD_JUNGLE: return 153;
                default: return 20;
            }
        case BLOCK_LEAVES: {
            int base = flat_fancy_leaf_texture_enabled() ? 52 : 53;
            int type = meta & 3;
            if (type == BLOCK_META_WOOD_SPRUCE) return base + 80;
            if (type == BLOCK_META_WOOD_JUNGLE) return base + 144;
            return base;
        }
        case BLOCK_SAPLING: return cross_plant_tile_for_block_meta(block_id, meta);
        case BLOCK_LAPIS_ORE: return 160;
        case BLOCK_LAPIS_BLOCK: return 144;
        case BLOCK_DISPENSER:
            return (face == 0 || face == 1) ? 62 : (face == 3 ? 46 : 45);
        case BLOCK_SANDSTONE:
            if (face == 1) return 176;
            if (face == 0) return (meta == BLOCK_META_SANDSTONE_CHISELED || meta == BLOCK_META_SANDSTONE_SMOOTH) ? 176 : 208;
            if (meta == BLOCK_META_SANDSTONE_CHISELED) return 229;
            if (meta == BLOCK_META_SANDSTONE_SMOOTH) return 230;
            return 192;
        case BLOCK_NOTE_BLOCK: return 75;
        case BLOCK_BED:
            if (face == 0) return 4;
            return (meta & 8) ? 134 : 135;
        case BLOCK_POWERED_RAIL: return ((meta & 7) >= 6) ? 163 : 179;
        case BLOCK_DETECTOR_RAIL: return ((meta & 7) >= 6) ? 179 : 195;
        case BLOCK_STICKY_PISTON:
        case BLOCK_PISTON: {
            int dir = meta & 7;
            int front = (block_id == BLOCK_STICKY_PISTON) ? 106 : 107;
            if (dir <= 5 && face == dir) return front;
            if (dir <= 5 && face == (dir ^ 1)) return 109;
            return 108;
        }
        case BLOCK_PISTON_EXTENSION:
        case BLOCK_PISTON_MOVING:
            return (face == (meta & 7)) ? (((meta & 8) != 0) ? 106 : 107) : 108;
        case BLOCK_WEB: return 11;
        case BLOCK_TALL_GRASS:
        case BLOCK_DEAD_BUSH:
        case BLOCK_NETHER_WART:
        case BLOCK_CROPS:
        case BLOCK_PUMPKIN_STEM:
        case BLOCK_MELON_STEM:
            return cross_plant_tile_for_block_meta(block_id, meta);
        case BLOCK_WOOL: {
            int wm = (meta & 15);
            if (wm != 0) wm = (~wm) & 15;
            return 113 + ((wm & 8) >> 3) + ((wm & 7) * 16);
        }
        case BLOCK_GOLD_BLOCK: return 23;
        case BLOCK_IRON_BLOCK: return 22;
        case BLOCK_DIAMOND_BLOCK: return 24;
        case BLOCK_REDSTONE_WIRE: return 164;
        case BLOCK_FARMLAND: return (face == 1) ? ((meta > 0) ? 86 : 87) : 2;
        case BLOCK_SLAB:
        case BLOCK_DOUBLE_SLAB: {
            int type = meta & 7;
            if (type == 0) return (face <= 1) ? 6 : 5;
            if (type == 1) return (face == 0) ? 208 : (face == 1 ? 176 : 192);
            if (type == 2) return 4;
            if (type == 3) return 16;
            if (type == 4) return 7;
            if (type == 5) return 54;
            return 6;
        }
        case BLOCK_CAKE:
            return face == 1 ? 121 : (face == 0 ? 124 : ((meta > 0 && face == 4) ? 123 : 122));
        case BLOCK_REDSTONE_REPEATER_OFF:
            return face == 1 ? 131 : (face == 0 ? 115 : 5);
        case BLOCK_REDSTONE_REPEATER_ON:
            return face == 1 ? 147 : (face == 0 ? 99 : 5);
        case BLOCK_LOCKED_CHEST:
            return (face == 0 || face == 1) ? chest_top_tile() : (face == 3 ? chest_front_tile() : chest_side_tile());
        case BLOCK_TRAPDOOR: return 84;
        case BLOCK_SILVERFISH:
            return meta == 1 ? 16 : (meta == 2 ? 54 : 1);
        case BLOCK_STONE_BRICK:
            if (meta == BLOCK_META_STONE_BRICK_MOSSY) return 100;
            if (meta == BLOCK_META_STONE_BRICK_CRACKED) return 101;
            if (meta == BLOCK_META_STONE_BRICK_CHISELED) return 213;
            return 54;
        case BLOCK_BROWN_MUSHROOM_CAP:
            return (meta == 10 && face > 1) ? 141 : (meta ? 126 : 142);
        case BLOCK_RED_MUSHROOM_CAP:
            return (meta == 10 && face > 1) ? 141 : (meta ? 125 : 142);
        case BLOCK_IRON_BARS: return 85;
        case BLOCK_GLASS_PANE: return 49;
        case BLOCK_MELON: return (face == 1 || face == 0) ? 137 : 136;
        case BLOCK_VINE: return 143;
        case BLOCK_FENCE_GATE: return 4;
        case BLOCK_BRICK_STAIRS: return 7;
        case BLOCK_STONE_BRICK_STAIRS: return 54;
        case BLOCK_MYCELIUM: return face == 1 ? 78 : (face == 0 ? 2 : 77);
        case BLOCK_LILY_PAD: return 76;
        case BLOCK_NETHER_BRICK:
        case BLOCK_NETHER_BRICK_FENCE:
        case BLOCK_NETHER_BRICK_STAIRS: return 224;
        case BLOCK_ENCHANTMENT_TABLE: return face == 0 ? 183 : (face == 1 ? 166 : 182);
        case BLOCK_BREWING_STAND: return 157;
        case BLOCK_CAULDRON: return face == 1 ? 138 : (face == 0 ? 155 : 154);
        case BLOCK_END_PORTAL: return 15;
        case BLOCK_END_PORTAL_FRAME: return face == 1 ? 158 : (face == 0 ? 175 : 159);
        case BLOCK_END_STONE: return 175;
        case BLOCK_DRAGON_EGG: return 167;
        case BLOCK_REDSTONE_LAMP_OFF: return 211;
        case BLOCK_REDSTONE_LAMP_ON: return 212;
        default: return -1;
    }
}


static void world_face_style(int id, int face, int *tile) {
    float shade = world_face_base_shade(face);

    if (id == BLOCK_GRASS) {
        if (face == 1) { *tile = 0; world_set_color_shade(java_grass_color_at(g_world_style_x, g_world_style_z), shade); return; }
        if (face == 0) { *tile = 2; world_set_shade(shade); return; }
        *tile = 3; world_set_shade(shade); return;
    }

    if (id == BLOCK_WATER || id == BLOCK_STILL_WATER) {
        *tile = (face == 0 || face == 1) ? water_top_tile() : water_side_tile();
        world_set_shade(shade);
        return;
    }

    if (id == BLOCK_LAVA || id == BLOCK_STILL_LAVA) {
        *tile = (face == 0 || face == 1) ? lava_top_tile() : lava_side_tile();
        world_set_shade(shade);
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
        else if (face == 2 || face == 4) *tile = 60;
        else *tile = 59;
        world_set_shade(shade);
        return;
    }

    if (id == BLOCK_CHEST) {
        if (face == 1 || face == 0) *tile = chest_top_tile();
        else if (face == 3) *tile = chest_front_tile();
        else *tile = chest_side_tile();
        world_set_shade(shade);
        return;
    }

    if (id == BLOCK_FURNACE || id == BLOCK_FURNACE_LIT) {
        if (face == 1 || face == 0) *tile = furnace_top_tile();
        else if (face == 3) *tile = (id == BLOCK_FURNACE_LIT) ? furnace_front_lit_tile() : furnace_front_tile();
        else *tile = furnace_side_tile();
        world_set_shade(shade);
        return;
    }


    if (id == BLOCK_SAND) { *tile = 18; world_set_shade(shade); return; }
    if (id == BLOCK_GRAVEL) { *tile = 19; world_set_shade(shade); return; }
    if (id == BLOCK_GOLD_ORE) { *tile = 32; world_set_shade(shade); return; }
    if (id == BLOCK_IRON_ORE) { *tile = 33; world_set_shade(shade); return; }
    if (id == BLOCK_COAL_ORE) { *tile = 34; world_set_shade(shade); return; }
    if (id == BLOCK_OBSIDIAN) { *tile = 37; world_set_shade(shade); return; }
    if (id == BLOCK_DIAMOND_ORE) { *tile = 50; world_set_shade(shade); return; }
    if (id == BLOCK_REDSTONE_ORE || id == BLOCK_REDSTONE_ORE_GLOWING) { *tile = 51; world_set_shade(shade); return; }
    if (id == BLOCK_LAPIS_ORE) { *tile = 38; world_set_shade(shade); return; }
    if (id == BLOCK_LAPIS_BLOCK) { *tile = 39; world_set_shade(shade); return; }
    if (id == BLOCK_GOLD_BLOCK) { *tile = 39; world_set_shade(shade); return; }
    if (id == BLOCK_IRON_BLOCK) { *tile = 38; world_set_shade(shade); return; }
    if (id == BLOCK_DIAMOND_BLOCK) { *tile = 40; world_set_shade(shade); return; }
    if (id == BLOCK_BRICK) { *tile = 7; world_set_shade(shade); return; }
    if (id == BLOCK_TNT) { *tile = (face == 0 ? 10 : (face == 1 ? 9 : 8)); world_set_shade(shade); return; }
    if (id == BLOCK_GLASS) { *tile = 49; world_set_shade(shade); return; }
    if (id == BLOCK_BOOKSHELF) { *tile = 35; world_set_shade(shade); return; }
    if (id == BLOCK_MOSSY_COBBLESTONE) { *tile = 36; world_set_shade(shade); return; }
    if (id == BLOCK_LEAVES) {
        /* BlockLeaves.setGraphicsLevel(): Fancy/minimal-Fancy terrain uses base
           texture 52; the legacy opaque Fast path uses texture 53. */
        *tile = flat_fancy_leaf_texture_enabled() ? 52 : 53;
        world_set_color_shade(java_foliage_color_at(g_world_style_x, g_world_style_z), shade);
        return;
    }
    if (id == BLOCK_SPONGE) { *tile = 48; world_set_shade(shade); return; }
    if (id == BLOCK_WOOL) { *tile = 64; world_set_shade(shade); return; }
    if (id == BLOCK_SNOW_BLOCK) { *tile = 66; world_set_shade(shade); return; }
    if (id == BLOCK_ICE) { *tile = 67; world_set_shade(shade); return; }
    if (id == BLOCK_CLAY) { *tile = 72; world_set_shade(shade); return; }
    if (id == BLOCK_NETHERRACK) { *tile = 103; world_set_shade(shade); return; }
    if (id == BLOCK_SOUL_SAND) { *tile = 104; world_set_shade(shade); return; }
    if (id == BLOCK_GLOWSTONE) { *tile = 105; world_set_shade(shade); return; }
    if (id == BLOCK_SANDSTONE) { *tile = (face == 1 ? 176 : (face == 0 ? 208 : 192)); world_set_shade(shade); return; }
    if (id == BLOCK_STONE_BRICK || id == BLOCK_STONE_BRICK_STAIRS) { *tile = 54; world_set_shade(shade); return; }
    if (id == BLOCK_NETHER_BRICK || id == BLOCK_NETHER_BRICK_FENCE || id == BLOCK_NETHER_BRICK_STAIRS) { *tile = 224; world_set_shade(shade); return; }
    if (id == BLOCK_END_STONE) { *tile = 175; world_set_shade(shade); return; }
    if (id == BLOCK_SAPLING) { *tile = 15; world_set_shade(shade); return; }
    if (id == BLOCK_FARMLAND) { *tile = (face == 1) ? 87 : 2; world_set_shade(shade); return; }
    if (id == BLOCK_CROPS) { *tile = 88; world_set_shade(shade); return; }
    if (id == BLOCK_SNOW_LAYER) { *tile = 66; world_set_shade(shade); return; }
    if (id == BLOCK_SIGN_POST || id == BLOCK_WALL_SIGN) { *tile = 4; world_set_shade(shade); return; }
    if (id == BLOCK_DOUBLE_SLAB || id == BLOCK_SLAB) { *tile = (face == 0 || face == 1) ? 6 : 5; world_set_shade(shade); return; }
    if (id == BLOCK_TORCH) { *tile = 80; world_set_shade(shade); return; }
    if (id == BLOCK_FIRE) { *tile = 31; world_set_shade(shade); return; }
    if (id == BLOCK_MOB_SPAWNER) { *tile = 65; world_set_shade(shade); return; }
    if (id == BLOCK_REDSTONE_WIRE) { *tile = 84; world_set_shade(shade); return; }
    if (id == BLOCK_WOOD_STAIRS || id == BLOCK_WOOD_PRESSURE_PLATE || id == BLOCK_FENCE_GATE) { *tile = 4; world_set_shade(shade); return; }
    if (id == BLOCK_BRICK_STAIRS) { *tile = 7; world_set_shade(shade); return; }
    if (id == BLOCK_RAILS) { *tile = 128; world_set_shade(shade); return; }
    if (id == BLOCK_COBBLE_STAIRS) { *tile = 16; world_set_shade(shade); return; }
    if (id == BLOCK_LADDER) { *tile = 83; world_set_shade(shade); return; }
    if (id == BLOCK_LEVER) { *tile = 96; world_set_shade(shade); return; }
    if (id == BLOCK_STONE_PRESSURE_PLATE || id == BLOCK_STONE_BUTTON) { *tile = 1; world_set_shade(shade); return; }
    if (id == BLOCK_WOOD_DOOR) { *tile = 97; world_set_shade(shade); return; }
    if (id == BLOCK_IRON_DOOR) { *tile = 98; world_set_shade(shade); return; }
    if (id == BLOCK_REDSTONE_TORCH_OFF) { *tile = 115; world_set_shade(shade); return; }
    if (id == BLOCK_REDSTONE_TORCH_ON) { *tile = 99; world_set_shade(shade); return; }
    if (id == BLOCK_CACTUS) { *tile = (face == 1 ? 69 : (face == 0 ? 71 : 70)); world_set_shade(shade); return; }
    if (id == BLOCK_REEDS) { *tile = 73; world_set_shade(shade); return; }
    if (id == BLOCK_JUKEBOX) { *tile = (face == 1 ? 75 : 74); world_set_shade(shade); return; }
    if (id == BLOCK_FENCE) { *tile = 4; world_set_shade(shade); return; }
    if (id == BLOCK_PUMPKIN || id == BLOCK_JACK_O_LANTERN) { *tile = (face == 1 || face == 0) ? 102 : (face == 3 ? 119 : 118); world_set_shade(shade); return; }
    if (id == BLOCK_PORTAL) { *tile = 14; world_set_shade(shade); return; }

    *tile = 2;
    world_set_shade(shade);
}

static void world_face_style_at(int id, int x, int y, int z, int face, int *tile) {
    world_style_set_pos(x, y, z);
    world_light_set_pos_for_face(x, y, z, face);
    /* BlockGrass.getBlockTexture(): when snow/snow block is above grass, side
       faces use tile 68 instead of the normal grass-side tile 3.  This is only
       a visual overlay choice; the block underneath remains grass. */
    if (id == BLOCK_GRASS && face >= 2 && face <= 5) {
        int above = flat_get_block(x, y + 1, z);
        if (above == BLOCK_SNOW_LAYER || above == BLOCK_SNOW_BLOCK) {
            float shade = (face == 2 || face == 3) ? 0.80f : 0.60f;
            *tile = 68;
            world_set_shade(shade);
            return;
        }
    }
    {
        int meta = flat_get_meta(x, y, z);
        int tile_idx = block_texture_resolve(id, meta, face);
        if (tile_idx >= 0) {
            float shade = world_face_base_shade(face);
            *tile = tile_idx;
            if (id == BLOCK_LEAVES || id == BLOCK_VINE) world_set_color_shade(java_foliage_color_at(x, z), shade);
            else if (id == BLOCK_TALL_GRASS || id == BLOCK_MYCELIUM) world_set_color_shade(java_grass_color_at(x, z), shade);
            else world_set_shade(shade);
            return;
        }
    }
    world_face_style(id, face, tile);
}

static int snow_layer_face_exposed(int x, int y, int z, int face) {
    if (face == 0) return 0;
    if (face == 1) return 1;
    int n = neighbor_for_face(x, y, z, face);
    if (n == BLOCK_SNOW_LAYER || n == BLOCK_SNOW_BLOCK) return 0;
    return !block_occludes_render_face(n);
}

static void emit_snow_layer_face_float(float x, float y, float z, int face) {
    const float h = 2.0f / 16.0f;
    float x0 = x, x1 = x + 1.0f;
    float y0 = y, y1 = y + h;
    float z0 = z, z1 = z + 1.0f;
    float u0, v0, u1, v1;
    terrain_tile_uv(66, &u0, &v0, &u1, &v1);
    if (face != 1) {
        float th = tex_terrain.h ? (float)tex_terrain.h : 256.0f;
        v1 = v0 + (2.0f / th);
    }

    if (face == 1) {
        world_set_shade(1.0f);
        world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y1, z1, u1, v1); world_tex_vertex(x0, y1, z1, u0, v1);
    } else if (face == 2) {
        world_set_shade(0.80f);
        world_tex_vertex(x1, y1, z0, u0, v0); world_tex_vertex(x0, y1, z0, u1, v0); world_tex_vertex(x0, y0, z0, u1, v1); world_tex_vertex(x1, y0, z0, u0, v1);
    } else if (face == 3) {
        world_set_shade(0.80f);
        world_tex_vertex(x0, y1, z1, u0, v0); world_tex_vertex(x1, y1, z1, u1, v0); world_tex_vertex(x1, y0, z1, u1, v1); world_tex_vertex(x0, y0, z1, u0, v1);
    } else if (face == 4) {
        world_set_shade(0.60f);
        world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x0, y1, z1, u1, v0); world_tex_vertex(x0, y0, z1, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    } else if (face == 5) {
        world_set_shade(0.60f);
        world_tex_vertex(x1, y1, z1, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x1, y0, z1, u0, v1);
    }
}

static void emit_snow_layer_block(int x, int y, int z) {
    for (int face = 0; face < 6; face++) {
        if (snow_layer_face_exposed(x, y, z, face)) emit_snow_layer_face_float((float)x, (float)y, (float)z, face);
    }
}

static void draw_snow_layer_block(float x, float y, float z) {
    glBegin(GL_QUADS);
    for (int face = 1; face < 6; face++) emit_snow_layer_face_float(x, y, z, face);
    glEnd();
    glColor4f(1, 1, 1, 1);
}

static int grass_side_overlay_face_needed(int x, int y, int z, int face) {
    if (!flat_fancy_grass_overlay_enabled()) return 0;
    if (face < 2 || face > 5) return 0;
    int above = flat_get_block(x, y + 1, z);
    if (above == BLOCK_SNOW_LAYER || above == BLOCK_SNOW_BLOCK) return 0;
    return flat_face_exposed_for_block(BLOCK_GRASS, x, y, z, face);
}

static void emit_grass_side_overlay_face(int x, int y, int z, int face) {
    float x0 = (float)x, x1 = (float)x + 1.0f;
    float y0 = (float)y, y1 = (float)y + 1.0f;
    float z0 = (float)z, z1 = (float)z + 1.0f;
    float u0, v0, u1, v1;
    int rgb = java_grass_color_at(x, z);
    float shade = world_face_base_shade(face);
    terrain_tile_uv(38, &u0, &v0, &u1, &v1);

    if (smooth_block_lighting_enabled(BLOCK_GRASS)) {
        float ao[4];
        uint32_t light[4];
        world_smooth_face_lights(x, y, z, face, ao, light);
        if (face == 2) {
            world_tex_vertex_lit(x1, y1, z0, u0, v0, rgb, shade, ao[0], light[0]);
            world_tex_vertex_lit(x0, y1, z0, u1, v0, rgb, shade, ao[1], light[1]);
            world_tex_vertex_lit(x0, y0, z0, u1, v1, rgb, shade, ao[2], light[2]);
            world_tex_vertex_lit(x1, y0, z0, u0, v1, rgb, shade, ao[3], light[3]);
        } else if (face == 3) {
            world_tex_vertex_lit(x0, y1, z1, u0, v0, rgb, shade, ao[0], light[0]);
            world_tex_vertex_lit(x1, y1, z1, u1, v0, rgb, shade, ao[1], light[1]);
            world_tex_vertex_lit(x1, y0, z1, u1, v1, rgb, shade, ao[2], light[2]);
            world_tex_vertex_lit(x0, y0, z1, u0, v1, rgb, shade, ao[3], light[3]);
        } else if (face == 4) {
            world_tex_vertex_lit(x0, y1, z0, u0, v0, rgb, shade, ao[0], light[0]);
            world_tex_vertex_lit(x0, y1, z1, u1, v0, rgb, shade, ao[1], light[1]);
            world_tex_vertex_lit(x0, y0, z1, u1, v1, rgb, shade, ao[2], light[2]);
            world_tex_vertex_lit(x0, y0, z0, u0, v1, rgb, shade, ao[3], light[3]);
        } else if (face == 5) {
            world_tex_vertex_lit(x1, y1, z1, u0, v0, rgb, shade, ao[0], light[0]);
            world_tex_vertex_lit(x1, y1, z0, u1, v0, rgb, shade, ao[1], light[1]);
            world_tex_vertex_lit(x1, y0, z0, u1, v1, rgb, shade, ao[2], light[2]);
            world_tex_vertex_lit(x1, y0, z1, u0, v1, rgb, shade, ao[3], light[3]);
        }
        return;
    }

    world_style_set_pos(x, y, z);
    world_light_set_pos_for_face(x, y, z, face);
    world_set_color_shade(rgb, shade);
    if (face == 2) {
        world_tex_vertex(x1, y1, z0, u0, v0); world_tex_vertex(x0, y1, z0, u1, v0); world_tex_vertex(x0, y0, z0, u1, v1); world_tex_vertex(x1, y0, z0, u0, v1);
    } else if (face == 3) {
        world_tex_vertex(x0, y1, z1, u0, v0); world_tex_vertex(x1, y1, z1, u1, v0); world_tex_vertex(x1, y0, z1, u1, v1); world_tex_vertex(x0, y0, z1, u0, v1);
    } else if (face == 4) {
        world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x0, y1, z1, u1, v0); world_tex_vertex(x0, y0, z1, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    } else if (face == 5) {
        world_tex_vertex(x1, y1, z1, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x1, y0, z1, u0, v1);
    }
}

static void emit_world_block_face_at(int id, int x, int y, int z, int face) {
    float x0 = (float)x, x1 = (float)x + 1.0f;
    float y0 = (float)y, y1 = (float)y + 1.0f;
    float z0 = (float)z, z1 = (float)z + 1.0f;
    float u0,v0,u1,v1;
    int tile = 2;
    if (id == BLOCK_LEAVES && flat_fancy_cutout_terrain_enabled() && neighbor_for_face(x, y, z, face) == BLOCK_LEAVES) {
        /* For the single canonical fancy-leaf interior quad, keep the local
           block light sample instead of sampling from the neighboring leaf.
           That keeps connected leaf faces the same color/brightness family as
           the exterior foliage instead of turning them into darker seams. */
        world_light_set_pos(x, y, z);
    }
    world_face_style_at(id, x, y, z, face, &tile);
    terrain_tile_uv(tile, &u0, &v0, &u1, &v1);
    if (smooth_block_lighting_enabled(id)) {
        float ao[4];
        uint32_t light[4];
        int rgb = world_face_tint_rgb(id, face);
        float shade = world_face_base_shade(face);
        world_smooth_face_lights(x, y, z, face, ao, light);
        if (face == 1) {
            world_tex_vertex_lit(x0, y1, z0, u0, v0, rgb, shade, ao[0], light[0]);
            world_tex_vertex_lit(x1, y1, z0, u1, v0, rgb, shade, ao[1], light[1]);
            world_tex_vertex_lit(x1, y1, z1, u1, v1, rgb, shade, ao[2], light[2]);
            world_tex_vertex_lit(x0, y1, z1, u0, v1, rgb, shade, ao[3], light[3]);
        } else if (face == 0) {
            world_tex_vertex_lit(x0, y0, z1, u0, v0, rgb, shade, ao[0], light[0]);
            world_tex_vertex_lit(x1, y0, z1, u1, v0, rgb, shade, ao[1], light[1]);
            world_tex_vertex_lit(x1, y0, z0, u1, v1, rgb, shade, ao[2], light[2]);
            world_tex_vertex_lit(x0, y0, z0, u0, v1, rgb, shade, ao[3], light[3]);
        } else if (face == 2) {
            world_tex_vertex_lit(x1, y1, z0, u0, v0, rgb, shade, ao[0], light[0]);
            world_tex_vertex_lit(x0, y1, z0, u1, v0, rgb, shade, ao[1], light[1]);
            world_tex_vertex_lit(x0, y0, z0, u1, v1, rgb, shade, ao[2], light[2]);
            world_tex_vertex_lit(x1, y0, z0, u0, v1, rgb, shade, ao[3], light[3]);
        } else if (face == 3) {
            world_tex_vertex_lit(x0, y1, z1, u0, v0, rgb, shade, ao[0], light[0]);
            world_tex_vertex_lit(x1, y1, z1, u1, v0, rgb, shade, ao[1], light[1]);
            world_tex_vertex_lit(x1, y0, z1, u1, v1, rgb, shade, ao[2], light[2]);
            world_tex_vertex_lit(x0, y0, z1, u0, v1, rgb, shade, ao[3], light[3]);
        } else if (face == 4) {
            world_tex_vertex_lit(x0, y1, z0, u0, v0, rgb, shade, ao[0], light[0]);
            world_tex_vertex_lit(x0, y1, z1, u1, v0, rgb, shade, ao[1], light[1]);
            world_tex_vertex_lit(x0, y0, z1, u1, v1, rgb, shade, ao[2], light[2]);
            world_tex_vertex_lit(x0, y0, z0, u0, v1, rgb, shade, ao[3], light[3]);
        } else if (face == 5) {
            world_tex_vertex_lit(x1, y1, z1, u0, v0, rgb, shade, ao[0], light[0]);
            world_tex_vertex_lit(x1, y1, z0, u1, v0, rgb, shade, ao[1], light[1]);
            world_tex_vertex_lit(x1, y0, z0, u1, v1, rgb, shade, ao[2], light[2]);
            world_tex_vertex_lit(x1, y0, z1, u0, v1, rgb, shade, ao[3], light[3]);
        }
        return;
    }
    if (face == 1) {
        world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y1, z1, u1, v1); world_tex_vertex(x0, y1, z1, u0, v1);
    } else if (face == 0) {
        world_tex_vertex(x0, y0, z1, u0, v0); world_tex_vertex(x1, y0, z1, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    } else if (face == 2) {
        world_tex_vertex(x1, y1, z0, u0, v0); world_tex_vertex(x0, y1, z0, u1, v0); world_tex_vertex(x0, y0, z0, u1, v1); world_tex_vertex(x1, y0, z0, u0, v1);
    } else if (face == 3) {
        world_tex_vertex(x0, y1, z1, u0, v0); world_tex_vertex(x1, y1, z1, u1, v0); world_tex_vertex(x1, y0, z1, u1, v1); world_tex_vertex(x0, y0, z1, u0, v1);
    } else if (face == 4) {
        world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x0, y1, z1, u1, v0); world_tex_vertex(x0, y0, z1, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    } else if (face == 5) {
        world_tex_vertex(x1, y1, z1, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x1, y0, z1, u0, v1);
    }
}

static void emit_world_block_face_float(int id, float x, float y, float z, int face) {
    float x0 = x, x1 = x + 1.0f;
    float y0 = y, y1 = y + 1.0f;
    float z0 = z, z1 = z + 1.0f;
    float u0,v0,u1,v1;
    int tile = 2;
    int bx = (int)floorf(x), by = (int)floorf(y), bz = (int)floorf(z);
    world_style_set_pos(bx, by, bz);
    world_light_set_pos_for_face(bx, by, bz, face);
    world_face_style_at(id, bx, by, bz, face, &tile);
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
    emit_world_block_face_at(id, x, y, z, face);
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

static float clamp_tex01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static void terrain_tile_uv_subrect(int tile, float su0, float sv0, float su1, float sv1,
                                    float *u0, float *v0, float *u1, float *v1) {
    float tu0, tv0, tu1, tv1;
    terrain_tile_uv(tile, &tu0, &tv0, &tu1, &tv1);
    su0 = clamp_tex01(su0); sv0 = clamp_tex01(sv0);
    su1 = clamp_tex01(su1); sv1 = clamp_tex01(sv1);
    *u0 = tu0 + (tu1 - tu0) * su0;
    *v0 = tv0 + (tv1 - tv0) * sv0;
    *u1 = tu0 + (tu1 - tu0) * su1;
    *v1 = tv0 + (tv1 - tv0) * sv1;
}

static int g_render_flip_texture_once = 0;

static void emit_cuboid_face_tile(float x0, float y0, float z0, float x1, float y1, float z1,
                                  float lx0, float ly0, float lz0, float lx1, float ly1, float lz1,
                                  int face, int tile) {
    float u0, v0, u1, v1;
    /* Java RenderBlocks does not stretch each partial cuboid face over a full
       terrain tile.  It crops the source 16x16 tile by the block bounds: a
       4x16 button face uses only 4 pixels of the texture, slab sides use the
       lower 8 pixels, etc. */
    if (face == 1 || face == 0) {
        terrain_tile_uv_subrect(tile, lx0, lz0, lx1, lz1, &u0, &v0, &u1, &v1);
    } else if (face == 2 || face == 3) {
        terrain_tile_uv_subrect(tile, lx0, 1.0f - ly1, lx1, 1.0f - ly0, &u0, &v0, &u1, &v1);
    } else {
        terrain_tile_uv_subrect(tile, lz0, 1.0f - ly1, lz1, 1.0f - ly0, &u0, &v0, &u1, &v1);
    }
    if (g_render_flip_texture_once) { float tu = u0; u0 = u1; u1 = tu; }

    if (face == 1) {
        world_set_shade(1.0f);
        world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y1, z1, u1, v1); world_tex_vertex(x0, y1, z1, u0, v1);
    } else if (face == 0) {
        world_set_shade(0.50f);
        world_tex_vertex(x0, y0, z1, u0, v0); world_tex_vertex(x1, y0, z1, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    } else if (face == 2) {
        world_set_shade(0.80f);
        world_tex_vertex(x1, y1, z0, u0, v0); world_tex_vertex(x0, y1, z0, u1, v0); world_tex_vertex(x0, y0, z0, u1, v1); world_tex_vertex(x1, y0, z0, u0, v1);
    } else if (face == 3) {
        world_set_shade(0.80f);
        world_tex_vertex(x0, y1, z1, u0, v0); world_tex_vertex(x1, y1, z1, u1, v0); world_tex_vertex(x1, y0, z1, u1, v1); world_tex_vertex(x0, y0, z1, u0, v1);
    } else if (face == 4) {
        world_set_shade(0.60f);
        world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x0, y1, z1, u1, v0); world_tex_vertex(x0, y0, z1, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    } else if (face == 5) {
        world_set_shade(0.60f);
        world_tex_vertex(x1, y1, z1, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x1, y0, z1, u0, v1);
    }
}

static void emit_cuboid_face_tile_auto(float x0, float y0, float z0, float x1, float y1, float z1, int face, int tile) {
    float bx = floorf(x0), by = floorf(y0), bz = floorf(z0);
    emit_cuboid_face_tile(x0, y0, z0, x1, y1, z1,
                          x0 - bx, y0 - by, z0 - bz, x1 - bx, y1 - by, z1 - bz, face, tile);
}

static void emit_cuboid_block_faces_lit(int id, float x0, float y0, float z0, float x1, float y1, float z1,
                                          float lx0, float ly0, float lz0, float lx1, float ly1, float lz1) {
    for (int face = 0; face < 6; face++) {
        int tile = 2;
        int meta = 0;
        int tile_idx = block_texture_resolve(id, meta, face);
        world_style_set_pos((int)floorf(x0), (int)floorf(y0), (int)floorf(z0));
        if (tile_idx >= 0) {
            float shade = world_face_base_shade(face);
            tile = tile_idx;
            if (id == BLOCK_LEAVES || id == BLOCK_VINE) world_set_color_shade(java_foliage_color_at(g_world_style_x, g_world_style_z), shade);
            else if (id == BLOCK_TALL_GRASS || id == BLOCK_MYCELIUM) world_set_color_shade(java_grass_color_at(g_world_style_x, g_world_style_z), shade);
            else world_set_shade(shade);
        } else {
            world_face_style(id, face, &tile);
        }
        emit_cuboid_face_tile(x0, y0, z0, x1, y1, z1,
                              lx0, ly0, lz0, lx1, ly1, lz1, face, tile);
    }
}

static void emit_cuboid_block_faces(int id, float x0, float y0, float z0, float x1, float y1, float z1) {
    float bx = floorf(x0), by = floorf(y0), bz = floorf(z0);
    float lx0 = x0 - bx, ly0 = y0 - by, lz0 = z0 - bz;
    float lx1 = x1 - bx, ly1 = y1 - by, lz1 = z1 - bz;
    emit_cuboid_block_faces_lit(id, x0, y0, z0, x1, y1, z1, lx0, ly0, lz0, lx1, ly1, lz1);
}

static void emit_block_cuboid_faces(int id, float x, float y, float z,
                                              float x0, float y0, float z0, float x1, float y1, float z1) {
    emit_cuboid_block_faces_lit(id, x + x0, y + y0, z + z0, x + x1, y + y1, z + z1,
                                x0, y0, z0, x1, y1, z1);
}

static void draw_cuboid_model_for_block(int id, float x, float y, float z, float x0, float y0, float z0, float x1, float y1, float z1) {
    glBegin(GL_QUADS);
    emit_block_cuboid_faces(id, x, y, z, x0, y0, z0, x1, y1, z1);
    glEnd();
}

static void emit_cuboid_model_faces_tile(float x, float y, float z,
                                         float x0, float y0, float z0, float x1, float y1, float z1,
                                         int tile) {
    for (int face = 0; face < 6; face++) {
        emit_cuboid_face_tile(x + x0, y + y0, z + z0, x + x1, y + y1, z + z1,
                              x0, y0, z0, x1, y1, z1, face, tile);
    }
}

static void draw_cuboid_model_tile(float x, float y, float z,
                                   float x0, float y0, float z0, float x1, float y1, float z1, int tile) {
    glBegin(GL_QUADS);
    emit_cuboid_model_faces_tile(x, y, z, x0, y0, z0, x1, y1, z1, tile);
    glEnd();
}

static void draw_block_item_model(int id, float x, float y, float z) {
    int pushed_fullbright = 0;
    GLint old_shade_model = pex_gl_save_shade_model();
    if (g_force_fullbright_item_model <= 0) { g_force_fullbright_item_model++; pushed_fullbright = 1; }
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glColor4f(1,1,1,1);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    pex_gl_shade_model_smooth();
    /* Disable culling here too: the same legacy cuboid emitter is used for
       held/dropped block items, and OpenGL culling makes their faces disappear
       or appear inverted while D3D looks fine. */
    glDisable(GL_CULL_FACE);
    /* RenderItem/ItemRenderer must not turn non-full block items into solid cubes.
       This path is used for dropped block entities and first-person held blocks. */
    if (id == BLOCK_SLAB) {
        draw_cuboid_model_for_block(id, x, y, z, 0, 0, 0, 1, 0.5f, 1);
        goto done;
    }
    if (id == BLOCK_SNOW_LAYER) {
        draw_cuboid_model_for_block(id, x, y, z, 0, 0, 0, 1, 2.0f / 16.0f, 1);
        goto done;
    }
    if (id == BLOCK_STONE_PRESSURE_PLATE || id == BLOCK_WOOD_PRESSURE_PLATE) {
        draw_cuboid_model_for_block(id, x, y, z, 1.0f/16.0f, 0, 1.0f/16.0f, 15.0f/16.0f, 1.0f/16.0f, 15.0f/16.0f);
        goto done;
    }
    if (id == BLOCK_STONE_BUTTON) {
        draw_cuboid_model_for_block(id, x, y, z, 5.0f/16.0f, 5.0f/16.0f, 7.0f/16.0f, 11.0f/16.0f, 11.0f/16.0f, 9.0f/16.0f);
        goto done;
    }
    if (id == BLOCK_CACTUS) {
        draw_cuboid_model_for_block(id, x, y, z, 1.0f/16.0f, 0, 1.0f/16.0f, 15.0f/16.0f, 1, 15.0f/16.0f);
        goto done;
    }
    if (id == BLOCK_CHEST) {
        draw_chest_block(x, y, z);
        goto done;
    }
    if (id == BLOCK_FENCE) {
        glBegin(GL_QUADS);
        emit_block_cuboid_faces(id, x, y, z, 0.375f, 0.0f, 0.375f, 0.625f, 1.0f, 0.625f);
        emit_block_cuboid_faces(id, x, y, z, 0.0f, 0.35f, 0.4375f, 1.0f, 0.55f, 0.5625f);
        emit_block_cuboid_faces(id, x, y, z, 0.0f, 0.70f, 0.4375f, 1.0f, 0.90f, 0.5625f);
        emit_block_cuboid_faces(id, x, y, z, 0.4375f, 0.35f, 0.0f, 0.5625f, 0.55f, 1.0f);
        emit_block_cuboid_faces(id, x, y, z, 0.4375f, 0.70f, 0.0f, 0.5625f, 0.90f, 1.0f);
        glEnd();
        goto done;
    }
    if (id == BLOCK_WOOD_STAIRS || id == BLOCK_COBBLE_STAIRS) {
        glBegin(GL_QUADS);
        emit_block_cuboid_faces(id, x, y, z, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
        emit_block_cuboid_faces(id, x, y, z, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f);
        glEnd();
        goto done;
    }
    draw_world_block_id(id, x, y, z);
    goto done;
done:
    if (pushed_fullbright) g_force_fullbright_item_model--;
    pex_gl_restore_shade_model(old_shade_model);
    glDisable(GL_CULL_FACE);
    glColor4f(1,1,1,1);
}



static void entity_texture_uv(Texture *tex, int px, int py, float *u, float *v) {
    float tw = tex && tex->w ? (float)tex->w : 64.0f;
    float th = tex && tex->h ? (float)tex->h : 32.0f;
    *u = (float)px / tw;
    *v = (float)py / th;
}

static void entity_tex_vertex(Texture *tex, float x, float y, float z, int px, int py) {
    float u, v;
    entity_texture_uv(tex, px, py, &u, &v);
    world_tex_vertex(x, y, z, u, v);
}

static void draw_entity_chest_box(Texture *tex, float x0, float y0, float z0, float x1, float y1, float z1,
                                  int model_w, int model_h, int model_d, int off_x, int off_y, int axis_x) {
    int u_side0 = off_x;
    int u_front = off_x + model_d;
    int u_side1 = off_x + model_d + model_w;
    int u_back  = off_x + model_d + model_w + model_d;
    int v_top = off_y;
    int v_side = off_y + model_d;
    int v_bot = off_y + model_d + model_h;

    world_set_shade(1.0f);
    if (axis_x) {
        entity_tex_vertex(tex, x0,y1,z0, u_front, v_top); entity_tex_vertex(tex, x1,y1,z0, u_front+model_w, v_top); entity_tex_vertex(tex, x1,y1,z1, u_front+model_w, v_top+model_d); entity_tex_vertex(tex, x0,y1,z1, u_front, v_top+model_d);
        world_set_shade(0.50f);
        entity_tex_vertex(tex, x0,y0,z1, u_side1, v_top); entity_tex_vertex(tex, x1,y0,z1, u_side1+model_w, v_top); entity_tex_vertex(tex, x1,y0,z0, u_side1+model_w, v_top+model_d); entity_tex_vertex(tex, x0,y0,z0, u_side1, v_top+model_d);
        world_set_shade(0.80f);
        entity_tex_vertex(tex, x0,y1,z1, u_front, v_side); entity_tex_vertex(tex, x1,y1,z1, u_front+model_w, v_side); entity_tex_vertex(tex, x1,y0,z1, u_front+model_w, v_bot); entity_tex_vertex(tex, x0,y0,z1, u_front, v_bot);
        entity_tex_vertex(tex, x1,y1,z0, u_back, v_side); entity_tex_vertex(tex, x0,y1,z0, u_back+model_w, v_side); entity_tex_vertex(tex, x0,y0,z0, u_back+model_w, v_bot); entity_tex_vertex(tex, x1,y0,z0, u_back, v_bot);
        world_set_shade(0.60f);
        entity_tex_vertex(tex, x0,y1,z0, u_side0, v_side); entity_tex_vertex(tex, x0,y1,z1, u_side0+model_d, v_side); entity_tex_vertex(tex, x0,y0,z1, u_side0+model_d, v_bot); entity_tex_vertex(tex, x0,y0,z0, u_side0, v_bot);
        entity_tex_vertex(tex, x1,y1,z1, u_side1, v_side); entity_tex_vertex(tex, x1,y1,z0, u_side1+model_d, v_side); entity_tex_vertex(tex, x1,y0,z0, u_side1+model_d, v_bot); entity_tex_vertex(tex, x1,y0,z1, u_side1, v_bot);
    } else {
        entity_tex_vertex(tex, x0,y1,z0, u_front, v_top); entity_tex_vertex(tex, x1,y1,z0, u_front, v_top+model_d); entity_tex_vertex(tex, x1,y1,z1, u_front+model_w, v_top+model_d); entity_tex_vertex(tex, x0,y1,z1, u_front+model_w, v_top);
        world_set_shade(0.50f);
        entity_tex_vertex(tex, x0,y0,z1, u_side1, v_top); entity_tex_vertex(tex, x1,y0,z1, u_side1, v_top+model_d); entity_tex_vertex(tex, x1,y0,z0, u_side1+model_w, v_top+model_d); entity_tex_vertex(tex, x0,y0,z0, u_side1+model_w, v_top);
        world_set_shade(0.80f);
        entity_tex_vertex(tex, x1,y1,z0, u_side0, v_side); entity_tex_vertex(tex, x1,y1,z1, u_side0+model_w, v_side); entity_tex_vertex(tex, x1,y0,z1, u_side0+model_w, v_bot); entity_tex_vertex(tex, x1,y0,z0, u_side0, v_bot);
        entity_tex_vertex(tex, x0,y1,z1, u_side1, v_side); entity_tex_vertex(tex, x0,y1,z0, u_side1+model_w, v_side); entity_tex_vertex(tex, x0,y0,z0, u_side1+model_w, v_bot); entity_tex_vertex(tex, x0,y0,z1, u_side1, v_bot);
        world_set_shade(0.60f);
        entity_tex_vertex(tex, x0,y1,z0, u_back, v_side); entity_tex_vertex(tex, x1,y1,z0, u_back+model_d, v_side); entity_tex_vertex(tex, x1,y0,z0, u_back+model_d, v_bot); entity_tex_vertex(tex, x0,y0,z0, u_back, v_bot);
        entity_tex_vertex(tex, x1,y1,z1, u_front, v_side); entity_tex_vertex(tex, x0,y1,z1, u_front+model_d, v_side); entity_tex_vertex(tex, x0,y0,z1, u_front+model_d, v_bot); entity_tex_vertex(tex, x1,y0,z1, u_front, v_bot);
    }
}

static void draw_entity_chest_model(Texture *tex, float x0, float y0, float z0, float x1, float z1, int wide, int axis_x) {
    if (!tex || !tex->id) return;
    glBindTexture(GL_TEXTURE_2D, tex->id);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    glBegin(GL_QUADS);
    int model_w = wide ? 30 : 14;
    int model_d = 14;
    draw_entity_chest_box(tex, x0, y0, z0, x1, y0 + 10.0f / 16.0f, z1, model_w, 10, model_d, 0, 19, axis_x);
    draw_entity_chest_box(tex, x0, y0 + 9.0f / 16.0f, z0, x1, y0 + 14.0f / 16.0f, z1, model_w, 5, model_d, 0, 0, axis_x);
    float cx0 = (x0 + x1) * 0.5f - 1.0f / 16.0f;
    float cx1 = (x0 + x1) * 0.5f + 1.0f / 16.0f;
    float kz0 = axis_x ? (z1 - 1.0f / 16.0f) : ((z0 + z1) * 0.5f - 1.0f / 32.0f);
    float kz1 = axis_x ? z1 : ((z0 + z1) * 0.5f + 1.0f / 32.0f);
    float kx0 = axis_x ? cx0 : (x1 - 1.0f / 16.0f);
    float kx1 = axis_x ? cx1 : x1;
    draw_entity_chest_box(tex, kx0, y0 + 7.0f / 16.0f, kz0, kx1, y0 + 11.0f / 16.0f, kz1, 2, 4, 1, 0, 0, axis_x);
    glEnd();
    glDisable(GL_ALPHA_TEST);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glColor4f(1,1,1,1);
}

static void emit_chest_face_tile_world(float x0, float y0, float z0, float x1, float y1, float z1, int face, int tile) {
    float u0, v0, u1, v1;
    terrain_tile_uv(tile, &u0, &v0, &u1, &v1);
    if (face == 1) {
        world_set_shade(1.0f);
        world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y1, z1, u1, v1); world_tex_vertex(x0, y1, z1, u0, v1);
    } else if (face == 0) {
        world_set_shade(0.50f);
        world_tex_vertex(x0, y0, z1, u0, v0); world_tex_vertex(x1, y0, z1, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    } else if (face == 2) {
        world_set_shade(0.80f);
        world_tex_vertex(x1, y1, z0, u0, v0); world_tex_vertex(x0, y1, z0, u1, v0); world_tex_vertex(x0, y0, z0, u1, v1); world_tex_vertex(x1, y0, z0, u0, v1);
    } else if (face == 3) {
        world_set_shade(0.80f);
        world_tex_vertex(x0, y1, z1, u0, v0); world_tex_vertex(x1, y1, z1, u1, v0); world_tex_vertex(x1, y0, z1, u1, v1); world_tex_vertex(x0, y0, z1, u0, v1);
    } else if (face == 4) {
        world_set_shade(0.60f);
        world_tex_vertex(x0, y1, z0, u0, v0); world_tex_vertex(x0, y1, z1, u1, v0); world_tex_vertex(x0, y0, z1, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
    } else if (face == 5) {
        world_set_shade(0.60f);
        world_tex_vertex(x1, y1, z1, u0, v0); world_tex_vertex(x1, y1, z0, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x1, y0, z1, u0, v1);
    }
}

static void draw_large_chest_terrain_span(float x0, float y0, float z0, float x1, float y1, float z1, int axis_x) {
    int lt = large_chest_left_tile();   /* terrain.png tile 41 in classic atlas */
    int rt = large_chest_right_tile();  /* terrain.png tile 42 in classic atlas */
    int top = chest_top_tile();
    int side = chest_side_tile();
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glBegin(GL_QUADS);

    if (axis_x) {
        float xm = (x0 + x1) * 0.5f;
        /* Keep the 41->42 strip in atlas order on the front face only.  The
           previous pass mirrored the large-front strip onto the rear face too,
           which made the chest read as flipped from one side. */
        emit_chest_face_tile_world(x0, y0, z0, xm, y1, z1, 1, top);
        emit_chest_face_tile_world(xm, y0, z0, x1, y1, z1, 1, top);
        emit_chest_face_tile_world(x0, y0, z0, xm, y1, z1, 0, top);
        emit_chest_face_tile_world(xm, y0, z0, x1, y1, z1, 0, top);
        emit_chest_face_tile_world(x0, y0, z1, xm, y1, z1, 3, lt);
        emit_chest_face_tile_world(xm, y0, z1, x1, y1, z1, 3, rt);
        /* The opposite face is UV-mirrored by the face-2 vertex order, so swap
           the two terrain tiles to keep the large-chest latch at the center
           instead of the outside edges. */
        emit_chest_face_tile_world(x0, y0, z0, xm, y1, z0, 2, rt);
        emit_chest_face_tile_world(xm, y0, z0, x1, y1, z0, 2, lt);
        emit_chest_face_tile_world(x0, y0, z0, x0, y1, z1, 4, side);
        emit_chest_face_tile_world(x1, y0, z0, x1, y1, z1, 5, side);
    } else {
        float zm = (z0 + z1) * 0.5f;
        emit_chest_face_tile_world(x0, y0, z0, x1, y1, zm, 1, top);
        emit_chest_face_tile_world(x0, y0, zm, x1, y1, z1, 1, top);
        emit_chest_face_tile_world(x0, y0, z0, x1, y1, zm, 0, top);
        emit_chest_face_tile_world(x0, y0, zm, x1, y1, z1, 0, top);
        /* Face 5 is UV-mirrored along Z, so use 42 then 41 there.  Face 4
           keeps the natural atlas order. */
        emit_chest_face_tile_world(x1, y0, z0, x1, y1, zm, 5, rt);
        emit_chest_face_tile_world(x1, y0, zm, x1, y1, z1, 5, lt);
        emit_chest_face_tile_world(x0, y0, z0, x0, y1, zm, 4, lt);
        emit_chest_face_tile_world(x0, y0, zm, x0, y1, z1, 4, rt);
        emit_chest_face_tile_world(x0, y0, z0, x1, y1, z0, 2, side);
        emit_chest_face_tile_world(x0, y0, z1, x1, y1, z1, 3, side);
    }
    glEnd();
}

static void draw_connected_chest_span(float x0, float y0, float z0, float x1, float y1, float z1, int axis) {
    /* Single chests still use the normal single chest terrain tiles.  Large chests
       use the terrain.png large chest strip below the single chest texture strip. */
    if ((axis && (x1 - x0) > 1.2f) || (!axis && (z1 - z0) > 1.2f)) {
        draw_large_chest_terrain_span(x0, y0, z0, x1, y1, z1, axis);
        return;
    }
    draw_chest_block(x0 - 1.0f / 16.0f, y0, z0 - 1.0f / 16.0f);
}

static void draw_connected_chest_block(int x, int y, int z) {
    /* Render double chests once, as one connected model, and never stretch a single
       terrain tile across the whole pair.  Each half gets its own 16x16 tile span. */
    int east = flat_get_block(x + 1, y, z) == BLOCK_CHEST;
    int west = flat_get_block(x - 1, y, z) == BLOCK_CHEST;
    int south = flat_get_block(x, y, z + 1) == BLOCK_CHEST;
    int north = flat_get_block(x, y, z - 1) == BLOCK_CHEST;
    if (west || north) return;

    float inset = 1.0f / 16.0f;
    float x0 = (float)x + inset, x1 = (float)x + 1.0f - inset;
    float z0 = (float)z + inset, z1 = (float)z + 1.0f - inset;
    int axis_x = 0;
    if (east) { x1 = (float)x + 2.0f - inset; axis_x = 1; }
    else if (south) { z1 = (float)z + 2.0f - inset; axis_x = 0; }
    float y0 = (float)y, y1 = (float)y + 14.0f / 16.0f;
    draw_connected_chest_span(x0, y0, z0, x1, y1, z1, axis_x);
}
static void emit_flat_quad_tile(float x0, float y, float z0, float x1, float z1, int tile) {
    float u0, v0, u1, v1;
    terrain_tile_uv(tile, &u0, &v0, &u1, &v1);
    world_set_shade(1.0f);
    world_tex_vertex(x0, y, z0, u0, v0); world_tex_vertex(x1, y, z0, u1, v0); world_tex_vertex(x1, y, z1, u1, v1); world_tex_vertex(x0, y, z1, u0, v1);
}

static void rot_x(float *y, float *z, float a) {
    float cy = cosf(a), sy = sinf(a);
    float oy = *y, oz = *z;
    *y = oy * cy - oz * sy;
    *z = oy * sy + oz * cy;
}

static void rot_y(float *x, float *z, float a) {
    float cy = cosf(a), sy = sinf(a);
    float ox = *x, oz = *z;
    *x = ox * cy + oz * sy;
    *z = -ox * sy + oz * cy;
}

static void emit_textured_quad_pts(float p[8][3], int a, int b, int c, int d, int tile,
                                   float su0, float sv0, float su1, float sv1) {
    float u0, v0, u1, v1;
    terrain_tile_uv_subrect(tile, su0, sv0, su1, sv1, &u0, &v0, &u1, &v1);
    world_tex_vertex(p[a][0], p[a][1], p[a][2], u0, v1);
    world_tex_vertex(p[b][0], p[b][1], p[b][2], u1, v1);
    world_tex_vertex(p[c][0], p[c][1], p[c][2], u1, v0);
    world_tex_vertex(p[d][0], p[d][1], p[d][2], u0, v0);
}

static float vec3_len(float x, float y, float z) {
    return sqrtf(x * x + y * y + z * z);
}

static void emit_lever_prism(float ax, float ay, float az, float bx, float by, float bz, float half, int tile) {
    float dx = bx - ax, dy = by - ay, dz = bz - az;
    float dl = vec3_len(dx, dy, dz);
    if (dl < 0.0001f) return;
    dx /= dl; dy /= dl; dz /= dl;

    float rx = 0.0f, ry = 1.0f, rz = 0.0f;
    if (fabsf(dy) > 0.9f) { rx = 1.0f; ry = 0.0f; rz = 0.0f; }

    float ux = dy * rz - dz * ry;
    float uy = dz * rx - dx * rz;
    float uz = dx * ry - dy * rx;
    float ul = vec3_len(ux, uy, uz);
    if (ul < 0.0001f) return;
    ux = ux / ul * half; uy = uy / ul * half; uz = uz / ul * half;

    float vx = dy * uz - dz * uy;
    float vy = dz * ux - dx * uz;
    float vz = dx * uy - dy * ux;
    float vl = vec3_len(vx, vy, vz);
    if (vl < 0.0001f) return;
    vx = vx / vl * half; vy = vy / vl * half; vz = vz / vl * half;

    float pts[8][3] = {
        {ax - ux - vx, ay - uy - vy, az - uz - vz},
        {ax + ux - vx, ay + uy - vy, az + uz - vz},
        {ax + ux + vx, ay + uy + vy, az + uz + vz},
        {ax - ux + vx, ay - uy + vy, az - uz + vz},
        {bx - ux - vx, by - uy - vy, bz - uz - vz},
        {bx + ux - vx, by + uy - vy, bz + uz - vz},
        {bx + ux + vx, by + uy + vy, bz + uz + vz},
        {bx - ux + vx, by - uy + vy, bz - uz + vz}
    };

    world_set_shade(1.0f);
    emit_textured_quad_pts(pts, 0,1,2,3, tile, 7.0f/16.0f, 6.0f/16.0f, 9.0f/16.0f, 8.0f/16.0f);
    emit_textured_quad_pts(pts, 7,6,5,4, tile, 7.0f/16.0f, 6.0f/16.0f, 9.0f/16.0f, 8.0f/16.0f);
    emit_textured_quad_pts(pts, 1,0,4,5, tile, 7.0f/16.0f, 6.0f/16.0f, 9.0f/16.0f, 15.0f/16.0f);
    emit_textured_quad_pts(pts, 2,1,5,6, tile, 7.0f/16.0f, 6.0f/16.0f, 9.0f/16.0f, 15.0f/16.0f);
    emit_textured_quad_pts(pts, 3,2,6,7, tile, 7.0f/16.0f, 6.0f/16.0f, 9.0f/16.0f, 15.0f/16.0f);
    emit_textured_quad_pts(pts, 0,3,7,4, tile, 7.0f/16.0f, 6.0f/16.0f, 9.0f/16.0f, 15.0f/16.0f);
}

static void emit_lever_handle_model(int x, int y, int z, int meta) {
    int side = meta & 7;
    int powered = (meta & 8) != 0;
    int tile = 96;
    float h = 1.0f / 16.0f;
    float ax = (float)x + 0.5f;
    float ay = (float)y + 0.5f;
    float az = (float)z + 0.5f;
    float bx = ax, by = ay, bz = az;

    if (side == 5 || side == 6) {
        /* Floor lever: small 2x2px stick anchored on the cobblestone plate.
           Metadata 5/6 chooses which horizontal axis the handle leans along;
           power flips the lean direction. */
        float dir = powered ? 1.0f : -1.0f;
        ax = (float)x + 0.5f;
        ay = (float)y + 3.0f / 16.0f;
        az = (float)z + 0.5f;
        bx = ax;
        by = (float)y + 11.0f / 16.0f;
        bz = az;
        if (side == 6) bx += dir * 4.0f / 16.0f;
        else bz += dir * 4.0f / 16.0f;
    } else {
        float nx = 0.0f, nz = 0.0f;
        if (side == 1) { nx =  1.0f; ax = (float)x + 3.0f / 16.0f; }
        else if (side == 2) { nx = -1.0f; ax = (float)x + 13.0f / 16.0f; }
        else if (side == 3) { nz =  1.0f; az = (float)z + 3.0f / 16.0f; }
        else { nz = -1.0f; az = (float)z + 13.0f / 16.0f; }
        ay = (float)y + 0.5f;
        bx = ax + nx * 6.0f / 16.0f;
        bz = az + nz * 6.0f / 16.0f;
        by = ay + (powered ? -4.0f : 4.0f) / 16.0f;
    }

    emit_lever_prism(ax, ay, az, bx, by, bz, h, tile);
}

static void draw_lever_block_model(int x, int y, int z) {
    int meta = flat_get_meta(x, y, z);
    int side = meta & 7;
    /* Base plate uses cobblestone exactly like RenderBlocks.renderBlockLever's override texture. */
    if (side == 5) draw_cuboid_model_tile((float)x,(float)y,(float)z, 0.5f-3.0f/16.0f,0.0f,0.5f-0.25f, 0.5f+3.0f/16.0f,3.0f/16.0f,0.5f+0.25f, 16);
    else if (side == 6) draw_cuboid_model_tile((float)x,(float)y,(float)z, 0.5f-0.25f,0.0f,0.5f-3.0f/16.0f, 0.5f+0.25f,3.0f/16.0f,0.5f+3.0f/16.0f, 16);
    else if (side == 4) draw_cuboid_model_tile((float)x,(float)y,(float)z, 0.5f-3.0f/16.0f,0.5f-0.25f,1.0f-3.0f/16.0f, 0.5f+3.0f/16.0f,0.5f+0.25f,1.0f, 16);
    else if (side == 3) draw_cuboid_model_tile((float)x,(float)y,(float)z, 0.5f-3.0f/16.0f,0.5f-0.25f,0.0f, 0.5f+3.0f/16.0f,0.5f+0.25f,3.0f/16.0f, 16);
    else if (side == 2) draw_cuboid_model_tile((float)x,(float)y,(float)z, 1.0f-3.0f/16.0f,0.5f-0.25f,0.5f-3.0f/16.0f, 1.0f,0.5f+0.25f,0.5f+3.0f/16.0f, 16);
    else draw_cuboid_model_tile((float)x,(float)y,(float)z, 0.0f,0.5f-0.25f,0.5f-3.0f/16.0f, 3.0f/16.0f,0.5f+0.25f,0.5f+3.0f/16.0f, 16);
    glBegin(GL_QUADS);
    emit_lever_handle_model(x, y, z, meta);
    glEnd();
}

static int torch_tile_for_id(int id) {
    if (id == BLOCK_REDSTONE_TORCH_OFF) return 115;
    if (id == BLOCK_REDSTONE_TORCH_ON) return 99;
    return 80;
}

static void emit_torch_at_angle(int id, double x, double y, double z, double ax, double az) {
    int tile = torch_tile_for_id(id);
    float tw = tex_terrain.w ? (float)tex_terrain.w : 256.0f;
    float th = tex_terrain.h ? (float)tex_terrain.h : 256.0f;
    float u0 = (float)((tile & 15) << 4) / tw;
    float v0 = (float)(tile & 240) / th;
    float u1 = (float)(((tile & 15) << 4) + 15.99f) / tw;
    float v1 = (float)((tile & 240) + 15.99f) / th;
    float cu0 = u0 + 7.0f / tw;
    float cv0 = v0 + 6.0f / th;
    float cu1 = u0 + 9.0f / tw;
    float cv1 = v0 + 8.0f / th;

    x += 0.5;
    z += 0.5;
    double x0 = x - 0.5;
    double x1 = x + 0.5;
    double z0 = z - 0.5;
    double z1 = z + 0.5;
    double r = 1.0 / 16.0;
    double cap_y = 0.625;

    world_set_shade(1.0f);
    world_tex_vertex((float)(x + ax * (1.0 - cap_y) - r), (float)(y + cap_y), (float)(z + az * (1.0 - cap_y) - r), cu0, cv0);
    world_tex_vertex((float)(x + ax * (1.0 - cap_y) - r), (float)(y + cap_y), (float)(z + az * (1.0 - cap_y) + r), cu0, cv1);
    world_tex_vertex((float)(x + ax * (1.0 - cap_y) + r), (float)(y + cap_y), (float)(z + az * (1.0 - cap_y) + r), cu1, cv1);
    world_tex_vertex((float)(x + ax * (1.0 - cap_y) + r), (float)(y + cap_y), (float)(z + az * (1.0 - cap_y) - r), cu1, cv0);

    world_tex_vertex((float)(x - r),       (float)(y + 1.0), (float)z0,        u0, v0);
    world_tex_vertex((float)(x - r + ax),  (float)y,         (float)(z0 + az), u0, v1);
    world_tex_vertex((float)(x - r + ax),  (float)y,         (float)(z1 + az), u1, v1);
    world_tex_vertex((float)(x - r),       (float)(y + 1.0), (float)z1,        u1, v0);

    world_tex_vertex((float)(x + r),       (float)(y + 1.0), (float)z1,        u0, v0);
    world_tex_vertex((float)(x + r + ax),  (float)y,         (float)(z1 + az), u0, v1);
    world_tex_vertex((float)(x + r + ax),  (float)y,         (float)(z0 + az), u1, v1);
    world_tex_vertex((float)(x + r),       (float)(y + 1.0), (float)z0,        u1, v0);

    world_tex_vertex((float)x0,       (float)(y + 1.0), (float)(z + r),      u0, v0);
    world_tex_vertex((float)(x0 + ax),(float)y,         (float)(z + r + az),u0, v1);
    world_tex_vertex((float)(x1 + ax),(float)y,         (float)(z + r + az),u1, v1);
    world_tex_vertex((float)x1,       (float)(y + 1.0), (float)(z + r),      u1, v0);

    world_tex_vertex((float)x1,       (float)(y + 1.0), (float)(z - r),      u0, v0);
    world_tex_vertex((float)(x1 + ax),(float)y,         (float)(z - r + az),u0, v1);
    world_tex_vertex((float)(x0 + ax),(float)y,         (float)(z - r + az),u1, v1);
    world_tex_vertex((float)x0,       (float)(y + 1.0), (float)(z - r),      u1, v0);
}

static void draw_torch_block_model(int id, int x, int y, int z) {
    int meta = flat_get_meta(x, y, z) & 7;
    int pushed_fullbright = 0;
    if (flat_block_light_value_for_id(id) > 0) { g_force_fullbright_item_model++; pushed_fullbright = 1; }
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    glBegin(GL_QUADS);
    if (meta == 1) emit_torch_at_angle(id, (double)x - 0.1, (double)y + 0.2, (double)z, -0.4, 0.0);
    else if (meta == 2) emit_torch_at_angle(id, (double)x + 0.1, (double)y + 0.2, (double)z, 0.4, 0.0);
    else if (meta == 3) emit_torch_at_angle(id, (double)x, (double)y + 0.2, (double)z - 0.1, 0.0, -0.4);
    else if (meta == 4) emit_torch_at_angle(id, (double)x, (double)y + 0.2, (double)z + 0.1, 0.0, 0.4);
    else emit_torch_at_angle(id, (double)x, (double)y, (double)z, 0.0, 0.0);
    glEnd();
    glDisable(GL_ALPHA_TEST);
    if (pushed_fullbright) g_force_fullbright_item_model--;
}

static void draw_ladder_block_model(int x, int y, int z) {
    int meta = flat_get_meta(x, y, z);
    if (meta < 2 || meta > 5) meta = 3;
    float u0, v0, u1, v1; terrain_tile_uv(83, &u0, &v0, &u1, &v1);
    float e = 1.0f / 16.0f;
    glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, 0.5f);
    glBegin(GL_QUADS); world_set_shade(1.0f);
    if (meta == 2) { float zz = (float)z + 1.0f - e; world_tex_vertex(x+1,y+1,zz,u0,v0); world_tex_vertex(x,y+1,zz,u1,v0); world_tex_vertex(x,y,zz,u1,v1); world_tex_vertex(x+1,y,zz,u0,v1); }
    else if (meta == 3) { float zz = (float)z + e; world_tex_vertex(x,y+1,zz,u0,v0); world_tex_vertex(x+1,y+1,zz,u1,v0); world_tex_vertex(x+1,y,zz,u1,v1); world_tex_vertex(x,y,zz,u0,v1); }
    else if (meta == 4) { float xx = (float)x + 1.0f - e; world_tex_vertex(xx,y+1,z,u0,v0); world_tex_vertex(xx,y+1,z+1,u1,v0); world_tex_vertex(xx,y,z+1,u1,v1); world_tex_vertex(xx,y,z,u0,v1); }
    else { float xx = (float)x + e; world_tex_vertex(xx,y+1,z+1,u0,v0); world_tex_vertex(xx,y+1,z,u1,v0); world_tex_vertex(xx,y,z,u1,v1); world_tex_vertex(xx,y,z+1,u0,v1); }
    glEnd(); glDisable(GL_ALPHA_TEST);
}

static int door_java_state(int meta) {
    return (meta & 4) == 0 ? ((meta - 1) & 3) : (meta & 3);
}

static int door_java_texture_for_face(int id, int meta, int face, int *flip) {
    int base = (id == BLOCK_IRON_DOOR) ? 98 : 97;
    if (flip) *flip = 0;
    if (face == 0 || face == 1) return base;
    int state = door_java_state(meta);
    if (((state == 0 || state == 2) ? 1 : 0) ^ (face <= 3)) return base;
    int v = state / 2 + ((face & 1) ^ state);
    v += (meta & 4) / 4;
    int tile = base - (meta & 8) * 2;
    if (v & 1) { if (flip) *flip = 1; }
    return tile;
}

static void draw_door_block_model(int id, int x, int y, int z) {
    int meta = flat_get_meta(x, y, z);
    int upper = door_meta_is_upper(meta);
    int ly = upper ? y - 1 : y;
    int lower_meta = flat_get_meta(x, ly, z);
    int state = door_java_state(lower_meta);
    float t = 3.0f / 16.0f;
    float x0 = (float)x, x1 = (float)x + 1.0f;
    float y0 = (float)y, y1 = (float)y + 1.0f;
    float z0 = (float)z, z1 = (float)z + 1.0f;

    if (state == 0) z1 = z0 + t;
    else if (state == 1) x0 = x1 - t;
    else if (state == 2) z0 = z1 - t;
    else x1 = x0 + t;

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    glBegin(GL_QUADS);
    world_style_set_pos(x, y, z);
    for (int face = 0; face < 6; face++) {
        if (!upper && face == 1 && flat_get_block(x, y + 1, z) == id) continue;
        if (upper && face == 0 && flat_get_block(x, y - 1, z) == id) continue;
        int flip = 0;
        int tile = door_java_texture_for_face(id, meta, face, &flip);
        if (tile < 0) { tile = -tile; flip = 1; }
        g_render_flip_texture_once = flip;
        emit_cuboid_face_tile_auto(x0, y0, z0, x1, y1, z1, face, tile);
        g_render_flip_texture_once = 0;
    }
    glEnd();
    glDisable(GL_ALPHA_TEST);
}


static void draw_furnace_block_model(int id, int x, int y, int z) {
    int front = flat_get_meta(x, y, z);
    if (front < 2 || front > 5) front = 3;
    float x0 = (float)x, x1 = (float)x + 1.0f;
    float y0 = (float)y, y1 = (float)y + 1.0f;
    float z0 = (float)z, z1 = (float)z + 1.0f;
    glBegin(GL_QUADS);
    for (int face = 0; face < 6; face++) {
        if (!flat_face_exposed_for_block(id, x, y, z, face)) continue;
        int tile;
        if (face == 0 || face == 1) tile = furnace_top_tile();
        else if (face == front) tile = (id == BLOCK_FURNACE_LIT) ? furnace_front_lit_tile() : furnace_front_tile();
        else tile = furnace_side_tile();
        emit_cuboid_face_tile_auto(x0, y0, z0, x1, y1, z1, face, tile);
    }
    glEnd();
}

static void draw_stairs_block_model(int id, int x, int y, int z) {
    int dir = flat_get_meta(x, y, z) & 3;
    glBegin(GL_QUADS);
    emit_block_cuboid_faces(id, (float)x, (float)y, (float)z, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
    if (dir == 0) emit_block_cuboid_faces(id, (float)x, (float)y, (float)z, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f);
    else if (dir == 1) emit_block_cuboid_faces(id, (float)x, (float)y, (float)z, 0.0f, 0.5f, 0.0f, 0.5f, 1.0f, 1.0f);
    else if (dir == 2) emit_block_cuboid_faces(id, (float)x, (float)y, (float)z, 0.0f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f);
    else emit_block_cuboid_faces(id, (float)x, (float)y, (float)z, 0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f);
    glEnd();
}

static void draw_fence_block_model(int id, int x, int y, int z) {
    glBegin(GL_QUADS);
    emit_block_cuboid_faces(id, x, y, z, 0.375f, 0.0f, 0.375f, 0.625f, 1.0f, 0.625f);
    if ((flat_get_block(x-1,y,z) == id || flat_get_block(x-1,y,z) == BLOCK_FENCE || flat_get_block(x-1,y,z) == BLOCK_NETHER_BRICK_FENCE) || block_occludes_render_face(flat_get_block(x-1,y,z))) {
        emit_cuboid_block_faces(id, x, y+0.35f, z+0.4375f, x+0.5f, y+0.55f, z+0.5625f);
        emit_cuboid_block_faces(id, x, y+0.70f, z+0.4375f, x+0.5f, y+0.90f, z+0.5625f);
    }
    if ((flat_get_block(x+1,y,z) == id || flat_get_block(x+1,y,z) == BLOCK_FENCE || flat_get_block(x+1,y,z) == BLOCK_NETHER_BRICK_FENCE) || block_occludes_render_face(flat_get_block(x+1,y,z))) {
        emit_cuboid_block_faces(id, x+0.5f, y+0.35f, z+0.4375f, x+1.0f, y+0.55f, z+0.5625f);
        emit_cuboid_block_faces(id, x+0.5f, y+0.70f, z+0.4375f, x+1.0f, y+0.90f, z+0.5625f);
    }
    if ((flat_get_block(x,y,z-1) == id || flat_get_block(x,y,z-1) == BLOCK_FENCE || flat_get_block(x,y,z-1) == BLOCK_NETHER_BRICK_FENCE) || block_occludes_render_face(flat_get_block(x,y,z-1))) {
        emit_cuboid_block_faces(id, x+0.4375f, y+0.35f, z, x+0.5625f, y+0.55f, z+0.5f);
        emit_cuboid_block_faces(id, x+0.4375f, y+0.70f, z, x+0.5625f, y+0.90f, z+0.5f);
    }
    if ((flat_get_block(x,y,z+1) == id || flat_get_block(x,y,z+1) == BLOCK_FENCE || flat_get_block(x,y,z+1) == BLOCK_NETHER_BRICK_FENCE) || block_occludes_render_face(flat_get_block(x,y,z+1))) {
        emit_cuboid_block_faces(id, x+0.4375f, y+0.35f, z+0.5f, x+0.5625f, y+0.55f, z+1.0f);
        emit_cuboid_block_faces(id, x+0.4375f, y+0.70f, z+0.5f, x+0.5625f, y+0.90f, z+1.0f);
    }
    glEnd();
}

static int block_uses_special_model(int id) {
    return id == BLOCK_CHEST || id == BLOCK_FURNACE || id == BLOCK_FURNACE_LIT || block_is_door_id(id) || id == BLOCK_LADDER || id == BLOCK_SNOW_LAYER ||
           id == BLOCK_SLAB || id == BLOCK_WOOD_STAIRS || id == BLOCK_COBBLE_STAIRS ||
           id == BLOCK_BRICK_STAIRS || id == BLOCK_STONE_BRICK_STAIRS || id == BLOCK_NETHER_BRICK_STAIRS ||
           id == BLOCK_FENCE || id == BLOCK_NETHER_BRICK_FENCE || id == BLOCK_GLASS_PANE || id == BLOCK_IRON_BARS ||
           id == BLOCK_LILY_PAD || id == BLOCK_END_PORTAL_FRAME || id == BLOCK_END_PORTAL || id == BLOCK_PORTAL ||
           id == BLOCK_CACTUS || id == BLOCK_RAILS || id == BLOCK_POWERED_RAIL || id == BLOCK_DETECTOR_RAIL || id == BLOCK_REDSTONE_WIRE ||
           id == BLOCK_STONE_PRESSURE_PLATE || id == BLOCK_WOOD_PRESSURE_PLATE || id == BLOCK_STONE_BUTTON ||
           id == BLOCK_SIGN_POST || id == BLOCK_WALL_SIGN ||
           id == BLOCK_LEVER || id == BLOCK_TORCH || id == BLOCK_REDSTONE_TORCH_OFF || id == BLOCK_REDSTONE_TORCH_ON ||
           id == BLOCK_VINE;
}

static void emit_vine_side_quad(int x, int y, int z, int side, float u0, float v0, float u1, float v1) {
    const float e = 0.05f;
    if (side == 2) {
        float xx = (float)x + e;
        world_tex_vertex(xx, (float)y + 1.0f, (float)z + 1.0f, u0, v0);
        world_tex_vertex(xx, (float)y,        (float)z + 1.0f, u0, v1);
        world_tex_vertex(xx, (float)y,        (float)z,        u1, v1);
        world_tex_vertex(xx, (float)y + 1.0f, (float)z,        u1, v0);
        world_tex_vertex(xx, (float)y + 1.0f, (float)z,        u1, v0);
        world_tex_vertex(xx, (float)y,        (float)z,        u1, v1);
        world_tex_vertex(xx, (float)y,        (float)z + 1.0f, u0, v1);
        world_tex_vertex(xx, (float)y + 1.0f, (float)z + 1.0f, u0, v0);
    } else if (side == 8) {
        float xx = (float)x + 1.0f - e;
        world_tex_vertex(xx, (float)y,        (float)z + 1.0f, u1, v1);
        world_tex_vertex(xx, (float)y + 1.0f, (float)z + 1.0f, u1, v0);
        world_tex_vertex(xx, (float)y + 1.0f, (float)z,        u0, v0);
        world_tex_vertex(xx, (float)y,        (float)z,        u0, v1);
        world_tex_vertex(xx, (float)y,        (float)z,        u0, v1);
        world_tex_vertex(xx, (float)y + 1.0f, (float)z,        u0, v0);
        world_tex_vertex(xx, (float)y + 1.0f, (float)z + 1.0f, u1, v0);
        world_tex_vertex(xx, (float)y,        (float)z + 1.0f, u1, v1);
    } else if (side == 4) {
        float zz = (float)z + e;
        world_tex_vertex((float)x + 1.0f, (float)y,        zz, u1, v1);
        world_tex_vertex((float)x + 1.0f, (float)y + 1.0f, zz, u1, v0);
        world_tex_vertex((float)x,        (float)y + 1.0f, zz, u0, v0);
        world_tex_vertex((float)x,        (float)y,        zz, u0, v1);
        world_tex_vertex((float)x,        (float)y,        zz, u0, v1);
        world_tex_vertex((float)x,        (float)y + 1.0f, zz, u0, v0);
        world_tex_vertex((float)x + 1.0f, (float)y + 1.0f, zz, u1, v0);
        world_tex_vertex((float)x + 1.0f, (float)y,        zz, u1, v1);
    } else if (side == 1) {
        float zz = (float)z + 1.0f - e;
        world_tex_vertex((float)x + 1.0f, (float)y + 1.0f, zz, u0, v0);
        world_tex_vertex((float)x + 1.0f, (float)y,        zz, u0, v1);
        world_tex_vertex((float)x,        (float)y,        zz, u1, v1);
        world_tex_vertex((float)x,        (float)y + 1.0f, zz, u1, v0);
        world_tex_vertex((float)x,        (float)y + 1.0f, zz, u1, v0);
        world_tex_vertex((float)x,        (float)y,        zz, u1, v1);
        world_tex_vertex((float)x + 1.0f, (float)y,        zz, u0, v1);
        world_tex_vertex((float)x + 1.0f, (float)y + 1.0f, zz, u0, v0);
    }
}

static void emit_vine_top_quad(int x, int y, int z, float u0, float v0, float u1, float v1) {
    const float e = 0.05f;
    float yy = (float)y + 1.0f - e;
    world_tex_vertex((float)x + 1.0f, yy, (float)z,        u0, v0);
    world_tex_vertex((float)x + 1.0f, yy, (float)z + 1.0f, u0, v1);
    world_tex_vertex((float)x,        yy, (float)z + 1.0f, u1, v1);
    world_tex_vertex((float)x,        yy, (float)z,        u1, v0);
}

static void emit_vine_block_model(int x, int y, int z) {
    int meta = flat_get_meta(x, y, z) & 15;
    int sides = meta & 15;
    float u0, v0, u1, v1;
    terrain_tile_uv(143, &u0, &v0, &u1, &v1);
    world_style_set_pos(x, y, z);
    world_light_set_pos(x, y, z);
    world_set_color_shade(java_foliage_color_at(x, z), 1.0f);

    if (sides & 2) emit_vine_side_quad(x, y, z, 2, u0, v0, u1, v1);
    if (sides & 8) emit_vine_side_quad(x, y, z, 8, u0, v0, u1, v1);
    if (sides & 4) emit_vine_side_quad(x, y, z, 4, u0, v0, u1, v1);
    if (sides & 1) emit_vine_side_quad(x, y, z, 1, u0, v0, u1, v1);
    if (java_block_normal_cube_at(x, y + 1, z)) emit_vine_top_quad(x, y, z, u0, v0, u1, v1);
}

static void draw_vine_block_model(int x, int y, int z) {
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    glBegin(GL_QUADS);
    emit_vine_block_model(x, y, z);
    glEnd();
    glDisable(GL_ALPHA_TEST);
    glColor4f(1, 1, 1, 1);
}

static void draw_special_block_model(int id, int x, int y, int z) {
    if (id == BLOCK_CHEST) { draw_connected_chest_block(x, y, z); return; }
    if (id == BLOCK_FURNACE || id == BLOCK_FURNACE_LIT) { draw_furnace_block_model(id, x, y, z); return; }
    if (id == BLOCK_SNOW_LAYER) { glBegin(GL_QUADS); emit_snow_layer_block(x, y, z); glEnd(); glColor4f(1,1,1,1); return; }
    if (block_is_door_id(id)) { draw_door_block_model(id, x, y, z); return; }
    if (id == BLOCK_LADDER) { draw_ladder_block_model(x, y, z); return; }
    if (id == BLOCK_VINE) { draw_vine_block_model(x, y, z); return; }
    if (id == BLOCK_SLAB) { draw_cuboid_model_for_block(id, (float)x, (float)y, (float)z, 0,0,0,1,0.5f,1); return; }
    if (id == BLOCK_WOOD_STAIRS || id == BLOCK_COBBLE_STAIRS || id == BLOCK_BRICK_STAIRS || id == BLOCK_STONE_BRICK_STAIRS || id == BLOCK_NETHER_BRICK_STAIRS) { draw_stairs_block_model(id, x, y, z); return; }
    if (id == BLOCK_FENCE || id == BLOCK_NETHER_BRICK_FENCE) { draw_fence_block_model(id, x, y, z); return; }
    if (id == BLOCK_GLASS_PANE || id == BLOCK_IRON_BARS) { draw_cuboid_model_for_block(id, (float)x, (float)y, (float)z, 0.4375f,0,0,0.5625f,1,1); draw_cuboid_model_for_block(id, (float)x, (float)y, (float)z, 0,0,0.4375f,1,1,0.5625f); return; }
    if (id == BLOCK_LILY_PAD) { glBegin(GL_QUADS); emit_flat_quad_tile((float)x, (float)y + 0.01f, (float)z, (float)x+1, (float)z+1, block_texture_resolve(id, flat_get_meta(x,y,z), 1)); glEnd(); return; }
    if (id == BLOCK_PORTAL) {
        float u0, v0, u1, v1;
        terrain_tile_uv(14, &u0, &v0, &u1, &v1);
        flat_direct_set_color_light4f(1.0f, 1.0f, 1.0f, 0.5f, flat_fullbright_packed_light());
        int runs_along_x = 1;
        if (flat_get_block(x, y, z - 1) == BLOCK_PORTAL || flat_get_block(x, y, z + 1) == BLOCK_PORTAL ||
            flat_get_block(x, y, z - 1) == BLOCK_OBSIDIAN || flat_get_block(x, y, z + 1) == BLOCK_OBSIDIAN) {
            runs_along_x = 0;
        }
        glBegin(GL_QUADS);
        if (runs_along_x) {
            // South face
            world_tex_vertex((float)x,   (float)y,        (float)z + 0.625f, u0, v1);
            world_tex_vertex((float)x+1, (float)y,        (float)z + 0.625f, u1, v1);
            world_tex_vertex((float)x+1, (float)y + 1.0f, (float)z + 0.625f, u1, v0);
            world_tex_vertex((float)x,   (float)y + 1.0f, (float)z + 0.625f, u0, v0);
            // North face
            world_tex_vertex((float)x,   (float)y,        (float)z + 0.375f, u1, v1);
            world_tex_vertex((float)x,   (float)y + 1.0f, (float)z + 0.375f, u1, v0);
            world_tex_vertex((float)x+1, (float)y + 1.0f, (float)z + 0.375f, u0, v0);
            world_tex_vertex((float)x+1, (float)y,        (float)z + 0.375f, u0, v1);
        } else {
            // East face
            world_tex_vertex((float)x + 0.625f, (float)y,        (float)z,   u0, v1);
            world_tex_vertex((float)x + 0.625f, (float)y,        (float)z+1, u1, v1);
            world_tex_vertex((float)x + 0.625f, (float)y + 1.0f, (float)z+1, u1, v0);
            world_tex_vertex((float)x + 0.625f, (float)y + 1.0f, (float)z,   u0, v0);
            // West face
            world_tex_vertex((float)x + 0.375f, (float)y,        (float)z,   u1, v1);
            world_tex_vertex((float)x + 0.375f, (float)y + 1.0f, (float)z,   u1, v0);
            world_tex_vertex((float)x + 0.375f, (float)y + 1.0f, (float)z+1, u0, v0);
            world_tex_vertex((float)x + 0.375f, (float)y,        (float)z+1, u0, v1);
        }
        glEnd();
        flat_direct_set_color_light4f(1.0f, 1.0f, 1.0f, 1.0f, flat_fullbright_packed_light());
        return;
    }
    if (id == BLOCK_END_PORTAL) {
        float u0, v0, u1, v1;
        terrain_tile_uv(15, &u0, &v0, &u1, &v1);
        // Solid black background layer (layer 0)
        flat_direct_set_color_light4f(0.0f, 0.0f, 0.0f, 1.0f, flat_fullbright_packed_light());
        glBegin(GL_QUADS);
        world_tex_vertex((float)x,   (float)y + 0.01f, (float)z,   u0, v0);
        world_tex_vertex((float)x+1, (float)y + 0.01f, (float)z,   u1, v0);
        world_tex_vertex((float)x+1, (float)y + 0.01f, (float)z+1, u1, v1);
        world_tex_vertex((float)x,   (float)y + 0.01f, (float)z+1, u0, v1);
        glEnd();
        int layers = 8;
        for (int layer = 1; layer <= layers; ++layer) {
            float t = (float)layer / (float)(layers + 1);
            float depth = (float)y + 0.01f - t * 0.25f;
            float cr = t * 0.01f;
            float cg = t * 0.05f;
            float cb = 0.15f + t * 0.20f;
            float alpha = 0.20f * (1.0f - t * 0.5f);
            flat_direct_set_color_light4f(cr, cg, cb, alpha, flat_fullbright_packed_light());
            glBegin(GL_QUADS);
            world_tex_vertex((float)x,   depth, (float)z,   u0, v0);
            world_tex_vertex((float)x+1, depth, (float)z,   u1, v0);
            world_tex_vertex((float)x+1, depth, (float)z+1, u1, v1);
            world_tex_vertex((float)x,   depth, (float)z+1, u0, v1);
            glEnd();
        }
        flat_direct_set_color_light4f(1.0f, 1.0f, 1.0f, 1.0f, flat_fullbright_packed_light());
        return;
    }
    if (id == BLOCK_END_PORTAL_FRAME) { draw_cuboid_model_for_block(id, (float)x, (float)y, (float)z, 0,0,0,1,13.0f/16.0f,1); return; }
    if (id == BLOCK_CACTUS) { draw_cuboid_model_for_block(id, (float)x, (float)y, (float)z, 0.0625f,0,0.0625f,0.9375f,1,0.9375f); return; }
    if (id == BLOCK_STONE_PRESSURE_PLATE || id == BLOCK_WOOD_PRESSURE_PLATE) { float h = (flat_get_meta(x,y,z) & 1) ? (1.0f/32.0f) : (1.0f/16.0f); draw_cuboid_model_for_block(id, (float)x, (float)y, (float)z, 0.0625f,0,0.0625f,0.9375f,h,0.9375f); return; }
    if (id == BLOCK_RAILS || id == BLOCK_POWERED_RAIL || id == BLOCK_DETECTOR_RAIL || id == BLOCK_REDSTONE_WIRE) { glBegin(GL_QUADS); emit_flat_quad_tile((float)x, (float)y + 0.01f, (float)z, (float)x+1, (float)z+1, (id == BLOCK_RAILS) ? 128 : (id == BLOCK_POWERED_RAIL ? 179 : (id == BLOCK_DETECTOR_RAIL ? 195 : 84))); glEnd(); return; }
    if (id == BLOCK_SIGN_POST) { draw_cuboid_model_for_block(id, (float)x, (float)y, (float)z, 0.25f,0.55f,0.4375f,0.75f,1.0f,0.5625f); draw_cuboid_model_for_block(id, (float)x, (float)y, (float)z, 0.46875f,0.0f,0.46875f,0.53125f,0.55f,0.53125f); return; }
    if (id == BLOCK_WALL_SIGN) { draw_cuboid_model_for_block(id, (float)x, (float)y, (float)z, 0.25f,0.35f,0.02f,0.75f,0.85f,0.10f); return; }
    if (id == BLOCK_STONE_BUTTON) {
        BlockBounds bb = block_bounds_for_selection(id, x, y, z);
        draw_cuboid_model_for_block(id, (float)x, (float)y, (float)z,
                                    bb.x0 - (float)x, bb.y0 - (float)y, bb.z0 - (float)z,
                                    bb.x1 - (float)x, bb.y1 - (float)y, bb.z1 - (float)z);
        return;
    }
    if (id == BLOCK_LEVER) { draw_lever_block_model(x, y, z); return; }
    if (id == BLOCK_TORCH || id == BLOCK_REDSTONE_TORCH_OFF || id == BLOCK_REDSTONE_TORCH_ON) { draw_torch_block_model(id, x, y, z); return; }
}


static int liquid_same_family_at(int x, int y, int z, int is_water) {
    int id = flat_get_block(x, y, z);
    return is_water ? block_is_water(id) : block_is_lava(id);
}

/* RenderBlocks.func_1224_a style weighted height for Beta liquid corners. */
static float liquid_corner_height(int x, int y, int z, int is_water) {
    int count = 0;
    float sum = 0.0f;
    for (int i = 0; i < 4; i++) {
        int sx = x - (i & 1);
        int sz = z - ((i >> 1) & 1);
        if (liquid_same_family_at(sx, y + 1, sz, is_water)) return 1.0f;
        if (!liquid_same_family_at(sx, y, sz, is_water)) {
            if (!block_material_blocks_liquid(flat_get_block(sx, y, sz))) {
                sum += 1.0f;
                count++;
            }
        } else {
            int level = flat_get_level(sx, y, sz);
            if (level >= 8 || level == 0) {
                sum += fluid_decay_height(level) * 10.0f;
                count += 10;
            }
            sum += fluid_decay_height(level);
            count++;
        }
    }
    if (count <= 0) return 0.0f;
    return 1.0f - sum / (float)count;
}


static float liquid_flow_angle_at(int x, int y, int z, int is_water) {
    float vx = 0.0f, vy = 0.0f, vz = 0.0f;
    (void)vy;
    fluid_flow_vector_at(x, y, z, is_water, &vx, &vy, &vz);
    if (vx == 0.0f && vz == 0.0f) return -1000.0f;
    return atan2f(vz, vx) - (float)M_PI * 0.5f;
}

static void liquid_top_uvs_source(int tile, float angle,
                                  float *u00, float *v00, float *u01, float *v01,
                                  float *u11, float *v11, float *u10, float *v10) {
    float tw = tex_terrain.w ? (float)tex_terrain.w : 256.0f;
    float th = tex_terrain.h ? (float)tex_terrain.h : 256.0f;
    int tx = (tile & 15) * 16;
    int ty = (tile >> 4) * 16;
    float cu = ((float)tx + 8.0f) / tw;
    float cv = ((float)ty + 8.0f) / th;
    if (angle > -999.0f) {
        cu = ((float)tx + 16.0f) / tw;
        cv = ((float)ty + 16.0f) / th;
    } else {
        angle = 0.0f;
    }
    float sx = sinf(angle) * 8.0f / tw;
    float cz = cosf(angle) * 8.0f / th;
    *u00 = cu - cz - sx; *v00 = cv - cz + sx;
    *u01 = cu - cz + sx; *v01 = cv + cz + sx;
    *u11 = cu + cz + sx; *v11 = cv + cz - sx;
    *u10 = cu + cz - sx; *v10 = cv - cz - sx;
}

static void emit_liquid_block_faces(int id, int x, int y, int z) {
    int is_water = (id == BLOCK_WATER || id == BLOCK_STILL_WATER);
    int top_tile = liquid_top_tile_for_block(id);
    int side_tile = liquid_side_tile_for_block(id);
    float h00 = liquid_corner_height(x, y, z, is_water);
    float h01 = liquid_corner_height(x, y, z + 1, is_water);
    float h11 = liquid_corner_height(x + 1, y, z + 1, is_water);
    float h10 = liquid_corner_height(x + 1, y, z, is_water);
    float x0 = (float)x, x1 = (float)x + 1.0f;
    float y0 = (float)y;
    float z0 = (float)z, z1 = (float)z + 1.0f;
    float u0, v0, u1, v1;

    world_style_set_pos(x, y, z);

    if (liquid_face_exposed(x, y, z, 1)) {
        world_light_set_pos_for_face(x, y, z, 1);
        world_set_shade(1.0f);
        float u00, v00, u01, v01, u11, v11, u10, v10;
        liquid_top_uvs_source(top_tile, liquid_flow_angle_at(x, y, z, is_water),
                              &u00, &v00, &u01, &v01, &u11, &v11, &u10, &v10);
        world_tex_vertex(x0, (float)y + h00, z0, u00, v00);
        world_tex_vertex(x0, (float)y + h01, z1, u01, v01);
        world_tex_vertex(x1, (float)y + h11, z1, u11, v11);
        world_tex_vertex(x1, (float)y + h10, z0, u10, v10);
    }

    if (liquid_face_exposed(x, y, z, 0)) {
        world_light_set_pos_for_face(x, y, z, 0);
        world_set_shade(0.5f);
        terrain_tile_uv(top_tile, &u0, &v0, &u1, &v1);
        world_tex_vertex(x0, y0, z1, u0, v0);
        world_tex_vertex(x1, y0, z1, u1, v0);
        world_tex_vertex(x1, y0, z0, u1, v1);
        world_tex_vertex(x0, y0, z0, u0, v1);
    }

    terrain_tile_uv(side_tile, &u0, &v0, &u1, &v1);
    if (liquid_face_exposed(x, y, z, 2)) {
        world_light_set_pos_for_face(x, y, z, 2);
        world_set_shade(0.8f);
        world_tex_vertex(x1, (float)y + h10, z0, u0, v0);
        world_tex_vertex(x0, (float)y + h00, z0, u1, v0);
        world_tex_vertex(x0, y0, z0, u1, v1);
        world_tex_vertex(x1, y0, z0, u0, v1);
    }
    if (liquid_face_exposed(x, y, z, 3)) {
        world_light_set_pos_for_face(x, y, z, 3);
        world_set_shade(0.8f);
        world_tex_vertex(x0, (float)y + h01, z1, u0, v0);
        world_tex_vertex(x1, (float)y + h11, z1, u1, v0);
        world_tex_vertex(x1, y0, z1, u1, v1);
        world_tex_vertex(x0, y0, z1, u0, v1);
    }
    if (liquid_face_exposed(x, y, z, 4)) {
        world_light_set_pos_for_face(x, y, z, 4);
        world_set_shade(0.6f);
        world_tex_vertex(x0, (float)y + h00, z0, u0, v0);
        world_tex_vertex(x0, (float)y + h01, z1, u1, v0);
        world_tex_vertex(x0, y0, z1, u1, v1);
        world_tex_vertex(x0, y0, z0, u0, v1);
    }
    if (liquid_face_exposed(x, y, z, 5)) {
        world_light_set_pos_for_face(x, y, z, 5);
        world_set_shade(0.6f);
        world_tex_vertex(x1, (float)y + h11, z1, u0, v0);
        world_tex_vertex(x1, (float)y + h10, z0, u1, v0);
        world_tex_vertex(x1, y0, z0, u1, v1);
        world_tex_vertex(x1, y0, z1, u0, v1);
    }
}

static void draw_liquid_block(int id, float x, float y, float z) {
    int is_water = (id == BLOCK_WATER || id == BLOCK_STILL_WATER);
    int top_tile = liquid_top_tile_for_block(id);
    int side_tile = liquid_side_tile_for_block(id);
    int ix = (int)x, iy = (int)y, iz = (int)z;
    float h00 = liquid_corner_height(ix, iy, iz, is_water);
    float h01 = liquid_corner_height(ix, iy, iz + 1, is_water);
    float h11 = liquid_corner_height(ix + 1, iy, iz + 1, is_water);
    float h10 = liquid_corner_height(ix + 1, iy, iz, is_water);
    float x0 = x, x1 = x + 1.0f;
    float y0 = y;
    float z0 = z, z1 = z + 1.0f;
    float u0, v0, u1, v1;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    world_style_set_pos(ix, iy, iz);

    if (liquid_face_exposed(ix, iy, iz, 1)) {
        world_set_shade(1.0f);
        float u00, v00, u01, v01, u11, v11, u10, v10;
        liquid_top_uvs_source(top_tile, liquid_flow_angle_at(ix, iy, iz, is_water),
                              &u00, &v00, &u01, &v01, &u11, &v11, &u10, &v10);
        glBegin(GL_QUADS);
        world_tex_vertex(x0, y + h00, z0, u00, v00);
        world_tex_vertex(x0, y + h01, z1, u01, v01);
        world_tex_vertex(x1, y + h11, z1, u11, v11);
        world_tex_vertex(x1, y + h10, z0, u10, v10);
        glEnd();
    }

    if (liquid_face_exposed(ix, iy, iz, 0)) {
        world_set_shade(0.5f);
        terrain_tile_uv(top_tile, &u0, &v0, &u1, &v1);
        glBegin(GL_QUADS);
        world_tex_vertex(x0, y0, z1, u0, v0); world_tex_vertex(x1, y0, z1, u1, v0); world_tex_vertex(x1, y0, z0, u1, v1); world_tex_vertex(x0, y0, z0, u0, v1);
        glEnd();
    }

    terrain_tile_uv(side_tile, &u0, &v0, &u1, &v1);
    glBegin(GL_QUADS);
    world_set_shade(0.8f);
    if (liquid_face_exposed(ix, iy, iz, 2)) {
        world_tex_vertex(x1,y+h10,z0,u0,v0); world_tex_vertex(x0,y+h00,z0,u1,v0); world_tex_vertex(x0,y0,z0,u1,v1); world_tex_vertex(x1,y0,z0,u0,v1);
    }
    if (liquid_face_exposed(ix, iy, iz, 3)) {
        world_tex_vertex(x0,y+h01,z1,u0,v0); world_tex_vertex(x1,y+h11,z1,u1,v0); world_tex_vertex(x1,y0,z1,u1,v1); world_tex_vertex(x0,y0,z1,u0,v1);
    }
    if (liquid_face_exposed(ix, iy, iz, 4)) {
        world_set_shade(0.6f);
        world_tex_vertex(x0,y+h00,z0,u0,v0); world_tex_vertex(x0,y+h01,z1,u1,v0); world_tex_vertex(x0,y0,z1,u1,v1); world_tex_vertex(x0,y0,z0,u0,v1);
    }
    if (liquid_face_exposed(ix, iy, iz, 5)) {
        world_set_shade(0.6f);
        world_tex_vertex(x1,y+h11,z1,u0,v0); world_tex_vertex(x1,y+h10,z0,u1,v0); world_tex_vertex(x1,y0,z0,u1,v1); world_tex_vertex(x1,y0,z1,u0,v1);
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glColor4f(1,1,1,1);
}

static void draw_world_block_exposed(int id, int x, int y, int z) {
    if (id == 0) return;
    if (id == BLOCK_SNOW_LAYER) { glBegin(GL_QUADS); emit_snow_layer_block(x, y, z); glEnd(); glColor4f(1,1,1,1); return; }
    if (block_is_liquid(id) && flat_separate_liquid_pass_enabled()) { draw_liquid_block(id, (float)x, (float)y, (float)z); return; }
    if (block_uses_special_model(id)) { draw_special_block_model(id, x, y, z); return; }
    if (block_uses_cross_plant_model(id)) { draw_cross_plant_block(id, (float)x, (float)y, (float)z); return; }
    for (int face = 0; face < 6; face++) {
        if (flat_face_exposed_for_block(id, x, y, z, face)) draw_world_block_face(id, x, y, z, face);
    }
}

static void rebuild_flat_chunk_list(int cx, int cz) {
    if (!tex_terrain.id) return;
    if (cx < 0 || cx >= FLAT_RENDER_CHUNKS || cz < 0 || cz >= FLAT_RENDER_CHUNKS) return;

    if (g_flat_world_chunk_lists[cz][cx] == 0) {
        g_flat_world_chunk_lists[cz][cx] = glGenLists(1);
    }
    if (g_flat_world_chunk_liquid_lists[cz][cx] == 0) {
        g_flat_world_chunk_liquid_lists[cz][cx] = glGenLists(1);
    }
    if (g_flat_world_chunk_lists[cz][cx] == 0 || g_flat_world_chunk_liquid_lists[cz][cx] == 0) return;

    int origin_x = g_async_mesh_origin_override ? g_async_mesh_origin_x : g_flat_world_origin_x;
    int origin_z = g_async_mesh_origin_override ? g_async_mesh_origin_z : g_flat_world_origin_z;
    int x0 = origin_x + cx * FLAT_RENDER_CHUNK;
    int z0 = origin_z + cz * FLAT_RENDER_CHUNK;
    int x1 = x0 + FLAT_RENDER_CHUNK - 1;
    int z1 = z0 + FLAT_RENDER_CHUNK - 1;
    if (x1 >= origin_x + FLAT_WORLD_SIZE) x1 = origin_x + FLAT_WORLD_SIZE - 1;
    if (z1 >= origin_z + FLAT_WORLD_SIZE) z1 = origin_z + FLAT_WORLD_SIZE - 1;

    int chunk_ymax = FLAT_WORLD_Y_MIN;
    for (int y = FLAT_WORLD_Y_MAX; y >= FLAT_WORLD_Y_MIN; y--) {
        int found = 0;
        for (int z = z0; z <= z1 && !found; z++) for (int x = x0; x <= x1; x++) {
            if (flat_get_block(x, y, z) != 0) { found = 1; break; }
        }
        if (found) { chunk_ymax = y + 1; break; }
    }
    if (chunk_ymax > FLAT_WORLD_Y_MAX) chunk_ymax = FLAT_WORLD_Y_MAX;

    int chunk_has_liquid = 0;
    int chunk_has_cutout = 0;
    int chunk_has_special = 0;

    glNewList(g_flat_world_chunk_lists[cz][cx], GL_COMPILE);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);

    /* Solid opaque pass.  Fast and Fancy now share the same terrain topology.
       Fast visuals still use opaque leaf textures/no grass overlay, but liquids
       and cutouts are no longer merged into the legacy Fast solid mesh. */
    glDisable(GL_ALPHA_TEST);
    glBegin(GL_QUADS);
    for (int y = FLAT_WORLD_Y_MIN; y <= chunk_ymax; y++) {
        for (int z = z0; z <= z1; z++) {
            for (int x = x0; x <= x1; x++) {
                int id = flat_get_block(x, y, z);
                if (!id) continue;
                if (id == BLOCK_SNOW_LAYER) {
                    emit_snow_layer_block(x, y, z);
                    continue;
                }
                if (block_is_liquid(id)) {
                    if (flat_separate_liquid_pass_enabled()) { chunk_has_liquid = 1; continue; }
                    /* Non-Android Fast: opaque full-cube water/lava. */
                }
                if (block_uses_special_model(id)) { chunk_has_special = 1; continue; }
                if (block_uses_cross_plant_model(id)) { chunk_has_cutout = 1; continue; }
                if (id == BLOCK_LEAVES && flat_fancy_cutout_terrain_enabled()) { chunk_has_cutout = 1; continue; }
                if (id == BLOCK_GRASS && flat_fancy_grass_overlay_enabled()) {
                    for (int face = 2; face <= 5; face++) {
                        if (grass_side_overlay_face_needed(x, y, z, face)) { chunk_has_cutout = 1; break; }
                    }
                }
                for (int face = 0; face < 6; face++) {
                    if (flat_face_exposed_for_block(id, x, y, z, face)) emit_world_block_face(id, x, y, z, face);
                }
            }
        }
    }
    glEnd();

    if (chunk_has_special) {
        for (int y = FLAT_WORLD_Y_MIN; y <= chunk_ymax; y++) {
            for (int z = z0; z <= z1; z++) {
                for (int x = x0; x <= x1; x++) {
                    int id = flat_get_block(x, y, z);
                    if (block_uses_special_model(id)) draw_special_block_model(id, x, y, z);
                }
            }
        }
    }

    if (chunk_has_cutout) {
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.5f);
        glBegin(GL_QUADS);
        for (int y = FLAT_WORLD_Y_MIN; y <= chunk_ymax; y++) {
            for (int z = z0; z <= z1; z++) {
                for (int x = x0; x <= x1; x++) {
                    int id = flat_get_block(x, y, z);
                    if (!id) continue;
                    if (block_uses_cross_plant_model(id)) {
                        emit_cross_plant_block(id, x, y, z);
                        continue;
                    }
                    if (id == BLOCK_LEAVES && flat_fancy_cutout_terrain_enabled()) {
                        for (int face = 0; face < 6; face++) {
                            if (flat_face_exposed_for_block(id, x, y, z, face)) emit_world_block_face(id, x, y, z, face);
                        }
                    }
                    if (id == BLOCK_GRASS && flat_fancy_grass_overlay_enabled()) {
                        for (int face = 2; face <= 5; face++) {
                            if (grass_side_overlay_face_needed(x, y, z, face)) emit_grass_side_overlay_face(x, y, z, face);
                        }
                    }
                }
            }
        }
        glEnd();
        glDisable(GL_ALPHA_TEST);
    }
    glColor4f(1,1,1,1);
    glEndList();

    glNewList(g_flat_world_chunk_liquid_lists[cz][cx], GL_COMPILE);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    if (chunk_has_liquid && flat_separate_liquid_pass_enabled()) {
        glBegin(GL_QUADS);
        for (int y = FLAT_WORLD_Y_MIN; y <= chunk_ymax; y++) {
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
    g_flat_world_chunk_has_liquid[cz][cx] = chunk_has_liquid && flat_separate_liquid_pass_enabled();
}



/* Java-style 16x16x16 render sections -------------------------------------------------
   The legacy C renderer cached one full 16x256x16 column per display list.  A
   single flower/snow/door/water edit dirtied the whole column and rebuilding it
   scanned up to 65k block positions.  Beta Java used 16x16x16 WorldRenderer
   sections instead; this is the same idea ported to the C renderer. */
typedef struct FlatRenderSectionRef {
    int cx, cz, sy;
    float dist2;
} FlatRenderSectionRef;

#define FLAT_MAX_VISIBLE_SECTIONS (FLAT_RENDER_CHUNKS * FLAT_RENDER_CHUNKS * FLAT_RENDER_SECTIONS_Y)
static FlatRenderSectionRef g_flat_visible_sections[FLAT_MAX_VISIBLE_SECTIONS];
static FlatRenderSectionRef g_flat_visible_sorted_sections[FLAT_MAX_VISIBLE_SECTIONS];
static int g_flat_visible_sorted_count = 0;
static int g_flat_section_sort_valid = 0;
static int g_flat_section_sort_last_count = -1;
static float g_flat_section_sort_x = 0.0f, g_flat_section_sort_y = 0.0f, g_flat_section_sort_z = 0.0f;
static float g_flat_section_sort_yaw = 0.0f, g_flat_section_sort_pitch = 0.0f;

typedef struct FlatFrustum { float p[6][4]; } FlatFrustum;

static void flat_frustum_normalize(float p[4]) {
    float l = sqrtf(p[0]*p[0] + p[1]*p[1] + p[2]*p[2]);
    if (l <= 0.000001f) return;
    p[0] /= l; p[1] /= l; p[2] /= l; p[3] /= l;
}

static void flat_frustum_build(FlatFrustum *fr) {
    float proj[16], modl[16], clip[16];
    glGetFloatv(GL_PROJECTION_MATRIX, proj);
    glGetFloatv(GL_MODELVIEW_MATRIX, modl);

    clip[ 0] = modl[ 0] * proj[ 0] + modl[ 1] * proj[ 4] + modl[ 2] * proj[ 8] + modl[ 3] * proj[12];
    clip[ 1] = modl[ 0] * proj[ 1] + modl[ 1] * proj[ 5] + modl[ 2] * proj[ 9] + modl[ 3] * proj[13];
    clip[ 2] = modl[ 0] * proj[ 2] + modl[ 1] * proj[ 6] + modl[ 2] * proj[10] + modl[ 3] * proj[14];
    clip[ 3] = modl[ 0] * proj[ 3] + modl[ 1] * proj[ 7] + modl[ 2] * proj[11] + modl[ 3] * proj[15];
    clip[ 4] = modl[ 4] * proj[ 0] + modl[ 5] * proj[ 4] + modl[ 6] * proj[ 8] + modl[ 7] * proj[12];
    clip[ 5] = modl[ 4] * proj[ 1] + modl[ 5] * proj[ 5] + modl[ 6] * proj[ 9] + modl[ 7] * proj[13];
    clip[ 6] = modl[ 4] * proj[ 2] + modl[ 5] * proj[ 6] + modl[ 6] * proj[10] + modl[ 7] * proj[14];
    clip[ 7] = modl[ 4] * proj[ 3] + modl[ 5] * proj[ 7] + modl[ 6] * proj[11] + modl[ 7] * proj[15];
    clip[ 8] = modl[ 8] * proj[ 0] + modl[ 9] * proj[ 4] + modl[10] * proj[ 8] + modl[11] * proj[12];
    clip[ 9] = modl[ 8] * proj[ 1] + modl[ 9] * proj[ 5] + modl[10] * proj[ 9] + modl[11] * proj[13];
    clip[10] = modl[ 8] * proj[ 2] + modl[ 9] * proj[ 6] + modl[10] * proj[10] + modl[11] * proj[14];
    clip[11] = modl[ 8] * proj[ 3] + modl[ 9] * proj[ 7] + modl[10] * proj[11] + modl[11] * proj[15];
    clip[12] = modl[12] * proj[ 0] + modl[13] * proj[ 4] + modl[14] * proj[ 8] + modl[15] * proj[12];
    clip[13] = modl[12] * proj[ 1] + modl[13] * proj[ 5] + modl[14] * proj[ 9] + modl[15] * proj[13];
    clip[14] = modl[12] * proj[ 2] + modl[13] * proj[ 6] + modl[14] * proj[10] + modl[15] * proj[14];
    clip[15] = modl[12] * proj[ 3] + modl[13] * proj[ 7] + modl[14] * proj[11] + modl[15] * proj[15];

    /* right, left, bottom, top, far, near */
    fr->p[0][0] = clip[ 3] - clip[ 0]; fr->p[0][1] = clip[ 7] - clip[ 4]; fr->p[0][2] = clip[11] - clip[ 8]; fr->p[0][3] = clip[15] - clip[12];
    fr->p[1][0] = clip[ 3] + clip[ 0]; fr->p[1][1] = clip[ 7] + clip[ 4]; fr->p[1][2] = clip[11] + clip[ 8]; fr->p[1][3] = clip[15] + clip[12];
    fr->p[2][0] = clip[ 3] + clip[ 1]; fr->p[2][1] = clip[ 7] + clip[ 5]; fr->p[2][2] = clip[11] + clip[ 9]; fr->p[2][3] = clip[15] + clip[13];
    fr->p[3][0] = clip[ 3] - clip[ 1]; fr->p[3][1] = clip[ 7] - clip[ 5]; fr->p[3][2] = clip[11] - clip[ 9]; fr->p[3][3] = clip[15] - clip[13];
    fr->p[4][0] = clip[ 3] - clip[ 2]; fr->p[4][1] = clip[ 7] - clip[ 6]; fr->p[4][2] = clip[11] - clip[10]; fr->p[4][3] = clip[15] - clip[14];
    fr->p[5][0] = clip[ 3] + clip[ 2]; fr->p[5][1] = clip[ 7] + clip[ 6]; fr->p[5][2] = clip[11] + clip[10]; fr->p[5][3] = clip[15] + clip[14];
    for (int i = 0; i < 6; i++) flat_frustum_normalize(fr->p[i]);
}

static int flat_aabb_in_frustum(const FlatFrustum *fr, float x0, float y0, float z0, float x1, float y1, float z1) {
    for (int i = 0; i < 6; i++) {
        const float *p = fr->p[i];
        float x = (p[0] >= 0.0f) ? x1 : x0;
        float y = (p[1] >= 0.0f) ? y1 : y0;
        float z = (p[2] >= 0.0f) ? z1 : z0;
        if (p[0] * x + p[1] * y + p[2] * z + p[3] < 0.0f) return 0;
    }
    return 1;
}

static int flat_section_ref_cmp_near(const void *a, const void *b) {
    const FlatRenderSectionRef *ra = (const FlatRenderSectionRef*)a;
    const FlatRenderSectionRef *rb = (const FlatRenderSectionRef*)b;
    if (ra->dist2 < rb->dist2) return -1;
    if (ra->dist2 > rb->dist2) return 1;
    return 0;
}

static float flat_angle_delta_abs(float a, float b) {
    float d = a - b;
    while (d < -180.0f) d += 360.0f;
    while (d >= 180.0f) d -= 360.0f;
    if (d < 0.0f) d = -d;
    return d;
}

static int flat_renderer_sort_needed(float px, float py, float pz, int count) {
    if (g_flat_renderer_sort_dirty) return 1;
    if (!g_flat_section_sort_valid || count != g_flat_section_sort_last_count) return 1;
    float dx = px - g_flat_section_sort_x;
    float dy = py - g_flat_section_sort_y;
    float dz = pz - g_flat_section_sort_z;
    /* RenderGlobal.sortAndRender in 1.5.2 only pays the sort/recenter cost
       after the camera has moved several blocks.  Also refresh occasionally on
       large view-angle changes so translucent water remains acceptable. */
    if (dx * dx + dy * dy + dz * dz > 16.0f) return 1;
    if (flat_angle_delta_abs(g_player_render_frame.yaw, g_flat_section_sort_yaw) > 12.0f) return 1;
    if (fabsf(g_player_render_frame.pitch - g_flat_section_sort_pitch) > 12.0f) return 1;
    return 0;
}

static void flat_note_renderer_sorted(float px, float py, float pz, int count) {
    g_flat_section_sort_valid = 1;
    g_flat_section_sort_last_count = count;
    g_flat_section_sort_x = px;
    g_flat_section_sort_y = py;
    g_flat_section_sort_z = pz;
    g_flat_section_sort_yaw = g_player_render_frame.yaw;
    g_flat_section_sort_pitch = g_player_render_frame.pitch;
    g_flat_renderer_sort_dirty = 0;
}

static void ensure_flat_section_lists(int sy, int cz, int cx) {
    if (g_flat_section_lists[sy][cz][cx][0] == 0 || g_flat_section_lists[sy][cz][cx][1] == 0) {
        GLuint base = glGenLists(2);
        if (base != 0) {
            g_flat_section_lists[sy][cz][cx][0] = base;
            g_flat_section_lists[sy][cz][cx][1] = base + 1;
        }
    }
}


static void rebuild_flat_section_mesh(int sy, int cx, int cz) {
    PexRendererBackend *rb = flat_direct_backend();
    if (!tex_terrain.rgba) return;
    if (!g_flat_direct_capture_only && !rb) return;
    if (cx < 0 || cx >= FLAT_RENDER_CHUNKS || cz < 0 || cz >= FLAT_RENDER_CHUNKS || sy < 0 || sy >= FLAT_RENDER_SECTIONS_Y) return;

    int origin_x = g_async_mesh_origin_override ? g_async_mesh_origin_x : g_flat_world_origin_x;
    int origin_z = g_async_mesh_origin_override ? g_async_mesh_origin_z : g_flat_world_origin_z;
    int x0 = origin_x + cx * FLAT_RENDER_CHUNK;
    int z0 = origin_z + cz * FLAT_RENDER_CHUNK;
    int x1 = x0 + FLAT_RENDER_CHUNK - 1;
    int z1 = z0 + FLAT_RENDER_CHUNK - 1;
    int y0 = FLAT_WORLD_Y_MIN + sy * FLAT_RENDER_SECTION;
    int y1 = y0 + FLAT_RENDER_SECTION - 1;
    if (x1 >= origin_x + FLAT_WORLD_SIZE) x1 = origin_x + FLAT_WORLD_SIZE - 1;
    if (z1 >= origin_z + FLAT_WORLD_SIZE) z1 = origin_z + FLAT_WORLD_SIZE - 1;
    if (y1 > FLAT_WORLD_Y_MAX) y1 = FLAT_WORLD_Y_MAX;

    int has0 = 0, has1 = 0, has_special = 0, has_cutout = 0;
    FlatDirectMeshBuilder mb0, mb1;

    flat_direct_begin(&mb0);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    for (int y = y0; y <= y1; y++) {
        for (int z = z0; z <= z1; z++) {
            for (int x = x0; x <= x1; x++) {
                int id = flat_get_block(x, y, z);
                if (!id) continue;
                if (id == BLOCK_PORTAL || id == BLOCK_END_PORTAL) { has1 = 1; continue; }
                if (id == BLOCK_SNOW_LAYER) { emit_snow_layer_block(x, y, z); has0 = 1; continue; }
                if (block_is_liquid(id)) { if (flat_separate_liquid_pass_enabled()) { has1 = 1; continue; } }
                if (block_uses_special_model(id)) { has_special = 1; continue; }
                if (block_uses_cross_plant_model(id)) { has_cutout = 1; continue; }
                if (id == BLOCK_LEAVES && flat_fancy_cutout_terrain_enabled()) { has_cutout = 1; continue; }
                if (id == BLOCK_GRASS && flat_fancy_grass_overlay_enabled()) {
                    for (int face = 2; face <= 5; face++) {
                        if (grass_side_overlay_face_needed(x, y, z, face)) { has_cutout = 1; break; }
                    }
                }
                for (int face = 0; face < 6; face++) {
                    if (flat_face_exposed_for_block(id, x, y, z, face)) { emit_world_block_face(id, x, y, z, face); has0 = 1; }
                }
            }
        }
    }
    if (has_special) {
        for (int y = y0; y <= y1; y++) for (int z = z0; z <= z1; z++) for (int x = x0; x <= x1; x++) {
            int id = flat_get_block(x, y, z);
            if (block_uses_special_model(id) && id != BLOCK_PORTAL && id != BLOCK_END_PORTAL) { draw_special_block_model(id, x, y, z); has0 = 1; }
        }
    }
    if (has_cutout) {
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.5f);
        for (int y = y0; y <= y1; y++) {
            for (int z = z0; z <= z1; z++) {
                for (int x = x0; x <= x1; x++) {
                    int id = flat_get_block(x, y, z);
                    if (!id) continue;
                    if (block_uses_cross_plant_model(id)) { emit_cross_plant_block(id, x, y, z); has0 = 1; continue; }
                    if (id == BLOCK_LEAVES && flat_fancy_cutout_terrain_enabled()) {
                        for (int face = 0; face < 6; face++) {
                            if (flat_face_exposed_for_block(id, x, y, z, face)) { emit_world_block_face(id, x, y, z, face); has0 = 1; }
                        }
                    }
                    if (id == BLOCK_GRASS && flat_fancy_grass_overlay_enabled()) {
                        for (int face = 2; face <= 5; face++) {
                            if (grass_side_overlay_face_needed(x, y, z, face)) { emit_grass_side_overlay_face(x, y, z, face); has0 = 1; }
                        }
                    }
                }
            }
        }
        glDisable(GL_ALPHA_TEST);
    }
    glColor4f(1,1,1,1);
    flat_direct_end();

    flat_direct_begin(&mb1);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    if (has1) {
        for (int y = y0; y <= y1; y++) for (int z = z0; z <= z1; z++) for (int x = x0; x <= x1; x++) {
            int id = flat_get_block(x, y, z);
            if (block_is_liquid(id)) {
                if (flat_separate_liquid_pass_enabled()) emit_liquid_block_faces(id, x, y, z);
            } else if (id == BLOCK_PORTAL || id == BLOCK_END_PORTAL) {
                draw_special_block_model(id, x, y, z);
            }
        }
    }
    glColor4f(1,1,1,1);
    flat_direct_end();

    if (g_flat_direct_capture_only) {
        if (g_flat_direct_capture_out0) *g_flat_direct_capture_out0 = mb0;
        if (g_flat_direct_capture_out1) *g_flat_direct_capture_out1 = mb1;
        if (g_flat_direct_capture_skip0) *g_flat_direct_capture_skip0 = !has0;
        if (g_flat_direct_capture_skip1) *g_flat_direct_capture_skip1 = !has1;
        g_flat_direct_capture_success = 1;
        return;
    }

    flat_direct_upload_builder(sy, cz, cx, 0, &mb0);
    flat_direct_upload_builder(sy, cz, cx, 1, &mb1);
    flat_direct_free_builder(&mb0);
    flat_direct_free_builder(&mb1);

    g_flat_section_mesh_building[sy][cz][cx] = 0;
    g_flat_section_dirty[sy][cz][cx] = 0;
    g_flat_section_valid[sy][cz][cx] = 1;
    g_flat_section_skip_pass[sy][cz][cx][0] = !has0;
    g_flat_section_skip_pass[sy][cz][cx][1] = !has1;
    g_flat_section_mesh_light_version[sy][cz][cx] = g_flat_chunk_light_version[cz][cx];
}

static void rebuild_flat_section_list(int sy, int cx, int cz) {
    if (flat_direct_backend()) { rebuild_flat_section_mesh(sy, cx, cz); return; }
    if (!tex_terrain.id) return;
    if (cx < 0 || cx >= FLAT_RENDER_CHUNKS || cz < 0 || cz >= FLAT_RENDER_CHUNKS || sy < 0 || sy >= FLAT_RENDER_SECTIONS_Y) return;
    ensure_flat_section_lists(sy, cz, cx);
    if (g_flat_section_lists[sy][cz][cx][0] == 0 || g_flat_section_lists[sy][cz][cx][1] == 0) return;

    int x0 = g_flat_world_origin_x + cx * FLAT_RENDER_CHUNK;
    int z0 = g_flat_world_origin_z + cz * FLAT_RENDER_CHUNK;
    int x1 = x0 + FLAT_RENDER_CHUNK - 1;
    int z1 = z0 + FLAT_RENDER_CHUNK - 1;
    int y0 = FLAT_WORLD_Y_MIN + sy * FLAT_RENDER_SECTION;
    int y1 = y0 + FLAT_RENDER_SECTION - 1;
    if (x1 >= g_flat_world_origin_x + FLAT_WORLD_SIZE) x1 = g_flat_world_origin_x + FLAT_WORLD_SIZE - 1;
    if (z1 >= g_flat_world_origin_z + FLAT_WORLD_SIZE) z1 = g_flat_world_origin_z + FLAT_WORLD_SIZE - 1;
    if (y1 > FLAT_WORLD_Y_MAX) y1 = FLAT_WORLD_Y_MAX;

    int has0 = 0, has1 = 0, has_special = 0, has_cutout = 0;

    glNewList(g_flat_section_lists[sy][cz][cx][0], GL_COMPILE);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);

    glBegin(GL_QUADS);
    for (int y = y0; y <= y1; y++) {
        for (int z = z0; z <= z1; z++) {
            for (int x = x0; x <= x1; x++) {
                int id = flat_get_block(x, y, z);
                if (!id) continue;
                if (id == BLOCK_PORTAL || id == BLOCK_END_PORTAL) { has1 = 1; continue; }
                if (id == BLOCK_SNOW_LAYER) {
                    emit_snow_layer_block(x, y, z);
                    has0 = 1;
                    continue;
                }
                if (block_is_liquid(id)) {
                    if (flat_separate_liquid_pass_enabled()) { has1 = 1; continue; }
                }
                if (block_uses_special_model(id)) { has_special = 1; continue; }
                if (block_uses_cross_plant_model(id)) { has_cutout = 1; continue; }
                if (id == BLOCK_LEAVES && flat_fancy_cutout_terrain_enabled()) { has_cutout = 1; continue; }
                if (id == BLOCK_GRASS && flat_fancy_grass_overlay_enabled()) {
                    for (int face = 2; face <= 5; face++) {
                        if (grass_side_overlay_face_needed(x, y, z, face)) { has_cutout = 1; break; }
                    }
                }
                for (int face = 0; face < 6; face++) {
                    if (flat_face_exposed_for_block(id, x, y, z, face)) { emit_world_block_face(id, x, y, z, face); has0 = 1; }
                }
            }
        }
    }
    glEnd();

    if (has_special) {
        for (int y = y0; y <= y1; y++) {
            for (int z = z0; z <= z1; z++) {
                for (int x = x0; x <= x1; x++) {
                    int id = flat_get_block(x, y, z);
                    if (block_uses_special_model(id) && id != BLOCK_PORTAL && id != BLOCK_END_PORTAL) { draw_special_block_model(id, x, y, z); has0 = 1; }
                }
            }
        }
    }

    if (has_cutout) {
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.5f);
        glBegin(GL_QUADS);
        for (int y = y0; y <= y1; y++) {
            for (int z = z0; z <= z1; z++) {
                for (int x = x0; x <= x1; x++) {
                    int id = flat_get_block(x, y, z);
                    if (!id) continue;
                    if (block_uses_cross_plant_model(id)) { emit_cross_plant_block(id, x, y, z); has0 = 1; continue; }
                    if (id == BLOCK_LEAVES && flat_fancy_cutout_terrain_enabled()) {
                        for (int face = 0; face < 6; face++) {
                            if (flat_face_exposed_for_block(id, x, y, z, face)) { emit_world_block_face(id, x, y, z, face); has0 = 1; }
                        }
                    }
                    if (id == BLOCK_GRASS && flat_fancy_grass_overlay_enabled()) {
                        for (int face = 2; face <= 5; face++) {
                            if (grass_side_overlay_face_needed(x, y, z, face)) { emit_grass_side_overlay_face(x, y, z, face); has0 = 1; }
                        }
                    }
                }
            }
        }
        glEnd();
        glDisable(GL_ALPHA_TEST);
    }
    glColor4f(1,1,1,1);
    glEndList();

    glNewList(g_flat_section_lists[sy][cz][cx][1], GL_COMPILE);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    if (has1) {
        int liquid_active = 0;
        for (int y = y0; y <= y1; y++) {
            for (int z = z0; z <= z1; z++) {
                for (int x = x0; x <= x1; x++) {
                    int id = flat_get_block(x, y, z);
                    if (block_is_liquid(id) && flat_separate_liquid_pass_enabled()) {
                        if (!liquid_active) {
                            glBegin(GL_QUADS);
                            liquid_active = 1;
                        }
                        emit_liquid_block_faces(id, x, y, z);
                    } else if (id == BLOCK_PORTAL || id == BLOCK_END_PORTAL) {
                        if (liquid_active) {
                            glEnd();
                            liquid_active = 0;
                        }
                        draw_special_block_model(id, x, y, z);
                    }
                }
            }
        }
        if (liquid_active) {
            glEnd();
        }
    }
    glColor4f(1,1,1,1);
    glEndList();

    g_flat_section_mesh_building[sy][cz][cx] = 0;
    g_flat_section_dirty[sy][cz][cx] = 0;
    g_flat_section_valid[sy][cz][cx] = 1;
    g_flat_section_skip_pass[sy][cz][cx][0] = !has0;
    g_flat_section_skip_pass[sy][cz][cx][1] = !has1;
    g_flat_section_mesh_light_version[sy][cz][cx] = g_flat_chunk_light_version[cz][cx];
}



/* Async D3D section-mesh build ------------------------------------------------------
   The GPU upload and live mesh-handle swap still happen on the render thread, but
   the expensive 16^3 block scan / face emission / CPU vertex+index allocation now
   runs on a Win32 worker using a tiny copied snapshot of the section plus a
   one-block neighbor border. */
#define ASYNC_SECTION_MESH_W (FLAT_RENDER_CHUNK + 2)
#define ASYNC_SECTION_MESH_H (FLAT_RENDER_SECTION + 2)
#define ASYNC_SECTION_MESH_D (FLAT_RENDER_CHUNK + 2)
#define ASYNC_SECTION_MESH_BYTES (ASYNC_SECTION_MESH_W * ASYNC_SECTION_MESH_H * ASYNC_SECTION_MESH_D)

typedef struct AsyncSectionMeshJob {
    int cx, cz, sy;
    int origin_x, origin_z;
    unsigned int version;
    unsigned int light_version;
    int priority;
    unsigned char blocks[ASYNC_SECTION_MESH_BYTES];
    unsigned char meta[ASYNC_SECTION_MESH_BYTES];
    unsigned char levels[ASYNC_SECTION_MESH_BYTES];
    unsigned char sky_light[ASYNC_SECTION_MESH_BYTES];
    unsigned char block_light[ASYNC_SECTION_MESH_BYTES];
} AsyncSectionMeshJob;

typedef struct AsyncSectionMeshResult {
    int ready;
    int cx, cz, sy;
    int origin_x, origin_z;
    unsigned int version;
    unsigned int light_version;
    int priority;
    FlatDirectMeshBuilder mb0, mb1;
    int skip0, skip1;
    /* D3D11 can pre-create terrain section GPU buffers on the worker using the
       ID3D11Device.  The render thread then only adopts already-created buffers,
       avoiding the immediate-context Map/CreateBuffer hitch that made D3D11 feel
       worse than OpenGL. */
    int d3d11_prebuilt;
    PexD3D11Mesh d3d11_mesh0, d3d11_mesh1;
} AsyncSectionMeshResult;

static void async_section_mesh_free_result(AsyncSectionMeshResult *r) {
    if (!r) return;
    flat_direct_free_builder(&r->mb0);
    flat_direct_free_builder(&r->mb1);
    if (r->d3d11_prebuilt) {
        d3d11_discard_mesh(&r->d3d11_mesh0);
        d3d11_discard_mesh(&r->d3d11_mesh1);
    }
    memset(r, 0, sizeof(*r));
}

static CRITICAL_SECTION g_async_section_mesh_cs;
static HANDLE g_async_section_mesh_event = NULL;
static HANDLE g_async_section_mesh_upload_event = NULL;
static HANDLE g_async_section_mesh_thread = NULL;
static HANDLE g_async_section_mesh_upload_thread = NULL;
static int g_async_section_mesh_initialized = 0;
static int g_async_section_mesh_stop = 0;
static int g_async_section_mesh_busy = 0;
static int g_async_section_mesh_upload_busy = 0;

#if defined(PEX_PLATFORM_PSP)
/* PSP has far less RAM than desktop.  Keep the async mesh queue intentionally
   small; a single job snapshot is about 29 KB and each finished section can own
   a sizeable vertex/index buffer until the render thread adopts it. */
#define ASYNC_SECTION_MESH_JOB_QUEUE_MAX 12
#define ASYNC_SECTION_MESH_UPLOAD_QUEUE_MAX 4
#define ASYNC_SECTION_MESH_RESULT_QUEUE_MAX 6
#else
#define ASYNC_SECTION_MESH_JOB_QUEUE_MAX 96
#define ASYNC_SECTION_MESH_UPLOAD_QUEUE_MAX 24
#define ASYNC_SECTION_MESH_RESULT_QUEUE_MAX 24
#endif

static AsyncSectionMeshJob g_async_section_mesh_jobs[ASYNC_SECTION_MESH_JOB_QUEUE_MAX];
static int g_async_section_mesh_job_head = 0, g_async_section_mesh_job_tail = 0, g_async_section_mesh_job_count = 0;
static AsyncSectionMeshResult g_async_section_mesh_upload_jobs[ASYNC_SECTION_MESH_UPLOAD_QUEUE_MAX];
static int g_async_section_mesh_upload_head = 0, g_async_section_mesh_upload_tail = 0, g_async_section_mesh_upload_count = 0;
static AsyncSectionMeshResult g_async_section_mesh_results[ASYNC_SECTION_MESH_RESULT_QUEUE_MAX];
static int g_async_section_mesh_result_head = 0, g_async_section_mesh_result_tail = 0, g_async_section_mesh_result_count = 0;

static void async_section_mesh_clear_tls(void) {
    g_async_mesh_blocks = NULL;
    g_async_mesh_meta = NULL;
    g_async_mesh_levels = NULL;
    g_async_mesh_sky_light = NULL;
    g_async_mesh_block_light = NULL;
    g_async_mesh_x0 = g_async_mesh_y0 = g_async_mesh_z0 = 0;
    g_async_mesh_w = g_async_mesh_h = g_async_mesh_d = 0;
    g_async_mesh_origin_override = 0;
    g_async_mesh_origin_x = g_async_mesh_origin_z = 0;
    g_flat_direct_capture_only = 0;
    g_flat_direct_capture_out0 = NULL;
    g_flat_direct_capture_out1 = NULL;
    g_flat_direct_capture_skip0 = NULL;
    g_flat_direct_capture_skip1 = NULL;
    g_flat_direct_capture_success = 0;
}

static void async_mesh_clear_results(AsyncSectionMeshResult *queue, int count) {
    for (int i = 0; i < count; i++) async_section_mesh_free_result(&queue[i]);
}

static int async_mesh_push_upload_job(AsyncSectionMeshResult *r) {
    for (;;) {
        EnterCriticalSection(&g_async_section_mesh_cs);
        if (g_async_section_mesh_stop) {
            LeaveCriticalSection(&g_async_section_mesh_cs);
            async_section_mesh_free_result(r);
            return 0;
        }
        if (g_async_section_mesh_upload_count < ASYNC_SECTION_MESH_UPLOAD_QUEUE_MAX) {
            if (r->priority) {
                g_async_section_mesh_upload_head = (g_async_section_mesh_upload_head - 1 + ASYNC_SECTION_MESH_UPLOAD_QUEUE_MAX) % ASYNC_SECTION_MESH_UPLOAD_QUEUE_MAX;
                g_async_section_mesh_upload_jobs[g_async_section_mesh_upload_head] = *r;
            } else {
                g_async_section_mesh_upload_jobs[g_async_section_mesh_upload_tail] = *r;
                g_async_section_mesh_upload_tail = (g_async_section_mesh_upload_tail + 1) % ASYNC_SECTION_MESH_UPLOAD_QUEUE_MAX;
            }
            memset(r, 0, sizeof(*r));
            g_async_section_mesh_upload_count++;
            LeaveCriticalSection(&g_async_section_mesh_cs);
            if (g_async_section_mesh_upload_event) SetEvent(g_async_section_mesh_upload_event);
            return 1;
        }
        LeaveCriticalSection(&g_async_section_mesh_cs);
        Sleep(0);
    }
}

static int async_mesh_push_result(AsyncSectionMeshResult *r) {
    for (;;) {
        EnterCriticalSection(&g_async_section_mesh_cs);
        if (g_async_section_mesh_stop) {
            LeaveCriticalSection(&g_async_section_mesh_cs);
            async_section_mesh_free_result(r);
            return 0;
        }
        if (g_async_section_mesh_result_count < ASYNC_SECTION_MESH_RESULT_QUEUE_MAX) {
            if (r->priority) {
                g_async_section_mesh_result_head = (g_async_section_mesh_result_head - 1 + ASYNC_SECTION_MESH_RESULT_QUEUE_MAX) % ASYNC_SECTION_MESH_RESULT_QUEUE_MAX;
                g_async_section_mesh_results[g_async_section_mesh_result_head] = *r;
            } else {
                g_async_section_mesh_results[g_async_section_mesh_result_tail] = *r;
                g_async_section_mesh_result_tail = (g_async_section_mesh_result_tail + 1) % ASYNC_SECTION_MESH_RESULT_QUEUE_MAX;
            }
            memset(r, 0, sizeof(*r));
            g_async_section_mesh_result_count++;
            LeaveCriticalSection(&g_async_section_mesh_cs);
            return 1;
        }
        LeaveCriticalSection(&g_async_section_mesh_cs);
        Sleep(0);
    }
}

static DWORD WINAPI async_mesh_upload_worker(LPVOID unused) {
    (void)unused;
    g_pex_profile_thread_role = PEX_PROFILE_ROLE_ASYNC_MESH;
    for (;;) {
        WaitForSingleObject(g_async_section_mesh_upload_event, INFINITE);
        for (;;) {
            AsyncSectionMeshResult r;
            memset(&r, 0, sizeof(r));
            int have_job = 0;

            EnterCriticalSection(&g_async_section_mesh_cs);
            if (g_async_section_mesh_stop) {
                LeaveCriticalSection(&g_async_section_mesh_cs);
                return 0;
            }
            if (g_async_section_mesh_upload_count > 0) {
                r = g_async_section_mesh_upload_jobs[g_async_section_mesh_upload_head];
                memset(&g_async_section_mesh_upload_jobs[g_async_section_mesh_upload_head], 0, sizeof(g_async_section_mesh_upload_jobs[0]));
                g_async_section_mesh_upload_head = (g_async_section_mesh_upload_head + 1) % ASYNC_SECTION_MESH_UPLOAD_QUEUE_MAX;
                g_async_section_mesh_upload_count--;
                g_async_section_mesh_upload_busy = 1;
                have_job = 1;
            }
            LeaveCriticalSection(&g_async_section_mesh_cs);
            if (!have_job) break;

            double upload_worker_start = now_seconds();
            if (pex_using_d3d11()) {
                int ok0 = 1, ok1 = 1;
                flat_direct_resolve_builder_lightmap(&r.mb0);
                flat_direct_resolve_builder_lightmap(&r.mb1);
                if (!r.skip0 && r.mb0.vcount > 0 && r.mb0.icount > 0) {
                    PexMesh mesh; memset(&mesh, 0, sizeof(mesh));
                    mesh.vertices = r.mb0.v; mesh.indices = r.mb0.i;
                    mesh.vertex_count = r.mb0.vcount; mesh.index_count = r.mb0.icount;
                    mesh.dynamic = 0;
                    ok0 = d3d11_prebuild_mesh(&mesh, &r.d3d11_mesh0);
                }
                if (!r.skip1 && r.mb1.vcount > 0 && r.mb1.icount > 0) {
                    PexMesh mesh; memset(&mesh, 0, sizeof(mesh));
                    mesh.vertices = r.mb1.v; mesh.indices = r.mb1.i;
                    mesh.vertex_count = r.mb1.vcount; mesh.index_count = r.mb1.icount;
                    mesh.dynamic = 0;
                    ok1 = d3d11_prebuild_mesh(&mesh, &r.d3d11_mesh1);
                }
                if (ok0 && ok1) {
                    r.d3d11_prebuilt = 1;
                    flat_direct_free_builder(&r.mb0);
                    flat_direct_free_builder(&r.mb1);
                } else {
                    d3d11_discard_mesh(&r.d3d11_mesh0);
                    d3d11_discard_mesh(&r.d3d11_mesh1);
                    r.d3d11_prebuilt = 0;
                    /* Keep the CPU builders so the render thread can still use
                       the existing fallback upload path instead of dropping the
                       section if the worker-side D3D allocation fails. */
                }
            }

            async_mesh_push_result(&r);
            {
                double upload_ms = (now_seconds() - upload_worker_start) * 1000.0;
                if (upload_ms < 0.0) upload_ms = 0.0;
                g_prof_mesh_upload_worker_last_ms = upload_ms;
                if (g_prof_mesh_upload_worker_samples <= 0) g_prof_mesh_upload_worker_avg_ms = upload_ms;
                else g_prof_mesh_upload_worker_avg_ms = g_prof_mesh_upload_worker_avg_ms * 0.90 + upload_ms * 0.10;
                g_prof_mesh_upload_worker_samples++;
            }

            EnterCriticalSection(&g_async_section_mesh_cs);
            g_async_section_mesh_upload_busy = 0;
            LeaveCriticalSection(&g_async_section_mesh_cs);
        }
    }
}

static DWORD WINAPI async_section_mesh_worker_proc(LPVOID unused) {
    (void)unused;
    g_pex_profile_thread_role = PEX_PROFILE_ROLE_ASYNC_MESH;
    for (;;) {
        WaitForSingleObject(g_async_section_mesh_event, INFINITE);

        for (;;) {
            AsyncSectionMeshJob job;
            int have_job = 0;
            EnterCriticalSection(&g_async_section_mesh_cs);
            if (g_async_section_mesh_stop) {
                LeaveCriticalSection(&g_async_section_mesh_cs);
                return 0;
            }
            if (g_async_section_mesh_job_count > 0) {
                job = g_async_section_mesh_jobs[g_async_section_mesh_job_head];
                g_async_section_mesh_job_head = (g_async_section_mesh_job_head + 1) % ASYNC_SECTION_MESH_JOB_QUEUE_MAX;
                g_async_section_mesh_job_count--;
                g_async_section_mesh_busy = 1;
                have_job = 1;
            }
            LeaveCriticalSection(&g_async_section_mesh_cs);
            if (!have_job) break;

            FlatDirectMeshBuilder mb0, mb1;
            int skip0 = 1, skip1 = 1;
            memset(&mb0, 0, sizeof(mb0));
            memset(&mb1, 0, sizeof(mb1));

            g_async_mesh_blocks = job.blocks;
            g_async_mesh_meta = job.meta;
            g_async_mesh_levels = job.levels;
            g_async_mesh_sky_light = job.sky_light;
            g_async_mesh_block_light = job.block_light;
            g_async_mesh_x0 = job.origin_x + job.cx * FLAT_RENDER_CHUNK - 1;
            g_async_mesh_y0 = FLAT_WORLD_Y_MIN + job.sy * FLAT_RENDER_SECTION - 1;
            g_async_mesh_z0 = job.origin_z + job.cz * FLAT_RENDER_CHUNK - 1;
            g_async_mesh_w = ASYNC_SECTION_MESH_W;
            g_async_mesh_h = ASYNC_SECTION_MESH_H;
            g_async_mesh_d = ASYNC_SECTION_MESH_D;
            g_async_mesh_origin_override = 1;
            g_async_mesh_origin_x = job.origin_x;
            g_async_mesh_origin_z = job.origin_z;

            g_flat_direct_capture_only = 1;
            g_flat_direct_capture_out0 = &mb0;
            g_flat_direct_capture_out1 = &mb1;
            g_flat_direct_capture_skip0 = &skip0;
            g_flat_direct_capture_skip1 = &skip1;
            g_flat_direct_capture_success = 0;
            double mesh_worker_start = now_seconds();
            rebuild_flat_section_mesh(job.sy, job.cx, job.cz);
            {
                double mesh_ms = (now_seconds() - mesh_worker_start) * 1000.0;
                if (mesh_ms < 0.0) mesh_ms = 0.0;
                g_prof_mesh_worker_last_ms = mesh_ms;
                if (g_prof_mesh_worker_samples <= 0) g_prof_mesh_worker_avg_ms = mesh_ms;
                else g_prof_mesh_worker_avg_ms = g_prof_mesh_worker_avg_ms * 0.90 + mesh_ms * 0.10;
                g_prof_mesh_worker_samples++;
            }
            int success = g_flat_direct_capture_success;
            async_section_mesh_clear_tls();

            if (success) {
                AsyncSectionMeshResult r;
                memset(&r, 0, sizeof(r));
                r.ready = 1;
                r.cx = job.cx;
                r.cz = job.cz;
                r.sy = job.sy;
                r.origin_x = job.origin_x;
                r.origin_z = job.origin_z;
                r.version = job.version;
                r.light_version = job.light_version;
                r.priority = job.priority;
                r.skip0 = skip0;
                r.skip1 = skip1;
                r.mb0 = mb0;
                r.mb1 = mb1;
                memset(&mb0, 0, sizeof(mb0));
                memset(&mb1, 0, sizeof(mb1));

                if (pex_using_d3d11()) async_mesh_push_upload_job(&r);
                else async_mesh_push_result(&r);
            } else {
                /* A failed capture used to leave g_flat_section_mesh_building set
                   forever.  The loading screen then reached N/N "Preparing chunks",
                   verify_complete failed, reset to zero, and repeated forever. */
                if (job.sy >= 0 && job.sy < FLAT_RENDER_SECTIONS_Y &&
                    job.cz >= 0 && job.cz < FLAT_RENDER_CHUNKS &&
                    job.cx >= 0 && job.cx < FLAT_RENDER_CHUNKS) {
                    g_flat_section_mesh_building[job.sy][job.cz][job.cx] = 0;
                    g_flat_section_dirty[job.sy][job.cz][job.cx] = 1;
                }
            }

            flat_direct_free_builder(&mb0);
            flat_direct_free_builder(&mb1);

            EnterCriticalSection(&g_async_section_mesh_cs);
            g_async_section_mesh_busy = 0;
            LeaveCriticalSection(&g_async_section_mesh_cs);
        }
    }
}

static void async_section_mesh_init(void) {
    if (g_async_section_mesh_initialized) return;
    g_async_section_mesh_initialized = 1;
    InitializeCriticalSection(&g_async_section_mesh_cs);
    g_async_section_mesh_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (pex_using_d3d11()) g_async_section_mesh_upload_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (!g_async_section_mesh_event || (pex_using_d3d11() && !g_async_section_mesh_upload_event)) return;
    g_async_section_mesh_stop = 0;
#if defined(PEX_PLATFORM_PSP)
    g_async_section_mesh_thread = CreateThread(NULL, 0x20000, async_section_mesh_worker_proc, NULL, 0, NULL);
#else
    g_async_section_mesh_thread = CreateThread(NULL, 0x400000, async_section_mesh_worker_proc, NULL, 0, NULL);
#endif
    if (pex_using_d3d11()) g_async_section_mesh_upload_thread = CreateThread(NULL, 0x200000, async_mesh_upload_worker, NULL, 0, NULL);
    if (g_async_section_mesh_thread) SetThreadPriority(g_async_section_mesh_thread, THREAD_PRIORITY_BELOW_NORMAL);
    if (g_async_section_mesh_upload_thread) SetThreadPriority(g_async_section_mesh_upload_thread, THREAD_PRIORITY_BELOW_NORMAL);
}

static int async_section_mesh_pending(void) {
    if (!g_async_section_mesh_initialized) return 0;
    int pending = 0;
    EnterCriticalSection(&g_async_section_mesh_cs);
    pending = g_async_section_mesh_job_count || g_async_section_mesh_busy ||
              g_async_section_mesh_upload_count || g_async_section_mesh_upload_busy ||
              g_async_section_mesh_result_count;
    LeaveCriticalSection(&g_async_section_mesh_cs);
    return pending;
}

static int async_mesh_submit(int sy, int cx, int cz, int priority) {
    if (!flat_async_section_mesh_enabled()) return 0;
    if (sy < 0 || sy >= FLAT_RENDER_SECTIONS_Y || cz < 0 || cz >= FLAT_RENDER_CHUNKS || cx < 0 || cx >= FLAT_RENDER_CHUNKS) return 0;
    if (!g_flat_world_chunk_generated[cz][cx] || !flat_chunk_light_ready(cx, cz)) return 2;
    if (g_flat_section_mesh_building[sy][cz][cx]) return 1;
    async_section_mesh_init();
    if (!g_async_section_mesh_event || !g_async_section_mesh_thread) return 0;
    if (pex_using_d3d11() && (!g_async_section_mesh_upload_event || !g_async_section_mesh_upload_thread)) return 0;

    int can_submit = 0;
    EnterCriticalSection(&g_async_section_mesh_cs);
    if (!g_async_section_mesh_stop && (g_async_section_mesh_job_count < ASYNC_SECTION_MESH_JOB_QUEUE_MAX || priority)) can_submit = 1;
    LeaveCriticalSection(&g_async_section_mesh_cs);
    if (!can_submit) return 2;

    AsyncSectionMeshJob job;
    memset(&job, 0, sizeof(job));
    job.cx = cx;
    job.cz = cz;
    job.sy = sy;
    job.origin_x = g_flat_world_origin_x;
    job.origin_z = g_flat_world_origin_z;
    job.version = g_flat_section_mesh_version[sy][cz][cx];
    job.light_version = g_flat_chunk_light_version[cz][cx];
    job.priority = priority ? 1 : 0;
    int sx0 = job.origin_x + cx * FLAT_RENDER_CHUNK - 1;
    int sy0 = FLAT_WORLD_Y_MIN + sy * FLAT_RENDER_SECTION - 1;
    int sz0 = job.origin_z + cz * FLAT_RENDER_CHUNK - 1;
    double snapshot_start = profile_begin();
    if (g_loggy_enabled) {
        g_loggy_mesh_submit_calls++;
        g_loggy_mesh_submit_snapshot_cells += ASYNC_SECTION_MESH_W * ASYNC_SECTION_MESH_H * ASYNC_SECTION_MESH_D;
    }
    for (int iy = 0; iy < ASYNC_SECTION_MESH_H; iy++) {
        int wy = sy0 + iy;
        for (int iz = 0; iz < ASYNC_SECTION_MESH_D; iz++) {
            int wz = sz0 + iz;
            for (int ix = 0; ix < ASYNC_SECTION_MESH_W; ix++) {
                int wx = sx0 + ix;
                int idx = ((iy * ASYNC_SECTION_MESH_D) + iz) * ASYNC_SECTION_MESH_W + ix;
                job.blocks[idx] = (unsigned char)flat_get_block(wx, wy, wz);
                job.meta[idx] = (unsigned char)flat_get_meta(wx, wy, wz);
                job.levels[idx] = (unsigned char)flat_get_level(wx, wy, wz);
                job.sky_light[idx] = (unsigned char)flat_get_sky_light(wx, wy, wz);
                job.block_light[idx] = (unsigned char)flat_get_block_light(wx, wy, wz);
            }
        }
    }
    profile_add_time(PROF_MESH_SUBMIT_SNAPSHOT, snapshot_start);

    EnterCriticalSection(&g_async_section_mesh_cs);
    if (!g_async_section_mesh_stop && !g_flat_section_mesh_building[sy][cz][cx]) {
        if (g_async_section_mesh_job_count >= ASYNC_SECTION_MESH_JOB_QUEUE_MAX && priority) {
            /* Player-near block/light edits must not wait behind a full streaming
               backlog.  Drop the newest queued background rebuild, clear its
               building flag, then push this edit at the queue head so the worker
               consumes it next.  A stale/dropped stream section remains dirty and
               will be resubmitted later. */
            int drop = (g_async_section_mesh_job_tail - 1 + ASYNC_SECTION_MESH_JOB_QUEUE_MAX) % ASYNC_SECTION_MESH_JOB_QUEUE_MAX;
            AsyncSectionMeshJob dropped = g_async_section_mesh_jobs[drop];
            if (dropped.sy >= 0 && dropped.sy < FLAT_RENDER_SECTIONS_Y &&
                dropped.cz >= 0 && dropped.cz < FLAT_RENDER_CHUNKS && dropped.cx >= 0 && dropped.cx < FLAT_RENDER_CHUNKS) {
                g_flat_section_mesh_building[dropped.sy][dropped.cz][dropped.cx] = 0;
            }
            memset(&g_async_section_mesh_jobs[drop], 0, sizeof(g_async_section_mesh_jobs[0]));
            g_async_section_mesh_job_tail = drop;
            g_async_section_mesh_job_count--;
        }
        if (g_async_section_mesh_job_count < ASYNC_SECTION_MESH_JOB_QUEUE_MAX) {
            if (priority) {
                g_async_section_mesh_job_head = (g_async_section_mesh_job_head - 1 + ASYNC_SECTION_MESH_JOB_QUEUE_MAX) % ASYNC_SECTION_MESH_JOB_QUEUE_MAX;
                g_async_section_mesh_jobs[g_async_section_mesh_job_head] = job;
            } else {
                g_async_section_mesh_jobs[g_async_section_mesh_job_tail] = job;
                g_async_section_mesh_job_tail = (g_async_section_mesh_job_tail + 1) % ASYNC_SECTION_MESH_JOB_QUEUE_MAX;
            }
            g_async_section_mesh_job_count++;
            g_flat_section_mesh_building[sy][cz][cx] = 1;
            can_submit = 1;
        } else {
            can_submit = 0;
        }
    } else {
        can_submit = 0;
    }
    LeaveCriticalSection(&g_async_section_mesh_cs);
    if (can_submit) { SetEvent(g_async_section_mesh_event); return 1; }
    return priority ? 2 : 2;
}

static int async_section_mesh_submit(int sy, int cx, int cz) {
    return async_mesh_submit(sy, cx, cz, 0);
}

static int async_mesh_submit_priority(int sy, int cx, int cz) {
    return async_mesh_submit(sy, cx, cz, 1);
}

static void async_section_mesh_install_ready(int max_uploads) {
    if (!g_async_section_mesh_initialized || max_uploads <= 0) return;
    while (max_uploads-- > 0) {
        AsyncSectionMeshResult r;
        memset(&r, 0, sizeof(r));
        EnterCriticalSection(&g_async_section_mesh_cs);
        if (g_async_section_mesh_result_count > 0) {
            r = g_async_section_mesh_results[g_async_section_mesh_result_head];
            memset(&g_async_section_mesh_results[g_async_section_mesh_result_head], 0, sizeof(g_async_section_mesh_results[0]));
            g_async_section_mesh_result_head = (g_async_section_mesh_result_head + 1) % ASYNC_SECTION_MESH_RESULT_QUEUE_MAX;
            g_async_section_mesh_result_count--;
        }
        LeaveCriticalSection(&g_async_section_mesh_cs);
        if (!r.ready) break;

        double install_start = profile_begin();
        int valid = 0;
        if (r.origin_x == g_flat_world_origin_x && r.origin_z == g_flat_world_origin_z &&
            r.sy >= 0 && r.sy < FLAT_RENDER_SECTIONS_Y && r.cz >= 0 && r.cz < FLAT_RENDER_CHUNKS && r.cx >= 0 && r.cx < FLAT_RENDER_CHUNKS &&
            g_flat_section_mesh_version[r.sy][r.cz][r.cx] == r.version &&
            flat_chunk_light_ready(r.cx, r.cz) &&
            g_flat_chunk_light_version[r.cz][r.cx] == r.light_version) {
            valid = 1;
        }
        if (valid) {
            int installed = 1;
            if (r.d3d11_prebuilt && pex_using_d3d11()) {
                PexMeshHandle *slot0 = (PexMeshHandle*)&g_flat_section_direct_mesh[r.sy][r.cz][r.cx][0];
                PexMeshHandle *slot1 = (PexMeshHandle*)&g_flat_section_direct_mesh[r.sy][r.cz][r.cx][1];
                if (r.skip0) d3d11_destroy_mesh_deferred(slot0);
                else if (!d3d11_adopt_mesh(slot0, &r.d3d11_mesh0)) installed = 0;
                if (r.skip1) d3d11_destroy_mesh_deferred(slot1);
                else if (!d3d11_adopt_mesh(slot1, &r.d3d11_mesh1)) installed = 0;
            } else if (flat_direct_backend()) {
                flat_direct_upload_builder(r.sy, r.cz, r.cx, 0, &r.mb0);
                flat_direct_upload_builder(r.sy, r.cz, r.cx, 1, &r.mb1);
            } else {
                flat_gl_cpu_mesh_install(r.sy, r.cz, r.cx, 0, &r.mb0, r.skip0, r.version, r.origin_x, r.origin_z);
                flat_gl_cpu_mesh_install(r.sy, r.cz, r.cx, 1, &r.mb1, r.skip1, r.version, r.origin_x, r.origin_z);
            }
            g_flat_section_mesh_building[r.sy][r.cz][r.cx] = 0;
            if (installed) {
                g_flat_section_dirty[r.sy][r.cz][r.cx] = 0;
                g_flat_section_valid[r.sy][r.cz][r.cx] = 1;
                g_flat_section_skip_pass[r.sy][r.cz][r.cx][0] = r.skip0;
                g_flat_section_skip_pass[r.sy][r.cz][r.cx][1] = r.skip1;
                g_flat_section_mesh_light_version[r.sy][r.cz][r.cx] = r.light_version;
                pex_logf_trace("chunk mesh installed sy=%d local=%d,%d skip=%d,%d version=%u", r.sy, r.cx, r.cz, r.skip0, r.skip1, r.version);
            } else {
                g_flat_section_dirty[r.sy][r.cz][r.cx] = 1;
            }
        } else if (r.sy >= 0 && r.sy < FLAT_RENDER_SECTIONS_Y && r.cz >= 0 && r.cz < FLAT_RENDER_CHUNKS && r.cx >= 0 && r.cx < FLAT_RENDER_CHUNKS) {
            g_flat_section_mesh_building[r.sy][r.cz][r.cx] = 0;
            g_flat_section_dirty[r.sy][r.cz][r.cx] = 1;
            if (g_loggy_enabled) g_loggy_mesh_stale_results++;
            pex_logf_trace("chunk mesh discarded stale result sy=%d local=%d,%d result_origin=%d,%d current_origin=%d,%d version=%u current_version=%u", r.sy, r.cx, r.cz, r.origin_x, r.origin_z, g_flat_world_origin_x, g_flat_world_origin_z, r.version, g_flat_section_mesh_version[r.sy][r.cz][r.cx]);
        }
        async_section_mesh_free_result(&r);
        profile_add_time(PROF_MESH_RESULT_INSTALL, install_start);
    }
}

static void async_section_mesh_shutdown(void) {
    if (!g_async_section_mesh_initialized) return;
    if (g_async_section_mesh_event || g_async_section_mesh_upload_event) {
        EnterCriticalSection(&g_async_section_mesh_cs);
        g_async_section_mesh_stop = 1;
        LeaveCriticalSection(&g_async_section_mesh_cs);
        if (g_async_section_mesh_event) SetEvent(g_async_section_mesh_event);
        if (g_async_section_mesh_upload_event) SetEvent(g_async_section_mesh_upload_event);
    }
    if (g_async_section_mesh_thread) {
        WaitForSingleObject(g_async_section_mesh_thread, INFINITE);
        CloseHandle(g_async_section_mesh_thread);
        g_async_section_mesh_thread = NULL;
    }
    if (g_async_section_mesh_upload_thread) {
        WaitForSingleObject(g_async_section_mesh_upload_thread, INFINITE);
        CloseHandle(g_async_section_mesh_upload_thread);
        g_async_section_mesh_upload_thread = NULL;
    }

    async_mesh_clear_results(g_async_section_mesh_upload_jobs, ASYNC_SECTION_MESH_UPLOAD_QUEUE_MAX);
    async_mesh_clear_results(g_async_section_mesh_results, ASYNC_SECTION_MESH_RESULT_QUEUE_MAX);

    if (g_async_section_mesh_event) {
        CloseHandle(g_async_section_mesh_event);
        g_async_section_mesh_event = NULL;
    }
    if (g_async_section_mesh_upload_event) {
        CloseHandle(g_async_section_mesh_upload_event);
        g_async_section_mesh_upload_event = NULL;
    }
    DeleteCriticalSection(&g_async_section_mesh_cs);
    g_async_section_mesh_initialized = 0;
    g_async_section_mesh_stop = 0;
    g_async_section_mesh_busy = 0;
    g_async_section_mesh_upload_busy = 0;
    g_async_section_mesh_job_head = g_async_section_mesh_job_tail = g_async_section_mesh_job_count = 0;
    g_async_section_mesh_upload_head = g_async_section_mesh_upload_tail = g_async_section_mesh_upload_count = 0;
    g_async_section_mesh_result_head = g_async_section_mesh_result_tail = g_async_section_mesh_result_count = 0;
    memset(g_flat_section_mesh_building, 0, sizeof(g_flat_section_mesh_building));
}

static int build_flat_visible_sections(const FlatFrustum *fr, FlatRenderSectionRef *out, int cap) {
    int count = 0;
    const PexPlayerRenderState *pr = &g_player_render_frame;
    float px = pr->prev_x + (pr->x - pr->prev_x) * g_frame_partial;
    float py = pr->prev_y + (pr->y - pr->prev_y) * g_frame_partial;
    float pz = pr->prev_z + (pr->z - pr->prev_z) * g_frame_partial;

    int pcx = flat_index((int)floorf(px)) / FLAT_RENDER_CHUNK;
    int pcz = flat_z_index((int)floorf(pz)) / FLAT_RENDER_CHUNK;
    if (pcx < 0) pcx = 0; if (pcx >= FLAT_RENDER_CHUNKS) pcx = FLAT_RENDER_CHUNKS - 1;
    if (pcz < 0) pcz = 0; if (pcz >= FLAT_RENDER_CHUNKS) pcz = FLAT_RENDER_CHUNKS - 1;

    int view_chunk_radius = generated_render_radius();
    int cz0 = pcz - view_chunk_radius, cz1 = pcz + view_chunk_radius;
    int cx0 = pcx - view_chunk_radius, cx1 = pcx + view_chunk_radius;
    if (cz0 < 0) cz0 = 0; if (cx0 < 0) cx0 = 0;
    if (cz1 >= FLAT_RENDER_CHUNKS) cz1 = FLAT_RENDER_CHUNKS - 1;
    if (cx1 >= FLAT_RENDER_CHUNKS) cx1 = FLAT_RENDER_CHUNKS - 1;

    for (int cz = cz0; cz <= cz1; cz++) {
        for (int cx = cx0; cx <= cx1; cx++) {
            if (!g_flat_world_chunk_generated[cz][cx]) continue;
            float x0 = (float)(g_flat_world_origin_x + cx * FLAT_RENDER_CHUNK) - 2.0f;
            float z0 = (float)(g_flat_world_origin_z + cz * FLAT_RENDER_CHUNK) - 2.0f;
            float x1 = x0 + (float)FLAT_RENDER_CHUNK + 4.0f;
            float z1 = z0 + (float)FLAT_RENDER_CHUNK + 4.0f;
            for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
                if (!flat_section_has_blocks(cx, cz, sy) &&
                    g_flat_section_valid[sy][cz][cx] &&
                    g_flat_section_skip_pass[sy][cz][cx][0] &&
                    g_flat_section_skip_pass[sy][cz][cx][1]) {
                    continue;
                }
                float y0 = (float)(FLAT_WORLD_Y_MIN + sy * FLAT_RENDER_SECTION) - 2.0f;
                float y1 = y0 + (float)FLAT_RENDER_SECTION + 4.0f;
                if (y1 > (float)FLAT_WORLD_Y_MAX + 3.0f) y1 = (float)FLAT_WORLD_Y_MAX + 3.0f;
                if (!flat_aabb_in_frustum(fr, x0, y0, z0, x1, y1, z1)) continue;
                if (count >= cap) return count;
                float mx = (x0 + x1) * 0.5f - px;
                float my = (y0 + y1) * 0.5f - py;
                float mz = (z0 + z1) * 0.5f - pz;
                out[count].cx = cx; out[count].cz = cz; out[count].sy = sy;
                out[count].dist2 = mx*mx + my*my + mz*mz;
                count++;
            }
        }
    }
    if (flat_renderer_sort_needed(px, py, pz, count) || g_flat_visible_sorted_count != count) {
        qsort(out, (size_t)count, sizeof(out[0]), flat_section_ref_cmp_near);
        if (count > 0) memcpy(g_flat_visible_sorted_sections, out, (size_t)count * sizeof(out[0]));
        g_flat_visible_sorted_count = count;
        flat_note_renderer_sorted(px, py, pz, count);
    } else if (count > 0) {
        memcpy(out, g_flat_visible_sorted_sections, (size_t)count * sizeof(out[0]));
    }
    return count;
}

static void flat_self_heal_visible_sections(const FlatRenderSectionRef *refs, int count) {
    double self_heal_start = profile_begin();
    if (!refs || count <= 0) {
        profile_add_time(PROF_MESH_SELF_HEAL, self_heal_start);
        return;
    }

    int direct = flat_direct_backend() ? 1 : 0;
    int async_mesh = flat_async_section_mesh_enabled() ? 1 : 0;
    int recent_edit = (g_ingame_ticks - g_flat_recent_block_mesh_dirty_tick) >= 0 &&
                      (g_ingame_ticks - g_flat_recent_block_mesh_dirty_tick) <= 12;
    int streaming = stream_generation_active() ? 1 : 0;
    int mesh_queue_active = streaming || async_section_mesh_pending() ||
                            g_prof_mesh_jobs_last > 0 || g_prof_mesh_results_last > 0 ||
                            g_prof_mesh_uploads_last > 0;

    /* Correct rule: CHAT, PAUSE and INGAME all render the same world.  Do not
       special-case overlays.  Self-heal must be cheap in the real in-game path.

       Self-heal is only a last-resort checker for stale/missing mesh handles.
       It must not do 16x16x16 block scans on the render thread.  Empty-section
       detection belongs to the normal mesh worker; if a suspicious section is
       empty, the worker will produce skip0/skip1 and mark it valid cheaply off
       the main thread. */
    enum {
        SELF_HEAL_IDLE_BACKOFF_FRAMES = 45,
        SELF_HEAL_BUSY_BACKOFF_FRAMES = 12,
        SELF_HEAL_PROBES_IDLE = 2,
        SELF_HEAL_PROBES_BUSY = 3,
        SELF_HEAL_PROBES_EDIT = 10
    };
    static int s_probe_cursor = 0;
    static int s_frame_counter = 0;
    s_frame_counter++;

    if (!recent_edit) {
        int backoff = mesh_queue_active ? SELF_HEAL_BUSY_BACKOFF_FRAMES : SELF_HEAL_IDLE_BACKOFF_FRAMES;
        if ((s_frame_counter % backoff) != 0) {
            profile_add_time(PROF_MESH_SELF_HEAL, self_heal_start);
            return;
        }
    }

    int probe_budget = recent_edit ? SELF_HEAL_PROBES_EDIT : (mesh_queue_active ? SELF_HEAL_PROBES_BUSY : SELF_HEAL_PROBES_IDLE);
    if (probe_budget > count) probe_budget = count;
    if (probe_budget <= 0) {
        profile_add_time(PROF_MESH_SELF_HEAL, self_heal_start);
        return;
    }

    int start = s_probe_cursor % count;
    s_probe_cursor = (start + probe_budget) % count;
    int probes_done = 0;

    /* Very hard budget.  This loop now only reads metadata, so it should be far
       below this, but keep the guard so this safety net can never own a frame. */
    double deadline = self_heal_start + (recent_edit ? 0.00020 : 0.00005);

    for (int i = 0; i < probe_budget; i++) {
        if (now_seconds() >= deadline) break;
        int ri = (start + i) % count;
        int cx = refs[ri].cx, cz = refs[ri].cz, sy = refs[ri].sy;
        if (cx < 0 || cx >= FLAT_RENDER_CHUNKS || cz < 0 || cz >= FLAT_RENDER_CHUNKS || sy < 0 || sy >= FLAT_RENDER_SECTIONS_Y) continue;
        if (!g_flat_world_chunk_generated[cz][cx]) continue;
        probes_done++;

        int dirty_or_invalid = (!g_flat_section_valid[sy][cz][cx] || g_flat_section_dirty[sy][cz][cx]);
        int payload_missing = 0;

        if (!dirty_or_invalid) {
            if (direct) {
                if (!g_flat_section_skip_pass[sy][cz][cx][0] && !g_flat_section_direct_mesh[sy][cz][cx][0]) payload_missing = 1;
                if (flat_separate_liquid_pass_enabled() && !g_flat_section_skip_pass[sy][cz][cx][1] && !g_flat_section_direct_mesh[sy][cz][cx][1]) payload_missing = 1;
            } else if (async_mesh) {
                if (!g_flat_section_skip_pass[sy][cz][cx][0] && !flat_gl_cpu_mesh_ready(sy, cz, cx, 0)) payload_missing = 1;
                if (flat_separate_liquid_pass_enabled() && !g_flat_section_skip_pass[sy][cz][cx][1] && !flat_gl_cpu_mesh_ready(sy, cz, cx, 1)) payload_missing = 1;
            } else {
                if (!g_flat_section_skip_pass[sy][cz][cx][0] && g_flat_section_lists[sy][cz][cx][0] == 0) payload_missing = 1;
                if (flat_separate_liquid_pass_enabled() && !g_flat_section_skip_pass[sy][cz][cx][1] && g_flat_section_lists[sy][cz][cx][1] == 0) payload_missing = 1;
            }
        }

        if (!dirty_or_invalid && !payload_missing) continue;

        /* No flat_section_has_blocks() here.  That was the 4-5 ms frame killer.
           Just mark the section for the normal async rebuild path. */
        if (payload_missing && !g_flat_section_dirty[sy][cz][cx]) {
            g_flat_section_dirty[sy][cz][cx] = 1;
            g_flat_world_chunk_dirty[cz][cx] = 1;
            g_flat_renderer_sort_dirty = 1;
            if (g_loggy_enabled) g_loggy_mesh_self_heal_missing++;
            pex_logf_trace("chunk render self-heal queued missing mesh local=%d,%d sy=%d valid=%d direct=%d async=%d", cx, cz, sy, g_flat_section_valid[sy][cz][cx], direct, async_mesh);
        }

        /* Stuck build flags are rare.  Only clear them for actually suspicious
           sections, and only occasionally, so a live worker is not disturbed. */
        if ((dirty_or_invalid || payload_missing) &&
            g_flat_section_mesh_building[sy][cz][cx] && (s_frame_counter % 120) == 0) {
            g_flat_section_mesh_building[sy][cz][cx] = 0;
            g_flat_section_dirty[sy][cz][cx] = 1;
            pex_logf_trace("chunk render self-heal cleared stuck build flag local=%d,%d sy=%d", cx, cz, sy);
        }
    }

    if (g_loggy_enabled) g_loggy_mesh_self_heal_refs += probes_done;
    profile_add_time(PROF_MESH_SELF_HEAL, self_heal_start);
}
static void rebuild_visible_flat_sections(const FlatRenderSectionRef *refs, int count) {
    flat_gl_cpu_mesh_check_origin();
    int direct = flat_direct_backend() ? 1 : 0;
    int async_mesh = flat_async_section_mesh_enabled() ? 1 : 0;
    int streaming = stream_generation_active() ? 1 : 0;
    if (g_async_section_mesh_initialized) {
        EnterCriticalSection(&g_async_section_mesh_cs);
        g_prof_mesh_jobs_last = g_async_section_mesh_job_count + (g_async_section_mesh_busy ? 1 : 0);
        g_prof_mesh_uploads_last = g_async_section_mesh_upload_count + (g_async_section_mesh_upload_busy ? 1 : 0);
        g_prof_mesh_results_last = g_async_section_mesh_result_count;
        LeaveCriticalSection(&g_async_section_mesh_cs);
    } else {
        g_prof_mesh_jobs_last = 0;
        g_prof_mesh_uploads_last = 0;
        g_prof_mesh_results_last = 0;
    }

    flat_self_heal_visible_sections(refs, count);
    int recent_edit = (g_ingame_ticks - g_flat_recent_block_mesh_dirty_tick) >= 0 &&
                      (g_ingame_ticks - g_flat_recent_block_mesh_dirty_tick) <= 12;
    int rebuilds_left = recent_edit ? 6 : 4;
    if (streaming && !recent_edit) rebuilds_left = direct ? 1 : 2;
    double deadline = now_seconds() + (recent_edit ? 0.0060 : (streaming ? 0.0015 : 0.0030));

#if defined(PEX_PLATFORM_PSP)
    if (async_mesh) async_section_mesh_install_ready(streaming ? 1 : 1);
#else
    /* Java's RenderGlobal keeps block edits at the front of worldRenderersToUpdate.
       The C renderer builds meshes off-thread, so the equivalent is to adopt a few
       already-finished priority edit meshes before background stream results.
       Keep the normal budget low to avoid frame hitches. */
    if (async_mesh) {
        int install_budget = recent_edit ? 4 : 1;
        if (!recent_edit && !streaming && g_prof_mesh_results_last > 12) install_budget = 2;
        async_section_mesh_install_ready(install_budget);
    }
#endif

    for (int i = 0; i < count && rebuilds_left > 0; i++) {
        if (now_seconds() > deadline) break;
        int cx = refs[i].cx, cz = refs[i].cz, sy = refs[i].sy;
        if (!flat_chunk_light_ready(cx, cz)) continue;
        int needs = 0;
        int light_stale = (g_flat_section_mesh_light_version[sy][cz][cx] != g_flat_chunk_light_version[cz][cx]);

        if (async_mesh) {
            if (!g_flat_section_valid[sy][cz][cx] || g_flat_section_dirty[sy][cz][cx] || light_stale) {
                needs = 1;
            } else if (direct) {
                if (!g_flat_section_skip_pass[sy][cz][cx][0] && !g_flat_section_direct_mesh[sy][cz][cx][0]) needs = 1;
                else if (flat_separate_liquid_pass_enabled() && !g_flat_section_skip_pass[sy][cz][cx][1] && !g_flat_section_direct_mesh[sy][cz][cx][1]) needs = 1;
            } else {
                if (!g_flat_section_skip_pass[sy][cz][cx][0] && !flat_gl_cpu_mesh_ready(sy, cz, cx, 0)) needs = 1;
                else if (flat_separate_liquid_pass_enabled() && !g_flat_section_skip_pass[sy][cz][cx][1] && !flat_gl_cpu_mesh_ready(sy, cz, cx, 1)) needs = 1;
            }
        } else if (direct) {
            if (!g_flat_section_valid[sy][cz][cx] || g_flat_section_dirty[sy][cz][cx] || light_stale) {
                needs = 1;
            } else if (!g_flat_section_skip_pass[sy][cz][cx][0] && !g_flat_section_direct_mesh[sy][cz][cx][0]) {
                needs = 1;
            } else if (flat_separate_liquid_pass_enabled() && !g_flat_section_skip_pass[sy][cz][cx][1] && !g_flat_section_direct_mesh[sy][cz][cx][1]) {
                needs = 1;
            }
        } else {
            if (g_flat_section_lists[sy][cz][cx][0] == 0 ||
                g_flat_section_lists[sy][cz][cx][1] == 0 ||
                g_flat_section_dirty[sy][cz][cx] ||
                light_stale) {
                needs = 1;
            }
        }

        if (needs) {
            if (async_mesh) {
                /* Never fall back to rebuilding a direct section mesh on the
                   render/game thread just because the worker queue is full.
                   Leaving the section dirty for the next frame is cheaper than
                   doing the heavy meshing work on the main thread during chunk
                   streaming. */
                int near_player_edit = (refs[i].dist2 < (48.0f * 48.0f));
                if (near_player_edit) (void)async_mesh_submit_priority(sy, cx, cz);
                else (void)async_section_mesh_submit(sy, cx, cz);
            } else {
                pex_logf_trace("chunk mesh sync rebuild sy=%d local=%d,%d", sy, cx, cz);
                rebuild_flat_section_list(sy, cx, cz);
            }
            rebuilds_left--;
        }
    }
}

static int flat_section_needs_mesh_rebuild(int sy, int cz, int cx) {
    if (sy < 0 || sy >= FLAT_RENDER_SECTIONS_Y || cz < 0 || cz >= FLAT_RENDER_CHUNKS || cx < 0 || cx >= FLAT_RENDER_CHUNKS) return 0;
    if (g_flat_world_chunk_generated[cz][cx] && !flat_chunk_light_ready(cx, cz)) return 0;
    if (g_flat_world_chunk_generated[cz][cx] &&
        g_flat_section_mesh_light_version[sy][cz][cx] != g_flat_chunk_light_version[cz][cx]) return 1;
    if (g_flat_section_skip_pass[sy][cz][cx][0] &&
        (!flat_separate_liquid_pass_enabled() || g_flat_section_skip_pass[sy][cz][cx][1]) &&
        !g_flat_section_dirty[sy][cz][cx]) return 0;
    if (flat_async_section_mesh_enabled()) {
        if (!g_flat_section_valid[sy][cz][cx] || g_flat_section_dirty[sy][cz][cx]) return 1;
        if (flat_direct_backend()) {
            if (!g_flat_section_skip_pass[sy][cz][cx][0] && !g_flat_section_direct_mesh[sy][cz][cx][0]) return 1;
            if (flat_separate_liquid_pass_enabled() && !g_flat_section_skip_pass[sy][cz][cx][1] && !g_flat_section_direct_mesh[sy][cz][cx][1]) return 1;
        } else {
            if (!g_flat_section_skip_pass[sy][cz][cx][0] && !flat_gl_cpu_mesh_ready(sy, cz, cx, 0)) return 1;
            if (flat_separate_liquid_pass_enabled() && !g_flat_section_skip_pass[sy][cz][cx][1] && !flat_gl_cpu_mesh_ready(sy, cz, cx, 1)) return 1;
        }
        return 0;
    }
    if (flat_direct_backend()) {
        if (!g_flat_section_valid[sy][cz][cx] || g_flat_section_dirty[sy][cz][cx]) return 1;
        if (!g_flat_section_skip_pass[sy][cz][cx][0] && !g_flat_section_direct_mesh[sy][cz][cx][0]) return 1;
        if (flat_separate_liquid_pass_enabled() && !g_flat_section_skip_pass[sy][cz][cx][1] && !g_flat_section_direct_mesh[sy][cz][cx][1]) return 1;
        return 0;
    }
    if (g_flat_section_lists[sy][cz][cx][0] == 0 ||
        g_flat_section_lists[sy][cz][cx][1] == 0 ||
        g_flat_section_dirty[sy][cz][cx]) return 1;
    return 0;
}

static int g_worldgen_mesh_prep_active = 0;
static int g_worldgen_mesh_prep_count = 0;
static int g_worldgen_mesh_prep_index = 0;
static FlatRenderSectionRef g_worldgen_mesh_prep_refs[FLAT_MAX_VISIBLE_SECTIONS];

static void worldgen_mesh_prep_reset(void) {
    g_worldgen_mesh_prep_active = 0;
    g_worldgen_mesh_prep_count = 0;
    g_worldgen_mesh_prep_index = 0;
}

static void worldgen_mesh_prep_build_list(void) {
    g_worldgen_mesh_prep_count = 0;
    g_worldgen_mesh_prep_index = 0;

    float px = g_player_x;
    float py = g_player_y;
    float pz = g_player_z;
    int pcx = flat_index((int)floorf(px)) / FLAT_RENDER_CHUNK;
    int pcz = flat_z_index((int)floorf(pz)) / FLAT_RENDER_CHUNK;
    if (pcx < 0) pcx = 0; if (pcx >= FLAT_RENDER_CHUNKS) pcx = FLAT_RENDER_CHUNKS - 1;
    if (pcz < 0) pcz = 0; if (pcz >= FLAT_RENDER_CHUNKS) pcz = FLAT_RENDER_CHUNKS - 1;

    int r = generated_render_radius();
    int cx0 = pcx - r, cx1 = pcx + r;
    int cz0 = pcz - r, cz1 = pcz + r;
    if (g_worldgen.active) {
        /* Java 1.2.5 preloadWorld() drains lighting before entering, but
           RenderGlobal.updateRenderers() only rebuilds a small prioritized set per
           frame.  Prepare only the near spawn view here: enough to stop first-frame
           brightness popping, not every generated section in the 49-chunk island. */
        int prep_r = 2;
        cx0 = pcx - prep_r; cx1 = pcx + prep_r;
        cz0 = pcz - prep_r; cz1 = pcz + prep_r;
    }
    if (cx0 < 0) cx0 = 0;
    if (cz0 < 0) cz0 = 0;
    if (cx1 >= FLAT_RENDER_CHUNKS) cx1 = FLAT_RENDER_CHUNKS - 1;
    if (cz1 >= FLAT_RENDER_CHUNKS) cz1 = FLAT_RENDER_CHUNKS - 1;

    for (int cz = cz0; cz <= cz1; cz++) {
        for (int cx = cx0; cx <= cx1; cx++) {
            if (!g_flat_world_chunk_generated[cz][cx] || !flat_chunk_light_ready(cx, cz)) continue;
            for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
                if (!flat_section_has_blocks(cx, cz, sy)) {
                    flat_mark_generated_section(cx, cz, sy);
                    continue;
                }
                if (!flat_section_needs_mesh_rebuild(sy, cz, cx)) continue;
                if (g_worldgen_mesh_prep_count >= FLAT_MAX_VISIBLE_SECTIONS) break;
                float x0 = (float)(g_flat_world_origin_x + cx * FLAT_RENDER_CHUNK);
                float z0 = (float)(g_flat_world_origin_z + cz * FLAT_RENDER_CHUNK);
                float y0 = (float)(FLAT_WORLD_Y_MIN + sy * FLAT_RENDER_SECTION);
                float mx = x0 + 8.0f - px;
                float my = y0 + 8.0f - py;
                float mz = z0 + 8.0f - pz;
                g_worldgen_mesh_prep_refs[g_worldgen_mesh_prep_count].cx = cx;
                g_worldgen_mesh_prep_refs[g_worldgen_mesh_prep_count].cz = cz;
                g_worldgen_mesh_prep_refs[g_worldgen_mesh_prep_count].sy = sy;
                g_worldgen_mesh_prep_refs[g_worldgen_mesh_prep_count].dist2 = mx * mx + my * my + mz * mz;
                g_worldgen_mesh_prep_count++;
            }
        }
    }
    qsort(g_worldgen_mesh_prep_refs, (size_t)g_worldgen_mesh_prep_count,
          sizeof(g_worldgen_mesh_prep_refs[0]), flat_section_ref_cmp_near);
    g_worldgen_mesh_prep_active = 1;
}

static int worldgen_mesh_verify_complete(void) {
    if (!g_worldgen_mesh_prep_active) return 0;
    for (int i = 0; i < g_worldgen_mesh_prep_count; ++i) {
        FlatRenderSectionRef *r = &g_worldgen_mesh_prep_refs[i];
        if (flat_section_needs_mesh_rebuild(r->sy, r->cz, r->cx)) return 0;
    }
    return 1;
}

static int worldgen_mesh_prep_step(int max_rebuilds) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_FAST_WORLD) && PEX_PSP_FAST_WORLD
    (void)max_rebuilds;
    g_worldgen_mesh_prep_active = 1;
    g_worldgen_mesh_prep_count = 1;
    g_worldgen_mesh_prep_index = 1;
    return 1;
#endif
    if (!g_worldgen_mesh_prep_active) worldgen_mesh_prep_build_list();
    if (max_rebuilds < 1) max_rebuilds = 1;

    if (g_worldgen.active) {
        /* Do not block on the whole preload island, but do require the near spawn
           renderers to be built from the final light version.  This is the missing
           light->mesh synchronization that made default spawn chunks brighten only
           after entering the world or after a block edit. */
        if (flat_async_section_mesh_enabled()) {
#if defined(PEX_PLATFORM_WII)
            int built = 0;
            while (g_worldgen_mesh_prep_index < g_worldgen_mesh_prep_count && built < max_rebuilds) {
                FlatRenderSectionRef *r = &g_worldgen_mesh_prep_refs[g_worldgen_mesh_prep_index++];
                if (flat_section_needs_mesh_rebuild(r->sy, r->cz, r->cx)) {
                    rebuild_flat_section_list(r->sy, r->cx, r->cz);
                    built++;
                }
            }
            return (g_worldgen_mesh_prep_index >= g_worldgen_mesh_prep_count);
#else
            async_section_mesh_install_ready(max_rebuilds);
            int submitted = 0;
            while (g_worldgen_mesh_prep_index < g_worldgen_mesh_prep_count && submitted < max_rebuilds) {
                FlatRenderSectionRef *r = &g_worldgen_mesh_prep_refs[g_worldgen_mesh_prep_index];
                if (!flat_section_needs_mesh_rebuild(r->sy, r->cz, r->cx)) {
                    g_worldgen_mesh_prep_index++;
                    continue;
                }
                if (g_flat_section_mesh_building[r->sy][r->cz][r->cx]) break;
                /* Loading/pre-entry prep must never use the priority path.  The
                   priority submitter is allowed to drop queued background jobs; when
                   the old Preparing Chunks gate used it, the prep index advanced
                   past sections whose jobs were silently discarded, verify_complete
                   failed, reset to zero, and the loading screen looped. */
                int submit_result = async_section_mesh_submit(r->sy, r->cx, r->cz);
                if (submit_result == 2) break;
                if (submit_result == 0) rebuild_flat_section_list(r->sy, r->cx, r->cz);
                g_worldgen_mesh_prep_index++;
                submitted++;
            }
            if (g_worldgen_mesh_prep_index >= g_worldgen_mesh_prep_count) {
                async_section_mesh_install_ready(max_rebuilds);

                int target_still_building = 0;
                for (int i = 0; i < g_worldgen_mesh_prep_count; ++i) {
                    FlatRenderSectionRef *r = &g_worldgen_mesh_prep_refs[i];
                    if (g_flat_section_mesh_building[r->sy][r->cz][r->cx]) {
                        target_still_building = 1;
                        break;
                    }
                }
                if (target_still_building) return 0;

                if (!worldgen_mesh_verify_complete()) {
                    /* Do not loop a full N/N Preparing Chunks pass forever.  Mark the
                       remaining sections dirty and let the normal Java-style renderer
                       update queue rebuild them after entry.  Lighting has already
                       settled by this phase, so any runtime rebuild uses final light. */
                    for (int i = 0; i < g_worldgen_mesh_prep_count; ++i) {
                        FlatRenderSectionRef *r = &g_worldgen_mesh_prep_refs[i];
                        if (flat_section_needs_mesh_rebuild(r->sy, r->cz, r->cx)) {
                            g_flat_section_dirty[r->sy][r->cz][r->cx] = 1;
                            g_flat_section_mesh_building[r->sy][r->cz][r->cx] = 0;
                        }
                    }
                }
                return 1;
            }
            return 0;
#endif
        }
    }

    if (flat_async_section_mesh_enabled()) {
#if defined(PEX_PLATFORM_WII)
        /* Wii: do not enter the async section-mesh worker while loading a world.
           The previous build crashed at flat_get_block() from the worker snapshot
           path during "Preparing chunks". Build a small number of direct GX
           meshes synchronously instead. */
        int built = 0;
        while (g_worldgen_mesh_prep_index < g_worldgen_mesh_prep_count && built < max_rebuilds) {
            FlatRenderSectionRef *r = &g_worldgen_mesh_prep_refs[g_worldgen_mesh_prep_index++];
            if (flat_section_needs_mesh_rebuild(r->sy, r->cz, r->cx)) {
                wii_debug_logf("wii sync prep mesh sy=%d cx=%d cz=%d", r->sy, r->cx, r->cz);
                rebuild_flat_section_list(r->sy, r->cx, r->cz);
                built++;
            }
        }
        return (g_worldgen_mesh_prep_index >= g_worldgen_mesh_prep_count);
#else
        /* Loading/pre-entry chunk preparation is not gameplay-critical, so drain
           a batch of completed worker uploads here.  Runtime rendering still
           uses the one-install-per-frame budget in rebuild_visible_flat_sections. */
        async_section_mesh_install_ready(max_rebuilds);
        int submitted = 0;
        while (g_worldgen_mesh_prep_index < g_worldgen_mesh_prep_count && submitted < max_rebuilds) {
            FlatRenderSectionRef *r = &g_worldgen_mesh_prep_refs[g_worldgen_mesh_prep_index];
            if (!flat_section_needs_mesh_rebuild(r->sy, r->cz, r->cx)) {
                g_worldgen_mesh_prep_index++;
                continue;
            }
            if (g_flat_section_mesh_building[r->sy][r->cz][r->cx]) break;
            int submit_result = async_section_mesh_submit(r->sy, r->cx, r->cz);
            if (submit_result == 2) break;
            if (submit_result == 0) {
                rebuild_flat_section_list(r->sy, r->cx, r->cz);
            }
            g_worldgen_mesh_prep_index++;
            submitted++;
        }
        if (g_worldgen_mesh_prep_index >= g_worldgen_mesh_prep_count) {
            async_section_mesh_install_ready(max_rebuilds);
            if (async_section_mesh_pending()) return 0;
            if (!worldgen_mesh_verify_complete()) {
                g_worldgen_mesh_prep_index = 0;
                return 0;
            }
            return 1;
        }
        return 0;
#endif
    }

    int built = 0;
    while (g_worldgen_mesh_prep_index < g_worldgen_mesh_prep_count && built < max_rebuilds) {
        FlatRenderSectionRef *r = &g_worldgen_mesh_prep_refs[g_worldgen_mesh_prep_index++];
        if (flat_section_needs_mesh_rebuild(r->sy, r->cz, r->cx)) {
            rebuild_flat_section_list(r->sy, r->cx, r->cz);
            built++;
        }
    }
    if (g_worldgen_mesh_prep_index >= g_worldgen_mesh_prep_count) {
        if (!worldgen_mesh_verify_complete()) {
            g_worldgen_mesh_prep_index = 0;
            return 0;
        }
        return 1;
    }
    return 0;
}

static int worldgen_mesh_prep_total(void) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_FAST_WORLD) && PEX_PSP_FAST_WORLD
    return 1;
#endif
    if (!g_worldgen_mesh_prep_active) worldgen_mesh_prep_build_list();
    return g_worldgen_mesh_prep_count;
}

static int worldgen_mesh_prep_done(void) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_FAST_WORLD) && PEX_PSP_FAST_WORLD
    return 1;
#endif
    if (!g_worldgen_mesh_prep_active) worldgen_mesh_prep_build_list();
    return g_worldgen_mesh_prep_index;
}


static void draw_flat_section_passes_direct(const FlatRenderSectionRef *refs, int count) {
    PexRendererBackend *rb = flat_direct_backend();
    if (!rb) return;
    PexRenderState st;
    flat_direct_make_state(&st, 0);
    for (int i = 0; i < count; i++) {
        int cx = refs[i].cx, cz = refs[i].cz, sy = refs[i].sy;
        PexMeshHandle h = g_flat_section_direct_mesh[sy][cz][cx][0];
        if (g_flat_section_valid[sy][cz][cx] && !g_flat_section_skip_pass[sy][cz][cx][0] && h) rb->draw_mesh(h, &st);
    }
}

static void draw_translucent_sections_direct(const FlatRenderSectionRef *refs, int count) {
    PexRendererBackend *rb = flat_direct_backend();
    if (!rb || !flat_separate_liquid_pass_enabled()) return;
    PexRenderState st;
    flat_direct_make_state(&st, 1);
    for (int i = count - 1; i >= 0; i--) {
        int cx = refs[i].cx, cz = refs[i].cz, sy = refs[i].sy;
        PexMeshHandle h = g_flat_section_direct_mesh[sy][cz][cx][1];
        if (g_flat_section_valid[sy][cz][cx] && !g_flat_section_skip_pass[sy][cz][cx][1] && h) rb->draw_mesh(h, &st);
    }
}

static void draw_flat_section_passes_gl_cpu(const FlatRenderSectionRef *refs, int count) {
    GLint old_shade_model = pex_gl_save_shade_model();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    /* Java EntityRenderer.renderWorld() switches terrain to GL_SMOOTH when
       ambient occlusion/smooth lighting is active.  The C mesh stores block
       light/AO as per-vertex colors; real OpenGL must interpolate those colors
       or block faces collapse into the diagonal dark triangles seen in the
       OpenGL screenshots. */
    pex_gl_shade_model_smooth();
    /* The captured terrain mesh reuses the old immediate-mode face emitters.
       Their quad winding is not consistent enough for OpenGL back-face culling,
       even though the D3D backends draw the same mesh fine without culling.
       Culling here removed random mountain/terrain faces and made OpenGL look
       like x-ray.  Keep depth test/write on, but render both sides. */
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    glDepthMask(GL_TRUE);
    for (int i = 0; i < count; i++) {
        int cx = refs[i].cx, cz = refs[i].cz, sy = refs[i].sy;
        if (g_flat_section_valid[sy][cz][cx] && !g_flat_section_skip_pass[sy][cz][cx][0] &&
            flat_gl_cpu_mesh_drawable(sy, cz, cx, 0)) {
            flat_gl_draw_cpu_mesh(&g_flat_section_gl_cpu_mesh[sy][cz][cx][0]);
        }
    }
    pex_gl_restore_shade_model(old_shade_model);
    glDisable(GL_ALPHA_TEST);
    glColor4f(1,1,1,1);
}

static void draw_translucent_sections_gl_cpu(const FlatRenderSectionRef *refs, int count) {
    if (!flat_separate_liquid_pass_enabled()) return;
    GLint old_shade_model = pex_gl_save_shade_model();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    pex_gl_shade_model_smooth();
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (int i = count - 1; i >= 0; i--) {
        int cx = refs[i].cx, cz = refs[i].cz, sy = refs[i].sy;
        if (g_flat_section_valid[sy][cz][cx] && !g_flat_section_skip_pass[sy][cz][cx][1] &&
            flat_gl_cpu_mesh_drawable(sy, cz, cx, 1)) {
            flat_gl_draw_cpu_mesh(&g_flat_section_gl_cpu_mesh[sy][cz][cx][1]);
        }
    }
    pex_gl_restore_shade_model(old_shade_model);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glColor4f(1,1,1,1);
}

static void draw_flat_section_passes(const FlatRenderSectionRef *refs, int count) {
    if (flat_direct_backend()) { draw_flat_section_passes_direct(refs, count); return; }
    if (flat_async_section_mesh_enabled()) { draw_flat_section_passes_gl_cpu(refs, count); return; }
    GLint old_shade_model = pex_gl_save_shade_model();
    pex_gl_shade_model_smooth();
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_ALPHA_TEST);
    glDepthMask(GL_TRUE);
    for (int i = 0; i < count; i++) {
        int cx = refs[i].cx, cz = refs[i].cz, sy = refs[i].sy;
        if (g_flat_section_valid[sy][cz][cx] && !g_flat_section_skip_pass[sy][cz][cx][0] &&
            g_flat_section_lists[sy][cz][cx][0] != 0) {
            glCallList(g_flat_section_lists[sy][cz][cx][0]);
        }
    }
    pex_gl_restore_shade_model(old_shade_model);
    glColor4f(1,1,1,1);
}

static void draw_translucent_sections(const FlatRenderSectionRef *refs, int count) {
    if (!flat_separate_liquid_pass_enabled()) return;
    if (flat_direct_backend()) { draw_translucent_sections_direct(refs, count); return; }
    if (flat_async_section_mesh_enabled()) { draw_translucent_sections_gl_cpu(refs, count); return; }
    GLint old_shade_model = pex_gl_save_shade_model();
    pex_gl_shade_model_smooth();
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (int i = count - 1; i >= 0; i--) {
        int cx = refs[i].cx, cz = refs[i].cz, sy = refs[i].sy;
        if (g_flat_section_valid[sy][cz][cx] && !g_flat_section_skip_pass[sy][cz][cx][1] &&
            g_flat_section_lists[sy][cz][cx][1] != 0) {
            glCallList(g_flat_section_lists[sy][cz][cx][1]);
        }
    }
    pex_gl_restore_shade_model(old_shade_model);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glColor4f(1,1,1,1);
}

static void rebuild_flat_geometry(void) {
    /* Kept for compatibility: mark/rebuild all chunks. */
    for (int cz = 0; cz < FLAT_RENDER_CHUNKS; cz++) {
        for (int cx = 0; cx < FLAT_RENDER_CHUNKS; cx++) {
            g_flat_world_chunk_dirty[cz][cx] = 1;
            rebuild_flat_chunk_list(cx, cz);
        }
    }
    g_flat_world_geometry_dirty = 0;
}



static void draw_armor_model_for_slots(const ItemStack armor_slots[4],
                                       float head_pivot_y, float leg_pivot_y, float leg_pivot_z,
                                       float body_pitch, float head_pitch, float head_yaw,
                                       float right_arm_pitch, float right_arm_yaw, float right_arm_roll,
                                       float left_arm_pitch, float left_arm_yaw, float left_arm_roll,
                                       float right_leg_pitch, float left_leg_pitch) {
    if (!armor_slots) return;

    for (int pass = 0; pass < 4; pass++) {
        int armor_index = 3 - pass;
        const ItemStack *st = &armor_slots[armor_index];
        if (stack_empty(st) || armor_stack_type(st->id) < 0) continue;
        int mat = armor_stack_material_index(st->id);
        int layer = (pass == 2) ? 1 : 0;
        if (mat < 0 || mat >= 5 || !tex_armor[mat][layer].id) continue;

        glBindTexture(GL_TEXTURE_2D, tex_armor[mat][layer].id);
        steve_set_texture_dims(&tex_armor[mat][layer]);
        steve_set_tint(1.0f, 1.0f, 1.0f);
        glColor4f(1, 1, 1, 1);

        if (pass == 0) {
            /* Helmet pass: ModelBiped(1.0F) head plus helmet/hat cube. */
            steve_part(0, 0, 0, head_pivot_y, 0, -4, -8, -4, 8, 8, 8, 1.0f, 0, head_pitch, head_yaw, 0);
            steve_part(32, 0, 0, head_pivot_y, 0, -4, -8, -4, 8, 8, 8, 1.5f, 0, head_pitch, head_yaw, 0);
        } else if (pass == 1) {
            /* Chestplate pass: torso and both arms from armor layer 1. */
            steve_part(16, 16, 0, 0, 0, -4, 0, -2, 8, 12, 4, 1.0f, 0, body_pitch, 0, 0);
            steve_part(40, 16, -5, 2, 0, -3, -2, -2, 4, 12, 4, 1.0f, 0, right_arm_pitch, right_arm_yaw, right_arm_roll);
            steve_part(40, 16,  5, 2, 0, -1, -2, -2, 4, 12, 4, 1.0f, 1, left_arm_pitch, left_arm_yaw, left_arm_roll);
        } else if (pass == 2) {
            /* Leggings pass: Beta uses armor layer 2 with the slimmer 0.5F model and includes the waist. */
            steve_part(16, 16, 0, 0, 0, -4, 0, -2, 8, 12, 4, 0.5f, 0, body_pitch, 0, 0);
            steve_part(0, 16, -2, leg_pivot_y, leg_pivot_z, -2, 0, -2, 4, 12, 4, 0.5f, 0, right_leg_pitch, 0, 0);
            steve_part(0, 16,  2, leg_pivot_y, leg_pivot_z, -2, 0, -2, 4, 12, 4, 0.5f, 1, left_leg_pitch, 0, 0);
        } else {
            /* Boots pass: both legs from armor layer 1. */
            steve_part(0, 16, -2, leg_pivot_y, leg_pivot_z, -2, 0, -2, 4, 12, 4, 1.0f, 0, right_leg_pitch, 0, 0);
            steve_part(0, 16,  2, leg_pivot_y, leg_pivot_z, -2, 0, -2, 4, 12, 4, 1.0f, 1, left_leg_pitch, 0, 0);
        }
    }
}

static void draw_local_player_held_item_125(float x, float eye_y, float z,
                                            float arm_pitch, float arm_yaw, float arm_roll);

static void draw_third_person_player(void) {
    if (!g_third_person_view || !tex_steve.id) return;

    const PexPlayerRenderState *pr = &g_player_render_frame;
    float x = pr->prev_x + (pr->x - pr->prev_x) * g_frame_partial;
    float eye_y = pr->prev_y + (pr->y - pr->prev_y) * g_frame_partial;
    float z = pr->prev_z + (pr->z - pr->prev_z) * g_frame_partial;
    float yaw = lerp_angle(pr->prev_yaw, pr->yaw, g_frame_partial);
    float pitch = pr->prev_pitch + (pr->pitch - pr->prev_pitch) * g_frame_partial;
    float feet_y = eye_y - 1.62f;
    int sneaking = (g_screen == SCREEN_INGAME && key_down_vk(g_opts.keys[5]));

    /* Source reference: dh.java model animation:
       arms/legs use cos(limbSwing*0.6662) with opposite phase. */
    float move = pr->prev_limb_swing_amount + (pr->limb_swing_amount - pr->prev_limb_swing_amount) * g_frame_partial;
    if (move < 0.0f) move = 0.0f;
    if (move > 1.0f) move = 1.0f;
    float limb = pr->prev_limb_swing + (pr->limb_swing - pr->prev_limb_swing) * g_frame_partial;
    float idle = (float)pr->ingame_ticks + g_frame_partial;

    float right_arm_pitch = cosf(limb * 0.6662f + (float)M_PI) * 2.0f * move * 0.5f * 57.29578f;
    float left_arm_pitch  = cosf(limb * 0.6662f) * 2.0f * move * 0.5f * 57.29578f;
    float right_leg_pitch = cosf(limb * 0.6662f) * 1.4f * move * 57.29578f;
    float left_leg_pitch  = cosf(limb * 0.6662f + (float)M_PI) * 1.4f * move * 57.29578f;

    float right_arm_yaw = 0.0f;
    float left_arm_yaw = 0.0f;
    float body_pitch = 0.0f;
    float head_pivot_y = 0.0f;
    float arm_extra_pitch = 0.0f;
    float leg_pivot_y = 12.0f;
    float leg_pivot_z = 0.0f;

    float right_arm_roll = (cosf(idle * 0.09f) * 0.05f + 0.05f) * 57.29578f;
    float left_arm_roll = -right_arm_roll;
    right_arm_pitch += sinf(idle * 0.067f) * 0.05f * 57.29578f;
    left_arm_pitch -= sinf(idle * 0.067f) * 0.05f * 57.29578f;

    if (sneaking) {
        body_pitch = 0.5f * 57.29578f;
        arm_extra_pitch = 0.4f * 57.29578f;
        leg_pivot_y = 9.0f;
        leg_pivot_z = 4.0f;
        head_pivot_y = 1.0f;
    }

    /* Make local F5 show the same attack/mining arm animation that other players see. */
    float swing = g_prev_hand_swing + (g_hand_swing - g_prev_hand_swing) * g_frame_partial;
    if (swing < 0.0f) swing = 0.0f;
    if (swing > 1.0f) swing = 1.0f;
    if (g_hand_swing_active || swing > 0.0f || g_breaking_block) {
        float body_yaw = sinf(sqrtf(swing) * (float)M_PI * 2.0f) * 0.2f;
        right_arm_yaw += body_yaw * 3.0f * 57.29578f;
        left_arm_yaw += body_yaw * 57.29578f;
        left_arm_pitch += body_yaw * 57.29578f;
        float v = 1.0f - swing;
        v *= v; v *= v; v = 1.0f - v;
        float swing_sin = sinf(v * (float)M_PI);
        float pitch_rad = pitch / 57.29578f;
        float head_term = sinf(swing * (float)M_PI) * -(pitch_rad - 0.7f) * 0.75f;
        right_arm_pitch -= (swing_sin * 1.2f + head_term) * 57.29578f;
        right_arm_roll = sinf(swing * (float)M_PI) * -0.4f * 57.29578f;
    }
    right_arm_pitch += arm_extra_pitch;
    left_arm_pitch += arm_extra_pitch;

    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_steve.id);
    steve_set_texture_dims(&tex_steve);
    {
        float entity_br = entity_light_factor_at(x, eye_y, z);
        if (pr->hurt_time > 0 || pr->dead) steve_set_tint(entity_br, entity_br * 0.35f, entity_br * 0.35f);
        else steve_set_tint(entity_br, entity_br, entity_br);
    }
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
    steve_part(16, 16, 0, 0, 0, -4, 0, -2, 8, 12, 4, 0.0f, 0, body_pitch, 0, 0);
    steve_part(40, 16, -5, 2, 0, -3, -2, -2, 4, 12, 4, 0.0f, 0, right_arm_pitch, right_arm_yaw, right_arm_roll);
    steve_part(40, 16,  5, 2, 0, -1, -2, -2, 4, 12, 4, 0.0f, 1, left_arm_pitch, left_arm_yaw, left_arm_roll);
    steve_part(0, 16, -2, leg_pivot_y, leg_pivot_z, -2, 0, -2, 4, 12, 4, 0.0f, 0, right_leg_pitch, 0, 0);
    steve_part(0, 16,  2, leg_pivot_y, leg_pivot_z, -2, 0, -2, 4, 12, 4, 0.0f, 1, left_leg_pitch, 0, 0);
    steve_part(0, 0, 0, head_pivot_y, 0, -4, -8, -4, 8, 8, 8, 0.0f, 0, pitch, 0, 0);
    steve_part(32, 0, 0, head_pivot_y, 0, -4, -8, -4, 8, 8, 8, 0.5f, 0, pitch, 0, 0);
    draw_armor_model_for_slots(g_armor_inventory, head_pivot_y, leg_pivot_y, leg_pivot_z,
                               body_pitch, pitch, 0.0f,
                               right_arm_pitch, right_arm_yaw, right_arm_roll,
                               left_arm_pitch, left_arm_yaw, left_arm_roll,
                               right_leg_pitch, left_leg_pitch);
    draw_local_player_held_item_125(x, eye_y, z, right_arm_pitch, right_arm_yaw, right_arm_roll);

    glDisable(GL_ALPHA_TEST);
    steve_set_tint(1.0f, 1.0f, 1.0f);
    glColor4f(1, 1, 1, 1);
    glPopMatrix();
}




static int multiplayer_renderable_remote_player_count(void) {
    if (!g_mp_connected) return 0;
    int count = 0;
    for (int i = 0; i < PEX_NET_MAX_PLAYERS; i++) {
        PexNetRenderPlayerState *r = &g_mp_render_players[i];
        if (!r->active || r->skin_only || r->player_id <= 0 || r->player_id == g_mp_player_id) continue;
        if (r->health <= 0) continue;
        count++;
    }
    return count;
}

static int pex_item_full3d_125(int id) {
    switch (id) {
        case ITEM_STICK:
        case ITEM_BONE:
        case ITEM_FISHING_ROD:
        case ITEM_SWORD_IRON: case ITEM_WOODEN_SWORD: case ITEM_STONE_SWORD:
        case ITEM_SWORD_DIAMOND: case ITEM_SWORD_GOLD:
        case ITEM_SHOVEL_IRON: case ITEM_WOODEN_SHOVEL: case ITEM_STONE_SHOVEL:
        case ITEM_SHOVEL_DIAMOND: case ITEM_SHOVEL_GOLD:
        case ITEM_PICKAXE_IRON: case ITEM_WOODEN_PICKAXE: case ITEM_STONE_PICKAXE:
        case ITEM_PICKAXE_DIAMOND: case ITEM_PICKAXE_GOLD:
        case ITEM_AXE_IRON: case ITEM_WOODEN_AXE: case ITEM_STONE_AXE:
        case ITEM_AXE_DIAMOND: case ITEM_AXE_GOLD:
        case ITEM_HOE_WOOD: case ITEM_HOE_STONE: case ITEM_HOE_IRON:
        case ITEM_HOE_DIAMOND: case ITEM_HOE_GOLD:
            return 1;
        default:
            return 0;
    }
}

static int pex_item_rotates_around_when_rendering_125(int id) {
    return id == ITEM_FISHING_ROD;
}

static void multiplayer_apply_right_arm_postrender_125(float arm_pitch, float arm_yaw, float arm_roll) {
    /* Same ModelRenderer.postRender order used by bipedRightArm: translate to
       the right-arm pivot, then apply Z/Y/X rotations.  The player root is
       already in the Java ModelBiped coordinate system at 1/16 scale. */
    glTranslatef(-5.0f * 0.0625f, 2.0f * 0.0625f, 0.0f);
    if (arm_roll != 0.0f) glRotatef(arm_roll, 0.0f, 0.0f, 1.0f);
    if (arm_yaw != 0.0f) glRotatef(arm_yaw, 0.0f, 1.0f, 0.0f);
    if (arm_pitch != 0.0f) glRotatef(arm_pitch, 1.0f, 0.0f, 0.0f);
}

static void draw_biped_held_item_125(int id, int count, float light_x, float light_y, float light_z,
                                       float arm_pitch, float arm_yaw, float arm_roll) {
    if (id <= 0 || count <= 0) return;

    entity_item_light_prepare(light_x, light_y, light_z);
    glPushMatrix();
    multiplayer_apply_right_arm_postrender_125(arm_pitch, arm_yaw, arm_roll);
    glTranslatef(-1.0f / 16.0f, 7.0f / 16.0f, 1.0f / 16.0f);

    if (render_item_as_block_id(id) && tex_terrain.id) {
        /* RenderBiped held 3D-block branch. */
        float s = 0.5f * (12.0f / 16.0f);
        glTranslatef(0.0f, 3.0f / 16.0f, -(5.0f / 16.0f));
        glRotatef(20.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
        glScalef(s, -s, s);
        glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
        draw_block_item_model(id, -0.5f, -0.5f, -0.5f);
    } else if (id == ITEM_BOW && tex_items.id) {
        float s = 10.0f / 16.0f;
        glTranslatef(0.0f, 2.0f / 16.0f, 5.0f / 16.0f);
        glRotatef(-20.0f, 0.0f, 1.0f, 0.0f);
        glScalef(s, -s, s);
        glRotatef(-100.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
        draw_item3d_from_texture(&tex_items, item_icon_tile(id));
    } else if (pex_item_full3d_125(id) && tex_items.id) {
        float s = 10.0f / 16.0f;
        if (pex_item_rotates_around_when_rendering_125(id)) {
            glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
            glTranslatef(0.0f, -(2.0f / 16.0f), 0.0f);
        }
        glTranslatef(0.0f, 3.0f / 16.0f, 0.0f);
        glScalef(s, -s, s);
        glRotatef(-100.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
        draw_item3d_from_texture(&tex_items, item_icon_tile(id));
    } else if (world_item_is_block_id(id) && tex_terrain.id) {
        float s = 6.0f / 16.0f;
        glTranslatef(0.25f, 3.0f / 16.0f, -(3.0f / 16.0f));
        glScalef(s, s, s);
        glRotatef(60.0f, 0.0f, 0.0f, 1.0f);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(20.0f, 0.0f, 0.0f, 1.0f);
        glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
        draw_dropped_terrain_sprite(world_block_item_tile(id));
    } else if (tex_items.id) {
        /* Ordinary held items remain the old flat icon transform.  Full3D tools,
           swords, stick, bone, bow, and fishing rod take the branches above. */
        float s = 6.0f / 16.0f;
        glTranslatef(0.25f, 3.0f / 16.0f, -(3.0f / 16.0f));
        glScalef(s, s, s);
        glRotatef(60.0f, 0.0f, 0.0f, 1.0f);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(20.0f, 0.0f, 0.0f, 1.0f);
        glBindTexture(GL_TEXTURE_2D, tex_items.id);
        draw_dropped_item_sprite(item_icon_tile(id));
    }

    glPopMatrix();
}

static void draw_remote_player_held_item(const PexNetRenderPlayerState *r, float arm_pitch, float arm_yaw, float arm_roll) {
    if (!r || r->held_item_id <= 0 || r->held_item_count <= 0) return;
    draw_biped_held_item_125(r->held_item_id, r->held_item_count, r->x, r->y, r->z,
                             arm_pitch, arm_yaw, arm_roll);
}

static void draw_local_player_held_item_125(float x, float eye_y, float z,
                                            float arm_pitch, float arm_yaw, float arm_roll) {
    const ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    if (stack_empty(held)) return;
    draw_biped_held_item_125(held->id, held->count, x, eye_y, z,
                             arm_pitch, arm_yaw, arm_roll);
}

static void draw_multiplayer_remote_players(void) {
    if (!g_mp_connected || !tex_steve.id) return;

    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);

    for (int i = 0; i < PEX_NET_MAX_PLAYERS; i++) {
        PexNetRenderPlayerState *r = &g_mp_render_players[i];
        if (!r->active || r->skin_only || r->player_id <= 0 || r->player_id == g_mp_player_id) continue;

        float rx = r->x;
        float ry = r->y;
        float rz = r->z;
        float yaw = r->yaw;
        float pitch = r->pitch;
        float death_time = r->death_time > 0.0f ? (r->death_time * 20.0f + g_frame_partial) : 0.0f;
        int sneaking = (r->flags & PEX_PLAYER_FLAG_SNEAKING) != 0;
        float feet_y = ry - 1.62f;
        float idle = (float)g_ingame_ticks + g_frame_partial + (float)(r->player_id * 7);
        float move = r->move_amount;
        if (move > 1.0f) move = 1.0f;
        float limb = r->limb_swing + g_frame_partial * move;
        float right_arm_pitch = cosf(limb * 0.6662f + (float)M_PI) * 35.0f * move;
        float left_arm_pitch  = cosf(limb * 0.6662f) * 35.0f * move;
        float right_leg_pitch = cosf(limb * 0.6662f) * 55.0f * move;
        float left_leg_pitch  = cosf(limb * 0.6662f + (float)M_PI) * 55.0f * move;
        float right_arm_yaw = 0.0f;
        float left_arm_yaw = 0.0f;
        float right_arm_roll = (cosf(idle * 0.09f) * 0.05f + 0.05f) * 57.29578f;
        float left_arm_roll = -right_arm_roll;
        float body_pitch = 0.0f;
        float head_pivot_y = 0.0f;
        float arm_extra_pitch = 0.0f;
        float leg_pivot_y = 12.0f;
        float leg_pivot_z = 0.0f;
        if (sneaking) {
            body_pitch = 0.5f * 57.29578f;
            arm_extra_pitch = 0.4f * 57.29578f;
            leg_pivot_y = 9.0f;
            leg_pivot_z = 4.0f;
            head_pivot_y = 1.0f;
        }
        float swing = r->prev_swing + (r->swing - r->prev_swing) * g_frame_partial;
        if (swing < 0.0f) swing = 0.0f;
        if (swing > 1.0f) swing = 1.0f;
        if (r->swing_active || swing > 0.0f) {
            float body_yaw = sinf(sqrtf(swing) * (float)M_PI * 2.0f) * 0.2f;
            right_arm_yaw += body_yaw * 3.0f * 57.29578f;
            left_arm_yaw += body_yaw * 57.29578f;
            left_arm_pitch += body_yaw * 57.29578f;
            float v = 1.0f - swing;
            v *= v; v *= v; v = 1.0f - v;
            float swing_sin = sinf(v * (float)M_PI);
            float pitch_rad = pitch / 57.29578f;
            float head_term = sinf(swing * (float)M_PI) * -(pitch_rad - 0.7f) * 0.75f;
            right_arm_pitch -= (swing_sin * 1.2f + head_term) * 57.29578f;
            right_arm_roll = sinf(swing * (float)M_PI) * -0.4f * 57.29578f;
        }
        right_arm_pitch += arm_extra_pitch;
        left_arm_pitch += arm_extra_pitch;

        glPushMatrix();
        Texture *skin_tex = (r->has_skin && r->skin.id) ? &r->skin : &tex_steve;
        glBindTexture(GL_TEXTURE_2D, skin_tex->id);
        steve_set_texture_dims(skin_tex);
        {
            float entity_br = entity_light_factor_at(rx, ry, rz);
            if (r->hurt_time > 0.0f || r->health <= 0) steve_set_tint(entity_br, entity_br * 0.35f, entity_br * 0.35f);
            else steve_set_tint(entity_br, entity_br, entity_br);
        }
        glTranslatef(rx, feet_y, rz);
        glRotatef(180.0f - yaw, 0.0f, 1.0f, 0.0f);
        if (death_time > 0.0f) {
            float death = ((death_time - 1.0f) / 20.0f) * 1.6f;
            if (death < 0.0f) death = 0.0f;
            death = sqrtf(death);
            if (death > 1.0f) death = 1.0f;
            glRotatef(death * 90.0f, 0.0f, 0.0f, 1.0f);
        }
        glScalef(-1.0f, -1.0f, 1.0f);
        glTranslatef(0.0f, -24.0f * 0.0625f - 0.0078125f, 0.0f);

        steve_part(16, 16, 0, 0, 0, -4, 0, -2, 8, 12, 4, 0.0f, 0, body_pitch, 0, 0);
        steve_part(40, 16, -5, 2, 0, -3, -2, -2, 4, 12, 4, 0.0f, 0, right_arm_pitch, right_arm_yaw, right_arm_roll);
        steve_part(40, 16,  5, 2, 0, -1, -2, -2, 4, 12, 4, 0.0f, 1, left_arm_pitch, left_arm_yaw, left_arm_roll);
        steve_part(0, 16, -2, leg_pivot_y, leg_pivot_z, -2, 0, -2, 4, 12, 4, 0.0f, 0, right_leg_pitch, 0, 0);
        steve_part(0, 16,  2, leg_pivot_y, leg_pivot_z, -2, 0, -2, 4, 12, 4, 0.0f, 1, left_leg_pitch, 0, 0);
        steve_part(0, 0, 0, head_pivot_y, 0, -4, -8, -4, 8, 8, 8, 0.0f, 0, pitch, 0, 0);
        steve_part(32, 0, 0, head_pivot_y, 0, -4, -8, -4, 8, 8, 8, 0.5f, 0, pitch, 0, 0);
        draw_remote_player_held_item(r, right_arm_pitch, right_arm_yaw, right_arm_roll);
        glPopMatrix();
    }

    glDisable(GL_ALPHA_TEST);
    steve_set_tint(1.0f, 1.0f, 1.0f);
    glColor4f(1, 1, 1, 1);
    glPopMatrix();
}

static void multiplayer_name_tag_view_angles(float *out_yaw, float *out_pitch) {
    const PexPlayerRenderState *pr = &g_player_render_frame;
    float yaw = lerp_angle(pr->prev_yaw, pr->yaw, g_frame_partial);
    float pitch = pr->prev_pitch + (pr->pitch - pr->prev_pitch) * g_frame_partial;
    if (g_third_person_view == 2) {
        yaw += 180.0f;
        pitch = -pitch;
    }
    if (out_yaw) *out_yaw = yaw;
    if (out_pitch) *out_pitch = pitch;
}

static void draw_multiplayer_name_tags(void) {
    if (!g_mp_connected || !tex_font.id) return;

    float view_yaw = 0.0f;
    float view_pitch = 0.0f;
    multiplayer_name_tag_view_angles(&view_yaw, &view_pitch);

    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (int i = 0; i < PEX_NET_MAX_PLAYERS; i++) {
        PexNetRenderPlayerState *r = &g_mp_render_players[i];
        if (!r->active || r->skin_only || r->player_id <= 0 || r->player_id == g_mp_player_id) continue;
        if (r->health <= 0) continue;

        float dx = r->x - g_player_render_frame.x;
        float dy = r->y - g_player_render_frame.y;
        float dz = r->z - g_player_render_frame.z;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);
        int sneaking = (r->flags & PEX_PLAYER_FLAG_SNEAKING) != 0;
        float max_dist = sneaking ? 32.0f : 64.0f;
        if (dist > max_dist) continue;

        const char *name = r->name[0] ? r->name : "Player";
        int tw = text_width(name);
        if (tw <= 0) continue;

        float feet_y = r->y - 1.62f;
        /* Java RenderLivingLabel anchors labels above the entity height.  The
           previous 2.3F sat almost on the Steve head; 2.55F gives the old
           small air gap above the 1.8-block player. */
        float label_y = feet_y + 2.55f;
        float scale = (1.0f / 60.0f) * 1.6f;
        int half = tw / 2;

        glPushMatrix();
        glTranslatef(r->x, label_y, r->z);
        glNormal3f(0.0f, 1.0f, 0.0f);
        glRotatef(-view_yaw, 0.0f, 1.0f, 0.0f);
        glRotatef(view_pitch, 1.0f, 0.0f, 0.0f);
        glScalef(-scale, -scale, scale);

        if (sneaking) {
            /* Sneaking labels in 1.2.5 are smaller/closer and translucent, and
               they keep depth testing so they do not shout through walls. */
            glTranslatef(0.0f, 0.25f / scale, 0.0f);
            glDepthMask(GL_FALSE);
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_TEXTURE_2D);
            draw_rect(-half - 1, -1, half + 1, 8, (int)0x40000000u);
            glEnable(GL_TEXTURE_2D);
            glDepthMask(GL_TRUE);
            draw_text_no_shadow(name, -tw / 2, 0, (int)0x20FFFFFFu);
        } else {
            /* Normal player labels are world-space billboards.  Draw the black
               strip and faint text without depth test, then the white text with
               depth test, matching the old RenderLivingLabel two-pass look. */
            glDepthMask(GL_FALSE);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_TEXTURE_2D);
            draw_rect(-half - 1, -1, half + 1, 8, (int)0x40000000u);
            glEnable(GL_TEXTURE_2D);
            draw_text_no_shadow(name, -tw / 2, 0, (int)0x20FFFFFFu);
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
            draw_text_no_shadow(name, -tw / 2, 0, 0xFFFFFF);
        }

        glPopMatrix();
    }

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glColor4f(1, 1, 1, 1);
}

static int overlay_tile_for_block(int id) {
    if (id == BLOCK_WATER || id == BLOCK_STILL_WATER) return liquid_top_tile_for_block(id);
    if (id == BLOCK_LAVA || id == BLOCK_STILL_LAVA) return liquid_top_tile_for_block(id);
    if (id == BLOCK_GRASS) return 2;
    if (id == BLOCK_STONE) return 1;
    if (id == BLOCK_DIRT) return 2;
    if (id == BLOCK_COBBLESTONE) return 16;
    if (id == BLOCK_BEDROCK) return 17;
    if (id == BLOCK_PLANKS) return 4;
    if (id == BLOCK_LOG) return 20;
    if (id == BLOCK_SAND) return 18;
    if (id == BLOCK_GRAVEL) return 19;
    if (id == BLOCK_FURNACE) return furnace_side_tile();
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
    int water_overlay = (id == BLOCK_WATER || id == BLOCK_STILL_WATER) && tex_water_overlay.id;
    if (water_overlay) {
        float yaw = g_player_render_frame.yaw / 64.0f;
        float pitch = g_player_render_frame.pitch / 64.0f;
        u0 = 4.0f + yaw; v0 = 4.0f + pitch;
        u1 = 0.0f + yaw; v1 = 0.0f + pitch;
    } else {
        terrain_tile_uv(overlay_tile_for_block(id), &u0, &v0, &u1, &v1);
    }

    glDisable(GL_FOG);
    setup_gui_projection();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, water_overlay ? tex_water_overlay.id : tex_terrain.id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (id == BLOCK_WATER || id == BLOCK_STILL_WATER) {
        glColor4f(1.0f, 1.0f, 1.0f, 0.50f);
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


#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_FAST_WORLD) && PEX_PSP_FAST_WORLD
/* PSP emergency world renderer.
   The PC renderer builds many 16x16x16 display-list sections while the player
   moves.  On PSP/PPSSPP this causes the visible freeze/unfreeze pattern even
   when the average FPS counter is low-but-not-zero.  For the first playable PSP
   build we use a stable surface renderer: no section rebuild queue, no display
   lists, no world mesh worker, no cache churn.  It draws the live RAM world
   directly around the player each frame with a hard vertex budget. */
static int psp_fast_surface_top(int wx, int wz, int *out_y, int *out_id) {
    for (int y = FLAT_WORLD_Y_MAX; y >= FLAT_WORLD_Y_MIN; --y) {
        int id = flat_get_block(wx, y, wz);
        if (id != 0 && !block_is_liquid(id)) {
            if (out_y) *out_y = y;
            if (out_id) *out_id = id;
            return 1;
        }
    }
    return 0;
}

static void psp_fast_emit_surface_block(int id, int x, int y, int z) {
    if (!id) return;
    if (id == BLOCK_SNOW_LAYER) {
        emit_snow_layer_block(x, y, z);
        return;
    }
    if (block_uses_special_model(id) || block_uses_cross_plant_model(id)) {
        /* The full special/cross models are expensive and often alpha-heavy.
           Draw a cheap supporting top face instead so forests/flowers do not
           stall the PSP build. */
        int below = flat_get_block(x, y - 1, z);
        if (below) { id = below; y = y - 1; }
        else return;
    }

    if (flat_face_exposed_for_block(id, x, y, z, 1)) emit_world_block_face_at(id, x, y, z, 1);

    /* Nearby cliff/wall hints.  This keeps the world from looking like a flat
       map without drawing whole vertical columns. */
    for (int face = 2; face <= 5; ++face) {
        if (flat_face_exposed_for_block(id, x, y, z, face)) emit_world_block_face_at(id, x, y, z, face);
    }
}

#define PEX_PSP_FAST_TILE_SIZE 16
#if defined(PEX_PSP_1000_TARGET) && PEX_PSP_1000_TARGET
#define PEX_PSP_FAST_MAX_TILES 72
#else
#define PEX_PSP_FAST_MAX_TILES 128
#endif
typedef struct PspFastSurfaceTile {
    int valid;
    int dirty;
    int tx, tz;
    GLuint tex;
    PspBatch *batch;
    PspBatch *water_batch;
    int vertex_count;
    int water_vertex_count;
} PspFastSurfaceTile;

static PspFastSurfaceTile g_psp_fast_surface_tiles[PEX_PSP_FAST_MAX_TILES];
static GLuint g_psp_fast_surface_tex = 0;

static int psp_floor_div_tile(int v) {
    int s = PEX_PSP_FAST_TILE_SIZE;
    return (v >= 0) ? (v / s) : ((v - s + 1) / s);
}

static void psp_fast_surface_free_cache(void) {
    for (int i = 0; i < PEX_PSP_FAST_MAX_TILES; ++i) {
        if (g_psp_fast_surface_tiles[i].batch) psp_batch_destroy(g_psp_fast_surface_tiles[i].batch);
        if (g_psp_fast_surface_tiles[i].water_batch) psp_batch_destroy(g_psp_fast_surface_tiles[i].water_batch);
        memset(&g_psp_fast_surface_tiles[i], 0, sizeof(g_psp_fast_surface_tiles[i]));
    }
}

static PspFastSurfaceTile *psp_fast_surface_find_tile(int tx, int tz) {
    for (int i = 0; i < PEX_PSP_FAST_MAX_TILES; ++i) {
        if (g_psp_fast_surface_tiles[i].valid && g_psp_fast_surface_tiles[i].tx == tx && g_psp_fast_surface_tiles[i].tz == tz) return &g_psp_fast_surface_tiles[i];
    }
    return NULL;
}

static PspFastSurfaceTile *psp_fast_surface_alloc_tile(int tx, int tz, int center_tx, int center_tz) {
    PspFastSurfaceTile *slot = NULL;
    for (int i = 0; i < PEX_PSP_FAST_MAX_TILES; ++i) {
        if (!g_psp_fast_surface_tiles[i].valid) { slot = &g_psp_fast_surface_tiles[i]; break; }
    }
    if (!slot) {
        int best = 0, best_d = -1;
        for (int i = 0; i < PEX_PSP_FAST_MAX_TILES; ++i) {
            int dx = g_psp_fast_surface_tiles[i].tx - center_tx;
            int dz = g_psp_fast_surface_tiles[i].tz - center_tz;
            int d = dx * dx + dz * dz;
            if (d > best_d) { best_d = d; best = i; }
        }
        slot = &g_psp_fast_surface_tiles[best];
        if (slot->batch) psp_batch_destroy(slot->batch);
        if (slot->water_batch) psp_batch_destroy(slot->water_batch);
    }
    memset(slot, 0, sizeof(*slot));
    slot->valid = 1;
    slot->dirty = 1;
    slot->tx = tx;
    slot->tz = tz;
    slot->tex = tex_terrain.id;
    return slot;
}

static int psp_fast_surface_tile_dirty(int tx, int tz) {
    if (!g_psp_fast_surface_dirty) return 0;
    int x0 = tx * PEX_PSP_FAST_TILE_SIZE;
    int z0 = tz * PEX_PSP_FAST_TILE_SIZE;
    int x1 = x0 + PEX_PSP_FAST_TILE_SIZE - 1;
    int z1 = z0 + PEX_PSP_FAST_TILE_SIZE - 1;
    if (x1 < g_psp_fast_dirty_min_x || x0 > g_psp_fast_dirty_max_x) return 0;
    if (z1 < g_psp_fast_dirty_min_z || z0 > g_psp_fast_dirty_max_z) return 0;
    return 1;
}

static void psp_fast_surface_capture_begin(void) {
    g_psp_begin_active = 1;
    g_psp_begin_mode = GL_QUADS;
    g_psp_imm_count = 0;
}

static PspBatch *psp_fast_capture_batch(void) {
    g_psp_begin_active = 0;
    PspBatch *nb = psp_batch_create(g_psp_imm, g_psp_imm_count, GL_QUADS);
    g_psp_imm_count = 0;
    return nb;
}

static PspBatch *psp_fast_build_tile_batch(int tx, int tz, int pcx, int pcz, int radius) {
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glColor4f(1, 1, 1, 1);

    int x0 = tx * PEX_PSP_FAST_TILE_SIZE;
    int z0 = tz * PEX_PSP_FAST_TILE_SIZE;
    int r2 = radius * radius;
    psp_fast_surface_capture_begin();
    for (int z = z0; z < z0 + PEX_PSP_FAST_TILE_SIZE; ++z) {
        int dz = z - pcz;
        for (int x = x0; x < x0 + PEX_PSP_FAST_TILE_SIZE; ++x) {
            int dx = x - pcx;
            if (dx * dx + dz * dz > r2) continue;
            int y = 0, id = 0;
            if (!psp_fast_surface_top(x, z, &y, &id)) continue;
            psp_fast_emit_surface_block(id, x, y, z);
            if (g_psp_imm_count > PEX_PSP_MAX_IMM_VERTS - 128) break;
        }
        if (g_psp_imm_count > PEX_PSP_MAX_IMM_VERTS - 128) break;
    }
    return psp_fast_capture_batch();
}


static PspBatch *psp_fast_build_water_batch(int tx, int tz, int pcx, int pcz, int radius) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glColor4f(1, 1, 1, 0.72f);

    int x0 = tx * PEX_PSP_FAST_TILE_SIZE;
    int z0 = tz * PEX_PSP_FAST_TILE_SIZE;
    int r2 = radius * radius;
    psp_fast_surface_capture_begin();
    for (int z = z0; z < z0 + PEX_PSP_FAST_TILE_SIZE; ++z) {
        int dz = z - pcz;
        for (int x = x0; x < x0 + PEX_PSP_FAST_TILE_SIZE; ++x) {
            int dx = x - pcx;
            if (dx * dx + dz * dz > r2) continue;
            for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; ++y) {
                int id = flat_get_block(x, y, z);
                if (!block_is_liquid(id)) continue;
                if (!liquid_face_exposed(x, y, z, 0) && !liquid_face_exposed(x, y, z, 1) &&
                    !liquid_face_exposed(x, y, z, 2) && !liquid_face_exposed(x, y, z, 3) &&
                    !liquid_face_exposed(x, y, z, 4) && !liquid_face_exposed(x, y, z, 5)) continue;
                emit_liquid_block_faces(id, x, y, z);
                if (g_psp_imm_count > PEX_PSP_MAX_IMM_VERTS - 192) break;
            }
            if (g_psp_imm_count > PEX_PSP_MAX_IMM_VERTS - 192) break;
        }
        if (g_psp_imm_count > PEX_PSP_MAX_IMM_VERTS - 192) break;
    }
    return psp_fast_capture_batch();
}

static int psp_fast_surface_radius_blocks(void) {
    int radius = generated_render_radius() * 16;
    if (radius < 16) radius = 16;
#if defined(PEX_PSP_1000_TARGET) && PEX_PSP_1000_TARGET
    if (radius > 48) radius = 48;
#else
    if (radius > 64) radius = 64;
#endif
    return radius;
}

static void psp_fast_surface_update_tiles(int pcx, int pcz) {
    int radius = psp_fast_surface_radius_blocks();
    int min_tx = psp_floor_div_tile(pcx - radius);
    int max_tx = psp_floor_div_tile(pcx + radius);
    int min_tz = psp_floor_div_tile(pcz - radius);
    int max_tz = psp_floor_div_tile(pcz + radius);
    int center_tx = psp_floor_div_tile(pcx);
    int center_tz = psp_floor_div_tile(pcz);

    if (g_psp_fast_surface_tex != tex_terrain.id) {
        for (int i = 0; i < PEX_PSP_FAST_MAX_TILES; ++i) g_psp_fast_surface_tiles[i].dirty = 1;
        g_psp_fast_surface_tex = tex_terrain.id;
        psp_fast_mark_all_dirty();
    }

    for (int i = 0; i < PEX_PSP_FAST_MAX_TILES; ++i) {
        PspFastSurfaceTile *t = &g_psp_fast_surface_tiles[i];
        if (!t->valid) continue;
        if (t->tx < min_tx - 1 || t->tx > max_tx + 1 || t->tz < min_tz - 1 || t->tz > max_tz + 1) {
            if (t->batch) psp_batch_destroy(t->batch);
            if (t->water_batch) psp_batch_destroy(t->water_batch);
            memset(t, 0, sizeof(*t));
            continue;
        }
        if (psp_fast_surface_tile_dirty(t->tx, t->tz)) t->dirty = 1;
    }

#if defined(PEX_PSP_1000_TARGET) && PEX_PSP_1000_TARGET
    int rebuild_budget = 1;
#else
    int rebuild_budget = 4;
#endif
    for (int pass = 0; pass < 2 && rebuild_budget > 0; ++pass) {
        for (int tz = min_tz; tz <= max_tz && rebuild_budget > 0; ++tz) {
            for (int tx = min_tx; tx <= max_tx && rebuild_budget > 0; ++tx) {
                int tcx = tx * PEX_PSP_FAST_TILE_SIZE + PEX_PSP_FAST_TILE_SIZE / 2;
                int tcz = tz * PEX_PSP_FAST_TILE_SIZE + PEX_PSP_FAST_TILE_SIZE / 2;
                int dx = tcx - pcx;
                int dz = tcz - pcz;
                if (pass == 0 && (dx * dx + dz * dz > (radius + PEX_PSP_FAST_TILE_SIZE) * (radius + PEX_PSP_FAST_TILE_SIZE))) continue;
                PspFastSurfaceTile *t = psp_fast_surface_find_tile(tx, tz);
                if (!t) t = psp_fast_surface_alloc_tile(tx, tz, center_tx, center_tz);
                if (!t) continue;
                if (!t->dirty && t->batch && t->tex == tex_terrain.id) continue;
                PspBatch *nb = psp_fast_build_tile_batch(tx, tz, pcx, pcz, radius);
                PspBatch *wb = psp_fast_build_water_batch(tx, tz, pcx, pcz, radius);
                if (t->batch) psp_batch_destroy(t->batch);
                if (t->water_batch) psp_batch_destroy(t->water_batch);
                t->batch = nb;
                t->water_batch = wb;
                t->vertex_count = nb ? nb->count : 0;
                t->water_vertex_count = wb ? wb->count : 0;
                t->tex = tex_terrain.id;
                t->dirty = 0;
                rebuild_budget--;
            }
        }
    }

    g_psp_fast_surface_dirty = 0;
    g_psp_fast_dirty_min_x = 0x7fffffff;
    g_psp_fast_dirty_max_x = -0x7fffffff;
    g_psp_fast_dirty_min_z = 0x7fffffff;
    g_psp_fast_dirty_max_z = -0x7fffffff;
}

static void psp_fast_surface_draw_tiles(int pcx, int pcz) {
    int radius = psp_fast_surface_radius_blocks() + PEX_PSP_FAST_TILE_SIZE;
    int r2 = radius * radius;
    for (int i = 0; i < PEX_PSP_FAST_MAX_TILES; ++i) {
        PspFastSurfaceTile *t = &g_psp_fast_surface_tiles[i];
        if (!t->valid || !t->batch) continue;
        int tcx = t->tx * PEX_PSP_FAST_TILE_SIZE + PEX_PSP_FAST_TILE_SIZE / 2;
        int tcz = t->tz * PEX_PSP_FAST_TILE_SIZE + PEX_PSP_FAST_TILE_SIZE / 2;
        int dx = tcx - pcx;
        int dz = tcz - pcz;
        if (dx * dx + dz * dz > r2) continue;
        psp_draw_persistent_batch(t->batch);
    }
}


static void psp_fast_draw_water(int pcx, int pcz) {
    int radius = psp_fast_surface_radius_blocks() + PEX_PSP_FAST_TILE_SIZE;
    int r2 = radius * radius;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glColor4f(1,1,1,1);
    for (int i = 0; i < PEX_PSP_FAST_MAX_TILES; ++i) {
        PspFastSurfaceTile *t = &g_psp_fast_surface_tiles[i];
        if (!t->valid || !t->water_batch) continue;
        int tcx = t->tx * PEX_PSP_FAST_TILE_SIZE + PEX_PSP_FAST_TILE_SIZE / 2;
        int tcz = t->tz * PEX_PSP_FAST_TILE_SIZE + PEX_PSP_FAST_TILE_SIZE / 2;
        int dx = tcx - pcx;
        int dz = tcz - pcz;
        if (dx * dx + dz * dz > r2) continue;
        psp_draw_persistent_batch(t->water_batch);
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glColor4f(1,1,1,1);
}

static void psp_fast_draw_edits(int pcx, int pcz) {
    int radius = psp_fast_surface_radius_blocks() + 4;
    int r2 = radius * radius;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);
    glColor4f(1, 1, 1, 1);
    for (int i = 0; i < 64; ++i) {
        if (!g_psp_fast_edited_blocks[i].stamp) continue;
        int x = g_psp_fast_edited_blocks[i].x;
        int y = g_psp_fast_edited_blocks[i].y;
        int z = g_psp_fast_edited_blocks[i].z;
        int dx = x - pcx;
        int dz = z - pcz;
        if (dx * dx + dz * dz > r2) continue;
        int id = flat_get_block(x, y, z);
        if (!id) continue;
        draw_generic_world_block_model(id, (float)x, (float)y, (float)z);
    }
}

static void psp_fast_draw_world(void) {
    setup_world_projection();
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);

    int pcx = (int)floorf(g_player_render_frame.x);
    int pcz = (int)floorf(g_player_render_frame.z);
    psp_fast_surface_update_tiles(pcx, pcz);

    apply_player_camera(g_frame_partial);
    apply_source_like_fog();
    psp_fast_surface_draw_tiles(pcx, pcz);
    psp_fast_draw_edits(pcx, pcz);
    psp_fast_draw_water(pcx, pcz);
    glEnable(GL_FOG);
    draw_source_clouds();

    glColor4f(1,1,1,1);
    glDisable(GL_FOG);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}
#endif

static void draw_flat_test_world(void) {
    if (!tex_terrain.id) return;
    update_portal_texture_animation();
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_FAST_WORLD) && PEX_PSP_FAST_WORLD
    psp_fast_draw_world();
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    draw_remote_break_overlays();
    if (g_breaking_block && flat_get_block(g_break_x, g_break_y, g_break_z) != 0) {
        float dmg = g_prev_break_damage + (g_break_damage - g_prev_break_damage) * g_frame_partial;
        int stage = (int)(dmg * 10.0f);
        draw_break_overlay_cube((float)g_break_x, (float)g_break_y, (float)g_break_z, stage);
    }
    glDepthMask(GL_TRUE);
    draw_block_selection_border();
    return;
#endif

    setup_world_projection();
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);

    apply_player_camera(g_frame_partial);
    apply_source_like_fog();

    /* Java-style WorldRenderer grid: build/draw 16x16x16 render sections.
       Missing/dirty geometry is rebuilt with a small per-frame budget; terrain
       generation is handled by the streaming worker, never by rendering. */
    if (g_flat_world_geometry_dirty || g_flat_section_geometry_dirty) {
        flat_mark_all_chunks_dirty();
    }

#if defined(PEX_PLATFORM_WII)
    flat_flush_pending_lighting();
#else
    /* Desktop streaming/lighting commits are serviced off the render thread. */
    if (g_mp_connected) flat_flush_pending_lighting();
#endif

    double prof_part = profile_begin();
    FlatFrustum fr;
    flat_frustum_build(&fr);
    int visible_count = build_flat_visible_sections(&fr, g_flat_visible_sections, FLAT_MAX_VISIBLE_SECTIONS);
    profile_add_time(PROF_CULL_SORT, prof_part);
    prof_part = profile_begin();
    if (g_loggy_enabled) {
        g_loggy_visible_sections = visible_count;
        g_loggy_visible_chunks = 0;
        for (int vc_z = 0; vc_z < FLAT_RENDER_CHUNKS; ++vc_z) {
            for (int vc_x = 0; vc_x < FLAT_RENDER_CHUNKS; ++vc_x) {
                if (g_flat_world_chunk_generated[vc_z][vc_x]) g_loggy_visible_chunks++;
            }
        }
    }
    rebuild_visible_flat_sections(g_flat_visible_sections, visible_count);
    profile_add_time(PROF_MESH_MAIN, prof_part);
    prof_part = profile_begin();
    draw_flat_section_passes(g_flat_visible_sections, visible_count);
    profile_add_time(PROF_WORLD_DRAW, prof_part);

    prof_part = profile_begin();
    double entity_part = profile_begin();
    draw_third_person_player();
    profile_add_time(PROF_ENTITY_LOCAL_PLAYER, entity_part);

    int remote_player_count = multiplayer_renderable_remote_player_count();
    if (g_loggy_enabled) g_loggy_entity_remote_players = remote_player_count;

    entity_part = profile_begin();
    draw_multiplayer_remote_players();
    draw_multiplayer_name_tags();
    profile_add_time(PROF_ENTITY_REMOTE_PLAYERS, entity_part);

    /* Name tags are now rendered as world-space billboards, so multiplayer no
       longer needs glGet* matrix readbacks just to project labels into GUI. */
    if (g_loggy_enabled) g_loggy_entity_matrix_skips++;

    entity_part = profile_begin();
    draw_falling_blocks(g_frame_partial);
    profile_add_time(PROF_ENTITY_FALLING_BLOCKS, entity_part);

    entity_part = profile_begin();
    draw_passive_mobs(g_frame_partial);
    draw_vehicles(g_frame_partial);
    draw_projectiles(g_frame_partial);
    draw_xp_orbs(g_frame_partial);
    profile_add_time(PROF_ENTITY_PASSIVE_MOBS, entity_part);

    profile_add_time(PROF_WORLD_ENTITIES, prof_part);

    prof_part = profile_begin();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    draw_dropped_items();
    draw_pickup_fx_items();
    draw_dig_particles(g_frame_partial);
    profile_add_time(PROF_WORLD_PARTICLES, prof_part);

    prof_part = profile_begin();
    draw_translucent_sections(g_flat_visible_sections, visible_count);
    profile_add_time(PROF_WORLD_TRANSLUCENT, prof_part);

    prof_part = profile_begin();
    draw_block_selection_border();
    draw_remote_break_overlays();
    if (g_breaking_block && flat_get_block(g_break_x, g_break_y, g_break_z) != 0) {
        float dmg = g_prev_break_damage + (g_break_damage - g_prev_break_damage) * g_frame_partial;
        int stage = (int)(dmg * 10.0f);
        draw_break_overlay_cube((float)g_break_x, (float)g_break_y, (float)g_break_z, stage);
    }
    profile_add_time(PROF_WORLD_OVERLAYS, prof_part);

    prof_part = profile_begin();
    glEnable(GL_FOG);
    draw_source_clouds();
    profile_add_time(PROF_WORLD_CLOUDS, prof_part);

    glColor4f(1,1,1,1);
    glDisable(GL_FOG);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}


static void draw_item3d_from_texture(Texture *atlas, int tile) {
    /* Port of uploaded source lk.java lines 16-118 for item icons.
       The atlas is parameterized so block items that Java renders from
       terrain.png, such as ladders, do not accidentally use gui/items.png. */
    if (!atlas || !atlas->id) return;
    float tw = atlas->w ? (float)atlas->w : 256.0f;
    float th = atlas->h ? (float)atlas->h : 256.0f;
    float u0 = (float)((tile & 15) * 16 + 0.0f) / tw;
    float u1 = (float)((tile & 15) * 16 + 15.99f) / tw;
    float v0 = (float)((tile >> 4) * 16 + 0.0f) / th;
    float v1 = (float)((tile >> 4) * 16 + 15.99f) / th;
    float w = 1.0f;
    float depth = 0.0625f;

    glBindTexture(GL_TEXTURE_2D, atlas->id);
    glPushMatrix();
    glTranslatef(-0.0f, -0.3f, 0.0f);
    glScalef(1.5f, 1.5f, 1.5f);
    glRotatef(50.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(335.0f, 0.0f, 0.0f, 1.0f);
    glTranslatef(-0.9375f, -0.0625f, 0.0f);

    /* Caller supplies entity/item brightness through the current GL color. */

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

static void draw_item3d_from_atlas(int tile) {
    draw_item3d_from_texture(&tex_items, tile);
}

static void draw_first_person_hand(void) {
    if (!tex_steve.id) return;

    setup_world_projection();
    glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Java 1.2.5: the first-person item/hand pass is rendered inside the same
       hurt-camera + view-bobbing matrix pair used by EntityRenderer before
       ItemRenderer.renderItemInFirstPerson(). */
    apply_hurt_camera_effect(g_frame_partial);
    apply_view_bobbing(g_frame_partial);

    /* Java 1.2.5 ItemRenderer.renderItemInFirstPerson(): add the smoothed arm
       yaw/pitch sway so the held hand/item responds when the player looks
       around, not only while walking. */
    {
        const PexPlayerRenderState *pr = &g_player_render_frame;
        float arm_pitch = pr->prev_render_arm_pitch + (pr->render_arm_pitch - pr->prev_render_arm_pitch) * g_frame_partial;
        float arm_yaw = pr->prev_render_arm_yaw + (pr->render_arm_yaw - pr->prev_render_arm_yaw) * g_frame_partial;
        glRotatef((pr->pitch - arm_pitch) * 0.1f, 1.0f, 0.0f, 0.0f);
        glRotatef((pr->yaw - arm_yaw) * 0.1f, 0.0f, 1.0f, 0.0f);
    }

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
#if defined(PEX_PLATFORM_WII)
    /* The Wii GX shim does not implement a safe mid-frame depth-only clear yet.
       Desktop GL clears depth before the hand/item pass; on Wii that clear was
       a no-op, so the first-person hand could be hidden by stale terrain depth.
       Draw this pass as an overlay until GX depth-clear support is added. */
    glDisable(GL_DEPTH_TEST);
#endif
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
    ItemStack *held = &g_equipped_item;
    float equip = g_prev_equipped_progress + (g_equipped_progress - g_prev_equipped_progress) * g_frame_partial;
    if (equip < 0.0f) equip = 0.0f; if (equip > 1.0f) equip = 1.0f;

    entity_item_light_prepare(g_player_x, g_player_y, g_player_z);

    if (!stack_empty(held) && render_item_as_block_id(held->id) && tex_terrain.id) {
        /* ItemRenderer held-block branch: when a block is selected Java renders the
           3D block in-hand, not the empty player arm.  This also makes the right
           click/use swing look different from mining with an empty hand. */
        glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
        glPushMatrix();
        glTranslatef(0.0f, -0.6f * (1.0f - equip), 0.0f);
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
        draw_block_item_model(held->id, -0.5f, -0.5f, -0.5f);
        glPopMatrix();
    } else if (!stack_empty(held) && world_item_is_block_id(held->id) && tex_terrain.id) {
        int tile = world_block_item_tile(held->id);
        glPushMatrix();
        glTranslatef(0.0f, -0.6f * (1.0f - equip), 0.0f);
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
        draw_item3d_from_texture(&tex_terrain, tile);
        glPopMatrix();
    } else if (!stack_empty(held) && tex_items.id) {
        /* Non-block first-person item branch, using uploaded source lk.java
           lines 16-118 for the actual extruded item geometry. */
        int tile = item_icon_tile(held->id);
        glPushMatrix();
        glTranslatef(0.0f, -0.6f * (1.0f - equip), 0.0f);
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
        draw_item3d_from_atlas(tile);
        glPopMatrix();
    } else {
        glBindTexture(GL_TEXTURE_2D, tex_steve.id);
        glPushMatrix();
        glTranslatef(0.0f, -0.6f * (1.0f - equip), 0.0f);
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
    draw_portal_overlay();
    setup_gui_projection();
}
