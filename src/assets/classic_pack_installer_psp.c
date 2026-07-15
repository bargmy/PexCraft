/* PSP classic texture pack installer.
   No runtime network is used. The PSP workflow downloads client.jar during CI,
   extracts the classic assets into a zip, generates psp_embedded_classic_pack.c,
   and links that data into EBOOT.PBP. */

extern const unsigned char pexcraft_psp_classic_pack_zip[];
extern const unsigned int pexcraft_psp_classic_pack_zip_len;

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
static volatile LONG g_classic_download_size_state = CLASSIC_SIZE_READY;
static volatile LONG g_classic_download_size_bytes = 0;

static void pack_install_set_state(LONG state, LONG progress, const char *status) {
    if (status) lstrcpynA(g_classic_install_status, status, sizeof(g_classic_install_status));
    InterlockedExchange(&g_classic_install_progress, progress);
    InterlockedExchange(&g_classic_install_state, state);
}

static void pack_install_fail(const char *msg) {
    lstrcpynA(g_classic_install_error, msg ? msg : "Unknown error", sizeof(g_classic_install_error));
    pack_install_set_state(CLASSIC_INSTALL_ERROR, 0, "Failed");
}

static void pack_install_start_size_fetch(void) {
    InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_READY);
    InterlockedExchange(&g_classic_download_size_bytes, (LONG)pexcraft_psp_classic_pack_zip_len);
}

static void format_download_size(char *out, size_t cap) {
    if (pexcraft_psp_classic_pack_zip_len > 0) snprintf(out, cap, "embedded in EBOOT (%u bytes)", pexcraft_psp_classic_pack_zip_len);
    else snprintf(out, cap, "not embedded in this build");
}

static int psp_write_embedded_classic_zip(const char *zip_path) {
    (void)zip_path;
    return pexcraft_psp_classic_pack_zip_len > 0;
}

static void pack_install_start(void) {
    /* Memory-only PSP mode: no extraction to Memory Stick.  The workflow embeds
       already-converted .mcrw textures in the EBOOT, so installation is instant. */
    pack_install_set_state(CLASSIC_INSTALL_DONE, 100, "Embedded in EBOOT");
}

static void pack_install_request_cancel(void) {
    /* Nothing is downloaded or extracted at runtime on PSP. */
}

static void psp_install_embedded_pack_if_needed(void) {
    g_opts.ignore_classic_resources_warning = 1;
    snprintf(g_opts.skin, sizeof(g_opts.skin), "%s", CLASSIC_PACK_NAME);
    pack_install_set_state(CLASSIC_INSTALL_DONE, 100, "Embedded in EBOOT");
}

static void pack_install_tick(void) {
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
        open_notice("Texture Pack", "Could not install embedded Classic pack.", g_classic_install_error);
    }
}
