#ifndef PEXCRAFT_PSP_COMPAT_H
#define PEXCRAFT_PSP_COMPAT_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspiofilemgr.h>
#include <psputils.h>
#include <psprtc.h>
#include <psppower.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <limits.h>

/* OpenGL-ish constants/types consumed by the shared renderer code.  The PSP
   backend below implements the small fixed-function subset PEXCraft uses. */
typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef float GLfloat;
typedef double GLdouble;
typedef unsigned char GLubyte;
typedef unsigned int GLbitfield;
typedef unsigned char GLboolean;

#define GL_FALSE 0
#define GL_TRUE 1
#define GL_POINTS 0x0000
#define GL_LINES 0x0001
#define GL_LINE_STRIP 0x0003
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006
#define GL_QUADS 0x0007
#define GL_COMPILE 0x1300
#define GL_MODELVIEW 0x1700
#define GL_PROJECTION 0x1701
#define GL_TEXTURE_2D 0x0DE1
#define GL_BLEND 0x0BE2
#define GL_DEPTH_TEST 0x0B71
#define GL_ALPHA_TEST 0x0BC0
#define GL_CULL_FACE 0x0B44
#define GL_FOG 0x0B60
#define GL_DITHER 0x0BD0
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_DST_COLOR 0x0306
#define GL_SRC_COLOR 0x0300
#define GL_ONE 1
#define GL_ZERO 0
#define GL_LEQUAL 0x0203
#define GL_LESS 0x0201
#define GL_GEQUAL 0x0206
#define GL_GREATER 0x0204
#define GL_ALWAYS 0x0207
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_REPEAT 0x2901
#define GL_CLAMP 0x2900
#define GL_RGBA 0x1908
#define GL_UNSIGNED_BYTE 0x1401
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_VIEWPORT 0x0BA2
#define GL_FOG_MODE 0x0B65
#define GL_FOG_START 0x0B63
#define GL_FOG_END 0x0B64
#define GL_FOG_COLOR 0x0B66
#define GL_MODELVIEW_MATRIX 0x0BA6
#define GL_PROJECTION_MATRIX 0x0BA7
#define GL_TEXTURE_ENV 0x2300
#define GL_TEXTURE_ENV_MODE 0x2200
#define GL_MODULATE 0x2100
#define GL_ALPHA 0x1906
#define GL_GREATER 0x0204

/* Minimal Win32-shaped compatibility types used by shared code. */
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
typedef void *HGLRC;
typedef void *HWND;
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
#ifndef INFINITE
#define INFINITE 0xffffffffu
#endif
#ifndef WAIT_OBJECT_0
#define WAIT_OBJECT_0 0
#endif
#ifndef WAIT_TIMEOUT
#define WAIT_TIMEOUT 258
#endif
#ifndef SW_SHOWNORMAL
#define SW_SHOWNORMAL 1
#endif
#ifndef THREAD_PRIORITY_NORMAL
#define THREAD_PRIORITY_NORMAL 0
#endif
#ifndef THREAD_PRIORITY_BELOW_NORMAL
#define THREAD_PRIORITY_BELOW_NORMAL (-1)
#endif

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

/* PSP build is memory-card optional: gameplay/options/world state stay in RAM.
   The EBOOT contains its required assets, so we avoid creating ms0:/ folders/files. */
#ifndef PEX_PSP_MEMORY_ONLY
#define PEX_PSP_MEMORY_ONLY 1
#endif

/* Networking is intentionally disabled for the PSP port for now. */
typedef int SOCKET;
typedef struct WSADATA { int unused; } WSADATA;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define WSAEWOULDBLOCK EWOULDBLOCK
#define WSAEINPROGRESS EINPROGRESS
#define WSAEALREADY EALREADY
#define WSAEACCES EACCES
#define closesocket(s) ((void)(s),0)
static inline int WSAGetLastError(void) { return errno; }
static inline int WSAStartup(WORD ver, void *data) { (void)ver; (void)data; return 1; }
static inline int WSACleanup(void) { return 0; }
static inline unsigned long PEX_MAKEWORD(int a, int b) { return ((unsigned long)((a) & 0xff) | ((unsigned long)((b) & 0xff) << 8)); }
#define MAKEWORD(a,b) PEX_MAKEWORD((a),(b))
static inline int ioctlsocket(SOCKET s, long cmd, unsigned long *argp) { (void)s; (void)cmd; (void)argp; return -1; }

static inline void pex_normalize_path(char *dst, size_t cap, const char *src) {
    if (!dst || cap == 0) return;
    if (!src) { dst[0] = 0; return; }
    size_t i = 0;
    for (; i + 1 < cap && src[i]; ++i) dst[i] = (src[i] == '\\') ? '/' : src[i];
    dst[i] = 0;
}
static inline int pex_file_mode_writes(const char *mode) {
    if (!mode) return 0;
    return strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+');
}
static inline int pex_path_is_memstick(const char *path) {
    if (!path) return 0;
    return !strncasecmp(path, "ms0:", 4) || !strncasecmp(path, "ef0:", 4) ||
           !strncasecmp(path, "disc0:", 6) || !strncasecmp(path, "memory:", 7);
}
static inline FILE *pex_fopen_portable(const char *path, const char *mode) {
#if PEX_PSP_MEMORY_ONLY
    /* No runtime filesystem dependency on PSP.  Assets/options/world data are
       memory-backed; writes are ignored and memstick reads are treated absent. */
    if (pex_file_mode_writes(mode) || pex_path_is_memstick(path)) return NULL;
#endif
    char tmp[1024]; pex_normalize_path(tmp, sizeof(tmp), path); return fopen(tmp, mode);
}
#define fopen pex_fopen_portable

static inline DWORD GetFileAttributesA(const char *path) {
#if PEX_PSP_MEMORY_ONLY
    if (pex_path_is_memstick(path)) return INVALID_FILE_ATTRIBUTES;
#endif
    char tmp[1024]; struct stat st; pex_normalize_path(tmp, sizeof(tmp), path);
    if (stat(tmp, &st) != 0) return INVALID_FILE_ATTRIBUTES;
    return S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
}
static inline BOOL CreateDirectoryA(const char *path, void *security) {
    (void)security;
#if PEX_PSP_MEMORY_ONLY
    (void)path; return TRUE;
#else
    char tmp[1024]; pex_normalize_path(tmp, sizeof(tmp), path);
    if (mkdir(tmp, 0775) == 0 || errno == EEXIST) return TRUE; return FALSE;
#endif
}
static inline BOOL DeleteFileA(const char *path) {
#if PEX_PSP_MEMORY_ONLY
    (void)path; return TRUE;
#else
    char tmp[1024]; pex_normalize_path(tmp, sizeof(tmp), path); return remove(tmp) == 0;
#endif
}
static inline BOOL RemoveDirectoryA(const char *path) {
#if PEX_PSP_MEMORY_ONLY
    (void)path; return TRUE;
#else
    char tmp[1024]; pex_normalize_path(tmp, sizeof(tmp), path); return rmdir(tmp) == 0;
#endif
}
static inline BOOL SetFileAttributesA(const char *path, DWORD attr) { (void)path; (void)attr; return TRUE; }
static inline BOOL CopyFileA(const char *src, const char *dst, BOOL fail_if_exists) {
#if PEX_PSP_MEMORY_ONLY
    (void)src; (void)dst; (void)fail_if_exists; return FALSE;
#else
    char s[1024], d[1024]; pex_normalize_path(s, sizeof(s), src); pex_normalize_path(d, sizeof(d), dst);
    if (fail_if_exists && access(d, F_OK) == 0) return FALSE;
    FILE *in = fopen(s, "rb"); if (!in) return FALSE;
    FILE *out = fopen(d, "wb"); if (!out) { fclose(in); return FALSE; }
    char buf[8192]; size_t n; int ok = 1;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) if (fwrite(buf, 1, n, out) != n) { ok = 0; break; }
    fclose(out); fclose(in); return ok ? TRUE : FALSE;
#endif
}
static inline char *lstrcpynA(char *dst, const char *src, int cap) { if (!dst || cap <= 0) return dst; snprintf(dst, (size_t)cap, "%s", src ? src : ""); return dst; }
static inline int lstrcmpA(const char *a, const char *b) {
    if (!a) a = "";
    if (!b) b = "";
    return strcmp(a, b);
}
static inline int lstrcmpiA(const char *a, const char *b) {
    if (!a) a = "";
    if (!b) b = "";
    return strcasecmp(a, b);
}

typedef struct WIN32_FIND_DATAA { DWORD dwFileAttributes; char cFileName[260]; DWORD nFileSizeHigh; DWORD nFileSizeLow; } WIN32_FIND_DATAA;
typedef enum PexHandleKind { PEX_HANDLE_NONE, PEX_HANDLE_DIR, PEX_HANDLE_THREAD, PEX_HANDLE_EVENT } PexHandleKind;
typedef struct PexHandle { PexHandleKind kind; DIR *dir; char dir_path[1024]; SceUID uid; int signaled; int manual_reset; } *HANDLE;
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
static inline int pex_fill_find_data(HANDLE h, WIN32_FIND_DATAA *fd) {
    if (!h || h == INVALID_HANDLE_VALUE || h->kind != PEX_HANDLE_DIR || !fd) return 0;
    struct dirent *de; while ((de = readdir(h->dir)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        memset(fd, 0, sizeof(*fd)); snprintf(fd->cFileName, sizeof(fd->cFileName), "%s", de->d_name);
        char full[1024]; snprintf(full, sizeof(full), "%s/%s", h->dir_path, de->d_name);
        struct stat st; if (stat(full, &st) == 0) { if (S_ISDIR(st.st_mode)) fd->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY; unsigned long long sz=(unsigned long long)st.st_size; fd->nFileSizeLow=(DWORD)sz; fd->nFileSizeHigh=(DWORD)(sz>>32); }
        return 1;
    }
    return 0;
}
static inline HANDLE FindFirstFileA(const char *pattern, WIN32_FIND_DATAA *fd) {
#if PEX_PSP_MEMORY_ONLY
    (void)fd;
    if (pex_path_is_memstick(pattern)) return INVALID_HANDLE_VALUE;
#endif
    char p[1024], dirp[1024]; pex_normalize_path(p, sizeof(p), pattern); snprintf(dirp, sizeof(dirp), "%s", p);
    char *slash = strrchr(dirp, '/'); if (slash) *slash = 0; else snprintf(dirp, sizeof(dirp), ".");
    DIR *dir = opendir(dirp); if (!dir) return INVALID_HANDLE_VALUE;
    HANDLE h = (HANDLE)calloc(1, sizeof(*h)); if (!h) { closedir(dir); return INVALID_HANDLE_VALUE; }
    h->kind = PEX_HANDLE_DIR; h->dir = dir; snprintf(h->dir_path, sizeof(h->dir_path), "%s", dirp);
    if (!pex_fill_find_data(h, fd)) { closedir(dir); free(h); return INVALID_HANDLE_VALUE; }
    return h;
}
static inline BOOL FindNextFileA(HANDLE h, WIN32_FIND_DATAA *fd) { return pex_fill_find_data(h, fd) ? TRUE : FALSE; }
static inline BOOL FindClose(HANDLE h);

typedef SceUID CRITICAL_SECTION;
static inline void InitializeCriticalSection(CRITICAL_SECTION *cs) { *cs = sceKernelCreateSema("pxc_cs", 0, 1, 1, NULL); }
static inline void DeleteCriticalSection(CRITICAL_SECTION *cs) { if (cs && *cs >= 0) sceKernelDeleteSema(*cs); }
static inline void EnterCriticalSection(CRITICAL_SECTION *cs) { if (cs && *cs >= 0) sceKernelWaitSema(*cs, 1, NULL); }
static inline void LeaveCriticalSection(CRITICAL_SECTION *cs) { if (cs && *cs >= 0) sceKernelSignalSema(*cs, 1); }

typedef DWORD (WINAPI *LPTHREAD_START_ROUTINE)(LPVOID);
typedef struct PexThreadStart { LPTHREAD_START_ROUTINE fn; LPVOID arg; } PexThreadStart;
static int pex_thread_trampoline(SceSize args, void *argp) { (void)args; PexThreadStart *ts=(PexThreadStart*)argp; DWORD r=ts->fn(ts->arg); free(ts); sceKernelExitThread((int)r); return (int)r; }
static inline HANDLE CreateThread(void *sa, size_t stack, LPTHREAD_START_ROUTINE fn, LPVOID arg, DWORD flags, DWORD *tid) {
    (void)sa; (void)flags; if (tid) *tid = 0;
    HANDLE h=(HANDLE)calloc(1,sizeof(*h)); PexThreadStart *ts=(PexThreadStart*)malloc(sizeof(*ts)); if(!h||!ts){free(h);free(ts);return NULL;}
    ts->fn=fn; ts->arg=arg; h->kind=PEX_HANDLE_THREAD;
    h->uid=sceKernelCreateThread("pxc_thread", pex_thread_trampoline, 0x18, stack?stack:0x10000, PSP_THREAD_ATTR_USER, NULL);
    if(h->uid<0){free(ts);free(h);return NULL;} sceKernelStartThread(h->uid, sizeof(ts), ts); return h;
}
static inline HANDLE CreateEventA(void *sa, BOOL manual_reset, BOOL initial_state, const char *name) {
    (void)sa; HANDLE h=(HANDLE)calloc(1,sizeof(*h)); if(!h) return NULL; h->kind=PEX_HANDLE_EVENT; h->manual_reset=manual_reset; h->signaled=initial_state; h->uid=sceKernelCreateSema(name?name:"pxc_ev",0,initial_state?1:0,1,NULL); return h;
}
static inline BOOL SetEvent(HANDLE h) { if(!h||h->kind!=PEX_HANDLE_EVENT)return FALSE; sceKernelSignalSema(h->uid,1); h->signaled=1; return TRUE; }
static inline DWORD WaitForSingleObject(HANDLE h, DWORD ms) {
    if(!h||h==INVALID_HANDLE_VALUE) return WAIT_TIMEOUT;
    if(h->kind==PEX_HANDLE_THREAD){ if(ms==0) return WAIT_TIMEOUT; sceKernelWaitThreadEnd(h->uid,NULL); return WAIT_OBJECT_0; }
    if(h->kind==PEX_HANDLE_EVENT){ if(ms==0){ SceUInt t=0; return sceKernelWaitSema(h->uid,1,&t)>=0 ? WAIT_OBJECT_0:WAIT_TIMEOUT; } sceKernelWaitSema(h->uid,1,NULL); return WAIT_OBJECT_0; }
    return WAIT_TIMEOUT;
}
static inline BOOL CloseHandle(HANDLE h) { if(!h||h==INVALID_HANDLE_VALUE)return FALSE; if(h->kind==PEX_HANDLE_DIR&&h->dir) closedir(h->dir); else if(h->kind==PEX_HANDLE_THREAD) sceKernelDeleteThread(h->uid); else if(h->kind==PEX_HANDLE_EVENT) sceKernelDeleteSema(h->uid); free(h); return TRUE; }
static inline BOOL FindClose(HANDLE h) { return CloseHandle(h); }
static inline BOOL SetThreadPriority(HANDLE h, int prio) { (void)h; (void)prio; return TRUE; }
static inline LONG InterlockedExchange(volatile LONG *target, LONG value) { return __sync_lock_test_and_set(target, value); }
static inline LONG InterlockedCompareExchange(volatile LONG *target, LONG exchange, LONG comparand) { return __sync_val_compare_and_swap(target, comparand, exchange); }
static inline void Sleep(DWORD ms) { sceKernelDelayThread((SceUInt)ms * 1000u); }
static inline void ExitProcess(UINT code) { sceKernelExitGame(); (void)code; }
static inline BOOL MoveFileA(const char *src, const char *dst) { char s[1024], d[1024]; pex_normalize_path(s,sizeof(s),src); pex_normalize_path(d,sizeof(d),dst); return rename(s,d)==0; }
static inline DWORD GetTickCount(void) { return (DWORD)(sceKernelGetSystemTimeLow() / 1000u); }
static inline HMODULE LoadLibraryA(const char *name) { (void)name; return NULL; }
static inline void *GetProcAddress(HMODULE h, const char *name) { (void)h; (void)name; return NULL; }
static inline BOOL FreeLibrary(HMODULE h) { (void)h; return TRUE; }
static inline HANDLE GetCurrentProcess(void) { return NULL; }
static inline UINT MapVirtualKeyA(UINT code, UINT maptype) { (void)maptype; return code; }
static inline int GetKeyNameTextA(LONG lparam, char *buf, int cap) { (void)lparam; if(buf&&cap)buf[0]=0; return 0; }
static inline DWORD GetModuleFileNameA(void *module, char *buf, DWORD cap) { (void)module; if(buf&&cap){snprintf(buf,cap,"EBOOT.PBP"); return (DWORD)strlen(buf);} return 0; }
static inline void *ShellExecuteA(void *hwnd, const char *op, const char *file, const char *params, const char *dir, int show) { (void)hwnd; (void)op; (void)file; (void)params; (void)dir; (void)show; return NULL; }
static inline void PostQuitMessage(int code) { (void)code; }


static inline int gluProject(GLdouble objx, GLdouble objy, GLdouble objz,
                             const GLdouble *model, const GLdouble *proj, const GLint *viewport,
                             GLdouble *winx, GLdouble *winy, GLdouble *winz) {
    (void)objx; (void)objy; (void)objz; (void)model; (void)proj;
    if (winx && viewport) *winx = (GLdouble)(viewport[0] + viewport[2] / 2);
    if (winy && viewport) *winy = (GLdouble)(viewport[1] + viewport[3] / 2);
    if (winz) *winz = 0.5;
    return 0; /* Name-tag projection is disabled on PSP for now. */
}

#endif /* PEXCRAFT_PSP_COMPAT_H */
