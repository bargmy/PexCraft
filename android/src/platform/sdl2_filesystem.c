/* Linux/SDL2 platform filesystem/timing layer. Included only by src/main_sdl2.c. */

static double now_seconds(void) {
    static double inv_freq = 0.0;
    if (inv_freq == 0.0) inv_freq = 1.0 / (double)SDL_GetPerformanceFrequency();
    return (double)SDL_GetPerformanceCounter() * inv_freq;
}

static double profile_begin(void) { return now_seconds(); }

static void profile_add_time(PexMainThreadProfileId id, double start_time) {
    if (g_pex_profile_thread_role != PEX_PROFILE_ROLE_MAIN) return;
    if (id < 0 || id >= PROF_COUNT) return;
    double ms = (now_seconds() - start_time) * 1000.0;
    if (ms < 0.0) ms = 0.0;
    g_prof_frame_ms[id] += ms;
    if (g_loggy_enabled) g_loggy_profile_calls[id]++;
}

static void profile_begin_frame(void) {
    memset(g_prof_frame_ms, 0, sizeof(g_prof_frame_ms));
    if (g_loggy_enabled) {
        memset(g_loggy_profile_calls, 0, sizeof(g_loggy_profile_calls));
        g_loggy_gui_text_calls = 0;
        g_loggy_gui_text_chars = 0;
        g_loggy_gui_quads = 0;
        g_loggy_gui_buttons = 0;
        g_loggy_mesh_submit_calls = 0;
        g_loggy_mesh_submit_snapshot_cells = 0;
        g_loggy_mesh_installs = 0;
        g_loggy_mesh_install_vertices = 0;
        g_loggy_mesh_install_indices = 0;
        g_loggy_mesh_stale_results = 0;
        g_loggy_mesh_self_heal_refs = 0;
        g_loggy_mesh_self_heal_missing = 0;
        g_loggy_entity_remote_players = 0;
        g_loggy_entity_matrix_reads = 0;
        g_loggy_entity_matrix_skips = 0;
        g_loggy_entity_passive_entries = 0;
        g_loggy_frame_no++;
    }
    g_prof_frame_start_time = now_seconds();
    if (g_prof_accum_start_time <= 0.0) g_prof_accum_start_time = g_prof_frame_start_time;
}

static void profile_end_frame(void) {
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

static double profile_percent(double ms) {
    double denom = g_prof_display_frame_ms > 0.001 ? g_prof_display_frame_ms : g_ft_last_frame_ms;
    if (denom <= 0.001) return 0.0;
    return (ms * 100.0) / denom;
}

static void log_msg(const char *fmt, ...) { (void)fmt; }

static void path_join(char *out, size_t cap, const char *a, const char *b) {
    snprintf(out, cap, "%s/%s", a, b);
}

static int dir_exists(const char *path) {
    DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

static void ensure_dir(const char *path) {
    if (!dir_exists(path)) CreateDirectoryA(path, NULL);
}

static void init_dirs(void) {
    const char *home = getenv("HOME");
    if (home && home[0]) snprintf(g_mc_dir, sizeof(g_mc_dir), "%s/.pexcraft", home);
    else snprintf(g_mc_dir, sizeof(g_mc_dir), ".pexcraft");
    ensure_dir(g_mc_dir);
    path_join(g_save_dir, sizeof(g_save_dir), g_mc_dir, "saves");
    path_join(g_texpack_dir, sizeof(g_texpack_dir), g_mc_dir, "texturepacks");
    path_join(g_skin_dir, sizeof(g_skin_dir), g_mc_dir, "skins");
    ensure_dir(g_save_dir);
    ensure_dir(g_texpack_dir);
    ensure_dir(g_skin_dir);
}

static void delete_recursive(const char *path) {
    char norm[MAX_PATHBUF];
    pex_normalize_path(norm, sizeof(norm), path);
    DIR *d = opendir(norm);
    if (d) {
        struct dirent *de;
        while ((de = readdir(d)) != NULL) {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
            char child[MAX_PATHBUF];
            snprintf(child, sizeof(child), "%s/%s", norm, de->d_name);
            struct stat st;
            if (stat(child, &st) == 0 && S_ISDIR(st.st_mode)) delete_recursive(child);
            else unlink(child);
        }
        closedir(d);
    }
    rmdir(norm);
}

static unsigned long long dir_size(const char *path) {
    unsigned long long total = 0;
    char norm[MAX_PATHBUF];
    pex_normalize_path(norm, sizeof(norm), path);
    DIR *d = opendir(norm);
    if (!d) return 0;
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        char child[MAX_PATHBUF];
        snprintf(child, sizeof(child), "%s/%s", norm, de->d_name);
        struct stat st;
        if (stat(child, &st) == 0) {
            if (S_ISDIR(st.st_mode)) total += dir_size(child);
            else total += (unsigned long long)st.st_size;
        }
    }
    closedir(d);
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
