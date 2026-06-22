/* Nintendo Wii GX fixed-function renderer shim.
   This is a first-pass native renderer: it keeps the old OpenGL-shaped calls
   alive but emits immediate/dynamic geometry through GX. */

#define PEX_WII_MAX_TEXTURES 1024
#define PEX_WII_MAX_MESHES 4096
#define PEX_WII_MAX_IMM_VERTS 32768
#define PEX_WII_LIST_COUNT 1024
#define PEX_WII_FIFO_SIZE (256 * 1024)

#ifndef GX_FALSE
#define GX_FALSE 0
#endif
#ifndef GX_TRUE
#define GX_TRUE 1
#endif

typedef struct WiiImmVertex { float x, y, z, u, v; uint32_t color; } WiiImmVertex;
typedef struct WiiTexture { int used; int w, h, tw, th; int repeat_s, repeat_t; void *pixels; GXTexObj obj; } WiiTexture;
typedef struct WiiMeshSlot { PexVertex *vertices; uint32_t *indices; uint32_t vertex_count, index_count; int dynamic; } WiiMeshSlot;
typedef struct WiiBatch {
    WiiImmVertex *v;
    int count;
    GLenum mode;
    GLuint tex;
    int texture_enabled, blend_enabled, depth_enabled, depth_write, alpha_enabled, cull_enabled;
    struct WiiBatch *next;
} WiiBatch;
typedef struct WiiList { WiiBatch *first, *last; int batch_count; } WiiList;

static void *g_wii_fifo = NULL;
static void *g_wii_xfb[2] = { NULL, NULL };
static int g_wii_fb = 0;
static GXRModeObj *g_wii_rmode = NULL;
static int g_wii_screen_w = 640;
static int g_wii_screen_h = 480;
static int g_wii_render_w = 640;
static int g_wii_render_h = 480;

static WiiTexture g_wii_textures[PEX_WII_MAX_TEXTURES];
static GLuint g_wii_next_tex = 1;
static GLuint g_wii_bound_tex = 0;
static WiiMeshSlot g_wii_meshes[PEX_WII_MAX_MESHES];
static PexRendererStats g_wii_stats;

static int g_wii_texture_enabled = 1;
static int g_wii_blend_enabled = 1;
static int g_wii_depth_enabled = 1;
static int g_wii_depth_write = 1;
static int g_wii_alpha_enabled = 0;
static int g_wii_cull_enabled = 0;
static float g_wii_clear_r = 0, g_wii_clear_g = 0, g_wii_clear_b = 0, g_wii_clear_a = 1;
static uint32_t g_wii_cur_color = 0xffffffffu;
static float g_wii_cur_u = 0.0f, g_wii_cur_v = 0.0f;
static int g_wii_vertex_array_enabled = 0;
static int g_wii_texcoord_array_enabled = 0;
static int g_wii_color_array_enabled = 0;
static GLint g_wii_vertex_size = 0, g_wii_texcoord_size = 0, g_wii_color_size = 0;
static GLenum g_wii_vertex_type = 0, g_wii_texcoord_type = 0, g_wii_color_type = 0;
static GLsizei g_wii_vertex_stride = 0, g_wii_texcoord_stride = 0, g_wii_color_stride = 0;
static const GLvoid *g_wii_vertex_ptr = NULL, *g_wii_texcoord_ptr = NULL, *g_wii_color_ptr = NULL;
static GLenum g_wii_begin_mode = 0;
static int g_wii_begin_active = 0;
static WiiImmVertex g_wii_imm[PEX_WII_MAX_IMM_VERTS];
static int g_wii_imm_count = 0;
static GLint g_wii_viewport[4] = {0,0,640,480};
static GLenum g_wii_matrix_mode = GL_MODELVIEW;
static float g_wii_modelview[16];
static float g_wii_projection[16];
static float g_wii_modelview_stack[32][16];
static int g_wii_modelview_sp = 0;
static float g_wii_projection_stack[16][16];
static int g_wii_projection_sp = 0;
static int g_wii_projection_is_ortho = 0;

static WiiList g_wii_lists[PEX_WII_LIST_COUNT];
static GLuint g_wii_next_list = 1;
static int g_wii_compiling = 0;
static GLuint g_wii_compile_list = 0;

static void wii_mat_identity(float m[16]) { memset(m, 0, sizeof(float) * 16); m[0]=m[5]=m[10]=m[15]=1.0f; }
static void wii_mat_copy(float dst[16], const float src[16]) { memcpy(dst, src, sizeof(float) * 16); }
static float *wii_current_matrix(void) { return (g_wii_matrix_mode == GL_PROJECTION) ? g_wii_projection : g_wii_modelview; }
static void wii_mat_mul(float out[16], const float a[16], const float b[16]) {
    float r[16];
    for (int c = 0; c < 4; ++c) for (int row = 0; row < 4; ++row) {
        r[c*4 + row] = a[0*4 + row] * b[c*4 + 0] + a[1*4 + row] * b[c*4 + 1] + a[2*4 + row] * b[c*4 + 2] + a[3*4 + row] * b[c*4 + 3];
    }
    memcpy(out, r, sizeof(r));
}
static void wii_mat_postmul(float m[16], const float rhs[16]) { float r[16]; wii_mat_mul(r, m, rhs); wii_mat_copy(m, r); }
static void wii_mat_translate(float x, float y, float z) { float t[16]; wii_mat_identity(t); t[12]=x; t[13]=y; t[14]=z; wii_mat_postmul(wii_current_matrix(), t); }
static void wii_mat_scale(float x, float y, float z) { float t[16]; wii_mat_identity(t); t[0]=x; t[5]=y; t[10]=z; wii_mat_postmul(wii_current_matrix(), t); }
static void wii_mat_rotate(float angle, float x, float y, float z) {
    float len = sqrtf(x*x + y*y + z*z); if (len <= 0.00001f) return; x/=len; y/=len; z/=len;
    float a = angle * (float)M_PI / 180.0f, c = cosf(a), s = sinf(a), ic = 1.0f - c;
    float r[16]; wii_mat_identity(r);
    r[0]=x*x*ic+c;   r[4]=x*y*ic-z*s; r[8]=x*z*ic+y*s;
    r[1]=y*x*ic+z*s; r[5]=y*y*ic+c;   r[9]=y*z*ic-x*s;
    r[2]=z*x*ic-y*s; r[6]=z*y*ic+x*s; r[10]=z*z*ic+c;
    wii_mat_postmul(wii_current_matrix(), r);
}
static void wii_set_ortho(double l,double r,double b,double t,double n,double f) {
    float *m = wii_current_matrix(); wii_mat_identity(m);
    m[0] = (float)(2.0 / (r - l));
    m[5] = (float)(2.0 / (t - b));
    m[10] = (float)(-2.0 / (f - n));
    m[12] = (float)(-(r + l) / (r - l));
    m[13] = (float)(-(t + b) / (t - b));
    m[14] = (float)(-(f + n) / (f - n));
    if (g_wii_matrix_mode == GL_PROJECTION) g_wii_projection_is_ortho = 1;
}
static void wii_set_perspective(double fovy,double aspect,double znear,double zfar) {
    float *m = wii_current_matrix(); wii_mat_identity(m);
    double f = 1.0 / tan((fovy * M_PI / 180.0) * 0.5);
    m[0] = (float)(f / aspect);
    m[5] = (float)f;
    m[10] = (float)((zfar + znear) / (znear - zfar));
    m[11] = -1.0f;
    m[14] = (float)((2.0 * zfar * znear) / (znear - zfar));
    m[15] = 0.0f;
    if (g_wii_matrix_mode == GL_PROJECTION) g_wii_projection_is_ortho = 0;
}
static void wii_load_gx_matrices_from(const float mv16[16], const float pr16[16], int ortho) {
    Mtx mv;
    Mtx44 pr;
    for (int row = 0; row < 3; ++row) for (int c = 0; c < 4; ++c) mv[row][c] = mv16[c*4 + row];
    for (int row = 0; row < 4; ++row) for (int c = 0; c < 4; ++c) pr[row][c] = pr16[c*4 + row];
    GX_LoadPosMtxImm(mv, GX_PNMTX0);
    GX_LoadProjectionMtx(pr, ortho ? GX_ORTHOGRAPHIC : GX_PERSPECTIVE);
}
static void wii_load_gx_matrices(void) { wii_load_gx_matrices_from(g_wii_modelview, g_wii_projection, g_wii_projection_is_ortho); }

static uint32_t wii_color4f_pack(float r,float g,float b,float a) {
    int ri=(int)(r*255.0f+0.5f), gi=(int)(g*255.0f+0.5f), bi=(int)(b*255.0f+0.5f), ai=(int)(a*255.0f+0.5f);
    if (ri < 0) ri = 0;
    if (ri > 255) ri = 255;
    if (gi < 0) gi = 0;
    if (gi > 255) gi = 255;
    if (bi < 0) bi = 0;
    if (bi > 255) bi = 255;
    if (ai < 0) ai = 0;
    if (ai > 255) ai = 255;
    return (uint32_t)(ri | (gi<<8) | (bi<<16) | (ai<<24));
}
static void wii_emit_color(uint32_t c) { GX_Color4u8((u8)(c & 255), (u8)((c >> 8) & 255), (u8)((c >> 16) & 255), (u8)((c >> 24) & 255)); }

static GLenum wii_draw_mode_for(GLenum mode, int count, int *out_count) {
    *out_count = count;
    if (mode == GL_TRIANGLES) return GX_TRIANGLES;
    if (mode == GL_TRIANGLE_STRIP) return GX_TRIANGLESTRIP;
    if (mode == GL_TRIANGLE_FAN) return GX_TRIANGLEFAN;
    if (mode == GL_LINES) return GX_LINES;
    if (mode == GL_LINE_STRIP || mode == GL_LINE_LOOP) return GX_LINESTRIP;
    if (mode == GL_POINTS) return GX_POINTS;
    if (mode == GL_QUADS) return GX_QUADS;
    return GX_TRIANGLES;
}

/* GX_Begin takes a 16-bit vertex count.  Passing more vertices and then
   writing them anyway corrupts the GX FIFO and Dolphin reports invalid MEM1
   pointers before crashing.  Always clamp/split before GX_Begin. */
static int wii_clamp_gx_count(GLenum mode, int count) {
    if (count <= 0) return 0;
    int max = 65535;
    if (mode == GL_QUADS) max = 65532;          /* multiple of 4 */
    else if (mode == GL_TRIANGLES) max = 65535; /* multiple of 3 */
    else if (mode == GL_LINES) max = 65534;     /* multiple of 2 */
    if (count > max) count = max;
    if (mode == GL_QUADS) count = (count / 4) * 4;
    else if (mode == GL_TRIANGLES) count = (count / 3) * 3;
    else if (mode == GL_LINES) count = (count / 2) * 2;
    return count;
}

static void wii_apply_state_values(GLuint tex, int tex_enabled, int blend, int depth, int depth_write, int alpha, int cull) {
    int valid_tex = tex_enabled && tex > 0 && tex < PEX_WII_MAX_TEXTURES && g_wii_textures[tex].used;
    if (depth) GX_SetZMode(GX_TRUE, GX_LEQUAL, depth_write ? GX_TRUE : GX_FALSE);
    else GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    if (blend) GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    else GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
    GX_SetCullMode(cull ? GX_CULL_BACK : GX_CULL_NONE);
    if (alpha) GX_SetAlphaCompare(GX_GREATER, 8, GX_AOP_AND, GX_ALWAYS, 0);
    else GX_SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    if (valid_tex) {
        GX_LoadTexObj(&g_wii_textures[tex].obj, GX_TEXMAP0);
        GX_SetNumTexGens(1);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
    } else {
        GX_SetNumTexGens(0);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    }
}
static void wii_apply_state(void) { wii_apply_state_values(g_wii_bound_tex, g_wii_texture_enabled, g_wii_blend_enabled, g_wii_depth_enabled, g_wii_depth_write, g_wii_alpha_enabled, g_wii_cull_enabled); }

static unsigned int wii_xfb_phys_offset(const void *xfb) {
    return (unsigned int)((uintptr_t)xfb & 0x0fffffffu);
}

static int wii_xfb_looks_valid(const void *xfb) {
    if (!xfb) return 0;
    unsigned int off = wii_xfb_phys_offset(xfb);
    /* XFBs must sit in valid MEM1 for Dolphin/libogc video.  MEM1 is 24MiB.
       When Arena1 is exhausted, SYS_AllocateFramebuffer can effectively hand
       us an address such as 0x01931000, which Dolphin reports as an unknown
       pointer / invalid XFB texture. */
    return off < 0x01800000u;
}

static void *wii_alloc_valid_xfb(const char *name) {
    void *xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(g_wii_rmode));
    wii_debug_logf("GX %s allocation: ptr=%p phys=%08x valid=%d", name ? name : "xfb", xfb, wii_xfb_phys_offset(xfb), wii_xfb_looks_valid(xfb));
    if (!wii_xfb_looks_valid(xfb)) return NULL;
    return xfb;
}

static void wii_emit_imm_vertex(const WiiImmVertex *v) { GX_Position3f32(v->x, v->y, v->z); wii_emit_color(v->color); GX_TexCoord2f32(v->u, v->v); }
static void wii_draw_imm_vertices(const WiiImmVertex *v, int count, GLenum mode) {
    if (!v || count <= 0) return;
    int draw_count = 0; GLenum gxmode = wii_draw_mode_for(mode, count, &draw_count);
    draw_count = wii_clamp_gx_count(mode, draw_count);
    if (draw_count <= 0) return;
    wii_load_gx_matrices();
    GX_Begin((u8)gxmode, GX_VTXFMT0, (u16)draw_count);
    for (int i = 0; i < draw_count; ++i) wii_emit_imm_vertex(&v[i]);
    GX_End();
    g_wii_stats.draw_calls++;
    if (mode == GL_QUADS) g_wii_stats.triangles += (uint32_t)((draw_count / 4) * 2);
    else if (mode == GL_TRIANGLES) g_wii_stats.triangles += (uint32_t)(draw_count / 3);
}

static void wii_batch_destroy(WiiBatch *b) { while (b) { WiiBatch *n = b->next; free(b->v); free(b); b = n; } }
static void wii_list_free(GLuint list) { if (list == 0 || list >= PEX_WII_LIST_COUNT) return; wii_batch_destroy(g_wii_lists[list].first); memset(&g_wii_lists[list], 0, sizeof(g_wii_lists[list])); }
static WiiBatch *wii_batch_create(const WiiImmVertex *src, int count, GLenum mode) {
    if (!src || count <= 0) return NULL;
    WiiBatch *b = (WiiBatch*)calloc(1, sizeof(WiiBatch)); if (!b) return NULL;
    int out_count = count;
    if (mode == GL_QUADS) out_count = (count / 4) * 4;
    b->v = (WiiImmVertex*)memalign(32, (size_t)out_count * sizeof(WiiImmVertex));
    if (!b->v) { free(b); return NULL; }
    memcpy(b->v, src, (size_t)out_count * sizeof(WiiImmVertex));
    DCFlushRange(b->v, (u32)((size_t)out_count * sizeof(WiiImmVertex)));
    b->count = out_count; b->mode = mode; b->tex = g_wii_bound_tex;
    b->texture_enabled = g_wii_texture_enabled; b->blend_enabled = g_wii_blend_enabled; b->depth_enabled = g_wii_depth_enabled; b->depth_write = g_wii_depth_write; b->alpha_enabled = g_wii_alpha_enabled; b->cull_enabled = g_wii_cull_enabled;
    return b;
}
static void wii_draw_batch(WiiBatch *b) { if (!b) return; wii_apply_state_values(b->tex, b->texture_enabled, b->blend_enabled, b->depth_enabled, b->depth_write, b->alpha_enabled, b->cull_enabled); wii_draw_imm_vertices(b->v, b->count, b->mode); }

static int wii_gx_init(void) {
    wii_debug_logf("GX init: VIDEO_Init");
    VIDEO_Init();
    g_wii_rmode = VIDEO_GetPreferredMode(NULL);
    if (!g_wii_rmode) { wii_debug_logf("GX init failed: VIDEO_GetPreferredMode returned NULL"); return 0; }
    wii_debug_logf("GX mode: fb=%d efb=%d xfb=%d vi=%d aa=%d field=%d",
                   (int)g_wii_rmode->fbWidth, (int)g_wii_rmode->efbHeight,
                   (int)g_wii_rmode->xfbHeight, (int)g_wii_rmode->viHeight,
                   (int)g_wii_rmode->aa, (int)g_wii_rmode->field_rendering);
    /* Reuse the early debug console XFB instead of allocating three framebuffers.
       The earlier build allocated a console XFB plus two GX XFBs after the
       game BSS; on Dolphin that pushed the second/third framebuffer past
       MEM1 and produced invalid address 0x01931000 warnings. */
    if (g_wii_debug_xfb && wii_xfb_looks_valid(g_wii_debug_xfb)) {
        g_wii_xfb[0] = g_wii_debug_xfb;
        wii_debug_logf("GX using debug XFB0: ptr=%p phys=%08x", g_wii_xfb[0], wii_xfb_phys_offset(g_wii_xfb[0]));
    } else {
        g_wii_xfb[0] = wii_alloc_valid_xfb("xfb0");
    }
    g_wii_xfb[1] = wii_alloc_valid_xfb("xfb1");
    if (!g_wii_xfb[0]) { wii_debug_logf("GX init failed: no valid XFB0"); return 0; }
    if (!g_wii_xfb[1]) {
        /* Single-buffer fallback is better than handing Dolphin an invalid
           framebuffer pointer.  It may flicker slightly but avoids crashing. */
        g_wii_xfb[1] = g_wii_xfb[0];
        wii_debug_logf("GX warning: using single-buffer XFB fallback");
    }
    wii_debug_logf("GX XFBs: %p phys=%08x / %p phys=%08x", g_wii_xfb[0], wii_xfb_phys_offset(g_wii_xfb[0]), g_wii_xfb[1], wii_xfb_phys_offset(g_wii_xfb[1]));
    VIDEO_Configure(g_wii_rmode);
    VIDEO_SetNextFramebuffer(g_wii_xfb[0]);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (g_wii_rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    g_wii_fifo = memalign(32, PEX_WII_FIFO_SIZE);
    if (!g_wii_fifo) { wii_debug_logf("GX init failed: FIFO allocation %u bytes", (unsigned)PEX_WII_FIFO_SIZE); return 0; }
    memset(g_wii_fifo, 0, PEX_WII_FIFO_SIZE);
    wii_debug_logf("GX FIFO: %p size=%u", g_wii_fifo, (unsigned)PEX_WII_FIFO_SIZE);
    GX_Init(g_wii_fifo, PEX_WII_FIFO_SIZE);
    g_wii_screen_w = (int)g_wii_rmode->fbWidth;
    g_wii_screen_h = (int)g_wii_rmode->efbHeight;
    g_wii_render_w = (int)g_wii_rmode->fbWidth;
    g_wii_render_h = (int)g_wii_rmode->efbHeight;
    g_wii_viewport[0] = 0; g_wii_viewport[1] = 0; g_wii_viewport[2] = g_wii_render_w; g_wii_viewport[3] = g_wii_render_h;
    GX_SetViewport(0, 0, (f32)g_wii_render_w, (f32)g_wii_render_h, 0.0f, 1.0f);
    GX_SetScissor(0, 0, (u32)g_wii_render_w, (u32)g_wii_render_h);
    GX_SetDispCopySrc(0, 0, (u16)g_wii_render_w, (u16)g_wii_render_h);
    GX_SetDispCopyDst((u16)g_wii_rmode->fbWidth, (u16)g_wii_rmode->xfbHeight);
    GX_SetCopyFilter(g_wii_rmode->aa, g_wii_rmode->sample_pattern, GX_TRUE, g_wii_rmode->vfilter);
    GX_SetFieldMode(g_wii_rmode->field_rendering, ((g_wii_rmode->viHeight == 2 * g_wii_rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
    GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
    GX_SetCopyClear((GXColor){0,0,0,255}, 0x00ffffff);
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHTNULL, GX_DF_NONE, GX_AF_NONE);
    GX_SetNumTevStages(1);
    GX_InvalidateTexAll();
    wii_mat_identity(g_wii_modelview); wii_mat_identity(g_wii_projection);
    wii_debug_logf("GX init OK render=%dx%d", g_wii_render_w, g_wii_render_h);
    return 1;
}
static void wii_gx_shutdown(void) { for (int i=1;i<PEX_WII_MAX_TEXTURES;i++) if(g_wii_textures[i].used){ free(g_wii_textures[i].pixels); memset(&g_wii_textures[i],0,sizeof(g_wii_textures[i])); } for (int i=1;i<PEX_WII_MAX_MESHES;i++){ free(g_wii_meshes[i].vertices); free(g_wii_meshes[i].indices); } for (int i=1;i<PEX_WII_LIST_COUNT;i++) wii_list_free((GLuint)i); free(g_wii_fifo); g_wii_fifo = NULL; }
static void wii_gx_begin_frame(void) {
    memset(&g_wii_stats, 0, sizeof(g_wii_stats));
    GXColor clear = {(u8)(g_wii_clear_r*255.0f), (u8)(g_wii_clear_g*255.0f), (u8)(g_wii_clear_b*255.0f), (u8)(g_wii_clear_a*255.0f)};
    GX_SetCopyClear(clear, 0x00ffffff);
    GX_SetViewport((f32)g_wii_viewport[0], (f32)g_wii_viewport[1], (f32)g_wii_viewport[2], (f32)g_wii_viewport[3], 0.0f, 1.0f);
    GX_SetScissor(0, 0, (u32)g_wii_render_w, (u32)g_wii_render_h);
}
static void wii_gx_end_frame(void) {
    static int s_present_count = 0;
    GX_DrawDone();
    g_wii_fb ^= 1;
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);
    GX_CopyDisp(g_wii_xfb[g_wii_fb], GX_TRUE);
    VIDEO_SetNextFramebuffer(g_wii_xfb[g_wii_fb]);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (++s_present_count <= 3) wii_debug_logf("present #%d fb=%d draws=%u tris=%u", s_present_count, g_wii_fb, (unsigned)g_wii_stats.draw_calls, (unsigned)g_wii_stats.triangles);
}

static void glClearColor(float r,float g,float b,float a){ g_wii_clear_r=r; g_wii_clear_g=g; g_wii_clear_b=b; g_wii_clear_a=a; }
static void glClear(GLbitfield mask){ (void)mask; }
static void glEnable(GLenum cap){ if(cap==GL_TEXTURE_2D)g_wii_texture_enabled=1; else if(cap==GL_BLEND)g_wii_blend_enabled=1; else if(cap==GL_DEPTH_TEST)g_wii_depth_enabled=1; else if(cap==GL_ALPHA_TEST)g_wii_alpha_enabled=1; else if(cap==GL_CULL_FACE)g_wii_cull_enabled=1; }
static void glDisable(GLenum cap){ if(cap==GL_TEXTURE_2D)g_wii_texture_enabled=0; else if(cap==GL_BLEND)g_wii_blend_enabled=0; else if(cap==GL_DEPTH_TEST)g_wii_depth_enabled=0; else if(cap==GL_ALPHA_TEST)g_wii_alpha_enabled=0; else if(cap==GL_CULL_FACE)g_wii_cull_enabled=0; }
static void glBlendFunc(GLenum s, GLenum d){ (void)s; (void)d; }
static void glDepthFunc(GLenum f){ (void)f; }
static void glDepthMask(GLboolean flag){ g_wii_depth_write = flag ? 1 : 0; }
static void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a){ (void)r; (void)g; (void)b; (void)a; }
static void glAlphaFunc(GLenum func, GLfloat ref){ (void)func; (void)ref; g_wii_alpha_enabled = 1; }
static void glLineWidth(GLfloat w){ (void)w; }
static void glColor4f(float r,float g,float b,float a){ g_wii_cur_color=wii_color4f_pack(r,g,b,a); }
static void glViewport(GLint x,GLint y,GLsizei w,GLsizei h){ g_wii_viewport[0]=x; g_wii_viewport[1]=y; g_wii_viewport[2]=w; g_wii_viewport[3]=h; }
static void glMatrixMode(GLenum mode){ if(mode==GL_PROJECTION||mode==GL_MODELVIEW) g_wii_matrix_mode=mode; }
static void glLoadIdentity(void){ wii_mat_identity(wii_current_matrix()); }
static void glPushMatrix(void){ if(g_wii_matrix_mode==GL_PROJECTION){ if(g_wii_projection_sp<16) wii_mat_copy(g_wii_projection_stack[g_wii_projection_sp++],g_wii_projection); } else { if(g_wii_modelview_sp<32) wii_mat_copy(g_wii_modelview_stack[g_wii_modelview_sp++],g_wii_modelview); } }
static void glPopMatrix(void){ if(g_wii_matrix_mode==GL_PROJECTION){ if(g_wii_projection_sp>0) wii_mat_copy(g_wii_projection,g_wii_projection_stack[--g_wii_projection_sp]); } else { if(g_wii_modelview_sp>0) wii_mat_copy(g_wii_modelview,g_wii_modelview_stack[--g_wii_modelview_sp]); } }
static void glTranslatef(float x,float y,float z){ wii_mat_translate(x,y,z); }
static void glScalef(float x,float y,float z){ wii_mat_scale(x,y,z); }
static void glRotatef(float angle,float x,float y,float z){ wii_mat_rotate(angle,x,y,z); }
static void glOrtho(double l,double r,double b,double t,double n,double f){ wii_set_ortho(l,r,b,t,n,f); }
static void gluPerspective(double fovy,double aspect,double znear,double zfar){ wii_set_perspective(fovy,aspect,znear,zfar); }
static void glGetFloatv(GLenum pname, GLfloat *params){ if(!params)return; if(pname==GL_MODELVIEW_MATRIX)memcpy(params,g_wii_modelview,16*sizeof(float)); else if(pname==GL_PROJECTION_MATRIX)memcpy(params,g_wii_projection,16*sizeof(float)); else memset(params,0,16*sizeof(float)); }
static void glGetDoublev(GLenum pname, GLdouble *params){ if(!params)return; float tmp[16]; glGetFloatv(pname,tmp); for(int i=0;i<16;i++)params[i]=(double)tmp[i]; }
static void glGetIntegerv(GLenum pname, GLint *params){ if(pname==GL_VIEWPORT&&params)memcpy(params,g_wii_viewport,sizeof(g_wii_viewport)); else if(params)*params=0; }
static void glFogi(GLenum pname, GLint param){ (void)pname; (void)param; }
static void glFogf(GLenum pname, GLfloat param){ if(pname==GL_FOG_START)g_d3d9.fog_start=param; else if(pname==GL_FOG_END)g_d3d9.fog_end=param; }
static void glFogfv(GLenum pname, const GLfloat *params){ (void)pname; (void)params; }
static void glPixelStorei(GLenum pname, GLint param){ (void)pname; (void)param; }
static void glActiveTexture(GLenum texture){ (void)texture; }
static void glTexCoord2f(float u,float v){ g_wii_cur_u=u; g_wii_cur_v=v; }
static void glBegin(GLenum mode){ g_wii_begin_active=1; g_wii_begin_mode=mode; g_wii_imm_count=0; }
static void glVertex3f(float x,float y,float z){ if(!g_wii_begin_active||g_wii_imm_count>=PEX_WII_MAX_IMM_VERTS)return; WiiImmVertex *v=&g_wii_imm[g_wii_imm_count++]; v->x=x; v->y=y; v->z=z; v->u=g_wii_cur_u; v->v=g_wii_cur_v; v->color=g_wii_cur_color; }
static void glVertex2f(float x,float y){ glVertex3f(x,y,0.0f); }
static void glEnd(void){ g_wii_begin_active=0; if(g_wii_compiling&&g_wii_compile_list>0&&g_wii_compile_list<PEX_WII_LIST_COUNT){ WiiBatch *b=wii_batch_create(g_wii_imm,g_wii_imm_count,g_wii_begin_mode); if(b){ WiiList *l=&g_wii_lists[g_wii_compile_list]; if(l->last)l->last->next=b; else l->first=b; l->last=b; l->batch_count++; } } else { wii_apply_state(); wii_draw_imm_vertices(g_wii_imm,g_wii_imm_count,g_wii_begin_mode); } }
static GLuint glGenLists(GLsizei range){ GLuint base=g_wii_next_list; int r=range>0?range:1; for(int i=0;i<r;i++) if(base+(GLuint)i<PEX_WII_LIST_COUNT) wii_list_free(base+(GLuint)i); g_wii_next_list += (GLuint)r; if(g_wii_next_list>=PEX_WII_LIST_COUNT)g_wii_next_list=PEX_WII_LIST_COUNT-1; return base; }
static void glNewList(GLuint list, GLenum mode){ (void)mode; if(list>0&&list<PEX_WII_LIST_COUNT)wii_list_free(list); g_wii_compiling=1; g_wii_compile_list=list; }
static void glEndList(void){ g_wii_compiling=0; g_wii_compile_list=0; }
static void glCallList(GLuint list){ if(list>0&&list<PEX_WII_LIST_COUNT){ for(WiiBatch *b=g_wii_lists[list].first;b;b=b->next)wii_draw_batch(b); } }
static void glGenTextures(GLsizei n, GLuint *tex){ for(int i=0;i<n;i++){ GLuint id=g_wii_next_tex++; if(id>=PEX_WII_MAX_TEXTURES)id=1; tex[i]=id; if(g_wii_textures[id].pixels)free(g_wii_textures[id].pixels); memset(&g_wii_textures[id],0,sizeof(g_wii_textures[id])); g_wii_textures[id].repeat_s=1; g_wii_textures[id].repeat_t=1; } }
static void glBindTexture(GLenum target, GLuint tex){ (void)target; g_wii_bound_tex=tex; }
static void glTexParameteri(GLenum target, GLenum pname, GLint param){ (void)target; if(g_wii_bound_tex<PEX_WII_MAX_TEXTURES){ WiiTexture *t=&g_wii_textures[g_wii_bound_tex]; if(pname==GL_TEXTURE_WRAP_S)t->repeat_s=(param==GL_REPEAT); else if(pname==GL_TEXTURE_WRAP_T)t->repeat_t=(param==GL_REPEAT); } }
static void *wii_convert_rgba8_texture(const unsigned char *src, int w, int h, int *out_tw, int *out_th) {
    int tw = (w + 3) & ~3, th = (h + 3) & ~3; size_t bytes = (size_t)tw * (size_t)th * 4u;
    u8 *dst = (u8*)memalign(32, bytes); if(!dst) return NULL; memset(dst,0,bytes);
    u8 *d = dst;
    for(int by=0; by<th; by+=4) for(int bx=0; bx<tw; bx+=4) {
        for(int y=0;y<4;y++) for(int x=0;x<4;x++){ int sx=bx+x, sy=by+y; const unsigned char *p=(sx<w&&sy<h)?&src[(sy*w+sx)*4]:NULL; *d++=p?p[3]:0; *d++=p?p[0]:0; }
        for(int y=0;y<4;y++) for(int x=0;x<4;x++){ int sx=bx+x, sy=by+y; const unsigned char *p=(sx<w&&sy<h)?&src[(sy*w+sx)*4]:NULL; *d++=p?p[1]:0; *d++=p?p[2]:0; }
    }
    DCFlushRange(dst, (u32)bytes); if(out_tw)*out_tw=tw; if(out_th)*out_th=th; return dst;
}
static void glTexImage2D(GLenum target, GLint level, GLint internal, GLsizei w, GLsizei h, GLint border, GLenum fmt, GLenum type, const void *pixels){
    (void)target;(void)level;(void)internal;(void)border;(void)fmt;(void)type;
    if(!pixels||!g_wii_bound_tex||g_wii_bound_tex>=PEX_WII_MAX_TEXTURES){
        wii_debug_logf("glTexImage2D skipped pixels=%p tex=%u size=%dx%d", pixels, (unsigned)g_wii_bound_tex, (int)w, (int)h);
        return;
    }
    if(w<=0||h<=0||w>4096||h>4096){
        wii_debug_logf("glTexImage2D bad size tex=%u size=%dx%d", (unsigned)g_wii_bound_tex, (int)w, (int)h);
        return;
    }
    WiiTexture *t=&g_wii_textures[g_wii_bound_tex];
    if(t->pixels)free(t->pixels);
    int tw=0,th=0;
    t->pixels=wii_convert_rgba8_texture((const unsigned char*)pixels,w,h,&tw,&th);
    if(!t->pixels){ wii_debug_logf("glTexImage2D convert FAILED tex=%u size=%dx%d", (unsigned)g_wii_bound_tex, (int)w, (int)h); memset(t,0,sizeof(*t));return;}
    t->used=1;t->w=w;t->h=h;t->tw=tw;t->th=th;
    GX_InitTexObj(&t->obj,t->pixels,(u16)tw,(u16)th,GX_TF_RGBA8,t->repeat_s?GX_REPEAT:GX_CLAMP,t->repeat_t?GX_REPEAT:GX_CLAMP,GX_FALSE);
    GX_InitTexObjFilterMode(&t->obj,GX_NEAR,GX_NEAR);
    g_wii_stats.texture_uploads++;
    if(g_wii_stats.texture_uploads<=24) wii_debug_logf("texture upload #%u tex=%u src=%dx%d gx=%dx%d ptr=%p", (unsigned)g_wii_stats.texture_uploads, (unsigned)g_wii_bound_tex, (int)w, (int)h, tw, th, t->pixels);
}
static void glDeleteTextures(GLsizei n, const GLuint *tex){ for(int i=0;i<n;i++){ GLuint id=tex[i]; if(id<PEX_WII_MAX_TEXTURES){ free(g_wii_textures[id].pixels); memset(&g_wii_textures[id],0,sizeof(g_wii_textures[id])); } } }
static void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff, GLint x, GLint y, GLsizei w, GLsizei h){ (void)target;(void)level;(void)xoff;(void)yoff;(void)x;(void)y;(void)w;(void)h; }
static void glEnableClientState(GLenum cap){ if(cap==GL_VERTEX_ARRAY)g_wii_vertex_array_enabled=1; else if(cap==GL_TEXTURE_COORD_ARRAY)g_wii_texcoord_array_enabled=1; else if(cap==GL_COLOR_ARRAY)g_wii_color_array_enabled=1; }
static void glDisableClientState(GLenum cap){ if(cap==GL_VERTEX_ARRAY)g_wii_vertex_array_enabled=0; else if(cap==GL_TEXTURE_COORD_ARRAY)g_wii_texcoord_array_enabled=0; else if(cap==GL_COLOR_ARRAY)g_wii_color_array_enabled=0; }
static void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr){ g_wii_vertex_size=size; g_wii_vertex_type=type; g_wii_vertex_stride=stride; g_wii_vertex_ptr=ptr; }
static void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr){ g_wii_texcoord_size=size; g_wii_texcoord_type=type; g_wii_texcoord_stride=stride; g_wii_texcoord_ptr=ptr; }
static void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr){ g_wii_color_size=size; g_wii_color_type=type; g_wii_color_stride=stride; g_wii_color_ptr=ptr; }

static unsigned int wii_read_index(const GLvoid *indices, GLenum type, int i) {
    if (!indices) return 0;
    if (type == GL_UNSIGNED_INT) return ((const uint32_t*)indices)[i];
    if (type == GL_UNSIGNED_SHORT) return ((const uint16_t*)indices)[i];
    if (type == GL_UNSIGNED_BYTE) return ((const uint8_t*)indices)[i];
    return 0;
}

static int wii_arrays_are_pexvertex_interleaved(const unsigned char **base_out) {
    if (!g_wii_vertex_array_enabled || !g_wii_texcoord_array_enabled || !g_wii_color_array_enabled) return 0;
    if (!g_wii_vertex_ptr || !g_wii_texcoord_ptr || !g_wii_color_ptr) return 0;
    if (g_wii_vertex_size != 3 || g_wii_vertex_type != GL_FLOAT) return 0;
    if (g_wii_texcoord_size != 2 || g_wii_texcoord_type != GL_FLOAT) return 0;
    if (g_wii_color_size != 4 || g_wii_color_type != GL_UNSIGNED_BYTE) return 0;
    GLsizei vs = g_wii_vertex_stride ? g_wii_vertex_stride : (GLsizei)(3 * sizeof(GLfloat));
    GLsizei ts = g_wii_texcoord_stride ? g_wii_texcoord_stride : (GLsizei)(2 * sizeof(GLfloat));
    GLsizei cs = g_wii_color_stride ? g_wii_color_stride : (GLsizei)(4 * sizeof(GLubyte));
    if (vs != (GLsizei)sizeof(PexVertex) || ts != (GLsizei)sizeof(PexVertex) || cs != (GLsizei)sizeof(PexVertex)) return 0;
    const unsigned char *base = ((const unsigned char*)g_wii_vertex_ptr) - offsetof(PexVertex, x);
    if ((const unsigned char*)g_wii_texcoord_ptr != base + offsetof(PexVertex, u)) return 0;
    if ((const unsigned char*)g_wii_color_ptr != base + offsetof(PexVertex, color)) return 0;
    if (base_out) *base_out = base;
    return 1;
}

static void wii_emit_pex_vertex(const PexVertex *v) {
    GX_Position3f32(v->x, v->y, v->z);
    wii_emit_color(v->color);
    GX_TexCoord2f32(v->u, v->v);
}

static void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices){
    if (count <= 0 || !indices) return;
    const unsigned char *base = NULL;
    if (!wii_arrays_are_pexvertex_interleaved(&base)) return;
    int draw_count = count;
    GLenum gxmode = wii_draw_mode_for(mode, count, &draw_count);
    draw_count = wii_clamp_gx_count(mode, draw_count);
    if (draw_count <= 0) return;
    wii_apply_state();
    wii_load_gx_matrices();
    GX_Begin((u8)gxmode, GX_VTXFMT0, (u16)draw_count);
    for (int i = 0; i < draw_count; ++i) {
        unsigned int vi = wii_read_index(indices, type, i);
        const PexVertex *v = (const PexVertex*)(const void*)(base + ((size_t)vi * sizeof(PexVertex)));
        wii_emit_pex_vertex(v);
    }
    GX_End();
    g_wii_stats.draw_calls++;
    if (mode == GL_QUADS) g_wii_stats.triangles += (uint32_t)((draw_count / 4) * 2);
    else if (mode == GL_TRIANGLES) g_wii_stats.triangles += (uint32_t)(draw_count / 3);
}
static const GLubyte *glGetString(GLenum name){ if(name==GL_VENDOR)return (const GLubyte*)"Nintendo/devkitPro"; if(name==GL_RENDERER)return (const GLubyte*)"Wii Hollywood GX"; if(name==GL_VERSION)return (const GLubyte*)"GX fixed-function shim"; return (const GLubyte*)""; }

static PexTextureHandle wii_backend_create_texture(const PexTextureDesc *desc){ if(!desc||!desc->rgba_pixels||desc->width<=0||desc->height<=0)return 0; GLuint id=0; glGenTextures(1,&id); glBindTexture(GL_TEXTURE_2D,id); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,desc->repeat?GL_REPEAT:GL_CLAMP); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,desc->repeat?GL_REPEAT:GL_CLAMP); glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,desc->width,desc->height,0,GL_RGBA,GL_UNSIGNED_BYTE,desc->rgba_pixels); return id; }
static void wii_backend_destroy_texture(PexTextureHandle h){ GLuint id=h; glDeleteTextures(1,&id); }
static PexMeshHandle wii_backend_upload_mesh(const PexMesh *mesh){ if(!mesh||!mesh->vertices||!mesh->indices||mesh->vertex_count==0||mesh->index_count==0)return 0; for(uint32_t i=1;i<PEX_WII_MAX_MESHES;i++){ if(!g_wii_meshes[i].vertices){ WiiMeshSlot *s=&g_wii_meshes[i]; s->vertices=(PexVertex*)memalign(32,(size_t)mesh->vertex_count*sizeof(PexVertex)); s->indices=(uint32_t*)memalign(32,(size_t)mesh->index_count*sizeof(uint32_t)); if(!s->vertices||!s->indices){free(s->vertices);free(s->indices);memset(s,0,sizeof(*s));return 0;} memcpy(s->vertices,mesh->vertices,(size_t)mesh->vertex_count*sizeof(PexVertex)); memcpy(s->indices,mesh->indices,(size_t)mesh->index_count*sizeof(uint32_t)); DCFlushRange(s->vertices,(u32)((size_t)mesh->vertex_count*sizeof(PexVertex))); DCFlushRange(s->indices,(u32)((size_t)mesh->index_count*sizeof(uint32_t))); s->vertex_count=mesh->vertex_count;s->index_count=mesh->index_count;s->dynamic=mesh->dynamic; g_wii_stats.buffer_uploads++; return i; }} return 0; }
static int wii_backend_update_mesh(PexMeshHandle h,const PexMesh *mesh){ if(!h||h>=PEX_WII_MAX_MESHES)return 0; WiiMeshSlot *s=&g_wii_meshes[h]; free(s->vertices); free(s->indices); memset(s,0,sizeof(*s)); PexMeshHandle nh=wii_backend_upload_mesh(mesh); if(nh&&nh!=h){ g_wii_meshes[h]=g_wii_meshes[nh]; memset(&g_wii_meshes[nh],0,sizeof(g_wii_meshes[nh])); } return nh?1:0; }
static void wii_backend_destroy_mesh(PexMeshHandle h){ if(h&&h<PEX_WII_MAX_MESHES){ free(g_wii_meshes[h].vertices); free(g_wii_meshes[h].indices); memset(&g_wii_meshes[h],0,sizeof(g_wii_meshes[h])); } }
static void wii_apply_render_state(const PexRenderState *st){ if(!st)return; wii_apply_state_values(st->texture,st->texture_enabled,st->blend_enabled,st->depth_enabled,st->depth_write,st->alpha_test_enabled,0); wii_load_gx_matrices_from(st->modelview,st->projection,0); }
static void wii_backend_draw_vertices_indexed(const PexVertex *v,const uint32_t *idx,uint32_t vc,uint32_t ic,const PexRenderState *st){
    if(!v||!idx||vc==0||ic==0)return;
    wii_apply_render_state(st);
    uint32_t total=(ic/3)*3;
    uint32_t off=0;
    while(off<total){
        uint32_t chunk=total-off;
        if(chunk>65535u) chunk=65535u;
        chunk=(chunk/3u)*3u;
        if(chunk==0) break;
        GX_Begin(GX_TRIANGLES,GX_VTXFMT0,(u16)chunk);
        for(uint32_t i=0;i<chunk;i++){
            uint32_t vi=idx[off+i];
            if(vi>=vc)vi=0;
            GX_Position3f32(v[vi].x,v[vi].y,v[vi].z);
            wii_emit_color(v[vi].color);
            GX_TexCoord2f32(v[vi].u,v[vi].v);
        }
        GX_End();
        g_wii_stats.draw_calls++;
        g_wii_stats.triangles += chunk/3u;
        off += chunk;
    }
}
static void wii_backend_draw_mesh(PexMeshHandle h,const PexRenderState *st){ if(!h||h>=PEX_WII_MAX_MESHES)return; WiiMeshSlot *s=&g_wii_meshes[h]; wii_backend_draw_vertices_indexed(s->vertices,s->indices,s->vertex_count,s->index_count,st); }
static void wii_backend_draw_dynamic(const PexMesh *mesh,const PexRenderState *st){ if(!mesh)return; wii_backend_draw_vertices_indexed(mesh->vertices,mesh->indices,mesh->vertex_count,mesh->index_count,st); }
static PexRendererStats wii_backend_get_stats(void){ return g_wii_stats; }
static int wii_backend_init(void *window_handle,int width,int height){ (void)window_handle;(void)width;(void)height; return 1; }
static void wii_backend_shutdown(void){ }
static void wii_backend_resize(int width,int height){ (void)width;(void)height; }
static int wii_backend_begin_frame(float r,float g,float b,float a){ glClearColor(r,g,b,a); wii_gx_begin_frame(); return 1; }
static void wii_backend_end_frame(void){ wii_gx_end_frame(); }
static PexRendererBackend g_wii_backend={"Wii GX",wii_backend_init,wii_backend_shutdown,wii_backend_resize,wii_backend_begin_frame,wii_backend_end_frame,wii_backend_create_texture,wii_backend_destroy_texture,wii_backend_upload_mesh,wii_backend_update_mesh,wii_backend_destroy_mesh,wii_backend_draw_mesh,wii_backend_draw_dynamic,wii_backend_get_stats};
static PexRendererBackend *wii_gx_get_backend(void){ return &g_wii_backend; }
