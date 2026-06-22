/* Beta 1.0 sound mapper/playback.  The downloader mirrors Mojang legacy
   assets to .pexcraft/resources/sound/...; this file turns Java-style logical
   keys such as "random.click" or "step.grass" into one of the matching .ogg
   files and asks a platform backend to play it. */

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

#if defined(PEX_PLATFORM_SDL2)
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
    if (!strcmp(key, "random.hurt")) return "random.classic_hurt";
    if (!strcmp(key, "random.drr")) return "random.bowhit";
    return key;
}

static const char *pex_sound_find_file(const char *key) {
    if (!g_pex_sound_scanned || (now_seconds() - g_pex_last_sound_rescan) > 5.0) pex_sound_rescan();
    const char *want = pex_sound_alias(key);
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
typedef struct Mix_Chunk Mix_Chunk;
typedef int (*PFN_Mix_OpenAudio)(int, unsigned short, int, int);
typedef int (*PFN_Mix_Init)(int);
typedef void (*PFN_Mix_Quit)(void);
typedef void (*PFN_Mix_CloseAudio)(void);
typedef int (*PFN_Mix_AllocateChannels)(int);
typedef Mix_Chunk *(*PFN_Mix_LoadWAV_RW)(SDL_RWops *, int);
typedef int (*PFN_Mix_PlayChannel)(int, Mix_Chunk *, int);
typedef int (*PFN_Mix_VolumeChunk)(Mix_Chunk *, int);
typedef void (*PFN_Mix_FreeChunk)(Mix_Chunk *);
static void *g_mix_lib = NULL;
static PFN_Mix_OpenAudio pMix_OpenAudio = NULL;
static PFN_Mix_Init pMix_Init = NULL;
static PFN_Mix_Quit pMix_Quit = NULL;
static PFN_Mix_CloseAudio pMix_CloseAudio = NULL;
static PFN_Mix_AllocateChannels pMix_AllocateChannels = NULL;
static PFN_Mix_LoadWAV_RW pMix_LoadWAV_RW = NULL;
static PFN_Mix_PlayChannel pMix_PlayChannel = NULL;
static PFN_Mix_VolumeChunk pMix_VolumeChunk = NULL;
static PFN_Mix_FreeChunk pMix_FreeChunk = NULL;
static int g_mix_ready = 0;

typedef struct PexSoundChunkCache { char path[MAX_PATHBUF]; Mix_Chunk *chunk; } PexSoundChunkCache;
#define PEX_SOUND_CHUNK_CACHE_MAX 128
static PexSoundChunkCache g_sound_chunk_cache[PEX_SOUND_CHUNK_CACHE_MAX];
static int g_sound_chunk_cache_count = 0;

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
        pMix_PlayChannel = (PFN_Mix_PlayChannel)pex_sdl_load_mixer_symbol("Mix_PlayChannel");
        pMix_VolumeChunk = (PFN_Mix_VolumeChunk)pex_sdl_load_mixer_symbol("Mix_VolumeChunk");
        pMix_FreeChunk = (PFN_Mix_FreeChunk)pex_sdl_load_mixer_symbol("Mix_FreeChunk");
        if (!pMix_OpenAudio || !pMix_LoadWAV_RW || !pMix_PlayChannel) { log_msg("Sound backend: SDL2_mixer missing required symbols"); return 0; }
    }
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    if (pMix_Init) pMix_Init(0x00000002); /* MIX_INIT_OGG */
    if (pMix_OpenAudio(44100, AUDIO_S16SYS, 2, 1024) != 0) { log_msg("Sound backend: Mix_OpenAudio failed"); return 0; }
    if (pMix_AllocateChannels) pMix_AllocateChannels(32);
    g_mix_ready = 1;
    return 1;
}

static Mix_Chunk *pex_sound_load_chunk_cached(const char *path) {
    if (!path || !*path) return NULL;
    for (int i = 0; i < g_sound_chunk_cache_count; ++i) if (!strcmp(g_sound_chunk_cache[i].path, path)) return g_sound_chunk_cache[i].chunk;
    if (!pex_sound_backend_init()) return NULL;
    SDL_RWops *rw = SDL_RWFromFile(path, "rb");
    if (!rw) return NULL;
    Mix_Chunk *c = pMix_LoadWAV_RW(rw, 1);
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

static int pex_sound_backend_play_file(const char *path, float volume, float pitch) {
    (void)pitch;
    Mix_Chunk *c = pex_sound_load_chunk_cached(path);
    if (!c) return 0;
    int v = (int)(volume * 128.0f);
    if (v < 0) v = 0; if (v > 128) v = 128;
    if (pMix_VolumeChunk) pMix_VolumeChunk(c, v);
    return pMix_PlayChannel(-1, c, 0) >= 0;
}

static void pex_sound_shutdown(void) {
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
#elif defined(_WIN32)
#ifndef COBJMACROS
#define COBJMACROS 1
#endif
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mmreg.h>

typedef struct PexMfPlayJob { char path[MAX_PATHBUF]; float volume; } PexMfPlayJob;

typedef struct PexPcmBuffer {
    BYTE *data;
    DWORD bytes;
    DWORD cap;
    WAVEFORMATEX fmt;
} PexPcmBuffer;

static int pex_pcm_append(PexPcmBuffer *pcm, const BYTE *data, DWORD bytes) {
    DWORD need;
    BYTE *next;
    if (!pcm || !data || bytes == 0) return 1;
    if (pcm->bytes > 0x7fffffffU - bytes) return 0;
    need = pcm->bytes + bytes;
    if (need > pcm->cap) {
        DWORD new_cap = pcm->cap ? pcm->cap * 2U : 65536U;
        while (new_cap < need) {
            if (new_cap > 0x40000000U) { new_cap = need; break; }
            new_cap *= 2U;
        }
        next = (BYTE *)realloc(pcm->data, new_cap);
        if (!next) return 0;
        pcm->data = next;
        pcm->cap = new_cap;
    }
    memcpy(pcm->data + pcm->bytes, data, bytes);
    pcm->bytes = need;
    return 1;
}

static void pex_pcm_apply_volume_16(PexPcmBuffer *pcm, float volume) {
    if (!pcm || !pcm->data || pcm->fmt.wBitsPerSample != 16) return;
    if (volume > 0.999f && volume < 1.001f) return;
    short *s = (short *)pcm->data;
    DWORD count = pcm->bytes / 2U;
    for (DWORD i = 0; i < count; ++i) {
        int v = (int)((float)s[i] * volume);
        if (v < -32768) v = -32768;
        if (v > 32767) v = 32767;
        s[i] = (short)v;
    }
}

static int pex_wide_from_utf8_or_ansi(const char *path, WCHAR *out, int cap) {
    int n;
    if (!path || !out || cap <= 0) return 0;
    n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, out, cap);
    if (n <= 0) n = MultiByteToWideChar(CP_ACP, 0, path, -1, out, cap);
    return n > 0;
}

static int pex_mf_decode_file_to_pcm(const char *path, PexPcmBuffer *out) {
    HRESULT hr;
    WCHAR wpath[MAX_PATHBUF];
    IMFSourceReader *reader = NULL;
    IMFMediaType *type = NULL;
    int ok = 0;
    int coinit_ok = 0;
    memset(out, 0, sizeof(*out));
    out->fmt.wFormatTag = WAVE_FORMAT_PCM;
    out->fmt.nChannels = 2;
    out->fmt.nSamplesPerSec = 44100;
    out->fmt.wBitsPerSample = 16;
    out->fmt.nBlockAlign = (WORD)(out->fmt.nChannels * out->fmt.wBitsPerSample / 8);
    out->fmt.nAvgBytesPerSec = out->fmt.nSamplesPerSec * out->fmt.nBlockAlign;

    if (!pex_wide_from_utf8_or_ansi(path, wpath, (int)ARRAY_COUNT(wpath))) return 0;
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr)) coinit_ok = 1;
    hr = MFStartup(MF_VERSION, MFSTARTUP_LITE);
    if (FAILED(hr)) goto done;

    hr = MFCreateSourceReaderFromURL(wpath, NULL, &reader);
    if (FAILED(hr)) goto done;

    IMFSourceReader_SetStreamSelection(reader, MF_SOURCE_READER_ALL_STREAMS, FALSE);
    IMFSourceReader_SetStreamSelection(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);

    hr = MFCreateMediaType(&type);
    if (FAILED(hr)) goto done;
    IMFMediaType_SetGUID(type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    IMFMediaType_SetGUID(type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    IMFMediaType_SetUINT32(type, &MF_MT_AUDIO_NUM_CHANNELS, 2);
    IMFMediaType_SetUINT32(type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100);
    IMFMediaType_SetUINT32(type, &MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
    hr = IMFSourceReader_SetCurrentMediaType(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, type);
    if (FAILED(hr)) goto done;

    for (;;) {
        DWORD stream = 0, flags = 0;
        LONGLONG ts = 0;
        IMFSample *sample = NULL;
        hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &stream, &flags, &ts, &sample);
        if (FAILED(hr)) { if (sample) IMFSample_Release(sample); goto done; }
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) { if (sample) IMFSample_Release(sample); break; }
        if (sample) {
            IMFMediaBuffer *buf = NULL;
            hr = IMFSample_ConvertToContiguousBuffer(sample, &buf);
            if (SUCCEEDED(hr) && buf) {
                BYTE *data = NULL;
                DWORD max_len = 0, cur_len = 0;
                hr = IMFMediaBuffer_Lock(buf, &data, &max_len, &cur_len);
                if (SUCCEEDED(hr)) {
                    if (!pex_pcm_append(out, data, cur_len)) { IMFMediaBuffer_Unlock(buf); IMFMediaBuffer_Release(buf); IMFSample_Release(sample); goto done; }
                    IMFMediaBuffer_Unlock(buf);
                }
                IMFMediaBuffer_Release(buf);
            }
            IMFSample_Release(sample);
        }
    }
    ok = out->bytes > 0;

done:
    if (type) IMFMediaType_Release(type);
    if (reader) IMFSourceReader_Release(reader);
    MFShutdown();
    if (coinit_ok) CoUninitialize();
    if (!ok) {
        free(out->data);
        memset(out, 0, sizeof(*out));
    }
    return ok;
}

static int pex_waveout_play_pcm(PexPcmBuffer *pcm, float volume) {
    HWAVEOUT hwo = NULL;
    WAVEHDR hdr;
    MMRESULT mm;
    if (!pcm || !pcm->data || pcm->bytes == 0) return 0;
    pex_pcm_apply_volume_16(pcm, volume);
    memset(&hdr, 0, sizeof(hdr));
    hdr.lpData = (LPSTR)pcm->data;
    hdr.dwBufferLength = pcm->bytes;
    mm = waveOutOpen(&hwo, WAVE_MAPPER, &pcm->fmt, 0, 0, CALLBACK_NULL);
    if (mm != MMSYSERR_NOERROR) return 0;
    if (waveOutPrepareHeader(hwo, &hdr, sizeof(hdr)) != MMSYSERR_NOERROR) { waveOutClose(hwo); return 0; }
    if (waveOutWrite(hwo, &hdr, sizeof(hdr)) != MMSYSERR_NOERROR) {
        waveOutUnprepareHeader(hwo, &hdr, sizeof(hdr));
        waveOutClose(hwo);
        return 0;
    }
    while (!(hdr.dwFlags & WHDR_DONE)) Sleep(2);
    waveOutUnprepareHeader(hwo, &hdr, sizeof(hdr));
    waveOutClose(hwo);
    return 1;
}

static DWORD WINAPI pex_mf_play_thread(LPVOID arg) {
    PexMfPlayJob *job = (PexMfPlayJob *)arg;
    PexPcmBuffer pcm;
    if (!job) return 0;
    if (pex_mf_decode_file_to_pcm(job->path, &pcm)) {
        if (!pex_waveout_play_pcm(&pcm, job->volume)) log_msg("Sound backend: waveOut could not play decoded audio");
        free(pcm.data);
    } else {
        log_msg("Sound backend: Media Foundation could not decode OGG: %s", job->path);
    }
    free(job);
    return 0;
}

static int pex_sound_backend_play_file(const char *path, float volume, float pitch) {
    (void)pitch;
    if (!path || !*path) return 0;
    PexMfPlayJob *job = (PexMfPlayJob *)calloc(1, sizeof(*job));
    if (!job) return 0;
    snprintf(job->path, sizeof(job->path), "%s", path);
    job->volume = volume;
    HANDLE th = CreateThread(NULL, 0, pex_mf_play_thread, job, 0, NULL);
    if (!th) { free(job); return 0; }
    CloseHandle(th);
    return 1;
}
static void pex_sound_shutdown(void) { }
#else
static int pex_sound_backend_play_file(const char *path, float volume, float pitch) {
    (void)path; (void)volume; (void)pitch;
    return 0;
}
static void pex_sound_shutdown(void) { }
#endif

static void pex_sound_missing_notice_once(void) {
    double t = now_seconds();
    if (t - g_pex_last_missing_sound_notice < 8.0) return;
    g_pex_last_missing_sound_notice = t;
    log_msg("Sound requested but no b1.0 sound assets are installed or no OGG backend is available");
}

static void pex_sound_play(const char *key, float volume, float pitch) {
    if (!key || !*key) return;
    if (g_opts.sound <= 0.001f) return;
    if (volume <= 0.001f) return;
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

static const char *pex_block_step_sound_key(int id) {
    switch (id) {
        case BLOCK_WOOL:
        case BLOCK_SNOW_LAYER:
        case BLOCK_SNOW_BLOCK:
        case BLOCK_CACTUS:
            return "step.cloth";
        case BLOCK_GRASS:
        case BLOCK_LEAVES:
        case BLOCK_SPONGE:
        case BLOCK_TNT:
        case BLOCK_CROPS:
        case BLOCK_REEDS:
        case BLOCK_YELLOW_FLOWER:
        case BLOCK_RED_ROSE:
        case BLOCK_BROWN_MUSHROOM:
        case BLOCK_RED_MUSHROOM:
            return "step.grass";
        case BLOCK_DIRT:
        case BLOCK_GRAVEL:
        case BLOCK_FARMLAND:
        case BLOCK_CLAY:
            return "step.gravel";
        case BLOCK_SAND:
        case BLOCK_SOUL_SAND:
            return "step.sand";
        case BLOCK_PLANKS:
        case BLOCK_LOG:
        case BLOCK_BOOKSHELF:
        case BLOCK_WOOD_STAIRS:
        case BLOCK_CHEST:
        case BLOCK_CRAFTING_TABLE:
        case BLOCK_SIGN_POST:
        case BLOCK_WALL_SIGN:
        case BLOCK_WOOD_DOOR:
        case BLOCK_LADDER:
        case BLOCK_WOOD_PRESSURE_PLATE:
        case BLOCK_FENCE:
        case BLOCK_JUKEBOX:
        case BLOCK_PUMPKIN:
        case BLOCK_JACK_O_LANTERN:
            return "step.wood";
        default:
            return "step.stone";
    }
}

static const char *pex_block_dig_sound_key(int id) {
    const char *step = pex_block_step_sound_key(id);
    if (!strncmp(step, "step.", 5)) {
        if (!strcmp(step + 5, "cloth")) return "dig.cloth";
        if (!strcmp(step + 5, "grass")) return "dig.grass";
        if (!strcmp(step + 5, "gravel")) return "dig.gravel";
        if (!strcmp(step + 5, "sand")) return "dig.sand";
        if (!strcmp(step + 5, "stone")) return "dig.stone";
        if (!strcmp(step + 5, "wood")) return "dig.wood";
    }
    return step;
}
