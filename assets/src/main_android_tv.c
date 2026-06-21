/* PEXCRAFT Android TV unity entry.
   Platform code lives under src/platform/android_tv/.  It reuses SDL2, the
   OpenGL ES 2.0 fixed-function compatibility renderer, and the shared gamepad
   abstraction so TV remote buttons behave like a controller. */
#define PEX_PLATFORM_ANDROID_TV 1
#define PEX_PLATFORM_SDL2 1
#include "platform/android_tv/android_tv_compat.h"
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

/* D3D-only helpers are referenced by shared world code, but the Android TV
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
static PexRendererBackend g_android_tv_null_backend;
static PexRendererBackend *renderer_d3d9_get_backend(void) { return &g_android_tv_null_backend; }
static PexRendererBackend *renderer_d3d11_get_backend(void) { return &g_android_tv_null_backend; }
static int renderer_d3d9_attach_device(void *dev, int width, int height) { (void)dev; (void)width; (void)height; return 0; }
static int renderer_d3d11_attach_device(void *dev, void *ctx, int width, int height) { (void)dev; (void)ctx; (void)width; (void)height; return 0; }
static int pex_using_d3d9(void) { return 0; }
static int pex_using_d3d11(void) { return 0; }
static int renderer_d3d11_prebuild_mesh_buffers(const PexMesh *mesh, PexD3D11Mesh *out) { (void)mesh; memset(out, 0, sizeof(*out)); return 0; }
static void renderer_d3d11_discard_prebuilt_mesh(PexD3D11Mesh *mesh) { if (mesh) memset(mesh, 0, sizeof(*mesh)); }
static int renderer_d3d11_adopt_prebuilt_mesh(PexMeshHandle *slot, PexD3D11Mesh *built) { (void)slot; (void)built; return 0; }
static void renderer_d3d11_destroy_mesh_deferred(PexMeshHandle *slot) { if (slot) *slot = 0; }

#include "platform/android_tv/android_tv_gles2_renderer.c"

#define glClearColor pex_android_tv_glClearColor
#define glClear pex_android_tv_glClear
#define glColorMask pex_android_tv_glColorMask
#define glReadBuffer pex_android_tv_glReadBuffer
#define glReadPixels pex_android_tv_glReadPixels
#define glEnable pex_android_tv_glEnable
#define glDisable pex_android_tv_glDisable
#define glBlendFunc pex_android_tv_glBlendFunc
#define glDepthFunc pex_android_tv_glDepthFunc
#define glDepthMask pex_android_tv_glDepthMask
#define glAlphaFunc pex_android_tv_glAlphaFunc
#define glLineWidth pex_android_tv_glLineWidth
#define glViewport pex_android_tv_glViewport
#define glMatrixMode pex_android_tv_glMatrixMode
#define glLoadIdentity pex_android_tv_glLoadIdentity
#define glPushMatrix pex_android_tv_glPushMatrix
#define glPopMatrix pex_android_tv_glPopMatrix
#define glTranslatef pex_android_tv_glTranslatef
#define glRotatef pex_android_tv_glRotatef
#define glScalef pex_android_tv_glScalef
#define glOrtho pex_android_tv_glOrtho
#define gluPerspective pex_android_tv_gluPerspective
#define glGetFloatv pex_android_tv_glGetFloatv
#define glGetDoublev pex_android_tv_glGetDoublev
#define glGetIntegerv pex_android_tv_glGetIntegerv
#define glFogi pex_android_tv_glFogi
#define glFogf pex_android_tv_glFogf
#define glFogfv pex_android_tv_glFogfv
#define glColor4f pex_android_tv_glColor4f
#define glTexCoord2f pex_android_tv_glTexCoord2f
#define glBegin pex_android_tv_glBegin
#define glVertex3f pex_android_tv_glVertex3f
#define glVertex2f pex_android_tv_glVertex2f
#define glEnd pex_android_tv_glEnd
#define glGenLists pex_android_tv_glGenLists
#define glNewList pex_android_tv_glNewList
#define glEndList pex_android_tv_glEndList
#define glCallList pex_android_tv_glCallList
#define glGenTextures pex_android_tv_glGenTextures
#define glBindTexture pex_android_tv_glBindTexture
#define glTexParameteri pex_android_tv_glTexParameteri
#define glTexImage2D pex_android_tv_glTexImage2D
#define glDeleteTextures pex_android_tv_glDeleteTextures
#define glCopyTexSubImage2D pex_android_tv_glCopyTexSubImage2D
#define glPixelStorei pex_android_tv_glPixelStorei
#define glGetString pex_android_tv_glGetString
#define glEnableClientState pex_android_tv_glEnableClientState
#define glDisableClientState pex_android_tv_glDisableClientState
#define glVertexPointer pex_android_tv_glVertexPointer
#define glTexCoordPointer pex_android_tv_glTexCoordPointer
#define glColorPointer pex_android_tv_glColorPointer
#define glDrawElements pex_android_tv_glDrawElements
#define gluProject pex_android_tv_gluProject

#include "platform/android_tv/android_tv_filesystem.c"
#include "assets/textures.c"
#include "save/nbt_gzip_save.c"
#include "assets/pxc_zip_extract.c"
#include "platform/android_tv/android_tv_classic_pack_installer.c"
#include "worldgen/level.c"
#include "game/world_session.c"
#include "render/gui_primitives.c"
#include "settings/options.c"
#include "platform/sdl2_input.c"

static void steve_set_texture_dims(const Texture *tex);
static void steve_set_tint(float r, float g, float b);

#include "game/inventory.c"
#include "platform/multiplayer_client.c"
#include "ui/screen_state_input.c"
#include "ui/title_menus.c"
#include "game/ingame_logic.c"
#include "render/world_view.c"
#include "render/item_render.c"
#include "ui/gui.c"
#include "platform/android_tv/android_tv_input.c"
#include "platform/gamepad.c"
#include "render/render_dispatch.c"
#include "platform/android_tv/android_tv_app.c"
