/* Android TV OpenGL ES 2.0 fixed-function compatibility shim.
   This file is intentionally platform-local.  It emulates just enough of the
   old OpenGL 1.x API used by PexCraft so world/UI code can remain untouched. */

#define PEX_ANDROID_TV_MAX_IMM_VERTS 262144
#define PEX_ANDROID_TV_MAX_LISTS 8192

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

typedef struct AndroidTVVertex {
    GLfloat x, y, z;
    GLfloat u, v;
    GLfloat r, g, b, a;
} AndroidTVVertex;

typedef struct AndroidTVMat4 { GLfloat m[16]; } AndroidTVMat4;

typedef struct AndroidTVBatch {
    AndroidTVVertex *v;
    GLsizei count;
    GLenum mode;
    GLuint tex;
    int texture_enabled;
    int blend_enabled;
    int depth_enabled;
    int alpha_enabled;
    int cull_enabled;
    GLfloat alpha_ref;
    struct AndroidTVBatch *next;
} AndroidTVBatch;

typedef struct AndroidTVList { AndroidTVBatch *first, *last; } AndroidTVList;

typedef struct AndroidTVStaticMeshBatch {
    GLuint vbo, ibo;
    GLsizei index_count;
    struct AndroidTVStaticMeshBatch *next;
} AndroidTVStaticMeshBatch;

typedef struct AndroidTVStaticMesh {
    AndroidTVStaticMeshBatch *first, *last;
    uint32_t vertex_count, index_count;
} AndroidTVStaticMesh;

typedef struct AndroidTVDeferredBatch {
    AndroidTVVertex *v;
    int count, cap;
    GLenum mode;
    GLuint tex;
    int texture_enabled;
    int blend_enabled;
    int depth_enabled;
    int alpha_enabled;
    int cull_enabled;
    GLfloat alpha_ref;
    AndroidTVMat4 projection;
    int valid;
} AndroidTVDeferredBatch;

static GLuint g_android_tv_program = 0;          /* Lightweight UI/entity shader. */
static GLuint g_android_tv_terrain_program = 0;  /* Terrain-only lightmap shader. */
static GLuint g_android_tv_vbo = 0;
static GLuint g_android_tv_ebo = 0;
static GLint g_android_tv_a_pos = -1;
static GLint g_android_tv_a_uv = -1;
static GLint g_android_tv_a_color = -1;
static GLint g_android_tv_u_mvp = -1;
static GLint g_android_tv_u_tex = -1;
static GLint g_android_tv_u_use_tex = -1;
static GLint g_android_tv_u_alpha_enabled = -1;
static GLint g_android_tv_u_alpha_ref = -1;

static GLint g_android_tv_terrain_a_pos = -1;
static GLint g_android_tv_terrain_a_uv = -1;
static GLint g_android_tv_terrain_a_color = -1;
static GLint g_android_tv_terrain_a_light = -1;
static GLint g_android_tv_terrain_u_mvp = -1;
static GLint g_android_tv_terrain_u_tex = -1;
static GLint g_android_tv_terrain_u_alpha_enabled = -1;
static GLint g_android_tv_terrain_u_alpha_ref = -1;
static GLint g_android_tv_terrain_u_lightmap = -1;

static int g_android_tv_texture_enabled = 1;
static int g_android_tv_blend_enabled = 1;
static int g_android_tv_depth_enabled = 0;
static int g_android_tv_alpha_enabled = 0;
static int g_android_tv_cull_enabled = 0;
static GLfloat g_android_tv_alpha_ref = 0.1f;
static GLuint g_android_tv_bound_tex = 0;
static GLuint g_android_tv_lightmap_tex = 0;
static unsigned int g_android_tv_lightmap_version = 0;
static int g_android_tv_static_pass_active = 0;
static AndroidTVDeferredBatch g_android_tv_deferred;

typedef struct AndroidTVWorldTarget {
    GLuint fbo, color_tex, depth_rb;
    GLuint program, vbo;
    GLint a_pos, a_uv, u_tex;
    int width, height;
    int valid;
    int active;
    Uint32 last_update_ms;
} AndroidTVWorldTarget;

typedef struct AndroidTVPanoramaFx {
    GLuint fbo, tex[2];
    GLuint program, vbo;
    GLint a_pos, a_uv, u_tex, u_step;
    int size;
    GLuint output_tex;
} AndroidTVPanoramaFx;

static AndroidTVWorldTarget g_android_tv_world_target;
static AndroidTVPanoramaFx g_android_tv_panorama_fx;

/* Small GL-state cache.  The compatibility layer used to redundantly submit the
   complete pipeline for every chunk section and every glyph. */
static int g_android_tv_applied_valid = 0;
static int g_android_tv_applied_blend = -1;
static int g_android_tv_applied_depth = -1;
static int g_android_tv_applied_cull = -1;
static int g_android_tv_applied_use_tex = -1;
static int g_android_tv_applied_alpha = -1;
static GLuint g_android_tv_applied_tex = (GLuint)~0u;
static GLfloat g_android_tv_applied_alpha_ref = -1.0f;
static AndroidTVMat4 g_android_tv_applied_mvp;
static int g_android_tv_applied_mvp_valid = 0;
static GLfloat g_android_tv_cur_u = 0.0f, g_android_tv_cur_v = 0.0f;
static GLfloat g_android_tv_cur_color[4] = {1,1,1,1};
static GLint g_android_tv_viewport[4] = {0,0,854,480};

static GLenum g_android_tv_begin_mode = 0;
static int g_android_tv_begin_active = 0;
static AndroidTVVertex *g_android_tv_imm = NULL;
static int g_android_tv_imm_count = 0;
static int g_android_tv_imm_cap = 0;

static AndroidTVList g_android_tv_lists[PEX_ANDROID_TV_MAX_LISTS];
static GLuint g_android_tv_next_list = 1;
static int g_android_tv_compiling = 0;
static GLuint g_android_tv_compile_list = 0;

static GLenum g_android_tv_matrix_mode = GL_MODELVIEW;
static AndroidTVMat4 g_android_tv_model_stack[64];
static int g_android_tv_model_top = 0;
static AndroidTVMat4 g_android_tv_proj_stack[32];
static int g_android_tv_proj_top = 0;

static const GLvoid *g_android_tv_vertex_ptr = NULL;
static const GLvoid *g_android_tv_texcoord_ptr = NULL;
static const GLvoid *g_android_tv_color_ptr = NULL;
static GLint g_android_tv_vertex_size = 3;
static GLint g_android_tv_texcoord_size = 2;
static GLint g_android_tv_color_size = 4;
static GLenum g_android_tv_vertex_type = GL_FLOAT;
static GLenum g_android_tv_texcoord_type = GL_FLOAT;
static GLenum g_android_tv_color_type = GL_UNSIGNED_BYTE;
static GLsizei g_android_tv_vertex_stride = 0;
static GLsizei g_android_tv_texcoord_stride = 0;
static GLsizei g_android_tv_color_stride = 0;
static int g_android_tv_vertex_array_enabled = 0;
static int g_android_tv_texcoord_array_enabled = 0;
static int g_android_tv_color_array_enabled = 0;
static GLushort *g_android_tv_index16_tmp = NULL;
static int g_android_tv_index16_cap = 0;

static void android_tv_mat_identity(AndroidTVMat4 *m) {
    memset(m->m, 0, sizeof(m->m));
    m->m[0] = m->m[5] = m->m[10] = m->m[15] = 1.0f;
}

static void android_tv_mat_mul(AndroidTVMat4 *out, const AndroidTVMat4 *a, const AndroidTVMat4 *b) {
    AndroidTVMat4 r;
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

static AndroidTVMat4 *android_tv_current_matrix(void) {
    return (g_android_tv_matrix_mode == GL_PROJECTION) ? &g_android_tv_proj_stack[g_android_tv_proj_top] : &g_android_tv_model_stack[g_android_tv_model_top];
}

static void android_tv_mat_postmul(AndroidTVMat4 *cur, const AndroidTVMat4 *rhs) {
    AndroidTVMat4 out;
    android_tv_mat_mul(&out, cur, rhs);
    *cur = out;
}

static void android_tv_make_mvp(AndroidTVMat4 *out) {
    android_tv_mat_mul(out, &g_android_tv_proj_stack[g_android_tv_proj_top], &g_android_tv_model_stack[g_android_tv_model_top]);
}

static int android_tv_ensure_imm(int needed) {
    if (needed <= g_android_tv_imm_cap) return 1;
    int cap = g_android_tv_imm_cap ? g_android_tv_imm_cap : 4096;
    while (cap < needed) {
        if (cap > PEX_ANDROID_TV_MAX_IMM_VERTS / 2) { cap = PEX_ANDROID_TV_MAX_IMM_VERTS; break; }
        cap *= 2;
    }
    if (needed > cap) return 0;
    AndroidTVVertex *nv = (AndroidTVVertex*)realloc(g_android_tv_imm, (size_t)cap * sizeof(*nv));
    if (!nv) return 0;
    g_android_tv_imm = nv;
    g_android_tv_imm_cap = cap;
    return 1;
}

static GLuint android_tv_compile_shader(GLenum type, const char *src) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]; GLsizei len = 0;
        glGetShaderInfoLog(sh, sizeof(log), &len, log);
        fprintf(stderr, "Android TV GLES shader compile failed: %s\n", log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

static GLuint android_tv_link_program(const char *vs_src, const char *fs_src, const char *label) {
    GLuint v = android_tv_compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint f = android_tv_compile_shader(GL_FRAGMENT_SHADER, fs_src);
    if (!v || !f) {
        if (v) glDeleteShader(v);
        if (f) glDeleteShader(f);
        return 0;
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, v);
    glAttachShader(program, f);
    glLinkProgram(program);
    glDeleteShader(v);
    glDeleteShader(f);
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024]; GLsizei len = 0;
        glGetProgramInfoLog(program, sizeof(log), &len, log);
        fprintf(stderr, "Android TV GLES %s program link failed: %s\n", label ? label : "", log);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

static int gles2_renderer_init(void) {
    /* Keep the menu, text, GUI, entities and compatibility geometry on the
       small one-texture shader.  v9 routed every pixel through the terrain
       lightmap shader; several low-end GLES2 drivers compile that uber-shader
       into a very slow path even while the lightmap branch is disabled. */
    const char *ui_vs =
        "attribute vec3 a_pos;\n"
        "attribute vec2 a_uv;\n"
        "attribute vec4 a_color;\n"
        "uniform mat4 u_mvp;\n"
        "varying vec2 v_uv;\n"
        "varying vec4 v_color;\n"
        "void main(){ gl_Position = u_mvp * vec4(a_pos, 1.0); v_uv = a_uv; v_color = a_color; }\n";
    const char *ui_fs =
        "precision mediump float;\n"
        "uniform sampler2D u_tex;\n"
        "uniform int u_use_tex;\n"
        "uniform int u_alpha_enabled;\n"
        "uniform float u_alpha_ref;\n"
        "varying vec2 v_uv;\n"
        "varying vec4 v_color;\n"
        "void main(){ vec4 c = v_color; if(u_use_tex == 1) c *= texture2D(u_tex, v_uv); if(u_alpha_enabled == 1 && c.a <= u_alpha_ref) discard; gl_FragColor = c; }\n";

    const char *terrain_vs =
        "attribute vec3 a_pos;\n"
        "attribute vec2 a_uv;\n"
        "attribute vec4 a_color;\n"
        "attribute vec4 a_light;\n"
        "uniform mat4 u_mvp;\n"
        "varying vec2 v_uv;\n"
        "varying vec4 v_color;\n"
        "varying vec2 v_light_uv;\n"
        "void main(){\n"
        "  gl_Position = u_mvp * vec4(a_pos, 1.0);\n"
        "  v_uv = a_uv; v_color = a_color;\n"
        "  float blockL = a_light.x * 15.9375;\n"
        "  float skyL = a_light.z * 15.9375;\n"
        "  v_light_uv = (vec2(blockL, skyL) + vec2(0.5)) / 16.0;\n"
        "}\n";
    const char *terrain_fs =
        "precision mediump float;\n"
        "uniform sampler2D u_tex;\n"
        "uniform sampler2D u_lightmap;\n"
        "uniform int u_alpha_enabled;\n"
        "uniform float u_alpha_ref;\n"
        "varying vec2 v_uv;\n"
        "varying vec4 v_color;\n"
        "varying vec2 v_light_uv;\n"
        "void main(){ vec4 c = v_color * texture2D(u_tex, v_uv); c.rgb *= texture2D(u_lightmap, v_light_uv).rgb; if(u_alpha_enabled == 1 && c.a <= u_alpha_ref) discard; gl_FragColor = c; }\n";

    g_android_tv_program = android_tv_link_program(ui_vs, ui_fs, "UI");
    g_android_tv_terrain_program = android_tv_link_program(terrain_vs, terrain_fs, "terrain");
    if (!g_android_tv_program || !g_android_tv_terrain_program) return 0;

    g_android_tv_a_pos = glGetAttribLocation(g_android_tv_program, "a_pos");
    g_android_tv_a_uv = glGetAttribLocation(g_android_tv_program, "a_uv");
    g_android_tv_a_color = glGetAttribLocation(g_android_tv_program, "a_color");
    g_android_tv_u_mvp = glGetUniformLocation(g_android_tv_program, "u_mvp");
    g_android_tv_u_tex = glGetUniformLocation(g_android_tv_program, "u_tex");
    g_android_tv_u_use_tex = glGetUniformLocation(g_android_tv_program, "u_use_tex");
    g_android_tv_u_alpha_enabled = glGetUniformLocation(g_android_tv_program, "u_alpha_enabled");
    g_android_tv_u_alpha_ref = glGetUniformLocation(g_android_tv_program, "u_alpha_ref");

    g_android_tv_terrain_a_pos = glGetAttribLocation(g_android_tv_terrain_program, "a_pos");
    g_android_tv_terrain_a_uv = glGetAttribLocation(g_android_tv_terrain_program, "a_uv");
    g_android_tv_terrain_a_color = glGetAttribLocation(g_android_tv_terrain_program, "a_color");
    g_android_tv_terrain_a_light = glGetAttribLocation(g_android_tv_terrain_program, "a_light");
    g_android_tv_terrain_u_mvp = glGetUniformLocation(g_android_tv_terrain_program, "u_mvp");
    g_android_tv_terrain_u_tex = glGetUniformLocation(g_android_tv_terrain_program, "u_tex");
    g_android_tv_terrain_u_alpha_enabled = glGetUniformLocation(g_android_tv_terrain_program, "u_alpha_enabled");
    g_android_tv_terrain_u_alpha_ref = glGetUniformLocation(g_android_tv_terrain_program, "u_alpha_ref");
    g_android_tv_terrain_u_lightmap = glGetUniformLocation(g_android_tv_terrain_program, "u_lightmap");

    glGenBuffers(1, &g_android_tv_vbo);
    glGenBuffers(1, &g_android_tv_ebo);
    glGenTextures(1, &g_android_tv_lightmap_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_android_tv_lightmap_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    unsigned char initial_lightmap[16 * 16 * 4];
    memset(initial_lightmap, 255, sizeof(initial_lightmap));
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, initial_lightmap);
    glActiveTexture(GL_TEXTURE0);

    android_tv_mat_identity(&g_android_tv_model_stack[0]);
    android_tv_mat_identity(&g_android_tv_proj_stack[0]);

    glUseProgram(g_android_tv_program);
    if (g_android_tv_u_tex >= 0) glUniform1i(g_android_tv_u_tex, 0);
    glUseProgram(g_android_tv_terrain_program);
    if (g_android_tv_terrain_u_tex >= 0) glUniform1i(g_android_tv_terrain_u_tex, 0);
    if (g_android_tv_terrain_u_lightmap >= 0) glUniform1i(g_android_tv_terrain_u_lightmap, 1);
    glUseProgram(g_android_tv_program);
    return 1;
}

static void android_tv_apply_mvp(const AndroidTVMat4 *mvp) {
    if (!mvp) return;
    if (!g_android_tv_applied_mvp_valid || memcmp(g_android_tv_applied_mvp.m, mvp->m, sizeof(mvp->m)) != 0) {
        if (g_android_tv_u_mvp >= 0) glUniformMatrix4fv(g_android_tv_u_mvp, 1, GL_FALSE, mvp->m);
        g_android_tv_applied_mvp = *mvp;
        g_android_tv_applied_mvp_valid = 1;
    }
}

static void android_tv_apply_state_values(int tex_enabled, int blend_enabled, int depth_enabled,
                                          int alpha_enabled, int cull_enabled, GLuint tex,
                                          GLfloat alpha_ref) {
    if (!g_android_tv_applied_valid || g_android_tv_applied_blend != blend_enabled) {
        if (blend_enabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        g_android_tv_applied_blend = blend_enabled;
    }
    if (!g_android_tv_applied_valid || g_android_tv_applied_depth != depth_enabled) {
        if (depth_enabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        g_android_tv_applied_depth = depth_enabled;
    }
    if (!g_android_tv_applied_valid || g_android_tv_applied_cull != cull_enabled) {
        if (cull_enabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        g_android_tv_applied_cull = cull_enabled;
    }
    GLuint wanted_tex = (tex_enabled && tex) ? tex : 0;
    if (!g_android_tv_applied_valid || g_android_tv_applied_tex != wanted_tex) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, wanted_tex);
        g_android_tv_applied_tex = wanted_tex;
    }
    int use_tex = wanted_tex ? 1 : 0;
    if (!g_android_tv_applied_valid || g_android_tv_applied_use_tex != use_tex) {
        if (g_android_tv_u_use_tex >= 0) glUniform1i(g_android_tv_u_use_tex, use_tex);
        g_android_tv_applied_use_tex = use_tex;
    }
    if (!g_android_tv_applied_valid || g_android_tv_applied_alpha != alpha_enabled) {
        if (g_android_tv_u_alpha_enabled >= 0) glUniform1i(g_android_tv_u_alpha_enabled, alpha_enabled ? 1 : 0);
        g_android_tv_applied_alpha = alpha_enabled;
    }
    if (!g_android_tv_applied_valid || fabsf(g_android_tv_applied_alpha_ref - alpha_ref) > 0.0001f) {
        if (g_android_tv_u_alpha_ref >= 0) glUniform1f(g_android_tv_u_alpha_ref, alpha_ref);
        g_android_tv_applied_alpha_ref = alpha_ref;
    }
    g_android_tv_applied_valid = 1;
}

static void android_tv_apply_state_for_batch(const AndroidTVBatch *b) {
    int tex_enabled = b ? b->texture_enabled : g_android_tv_texture_enabled;
    int blend_enabled = b ? b->blend_enabled : g_android_tv_blend_enabled;
    int depth_enabled = b ? b->depth_enabled : g_android_tv_depth_enabled;
    int alpha_enabled = b ? b->alpha_enabled : g_android_tv_alpha_enabled;
    int cull_enabled = b ? b->cull_enabled : g_android_tv_cull_enabled;
    GLuint tex = b ? b->tex : g_android_tv_bound_tex;
    GLfloat alpha_ref = b ? b->alpha_ref : g_android_tv_alpha_ref;
    android_tv_apply_state_values(tex_enabled, blend_enabled, depth_enabled, alpha_enabled,
                                  cull_enabled, tex, alpha_ref);
}

static void android_tv_draw_raw_mvp(const AndroidTVVertex *v, GLsizei count, GLenum mode,
                                    const AndroidTVBatch *state, const AndroidTVMat4 *mvp) {
    if (!v || count <= 0 || !g_android_tv_program) return;
    glUseProgram(g_android_tv_program);
    android_tv_apply_mvp(mvp);
    android_tv_apply_state_for_batch(state);

    /* v9 orphaned a fixed 2 MiB buffer at frame start and could orphan it
       again after indexed draws.  Some low-end Android GLES drivers serialize
       those allocations, making even the title screen crawl.  One exact
       streaming upload per compatible batch is both smaller and predictable. */
    glBindBuffer(GL_ARRAY_BUFFER, g_android_tv_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)((size_t)count * sizeof(AndroidTVVertex)),
                 v, GL_STREAM_DRAW);
    if (g_android_tv_a_pos >= 0) {
        glEnableVertexAttribArray((GLuint)g_android_tv_a_pos);
        glVertexAttribPointer((GLuint)g_android_tv_a_pos, 3, GL_FLOAT, GL_FALSE,
                              sizeof(AndroidTVVertex),
                              (const GLvoid*)(uintptr_t)offsetof(AndroidTVVertex, x));
    }
    if (g_android_tv_a_uv >= 0) {
        glEnableVertexAttribArray((GLuint)g_android_tv_a_uv);
        glVertexAttribPointer((GLuint)g_android_tv_a_uv, 2, GL_FLOAT, GL_FALSE,
                              sizeof(AndroidTVVertex),
                              (const GLvoid*)(uintptr_t)offsetof(AndroidTVVertex, u));
    }
    if (g_android_tv_a_color >= 0) {
        glEnableVertexAttribArray((GLuint)g_android_tv_a_color);
        glVertexAttribPointer((GLuint)g_android_tv_a_color, 4, GL_FLOAT, GL_FALSE,
                              sizeof(AndroidTVVertex),
                              (const GLvoid*)(uintptr_t)offsetof(AndroidTVVertex, r));
    }
    glDrawArrays(mode, 0, count);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void android_tv_draw_raw(const AndroidTVVertex *v, GLsizei count, GLenum mode, const AndroidTVBatch *state) {
    AndroidTVMat4 mvp;
    android_tv_make_mvp(&mvp);
    android_tv_draw_raw_mvp(v, count, mode, state, &mvp);
}

static int android_tv_deferred_ensure(int needed) {
    if (needed <= g_android_tv_deferred.cap) return 1;
    int cap = g_android_tv_deferred.cap ? g_android_tv_deferred.cap : 8192;
    while (cap < needed) {
        if (cap > PEX_ANDROID_TV_MAX_IMM_VERTS / 2) { cap = PEX_ANDROID_TV_MAX_IMM_VERTS; break; }
        cap *= 2;
    }
    if (needed > cap) return 0;
    AndroidTVVertex *nv = (AndroidTVVertex*)realloc(g_android_tv_deferred.v, (size_t)cap * sizeof(*nv));
    if (!nv) return 0;
    g_android_tv_deferred.v = nv;
    g_android_tv_deferred.cap = cap;
    return 1;
}

static int android_tv_deferred_state_matches(GLenum mode) {
    AndroidTVDeferredBatch *d = &g_android_tv_deferred;
    if (!d->valid || d->mode != mode || d->tex != g_android_tv_bound_tex ||
        d->texture_enabled != g_android_tv_texture_enabled ||
        d->blend_enabled != g_android_tv_blend_enabled ||
        d->depth_enabled != g_android_tv_depth_enabled ||
        d->alpha_enabled != g_android_tv_alpha_enabled ||
        d->cull_enabled != g_android_tv_cull_enabled ||
        fabsf(d->alpha_ref - g_android_tv_alpha_ref) > 0.0001f) return 0;
    return memcmp(d->projection.m, g_android_tv_proj_stack[g_android_tv_proj_top].m,
                  sizeof(d->projection.m)) == 0;
}

static void pex_android_tv_flush(void) {
    AndroidTVDeferredBatch *d = &g_android_tv_deferred;
    if (!d->valid || d->count <= 0) { d->count = 0; d->valid = 0; return; }
    AndroidTVBatch state;
    memset(&state, 0, sizeof(state));
    state.tex = d->tex;
    state.texture_enabled = d->texture_enabled;
    state.blend_enabled = d->blend_enabled;
    state.depth_enabled = d->depth_enabled;
    state.alpha_enabled = d->alpha_enabled;
    state.cull_enabled = d->cull_enabled;
    state.alpha_ref = d->alpha_ref;
    android_tv_draw_raw_mvp(d->v, (GLsizei)d->count, d->mode, &state, &d->projection);
    d->count = 0;
    d->valid = 0;
}


/* Low-resolution 3D target and title-panorama filter. These are deliberately
   outside the compatibility batching path: the world is rendered into an FBO,
   stretched once with linear sampling, and the GUI is then drawn at native
   resolution. The stored option is a percentage, so changing displays keeps
   the same quality/performance ratio instead of restoring stale pixel sizes. */
static void android_tv_fx_restore_compat(int viewport_w, int viewport_h) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(g_android_tv_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_android_tv_bound_tex);
    g_android_tv_viewport[0] = 0;
    g_android_tv_viewport[1] = 0;
    g_android_tv_viewport[2] = viewport_w;
    g_android_tv_viewport[3] = viewport_h;
    glViewport(0, 0, viewport_w, viewport_h);
    if (g_android_tv_blend_enabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (g_android_tv_depth_enabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (g_android_tv_cull_enabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    g_android_tv_applied_valid = 0;
    g_android_tv_applied_mvp_valid = 0;
    g_android_tv_applied_tex = (GLuint)~0u;
}

static void android_tv_world_target_destroy(void) {
    AndroidTVWorldTarget *fx = &g_android_tv_world_target;
    if (fx->vbo) glDeleteBuffers(1, &fx->vbo);
    if (fx->program) glDeleteProgram(fx->program);
    if (fx->depth_rb) glDeleteRenderbuffers(1, &fx->depth_rb);
    if (fx->color_tex) glDeleteTextures(1, &fx->color_tex);
    if (fx->fbo) glDeleteFramebuffers(1, &fx->fbo);
    memset(fx, 0, sizeof(*fx));
}

static int android_tv_world_target_ensure(int width, int height) {
    AndroidTVWorldTarget *fx = &g_android_tv_world_target;
    if (width < 16) width = 16;
    if (height < 16) height = 16;
    if (fx->fbo && fx->color_tex && fx->depth_rb && fx->program &&
        fx->width == width && fx->height == height) return 1;

    android_tv_world_target_destroy();
    const char *vs =
        "attribute vec2 a_pos;\n"
        "attribute vec2 a_uv;\n"
        "varying vec2 v_uv;\n"
        "void main(){ gl_Position=vec4(a_pos,0.0,1.0); v_uv=a_uv; }\n";
    const char *fs =
        "precision mediump float;\n"
        "uniform sampler2D u_tex;\n"
        "varying vec2 v_uv;\n"
        "void main(){ gl_FragColor=texture2D(u_tex,v_uv); }\n";
    fx->program = android_tv_link_program(vs, fs, "scaled world");
    if (!fx->program) return 0;
    fx->a_pos = glGetAttribLocation(fx->program, "a_pos");
    fx->a_uv = glGetAttribLocation(fx->program, "a_uv");
    fx->u_tex = glGetUniformLocation(fx->program, "u_tex");
    glGenBuffers(1, &fx->vbo);
    glGenFramebuffers(1, &fx->fbo);
    glGenTextures(1, &fx->color_tex);
    glGenRenderbuffers(1, &fx->depth_rb);
    if (!fx->vbo || !fx->fbo || !fx->color_tex || !fx->depth_rb) {
        android_tv_world_target_destroy();
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, fx->color_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glBindRenderbuffer(GL_RENDERBUFFER, fx->depth_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, fx->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fx->color_tex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fx->depth_rb);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, g_android_tv_bound_tex);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Android GLES scaled-world framebuffer incomplete: 0x%x\n", (unsigned)status);
        android_tv_world_target_destroy();
        return 0;
    }
    fx->width = width;
    fx->height = height;
    fx->valid = 0;
    return 1;
}

static void android_tv_world_target_present(int full_w, int full_h) {
    AndroidTVWorldTarget *fx = &g_android_tv_world_target;
    static const GLfloat quad[16] = {
        -1.0f,-1.0f, 0.0f,0.0f,
         1.0f,-1.0f, 1.0f,0.0f,
        -1.0f, 1.0f, 0.0f,1.0f,
         1.0f, 1.0f, 1.0f,1.0f
    };
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, full_w, full_h);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glUseProgram(fx->program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fx->color_tex);
    if (fx->u_tex >= 0) glUniform1i(fx->u_tex, 0);
    glBindBuffer(GL_ARRAY_BUFFER, fx->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(quad), quad, GL_DYNAMIC_DRAW);
    if (fx->a_pos >= 0) {
        glEnableVertexAttribArray((GLuint)fx->a_pos);
        glVertexAttribPointer((GLuint)fx->a_pos, 2, GL_FLOAT, GL_FALSE, 4 * (GLsizei)sizeof(GLfloat), (const GLvoid*)0);
    }
    if (fx->a_uv >= 0) {
        glEnableVertexAttribArray((GLuint)fx->a_uv);
        glVertexAttribPointer((GLuint)fx->a_uv, 2, GL_FLOAT, GL_FALSE, 4 * (GLsizei)sizeof(GLfloat), (const GLvoid*)(2 * sizeof(GLfloat)));
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    android_tv_fx_restore_compat(full_w, full_h);
}

/* Returns 0 for native direct rendering, 1 when the caller should render into
   the scaled target, or 2 when a cached container-screen world was presented. */
static int pex_android_tv_world_target_width(void) {
    return g_android_tv_world_target.active ? g_android_tv_world_target.width : 0;
}

static int pex_android_tv_world_target_height(void) {
    return g_android_tv_world_target.active ? g_android_tv_world_target.height : 0;
}

static int pex_android_tv_world_target_begin(int percent, int full_w, int full_h, int allow_cache) {
    if (percent < 10) percent = 10;
    if (percent > 100) percent = 100;
    if (percent >= 100 && !allow_cache) return 0;
    int width = (full_w * percent + 50) / 100;
    int height = (full_h * percent + 50) / 100;
    if (width < 16) width = 16;
    if (height < 16) height = 16;

    pex_android_tv_flush();
    if (!android_tv_world_target_ensure(width, height)) return 0;
    AndroidTVWorldTarget *fx = &g_android_tv_world_target;
    Uint32 now = SDL_GetTicks();
    if (allow_cache && fx->valid && (Uint32)(now - fx->last_update_ms) < 50u) {
        android_tv_world_target_present(full_w, full_h);
        return 2;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fx->fbo);
    fx->active = 1;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fx->color_tex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fx->depth_rb);
    glViewport(0, 0, fx->width, fx->height);
    g_android_tv_viewport[0] = 0;
    g_android_tv_viewport[1] = 0;
    g_android_tv_viewport[2] = fx->width;
    g_android_tv_viewport[3] = fx->height;
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    g_android_tv_applied_valid = 0;
    g_android_tv_applied_mvp_valid = 0;
    return 1;
}

static void pex_android_tv_world_target_end(int full_w, int full_h) {
    pex_android_tv_flush();
    g_android_tv_world_target.active = 0;
    g_android_tv_world_target.valid = 1;
    g_android_tv_world_target.last_update_ms = SDL_GetTicks();
    android_tv_world_target_present(full_w, full_h);
}

static void android_tv_panorama_fx_destroy(void) {
    AndroidTVPanoramaFx *fx = &g_android_tv_panorama_fx;
    if (fx->vbo) glDeleteBuffers(1, &fx->vbo);
    if (fx->program) glDeleteProgram(fx->program);
    if (fx->tex[0] || fx->tex[1]) glDeleteTextures(2, fx->tex);
    if (fx->fbo) glDeleteFramebuffers(1, &fx->fbo);
    memset(fx, 0, sizeof(*fx));
}

static int android_tv_panorama_fx_ensure(int size) {
    AndroidTVPanoramaFx *fx = &g_android_tv_panorama_fx;
    if (size < 64) size = 64;
    if (size > 512) size = 512;
    if (fx->program && fx->fbo && fx->tex[0] && fx->tex[1] && fx->size == size) return 1;
    android_tv_panorama_fx_destroy();
    const char *vs =
        "attribute vec2 a_pos;\n"
        "attribute vec2 a_uv;\n"
        "varying vec2 v_uv;\n"
        "void main(){ gl_Position=vec4(a_pos,0.0,1.0); v_uv=a_uv; }\n";
    const char *fs =
        "precision mediump float;\n"
        "uniform sampler2D u_tex;\n"
        "uniform vec2 u_step;\n"
        "varying vec2 v_uv;\n"
        "void main(){\n"
        " vec4 c=texture2D(u_tex,v_uv)*0.2270270270;\n"
        " c+=texture2D(u_tex,v_uv+u_step*1.3846153846)*0.3162162162;\n"
        " c+=texture2D(u_tex,v_uv-u_step*1.3846153846)*0.3162162162;\n"
        " c+=texture2D(u_tex,v_uv+u_step*3.2307692308)*0.0702702703;\n"
        " c+=texture2D(u_tex,v_uv-u_step*3.2307692308)*0.0702702703;\n"
        " gl_FragColor=c;\n"
        "}\n";
    fx->program = android_tv_link_program(vs, fs, "panorama blur");
    if (!fx->program) return 0;
    fx->a_pos = glGetAttribLocation(fx->program, "a_pos");
    fx->a_uv = glGetAttribLocation(fx->program, "a_uv");
    fx->u_tex = glGetUniformLocation(fx->program, "u_tex");
    fx->u_step = glGetUniformLocation(fx->program, "u_step");
    glGenBuffers(1, &fx->vbo);
    glGenFramebuffers(1, &fx->fbo);
    glGenTextures(2, fx->tex);
    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, fx->tex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    }
    fx->size = size;
    fx->output_tex = fx->tex[0];
    glBindFramebuffer(GL_FRAMEBUFFER, fx->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fx->tex[0], 0);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, g_android_tv_bound_tex);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        android_tv_panorama_fx_destroy();
        return 0;
    }
    return 1;
}

static void android_tv_panorama_fx_draw(GLuint tex, GLfloat step_x, GLfloat step_y, const GLfloat vertices[16]) {
    AndroidTVPanoramaFx *fx = &g_android_tv_panorama_fx;
    glUseProgram(fx->program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    if (fx->u_tex >= 0) glUniform1i(fx->u_tex, 0);
    if (fx->u_step >= 0) glUniform2f(fx->u_step, step_x, step_y);
    glBindBuffer(GL_ARRAY_BUFFER, fx->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(16 * sizeof(GLfloat)), vertices, GL_DYNAMIC_DRAW);
    if (fx->a_pos >= 0) {
        glEnableVertexAttribArray((GLuint)fx->a_pos);
        glVertexAttribPointer((GLuint)fx->a_pos, 2, GL_FLOAT, GL_FALSE, 4 * (GLsizei)sizeof(GLfloat), (const GLvoid*)0);
    }
    if (fx->a_uv >= 0) {
        glEnableVertexAttribArray((GLuint)fx->a_uv);
        glVertexAttribPointer((GLuint)fx->a_uv, 2, GL_FLOAT, GL_FALSE, 4 * (GLsizei)sizeof(GLfloat), (const GLvoid*)(2 * sizeof(GLfloat)));
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static int pex_android_tv_panorama_begin(int size) {
    pex_android_tv_flush();
    if (!android_tv_panorama_fx_ensure(size)) return 0;
    AndroidTVPanoramaFx *fx = &g_android_tv_panorama_fx;
    glBindFramebuffer(GL_FRAMEBUFFER, fx->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fx->tex[0], 0);
    glViewport(0, 0, fx->size, fx->size);
    g_android_tv_viewport[0]=0; g_android_tv_viewport[1]=0;
    g_android_tv_viewport[2]=fx->size; g_android_tv_viewport[3]=fx->size;
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_FALSE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    fx->output_tex = fx->tex[0];
    return 1;
}

static int pex_android_tv_panorama_blur(int iterations) {
    pex_android_tv_flush();
    AndroidTVPanoramaFx *fx = &g_android_tv_panorama_fx;
    if (!fx->fbo || !fx->program || !fx->tex[0] || !fx->tex[1]) return 0;
    if (iterations < 1) iterations = 1;
    if (iterations > 4) iterations = 4;
    static const GLfloat quad[16] = {
        -1.0f,-1.0f,0.0f,0.0f, 1.0f,-1.0f,1.0f,0.0f,
        -1.0f, 1.0f,0.0f,1.0f, 1.0f, 1.0f,1.0f,1.0f
    };
    GLuint src = fx->tex[0], dst = fx->tex[1];
    GLfloat inv = 1.0f / (GLfloat)fx->size;
    glBindFramebuffer(GL_FRAMEBUFFER, fx->fbo);
    glViewport(0,0,fx->size,fx->size);
    glDisable(GL_BLEND); glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE); glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    for (int i=0;i<iterations;++i) {
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,dst,0);
        android_tv_panorama_fx_draw(src,inv,0.0f,quad);
        GLuint swap=src; src=dst; dst=swap;
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,dst,0);
        android_tv_panorama_fx_draw(src,0.0f,inv,quad);
        swap=src; src=dst; dst=swap;
    }
    fx->output_tex=src;
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    return 1;
}

static int pex_android_tv_panorama_present(int render_w, int render_h,
                                           GLfloat u0, GLfloat v0, GLfloat u1, GLfloat v1) {
    pex_android_tv_flush();
    AndroidTVPanoramaFx *fx = &g_android_tv_panorama_fx;
    if (!fx->output_tex || !fx->program) return 0;
    const GLfloat quad[16] = {
        -1.0f,-1.0f,u0,v0, 1.0f,-1.0f,u1,v0,
        -1.0f, 1.0f,u0,v1, 1.0f, 1.0f,u1,v1
    };
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glViewport(0,0,render_w,render_h);
    glDisable(GL_SCISSOR_TEST); glDisable(GL_BLEND); glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE); glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    android_tv_panorama_fx_draw(fx->output_tex,0.0f,0.0f,quad);
    android_tv_fx_restore_compat(render_w,render_h);
    return 1;
}

static void android_tv_deferred_begin(GLenum mode) {
    AndroidTVDeferredBatch *d = &g_android_tv_deferred;
    if (!android_tv_deferred_state_matches(mode)) {
        pex_android_tv_flush();
        d->valid = 1;
        d->mode = mode;
        d->tex = g_android_tv_bound_tex;
        d->texture_enabled = g_android_tv_texture_enabled;
        d->blend_enabled = g_android_tv_blend_enabled;
        d->depth_enabled = g_android_tv_depth_enabled;
        d->alpha_enabled = g_android_tv_alpha_enabled;
        d->cull_enabled = g_android_tv_cull_enabled;
        d->alpha_ref = g_android_tv_alpha_ref;
        d->projection = g_android_tv_proj_stack[g_android_tv_proj_top];
    }
}

static int android_tv_deferred_append(const AndroidTVVertex *src, int count, GLenum mode) {
    if (!src || count <= 0) return 1;
    /* Independent triangle/line lists can be concatenated safely. Strips, fans,
       and loops cannot: joining two calls would create connector primitives and
       change the image. Keep those rare modes immediate while still using the
       retained ring buffer. */
    if (mode != GL_QUADS && mode != GL_TRIANGLES &&
        !(mode == GL_LINES && (count & 1) == 0)) {
        AndroidTVMat4 projection = g_android_tv_proj_stack[g_android_tv_proj_top];
        pex_android_tv_flush();
        android_tv_draw_raw_mvp(src, (GLsizei)count, mode, NULL, &projection);
        return 1;
    }
    GLenum out_mode = mode;
    int out_count = count;
    if (mode == GL_QUADS) { out_mode = GL_TRIANGLES; out_count = (count / 4) * 6; }
    if (out_count <= 0) return 1;
    android_tv_deferred_begin(out_mode);
    AndroidTVDeferredBatch *d = &g_android_tv_deferred;
    if (!android_tv_deferred_ensure(d->count + out_count)) {
        pex_android_tv_flush();
        android_tv_deferred_begin(out_mode);
        if (!android_tv_deferred_ensure(out_count)) return 0;
    }
    if (mode == GL_QUADS) {
        int n = d->count;
        for (int i = 0; i < count / 4; ++i) {
            const AndroidTVVertex *q = src + i * 4;
            d->v[n++] = q[0]; d->v[n++] = q[1]; d->v[n++] = q[2];
            d->v[n++] = q[0]; d->v[n++] = q[2]; d->v[n++] = q[3];
        }
        d->count = n;
    } else {
        memcpy(d->v + d->count, src, (size_t)out_count * sizeof(*src));
        d->count += out_count;
    }
    return 1;
}

static AndroidTVBatch *android_tv_batch_create(const AndroidTVVertex *src, int count, GLenum mode) {
    if (!src || count <= 0) return NULL;
    int out_count = count;
    GLenum out_mode = mode;
    if (mode == GL_QUADS) {
        int q = count / 4;
        if (q <= 0) return NULL;
        out_count = q * 6;
        out_mode = GL_TRIANGLES;
    }
    AndroidTVBatch *b = (AndroidTVBatch*)calloc(1, sizeof(*b));
    if (!b) return NULL;
    b->v = (AndroidTVVertex*)malloc((size_t)out_count * sizeof(*b->v));
    if (!b->v) { free(b); return NULL; }
    b->count = (GLsizei)out_count;
    b->mode = out_mode;
    b->tex = g_android_tv_bound_tex;
    b->texture_enabled = g_android_tv_texture_enabled;
    b->blend_enabled = g_android_tv_blend_enabled;
    b->depth_enabled = g_android_tv_depth_enabled;
    b->alpha_enabled = g_android_tv_alpha_enabled;
    b->cull_enabled = g_android_tv_cull_enabled;
    b->alpha_ref = g_android_tv_alpha_ref;
    if (mode == GL_QUADS) {
        int n = 0;
        int q = count / 4;
        for (int i = 0; i < q; ++i) {
            const AndroidTVVertex *v = src + i * 4;
            b->v[n++] = v[0]; b->v[n++] = v[1]; b->v[n++] = v[2];
            b->v[n++] = v[0]; b->v[n++] = v[2]; b->v[n++] = v[3];
        }
    } else {
        memcpy(b->v, src, (size_t)out_count * sizeof(*b->v));
    }
    return b;
}

static void android_tv_batch_free_chain(AndroidTVBatch *b) {
    while (b) {
        AndroidTVBatch *n = b->next;
        free(b->v);
        free(b);
        b = n;
    }
}

static void android_tv_list_free(GLuint list) {
    if (list == 0 || list >= PEX_ANDROID_TV_MAX_LISTS) return;
    android_tv_batch_free_chain(g_android_tv_lists[list].first);
    memset(&g_android_tv_lists[list], 0, sizeof(g_android_tv_lists[list]));
}

static void android_tv_list_append(GLuint list, AndroidTVBatch *b) {
    if (!b || list == 0 || list >= PEX_ANDROID_TV_MAX_LISTS) { if (b) { free(b->v); free(b); } return; }
    AndroidTVList *l = &g_android_tv_lists[list];
    if (!l->first) l->first = b;
    else l->last->next = b;
    l->last = b;
}

static void android_tv_draw_or_store(void) {
    if (g_android_tv_compiling) {
        AndroidTVBatch *b = android_tv_batch_create(g_android_tv_imm, g_android_tv_imm_count, g_android_tv_begin_mode);
        if (b) android_tv_list_append(g_android_tv_compile_list, b);
        return;
    }
    android_tv_deferred_append(g_android_tv_imm, g_android_tv_imm_count, g_android_tv_begin_mode);
}

static int android_tv_sizeof_type(GLenum type) {
    switch (type) {
        case GL_FLOAT: return 4;
        case GL_UNSIGNED_BYTE: return 1;
        case GL_UNSIGNED_SHORT: return 2;
        case GL_UNSIGNED_INT: return 4;
        default: return 4;
    }
}

static const unsigned char *android_tv_elem_ptr(const GLvoid *base, int index, GLsizei stride, int comps, GLenum type) {
    int sz = android_tv_sizeof_type(type);
    if (stride <= 0) stride = (GLsizei)(comps * sz);
    return ((const unsigned char*)base) + (size_t)index * (size_t)stride;
}

static float android_tv_read_component(const unsigned char *p, GLenum type, int normalized) {
    if (type == GL_FLOAT) return *(const GLfloat*)p;
    if (type == GL_UNSIGNED_BYTE) return normalized ? ((float)(*(const GLubyte*)p) / 255.0f) : (float)(*(const GLubyte*)p);
    if (type == GL_UNSIGNED_SHORT) return normalized ? ((float)(*(const GLushort*)p) / 65535.0f) : (float)(*(const GLushort*)p);
    return 0.0f;
}

static unsigned int android_tv_read_index(const GLvoid *indices, GLenum type, int i) {
    if (type == GL_UNSIGNED_SHORT) return (unsigned int)((const GLushort*)indices)[i];
    if (type == GL_UNSIGNED_INT) return (unsigned int)((const GLuint*)indices)[i];
    return (unsigned int)((const GLubyte*)indices)[i];
}

static AndroidTVVertex android_tv_vertex_from_arrays(unsigned int idx) {
    AndroidTVVertex v;
    memset(&v, 0, sizeof(v));
    v.r = g_android_tv_cur_color[0]; v.g = g_android_tv_cur_color[1]; v.b = g_android_tv_cur_color[2]; v.a = g_android_tv_cur_color[3];
    v.u = g_android_tv_cur_u; v.v = g_android_tv_cur_v;
    if (g_android_tv_vertex_array_enabled && g_android_tv_vertex_ptr) {
        const unsigned char *p = android_tv_elem_ptr(g_android_tv_vertex_ptr, (int)idx, g_android_tv_vertex_stride, g_android_tv_vertex_size, g_android_tv_vertex_type);
        v.x = android_tv_read_component(p + 0 * android_tv_sizeof_type(g_android_tv_vertex_type), g_android_tv_vertex_type, 0);
        if (g_android_tv_vertex_size > 1) v.y = android_tv_read_component(p + 1 * android_tv_sizeof_type(g_android_tv_vertex_type), g_android_tv_vertex_type, 0);
        if (g_android_tv_vertex_size > 2) v.z = android_tv_read_component(p + 2 * android_tv_sizeof_type(g_android_tv_vertex_type), g_android_tv_vertex_type, 0);
    }
    if (g_android_tv_texcoord_array_enabled && g_android_tv_texcoord_ptr) {
        const unsigned char *p = android_tv_elem_ptr(g_android_tv_texcoord_ptr, (int)idx, g_android_tv_texcoord_stride, g_android_tv_texcoord_size, g_android_tv_texcoord_type);
        v.u = android_tv_read_component(p + 0 * android_tv_sizeof_type(g_android_tv_texcoord_type), g_android_tv_texcoord_type, 0);
        if (g_android_tv_texcoord_size > 1) v.v = android_tv_read_component(p + 1 * android_tv_sizeof_type(g_android_tv_texcoord_type), g_android_tv_texcoord_type, 0);
    }
    if (g_android_tv_color_array_enabled && g_android_tv_color_ptr) {
        const unsigned char *p = android_tv_elem_ptr(g_android_tv_color_ptr, (int)idx, g_android_tv_color_stride, g_android_tv_color_size, g_android_tv_color_type);
        int sz = android_tv_sizeof_type(g_android_tv_color_type);
        v.r = android_tv_read_component(p + 0 * sz, g_android_tv_color_type, 1);
        if (g_android_tv_color_size > 1) v.g = android_tv_read_component(p + 1 * sz, g_android_tv_color_type, 1);
        if (g_android_tv_color_size > 2) v.b = android_tv_read_component(p + 2 * sz, g_android_tv_color_type, 1);
        if (g_android_tv_color_size > 3) v.a = android_tv_read_component(p + 3 * sz, g_android_tv_color_type, 1);
    }
    return v;
}

static void pex_android_tv_begin_frame(void) {
    pex_android_tv_flush();
    /* External SDL/driver work may change bindings between frames.  Reset only
       the tiny state cache; do not allocate or orphan GPU memory here. */
    g_android_tv_applied_valid = 0;
    g_android_tv_applied_mvp_valid = 0;
}

static void pex_android_tv_glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { pex_android_tv_flush(); glClearColor(r,g,b,a); }
static void pex_android_tv_glClear(GLbitfield mask) { pex_android_tv_flush(); glClear(mask); }
static void pex_android_tv_glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) { pex_android_tv_flush(); glColorMask(r,g,b,a); }
static void pex_android_tv_glReadBuffer(GLenum mode) { (void)mode; }
static void pex_android_tv_glReadPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum format, GLenum type, GLvoid *pixels) { pex_android_tv_flush(); glReadPixels(x,y,w,h,format,type,pixels); }
static void pex_android_tv_glLineWidth(GLfloat w) { pex_android_tv_flush(); glLineWidth(w); }
static void pex_android_tv_glBlendFunc(GLenum s, GLenum d) { pex_android_tv_flush(); glBlendFunc(s,d); }
static void pex_android_tv_glDepthFunc(GLenum f) { pex_android_tv_flush(); glDepthFunc(f); }
static void pex_android_tv_glDepthMask(GLboolean flag) { pex_android_tv_flush(); glDepthMask(flag); }
static void pex_android_tv_glAlphaFunc(GLenum func, GLfloat ref) { (void)func; if (fabsf(g_android_tv_alpha_ref - ref) > 0.0001f) pex_android_tv_flush(); g_android_tv_alpha_ref = ref; }
static void pex_android_tv_glViewport(GLint x, GLint y, GLsizei w, GLsizei h) { pex_android_tv_flush(); g_android_tv_viewport[0]=x; g_android_tv_viewport[1]=y; g_android_tv_viewport[2]=w; g_android_tv_viewport[3]=h; glViewport(x,y,w,h); }

static void pex_android_tv_glEnable(GLenum cap) {
    if (cap == GL_TEXTURE_2D) { if (!g_android_tv_texture_enabled) pex_android_tv_flush(); g_android_tv_texture_enabled = 1; }
    else if (cap == GL_ALPHA_TEST) { if (!g_android_tv_alpha_enabled) pex_android_tv_flush(); g_android_tv_alpha_enabled = 1; }
    else if (cap == GL_BLEND) { if (!g_android_tv_blend_enabled) pex_android_tv_flush(); g_android_tv_blend_enabled = 1; }
    else if (cap == GL_DEPTH_TEST) { if (!g_android_tv_depth_enabled) pex_android_tv_flush(); g_android_tv_depth_enabled = 1; }
    else if (cap == GL_CULL_FACE) { if (!g_android_tv_cull_enabled) pex_android_tv_flush(); g_android_tv_cull_enabled = 1; }
    else { pex_android_tv_flush(); glEnable(cap); }
}

static void pex_android_tv_glDisable(GLenum cap) {
    if (cap == GL_TEXTURE_2D) { if (g_android_tv_texture_enabled) pex_android_tv_flush(); g_android_tv_texture_enabled = 0; }
    else if (cap == GL_ALPHA_TEST) { if (g_android_tv_alpha_enabled) pex_android_tv_flush(); g_android_tv_alpha_enabled = 0; }
    else if (cap == GL_BLEND) { if (g_android_tv_blend_enabled) pex_android_tv_flush(); g_android_tv_blend_enabled = 0; }
    else if (cap == GL_DEPTH_TEST) { if (g_android_tv_depth_enabled) pex_android_tv_flush(); g_android_tv_depth_enabled = 0; }
    else if (cap == GL_CULL_FACE) { if (g_android_tv_cull_enabled) pex_android_tv_flush(); g_android_tv_cull_enabled = 0; }
    else if (cap == GL_FOG) { }
    else { pex_android_tv_flush(); glDisable(cap); }
}

static void pex_android_tv_glMatrixMode(GLenum mode) { g_android_tv_matrix_mode = mode; }
static void pex_android_tv_glLoadIdentity(void) { if (g_android_tv_matrix_mode == GL_PROJECTION) pex_android_tv_flush(); android_tv_mat_identity(android_tv_current_matrix()); }
static void pex_android_tv_glPushMatrix(void) {
    if (g_android_tv_matrix_mode == GL_PROJECTION) pex_android_tv_flush();
    if (g_android_tv_matrix_mode == GL_PROJECTION) { if (g_android_tv_proj_top < 31) { g_android_tv_proj_stack[g_android_tv_proj_top+1] = g_android_tv_proj_stack[g_android_tv_proj_top]; g_android_tv_proj_top++; } }
    else { if (g_android_tv_model_top < 63) { g_android_tv_model_stack[g_android_tv_model_top+1] = g_android_tv_model_stack[g_android_tv_model_top]; g_android_tv_model_top++; } }
}
static void pex_android_tv_glPopMatrix(void) { if (g_android_tv_matrix_mode == GL_PROJECTION) pex_android_tv_flush(); if (g_android_tv_matrix_mode == GL_PROJECTION) { if (g_android_tv_proj_top > 0) g_android_tv_proj_top--; } else { if (g_android_tv_model_top > 0) g_android_tv_model_top--; } }
static void pex_android_tv_glTranslatef(GLfloat x, GLfloat y, GLfloat z) { if (g_android_tv_matrix_mode == GL_PROJECTION) pex_android_tv_flush(); AndroidTVMat4 t; android_tv_mat_identity(&t); t.m[12]=x; t.m[13]=y; t.m[14]=z; android_tv_mat_postmul(android_tv_current_matrix(), &t); }
static void pex_android_tv_glScalef(GLfloat x, GLfloat y, GLfloat z) { if (g_android_tv_matrix_mode == GL_PROJECTION) pex_android_tv_flush(); AndroidTVMat4 s; android_tv_mat_identity(&s); s.m[0]=x; s.m[5]=y; s.m[10]=z; android_tv_mat_postmul(android_tv_current_matrix(), &s); }
static void pex_android_tv_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    if (g_android_tv_matrix_mode == GL_PROJECTION) pex_android_tv_flush();
    GLfloat len = sqrtf(x*x + y*y + z*z);
    if (len <= 0.00001f) return;
    x /= len; y /= len; z /= len;
    GLfloat rad = angle * (GLfloat)(M_PI / 180.0);
    GLfloat c = cosf(rad), s = sinf(rad), ic = 1.0f - c;
    AndroidTVMat4 r;
    r.m[0]=x*x*ic+c;   r.m[4]=x*y*ic-z*s; r.m[8] =x*z*ic+y*s; r.m[12]=0;
    r.m[1]=y*x*ic+z*s; r.m[5]=y*y*ic+c;   r.m[9] =y*z*ic-x*s; r.m[13]=0;
    r.m[2]=z*x*ic-y*s; r.m[6]=z*y*ic+x*s; r.m[10]=z*z*ic+c;   r.m[14]=0;
    r.m[3]=0;          r.m[7]=0;          r.m[11]=0;          r.m[15]=1;
    android_tv_mat_postmul(android_tv_current_matrix(), &r);
}
static void pex_android_tv_glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) {
    pex_android_tv_flush();
    AndroidTVMat4 o; android_tv_mat_identity(&o);
    o.m[0] = (GLfloat)(2.0 / (r - l));
    o.m[5] = (GLfloat)(2.0 / (t - b));
    o.m[10] = (GLfloat)(-2.0 / (f - n));
    o.m[12] = (GLfloat)(-(r + l) / (r - l));
    o.m[13] = (GLfloat)(-(t + b) / (t - b));
    o.m[14] = (GLfloat)(-(f + n) / (f - n));
    android_tv_mat_postmul(android_tv_current_matrix(), &o);
}
static void pex_android_tv_gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble znear, GLdouble zfar) {
    pex_android_tv_flush();
    GLdouble ymax = znear * tan(fovy * M_PI / 360.0);
    GLdouble xmax = ymax * aspect;
    GLdouble l = -xmax, r = xmax, b = -ymax, t = ymax;
    AndroidTVMat4 p; memset(&p, 0, sizeof(p));
    p.m[0] = (GLfloat)((2.0 * znear) / (r - l));
    p.m[5] = (GLfloat)((2.0 * znear) / (t - b));
    p.m[8] = (GLfloat)((r + l) / (r - l));
    p.m[9] = (GLfloat)((t + b) / (t - b));
    p.m[10] = (GLfloat)(-(zfar + znear) / (zfar - znear));
    p.m[11] = -1.0f;
    p.m[14] = (GLfloat)(-(2.0 * zfar * znear) / (zfar - znear));
    android_tv_mat_postmul(android_tv_current_matrix(), &p);
}

static void pex_android_tv_glGetFloatv(GLenum pname, GLfloat *params) { if (!params) return; if (pname == GL_PROJECTION_MATRIX) memcpy(params, g_android_tv_proj_stack[g_android_tv_proj_top].m, 16*sizeof(GLfloat)); else if (pname == GL_MODELVIEW_MATRIX) memcpy(params, g_android_tv_model_stack[g_android_tv_model_top].m, 16*sizeof(GLfloat)); else glGetFloatv(pname, params); }
static void pex_android_tv_glGetDoublev(GLenum pname, GLdouble *params) { if (!params) return; GLfloat f[16]; pex_android_tv_glGetFloatv(pname, f); for (int i=0;i<16;i++) params[i] = (GLdouble)f[i]; }
static void pex_android_tv_glGetIntegerv(GLenum pname, GLint *params) { if (!params) return; if (pname == GL_VIEWPORT) memcpy(params, g_android_tv_viewport, sizeof(g_android_tv_viewport)); else glGetIntegerv(pname, params); }
static void pex_android_tv_glFogi(GLenum pname, GLint param) { (void)pname; (void)param; }
static void pex_android_tv_glFogf(GLenum pname, GLfloat param) { (void)pname; (void)param; }
static void pex_android_tv_glFogfv(GLenum pname, const GLfloat *params) { (void)pname; (void)params; }

static void android_tv_transform_modelview(GLfloat x, GLfloat y, GLfloat z,
                                           GLfloat *ox, GLfloat *oy, GLfloat *oz) {
    const AndroidTVMat4 *m = &g_android_tv_model_stack[g_android_tv_model_top];
    GLfloat tx = m->m[0] * x + m->m[4] * y + m->m[8]  * z + m->m[12];
    GLfloat ty = m->m[1] * x + m->m[5] * y + m->m[9]  * z + m->m[13];
    GLfloat tz = m->m[2] * x + m->m[6] * y + m->m[10] * z + m->m[14];
    GLfloat tw = m->m[3] * x + m->m[7] * y + m->m[11] * z + m->m[15];
    if (fabsf(tw) > 0.000001f && fabsf(tw - 1.0f) > 0.000001f) {
        tx /= tw; ty /= tw; tz /= tw;
    }
    *ox = tx; *oy = ty; *oz = tz;
}

static void pex_android_tv_glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { g_android_tv_cur_color[0]=r; g_android_tv_cur_color[1]=g; g_android_tv_cur_color[2]=b; g_android_tv_cur_color[3]=a; }
static void pex_android_tv_glTexCoord2f(GLfloat u, GLfloat v) { g_android_tv_cur_u = u; g_android_tv_cur_v = v; }
static void pex_android_tv_glBegin(GLenum mode) { g_android_tv_begin_active = 1; g_android_tv_begin_mode = mode; g_android_tv_imm_count = 0; }
static void pex_android_tv_glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
    if (!g_android_tv_begin_active) return;
    if (!android_tv_ensure_imm(g_android_tv_imm_count + 1)) return;
    AndroidTVVertex *v = &g_android_tv_imm[g_android_tv_imm_count++];
    if (g_android_tv_compiling) { v->x = x; v->y = y; v->z = z; }
    else android_tv_transform_modelview(x, y, z, &v->x, &v->y, &v->z);
    v->u = g_android_tv_cur_u; v->v = g_android_tv_cur_v;
    v->r = g_android_tv_cur_color[0]; v->g = g_android_tv_cur_color[1]; v->b = g_android_tv_cur_color[2]; v->a = g_android_tv_cur_color[3];
}
static void pex_android_tv_glVertex2f(GLfloat x, GLfloat y) { pex_android_tv_glVertex3f(x, y, 0.0f); }
static void pex_android_tv_glEnd(void) { if (!g_android_tv_begin_active) return; g_android_tv_begin_active = 0; android_tv_draw_or_store(); }

static GLuint pex_android_tv_glGenLists(GLsizei range) { GLuint base = g_android_tv_next_list; int r = range > 0 ? range : 1; for (int i=0;i<r;i++) if (base + (GLuint)i < PEX_ANDROID_TV_MAX_LISTS) android_tv_list_free(base + (GLuint)i); g_android_tv_next_list += (GLuint)r; if (g_android_tv_next_list >= PEX_ANDROID_TV_MAX_LISTS) g_android_tv_next_list = PEX_ANDROID_TV_MAX_LISTS - 1; return base; }
static void pex_android_tv_glNewList(GLuint list, GLenum mode) { (void)mode; if (list > 0 && list < PEX_ANDROID_TV_MAX_LISTS) android_tv_list_free(list); g_android_tv_compiling = 1; g_android_tv_compile_list = list; }
static void pex_android_tv_glEndList(void) { g_android_tv_compiling = 0; g_android_tv_compile_list = 0; }
static void pex_android_tv_glCallList(GLuint list) { pex_android_tv_flush(); if (list == 0 || list >= PEX_ANDROID_TV_MAX_LISTS) return; for (AndroidTVBatch *b = g_android_tv_lists[list].first; b; b = b->next) android_tv_draw_raw(b->v, b->count, b->mode, b); }

static void pex_android_tv_glGenTextures(GLsizei n, GLuint *textures) { glGenTextures(n, textures); }
static void pex_android_tv_glBindTexture(GLenum target, GLuint texture) { if (target == GL_TEXTURE_2D) { if (g_android_tv_bound_tex != texture) pex_android_tv_flush(); g_android_tv_bound_tex = texture; glActiveTexture(GL_TEXTURE0); if (g_android_tv_applied_tex != texture) { glBindTexture(target, texture); g_android_tv_applied_tex = texture; } } else { pex_android_tv_flush(); glBindTexture(target, texture); } }
static void pex_android_tv_glTexParameteri(GLenum target, GLenum pname, GLint param) { pex_android_tv_flush(); if (param == GL_CLAMP) param = GL_CLAMP_TO_EDGE; glTexParameteri(target, pname, param); }
static void pex_android_tv_glTexImage2D(GLenum target, GLint level, GLint internal, GLsizei w, GLsizei h, GLint border, GLenum format, GLenum type, const GLvoid *pixels) { pex_android_tv_flush(); if (internal == 4) internal = GL_RGBA; if (format == GL_BGRA) format = GL_RGBA; glTexImage2D(target, level, internal, w, h, border, format, type, pixels); }
static void pex_android_tv_glDeleteTextures(GLsizei n, const GLuint *textures) { pex_android_tv_flush(); glDeleteTextures(n, textures); g_android_tv_applied_valid = 0; }
static void android_gl_copy_tex_sub_image_2d(GLenum target, GLint level, GLint xoff, GLint yoff, GLint x, GLint y, GLsizei w, GLsizei h) { pex_android_tv_flush(); glCopyTexSubImage2D(target, level, xoff, yoff, x, y, w, h); }
static void pex_android_tv_glPixelStorei(GLenum pname, GLint param) { pex_android_tv_flush(); glPixelStorei(pname, param); }
static const GLubyte *pex_android_tv_glGetString(GLenum name) { return glGetString(name); }

static void pex_android_tv_glEnableClientState(GLenum array) { if (array == GL_VERTEX_ARRAY) g_android_tv_vertex_array_enabled = 1; else if (array == GL_TEXTURE_COORD_ARRAY) g_android_tv_texcoord_array_enabled = 1; else if (array == GL_COLOR_ARRAY) g_android_tv_color_array_enabled = 1; }
static void pex_android_tv_glDisableClientState(GLenum array) { if (array == GL_VERTEX_ARRAY) g_android_tv_vertex_array_enabled = 0; else if (array == GL_TEXTURE_COORD_ARRAY) g_android_tv_texcoord_array_enabled = 0; else if (array == GL_COLOR_ARRAY) g_android_tv_color_array_enabled = 0; }
static void pex_android_tv_glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) { g_android_tv_vertex_size=size; g_android_tv_vertex_type=type; g_android_tv_vertex_stride=stride; g_android_tv_vertex_ptr=pointer; }
static void pex_android_tv_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) { g_android_tv_texcoord_size=size; g_android_tv_texcoord_type=type; g_android_tv_texcoord_stride=stride; g_android_tv_texcoord_ptr=pointer; }
static void pex_android_tv_glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) { g_android_tv_color_size=size; g_android_tv_color_type=type; g_android_tv_color_stride=stride; g_android_tv_color_ptr=pointer; }
static int gles2_ensure_index16_buffer(int needed) {
    if (needed <= g_android_tv_index16_cap) return 1;
    int cap = g_android_tv_index16_cap ? g_android_tv_index16_cap : 4096;
    while (cap < needed) cap *= 2;
    GLushort *ni = (GLushort*)realloc(g_android_tv_index16_tmp, (size_t)cap * sizeof(*ni));
    if (!ni) return 0;
    g_android_tv_index16_tmp = ni;
    g_android_tv_index16_cap = cap;
    return 1;
}

static int gles2_arrays_are_interleaved(const unsigned char **base_out, GLsizei *stride_out) {
    if (!g_android_tv_vertex_array_enabled || !g_android_tv_texcoord_array_enabled || !g_android_tv_color_array_enabled) return 0;
    if (!g_android_tv_vertex_ptr || !g_android_tv_texcoord_ptr || !g_android_tv_color_ptr) return 0;
    if (g_android_tv_vertex_size != 3 || g_android_tv_vertex_type != GL_FLOAT) return 0;
    if (g_android_tv_texcoord_size != 2 || g_android_tv_texcoord_type != GL_FLOAT) return 0;
    if (g_android_tv_color_size != 4 || g_android_tv_color_type != GL_UNSIGNED_BYTE) return 0;
    GLsizei vs = g_android_tv_vertex_stride ? g_android_tv_vertex_stride : (GLsizei)(3 * sizeof(GLfloat));
    GLsizei ts = g_android_tv_texcoord_stride ? g_android_tv_texcoord_stride : (GLsizei)(2 * sizeof(GLfloat));
    GLsizei cs = g_android_tv_color_stride ? g_android_tv_color_stride : (GLsizei)(4 * sizeof(GLubyte));
    if (vs != (GLsizei)sizeof(PexVertex) || ts != (GLsizei)sizeof(PexVertex) || cs != (GLsizei)sizeof(PexVertex)) return 0;
    const unsigned char *base = ((const unsigned char*)g_android_tv_vertex_ptr) - offsetof(PexVertex, x);
    if ((const unsigned char*)g_android_tv_texcoord_ptr != base + offsetof(PexVertex, u)) return 0;
    if ((const unsigned char*)g_android_tv_color_ptr != base + offsetof(PexVertex, color)) return 0;
    if (base_out) *base_out = base;
    if (stride_out) *stride_out = vs;
    return 1;
}

static void pex_android_tv_update_lightmap(const unsigned char *rgba, unsigned int version) {
    if (!rgba || !g_android_tv_lightmap_tex || g_android_tv_lightmap_version == version) return;
    pex_android_tv_flush();
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_android_tv_lightmap_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glActiveTexture(GL_TEXTURE0);
    g_android_tv_lightmap_version = version;
}

static void pex_android_tv_static_mesh_destroy(AndroidTVStaticMesh *mesh) {
    if (!mesh) return;
    pex_android_tv_flush();
    AndroidTVStaticMeshBatch *b = mesh->first;
    while (b) {
        AndroidTVStaticMeshBatch *next = b->next;
        if (b->ibo) glDeleteBuffers(1, &b->ibo);
        if (b->vbo) glDeleteBuffers(1, &b->vbo);
        free(b);
        b = next;
    }
    free(mesh);
}

static AndroidTVStaticMesh *pex_android_tv_static_mesh_create(const PexVertex *vertices, uint32_t vertex_count,
                                                               const uint32_t *indices, uint32_t index_count) {
    if (!vertices || !indices || vertex_count == 0 || index_count < 3) return NULL;
    pex_android_tv_flush();
    AndroidTVStaticMesh *mesh = (AndroidTVStaticMesh*)calloc(1, sizeof(*mesh));
    if (!mesh) return NULL;
    mesh->vertex_count = vertex_count;
    mesh->index_count = index_count;

    uint32_t tri = 0;
    while (tri + 2u < index_count) {
        uint32_t first = tri;
        uint32_t min_idx = UINT32_MAX, max_idx = 0, out_count = 0;
        while (tri + 2u < index_count) {
            uint32_t a = indices[tri], b = indices[tri + 1u], c = indices[tri + 2u];
            uint32_t tmin = a < b ? (a < c ? a : c) : (b < c ? b : c);
            uint32_t tmax = a > b ? (a > c ? a : c) : (b > c ? b : c);
            if (tmax >= vertex_count) { pex_android_tv_static_mesh_destroy(mesh); return NULL; }
            uint32_t nmin = min_idx == UINT32_MAX || tmin < min_idx ? tmin : min_idx;
            uint32_t nmax = tmax > max_idx ? tmax : max_idx;
            if (out_count && nmax - nmin > 65535u) break;
            min_idx = nmin; max_idx = nmax; out_count += 3u; tri += 3u;
        }
        if (!out_count || min_idx == UINT32_MAX) {
            if (tri == first) tri += 3u;
            continue;
        }
        GLushort *rebased = (GLushort*)malloc((size_t)out_count * sizeof(*rebased));
        AndroidTVStaticMeshBatch *batch = (AndroidTVStaticMeshBatch*)calloc(1, sizeof(*batch));
        if (!rebased || !batch) { free(rebased); free(batch); pex_android_tv_static_mesh_destroy(mesh); return NULL; }
        for (uint32_t i = 0; i < out_count; ++i) rebased[i] = (GLushort)(indices[first + i] - min_idx);
        glGenBuffers(1, &batch->vbo);
        glGenBuffers(1, &batch->ibo);
        if (!batch->vbo || !batch->ibo) { free(rebased); free(batch); pex_android_tv_static_mesh_destroy(mesh); return NULL; }
        uint32_t span = max_idx - min_idx + 1u;
        glBindBuffer(GL_ARRAY_BUFFER, batch->vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)((size_t)span * sizeof(PexVertex)), vertices + min_idx, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch->ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)((size_t)out_count * sizeof(GLushort)), rebased, GL_STATIC_DRAW);
        free(rebased);
        batch->index_count = (GLsizei)out_count;
        if (!mesh->first) mesh->first = batch; else mesh->last->next = batch;
        mesh->last = batch;
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (!mesh->first) { pex_android_tv_static_mesh_destroy(mesh); return NULL; }
    return mesh;
}

static void pex_android_tv_static_mesh_begin(GLuint texture, int alpha_test, int blend, int cull) {
    pex_android_tv_flush();
    if (!g_android_tv_terrain_program) return;

    glUseProgram(g_android_tv_terrain_program);
    AndroidTVMat4 mvp;
    android_tv_make_mvp(&mvp);
    if (g_android_tv_terrain_u_mvp >= 0)
        glUniformMatrix4fv(g_android_tv_terrain_u_mvp, 1, GL_FALSE, mvp.m);
    if (g_android_tv_terrain_u_alpha_enabled >= 0)
        glUniform1i(g_android_tv_terrain_u_alpha_enabled, alpha_test ? 1 : 0);
    if (g_android_tv_terrain_u_alpha_ref >= 0)
        glUniform1f(g_android_tv_terrain_u_alpha_ref, g_android_tv_alpha_ref);

    if (blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (g_android_tv_depth_enabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (cull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_android_tv_lightmap_tex);
    glActiveTexture(GL_TEXTURE0);

    if (cull) glFrontFace(GL_CW);
    if (g_android_tv_terrain_a_pos >= 0) glEnableVertexAttribArray((GLuint)g_android_tv_terrain_a_pos);
    if (g_android_tv_terrain_a_uv >= 0) glEnableVertexAttribArray((GLuint)g_android_tv_terrain_a_uv);
    if (g_android_tv_terrain_a_color >= 0) glEnableVertexAttribArray((GLuint)g_android_tv_terrain_a_color);
    if (g_android_tv_terrain_a_light >= 0) glEnableVertexAttribArray((GLuint)g_android_tv_terrain_a_light);

    /* The terrain shader has its own program and bindings.  Force the compact
       UI state cache to reapply its values on the next compatibility draw. */
    g_android_tv_applied_valid = 0;
    g_android_tv_applied_mvp_valid = 0;
    g_android_tv_static_pass_active = 1;
}

static void pex_android_tv_static_mesh_draw(AndroidTVStaticMesh *mesh) {
    if (!mesh || !g_android_tv_static_pass_active) return;
    for (AndroidTVStaticMeshBatch *b = mesh->first; b; b = b->next) {
        glBindBuffer(GL_ARRAY_BUFFER, b->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b->ibo);
        if (g_android_tv_terrain_a_pos >= 0)
            glVertexAttribPointer((GLuint)g_android_tv_terrain_a_pos, 3, GL_FLOAT, GL_FALSE,
                                  sizeof(PexVertex), (const GLvoid*)(uintptr_t)offsetof(PexVertex, x));
        if (g_android_tv_terrain_a_uv >= 0)
            glVertexAttribPointer((GLuint)g_android_tv_terrain_a_uv, 2, GL_FLOAT, GL_FALSE,
                                  sizeof(PexVertex), (const GLvoid*)(uintptr_t)offsetof(PexVertex, u));
        if (g_android_tv_terrain_a_color >= 0)
            glVertexAttribPointer((GLuint)g_android_tv_terrain_a_color, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                                  sizeof(PexVertex), (const GLvoid*)(uintptr_t)offsetof(PexVertex, color));
        if (g_android_tv_terrain_a_light >= 0)
            glVertexAttribPointer((GLuint)g_android_tv_terrain_a_light, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                                  sizeof(PexVertex), (const GLvoid*)(uintptr_t)offsetof(PexVertex, light));
        glDrawElements(GL_TRIANGLES, b->index_count, GL_UNSIGNED_SHORT, (const GLvoid*)0);
    }
}

static void pex_android_tv_static_mesh_end(void) {
    if (!g_android_tv_static_pass_active) return;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glFrontFace(GL_CCW);
    glActiveTexture(GL_TEXTURE0);
    g_android_tv_static_pass_active = 0;
    g_android_tv_applied_valid = 0;
    g_android_tv_applied_mvp_valid = 0;
}

static int android_tv_draw_elements_fast(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
    if (!g_android_tv_program || !g_android_tv_vbo || !g_android_tv_ebo) return 0;
    if (mode != GL_TRIANGLES && mode != GL_LINES && mode != GL_LINE_STRIP && mode != GL_TRIANGLE_STRIP) return 0;
    const unsigned char *base = NULL;
    GLsizei stride = 0;
    if (!gles2_arrays_are_interleaved(&base, &stride)) return 0;
    if (type != GL_UNSIGNED_INT && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_BYTE) return 0;

    unsigned int max_idx = 0;
    for (int i = 0; i < count; ++i) {
        unsigned int idx = android_tv_read_index(indices, type, i);
        if (idx > max_idx) max_idx = idx;
    }
    if (max_idx > 262143u) return 0;
    if (type == GL_UNSIGNED_INT && max_idx > 65535u) return 0;

    AndroidTVMat4 mvp;
    android_tv_make_mvp(&mvp);
    glUseProgram(g_android_tv_program);
    android_tv_apply_mvp(&mvp);
    android_tv_apply_state_for_batch(NULL);

    glBindBuffer(GL_ARRAY_BUFFER, g_android_tv_vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(((size_t)max_idx + 1u) * (size_t)stride), base, GL_DYNAMIC_DRAW);
    if (g_android_tv_a_pos >= 0) {
        glEnableVertexAttribArray((GLuint)g_android_tv_a_pos);
        glVertexAttribPointer((GLuint)g_android_tv_a_pos, 3, GL_FLOAT, GL_FALSE, stride, (const GLvoid*)(uintptr_t)offsetof(PexVertex, x));
    }
    if (g_android_tv_a_uv >= 0) {
        glEnableVertexAttribArray((GLuint)g_android_tv_a_uv);
        glVertexAttribPointer((GLuint)g_android_tv_a_uv, 2, GL_FLOAT, GL_FALSE, stride, (const GLvoid*)(uintptr_t)offsetof(PexVertex, u));
    }
    if (g_android_tv_a_color >= 0) {
        glEnableVertexAttribArray((GLuint)g_android_tv_a_color);
        glVertexAttribPointer((GLuint)g_android_tv_a_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (const GLvoid*)(uintptr_t)offsetof(PexVertex, color));
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_android_tv_ebo);
    GLenum draw_type = type;
    const GLvoid *draw_offset = (const GLvoid*)0;
    if (type == GL_UNSIGNED_INT && max_idx <= 65535u) {
        if (!gles2_ensure_index16_buffer(count)) { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); glBindBuffer(GL_ARRAY_BUFFER, 0); return 0; }
        const GLuint *src = (const GLuint*)indices;
        for (int i = 0; i < count; ++i) g_android_tv_index16_tmp[i] = (GLushort)src[i];
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)((size_t)count * sizeof(GLushort)), g_android_tv_index16_tmp, GL_DYNAMIC_DRAW);
        draw_type = GL_UNSIGNED_SHORT;
    } else {
        int index_size = android_tv_sizeof_type(type);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)((size_t)count * (size_t)index_size), indices, GL_DYNAMIC_DRAW);
    }
    glDrawElements(mode, count, draw_type, draw_offset);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return 1;
}

static void pex_android_tv_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
    pex_android_tv_flush();
    if (count <= 0 || !indices) return;
    if (android_tv_draw_elements_fast(mode, count, type, indices)) return;
    AndroidTVVertex *tmp = (AndroidTVVertex*)malloc((size_t)count * sizeof(*tmp));
    if (!tmp) return;
    for (int i = 0; i < count; ++i) tmp[i] = android_tv_vertex_from_arrays(android_tv_read_index(indices, type, i));
    android_tv_draw_raw(tmp, count, mode, NULL);
    free(tmp);
}

static int pex_android_tv_gluProject(GLdouble objx, GLdouble objy, GLdouble objz,
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

static void gles2_renderer_shutdown(void) {
    pex_android_tv_flush();
    android_tv_world_target_destroy();
    android_tv_panorama_fx_destroy();
    for (GLuint i = 1; i < PEX_ANDROID_TV_MAX_LISTS; ++i) android_tv_list_free(i);
    free(g_android_tv_imm); g_android_tv_imm = NULL; g_android_tv_imm_cap = 0;
    free(g_android_tv_deferred.v); memset(&g_android_tv_deferred, 0, sizeof(g_android_tv_deferred));
    free(g_android_tv_index16_tmp); g_android_tv_index16_tmp = NULL; g_android_tv_index16_cap = 0;
    if (g_android_tv_lightmap_tex) { glDeleteTextures(1, &g_android_tv_lightmap_tex); g_android_tv_lightmap_tex = 0; }
    if (g_android_tv_ebo) { glDeleteBuffers(1, &g_android_tv_ebo); g_android_tv_ebo = 0; }
    if (g_android_tv_vbo) { glDeleteBuffers(1, &g_android_tv_vbo); g_android_tv_vbo = 0; }
    if (g_android_tv_terrain_program) { glDeleteProgram(g_android_tv_terrain_program); g_android_tv_terrain_program = 0; }
    if (g_android_tv_program) { glDeleteProgram(g_android_tv_program); g_android_tv_program = 0; }
}
