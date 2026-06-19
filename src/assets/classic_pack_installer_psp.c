/* PSP classic texture pack installer.
   No runtime network is used. The PSP workflow downloads client.jar during CI,
   extracts the classic assets into a zip, generates psp_embedded_classic_pack.c,
   and links that data into EBOOT.PBP. */

extern const unsigned char pexcraft_psp_classic_pack_zip[];
extern const unsigned int pexcraft_psp_classic_pack_zip_len;

static volatile LONG g_classic_install_state = CLASSIC_INSTALL_IDLE;
static volatile LONG g_classic_install_progress = 0;
static char g_classic_install_status[MAX_LABEL] = "";
static char g_classic_install_error[MAX_LABEL] = "";
static volatile LONG g_classic_download_size_state = CLASSIC_SIZE_READY;
static volatile LONG g_classic_download_size_bytes = 0;

static void classic_install_set_state(LONG state, LONG progress, const char *status) {
    if (status) lstrcpynA(g_classic_install_status, status, sizeof(g_classic_install_status));
    InterlockedExchange(&g_classic_install_progress, progress);
    InterlockedExchange(&g_classic_install_state, state);
}

static void classic_install_fail(const char *msg) {
    lstrcpynA(g_classic_install_error, msg ? msg : "Unknown error", sizeof(g_classic_install_error));
    classic_install_set_state(CLASSIC_INSTALL_ERROR, 0, "Failed");
}

static void classic_resource_size_start_fetch(void) {
    InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_READY);
    InterlockedExchange(&g_classic_download_size_bytes, (LONG)pexcraft_psp_classic_pack_zip_len);
}

static void classic_resource_size_format(char *out, size_t cap) {
    if (pexcraft_psp_classic_pack_zip_len > 0) snprintf(out, cap, "embedded in EBOOT (%u bytes)", pexcraft_psp_classic_pack_zip_len);
    else snprintf(out, cap, "not embedded in this build");
}

static int psp_write_embedded_classic_zip(const char *zip_path) {
    if (pexcraft_psp_classic_pack_zip_len == 0) return 0;
    FILE *f = fopen(zip_path, "wb");
    if (!f) return 0;
    size_t n = fwrite(pexcraft_psp_classic_pack_zip, 1, pexcraft_psp_classic_pack_zip_len, f);
    fclose(f);
    return n == pexcraft_psp_classic_pack_zip_len;
}

static void start_classic_pack_install(void) {
    char zip_path[MAX_PATHBUF], pack_dir[MAX_PATHBUF], err[256];
    err[0] = 0;
    classic_pack_path(pack_dir, sizeof(pack_dir));
    snprintf(zip_path, sizeof(zip_path), "%s/psp_embedded_classic_pack.zip", g_mc_dir);
    classic_install_set_state(CLASSIC_INSTALL_EXTRACTING, 10, "Installing embedded Classic pack...");
    if (!psp_write_embedded_classic_zip(zip_path)) { classic_install_fail("This EBOOT has no embedded Classic pack"); return; }
    delete_recursive(pack_dir);
    ensure_dir(pack_dir);
    if (!pxc_extract_classic_pack(zip_path, pack_dir, err, sizeof(err))) { classic_install_fail(err[0] ? err : "Could not extract embedded Classic pack"); return; }
    DeleteFileA(zip_path);
    if (!classic_pack_installed() || classic_pack_missing_required_textures()) { classic_install_fail("Embedded Classic pack is missing required textures"); return; }
    classic_install_set_state(CLASSIC_INSTALL_DONE, 100, "Done");
}

static void psp_install_embedded_classic_pack_if_needed(void) {
    g_opts.ignore_classic_resources_warning = 1;
    snprintf(g_opts.skin, sizeof(g_opts.skin), "%s", CLASSIC_PACK_NAME);
    if (!classic_pack_installed() && pexcraft_psp_classic_pack_zip_len > 0) start_classic_pack_install();
    save_options();
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
        open_notice("Texture Pack", "Could not install embedded Classic pack.", g_classic_install_error);
    }
}
