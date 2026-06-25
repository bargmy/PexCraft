/* Native Direct3D 11 renderer backend for Pexcraft.
   This backend consumes backend-neutral indexed meshes directly and draws with
   ID3D11Device/ID3D11DeviceContext.  It does not emulate OpenGL. */

#include <windows.h>
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
#ifndef DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
#define DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING 0x00000800U
#endif
#ifndef DXGI_SWAP_EFFECT_FLIP_DISCARD
#define DXGI_SWAP_EFFECT_FLIP_DISCARD ((DXGI_SWAP_EFFECT)4)
#endif

typedef struct PexD3D11Mesh {
    ID3D11Buffer *vb;
    ID3D11Buffer *ib;
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t vertex_capacity;
    uint32_t index_capacity;
    int dynamic;
} PexD3D11Mesh;

typedef struct PexD3D11TextureNative {
    ID3D11Texture2D *tex;
    ID3D11ShaderResourceView *srv;
    int width, height;
    int repeat;
} PexD3D11TextureNative;

typedef struct PexD3D11CBNative {
    float modelview[16];
    float projection[16];
    float fog_color[4];
    float fog_params[4];
} PexD3D11CBNative;

typedef struct PexD3D11Native {
    HWND hwnd;
    IDXGISwapChain *swap;
    ID3D11Device *dev;
    ID3D11DeviceContext *ctx;
    ID3D11RenderTargetView *rtv;
    ID3D11Texture2D *depth_tex;
    ID3D11DepthStencilView *dsv;
    ID3D11VertexShader *vs;
    ID3D11PixelShader *ps;
    ID3D11InputLayout *input_layout;
    ID3D11Buffer *constant_buffer;
    ID3D11SamplerState *sampler_wrap;
    ID3D11SamplerState *sampler_clamp;
    ID3D11BlendState *blend_off;
    ID3D11BlendState *blend_alpha;
    ID3D11DepthStencilState *depth_on_write;
    ID3D11DepthStencilState *depth_on_nowrite;
    ID3D11DepthStencilState *depth_off;
    ID3D11RasterizerState *rast_nocull;

    PexD3D11Mesh meshes[PEX_RENDERER_MAX_MESHES];
    PexD3D11TextureNative textures[PEX_RENDERER_MAX_TEXTURES];
    uint32_t next_mesh;
    uint32_t next_texture;

    ID3D11Buffer *dynamic_vb;
    ID3D11Buffer *dynamic_ib;
    uint32_t dynamic_v_cap;
    uint32_t dynamic_i_cap;

    int active;
    int borrowed_device;
    int allow_tearing;
    UINT swap_flags;
    int width, height;
    PexRendererStats stats;

    PexTextureHandle last_texture;
    int last_texture_enabled;
    int last_blend;
    int last_depth;
    int last_depth_write;
    int last_alpha_test;
    int cache_valid;
} PexD3D11Native;

static PexD3D11Native g_pxr_d3d11;

/* Terrain section meshes can be rebuilt while the GPU is still consuming the
   previous buffers.  Releasing/recreating those buffers immediately on the
   render thread is a common D3D11 hitch source.  Keep old buffers alive for a
   few frames so the driver does not have to synchronize the CPU with in-flight
   draws during mesh swaps. */
#define PXR_D3D11_DEFERRED_MESH_RELEASE_SLOTS 4096
#define PXR_D3D11_DEFERRED_MESH_RELEASE_FRAMES 6u
typedef struct PexD3D11DeferredMeshRelease {
    PexD3D11Mesh mesh;
    uint32_t frame;
} PexD3D11DeferredMeshRelease;
static PexD3D11DeferredMeshRelease g_pxr_d3d11_deferred_mesh_release[PXR_D3D11_DEFERRED_MESH_RELEASE_SLOTS];
static uint32_t g_pxr_d3d11_frame_serial = 1;

static void d3d11_release_failed_device(IDXGISwapChain **swap, ID3D11Device **dev, ID3D11DeviceContext **ctx) {
    if (ctx && *ctx) { ID3D11DeviceContext_Release(*ctx); *ctx = NULL; }
    if (swap && *swap) { IDXGISwapChain_Release(*swap); *swap = NULL; }
    if (dev && *dev) { ID3D11Device_Release(*dev); *dev = NULL; }
}

static HRESULT d3d11_create_swap_chain(DXGI_SWAP_CHAIN_DESC *base_desc,
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
            }

            hr = D3D11CreateDeviceAndSwapChain(NULL, drivers[d], NULL, 0,
                                               levels, level_count, D3D11_SDK_VERSION,
                                               &desc, &g_pxr_d3d11.swap, &g_pxr_d3d11.dev,
                                               got_level, &g_pxr_d3d11.ctx);
            if (SUCCEEDED(hr)) {
                g_pxr_d3d11.allow_tearing = (attempt == 0);
                g_pxr_d3d11.swap_flags = desc.Flags;
                return hr;
            }
            d3d11_release_failed_device(&g_pxr_d3d11.swap, &g_pxr_d3d11.dev, &g_pxr_d3d11.ctx);
        }
    }
    return hr;
}

static void pxr_d3d11_mat_mul(float out[16], const float a[16], const float b[16]) {
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

static void d3d11_depth_remap_gl_to_d3d(float m[16]) {
    float remap[16];
    memset(remap, 0, sizeof(remap));
    remap[0] = remap[5] = remap[15] = 1.0f;
    remap[10] = 0.5f;
    remap[14] = 0.5f;
    float out[16];
    pxr_d3d11_mat_mul(out, remap, m);
    memcpy(m, out, sizeof(out));
}

static void d3d11_matrix_from_gl(float out[16], const float m[16]) {
    /* The engine matrices are OpenGL column-major/column-vector.  HLSL uses
       row-vector math here, so place the GL column-major array into row-major
       memory exactly like the D3D9 fixed-function path. */
    out[0]=m[0];   out[1]=m[1];   out[2]=m[2];   out[3]=m[3];
    out[4]=m[4];   out[5]=m[5];   out[6]=m[6];   out[7]=m[7];
    out[8]=m[8];   out[9]=m[9];   out[10]=m[10]; out[11]=m[11];
    out[12]=m[12]; out[13]=m[13]; out[14]=m[14]; out[15]=m[15];
}

static void pxr_d3d11_release_mesh(PexD3D11Mesh *m) {
    if (!m) return;
    if (m->vb) { ID3D11Buffer_Release(m->vb); m->vb = NULL; }
    if (m->ib) { ID3D11Buffer_Release(m->ib); m->ib = NULL; }
    memset(m, 0, sizeof(*m));
}

static void d3d11_release_retired_meshes(int release_all) {
    for (int n = 0; n < PXR_D3D11_DEFERRED_MESH_RELEASE_SLOTS; ++n) {
        PexD3D11DeferredMeshRelease *r = &g_pxr_d3d11_deferred_mesh_release[n];
        if (!r->mesh.vb && !r->mesh.ib) continue;
        if (!release_all && (uint32_t)(g_pxr_d3d11_frame_serial - r->frame) < PXR_D3D11_DEFERRED_MESH_RELEASE_FRAMES) continue;
        pxr_d3d11_release_mesh(&r->mesh);
        r->frame = 0;
    }
}

static void d3d11_retire_mesh_later(PexD3D11Mesh *m) {
    if (!m || (!m->vb && !m->ib)) return;
    for (int n = 0; n < PXR_D3D11_DEFERRED_MESH_RELEASE_SLOTS; ++n) {
        PexD3D11DeferredMeshRelease *r = &g_pxr_d3d11_deferred_mesh_release[n];
        if (!r->mesh.vb && !r->mesh.ib) {
            r->mesh = *m;
            r->frame = g_pxr_d3d11_frame_serial;
            memset(m, 0, sizeof(*m));
            return;
        }
    }
    /* Extremely unlikely fallback.  Better to release one stale slot than grow
       unbounded memory if a pathological rebuild burst fills the retire queue. */
    pxr_d3d11_release_mesh(&g_pxr_d3d11_deferred_mesh_release[0].mesh);
    g_pxr_d3d11_deferred_mesh_release[0].mesh = *m;
    g_pxr_d3d11_deferred_mesh_release[0].frame = g_pxr_d3d11_frame_serial;
    memset(m, 0, sizeof(*m));
}

/* Worker-thread friendly D3D11 mesh build.  This uses ID3D11Device only, not the
   immediate context, so the expensive terrain-section buffer creation/upload can
   happen outside the render thread.  The render thread later adopts the finished
   buffers with a pointer swap. */
static int d3d11_prebuild_mesh(const PexMesh *mesh, PexD3D11Mesh *out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!out || !g_pxr_d3d11.dev || !mesh || !mesh->vertices || !mesh->indices || mesh->vertex_count == 0 || mesh->index_count == 0) return 0;

    D3D11_SUBRESOURCE_DATA init;
    D3D11_BUFFER_DESC bd;
    memset(&init, 0, sizeof(init));
    memset(&bd, 0, sizeof(bd));

    bd.ByteWidth = mesh->vertex_count * sizeof(PexVertex);
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    init.pSysMem = mesh->vertices;
    if (FAILED(ID3D11Device_CreateBuffer(g_pxr_d3d11.dev, &bd, &init, &out->vb))) {
        pxr_d3d11_release_mesh(out);
        return 0;
    }

    memset(&init, 0, sizeof(init));
    memset(&bd, 0, sizeof(bd));
    bd.ByteWidth = mesh->index_count * sizeof(uint32_t);
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    init.pSysMem = mesh->indices;
    if (FAILED(ID3D11Device_CreateBuffer(g_pxr_d3d11.dev, &bd, &init, &out->ib))) {
        pxr_d3d11_release_mesh(out);
        return 0;
    }

    out->vertex_count = mesh->vertex_count;
    out->index_count = mesh->index_count;
    out->vertex_capacity = mesh->vertex_count;
    out->index_capacity = mesh->index_count;
    out->dynamic = 0;
    return 1;
}

static void d3d11_discard_mesh(PexD3D11Mesh *mesh) {
    pxr_d3d11_release_mesh(mesh);
}

static int d3d11_adopt_mesh(PexMeshHandle *slot, PexD3D11Mesh *built) {
    if (!slot || !built || !g_pxr_d3d11.dev || (!built->vb && !built->ib)) return 0;
    uint32_t id = *slot;
    if (id == 0) {
        id = g_pxr_d3d11.next_mesh++;
        if (id == 0 || id >= PEX_RENDERER_MAX_MESHES) {
            d3d11_discard_mesh(built);
            return 0;
        }
        *slot = id;
    }
    if (id >= PEX_RENDERER_MAX_MESHES) {
        d3d11_discard_mesh(built);
        return 0;
    }
    PexD3D11Mesh old = g_pxr_d3d11.meshes[id];
    g_pxr_d3d11.meshes[id] = *built;
    memset(built, 0, sizeof(*built));
    d3d11_retire_mesh_later(&old);
    g_pxr_d3d11.stats.buffer_uploads++;
    return 1;
}

static void d3d11_destroy_mesh_deferred(PexMeshHandle *slot) {
    if (!slot || *slot == 0 || *slot >= PEX_RENDERER_MAX_MESHES) return;
    PexD3D11Mesh old = g_pxr_d3d11.meshes[*slot];
    memset(&g_pxr_d3d11.meshes[*slot], 0, sizeof(g_pxr_d3d11.meshes[*slot]));
    *slot = 0;
    d3d11_retire_mesh_later(&old);
}

static void pxr_d3d11_release_views(void) {
    if (g_pxr_d3d11.rtv) { ID3D11RenderTargetView_Release(g_pxr_d3d11.rtv); g_pxr_d3d11.rtv = NULL; }
    if (g_pxr_d3d11.dsv) { ID3D11DepthStencilView_Release(g_pxr_d3d11.dsv); g_pxr_d3d11.dsv = NULL; }
    if (g_pxr_d3d11.depth_tex) { ID3D11Texture2D_Release(g_pxr_d3d11.depth_tex); g_pxr_d3d11.depth_tex = NULL; }
}

static void pxr_d3d11_release_dynamic(void) {
    if (g_pxr_d3d11.dynamic_vb) { ID3D11Buffer_Release(g_pxr_d3d11.dynamic_vb); g_pxr_d3d11.dynamic_vb = NULL; }
    if (g_pxr_d3d11.dynamic_ib) { ID3D11Buffer_Release(g_pxr_d3d11.dynamic_ib); g_pxr_d3d11.dynamic_ib = NULL; }
    g_pxr_d3d11.dynamic_v_cap = 0;
    g_pxr_d3d11.dynamic_i_cap = 0;
}

static int pxr_d3d11_compile_shader(const char *src, const char *entry, const char *target, ID3DBlob **out) {
    ID3DBlob *err = NULL;
    HRESULT hr = D3DCompile(src, strlen(src), NULL, NULL, NULL, entry, target, 0, 0, out, &err);
    if (err) ID3DBlob_Release(err);
    return SUCCEEDED(hr) && *out != NULL;
}

static int pxr_d3d11_create_pipeline(void) {
    static const char *vs_src =
        "#pragma pack_matrix(row_major)\n"
        "cbuffer C : register(b0) { row_major float4x4 uModelView; row_major float4x4 uProj; float4 uFogColor; float4 uFogParams; };\n"
        "struct VSIn { float3 pos:POSITION; float2 uv:TEXCOORD0; float4 color:COLOR0; };\n"
        "struct VSOut { float4 pos:SV_POSITION; float2 uv:TEXCOORD0; float4 color:COLOR0; float fog:TEXCOORD1; };\n"
        "VSOut main(VSIn i) { VSOut o; float4 v = mul(float4(i.pos,1), uModelView); o.pos = mul(v, uProj); o.uv=i.uv; o.color=i.color; o.fog = saturate((uFogParams.y - abs(v.z)) / max(0.0001, uFogParams.y-uFogParams.x)); return o; }\n";
    static const char *ps_src =
        "Texture2D t0 : register(t0); SamplerState s0 : register(s0);\n"
        "#pragma pack_matrix(row_major)\n"
        "cbuffer C : register(b0) { row_major float4x4 uModelView; row_major float4x4 uProj; float4 uFogColor; float4 uFogParams; };\n"
        "struct PSIn { float4 pos:SV_POSITION; float2 uv:TEXCOORD0; float4 color:COLOR0; float fog:TEXCOORD1; };\n"
        "float4 main(PSIn i) : SV_TARGET { float4 tex = t0.Sample(s0, i.uv); float4 c = tex * i.color; if (uFogParams.w > 0.5 && c.a < 0.10) discard; if (uFogParams.z > 0.5) c.rgb = lerp(uFogColor.rgb, c.rgb, i.fog); return c; }\n";
    ID3DBlob *vsb = NULL, *psb = NULL;
    if (!pxr_d3d11_compile_shader(vs_src, "main", "vs_4_0", &vsb)) return 0;
    if (!pxr_d3d11_compile_shader(ps_src, "main", "ps_4_0", &psb)) { ID3DBlob_Release(vsb); return 0; }
    if (FAILED(ID3D11Device_CreateVertexShader(g_pxr_d3d11.dev, ID3DBlob_GetBufferPointer(vsb), ID3DBlob_GetBufferSize(vsb), NULL, &g_pxr_d3d11.vs))) { ID3DBlob_Release(vsb); ID3DBlob_Release(psb); return 0; }
    if (FAILED(ID3D11Device_CreatePixelShader(g_pxr_d3d11.dev, ID3DBlob_GetBufferPointer(psb), ID3DBlob_GetBufferSize(psb), NULL, &g_pxr_d3d11.ps))) { ID3DBlob_Release(vsb); ID3DBlob_Release(psb); return 0; }
    D3D11_INPUT_ELEMENT_DESC layout[3];
    memset(layout, 0, sizeof(layout));
    layout[0].SemanticName = "POSITION"; layout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT; layout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA; layout[0].AlignedByteOffset = 0;
    layout[1].SemanticName = "TEXCOORD"; layout[1].Format = DXGI_FORMAT_R32G32_FLOAT; layout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA; layout[1].AlignedByteOffset = 12;
    layout[2].SemanticName = "COLOR"; layout[2].Format = DXGI_FORMAT_R8G8B8A8_UNORM; layout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA; layout[2].AlignedByteOffset = 20;
    HRESULT hr = ID3D11Device_CreateInputLayout(g_pxr_d3d11.dev, layout, 3, ID3DBlob_GetBufferPointer(vsb), ID3DBlob_GetBufferSize(vsb), &g_pxr_d3d11.input_layout);
    ID3DBlob_Release(vsb); ID3DBlob_Release(psb);
    if (FAILED(hr)) return 0;

    D3D11_BUFFER_DESC cbd; memset(&cbd, 0, sizeof(cbd));
    cbd.ByteWidth = sizeof(PexD3D11CBNative);
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(ID3D11Device_CreateBuffer(g_pxr_d3d11.dev, &cbd, NULL, &g_pxr_d3d11.constant_buffer))) return 0;

    D3D11_SAMPLER_DESC sd; memset(&sd, 0, sizeof(sd));
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    ID3D11Device_CreateSamplerState(g_pxr_d3d11.dev, &sd, &g_pxr_d3d11.sampler_wrap);
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    ID3D11Device_CreateSamplerState(g_pxr_d3d11.dev, &sd, &g_pxr_d3d11.sampler_clamp);

    D3D11_BLEND_DESC bd; memset(&bd, 0, sizeof(bd));
    bd.RenderTarget[0].BlendEnable = FALSE;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    ID3D11Device_CreateBlendState(g_pxr_d3d11.dev, &bd, &g_pxr_d3d11.blend_off);
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    ID3D11Device_CreateBlendState(g_pxr_d3d11.dev, &bd, &g_pxr_d3d11.blend_alpha);

    D3D11_DEPTH_STENCIL_DESC ds; memset(&ds, 0, sizeof(ds));
    ds.DepthEnable = TRUE; ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; ds.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    ID3D11Device_CreateDepthStencilState(g_pxr_d3d11.dev, &ds, &g_pxr_d3d11.depth_on_write);
    ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    ID3D11Device_CreateDepthStencilState(g_pxr_d3d11.dev, &ds, &g_pxr_d3d11.depth_on_nowrite);
    ds.DepthEnable = FALSE;
    ID3D11Device_CreateDepthStencilState(g_pxr_d3d11.dev, &ds, &g_pxr_d3d11.depth_off);

    D3D11_RASTERIZER_DESC rd; memset(&rd, 0, sizeof(rd));
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    ID3D11Device_CreateRasterizerState(g_pxr_d3d11.dev, &rd, &g_pxr_d3d11.rast_nocull);
    return g_pxr_d3d11.vs && g_pxr_d3d11.ps && g_pxr_d3d11.input_layout && g_pxr_d3d11.constant_buffer;
}

static void pxr_d3d11_release_pipeline(void) {
    pxr_d3d11_release_dynamic();
    if (g_pxr_d3d11.constant_buffer) { ID3D11Buffer_Release(g_pxr_d3d11.constant_buffer); g_pxr_d3d11.constant_buffer = NULL; }
    if (g_pxr_d3d11.input_layout) { ID3D11InputLayout_Release(g_pxr_d3d11.input_layout); g_pxr_d3d11.input_layout = NULL; }
    if (g_pxr_d3d11.vs) { ID3D11VertexShader_Release(g_pxr_d3d11.vs); g_pxr_d3d11.vs = NULL; }
    if (g_pxr_d3d11.ps) { ID3D11PixelShader_Release(g_pxr_d3d11.ps); g_pxr_d3d11.ps = NULL; }
    if (g_pxr_d3d11.sampler_wrap) { ID3D11SamplerState_Release(g_pxr_d3d11.sampler_wrap); g_pxr_d3d11.sampler_wrap = NULL; }
    if (g_pxr_d3d11.sampler_clamp) { ID3D11SamplerState_Release(g_pxr_d3d11.sampler_clamp); g_pxr_d3d11.sampler_clamp = NULL; }
    if (g_pxr_d3d11.blend_off) { ID3D11BlendState_Release(g_pxr_d3d11.blend_off); g_pxr_d3d11.blend_off = NULL; }
    if (g_pxr_d3d11.blend_alpha) { ID3D11BlendState_Release(g_pxr_d3d11.blend_alpha); g_pxr_d3d11.blend_alpha = NULL; }
    if (g_pxr_d3d11.depth_on_write) { ID3D11DepthStencilState_Release(g_pxr_d3d11.depth_on_write); g_pxr_d3d11.depth_on_write = NULL; }
    if (g_pxr_d3d11.depth_on_nowrite) { ID3D11DepthStencilState_Release(g_pxr_d3d11.depth_on_nowrite); g_pxr_d3d11.depth_on_nowrite = NULL; }
    if (g_pxr_d3d11.depth_off) { ID3D11DepthStencilState_Release(g_pxr_d3d11.depth_off); g_pxr_d3d11.depth_off = NULL; }
    if (g_pxr_d3d11.rast_nocull) { ID3D11RasterizerState_Release(g_pxr_d3d11.rast_nocull); g_pxr_d3d11.rast_nocull = NULL; }
}

static int pxr_d3d11_create_views(int width, int height) {
    ID3D11Texture2D *back = NULL;
    if (FAILED(IDXGISwapChain_GetBuffer(g_pxr_d3d11.swap, 0, &IID_ID3D11Texture2D, (void**)&back)) || !back) return 0;
    HRESULT hr = ID3D11Device_CreateRenderTargetView(g_pxr_d3d11.dev, (ID3D11Resource*)back, NULL, &g_pxr_d3d11.rtv);
    ID3D11Texture2D_Release(back);
    if (FAILED(hr)) return 0;
    D3D11_TEXTURE2D_DESC dd; memset(&dd, 0, sizeof(dd));
    dd.Width = width > 0 ? (UINT)width : 1;
    dd.Height = height > 0 ? (UINT)height : 1;
    dd.MipLevels = 1; dd.ArraySize = 1;
    dd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dd.SampleDesc.Count = 1;
    dd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    if (FAILED(ID3D11Device_CreateTexture2D(g_pxr_d3d11.dev, &dd, NULL, &g_pxr_d3d11.depth_tex))) return 0;
    if (FAILED(ID3D11Device_CreateDepthStencilView(g_pxr_d3d11.dev, (ID3D11Resource*)g_pxr_d3d11.depth_tex, NULL, &g_pxr_d3d11.dsv))) return 0;
    ID3D11DeviceContext_OMSetRenderTargets(g_pxr_d3d11.ctx, 1, &g_pxr_d3d11.rtv, g_pxr_d3d11.dsv);
    D3D11_VIEWPORT vp; memset(&vp, 0, sizeof(vp));
    vp.Width = (FLOAT)dd.Width; vp.Height = (FLOAT)dd.Height; vp.MinDepth = 0; vp.MaxDepth = 1;
    ID3D11DeviceContext_RSSetViewports(g_pxr_d3d11.ctx, 1, &vp);
    return 1;
}

static int d3d11_init(void *window_handle, int width, int height) {
    memset(&g_pxr_d3d11, 0, sizeof(g_pxr_d3d11));
    HWND hwnd = (HWND)window_handle;
    if (!hwnd) return 0;
    if (width < 1) width = 640;
    if (height < 1) height = 480;
    g_pxr_d3d11.hwnd = hwnd; g_pxr_d3d11.width = width; g_pxr_d3d11.height = height;
    g_pxr_d3d11.next_mesh = 1; g_pxr_d3d11.next_texture = 1;

    DXGI_SWAP_CHAIN_DESC scd; memset(&scd, 0, sizeof(scd));
    scd.BufferCount = 2;
    scd.BufferDesc.Width = (UINT)width;
    scd.BufferDesc.Height = (UINT)height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL got;
    HRESULT hr = d3d11_create_swap_chain(&scd, levels, (UINT)ARRAY_COUNT(levels), &got);
    if (FAILED(hr) || !g_pxr_d3d11.dev || !g_pxr_d3d11.ctx || !g_pxr_d3d11.swap) { memset(&g_pxr_d3d11, 0, sizeof(g_pxr_d3d11)); return 0; }
    if (!pxr_d3d11_create_views(width, height)) return 0;
    if (!pxr_d3d11_create_pipeline()) return 0;
    ID3D11DeviceContext_IASetInputLayout(g_pxr_d3d11.ctx, g_pxr_d3d11.input_layout);
    ID3D11DeviceContext_VSSetShader(g_pxr_d3d11.ctx, g_pxr_d3d11.vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(g_pxr_d3d11.ctx, g_pxr_d3d11.ps, NULL, 0);
    ID3D11DeviceContext_RSSetState(g_pxr_d3d11.ctx, g_pxr_d3d11.rast_nocull);
    g_pxr_d3d11.active = 1;
    return 1;
}

static void d3d11_shutdown(void) {
    d3d11_release_retired_meshes(1);
    for (uint32_t i = 1; i < PEX_RENDERER_MAX_MESHES; ++i) pxr_d3d11_release_mesh(&g_pxr_d3d11.meshes[i]);
    for (uint32_t i = 1; i < PEX_RENDERER_MAX_TEXTURES; ++i) {
        if (g_pxr_d3d11.textures[i].srv) { ID3D11ShaderResourceView_Release(g_pxr_d3d11.textures[i].srv); g_pxr_d3d11.textures[i].srv = NULL; }
        if (g_pxr_d3d11.textures[i].tex) { ID3D11Texture2D_Release(g_pxr_d3d11.textures[i].tex); g_pxr_d3d11.textures[i].tex = NULL; }
    }
    pxr_d3d11_release_pipeline();
    pxr_d3d11_release_views();
    if (g_pxr_d3d11.ctx) { if (!g_pxr_d3d11.borrowed_device) ID3D11DeviceContext_ClearState(g_pxr_d3d11.ctx); ID3D11DeviceContext_Release(g_pxr_d3d11.ctx); g_pxr_d3d11.ctx = NULL; }
    if (!g_pxr_d3d11.borrowed_device && g_pxr_d3d11.swap) { IDXGISwapChain_Release(g_pxr_d3d11.swap); g_pxr_d3d11.swap = NULL; }
    if (g_pxr_d3d11.dev) { ID3D11Device_Release(g_pxr_d3d11.dev); g_pxr_d3d11.dev = NULL; }
    memset(&g_pxr_d3d11, 0, sizeof(g_pxr_d3d11));
    memset(g_pxr_d3d11_deferred_mesh_release, 0, sizeof(g_pxr_d3d11_deferred_mesh_release));
    g_pxr_d3d11_frame_serial = 1;
}

static void d3d11_resize(int width, int height) {
    if (!g_pxr_d3d11.active || !g_pxr_d3d11.swap) return;
    if (width < 1) width = 1;
    if (height < 1) height = 1;
    g_pxr_d3d11.width = width; g_pxr_d3d11.height = height;
    ID3D11DeviceContext_OMSetRenderTargets(g_pxr_d3d11.ctx, 0, NULL, NULL);
    pxr_d3d11_release_views();
    IDXGISwapChain_ResizeBuffers(g_pxr_d3d11.swap, 0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, g_pxr_d3d11.swap_flags);
    pxr_d3d11_create_views(width, height);
    g_pxr_d3d11.cache_valid = 0;
}

static int d3d11_begin_frame(float r, float g, float b, float a) {
    if (!g_pxr_d3d11.ctx) return 0;
    g_pxr_d3d11_frame_serial++;
    d3d11_release_retired_meshes(0);
    g_pxr_d3d11.stats.draw_calls = 0;
    g_pxr_d3d11.stats.triangles = 0;
    g_pxr_d3d11.stats.buffer_uploads = 0;
    float clear[4] = { r, g, b, a };
    ID3D11DeviceContext_OMSetRenderTargets(g_pxr_d3d11.ctx, 1, &g_pxr_d3d11.rtv, g_pxr_d3d11.dsv);
    ID3D11DeviceContext_ClearRenderTargetView(g_pxr_d3d11.ctx, g_pxr_d3d11.rtv, clear);
    ID3D11DeviceContext_ClearDepthStencilView(g_pxr_d3d11.ctx, g_pxr_d3d11.dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    ID3D11DeviceContext_IASetInputLayout(g_pxr_d3d11.ctx, g_pxr_d3d11.input_layout);
    ID3D11DeviceContext_VSSetShader(g_pxr_d3d11.ctx, g_pxr_d3d11.vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(g_pxr_d3d11.ctx, g_pxr_d3d11.ps, NULL, 0);
    ID3D11DeviceContext_RSSetState(g_pxr_d3d11.ctx, g_pxr_d3d11.rast_nocull);
    ID3D11DeviceContext_IASetPrimitiveTopology(g_pxr_d3d11.ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(g_pxr_d3d11.ctx, g_pxr_d3d11.vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(g_pxr_d3d11.ctx, g_pxr_d3d11.ps, NULL, 0);
    ID3D11DeviceContext_RSSetState(g_pxr_d3d11.ctx, g_pxr_d3d11.rast_nocull);
    return 1;
}

static void d3d11_end_frame(void) {
    if (!g_pxr_d3d11.swap) return;
    UINT flags = (g_pxr_d3d11.allow_tearing && g_opts.max_fps <= 0) ? DXGI_PRESENT_ALLOW_TEARING : 0;
    IDXGISwapChain_Present(g_pxr_d3d11.swap, 0, flags);
}

static PexTextureHandle d3d11_create_texture(const PexTextureDesc *desc) {
    if (!g_pxr_d3d11.dev || !desc || desc->width <= 0 || desc->height <= 0) return 0;
    uint32_t id = g_pxr_d3d11.next_texture++;
    if (id >= PEX_RENDERER_MAX_TEXTURES) return 0;
    PexD3D11TextureNative *t = &g_pxr_d3d11.textures[id];
    memset(t, 0, sizeof(*t));
    D3D11_TEXTURE2D_DESC td; memset(&td, 0, sizeof(td));
    td.Width = (UINT)desc->width; td.Height = (UINT)desc->height; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_DEFAULT; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA init; memset(&init, 0, sizeof(init));
    init.pSysMem = desc->rgba_pixels; init.SysMemPitch = desc->width * sizeof(uint32_t);
    if (FAILED(ID3D11Device_CreateTexture2D(g_pxr_d3d11.dev, &td, desc->rgba_pixels ? &init : NULL, &t->tex))) return 0;
    if (FAILED(ID3D11Device_CreateShaderResourceView(g_pxr_d3d11.dev, (ID3D11Resource*)t->tex, NULL, &t->srv))) { ID3D11Texture2D_Release(t->tex); memset(t, 0, sizeof(*t)); return 0; }
    t->width = desc->width; t->height = desc->height; t->repeat = desc->repeat;
    g_pxr_d3d11.stats.texture_uploads++;
    return id;
}

static void d3d11_destroy_texture(PexTextureHandle handle) {
    if (handle == 0 || handle >= PEX_RENDERER_MAX_TEXTURES) return;
    if (g_pxr_d3d11.textures[handle].srv) ID3D11ShaderResourceView_Release(g_pxr_d3d11.textures[handle].srv);
    if (g_pxr_d3d11.textures[handle].tex) ID3D11Texture2D_Release(g_pxr_d3d11.textures[handle].tex);
    memset(&g_pxr_d3d11.textures[handle], 0, sizeof(g_pxr_d3d11.textures[handle]));
}

static uint32_t d3d11_next_pow2(uint32_t v, uint32_t minv) {
    uint32_t c = minv;
    while (c < v && c < 0x80000000u) c <<= 1;
    return c < v ? v : c;
}

static int d3d11_ensure_mesh_capacity(PexD3D11Mesh *m, const PexMesh *mesh) {
    if (!g_pxr_d3d11.dev || !m || !mesh) return 0;
    int recreate = 0;
    if (!m->vb || !m->ib) recreate = 1;
    if (m->dynamic != mesh->dynamic) recreate = 1;
    if (m->vertex_capacity < mesh->vertex_count || m->index_capacity < mesh->index_count) recreate = 1;
    if (!recreate) return 1;

    if (m->vb) { ID3D11Buffer_Release(m->vb); m->vb = NULL; }
    if (m->ib) { ID3D11Buffer_Release(m->ib); m->ib = NULL; }

    m->dynamic = mesh->dynamic;
    m->vertex_capacity = d3d11_next_pow2(mesh->vertex_count, 4096u);
    m->index_capacity = d3d11_next_pow2(mesh->index_count, 6144u);

    D3D11_BUFFER_DESC bd; memset(&bd, 0, sizeof(bd));
    bd.ByteWidth = m->vertex_capacity * sizeof(PexVertex);
    bd.Usage = mesh->dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = mesh->dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    if (FAILED(ID3D11Device_CreateBuffer(g_pxr_d3d11.dev, &bd, NULL, &m->vb))) { pxr_d3d11_release_mesh(m); return 0; }

    memset(&bd, 0, sizeof(bd));
    bd.ByteWidth = m->index_capacity * sizeof(uint32_t);
    bd.Usage = mesh->dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = mesh->dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    if (FAILED(ID3D11Device_CreateBuffer(g_pxr_d3d11.dev, &bd, NULL, &m->ib))) { pxr_d3d11_release_mesh(m); return 0; }
    g_pxr_d3d11.stats.buffer_uploads++;
    return 1;
}

static int d3d11_upload_mesh_to_slot(uint32_t id, const PexMesh *mesh) {
    if (!g_pxr_d3d11.dev || !mesh || !mesh->vertices || !mesh->indices || mesh->vertex_count == 0 || mesh->index_count == 0) return 0;
    PexD3D11Mesh *m = &g_pxr_d3d11.meshes[id];
    if (!d3d11_ensure_mesh_capacity(m, mesh)) return 0;
    m->vertex_count = mesh->vertex_count;
    m->index_count = mesh->index_count;

    if (mesh->dynamic) {
        D3D11_MAPPED_SUBRESOURCE map;
        if (FAILED(ID3D11DeviceContext_Map(g_pxr_d3d11.ctx, (ID3D11Resource*)m->vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &map))) return 0;
        memcpy(map.pData, mesh->vertices, mesh->vertex_count * sizeof(PexVertex));
        ID3D11DeviceContext_Unmap(g_pxr_d3d11.ctx, (ID3D11Resource*)m->vb, 0);
        if (FAILED(ID3D11DeviceContext_Map(g_pxr_d3d11.ctx, (ID3D11Resource*)m->ib, 0, D3D11_MAP_WRITE_DISCARD, 0, &map))) return 0;
        memcpy(map.pData, mesh->indices, mesh->index_count * sizeof(uint32_t));
        ID3D11DeviceContext_Unmap(g_pxr_d3d11.ctx, (ID3D11Resource*)m->ib, 0);
    } else {
        ID3D11DeviceContext_UpdateSubresource(g_pxr_d3d11.ctx, (ID3D11Resource*)m->vb, 0, NULL, mesh->vertices, 0, 0);
        ID3D11DeviceContext_UpdateSubresource(g_pxr_d3d11.ctx, (ID3D11Resource*)m->ib, 0, NULL, mesh->indices, 0, 0);
    }
    return 1;
}

static PexMeshHandle d3d11_upload_mesh(const PexMesh *mesh) {
    uint32_t id = g_pxr_d3d11.next_mesh++;
    if (id == 0 || id >= PEX_RENDERER_MAX_MESHES) return 0;
    if (!d3d11_upload_mesh_to_slot(id, mesh)) return 0;
    return id;
}

static int d3d11_update_mesh(PexMeshHandle handle, const PexMesh *mesh) {
    if (handle == 0 || handle >= PEX_RENDERER_MAX_MESHES) return 0;
    return d3d11_upload_mesh_to_slot(handle, mesh);
}

static void d3d11_destroy_mesh(PexMeshHandle handle) {
    if (handle == 0 || handle >= PEX_RENDERER_MAX_MESHES) return;
    pxr_d3d11_release_mesh(&g_pxr_d3d11.meshes[handle]);
}

static void pxr_d3d11_apply_state(const PexRenderState *s) {
    if (!g_pxr_d3d11.ctx || !s) return;
    float blend_factor[4] = {0,0,0,0};
    ID3D11BlendState *bs = s->blend_enabled ? g_pxr_d3d11.blend_alpha : g_pxr_d3d11.blend_off;
    ID3D11DepthStencilState *ds = s->depth_enabled ? (s->depth_write ? g_pxr_d3d11.depth_on_write : g_pxr_d3d11.depth_on_nowrite) : g_pxr_d3d11.depth_off;
    ID3D11DeviceContext_OMSetBlendState(g_pxr_d3d11.ctx, bs, blend_factor, 0xffffffffu);
    ID3D11DeviceContext_OMSetDepthStencilState(g_pxr_d3d11.ctx, ds, 0);
    ID3D11ShaderResourceView *srv = NULL;
    ID3D11SamplerState *samp = g_pxr_d3d11.sampler_wrap;
    if (s->texture_enabled && s->texture > 0 && s->texture < PEX_RENDERER_MAX_TEXTURES) {
        srv = g_pxr_d3d11.textures[s->texture].srv;
        samp = g_pxr_d3d11.textures[s->texture].repeat ? g_pxr_d3d11.sampler_wrap : g_pxr_d3d11.sampler_clamp;
    }
    ID3D11DeviceContext_PSSetShaderResources(g_pxr_d3d11.ctx, 0, 1, &srv);
    ID3D11DeviceContext_PSSetSamplers(g_pxr_d3d11.ctx, 0, 1, &samp);
    D3D11_MAPPED_SUBRESOURCE map;
    if (SUCCEEDED(ID3D11DeviceContext_Map(g_pxr_d3d11.ctx, (ID3D11Resource*)g_pxr_d3d11.constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map))) {
        PexD3D11CBNative *cb = (PexD3D11CBNative*)map.pData;
        float proj_d3d[16];
        memcpy(proj_d3d, s->projection, sizeof(proj_d3d));
        d3d11_depth_remap_gl_to_d3d(proj_d3d);
        d3d11_matrix_from_gl(cb->modelview, s->modelview);
        d3d11_matrix_from_gl(cb->projection, proj_d3d);
        cb->fog_color[0] = ((s->fog_color >> 16) & 255) / 255.0f;
        cb->fog_color[1] = ((s->fog_color >> 8) & 255) / 255.0f;
        cb->fog_color[2] = (s->fog_color & 255) / 255.0f;
        cb->fog_color[3] = 1.0f;
        cb->fog_params[0] = s->fog_start;
        cb->fog_params[1] = s->fog_end;
        cb->fog_params[2] = s->fog_enabled ? 1.0f : 0.0f;
        cb->fog_params[3] = s->alpha_test_enabled ? 1.0f : 0.0f;
        ID3D11DeviceContext_Unmap(g_pxr_d3d11.ctx, (ID3D11Resource*)g_pxr_d3d11.constant_buffer, 0);
    }
    ID3D11DeviceContext_VSSetConstantBuffers(g_pxr_d3d11.ctx, 0, 1, &g_pxr_d3d11.constant_buffer);
    ID3D11DeviceContext_PSSetConstantBuffers(g_pxr_d3d11.ctx, 0, 1, &g_pxr_d3d11.constant_buffer);
}

static void d3d11_draw_mesh(PexMeshHandle handle, const PexRenderState *state) {
    if (handle == 0 || handle >= PEX_RENDERER_MAX_MESHES || !g_pxr_d3d11.ctx) return;
    PexD3D11Mesh *m = &g_pxr_d3d11.meshes[handle];
    if (!m->vb || !m->ib || m->index_count < 3) return;
    pxr_d3d11_apply_state(state);
    UINT stride = sizeof(PexVertex), offset = 0;
    ID3D11DeviceContext_IASetInputLayout(g_pxr_d3d11.ctx, g_pxr_d3d11.input_layout);
    ID3D11DeviceContext_VSSetShader(g_pxr_d3d11.ctx, g_pxr_d3d11.vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(g_pxr_d3d11.ctx, g_pxr_d3d11.ps, NULL, 0);
    ID3D11DeviceContext_RSSetState(g_pxr_d3d11.ctx, g_pxr_d3d11.rast_nocull);
    ID3D11DeviceContext_IASetPrimitiveTopology(g_pxr_d3d11.ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_IASetVertexBuffers(g_pxr_d3d11.ctx, 0, 1, &m->vb, &stride, &offset);
    ID3D11DeviceContext_IASetIndexBuffer(g_pxr_d3d11.ctx, m->ib, DXGI_FORMAT_R32_UINT, 0);
    ID3D11DeviceContext_DrawIndexed(g_pxr_d3d11.ctx, m->index_count, 0, 0);
    g_pxr_d3d11.stats.draw_calls++;
    g_pxr_d3d11.stats.triangles += m->index_count / 3;
}

static int pxr_d3d11_ensure_dynamic(uint32_t vcount, uint32_t icount) {
    if (!g_pxr_d3d11.dev) return 0;
    if (!g_pxr_d3d11.dynamic_vb || g_pxr_d3d11.dynamic_v_cap < vcount) {
        if (g_pxr_d3d11.dynamic_vb) ID3D11Buffer_Release(g_pxr_d3d11.dynamic_vb);
        uint32_t cap = g_pxr_d3d11.dynamic_v_cap ? g_pxr_d3d11.dynamic_v_cap * 2 : 8192;
        while (cap < vcount) cap *= 2;
        D3D11_BUFFER_DESC bd; memset(&bd, 0, sizeof(bd));
        bd.ByteWidth = cap * sizeof(PexVertex); bd.Usage = D3D11_USAGE_DYNAMIC; bd.BindFlags = D3D11_BIND_VERTEX_BUFFER; bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(ID3D11Device_CreateBuffer(g_pxr_d3d11.dev, &bd, NULL, &g_pxr_d3d11.dynamic_vb))) return 0;
        g_pxr_d3d11.dynamic_v_cap = cap;
    }
    if (!g_pxr_d3d11.dynamic_ib || g_pxr_d3d11.dynamic_i_cap < icount) {
        if (g_pxr_d3d11.dynamic_ib) ID3D11Buffer_Release(g_pxr_d3d11.dynamic_ib);
        uint32_t cap = g_pxr_d3d11.dynamic_i_cap ? g_pxr_d3d11.dynamic_i_cap * 2 : 12288;
        while (cap < icount) cap *= 2;
        D3D11_BUFFER_DESC bd; memset(&bd, 0, sizeof(bd));
        bd.ByteWidth = cap * sizeof(uint32_t); bd.Usage = D3D11_USAGE_DYNAMIC; bd.BindFlags = D3D11_BIND_INDEX_BUFFER; bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(ID3D11Device_CreateBuffer(g_pxr_d3d11.dev, &bd, NULL, &g_pxr_d3d11.dynamic_ib))) return 0;
        g_pxr_d3d11.dynamic_i_cap = cap;
    }
    return 1;
}

static void d3d11_draw_dynamic(const PexMesh *mesh, const PexRenderState *state) {
    if (!mesh || !mesh->vertices || !mesh->indices || mesh->vertex_count == 0 || mesh->index_count < 3) return;
    if (!pxr_d3d11_ensure_dynamic(mesh->vertex_count, mesh->index_count)) return;
    D3D11_MAPPED_SUBRESOURCE map;
    if (FAILED(ID3D11DeviceContext_Map(g_pxr_d3d11.ctx, (ID3D11Resource*)g_pxr_d3d11.dynamic_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &map))) return;
    memcpy(map.pData, mesh->vertices, mesh->vertex_count * sizeof(PexVertex));
    ID3D11DeviceContext_Unmap(g_pxr_d3d11.ctx, (ID3D11Resource*)g_pxr_d3d11.dynamic_vb, 0);
    if (FAILED(ID3D11DeviceContext_Map(g_pxr_d3d11.ctx, (ID3D11Resource*)g_pxr_d3d11.dynamic_ib, 0, D3D11_MAP_WRITE_DISCARD, 0, &map))) return;
    memcpy(map.pData, mesh->indices, mesh->index_count * sizeof(uint32_t));
    ID3D11DeviceContext_Unmap(g_pxr_d3d11.ctx, (ID3D11Resource*)g_pxr_d3d11.dynamic_ib, 0);
    pxr_d3d11_apply_state(state);
    UINT stride = sizeof(PexVertex), offset = 0;
    ID3D11DeviceContext_IASetPrimitiveTopology(g_pxr_d3d11.ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_IASetVertexBuffers(g_pxr_d3d11.ctx, 0, 1, &g_pxr_d3d11.dynamic_vb, &stride, &offset);
    ID3D11DeviceContext_IASetIndexBuffer(g_pxr_d3d11.ctx, g_pxr_d3d11.dynamic_ib, DXGI_FORMAT_R32_UINT, 0);
    ID3D11DeviceContext_DrawIndexed(g_pxr_d3d11.ctx, mesh->index_count, 0, 0);
    g_pxr_d3d11.stats.draw_calls++;
    g_pxr_d3d11.stats.triangles += mesh->index_count / 3;
}


static int renderer_d3d11_attach_device(void *dev, void *ctx, int width, int height) {
    if (!dev || !ctx) return 0;
    d3d11_shutdown();
    memset(&g_pxr_d3d11, 0, sizeof(g_pxr_d3d11));
    g_pxr_d3d11.dev = (ID3D11Device*)dev;
    g_pxr_d3d11.ctx = (ID3D11DeviceContext*)ctx;
    ID3D11Device_AddRef(g_pxr_d3d11.dev);
    ID3D11DeviceContext_AddRef(g_pxr_d3d11.ctx);
    g_pxr_d3d11.borrowed_device = 1;
    g_pxr_d3d11.active = 1;
    g_pxr_d3d11.width = width > 0 ? width : 640;
    g_pxr_d3d11.height = height > 0 ? height : 480;
    g_pxr_d3d11.next_mesh = 1;
    g_pxr_d3d11.next_texture = 1;
    if (!pxr_d3d11_create_pipeline()) { d3d11_shutdown(); return 0; }
    ID3D11DeviceContext_IASetInputLayout(g_pxr_d3d11.ctx, g_pxr_d3d11.input_layout);
    ID3D11DeviceContext_VSSetShader(g_pxr_d3d11.ctx, g_pxr_d3d11.vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(g_pxr_d3d11.ctx, g_pxr_d3d11.ps, NULL, 0);
    ID3D11DeviceContext_RSSetState(g_pxr_d3d11.ctx, g_pxr_d3d11.rast_nocull);
    return 1;
}

static PexRendererStats d3d11_get_stats(void) { return g_pxr_d3d11.stats; }

static PexRendererBackend g_renderer_d3d11_native = {
    "Direct3D 11 Native",
    d3d11_init,
    d3d11_shutdown,
    d3d11_resize,
    d3d11_begin_frame,
    d3d11_end_frame,
    d3d11_create_texture,
    d3d11_destroy_texture,
    d3d11_upload_mesh,
    d3d11_update_mesh,
    d3d11_destroy_mesh,
    d3d11_draw_mesh,
    d3d11_draw_dynamic,
    d3d11_get_stats
};

static PexRendererBackend *renderer_d3d11_get_backend(void) { return &g_renderer_d3d11_native; }
