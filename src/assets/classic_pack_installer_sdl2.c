/* SDL2/Linux stub for the Windows WinINet texture-pack installer.
   Built-in .mcrw assets still load normally. Users can manually place Classic
   pack PNGs under ~/.pexcraft/texturepacks/Minecraft Classic/. */
#define CLASSIC_INSTALL_IDLE 0
#define CLASSIC_INSTALL_DOWNLOADING 1
#define CLASSIC_INSTALL_EXTRACTING 2
#define CLASSIC_INSTALL_DONE 3
#define CLASSIC_INSTALL_ERROR 4

static volatile LONG g_classic_install_state = CLASSIC_INSTALL_IDLE;
static volatile LONG g_classic_install_progress = 0;
static char g_classic_install_status[MAX_LABEL] = "";
static char g_classic_install_error[MAX_LABEL] = "";

static void classic_resource_size_start_fetch(void) {
    snprintf(g_classic_install_status, sizeof(g_classic_install_status), "Manual install on Linux/SDL2");
}

static void classic_resource_size_format(char *out, size_t cap) {
    snprintf(out, cap, "Linux SDL2 build: manually copy Classic textures into ~/.pexcraft/texturepacks/%s", CLASSIC_PACK_NAME);
}

static void start_classic_pack_install(void) {
    snprintf(g_classic_install_error, sizeof(g_classic_install_error), "Automatic Classic texture download is Windows-only in this branch.");
    InterlockedExchange(&g_classic_install_progress, 0);
    InterlockedExchange(&g_classic_install_state, CLASSIC_INSTALL_ERROR);
    open_notice("Texture Pack", "Automatic download is disabled on Linux/SDL2.", "Copy PNGs into ~/.pexcraft/texturepacks/Minecraft Classic/.");
}

static void classic_pack_install_tick(void) {
    if (InterlockedCompareExchange(&g_classic_install_state, 0, 0) == CLASSIC_INSTALL_ERROR) {
        InterlockedExchange(&g_classic_install_state, CLASSIC_INSTALL_IDLE);
    }
}
