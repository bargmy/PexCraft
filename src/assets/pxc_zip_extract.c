/* Internal ZIP/JAR extractor used by both Windows and Linux builds.
   Supports stored and deflated entries.  No powershell/unzip/find process. */
#include <zlib.h>

#ifndef PXC_ZIP_MAX_ENTRY_SIZE
#define PXC_ZIP_MAX_ENTRY_SIZE (64u * 1024u * 1024u)
#endif
#ifndef PXC_ZIP_MAX_ARCHIVE_SIZE
#define PXC_ZIP_MAX_ARCHIVE_SIZE (128u * 1024u * 1024u)
#endif

static uint16_t pxc_rd16(const unsigned char *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t pxc_rd32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int pxc_ascii_equal_ci_char(char a, char b) {
    if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
    if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
    return a == b;
}

static int pxc_ascii_starts_ci(const char *s, const char *prefix) {
    while (*prefix) {
        if (!*s || !pxc_ascii_equal_ci_char(*s, *prefix)) return 0;
        ++s; ++prefix;
    }
    return 1;
}

static int pxc_ascii_ends_ci(const char *s, const char *suffix) {
    size_t sl = strlen(s), tl = strlen(suffix);
    if (tl > sl) return 0;
    return pxc_ascii_starts_ci(s + sl - tl, suffix);
}

static void pxc_zip_make_output_path(char *out, size_t cap, const char *root, const char *name) {
    size_t n = 0;
    if (!out || cap == 0) return;
    out[0] = 0;
    if (!root) root = "";
    n = strlen(root);
    if (n >= cap) n = cap - 1;
    memcpy(out, root, n);
    out[n] = 0;
    if (n > 0 && out[n - 1] != '/' && out[n - 1] != '\\') {
#ifdef PEX_PLATFORM_SDL2
        if (n + 1 < cap) { out[n++] = '/'; out[n] = 0; }
#else
        if (n + 1 < cap) { out[n++] = '\\'; out[n] = 0; }
#endif
    }
    for (; name && *name && n + 1 < cap; ++name) {
        char c = *name;
        if (c == '/' || c == '\\') {
#ifdef PEX_PLATFORM_SDL2
            c = '/';
#else
            c = '\\';
#endif
        }
        out[n++] = c;
    }
    out[n] = 0;
}

static int pxc_zip_name_is_safe(const char *name) {
    const char *p;
    if (!name || !name[0]) return 0;
    if (name[0] == '/' || name[0] == '\\') return 0;
    if (name[1] == ':') return 0;
    for (p = name; *p; ++p) {
        if (*p == '\\') return 0;
        if ((p[0] == '.' && p[1] == '.' && (p[2] == '/' || p[2] == 0)) ||
            ((p == name || p[-1] == '/') && p[0] == '.' && p[1] == '.' && (p[2] == '/' || p[2] == 0))) return 0;
    }
    return 1;
}

static int pxc_zip_extract_lang_only = 0;
static int pxc_zip_write_pack_txt_after_extract = 1;

static int pxc_zip_entry_should_skip(const char *name) {
    if (!name || !name[0]) return 1;
    if (name[strlen(name) - 1] == '/') return 1;
    if (pxc_ascii_starts_ci(name, "META-INF/") || pxc_ascii_starts_ci(name, "META-INF\\")) return 1;
    if (pxc_ascii_ends_ci(name, ".class")) return 1;
    if (pxc_zip_extract_lang_only) {
        /* Language-only repair must also bring Java FontRenderer Unicode data.
           Minecraft 1.2.5 draws translated/non-Latin language names through
           /font/glyph_sizes.bin and /font/glyph_%02X.png; extracting only /lang
           leaves Russian, Persian, Arabic, Hebrew, Chinese, etc. present in
           languages.txt but impossible to render. */
        if (pxc_ascii_starts_ci(name, "lang/")) {
            if (!pxc_ascii_ends_ci(name, ".lang") && !pxc_ascii_ends_ci(name, "languages.txt")) return 1;
            return 0;
        }
        if (pxc_ascii_starts_ci(name, "font/")) {
            if (pxc_ascii_ends_ci(name, "font/default.png") ||
                pxc_ascii_ends_ci(name, "font/glyph_sizes.bin") ||
                (pxc_ascii_starts_ci(name, "font/glyph_") && pxc_ascii_ends_ci(name, ".png"))) return 0;
        }
        return 1;
    }
    return 0;
}

static void pxc_mkdirs_for_file(const char *path) {
    char tmp[MAX_PATHBUF];
    size_t i, len;
    if (!path || !path[0]) return;
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    for (i = 0; i < len; ++i) {
        if (tmp[i] == '/' || tmp[i] == '\\') {
            char old = tmp[i];
            if (i > 0) {
                tmp[i] = 0;
                ensure_dir(tmp);
                tmp[i] = old;
            }
        }
    }
}

static int pxc_write_file_all(const char *path, const unsigned char *data, size_t len) {
    FILE *f;
    pxc_mkdirs_for_file(path);
    f = fopen(path, "wb");
    if (!f) return 0;
    if (len && fwrite(data, 1, len, f) != len) { fclose(f); return 0; }
    fclose(f);
    return 1;
}

static int pxc_inflate_raw(const unsigned char *src, size_t src_len, unsigned char *dst, size_t dst_len) {
    z_stream zs;
    int ret;
    memset(&zs, 0, sizeof(zs));
    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)src_len;
    zs.next_out = dst;
    zs.avail_out = (uInt)dst_len;
    ret = inflateInit2(&zs, -MAX_WBITS);
    if (ret != Z_OK) return 0;
    ret = inflate(&zs, Z_FINISH);
    inflateEnd(&zs);
    return ret == Z_STREAM_END && zs.total_out == dst_len;
}

static int pxc_zip_extract_one(const unsigned char *zip, size_t zip_len,
                               const unsigned char *cd, const char *dest_dir,
                               char *err, size_t err_cap) {
    uint16_t method, flags, name_len, extra_len;
    uint32_t comp_size, uncomp_size, lho;
    const unsigned char *lh;
    char name[MAX_PATHBUF];
    char out_path[MAX_PATHBUF];
    const unsigned char *data;

    if (pxc_rd32(cd) != 0x02014b50u) return 0;
    flags = pxc_rd16(cd + 8);
    method = pxc_rd16(cd + 10);
    comp_size = pxc_rd32(cd + 20);
    uncomp_size = pxc_rd32(cd + 24);
    name_len = pxc_rd16(cd + 28);
    extra_len = pxc_rd16(cd + 30);
    (void)extra_len;
    lho = pxc_rd32(cd + 42);

    if (name_len == 0 || name_len >= sizeof(name)) return 1;
    if (46u + (uint32_t)name_len > (uint32_t)(zip_len - (size_t)(cd - zip))) return 0;
    memcpy(name, cd + 46, name_len);
    name[name_len] = 0;

    if (!pxc_zip_name_is_safe(name)) return 1;
    if (pxc_zip_entry_should_skip(name)) return 1;
    if (flags & 1u) {
        snprintf(err, err_cap, "Encrypted ZIP entry is not supported: %s", name);
        return 0;
    }
    if (uncomp_size > PXC_ZIP_MAX_ENTRY_SIZE || comp_size > PXC_ZIP_MAX_ENTRY_SIZE) {
        snprintf(err, err_cap, "ZIP entry too large: %s", name);
        return 0;
    }
    if ((size_t)lho + 30 > zip_len) return 0;
    lh = zip + lho;
    if (pxc_rd32(lh) != 0x04034b50u) return 0;
    {
        uint16_t lh_name_len = pxc_rd16(lh + 26);
        uint16_t lh_extra_len = pxc_rd16(lh + 28);
        size_t data_off = (size_t)lho + 30u + lh_name_len + lh_extra_len;
        if (data_off > zip_len || comp_size > zip_len - data_off) return 0;
        data = zip + data_off;
    }

    pxc_zip_make_output_path(out_path, sizeof(out_path), dest_dir, name);
    if (method == 0) {
        if (comp_size != uncomp_size) return 0;
        if (!pxc_write_file_all(out_path, data, comp_size)) {
            snprintf(err, err_cap, "Could not write %s", name);
            return 0;
        }
    } else if (method == 8) {
        unsigned char *out = NULL;
        int ok;
        if (uncomp_size == 0) return 1;
        out = (unsigned char *)malloc(uncomp_size);
        if (!out) {
            snprintf(err, err_cap, "Out of memory extracting %s", name);
            return 0;
        }
        ok = pxc_inflate_raw(data, comp_size, out, uncomp_size);
        if (ok) ok = pxc_write_file_all(out_path, out, uncomp_size);
        free(out);
        if (!ok) {
            snprintf(err, err_cap, "Could not extract %s", name);
            return 0;
        }
    } else {
        snprintf(err, err_cap, "Unsupported ZIP compression method %u for %s", (unsigned)method, name);
        return 0;
    }
    return 1;
}

static int pxc_write_classic_pack_txt(const char *pack_dir) {
    char path[MAX_PATHBUF];
    pxc_zip_make_output_path(path, sizeof(path), pack_dir, "pack.txt");
    const char *txt = "Release Textures\nDownloaded from Minecraft 1.2.5 client.jar\n";
    return pxc_write_file_all(path, (const unsigned char *)txt, strlen(txt));
}

static int pxc_extract_zip_file(const char *zip_path, const char *dest_dir, char *err, size_t err_cap) {
    FILE *f = NULL;
    unsigned char *zip = NULL;
    long flen_l;
    size_t zip_len;
    size_t search_start, i;
    size_t eocd = (size_t)-1;
    uint32_t cd_size, cd_off;
    uint16_t entries;
    const unsigned char *cd;
    int ok = 1;

    if (err && err_cap) err[0] = 0;
    f = fopen(zip_path, "rb");
    if (!f) { snprintf(err, err_cap, "Could not open downloaded client.jar"); return 0; }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    flen_l = ftell(f);
    if (flen_l <= 0 || (unsigned long)flen_l > PXC_ZIP_MAX_ARCHIVE_SIZE) {
        fclose(f);
        snprintf(err, err_cap, "Downloaded file size is invalid");
        return 0;
    }
    rewind(f);
    zip_len = (size_t)flen_l;
    zip = (unsigned char *)malloc(zip_len);
    if (!zip) { fclose(f); snprintf(err, err_cap, "Out of memory reading client.jar"); return 0; }
    if (fread(zip, 1, zip_len, f) != zip_len) {
        free(zip); fclose(f); snprintf(err, err_cap, "Could not read downloaded client.jar"); return 0;
    }
    fclose(f);

    search_start = zip_len > (22u + 0xffffu) ? zip_len - (22u + 0xffffu) : 0;
    if (zip_len >= 22) {
        i = zip_len - 22;
        for (;;) {
            if (pxc_rd32(zip + i) == 0x06054b50u) { eocd = i; break; }
            if (i == search_start) break;
            --i;
        }
    }
    if (eocd == (size_t)-1 || eocd + 22 > zip_len) {
        free(zip); snprintf(err, err_cap, "Downloaded file is not a ZIP/JAR archive"); return 0;
    }
    entries = pxc_rd16(zip + eocd + 10);
    cd_size = pxc_rd32(zip + eocd + 12);
    cd_off = pxc_rd32(zip + eocd + 16);
    if ((size_t)cd_off + cd_size > zip_len) {
        free(zip); snprintf(err, err_cap, "ZIP central directory is invalid"); return 0;
    }

    if (!pxc_zip_extract_lang_only) delete_recursive(dest_dir);
    ensure_dir(dest_dir);
    cd = zip + cd_off;
    for (uint16_t n = 0; n < entries; ++n) {
        uint16_t name_len, extra_len, comment_len;
        if ((size_t)(cd - zip) + 46 > zip_len || pxc_rd32(cd) != 0x02014b50u) { ok = 0; break; }
        name_len = pxc_rd16(cd + 28);
        extra_len = pxc_rd16(cd + 30);
        comment_len = pxc_rd16(cd + 32);
        if ((size_t)(cd - zip) + 46u + name_len + extra_len + comment_len > zip_len) { ok = 0; break; }
        if (!pxc_zip_extract_one(zip, zip_len, cd, dest_dir, err, err_cap)) { ok = 0; break; }
        cd += 46u + name_len + extra_len + comment_len;
    }

    free(zip);
    if (!ok) {
        if (err && err_cap && !err[0]) snprintf(err, err_cap, "Could not extract ZIP/JAR archive");
        return 0;
    }
    if (pxc_zip_write_pack_txt_after_extract && !pxc_write_classic_pack_txt(dest_dir)) {
        snprintf(err, err_cap, "Could not write pack.txt");
        return 0;
    }
    return 1;
}

static int pxc_extract_zip_lang_file(const char *zip_path, const char *dest_dir, char *err, size_t err_cap) {
    int old_lang = pxc_zip_extract_lang_only;
    int old_pack = pxc_zip_write_pack_txt_after_extract;
    int ok;
    pxc_zip_extract_lang_only = 1;
    pxc_zip_write_pack_txt_after_extract = 0;
    ensure_dir(dest_dir);
    ok = pxc_extract_zip_file(zip_path, dest_dir, err, err_cap);
    pxc_zip_extract_lang_only = old_lang;
    pxc_zip_write_pack_txt_after_extract = old_pack;
    return ok;
}
