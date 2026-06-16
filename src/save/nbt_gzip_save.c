/* Split from original monolithic main.c. Included by src/main.c unity build. */

static void bb_reserve(BinBuf *b, size_t extra) {
    size_t need = b->len + extra;
    if (need <= b->cap) return;
    size_t nc = b->cap ? b->cap * 2 : 1024;
    while (nc < need) nc *= 2;
    unsigned char *p = (unsigned char*)realloc(b->data, nc);
    if (!p) ExitProcess(2);
    b->data = p;
    b->cap = nc;
}

static void bb_put(BinBuf *b, const void *data, size_t n) {
    bb_reserve(b, n);
    memcpy(b->data + b->len, data, n);
    b->len += n;
}

static void bb_u8(BinBuf *b, unsigned int v) { unsigned char x = (unsigned char)v; bb_put(b, &x, 1); }
static void bb_u16be(BinBuf *b, unsigned int v) { unsigned char x[2] = {(unsigned char)(v >> 8), (unsigned char)v}; bb_put(b, x, 2); }
static void bb_u32be(BinBuf *b, unsigned int v) { unsigned char x[4] = {(unsigned char)(v >> 24), (unsigned char)(v >> 16), (unsigned char)(v >> 8), (unsigned char)v}; bb_put(b, x, 4); }
static void bb_i64be(BinBuf *b, long long v) {
    unsigned long long u = (unsigned long long)v;
    unsigned char x[8] = {(unsigned char)(u >> 56), (unsigned char)(u >> 48), (unsigned char)(u >> 40), (unsigned char)(u >> 32), (unsigned char)(u >> 24), (unsigned char)(u >> 16), (unsigned char)(u >> 8), (unsigned char)u};
    bb_put(b, x, 8);
}
static void bb_free(BinBuf *b) { free(b->data); b->data = NULL; b->len = b->cap = 0; }

static void nbt_name(BinBuf *b, const char *name) {
    size_t n = name ? strlen(name) : 0;
    if (n > 65535) n = 65535;
    bb_u16be(b, (unsigned int)n);
    if (n) bb_put(b, name, n);
}

static void nbt_tag(BinBuf *b, int type, const char *name) { bb_u8(b, (unsigned int)type); nbt_name(b, name); }
static void nbt_end(BinBuf *b) { bb_u8(b, 0); }
static void nbt_compound(BinBuf *b, const char *name) { nbt_tag(b, 10, name); }
static void nbt_byte(BinBuf *b, const char *name, int v) { nbt_tag(b, 1, name); bb_u8(b, (unsigned int)v); }
static void nbt_int(BinBuf *b, const char *name, int v) { nbt_tag(b, 3, name); bb_u32be(b, (unsigned int)v); }
static void nbt_long(BinBuf *b, const char *name, long long v) { nbt_tag(b, 4, name); bb_i64be(b, v); }
static void nbt_string(BinBuf *b, const char *name, const char *value) { nbt_tag(b, 8, name); nbt_name(b, value ? value : ""); }
static void nbt_byte_array(BinBuf *b, const char *name, const unsigned char *data, int len) { nbt_tag(b, 7, name); bb_u32be(b, (unsigned int)len); if (len > 0) bb_put(b, data, (size_t)len); }
static void nbt_empty_compound_list(BinBuf *b, const char *name) { nbt_tag(b, 9, name); bb_u8(b, 10); bb_u32be(b, 0); }

static void crc32_init(void) {
    if (g_crc_table_ready) return;
    for (unsigned int i = 0; i < 256; i++) {
        unsigned int c = i;
        for (int k = 0; k < 8; k++) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        g_crc_table[i] = c;
    }
    g_crc_table_ready = 1;
}

static unsigned int crc32_bytes(const unsigned char *data, size_t len) {
    crc32_init();
    unsigned int c = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) c = g_crc_table[(c ^ data[i]) & 255] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

static void write_le16(FILE *f, unsigned int v) { fputc(v & 255, f); fputc((v >> 8) & 255, f); }
static void write_le32(FILE *f, unsigned int v) { fputc(v & 255, f); fputc((v >> 8) & 255, f); fputc((v >> 16) & 255, f); fputc((v >> 24) & 255, f); }
static void write_be64_file(FILE *f, long long v) {
    unsigned long long u = (unsigned long long)v;
    for (int i = 7; i >= 0; i--) fputc((int)((u >> (i * 8)) & 255), f);
}

static int write_gzip_store(const char *path, const unsigned char *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    unsigned char header[10] = {0x1F,0x8B,8,0,0,0,0,0,0,0xFF};
    fwrite(header, 1, 10, f);
    size_t pos = 0;
    while (pos < len || (len == 0 && pos == 0)) {
        size_t remain = len - pos;
        unsigned int block = (unsigned int)(remain > 65535 ? 65535 : remain);
        int final_block = (pos + block >= len);
        fputc(final_block ? 0x01 : 0x00, f);
        write_le16(f, block);
        write_le16(f, (~block) & 0xFFFFu);
        if (block) fwrite(data + pos, 1, block, f);
        pos += block;
        if (len == 0) break;
    }
    write_le32(f, crc32_bytes(data, len));
    write_le32(f, (unsigned int)(len & 0xFFFFFFFFu));
    fclose(f);
    return 1;
}

static int make_dir_recursive(const char *path) {
    char tmp[MAX_PATHBUF];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp; *p; p++) {
        if (*p == '/' || *p == '\\') {
            char old = *p;
            *p = 0;
            if (tmp[0] && !dir_exists(tmp)) CreateDirectoryA(tmp, NULL);
            *p = old;
        }
    }
    if (!dir_exists(tmp)) CreateDirectoryA(tmp, NULL);
    return dir_exists(tmp);
}

static void base36_i32(int value, char *out, size_t cap) {
    const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    char buf[64];
    unsigned int v;
    int neg = value < 0;
    int i = 0;
    if (neg) v = (unsigned int)(-(long long)value); else v = (unsigned int)value;
    do { buf[i++] = digits[v % 36u]; v /= 36u; } while (v && i < (int)sizeof(buf)-2);
    int o = 0;
    if (neg && o < (int)cap - 1) out[o++] = '-';
    while (i > 0 && o < (int)cap - 1) out[o++] = buf[--i];
    out[o] = 0;
}



static int write_level_dat(const char *world_dir, const char *world_name, long long seed, int spawn_x, int spawn_y, int spawn_z, long long size_on_disk) {
    BinBuf b = {0};
    bb_u8(&b, 10); nbt_name(&b, "");
    nbt_compound(&b, "Data");
    nbt_long(&b, "RandomSeed", seed);
    nbt_int(&b, "SpawnX", spawn_x);
    nbt_int(&b, "SpawnY", spawn_y);
    nbt_int(&b, "SpawnZ", spawn_z);
    nbt_long(&b, "Time", 0);
    nbt_long(&b, "SizeOnDisk", size_on_disk);
    nbt_long(&b, "LastPlayed", (long long)time(NULL) * 1000LL);
    nbt_string(&b, "LevelName", world_name);
    nbt_int(&b, "version", 19132);
    nbt_end(&b);
    nbt_end(&b);
    char path[MAX_PATHBUF];
    snprintf(path, sizeof(path), "%s\\level.dat", world_dir);
    int ok = write_gzip_store(path, b.data, b.len);
    bb_free(&b);
    return ok;
}

static int write_session_lock(const char *world_dir) {
    char path[MAX_PATHBUF];
    snprintf(path, sizeof(path), "%s\\session.lock", world_dir);
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    write_be64_file(f, (long long)time(NULL) * 1000LL);
    fclose(f);
    return 1;
}

/* -------------------------------------------------------------------------
   Source-based Beta 1.0 overworld generator port.

   Ported from the provided decompiled sources:
   - qm.java  : overworld chunk provider base terrain + biome surfaces + populate sequence
   - mp.java/dr.java : cave map generation
   - nt.java/aa.java : octave Perlin noise
   - qd.java/nu.java/ay.java/gq.java : Beta biome/noise manager

   The menu still returns to title after Done! because rendering/gameplay is not
   part of this UI port, but the files written here are old Beta-format chunks.
   ------------------------------------------------------------------------- */
