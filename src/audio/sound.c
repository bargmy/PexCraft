/* Beta/1.2.5 sound mapper/playback.  Normal effects are indexed from
   .pexcraft/resources/sound/..., while records are handled separately through
   the Java 1.2.5 streaming pool (.pexcraft/resources/streaming/<name>.ogg). */

typedef struct PexSoundEntry {
    char key[96];
    char path[MAX_PATHBUF];
} PexSoundEntry;

#define PEX_MAX_SOUND_ENTRIES 1024
static PexSoundEntry g_pex_sounds[PEX_MAX_SOUND_ENTRIES];
static int g_pex_sound_count = 0;
static int g_pex_sound_scanned = 0;
static double g_pex_last_sound_rescan = 0.0;
static double g_pex_last_missing_sound_notice = 0.0;
/* pex_menu_music_start_once() uses the same sound backend as effects, but the
   menu track must be stoppable when the user enters/joins a world. */
static int g_pex_menu_music_request = 0;
static int g_pex_game_music_ticks_before = -1;
static int g_pex_game_music_started = 0;

static void pex_sound_make_key_from_rel(const char *rel, char *out, size_t cap) {
    char tmp[160];
    snprintf(tmp, sizeof(tmp), "%s", rel ? rel : "");
    for (char *p = tmp; *p; ++p) if (*p == '\\') *p = '/';
    size_t n = strlen(tmp);
    if (n >= 4 && !strcmp(tmp + n - 4, ".ogg")) tmp[n - 4] = 0;
    char *base = strrchr(tmp, '/');
    char *end = tmp + strlen(tmp);
    while (end > (base ? base + 1 : tmp) && end[-1] >= '0' && end[-1] <= '9') *--end = 0;
    for (char *p = tmp; *p; ++p) if (*p == '/') *p = '.';
    snprintf(out, cap, "%s", tmp);
}

static void pex_sound_add_file(const char *root, const char *rel) {
    if (g_pex_sound_count >= PEX_MAX_SOUND_ENTRIES) return;
    if (!rel || !*rel) return;
    size_t n = strlen(rel);
    if (n < 5 || strcmp(rel + n - 4, ".ogg")) return;
    PexSoundEntry *e = &g_pex_sounds[g_pex_sound_count++];
    pex_sound_make_key_from_rel(rel, e->key, sizeof(e->key));
    path_join(e->path, sizeof(e->path), root, rel);
#if defined(_WIN32) && !defined(PEX_PLATFORM_SDL2)
    for (char *p = e->path; *p; ++p) if (*p == '/') *p = '\\';
#endif
}

#if defined(PEX_PLATFORM_XBOX_UWP)
static void pex_sound_scan_dir_recursive(const char *root, const char *rel_dir) {
    (void)root; (void)rel_dir;
}
#elif defined(PEX_PLATFORM_SDL2)
static void pex_sound_scan_dir_recursive(const char *root, const char *rel_dir) {
    char dir[MAX_PATHBUF];
    if (rel_dir && rel_dir[0]) path_join(dir, sizeof(dir), root, rel_dir);
    else snprintf(dir, sizeof(dir), "%s", root);
    char norm[MAX_PATHBUF];
    pex_normalize_path(norm, sizeof(norm), dir);
    DIR *d = opendir(norm);
    if (!d) return;
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        char rel[MAX_PATHBUF];
        if (rel_dir && rel_dir[0]) snprintf(rel, sizeof(rel), "%s/%s", rel_dir, de->d_name);
        else snprintf(rel, sizeof(rel), "%s", de->d_name);
        char full[MAX_PATHBUF];
        snprintf(full, sizeof(full), "%s/%s", norm, de->d_name);
        struct stat st;
        if (stat(full, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) pex_sound_scan_dir_recursive(root, rel);
        else pex_sound_add_file(root, rel);
    }
    closedir(d);
}
#else
static void pex_sound_scan_dir_recursive(const char *root, const char *rel_dir) {
    char dir[MAX_PATHBUF];
    if (rel_dir && rel_dir[0]) path_join(dir, sizeof(dir), root, rel_dir);
    else snprintf(dir, sizeof(dir), "%s", root);
    char pattern[MAX_PATHBUF];
    snprintf(pattern, sizeof(pattern), "%s\\*", dir);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..")) continue;
        char rel[MAX_PATHBUF];
        if (rel_dir && rel_dir[0]) snprintf(rel, sizeof(rel), "%s/%s", rel_dir, fd.cFileName);
        else snprintf(rel, sizeof(rel), "%s", fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) pex_sound_scan_dir_recursive(root, rel);
        else pex_sound_add_file(root, rel);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}
#endif

static void pex_sound_rescan(void) {
    char resources[MAX_PATHBUF], sound_root[MAX_PATHBUF];
    path_join(resources, sizeof(resources), g_mc_dir, "resources");
    path_join(sound_root, sizeof(sound_root), resources, "sound");
    g_pex_sound_count = 0;
    pex_sound_scan_dir_recursive(sound_root, "");
    g_pex_sound_scanned = 1;
    g_pex_last_sound_rescan = now_seconds();
    log_msg("Indexed %d PexCraft legacy sounds", g_pex_sound_count);
}

static const char *pex_sound_alias(const char *key) {
    if (!key) return "";
    if (!strcmp(key, "mob.chicken")) return "mob.chicken.say";
    if (!strcmp(key, "mob.chickenhurt")) return "mob.chicken.hurt";
    if (!strcmp(key, "mob.chickenplop")) return "mob.chicken.plop";
    if (!strcmp(key, "mob.cow")) return "mob.cow.say";
    if (!strcmp(key, "mob.cowhurt")) return "mob.cow.hurt";
    if (!strcmp(key, "mob.creeper")) return "mob.creeper.say";
    if (!strcmp(key, "mob.creeperdeath")) return "mob.creeper.death";
    if (!strcmp(key, "mob.pig")) return "mob.pig.say";
    if (!strcmp(key, "mob.pigdeath")) return "mob.pig.death";
    if (!strcmp(key, "mob.sheep")) return "mob.sheep.say";
    if (!strcmp(key, "mob.skeleton")) return "mob.skeleton.say";
    if (!strcmp(key, "mob.skeletonhurt")) return "mob.skeleton.hurt";
    if (!strcmp(key, "mob.slime")) return "mob.slime.small";
    if (!strcmp(key, "mob.slimeattack")) return "mob.slime.attack";
    if (!strcmp(key, "mob.spider")) return "mob.spider.say";
    if (!strcmp(key, "mob.spiderdeath")) return "mob.spider.death";
    if (!strcmp(key, "mob.zombie")) return "mob.zombie.say";
    if (!strcmp(key, "mob.zombiehurt")) return "mob.zombie.hurt";
    if (!strcmp(key, "mob.zombiedeath")) return "mob.zombie.death";
    if (!strcmp(key, "mob.villager.default")) return "mob.villager.idle";
    if (!strcmp(key, "mob.villager.defaulthurt")) return "mob.villager.hit";
    if (!strcmp(key, "mob.villager.defaultdeath")) return "mob.villager.death";
    if (!strcmp(key, "mob.snowman")) return "";
    if (!strcmp(key, "random.hurt")) return "damage.hurtflesh";
    if (!strcmp(key, "random.drr")) return "random.bowhit";
    return key;
}

static const char *pex_sound_find_file(const char *key) {
    if (!g_pex_sound_scanned || (now_seconds() - g_pex_last_sound_rescan) > 5.0) pex_sound_rescan();
    const char *want = pex_sound_alias(key);
    if (!want || !*want) return NULL;
    int matches[64];
    int count = 0;
    for (int i = 0; i < g_pex_sound_count; ++i) {
        if (!strcmp(g_pex_sounds[i].key, want)) {
            if (count < (int)ARRAY_COUNT(matches)) matches[count] = i;
            count++;
        }
    }
    if (count <= 0 && want != key) {
        want = key;
        for (int i = 0; i < g_pex_sound_count; ++i) {
            if (!strcmp(g_pex_sounds[i].key, want)) {
                if (count < (int)ARRAY_COUNT(matches)) matches[count] = i;
                count++;
            }
        }
    }
    if (count <= 0) return NULL;
    int pick = matches[rand() % (count < (int)ARRAY_COUNT(matches) ? count : (int)ARRAY_COUNT(matches))];
    return g_pex_sounds[pick].path;
}


#if defined(PEX_PLATFORM_SDL2)
typedef struct Mix_Chunk {
    int allocated;
    unsigned char *abuf;
    unsigned int alen;
    unsigned char volume;
} Mix_Chunk;
typedef int (*PFN_Mix_OpenAudio)(int, unsigned short, int, int);
typedef int (*PFN_Mix_Init)(int);
typedef void (*PFN_Mix_Quit)(void);
typedef void (*PFN_Mix_CloseAudio)(void);
typedef int (*PFN_Mix_AllocateChannels)(int);
typedef Mix_Chunk *(*PFN_Mix_LoadWAV_RW)(SDL_RWops *, int);
typedef struct Mix_Music Mix_Music;
typedef Mix_Music *(*PFN_Mix_LoadMUS)(const char *);
typedef int (*PFN_Mix_PlayMusic)(Mix_Music *, int);
typedef int (*PFN_Mix_HaltMusic)(void);
typedef int (*PFN_Mix_VolumeMusic)(int);
typedef void (*PFN_Mix_FreeMusic)(Mix_Music *);
typedef int (*PFN_Mix_PlayChannel)(int, Mix_Chunk *, int);
typedef int (*PFN_Mix_HaltChannel)(int);
typedef int (*PFN_Mix_VolumeChunk)(Mix_Chunk *, int);
typedef int (*PFN_Mix_Volume)(int, int);
typedef int (*PFN_Mix_SetPosition)(int, Sint16, Uint8);
typedef int (*PFN_Mix_Playing)(int);
typedef void (*PFN_Mix_FreeChunk)(Mix_Chunk *);
static void *g_mix_lib = NULL;
static PFN_Mix_OpenAudio pMix_OpenAudio = NULL;
static PFN_Mix_Init pMix_Init = NULL;
static PFN_Mix_Quit pMix_Quit = NULL;
static PFN_Mix_CloseAudio pMix_CloseAudio = NULL;
static PFN_Mix_AllocateChannels pMix_AllocateChannels = NULL;
static PFN_Mix_LoadWAV_RW pMix_LoadWAV_RW = NULL;
static PFN_Mix_LoadMUS pMix_LoadMUS = NULL;
static PFN_Mix_PlayMusic pMix_PlayMusic = NULL;
static PFN_Mix_HaltMusic pMix_HaltMusic = NULL;
static PFN_Mix_VolumeMusic pMix_VolumeMusic = NULL;
static PFN_Mix_FreeMusic pMix_FreeMusic = NULL;
static PFN_Mix_PlayChannel pMix_PlayChannel = NULL;
static PFN_Mix_HaltChannel pMix_HaltChannel = NULL;
static PFN_Mix_VolumeChunk pMix_VolumeChunk = NULL;
static PFN_Mix_Volume pMix_Volume = NULL;
static PFN_Mix_SetPosition pMix_SetPosition = NULL;
static PFN_Mix_Playing pMix_Playing = NULL;
static PFN_Mix_FreeChunk pMix_FreeChunk = NULL;
static int g_mix_ready = 0;
static int g_pex_menu_music_channel = -1;
static Mix_Music *g_pex_record_music = NULL; /* legacy cleanup for old builds that used Mix_Music. */
static int g_pex_record_channel = -1;
static float g_pex_record_x = 0.0f, g_pex_record_y = 0.0f, g_pex_record_z = 0.0f;
static float g_pex_record_volume_arg = 1.0f;
static int g_pex_record_active = 0;
static void sound_backend_stop_menu_music(void);

typedef struct PexSoundChunkCache { char path[MAX_PATHBUF]; Mix_Chunk *chunk; } PexSoundChunkCache;
#define PEX_SOUND_CHUNK_CACHE_MAX 128
static PexSoundChunkCache g_sound_chunk_cache[PEX_SOUND_CHUNK_CACHE_MAX];
static int g_sound_chunk_cache_count = 0;

typedef struct PexSoundPitchChunkCache {
    char path[MAX_PATHBUF];
    int pitch_q;
    Mix_Chunk chunk;
} PexSoundPitchChunkCache;
#define PEX_SOUND_PITCH_CACHE_MAX 96
static PexSoundPitchChunkCache g_sound_pitch_cache[PEX_SOUND_PITCH_CACHE_MAX];
static int g_sound_pitch_cache_count = 0;

static float pex_sound_clamp_pitch(float pitch) {
    if (pitch < 0.50f) pitch = 0.50f;
    if (pitch > 2.00f) pitch = 2.00f;
    return pitch;
}

static void *pex_sdl_load_mixer_symbol(const char *name) {
    return g_mix_lib ? SDL_LoadFunction(g_mix_lib, name) : NULL;
}

static int pex_sound_backend_init(void) {
    if (g_mix_ready) return 1;
    if (!g_mix_lib) {
        const char *libs[] = {
#if defined(_WIN32)
            "SDL2_mixer.dll", "libSDL2_mixer-2.0-0.dll", "SDL2_mixer",
#elif defined(__APPLE__)
            "libSDL2_mixer-2.0.0.dylib", "libSDL2_mixer.dylib",
#else
            "libSDL2_mixer-2.0.so.0", "libSDL2_mixer.so",
#endif
            NULL
        };
        for (int i = 0; libs[i] && !g_mix_lib; ++i) g_mix_lib = SDL_LoadObject(libs[i]);
        if (!g_mix_lib) { log_msg("Sound backend: SDL2_mixer not found"); return 0; }
        pMix_OpenAudio = (PFN_Mix_OpenAudio)pex_sdl_load_mixer_symbol("Mix_OpenAudio");
        pMix_Init = (PFN_Mix_Init)pex_sdl_load_mixer_symbol("Mix_Init");
        pMix_Quit = (PFN_Mix_Quit)pex_sdl_load_mixer_symbol("Mix_Quit");
        pMix_CloseAudio = (PFN_Mix_CloseAudio)pex_sdl_load_mixer_symbol("Mix_CloseAudio");
        pMix_AllocateChannels = (PFN_Mix_AllocateChannels)pex_sdl_load_mixer_symbol("Mix_AllocateChannels");
        pMix_LoadWAV_RW = (PFN_Mix_LoadWAV_RW)pex_sdl_load_mixer_symbol("Mix_LoadWAV_RW");
        pMix_LoadMUS = (PFN_Mix_LoadMUS)pex_sdl_load_mixer_symbol("Mix_LoadMUS");
        pMix_PlayMusic = (PFN_Mix_PlayMusic)pex_sdl_load_mixer_symbol("Mix_PlayMusic");
        pMix_HaltMusic = (PFN_Mix_HaltMusic)pex_sdl_load_mixer_symbol("Mix_HaltMusic");
        pMix_VolumeMusic = (PFN_Mix_VolumeMusic)pex_sdl_load_mixer_symbol("Mix_VolumeMusic");
        pMix_FreeMusic = (PFN_Mix_FreeMusic)pex_sdl_load_mixer_symbol("Mix_FreeMusic");
        pMix_PlayChannel = (PFN_Mix_PlayChannel)pex_sdl_load_mixer_symbol("Mix_PlayChannel");
        pMix_HaltChannel = (PFN_Mix_HaltChannel)pex_sdl_load_mixer_symbol("Mix_HaltChannel");
        pMix_VolumeChunk = (PFN_Mix_VolumeChunk)pex_sdl_load_mixer_symbol("Mix_VolumeChunk");
        pMix_Volume = (PFN_Mix_Volume)pex_sdl_load_mixer_symbol("Mix_Volume");
        pMix_SetPosition = (PFN_Mix_SetPosition)pex_sdl_load_mixer_symbol("Mix_SetPosition");
        pMix_Playing = (PFN_Mix_Playing)pex_sdl_load_mixer_symbol("Mix_Playing");
        pMix_FreeChunk = (PFN_Mix_FreeChunk)pex_sdl_load_mixer_symbol("Mix_FreeChunk");
        if (!pMix_OpenAudio || !pMix_LoadWAV_RW || !pMix_PlayChannel) { log_msg("Sound backend: SDL2_mixer missing required symbols"); return 0; }
    }
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    if (pMix_Init) pMix_Init(0x00000002 | 0x00000008 | 0x00000010); /* MOD/MP3/OGG where supported. */
    if (pMix_OpenAudio(44100, AUDIO_S16SYS, 2, 1024) != 0) { log_msg("Sound backend: Mix_OpenAudio failed"); return 0; }
    if (pMix_AllocateChannels) pMix_AllocateChannels(32);
    g_mix_ready = 1;
    return 1;
}


static int pex_path_has_ext_ci(const char *path, const char *ext) {
    if (!path || !ext) return 0;
    size_t np = strlen(path), ne = strlen(ext);
    if (np < ne) return 0;
    const char *p = path + np - ne;
    for (size_t i = 0; i < ne; ++i) {
        char a = p[i], b = ext[i];
        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
        if (a != b) return 0;
    }
    return 1;
}

static Mix_Chunk *pex_sound_decode_mp3_chunk_mpg123(const char *path) {
#if defined(PEX_PLATFORM_SDL2)
    typedef struct mpg123_handle mpg123_handle;
    typedef int (*PFN_mpg123_init)(void);
    typedef void (*PFN_mpg123_exit)(void);
    typedef mpg123_handle *(*PFN_mpg123_new)(const char *, int *);
    typedef void (*PFN_mpg123_delete)(mpg123_handle *);
    typedef int (*PFN_mpg123_open)(mpg123_handle *, const char *);
    typedef int (*PFN_mpg123_getformat)(mpg123_handle *, long *, int *, int *);
    typedef int (*PFN_mpg123_format_none)(mpg123_handle *);
    typedef int (*PFN_mpg123_format)(mpg123_handle *, long, int, int);
    typedef int (*PFN_mpg123_read)(mpg123_handle *, unsigned char *, size_t, size_t *);
    typedef int (*PFN_mpg123_close)(mpg123_handle *);
    typedef const char *(*PFN_mpg123_plain_strerror)(int);
    enum { PEX_MPG123_OK = 0, PEX_MPG123_NEW_FORMAT = -11, PEX_MPG123_DONE = -12, PEX_MPG123_STEREO = 2, PEX_MPG123_ENC_SIGNED_16 = 0x040 };
    void *lib = NULL;
    const char *libs[] = { "libmpg123.so.0", "libmpg123.so", NULL };
    for (int i = 0; libs[i] && !lib; ++i) lib = SDL_LoadObject(libs[i]);
    if (!lib) return NULL;
    PFN_mpg123_init f_init = (PFN_mpg123_init)SDL_LoadFunction(lib, "mpg123_init");
    PFN_mpg123_exit f_exit = (PFN_mpg123_exit)SDL_LoadFunction(lib, "mpg123_exit");
    PFN_mpg123_new f_new = (PFN_mpg123_new)SDL_LoadFunction(lib, "mpg123_new");
    PFN_mpg123_delete f_delete = (PFN_mpg123_delete)SDL_LoadFunction(lib, "mpg123_delete");
    PFN_mpg123_open f_open = (PFN_mpg123_open)SDL_LoadFunction(lib, "mpg123_open");
    PFN_mpg123_getformat f_getformat = (PFN_mpg123_getformat)SDL_LoadFunction(lib, "mpg123_getformat");
    PFN_mpg123_format_none f_format_none = (PFN_mpg123_format_none)SDL_LoadFunction(lib, "mpg123_format_none");
    PFN_mpg123_format f_format = (PFN_mpg123_format)SDL_LoadFunction(lib, "mpg123_format");
    PFN_mpg123_read f_read = (PFN_mpg123_read)SDL_LoadFunction(lib, "mpg123_read");
    PFN_mpg123_close f_close = (PFN_mpg123_close)SDL_LoadFunction(lib, "mpg123_close");
    PFN_mpg123_plain_strerror f_err = (PFN_mpg123_plain_strerror)SDL_LoadFunction(lib, "mpg123_plain_strerror");
    if (!f_init || !f_exit || !f_new || !f_delete || !f_open || !f_getformat || !f_format_none || !f_format || !f_read || !f_close) {
        SDL_UnloadObject(lib);
        return NULL;
    }
    if (f_init() != PEX_MPG123_OK) { SDL_UnloadObject(lib); return NULL; }
    int err = 0;
    mpg123_handle *mh = f_new(NULL, &err);
    if (!mh) { f_exit(); SDL_UnloadObject(lib); return NULL; }
    f_format_none(mh);
    f_format(mh, 44100, PEX_MPG123_STEREO, PEX_MPG123_ENC_SIGNED_16);
    if (f_open(mh, path) != PEX_MPG123_OK) {
        log_msg("Record backend: mpg123_open failed for %s", path);
        f_delete(mh); f_exit(); SDL_UnloadObject(lib); return NULL;
    }
    long rate = 0; int channels = 0, enc = 0;
    f_getformat(mh, &rate, &channels, &enc);
    if (rate != 44100 || channels != PEX_MPG123_STEREO || enc != PEX_MPG123_ENC_SIGNED_16) {
        /* SDL_mixer channels use the mixer format opened above: 44100Hz signed 16-bit stereo. */
        log_msg("Record backend: mpg123 decoded unsupported format rate=%ld channels=%d enc=%d for %s", rate, channels, enc, path);
        f_close(mh); f_delete(mh); f_exit(); SDL_UnloadObject(lib); return NULL;
    }
    size_t cap = 262144, len = 0;
    unsigned char *pcm = (unsigned char *)malloc(cap);
    if (!pcm) { f_close(mh); f_delete(mh); f_exit(); SDL_UnloadObject(lib); return NULL; }
    unsigned char buf[32768];
    for (;;) {
        size_t got = 0;
        int r = f_read(mh, buf, sizeof(buf), &got);
        if (got > 0) {
            if (len + got > cap) {
                size_t ncap = cap * 2;
                while (ncap < len + got) ncap *= 2;
                unsigned char *np = (unsigned char *)realloc(pcm, ncap);
                if (!np) { free(pcm); f_close(mh); f_delete(mh); f_exit(); SDL_UnloadObject(lib); return NULL; }
                pcm = np; cap = ncap;
            }
            memcpy(pcm + len, buf, got);
            len += got;
        }
        if (r == PEX_MPG123_DONE) break;
        if (r == PEX_MPG123_NEW_FORMAT || r == PEX_MPG123_OK) continue;
        log_msg("Record backend: mpg123_read failed for %s%s%s", path, f_err ? ": " : "", f_err ? f_err(r) : "");
        free(pcm); f_close(mh); f_delete(mh); f_exit(); SDL_UnloadObject(lib); return NULL;
    }
    f_close(mh); f_delete(mh); f_exit(); SDL_UnloadObject(lib);
    if (len == 0) { free(pcm); return NULL; }
    Mix_Chunk *chunk = (Mix_Chunk *)malloc(sizeof(Mix_Chunk));
    if (!chunk) { free(pcm); return NULL; }
    chunk->allocated = 1;
    chunk->abuf = pcm;
    chunk->alen = (unsigned int)len;
    chunk->volume = 128;
    log_msg("Record backend: decoded MP3 through libmpg123: %s", path);
    return chunk;
#else
    (void)path;
    return NULL;
#endif
}

static Mix_Chunk *pex_sound_load_chunk_cached(const char *path) {
    if (!path || !*path) return NULL;
    for (int i = 0; i < g_sound_chunk_cache_count; ++i) if (!strcmp(g_sound_chunk_cache[i].path, path)) return g_sound_chunk_cache[i].chunk;
    if (!pex_sound_backend_init()) return NULL;
    SDL_RWops *rw = SDL_RWFromFile(path, "rb");
    if (!rw) return NULL;
    Mix_Chunk *c = pMix_LoadWAV_RW(rw, 1);
    if (!c && pex_path_has_ext_ci(path, ".mp3")) c = pex_sound_decode_mp3_chunk_mpg123(path);
    if (!c) return NULL;
    int slot = g_sound_chunk_cache_count;
    if (slot >= PEX_SOUND_CHUNK_CACHE_MAX) {
        slot = rand() % PEX_SOUND_CHUNK_CACHE_MAX;
        if (pMix_FreeChunk && g_sound_chunk_cache[slot].chunk) pMix_FreeChunk(g_sound_chunk_cache[slot].chunk);
    } else {
        g_sound_chunk_cache_count++;
    }
    snprintf(g_sound_chunk_cache[slot].path, sizeof(g_sound_chunk_cache[slot].path), "%s", path);
    g_sound_chunk_cache[slot].chunk = c;
    return c;
}

static Mix_Chunk *pex_sound_get_pitched_chunk(const char *path, Mix_Chunk *base, float pitch) {
    pitch = pex_sound_clamp_pitch(pitch);
    if (!base || !base->abuf || base->alen < 4 || fabsf(pitch - 1.0f) < 0.015f) return base;
    int q = (int)floorf(pitch * 100.0f + 0.5f);
    for (int i = 0; i < g_sound_pitch_cache_count; ++i) {
        if (g_sound_pitch_cache[i].pitch_q == q && !strcmp(g_sound_pitch_cache[i].path, path)) return &g_sound_pitch_cache[i].chunk;
    }

    const unsigned int frame_bytes = 4; /* Mix_OpenAudio uses signed 16-bit stereo. */
    unsigned int src_frames = base->alen / frame_bytes;
    if (src_frames < 2) return base;
    unsigned int dst_frames = (unsigned int)((float)src_frames / pitch);
    if (dst_frames < 2) dst_frames = 2;
    unsigned int dst_bytes = dst_frames * frame_bytes;
    unsigned char *buf = (unsigned char *)malloc(dst_bytes);
    if (!buf) return base;
    for (unsigned int i = 0; i < dst_frames; ++i) {
        unsigned int si = (unsigned int)((float)i * pitch);
        if (si >= src_frames) si = src_frames - 1;
        memcpy(buf + i * frame_bytes, base->abuf + si * frame_bytes, frame_bytes);
    }

    int slot = g_sound_pitch_cache_count;
    if (slot >= PEX_SOUND_PITCH_CACHE_MAX) {
        slot = rand() % PEX_SOUND_PITCH_CACHE_MAX;
        free(g_sound_pitch_cache[slot].chunk.abuf);
    } else {
        g_sound_pitch_cache_count++;
    }
    memset(&g_sound_pitch_cache[slot], 0, sizeof(g_sound_pitch_cache[slot]));
    snprintf(g_sound_pitch_cache[slot].path, sizeof(g_sound_pitch_cache[slot].path), "%s", path);
    g_sound_pitch_cache[slot].pitch_q = q;
    g_sound_pitch_cache[slot].chunk.allocated = 1;
    g_sound_pitch_cache[slot].chunk.abuf = buf;
    g_sound_pitch_cache[slot].chunk.alen = dst_bytes;
    g_sound_pitch_cache[slot].chunk.volume = 128;
    return &g_sound_pitch_cache[slot].chunk;
}

static int pex_sound_backend_play_file(const char *path, float volume, float pitch) {
    Mix_Chunk *base = pex_sound_load_chunk_cached(path);
    Mix_Chunk *c = pex_sound_get_pitched_chunk(path, base, pitch);
    if (!c) return 0;
    int v = (int)(volume * 128.0f);
    if (v < 0) v = 0;
    if (v > 128) v = 128;
    if (pMix_VolumeChunk) pMix_VolumeChunk(c, pMix_Volume ? 128 : v);
    if (g_pex_menu_music_request && pMix_HaltChannel && g_pex_menu_music_channel >= 0) {
        pMix_HaltChannel(g_pex_menu_music_channel);
        g_pex_menu_music_channel = -1;
    }
    int ch = pMix_PlayChannel(-1, c, 0);
    if (ch >= 0 && pMix_Volume) pMix_Volume(ch, v);
    if (g_pex_menu_music_request) g_pex_menu_music_channel = ch;
    return ch >= 0;
}

static void pex_sdl_record_update_position(void) {
    if (!g_pex_record_active || g_pex_record_channel < 0 || !pMix_SetPosition) return;
    if (pMix_Playing && !pMix_Playing(g_pex_record_channel)) {
        g_pex_record_active = 0;
        g_pex_record_channel = -1;
        return;
    }
    float dx = g_pex_record_x - g_player_x;
    float dz = g_pex_record_z - g_player_z;
    float dy = g_pex_record_y - g_player_y;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);
    float yaw = g_player_yaw * (float)M_PI / 180.0f;
    float fwd_x = -sinf(yaw), fwd_z = cosf(yaw);
    float right_x = cosf(yaw), right_z = sinf(yaw);
    float rel_fwd = dx * fwd_x + dz * fwd_z;
    float rel_right = dx * right_x + dz * right_z;
    float angle_f = atan2f(rel_right, rel_fwd) * 180.0f / (float)M_PI;
    if (angle_f < 0.0f) angle_f += 360.0f;
    int angle_i = (int)(angle_f + 0.5f);
    if (angle_i >= 360) angle_i -= 360;
    int distance_i = (int)(dist * 255.0f / 64.0f + 0.5f);
    if (distance_i < 0) distance_i = 0;
    if (distance_i > 255) distance_i = 255;
    int vol = (int)(0.5f * g_opts.sound * 128.0f + 0.5f);
    (void)g_pex_record_volume_arg; /* 1.2.5 passes 1.0F; SoundManager fixes records to 0.5 * soundVolume. */
    if (vol < 0) vol = 0;
    if (vol > 128) vol = 128;
    if (pMix_Volume) pMix_Volume(g_pex_record_channel, vol);
    pMix_SetPosition(g_pex_record_channel, (Sint16)angle_i, (Uint8)distance_i);
}

static void pex_sound_backend_stop_record(void) {
    if (g_pex_record_channel >= 0 && pMix_HaltChannel) pMix_HaltChannel(g_pex_record_channel);
    g_pex_record_channel = -1;
    g_pex_record_active = 0;
    if (pMix_HaltMusic) pMix_HaltMusic(); /* cleanup for older PexCraft builds that used Mix_Music for records. */
    if (g_pex_record_music && pMix_FreeMusic) pMix_FreeMusic(g_pex_record_music);
    g_pex_record_music = NULL;
}

static int pex_sound_backend_play_record_file_at(const char *path, float x, float y, float z, float volume) {
    if (!path || !*path) return 0;
    if (g_opts.sound <= 0.001f) { pex_sound_backend_stop_record(); return 0; }
    if (!pex_sound_backend_init()) return 0;
    if (!pMix_SetPosition) {
        log_msg("Record backend: Mix_SetPosition missing; refusing non-positional record playback");
        return 0;
    }
    Mix_Chunk *c = pex_sound_load_chunk_cached(path);
    if (!c) { log_msg("Record backend: Mix_LoadWAV_RW failed for %s", path); return 0; }
    pex_sound_backend_stop_record();
    sound_backend_stop_menu_music();
    int ch = pMix_PlayChannel(-1, c, 0);
    if (ch < 0) { log_msg("Record backend: Mix_PlayChannel failed for %s", path); return 0; }
    g_pex_record_channel = ch;
    g_pex_record_x = x;
    g_pex_record_y = y;
    g_pex_record_z = z;
    g_pex_record_volume_arg = volume;
    g_pex_record_active = 1;
    pex_sdl_record_update_position();
    return 1;
}

static void pex_sound_backend_tick_record(void) {
    pex_sdl_record_update_position();
}

static void sound_backend_stop_menu_music(void) {
    if (pMix_HaltChannel && g_pex_menu_music_channel >= 0) {
        pMix_HaltChannel(g_pex_menu_music_channel);
    }
    g_pex_menu_music_channel = -1;
}

static void sound_backend_stop_all_audio(void) {
    /* Mix_HaltChannel(-1) stops every effect/stream channel, including sounds
       that are not individually tracked by the gameplay layer. */
    if (pMix_HaltChannel) pMix_HaltChannel(-1);
    if (pMix_HaltMusic) pMix_HaltMusic();
    g_pex_menu_music_channel = -1;
    g_pex_record_channel = -1;
    g_pex_record_active = 0;
    if (g_pex_record_music && pMix_FreeMusic) pMix_FreeMusic(g_pex_record_music);
    g_pex_record_music = NULL;
}

static void pex_sound_shutdown(void) {
    sound_backend_stop_all_audio();
    for (int i = 0; i < g_sound_pitch_cache_count; ++i) {
        free(g_sound_pitch_cache[i].chunk.abuf);
        g_sound_pitch_cache[i].chunk.abuf = NULL;
    }
    g_sound_pitch_cache_count = 0;
    for (int i = 0; i < g_sound_chunk_cache_count; ++i) {
        if (pMix_FreeChunk && g_sound_chunk_cache[i].chunk) pMix_FreeChunk(g_sound_chunk_cache[i].chunk);
        g_sound_chunk_cache[i].chunk = NULL;
    }
    g_sound_chunk_cache_count = 0;
    if (g_mix_ready && pMix_CloseAudio) pMix_CloseAudio();
    if (pMix_Quit) pMix_Quit();
    if (g_mix_lib) SDL_UnloadObject(g_mix_lib);
    g_mix_lib = NULL;
    g_mix_ready = 0;
}
#elif defined(_WIN32) && !defined(PEX_PLATFORM_XBOX_UWP)
/* Windows uses libvorbisfile directly.  Do not use MCI or Media Foundation for
   .ogg: they are codec-dependent on many Windows installs and silently fail.
   The CI workflow installs mingw-w64 libogg/libvorbis and packages/link them. */
#include <vorbis/vorbisfile.h>
#include <mmreg.h>

typedef struct PexWinDecodedSound {
    char path[MAX_PATHBUF];
    BYTE *data;
    DWORD bytes;
    WAVEFORMATEX fmt;
} PexWinDecodedSound;

#define PEX_WIN_SOUND_CACHE_MAX 160
static PexWinDecodedSound g_win_sound_cache[PEX_WIN_SOUND_CACHE_MAX];
static int g_win_sound_cache_count = 0;

typedef struct PexWinPlayJob {
    BYTE *data;
    DWORD bytes;
    WAVEFORMATEX fmt;
    float volume;
    int is_menu_music;
    LONG menu_generation;
    LONG session_generation;
} PexWinPlayJob;

static volatile LONG g_pex_menu_music_stop_generation = 0;
static volatile LONG g_pex_sound_session_generation = 0;

static int pex_win_pcm_append(BYTE **data, DWORD *bytes, DWORD *cap, const char *src, long src_bytes) {
    DWORD need;
    BYTE *next;
    if (!data || !bytes || !cap || !src || src_bytes <= 0) return 1;
    if (*bytes > 0x7fffffffU - (DWORD)src_bytes) return 0;
    need = *bytes + (DWORD)src_bytes;
    if (need > *cap) {
        DWORD new_cap = *cap ? (*cap * 2U) : 65536U;
        while (new_cap < need) {
            if (new_cap > 0x40000000U) { new_cap = need; break; }
            new_cap *= 2U;
        }
        next = (BYTE *)realloc(*data, new_cap);
        if (!next) return 0;
        *data = next;
        *cap = new_cap;
    }
    memcpy(*data + *bytes, src, (size_t)src_bytes);
    *bytes = need;
    return 1;
}

static void win_pcm_apply_volume_16(BYTE *data, DWORD bytes, float volume) {
    if (!data || bytes == 0) return;
    if (volume > 0.999f && volume < 1.001f) return;
    short *s = (short *)data;
    DWORD count = bytes / 2U;
    for (DWORD i = 0; i < count; ++i) {
        int v = (int)((float)s[i] * volume);
        if (v < -32768) v = -32768;
        if (v > 32767) v = 32767;
        s[i] = (short)v;
    }
}

static int pex_win_decode_ogg_to_pcm(const char *path, PexWinDecodedSound *out) {
    OggVorbis_File vf;
    vorbis_info *vi;
    char tmp[32768];
    int section = 0;
    DWORD cap = 0;
    int ok = 0;

    if (!path || !*path || !out) return 0;
    memset(out, 0, sizeof(*out));

    if (ov_fopen((char *)path, &vf) != 0) {
        log_msg("Sound backend: libvorbis could not open OGG: %s", path);
        return 0;
    }

    vi = ov_info(&vf, -1);
    if (!vi || vi->channels <= 0 || vi->rate <= 0) {
        log_msg("Sound backend: invalid Vorbis stream: %s", path);
        goto done;
    }

    out->fmt.wFormatTag = WAVE_FORMAT_PCM;
    out->fmt.nChannels = (WORD)vi->channels;
    out->fmt.nSamplesPerSec = (DWORD)vi->rate;
    out->fmt.wBitsPerSample = 16;
    out->fmt.nBlockAlign = (WORD)(out->fmt.nChannels * out->fmt.wBitsPerSample / 8);
    out->fmt.nAvgBytesPerSec = out->fmt.nSamplesPerSec * out->fmt.nBlockAlign;
    out->fmt.cbSize = 0;

    for (;;) {
        long got = ov_read(&vf, tmp, (int)sizeof(tmp), 0, 2, 1, &section);
        if (got == 0) break;
        if (got < 0) continue; /* tolerate small decode holes */
        if (!pex_win_pcm_append(&out->data, &out->bytes, &cap, tmp, got)) goto done;
    }

    ok = out->data && out->bytes > 0;
    if (ok) snprintf(out->path, sizeof(out->path), "%s", path);

done:
    ov_clear(&vf);
    if (!ok) {
        free(out->data);
        memset(out, 0, sizeof(*out));
    }
    return ok;
}

static PexWinDecodedSound *pex_win_get_decoded_sound(const char *path) {
    if (!path || !*path) return NULL;
    for (int i = 0; i < g_win_sound_cache_count; ++i) {
        if (!strcmp(g_win_sound_cache[i].path, path)) return &g_win_sound_cache[i];
    }

    int slot = g_win_sound_cache_count;
    if (slot >= PEX_WIN_SOUND_CACHE_MAX) {
        slot = rand() % PEX_WIN_SOUND_CACHE_MAX;
        free(g_win_sound_cache[slot].data);
        memset(&g_win_sound_cache[slot], 0, sizeof(g_win_sound_cache[slot]));
    } else {
        g_win_sound_cache_count++;
    }

    if (!pex_win_decode_ogg_to_pcm(path, &g_win_sound_cache[slot])) {
        if (slot == g_win_sound_cache_count - 1) g_win_sound_cache_count--;
        return NULL;
    }
    return &g_win_sound_cache[slot];
}

static int pex_win_waveout_play_buffer(BYTE *data, DWORD bytes, const WAVEFORMATEX *fmt, float volume,
                                       int is_menu_music, LONG menu_generation, LONG session_generation) {
    HWAVEOUT hwo = NULL;
    WAVEHDR hdr;
    MMRESULT mm;
    if (!data || bytes == 0 || !fmt) return 0;
    win_pcm_apply_volume_16(data, bytes, volume);
    memset(&hdr, 0, sizeof(hdr));
    hdr.lpData = (LPSTR)data;
    hdr.dwBufferLength = bytes;
    mm = waveOutOpen(&hwo, WAVE_MAPPER, fmt, 0, 0, CALLBACK_NULL);
    if (mm != MMSYSERR_NOERROR) return 0;
    if (waveOutPrepareHeader(hwo, &hdr, sizeof(hdr)) != MMSYSERR_NOERROR) { waveOutClose(hwo); return 0; }
    if (waveOutWrite(hwo, &hdr, sizeof(hdr)) != MMSYSERR_NOERROR) {
        waveOutUnprepareHeader(hwo, &hdr, sizeof(hdr));
        waveOutClose(hwo);
        return 0;
    }
    while (!(hdr.dwFlags & WHDR_DONE)) {
        /* Detached waveOut jobs cannot be joined or addressed by channel.  A
           generation mismatch is the session-wide stop signal used when a
           world is torn down.  Newly started title music captures the new
           generation and is therefore not cancelled with the old world. */
        if (session_generation != g_pex_sound_session_generation ||
            (is_menu_music && menu_generation != g_pex_menu_music_stop_generation)) {
            waveOutReset(hwo);
            break;
        }
        Sleep(5);
    }
    waveOutUnprepareHeader(hwo, &hdr, sizeof(hdr));
    waveOutClose(hwo);
    return 1;
}

static DWORD WINAPI pex_win_play_thread(LPVOID arg) {
    PexWinPlayJob *job = (PexWinPlayJob *)arg;
    if (!job) return 0;
    if (!pex_win_waveout_play_buffer(job->data, job->bytes, &job->fmt, job->volume,
                                     job->is_menu_music, job->menu_generation,
                                     job->session_generation)) {
        log_msg("Sound backend: waveOut failed to play PCM sound");
    }
    free(job->data);
    free(job);
    return 0;
}

static int pex_sound_backend_play_file(const char *path, float volume, float pitch) {
    if (!path || !*path) return 0;
    PexWinDecodedSound *cached = pex_win_get_decoded_sound(path);
    if (!cached || !cached->data || cached->bytes == 0) return 0;

    PexWinPlayJob *job = (PexWinPlayJob *)calloc(1, sizeof(*job));
    if (!job) return 0;
    job->data = (BYTE *)malloc(cached->bytes);
    if (!job->data) { free(job); return 0; }
    memcpy(job->data, cached->data, cached->bytes);
    job->bytes = cached->bytes;
    job->fmt = cached->fmt;
    if (pitch < 0.50f) pitch = 0.50f;
    if (pitch > 2.00f) pitch = 2.00f;
    if (fabsf(pitch - 1.0f) > 0.015f) {
        DWORD sr = (DWORD)((float)job->fmt.nSamplesPerSec * pitch);
        if (sr < 4000) sr = 4000;
        if (sr > 192000) sr = 192000;
        job->fmt.nSamplesPerSec = sr;
        job->fmt.nAvgBytesPerSec = job->fmt.nSamplesPerSec * job->fmt.nBlockAlign;
    }
    job->volume = volume;
    job->is_menu_music = g_pex_menu_music_request ? 1 : 0;
    job->menu_generation = g_pex_menu_music_stop_generation;
    job->session_generation = g_pex_sound_session_generation;

    HANDLE th = CreateThread(NULL, 0, pex_win_play_thread, job, 0, NULL);
    if (!th) { free(job->data); free(job); return 0; }
    CloseHandle(th);
    return 1;
}

static void pex_sound_backend_stop_record(void) { }
static int pex_sound_backend_play_record_file_at(const char *path, float x, float y, float z, float volume) {
    (void)path; (void)x; (void)y; (void)z; (void)volume;
    return 0;
}
static void pex_sound_backend_tick_record(void) { }

static void sound_backend_stop_menu_music(void) {
    InterlockedIncrement((volatile LONG *)&g_pex_menu_music_stop_generation);
}

static void sound_backend_stop_all_audio(void) {
    InterlockedIncrement((volatile LONG *)&g_pex_sound_session_generation);
    InterlockedIncrement((volatile LONG *)&g_pex_menu_music_stop_generation);
}

static void pex_sound_shutdown(void) {
    sound_backend_stop_all_audio();
    for (int i = 0; i < g_win_sound_cache_count; ++i) {
        free(g_win_sound_cache[i].data);
        g_win_sound_cache[i].data = NULL;
    }
    g_win_sound_cache_count = 0;
}
#else
static int pex_sound_backend_play_file(const char *path, float volume, float pitch) {
    (void)path; (void)volume; (void)pitch;
    return 0;
}
static void pex_sound_backend_stop_record(void) { }
static int pex_sound_backend_play_record_file_at(const char *path, float x, float y, float z, float volume) { (void)path; (void)x; (void)y; (void)z; (void)volume; return 0; }
static void pex_sound_backend_tick_record(void) { }
static void sound_backend_stop_menu_music(void) { }
static void sound_backend_stop_all_audio(void) { }
static void pex_sound_shutdown(void) { }
#endif


#if defined(PEX_PLATFORM_SDL2)
static int pex_sdl_can_load_one_of(const char **libs) {
    void *lib = NULL;
    if (!libs) return 0;
    for (int i = 0; libs[i] && !lib; ++i) lib = SDL_LoadObject(libs[i]);
    if (lib) {
        SDL_UnloadObject(lib);
        return 1;
    }
    return 0;
}

static int pex_sound_record_dependency_report(char *out, size_t cap) {
    int missing = 0;
    char line[MAX_LABEL * 2];
    if (out && cap > 0) out[0] = 0;
    const char *mix_libs[] = {
#if defined(_WIN32)
        "SDL2_mixer.dll", "libSDL2_mixer-2.0-0.dll", "SDL2_mixer",
#elif defined(__APPLE__)
        "libSDL2_mixer-2.0.0.dylib", "libSDL2_mixer.dylib",
#else
        "libSDL2_mixer-2.0.so.0", "libSDL2_mixer.so",
#endif
        NULL
    };
    const char *mpg_libs[] = {
#if defined(_WIN32)
        "libmpg123-0.dll", "mpg123.dll",
#elif defined(__APPLE__)
        "libmpg123.0.dylib", "libmpg123.dylib",
#else
        "libmpg123.so.0", "libmpg123.so",
#endif
        NULL
    };
    if (!pex_sdl_can_load_one_of(mix_libs)) {
        snprintf(line, sizeof(line), "Missing SDL2_mixer (install libsdl2-mixer-2.0-0). ");
        if (out && cap > 0) strncat(out, line, cap - strlen(out) - 1);
        missing = 1;
    }
    if (!pex_sdl_can_load_one_of(mpg_libs)) {
        snprintf(line, sizeof(line), "Missing libmpg123 for MP3 records. ");
        if (out && cap > 0) strncat(out, line, cap - strlen(out) - 1);
        missing = 1;
    }
    return missing ? 0 : 1;
}
#else
static int pex_sound_record_dependency_report(char *out, size_t cap) { if (out && cap > 0) out[0] = 0; return 1; }
#endif


static int pex_background_music_playing(void) {
#if defined(PEX_PLATFORM_SDL2)
    if (g_pex_menu_music_channel < 0) return 0;
    if (pMix_Playing && !pMix_Playing(g_pex_menu_music_channel)) {
        g_pex_menu_music_channel = -1;
        return 0;
    }
    return 1;
#else
    return g_pex_game_music_started || g_menu_music_started;
#endif
}

static void pex_pick_music_file_from_list(const char **names, int count, const char *base_dir, char *out, size_t cap) {
    if (!out || cap == 0) return;
    out[0] = 0;
    if (!names || count <= 0 || !base_dir || !*base_dir) return;
    int start = rand() % count;
    for (int i = 0; i < count; ++i) {
        const char *name = names[(start + i) % count];
        if (!name || !*name) continue;
        path_join(out, cap, base_dir, name);
        if (file_exists(out)) return;
        out[0] = 0;
    }
}

static int pex_play_background_music_file(const char *path) {
    if (!path || !*path || g_opts.music <= 0.001f) return 0;
    float volume = g_opts.music;
    if (volume > 1.0f) volume = 1.0f;
    if (volume < 0.0f) volume = 0.0f;
    g_pex_menu_music_request = 1;
    int ok = pex_sound_backend_play_file(path, volume, 1.0f);
    g_pex_menu_music_request = 0;
    return ok;
}

static void pex_game_music_reset_delay(int ticks) {
    if (ticks < 0) ticks = 0;
    g_pex_game_music_ticks_before = ticks;
    g_pex_game_music_started = 0;
}

static void pex_menu_music_start_once(void) {
    static const char *menu_tracks[] = { "menu1.ogg", "menu2.ogg", "menu3.ogg", "menu4.ogg" };
    static const char *legacy_title_tracks[] = {
        "calm1.ogg", "calm2.ogg", "calm3.ogg",
        "hal1.ogg", "hal2.ogg", "hal3.ogg", "hal4.ogg",
        "nuance1.ogg", "nuance2.ogg",
        "piano1.ogg", "piano2.ogg", "piano3.ogg"
    };
    if (g_opts.music <= 0.001f) return;
    char resources[MAX_PATHBUF], music[MAX_PATHBUF], menu[MAX_PATHBUF], game[MAX_PATHBUF], picked[MAX_PATHBUF];
    classic_resources_path(resources, sizeof(resources));
    path_join(music, sizeof(music), resources, "music");
    path_join(menu, sizeof(menu), music, "menu");
    path_join(game, sizeof(game), music, "game");
    pex_pick_music_file_from_list(menu_tracks, (int)ARRAY_COUNT(menu_tracks), menu, picked, sizeof(picked));
    /* 1.2.5/legacy assets do not have modern menu1-menu4 files; their music
       lives in the music directory as OGG files.  The legacy downloader maps those into music/game,
       so title-screen music should fall back to that pool instead of playing
       the same one built-in/menu track forever. */
    if (!picked[0]) pex_pick_music_file_from_list(legacy_title_tracks, (int)ARRAY_COUNT(legacy_title_tracks), game, picked, sizeof(picked));
    if (!picked[0]) return;
    if (pex_play_background_music_file(picked)) g_menu_music_started = 1;
}

static void pex_menu_music_stop(void) {
    g_pex_menu_music_request = 0;
    g_menu_music_started = 0;
    g_pex_game_music_started = 0;
    sound_backend_stop_menu_music();
}

static void pex_sound_stop_world_audio(void) {
    /* Save and Quit is a hard world-session boundary.  Do not leave footsteps,
       mobs, weather, game music, or a jukebox job playing behind the title UI. */
    g_pex_menu_music_request = 0;
    g_menu_music_started = 0;
    g_pex_game_music_started = 0;
    g_pex_game_music_ticks_before = -1;
    sound_backend_stop_all_audio();
    pex_sound_backend_stop_record();
}

static void pex_game_music_tick(void) {
    static const char *overworld_tracks[] = {
        "calm1.ogg", "calm2.ogg", "calm3.ogg",
        "hal1.ogg", "hal2.ogg", "hal3.ogg", "hal4.ogg",
        "nuance1.ogg", "nuance2.ogg",
        "piano1.ogg", "piano2.ogg", "piano3.ogg"
    };
    static const char *nether_tracks[] = {
        "nether/nether1.ogg", "nether/nether2.ogg", "nether/nether3.ogg", "nether/nether4.ogg"
    };
    static const char *creative_tracks[] = {
        "creative/creative1.ogg", "creative/creative2.ogg", "creative/creative3.ogg",
        "creative/creative4.ogg", "creative/creative5.ogg", "creative/creative6.ogg"
    };
    char resources[MAX_PATHBUF], music[MAX_PATHBUF], game[MAX_PATHBUF], picked[MAX_PATHBUF];

    if (g_screen != SCREEN_INGAME || g_player_dead) return;
    if (g_opts.music <= 0.001f) { pex_menu_music_stop(); return; }
    if (pex_background_music_playing()) return;

    if (g_pex_game_music_ticks_before < 0) g_pex_game_music_ticks_before = 40 + (rand() % 80);
    if (g_pex_game_music_ticks_before > 0) { g_pex_game_music_ticks_before--; return; }

    classic_resources_path(resources, sizeof(resources));
    path_join(music, sizeof(music), resources, "music");
    path_join(game, sizeof(game), music, "game");
    picked[0] = 0;

    if (g_current_dimension == PEX_DIM_NETHER) {
        pex_pick_music_file_from_list(nether_tracks, (int)ARRAY_COUNT(nether_tracks), game, picked, sizeof(picked));
    }
    if (!picked[0] && player_is_creative()) {
        pex_pick_music_file_from_list(creative_tracks, (int)ARRAY_COUNT(creative_tracks), game, picked, sizeof(picked));
    }
    if (!picked[0]) {
        pex_pick_music_file_from_list(overworld_tracks, (int)ARRAY_COUNT(overworld_tracks), game, picked, sizeof(picked));
    }
    if (picked[0] && pex_play_background_music_file(picked)) {
        g_pex_game_music_started = 1;
        g_pex_game_music_ticks_before = (rand() % 12000) + 12000;
    } else {
        g_pex_game_music_ticks_before = 1200;
    }
}

static void pex_sound_missing_notice_once(void) {
    double t = now_seconds();
    if (t - g_pex_last_missing_sound_notice < 8.0) return;
    g_pex_last_missing_sound_notice = t;
    log_msg("Sound requested but the required legacy sound/streaming resource is missing or the SDL2_mixer backend cannot decode it");
}

static void pex_sound_stop_record(void) {
    pex_sound_backend_stop_record();
}

static void pex_sound_tick_record_stream(void) {
    pex_sound_backend_tick_record();
}

static int pex_sound_play_record_file_at(const char *path, float x, float y, float z, float volume) {
    if (!path || !*path) return 0;
    /* Java stops the background music before starting the named streaming record.
       Do this before backend failure paths too, so a failed/missing record never
       leaves title music playing over an inserted jukebox disc. */
    pex_menu_music_stop();
    if (g_opts.sound <= 0.001f) { pex_sound_stop_record(); return 0; }
    if (!pex_sound_backend_play_record_file_at(path, x, y, z, volume)) {
        pex_sound_missing_notice_once();
        return 0;
    }
    return 1;
}

static int pex_sound_find_streaming_file(const char *key, char *out, size_t cap) {
    if (!key || !*key || !out || cap == 0) return 0;
    char resources[MAX_PATHBUF], streaming[MAX_PATHBUF], file[MAX_PATHBUF];
    classic_resources_path(resources, sizeof(resources));
    path_join(streaming, sizeof(streaming), resources, "streaming");
    snprintf(file, sizeof(file), "%s.ogg", key);
    path_join(out, cap, streaming, file);
    if (file_exists(out)) return 1;
    snprintf(file, sizeof(file), "%s.mp3", key);
    path_join(out, cap, streaming, file);
    if (file_exists(out)) return 1;
    return 0;
}

static int pex_sound_play_record_key_at(const char *key, float x, float y, float z, float volume) {
    char path[MAX_PATHBUF];
    if (!key || !*key) return 0;
    if (!pex_sound_find_streaming_file(key, path, sizeof(path))) {
        log_msg("Record backend: missing streaming resource for %s in resources/streaming", key);
        pex_sound_missing_notice_once();
        return 0;
    }
    return pex_sound_play_record_file_at(path, x, y, z, volume);
}

static int pex_sound_play_twoface_record_at(float x, float y, float z, float volume) {
    char path[MAX_PATHBUF];
    path_join(path, sizeof(path), g_mc_dir, "twoface.mp3");
    if (!file_exists(path)) {
        log_msg("Custom record missing: %s", path);
        return 0;
    }
    return pex_sound_play_record_file_at(path, x, y, z, volume);
}

static void pex_sound_play(const char *key, float volume, float pitch) {
    if (!key || !*key) return;
    if (g_opts.sound <= 0.001f) return;
    if (volume <= 0.001f) return;
    if (!pex_sound_alias(key)[0]) return;
    float final_volume = volume * g_opts.sound;
    if (final_volume > 1.0f) final_volume = 1.0f;
    if (final_volume < 0.0f) final_volume = 0.0f;
    const char *path = pex_sound_find_file(key);
    if (!path) { pex_sound_missing_notice_once(); return; }
    if (!pex_sound_backend_play_file(path, final_volume, pitch)) pex_sound_missing_notice_once();
}

static void pex_sound_play_at(const char *key, float x, float y, float z, float volume, float pitch) {
    float dx = x - g_player_x, dy = y - g_player_y, dz = z - g_player_z;
    float dist = sqrtf(dx*dx + dy*dy + dz*dz);
    float atten = 1.0f;
    const float max_dist = 16.0f;
    if (dist > 1.0f) atten = 1.0f - (dist - 1.0f) / max_dist;
    if (atten <= 0.0f) return;
    if (atten > 1.0f) atten = 1.0f;
    pex_sound_play(key, volume * atten, pitch);
}

typedef struct PexBlockStepSound125 {
    const char *step_key;
    const char *break_key;
    float volume;
    float pitch;
} PexBlockStepSound125;

static PexBlockStepSound125 pex_block_step_sound_125(int id) {
    /* Block.java 1.2.5 StepSound assignments.  Unlisted blocks inherit
       soundPowderFootstep (stone, volume 1, pitch 1). */
    PexBlockStepSound125 out = {"step.stone", "step.stone", 1.0f, 1.0f};
    switch (id) {
        case BLOCK_GRASS: case BLOCK_SAPLING: case BLOCK_LEAVES: case BLOCK_SPONGE:
        case BLOCK_TALL_GRASS: case BLOCK_DEAD_BUSH: case BLOCK_YELLOW_FLOWER:
        case BLOCK_RED_ROSE: case BLOCK_BROWN_MUSHROOM: case BLOCK_RED_MUSHROOM:
        case BLOCK_TNT: case BLOCK_CROPS: case BLOCK_REEDS: case BLOCK_VINE:
        case BLOCK_MYCELIUM: case BLOCK_LILY_PAD:
            out.step_key = out.break_key = "step.grass"; break;
        case BLOCK_DIRT: case BLOCK_GRAVEL: case BLOCK_FARMLAND: case BLOCK_CLAY:
            out.step_key = out.break_key = "step.gravel"; break;
        case BLOCK_SAND: case BLOCK_SOUL_SAND:
            out.step_key = "step.sand"; out.break_key = "step.gravel"; break;
        case BLOCK_WOOL: case BLOCK_SNOW_LAYER: case BLOCK_SNOW_BLOCK:
        case BLOCK_CACTUS: case BLOCK_CAKE:
            out.step_key = out.break_key = "step.cloth"; break;
        case BLOCK_PLANKS: case BLOCK_LOG: case BLOCK_BOOKSHELF: case BLOCK_TORCH:
        case BLOCK_FIRE: case BLOCK_WOOD_STAIRS: case BLOCK_CHEST:
        case BLOCK_CRAFTING_TABLE: case BLOCK_SIGN_POST: case BLOCK_WOOD_DOOR:
        case BLOCK_LADDER: case BLOCK_WALL_SIGN: case BLOCK_LEVER:
        case BLOCK_WOOD_PRESSURE_PLATE: case BLOCK_REDSTONE_TORCH_OFF:
        case BLOCK_REDSTONE_TORCH_ON: case BLOCK_FENCE: case BLOCK_PUMPKIN:
        case BLOCK_JACK_O_LANTERN: case BLOCK_REDSTONE_REPEATER_OFF:
        case BLOCK_REDSTONE_REPEATER_ON: case BLOCK_LOCKED_CHEST:
        case BLOCK_TRAPDOOR: case BLOCK_BROWN_MUSHROOM_CAP:
        case BLOCK_RED_MUSHROOM_CAP: case BLOCK_MELON: case BLOCK_PUMPKIN_STEM:
        case BLOCK_MELON_STEM: case BLOCK_FENCE_GATE:
            out.step_key = out.break_key = "step.wood"; break;
        case BLOCK_POWERED_RAIL: case BLOCK_DETECTOR_RAIL: case BLOCK_GOLD_BLOCK:
        case BLOCK_IRON_BLOCK: case BLOCK_MOB_SPAWNER: case BLOCK_DIAMOND_BLOCK:
        case BLOCK_RAILS: case BLOCK_IRON_DOOR: case BLOCK_IRON_BARS:
            out.pitch = 1.5f; break;
        case BLOCK_GLASS: case BLOCK_ICE: case BLOCK_GLOWSTONE: case BLOCK_PORTAL:
        case BLOCK_GLASS_PANE: case BLOCK_END_PORTAL_FRAME:
        case BLOCK_REDSTONE_LAMP_OFF: case BLOCK_REDSTONE_LAMP_ON:
            out.break_key = "random.glass"; break;
        default:
            break;
    }
    return out;
}

static float pex_living_sound_pitch_125(void) {
    /* EntityLiving.getSoundPitch for an adult entity. */
    float a = (float)rand() / (float)RAND_MAX;
    float b = (float)rand() / (float)RAND_MAX;
    return (a - b) * 0.2f + 1.0f;
}
