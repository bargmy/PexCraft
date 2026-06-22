#ifndef MC_B1_SPLIT_COMMON_H
#define MC_B1_SPLIT_COMMON_H

// PEXCRAFT Beta 1.0 title/options/world UI port to C/Win32/OpenGL.
// Build with MinGW32: gcc -std=c99 -O2 -mwindows main.c -o pexcraft.exe -lopengl32 -lglu32 -lgdi32 -luser32 -lshell32 -lole32 -lwindowscodecs -lwinmm
#if defined(PEX_PLATFORM_PSP)
#include "../platform/psp_compat.h"
#elif defined(PEX_PLATFORM_WII)
#include "../../platforms/wii/wii_compat.h"
#elif defined(PEX_PLATFORM_ANDROID_TV)
#include "../platform/android_tv/android_tv_compat.h"
#elif defined(PEX_PLATFORM_SDL2)
#include "../platform/sdl2_compat.h"
#else
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <mmsystem.h>
#include <objbase.h>
#include <initguid.h>
#include <wincodec.h>
#include <shellapi.h>
#include <commdlg.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <direct.h>
#endif

#include "net_protocol.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define APP_TITLE "PEXCRAFT"
#define VERSION_TEXT "PEXCRAFT Beta 1.0"
#define MAX_BUTTONS 64
#define MAX_LABEL 256
#define MAX_PATHBUF 1024
#define ARRAY_COUNT(a) ((int)(sizeof(a)/sizeof((a)[0])))
#ifndef PEX_THREAD_LOCAL
#if defined(PEX_PLATFORM_WII)
/* devkitPPC/libogc Wii DOLs do not have the desktop TLS runtime that GCC
   __thread assumes.  Leaving these as __thread makes small TLS offsets such
   as 0x00000004 show up in the map and can turn globals into bogus pointers
   at runtime, which caused Dolphin DSI crashes while preparing world meshes.
   Wii currently runs mesh/world generation on the main thread, so plain static
   storage is the safe choice. */
#define PEX_THREAD_LOCAL
#elif defined(_MSC_VER)
#define PEX_THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__)
#define PEX_THREAD_LOCAL __thread
#else
#define PEX_THREAD_LOCAL
#endif
#endif
#define MAX_FPS_CAP 200
#define FPS_UNLIMITED_SLIDER_VALUE 0.999f
#define MAX_WORLD_SIM_RADIUS 24
#define MAX_CHUNK_REBUILDS_PER_FRAME 1 /* legacy column renderer */
#define MAX_SECTION_REBUILDS_PER_FRAME 3
#define MAX_WORLD_SIM_CHANGES_PER_TICK 48
#define CLASSIC_PACK_NAME "Minecraft Classic"
#define CLASSIC_PACK_URL "https://piston-data.mojang.com/v1/objects/93faf3398ebf8008d59852dc3c2b22b909ca8a49/client.jar"

typedef struct Texture {
    GLuint id;
    int w, h;
    unsigned char *rgba;
} Texture;

#define MAX_TEXPACKS 32
typedef struct TexturePackEntry {
    char name[MAX_LABEL];
    char path[MAX_PATHBUF];
    char desc1[MAX_LABEL];
    char desc2[MAX_LABEL];
    int is_default;
    int is_builtin_classic;
    int no_icon;
    int has_icon;
    Texture icon;
} TexturePackEntry;

typedef struct BinBuf {
    unsigned char *data;
    size_t len;
    size_t cap;
} BinBuf;

typedef struct WorldGenJob {
    int active;
    int slot;
    int existing_world;
    int radius;
    int total_chunks;
    int chunks_done;
    int done_ticks;
    int progress;
    int phase;
    int loaded_state;
    int terrain_total;
    int terrain_done;
    long long seed;
    char world_name[32];
    char world_dir[MAX_PATHBUF];
    char title[MAX_LABEL];
    char status[MAX_LABEL];
} WorldGenJob;

typedef enum ScreenId {
    SCREEN_TITLE,
    SCREEN_OPTIONS,
    SCREEN_OPTIONS_MORE,
    SCREEN_SYSTEM_INFO,
    SCREEN_SKINS,
    SCREEN_CONTROLS,
    SCREEN_WORLD_SELECT,
    SCREEN_WORLD_TYPE,
    SCREEN_WORLD_DELETE,
    SCREEN_CONFIRM_DELETE,
    SCREEN_MULTIPLAYER,
    SCREEN_CONNECTING,
    SCREEN_TEXPACK,
    SCREEN_TEXPACK_INSTALL,
    SCREEN_GENERATING,
    SCREEN_INGAME,
    SCREEN_PAUSE,
    SCREEN_INVENTORY,
    SCREEN_WORKBENCH,
    SCREEN_FURNACE,
    SCREEN_CHEST,
    SCREEN_DEATH,
    SCREEN_CHAT,
    SCREEN_NOTICE,
    SCREEN_RENDERER_RESTART_PROMPT,
    SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT,
    SCREEN_CLASSIC_PACK_WARNING
} ScreenId;

typedef enum ButtonKind {
    BUTTON_NORMAL,
    BUTTON_SLIDER
} ButtonKind;


/* Last intentional input source.  Touch is not a controller: it uses mouse-style
   direct UI focus plus a separate mobile movement/action path. */
typedef enum PexInputFocusMode {
    PEX_INPUT_FOCUS_MOUSE = 0,
    PEX_INPUT_FOCUS_GAMEPAD = 1,
    PEX_INPUT_FOCUS_TOUCH = 2
} PexInputFocusMode;

typedef enum RendererBackend {
    RENDERER_OPENGL = 0,
    RENDERER_D3D9,
    RENDERER_D3D11,
    RENDERER_COUNT
} RendererBackend;

typedef enum OptionId {
    OPT_MUSIC = 0,
    OPT_SOUND,
    OPT_INVERT_MOUSE,
    OPT_SENSITIVITY,
    OPT_RENDER_DISTANCE,
    OPT_VIEW_BOBBING,
    OPT_ANAGLYPH,
    OPT_LIMIT_FRAMERATE,
    OPT_FOV,
    OPT_DIFFICULTY,
    OPT_GRAPHICS,
    OPT_FULLSCREEN,
    OPT_SHOW_FPS,
    OPT_RENDERER,
    OPT_COUNT
} OptionId;

typedef struct Button {
    int id;
    int x, y, w, h;
    int enabled;
    int visible;
    ButtonKind kind;
    OptionId opt;
    float slider_value;
    int dragging;
    char label[MAX_LABEL];
} Button;

typedef struct Options {
    float music;
    float sound;
    float sensitivity;
    int invert_mouse;
    int render_distance;
    int view_bobbing;
    int anaglyph; /* repurposed: V-Sync */
    int max_fps;  /* 1..200, 0 = unlimited */
    float fov;    /* vertical field of view in degrees */
    int difficulty;
    int fancy_graphics;
    int fullscreen;
    int show_fps;
    int renderer_backend; /* RendererBackend saved to options.txt; restart required after changing */
    int ignore_classic_resources_warning;
    char skin[64]; /* selected texture-pack name, kept for old options.txt compatibility */
    char skin_path[MAX_PATHBUF];
    char last_server[64];
    char username[32];
    int keys[10];
} Options;

#define PEX_GAMEPAD_MAX 8
#define PEX_GAMEPAD_DEADZONE 0.20f

typedef struct PexGamepadState {
    int connected;
    int slot;
    char name[128];
    char kind[48];
    int is_xbox;
    int is_dualshock;
    float lx, ly, rx, ry, lt, rt;
    int a, b, x, y;
    int lb, rb, back, start, guide;
    int ls, rs;
    int dpad_up, dpad_down, dpad_left, dpad_right;
    int prev_a, prev_b, prev_x, prev_y;
    int prev_lb, prev_rb, prev_back, prev_start;
    int prev_lt, prev_rt;
    int prev_dpad_up, prev_dpad_down, prev_dpad_left, prev_dpad_right;
} PexGamepadState;

typedef struct PexSystemInfo {
    char client_version[64];
    char platform[64];
    char renderer_backend[64];
    char gpu_vendor[128];
    char gpu_renderer[160];
    char gpu_version[128];
    char display_name[128];
    int display_refresh_hz;
    int screen_w, screen_h;
    unsigned long long ram_total_mb;
    unsigned long long ram_available_mb;
    int network_available;
} PexSystemInfo;


typedef struct PexNetRenderPlayerState {
    int active;
    int player_id;
    char name[32];
    float x, y, z;
    float prev_x, prev_y, prev_z;
    float yaw, pitch;
    float prev_yaw, prev_pitch;
    float move_amount;
    float limb_swing;
    float swing;
    float prev_swing;
    float swing_time;
    int swing_active;
    int swing_ticks;
    int health;
    float hurt_time;
    float death_time;
    int mining_active;
    int mining_x, mining_y, mining_z;
    int mining_stage;
    float mining_time;
    int flags;
    int skin_only;
    int has_skin;
    Texture skin;
} PexNetRenderPlayerState;

typedef struct PexNetRenderDropState {
    int active;
    int net_id;
    float x, y, z;
    float prev_x, prev_y, prev_z;
} PexNetRenderDropState;

typedef struct FlatFallingBlock {
    int active;
    int net_id;
    int block_id;
    float x, y, z;
    float prev_x, prev_y, prev_z;
    float mx, my, mz;
    int age;
    /* Multiplayer smooth-fall path supplied by the server.  Singleplayer uses
       the regular entity physics above; these fields stay zero there. */
    float start_x, start_y, start_z;
    float end_x, end_y, end_z;
    int path_id;
    int path_age;
    int path_duration;
    double path_start_time;
} FlatFallingBlock;

#ifdef PEX_PLATFORM_SDL2
static HINSTANCE g_inst;
static SDL_Window *g_hwnd;
static HDC g_hdc;
static SDL_GLContext g_glrc;
#else
static HINSTANCE g_inst;
static HWND g_hwnd;
static HDC g_hdc;
static HGLRC g_glrc;
#endif
static int g_win_w = 854, g_win_h = 480;
static int g_render_w = 854, g_render_h = 480; /* current framebuffer target; can be smaller than window */
static int g_gui_w = 427, g_gui_h = 240, g_gui_scale = 2;
static int g_fullscreen_active = 0;
static DWORD g_windowed_style = 0;
static DWORD g_windowed_exstyle = 0;
static RECT g_windowed_rect = {0, 0, 854, 480};
static int g_mouse_x = 0, g_mouse_y = 0;
static int g_mouse_down = 0;
static int g_running = 1;
static ScreenId g_screen = SCREEN_TITLE;
static ScreenId g_parent_screen = SCREEN_TITLE;
static Button g_buttons[MAX_BUTTONS];
static int g_button_count = 0;
static Button *g_drag_slider = NULL;
static Options g_opts;
static char g_mc_dir[MAX_PATHBUF];
static char g_save_dir[MAX_PATHBUF];
static char g_texpack_dir[MAX_PATHBUF];
static char g_skin_dir[MAX_PATHBUF];
static TexturePackEntry g_texpacks[MAX_TEXPACKS];
static int g_texpack_count = 0;
static int g_selected_texpack = 0;
static int g_texpack_scroll = 0;
static int g_runtime_renderer_backend = RENDERER_OPENGL;
static int g_selected_renderer_backend = RENDERER_OPENGL;
static int g_renderer_prompt_target_screen = SCREEN_TITLE;
static int g_renderer_backend_unavailable_notice = 0;
static int g_texpack_drag_anchor = -1;
static char g_current_texpack[MAX_LABEL] = "Default";
#if defined(PEX_PLATFORM_SDL2) || defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
static void *g_wic_factory = NULL;
#else
static IWICImagingFactory *g_wic_factory = NULL;
#endif
static char g_notice_title[MAX_LABEL] = "";
static char g_notice_line1[MAX_LABEL] = "";
static char g_notice_line2[MAX_LABEL] = "";
static char g_multiplayer_ip[64] = "";
static char g_multiplayer_username[32] = "Player";
static int g_multiplayer_edit_field = 0;
static char g_multiplayer_status[MAX_LABEL] = "";
static SOCKET g_mp_socket = INVALID_SOCKET;
static int g_mp_winsock_started = 0;
static int g_mp_connected = 0;
static int g_mp_connecting = 0;
static int g_mp_world_ready = 0;
static int g_mp_player_id = 0;
static int g_mp_expected_chunks = 0;
static int g_mp_chunks_received = 0;
static int g_mp_connect_progress = 0;
static char g_mp_server_name[96] = "";
static PexNetPlayerState g_mp_players[PEX_NET_MAX_PLAYERS];
static PexNetPlayerState g_mp_prev_players[PEX_NET_MAX_PLAYERS];
static PexNetRenderPlayerState g_mp_render_players[PEX_NET_MAX_PLAYERS];
static PexNetRenderDropState g_mp_render_drops[PEX_NET_MAX_DROPS];
static int g_mp_player_count = 0;
static double g_mp_interp_start_time = 0.0;
static double g_mp_interp_duration = 0.10;
static double g_mp_render_last_time = 0.0;
static int g_ticks = 0;
static int g_waiting_key = -1;
static int g_confirm_world = 0;
static int g_pending_world_slot = 0;
static int g_pending_world_type = 0;
static int g_world_type = 0; /* 0 flat, 1 normal/local terrain */
static long long g_world_seed = 0;
static WorldGenJob g_worldgen;
static int g_load_state_skip_terrain_rebuild = 0;
static int g_last_load_state_had_terrain = 0;
static char g_loaded_world_dir[MAX_PATHBUF] = "";
static char g_loaded_world_name[64] = "";
static int g_selected_hotbar_slot = 0;
static int g_player_health = 20;
static int g_player_prev_health = 20;
static int g_player_armor = 0;
static int g_ingame_ticks = 0;
static int g_hearts_life = 0;

static PexGamepadState g_gamepads[PEX_GAMEPAD_MAX];
static int g_gamepad_count = 0;
static int g_gamepad_primary = -1;
static int g_gamepad_vk_state[512];
static float g_gamepad_virtual_cursor_x = 213.0f;
static float g_gamepad_virtual_cursor_y = 120.0f;
static int g_gamepad_virtual_cursor_active = 0;
static int g_gamepad_menu_index = 0;
static double g_gamepad_nav_last_time = 0.0;
static double g_gamepad_slider_last_time = 0.0;
static double g_gamepad_click_last_time = 0.0;
static double g_gamepad_probe_last_time = -10.0;
static PexInputFocusMode g_input_focus_mode = PEX_INPUT_FOCUS_MOUSE;
static PexSystemInfo g_system_info;
static double g_system_info_last_time = -10.0;


/* Active in-memory terrain window.
   PC builds keep the larger 512x256x512 world window.
   PSP and Wii cannot use the PC-sized static terrain arrays: the three
   flat-world arrays alone are about 201 MB at 512*256*512.  Keep a separate
   low-memory console window so PPSSPP/real PSP and Wii homebrew can load. */
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_1000_TARGET) && PEX_PSP_1000_TARGET && !(defined(PEX_PSP_REAL_BETA_GEN) && PEX_PSP_REAL_BETA_GEN)
/* Real PSP-1000 only has 32MB main RAM.  Keep the 128x128 horizontal window
   for streaming/render-distance behavior, but cap vertical storage to 96 blocks
   so the three dense block/meta/liquid arrays save about 1.6MB.  The real Beta
   generator opt-in below needs the full 0..127 vertical range, so it bypasses
   this legacy safety cap. */
#define FLAT_WORLD_SIZE 128
#define FLAT_WORLD_Y_MIN 0
#define FLAT_WORLD_Y_MAX 95
#define MAX_DROP_ENTITIES 24
#define MAX_FALLING_BLOCK_ENTITIES 16
#else
#if defined(PEX_PLATFORM_WII)
/* Wii safety profile: keep the active terrain window very small until the
   GX/video path is proven stable.  This cuts the five dense block/light arrays
   from ~5.9MB at 96x128x96 to ~1.25MB at 64x64x64, leaving much more MEM1
   for libogc framebuffers and avoiding Dolphin DSI/XFB pointer failures during
   world generation. */
#define FLAT_WORLD_SIZE 64
#else
#define FLAT_WORLD_SIZE 128
#endif
#define FLAT_WORLD_Y_MIN 0
#if defined(PEX_PLATFORM_WII)
#define FLAT_WORLD_Y_MAX 63
#else
#define FLAT_WORLD_Y_MAX 127
#endif
#define MAX_DROP_ENTITIES 64
#define MAX_FALLING_BLOCK_ENTITIES 64
#endif
#else
#define FLAT_WORLD_SIZE 512
#define FLAT_WORLD_Y_MIN 0
#define FLAT_WORLD_Y_MAX 255
#define MAX_DROP_ENTITIES 256
#define MAX_FALLING_BLOCK_ENTITIES PEX_NET_MAX_FALLING_BLOCKS
#endif
#define FLAT_WORLD_MIN (-(FLAT_WORLD_SIZE / 2))
#define FLAT_WORLD_MAX ((FLAT_WORLD_SIZE / 2) - 1)
#define FLAT_WORLD_HEIGHT (FLAT_WORLD_Y_MAX - FLAT_WORLD_Y_MIN + 1)
#define ITEM_MAX_STACK 64
/* Block and item IDs from the uploaded deobfuscated Java client. */
#define BLOCK_STONE 1
#define BLOCK_GRASS 2
#define BLOCK_DIRT 3
#define BLOCK_COBBLESTONE 4
#define BLOCK_PLANKS 5
#define BLOCK_SAPLING 6
#define BLOCK_BEDROCK 7
#define BLOCK_WATER 8
#define BLOCK_STILL_WATER 9
#define BLOCK_LAVA 10
#define BLOCK_STILL_LAVA 11
#define BLOCK_SAND 12
#define BLOCK_GRAVEL 13
#define BLOCK_GOLD_ORE 14
#define BLOCK_IRON_ORE 15
#define BLOCK_COAL_ORE 16
#define BLOCK_LOG 17
#define BLOCK_LEAVES 18
#define BLOCK_SPONGE 19
#define BLOCK_GLASS 20
#define BLOCK_WOOL 35
#define BLOCK_YELLOW_FLOWER 37
#define BLOCK_RED_ROSE 38
#define BLOCK_BROWN_MUSHROOM 39
#define BLOCK_RED_MUSHROOM 40
#define BLOCK_GOLD_BLOCK 41
#define BLOCK_IRON_BLOCK 42
#define BLOCK_DOUBLE_SLAB 43
#define BLOCK_SLAB 44
#define BLOCK_BRICK 45
#define BLOCK_TNT 46
#define BLOCK_BOOKSHELF 47
#define BLOCK_MOSSY_COBBLESTONE 48
#define BLOCK_OBSIDIAN 49
#define BLOCK_TORCH 50
#define BLOCK_FIRE 51
#define BLOCK_MOB_SPAWNER 52
#define BLOCK_WOOD_STAIRS 53
#define BLOCK_CHEST 54
#define BLOCK_REDSTONE_WIRE 55
#define BLOCK_DIAMOND_ORE 56
#define BLOCK_DIAMOND_BLOCK 57
#define BLOCK_CRAFTING_TABLE 58
#define BLOCK_CROPS 59
#define BLOCK_FARMLAND 60
#define BLOCK_FURNACE 61
#define BLOCK_FURNACE_LIT 62
#define BLOCK_SIGN_POST 63
#define BLOCK_WOOD_DOOR 64
#define BLOCK_LADDER 65
#define BLOCK_RAILS 66
#define BLOCK_COBBLE_STAIRS 67
#define BLOCK_WALL_SIGN 68
#define BLOCK_LEVER 69
#define BLOCK_STONE_PRESSURE_PLATE 70
#define BLOCK_IRON_DOOR 71
#define BLOCK_WOOD_PRESSURE_PLATE 72
#define BLOCK_REDSTONE_ORE 73
#define BLOCK_REDSTONE_ORE_GLOWING 74
#define BLOCK_REDSTONE_TORCH_OFF 75
#define BLOCK_REDSTONE_TORCH_ON 76
#define BLOCK_STONE_BUTTON 77
#define BLOCK_SNOW_LAYER 78
#define BLOCK_ICE 79
#define BLOCK_SNOW_BLOCK 80
#define BLOCK_CACTUS 81
#define BLOCK_CLAY 82
#define BLOCK_REEDS 83
#define BLOCK_JUKEBOX 84
#define BLOCK_FENCE 85
#define BLOCK_PUMPKIN 86
#define BLOCK_NETHERRACK 87
#define BLOCK_SOUL_SAND 88
#define BLOCK_GLOWSTONE 89
#define BLOCK_PORTAL 90
#define BLOCK_JACK_O_LANTERN 91
#define ITEM_SHOVEL_IRON 256
#define ITEM_PICKAXE_IRON 257
#define ITEM_AXE_IRON 258
#define ITEM_FLINT_AND_IRON 259
#define ITEM_APPLE_RED 260
#define ITEM_BOW 261
#define ITEM_ARROW 262
#define ITEM_COAL 263
#define ITEM_DIAMOND 264
#define ITEM_INGOT_IRON 265
#define ITEM_INGOT_GOLD 266
#define ITEM_SWORD_IRON 267
#define ITEM_WOODEN_SWORD 268
#define ITEM_WOODEN_SHOVEL 269
#define ITEM_WOODEN_PICKAXE 270
#define ITEM_WOODEN_AXE 271
#define ITEM_STONE_SWORD 272
#define ITEM_STONE_SHOVEL 273
#define ITEM_STONE_PICKAXE 274
#define ITEM_STONE_AXE 275
#define ITEM_SWORD_DIAMOND 276
#define ITEM_SHOVEL_DIAMOND 277
#define ITEM_PICKAXE_DIAMOND 278
#define ITEM_AXE_DIAMOND 279
#define ITEM_STICK 280
#define ITEM_BOWL_EMPTY 281
#define ITEM_BOWL_SOUP 282
#define ITEM_SWORD_GOLD 283
#define ITEM_SHOVEL_GOLD 284
#define ITEM_PICKAXE_GOLD 285
#define ITEM_AXE_GOLD 286
#define ITEM_STRING 287
#define ITEM_FEATHER 288
#define ITEM_GUNPOWDER 289
#define ITEM_HOE_WOOD 290
#define ITEM_HOE_STONE 291
#define ITEM_HOE_IRON 292
#define ITEM_HOE_DIAMOND 293
#define ITEM_HOE_GOLD 294
#define ITEM_SEEDS 295
#define ITEM_WHEAT 296
#define ITEM_BREAD 297
#define ITEM_HELMET_LEATHER 298
#define ITEM_PLATE_LEATHER 299
#define ITEM_LEGS_LEATHER 300
#define ITEM_BOOTS_LEATHER 301
#define ITEM_HELMET_CHAIN 302
#define ITEM_PLATE_CHAIN 303
#define ITEM_LEGS_CHAIN 304
#define ITEM_BOOTS_CHAIN 305
#define ITEM_HELMET_IRON 306
#define ITEM_PLATE_IRON 307
#define ITEM_LEGS_IRON 308
#define ITEM_BOOTS_IRON 309
#define ITEM_HELMET_DIAMOND 310
#define ITEM_PLATE_DIAMOND 311
#define ITEM_LEGS_DIAMOND 312
#define ITEM_BOOTS_DIAMOND 313
#define ITEM_HELMET_GOLD 314
#define ITEM_PLATE_GOLD 315
#define ITEM_LEGS_GOLD 316
#define ITEM_BOOTS_GOLD 317
#define ITEM_FLINT 318
#define ITEM_PORK_RAW 319
#define ITEM_PORK_COOKED 320
#define ITEM_PAINTING 321
#define ITEM_APPLE_GOLD 322
#define ITEM_SIGN 323
#define ITEM_DOOR_WOOD 324
#define ITEM_BUCKET_EMPTY 325
#define ITEM_BUCKET_WATER 326
#define ITEM_BUCKET_LAVA 327
#define ITEM_MINECART_EMPTY 328
#define ITEM_SADDLE 329
#define ITEM_DOOR_IRON 330
#define ITEM_REDSTONE 331
#define ITEM_SNOWBALL 332
#define ITEM_BOAT 333
#define ITEM_LEATHER 334
#define ITEM_BUCKET_MILK 335
#define ITEM_BRICK 336
#define ITEM_CLAY 337
#define ITEM_REED 338
#define ITEM_PAPER 339
#define ITEM_BOOK 340
#define ITEM_SLIME_BALL 341
#define ITEM_MINECART_CRATE 342
#define ITEM_MINECART_POWERED 343
#define ITEM_EGG 344
#define ITEM_COMPASS 345
#define ITEM_FISHING_ROD 346
#define ITEM_CLOCK 347
#define ITEM_GLOWSTONE_DUST 348
#define ITEM_FISH_RAW 349
#define ITEM_FISH_COOKED 350
#define ITEM_RECORD13 2256
#define ITEM_RECORD_CAT 2257
/* Legacy/alias IDs kept so old save/tool code still compiles while the Java names are available. */
#define BLOCK_LAPIS_ORE 21
#define BLOCK_LAPIS_BLOCK 22
#define BLOCK_WOOD_PLANKS BLOCK_PLANKS
#define ITEM_IRON_SHOVEL ITEM_SHOVEL_IRON
#define ITEM_IRON_PICKAXE ITEM_PICKAXE_IRON
#define ITEM_IRON_AXE ITEM_AXE_IRON
#define ITEM_IRON_SWORD ITEM_SWORD_IRON
#define ITEM_DIAMOND_SWORD ITEM_SWORD_DIAMOND
#define ITEM_DIAMOND_SHOVEL ITEM_SHOVEL_DIAMOND
#define ITEM_DIAMOND_PICKAXE ITEM_PICKAXE_DIAMOND
#define ITEM_DIAMOND_AXE ITEM_AXE_DIAMOND
#define ITEM_GOLD_SWORD ITEM_SWORD_GOLD
#define ITEM_GOLD_SHOVEL ITEM_SHOVEL_GOLD
#define ITEM_GOLD_PICKAXE ITEM_PICKAXE_GOLD
#define ITEM_GOLD_AXE ITEM_AXE_GOLD
#define ITEM_WOODEN_HOE ITEM_HOE_WOOD
#define ITEM_STONE_HOE ITEM_HOE_STONE
#define ITEM_IRON_HOE ITEM_HOE_IRON
#define ITEM_DIAMOND_HOE ITEM_HOE_DIAMOND
#define ITEM_GOLD_HOE ITEM_HOE_GOLD
#define ITEM_LEATHER_CAP ITEM_HELMET_LEATHER
#define ITEM_LEATHER_CHESTPLATE ITEM_PLATE_LEATHER
#define ITEM_LEATHER_LEGGINGS ITEM_LEGS_LEATHER
#define ITEM_LEATHER_BOOTS ITEM_BOOTS_LEATHER
#define ITEM_CHAIN_HELMET ITEM_HELMET_CHAIN
#define ITEM_CHAIN_CHESTPLATE ITEM_PLATE_CHAIN
#define ITEM_CHAIN_LEGGINGS ITEM_LEGS_CHAIN
#define ITEM_CHAIN_BOOTS ITEM_BOOTS_CHAIN
#define ITEM_IRON_HELMET ITEM_HELMET_IRON
#define ITEM_IRON_CHESTPLATE ITEM_PLATE_IRON
#define ITEM_IRON_LEGGINGS ITEM_LEGS_IRON
#define ITEM_IRON_BOOTS ITEM_BOOTS_IRON
#define ITEM_DIAMOND_HELMET ITEM_HELMET_DIAMOND
#define ITEM_DIAMOND_CHESTPLATE ITEM_PLATE_DIAMOND
#define ITEM_DIAMOND_LEGGINGS ITEM_LEGS_DIAMOND
#define ITEM_DIAMOND_BOOTS ITEM_BOOTS_DIAMOND
#define ITEM_GOLD_HELMET ITEM_HELMET_GOLD
#define ITEM_GOLD_CHESTPLATE ITEM_PLATE_GOLD
#define ITEM_GOLD_LEGGINGS ITEM_LEGS_GOLD
#define ITEM_GOLD_BOOTS ITEM_BOOTS_GOLD
#define ITEM_MINECART ITEM_MINECART_EMPTY
#define ITEM_WOOD_DOOR ITEM_DOOR_WOOD
#define ITEM_IRON_DOOR ITEM_DOOR_IRON
#define ITEM_WATER_BUCKET ITEM_BUCKET_WATER
#define ITEM_LAVA_BUCKET ITEM_BUCKET_LAVA
#define ITEM_MILK ITEM_BUCKET_MILK
#define ITEM_MUSHROOM_STEW ITEM_BOWL_SOUP


typedef struct ItemStack {
    int id;
    int count;
    int damage;
    int pop_time;
} ItemStack;

typedef struct FlatDroppedItem {
    int active;
    int net_id;
    ItemStack stack;
    float x, y, z;
    float prev_x, prev_y, prev_z;
    float mx, my, mz;
    float rot;
    int age;
    int pickup_delay;
    int on_ground;
} FlatDroppedItem;

typedef struct PickupFx {
    int active;
    ItemStack stack;
    float start_x, start_y, start_z;
    float prev_player_x, prev_player_y, prev_player_z;
    int age;
    int max_age;
    float rot;
} PickupFx;

#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
#define MAX_CHEST_TILES 64
#else
#define MAX_CHEST_TILES 1024
#endif
typedef struct ChestTile {
    int active;
    int x, y, z;
    ItemStack slots[27];
} ChestTile;

#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
#define MAX_BUTTON_TIMERS 32
#else
#define MAX_BUTTON_TIMERS 128
#endif
typedef struct ButtonTimer {
    int active;
    int x, y, z;
    int ticks_left;
} ButtonTimer;

#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
#define MAX_PRESSURE_PLATE_TIMERS 32
#else
#define MAX_PRESSURE_PLATE_TIMERS 128
#endif
typedef struct PressurePlateTimer {
    int active;
    int x, y, z;
    int ticks_left;
} PressurePlateTimer;

#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
#define MAX_FURNACE_TILES 64
#else
#define MAX_FURNACE_TILES 1024
#endif
typedef struct FurnaceTile {
    int active;
    int x, y, z;
    ItemStack slots[3]; /* 0=input, 1=fuel, 2=output */
    int burn_time;
    int current_item_burn_time;
    int cook_time;
} FurnaceTile;

typedef struct FlatRayHit {
    int hit;
    int bx, by, bz;
    int face;
    float hx, hy, hz;
} FlatRayHit;

static unsigned char g_flat_blocks[FLAT_WORLD_HEIGHT][FLAT_WORLD_SIZE][FLAT_WORLD_SIZE];
static unsigned char g_flat_meta[FLAT_WORLD_HEIGHT][FLAT_WORLD_SIZE][FLAT_WORLD_SIZE];
/* Parallel fluid level array (Beta water/lava metadata): 0 = source,
   1..7 = flow decay (higher = farther), bit 0x8 = falling. 0 for non-fluid. */
static unsigned char g_flat_levels[FLAT_WORLD_HEIGHT][FLAT_WORLD_SIZE][FLAT_WORLD_SIZE];
/* Java-style saved light arrays for the active streamed world.  Values are 0..15,
   stored as bytes instead of nibbles so section snapshots can read them without
   bit packing. */
static unsigned char g_flat_sky_light[FLAT_WORLD_HEIGHT][FLAT_WORLD_SIZE][FLAT_WORLD_SIZE];
static unsigned char g_flat_block_light[FLAT_WORLD_HEIGHT][FLAT_WORLD_SIZE][FLAT_WORLD_SIZE];
/* Worker-side mesh building uses a tiny thread-local section snapshot so the
   mesh worker never reads the live world arrays while the game thread mutates
   them.  When these pointers are NULL, flat_get_block/meta read normal globals. */
static PEX_THREAD_LOCAL const unsigned char *g_async_mesh_blocks = NULL;
static PEX_THREAD_LOCAL const unsigned char *g_async_mesh_meta = NULL;
static PEX_THREAD_LOCAL const unsigned char *g_async_mesh_levels = NULL;
static PEX_THREAD_LOCAL const unsigned char *g_async_mesh_sky_light = NULL;
static PEX_THREAD_LOCAL const unsigned char *g_async_mesh_block_light = NULL;
static PEX_THREAD_LOCAL int g_async_mesh_x0 = 0, g_async_mesh_y0 = 0, g_async_mesh_z0 = 0;
static PEX_THREAD_LOCAL int g_async_mesh_w = 0, g_async_mesh_h = 0, g_async_mesh_d = 0;
static PEX_THREAD_LOCAL int g_async_mesh_origin_override = 0;
static PEX_THREAD_LOCAL int g_async_mesh_origin_x = 0, g_async_mesh_origin_z = 0;
static GLuint g_flat_world_display_list = 0; /* legacy unused single-list slot */
#define FLAT_RENDER_CHUNK 16
#define FLAT_RENDER_CHUNKS ((FLAT_WORLD_SIZE + FLAT_RENDER_CHUNK - 1) / FLAT_RENDER_CHUNK)
static GLuint g_flat_world_chunk_lists[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
static GLuint g_flat_world_chunk_liquid_lists[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
static int g_flat_world_chunk_dirty[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
/* Dirty means the cached mesh is out of date, but it may still be safe to draw
   for a frame.  Valid distinguishes that from newly-streamed chunks whose list
   contains geometry for old coordinates and must not be shown. */
static int g_flat_world_chunk_valid[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
static int g_flat_world_chunk_has_liquid[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
/* Generated means terrain data has actually been filled for this chunk in the
   active window.  Mesh validity is separate; ungenerated chunks must not be
   treated as empty finished chunks or the loader can waste time warming up
   huge air-only areas at high render distance. */
static int g_flat_world_chunk_generated[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];

/* Java-b1 style render sections.  A section is 16x16x16, so editing or
   generating one part of a chunk no longer rebuilds the whole 256-block-tall
   column.  Pass 0 is solid/cutout geometry, pass 1 is fancy liquids. */
#define FLAT_RENDER_SECTION 16
#define FLAT_RENDER_SECTIONS_Y ((FLAT_WORLD_HEIGHT + FLAT_RENDER_SECTION - 1) / FLAT_RENDER_SECTION)
static GLuint g_flat_section_lists[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS][2];
static unsigned int g_flat_section_direct_mesh[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS][2];
static int g_flat_section_dirty[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
static int g_flat_section_valid[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
static unsigned int g_flat_section_mesh_version[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
static int g_flat_section_mesh_building[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
static int g_flat_section_skip_pass[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS][2];
/* Minecraft stores a 16x16x16 renderer on top of section-aware chunk storage.
   Pexcraft's active world is a dense byte volume, so repeatedly scanning a
   whole section just to learn if it is air caused streaming hitches.  This
   mask is the section-existence cache: bit sy is set when local chunk (cx,cz)
   contains any non-air block in that 16-high section. */
static unsigned short g_flat_chunk_section_non_empty_mask[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
static int g_flat_renderer_sort_dirty = 1;
static int g_flat_section_geometry_dirty = 1;
/* Per-window persistence flag.  Generated chunks are deterministic from the
   world seed; only chunks touched by player edits or block simulation need
   delta files on disk. */
static int g_flat_world_chunk_modified[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
static int g_flat_world_geometry_dirty = 1;
static int g_flat_world_origin_x = FLAT_WORLD_MIN;
static int g_flat_world_origin_z = FLAT_WORLD_MIN;
static int g_stream_last_center_chunk_x = 999999;
static int g_stream_last_center_chunk_z = 999999;
#define STREAM_GEN_QUEUE_MAX (FLAT_RENDER_CHUNKS * FLAT_RENDER_CHUNKS)
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
#define STREAM_CHUNKS_PER_TICK 1
#else
#define STREAM_CHUNKS_PER_TICK 2
#endif
static int g_stream_gen_queue_cx[STREAM_GEN_QUEUE_MAX];
static int g_stream_gen_queue_cz[STREAM_GEN_QUEUE_MAX];
static int g_stream_gen_queue_count = 0;
static int g_stream_gen_queue_index = 0;
static int g_stream_gen_queue_installed_count = 0;
static int g_stream_generation_keep_completed = 0;
static int g_stream_generation_epoch = 1;
/* PSP real-world streaming can leave the player briefly over a not-yet-installed
   terrain chunk.  These flags let respawn/physics wait for the worker instead
   of falling into air or doing a full blocking spawn scan on the main thread. */
static int g_psp_respawn_snap_pending = 0;
static int g_psp_terrain_wait_chat_tick = -1000;
#if defined(PEX_PLATFORM_PSP)
/* PSP-only spawn/platform guard.  Real Beta terrain generation is async on PSP,
   so first spawn/respawn needs a tiny, explicit solid surface until the real
   chunk data and collision are ready.  The marker is stored in block metadata
   for the on-ground platform blocks so collision/spawn code can recognize the
   PSP safety floor without affecting desktop worlds. */
#define PSP_SPAWN_SURFACE_META 15
static int g_psp_spawn_surface_guard_active = 0;
static int g_psp_spawn_surface_x0 = 0;
static int g_psp_spawn_surface_x1 = 0;
static int g_psp_spawn_surface_z0 = 0;
static int g_psp_spawn_surface_z1 = 0;
static int g_psp_spawn_surface_ground_y = 0;
#endif
#define AUTOSAVE_INTERVAL_TICKS 600
#define SAVE_MESSAGE_TICKS 40
static int g_save_dirty = 0;
static int g_last_autosave_tick = 0;
static int g_save_message_ticks = 0;
static ItemStack g_inventory[36];
static ItemStack g_craft_grid[4];
static ItemStack g_workbench_grid[9];
/* Legacy global chest inventory kept only to migrate old v9-v13 saves that
   had one shared chest, which behaved like an ender chest.  New saves store
   one ChestTile inventory per chest coordinate. */
static ItemStack g_chest_slots[27];
static int g_legacy_global_chest_pending = 0;
static ChestTile g_chest_tiles[MAX_CHEST_TILES];
static int g_open_chest_indices[2];
static int g_open_chest_count = 0;
static int g_open_chest_rows = 3;
static char g_open_chest_title[32] = "Chest";
static ButtonTimer g_button_timers[MAX_BUTTON_TIMERS];
static PressurePlateTimer g_pressure_plate_timers[MAX_PRESSURE_PLATE_TIMERS];
static FurnaceTile g_furnace_tiles[MAX_FURNACE_TILES];
static int g_open_furnace_index = -1;
static ItemStack g_carried_stack;
static ItemStack g_equipped_item;
static int g_equipped_slot = -1;
static float g_equipped_progress = 0.0f;
static float g_prev_equipped_progress = 0.0f;
static FlatDroppedItem g_drops[MAX_DROP_ENTITIES];
#define MAX_PICKUP_FX 32
static PickupFx g_pickup_fx[MAX_PICKUP_FX];
static FlatFallingBlock g_falling_blocks[MAX_FALLING_BLOCK_ENTITIES];

typedef struct PexSaveChunkSnapshot {
    int cx, cz;
    unsigned char *blocks;
    unsigned char *meta;
} PexSaveChunkSnapshot;

#define MAX_SAVE_CHUNK_SNAPSHOTS (FLAT_RENDER_CHUNKS * FLAT_RENDER_CHUNKS)
typedef struct PexSaveSnapshot {
    int write_world_state;
    int chunk_count;
    PexSaveChunkSnapshot chunks[MAX_SAVE_CHUNK_SNAPSHOTS];
    int flat_world_origin_x;
    int flat_world_origin_z;
    long long world_seed;
    int world_type;
    float player_x, player_y, player_z;
    float player_yaw, player_pitch;
    int player_health;
    int player_armor;
    int selected_hotbar_slot;
    float player_fall_distance;
    int player_dead;
    ItemStack inventory[36];
    ChestTile chest_tiles[MAX_CHEST_TILES];
    FurnaceTile furnace_tiles[MAX_FURNACE_TILES];
    FlatDroppedItem drops[MAX_DROP_ENTITIES];
    char loaded_world_dir[MAX_PATHBUF];
    int ingame_ticks;
    struct PexSaveSnapshot *next;
} PexSaveSnapshot;

static HANDLE g_save_thread = NULL;
static HANDLE g_save_event = NULL;
static volatile int g_save_thread_done = 1;
static volatile int g_save_thread_shutdown = 0;
static PexSaveSnapshot *g_save_queue_head = NULL;
static PexSaveSnapshot *g_save_queue_tail = NULL;
static CRITICAL_SECTION g_save_cs;

#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
#define MAX_DIG_PARTICLES 96
#else
#define MAX_DIG_PARTICLES 384
#endif
typedef enum ParticleKind { PARTICLE_DIG = 0, PARTICLE_BUBBLE = 1, PARTICLE_SPLASH = 2 } ParticleKind;

typedef struct DigParticle {
    int active;
    int tile;
    int kind;
    float x, y, z;
    float prev_x, prev_y, prev_z;
    float mx, my, mz;
    float r, g, b;
    float scale;
    float gravity;
    int age;
    int max_age;
    float tex_u_jitter;
    float tex_v_jitter;
} DigParticle;
static DigParticle g_dig_particles[MAX_DIG_PARTICLES];
static int g_next_dig_particle = 0;
static int g_last_hit_particle_x = 0, g_last_hit_particle_y = -9999, g_last_hit_particle_z = 0;
static int g_last_hit_particle_face = -1;
static int g_last_hit_particle_tick = -9999;

static int g_breaking_block = 0;
static int g_break_x = 0, g_break_y = -1, g_break_z = 0, g_break_face = 1;
static float g_break_damage = 0.0f;
static float g_prev_break_damage = 0.0f;
static float g_break_sound_counter = 0.0f;
static int g_block_hit_delay = 0;
static float g_hand_swing = 0.0f;
static float g_prev_hand_swing = 0.0f;
static int g_hand_swing_ticks = 0;
static float g_hand_swing_progress = 0.0f;
static int g_hand_swing_active = 0;
/* Air-click one-shot swing from the current branch:
   clicking air starts one swing, holding does not loop it. */
static int g_air_swing_playing = 0;
static int g_air_swing_ticks = 0;
static int g_air_swing_consumed = 0;
/* Block-breaking swing one-shot while LMB is held: play one full cycle, do not loop/flicker. */
static int g_break_swing_consumed = 0;
static int g_break_swing_holding = 0;
static int g_last_sent_mine_stage = -1;

/* Minimal Beta 1.0 player/camera state for the flat 256-high in-game world.
   Names mirror the decompiled fields conceptually:
   yaw/pitch = Entity rotationYaw/rotationPitch, distance_walked/camera_* feed view bobbing. */
static float g_player_x = 50.5f, g_player_y = 5.62f, g_player_z = 50.5f;
static float g_player_prev_x = 50.5f, g_player_prev_y = 5.62f, g_player_prev_z = 50.5f;
static float g_player_motion_x = 0.0f, g_player_motion_y = 0.0f, g_player_motion_z = 0.0f;
static float g_player_fall_distance = 0.0f;
static int g_player_dead = 0;
static int g_player_death_time = 0;
static int g_player_hurt_time = 0;
static int g_player_max_hurt_time = 10;
static float g_player_attacked_at_yaw = 0.0f;
static int g_suffocation_damage_timer = 0;
static float g_player_yaw = 0.0f, g_player_pitch = 0.0f;
static float g_player_prev_yaw = 0.0f, g_player_prev_pitch = 0.0f;
static int g_player_on_ground = 1;
static int g_player_was_in_water = 0;
static float g_distance_walked = 0.0f, g_prev_distance_walked = 0.0f;
static float g_limb_swing = 0.0f, g_prev_limb_swing = 0.0f;
static float g_limb_swing_amount = 0.0f, g_prev_limb_swing_amount = 0.0f;
static float g_camera_yaw = 0.0f, g_prev_camera_yaw = 0.0f;
static float g_camera_pitch = 0.0f, g_prev_camera_pitch = 0.0f;
static float g_frame_partial = 0.0f;

/* F3 debug stats and F5 third-person toggle. */
static int g_debug_fps = 0;
static int g_debug_min_fps = 0;
static int g_debug_max_fps = 0;
static int g_debug_frame_counter = 0;
static double g_debug_fps_last_time = 0.0;
static double g_ft_history[128];
static int g_ft_history_head = 0;
static double g_ft_last_frame_ms = 0.0;
static double g_ft_p50_ms = 0.0;
static double g_ft_p95_ms = 0.0;
static double g_ft_p99_ms = 0.0;
static double g_ft_worst_ms = 0.0;
static double g_sleep_ms_last = 0.0;
static double g_render_ms_last = 0.0;

typedef enum PexMainThreadProfileId {
    PROF_PUMP = 0,
    PROF_NET_POLL,
    PROF_TICK_TOTAL,
    PROF_WORLDGEN_TICK,
    PROF_INGAME_TOTAL,
    PROF_INVENTORY,
    PROF_FURNACE,
    PROF_DROPS,
    PROF_PARTICLES,
    PROF_FALLING,
    PROF_WORLD_STREAM,
    PROF_LIQUIDS,
    PROF_BUTTONS,
    PROF_PLAYER_LOGIC,
    PROF_MESH_MAIN,
    PROF_CULL_SORT,
    PROF_WORLD_DRAW,
    PROF_HUD_GUI,
    PROF_RENDER_TOTAL,
    PROF_COUNT
} PexMainThreadProfileId;

static const char *g_prof_names[PROF_COUNT] = {
    "Pump/messages",
    "Net poll/apply",
    "Tick total",
    "Worldgen tick",
    "Ingame tick",
    "Inventory",
    "Furnaces",
    "Drops/items",
    "Particles",
    "Sand+gravel",
    "World streaming",
    "Liquids",
    "Buttons/plates",
    "Player logic",
    "Mesh main",
    "Cull/sort",
    "World draw",
    "HUD/GUI",
    "Render total"
};

static double g_prof_frame_ms[PROF_COUNT];
static double g_prof_accum_ms[PROF_COUNT];
static double g_prof_display_ms[PROF_COUNT];
static double g_prof_frame_start_time = 0.0;
static double g_prof_accum_frame_ms = 0.0;
static double g_prof_display_frame_ms = 0.0;
static double g_prof_accum_start_time = 0.0;
static int g_prof_accum_frames = 0;
static int g_prof_packets_last = 0;
static int g_prof_chunks_last = 0;
static int g_prof_rx_queue_last = 0;
static int g_prof_mesh_jobs_last = 0;
static int g_prof_mesh_results_last = 0;
static int g_prof_mesh_uploads_last = 0;
static int g_prof_falling_cells_last = 0;
static int g_prof_falling_spawns_last = 0;
static int g_prof_falling_active_last = 0;
static int g_prof_stream_pending_last = 0;

static int g_third_person_view = 0;
static int g_debug_menu_shown = 0;
static int g_mouse_grabbed = 0;
static int g_cursor_hidden = 0;
static int g_recentering_mouse = 0;
static char g_chat_input[128] = "";
static int g_suppress_next_chat_char = 0;
#define MAX_CHAT_LINES 50
typedef struct ChatLine { char text[256]; int age; } ChatLine;
static ChatLine g_chat_lines[MAX_CHAT_LINES];
static int g_chat_count = 0;
static unsigned int g_crc_table[256];
static int g_crc_table_ready = 0;
static char g_splash[MAX_LABEL] = "Finally beta!";
static double g_last_time = 0.0;

static Texture tex_bg, tex_gui, tex_font, tex_terrain, tex_black, tex_pack, tex_default_pack_icon, tex_unknown_pack;
static Texture tex_icons, tex_inventory, tex_workbench, tex_furnace_gui, tex_chest_gui, tex_items, tex_steve;
static Texture tex_chest_entity, tex_large_chest_entity, tex_clouds;
static Texture tex_water_overlay, tex_shadow, tex_grasscolor, tex_foliagecolor, tex_particles;
static int font_widths[256];

static const char *TITLE_BLOCKS[5] = {
    "    *** *** * * *** **   *  *** ***    ",
    "    * * *   * * *   * * * * *    *     ",
    "    *** **   *  *   **  *** **   *     ",
    "    *   *   * * *   * * * * *    *     ",
    "    *   *** * * *** * * * * *    *     "
};
#define TITLE_COLS 39
#define TITLE_ROWS 5
static double title_z[TITLE_COLS][TITLE_ROWS];
static double title_z_prev[TITLE_COLS][TITLE_ROWS];
static double title_vel[TITLE_COLS][TITLE_ROWS];
static int title_inited = 0;

static const char *FONT_CHARS = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_'abcdefghijklmnopqrstuvwxyz{|}~";
static const char *opt_names[OPT_COUNT] = {
    "Music", "Sound", "Invert Mouse", "Sensitivity", "Render Distance",
    "View Bobbing", "V-Sync", "Max FPS", "FOV", "Difficulty", "Graphics",
    "Enable Fullscreen", "Show FPS Counter", "Renderer"
};
static const int opt_is_slider[OPT_COUNT] = {1,1,0,1,1,0,0,1,1,0,0,0,0,0};
static const int opt_is_boolean[OPT_COUNT] = {0,0,1,0,0,1,1,0,0,0,0,1,1,0};
#ifdef PEX_PLATFORM_SDL2
static const char *renderer_backend_names[RENDERER_COUNT] = {"SDL2/OpenGL", "Direct3D 9", "Direct3D 11"};
#else
static const char *renderer_backend_names[RENDERER_COUNT] = {"OpenGL", "Direct3D 9", "Direct3D 11"};
#endif
static const char *renderer_backend_keys[RENDERER_COUNT] = {"opengl", "d3d9", "d3d11"};
static const char *render_distance_names[4] = {"Far", "Normal", "Short", "Tiny"};
static const char *difficulty_names[4] = {"Peaceful", "Easy", "Normal", "Hard"};
static const char *key_action_names[10] = {"Forward", "Left", "Back", "Right", "Jump", "Sneak", "Drop", "Inventory", "Chat", "Toggle Fog"};
static const int default_keys[10] = {'W','A','S','D',VK_SPACE,VK_SHIFT,'Q','I','T','F'};
static int lwjgl_to_vk(int code);
static int vk_to_lwjgl(int vk);
static void init_font_widths(void);
static void save_options(void);
static void set_screen(ScreenId s);
static int load_custom_skin_path(const char *path, int persist);
static int choose_and_import_skin(void);
static const char *tr(const char *en);
static void open_notice(const char *title, const char *line1, const char *line2);
static int renderer_backend_supported(int backend);
static const char *renderer_backend_label(int backend);
static void restart_application_now(void);
static void start_world_generation(int slot);
static void worldgen_tick(void);
static int classic_pack_installed(void);
static int classic_pack_missing_required_textures(void);
static int should_show_classic_pack_download_prompt(void);
static void start_classic_pack_install(void);
static void classic_pack_install_tick(void);
static void enter_world_from_job(void);
static void ingame_tick(void);
static void hud_add_chat(const char *msg);
static void reset_flat_player_spawn(void);
static void start_air_swing_once(void);
static void set_mouse_grabbed(int grabbed);
static void apply_vsync_setting(void);
static void set_fullscreen_enabled(int enabled);
static void toggle_fullscreen(void);
static void handle_grabbed_mouse_move(int px, int py);
static void flat_world_reset_blocks(void);
static void flat_world_center_origin_near(float px, float pz);
static void flat_world_generate_blocks_for_current_origin(void);
static void flat_world_prepare_initial_generation(void);
static void flat_world_begin_initial_generation(void);
static void flat_world_continue_initial_generation(void);
static int flat_world_initial_generation_active(void);
static int flat_world_initial_generation_total(void);
static int flat_world_initial_generation_done(void);
static void flat_world_finish_initial_generation(void);
static void mark_flat_render_chunks_dirty_all(void);
static void worldgen_mesh_prep_reset(void);
static int worldgen_mesh_prep_step(int max_rebuilds);
static void flat_gl_cpu_mesh_remap_after_shift(int old_origin_x, int old_origin_z, int new_origin_x, int new_origin_z);
static int worldgen_mesh_prep_total(void);
static int worldgen_mesh_prep_done(void);
static void inventory_reset(void);
static void inventory_tick(void);
static void inventory_drop_selected_one(void);
static void inventory_mouse_click(int mx, int my, int button);
static ItemStack inventory_crafting_output(void);
static ItemStack *chest_get_open_slot_ptr(int local_slot);
static int chest_open_slot_count(void);
static int chest_can_place_at(int x, int y, int z);
static void chest_on_block_placed(int x, int y, int z);
static int chest_open_at(int x, int y, int z);
static void chest_drop_contents_and_remove_at(int x, int y, int z);
static int chest_slots_have_items(const ItemStack *slots, int n);
static void chest_clear_all_tiles(void);
static void chest_close_open_inventory(void);
static FurnaceTile *furnace_open_tile(void);
static ItemStack *furnace_get_slot_ptr(int local_slot);
static int furnace_open_at(int x, int y, int z);
static void furnace_on_block_placed(int x, int y, int z);
static void furnace_drop_contents_and_remove_at(int x, int y, int z);
static void furnace_clear_all_tiles(void);
static void furnace_close_open_inventory(void);
static void furnace_tick_all(void);
static int furnace_burn_scaled(int pixels);
static int furnace_cook_scaled(int pixels);
static void world_left_mouse_released(void);
static void ingame_right_click(void);
static void spawn_block_destroy_particles(int bx, int by, int bz, int block_id);
static void spawn_block_hit_particle(int bx, int by, int bz, int face, int block_id);
static void spawn_water_entry_particles(float x, float y, float z, float mx, float mz);
static void update_dig_particles(void);
static void draw_item_stack_gui(const ItemStack *st, int x, int y);
static void draw_item_stack_gui_animated(const ItemStack *st, int x, int y);
static void draw_carried_stack(void);
static void update_breaking(void);
static void update_dropped_items(void);
static void update_equipped_item(void);
static int flat_player_aabb_collides(float px, float py, float pz);
static int flat_player_has_sneak_support(float px, float py, float pz);
static int flat_player_has_water_exit_ledge(float px, float py, float pz, float dx, float dz);
static int flat_block_intersects_player(int bx, int by, int bz);
static void trigger_hand_swing(void);
static void restart_hand_swing(void);
static void player_take_damage(int amount, const char *reason);
static void player_die(const char *reason);
static void player_respawn(void);
static int flat_player_suffocation_block(void);
static int flat_player_head_block(void);
static int flat_player_head_in_water(void);
static int flat_player_head_in_lava(void);
static int flat_block_is_underwater_target(int bx, int by, int bz);
static void apply_player_fluid_velocity(int is_water);
static void update_falling_blocks(void);
static void update_liquids(void);
static void update_infinite_world_streaming(void);
static void world_stream_service_ensure(void);
static int world_stream_service_active(void);
static void stream_generation_queue_clear(void);
static int item_icon_tile(int id);
static int write_level_dat(const char *world_dir, const char *world_name, long long seed, int spawn_x, int spawn_y, int spawn_z, long long size_on_disk);
static int write_session_lock(const char *world_dir);
static void save_modified_flat_chunks(void);
static void save_modified_flat_chunks_sync(void);
static void pex_request_modified_chunk_save_async(void);
static void load_modified_flat_chunk_delta_into_flat(int cx, int cz);
static void mark_flat_chunks_modified_all(void);
static void async_section_mesh_shutdown(void);
static int pex_net_connect_to_server(const char *server);
static void pex_net_poll(void);
static int pex_net_is_connecting(void);
static void pex_net_update_smoothing(void);
static void pex_net_disconnect(void);
static void pex_net_send_player_state(void);
static void pex_net_send_block_action(int action, int x, int y, int z, int face, int block_id);
static void pex_net_send_player_action(int action, int x, int y, int z, int face, int block_id);
static void pex_net_send_player_action_progress(int action, int x, int y, int z, int face, int block_id, int progress);
static int pex_net_player_ray_distance(float max_dist, float *out_t);
static int pex_net_try_attack_player(void);
static void pex_net_send_drop_item(ItemStack st);
static void pex_net_send_pickup_drop(int drop_id);
static void pex_net_send_chest_open(void);
static void pex_net_send_chest_update(void);
static void pex_net_send_craft_request(void);
static void pex_net_send_chat(const char *text);
static void pex_net_send_skin(void);

#endif /* MC_B1_SPLIT_COMMON_H */
