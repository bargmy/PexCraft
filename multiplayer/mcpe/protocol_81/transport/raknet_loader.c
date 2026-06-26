#include "raknet_loader.h"
#include "raknet_library_names.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

const char* pex_raknet_library_expected_name(void) {
#if defined(_WIN32)
    return PEX_RAKNET_LIB_NAME_WINDOWS;
#else
    return PEX_RAKNET_LIB_NAME_LINUX;
#endif
}

const char* pex_raknet_library_expected_platform_folder(void) {
#if defined(_WIN32) && (defined(_M_ARM64) || defined(__aarch64__))
    return PEX_RAKNET_PLATFORM_WINDOWS_ARM64;
#elif defined(_WIN32) && (defined(_M_IX86) || defined(__i386__))
    return PEX_RAKNET_PLATFORM_WINDOWS_X86;
#elif defined(_WIN32)
    return PEX_RAKNET_PLATFORM_WINDOWS_X64;
#elif defined(__aarch64__)
    return PEX_RAKNET_PLATFORM_LINUX_ARM64;
#elif defined(__i386__)
    return PEX_RAKNET_PLATFORM_LINUX_X86;
#else
    return PEX_RAKNET_PLATFORM_LINUX_X64;
#endif
}

static void pex_append_error(char *dst, size_t dst_cap, const char *msg) {
    if (!dst || dst_cap == 0 || !msg || !msg[0]) return;
    size_t len = strlen(dst);
    if (len + 3 >= dst_cap) return;
    if (len) {
        dst[len++] = '\n';
        dst[len] = 0;
    }
    snprintf(dst + len, dst_cap - len, "%s", msg);
}

#if defined(_WIN32)
static void pex_win_format_error(DWORD code, char *buf, size_t cap) {
    if (!buf || cap == 0) return;
    buf[0] = 0;
    if (!code) {
        snprintf(buf, cap, "GetLastError=0");
        return;
    }
    char msg[512] = "";
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   msg, (DWORD)sizeof(msg), NULL);
    for (char *p = msg; *p; ++p) {
        if (*p == '\r' || *p == '\n') *p = ' ';
    }
    snprintf(buf, cap, "GetLastError=%lu (%s)", (unsigned long)code, msg[0] ? msg : "unknown Windows loader error");
}

static int pex_win_get_exe_dir(char *buf, size_t cap) {
    if (!buf || cap == 0) return 0;
    DWORD n = GetModuleFileNameA(NULL, buf, (DWORD)cap);
    if (!n || n >= cap) {
        buf[0] = 0;
        return 0;
    }
    for (char *p = buf + strlen(buf); p > buf; --p) {
        if (p[-1] == '\\' || p[-1] == '/') {
            p[-1] = 0;
            return 1;
        }
    }
    return 0;
}
#endif

static void* pex_dyn_open(const char *path, char *err, size_t err_cap) {
#if defined(_WIN32)
    HMODULE h = NULL;
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    /* LOAD_WITH_ALTERED_SEARCH_PATH makes Windows search the DLL's own folder
       for secondary dependencies like libstdc++-6.dll / libgcc / pthread DLLs. */
    if (strchr(path, '\\') || strchr(path, '/')) {
        h = LoadLibraryExA(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    } else {
        h = LoadLibraryA(path);
    }

    if (!h && err && err_cap) {
        DWORD code = GetLastError();
        char winerr[640];
        pex_win_format_error(code, winerr, sizeof(winerr));
        snprintf(err, err_cap, "LoadLibrary failed for %s: %s", path, winerr);
    }
    return (void*)h;
#else
    void *h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!h && err && err_cap) {
        const char *de = dlerror();
        snprintf(err, err_cap, "%s", de ? de : "dlopen failed");
    }
    return h;
#endif
}

static void pex_dyn_close(void *handle) {
    if (!handle) return;
#if defined(_WIN32)
    FreeLibrary((HMODULE)handle);
#else
    dlclose(handle);
#endif
}

static void* pex_dyn_sym(void *handle, const char *name) {
#if defined(_WIN32)
    return (void*)GetProcAddress((HMODULE)handle, name);
#else
    return dlsym(handle, name);
#endif
}

#define LOAD_SYM(lib, field, sym) do { \
    (lib)->field = (void*)pex_dyn_sym((lib)->handle, (sym)); \
    if (!(lib)->field) { snprintf((lib)->error, sizeof((lib)->error), "RakNetDLL missing symbol %s", (sym)); goto fail; } \
} while (0)

static int pex_raknet_library_load_symbols(PexRakNetLibrary *lib) {
    LOAD_SYM(lib, peer_create, "raknet_peer_create");
    LOAD_SYM(lib, peer_destroy, "raknet_peer_destroy");
    LOAD_SYM(lib, peer_startup, "raknet_peer_startup");
    LOAD_SYM(lib, peer_set_max_incoming, "raknet_peer_set_max_incoming");
    LOAD_SYM(lib, peer_connect, "raknet_peer_connect");
    LOAD_SYM(lib, peer_shutdown, "raknet_peer_shutdown");
    LOAD_SYM(lib, peer_send, "raknet_peer_send");
    LOAD_SYM(lib, peer_receive, "raknet_peer_receive");
    LOAD_SYM(lib, packet_get_info, "raknet_packet_get_info");
    LOAD_SYM(lib, peer_deallocate_packet, "raknet_peer_deallocate_packet");
    LOAD_SYM(lib, peer_is_active, "raknet_peer_is_active");
    LOAD_SYM(lib, peer_get_connection_count, "raknet_peer_get_connection_count");
    LOAD_SYM(lib, peer_get_average_ping, "raknet_peer_get_average_ping");
    return 1;
fail:
    return 0;
}

PexRakNetLibrary* pex_raknet_library_open_for_current_platform(void) {
    const char *platform = pex_raknet_library_expected_platform_folder();
    const char *name = pex_raknet_library_expected_name();
    const char *candidates[16];
    char p0[512], p1[512], p2[512], p3[512], p4[512], p5[512], p6[512], p7[512];
    int ci = 0;

#if defined(_WIN32)
    char exe_dir[512] = "";
    if (pex_win_get_exe_dir(exe_dir, sizeof(exe_dir))) {
        snprintf(p0, sizeof(p0), "%s\\%s", exe_dir, name);
        snprintf(p1, sizeof(p1), "%s\\multiplayer\\mcpe\\protocol_81\\transport\\bin\\%s\\%s", exe_dir, platform, name);
        snprintf(p2, sizeof(p2), "%s\\src\\multiplayer\\mcpe\\protocol_81\\transport\\bin\\%s\\%s", exe_dir, platform, name);
        candidates[ci++] = p0;
        candidates[ci++] = p1;
        candidates[ci++] = p2;
    }
    snprintf(p3, sizeof(p3), ".\\%s", name);
    snprintf(p4, sizeof(p4), ".\\multiplayer\\mcpe\\protocol_81\\transport\\bin\\%s\\%s", platform, name);
    snprintf(p5, sizeof(p5), ".\\src\\multiplayer\\mcpe\\protocol_81\\transport\\bin\\%s\\%s", platform, name);
    snprintf(p6, sizeof(p6), "multiplayer\\mcpe\\protocol_81\\transport\\bin\\%s\\%s", platform, name);
    snprintf(p7, sizeof(p7), "%s", name);
#else
    snprintf(p0, sizeof(p0), "./%s", name);
    snprintf(p1, sizeof(p1), "multiplayer/mcpe/protocol_81/transport/bin/%s/%s", platform, name);
    snprintf(p2, sizeof(p2), "./multiplayer/mcpe/protocol_81/transport/bin/%s/%s", platform, name);
    candidates[ci++] = p0;
    candidates[ci++] = p1;
    candidates[ci++] = p2;
    snprintf(p3, sizeof(p3), "../multiplayer/mcpe/protocol_81/transport/bin/%s/%s", platform, name);
    snprintf(p4, sizeof(p4), "src/multiplayer/mcpe/protocol_81/transport/bin/%s/%s", platform, name);
    snprintf(p5, sizeof(p5), "./src/multiplayer/mcpe/protocol_81/transport/bin/%s/%s", platform, name);
    snprintf(p6, sizeof(p6), "%s", name);
    p7[0] = 0;
#endif
    candidates[ci++] = p3;
    candidates[ci++] = p4;
    candidates[ci++] = p5;
    candidates[ci++] = p6;
    candidates[ci++] = p7[0] ? p7 : name;
    candidates[ci] = NULL;

    PexRakNetLibrary *lib = (PexRakNetLibrary*)calloc(1, sizeof(*lib));
    if (!lib) return NULL;

    char attempts[2048] = "";
    for (int i = 0; candidates[i]; ++i) {
        if (!candidates[i][0]) continue;
        char err[768] = "";
        void *h = pex_dyn_open(candidates[i], err, sizeof(err));
        if (!h) {
            char line[900];
            snprintf(line, sizeof(line), "%s", err[0] ? err : "Could not load RakNetDLL");
            pex_append_error(attempts, sizeof(attempts), line);
            continue;
        }
        lib->handle = h;
        snprintf(lib->path, sizeof(lib->path), "%s", candidates[i]);
        if (pex_raknet_library_load_symbols(lib)) return lib;

        char line[768];
        snprintf(line, sizeof(line), "%s loaded but symbol check failed: %s", candidates[i], lib->error[0] ? lib->error : "unknown symbol error");
        pex_append_error(attempts, sizeof(attempts), line);
        pex_dyn_close(lib->handle);
        lib->handle = NULL;
    }

    if (attempts[0]) {
        snprintf(lib->error, sizeof(lib->error), "%s", attempts);
    } else {
        snprintf(lib->error, sizeof(lib->error), "Could not find %s for %s", name, platform);
    }
    return lib;
}

void pex_raknet_library_close(PexRakNetLibrary* lib) {
    if (!lib) return;
    if (lib->handle) pex_dyn_close(lib->handle);
    free(lib);
}

const char* pex_raknet_library_last_error(const PexRakNetLibrary* lib) {
    return lib && lib->error[0] ? lib->error : "RakNetDLL load failed";
}
