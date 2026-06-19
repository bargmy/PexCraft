/* Sony PSP GU fixed-function renderer shim.
   Stable PSP path: persistent display-list batches + tiny command-list usage.
   This avoids exhausting GU command-list memory and fixes the earlier one-batch
   display-list bug that made world/menu rendering corrupt on PPSSPP/PSP. */

#define PEX_PSP_FB_WIDTH 512
#define PEX_PSP_SCR_WIDTH 480
#define PEX_PSP_SCR_HEIGHT 272
#define PEX_PSP_MAX_TEXTURES 1024
#define PEX_PSP_MAX_IMM_VERTS 16384
#define PEX_PSP_LIST_COUNT 4096
#define PEX_PSP_MAX_IMMEDIATE_DRAW_VERTS 4096

static unsigned int __attribute__((aligned(16))) g_psp_gu_list[262144];
static void *g_psp_drawbuf = NULL;
static void *g_psp_dispbuf = NULL;
static void *g_psp_depthbuf = NULL;
static unsigned int g_psp_display_offset = 0x00088000u;

static int g_psp_texture_enabled = 1;
static int g_psp_blend_enabled = 1;
static int g_psp_depth_enabled = 1;
static int g_psp_alpha_enabled = 0;
static unsigned int g_psp_clear_color = 0xff000000u;
static unsigned int g_psp_cur_color = 0xffffffffu;
static GLuint g_psp_bound_tex = 0;
static GLenum g_psp_begin_mode = 0;
static int g_psp_begin_active = 0;
static float g_psp_cur_u = 0.0f, g_psp_cur_v = 0.0f;
static int g_psp_viewport[4] = {0,0,480,272};
static unsigned int g_psp_verbose_frame_counter = 0;
static int g_psp_cull_enabled = 0;

typedef struct PspImmVertex { float u, v; unsigned int color; float x, y, z; } PspImmVertex;
static PspImmVertex g_psp_imm[PEX_PSP_MAX_IMM_VERTS];
static int g_psp_imm_count = 0;

typedef struct PspTexture { int used; int w, h, stride; unsigned int *pixels; } PspTexture;
static PspTexture g_psp_textures[PEX_PSP_MAX_TEXTURES];
static GLuint g_psp_next_tex = 1;

typedef struct PspBatch {
    PspImmVertex *v;
    int count;
    GLenum mode;
    GLuint tex;
    int texture_enabled;
    int blend_enabled;
    int depth_enabled;
    int alpha_enabled;
    int cull_enabled;
    struct PspBatch *next;
} PspBatch;

typedef struct PspList { PspBatch *first; PspBatch *last; int batch_count; } PspList;
static PspList g_psp_lists[PEX_PSP_LIST_COUNT];
static GLuint g_psp_next_list = 1;
static int g_psp_compiling = 0;
static GLuint g_psp_compile_list = 0;

static unsigned int psp_rgba_to_abgr(float r, float g, float b, float a) {
    int ri=(int)(r*255.0f+0.5f), gi=(int)(g*255.0f+0.5f), bi=(int)(b*255.0f+0.5f), ai=(int)(a*255.0f+0.5f);
    if(ri<0)ri=0; if(ri>255)ri=255;
    if(gi<0)gi=0; if(gi>255)gi=255;
    if(bi<0)bi=0; if(bi>255)bi=255;
    if(ai<0)ai=0; if(ai>255)ai=255;
    /* GU_COLOR_8888 is stored in memory as RGBA bytes on little-endian PSP. */
    return (unsigned int)(ri | (gi<<8) | (bi<<16) | (ai<<24));
}

static void psp_apply_state_values(GLuint tex, int tex_enabled, int blend_enabled, int depth_enabled, int alpha_enabled, int cull_enabled) {
    if (tex_enabled && tex && tex < PEX_PSP_MAX_TEXTURES && g_psp_textures[tex].used) {
        PspTexture *t=&g_psp_textures[tex];
        sceGuEnable(GU_TEXTURE_2D);
        sceGuTexMode(GU_PSM_8888,0,0,0);
        sceGuTexImage(0,t->w,t->h,t->stride,t->pixels);
        sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
        sceGuTexFilter(GU_NEAREST,GU_NEAREST);
        sceGuTexWrap(GU_REPEAT,GU_REPEAT);
        sceGuTexScale(1.0f, 1.0f);
        sceGuTexOffset(0.0f, 0.0f);
    } else {
        sceGuDisable(GU_TEXTURE_2D);
    }
    if (blend_enabled) { sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); }
    else sceGuDisable(GU_BLEND);
    if (depth_enabled) sceGuEnable(GU_DEPTH_TEST); else sceGuDisable(GU_DEPTH_TEST);
    if (alpha_enabled) { sceGuEnable(GU_ALPHA_TEST); sceGuAlphaFunc(GU_GREATER, 0x08, 0xff); }
    else sceGuDisable(GU_ALPHA_TEST);
    if (cull_enabled) sceGuEnable(GU_CULL_FACE); else sceGuDisable(GU_CULL_FACE);
}

static void psp_apply_state(void) {
    psp_apply_state_values(g_psp_bound_tex, g_psp_texture_enabled, g_psp_blend_enabled, g_psp_depth_enabled, g_psp_alpha_enabled, g_psp_cull_enabled);
}

static int psp_prim_from_gl(GLenum mode) {
    if (mode == GL_TRIANGLES) return GU_TRIANGLES;
    if (mode == GL_TRIANGLE_STRIP) return GU_TRIANGLE_STRIP;
    if (mode == GL_TRIANGLE_FAN) return GU_TRIANGLE_FAN;
    if (mode == GL_LINES) return GU_LINES;
    if (mode == GL_LINE_STRIP) return GU_LINE_STRIP;
    if (mode == GL_POINTS) return GU_POINTS;
    return GU_TRIANGLES;
}

static void psp_list_free(GLuint list) {
    if (list == 0 || list >= PEX_PSP_LIST_COUNT) return;
    PspBatch *b = g_psp_lists[list].first;
    while (b) {
        PspBatch *n = b->next;
        free(b->v);
        free(b);
        b = n;
    }
    memset(&g_psp_lists[list], 0, sizeof(g_psp_lists[list]));
}

static PspBatch *psp_batch_create(const PspImmVertex *src, int count, GLenum mode) {
    if (!src || count <= 0) return NULL;
    int out_count = count;
    GLenum out_mode = mode;
    if (mode == GL_QUADS) {
        int q = count / 4;
        if (q <= 0) return NULL;
        out_count = q * 6;
        out_mode = GL_TRIANGLES;
    }
    PspBatch *b = (PspBatch*)calloc(1, sizeof(PspBatch));
    if (!b) return NULL;
    b->v = (PspImmVertex*)memalign(16, (size_t)out_count * sizeof(PspImmVertex));
    if (!b->v) { free(b); return NULL; }
    b->count = out_count;
    b->mode = out_mode;
    b->tex = g_psp_bound_tex;
    b->texture_enabled = g_psp_texture_enabled;
    b->blend_enabled = g_psp_blend_enabled;
    b->depth_enabled = g_psp_depth_enabled;
    b->alpha_enabled = g_psp_alpha_enabled;
    b->cull_enabled = g_psp_cull_enabled;
    if (mode == GL_QUADS) {
        int n = 0;
        int q = count / 4;
        for (int i=0;i<q;i++) {
            const PspImmVertex *v = src + i*4;
            b->v[n++]=v[0]; b->v[n++]=v[1]; b->v[n++]=v[2];
            b->v[n++]=v[0]; b->v[n++]=v[2]; b->v[n++]=v[3];
        }
    } else {
        memcpy(b->v, src, (size_t)count * sizeof(PspImmVertex));
    }
    sceKernelDcacheWritebackInvalidateRange(b->v, (SceSize)((size_t)b->count * sizeof(PspImmVertex)));
    return b;
}

static void psp_list_append_batch(GLuint list, PspBatch *b) {
    if (!b || list == 0 || list >= PEX_PSP_LIST_COUNT) { if (b) { free(b->v); free(b); } return; }
    PspList *l = &g_psp_lists[list];
    if (!l->first) l->first = b;
    else l->last->next = b;
    l->last = b;
    l->batch_count++;
}

static void psp_draw_persistent_batch(const PspBatch *b) {
    if (!b || !b->v || b->count <= 0) return;
    psp_apply_state_values(b->tex, b->texture_enabled, b->blend_enabled, b->depth_enabled, b->alpha_enabled, b->cull_enabled);
    sceKernelDcacheWritebackInvalidateRange((void*)b->v, (SceSize)((size_t)b->count * sizeof(PspImmVertex)));
    sceGumDrawArray(psp_prim_from_gl(b->mode), GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D, b->count, 0, b->v);
}

static void psp_draw_vertex_array_immediate(const PspImmVertex *src, int count, GLenum mode) {
    if (!src || count <= 0) return;
    psp_apply_state();
    if (mode == GL_QUADS) {
        int q_total = count / 4;
        int q_pos = 0;
        while (q_pos < q_total) {
            int q = q_total - q_pos;
            int max_q = PEX_PSP_MAX_IMMEDIATE_DRAW_VERTS / 6;
            if (q > max_q) q = max_q;
            PspImmVertex *out = (PspImmVertex*)sceGuGetMemory((q * 6) * sizeof(PspImmVertex));
            if (!out) return;
            int n=0;
            for (int i=0;i<q;i++) {
                const PspImmVertex *v = src + (q_pos + i)*4;
                out[n++]=v[0]; out[n++]=v[1]; out[n++]=v[2];
                out[n++]=v[0]; out[n++]=v[2]; out[n++]=v[3];
            }
            sceGumDrawArray(GU_TRIANGLES, GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D, n, 0, out);
            q_pos += q;
        }
    } else {
        int pos = 0;
        while (pos < count) {
            int n = count - pos;
            if (n > PEX_PSP_MAX_IMMEDIATE_DRAW_VERTS) n = PEX_PSP_MAX_IMMEDIATE_DRAW_VERTS;
            PspImmVertex *out = (PspImmVertex*)sceGuGetMemory(n * sizeof(PspImmVertex));
            if (!out) return;
            memcpy(out, src + pos, (size_t)n * sizeof(PspImmVertex));
            sceGumDrawArray(psp_prim_from_gl(mode), GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D, n, 0, out);
            pos += n;
        }
    }
}

static void psp_flush_immediate(void) {
    if (g_psp_imm_count <= 0) return;
    if (g_psp_compiling && g_psp_compile_list > 0 && g_psp_compile_list < PEX_PSP_LIST_COUNT) {
        PspBatch *b = psp_batch_create(g_psp_imm, g_psp_imm_count, g_psp_begin_mode);
        psp_list_append_batch(g_psp_compile_list, b);
    } else {
        psp_draw_vertex_array_immediate(g_psp_imm, g_psp_imm_count, g_psp_begin_mode);
    }
    g_psp_imm_count = 0;
}

static int psp_gu_init(void) {
    PEX_PSP_LOGF("GU_INIT enter");
    sceKernelDcacheWritebackInvalidateAll();
    sceGuInit();
    g_psp_drawbuf = (void*)0;
    g_psp_dispbuf = (void*)0x88000;
    g_psp_depthbuf = (void*)0x110000;
    g_psp_display_offset = 0x00088000u;
    PEX_PSP_LOGF("GU_INIT buffers draw=%p disp=%p depth=%p list=%p", g_psp_drawbuf, g_psp_dispbuf, g_psp_depthbuf, g_psp_gu_list);
    sceGuStart(GU_DIRECT, g_psp_gu_list);
    sceGuDrawBuffer(GU_PSM_8888, g_psp_drawbuf, PEX_PSP_FB_WIDTH);
    sceGuDispBuffer(PEX_PSP_SCR_WIDTH, PEX_PSP_SCR_HEIGHT, g_psp_dispbuf, PEX_PSP_FB_WIDTH);
    sceGuDepthBuffer(g_psp_depthbuf, PEX_PSP_FB_WIDTH);
    sceGuOffset(2048 - (PEX_PSP_SCR_WIDTH/2), 2048 - (PEX_PSP_SCR_HEIGHT/2));
    sceGuViewport(2048, 2048, PEX_PSP_SCR_WIDTH, PEX_PSP_SCR_HEIGHT);
    sceGuDepthRange(65535, 0);
    sceGuScissor(0, 0, PEX_PSP_SCR_WIDTH, PEX_PSP_SCR_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    /* PSP depth is configured with sceGuDepthRange(65535, 0) and sceGuClearDepth(0),
       so visible 3D geometry must pass with GEQUAL. Using LEQUAL makes normal
       world geometry fail depth against the cleared depth buffer, leaving the
       HUD/UI visible over a black world. */
    sceGuDepthFunc(GU_GEQUAL);
    sceGuFrontFace(GU_CW);
    sceGuDisable(GU_CULL_FACE);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuClearColor(0xff000000u);
    sceGuClearDepth(0);
    sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
    sceGuFinish();
    sceGuSync(0,0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
    sceGumMatrixMode(GU_PROJECTION); sceGumLoadIdentity(); sceGumPerspective(70.0f, 480.0f/272.0f, 0.05f, 1024.0f);
    sceGumMatrixMode(GU_VIEW); sceGumLoadIdentity();
    sceGumMatrixMode(GU_MODEL); sceGumLoadIdentity();
    PEX_PSP_LOGF("GU_INIT done");
    return 1;
}

static unsigned int psp_gu_current_display_offset(void) { return g_psp_display_offset; }
static void psp_gu_shutdown(void) {
    PEX_PSP_LOGF("GU_SHUTDOWN sceGuTerm");
    for (GLuint i=1;i<PEX_PSP_LIST_COUNT;i++) psp_list_free(i);
    for (GLuint i=1;i<PEX_PSP_MAX_TEXTURES;i++) { free(g_psp_textures[i].pixels); g_psp_textures[i].pixels=NULL; }
    sceGuTerm();
}
static void psp_gu_begin_frame(void) {
    g_psp_verbose_frame_counter++;
    if (g_psp_verbose_frame_counter <= 30 || (g_psp_verbose_frame_counter % 120u) == 0u) PEX_PSP_LOGF("GU_FRAME %u begin", g_psp_verbose_frame_counter);
    sceGuStart(GU_DIRECT, g_psp_gu_list);
    sceGuClearColor(g_psp_clear_color);
    sceGuClearDepth(0);
    sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
}
static void psp_gu_end_frame(void) {
    if (g_psp_verbose_frame_counter <= 30 || (g_psp_verbose_frame_counter % 120u) == 0u) PEX_PSP_LOGF("GU_FRAME %u end/swap", g_psp_verbose_frame_counter);
    sceGuFinish();
    sceGuSync(0,0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
    g_psp_display_offset = (g_psp_display_offset == 0u) ? 0x00088000u : 0u;
}

static void glClearColor(float r,float g,float b,float a){ g_psp_clear_color=psp_rgba_to_abgr(r,g,b,a); }
static void glClear(GLbitfield mask){ (void)mask; sceGuClearColor(g_psp_clear_color); sceGuClearDepth(0); sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT); }
static void glEnable(GLenum cap){ if(cap==GL_TEXTURE_2D)g_psp_texture_enabled=1; else if(cap==GL_BLEND)g_psp_blend_enabled=1; else if(cap==GL_DEPTH_TEST)g_psp_depth_enabled=1; else if(cap==GL_ALPHA_TEST)g_psp_alpha_enabled=1; else if(cap==GL_CULL_FACE)g_psp_cull_enabled=1; }
static void glDisable(GLenum cap){ if(cap==GL_TEXTURE_2D)g_psp_texture_enabled=0; else if(cap==GL_BLEND)g_psp_blend_enabled=0; else if(cap==GL_DEPTH_TEST)g_psp_depth_enabled=0; else if(cap==GL_ALPHA_TEST)g_psp_alpha_enabled=0; else if(cap==GL_CULL_FACE)g_psp_cull_enabled=0; }
static void glBlendFunc(GLenum s, GLenum d){ (void)s; (void)d; }
static void glDepthFunc(GLenum f){
    (void)f;
    /* PSP uses a reversed depth range in this shim: clear depth is 0 and
       nearer geometry produces larger depth values. Force GEQUAL so 3D world
       geometry is not rejected while HUD/menu drawing still works normally. */
    sceGuDepthFunc(GU_GEQUAL);
}
static void glDepthMask(GLboolean flag){ sceGuDepthMask(flag?GU_FALSE:GU_TRUE); }
static void glAlphaFunc(GLenum func, GLfloat ref){ (void)func; sceGuAlphaFunc(GU_GREATER, (int)(ref*255.0f), 0xff); }
static void glLineWidth(GLfloat w){ (void)w; }
static void glColor4f(float r,float g,float b,float a){ g_psp_cur_color=psp_rgba_to_abgr(r,g,b,a); }
static void glViewport(GLint x,GLint y,GLsizei w,GLsizei h){
    /* OpenGL glViewport uses a bottom-left origin and describes the viewport rect.
       PSP GU's sceGuViewport wants the viewport *center* in its offset coordinate
       space. The old shim used 2048+x+w/2 directly, which shifted a fullscreen
       480x272 viewport by +240,+136. That made every GUI/menu draw down-right
       and clipped most of the screen. Keep fullscreen centered at 2048,2048. */
    if (w <= 0) w = PEX_PSP_SCR_WIDTH;
    if (h <= 0) h = PEX_PSP_SCR_HEIGHT;
    g_psp_viewport[0]=x; g_psp_viewport[1]=y; g_psp_viewport[2]=w; g_psp_viewport[3]=h;
    int top_y = PEX_PSP_SCR_HEIGHT - (int)y - (int)h;
    int cx = 2048 + (int)x + ((int)w / 2) - (PEX_PSP_SCR_WIDTH / 2);
    int cy = 2048 + top_y + ((int)h / 2) - (PEX_PSP_SCR_HEIGHT / 2);
    if (w == PEX_PSP_SCR_WIDTH && h == PEX_PSP_SCR_HEIGHT && x == 0 && y == 0) { cx = 2048; cy = 2048; }
    sceGuViewport(cx, cy, w, h);
    int sx0 = x; int sy0 = top_y; int sx1 = x + w; int sy1 = top_y + h;
    if (sx0 < 0) sx0 = 0; if (sy0 < 0) sy0 = 0;
    if (sx1 > PEX_PSP_SCR_WIDTH) sx1 = PEX_PSP_SCR_WIDTH;
    if (sy1 > PEX_PSP_SCR_HEIGHT) sy1 = PEX_PSP_SCR_HEIGHT;
    sceGuScissor(sx0, sy0, sx1, sy1);
}
static void glMatrixMode(GLenum mode){ if(mode==GL_PROJECTION) sceGumMatrixMode(GU_PROJECTION); else sceGumMatrixMode(GU_MODEL); }
static void glLoadIdentity(void){ sceGumLoadIdentity(); }
static void glPushMatrix(void){ sceGumPushMatrix(); }
static void glPopMatrix(void){ sceGumPopMatrix(); }
static void glTranslatef(float x,float y,float z){ ScePspFVector3 v={x,y,z}; sceGumTranslate(&v); }
static void glScalef(float x,float y,float z){ ScePspFVector3 v={x,y,z}; sceGumScale(&v); }
static void glRotatef(float angle,float x,float y,float z){ ScePspFVector3 v={x*angle*(GU_PI/180.0f), y*angle*(GU_PI/180.0f), z*angle*(GU_PI/180.0f)}; sceGumRotateXYZ(&v); }
static void glOrtho(double l,double r,double b,double t,double n,double f){ sceGumOrtho((float)l,(float)r,(float)b,(float)t,(float)n,(float)f); }
static void gluPerspective(double fovy,double aspect,double znear,double zfar){ sceGumPerspective((float)fovy,(float)aspect,(float)znear,(float)zfar); }
static void glGetFloatv(GLenum pname, GLfloat *params){ if(params) memset(params,0,16*sizeof(float)); (void)pname; }
static void glGetDoublev(GLenum pname, GLdouble *params){ if(params) memset(params,0,16*sizeof(double)); (void)pname; }
static void glGetIntegerv(GLenum pname, GLint *params){ if(pname==GL_VIEWPORT && params) memcpy(params,g_psp_viewport,sizeof(g_psp_viewport)); else if(params) *params=0; }
static void glFogi(GLenum pname, GLint param){ (void)pname; (void)param; }
static void glFogf(GLenum pname, GLfloat param){ (void)pname; (void)param; }
static void glFogfv(GLenum pname, const GLfloat *params){ (void)pname; (void)params; }
static void glTexCoord2f(float u,float v){ g_psp_cur_u=u; g_psp_cur_v=v; }
static void glBegin(GLenum mode){ g_psp_begin_active=1; g_psp_begin_mode=mode; g_psp_imm_count=0; }
static void glVertex3f(float x,float y,float z){ if(!g_psp_begin_active||g_psp_imm_count>=PEX_PSP_MAX_IMM_VERTS)return; PspImmVertex *v=&g_psp_imm[g_psp_imm_count++]; v->u=g_psp_cur_u; v->v=g_psp_cur_v; v->color=g_psp_cur_color; v->x=x; v->y=y; v->z=z; }
static void glVertex2f(float x,float y){ glVertex3f(x,y,0.0f); }
static void glEnd(void){ g_psp_begin_active=0; psp_flush_immediate(); }
static GLuint glGenLists(GLsizei range){ GLuint base=g_psp_next_list; int r=range>0?range:1; for(int i=0;i<r;i++) if(base+(GLuint)i<PEX_PSP_LIST_COUNT) psp_list_free(base+(GLuint)i); g_psp_next_list += (GLuint)r; if(g_psp_next_list>=PEX_PSP_LIST_COUNT)g_psp_next_list=PEX_PSP_LIST_COUNT-1; return base; }
static void glNewList(GLuint list, GLenum mode){ (void)mode; if(list>0&&list<PEX_PSP_LIST_COUNT) psp_list_free(list); g_psp_compiling=1; g_psp_compile_list=list; }
static void glEndList(void){ g_psp_compiling=0; g_psp_compile_list=0; }
static void glCallList(GLuint list){ if(list>0&&list<PEX_PSP_LIST_COUNT){ for(PspBatch *b=g_psp_lists[list].first;b;b=b->next) psp_draw_persistent_batch(b); } }
static void glGenTextures(GLsizei n, GLuint *tex){ for(int i=0;i<n;i++){ GLuint id=g_psp_next_tex++; if(id>=PEX_PSP_MAX_TEXTURES)id=1; tex[i]=id; free(g_psp_textures[id].pixels); memset(&g_psp_textures[id],0,sizeof(g_psp_textures[id])); } }
static void glBindTexture(GLenum target, GLuint tex){ (void)target; g_psp_bound_tex=tex; }
static void glTexParameteri(GLenum target, GLenum pname, GLint param){ (void)target; (void)pname; (void)param; }
static void glTexImage2D(GLenum target, GLint level, GLint internal, GLsizei w, GLsizei h, GLint border, GLenum fmt, GLenum type, const void *pixels){ (void)target;(void)level;(void)internal;(void)border;(void)fmt;(void)type; if(!g_psp_bound_tex||g_psp_bound_tex>=PEX_PSP_MAX_TEXTURES)return; PspTexture *t=&g_psp_textures[g_psp_bound_tex]; free(t->pixels); memset(t,0,sizeof(*t)); if(!pixels||w<=0||h<=0)return; t->w=w; t->h=h; t->stride=w; t->pixels=(unsigned int*)memalign(16,(size_t)w*h*4); if(t->pixels){ memcpy(t->pixels,pixels,(size_t)w*h*4); sceKernelDcacheWritebackInvalidateRange(t->pixels,(SceSize)((size_t)w*h*4)); t->used=1; } }
static void glDeleteTextures(GLsizei n, const GLuint *tex){ for(int i=0;i<n;i++){ GLuint id=tex[i]; if(id<PEX_PSP_MAX_TEXTURES){ free(g_psp_textures[id].pixels); memset(&g_psp_textures[id],0,sizeof(g_psp_textures[id])); } } }
static void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff, GLint x, GLint y, GLsizei w, GLsizei h){ (void)target;(void)level;(void)xoff;(void)yoff;(void)x;(void)y;(void)w;(void)h; /* PSP path avoids backbuffer copies; title snapshot is disabled. */ }
static const GLubyte *glGetString(GLenum name){ if(name==GL_VENDOR)return (const GLubyte*)"Sony"; if(name==GL_RENDERER)return (const GLubyte*)"PSP Graphics Engine (GU)"; if(name==GL_VERSION)return (const GLubyte*)"PSP GU fixed-function"; return (const GLubyte*)""; }
