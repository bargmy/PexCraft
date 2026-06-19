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

#if defined(PEX_PLATFORM_PSP)
/* Generated in GitHub Actions as build/psp_generated/psp_mcrw_assets.pak
   and embedded into EBOOT.PBP through build/psp_generated/psp_mcrw_assets.S.
   This avoids committing a giant generated 0xNN C-array file. */
extern const unsigned char pexcraft_psp_mcrw_assets_pak_start[];
extern const unsigned char pexcraft_psp_mcrw_assets_pak_end[];
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->rgba);
    return t->id != 0;
}

#if defined(PEX_PLATFORM_PSP)
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

static const unsigned char *psp_mcrw_pak_data(size_t *out_len) {
    const unsigned char *start = pexcraft_psp_mcrw_assets_pak_start;
    const unsigned char *end = pexcraft_psp_mcrw_assets_pak_end;
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
#if defined(PEX_PLATFORM_PSP)
    if (load_embedded_mcrw(t, filename, repeat)) return 1;
#endif
    char path[MAX_PATHBUF];
#if defined(PEX_PLATFORM_PSP)
    snprintf(path, sizeof(path), "assets/%s", filename);
    if (load_mcrw_path(t, path, repeat)) return 1;
    snprintf(path, sizeof(path), "./assets/%s", filename);
#else
    snprintf(path, sizeof(path), "assets\\%s", filename);
    if (load_mcrw_path(t, path, repeat)) return 1;
    snprintf(path, sizeof(path), ".\\assets\\%s", filename);
#endif
    return load_mcrw_path(t, path, repeat);
}

#if defined(PEX_PLATFORM_PSP)
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
    snprintf(txt, sizeof(txt), "%s\\pack.txt", e->path);
    FILE *f = fopen(txt, "r");
    if (!f) return;
    if (fgets(e->desc1, sizeof(e->desc1), f)) trim_pack_line_34(e->desc1);
    if (fgets(e->desc2, sizeof(e->desc2), f)) trim_pack_line_34(e->desc2);
    fclose(f);
}

static void load_pack_icon(TexturePackEntry *e) {
    char path[MAX_PATHBUF];
    snprintf(path, sizeof(path), "%s\\pack.png", e->path);
    if (load_png_texture(&e->icon, path, 0)) e->has_icon = 1;
}


static void classic_pack_path(char *out, size_t cap) {
    snprintf(out, cap, "%s\\%s", g_texpack_dir, CLASSIC_PACK_NAME);
}

static int classic_pack_installed(void) {
#if defined(PEX_PLATFORM_PSP)
    return 1;
#endif
    char dir[MAX_PATHBUF], terrain[MAX_PATHBUF], gui[MAX_PATHBUF], font[MAX_PATHBUF];
    classic_pack_path(dir, sizeof(dir));
    snprintf(terrain, sizeof(terrain), "%s\\terrain.png", dir);
    snprintf(gui, sizeof(gui), "%s\\gui\\gui.png", dir);
    snprintf(font, sizeof(font), "%s\\font\\default.png", dir);
    return file_exists(terrain) && file_exists(gui) && file_exists(font);
}

static int classic_pack_missing_required_textures(void) {
#if defined(PEX_PLATFORM_PSP)
    return 0;
#endif
    char dir[MAX_PATHBUF], items[MAX_PATHBUF], terrain[MAX_PATHBUF];
    if (!classic_pack_installed()) return 0;
    classic_pack_path(dir, sizeof(dir));
    snprintf(items, sizeof(items), "%s\\gui\\items.png", dir);
    snprintf(terrain, sizeof(terrain), "%s\\terrain.png", dir);
    /* The newly exposed block/item stubs need the downloaded terrain atlas plus
       gui/items.png.  Old saved Classic downloads can have terrain/gui/font
       only, which makes tools/items fall back or render as unrelated tiles. */
    if (!file_exists(items)) return 1;
    if (!file_exists(terrain)) return 1;
    return 0;
}

static int should_show_classic_pack_download_prompt(void) {
    if (g_opts.ignore_classic_resources_warning) return 0;
    if (classic_pack_installed()) return 0;
    return 1;
}

static void add_builtin_classic_texture_pack(int *wanted) {
    if (g_texpack_count >= MAX_TEXPACKS) return;
    TexturePackEntry *e = &g_texpacks[g_texpack_count++];
    memset(e, 0, sizeof(*e));
    snprintf(e->name, sizeof(e->name), "%s", CLASSIC_PACK_NAME);
    classic_pack_path(e->path, sizeof(e->path));
    snprintf(e->desc1, sizeof(e->desc1), "Minecraft Classic texture pack");
    int installed = classic_pack_installed();
    if (installed) {
        if (classic_pack_missing_required_textures()) snprintf(e->desc2, sizeof(e->desc2), "Missing item/block textures");
        else snprintf(e->desc2, sizeof(e->desc2), "Downloaded from client.jar");
    } else {
        snprintf(e->desc2, sizeof(e->desc2), "Click to download client.jar");
    }
    e->is_builtin_classic = 1;
    /* Use the normal fallback icon slot so the built-in Minecraft Classic entry
       shows the bundled unknown-pack icon instead of being iconless. */
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

    TexturePackEntry *d = &g_texpacks[g_texpack_count++];
    memset(d, 0, sizeof(*d));
    snprintf(d->name, sizeof(d->name), "Default");
    snprintf(d->desc1, sizeof(d->desc1), "The default look of PEXCRAFT");
    d->desc2[0] = 0;
    d->is_default = 1;

    add_builtin_classic_texture_pack(&wanted);

#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    /* PSP assets are linked into EBOOT.PBP; do not scan or create texturepack folders. */
    g_selected_texpack = wanted;
    snprintf(g_current_texpack, sizeof(g_current_texpack), "%s", g_texpacks[g_selected_texpack].name);
    clamp_texpack_scroll();
    return;
#endif

    ensure_dir(g_texpack_dir);
    char pattern[MAX_PATHBUF];
    WIN32_FIND_DATAA fd;
    snprintf(pattern, sizeof(pattern), "%s\\*", g_texpack_dir);
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
            snprintf(e->path, sizeof(e->path), "%s\\%s", g_texpack_dir, fd.cFileName);
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

static int load_default_textures(void) {
#if defined(PEX_PLATFORM_PSP)
    /* PSP must never stop booting just because an embedded texture is missing.
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
    PEX_PSP_LOAD_OPT(&tex_items, "gui_items.mcrw", 0, 256, 256);
    PEX_PSP_LOAD_REQ(&tex_steve, "mob_char.mcrw", 0, 64, 32);
    PEX_PSP_LOGF("load_default_textures PSP done: missing_required=%d embedded_count=%u", missing, psp_embedded_mcrw_count());
#undef PEX_PSP_LOAD_REQ
#undef PEX_PSP_LOAD_OPT
    return 1;
#else
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
    load_mcrw(&tex_items, "gui_items.mcrw", 0);
    ok = load_mcrw(&tex_steve, "mob_char.mcrw", 0) && ok;
    return ok;
#endif
}

static void try_pack_texture(TexturePackEntry *e, Texture *tex, const char *rel, int repeat) {
    char path[MAX_PATHBUF];
    snprintf(path, sizeof(path), "%s\\%s", e->path, rel);
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
#if defined(PEX_PLATFORM_PSP)
        /* The PSP build already uses CI-converted Classic .mcrw assets as its
           default embedded textures.  No PNG texturepack files are needed. */
        init_font_widths();
        log_msg("Applied embedded PSP texture pack: %s", g_current_texpack);
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
        try_pack_texture(e, &tex_steve, "mob\\char.png", 0);
        try_pack_texture(e, &tex_steve, "char.png", 0);
#endif
    }
    if (g_opts.skin_path[0]) load_custom_skin_path(g_opts.skin_path, 0);
    init_font_widths();
    log_msg("Applied texture pack: %s", g_current_texpack);
}

