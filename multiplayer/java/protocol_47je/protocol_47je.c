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
#define J47_SKIN_URL_MAX 768
#define J47_TEAM_MAX 64
#define J47_TEAM_MEMBER_MAX 128
#define J47_SCORE_OBJECTIVE_MAX 32
#define J47_SCORE_ENTRY_MAX 512
#define J47_BLOCK_UPDATE_CACHE_MAX 8192


/* The Java adapter is unity-included after game/mobs.c and before
   platform/multiplayer_client.c. These declarations bind the protocol layer to
   the existing PexCraft multiplayer presentation helpers without duplicating
   gameplay systems. */
static void pex_net_mark_chunk_ready(int chunk_x, int chunk_z);
static PexNetRenderPlayerState *pex_net_find_render_player(int player_id);
static void pex_net_java_server_closed_window(void);
static void pex_net_apply_skin(const PexNetSkin *skin, uint32_t size);

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
    int listed;
    unsigned char uuid[16];
    char name[32];
    char display_name[192];
    int game_mode;
    int ping;
    char skin_url[J47_SKIN_URL_MAX];
    char skin_path[MAX_PATHBUF];
    int skin_state; /* 0 none, 1 queued, 2 downloading, 3 ready, 4 failed */
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
    J47_ENTITY_ARMOR_STAND,
    J47_ENTITY_OTHER
} J47EntityKind;

typedef struct J47Entity {
    int used;
    int entity_id;
    unsigned char uuid[16];
    J47EntityKind kind;
    int java_type;
    int player_slot;
    int mob_slot;
    int drop_slot;
    int projectile_slot;
    float projectile_ax, projectile_ay, projectile_az;
    int held_item_id;
    int held_item_count;
    int held_item_damage;
    int held_slot;
    int is_armor_stand;
    int metadata_flags;
    int custom_name_visible;
    int armor_stand_flags;
    int proxy_item_drop_slot;
    ItemStack equipment[5];
    char custom_name[96];
    float armor_pose[6][3]; /* head, body, left arm, right arm, left leg, right leg */
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

typedef struct J47Team {
    int used;
    char name[17];
    char display_name[65];
    char prefix[65];
    char suffix[65];
    char name_tag_visibility[24];
    int friendly_flags;
    int color;
    char members[J47_TEAM_MEMBER_MAX][41];
    int member_count;
} J47Team;

typedef struct J47ScoreObjective {
    int used;
    char name[17];
    char display_name[65];
    int render_hearts;
} J47ScoreObjective;

typedef struct J47ScoreEntry {
    int used;
    char owner[41];
    char objective[17];
    int value;
} J47ScoreEntry;

typedef struct J47BlockUpdate {
    int used;
    int x, y, z;
    int state;
    unsigned int serial;
} J47BlockUpdate;

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
    int daylight_cycle;
    int local_entity_id;
    unsigned char local_profile_uuid[16];
    int local_profile_uuid_valid;
    int chunks_received;
    int position_received;
    int progress;
    int held_slot;
    int last_held_slot;
    int last_sneaking;
    int last_sprinting;
    int last_abilities_flags;
    int server_abilities_flags;
    float server_fly_speed;
    float server_walk_speed;
    float server_movement_speed_scale;
    int movement_ticks;
    int last_movement_game_tick;
    int last_on_ground;
    int pending_respawn_dimension;
    int pending_respawn_from;
    int pending_respawn_to;
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
    unsigned int packet_serial;
    int last_world_origin_x;
    int last_world_origin_z;
    char tab_header[512];
    char tab_footer[512];
    char display_objectives[19][17];
    J47Team teams[J47_TEAM_MAX];
    J47ScoreObjective objectives[J47_SCORE_OBJECTIVE_MAX];
    J47ScoreEntry scores[J47_SCORE_ENTRY_MAX];
    J47BlockUpdate block_updates[J47_BLOCK_UPDATE_CACHE_MAX];
    int block_update_cursor;
    J47Profile profiles[J47_PROFILE_MAX];
    J47Entity entities[J47_ENTITY_MAX];
    J47RawItem raw_player_slots[46];
    J47RawItem raw_cursor;
    J47Window window;
} PexJava47Session;

static PexJava47Session g_j47;
static CRITICAL_SECTION g_j47_tx_cs;
static int g_j47_tx_cs_ready = 0;

static void j47_parse_entity_metadata(J47Reader *r, int entity_id);
static void multiplayer_name_tag_view_angles(float *out_yaw, float *out_pitch);

typedef struct J47SkinDownloadJob {
    unsigned char uuid[16];
    char url[J47_SKIN_URL_MAX];
    char path[MAX_PATHBUF];
    volatile LONG done;
    volatile LONG ok;
} J47SkinDownloadJob;

static J47SkinDownloadJob g_j47_skin_job;
static HANDLE g_j47_skin_thread = NULL;
static Texture g_j47_local_skin;

static int j47_socket_would_block(void) {
    int e = WSAGetLastError();
    return e == WSAEWOULDBLOCK || e == WSAEINPROGRESS || e == WSAEALREADY;
}

static int j47_socket_peer_closed_error(void) {
    int e = WSAGetLastError();
#ifdef WSAECONNRESET
    if (e == WSAECONNRESET) return 1;
#endif
#ifdef WSAECONNABORTED
    if (e == WSAECONNABORTED) return 1;
#endif
#ifdef WSAENOTCONN
    if (e == WSAENOTCONN) return 1;
#endif
#ifdef WSAESHUTDOWN
    if (e == WSAESHUTDOWN) return 1;
#endif
    return 0;
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

static int j47_base64_value(int c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static int j47_base64_decode(const char *src, unsigned char *out, size_t cap, size_t *out_len) {
    size_t n = 0;
    uint32_t accum = 0;
    int bits = 0;
    if (out_len) *out_len = 0;
    if (!src || !out || cap == 0) return 0;
    for (; *src; ++src) {
        unsigned char ch = (unsigned char)*src;
        if (ch == '=') break;
        if (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t') continue;
        int v = j47_base64_value(ch);
        if (v < 0) return 0;
        accum = (accum << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            if (n >= cap) return 0;
            out[n++] = (unsigned char)((accum >> bits) & 0xff);
        }
    }
    if (n < cap) out[n] = 0;
    if (out_len) *out_len = n;
    return n > 0;
}

static int j47_extract_skin_url(const char *textures_value, char *out, size_t cap) {
    unsigned char decoded[8192];
    size_t decoded_len = 0;
    if (!out || cap == 0) return 0;
    out[0] = 0;
    if (!j47_base64_decode(textures_value, decoded, sizeof(decoded) - 1, &decoded_len)) return 0;
    decoded[decoded_len] = 0;
    const char *json = (const char *)decoded;
    const char *skin = strstr(json, "\"SKIN\"");
    if (!skin) return 0;
    const char *url_key = strstr(skin, "\"url\"");
    if (!url_key) return 0;
    const char *colon = strchr(url_key + 5, ':');
    if (!colon) return 0;
    const char *p = j47_json_ws(colon + 1, json + decoded_len);
    const char *after = p;
    if (p >= json + decoded_len || *p != '"') return 0;
    if (!j47_json_unescape(p, json + decoded_len, out, cap, &after)) return 0;
    return !strncmp(out, "http://", 7) || !strncmp(out, "https://", 8);
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

static void j47_tx_lock_ensure(void) {
    if (!g_j47_tx_cs_ready) {
        InitializeCriticalSection(&g_j47_tx_cs);
        g_j47_tx_cs_ready = 1;
    }
}

static int j47_queue_bytes_unlocked(const void *data, size_t len) {
    if (!data || len == 0) return 1;

    /* Keep the transmit buffer as an ordered byte stream, like Netty's
       outbound queue.  When a nonblocking send consumed only a prefix, move
       the remaining bytes to the front before appending more packets.  This
       prevents harmless partial writes from making the buffer look full and,
       importantly, lets us retain every 20 Hz movement packet instead of
       replacing intermediate positions. */
    if (g_j47.tx_pos > 0) {
        if (g_j47.tx_pos >= g_j47.tx_len) {
            g_j47.tx_pos = 0;
            g_j47.tx_len = 0;
        } else if (g_j47.tx_pos >= 4096 || g_j47.tx_len > J47_TX_MAX - len) {
            size_t remaining = g_j47.tx_len - g_j47.tx_pos;
            memmove(g_j47.tx, g_j47.tx + g_j47.tx_pos, remaining);
            g_j47.tx_len = remaining;
            g_j47.tx_pos = 0;
        }
    }

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
    j47_tx_lock_ensure();
    EnterCriticalSection(&g_j47_tx_cs);
    while (g_j47.tx_pos < g_j47.tx_len) {
        int n = send(g_j47.socket, (const char *)g_j47.tx + g_j47.tx_pos,
                     (int)(g_j47.tx_len - g_j47.tx_pos), 0);
        if (n > 0) { g_j47.tx_pos += (size_t)n; continue; }
        if (n == 0) { g_j47.peer_closed = 1; LeaveCriticalSection(&g_j47_tx_cs); return 1; }
        if (j47_socket_would_block()) { LeaveCriticalSection(&g_j47_tx_cs); return 1; }
        LeaveCriticalSection(&g_j47_tx_cs);
        j47_fail("Could not send Java protocol data");
        return 0;
    }
    g_j47.tx_len = 0;
    g_j47.tx_pos = 0;
    LeaveCriticalSection(&g_j47_tx_cs);
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
    int ok = 0;
    if (!frame.failed) {
        j47_tx_lock_ensure();
        EnterCriticalSection(&g_j47_tx_cs);
        ok = j47_queue_bytes_unlocked(frame.data, frame.len);
        LeaveCriticalSection(&g_j47_tx_cs);
    }
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
        if (n == 0) {
            /* Java servers normally write S40/Login Disconnect and then close
               immediately.  Keep the bytes already read so the disconnect JSON
               is decoded before the transport close is reported. */
            g_j47.peer_closed = 1;
            return 1;
        }
        if (j47_socket_would_block()) return 1;
        if (j47_socket_peer_closed_error()) {
            /* Some platforms surface the same orderly server shutdown as
               ECONNRESET/WSAECONNRESET instead of recv()==0. */
            g_j47.peer_closed = 1;
            return 1;
        }
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
            case 171: mapped_id = BLOCK_SNOW_LAYER; mapped_meta = 15; break;
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
        case 65: return PASSIVE_MOB_CHICKEN;   /* bat: small existing flyer proxy */
        case 66: return PASSIVE_MOB_VILLAGER;  /* witch: closest humanoid NPC */
        case 67: return PASSIVE_MOB_SILVERFISH;/* endermite */
        case 68: return PASSIVE_MOB_SQUID;     /* guardian */
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
        case 100: return PASSIVE_MOB_PIG;      /* horse */
        case 101: return PASSIVE_MOB_CHICKEN;  /* rabbit */
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
        g_j47.entities[i].projectile_slot = -1;
        g_j47.entities[i].proxy_item_drop_slot = -1;
        return &g_j47.entities[i];
    }
    return NULL;
}

static void j47_entity_clear_visuals(J47Entity *e) {
    if (!e || !e->used) return;
    if (e->kind == J47_ENTITY_PLAYER && e->player_slot >= 0 && e->player_slot < PEX_NET_MAX_PLAYERS) {
        int slot = e->player_slot;
        for (int i = slot; i + 1 < g_mp_player_count; i++) {
            g_mp_players[i] = g_mp_players[i + 1];
            g_mp_prev_players[i] = g_mp_prev_players[i + 1];
            for (int k = 0; k < J47_ENTITY_MAX; k++)
                if (g_j47.entities[k].used && &g_j47.entities[k] != e && g_j47.entities[k].player_slot == i + 1)
                    g_j47.entities[k].player_slot = i;
        }
        if (g_mp_player_count > 0) g_mp_player_count--;
    } else if (e->kind == J47_ENTITY_MOB && e->mob_slot >= 0 && e->mob_slot < MAX_PASSIVE_MOBS) {
        g_passive_mobs[e->mob_slot].active = 0;
    } else if (e->kind == J47_ENTITY_DROP && e->drop_slot >= 0 && e->drop_slot < MAX_DROP_ENTITIES) {
        g_drops[e->drop_slot].active = 0;
    }
    if (e->projectile_slot >= 0 && e->projectile_slot < MAX_PROJECTILE_ENTITIES)
        g_projectiles[e->projectile_slot].active = 0;
    if (e->proxy_item_drop_slot >= 0 && e->proxy_item_drop_slot < MAX_DROP_ENTITIES)
        g_drops[e->proxy_item_drop_slot].active = 0;
    e->kind = J47_ENTITY_NONE;
    e->player_slot = -1;
    e->mob_slot = -1;
    e->drop_slot = -1;
    e->projectile_slot = -1;
    e->proxy_item_drop_slot = -1;
    e->is_armor_stand = 0;
    memset(e->equipment, 0, sizeof(e->equipment));
}

static J47Entity *j47_entity_prepare_spawn(int entity_id) {
    J47Entity *e = j47_entity_alloc(entity_id);
    if (!e) return NULL;
    j47_entity_clear_visuals(e);
    {
        int used = e->used;
        int id = e->entity_id;
        memset(e, 0, sizeof(*e));
        e->used = used;
        e->entity_id = id;
        e->player_slot = -1;
        e->mob_slot = -1;
        e->drop_slot = -1;
        e->projectile_slot = -1;
        e->proxy_item_drop_slot = -1;
    }
    return e;
}

static void j47_entity_remove(int entity_id) {
    J47Entity *e = j47_entity_find(entity_id);
    if (!e) return;
    j47_entity_clear_visuals(e);
    memset(e, 0, sizeof(*e));
}

static void j47_clear_remote_entities(void) {
    for (int i = 0; i < J47_ENTITY_MAX; i++) {
        if (g_j47.entities[i].used) j47_entity_remove(g_j47.entities[i].entity_id);
    }
    g_mp_player_count = 0;
}

static void j47_reset_world_for_respawn_ex(int preserve_position) {
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
    memset(g_j47.block_updates, 0, sizeof(g_j47.block_updates));
    g_j47.block_update_cursor = 0;
    g_j47.chunks_received = 0;
    g_j47.world_ready = 0;
    if (!preserve_position) g_j47.position_received = 0;
}

static void j47_reset_world_for_respawn(void) {
    j47_reset_world_for_respawn_ex(0);
}

static void j47_commit_pending_respawn_dimension(void) {
    if (!g_j47.pending_respawn_dimension) return;
    g_j47.dimension = g_j47.pending_respawn_to;
    g_j47.pending_respawn_dimension = 0;
    /* S08 commonly arrives before the first chunk. Keep that acknowledged
       position while replacing the old dimension's terrain. */
    j47_reset_world_for_respawn_ex(g_j47.position_received ? 1 : 0);
}

static J47Team *j47_team_find(const char *name) {
    if (!name || !name[0]) return NULL;
    for (int i = 0; i < J47_TEAM_MAX; ++i)
        if (g_j47.teams[i].used && strcmp(g_j47.teams[i].name, name) == 0) return &g_j47.teams[i];
    return NULL;
}

static J47Team *j47_team_put(const char *name) {
    J47Team *t = j47_team_find(name);
    if (t) return t;
    for (int i = 0; i < J47_TEAM_MAX; ++i) {
        if (!g_j47.teams[i].used) {
            t = &g_j47.teams[i];
            memset(t, 0, sizeof(*t));
            t->used = 1;
            t->color = -1;
            snprintf(t->name_tag_visibility, sizeof(t->name_tag_visibility), "always");
            snprintf(t->name, sizeof(t->name), "%.*s", 16, name ? name : "");
            return t;
        }
    }
    return NULL;
}

static void j47_team_remove_member_from_all(const char *member) {
    if (!member || !member[0]) return;
    for (int ti = 0; ti < J47_TEAM_MAX; ++ti) {
        J47Team *t = &g_j47.teams[ti];
        if (!t->used) continue;
        for (int i = 0; i < t->member_count; ) {
            if (strcmp(t->members[i], member) == 0) {
                for (int j = i; j + 1 < t->member_count; ++j)
                    memcpy(t->members[j], t->members[j + 1], sizeof(t->members[j]));
                --t->member_count;
            } else ++i;
        }
    }
}

static void j47_team_add_member(J47Team *t, const char *member) {
    if (!t || !member || !member[0]) return;
    j47_team_remove_member_from_all(member);
    if (t->member_count >= J47_TEAM_MEMBER_MAX) return;
    snprintf(t->members[t->member_count++], sizeof(t->members[0]), "%.*s", 40, member);
}

static void j47_team_remove_member(J47Team *t, const char *member) {
    if (!t || !member) return;
    for (int i = 0; i < t->member_count; ++i) {
        if (strcmp(t->members[i], member) != 0) continue;
        for (int j = i; j + 1 < t->member_count; ++j)
            memcpy(t->members[j], t->members[j + 1], sizeof(t->members[j]));
        --t->member_count;
        return;
    }
}

static J47Team *j47_team_for_member(const char *member) {
    if (!member || !member[0]) return NULL;
    for (int ti = 0; ti < J47_TEAM_MAX; ++ti) {
        J47Team *t = &g_j47.teams[ti];
        if (!t->used) continue;
        for (int i = 0; i < t->member_count; ++i)
            if (strcmp(t->members[i], member) == 0) return t;
    }
    return NULL;
}

int pex_java47_player_name_tag_visible(const char *player_name) {
    if (g_j47.state != PEX_JAVA47_PLAY || !player_name || !player_name[0]) return 1;
    J47Team *target = j47_team_for_member(player_name);
    if (!target || !target->name_tag_visibility[0] ||
        !strcmp(target->name_tag_visibility, "always")) return 1;
    if (!strcmp(target->name_tag_visibility, "never")) return 0;
    J47Team *viewer = j47_team_for_member(g_j47.username);
    int same_team = viewer && viewer == target;
    if (!strcmp(target->name_tag_visibility, "hideForOtherTeams")) return same_team;
    if (!strcmp(target->name_tag_visibility, "hideForOwnTeam")) return !same_team;
    return 1;
}

static J47ScoreObjective *j47_objective_find(const char *name) {
    if (!name || !name[0]) return NULL;
    for (int i = 0; i < J47_SCORE_OBJECTIVE_MAX; ++i)
        if (g_j47.objectives[i].used && strcmp(g_j47.objectives[i].name, name) == 0) return &g_j47.objectives[i];
    return NULL;
}

static J47ScoreObjective *j47_objective_put(const char *name) {
    J47ScoreObjective *o = j47_objective_find(name);
    if (o) return o;
    for (int i = 0; i < J47_SCORE_OBJECTIVE_MAX; ++i) {
        if (!g_j47.objectives[i].used) {
            o = &g_j47.objectives[i];
            memset(o, 0, sizeof(*o));
            o->used = 1;
            snprintf(o->name, sizeof(o->name), "%.*s", 16, name ? name : "");
            return o;
        }
    }
    return NULL;
}

static void j47_objective_remove(const char *name) {
    J47ScoreObjective *o = j47_objective_find(name);
    if (o) memset(o, 0, sizeof(*o));
    for (int i = 0; i < J47_SCORE_ENTRY_MAX; ++i)
        if (g_j47.scores[i].used && strcmp(g_j47.scores[i].objective, name) == 0)
            memset(&g_j47.scores[i], 0, sizeof(g_j47.scores[i]));
    for (int i = 0; i < 19; ++i)
        if (strcmp(g_j47.display_objectives[i], name) == 0) g_j47.display_objectives[i][0] = 0;
}

static J47ScoreEntry *j47_score_find(const char *owner, const char *objective) {
    for (int i = 0; i < J47_SCORE_ENTRY_MAX; ++i) {
        J47ScoreEntry *e = &g_j47.scores[i];
        if (e->used && strcmp(e->owner, owner) == 0 && strcmp(e->objective, objective) == 0) return e;
    }
    return NULL;
}

static void j47_score_set(const char *owner, const char *objective, int value) {
    J47ScoreEntry *e = j47_score_find(owner, objective);
    if (!e) {
        for (int i = 0; i < J47_SCORE_ENTRY_MAX; ++i) if (!g_j47.scores[i].used) { e = &g_j47.scores[i]; break; }
    }
    if (!e) return;
    memset(e, 0, sizeof(*e));
    e->used = 1;
    snprintf(e->owner, sizeof(e->owner), "%.*s", 40, owner ? owner : "");
    snprintf(e->objective, sizeof(e->objective), "%.*s", 16, objective ? objective : "");
    e->value = value;
}

static void j47_score_remove(const char *owner, const char *objective) {
    for (int i = 0; i < J47_SCORE_ENTRY_MAX; ++i) {
        J47ScoreEntry *e = &g_j47.scores[i];
        if (!e->used || strcmp(e->owner, owner) != 0) continue;
        if (objective && objective[0] && strcmp(e->objective, objective) != 0) continue;
        memset(e, 0, sizeof(*e));
    }
}

static void j47_format_team_name(const char *raw, char *out, size_t cap) {
    if (!out || cap == 0) return;
    out[0] = 0;
    if (!raw) raw = "";
    J47Team *t = j47_team_for_member(raw);
    /* ScorePlayerTeam.formatPlayerName() in 1.8.8 is exactly
       prefix + player + suffix. The separate team colour byte selects
       sidebar.team.<colour> and name-tag rules; it is not automatically
       inserted into the rendered name. Injecting it here caused broken TAB
       and scoreboard colour runs on servers that already colour their prefix. */
    if (t) snprintf(out, cap, "%s%s%s", t->prefix, raw, t->suffix);
    else snprintf(out, cap, "%s", raw);
}

static void j47_record_block_update(int x, int y, int z, int state) {
    J47BlockUpdate *u = NULL;
    for (int i = 0; i < J47_BLOCK_UPDATE_CACHE_MAX; ++i) {
        J47BlockUpdate *candidate = &g_j47.block_updates[i];
        if (candidate->used && candidate->x == x && candidate->y == y && candidate->z == z) { u = candidate; break; }
    }
    if (!u) {
        u = &g_j47.block_updates[g_j47.block_update_cursor++ % J47_BLOCK_UPDATE_CACHE_MAX];
        memset(u, 0, sizeof(*u));
    }
    u->used = 1;
    u->x = x; u->y = y; u->z = z; u->state = state; u->serial = g_j47.packet_serial;
}

static void j47_prepare_visible_chunk_for_update(int x, int z) {
    int cx = x >> 4, cz = z >> 4;
    int lcx = (cx * 16 - g_flat_world_origin_x) / 16;
    int lcz = (cz * 16 - g_flat_world_origin_z) / 16;
    if (!flat_local_chunk_valid(lcx, lcz)) return;
    if (g_flat_world_chunk_generated[lcz][lcx] &&
        g_flat_chunk_world_cx[lcz][lcx] == cx && g_flat_chunk_world_cz[lcz][lcx] == cz) return;

    /* A block change is valid even when the chunk snapshot was completely air
       (common on void/BedWars maps). Publish an empty client chunk first so the
       mesh system has a real owner for the new block instead of silently
       ignoring it because the local ring slot is still marked missing. */
    for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; ++y) {
        int yi = flat_y_index(y);
        for (int lz = 0; lz < 16; ++lz) for (int lx = 0; lx < 16; ++lx) {
            int wx = cx * 16 + lx, wz = cz * 16 + lz;
            int zi = flat_storage_z_index(wz), xi = flat_storage_index(wx);
            g_flat_blocks[yi][zi][xi] = 0;
            g_flat_meta[yi][zi][xi] = 0;
            g_flat_levels[yi][zi][xi] = 0;
            g_flat_block_light[yi][zi][xi] = 0;
            g_flat_sky_light[yi][zi][xi] = g_j47.dimension == 0 ? 15 : 0;
        }
    }
    g_flat_chunk_section_non_empty_mask[lcz][lcx] = 0;
    g_flat_world_chunk_has_liquid[lcz][lcx] = 0;
    stream_mark_chunk_generated(lcx, lcz);
}

static void j47_apply_block_state_visible(int x, int y, int z, int state) {
    j47_prepare_visible_chunk_for_update(x, z);
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

static void j47_reapply_cached_block_updates(void) {
    for (int i = 0; i < J47_BLOCK_UPDATE_CACHE_MAX; ++i) {
        J47BlockUpdate *u = &g_j47.block_updates[i];
        if (u->used && flat_in_bounds(u->x, u->y, u->z))
            j47_apply_block_state_visible(u->x, u->y, u->z, u->state);
    }
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

static int j47_hex_value(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int j47_parse_uuid_text(const char *text, unsigned char out[16]) {
    int high = -1;
    int n = 0;
    if (!text || !out) return 0;
    memset(out, 0, 16);
    for (; *text; ++text) {
        if (*text == '-') continue;
        int v = j47_hex_value((unsigned char)*text);
        if (v < 0) return 0;
        if (high < 0) high = v;
        else {
            if (n >= 16) return 0;
            out[n++] = (unsigned char)((high << 4) | v);
            high = -1;
        }
    }
    return n == 16 && high < 0;
}

static void j47_uuid_hex(const unsigned char uuid[16], char out[33]) {
    static const char hex[] = "0123456789abcdef";
    for (int i = 0; i < 16; ++i) {
        out[i * 2] = hex[(uuid[i] >> 4) & 15];
        out[i * 2 + 1] = hex[uuid[i] & 15];
    }
    out[32] = 0;
}

static int j47_download_skin_file(const char *url, const char *path) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    (void)url; (void)path;
    return 0;
#elif defined(PEX_PLATFORM_ANDROID) || defined(PEX_PLATFORM_ANDROID_TV) || defined(PEX_PLATFORM_LGWEBOS)
    return pex_platform_http_download_to_file(url, path, 0, 1);
#else
    return http_download_to_file(url, path, 0);
#endif
}

static DWORD WINAPI j47_skin_download_worker(LPVOID unused) {
    (void)unused;
    int ok = j47_download_skin_file(g_j47_skin_job.url, g_j47_skin_job.path);
    InterlockedExchange(&g_j47_skin_job.ok, ok ? 1 : 0);
    InterlockedExchange(&g_j47_skin_job.done, 1);
    return 0;
}

static int j47_profile_is_local(const J47Profile *p) {
    if (!p) return 0;
    if (g_j47.local_profile_uuid_valid &&
        memcmp(p->uuid, g_j47.local_profile_uuid, 16) == 0) return 1;
    return p->name[0] && g_j47.username[0] &&
           strcmp(p->name, g_j47.username) == 0;
}

static void j47_apply_profile_skin(J47Profile *p) {
    if (!p || p->skin_state != 3 || !p->skin_path[0]) return;
    Texture loaded;
    memset(&loaded, 0, sizeof(loaded));
    if (!load_png_texture(&loaded, p->skin_path, 0)) {
        p->skin_state = 4;
        return;
    }
    if (loaded.w != 64 || (loaded.h != 32 && loaded.h != 64) || !loaded.rgba) {
        free_texture(&loaded);
        p->skin_state = 4;
        return;
    }

    if (j47_profile_is_local(p)) {
        free_texture(&g_j47_local_skin);
        g_j47_local_skin = loaded;
        memset(&loaded, 0, sizeof(loaded));
    }

    for (int i = 0; i < J47_ENTITY_MAX; ++i) {
        J47Entity *e = &g_j47.entities[i];
        if (!e->used || e->kind != J47_ENTITY_PLAYER || memcmp(e->uuid, p->uuid, 16) != 0) continue;
        if (e->entity_id == g_j47.local_entity_id || !loaded.rgba) continue;
        PexNetSkin skin;
        memset(&skin, 0, sizeof(skin));
        skin.player_id = e->entity_id;
        skin.width = loaded.w;
        skin.height = loaded.h;
        skin.byte_count = (uint32_t)((size_t)loaded.w * (size_t)loaded.h * 4u);
        if (skin.byte_count <= PEX_NET_SKIN_MAX_BYTES) {
            memcpy(skin.rgba, loaded.rgba, skin.byte_count);
            pex_net_apply_skin(&skin, (uint32_t)(offsetof(PexNetSkin, rgba) + skin.byte_count));
        }
    }
    free_texture(&loaded);
}

static void j47_profile_set_skin_url(J47Profile *p, const char *url) {
    if (!p) return;
    if (!url || !url[0]) {
        p->skin_url[0] = 0;
        p->skin_path[0] = 0;
        p->skin_state = 0;
        if (j47_profile_is_local(p)) free_texture(&g_j47_local_skin);
        for (int i = 0; i < J47_ENTITY_MAX; ++i) {
            J47Entity *e = &g_j47.entities[i];
            if (!e->used || e->kind != J47_ENTITY_PLAYER ||
                memcmp(e->uuid, p->uuid, 16) != 0) continue;
            PexNetRenderPlayerState *rp = pex_net_find_render_player(e->entity_id);
            if (rp) { free_texture(&rp->skin); rp->has_skin = 0; }
        }
        return;
    }
    if (!strcmp(p->skin_url, url) && p->skin_state != 4) return;
    snprintf(p->skin_url, sizeof(p->skin_url), "%.*s", (int)sizeof(p->skin_url) - 1, url);
    char uuid_hex[33];
    j47_uuid_hex(p->uuid, uuid_hex);
    uint32_t url_hash=2166136261u;
    for(const unsigned char*s=(const unsigned char*)url;*s;++s)url_hash=(url_hash^*s)*16777619u;
    snprintf(p->skin_path, sizeof(p->skin_path), "%s/java_%s_%08x.png",
             g_skin_dir[0] ? g_skin_dir : ".", uuid_hex, (unsigned)url_hash);
    p->skin_state = file_exists(p->skin_path) ? 3 : 1;
    if (p->skin_state == 3) j47_apply_profile_skin(p);
}

static void j47_skin_pump(void) {
    if (g_j47_skin_thread && InterlockedCompareExchange(&g_j47_skin_job.done, 0, 0)) {
        CloseHandle(g_j47_skin_thread);
        g_j47_skin_thread = NULL;
        J47Profile *p = j47_profile_find_uuid(g_j47_skin_job.uuid);
        if (p && !strcmp(p->skin_url, g_j47_skin_job.url)) {
            p->skin_state = InterlockedCompareExchange(&g_j47_skin_job.ok, 0, 0) ? 3 : 4;
            if (p->skin_state == 3) j47_apply_profile_skin(p);
        }
        memset(&g_j47_skin_job, 0, sizeof(g_j47_skin_job));
    }
    if (g_j47_skin_thread) return;
    for (int i = 0; i < J47_PROFILE_MAX; ++i) {
        J47Profile *p = &g_j47.profiles[i];
        if (!p->used || p->skin_state != 1 || !p->skin_url[0]) continue;
        memset(&g_j47_skin_job, 0, sizeof(g_j47_skin_job));
        memcpy(g_j47_skin_job.uuid, p->uuid, 16);
        snprintf(g_j47_skin_job.url, sizeof(g_j47_skin_job.url), "%s", p->skin_url);
        snprintf(g_j47_skin_job.path, sizeof(g_j47_skin_job.path), "%s", p->skin_path);
        pxc_mkdirs_for_file(g_j47_skin_job.path);
        p->skin_state = 2;
        g_j47_skin_thread = CreateThread(NULL, 0, j47_skin_download_worker, NULL, 0, NULL);
        if (!g_j47_skin_thread) p->skin_state = 4;
        break;
    }
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
    /* BedWars places often arrive just outside the currently mapped ring while
       the local world origin is moving. Keep the authoritative Java state and
       replay it whenever that coordinate becomes visible instead of silently
       dropping the update. */
    j47_record_block_update(x, y, z, state);
    j47_apply_block_state_visible(x, y, z, state);
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

static void j47_install_chunk(int cx, int cz, int full, unsigned int mask, const unsigned char *data, size_t len, int has_sky, int empty_means_unload) {
    if (full && mask == 0 && empty_means_unload) {
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
        /* This chunk snapshot is newer than every block update already
           parsed before it. Retire those older cached records for the sections
           replaced by the snapshot; later S22/S23 packets will be cached and
           applied in normal TCP order. */
        for (int i = 0; i < J47_BLOCK_UPDATE_CACHE_MAX; ++i) {
            J47BlockUpdate *u = &g_j47.block_updates[i];
            if (!u->used || (u->x >> 4) != cx || (u->z >> 4) != cz) continue;
            if (full || (mask & (1u << ((unsigned)u->y >> 4)))) u->used = 0;
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
    J47Profile *profile = j47_profile_find_uuid(uuid);
    /* Some skin plugins briefly destroy and respawn the local player entity.
       The vanilla client never renders that entity as a separate remote
       player, so ignore it here while still accepting the refreshed profile. */
    if (entity_id == g_j47.local_entity_id) {
        j47_parse_entity_metadata(r, entity_id);
        if (r->failed) return;
        if (profile && profile->skin_state == 3) j47_apply_profile_skin(profile);
        return;
    }
    J47Entity *e = j47_entity_prepare_spawn(entity_id);
    if (!e) return;
    int slot = e->player_slot;
    if (slot < 0) slot = j47_player_alloc();
    if (slot < 0) return;
    e->kind = J47_ENTITY_PLAYER; e->player_slot = slot; memcpy(e->uuid, uuid, 16); e->x = x; e->y = y; e->z = z; e->yaw = yaw; e->pitch = pitch;
    e->held_item_id = pex_java47_translate_item_id(held);
    e->held_item_count = e->held_item_id > 0 ? 1 : 0;
    PexNetPlayerState *p = &g_mp_players[slot];
    memset(p, 0, sizeof(*p));
    p->player_id = entity_id; p->x = (float)x; p->y = (float)y + 1.62f; p->z = (float)z;
    p->yaw = yaw; p->pitch = pitch; p->health = 20;
    snprintf(p->name, sizeof(p->name), "%s", profile && profile->name[0] ? profile->name : "Player");
    g_mp_prev_players[slot] = *p;
    j47_parse_entity_metadata(r, entity_id);
    if (r->failed) { j47_entity_remove(entity_id); return; }
    if (profile && profile->skin_state == 3) j47_apply_profile_skin(profile);
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
    J47Entity *e = j47_entity_prepare_spawn(entity_id);
    if (!e) { (void)j47_skip_metadata(r); return; }
    /* Vanilla 1.8.8 normally uses Spawn Object type 78 for armor stands, but
       several proxy/plugin stacks encode the EntityList id (30) through Spawn
       Mob. Treat both forms as the same dedicated entity so the generic mob
       fallback can never turn an armor stand into a pig. */
    if (java_type == 30) {
        e->kind = J47_ENTITY_ARMOR_STAND;
        e->java_type = java_type;
        e->is_armor_stand = 1;
        e->x=x; e->y=y; e->z=z; e->yaw=yaw; e->pitch=pitch; e->head_yaw=head;
        e->armor_pose[2][0] = -10.0f; e->armor_pose[2][2] = -10.0f;
        e->armor_pose[3][0] = -15.0f; e->armor_pose[3][2] =  10.0f;
        e->armor_pose[4][0] =  -1.0f; e->armor_pose[4][2] =  -1.0f;
        e->armor_pose[5][0] =   1.0f; e->armor_pose[5][2] =   1.0f;
        j47_parse_entity_metadata(r, entity_id);
        if(r->failed){j47_entity_remove(entity_id);return;}
        return;
    }
    int slot = e->mob_slot;
    if (slot < 0) slot = j47_mob_alloc_slot();
    if (slot < 0) { (void)j47_skip_metadata(r); return; }
    int type = pex_java47_translate_mob_type(java_type);
    passive_mob_init(&g_passive_mobs[slot], type, (float)x, (float)y, (float)z);
    PassiveMob *m = &g_passive_mobs[slot];
    m->entity_id = entity_id; m->yaw = m->prev_yaw = yaw; m->pitch = m->prev_pitch = pitch;
    m->head_yaw = m->prev_head_yaw = head; m->render_yaw = m->prev_render_yaw = yaw;
    e->kind = J47_ENTITY_MOB; e->java_type = java_type; e->mob_slot = slot; e->x=x; e->y=y; e->z=z; e->yaw=yaw; e->pitch=pitch; e->head_yaw=head;
    j47_parse_entity_metadata(r, entity_id);
    if(r->failed){j47_entity_remove(entity_id);return;}
}

static int j47_projectile_alloc_slot(void) {
    for (int i = 0; i < MAX_PROJECTILE_ENTITIES; ++i)
        if (!g_projectiles[i].active) return i;
    /* Prefer replacing another server-owned visual rather than a local arrow. */
    for (int i = 0; i < MAX_PROJECTILE_ENTITIES; ++i)
        if (g_projectiles[i].owner_type == 2) return i;
    return -1;
}

static void j47_spawn_server_fireball(J47Entity *e, int large, double x, double y, double z, int vx, int vy, int vz) {
    int slot = j47_projectile_alloc_slot();
    if (!e || slot < 0) return;
    FlatProjectile *p = &g_projectiles[slot];
    memset(p, 0, sizeof(*p));
    p->active = 1;
    p->type = large ? FLAT_PROJECTILE_LARGE_FIREBALL : FLAT_PROJECTILE_SMALL_FIREBALL;
    p->owner_type = 2; /* server-owned: rendered/predicted, never locally impacts */
    p->owner_mob_index = -1;
    p->x = p->prev_x = (float)x;
    p->y = p->prev_y = (float)y;
    p->z = p->prev_z = (float)z;
    p->fire_ticks = 2;
    e->projectile_slot = slot;
    e->projectile_ax = (float)vx / 8000.0f;
    e->projectile_ay = (float)vy / 8000.0f;
    e->projectile_az = (float)vz / 8000.0f;
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
    J47Entity *e = j47_entity_prepare_spawn(entity_id); if (!e) return;
    e->kind = J47_ENTITY_OTHER; e->java_type = type; e->x=x; e->y=y; e->z=z; e->yaw=yaw; e->pitch=pitch;
    if (type == 2) { /* dropped item: item metadata arrives in S1C */
        int slot = j47_drop_alloc();
        if (slot >= 0) {
            FlatDroppedItem *d=&g_drops[slot]; memset(d,0,sizeof(*d)); d->active=1; d->net_id=entity_id;
            d->x=d->prev_x=(float)x; d->y=d->prev_y=(float)y; d->z=d->prev_z=(float)z;
            d->mx=(float)vx/8000.0f; d->my=(float)vy/8000.0f; d->mz=(float)vz/8000.0f; d->stack.id=BLOCK_STONE; d->stack.count=1;
            e->kind=J47_ENTITY_DROP; e->drop_slot=slot;
        }
    } else if (type == 63 || type == 64 || type == 66) {
        /* Large fireball, small fireball and wither skull.  The speed fields
           are acceleration vectors in protocol 47, not ordinary velocity. */
        j47_spawn_server_fireball(e, type == 63, x, y, z, vx, vy, vz);
    } else if (type == 78) { /* EntityArmorStand: protocol-47 spawn-object type */
        /* Keep it as its own protocol entity instead of pretending it is a pig.
           The renderer below understands the 1.8 armor-stand metadata flags,
           poses, equipment and name while preserving the real entity ID for
           plugin interactions. */
        e->kind = J47_ENTITY_ARMOR_STAND;
        e->mob_slot = -1;
        e->is_armor_stand = 1;
        e->armor_pose[2][0] = -10.0f; e->armor_pose[2][2] = -10.0f;
        e->armor_pose[3][0] = -15.0f; e->armor_pose[3][2] =  10.0f;
        e->armor_pose[4][0] =  -1.0f; e->armor_pose[4][2] =  -1.0f;
        e->armor_pose[5][0] =   1.0f; e->armor_pose[5][2] =   1.0f;
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
    if (e->projectile_slot >= 0 && e->projectile_slot < MAX_PROJECTILE_ENTITIES) {
        FlatProjectile *p = &g_projectiles[e->projectile_slot];
        if (p->active) {
            p->prev_x = p->x; p->prev_y = p->y; p->prev_z = p->z;
            p->x = (float)x; p->y = (float)y; p->z = (float)z;
        }
    }
    if (e->is_armor_stand) j47_sync_proxy_item_visual(e);
}

static void j47_apply_player_metadata_flags(J47Entity *e, int metadata_flags) {
    if (!e || e->kind != J47_ENTITY_PLAYER || e->player_slot < 0 || e->player_slot >= g_mp_player_count) return;
    PexNetPlayerState *p = &g_mp_players[e->player_slot];
    p->flags &= ~(PEX_PLAYER_FLAG_SNEAKING | PEX_PLAYER_FLAG_INVISIBLE);
    if (metadata_flags & 0x02) p->flags |= PEX_PLAYER_FLAG_SNEAKING;
    if (metadata_flags & 0x20) p->flags |= PEX_PLAYER_FLAG_INVISIBLE;
    PexNetRenderPlayerState *rp = pex_net_find_render_player(e->entity_id);
    if (rp) rp->flags = p->flags;
}

static void j47_parse_entity_metadata(J47Reader *r, int entity_id) {
    J47Entity *e = j47_entity_find(entity_id);
    int is_local_player = entity_id == g_j47.local_entity_id;
    for (;;) {
        int h = (int)j47_r_u8(r);
        if (r->failed || h == 0x7f) break;
        int type = (h >> 5) & 7, index = h & 31;
        if (type == 5) {
            ItemStack st;
            if (!j47_read_item(r, &st)) return;
            if (e && e->kind == J47_ENTITY_DROP && index == 10 && e->drop_slot >= 0)
                g_drops[e->drop_slot].stack = st;
        } else if (type == 0) {
            int v = (int8_t)j47_r_u8(r);
            if (e && index == 0) {
                e->metadata_flags = v;
                j47_apply_player_metadata_flags(e, v);
            }
            if (is_local_player && index == 0) {
                /* The server owns the local entity flags. Keep the fire overlay
                   in sync, but never derive health loss from it locally. */
                if (v & 0x01) {
                    if (g_player_fire_ticks < 2) g_player_fire_ticks = 2;
                } else {
                    g_player_fire_ticks = 0;
                }
            }
            if (e && index == 3) e->custom_name_visible = v ? 1 : 0;
            if (e && e->is_armor_stand && index == 10) e->armor_stand_flags = v & 0xff;
            if (e && e->kind == J47_ENTITY_MOB && index == 6 && e->mob_slot >= 0)
                g_passive_mobs[e->mob_slot].health = v;
            if (e && e->kind == J47_ENTITY_MOB && index == 12 && e->mob_slot >= 0)
                g_passive_mobs[e->mob_slot].baby_age = v < 0 ? -24000 : 0;
        } else if (type == 1) {
            int v = (int16_t)j47_r_be16(r);
            if (is_local_player && index == 1) {
                if (v < 0) v = 0;
                if (v > 300) v = 300;
                g_player_air = v;
            }
        } else if (type == 2) {
            int v = j47_r_be32(r);
            if (e && e->kind == J47_ENTITY_MOB && e->java_type == 120 && index == 16 && e->mob_slot >= 0) {
                if (v < 0) v = 0;
                if (v > 4) v %= 5;
                g_passive_mobs[e->mob_slot].fleece_color = v;
            }
        } else if (type == 3) {
            float v = j47_r_float(r);
            if (e && e->kind == J47_ENTITY_MOB && index == 6 && e->mob_slot >= 0)
                g_passive_mobs[e->mob_slot].health = (int)ceilf(v);
        } else if (type == 4) {
            char tmp[512];
            if (!j47_r_string(r, tmp, sizeof(tmp))) return;
            if (e && index == 2)
                snprintf(e->custom_name, sizeof(e->custom_name), "%.*s", (int)sizeof(e->custom_name) - 1, tmp);
        } else if (type == 6) {
            (void)j47_r_be32(r); (void)j47_r_be32(r); (void)j47_r_be32(r);
        } else if (type == 7) {
            float px = j47_r_float(r), py = j47_r_float(r), pz = j47_r_float(r);
            if (e && e->is_armor_stand && index >= 11 && index <= 16) {
                int pose = index - 11;
                e->armor_pose[pose][0] = px;
                e->armor_pose[pose][1] = py;
                e->armor_pose[pose][2] = pz;
            }
        }
    }
    if (e && e->is_armor_stand) j47_sync_proxy_item_visual(e);
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
    int32_t action = 0, count = 0;
    if (!j47_r_varint(r, &action) || !j47_r_varint(r, &count) || count < 0 || count > 10000) return;
    for (int i = 0; i < count; ++i) {
        if (r->pos + 16 > r->len) { r->failed = 1; return; }
        unsigned char uuid[16];
        memcpy(uuid, r->data + r->pos, 16);
        r->pos += 16;
        J47Profile *profile = j47_profile_find_uuid(uuid);

        if (action == 0) { /* ADD_PLAYER */
            char name[64] = {0};
            char skin_url[J47_SKIN_URL_MAX] = {0};
            int32_t props = 0, gm = 0, ping = 0;
            if (!j47_r_string(r, name, sizeof(name)) || !j47_r_varint(r, &props) || props < 0 || props > 256) return;
            for (int pidx = 0; pidx < props; ++pidx) {
                char property_name[128], value[8192];
                if (!j47_r_string(r, property_name, sizeof(property_name)) ||
                    !j47_r_string(r, value, sizeof(value))) return;
                int signed_property = j47_r_u8(r);
                if (signed_property) {
                    char signature[8192];
                    if (!j47_r_string(r, signature, sizeof(signature))) return;
                }
                if (!strcmp(property_name, "textures"))
                    j47_extract_skin_url(value, skin_url, sizeof(skin_url));
            }
            if (!j47_r_varint(r, &gm) || !j47_r_varint(r, &ping)) return;
            int has_display = j47_r_u8(r);
            char display_text[192] = {0};
            if (has_display) {
                char display_json[4096];
                if (!j47_r_string(r, display_json, sizeof(display_json))) return;
                j47_json_collect_text(display_json, display_text, sizeof(display_text));
            }
            profile = j47_profile_put(uuid, name);
            if (profile) {
                profile->listed = 1;
                profile->game_mode = gm;
                profile->ping = ping;
                snprintf(profile->display_name, sizeof(profile->display_name), "%s", display_text);
                j47_profile_set_skin_url(profile, skin_url);
            }
        } else if (action == 1) { /* UPDATE_GAME_MODE */
            int32_t gm = 0;
            if (!j47_r_varint(r, &gm)) return;
            if (profile) profile->game_mode = gm;
        } else if (action == 2) { /* UPDATE_LATENCY */
            int32_t ping = 0;
            if (!j47_r_varint(r, &ping)) return;
            if (profile) profile->ping = ping;
        } else if (action == 3) { /* UPDATE_DISPLAY_NAME */
            int has_display = j47_r_u8(r);
            char display_text[192] = {0};
            if (has_display) {
                char display_json[4096];
                if (!j47_r_string(r, display_json, sizeof(display_json))) return;
                j47_json_collect_text(display_json, display_text, sizeof(display_text));
            }
            if (profile) snprintf(profile->display_name, sizeof(profile->display_name), "%s", display_text);
        } else if (action == 4) { /* REMOVE_PLAYER */
            /* Preserve cached skin/profile data for skin-plugin remove/re-add
               sequences, but remove the player from the visible tab list. */
            if (profile) profile->listed = 0;
        } else {
            r->failed = 1;
            return;
        }
        if (r->failed) return;
    }
}

static void j47_handle_scoreboard_objective(J47Reader *r) {
    char name[32] = {0};
    if (!j47_r_string(r, name, sizeof(name))) return;
    int mode = (int)(int8_t)j47_r_u8(r);
    if (mode == 1) {
        j47_objective_remove(name);
        return;
    }
    if (mode != 0 && mode != 2) { r->failed = 1; return; }
    char display_json[256] = {0};
    char render_type[32] = {0};
    if (!j47_r_string(r, display_json, sizeof(display_json)) ||
        !j47_r_string(r, render_type, sizeof(render_type))) return;
    J47ScoreObjective *o = j47_objective_put(name);
    if (!o) return;
    /* Objective display names are legacy formatted strings in protocol 47,
       not chat JSON. Preserve section-sign colour/style codes verbatim. */
    snprintf(o->display_name, sizeof(o->display_name), "%.*s", 64, display_json);
    o->render_hearts = !strcmp(render_type, "hearts");
}

static void j47_handle_update_score(J47Reader *r) {
    char owner[64] = {0};
    if (!j47_r_string(r, owner, sizeof(owner))) return;
    int32_t action = 0;
    if (!j47_r_varint(r, &action)) return;
    char objective[32] = {0};
    if (!j47_r_string(r, objective, sizeof(objective))) return;
    if (action == 1) {
        j47_score_remove(owner, objective);
    } else if (action == 0) {
        int32_t value = 0;
        if (!j47_r_varint(r, &value)) return;
        j47_score_set(owner, objective, value);
    } else r->failed = 1;
}

static void j47_handle_display_scoreboard(J47Reader *r) {
    int slot = (int)(int8_t)j47_r_u8(r);
    char objective[32] = {0};
    if (!j47_r_string(r, objective, sizeof(objective))) return;
    if (slot >= 0 && slot < 19)
        snprintf(g_j47.display_objectives[slot], sizeof(g_j47.display_objectives[slot]), "%.*s", 16, objective);
}

static void j47_handle_teams(J47Reader *r) {
    char name[32] = {0};
    if (!j47_r_string(r, name, sizeof(name))) return;
    int action = (int)(int8_t)j47_r_u8(r);
    if (action == 1) {
        J47Team *old = j47_team_find(name);
        if (old) memset(old, 0, sizeof(*old));
        return;
    }
    J47Team *team = j47_team_put(name);
    if (!team) return;
    if (action == 0 || action == 2) {
        char display[128], prefix[128], suffix[128], visibility[64];
        if (!j47_r_string(r, display, sizeof(display)) ||
            !j47_r_string(r, prefix, sizeof(prefix)) ||
            !j47_r_string(r, suffix, sizeof(suffix))) return;
        int friendly = j47_r_u8(r);
        if (!j47_r_string(r, visibility, sizeof(visibility))) return;
        int color = (int)(int8_t)j47_r_u8(r);
        snprintf(team->display_name, sizeof(team->display_name), "%.*s", 64, display);
        snprintf(team->prefix, sizeof(team->prefix), "%.*s", 64, prefix);
        snprintf(team->suffix, sizeof(team->suffix), "%.*s", 64, suffix);
        snprintf(team->name_tag_visibility, sizeof(team->name_tag_visibility), "%.*s", 23,
                 visibility[0] ? visibility : "always");
        team->friendly_flags = friendly;
        team->color = color;
    }
    if (action == 0 || action == 3 || action == 4) {
        int32_t member_count = 0;
        if (!j47_r_varint(r, &member_count) || member_count < 0 || member_count > 10000) return;
        if (action == 0) team->member_count = 0;
        for (int i = 0; i < member_count; ++i) {
            char member[64];
            if (!j47_r_string(r, member, sizeof(member))) return;
            if (action == 4) j47_team_remove_member(team, member);
            else j47_team_add_member(team, member);
        }
    }
    if (action < 0 || action > 4) r->failed = 1;
}

static void j47_handle_tab_header_footer(J47Reader *r) {
    char header_json[8192], footer_json[8192];
    if (!j47_r_string(r, header_json, sizeof(header_json)) ||
        !j47_r_string(r, footer_json, sizeof(footer_json))) return;
    j47_json_collect_text(header_json, g_j47.tab_header, sizeof(g_j47.tab_header));
    j47_json_collect_text(footer_json, g_j47.tab_footer, sizeof(g_j47.tab_footer));
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

static int j47_uuid_equals(const unsigned char uuid[16], const unsigned char expected[16]) {
    return memcmp(uuid, expected, 16) == 0;
}

static int j47_movement_modifier_is_locally_recreated(const unsigned char uuid[16]) {
    /* Vanilla client-side modifiers that PexCraft already recreates from its
       sprint and potion state. Excluding them from S20 avoids applying the same
       speed change twice while still honoring custom server modifiers. */
    static const unsigned char sprint_uuid[16] = {
        0x66,0x2a,0x6b,0x8d,0xda,0x3e,0x4c,0x1c,0x88,0x13,0x96,0xea,0x60,0x97,0x27,0x8d
    };
    static const unsigned char speed_uuid[16] = {
        0x91,0xae,0xaa,0x56,0x37,0x6b,0x44,0x98,0x93,0x5b,0x2f,0x7f,0x68,0x07,0x06,0x35
    };
    static const unsigned char slowness_uuid[16] = {
        0x71,0x07,0xde,0x5e,0x7c,0xe8,0x40,0x30,0x94,0x0e,0x51,0x4c,0x1f,0x16,0x08,0x90
    };
    return j47_uuid_equals(uuid, sprint_uuid) ||
           j47_uuid_equals(uuid, speed_uuid) ||
           j47_uuid_equals(uuid, slowness_uuid);
}

static void j47_handle_entity_properties(J47Reader *r) {
    int32_t entity_id=0;
    int attribute_count=0;
    if(!j47_r_varint(r,&entity_id))return;
    attribute_count=(int)j47_r_be32(r);
    if(r->failed||attribute_count<0||attribute_count>1024){r->failed=1;return;}

    for(int i=0;i<attribute_count&&!r->failed;i++){
        char name[96];
        double base=0.0;
        int32_t modifier_count=0;
        if(!j47_r_string(r,name,sizeof(name)))return;
        base=j47_r_double(r);
        if(!j47_r_varint(r,&modifier_count)||modifier_count<0||modifier_count>4096){r->failed=1;return;}

        double op0=0.0;
        double op1=0.0;
        double op2_product=1.0;
        int movement_attribute=(entity_id==g_j47.local_entity_id&&!strcmp(name,"generic.movementSpeed"));
        for(int m=0;m<modifier_count&&!r->failed;m++){
            unsigned char uuid[16];
            double amount;
            int operation;
            if(r->pos+16>r->len){r->failed=1;return;}
            memcpy(uuid,r->data+r->pos,16);r->pos+=16;
            amount=j47_r_double(r);
            operation=(int)(int8_t)j47_r_u8(r);
            if(!movement_attribute||j47_movement_modifier_is_locally_recreated(uuid))continue;
            if(operation==0)op0+=amount;
            else if(operation==1)op1+=amount;
            else if(operation==2)op2_product*=1.0+amount;
        }

        if(movement_attribute&&!r->failed){
            double after_add=base+op0;
            double value=(after_add+after_add*op1)*op2_product;
            double scale=value/0.1;
            if(!isfinite(scale)||scale<=0.0)scale=1.0;
            if(scale<0.05)scale=0.05;
            if(scale>10.0)scale=10.0;
            g_j47.server_movement_speed_scale=(float)scale;
        }
    }
}

static void j47_handle_play_packet(int packet_id, J47Reader *r) {
    switch(packet_id){
        case 0x00:{int32_t id;if(j47_r_varint(r,&id)){J47Writer w;j47_writer_init(&w,8);j47_w_varint(&w,id);j47_send_packet(0x00,w.data,w.len);j47_writer_free(&w);}break;}
        case 0x01:{g_j47.local_entity_id=j47_r_be32(r);g_mp_player_id=g_j47.local_entity_id;int gm=(int)j47_r_u8(r);g_j47.game_mode=gm&7;g_game_mode=g_j47.game_mode==1?1:0;g_pending_game_mode=g_game_mode;if(!g_game_mode)g_creative_flying=0;memset(g_player_potion_effects,0,sizeof(g_player_potion_effects));g_j47.server_movement_speed_scale=1.0f;g_j47.last_abilities_flags=-1;g_j47.dimension=(int8_t)j47_r_u8(r);g_j47.difficulty=(int)j47_r_u8(r);(void)j47_r_u8(r);char wt[64];j47_r_string(r,wt,sizeof(wt));g_j47.has_join_game=1;g_j47.progress=35;j47_set_status("Downloading terrain");if(!r->failed&&!j47_send_client_identity())j47_fail("Could not send Java client settings");break;}
        case 0x02:{char json[8192],text[512];if(j47_r_string(r,json,sizeof(json))){(void)j47_r_u8(r);j47_json_collect_text(json,text,sizeof(text));if(text[0])hud_add_chat(text);}break;}
        case 0x03:{(void)j47_r_be64(r);long long t=j47_r_be64(r);g_j47.daylight_cycle=t>=0;g_world_time=t<0?-t:t;break;}
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
        case 0x07:{
            int old_dimension=g_j47.dimension;
            int new_dimension=j47_r_be32(r);
            g_j47.difficulty=(int)j47_r_u8(r);
            g_j47.game_mode=(int)j47_r_u8(r)&7;
            g_game_mode=g_j47.game_mode==1?1:0;
            g_pending_game_mode=g_game_mode;
            if(!g_game_mode)g_creative_flying=0;
            g_j47.last_abilities_flags=-1;
            g_player_dead=0;
            g_player_death_time=0;
            g_player_motion_x=g_player_motion_y=g_player_motion_z=0.0f;
            char wt[64];j47_r_string(r,wt,sizeof(wt));
            if(r->failed)break;
            /* Skin-restoration plugins use either a same-dimension Respawn
               or a rapid fake-dimension/real-dimension pair to force the
               vanilla client to rebuild its skin. Do not erase terrain merely
               because S07 arrived. A real dimension transfer is committed
               when the server actually starts sending new chunk data. */
            if(!g_j47.world_ready){
                g_j47.dimension=new_dimension;
                g_j47.pending_respawn_dimension=0;
                j47_reset_world_for_respawn();
            }else if(g_j47.pending_respawn_dimension){
                if(new_dimension==g_j47.pending_respawn_from){
                    /* Fake transition returned to the original dimension. */
                    g_j47.pending_respawn_dimension=0;
                    g_j47.dimension=new_dimension;
                }else{
                    g_j47.pending_respawn_to=new_dimension;
                }
            }else if(new_dimension!=old_dimension){
                g_j47.pending_respawn_dimension=1;
                g_j47.pending_respawn_from=old_dimension;
                g_j47.pending_respawn_to=new_dimension;
            }else{
                g_j47.dimension=new_dimension;
            }
            break;
        }
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
            g_player_server_on_ground=0;
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
        case 0x12:{int32_t id;if(j47_r_varint(r,&id)){int vx=(int16_t)j47_r_be16(r),vy=(int16_t)j47_r_be16(r),vz=(int16_t)j47_r_be16(r);if(id==g_j47.local_entity_id){g_player_motion_x=vx/8000.0f;g_player_motion_y=vy/8000.0f;g_player_motion_z=vz/8000.0f;if(vy>0)g_player_on_ground=0;}else{J47Entity*e=j47_entity_find(id);if(e&&e->kind==J47_ENTITY_DROP&&e->drop_slot>=0){g_drops[e->drop_slot].mx=vx/8000.0f;g_drops[e->drop_slot].my=vy/8000.0f;g_drops[e->drop_slot].mz=vz/8000.0f;}if(e&&e->projectile_slot>=0&&e->projectile_slot<MAX_PROJECTILE_ENTITIES){FlatProjectile*p=&g_projectiles[e->projectile_slot];p->mx=vx/8000.0f;p->my=vy/8000.0f;p->mz=vz/8000.0f;}}}break;}
        case 0x13:{int32_t n;if(j47_r_varint(r,&n)&&n>=0&&n<100000)for(int i=0;i<n;i++){int32_t id;if(!j47_r_varint(r,&id))break;j47_entity_remove(id);}break;}
        case 0x14:case 0x15:case 0x16:case 0x17:{int32_t id;if(!j47_r_varint(r,&id))break;J47Entity*e=j47_entity_find(id);double x=e?e->x:0,y=e?e->y:0,z=e?e->z:0;float yaw=e?e->yaw:0,pitch=e?e->pitch:0;int rot=packet_id==0x16||packet_id==0x17;if(packet_id==0x15||packet_id==0x17){x+=(int8_t)j47_r_u8(r)/32.0;y+=(int8_t)j47_r_u8(r)/32.0;z+=(int8_t)j47_r_u8(r)/32.0;}if(rot){yaw=(int8_t)j47_r_u8(r)*360.0f/256.0f;pitch=(int8_t)j47_r_u8(r)*360.0f/256.0f;}(void)j47_r_u8(r);j47_apply_entity_position(e,x,y,z,yaw,pitch,rot);break;}
        case 0x18:{int32_t id;if(j47_r_varint(r,&id)){double x=j47_r_be32(r)/32.0,y=j47_r_be32(r)/32.0,z=j47_r_be32(r)/32.0;float yaw=(int8_t)j47_r_u8(r)*360.0f/256.0f,pitch=(int8_t)j47_r_u8(r)*360.0f/256.0f;(void)j47_r_u8(r);j47_apply_entity_position(j47_entity_find(id),x,y,z,yaw,pitch,1);}break;}
        case 0x19:{int32_t id;if(j47_r_varint(r,&id)){float head=(int8_t)j47_r_u8(r)*360.0f/256.0f;J47Entity*e=j47_entity_find(id);if(e){e->head_yaw=head;if(e->kind==J47_ENTITY_MOB&&e->mob_slot>=0){g_passive_mobs[e->mob_slot].prev_head_yaw=g_passive_mobs[e->mob_slot].head_yaw;g_passive_mobs[e->mob_slot].head_yaw=head;}}}break;}
        case 0x1A:{int32_t id;if(j47_r_varint(r,&id)){int status=(int8_t)j47_r_u8(r);J47Entity*e=j47_entity_find(id);if(e&&e->kind==J47_ENTITY_MOB&&e->mob_slot>=0){PassiveMob*m=&g_passive_mobs[e->mob_slot];if(status==2)m->hurt_time=10;if(status==3)m->death_time=1;}}break;}
        case 0x1C:{int32_t id;if(j47_r_varint(r,&id))j47_parse_entity_metadata(r,id);break;}
        case 0x1D:{int32_t id=0,duration=0;if(j47_r_varint(r,&id)){int effect=(int)(int8_t)j47_r_u8(r);int amplifier=(int)(int8_t)j47_r_u8(r);j47_r_varint(r,&duration);(void)j47_r_u8(r);if(!r->failed&&id==g_j47.local_entity_id&&effect>0&&effect<PEX_POTION_MAX){if(amplifier<0)amplifier=0;g_player_potion_effects[effect].duration=duration>0?duration:1;g_player_potion_effects[effect].amplifier=amplifier;}}break;}
        case 0x1E:{int32_t id=0;if(j47_r_varint(r,&id)){int effect=j47_r_u8(r);if(!r->failed&&id==g_j47.local_entity_id&&effect>0&&effect<PEX_POTION_MAX){g_player_potion_effects[effect].duration=0;g_player_potion_effects[effect].amplifier=0;}}break;}
        case 0x1F:{g_player_xp_progress=j47_r_float(r);int32_t level,total;j47_r_varint(r,&level);j47_r_varint(r,&total);g_player_xp_level=level;g_player_xp_total=total;break;}
        case 0x20:j47_handle_entity_properties(r);break;
        case 0x21:{j47_commit_pending_respawn_dimension();int cx=j47_r_be32(r),cz=j47_r_be32(r),full=j47_r_u8(r);unsigned mask=j47_r_be16(r);int32_t n;if(j47_r_varint(r,&n)&&n>=0&&(size_t)n<=r->len-r->pos)j47_install_chunk(cx,cz,full,mask,r->data+r->pos,(size_t)n,g_j47.dimension==0,1);break;}
        case 0x22:{int cx=j47_r_be32(r),cz=j47_r_be32(r);int32_t n;if(j47_r_varint(r,&n)&&n>=0&&n<100000)for(int i=0;i<n;i++){int packed=j47_r_be16(r);int32_t state;j47_r_varint(r,&state);int x=cx*16+((packed>>12)&15),z=cz*16+((packed>>8)&15),y=packed&255;j47_set_block_state(x,y,z,state);}break;}
        case 0x23:{int x=0,y=0,z=0;int32_t s;if(j47_read_block_pos(r,&x,&y,&z)&&j47_r_varint(r,&s))j47_set_block_state(x,y,z,s);break;}
        case 0x26:{j47_commit_pending_respawn_dimension();int has_sky=j47_r_u8(r);int32_t n;if(!j47_r_varint(r,&n)||n<0||n>1024)break;int*cx=malloc((size_t)n*sizeof(int));int*cz=malloc((size_t)n*sizeof(int));unsigned short*mask=malloc((size_t)n*sizeof(unsigned short));if(!cx||!cz||!mask){free(cx);free(cz);free(mask);break;}for(int i=0;i<n;i++){cx[i]=j47_r_be32(r);cz[i]=j47_r_be32(r);mask[i]=j47_r_be16(r);}for(int i=0;i<n&&!r->failed;i++){size_t bytes=0;for(int sy=0;sy<16;sy++)if(mask[i]&(1u<<sy))bytes+=8192+2048+(has_sky?2048:0);bytes+=256;if(bytes>r->len-r->pos){r->failed=1;break;}j47_install_chunk(cx[i],cz[i],1,mask[i],r->data+r->pos,bytes,has_sky,0);r->pos+=bytes;}free(cx);free(cz);free(mask);break;}
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
        case 0x3B:j47_handle_scoreboard_objective(r);break;
        case 0x3C:j47_handle_update_score(r);break;
        case 0x3D:j47_handle_display_scoreboard(r);break;
        case 0x3E:j47_handle_teams(r);break;
        case 0x39:{
            int flags=j47_r_u8(r);
            float fly_speed=j47_r_float(r);
            float walk_speed=j47_r_float(r);
            if(!r->failed){
                g_j47.server_abilities_flags=flags;
                g_j47.server_fly_speed=fly_speed;
                g_j47.server_walk_speed=walk_speed;
                /* This is the state the server has just assigned.  Do not
                   echo a synthetic abilities packet during ordinary survival
                   movement; vanilla only sends C13 when the player actually
                   toggles flight. */
                g_j47.last_abilities_flags=flags;
                g_player_capabilities.disable_damage=(flags&8)?1:0;
                g_player_capabilities.is_flying=(flags&2)?1:0;
                g_player_capabilities.allow_flying=(flags&4)?1:0;
                g_player_capabilities.is_creative_mode=(flags&1)?1:0;
            }
            break;
        }
        case 0x40:{char json[8192],text[512];if(j47_r_string(r,json,sizeof(json))){j47_json_collect_text(json,text,sizeof(text));j47_fail(text[0]?text:"Disconnected by Java server");}break;}
        case 0x41:g_j47.difficulty=j47_r_u8(r);break;
        case 0x46:{int32_t threshold;if(j47_r_varint(r,&threshold))g_j47.compression_threshold=threshold;break;}
        case 0x47:j47_handle_tab_header_footer(r);break;
        case 0x48:{char url[2048],hash[128];j47_r_string(r,url,sizeof(url));j47_r_string(r,hash,sizeof(hash));J47Writer w;j47_writer_init(&w,128);j47_w_string(&w,hash);j47_w_varint(&w,1);j47_send_packet(0x19,w.data,w.len);j47_writer_free(&w);break;}
        default: break;
    }
}

static void j47_handle_packet(const unsigned char *data,size_t len){
    J47Reader r;int32_t id=-1;j47_reader_init(&r,data,len);if(!j47_r_varint(&r,&id))return;
    ++g_j47.packet_serial;
    if(g_j47.state==PEX_JAVA47_LOGIN){
        if(id==0){char json[8192],text[512];if(j47_r_string(&r,json,sizeof(json))){j47_json_collect_text(json,text,sizeof(text));j47_fail(text[0]?text:"Login rejected");}}
        else if(id==1){j47_fail("This Java server requires online-mode authentication");}
        else if(id==2){char uuid[64],name[64];if(j47_r_string(&r,uuid,sizeof(uuid))&&j47_r_string(&r,name,sizeof(name))){g_j47.local_profile_uuid_valid=j47_parse_uuid_text(uuid,g_j47.local_profile_uuid);g_j47.state=PEX_JAVA47_PLAY;g_j47.progress=25;j47_set_status("Logging in");}}
        else if(id==3){int32_t threshold;if(j47_r_varint(&r,&threshold))g_j47.compression_threshold=threshold;}
    }else if(g_j47.state==PEX_JAVA47_PLAY)j47_handle_play_packet(id,&r);
    if(r.failed&&g_j47.state!=PEX_JAVA47_FAILED)j47_fail("Malformed Java protocol packet");
}

static int j47_process_rx(void){
    size_t consumed=0;
    int packet_count=0;
    double started=now_seconds();
    while(consumed<g_j47.rx_len){int32_t frame_len=0;size_t prefix=0;int vr=j47_peek_varint(g_j47.rx+consumed,g_j47.rx_len-consumed,&frame_len,&prefix);if(vr==0)break;if(vr<0||frame_len<0||frame_len>(int)J47_PACKET_MAX){j47_fail("Invalid Java packet length");return 0;}if(g_j47.rx_len-consumed-prefix<(size_t)frame_len)break;
        const unsigned char*frame=g_j47.rx+consumed+prefix;size_t flen=(size_t)frame_len;
        if(g_j47.compression_threshold>=0){J47Reader rr;int32_t raw_len=0;j47_reader_init(&rr,frame,flen);if(!j47_r_varint(&rr,&raw_len)){j47_fail("Invalid compressed Java packet");return 0;}if(raw_len==0)j47_handle_packet(frame+rr.pos,flen-rr.pos);else{if(raw_len<g_j47.compression_threshold||raw_len>(int)J47_PACKET_MAX){j47_fail("Invalid Java decompressed length");return 0;}unsigned char*out=malloc((size_t)raw_len);if(!out){j47_fail("Out of memory decoding Java packet");return 0;}uLongf outlen=(uLongf)raw_len;int zr=uncompress(out,&outlen,frame+rr.pos,(uLong)(flen-rr.pos));if(zr!=Z_OK||outlen!=(uLongf)raw_len){free(out);j47_fail("Invalid Java compressed packet");return 0;}j47_handle_packet(out,(size_t)outlen);free(out);}}
        else j47_handle_packet(frame,flen);
        if(g_j47.state==PEX_JAVA47_FAILED) return 0;
        consumed += prefix + flen;
        packet_count++;
        /* A busy BedWars lobby can produce hundreds of entity/tab packets in
           one socket read. Processing the entire backlog before the game tick
           starves local movement, so bound play-state work per rendered frame
           while leaving the remaining complete frames buffered. */
        if(!g_j47.peer_closed && g_j47.state==PEX_JAVA47_PLAY &&
           (packet_count>=4096 || now_seconds()-started>=0.010))break;
    }
    if(consumed){memmove(g_j47.rx,g_j47.rx+consumed,g_j47.rx_len-consumed);g_j47.rx_len-=consumed;}return 1;
}


static void j47_draw_solid_box(float x0,float y0,float z0,float x1,float y1,float z1) {
    glBegin(GL_QUADS);
    glNormal3f(0,0,-1); glVertex3f(x1,y0,z0);glVertex3f(x0,y0,z0);glVertex3f(x0,y1,z0);glVertex3f(x1,y1,z0);
    glNormal3f(0,0, 1); glVertex3f(x0,y0,z1);glVertex3f(x1,y0,z1);glVertex3f(x1,y1,z1);glVertex3f(x0,y1,z1);
    glNormal3f(-1,0,0); glVertex3f(x0,y0,z0);glVertex3f(x0,y0,z1);glVertex3f(x0,y1,z1);glVertex3f(x0,y1,z0);
    glNormal3f( 1,0,0); glVertex3f(x1,y0,z1);glVertex3f(x1,y0,z0);glVertex3f(x1,y1,z0);glVertex3f(x1,y1,z1);
    glNormal3f(0, 1,0); glVertex3f(x0,y1,z1);glVertex3f(x1,y1,z1);glVertex3f(x1,y1,z0);glVertex3f(x0,y1,z0);
    glNormal3f(0,-1,0); glVertex3f(x0,y0,z0);glVertex3f(x1,y0,z0);glVertex3f(x1,y0,z1);glVertex3f(x0,y0,z1);
    glEnd();
}

static void j47_draw_armor_part(float cx,float cy,float cz,float sx,float sy,float sz,const float pose[3]) {
    glPushMatrix();
    glTranslatef(cx,cy,cz);
    glRotatef(pose[2],0,0,1);
    glRotatef(pose[1],0,1,0);
    glRotatef(pose[0],1,0,0);
    j47_draw_solid_box(-sx*.5f,-sy*.5f,-sz*.5f,sx*.5f,sy*.5f,sz*.5f);
    glPopMatrix();
}

static void j47_draw_armor_stand_name(const J47Entity *e,float label_y) {
    if (!e || !e->custom_name_visible || !e->custom_name[0] || !tex_font.id) return;
    float dx=(float)e->x-g_player_render_frame.x,dy=label_y-g_player_render_frame.y,dz=(float)e->z-g_player_render_frame.z;
    if (dx*dx+dy*dy+dz*dz > 64.0f*64.0f) return;
    int tw=text_width(e->custom_name);
    if(tw<=0)return;
    float view_yaw=0.0f,view_pitch=0.0f;
    multiplayer_name_tag_view_angles(&view_yaw,&view_pitch);
    float scale=(1.0f/60.0f)*1.6f;
    glPushMatrix();
    glTranslatef((float)e->x,label_y,(float)e->z);
    glRotatef(-view_yaw,0,1,0);glRotatef(view_pitch,1,0,0);glScalef(-scale,-scale,scale);
    glDepthMask(GL_FALSE);glDisable(GL_DEPTH_TEST);glDisable(GL_TEXTURE_2D);
    draw_rect(-tw/2-1,-1,tw/2+1,8,(int)0x40000000u);
    glEnable(GL_TEXTURE_2D);draw_text_no_shadow(e->custom_name,-tw/2,0,(int)0x20FFFFFFu);
    glEnable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);draw_text_no_shadow(e->custom_name,-tw/2,0,0xFFFFFF);
    glPopMatrix();
}

void pex_java47_draw_special_entities(float partial) {
    (void)partial;
    if(g_j47.state!=PEX_JAVA47_PLAY)return;
    glEnable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glDisable(GL_TEXTURE_2D);
    for(int i=0;i<J47_ENTITY_MAX;++i){
        J47Entity *e=&g_j47.entities[i];
        if(!e->used||!e->is_armor_stand)continue;
        int invisible=(e->metadata_flags&0x20)!=0;
        int small=(e->armor_stand_flags&0x01)!=0;
        int show_arms=(e->armor_stand_flags&0x04)!=0;
        int no_base=(e->armor_stand_flags&0x08)!=0;
        float scale=small?0.5f:1.0f;
        if(!invisible){
            glPushMatrix();
            glTranslatef((float)e->x,(float)e->y,(float)e->z);
            glRotatef(180.0f-e->yaw,0,1,0);
            glScalef(scale,scale,scale);
            glColor4f(0.58f,0.39f,0.20f,1.0f);
            if(!no_base)j47_draw_solid_box(-0.38f,0.0f,-0.38f,0.38f,0.06f,0.38f);
            j47_draw_solid_box(-0.035f,0.06f,-0.035f,0.035f,1.48f,0.035f);
            j47_draw_armor_part(0.0f,1.60f,0.0f,0.50f,0.50f,0.50f,e->armor_pose[0]);
            glPushMatrix();
            glTranslatef(0.0f,1.15f,0.0f);
            glRotatef(e->armor_pose[1][2],0,0,1);glRotatef(e->armor_pose[1][1],0,1,0);glRotatef(e->armor_pose[1][0],1,0,0);
            j47_draw_solid_box(-0.30f,0.24f,-0.035f,0.30f,0.31f,0.035f);
            j47_draw_solid_box(-0.22f,-0.31f,-0.035f,-0.15f,0.28f,0.035f);
            j47_draw_solid_box( 0.15f,-0.31f,-0.035f, 0.22f,0.28f,0.035f);
            j47_draw_solid_box(-0.23f,-0.35f,-0.055f,0.23f,-0.28f,0.055f);
            glPopMatrix();
            j47_draw_armor_part(-0.12f,0.52f,0.0f,0.10f,1.02f,0.10f,e->armor_pose[4]);
            j47_draw_armor_part( 0.12f,0.52f,0.0f,0.10f,1.02f,0.10f,e->armor_pose[5]);
            if(show_arms){
                j47_draw_armor_part(-0.36f,1.10f,0.0f,0.10f,0.88f,0.10f,e->armor_pose[2]);
                j47_draw_armor_part( 0.36f,1.10f,0.0f,0.10f,0.88f,0.10f,e->armor_pose[3]);
            }
            glPopMatrix();
        }
        glEnable(GL_TEXTURE_2D);
        j47_draw_armor_stand_name(e,(float)e->y+(small?1.25f:2.18f));
        glDisable(GL_TEXTURE_2D);
    }
    glEnable(GL_TEXTURE_2D);glEnable(GL_CULL_FACE);glColor4f(1,1,1,1);
}


static int j47_ascii_casecmp(const char *a,const char *b) {
    if(!a)a="";if(!b)b="";
    while(*a&&*b){unsigned char ca=(unsigned char)*a++,cb=(unsigned char)*b++;if(ca>='A'&&ca<='Z')ca=(unsigned char)(ca+32);if(cb>='A'&&cb<='Z')cb=(unsigned char)(cb+32);if(ca!=cb)return (int)ca-(int)cb;}
    return (unsigned char)*a-(unsigned char)*b;
}

typedef struct J47TabSortEntry {
    J47Profile *profile;
    J47Team *team;
} J47TabSortEntry;

static int j47_tab_compare(const void *ap,const void *bp) {
    const J47TabSortEntry *a=(const J47TabSortEntry*)ap,*b=(const J47TabSortEntry*)bp;
    int aspec=a->profile->game_mode==3,bspec=b->profile->game_mode==3;
    if(aspec!=bspec)return aspec-bspec;
    const char *at=a->team?a->team->name:"",*bt=b->team?b->team->name:"";
    int c=strcmp(at,bt);if(c)return c;
    return strcmp(a->profile->name,b->profile->name);
}

static int j47_copy_active_formats(const char *text, char *out, size_t cap) {
    char color = 0;
    int obfuscated=0,bold=0,strike=0,underline=0,italic=0;
    const unsigned char *p=(const unsigned char*)(text?text:"");
    if(!out||cap==0)return 0;
    out[0]=0;
    while(*p){
        if(p[0]==0xC2u&&p[1]==0xA7u&&p[2]){
            char c=(char)p[2];if(c>='A'&&c<='Z')c=(char)(c+32);
            if((c>='0'&&c<='9')||(c>='a'&&c<='f')){color=c;obfuscated=bold=strike=underline=italic=0;}
            else if(c=='k')obfuscated=1;else if(c=='l')bold=1;else if(c=='m')strike=1;
            else if(c=='n')underline=1;else if(c=='o')italic=1;
            else if(c=='r'){color=0;obfuscated=bold=strike=underline=italic=0;}
            p+=3;continue;
        }
        ++p;
    }
    size_t n=0;
#define J47_FMT_APPEND(code) do{if(n+3<cap){out[n++]=(char)0xC2;out[n++]=(char)0xA7;out[n++]=(code);out[n]=0;}}while(0)
    if(color)J47_FMT_APPEND(color);
    if(obfuscated)J47_FMT_APPEND('k');
    if(bold)J47_FMT_APPEND('l');
    if(strike)J47_FMT_APPEND('m');
    if(underline)J47_FMT_APPEND('n');
    if(italic)J47_FMT_APPEND('o');
#undef J47_FMT_APPEND
    return (int)n;
}

static int j47_utf8_atom_bytes(const unsigned char *p) {
    if(!p||!*p)return 0;
    if(p[0]==0xC2u&&p[1]==0xA7u&&p[2])return 3;
    if(p[0]<0x80u)return 1;
    if((p[0]&0xE0u)==0xC0u&&p[1])return 2;
    if((p[0]&0xF0u)==0xE0u&&p[1]&&p[2])return 3;
    if((p[0]&0xF8u)==0xF0u&&p[1]&&p[2]&&p[3])return 4;
    return 1;
}

static int j47_wrap_formatted_width(const char *text,int max_width,char lines[][512],int max_lines) {
    if(!text||!text[0]||max_lines<=0)return 0;
    const unsigned char *p=(const unsigned char*)text;
    int line=0,pos=0;
    lines[0][0]=0;
    while(*p&&line<max_lines){
        if(*p=='\r'){++p;continue;}
        if(*p=='\n'){
            lines[line][pos]=0;
            ++line;
            ++p;
            if(line>=max_lines)break;
            pos=j47_copy_active_formats(lines[line-1],lines[line],sizeof(lines[line]));
            continue;
        }
        int atom=j47_utf8_atom_bytes(p);
        if(atom<=0)break;
        if(pos+atom>=(int)sizeof(lines[0])){
            lines[line][pos]=0;++line;if(line>=max_lines)break;
            pos=j47_copy_active_formats(lines[line-1],lines[line],sizeof(lines[line]));
        }
        int before=pos;
        memcpy(lines[line]+pos,p,(size_t)atom);pos+=atom;lines[line][pos]=0;
        if(max_width>0&&text_width(lines[line])>max_width&&before>0){
            pos=before;lines[line][pos]=0;
            char active[64];j47_copy_active_formats(lines[line],active,sizeof(active));
            ++line;if(line>=max_lines)break;
            snprintf(lines[line],sizeof(lines[line]),"%s",active);pos=(int)strlen(lines[line]);
            if(!(*p==' '||*p=='\t')){
                if(pos+atom<(int)sizeof(lines[line])){memcpy(lines[line]+pos,p,(size_t)atom);pos+=atom;lines[line][pos]=0;}
            }
        }
        p+=atom;
    }
    if(line<max_lines&&(pos>0||line==0)){lines[line][pos]=0;++line;}
    return line;
}

static int j47_score_ptr_compare(const void *ap,const void *bp) {
    const J47ScoreEntry *a=*(J47ScoreEntry *const*)ap,*b=*(J47ScoreEntry *const*)bp;
    if(a->value<b->value)return -1;
    if(a->value>b->value)return 1;
    return -j47_ascii_casecmp(a->owner,b->owner);
}

void pex_java47_draw_server_scoreboard(void) {
    if(g_j47.state!=PEX_JAVA47_PLAY||g_screen!=SCREEN_INGAME)return;
    int slot=1;
    J47Team *local_team=j47_team_for_member(g_j47.username);
    if(local_team&&local_team->color>=0&&local_team->color<16&&g_j47.display_objectives[3+local_team->color][0])
        slot=3+local_team->color;
    const char *objective_name=g_j47.display_objectives[slot];
    if(!objective_name[0])return;
    J47ScoreObjective *objective=j47_objective_find(objective_name);
    if(!objective)return;

    J47ScoreEntry *all[J47_SCORE_ENTRY_MAX];int all_count=0;
    for(int i=0;i<J47_SCORE_ENTRY_MAX;++i){
        J47ScoreEntry *e=&g_j47.scores[i];
        if(e->used&&!strcmp(e->objective,objective_name)&&e->owner[0]!='#')all[all_count++]=e;
    }
    if(all_count<=0)return;
    qsort(all,(size_t)all_count,sizeof(all[0]),j47_score_ptr_compare);
    int first=all_count>15?all_count-15:0;
    int total=all_count-first;
    int width=text_width(objective->display_name);
    char names[15][192],values[15][32],measure[256];
    for(int i=0;i<total;++i){
        J47ScoreEntry *e=all[first+i];
        j47_format_team_name(e->owner,names[i],sizeof(names[i]));
        snprintf(values[i],sizeof(values[i]),"\xC2\xA7" "c%d",e->value);
        snprintf(measure,sizeof(measure),"%s: %s",names[i],values[i]);
        int w=text_width(measure);if(w>width)width=w;
    }
    int font_h=9;
    int content_h=total*font_h;
    int base_y=g_gui_h/2+content_h/3;
    int right=g_gui_w-1;
    int left=g_gui_w-width-3;
    for(int i=0;i<total;++i){
        int row=i+1;
        int y=base_y-row*font_h;
        draw_rect(left-2,y,right,y+font_h,(int)0x50000000u);
        draw_text_no_shadow(names[i],left,y,(int)0xFFFFFFFFu);
        draw_text_no_shadow(values[i],right-text_width(values[i]),y,(int)0xFFFFFFFFu);
        if(row==total){
            int title_y=y-font_h;
            draw_rect(left-2,title_y-1,right,y-1,(int)0x60000000u);
            draw_rect(left-2,y-1,right,y,(int)0x50000000u);
            draw_text_no_shadow(objective->display_name,left+width/2-text_width(objective->display_name)/2,title_y,(int)0xFFFFFFFFu);
        }
    }
}

void pex_java47_draw_tab_overlay(void) {
    if(g_j47.state!=PEX_JAVA47_PLAY||g_screen!=SCREEN_INGAME||!key_down_vk(VK_TAB))return;
    J47TabSortEntry entries[J47_PROFILE_MAX];int count=0;
    for(int i=0;i<J47_PROFILE_MAX;++i){
        J47Profile *p=&g_j47.profiles[i];
        if(!p->used||!p->listed||!p->name[0])continue;
        entries[count].profile=p;entries[count].team=j47_team_for_member(p->name);++count;
    }
    if(count<=0)return;
    qsort(entries,(size_t)count,sizeof(entries[0]),j47_tab_compare);
    if(count>80)count=80;

    J47ScoreObjective *list_objective=NULL;
    if(g_j47.display_objectives[0][0])list_objective=j47_objective_find(g_j47.display_objectives[0]);
    char names[80][192],scores[80][40];
    int max_name=0,max_score=0;
    for(int i=0;i<count;++i){
        J47Profile *p=entries[i].profile;
        if(p->display_name[0])snprintf(names[i],sizeof(names[i]),"%s",p->display_name);
        else j47_format_team_name(p->name,names[i],sizeof(names[i]));
        if(p->game_mode==3){
            char tmp[192];snprintf(tmp,sizeof(tmp),"\xC2\xA7" "o%s",names[i]);snprintf(names[i],sizeof(names[i]),"%s",tmp);
        }
        scores[i][0]=0;
        if(list_objective&&p->game_mode!=3){
            J47ScoreEntry *score=j47_score_find(p->name,list_objective->name);
            if(score){
                if(list_objective->render_hearts)snprintf(scores[i],sizeof(scores[i]),"\xC2\xA7" "c%d\xE2\x99\xA5",(score->value+1)/2);
                else snprintf(scores[i],sizeof(scores[i])," %d",score->value);
            }
        }
        int nw=text_width(names[i]);if(nw>max_name)max_name=nw;
        int sw=text_width(scores[i]);if(sw>max_score)max_score=sw;
    }

    int columns=1,rows=count;
    while(rows>20){++columns;rows=(count+columns-1)/columns;}
    if(list_objective&&list_objective->render_hearts)max_score=90;
    int desired=max_name+max_score+13;
    int col_width=(columns*desired<g_gui_w-50?columns*desired:g_gui_w-50)/columns;
    if(col_width<32)col_width=32;
    int list_width=col_width*columns+(columns-1)*5;
    int list_left=g_gui_w/2-list_width/2;

    char header[32][512],footer[32][512];
    int hn=j47_wrap_formatted_width(g_j47.tab_header,g_gui_w-50,header,32);
    int fn=j47_wrap_formatted_width(g_j47.tab_footer,g_gui_w-50,footer,32);
    int panel_width=list_width;
    for(int i=0;i<hn;++i){int w=text_width(header[i]);if(w>panel_width)panel_width=w;}
    for(int i=0;i<fn;++i){int w=text_width(footer[i]);if(w>panel_width)panel_width=w;}
    int panel_left=g_gui_w/2-panel_width/2-1,panel_right=g_gui_w/2+panel_width/2+1;
    int y=10;
    if(hn>0){
        draw_rect(panel_left,y-1,panel_right,y+hn*9,(int)0x80000000u);
        for(int i=0;i<hn;++i){draw_centered_text(header[i],g_gui_w/2,y,0xFFFFFF);y+=9;}
        ++y;
    }
    draw_rect(panel_left,y-1,panel_right,y+rows*9,(int)0x80000000u);
    for(int i=0;i<count;++i){
        int col=i/rows,row=i%rows;
        int x=list_left+col*(col_width+5),py=y+row*9;
        draw_rect(x,py,x+col_width,py+8,(int)0x20FFFFFFu);
        int name_color=entries[i].profile->game_mode==3?(int)0x90FFFFFFu:0xFFFFFF;
        draw_text(names[i],x,py,name_color);
        if(scores[i][0]){
            int sx=x+col_width-12-text_width(scores[i]);
            if(sx>x+max_name+1)draw_text(scores[i],sx,py,0xFFFFFF);
        }
        int ping=entries[i].profile->ping;
        int level=ping<0?5:ping<150?0:ping<300?1:ping<600?2:ping<1000?3:4;
        int active=level==5?0:5-level;
        int bx=x+col_width-10;
        for(int b=0;b<5;++b){int bh=2+b;draw_rect(bx+b*2,py+8-bh,bx+b*2+1,py+8,b<active?0x55FF55:0x555555);}
    }
    y+=rows*9+1;
    if(fn>0){
        draw_rect(panel_left,y-1,panel_right,y+fn*9,(int)0x80000000u);
        for(int i=0;i<fn;++i){draw_centered_text(footer[i],g_gui_w/2,y,0xFFFFFF);y+=9;}
    }
}

int pex_java47_begin_join(const char *host,int port,const char *username){
    pex_java47_disconnect();
    g_player_capabilities_server_authoritative=1;
    memset(&g_j47,0,sizeof(g_j47));g_j47.socket=INVALID_SOCKET;g_j47.compression_threshold=-1;g_j47.server_protocol=PEX_JAVA47_PROTOCOL_VERSION;g_j47.state=PEX_JAVA47_CONNECTING;g_j47.active=1;g_j47.port=port>0?port:PEX_JAVA47_DEFAULT_PORT;g_j47.progress=5;g_j47.last_abilities_flags=-1;g_j47.server_movement_speed_scale=1.0f;g_j47.daylight_cycle=1;g_j47.last_movement_game_tick=-1;g_j47.last_world_origin_x=g_flat_world_origin_x;g_j47.last_world_origin_z=g_flat_world_origin_z;
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
    j47_skin_pump();
    if(!j47_flush_tx())return 0;
    if(!j47_receive_available())return 0;
    if(!j47_process_rx())return 0;
    if (g_j47.last_world_origin_x != g_flat_world_origin_x ||
        g_j47.last_world_origin_z != g_flat_world_origin_z) {
        g_j47.last_world_origin_x = g_flat_world_origin_x;
        g_j47.last_world_origin_z = g_flat_world_origin_z;
        j47_reapply_cached_block_updates();
    }
    if(g_j47.peer_closed){
        /* If a valid disconnect packet was buffered, j47_process_rx() has
           already converted it into the real server-provided reason.  Reach
           this fallback only when the peer closed without such a packet. */
        if(g_j47.rx_len) j47_fail("Java server closed with a truncated packet");
        else j47_fail("Java server closed the connection without a disconnect reason");
        return 0;
    }
    return j47_flush_tx();
}

void pex_java47_disconnect(void){
    g_player_capabilities_server_authoritative=0;
    if(g_j47.socket!=INVALID_SOCKET)closesocket(g_j47.socket);
    j47_clear_remote_entities();
    free_texture(&g_j47_local_skin);
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
int pex_java47_daylight_cycle_enabled(void){return g_j47.daylight_cycle;}
int pex_java47_local_entity_id(void){return g_j47.local_entity_id;}
float pex_java47_movement_speed_scale(void){return g_j47.server_movement_speed_scale>0.0f?g_j47.server_movement_speed_scale:1.0f;}
int pex_java47_get_equipment(int entity_id,int *item_id,int *count,int *damage,int *slot){J47Entity*e=j47_entity_find(entity_id);if(!e)return 0;if(item_id)*item_id=e->held_item_id;if(count)*count=e->held_item_count;if(damage)*damage=e->held_item_damage;if(slot)*slot=e->held_slot;return 1;}
int pex_java47_entity_is_player(int entity_id){J47Entity*e=j47_entity_find(entity_id);return e&&e->kind==J47_ENTITY_PLAYER;}
Texture *pex_java47_local_skin_texture(void){return g_j47_local_skin.id?&g_j47_local_skin:&tex_steve;}

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

static void j47_tick_server_projectiles(void) {
    for (int i = 0; i < J47_ENTITY_MAX; ++i) {
        J47Entity *e = &g_j47.entities[i];
        if (!e->used || e->projectile_slot < 0 || e->projectile_slot >= MAX_PROJECTILE_ENTITIES) continue;
        FlatProjectile *p = &g_projectiles[e->projectile_slot];
        if (!p->active || p->owner_type != 2) continue;
        p->prev_x=p->x; p->prev_y=p->y; p->prev_z=p->z;
        p->prev_yaw=p->yaw; p->prev_pitch=p->pitch;
        p->x += p->mx; p->y += p->my; p->z += p->mz;
        p->mx = (p->mx + e->projectile_ax) * 0.95f;
        p->my = (p->my + e->projectile_ay) * 0.95f;
        p->mz = (p->mz + e->projectile_az) * 0.95f;
        p->fire_ticks = 2;
        pex_projectile_update_rotation_125(p);
        e->x=p->x; e->y=p->y; e->z=p->z;
        add_smoke_particle(p->x,p->y+0.5f,p->z,0.0f,0.0f,0.0f,1.0f);
    }
}

int pex_java47_send_player_state(double x,double feet_y,double z,float yaw,float pitch,int on_ground,int sneaking,int sprinting,int held_slot){
    if(g_j47.state!=PEX_JAVA47_PLAY||!g_j47.position_received)return 0;
    if(!isfinite(x)||!isfinite(feet_y)||!isfinite(z)||!isfinite(yaw)||!isfinite(pitch))return 1;
    flat_multiplayer_recenter_world((float)x,(float)z,0);
    if(held_slot<0||held_slot>8) held_slot=0;
    if(g_j47.last_held_slot!=held_slot){J47Writer h;j47_writer_init(&h,4);j47_w_be16(&h,held_slot);j47_send_packet(0x09,h.data,h.len);j47_writer_free(&h);g_j47.last_held_slot=held_slot;}
    if(g_j47.last_sneaking!=sneaking){j47_send_entity_action(sneaking?0:1);g_j47.last_sneaking=sneaking;}
    if(g_j47.last_sprinting!=sprinting){j47_send_entity_action(sprinting?3:4);g_j47.last_sprinting=sprinting;}
    /* C13 is not a heartbeat.  Vanilla sends it only when an allowed-flight
       client changes its flying bit.  Repeated or unsolicited ability packets
       are unusual enough that stricter servers can treat them as movement
       manipulation. Preserve every server-assigned capability bit and change
       only the flying flag. */
    if(g_j47.server_abilities_flags&4){
        int ability_flags=g_j47.server_abilities_flags;
        if(g_creative_flying)ability_flags|=2;else ability_flags&=~2;
        if(g_j47.last_abilities_flags!=ability_flags){
            J47Writer a;j47_writer_init(&a,16);
            j47_w_u8(&a,ability_flags);
            j47_w_float(&a,g_j47.server_fly_speed>0.0f?g_j47.server_fly_speed:0.05f);
            j47_w_float(&a,g_j47.server_walk_speed>0.0f?g_j47.server_walk_speed:0.1f);
            j47_send_packet(0x13,a.data,a.len);
            j47_writer_free(&a);
            g_j47.last_abilities_flags=ability_flags;
        }
    }

    /* EntityPlayerSP sends one movement packet per client tick. The previous
       wall-clock limiter could emit irregular bursts when the simulation and
       render threads called this path in the same frame, which looks like
       impossible movement to stricter servers. */
    if(g_j47.last_movement_game_tick==g_ingame_ticks)return 1;
    g_j47.last_movement_game_tick=g_ingame_ticks;
    j47_tick_server_projectiles();

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
    /* Preserve the exact vanilla packet stream.  The previous newest-only
       pending slot dropped intermediate 20 Hz positions whenever send() was
       briefly back-pressured.  A normal server tolerates the resulting larger
       deltas, but movement checks see them as packet skipping/speed spikes.
       Queue every C03/C04/C05/C06 in order and let TCP coalesce bytes, never
       semantic movement states. */
    int ok=j47_flush_tx()&&j47_send_packet(pid,w.data,w.len)&&j47_flush_tx();
    j47_writer_free(&w);
    if(!ok)return 0;
    g_j47.movement_ticks++;
    if(moved){g_j47.last_x=x;g_j47.last_y=feet_y;g_j47.last_z=z;g_j47.movement_ticks=0;}
    if(looked){g_j47.last_yaw=yaw;g_j47.last_pitch=pitch;}
    g_j47.last_on_ground=on_ground;
    return 1;
}

int pex_java47_send_chat(const char*text){if(g_j47.state!=PEX_JAVA47_PLAY||!text)return 0;J47Writer w;j47_writer_init(&w,300);j47_w_string_n(&w,text,100);int ok=j47_send_packet(0x01,w.data,w.len);j47_writer_free(&w);return ok;}
int pex_java47_send_swing(void){if(g_j47.state!=PEX_JAVA47_PLAY)return 0;return j47_send_packet(0x0A,NULL,0);}
int pex_java47_send_attack(int entity_id){if(g_j47.state!=PEX_JAVA47_PLAY)return 0;J47Writer w;j47_writer_init(&w,16);j47_w_varint(&w,entity_id);j47_w_varint(&w,1);int ok=j47_send_packet(0x02,w.data,w.len);j47_writer_free(&w);return ok;}

static int j47_send_use_entity_action(int entity_id,int action,float hit_x,float hit_y,float hit_z){
    if(g_j47.state!=PEX_JAVA47_PLAY)return 0;
    J47Writer w;j47_writer_init(&w,32);
    j47_w_varint(&w,entity_id);j47_w_varint(&w,action);
    if(action==2){j47_w_float(&w,hit_x);j47_w_float(&w,hit_y);j47_w_float(&w,hit_z);}
    int ok=j47_send_packet(0x02,w.data,w.len);j47_writer_free(&w);
    if(ok&&g_j47.socket!=INVALID_SOCKET)ok=j47_flush_tx();
    return ok;
}

int pex_java47_send_interact(int entity_id,float hit_x,float hit_y,float hit_z){
    (void)hit_x;(void)hit_y;(void)hit_z;
    /* Villagers, fake players and ordinary NPCs use the vanilla INTERACT
       action. Sending INTERACT_AT first can be cancelled by NPC plugins or
       duplicate their click event, so reserve the precise action for actual
       armor stands. */
    return j47_send_use_entity_action(entity_id,0,0.0f,0.0f,0.0f);
}

static int j47_send_armor_stand_interact(int entity_id,float hit_x,float hit_y,float hit_z){
    /* EntityArmorStand implements interactAt in 1.8.8. Send the precise hit
       first, then the generic action as a compatibility fallback for shop
       plugins that listen only for PlayerInteractEntityEvent. */
    if(!j47_send_use_entity_action(entity_id,2,hit_x,hit_y,hit_z))return 0;
    return j47_send_use_entity_action(entity_id,0,0.0f,0.0f,0.0f);
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
    if(max_dist<=0.0f)max_dist=player_is_creative()?5.0f:4.5f;

    float look_x,look_y,look_z;
    pex_touch_aware_look_vector(&look_x,&look_y,&look_z);
    float origin[3]={g_player_x,g_player_y,g_player_z};
    float dir[3]={look_x,look_y,look_z};
    float best=max_dist;
    float best_fallback=max_dist;
    J47Entity *best_entity=NULL,*fallback_entity=NULL;

    for(int i=0;i<J47_ENTITY_MAX;i++){
        J47Entity *e=&g_j47.entities[i];
        if(!e->used||e->entity_id==g_j47.local_entity_id)continue;
        float width=0.75f,height=0.75f,feet_y=(float)e->y;
        if(e->kind==J47_ENTITY_PLAYER){width=0.7f;height=1.8f;}
        else if(e->is_armor_stand){
            int small=(e->armor_stand_flags&0x01)!=0;
            int marker=(e->armor_stand_flags&0x10)!=0;
            width=small?0.35f:0.65f;height=small?1.15f:2.05f;
            if(marker){width=0.90f;height=small?1.35f:2.25f;}
        } else if(e->kind==J47_ENTITY_MOB&&e->mob_slot>=0&&e->mob_slot<MAX_PASSIVE_MOBS){
            PassiveMob *m=&g_passive_mobs[e->mob_slot];
            if(!m->active||m->death_time>0)continue;
            width=m->width>0.1f?m->width:0.9f;height=m->height>0.1f?m->height:0.9f;feet_y=m->y;
        } else if(e->kind==J47_ENTITY_DROP&&e->drop_slot>=0&&e->drop_slot<MAX_DROP_ENTITIES){
            FlatDroppedItem *d=&g_drops[e->drop_slot];
            if(!d->active)continue;
            width=0.55f;height=0.55f;feet_y=d->y-0.15f;
        } else if(e->projectile_slot>=0) { width=1.0f;height=1.0f;feet_y=(float)e->y-0.5f; }

        /* Use a generous collision-border expansion for server NPCs. Plugin
           entities are often invisible, tiny, inside a slab, or represented by
           a pig while their clickable point sits slightly away from the model. */
        float half=width*0.5f+0.18f;
        float mn[3]={(float)e->x-half,feet_y-0.18f,(float)e->z-half};
        float mx[3]={(float)e->x+half,feet_y+height+0.18f,(float)e->z+half};
        float t=best;
        if(j47_ray_aabb(origin,dir,mn,mx,best,&t)&&t<best){best=t;best_entity=e;}

        /* Final compatibility cone: if the crosshair is very close to an NPC
           model, still send C02 instead of falling through to item use. This is
           intentionally interaction-only and does not change collision. */
        float cx=(float)e->x,cy=feet_y+height*0.5f,cz=(float)e->z;
        float vx=cx-origin[0],vy=cy-origin[1],vz=cz-origin[2];
        float along=vx*dir[0]+vy*dir[1]+vz*dir[2];
        if(along>0.0f&&along<=max_dist){
            float d2=vx*vx+vy*vy+vz*vz;
            float off2=d2-along*along;
            float radius=half+0.30f;
            if(off2<=radius*radius&&along<best_fallback){best_fallback=along;fallback_entity=e;}
        }
    }

    if(!best_entity)best_entity=fallback_entity;
    if(!best_entity)return 0;
    float use_t=best_entity==fallback_entity?best_fallback:best;
    float hit_x=origin[0]+dir[0]*use_t-(float)best_entity->x;
    float hit_y=origin[1]+dir[1]*use_t-(float)best_entity->y;
    float hit_z=origin[2]+dir[2]*use_t-(float)best_entity->z;
    if(best_entity->is_armor_stand)
        return j47_send_armor_stand_interact(best_entity->entity_id,hit_x,hit_y,hit_z)?1:0;
    /* Every other server entity—including pig NPCs, dropped menu icons and
       generic object proxies—gets a normal right-click INTERACT packet. */
    return pex_java47_send_interact(best_entity->entity_id,hit_x,hit_y,hit_z)?1:0;
}
