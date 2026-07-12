/* The browser package is immutable and already contains the verified 1.2.5
   client.jar resources. Runtime HTTP installers are deliberately unavailable. */
#ifndef CLASSIC_INSTALL_IDLE
#define CLASSIC_INSTALL_IDLE 0
#define CLASSIC_INSTALL_DOWNLOADING 1
#define CLASSIC_INSTALL_EXTRACTING 2
#define CLASSIC_INSTALL_DONE 3
#define CLASSIC_INSTALL_ERROR 4
#endif

static volatile LONG g_classic_install_state = CLASSIC_INSTALL_DONE;
static volatile LONG g_classic_install_progress = 100;
static volatile LONG g_classic_download_size_state = CLASSIC_SIZE_READY;
static HANDLE g_classic_install_thread = NULL;
static char g_classic_install_status[MAX_LABEL] = "Bundled into this HTML";
static char g_classic_install_error[MAX_LABEL] = "";

static void pack_install_set_state(LONG state, LONG progress, const char *status) {
    InterlockedExchange(&g_classic_install_state, state);
    InterlockedExchange(&g_classic_install_progress, progress);
    if (status) lstrcpynA(g_classic_install_status, status, (int)sizeof(g_classic_install_status));
}
static void pack_install_fail(const char *msg) {
    lstrcpynA(g_classic_install_error, msg ? msg : "Unavailable", (int)sizeof(g_classic_install_error));
    pack_install_set_state(CLASSIC_INSTALL_ERROR, 0, "Unavailable in browser");
}
static void pack_install_reset_cancel(void) { }
static void pack_install_request_cancel(void) { }
static int pack_install_is_cancelled(void) { return 0; }
static void pack_install_start_size_fetch(void) { InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_READY); }
static void format_download_size(char *out, size_t cap) {
    if (out && cap) snprintf(out, cap, "Resources are embedded in this HTML");
}
static int pack_resources_install_blocking(void) { return pack_is_installed(); }
static void pack_install_start(void) {
    if (pack_is_installed()) {
        pack_install_set_state(CLASSIC_INSTALL_DONE, 100, "Resources are embedded");
        scan_texture_packs();
        load_default_textures();
        set_screen(SCREEN_TITLE);
    } else {
        pack_install_fail("Embedded Minecraft 1.2.5 resources are missing");
    }
}
static void pack_install_tick(void) {
    if (InterlockedCompareExchange(&g_classic_install_state, 0, 0) == CLASSIC_INSTALL_DONE &&
        g_screen == SCREEN_TEXPACK_INSTALL) set_screen(SCREEN_TITLE);
}
