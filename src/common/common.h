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
#elif defined(PEX_PLATFORM_XBOX_UWP)
#include "../platform/xbox_uwp/xbox_uwp_compat.h"
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
#include <signal.h>
#include <stddef.h>

#ifndef VK_PRIOR
#define VK_PRIOR 0x21
#endif
#ifndef VK_NEXT
#define VK_NEXT 0x22
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define APP_TITLE "PexCraft"
#define VERSION_TEXT "PexCraft 1.2.5"
#define MAX_BUTTONS 64
#define MAX_LABEL 256
#define MAX_PATHBUF 1024
#define MAX_WORLD_SAVES 256
#define ARRAY_COUNT(a) ((int)(sizeof(a)/sizeof((a)[0])))
#ifndef PEX_GL_CLAMP_EDGE
#ifdef GL_CLAMP_TO_EDGE
#define PEX_GL_CLAMP_EDGE GL_CLAMP_TO_EDGE
#else
#define PEX_GL_CLAMP_EDGE GL_CLAMP
#endif
#endif
#ifndef GL_NEAREST_MIPMAP_NEAREST
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#endif
#ifndef GL_LINEAR_MIPMAP_NEAREST
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#endif
#ifndef GL_NEAREST_MIPMAP_LINEAR
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#endif
#ifndef GL_LINEAR_MIPMAP_LINEAR
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#endif
#ifndef GL_TEXTURE_BASE_LEVEL
#define GL_TEXTURE_BASE_LEVEL 0x813C
#endif
#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL 0x813D
#endif
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
#define CLASSIC_PACK_NAME "Release"
#define CLASSIC_PACK_URL "https://piston-data.mojang.com/v1/objects/4a2fac7504182a97dcbcd7560c6392d7c8139928/client.jar"
#define CLASSIC_SOUNDS_INDEX_URL "https://launchermeta.mojang.com/v1/packages/770572e819335b6c0a053f8378ad88eda189fc14/legacy.json"
#define CLASSIC_SOUND_OBJECT_URL_PREFIX "https://resources.download.minecraft.net"
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII) || defined(PEX_PLATFORM_ANDROID) || defined(PEX_PLATFORM_ANDROID_TV) || defined(PEX_PLATFORM_LGWEBOS) || defined(PEX_PLATFORM_XBOX_UWP) || defined(PEX_PLATFORM_SDL2)
#define PEX_CLASSIC_SOUND_DOWNLOAD_SUPPORTED 0
#else
#define PEX_CLASSIC_SOUND_DOWNLOAD_SUPPORTED 1
#endif

#ifndef PEX_LOG_EVERYTHING
#define PEX_LOG_EVERYTHING 1
#endif
#ifndef PEX_VERBOSE_CHUNK_LOG
#define PEX_VERBOSE_CHUNK_LOG 0
#endif

static FILE *g_pex_log_file = NULL;
static int g_pex_log_ready = 0;
static int g_pex_crash_handlers_ready = 0;
static CRITICAL_SECTION g_pex_log_cs;
static int g_pex_log_cs_ready = 0;
static unsigned int g_pex_log_lines_since_flush = 0;

static void pex_log_timestamp(char *out, size_t cap) {
    time_t t = time(NULL);
    struct tm tmv;
    memset(&tmv, 0, sizeof(tmv));
#if defined(_MSC_VER)
    localtime_s(&tmv, &t);
#else
    struct tm *tmp = localtime(&t);
    if (tmp) tmv = *tmp;
#endif
    snprintf(out, cap, "%04d-%02d-%02d %02d:%02d:%02d",
             tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
             tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
}

#if defined(PEX_PLATFORM_XBOX_UWP)
extern const char *pex_xbox_uwp_get_local_folder(void);
#endif

static void pex_log_init(void) {
    if (g_pex_log_ready) return;
    if (!g_pex_log_cs_ready) {
        InitializeCriticalSection(&g_pex_log_cs);
        g_pex_log_cs_ready = 1;
    }
#if defined(PEX_PLATFORM_XBOX_UWP)
    {
        char log_path[MAX_PATHBUF];
        const char *local = pex_xbox_uwp_get_local_folder();
        if (local && local[0]) {
            CreateDirectoryA(local, NULL);
            snprintf(log_path, sizeof(log_path), "%s\\log.txt", local);
            g_pex_log_file = fopen(log_path, "a");
        }
    }
#else
    g_pex_log_file = fopen("log.txt", "a");
#endif
    g_pex_log_ready = 1;
    if (g_pex_log_file) {
        char ts[64];
        pex_log_timestamp(ts, sizeof(ts));
        fprintf(g_pex_log_file, "[%s] {log open: %s}\n", ts, VERSION_TEXT);
        fflush(g_pex_log_file);
    }
}

static void pex_log_shutdown(void) {
    if (g_pex_log_cs_ready) EnterCriticalSection(&g_pex_log_cs);
    if (g_pex_log_file) {
        char ts[64];
        pex_log_timestamp(ts, sizeof(ts));
        fprintf(g_pex_log_file, "[%s] {log close}\n", ts);
        fflush(g_pex_log_file);
        fclose(g_pex_log_file);
        g_pex_log_file = NULL;
    }
    g_pex_log_ready = 0;
    if (g_pex_log_cs_ready) LeaveCriticalSection(&g_pex_log_cs);
}

static void pex_logf(const char *fmt, ...) {
#if PEX_LOG_EVERYTHING
    if (!fmt) return;
    if (!g_pex_log_ready) pex_log_init();
    char msg[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    char ts[64];
    pex_log_timestamp(ts, sizeof(ts));
    if (g_pex_log_cs_ready) EnterCriticalSection(&g_pex_log_cs);
    if (g_pex_log_file) {
        fprintf(g_pex_log_file, "[%s] {%s}\n", ts, msg);
        g_pex_log_lines_since_flush++;
        if ((g_pex_log_lines_since_flush & 63u) == 0u ||
            strstr(msg, "CRASH") || strstr(msg, "ERROR") || strstr(msg, "error") ||
            strstr(msg, "failed") || strstr(msg, "WinINet") || strstr(msg, "log close")) {
            fflush(g_pex_log_file);
        }
    }
    if (g_pex_log_cs_ready) LeaveCriticalSection(&g_pex_log_cs);
#else
    (void)fmt;
#endif
}

#if PEX_VERBOSE_CHUNK_LOG
#define pex_logf_trace(...) pex_logf(__VA_ARGS__)
#else
#define pex_logf_trace(...) do { } while (0)
#endif

#if !defined(PEX_PLATFORM_PSP) && !defined(PEX_PLATFORM_WII)
static void pex_signal_crash_handler(int sig) {
    pex_logf("CRASH: signal=%d", sig);
    pex_log_shutdown();
    signal(sig, SIG_DFL);
    raise(sig);
}
#endif

#if !defined(PEX_PLATFORM_SDL2) && !defined(PEX_PLATFORM_PSP) && !defined(PEX_PLATFORM_WII)
static LONG WINAPI pex_windows_exception_filter(EXCEPTION_POINTERS *info) {
    void *addr = NULL;
    unsigned long code = 0;
    uintptr_t module_offset = 0;
    uintptr_t exe_offset = 0;
    int addr_in_exe = 0;
    char module_name[MAX_PATH];
    module_name[0] = 0;
    if (info && info->ExceptionRecord) {
        addr = info->ExceptionRecord->ExceptionAddress;
        code = (unsigned long)info->ExceptionRecord->ExceptionCode;
        if (addr) {
            MEMORY_BASIC_INFORMATION mbi;
            memset(&mbi, 0, sizeof(mbi));
            if (VirtualQuery(addr, &mbi, sizeof(mbi)) == sizeof(mbi) && mbi.AllocationBase) {
                HMODULE fault_mod = (HMODULE)mbi.AllocationBase;
                module_offset = (uintptr_t)addr - (uintptr_t)fault_mod;
                GetModuleFileNameA(fault_mod, module_name, sizeof(module_name));
                HMODULE exe = GetModuleHandleA(NULL);
                if (exe && fault_mod == exe) {
                    exe_offset = module_offset;
                    addr_in_exe = 1;
                }
            }
        }
    }
    pex_logf("CRASH: unhandled exception code=0x%08lX address=%p module=%s module_offset=0x%llX exe_offset=%s0x%llX%s",
             code, addr, module_name[0] ? module_name : "unknown",
             (unsigned long long)module_offset,
             addr_in_exe ? "" : "n/a(",
             (unsigned long long)exe_offset,
             addr_in_exe ? "" : ")");
    if (code == 0xC00000FDul) {
        pex_logf("CRASH DETAIL: stack overflow; likely worker stack exhaustion or recursive/racing world-stream work");
    } else {
        void *frames[16];
        USHORT n = CaptureStackBackTrace(0, 16, frames, NULL);
        for (USHORT i = 0; i < n; ++i) {
            MEMORY_BASIC_INFORMATION mbi;
            char modname[MAX_PATH];
            modname[0] = 0;
            uintptr_t off = 0;
            if (VirtualQuery(frames[i], &mbi, sizeof(mbi)) == sizeof(mbi) && mbi.AllocationBase) {
                HMODULE hm = (HMODULE)mbi.AllocationBase;
                off = (uintptr_t)frames[i] - (uintptr_t)hm;
                GetModuleFileNameA(hm, modname, sizeof(modname));
            }
            pex_logf("CRASH STACK[%u]: address=%p module=%s offset=0x%llX", (unsigned)i, frames[i], modname[0] ? modname : "unknown", (unsigned long long)off);
        }
    }
    pex_log_shutdown();
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

static void pex_install_crash_handlers(void) {
    if (g_pex_crash_handlers_ready) return;
    g_pex_crash_handlers_ready = 1;
#if !defined(PEX_PLATFORM_PSP) && !defined(PEX_PLATFORM_WII)
    signal(SIGSEGV, pex_signal_crash_handler);
    signal(SIGABRT, pex_signal_crash_handler);
    signal(SIGFPE, pex_signal_crash_handler);
    signal(SIGILL, pex_signal_crash_handler);
#endif
#if !defined(PEX_PLATFORM_SDL2) && !defined(PEX_PLATFORM_PSP) && !defined(PEX_PLATFORM_WII)
    SetUnhandledExceptionFilter(pex_windows_exception_filter);
#endif
    pex_logf("crash handlers installed");
}

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
    int newly_created;
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
    char world_name[MAX_LABEL];
    char world_dir[MAX_PATHBUF];
    char title[MAX_LABEL];
    char status[MAX_LABEL];
} WorldGenJob;

typedef struct WorldSaveEntry {
    char dir_name[MAX_LABEL];
    char display_name[MAX_LABEL];
    char path[MAX_PATHBUF];
    long long last_played;
    unsigned long long size_on_disk;
    int world_type;
    int game_mode;
    int hardcore;
    int requires_conversion;
} WorldSaveEntry;

typedef enum ScreenId {
    SCREEN_TITLE,
    SCREEN_OPTIONS,
    SCREEN_OPTIONS_MORE,
    SCREEN_STIVUFINE,
    SCREEN_STIVUFINE_DETAILS,
    SCREEN_STIVUFINE_QUALITY,
    SCREEN_STIVUFINE_ANIMATIONS,
    SCREEN_STIVUFINE_PERFORMANCE,
    SCREEN_STIVUFINE_OTHER,
    SCREEN_ASSETS,
    SCREEN_LANGUAGE,
    SCREEN_SET_NAME,
    SCREEN_TV_REMOTE_MAP,
    SCREEN_SYSTEM_INFO,
    SCREEN_SKINS,
    SCREEN_CONTROLS,
    SCREEN_WORLD_SELECT,
    SCREEN_CREATE_WORLD,
    SCREEN_WORLD_TYPE,
    SCREEN_WORLD_DELETE,
    SCREEN_RENAME_WORLD,
    SCREEN_CONFIRM_DELETE,
    SCREEN_MULTIPLAYER,
    SCREEN_CONNECTING,
    SCREEN_TEXPACK,
    SCREEN_TEXPACK_INSTALL,
    SCREEN_GENERATING,
    SCREEN_SAVING_QUIT,
    SCREEN_INGAME,
    SCREEN_PAUSE,
    SCREEN_ACHIEVEMENTS,
    SCREEN_STATISTICS,
    SCREEN_INVENTORY,
    SCREEN_CREATIVE,
    SCREEN_WORKBENCH,
    SCREEN_FURNACE,
    SCREEN_CHEST,
    SCREEN_DEATH,
    SCREEN_CHAT,
    SCREEN_VIRTUAL_KEYBOARD,
    SCREEN_NOTICE,
    SCREEN_RENDERER_RESTART_PROMPT,
    SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT,
    SCREEN_CLASSIC_PACK_WARNING
} ScreenId;

typedef enum ButtonKind {
    BUTTON_NORMAL,
    BUTTON_SLIDER,
    BUTTON_LANGUAGE,
    BUTTON_HITBOX
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

#define PEX_TV_REMOTE_ACTION_COUNT 13

typedef enum PexTvRemoteAction {
    PEX_TV_REMOTE_UP = 0,
    PEX_TV_REMOTE_DOWN,
    PEX_TV_REMOTE_LEFT,
    PEX_TV_REMOTE_RIGHT,
    PEX_TV_REMOTE_OK,
    PEX_TV_REMOTE_BACK,
    PEX_TV_REMOTE_BREAK,
    PEX_TV_REMOTE_PLACE,
    PEX_TV_REMOTE_INVENTORY,
    PEX_TV_REMOTE_SNEAK,
    PEX_TV_REMOTE_DROP,
    PEX_TV_REMOTE_HOTBAR_PREV,
    PEX_TV_REMOTE_HOTBAR_NEXT
} PexTvRemoteAction;

static const char *pex_tv_remote_action_key(int action) {
    switch (action) {
        case PEX_TV_REMOTE_UP: return "up";
        case PEX_TV_REMOTE_DOWN: return "down";
        case PEX_TV_REMOTE_LEFT: return "left";
        case PEX_TV_REMOTE_RIGHT: return "right";
        case PEX_TV_REMOTE_OK: return "ok";
        case PEX_TV_REMOTE_BACK: return "back";
        case PEX_TV_REMOTE_BREAK: return "break";
        case PEX_TV_REMOTE_PLACE: return "place";
        case PEX_TV_REMOTE_INVENTORY: return "inventory";
        case PEX_TV_REMOTE_SNEAK: return "sneak";
        case PEX_TV_REMOTE_DROP: return "drop";
        case PEX_TV_REMOTE_HOTBAR_PREV: return "hotbarPrev";
        case PEX_TV_REMOTE_HOTBAR_NEXT: return "hotbarNext";
        default: return "unknown";
    }
}

static const char *pex_tv_remote_action_label(int action) {
    switch (action) {
        case PEX_TV_REMOTE_UP: return "Move / menu up";
        case PEX_TV_REMOTE_DOWN: return "Move / menu down";
        case PEX_TV_REMOTE_LEFT: return "Move / menu left";
        case PEX_TV_REMOTE_RIGHT: return "Move / menu right";
        case PEX_TV_REMOTE_OK: return "OK / jump / select";
        case PEX_TV_REMOTE_BACK: return "Back / pause";
        case PEX_TV_REMOTE_BREAK: return "Break / attack";
        case PEX_TV_REMOTE_PLACE: return "Place / use";
        case PEX_TV_REMOTE_INVENTORY: return "Inventory";
        case PEX_TV_REMOTE_SNEAK: return "Sneak";
        case PEX_TV_REMOTE_DROP: return "Drop item";
        case PEX_TV_REMOTE_HOTBAR_PREV: return "Hotbar previous";
        case PEX_TV_REMOTE_HOTBAR_NEXT: return "Hotbar next";
        default: return "Unknown";
    }
}


typedef enum StivuFineOptionId {
    SF_GRAPHICS = 0,
    SF_RENDER_DISTANCE_FINE,
    SF_AO_LEVEL,
    SF_FRAMERATE_LIMIT,
    SF_ANAGLYPH,
    SF_VIEW_BOBBING,
    SF_GUI_SCALE,
    SF_ADVANCED_OPENGL,
    SF_GAMMA,
    SF_RENDER_CLOUDS,
    SF_FOG_FANCY,
    SF_FOG_START,

    SF_CLOUDS,
    SF_CLOUD_HEIGHT,
    SF_TREES,
    SF_GRASS,
    SF_WATER,
    SF_RAIN,
    SF_SKY,
    SF_STARS,
    SF_SUN_MOON,
    SF_SHOW_CAPES,
    SF_DEPTH_FOG,

    SF_MIPMAP_LEVEL,
    SF_MIPMAP_TYPE,
    SF_CLEAR_WATER,
    SF_RANDOM_MOBS,
    SF_BETTER_GRASS,
    SF_BETTER_SNOW,
    SF_CUSTOM_FONTS,
    SF_CUSTOM_COLORS,
    SF_SWAMP_COLORS,
    SF_SMOOTH_BIOMES,
    SF_CONNECTED_TEXTURES,
    SF_NATURAL_TEXTURES,
    SF_AA_LEVEL,
    SF_AF_LEVEL,

    SF_ANIMATED_WATER,
    SF_ANIMATED_LAVA,
    SF_ANIMATED_FIRE,
    SF_ANIMATED_PORTAL,
    SF_ANIMATED_REDSTONE,
    SF_ANIMATED_EXPLOSION,
    SF_ANIMATED_FLAME,
    SF_ANIMATED_SMOKE,
    SF_VOID_PARTICLES,
    SF_WATER_PARTICLES,
    SF_RAIN_SPLASH,
    SF_PORTAL_PARTICLES,
    SF_PARTICLES,
    SF_DRIPPING_WATER_LAVA,
    SF_ANIMATED_TERRAIN,
    SF_ANIMATED_ITEMS,
    SF_ANIMATED_TEXTURES,

    SF_SMOOTH_FPS,
    SF_SMOOTH_INPUT,
    SF_LOAD_FAR,
    SF_PRELOADED_CHUNKS,
    SF_CHUNK_UPDATES,
    SF_CHUNK_UPDATES_DYNAMIC,

    SF_FAST_DEBUG_INFO,
    SF_PROFILER,
    SF_WEATHER,
    SF_TIME,
    SF_FULLSCREEN_MODE,
    SF_AUTOSAVE_TICKS,

    SF_COUNT
} StivuFineOptionId;

#define SF_BUTTON_BASE 5000
#define SF_NAV_DETAILS 5101
#define SF_NAV_QUALITY 5102
#define SF_NAV_ANIMATIONS 5111
#define SF_NAV_PERFORMANCE 5112
#define SF_NAV_TEXPACKS 5121
#define SF_NAV_OTHER 5122

typedef enum StivuFineMode {
    SF_DEFAULT = 0,
    SF_FAST = 1,
    SF_FANCY = 2,
    SF_OFF = 3
} StivuFineMode;

typedef enum StivuFineAnimationMode {
    SF_ANIM_ON = 0,
    SF_ANIM_DYNAMIC = 1,
    SF_ANIM_OFF = 2
} StivuFineAnimationMode;

/* Unity-build helpers defined in settings/options.c and used by earlier
   included modules such as textures.c. */
static int stivufine_custom_fonts_enabled(void);
static int stivufine_custom_colors_enabled(void);
static int stivufine_random_mobs_enabled(void);
static int stivufine_connected_textures_mode(void);
static int stivufine_natural_textures_enabled(void);
static int stivufine_aa_level(void);
static int stivufine_af_level(void);

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


#define CLASSIC_AUDIO_MOBS        0x01
#define CLASSIC_AUDIO_WORLD_UI    0x02
#define CLASSIC_AUDIO_RECORDS     0x04
#define CLASSIC_AUDIO_MENU_MUSIC  0x08
#define CLASSIC_AUDIO_GAME_MUSIC  0x10
#define CLASSIC_AUDIO_ALL (CLASSIC_AUDIO_MOBS | CLASSIC_AUDIO_WORLD_UI | CLASSIC_AUDIO_RECORDS | CLASSIC_AUDIO_MENU_MUSIC | CLASSIC_AUDIO_GAME_MUSIC)

#define PEX_KEY_BIND_COUNT 11
#define PEX_KEY_SPRINT 10

typedef struct Options {
    float music;
    float sound;
    float sensitivity;
    int invert_mouse;
    int render_distance;
    int gui_scale; /* Minecraft 1.2.5: 0 Auto, 1 Small, 2 Normal, 3 Large */
    int view_bobbing;
    int anaglyph; /* repurposed: V-Sync */
    int max_fps;  /* 1..200, 0 = unlimited */
    float fov;    /* Java 1.2.5 FOV degrees: 70..110 before runtime modifiers */
    int difficulty;
    int fancy_graphics;
    int fullscreen;
    int show_fps;
    int renderer_backend; /* RendererBackend saved to options.txt; restart required after changing */
    /* StivuFine (StivuFine graphics) graphics settings.  Unsupported
       features stay present in the UI as disabled/gray entries, but the stored
       values mirror StivuFine names so old optionsstivufine_legacy.txt files can be imported. */
    int sf_fog_type;
    float sf_fog_start;
    float sf_ao_level;
    int sf_clouds;
    float sf_cloud_height;
    int sf_trees;
    int sf_grass;
    int sf_water;
    int sf_rain;
    int sf_sky;
    int sf_stars;
    int sf_sun_moon;
    int sf_depth_fog;
    int sf_animated_water;
    int sf_animated_lava;
    int sf_animated_fire;
    int sf_animated_portal;
    int sf_animated_redstone;
    int sf_animated_explosion;
    int sf_animated_flame;
    int sf_animated_smoke;
    int sf_void_particles;
    int sf_water_particles;
    int sf_rain_splash;
    int sf_portal_particles;
    int sf_dripping_water_lava;
    int sf_animated_terrain;
    int sf_animated_items;
    int sf_animated_textures;
    int sf_particles;
    int sf_chunk_updates;
    int sf_chunk_updates_dynamic;
    int sf_fast_debug_info;
    int sf_profiler;
    int sf_weather;
    float sf_gamma;
    int sf_clear_water;
    int sf_better_snow;
    int sf_swamp_colors;
    int sf_smooth_biomes;
    int sf_custom_fonts;
    int sf_custom_colors;
    int sf_random_mobs;
    int sf_mipmap_level;
    int sf_mipmap_linear;
    int sf_connected_textures; /* 1 Fast, 2 Fancy, 3 OFF */
    int sf_natural_textures;
    int sf_aa_level;           /* 0,2,4,6,8,12,16; restart on SDL/OpenGL */
    int sf_af_level;           /* 1=OFF, then 2/4/8/16 */

    int ignore_classic_resources_warning;
    int download_classic_textures;
    int download_classic_sounds;
    int classic_audio_mask;
    int ignore_classic_sounds_warning;
    char skin[64]; /* selected texture-pack name, kept for old options.txt compatibility */
    char skin_path[MAX_PATHBUF];
    char last_server[64];
    char username[32];
    int name_set;
    int tv_remote_mapped;
    int tv_remote_map[PEX_TV_REMOTE_ACTION_COUNT];
    char language[16];
    int keys[PEX_KEY_BIND_COUNT];
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
    int lt_down, rt_down;
    int lt_release_frames, rt_release_frames;
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


typedef struct PexNetArmorRenderStack {
    int id;
    int count;
    int damage;
    int has_custom_color;
    int custom_color;
} PexNetArmorRenderStack;

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
    int held_item_id;
    int held_item_count;
    int held_item_damage;
    int held_slot;
    PexNetArmorRenderStack armor[4]; /* boots, leggings, chestplate, helmet */
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

#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
#define MAX_PASSIVE_MOBS 32
#else
/* Java 1.2.5 caps are per EnumCreatureType over the 17x17 eligible chunk set:
   monster=70, creature=15, waterCreature=5 scaled by eligibleChunks/256.
   One player therefore allows roughly 79 monsters, 16 animals, and 5 water
   mobs.  Keep enough slots for all categories plus village utility mobs. */
#define MAX_PASSIVE_MOBS 128
#endif

#define PEX_MOB_PATH_MAX 256
#define PEX_MOB_AI_TASK_MAX 28
#define PEX_MOB_OWNER_NONE 0
#define PEX_MOB_OWNER_SINGLEPLAYER 1

typedef enum PassiveMobType {
    PASSIVE_MOB_NONE = 0,
    PASSIVE_MOB_PIG = 1,
    PASSIVE_MOB_SHEEP = 2,
    PASSIVE_MOB_COW = 3,
    PASSIVE_MOB_CHICKEN = 4,
    PASSIVE_MOB_CREEPER = 5,
    PASSIVE_MOB_SKELETON = 6,
    PASSIVE_MOB_SPIDER = 7,
    PASSIVE_MOB_GIANT = 8,
    PASSIVE_MOB_ZOMBIE = 9,
    PASSIVE_MOB_SLIME = 10,
    PASSIVE_MOB_GHAST = 11,
    PASSIVE_MOB_PIG_ZOMBIE = 12,
    PASSIVE_MOB_ENDERMAN = 13,
    PASSIVE_MOB_CAVE_SPIDER = 14,
    PASSIVE_MOB_SILVERFISH = 15,
    PASSIVE_MOB_BLAZE = 16,
    PASSIVE_MOB_MAGMA_CUBE = 17,
    PASSIVE_MOB_ENDER_DRAGON = 18,
    PASSIVE_MOB_SQUID = 19,
    PASSIVE_MOB_WOLF = 20,
    PASSIVE_MOB_MOOSHROOM = 21,
    PASSIVE_MOB_SNOWMAN = 22,
    PASSIVE_MOB_OCELOT = 23,
    PASSIVE_MOB_IRON_GOLEM = 24,
    PASSIVE_MOB_VILLAGER = 25
} PassiveMobType;

typedef enum PexDamageType {
    PEX_DAMAGE_IN_FIRE = 0,
    PEX_DAMAGE_ON_FIRE,
    PEX_DAMAGE_LAVA,
    PEX_DAMAGE_IN_WALL,
    PEX_DAMAGE_DROWN,
    PEX_DAMAGE_STARVE,
    PEX_DAMAGE_CACTUS,
    PEX_DAMAGE_FALL,
    PEX_DAMAGE_OUT_OF_WORLD,
    PEX_DAMAGE_GENERIC,
    PEX_DAMAGE_EXPLOSION,
    PEX_DAMAGE_MAGIC,
    PEX_DAMAGE_MOB,
    PEX_DAMAGE_PLAYER,
    PEX_DAMAGE_ARROW,
    PEX_DAMAGE_FIREBALL,
    PEX_DAMAGE_THROWN,
    PEX_DAMAGE_INDIRECT_MAGIC
} PexDamageType;

typedef enum PexDamageEntityKind {
    PEX_DAMAGE_ENTITY_NONE = 0,
    PEX_DAMAGE_ENTITY_PLAYER = 1,
    PEX_DAMAGE_ENTITY_MOB = 2,
    PEX_DAMAGE_ENTITY_PROJECTILE = 3
} PexDamageEntityKind;

typedef struct PexDamageSource {
    PexDamageType type;
    const char *damage_type;
    int unblockable;
    int fire_damage;
    int projectile;
    int creative_allowed;
    float hunger_damage;
    PexDamageEntityKind direct_kind;
    PexDamageEntityKind true_kind;
    int direct_mob_index;
    int direct_mob_type;
    int true_mob_index;
    int true_mob_type;
    float direct_x, direct_y, direct_z;
    float true_x, true_y, true_z;
} PexDamageSource;

typedef enum PexMobAITaskId {
    PEX_MOB_TASK_NONE = 0,
    PEX_MOB_TASK_SWIM,
    PEX_MOB_TASK_SIT,
    PEX_MOB_TASK_CREEPER_SWELL,
    PEX_MOB_TASK_FLEE_SUN,
    PEX_MOB_TASK_AVOID_PLAYER,
    PEX_MOB_TASK_ATTACK,
    PEX_MOB_TASK_FOLLOW_OWNER,
    PEX_MOB_TASK_MATE,
    PEX_MOB_TASK_TEMPT,
    PEX_MOB_TASK_FOLLOW_PARENT,
    PEX_MOB_TASK_MOVE_VILLAGE,
    PEX_MOB_TASK_WANDER,
    PEX_MOB_TASK_WATCH_PLAYER,
    PEX_MOB_TASK_LOOK_IDLE,
    PEX_MOB_TASK_TARGET_PLAYER,
    PEX_MOB_TASK_TARGET_MOB,
    PEX_MOB_TASK_TARGET_CURRENT,
    PEX_MOB_TASK_PANIC,
    PEX_MOB_TASK_EAT_GRASS,
    PEX_MOB_TASK_RESTRICT_SUN,
    PEX_MOB_TASK_LEAP_AT_TARGET,
    PEX_MOB_TASK_AVOID_MOB,
    PEX_MOB_TASK_BEG,
    PEX_MOB_TASK_MOVE_INDOORS,
    PEX_MOB_TASK_RESTRICT_OPEN_DOOR,
    PEX_MOB_TASK_OPEN_DOOR,
    PEX_MOB_TASK_VILLAGER_MATE,
    PEX_MOB_TASK_PLAY,
    PEX_MOB_TASK_FOLLOW_GOLEM,
    PEX_MOB_TASK_LOOK_AT_VILLAGER,
    PEX_MOB_TASK_OCELOT_SIT,
    PEX_MOB_TASK_TARGET_OWNER_HURT_BY,
    PEX_MOB_TASK_TARGET_OWNER_HURT_TARGET,
    PEX_MOB_TASK_TARGET_REVENGE,
    PEX_MOB_TASK_DEFEND_VILLAGE
} PexMobAITaskId;

typedef struct PexMobAITaskEntry {
    unsigned char id;
    unsigned char priority;
    unsigned char mutex_bits;
    unsigned char running;
    unsigned char continuous;
    unsigned char start_pending;
    short reserved;
    int timer;
    int target_entity_id;
    int arg0;
    int arg1;
    float speed;
    float range;
    float target_x, target_y, target_z;
    float aux_yaw, aux_pitch;
} PexMobAITaskEntry;

typedef struct PexMobSenses {
    int cache_tick;
    int player_cached;
    int player_visible;
    int mob_cached_entity_id;
    int mob_visible;
} PexMobSenses;

typedef struct PexMobMoveController {
    int update;
    float x, y, z;
    float speed;
} PexMobMoveController;

typedef struct PexMobLookController {
    int update;
    float x, y, z;
    float yaw_limit;
    float pitch_limit;
} PexMobLookController;

typedef struct PexMobJumpController {
    int jump;
} PexMobJumpController;

typedef struct PexMobNavigationState {
    int total_ticks;
    int ticks_at_last_pos;
    int avoids_water;
    int can_swim;
    int pass_open_doors;
    int pass_closed_doors;
    int avoid_sun;
    float speed;
    float last_x, last_y, last_z;
} PexMobNavigationState;

/* Explicit species state.  The older generic fields remain below solely as a
   save/render compatibility adapter for existing worlds.  Runtime AI uses
   these named fields so unrelated species no longer share semantics. */
typedef struct PexMobSpeciesState {
    int sheep_sheared;
    int sheep_color;
    int pig_saddled;
    int chicken_egg_timer;
    int creeper_fuse;
    int creeper_last_fuse;
    int creeper_state;
    int creeper_powered;
    int slime_size;
    int slime_jump_delay;
    int slime_was_on_ground;
    int ranged_visible_ticks;
    int ranged_cooldown;
    int blaze_attack_step;
    int blaze_charged;
    int blaze_height_offset_ticks;
    float blaze_height_offset;
    int ghast_course_cooldown;
    int ghast_aggro_cooldown;
    int ghast_attack_counter;
    int ghast_prev_attack_counter;
    float ghast_waypoint_x, ghast_waypoint_y, ghast_waypoint_z;
    int pig_zombie_anger;
    int enderman_aggressive;
    int enderman_stare_ticks;
    int enderman_teleport_delay;
    int wolf_angry;
    int wolf_begging;
    int wolf_beg_ticks;
    int ocelot_variant;
    int ocelot_sneaking;
    int ocelot_sprinting;
    int villager_profession;
    int villager_random_tick_divider;
    int villager_mating;
    int villager_playing;
    int golem_attack_timer;
    int golem_rose_timer;
    int golem_player_created;
    int pig_zombie_sound_delay;
    int wolf_wet;
    int wolf_shaking;
    int wolf_shake_started;
    float wolf_shake_time;
    float wolf_prev_shake_time;
    float wolf_interest;
    float wolf_prev_interest;
    int ocelot_sit_x, ocelot_sit_y, ocelot_sit_z;
    int ocelot_sit_time;
    int squid_motion_age;
    float squid_pitch, squid_prev_pitch;
    float squid_body_rotation, squid_prev_body_rotation;
    float squid_phase;
    float squid_phase_speed;
    float squid_tentacle_angle, squid_prev_tentacle_angle;
    float squid_motion_speed;
    float squid_rotation_velocity;
    float squid_random_x, squid_random_y, squid_random_z;
    int silverfish_ally_delay;
    int tameable_tamed;
} PexMobSpeciesState;

typedef struct PassiveMob {
    int active;
    int type;
    int entity_id;       /* stable runtime identity; array slots may be reused */
    int target_entity_id;/* 0=player, >0=mob entity id, -1=no target */
    int riding_entity_id;   /* Java ridingEntity: >0 mount entity id, -1 none */
    int ridden_by_entity_id;/* Java riddenByEntity: >0 passenger entity id, -1 none */
    float x, y, z;
    float prev_x, prev_y, prev_z;
    float mx, my, mz;
    float yaw, prev_yaw;                 /* Entity.rotationYaw: movement/body facing */
    float head_yaw, prev_head_yaw;       /* EntityLiving.rotationYawHead */
    float render_yaw, prev_render_yaw;   /* EntityLiving.renderYawOffset */
    float pitch, prev_pitch;
    float width, height;
    int health;
    int prev_health;
    int hurt_time;
    int damage_cooldown;
    int damage_remainder;
    int recently_hit;     /* Java EntityLiving.recentlyHit: 60 ticks after player damage */
    int last_looting_level;/* last player Looting level; currently 0 until enchant data exists */
    int death_time;
    int death_drops_done;
    int on_ground;
    int collided_horizontal;
    int collided_vertical;
    int in_water;
    int was_in_water;
    int in_lava;
    int in_web;
    int on_ladder;
    int air;
    int suffocation_ticks;
    int revenge_entity_id; /* 0=player, >0=mob, -1=none */
    int revenge_timer;
    float fall_distance;
    int age;
    int living_sound_delay;
    float limb_swing;
    float prev_limb_swing;
    float limb_amount;
    float prev_limb_amount;
    /* Minecraft 1.2.5 EntityBodyHelper animation state. Runtime-only: the
       helper deliberately converges after load rather than serializing a
       transient render interpolation cache. */
    float body_head_yaw_anchor;
    int body_yaw_idle_ticks;
    int has_path_target;
    float target_x, target_y, target_z;
    int sheared;
    int fleece_color;
    int rideable;
    float chicken_wing_rot;
    float chicken_prev_wing_rot;
    float chicken_dest_pos;
    float chicken_prev_dest_pos;
    float chicken_wing_speed;
    int egg_timer;
    int attack_time;
    int burn_time;
    int target_mob_index;
    int baby_age;
    int sitting;
    int held_block;
    int held_item;      /* Java EntityLiving equipment slot 0 / getHeldItem port state */
    int equipment[4];   /* armor slots for normal mob equipment parity; 0 until spawn/enchant data exists */
    int love_time;
    int jump_cooldown;
    int stuck_ticks;
    int wander_cooldown;
    int owner_id;        /* single-player persistent owner id for tamed wolves/cats */
    int tame_state;      /* wolf/cat/cat variant and anger state mirror */
    int collar_color;    /* wolf collar dye color; Java default red=14 */
    int village_id;      /* runtime village membership, rebuilt from nearest 1.2.5 door set */
    int door_target_x, door_target_y, door_target_z;
    int door_open_time;
    int ai_task_mask;    /* currently selected Java-style EntityAI task bits */
    int path_len;
    int path_index;
    int path_recalc_cooldown;
    int ai_age;
    int ai_repath_delay;
    int path_stuck_check_tick;
    float path_stuck_check_x, path_stuck_check_y, path_stuck_check_z;
    float path_goal_x, path_goal_y, path_goal_z;
    /* Java PathEntity stores a dynamically-sized PathPoint array.  Keep an
       initial reserve hint for low-memory platforms, but never truncate a
       completed path to an arbitrary fixed node count.  These buffers are
       runtime-only and are not serialized. */
    int *path_x;
    int *path_y;
    int *path_z;
    int path_capacity;
    /* Java 1.2.5 EntityLiving potion state for currently implemented passive mobs.
       Fixed size avoids moving the potion id constants above PassiveMob in this
       unity-style codebase.  Indices use PEX_POTION_* ids. */
    int potion_duration[32];
    int potion_amplifier[32];

    PexMobSpeciesState species;
    PexMobSenses senses;
    PexMobNavigationState navigation;
    PexMobMoveController move_controller;
    PexMobLookController look_controller;
    PexMobJumpController jump_controller;
    int goal_task_count;
    int target_task_count;
    PexMobAITaskEntry goal_tasks[PEX_MOB_AI_TASK_MAX];
    PexMobAITaskEntry target_tasks[PEX_MOB_AI_TASK_MAX];

    int random_mob_id; /* StivuFine Random Mobs persistent numeric skin id. */
} PassiveMob;

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
static double g_gui_w_d = 427.0, g_gui_h_d = 240.0; /* ScaledResolution.scaledWidthD/HeightD */
static int g_fullscreen_active = 0;
static DWORD g_windowed_style = 0;
static DWORD g_windowed_exstyle = 0;
static RECT g_windowed_rect = {0, 0, 854, 480};
static int g_mouse_x = 0, g_mouse_y = 0;
static int g_mouse_down = 0;
static int g_running = 1;
static ScreenId g_screen = SCREEN_TITLE;
static ScreenId g_parent_screen = SCREEN_TITLE;
static ScreenId g_language_return_screen = SCREEN_TITLE;
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
static int g_texpack_drag_mode = 0; /* 0=list swipe, 1=scrollbar thumb */
static int g_texpack_drag_start_y = 0;
static int g_texpack_drag_start_scroll = 0;
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
static char g_name_edit_text[32] = "";
static ScreenId g_name_return_screen = SCREEN_TITLE;
static int g_name_screen_first_run = 0;
static ScreenId g_tv_remote_return_screen = SCREEN_TITLE;
static int g_tv_remote_map_step = 0;
static int g_tv_remote_state[PEX_TV_REMOTE_ACTION_COUNT];
static int g_tv_remote_seen = 0;
static ScreenId g_virtual_keyboard_return_screen = SCREEN_TITLE;
static int g_virtual_keyboard_row = 0;
static int g_virtual_keyboard_col = 0;
static int g_virtual_keyboard_caps = 0;
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
/* Shared multiplayer respawn handshake state.  This must be declared in the
   common unity-build section because the Java protocol handler is included
   before platform/multiplayer_client.c on desktop and SDL2 targets. */
static int g_mp_pending_respawn_sync = 0;
static int g_mp_player_count = 0;
static double g_mp_interp_start_time = 0.0;
static double g_mp_interp_duration = 0.10;
static double g_mp_render_last_time = 0.0;
static int g_ticks = 0;
static int g_waiting_key = -1;
static int g_confirm_world = 0;
static int g_pending_world_slot = 0;
static int g_pending_world_type = 1;
static int g_pending_game_mode = 0; /* 0 survival, 1 creative */
static int g_pending_map_features = 1;
static int g_pending_seed_set = 0;
static long long g_pending_world_seed = 0;
static char g_pending_seed_text[MAX_LABEL] = "";
static int g_create_more_options = 0;
static int g_create_edit_field = 0;
static char g_pending_world_dir[MAX_PATHBUF] = "";
static char g_pending_world_name[MAX_LABEL] = "";
static char g_rename_world_text[MAX_LABEL] = "";
static WorldSaveEntry g_world_saves[MAX_WORLD_SAVES];
static int g_world_save_count = 0;
static int g_selected_world_index = -1;
static int g_world_save_scroll = 0;
static int g_world_drag_scroll_pixels = 0;
static int g_world_drag_mode = 0; /* 0=list swipe, 1=scrollbar thumb */
static int g_world_drag_start_y = 0;
static int g_world_drag_start_scroll = 0;
static int g_world_type = 1; /* 0 superflat, 1 default terrain */
static int g_game_mode = 0;  /* 0 survival, 1 creative */
typedef struct PexPlayerCapabilities {
    int disable_damage;
    int is_flying;
    int allow_flying;
    int is_creative_mode;
} PexPlayerCapabilities;
static PexPlayerCapabilities g_player_capabilities = {0, 0, 0, 0};
/* Set by protocol adapters that receive authoritative capability packets. */
static int g_player_capabilities_server_authoritative = 0;
static int g_world_map_features = 1;
static long long g_world_seed = 0;
static int g_current_dimension = 0; /* 0=Overworld, -1=Nether, 1=End (PexDimension) */
static int g_portal_timer = 0;      /* ticks player has been standing in a Nether portal */
static int g_portal_cooldown = 0;   /* Java-style timeUntilPortal cooldown after travel */
static float g_time_in_portal = 0.0f;      /* Java-style 0..1 portal overlay/camera amount */
static float g_prev_time_in_portal = 0.0f; /* previous tick value for partial-tick interpolation */
static int g_creative_scroll_row = 0;
static float g_creative_scroll = 0.0f;
static int g_creative_dragging_scroll = 0;
static int g_right_click_delay_timer = 0;
static int g_right_use_button_down = 0;
static int g_bow_item_in_use = 0;
static int g_bow_use_ticks = 0;
static int g_bow_use_slot = -1;
static int g_bow_use_damage = 0;
/* Java 1.2.5 ItemSword use state.  Protocol-47 multiplayer still starts and
   stops this state with the vanilla 1.8.8 C08/C07 packet pair. */
static int g_sword_item_in_use = 0;
static int g_sword_use_ticks = 0;
static int g_sword_use_slot = -1;
static int g_sword_use_id = 0;
static int g_sword_use_damage = 0;
static int g_creative_fly_toggle_timer = 0;
static int g_prev_jump_down = 0;
static int g_player_sprinting = 0;
static int g_sprint_toggle_timer = 0;
static int g_sprinting_ticks_left = 0;
static int g_prev_sprint_forward = 0;
static int g_player_collided_horiz = 0;
static float g_fov_modifier_hand = 1.0f;
static float g_prev_fov_modifier_hand = 1.0f;
static WorldGenJob g_worldgen;
static int g_load_state_skip_terrain_rebuild = 0;
static int g_last_load_state_had_terrain = 0;
static int g_passive_mobs_need_initial_spawn = 0;
static char g_loaded_world_dir[MAX_PATHBUF] = "";
static char g_loaded_world_name[64] = "";
static int g_selected_hotbar_slot = 0;
static int g_player_health = 20;
static int g_player_prev_health = 20;
static int g_player_armor = 0;
static int g_player_natural_armor_rating = 0;
static int g_player_damage_remainder = 0;
static int g_player_food_level = 20;
static int g_player_prev_food_level = 20;
static float g_player_food_saturation = 5.0f;
static float g_player_food_exhaustion = 0.0f;
static int g_player_food_timer = 0;
static int g_player_air = 300;
static int g_player_fire_ticks = 0;
static int g_player_xp_level = 0;
static int g_player_xp_total = 0;
static float g_player_xp_progress = 0.0f;
static int g_player_xp_pickup_cooldown = 0; /* EntityPlayer.xpCooldown */

static void player_capabilities_apply_game_mode(void) {
    /* Java multiplayer capabilities are assigned by S39 Player Abilities.
       Re-deriving them from the local game-mode flag every tick could revoke
       server-granted flight or invulnerability and then make the client send
       movement that disagrees with the server.  Keep only the UI/gameplay
       creative classification here; the protocol handler owns the remaining
       capability bits while connected to protocol 47. */
    if (g_mp_connected && g_player_capabilities_server_authoritative) {
        g_player_capabilities.is_creative_mode = (g_game_mode == 1) ? 1 : 0;
        return;
    }
    if (g_game_mode == 1) {
        g_player_capabilities.disable_damage = 1;
        g_player_capabilities.allow_flying = 1;
        g_player_capabilities.is_creative_mode = 1;
    } else {
        g_player_capabilities.disable_damage = 0;
        g_player_capabilities.allow_flying = 0;
        g_player_capabilities.is_flying = 0;
        g_player_capabilities.is_creative_mode = 0;
    }
}
static int player_is_creative(void) {
    player_capabilities_apply_game_mode();
    return g_player_capabilities.is_creative_mode != 0;
}
static int player_damage_disabled(void) {
    player_capabilities_apply_game_mode();
    return g_player_capabilities.disable_damage != 0;
}
#define g_creative_flying (g_player_capabilities.is_flying)

static int g_ingame_ticks = 0;
static int g_hearts_life = 0; /* Java EntityLiving.heartsLife countdown, in 20 Hz ticks. */

#define PEX_HEARTS_HALVES_LIFE 20

static int player_health_clamp(int health) {
    if (health < 0) return 0;
    if (health > 20) return 20;
    return health;
}

static void player_health_set_silent(int health) {
    g_player_health = player_health_clamp(health);
    g_player_prev_health = g_player_health;
    g_player_natural_armor_rating = 0;
    g_hearts_life = 0;
}

static void player_health_set_hearts(int health) {
    int old_health = player_health_clamp(g_player_health);
    int new_health = player_health_clamp(health);
    if (new_health < old_health) {
        g_player_prev_health = old_health;
        g_player_health = new_health;
        g_hearts_life = PEX_HEARTS_HALVES_LIFE;
    } else if (new_health > old_health) {
        g_player_prev_health = old_health;
        g_player_health = new_health;
        g_hearts_life = PEX_HEARTS_HALVES_LIFE / 2;
    } else {
        g_player_health = new_health;
    }
}

static void player_health_damage_hearts(void) {
    g_player_prev_health = player_health_clamp(g_player_health);
    g_hearts_life = PEX_HEARTS_HALVES_LIFE;
}


static int player_food_clamp(int food) {
    if (food < 0) return 0;
    if (food > 20) return 20;
    return food;
}

static void player_food_reset(void) {
    g_player_food_level = 20;
    g_player_prev_food_level = 20;
    g_player_food_saturation = 5.0f;
    g_player_food_exhaustion = 0.0f;
    g_player_food_timer = 0;
}

static void player_food_sanitize(void) {
    g_player_food_level = player_food_clamp(g_player_food_level);
    g_player_prev_food_level = player_food_clamp(g_player_prev_food_level);
    if (g_player_food_saturation < 0.0f) g_player_food_saturation = 0.0f;
    if (g_player_food_saturation > (float)g_player_food_level) g_player_food_saturation = (float)g_player_food_level;
    if (g_player_food_exhaustion < 0.0f) g_player_food_exhaustion = 0.0f;
    if (g_player_food_exhaustion > 40.0f) g_player_food_exhaustion = 40.0f;
    if (g_player_food_timer < 0) g_player_food_timer = 0;
}

static void player_food_add_stats(int food, float saturation_modifier) {
    /* Multiplayer food state is supplied by the server's health packet.  Do not
       predict eating locally or the HUD can briefly disagree with the server. */
    if (g_mp_connected || food <= 0) return;
    g_player_prev_food_level = player_food_clamp(g_player_food_level);
    g_player_food_level = player_food_clamp(g_player_food_level + food);
    g_player_food_saturation += (float)food * saturation_modifier * 2.0f;
    if (g_player_food_saturation > (float)g_player_food_level) g_player_food_saturation = (float)g_player_food_level;
    if (g_player_food_saturation < 0.0f) g_player_food_saturation = 0.0f;
}

static int player_can_eat(int always_edible) {
    return !player_damage_disabled() && (always_edible || g_player_food_level < 20);
}

static void player_add_exhaustion(float amount) {
    /* Exhaustion/hunger are server-authoritative in every multiplayer backend. */
    if (g_mp_connected || player_is_creative()) return;
    if (amount <= 0.0f) return;
    g_player_food_exhaustion += amount;
    if (g_player_food_exhaustion > 40.0f) g_player_food_exhaustion = 40.0f;
    if (g_player_food_exhaustion < 0.0f) g_player_food_exhaustion = 0.0f;
}

static int player_xp_bar_cap(void) {
    if (g_player_xp_level < 0) g_player_xp_level = 0;
    return 7 + ((g_player_xp_level * 7) >> 1);
}

static void player_xp_sanitize(void) {
    if (g_player_xp_level < 0) g_player_xp_level = 0;
    if (g_player_xp_total < 0) g_player_xp_total = 0;
    if (g_player_xp_progress < 0.0f) g_player_xp_progress = 0.0f;
    if (g_player_xp_progress >= 1.0f) g_player_xp_progress = 0.999999f;
}

static void player_add_experience(int amount) {
    if (amount <= 0) return;
    int space = 0x7fffffff - g_player_xp_total;
    if (amount > space) amount = space;
    if (amount <= 0) return;

    g_player_xp_progress += (float)amount / (float)player_xp_bar_cap();
    g_player_xp_total += amount;
    while (g_player_xp_progress >= 1.0f) {
        g_player_xp_progress = (g_player_xp_progress - 1.0f) * (float)player_xp_bar_cap();
        g_player_xp_level++;
        g_player_xp_progress /= (float)player_xp_bar_cap();
    }
}

static void player_remove_experience(int levels) {
    g_player_xp_level -= levels;
    if (g_player_xp_level < 0) g_player_xp_level = 0;
}

static void player_xp_reset(void) {
    g_player_xp_level = 0;
    g_player_xp_total = 0;
    g_player_xp_progress = 0.0f;
}

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

/* 16x16 cells in src/assets/XINPUT.png.  The updated sheet keeps the face
   buttons on row zero, adds left/right-stick glyphs on row one, and moves the
   bumpers to row two.  Keep these names centralized so UI hints never depend
   on fragile numeric tile IDs. */
#define PEX_XINPUT_TILE_A   0
#define PEX_XINPUT_TILE_B   1
#define PEX_XINPUT_TILE_X   2
#define PEX_XINPUT_TILE_Y   3
#define PEX_XINPUT_TILE_LS  4
#define PEX_XINPUT_TILE_RS  5
#define PEX_XINPUT_TILE_RT  6
#define PEX_XINPUT_TILE_LT  7
#define PEX_XINPUT_TILE_LB  8
#define PEX_XINPUT_TILE_RB  9
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
#define FLAT_WORLD_SIZE 576
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
#define BLOCK_LAPIS_ORE 21
#define BLOCK_LAPIS_BLOCK 22
#define BLOCK_DISPENSER 23
#define BLOCK_SANDSTONE 24
#define BLOCK_NOTE_BLOCK 25
#define BLOCK_BED 26
#define BLOCK_POWERED_RAIL 27
#define BLOCK_DETECTOR_RAIL 28
#define BLOCK_STICKY_PISTON 29
#define BLOCK_WEB 30
#define BLOCK_TALL_GRASS 31
#define BLOCK_DEAD_BUSH 32
#define BLOCK_PISTON 33
#define BLOCK_PISTON_EXTENSION 34
#define BLOCK_WOOL 35
#define BLOCK_PISTON_MOVING 36
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
#define BLOCK_CAKE 92
#define BLOCK_REDSTONE_REPEATER_OFF 93
#define BLOCK_REDSTONE_REPEATER_ON 94
#define BLOCK_LOCKED_CHEST 95
#define BLOCK_TRAPDOOR 96
#define BLOCK_SILVERFISH 97
#define BLOCK_STONE_BRICK 98
#define BLOCK_BROWN_MUSHROOM_CAP 99
#define BLOCK_RED_MUSHROOM_CAP 100
#define BLOCK_IRON_BARS 101
#define BLOCK_GLASS_PANE 102
#define BLOCK_MELON 103
#define BLOCK_PUMPKIN_STEM 104
#define BLOCK_MELON_STEM 105
#define BLOCK_VINE 106
#define BLOCK_FENCE_GATE 107
#define BLOCK_BRICK_STAIRS 108
#define BLOCK_STONE_BRICK_STAIRS 109
#define BLOCK_MYCELIUM 110
#define BLOCK_LILY_PAD 111
#define BLOCK_NETHER_BRICK 112
#define BLOCK_NETHER_BRICK_FENCE 113
#define BLOCK_NETHER_BRICK_STAIRS 114
#define BLOCK_NETHER_WART 115
#define BLOCK_ENCHANTMENT_TABLE 116
#define BLOCK_BREWING_STAND 117
#define BLOCK_CAULDRON 118
#define BLOCK_END_PORTAL 119
#define BLOCK_END_PORTAL_FRAME 120
#define BLOCK_END_STONE 121
#define BLOCK_DRAGON_EGG 122
#define BLOCK_REDSTONE_LAMP_OFF 123
#define BLOCK_REDSTONE_LAMP_ON 124

typedef enum PexDimension {
    PEX_DIM_OVERWORLD = 0,
    PEX_DIM_NETHER = -1,
    PEX_DIM_END = 1
} PexDimension;

#define PEX_WORLDGEN_LEGACY_BETA 0
#define PEX_WORLDGEN_MODERN 1

/* Java 1.2.5 metadata values used by generated content. */
#define BLOCK_META_WOOD_OAK 0
#define BLOCK_META_WOOD_SPRUCE 1
#define BLOCK_META_WOOD_BIRCH 2
#define BLOCK_META_WOOD_JUNGLE 3
#define BLOCK_META_SANDSTONE_NORMAL 0
#define BLOCK_META_SANDSTONE_CHISELED 1
#define BLOCK_META_SANDSTONE_SMOOTH 2
#define BLOCK_META_STONE_BRICK_NORMAL 0
#define BLOCK_META_STONE_BRICK_MOSSY 1
#define BLOCK_META_STONE_BRICK_CRACKED 2
#define BLOCK_META_STONE_BRICK_CHISELED 3
#define BLOCK_META_TALL_GRASS_SHRUB 0
#define BLOCK_META_TALL_GRASS_GRASS 1
#define BLOCK_META_TALL_GRASS_FERN 2

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
#define ITEM_DYE_POWDER 351
#define ITEM_BONE 352
#define ITEM_SUGAR 353
#define ITEM_CAKE 354
#define ITEM_BED 355
#define ITEM_REDSTONE_REPEATER 356
#define ITEM_COOKIE 357
#define ITEM_MAP 358
#define ITEM_SHEARS 359
#define ITEM_MELON 360
#define ITEM_PUMPKIN_SEEDS 361
#define ITEM_MELON_SEEDS 362
#define ITEM_BEEF_RAW 363
#define ITEM_BEEF_COOKED 364
#define ITEM_CHICKEN_RAW 365
#define ITEM_CHICKEN_COOKED 366
#define ITEM_ROTTEN_FLESH 367
#define ITEM_ENDER_PEARL 368
#define ITEM_BLAZE_ROD 369
#define ITEM_GHAST_TEAR 370
#define ITEM_GOLD_NUGGET 371
#define ITEM_NETHER_WART 372
#define ITEM_POTION 373
#define ITEM_GLASS_BOTTLE 374
#define ITEM_SPIDER_EYE 375
#define ITEM_FERMENTED_SPIDER_EYE 376
#define ITEM_BLAZE_POWDER 377
#define ITEM_MAGMA_CREAM 378
#define ITEM_BREWING_STAND 379
#define ITEM_CAULDRON 380
#define ITEM_EYE_OF_ENDER 381
#define ITEM_SPECKLED_MELON 382
#define ITEM_MONSTER_PLACER 383
#define ITEM_EXP_BOTTLE 384
#define ITEM_FIREBALL_CHARGE 385
#define ITEM_RECORD13 2256
#define ITEM_RECORD_CAT 2257
#define ITEM_RECORD_BLOCKS 2258
#define ITEM_RECORD_CHIRP 2259
#define ITEM_RECORD_FAR 2260
#define ITEM_RECORD_MALL 2261
#define ITEM_RECORD_MELLOHI 2262
#define ITEM_RECORD_STAL 2263
#define ITEM_RECORD_STRAD 2264
#define ITEM_RECORD_WARD 2265
#define ITEM_RECORD_11 2266
#define ITEM_RECORD_TWOFACE 2267
/* Legacy/alias IDs kept so old save/tool code still compiles while the Java names are available. */
#define BLOCK_WOOD_PLANKS BLOCK_PLANKS
#define BLOCK_NOTE BLOCK_NOTE_BLOCK
#define BLOCK_POWERED_RAILS BLOCK_POWERED_RAIL
#define BLOCK_DETECTOR_RAILS BLOCK_DETECTOR_RAIL
#define BLOCK_TALLGRASS BLOCK_TALL_GRASS
#define BLOCK_DEADBUSH BLOCK_DEAD_BUSH
#define BLOCK_STONEBRICK BLOCK_STONE_BRICK
#define BLOCK_IRON_FENCE BLOCK_IRON_BARS
#define BLOCK_THIN_GLASS BLOCK_GLASS_PANE
#define BLOCK_WATERLILY BLOCK_LILY_PAD
#define BLOCK_NETHER_FENCE BLOCK_NETHER_BRICK_FENCE
#define BLOCK_NETHER_STALK BLOCK_NETHER_WART
#define BLOCK_WHITE_STONE BLOCK_END_STONE
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
#define ITEM_DYE ITEM_DYE_POWDER
#define ITEM_COOKED_FISH ITEM_FISH_COOKED
#define ITEM_RAW_FISH ITEM_FISH_RAW
#define ITEM_GOLDEN_APPLE ITEM_APPLE_GOLD
#define ITEM_COOKED_PORKCHOP ITEM_PORK_COOKED
#define ITEM_RAW_PORKCHOP ITEM_PORK_RAW
#define ITEM_COOKED_BEEF ITEM_BEEF_COOKED
#define ITEM_RAW_BEEF ITEM_BEEF_RAW
#define ITEM_COOKED_CHICKEN ITEM_CHICKEN_COOKED
#define ITEM_RAW_CHICKEN ITEM_CHICKEN_RAW
#define ITEM_BLAZE_POWDER_ALIAS ITEM_BLAZE_POWDER
#define ITEM_NETHER_STALK_SEEDS ITEM_NETHER_WART
#define ITEM_EYE_ENDER ITEM_EYE_OF_ENDER
#define ITEM_SPAWN_EGG ITEM_MONSTER_PLACER
#define ITEM_XP_BOTTLE ITEM_EXP_BOTTLE
#define ITEM_FIRE_CHARGE ITEM_FIREBALL_CHARGE


#define PEX_ITEMSTACK_ENCHANT_MAX 4
#define PEX_ENCHANT_AQUA_AFFINITY 6
#define PEX_ENCHANT_LOOTING 21
#define PEX_ENCHANT_EFFICIENCY 32

typedef struct ItemStack {
    int id;
    int count;
    int damage;
    int pop_time;
    /* Minimal Java 1.2.5 ItemStack enchantment-NBT port: enchantment
       compounds store short id/lvl pairs under the "ench" list.  Keep a
       compact fixed array in C so Looting and rare-drop enchant branches can
       be represented without cloning Minecraft's full NBT heap model. */
    int enchant_id[PEX_ITEMSTACK_ENCHANT_MAX];
    int enchant_level[PEX_ITEMSTACK_ENCHANT_MAX];
    /* Java display.color from leather-armor NBT. Kept on the stack so remote
       multiplayer equipment can be tinted without changing single-player
       armor behavior. */
    int has_custom_color;
    int custom_color;
} ItemStack;

int pex_java47_try_interact_entity(float max_dist);
ItemStack *pex_java47_get_open_container_slot(int local_slot);
int pex_java47_open_container_slot_count(void);
const char *pex_java47_slot_custom_name(int pex_slot);
int pex_java47_slot_lore(int pex_slot, const char **out_lines, int max_lines);

static void armor_sync_player_armor(void);
static int armor_stack_type(int id);
static int armor_stack_material_index(int id);
static int armor_apply_damage_reduction(int incoming);

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
    int health;       /* EntityItem.health, initialized to 5. */
    int on_ground;
} FlatDroppedItem;

#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
#define MAX_VEHICLE_ENTITIES 16
#define MAX_JUKEBOX_TILES 32
#else
#define MAX_VEHICLE_ENTITIES 64
#define MAX_JUKEBOX_TILES 256
#endif

#define FLAT_VEHICLE_BOAT 1
#define FLAT_VEHICLE_MINECART_RIDEABLE 2
#define FLAT_VEHICLE_MINECART_CHEST 3
#define FLAT_VEHICLE_MINECART_FURNACE 4

typedef struct FlatVehicle {
    int active;
    int type;
    float x, y, z;
    float prev_x, prev_y, prev_z;
    float mx, my, mz;
    float yaw, prev_yaw;
    int age;
    int on_ground;
} FlatVehicle;

typedef struct JukeboxTile {
    int active;
    int x, y, z;
    int record_item;
} JukeboxTile;

#define PEX_POTION_MAX 32
#define PEX_POTION_SPEED 1
#define PEX_POTION_SLOWNESS 2
#define PEX_POTION_HASTE 3
#define PEX_POTION_MINING_FATIGUE 4
#define PEX_POTION_STRENGTH 5
#define PEX_POTION_HEAL 6
#define PEX_POTION_HARM 7
#define PEX_POTION_JUMP 8
#define PEX_POTION_NAUSEA 9
#define PEX_POTION_REGENERATION 10
#define PEX_POTION_RESISTANCE 11
#define PEX_POTION_FIRE_RESISTANCE 12
#define PEX_POTION_WATER_BREATHING 13
#define PEX_POTION_INVISIBILITY 14
#define PEX_POTION_BLINDNESS 15
#define PEX_POTION_NIGHT_VISION 16
#define PEX_POTION_HUNGER 17
#define PEX_POTION_WEAKNESS 18
#define PEX_POTION_POISON 19

typedef struct PexPotionEffectState {
    int duration;
    int amplifier;
} PexPotionEffectState;

#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
#define MAX_MAP_DATA 8
#else
#define MAX_MAP_DATA 64
#endif

typedef struct PexMapData {
    int active;
    int id;
    int x_center;
    int z_center;
    int dimension;
    int scale;
    int tick;
    unsigned char colors[128 * 128];
} PexMapData;

#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
#define MAX_PROJECTILE_ENTITIES 16
#define MAX_XP_ORBS 64
#else
#define MAX_PROJECTILE_ENTITIES 64
#define MAX_XP_ORBS 512
#endif

#define FLAT_PROJECTILE_POTION 1
#define FLAT_PROJECTILE_XP_BOTTLE 2
#define FLAT_PROJECTILE_ARROW 3
#define FLAT_PROJECTILE_SMALL_FIREBALL 4
#define FLAT_PROJECTILE_LARGE_FIREBALL 5
#define FLAT_PROJECTILE_SNOWBALL 6

typedef struct FlatProjectile {
    int active;
    int type;
    int item_damage;
    int owner_type;   /* 0=player/local thrower, 1=mob */
    int owner_mob_type;
    int owner_mob_index;
    int damage;
    int critical;
    float x, y, z;
    float prev_x, prev_y, prev_z;
    float mx, my, mz;
    float yaw, pitch;
    float prev_yaw, prev_pitch;
    int age;
    int fire_ticks;
    int in_ground;
    /* EntityArrow 1.2.5 state. Other projectile types leave these zero. */
    int tile_x, tile_y, tile_z;
    int in_tile, in_data;
    int ticks_in_ground, ticks_in_air;
    int arrow_shake;
    int player_arrow;
    int arrow_knockback;
} FlatProjectile;

typedef struct FlatXPOrb {
    int active;
    int value;
    float x, y, z;
    float prev_x, prev_y, prev_z;
    float mx, my, mz;
    float rot;
    int age;
    int color;
    int pickup_delay; /* EntityXPOrb.field_35126_c */
    int health;
    int on_ground;
} FlatXPOrb;

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
static int g_flat_recent_block_mesh_dirty_tick = -1000000;
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
/* Absolute identity of the terrain currently published in each logical slot.
   The generated bit alone is not enough while the streaming window moves: a
   stale local slot can otherwise be mistaken for the new world coordinate for
   one frame. */
static int g_flat_chunk_world_cx[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
static int g_flat_chunk_world_cz[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
/* A generated chunk may exist before its neighbor-aware skylight has settled.
   Meshes stamp the light version they were built from so spawn/preload chunks
   cannot keep random brightness until a player block edit happens to dirty them. */
static int g_flat_chunk_light_ready[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
/* ready = a complete seed buffer exists and may be rendered; valid = the
   neighbor-aware propagation pass has committed for the current neighbors. */
static int g_flat_chunk_light_valid[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
static unsigned int g_flat_chunk_light_version[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
/* Diagnostic tag for chunks published by the loading-screen spawn preload.
   This is not gameplay logic; it lets F3+V distinguish the chunks that should
   have been lit during initial world load from chunks streamed later while
   walking. */
static int g_flat_chunk_initial_preload[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];

/* Java-b1 style render sections.  A section is 16x16x16, so editing or
   generating one part of a chunk no longer rebuilds the whole 256-block-tall
   column.  Pass 0 is solid/cutout geometry, pass 1 is fancy liquids. */
#define FLAT_RENDER_SECTION 16
#define FLAT_RENDER_SECTIONS_Y ((FLAT_WORLD_HEIGHT + FLAT_RENDER_SECTION - 1) / FLAT_RENDER_SECTION)
static GLuint g_flat_section_lists[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS][2];
static unsigned int g_flat_section_direct_mesh[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS][2];
static int g_flat_section_dirty[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
static int g_flat_section_valid[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
/* Strict state identity.  mesh_version is the terrain revision and changes only
   for block/stream/remap geometry.  chunk_light_version is the light revision
   and changes only after an explicit validated light commit.  build stamps make
   the async queue one-job-per-section-per-version instead of a rebuild loop. */
static unsigned int g_flat_section_mesh_version[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
static unsigned int g_flat_section_mesh_light_version[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
static unsigned int g_flat_section_building_mesh_version[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
static unsigned int g_flat_section_building_light_version[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
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
/*
 * The terrain volumes are a physical ring, not a sliding array.  A world X/Z
 * coordinate always maps to the same physical cell modulo FLAT_WORLD_SIZE.
 * Moving the active window therefore changes only the logical origin and the
 * small per-chunk metadata tables; the 3D block/light arrays are never memmoved.
 *
 * Desktop uses a 36x36-chunk (576-block) ring so render distance 16 has
 * room for the full 33-chunk diameter plus safety slots.  Use mathematical
 * modulo so the same mapping also works for negative world coordinates and the
 * smaller console window sizes.
 */
static int flat_storage_ring_index(int world_coord) {
    int r = world_coord % FLAT_WORLD_SIZE;
    return r < 0 ? r + FLAT_WORLD_SIZE : r;
}
static int flat_storage_x_world(int world_x) {
    return flat_storage_ring_index(world_x);
}
static int flat_storage_z_world(int world_z) {
    return flat_storage_ring_index(world_z);
}
static int flat_storage_x_local(int local_x) {
    return flat_storage_x_world(g_flat_world_origin_x + local_x);
}
static int flat_storage_z_local(int local_z) {
    return flat_storage_z_world(g_flat_world_origin_z + local_z);
}
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
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
/* PSP/Wii spawn/platform guard.  Real Beta terrain generation is async on PSP,
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
static ItemStack g_armor_inventory[4]; /* 0 boots, 1 leggings, 2 chestplate, 3 helmet */
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
static FlatVehicle g_vehicles[MAX_VEHICLE_ENTITIES];
static JukeboxTile g_jukebox_tiles[MAX_JUKEBOX_TILES];
static PexPotionEffectState g_player_potion_effects[PEX_POTION_MAX];
static PexMapData g_map_data[MAX_MAP_DATA];
static int g_next_map_id = 0;
#define PEX_DEAR_MEMORIES_BOOK_DAMAGE 1250427
static int g_resource_pack_cache_revision = 0;
static FlatProjectile g_projectiles[MAX_PROJECTILE_ENTITIES];
static FlatXPOrb g_xp_orbs[MAX_XP_ORBS];
#define MAX_PICKUP_FX 32
static PickupFx g_pickup_fx[MAX_PICKUP_FX];
static FlatFallingBlock g_falling_blocks[MAX_FALLING_BLOCK_ENTITIES];
static PassiveMob g_passive_mobs[MAX_PASSIVE_MOBS];
static int g_player_riding_passive_mob = -1;
/* Single-player owner AI memory, matching EntityLiving.getAITarget() and
   getLastAttackingEntity().  Multiplayer remains deliberately out of scope. */
static int g_player_last_hurt_by_mob_entity_id = -1;
static int g_player_last_hurt_by_mob_ticks = 0;
static int g_player_last_attacked_mob_entity_id = -1;
static int g_player_last_attacked_mob_ticks = 0;

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
    long long world_time;
    int world_type;
    int game_mode;
    int dimension;
    PexPlayerCapabilities player_capabilities;
    float player_x, player_y, player_z;
    float player_yaw, player_pitch;
    int player_health;
    int player_food_level;
    int player_prev_food_level;
    float player_food_saturation;
    float player_food_exhaustion;
    int player_food_timer;
    int player_fire_ticks;
    int player_xp_level;
    int player_xp_total;
    float player_xp_progress;
    int player_armor;
    int selected_hotbar_slot;
    float player_fall_distance;
    int player_dead;
    ItemStack inventory[36];
    ItemStack armor_inventory[4];

    ChestTile chest_tiles[MAX_CHEST_TILES];
    FurnaceTile furnace_tiles[MAX_FURNACE_TILES];
    FlatDroppedItem drops[MAX_DROP_ENTITIES];
    FlatXPOrb xp_orbs[MAX_XP_ORBS];
    FlatProjectile projectiles[MAX_PROJECTILE_ENTITIES];
    FlatVehicle vehicles[MAX_VEHICLE_ENTITIES];
    JukeboxTile jukebox_tiles[MAX_JUKEBOX_TILES];
    PexPotionEffectState player_potion_effects[PEX_POTION_MAX];
    PexMapData map_data[MAX_MAP_DATA];
    int next_map_id;
    PassiveMob passive_mobs[MAX_PASSIVE_MOBS];
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
typedef enum ParticleKind { PARTICLE_DIG = 0, PARTICLE_BUBBLE = 1, PARTICLE_SPLASH = 2, PARTICLE_PORTAL = 3, PARTICLE_CRIT = 4, PARTICLE_SMOKE = 5, PARTICLE_FLAME = 6, PARTICLE_EXPLODE = 7 } ParticleKind;

typedef struct DigParticle {
    int active;
    int tile;
    int kind;
    float x, y, z;
    float prev_x, prev_y, prev_z;
    float mx, my, mz;
    float ox, oy, oz;
    float r, g, b;
    float scale;
    float base_scale;
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
/* Java servers use the on-ground bit for movement validation.  Keep a
   network-facing value that is set only by a real downward collision rather
   than the looser support probe used to make the local 1.2.5 controller feel
   stable at block edges. */
static int g_player_server_on_ground = 1;
static int g_player_was_in_water = 0;
static float g_distance_walked = 0.0f, g_prev_distance_walked = 0.0f;
static float g_next_footstep_distance = 0.0f;
static float g_limb_swing = 0.0f, g_prev_limb_swing = 0.0f;
static float g_limb_swing_amount = 0.0f, g_prev_limb_swing_amount = 0.0f;
static float g_camera_yaw = 0.0f, g_prev_camera_yaw = 0.0f;
static float g_camera_pitch = 0.0f, g_prev_camera_pitch = 0.0f;
static float g_render_arm_yaw = 0.0f, g_prev_render_arm_yaw = 0.0f;
static float g_render_arm_pitch = 0.0f, g_prev_render_arm_pitch = 0.0f;
static float g_frame_partial = 0.0f;

typedef struct PexPlayerRenderState {
    float x, y, z;
    float prev_x, prev_y, prev_z;
    float yaw, pitch;
    float prev_yaw, prev_pitch;
    float distance_walked, prev_distance_walked;
    float limb_swing, prev_limb_swing;
    float limb_swing_amount, prev_limb_swing_amount;
    float camera_yaw, prev_camera_yaw;
    float camera_pitch, prev_camera_pitch;
    float render_arm_yaw, prev_render_arm_yaw;
    float render_arm_pitch, prev_render_arm_pitch;
    int dead;
    int death_time;
    int hurt_time;
    int max_hurt_time;
    float attacked_at_yaw;
    int ingame_ticks;
} PexPlayerRenderState;

static PexPlayerRenderState g_player_render_published;
static PexPlayerRenderState g_player_render_frame;
static int g_player_render_published_valid = 0;
static int g_player_render_frame_from_async_partial = 0;

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
    PROF_GAMEPAD_POLL,
    PROF_NET_POLL,
    PROF_CLOCK_DT,
    PROF_TICK_TOTAL,
    PROF_TICK_TITLE,
    PROF_WORLDGEN_TICK,
    PROF_TICK_PACK_INSTALL,
    PROF_TICK_INGAME_ENQUEUE,
    PROF_ASYNC_RENDER_PARTIAL,
    PROF_NET_SMOOTHING,
    PROF_ASYNC_TICK_PUMP,
    PROF_INGAME_TOTAL,
    PROF_DAYLIGHT_MESH,
    PROF_INVENTORY,
    PROF_FURNACE,
    PROF_DROPS,
    PROF_PARTICLES,
    PROF_FALLING,
    PROF_WORLD_STREAM,
    PROF_LIQUIDS,
    PROF_BUTTONS,
    PROF_PLAYER_LOGIC,
    PROF_RENDER_FPS_UPDATE,
    PROF_PLAYER_RENDER_SNAPSHOT,
    PROF_RENDERER_BEGIN_FRAME,
    PROF_RENDER_CLEAR_SETUP,
    PROF_DRAW_CURRENT_SCREEN,
    PROF_MESH_MAIN,
    PROF_CULL_SORT,
    PROF_WORLD_DRAW,
    PROF_WORLD_ENTITIES,
    PROF_ENTITY_LOCAL_PLAYER,
    PROF_ENTITY_REMOTE_PLAYERS,
    PROF_ENTITY_MATRIX_READBACK,
    PROF_ENTITY_FALLING_BLOCKS,
    PROF_ENTITY_PASSIVE_MOBS,
    PROF_WORLD_PARTICLES,
    PROF_WORLD_TRANSLUCENT,
    PROF_WORLD_OVERLAYS,
    PROF_WORLD_CLOUDS,
    PROF_HUD_GUI,
    PROF_SCREEN_DRAW,
    PROF_FPS_COUNTER,
    PROF_GAMEPAD_CURSOR,
    PROF_PRESENT,
    PROF_MESH_SELF_HEAL,
    PROF_MESH_RESULT_INSTALL,
    PROF_MESH_SUBMIT_SNAPSHOT,
    PROF_RENDER_TOTAL,
    PROF_SLEEP_LIMIT,
    PROF_LOGGY_REFRESH,
    PROF_COUNT
} PexMainThreadProfileId;

static const char *g_prof_names[PROF_COUNT] = {
    "Pump/messages",
    "Gamepad poll",
    "Net poll/apply",
    "Clock/delta",
    "Tick total",
    "Title tick",
    "Worldgen tick",
    "Pack install tick",
    "Ingame enqueue",
    "Async render partial",
    "Net smoothing",
    "Async tick pump",
    "Ingame tick",
    "Daylight mesh",
    "Inventory",
    "Furnaces",
    "Drops/items",
    "Particles",
    "Sand+gravel",
    "World streaming",
    "Liquids",
    "Buttons/plates",
    "Player logic",
    "Render FPS update",
    "Player render snap",
    "Renderer begin frame",
    "Clear/setup",
    "Draw current screen",
    "Mesh install",
    "Cull/sort",
    "World draw solid",
    "World entities",
    "Entity local player",
    "Entity remote players",
    "Entity matrix readback",
    "Entity falling blocks",
    "Entity passive mobs",
    "World particles",
    "World translucent",
    "World overlays",
    "World clouds",
    "HUD/GUI",
    "Screen draw",
    "FPS counter",
    "Gamepad cursor",
    "Present/swap",
    "Mesh self-heal",
    "Mesh result install",
    "Mesh submit snapshot",
    "Render total",
    "FPS limiter sleep",
    "Loggy refresh"
};

static double g_prof_frame_ms[PROF_COUNT];
static double g_prof_accum_ms[PROF_COUNT];
static double g_prof_display_ms[PROF_COUNT];
static double g_prof_frame_start_time = 0.0;
static double g_prof_accum_frame_ms = 0.0;
static double g_prof_display_frame_ms = 0.0;
static double g_prof_accum_start_time = 0.0;
static int g_prof_accum_frames = 0;

typedef enum PexProfileThreadRole {
    PEX_PROFILE_ROLE_MAIN = 0,
    PEX_PROFILE_ROLE_ASYNC_TICK = 1,
    PEX_PROFILE_ROLE_ASYNC_MESH = 2,
    PEX_PROFILE_ROLE_ASYNC_STREAM = 3
} PexProfileThreadRole;

/* The F3 profiler is explicitly a render/main-thread profiler.  Worker threads
   set this TLS role so their timings do not get mixed into "Main thread average"
   and so worker writes do not race with the frame accumulator. */
static PEX_THREAD_LOCAL int g_pex_profile_thread_role = PEX_PROFILE_ROLE_MAIN;
static double g_prof_async_tick_last_ms = 0.0;
static double g_prof_async_tick_avg_ms = 0.0;
static int g_prof_async_tick_samples = 0;
static double g_prof_stream_worker_last_ms = 0.0;
static double g_prof_stream_worker_avg_ms = 0.0;
static int g_prof_stream_worker_samples = 0;
static double g_prof_stream_worker_terrain_ms = 0.0;
static double g_prof_stream_worker_delta_ms = 0.0;
static double g_prof_stream_worker_light_ms = 0.0;
static double g_prof_stream_worker_push_wait_ms = 0.0;
static int g_prof_stream_worker_last_cx = 0;
static int g_prof_stream_worker_last_cz = 0;
static double g_prof_stream_service_install_ms = 0.0;
static double g_prof_stream_service_submit_ms = 0.0;
static int g_prof_stream_service_installs = 0;
static double g_prof_stream_install_one_ms = 0.0;
static double g_prof_light_worker_last_ms = 0.0;
static double g_prof_light_worker_avg_ms = 0.0;
static int g_prof_light_worker_samples = 0;
static double g_prof_mesh_worker_last_ms = 0.0;
static double g_prof_mesh_worker_avg_ms = 0.0;
static int g_prof_mesh_worker_samples = 0;
static double g_prof_mesh_upload_worker_last_ms = 0.0;
static double g_prof_mesh_upload_worker_avg_ms = 0.0;
static int g_prof_mesh_upload_worker_samples = 0;
static int g_prof_skylight_subtracted_last = 0;
static int g_prof_village_scan_blocks_last = 0;
static int g_prof_spawn_y_cache_hits = 0;
static int g_prof_spawn_y_cache_misses = 0;
static int g_prof_mob_spawn_probe_budget_last = 0;
static int g_prof_mob_spawn_probe_hits_last = 0;
static int g_prof_mob_spawn_probe_misses_last = 0;
static int g_prof_mob_spawn_calls_last = 0;
static int g_prof_mob_spawn_columns_last = 0;
static double g_prof_mob_spawn_ms_last = 0.0;
static double g_prof_mob_ender_ms_last = 0.0;
static double g_prof_mob_village_ms_last = 0.0;
static double g_prof_mob_natural_spawn_ms_last = 0.0;
static double g_prof_village_refresh_ms_last = 0.0;
static int g_prof_village_refresh_candidates_last = 0;
static int g_prof_village_refresh_biome_queries_last = 0;
static double g_prof_mob_spawner_ms_last = 0.0;
static int g_prof_mob_spawner_scan_blocks_last = 0;
static int g_prof_mob_spawner_active_last = 0;
static double g_prof_mob_living_ms_last = 0.0;
static int g_prof_mob_living_ticked_last = 0;
static int g_prof_mob_living_deferred_last = 0;
static int g_prof_mob_path_solves_last = 0;
static int g_prof_mob_path_nodes_last = 0;
static int g_prof_mob_path_failed_last = 0;
static int g_prof_mob_path_peak_nodes_last = 0;
static double g_prof_mob_path_ms_last = 0.0;

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

/* --loggy diagnostic counters.  They are intentionally lightweight and are
   only displayed by the SDL2 diagnostic window when requested. */
static int g_loggy_enabled = 0;
static int g_loggy_frame_no = 0;
static int g_loggy_profile_calls[PROF_COUNT];
static int g_loggy_gui_text_calls = 0;
static int g_loggy_gui_text_chars = 0;
static int g_loggy_gui_quads = 0;
static int g_loggy_gui_buttons = 0;
static int g_loggy_mesh_submit_calls = 0;
static int g_loggy_mesh_submit_snapshot_cells = 0;
static int g_loggy_mesh_installs = 0;
static int g_loggy_mesh_install_vertices = 0;
static int g_loggy_mesh_install_indices = 0;
static int g_loggy_mesh_stale_results = 0;
static int g_loggy_mesh_self_heal_refs = 0;
static int g_loggy_mesh_self_heal_missing = 0;
static int g_loggy_mesh_edit_priority_queued = 0;
static int g_loggy_mesh_edit_priority_drained = 0;
static int g_loggy_mesh_edit_priority_sync = 0;
static int g_loggy_mesh_edit_priority_async = 0;
static int g_loggy_mesh_edit_priority_left = 0;
static int g_loggy_stream_async_jobs = 0;
static int g_loggy_stream_async_active = 0;
static int g_loggy_stream_async_results = 0;
static int g_loggy_stream_queue_remaining = 0;
static int g_loggy_stream_queue_total = 0;
static int g_loggy_stream_queue_index = 0;
static int g_loggy_stream_installed_total = 0;
static int g_loggy_stream_remap_active = 0;
static int g_loggy_visible_sections = 0;
static int g_loggy_visible_chunks = 0;
static int g_loggy_ticks_this_frame = 0;
static double g_loggy_tick_accum = 0.0;
static double g_loggy_dt_ms = 0.0;
static double g_loggy_partial = 0.0;
static int g_loggy_win_msg_count = 0;
static int g_loggy_win_quit_seen = 0;
static unsigned int g_loggy_win_last_msg = 0;
static unsigned long g_loggy_win_last_msg_time = 0;
static double g_loggy_last_frame_wall_ms = 0.0;
static double g_loggy_last_frame_begin_time = 0.0;
static double g_loggy_last_frame_end_time = 0.0;
static int g_loggy_xinput_calls = 0;
static int g_loggy_xinput_ok = 0;
static int g_loggy_xinput_fail = 0;
static int g_loggy_xinput_skipped = 0;
static int g_loggy_winmm_numdev_calls = 0;
static int g_loggy_winmm_pos_calls = 0;
static int g_loggy_winmm_pos_ok = 0;
static int g_loggy_winmm_pos_fail = 0;
static int g_loggy_winmm_caps_calls = 0;
static int g_loggy_winmm_slots_skipped = 0;
static int g_loggy_gamepad_connected = 0;
static int g_loggy_gamepad_primary = -1;
static int g_loggy_gamepad_focus = 0;
static int g_loggy_clipboard_copies = 0;
static int g_loggy_edit_refreshes = 0;
static double g_loggy_edit_refresh_last_ms = 0.0;
static int g_loggy_d3d11_active = 0;
static int g_loggy_d3d11_allow_tearing = 0;
static unsigned int g_loggy_d3d11_swap_flags = 0;
static unsigned int g_loggy_d3d11_present_flags = 0;
static long g_loggy_d3d11_present_hr = 0;
static int g_loggy_d3d11_present_failures = 0;
static int g_loggy_d3d11_buffer_count = 0;
static int g_loggy_d3d11_frame_latency = 0;
static int g_loggy_d3d11_frame_latency_set = 0;
static int g_loggy_d3d11_present_stall_warning = 0;
static int g_loggy_entity_remote_players = 0;
static int g_loggy_entity_matrix_reads = 0;
static int g_loggy_entity_matrix_skips = 0;
static int g_loggy_entity_passive_entries = 0;

static int g_third_person_view = 0; /* Java 1.2.5 GameSettings.thirdPersonView: 0=first, 1=back, 2=front. */
static void third_person_view_cycle(void) {
    g_third_person_view++;
    if (g_third_person_view > 2) g_third_person_view = 0;
}

static int g_debug_menu_shown = 0;
static int g_debug_chunk_info_shown = 0;
static int g_debug_task_info_shown = 0;
static int g_mouse_grabbed = 0;
static int g_cursor_hidden = 0;
static int g_recentering_mouse = 0;
static char g_chat_input[128] = "";
static int g_suppress_next_chat_char = 0;
#define MAX_CHAT_LINES 50
typedef struct ChatLine { char text[256]; int age; } ChatLine;
static ChatLine g_chat_lines[MAX_CHAT_LINES];
static int g_chat_count = 0;
/* Java 1.2.5 GuiIngame recordPlaying state.  Set by record playback, drawn
   above the hotbar for 60 ticks; not a chat line. */
static char g_record_playing_text[128] = "";
static int g_record_playing_up_for = 0;
static int g_record_is_playing = 0;
static unsigned int g_crc_table[256];
static int g_crc_table_ready = 0;
static char g_splash[MAX_LABEL] = "Finally beta!";
static double g_last_time = 0.0;
static long long g_world_time = 0;

static Texture tex_bg, tex_gui, tex_font, tex_terrain, tex_black, tex_pack, tex_default_pack_icon, tex_unknown_pack;
/* Java 1.2.5 FontRenderer Unicode glyph pages: /font/glyph_sizes.bin and /font/glyph_%02X.png.
   Pages are loaded lazily by the GUI font renderer so non-Latin language names
   from /lang/languages.txt render instead of disappearing at byte >= 127. */
static Texture tex_font_glyph[256];
static unsigned char font_glyph_widths[65536];
static int font_glyph_widths_loaded = 0;
static Texture tex_icons, tex_inventory, tex_allitems, tex_workbench, tex_furnace_gui, tex_chest_gui, tex_items, tex_achievement, tex_stat_slot, tex_steve, tex_xinput;
static Texture tex_armor[5][2];
static Texture tex_mob_pig, tex_mob_sheep, tex_mob_sheep_fur, tex_mob_cow, tex_mob_chicken, tex_mob_saddle;
static Texture tex_mob_creeper, tex_mob_creeper_power, tex_mob_skeleton, tex_mob_spider, tex_mob_spider_eyes, tex_mob_zombie, tex_mob_slime;
static Texture tex_mob_ghast, tex_mob_ghast_fire, tex_mob_pigzombie, tex_mob_enderman, tex_mob_enderman_eyes, tex_mob_cavespider;
static Texture tex_mob_silverfish, tex_mob_blaze, tex_mob_lava, tex_mob_enderdragon;
static Texture tex_mob_squid, tex_mob_wolf, tex_mob_wolf_tame, tex_mob_wolf_angry, tex_mob_mooshroom;
static Texture tex_mob_snowman, tex_mob_ocelot, tex_mob_cat_black, tex_mob_cat_red, tex_mob_cat_siamese;
static Texture tex_mob_villager_golem, tex_mob_villager, tex_mob_villager_farmer, tex_mob_villager_librarian, tex_mob_villager_priest, tex_mob_villager_smith, tex_mob_villager_butcher;
static Texture tex_chest_entity, tex_large_chest_entity, tex_clouds;
static Texture tex_sun, tex_moon, tex_moon_phases;
static Texture tex_water_overlay, tex_shadow, tex_xporb, tex_arrows;
static Texture tex_grasscolor, tex_foliagecolor, tex_watercolor;
static Texture tex_pinecolor, tex_birchcolor, tex_swampgrasscolor, tex_swampfoliagecolor;
static int stivufine_lilypad_color = -1;
static Texture tex_particles;
static Texture tex_title_logo, tex_mojang, tex_panorama[6];
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
static double g_title_enter_time = 0.0;
static int g_menu_music_started = 0;
static int g_boot_sequence_done = 0;

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
static const char *key_action_names[PEX_KEY_BIND_COUNT] = {"Forward", "Left", "Back", "Right", "Jump", "Sneak", "Drop", "Inventory", "Chat", "Toggle Fog", "Sprint"};
static const char *key_option_names[PEX_KEY_BIND_COUNT] = {"key.forward", "key.left", "key.back", "key.right", "key.jump", "key.sneak", "key.drop", "key.inventory", "key.chat", "key.fog", "key.sprint"};
static const int default_keys[PEX_KEY_BIND_COUNT] = {'W','A','S','D',VK_SPACE,VK_SHIFT,'Q','E','T','F',VK_CONTROL};
static int lwjgl_to_vk(int code);
static int vk_to_lwjgl(int vk);
static void init_font_widths(void);
static void save_options(void);
static void set_screen(ScreenId s);
static int load_custom_skin_path(const char *path, int persist);
static int choose_and_import_skin(void);
static const char *tr(const char *en);
static const char *tr_key(const char *key);
static const char *tr_key_default(const char *key, const char *fallback);
static void pex_set_language_code(const char *code);
static void pex_language_refresh_available(void);
static int pex_language_count(void);
static const char *pex_language_code_at(int idx);
static const char *pex_language_name_at(int idx);
static int pex_current_language_index(void);
static const char *pex_current_language_code(void);
static int pex_current_language_is_unicode(void);
static int pex_current_language_is_bidi(void);
static int pex_language_runtime_files_available(void);
static void language_scroll_by(int rows);
static void language_mouse_down(int mx, int my);
static void language_mouse_up(void);
static void language_drag_scroll(int delta_y);
static void language_ensure_selected_visible(void);
static int pex_language_download_from_jar_blocking(void);
static void open_notice(const char *title, const char *line1, const char *line2);

/* Statistics/achievement screens are defined after item rendering in the unity build. */
static void draw_achievements_screen(void);
static void draw_statistics_screen(void);
static void pex_achievements_mouse_down(int mx, int my);
static void pex_achievements_mouse_drag(int mx, int my);
static void pex_achievements_mouse_up(void);
static void pex_statistics_scroll_by(int rows);
static void pex_statistics_mouse_down(int mx, int my);
static void pex_statistics_mouse_drag(int mx, int my);
static void pex_statistics_mouse_up(void);
static void pex_statistics_set_tab(int tab);
static void pex_draw_achievement_toast(void);
static int renderer_backend_supported(int backend);
static const char *renderer_backend_label(int backend);
static void restart_application_now(void);
static void start_world_generation(int slot);
static void start_world_generation_in_dir(const char *world_dir, const char *world_name, int slot);
static void worldgen_tick(void);
static int world_quit_is_active(void);
static int world_quit_progress_percent(void);
static const char *world_quit_stage_text(void);
static void world_quit_pump(void);
static void world_quit_shutdown_for_app_exit(void);
static void draw_saving_quit_screen(void);
static int pack_is_installed(void);
static int pack_resources_install_blocking(void);
static void pex_menu_music_start_once(void);
static void pex_menu_music_stop(void);
static void pex_game_music_reset_delay(int ticks);
static void pex_game_music_tick(void);
static int pack_missing_required_textures(void);
static int classic_sounds_installed(void);
static void pex_sound_rescan(void);
static void pex_sound_play(const char *key, float volume, float pitch);
static void pex_sound_play_at(const char *key, float x, float y, float z, float volume, float pitch);
static void pex_sound_tick_record_stream(void);
static void pex_sound_stop_record(void);
static void pex_sound_stop_world_audio(void);
static void pex_sound_shutdown(void);
static int classic_resources_need_update(void);


#ifndef CLASSIC_SIZE_UNKNOWN
#define CLASSIC_SIZE_UNKNOWN 0
#define CLASSIC_SIZE_FETCHING 1
#define CLASSIC_SIZE_READY 2
#define CLASSIC_SIZE_ERROR 3
#endif

#ifndef LEGACY_ASSET_LANG
#define LEGACY_ASSET_LANG       0x0020
#define LEGACY_ASSET_OTHER      0x0040
#define LEGACY_ASSET_ALL        (CLASSIC_AUDIO_ALL | LEGACY_ASSET_LANG | LEGACY_ASSET_OTHER)
#define LEGACY_ASSET_THREADS    24
#endif

static void legacy_assets_start_index_fetch(void);
static void legacy_assets_refresh_index(void);
static int legacy_assets_index_ready(void);
static int legacy_assets_index_state(void);
static int legacy_assets_download_mask(void);
static void legacy_assets_index_error_line(char *out, size_t cap);
static void legacy_assets_status_line(char *out, size_t cap);
static int legacy_assets_progress_percent(void);
static int legacy_assets_any_missing(void);
static int legacy_asset_group_missing(int category);
static void legacy_asset_group_summary(int category, char *out, size_t cap);
static void legacy_asset_button_label(int category, char *out, size_t cap);
static void legacy_asset_button_lines(int category, char *line1, size_t cap1, char *line2, size_t cap2);
static int legacy_asset_group_progress_percent(int category);
static void legacy_assets_start_download(int mask);
static int legacy_assets_is_downloading(void);
static void legacy_assets_request_cancel(void);
static void legacy_assets_tick(void);
static void legacy_assets_progress_line(char *out, size_t cap);

#if !defined(PEX_HAS_LEGACY_ASSET_MANAGER)
/* Legacy asset manager stubs for platforms without the desktop SDL2 runtime HTTP asset UI. */
static void legacy_assets_start_index_fetch(void) {}
static void legacy_assets_refresh_index(void) {}
static int legacy_assets_index_ready(void) { return 0; }
static int legacy_assets_index_state(void) { return CLASSIC_SIZE_ERROR; }
static int legacy_assets_download_mask(void) { return 0; }
static void legacy_assets_index_error_line(char *out, size_t cap) { if (out && cap) snprintf(out, cap, "Asset downloads unavailable"); }
static void legacy_assets_status_line(char *out, size_t cap) { if (out && cap) snprintf(out, cap, "Asset downloads unavailable"); }
static int legacy_assets_progress_percent(void) { return 0; }
static int legacy_assets_any_missing(void) { return 0; }
static int legacy_asset_group_missing(int category) { (void)category; return 0; }
static void legacy_asset_group_summary(int category, char *out, size_t cap) { (void)category; if (out && cap) snprintf(out, cap, "Legacy asset downloads are not available on this platform."); }
static void legacy_asset_button_label(int category, char *out, size_t cap) { (void)category; if (out && cap) snprintf(out, cap, "Unavailable"); }
static void legacy_asset_button_lines(int category, char *line1, size_t cap1, char *line2, size_t cap2) { (void)category; if (line1 && cap1) snprintf(line1, cap1, "Unavailable"); if (line2 && cap2) line2[0] = 0; }
static int legacy_asset_group_progress_percent(int category) { (void)category; return 0; }
static void legacy_assets_start_download(int mask) { (void)mask; }
static int legacy_assets_is_downloading(void) { return 0; }
static void legacy_assets_request_cancel(void) {}
static void legacy_assets_tick(void) {}
static void legacy_assets_progress_line(char *out, size_t cap) { if (out && cap) out[0] = '\0'; }
#endif

static int should_show_pack_download_prompt(void);
static void classic_resource_missing_summary(char *out, size_t cap);
static void pack_install_start(void);
static void pack_install_tick(void);
static void enter_world_from_job(void);
static void pex_stats_world_left(void);
static void pex_stats_flush(void);
static void ingame_tick(void);
static void ingame_tick_async_queue(void);
static void ingame_pump_async_tick(void);
static float ingame_tick_async_render_partial(float fallback_partial);
static void player_render_begin_frame(void);
static void ingame_tick_async_request_stop(void);
static void ingame_tick_async_shutdown(void);
static int ingame_tick_async_pending_count(void);
static int ingame_tick_async_busy(void);
static int ingame_tick_async_dropped_count(void);
static double ingame_tick_async_last_ms(void);
static double ingame_tick_async_avg_ms(void);
static void hud_add_chat(const char *msg);
static void reset_flat_player_spawn(void);
static void start_air_swing_once(void);
static void set_mouse_grabbed(int grabbed);
static void apply_vsync_setting(void);
static void set_fullscreen_enabled(int enabled);
static void toggle_fullscreen(void);
static void handle_grabbed_mouse_move(int px, int py);
static void flat_world_reset_blocks(void);
static void flat_world_unload_state(void);
static void flat_center_origin_near(float px, float pz);
static void flat_generate_origin_blocks(void);
static void flat_generate_portal_travel_blocks(float px, float pz);
static void flat_relight_chunks_near(float px, float pz, int radius_chunks);
static void stivufine_update_water_opacity(void);
static void flat_prepare_initial_generation(void);
static void flat_begin_initial_generation(void);
static void flat_continue_initial_generation(void);
static int flat_initial_generation_active(void);
static int flat_initial_generation_total(void);
static int flat_initial_generation_done(void);
static void flat_begin_initial_light_settle(void);
static int flat_initial_light_settle_done(void);
static int flat_initial_light_settle_progress(void);
static void flat_finish_initial_generation(void);
static void flat_mark_all_chunks_dirty(void);
static void worldgen_mesh_prep_reset(void);
static int worldgen_mesh_prep_step(int max_rebuilds);
static void flat_cpu_mesh_remap_shift(int old_origin_x, int old_origin_z, int new_origin_x, int new_origin_z);
static int worldgen_mesh_prep_total(void);
static int worldgen_mesh_prep_done(void);
static void inventory_reset(void);
static void inventory_tick(void);
static void inventory_drop_selected_one(void);
static void inventory_mouse_click(int mx, int my, int button);
static void creative_mouse_click(int mx, int my, int button);
static void creative_mouse_drag(int my);
static void creative_scroll_by(int rows);
static void creative_scroll_page(int delta);
static ItemStack inventory_crafting_output(void);
static ItemStack *chest_get_open_slot_ptr(int local_slot);
static int chest_open_slot_count(void);
static int chest_can_place_at(int x, int y, int z);
static void chest_on_block_placed(int x, int y, int z);
static int chest_open_at(int x, int y, int z);
static void chest_drop_contents(int x, int y, int z);
static int chest_slots_have_items(const ItemStack *slots, int n);
static void chest_clear_all_tiles(void);
static void chest_close_open_inventory(void);
static FurnaceTile *furnace_open_tile(void);
static ItemStack *furnace_get_slot_ptr(int local_slot);
static int furnace_open_at(int x, int y, int z);
static void furnace_on_block_placed(int x, int y, int z);
static void furnace_drop_contents(int x, int y, int z);
static void furnace_clear_all_tiles(void);
static void furnace_close_open_inventory(void);
static void furnace_tick_all(void);
static int furnace_burn_scaled(int pixels);
static int furnace_cook_scaled(int pixels);
static void world_left_mouse_released(void);
static void ingame_right_click(void);
static int ingame_has_context_use_target(void);
static int passive_mobs_player_can_interact(void);
static int pex_xinput_hud_active(void);
static int pex_xinput_hud_bottom_offset(void);
static void ingame_right_release(void);
static float frand01(void);
static void spawn_block_destroy_particles(int bx, int by, int bz, int block_id);
static void spawn_block_hit_particle(int bx, int by, int bz, int face, int block_id);
static void spawn_water_entry_particles(float x, float y, float z, float mx, float mz);
static void add_crit_particle(float x, float y, float z, float mx, float my, float mz);
static void add_smoke_particle(float x, float y, float z, float mx, float my, float mz, float size);
static void add_flame_particle(float x, float y, float z, float mx, float my, float mz);
static void add_explode_particle(float x, float y, float z, float mx, float my, float mz);
static void update_portal_ambient_effects(void);
static void update_dig_particles(void);
static void draw_item_stack_gui(const ItemStack *st, int x, int y);
static void draw_item_stack_gui_animated(const ItemStack *st, int x, int y);
static const char *item_stack_display_name(const ItemStack *st);
static void draw_carried_stack(void);
static void draw_creative_screen(void);
static void update_breaking(void);
static float player_fov_multiplier_125(void);
static int player_is_using_bow_125(void);
static int player_bow_use_duration_125(void);
static void update_bow_item_use_tick(void);
static void update_dropped_items(void);
static void update_equipped_item(void);
static int flat_player_aabb_collides(float px, float py, float pz);
static int flat_player_has_sneak_support(float px, float py, float pz);
static int flat_player_has_water_exit_ledge(float px, float py, float pz, float dx, float dz);
static int flat_block_intersects_player(int bx, int by, int bz);
static float pex_rand_float01(void);
static void trigger_hand_swing(void);
static void restart_hand_swing(void);
static void player_potion_update_tick(void);
static int player_has_potion(int potion_id);
static int player_potion_amplifier(int potion_id);
static const char *pex_potion_effect_display_name(int potion_id, int amplifier);
static void pex_potion_duration_string(int ticks, char *out, size_t out_sz);
static int pex_potion_status_icon_index(int potion_id);
static int pex_potion_tooltip_lines(const ItemStack *st, char out[][96], int max_lines);
static int pex_active_potion_count(void);
static PexMapData *pex_map_find(int id, int create);
static void update_held_map_item_tick(void);
static void update_projectiles(void);
static void update_xp_orbs(void);
static float block_slipperiness_for_item(int id);
static int flat_entity_handle_water_025(float x, float y, float z, float *mx, float *my, float *mz);
static PexDamageSource pex_damage_source_simple(PexDamageType type);
static PexDamageSource pex_damage_source_mob(PassiveMob *mob);
static PexDamageSource pex_damage_source_player(void);
static PexDamageSource pex_damage_source_arrow(const FlatProjectile *projectile);
static PexDamageSource pex_damage_source_fireball(const FlatProjectile *projectile);
static PexDamageSource pex_damage_source_thrown(const FlatProjectile *projectile);
static PexDamageSource pex_damage_source_indirect_magic(const FlatProjectile *projectile);
static const char *pex_damage_source_death_reason(const PexDamageSource *source);
static int player_attack_entity_from(PexDamageSource source, int amount);
static void player_die_from_source(const PexDamageSource *source);
static void player_die(const char *reason);
static void player_respawn(void);
static int flat_player_suffocation_block(void);
static int flat_player_head_block(void);
static int flat_player_head_in_water(void);
static int flat_player_head_in_lava(void);
static void apply_player_fluid_velocity(int is_water);
static void update_falling_blocks(void);
static void passive_mobs_reset(void);
static void passive_mobs_spawn_initial(void);
static void passive_mobs_ensure_population(int wanted, int attempts);
static void update_passive_mobs(void);
static void update_vehicles(void);
static void passive_mobs_apply_riding(void);
static void passive_render_worker_request_stop(void);
static void passive_render_worker_shutdown(void);
static int passive_mobs_attack_from_player(void);
static int passive_mobs_player_interact(void);
static int pex_mob_attack_entity_from(PassiveMob *mob, PexDamageSource source, int damage);
static int pex_dragon_crystal_projectile_hit_125(const FlatProjectile *p, float x0, float y0, float z0, float x1, float y1, float z1, float max_t, float *out_t, float *out_x, float *out_y, float *out_z);
static void pex_passive_apply_potion_effect(PassiveMob *mob, int id, int duration, int amplifier, float splash_scale);
static void draw_java_entity_shadow(float x, float y, float z, float shadow_size, float shadow_alpha);
static void draw_passive_mobs(float partial);
static void draw_vehicles(float partial);
static void draw_projectiles(float partial);
static void draw_xp_orbs(float partial);
static int java125_block_render_type(int id);
static int java125_render_item_in_3d(int render_type);
static void passive_mobs_read_from_file(FILE *f, int version);
static void passive_mobs_write_to_file(FILE *f, const PassiveMob *mobs);
static void add_bubble_particle(float x, float y, float z, float mx, float my, float mz);
static void add_splash_particle(float x, float y, float z, float mx, float my, float mz);
static void update_liquids(void);
static void update_infinite_world_streaming(void);
static void world_stream_service_ensure(void);
static void world_stream_service_request_stop(void);
static void world_stream_service_shutdown(void);
static void world_stream_shared_locks_shutdown(void);
static int world_stream_service_active(void);
static void stream_generation_queue_clear(void);
static int item_icon_tile(int id);
static int item_icon_tile_for_stack(const ItemStack *st);
static int spawn_egg_color(int damage, int layer);
static int java125_item_block_tint(const ItemStack *st);
static void draw_item3d_from_texture(Texture *atlas, int tile);
static int write_level_dat(const char *world_dir, const char *world_name, long long seed, int spawn_x, int spawn_y, int spawn_z, long long size_on_disk);
static int write_session_lock(const char *world_dir);
static void pex_join_save_thread_for_exit(void);
static void save_modified_flat_chunks(void);
static void save_modified_flat_chunks_sync(void);
static void request_chunk_save_async(void);
static void flat_load_chunk_delta(int cx, int cz);
static void flat_mark_all_chunks_modified(void);
static void async_section_mesh_request_stop(void);
static void async_section_mesh_shutdown(void);
static void flat_world_render_resources_release_begin(void);
static int flat_world_render_resources_release_step(int budget);
static int flat_world_render_resources_release_progress(void);
static void flat_world_render_resources_release(void);
static int pex_net_connect_to_server(const char *server);
static void pex_net_poll(void);
static int pex_net_is_connecting(void);
static void pex_net_update_smoothing(void);
static void pex_net_disconnect_request_stop(void);
static void pex_net_disconnect(void);
static void pex_net_release_render_resources(void);
static void pex_mp_cache_save_request_stop(void);
static void pex_mp_cache_save_shutdown(void);
static void pex_net_send_player_state(void);
static void java47_player_network_position(double *x, double *feet_y, double *z);
static void pex_net_send_block_action(int action, int x, int y, int z, int face, int block_id);
static int pex_net_send_block_interact(int x, int y, int z, int face);
static int pex_net_send_use_item_air(void);
static void pex_net_send_release_use_item(void);
static int pex_net_send_inventory_click(int pex_slot, int button, int mode, const ItemStack *clicked_result);
static int pex_net_send_creative_slot(int pex_slot, const ItemStack *stack);
static void pex_net_send_container_close(void);
static void pex_net_send_player_action(int action, int x, int y, int z, int face, int block_id);
static void net_send_action_progress(int action, int x, int y, int z, int face, int block_id, int progress);
static int pex_net_player_ray_distance(float max_dist, float *out_t);
static int pex_net_try_attack_player(void);
static void pex_net_send_drop_item(ItemStack st);
static void pex_net_send_pickup_drop(int drop_id);
static void pex_net_send_chest_open(void);
static void pex_net_send_chest_update(void);
static void pex_net_send_craft_request(void);
static void pex_net_send_chat(const char *text);
static void pex_net_send_skin(void);

/* Dimension and Portal forward declarations */
static void block_on_placed(int x, int y, int z, int id);
static int check_end_portal_complete(int fx, int fy, int fz);
static void dimension_tick_portal_collision(void);
static void dimension_chunk_dir(int dimension, const char *world_dir, char *out, size_t cap);
static void save_world_state_sync(void);
static void flat_mark_all_chunks_dirty(void);


#endif /* MC_B1_SPLIT_COMMON_H */
