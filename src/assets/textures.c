/* Split from original monolithic main.c. Included by src/main.c unity build. */

static int read_u32_le(FILE *f) {
    unsigned char b[4];
    if (fread(b, 1, 4, f) != 4) return 0;
    return (int)(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
}

static int file_exists(const char *path) {
    DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

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
    char path[MAX_PATHBUF];
    snprintf(path, sizeof(path), "assets\\%s", filename);
    if (load_mcrw_path(t, path, repeat)) return 1;
    snprintf(path, sizeof(path), ".\\assets\\%s", filename);
    return load_mcrw_path(t, path, repeat);
}

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

    ensure_dir(g_texpack_dir);
    char pattern[MAX_PATHBUF];
    WIN32_FIND_DATAA fd;
    snprintf(pattern, sizeof(pattern), "%s\\*", g_texpack_dir);
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
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
    load_mcrw(&tex_items, "gui_items.mcrw", 0);
    ok = load_mcrw(&tex_steve, "mob_char.mcrw", 0) && ok;
    return ok;
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
        TexturePackEntry *e = &g_texpacks[index];
        try_pack_texture(e, &tex_terrain, "terrain.png", 1);
        try_pack_texture(e, &tex_gui, "gui\\gui.png", 0);
        try_pack_texture(e, &tex_bg, "gui\\background.png", 1);
        try_pack_texture(e, &tex_font, "font\\default.png", 0);
        try_pack_texture(e, &tex_pack, "pack.png", 0);
        try_pack_texture(e, &tex_icons, "gui\\icons.png", 0);
        try_pack_texture(e, &tex_inventory, "gui\\inventory.png", 0);
        try_pack_texture(e, &tex_items, "gui\\items.png", 0);
        try_pack_texture(e, &tex_steve, "mob\\char.png", 0);
        try_pack_texture(e, &tex_steve, "char.png", 0);
    }
    init_font_widths();
    log_msg("Applied texture pack: %s", g_current_texpack);
}

