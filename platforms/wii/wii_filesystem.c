/* Nintendo Wii filesystem/timing layer. Included only by src/main_wii.c. */

static int g_wii_fat_ready = 0;

static double now_seconds(void) {
    return (double)ticks_to_millisecs(gettime()) / 1000.0;
}

static double pex_profile_begin(void) { return now_seconds(); }

static void pex_profile_add(PexMainThreadProfileId id, double start_time) {
    if (id < 0 || id >= PROF_COUNT) return;
    double ms = (now_seconds() - start_time) * 1000.0;
    if (ms < 0.0) ms = 0.0;
    g_prof_frame_ms[id] += ms;
}

static void pex_profile_frame_begin(void) {
    memset(g_prof_frame_ms, 0, sizeof(g_prof_frame_ms));
    g_prof_frame_start_time = now_seconds();
    if (g_prof_accum_start_time <= 0.0) g_prof_accum_start_time = g_prof_frame_start_time;
}

static void pex_profile_frame_end(void) {
    double now = now_seconds();
    double frame_ms = (now - g_prof_frame_start_time) * 1000.0;
    if (frame_ms < 0.0) frame_ms = 0.0;
    g_prof_accum_frame_ms += frame_ms;
    for (int i = 0; i < PROF_COUNT; i++) g_prof_accum_ms[i] += g_prof_frame_ms[i];
    g_prof_accum_frames++;
    double span = now - g_prof_accum_start_time;
    if (span >= 0.50 || g_prof_accum_frames >= 60) {
        int frames = g_prof_accum_frames > 0 ? g_prof_accum_frames : 1;
        g_prof_display_frame_ms = g_prof_accum_frame_ms / (double)frames;
        for (int i = 0; i < PROF_COUNT; i++) g_prof_display_ms[i] = g_prof_accum_ms[i] / (double)frames;
        memset(g_prof_accum_ms, 0, sizeof(g_prof_accum_ms));
        g_prof_accum_frame_ms = 0.0;
        g_prof_accum_frames = 0;
        g_prof_accum_start_time = now;
    }
}

static double pex_profile_pct(double ms) {
    double denom = g_prof_display_frame_ms > 0.001 ? g_prof_display_frame_ms : g_ft_last_frame_ms;
    if (denom <= 0.001) return 0.0;
    return (ms * 100.0) / denom;
}

static void log_msg(const char *fmt, ...) {
    char msg[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    wii_debug_logf("%s", msg);
}

static void path_join(char *out, size_t cap, const char *a, const char *b) {
    snprintf(out, cap, "%s/%s", a, b);
}

static int dir_exists(const char *path) {
    DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

static void ensure_dir(const char *path) {
    if (!g_wii_fat_ready) return;
    if (!dir_exists(path)) {
        BOOL ok = CreateDirectoryA(path, NULL);
        wii_debug_logf("ensure_dir %s -> %s", path ? path : "?", ok ? "ok" : "failed/already");
    }
}

static void init_dirs(void) {
    wii_debug_stagef("init_dirs / fatInitDefault");
    g_wii_fat_ready = fatInitDefault() ? 1 : 0;
    wii_debug_logf("fatInitDefault -> %s", g_wii_fat_ready ? "ok" : "FAILED; continuing without SD saves/options");
    snprintf(g_mc_dir, sizeof(g_mc_dir), "sd:/apps/pexcraft");
    path_join(g_save_dir, sizeof(g_save_dir), g_mc_dir, "saves");
    path_join(g_texpack_dir, sizeof(g_texpack_dir), g_mc_dir, "texturepacks");
    path_join(g_skin_dir, sizeof(g_skin_dir), g_mc_dir, "skins");
    if (!g_wii_fat_ready) return;
    ensure_dir("sd:/apps");
    ensure_dir(g_mc_dir);
    ensure_dir(g_save_dir);
    ensure_dir(g_texpack_dir);
    ensure_dir(g_skin_dir);
}

static void delete_recursive(const char *path) {
    char norm[MAX_PATHBUF]; pex_normalize_path(norm, sizeof(norm), path);
    DIR *d = opendir(norm);
    if (d) {
        struct dirent *de;
        while ((de = readdir(d)) != NULL) {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
            char child[MAX_PATHBUF]; snprintf(child, sizeof(child), "%s/%s", norm, de->d_name);
            struct stat st; if (stat(child, &st) == 0 && S_ISDIR(st.st_mode)) delete_recursive(child); else remove(child);
        }
        closedir(d);
    }
    rmdir(norm);
}

static unsigned long long dir_size(const char *path) {
    unsigned long long total = 0;
    char norm[MAX_PATHBUF]; pex_normalize_path(norm, sizeof(norm), path);
    DIR *d = opendir(norm); if (!d) return 0;
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        char child[MAX_PATHBUF]; snprintf(child, sizeof(child), "%s/%s", norm, de->d_name);
        struct stat st; if (stat(child, &st) == 0) { if (S_ISDIR(st.st_mode)) total += dir_size(child); else total += (unsigned long long)st.st_size; }
    }
    closedir(d);
    return total;
}
