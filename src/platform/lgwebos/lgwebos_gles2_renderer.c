/* LG webOS OpenGL ES 2.0 fixed-function compatibility shim.
   This file is intentionally platform-local.  It emulates just enough of the
   old OpenGL 1.x API used by PexCraft so world/UI code can remain untouched. */

#define PEX_LGWEBOS_MAX_IMM_VERTS 262144
#define PEX_LGWEBOS_MAX_LISTS 8192

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

typedef struct LgWebOSVertex {
    GLfloat x, y, z;
    GLfloat u, v;
    GLfloat r, g, b, a;
} LgWebOSVertex;

typedef struct LgWebOSMat4 { GLfloat m[16]; } LgWebOSMat4;

typedef struct LgWebOSBatch {
    LgWebOSVertex *v;
    GLsizei count;
    GLenum mode;
    GLuint tex;
    int texture_enabled;
    int blend_enabled;
    int depth_enabled;
    int alpha_enabled;
    int cull_enabled;
    GLfloat alpha_ref;
    struct LgWebOSBatch *next;
} LgWebOSBatch;

typedef struct LgWebOSList { LgWebOSBatch *first, *last; } LgWebOSList;

static GLuint g_lgwebos_program = 0;
static GLuint g_lgwebos_vbo = 0;
static GLuint g_lgwebos_ebo = 0;
static GLint g_lgwebos_a_pos = -1;
static GLint g_lgwebos_a_uv = -1;
static GLint g_lgwebos_a_color = -1;
static GLint g_lgwebos_u_mvp = -1;
static GLint g_lgwebos_u_tex = -1;
static GLint g_lgwebos_u_use_tex = -1;
static GLint g_lgwebos_u_alpha_enabled = -1;
static GLint g_lgwebos_u_alpha_ref = -1;

static int g_lgwebos_texture_enabled = 1;
static int g_lgwebos_blend_enabled = 1;
static int g_lgwebos_depth_enabled = 0;
static int g_lgwebos_alpha_enabled = 0;
static int g_lgwebos_cull_enabled = 0;
static GLfloat g_lgwebos_alpha_ref = 0.1f;
static GLuint g_lgwebos_bound_tex = 0;
static GLfloat g_lgwebos_cur_u = 0.0f, g_lgwebos_cur_v = 0.0f;
static GLfloat g_lgwebos_cur_color[4] = {1,1,1,1};
static GLint g_lgwebos_viewport[4] = {0,0,854,480};

static GLenum g_lgwebos_begin_mode = 0;
static int g_lgwebos_begin_active = 0;
static LgWebOSVertex *g_lgwebos_imm = NULL;
static int g_lgwebos_imm_count = 0;
static int g_lgwebos_imm_cap = 0;

static LgWebOSList g_lgwebos_lists[PEX_LGWEBOS_MAX_LISTS];
static GLuint g_lgwebos_next_list = 1;
static int g_lgwebos_compiling = 0;
static GLuint g_lgwebos_compile_list = 0;

static GLenum g_lgwebos_matrix_mode = GL_MODELVIEW;
static LgWebOSMat4 g_lgwebos_model_stack[64];
static int g_lgwebos_model_top = 0;
static LgWebOSMat4 g_lgwebos_proj_stack[32];
static int g_lgwebos_proj_top = 0;

static const GLvoid *g_lgwebos_vertex_ptr = NULL;
static const GLvoid *g_lgwebos_texcoord_ptr = NULL;
static const GLvoid *g_lgwebos_color_ptr = NULL;
static GLint g_lgwebos_vertex_size = 3;
static GLint g_lgwebos_texcoord_size = 2;
static GLint g_lgwebos_color_size = 4;
static GLenum g_lgwebos_vertex_type = GL_FLOAT;
static GLenum g_lgwebos_texcoord_type = GL_FLOAT;
static GLenum g_lgwebos_color_type = GL_UNSIGNED_BYTE;
static GLsizei g_lgwebos_vertex_stride = 0;
static GLsizei g_lgwebos_texcoord_stride = 0;
static GLsizei g_lgwebos_color_stride = 0;
static int g_lgwebos_vertex_array_enabled = 0;
static int g_lgwebos_texcoord_array_enabled = 0;
static int g_lgwebos_color_array_enabled = 0;
static GLushort *g_lgwebos_index16_tmp = NULL;
static int g_lgwebos_index16_cap = 0;

static void lgwebos_mat_identity(LgWebOSMat4 *m) {
    memset(m->m, 0, sizeof(m->m));
    m->m[0] = m->m[5] = m->m[10] = m->m[15] = 1.0f;
}

static void lgwebos_mat_mul(LgWebOSMat4 *out, const LgWebOSMat4 *a, const LgWebOSMat4 *b) {
    LgWebOSMat4 r;
    for (int c = 0; c < 4; ++c) {
        for (int row = 0; row < 4; ++row) {
            r.m[c*4 + row] =
                a->m[0*4 + row] * b->m[c*4 + 0] +
                a->m[1*4 + row] * b->m[c*4 + 1] +
                a->m[2*4 + row] * b->m[c*4 + 2] +
                a->m[3*4 + row] * b->m[c*4 + 3];
        }
    }
    *out = r;
}

static LgWebOSMat4 *lgwebos_current_matrix(void) {
    return (g_lgwebos_matrix_mode == GL_PROJECTION) ? &g_lgwebos_proj_stack[g_lgwebos_proj_top] : &g_lgwebos_model_stack[g_lgwebos_model_top];
}

static void lgwebos_mat_postmul(LgWebOSMat4 *cur, const LgWebOSMat4 *rhs) {
    LgWebOSMat4 out;
    lgwebos_mat_mul(&out, cur, rhs);
    *cur = out;
}

static void lgwebos_make_mvp(LgWebOSMat4 *out) {
    lgwebos_mat_mul(out, &g_lgwebos_proj_stack[g_lgwebos_proj_top], &g_lgwebos_model_stack[g_lgwebos_model_top]);
}

static int lgwebos_ensure_imm(int needed) {
    if (needed <= g_lgwebos_imm_cap) return 1;
    int cap = g_lgwebos_imm_cap ? g_lgwebos_imm_cap : 4096;
    while (cap < needed) {
        if (cap > PEX_LGWEBOS_MAX_IMM_VERTS / 2) { cap = PEX_LGWEBOS_MAX_IMM_VERTS; break; }
        cap *= 2;
    }
    if (needed > cap) return 0;
    LgWebOSVertex *nv = (LgWebOSVertex*)realloc(g_lgwebos_imm, (size_t)cap * sizeof(*nv));
    if (!nv) return 0;
    g_lgwebos_imm = nv;
    g_lgwebos_imm_cap = cap;
    return 1;
}

static GLuint lgwebos_compile_shader(GLenum type, const char *src) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]; GLsizei len = 0;
        glGetShaderInfoLog(sh, sizeof(log), &len, log);
        fprintf(stderr, "LG webOS GLES shader compile failed: %s\n", log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

static int pex_lgwebos_gles2_init(void) {
    const char *vs =
        "attribute vec3 a_pos;\n"
        "attribute vec2 a_uv;\n"
        "attribute vec4 a_color;\n"
        "uniform mat4 u_mvp;\n"
        "varying vec2 v_uv;\n"
        "varying vec4 v_color;\n"
        "void main(){ gl_Position = u_mvp * vec4(a_pos, 1.0); v_uv = a_uv; v_color = a_color; }\n";
    const char *fs =
        "precision mediump float;\n"
        "uniform sampler2D u_tex;\n"
        "uniform int u_use_tex;\n"
        "uniform int u_alpha_enabled;\n"
        "uniform float u_alpha_ref;\n"
        "varying vec2 v_uv;\n"
        "varying vec4 v_color;\n"
        "void main(){ vec4 c = v_color; if(u_use_tex == 1) c *= texture2D(u_tex, v_uv); if(u_alpha_enabled == 1 && c.a <= u_alpha_ref) discard; gl_FragColor = c; }\n";
    GLuint v = lgwebos_compile_shader(GL_VERTEX_SHADER, vs);
    GLuint f = lgwebos_compile_shader(GL_FRAGMENT_SHADER, fs);
    if (!v || !f) return 0;
    g_lgwebos_program = glCreateProgram();
    glAttachShader(g_lgwebos_program, v);
    glAttachShader(g_lgwebos_program, f);
    glLinkProgram(g_lgwebos_program);
    glDeleteShader(v);
    glDeleteShader(f);
    GLint ok = 0;
    glGetProgramiv(g_lgwebos_program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024]; GLsizei len = 0;
        glGetProgramInfoLog(g_lgwebos_program, sizeof(log), &len, log);
        fprintf(stderr, "LG webOS GLES program link failed: %s\n", log);
        return 0;
    }
    g_lgwebos_a_pos = glGetAttribLocation(g_lgwebos_program, "a_pos");
    g_lgwebos_a_uv = glGetAttribLocation(g_lgwebos_program, "a_uv");
    g_lgwebos_a_color = glGetAttribLocation(g_lgwebos_program, "a_color");
    g_lgwebos_u_mvp = glGetUniformLocation(g_lgwebos_program, "u_mvp");
    g_lgwebos_u_tex = glGetUniformLocation(g_lgwebos_program, "u_tex");
    g_lgwebos_u_use_tex = glGetUniformLocation(g_lgwebos_program, "u_use_tex");
    g_lgwebos_u_alpha_enabled = glGetUniformLocation(g_lgwebos_program, "u_alpha_enabled");
    g_lgwebos_u_alpha_ref = glGetUniformLocation(g_lgwebos_program, "u_alpha_ref");
    glGenBuffers(1, &g_lgwebos_vbo);
    glGenBuffers(1, &g_lgwebos_ebo);
    lgwebos_mat_identity(&g_lgwebos_model_stack[0]);
    lgwebos_mat_identity(&g_lgwebos_proj_stack[0]);
    glUseProgram(g_lgwebos_program);
    if (g_lgwebos_u_tex >= 0) glUniform1i(g_lgwebos_u_tex, 0);
    return 1;
}

static void lgwebos_apply_state_for_batch(const LgWebOSBatch *b) {
    int tex_enabled = b ? b->texture_enabled : g_lgwebos_texture_enabled;
    int blend_enabled = b ? b->blend_enabled : g_lgwebos_blend_enabled;
    int depth_enabled = b ? b->depth_enabled : g_lgwebos_depth_enabled;
    int alpha_enabled = b ? b->alpha_enabled : g_lgwebos_alpha_enabled;
    int cull_enabled = b ? b->cull_enabled : g_lgwebos_cull_enabled;
    GLuint tex = b ? b->tex : g_lgwebos_bound_tex;
    GLfloat alpha_ref = b ? b->alpha_ref : g_lgwebos_alpha_ref;

    if (blend_enabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (depth_enabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (cull_enabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (tex_enabled && tex) ? tex : 0);
    if (g_lgwebos_u_use_tex >= 0) glUniform1i(g_lgwebos_u_use_tex, (tex_enabled && tex) ? 1 : 0);
    if (g_lgwebos_u_alpha_enabled >= 0) glUniform1i(g_lgwebos_u_alpha_enabled, alpha_enabled ? 1 : 0);
    if (g_lgwebos_u_alpha_ref >= 0) glUniform1f(g_lgwebos_u_alpha_ref, alpha_ref);
}

static void lgwebos_draw_raw(const LgWebOSVertex *v, GLsizei count, GLenum mode, const LgWebOSBatch *state) {
    if (!v || count <= 0 || !g_lgwebos_program) return;
    LgWebOSMat4 mvp;
    lgwebos_make_mvp(&mvp);
    glUseProgram(g_lgwebos_program);
    if (g_lgwebos_u_mvp >= 0) glUniformMatrix4fv(g_lgwebos_u_mvp, 1, GL_FALSE, mvp.m);
    lgwebos_apply_state_for_batch(state);
    glBindBuffer(GL_ARRAY_BUFFER, g_lgwebos_vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)((size_t)count * sizeof(LgWebOSVertex)), v, GL_DYNAMIC_DRAW);
    if (g_lgwebos_a_pos >= 0) { glEnableVertexAttribArray((GLuint)g_lgwebos_a_pos); glVertexAttribPointer((GLuint)g_lgwebos_a_pos, 3, GL_FLOAT, GL_FALSE, sizeof(LgWebOSVertex), (const GLvoid*)offsetof(LgWebOSVertex, x)); }
    if (g_lgwebos_a_uv >= 0) { glEnableVertexAttribArray((GLuint)g_lgwebos_a_uv); glVertexAttribPointer((GLuint)g_lgwebos_a_uv, 2, GL_FLOAT, GL_FALSE, sizeof(LgWebOSVertex), (const GLvoid*)offsetof(LgWebOSVertex, u)); }
    if (g_lgwebos_a_color >= 0) { glEnableVertexAttribArray((GLuint)g_lgwebos_a_color); glVertexAttribPointer((GLuint)g_lgwebos_a_color, 4, GL_FLOAT, GL_FALSE, sizeof(LgWebOSVertex), (const GLvoid*)offsetof(LgWebOSVertex, r)); }
    glDrawArrays(mode, 0, count);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static LgWebOSBatch *lgwebos_batch_create(const LgWebOSVertex *src, int count, GLenum mode) {
    if (!src || count <= 0) return NULL;
    int out_count = count;
    GLenum out_mode = mode;
    if (mode == GL_QUADS) {
        int q = count / 4;
        if (q <= 0) return NULL;
        out_count = q * 6;
        out_mode = GL_TRIANGLES;
    }
    LgWebOSBatch *b = (LgWebOSBatch*)calloc(1, sizeof(*b));
    if (!b) return NULL;
    b->v = (LgWebOSVertex*)malloc((size_t)out_count * sizeof(*b->v));
    if (!b->v) { free(b); return NULL; }
    b->count = (GLsizei)out_count;
    b->mode = out_mode;
    b->tex = g_lgwebos_bound_tex;
    b->texture_enabled = g_lgwebos_texture_enabled;
    b->blend_enabled = g_lgwebos_blend_enabled;
    b->depth_enabled = g_lgwebos_depth_enabled;
    b->alpha_enabled = g_lgwebos_alpha_enabled;
    b->cull_enabled = g_lgwebos_cull_enabled;
    b->alpha_ref = g_lgwebos_alpha_ref;
    if (mode == GL_QUADS) {
        int n = 0;
        int q = count / 4;
        for (int i = 0; i < q; ++i) {
            const LgWebOSVertex *v = src + i * 4;
            b->v[n++] = v[0]; b->v[n++] = v[1]; b->v[n++] = v[2];
            b->v[n++] = v[0]; b->v[n++] = v[2]; b->v[n++] = v[3];
        }
    } else {
        memcpy(b->v, src, (size_t)out_count * sizeof(*b->v));
    }
    return b;
}

static void lgwebos_batch_free_chain(LgWebOSBatch *b) {
    while (b) {
        LgWebOSBatch *n = b->next;
        free(b->v);
        free(b);
        b = n;
    }
}

static void lgwebos_list_free(GLuint list) {
    if (list == 0 || list >= PEX_LGWEBOS_MAX_LISTS) return;
    lgwebos_batch_free_chain(g_lgwebos_lists[list].first);
    memset(&g_lgwebos_lists[list], 0, sizeof(g_lgwebos_lists[list]));
}

static void lgwebos_list_append(GLuint list, LgWebOSBatch *b) {
    if (!b || list == 0 || list >= PEX_LGWEBOS_MAX_LISTS) { if (b) { free(b->v); free(b); } return; }
    LgWebOSList *l = &g_lgwebos_lists[list];
    if (!l->first) l->first = b;
    else l->last->next = b;
    l->last = b;
}

static void lgwebos_draw_or_store_current(void) {
    LgWebOSBatch *b = lgwebos_batch_create(g_lgwebos_imm, g_lgwebos_imm_count, g_lgwebos_begin_mode);
    if (!b) return;
    if (g_lgwebos_compiling) lgwebos_list_append(g_lgwebos_compile_list, b);
    else {
        lgwebos_draw_raw(b->v, b->count, b->mode, b);
        free(b->v);
        free(b);
    }
}

static int lgwebos_sizeof_type(GLenum type) {
    switch (type) {
        case GL_FLOAT: return 4;
        case GL_UNSIGNED_BYTE: return 1;
        case GL_UNSIGNED_SHORT: return 2;
        case GL_UNSIGNED_INT: return 4;
        default: return 4;
    }
}

static const unsigned char *lgwebos_elem_ptr(const GLvoid *base, int index, GLsizei stride, int comps, GLenum type) {
    int sz = lgwebos_sizeof_type(type);
    if (stride <= 0) stride = (GLsizei)(comps * sz);
    return ((const unsigned char*)base) + (size_t)index * (size_t)stride;
}

static float lgwebos_read_component(const unsigned char *p, GLenum type, int normalized) {
    if (type == GL_FLOAT) return *(const GLfloat*)p;
    if (type == GL_UNSIGNED_BYTE) return normalized ? ((float)(*(const GLubyte*)p) / 255.0f) : (float)(*(const GLubyte*)p);
    if (type == GL_UNSIGNED_SHORT) return normalized ? ((float)(*(const GLushort*)p) / 65535.0f) : (float)(*(const GLushort*)p);
    return 0.0f;
}

static unsigned int lgwebos_read_index(const GLvoid *indices, GLenum type, int i) {
    if (type == GL_UNSIGNED_SHORT) return (unsigned int)((const GLushort*)indices)[i];
    if (type == GL_UNSIGNED_INT) return (unsigned int)((const GLuint*)indices)[i];
    return (unsigned int)((const GLubyte*)indices)[i];
}

static LgWebOSVertex lgwebos_vertex_from_arrays(unsigned int idx) {
    LgWebOSVertex v;
    memset(&v, 0, sizeof(v));
    v.r = g_lgwebos_cur_color[0]; v.g = g_lgwebos_cur_color[1]; v.b = g_lgwebos_cur_color[2]; v.a = g_lgwebos_cur_color[3];
    v.u = g_lgwebos_cur_u; v.v = g_lgwebos_cur_v;
    if (g_lgwebos_vertex_array_enabled && g_lgwebos_vertex_ptr) {
        const unsigned char *p = lgwebos_elem_ptr(g_lgwebos_vertex_ptr, (int)idx, g_lgwebos_vertex_stride, g_lgwebos_vertex_size, g_lgwebos_vertex_type);
        v.x = lgwebos_read_component(p + 0 * lgwebos_sizeof_type(g_lgwebos_vertex_type), g_lgwebos_vertex_type, 0);
        if (g_lgwebos_vertex_size > 1) v.y = lgwebos_read_component(p + 1 * lgwebos_sizeof_type(g_lgwebos_vertex_type), g_lgwebos_vertex_type, 0);
        if (g_lgwebos_vertex_size > 2) v.z = lgwebos_read_component(p + 2 * lgwebos_sizeof_type(g_lgwebos_vertex_type), g_lgwebos_vertex_type, 0);
    }
    if (g_lgwebos_texcoord_array_enabled && g_lgwebos_texcoord_ptr) {
        const unsigned char *p = lgwebos_elem_ptr(g_lgwebos_texcoord_ptr, (int)idx, g_lgwebos_texcoord_stride, g_lgwebos_texcoord_size, g_lgwebos_texcoord_type);
        v.u = lgwebos_read_component(p + 0 * lgwebos_sizeof_type(g_lgwebos_texcoord_type), g_lgwebos_texcoord_type, 0);
        if (g_lgwebos_texcoord_size > 1) v.v = lgwebos_read_component(p + 1 * lgwebos_sizeof_type(g_lgwebos_texcoord_type), g_lgwebos_texcoord_type, 0);
    }
    if (g_lgwebos_color_array_enabled && g_lgwebos_color_ptr) {
        const unsigned char *p = lgwebos_elem_ptr(g_lgwebos_color_ptr, (int)idx, g_lgwebos_color_stride, g_lgwebos_color_size, g_lgwebos_color_type);
        int sz = lgwebos_sizeof_type(g_lgwebos_color_type);
        v.r = lgwebos_read_component(p + 0 * sz, g_lgwebos_color_type, 1);
        if (g_lgwebos_color_size > 1) v.g = lgwebos_read_component(p + 1 * sz, g_lgwebos_color_type, 1);
        if (g_lgwebos_color_size > 2) v.b = lgwebos_read_component(p + 2 * sz, g_lgwebos_color_type, 1);
        if (g_lgwebos_color_size > 3) v.a = lgwebos_read_component(p + 3 * sz, g_lgwebos_color_type, 1);
    }
    return v;
}

static void pex_lgwebos_glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { glClearColor(r,g,b,a); }
static void pex_lgwebos_glClear(GLbitfield mask) { glClear(mask); }
static void pex_lgwebos_glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) { glColorMask(r,g,b,a); }
static void pex_lgwebos_glReadBuffer(GLenum mode) { (void)mode; }
static void pex_lgwebos_glReadPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum format, GLenum type, GLvoid *pixels) { glReadPixels(x,y,w,h,format,type,pixels); }
static void pex_lgwebos_glLineWidth(GLfloat w) { glLineWidth(w); }
static void pex_lgwebos_glBlendFunc(GLenum s, GLenum d) { glBlendFunc(s,d); }
static void pex_lgwebos_glDepthFunc(GLenum f) { glDepthFunc(f); }
static void pex_lgwebos_glDepthMask(GLboolean flag) { glDepthMask(flag); }
static void pex_lgwebos_glAlphaFunc(GLenum func, GLfloat ref) { (void)func; g_lgwebos_alpha_ref = ref; }
static void pex_lgwebos_glViewport(GLint x, GLint y, GLsizei w, GLsizei h) { g_lgwebos_viewport[0]=x; g_lgwebos_viewport[1]=y; g_lgwebos_viewport[2]=w; g_lgwebos_viewport[3]=h; glViewport(x,y,w,h); }

static void pex_lgwebos_glEnable(GLenum cap) {
    if (cap == GL_TEXTURE_2D) g_lgwebos_texture_enabled = 1;
    else if (cap == GL_ALPHA_TEST) g_lgwebos_alpha_enabled = 1;
    else if (cap == GL_BLEND) { g_lgwebos_blend_enabled = 1; glEnable(GL_BLEND); }
    else if (cap == GL_DEPTH_TEST) { g_lgwebos_depth_enabled = 1; glEnable(GL_DEPTH_TEST); }
    else if (cap == GL_CULL_FACE) { g_lgwebos_cull_enabled = 1; glEnable(GL_CULL_FACE); }
    else glEnable(cap);
}

static void pex_lgwebos_glDisable(GLenum cap) {
    if (cap == GL_TEXTURE_2D) g_lgwebos_texture_enabled = 0;
    else if (cap == GL_ALPHA_TEST) g_lgwebos_alpha_enabled = 0;
    else if (cap == GL_BLEND) { g_lgwebos_blend_enabled = 0; glDisable(GL_BLEND); }
    else if (cap == GL_DEPTH_TEST) { g_lgwebos_depth_enabled = 0; glDisable(GL_DEPTH_TEST); }
    else if (cap == GL_CULL_FACE) { g_lgwebos_cull_enabled = 0; glDisable(GL_CULL_FACE); }
    else if (cap == GL_FOG) { }
    else glDisable(cap);
}

static void pex_lgwebos_glMatrixMode(GLenum mode) { g_lgwebos_matrix_mode = mode; }
static void pex_lgwebos_glLoadIdentity(void) { lgwebos_mat_identity(lgwebos_current_matrix()); }
static void pex_lgwebos_glPushMatrix(void) {
    if (g_lgwebos_matrix_mode == GL_PROJECTION) { if (g_lgwebos_proj_top < 31) { g_lgwebos_proj_stack[g_lgwebos_proj_top+1] = g_lgwebos_proj_stack[g_lgwebos_proj_top]; g_lgwebos_proj_top++; } }
    else { if (g_lgwebos_model_top < 63) { g_lgwebos_model_stack[g_lgwebos_model_top+1] = g_lgwebos_model_stack[g_lgwebos_model_top]; g_lgwebos_model_top++; } }
}
static void pex_lgwebos_glPopMatrix(void) { if (g_lgwebos_matrix_mode == GL_PROJECTION) { if (g_lgwebos_proj_top > 0) g_lgwebos_proj_top--; } else { if (g_lgwebos_model_top > 0) g_lgwebos_model_top--; } }
static void pex_lgwebos_glTranslatef(GLfloat x, GLfloat y, GLfloat z) { LgWebOSMat4 t; lgwebos_mat_identity(&t); t.m[12]=x; t.m[13]=y; t.m[14]=z; lgwebos_mat_postmul(lgwebos_current_matrix(), &t); }
static void pex_lgwebos_glScalef(GLfloat x, GLfloat y, GLfloat z) { LgWebOSMat4 s; lgwebos_mat_identity(&s); s.m[0]=x; s.m[5]=y; s.m[10]=z; lgwebos_mat_postmul(lgwebos_current_matrix(), &s); }
static void pex_lgwebos_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    GLfloat len = sqrtf(x*x + y*y + z*z);
    if (len <= 0.00001f) return;
    x /= len; y /= len; z /= len;
    GLfloat rad = angle * (GLfloat)(M_PI / 180.0);
    GLfloat c = cosf(rad), s = sinf(rad), ic = 1.0f - c;
    LgWebOSMat4 r;
    r.m[0]=x*x*ic+c;   r.m[4]=x*y*ic-z*s; r.m[8] =x*z*ic+y*s; r.m[12]=0;
    r.m[1]=y*x*ic+z*s; r.m[5]=y*y*ic+c;   r.m[9] =y*z*ic-x*s; r.m[13]=0;
    r.m[2]=z*x*ic-y*s; r.m[6]=z*y*ic+x*s; r.m[10]=z*z*ic+c;   r.m[14]=0;
    r.m[3]=0;          r.m[7]=0;          r.m[11]=0;          r.m[15]=1;
    lgwebos_mat_postmul(lgwebos_current_matrix(), &r);
}
static void pex_lgwebos_glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) {
    LgWebOSMat4 o; lgwebos_mat_identity(&o);
    o.m[0] = (GLfloat)(2.0 / (r - l));
    o.m[5] = (GLfloat)(2.0 / (t - b));
    o.m[10] = (GLfloat)(-2.0 / (f - n));
    o.m[12] = (GLfloat)(-(r + l) / (r - l));
    o.m[13] = (GLfloat)(-(t + b) / (t - b));
    o.m[14] = (GLfloat)(-(f + n) / (f - n));
    lgwebos_mat_postmul(lgwebos_current_matrix(), &o);
}
static void pex_lgwebos_gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble znear, GLdouble zfar) {
    GLdouble ymax = znear * tan(fovy * M_PI / 360.0);
    GLdouble xmax = ymax * aspect;
    GLdouble l = -xmax, r = xmax, b = -ymax, t = ymax;
    LgWebOSMat4 p; memset(&p, 0, sizeof(p));
    p.m[0] = (GLfloat)((2.0 * znear) / (r - l));
    p.m[5] = (GLfloat)((2.0 * znear) / (t - b));
    p.m[8] = (GLfloat)((r + l) / (r - l));
    p.m[9] = (GLfloat)((t + b) / (t - b));
    p.m[10] = (GLfloat)(-(zfar + znear) / (zfar - znear));
    p.m[11] = -1.0f;
    p.m[14] = (GLfloat)(-(2.0 * zfar * znear) / (zfar - znear));
    lgwebos_mat_postmul(lgwebos_current_matrix(), &p);
}

static void pex_lgwebos_glGetFloatv(GLenum pname, GLfloat *params) { if (!params) return; if (pname == GL_PROJECTION_MATRIX) memcpy(params, g_lgwebos_proj_stack[g_lgwebos_proj_top].m, 16*sizeof(GLfloat)); else if (pname == GL_MODELVIEW_MATRIX) memcpy(params, g_lgwebos_model_stack[g_lgwebos_model_top].m, 16*sizeof(GLfloat)); else glGetFloatv(pname, params); }
static void pex_lgwebos_glGetDoublev(GLenum pname, GLdouble *params) { if (!params) return; GLfloat f[16]; pex_lgwebos_glGetFloatv(pname, f); for (int i=0;i<16;i++) params[i] = (GLdouble)f[i]; }
static void pex_lgwebos_glGetIntegerv(GLenum pname, GLint *params) { if (!params) return; if (pname == GL_VIEWPORT) memcpy(params, g_lgwebos_viewport, sizeof(g_lgwebos_viewport)); else glGetIntegerv(pname, params); }
static void pex_lgwebos_glFogi(GLenum pname, GLint param) { (void)pname; (void)param; }
static void pex_lgwebos_glFogf(GLenum pname, GLfloat param) { (void)pname; (void)param; }
static void pex_lgwebos_glFogfv(GLenum pname, const GLfloat *params) { (void)pname; (void)params; }

static void pex_lgwebos_glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { g_lgwebos_cur_color[0]=r; g_lgwebos_cur_color[1]=g; g_lgwebos_cur_color[2]=b; g_lgwebos_cur_color[3]=a; }
static void pex_lgwebos_glTexCoord2f(GLfloat u, GLfloat v) { g_lgwebos_cur_u = u; g_lgwebos_cur_v = v; }
static void pex_lgwebos_glBegin(GLenum mode) { g_lgwebos_begin_active = 1; g_lgwebos_begin_mode = mode; g_lgwebos_imm_count = 0; }
static void pex_lgwebos_glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
    if (!g_lgwebos_begin_active) return;
    if (!lgwebos_ensure_imm(g_lgwebos_imm_count + 1)) return;
    LgWebOSVertex *v = &g_lgwebos_imm[g_lgwebos_imm_count++];
    v->x = x; v->y = y; v->z = z;
    v->u = g_lgwebos_cur_u; v->v = g_lgwebos_cur_v;
    v->r = g_lgwebos_cur_color[0]; v->g = g_lgwebos_cur_color[1]; v->b = g_lgwebos_cur_color[2]; v->a = g_lgwebos_cur_color[3];
}
static void pex_lgwebos_glVertex2f(GLfloat x, GLfloat y) { pex_lgwebos_glVertex3f(x, y, 0.0f); }
static void pex_lgwebos_glEnd(void) { if (!g_lgwebos_begin_active) return; g_lgwebos_begin_active = 0; lgwebos_draw_or_store_current(); }

static GLuint pex_lgwebos_glGenLists(GLsizei range) { GLuint base = g_lgwebos_next_list; int r = range > 0 ? range : 1; for (int i=0;i<r;i++) if (base + (GLuint)i < PEX_LGWEBOS_MAX_LISTS) lgwebos_list_free(base + (GLuint)i); g_lgwebos_next_list += (GLuint)r; if (g_lgwebos_next_list >= PEX_LGWEBOS_MAX_LISTS) g_lgwebos_next_list = PEX_LGWEBOS_MAX_LISTS - 1; return base; }
static void pex_lgwebos_glNewList(GLuint list, GLenum mode) { (void)mode; if (list > 0 && list < PEX_LGWEBOS_MAX_LISTS) lgwebos_list_free(list); g_lgwebos_compiling = 1; g_lgwebos_compile_list = list; }
static void pex_lgwebos_glEndList(void) { g_lgwebos_compiling = 0; g_lgwebos_compile_list = 0; }
static void pex_lgwebos_glCallList(GLuint list) { if (list == 0 || list >= PEX_LGWEBOS_MAX_LISTS) return; for (LgWebOSBatch *b = g_lgwebos_lists[list].first; b; b = b->next) lgwebos_draw_raw(b->v, b->count, b->mode, b); }

static void pex_lgwebos_glGenTextures(GLsizei n, GLuint *textures) { glGenTextures(n, textures); }
static void pex_lgwebos_glBindTexture(GLenum target, GLuint texture) { if (target == GL_TEXTURE_2D) g_lgwebos_bound_tex = texture; glBindTexture(target, texture); }
static void pex_lgwebos_glTexParameteri(GLenum target, GLenum pname, GLint param) { if (param == GL_CLAMP) param = GL_CLAMP_TO_EDGE; glTexParameteri(target, pname, param); }
static void pex_lgwebos_glTexImage2D(GLenum target, GLint level, GLint internal, GLsizei w, GLsizei h, GLint border, GLenum format, GLenum type, const GLvoid *pixels) { if (internal == 4) internal = GL_RGBA; if (format == GL_BGRA) format = GL_RGBA; glTexImage2D(target, level, internal, w, h, border, format, type, pixels); }
static void pex_lgwebos_glDeleteTextures(GLsizei n, const GLuint *textures) { glDeleteTextures(n, textures); }
static void pex_lgwebos_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff, GLint x, GLint y, GLsizei w, GLsizei h) { glCopyTexSubImage2D(target, level, xoff, yoff, x, y, w, h); }
static void pex_lgwebos_glPixelStorei(GLenum pname, GLint param) { glPixelStorei(pname, param); }
static const GLubyte *pex_lgwebos_glGetString(GLenum name) { return glGetString(name); }

static void pex_lgwebos_glEnableClientState(GLenum array) { if (array == GL_VERTEX_ARRAY) g_lgwebos_vertex_array_enabled = 1; else if (array == GL_TEXTURE_COORD_ARRAY) g_lgwebos_texcoord_array_enabled = 1; else if (array == GL_COLOR_ARRAY) g_lgwebos_color_array_enabled = 1; }
static void pex_lgwebos_glDisableClientState(GLenum array) { if (array == GL_VERTEX_ARRAY) g_lgwebos_vertex_array_enabled = 0; else if (array == GL_TEXTURE_COORD_ARRAY) g_lgwebos_texcoord_array_enabled = 0; else if (array == GL_COLOR_ARRAY) g_lgwebos_color_array_enabled = 0; }
static void pex_lgwebos_glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) { g_lgwebos_vertex_size=size; g_lgwebos_vertex_type=type; g_lgwebos_vertex_stride=stride; g_lgwebos_vertex_ptr=pointer; }
static void pex_lgwebos_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) { g_lgwebos_texcoord_size=size; g_lgwebos_texcoord_type=type; g_lgwebos_texcoord_stride=stride; g_lgwebos_texcoord_ptr=pointer; }
static void pex_lgwebos_glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) { g_lgwebos_color_size=size; g_lgwebos_color_type=type; g_lgwebos_color_stride=stride; g_lgwebos_color_ptr=pointer; }
static int lgwebos_ensure_index16_tmp(int needed) {
    if (needed <= g_lgwebos_index16_cap) return 1;
    int cap = g_lgwebos_index16_cap ? g_lgwebos_index16_cap : 4096;
    while (cap < needed) cap *= 2;
    GLushort *ni = (GLushort*)realloc(g_lgwebos_index16_tmp, (size_t)cap * sizeof(*ni));
    if (!ni) return 0;
    g_lgwebos_index16_tmp = ni;
    g_lgwebos_index16_cap = cap;
    return 1;
}

static int lgwebos_arrays_are_pexvertex_interleaved(const unsigned char **base_out, GLsizei *stride_out) {
    if (!g_lgwebos_vertex_array_enabled || !g_lgwebos_texcoord_array_enabled || !g_lgwebos_color_array_enabled) return 0;
    if (!g_lgwebos_vertex_ptr || !g_lgwebos_texcoord_ptr || !g_lgwebos_color_ptr) return 0;
    if (g_lgwebos_vertex_size != 3 || g_lgwebos_vertex_type != GL_FLOAT) return 0;
    if (g_lgwebos_texcoord_size != 2 || g_lgwebos_texcoord_type != GL_FLOAT) return 0;
    if (g_lgwebos_color_size != 4 || g_lgwebos_color_type != GL_UNSIGNED_BYTE) return 0;
    GLsizei vs = g_lgwebos_vertex_stride ? g_lgwebos_vertex_stride : (GLsizei)(3 * sizeof(GLfloat));
    GLsizei ts = g_lgwebos_texcoord_stride ? g_lgwebos_texcoord_stride : (GLsizei)(2 * sizeof(GLfloat));
    GLsizei cs = g_lgwebos_color_stride ? g_lgwebos_color_stride : (GLsizei)(4 * sizeof(GLubyte));
    if (vs != (GLsizei)sizeof(PexVertex) || ts != (GLsizei)sizeof(PexVertex) || cs != (GLsizei)sizeof(PexVertex)) return 0;
    const unsigned char *base = ((const unsigned char*)g_lgwebos_vertex_ptr) - offsetof(PexVertex, x);
    if ((const unsigned char*)g_lgwebos_texcoord_ptr != base + offsetof(PexVertex, u)) return 0;
    if ((const unsigned char*)g_lgwebos_color_ptr != base + offsetof(PexVertex, color)) return 0;
    if (base_out) *base_out = base;
    if (stride_out) *stride_out = vs;
    return 1;
}

static int lgwebos_draw_elements_fast(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
    if (!g_lgwebos_program || !g_lgwebos_vbo || !g_lgwebos_ebo) return 0;
    if (mode != GL_TRIANGLES && mode != GL_LINES && mode != GL_LINE_STRIP && mode != GL_TRIANGLE_STRIP) return 0;
    const unsigned char *base = NULL;
    GLsizei stride = 0;
    if (!lgwebos_arrays_are_pexvertex_interleaved(&base, &stride)) return 0;
    if (type != GL_UNSIGNED_INT && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_BYTE) return 0;

    unsigned int max_idx = 0;
    for (int i = 0; i < count; ++i) {
        unsigned int idx = lgwebos_read_index(indices, type, i);
        if (idx > max_idx) max_idx = idx;
    }
    if (max_idx > 262143u) return 0;
    if (type == GL_UNSIGNED_INT && max_idx > 65535u) return 0;

    LgWebOSMat4 mvp;
    lgwebos_make_mvp(&mvp);
    glUseProgram(g_lgwebos_program);
    if (g_lgwebos_u_mvp >= 0) glUniformMatrix4fv(g_lgwebos_u_mvp, 1, GL_FALSE, mvp.m);
    lgwebos_apply_state_for_batch(NULL);

    glBindBuffer(GL_ARRAY_BUFFER, g_lgwebos_vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(((size_t)max_idx + 1u) * (size_t)stride), base, GL_DYNAMIC_DRAW);
    if (g_lgwebos_a_pos >= 0) {
        glEnableVertexAttribArray((GLuint)g_lgwebos_a_pos);
        glVertexAttribPointer((GLuint)g_lgwebos_a_pos, 3, GL_FLOAT, GL_FALSE, stride, (const GLvoid*)(uintptr_t)offsetof(PexVertex, x));
    }
    if (g_lgwebos_a_uv >= 0) {
        glEnableVertexAttribArray((GLuint)g_lgwebos_a_uv);
        glVertexAttribPointer((GLuint)g_lgwebos_a_uv, 2, GL_FLOAT, GL_FALSE, stride, (const GLvoid*)(uintptr_t)offsetof(PexVertex, u));
    }
    if (g_lgwebos_a_color >= 0) {
        glEnableVertexAttribArray((GLuint)g_lgwebos_a_color);
        glVertexAttribPointer((GLuint)g_lgwebos_a_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (const GLvoid*)(uintptr_t)offsetof(PexVertex, color));
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_lgwebos_ebo);
    GLenum draw_type = type;
    const GLvoid *draw_offset = (const GLvoid*)0;
    if (type == GL_UNSIGNED_INT && max_idx <= 65535u) {
        if (!lgwebos_ensure_index16_tmp(count)) { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); glBindBuffer(GL_ARRAY_BUFFER, 0); return 0; }
        const GLuint *src = (const GLuint*)indices;
        for (int i = 0; i < count; ++i) g_lgwebos_index16_tmp[i] = (GLushort)src[i];
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)((size_t)count * sizeof(GLushort)), g_lgwebos_index16_tmp, GL_DYNAMIC_DRAW);
        draw_type = GL_UNSIGNED_SHORT;
    } else {
        int index_size = lgwebos_sizeof_type(type);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)((size_t)count * (size_t)index_size), indices, GL_DYNAMIC_DRAW);
    }
    glDrawElements(mode, count, draw_type, draw_offset);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return 1;
}

static void pex_lgwebos_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
    if (count <= 0 || !indices) return;
    if (lgwebos_draw_elements_fast(mode, count, type, indices)) return;
    LgWebOSVertex *tmp = (LgWebOSVertex*)malloc((size_t)count * sizeof(*tmp));
    if (!tmp) return;
    for (int i = 0; i < count; ++i) tmp[i] = lgwebos_vertex_from_arrays(lgwebos_read_index(indices, type, i));
    lgwebos_draw_raw(tmp, count, mode, NULL);
    free(tmp);
}

static int pex_lgwebos_gluProject(GLdouble objx, GLdouble objy, GLdouble objz,
                                  const GLdouble model[16], const GLdouble proj[16], const GLint viewport[4],
                                  GLdouble *winx, GLdouble *winy, GLdouble *winz) {
    GLdouble in[4] = {objx, objy, objz, 1.0};
    GLdouble mv[4];
    for (int row=0; row<4; ++row) mv[row] = model[0*4+row]*in[0] + model[1*4+row]*in[1] + model[2*4+row]*in[2] + model[3*4+row]*in[3];
    GLdouble clip[4];
    for (int row=0; row<4; ++row) clip[row] = proj[0*4+row]*mv[0] + proj[1*4+row]*mv[1] + proj[2*4+row]*mv[2] + proj[3*4+row]*mv[3];
    if (clip[3] == 0.0) return 0;
    clip[0] /= clip[3]; clip[1] /= clip[3]; clip[2] /= clip[3];
    if (winx) *winx = viewport[0] + (1.0 + clip[0]) * viewport[2] / 2.0;
    if (winy) *winy = viewport[1] + (1.0 + clip[1]) * viewport[3] / 2.0;
    if (winz) *winz = (1.0 + clip[2]) / 2.0;
    return 1;
}

static void pex_lgwebos_gles2_shutdown(void) {
    for (GLuint i = 1; i < PEX_LGWEBOS_MAX_LISTS; ++i) lgwebos_list_free(i);
    free(g_lgwebos_imm); g_lgwebos_imm = NULL; g_lgwebos_imm_cap = 0;
    free(g_lgwebos_index16_tmp); g_lgwebos_index16_tmp = NULL; g_lgwebos_index16_cap = 0;
    if (g_lgwebos_ebo) { glDeleteBuffers(1, &g_lgwebos_ebo); g_lgwebos_ebo = 0; }
    if (g_lgwebos_vbo) { glDeleteBuffers(1, &g_lgwebos_vbo); g_lgwebos_vbo = 0; }
    if (g_lgwebos_program) { glDeleteProgram(g_lgwebos_program); g_lgwebos_program = 0; }
}
