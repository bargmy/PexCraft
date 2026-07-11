#include "protocol_47je.h"
#include <zlib.h>

#define J47_RX_MAX (8u * 1024u * 1024u)
#define J47_TX_MAX (2u * 1024u * 1024u)
#define J47_PACKET_MAX (2u * 1024u * 1024u)
#define J47_PROFILE_MAX 128
#define J47_ENTITY_MAX 512
#define J47_WINDOW_MAX_SLOTS 128
#define J47_ITEM_NBT_MAX 2048
#define J47_ITEM_NAME_MAX 192
#define J47_ITEM_LORE_MAX 8
#define J47_ITEM_LORE_LINE_MAX 192

/* The Java adapter is unity-included after game/mobs.c and before
   platform/multiplayer_client.c. These declarations bind the protocol layer to
   the existing PexCraft multiplayer presentation helpers without duplicating
   gameplay systems. */
static void pex_net_mark_chunk_ready(int chunk_x, int chunk_z);
static PexNetRenderPlayerState *pex_net_find_render_player(int player_id);
static void pex_net_java_server_closed_window(void);

typedef struct J47Writer {
    unsigned char *data;
    size_t len;
    size_t cap;
    int failed;
} J47Writer;

typedef struct J47Reader {
    const unsigned char *data;
    size_t len;
    size_t pos;
    int failed;
} J47Reader;

typedef struct J47Profile {
    int used;
    unsigned char uuid[16];
    char name[32];
} J47Profile;

typedef struct J47RawItem {
    int id;
    int count;
    int damage;
    int has_enchantment;
    char display_name[J47_ITEM_NAME_MAX];
    int lore_count;
    char lore[J47_ITEM_LORE_MAX][J47_ITEM_LORE_LINE_MAX];
    size_t nbt_len;
    unsigned char nbt[J47_ITEM_NBT_MAX];
} J47RawItem;

typedef enum J47EntityKind {
    J47_ENTITY_NONE = 0,
    J47_ENTITY_PLAYER,
    J47_ENTITY_MOB,
    J47_ENTITY_DROP,
    J47_ENTITY_OTHER
} J47EntityKind;

typedef struct J47Entity {
    int used;
    int entity_id;
    J47EntityKind kind;
    int java_type;
    int player_slot;
    int mob_slot;
    int drop_slot;
    int held_item_id;
    int held_item_count;
    int held_item_damage;
    int held_slot;
    int is_armor_stand;
    int metadata_flags;
    int proxy_item_drop_slot;
    ItemStack equipment[5];
    char custom_name[96];
    double x, y, z;
    float yaw, pitch, head_yaw;
} J47Entity;

typedef struct J47Window {
    int open;
    int window_id;
    int container_slots;
    int total_slots;
    short action_number;
    char type[40];
    char title[96];
    ItemStack slots[J47_WINDOW_MAX_SLOTS];
    J47RawItem raw_slots[J47_WINDOW_MAX_SLOTS];
} J47Window;

typedef struct PexJava47Session {
    SOCKET socket;
    PexJava47State state;
    int connect_pending;
    int active;
    int peer_closed;
    int has_join_game;
    int world_ready;
    int server_protocol;
    int compression_threshold;
    int dimension;
    int game_mode;
    int difficulty;
    int local_entity_id;
    int chunks_received;
    int position_received;
    int progress;
    int held_slot;
    int last_held_slot;
    int last_sneaking;
    int last_sprinting;
    int last_abilities_flags;
    int movement_ticks;
    int last_movement_game_tick;
    int last_on_ground;
    double last_x, last_y, last_z;
    float last_yaw, last_pitch;
    double connect_started;
    double last_move_sent;
    char host[256];
    int port;
    char username[17];
    char status[128];
    char disconnect_reason[256];
    unsigned char *rx;
    size_t rx_len;
    size_t rx_cap;
    unsigned char *tx;
    size_t tx_len;
    size_t tx_pos;
    size_t tx_cap;
    J47Profile profiles[J47_PROFILE_MAX];
    J47Entity entities[J47_ENTITY_MAX];
    J47RawItem raw_player_slots[46];
    J47RawItem raw_cursor;
    J47Window window;
} PexJava47Session;

static PexJava47Session g_j47;

static int j47_socket_would_block(void) {
    int e = WSAGetLastError();
    return e == WSAEWOULDBLOCK || e == WSAEINPROGRESS || e == WSAEALREADY;
}

static int j47_wait_socket(SOCKET s, int write_ready, int timeout_ms) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(s, &set);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    return write_ready ? select((int)s + 1, NULL, &set, NULL, &tv) : select((int)s + 1, &set, NULL, NULL, &tv);
}

static SOCKET j47_connect_timeout(const char *host, int port, int timeout_ms) {
    char port_text[16];
    snprintf(port_text, sizeof(port_text), "%d", port);
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo(host, port_text, &hints, &result) != 0 || !result) return INVALID_SOCKET;

    SOCKET out = INVALID_SOCKET;
    for (struct addrinfo *rp = result; rp; rp = rp->ai_next) {
        SOCKET s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (s == INVALID_SOCKET) continue;
        BOOL yes = TRUE;
        int sock_buf = 1024 * 1024;
        setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *)&yes, sizeof(yes));
        setsockopt(s, SOL_SOCKET, SO_SNDBUF, (const char *)&sock_buf, sizeof(sock_buf));
        setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char *)&sock_buf, sizeof(sock_buf));
        u_long nonblock = 1;
        ioctlsocket(s, FIONBIO, &nonblock);
        int cr = connect(s, rp->ai_addr, (int)rp->ai_addrlen);
        if (cr == 0) { out = s; break; }
        if (cr == SOCKET_ERROR && j47_socket_would_block()) {
            int sr = j47_wait_socket(s, 1, timeout_ms);
            if (sr > 0) {
                int err = 0;
#if defined(_WIN32) && !defined(PEX_PLATFORM_SDL2) && !defined(PEX_PLATFORM_ANDROID) && !defined(PEX_PLATFORM_ANDROID_TV)
                int err_len = (int)sizeof(err);
#else
                socklen_t err_len = (socklen_t)sizeof(err);
#endif
                getsockopt(s, SOL_SOCKET, SO_ERROR, (char *)&err, &err_len);
                if (err == 0) { out = s; break; }
            }
        }
        closesocket(s);
    }
    freeaddrinfo(result);
    return out;
}

static int j47_send_all_timeout(SOCKET s, const unsigned char *data, size_t len, int timeout_ms) {
    size_t off = 0;
    double started = now_seconds();
    while (off < len) {
        int n = send(s, (const char *)data + off, (int)(len - off), 0);
        if (n > 0) { off += (size_t)n; continue; }
        if (n == 0) return 0;
        if (!j47_socket_would_block()) return 0;
        int remaining = timeout_ms - (int)((now_seconds() - started) * 1000.0);
        if (remaining <= 0 || j47_wait_socket(s, 1, remaining) <= 0) return 0;
    }
    return 1;
}

static int j47_recv_some_timeout(SOCKET s, unsigned char *data, size_t cap, int timeout_ms) {
    if (j47_wait_socket(s, 0, timeout_ms) <= 0) return -1;
    int n = recv(s, (char *)data, (int)cap, 0);
    return n;
}

static void j47_writer_init(J47Writer *w, size_t initial) {
    memset(w, 0, sizeof(*w));
    if (initial < 64) initial = 64;
    w->data = (unsigned char *)malloc(initial);
    if (!w->data) w->failed = 1;
    else w->cap = initial;
}

static void j47_writer_free(J47Writer *w) {
    if (!w) return;
    free(w->data);
    memset(w, 0, sizeof(*w));
}

static int j47_writer_reserve(J47Writer *w, size_t add) {
    if (!w || w->failed || add > SIZE_MAX - w->len) return 0;
    size_t need = w->len + add;
    if (need <= w->cap) return 1;
    size_t cap = w->cap ? w->cap : 64;
    while (cap < need) {
        if (cap > SIZE_MAX / 2) { w->failed = 1; return 0; }
        cap *= 2;
    }
    unsigned char *p = (unsigned char *)realloc(w->data, cap);
    if (!p) { w->failed = 1; return 0; }
    w->data = p;
    w->cap = cap;
    return 1;
}

static void j47_w_bytes(J47Writer *w, const void *data, size_t len) {
    if (!j47_writer_reserve(w, len)) return;
    if (len) memcpy(w->data + w->len, data, len);
    w->len += len;
}

static void j47_w_u8(J47Writer *w, unsigned int v) {
    unsigned char b = (unsigned char)v;
    j47_w_bytes(w, &b, 1);
}

static void j47_w_be16(J47Writer *w, int v) {
    uint16_t x = (uint16_t)v;
    unsigned char b[2] = {(unsigned char)(x >> 8), (unsigned char)x};
    j47_w_bytes(w, b, sizeof(b));
}

static void j47_w_be32(J47Writer *w, int32_t v) {
    uint32_t x = (uint32_t)v;
    unsigned char b[4] = {
        (unsigned char)(x >> 24), (unsigned char)(x >> 16),
        (unsigned char)(x >> 8), (unsigned char)x
    };
    j47_w_bytes(w, b, sizeof(b));
}

static void j47_w_be64(J47Writer *w, int64_t v) {
    uint64_t x = (uint64_t)v;
    unsigned char b[8];
    for (int i = 0; i < 8; i++) b[i] = (unsigned char)(x >> (56 - i * 8));
    j47_w_bytes(w, b, sizeof(b));
}

static void j47_w_float(J47Writer *w, float v) {
    uint32_t bits = 0;
    memcpy(&bits, &v, sizeof(bits));
    j47_w_be32(w, (int32_t)bits);
}

static void j47_w_double(J47Writer *w, double v) {
    uint64_t bits = 0;
    memcpy(&bits, &v, sizeof(bits));
    j47_w_be64(w, (int64_t)bits);
}

static void j47_w_varint(J47Writer *w, int32_t value) {
    uint32_t v = (uint32_t)value;
    do {
        unsigned char b = (unsigned char)(v & 0x7fu);
        v >>= 7;
        if (v) b |= 0x80u;
        j47_w_u8(w, b);
    } while (v && !w->failed);
}

static void j47_w_string_n(J47Writer *w, const char *s, size_t max_bytes) {
    if (!s) s = "";
    size_t n = strlen(s);
    if (n > max_bytes) n = max_bytes;
    j47_w_varint(w, (int32_t)n);
    j47_w_bytes(w, s, n);
}

static void j47_w_string(J47Writer *w, const char *s) {
    j47_w_string_n(w, s, 32767);
}

static void j47_reader_init(J47Reader *r, const unsigned char *data, size_t len) {
    r->data = data;
    r->len = len;
    r->pos = 0;
    r->failed = 0;
}

static int j47_r_need(J47Reader *r, size_t n) {
    if (!r || r->failed || n > r->len - r->pos) {
        if (r) r->failed = 1;
        return 0;
    }
    return 1;
}

static unsigned int j47_r_u8(J47Reader *r) {
    if (!j47_r_need(r, 1)) return 0;
    return r->data[r->pos++];
}


static int16_t j47_r_be16(J47Reader *r) {
    if (!j47_r_need(r, 2)) return 0;
    uint16_t x = ((uint16_t)r->data[r->pos] << 8) | (uint16_t)r->data[r->pos + 1];
    r->pos += 2;
    return (int16_t)x;
}

static int32_t j47_r_be32(J47Reader *r) {
    if (!j47_r_need(r, 4)) return 0;
    uint32_t x = ((uint32_t)r->data[r->pos] << 24) |
                 ((uint32_t)r->data[r->pos + 1] << 16) |
                 ((uint32_t)r->data[r->pos + 2] << 8) |
                 (uint32_t)r->data[r->pos + 3];
    r->pos += 4;
    return (int32_t)x;
}

static int64_t j47_r_be64(J47Reader *r) {
    if (!j47_r_need(r, 8)) return 0;
    uint64_t x = 0;
    for (int i = 0; i < 8; i++) x = (x << 8) | (uint64_t)r->data[r->pos + i];
    r->pos += 8;
    return (int64_t)x;
}

static float j47_r_float(J47Reader *r) {
    uint32_t x = (uint32_t)j47_r_be32(r);
    float v = 0.0f;
    memcpy(&v, &x, 4);
    return v;
}

static double j47_r_double(J47Reader *r) {
    uint64_t x = (uint64_t)j47_r_be64(r);
    double v = 0.0;
    memcpy(&v, &x, 8);
    return v;
}

static int j47_r_varint(J47Reader *r, int32_t *out) {
    uint32_t result = 0;
    int shift = 0;
    for (int i = 0; i < 5; ++i) {
        if (!j47_r_need(r, 1)) return 0;
        unsigned char b = r->data[r->pos++];
        result |= (uint32_t)(b & 0x7fu) << shift;
        if (!(b & 0x80u)) {
            if (out) *out = (int32_t)result;
            return 1;
        }
        shift += 7;
    }
    r->failed = 1;
    return 0;
}

static int j47_decode_varint_at(const unsigned char *data, size_t len, size_t *used, int32_t *out) {
    uint32_t result = 0;
    int shift = 0;
    for (size_t i = 0; i < len && i < 5; ++i) {
        unsigned char b = data[i];
        result |= (uint32_t)(b & 0x7fu) << shift;
        if (!(b & 0x80u)) {
            if (used) *used = i + 1;
            if (out) *out = (int32_t)result;
            return 1;
        }
        shift += 7;
    }
    return 0;
}

static int j47_r_string(J47Reader *r, char *out, size_t cap) {
    int32_t n = 0;
    if (!j47_r_varint(r, &n) || n < 0 || (size_t)n > J47_PACKET_MAX || !j47_r_need(r, (size_t)n)) return 0;
    if (out && cap) {
        size_t copy = (size_t)n;
        if (copy >= cap) copy = cap - 1;
        memcpy(out, r->data + r->pos, copy);
        out[copy] = 0;
    }
    r->pos += (size_t)n;
    return 1;
}

static int j47_hex4(const char *p, const char *end, unsigned int *out) {
    if (!p || end - p < 4) return 0;
    unsigned int cp = 0;
    for (int i = 0; i < 4; ++i) {
        unsigned char h = (unsigned char)p[i];
        cp <<= 4;
        if (h >= '0' && h <= '9') cp |= h - '0';
        else if (h >= 'a' && h <= 'f') cp |= h - 'a' + 10;
        else if (h >= 'A' && h <= 'F') cp |= h - 'A' + 10;
        else return 0;
    }
    if (out) *out = cp;
    return 1;
}

static void j47_utf8_put(char *out, size_t cap, size_t *n, unsigned int cp) {
    unsigned char bytes[4];
    size_t count = 0;
    if (cp <= 0x7f) bytes[count++] = (unsigned char)cp;
    else if (cp <= 0x7ff) {
        bytes[count++] = (unsigned char)(0xc0 | (cp >> 6));
        bytes[count++] = (unsigned char)(0x80 | (cp & 0x3f));
    } else if (cp <= 0xffff) {
        bytes[count++] = (unsigned char)(0xe0 | (cp >> 12));
        bytes[count++] = (unsigned char)(0x80 | ((cp >> 6) & 0x3f));
        bytes[count++] = (unsigned char)(0x80 | (cp & 0x3f));
    } else if (cp <= 0x10ffff) {
        bytes[count++] = (unsigned char)(0xf0 | (cp >> 18));
        bytes[count++] = (unsigned char)(0x80 | ((cp >> 12) & 0x3f));
        bytes[count++] = (unsigned char)(0x80 | ((cp >> 6) & 0x3f));
        bytes[count++] = (unsigned char)(0x80 | (cp & 0x3f));
    } else {
        bytes[count++] = '?';
    }
    for (size_t i = 0; i < count; ++i) {
        if (out && cap && *n + 1 < cap) out[*n] = (char)bytes[i];
        ++*n;
    }
}

static int j47_json_unescape(const char *p, const char *end, char *out, size_t cap, const char **after) {
    if (!p || p >= end || *p != '"') return 0;
    ++p;
    size_t n = 0;
    while (p < end) {
        unsigned char c = (unsigned char)*p++;
        if (c == '"') {
            if (out && cap) out[n < cap ? n : cap - 1] = 0;
            if (after) *after = p;
            return 1;
        }
        if (c == '\\' && p < end) {
            unsigned char e = (unsigned char)*p++;
            if (e == 'n') c = '\n';
            else if (e == 'r') c = '\r';
            else if (e == 't') c = '\t';
            else if (e == 'b') c = '\b';
            else if (e == 'f') c = '\f';
            else if (e == 'u') {
                unsigned int cp = 0;
                if (!j47_hex4(p, end, &cp)) return 0;
                p += 4;
                if (cp >= 0xd800 && cp <= 0xdbff && end - p >= 6 && p[0] == '\\' && p[1] == 'u') {
                    unsigned int low = 0;
                    if (j47_hex4(p + 2, end, &low) && low >= 0xdc00 && low <= 0xdfff) {
                        cp = 0x10000u + ((cp - 0xd800u) << 10) + (low - 0xdc00u);
                        p += 6;
                    }
                }
                j47_utf8_put(out, cap, &n, cp);
                continue;
            } else c = e;
        }
        if (out && cap && n + 1 < cap) out[n] = (char)c;
        ++n;
    }
    return 0;
}

static const char *j47_json_find_key(const char *json, const char *key) {
    if (!json || !key) return NULL;
    size_t key_len = strlen(key);
    const char *p = json;
    while (*p) {
        if (*p != '"') { ++p; continue; }
        const char *start = ++p;
        int escaped = 0;
        while (*p) {
            if (!escaped && *p == '"') break;
            if (!escaped && *p == '\\') escaped = 1;
            else escaped = 0;
            ++p;
        }
        if (!*p) return NULL;
        size_t n = (size_t)(p - start);
        const char *after = p + 1;
        while (*after == ' ' || *after == '\t' || *after == '\r' || *after == '\n') ++after;
        if (n == key_len && memcmp(start, key, key_len) == 0 && *after == ':') {
            ++after;
            while (*after == ' ' || *after == '\t' || *after == '\r' || *after == '\n') ++after;
            return after;
        }
        p = after;
    }
    return NULL;
}

static int j47_json_get_string(const char *json, const char *key, char *out, size_t cap) {
    const char *v = j47_json_find_key(json, key);
    if (!v || *v != '"') return 0;
    return j47_json_unescape(v, json + strlen(json), out, cap, NULL);
}

static int j47_json_get_int(const char *json, const char *key, int *out) {
    const char *v = j47_json_find_key(json, key);
    if (!v) return 0;
    char *end = NULL;
    long n = strtol(v, &end, 10);
    if (end == v) return 0;
    if (out) *out = (int)n;
    return 1;
}

typedef struct J47TextStyle {
    char color;
    unsigned char bold;
    unsigned char italic;
    unsigned char underlined;
    unsigned char strikethrough;
    unsigned char obfuscated;
} J47TextStyle;

typedef struct J47TextSink {
    char *out;
    size_t cap;
    size_t len;
} J47TextSink;

static const char *j47_json_ws(const char *p, const char *end) {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) ++p;
    return p;
}

static void j47_text_put_bytes(J47TextSink *sink, const char *text, size_t n) {
    if (!sink || !text) return;
    if (sink->out && sink->cap) {
        size_t room = sink->len < sink->cap - 1 ? sink->cap - 1 - sink->len : 0;
        size_t copy = n < room ? n : room;
        if (copy) memcpy(sink->out + sink->len, text, copy);
    }
    sink->len += n;
    if (sink->out && sink->cap) sink->out[sink->len < sink->cap ? sink->len : sink->cap - 1] = 0;
}

static void j47_text_put_cstr(J47TextSink *sink, const char *text) {
    if (text) j47_text_put_bytes(sink, text, strlen(text));
}

static void j47_text_put_code(J47TextSink *sink, char code) {
    char token[3] = {(char)0xc2, (char)0xa7, code};
    j47_text_put_bytes(sink, token, sizeof(token));
}

static void j47_text_apply_style(J47TextSink *sink, const J47TextStyle *style) {
    /* Start every JSON component from a known renderer state. A component's
       text may itself contain legacy section-sign codes, which cannot be
       inferred by comparing only the JSON style object. */
    if (sink->len > 0) j47_text_put_code(sink, 'r');
    if (style->color) j47_text_put_code(sink, style->color);
    if (style->obfuscated) j47_text_put_code(sink, 'k');
    if (style->bold) j47_text_put_code(sink, 'l');
    if (style->strikethrough) j47_text_put_code(sink, 'm');
    if (style->underlined) j47_text_put_code(sink, 'n');
    if (style->italic) j47_text_put_code(sink, 'o');
}

static char j47_json_color_code(const char *name) {
    static const struct { const char *name; char code; } map[] = {
        {"black",'0'},{"dark_blue",'1'},{"dark_green",'2'},{"dark_aqua",'3'},
        {"dark_red",'4'},{"dark_purple",'5'},{"gold",'6'},{"gray",'7'},
        {"dark_gray",'8'},{"blue",'9'},{"green",'a'},{"aqua",'b'},
        {"red",'c'},{"light_purple",'d'},{"yellow",'e'},{"white",'f'}
    };
    if (!name) return 0;
    if (!strcmp(name, "reset")) return 0;
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); ++i)
        if (!strcmp(name, map[i].name)) return map[i].code;
    return 0;
}

static int j47_json_skip_value_ptr(const char **pp, const char *end, int depth) {
    if (!pp || depth > 64) return 0;
    const char *p = j47_json_ws(*pp, end);
    if (p >= end) return 0;
    if (*p == '"') {
        if (!j47_json_unescape(p, end, NULL, 0, &p)) return 0;
    } else if (*p == '{') {
        ++p;
        p = j47_json_ws(p, end);
        while (p < end && *p != '}') {
            if (*p != '"' || !j47_json_unescape(p, end, NULL, 0, &p)) return 0;
            p = j47_json_ws(p, end);
            if (p >= end || *p++ != ':') return 0;
            if (!j47_json_skip_value_ptr(&p, end, depth + 1)) return 0;
            p = j47_json_ws(p, end);
            if (p < end && *p == ',') { ++p; p = j47_json_ws(p, end); continue; }
            break;
        }
        if (p >= end || *p != '}') return 0;
        ++p;
    } else if (*p == '[') {
        ++p;
        p = j47_json_ws(p, end);
        while (p < end && *p != ']') {
            if (!j47_json_skip_value_ptr(&p, end, depth + 1)) return 0;
            p = j47_json_ws(p, end);
            if (p < end && *p == ',') { ++p; p = j47_json_ws(p, end); continue; }
            break;
        }
        if (p >= end || *p != ']') return 0;
        ++p;
    } else {
        while (p < end && *p != ',' && *p != '}' && *p != ']' &&
               *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') ++p;
    }
    *pp = p;
    return 1;
}

static int j47_json_bool_value(const char *p, const char *end, int fallback) {
    p = j47_json_ws(p, end);
    if (end - p >= 4 && !memcmp(p, "true", 4)) return 1;
    if (end - p >= 5 && !memcmp(p, "false", 5)) return 0;
    return fallback;
}

static void j47_json_emit_value(const char **pp, const char *end, J47TextSink *sink,
                                J47TextStyle inherited, int depth);

static int j47_json_collect_array(const char *start, const char *end,
                                  J47TextStyle inherited,
                                  char out[][512], int max_items) {
    const char *p = j47_json_ws(start, end);
    if (p >= end || *p != '[') return 0;
    ++p;
    int count = 0;
    for (;;) {
        p = j47_json_ws(p, end);
        if (p >= end || *p == ']') break;
        if (count < max_items) {
            J47TextSink tmp;
            memset(&tmp, 0, sizeof(tmp));
            tmp.out = out[count];
            tmp.cap = 512;
            out[count][0] = 0;
            j47_json_emit_value(&p, end, &tmp, inherited, 0);
            count++;
        } else if (!j47_json_skip_value_ptr(&p, end, 0)) break;
        p = j47_json_ws(p, end);
        if (p < end && *p == ',') { ++p; continue; }
        break;
    }
    return count;
}

static void j47_json_emit_translate(J47TextSink *sink, const char *key,
                                    const char *with_start, const char *with_end,
                                    J47TextStyle style) {
    char args[8][512];
    int argc = with_start ? j47_json_collect_array(with_start, with_end, style, args, 8) : 0;
    j47_text_apply_style(sink, &style);
    if (!strcmp(key, "chat.type.text") && argc >= 2) {
        j47_text_put_cstr(sink, "<"); j47_text_put_cstr(sink, args[0]);
        j47_text_put_cstr(sink, "> "); j47_text_put_cstr(sink, args[1]);
    } else if (!strcmp(key, "chat.type.announcement") && argc >= 2) {
        j47_text_put_cstr(sink, "["); j47_text_put_cstr(sink, args[0]);
        j47_text_put_cstr(sink, "] "); j47_text_put_cstr(sink, args[1]);
    } else if (!strcmp(key, "commands.message.display.incoming") && argc >= 2) {
        j47_text_put_cstr(sink, args[0]); j47_text_put_cstr(sink, " whispers to you: ");
        j47_text_put_cstr(sink, args[1]);
    } else if (!strcmp(key, "commands.message.display.outgoing") && argc >= 2) {
        j47_text_put_cstr(sink, "You whisper to "); j47_text_put_cstr(sink, args[0]);
        j47_text_put_cstr(sink, ": "); j47_text_put_cstr(sink, args[1]);
    } else if (!strcmp(key, "multiplayer.player.joined") && argc >= 1) {
        j47_text_put_cstr(sink, args[0]); j47_text_put_cstr(sink, " joined the game");
    } else if (!strcmp(key, "multiplayer.player.left") && argc >= 1) {
        j47_text_put_cstr(sink, args[0]); j47_text_put_cstr(sink, " left the game");
    } else {
        j47_text_put_cstr(sink, key);
        for (int i = 0; i < argc; ++i) {
            j47_text_put_cstr(sink, i == 0 ? ": " : " ");
            j47_text_put_cstr(sink, args[i]);
        }
    }
}

static void j47_json_emit_value(const char **pp, const char *end, J47TextSink *sink,
                                J47TextStyle inherited, int depth) {
    if (!pp || !sink || depth > 32) return;
    const char *p = j47_json_ws(*pp, end);
    if (p >= end) { *pp = p; return; }
    if (*p == '"') {
        char text[2048];
        const char *after = p;
        if (j47_json_unescape(p, end, text, sizeof(text), &after)) {
            j47_text_apply_style(sink, &inherited);
            j47_text_put_cstr(sink, text);
            p = after;
        }
        *pp = p;
        return;
    }
    if (*p == '[') {
        ++p;
        for (;;) {
            p = j47_json_ws(p, end);
            if (p >= end || *p == ']') { if (p < end) ++p; break; }
            j47_json_emit_value(&p, end, sink, inherited, depth + 1);
            p = j47_json_ws(p, end);
            if (p < end && *p == ',') { ++p; continue; }
            if (p < end && *p == ']') { ++p; break; }
            break;
        }
        *pp = p;
        return;
    }
    if (*p != '{') {
        j47_json_skip_value_ptr(&p, end, depth + 1);
        *pp = p;
        return;
    }

    ++p;
    J47TextStyle style = inherited;
    char text[2048] = {0};
    char translate[256] = {0};
    char selector[512] = {0};
    const char *with_start = NULL, *with_end = NULL;
    const char *extra_start = NULL, *extra_end = NULL;

    for (;;) {
        p = j47_json_ws(p, end);
        if (p >= end || *p == '}') { if (p < end) ++p; break; }
        char key[64];
        if (*p != '"' || !j47_json_unescape(p, end, key, sizeof(key), &p)) break;
        p = j47_json_ws(p, end);
        if (p >= end || *p++ != ':') break;
        p = j47_json_ws(p, end);
        const char *value_start = p;

        if (!strcmp(key, "text") && *p == '"') {
            j47_json_unescape(p, end, text, sizeof(text), &p);
        } else if (!strcmp(key, "translate") && *p == '"') {
            j47_json_unescape(p, end, translate, sizeof(translate), &p);
        } else if (!strcmp(key, "selector") && *p == '"') {
            j47_json_unescape(p, end, selector, sizeof(selector), &p);
        } else if (!strcmp(key, "color") && *p == '"') {
            char color[40];
            if (j47_json_unescape(p, end, color, sizeof(color), &p)) style.color = j47_json_color_code(color);
        } else if (!strcmp(key, "bold")) {
            style.bold = (unsigned char)j47_json_bool_value(p, end, style.bold);
            j47_json_skip_value_ptr(&p, end, depth + 1);
        } else if (!strcmp(key, "italic")) {
            style.italic = (unsigned char)j47_json_bool_value(p, end, style.italic);
            j47_json_skip_value_ptr(&p, end, depth + 1);
        } else if (!strcmp(key, "underlined")) {
            style.underlined = (unsigned char)j47_json_bool_value(p, end, style.underlined);
            j47_json_skip_value_ptr(&p, end, depth + 1);
        } else if (!strcmp(key, "strikethrough")) {
            style.strikethrough = (unsigned char)j47_json_bool_value(p, end, style.strikethrough);
            j47_json_skip_value_ptr(&p, end, depth + 1);
        } else if (!strcmp(key, "obfuscated")) {
            style.obfuscated = (unsigned char)j47_json_bool_value(p, end, style.obfuscated);
            j47_json_skip_value_ptr(&p, end, depth + 1);
        } else if (!strcmp(key, "with")) {
            with_start = value_start;
            if (j47_json_skip_value_ptr(&p, end, depth + 1)) with_end = p;
        } else if (!strcmp(key, "extra")) {
            extra_start = value_start;
            if (j47_json_skip_value_ptr(&p, end, depth + 1)) extra_end = p;
        } else {
            j47_json_skip_value_ptr(&p, end, depth + 1);
        }

        p = j47_json_ws(p, end);
        if (p < end && *p == ',') { ++p; continue; }
        if (p < end && *p == '}') { ++p; break; }
        break;
    }

    if (text[0]) {
        j47_text_apply_style(sink, &style);
        j47_text_put_cstr(sink, text);
    } else if (translate[0]) {
        j47_json_emit_translate(sink, translate, with_start, with_end ? with_end : end, style);
    } else if (selector[0]) {
        j47_text_apply_style(sink, &style);
        j47_text_put_cstr(sink, selector);
    }
    if (extra_start && extra_end) {
        const char *ep = extra_start;
        j47_json_emit_value(&ep, extra_end, sink, style, depth + 1);
    }
    *pp = p;
}

static void j47_json_collect_text(const char *json, char *out, size_t cap) {
    if (!out || cap == 0) return;
    out[0] = 0;
    if (!json || !json[0]) return;
    J47TextSink sink;
    J47TextStyle style;
    memset(&sink, 0, sizeof(sink));
    memset(&style, 0, sizeof(style));
    sink.out = out;
    sink.cap = cap;
    const char *p = json;
    const char *end = json + strlen(json);
    j47_json_emit_value(&p, end, &sink, style, 0);
    out[cap - 1] = 0;
}

static int j47_build_raw_frame(int packet_id, const unsigned char *payload, size_t payload_len, J47Writer *out) {
    J47Writer body;
    j47_writer_init(&body, payload_len + 16);
    j47_w_varint(&body, packet_id);
    j47_w_bytes(&body, payload, payload_len);
    if (body.failed || body.len > J47_PACKET_MAX) { j47_writer_free(&body); return 0; }
    j47_w_varint(out, (int32_t)body.len);
    j47_w_bytes(out, body.data, body.len);
    j47_writer_free(&body);
    return !out->failed;
}

static int j47_probe_read_frame(SOCKET s, int timeout_ms, unsigned char **out_data, size_t *out_len) {
    unsigned char header[5];
    size_t hn = 0;
    double started = now_seconds();
    int32_t packet_len = -1;
    while (hn < sizeof(header)) {
        int remaining = timeout_ms - (int)((now_seconds() - started) * 1000.0);
        if (remaining <= 0) return 0;
        int n = j47_recv_some_timeout(s, header + hn, 1, remaining);
        if (n != 1) return 0;
        ++hn;
        size_t used = 0;
        if (j47_decode_varint_at(header, hn, &used, &packet_len)) break;
    }
    if (packet_len < 0 || packet_len > (int32_t)J47_PACKET_MAX) return 0;
    unsigned char *buf = (unsigned char *)malloc((size_t)packet_len);
    if (!buf) return 0;
    size_t got = 0;
    while (got < (size_t)packet_len) {
        int remaining = timeout_ms - (int)((now_seconds() - started) * 1000.0);
        if (remaining <= 0) { free(buf); return 0; }
        int n = j47_recv_some_timeout(s, buf + got, (size_t)packet_len - got, remaining);
        if (n <= 0) { free(buf); return 0; }
        got += (size_t)n;
    }
    *out_data = buf;
    *out_len = got;
    return 1;
}

int pex_java47_probe_status(const char *host, int port, int timeout_ms, PexJava47Status *out_status) {
    PexJava47Status st;
    memset(&st, 0, sizeof(st));
    st.ping_ms = -1;
    if (!host || !host[0] || port <= 0 || port > 65535) {
        if (out_status) *out_status = st;
        return 0;
    }
    if (timeout_ms < 100) timeout_ms = 100;
    SOCKET s = j47_connect_timeout(host, port, timeout_ms);
    if (s == INVALID_SOCKET) { if (out_status) *out_status = st; return 0; }

    J47Writer handshake_payload, stream;
    j47_writer_init(&handshake_payload, 256);
    j47_writer_init(&stream, 512);
    j47_w_varint(&handshake_payload, PEX_JAVA47_PROTOCOL_VERSION);
    j47_w_string_n(&handshake_payload, host, 255);
    j47_w_be16(&handshake_payload, port);
    j47_w_varint(&handshake_payload, 1);
    j47_build_raw_frame(0, handshake_payload.data, handshake_payload.len, &stream);
    j47_build_raw_frame(0, NULL, 0, &stream);
    int ok = !stream.failed && j47_send_all_timeout(s, stream.data, stream.len, timeout_ms);
    j47_writer_free(&handshake_payload);
    j47_writer_free(&stream);
    if (!ok) { closesocket(s); if (out_status) *out_status = st; return 0; }

    unsigned char *frame = NULL;
    size_t frame_len = 0;
    if (!j47_probe_read_frame(s, timeout_ms, &frame, &frame_len)) {
        closesocket(s); if (out_status) *out_status = st; return 0;
    }
    J47Reader r;
    j47_reader_init(&r, frame, frame_len);
    int32_t packet_id = -1;
    char json[16384];
    json[0] = 0;
    if (!j47_r_varint(&r, &packet_id) || packet_id != 0 || !j47_r_string(&r, json, sizeof(json))) {
        free(frame); closesocket(s); if (out_status) *out_status = st; return 0;
    }
    free(frame);

    st.protocol = 0;
    st.online_players = 0;
    st.max_players = 0;
    /* Scope repeated field names to their containing objects. Player samples
       also contain a "name" key and must not replace the version label. */
    const char *version_object = j47_json_find_key(json, "version");
    const char *players_object = j47_json_find_key(json, "players");
    if (!version_object || *version_object != '{' ||
        !j47_json_get_int(version_object, "protocol", &st.protocol)) {
        closesocket(s);
        if (out_status) *out_status = st;
        return 0;
    }
    if (players_object && *players_object == '{') {
        j47_json_get_int(players_object, "online", &st.online_players);
        j47_json_get_int(players_object, "max", &st.max_players);
    }
    if (!j47_json_get_string(version_object, "name", st.version, sizeof(st.version)))
        snprintf(st.version, sizeof(st.version), "Java");
    const char *description = j47_json_find_key(json, "description");
    j47_json_collect_text(description ? description : "", st.motd, sizeof(st.motd));

    int64_t stamp = (int64_t)(now_seconds() * 1000.0);
    J47Writer ping_payload, ping_frame;
    j47_writer_init(&ping_payload, 16);
    j47_writer_init(&ping_frame, 32);
    j47_w_be64(&ping_payload, stamp);
    j47_build_raw_frame(1, ping_payload.data, ping_payload.len, &ping_frame);
    double ping_started = now_seconds();
    if (j47_send_all_timeout(s, ping_frame.data, ping_frame.len, timeout_ms)) {
        frame = NULL; frame_len = 0;
        if (j47_probe_read_frame(s, timeout_ms, &frame, &frame_len)) {
            J47Reader pr;
            int32_t pid = -1;
            j47_reader_init(&pr, frame, frame_len);
            if (j47_r_varint(&pr, &pid) && pid == 1) {
                (void)j47_r_be64(&pr);
                st.ping_ms = (int)((now_seconds() - ping_started) * 1000.0);
                if (st.ping_ms < 0) st.ping_ms = 0;
            }
            free(frame);
        }
    }
    j47_writer_free(&ping_payload);
    j47_writer_free(&ping_frame);
    closesocket(s);
    st.ok = 1;
    if (out_status) *out_status = st;
    return 1;
}

/* --------------------------- play/session codec --------------------------- */

static void j47_set_status(const char *text) {
    snprintf(g_j47.status, sizeof(g_j47.status), "%.*s", (int)sizeof(g_j47.status) - 1, text ? text : "");
}

static void j47_fail(const char *reason) {
    if (g_j47.socket != INVALID_SOCKET) closesocket(g_j47.socket);
    g_j47.socket = INVALID_SOCKET;
    g_j47.active = 0;
    g_j47.state = PEX_JAVA47_FAILED;
    snprintf(g_j47.disconnect_reason, sizeof(g_j47.disconnect_reason), "%s", reason && reason[0] ? reason : "Connection lost");
    j47_set_status(g_j47.disconnect_reason);
}

static int j47_queue_bytes(const void *data, size_t len) {
    if (!data || len == 0) return 1;
    if (len > J47_TX_MAX || g_j47.tx_len > J47_TX_MAX - len) return 0;
    size_t need = g_j47.tx_len + len;
    if (need > g_j47.tx_cap) {
        size_t cap = g_j47.tx_cap ? g_j47.tx_cap : 4096;
        while (cap < need) cap *= 2;
        unsigned char *p = (unsigned char *)realloc(g_j47.tx, cap);
        if (!p) return 0;
        g_j47.tx = p;
        g_j47.tx_cap = cap;
    }
    memcpy(g_j47.tx + g_j47.tx_len, data, len);
    g_j47.tx_len += len;
    return 1;
}

static int j47_flush_tx(void) {
    while (g_j47.tx_pos < g_j47.tx_len) {
        int n = send(g_j47.socket, (const char *)g_j47.tx + g_j47.tx_pos,
                     (int)(g_j47.tx_len - g_j47.tx_pos), 0);
        if (n > 0) { g_j47.tx_pos += (size_t)n; continue; }
        if (n == 0) { g_j47.peer_closed = 1; return 1; }
        if (j47_socket_would_block()) return 1;
        j47_fail("Could not send Java protocol data");
        return 0;
    }
    g_j47.tx_len = 0;
    g_j47.tx_pos = 0;
    return 1;
}

static int j47_send_packet(int packet_id, const unsigned char *payload, size_t payload_len) {
    J47Writer packet, inner, frame;
    j47_writer_init(&packet, payload_len + 16);
    j47_w_varint(&packet, packet_id);
    if (payload_len) j47_w_bytes(&packet, payload, payload_len);
    if (packet.failed || packet.len > J47_PACKET_MAX) { j47_writer_free(&packet); return 0; }

    j47_writer_init(&inner, packet.len + 32);
    if (g_j47.compression_threshold >= 0) {
        if ((int)packet.len >= g_j47.compression_threshold) {
            uLongf bound = compressBound((uLong)packet.len);
            unsigned char *compressed = (unsigned char *)malloc((size_t)bound);
            if (!compressed) { j47_writer_free(&packet); j47_writer_free(&inner); return 0; }
            uLongf compressed_len = bound;
            int zr = compress2(compressed, &compressed_len, packet.data, (uLong)packet.len, Z_DEFAULT_COMPRESSION);
            if (zr != Z_OK) { free(compressed); j47_writer_free(&packet); j47_writer_free(&inner); return 0; }
            j47_w_varint(&inner, (int32_t)packet.len);
            j47_w_bytes(&inner, compressed, (size_t)compressed_len);
            free(compressed);
        } else {
            j47_w_varint(&inner, 0);
            j47_w_bytes(&inner, packet.data, packet.len);
        }
    } else {
        j47_w_bytes(&inner, packet.data, packet.len);
    }

    j47_writer_init(&frame, inner.len + 8);
    j47_w_varint(&frame, (int32_t)inner.len);
    j47_w_bytes(&frame, inner.data, inner.len);
    int ok = !frame.failed && j47_queue_bytes(frame.data, frame.len);
    j47_writer_free(&packet);
    j47_writer_free(&inner);
    j47_writer_free(&frame);
    return ok;
}

static int j47_rx_reserve(size_t add) {
    if (add > J47_RX_MAX || g_j47.rx_len > J47_RX_MAX - add) return 0;
    size_t need = g_j47.rx_len + add;
    if (need <= g_j47.rx_cap) return 1;
    size_t cap = g_j47.rx_cap ? g_j47.rx_cap : 16384;
    while (cap < need) cap *= 2;
    unsigned char *p = (unsigned char *)realloc(g_j47.rx, cap);
    if (!p) return 0;
    g_j47.rx = p;
    g_j47.rx_cap = cap;
    return 1;
}

static int j47_receive_available(void) {
    unsigned char temp[32768];
    for (;;) {
        int n = recv(g_j47.socket, (char *)temp, sizeof(temp), 0);
        if (n > 0) {
            if (!j47_rx_reserve((size_t)n)) { j47_fail("Java receive buffer overflow"); return 0; }
            memcpy(g_j47.rx + g_j47.rx_len, temp, (size_t)n);
            g_j47.rx_len += (size_t)n;
            continue;
        }
        if (n == 0) { j47_fail("Java server closed the connection"); return 0; }
        if (j47_socket_would_block()) return 1;
        j47_fail("Java network receive failed");
        return 0;
    }
}

static int j47_peek_varint(const unsigned char *data, size_t len, int32_t *out, size_t *used) {
    uint32_t result = 0;
    for (size_t i = 0; i < len && i < 5; i++) {
        unsigned char b = data[i];
        result |= (uint32_t)(b & 0x7f) << (7 * i);
        if (!(b & 0x80)) {
            if (out) *out = (int32_t)result;
            if (used) *used = i + 1;
            return 1;
        }
    }
    if (len >= 5) return -1;
    return 0;
}

static int j47_nbt_skip_payload(J47Reader *r, int tag_type, int depth);

static int j47_nbt_skip_string(J47Reader *r) {
    uint16_t n = j47_r_be16(r);
    if (r->failed || n > r->len - r->pos) { r->failed = 1; return 0; }
    r->pos += n;
    return 1;
}

static int j47_nbt_skip_payload(J47Reader *r, int tag_type, int depth) {
    if (depth > 64) { r->failed = 1; return 0; }
    switch (tag_type) {
        case 0: return 1;
        case 1: (void)j47_r_u8(r); return !r->failed;
        case 2: (void)j47_r_be16(r); return !r->failed;
        case 3: (void)j47_r_be32(r); return !r->failed;
        case 4: (void)j47_r_be64(r); return !r->failed;
        case 5: (void)j47_r_float(r); return !r->failed;
        case 6: (void)j47_r_double(r); return !r->failed;
        case 7: {
            int32_t n = j47_r_be32(r);
            if (r->failed || n < 0 || (size_t)n > r->len - r->pos) { r->failed = 1; return 0; }
            r->pos += (size_t)n; return 1;
        }
        case 8: return j47_nbt_skip_string(r);
        case 9: {
            int child = (int)j47_r_u8(r);
            int32_t n = j47_r_be32(r);
            if (r->failed || n < 0 || n > 1000000) { r->failed = 1; return 0; }
            for (int32_t i = 0; i < n; i++) if (!j47_nbt_skip_payload(r, child, depth + 1)) return 0;
            return 1;
        }
        case 10: {
            for (;;) {
                int child = (int)j47_r_u8(r);
                if (r->failed) return 0;
                if (child == 0) return 1;
                if (!j47_nbt_skip_string(r) || !j47_nbt_skip_payload(r, child, depth + 1)) return 0;
            }
        }
        case 11: {
            int32_t n = j47_r_be32(r);
            if (r->failed || n < 0 || (size_t)n > (r->len - r->pos) / 4) { r->failed = 1; return 0; }
            r->pos += (size_t)n * 4; return 1;
        }
        case 12: {
            int32_t n = j47_r_be32(r);
            if (r->failed || n < 0 || (size_t)n > (r->len - r->pos) / 8) { r->failed = 1; return 0; }
            r->pos += (size_t)n * 8; return 1;
        }
        default: r->failed = 1; return 0;
    }
}

static int j47_nbt_read_string(J47Reader *r, char *out, size_t cap) {
    uint16_t n = j47_r_be16(r);
    if (r->failed || n > r->len - r->pos) { r->failed = 1; return 0; }
    if (out && cap) {
        size_t copy = n;
        if (copy >= cap) copy = cap - 1;
        memcpy(out, r->data + r->pos, copy);
        out[copy] = 0;
    }
    r->pos += n;
    return 1;
}

static void j47_item_text_normalize(const char *src, char *out, size_t cap) {
    if (!out || cap == 0) return;
    out[0] = 0;
    if (!src || !src[0]) return;
    const char *p = src;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
    if (*p == '{' || *p == '[' || *p == '"') {
        j47_json_collect_text(p, out, cap);
        if (out[0]) return;
    }
    snprintf(out, cap, "%s", src);
}

static int j47_nbt_parse_item_compound(J47Reader *r, int depth, int in_display, J47RawItem *raw) {
    if (depth > 32) { r->failed = 1; return 0; }
    for (;;) {
        int child = (int)j47_r_u8(r);
        if (r->failed) return 0;
        if (child == 0) return 1;
        char name[96];
        if (!j47_nbt_read_string(r, name, sizeof(name))) return 0;

        if (child == 10 && !strcmp(name, "display")) {
            if (!j47_nbt_parse_item_compound(r, depth + 1, 1, raw)) return 0;
        } else if (in_display && child == 8 && !strcmp(name, "Name")) {
            char value[512];
            if (!j47_nbt_read_string(r, value, sizeof(value))) return 0;
            j47_item_text_normalize(value, raw->display_name, sizeof(raw->display_name));
        } else if (in_display && child == 9 && !strcmp(name, "Lore")) {
            int item_type = (int)j47_r_u8(r);
            int32_t count = j47_r_be32(r);
            if (r->failed || count < 0 || count > 100000) { r->failed = 1; return 0; }
            raw->lore_count = 0;
            for (int32_t i = 0; i < count; ++i) {
                if (item_type == 8) {
                    char value[512];
                    if (!j47_nbt_read_string(r, value, sizeof(value))) return 0;
                    if (raw->lore_count < J47_ITEM_LORE_MAX) {
                        j47_item_text_normalize(value, raw->lore[raw->lore_count],
                                                sizeof(raw->lore[raw->lore_count]));
                        raw->lore_count++;
                    }
                } else if (!j47_nbt_skip_payload(r, item_type, depth + 1)) return 0;
            }
        } else if (child == 9 && (!strcmp(name, "ench") || !strcmp(name, "StoredEnchantments"))) {
            raw->has_enchantment = 1;
            if (!j47_nbt_skip_payload(r, child, depth + 1)) return 0;
        } else if (child == 10) {
            if (!j47_nbt_parse_item_compound(r, depth + 1, in_display, raw)) return 0;
        } else if (!j47_nbt_skip_payload(r, child, depth + 1)) return 0;
    }
}

static int j47_parse_item_nbt(J47Reader *r, J47RawItem *raw) {
    int root = (int)j47_r_u8(r);
    if (r->failed) return 0;
    if (root == 0) return 1;
    char root_name[96];
    if (!j47_nbt_read_string(r, root_name, sizeof(root_name))) return 0;
    if (root == 10) return j47_nbt_parse_item_compound(r, 0, 0, raw);
    return j47_nbt_skip_payload(r, root, 0);
}

static void j47_translate_block_state_parts(int id, int meta, int *out_id, int *out_meta) {
    int mapped_id = id;
    int mapped_meta = meta & 15;

    /* Protocol 47 is still pre-flattening, but several old IDs gained new
       metadata values after 1.2.5. Translate those before the simple ID map. */
    if (id == 1) {
        switch (meta & 7) {
            case 1: mapped_id = BLOCK_STONE; mapped_meta = 0; break;                 /* granite */
            case 2: mapped_id = BLOCK_STONE_BRICK; mapped_meta = 0; break;           /* polished granite */
            case 3: mapped_id = BLOCK_STONE; mapped_meta = 0; break;                 /* diorite */
            case 4: mapped_id = BLOCK_STONE_BRICK; mapped_meta = 0; break;           /* polished diorite */
            case 5: mapped_id = BLOCK_COBBLESTONE; mapped_meta = 0; break;           /* andesite */
            case 6: mapped_id = BLOCK_STONE_BRICK; mapped_meta = 0; break;           /* polished andesite */
            default: mapped_meta = 0; break;
        }
    } else if (id == 3) {
        if ((meta & 3) == 2) { mapped_id = BLOCK_MYCELIUM; mapped_meta = 0; }         /* podzol */
        else mapped_meta = 0;                                                        /* dirt/coarse dirt */
    } else if (id == 5) {
        if ((meta & 7) == 4) mapped_meta = BLOCK_META_WOOD_JUNGLE;                   /* acacia -> jungle */
        else if ((meta & 7) == 5) mapped_meta = BLOCK_META_WOOD_SPRUCE;              /* dark oak -> spruce */
    } else if (id == 6) {
        int grow = meta & 8;
        int kind = meta & 7;
        if (kind == 4) kind = BLOCK_META_WOOD_JUNGLE;
        else if (kind == 5) kind = BLOCK_META_WOOD_SPRUCE;
        mapped_meta = grow | kind;
    } else if (id == 12) {
        mapped_meta = 0;                                                             /* red sand -> sand */
    } else if (id == 19) {
        mapped_meta = 0;                                                             /* wet sponge -> sponge */
    } else if (id == 38) {
        mapped_id = ((meta & 15) == 3 || (meta & 15) == 8) ? BLOCK_YELLOW_FLOWER : BLOCK_RED_ROSE;
        mapped_meta = 0;
    } else if (id == 43 || id == 44) {
        int top = meta & 8;
        int kind = meta & 7;
        if (kind == 6) kind = 4;                                                      /* nether brick -> brick slab */
        else if (kind == 7) kind = 0;                                                 /* quartz -> stone slab */
        mapped_meta = top | kind;
    } else if (id == 95) {
        mapped_id = BLOCK_GLASS;                                                      /* 1.8 stained glass, not locked chest */
        mapped_meta = 0;
    } else if (id >= 0 && id <= 124) {
        /* Existing 1.2.5 block. Keep its compatible metadata. */
    } else {
        switch (id) {
            case 125: mapped_id = BLOCK_PLANKS; mapped_meta = (meta & 7) > 3 ? BLOCK_META_WOOD_JUNGLE : (meta & 7); break;
            case 126: mapped_id = BLOCK_SLAB; mapped_meta = (meta & 8) | 2; break;     /* wooden slab shape */
            case 127: mapped_id = BLOCK_TORCH; mapped_meta = 0; break;
            case 128: mapped_id = BLOCK_STONE_BRICK_STAIRS; mapped_meta = meta & 7; break;
            case 129: mapped_id = BLOCK_DIAMOND_ORE; mapped_meta = 0; break;
            case 130: mapped_id = BLOCK_CHEST; break;
            case 131: mapped_id = BLOCK_LEVER; mapped_meta = meta & 15; break;
            case 132: mapped_id = BLOCK_REDSTONE_WIRE; mapped_meta = meta & 15; break;
            case 133: mapped_id = BLOCK_DIAMOND_BLOCK; mapped_meta = 0; break;
            case 134: case 135: case 136: mapped_id = BLOCK_WOOD_STAIRS; mapped_meta = meta & 7; break;
            case 137: mapped_id = BLOCK_DISPENSER; mapped_meta = meta & 7; break;
            case 138: mapped_id = BLOCK_GLOWSTONE; mapped_meta = 0; break;
            case 139: mapped_id = BLOCK_FENCE; mapped_meta = 0; break;
            case 140: mapped_id = BLOCK_BROWN_MUSHROOM; mapped_meta = 0; break;
            case 141: case 142: mapped_id = BLOCK_CROPS; mapped_meta = meta & 7; break;
            case 143: mapped_id = BLOCK_STONE_BUTTON; mapped_meta = meta & 15; break;
            case 144: mapped_id = BLOCK_PUMPKIN; mapped_meta = meta & 3; break;
            case 145: mapped_id = BLOCK_ENCHANTMENT_TABLE; mapped_meta = 0; break;
            case 146: mapped_id = BLOCK_CHEST; break;
            case 147: case 148: mapped_id = BLOCK_STONE_PRESSURE_PLATE; mapped_meta = meta ? 1 : 0; break;
            case 149: mapped_id = BLOCK_REDSTONE_REPEATER_OFF; mapped_meta = meta & 15; break;
            case 150: mapped_id = BLOCK_REDSTONE_REPEATER_ON; mapped_meta = meta & 15; break;
            case 151: case 178: mapped_id = BLOCK_SLAB; mapped_meta = 0; break;
            case 152: mapped_id = BLOCK_REDSTONE_LAMP_ON; mapped_meta = 0; break;
            case 153: mapped_id = BLOCK_COAL_ORE; mapped_meta = 0; break;
            case 154: mapped_id = BLOCK_CAULDRON; mapped_meta = meta & 3; break;
            case 155: mapped_id = BLOCK_IRON_BLOCK; mapped_meta = 0; break;
            case 156: mapped_id = BLOCK_STONE_BRICK_STAIRS; mapped_meta = meta & 7; break;
            case 157: mapped_id = BLOCK_POWERED_RAIL; mapped_meta = meta & 15; break;
            case 158: mapped_id = BLOCK_DISPENSER; mapped_meta = meta & 7; break;
            case 159: mapped_id = BLOCK_WOOL; mapped_meta = meta & 15; break;
            case 160: mapped_id = BLOCK_GLASS_PANE; mapped_meta = 0; break;
            case 161: {
                int flags = meta & 12;
                mapped_id = BLOCK_LEAVES;
                mapped_meta = flags | ((meta & 1) ? BLOCK_META_WOOD_SPRUCE : BLOCK_META_WOOD_JUNGLE);
                break;
            }
            case 162: {
                int axis = meta & 12;
                mapped_id = BLOCK_LOG;
                mapped_meta = axis | ((meta & 1) ? BLOCK_META_WOOD_SPRUCE : BLOCK_META_WOOD_JUNGLE);
                break;
            }
            case 163: case 164: mapped_id = BLOCK_WOOD_STAIRS; mapped_meta = meta & 7; break;
            case 165: mapped_id = BLOCK_WOOL; mapped_meta = 5; break;                  /* lime wool */
            case 166: mapped_id = BLOCK_GLASS; mapped_meta = 0; break;                /* visible barrier */
            case 167: mapped_id = BLOCK_TRAPDOOR; mapped_meta = meta & 15; break;
            case 168: mapped_id = BLOCK_MOSSY_COBBLESTONE; mapped_meta = 0; break;
            case 169: mapped_id = BLOCK_GLOWSTONE; mapped_meta = 0; break;
            case 170: mapped_id = BLOCK_WOOL; mapped_meta = 4; break;                  /* yellow wool */
            case 171: mapped_id = BLOCK_SNOW_LAYER; mapped_meta = 0; break;
            case 172: mapped_id = BLOCK_CLAY; mapped_meta = 0; break;
            case 173: mapped_id = BLOCK_OBSIDIAN; mapped_meta = 0; break;
            case 174: mapped_id = BLOCK_ICE; mapped_meta = 0; break;
            case 175:
                if ((meta & 7) == 0) mapped_id = BLOCK_YELLOW_FLOWER;
                else if ((meta & 7) == 2 || (meta & 7) == 3) mapped_id = BLOCK_TALL_GRASS;
                else mapped_id = BLOCK_RED_ROSE;
                mapped_meta = ((meta & 7) == 3) ? BLOCK_META_TALL_GRASS_FERN : 0;
                break;
            case 176: mapped_id = BLOCK_SIGN_POST; mapped_meta = meta & 15; break;
            case 177: mapped_id = BLOCK_WALL_SIGN; mapped_meta = meta & 7; break;
            case 179: mapped_id = BLOCK_SANDSTONE; mapped_meta = (meta & 3) <= 2 ? (meta & 3) : 0; break;
            case 180: mapped_id = BLOCK_STONE_BRICK_STAIRS; mapped_meta = meta & 7; break;
            case 181: mapped_id = BLOCK_SANDSTONE; mapped_meta = 0; break;
            case 182: mapped_id = BLOCK_SLAB; mapped_meta = (meta & 8) | 1; break;
            case 183: case 184: case 185: case 186: case 187:
                mapped_id = BLOCK_FENCE_GATE; mapped_meta = meta & 15; break;
            case 188: case 189: case 190: case 191: case 192:
                mapped_id = BLOCK_FENCE; mapped_meta = 0; break;
            case 193: case 194: case 195: case 196: case 197:
                mapped_id = BLOCK_WOOD_DOOR; mapped_meta = meta & 15; break;
            default: mapped_id = BLOCK_STONE; mapped_meta = 0; break;
        }
    }

    if (out_id) *out_id = mapped_id;
    if (out_meta) *out_meta = mapped_meta;
}

int pex_java47_translate_block_id(int id) {
    int mapped = BLOCK_STONE;
    j47_translate_block_state_parts(id, 0, &mapped, NULL);
    return mapped;
}

static void j47_translate_item_stack(int id, int damage, int *out_id, int *out_damage) {
    int mapped_id = id;
    int mapped_damage = damage;

    if (id <= 0) {
        mapped_id = 0;
        mapped_damage = 0;
    } else if (id <= 255) {
        j47_translate_block_state_parts(id, damage, &mapped_id, &mapped_damage);
    } else if (id <= 385) {
        if (id == ITEM_FISH_RAW) {
            int fish = damage & 3;
            mapped_id = fish == 3 ? ITEM_SPIDER_EYE : ITEM_FISH_RAW;                  /* pufferfish is visibly unsafe */
            mapped_damage = 0;
        } else if (id == ITEM_FISH_COOKED) {
            mapped_damage = 0;
        }
    } else {
        mapped_damage = 0;
        switch (id) {
            case 386: case 387: mapped_id = ITEM_BOOK; break;
            case 388: mapped_id = ITEM_DIAMOND; break;
            case 389: mapped_id = ITEM_PAINTING; break;
            case 390: mapped_id = ITEM_BRICK; break;
            case 391: case 392: mapped_id = ITEM_APPLE_RED; break;
            case 393: mapped_id = ITEM_BREAD; break;
            case 394: mapped_id = ITEM_SPIDER_EYE; break;
            case 395: mapped_id = ITEM_MAP; break;
            case 396: mapped_id = ITEM_APPLE_GOLD; break;
            case 397: mapped_id = BLOCK_PUMPKIN; break;
            case 398: mapped_id = ITEM_FISHING_ROD; break;
            case 399: mapped_id = ITEM_DIAMOND; break;
            case 400: mapped_id = ITEM_CAKE; break;
            case 401: mapped_id = ITEM_FIREBALL_CHARGE; break;
            case 402: mapped_id = ITEM_GUNPOWDER; break;
            case 403: mapped_id = ITEM_BOOK; break;
            case 404: mapped_id = ITEM_REDSTONE_REPEATER; break;
            case 405: mapped_id = ITEM_BRICK; break;
            case 406: mapped_id = ITEM_FLINT; break;
            case 407: mapped_id = ITEM_MINECART_EMPTY; break;
            case 408: mapped_id = ITEM_MINECART_CRATE; break;
            case 409: mapped_id = ITEM_FLINT; break;
            case 410: mapped_id = ITEM_DIAMOND; break;
            case 411: mapped_id = ITEM_CHICKEN_RAW; break;
            case 412: mapped_id = ITEM_CHICKEN_COOKED; break;
            case 413: mapped_id = ITEM_BOWL_SOUP; break;
            case 414: mapped_id = ITEM_GHAST_TEAR; break;
            case 415: mapped_id = ITEM_LEATHER; break;
            case 416: mapped_id = ITEM_SIGN; break;
            case 417: mapped_id = ITEM_PLATE_IRON; break;
            case 418: mapped_id = ITEM_PLATE_GOLD; break;
            case 419: mapped_id = ITEM_PLATE_DIAMOND; break;
            case 420: mapped_id = ITEM_STRING; break;
            case 421: mapped_id = ITEM_SIGN; break;
            case 422: mapped_id = ITEM_MINECART_EMPTY; break;
            case 423: mapped_id = ITEM_PORK_RAW; break;
            case 424: mapped_id = ITEM_PORK_COOKED; break;
            case 425: mapped_id = ITEM_SIGN; break;
            case 427: case 428: case 429: case 430: case 431: mapped_id = ITEM_DOOR_WOOD; break;
            default:
                if (id >= 2256 && id <= 2267) mapped_id = id;
                else mapped_id = ITEM_PAPER;                                           /* abstract server/menu item */
                break;
        }
    }

    if (out_id) *out_id = mapped_id;
    if (out_damage) *out_damage = mapped_damage;
}

int pex_java47_translate_item_id(int id) {
    int mapped = ITEM_PAPER;
    j47_translate_item_stack(id, 0, &mapped, NULL);
    return mapped;
}

int pex_java47_translate_mob_type(int type) {
    switch (type) {
        case 50: return PASSIVE_MOB_CREEPER;
        case 51: return PASSIVE_MOB_SKELETON;
        case 52: return PASSIVE_MOB_SPIDER;
        case 54: return PASSIVE_MOB_ZOMBIE;
        case 55: return PASSIVE_MOB_SLIME;
        case 56: return PASSIVE_MOB_GHAST;
        case 57: return PASSIVE_MOB_PIG_ZOMBIE;
        case 58: return PASSIVE_MOB_ENDERMAN;
        case 59: return PASSIVE_MOB_CAVE_SPIDER;
        case 60: return PASSIVE_MOB_SILVERFISH;
        case 61: return PASSIVE_MOB_BLAZE;
        case 62: return PASSIVE_MOB_MAGMA_CUBE;
        case 53: return PASSIVE_MOB_GIANT;
        case 63: return PASSIVE_MOB_ENDER_DRAGON;
        case 90: return PASSIVE_MOB_PIG;
        case 91: return PASSIVE_MOB_SHEEP;
        case 92: return PASSIVE_MOB_COW;
        case 93: return PASSIVE_MOB_CHICKEN;
        case 94: return PASSIVE_MOB_SQUID;
        case 95: return PASSIVE_MOB_WOLF;
        case 96: return PASSIVE_MOB_MOOSHROOM;
        case 97: return PASSIVE_MOB_SNOWMAN;
        case 98: return PASSIVE_MOB_OCELOT;
        case 99: return PASSIVE_MOB_IRON_GOLEM;
        case 120: return PASSIVE_MOB_VILLAGER;
        default: return PASSIVE_MOB_PIG;
    }
}

static int j47_read_item_ex(J47Reader *r, ItemStack *out, J47RawItem *raw_out) {
    ItemStack st;
    J47RawItem raw;
    memset(&st, 0, sizeof(st));
    memset(&raw, 0, sizeof(raw));
    int16_t raw_id = (int16_t)j47_r_be16(r);
    if (r->failed) return 0;
    if (raw_id < 0) {
        if (out) *out = st;
        if (raw_out) *raw_out = raw;
        return 1;
    }
    raw.id = raw_id;
    raw.count = (int)(int8_t)j47_r_u8(r);
    raw.damage = (int)(int16_t)j47_r_be16(r);
    st.count = raw.count;
    j47_translate_item_stack(raw.id, raw.damage, &st.id, &st.damage);
    size_t nbt_start = r->pos;
    if (!j47_parse_item_nbt(r, &raw)) return 0;
    size_t nbt_len = r->pos - nbt_start;
    if (nbt_len <= sizeof(raw.nbt)) {
        raw.nbt_len = nbt_len;
        if (nbt_len) memcpy(raw.nbt, r->data + nbt_start, nbt_len);
    }
    if (st.count < 1) st.count = 1;
    if (raw.count < 1) raw.count = 1;
    if (raw.has_enchantment) {
        st.enchant_id[0] = 1;
        st.enchant_level[0] = 1;
    }
    if (out) *out = st;
    if (raw_out) *raw_out = raw;
    return !r->failed;
}

static int j47_read_item(J47Reader *r, ItemStack *out) {
    return j47_read_item_ex(r, out, NULL);
}

static void j47_write_raw_item(J47Writer *w, const J47RawItem *raw, int count_override) {
    if (!raw || raw->id <= 0 || (count_override == 0) || (count_override < 0 && raw->count <= 0)) {
        j47_w_be16(w, -1);
        return;
    }
    int count = count_override > 0 ? count_override : raw->count;
    j47_w_be16(w, raw->id);
    j47_w_u8(w, count);
    j47_w_be16(w, raw->damage);
    if (raw->nbt_len > 0) j47_w_bytes(w, raw->nbt, raw->nbt_len);
    else j47_w_u8(w, 0);
}

static void j47_write_item(J47Writer *w, int item_id, int count, int damage) {
    if (item_id <= 0 || count <= 0) { j47_w_be16(w, -1); return; }
    j47_w_be16(w, item_id);
    j47_w_u8(w, count);
    j47_w_be16(w, damage);
    j47_w_u8(w, 0); /* TAG_End: local 1.2.5 items do not have arbitrary NBT. */
}

static int j47_read_block_pos(J47Reader *r, int *x, int *y, int *z) {
    uint64_t v = (uint64_t)j47_r_be64(r);
    if (r->failed) return 0;
    int xx = (int)(v >> 38);
    int yy = (int)((v >> 26) & 0xfff);
    int zz = (int)(v & 0x3ffffff);
    if (xx & 0x2000000) xx -= 0x4000000;
    if (zz & 0x2000000) zz -= 0x4000000;
    if (x) *x = xx;
    if (y) *y = yy;
    if (z) *z = zz;
    return 1;
}

static void j47_write_block_pos(J47Writer *w, int x, int y, int z) {
    uint64_t v = (((uint64_t)x & 0x3ffffffu) << 38) |
                 (((uint64_t)y & 0xfffu) << 26) |
                 ((uint64_t)z & 0x3ffffffu);
    j47_w_be64(w, (int64_t)v);
}

static int j47_skip_metadata(J47Reader *r) {
    for (;;) {
        int h = (int)j47_r_u8(r);
        if (r->failed) return 0;
        if (h == 0x7f) return 1;
        switch ((h >> 5) & 7) {
            case 0: (void)j47_r_u8(r); break;
            case 1: (void)j47_r_be16(r); break;
            case 2: (void)j47_r_be32(r); break;
            case 3: (void)j47_r_float(r); break;
            case 4: { char tmp[512]; if (!j47_r_string(r, tmp, sizeof(tmp))) return 0; break; }
            case 5: { ItemStack st; if (!j47_read_item(r, &st)) return 0; break; }
            case 6: (void)j47_r_be32(r); (void)j47_r_be32(r); (void)j47_r_be32(r); break;
            case 7: (void)j47_r_float(r); (void)j47_r_float(r); (void)j47_r_float(r); break;
        }
        if (r->failed) return 0;
    }
}

static J47Entity *j47_entity_find(int entity_id) {
    for (int i = 0; i < J47_ENTITY_MAX; i++) if (g_j47.entities[i].used && g_j47.entities[i].entity_id == entity_id) return &g_j47.entities[i];
    return NULL;
}

static J47Entity *j47_entity_alloc(int entity_id) {
    J47Entity *e = j47_entity_find(entity_id);
    if (e) return e;
    for (int i = 0; i < J47_ENTITY_MAX; i++) if (!g_j47.entities[i].used) {
        memset(&g_j47.entities[i], 0, sizeof(g_j47.entities[i]));
        g_j47.entities[i].used = 1;
        g_j47.entities[i].entity_id = entity_id;
        g_j47.entities[i].player_slot = -1;
        g_j47.entities[i].mob_slot = -1;
        g_j47.entities[i].drop_slot = -1;
        g_j47.entities[i].proxy_item_drop_slot = -1;
        return &g_j47.entities[i];
    }
    return NULL;
}

static void j47_entity_remove(int entity_id) {
    J47Entity *e = j47_entity_find(entity_id);
    if (!e) return;
    if (e->kind == J47_ENTITY_PLAYER && e->player_slot >= 0 && e->player_slot < PEX_NET_MAX_PLAYERS) {
        int slot = e->player_slot;
        for (int i = slot; i + 1 < g_mp_player_count; i++) {
            g_mp_players[i] = g_mp_players[i + 1];
            g_mp_prev_players[i] = g_mp_prev_players[i + 1];
            for (int k = 0; k < J47_ENTITY_MAX; k++) if (g_j47.entities[k].used && g_j47.entities[k].player_slot == i + 1) g_j47.entities[k].player_slot = i;
        }
        if (g_mp_player_count > 0) g_mp_player_count--;
    } else if (e->kind == J47_ENTITY_MOB && e->mob_slot >= 0 && e->mob_slot < MAX_PASSIVE_MOBS) {
        g_passive_mobs[e->mob_slot].active = 0;
    } else if (e->kind == J47_ENTITY_DROP && e->drop_slot >= 0 && e->drop_slot < MAX_DROP_ENTITIES) {
        g_drops[e->drop_slot].active = 0;
    }
    if (e->proxy_item_drop_slot >= 0 && e->proxy_item_drop_slot < MAX_DROP_ENTITIES) {
        g_drops[e->proxy_item_drop_slot].active = 0;
    }
    memset(e, 0, sizeof(*e));
}

static void j47_clear_remote_entities(void) {
    for (int i = 0; i < J47_ENTITY_MAX; i++) {
        if (g_j47.entities[i].used) j47_entity_remove(g_j47.entities[i].entity_id);
    }
    g_mp_player_count = 0;
}

static void j47_reset_world_for_respawn(void) {
    memset(g_flat_blocks, 0, sizeof(g_flat_blocks));
    memset(g_flat_meta, 0, sizeof(g_flat_meta));
    memset(g_flat_levels, 0, sizeof(g_flat_levels));
    memset(g_flat_block_light, 0, sizeof(g_flat_block_light));
    memset(g_flat_sky_light, 0, sizeof(g_flat_sky_light));
    memset(g_flat_world_chunk_generated, 0, sizeof(g_flat_world_chunk_generated));
    memset(g_flat_world_chunk_valid, 0, sizeof(g_flat_world_chunk_valid));
    memset(g_flat_chunk_light_ready, 0, sizeof(g_flat_chunk_light_ready));
    memset(g_flat_chunk_light_valid, 0, sizeof(g_flat_chunk_light_valid));
    for (int cz = 0; cz < FLAT_RENDER_CHUNKS; cz++) {
        for (int cx = 0; cx < FLAT_RENDER_CHUNKS; cx++) {
            g_flat_world_chunk_dirty[cz][cx] = 1;
            g_flat_world_chunk_has_liquid[cz][cx] = 0;
        }
    }
    g_flat_renderer_sort_dirty = 1;
    j47_clear_remote_entities();
    g_j47.chunks_received = 0;
    g_j47.world_ready = 0;
    g_j47.position_received = 0;
}

static J47Profile *j47_profile_find_uuid(const unsigned char uuid[16]) {
    for (int i = 0; i < J47_PROFILE_MAX; i++) if (g_j47.profiles[i].used && memcmp(g_j47.profiles[i].uuid, uuid, 16) == 0) return &g_j47.profiles[i];
    return NULL;
}

static J47Profile *j47_profile_put(const unsigned char uuid[16], const char *name) {
    J47Profile *p = j47_profile_find_uuid(uuid);
    if (!p) for (int i = 0; i < J47_PROFILE_MAX; i++) if (!g_j47.profiles[i].used) { p = &g_j47.profiles[i]; memset(p, 0, sizeof(*p)); p->used = 1; memcpy(p->uuid, uuid, 16); break; }
    if (p && name) snprintf(p->name, sizeof(p->name), "%.*s", (int)sizeof(p->name) - 1, name);
    return p;
}

static int j47_drop_alloc(void) {
    for (int i = 0; i < MAX_DROP_ENTITIES; i++) if (!g_drops[i].active) return i;
    return -1;
}

static int j47_player_alloc(void) {
    if (g_mp_player_count >= PEX_NET_MAX_PLAYERS) return -1;
    return g_mp_player_count++;
}

static void j47_sync_proxy_item_visual(J47Entity *e) {
    if (!e || !e->used || !e->is_armor_stand) return;
    int chosen = -1;
    /* Armor-stand menu displays overwhelmingly use the helmet or hand slot.
       Prefer those, then fall back through the remaining armor slots. */
    static const int order[5] = {4, 0, 3, 2, 1};
    for (int i = 0; i < 5; i++) {
        int slot = order[i];
        if (!stack_empty(&e->equipment[slot])) { chosen = slot; break; }
    }
    if (chosen < 0) {
        if (e->proxy_item_drop_slot >= 0 && e->proxy_item_drop_slot < MAX_DROP_ENTITIES)
            g_drops[e->proxy_item_drop_slot].active = 0;
        e->proxy_item_drop_slot = -1;
        e->held_item_id = e->held_item_count = e->held_item_damage = 0;
        e->held_slot = 0;
        return;
    }

    if (e->proxy_item_drop_slot < 0 || e->proxy_item_drop_slot >= MAX_DROP_ENTITIES ||
        !g_drops[e->proxy_item_drop_slot].active) {
        e->proxy_item_drop_slot = j47_drop_alloc();
        if (e->proxy_item_drop_slot < 0) return;
        memset(&g_drops[e->proxy_item_drop_slot], 0, sizeof(g_drops[e->proxy_item_drop_slot]));
        g_drops[e->proxy_item_drop_slot].active = 1;
        g_drops[e->proxy_item_drop_slot].net_id = -e->entity_id;
        g_drops[e->proxy_item_drop_slot].pickup_delay = 32767;
    }

    FlatDroppedItem *d = &g_drops[e->proxy_item_drop_slot];
    d->stack = e->equipment[chosen];
    d->x = d->prev_x = (float)e->x;
    d->y = d->prev_y = (float)e->y + (chosen == 4 ? 1.35f : 0.85f);
    d->z = d->prev_z = (float)e->z;
    d->mx = d->my = d->mz = 0.0f;
    e->held_item_id = d->stack.id;
    e->held_item_count = d->stack.count;
    e->held_item_damage = d->stack.damage;
    e->held_slot = chosen;
}

static int j47_mob_alloc_slot(void) {
    PassiveMob *m = passive_mob_alloc();
    if (!m) return -1;
    return (int)(m - g_passive_mobs);
}

static void j47_set_block_state(int x, int y, int z, int state) {
    if (!flat_in_bounds(x, y, z)) return;
    int id = BLOCK_STONE;
    int meta = 0;
    j47_translate_block_state_parts((state >> 4) & 0xfff, state & 15, &id, &meta);
    int yi = flat_y_index(y), zi = flat_storage_z_index(z), xi = flat_storage_index(x);
    int old = g_flat_blocks[yi][zi][xi];
    g_flat_blocks[yi][zi][xi] = (unsigned char)id;
    g_flat_meta[yi][zi][xi] = (unsigned char)meta;
    g_flat_levels[yi][zi][xi] = block_is_liquid(id) ? (unsigned char)meta : 0;
    flat_update_section_after_block_change(x, y, z, id);
    flat_mark_sections_dirty_near_block(x, y, z);
    flat_mark_chunks_dirty_near(x, z);
    if (old != id) flat_mark_light_dirty_for_change(x, y, z, old, id);
}

static void j47_unload_chunk(int cx, int cz) {
    int lcx = (cx * 16 - g_flat_world_origin_x) / 16;
    int lcz = (cz * 16 - g_flat_world_origin_z) / 16;
    if (!flat_local_chunk_valid(lcx, lcz)) return;
    for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
        for (int z = 0; z < 16; z++) {
            for (int x = 0; x < 16; x++) {
                int wx = cx * 16 + x, wz = cz * 16 + z;
                int yi = flat_y_index(y), zi = flat_storage_z_index(wz), xi = flat_storage_index(wx);
                g_flat_blocks[yi][zi][xi] = 0;
                g_flat_meta[yi][zi][xi] = 0;
                g_flat_levels[yi][zi][xi] = 0;
                g_flat_block_light[yi][zi][xi] = 0;
                g_flat_sky_light[yi][zi][xi] = 0;
            }
        }
    }
    g_flat_world_chunk_generated[lcz][lcx] = 0;
    g_flat_world_chunk_valid[lcz][lcx] = 0;
    g_flat_chunk_light_ready[lcz][lcx] = 0;
    g_flat_chunk_light_valid[lcz][lcx] = 0;
    g_flat_chunk_section_non_empty_mask[lcz][lcx] = 0;
    g_flat_world_chunk_has_liquid[lcz][lcx] = 0;
    g_flat_world_chunk_dirty[lcz][lcx] = 1;
    for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
        g_flat_section_dirty[sy][lcz][lcx] = 0;
        g_flat_section_valid[sy][lcz][lcx] = 0;
    }
    g_flat_renderer_sort_dirty = 1;
}

static void j47_install_chunk(int cx, int cz, int full, unsigned int mask, const unsigned char *data, size_t len, int has_sky) {
    if (full && mask == 0) {
        j47_unload_chunk(cx, cz);
        return;
    }
    size_t pos = 0;
    int lcx = (cx * 16 - g_flat_world_origin_x) / 16;
    int lcz = (cz * 16 - g_flat_world_origin_z) / 16;
    int visible = flat_local_chunk_valid(lcx, lcz);
    if (full && visible) {
        for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) for (int z = 0; z < 16; z++) for (int x = 0; x < 16; x++) {
            int wx = cx * 16 + x, wz = cz * 16 + z;
            int yi = flat_y_index(y), zi = flat_storage_z_index(wz), xi = flat_storage_index(wx);
            g_flat_blocks[yi][zi][xi] = 0; g_flat_meta[yi][zi][xi] = 0; g_flat_levels[yi][zi][xi] = 0;
            g_flat_block_light[yi][zi][xi] = 0; g_flat_sky_light[yi][zi][xi] = has_sky ? 15 : 0;
        }
    }
    for (int sy = 0; sy < 16; sy++) if (mask & (1u << sy)) {
        if (pos + 8192 > len) return;
        const unsigned char *states = data + pos; pos += 8192;
        if (visible) for (int ly = 0; ly < 16; ly++) for (int z = 0; z < 16; z++) for (int x = 0; x < 16; x++) {
            int idx = (ly << 8) | (z << 4) | x;
            int state = states[idx * 2] | (states[idx * 2 + 1] << 8);
            int id = BLOCK_STONE, meta = 0;
            j47_translate_block_state_parts((state >> 4) & 0xfff, state & 15, &id, &meta);
            int wx = cx * 16 + x, wy = sy * 16 + ly, wz = cz * 16 + z;
            int yi = flat_y_index(wy), zi = flat_storage_z_index(wz), xi = flat_storage_index(wx);
            g_flat_blocks[yi][zi][xi] = (unsigned char)id;
            g_flat_meta[yi][zi][xi] = (unsigned char)meta;
            g_flat_levels[yi][zi][xi] = block_is_liquid(id) ? (unsigned char)meta : 0;
        }
    }
    for (int sy = 0; sy < 16; sy++) if (mask & (1u << sy)) {
        if (pos + 2048 > len) return;
        if (visible) for (int i = 0; i < 4096; i++) {
            int nib = (data[pos + (i >> 1)] >> ((i & 1) * 4)) & 15;
            int x = i & 15, z = (i >> 4) & 15, ly = (i >> 8) & 15;
            int wx = cx * 16 + x, wy = sy * 16 + ly, wz = cz * 16 + z;
            g_flat_block_light[flat_y_index(wy)][flat_storage_z_index(wz)][flat_storage_index(wx)] = (unsigned char)nib;
        }
        pos += 2048;
    }
    if (has_sky) for (int sy = 0; sy < 16; sy++) if (mask & (1u << sy)) {
        if (pos + 2048 > len) return;
        if (visible) for (int i = 0; i < 4096; i++) {
            int nib = (data[pos + (i >> 1)] >> ((i & 1) * 4)) & 15;
            int x = i & 15, z = (i >> 4) & 15, ly = (i >> 8) & 15;
            int wx = cx * 16 + x, wy = sy * 16 + ly, wz = cz * 16 + z;
            g_flat_sky_light[flat_y_index(wy)][flat_storage_z_index(wz)][flat_storage_index(wx)] = (unsigned char)nib;
        }
        pos += 2048;
    }
    if (full && pos + 256 <= len) pos += 256;
    if (visible) {
        g_flat_chunk_section_non_empty_mask[lcz][lcx] = (unsigned short)mask;
        for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
            g_flat_section_dirty[sy][lcz][lcx] = 1;
            g_flat_section_valid[sy][lcz][lcx] = 0;
            flat_note_section_mesh_changed(sy, lcz, lcx);
        }
        g_flat_renderer_sort_dirty = 1;
        pex_net_mark_chunk_ready(cx, cz);
    }
    g_j47.chunks_received++;
    if (!g_j47.world_ready && g_j47.position_received && g_j47.chunks_received >= 1) {
        g_j47.world_ready = 1;
        g_j47.progress = 100;
    }
}

static void j47_apply_player_slot_raw(int java_slot, const ItemStack *st, const J47RawItem *raw);
static void j47_apply_player_slot(int java_slot, const ItemStack *st);


static int j47_window_is_container_type(const char *type) {
    /* PexCraft has native crafting-table and furnace screens. Every other
       protocol-47 window is represented by the nearest generic chest layout,
       including hopper, dispenser, beacon, anvil, horse and plugin menus. */
    return type && strcmp(type, "minecraft:crafting_table") && strcmp(type, "minecraft:furnace");
}

static int j47_map_pex_slot_to_java(int pex_slot) {
    if (pex_slot == -999) return -999;

    if (g_j47.window.open) {
        int container = g_j47.window.container_slots;
        if (j47_window_is_container_type(g_j47.window.type)) {
            if (pex_slot >= 200 && pex_slot < 200 + container) return pex_slot - 200;
            if (pex_slot >= 9 && pex_slot <= 35) return container + (pex_slot - 9);
            if (pex_slot >= 0 && pex_slot <= 8) return container + 27 + pex_slot;
        } else if (!strcmp(g_j47.window.type, "minecraft:crafting_table")) {
            if (pex_slot == 309) return 0;
            if (pex_slot >= 300 && pex_slot <= 308) return 1 + (pex_slot - 300);
            if (pex_slot >= 9 && pex_slot <= 35) return 10 + (pex_slot - 9);
            if (pex_slot >= 0 && pex_slot <= 8) return 37 + pex_slot;
        } else if (!strcmp(g_j47.window.type, "minecraft:furnace")) {
            if (pex_slot >= 400 && pex_slot <= 402) return pex_slot - 400;
            if (pex_slot >= 9 && pex_slot <= 35) return 3 + (pex_slot - 9);
            if (pex_slot >= 0 && pex_slot <= 8) return 30 + pex_slot;
        } else {
            if (pex_slot >= 9 && pex_slot <= 35) return container + (pex_slot - 9);
            if (pex_slot >= 0 && pex_slot <= 8) return container + 27 + pex_slot;
        }
        return -32768;
    }

    if (pex_slot == 104) return 0;
    if (pex_slot >= 100 && pex_slot <= 103) return 1 + (pex_slot - 100);
    if (pex_slot >= 110 && pex_slot <= 113) return 5 + (pex_slot - 110);
    if (pex_slot >= 9 && pex_slot <= 35) return pex_slot;
    if (pex_slot >= 0 && pex_slot <= 8) return 36 + pex_slot;
    return -32768;
}


static const J47RawItem *j47_raw_for_java_slot(int java_slot) {
    if (java_slot < 0) return NULL;
    if (g_j47.window.open && java_slot < J47_WINDOW_MAX_SLOTS) {
        const J47RawItem *raw = &g_j47.window.raw_slots[java_slot];
        if (raw->id > 0) return raw;
    }
    if (!g_j47.window.open && java_slot < (int)(sizeof(g_j47.raw_player_slots) / sizeof(g_j47.raw_player_slots[0]))) {
        const J47RawItem *raw = &g_j47.raw_player_slots[java_slot];
        if (raw->id > 0) return raw;
    }
    return NULL;
}

static const J47RawItem *j47_raw_for_pex_slot(int pex_slot) {
    int java_slot = j47_map_pex_slot_to_java(pex_slot);
    if (java_slot == -32768 || java_slot == -999) return NULL;
    return j47_raw_for_java_slot(java_slot);
}

const char *pex_java47_slot_custom_name(int pex_slot) {
    const J47RawItem *raw = j47_raw_for_pex_slot(pex_slot);
    return raw && raw->display_name[0] ? raw->display_name : NULL;
}

int pex_java47_slot_lore(int pex_slot, const char **out_lines, int max_lines) {
    const J47RawItem *raw = j47_raw_for_pex_slot(pex_slot);
    if (!raw || !out_lines || max_lines <= 0) return 0;
    int count = raw->lore_count;
    if (count > max_lines) count = max_lines;
    for (int i = 0; i < count; ++i) out_lines[i] = raw->lore[i];
    return count;
}

static const J47RawItem *j47_raw_for_selected_hotbar(int display_id, int display_damage) {
    int java_slot = 36 + g_selected_hotbar_slot;
    if (java_slot < 36 || java_slot > 44) return NULL;
    const J47RawItem *raw = &g_j47.raw_player_slots[java_slot];
    if (raw->id <= 0) return NULL;
    int mapped_id = 0, mapped_damage = 0;
    j47_translate_item_stack(raw->id, raw->damage, &mapped_id, &mapped_damage);
    if (mapped_id != display_id || mapped_damage != display_damage) return NULL;
    return raw;
}

static void j47_write_held_or_local_item(J47Writer *w, int item_id, int count, int damage) {
    const J47RawItem *raw = j47_raw_for_selected_hotbar(item_id, damage);
    if (raw) j47_write_raw_item(w, raw, count);
    else j47_write_item(w, item_id, count, damage);
}

static void j47_apply_open_window_slot_raw(int slot, const ItemStack *st, const J47RawItem *raw) {
    if (!st || slot < 0) return;
    if (slot < J47_WINDOW_MAX_SLOTS) {
        g_j47.window.slots[slot] = *st;
        if (raw) g_j47.window.raw_slots[slot] = *raw;
        else if (stack_empty(st)) memset(&g_j47.window.raw_slots[slot], 0, sizeof(g_j47.window.raw_slots[slot]));
    }

    if (j47_window_is_container_type(g_j47.window.type)) {
        int container = g_j47.window.container_slots;
        if (slot >= container) {
            int relative = slot - container;
            if (relative < 27) j47_apply_player_slot_raw(9 + relative, st, raw);
            else if (relative < 36) j47_apply_player_slot_raw(36 + relative - 27, st, raw);
        }
    } else if (!strcmp(g_j47.window.type, "minecraft:crafting_table")) {
        if (slot >= 1 && slot <= 9) g_workbench_grid[slot - 1] = *st;
        else if (slot >= 10 && slot <= 36) j47_apply_player_slot_raw(9 + slot - 10, st, raw);
        else if (slot >= 37 && slot <= 45) j47_apply_player_slot_raw(36 + slot - 37, st, raw);
    } else if (!strcmp(g_j47.window.type, "minecraft:furnace")) {
        if (slot >= 0 && slot <= 2) {
            ItemStack *dst = furnace_get_slot_ptr(slot);
            if (dst) *dst = *st;
        } else if (slot >= 3 && slot <= 29) j47_apply_player_slot_raw(9 + slot - 3, st, raw);
        else if (slot >= 30 && slot <= 38) j47_apply_player_slot_raw(36 + slot - 30, st, raw);
    }
}

static void j47_apply_open_window_slot(int slot, const ItemStack *st) {
    j47_apply_open_window_slot_raw(slot, st, NULL);
}

static void j47_apply_player_slot_raw(int java_slot, const ItemStack *st, const J47RawItem *raw) {
    if (!st) return;
    if (java_slot >= 0 && java_slot < (int)(sizeof(g_j47.raw_player_slots) / sizeof(g_j47.raw_player_slots[0]))) {
        if (raw) g_j47.raw_player_slots[java_slot] = *raw;
        else if (stack_empty(st)) memset(&g_j47.raw_player_slots[java_slot], 0, sizeof(g_j47.raw_player_slots[java_slot]));
    }
    if (java_slot >= 36 && java_slot <= 44) g_inventory[java_slot - 36] = *st;
    else if (java_slot >= 9 && java_slot <= 35) g_inventory[java_slot] = *st;
    else if (java_slot >= 5 && java_slot <= 8) g_armor_inventory[8 - java_slot] = *st;
    armor_sync_player_armor();
}

static void j47_apply_player_slot(int java_slot, const ItemStack *st) {
    j47_apply_player_slot_raw(java_slot, st, NULL);
}

static void j47_apply_window_items(int window_id, ItemStack *items, int count) {
    if (window_id == 0) {
        for (int i = 0; i < count; i++) j47_apply_player_slot(i, &items[i]);
        return;
    }
    if (!g_j47.window.open || window_id != g_j47.window.window_id) return;
    int n = count < J47_WINDOW_MAX_SLOTS ? count : J47_WINDOW_MAX_SLOTS;
    g_j47.window.total_slots = n;
    for (int i = 0; i < n; i++) j47_apply_open_window_slot(i, &items[i]);
}

static void j47_spawn_player(J47Reader *r) {
    int32_t entity_id = 0;
    if (!j47_r_varint(r, &entity_id) || r->pos + 16 > r->len) return;
    unsigned char uuid[16]; memcpy(uuid, r->data + r->pos, 16); r->pos += 16;
    double x = (double)j47_r_be32(r) / 32.0;
    double y = (double)j47_r_be32(r) / 32.0;
    double z = (double)j47_r_be32(r) / 32.0;
    float yaw = (float)((int8_t)j47_r_u8(r)) * 360.0f / 256.0f;
    float pitch = (float)((int8_t)j47_r_u8(r)) * 360.0f / 256.0f;
    int held = (int16_t)j47_r_be16(r);
    if (!j47_skip_metadata(r) || r->failed) return;
    J47Entity *e = j47_entity_alloc(entity_id);
    if (!e) return;
    int slot = e->player_slot;
    if (slot < 0) slot = j47_player_alloc();
    if (slot < 0) return;
    e->kind = J47_ENTITY_PLAYER; e->player_slot = slot; e->x = x; e->y = y; e->z = z; e->yaw = yaw; e->pitch = pitch;
    e->held_item_id = pex_java47_translate_item_id(held);
    e->held_item_count = e->held_item_id > 0 ? 1 : 0;
    PexNetPlayerState *p = &g_mp_players[slot];
    memset(p, 0, sizeof(*p));
    p->player_id = entity_id; p->x = (float)x; p->y = (float)y + 1.62f; p->z = (float)z;
    p->yaw = yaw; p->pitch = pitch; p->health = 20;
    J47Profile *profile = j47_profile_find_uuid(uuid);
    snprintf(p->name, sizeof(p->name), "%s", profile && profile->name[0] ? profile->name : "Player");
    g_mp_prev_players[slot] = *p;
}

static void j47_spawn_mob(J47Reader *r) {
    int32_t entity_id = 0;
    if (!j47_r_varint(r, &entity_id)) return;
    int java_type = (int)j47_r_u8(r);
    double x = (double)j47_r_be32(r) / 32.0;
    double y = (double)j47_r_be32(r) / 32.0;
    double z = (double)j47_r_be32(r) / 32.0;
    float yaw = (float)((int8_t)j47_r_u8(r)) * 360.0f / 256.0f;
    float pitch = (float)((int8_t)j47_r_u8(r)) * 360.0f / 256.0f;
    float head = (float)((int8_t)j47_r_u8(r)) * 360.0f / 256.0f;
    (void)j47_r_be16(r); (void)j47_r_be16(r); (void)j47_r_be16(r);
    if (!j47_skip_metadata(r) || r->failed) return;
    J47Entity *e = j47_entity_alloc(entity_id);
    if (!e) return;
    int slot = e->mob_slot;
    if (slot < 0) slot = j47_mob_alloc_slot();
    if (slot < 0) return;
    int type = pex_java47_translate_mob_type(java_type);
    passive_mob_init(&g_passive_mobs[slot], type, (float)x, (float)y, (float)z);
    PassiveMob *m = &g_passive_mobs[slot];
    m->entity_id = entity_id; m->yaw = m->prev_yaw = yaw; m->pitch = m->prev_pitch = pitch;
    m->head_yaw = m->prev_head_yaw = head; m->render_yaw = m->prev_render_yaw = yaw;
    e->kind = J47_ENTITY_MOB; e->java_type = java_type; e->mob_slot = slot; e->x=x; e->y=y; e->z=z; e->yaw=yaw; e->pitch=pitch; e->head_yaw=head;
}

static void j47_spawn_object(J47Reader *r) {
    int32_t entity_id = 0;
    if (!j47_r_varint(r, &entity_id)) return;
    int type = (int)j47_r_u8(r);
    double x = (double)j47_r_be32(r) / 32.0;
    double y = (double)j47_r_be32(r) / 32.0;
    double z = (double)j47_r_be32(r) / 32.0;
    float pitch = (float)((int8_t)j47_r_u8(r)) * 360.0f / 256.0f;
    float yaw = (float)((int8_t)j47_r_u8(r)) * 360.0f / 256.0f;
    int data = j47_r_be32(r);
    int vx=0,vy=0,vz=0;
    if (data > 0) { vx=(int16_t)j47_r_be16(r); vy=(int16_t)j47_r_be16(r); vz=(int16_t)j47_r_be16(r); }
    if (r->failed) return;
    J47Entity *e = j47_entity_alloc(entity_id); if (!e) return;
    e->kind = J47_ENTITY_OTHER; e->java_type = type; e->x=x; e->y=y; e->z=z; e->yaw=yaw; e->pitch=pitch;
    if (type == 2) { /* dropped item: item metadata arrives in S1C */
        int slot = j47_drop_alloc();
        if (slot >= 0) {
            FlatDroppedItem *d=&g_drops[slot]; memset(d,0,sizeof(*d)); d->active=1; d->net_id=entity_id;
            d->x=d->prev_x=(float)x; d->y=d->prev_y=(float)y; d->z=d->prev_z=(float)z;
            d->mx=(float)vx/8000.0f; d->my=(float)vy/8000.0f; d->mz=(float)vz/8000.0f; d->stack.id=BLOCK_STONE; d->stack.count=1;
            e->kind=J47_ENTITY_DROP; e->drop_slot=slot;
        }
    } else if (type == 78) { /* EntityArmorStand: protocol-47 spawn-object type */
        int slot = j47_mob_alloc_slot();
        if (slot >= 0) {
            /* PexCraft has no armor-stand model. A pig proxy supplies a visible,
               targetable entity while equipment is rendered as a local item
               display above it. The real Java entity ID remains authoritative. */
            passive_mob_init(&g_passive_mobs[slot], PASSIVE_MOB_PIG, (float)x, (float)y, (float)z);
            PassiveMob *m = &g_passive_mobs[slot];
            m->entity_id = entity_id;
            m->yaw = m->prev_yaw = yaw;
            m->pitch = m->prev_pitch = pitch;
            m->head_yaw = m->prev_head_yaw = yaw;
            m->render_yaw = m->prev_render_yaw = yaw;
            e->kind = J47_ENTITY_MOB;
            e->mob_slot = slot;
            e->is_armor_stand = 1;
        }
    }
}

static void j47_apply_entity_position(J47Entity *e, double x, double y, double z, float yaw, float pitch, int set_rot) {
    if (!e) return;
    e->x=x; e->y=y; e->z=z; if(set_rot){e->yaw=yaw;e->pitch=pitch;}
    if (e->kind==J47_ENTITY_PLAYER && e->player_slot>=0 && e->player_slot<g_mp_player_count) {
        PexNetPlayerState *p=&g_mp_players[e->player_slot]; g_mp_prev_players[e->player_slot]=*p;
        p->x=(float)x;p->y=(float)y+1.62f;p->z=(float)z;if(set_rot){p->yaw=yaw;p->pitch=pitch;}
    } else if (e->kind==J47_ENTITY_MOB && e->mob_slot>=0 && e->mob_slot<MAX_PASSIVE_MOBS) {
        PassiveMob *m=&g_passive_mobs[e->mob_slot]; m->prev_x=m->x;m->prev_y=m->y;m->prev_z=m->z;m->x=(float)x;m->y=(float)y;m->z=(float)z;
        if(set_rot){m->prev_yaw=m->yaw;m->yaw=yaw;m->prev_pitch=m->pitch;m->pitch=pitch;m->prev_head_yaw=m->head_yaw;m->head_yaw=yaw;}
    } else if (e->kind==J47_ENTITY_DROP && e->drop_slot>=0 && e->drop_slot<MAX_DROP_ENTITIES) {
        FlatDroppedItem *d=&g_drops[e->drop_slot]; d->prev_x=d->x;d->prev_y=d->y;d->prev_z=d->z;d->x=(float)x;d->y=(float)y;d->z=(float)z;
    }
    if (e->is_armor_stand) j47_sync_proxy_item_visual(e);
}

static void j47_parse_entity_metadata(J47Reader *r, int entity_id) {
    J47Entity *e=j47_entity_find(entity_id);
    for (;;) {
        int h=(int)j47_r_u8(r);
        if(r->failed||h==0x7f) break;
        int type=(h>>5)&7, index=h&31;
        if(type==5){
            ItemStack st;
            if(!j47_read_item(r,&st))return;
            if(e&&e->kind==J47_ENTITY_DROP&&index==10&&e->drop_slot>=0)g_drops[e->drop_slot].stack=st;
        } else if(type==0){
            int v=(int8_t)j47_r_u8(r);
            if(e&&index==0)e->metadata_flags=v;
            if(e&&e->kind==J47_ENTITY_MOB&&index==6&&e->mob_slot>=0)g_passive_mobs[e->mob_slot].health=v;
        } else if(type==1){
            (void)j47_r_be16(r);
        } else if(type==2){
            (void)j47_r_be32(r);
        } else if(type==3){
            float v=j47_r_float(r);
            if(e&&e->kind==J47_ENTITY_MOB&&index==6&&e->mob_slot>=0)g_passive_mobs[e->mob_slot].health=(int)ceilf(v);
        } else if(type==4){
            char tmp[512];
            if(!j47_r_string(r,tmp,sizeof(tmp)))return;
            if(e&&index==2)snprintf(e->custom_name,sizeof(e->custom_name),"%.*s",(int)sizeof(e->custom_name)-1,tmp);
        } else if(type==6){
            (void)j47_r_be32(r);(void)j47_r_be32(r);(void)j47_r_be32(r);
        } else if(type==7){
            (void)j47_r_float(r);(void)j47_r_float(r);(void)j47_r_float(r);
        }
    }
    if(e&&e->is_armor_stand)j47_sync_proxy_item_visual(e);
}

static int j47_send_position_look_immediate(double x, double feet_y, double z,
                                              float yaw, float pitch, int on_ground) {
    J47Writer w;
    j47_writer_init(&w, 48);
    j47_w_double(&w, x);
    j47_w_double(&w, feet_y);
    j47_w_double(&w, z);
    j47_w_float(&w, yaw);
    j47_w_float(&w, pitch);
    j47_w_u8(&w, on_ground);
    int ok = j47_send_packet(0x06, w.data, w.len);
    j47_writer_free(&w);
    if (ok) {
        g_j47.last_x = x;
        g_j47.last_y = feet_y;
        g_j47.last_z = z;
        g_j47.last_yaw = yaw;
        g_j47.last_pitch = pitch;
        g_j47.last_on_ground = on_ground;
        g_j47.last_move_sent = now_seconds();
        g_j47.movement_ticks = 0;
    }
    return ok;
}

static void j47_handle_player_list(J47Reader *r) {
    int32_t action=0,count=0;if(!j47_r_varint(r,&action)||!j47_r_varint(r,&count)||count<0||count>10000)return;
    for(int i=0;i<count;i++){
        if(r->pos+16>r->len){r->failed=1;return;}unsigned char uuid[16];memcpy(uuid,r->data+r->pos,16);r->pos+=16;
        if(action==0){char name[64];int32_t props=0,gm=0,ping=0;j47_r_string(r,name,sizeof(name));j47_r_varint(r,&props);
            for(int p=0;p<props;p++){char a[128],b[4096];j47_r_string(r,a,sizeof(a));j47_r_string(r,b,sizeof(b));int sig=j47_r_u8(r);if(sig){char c[4096];j47_r_string(r,c,sizeof(c));}}
            j47_r_varint(r,&gm);j47_r_varint(r,&ping);int display=j47_r_u8(r);if(display){char d[4096];j47_r_string(r,d,sizeof(d));}j47_profile_put(uuid,name);
        } else if(action==1||action==2){int32_t v;j47_r_varint(r,&v);} else if(action==3){int display=j47_r_u8(r);if(display){char d[4096];j47_r_string(r,d,sizeof(d));}}
        else if(action==4){J47Profile*p=j47_profile_find_uuid(uuid);if(p)p->used=0;}
        if(r->failed)return;
    }
}

static int j47_send_client_identity(void) {
    J47Writer settings;
    j47_writer_init(&settings, 64);
    j47_w_string(&settings, "en_US");
    int view_distance = FLAT_RENDER_CHUNKS / 2 - 1;
    if (view_distance < 1) view_distance = 1;
    if (view_distance > 8) view_distance = 8;
    j47_w_u8(&settings, view_distance);
    j47_w_u8(&settings, 0);       /* full chat */
    j47_w_u8(&settings, 1);       /* chat colours */
    j47_w_u8(&settings, 0x7f);    /* all model parts */
    int ok = j47_send_packet(0x15, settings.data, settings.len);
    j47_writer_free(&settings);
    if (!ok) return 0;

    J47Writer brand;
    J47Writer brand_data;
    j47_writer_init(&brand, 64);
    j47_writer_init(&brand_data, 32);
    j47_w_string_n(&brand, "MC|Brand", 20);
    j47_w_string_n(&brand_data, "vanilla", 32767);
    j47_w_bytes(&brand, brand_data.data, brand_data.len);
    ok = j47_send_packet(0x17, brand.data, brand.len);
    j47_writer_free(&brand_data);
    j47_writer_free(&brand);
    return ok;
}

static void j47_handle_play_packet(int packet_id, J47Reader *r) {
    switch(packet_id){
        case 0x00:{int32_t id;if(j47_r_varint(r,&id)){J47Writer w;j47_writer_init(&w,8);j47_w_varint(&w,id);j47_send_packet(0x00,w.data,w.len);j47_writer_free(&w);}break;}
        case 0x01:{g_j47.local_entity_id=j47_r_be32(r);g_mp_player_id=g_j47.local_entity_id;int gm=(int)j47_r_u8(r);g_j47.game_mode=gm&7;g_game_mode=g_j47.game_mode==1?1:0;g_pending_game_mode=g_game_mode;if(!g_game_mode)g_creative_flying=0;g_j47.last_abilities_flags=-1;g_j47.dimension=(int8_t)j47_r_u8(r);g_j47.difficulty=(int)j47_r_u8(r);(void)j47_r_u8(r);char wt[64];j47_r_string(r,wt,sizeof(wt));g_j47.has_join_game=1;g_j47.progress=35;j47_set_status("Downloading terrain");if(!r->failed&&!j47_send_client_identity())j47_fail("Could not send Java client settings");break;}
        case 0x02:{char json[8192],text[512];if(j47_r_string(r,json,sizeof(json))){(void)j47_r_u8(r);j47_json_collect_text(json,text,sizeof(text));if(text[0])hud_add_chat(text);}break;}
        case 0x03:{(void)j47_r_be64(r);g_world_time=j47_r_be64(r);break;}
        case 0x04:{
            int32_t id;
            if(j47_r_varint(r,&id)){
                int slot=(int16_t)j47_r_be16(r);
                ItemStack st;
                if(j47_read_item(r,&st)){
                    J47Entity*e=j47_entity_find(id);
                    if(e){
                        if(slot>=0&&slot<5)e->equipment[slot]=st;
                        if(slot==0){
                            e->held_item_id=st.id;e->held_item_count=st.count;e->held_item_damage=st.damage;e->held_slot=0;
                            PexNetRenderPlayerState*rp=pex_net_find_render_player(id);
                            if(rp){rp->held_item_id=st.id;rp->held_item_count=st.count;rp->held_item_damage=st.damage;rp->held_slot=0;}
                        }
                        if(e->is_armor_stand)j47_sync_proxy_item_visual(e);
                    }
                }
            }
            break;
        }
        case 0x05:{int x,y,z;j47_read_block_pos(r,&x,&y,&z);break;}
        case 0x06:{float hp=j47_r_float(r);int32_t food=20;j47_r_varint(r,&food);float sat=j47_r_float(r);g_player_health=(int)ceilf(hp);g_player_food_level=food;g_player_food_saturation=sat;if(g_player_health<=0&&!g_player_dead)player_die("was slain");else if(g_player_health>0&&g_player_dead){g_player_dead=0;g_player_death_time=0;}break;}
        case 0x07:{g_j47.dimension=j47_r_be32(r);g_j47.difficulty=(int)j47_r_u8(r);g_j47.game_mode=(int)j47_r_u8(r)&7;g_game_mode=g_j47.game_mode==1?1:0;g_pending_game_mode=g_game_mode;if(!g_game_mode)g_creative_flying=0;g_j47.last_abilities_flags=-1;g_player_dead=0;g_player_death_time=0;g_player_motion_x=g_player_motion_y=g_player_motion_z=0.0f;char wt[64];j47_r_string(r,wt,sizeof(wt));j47_reset_world_for_respawn();break;}
        case 0x08:{
            double x=j47_r_double(r),y=j47_r_double(r),z=j47_r_double(r);
            float yaw=j47_r_float(r),pitch=j47_r_float(r);
            int flags=j47_r_u8(r);
            if(flags&1)x+=g_player_x;else g_player_motion_x=0.0f;
            if(flags&2)y+=(g_player_y-1.62);else g_player_motion_y=0.0f;
            if(flags&4)z+=g_player_z;else g_player_motion_z=0.0f;
            if(flags&8)yaw+=g_player_yaw;
            if(flags&16)pitch+=g_player_pitch;
            g_player_x=g_player_prev_x=(float)x;
            g_player_y=g_player_prev_y=(float)y+1.62f;
            g_player_z=g_player_prev_z=(float)z;
            g_player_yaw=g_player_prev_yaw=yaw;
            g_player_pitch=g_player_prev_pitch=pitch;
            flat_multiplayer_recenter_world(g_player_x,g_player_z,1);
            g_j47.position_received=1;
            g_j47.progress=55;
            g_j47.last_movement_game_tick=g_ingame_ticks;
            /* Vanilla 1.8.8 acknowledges S08 with onGround=false. Using the
               local collision flag here can make strict movement checks treat
               the correction as a fabricated landing. */
            j47_send_position_look_immediate(x,y,z,yaw,pitch,0);
            break;
        }
        case 0x09:g_selected_hotbar_slot=(int)j47_r_u8(r);if(g_selected_hotbar_slot<0||g_selected_hotbar_slot>8)g_selected_hotbar_slot=0;break;
        case 0x0B:{int32_t id;if(j47_r_varint(r,&id)){int anim=j47_r_u8(r);PexNetRenderPlayerState*rp=pex_net_find_render_player(id);if(rp&&anim==0){rp->swing_active=1;rp->swing_ticks=0;}}break;}
        case 0x0C:j47_spawn_player(r);break;
        case 0x0D:{int32_t collected,collector;j47_r_varint(r,&collected);j47_r_varint(r,&collector);j47_entity_remove(collected);break;}
        case 0x0E:j47_spawn_object(r);break;
        case 0x0F:j47_spawn_mob(r);break;
        case 0x12:{int32_t id;if(j47_r_varint(r,&id)){J47Entity*e=j47_entity_find(id);int vx=(int16_t)j47_r_be16(r),vy=(int16_t)j47_r_be16(r),vz=(int16_t)j47_r_be16(r);if(e&&e->kind==J47_ENTITY_DROP&&e->drop_slot>=0){g_drops[e->drop_slot].mx=vx/8000.0f;g_drops[e->drop_slot].my=vy/8000.0f;g_drops[e->drop_slot].mz=vz/8000.0f;}}break;}
        case 0x13:{int32_t n;if(j47_r_varint(r,&n)&&n>=0&&n<100000)for(int i=0;i<n;i++){int32_t id;if(!j47_r_varint(r,&id))break;j47_entity_remove(id);}break;}
        case 0x14:case 0x15:case 0x16:case 0x17:{int32_t id;if(!j47_r_varint(r,&id))break;J47Entity*e=j47_entity_find(id);double x=e?e->x:0,y=e?e->y:0,z=e?e->z:0;float yaw=e?e->yaw:0,pitch=e?e->pitch:0;int rot=packet_id==0x16||packet_id==0x17;if(packet_id==0x15||packet_id==0x17){x+=(int8_t)j47_r_u8(r)/32.0;y+=(int8_t)j47_r_u8(r)/32.0;z+=(int8_t)j47_r_u8(r)/32.0;}if(rot){yaw=(int8_t)j47_r_u8(r)*360.0f/256.0f;pitch=(int8_t)j47_r_u8(r)*360.0f/256.0f;}(void)j47_r_u8(r);j47_apply_entity_position(e,x,y,z,yaw,pitch,rot);break;}
        case 0x18:{int32_t id;if(j47_r_varint(r,&id)){double x=j47_r_be32(r)/32.0,y=j47_r_be32(r)/32.0,z=j47_r_be32(r)/32.0;float yaw=(int8_t)j47_r_u8(r)*360.0f/256.0f,pitch=(int8_t)j47_r_u8(r)*360.0f/256.0f;(void)j47_r_u8(r);j47_apply_entity_position(j47_entity_find(id),x,y,z,yaw,pitch,1);}break;}
        case 0x19:{int32_t id;if(j47_r_varint(r,&id)){float head=(int8_t)j47_r_u8(r)*360.0f/256.0f;J47Entity*e=j47_entity_find(id);if(e){e->head_yaw=head;if(e->kind==J47_ENTITY_MOB&&e->mob_slot>=0){g_passive_mobs[e->mob_slot].prev_head_yaw=g_passive_mobs[e->mob_slot].head_yaw;g_passive_mobs[e->mob_slot].head_yaw=head;}}}break;}
        case 0x1A:{int32_t id;if(j47_r_varint(r,&id)){int status=(int8_t)j47_r_u8(r);J47Entity*e=j47_entity_find(id);if(e&&e->kind==J47_ENTITY_MOB&&e->mob_slot>=0){PassiveMob*m=&g_passive_mobs[e->mob_slot];if(status==2)m->hurt_time=10;if(status==3)m->death_time=1;}}break;}
        case 0x1C:{int32_t id;if(j47_r_varint(r,&id))j47_parse_entity_metadata(r,id);break;}
        case 0x1F:{g_player_xp_progress=j47_r_float(r);int32_t level,total;j47_r_varint(r,&level);j47_r_varint(r,&total);g_player_xp_level=level;g_player_xp_total=total;break;}
        case 0x21:{int cx=j47_r_be32(r),cz=j47_r_be32(r),full=j47_r_u8(r);unsigned mask=j47_r_be16(r);int32_t n;if(j47_r_varint(r,&n)&&n>=0&&(size_t)n<=r->len-r->pos)j47_install_chunk(cx,cz,full,mask,r->data+r->pos,(size_t)n,g_j47.dimension==0);break;}
        case 0x22:{int cx=j47_r_be32(r),cz=j47_r_be32(r);int32_t n;if(j47_r_varint(r,&n)&&n>=0&&n<100000)for(int i=0;i<n;i++){int packed=j47_r_be16(r);int32_t state;j47_r_varint(r,&state);int x=cx*16+((packed>>12)&15),z=cz*16+((packed>>8)&15),y=packed&255;j47_set_block_state(x,y,z,state);}break;}
        case 0x23:{int x=0,y=0,z=0;int32_t s;if(j47_read_block_pos(r,&x,&y,&z)&&j47_r_varint(r,&s))j47_set_block_state(x,y,z,s);break;}
        case 0x26:{int has_sky=j47_r_u8(r);int32_t n;if(!j47_r_varint(r,&n)||n<0||n>1024)break;int*cx=malloc((size_t)n*sizeof(int));int*cz=malloc((size_t)n*sizeof(int));unsigned short*mask=malloc((size_t)n*sizeof(unsigned short));if(!cx||!cz||!mask){free(cx);free(cz);free(mask);break;}for(int i=0;i<n;i++){cx[i]=j47_r_be32(r);cz[i]=j47_r_be32(r);mask[i]=j47_r_be16(r);}for(int i=0;i<n&&!r->failed;i++){size_t bytes=0;for(int sy=0;sy<16;sy++)if(mask[i]&(1u<<sy))bytes+=8192+2048+(has_sky?2048:0);bytes+=256;if(bytes>r->len-r->pos){r->failed=1;break;}j47_install_chunk(cx[i],cz[i],1,mask[i],r->data+r->pos,bytes,has_sky);r->pos+=bytes;}free(cx);free(cz);free(mask);break;}
        case 0x2B:{int reason=j47_r_u8(r);float v=j47_r_float(r);if(reason==3){g_game_mode=(int)v==1?1:0;g_pending_game_mode=g_game_mode;g_j47.game_mode=g_game_mode;if(!g_game_mode)g_creative_flying=0;g_j47.last_abilities_flags=-1;}break;}
        case 0x2D:{
            memset(&g_j47.window,0,sizeof(g_j47.window));
            g_j47.window.open=1;
            g_j47.window.window_id=j47_r_u8(r);
            j47_r_string(r,g_j47.window.type,sizeof(g_j47.window.type));
            char title_json[1024];
            j47_r_string(r,title_json,sizeof(title_json));
            j47_json_collect_text(title_json,g_j47.window.title,sizeof(g_j47.window.title));
            g_j47.window.container_slots=j47_r_u8(r);
            if(!strcmp(g_j47.window.type,"EntityHorse"))j47_r_be32(r);
            if(r->failed){memset(&g_j47.window,0,sizeof(g_j47.window));break;}

            if(!strcmp(g_j47.window.type,"minecraft:crafting_table")){
                memset(g_workbench_grid,0,sizeof(g_workbench_grid));
                set_screen(SCREEN_WORKBENCH);
            } else if(!strcmp(g_j47.window.type,"minecraft:furnace")){
                set_screen(SCREEN_FURNACE);
            } else {
                g_open_chest_rows = g_j47.window.container_slots > 27 ? 6 : 3;
                snprintf(g_open_chest_title,sizeof(g_open_chest_title),"%.*s",
                         (int)sizeof(g_open_chest_title)-1,
                         g_j47.window.title[0]?g_j47.window.title:"Container");
                set_screen(SCREEN_CHEST);
            }
            break;
        }
        case 0x2E:{int id=j47_r_u8(r);if(id==g_j47.window.window_id){memset(&g_j47.window,0,sizeof(g_j47.window));pex_net_java_server_closed_window();}break;}
        case 0x2F:{
            int wid=(int)(int8_t)j47_r_u8(r);
            int slot=(int16_t)j47_r_be16(r);
            ItemStack st;J47RawItem raw;
            if(j47_read_item_ex(r,&st,&raw)){
                if(wid==-1 && slot==-1){g_carried_stack=st;g_j47.raw_cursor=raw;}
                else if(wid==0 || wid==-2)j47_apply_player_slot_raw(slot,&st,&raw);
                else if(g_j47.window.open && wid==g_j47.window.window_id)j47_apply_open_window_slot_raw(slot,&st,&raw);
            }
            break;
        }
        case 0x30:{
            int wid=j47_r_u8(r);int count=(int16_t)j47_r_be16(r);
            if(count<0||count>4096)break;
            if(g_j47.window.open&&wid==g_j47.window.window_id)g_j47.window.total_slots=count<J47_WINDOW_MAX_SLOTS?count:J47_WINDOW_MAX_SLOTS;
            for(int i=0;i<count&&!r->failed;i++){
                ItemStack st;J47RawItem raw;
                if(!j47_read_item_ex(r,&st,&raw))break;
                if(wid==0)j47_apply_player_slot_raw(i,&st,&raw);
                else if(g_j47.window.open&&wid==g_j47.window.window_id)j47_apply_open_window_slot_raw(i,&st,&raw);
            }
            break;
        }
        case 0x31:{int wid=j47_r_u8(r);int property=(int16_t)j47_r_be16(r);int value=(int16_t)j47_r_be16(r);if(g_j47.window.open&&wid==g_j47.window.window_id&&!strcmp(g_j47.window.type,"minecraft:furnace")){FurnaceTile*ft=furnace_open_tile();if(ft){if(property==0)ft->burn_time=value;else if(property==1)ft->current_item_burn_time=value;else if(property==2)ft->cook_time=value;}}break;}
        case 0x32:{int wid=j47_r_u8(r);int action=(int16_t)j47_r_be16(r);int accepted=j47_r_u8(r);if(!accepted){J47Writer w;j47_writer_init(&w,8);j47_w_u8(&w,wid);j47_w_be16(&w,action);j47_w_u8(&w,1);j47_send_packet(0x0F,w.data,w.len);j47_writer_free(&w);}break;}
        case 0x38:j47_handle_player_list(r);break;
        case 0x39:{(void)j47_r_u8(r);(void)j47_r_float(r);(void)j47_r_float(r);break;}
        case 0x40:{char json[8192],text[512];if(j47_r_string(r,json,sizeof(json))){j47_json_collect_text(json,text,sizeof(text));j47_fail(text[0]?text:"Disconnected by Java server");}break;}
        case 0x41:g_j47.difficulty=j47_r_u8(r);break;
        case 0x46:{int32_t threshold;if(j47_r_varint(r,&threshold))g_j47.compression_threshold=threshold;break;}
        case 0x48:{char url[2048],hash[128];j47_r_string(r,url,sizeof(url));j47_r_string(r,hash,sizeof(hash));J47Writer w;j47_writer_init(&w,128);j47_w_string(&w,hash);j47_w_varint(&w,1);j47_send_packet(0x19,w.data,w.len);j47_writer_free(&w);break;}
        default: break;
    }
}

static void j47_handle_packet(const unsigned char *data,size_t len){
    J47Reader r;int32_t id=-1;j47_reader_init(&r,data,len);if(!j47_r_varint(&r,&id))return;
    if(g_j47.state==PEX_JAVA47_LOGIN){
        if(id==0){char json[8192],text[512];if(j47_r_string(&r,json,sizeof(json))){j47_json_collect_text(json,text,sizeof(text));j47_fail(text[0]?text:"Login rejected");}}
        else if(id==1){j47_fail("This Java server requires online-mode authentication");}
        else if(id==2){char uuid[64],name[64];if(j47_r_string(&r,uuid,sizeof(uuid))&&j47_r_string(&r,name,sizeof(name))){g_j47.state=PEX_JAVA47_PLAY;g_j47.progress=25;j47_set_status("Logging in");}}
        else if(id==3){int32_t threshold;if(j47_r_varint(&r,&threshold))g_j47.compression_threshold=threshold;}
    }else if(g_j47.state==PEX_JAVA47_PLAY)j47_handle_play_packet(id,&r);
    if(r.failed&&g_j47.state!=PEX_JAVA47_FAILED)j47_fail("Malformed Java protocol packet");
}

static int j47_process_rx(void){
    size_t consumed=0;
    while(consumed<g_j47.rx_len){int32_t frame_len=0;size_t prefix=0;int vr=j47_peek_varint(g_j47.rx+consumed,g_j47.rx_len-consumed,&frame_len,&prefix);if(vr==0)break;if(vr<0||frame_len<0||frame_len>(int)J47_PACKET_MAX){j47_fail("Invalid Java packet length");return 0;}if(g_j47.rx_len-consumed-prefix<(size_t)frame_len)break;
        const unsigned char*frame=g_j47.rx+consumed+prefix;size_t flen=(size_t)frame_len;
        if(g_j47.compression_threshold>=0){J47Reader rr;int32_t raw_len=0;j47_reader_init(&rr,frame,flen);if(!j47_r_varint(&rr,&raw_len)){j47_fail("Invalid compressed Java packet");return 0;}if(raw_len==0)j47_handle_packet(frame+rr.pos,flen-rr.pos);else{if(raw_len<g_j47.compression_threshold||raw_len>(int)J47_PACKET_MAX){j47_fail("Invalid Java decompressed length");return 0;}unsigned char*out=malloc((size_t)raw_len);if(!out){j47_fail("Out of memory decoding Java packet");return 0;}uLongf outlen=(uLongf)raw_len;int zr=uncompress(out,&outlen,frame+rr.pos,(uLong)(flen-rr.pos));if(zr!=Z_OK||outlen!=(uLongf)raw_len){free(out);j47_fail("Invalid Java compressed packet");return 0;}j47_handle_packet(out,(size_t)outlen);free(out);}}
        else j47_handle_packet(frame,flen);
        if(g_j47.state==PEX_JAVA47_FAILED) return 0;
        consumed += prefix + flen;
    }
    if(consumed){memmove(g_j47.rx,g_j47.rx+consumed,g_j47.rx_len-consumed);g_j47.rx_len-=consumed;}return 1;
}

int pex_java47_begin_join(const char *host,int port,const char *username){
    pex_java47_disconnect();
    memset(&g_j47,0,sizeof(g_j47));g_j47.socket=INVALID_SOCKET;g_j47.compression_threshold=-1;g_j47.server_protocol=PEX_JAVA47_PROTOCOL_VERSION;g_j47.state=PEX_JAVA47_CONNECTING;g_j47.active=1;g_j47.port=port>0?port:PEX_JAVA47_DEFAULT_PORT;g_j47.progress=5;g_j47.last_abilities_flags=-1;g_j47.last_movement_game_tick=-1;
    snprintf(g_j47.host,sizeof(g_j47.host),"%s",host?host:"");snprintf(g_j47.username,sizeof(g_j47.username),"%.16s",username&&username[0]?username:"Player");j47_set_status("Connecting to Java server");
    g_j47.socket=j47_connect_timeout(g_j47.host,g_j47.port,5000);if(g_j47.socket==INVALID_SOCKET){j47_fail("Could not connect to Java server");return 0;}
    J47Writer hs; j47_writer_init(&hs,300);j47_w_varint(&hs,PEX_JAVA47_PROTOCOL_VERSION);j47_w_string_n(&hs,g_j47.host,255);j47_w_be16(&hs,g_j47.port);j47_w_varint(&hs,2);
    if(!j47_send_packet(0x00,hs.data,hs.len)){j47_writer_free(&hs);j47_fail("Could not send Java handshake");return 0;}j47_writer_free(&hs);
    J47Writer login;j47_writer_init(&login,32);j47_w_string_n(&login,g_j47.username,16);if(!j47_send_packet(0x00,login.data,login.len)){j47_writer_free(&login);j47_fail("Could not send Java login");return 0;}j47_writer_free(&login);
    g_j47.state=PEX_JAVA47_LOGIN;g_j47.progress=15;j47_set_status("Logging in");return 1;
}

int pex_java47_tick(void){
    if(!g_j47.active&&g_j47.state!=PEX_JAVA47_PLAY)return g_j47.state!=PEX_JAVA47_FAILED;
    if(g_j47.socket==INVALID_SOCKET)return 0;
    if(!j47_flush_tx())return 0;
    if(!j47_receive_available())return 0;
    if(!j47_process_rx())return 0;
    if(g_j47.peer_closed){
        if(g_j47.rx_len) j47_fail("Java server closed with a truncated packet");
        else j47_fail("Java server closed the connection");
        return 0;
    }
    return j47_flush_tx();
}

void pex_java47_disconnect(void){
    if(g_j47.socket!=INVALID_SOCKET)closesocket(g_j47.socket);
    j47_clear_remote_entities();
    free(g_j47.rx);free(g_j47.tx);memset(&g_j47,0,sizeof(g_j47));g_j47.socket=INVALID_SOCKET;g_j47.compression_threshold=-1;g_j47.state=PEX_JAVA47_IDLE;
}

int pex_java47_is_active(void){return g_j47.active||g_j47.state==PEX_JAVA47_PLAY;}
int pex_java47_is_playing(void){return g_j47.state==PEX_JAVA47_PLAY;}
int pex_java47_has_join_game(void){return g_j47.has_join_game;}
int pex_java47_world_ready(void){return g_j47.world_ready;}
int pex_java47_progress(void){return g_j47.progress;}
const char*pex_java47_status_text(void){return g_j47.status;}
const char*pex_java47_disconnect_reason(void){return g_j47.disconnect_reason;}
int pex_java47_server_protocol(void){return g_j47.server_protocol;}
int pex_java47_local_entity_id(void){return g_j47.local_entity_id;}
int pex_java47_get_equipment(int entity_id,int *item_id,int *count,int *damage,int *slot){J47Entity*e=j47_entity_find(entity_id);if(!e)return 0;if(item_id)*item_id=e->held_item_id;if(count)*count=e->held_item_count;if(damage)*damage=e->held_item_damage;if(slot)*slot=e->held_slot;return 1;}
int pex_java47_entity_is_player(int entity_id){J47Entity*e=j47_entity_find(entity_id);return e&&e->kind==J47_ENTITY_PLAYER;}

ItemStack *pex_java47_get_open_container_slot(int local_slot){
    if(!g_j47.window.open||!j47_window_is_container_type(g_j47.window.type)||local_slot<0||
       local_slot>=g_j47.window.container_slots||local_slot>=J47_WINDOW_MAX_SLOTS)return NULL;
    return &g_j47.window.slots[local_slot];
}

int pex_java47_open_container_slot_count(void){
    if(!g_j47.window.open||!j47_window_is_container_type(g_j47.window.type))return 0;
    int n=g_j47.window.container_slots;
    if(n<0)n=0;
    if(n>J47_WINDOW_MAX_SLOTS)n=J47_WINDOW_MAX_SLOTS;
    return n;
}

static int j47_send_entity_action(int action){J47Writer w;j47_writer_init(&w,16);j47_w_varint(&w,g_j47.local_entity_id);j47_w_varint(&w,action);j47_w_varint(&w,0);int ok=j47_send_packet(0x0B,w.data,w.len);j47_writer_free(&w);return ok;}

int pex_java47_send_player_state(double x,double feet_y,double z,float yaw,float pitch,int on_ground,int sneaking,int sprinting,int held_slot){
    if(g_j47.state!=PEX_JAVA47_PLAY||!g_j47.position_received)return 0;
    if(!isfinite(x)||!isfinite(feet_y)||!isfinite(z)||!isfinite(yaw)||!isfinite(pitch))return 1;
    flat_multiplayer_recenter_world((float)x,(float)z,0);
    if(held_slot<0||held_slot>8) held_slot=0;
    if(g_j47.last_held_slot!=held_slot){J47Writer h;j47_writer_init(&h,4);j47_w_be16(&h,held_slot);j47_send_packet(0x09,h.data,h.len);j47_writer_free(&h);g_j47.last_held_slot=held_slot;}
    if(g_j47.last_sneaking!=sneaking){j47_send_entity_action(sneaking?0:1);g_j47.last_sneaking=sneaking;}
    if(g_j47.last_sprinting!=sprinting){j47_send_entity_action(sprinting?3:4);g_j47.last_sprinting=sprinting;}
    int ability_flags=0;if(g_j47.game_mode==1){ability_flags=1|4|8;if(g_creative_flying)ability_flags|=2;}
    if(g_j47.last_abilities_flags!=ability_flags){J47Writer a;j47_writer_init(&a,16);j47_w_u8(&a,ability_flags);j47_w_float(&a,0.05f);j47_w_float(&a,0.1f);j47_send_packet(0x13,a.data,a.len);j47_writer_free(&a);g_j47.last_abilities_flags=ability_flags;}

    /* EntityPlayerSP sends one movement packet per client tick. The previous
       wall-clock limiter could emit irregular bursts when the simulation and
       render threads called this path in the same frame, which looks like
       impossible movement to stricter servers. */
    if(g_j47.last_movement_game_tick==g_ingame_ticks)return 1;
    g_j47.last_movement_game_tick=g_ingame_ticks;

    double dx=x-g_j47.last_x,dy=feet_y-g_j47.last_y,dz=z-g_j47.last_z;
    double dist2=dx*dx+dy*dy+dz*dz;
    int moved=dist2>9.0e-4||g_j47.movement_ticks>=20;
    int looked=(yaw!=g_j47.last_yaw)||(pitch!=g_j47.last_pitch);
    J47Writer w;j47_writer_init(&w,48);int pid=0x03;
    on_ground=on_ground?1:0;
    if(moved&&looked){pid=0x06;j47_w_double(&w,x);j47_w_double(&w,feet_y);j47_w_double(&w,z);j47_w_float(&w,yaw);j47_w_float(&w,pitch);j47_w_u8(&w,on_ground);}
    else if(moved){pid=0x04;j47_w_double(&w,x);j47_w_double(&w,feet_y);j47_w_double(&w,z);j47_w_u8(&w,on_ground);}
    else if(looked){pid=0x05;j47_w_float(&w,yaw);j47_w_float(&w,pitch);j47_w_u8(&w,on_ground);}
    else {pid=0x03;j47_w_u8(&w,on_ground);}
    int ok=j47_send_packet(pid,w.data,w.len);j47_writer_free(&w);
    g_j47.movement_ticks++;
    if(moved){g_j47.last_x=x;g_j47.last_y=feet_y;g_j47.last_z=z;g_j47.movement_ticks=0;}
    if(looked){g_j47.last_yaw=yaw;g_j47.last_pitch=pitch;}
    g_j47.last_on_ground=on_ground;
    return ok;
}

int pex_java47_send_chat(const char*text){if(g_j47.state!=PEX_JAVA47_PLAY||!text)return 0;J47Writer w;j47_writer_init(&w,300);j47_w_string_n(&w,text,100);int ok=j47_send_packet(0x01,w.data,w.len);j47_writer_free(&w);return ok;}
int pex_java47_send_swing(void){if(g_j47.state!=PEX_JAVA47_PLAY)return 0;return j47_send_packet(0x0A,NULL,0);}
int pex_java47_send_attack(int entity_id){if(g_j47.state!=PEX_JAVA47_PLAY)return 0;J47Writer w;j47_writer_init(&w,16);j47_w_varint(&w,entity_id);j47_w_varint(&w,1);int ok=j47_send_packet(0x02,w.data,w.len);j47_writer_free(&w);return ok;}

int pex_java47_send_interact(int entity_id,float hit_x,float hit_y,float hit_z){
    if(g_j47.state!=PEX_JAVA47_PLAY)return 0;
    J47Entity *entity=j47_entity_find(entity_id);
    J47Writer w;j47_writer_init(&w,32);j47_w_varint(&w,entity_id);
    if(entity&&entity->is_armor_stand){
        /* Vanilla 1.8.8 handles armor stands through INTERACT_AT. Sending a
           second generic interaction can make plugin menus fire twice. */
        j47_w_varint(&w,2);
        j47_w_float(&w,hit_x);j47_w_float(&w,hit_y);j47_w_float(&w,hit_z);
    }else{
        /* Villagers, fake-player NPCs and other ordinary entities use the
           generic right-click action. */
        j47_w_varint(&w,0);
    }
    int ok=j47_send_packet(0x02,w.data,w.len);j47_writer_free(&w);return ok;
}
int pex_java47_send_dig(int status,int x,int y,int z,int face){if(g_j47.state!=PEX_JAVA47_PLAY)return 0;J47Writer w;j47_writer_init(&w,24);j47_w_varint(&w,status);j47_write_block_pos(&w,x,y,z);j47_w_u8(&w,face);int ok=j47_send_packet(0x07,w.data,w.len);j47_writer_free(&w);return ok;}
int pex_java47_send_place(int x,int y,int z,int face,int item_id,int count,int damage,float cursor_x,float cursor_y,float cursor_z){if(g_j47.state!=PEX_JAVA47_PLAY)return 0;J47Writer w;j47_writer_init(&w,48);j47_write_block_pos(&w,x,y,z);j47_w_u8(&w,face);j47_write_held_or_local_item(&w,item_id,count,damage);int cx=(int)(cursor_x*16.0f);int cy=(int)(cursor_y*16.0f);int cz=(int)(cursor_z*16.0f);if(cx<0)cx=0;if(cx>15)cx=15;if(cy<0)cy=0;if(cy>15)cy=15;if(cz<0)cz=0;if(cz>15)cz=15;j47_w_u8(&w,cx);j47_w_u8(&w,cy);j47_w_u8(&w,cz);int ok=j47_send_packet(0x08,w.data,w.len);j47_writer_free(&w);return ok;}
int pex_java47_send_drop(int whole_stack){return pex_java47_send_dig(whole_stack?3:4,0,0,0,0);}
int pex_java47_send_use_item(int item_id,int count,int damage){return pex_java47_send_place(-1,-1,-1,255,item_id,count,damage,0.0f,0.0f,0.0f);}
int pex_java47_send_release_use_item(void){return pex_java47_send_dig(5,0,0,0,0);}
int pex_java47_send_respawn(void){if(g_j47.state!=PEX_JAVA47_PLAY)return 0;J47Writer w;j47_writer_init(&w,4);j47_w_varint(&w,0);int ok=j47_send_packet(0x16,w.data,w.len);j47_writer_free(&w);return ok;}
int pex_java47_send_close_window(void){if(g_j47.state!=PEX_JAVA47_PLAY||!g_j47.window.open)return 0;J47Writer w;j47_writer_init(&w,4);j47_w_u8(&w,g_j47.window.window_id);int ok=j47_send_packet(0x0D,w.data,w.len);j47_writer_free(&w);memset(&g_j47.window,0,sizeof(g_j47.window));return ok;}

int pex_java47_send_window_click(int pex_slot,int button,int mode,const ItemStack *clicked_result){
    if(g_j47.state!=PEX_JAVA47_PLAY)return 0;
    int java_slot=j47_map_pex_slot_to_java(pex_slot);
    if(java_slot==-32768)return 0;
    int window_id=g_j47.window.open?g_j47.window.window_id:0;
    short action=++g_j47.window.action_number;
    J47Writer w;j47_writer_init(&w,48);
    j47_w_u8(&w,window_id);
    j47_w_be16(&w,java_slot);
    j47_w_u8(&w,button);
    j47_w_be16(&w,action);
    j47_w_u8(&w,mode);
    if(clicked_result&&!stack_empty(clicked_result)){
        const J47RawItem *raw=j47_raw_for_java_slot(java_slot);
        if(raw)j47_write_raw_item(&w,raw,clicked_result->count);
        else j47_write_item(&w,clicked_result->id,clicked_result->count,clicked_result->damage);
    } else j47_write_item(&w,0,0,0);
    int ok=j47_send_packet(0x0E,w.data,w.len);j47_writer_free(&w);return ok;
}

int pex_java47_send_creative_slot(int pex_slot,const ItemStack *stack){
    if(g_j47.state!=PEX_JAVA47_PLAY||g_j47.game_mode!=1)return 0;
    int java_slot=j47_map_pex_slot_to_java(pex_slot);
    if(java_slot==-32768)return 0;
    J47Writer w;j47_writer_init(&w,40);j47_w_be16(&w,java_slot);
    if(stack&&!stack_empty(stack))j47_write_item(&w,stack->id,stack->count,stack->damage);else j47_write_item(&w,0,0,0);
    int ok=j47_send_packet(0x10,w.data,w.len);j47_writer_free(&w);return ok;
}

int pex_java47_sync_inventory_from_game(void){return g_j47.state==PEX_JAVA47_PLAY;}

int pex_java47_try_attack_mob(float max_dist){
    if(g_j47.state!=PEX_JAVA47_PLAY) return 0;
    float lx,ly,lz;
    pex_touch_aware_look_vector(&lx,&ly,&lz);
    float best=max_dist;
    int best_id=0;
    for(int i=0;i<J47_ENTITY_MAX;i++){J47Entity*e=&g_j47.entities[i];if(!e->used||e->kind!=J47_ENTITY_MOB||e->mob_slot<0)continue;PassiveMob*m=&g_passive_mobs[e->mob_slot];if(!m->active)continue;float mn[3]={m->x-m->width*.5f,m->y,m->z-m->width*.5f},mx[3]={m->x+m->width*.5f,m->y+m->height,m->z+m->width*.5f},o[3]={g_player_x,g_player_y,g_player_z},d[3]={lx,ly,lz};float tmin=0,tmax=best;int hit=1;for(int a=0;a<3;a++){if(fabsf(d[a])<1e-5f){if(o[a]<mn[a]||o[a]>mx[a]){hit=0;break;}}else{float inv=1/d[a],t1=(mn[a]-o[a])*inv,t2=(mx[a]-o[a])*inv;if(t1>t2){float q=t1;t1=t2;t2=q;}if(t1>tmin)tmin=t1;if(t2<tmax)tmax=t2;if(tmin>tmax){hit=0;break;}}}if(hit&&tmin>=0&&tmin<best){best=tmin;best_id=e->entity_id;}}
    if(best_id){pex_java47_send_swing();pex_java47_send_attack(best_id);restart_hand_swing();return 1;}return 0;
}


static int j47_ray_aabb(const float origin[3], const float dir[3], const float mn[3], const float mx[3],
                        float max_t, float *out_t) {
    float tmin = 0.0f, tmax = max_t;
    for (int axis = 0; axis < 3; axis++) {
        if (fabsf(dir[axis]) < 0.00001f) {
            if (origin[axis] < mn[axis] || origin[axis] > mx[axis]) return 0;
        } else {
            float inv = 1.0f / dir[axis];
            float t1 = (mn[axis] - origin[axis]) * inv;
            float t2 = (mx[axis] - origin[axis]) * inv;
            if (t1 > t2) { float tmp=t1; t1=t2; t2=tmp; }
            if (t1 > tmin) tmin=t1;
            if (t2 < tmax) tmax=t2;
            if (tmin > tmax) return 0;
        }
    }
    if (tmin < 0.0f || tmin > max_t) return 0;
    if (out_t) *out_t=tmin;
    return 1;
}

int pex_java47_try_interact_entity(float max_dist) {
    if(g_j47.state!=PEX_JAVA47_PLAY||g_player_dead)return 0;
    if(max_dist<=0.0f)max_dist=player_is_creative()?5.0f:4.0f;

    FlatRayHit block=flat_raycast();
    if(block.hit){
        float dx=block.hx-g_player_x,dy=block.hy-g_player_y,dz=block.hz-g_player_z;
        float block_dist=sqrtf(dx*dx+dy*dy+dz*dz);
        if(block_dist<max_dist)max_dist=block_dist;
    }

    float look_x,look_y,look_z;
    pex_touch_aware_look_vector(&look_x,&look_y,&look_z);
    float origin[3]={g_player_x,g_player_y,g_player_z};
    float dir[3]={look_x,look_y,look_z};
    float best=max_dist;
    J47Entity *best_entity=NULL;

    for(int i=0;i<J47_ENTITY_MAX;i++){
        J47Entity *e=&g_j47.entities[i];
        if(!e->used||e->entity_id==g_j47.local_entity_id||e->kind==J47_ENTITY_DROP)continue;
        float width=0.7f,height=1.8f,feet_y=(float)e->y;
        if(e->kind==J47_ENTITY_PLAYER){width=0.7f;height=1.8f;}
        else if(e->kind==J47_ENTITY_MOB&&e->mob_slot>=0&&e->mob_slot<MAX_PASSIVE_MOBS){
            PassiveMob *m=&g_passive_mobs[e->mob_slot];
            if(!m->active||m->death_time>0)continue;
            if(e->is_armor_stand){
                /* Keep the real armor-stand interaction volume rather than the
                   shorter pig renderer, so a helmet/menu icon can be clicked. */
                width=0.65f;height=2.0f;feet_y=(float)e->y;
            }else{
                width=m->width>0.1f?m->width:0.9f;
                height=m->height>0.1f?m->height:0.9f;
                feet_y=m->y;
            }
        }
        float half=width*0.5f;
        float mn[3]={(float)e->x-half,feet_y,(float)e->z-half};
        float mx[3]={(float)e->x+half,feet_y+height,(float)e->z+half};
        float t=best;
        if(j47_ray_aabb(origin,dir,mn,mx,best,&t)&&t<best){best=t;best_entity=e;}
    }

    if(!best_entity)return 0;
    float hit_x=origin[0]+dir[0]*best-(float)best_entity->x;
    float hit_y=origin[1]+dir[1]*best-(float)best_entity->y;
    float hit_z=origin[2]+dir[2]*best-(float)best_entity->z;
    pex_java47_send_swing();
    pex_java47_send_interact(best_entity->entity_id,hit_x,hit_y,hit_z);
    restart_hand_swing();
    return 1;
}
