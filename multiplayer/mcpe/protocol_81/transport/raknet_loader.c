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

static void* pex_dyn_open(const char *path, char *err, size_t err_cap) {
#if defined(_WIN32)
    HMODULE h = LoadLibraryA(path);
    if (!h && err && err_cap) snprintf(err, err_cap, "LoadLibraryA failed for %s", path);
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
    const char *candidates[8];
    char p0[512], p1[512], p2[512], p3[512];
    snprintf(p0, sizeof(p0), "multiplayer/mcpe/protocol_81/transport/bin/%s/%s", platform, name);
    snprintf(p1, sizeof(p1), "./multiplayer/mcpe/protocol_81/transport/bin/%s/%s", platform, name);
    snprintf(p2, sizeof(p2), "../multiplayer/mcpe/protocol_81/transport/bin/%s/%s", platform, name);
    snprintf(p3, sizeof(p3), "%s", name);
    candidates[0] = p0; candidates[1] = p1; candidates[2] = p2; candidates[3] = p3; candidates[4] = NULL;

    PexRakNetLibrary *lib = (PexRakNetLibrary*)calloc(1, sizeof(*lib));
    if (!lib) return NULL;

    for (int i = 0; candidates[i]; ++i) {
        char err[256] = "";
        void *h = pex_dyn_open(candidates[i], err, sizeof(err));
        if (!h) {
            snprintf(lib->error, sizeof(lib->error), "%s", err[0] ? err : "Could not load RakNetDLL");
            continue;
        }
        lib->handle = h;
        snprintf(lib->path, sizeof(lib->path), "%s", candidates[i]);
        if (pex_raknet_library_load_symbols(lib)) return lib;
        pex_dyn_close(lib->handle);
        lib->handle = NULL;
    }
    if (!lib->error[0]) snprintf(lib->error, sizeof(lib->error), "Could not find %s for %s", name, platform);
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
