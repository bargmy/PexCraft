/* First-pass Wii build: no network/classic-pack installer yet.
   Use bundled/default assets or copy texture packs to sd:/apps/pexcraft/texturepacks. */

#define CLASSIC_INSTALL_IDLE 0
#define CLASSIC_INSTALL_DOWNLOADING 1
#define CLASSIC_INSTALL_EXTRACTING 2
#define CLASSIC_INSTALL_DONE 3
#define CLASSIC_INSTALL_ERROR 4

#define CLASSIC_SIZE_IDLE 0
#define CLASSIC_SIZE_FETCHING 1
#define CLASSIC_SIZE_DONE 2
#define CLASSIC_SIZE_ERROR 3

static volatile LONG g_classic_install_state = CLASSIC_INSTALL_IDLE;
static volatile LONG g_classic_install_progress = 0;
static HANDLE g_classic_install_thread = NULL;
static char g_classic_install_status[MAX_LABEL] = "";
static char g_classic_install_error[MAX_LABEL] = "";
static volatile LONG g_classic_download_size_state = CLASSIC_SIZE_ERROR;
static volatile LONG g_classic_download_size_bytes = 0;

static void pack_install_start_size_fetch(void) {
    InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_ERROR);
    InterlockedExchange(&g_classic_download_size_bytes, 0);
}

static void format_download_size(char *out, size_t cap) {
    if (!out || cap == 0) return;
    snprintf(out, cap, "unavailable on Wii");
}

static void pack_install_start(void) {
    (void)g_classic_install_thread;
    InterlockedExchange(&g_classic_install_progress, 0);
    lstrcpynA(g_classic_install_status, "Classic pack download is disabled on Wii.", sizeof(g_classic_install_status));
    lstrcpynA(g_classic_install_error, "Copy texture packs to sd:/apps/pexcraft/texturepacks.", sizeof(g_classic_install_error));
    InterlockedExchange(&g_classic_install_state, CLASSIC_INSTALL_ERROR);
    set_screen(SCREEN_TEXPACK_INSTALL);
}

static void pack_install_tick(void) {
    LONG state = InterlockedCompareExchange(&g_classic_install_state, 0, 0);
    if (state == CLASSIC_INSTALL_ERROR) {
        InterlockedExchange(&g_classic_install_state, CLASSIC_INSTALL_IDLE);
        open_notice("Texture Pack", "Classic pack download is disabled on Wii.", g_classic_install_error);
    }
}
