#ifndef PEXCRAFT_ANDROID_TV_COMPAT_H
#define PEXCRAFT_ANDROID_TV_COMPAT_H
#ifndef PEXCRAFT_SDL2_COMPAT_H
#define PEXCRAFT_SDL2_COMPAT_H
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>
#include <EGL/egl.h>
#include <jni.h>
#include <android/log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#ifndef PEX_ANDROID_LOG_TAG
#define PEX_ANDROID_LOG_TAG "PexCraftTV"
#endif
#define PEX_ANDROID_LOGI(...) __android_log_print(ANDROID_LOG_INFO, PEX_ANDROID_LOG_TAG, __VA_ARGS__)
#define PEX_ANDROID_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, PEX_ANDROID_LOG_TAG, __VA_ARGS__)


#ifndef GLdouble
typedef double GLdouble;
#endif
#ifndef GLclampd
typedef double GLclampd;
#endif
#ifndef GLvoid
typedef void GLvoid;
#endif
#ifndef GL_PROJECTION
#define GL_PROJECTION 0x1701
#endif
#ifndef GL_MODELVIEW
#define GL_MODELVIEW 0x1700
#endif
#ifndef GL_PROJECTION_MATRIX
#define GL_PROJECTION_MATRIX 0x0BA7
#endif
#ifndef GL_MODELVIEW_MATRIX
#define GL_MODELVIEW_MATRIX 0x0BA6
#endif
#ifndef GL_COMPILE
#define GL_COMPILE 0x1300
#endif
#ifndef GL_VERTEX_ARRAY
#define GL_VERTEX_ARRAY 0x8074
#endif
#ifndef GL_TEXTURE_COORD_ARRAY
#define GL_TEXTURE_COORD_ARRAY 0x8078
#endif
#ifndef GL_COLOR_ARRAY
#define GL_COLOR_ARRAY 0x8076
#endif
#ifndef GL_UNSIGNED_INT
#define GL_UNSIGNED_INT 0x1405
#endif
#ifndef GL_LINE_LOOP
#define GL_LINE_LOOP 0x0002
#endif
#ifndef GL_LINE_STRIP
#define GL_LINE_STRIP 0x0003
#endif
#ifndef GL_TRIANGLE_FAN
#define GL_TRIANGLE_FAN 0x0006
#endif
#ifndef GL_TRIANGLE_STRIP
#define GL_TRIANGLE_STRIP 0x0005
#endif

#ifndef GL_CLAMP
#define GL_CLAMP 0x2900
#endif
#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif
#ifndef GL_ALPHA_TEST
#define GL_ALPHA_TEST 0x0BC0
#endif
#ifndef GL_FOG
#define GL_FOG 0x0B60
#endif
#ifndef GL_FOG_MODE
#define GL_FOG_MODE 0x0B65
#endif
#ifndef GL_FOG_START
#define GL_FOG_START 0x0B63
#endif
#ifndef GL_FOG_END
#define GL_FOG_END 0x0B64
#endif
#ifndef GL_FOG_COLOR
#define GL_FOG_COLOR 0x0B66
#endif
#ifndef GL_FOG_DENSITY
#define GL_FOG_DENSITY 0x0B62
#endif
#ifndef GL_EXP
#define GL_EXP 0x0800
#endif

/* Minimal Win32-shaped compatibility types used by the old code. */
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef int BOOL;
typedef unsigned int UINT;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef intptr_t LRESULT;
typedef void *HINSTANCE;
typedef void *HDC;
typedef SDL_GLContext HGLRC;
typedef SDL_Window HWND;
typedef struct RECT { long left, top, right, bottom; } RECT;
typedef struct POINT { long x, y; } POINT;
typedef const char *LPCSTR;
typedef void *LPVOID;
typedef DWORD *LPDWORD;
typedef uintptr_t DWORD_PTR;
typedef size_t SIZE_T;
typedef void *HMODULE;
typedef char WCHAR;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#ifndef WINAPI
#define WINAPI
#endif
#ifndef CALLBACK
#define CALLBACK
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)0xffffffffu)
#endif
#ifndef FILE_ATTRIBUTE_DIRECTORY
#define FILE_ATTRIBUTE_DIRECTORY 0x10u
#endif
#ifndef FILE_ATTRIBUTE_NORMAL
#define FILE_ATTRIBUTE_NORMAL 0x80u
#endif
#ifndef GENERIC_READ
#define GENERIC_READ 0x80000000u
#endif
#ifndef CREATE_NO_WINDOW
#define CREATE_NO_WINDOW 0
#endif
#ifndef INFINITE
#define INFINITE 0xffffffffu
#endif
#ifndef WAIT_OBJECT_0
#define WAIT_OBJECT_0 0
#endif
#ifndef WAIT_TIMEOUT
#define WAIT_TIMEOUT 258
#endif
#ifndef THREAD_PRIORITY_NORMAL
#define THREAD_PRIORITY_NORMAL 0
#endif
#ifndef THREAD_PRIORITY_BELOW_NORMAL
#define THREAD_PRIORITY_BELOW_NORMAL -1
#endif
#ifndef THREAD_PRIORITY_ABOVE_NORMAL
#define THREAD_PRIORITY_ABOVE_NORMAL 1
#endif
#ifndef SW_SHOWNORMAL
#define SW_SHOWNORMAL 1
#endif

/* Virtual-key constants kept so options.txt and input code remain compatible. */
#define VK_LBUTTON 0x01
#define VK_RBUTTON 0x02
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_F3 0x72
#define VK_F5 0x74
#define VK_F11 0x7A
#define MAPVK_VK_TO_VSC 0

/* Socket compatibility. */
typedef int SOCKET;
typedef struct WSADATA { int unused; } WSADATA;
#ifndef u_long
typedef unsigned long u_long;
#endif
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define WSAEWOULDBLOCK EWOULDBLOCK
#define WSAEINPROGRESS EINPROGRESS
#define WSAEALREADY EALREADY
#define WSAEACCES EACCES
#define IPPROTO_TCP IPPROTO_TCP
#define closesocket close
static inline int WSAGetLastError(void) { return errno; }
static inline int WSAStartup(WORD ver, void *data) { (void)ver; (void)data; return 0; }
static inline int WSACleanup(void) { return 0; }
static inline unsigned long PEX_MAKEWORD(int a, int b) { return ((unsigned long)((a) & 0xff) | ((unsigned long)((b) & 0xff) << 8)); }
#define MAKEWORD(a,b) PEX_MAKEWORD((a),(b))
static inline int ioctlsocket(SOCKET s, long cmd, unsigned long *argp) { return ioctl(s, (unsigned long)cmd, argp); }

/* Path normalization: old code builds Windows paths with '\\'. */
static inline void pex_normalize_path(char *dst, size_t cap, const char *src) {
    if (!dst || cap == 0) return;
    if (!src) { dst[0] = 0; return; }
    size_t i = 0;
    for (; i + 1 < cap && src[i]; ++i) dst[i] = (src[i] == '\\') ? '/' : src[i];
    dst[i] = 0;
}

static inline FILE *pex_fopen_portable(const char *path, const char *mode) {
    char tmp[4096];
    pex_normalize_path(tmp, sizeof(tmp), path);
    return fopen(tmp, mode);
}
#define fopen pex_fopen_portable

static inline DWORD GetFileAttributesA(const char *path) {
    char tmp[4096]; struct stat st;
    pex_normalize_path(tmp, sizeof(tmp), path);
    if (stat(tmp, &st) != 0) return INVALID_FILE_ATTRIBUTES;
    return S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
}
static inline BOOL CreateDirectoryA(const char *path, void *security) {
    (void)security; char tmp[4096]; pex_normalize_path(tmp, sizeof(tmp), path);
    if (mkdir(tmp, 0775) == 0 || errno == EEXIST) return TRUE;
    return FALSE;
}
static inline BOOL DeleteFileA(const char *path) {
    char tmp[4096]; pex_normalize_path(tmp, sizeof(tmp), path); return unlink(tmp) == 0;
}
static inline BOOL RemoveDirectoryA(const char *path) {
    char tmp[4096]; pex_normalize_path(tmp, sizeof(tmp), path); return rmdir(tmp) == 0;
}
static inline BOOL SetFileAttributesA(const char *path, DWORD attr) { (void)path; (void)attr; return TRUE; }
static inline BOOL CopyFileA(const char *src, const char *dst, BOOL fail_if_exists) {
    char s[4096], d[4096]; pex_normalize_path(s, sizeof(s), src); pex_normalize_path(d, sizeof(d), dst);
    if (fail_if_exists && access(d, F_OK) == 0) return FALSE;
    FILE *in = fopen(s, "rb"); if (!in) return FALSE;
    FILE *out = fopen(d, "wb"); if (!out) { fclose(in); return FALSE; }
    char buf[16384]; size_t n; int ok = 1;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) if (fwrite(buf, 1, n, out) != n) { ok = 0; break; }
    fclose(out); fclose(in); return ok ? TRUE : FALSE;
}
static inline char *lstrcpynA(char *dst, const char *src, int cap) {
    if (!dst || cap <= 0) return dst;
    snprintf(dst, (size_t)cap, "%s", src ? src : "");
    return dst;
}

/* Directory enumeration for the simple path-star patterns used by this project. */
typedef struct WIN32_FIND_DATAA {
    DWORD dwFileAttributes;
    char cFileName[260];
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
} WIN32_FIND_DATAA;

typedef enum PexHandleKind { PEX_HANDLE_NONE, PEX_HANDLE_DIR, PEX_HANDLE_THREAD, PEX_HANDLE_EVENT } PexHandleKind;
typedef struct PexHandle {
    PexHandleKind kind;
    DIR *dir;
    char dir_path[4096];
    pthread_t thread;
    int thread_joined;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int manual_reset;
    int signaled;
} *HANDLE;
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)

static inline int pex_fill_find_data(HANDLE h, WIN32_FIND_DATAA *fd) {
    if (!h || h == INVALID_HANDLE_VALUE || h->kind != PEX_HANDLE_DIR || !fd) return 0;
    struct dirent *de;
    while ((de = readdir(h->dir)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        memset(fd, 0, sizeof(*fd));
        snprintf(fd->cFileName, sizeof(fd->cFileName), "%s", de->d_name);
        char full[4096]; snprintf(full, sizeof(full), "%s/%s", h->dir_path, de->d_name);
        struct stat st; if (stat(full, &st) == 0) {
            if (S_ISDIR(st.st_mode)) fd->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
            unsigned long long sz = (unsigned long long)st.st_size;
            fd->nFileSizeLow = (DWORD)(sz & 0xffffffffu);
            fd->nFileSizeHigh = (DWORD)(sz >> 32);
        }
        return 1;
    }
    return 0;
}
static inline HANDLE FindFirstFileA(const char *pattern, WIN32_FIND_DATAA *fd) {
    char p[4096], dirp[4096]; pex_normalize_path(p, sizeof(p), pattern);
    snprintf(dirp, sizeof(dirp), "%s", p);
    char *slash = strrchr(dirp, '/');
    if (slash) *slash = 0; else snprintf(dirp, sizeof(dirp), ".");
    DIR *dir = opendir(dirp); if (!dir) return INVALID_HANDLE_VALUE;
    HANDLE h = (HANDLE)calloc(1, sizeof(*h)); if (!h) { closedir(dir); return INVALID_HANDLE_VALUE; }
    h->kind = PEX_HANDLE_DIR; h->dir = dir; snprintf(h->dir_path, sizeof(h->dir_path), "%s", dirp);
    if (!pex_fill_find_data(h, fd)) { closedir(dir); free(h); return INVALID_HANDLE_VALUE; }
    return h;
}
static inline BOOL FindNextFileA(HANDLE h, WIN32_FIND_DATAA *fd) { return pex_fill_find_data(h, fd) ? TRUE : FALSE; }
static inline BOOL FindClose(HANDLE h);

/* Threads/events/critical sections. */
typedef pthread_mutex_t CRITICAL_SECTION;
static inline void InitializeCriticalSection(CRITICAL_SECTION *cs) { pthread_mutex_init(cs, NULL); }
static inline void DeleteCriticalSection(CRITICAL_SECTION *cs) { pthread_mutex_destroy(cs); }
static inline void EnterCriticalSection(CRITICAL_SECTION *cs) { pthread_mutex_lock(cs); }
static inline void LeaveCriticalSection(CRITICAL_SECTION *cs) { pthread_mutex_unlock(cs); }

typedef DWORD (WINAPI *LPTHREAD_START_ROUTINE)(LPVOID);
typedef struct PexThreadStart { LPTHREAD_START_ROUTINE fn; LPVOID arg; } PexThreadStart;
static inline void *pex_thread_trampoline(void *p) {
    PexThreadStart *ts = (PexThreadStart*)p;
    LPTHREAD_START_ROUTINE fn = ts->fn; LPVOID arg = ts->arg; free(ts);
    return (void*)(uintptr_t)fn(arg);
}
static inline HANDLE CreateThread(void *sa, size_t stack, LPTHREAD_START_ROUTINE fn, LPVOID arg, DWORD flags, DWORD *tid) {
    (void)sa; (void)flags; if (tid) *tid = 0;
    HANDLE h = (HANDLE)calloc(1, sizeof(*h)); if (!h) return NULL;
    PexThreadStart *ts = (PexThreadStart*)malloc(sizeof(*ts)); if (!ts) { free(h); return NULL; }
    ts->fn = fn; ts->arg = arg; h->kind = PEX_HANDLE_THREAD;
    pthread_attr_t attr; pthread_attr_t *attrp = NULL;
    if (stack > 0 && pthread_attr_init(&attr) == 0) {
        size_t min_stack = stack < 262144u ? 262144u : stack;
        if (pthread_attr_setstacksize(&attr, min_stack) == 0) attrp = &attr;
    }
    int pr = pthread_create(&h->thread, attrp, pex_thread_trampoline, ts);
    if (attrp) pthread_attr_destroy(attrp);
    if (pr != 0) { free(ts); free(h); return NULL; }
    return h;
}
static inline HANDLE CreateEventA(void *sa, BOOL manual_reset, BOOL initial_state, const char *name) {
    (void)sa; (void)name; HANDLE h = (HANDLE)calloc(1, sizeof(*h)); if (!h) return NULL;
    h->kind = PEX_HANDLE_EVENT; h->manual_reset = manual_reset ? 1 : 0; h->signaled = initial_state ? 1 : 0;
    pthread_mutex_init(&h->mutex, NULL); pthread_cond_init(&h->cond, NULL); return h;
}
static inline BOOL SetEvent(HANDLE h) {
    if (!h || h->kind != PEX_HANDLE_EVENT) return FALSE;
    pthread_mutex_lock(&h->mutex); h->signaled = 1; pthread_cond_broadcast(&h->cond); pthread_mutex_unlock(&h->mutex); return TRUE;
}
static inline DWORD WaitForSingleObject(HANDLE h, DWORD ms) {
    if (!h || h == INVALID_HANDLE_VALUE) return WAIT_TIMEOUT;
    if (h->kind == PEX_HANDLE_THREAD) {
        if (ms == 0) return h->thread_joined ? WAIT_OBJECT_0 : WAIT_TIMEOUT;
        if (!h->thread_joined) { pthread_join(h->thread, NULL); h->thread_joined = 1; }
        return WAIT_OBJECT_0;
    }
    if (h->kind == PEX_HANDLE_EVENT) {
        pthread_mutex_lock(&h->mutex);
        if (!h->signaled && ms == 0) { pthread_mutex_unlock(&h->mutex); return WAIT_TIMEOUT; }
        while (!h->signaled) pthread_cond_wait(&h->cond, &h->mutex);
        if (!h->manual_reset) h->signaled = 0;
        pthread_mutex_unlock(&h->mutex); return WAIT_OBJECT_0;
    }
    return WAIT_TIMEOUT;
}
static inline BOOL CloseHandle(HANDLE h) {
    if (!h || h == INVALID_HANDLE_VALUE) return FALSE;
    if (h->kind == PEX_HANDLE_DIR && h->dir) closedir(h->dir);
    else if (h->kind == PEX_HANDLE_THREAD && !h->thread_joined) { pthread_detach(h->thread); }
    else if (h->kind == PEX_HANDLE_EVENT) { pthread_cond_destroy(&h->cond); pthread_mutex_destroy(&h->mutex); }
    free(h); return TRUE;
}
static inline BOOL FindClose(HANDLE h) { return CloseHandle(h); }
static inline BOOL SetThreadPriority(HANDLE h, int prio) { (void)h; (void)prio; return TRUE; }

static inline LONG InterlockedExchange(volatile LONG *target, LONG value) { return __sync_lock_test_and_set(target, value); }
static inline LONG InterlockedCompareExchange(volatile LONG *target, LONG exchange, LONG comparand) { return __sync_val_compare_and_swap(target, comparand, exchange); }
static inline void Sleep(DWORD ms) { SDL_Delay(ms); }


static inline void ExitProcess(UINT code) { exit((int)code); }
static inline BOOL MoveFileA(const char *src, const char *dst) { char s[4096], d[4096]; pex_normalize_path(s, sizeof(s), src); pex_normalize_path(d, sizeof(d), dst); return rename(s, d) == 0; }
static inline DWORD GetTickCount(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (DWORD)((ts.tv_sec * 1000ull + ts.tv_nsec / 1000000ull) & 0xffffffffu); }
static inline HMODULE LoadLibraryA(const char *name) { (void)name; return NULL; }
static inline void *GetProcAddress(HMODULE h, const char *name) { (void)h; (void)name; return NULL; }
static inline BOOL FreeLibrary(HMODULE h) { (void)h; return TRUE; }
static inline HANDLE GetCurrentProcess(void) { return NULL; }

static inline UINT MapVirtualKeyA(UINT code, UINT maptype) { (void)maptype; return code; }
static inline int GetKeyNameTextA(LONG lparam, char *buf, int cap) { (void)lparam; if (buf && cap > 0) buf[0] = 0; return 0; }
static inline DWORD GetModuleFileNameA(void *module, char *buf, DWORD cap) { (void)module; if (buf && cap) { snprintf(buf, cap, "pexcraft_sdl2"); return (DWORD)strlen(buf); } return 0; }
static inline void *ShellExecuteA(void *hwnd, const char *op, const char *file, const char *params, const char *dir, int show) { (void)hwnd; (void)op; (void)file; (void)params; (void)dir; (void)show; return NULL; }
static inline void PostQuitMessage(int code) { (void)code; SDL_Event e; memset(&e, 0, sizeof(e)); e.type = SDL_QUIT; SDL_PushEvent(&e); }

#endif /* PEXCRAFT_SDL2_COMPAT_H */
