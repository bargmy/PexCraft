/* Wii classic texture pack installer.
   No runtime network or SD texturepack dependency is used.  The Wii workflow
   downloads client.jar during CI, converts the needed PNGs to MCRW, packs them
   into build/wii_generated/wii_mcrw_assets.pak, and links that pak into
   boot.dol through platforms/wii/Makefile.wii. */

extern const unsigned char pexcraft_wii_mcrw_assets_pak_start[];
extern const unsigned char pexcraft_wii_mcrw_assets_pak_end[];

#define CLASSIC_INSTALL_IDLE 0
#define CLASSIC_INSTALL_DOWNLOADING 1
#define CLASSIC_INSTALL_EXTRACTING 2
#define CLASSIC_INSTALL_DONE 3
#define CLASSIC_INSTALL_ERROR 4

#define CLASSIC_SIZE_UNKNOWN 0
#define CLASSIC_SIZE_FETCHING 1
#define CLASSIC_SIZE_READY 2
#define CLASSIC_SIZE_ERROR 3

static volatile LONG g_classic_install_state = CLASSIC_INSTALL_IDLE;
static volatile LONG g_classic_install_progress = 0;
static HANDLE g_classic_install_thread = NULL;
static char g_classic_install_status[MAX_LABEL] = "";
static char g_classic_install_error[MAX_LABEL] = "";
static volatile LONG g_classic_download_size_state = CLASSIC_SIZE_READY;
static volatile LONG g_classic_download_size_bytes = 0;

static unsigned int wii_embedded_mcrw_pak_len(void) {
    const unsigned char *start = pexcraft_wii_mcrw_assets_pak_start;
    const unsigned char *end = pexcraft_wii_mcrw_assets_pak_end;
    if (!start || !end || end < start) return 0;
    return (unsigned int)(end - start);
}

static void classic_install_set_state(LONG state, LONG progress, const char *status) {
    if (status) lstrcpynA(g_classic_install_status, status, sizeof(g_classic_install_status));
    InterlockedExchange(&g_classic_install_progress, progress);
    InterlockedExchange(&g_classic_install_state, state);
}

static void classic_resource_size_start_fetch(void) {
    InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_READY);
    InterlockedExchange(&g_classic_download_size_bytes, (LONG)wii_embedded_mcrw_pak_len());
}

static void classic_resource_size_format(char *out, size_t cap) {
    if (!out || cap == 0) return;
    unsigned int len = wii_embedded_mcrw_pak_len();
    if (len > 0) snprintf(out, cap, "embedded in DOL (%u bytes)", len);
    else snprintf(out, cap, "not embedded in this build");
}

static void start_classic_pack_install(void) {
    (void)g_classic_install_thread;
    if (wii_embedded_mcrw_pak_len() == 0) {
        lstrcpynA(g_classic_install_error, "Embedded Wii texture bundle is empty.", sizeof(g_classic_install_error));
        classic_install_set_state(CLASSIC_INSTALL_ERROR, 0, "Failed");
    } else {
        classic_install_set_state(CLASSIC_INSTALL_DONE, 100, "Embedded in DOL");
    }
    set_screen(SCREEN_TEXPACK_INSTALL);
}

static void wii_install_embedded_classic_pack_if_needed(void) {
    g_opts.ignore_classic_resources_warning = 1;
    snprintf(g_opts.skin, sizeof(g_opts.skin), "%s", CLASSIC_PACK_NAME);
    classic_install_set_state(CLASSIC_INSTALL_DONE, 100, "Embedded in DOL");
}

static void classic_pack_install_tick(void) {
    LONG state = InterlockedCompareExchange(&g_classic_install_state, 0, 0);
    if (state == CLASSIC_INSTALL_DONE) {
        InterlockedExchange(&g_classic_install_state, CLASSIC_INSTALL_IDLE);
        scan_texture_packs();
        for (int i = 0; i < g_texpack_count; i++) {
            if (g_texpacks[i].is_builtin_classic) { apply_texture_pack_index(i); break; }
        }
        set_screen(SCREEN_TEXPACK);
    } else if (state == CLASSIC_INSTALL_ERROR) {
        InterlockedExchange(&g_classic_install_state, CLASSIC_INSTALL_IDLE);
        open_notice("Texture Pack", "Could not use embedded Wii Classic pack.", g_classic_install_error);
    }
}
