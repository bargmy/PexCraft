/* PEXCRAFT LG webOS unity entry.
   New platform code lives under src/platform/lgwebos/ so existing SDL2, PSP,
   Win32, gameplay, world, UI, and save code stay untouched. */
#define PEX_PLATFORM_LGWEBOS 1
#define PEX_PLATFORM_SDL2 1
#include "platform/lgwebos/lgwebos_compat.h"
#include "common/common.h"

static int init_gl(SDL_Window *window);
static void scan_texture_packs(void);
static int load_default_textures(void);
static void apply_texture_pack_index(int index);
static int pex_renderer_backend_init(SDL_Window *window);
static int pex_renderer_begin_frame(void);
static void pex_renderer_present(void);
static void pex_renderer_resize(int w, int h);
static void pex_renderer_shutdown(void);
static void pex_gl_suppress_immediate(int on);
static void save_world_state_for_exit(void);

#include "render/renderer_backend.h"

/* D3D-only helpers are referenced by shared world code, but the LG webOS
   target always uses the OpenGL ES compatibility shim. */
typedef struct PexMat4 { float m[16]; } PexMat4;
typedef struct PexD3DBackendStub {
    int active;
    float color[4];
    int fog_enabled;
    float fog_start, fog_end;
    unsigned int fog_color;
    PexMat4 modelview;
    PexMat4 projection;
} PexD3DBackendStub;
static PexD3DBackendStub g_d3d9 = {0, {1,1,1,1}, 0, 0.0f, 1.0f, 0, {{0}}, {{0}}};
typedef struct PexD3D11Mesh { void *vb; void *ib; uint32_t vertex_count, index_count, vertex_capacity, index_capacity; int dynamic; } PexD3D11Mesh;
static PexRendererBackend g_lgwebos_null_backend;
static PexRendererBackend *renderer_d3d9_get_backend(void) { return &g_lgwebos_null_backend; }
static PexRendererBackend *renderer_d3d11_get_backend(void) { return &g_lgwebos_null_backend; }
static int renderer_d3d9_attach_device(void *dev, int width, int height) { (void)dev; (void)width; (void)height; return 0; }
static int renderer_d3d11_attach_device(void *dev, void *ctx, int width, int height) { (void)dev; (void)ctx; (void)width; (void)height; return 0; }
static int pex_using_d3d9(void) { return 0; }
static int pex_using_d3d11(void) { return 0; }
static int d3d11_prebuild_mesh(const PexMesh *mesh, PexD3D11Mesh *out) { (void)mesh; memset(out, 0, sizeof(*out)); return 0; }
static void d3d11_discard_mesh(PexD3D11Mesh *mesh) { if (mesh) memset(mesh, 0, sizeof(*mesh)); }
static int d3d11_adopt_mesh(PexMeshHandle *slot, PexD3D11Mesh *built) { (void)slot; (void)built; return 0; }
static void d3d11_destroy_mesh_deferred(PexMeshHandle *slot) { if (slot) *slot = 0; }

#include "platform/lgwebos/lgwebos_gles2_renderer.c"

#define glClearColor pex_lgwebos_glClearColor
#define glClear pex_lgwebos_glClear
#define glColorMask pex_lgwebos_glColorMask
#define glReadBuffer pex_lgwebos_glReadBuffer
#define glReadPixels pex_lgwebos_glReadPixels
#define glEnable pex_lgwebos_glEnable
#define glDisable pex_lgwebos_glDisable
#define glBlendFunc pex_lgwebos_glBlendFunc
#define glDepthFunc pex_lgwebos_glDepthFunc
#define glDepthMask pex_lgwebos_glDepthMask
#define glAlphaFunc pex_lgwebos_glAlphaFunc
#define glLineWidth pex_lgwebos_glLineWidth
#define glViewport pex_lgwebos_glViewport
#define glMatrixMode pex_lgwebos_glMatrixMode
#define glLoadIdentity pex_lgwebos_glLoadIdentity
#define glPushMatrix pex_lgwebos_glPushMatrix
#define glPopMatrix pex_lgwebos_glPopMatrix
#define glTranslatef pex_lgwebos_glTranslatef
#define glRotatef pex_lgwebos_glRotatef
#define glScalef pex_lgwebos_glScalef
#define glOrtho pex_lgwebos_glOrtho
#define gluPerspective pex_lgwebos_gluPerspective
#define glGetFloatv pex_lgwebos_glGetFloatv
#define glGetDoublev pex_lgwebos_glGetDoublev
#define glGetIntegerv pex_lgwebos_glGetIntegerv
#define glFogi pex_lgwebos_glFogi
#define glFogf pex_lgwebos_glFogf
#define glFogfv pex_lgwebos_glFogfv
#define glColor4f pex_lgwebos_glColor4f
#define glTexCoord2f pex_lgwebos_glTexCoord2f
#define glBegin pex_lgwebos_glBegin
#define glVertex3f pex_lgwebos_glVertex3f
#define glVertex2f pex_lgwebos_glVertex2f
#define glEnd pex_lgwebos_glEnd
#define glGenLists pex_lgwebos_glGenLists
#define glNewList pex_lgwebos_glNewList
#define glEndList pex_lgwebos_glEndList
#define glCallList pex_lgwebos_glCallList
#define glGenTextures pex_lgwebos_glGenTextures
#define glBindTexture pex_lgwebos_glBindTexture
#define glTexParameteri pex_lgwebos_glTexParameteri
#define glTexImage2D pex_lgwebos_glTexImage2D
#define glDeleteTextures pex_lgwebos_glDeleteTextures
#define glCopyTexSubImage2D pex_lgwebos_glCopyTexSubImage2D
#define glPixelStorei pex_lgwebos_glPixelStorei
#define glGetString pex_lgwebos_glGetString
#define glEnableClientState pex_lgwebos_glEnableClientState
#define glDisableClientState pex_lgwebos_glDisableClientState
#define glVertexPointer pex_lgwebos_glVertexPointer
#define glTexCoordPointer pex_lgwebos_glTexCoordPointer
#define glColorPointer pex_lgwebos_glColorPointer
#define glDrawElements pex_lgwebos_glDrawElements
#define gluProject pex_lgwebos_gluProject

#include "platform/lgwebos/lgwebos_filesystem.c"
#include "assets/textures.c"
#include "save/nbt_gzip_save.c"
#include "assets/pxc_zip_extract.c"
#include "platform/lgwebos/lgwebos_classic_pack_installer.c"
#include "worldgen/level.c"
#include "game/world_session.c"
#include "i18n/language.c"
#include "render/gui_primitives.c"
#include "settings/options.c"
#include "audio/sound.c"
#include "platform/sdl2_input.c"

static void steve_set_texture_dims(const Texture *tex);
static void steve_set_tint(float r, float g, float b);

#include "game/inventory.c"
#include "game/passive_mobs.c"

/* MCPE protocol_81 implementation must be included before multiplayer_client.c
   because this project builds as one translation unit per platform. */
#include "../multiplayer/mcpe/protocol_81/packets/packet_codec.c"
#include "../multiplayer/mcpe/protocol_81/packets/login_packet.c"
#include "../multiplayer/mcpe/protocol_81/packets/play_status_packet.c"
#include "../multiplayer/mcpe/protocol_81/packets/start_game_packet.c"
#include "../multiplayer/mcpe/protocol_81/packets/full_chunk_data_packet.c"
#include "../multiplayer/mcpe/protocol_81/packets/move_player_packet.c"
#include "../multiplayer/mcpe/protocol_81/packets/request_chunk_radius_packet.c"
#include "../multiplayer/mcpe/protocol_81/packets/text_packet.c"
#include "../multiplayer/mcpe/protocol_81/packets/batch_packet.c"
#include "../multiplayer/mcpe/protocol_81/packets/player_action_packet.c"
#include "../multiplayer/mcpe/protocol_81/packets/remove_block_packet.c"
#include "../multiplayer/mcpe/protocol_81/packets/mob_equipment_packet.c"
#include "../multiplayer/mcpe/protocol_81/packets/use_item_packet.c"
#include "../multiplayer/mcpe/protocol_81/packets/update_block_packet.c"
#include "../multiplayer/mcpe/protocol_81/packets/entity_packets.c"
#include "../multiplayer/mcpe/protocol_81/transport/raknet_loader.c"
#include "../multiplayer/mcpe/protocol_81/transport/raknet_wrapper.c"
#include "../multiplayer/mcpe/protocol_81/session/motd_detect.c"
#include "../multiplayer/mcpe/protocol_81/session/join_session.c"
#include "../multiplayer/mcpe/protocol_81/session/bedrock_join.c"
#include "../multiplayer/mcpe/protocol_81/world/chunk_convert.c"

#include "platform/multiplayer_client.c"
#include "ui/screen_state_input.c"
#include "ui/title_menus.c"
#include "game/ingame_logic.c"
#include "render/world_view.c"
#include "render/item_render.c"
#include "ui/gui.c"
#include "platform/gamepad.c"
#include "render/render_dispatch.c"
#include "platform/lgwebos/lgwebos_app.c"
