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

static void draw_gradient(int x1, int y1, int x2, int y2, int c1, int c2) {
    float r1,g1,b1,a1,r2,g2,b2,a2;
    color_from_int_alpha(c1, &r1,&g1,&b1,&a1, 0);
    color_from_int_alpha(c2, &r2,&g2,&b2,&a2, 0);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#ifdef GL_SMOOTH
    glShadeModel(GL_SMOOTH);
#endif
    glBegin(GL_QUADS);
    glColor4f(r1,g1,b1,a1); glVertex3f((float)x2, (float)y1, 0.0f); glVertex3f((float)x1, (float)y1, 0.0f);
    glColor4f(r2,g2,b2,a2); glVertex3f((float)x1, (float)y2, 0.0f); glVertex3f((float)x2, (float)y2, 0.0f);
    glEnd();
#ifdef GL_FLAT
    glShadeModel(GL_FLAT);
#endif
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
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v1); glVertex3f((float)x, (float)(y + h), 0.0f);
    glTexCoord2f(u1, v1); glVertex3f((float)(x + w), (float)(y + h), 0.0f);
    glTexCoord2f(u1, v0); glVertex3f((float)(x + w), (float)y, 0.0f);
    glTexCoord2f(u0, v0); glVertex3f((float)x, (float)y, 0.0f);
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
    glBegin(GL_QUADS);
    glTexCoord2f((float)x1 / 32.0f, (float)(y2 + scroll) / 32.0f); glVertex3f((float)x1, (float)y2, 0.0f);
    glTexCoord2f((float)x2 / 32.0f, (float)(y2 + scroll) / 32.0f); glVertex3f((float)x2, (float)y2, 0.0f);
    glTexCoord2f((float)x2 / 32.0f, (float)(y1 + scroll) / 32.0f); glVertex3f((float)x2, (float)y1, 0.0f);
    glTexCoord2f((float)x1 / 32.0f, (float)(y1 + scroll) / 32.0f); glVertex3f((float)x1, (float)y1, 0.0f);
    glEnd();
    glColor4f(1,1,1,1);
}

static void draw_tiled_background_tint(int color, int scroll) {
    glBindTexture(GL_TEXTURE_2D, tex_bg.id);
    set_color_int(color);
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

static int font_index(unsigned char ch) {
    if (ch < 32) return -1;
    if (ch < 127) return ch - 32;
    return -1;
}

static void init_font_widths(void) {
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
                if (p[2] > 0 || p[3] > 0) empty = 0;
            }
            if (!empty) break;
        }
        if (i == 32) last = 2;
        font_widths[i] = last + 2;
    }
}

static int text_width(const char *s) {
    int w = 0;
    if (!s) return 0;
    for (const unsigned char *p = (const unsigned char*)s; *p; p++) {
        if (*p == 0xA7 && p[1]) { p++; continue; }
        int idx = font_index(*p);
        if (idx >= 0) w += font_widths[idx + 32];
    }
    return w;
}

static int shadow_color(int color) {
    unsigned int c = (unsigned int)color;
    unsigned int a = c & 0xFF000000u;
    return (int)(a + ((c & 0xFCFCFCu) >> 2));
}

static void draw_text_raw(const char *s, int x, int y, int color) {
    if (!s) return;
    glBindTexture(GL_TEXTURE_2D, tex_font.id);
    set_color_int(color);
    int cx = x;
    for (const unsigned char *p = (const unsigned char*)s; *p; p++) {
        if (*p == 0xA7 && p[1]) { p++; continue; }
        int idx = font_index(*p);
        if (idx >= 0) {
            int code = idx + 32;
            int sx = (code % 16) * 8;
            int sy = (code / 16) * 8;
            draw_textured_modal_rect(&tex_font, cx, y, sx, sy, 8, 8, color);
            cx += font_widths[code];
        }
    }
    glColor4f(1,1,1,1);
}

static void draw_text(const char *s, int x, int y, int color) {
    draw_text_raw(s, x + 1, y + 1, shadow_color(color));
    draw_text_raw(s, x, y, color);
}

static void draw_text_no_shadow(const char *s, int x, int y, int color) {
    draw_text_raw(s, x, y, color);
}

static void draw_centered_text(const char *s, int cx, int y, int color) {
    draw_text(s, cx - text_width(s) / 2, y, color);
}

static const char *tr(const char *en) {
    return en ? en : "";
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
    snprintf(b->label, sizeof(b->label), "%s", label ? label : "");
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
