/* Sony PSP GU fixed-function renderer shim.
   This exposes the tiny OpenGL 1.1 immediate-mode subset used by PEXCraft and
   maps it to pspgu/pspgum. It is intentionally separate from Win32/D3D/SDL. */

#define PEX_PSP_FB_WIDTH 512
#define PEX_PSP_SCR_WIDTH 480
#define PEX_PSP_SCR_HEIGHT 272
#define PEX_PSP_MAX_TEXTURES 1024
#define PEX_PSP_MAX_IMM_VERTS 16384
#define PEX_PSP_LIST_COUNT 4096

static unsigned int __attribute__((aligned(16))) g_psp_gu_list[262144];
static void *g_psp_drawbuf = NULL;
static void *g_psp_dispbuf = NULL;
static void *g_psp_depthbuf = NULL;

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

typedef struct PspImmVertex { float u, v; unsigned int color; float x, y, z; } PspImmVertex;
static PspImmVertex g_psp_imm[PEX_PSP_MAX_IMM_VERTS];
static int g_psp_imm_count = 0;

typedef struct PspTexture { int used; int w, h, stride; unsigned int *pixels; } PspTexture;
static PspTexture g_psp_textures[PEX_PSP_MAX_TEXTURES];
static GLuint g_psp_next_tex = 1;

typedef struct PspList { PspImmVertex *v; int count; GLenum mode; GLuint tex; int texture_enabled; int blend_enabled; int depth_enabled; int alpha_enabled; } PspList;
static PspList g_psp_lists[PEX_PSP_LIST_COUNT];
static GLuint g_psp_next_list = 1;
static int g_psp_compiling = 0;
static GLuint g_psp_compile_list = 0;

static unsigned int psp_rgba_to_abgr(float r, float g, float b, float a) {
    int ri=(int)(r*255.0f+0.5f), gi=(int)(g*255.0f+0.5f), bi=(int)(b*255.0f+0.5f), ai=(int)(a*255.0f+0.5f);
    if(ri<0)ri=0; if(ri>255)ri=255; if(gi<0)gi=0; if(gi>255)gi=255; if(bi<0)bi=0; if(bi>255)bi=255; if(ai<0)ai=0; if(ai>255)ai=255;
    return (unsigned int)(ri | (gi<<8) | (bi<<16) | (ai<<24));
}

static void psp_apply_state(void) {
    if (g_psp_texture_enabled && g_psp_bound_tex && g_psp_bound_tex < PEX_PSP_MAX_TEXTURES && g_psp_textures[g_psp_bound_tex].used) {
        PspTexture *t=&g_psp_textures[g_psp_bound_tex];
        sceGuEnable(GU_TEXTURE_2D);
        sceGuTexMode(GU_PSM_8888,0,0,0);
        sceGuTexImage(0,t->w,t->h,t->stride,t->pixels);
        sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
        sceGuTexFilter(GU_NEAREST,GU_NEAREST);
        sceGuTexWrap(GU_REPEAT,GU_REPEAT);
    } else sceGuDisable(GU_TEXTURE_2D);
    if (g_psp_blend_enabled) { sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); } else sceGuDisable(GU_BLEND);
    if (g_psp_depth_enabled) sceGuEnable(GU_DEPTH_TEST); else sceGuDisable(GU_DEPTH_TEST);
    if (g_psp_alpha_enabled) { sceGuEnable(GU_ALPHA_TEST); sceGuAlphaFunc(GU_GREATER, 0x08, 0xff); } else sceGuDisable(GU_ALPHA_TEST);
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

static void psp_draw_vertex_array(const PspImmVertex *src, int count, GLenum mode) {
    if (!src || count <= 0) return;
    psp_apply_state();
    if (mode == GL_QUADS) {
        int q = count / 4;
        if (q <= 0) return;
        PspImmVertex *out = (PspImmVertex*)sceGuGetMemory((q * 6) * sizeof(PspImmVertex));
        if (!out) return;
        int n=0;
        for (int i=0;i<q;i++) {
            const PspImmVertex *v = src + i*4;
            out[n++]=v[0]; out[n++]=v[1]; out[n++]=v[2];
            out[n++]=v[0]; out[n++]=v[2]; out[n++]=v[3];
        }
        sceGumDrawArray(GU_TRIANGLES, GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D, n, 0, out);
    } else {
        PspImmVertex *out = (PspImmVertex*)sceGuGetMemory(count * sizeof(PspImmVertex));
        if (!out) return;
        memcpy(out, src, count * sizeof(PspImmVertex));
        sceGumDrawArray(psp_prim_from_gl(mode), GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D, count, 0, out);
    }
}

static void psp_flush_immediate(void) {
    if (g_psp_imm_count <= 0) return;
    if (g_psp_compiling && g_psp_compile_list > 0 && g_psp_compile_list < PEX_PSP_LIST_COUNT) {
        PspList *lst=&g_psp_lists[g_psp_compile_list];
        free(lst->v); memset(lst,0,sizeof(*lst));
        lst->v=(PspImmVertex*)malloc((size_t)g_psp_imm_count*sizeof(PspImmVertex));
        if (lst->v) { memcpy(lst->v,g_psp_imm,(size_t)g_psp_imm_count*sizeof(PspImmVertex)); lst->count=g_psp_imm_count; }
        lst->mode=g_psp_begin_mode; lst->tex=g_psp_bound_tex; lst->texture_enabled=g_psp_texture_enabled; lst->blend_enabled=g_psp_blend_enabled; lst->depth_enabled=g_psp_depth_enabled; lst->alpha_enabled=g_psp_alpha_enabled;
    } else psp_draw_vertex_array(g_psp_imm, g_psp_imm_count, g_psp_begin_mode);
    g_psp_imm_count = 0;
}

static int psp_gu_init(void) {
    sceKernelDcacheWritebackInvalidateAll();
    sceGuInit();
    g_psp_drawbuf = (void*)0;
    g_psp_dispbuf = (void*)0x88000;
    g_psp_depthbuf = (void*)0x110000;
    sceGuStart(GU_DIRECT, g_psp_gu_list);
    sceGuDrawBuffer(GU_PSM_8888, g_psp_drawbuf, PEX_PSP_FB_WIDTH);
    sceGuDispBuffer(PEX_PSP_SCR_WIDTH, PEX_PSP_SCR_HEIGHT, g_psp_dispbuf, PEX_PSP_FB_WIDTH);
    sceGuDepthBuffer(g_psp_depthbuf, PEX_PSP_FB_WIDTH);
    sceGuOffset(2048 - (PEX_PSP_SCR_WIDTH/2), 2048 - (PEX_PSP_SCR_HEIGHT/2));
    sceGuViewport(2048, 2048, PEX_PSP_SCR_WIDTH, PEX_PSP_SCR_HEIGHT);
    sceGuDepthRange(65535, 0);
    sceGuScissor(0, 0, PEX_PSP_SCR_WIDTH, PEX_PSP_SCR_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDepthFunc(GU_LEQUAL);
    sceGuFrontFace(GU_CW);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuFinish();
    sceGuSync(0,0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
    sceGumMatrixMode(GU_PROJECTION); sceGumLoadIdentity(); sceGumPerspective(70.0f, 480.0f/272.0f, 0.05f, 1024.0f);
    sceGumMatrixMode(GU_VIEW); sceGumLoadIdentity();
    sceGumMatrixMode(GU_MODEL); sceGumLoadIdentity();
    return 1;
}

static void psp_gu_shutdown(void) { sceGuTerm(); }
static void psp_gu_begin_frame(void) { sceGuStart(GU_DIRECT, g_psp_gu_list); sceGuClearColor(g_psp_clear_color); sceGuClearDepth(0); sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT); }
static void psp_gu_end_frame(void) { sceGuFinish(); sceGuSync(0,0); sceDisplayWaitVblankStart(); sceGuSwapBuffers(); }

static void glClearColor(float r,float g,float b,float a){ g_psp_clear_color=psp_rgba_to_abgr(r,g,b,a); }
static void glClear(GLbitfield mask){ (void)mask; sceGuClearColor(g_psp_clear_color); sceGuClearDepth(0); sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT); }
static void glEnable(GLenum cap){ if(cap==GL_TEXTURE_2D)g_psp_texture_enabled=1; else if(cap==GL_BLEND)g_psp_blend_enabled=1; else if(cap==GL_DEPTH_TEST)g_psp_depth_enabled=1; else if(cap==GL_ALPHA_TEST)g_psp_alpha_enabled=1; }
static void glDisable(GLenum cap){ if(cap==GL_TEXTURE_2D)g_psp_texture_enabled=0; else if(cap==GL_BLEND)g_psp_blend_enabled=0; else if(cap==GL_DEPTH_TEST)g_psp_depth_enabled=0; else if(cap==GL_ALPHA_TEST)g_psp_alpha_enabled=0; }
static void glBlendFunc(GLenum s, GLenum d){ (void)s; (void)d; }
static void glDepthFunc(GLenum f){ if(f==GL_LESS) sceGuDepthFunc(GU_LESS); else sceGuDepthFunc(GU_LEQUAL); }
static void glDepthMask(GLboolean flag){ sceGuDepthMask(flag?GU_FALSE:GU_TRUE); }
static void glAlphaFunc(GLenum func, GLfloat ref){ (void)func; sceGuAlphaFunc(GU_GREATER, (int)(ref*255.0f), 0xff); }
static void glLineWidth(GLfloat w){ (void)w; }
static void glColor4f(float r,float g,float b,float a){ g_psp_cur_color=psp_rgba_to_abgr(r,g,b,a); }
static void glViewport(GLint x,GLint y,GLsizei w,GLsizei h){ g_psp_viewport[0]=x; g_psp_viewport[1]=y; g_psp_viewport[2]=w; g_psp_viewport[3]=h; sceGuViewport(2048 + x, 2048 + y, w, h); }
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
static GLuint glGenLists(GLsizei range){ GLuint base=g_psp_next_list; g_psp_next_list += range>0?range:1; if(g_psp_next_list>=PEX_PSP_LIST_COUNT)g_psp_next_list=PEX_PSP_LIST_COUNT-1; return base; }
static void glNewList(GLuint list, GLenum mode){ (void)mode; g_psp_compiling=1; g_psp_compile_list=list; }
static void glEndList(void){ g_psp_compiling=0; g_psp_compile_list=0; }
static void glCallList(GLuint list){ if(list>0&&list<PEX_PSP_LIST_COUNT&&g_psp_lists[list].v){ PspList *l=&g_psp_lists[list]; GLuint old=g_psp_bound_tex; int ote=g_psp_texture_enabled, ob=g_psp_blend_enabled, od=g_psp_depth_enabled, oa=g_psp_alpha_enabled; g_psp_bound_tex=l->tex; g_psp_texture_enabled=l->texture_enabled; g_psp_blend_enabled=l->blend_enabled; g_psp_depth_enabled=l->depth_enabled; g_psp_alpha_enabled=l->alpha_enabled; psp_draw_vertex_array(l->v,l->count,l->mode); g_psp_bound_tex=old; g_psp_texture_enabled=ote; g_psp_blend_enabled=ob; g_psp_depth_enabled=od; g_psp_alpha_enabled=oa; } }
static void glGenTextures(GLsizei n, GLuint *tex){ for(int i=0;i<n;i++){ GLuint id=g_psp_next_tex++; if(id>=PEX_PSP_MAX_TEXTURES)id=1; tex[i]=id; memset(&g_psp_textures[id],0,sizeof(g_psp_textures[id])); } }
static void glBindTexture(GLenum target, GLuint tex){ (void)target; g_psp_bound_tex=tex; }
static void glTexParameteri(GLenum target, GLenum pname, GLint param){ (void)target; (void)pname; (void)param; }
static void glTexImage2D(GLenum target, GLint level, GLint internal, GLsizei w, GLsizei h, GLint border, GLenum fmt, GLenum type, const void *pixels){ (void)target;(void)level;(void)internal;(void)border;(void)fmt;(void)type; if(!g_psp_bound_tex||g_psp_bound_tex>=PEX_PSP_MAX_TEXTURES||!pixels)return; PspTexture *t=&g_psp_textures[g_psp_bound_tex]; free(t->pixels); t->w=w; t->h=h; t->stride=w; t->pixels=(unsigned int*)memalign(16,(size_t)w*h*4); if(t->pixels){ memcpy(t->pixels,pixels,(size_t)w*h*4); sceKernelDcacheWritebackInvalidateRange(t->pixels,(size_t)w*h*4); t->used=1; } }
static void glDeleteTextures(GLsizei n, const GLuint *tex){ for(int i=0;i<n;i++){ GLuint id=tex[i]; if(id<PEX_PSP_MAX_TEXTURES){ free(g_psp_textures[id].pixels); memset(&g_psp_textures[id],0,sizeof(g_psp_textures[id])); } } }
static void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff, GLint x, GLint y, GLsizei w, GLsizei h){ (void)target;(void)level;(void)xoff;(void)yoff;(void)x;(void)y;(void)w;(void)h; }
static const GLubyte *glGetString(GLenum name){ if(name==GL_VENDOR)return (const GLubyte*)"Sony"; if(name==GL_RENDERER)return (const GLubyte*)"PSP Graphics Engine (GU)"; if(name==GL_VERSION)return (const GLubyte*)"PSP GU fixed-function"; return (const GLubyte*)""; }
