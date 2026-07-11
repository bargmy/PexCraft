/* Split from original monolithic main.c. Included by src/main.c unity build. */

static int read_u32_le(FILE *f) {
    unsigned char b[4];
    if (fread(b, 1, 4, f) != 4) return 0;
    return (int)(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
}

static int read_u32_le_mem(const unsigned char *p) {
    return (int)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

static int file_exists(const char *path) {
    DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
/* Generated in GitHub Actions as a platform-specific mcrw assets pak
   and embedded into the console executable through an .incbin assembly wrapper.
   This avoids requiring texture files on external storage. */
#if defined(PEX_PLATFORM_PSP)
extern const unsigned char pexcraft_psp_mcrw_assets_pak_start[];
extern const unsigned char pexcraft_psp_mcrw_assets_pak_end[];
#elif defined(PEX_PLATFORM_WII)
extern const unsigned char pexcraft_wii_mcrw_assets_pak_start[];
extern const unsigned char pexcraft_wii_mcrw_assets_pak_end[];
#ifndef PEX_PSP_LOGF
#define PEX_PSP_LOGF(...) do { } while (0)
#endif
#endif
#endif

static void free_texture(Texture *t) {
    if (!t) return;
    if (t->id) glDeleteTextures(1, &t->id);
    if (t->rgba) free(t->rgba);
    t->id = 0;
    t->w = 0;
    t->h = 0;
    t->rgba = NULL;
}

static void stivufine_clear_random_mob_variants(void);
static int stivufine_mipmaps_suppressed = 0;
static int stivufine_terrain_tiles_per_axis = 16;
static int stivufine_defer_quality_update = 0;
static void stivufine_apply_anisotropy_bound(void);
static int load_png_texture(Texture *t, const char *path, int repeat);
static char *stivufine_trim_ascii(char *s);

static int stivufine_texture_is_font(const Texture *t) {
    if (!t) return 0;
    if (t == &tex_font) return 1;
    for (int i = 0; i < 256; ++i) if (t == &tex_font_glyph[i]) return 1;
    return 0;
}


/* UI atlases must remain point-filtered and mipmap-free.  A high-resolution
   resource-pack GUI is already downsampled deliberately by the fixed logical
   256x256 atlas mapping in gui_primitives.c.  Letting the general texture
   mipmap path process it blends neighbouring button states, slots and icons,
   producing the distinctly non-Minecraft-looking soft/stretchy UI. */
static int stivufine_texture_is_gui(const Texture *t) {
    if (!t) return 0;
    return t == &tex_bg || t == &tex_gui || t == &tex_icons ||
           t == &tex_inventory || t == &tex_allitems ||
           t == &tex_workbench || t == &tex_furnace_gui ||
           t == &tex_chest_gui || t == &tex_items ||
           t == &tex_achievement || t == &tex_stat_slot ||
           t == &tex_title_logo || t == &tex_pack ||
           t == &tex_default_pack_icon || t == &tex_unknown_pack;
}

static void set_texture_filter_wrap(Texture *t, int linear, int repeat) {
    if (!t || !t->id) return;
    glBindTexture(GL_TEXTURE_2D, t->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : PEX_GL_CLAMP_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : PEX_GL_CLAMP_EDGE);
    stivufine_apply_anisotropy_bound();
}

static int stivufine_texture_mipmap_level_for(const Texture *t) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII) || defined(PEX_PLATFORM_ANDROID) || defined(PEX_PLATFORM_ANDROID_TV) || defined(PEX_PLATFORM_LGWEBOS)
    (void)t;
    return 0;
#else
    if (!t || stivufine_mipmaps_suppressed || stivufine_texture_is_font(t) || stivufine_texture_is_gui(t)) return 0;
#if !defined(PEX_PLATFORM_SDL2)
    if (pex_using_gpu_backend()) return 0;
#endif
    int level = g_opts.sf_mipmap_level;
    if (level <= 0 || !t->rgba || t->w <= 1 || t->h <= 1) return 0;
    int max_level = 0;
    if (t == &tex_terrain) {
        int grid = stivufine_terrain_tiles_per_axis > 0 ? stivufine_terrain_tiles_per_axis : 16;
        int tile_w = t->w / grid;
        int tile_h = t->h / grid;
        if (tile_w <= 1 || tile_h <= 1 || tile_w * grid != t->w || tile_h * grid != t->h) return 0;
        int min_tile = tile_w < tile_h ? tile_w : tile_h;
        while (min_tile > 1 && max_level < 12) { min_tile >>= 1; max_level++; }
    } else {
        int min_dim = t->w < t->h ? t->w : t->h;
        while (min_dim > 1 && max_level < 12) { min_dim >>= 1; max_level++; }
        if (level >= 4 && max_level > 4) max_level -= 4; /* Match Java's Max headroom for non-atlas textures. */
    }
    if (level >= 4) level = max_level;
    if (level > max_level) level = max_level;
    if (level < 0) level = 0;
    return level;
#endif
}

static void stivufine_alpha_blend_pair(const unsigned char *a, const unsigned char *b, unsigned char out[4]) {
    int a1 = a[3], a2 = b[3];
    int ax = (a1 + a2) / 2;
    const unsigned char *c1 = a, *c2 = b;
    if (a1 == 0 && a2 == 0) {
        a1 = 1;
        a2 = 1;
    } else {
        if (a1 == 0) { c1 = b; ax /= 2; }
        if (a2 == 0) { c2 = c1; ax /= 2; }
    }
    out[0] = (unsigned char)(((int)c1[0] * a1 + (int)c2[0] * a2) / (a1 + a2));
    out[1] = (unsigned char)(((int)c1[1] * a1 + (int)c2[1] * a2) / (a1 + a2));
    out[2] = (unsigned char)(((int)c1[2] * a1 + (int)c2[2] * a2) / (a1 + a2));
    out[3] = (unsigned char)ax;
}

/* Exact RenderEngine.alphaBlend(c1,c2,c3,c4) pairing/order from the
   Minecraft 1.2.5 StivuFine reference: blend top pair, bottom pair, then both. */
static void stivufine_alpha_blend_four(const unsigned char *p1, const unsigned char *p2,
                                       const unsigned char *p3, const unsigned char *p4,
                                       unsigned char out[4]) {
    unsigned char a[4], b[4];
    stivufine_alpha_blend_pair(p1, p2, a);
    stivufine_alpha_blend_pair(p3, p4, b);
    stivufine_alpha_blend_pair(a, b, out);
}

static void stivufine_prepare_terrain_transparent_rgb(unsigned char *rgba, int w, int h) {
    if (!rgba || w <= 0 || h <= 0 || (w % 16) || (h % 16)) return;
    int grid = stivufine_terrain_tiles_per_axis > 0 ? stivufine_terrain_tiles_per_axis : 16;
    int tw = w / grid, th = h / grid;
    for (int ty = 0; ty < grid; ++ty) for (int tx = 0; tx < grid; ++tx) {
        unsigned long long rs = 0, gs = 0, bs = 0, count = 0;
        int x0 = tx * tw, y0 = ty * th;
        for (int y = 0; y < th; ++y) for (int x = 0; x < tw; ++x) {
            unsigned char *px = &rgba[((y0 + y) * w + (x0 + x)) * 4];
            if (px[3] != 0) { rs += px[0]; gs += px[1]; bs += px[2]; count++; }
        }
        if (!count) continue;
        unsigned char r = (unsigned char)(rs / count);
        unsigned char g = (unsigned char)(gs / count);
        unsigned char b = (unsigned char)(bs / count);
        for (int y = 0; y < th; ++y) for (int x = 0; x < tw; ++x) {
            unsigned char *px = &rgba[((y0 + y) * w + (x0 + x)) * 4];
            if (px[3] == 0) { px[0] = r; px[1] = g; px[2] = b; }
        }
    }
}

static void stivufine_upload_atlas_mipmap_levels(Texture *t) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    (void)t;
#else
    int levels = stivufine_texture_mipmap_level_for(t);
    if (!t || !t->rgba || !t->id || levels <= 0) return;
    int grid = stivufine_terrain_tiles_per_axis > 0 ? stivufine_terrain_tiles_per_axis : 16;
    int base_tile_w = t->w / grid;
    int base_tile_h = t->h / grid;
    int prev_w = t->w;
    unsigned char *prev = (unsigned char*)malloc((size_t)t->w * (size_t)t->h * 4u);
    if (!prev) return;
    memcpy(prev, t->rgba, (size_t)t->w * (size_t)t->h * 4u);
    stivufine_prepare_terrain_transparent_rgb(prev, t->w, t->h);
    for (int level = 1; level <= levels; ++level) {
        int src_tile_w = base_tile_w >> (level - 1);
        int src_tile_h = base_tile_h >> (level - 1);
        int dst_tile_w = base_tile_w >> level;
        int dst_tile_h = base_tile_h >> level;
        if (src_tile_w < 1) src_tile_w = 1;
        if (src_tile_h < 1) src_tile_h = 1;
        if (dst_tile_w < 1) dst_tile_w = 1;
        if (dst_tile_h < 1) dst_tile_h = 1;
        int dst_w = dst_tile_w * grid;
        int dst_h = dst_tile_h * grid;
        unsigned char *dst = (unsigned char*)calloc((size_t)dst_w * (size_t)dst_h * 4u, 1u);
        if (!dst) break;
        for (int ty = 0; ty < grid; ++ty) for (int tx = 0; tx < grid; ++tx) {
            int src0x = tx * src_tile_w, src0y = ty * src_tile_h;
            int dst0x = tx * dst_tile_w, dst0y = ty * dst_tile_h;
            for (int y = 0; y < dst_tile_h; ++y) for (int x = 0; x < dst_tile_w; ++x) {
                int sx0 = x * 2, sy0 = y * 2;
                int sx1 = sx0 + 1 < src_tile_w ? sx0 + 1 : sx0;
                int sy1 = sy0 + 1 < src_tile_h ? sy0 + 1 : sy0;
                const unsigned char *p1 = &prev[((src0y + sy0) * prev_w + (src0x + sx0)) * 4];
                const unsigned char *p2 = &prev[((src0y + sy0) * prev_w + (src0x + sx1)) * 4];
                const unsigned char *p3 = &prev[((src0y + sy1) * prev_w + (src0x + sx1)) * 4];
                const unsigned char *p4 = &prev[((src0y + sy1) * prev_w + (src0x + sx0)) * 4];
                unsigned char *dp = &dst[((dst0y + y) * dst_w + (dst0x + x)) * 4];
                stivufine_alpha_blend_four(p1, p2, p3, p4, dp);
            }
        }
        glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, dst_w, dst_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dst);
        free(prev);
        prev = dst;
        prev_w = dst_w;
    }
    free(prev);
#endif
}

static void stivufine_upload_generic_mipmap_levels(Texture *t) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII) || defined(PEX_PLATFORM_ANDROID) || defined(PEX_PLATFORM_ANDROID_TV) || defined(PEX_PLATFORM_LGWEBOS)
    (void)t;
#else
    int levels = stivufine_texture_mipmap_level_for(t);
    if (!t || !t->rgba || !t->id || levels <= 0) return;
    int prev_w = t->w;
    int prev_h = t->h;
    unsigned char *prev = (unsigned char*)malloc((size_t)prev_w * (size_t)prev_h * 4u);
    if (!prev) return;
    memcpy(prev, t->rgba, (size_t)prev_w * (size_t)prev_h * 4u);
    for (int level = 1; level <= levels; ++level) {
        int dst_w = prev_w > 1 ? (prev_w >> 1) : 1;
        int dst_h = prev_h > 1 ? (prev_h >> 1) : 1;
        unsigned char *dst = (unsigned char*)calloc((size_t)dst_w * (size_t)dst_h * 4u, 1u);
        if (!dst) break;
        for (int y = 0; y < dst_h; ++y) for (int x = 0; x < dst_w; ++x) {
            int sx0 = x * 2, sy0 = y * 2;
            int sx1 = sx0 + 1 < prev_w ? sx0 + 1 : sx0;
            int sy1 = sy0 + 1 < prev_h ? sy0 + 1 : sy0;
            const unsigned char *p1 = &prev[(sy0 * prev_w + sx0) * 4];
            const unsigned char *p2 = &prev[(sy0 * prev_w + sx1) * 4];
            const unsigned char *p3 = &prev[(sy1 * prev_w + sx1) * 4];
            const unsigned char *p4 = &prev[(sy1 * prev_w + sx0) * 4];
            unsigned char *dp = &dst[(y * dst_w + x) * 4];
            stivufine_alpha_blend_four(p1, p2, p3, p4, dp);
        }
        glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, dst_w, dst_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dst);
        free(prev);
        prev = dst;
        prev_w = dst_w;
        prev_h = dst_h;
        if (dst_w <= 1 && dst_h <= 1) break;
    }
    free(prev);
#endif
}

static void stivufine_apply_texture_mipmap_params(Texture *t) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    (void)t;
#else
    if (!t || !t->id) return;
    int levels = stivufine_texture_mipmap_level_for(t);
    glBindTexture(GL_TEXTURE_2D, t->id);
    if (levels > 0) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, levels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, g_opts.sf_mipmap_linear ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        stivufine_apply_anisotropy_bound();
        if (t == &tex_terrain) stivufine_upload_atlas_mipmap_levels(t);
        else stivufine_upload_generic_mipmap_levels(t);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        stivufine_apply_anisotropy_bound();
    }
#endif
}

static void stivufine_rebuild_terrain_mipmaps(void) {
    if (!tex_terrain.id || !tex_terrain.rgba) return;
    stivufine_apply_texture_mipmap_params(&tex_terrain);
}

static int upload_rgba_texture(Texture *t, int w, int h, unsigned char *rgba, int repeat);

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#define SF_CTM_MAX_RULES 768
#define SF_CTM_MAX_SOURCES 15
#define SF_CTM_MAX_TILES 48

typedef struct StivuFineCtmRule {
    int target_kind; /* 1 block id, 2 terrain tile */
    int target;
    int method;      /* 1 ctm, 2 horizontal, 3 top, 4 random, 5 repeat, 6 vertical */
    int source_slot;
    int connect;     /* 1 block, 2 tile */
    int faces;
    int metadata[16], metadata_count;
    int tiles[SF_CTM_MAX_TILES], tile_count;
    int weights[SF_CTM_MAX_TILES], weight_count, weight_sum;
    int symmetry;
    int width, height;
} StivuFineCtmRule;

typedef struct StivuFineCtmSource {
    char rel[MAX_PATHBUF];
    Texture texture;
    int matched_base_dimensions;
} StivuFineCtmSource;

static StivuFineCtmRule stivufine_ctm_rules[SF_CTM_MAX_RULES];
static int stivufine_ctm_rule_count = 0;
static StivuFineCtmSource stivufine_ctm_sources[SF_CTM_MAX_SOURCES];
static int stivufine_ctm_source_count = 0;
static int stivufine_terrain_cells_per_row = 1;
static int stivufine_terrain_base_w = 256;
static int stivufine_terrain_base_h = 256;
static unsigned char stivufine_natural_rotation[SF_CTM_MAX_SOURCES + 1][256];
static unsigned char stivufine_natural_flip[SF_CTM_MAX_SOURCES + 1][256];
static int stivufine_anisotropy_checked = 0;
static float stivufine_anisotropy_max = 1.0f;

static int stivufine_int_hash(int x) {
    uint32_t u = (uint32_t)x;
    u = u ^ 61u ^ (u >> 16);
    u += u << 3;
    u ^= u >> 4;
    u *= 668265261u;
    u ^= u >> 15;
    return (int)u;
}

static int stivufine_coord_random(int x, int y, int z, int face) {
    int r = stivufine_int_hash(face + 37);
    r = stivufine_int_hash(r + x);
    r = stivufine_int_hash(r + z);
    return stivufine_int_hash(r + y);
}

static void stivufine_detect_anisotropy(void) {
    if (stivufine_anisotropy_checked) return;
    stivufine_anisotropy_checked = 1;
    stivufine_anisotropy_max = 1.0f;
#if !defined(PEX_PLATFORM_PSP) && !defined(PEX_PLATFORM_WII)
    const char *ext = (const char*)glGetString(GL_EXTENSIONS);
    if (ext && strstr(ext, "GL_EXT_texture_filter_anisotropic")) {
        GLfloat maxv = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxv);
        if (maxv > 1.0f) stivufine_anisotropy_max = maxv;
    }
#endif
}

static void stivufine_apply_anisotropy_bound(void) {
#if !defined(PEX_PLATFORM_PSP) && !defined(PEX_PLATFORM_WII)
    stivufine_detect_anisotropy();
    float want = (float)stivufine_af_level();
    if (want < 1.0f) want = 1.0f;
    if (want > stivufine_anisotropy_max) want = stivufine_anisotropy_max;
    if (stivufine_anisotropy_max > 1.0f) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, want);
#endif
}

static void stivufine_reset_quality_texture_state(void) {
    for (int i = 0; i < stivufine_ctm_source_count; ++i) free_texture(&stivufine_ctm_sources[i].texture);
    memset(stivufine_ctm_sources, 0, sizeof(stivufine_ctm_sources));
    memset(stivufine_ctm_rules, 0, sizeof(stivufine_ctm_rules));
    memset(stivufine_natural_rotation, 0, sizeof(stivufine_natural_rotation));
    memset(stivufine_natural_flip, 0, sizeof(stivufine_natural_flip));
    stivufine_ctm_source_count = 0;
    stivufine_ctm_rule_count = 0;
    stivufine_terrain_cells_per_row = 1;
    stivufine_terrain_tiles_per_axis = 16;
    stivufine_terrain_base_w = tex_terrain.w > 0 ? tex_terrain.w : 256;
    stivufine_terrain_base_h = tex_terrain.h > 0 ? tex_terrain.h : 256;
}

static int stivufine_parse_int_list(const char *text, int *out, int cap) {
    int n = 0;
    char buf[512];
    if (!text || !out || cap <= 0) return 0;
    snprintf(buf, sizeof(buf), "%s", text);
    for (char *tok = strtok(buf, " ,\t"); tok && n < cap; tok = strtok(NULL, " ,\t")) {
        char *dash = strchr(tok, '-');
        if (dash) {
            *dash++ = 0;
            int a = atoi(tok), b = atoi(dash);
            if (a >= 0 && b >= a) for (int v = a; v <= b && n < cap; ++v) out[n++] = v;
        } else {
            int v = atoi(tok);
            if (v >= 0) out[n++] = v;
        }
    }
    return n;
}

static int stivufine_parse_faces(const char *text) {
    if (!text || !*text) return 63;
    int mask = 0;
    char buf[256]; snprintf(buf, sizeof(buf), "%s", text);
    for (char *tok = strtok(buf, " ,\t"); tok; tok = strtok(NULL, " ,\t")) {
        if (!strcmp(tok,"bottom")) mask |= 1;
        else if (!strcmp(tok,"top")) mask |= 2;
        else if (!strcmp(tok,"north")) mask |= 4;
        else if (!strcmp(tok,"south")) mask |= 8;
        else if (!strcmp(tok,"west")) mask |= 16;
        else if (!strcmp(tok,"east")) mask |= 32;
        else if (!strcmp(tok,"sides")) mask |= 60;
        else if (!strcmp(tok,"all")) mask |= 63;
    }
    return mask;
}

static int stivufine_load_pack_or_builtin(TexturePackEntry *e, Texture *out, const char *rel) {
    char path[MAX_PATHBUF];
    if (e && !e->is_default) {
        snprintf(path, sizeof(path), "%s/%s", e->path, rel[0]=='/' ? rel+1 : rel);
        for (char *c=path; *c; ++c) if (*c=='\\') *c='/';
        if (load_png_texture(out, path, 0)) return 1;
    }
    if (!strcmp(rel, "/ctm.png") || !strcmp(rel, "ctm.png")) {
        if (load_png_texture(out, "stivufine/ctm.png", 0)) return 1;
        snprintf(path, sizeof(path), "%s/stivufine/ctm.png", g_mc_dir);
        if (load_png_texture(out, path, 0)) return 1;
    }
    return 0;
}

static int stivufine_resize_texture_rgba(Texture *t, int w, int h) {
    if (!t || !t->rgba || t->w <= 0 || t->h <= 0 || w <= 0 || h <= 0) return 0;
    if (t->w == w && t->h == h) return 1;
    unsigned char *dst = (unsigned char*)malloc((size_t)w * (size_t)h * 4u);
    if (!dst) return 0;
    for (int y = 0; y < h; ++y) {
        int sy = (int)((long long)y * t->h / h);
        if (sy >= t->h) sy = t->h - 1;
        for (int x = 0; x < w; ++x) {
            int sx = (int)((long long)x * t->w / w);
            if (sx >= t->w) sx = t->w - 1;
            memcpy(&dst[((size_t)y * w + x) * 4u], &t->rgba[((size_t)sy * t->w + sx) * 4u], 4u);
        }
    }
    if (t->id) glDeleteTextures(1, &t->id);
    free(t->rgba);
    t->rgba = dst; t->w = w; t->h = h; t->id = 0;
    return 1;
}

static int stivufine_ctm_source_slot(TexturePackEntry *e, const char *source) {
    char norm[MAX_PATHBUF];
    if (!source || !*source) source = "/ctm.png";
    snprintf(norm, sizeof(norm), "%s", source);
    for (char *c=norm; *c; ++c) if (*c=='\\') *c='/';
    if (norm[0] != '/') { char tmp[MAX_PATHBUF]; snprintf(tmp,sizeof(tmp),"/%s",norm); snprintf(norm,sizeof(norm),"%s",tmp); }
    for (int i=0;i<stivufine_ctm_source_count;++i) if (!strcmp(stivufine_ctm_sources[i].rel,norm)) return i+1;
    if (stivufine_ctm_source_count >= SF_CTM_MAX_SOURCES) return -1;
    Texture t={0};
    if (!stivufine_load_pack_or_builtin(e,&t,norm)) return -1;
    int matched_base = (t.w == stivufine_terrain_base_w && t.h == stivufine_terrain_base_h);
    if (!matched_base && !stivufine_resize_texture_rgba(&t, stivufine_terrain_base_w, stivufine_terrain_base_h)) {
        free_texture(&t); return -1;
    }
    int i=stivufine_ctm_source_count++;
    snprintf(stivufine_ctm_sources[i].rel,sizeof(stivufine_ctm_sources[i].rel),"%s",norm);
    stivufine_ctm_sources[i].texture=t;
    stivufine_ctm_sources[i].matched_base_dimensions=matched_base;
    return i+1;
}

static void stivufine_natural_defaults(void) {
    static const struct { unsigned char tile, rot, flip; } d[] = {
        {0,4,1},{1,2,1},{2,4,1},{3,1,1},{38,1,1},{6,1,1},{17,2,1},{18,4,1},{19,4,0},
        {20,2,1},{21,4,1},{32,2,1},{33,2,1},{34,2,1},{50,2,1},{51,2,1},{160,2,1},
        {37,4,1},{52,2,1},{53,2,1},{196,2,0},{197,2,0},{66,4,1},{68,1,1},{70,2,1},
        {72,4,1},{77,1,1},{78,4,1},{86,2,1},{87,2,1},{103,4,1},{104,4,1},{105,4,0},
        {116,2,1},{117,1,1},{132,2,1},{133,2,1},{153,2,1},{175,4,0},{176,4,0},
        {208,4,1},{211,4,1},{212,4,1}
    };
    for (size_t i=0;i<sizeof(d)/sizeof(d[0]);++i) { stivufine_natural_rotation[0][d[i].tile]=d[i].rot; stivufine_natural_flip[0][d[i].tile]=d[i].flip; }
}

static int stivufine_natural_texture_slot(const char *name) {
    char norm[MAX_PATHBUF];
    if (!name || !*name) return -1;
    snprintf(norm, sizeof(norm), "%s", name);
    for (char *c = norm; *c; ++c) if (*c == '\\') *c = '/';
    if (norm[0] != '/') { char tmp[MAX_PATHBUF]; snprintf(tmp, sizeof(tmp), "/%s", norm); snprintf(norm, sizeof(norm), "%s", tmp); }
    if (!strcmp(norm, "/terrain.png")) return 0;
    for (int i = 0; i < stivufine_ctm_source_count; ++i)
        if (!strcmp(norm, stivufine_ctm_sources[i].rel)) return i + 1;
    return -1;
}

static void stivufine_load_natural_properties(TexturePackEntry *e) {
    if (!stivufine_natural_textures_enabled()) return;
    if (!e || e->is_default) { stivufine_natural_defaults(); return; }
    char path[MAX_PATHBUF]; snprintf(path,sizeof(path),"%s/natural.properties",e->path);
    FILE *f=fopen(path,"r");
    if (!f) return; /* exact 1.2.5 behavior: custom packs do not inherit defaults */
    char line[512];
    while (fgets(line,sizeof(line),f)) {
        char *hash=strchr(line,'#'); if(hash)*hash=0;
        char *eq=strchr(line,'='); if(!eq)continue; *eq++=0;
        char *lhs=stivufine_trim_ascii(line), *type=stivufine_trim_ascii(eq);
        char *colon=strrchr(lhs,':'); if(!colon)continue; *colon++=0;
        int slot=stivufine_natural_texture_slot(lhs); if(slot<0||slot>SF_CTM_MAX_SOURCES)continue;
        int tile=atoi(colon); if(tile<0||tile>255)continue;
        int rot=1, flip=0;
        if(!strcmp(type,"4")){rot=4;} else if(!strcmp(type,"2")){rot=2;} else if(!strcmp(type,"F")){flip=1;}
        else if(!strcmp(type,"4F")){rot=4;flip=1;} else if(!strcmp(type,"2F")){rot=2;flip=1;} else continue;
        stivufine_natural_rotation[slot][tile]=(unsigned char)rot; stivufine_natural_flip[slot][tile]=(unsigned char)flip;
    }
    fclose(f);
}

static int stivufine_ctm_method(const char *v) {
    if(!v||!*v||!strcmp(v,"ctm"))return 1; if(!strcmp(v,"horizontal"))return 2; if(!strcmp(v,"top"))return 3;
    if(!strcmp(v,"random"))return 4; if(!strcmp(v,"repeat"))return 5; if(!strcmp(v,"vertical"))return 6; return 0;
}

static int stivufine_load_ctm_rule_file(TexturePackEntry *e, const char *path, int target_kind, int target, int def_connect) {
    FILE *f=fopen(path,"r"); if(!f||stivufine_ctm_rule_count>=SF_CTM_MAX_RULES){if(f)fclose(f);return 0;}
    char source[MAX_PATHBUF]="", method[32]="ctm", tiles[512]="", connect[32]="", faces[128]="", metadata[128]="", weights[512]="", symmetry[32]="", width[32]="", height[32]="";
    char line[768];
    while(fgets(line,sizeof(line),f)){
        char *hash=strchr(line,'#');if(hash)*hash=0; char *eq=strchr(line,'=');if(!eq)continue;*eq++=0;
        char *k=stivufine_trim_ascii(line),*v=stivufine_trim_ascii(eq);
        char *dst=NULL; size_t cap=0;
        if(!strcmp(k,"source")){dst=source;cap=sizeof(source);}else if(!strcmp(k,"method")){dst=method;cap=sizeof(method);}
        else if(!strcmp(k,"tiles")){dst=tiles;cap=sizeof(tiles);}else if(!strcmp(k,"connect")){dst=connect;cap=sizeof(connect);}
        else if(!strcmp(k,"faces")){dst=faces;cap=sizeof(faces);}else if(!strcmp(k,"metadata")){dst=metadata;cap=sizeof(metadata);}
        else if(!strcmp(k,"weights")){dst=weights;cap=sizeof(weights);}else if(!strcmp(k,"symmetry")){dst=symmetry;cap=sizeof(symmetry);}
        else if(!strcmp(k,"width")){dst=width;cap=sizeof(width);}else if(!strcmp(k,"height")){dst=height;cap=sizeof(height);}
        if(dst)snprintf(dst,cap,"%s",v);
    } fclose(f);
    StivuFineCtmRule r; memset(&r,0,sizeof(r)); r.target_kind=target_kind;r.target=target;r.method=stivufine_ctm_method(method);r.connect=def_connect;r.faces=stivufine_parse_faces(faces);r.symmetry=1;
    if(!strcmp(connect,"block"))r.connect=1;else if(!strcmp(connect,"tile"))r.connect=2;
    if(!strcmp(symmetry,"opposite"))r.symmetry=2;else if(!strcmp(symmetry,"all"))r.symmetry=6;
    r.width=atoi(width);r.height=atoi(height);r.metadata_count=stivufine_parse_int_list(metadata,r.metadata,16);
    r.tile_count=stivufine_parse_int_list(tiles,r.tiles,SF_CTM_MAX_TILES);r.weight_count=stivufine_parse_int_list(weights,r.weights,SF_CTM_MAX_TILES);
    if(r.method==1&&r.tile_count==0)r.tile_count=stivufine_parse_int_list("0-11 16-27 32-43 48-59",r.tiles,SF_CTM_MAX_TILES);
    if(r.method==2&&r.tile_count==0)r.tile_count=stivufine_parse_int_list("12-15",r.tiles,SF_CTM_MAX_TILES);
    if(r.method==3&&r.tile_count==0){r.tiles[0]=66;r.tile_count=1;}
    if(r.method==1&&r.tile_count!=48)return 0;if((r.method==2||r.method==6)&&r.tile_count!=4)return 0;if(r.method==3&&r.tile_count!=1)return 0;
    if(r.method==5&&(r.width<1||r.width>16||r.height<1||r.height>16||r.width*r.height!=r.tile_count))return 0;if(r.method==4&&r.tile_count<1)return 0;
    if(r.weight_count==r.tile_count){for(int i=0;i<r.weight_count;++i){if(r.weights[i]<0)return 0;r.weight_sum+=r.weights[i];}if(r.weight_sum<=0)return 0;}else if(r.weight_count>0)return 0;
    if(!source[0])return 0;
    r.source_slot=stivufine_ctm_source_slot(e,source); if(r.source_slot<1)return 0;
    stivufine_ctm_rules[stivufine_ctm_rule_count++]=r; return 1;
}

static void stivufine_add_default_ctm_rule(int kind,int target,int method,int source_slot) {
    if(stivufine_ctm_rule_count>=SF_CTM_MAX_RULES)return; StivuFineCtmRule r;memset(&r,0,sizeof(r));
    r.target_kind=kind;r.target=target;r.method=method;r.source_slot=source_slot;r.connect=kind==1?1:2;r.faces=63;r.symmetry=1;
    if(method==1)r.tile_count=stivufine_parse_int_list("0-11 16-27 32-43 48-59",r.tiles,SF_CTM_MAX_TILES);
    else if(method==2)r.tile_count=stivufine_parse_int_list("12-15",r.tiles,SF_CTM_MAX_TILES);
    else {r.tiles[0]=66;r.tile_count=1;}
    stivufine_ctm_rules[stivufine_ctm_rule_count++]=r;
}

static void stivufine_load_connected_properties(TexturePackEntry *e) {
    if(stivufine_connected_textures_mode()==SF_OFF)return;
    int before=stivufine_ctm_rule_count;
    if(e&&!e->is_default){
        static const char suff[]=" abcdefghijklmnopqrstuvwxyz"; char path[MAX_PATHBUF];
        /* The 1.2.5 resolver checks terrain/tile properties before block
           properties. Preserve that priority when both match the same face. */
        static const int kinds[] = {2, 1};
        for (int ki = 0; ki < 2; ++ki) {
            int kind = kinds[ki];
            for(int n=0;n<256;++n)for(int si=0;si<27;++si){
                char suffix[2]={0,0};if(si>0)suffix[0]=suff[si];
                snprintf(path,sizeof(path),"%s/ctm/%s%d%s.properties",e->path,kind==1?"block":"terrain",n,suffix);
                if(!file_exists(path))break;
                stivufine_load_ctm_rule_file(e,path,kind,n,kind==1?1:2);
            }
        }
    }
    if(stivufine_ctm_rule_count==before){
        int slot=stivufine_ctm_source_slot(e,"/ctm.png");
        if(slot>0 && stivufine_ctm_sources[slot-1].matched_base_dimensions){
            stivufine_add_default_ctm_rule(1,BLOCK_GLASS,1,slot);
            stivufine_add_default_ctm_rule(1,BLOCK_BOOKSHELF,2,slot);
            stivufine_add_default_ctm_rule(2,176,3,slot); /* sandstone top */
        }
    }
}

static void stivufine_build_combined_terrain_atlas(void) {
    int sources=1+stivufine_ctm_source_count;if(sources<=1||!tex_terrain.rgba)return;
    int cells=1;while(cells*cells<sources)cells<<=1;if(cells>4)cells=4;
    int w=stivufine_terrain_base_w*cells,h=stivufine_terrain_base_h*cells;
    unsigned char *rgba=(unsigned char*)calloc((size_t)w*(size_t)h*4u,1u);if(!rgba)return;
    for(int slot=0;slot<sources&&slot<cells*cells;++slot){
        Texture *src=slot==0?&tex_terrain:&stivufine_ctm_sources[slot-1].texture;
        int cx=slot%cells,cy=slot/cells;
        for(int y=0;y<stivufine_terrain_base_h;++y)memcpy(&rgba[((cy*stivufine_terrain_base_h+y)*w+cx*stivufine_terrain_base_w)*4],&src->rgba[(size_t)y*src->w*4],(size_t)stivufine_terrain_base_w*4);
    }
    stivufine_terrain_cells_per_row=cells;stivufine_terrain_tiles_per_axis=16*cells;
    upload_rgba_texture(&tex_terrain,w,h,rgba,1);
    /* CTM pixels now live in the combined terrain atlas. Keep only rule/source
       metadata so reloads do not retain duplicate GPU/CPU texture copies. */
    for (int i = 0; i < stivufine_ctm_source_count; ++i) free_texture(&stivufine_ctm_sources[i].texture);
}

static void stivufine_update_quality_textures(TexturePackEntry *e) {
    stivufine_reset_quality_texture_state();
    stivufine_terrain_base_w=tex_terrain.w;stivufine_terrain_base_h=tex_terrain.h;
    stivufine_load_connected_properties(e);
    stivufine_load_natural_properties(e);
    stivufine_build_combined_terrain_atlas();
}

static int upload_rgba_texture(Texture *t, int w, int h, unsigned char *rgba, int repeat) {
    if (!t || !rgba || w <= 0 || h <= 0) return 0;
    free_texture(t);
    t->w = w;
    t->h = h;
    t->rgba = rgba;
    glGenTextures(1, &t->id);
    glBindTexture(GL_TEXTURE_2D, t->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : PEX_GL_CLAMP_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : PEX_GL_CLAMP_EDGE);
    stivufine_apply_anisotropy_bound();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->rgba);
    stivufine_apply_texture_mipmap_params(t);
    return t->id != 0;
}


static int pex_make_fallback_texture(Texture *t, const char *name, int w, int h, int repeat) {
    if (w <= 0) w = 16;
    if (h <= 0) h = 16;
    size_t n = (size_t)w * (size_t)h * 4u;
    unsigned char *rgba = (unsigned char*)malloc(n);
    if (!rgba) return 0;
    unsigned int seed = 2166136261u;
    if (name) {
        const unsigned char *p = (const unsigned char*)name;
        while (*p) seed = (seed ^ *p++) * 16777619u;
    }
    unsigned char r = (unsigned char)(80u + (seed & 0x7fu));
    unsigned char g = (unsigned char)(80u + ((seed >> 8) & 0x7fu));
    unsigned char b = (unsigned char)(80u + ((seed >> 16) & 0x7fu));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int k = (y * w + x) * 4;
            int checker = ((x >> 3) ^ (y >> 3)) & 1;
            rgba[k + 0] = checker ? r : (unsigned char)(r / 2);
            rgba[k + 1] = checker ? g : (unsigned char)(g / 2);
            rgba[k + 2] = checker ? b : (unsigned char)(b / 2);
            rgba[k + 3] = 255;
        }
    }
    return upload_rgba_texture(t, w, h, rgba, repeat);
}

static void normalize_sky_alpha(Texture *t) {
    if (!t || !t->rgba || !t->id || t->w <= 0 || t->h <= 0) return;
    size_t pixels = (size_t)t->w * (size_t)t->h;
    if (pixels == 0 || pixels > (size_t)4096 * (size_t)4096) return;

    size_t opaque = 0, transparent = 0, dark_opaque = 0, nonblack = 0;
    for (size_t i = 0; i < pixels; ++i) {
        unsigned char *px = &t->rgba[i * 4u];
        if (px[3] >= 250) opaque++;
        if (px[3] <= 5) transparent++;
        if (px[3] >= 250 && px[0] < 12 && px[1] < 12 && px[2] < 12) dark_opaque++;
        if (px[0] >= 12 || px[1] >= 12 || px[2] >= 12) nonblack++;
    }

    /* Java Release 1.2.5 renders /terrain/sun.png and /terrain/moon_phases.png
       with GL_SRC_ALPHA, GL_ONE.  Several of this port's MCRW/conversion paths
       can leave those sprites as fully opaque black-background quads.  If the
       image is essentially opaque and contains a meaningful black background,
       rebuild alpha from luminance and re-upload the same GL/D3D texture.  With
       additive blending, black lunar shadow pixels contribute zero light, which
       matches the Java render path. */
    int mostly_opaque = (opaque * 100u >= pixels * 95u && transparent == 0);
    int has_black_bg = (dark_opaque * 100u >= pixels * 5u && nonblack > 0);
    if (!mostly_opaque || !has_black_bg) return;

    for (size_t i = 0; i < pixels; ++i) {
        unsigned char *px = &t->rgba[i * 4u];
        unsigned char a = px[0];
        if (px[1] > a) a = px[1];
        if (px[2] > a) a = px[2];
        px[3] = a;
    }
    glBindTexture(GL_TEXTURE_2D, t->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->rgba);
}

static void normalize_sky_sprite_alphas(void) {
    normalize_sky_alpha(&tex_sun);
    normalize_sky_alpha(&tex_moon);
    normalize_sky_alpha(&tex_moon_phases);
}

static int terrain_tile_liquid_like_for_texture(const Texture *t, int tile, int water) {
    if (!t || !t->rgba || t->w <= 0 || t->h <= 0) return 0;
    int step_x = t->w / 16;
    int step_y = t->h / 16;
    if (step_x <= 0 || step_y <= 0) return 0;
    int tx = (tile & 15) * step_x;
    int ty = (tile >> 4) * step_y;
    if (tx + step_x > t->w || ty + step_y > t->h) return 0;
    int opaque = 0, match = 0, gray = 0;
    for (int yy = 0; yy < step_y; yy++) {
        for (int xx = 0; xx < step_x; xx++) {
            unsigned char *p = &t->rgba[((ty + yy) * t->w + (tx + xx)) * 4];
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
    if (opaque * 8 < step_x * step_y) return 0;
    if (gray * 3 > opaque) return 0;
    return match * 3 >= opaque;
}

static int terrain_liquid_region_like_for_texture(const Texture *t, int tile, int water, int tile_size) {
    if (tile_size < 1) tile_size = 1;
    for (int oy = 0; oy < tile_size; ++oy) {
        for (int ox = 0; ox < tile_size; ++ox) {
            if (!terrain_tile_liquid_like_for_texture(t, tile + ox + oy * 16, water)) return 0;
        }
    }
    return 1;
}

static void terrain_write_java_liquid_tile(Texture *t, int tile, int water, int flow, int tile_size) {
    if (!t || !t->rgba || t->w <= 0 || t->h <= 0) return;
    int step_x = t->w / 16;
    int step_y = t->h / 16;
    if (step_x <= 0 || step_y <= 0) return;
    if (tile_size < 1) tile_size = 1;
    int tx = (tile & 15) * step_x;
    int ty = (tile >> 4) * step_y;
    if (tx + step_x * tile_size > t->w || ty + step_y * tile_size > t->h) return;
    for (int oy = 0; oy < tile_size; ++oy) {
        for (int ox = 0; ox < tile_size; ++ox) {
            int dx = tx + ox * step_x;
            int dy = ty + oy * step_y;
            for (int y = 0; y < step_y; ++y) {
                for (int x = 0; x < step_x; ++x) {
                    float fx = (float)x / (float)step_x;
                    float fy = (float)y / (float)step_y;
                    float wave = (sinf((fx * 6.2831853f) + (flow ? fy * 12.5663706f : fy * 3.1415927f)) +
                                  cosf((fy * 6.2831853f) - (flow ? fx * 6.2831853f : fx * 3.1415927f))) * 0.5f;
                    float v = wave * 0.5f + 0.5f;
                    if (v < 0.0f) v = 0.0f;
                    if (v > 1.0f) v = 1.0f;
                    unsigned char *p = &t->rgba[((dy + y) * t->w + (dx + x)) * 4];
                    if (water) {
                        float vv = v * v;
                        p[0] = (unsigned char)(32.0f + vv * 32.0f);
                        p[1] = (unsigned char)(50.0f + vv * 64.0f);
                        p[2] = 255;
                        p[3] = (unsigned char)(146.0f + vv * 50.0f);
                    } else {
                        float vv = v * v;
                        p[0] = (unsigned char)(155.0f + v * 100.0f);
                        p[1] = (unsigned char)(vv * 255.0f);
                        p[2] = (unsigned char)(vv * vv * 128.0f);
                        p[3] = 255;
                    }
                }
            }
        }
    }
}

static void normalize_terrain_liquid_tiles(void) {
    if (!tex_terrain.id || !tex_terrain.rgba || tex_terrain.w < 128 || tex_terrain.h < 128) return;
    int changed = 0;
    if (!terrain_liquid_region_like_for_texture(&tex_terrain, 205, 1, 1)) {
        terrain_write_java_liquid_tile(&tex_terrain, 205, 1, 0, 1);
        changed = 1;
    }
    if (!terrain_liquid_region_like_for_texture(&tex_terrain, 206, 1, 2)) {
        terrain_write_java_liquid_tile(&tex_terrain, 206, 1, 1, 2);
        changed = 1;
    }
    if (!terrain_liquid_region_like_for_texture(&tex_terrain, 237, 0, 1)) {
        terrain_write_java_liquid_tile(&tex_terrain, 237, 0, 0, 1);
        changed = 1;
    }
    if (!terrain_liquid_region_like_for_texture(&tex_terrain, 238, 0, 2)) {
        terrain_write_java_liquid_tile(&tex_terrain, 238, 0, 1, 2);
        changed = 1;
    }
    if (!changed) return;
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_terrain.w, tex_terrain.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_terrain.rgba);
    stivufine_rebuild_terrain_mipmaps();
}


#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
static const char *pex_basename_asset(const char *path) {
    const char *base = path;
    for (const char *p = path; p && *p; ++p) {
        if (*p == '/' || *p == '\\') base = p + 1;
    }
    return base;
}

static int load_mcrw_memory(Texture *t, const unsigned char *data, unsigned int len, int repeat) {
    if (!data || len < 12) { PEX_PSP_LOGF("MCRW memory reject: null/short len=%u", len); return 0; }
    if (memcmp(data, "MCRW", 4) != 0) { PEX_PSP_LOGF("MCRW memory reject: bad magic %02x %02x %02x %02x", data[0], data[1], data[2], data[3]); return 0; }
    int w = read_u32_le_mem(data + 4);
    int h = read_u32_le_mem(data + 8);
    if (w <= 0 || h <= 0 || w > 4096 || h > 4096) { PEX_PSP_LOGF("MCRW memory reject: bad dims %dx%d len=%u", w, h, len); return 0; }
    size_t n = (size_t)w * (size_t)h * 4;
    if ((size_t)len < 12 + n) { PEX_PSP_LOGF("MCRW memory reject: truncated %dx%d need=%u got=%u", w, h, (unsigned)(12+n), len); return 0; }
    unsigned char *rgba = (unsigned char*)malloc(n);
    if (!rgba) { PEX_PSP_LOGF("MCRW memory reject: malloc failed bytes=%u", (unsigned)n); return 0; }
    memcpy(rgba, data + 12, n);
    int ok = upload_rgba_texture(t, w, h, rgba, repeat);
    PEX_PSP_LOGF("MCRW upload %dx%d repeat=%d -> tex=%u ok=%d", w, h, repeat, (unsigned)(t ? t->id : 0), ok);
    return ok;
}

static int psp_make_fallback_texture(Texture *t, const char *name, int w, int h, int repeat) {
    if (w <= 0) w = 16;
    if (h <= 0) h = 16;
    size_t n = (size_t)w * (size_t)h * 4;
    unsigned char *rgba = (unsigned char*)malloc(n);
    if (!rgba) { PEX_PSP_LOGF("fallback texture malloc failed for %s bytes=%u", name ? name : "?", (unsigned)n); return 0; }
    unsigned int seed = 2166136261u;
    if (name) for (const unsigned char *p=(const unsigned char*)name; *p; ++p) seed = (seed ^ *p) * 16777619u;
    unsigned char r = (unsigned char)(96u + (seed & 0x7fu));
    unsigned char g = (unsigned char)(96u + ((seed >> 8) & 0x7fu));
    unsigned char b = (unsigned char)(96u + ((seed >> 16) & 0x7fu));
    for (int y=0; y<h; ++y) {
        for (int x=0; x<w; ++x) {
            int k = (y*w + x) * 4;
            int checker = ((x >> 3) ^ (y >> 3)) & 1;
            rgba[k+0] = checker ? r : (unsigned char)(r/2);
            rgba[k+1] = checker ? g : (unsigned char)(g/2);
            rgba[k+2] = checker ? b : (unsigned char)(b/2);
            rgba[k+3] = 255;
        }
    }
    int ok = upload_rgba_texture(t, w, h, rgba, repeat);
    PEX_PSP_LOGF("fallback texture %s %dx%d -> tex=%u ok=%d", name ? name : "?", w, h, (unsigned)(t ? t->id : 0), ok);
    return ok;
}


#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
static void psp_drop_texture_cpu_copy(Texture *t) {
    if (!t || !t->rgba) return;
    free(t->rgba);
    t->rgba = NULL;
}
static void psp_drop_gui_texture_cpu_copies(void) {
    /* Keep CPU copies only where the game actually samples pixels on CPU: font
       for text width/alpha, terrain for item/color/cutout logic, and Steve. */
    psp_drop_texture_cpu_copy(&tex_bg);
    psp_drop_texture_cpu_copy(&tex_gui);
    psp_drop_texture_cpu_copy(&tex_black);
    psp_drop_texture_cpu_copy(&tex_pack);
    psp_drop_texture_cpu_copy(&tex_default_pack_icon);
    psp_drop_texture_cpu_copy(&tex_unknown_pack);
    psp_drop_texture_cpu_copy(&tex_icons);
    psp_drop_texture_cpu_copy(&tex_achievement);
    psp_drop_texture_cpu_copy(&tex_stat_slot);
    psp_drop_texture_cpu_copy(&tex_inventory);
    psp_drop_texture_cpu_copy(&tex_allitems);
    psp_drop_texture_cpu_copy(&tex_workbench);
    psp_drop_texture_cpu_copy(&tex_furnace_gui);
    psp_drop_texture_cpu_copy(&tex_chest_gui);
    psp_drop_texture_cpu_copy(&tex_items);
    for (int m = 0; m < 5; m++) for (int l = 0; l < 2; l++) psp_drop_texture_cpu_copy(&tex_armor[m][l]);
    psp_drop_texture_cpu_copy(&tex_clouds);
    psp_drop_texture_cpu_copy(&tex_water_overlay);
    psp_drop_texture_cpu_copy(&tex_shadow);
    psp_drop_texture_cpu_copy(&tex_particles);
}

#endif

static const unsigned char *psp_mcrw_pak_data(size_t *out_len) {
#if defined(PEX_PLATFORM_PSP)
    const unsigned char *start = pexcraft_psp_mcrw_assets_pak_start;
    const unsigned char *end = pexcraft_psp_mcrw_assets_pak_end;
#elif defined(PEX_PLATFORM_WII)
    const unsigned char *start = pexcraft_wii_mcrw_assets_pak_start;
    const unsigned char *end = pexcraft_wii_mcrw_assets_pak_end;
#else
    const unsigned char *start = NULL;
    const unsigned char *end = NULL;
#endif
    size_t len = 0;
    if (end >= start) len = (size_t)(end - start);
    if (out_len) *out_len = len;
    return start;
}

static unsigned int psp_embedded_mcrw_count(void) {
    size_t len = 0;
    const unsigned char *pak = psp_mcrw_pak_data(&len);
    if (!pak || len < 12) return 0;
    if (memcmp(pak, "PXCMCRW1", 8) != 0) return 0;
    return (unsigned int)read_u32_le_mem(pak + 8);
}

static int psp_ascii_ieq_len(const char *a, const unsigned char *b, unsigned int blen) {
    if (!a || !b) return 0;
    unsigned int i = 0;
    for (; i < blen; ++i) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = b[i];
        if (!ca) return 0;
        if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb - 'A' + 'a');
        if (ca != cb) return 0;
    }
    return a[i] == '\0';
}

static int psp_find_embedded_mcrw(const char *base, const unsigned char **out_data, unsigned int *out_len) {
    size_t len = 0;
    const unsigned char *pak = psp_mcrw_pak_data(&len);
    if (out_data) *out_data = NULL;
    if (out_len) *out_len = 0;
    if (!pak || len < 12) { PEX_PSP_LOGF("MCRW pak missing/short len=%u", (unsigned)len); return 0; }
    if (memcmp(pak, "PXCMCRW1", 8) != 0) { PEX_PSP_LOGF("MCRW pak bad magic len=%u", (unsigned)len); return 0; }
    unsigned int count = (unsigned int)read_u32_le_mem(pak + 8);
    size_t pos = 12;
    for (unsigned int i = 0; i < count; ++i) {
        if (pos + 12 > len) { PEX_PSP_LOGF("MCRW pak truncated entry header i=%u pos=%u len=%u", i, (unsigned)pos, (unsigned)len); return 0; }
        unsigned int name_len = (unsigned int)read_u32_le_mem(pak + pos + 0);
        unsigned int data_len = (unsigned int)read_u32_le_mem(pak + pos + 4);
        unsigned int data_off = (unsigned int)read_u32_le_mem(pak + pos + 8);
        pos += 12;
        if (name_len > 255 || pos + name_len > len) { PEX_PSP_LOGF("MCRW pak bad name i=%u name_len=%u", i, name_len); return 0; }
        const unsigned char *name = pak + pos;
        pos += name_len;
        if ((size_t)data_off > len || (size_t)data_len > len || (size_t)data_off + (size_t)data_len > len) {
            PEX_PSP_LOGF("MCRW pak bad data i=%u off=%u len=%u pak=%u", i, data_off, data_len, (unsigned)len);
            return 0;
        }
        if (psp_ascii_ieq_len(base, name, name_len)) {
            if (out_data) *out_data = pak + data_off;
            if (out_len) *out_len = data_len;
            PEX_PSP_LOGF("MCRW pak hit[%u]: %s len=%u", i, base ? base : "?", data_len);
            return 1;
        }
    }
    return 0;
}

static int load_embedded_mcrw(Texture *t, const char *filename, int repeat) {
    const char *base = pex_basename_asset(filename);
    unsigned int count = psp_embedded_mcrw_count();
    size_t pak_len = 0;
    (void)psp_mcrw_pak_data(&pak_len);
    PEX_PSP_LOGF("embedded pak lookup: %s base=%s count=%u pak_len=%u", filename ? filename : "?", base ? base : "?", count, (unsigned)pak_len);
    const unsigned char *data = NULL;
    unsigned int len = 0;
    if (psp_find_embedded_mcrw(base, &data, &len)) {
        return load_mcrw_memory(t, data, len, repeat);
    }
    PEX_PSP_LOGF("embedded pak miss: %s", base ? base : "?");
    return 0;
}
#endif

static int load_mcrw_path(Texture *t, const char *path, int repeat) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    char magic[4];
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, "MCRW", 4) != 0) { fclose(f); return 0; }
    int w = read_u32_le(f);
    int h = read_u32_le(f);
    if (w <= 0 || h <= 0 || w > 4096 || h > 4096) { fclose(f); return 0; }
    size_t n = (size_t)w * (size_t)h * 4;
    unsigned char *rgba = (unsigned char*)malloc(n);
    if (!rgba || fread(rgba, 1, n, f) != n) { if (rgba) free(rgba); fclose(f); return 0; }
    fclose(f);
    return upload_rgba_texture(t, w, h, rgba, repeat);
}

static int load_mcrw(Texture *t, const char *filename, int repeat) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    if (load_embedded_mcrw(t, filename, repeat)) return 1;
#endif
    char path[MAX_PATHBUF];
#if defined(PEX_PLATFORM_PSP)
    snprintf(path, sizeof(path), "assets/%s", filename);
    if (load_mcrw_path(t, path, repeat)) return 1;
    snprintf(path, sizeof(path), "./assets/%s", filename);
#elif defined(PEX_PLATFORM_WII)
    /* Wii textures are linked into boot.dol.  Do not require sd:/assets. */
    (void)path;
    return 0;
#else
    snprintf(path, sizeof(path), "assets\\%s", filename);
    if (load_mcrw_path(t, path, repeat)) return 1;
    snprintf(path, sizeof(path), ".\\assets\\%s", filename);
#endif
    return load_mcrw_path(t, path, repeat);
}

#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
static int ensure_wic(void) { return 1; }
static int load_png_texture(Texture *t, const char *path, int repeat) { (void)t; (void)path; (void)repeat; return 0; }
#elif defined(PEX_PLATFORM_ANDROID) || defined(PEX_PLATFORM_ANDROID_TV)
static int ensure_wic(void) { return 1; }

static unsigned char *android_decode_png_rgba(const char *path, int *out_w, int *out_h) {
    if (out_w) *out_w = 0;
    if (out_h) *out_h = 0;
    if (!path || !file_exists(path)) return NULL;

    char norm[MAX_PATHBUF];
    pex_normalize_path(norm, sizeof(norm), path);
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = SDL_AndroidGetActivity();
    if (!env || !activity) return NULL;
    jclass cls = (*env)->GetObjectClass(env, activity);
    jmethodID mid = cls ? (*env)->GetMethodID(env, cls, "decodePngToRgba", "(Ljava/lang/String;)[B") : NULL;
    if (!mid) {
        if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionDescribe(env); (*env)->ExceptionClear(env); }
        if (cls) (*env)->DeleteLocalRef(env, cls);
        (*env)->DeleteLocalRef(env, activity);
        return NULL;
    }
    jstring jpath = (*env)->NewStringUTF(env, norm);
    jbyteArray arr = (jbyteArray)(*env)->CallObjectMethod(env, activity, mid, jpath);
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionDescribe(env); (*env)->ExceptionClear(env); }
    if (jpath) (*env)->DeleteLocalRef(env, jpath);
    if (cls) (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, activity);
    if (!arr) return NULL;

    jsize len = (*env)->GetArrayLength(env, arr);
    if (len < 8) { (*env)->DeleteLocalRef(env, arr); return NULL; }
    jbyte *bytes = (*env)->GetByteArrayElements(env, arr, NULL);
    if (!bytes) { (*env)->DeleteLocalRef(env, arr); return NULL; }
    unsigned char *b = (unsigned char *)bytes;
    int w = (int)((unsigned)b[0] | ((unsigned)b[1] << 8) | ((unsigned)b[2] << 16) | ((unsigned)b[3] << 24));
    int h = (int)((unsigned)b[4] | ((unsigned)b[5] << 8) | ((unsigned)b[6] << 16) | ((unsigned)b[7] << 24));
    size_t need = (size_t)w * (size_t)h * 4u;
    unsigned char *rgba = NULL;
    if (w > 0 && h > 0 && w <= 4096 && h <= 4096 && (size_t)len >= 8u + need) {
        rgba = (unsigned char *)malloc(need);
        if (rgba) memcpy(rgba, b + 8, need);
        if (out_w) *out_w = w;
        if (out_h) *out_h = h;
    }
    (*env)->ReleaseByteArrayElements(env, arr, bytes, JNI_ABORT);
    (*env)->DeleteLocalRef(env, arr);
    return rgba;
}

static int load_png_texture(Texture *t, const char *path, int repeat) {
    int w = 0, h = 0;
    unsigned char *rgba = android_decode_png_rgba(path, &w, &h);
    if (!rgba) return 0;
    log_msg("Loaded PNG texture: %s (%dx%d)", path, w, h);
    return upload_rgba_texture(t, w, h, rgba, repeat);
}
#elif defined(PEX_PLATFORM_SDL2)
static int ensure_wic(void) { return 1; }

static int load_png_texture(Texture *t, const char *path, int repeat) {
    if (!file_exists(path)) return 0;
    char norm[MAX_PATHBUF];
    pex_normalize_path(norm, sizeof(norm), path);
    SDL_Surface *surf = IMG_Load(norm);
    if (!surf) {
        log_msg("Failed to load PNG texture: %s", norm);
        return 0;
    }
    SDL_Surface *rgba_surf = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surf);
    if (!rgba_surf) return 0;
    int w = rgba_surf->w;
    int h = rgba_surf->h;
    if (w <= 0 || h <= 0 || w > 4096 || h > 4096) { SDL_FreeSurface(rgba_surf); return 0; }
    size_t bytes = (size_t)w * (size_t)h * 4;
    unsigned char *rgba = (unsigned char*)malloc(bytes);
    if (!rgba) { SDL_FreeSurface(rgba_surf); return 0; }
    unsigned char *src = (unsigned char*)rgba_surf->pixels;
    for (int y = 0; y < h; ++y) memcpy(rgba + (size_t)y * (size_t)w * 4, src + (size_t)y * (size_t)rgba_surf->pitch, (size_t)w * 4);
    SDL_FreeSurface(rgba_surf);
    log_msg("Loaded PNG texture: %s (%dx%d)", norm, w, h);
    return upload_rgba_texture(t, w, h, rgba, repeat);
}
#else
static int ensure_wic(void) {
    if (g_wic_factory) return 1;
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return 0;
    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICImagingFactory, (void**)&g_wic_factory);
    return SUCCEEDED(hr) && g_wic_factory != NULL;
}

static int load_png_texture(Texture *t, const char *path, int repeat) {
    if (!file_exists(path) || !ensure_wic()) return 0;

    WCHAR wpath[MAX_PATHBUF];
    if (!MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATHBUF)) return 0;

    IWICBitmapDecoder *decoder = NULL;
    IWICBitmapFrameDecode *frame = NULL;
    IWICFormatConverter *converter = NULL;
    HRESULT hr = IWICImagingFactory_CreateDecoderFromFilename(
        g_wic_factory, wpath, NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr)) goto fail;

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    if (FAILED(hr)) goto fail;

    hr = IWICImagingFactory_CreateFormatConverter(g_wic_factory, &converter);
    if (FAILED(hr)) goto fail;

    hr = IWICFormatConverter_Initialize(converter, (IWICBitmapSource*)frame,
        &GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, NULL, 0.0,
        WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) goto fail;

    UINT w = 0, h = 0;
    hr = IWICFormatConverter_GetSize(converter, &w, &h);
    if (FAILED(hr) || w == 0 || h == 0 || w > 4096 || h > 4096) goto fail;

    UINT stride = w * 4;
    UINT bytes = stride * h;
    unsigned char *rgba = (unsigned char*)malloc(bytes);
    if (!rgba) goto fail;

    hr = IWICFormatConverter_CopyPixels(converter, NULL, stride, bytes, rgba);
    if (FAILED(hr)) { free(rgba); goto fail; }

    if (converter) IWICFormatConverter_Release(converter);
    if (frame) IWICBitmapFrameDecode_Release(frame);
    if (decoder) IWICBitmapDecoder_Release(decoder);
    log_msg("Loaded PNG texture: %s (%ux%u)", path, (unsigned)w, (unsigned)h);
    return upload_rgba_texture(t, (int)w, (int)h, rgba, repeat);

fail:
    if (converter) IWICFormatConverter_Release(converter);
    if (frame) IWICBitmapFrameDecode_Release(frame);
    if (decoder) IWICBitmapDecoder_Release(decoder);
    log_msg("Failed to load PNG texture: %s", path);
    return 0;
}
#endif

static int load_png_asset(Texture *t, const char *rel, int repeat) {
    char path[MAX_PATHBUF];
    snprintf(path, sizeof(path), "assets\\%s", rel);
    if (load_png_texture(t, path, repeat)) return 1;
    snprintf(path, sizeof(path), ".\\assets\\%s", rel);
    return load_png_texture(t, path, repeat);
}

static int load_custom_skin_path(const char *path, int persist) {
    if (!path || !path[0]) return 0;
    Texture tmp = {0};
    if (!load_png_texture(&tmp, path, 0)) return 0;
    if (!((tmp.w == 64 && tmp.h == 32) || (tmp.w == 64 && tmp.h == 64))) {
        free_texture(&tmp);
        open_notice("Skins", "Skin PNG must be 64x32 or 64x64.", "Classic 64x32 skins work best.");
        return 0;
    }
    free_texture(&tex_steve);
    tex_steve = tmp;
    if (persist) {
        snprintf(g_opts.skin_path, sizeof(g_opts.skin_path), "%s", path);
        save_options();
    }
    if (g_mp_connected) pex_net_send_skin();
    return 1;
}

static int choose_and_import_skin(void) {
#ifdef PEX_PLATFORM_PSP
    open_notice("Skins", "Skin import is not available on PSP.", "Copy converted assets before building the EBOOT.");
    return 0;
#elif defined(PEX_PLATFORM_SDL2) || defined(PEX_PLATFORM_XBOX_UWP)
    char path[MAX_PATHBUF];
    snprintf(path, sizeof(path), "%s/custom.png", g_skin_dir);
    if (load_custom_skin_path(path, 1)) return 1;
    open_notice("Skins", "Skin import is not available here.", "Copy your PNG to the PexCraft skins folder as custom.png first.");
    return 0;
#else
    char chosen[MAX_PATHBUF] = "";
    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFilter = "PNG Skin (*.png)\0*.png\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = chosen;
    ofn.nMaxFile = sizeof(chosen);
    ofn.lpstrTitle = "Import skin";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameA(&ofn)) return 0;

    ensure_dir(g_skin_dir);
    char dst[MAX_PATHBUF];
    snprintf(dst, sizeof(dst), "%s\\custom.png", g_skin_dir);
    if (!CopyFileA(chosen, dst, FALSE)) {
        open_notice("Skins", "Could not copy the selected PNG", "into the PEXCRAFT skins folder.");
        return 0;
    }
    return load_custom_skin_path(dst, 1);
#endif
}


static void trim_pack_line_34(char *line) {
    line[strcspn(line, "\r\n")] = 0;
    if (strlen(line) > 34) line[34] = 0;
}

static void free_texture_pack_icons(void) {
    for (int i = 0; i < g_texpack_count; i++) {
        if (g_texpacks[i].has_icon) {
            free_texture(&g_texpacks[i].icon);
            g_texpacks[i].has_icon = 0;
        }
    }
}

static void read_pack_text(TexturePackEntry *e) {
    e->desc1[0] = 0;
    e->desc2[0] = 0;
    char txt[MAX_PATHBUF];
    path_join(txt, sizeof(txt), e->path, "pack.txt");
    FILE *f = fopen(txt, "r");
    if (!f) return;
    if (fgets(e->desc1, sizeof(e->desc1), f)) trim_pack_line_34(e->desc1);
    if (fgets(e->desc2, sizeof(e->desc2), f)) trim_pack_line_34(e->desc2);
    fclose(f);
}

static void load_pack_icon(TexturePackEntry *e) {
    char path[MAX_PATHBUF];
    path_join(path, sizeof(path), e->path, "pack.png");
    if (load_png_texture(&e->icon, path, 0)) e->has_icon = 1;
}


static void pack_asset_path(char *out, size_t cap) {
    path_join(out, cap, g_texpack_dir, CLASSIC_PACK_NAME);
}

static int release_file_exists_in_dir(const char *dir, const char *rel) {
    char norm[MAX_PATHBUF], path[MAX_PATHBUF];
    snprintf(norm, sizeof(norm), "%s", rel ? rel : "");
    for (char *c = norm; *c; ++c) {
#ifdef PEX_PLATFORM_SDL2
        if (*c == '\\') *c = '/';
#else
        if (*c == '/') *c = '\\';
#endif
    }
    path_join(path, sizeof(path), dir, norm);
    return file_exists(path);
}

static int pack_is_installed(void) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    return 1;
#endif
    char dir[MAX_PATHBUF];
    pack_asset_path(dir, sizeof(dir));
    return release_file_exists_in_dir(dir, "terrain.png") &&
           release_file_exists_in_dir(dir, "gui/gui.png") &&
           release_file_exists_in_dir(dir, "font/default.png") &&
           release_file_exists_in_dir(dir, "font.txt") &&
           release_file_exists_in_dir(dir, "font/glyph_sizes.bin") &&
           release_file_exists_in_dir(dir, "font/glyph_04.png") &&
           release_file_exists_in_dir(dir, "font/glyph_06.png") &&
           release_file_exists_in_dir(dir, "title/mclogo.png") &&
           release_file_exists_in_dir(dir, "title/mojang.png") &&
           release_file_exists_in_dir(dir, "title/bg/panorama0.png");
}

static int pack_missing_required_textures(void) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    return 0;
#endif
    char dir[MAX_PATHBUF];
    static const char *required[] = {
        "terrain.png",
        "gui/gui.png",
        "gui/items.png",
        "gui/icons.png",
        "achievement/bg.png",
        "gui/slot.png",
        "font/default.png",
        "font.txt",
        "font/glyph_sizes.bin",
        "font/glyph_04.png",
        "font/glyph_06.png",
        "title/mclogo.png",
        "title/mojang.png",
        "title/bg/panorama0.png",
        "title/bg/panorama1.png",
        "title/bg/panorama2.png",
        "title/bg/panorama3.png",
        "title/bg/panorama4.png",
        "title/bg/panorama5.png",
        "mob/pig.png",
        "mob/sheep.png",
        "mob/sheep_fur.png",
        "mob/cow.png",
        "mob/chicken.png",
        "mob/saddle.png",
        "mob/creeper.png",
        "mob/skeleton.png",
        "mob/spider.png",
        "mob/zombie.png",
        "mob/slime.png",
        "mob/ghast.png",
        "mob/ghast_fire.png",
        "mob/pigzombie.png",
        "mob/enderman.png",
        "mob/cavespider.png",
        "mob/silverfish.png",
        "mob/fire.png",
        "mob/lava.png",
        "mob/enderdragon/ender.png",
        "mob/squid.png",
        "mob/wolf.png",
        "mob/wolf_tame.png",
        "mob/wolf_angry.png",
        "mob/redcow.png",
        "mob/snowman.png",
        "mob/ozelot.png",
        "mob/cat_red.png",
        "mob/villager_golem.png",
        "mob/villager/villager.png"
    };
    if (!pack_is_installed()) return 0;
    pack_asset_path(dir, sizeof(dir));
    for (int i = 0; i < (int)(sizeof(required) / sizeof(required[0])); ++i) {
        if (!release_file_exists_in_dir(dir, required[i])) return 1;
    }
    return 0;
}

static void classic_resources_path(char *out, size_t cap) {
    path_join(out, cap, g_mc_dir, "resources");
}

static void classic_sound_marker_path(char *out, size_t cap) {
    char root[MAX_PATHBUF];
    classic_resources_path(root, sizeof(root));
    path_join(out, cap, root, ".pexcraft-release-sounds");
}

static void classic_sound_manifest_path(char *out, size_t cap) {
    char root[MAX_PATHBUF];
    classic_resources_path(root, sizeof(root));
    path_join(out, cap, root, ".pexcraft-release-sounds-manifest");
}

static unsigned long long classic_file_size_bytes(const char *path) {
    FILE *f = fopen(path, "rb");
    long n;
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    n = ftell(f);
    fclose(f);
    return n > 0 ? (unsigned long long)n : 0ULL;
}

static int classic_sound_manifest_valid(const char *manifest, const char *root) {
    FILE *f = fopen(manifest, "r");
    char line[MAX_PATHBUF + 96];
    int checked = 0;
    if (!f) return 0;
    while (fgets(line, sizeof(line), f)) {
        char *path = line;
        char *size_s;
        char full[MAX_PATHBUF];
        unsigned long long expect;
        char *nl = strchr(line, '\n');
        if (nl) *nl = 0;
        if (!strncmp(path, "asset|", 6)) path += 6;
        else continue;
        size_s = strchr(path, '|');
        if (!size_s) { fclose(f); return 0; }
        *size_s++ = 0;
        expect = (unsigned long long)strtoull(size_s, NULL, 10);
        if (!path[0] || strstr(path, "..") || path[0] == '/' || path[0] == '\\' || expect == 0) { fclose(f); return 0; }
        path_join(full, sizeof(full), root, path);
        if (classic_file_size_bytes(full) != expect) { fclose(f); return 0; }
        checked++;
    }
    fclose(f);
    return checked > 0;
}

static int classic_sound_marker_installed_mask(void) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    return CLASSIC_AUDIO_ALL;
#else
    char marker[MAX_PATHBUF];
    char text[256];
    FILE *f;
    classic_sound_marker_path(marker, sizeof(marker));
    if (!file_exists(marker)) return 0;
    f = fopen(marker, "r");
    if (!f) return 0;
    text[0] = 0;
    if (!fgets(text, sizeof(text), f)) { fclose(f); return 0; }
    if (strstr(text, "legacy-all-audio-v2")) { fclose(f); return CLASSIC_AUDIO_ALL; }
    if (!strstr(text, "legacy-selected-audio-v3")) { fclose(f); return 0; }
    while (fgets(text, sizeof(text), f)) {
        if (!strncmp(text, "mask:", 5)) {
            int mask = atoi(text + 5) & CLASSIC_AUDIO_ALL;
            fclose(f);
            return mask;
        }
    }
    fclose(f);
    return 0;
#endif
}

static int classic_sounds_installed_mask(int mask) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    (void)mask;
    return 1;
#else
    char marker[MAX_PATHBUF];
    char manifest[MAX_PATHBUF];
    char root[MAX_PATHBUF];
    int installed_mask;
    if ((mask & CLASSIC_AUDIO_ALL) == 0) return 1;
    classic_resources_path(root, sizeof(root));
    classic_sound_marker_path(marker, sizeof(marker));
    classic_sound_manifest_path(manifest, sizeof(manifest));
    if (!file_exists(marker) || !file_exists(manifest)) return 0;
    installed_mask = classic_sound_marker_installed_mask();
    if ((installed_mask & mask) != (mask & CLASSIC_AUDIO_ALL)) return 0;
    return classic_sound_manifest_valid(manifest, root);
#endif
}

static int classic_sounds_installed(void) {
    return classic_sounds_installed_mask(CLASSIC_AUDIO_ALL);
}

static int classic_selected_audio_mask(void) {
#if PEX_CLASSIC_SOUND_DOWNLOAD_SUPPORTED
    if (!g_opts.download_classic_sounds || g_opts.ignore_classic_sounds_warning) return 0;
    return g_opts.classic_audio_mask & CLASSIC_AUDIO_ALL;
#else
    return 0;
#endif
}

static int classic_wants_sound_download(void) {
    return classic_selected_audio_mask() != 0;
}

static int classic_resources_need_update(void) {
    int missing_textures = g_opts.download_classic_textures && (!pack_is_installed() || pack_missing_required_textures());
    int audio_mask = classic_selected_audio_mask();
    int missing_sounds = audio_mask && !classic_sounds_installed_mask(audio_mask);
    if (missing_textures && !g_opts.ignore_classic_resources_warning) return 1;
    if (missing_sounds) return 1;
    return 0;
}

static int should_show_pack_download_prompt(void) {
    return classic_resources_need_update();
}

static void classic_resource_missing_summary(char *out, size_t cap) {
    int missing_textures = g_opts.download_classic_textures && (!pack_is_installed() || pack_missing_required_textures());
    int audio_mask = classic_selected_audio_mask();
    int missing_sounds = audio_mask && !classic_sounds_installed_mask(audio_mask);
    if (missing_textures && missing_sounds) snprintf(out, cap, "Selected Release textures/audio should be downloaded.");
    else if (missing_textures) snprintf(out, cap, "Release textures should be downloaded.");
    else if (missing_sounds) snprintf(out, cap, "Selected Release audio should be downloaded.");
    else snprintf(out, cap, "Selected Release resources are up to date.");
}

static void add_builtin_classic_texture_pack(int *wanted) {
    if (g_texpack_count >= MAX_TEXPACKS) return;
    TexturePackEntry *e = &g_texpacks[g_texpack_count++];
    memset(e, 0, sizeof(*e));
    snprintf(e->name, sizeof(e->name), "%s", CLASSIC_PACK_NAME);
    pack_asset_path(e->path, sizeof(e->path));
    snprintf(e->desc1, sizeof(e->desc1), "Minecraft 1.2.5 release textures");
    e->is_default = 1;
    int installed = pack_is_installed();
    if (installed) {
        if (pack_missing_required_textures()) snprintf(e->desc2, sizeof(e->desc2), "Missing item/block/mob textures");
        else if (classic_wants_sound_download() && !classic_sounds_installed_mask(classic_selected_audio_mask())) snprintf(e->desc2, sizeof(e->desc2), "Missing Release audio");
        else if (classic_sounds_installed()) snprintf(e->desc2, sizeof(e->desc2), "Release textures + audio");
        else snprintf(e->desc2, sizeof(e->desc2), "Downloaded from 1.2.5 client.jar");
    } else {
        snprintf(e->desc2, sizeof(e->desc2), "Click to download Release resources");
    }
    e->is_builtin_classic = 1;
    /* Use the normal fallback icon slot so the built-in Release entry stays iconless only when missing. */
    e->no_icon = 0;
    if (installed && !strcmp(g_opts.skin, e->name)) *wanted = g_texpack_count - 1;
}

static void clamp_texpack_scroll(void) {
    int top = 32;
    int bottom = g_gui_h - 58 + 4;
    int max_scroll = g_texpack_count * 36 - (bottom - top - 4);
    if (max_scroll < 0) max_scroll /= 2;
    if (g_texpack_scroll < 0) g_texpack_scroll = 0;
    if (g_texpack_scroll > max_scroll) g_texpack_scroll = max_scroll;
}

static void scan_texture_packs(void) {
    free_texture_pack_icons();
    g_texpack_count = 0;
    int wanted = 0;

    add_builtin_classic_texture_pack(&wanted);

#if (defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY) || defined(PEX_PLATFORM_WII)
    /* Console assets are linked into the executable; do not scan or create texturepack folders. */
    g_selected_texpack = wanted;
    snprintf(g_current_texpack, sizeof(g_current_texpack), "%s", g_texpacks[g_selected_texpack].name);
    clamp_texpack_scroll();
    return;
#endif

    ensure_dir(g_texpack_dir);
    char pattern[MAX_PATHBUF];
    WIN32_FIND_DATAA fd;
#ifdef PEX_PLATFORM_SDL2
    snprintf(pattern, sizeof(pattern), "%s/*", g_texpack_dir);
#else
    snprintf(pattern, sizeof(pattern), "%s\\*", g_texpack_dir);
#endif
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
            if (strcmp(fd.cFileName, CLASSIC_PACK_NAME) == 0) continue;
            if (g_texpack_count >= MAX_TEXPACKS) break;
            TexturePackEntry *e = &g_texpacks[g_texpack_count++];
            memset(e, 0, sizeof(*e));
            snprintf(e->name, sizeof(e->name), "%s", fd.cFileName);
            path_join(e->path, sizeof(e->path), g_texpack_dir, fd.cFileName);
            read_pack_text(e);
            load_pack_icon(e);
            if (!strcmp(g_opts.skin, e->name)) wanted = g_texpack_count - 1;
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }

    g_selected_texpack = wanted;
    snprintf(g_current_texpack, sizeof(g_current_texpack), "%s", g_texpacks[g_selected_texpack].name);
    clamp_texpack_scroll();
}


static int make_solid_texture(Texture *t, int w, int h, unsigned char r, unsigned char g, unsigned char b, unsigned char a, int repeat) {
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;
    size_t n = (size_t)w * (size_t)h * 4u;
    unsigned char *rgba = (unsigned char *)malloc(n);
    if (!rgba) return 0;
    for (size_t i = 0; i < n; i += 4) {
        rgba[i + 0] = r;
        rgba[i + 1] = g;
        rgba[i + 2] = b;
        rgba[i + 3] = a;
    }
    return upload_rgba_texture(t, w, h, rgba, repeat);
}

static int try_release_texture(Texture *tex, const char *rel, int repeat) {
    char dir[MAX_PATHBUF], path[MAX_PATHBUF];
    pack_asset_path(dir, sizeof(dir));
    snprintf(path, sizeof(path), "%s\\%s", dir, rel);
    if (load_png_texture(tex, path, repeat)) return 1;
    snprintf(path, sizeof(path), "%s/%s", dir, rel);
    for (char *c = path; *c; ++c) if (*c == '\\') *c = '/';
    return load_png_texture(tex, path, repeat);
}

static int load_release_textures_from_pack(void) {
    if (!pack_is_installed()) return 0;
    int ok = 1;
    ok = try_release_texture(&tex_bg, "gui\\background.png", 1) && ok;
    ok = try_release_texture(&tex_gui, "gui\\gui.png", 0) && ok;
    ok = try_release_texture(&tex_font, "font\\default.png", 0) && ok;
    ok = try_release_texture(&tex_terrain, "terrain.png", 1) && ok;
    normalize_terrain_liquid_tiles();
    make_solid_texture(&tex_black, 16, 16, 0, 0, 0, 255, 1);
    try_release_texture(&tex_pack, "pack.png", 0);
    if (!tex_pack.id) make_solid_texture(&tex_pack, 32, 32, 64, 64, 64, 255, 0);
    if (tex_pack.id && tex_pack.rgba) {
        Texture tmp = {0};
        size_t bytes = (size_t)tex_pack.w * (size_t)tex_pack.h * 4u;
        unsigned char *copy = (unsigned char*)malloc(bytes);
        if (copy) { memcpy(copy, tex_pack.rgba, bytes); upload_rgba_texture(&tmp, tex_pack.w, tex_pack.h, copy, 0); }
        tex_default_pack_icon = tmp;
    } else make_solid_texture(&tex_default_pack_icon, 32, 32, 64, 64, 64, 255, 0);
    if (tex_pack.id && tex_pack.rgba) {
        Texture tmp = {0};
        size_t bytes = (size_t)tex_pack.w * (size_t)tex_pack.h * 4u;
        unsigned char *copy = (unsigned char*)malloc(bytes);
        if (copy) { memcpy(copy, tex_pack.rgba, bytes); upload_rgba_texture(&tmp, tex_pack.w, tex_pack.h, copy, 0); }
        tex_unknown_pack = tmp;
    } else make_solid_texture(&tex_unknown_pack, 32, 32, 96, 96, 96, 255, 0);
    ok = try_release_texture(&tex_icons, "gui\\icons.png", 0) && ok;
    ok = try_release_texture(&tex_achievement, "achievement\\bg.png", 0) && ok;
    try_release_texture(&tex_stat_slot, "gui\\slot.png", 0);
    ok = try_release_texture(&tex_inventory, "gui\\inventory.png", 0) && ok;
    try_release_texture(&tex_allitems, "gui\\allitems.png", 0);
    try_release_texture(&tex_workbench, "gui\\crafting.png", 0);
    try_release_texture(&tex_furnace_gui, "gui\\furnace.png", 0);
    try_release_texture(&tex_chest_gui, "gui\\container.png", 0);
    try_release_texture(&tex_chest_gui, "gui\\chest.png", 0);
    try_release_texture(&tex_chest_entity, "item\\chest.png", 0);
    try_release_texture(&tex_large_chest_entity, "item\\largechest.png", 0);
    try_release_texture(&tex_items, "gui\\items.png", 0);
    try_release_texture(&tex_xporb, "item\\xporb.png", 0);
    try_release_texture(&tex_arrows, "item\\arrows.png", 0);
    for (int m = 0; m < 5; m++) for (int l = 0; l < 2; l++) free_texture(&tex_armor[m][l]);
    try_release_texture(&tex_armor[0][0], "armor\\cloth_1.png", 0);
    try_release_texture(&tex_armor[0][1], "armor\\cloth_2.png", 0);
    try_release_texture(&tex_armor[1][0], "armor\\chain_1.png", 0);
    try_release_texture(&tex_armor[1][1], "armor\\chain_2.png", 0);
    try_release_texture(&tex_armor[2][0], "armor\\iron_1.png", 0);
    try_release_texture(&tex_armor[2][1], "armor\\iron_2.png", 0);
    try_release_texture(&tex_armor[3][0], "armor\\diamond_1.png", 0);
    try_release_texture(&tex_armor[3][1], "armor\\diamond_2.png", 0);
    try_release_texture(&tex_armor[4][0], "armor\\gold_1.png", 0);
    try_release_texture(&tex_armor[4][1], "armor\\gold_2.png", 0);
    ok = (try_release_texture(&tex_steve, "mob\\char.png", 0) || try_release_texture(&tex_steve, "char.png", 0)) && ok;
    try_release_texture(&tex_mob_pig, "mob\\pig.png", 0);
    try_release_texture(&tex_mob_sheep, "mob\\sheep.png", 0);
    try_release_texture(&tex_mob_sheep_fur, "mob\\sheep_fur.png", 0);
    try_release_texture(&tex_mob_cow, "mob\\cow.png", 0);
    try_release_texture(&tex_mob_chicken, "mob\\chicken.png", 0);
    try_release_texture(&tex_mob_saddle, "mob\\saddle.png", 0);
    try_release_texture(&tex_mob_creeper, "mob\\creeper.png", 0);
    try_release_texture(&tex_mob_creeper_power, "armor\\power.png", 1);
    try_release_texture(&tex_mob_skeleton, "mob\\skeleton.png", 0);
    try_release_texture(&tex_mob_spider, "mob\\spider.png", 0);
    try_release_texture(&tex_mob_spider_eyes, "mob\\spider_eyes.png", 0);
    try_release_texture(&tex_mob_zombie, "mob\\zombie.png", 0);
    try_release_texture(&tex_mob_slime, "mob\\slime.png", 0);
    try_release_texture(&tex_mob_ghast, "mob\\ghast.png", 0);
    try_release_texture(&tex_mob_ghast_fire, "mob\\ghast_fire.png", 0);
    try_release_texture(&tex_mob_pigzombie, "mob\\pigzombie.png", 0);
    try_release_texture(&tex_mob_enderman, "mob\\enderman.png", 0);
    try_release_texture(&tex_mob_enderman_eyes, "mob\\enderman_eyes.png", 0);
    try_release_texture(&tex_mob_cavespider, "mob\\cavespider.png", 0);
    try_release_texture(&tex_mob_silverfish, "mob\\silverfish.png", 0);
    try_release_texture(&tex_mob_blaze, "mob\\fire.png", 0);
    try_release_texture(&tex_mob_lava, "mob\\lava.png", 0);
    try_release_texture(&tex_mob_enderdragon, "mob\\enderdragon\\ender.png", 0);
    try_release_texture(&tex_mob_squid, "mob\\squid.png", 0);
    try_release_texture(&tex_mob_wolf, "mob\\wolf.png", 0);
    try_release_texture(&tex_mob_wolf_tame, "mob\\wolf_tame.png", 0);
    try_release_texture(&tex_mob_wolf_angry, "mob\\wolf_angry.png", 0);
    try_release_texture(&tex_mob_mooshroom, "mob\\redcow.png", 0);
    try_release_texture(&tex_mob_snowman, "mob\\snowman.png", 0);
    try_release_texture(&tex_mob_ocelot, "mob\\ozelot.png", 0);
    try_release_texture(&tex_mob_cat_black, "mob\\cat_black.png", 0);
    try_release_texture(&tex_mob_cat_red, "mob\\cat_red.png", 0);
    try_release_texture(&tex_mob_cat_siamese, "mob\\cat_siamese.png", 0);
    try_release_texture(&tex_mob_villager_golem, "mob\\villager_golem.png", 0);
    try_release_texture(&tex_mob_villager, "mob\\villager\\villager.png", 0);
    try_release_texture(&tex_mob_villager_farmer, "mob\\villager\\farmer.png", 0);
    try_release_texture(&tex_mob_villager_librarian, "mob\\villager\\librarian.png", 0);
    try_release_texture(&tex_mob_villager_priest, "mob\\villager\\priest.png", 0);
    try_release_texture(&tex_mob_villager_smith, "mob\\villager\\smith.png", 0);
    try_release_texture(&tex_mob_villager_butcher, "mob\\villager\\butcher.png", 0);
    try_release_texture(&tex_clouds, "environment\\clouds.png", 1);
    try_release_texture(&tex_sun, "terrain\\sun.png", 0);
    try_release_texture(&tex_moon, "terrain\\moon.png", 0);
    try_release_texture(&tex_moon_phases, "terrain\\moon_phases.png", 0);
    /* Source C++ RenderGlobal renders sun/moon with original texture alpha. */
    try_release_texture(&tex_water_overlay, "misc\\water.png", 1);
    try_release_texture(&tex_shadow, "misc\\shadow.png", 0);
    try_release_texture(&tex_grasscolor, "misc\\grasscolor.png", 0);
    try_release_texture(&tex_foliagecolor, "misc\\foliagecolor.png", 0);
    /* StivuFine custom color maps are texture-pack overlays, not vanilla base
       assets.  Reset them when returning to the base pack so stale custom maps
       cannot survive Custom Colors OFF or a texture-pack switch. */
    free_texture(&tex_watercolor);
    free_texture(&tex_pinecolor);
    free_texture(&tex_birchcolor);
    free_texture(&tex_swampgrasscolor);
    free_texture(&tex_swampfoliagecolor);
    stivufine_clear_random_mob_variants();
    stivufine_lilypad_color = -1;
    try_release_texture(&tex_particles, "particles.png", 0);
    ok = try_release_texture(&tex_title_logo, "title\\mclogo.png", 0) && ok;
    ok = try_release_texture(&tex_mojang, "title\\mojang.png", 0) && ok;
    set_texture_filter_wrap(&tex_title_logo, 0, 0);
    set_texture_filter_wrap(&tex_mojang, 1, 0);
    for (int i = 0; i < 6; ++i) {
        char rel[64];
        snprintf(rel, sizeof(rel), "title\\bg\\panorama%d.png", i);
        ok = try_release_texture(&tex_panorama[i], rel, 1) && ok;
        set_texture_filter_wrap(&tex_panorama[i], 0, 1);
    }
    return ok;
}

static int load_default_textures(void) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    /* Console builds must never stop booting just because an embedded texture is missing.
       Log every asset, then create a tiny fallback texture so we can reach the
       menu and debug the renderer/input on real hardware/PPSSPP. */
    int missing = 0;
#define PEX_PSP_LOAD_REQ(tex, file, repeat, fw, fh) do {         PEX_PSP_LOGF("LOAD_REQ %s", file);         if (!load_mcrw((tex), (file), (repeat))) {             ++missing;             PEX_PSP_LOGF("LOAD_REQ failed: %s; using fallback", file);             psp_make_fallback_texture((tex), (file), (fw), (fh), (repeat));         }     } while (0)
#define PEX_PSP_LOAD_OPT(tex, file, repeat, fw, fh) do {         PEX_PSP_LOGF("LOAD_OPT %s", file);         if (!load_mcrw((tex), (file), (repeat))) {             PEX_PSP_LOGF("LOAD_OPT missing: %s; using fallback", file);             psp_make_fallback_texture((tex), (file), (fw), (fh), (repeat));         }     } while (0)
    PEX_PSP_LOAD_REQ(&tex_bg, "gui_background.mcrw", 1, 16, 16);
    PEX_PSP_LOAD_REQ(&tex_gui, "gui_gui.mcrw", 0, 256, 256);
    PEX_PSP_LOAD_REQ(&tex_font, "font_default.mcrw", 0, 128, 128);
    PEX_PSP_LOAD_REQ(&tex_terrain, "terrain.mcrw", 1, 128, 128);
    normalize_terrain_liquid_tiles();
    PEX_PSP_LOAD_REQ(&tex_black, "title_black.mcrw", 1, 16, 16);
    PEX_PSP_LOAD_OPT(&tex_pack, "pack.mcrw", 0, 32, 32);
    PEX_PSP_LOAD_OPT(&tex_default_pack_icon, "pack.mcrw", 0, 32, 32);
    PEX_PSP_LOAD_OPT(&tex_unknown_pack, "unknown_pack.mcrw", 0, 32, 32);
    PEX_PSP_LOAD_REQ(&tex_icons, "gui_icons.mcrw", 0, 256, 256);
    PEX_PSP_LOAD_OPT(&tex_achievement, "achievement_bg.mcrw", 0, 256, 256);
    PEX_PSP_LOAD_OPT(&tex_stat_slot, "gui_slot.mcrw", 0, 256, 256);
    PEX_PSP_LOAD_REQ(&tex_inventory, "gui_inventory.mcrw", 0, 256, 256);
    PEX_PSP_LOAD_OPT(&tex_allitems, "gui_allitems.mcrw", 0, 256, 256);
    PEX_PSP_LOAD_OPT(&tex_workbench, "gui_crafting_table.mcrw", 0, 256, 256);
    PEX_PSP_LOAD_OPT(&tex_furnace_gui, "gui_furnace.mcrw", 0, 256, 256);
    PEX_PSP_LOAD_OPT(&tex_chest_gui, "gui_chest.mcrw", 0, 256, 256);
    free_texture(&tex_chest_entity);
    free_texture(&tex_large_chest_entity);
    free_texture(&tex_mob_pig);
    free_texture(&tex_mob_sheep);
    free_texture(&tex_mob_sheep_fur);
    free_texture(&tex_mob_cow);
    free_texture(&tex_mob_chicken);
    free_texture(&tex_mob_saddle);
    PEX_PSP_LOAD_OPT(&tex_items, "gui_items.mcrw", 0, 256, 256);
    free_texture(&tex_xporb);
    PEX_PSP_LOAD_REQ(&tex_steve, "mob_char.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[0][0], "armor_cloth_1.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[0][1], "armor_cloth_2.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[1][0], "armor_chain_1.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[1][1], "armor_chain_2.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[2][0], "armor_iron_1.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[2][1], "armor_iron_2.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[3][0], "armor_diamond_1.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[3][1], "armor_diamond_2.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[4][0], "armor_gold_1.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[4][1], "armor_gold_2.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_mob_pig, "mob_pig.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_mob_sheep, "mob_sheep.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_mob_sheep_fur, "mob_sheep_fur.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_mob_cow, "mob_cow.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_mob_chicken, "mob_chicken.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_mob_saddle, "mob_saddle.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_REQ(&tex_clouds, "environment_clouds.mcrw", 1, 256, 256);
    PEX_PSP_LOAD_OPT(&tex_sun, "terrain_sun.mcrw", 0, 32, 32);
    PEX_PSP_LOAD_OPT(&tex_moon, "terrain_moon.mcrw", 0, 32, 32);
    PEX_PSP_LOAD_OPT(&tex_moon_phases, "terrain_moon_phases.mcrw", 0, 64, 32);
    normalize_sky_sprite_alphas();
    /* Java RenderGlobal.renderSky uses additive alpha for sun/moon sprites. */
    PEX_PSP_LOAD_OPT(&tex_water_overlay, "misc_water.mcrw", 1, 256, 256);
    PEX_PSP_LOAD_OPT(&tex_shadow, "misc_shadow.mcrw", 0, 64, 64);
    PEX_PSP_LOAD_OPT(&tex_grasscolor, "misc_grasscolor.mcrw", 0, 256, 256);
    PEX_PSP_LOAD_OPT(&tex_foliagecolor, "misc_foliagecolor.mcrw", 0, 256, 256);
    free_texture(&tex_watercolor);
    free_texture(&tex_pinecolor);
    free_texture(&tex_birchcolor);
    free_texture(&tex_swampgrasscolor);
    free_texture(&tex_swampfoliagecolor);
    stivufine_lilypad_color = -1;
    PEX_PSP_LOAD_OPT(&tex_particles, "particles.mcrw", 0, 128, 128);
    psp_drop_gui_texture_cpu_copies();
    #if defined(PEX_PLATFORM_PSP)
    PEX_PSP_LOGF("load_default_textures PSP done: missing_required=%d embedded_count=%u free=%u", missing, psp_embedded_mcrw_count(), (unsigned)sceKernelTotalFreeMemSize());
#else
    PEX_PSP_LOGF("load_default_textures Wii done: missing_required=%d embedded_count=%u", missing, psp_embedded_mcrw_count());
#endif
#undef PEX_PSP_LOAD_REQ
#undef PEX_PSP_LOAD_OPT
    return 1;
#else
    if (load_release_textures_from_pack()) {
        if (!stivufine_defer_quality_update) stivufine_update_quality_textures(NULL);
        return 1;
    }
    int missing_required = 0;
#define PEX_LOAD_REQ(tex, file, repeat, fw, fh) do { \
        if (!load_mcrw((tex), (file), (repeat))) { \
            ++missing_required; \
            log_msg("Missing required asset %s; using temporary fallback so resource downloader can start", (file)); \
            pex_make_fallback_texture((tex), (file), (fw), (fh), (repeat)); \
        } \
    } while (0)
#define PEX_LOAD_OPT(tex, file, repeat, fw, fh) do { \
        if (!load_mcrw((tex), (file), (repeat))) { \
            pex_make_fallback_texture((tex), (file), (fw), (fh), (repeat)); \
        } \
    } while (0)
    PEX_LOAD_REQ(&tex_bg, "gui_background.mcrw", 1, 16, 16);
    PEX_LOAD_REQ(&tex_gui, "gui_gui.mcrw", 0, 256, 256);
    PEX_LOAD_REQ(&tex_font, "font_default.mcrw", 0, 128, 128);
    PEX_LOAD_REQ(&tex_terrain, "terrain.mcrw", 1, 256, 256);
    normalize_terrain_liquid_tiles();
    PEX_LOAD_REQ(&tex_black, "title_black.mcrw", 1, 16, 16);
    PEX_LOAD_OPT(&tex_pack, "pack.mcrw", 0, 32, 32);
    PEX_LOAD_OPT(&tex_default_pack_icon, "pack.mcrw", 0, 32, 32);
    PEX_LOAD_OPT(&tex_unknown_pack, "unknown_pack.mcrw", 0, 32, 32);
    PEX_LOAD_REQ(&tex_icons, "gui_icons.mcrw", 0, 256, 256);
    PEX_LOAD_OPT(&tex_achievement, "achievement_bg.mcrw", 0, 256, 256);
    PEX_LOAD_OPT(&tex_stat_slot, "gui_slot.mcrw", 0, 256, 256);
    PEX_LOAD_REQ(&tex_inventory, "gui_inventory.mcrw", 0, 256, 256);
    PEX_LOAD_OPT(&tex_allitems, "gui_allitems.mcrw", 0, 256, 256);
    PEX_LOAD_OPT(&tex_workbench, "gui_crafting_table.mcrw", 0, 256, 256);
    PEX_LOAD_OPT(&tex_furnace_gui, "gui_furnace.mcrw", 0, 256, 256);
    PEX_LOAD_OPT(&tex_chest_gui, "gui_chest.mcrw", 0, 256, 256);
    free_texture(&tex_chest_entity);
    free_texture(&tex_large_chest_entity);
    free_texture(&tex_mob_pig);
    free_texture(&tex_mob_sheep);
    free_texture(&tex_mob_sheep_fur);
    free_texture(&tex_mob_cow);
    free_texture(&tex_mob_chicken);
    free_texture(&tex_mob_saddle);
    for (int m = 0; m < 5; m++) for (int l = 0; l < 2; l++) free_texture(&tex_armor[m][l]);
    PEX_LOAD_OPT(&tex_items, "gui_items.mcrw", 0, 256, 256);
    free_texture(&tex_xporb);
    free_texture(&tex_arrows);
    PEX_LOAD_REQ(&tex_steve, "mob_char.mcrw", 0, 64, 32);
    PEX_LOAD_OPT(&tex_mob_pig, "mob_pig.mcrw", 0, 64, 32);
    PEX_LOAD_OPT(&tex_mob_sheep, "mob_sheep.mcrw", 0, 64, 32);
    PEX_LOAD_OPT(&tex_mob_sheep_fur, "mob_sheep_fur.mcrw", 0, 64, 32);
    PEX_LOAD_OPT(&tex_mob_cow, "mob_cow.mcrw", 0, 64, 32);
    PEX_LOAD_OPT(&tex_mob_chicken, "mob_chicken.mcrw", 0, 64, 32);
    PEX_LOAD_OPT(&tex_mob_saddle, "mob_saddle.mcrw", 0, 64, 32);
    PEX_LOAD_OPT(&tex_clouds, "environment_clouds.mcrw", 1, 256, 256);
    PEX_LOAD_OPT(&tex_sun, "terrain_sun.mcrw", 0, 32, 32);
    PEX_LOAD_OPT(&tex_moon, "terrain_moon.mcrw", 0, 32, 32);
    PEX_LOAD_OPT(&tex_moon_phases, "terrain_moon_phases.mcrw", 0, 64, 32);
    normalize_sky_sprite_alphas();
    PEX_LOAD_OPT(&tex_water_overlay, "misc_water.mcrw", 1, 256, 256);
    PEX_LOAD_OPT(&tex_shadow, "misc_shadow.mcrw", 0, 64, 64);
    PEX_LOAD_OPT(&tex_grasscolor, "misc_grasscolor.mcrw", 0, 256, 256);
    PEX_LOAD_OPT(&tex_foliagecolor, "misc_foliagecolor.mcrw", 0, 256, 256);
    free_texture(&tex_watercolor);
    free_texture(&tex_pinecolor);
    free_texture(&tex_birchcolor);
    free_texture(&tex_swampgrasscolor);
    free_texture(&tex_swampfoliagecolor);
    stivufine_lilypad_color = -1;
    PEX_LOAD_OPT(&tex_particles, "particles.mcrw", 0, 128, 128);
    if (missing_required) log_msg("Started with %d fallback required assets; showing resource download UI", missing_required);
#undef PEX_LOAD_REQ
#undef PEX_LOAD_OPT
    if (!stivufine_defer_quality_update) stivufine_update_quality_textures(NULL);
    return 1;
#endif
}


static char *stivufine_trim_ascii(char *s) {
    char *e;
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') ++s;
    e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) *--e = 0;
    return s;
}

static int stivufine_parse_rgb_hex(const char *s, int *out) {
    char buf[16];
    int n = 0;
    if (!s) return 0;
    while (*s == ' ' || *s == '\t') ++s;
    if (*s == '#') ++s;
    if ((s[0] == '0') && (s[1] == 'x' || s[1] == 'X')) s += 2;
    while (((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f') || (*s >= 'A' && *s <= 'F')) && n < 6) buf[n++] = *s++;
    if (n != 6) return 0;
    buf[n] = 0;
    *out = (int)strtol(buf, NULL, 16) & 0xFFFFFF;
    return 1;
}

static void stivufine_load_color_properties(TexturePackEntry *e) {
    char path[MAX_PATHBUF];
    FILE *f;
    char line[512];
    if (!e || e->is_default || !stivufine_custom_colors_enabled()) return;
    snprintf(path, sizeof(path), "%s/color.properties", e->path);
    f = fopen(path, "r");
    if (!f) {
        snprintf(path, sizeof(path), "%s\\color.properties", e->path);
        f = fopen(path, "r");
    }
    if (!f) return;
    while (fgets(line, sizeof(line), f)) {
        char *hash = strchr(line, '#');
        char *sep;
        char *key;
        char *val;
        if (hash) *hash = 0;
        sep = strchr(line, '=');
        if (!sep) sep = strchr(line, ':');
        if (!sep) continue;
        *sep++ = 0;
        key = stivufine_trim_ascii(line);
        val = stivufine_trim_ascii(sep);
        if (!strcmp(key, "lilypad")) {
            int rgb;
            if (stivufine_parse_rgb_hex(val, &rgb)) stivufine_lilypad_color = rgb;
        }
    }
    fclose(f);
}

static void try_pack_texture(TexturePackEntry *e, Texture *tex, const char *rel, int repeat) {
    char path[MAX_PATHBUF];
    snprintf(path, sizeof(path), "%s\\%s", e->path, rel);
    if (load_png_texture(tex, path, repeat)) return;
    snprintf(path, sizeof(path), "%s/%s", e->path, rel);
    for (char *c = path; *c; ++c) if (*c == '\\') *c = '/';
    load_png_texture(tex, path, repeat);
}

#define SF_RANDOM_MOB_MAX_TEXTURE_NUMBER 1000

typedef struct StivuFineRandomMobTextureSet {
    Texture *base;
    const char *rel;
    Texture *variants;
    int variant_count;
    int variant_capacity;
} StivuFineRandomMobTextureSet;

static StivuFineRandomMobTextureSet stivufine_random_mob_sets[] = {
    { &tex_mob_pig, "mob\\pig.png", NULL, 0, 0 },
    { &tex_mob_sheep, "mob\\sheep.png", NULL, 0, 0 },
    { &tex_mob_cow, "mob\\cow.png", NULL, 0, 0 },
    { &tex_mob_chicken, "mob\\chicken.png", NULL, 0, 0 },
    { &tex_mob_creeper, "mob\\creeper.png", NULL, 0, 0 },
    { &tex_mob_skeleton, "mob\\skeleton.png", NULL, 0, 0 },
    { &tex_mob_spider, "mob\\spider.png", NULL, 0, 0 },
    { &tex_mob_zombie, "mob\\zombie.png", NULL, 0, 0 },
    { &tex_mob_slime, "mob\\slime.png", NULL, 0, 0 },
    { &tex_mob_ghast, "mob\\ghast.png", NULL, 0, 0 },
    { &tex_mob_ghast_fire, "mob\\ghast_fire.png", NULL, 0, 0 },
    { &tex_mob_pigzombie, "mob\\pigzombie.png", NULL, 0, 0 },
    { &tex_mob_enderman, "mob\\enderman.png", NULL, 0, 0 },
    { &tex_mob_cavespider, "mob\\cavespider.png", NULL, 0, 0 },
    { &tex_mob_silverfish, "mob\\silverfish.png", NULL, 0, 0 },
    { &tex_mob_blaze, "mob\\fire.png", NULL, 0, 0 },
    { &tex_mob_lava, "mob\\lava.png", NULL, 0, 0 },
    { &tex_mob_enderdragon, "mob\\enderdragon\\ender.png", NULL, 0, 0 },
    { &tex_mob_squid, "mob\\squid.png", NULL, 0, 0 },
    { &tex_mob_wolf, "mob\\wolf.png", NULL, 0, 0 },
    { &tex_mob_wolf_tame, "mob\\wolf_tame.png", NULL, 0, 0 },
    { &tex_mob_wolf_angry, "mob\\wolf_angry.png", NULL, 0, 0 },
    { &tex_mob_mooshroom, "mob\\redcow.png", NULL, 0, 0 },
    { &tex_mob_snowman, "mob\\snowman.png", NULL, 0, 0 },
    { &tex_mob_ocelot, "mob\\ozelot.png", NULL, 0, 0 },
    { &tex_mob_cat_black, "mob\\cat_black.png", NULL, 0, 0 },
    { &tex_mob_cat_red, "mob\\cat_red.png", NULL, 0, 0 },
    { &tex_mob_cat_siamese, "mob\\cat_siamese.png", NULL, 0, 0 },
    { &tex_mob_villager_golem, "mob\\villager_golem.png", NULL, 0, 0 },
    { &tex_mob_villager, "mob\\villager\\villager.png", NULL, 0, 0 },
    { &tex_mob_villager_farmer, "mob\\villager\\farmer.png", NULL, 0, 0 },
    { &tex_mob_villager_librarian, "mob\\villager\\librarian.png", NULL, 0, 0 },
    { &tex_mob_villager_priest, "mob\\villager\\priest.png", NULL, 0, 0 },
    { &tex_mob_villager_smith, "mob\\villager\\smith.png", NULL, 0, 0 },
    { &tex_mob_villager_butcher, "mob\\villager\\butcher.png", NULL, 0, 0 },
};

static void stivufine_clear_random_mob_variants(void) {
    size_t n = sizeof(stivufine_random_mob_sets) / sizeof(stivufine_random_mob_sets[0]);
    for (size_t i = 0; i < n; ++i) {
        StivuFineRandomMobTextureSet *set = &stivufine_random_mob_sets[i];
        for (int j = 0; j < set->variant_count; ++j) free_texture(&set->variants[j]);
        free(set->variants);
        set->variants = NULL;
        set->variant_count = 0;
        set->variant_capacity = 0;
    }
}

static int stivufine_try_load_pack_texture(TexturePackEntry *e, Texture *tex, const char *rel, int repeat) {
    char path[MAX_PATHBUF];
    snprintf(path, sizeof(path), "%s\\%s", e->path, rel);
    if (load_png_texture(tex, path, repeat)) return 1;
    snprintf(path, sizeof(path), "%s/%s", e->path, rel);
    for (char *c = path; *c; ++c) if (*c == '\\') *c = '/';
    return load_png_texture(tex, path, repeat);
}

static void stivufine_make_random_variant_rel(char *out, size_t cap, const char *rel, int num) {
    const char *dot = strrchr(rel, '.');
    if (!dot) { snprintf(out, cap, "%s%d", rel, num); return; }
    size_t prefix = (size_t)(dot - rel);
    if (prefix >= cap) prefix = cap - 1;
    memcpy(out, rel, prefix);
    out[prefix] = 0;
    snprintf(out + prefix, cap - prefix, "%d%s", num, dot);
}

static int stivufine_random_mob_append(StivuFineRandomMobTextureSet *set, Texture *loaded) {
    if (set->variant_count >= set->variant_capacity) {
        int next = set->variant_capacity ? set->variant_capacity * 2 : 4;
        if (next > SF_RANDOM_MOB_MAX_TEXTURE_NUMBER - 1) next = SF_RANDOM_MOB_MAX_TEXTURE_NUMBER - 1;
        Texture *grown = (Texture*)realloc(set->variants, (size_t)next * sizeof(Texture));
        if (!grown) return 0;
        memset(grown + set->variant_capacity, 0, (size_t)(next - set->variant_capacity) * sizeof(Texture));
        set->variants = grown;
        set->variant_capacity = next;
    }
    set->variants[set->variant_count++] = *loaded;
    memset(loaded, 0, sizeof(*loaded));
    return 1;
}

static void stivufine_load_random_mob_variants(TexturePackEntry *e) {
    stivufine_clear_random_mob_variants();
    if (!e || e->is_default || !stivufine_random_mobs_enabled()) return;
    size_t n = sizeof(stivufine_random_mob_sets) / sizeof(stivufine_random_mob_sets[0]);
    for (size_t i = 0; i < n; ++i) {
        StivuFineRandomMobTextureSet *set = &stivufine_random_mob_sets[i];
        /* Reference behavior scans numbered files sequentially and stops at the
           first missing entry, with a maximum texture number of 1000. */
        for (int suffix = 2; suffix <= SF_RANDOM_MOB_MAX_TEXTURE_NUMBER; ++suffix) {
            char rel[MAX_PATHBUF];
            Texture loaded = {0};
            stivufine_make_random_variant_rel(rel, sizeof(rel), set->rel, suffix);
            if (!stivufine_try_load_pack_texture(e, &loaded, rel, 0)) break;
            if (!stivufine_random_mob_append(set, &loaded)) { free_texture(&loaded); break; }
        }
    }
}

static unsigned int stivufine_java_decimal_string_hash_text(const char *text) {
    uint32_t h = 0;
    for (const unsigned char *p = (const unsigned char*)text; *p; ++p) h = h * 31u + (uint32_t)*p;
    /* Java Math.abs(int), expressed without signed-overflow UB. Persistent IDs
       are non-negative, and this returns the same positive magnitude used by RandomMobs. */
    if (h & 0x80000000u) h = (~h) + 1u;
    return h;
}

static Texture *stivufine_random_mob_texture_for_base(Texture *base, int random_id) {
    char skin_url[24];
    if (!base || !stivufine_random_mobs_enabled()) return base;
    snprintf(skin_url, sizeof(skin_url), "%d", random_id);
    if (strlen(skin_url) <= 1 || skin_url[0] < '0' || skin_url[0] > '9') return base;
    size_t n = sizeof(stivufine_random_mob_sets) / sizeof(stivufine_random_mob_sets[0]);
    unsigned int num = stivufine_java_decimal_string_hash_text(skin_url);
    for (size_t i = 0; i < n; ++i) {
        StivuFineRandomMobTextureSet *set = &stivufine_random_mob_sets[i];
        if (set->base != base || set->variant_count <= 0) continue;
        int pick = (int)(num % (unsigned int)(set->variant_count + 1));
        if (pick == 0) return base;
        Texture *v = &set->variants[pick - 1];
        return (v && v->id) ? v : base;
    }
    return base;
}

static void apply_texture_pack_index(int index) {
    if (index < 0 || index >= g_texpack_count) index = 0;
    free_texture(&tex_mob_creeper_power);
    stivufine_defer_quality_update = 1;
    int default_ok = load_default_textures();
    stivufine_defer_quality_update = 0;
    if (!default_ok) return;
    free_texture(&tex_watercolor);
    free_texture(&tex_pinecolor);
    free_texture(&tex_birchcolor);
    free_texture(&tex_swampgrasscolor);
    free_texture(&tex_swampfoliagecolor);
    stivufine_lilypad_color = -1;
    g_selected_texpack = index;
    snprintf(g_current_texpack, sizeof(g_current_texpack), "%s", g_texpacks[index].name);
    snprintf(g_opts.skin, sizeof(g_opts.skin), "%s", g_texpacks[index].name);
    save_options();
    if (!g_texpacks[index].is_default) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
        /* Console builds already use CI-converted Classic .mcrw assets as their
           default embedded textures.  No PNG texturepack files are needed. */
        init_font_widths();
        log_msg("Applied embedded console texture pack: %s", g_current_texpack);
        return;
#else
        TexturePackEntry *e = &g_texpacks[index];
        try_pack_texture(e, &tex_terrain, "terrain.png", 1);
        normalize_terrain_liquid_tiles();
        try_pack_texture(e, &tex_gui, "gui\\gui.png", 0);
        try_pack_texture(e, &tex_bg, "gui\\background.png", 1);
        if (stivufine_custom_fonts_enabled())
            try_pack_texture(e, &tex_font, "font\\default.png", 0);
        try_pack_texture(e, &tex_pack, "pack.png", 0);
        try_pack_texture(e, &tex_icons, "gui\\icons.png", 0);
        try_pack_texture(e, &tex_achievement, "achievement\\bg.png", 0);
        try_pack_texture(e, &tex_stat_slot, "gui\\slot.png", 0);
        try_pack_texture(e, &tex_inventory, "gui\\inventory.png", 0);
        try_pack_texture(e, &tex_allitems, "gui\\allitems.png", 0);
        try_pack_texture(e, &tex_workbench, "gui\\crafting.png", 0);
        try_pack_texture(e, &tex_furnace_gui, "gui\\furnace.png", 0);
        try_pack_texture(e, &tex_chest_gui, "gui\\container.png", 0);
        try_pack_texture(e, &tex_chest_gui, "gui\\chest.png", 0);
        try_pack_texture(e, &tex_chest_entity, "item\\chest.png", 0);
        try_pack_texture(e, &tex_arrows, "item\\arrows.png", 0);
        try_pack_texture(e, &tex_large_chest_entity, "item\\largechest.png", 0);
        try_pack_texture(e, &tex_items, "gui\\items.png", 0);
        try_pack_texture(e, &tex_armor[0][0], "armor\\cloth_1.png", 0);
        try_pack_texture(e, &tex_armor[0][1], "armor\\cloth_2.png", 0);
        try_pack_texture(e, &tex_armor[1][0], "armor\\chain_1.png", 0);
        try_pack_texture(e, &tex_armor[1][1], "armor\\chain_2.png", 0);
        try_pack_texture(e, &tex_armor[2][0], "armor\\iron_1.png", 0);
        try_pack_texture(e, &tex_armor[2][1], "armor\\iron_2.png", 0);
        try_pack_texture(e, &tex_armor[3][0], "armor\\diamond_1.png", 0);
        try_pack_texture(e, &tex_armor[3][1], "armor\\diamond_2.png", 0);
        try_pack_texture(e, &tex_armor[4][0], "armor\\gold_1.png", 0);
        try_pack_texture(e, &tex_armor[4][1], "armor\\gold_2.png", 0);
        try_pack_texture(e, &tex_steve, "mob\\char.png", 0);
        try_pack_texture(e, &tex_steve, "char.png", 0);
        try_pack_texture(e, &tex_mob_pig, "mob\\pig.png", 0);
        try_pack_texture(e, &tex_mob_sheep, "mob\\sheep.png", 0);
        try_pack_texture(e, &tex_mob_sheep_fur, "mob\\sheep_fur.png", 0);
        try_pack_texture(e, &tex_mob_cow, "mob\\cow.png", 0);
        try_pack_texture(e, &tex_mob_chicken, "mob\\chicken.png", 0);
        try_pack_texture(e, &tex_mob_saddle, "mob\\saddle.png", 0);
        try_pack_texture(e, &tex_mob_creeper, "mob\\creeper.png", 0);
        try_pack_texture(e, &tex_mob_creeper_power, "armor\\power.png", 1);
        try_pack_texture(e, &tex_mob_skeleton, "mob\\skeleton.png", 0);
        try_pack_texture(e, &tex_mob_spider, "mob\\spider.png", 0);
        try_pack_texture(e, &tex_mob_spider_eyes, "mob\\spider_eyes.png", 0);
        try_pack_texture(e, &tex_mob_zombie, "mob\\zombie.png", 0);
        try_pack_texture(e, &tex_mob_slime, "mob\\slime.png", 0);
        try_pack_texture(e, &tex_mob_ghast, "mob\\ghast.png", 0);
        try_pack_texture(e, &tex_mob_ghast_fire, "mob\\ghast_fire.png", 0);
        try_pack_texture(e, &tex_mob_pigzombie, "mob\\pigzombie.png", 0);
        try_pack_texture(e, &tex_mob_enderman, "mob\\enderman.png", 0);
        try_pack_texture(e, &tex_mob_enderman_eyes, "mob\\enderman_eyes.png", 0);
        try_pack_texture(e, &tex_mob_cavespider, "mob\\cavespider.png", 0);
        try_pack_texture(e, &tex_mob_silverfish, "mob\\silverfish.png", 0);
        try_pack_texture(e, &tex_mob_blaze, "mob\\fire.png", 0);
        try_pack_texture(e, &tex_mob_lava, "mob\\lava.png", 0);
        try_pack_texture(e, &tex_mob_enderdragon, "mob\\enderdragon\\ender.png", 0);
        try_pack_texture(e, &tex_mob_squid, "mob\\squid.png", 0);
        try_pack_texture(e, &tex_mob_wolf, "mob\\wolf.png", 0);
        try_pack_texture(e, &tex_mob_wolf_tame, "mob\\wolf_tame.png", 0);
        try_pack_texture(e, &tex_mob_wolf_angry, "mob\\wolf_angry.png", 0);
        try_pack_texture(e, &tex_mob_mooshroom, "mob\\redcow.png", 0);
        try_pack_texture(e, &tex_mob_snowman, "mob\\snowman.png", 0);
        try_pack_texture(e, &tex_mob_ocelot, "mob\\ozelot.png", 0);
        try_pack_texture(e, &tex_mob_cat_black, "mob\\cat_black.png", 0);
        try_pack_texture(e, &tex_mob_cat_red, "mob\\cat_red.png", 0);
        try_pack_texture(e, &tex_mob_cat_siamese, "mob\\cat_siamese.png", 0);
        try_pack_texture(e, &tex_mob_villager_golem, "mob\\villager_golem.png", 0);
        try_pack_texture(e, &tex_mob_villager, "mob\\villager\\villager.png", 0);
        try_pack_texture(e, &tex_mob_villager_farmer, "mob\\villager\\farmer.png", 0);
        try_pack_texture(e, &tex_mob_villager_librarian, "mob\\villager\\librarian.png", 0);
        try_pack_texture(e, &tex_mob_villager_priest, "mob\\villager\\priest.png", 0);
        try_pack_texture(e, &tex_mob_villager_smith, "mob\\villager\\smith.png", 0);
        try_pack_texture(e, &tex_mob_villager_butcher, "mob\\villager\\butcher.png", 0);
        try_pack_texture(e, &tex_clouds, "environment\\clouds.png", 1);
        try_pack_texture(e, &tex_sun, "terrain\\sun.png", 0);
        try_pack_texture(e, &tex_moon, "terrain\\moon.png", 0);
        try_pack_texture(e, &tex_moon_phases, "terrain\\moon_phases.png", 0);
        normalize_sky_sprite_alphas();
        /* Java RenderGlobal.renderSky uses additive alpha for sun/moon sprites. */
        try_pack_texture(e, &tex_water_overlay, "misc\\water.png", 1);
        try_pack_texture(e, &tex_shadow, "misc\\shadow.png", 0);
        if (stivufine_custom_colors_enabled()) {
            try_pack_texture(e, &tex_grasscolor, "misc\\grasscolor.png", 0);
            try_pack_texture(e, &tex_foliagecolor, "misc\\foliagecolor.png", 0);
            try_pack_texture(e, &tex_watercolor, "misc\\watercolorX.png", 0);
            try_pack_texture(e, &tex_pinecolor, "misc\\pinecolor.png", 0);
            try_pack_texture(e, &tex_birchcolor, "misc\\birchcolor.png", 0);
            try_pack_texture(e, &tex_swampgrasscolor, "misc\\swampgrasscolor.png", 0);
            try_pack_texture(e, &tex_swampfoliagecolor, "misc\\swampfoliagecolor.png", 0);
        }
#endif
    }
    stivufine_update_quality_textures(&g_texpacks[index]);
    stivufine_load_random_mob_variants(&g_texpacks[index]);
    if (g_opts.skin_path[0]) load_custom_skin_path(g_opts.skin_path, 0);
    init_font_widths();
    log_msg("Applied texture pack: %s", g_current_texpack);
}
