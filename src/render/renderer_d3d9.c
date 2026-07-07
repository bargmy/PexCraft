/* Native Direct3D 9 renderer backend for Pexcraft.
   This file is intentionally not an OpenGL wrapper: it exposes a backend-neutral
   mesh/texture API and draws through D3D9 vertex/index buffers. */

#include <windows.h>
#include <d3d9.h>

#ifndef PEX_D3D9_FVF
#define PEX_D3D9_FVF (D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_DIFFUSE)
#endif

typedef struct PexD3D9Vertex {
    float x, y, z;
    /* D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 requires
       position, then diffuse color, then texcoord.  The previous order
       was x/y/z/u/v/color, so Direct3D read UV bits as the diffuse color
       and color bits as UVs.  That caused the dark/green untextured world. */
    DWORD color;
    float u, v;
} PexD3D9Vertex;

typedef struct PexD3D9Mesh {
    IDirect3DVertexBuffer9 *vb;
    IDirect3DIndexBuffer9 *ib;
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t vertex_capacity;
    uint32_t index_capacity;
    int dynamic;
} PexD3D9Mesh;

typedef struct PexD3D9TextureNative {
    IDirect3DTexture9 *tex;
    int width, height;
    int repeat;
} PexD3D9TextureNative;

typedef struct PexD3D9Native {
    HWND hwnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *dev;
    D3DPRESENT_PARAMETERS pp;
    int active;
    int borrowed_device;
    int width, height;
    int scene_open;

    PexD3D9Mesh meshes[PEX_RENDERER_MAX_MESHES];
    PexD3D9TextureNative textures[PEX_RENDERER_MAX_TEXTURES];
    uint32_t next_mesh;
    uint32_t next_texture;

    IDirect3DVertexBuffer9 *dynamic_vb;
    IDirect3DIndexBuffer9 *dynamic_ib;
    uint32_t dynamic_v_cap;
    uint32_t dynamic_i_cap;

    PexTextureHandle last_texture;
    int last_texture_enabled;
    int last_blend;
    int last_depth;
    int last_depth_write;
    int last_alpha_test;
    int cache_valid;

    PexRendererStats stats;
} PexD3D9Native;

static PexD3D9Native g_pxr_d3d9;

static void pxr_d3d9_mat_mul(float out[16], const float a[16], const float b[16]) {
    float r[16];
    for (int c = 0; c < 4; ++c) {
        for (int row = 0; row < 4; ++row) {
            r[c*4 + row] = a[0*4 + row] * b[c*4 + 0] +
                           a[1*4 + row] * b[c*4 + 1] +
                           a[2*4 + row] * b[c*4 + 2] +
                           a[3*4 + row] * b[c*4 + 3];
        }
    }
    memcpy(out, r, sizeof(r));
}

static void d3d9_depth_remap_gl_to_d3d(float m[16]) {
    float remap[16];
    memset(remap, 0, sizeof(remap));
    remap[0] = remap[5] = remap[15] = 1.0f;
    remap[10] = 0.5f;
    remap[14] = 0.5f;
    float out[16];
    pxr_d3d9_mat_mul(out, remap, m);
    memcpy(m, out, sizeof(out));
}

static D3DMATRIX d3d9_matrix_from_gl(const float m[16]) {
    D3DMATRIX d;
    d._11=m[0];  d._12=m[1];  d._13=m[2];  d._14=m[3];
    d._21=m[4];  d._22=m[5];  d._23=m[6];  d._24=m[7];
    d._31=m[8];  d._32=m[9];  d._33=m[10]; d._34=m[11];
    d._41=m[12]; d._42=m[13]; d._43=m[14]; d._44=m[15];
    return d;
}

static DWORD d3d9_pack_argb(uint32_t rgba) {
    /* PexVertex.color is memory-RGBA: r | g<<8 | b<<16 | a<<24. */
    DWORD r = rgba & 255;
    DWORD g = (rgba >> 8) & 255;
    DWORD b = (rgba >> 16) & 255;
    DWORD a = (rgba >> 24) & 255;
    return (a << 24) | (r << 16) | (g << 8) | b;
}

static void pxr_d3d9_release_mesh(PexD3D9Mesh *m) {
    if (!m) return;
    if (m->vb) { IDirect3DVertexBuffer9_Release(m->vb); m->vb = NULL; }
    if (m->ib) { IDirect3DIndexBuffer9_Release(m->ib); m->ib = NULL; }
    memset(m, 0, sizeof(*m));
}

static void pxr_d3d9_release_dynamic(void) {
    if (g_pxr_d3d9.dynamic_vb) { IDirect3DVertexBuffer9_Release(g_pxr_d3d9.dynamic_vb); g_pxr_d3d9.dynamic_vb = NULL; }
    if (g_pxr_d3d9.dynamic_ib) { IDirect3DIndexBuffer9_Release(g_pxr_d3d9.dynamic_ib); g_pxr_d3d9.dynamic_ib = NULL; }
    g_pxr_d3d9.dynamic_v_cap = 0;
    g_pxr_d3d9.dynamic_i_cap = 0;
}

static void d3d9_set_initial_states(void) {
    IDirect3DDevice9 *d = g_pxr_d3d9.dev;
    if (!d) return;
    IDirect3DDevice9_SetFVF(d, PEX_D3D9_FVF);
    IDirect3DDevice9_SetRenderState(d, D3DRS_LIGHTING, FALSE);
    IDirect3DDevice9_SetRenderState(d, D3DRS_CULLMODE, D3DCULL_NONE);
    IDirect3DDevice9_SetRenderState(d, D3DRS_ZENABLE, TRUE);
    IDirect3DDevice9_SetRenderState(d, D3DRS_ZWRITEENABLE, TRUE);
    IDirect3DDevice9_SetRenderState(d, D3DRS_ALPHATESTENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(d, D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    IDirect3DDevice9_SetRenderState(d, D3DRS_ALPHAREF, 25);
    IDirect3DDevice9_SetRenderState(d, D3DRS_ALPHABLENDENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(d, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    IDirect3DDevice9_SetRenderState(d, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    IDirect3DDevice9_SetRenderState(d, D3DRS_FOGENABLE, FALSE);
    IDirect3DDevice9_SetSamplerState(d, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    IDirect3DDevice9_SetSamplerState(d, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    IDirect3DDevice9_SetSamplerState(d, 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
    IDirect3DDevice9_SetTextureStageState(d, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    IDirect3DDevice9_SetTextureStageState(d, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    IDirect3DDevice9_SetTextureStageState(d, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    IDirect3DDevice9_SetTextureStageState(d, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    IDirect3DDevice9_SetTextureStageState(d, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    IDirect3DDevice9_SetTextureStageState(d, 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g_pxr_d3d9.cache_valid = 0;
}

static int pxr_d3d9_reset_device(int w, int h) {
    if (!g_pxr_d3d9.dev) return 0;
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    pxr_d3d9_release_dynamic();
    g_pxr_d3d9.pp.BackBufferWidth = (UINT)w;
    g_pxr_d3d9.pp.BackBufferHeight = (UINT)h;
    HRESULT hr = IDirect3DDevice9_Reset(g_pxr_d3d9.dev, &g_pxr_d3d9.pp);
    if (FAILED(hr)) return 0;
    g_pxr_d3d9.width = w;
    g_pxr_d3d9.height = h;
    d3d9_set_initial_states();
    return 1;
}

static int d3d9_init(void *window_handle, int width, int height) {
    memset(&g_pxr_d3d9, 0, sizeof(g_pxr_d3d9));
    HWND hwnd = (HWND)window_handle;
    if (!hwnd) return 0;
    g_pxr_d3d9.hwnd = hwnd;
    g_pxr_d3d9.width = width > 0 ? width : 640;
    g_pxr_d3d9.height = height > 0 ? height : 480;
    g_pxr_d3d9.next_mesh = 1;
    g_pxr_d3d9.next_texture = 1;

    g_pxr_d3d9.d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_pxr_d3d9.d3d) return 0;

    D3DPRESENT_PARAMETERS pp;
    memset(&pp, 0, sizeof(pp));
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;
    pp.BackBufferWidth = (UINT)g_pxr_d3d9.width;
    pp.BackBufferHeight = (UINT)g_pxr_d3d9.height;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    pp.hDeviceWindow = hwnd;
    g_pxr_d3d9.pp = pp;

    DWORD flags = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE;
    HRESULT hr = IDirect3D9_CreateDevice(g_pxr_d3d9.d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, flags, &g_pxr_d3d9.pp, &g_pxr_d3d9.dev);
    if (FAILED(hr)) {
        flags = D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE;
        hr = IDirect3D9_CreateDevice(g_pxr_d3d9.d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, flags, &g_pxr_d3d9.pp, &g_pxr_d3d9.dev);
    }
    if (FAILED(hr) || !g_pxr_d3d9.dev) {
        if (g_pxr_d3d9.d3d) IDirect3D9_Release(g_pxr_d3d9.d3d);
        memset(&g_pxr_d3d9, 0, sizeof(g_pxr_d3d9));
        return 0;
    }
    d3d9_set_initial_states();
    g_pxr_d3d9.active = 1;
    return 1;
}

static void d3d9_shutdown(void) {
    for (uint32_t i = 1; i < PEX_RENDERER_MAX_MESHES; ++i) pxr_d3d9_release_mesh(&g_pxr_d3d9.meshes[i]);
    for (uint32_t i = 1; i < PEX_RENDERER_MAX_TEXTURES; ++i) {
        if (g_pxr_d3d9.textures[i].tex) { IDirect3DTexture9_Release(g_pxr_d3d9.textures[i].tex); g_pxr_d3d9.textures[i].tex = NULL; }
    }
    pxr_d3d9_release_dynamic();
    if (g_pxr_d3d9.dev) { IDirect3DDevice9_Release(g_pxr_d3d9.dev); g_pxr_d3d9.dev = NULL; }
    if (!g_pxr_d3d9.borrowed_device && g_pxr_d3d9.d3d) { IDirect3D9_Release(g_pxr_d3d9.d3d); g_pxr_d3d9.d3d = NULL; }
    memset(&g_pxr_d3d9, 0, sizeof(g_pxr_d3d9));
}

static void d3d9_resize(int width, int height) {
    if (!g_pxr_d3d9.active) return;
    pxr_d3d9_reset_device(width, height);
}

static int d3d9_begin_frame(float r, float g, float b, float a) {
    if (!g_pxr_d3d9.dev) return 0;
    HRESULT coop = IDirect3DDevice9_TestCooperativeLevel(g_pxr_d3d9.dev);
    if (coop == D3DERR_DEVICELOST) return 0;
    if (coop == D3DERR_DEVICENOTRESET) {
        if (!pxr_d3d9_reset_device(g_pxr_d3d9.width, g_pxr_d3d9.height)) return 0;
    }
    g_pxr_d3d9.stats.draw_calls = 0;
    g_pxr_d3d9.stats.triangles = 0;
    g_pxr_d3d9.stats.buffer_uploads = 0;
    DWORD clear = D3DCOLOR_COLORVALUE(r, g, b, a);
    IDirect3DDevice9_Clear(g_pxr_d3d9.dev, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear, 1.0f, 0);
    if (FAILED(IDirect3DDevice9_BeginScene(g_pxr_d3d9.dev))) return 0;
    g_pxr_d3d9.scene_open = 1;
    return 1;
}

static void d3d9_end_frame(void) {
    if (!g_pxr_d3d9.dev) return;
    if (g_pxr_d3d9.scene_open) {
        IDirect3DDevice9_EndScene(g_pxr_d3d9.dev);
        g_pxr_d3d9.scene_open = 0;
    }
    IDirect3DDevice9_Present(g_pxr_d3d9.dev, NULL, NULL, NULL, NULL);
}

static PexTextureHandle d3d9_create_texture(const PexTextureDesc *desc) {
    if (!g_pxr_d3d9.dev || !desc || desc->width <= 0 || desc->height <= 0) return 0;
    uint32_t id = g_pxr_d3d9.next_texture++;
    if (id >= PEX_RENDERER_MAX_TEXTURES) return 0;
    PexD3D9TextureNative *t = &g_pxr_d3d9.textures[id];
    memset(t, 0, sizeof(*t));
    if (FAILED(IDirect3DDevice9_CreateTexture(g_pxr_d3d9.dev, (UINT)desc->width, (UINT)desc->height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &t->tex, NULL))) return 0;
    if (desc->rgba_pixels) {
        D3DLOCKED_RECT lr;
        if (SUCCEEDED(IDirect3DTexture9_LockRect(t->tex, 0, &lr, NULL, 0))) {
            for (int y = 0; y < desc->height; ++y) {
                DWORD *dst = (DWORD*)((unsigned char*)lr.pBits + y * lr.Pitch);
                for (int x = 0; x < desc->width; ++x) {
                    const unsigned char *src = ((const unsigned char*)desc->rgba_pixels) + ((y * desc->width + x) * 4);
                    dst[x] = ((DWORD)src[3] << 24) | ((DWORD)src[0] << 16) | ((DWORD)src[1] << 8) | (DWORD)src[2];
                }
            }
            IDirect3DTexture9_UnlockRect(t->tex, 0);
        }
    }
    t->width = desc->width;
    t->height = desc->height;
    t->repeat = desc->repeat;
    g_pxr_d3d9.stats.texture_uploads++;
    return id;
}

static void d3d9_destroy_texture(PexTextureHandle handle) {
    if (handle == 0 || handle >= PEX_RENDERER_MAX_TEXTURES) return;
    if (g_pxr_d3d9.textures[handle].tex) IDirect3DTexture9_Release(g_pxr_d3d9.textures[handle].tex);
    memset(&g_pxr_d3d9.textures[handle], 0, sizeof(g_pxr_d3d9.textures[handle]));
}

static uint32_t pxr_next_pow2_u32(uint32_t v, uint32_t minv) {
    uint32_t c = minv;
    while (c < v && c < 0x80000000u) c <<= 1;
    return c < v ? v : c;
}

static int d3d9_ensure_mesh_capacity(PexD3D9Mesh *m, const PexMesh *mesh) {
    if (!g_pxr_d3d9.dev || !m || !mesh) return 0;
    int recreate = 0;
    if (!m->vb || !m->ib) recreate = 1;
    if (m->dynamic != mesh->dynamic) recreate = 1;
    if (m->vertex_capacity < mesh->vertex_count || m->index_capacity < mesh->index_count) recreate = 1;
    if (!recreate) return 1;

    if (m->vb) { IDirect3DVertexBuffer9_Release(m->vb); m->vb = NULL; }
    if (m->ib) { IDirect3DIndexBuffer9_Release(m->ib); m->ib = NULL; }

    m->dynamic = mesh->dynamic;
    m->vertex_capacity = pxr_next_pow2_u32(mesh->vertex_count, 4096u);
    m->index_capacity = pxr_next_pow2_u32(mesh->index_count, 6144u);
    DWORD usage = mesh->dynamic ? (D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY) : D3DUSAGE_WRITEONLY;
    D3DPOOL pool = mesh->dynamic ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;
    if (FAILED(IDirect3DDevice9_CreateVertexBuffer(g_pxr_d3d9.dev, m->vertex_capacity * sizeof(PexD3D9Vertex), usage, PEX_D3D9_FVF, pool, &m->vb, NULL))) {
        pxr_d3d9_release_mesh(m);
        return 0;
    }
    if (FAILED(IDirect3DDevice9_CreateIndexBuffer(g_pxr_d3d9.dev, m->index_capacity * sizeof(uint32_t), usage, D3DFMT_INDEX32, pool, &m->ib, NULL))) {
        pxr_d3d9_release_mesh(m);
        return 0;
    }
    g_pxr_d3d9.stats.buffer_uploads++;
    return 1;
}

static int d3d9_upload_mesh_to_slot(uint32_t id, const PexMesh *mesh) {
    if (!g_pxr_d3d9.dev || !mesh || !mesh->vertices || !mesh->indices || mesh->vertex_count == 0 || mesh->index_count == 0) return 0;
    PexD3D9Mesh *m = &g_pxr_d3d9.meshes[id];
    if (!d3d9_ensure_mesh_capacity(m, mesh)) return 0;
    m->vertex_count = mesh->vertex_count;
    m->index_count = mesh->index_count;

    PexD3D9Vertex *vdst = NULL;
    DWORD lock_flags = mesh->dynamic ? D3DLOCK_DISCARD : 0;
    if (SUCCEEDED(IDirect3DVertexBuffer9_Lock(m->vb, 0, mesh->vertex_count * sizeof(PexD3D9Vertex), (void**)&vdst, lock_flags))) {
        for (uint32_t i = 0; i < mesh->vertex_count; ++i) {
            vdst[i].x = mesh->vertices[i].x;
            vdst[i].y = mesh->vertices[i].y;
            vdst[i].z = mesh->vertices[i].z;
            vdst[i].color = d3d9_pack_argb(mesh->vertices[i].color);
            vdst[i].u = mesh->vertices[i].u;
            vdst[i].v = mesh->vertices[i].v;
        }
        IDirect3DVertexBuffer9_Unlock(m->vb);
    }
    uint32_t *idst = NULL;
    if (SUCCEEDED(IDirect3DIndexBuffer9_Lock(m->ib, 0, mesh->index_count * sizeof(uint32_t), (void**)&idst, lock_flags))) {
        memcpy(idst, mesh->indices, mesh->index_count * sizeof(uint32_t));
        IDirect3DIndexBuffer9_Unlock(m->ib);
    }
    return 1;
}

static PexMeshHandle d3d9_upload_mesh(const PexMesh *mesh) {
    uint32_t id = g_pxr_d3d9.next_mesh++;
    if (id == 0 || id >= PEX_RENDERER_MAX_MESHES) return 0;
    if (!d3d9_upload_mesh_to_slot(id, mesh)) return 0;
    return id;
}

static int d3d9_update_mesh(PexMeshHandle handle, const PexMesh *mesh) {
    if (handle == 0 || handle >= PEX_RENDERER_MAX_MESHES) return 0;
    return d3d9_upload_mesh_to_slot(handle, mesh);
}

static void d3d9_destroy_mesh(PexMeshHandle handle) {
    if (handle == 0 || handle >= PEX_RENDERER_MAX_MESHES) return;
    pxr_d3d9_release_mesh(&g_pxr_d3d9.meshes[handle]);
}

static void pxr_d3d9_apply_state(const PexRenderState *s) {
    if (!g_pxr_d3d9.dev || !s) return;
    IDirect3DDevice9 *d = g_pxr_d3d9.dev;
    if (!g_pxr_d3d9.cache_valid || g_pxr_d3d9.last_texture_enabled != s->texture_enabled || g_pxr_d3d9.last_texture != s->texture) {
        IDirect3DBaseTexture9 *tex = NULL;
        if (s->texture_enabled && s->texture > 0 && s->texture < PEX_RENDERER_MAX_TEXTURES) tex = (IDirect3DBaseTexture9*)g_pxr_d3d9.textures[s->texture].tex;
        IDirect3DDevice9_SetTexture(d, 0, tex);
        if (tex && s->texture < PEX_RENDERER_MAX_TEXTURES) {
            DWORD addr = g_pxr_d3d9.textures[s->texture].repeat ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
            IDirect3DDevice9_SetSamplerState(d, 0, D3DSAMP_ADDRESSU, addr);
            IDirect3DDevice9_SetSamplerState(d, 0, D3DSAMP_ADDRESSV, addr);
            IDirect3DDevice9_SetTextureStageState(d, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
            IDirect3DDevice9_SetTextureStageState(d, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            IDirect3DDevice9_SetTextureStageState(d, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            IDirect3DDevice9_SetTextureStageState(d, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
            IDirect3DDevice9_SetTextureStageState(d, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            IDirect3DDevice9_SetTextureStageState(d, 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        } else {
            IDirect3DDevice9_SetTextureStageState(d, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
            IDirect3DDevice9_SetTextureStageState(d, 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
            IDirect3DDevice9_SetTextureStageState(d, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
            IDirect3DDevice9_SetTextureStageState(d, 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
        }
        g_pxr_d3d9.last_texture_enabled = s->texture_enabled;
        g_pxr_d3d9.last_texture = s->texture;
    }
    if (!g_pxr_d3d9.cache_valid || g_pxr_d3d9.last_blend != s->blend_enabled) {
        IDirect3DDevice9_SetRenderState(d, D3DRS_ALPHABLENDENABLE, s->blend_enabled ? TRUE : FALSE);
        g_pxr_d3d9.last_blend = s->blend_enabled;
    }
    if (!g_pxr_d3d9.cache_valid || g_pxr_d3d9.last_depth != s->depth_enabled) {
        IDirect3DDevice9_SetRenderState(d, D3DRS_ZENABLE, s->depth_enabled ? TRUE : FALSE);
        g_pxr_d3d9.last_depth = s->depth_enabled;
    }
    if (!g_pxr_d3d9.cache_valid || g_pxr_d3d9.last_depth_write != s->depth_write) {
        IDirect3DDevice9_SetRenderState(d, D3DRS_ZWRITEENABLE, s->depth_write ? TRUE : FALSE);
        g_pxr_d3d9.last_depth_write = s->depth_write;
    }
    if (!g_pxr_d3d9.cache_valid || g_pxr_d3d9.last_alpha_test != s->alpha_test_enabled) {
        IDirect3DDevice9_SetRenderState(d, D3DRS_ALPHATESTENABLE, s->alpha_test_enabled ? TRUE : FALSE);
        g_pxr_d3d9.last_alpha_test = s->alpha_test_enabled;
    }
    float proj_d3d[16];
    memcpy(proj_d3d, s->projection, sizeof(proj_d3d));
    d3d9_depth_remap_gl_to_d3d(proj_d3d);
    D3DMATRIX mv = d3d9_matrix_from_gl(s->modelview);
    D3DMATRIX pr = d3d9_matrix_from_gl(proj_d3d);
    D3DMATRIX world;
    memset(&world, 0, sizeof(world));
    world._11 = world._22 = world._33 = world._44 = 1.0f;
    IDirect3DDevice9_SetTransform(d, D3DTS_WORLD, &world);
    IDirect3DDevice9_SetTransform(d, D3DTS_VIEW, &mv);
    IDirect3DDevice9_SetTransform(d, D3DTS_PROJECTION, &pr);
    g_pxr_d3d9.cache_valid = 1;
}

static void d3d9_draw_mesh(PexMeshHandle handle, const PexRenderState *state) {
    if (handle == 0 || handle >= PEX_RENDERER_MAX_MESHES || !g_pxr_d3d9.dev) return;
    PexD3D9Mesh *m = &g_pxr_d3d9.meshes[handle];
    if (!m->vb || !m->ib || m->index_count < 3) return;
    pxr_d3d9_apply_state(state);
    IDirect3DDevice9_SetFVF(g_pxr_d3d9.dev, PEX_D3D9_FVF);
    IDirect3DDevice9_SetStreamSource(g_pxr_d3d9.dev, 0, m->vb, 0, sizeof(PexD3D9Vertex));
    IDirect3DDevice9_SetIndices(g_pxr_d3d9.dev, m->ib);
    IDirect3DDevice9_DrawIndexedPrimitive(g_pxr_d3d9.dev, D3DPT_TRIANGLELIST, 0, 0, m->vertex_count, 0, m->index_count / 3);
    g_pxr_d3d9.stats.draw_calls++;
    g_pxr_d3d9.stats.triangles += m->index_count / 3;
}

static int pxr_d3d9_ensure_dynamic(uint32_t vcount, uint32_t icount) {
    if (!g_pxr_d3d9.dev) return 0;
    if (!g_pxr_d3d9.dynamic_vb || g_pxr_d3d9.dynamic_v_cap < vcount) {
        if (g_pxr_d3d9.dynamic_vb) IDirect3DVertexBuffer9_Release(g_pxr_d3d9.dynamic_vb);
        uint32_t cap = g_pxr_d3d9.dynamic_v_cap ? g_pxr_d3d9.dynamic_v_cap * 2 : 8192;
        while (cap < vcount) cap *= 2;
        if (FAILED(IDirect3DDevice9_CreateVertexBuffer(g_pxr_d3d9.dev, cap * sizeof(PexD3D9Vertex), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, PEX_D3D9_FVF, D3DPOOL_DEFAULT, &g_pxr_d3d9.dynamic_vb, NULL))) return 0;
        g_pxr_d3d9.dynamic_v_cap = cap;
    }
    if (!g_pxr_d3d9.dynamic_ib || g_pxr_d3d9.dynamic_i_cap < icount) {
        if (g_pxr_d3d9.dynamic_ib) IDirect3DIndexBuffer9_Release(g_pxr_d3d9.dynamic_ib);
        uint32_t cap = g_pxr_d3d9.dynamic_i_cap ? g_pxr_d3d9.dynamic_i_cap * 2 : 12288;
        while (cap < icount) cap *= 2;
        if (FAILED(IDirect3DDevice9_CreateIndexBuffer(g_pxr_d3d9.dev, cap * sizeof(uint32_t), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &g_pxr_d3d9.dynamic_ib, NULL))) return 0;
        g_pxr_d3d9.dynamic_i_cap = cap;
    }
    return 1;
}

static void d3d9_draw_dynamic(const PexMesh *mesh, const PexRenderState *state) {
    if (!mesh || !mesh->vertices || !mesh->indices || mesh->vertex_count == 0 || mesh->index_count < 3) return;
    if (!pxr_d3d9_ensure_dynamic(mesh->vertex_count, mesh->index_count)) return;
    PexD3D9Vertex *vdst = NULL;
    uint32_t *idst = NULL;
    if (FAILED(IDirect3DVertexBuffer9_Lock(g_pxr_d3d9.dynamic_vb, 0, mesh->vertex_count * sizeof(PexD3D9Vertex), (void**)&vdst, D3DLOCK_DISCARD))) return;
    for (uint32_t i = 0; i < mesh->vertex_count; ++i) {
        vdst[i].x = mesh->vertices[i].x; vdst[i].y = mesh->vertices[i].y; vdst[i].z = mesh->vertices[i].z;
        vdst[i].color = d3d9_pack_argb(mesh->vertices[i].color); vdst[i].u = mesh->vertices[i].u; vdst[i].v = mesh->vertices[i].v;
    }
    IDirect3DVertexBuffer9_Unlock(g_pxr_d3d9.dynamic_vb);
    if (FAILED(IDirect3DIndexBuffer9_Lock(g_pxr_d3d9.dynamic_ib, 0, mesh->index_count * sizeof(uint32_t), (void**)&idst, D3DLOCK_DISCARD))) return;
    memcpy(idst, mesh->indices, mesh->index_count * sizeof(uint32_t));
    IDirect3DIndexBuffer9_Unlock(g_pxr_d3d9.dynamic_ib);
    pxr_d3d9_apply_state(state);
    IDirect3DDevice9_SetFVF(g_pxr_d3d9.dev, PEX_D3D9_FVF);
    IDirect3DDevice9_SetStreamSource(g_pxr_d3d9.dev, 0, g_pxr_d3d9.dynamic_vb, 0, sizeof(PexD3D9Vertex));
    IDirect3DDevice9_SetIndices(g_pxr_d3d9.dev, g_pxr_d3d9.dynamic_ib);
    IDirect3DDevice9_DrawIndexedPrimitive(g_pxr_d3d9.dev, D3DPT_TRIANGLELIST, 0, 0, mesh->vertex_count, 0, mesh->index_count / 3);
    g_pxr_d3d9.stats.draw_calls++;
    g_pxr_d3d9.stats.triangles += mesh->index_count / 3;
}


static int renderer_d3d9_attach_device(void *dev, int width, int height) {
    if (!dev) return 0;
    d3d9_shutdown();
    memset(&g_pxr_d3d9, 0, sizeof(g_pxr_d3d9));
    g_pxr_d3d9.dev = (IDirect3DDevice9*)dev;
    IDirect3DDevice9_AddRef(g_pxr_d3d9.dev);
    g_pxr_d3d9.borrowed_device = 1;
    g_pxr_d3d9.active = 1;
    g_pxr_d3d9.width = width > 0 ? width : 640;
    g_pxr_d3d9.height = height > 0 ? height : 480;
    g_pxr_d3d9.next_mesh = 1;
    g_pxr_d3d9.next_texture = 1;
    d3d9_set_initial_states();
    return 1;
}

static PexRendererStats d3d9_get_stats(void) { return g_pxr_d3d9.stats; }

static PexRendererBackend g_renderer_d3d9_native = {
    "Direct3D 9 Native",
    d3d9_init,
    d3d9_shutdown,
    d3d9_resize,
    d3d9_begin_frame,
    d3d9_end_frame,
    d3d9_create_texture,
    d3d9_destroy_texture,
    d3d9_upload_mesh,
    d3d9_update_mesh,
    d3d9_destroy_mesh,
    d3d9_draw_mesh,
    d3d9_draw_dynamic,
    d3d9_get_stats
};

static PexRendererBackend *renderer_d3d9_get_backend(void) { return &g_renderer_d3d9_native; }
