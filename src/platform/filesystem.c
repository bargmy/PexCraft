/* Split from original monolithic main.c. Included by src/main.c unity build. */

static double now_seconds(void) {
    static double inv_freq = 0.0;
    LARGE_INTEGER counter;
    if (inv_freq == 0.0) {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        inv_freq = 1.0 / (double)freq.QuadPart;
    }
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * inv_freq;
}

static double pex_profile_begin(void) { return now_seconds(); }

static void pex_profile_add(PexMainThreadProfileId id, double start_time) {
    if (g_pex_profile_thread_role != PEX_PROFILE_ROLE_MAIN) return;
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
    if (!fmt) return;
    char msg[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    pex_logf("%s", msg);
}

static void path_join(char *out, size_t cap, const char *a, const char *b) {
    snprintf(out, cap, "%s\\%s", a, b);
}

static int dir_exists(const char *path) {
    DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

static void ensure_dir(const char *path) {
    if (!dir_exists(path)) CreateDirectoryA(path, NULL);
}

static void init_dirs(void) {
    const char *appdata = getenv("APPDATA");
    if (appdata && appdata[0]) snprintf(g_mc_dir, sizeof(g_mc_dir), "%s\\.pexcraft", appdata);
    else snprintf(g_mc_dir, sizeof(g_mc_dir), ".\\.pexcraft");
    ensure_dir(g_mc_dir);
    path_join(g_save_dir, sizeof(g_save_dir), g_mc_dir, "saves");
    path_join(g_texpack_dir, sizeof(g_texpack_dir), g_mc_dir, "texturepacks");
    path_join(g_skin_dir, sizeof(g_skin_dir), g_mc_dir, "skins");
    ensure_dir(g_save_dir);
    ensure_dir(g_texpack_dir);
    ensure_dir(g_skin_dir);
}

static void delete_recursive(const char *path) {
    char pattern[MAX_PATHBUF];
    WIN32_FIND_DATAA fd;
    snprintf(pattern, sizeof(pattern), "%s\\*", path);
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
            char child[MAX_PATHBUF];
            snprintf(child, sizeof(child), "%s\\%s", path, fd.cFileName);
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) delete_recursive(child);
            else {
                SetFileAttributesA(child, FILE_ATTRIBUTE_NORMAL);
                DeleteFileA(child);
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    SetFileAttributesA(path, FILE_ATTRIBUTE_NORMAL);
    RemoveDirectoryA(path);
}

static unsigned long long dir_size(const char *path) {
    unsigned long long total = 0;
    char pattern[MAX_PATHBUF];
    WIN32_FIND_DATAA fd;
    snprintf(pattern, sizeof(pattern), "%s\\*", path);
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
        char child[MAX_PATHBUF];
        snprintf(child, sizeof(child), "%s\\%s", path, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) total += dir_size(child);
        else total += ((unsigned long long)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return total;
}

static void save_options(void);
static void setup_gui_projection(void);
static void draw_flat_test_world(void);
static void draw_first_person_hand(void);
static void draw_ingame_world_view(int with_hand);
static void steve_part(int tex_x, int tex_y,
                       float pivot_x, float pivot_y, float pivot_z,
                       float box_x, float box_y, float box_z,
                       int box_w, int box_h, int box_d,
                       float inflate, int mirror,
                       float rot_x_deg, float rot_y_deg, float rot_z_deg);

