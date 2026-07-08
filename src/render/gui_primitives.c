/* Split from original monolithic main.c. Included by src/main.c unity build. */

static void setup_scale(void) {
    g_gui_scale = 1;
    while (g_win_w / (g_gui_scale + 1) >= 320 && g_win_h / (g_gui_scale + 1) >= 240) g_gui_scale++;
    g_gui_w = g_win_w / g_gui_scale;
    g_gui_h = g_win_h / g_gui_scale;
}

static void setup_gui_projection(void) {
    glViewport(0, 0, g_render_w, g_render_h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (double)g_gui_w, (double)g_gui_h, 0.0, 1000.0, 3000.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -2000.0f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    glColor4f(1,1,1,1);
}

static void color_from_int_alpha(int color, float *r, float *g, float *b, float *a, int zero_alpha_is_opaque) {
    unsigned int c = (unsigned int)color;
    *a = ((c >> 24) & 255) / 255.0f;
    if (zero_alpha_is_opaque && *a == 0.0f) *a = 1.0f;
    *r = ((c >> 16) & 255) / 255.0f;
    *g = ((c >> 8) & 255) / 255.0f;
    *b = (c & 255) / 255.0f;
}

static void color_from_int(int color, float *r, float *g, float *b, float *a) {
    color_from_int_alpha(color, r, g, b, a, 1);
}

static void set_color_int(int color) {
    float r,g,b,a;
    color_from_int_alpha(color, &r, &g, &b, &a, 1);
    glColor4f(r,g,b,a);
}

static void draw_rect(int x1, int y1, int x2, int y2, int color) {
    glDisable(GL_TEXTURE_2D);
    set_color_int(color);
    if (g_loggy_enabled) g_loggy_gui_quads++;
    glBegin(GL_QUADS);
    glVertex3f((float)x1, (float)y2, 0.0f);
    glVertex3f((float)x2, (float)y2, 0.0f);
    glVertex3f((float)x2, (float)y1, 0.0f);
    glVertex3f((float)x1, (float)y1, 0.0f);
    glEnd();
    glEnable(GL_TEXTURE_2D);
}

static void gradient_color_from_int(int color, float *r, float *g, float *b, float *a) {
    unsigned int c = (unsigned int)color;
    *a = ((c >> 24) & 255) / 255.0f;
    *r = ((c >> 16) & 255) / 255.0f;
    *g = ((c >> 8) & 255) / 255.0f;
    *b = (c & 255) / 255.0f;
}

static GLint pex_gl_save_shade_model(void) {
#if defined(GL_SHADE_MODEL) && (defined(GL_SMOOTH) || defined(GL_FLAT))
    GLint old_mode = 0;
    glGetIntegerv(GL_SHADE_MODEL, &old_mode);
    return old_mode;
#else
    return 0;
#endif
}

static void pex_gl_restore_shade_model(GLint old_mode) {
#if defined(GL_SMOOTH) && defined(GL_FLAT)
    if (old_mode == GL_SMOOTH || old_mode == GL_FLAT) glShadeModel((GLenum)old_mode);
#else
    (void)old_mode;
#endif
}

static void pex_gl_shade_model_smooth(void) {
#ifdef GL_SMOOTH
    glShadeModel(GL_SMOOTH);
#endif
}

static void draw_gradient(int x1, int y1, int x2, int y2, int c1, int c2) {
    float r1,g1,b1,a1,r2,g2,b2,a2;
    GLint old_shade_model = pex_gl_save_shade_model();
    color_from_int_alpha(c1, &r1,&g1,&b1,&a1, 0);
    color_from_int_alpha(c2, &r2,&g2,&b2,&a2, 0);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    pex_gl_shade_model_smooth();
    if (g_loggy_enabled) g_loggy_gui_quads++;
    glBegin(GL_QUADS);
    glColor4f(r1,g1,b1,a1); glVertex3f((float)x2, (float)y1, 0.0f); glVertex3f((float)x1, (float)y1, 0.0f);
    glColor4f(r2,g2,b2,a2); glVertex3f((float)x1, (float)y2, 0.0f); glVertex3f((float)x2, (float)y2, 0.0f);
    glEnd();
    pex_gl_restore_shade_model(old_shade_model);
    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1,1,1,1);
}

static void draw_textured_rect_tex(Texture *tex, int x, int y, int sx, int sy, int w, int h, int color) {
    if (!tex || !tex->id || tex->w <= 0 || tex->h <= 0 || w <= 0 || h <= 0) return;
    float u0 = ((float)sx + 0.5f) / (float)tex->w;
    float v0 = ((float)sy + 0.5f) / (float)tex->h;
    float u1 = ((float)(sx + w) - 0.5f) / (float)tex->w;
    float v1 = ((float)(sy + h) - 0.5f) / (float)tex->h;
    if (u1 < u0) { float mid = ((float)sx + (float)(sx + w)) * 0.5f / (float)tex->w; u0 = u1 = mid; }
    if (v1 < v0) { float mid = ((float)sy + (float)(sy + h)) * 0.5f / (float)tex->h; v0 = v1 = mid; }
    glBindTexture(GL_TEXTURE_2D, tex->id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    set_color_int(color);
    if (g_loggy_enabled) g_loggy_gui_quads++;
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v1); glVertex3f((float)x, (float)(y + h), 0.0f);
    glTexCoord2f(u1, v1); glVertex3f((float)(x + w), (float)(y + h), 0.0f);
    glTexCoord2f(u1, v0); glVertex3f((float)(x + w), (float)y, 0.0f);
    glTexCoord2f(u0, v0); glVertex3f((float)x, (float)y, 0.0f);
    glEnd();
    glColor4f(1,1,1,1);
}


static void draw_textured_rect_256(Texture *tex, int x, int y, int sx, int sy, int w, int h, int color) {
    if (!tex || !tex->id || w <= 0 || h <= 0) return;
    /* Java Gui.drawTexturedModalRect uses a fixed 1/256 atlas coordinate scale.
       Do not divide by the uploaded texture's pixel size here: HD/converted GUI
       sheets keep the same 256x256 logical coordinate space, and using the
       physical size samples the wrong heart/hunger/XP pixels. */
    const float us = 1.0f / 256.0f;
    const float vs = 1.0f / 256.0f;
    glBindTexture(GL_TEXTURE_2D, tex->id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    set_color_int(color);
    if (g_loggy_enabled) g_loggy_gui_quads++;
    glBegin(GL_QUADS);
    glTexCoord2f((float)sx * us, (float)(sy + h) * vs); glVertex3f((float)x, (float)(y + h), 0.0f);
    glTexCoord2f((float)(sx + w) * us, (float)(sy + h) * vs); glVertex3f((float)(x + w), (float)(y + h), 0.0f);
    glTexCoord2f((float)(sx + w) * us, (float)sy * vs); glVertex3f((float)(x + w), (float)y, 0.0f);
    glTexCoord2f((float)sx * us, (float)sy * vs); glVertex3f((float)x, (float)y, 0.0f);
    glEnd();
    glColor4f(1,1,1,1);
}

static void draw_textured_modal_rect(Texture *tex, int x, int y, int sx, int sy, int w, int h, int color) {
    if (!tex || !tex->id || tex->w <= 0 || tex->h <= 0 || w <= 0 || h <= 0) return;
    float us = 1.0f / (float)tex->w;
    float vs = 1.0f / (float)tex->h;
    glBindTexture(GL_TEXTURE_2D, tex->id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    set_color_int(color);
    if (g_loggy_enabled) g_loggy_gui_quads++;
    glBegin(GL_QUADS);
    glTexCoord2f((float)(sx + 0) * us, (float)(sy + h) * vs); glVertex3f((float)(x + 0), (float)(y + h), 0.0f);
    glTexCoord2f((float)(sx + w) * us, (float)(sy + h) * vs); glVertex3f((float)(x + w), (float)(y + h), 0.0f);
    glTexCoord2f((float)(sx + w) * us, (float)(sy + 0) * vs); glVertex3f((float)(x + w), (float)(y + 0), 0.0f);
    glTexCoord2f((float)(sx + 0) * us, (float)(sy + 0) * vs); glVertex3f((float)(x + 0), (float)(y + 0), 0.0f);
    glEnd();
    glColor4f(1,1,1,1);
}

static void draw_textured_rect_part_scaled(Texture *tex, int x, int y, int dw, int dh, int sx, int sy, int sw, int sh, int color) {
    if (!tex || tex->id == 0 || tex->w <= 0 || tex->h <= 0 || dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0) return;
    float u0 = ((float)sx + 0.5f) / (float)tex->w;
    float v0 = ((float)sy + 0.5f) / (float)tex->h;
    float u1 = ((float)(sx + sw) - 0.5f) / (float)tex->w;
    float v1 = ((float)(sy + sh) - 0.5f) / (float)tex->h;
    if (u1 < u0) { float mid = ((float)sx + (float)(sx + sw)) * 0.5f / (float)tex->w; u0 = u1 = mid; }
    if (v1 < v0) { float mid = ((float)sy + (float)(sy + sh)) * 0.5f / (float)tex->h; v0 = v1 = mid; }
    glBindTexture(GL_TEXTURE_2D, tex->id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    set_color_int(color);
    if (g_loggy_enabled) g_loggy_gui_quads++;
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v1); glVertex3f((float)x, (float)(y + dh), 0.0f);
    glTexCoord2f(u1, v1); glVertex3f((float)(x + dw), (float)(y + dh), 0.0f);
    glTexCoord2f(u1, v0); glVertex3f((float)(x + dw), (float)y, 0.0f);
    glTexCoord2f(u0, v0); glVertex3f((float)x, (float)y, 0.0f);
    glEnd();
    glColor4f(1,1,1,1);
}

static void draw_texture_scaled_full(Texture *tex, int x, int y, int w, int h, int color) {
    if (!tex || tex->id == 0 || tex->w <= 0 || tex->h <= 0 || w <= 0 || h <= 0) return;
    float inset_u = 0.5f / (float)tex->w;
    float inset_v = 0.5f / (float)tex->h;
    glBindTexture(GL_TEXTURE_2D, tex->id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    set_color_int(color);
    if (g_loggy_enabled) g_loggy_gui_quads++;
    glBegin(GL_QUADS);
    glTexCoord2f(inset_u, 1.0f - inset_v); glVertex3f((float)x, (float)(y + h), 0.0f);
    glTexCoord2f(1.0f - inset_u, 1.0f - inset_v); glVertex3f((float)(x + w), (float)(y + h), 0.0f);
    glTexCoord2f(1.0f - inset_u, inset_v); glVertex3f((float)(x + w), (float)y, 0.0f);
    glTexCoord2f(inset_u, inset_v); glVertex3f((float)x, (float)y, 0.0f);
    glEnd();
    glColor4f(1,1,1,1);
}

static void draw_tiled_rect_tint(int x1, int y1, int x2, int y2, int color, int scroll) {
    if (x2 <= x1 || y2 <= y1) return;
    glBindTexture(GL_TEXTURE_2D, tex_bg.id);
    set_color_int(color);
    if (g_loggy_enabled) g_loggy_gui_quads++;
    glBegin(GL_QUADS);
    glTexCoord2f((float)x1 / 32.0f, (float)(y2 + scroll) / 32.0f); glVertex3f((float)x1, (float)y2, 0.0f);
    glTexCoord2f((float)x2 / 32.0f, (float)(y2 + scroll) / 32.0f); glVertex3f((float)x2, (float)y2, 0.0f);
    glTexCoord2f((float)x2 / 32.0f, (float)(y1 + scroll) / 32.0f); glVertex3f((float)x2, (float)y1, 0.0f);
    glTexCoord2f((float)x1 / 32.0f, (float)(y1 + scroll) / 32.0f); glVertex3f((float)x1, (float)y1, 0.0f);
    glEnd();
    glColor4f(1,1,1,1);
}


static void draw_tiled_overlay_background(int y1, int y2, int alpha_top, int alpha_bottom) {
    if (y2 <= y1) return;
    glBindTexture(GL_TEXTURE_2D, tex_bg.id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (g_loggy_enabled) g_loggy_gui_quads++;
    glBegin(GL_QUADS);
    glColor4f(0.25f, 0.25f, 0.25f, (float)alpha_bottom / 255.0f);
    glTexCoord2f(0.0f, (float)y2 / 32.0f); glVertex3f(0.0f, (float)y2, 0.0f);
    glTexCoord2f((float)g_gui_w / 32.0f, (float)y2 / 32.0f); glVertex3f((float)g_gui_w, (float)y2, 0.0f);
    glColor4f(0.25f, 0.25f, 0.25f, (float)alpha_top / 255.0f);
    glTexCoord2f((float)g_gui_w / 32.0f, (float)y1 / 32.0f); glVertex3f((float)g_gui_w, (float)y1, 0.0f);
    glTexCoord2f(0.0f, (float)y1 / 32.0f); glVertex3f(0.0f, (float)y1, 0.0f);
    glEnd();
    glColor4f(1,1,1,1);
}

static void draw_tiled_background_tint(int color, int scroll) {
    glBindTexture(GL_TEXTURE_2D, tex_bg.id);
    set_color_int(color);
    if (g_loggy_enabled) g_loggy_gui_quads++;
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, (float)(g_gui_h + scroll) / 32.0f); glVertex3f(0.0f, (float)g_gui_h, 0.0f);
    glTexCoord2f((float)g_gui_w / 32.0f, (float)(g_gui_h + scroll) / 32.0f); glVertex3f((float)g_gui_w, (float)g_gui_h, 0.0f);
    glTexCoord2f((float)g_gui_w / 32.0f, (float)scroll / 32.0f); glVertex3f((float)g_gui_w, 0.0f, 0.0f);
    glTexCoord2f(0.0f, (float)scroll / 32.0f); glVertex3f(0.0f, 0.0f, 0.0f);
    glEnd();
    glColor4f(1,1,1,1);
}

static void draw_default_bg(void) {
    draw_tiled_background_tint(0x404040, 0);
}

static unsigned int font_utf8_next(const unsigned char **pp) {
    const unsigned char *p = pp && *pp ? *pp : (const unsigned char *)"";
    unsigned int cp;
    if (!*p) return 0;
    if (*p < 0x80u) { cp = *p++; *pp = p; return cp; }
    if ((*p & 0xE0u) == 0xC0u && (p[1] & 0xC0u) == 0x80u) {
        cp = ((*p & 0x1Fu) << 6) | (p[1] & 0x3Fu);
        p += 2; *pp = p; return cp;
    }
    if ((*p & 0xF0u) == 0xE0u && (p[1] & 0xC0u) == 0x80u && (p[2] & 0xC0u) == 0x80u) {
        cp = ((*p & 0x0Fu) << 12) | ((p[1] & 0x3Fu) << 6) | (p[2] & 0x3Fu);
        p += 3; *pp = p; return cp;
    }
    if ((*p & 0xF8u) == 0xF0u && (p[1] & 0xC0u) == 0x80u && (p[2] & 0xC0u) == 0x80u && (p[3] & 0xC0u) == 0x80u) {
        cp = ((*p & 0x07u) << 18) | ((p[1] & 0x3Fu) << 12) | ((p[2] & 0x3Fu) << 6) | (p[3] & 0x3Fu);
        p += 4; *pp = p; return cp;
    }
    cp = *p++;
    *pp = p;
    return cp;
}

static int font_utf8_put(char *out, size_t cap, size_t *n, unsigned int cp) {
    if (!out || !n || cap == 0) return 0;
    if (cp <= 0x7Fu) {
        if (*n + 1 >= cap) return 0;
        out[(*n)++] = (char)cp;
    } else if (cp <= 0x7FFu) {
        if (*n + 2 >= cap) return 0;
        out[(*n)++] = (char)(0xC0u | (cp >> 6));
        out[(*n)++] = (char)(0x80u | (cp & 0x3Fu));
    } else if (cp <= 0xFFFFu) {
        if (*n + 3 >= cap) return 0;
        out[(*n)++] = (char)(0xE0u | (cp >> 12));
        out[(*n)++] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        out[(*n)++] = (char)(0x80u | (cp & 0x3Fu));
    } else {
        if (*n + 4 >= cap) return 0;
        out[(*n)++] = (char)(0xF0u | (cp >> 18));
        out[(*n)++] = (char)(0x80u | ((cp >> 12) & 0x3Fu));
        out[(*n)++] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        out[(*n)++] = (char)(0x80u | (cp & 0x3Fu));
    }
    out[*n] = 0;
    return 1;
}

static int font_codepoint_is_rtl(unsigned int cp) {
    return (cp >= 0x0590u && cp <= 0x08FFu) ||
           (cp >= 0xFB1Du && cp <= 0xFDFFu) ||
           (cp >= 0xFE70u && cp <= 0xFEFFu);
}

static int font_codepoint_is_ltr(unsigned int cp) {
    return (cp >= '0' && cp <= '9') ||
           (cp >= 'A' && cp <= 'Z') ||
           (cp >= 'a' && cp <= 'z') ||
           (cp >= 0x00C0u && cp <= 0x024Fu) ||
           (cp >= 0x0400u && cp <= 0x052Fu);
}

static unsigned int font_bidi_mirror(unsigned int cp) {
    switch (cp) {
        case '(': return ')'; case ')': return '(';
        case '[': return ']'; case ']': return '[';
        case '{': return '}'; case '}': return '{';
        case '<': return '>'; case '>': return '<';
        default: return cp;
    }
}

static int font_bidi_reorder_utf8(const char *src, char *dst, size_t cap) {
    enum { MAX_CPS = 2048, MAX_RUNS = 256 };
    unsigned int cps[MAX_CPS];
    int types[MAX_CPS];
    int run_start[MAX_RUNS], run_end[MAX_RUNS], run_type[MAX_RUNS];
    int n = 0, has_rtl = 0, runs = 0;
    const unsigned char *p = (const unsigned char *)src;
    if (!src || !dst || cap == 0) return 0;
    while (*p && n < MAX_CPS) {
        unsigned int cp = font_utf8_next(&p);
        cps[n++] = cp;
        if (font_codepoint_is_rtl(cp)) has_rtl = 1;
    }
    if (!has_rtl) return 0;
    for (int i = 0; i < n; ++i) {
        if (font_codepoint_is_rtl(cps[i])) types[i] = 1;
        else if (font_codepoint_is_ltr(cps[i])) types[i] = 0;
        else types[i] = -1;
    }
    /* Java uses java.text.Bidi.  This compact fallback covers the language-list
       use case: an RTL base paragraph with visually reordered RTL runs and
       mirrored brackets, while preserving LTR runs such as numbers/US tags. */
    for (int i = 0; i < n; ) {
        int t = types[i];
        int j;
        if (t < 0) t = (i > 0 && types[i - 1] >= 0) ? types[i - 1] : 1;
        j = i + 1;
        while (j < n) {
            int tj = types[j];
            if (tj < 0) tj = t;
            if (tj != t) break;
            ++j;
        }
        if (runs < MAX_RUNS) {
            run_start[runs] = i;
            run_end[runs] = j;
            run_type[runs] = t;
            ++runs;
        }
        i = j;
    }
    dst[0] = 0;
    size_t outn = 0;
    for (int r = runs - 1; r >= 0; --r) {
        if (run_type[r]) {
            for (int i = run_end[r] - 1; i >= run_start[r]; --i) {
                if (!font_utf8_put(dst, cap, &outn, font_bidi_mirror(cps[i]))) break;
            }
        } else {
            for (int i = run_start[r]; i < run_end[r]; ++i) {
                if (!font_utf8_put(dst, cap, &outn, cps[i])) break;
            }
        }
    }
    return 1;
}

#define PEX_FONT_ALLOWED_MAX 2048
static unsigned int font_allowed_cps[PEX_FONT_ALLOWED_MAX];
static int font_allowed_count = -1;
static int font_color_code[32];

static void font_allowed_append(unsigned int cp) {
    if (font_allowed_count >= 0 && font_allowed_count < PEX_FONT_ALLOWED_MAX) {
        font_allowed_cps[font_allowed_count++] = cp;
    }
}

static void font_allowed_load_fallback(void) {
    if (font_allowed_count < 0) font_allowed_count = 0;
    /* Last-resort order.  The real Java path is /font.txt; this keeps ASCII
       sane if a damaged pack lacks it, while extracted client.jar packs use
       the exact ChatAllowedCharacters.allowedCharacters order. */
    for (unsigned int cp = 32u; cp < 256u; ++cp) font_allowed_append(cp);
}

static void font_allowed_load(void) {
    char root[MAX_PATHBUF], path[MAX_PATHBUF], line[2048];
    FILE *f;
    if (font_allowed_count >= 0) return;
    font_allowed_count = 0;
    pack_asset_path(root, sizeof(root));
    snprintf(path, sizeof(path), "%s\\font.txt", root);
    f = fopen(path, "rb");
    if (!f) {
        snprintf(path, sizeof(path), "%s/font.txt", root);
        f = fopen(path, "rb");
    }
    if (!f) { font_allowed_load_fallback(); return; }
    while (fgets(line, sizeof(line), f)) {
        const unsigned char *q;
        char *e = line + strlen(line);
        while (e > line && (e[-1] == '\n' || e[-1] == '\r')) *--e = 0;
        if (line[0] == '#') continue;
        q = (const unsigned char *)line;
        while (*q) font_allowed_append(font_utf8_next(&q));
    }
    fclose(f);
    if (font_allowed_count <= 0) font_allowed_load_fallback();
}

static int font_allowed_index(unsigned int cp) {
    font_allowed_load();
    for (int i = 0; i < font_allowed_count; ++i) {
        if (font_allowed_cps[i] == cp) return i;
    }
    return -1;
}

static int font_default_glyph_index(unsigned int cp) {
    int idx = font_allowed_index(cp);
    if (idx < 0) return -1;
    idx += 32;
    return (idx >= 0 && idx < 256) ? idx : -1;
}

static int font_codepoint_is_directional_control(unsigned int cp) {
    return cp == 0x200Eu || cp == 0x200Fu ||
           (cp >= 0x202Au && cp <= 0x202Eu) ||
           (cp >= 0x2066u && cp <= 0x2069u);
}

static void font_unload_unicode_pages(void) {
    for (int i = 0; i < 256; ++i) free_texture(&tex_font_glyph[i]);
    memset(font_glyph_widths, 0, sizeof(font_glyph_widths));
    font_glyph_widths_loaded = 0;
    font_allowed_count = -1;
}

static int font_load_glyph_widths(void) {
    char root[MAX_PATHBUF], path[MAX_PATHBUF];
    FILE *f;
    size_t got;
    if (font_glyph_widths_loaded) return font_glyph_widths_loaded > 0;
    memset(font_glyph_widths, 0, sizeof(font_glyph_widths));
    pack_asset_path(root, sizeof(root));
    snprintf(path, sizeof(path), "%s\\font\\glyph_sizes.bin", root);
    f = fopen(path, "rb");
    if (!f) {
        snprintf(path, sizeof(path), "%s/font/glyph_sizes.bin", root);
        f = fopen(path, "rb");
    }
    if (!f) { font_glyph_widths_loaded = -1; return 0; }
    got = fread(font_glyph_widths, 1, sizeof(font_glyph_widths), f);
    fclose(f);
    font_glyph_widths_loaded = got == sizeof(font_glyph_widths) ? 1 : (got > 0 ? 1 : -1);
    return font_glyph_widths_loaded > 0;
}

static Texture *font_load_glyph_page(unsigned int page) {
    char rel[64];
    if (page >= 256u) return NULL;
    if (!tex_font_glyph[page].id) {
        snprintf(rel, sizeof(rel), "font\\glyph_%02X.png", page & 255u);
        try_release_texture(&tex_font_glyph[page], rel, 0);
    }
    return tex_font_glyph[page].id ? &tex_font_glyph[page] : NULL;
}

static int font_unicode_width(unsigned int cp) {
    unsigned char b;
    int left, right;
    if (cp == 32u) return 4;
    if (cp >= 65536u || !font_load_glyph_widths()) return 0;
    b = font_glyph_widths[cp];
    if (!b) return 0;
    left = b >> 4;
    right = b & 15;
    if (right > 7) { right = 15; left = 0; }
    ++right;
    return (right - left) / 2 + 1;
}

static int font_codepoint_width(unsigned int cp, int unicode_flag) {
    int glyph;
    if (cp == 0xA7u) return -1;
    if (font_codepoint_is_directional_control(cp)) return 0;
    if (cp == 32u) return 4;
    glyph = font_default_glyph_index(cp);
    if (glyph >= 0 && !unicode_flag) return font_widths[glyph];
    if (cp < 65536u) {
        int w = font_unicode_width(cp);
        if (w > 0) return w;
    }
    if (glyph >= 0) return font_widths[glyph];
    return 0;
}

static void init_font_color_codes(void) {
    for (int i = 0; i < 32; ++i) {
        int base = (i >> 3 & 1) * 85;
        int r = (i >> 2 & 1) * 170 + base;
        int g = (i >> 1 & 1) * 170 + base;
        int b = (i >> 0 & 1) * 170 + base;
        if (i == 6) r += 85;
        if (i >= 16) { r /= 4; g /= 4; b /= 4; }
        font_color_code[i] = ((r & 255) << 16) | ((g & 255) << 8) | (b & 255);
    }
}

static void init_font_widths(void) {
    font_unload_unicode_pages();
    memset(font_widths, 0, sizeof(font_widths));
    for (int i = 0; i < 256; i++) {
        int col = i % 16;
        int row = i / 16;
        int last;
        for (last = 7; last >= 0; last--) {
            int empty = 1;
            int px = col * 8 + last;
            for (int yy = 0; yy < 8 && empty; yy++) {
                int py = row * 8 + yy;
                unsigned char *p = &tex_font.rgba[(py * tex_font.w + px) * 4];
                /* Java FontRenderer reads getRGB() & 255, so only the blue
                   channel decides whether a default-font column is empty. */
                if (p[2] > 0) empty = 0;
            }
            if (!empty) break;
        }
        if (i == 32) last = 2;
        font_widths[i] = last + 2;
    }
    init_font_color_codes();
}

static int text_width(const char *s) {
    int w = 0;
    int unicode_flag = pex_current_language_is_unicode();
    int bold = 0;
    const unsigned char *p = (const unsigned char*)s;
    if (!s) return 0;
    while (*p) {
        unsigned int cp = font_utf8_next(&p);
        int cw;
        if (font_codepoint_is_directional_control(cp)) continue;
        if (cp == 0xA7u && *p) {
            unsigned int fmt = font_utf8_next(&p);
            if (fmt >= 'A' && fmt <= 'Z') fmt += 32u;
            if (fmt == 'l') bold = 1;
            else if (fmt == 'r' || (fmt >= '0' && fmt <= '9') || (fmt >= 'a' && fmt <= 'f')) bold = 0;
            continue;
        }
        cw = font_codepoint_width(cp, unicode_flag);
        if (cw > 0) {
            w += cw;
            if (bold) ++w;
        }
    }
    return w;
}

static int shadow_color(int color) {
    unsigned int c = (unsigned int)color;
    unsigned int a = c & 0xFF000000u;
    return (int)(a + ((c & 0xFCFCFCu) >> 2));
}

static void draw_font_quad(Texture *tex, float x0, float y0, float x1, float y1,
                           float u0, float v0, float u1, float v1, int color) {
    if (!tex || !tex->id) return;
    glBindTexture(GL_TEXTURE_2D, tex->id);
    set_color_int(color);
    if (g_loggy_enabled) g_loggy_gui_quads++;
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v1); glVertex3f(x0, y1, 0.0f);
    glTexCoord2f(u1, v1); glVertex3f(x1, y1, 0.0f);
    glTexCoord2f(u1, v0); glVertex3f(x1, y0, 0.0f);
    glTexCoord2f(u0, v0); glVertex3f(x0, y0, 0.0f);
    glEnd();
}

static float draw_default_glyph_java(int glyph, float x, float y, int color, int italic) {
    float sx, sy, w, skew;
    if (glyph < 0 || glyph >= 256 || !tex_font.id) return 0.0f;
    sx = (float)(glyph % 16 * 8);
    sy = (float)(glyph / 16 * 8);
    w = (float)font_widths[glyph] - 0.01f;
    if (w < 0.0f) w = 0.0f;
    skew = italic ? 1.0f : 0.0f;
    /* Same logical atlas as Java 1.2.5: default.png is addressed as 128x128. */
    draw_font_quad(&tex_font,
                   x - skew, y, x + w + skew, y + 7.99f,
                   sx / 128.0f, sy / 128.0f,
                   (sx + w) / 128.0f, (sy + 7.99f) / 128.0f,
                   color);
    return (float)font_widths[glyph];
}

static float draw_unicode_glyph_java(unsigned int cp, float x, float y, int color, int italic) {
    unsigned char b;
    int left, right;
    float src_x, src_y, src_w, skew;
    Texture *page_tex;
    if (cp >= 65536u || !font_load_glyph_widths()) return 0.0f;
    b = font_glyph_widths[cp];
    if (!b) return 0.0f;
    page_tex = font_load_glyph_page(cp / 256u);
    if (!page_tex) return 0.0f;
    left = b >> 4;
    right = b & 15;
    if (right > 7) { right = 15; left = 0; }
    src_x = (float)(cp % 16u * 16u) + (float)left;
    src_y = (float)((cp & 255u) / 16u * 16u);
    src_w = (float)(right + 1 - left) - 0.02f;
    if (src_w <= 0.0f) return 0.0f;
    skew = italic ? 1.0f : 0.0f;
    /* Java func_50111_a draws 16px-high Unicode glyph cells at 8px GUI height,
       with 256x256 logical glyph-page coordinates and fractional quad width. */
    draw_font_quad(page_tex,
                   x - skew, y, x + src_w / 2.0f + skew, y + 7.99f,
                   src_x / 256.0f, src_y / 256.0f,
                   (src_x + src_w) / 256.0f, (src_y + 15.98f) / 256.0f,
                   color);
    return (float)(right + 1 - left) / 2.0f + 1.0f;
}

static void draw_text_line(float x0, float x1, float y, int color) {
    draw_rect((int)x0, (int)y, (int)(x1 + 1.0f), (int)(y + 1.0f), color);
}

static int font_format_index(unsigned int cp) {
    static const char *codes = "0123456789abcdefklmnor";
    if (cp >= 'A' && cp <= 'Z') cp += 32u;
    for (int i = 0; codes[i]; ++i) if ((unsigned int)codes[i] == cp) return i;
    return -1;
}

static void draw_text_raw_impl(const char *s, int x, int y, int color, int force_bidi, int shadow_pass) {
    char bidi_buf[4096];
    int unicode_flag = pex_current_language_is_unicode();
    float cx = (float)x;
    int base_color = color;
    int cur_color = color;
    int obfuscated = 0, bold = 0, strike = 0, underline = 0, italic = 0;
    const unsigned char *p;
    if (!s) return;
    if ((force_bidi || pex_current_language_is_bidi()) && font_bidi_reorder_utf8(s, bidi_buf, sizeof(bidi_buf))) s = bidi_buf;
    if (g_loggy_enabled) {
        const unsigned char *lp = (const unsigned char*)s;
        g_loggy_gui_text_calls++;
        while (*lp) {
            unsigned int cp = font_utf8_next(&lp);
            if (font_codepoint_is_directional_control(cp)) continue;
            if (cp == 0xA7u && *lp) { (void)font_utf8_next(&lp); continue; }
            if (font_codepoint_width(cp, unicode_flag) > 0) g_loggy_gui_text_chars++;
        }
    }
    p = (const unsigned char*)s;
    while (*p) {
        unsigned int cp = font_utf8_next(&p);
        int fmt, glyph, adv_i;
        float adv = 0.0f;
        if (font_codepoint_is_directional_control(cp)) continue;
        if (cp == 0xA7u && *p) {
            fmt = font_format_index(font_utf8_next(&p));
            if (fmt >= 0 && fmt < 16) {
                obfuscated = bold = strike = underline = italic = 0;
                cur_color = (base_color & 0xFF000000u) | (unsigned int)font_color_code[fmt + (shadow_pass ? 16 : 0)];
            } else if (fmt == 16) obfuscated = 1;
            else if (fmt == 17) bold = 1;
            else if (fmt == 18) strike = 1;
            else if (fmt == 19) underline = 1;
            else if (fmt == 20) italic = 1;
            else if (fmt == 21) { obfuscated = bold = strike = underline = italic = 0; cur_color = base_color; }
            continue;
        }
        if (cp == 32u) { cx += 4.0f + (bold ? 1.0f : 0.0f); continue; }
        glyph = font_default_glyph_index(cp);
        adv_i = font_codepoint_width(cp, unicode_flag);
        if (adv_i <= 0) continue;
        if (obfuscated && glyph >= 0 && !unicode_flag) {
            /* Deterministic, cheap stand-in for Java's per-frame Random width-matched obfuscation. */
            int start = glyph;
            do { glyph = 32 + ((glyph * 1103515245u + 12345u) % 224u); }
            while (font_widths[glyph] != font_widths[start]);
        }
        if (glyph >= 0 && !unicode_flag) adv = draw_default_glyph_java(glyph, cx, (float)y, cur_color, italic);
        else adv = draw_unicode_glyph_java(cp, cx, (float)y, cur_color, italic);
        if (adv <= 0.0f && glyph >= 0) adv = draw_default_glyph_java(glyph, cx, (float)y, cur_color, italic);
        if (adv <= 0.0f) adv = (float)adv_i;
        if (bold) {
            if (glyph >= 0 && !unicode_flag) (void)draw_default_glyph_java(glyph, cx + 1.0f, (float)y, cur_color, italic);
            else (void)draw_unicode_glyph_java(cp, cx + 1.0f, (float)y, cur_color, italic);
            adv += 1.0f;
        }
        if (strike) draw_text_line(cx, cx + adv, (float)y + 4.0f, cur_color);
        if (underline) draw_text_line(cx, cx + adv, (float)y + 8.0f, cur_color);
        cx += adv;
    }
    glColor4f(1,1,1,1);
}

static void draw_text_raw(const char *s, int x, int y, int color) {
    draw_text_raw_impl(s, x, y, color, 0, 0);
}

static void draw_text(const char *s, int x, int y, int color) {
    draw_text_raw_impl(s, x + 1, y + 1, shadow_color(color), 0, 1);
    draw_text_raw_impl(s, x, y, color, 0, 0);
}

static void draw_text_no_shadow(const char *s, int x, int y, int color) {
    draw_text_raw_impl(s, x, y, color, 0, 0);
}

static void draw_text_force_bidi(const char *s, int x, int y, int color) {
    draw_text_raw_impl(s, x + 1, y + 1, shadow_color(color), 1, 1);
    draw_text_raw_impl(s, x, y, color, 1, 0);
}

static void draw_centered_text(const char *s, int cx, int y, int color) {
    draw_text(s, cx - text_width(s) / 2, y, color);
}

static void draw_centered_text_force_bidi(const char *s, int cx, int y, int color) {
    draw_text_force_bidi(s, cx - text_width(s) / 2, y, color);
}

static void button_label(Button *b, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(b->label, sizeof(b->label), fmt, ap);
    va_end(ap);
}

static Button *add_button_full(int id, int x, int y, int w, int h, const char *label, ButtonKind kind) {
    if (g_button_count >= MAX_BUTTONS) return NULL;
    Button *b = &g_buttons[g_button_count++];
    memset(b, 0, sizeof(*b));
    b->id = id;
    b->x = x; b->y = y; b->w = w; b->h = h;
    b->enabled = 1; b->visible = 1;
    b->kind = kind;
    b->opt = OPT_COUNT;
    snprintf(b->label, sizeof(b->label), "%s", (label && label[0]) ? tr(label) : "");
    return b;
}

static Button *add_button(int id, int x, int y, const char *label) {
    return add_button_full(id, x, y, 200, 20, label, BUTTON_NORMAL);
}

static int button_hover(Button *b, int mx, int my) {
    return b->visible && mx >= b->x && my >= b->y && mx < b->x + b->w && my < b->y + b->h;
}

static void draw_button(Button *b) {
    if (!b->visible) return;
    if (g_loggy_enabled) g_loggy_gui_buttons++;
    if (b->kind == BUTTON_HITBOX) return;
    int hover = button_hover(b, g_mouse_x, g_mouse_y);
    int state = 1;
    if (!b->enabled) state = 0;
    else if (hover) state = 2;
    if (b->kind == BUTTON_SLIDER) state = 0;
    draw_textured_modal_rect(&tex_gui, b->x, b->y, 0, 46 + state * 20, b->w / 2, b->h, 0xFFFFFF);
    draw_textured_modal_rect(&tex_gui, b->x + b->w / 2, b->y, 200 - b->w / 2, 46 + state * 20, b->w / 2, b->h, 0xFFFFFF);
    if (b->kind == BUTTON_SLIDER) {
        int knob = b->x + (int)(b->slider_value * (float)(b->w - 8));
        draw_textured_modal_rect(&tex_gui, knob, b->y, 0, 66, 4, 20, 0xFFFFFF);
        draw_textured_modal_rect(&tex_gui, knob + 4, b->y, 196, 66, 4, 20, 0xFFFFFF);
    }
    if (b->kind == BUTTON_LANGUAGE) {
        int lang_y = hover ? 126 : 106;
        draw_textured_modal_rect(&tex_gui, b->x, b->y, 0, lang_y, b->w, b->h, 0xFFFFFF);
        return;
    }
    int col;
    if (!b->enabled) col = -6250336;
    else if (hover) col = 16777120;
    else col = 14737632;
    draw_centered_text(b->label, b->x + b->w / 2, b->y + (b->h - 8) / 2, col);
}

static void clear_buttons(void) {
    memset(g_buttons, 0, sizeof(g_buttons));
    g_button_count = 0;
    g_drag_slider = NULL;
}
