/* PEXCRAFT Nintendo Wii unity entry.
   Builds a separate DOL/ELF with devkitPPC/libogc and the Wii GX renderer. */
#ifndef PEX_PLATFORM_WII
#define PEX_PLATFORM_WII 1
#endif
#include "common/common.h"

static int init_gl(HWND unused);
static void scan_texture_packs(void);
static int load_default_textures(void);
static void apply_texture_pack_index(int index);
static int pex_renderer_backend_init(HWND unused);
static int pex_renderer_begin_frame(void);
static void pex_renderer_present(void);
static void pex_renderer_resize(int w, int h);
static void pex_renderer_shutdown(void);
static void pex_gl_suppress_immediate(int on);
static void save_world_state_for_exit(void);

#include "render/renderer_backend.h"
#include "../platforms/wii/wii_debug.c"

typedef struct PexMat4 { float m[16]; } PexMat4;
typedef struct PexD3DBackendStub { int active; float color[4]; int fog_enabled; float fog_start, fog_end; unsigned int fog_color; PexMat4 modelview; PexMat4 projection; } PexD3DBackendStub;
static PexD3DBackendStub g_d3d9 = {0, {1,1,1,1}, 0, 0.0f, 1.0f, 0, {{0}}, {{0}}};
typedef struct PexD3D11Mesh { void *vb; void *ib; uint32_t vertex_count, index_count, vertex_capacity, index_capacity; int dynamic; } PexD3D11Mesh;
static PexRendererBackend g_wii_null_backend;
static PexRendererBackend *renderer_d3d9_get_backend(void) { return &g_wii_null_backend; }
static PexRendererBackend *renderer_d3d11_get_backend(void) { return &g_wii_null_backend; }
static int renderer_d3d9_attach_device(void *dev, int width, int height) { (void)dev; (void)width; (void)height; return 0; }
static int renderer_d3d11_attach_device(void *dev, void *ctx, int width, int height) { (void)dev; (void)ctx; (void)width; (void)height; return 0; }
static int pex_using_d3d9(void) { return 0; }
static int pex_using_d3d11(void) { return 0; }
static int d3d11_prebuild_mesh(const PexMesh *mesh, PexD3D11Mesh *out) { (void)mesh; memset(out, 0, sizeof(*out)); return 0; }
static void d3d11_discard_mesh(PexD3D11Mesh *mesh) { if (mesh) memset(mesh, 0, sizeof(*mesh)); }
static int d3d11_adopt_mesh(PexMeshHandle *slot, PexD3D11Mesh *built) { (void)slot; (void)built; return 0; }
static void d3d11_destroy_mesh_deferred(PexMeshHandle *slot) { if (slot) *slot = 0; }

#include "../platforms/wii/wii_gx_renderer.c"
#include "../platforms/wii/wii_filesystem.c"
#include "assets/textures.c"
#include "save/nbt_gzip_save.c"
/* Runtime ZIP/JAR extraction uses zlib, which is not part of the minimal
   devkitPPC/libogc image used by CI.  The first Wii port disables runtime
   Classic pack download/extraction and expects texture packs to be copied to SD. */
#include "../platforms/wii/wii_classic_pack_embedded.c"
#include "worldgen/level.c"
#include "game/world_session.c"
#include "i18n/language.c"
#include "render/gui_primitives.c"
#include "settings/options.c"
#include "audio/sound.c"
#include "../platforms/wii/wii_input.c"

static void steve_set_texture_dims(const Texture *tex);
static void steve_set_tint(float r, float g, float b);
static void draw_chat_lines(int force_visible);
static void draw_ingame_world_view(int with_hand);
static const char *item_display_name(int id);

#include "game/inventory.c"
#include "game/passive_mobs.c"
#include "platform/psp_multiplayer_stub.c"
#include "ui/screen_state_input.c"
#include "ui/gui.c"
#include "ui/title_menus.c"
#include "game/ingame_logic.c"
#include "render/world_view.c"
#include "render/item_render.c"
#include "platform/gamepad.c"
#include "render/render_dispatch.c"
#include "../platforms/wii/wii_app.c"
