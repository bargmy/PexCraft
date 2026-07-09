/* Split from original monolithic main.c. Included by src/main.c unity build. */

static int g_release_panorama_timer = 0;

static void init_title_blocks(void) {
    if (title_inited) return;
    unsigned int title_seed = 12345u;
    for (int x = 0; x < TITLE_COLS; x++) {
        for (int y = 0; y < TITLE_ROWS; y++) {
            title_seed = title_seed * 1103515245u + 12345u;
            double rnd = (double)((title_seed >> 16) & 0x7FFFu) / 32767.0;
            double z = 10.0 + y + rnd * 32.0 + x;
            title_z[x][y] = title_z_prev[x][y] = z;
            title_vel[x][y] = 0.0;
        }
    }
    title_inited = 1;
}

static void tick_title_blocks(void) {
    /* Java GuiMainMenu::updateScreen increments panoramaTimer while the menu
       screen is active, not while the separate boot/Mojang screen is still
       covering it.  Keeping the timer at zero until the first real title frame
       makes first startup and post-world-return enter the same panorama state. */
    if (g_boot_sequence_done && g_release_panorama_timer < 0x3fffffff) ++g_release_panorama_timer;
    init_title_blocks();
    for (int x = 0; x < TITLE_COLS; x++) {
        for (int y = 0; y < TITLE_ROWS; y++) {
            title_z_prev[x][y] = title_z[x][y];
            if (title_z[x][y] > 0.0) title_vel[x][y] -= 0.6;
            title_z[x][y] += title_vel[x][y];
            title_vel[x][y] *= 0.9;
            if (title_z[x][y] < 0.0) { title_z[x][y] = 0.0; title_vel[x][y] = 0.0; }
        }
    }
}

static void cube_vertices(float x0, float y0, float z0, float x1, float y1, float z1, float shade) {
    float u0 = 16.0f / 256.0f, v0 = 0.0f;
    float u1 = 32.0f / 256.0f, v1 = 16.0f / 256.0f;
    float base = shade > 0.0f ? shade : 0.0f;
    glColor4f(base, base, base, 1.0f);
    glBegin(GL_QUADS);
    // Front
    glTexCoord2f(u0,v1); glVertex3f(x0,y1,z1);
    glTexCoord2f(u1,v1); glVertex3f(x1,y1,z1);
    glTexCoord2f(u1,v0); glVertex3f(x1,y0,z1);
    glTexCoord2f(u0,v0); glVertex3f(x0,y0,z1);
    // Back
    glTexCoord2f(u1,v1); glVertex3f(x0,y1,z0);
    glTexCoord2f(u1,v0); glVertex3f(x0,y0,z0);
    glTexCoord2f(u0,v0); glVertex3f(x1,y0,z0);
    glTexCoord2f(u0,v1); glVertex3f(x1,y1,z0);
    // Left
    glColor4f(base*0.75f, base*0.75f, base*0.75f, 1.0f);
    glTexCoord2f(u0,v1); glVertex3f(x0,y1,z0);
    glTexCoord2f(u1,v1); glVertex3f(x0,y1,z1);
    glTexCoord2f(u1,v0); glVertex3f(x0,y0,z1);
    glTexCoord2f(u0,v0); glVertex3f(x0,y0,z0);
    // Right
    glTexCoord2f(u0,v1); glVertex3f(x1,y1,z1);
    glTexCoord2f(u1,v1); glVertex3f(x1,y1,z0);
    glTexCoord2f(u1,v0); glVertex3f(x1,y0,z0);
    glTexCoord2f(u0,v0); glVertex3f(x1,y0,z1);
    // Top
    glColor4f(base*1.15f, base*1.15f, base*1.15f, 1.0f);
    glTexCoord2f(u0,v1); glVertex3f(x0,y0,z1);
    glTexCoord2f(u1,v1); glVertex3f(x1,y0,z1);
    glTexCoord2f(u1,v0); glVertex3f(x1,y0,z0);
    glTexCoord2f(u0,v0); glVertex3f(x0,y0,z0);
    // Bottom
    glColor4f(base*0.55f, base*0.55f, base*0.55f, 1.0f);
    glTexCoord2f(u0,v0); glVertex3f(x0,y1,z0);
    glTexCoord2f(u0,v1); glVertex3f(x1,y1,z0);
    glTexCoord2f(u1,v1); glVertex3f(x1,y1,z1);
    glTexCoord2f(u1,v0); glVertex3f(x0,y1,z1);
    glEnd();
    glColor4f(1,1,1,1);
}

static GLuint g_title_snapshot_tex = 0;
static int g_title_snapshot_ready = 0;
static int g_title_snapshot_w = 0;
static int g_title_snapshot_h = 0;
static int g_title_snapshot_wait_frames = 0;

static void draw_title_logo_3d(float partial);

static int title_animation_finished(void) {
    if (!title_inited) return 0;
    for (int x = 0; x < TITLE_COLS; x++) {
        for (int y = 0; y < TITLE_ROWS; y++) {
            if (title_z[x][y] > 0.01 || fabs(title_vel[x][y]) > 0.01) return 0;
        }
    }
    return 1;
}

static void title_snapshot_reset_on_resize(int title_h) {
    if (g_title_snapshot_w == g_render_w && g_title_snapshot_h == title_h) return;
    if (g_title_snapshot_tex) glDeleteTextures(1, &g_title_snapshot_tex);
    g_title_snapshot_tex = 0;
    g_title_snapshot_ready = 0;
    g_title_snapshot_w = 0;
    g_title_snapshot_h = 0;
    g_title_snapshot_wait_frames = 0;
}

static void capture_title_logo_snapshot(int title_h) {
#if defined(PEX_PLATFORM_PSP)
    /* PSP GU shim does not support glCopyTexSubImage2D. Keep rendering the 3D logo instead of caching a blank snapshot. */
    (void)title_h;
    return;
#endif
    if (g_title_snapshot_ready || title_h <= 0 || g_render_w <= 0) return;
    if (!g_title_snapshot_tex) {
        glGenTextures(1, &g_title_snapshot_tex);
        glBindTexture(GL_TEXTURE_2D, g_title_snapshot_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, PEX_GL_CLAMP_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, PEX_GL_CLAMP_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_render_w, title_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        g_title_snapshot_w = g_render_w;
        g_title_snapshot_h = title_h;
    } else {
        glBindTexture(GL_TEXTURE_2D, g_title_snapshot_tex);
    }
    /* Copy the actually rendered 3D title strip from the current backbuffer. */
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, g_render_h - title_h, g_render_w, title_h);
    g_title_snapshot_ready = 1;
}

static void draw_title_logo_snapshot(void) {
    if (!g_title_snapshot_ready || !g_title_snapshot_tex || g_title_snapshot_w <= 0 || g_title_snapshot_h <= 0) return;
    glBindTexture(GL_TEXTURE_2D, g_title_snapshot_tex);
    glColor4f(1,1,1,1);
    int gh = g_title_snapshot_h / (g_gui_scale > 0 ? g_gui_scale : 1);
    if (gh < 1) gh = 1;
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(0.0f,      (float)gh, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f((float)g_gui_w, (float)gh, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f((float)g_gui_w, 0.0f,      0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0f,      0.0f,      0.0f);
    glEnd();
}

static void draw_title_logo_3d(float partial) {
    init_title_blocks();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    int title_h = 120 * g_gui_scale;
    if (title_h < 1) title_h = 1;
    if (title_h > g_render_h) title_h = g_render_h;
    gluPerspective(70.0, (double)g_render_w / (double)title_h, 0.05, 100.0);
    glViewport(0, g_render_h - title_h, g_render_w, title_h);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    for (int pass = 0; pass < 3; pass++) {
        glPushMatrix();
        glTranslatef(0.4f, 0.6f, -13.0f);
        if (pass == 0) {
            glClear(GL_DEPTH_BUFFER_BIT);
            glTranslatef(0.0f, -0.4f, 0.0f);
            glScalef(0.98f, 1.0f, 1.0f);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else if (pass == 1) {
            glDisable(GL_BLEND);
            glClear(GL_DEPTH_BUFFER_BIT);
        } else {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        }
        glScalef(1.0f, -1.0f, 1.0f);
        glRotatef(15.0f, 1.0f, 0.0f, 0.0f);
        glScalef(0.89f, 1.0f, 0.4f);
        glTranslatef(-TITLE_COLS * 0.5f, -TITLE_ROWS * 0.5f, 0.0f);
        glBindTexture(GL_TEXTURE_2D, pass == 0 ? tex_black.id : tex_terrain.id);
        for (int y = 0; y < TITLE_ROWS; y++) {
            for (int x = 0; x < TITLE_COLS; x++) {
                if (TITLE_BLOCKS[y][x] == ' ') continue;
                glPushMatrix();
                float z = (float)(title_z_prev[x][y] + (title_z[x][y] - title_z_prev[x][y]) * partial);
                float scale = 1.0f;
                if (pass == 0) {
                    scale = z * 0.04f + 1.0f;
                    z = 0.0f;
                }
                glTranslatef((float)x, (float)y, z);
                glScalef(scale, scale, scale);
                cube_vertices(0, 0, 0, 1, 1, 1, pass == 0 ? 0.0f : 1.0f);
                glPopMatrix();
            }
        }
        glPopMatrix();
    }
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    setup_gui_projection();
}


static GLuint g_release_panorama_viewport_tex = 0;
static int g_release_panorama_alloc_size = 0;
static int g_release_panorama_boot_reset_done = 0;
#define RELEASE_PANORAMA_TEX_SIZE 256

static void release_title_state_enter(void) {
    /* Java GuiMainMenu owns panoramaTimer and viewportTexture per screen
       instance.  Reset both when entering the title screen so startup and
       post-world-return take the same path on real OpenGL. */
    g_release_panorama_timer = 0;
    g_release_panorama_boot_reset_done = g_boot_sequence_done ? 1 : 0;
    if (g_release_panorama_viewport_tex) {
        glDeleteTextures(1, &g_release_panorama_viewport_tex);
        g_release_panorama_viewport_tex = 0;
    }
    g_release_panorama_alloc_size = 0;
}
static int release_panorama_target_size(void) {
    /* Java GuiMainMenu.renderSkybox always renders and copies a 256x256
       viewport texture.  Do not scale this with the window size: copying a
       larger region than the real framebuffer breaks native OpenGL on
       resizable windows and causes quadrant/split artifacts. */
    return RELEASE_PANORAMA_TEX_SIZE;
}


static int release_panorama_viewport_y(int target_size) {
    (void)target_size;
    return 0;
}

static void panorama_reset_gl_state(void) {
#if defined(GL_SCISSOR_TEST)
    glDisable(GL_SCISSOR_TEST);
#endif
#if defined(GL_LIGHTING)
    glDisable(GL_LIGHTING);
#endif
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    /* Java/Minecraft leaves the depth compare as LEQUAL before GuiMainMenu
       renders its 256x256 feedback blur.  Fresh Windows OpenGL starts at LESS,
       which lets only the first feedback quad write depth and rejects the later
       blur passes.  Returning from a world happened to work because world
       rendering had already set GL_LEQUAL. */
    glDepthFunc(GL_LEQUAL);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if defined(GL_TEXTURE)
    if (!pex_using_d3d9() && !pex_using_d3d11()) {
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
    }
#endif
    glMatrixMode(GL_MODELVIEW);
}

static void ensure_panorama_viewport_texture(void) {
    int size = release_panorama_target_size();
    if (g_release_panorama_viewport_tex && g_release_panorama_alloc_size == size) return;
    if (g_release_panorama_viewport_tex) {
        glDeleteTextures(1, &g_release_panorama_viewport_tex);
        g_release_panorama_viewport_tex = 0;
    }
    g_release_panorama_alloc_size = size;
    glGenTextures(1, &g_release_panorama_viewport_tex);
    glBindTexture(GL_TEXTURE_2D, g_release_panorama_viewport_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    /* Real OpenGL leaves glTexImage2D(NULL) contents undefined until the first
       copy.  Zero-initialize the CPU side only for real GL; D3D11 must keep
       this texture DEFAULT-usage so glCopyTexSubImage2D can update it. */
    unsigned char *blank = NULL;
    if (!pex_using_d3d9() && !pex_using_d3d11()) {
        blank = (unsigned char*)calloc((size_t)size * (size_t)size, 4u);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, blank);
    free(blank);
}

static void draw_release_panorama_cube(float partial) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(120.0, 1.0, 0.05, 10.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor4f(1,1,1,1);
    glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
    glEnable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    const int samples = 8;
    float timer = (float)g_release_panorama_timer + partial;
    for (int pass = 0; pass < samples * samples; ++pass) {
        glPushMatrix();
        float ox = (((float)(pass % samples) / (float)samples) - 0.5f) / 64.0f;
        float oy = (((float)(pass / samples) / (float)samples) - 0.5f) / 64.0f;
        glTranslatef(ox, oy, 0.0f);
        glRotatef(sinf(timer / 400.0f) * 25.0f + 20.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(-timer * 0.1f, 0.0f, 1.0f, 0.0f);
        for (int face = 0; face < 6; ++face) {
            if (!tex_panorama[face].id) continue;
            glPushMatrix();
            if (face == 1) glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
            if (face == 2) glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
            if (face == 3) glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
            if (face == 4) glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            if (face == 5) glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            glBindTexture(GL_TEXTURE_2D, tex_panorama[face].id);
            /* Java's title panorama is heavily blurred; force linear sampling on
               real OpenGL so first startup does not show nearest/blocky faces. */
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f / (float)(pass + 1));
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 1.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, 1.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, 1.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, 1.0f);
            glEnd();
            glPopMatrix();
        }
        glPopMatrix();
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
    }
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);
    glColor4f(1,1,1,1);
}


#if defined(PEX_PLATFORM_ANDROID) || defined(PEX_PLATFORM_ANDROID_TV)
static void draw_release_panorama_cube_android_direct(float partial) {
    /* Android GLES2 had trouble with the Java-style 256x256 feedback texture:
       the copy/blur path could remain visible as a small corner square and
       flicker.  Draw the panorama cube directly to the real framebuffer on
       Android instead.  Blur is done by direct multi-sampling, not by copying
       a 256x256 viewport texture, so it fills the screen without corner flicker. */
    double aspect = (g_render_h > 0) ? (double)g_render_w / (double)g_render_h : 16.0 / 9.0;
    float timer = (float)g_release_panorama_timer + partial;

    panorama_reset_gl_state();
    glViewport(0, 0, g_render_w, g_render_h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(120.0, aspect, 0.05, 10.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor4f(1,1,1,1);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Keep Android on the full-screen direct path, but restore the title-menu
       blur look.  The old feedback-copy blur was the source of the 256x256
       corner flicker; this uses the same sub-pixel multi-sample idea directly
       in the real framebuffer instead of copying a tiny viewport texture. */
    const int samples = 4;
    for (int pass = 0; pass < samples * samples; ++pass) {
        glPushMatrix();
        float ox = (((float)(pass % samples) / (float)samples) - 0.5f) / 64.0f;
        float oy = (((float)(pass / samples) / (float)samples) - 0.5f) / 64.0f;
        glTranslatef(ox, oy, 0.0f);
        glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(sinf(timer / 400.0f) * 25.0f + 20.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(-timer * 0.1f, 0.0f, 1.0f, 0.0f);

        for (int face = 0; face < 6; ++face) {
            if (!tex_panorama[face].id) continue;
            glPushMatrix();
            if (face == 1) glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
            if (face == 2) glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
            if (face == 3) glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
            if (face == 4) glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            if (face == 5) glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            glBindTexture(GL_TEXTURE_2D, tex_panorama[face].id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f / (float)(pass + 1));
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 1.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, 1.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, 1.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, 1.0f);
            glEnd();
            glPopMatrix();
        }
        glPopMatrix();
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
    }
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glDepthMask(GL_TRUE);
    glColor4f(1,1,1,1);
    setup_gui_projection();
}
#endif

static void release_panorama_blur_pass(void) {
    ensure_panorama_viewport_texture();
    glBindTexture(GL_TEXTURE_2D, g_release_panorama_viewport_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int target_size = release_panorama_target_size();
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0,
                        release_panorama_viewport_y(target_size), target_size, target_size);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

    /* Java GuiMainMenu.rotateAndBlurSkybox(): draw the blur pass while the
       viewport is still the 256x256 panorama viewport.  Using the real GUI
       dimensions here is intentional; Java does the same. */
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, (double)g_gui_w, (double)g_gui_h, 0.0, 1000.0, 3000.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -2000.0f);

    for (int i = 0; i < 3; ++i) {
        float off = (float)(i - 1) / 256.0f;
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f / (float)(i + 1));
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f + off, 0.0f); glVertex3f((float)g_gui_w, (float)g_gui_h, 0.0f);
        glTexCoord2f(1.0f + off, 0.0f); glVertex3f((float)g_gui_w, 0.0f, 0.0f);
        glTexCoord2f(1.0f + off, 1.0f); glVertex3f(0.0f, 0.0f, 0.0f);
        glTexCoord2f(0.0f + off, 1.0f); glVertex3f(0.0f, (float)g_gui_h, 0.0f);
        glEnd();
    }

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glColor4f(1,1,1,1);
}

static void draw_release_skybox(float partial) {
    static double s_last_panorama_update = -1000.0;
    static int s_have_panorama_frame = 0;
    static int s_cached_panorama_size = 0;

#if defined(PEX_PLATFORM_ANDROID) || defined(PEX_PLATFORM_ANDROID_TV)
    (void)s_last_panorama_update;
    (void)s_have_panorama_frame;
    (void)s_cached_panorama_size;
    draw_release_panorama_cube_android_direct(partial);
    return;
#endif

    panorama_reset_gl_state();
    ensure_panorama_viewport_texture();

    /* Java GuiMainMenu.renderSkybox is extremely expensive in this fixed-function
       compatibility renderer because it performs 64 panorama samples and 8
       framebuffer-copy blur passes.  Preserve the look, but build the blurred
       256x256 feedback texture at a sane animation rate and reuse it for the
       uncapped menu draw frames. */
    int target_size = release_panorama_target_size();
    double now = now_seconds();
    int rebuild_panorama = !s_have_panorama_frame || s_cached_panorama_size != target_size ||
                           (now - s_last_panorama_update) >= (1.0 / 30.0);
    if (rebuild_panorama) {
        glViewport(0, release_panorama_viewport_y(target_size), target_size, target_size);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        draw_release_panorama_cube(partial);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_TEXTURE_2D);
        for (int i = 0; i < 8; ++i) release_panorama_blur_pass();
        s_last_panorama_update = now;
        s_have_panorama_frame = 1;
        s_cached_panorama_size = target_size;
    }

    /* Then restore the actual window viewport and draw the cached 256 texture
       through the normal GUI projection, just like the Java ScaledResolution path. */
    glViewport(0, 0, g_render_w, g_render_h);
    setup_gui_projection();
    glBindTexture(GL_TEXTURE_2D, g_release_panorama_viewport_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    float scale = g_gui_w > g_gui_h ? 120.0f / (float)g_gui_w : 120.0f / (float)g_gui_h;
    float u = (float)g_gui_h * scale / 256.0f;
    float v = (float)g_gui_w * scale / 256.0f;
    float tv0 = 0.5f - v;
    float tv1 = 0.5f + v;
    glColor4f(1,1,1,1);
    glBegin(GL_QUADS);
    glTexCoord2f(0.5f - u, tv1); glVertex3f(0.0f,          (float)g_gui_h, 0.0f);
    glTexCoord2f(0.5f - u, tv0); glVertex3f((float)g_gui_w, (float)g_gui_h, 0.0f);
    glTexCoord2f(0.5f + u, tv0); glVertex3f((float)g_gui_w, 0.0f,          0.0f);
    glTexCoord2f(0.5f + u, tv1); glVertex3f(0.0f,          0.0f,          0.0f);
    glEnd();
}

static void draw_release_minecraft_logo(void) {
    if (!tex_title_logo.id) return;
    const int logo_anchor_w = 274;
    int x = g_gui_w / 2 - logo_anchor_w / 2;
    int y = 30;
    draw_textured_modal_rect(&tex_title_logo, x + 0,   y + 0, 0,   0, 99, 44, 0xFFFFFF);
    draw_textured_modal_rect(&tex_title_logo, x + 99,  y + 0, 129, 0, 27, 44, 0xFFFFFF);
    draw_textured_modal_rect(&tex_title_logo, x + 125, y + 0, 126, 0, 3,  44, 0xFFFFFF);
    draw_textured_modal_rect(&tex_title_logo, x + 128, y + 0, 99,  0, 26, 44, 0xFFFFFF);
    draw_textured_modal_rect(&tex_title_logo, x + 155, y + 0, 0,  45, 155, 44, 0xFFFFFF);
}

static void draw_boot_mojang_logo(void) {
    draw_rect(0, 0, g_gui_w, g_gui_h, 0xFFFFFF);
    if (!tex_mojang.id) return;
    int w = tex_mojang.w;
    int h = tex_mojang.h;
    if (w > g_gui_w - 16) { h = h * (g_gui_w - 16) / w; w = g_gui_w - 16; }
    if (h > g_gui_h - 16) { w = w * (g_gui_h - 16) / h; h = g_gui_h - 16; }
    draw_texture_scaled_full(&tex_mojang, (g_gui_w - w) / 2, (g_gui_h - h) / 2, w, h, 0xFFFFFF);
}

static void draw_title_screen(float partial) {
    if (!g_boot_sequence_done && g_title_enter_time > 0.0) {
        double elapsed = now_seconds() - g_title_enter_time;
        if (elapsed >= 2.0 && !g_menu_music_started) {
            pex_menu_music_start_once();
            g_menu_music_started = 1;
        }
        if (elapsed < 3.0) {
            draw_boot_mojang_logo();
            return;
        }
        g_boot_sequence_done = 1;
        if (!g_release_panorama_boot_reset_done) {
            /* First startup reaches the real menu through the separate Mojang
               boot screen, while returning from a world enters the title
               directly.  Reset the 256x256 feedback texture at the boot->menu
               boundary so the first visible OpenGL title frame follows the
               same lifecycle as post-world-return GuiMainMenu.initGui(). */
            release_title_state_enter();
            g_release_panorama_boot_reset_done = 1;
        }
    }

    draw_release_skybox(partial);
    draw_gradient(0, 0, g_gui_w, g_gui_h, (int)0x80FFFFFFu, 0x00FFFFFF);
    draw_gradient(0, 0, g_gui_w, g_gui_h, 0x00000000, (int)0x80000000u);
    draw_release_minecraft_logo();
    glPushMatrix();
    glTranslatef((float)(g_gui_w / 2 + 90), 70.0f, 0.0f);
    glRotatef(-20.0f, 0.0f, 0.0f, 1.0f);
    float s = 1.8f - fabsf(sinf((float)((GetTickCount() % 1000UL) / 1000.0 * M_PI * 2.0)) * 0.1f);
    s = s * 100.0f / (float)(text_width(g_splash) + 32);
    glScalef(s, s, s);
    draw_centered_text(g_splash, 0, -8, 16776960);
    glPopMatrix();
    draw_text(VERSION_TEXT, 2, g_gui_h - 10, 16777215);
    const char *copy = "Copyright to no one. This is for everyone!";
    draw_text(copy, g_gui_w - text_width(copy) - 2, g_gui_h - 10, 16777215);
    draw_all_buttons();
}



static void draw_language_screen(void) {
    draw_default_bg();

    if (!pex_language_runtime_files_available()) {
        draw_centered_text(tr_key_default("options.language", "Language"), g_gui_w / 2, 16, 16777215);
        draw_centered_text("Minecraft 1.2.5 language files are not installed.", g_gui_w / 2, g_gui_h / 2 - 36, 16777215);
        draw_centered_text("They will be extracted client-side from client.jar.", g_gui_w / 2, g_gui_h / 2 - 24, 10526880);
    } else {
        int count = pex_language_count();
        int top = pex_language_top();
        int bottom = pex_language_bottom();
        int view_h = pex_language_view_height();
        int first = pex_language_first_visible_row();
        int visible = pex_language_visible_rows();
        int cur = pex_current_language_index();
        int content_h = pex_language_content_height();
        int max_scroll = pex_language_max_scroll_pixels();

        /* Java GuiSlot.drawScreen paints the scrolling language section as a
           darker tiled dirt panel between top/bottom, not as the plain full
           screen background.  The UV scroll is tied to amountScrolled. */
        draw_tiled_rect_tint(0, top, g_gui_w, bottom, 0x202020, g_language_scroll);

        for (int i = 0; i < visible; ++i) {
            int idx = first + i;
            int y = pex_language_row_y(idx);
            const char *name;
            if (idx >= count) break;
            if (y + 14 < top || y > bottom) continue;
            name = pex_language_name_at(idx);
            if (idx == cur) {
                /* GuiSlot selected row: outer grey quad and inner black quad. */
                draw_rect(g_gui_w / 2 - 110, y - 2, g_gui_w / 2 + 110, y + 16, 0xFF808080);
                draw_rect(g_gui_w / 2 - 109, y - 1, g_gui_w / 2 + 109, y + 15, 0xFF000000);
            }
            /* Java GuiSlotLanguage temporarily forces bidi while drawing each language name. */
            draw_centered_text_force_bidi_lang(name, pex_language_code_at(idx), g_gui_w / 2, y + 1, 16777215);
        }

        /* Java GuiSlot overlays the regions outside the list and fades the
           first/last 4 pixels of the scroll viewport before drawing the bar. */
        draw_tiled_overlay_background(0, top, 255, 255);
        draw_tiled_overlay_background(bottom, g_gui_h, 255, 255);
        draw_gradient(0, top, g_gui_w, top + 4, 0xFF000000, 0x00000000);
        draw_gradient(0, bottom - 4, g_gui_w, bottom, 0x00000000, 0xFF000000);

        if (max_scroll > 0 && content_h > 0) {
            int track_x = g_gui_w / 2 + 124;
            int track_w = 6;
            int thumb_h = view_h * view_h / content_h;
            int thumb_y;
            if (thumb_h < 32) thumb_h = 32;
            if (thumb_h > view_h - 8) thumb_h = view_h - 8;
            thumb_y = top + (g_language_scroll * (view_h - thumb_h)) / max_scroll;
            if (thumb_y < top) thumb_y = top;
            draw_rect(track_x, top, track_x + track_w, bottom, 0xFF000000);
            draw_rect(track_x, thumb_y, track_x + track_w, thumb_y + thumb_h, 0xFF808080);
            draw_rect(track_x, thumb_y, track_x + track_w - 1, thumb_y + thumb_h - 1, 0xFFC0C0C0);
        }

        draw_centered_text(tr_key_default("options.language", "Language"), g_gui_w / 2, 16, 16777215);
        {
            char warn[256];
            snprintf(warn, sizeof(warn), "(%s)", tr_key_default("options.languageWarning", "Language translations may not be 100% accurate"));
            draw_centered_text(warn, g_gui_w / 2, g_gui_h - 56, 8421504);
        }
    }

    draw_all_buttons();
}


static void draw_assets_screen(void) {
    draw_default_bg();
    draw_centered_text("Assets", g_gui_w / 2, 20, 16777215);
    draw_centered_text("legacy.json resources", g_gui_w / 2, 34, 8421504);

    int idx_state = legacy_assets_index_state();
    int downloading = legacy_assets_is_downloading();

    if (idx_state == CLASSIC_SIZE_FETCHING) {
        draw_centered_text("Fetching legacy.json asset index...", g_gui_w / 2, 80, 14737632);
    } else if (idx_state == CLASSIC_SIZE_ERROR) {
        draw_centered_text("Could not fetch legacy.json", g_gui_w / 2, 64, 0xFF5555);
        { char err[MAX_LABEL]; legacy_assets_index_error_line(err, sizeof(err)); draw_centered_text(err, g_gui_w / 2, 76, 8421504); }
    } else if (idx_state == CLASSIC_SIZE_READY && !downloading && !legacy_assets_any_missing()) {
        draw_centered_text("All legacy asset groups are installed.", g_gui_w / 2, 70, 0x55FF55);
    }

    /* Keep all download progress/status on the asset buttons themselves. */
    for (int i = 0; i < g_button_count; ++i) {
        Button *b = &g_buttons[i];
        if (b->id >= 6100 && b->id < 6107) {
            int cats[] = { LEGACY_ASSET_LANG, CLASSIC_AUDIO_MOBS, CLASSIC_AUDIO_WORLD_UI, CLASSIC_AUDIO_RECORDS, CLASSIC_AUDIO_MENU_MUSIC, CLASSIC_AUDIO_GAME_MUSIC, LEGACY_ASSET_OTHER };
            legacy_asset_button_label(cats[b->id - 6100], b->label, sizeof(b->label));
        }
    }

    draw_all_buttons();
}

static void draw_options_screen(void) {
    draw_default_bg();
    draw_centered_text(tr("Options"), g_gui_w / 2, 20, 16777215);
    draw_all_buttons();
}

static void draw_options_more_screen(void) {
    draw_default_bg();
    draw_centered_text(tr("Video Settings"), g_gui_w / 2, 20, 16777215);
    draw_all_buttons();
}

static void draw_hptibine_screen(void) {
    draw_default_bg();
    draw_centered_text(hptibine_screen_title(g_screen), g_gui_w / 2, 20, 16777215);
    draw_all_buttons();
}

static void draw_skin_preview_model(int cx, int cy) {
    if (!tex_steve.id) return;
    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_steve.id);
    steve_set_texture_dims(&tex_steve);
    steve_set_tint(1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);

    glTranslatef((float)cx, (float)cy, 80.0f);
    glScalef(-44.0f, 44.0f, 44.0f);
    glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
    glRotatef(25.0f, 1.0f, 0.0f, 0.0f);
    glRotatef((float)((g_ticks * 2) % 360), 0.0f, 1.0f, 0.0f);
    glScalef(-1.0f, -1.0f, 1.0f);
    glTranslatef(0.0f, -24.0f * 0.0625f - 0.0078125f, 0.0f);

    glColor4f(1,1,1,1);
    steve_part(16, 16, 0, 0, 0, -4, 0, -2, 8, 12, 4, 0.0f, 0, 0, 0, 0);
    steve_part(40, 16, -5, 2, 0, -3, -2, -2, 4, 12, 4, 0.0f, 0, 0, 0, 3.0f);
    steve_part(40, 16,  5, 2, 0, -1, -2, -2, 4, 12, 4, 0.0f, 1, 0, 0, -3.0f);
    steve_part(0, 16, -2, 12, 0, -2, 0, -2, 4, 12, 4, 0.0f, 0, 0, 0, 0);
    steve_part(0, 16,  2, 12, 0, -2, 0, -2, 4, 12, 4, 0.0f, 1, 0, 0, 0);
    steve_part(0, 0, 0, 0, 0, -4, -8, -4, 8, 8, 8, 0.0f, 0, 0, 0, 0);
    steve_part(32, 0, 0, 0, 0, -4, -8, -4, 8, 8, 8, 0.5f, 0, 0, 0, 0);

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_DEPTH_TEST);
    glColor4f(1,1,1,1);
    glPopMatrix();
}

static void draw_skins_screen(void) {
    draw_default_bg();
    draw_centered_text(tr("Skins"), g_gui_w / 2, 20, 16777215);
    draw_rect(g_gui_w / 2 - 42, 42, g_gui_w / 2 + 42, 154, (int)0x80000000u);
    draw_skin_preview_model(g_gui_w / 2, 142);
    char line[MAX_LABEL];
    snprintf(line, sizeof(line), "%s: %s", tr("Current skin"), g_opts.skin_path[0] ? tr("Custom") : tr("Default"));
    draw_centered_text(line, g_gui_w / 2, 160, 14737632);
    draw_all_buttons();
}

static void draw_controls_screen(void) {
    draw_default_bg();
    draw_centered_text(tr("Controls"), g_gui_w / 2, 20, 16777215);
    int x0 = g_gui_w / 2 - 155;
    for (int i = 0; i < 10; i++) {
        draw_text(key_action_names[i], x0 + (i % 2) * 160 + 70 + 6, g_gui_h / 6 + 24 * (i >> 1) + 7, -1);
    }
    draw_all_buttons();
}

static void format_world_last_played(long long ms, char *out, size_t cap) {
    if (!out || cap == 0) return;
    if (ms <= 0) {
        snprintf(out, cap, "Unknown time");
        return;
    }
    time_t seconds = (time_t)(ms / 1000LL);
    struct tm *tmv = localtime(&seconds);
    if (!tmv) {
        snprintf(out, cap, "Unknown time");
        return;
    }
    strftime(out, cap, "%x %X", tmv);
}

static void draw_world_slot_rows(void) {
    int top = world_save_list_top();
    int bottom = world_save_list_bottom();
    int left = g_gui_w / 2 - 110;
    int right = g_gui_w / 2 + 110;

    draw_tiled_rect_tint(0, top, g_gui_w, bottom, 0x202020, 0);

    int visible_rows = world_save_visible_rows();
    if (visible_rows > g_world_save_count - g_world_save_scroll) visible_rows = g_world_save_count - g_world_save_scroll;
    if (visible_rows < 0) visible_rows = 0;

    for (int i = 0; i < visible_rows; i++) {
        int save_index = g_world_save_scroll + i;
        WorldSaveEntry *e = &g_world_saves[save_index];
        int row_y = top + 4 + i * 36;
        int row_h = 32;
        if (row_y > bottom || row_y + row_h < top) continue;
        if (save_index == g_selected_world_index) {
            draw_rect(left, row_y - 2, right, row_y + row_h + 2, 8421504);
            draw_rect(left + 1, row_y - 1, right - 1, row_y + row_h + 1, 0);
        }

        char date[64];
        format_world_last_played(e->last_played, date, sizeof(date));

        char file_line[128];
        snprintf(file_line, sizeof(file_line), "%s (%s)", e->dir_name, date);
        const char *mode_line = "Survival Mode";
        if (e->requires_conversion) mode_line = "Must be converted!";
        else if (e->hardcore) mode_line = "Hardcore Mode!";
        else if (e->game_mode == 1) mode_line = "Creative Mode";
        draw_text(e->display_name[0] ? e->display_name : e->dir_name, left + 2, row_y + 1, 16777215);
        draw_text(file_line, left + 2, row_y + 12, 8421504);
        draw_text(mode_line, left + 2, row_y + 22, e->hardcore ? 16733525 : 8421504);
    }

    if (g_world_save_count == 0) draw_centered_text(tr_key_default("selectWorld.empty", "No worlds found"), g_gui_w / 2, top + 24, 8421504);

    int max_scroll = g_world_save_count - world_save_visible_rows();
    if (max_scroll > 0) {
        int content_h = g_world_save_count * 36;
        int bar_x0 = g_gui_w / 2 + 124;
        int bar_x1 = bar_x0 + 6;
        int thumb_h = (bottom - top) * (bottom - top) / content_h;
        if (thumb_h < 32) thumb_h = 32;
        if (thumb_h > bottom - top - 8) thumb_h = bottom - top - 8;
        int thumb_y = top + g_world_save_scroll * (bottom - top - thumb_h) / max_scroll;
        if (thumb_y < top) thumb_y = top;
        draw_rect(bar_x0, top, bar_x1, bottom, 0);
        draw_rect(bar_x0, thumb_y, bar_x1, thumb_y + thumb_h, 8421504);
        draw_rect(bar_x0, thumb_y, bar_x1 - 1, thumb_y + thumb_h - 1, 12632256);
    }

    draw_tiled_rect_tint(0, 0, g_gui_w, top, 0x404040, 0);
    draw_tiled_rect_tint(0, bottom, g_gui_w, g_gui_h, 0x404040, 0);
    draw_gradient(0, top, g_gui_w, top + 4, 0xFF000000, 0x00000000);
    draw_gradient(0, bottom - 4, g_gui_w, bottom, 0x00000000, 0xFF000000);
}

static void draw_world_screen(void) {
    draw_world_slot_rows();
    draw_centered_text(g_screen == SCREEN_WORLD_DELETE ? tr_key_default("selectWorld.deleteQuestion", "Delete World") : tr_key_default("selectWorld.title", "Select World"), g_gui_w / 2, 20, 16777215);
    if (g_input_focus_mode == PEX_INPUT_FOCUS_GAMEPAD) {
        if (g_screen == SCREEN_WORLD_SELECT) draw_centered_text("A: Play   X: Create   Y: Rename   B: Back", g_gui_w / 2, g_gui_h - 72, 10526880);
        else draw_centered_text("A: Delete   B: Back", g_gui_w / 2, g_gui_h - 72, 10526880);
    }
    draw_all_buttons();
}

static void draw_confirm_delete(void) {
    draw_default_bg();
    char world_line[64];
    snprintf(world_line, sizeof(world_line), "'%s' will be lost forever!", g_pending_world_name[0] ? g_pending_world_name : "World");
    draw_centered_text(tr_key_default("selectWorld.deleteWarning", "Are you sure you want to delete this world?"), g_gui_w / 2, 70, 16777215);
    draw_centered_text(world_line, g_gui_w / 2, 90, 16777215);
    draw_all_buttons();
}

static void draw_multiplayer(void) {
    pex_mp_server_list_ensure();
    draw_default_bg();

    if (pex_mp_server_mode_get() != 0) {
        int mode = pex_mp_server_mode_get();
        const char *title = mode == 1 ? tr_key_default("selectServer.direct", "Direct Connect") :
                            mode == 2 ? tr_key_default("selectServer.add", "Add Server") :
                                        tr_key_default("selectServer.edit", "Edit Server");
        draw_centered_text(title, g_gui_w / 2, g_gui_h / 4 - 60 + 20, 16777215);
        int x = g_gui_w / 2 - 100;
        int y = 96;
        if (mode != 1) {
            draw_text(tr_key_default("addServer.enterName", "Server Name"), x, y - 14, 10526880);
            draw_rect(x - 1, y - 1, x + 201, y + 21, pex_mp_server_edit_field_get() == 1 ? -1 : -6250336);
            draw_rect(x, y, x + 200, y + 20, -16777216);
            char name_line[96];
            snprintf(name_line, sizeof(name_line), "%s%s", pex_mp_server_edit_name_get(), (pex_mp_server_edit_field_get() == 1 && g_ticks / 6 % 2 == 0) ? "_" : "");
            draw_text(name_line, x + 4, y + 6, 14737632);
            y += 44;
        }
        draw_text(tr_key_default("addServer.enterIp", "Server Address"), x, y - 14, 10526880);
        draw_rect(x - 1, y - 1, x + 201, y + 21, pex_mp_server_edit_field_get() == 0 ? -1 : -6250336);
        draw_rect(x, y, x + 200, y + 20, -16777216);
        char address_line[128];
        snprintf(address_line, sizeof(address_line), "%s%s", pex_mp_server_edit_address_get(), (pex_mp_server_edit_field_get() == 0 && g_ticks / 6 % 2 == 0) ? "_" : "");
        draw_text(address_line, x + 4, y + 6, 14737632);
        draw_all_buttons();
        return;
    }

    int top = 32;
    int bottom = g_gui_h - 64;
    int left = g_gui_w / 2 - 110;
    int right = g_gui_w / 2 + 110;
    pex_mp_server_clamp_scroll();
    draw_tiled_rect_tint(0, top, g_gui_w, bottom, 0x202020, 0);

    int visible = pex_mp_server_visible_rows();
    int scroll = pex_mp_server_scroll_get();
    for (int row = 0; row < visible; ++row) {
        int index = scroll + row;
        const PexMpServerEntry *e = pex_mp_server_entry_get(index);
        if (!e) break;
        int y = top + 4 + row * 36;
        if (index == pex_mp_server_selected_get()) {
            draw_rect(left, y - 2, right, y + 34, 8421504);
            draw_rect(left + 1, y - 1, right - 1, y + 33, 0);
        }
        draw_text(e->name[0] ? e->name : "Minecraft Server", left + 2, y + 1, 16777215);
        int motd_color = (e->ping_ms < 0 && e->polled) ? 0xFF5555 : 8421504;
        draw_text(e->motd[0] ? e->motd : (e->polling ? "Polling.." : ""), left + 2, y + 12, motd_color);
        if (e->player_count[0]) {
            int w = text_width(e->player_count);
            draw_text(e->player_count, left + 215 - w, y + 12, 8421504);
        }
        draw_text(e->host, left + 2, y + 23, 3158064);
        int bars = 5;
        const char *tip = "Polling..";
        if (e->polled && !e->polling) {
            if (e->ping_ms < 0) { bars = 0; tip = "(no connection)"; }
            else if (e->ping_ms < 150) { bars = 5; tip = "fast"; }
            else if (e->ping_ms < 300) { bars = 4; tip = "good"; }
            else if (e->ping_ms < 600) { bars = 3; tip = "slow"; }
            else if (e->ping_ms < 1000) { bars = 2; tip = "very slow"; }
            else { bars = 1; tip = "bad"; }
        } else {
            bars = 1 + ((g_ticks / 8 + index) % 5);
        }
        int bx = left + 205;
        for (int b = 0; b < 5; ++b) {
            int bh = 2 + b;
            int col = (b < bars) ? 0x55FF55 : 0x555555;
            draw_rect(bx + b * 2, y + 8 - bh, bx + b * 2 + 1, y + 8, col);
        }
        (void)tip;
    }

    if (pex_mp_server_count_get() == 0) {
        draw_centered_text("No servers added", g_gui_w / 2, top + 24, 8421504);
    }

    draw_tiled_rect_tint(0, 0, g_gui_w, top, 0x404040, 0);
    draw_tiled_rect_tint(0, bottom, g_gui_w, g_gui_h, 0x404040, 0);
    draw_gradient(0, top, g_gui_w, top + 4, 0xFF000000, 0x00000000);
    draw_gradient(0, bottom - 4, g_gui_w, bottom, 0x00000000, 0xFF000000);
    draw_centered_text(tr_key_default("multiplayer.title", "Play Multiplayer"), g_gui_w / 2, 20, 16777215);
    if (g_multiplayer_status[0]) draw_centered_text(g_multiplayer_status, g_gui_w / 2, bottom + 5, 10526880);
    draw_all_buttons();
}

static void draw_connecting_screen(void) {
    draw_default_bg();

    int pct = g_mp_connect_progress;
    if (g_mp_expected_chunks > 0 && !g_mp_world_ready) pct = (g_mp_chunks_received * 100) / g_mp_expected_chunks;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;

    const char *top = tr_key_default("connect.connecting", "Locating server");
    if (g_mp_expected_chunks > 0 || g_mp_chunks_received > 0 || pct >= 65) top = tr_key_default("downloadTerrain", "Downloading terrain");
    if (g_mp_world_ready || pct >= 100) top = tr_key_default("gui.done", "Done!");

    int cx = g_gui_w / 2;
    int cy = g_gui_h / 2;
    draw_centered_text(top, cx, cy - 8, 16777215);

    if (pct >= 0) {
        int bw = 100;
        int bh = 2;
        int bx = cx - bw / 2;
        int by = cy + 16;
        draw_rect(bx, by, bx + bw, by + bh, 8421504);
        draw_rect(bx, by, bx + pct, by + bh, 8454016);
    }

    draw_all_buttons();
}

static void draw_texpack(void) {
    draw_default_bg();
    int top = 32;
    int bottom = g_gui_h - 58 + 4;
    clamp_texpack_scroll();

    draw_tiled_rect_tint(0, top, g_gui_w, bottom, 0x202020, g_texpack_scroll);

    int left = g_gui_w / 2 - 110;
    int right = g_gui_w / 2 + 110;
    for (int i = 0; i < g_texpack_count; i++) {
        TexturePackEntry *e = &g_texpacks[i];
        int row_y = 36 + i * 36 - g_texpack_scroll;
        if (row_y > bottom || row_y + 32 < top) continue;

        if (i == g_selected_texpack) {
            draw_rect(left, row_y - 2, right, row_y + 34, 8421504);
            draw_rect(left + 1, row_y - 1, right - 1, row_y + 33, 0);
        }

        Texture *icon = NULL;
        if (!e->no_icon) {
            if (e->is_default) icon = tex_default_pack_icon.id ? &tex_default_pack_icon : &tex_pack;
            else if (e->has_icon) icon = &e->icon;
            else icon = &tex_unknown_pack;
            draw_texture_scaled_full(icon, g_gui_w / 2 - 92 - 16, row_y, 32, 32, 0xFFFFFF);
        }
        draw_text(e->name, g_gui_w / 2 - 92 - 16 + 34, row_y + 1, 16777215);
        draw_text(e->desc1, g_gui_w / 2 - 92 - 16 + 34, row_y + 12, 8421504);
        draw_text(e->desc2, g_gui_w / 2 - 92 - 16 + 34, row_y + 22, 8421504);
    }

    {
        int max_scroll = g_texpack_count * 36 - (bottom - top - 4);
        if (max_scroll < 0) max_scroll = 0;
        if (max_scroll > 0) {
            int content_h = g_texpack_count * 36;
            int view_h = bottom - top;
            int bar_x0 = g_gui_w / 2 + 124;
            int bar_x1 = bar_x0 + 6;
            int thumb_h = content_h > 0 ? (view_h * view_h / content_h) : view_h;
            int thumb_y;
            if (thumb_h < 32) thumb_h = 32;
            if (thumb_h > view_h - 8) thumb_h = view_h - 8;
            thumb_y = top + g_texpack_scroll * (view_h - thumb_h) / max_scroll;
            if (thumb_y < top) thumb_y = top;
            draw_rect(bar_x0, top, bar_x1, bottom, 0);
            draw_rect(bar_x0, thumb_y, bar_x1, thumb_y + thumb_h, 8421504);
            draw_rect(bar_x0, thumb_y, bar_x1 - 1, thumb_y + thumb_h - 1, 12632256);
        }
    }

    draw_tiled_rect_tint(0, 0, g_gui_w, top, 0x404040, 0);
    draw_tiled_rect_tint(0, bottom, g_gui_w, g_gui_h, 0x404040, 0);
    draw_gradient(0, top, g_gui_w, top + 4, 0xFF000000, 0x00000000);
    draw_gradient(0, bottom - 4, g_gui_w, bottom, 0x00000000, 0xFF000000);

    draw_centered_text(tr_key_default("texturePack.title", "Select Texture Pack"), g_gui_w / 2, 16, 16777215);
    draw_centered_text(tr_key_default("texturePack.folderInfo", "(Place texture pack files here)"), g_gui_w / 2 - 77, g_gui_h - 26, 8421504);
    draw_all_buttons();
}
