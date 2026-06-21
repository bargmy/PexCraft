#ifndef PEXCRAFT_RENDERER_BACKEND_H
#define PEXCRAFT_RENDERER_BACKEND_H

#include <stdint.h>

#ifndef PEX_RENDERER_MAX_TEXTURES
#define PEX_RENDERER_MAX_TEXTURES 4096
#endif
#ifndef PEX_RENDERER_MAX_MESHES
#define PEX_RENDERER_MAX_MESHES 65536
#endif

typedef uint32_t PexMeshHandle;
typedef uint32_t PexTextureHandle;

typedef struct PexVertex {
    float x, y, z;
    float u, v;
    /* Packed as memory RGBA for D3D11 DXGI_FORMAT_R8G8B8A8_UNORM:
       value = r | (g << 8) | (b << 16) | (a << 24). */
    uint32_t color;
} PexVertex;

typedef struct PexMesh {
    const PexVertex *vertices;
    const uint32_t *indices;
    uint32_t vertex_count;
    uint32_t index_count;
    int dynamic;
} PexMesh;

typedef struct PexTextureDesc {
    int width;
    int height;
    const uint32_t *rgba_pixels;
    int repeat;
    int nearest;
} PexTextureDesc;

typedef struct PexRenderState {
    PexTextureHandle texture;
    int texture_enabled;
    int blend_enabled;
    int alpha_test_enabled;
    int depth_enabled;
    int depth_write;
    int fog_enabled;
    float fog_start;
    float fog_end;
    uint32_t fog_color;
    float modelview[16];
    float projection[16];
} PexRenderState;

typedef struct PexRendererStats {
    uint32_t draw_calls;
    uint32_t triangles;
    uint32_t buffer_uploads;
    uint32_t texture_uploads;
} PexRendererStats;

typedef struct PexRendererBackend {
    const char *name;
    int  (*init)(void *window_handle, int width, int height);
    void (*shutdown)(void);
    void (*resize)(int width, int height);
    int  (*begin_frame)(float r, float g, float b, float a);
    void (*end_frame)(void);

    PexTextureHandle (*create_texture)(const PexTextureDesc *desc);
    void (*destroy_texture)(PexTextureHandle handle);

    PexMeshHandle (*upload_mesh)(const PexMesh *mesh);
    int  (*update_mesh)(PexMeshHandle handle, const PexMesh *mesh);
    void (*destroy_mesh)(PexMeshHandle handle);
    void (*draw_mesh)(PexMeshHandle handle, const PexRenderState *state);

    void (*draw_dynamic)(const PexMesh *mesh, const PexRenderState *state);
    PexRendererStats (*get_stats)(void);
} PexRendererBackend;

static PexRendererBackend *renderer_d3d9_get_backend(void);
static PexRendererBackend *renderer_d3d11_get_backend(void);
static int renderer_d3d9_attach_device(void *dev, int width, int height);
static int renderer_d3d11_attach_device(void *dev, void *ctx, int width, int height);

#endif /* PEXCRAFT_RENDERER_BACKEND_H */
