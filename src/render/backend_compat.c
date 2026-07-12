/* Native renderer compatibility layer.
   OpenGL mode forwards to real OpenGL. Direct3D 9 and Direct3D 11 modes use
   native devices and GPU vertex buffers; no OpenGL context/wrapper is created
   for the D3D backends. */

#if defined(PEX_PLATFORM_XBOX_UWP)
#include "render/d3d9_uwp_stub.h"
#else
#include <d3d9.h>
#endif
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#ifndef ID3DBlob_Release
#define ID3DBlob_Release ID3D10Blob_Release
#endif
#ifndef ID3DBlob_GetBufferPointer
#define ID3DBlob_GetBufferPointer ID3D10Blob_GetBufferPointer
#endif
#ifndef ID3DBlob_GetBufferSize
#define ID3DBlob_GetBufferSize ID3D10Blob_GetBufferSize
#endif

#ifndef DXGI_PRESENT_ALLOW_TEARING
#define DXGI_PRESENT_ALLOW_TEARING 0x00000200U
#endif
#ifndef DXGI_PRESENT_DO_NOT_WAIT
#define DXGI_PRESENT_DO_NOT_WAIT 0x00000008U
#endif
#ifndef DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
#define DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING 0x00000800U
#endif
#ifndef DXGI_SWAP_EFFECT_FLIP_DISCARD
#define DXGI_SWAP_EFFECT_FLIP_DISCARD ((DXGI_SWAP_EFFECT)4)
#endif

#ifndef GL_CLAMP
#define GL_CLAMP 0x2900
#endif
#ifndef GL_REPEAT
#define GL_REPEAT 0x2901
#endif

#define PEX_D3D_MAX_TEXTURES 4096
#define PEX_D3D_MAX_LISTS 65536
#define PEX_D3D_STACK_MAX 64
#define PEX_D3D_FVF (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)
#define PEX_D3D_STATE_MAX 256
#define PEX_D3D_TSS_MAX 64
#define PEX_D3D_SAMPLER_MAX 16

typedef struct PexMat4 { float m[16]; } PexMat4; /* OpenGL column-major */
typedef struct PexRawVertex { float x,y,z,u,v; unsigned int color; } PexRawVertex;
/* Hardware-transformed D3D9 vertex.  The first D3D9 attempt used
   D3DFVF_XYZRHW and manually projected vertices on the CPU.  That breaks real
   3D because triangles crossing the near plane are never clipped; they become
   giant screen-space spikes.  Use normal XYZ vertices and let D3D9 do clip and
   perspective-correct interpolation. */
typedef struct PexD3DVertex { float x,y,z; DWORD color; float u,v; } PexD3DVertex;
typedef struct PexD3DStateSnap {
    int texture_enabled;
    unsigned int texture_id;
    int blend_enabled;
    DWORD src_blend;
    DWORD dst_blend;
    int alpha_test_enabled;
    int depth_enabled;
    int depth_write;
    DWORD depth_func;
    int fog_enabled;
    float fog_start, fog_end;
    DWORD fog_color;
    int cull_enabled;
    DWORD color_write_mask;
} PexD3DStateSnap;
typedef struct PexD3DBatch {
    int mode;
    PexD3DStateSnap state;
    /* Raw GL-style vertices are used only while a glBegin/glEnd batch is being
       collected.  Compiled display-list batches are converted to D3D-ready
       vertices once and uploaded to a static vertex buffer. */
    PexRawVertex *v;
    int count, cap;
    D3DPRIMITIVETYPE d3d_prim;
    UINT primitive_count;
    UINT vertex_count;
    IDirect3DVertexBuffer9 *vb;
    ID3D11Buffer *vb11;
    PexD3DVertex *sysmem;
    int sysmem_count;
} PexD3DBatch;
typedef struct PexD3DList {
    PexD3DBatch *batches;
    int count, cap;
} PexD3DList;
typedef struct PexD3DStateCache {
    int valid;
    unsigned char rs_valid[PEX_D3D_STATE_MAX];
    DWORD rs[PEX_D3D_STATE_MAX];
    unsigned char tss_valid[PEX_D3D_TSS_MAX];
    DWORD tss[PEX_D3D_TSS_MAX];
    unsigned char sampler_valid[PEX_D3D_SAMPLER_MAX];
    DWORD sampler[PEX_D3D_SAMPLER_MAX];
    int texture_valid;
    IDirect3DBaseTexture9 *texture0;
    int fvf_valid;
    DWORD fvf;
    int stream_valid;
    IDirect3DVertexBuffer9 *stream0;
    UINT stride0;
} PexD3DStateCache;
typedef struct PexD3DTexture {
    IDirect3DTexture9 *tex;
    ID3D11Texture2D *tex11;
    ID3D11ShaderResourceView *srv11;
    int w, h;
    DWORD address_u, address_v;
    DWORD min_filter, mag_filter;
} PexD3DTexture;

typedef struct PexD3DBackend {
    int active;
    HWND hwnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *dev;
    D3DPRESENT_PARAMETERS pp;
    D3DFORMAT depth_format;
    int lost;
    int scene_open;
    int width, height;
    PexD3DTexture textures[PEX_D3D_MAX_TEXTURES];
    unsigned int next_texture;
    PexD3DList *lists[PEX_D3D_MAX_LISTS];
    unsigned int next_list;
    unsigned int bound_texture;
    int texture_enabled;
    int blend_enabled;
    DWORD src_blend, dst_blend;
    int alpha_test_enabled;
    int depth_enabled;
    int depth_write;
    DWORD depth_func;
    int cull_enabled;
    int fog_enabled;
    float fog_start, fog_end;
    DWORD fog_color;
    DWORD color_write_mask;
    float color[4];
    DWORD clear_color;
    GLenum matrix_mode;
    PexMat4 modelview;
    PexMat4 projection;
    PexMat4 mv_stack[PEX_D3D_STACK_MAX]; int mv_sp;
    PexMat4 pr_stack[PEX_D3D_STACK_MAX]; int pr_sp;
    int viewport_x, viewport_y, viewport_w, viewport_h;
    int begin_active;
    int begin_mode;
    PexD3DBatch immediate;
    int compiling;
    unsigned int compile_list;
    PexD3DBatch *compile_batch;
    PexD3DVertex *scratch;
    int scratch_cap;
    IDirect3DVertexBuffer9 *dynamic_vb;
    UINT dynamic_vb_capacity;
    PexD3DStateCache cache;
    int matrix_dirty;
    int matrix_cache_valid;
} PexD3DBackend;

static PexD3DBackend g_d3d9;


#define PEX_D3D11_MAX_FRAMES 2

typedef struct PexD3D11CB {
    float model[16];
    float proj[16];
    float flags[4];
    float fog_color[4];
    float fog_params[4];
} PexD3D11CB;

typedef struct PexD3D11Backend {
    int active;
    HWND hwnd;
    IDXGISwapChain *swap;
    ID3D11Device *dev;
    ID3D11DeviceContext *ctx;
    ID3D11RenderTargetView *rtv;
    ID3D11Texture2D *depth_tex;
    ID3D11DepthStencilView *dsv;
    ID3D11VertexShader *vs;
    ID3D11PixelShader *ps;
    ID3D11InputLayout *layout;
    ID3D11Buffer *cbuffer;
    ID3D11Buffer *dynamic_vb;
    UINT dynamic_vb_capacity;
    ID3D11SamplerState *sampler_wrap;
    ID3D11SamplerState *sampler_clamp;
    ID3D11SamplerState *sampler_wrap_linear;
    ID3D11SamplerState *sampler_clamp_linear;
    ID3D11BlendState *blend_off;
    ID3D11BlendState *blend_alpha;
    ID3D11BlendState *blend_additive;
    ID3D11BlendState *blend_multiply;
    ID3D11BlendState *blend_color_off;
    ID3D11DepthStencilState *depth_on_write;
    ID3D11DepthStencilState *depth_on_nowrite;
    ID3D11DepthStencilState *depth_off;
    ID3D11RasterizerState *rast_nocull;
    int width, height;
    int allow_tearing;
    UINT swap_flags;
    int buffer_count;
    int frame_latency;
    int frame_latency_set;
    int draw_calls;
    int triangles;
} PexD3D11Backend;

typedef struct PexGPUImmediateStream {
    int active;
    int mode;
    D3DPRIMITIVETYPE d3d_prim;
    D3D11_PRIMITIVE_TOPOLOGY d3d11_top;
    PexD3DStateSnap state;
    PexD3DVertex *vertices;
    int count;
    int cap;
} PexGPUImmediateStream;

static PexGPUImmediateStream g_gpu_imm;

static PexD3D11Backend g_d3d11;
static void compat_d3d11_release_failed_device(IDXGISwapChain **swap, ID3D11Device **dev, ID3D11DeviceContext **ctx) {
    if (ctx && *ctx) { ID3D11DeviceContext_Release(*ctx); *ctx = NULL; }
    if (swap && *swap) { IDXGISwapChain_Release(*swap); *swap = NULL; }
    if (dev && *dev) { ID3D11Device_Release(*dev); *dev = NULL; }
}

static HRESULT compat_d3d11_create_swap_chain(DXGI_SWAP_CHAIN_DESC *base_desc,
                                                         const D3D_FEATURE_LEVEL *levels,
                                                         UINT level_count,
                                                         D3D_FEATURE_LEVEL *got_level) {
    D3D_DRIVER_TYPE drivers[] = { D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP };
    HRESULT hr = E_FAIL;

    for (int d = 0; d < (int)ARRAY_COUNT(drivers); ++d) {
        for (int attempt = 0; attempt < 3; ++attempt) {
            DXGI_SWAP_CHAIN_DESC desc = *base_desc;
            desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            desc.Flags = 0;
            if (attempt == 0) {
                desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            } else if (attempt == 2) {
                desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
                desc.BufferCount = 1;
            }

#if defined(PEX_PLATFORM_XBOX_UWP)
            hr = E_FAIL;
            (void)levels; (void)level_count; (void)got_level;
#else
            hr = D3D11CreateDeviceAndSwapChain(NULL, drivers[d], NULL, 0,
                                               levels, level_count, D3D11_SDK_VERSION,
                                               &desc, &g_d3d11.swap, &g_d3d11.dev,
                                               got_level, &g_d3d11.ctx);
#endif
            if (SUCCEEDED(hr)) {
                g_d3d11.allow_tearing = (attempt == 0);
                g_d3d11.swap_flags = desc.Flags;
                g_d3d11.buffer_count = (int)desc.BufferCount;
                return hr;
            }
            compat_d3d11_release_failed_device(&g_d3d11.swap, &g_d3d11.dev, &g_d3d11.ctx);
        }
    }
    return hr;
}

static void compat_d3d11_set_latency_for_unlimited_fps(void) {
    int desired = (g_opts.max_fps <= 0) ? 8 : 1;
    g_d3d11.frame_latency = desired;
    g_d3d11.frame_latency_set = 0;
#if defined(_WIN32)
    if (g_d3d11.dev) {
        IDXGIDevice1 *dxgi_dev = NULL;
        if (SUCCEEDED(ID3D11Device_QueryInterface(g_d3d11.dev, &IID_IDXGIDevice1, (void**)&dxgi_dev)) && dxgi_dev) {
            if (SUCCEEDED(IDXGIDevice1_SetMaximumFrameLatency(dxgi_dev, (UINT)desired))) {
                g_d3d11.frame_latency_set = 1;
            }
            IDXGIDevice1_Release(dxgi_dev);
        }
    }
#endif
}

static void pex_gpu_flush_immediate_stream(void);
static void compat_queue_or_draw_batch(const PexD3DBatch *b);
static int pex_using_d3d11(void) { return g_runtime_renderer_backend == RENDERER_D3D11 && g_d3d11.active; }
static int pex_using_gpu_backend(void) { return (g_runtime_renderer_backend == RENDERER_D3D9 && g_d3d9.active) || (g_runtime_renderer_backend == RENDERER_D3D11 && g_d3d11.active); }

static int pex_using_d3d9(void) { return g_runtime_renderer_backend == RENDERER_D3D9 && g_d3d9.active; }

static void pex_mat_identity(PexMat4 *m) {
    memset(m->m, 0, sizeof(m->m));
    m->m[0]=m->m[5]=m->m[10]=m->m[15]=1.0f;
}

static void pex_mat_mul(PexMat4 *out, const PexMat4 *a, const PexMat4 *b) {
    PexMat4 r;
    for (int c=0;c<4;c++) for (int row=0;row<4;row++) {
        r.m[c*4+row] = a->m[0*4+row]*b->m[c*4+0] + a->m[1*4+row]*b->m[c*4+1] +
                       a->m[2*4+row]*b->m[c*4+2] + a->m[3*4+row]*b->m[c*4+3];
    }
    *out = r;
}

static void pex_mat_postmul(PexMat4 *cur, const PexMat4 *rhs) {
    PexMat4 r; pex_mat_mul(&r, cur, rhs); *cur = r;
}

static PexMat4 *pex_current_matrix(void) {
    return (g_d3d9.matrix_mode == GL_PROJECTION) ? &g_d3d9.projection : &g_d3d9.modelview;
}

static void pex_mat_translate(float x, float y, float z) {
    PexMat4 t; pex_mat_identity(&t);
    t.m[12]=x; t.m[13]=y; t.m[14]=z;
    pex_mat_postmul(pex_current_matrix(), &t);
}

static void pex_mat_scale(float x, float y, float z) {
    PexMat4 s; pex_mat_identity(&s);
    s.m[0]=x; s.m[5]=y; s.m[10]=z;
    pex_mat_postmul(pex_current_matrix(), &s);
}

static void pex_mat_rotate(float angle, float x, float y, float z) {
    float len = sqrtf(x*x+y*y+z*z);
    if (len <= 0.000001f) return;
    x/=len; y/=len; z/=len;
    float a = angle * (float)M_PI / 180.0f;
    float c = cosf(a), s = sinf(a), ic = 1.0f - c;
    PexMat4 r; pex_mat_identity(&r);
    r.m[0] = x*x*ic + c;     r.m[4] = x*y*ic - z*s; r.m[8]  = x*z*ic + y*s;
    r.m[1] = y*x*ic + z*s;   r.m[5] = y*y*ic + c;   r.m[9]  = y*z*ic - x*s;
    r.m[2] = z*x*ic - y*s;   r.m[6] = z*y*ic + x*s; r.m[10] = z*z*ic + c;
    pex_mat_postmul(pex_current_matrix(), &r);
}

static void pex_glstyle_perspective(double fovy, double aspect, double znear, double zfar) {
    double f = 1.0 / tan(fovy * 0.5 * M_PI / 180.0);
    PexMat4 p; memset(&p, 0, sizeof(p));
    p.m[0] = (float)(f / aspect);
    p.m[5] = (float)f;
    p.m[10] = (float)((zfar + znear) / (znear - zfar));
    p.m[11] = -1.0f;
    p.m[14] = (float)((2.0 * zfar * znear) / (znear - zfar));
    pex_mat_postmul(pex_current_matrix(), &p);
}

static void compat_depth_remap_gl_to_d3d(PexMat4 *m) {
    /* Convert OpenGL clip depth [-w,+w] to D3D clip depth [0,+w].
       This is applied after projection: z_d3d = 0.5*z_gl + 0.5*w. */
    PexMat4 r; pex_mat_identity(&r);
    r.m[10] = 0.5f;
    r.m[14] = 0.5f;
    pex_mat_postmul(&r, m);
    *m = r;
}

static D3DMATRIX pex_to_d3d_matrix(const PexMat4 *m) {
    /* PexMat4 is OpenGL column-major/column-vector.  D3D fixed function uses
       row-vector math, so assigning the column-major array into D3D rows gives
       the needed transpose. */
    D3DMATRIX d;
    d._11=m->m[0];  d._12=m->m[1];  d._13=m->m[2];  d._14=m->m[3];
    d._21=m->m[4];  d._22=m->m[5];  d._23=m->m[6];  d._24=m->m[7];
    d._31=m->m[8];  d._32=m->m[9];  d._33=m->m[10]; d._34=m->m[11];
    d._41=m->m[12]; d._42=m->m[13]; d._43=m->m[14]; d._44=m->m[15];
    return d;
}

static void pex_d3d_invalidate_cache(void) {
    memset(&g_d3d9.cache, 0, sizeof(g_d3d9.cache));
    g_d3d9.matrix_cache_valid = 0;
    g_d3d9.matrix_dirty = 1;
}

static void compat_d3d_mark_matrix_dirty(void) {
    g_d3d9.matrix_dirty = 1;
}

static DWORD pex_float_as_dword(float f) {
    union { float f; DWORD d; } u;
    u.f = f;
    return u.d;
}

static void compat_d3d_set_render_state(D3DRENDERSTATETYPE state, DWORD value) {
    unsigned int idx = (unsigned int)state;
    if (idx < PEX_D3D_STATE_MAX && g_d3d9.cache.valid && g_d3d9.cache.rs_valid[idx] && g_d3d9.cache.rs[idx] == value) return;
    IDirect3DDevice9_SetRenderState(g_d3d9.dev, state, value);
    if (idx < PEX_D3D_STATE_MAX) {
        g_d3d9.cache.rs_valid[idx] = 1;
        g_d3d9.cache.rs[idx] = value;
    }
}

static void compat_d3d_set_texture_stage(D3DTEXTURESTAGESTATETYPE state, DWORD value) {
    unsigned int idx = (unsigned int)state;
    if (idx < PEX_D3D_TSS_MAX && g_d3d9.cache.valid && g_d3d9.cache.tss_valid[idx] && g_d3d9.cache.tss[idx] == value) return;
    IDirect3DDevice9_SetTextureStageState(g_d3d9.dev, 0, state, value);
    if (idx < PEX_D3D_TSS_MAX) {
        g_d3d9.cache.tss_valid[idx] = 1;
        g_d3d9.cache.tss[idx] = value;
    }
}

static void compat_d3d_set_sampler_state(D3DSAMPLERSTATETYPE state, DWORD value) {
    unsigned int idx = (unsigned int)state;
    if (idx < PEX_D3D_SAMPLER_MAX && g_d3d9.cache.valid && g_d3d9.cache.sampler_valid[idx] && g_d3d9.cache.sampler[idx] == value) return;
    IDirect3DDevice9_SetSamplerState(g_d3d9.dev, 0, state, value);
    if (idx < PEX_D3D_SAMPLER_MAX) {
        g_d3d9.cache.sampler_valid[idx] = 1;
        g_d3d9.cache.sampler[idx] = value;
    }
}

static void pex_d3d_set_texture0(IDirect3DBaseTexture9 *texture) {
    if (g_d3d9.cache.valid && g_d3d9.cache.texture_valid && g_d3d9.cache.texture0 == texture) return;
    IDirect3DDevice9_SetTexture(g_d3d9.dev, 0, texture);
    g_d3d9.cache.texture_valid = 1;
    g_d3d9.cache.texture0 = texture;
}

static void pex_d3d_set_fvf(DWORD fvf) {
    if (g_d3d9.cache.valid && g_d3d9.cache.fvf_valid && g_d3d9.cache.fvf == fvf) return;
    IDirect3DDevice9_SetFVF(g_d3d9.dev, fvf);
    g_d3d9.cache.fvf_valid = 1;
    g_d3d9.cache.fvf = fvf;
}

static void pex_d3d_set_stream0(IDirect3DVertexBuffer9 *vb, UINT stride) {
    if (g_d3d9.cache.valid && g_d3d9.cache.stream_valid && g_d3d9.cache.stream0 == vb && g_d3d9.cache.stride0 == stride) return;
    IDirect3DDevice9_SetStreamSource(g_d3d9.dev, 0, vb, 0, stride);
    g_d3d9.cache.stream_valid = 1;
    g_d3d9.cache.stream0 = vb;
    g_d3d9.cache.stride0 = stride;
}

static void pex_d3d_apply_matrices(void) {
    if (!g_d3d9.dev) return;
    if (!g_d3d9.matrix_dirty && g_d3d9.matrix_cache_valid && g_d3d9.cache.valid) return;
    PexMat4 mvp;
    pex_mat_mul(&mvp, &g_d3d9.projection, &g_d3d9.modelview);
    compat_depth_remap_gl_to_d3d(&mvp);

    D3DMATRIX id; memset(&id, 0, sizeof(id));
    id._11 = id._22 = id._33 = id._44 = 1.0f;
    D3DMATRIX clip = pex_to_d3d_matrix(&mvp);
    IDirect3DDevice9_SetTransform(g_d3d9.dev, D3DTS_WORLD, &id);
    IDirect3DDevice9_SetTransform(g_d3d9.dev, D3DTS_VIEW, &id);
    IDirect3DDevice9_SetTransform(g_d3d9.dev, D3DTS_PROJECTION, &clip);
    g_d3d9.matrix_dirty = 0;
    g_d3d9.matrix_cache_valid = 1;
}

static void pex_copy_vertex(const PexRawVertex *rv, PexD3DVertex *dv) {
    dv->x = rv->x;
    dv->y = rv->y;
    dv->z = rv->z;
    dv->color = rv->color;
    dv->u = rv->u;
    dv->v = rv->v;
}

static DWORD pex_pack_color(float r,float g,float b,float a) {
    int R=(int)(r*255.0f+0.5f), G=(int)(g*255.0f+0.5f), B=(int)(b*255.0f+0.5f), A=(int)(a*255.0f+0.5f);
    if(R<0)R=0;if(R>255)R=255;if(G<0)G=0;if(G>255)G=255;if(B<0)B=0;if(B>255)B=255;if(A<0)A=0;if(A>255)A=255;
    return D3DCOLOR_ARGB(A,R,G,B);
}

static DWORD pex_convert_blend(GLenum f) {
    switch (f) {
        case GL_ZERO: return D3DBLEND_ZERO;
        case GL_ONE: return D3DBLEND_ONE;
        case GL_SRC_ALPHA: return D3DBLEND_SRCALPHA;
        case GL_ONE_MINUS_SRC_ALPHA: return D3DBLEND_INVSRCALPHA;
        case GL_DST_COLOR: return D3DBLEND_DESTCOLOR;
        case GL_ONE_MINUS_DST_COLOR: return D3DBLEND_INVDESTCOLOR;
        case GL_SRC_COLOR: return D3DBLEND_SRCCOLOR;
        case GL_ONE_MINUS_SRC_COLOR: return D3DBLEND_INVSRCCOLOR;
        default: return D3DBLEND_ONE;
    }
}

static DWORD pex_convert_depth_func(GLenum f) {
    switch (f) {
        case GL_LEQUAL: return D3DCMP_LESSEQUAL;
        case GL_LESS: return D3DCMP_LESS;
        case GL_ALWAYS: return D3DCMP_ALWAYS;
        case GL_GEQUAL: return D3DCMP_GREATEREQUAL;
        case GL_GREATER: return D3DCMP_GREATER;
        case GL_EQUAL: return D3DCMP_EQUAL;
        default: return D3DCMP_LESSEQUAL;
    }
}

static void pex_batch_reserve(PexD3DBatch *b, int add) {
    if (b->count + add <= b->cap) return;
    int nc = b->cap ? b->cap * 2 : 64;
    while (nc < b->count + add) nc *= 2;
    b->v = (PexRawVertex*)realloc(b->v, sizeof(PexRawVertex) * (size_t)nc);
    b->cap = nc;
}

static int compat_d3d_count_vertices(int mode, int in_count) {
    if (in_count <= 0) return 0;
    if (mode == GL_TRIANGLES) return (in_count / 3) * 3;
    if (mode == GL_QUADS) return (in_count / 4) * 6;
    if (mode == GL_TRIANGLE_FAN) return (in_count >= 3) ? (in_count - 2) * 3 : 0;
    if (mode == GL_LINES) return in_count & ~1;
    if (mode == GL_LINE_STRIP) return (in_count >= 2) ? in_count : 0;
    return 0;
}

static D3DPRIMITIVETYPE compat_d3d_primitive_for_mode(int mode) {
    if (mode == GL_LINES) return D3DPT_LINELIST;
    if (mode == GL_LINE_STRIP) return D3DPT_LINESTRIP;
    return D3DPT_TRIANGLELIST;
}

static UINT compat_d3d_primitive_count(int mode, int out_count) {
    if (mode == GL_LINES) return (UINT)(out_count / 2);
    if (mode == GL_LINE_STRIP) return (out_count >= 2) ? (UINT)(out_count - 1) : 0;
    return (UINT)(out_count / 3);
}

static void compat_d3d_convert_batch(const PexD3DBatch *b, PexD3DVertex *out) {
    int oi = 0;
    if (b->mode == GL_TRIANGLES) {
        int n = (b->count / 3) * 3;
        for (int i = 0; i < n; i++) pex_copy_vertex(&b->v[i], &out[oi++]);
    } else if (b->mode == GL_QUADS) {
        int quads = b->count / 4;
        for (int q = 0; q < quads; q++) {
            PexD3DVertex a, bv, c, d;
            pex_copy_vertex(&b->v[q*4+0], &a);
            pex_copy_vertex(&b->v[q*4+1], &bv);
            pex_copy_vertex(&b->v[q*4+2], &c);
            pex_copy_vertex(&b->v[q*4+3], &d);
            out[oi++] = a;  out[oi++] = bv; out[oi++] = c;
            out[oi++] = a;  out[oi++] = c;  out[oi++] = d;
        }
    } else if (b->mode == GL_TRIANGLE_FAN) {
        int n = b->count;
        for (int i = 1; i + 1 < n; i++) {
            pex_copy_vertex(&b->v[0], &out[oi++]);
            pex_copy_vertex(&b->v[i], &out[oi++]);
            pex_copy_vertex(&b->v[i+1], &out[oi++]);
        }
    } else if (b->mode == GL_LINES || b->mode == GL_LINE_STRIP) {
        int n = compat_d3d_count_vertices(b->mode, b->count);
        for (int i = 0; i < n; i++) pex_copy_vertex(&b->v[i], &out[oi++]);
    }
}

static int compat_d3d11_create_static_vb(PexD3DBatch *b, const PexD3DVertex *vertices, int count);

static void compat_d3d_release_batch(PexD3DBatch *b) {
    if (!b) return;
    if (b->vb) { IDirect3DVertexBuffer9_Release(b->vb); b->vb = NULL; }
    if (b->vb11) { ID3D11Buffer_Release(b->vb11); b->vb11 = NULL; }
    free(b->sysmem); b->sysmem = NULL; b->sysmem_count = 0;
}

static int compat_d3d_create_static_vb(PexD3DBatch *b, const PexD3DVertex *vertices, int count) {
    if (!g_d3d9.dev || !b || !vertices || count <= 0) return 0;
    UINT bytes = (UINT)(sizeof(PexD3DVertex) * (size_t)count);
    if (FAILED(IDirect3DDevice9_CreateVertexBuffer(g_d3d9.dev, bytes, D3DUSAGE_WRITEONLY, PEX_D3D_FVF, D3DPOOL_MANAGED, &b->vb, NULL))) return 0;
    void *dst = NULL;
    if (FAILED(IDirect3DVertexBuffer9_Lock(b->vb, 0, bytes, &dst, 0))) {
        IDirect3DVertexBuffer9_Release(b->vb);
        b->vb = NULL;
        return 0;
    }
    memcpy(dst, vertices, bytes);
    IDirect3DVertexBuffer9_Unlock(b->vb);
    return 1;
}

static void pex_batch_finalize_static(PexD3DBatch *b) {
    if (!b) return;
    int outn = compat_d3d_count_vertices(b->mode, b->count);
    b->d3d_prim = compat_d3d_primitive_for_mode(b->mode);
    b->primitive_count = compat_d3d_primitive_count(b->mode, outn);
    b->vertex_count = (UINT)outn;
    if (outn <= 0 || b->primitive_count == 0) {
        free(b->v); b->v = NULL; b->count = b->cap = 0;
        return;
    }
    PexD3DVertex *converted = (PexD3DVertex*)malloc(sizeof(PexD3DVertex) * (size_t)outn);
    if (!converted) return;
    compat_d3d_convert_batch(b, converted);
    free(b->v); b->v = NULL; b->count = b->cap = 0;
    int ok = pex_using_d3d11() ? compat_d3d11_create_static_vb(b, converted, outn) : compat_d3d_create_static_vb(b, converted, outn);
    if (!ok) {
        b->sysmem = converted;
        b->sysmem_count = outn;
    } else {
        free(converted);
    }
}


static void pex_list_free(PexD3DList *l) {
    if (!l) return;
    for (int i=0;i<l->count;i++) { free(l->batches[i].v); compat_d3d_release_batch(&l->batches[i]); }
    free(l->batches);
    free(l);
}

static PexD3DList *pex_list_ensure(unsigned int id) {
    if (id >= PEX_D3D_MAX_LISTS) return NULL;
    if (!g_d3d9.lists[id]) g_d3d9.lists[id] = (PexD3DList*)calloc(1, sizeof(PexD3DList));
    return g_d3d9.lists[id];
}

static PexD3DBatch *pex_list_add_batch(PexD3DList *l, int mode, const PexD3DStateSnap *s) {
    if (!l) return NULL;
    if (l->count + 1 > l->cap) {
        int nc = l->cap ? l->cap * 2 : 8;
        l->batches = (PexD3DBatch*)realloc(l->batches, sizeof(PexD3DBatch)*(size_t)nc);
        memset(l->batches + l->cap, 0, sizeof(PexD3DBatch)*(size_t)(nc-l->cap));
        l->cap = nc;
    }
    PexD3DBatch *b = &l->batches[l->count++];
    memset(b, 0, sizeof(*b));
    b->mode = mode;
    b->state = *s;
    return b;
}

static void pex_capture_state(PexD3DStateSnap *s) {
    s->texture_enabled = g_d3d9.texture_enabled;
    s->texture_id = g_d3d9.bound_texture;
    s->blend_enabled = g_d3d9.blend_enabled;
    s->src_blend = g_d3d9.src_blend;
    s->dst_blend = g_d3d9.dst_blend;
    s->alpha_test_enabled = g_d3d9.alpha_test_enabled;
    s->depth_enabled = g_d3d9.depth_enabled;
    s->depth_write = g_d3d9.depth_write;
    s->depth_func = g_d3d9.depth_func;
    s->fog_enabled = g_d3d9.fog_enabled;
    s->fog_start = g_d3d9.fog_start;
    s->fog_end = g_d3d9.fog_end;
    s->fog_color = g_d3d9.fog_color;
    s->cull_enabled = g_d3d9.cull_enabled;
    s->color_write_mask = g_d3d9.color_write_mask;
}

static void pex_d3d_apply_state(const PexD3DStateSnap *s) {
    if (!g_d3d9.dev) return;
    compat_d3d_set_render_state(D3DRS_ZENABLE, s->depth_enabled ? D3DZB_TRUE : D3DZB_FALSE);
    compat_d3d_set_render_state(D3DRS_ZWRITEENABLE, s->depth_write ? TRUE : FALSE);
    compat_d3d_set_render_state(D3DRS_ZFUNC, s->depth_func);
    compat_d3d_set_render_state(D3DRS_ALPHABLENDENABLE, s->blend_enabled ? TRUE : FALSE);
    compat_d3d_set_render_state(D3DRS_SRCBLEND, s->src_blend);
    compat_d3d_set_render_state(D3DRS_DESTBLEND, s->dst_blend);
    compat_d3d_set_render_state(D3DRS_ALPHATESTENABLE, s->alpha_test_enabled ? TRUE : FALSE);
    compat_d3d_set_render_state(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    compat_d3d_set_render_state(D3DRS_ALPHAREF, 25);
    compat_d3d_set_render_state(D3DRS_CULLMODE, D3DCULL_NONE);
    compat_d3d_set_render_state(D3DRS_COLORWRITEENABLE, s->color_write_mask);
    compat_d3d_set_render_state(D3DRS_FOGENABLE, s->fog_enabled ? TRUE : FALSE);
    compat_d3d_set_render_state(D3DRS_FOGCOLOR, s->fog_color);
    compat_d3d_set_render_state(D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
    compat_d3d_set_render_state(D3DRS_FOGSTART, pex_float_as_dword(s->fog_start));
    compat_d3d_set_render_state(D3DRS_FOGEND, pex_float_as_dword(s->fog_end));
    if (s->texture_enabled && s->texture_id < PEX_D3D_MAX_TEXTURES && g_d3d9.textures[s->texture_id].tex) {
        pex_d3d_set_texture0((IDirect3DBaseTexture9*)g_d3d9.textures[s->texture_id].tex);
        compat_d3d_set_texture_stage(D3DTSS_COLOROP, D3DTOP_MODULATE);
        compat_d3d_set_texture_stage(D3DTSS_COLORARG1, D3DTA_TEXTURE);
        compat_d3d_set_texture_stage(D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        compat_d3d_set_texture_stage(D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        compat_d3d_set_texture_stage(D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        compat_d3d_set_texture_stage(D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        compat_d3d_set_sampler_state(D3DSAMP_ADDRESSU, g_d3d9.textures[s->texture_id].address_u);
        compat_d3d_set_sampler_state(D3DSAMP_ADDRESSV, g_d3d9.textures[s->texture_id].address_v);
        compat_d3d_set_sampler_state(D3DSAMP_MINFILTER, g_d3d9.textures[s->texture_id].min_filter ? g_d3d9.textures[s->texture_id].min_filter : D3DTEXF_POINT);
        compat_d3d_set_sampler_state(D3DSAMP_MAGFILTER, g_d3d9.textures[s->texture_id].mag_filter ? g_d3d9.textures[s->texture_id].mag_filter : D3DTEXF_POINT);
        compat_d3d_set_sampler_state(D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    } else {
        pex_d3d_set_texture0(NULL);
        compat_d3d_set_texture_stage(D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        compat_d3d_set_texture_stage(D3DTSS_COLORARG1, D3DTA_DIFFUSE);
        compat_d3d_set_texture_stage(D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        compat_d3d_set_texture_stage(D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    }
}

static int pex_scratch_reserve(int n) {
    if (n <= g_d3d9.scratch_cap) return 1;
    int nc = g_d3d9.scratch_cap ? g_d3d9.scratch_cap * 2 : 4096;
    while (nc < n) nc *= 2;
    PexD3DVertex *nv = (PexD3DVertex*)realloc(g_d3d9.scratch, sizeof(PexD3DVertex)*(size_t)nc);
    if (!nv) return 0;
    g_d3d9.scratch = nv;
    g_d3d9.scratch_cap = nc;
    return 1;
}

static int compat_d3d_ensure_dynamic_vb(UINT vertex_count) {
    if (!g_d3d9.dev || vertex_count == 0) return 0;
    if (g_d3d9.dynamic_vb && g_d3d9.dynamic_vb_capacity >= vertex_count) return 1;
    if (g_d3d9.dynamic_vb) {
        if (g_d3d9.cache.stream0 == g_d3d9.dynamic_vb) g_d3d9.cache.stream_valid = 0;
        IDirect3DVertexBuffer9_Release(g_d3d9.dynamic_vb);
        g_d3d9.dynamic_vb = NULL;
    }
    UINT cap = g_d3d9.dynamic_vb_capacity ? g_d3d9.dynamic_vb_capacity * 2 : 8192;
    while (cap < vertex_count) cap *= 2;
    UINT bytes = cap * (UINT)sizeof(PexD3DVertex);
    if (FAILED(IDirect3DDevice9_CreateVertexBuffer(g_d3d9.dev, bytes, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, PEX_D3D_FVF, D3DPOOL_DEFAULT, &g_d3d9.dynamic_vb, NULL))) return 0;
    g_d3d9.dynamic_vb_capacity = cap;
    return 1;
}

static void compat_d3d_draw_dynamic(D3DPRIMITIVETYPE prim, UINT primitive_count, const PexD3DVertex *vertices, UINT vertex_count) {
    if (!g_d3d9.dev || !vertices || vertex_count == 0 || primitive_count == 0) return;
    if (!compat_d3d_ensure_dynamic_vb(vertex_count)) return;
    void *dst = NULL;
    UINT bytes = vertex_count * (UINT)sizeof(PexD3DVertex);
    if (FAILED(IDirect3DVertexBuffer9_Lock(g_d3d9.dynamic_vb, 0, bytes, &dst, D3DLOCK_DISCARD))) return;
    memcpy(dst, vertices, bytes);
    IDirect3DVertexBuffer9_Unlock(g_d3d9.dynamic_vb);
    pex_d3d_set_fvf(PEX_D3D_FVF);
    pex_d3d_set_stream0(g_d3d9.dynamic_vb, sizeof(PexD3DVertex));
    IDirect3DDevice9_DrawPrimitive(g_d3d9.dev, prim, 0, primitive_count);
}

static void compat_d3d_draw_batch(const PexD3DBatch *b) {
    if (!g_d3d9.dev || !b || b->primitive_count == 0 || b->vertex_count == 0) return;
    pex_d3d_apply_state(&b->state);
    pex_d3d_apply_matrices();
    pex_d3d_set_fvf(PEX_D3D_FVF);
    if (b->vb) {
        pex_d3d_set_stream0(b->vb, sizeof(PexD3DVertex));
        IDirect3DDevice9_DrawPrimitive(g_d3d9.dev, b->d3d_prim, 0, b->primitive_count);
    } else if (b->sysmem && b->sysmem_count > 0) {
        compat_d3d_draw_dynamic(b->d3d_prim, b->primitive_count, b->sysmem, b->vertex_count);
    }
}

static void pex_d3d_flush_batch(const PexD3DBatch *b) {
    if (!g_d3d9.dev || !b || b->count <= 0) return;
    int outn = compat_d3d_count_vertices(b->mode, b->count);
    if (outn <= 0 || !pex_scratch_reserve(outn)) return;
    compat_d3d_convert_batch(b, g_d3d9.scratch);
    D3DPRIMITIVETYPE prim = compat_d3d_primitive_for_mode(b->mode);
    UINT prim_count = compat_d3d_primitive_count(b->mode, outn);
    if (!prim_count) return;
    pex_d3d_apply_state(&b->state);
    pex_d3d_apply_matrices();
    compat_d3d_draw_dynamic(prim, prim_count, g_d3d9.scratch, (UINT)outn);
}

static void compat_d3d_set_initial_states(void) {
    if (!g_d3d9.dev) return;
    pex_d3d_invalidate_cache();
    g_d3d9.cache.valid = 1;
    compat_d3d_set_render_state(D3DRS_LIGHTING, FALSE);
    compat_d3d_set_render_state(D3DRS_DITHERENABLE, FALSE);
    compat_d3d_set_render_state(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    compat_d3d_set_render_state(D3DRS_ZENABLE, D3DZB_TRUE);
    compat_d3d_set_render_state(D3DRS_ZWRITEENABLE, TRUE);
    compat_d3d_set_render_state(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    compat_d3d_set_render_state(D3DRS_CULLMODE, D3DCULL_NONE);
    compat_d3d_set_render_state(D3DRS_CLIPPING, TRUE);
    compat_d3d_set_render_state(D3DRS_SPECULARENABLE, FALSE);
    compat_d3d_set_sampler_state(D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    compat_d3d_set_sampler_state(D3DSAMP_MINFILTER, D3DTEXF_POINT);
    compat_d3d_set_sampler_state(D3DSAMP_MIPFILTER, D3DTEXF_NONE);
}

static int pex_d3d9_choose_depth(IDirect3D9 *d3d, D3DFORMAT adapter_fmt, D3DFORMAT *out) {
    D3DFORMAT tries[2] = { D3DFMT_D24S8, D3DFMT_D16 };
    for (int i=0;i<2;i++) {
        if (SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, adapter_fmt,
                                                   D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, tries[i])) &&
            SUCCEEDED(IDirect3D9_CheckDepthStencilMatch(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, adapter_fmt, adapter_fmt, tries[i]))) {
            *out = tries[i]; return 1;
        }
    }
    return 0;
}

static int pex_renderer_d3d9_init(HWND hwnd) {
    memset(&g_d3d9, 0, sizeof(g_d3d9));
    g_d3d9.hwnd = hwnd;
    g_d3d9.next_texture = 1;
    g_d3d9.next_list = 1;
    g_d3d9.texture_enabled = 1;
    g_d3d9.blend_enabled = 1;
    g_d3d9.src_blend = D3DBLEND_SRCALPHA;
    g_d3d9.dst_blend = D3DBLEND_INVSRCALPHA;
    g_d3d9.color_write_mask = D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA;
    g_d3d9.depth_enabled = 0;
    g_d3d9.depth_write = 1;
    g_d3d9.depth_func = D3DCMP_LESSEQUAL;
    g_d3d9.color[0]=g_d3d9.color[1]=g_d3d9.color[2]=g_d3d9.color[3]=1.0f;
    g_d3d9.clear_color = D3DCOLOR_XRGB(0, 0, 0);
    g_d3d9.matrix_mode = GL_MODELVIEW;
    pex_mat_identity(&g_d3d9.modelview);
    pex_mat_identity(&g_d3d9.projection);
    g_d3d9.matrix_dirty = 1;

    g_d3d9.d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_d3d9.d3d) return 0;
    D3DDISPLAYMODE dm;
    if (FAILED(IDirect3D9_GetAdapterDisplayMode(g_d3d9.d3d, D3DADAPTER_DEFAULT, &dm))) { IDirect3D9_Release(g_d3d9.d3d); g_d3d9.d3d = NULL; return 0; }
    if (!pex_d3d9_choose_depth(g_d3d9.d3d, dm.Format, &g_d3d9.depth_format)) { IDirect3D9_Release(g_d3d9.d3d); g_d3d9.d3d = NULL; return 0; }
    D3DCAPS9 caps;
    if (FAILED(IDirect3D9_GetDeviceCaps(g_d3d9.d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps))) { IDirect3D9_Release(g_d3d9.d3d); g_d3d9.d3d = NULL; return 0; }
    DWORD vp = (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) ? D3DCREATE_HARDWARE_VERTEXPROCESSING : D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    vp |= D3DCREATE_FPU_PRESERVE;
    RECT rc; GetClientRect(hwnd, &rc);
    g_d3d9.width = rc.right - rc.left; if (g_d3d9.width < 1) g_d3d9.width = 1;
    g_d3d9.height = rc.bottom - rc.top; if (g_d3d9.height < 1) g_d3d9.height = 1;
    memset(&g_d3d9.pp, 0, sizeof(g_d3d9.pp));
    g_d3d9.pp.Windowed = TRUE;
    g_d3d9.pp.hDeviceWindow = hwnd;
    g_d3d9.pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3d9.pp.BackBufferFormat = dm.Format;
    g_d3d9.pp.BackBufferWidth = g_d3d9.width;
    g_d3d9.pp.BackBufferHeight = g_d3d9.height;
    g_d3d9.pp.EnableAutoDepthStencil = TRUE;
    g_d3d9.pp.AutoDepthStencilFormat = g_d3d9.depth_format;
    g_d3d9.pp.PresentationInterval = (g_opts.anaglyph && g_opts.max_fps > 0) ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
    HRESULT hr = IDirect3D9_CreateDevice(g_d3d9.d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, vp, &g_d3d9.pp, &g_d3d9.dev);
    if (FAILED(hr)) {
        hr = IDirect3D9_CreateDevice(g_d3d9.d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &g_d3d9.pp, &g_d3d9.dev);
    }
    if (FAILED(hr) || !g_d3d9.dev) { if (g_d3d9.d3d) { IDirect3D9_Release(g_d3d9.d3d); g_d3d9.d3d = NULL; } return 0; }
    g_d3d9.active = 1;
    g_d3d9.viewport_x = 0; g_d3d9.viewport_y = 0; g_d3d9.viewport_w = g_d3d9.width; g_d3d9.viewport_h = g_d3d9.height;
    compat_d3d_set_initial_states();
    return 1;
}

static void pex_renderer_d3d9_shutdown(void) {
    if (g_d3d9.scene_open && g_d3d9.dev) { IDirect3DDevice9_EndScene(g_d3d9.dev); g_d3d9.scene_open = 0; }
    for (int i=0;i<PEX_D3D_MAX_TEXTURES;i++) if (g_d3d9.textures[i].tex) { IDirect3DTexture9_Release(g_d3d9.textures[i].tex); g_d3d9.textures[i].tex = NULL; }
    for (int i=0;i<PEX_D3D_MAX_LISTS;i++) if (g_d3d9.lists[i]) { pex_list_free(g_d3d9.lists[i]); g_d3d9.lists[i]=NULL; }
    free(g_d3d9.immediate.v); g_d3d9.immediate.v = NULL; g_d3d9.immediate.count = g_d3d9.immediate.cap = 0;
    free(g_d3d9.scratch); g_d3d9.scratch=NULL; g_d3d9.scratch_cap=0;
    if (g_d3d9.dynamic_vb) { IDirect3DVertexBuffer9_Release(g_d3d9.dynamic_vb); g_d3d9.dynamic_vb = NULL; g_d3d9.dynamic_vb_capacity = 0; }
    if (g_d3d9.dev) { IDirect3DDevice9_Release(g_d3d9.dev); g_d3d9.dev = NULL; }
    if (g_d3d9.d3d) { IDirect3D9_Release(g_d3d9.d3d); g_d3d9.d3d = NULL; }
    g_d3d9.active = 0;
}

static void pex_renderer_d3d9_resize(int w, int h) {
    if (!pex_using_d3d9() || !g_d3d9.dev) return;
    if (w < 1) w = 1; if (h < 1) h = 1;
    g_d3d9.width = w; g_d3d9.height = h;
    if (g_d3d9.scene_open) { IDirect3DDevice9_EndScene(g_d3d9.dev); g_d3d9.scene_open = 0; }
    if (g_d3d9.dynamic_vb) { IDirect3DVertexBuffer9_Release(g_d3d9.dynamic_vb); g_d3d9.dynamic_vb = NULL; g_d3d9.dynamic_vb_capacity = 0; }
    pex_d3d_invalidate_cache();
    g_d3d9.pp.BackBufferWidth = w;
    g_d3d9.pp.BackBufferHeight = h;
    g_d3d9.pp.PresentationInterval = (g_opts.anaglyph && g_opts.max_fps > 0) ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
    HRESULT hr = IDirect3DDevice9_Reset(g_d3d9.dev, &g_d3d9.pp);
    if (SUCCEEDED(hr)) { compat_d3d_set_initial_states(); g_d3d9.lost = 0; }
    else g_d3d9.lost = 1;
}

static int compat_d3d9_begin_frame(void) {
    if (!pex_using_d3d9() || !g_d3d9.dev) return 0;
    HRESULT coop = IDirect3DDevice9_TestCooperativeLevel(g_d3d9.dev);
    if (coop == D3DERR_DEVICELOST) { g_d3d9.lost = 1; return 0; }
    if (coop == D3DERR_DEVICENOTRESET) { pex_renderer_d3d9_resize(g_d3d9.width, g_d3d9.height); if (g_d3d9.lost) return 0; }
    if (FAILED(IDirect3DDevice9_BeginScene(g_d3d9.dev))) return 0;
    g_d3d9.scene_open = 1;
    return 1;
}

static void pex_renderer_d3d9_present(void) {
    if (!pex_using_d3d9() || !g_d3d9.dev) return;
    pex_gpu_flush_immediate_stream();
    if (g_d3d9.scene_open) { IDirect3DDevice9_EndScene(g_d3d9.dev); g_d3d9.scene_open = 0; }
    HRESULT hr = IDirect3DDevice9_Present(g_d3d9.dev, NULL, NULL, NULL, NULL);
    if (hr == D3DERR_DEVICELOST) g_d3d9.lost = 1;
}



static void pex_d3d11_log_hresult(const char *stage, HRESULT hr) {
    char line[512];
    snprintf(line, sizeof(line), "D3D11 failure: %s hr=0x%08lX", stage ? stage : "unknown", (unsigned long)hr);
    pex_logf("%s", line);
#if defined(_WIN32) || defined(PEX_PLATFORM_XBOX_UWP)
    OutputDebugStringA("[PexCraft UWP] ");
    OutputDebugStringA(line);
    OutputDebugStringA("\n");
#endif
}

static void pex_d3d11_release_views(void) {
    if (g_d3d11.rtv) { ID3D11RenderTargetView_Release(g_d3d11.rtv); g_d3d11.rtv = NULL; }
    if (g_d3d11.dsv) { ID3D11DepthStencilView_Release(g_d3d11.dsv); g_d3d11.dsv = NULL; }
    if (g_d3d11.depth_tex) { ID3D11Texture2D_Release(g_d3d11.depth_tex); g_d3d11.depth_tex = NULL; }
}

static int pex_d3d11_create_views(int w, int h) {
    if (!g_d3d11.dev || !g_d3d11.swap) { pex_logf("D3D11 failure: create views called without device/swap chain"); return 0; }
    if (w < 1) w = 1; if (h < 1) h = 1;
    ID3D11Texture2D *back = NULL;
    HRESULT hr = IDXGISwapChain_GetBuffer(g_d3d11.swap, 0, &IID_ID3D11Texture2D, (void**)&back);
    if (FAILED(hr) || !back) { pex_d3d11_log_hresult("IDXGISwapChain::GetBuffer", hr); return 0; }
    hr = ID3D11Device_CreateRenderTargetView(g_d3d11.dev, (ID3D11Resource*)back, NULL, &g_d3d11.rtv);
    ID3D11Texture2D_Release(back);
    if (FAILED(hr)) { pex_d3d11_log_hresult("ID3D11Device::CreateRenderTargetView", hr); return 0; }
    D3D11_TEXTURE2D_DESC dd;
    memset(&dd, 0, sizeof(dd));
    dd.Width = (UINT)w;
    dd.Height = (UINT)h;
    dd.MipLevels = 1;
    dd.ArraySize = 1;
    dd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dd.SampleDesc.Count = 1;
    dd.Usage = D3D11_USAGE_DEFAULT;
    dd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    hr = ID3D11Device_CreateTexture2D(g_d3d11.dev, &dd, NULL, &g_d3d11.depth_tex);
    if (FAILED(hr)) { pex_d3d11_log_hresult("ID3D11Device::CreateTexture2D(depth)", hr); return 0; }
    hr = ID3D11Device_CreateDepthStencilView(g_d3d11.dev, (ID3D11Resource*)g_d3d11.depth_tex, NULL, &g_d3d11.dsv);
    if (FAILED(hr)) { pex_d3d11_log_hresult("ID3D11Device::CreateDepthStencilView", hr); return 0; }
    ID3D11DeviceContext_OMSetRenderTargets(g_d3d11.ctx, 1, &g_d3d11.rtv, g_d3d11.dsv);
    D3D11_VIEWPORT vp;
    memset(&vp, 0, sizeof(vp));
    vp.TopLeftX = 0.0f; vp.TopLeftY = 0.0f;
    vp.Width = (FLOAT)w; vp.Height = (FLOAT)h;
    vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;
    ID3D11DeviceContext_RSSetViewports(g_d3d11.ctx, 1, &vp);
    return 1;
}

static void pex_d3d11_release_pipeline(void) {
    if (g_d3d11.dynamic_vb) { ID3D11Buffer_Release(g_d3d11.dynamic_vb); g_d3d11.dynamic_vb = NULL; g_d3d11.dynamic_vb_capacity = 0; }
    if (g_d3d11.cbuffer) { ID3D11Buffer_Release(g_d3d11.cbuffer); g_d3d11.cbuffer = NULL; }
    if (g_d3d11.layout) { ID3D11InputLayout_Release(g_d3d11.layout); g_d3d11.layout = NULL; }
    if (g_d3d11.vs) { ID3D11VertexShader_Release(g_d3d11.vs); g_d3d11.vs = NULL; }
    if (g_d3d11.ps) { ID3D11PixelShader_Release(g_d3d11.ps); g_d3d11.ps = NULL; }
    if (g_d3d11.sampler_wrap) { ID3D11SamplerState_Release(g_d3d11.sampler_wrap); g_d3d11.sampler_wrap = NULL; }
    if (g_d3d11.sampler_clamp) { ID3D11SamplerState_Release(g_d3d11.sampler_clamp); g_d3d11.sampler_clamp = NULL; }
    if (g_d3d11.sampler_wrap_linear) { ID3D11SamplerState_Release(g_d3d11.sampler_wrap_linear); g_d3d11.sampler_wrap_linear = NULL; }
    if (g_d3d11.sampler_clamp_linear) { ID3D11SamplerState_Release(g_d3d11.sampler_clamp_linear); g_d3d11.sampler_clamp_linear = NULL; }
    if (g_d3d11.blend_off) { ID3D11BlendState_Release(g_d3d11.blend_off); g_d3d11.blend_off = NULL; }
    if (g_d3d11.blend_alpha) { ID3D11BlendState_Release(g_d3d11.blend_alpha); g_d3d11.blend_alpha = NULL; }
    if (g_d3d11.blend_additive) { ID3D11BlendState_Release(g_d3d11.blend_additive); g_d3d11.blend_additive = NULL; }
    if (g_d3d11.blend_multiply) { ID3D11BlendState_Release(g_d3d11.blend_multiply); g_d3d11.blend_multiply = NULL; }
    if (g_d3d11.blend_color_off) { ID3D11BlendState_Release(g_d3d11.blend_color_off); g_d3d11.blend_color_off = NULL; }
    if (g_d3d11.depth_on_write) { ID3D11DepthStencilState_Release(g_d3d11.depth_on_write); g_d3d11.depth_on_write = NULL; }
    if (g_d3d11.depth_on_nowrite) { ID3D11DepthStencilState_Release(g_d3d11.depth_on_nowrite); g_d3d11.depth_on_nowrite = NULL; }
    if (g_d3d11.depth_off) { ID3D11DepthStencilState_Release(g_d3d11.depth_off); g_d3d11.depth_off = NULL; }
    if (g_d3d11.rast_nocull) { ID3D11RasterizerState_Release(g_d3d11.rast_nocull); g_d3d11.rast_nocull = NULL; }
}

static int pex_d3d11_compile_shader(const char *src, const char *entry, const char *target, ID3DBlob **out_blob) {
    ID3DBlob *err = NULL;
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
    HRESULT hr = D3DCompile(src, strlen(src), NULL, NULL, NULL, entry, target, flags, 0, out_blob, &err);
    if (FAILED(hr) || !out_blob || !*out_blob) {
        pex_d3d11_log_hresult(target ? target : "D3DCompile", hr);
        if (err && ID3DBlob_GetBufferPointer(err)) {
            size_t n = ID3DBlob_GetBufferSize(err);
            if (n > 900) n = 900;
            char compiler_msg[901];
            memcpy(compiler_msg, ID3DBlob_GetBufferPointer(err), n);
            compiler_msg[n] = 0;
            pex_logf("D3D shader compiler: %s", compiler_msg);
            OutputDebugStringA(compiler_msg);
        }
    }
    if (err) ID3DBlob_Release(err);
    return SUCCEEDED(hr) && out_blob && *out_blob;
}

static int pex_d3d11_create_pipeline(void) {
    static const char *vs_src =
        "cbuffer CB0 : register(b0) { row_major float4x4 model; row_major float4x4 proj; float4 flags; float4 fogColor; float4 fogParams; };"
        "struct VSIn { float3 pos:POSITION; float4 color:COLOR0; float2 uv:TEXCOORD0; };"
        "struct VSOut { float4 pos:SV_POSITION; float4 color:COLOR0; float2 uv:TEXCOORD0; float fog:TEXCOORD1; };"
        "VSOut main(VSIn i){ VSOut o; float4 w=mul(float4(i.pos,1.0), model); o.pos=mul(w, proj); o.color=i.color; o.uv=i.uv; float d=abs(w.z); o.fog=saturate((d-fogParams.x)/max(0.001,fogParams.y-fogParams.x)); return o; }";
    static const char *ps_src =
        "Texture2D tex0 : register(t0); SamplerState samp0 : register(s0);"
        "cbuffer CB0 : register(b0) { row_major float4x4 model; row_major float4x4 proj; float4 flags; float4 fogColor; float4 fogParams; };"
        "struct VSOut { float4 pos:SV_POSITION; float4 color:COLOR0; float2 uv:TEXCOORD0; float fog:TEXCOORD1; };"
        "float4 main(VSOut i) : SV_Target { float4 t = (flags.y > 0.5) ? tex0.Sample(samp0, i.uv) : float4(1,1,1,1); float4 c=t*i.color; if(flags.x > 0.5 && c.a < 0.10) discard; if(flags.z > 0.5) c.rgb=lerp(c.rgb, fogColor.rgb, i.fog); return c; }";
    ID3DBlob *vsb = NULL, *psb = NULL;
    if (!pex_d3d11_compile_shader(vs_src, "main", "vs_4_0", &vsb)) return 0;
    if (!pex_d3d11_compile_shader(ps_src, "main", "ps_4_0", &psb)) { ID3DBlob_Release(vsb); return 0; }
    HRESULT create_hr = ID3D11Device_CreateVertexShader(g_d3d11.dev, ID3DBlob_GetBufferPointer(vsb), ID3DBlob_GetBufferSize(vsb), NULL, &g_d3d11.vs);
    if (FAILED(create_hr)) { pex_d3d11_log_hresult("ID3D11Device::CreateVertexShader", create_hr); ID3DBlob_Release(vsb); ID3DBlob_Release(psb); return 0; }
    create_hr = ID3D11Device_CreatePixelShader(g_d3d11.dev, ID3DBlob_GetBufferPointer(psb), ID3DBlob_GetBufferSize(psb), NULL, &g_d3d11.ps);
    if (FAILED(create_hr)) { pex_d3d11_log_hresult("ID3D11Device::CreatePixelShader", create_hr); ID3DBlob_Release(vsb); ID3DBlob_Release(psb); return 0; }
    D3D11_INPUT_ELEMENT_DESC il[3];
    memset(il, 0, sizeof(il));
    il[0].SemanticName="POSITION"; il[0].Format=DXGI_FORMAT_R32G32B32_FLOAT; il[0].InputSlotClass=D3D11_INPUT_PER_VERTEX_DATA; il[0].AlignedByteOffset=0;
    il[1].SemanticName="COLOR"; il[1].Format=DXGI_FORMAT_B8G8R8A8_UNORM; il[1].InputSlotClass=D3D11_INPUT_PER_VERTEX_DATA; il[1].AlignedByteOffset=12;
    il[2].SemanticName="TEXCOORD"; il[2].Format=DXGI_FORMAT_R32G32_FLOAT; il[2].InputSlotClass=D3D11_INPUT_PER_VERTEX_DATA; il[2].AlignedByteOffset=16;
    create_hr = ID3D11Device_CreateInputLayout(g_d3d11.dev, il, 3, ID3DBlob_GetBufferPointer(vsb), ID3DBlob_GetBufferSize(vsb), &g_d3d11.layout);
    if (FAILED(create_hr)) { pex_d3d11_log_hresult("ID3D11Device::CreateInputLayout", create_hr); ID3DBlob_Release(vsb); ID3DBlob_Release(psb); return 0; }
    ID3DBlob_Release(vsb); ID3DBlob_Release(psb);
    D3D11_BUFFER_DESC cbd; memset(&cbd, 0, sizeof(cbd)); cbd.ByteWidth=(UINT)((sizeof(PexD3D11CB)+15)&~15); cbd.Usage=D3D11_USAGE_DYNAMIC; cbd.BindFlags=D3D11_BIND_CONSTANT_BUFFER; cbd.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
    create_hr = ID3D11Device_CreateBuffer(g_d3d11.dev, &cbd, NULL, &g_d3d11.cbuffer);
    if (FAILED(create_hr)) { pex_d3d11_log_hresult("ID3D11Device::CreateBuffer(constants)", create_hr); return 0; }
    D3D11_SAMPLER_DESC sd; memset(&sd,0,sizeof(sd)); sd.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT; sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_WRAP; sd.MaxLOD=D3D11_FLOAT32_MAX;
    ID3D11Device_CreateSamplerState(g_d3d11.dev, &sd, &g_d3d11.sampler_wrap);
    sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
    ID3D11Device_CreateSamplerState(g_d3d11.dev, &sd, &g_d3d11.sampler_clamp);
    sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR; sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;
    ID3D11Device_CreateSamplerState(g_d3d11.dev, &sd, &g_d3d11.sampler_wrap_linear);
    sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
    ID3D11Device_CreateSamplerState(g_d3d11.dev, &sd, &g_d3d11.sampler_clamp_linear);
    D3D11_BLEND_DESC bd; memset(&bd,0,sizeof(bd)); bd.RenderTarget[0].RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;
    ID3D11Device_CreateBlendState(g_d3d11.dev, &bd, &g_d3d11.blend_off);
    D3D11_BLEND_DESC bd_mask = bd; bd_mask.RenderTarget[0].RenderTargetWriteMask = 0;
    ID3D11Device_CreateBlendState(g_d3d11.dev, &bd_mask, &g_d3d11.blend_color_off);
    bd.RenderTarget[0].BlendEnable=TRUE; bd.RenderTarget[0].SrcBlend=D3D11_BLEND_SRC_ALPHA; bd.RenderTarget[0].DestBlend=D3D11_BLEND_INV_SRC_ALPHA; bd.RenderTarget[0].BlendOp=D3D11_BLEND_OP_ADD; bd.RenderTarget[0].SrcBlendAlpha=D3D11_BLEND_ONE; bd.RenderTarget[0].DestBlendAlpha=D3D11_BLEND_INV_SRC_ALPHA; bd.RenderTarget[0].BlendOpAlpha=D3D11_BLEND_OP_ADD;
    ID3D11Device_CreateBlendState(g_d3d11.dev, &bd, &g_d3d11.blend_alpha);
    bd.RenderTarget[0].SrcBlend=D3D11_BLEND_SRC_ALPHA; bd.RenderTarget[0].DestBlend=D3D11_BLEND_ONE; bd.RenderTarget[0].BlendOp=D3D11_BLEND_OP_ADD; bd.RenderTarget[0].SrcBlendAlpha=D3D11_BLEND_ONE; bd.RenderTarget[0].DestBlendAlpha=D3D11_BLEND_ONE; bd.RenderTarget[0].BlendOpAlpha=D3D11_BLEND_OP_ADD;
    ID3D11Device_CreateBlendState(g_d3d11.dev, &bd, &g_d3d11.blend_additive);
    bd.RenderTarget[0].SrcBlend=D3D11_BLEND_DEST_COLOR; bd.RenderTarget[0].DestBlend=D3D11_BLEND_SRC_COLOR; bd.RenderTarget[0].BlendOp=D3D11_BLEND_OP_ADD; bd.RenderTarget[0].SrcBlendAlpha=D3D11_BLEND_ONE; bd.RenderTarget[0].DestBlendAlpha=D3D11_BLEND_ZERO; bd.RenderTarget[0].BlendOpAlpha=D3D11_BLEND_OP_ADD;
    ID3D11Device_CreateBlendState(g_d3d11.dev, &bd, &g_d3d11.blend_multiply);
    D3D11_DEPTH_STENCIL_DESC dsd; memset(&dsd,0,sizeof(dsd)); dsd.DepthEnable=TRUE; dsd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL; dsd.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;
    ID3D11Device_CreateDepthStencilState(g_d3d11.dev, &dsd, &g_d3d11.depth_on_write);
    dsd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO; ID3D11Device_CreateDepthStencilState(g_d3d11.dev, &dsd, &g_d3d11.depth_on_nowrite);
    dsd.DepthEnable=FALSE; ID3D11Device_CreateDepthStencilState(g_d3d11.dev, &dsd, &g_d3d11.depth_off);
    D3D11_RASTERIZER_DESC rd; memset(&rd,0,sizeof(rd)); rd.FillMode=D3D11_FILL_SOLID; rd.CullMode=D3D11_CULL_NONE; rd.DepthClipEnable=TRUE;
    ID3D11Device_CreateRasterizerState(g_d3d11.dev, &rd, &g_d3d11.rast_nocull);
    return g_d3d11.sampler_wrap && g_d3d11.sampler_clamp && g_d3d11.blend_off && g_d3d11.blend_alpha && g_d3d11.blend_additive && g_d3d11.blend_multiply && g_d3d11.blend_color_off && g_d3d11.depth_on_write && g_d3d11.depth_on_nowrite && g_d3d11.depth_off && g_d3d11.rast_nocull;
}

static int pex_renderer_d3d11_init(HWND hwnd) {
    memset(&g_d3d9, 0, sizeof(g_d3d9));
    memset(&g_d3d11, 0, sizeof(g_d3d11));
    g_d3d9.next_texture = 1; g_d3d9.next_list = 1;
    g_d3d9.texture_enabled = 1; g_d3d9.blend_enabled = 1;
    g_d3d9.src_blend = D3DBLEND_SRCALPHA; g_d3d9.dst_blend = D3DBLEND_INVSRCALPHA;
    g_d3d9.color_write_mask = D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA;
    g_d3d9.depth_enabled = 0; g_d3d9.depth_write = 1; g_d3d9.depth_func = D3DCMP_LESSEQUAL;
    g_d3d9.color[0]=g_d3d9.color[1]=g_d3d9.color[2]=g_d3d9.color[3]=1.0f;
    g_d3d9.clear_color = D3DCOLOR_XRGB(0, 0, 0);
    g_d3d9.matrix_mode = GL_MODELVIEW; pex_mat_identity(&g_d3d9.modelview); pex_mat_identity(&g_d3d9.projection); g_d3d9.matrix_dirty = 1;
    RECT rc; GetClientRect(hwnd, &rc);
    int w = rc.right - rc.left; int h = rc.bottom - rc.top; if (w < 1) w = 1; if (h < 1) h = 1;
    g_d3d11.hwnd = hwnd; g_d3d11.width = w; g_d3d11.height = h;
    DXGI_SWAP_CHAIN_DESC scd; memset(&scd,0,sizeof(scd));
    scd.BufferCount = 3; scd.BufferDesc.Width=(UINT)w; scd.BufferDesc.Height=(UINT)h; scd.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM; scd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT; scd.OutputWindow=hwnd; scd.SampleDesc.Count=1; scd.Windowed=TRUE; scd.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
    D3D_FEATURE_LEVEL fls[3] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL got;
    HRESULT hr = compat_d3d11_create_swap_chain(&scd, fls, (UINT)ARRAY_COUNT(fls), &got);
    if (FAILED(hr) || !g_d3d11.dev || !g_d3d11.ctx || !g_d3d11.swap) return 0;
    compat_d3d11_set_latency_for_unlimited_fps();
    if (!pex_d3d11_create_views(w,h)) return 0;
    if (!pex_d3d11_create_pipeline()) return 0;
    ID3D11DeviceContext_IASetInputLayout(g_d3d11.ctx, g_d3d11.layout);
    ID3D11DeviceContext_VSSetShader(g_d3d11.ctx, g_d3d11.vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(g_d3d11.ctx, g_d3d11.ps, NULL, 0);
    ID3D11DeviceContext_RSSetState(g_d3d11.ctx, g_d3d11.rast_nocull);
    g_d3d11.active = 1;
    return 1;
}

static void pex_renderer_d3d11_shutdown(void) {
    for (int i=0;i<PEX_D3D_MAX_TEXTURES;i++) {
        if (g_d3d9.textures[i].srv11) { ID3D11ShaderResourceView_Release(g_d3d9.textures[i].srv11); g_d3d9.textures[i].srv11 = NULL; }
        if (g_d3d9.textures[i].tex11) { ID3D11Texture2D_Release(g_d3d9.textures[i].tex11); g_d3d9.textures[i].tex11 = NULL; }
    }
    for (int i=0;i<PEX_D3D_MAX_LISTS;i++) if (g_d3d9.lists[i]) { pex_list_free(g_d3d9.lists[i]); g_d3d9.lists[i]=NULL; }
    free(g_d3d9.immediate.v); g_d3d9.immediate.v = NULL; g_d3d9.immediate.count = g_d3d9.immediate.cap = 0;
    free(g_d3d9.scratch); g_d3d9.scratch=NULL; g_d3d9.scratch_cap=0;
    pex_d3d11_release_pipeline();
    pex_d3d11_release_views();
    if (g_d3d11.ctx) { ID3D11DeviceContext_ClearState(g_d3d11.ctx); ID3D11DeviceContext_Release(g_d3d11.ctx); g_d3d11.ctx = NULL; }
    if (g_d3d11.swap) { IDXGISwapChain_Release(g_d3d11.swap); g_d3d11.swap = NULL; }
    if (g_d3d11.dev) { ID3D11Device_Release(g_d3d11.dev); g_d3d11.dev = NULL; }
    g_d3d11.active = 0;
}

static void pex_renderer_d3d11_resize(int w, int h) {
    if (!pex_using_d3d11() || !g_d3d11.swap) return;
    if (w < 1) w = 1; if (h < 1) h = 1;
    g_d3d11.width = w; g_d3d11.height = h;
    ID3D11DeviceContext_OMSetRenderTargets(g_d3d11.ctx, 0, NULL, NULL);
    pex_d3d11_release_views();
    IDXGISwapChain_ResizeBuffers(g_d3d11.swap, 0, (UINT)w, (UINT)h, DXGI_FORMAT_UNKNOWN, g_d3d11.swap_flags);
    pex_d3d11_create_views(w,h);
}

static int compat_d3d11_begin_frame(void) {
    if (!pex_using_d3d11() || !g_d3d11.ctx) return 0;
    g_d3d11.draw_calls = 0; g_d3d11.triangles = 0;
    ID3D11DeviceContext_OMSetRenderTargets(g_d3d11.ctx, 1, &g_d3d11.rtv, g_d3d11.dsv);
    ID3D11DeviceContext_IASetInputLayout(g_d3d11.ctx, g_d3d11.layout);
    ID3D11DeviceContext_VSSetShader(g_d3d11.ctx, g_d3d11.vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(g_d3d11.ctx, g_d3d11.ps, NULL, 0);
    ID3D11DeviceContext_RSSetState(g_d3d11.ctx, g_d3d11.rast_nocull);
    return 1;
}

static void pex_renderer_d3d11_present(void) {
    if (!pex_using_d3d11() || !g_d3d11.swap) return;
    pex_gpu_flush_immediate_stream();
    UINT flags = (g_d3d11.allow_tearing && g_opts.max_fps <= 0) ? DXGI_PRESENT_ALLOW_TEARING : 0;
    HRESULT hr;
    g_loggy_d3d11_active = 1;
    g_loggy_d3d11_allow_tearing = g_d3d11.allow_tearing;
    g_loggy_d3d11_swap_flags = g_d3d11.swap_flags;
    g_loggy_d3d11_present_flags = flags;
    g_loggy_d3d11_buffer_count = g_d3d11.buffer_count;
    g_loggy_d3d11_frame_latency = g_d3d11.frame_latency;
    g_loggy_d3d11_frame_latency_set = g_d3d11.frame_latency_set;
    hr = IDXGISwapChain_Present(g_d3d11.swap, 0, flags);
    g_loggy_d3d11_present_hr = (long)hr;
    if (FAILED(hr)) g_loggy_d3d11_present_failures++;
}

static D3D11_PRIMITIVE_TOPOLOGY compat_d3d11_topology_for_mode(int mode) {
    if (mode == GL_LINES) return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
    if (mode == GL_LINE_STRIP) return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
    return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

static int compat_d3d11_create_static_vb(PexD3DBatch *b, const PexD3DVertex *vertices, int count) {
    if (!g_d3d11.dev || !b || !vertices || count <= 0) return 0;
    D3D11_BUFFER_DESC bd; memset(&bd,0,sizeof(bd));
    bd.ByteWidth = (UINT)(sizeof(PexD3DVertex) * (size_t)count);
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA init; memset(&init,0,sizeof(init)); init.pSysMem = vertices;
    return SUCCEEDED(ID3D11Device_CreateBuffer(g_d3d11.dev, &bd, &init, &b->vb11));
}

static int compat_d3d11_ensure_dynamic_vb(UINT vertex_count) {
    if (!g_d3d11.dev || vertex_count == 0) return 0;
    if (g_d3d11.dynamic_vb && g_d3d11.dynamic_vb_capacity >= vertex_count) return 1;
    if (g_d3d11.dynamic_vb) { ID3D11Buffer_Release(g_d3d11.dynamic_vb); g_d3d11.dynamic_vb = NULL; }
    UINT cap = g_d3d11.dynamic_vb_capacity ? g_d3d11.dynamic_vb_capacity * 2 : 8192;
    while (cap < vertex_count) cap *= 2;
    D3D11_BUFFER_DESC bd; memset(&bd,0,sizeof(bd));
    bd.ByteWidth = cap * (UINT)sizeof(PexD3DVertex);
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(ID3D11Device_CreateBuffer(g_d3d11.dev, &bd, NULL, &g_d3d11.dynamic_vb))) return 0;
    g_d3d11.dynamic_vb_capacity = cap;
    return 1;
}

static void pex_d3d11_make_cb(const PexD3DStateSnap *s, PexD3D11CB *cb) {
    D3DMATRIX mv = pex_to_d3d_matrix(&g_d3d9.modelview);
    PexMat4 p = g_d3d9.projection; compat_depth_remap_gl_to_d3d(&p);
    D3DMATRIX pr = pex_to_d3d_matrix(&p);
    memcpy(cb->model, &mv, sizeof(float)*16);
    memcpy(cb->proj, &pr, sizeof(float)*16);
    cb->flags[0] = s->alpha_test_enabled ? 1.0f : 0.0f;
    cb->flags[1] = (s->texture_enabled && s->texture_id < PEX_D3D_MAX_TEXTURES && g_d3d9.textures[s->texture_id].srv11) ? 1.0f : 0.0f;
    cb->flags[2] = s->fog_enabled ? 1.0f : 0.0f;
    cb->flags[3] = 0.0f;
    DWORD fc = s->fog_color;
    cb->fog_color[0] = ((fc >> 16) & 255) / 255.0f;
    cb->fog_color[1] = ((fc >> 8) & 255) / 255.0f;
    cb->fog_color[2] = (fc & 255) / 255.0f;
    cb->fog_color[3] = ((fc >> 24) & 255) / 255.0f;
    cb->fog_params[0] = s->fog_start; cb->fog_params[1] = s->fog_end; cb->fog_params[2] = cb->fog_params[3] = 0.0f;
}

static void compat_d3d11_bind_pipeline(void) {
    if (!g_d3d11.ctx) return;
    /* Native world drawing uses renderer_d3d11.c, which binds a different
       input layout/shader pair for PexVertex (pos,uv,color).  The remaining
       GUI/text/sky compatibility draws feed PexD3DVertex (pos,color,uv).
       Rebind this pipeline before every compatibility draw so UI/text does
       not read color bytes as UVs or UVs as colors. */
    ID3D11DeviceContext_IASetInputLayout(g_d3d11.ctx, g_d3d11.layout);
    ID3D11DeviceContext_VSSetShader(g_d3d11.ctx, g_d3d11.vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(g_d3d11.ctx, g_d3d11.ps, NULL, 0);
    ID3D11DeviceContext_RSSetState(g_d3d11.ctx, g_d3d11.rast_nocull);
}

static void pex_d3d11_apply_state(const PexD3DStateSnap *s) {
    if (!g_d3d11.ctx) return;
    compat_d3d11_bind_pipeline();
    ID3D11BlendState *blend = (s->color_write_mask == 0) ? g_d3d11.blend_color_off : g_d3d11.blend_off;
    if (s->color_write_mask != 0 && s->blend_enabled) {
        if (s->src_blend == D3DBLEND_DESTCOLOR && s->dst_blend == D3DBLEND_SRCCOLOR) blend = g_d3d11.blend_multiply;
        else if (s->src_blend == D3DBLEND_SRCALPHA && s->dst_blend == D3DBLEND_ONE) blend = g_d3d11.blend_additive;
        else blend = g_d3d11.blend_alpha;
    }
    ID3D11DeviceContext_OMSetBlendState(g_d3d11.ctx, blend, NULL, 0xffffffffu);
    ID3D11DepthStencilState *ds = s->depth_enabled ? (s->depth_write ? g_d3d11.depth_on_write : g_d3d11.depth_on_nowrite) : g_d3d11.depth_off;
    ID3D11DeviceContext_OMSetDepthStencilState(g_d3d11.ctx, ds, 0);
    ID3D11SamplerState *samp = g_d3d11.sampler_clamp;
    ID3D11ShaderResourceView *srv = NULL;
    if (s->texture_enabled && s->texture_id < PEX_D3D_MAX_TEXTURES && g_d3d9.textures[s->texture_id].srv11) {
        int wrap = (g_d3d9.textures[s->texture_id].address_u == D3DTADDRESS_WRAP || g_d3d9.textures[s->texture_id].address_v == D3DTADDRESS_WRAP);
        int linear = (g_d3d9.textures[s->texture_id].min_filter == D3DTEXF_LINEAR || g_d3d9.textures[s->texture_id].mag_filter == D3DTEXF_LINEAR);
        srv = g_d3d9.textures[s->texture_id].srv11;
        if (linear) samp = wrap ? g_d3d11.sampler_wrap_linear : g_d3d11.sampler_clamp_linear;
        else if (wrap) samp = g_d3d11.sampler_wrap;
    }
    ID3D11DeviceContext_PSSetShaderResources(g_d3d11.ctx, 0, 1, &srv);
    ID3D11DeviceContext_PSSetSamplers(g_d3d11.ctx, 0, 1, &samp);
    PexD3D11CB cb; pex_d3d11_make_cb(s, &cb);
    D3D11_MAPPED_SUBRESOURCE map;
    if (SUCCEEDED(ID3D11DeviceContext_Map(g_d3d11.ctx, (ID3D11Resource*)g_d3d11.cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map))) {
        memcpy(map.pData, &cb, sizeof(cb));
        ID3D11DeviceContext_Unmap(g_d3d11.ctx, (ID3D11Resource*)g_d3d11.cbuffer, 0);
    }
    ID3D11DeviceContext_VSSetConstantBuffers(g_d3d11.ctx, 0, 1, &g_d3d11.cbuffer);
    ID3D11DeviceContext_PSSetConstantBuffers(g_d3d11.ctx, 0, 1, &g_d3d11.cbuffer);
}

static void compat_d3d11_draw_dynamic(D3D11_PRIMITIVE_TOPOLOGY top, const PexD3DVertex *vertices, UINT vertex_count) {
    if (!g_d3d11.ctx || !vertices || vertex_count == 0) return;
    compat_d3d11_bind_pipeline();
    if (!compat_d3d11_ensure_dynamic_vb(vertex_count)) return;
    D3D11_MAPPED_SUBRESOURCE map;
    UINT bytes = vertex_count * (UINT)sizeof(PexD3DVertex);
    if (FAILED(ID3D11DeviceContext_Map(g_d3d11.ctx, (ID3D11Resource*)g_d3d11.dynamic_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &map))) return;
    memcpy(map.pData, vertices, bytes);
    ID3D11DeviceContext_Unmap(g_d3d11.ctx, (ID3D11Resource*)g_d3d11.dynamic_vb, 0);
    UINT stride = sizeof(PexD3DVertex), offset = 0;
    ID3D11DeviceContext_IASetPrimitiveTopology(g_d3d11.ctx, top);
    ID3D11DeviceContext_IASetVertexBuffers(g_d3d11.ctx, 0, 1, &g_d3d11.dynamic_vb, &stride, &offset);
    ID3D11DeviceContext_Draw(g_d3d11.ctx, vertex_count, 0);
    g_d3d11.draw_calls++;
    if (top == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) g_d3d11.triangles += (int)(vertex_count / 3);
}

static void compat_d3d11_draw_batch(const PexD3DBatch *b) {
    if (!g_d3d11.ctx || !b || b->vertex_count == 0) return;
    pex_d3d11_apply_state(&b->state);
    D3D11_PRIMITIVE_TOPOLOGY top = compat_d3d11_topology_for_mode(b->mode);
    ID3D11DeviceContext_IASetPrimitiveTopology(g_d3d11.ctx, top);
    UINT stride = sizeof(PexD3DVertex), offset = 0;
    if (b->vb11) {
        ID3D11DeviceContext_IASetVertexBuffers(g_d3d11.ctx, 0, 1, &b->vb11, &stride, &offset);
        ID3D11DeviceContext_Draw(g_d3d11.ctx, b->vertex_count, 0);
        g_d3d11.draw_calls++;
        if (top == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) g_d3d11.triangles += (int)(b->vertex_count / 3);
    } else if (b->sysmem && b->sysmem_count > 0) {
        compat_d3d11_draw_dynamic(top, b->sysmem, b->vertex_count);
    }
}

static void pex_d3d11_flush_batch(const PexD3DBatch *b) {
    if (!g_d3d11.ctx || !b || b->count <= 0) return;
    int outn = compat_d3d_count_vertices(b->mode, b->count);
    if (outn <= 0 || !pex_scratch_reserve(outn)) return;
    compat_d3d_convert_batch(b, g_d3d9.scratch);
    pex_d3d11_apply_state(&b->state);
    compat_d3d11_draw_dynamic(compat_d3d11_topology_for_mode(b->mode), g_d3d9.scratch, (UINT)outn);
}

static int pex_gpu_state_equal(const PexD3DStateSnap *a, const PexD3DStateSnap *b) {
    return a->texture_enabled == b->texture_enabled &&
           a->texture_id == b->texture_id &&
           a->blend_enabled == b->blend_enabled &&
           a->src_blend == b->src_blend &&
           a->dst_blend == b->dst_blend &&
           a->alpha_test_enabled == b->alpha_test_enabled &&
           a->depth_enabled == b->depth_enabled &&
           a->depth_write == b->depth_write &&
           a->depth_func == b->depth_func &&
           a->fog_enabled == b->fog_enabled &&
           a->fog_start == b->fog_start &&
           a->fog_end == b->fog_end &&
           a->fog_color == b->fog_color &&
           a->cull_enabled == b->cull_enabled &&
           a->color_write_mask == b->color_write_mask;
}

static int pex_gpu_stream_reserve(int add) {
    if (add <= 0) return 1;
    if (g_gpu_imm.count + add <= g_gpu_imm.cap) return 1;
    int nc = g_gpu_imm.cap ? g_gpu_imm.cap * 2 : 4096;
    while (nc < g_gpu_imm.count + add) nc *= 2;
    PexD3DVertex *nv = (PexD3DVertex*)realloc(g_gpu_imm.vertices, sizeof(PexD3DVertex) * (size_t)nc);
    if (!nv) return 0;
    g_gpu_imm.vertices = nv;
    g_gpu_imm.cap = nc;
    return 1;
}

static int pex_gpu_batch_can_stream(const PexD3DBatch *b) {
    /* GL_LINE_STRIP cannot be merged safely because concatenating strips would
       create an unwanted line between consecutive UI/debug batches. */
    return b && (b->mode == GL_QUADS || b->mode == GL_TRIANGLES || b->mode == GL_TRIANGLE_FAN || b->mode == GL_LINES);
}

static void pex_gpu_flush_immediate_stream(void) {
    if (!g_gpu_imm.active || g_gpu_imm.count <= 0) {
        g_gpu_imm.active = 0;
        g_gpu_imm.count = 0;
        return;
    }

    if (pex_using_d3d11()) {
        pex_d3d11_apply_state(&g_gpu_imm.state);
        compat_d3d11_draw_dynamic(g_gpu_imm.d3d11_top, g_gpu_imm.vertices, (UINT)g_gpu_imm.count);
    } else if (pex_using_d3d9()) {
        UINT prim_count = 0;
        if (g_gpu_imm.d3d_prim == D3DPT_TRIANGLELIST) prim_count = (UINT)(g_gpu_imm.count / 3);
        else if (g_gpu_imm.d3d_prim == D3DPT_LINELIST) prim_count = (UINT)(g_gpu_imm.count / 2);
        else if (g_gpu_imm.d3d_prim == D3DPT_LINESTRIP) prim_count = g_gpu_imm.count > 1 ? (UINT)(g_gpu_imm.count - 1) : 0;
        if (prim_count) {
            pex_d3d_apply_state(&g_gpu_imm.state);
            pex_d3d_apply_matrices();
            compat_d3d_draw_dynamic(g_gpu_imm.d3d_prim, prim_count, g_gpu_imm.vertices, (UINT)g_gpu_imm.count);
        }
    }

    g_gpu_imm.active = 0;
    g_gpu_imm.count = 0;
}

static void compat_queue_or_draw_batch(const PexD3DBatch *b) {
    if (!b || b->count <= 0) return;

    if (!pex_gpu_batch_can_stream(b)) {
        pex_gpu_flush_immediate_stream();
        if (pex_using_d3d11()) pex_d3d11_flush_batch(b);
        else pex_d3d_flush_batch(b);
        return;
    }

    int outn = compat_d3d_count_vertices(b->mode, b->count);
    if (outn <= 0 || !pex_scratch_reserve(outn)) return;
    compat_d3d_convert_batch(b, g_d3d9.scratch);

    D3DPRIMITIVETYPE prim = compat_d3d_primitive_for_mode(b->mode);
    D3D11_PRIMITIVE_TOPOLOGY top = compat_d3d11_topology_for_mode(b->mode);

    if (g_gpu_imm.active &&
        (g_gpu_imm.d3d_prim != prim || g_gpu_imm.d3d11_top != top || !pex_gpu_state_equal(&g_gpu_imm.state, &b->state))) {
        pex_gpu_flush_immediate_stream();
    }

    if (!g_gpu_imm.active) {
        g_gpu_imm.active = 1;
        g_gpu_imm.mode = b->mode;
        g_gpu_imm.d3d_prim = prim;
        g_gpu_imm.d3d11_top = top;
        g_gpu_imm.state = b->state;
        g_gpu_imm.count = 0;
    }

    if (!pex_gpu_stream_reserve(outn)) {
        pex_gpu_flush_immediate_stream();
        if (pex_using_d3d11()) pex_d3d11_flush_batch(b);
        else pex_d3d_flush_batch(b);
        return;
    }

    memcpy(g_gpu_imm.vertices + g_gpu_imm.count, g_d3d9.scratch, sizeof(PexD3DVertex) * (size_t)outn);
    g_gpu_imm.count += outn;
}


static int pex_renderer_backend_init(HWND hwnd) {
#if defined(PEX_PLATFORM_XBOX_UWP)
    g_runtime_renderer_backend = RENDERER_D3D11;
    g_selected_renderer_backend = RENDERER_D3D11;
    if (!pex_renderer_d3d11_xbox_init_corewindow((void*)hwnd, g_win_w, g_win_h)) return 0;
    renderer_d3d11_attach_device((void*)g_d3d11.dev, (void*)g_d3d11.ctx, g_d3d11.width, g_d3d11.height);
    scan_texture_packs();
    if (!load_default_textures()) return 0;
    if (g_selected_texpack > 0) apply_texture_pack_index(g_selected_texpack);
    else { if (g_opts.skin_path[0]) load_custom_skin_path(g_opts.skin_path, 0); init_font_widths(); }
    return 1;
#else
    if (g_runtime_renderer_backend == RENDERER_D3D9) {
        if (!pex_renderer_d3d9_init(hwnd)) return 0;
        renderer_d3d9_attach_device((void*)g_d3d9.dev, g_d3d9.width, g_d3d9.height);
        scan_texture_packs();
        if (!load_default_textures()) return 0;
        if (g_selected_texpack > 0) apply_texture_pack_index(g_selected_texpack);
        else { if (g_opts.skin_path[0]) load_custom_skin_path(g_opts.skin_path, 0); init_font_widths(); }
        return 1;
    }
    if (g_runtime_renderer_backend == RENDERER_D3D11) {
        if (!pex_renderer_d3d11_init(hwnd)) return 0;
        renderer_d3d11_attach_device((void*)g_d3d11.dev, (void*)g_d3d11.ctx, g_d3d11.width, g_d3d11.height);
        scan_texture_packs();
        if (!load_default_textures()) return 0;
        if (g_selected_texpack > 0) apply_texture_pack_index(g_selected_texpack);
        else { if (g_opts.skin_path[0]) load_custom_skin_path(g_opts.skin_path, 0); init_font_widths(); }
        return 1;
    }
    return init_gl(hwnd);
#endif
}

static int pex_renderer_begin_frame(void) {
    if (pex_using_d3d9()) return compat_d3d9_begin_frame();
    if (pex_using_d3d11()) return compat_d3d11_begin_frame();
    return 1;
}

static void pex_renderer_present(void) {
    if (pex_using_d3d9()) pex_renderer_d3d9_present();
    else if (pex_using_d3d11()) pex_renderer_d3d11_present();
    else SwapBuffers(g_hdc);
}

static void pex_renderer_resize(int w, int h) {
    if (pex_using_d3d9()) pex_renderer_d3d9_resize(w, h);
    else if (pex_using_d3d11()) pex_renderer_d3d11_resize(w, h);
}

static void pex_renderer_shutdown(void) {
    pex_gpu_flush_immediate_stream();
    free(g_gpu_imm.vertices); g_gpu_imm.vertices = NULL; g_gpu_imm.count = g_gpu_imm.cap = g_gpu_imm.active = 0;
    renderer_d3d11_get_backend()->shutdown();
    renderer_d3d9_get_backend()->shutdown();
    if (g_d3d11.active) pex_renderer_d3d11_shutdown();
    if (g_d3d9.active) pex_renderer_d3d9_shutdown();
}

static PEX_THREAD_LOCAL int g_pex_suppress_gl_immediate = 0;
static void pex_gl_suppress_immediate(int on) { g_pex_suppress_gl_immediate = on ? 1 : 0; }

/* OpenGL-compatible entry points used by the rest of this codebase. */
static void pex_glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    if (pex_using_gpu_backend()) { g_d3d9.clear_color = pex_pack_color(r,g,b,a); }
    else glClearColor(r,g,b,a);
}
static void pex_glDisable(GLenum cap) { if (g_pex_suppress_gl_immediate) return; if (!pex_using_gpu_backend()) { glDisable(cap); return; } if(cap==GL_TEXTURE_2D)g_d3d9.texture_enabled=0; else if(cap==GL_BLEND)g_d3d9.blend_enabled=0; else if(cap==GL_ALPHA_TEST)g_d3d9.alpha_test_enabled=0; else if(cap==GL_DEPTH_TEST)g_d3d9.depth_enabled=0; else if(cap==GL_CULL_FACE)g_d3d9.cull_enabled=0; else if(cap==GL_FOG)g_d3d9.fog_enabled=0; }
static void pex_glEnable(GLenum cap) { if (g_pex_suppress_gl_immediate) return; if (!pex_using_gpu_backend()) { glEnable(cap); return; } if(cap==GL_TEXTURE_2D)g_d3d9.texture_enabled=1; else if(cap==GL_BLEND)g_d3d9.blend_enabled=1; else if(cap==GL_ALPHA_TEST)g_d3d9.alpha_test_enabled=1; else if(cap==GL_DEPTH_TEST)g_d3d9.depth_enabled=1; else if(cap==GL_CULL_FACE)g_d3d9.cull_enabled=1; else if(cap==GL_FOG)g_d3d9.fog_enabled=1; }
static void pex_glBlendFunc(GLenum s, GLenum d) { if (g_pex_suppress_gl_immediate) return; if (!pex_using_gpu_backend()) { glBlendFunc(s,d); return; } g_d3d9.src_blend=pex_convert_blend(s); g_d3d9.dst_blend=pex_convert_blend(d); }
static void pex_glDepthFunc(GLenum f) { if (g_pex_suppress_gl_immediate) return; if (!pex_using_gpu_backend()) { glDepthFunc(f); return; } g_d3d9.depth_func = pex_convert_depth_func(f); }
static void pex_glDepthMask(GLboolean b) { if (g_pex_suppress_gl_immediate) return; if (!pex_using_gpu_backend()) { glDepthMask(b); return; } g_d3d9.depth_write = b ? 1 : 0; }
static void pex_glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
    if (g_pex_suppress_gl_immediate) return;
    if (!pex_using_gpu_backend()) { glColorMask(r,g,b,a); return; }
    pex_gpu_flush_immediate_stream();
    DWORD mask = 0;
    if (r) mask |= D3DCOLORWRITEENABLE_RED;
    if (g) mask |= D3DCOLORWRITEENABLE_GREEN;
    if (b) mask |= D3DCOLORWRITEENABLE_BLUE;
    if (a) mask |= D3DCOLORWRITEENABLE_ALPHA;
    g_d3d9.color_write_mask = mask;
    if (pex_using_d3d9() && g_d3d9.dev) compat_d3d_set_render_state(D3DRS_COLORWRITEENABLE, mask);
}
static void pex_glAlphaFunc(GLenum f, GLclampf ref) { if (g_pex_suppress_gl_immediate) return; if (!pex_using_gpu_backend()) { glAlphaFunc(f,ref); return; } (void)f; (void)ref; }
static void pex_glLineWidth(GLfloat w) { if (g_pex_suppress_gl_immediate) return; if (!pex_using_gpu_backend()) glLineWidth(w); else (void)w; }
static void pex_glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { if (g_pex_suppress_gl_immediate) return; if (!pex_using_gpu_backend()) { glColor4f(r,g,b,a); return; } g_d3d9.color[0]=r;g_d3d9.color[1]=g;g_d3d9.color[2]=b;g_d3d9.color[3]=a; }
static void pex_glViewport(GLint x, GLint y, GLsizei w, GLsizei h) { if (!pex_using_gpu_backend()) { glViewport(x,y,w,h); return; } pex_gpu_flush_immediate_stream(); g_d3d9.viewport_x=x; g_d3d9.viewport_y=y; g_d3d9.viewport_w=w; g_d3d9.viewport_h=h; int dy = g_render_h - y - h; if (dy < 0) dy = 0; if (pex_using_d3d11() && g_d3d11.ctx) { D3D11_VIEWPORT vp; memset(&vp,0,sizeof(vp)); vp.TopLeftX=(FLOAT)x; vp.TopLeftY=(FLOAT)dy; vp.Width=(FLOAT)w; vp.Height=(FLOAT)h; vp.MinDepth=0.0f; vp.MaxDepth=1.0f; ID3D11DeviceContext_RSSetViewports(g_d3d11.ctx,1,&vp); } else if (g_d3d9.dev) { D3DVIEWPORT9 vp={(DWORD)x,(DWORD)dy,(DWORD)w,(DWORD)h,0.0f,1.0f}; IDirect3DDevice9_SetViewport(g_d3d9.dev,&vp); } }
static void pex_glClear(GLbitfield mask) { if (!pex_using_gpu_backend()) { glClear(mask); return; } pex_gpu_flush_immediate_stream(); if (pex_using_d3d11()) { if((mask&GL_COLOR_BUFFER_BIT) && g_d3d11.rtv){ DWORD c=g_d3d9.clear_color; float col[4]={((c>>16)&255)/255.0f,((c>>8)&255)/255.0f,(c&255)/255.0f,((c>>24)&255)/255.0f}; ID3D11DeviceContext_ClearRenderTargetView(g_d3d11.ctx,g_d3d11.rtv,col); } if((mask&GL_DEPTH_BUFFER_BIT) && g_d3d11.dsv) ID3D11DeviceContext_ClearDepthStencilView(g_d3d11.ctx,g_d3d11.dsv,D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL,1.0f,0); return; } DWORD flags=0; if(mask&GL_COLOR_BUFFER_BIT)flags|=D3DCLEAR_TARGET; if(mask&GL_DEPTH_BUFFER_BIT)flags|=D3DCLEAR_ZBUFFER; IDirect3DDevice9_Clear(g_d3d9.dev,0,NULL,flags,g_d3d9.clear_color,1.0f,0); }
static void pex_glMatrixMode(GLenum m) { if (!pex_using_gpu_backend()) { glMatrixMode(m); return; } g_d3d9.matrix_mode = m; }
static void pex_glLoadIdentity(void) { if (!pex_using_gpu_backend()) { glLoadIdentity(); return; } pex_gpu_flush_immediate_stream(); pex_mat_identity(pex_current_matrix()); compat_d3d_mark_matrix_dirty(); }
static void pex_glPushMatrix(void) { if (!pex_using_gpu_backend()) { glPushMatrix(); return; } pex_gpu_flush_immediate_stream(); if(g_d3d9.matrix_mode==GL_PROJECTION){ if(g_d3d9.pr_sp<PEX_D3D_STACK_MAX) g_d3d9.pr_stack[g_d3d9.pr_sp++]=g_d3d9.projection; } else { if(g_d3d9.mv_sp<PEX_D3D_STACK_MAX) g_d3d9.mv_stack[g_d3d9.mv_sp++]=g_d3d9.modelview; } }
static void pex_glPopMatrix(void) { if (!pex_using_gpu_backend()) { glPopMatrix(); return; } pex_gpu_flush_immediate_stream(); if(g_d3d9.matrix_mode==GL_PROJECTION){ if(g_d3d9.pr_sp>0) g_d3d9.projection=g_d3d9.pr_stack[--g_d3d9.pr_sp]; } else { if(g_d3d9.mv_sp>0) g_d3d9.modelview=g_d3d9.mv_stack[--g_d3d9.mv_sp]; } compat_d3d_mark_matrix_dirty(); }
static void pex_glTranslatef(GLfloat x, GLfloat y, GLfloat z) { if (!pex_using_gpu_backend()) { glTranslatef(x,y,z); return; } pex_gpu_flush_immediate_stream(); pex_mat_translate(x,y,z); compat_d3d_mark_matrix_dirty(); }
static void pex_glRotatef(GLfloat a, GLfloat x, GLfloat y, GLfloat z) { if (!pex_using_gpu_backend()) { glRotatef(a,x,y,z); return; } pex_gpu_flush_immediate_stream(); pex_mat_rotate(a,x,y,z); compat_d3d_mark_matrix_dirty(); }
static void pex_glScalef(GLfloat x, GLfloat y, GLfloat z) { if (!pex_using_gpu_backend()) { glScalef(x,y,z); return; } pex_gpu_flush_immediate_stream(); pex_mat_scale(x,y,z); compat_d3d_mark_matrix_dirty(); }
static void pex_glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) { if (!pex_using_gpu_backend()) { glOrtho(l,r,b,t,n,f); return; } pex_gpu_flush_immediate_stream(); PexMat4 m; pex_mat_identity(&m); m.m[0]=(float)(2.0/(r-l)); m.m[5]=(float)(2.0/(t-b)); m.m[10]=(float)(-2.0/(f-n)); m.m[12]=(float)(-(r+l)/(r-l)); m.m[13]=(float)(-(t+b)/(t-b)); m.m[14]=(float)(-(f+n)/(f-n)); pex_mat_postmul(pex_current_matrix(), &m); compat_d3d_mark_matrix_dirty(); }
static void pex_gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zn, GLdouble zf) { if (!pex_using_gpu_backend()) { gluPerspective(fovy,aspect,zn,zf); return; } pex_gpu_flush_immediate_stream(); pex_glstyle_perspective(fovy,aspect,zn,zf); compat_d3d_mark_matrix_dirty(); }
static void pex_glGetFloatv(GLenum pname, GLfloat *params) { if (!pex_using_gpu_backend()) { glGetFloatv(pname, params); return; } if(pname==GL_PROJECTION_MATRIX) memcpy(params,g_d3d9.projection.m,sizeof(float)*16); else if(pname==GL_MODELVIEW_MATRIX) memcpy(params,g_d3d9.modelview.m,sizeof(float)*16); }
static void pex_glGetDoublev(GLenum pname, GLdouble *params) {
    if (!pex_using_gpu_backend()) { glGetDoublev(pname, params); return; }
    const PexMat4 *m = NULL;
    if (pname == GL_PROJECTION_MATRIX) m = &g_d3d9.projection;
    else if (pname == GL_MODELVIEW_MATRIX) m = &g_d3d9.modelview;
    if (!m || !params) return;
    for (int i = 0; i < 16; i++) params[i] = (GLdouble)m->m[i];
}
static void pex_glGetIntegerv(GLenum pname, GLint *params) {
    if (!pex_using_gpu_backend()) { glGetIntegerv(pname, params); return; }
    if (!params) return;
    if (pname == GL_VIEWPORT) {
        params[0] = g_d3d9.viewport_x;
        params[1] = g_d3d9.viewport_y;
        params[2] = g_d3d9.viewport_w;
        params[3] = g_d3d9.viewport_h;
    }
}
static void pex_glFogi(GLenum pname, GLint param) { if (!pex_using_gpu_backend()) glFogi(pname,param); else (void)pname,(void)param; }
static void pex_glFogf(GLenum pname, GLfloat param) { if (!pex_using_gpu_backend()) { glFogf(pname,param); return; } if(pname==GL_FOG_START)g_d3d9.fog_start=param; else if(pname==GL_FOG_END)g_d3d9.fog_end=param; }
static void pex_glFogfv(GLenum pname, const GLfloat *p) { if (!pex_using_gpu_backend()) { glFogfv(pname,p); return; } if(pname==GL_FOG_COLOR) g_d3d9.fog_color=pex_pack_color(p[0],p[1],p[2],p[3]); }
static float pex_current_u = 0.0f, pex_current_v = 0.0f;
static void pex_glTexCoord2f_fixed(GLfloat u, GLfloat v) { if (!pex_using_gpu_backend()) { glTexCoord2f(u,v); return; } pex_current_u=u; pex_current_v=v; }

static void pex_glBegin(GLenum mode) {
    if (g_pex_suppress_gl_immediate) return;
    if (!pex_using_gpu_backend()) { glBegin(mode); return; }
    g_d3d9.begin_active=1; g_d3d9.begin_mode=mode;
    PexD3DStateSnap s; pex_capture_state(&s);
    if (g_d3d9.compiling) {
        PexD3DList *l = pex_list_ensure(g_d3d9.compile_list);
        g_d3d9.compile_batch = pex_list_add_batch(l, mode, &s);
    } else {
        g_d3d9.immediate.count = 0;
        g_d3d9.immediate.mode = mode;
        g_d3d9.immediate.state = s;
        g_d3d9.immediate.d3d_prim = 0;
        g_d3d9.immediate.primitive_count = 0;
        g_d3d9.immediate.vertex_count = 0;
        g_d3d9.compile_batch = &g_d3d9.immediate;
    }
}
static void pex_glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
    if (g_pex_suppress_gl_immediate) return;
    if (!pex_using_gpu_backend()) { glVertex3f(x,y,z); return; }
    if (!g_d3d9.begin_active || !g_d3d9.compile_batch) return;
    PexD3DBatch *b = g_d3d9.compile_batch; pex_batch_reserve(b,1);
    PexRawVertex *v = &b->v[b->count++];
    v->x=x; v->y=y; v->z=z; v->u=pex_current_u; v->v=pex_current_v;
    v->color=pex_pack_color(g_d3d9.color[0],g_d3d9.color[1],g_d3d9.color[2],g_d3d9.color[3]);
}
static void pex_glVertex2f(GLfloat x, GLfloat y) { if (!pex_using_gpu_backend()) { glVertex2f(x,y); return; } pex_glVertex3f(x,y,0.0f); }
static void pex_glEnd(void) {
    if (g_pex_suppress_gl_immediate) return;
    if (!pex_using_gpu_backend()) { glEnd(); return; }
    if (g_d3d9.compiling && g_d3d9.compile_batch) {
        pex_batch_finalize_static(g_d3d9.compile_batch);
    } else {
        compat_queue_or_draw_batch(&g_d3d9.immediate);
    }
    g_d3d9.begin_active=0; g_d3d9.compile_batch=NULL;
}

static GLuint pex_glGenLists(GLsizei range) { if (!pex_using_gpu_backend()) return glGenLists(range); if(range<=0)return 0; unsigned int base=g_d3d9.next_list; if(base+(unsigned int)range>=PEX_D3D_MAX_LISTS)return 0; for(int i=0;i<range;i++){ unsigned int id=base+(unsigned int)i; if(g_d3d9.lists[id]){ pex_list_free(g_d3d9.lists[id]); g_d3d9.lists[id]=NULL; } pex_list_ensure(id); } g_d3d9.next_list += (unsigned int)range; return base; }
static void pex_glNewList(GLuint list, GLenum mode) { if (!pex_using_gpu_backend()) { glNewList(list,mode); return; } (void)mode; if(list>=PEX_D3D_MAX_LISTS)return; if(g_d3d9.lists[list]){ pex_list_free(g_d3d9.lists[list]); g_d3d9.lists[list]=NULL; } pex_list_ensure(list); g_d3d9.compiling=1; g_d3d9.compile_list=list; }
static void pex_glEndList(void) { if (!pex_using_gpu_backend()) { glEndList(); return; } g_d3d9.compiling=0; g_d3d9.compile_list=0; }
static void pex_glCallList(GLuint list) { if (!pex_using_gpu_backend()) { glCallList(list); return; } pex_gpu_flush_immediate_stream(); if(list>=PEX_D3D_MAX_LISTS||!g_d3d9.lists[list])return; PexD3DList *l=g_d3d9.lists[list]; for(int i=0;i<l->count;i++){ if(pex_using_d3d11()) compat_d3d11_draw_batch(&l->batches[i]); else compat_d3d_draw_batch(&l->batches[i]); } }

static void pex_glGenTextures(GLsizei n, GLuint *ids) { if (!pex_using_gpu_backend()) { glGenTextures(n,ids); return; } for(int i=0;i<n;i++){ if(g_d3d9.next_texture>=PEX_D3D_MAX_TEXTURES) ids[i]=0; else { ids[i]=g_d3d9.next_texture++; g_d3d9.textures[ids[i]].address_u=D3DTADDRESS_CLAMP; g_d3d9.textures[ids[i]].address_v=D3DTADDRESS_CLAMP; g_d3d9.textures[ids[i]].min_filter=D3DTEXF_POINT; g_d3d9.textures[ids[i]].mag_filter=D3DTEXF_POINT; } } }
static void pex_glBindTexture(GLenum target, GLuint id) { if (g_pex_suppress_gl_immediate) return; if (!pex_using_gpu_backend()) { glBindTexture(target,id); return; } (void)target; g_d3d9.bound_texture=id; }
static void pex_glTexParameteri(GLenum target, GLenum pname, GLint param) {
    if (!pex_using_gpu_backend()) { glTexParameteri(target,pname,param); return; }
    pex_gpu_flush_immediate_stream();
    (void)target;
    if (g_d3d9.bound_texture < PEX_D3D_MAX_TEXTURES) {
        PexD3DTexture *t = &g_d3d9.textures[g_d3d9.bound_texture];
        if (pname == GL_TEXTURE_WRAP_S) {
            t->address_u = (param == GL_REPEAT) ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
        } else if (pname == GL_TEXTURE_WRAP_T) {
            t->address_v = (param == GL_REPEAT) ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
        } else if (pname == GL_TEXTURE_MIN_FILTER) {
            t->min_filter = (param == GL_LINEAR) ? D3DTEXF_LINEAR : D3DTEXF_POINT;
        } else if (pname == GL_TEXTURE_MAG_FILTER) {
            t->mag_filter = (param == GL_LINEAR) ? D3DTEXF_LINEAR : D3DTEXF_POINT;
        }
    }
}
static void pex_glTexImage2D(GLenum target, GLint level, GLint internal, GLsizei w, GLsizei h, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
    if (!pex_using_gpu_backend()) { glTexImage2D(target,level,internal,w,h,border,format,type,pixels); return; }
    pex_gpu_flush_immediate_stream();
    (void)target;(void)level;(void)internal;(void)border;(void)format;(void)type;
    unsigned int id=g_d3d9.bound_texture; if(id==0||id>=PEX_D3D_MAX_TEXTURES)return;
    g_d3d9.textures[id].w=w; g_d3d9.textures[id].h=h;
    if (pex_using_d3d11()) {
        if(g_d3d9.textures[id].srv11){ ID3D11ShaderResourceView_Release(g_d3d9.textures[id].srv11); g_d3d9.textures[id].srv11=NULL; }
        if(g_d3d9.textures[id].tex11){ ID3D11Texture2D_Release(g_d3d9.textures[id].tex11); g_d3d9.textures[id].tex11=NULL; }
        if(!g_d3d11.dev || w<=0 || h<=0) return;
        D3D11_TEXTURE2D_DESC td; memset(&td,0,sizeof(td));
        td.Width=(UINT)w; td.Height=(UINT)h; td.MipLevels=1; td.ArraySize=1; td.Format=DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count=1; td.Usage=D3D11_USAGE_DEFAULT; td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA init; memset(&init,0,sizeof(init));
        unsigned char *rgba = NULL;
        if(pixels) { rgba=(unsigned char*)malloc((size_t)w*(size_t)h*4u); if(!rgba) return; memcpy(rgba,pixels,(size_t)w*(size_t)h*4u); init.pSysMem=rgba; init.SysMemPitch=(UINT)w*4u; }
        HRESULT hr = ID3D11Device_CreateTexture2D(g_d3d11.dev,&td,pixels?&init:NULL,&g_d3d9.textures[id].tex11);
        free(rgba);
        if(FAILED(hr) || !g_d3d9.textures[id].tex11) return;
        ID3D11Device_CreateShaderResourceView(g_d3d11.dev,(ID3D11Resource*)g_d3d9.textures[id].tex11,NULL,&g_d3d9.textures[id].srv11);
        return;
    }
    if(id==0||id>=PEX_D3D_MAX_TEXTURES||!g_d3d9.dev)return;
    if(g_d3d9.textures[id].tex){ if (g_d3d9.cache.texture0 == (IDirect3DBaseTexture9*)g_d3d9.textures[id].tex) g_d3d9.cache.texture_valid = 0; IDirect3DTexture9_Release(g_d3d9.textures[id].tex); g_d3d9.textures[id].tex=NULL; }
    if(FAILED(IDirect3DDevice9_CreateTexture(g_d3d9.dev,w,h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&g_d3d9.textures[id].tex,NULL))) return;
    if(pixels){ D3DLOCKED_RECT lr; if(SUCCEEDED(IDirect3DTexture9_LockRect(g_d3d9.textures[id].tex,0,&lr,NULL,0))){ const unsigned char *src=(const unsigned char*)pixels; for(int y=0;y<h;y++){ DWORD *dst=(DWORD*)((unsigned char*)lr.pBits + y*lr.Pitch); for(int x=0;x<w;x++){ const unsigned char *p=&src[(y*w+x)*4]; dst[x]=D3DCOLOR_ARGB(p[3],p[0],p[1],p[2]); } } IDirect3DTexture9_UnlockRect(g_d3d9.textures[id].tex,0); } }
}
static void pex_glTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff, GLsizei w, GLsizei h, GLenum format, GLenum type, const GLvoid *pixels) {
    if (!pex_using_gpu_backend()) { glTexSubImage2D(target,level,xoff,yoff,w,h,format,type,pixels); return; }
    pex_gpu_flush_immediate_stream();
    (void)target; (void)level;
    if (!pixels || format != GL_RGBA || type != GL_UNSIGNED_BYTE || w <= 0 || h <= 0) return;
    unsigned int id = g_d3d9.bound_texture;
    if (id == 0 || id >= PEX_D3D_MAX_TEXTURES) return;
    PexD3DTexture *t = &g_d3d9.textures[id];
    if (xoff < 0 || yoff < 0 || xoff + w > t->w || yoff + h > t->h) return;
    if (pex_using_d3d11()) {
        if (!g_d3d11.ctx || !t->tex11) return;
        D3D11_BOX box;
        box.left=(UINT)xoff; box.top=(UINT)yoff; box.front=0;
        box.right=(UINT)(xoff+w); box.bottom=(UINT)(yoff+h); box.back=1;
        ID3D11DeviceContext_UpdateSubresource(g_d3d11.ctx,(ID3D11Resource*)t->tex11,0,&box,pixels,(UINT)w*4u,0);
        return;
    }
    if (!t->tex) return;
    RECT rect; rect.left=xoff; rect.top=yoff; rect.right=xoff+w; rect.bottom=yoff+h;
    D3DLOCKED_RECT lr;
    if (SUCCEEDED(IDirect3DTexture9_LockRect(t->tex,0,&lr,&rect,0))) {
        const unsigned char *src=(const unsigned char*)pixels;
        for (int y=0; y<h; ++y) {
            DWORD *dst=(DWORD*)((unsigned char*)lr.pBits + y*lr.Pitch);
            for (int x=0; x<w; ++x) {
                const unsigned char *px=&src[(y*w+x)*4];
                dst[x]=D3DCOLOR_ARGB(px[3],px[0],px[1],px[2]);
            }
        }
        IDirect3DTexture9_UnlockRect(t->tex,0);
    }
}
static void pex_glDeleteTextures(GLsizei n, const GLuint *ids) { if (!pex_using_gpu_backend()) { glDeleteTextures(n,ids); return; } pex_gpu_flush_immediate_stream(); for(int i=0;i<n;i++){ unsigned int id=ids[i]; if(id<PEX_D3D_MAX_TEXTURES){ if(g_d3d9.textures[id].tex){ if (g_d3d9.cache.texture0 == (IDirect3DBaseTexture9*)g_d3d9.textures[id].tex) g_d3d9.cache.texture_valid = 0; IDirect3DTexture9_Release(g_d3d9.textures[id].tex); g_d3d9.textures[id].tex=NULL; } if(g_d3d9.textures[id].srv11){ ID3D11ShaderResourceView_Release(g_d3d9.textures[id].srv11); g_d3d9.textures[id].srv11=NULL; } if(g_d3d9.textures[id].tex11){ ID3D11Texture2D_Release(g_d3d9.textures[id].tex11); g_d3d9.textures[id].tex11=NULL; } } } }
static void pex_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff, GLint x, GLint y, GLsizei w, GLsizei h) {
    if (!pex_using_gpu_backend()) { glCopyTexSubImage2D(target,level,xoff,yoff,x,y,w,h); return; }
    pex_gpu_flush_immediate_stream();
    (void)target; (void)level;
    unsigned int id = g_d3d9.bound_texture;
    if (id == 0 || id >= PEX_D3D_MAX_TEXTURES || w <= 0 || h <= 0) return;
    int src_y = g_render_h - y - h;
    if (src_y < 0) src_y = 0;
    if (pex_using_d3d11()) {
        if (!g_d3d11.swap || !g_d3d11.ctx || !g_d3d9.textures[id].tex11) return;
        ID3D11Texture2D *back = NULL;
        if (FAILED(IDXGISwapChain_GetBuffer(g_d3d11.swap, 0, &IID_ID3D11Texture2D, (void**)&back)) || !back) return;
        D3D11_BOX box;
        box.left = (UINT)x; box.top = (UINT)src_y; box.front = 0;
        box.right = (UINT)(x + w); box.bottom = (UINT)(src_y + h); box.back = 1;
        ID3D11DeviceContext_CopySubresourceRegion(g_d3d11.ctx, (ID3D11Resource*)g_d3d9.textures[id].tex11, 0,
            (UINT)xoff, (UINT)yoff, 0, (ID3D11Resource*)back, 0, &box);
        ID3D11Texture2D_Release(back);
        return;
    }
    if (!g_d3d9.dev || !g_d3d9.textures[id].tex) return;
    IDirect3DSurface9 *rt = NULL;
    if (FAILED(IDirect3DDevice9_GetRenderTarget(g_d3d9.dev, 0, &rt)) || !rt) return;
    D3DSURFACE_DESC desc;
    if (FAILED(IDirect3DSurface9_GetDesc(rt, &desc))) { IDirect3DSurface9_Release(rt); return; }
    IDirect3DSurface9 *sys = NULL;
    if (FAILED(IDirect3DDevice9_CreateOffscreenPlainSurface(g_d3d9.dev, desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &sys, NULL)) || !sys) {
        IDirect3DSurface9_Release(rt); return;
    }
    if (SUCCEEDED(IDirect3DDevice9_GetRenderTargetData(g_d3d9.dev, rt, sys))) {
        D3DLOCKED_RECT sr, dr;
        RECT src_rect; src_rect.left=x; src_rect.top=src_y; src_rect.right=x+w; src_rect.bottom=src_y+h;
        RECT dst_rect; dst_rect.left=xoff; dst_rect.top=yoff; dst_rect.right=xoff+w; dst_rect.bottom=yoff+h;
        int sr_locked = 0;
        int dr_locked = 0;
        if (SUCCEEDED(IDirect3DSurface9_LockRect(sys, &sr, &src_rect, D3DLOCK_READONLY))) {
            sr_locked = 1;
            if (SUCCEEDED(IDirect3DTexture9_LockRect(g_d3d9.textures[id].tex, 0, &dr, &dst_rect, 0))) {
                dr_locked = 1;
                for (int row = 0; row < h; row++) {
                    memcpy((unsigned char*)dr.pBits + row * dr.Pitch, (unsigned char*)sr.pBits + row * sr.Pitch, (size_t)w * 4u);
                }
            }
        }
        if (dr_locked) IDirect3DTexture9_UnlockRect(g_d3d9.textures[id].tex, 0);
        if (sr_locked) IDirect3DSurface9_UnlockRect(sys);
    }
    IDirect3DSurface9_Release(sys);
    IDirect3DSurface9_Release(rt);
}
