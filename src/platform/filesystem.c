/* Split from original monolithic main.c. Included by src/main.c unity build. */

static double now_seconds(void) {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
}

static void log_msg(const char *fmt, ...) {
    char path[MAX_PATHBUF];
    snprintf(path, sizeof(path), "%s\\menu_port.log", g_mc_dir[0] ? g_mc_dir : ".");
    FILE *f = fopen(path, "a");
    if (!f) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    fprintf(f, "\n");
    va_end(ap);
    fclose(f);
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
    ensure_dir(g_save_dir);
    ensure_dir(g_texpack_dir);
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

