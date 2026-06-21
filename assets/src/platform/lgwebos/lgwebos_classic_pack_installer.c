/* LG webOS classic texture pack installer stub.
   Runtime downloading is intentionally disabled for the first native TV port.
   The CI/package flow extracts the app icon from client.jar, but game assets
   should be bundled or copied to the writable texturepacks directory later. */

#ifndef CLASSIC_INSTALL_IDLE
#define CLASSIC_INSTALL_IDLE 0
#define CLASSIC_INSTALL_DOWNLOADING 1
#define CLASSIC_INSTALL_EXTRACTING 2
#define CLASSIC_INSTALL_DONE 3
#define CLASSIC_INSTALL_ERROR 4
#endif
#ifndef CLASSIC_SIZE_UNKNOWN
#define CLASSIC_SIZE_UNKNOWN 0
#define CLASSIC_SIZE_FETCHING 1
#define CLASSIC_SIZE_READY 2
#define CLASSIC_SIZE_ERROR 3
#endif

static volatile LONG g_classic_install_state = CLASSIC_INSTALL_IDLE;
static volatile LONG g_classic_install_progress = 0;
static char g_classic_install_status[MAX_LABEL] = "";
static char g_classic_install_error[MAX_LABEL] = "";
static volatile LONG g_classic_download_size_state = CLASSIC_SIZE_ERROR;
static volatile LONG g_classic_download_size_bytes = 0;

static void classic_install_set_state(LONG state, LONG progress, const char *status) {
    if (status) lstrcpynA(g_classic_install_status, status, sizeof(g_classic_install_status));
    InterlockedExchange(&g_classic_install_progress, progress);
    InterlockedExchange(&g_classic_install_state, state);
}

static void classic_install_fail(const char *msg) {
    lstrcpynA(g_classic_install_error, msg ? msg : "Unsupported on LG webOS build", sizeof(g_classic_install_error));
    classic_install_set_state(CLASSIC_INSTALL_ERROR, 0, "Unavailable");
}

static void classic_resource_size_start_fetch(void) {
    InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_ERROR);
    InterlockedExchange(&g_classic_download_size_bytes, 0);
}

static void classic_resource_size_format(char *out, size_t cap) {
    snprintf(out, cap, "not available in this LG webOS build");
}

static void start_classic_pack_install(void) {
    classic_install_fail("Runtime client.jar download is disabled for LG webOS. Bundle or copy a texture pack instead.");
}

static void classic_pack_install_tick(void) {
    LONG state = InterlockedCompareExchange(&g_classic_install_state, 0, 0);
    if (state == CLASSIC_INSTALL_ERROR) {
        InterlockedExchange(&g_classic_install_state, CLASSIC_INSTALL_IDLE);
        open_notice("Texture Pack", "Classic pack download is disabled on LG webOS for now.", g_classic_install_error);
    }
}
