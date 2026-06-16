#ifndef MC_B1_SPLIT_COMMON_H
#define MC_B1_SPLIT_COMMON_H

// PEXCRAFT Beta 1.0 title/options/world UI port to C/Win32/OpenGL.
// Build with MinGW32: gcc -std=c99 -O2 -mwindows main.c -o pexcraft.exe -lopengl32 -lglu32 -lgdi32 -luser32 -lshell32 -lole32 -lwindowscodecs
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <initguid.h>
#include <wincodec.h>
#include <shellapi.h>
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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define APP_TITLE "PEXCRAFT"
#define VERSION_TEXT "PEXCRAFT Beta 1.0"
#define MAX_BUTTONS 64
#define MAX_LABEL 256
#define MAX_PATHBUF 1024
#define ARRAY_COUNT(a) ((int)(sizeof(a)/sizeof((a)[0])))

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
    long long seed;
    char world_name[32];
    char world_dir[MAX_PATHBUF];
    char title[MAX_LABEL];
    char status[MAX_LABEL];
} WorldGenJob;

typedef enum ScreenId {
    SCREEN_TITLE,
    SCREEN_OPTIONS,
    SCREEN_CONTROLS,
    SCREEN_WORLD_SELECT,
    SCREEN_WORLD_DELETE,
    SCREEN_CONFIRM_DELETE,
    SCREEN_MULTIPLAYER,
    SCREEN_TEXPACK,
    SCREEN_GENERATING,
    SCREEN_INGAME,
    SCREEN_PAUSE,
    SCREEN_INVENTORY,
    SCREEN_CHAT,
    SCREEN_NOTICE
} ScreenId;

typedef enum ButtonKind {
    BUTTON_NORMAL,
    BUTTON_SLIDER
} ButtonKind;

typedef enum OptionId {
    OPT_MUSIC = 0,
    OPT_SOUND,
    OPT_INVERT_MOUSE,
    OPT_SENSITIVITY,
    OPT_RENDER_DISTANCE,
    OPT_VIEW_BOBBING,
    OPT_ANAGLYPH,
    OPT_LIMIT_FRAMERATE,
    OPT_DIFFICULTY,
    OPT_GRAPHICS,
    OPT_FULLSCREEN,
    OPT_SHOW_FPS,
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
    int max_fps;  /* 1..10000, 0 = unlimited */
    int difficulty;
    int fancy_graphics;
    int fullscreen;
    int show_fps;
    char skin[64];
    char last_server[64];
    int keys[10];
} Options;

static HINSTANCE g_inst;
static HWND g_hwnd;
static HDC g_hdc;
static HGLRC g_glrc;
static int g_win_w = 854, g_win_h = 480;
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
static TexturePackEntry g_texpacks[MAX_TEXPACKS];
static int g_texpack_count = 0;
static int g_selected_texpack = 0;
static int g_texpack_scroll = 0;
static int g_texpack_drag_anchor = -1;
static char g_current_texpack[MAX_LABEL] = "Default";
static IWICImagingFactory *g_wic_factory = NULL;
static char g_notice_title[MAX_LABEL] = "";
static char g_notice_line1[MAX_LABEL] = "";
static char g_notice_line2[MAX_LABEL] = "";
static char g_multiplayer_ip[64] = "";
static int g_ticks = 0;
static int g_waiting_key = -1;
static int g_confirm_world = 0;
static WorldGenJob g_worldgen;
static char g_loaded_world_dir[MAX_PATHBUF] = "";
static char g_loaded_world_name[64] = "";
static int g_selected_hotbar_slot = 0;
static int g_player_health = 20;
static int g_player_prev_health = 20;
static int g_player_armor = 0;
static int g_ingame_ticks = 0;


#define FLAT_WORLD_MIN 0
#define FLAT_WORLD_MAX 99
#define FLAT_WORLD_SIZE 100
#define FLAT_WORLD_Y_MIN 0
#define FLAT_WORLD_Y_MAX 63
#define FLAT_WORLD_HEIGHT (FLAT_WORLD_Y_MAX - FLAT_WORLD_Y_MIN + 1)
#define MAX_DROP_ENTITIES 64
#define ITEM_MAX_STACK 64
#define BLOCK_GRASS 2
#define BLOCK_DIRT 3
#define BLOCK_BEDROCK 7
#define ITEM_STONE_SWORD 272
#define ITEM_STONE_SHOVEL 273
#define ITEM_STONE_PICKAXE 274
#define ITEM_STONE_AXE 275

typedef struct ItemStack {
    int id;
    int count;
    int damage;
    int pop_time;
} ItemStack;

typedef struct FlatDroppedItem {
    int active;
    ItemStack stack;
    float x, y, z;
    float prev_x, prev_y, prev_z;
    float mx, my, mz;
    float rot;
    int age;
    int pickup_delay;
} FlatDroppedItem;

typedef struct FlatRayHit {
    int hit;
    int bx, by, bz;
    int face;
    float hx, hy, hz;
} FlatRayHit;

static int g_flat_blocks[FLAT_WORLD_HEIGHT][FLAT_WORLD_SIZE][FLAT_WORLD_SIZE];
static GLuint g_flat_world_display_list = 0;
static int g_flat_world_geometry_dirty = 1;
static ItemStack g_inventory[36];
static ItemStack g_carried_stack;
static FlatDroppedItem g_drops[MAX_DROP_ENTITIES];
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
/* Consecutive shovel bonus: first dirt/grass block 50%, second+ 60% while holding. */
static int g_shovel_combo_count = 0;

/* Minimal Beta 1.0 player/camera state for the flat 100x100x64 in-game test world.
   Names mirror the decompiled fields conceptually:
   yaw/pitch = Entity rotationYaw/rotationPitch, distance_walked/camera_* feed view bobbing. */
static float g_player_x = 50.5f, g_player_y = 5.62f, g_player_z = 50.5f;
static float g_player_prev_x = 50.5f, g_player_prev_y = 5.62f, g_player_prev_z = 50.5f;
static float g_player_motion_x = 0.0f, g_player_motion_y = 0.0f, g_player_motion_z = 0.0f;
static float g_player_yaw = 0.0f, g_player_pitch = 0.0f;
static float g_player_prev_yaw = 0.0f, g_player_prev_pitch = 0.0f;
static int g_player_on_ground = 1;
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
static int g_third_person_view = 0;
static int g_debug_menu_shown = 0;
static int g_mouse_grabbed = 0;
static int g_cursor_hidden = 0;
static int g_recentering_mouse = 0;
static char g_chat_input[128] = "";
#define MAX_CHAT_LINES 50
typedef struct ChatLine { char text[256]; int age; } ChatLine;
static ChatLine g_chat_lines[MAX_CHAT_LINES];
static int g_chat_count = 0;
static unsigned int g_crc_table[256];
static int g_crc_table_ready = 0;
static char g_splash[MAX_LABEL] = "Finally beta!";
static double g_last_time = 0.0;

static Texture tex_bg, tex_gui, tex_font, tex_terrain, tex_black, tex_pack, tex_default_pack_icon, tex_unknown_pack;
static Texture tex_icons, tex_inventory, tex_items, tex_steve;
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
    "View Bobbing", "V-Sync", "Max FPS", "Difficulty", "Graphics",
    "Enable Fullscreen", "Show FPS Counter"
};
static const int opt_is_slider[OPT_COUNT] = {1,1,0,1,0,0,0,1,0,0,0,0};
static const int opt_is_boolean[OPT_COUNT] = {0,0,1,0,0,1,1,0,0,0,1,1};
static const char *render_distance_names[4] = {"Far", "Normal", "Short", "Tiny"};
static const char *difficulty_names[4] = {"Peaceful", "Easy", "Normal", "Hard"};
static const char *key_action_names[10] = {"Forward", "Left", "Back", "Right", "Jump", "Sneak", "Drop", "Inventory", "Chat", "Toggle Fog"};
static const int default_keys[10] = {'W','A','S','D',VK_SPACE,VK_SHIFT,'Q','I','T','F'};
static int lwjgl_to_vk(int code);
static int vk_to_lwjgl(int vk);
static void init_font_widths(void);
static void set_screen(ScreenId s);
static void start_world_generation(int slot);
static void worldgen_tick(void);
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
static void inventory_reset(void);
static void inventory_tick(void);
static void inventory_drop_selected_one(void);
static void inventory_mouse_click(int mx, int my, int button);
static void world_left_mouse_released(void);
static void ingame_right_click(void);
static void draw_item_stack_gui(const ItemStack *st, int x, int y);
static void draw_carried_stack(void);
static void update_breaking(void);
static void update_dropped_items(void);
static int flat_player_aabb_collides(float px, float py, float pz);
static int flat_player_has_sneak_support(float px, float py, float pz);
static int flat_block_intersects_player(int bx, int by, int bz);
static void trigger_hand_swing(void);
static int item_icon_tile(int id);
static int write_level_dat(const char *world_dir, const char *world_name, long long seed, int spawn_x, int spawn_y, int spawn_z, long long size_on_disk);
static int write_session_lock(const char *world_dir);

#endif /* MC_B1_SPLIT_COMMON_H */
