/* Split from original monolithic main.c. Included by src/main.c unity build. */

static int read_u32_le(FILE *f) {
    unsigned char b[4];
    if (fread(b, 1, 4, f) != 4) return 0;
    return (int)(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
}

static int read_u32_le_mem(const unsigned char *p) {
    return (int)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

static int file_exists(const char *path) {
    DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
/* Generated in GitHub Actions as build/<platform>_generated/*_mcrw_assets.pak
   and embedded into the console executable through an .incbin assembly wrapper.
   This avoids requiring texture files on external storage. */
#if defined(PEX_PLATFORM_PSP)
extern const unsigned char pexcraft_psp_mcrw_assets_pak_start[];
extern const unsigned char pexcraft_psp_mcrw_assets_pak_end[];
#elif defined(PEX_PLATFORM_WII)
extern const unsigned char pexcraft_wii_mcrw_assets_pak_start[];
extern const unsigned char pexcraft_wii_mcrw_assets_pak_end[];
#ifndef PEX_PSP_LOGF
#define PEX_PSP_LOGF(...) do { } while (0)
#endif
#endif
#endif

static void free_texture(Texture *t) {
    if (!t) return;
    if (t->id) glDeleteTextures(1, &t->id);
    if (t->rgba) free(t->rgba);
    t->id = 0;
    t->w = 0;
    t->h = 0;
    t->rgba = NULL;
}


static void set_texture_filter_wrap(Texture *t, int linear, int repeat) {
    if (!t || !t->id) return;
    glBindTexture(GL_TEXTURE_2D, t->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : PEX_GL_CLAMP_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : PEX_GL_CLAMP_EDGE);
}

static int upload_rgba_texture(Texture *t, int w, int h, unsigned char *rgba, int repeat) {
    if (!t || !rgba || w <= 0 || h <= 0) return 0;
    free_texture(t);
    t->w = w;
    t->h = h;
    t->rgba = rgba;
    glGenTextures(1, &t->id);
    glBindTexture(GL_TEXTURE_2D, t->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : PEX_GL_CLAMP_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : PEX_GL_CLAMP_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->rgba);
    return t->id != 0;
}

static void normalize_sky_alpha_from_luminance(Texture *t) {
    if (!t || !t->rgba || !t->id || t->w <= 0 || t->h <= 0) return;
    size_t pixels = (size_t)t->w * (size_t)t->h;
    if (pixels == 0 || pixels > (size_t)4096 * (size_t)4096) return;
    size_t opaque = 0, transparent = 0, dark_opaque = 0;
    for (size_t i = 0; i < pixels; ++i) {
        unsigned char *px = &t->rgba[i * 4u];
        if (px[3] >= 250) opaque++;
        if (px[3] <= 5) transparent++;
        if (px[3] >= 250 && px[0] < 12 && px[1] < 12 && px[2] < 12) dark_opaque++;
    }
    /* Vanilla terrain/sun.png is a square glow sprite.  Some pack/conversion
       paths preserve the RGB glow but lose its alpha, which renders the famous
       black square around the sun.  If an opaque square sky sprite is loaded,
       rebuild alpha from luminance and re-upload it.  Moon phases are usually
       64x32, so the square-size rule avoids making dark moon pixels disappear. */
    int looks_like_sun = (t->w == t->h && t->w <= 128 && opaque * 100u >= pixels * 95u);
    int looks_like_lost_alpha = (transparent == 0 && opaque * 100u >= pixels * 95u && dark_opaque > 0);
    if (!looks_like_sun && (!looks_like_lost_alpha || dark_opaque * 100u < pixels * 5u)) return;
    for (size_t i = 0; i < pixels; ++i) {
        unsigned char *px = &t->rgba[i * 4u];
        unsigned char a = px[0];
        if (px[1] > a) a = px[1];
        if (px[2] > a) a = px[2];
        px[3] = a;
    }
    glBindTexture(GL_TEXTURE_2D, t->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->rgba);
}

#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
static const char *pex_basename_asset(const char *path) {
    const char *base = path;
    for (const char *p = path; p && *p; ++p) {
        if (*p == '/' || *p == '\\') base = p + 1;
    }
    return base;
}

static int load_mcrw_memory(Texture *t, const unsigned char *data, unsigned int len, int repeat) {
    if (!data || len < 12) { PEX_PSP_LOGF("MCRW memory reject: null/short len=%u", len); return 0; }
    if (memcmp(data, "MCRW", 4) != 0) { PEX_PSP_LOGF("MCRW memory reject: bad magic %02x %02x %02x %02x", data[0], data[1], data[2], data[3]); return 0; }
    int w = read_u32_le_mem(data + 4);
    int h = read_u32_le_mem(data + 8);
    if (w <= 0 || h <= 0 || w > 4096 || h > 4096) { PEX_PSP_LOGF("MCRW memory reject: bad dims %dx%d len=%u", w, h, len); return 0; }
    size_t n = (size_t)w * (size_t)h * 4;
    if ((size_t)len < 12 + n) { PEX_PSP_LOGF("MCRW memory reject: truncated %dx%d need=%u got=%u", w, h, (unsigned)(12+n), len); return 0; }
    unsigned char *rgba = (unsigned char*)malloc(n);
    if (!rgba) { PEX_PSP_LOGF("MCRW memory reject: malloc failed bytes=%u", (unsigned)n); return 0; }
    memcpy(rgba, data + 12, n);
    int ok = upload_rgba_texture(t, w, h, rgba, repeat);
    PEX_PSP_LOGF("MCRW upload %dx%d repeat=%d -> tex=%u ok=%d", w, h, repeat, (unsigned)(t ? t->id : 0), ok);
    return ok;
}

static int psp_make_fallback_texture(Texture *t, const char *name, int w, int h, int repeat) {
    if (w <= 0) w = 16;
    if (h <= 0) h = 16;
    size_t n = (size_t)w * (size_t)h * 4;
    unsigned char *rgba = (unsigned char*)malloc(n);
    if (!rgba) { PEX_PSP_LOGF("fallback texture malloc failed for %s bytes=%u", name ? name : "?", (unsigned)n); return 0; }
    unsigned int seed = 2166136261u;
    if (name) for (const unsigned char *p=(const unsigned char*)name; *p; ++p) seed = (seed ^ *p) * 16777619u;
    unsigned char r = (unsigned char)(96u + (seed & 0x7fu));
    unsigned char g = (unsigned char)(96u + ((seed >> 8) & 0x7fu));
    unsigned char b = (unsigned char)(96u + ((seed >> 16) & 0x7fu));
    for (int y=0; y<h; ++y) {
        for (int x=0; x<w; ++x) {
            int k = (y*w + x) * 4;
            int checker = ((x >> 3) ^ (y >> 3)) & 1;
            rgba[k+0] = checker ? r : (unsigned char)(r/2);
            rgba[k+1] = checker ? g : (unsigned char)(g/2);
            rgba[k+2] = checker ? b : (unsigned char)(b/2);
            rgba[k+3] = 255;
        }
    }
    int ok = upload_rgba_texture(t, w, h, rgba, repeat);
    PEX_PSP_LOGF("fallback texture %s %dx%d -> tex=%u ok=%d", name ? name : "?", w, h, (unsigned)(t ? t->id : 0), ok);
    return ok;
}


#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
static void psp_drop_texture_cpu_copy(Texture *t) {
    if (!t || !t->rgba) return;
    free(t->rgba);
    t->rgba = NULL;
}
static void psp_drop_gui_texture_cpu_copies(void) {
    /* Keep CPU copies only where the game actually samples pixels on CPU: font
       for text width/alpha, terrain for item/color/cutout logic, and Steve. */
    psp_drop_texture_cpu_copy(&tex_bg);
    psp_drop_texture_cpu_copy(&tex_gui);
    psp_drop_texture_cpu_copy(&tex_black);
    psp_drop_texture_cpu_copy(&tex_pack);
    psp_drop_texture_cpu_copy(&tex_default_pack_icon);
    psp_drop_texture_cpu_copy(&tex_unknown_pack);
    psp_drop_texture_cpu_copy(&tex_icons);
    psp_drop_texture_cpu_copy(&tex_inventory);
    psp_drop_texture_cpu_copy(&tex_workbench);
    psp_drop_texture_cpu_copy(&tex_furnace_gui);
    psp_drop_texture_cpu_copy(&tex_chest_gui);
    psp_drop_texture_cpu_copy(&tex_items);
    for (int m = 0; m < 5; m++) for (int l = 0; l < 2; l++) psp_drop_texture_cpu_copy(&tex_armor[m][l]);
    psp_drop_texture_cpu_copy(&tex_clouds);
    psp_drop_texture_cpu_copy(&tex_water_overlay);
    psp_drop_texture_cpu_copy(&tex_shadow);
    psp_drop_texture_cpu_copy(&tex_particles);
}

#endif

static const unsigned char *psp_mcrw_pak_data(size_t *out_len) {
#if defined(PEX_PLATFORM_PSP)
    const unsigned char *start = pexcraft_psp_mcrw_assets_pak_start;
    const unsigned char *end = pexcraft_psp_mcrw_assets_pak_end;
#elif defined(PEX_PLATFORM_WII)
    const unsigned char *start = pexcraft_wii_mcrw_assets_pak_start;
    const unsigned char *end = pexcraft_wii_mcrw_assets_pak_end;
#else
    const unsigned char *start = NULL;
    const unsigned char *end = NULL;
#endif
    size_t len = 0;
    if (end >= start) len = (size_t)(end - start);
    if (out_len) *out_len = len;
    return start;
}

static unsigned int psp_embedded_mcrw_count(void) {
    size_t len = 0;
    const unsigned char *pak = psp_mcrw_pak_data(&len);
    if (!pak || len < 12) return 0;
    if (memcmp(pak, "PXCMCRW1", 8) != 0) return 0;
    return (unsigned int)read_u32_le_mem(pak + 8);
}

static int psp_ascii_ieq_len(const char *a, const unsigned char *b, unsigned int blen) {
    if (!a || !b) return 0;
    unsigned int i = 0;
    for (; i < blen; ++i) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = b[i];
        if (!ca) return 0;
        if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb - 'A' + 'a');
        if (ca != cb) return 0;
    }
    return a[i] == '\0';
}

static int psp_find_embedded_mcrw(const char *base, const unsigned char **out_data, unsigned int *out_len) {
    size_t len = 0;
    const unsigned char *pak = psp_mcrw_pak_data(&len);
    if (out_data) *out_data = NULL;
    if (out_len) *out_len = 0;
    if (!pak || len < 12) { PEX_PSP_LOGF("MCRW pak missing/short len=%u", (unsigned)len); return 0; }
    if (memcmp(pak, "PXCMCRW1", 8) != 0) { PEX_PSP_LOGF("MCRW pak bad magic len=%u", (unsigned)len); return 0; }
    unsigned int count = (unsigned int)read_u32_le_mem(pak + 8);
    size_t pos = 12;
    for (unsigned int i = 0; i < count; ++i) {
        if (pos + 12 > len) { PEX_PSP_LOGF("MCRW pak truncated entry header i=%u pos=%u len=%u", i, (unsigned)pos, (unsigned)len); return 0; }
        unsigned int name_len = (unsigned int)read_u32_le_mem(pak + pos + 0);
        unsigned int data_len = (unsigned int)read_u32_le_mem(pak + pos + 4);
        unsigned int data_off = (unsigned int)read_u32_le_mem(pak + pos + 8);
        pos += 12;
        if (name_len > 255 || pos + name_len > len) { PEX_PSP_LOGF("MCRW pak bad name i=%u name_len=%u", i, name_len); return 0; }
        const unsigned char *name = pak + pos;
        pos += name_len;
        if ((size_t)data_off > len || (size_t)data_len > len || (size_t)data_off + (size_t)data_len > len) {
            PEX_PSP_LOGF("MCRW pak bad data i=%u off=%u len=%u pak=%u", i, data_off, data_len, (unsigned)len);
            return 0;
        }
        if (psp_ascii_ieq_len(base, name, name_len)) {
            if (out_data) *out_data = pak + data_off;
            if (out_len) *out_len = data_len;
            PEX_PSP_LOGF("MCRW pak hit[%u]: %s len=%u", i, base ? base : "?", data_len);
            return 1;
        }
    }
    return 0;
}

static int load_embedded_mcrw(Texture *t, const char *filename, int repeat) {
    const char *base = pex_basename_asset(filename);
    unsigned int count = psp_embedded_mcrw_count();
    size_t pak_len = 0;
    (void)psp_mcrw_pak_data(&pak_len);
    PEX_PSP_LOGF("embedded pak lookup: %s base=%s count=%u pak_len=%u", filename ? filename : "?", base ? base : "?", count, (unsigned)pak_len);
    const unsigned char *data = NULL;
    unsigned int len = 0;
    if (psp_find_embedded_mcrw(base, &data, &len)) {
        return load_mcrw_memory(t, data, len, repeat);
    }
    PEX_PSP_LOGF("embedded pak miss: %s", base ? base : "?");
    return 0;
}
#endif

static int load_mcrw_path(Texture *t, const char *path, int repeat) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    char magic[4];
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, "MCRW", 4) != 0) { fclose(f); return 0; }
    int w = read_u32_le(f);
    int h = read_u32_le(f);
    if (w <= 0 || h <= 0 || w > 4096 || h > 4096) { fclose(f); return 0; }
    size_t n = (size_t)w * (size_t)h * 4;
    unsigned char *rgba = (unsigned char*)malloc(n);
    if (!rgba || fread(rgba, 1, n, f) != n) { if (rgba) free(rgba); fclose(f); return 0; }
    fclose(f);
    return upload_rgba_texture(t, w, h, rgba, repeat);
}

static int load_mcrw(Texture *t, const char *filename, int repeat) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    if (load_embedded_mcrw(t, filename, repeat)) return 1;
#endif
    char path[MAX_PATHBUF];
#if defined(PEX_PLATFORM_PSP)
    snprintf(path, sizeof(path), "assets/%s", filename);
    if (load_mcrw_path(t, path, repeat)) return 1;
    snprintf(path, sizeof(path), "./assets/%s", filename);
#elif defined(PEX_PLATFORM_WII)
    /* Wii textures are linked into boot.dol.  Do not require sd:/assets. */
    (void)path;
    return 0;
#else
    snprintf(path, sizeof(path), "assets\\%s", filename);
    if (load_mcrw_path(t, path, repeat)) return 1;
    snprintf(path, sizeof(path), ".\\assets\\%s", filename);
#endif
    return load_mcrw_path(t, path, repeat);
}

#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
static int ensure_wic(void) { return 1; }
static int load_png_texture(Texture *t, const char *path, int repeat) { (void)t; (void)path; (void)repeat; return 0; }
#elif defined(PEX_PLATFORM_ANDROID)
static int ensure_wic(void) { return 1; }

static unsigned char *android_decode_png_rgba(const char *path, int *out_w, int *out_h) {
    if (out_w) *out_w = 0;
    if (out_h) *out_h = 0;
    if (!path || !file_exists(path)) return NULL;

    char norm[MAX_PATHBUF];
    pex_normalize_path(norm, sizeof(norm), path);
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = SDL_AndroidGetActivity();
    if (!env || !activity) return NULL;
    jclass cls = (*env)->GetObjectClass(env, activity);
    jmethodID mid = cls ? (*env)->GetMethodID(env, cls, "decodePngToRgba", "(Ljava/lang/String;)[B") : NULL;
    if (!mid) {
        if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionDescribe(env); (*env)->ExceptionClear(env); }
        if (cls) (*env)->DeleteLocalRef(env, cls);
        (*env)->DeleteLocalRef(env, activity);
        return NULL;
    }
    jstring jpath = (*env)->NewStringUTF(env, norm);
    jbyteArray arr = (jbyteArray)(*env)->CallObjectMethod(env, activity, mid, jpath);
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionDescribe(env); (*env)->ExceptionClear(env); }
    if (jpath) (*env)->DeleteLocalRef(env, jpath);
    if (cls) (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, activity);
    if (!arr) return NULL;

    jsize len = (*env)->GetArrayLength(env, arr);
    if (len < 8) { (*env)->DeleteLocalRef(env, arr); return NULL; }
    jbyte *bytes = (*env)->GetByteArrayElements(env, arr, NULL);
    if (!bytes) { (*env)->DeleteLocalRef(env, arr); return NULL; }
    unsigned char *b = (unsigned char *)bytes;
    int w = (int)((unsigned)b[0] | ((unsigned)b[1] << 8) | ((unsigned)b[2] << 16) | ((unsigned)b[3] << 24));
    int h = (int)((unsigned)b[4] | ((unsigned)b[5] << 8) | ((unsigned)b[6] << 16) | ((unsigned)b[7] << 24));
    size_t need = (size_t)w * (size_t)h * 4u;
    unsigned char *rgba = NULL;
    if (w > 0 && h > 0 && w <= 4096 && h <= 4096 && (size_t)len >= 8u + need) {
        rgba = (unsigned char *)malloc(need);
        if (rgba) memcpy(rgba, b + 8, need);
        if (out_w) *out_w = w;
        if (out_h) *out_h = h;
    }
    (*env)->ReleaseByteArrayElements(env, arr, bytes, JNI_ABORT);
    (*env)->DeleteLocalRef(env, arr);
    return rgba;
}

static int load_png_texture(Texture *t, const char *path, int repeat) {
    int w = 0, h = 0;
    unsigned char *rgba = android_decode_png_rgba(path, &w, &h);
    if (!rgba) return 0;
    log_msg("Loaded PNG texture: %s (%dx%d)", path, w, h);
    return upload_rgba_texture(t, w, h, rgba, repeat);
}
#elif defined(PEX_PLATFORM_ANDROID_TV)
static int ensure_wic(void) { return 1; }
static int load_png_texture(Texture *t, const char *path, int repeat) { (void)t; (void)path; (void)repeat; return 0; }
#elif defined(PEX_PLATFORM_SDL2)
static int ensure_wic(void) { return 1; }

static int load_png_texture(Texture *t, const char *path, int repeat) {
    if (!file_exists(path)) return 0;
    char norm[MAX_PATHBUF];
    pex_normalize_path(norm, sizeof(norm), path);
    SDL_Surface *surf = IMG_Load(norm);
    if (!surf) {
        log_msg("Failed to load PNG texture: %s", norm);
        return 0;
    }
    SDL_Surface *rgba_surf = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surf);
    if (!rgba_surf) return 0;
    int w = rgba_surf->w;
    int h = rgba_surf->h;
    if (w <= 0 || h <= 0 || w > 4096 || h > 4096) { SDL_FreeSurface(rgba_surf); return 0; }
    size_t bytes = (size_t)w * (size_t)h * 4;
    unsigned char *rgba = (unsigned char*)malloc(bytes);
    if (!rgba) { SDL_FreeSurface(rgba_surf); return 0; }
    unsigned char *src = (unsigned char*)rgba_surf->pixels;
    for (int y = 0; y < h; ++y) memcpy(rgba + (size_t)y * (size_t)w * 4, src + (size_t)y * (size_t)rgba_surf->pitch, (size_t)w * 4);
    SDL_FreeSurface(rgba_surf);
    log_msg("Loaded PNG texture: %s (%dx%d)", norm, w, h);
    return upload_rgba_texture(t, w, h, rgba, repeat);
}
#else
static int ensure_wic(void) {
    if (g_wic_factory) return 1;
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return 0;
    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICImagingFactory, (void**)&g_wic_factory);
    return SUCCEEDED(hr) && g_wic_factory != NULL;
}

static int load_png_texture(Texture *t, const char *path, int repeat) {
    if (!file_exists(path) || !ensure_wic()) return 0;

    WCHAR wpath[MAX_PATHBUF];
    if (!MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATHBUF)) return 0;

    IWICBitmapDecoder *decoder = NULL;
    IWICBitmapFrameDecode *frame = NULL;
    IWICFormatConverter *converter = NULL;
    HRESULT hr = IWICImagingFactory_CreateDecoderFromFilename(
        g_wic_factory, wpath, NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr)) goto fail;

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    if (FAILED(hr)) goto fail;

    hr = IWICImagingFactory_CreateFormatConverter(g_wic_factory, &converter);
    if (FAILED(hr)) goto fail;

    hr = IWICFormatConverter_Initialize(converter, (IWICBitmapSource*)frame,
        &GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, NULL, 0.0,
        WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) goto fail;

    UINT w = 0, h = 0;
    hr = IWICFormatConverter_GetSize(converter, &w, &h);
    if (FAILED(hr) || w == 0 || h == 0 || w > 4096 || h > 4096) goto fail;

    UINT stride = w * 4;
    UINT bytes = stride * h;
    unsigned char *rgba = (unsigned char*)malloc(bytes);
    if (!rgba) goto fail;

    hr = IWICFormatConverter_CopyPixels(converter, NULL, stride, bytes, rgba);
    if (FAILED(hr)) { free(rgba); goto fail; }

    if (converter) IWICFormatConverter_Release(converter);
    if (frame) IWICBitmapFrameDecode_Release(frame);
    if (decoder) IWICBitmapDecoder_Release(decoder);
    log_msg("Loaded PNG texture: %s (%ux%u)", path, (unsigned)w, (unsigned)h);
    return upload_rgba_texture(t, (int)w, (int)h, rgba, repeat);

fail:
    if (converter) IWICFormatConverter_Release(converter);
    if (frame) IWICBitmapFrameDecode_Release(frame);
    if (decoder) IWICBitmapDecoder_Release(decoder);
    log_msg("Failed to load PNG texture: %s", path);
    return 0;
}
#endif

static int load_png_asset(Texture *t, const char *rel, int repeat) {
    char path[MAX_PATHBUF];
    snprintf(path, sizeof(path), "assets\\%s", rel);
    if (load_png_texture(t, path, repeat)) return 1;
    snprintf(path, sizeof(path), ".\\assets\\%s", rel);
    return load_png_texture(t, path, repeat);
}

static int load_custom_skin_path(const char *path, int persist) {
    if (!path || !path[0]) return 0;
    Texture tmp = {0};
    if (!load_png_texture(&tmp, path, 0)) return 0;
    if (!((tmp.w == 64 && tmp.h == 32) || (tmp.w == 64 && tmp.h == 64))) {
        free_texture(&tmp);
        open_notice("Skins", "Skin PNG must be 64x32 or 64x64.", "Classic 64x32 skins work best.");
        return 0;
    }
    free_texture(&tex_steve);
    tex_steve = tmp;
    if (persist) {
        snprintf(g_opts.skin_path, sizeof(g_opts.skin_path), "%s", path);
        save_options();
    }
    if (g_mp_connected) pex_net_send_skin();
    return 1;
}

static int choose_and_import_skin(void) {
#ifdef PEX_PLATFORM_PSP
    open_notice("Skins", "Skin import is not available on PSP.", "Copy converted assets before building the EBOOT.");
    return 0;
#elif defined(PEX_PLATFORM_SDL2)
    char path[MAX_PATHBUF];
    snprintf(path, sizeof(path), "%s/custom.png", g_skin_dir);
    if (load_custom_skin_path(path, 1)) return 1;
    open_notice("Skins", "Linux/SDL2 has no native file dialog here.", "Copy your PNG to ~/.pexcraft/skins/custom.png first.");
    return 0;
#else
    char chosen[MAX_PATHBUF] = "";
    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFilter = "PNG Skin (*.png)\0*.png\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = chosen;
    ofn.nMaxFile = sizeof(chosen);
    ofn.lpstrTitle = "Import skin";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameA(&ofn)) return 0;

    ensure_dir(g_skin_dir);
    char dst[MAX_PATHBUF];
    snprintf(dst, sizeof(dst), "%s\\custom.png", g_skin_dir);
    if (!CopyFileA(chosen, dst, FALSE)) {
        open_notice("Skins", "Could not copy the selected PNG", "into the PEXCRAFT skins folder.");
        return 0;
    }
    return load_custom_skin_path(dst, 1);
#endif
}


static void trim_pack_line_34(char *line) {
    line[strcspn(line, "\r\n")] = 0;
    if (strlen(line) > 34) line[34] = 0;
}

static void free_texture_pack_icons(void) {
    for (int i = 0; i < g_texpack_count; i++) {
        if (g_texpacks[i].has_icon) {
            free_texture(&g_texpacks[i].icon);
            g_texpacks[i].has_icon = 0;
        }
    }
}

static void read_pack_text(TexturePackEntry *e) {
    e->desc1[0] = 0;
    e->desc2[0] = 0;
    char txt[MAX_PATHBUF];
    path_join(txt, sizeof(txt), e->path, "pack.txt");
    FILE *f = fopen(txt, "r");
    if (!f) return;
    if (fgets(e->desc1, sizeof(e->desc1), f)) trim_pack_line_34(e->desc1);
    if (fgets(e->desc2, sizeof(e->desc2), f)) trim_pack_line_34(e->desc2);
    fclose(f);
}

static void load_pack_icon(TexturePackEntry *e) {
    char path[MAX_PATHBUF];
    path_join(path, sizeof(path), e->path, "pack.png");
    if (load_png_texture(&e->icon, path, 0)) e->has_icon = 1;
}


static void classic_pack_path(char *out, size_t cap) {
    path_join(out, cap, g_texpack_dir, CLASSIC_PACK_NAME);
}

static int release_file_exists_in_dir(const char *dir, const char *rel) {
    char norm[MAX_PATHBUF], path[MAX_PATHBUF];
    snprintf(norm, sizeof(norm), "%s", rel ? rel : "");
    for (char *c = norm; *c; ++c) {
#ifdef PEX_PLATFORM_SDL2
        if (*c == '\\') *c = '/';
#else
        if (*c == '/') *c = '\\';
#endif
    }
    path_join(path, sizeof(path), dir, norm);
    return file_exists(path);
}

static int classic_pack_installed(void) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    return 1;
#endif
    char dir[MAX_PATHBUF];
    classic_pack_path(dir, sizeof(dir));
    return release_file_exists_in_dir(dir, "terrain.png") &&
           release_file_exists_in_dir(dir, "gui/gui.png") &&
           release_file_exists_in_dir(dir, "font/default.png") &&
           release_file_exists_in_dir(dir, "title/mclogo.png") &&
           release_file_exists_in_dir(dir, "title/mojang.png") &&
           release_file_exists_in_dir(dir, "title/bg/panorama0.png");
}

static int classic_pack_missing_required_textures(void) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    return 0;
#endif
    char dir[MAX_PATHBUF];
    static const char *required[] = {
        "terrain.png",
        "gui/gui.png",
        "gui/items.png",
        "gui/icons.png",
        "font/default.png",
        "title/mclogo.png",
        "title/mojang.png",
        "title/bg/panorama0.png",
        "title/bg/panorama1.png",
        "title/bg/panorama2.png",
        "title/bg/panorama3.png",
        "title/bg/panorama4.png",
        "title/bg/panorama5.png",
        "mob/pig.png",
        "mob/sheep.png",
        "mob/sheep_fur.png",
        "mob/cow.png",
        "mob/chicken.png",
        "mob/saddle.png"
    };
    if (!classic_pack_installed()) return 0;
    classic_pack_path(dir, sizeof(dir));
    for (int i = 0; i < (int)(sizeof(required) / sizeof(required[0])); ++i) {
        if (!release_file_exists_in_dir(dir, required[i])) return 1;
    }
    return 0;
}

static void classic_resources_path(char *out, size_t cap) {
    path_join(out, cap, g_mc_dir, "resources");
}

static void classic_sound_marker_path(char *out, size_t cap) {
    char root[MAX_PATHBUF];
    classic_resources_path(root, sizeof(root));
    path_join(out, cap, root, ".pexcraft-release-music");
}

static int classic_sounds_installed(void) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    return 1;
#else
    char marker[MAX_PATHBUF];
    char root[MAX_PATHBUF];
    char music_dir[MAX_PATHBUF];
    char menu_dir[MAX_PATHBUF];
    char menu2[MAX_PATHBUF];

    classic_sound_marker_path(marker, sizeof(marker));
    if (!file_exists(marker)) return 0;

    classic_resources_path(root, sizeof(root));
    path_join(music_dir, sizeof(music_dir), root, "music");
    path_join(menu_dir, sizeof(menu_dir), music_dir, "menu");
    path_join(menu2, sizeof(menu2), menu_dir, "menu2.ogg");
    return file_exists(menu2);
#endif
}

static int classic_wants_sound_download(void) {
#if PEX_CLASSIC_SOUND_DOWNLOAD_SUPPORTED
    return !classic_sounds_installed();
#else
    return 0;
#endif
}

static int classic_resources_need_update(void) {
    int missing_textures = !classic_pack_installed() || classic_pack_missing_required_textures();
    int missing_sounds = classic_wants_sound_download() && !classic_sounds_installed();
    if (missing_textures && !g_opts.ignore_classic_resources_warning) return 1;
    if (missing_sounds) return 1;
    return 0;
}

static int should_show_classic_pack_download_prompt(void) {
    return classic_resources_need_update();
}

static void classic_resource_missing_summary(char *out, size_t cap) {
    int missing_textures = !classic_pack_installed() || classic_pack_missing_required_textures();
    int missing_sounds = classic_wants_sound_download() && !classic_sounds_installed();
    if (missing_textures && missing_sounds) snprintf(out, cap, "Release textures and Moog City 2 should be downloaded.");
    else if (missing_textures) snprintf(out, cap, "Release textures should be downloaded.");
    else if (missing_sounds) snprintf(out, cap, "Moog City 2 should be downloaded.");
    else snprintf(out, cap, "Release resources are up to date.");
}

static void add_builtin_classic_texture_pack(int *wanted) {
    if (g_texpack_count >= MAX_TEXPACKS) return;
    TexturePackEntry *e = &g_texpacks[g_texpack_count++];
    memset(e, 0, sizeof(*e));
    snprintf(e->name, sizeof(e->name), "%s", CLASSIC_PACK_NAME);
    classic_pack_path(e->path, sizeof(e->path));
    snprintf(e->desc1, sizeof(e->desc1), "Minecraft 1.2.5 release textures");
    e->is_default = 1;
    int installed = classic_pack_installed();
    if (installed) {
        if (classic_pack_missing_required_textures()) snprintf(e->desc2, sizeof(e->desc2), "Missing item/block/mob textures");
        else if (classic_wants_sound_download() && !classic_sounds_installed()) snprintf(e->desc2, sizeof(e->desc2), "Missing Moog City 2");
        else if (classic_sounds_installed()) snprintf(e->desc2, sizeof(e->desc2), "Release textures + Moog City 2");
        else snprintf(e->desc2, sizeof(e->desc2), "Downloaded from 1.2.5 client.jar");
    } else {
        snprintf(e->desc2, sizeof(e->desc2), "Click to download Release resources");
    }
    e->is_builtin_classic = 1;
    /* Use the normal fallback icon slot so the built-in Release entry stays iconless only when missing. */
    e->no_icon = 0;
    if (installed && !strcmp(g_opts.skin, e->name)) *wanted = g_texpack_count - 1;
}

static void clamp_texpack_scroll(void) {
    int top = 32;
    int bottom = g_gui_h - 58 + 4;
    int max_scroll = g_texpack_count * 36 - (bottom - top - 4);
    if (max_scroll < 0) max_scroll /= 2;
    if (g_texpack_scroll < 0) g_texpack_scroll = 0;
    if (g_texpack_scroll > max_scroll) g_texpack_scroll = max_scroll;
}

static void scan_texture_packs(void) {
    free_texture_pack_icons();
    g_texpack_count = 0;
    int wanted = 0;

    add_builtin_classic_texture_pack(&wanted);

#if (defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY) || defined(PEX_PLATFORM_WII)
    /* Console assets are linked into the executable; do not scan or create texturepack folders. */
    g_selected_texpack = wanted;
    snprintf(g_current_texpack, sizeof(g_current_texpack), "%s", g_texpacks[g_selected_texpack].name);
    clamp_texpack_scroll();
    return;
#endif

    ensure_dir(g_texpack_dir);
    char pattern[MAX_PATHBUF];
    WIN32_FIND_DATAA fd;
#ifdef PEX_PLATFORM_SDL2
    snprintf(pattern, sizeof(pattern), "%s/*", g_texpack_dir);
#else
    snprintf(pattern, sizeof(pattern), "%s\\*", g_texpack_dir);
#endif
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
            if (strcmp(fd.cFileName, CLASSIC_PACK_NAME) == 0) continue;
            if (g_texpack_count >= MAX_TEXPACKS) break;
            TexturePackEntry *e = &g_texpacks[g_texpack_count++];
            memset(e, 0, sizeof(*e));
            snprintf(e->name, sizeof(e->name), "%s", fd.cFileName);
            path_join(e->path, sizeof(e->path), g_texpack_dir, fd.cFileName);
            read_pack_text(e);
            load_pack_icon(e);
            if (!strcmp(g_opts.skin, e->name)) wanted = g_texpack_count - 1;
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }

    g_selected_texpack = wanted;
    snprintf(g_current_texpack, sizeof(g_current_texpack), "%s", g_texpacks[g_selected_texpack].name);
    clamp_texpack_scroll();
}


static int make_solid_texture(Texture *t, int w, int h, unsigned char r, unsigned char g, unsigned char b, unsigned char a, int repeat) {
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;
    size_t n = (size_t)w * (size_t)h * 4u;
    unsigned char *rgba = (unsigned char *)malloc(n);
    if (!rgba) return 0;
    for (size_t i = 0; i < n; i += 4) {
        rgba[i + 0] = r;
        rgba[i + 1] = g;
        rgba[i + 2] = b;
        rgba[i + 3] = a;
    }
    return upload_rgba_texture(t, w, h, rgba, repeat);
}

static int try_release_texture(Texture *tex, const char *rel, int repeat) {
    char dir[MAX_PATHBUF], path[MAX_PATHBUF];
    classic_pack_path(dir, sizeof(dir));
    snprintf(path, sizeof(path), "%s\\%s", dir, rel);
    if (load_png_texture(tex, path, repeat)) return 1;
    snprintf(path, sizeof(path), "%s/%s", dir, rel);
    for (char *c = path; *c; ++c) if (*c == '\\') *c = '/';
    return load_png_texture(tex, path, repeat);
}

static int load_release_textures_from_pack(void) {
    if (!classic_pack_installed()) return 0;
    int ok = 1;
    ok = try_release_texture(&tex_bg, "gui\\background.png", 1) && ok;
    ok = try_release_texture(&tex_gui, "gui\\gui.png", 0) && ok;
    ok = try_release_texture(&tex_font, "font\\default.png", 0) && ok;
    ok = try_release_texture(&tex_terrain, "terrain.png", 1) && ok;
    make_solid_texture(&tex_black, 16, 16, 0, 0, 0, 255, 1);
    try_release_texture(&tex_pack, "pack.png", 0);
    if (!tex_pack.id) make_solid_texture(&tex_pack, 32, 32, 64, 64, 64, 255, 0);
    if (tex_pack.id && tex_pack.rgba) {
        Texture tmp = {0};
        size_t bytes = (size_t)tex_pack.w * (size_t)tex_pack.h * 4u;
        unsigned char *copy = (unsigned char*)malloc(bytes);
        if (copy) { memcpy(copy, tex_pack.rgba, bytes); upload_rgba_texture(&tmp, tex_pack.w, tex_pack.h, copy, 0); }
        tex_default_pack_icon = tmp;
    } else make_solid_texture(&tex_default_pack_icon, 32, 32, 64, 64, 64, 255, 0);
    if (tex_pack.id && tex_pack.rgba) {
        Texture tmp = {0};
        size_t bytes = (size_t)tex_pack.w * (size_t)tex_pack.h * 4u;
        unsigned char *copy = (unsigned char*)malloc(bytes);
        if (copy) { memcpy(copy, tex_pack.rgba, bytes); upload_rgba_texture(&tmp, tex_pack.w, tex_pack.h, copy, 0); }
        tex_unknown_pack = tmp;
    } else make_solid_texture(&tex_unknown_pack, 32, 32, 96, 96, 96, 255, 0);
    ok = try_release_texture(&tex_icons, "gui\\icons.png", 0) && ok;
    ok = try_release_texture(&tex_inventory, "gui\\inventory.png", 0) && ok;
    try_release_texture(&tex_workbench, "gui\\crafting.png", 0);
    try_release_texture(&tex_furnace_gui, "gui\\furnace.png", 0);
    try_release_texture(&tex_chest_gui, "gui\\container.png", 0);
    try_release_texture(&tex_chest_gui, "gui\\chest.png", 0);
    try_release_texture(&tex_chest_entity, "item\\chest.png", 0);
    try_release_texture(&tex_large_chest_entity, "item\\largechest.png", 0);
    try_release_texture(&tex_items, "gui\\items.png", 0);
    for (int m = 0; m < 5; m++) for (int l = 0; l < 2; l++) free_texture(&tex_armor[m][l]);
    try_release_texture(&tex_armor[0][0], "armor\\cloth_1.png", 0);
    try_release_texture(&tex_armor[0][1], "armor\\cloth_2.png", 0);
    try_release_texture(&tex_armor[1][0], "armor\\chain_1.png", 0);
    try_release_texture(&tex_armor[1][1], "armor\\chain_2.png", 0);
    try_release_texture(&tex_armor[2][0], "armor\\iron_1.png", 0);
    try_release_texture(&tex_armor[2][1], "armor\\iron_2.png", 0);
    try_release_texture(&tex_armor[3][0], "armor\\diamond_1.png", 0);
    try_release_texture(&tex_armor[3][1], "armor\\diamond_2.png", 0);
    try_release_texture(&tex_armor[4][0], "armor\\gold_1.png", 0);
    try_release_texture(&tex_armor[4][1], "armor\\gold_2.png", 0);
    ok = (try_release_texture(&tex_steve, "mob\\char.png", 0) || try_release_texture(&tex_steve, "char.png", 0)) && ok;
    try_release_texture(&tex_mob_pig, "mob\\pig.png", 0);
    try_release_texture(&tex_mob_sheep, "mob\\sheep.png", 0);
    try_release_texture(&tex_mob_sheep_fur, "mob\\sheep_fur.png", 0);
    try_release_texture(&tex_mob_cow, "mob\\cow.png", 0);
    try_release_texture(&tex_mob_chicken, "mob\\chicken.png", 0);
    try_release_texture(&tex_mob_saddle, "mob\\saddle.png", 0);
    try_release_texture(&tex_clouds, "environment\\clouds.png", 1);
    try_release_texture(&tex_sun, "terrain\\sun.png", 0);
    try_release_texture(&tex_moon, "terrain\\moon.png", 0);
    try_release_texture(&tex_moon_phases, "terrain\\moon_phases.png", 0);
    /* Source C++ RenderGlobal renders sun/moon with original texture alpha. */
    try_release_texture(&tex_water_overlay, "misc\\water.png", 1);
    try_release_texture(&tex_shadow, "misc\\shadow.png", 0);
    try_release_texture(&tex_grasscolor, "misc\\grasscolor.png", 0);
    try_release_texture(&tex_foliagecolor, "misc\\foliagecolor.png", 0);
    try_release_texture(&tex_particles, "particles.png", 0);
    ok = try_release_texture(&tex_title_logo, "title\\mclogo.png", 0) && ok;
    ok = try_release_texture(&tex_mojang, "title\\mojang.png", 0) && ok;
    set_texture_filter_wrap(&tex_title_logo, 0, 0);
    set_texture_filter_wrap(&tex_mojang, 1, 0);
    for (int i = 0; i < 6; ++i) {
        char rel[64];
        snprintf(rel, sizeof(rel), "title\\bg\\panorama%d.png", i);
        ok = try_release_texture(&tex_panorama[i], rel, 1) && ok;
        set_texture_filter_wrap(&tex_panorama[i], 0, 1);
    }
    return ok;
}

static int load_default_textures(void) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    /* Console builds must never stop booting just because an embedded texture is missing.
       Log every asset, then create a tiny fallback texture so we can reach the
       menu and debug the renderer/input on real hardware/PPSSPP. */
    int missing = 0;
#define PEX_PSP_LOAD_REQ(tex, file, repeat, fw, fh) do {         PEX_PSP_LOGF("LOAD_REQ %s", file);         if (!load_mcrw((tex), (file), (repeat))) {             ++missing;             PEX_PSP_LOGF("LOAD_REQ failed: %s; using fallback", file);             psp_make_fallback_texture((tex), (file), (fw), (fh), (repeat));         }     } while (0)
#define PEX_PSP_LOAD_OPT(tex, file, repeat, fw, fh) do {         PEX_PSP_LOGF("LOAD_OPT %s", file);         if (!load_mcrw((tex), (file), (repeat))) {             PEX_PSP_LOGF("LOAD_OPT missing: %s; using fallback", file);             psp_make_fallback_texture((tex), (file), (fw), (fh), (repeat));         }     } while (0)
    PEX_PSP_LOAD_REQ(&tex_bg, "gui_background.mcrw", 1, 16, 16);
    PEX_PSP_LOAD_REQ(&tex_gui, "gui_gui.mcrw", 0, 256, 256);
    PEX_PSP_LOAD_REQ(&tex_font, "font_default.mcrw", 0, 128, 128);
    PEX_PSP_LOAD_REQ(&tex_terrain, "terrain.mcrw", 1, 128, 128);
    PEX_PSP_LOAD_REQ(&tex_black, "title_black.mcrw", 1, 16, 16);
    PEX_PSP_LOAD_OPT(&tex_pack, "pack.mcrw", 0, 32, 32);
    PEX_PSP_LOAD_OPT(&tex_default_pack_icon, "pack.mcrw", 0, 32, 32);
    PEX_PSP_LOAD_OPT(&tex_unknown_pack, "unknown_pack.mcrw", 0, 32, 32);
    PEX_PSP_LOAD_REQ(&tex_icons, "gui_icons.mcrw", 0, 256, 256);
    PEX_PSP_LOAD_REQ(&tex_inventory, "gui_inventory.mcrw", 0, 256, 256);
    PEX_PSP_LOAD_OPT(&tex_workbench, "gui_crafting_table.mcrw", 0, 256, 256);
    PEX_PSP_LOAD_OPT(&tex_furnace_gui, "gui_furnace.mcrw", 0, 256, 256);
    PEX_PSP_LOAD_OPT(&tex_chest_gui, "gui_chest.mcrw", 0, 256, 256);
    free_texture(&tex_chest_entity);
    free_texture(&tex_large_chest_entity);
    free_texture(&tex_mob_pig);
    free_texture(&tex_mob_sheep);
    free_texture(&tex_mob_sheep_fur);
    free_texture(&tex_mob_cow);
    free_texture(&tex_mob_chicken);
    free_texture(&tex_mob_saddle);
    PEX_PSP_LOAD_OPT(&tex_items, "gui_items.mcrw", 0, 256, 256);
    PEX_PSP_LOAD_REQ(&tex_steve, "mob_char.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[0][0], "armor_cloth_1.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[0][1], "armor_cloth_2.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[1][0], "armor_chain_1.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[1][1], "armor_chain_2.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[2][0], "armor_iron_1.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[2][1], "armor_iron_2.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[3][0], "armor_diamond_1.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[3][1], "armor_diamond_2.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[4][0], "armor_gold_1.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_armor[4][1], "armor_gold_2.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_mob_pig, "mob_pig.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_mob_sheep, "mob_sheep.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_mob_sheep_fur, "mob_sheep_fur.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_mob_cow, "mob_cow.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_mob_chicken, "mob_chicken.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_OPT(&tex_mob_saddle, "mob_saddle.mcrw", 0, 64, 32);
    PEX_PSP_LOAD_REQ(&tex_clouds, "environment_clouds.mcrw", 1, 256, 256);
    PEX_PSP_LOAD_OPT(&tex_sun, "terrain_sun.mcrw", 0, 32, 32);
    PEX_PSP_LOAD_OPT(&tex_moon, "terrain_moon.mcrw", 0, 32, 32);
    PEX_PSP_LOAD_OPT(&tex_moon_phases, "terrain_moon_phases.mcrw", 0, 64, 32);
    /* Source C++ RenderGlobal renders sun/moon with original texture alpha. */
    PEX_PSP_LOAD_OPT(&tex_water_overlay, "misc_water.mcrw", 1, 256, 256);
    PEX_PSP_LOAD_OPT(&tex_shadow, "misc_shadow.mcrw", 0, 64, 64);
    PEX_PSP_LOAD_OPT(&tex_grasscolor, "misc_grasscolor.mcrw", 0, 256, 256);
    PEX_PSP_LOAD_OPT(&tex_foliagecolor, "misc_foliagecolor.mcrw", 0, 256, 256);
    PEX_PSP_LOAD_OPT(&tex_particles, "particles.mcrw", 0, 128, 128);
    psp_drop_gui_texture_cpu_copies();
    #if defined(PEX_PLATFORM_PSP)
    PEX_PSP_LOGF("load_default_textures PSP done: missing_required=%d embedded_count=%u free=%u", missing, psp_embedded_mcrw_count(), (unsigned)sceKernelTotalFreeMemSize());
#else
    PEX_PSP_LOGF("load_default_textures Wii done: missing_required=%d embedded_count=%u", missing, psp_embedded_mcrw_count());
#endif
#undef PEX_PSP_LOAD_REQ
#undef PEX_PSP_LOAD_OPT
    return 1;
#else
    if (load_release_textures_from_pack()) return 1;
    int ok = 1;
    ok = load_mcrw(&tex_bg, "gui_background.mcrw", 1) && ok;
    ok = load_mcrw(&tex_gui, "gui_gui.mcrw", 0) && ok;
    ok = load_mcrw(&tex_font, "font_default.mcrw", 0) && ok;
    ok = load_mcrw(&tex_terrain, "terrain.mcrw", 1) && ok;
    ok = load_mcrw(&tex_black, "title_black.mcrw", 1) && ok;
    load_mcrw(&tex_pack, "pack.mcrw", 0);
    load_mcrw(&tex_default_pack_icon, "pack.mcrw", 0);
    load_mcrw(&tex_unknown_pack, "unknown_pack.mcrw", 0);
    ok = load_mcrw(&tex_icons, "gui_icons.mcrw", 0) && ok;
    ok = load_mcrw(&tex_inventory, "gui_inventory.mcrw", 0) && ok;
    load_mcrw(&tex_workbench, "gui_crafting_table.mcrw", 0);
    load_mcrw(&tex_furnace_gui, "gui_furnace.mcrw", 0);
    load_mcrw(&tex_chest_gui, "gui_chest.mcrw", 0);
    free_texture(&tex_chest_entity);
    free_texture(&tex_large_chest_entity);
    free_texture(&tex_mob_pig);
    free_texture(&tex_mob_sheep);
    free_texture(&tex_mob_sheep_fur);
    free_texture(&tex_mob_cow);
    free_texture(&tex_mob_chicken);
    free_texture(&tex_mob_saddle);
    for (int m = 0; m < 5; m++) for (int l = 0; l < 2; l++) free_texture(&tex_armor[m][l]);
    load_mcrw(&tex_items, "gui_items.mcrw", 0);
    ok = load_mcrw(&tex_steve, "mob_char.mcrw", 0) && ok;
    load_mcrw(&tex_mob_pig, "mob_pig.mcrw", 0);
    load_mcrw(&tex_mob_sheep, "mob_sheep.mcrw", 0);
    load_mcrw(&tex_mob_sheep_fur, "mob_sheep_fur.mcrw", 0);
    load_mcrw(&tex_mob_cow, "mob_cow.mcrw", 0);
    load_mcrw(&tex_mob_chicken, "mob_chicken.mcrw", 0);
    load_mcrw(&tex_mob_saddle, "mob_saddle.mcrw", 0);
    load_mcrw(&tex_clouds, "environment_clouds.mcrw", 1);
    load_mcrw(&tex_sun, "terrain_sun.mcrw", 0);
    load_mcrw(&tex_moon, "terrain_moon.mcrw", 0);
    load_mcrw(&tex_moon_phases, "terrain_moon_phases.mcrw", 0);
    /* Source C++ RenderGlobal renders sun/moon with original texture alpha. */
    load_mcrw(&tex_water_overlay, "misc_water.mcrw", 1);
    load_mcrw(&tex_shadow, "misc_shadow.mcrw", 0);
    load_mcrw(&tex_grasscolor, "misc_grasscolor.mcrw", 0);
    load_mcrw(&tex_foliagecolor, "misc_foliagecolor.mcrw", 0);
    load_mcrw(&tex_particles, "particles.mcrw", 0);
    return ok;
#endif
}

static void try_pack_texture(TexturePackEntry *e, Texture *tex, const char *rel, int repeat) {
    char path[MAX_PATHBUF];
    snprintf(path, sizeof(path), "%s\\%s", e->path, rel);
    if (load_png_texture(tex, path, repeat)) return;
    snprintf(path, sizeof(path), "%s/%s", e->path, rel);
    for (char *c = path; *c; ++c) if (*c == '\\') *c = '/';
    load_png_texture(tex, path, repeat);
}

static void apply_texture_pack_index(int index) {
    if (index < 0 || index >= g_texpack_count) index = 0;
    if (!load_default_textures()) return;
    g_selected_texpack = index;
    snprintf(g_current_texpack, sizeof(g_current_texpack), "%s", g_texpacks[index].name);
    snprintf(g_opts.skin, sizeof(g_opts.skin), "%s", g_texpacks[index].name);
    save_options();
    if (!g_texpacks[index].is_default) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
        /* Console builds already use CI-converted Classic .mcrw assets as their
           default embedded textures.  No PNG texturepack files are needed. */
        init_font_widths();
        log_msg("Applied embedded console texture pack: %s", g_current_texpack);
        return;
#else
        TexturePackEntry *e = &g_texpacks[index];
        try_pack_texture(e, &tex_terrain, "terrain.png", 1);
        try_pack_texture(e, &tex_gui, "gui\\gui.png", 0);
        try_pack_texture(e, &tex_bg, "gui\\background.png", 1);
        try_pack_texture(e, &tex_font, "font\\default.png", 0);
        try_pack_texture(e, &tex_pack, "pack.png", 0);
        try_pack_texture(e, &tex_icons, "gui\\icons.png", 0);
        try_pack_texture(e, &tex_inventory, "gui\\inventory.png", 0);
        try_pack_texture(e, &tex_workbench, "gui\\crafting.png", 0);
        try_pack_texture(e, &tex_furnace_gui, "gui\\furnace.png", 0);
        try_pack_texture(e, &tex_chest_gui, "gui\\container.png", 0);
        try_pack_texture(e, &tex_chest_gui, "gui\\chest.png", 0);
        try_pack_texture(e, &tex_chest_entity, "item\\chest.png", 0);
        try_pack_texture(e, &tex_large_chest_entity, "item\\largechest.png", 0);
        try_pack_texture(e, &tex_items, "gui\\items.png", 0);
        try_pack_texture(e, &tex_armor[0][0], "armor\\cloth_1.png", 0);
        try_pack_texture(e, &tex_armor[0][1], "armor\\cloth_2.png", 0);
        try_pack_texture(e, &tex_armor[1][0], "armor\\chain_1.png", 0);
        try_pack_texture(e, &tex_armor[1][1], "armor\\chain_2.png", 0);
        try_pack_texture(e, &tex_armor[2][0], "armor\\iron_1.png", 0);
        try_pack_texture(e, &tex_armor[2][1], "armor\\iron_2.png", 0);
        try_pack_texture(e, &tex_armor[3][0], "armor\\diamond_1.png", 0);
        try_pack_texture(e, &tex_armor[3][1], "armor\\diamond_2.png", 0);
        try_pack_texture(e, &tex_armor[4][0], "armor\\gold_1.png", 0);
        try_pack_texture(e, &tex_armor[4][1], "armor\\gold_2.png", 0);
        try_pack_texture(e, &tex_steve, "mob\\char.png", 0);
        try_pack_texture(e, &tex_steve, "char.png", 0);
        try_pack_texture(e, &tex_mob_pig, "mob\\pig.png", 0);
        try_pack_texture(e, &tex_mob_sheep, "mob\\sheep.png", 0);
        try_pack_texture(e, &tex_mob_sheep_fur, "mob\\sheep_fur.png", 0);
        try_pack_texture(e, &tex_mob_cow, "mob\\cow.png", 0);
        try_pack_texture(e, &tex_mob_chicken, "mob\\chicken.png", 0);
        try_pack_texture(e, &tex_mob_saddle, "mob\\saddle.png", 0);
        try_pack_texture(e, &tex_clouds, "environment\\clouds.png", 1);
        try_pack_texture(e, &tex_sun, "terrain\\sun.png", 0);
        try_pack_texture(e, &tex_moon, "terrain\\moon.png", 0);
        try_pack_texture(e, &tex_moon_phases, "terrain\\moon_phases.png", 0);
        /* Source C++ sky keeps original sun/moon texture alpha. */
        try_pack_texture(e, &tex_water_overlay, "misc\\water.png", 1);
        try_pack_texture(e, &tex_shadow, "misc\\shadow.png", 0);
        try_pack_texture(e, &tex_grasscolor, "misc\\grasscolor.png", 0);
        try_pack_texture(e, &tex_foliagecolor, "misc\\foliagecolor.png", 0);
        try_pack_texture(e, &tex_particles, "particles.png", 0);
#endif
    }
    if (g_opts.skin_path[0]) load_custom_skin_path(g_opts.skin_path, 0);
    init_font_widths();
    log_msg("Applied texture pack: %s", g_current_texpack);
}

