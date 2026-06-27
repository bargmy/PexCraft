/* PEXCRAFT Xbox One / UWP unity entry.
   This is the real engine translation unit for the UWP package.  The C++/WinRT
   CoreWindow shell calls the exported pex_xbox_uwp_engine_* functions below. */
#define PEX_PLATFORM_XBOX_UWP 1
#include "platform/xbox_uwp/xbox_uwp_compat.h"
#include "common/common.h"

static void scan_texture_packs(void);
static int load_default_textures(void);
static void apply_texture_pack_index(int index);
static int pex_renderer_backend_init(HWND core_window);
static int pex_renderer_begin_frame(void);
static void pex_renderer_present(void);
static void pex_renderer_resize(int w, int h);
static void pex_renderer_shutdown(void);
static void pex_gl_suppress_immediate(int on);
static void save_world_state_for_exit(void);

#include "render/renderer_backend.h"
static PexRendererBackend g_xbox_d3d9_null_backend;
static PexRendererBackend *renderer_d3d9_get_backend(void) { return &g_xbox_d3d9_null_backend; }
static int renderer_d3d9_attach_device(void *dev, int width, int height) { (void)dev; (void)width; (void)height; return 0; }
#include "render/renderer_d3d11.c"
static int pex_renderer_d3d11_xbox_init_corewindow(void *core_window_unknown, int width, int height);
#include "render/backend_compat.c"
#include "render/renderer_d3d11_xbox.c"

#define glClearColor pex_glClearColor
#define glDisable pex_glDisable
#define glEnable pex_glEnable
#define glBlendFunc pex_glBlendFunc
#define glDepthFunc pex_glDepthFunc
#define glDepthMask pex_glDepthMask
#define glColorMask pex_glColorMask
#define glAlphaFunc pex_glAlphaFunc
#define glLineWidth pex_glLineWidth
#define glColor4f pex_glColor4f
#define glViewport pex_glViewport
#define glClear pex_glClear
#define glMatrixMode pex_glMatrixMode
#define glLoadIdentity pex_glLoadIdentity
#define glPushMatrix pex_glPushMatrix
#define glPopMatrix pex_glPopMatrix
#define glTranslatef pex_glTranslatef
#define glRotatef pex_glRotatef
#define glScalef pex_glScalef
#define glOrtho pex_glOrtho
#define gluPerspective pex_gluPerspective
#define glGetFloatv pex_glGetFloatv
#define glGetDoublev pex_glGetDoublev
#define glGetIntegerv pex_glGetIntegerv
#define glFogi pex_glFogi
#define glFogf pex_glFogf
#define glFogfv pex_glFogfv
#define glTexCoord2f pex_glTexCoord2f_fixed
#define glBegin pex_glBegin
#define glVertex3f pex_glVertex3f
#define glVertex2f pex_glVertex2f
#define glEnd pex_glEnd
#define glGenLists pex_glGenLists
#define glNewList pex_glNewList
#define glEndList pex_glEndList
#define glCallList pex_glCallList
#define glGenTextures pex_glGenTextures
#define glBindTexture pex_glBindTexture
#define glTexParameteri pex_glTexParameteri
#define glTexImage2D pex_glTexImage2D
#define glDeleteTextures pex_glDeleteTextures
#define glCopyTexSubImage2D pex_glCopyTexSubImage2D

#include "platform/filesystem.c"
#include "assets/textures.c"
#include "save/nbt_gzip_save.c"
#include "assets/pxc_zip_extract.c"
#include "assets/classic_pack_installer.c"
#include "worldgen/level.c"
#include "game/world_session.c"
#include "i18n/language.c"
#include "render/gui_primitives.c"
#include "settings/options.c"
#include "audio/sound.c"
#include "platform/xbox_uwp/xbox_uwp_input.c"

static void steve_set_texture_dims(const Texture *tex);
static void steve_set_tint(float r, float g, float b);

#include "game/inventory.c"
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
#include "game/passive_mobs.c"
#include "platform/multiplayer_client.c"
#include "ui/screen_state_input.c"
#include "ui/title_menus.c"
#include "game/ingame_logic.c"
#include "render/world_view.c"
#include "render/item_render.c"
#include "ui/gui.c"
#include "platform/gamepad.c"
#include "render/render_dispatch.c"
#include "platform/loggy_window.c"
#include "platform/xbox_uwp/xbox_uwp_app.c"
