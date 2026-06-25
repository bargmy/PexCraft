/* Split from original monolithic main.c. Included by src/main.c unity build. */

static unsigned char *g_mp_recv_buf = NULL;
static size_t g_mp_recv_len = 0;
static size_t g_mp_recv_cap = 0;
static int g_mp_pending_pickups[PEX_NET_MAX_DROPS];
static SOCKET g_mp_connect_socket = INVALID_SOCKET;
static double g_mp_connect_start_time = 0.0;
static double g_mp_last_chunk_request_time = 0.0;
static int g_mp_last_request_cx = 999999;
static int g_mp_last_request_cz = 999999;
static unsigned short g_mp_chunk_section_recv_mask[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
static unsigned int g_mp_chunk_hash_cache[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
static int g_mp_chunk_cache_loaded[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
static int g_mp_pending_respawn_sync = 0;
static HANDLE g_mp_connect_thread = NULL;
static volatile LONG g_mp_connect_thread_done = 0;
static volatile LONG g_mp_connect_thread_ok = 0;
static SOCKET g_mp_connect_thread_socket = INVALID_SOCKET;
static char g_mp_connect_thread_error[128] = "";
static char g_mp_connect_thread_host[96] = "";
static int g_mp_connect_thread_port = PEX_NET_DEFAULT_PORT;

#define PEX_MP_PACKET_QUEUE_MAX 384
typedef struct PexMpQueuedPacket {
    uint16_t type;
    uint32_t size;
    unsigned char *data;
} PexMpQueuedPacket;

static CRITICAL_SECTION g_mp_rx_cs;
static int g_mp_rx_initialized = 0;
static HANDLE g_mp_rx_thread = NULL;
static volatile LONG g_mp_rx_stop = 0;
static volatile LONG g_mp_rx_lost = 0;
static char g_mp_rx_error[128] = "";
static PexMpQueuedPacket g_mp_rx_packets[PEX_MP_PACKET_QUEUE_MAX];
static int g_mp_rx_head = 0;
static int g_mp_rx_tail = 0;
static int g_mp_rx_count = 0;


static int net_error_is_would_block(void);

static void pex_mp_rx_packet_free(PexMpQueuedPacket *p) {
    if (!p) return;
    if (p->data) free(p->data);
    memset(p, 0, sizeof(*p));
}

static void pex_mp_rx_queue_init(void) {
    if (g_mp_rx_initialized) return;
    InitializeCriticalSection(&g_mp_rx_cs);
    g_mp_rx_initialized = 1;
}

static void pex_mp_rx_queue_clear(void) {
    if (!g_mp_rx_initialized) return;
    EnterCriticalSection(&g_mp_rx_cs);
    for (int i = 0; i < PEX_MP_PACKET_QUEUE_MAX; i++) pex_mp_rx_packet_free(&g_mp_rx_packets[i]);
    g_mp_rx_head = g_mp_rx_tail = g_mp_rx_count = 0;
    LeaveCriticalSection(&g_mp_rx_cs);
}

static int pex_mp_rx_queue_push(uint16_t type, const void *payload, uint32_t size) {
    pex_mp_rx_queue_init();
    unsigned char *copy = NULL;
    if (size > 0) {
        copy = (unsigned char *)malloc(size);
        if (!copy) return 0;
        memcpy(copy, payload, size);
    }

    for (;;) {
        if (InterlockedCompareExchange(&g_mp_rx_stop, 0, 0)) {
            if (copy) free(copy);
            return 0;
        }
        EnterCriticalSection(&g_mp_rx_cs);
        if (g_mp_rx_count < PEX_MP_PACKET_QUEUE_MAX) {
            PexMpQueuedPacket *p = &g_mp_rx_packets[g_mp_rx_tail];
            pex_mp_rx_packet_free(p);
            p->type = type;
            p->size = size;
            p->data = copy;
            g_mp_rx_tail = (g_mp_rx_tail + 1) % PEX_MP_PACKET_QUEUE_MAX;
            g_mp_rx_count++;
            LeaveCriticalSection(&g_mp_rx_cs);
            return 1;
        }
        LeaveCriticalSection(&g_mp_rx_cs);
        Sleep(1);
    }
}

static int pex_mp_rx_queue_pop(PexMpQueuedPacket *out) {
    if (!out || !g_mp_rx_initialized) return 0;
    memset(out, 0, sizeof(*out));
    EnterCriticalSection(&g_mp_rx_cs);
    if (g_mp_rx_count > 0) {
        *out = g_mp_rx_packets[g_mp_rx_head];
        memset(&g_mp_rx_packets[g_mp_rx_head], 0, sizeof(g_mp_rx_packets[0]));
        g_mp_rx_head = (g_mp_rx_head + 1) % PEX_MP_PACKET_QUEUE_MAX;
        g_mp_rx_count--;
        LeaveCriticalSection(&g_mp_rx_cs);
        return 1;
    }
    LeaveCriticalSection(&g_mp_rx_cs);
    return 0;
}

static int pex_mp_rx_reserve(unsigned char **buf, size_t *cap, size_t need) {
    if (!buf || !cap) return 0;
    if (need <= *cap) return 1;
    size_t ncap = *cap ? *cap : 65536;
    while (ncap < need) ncap *= 2;
    unsigned char *nbuf = (unsigned char *)realloc(*buf, ncap);
    if (!nbuf) return 0;
    *buf = nbuf;
    *cap = ncap;
    return 1;
}

static DWORD WINAPI pex_net_rx_worker(LPVOID unused) {
    (void)unused;
    unsigned char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;

    for (;;) {
        if (InterlockedCompareExchange(&g_mp_rx_stop, 0, 0)) break;
        if (g_mp_socket == INVALID_SOCKET) break;
        if (!pex_mp_rx_reserve(&buf, &cap, len + 32768)) {
            snprintf(g_mp_rx_error, sizeof(g_mp_rx_error), "Out of memory while reading server data.");
            InterlockedExchange(&g_mp_rx_lost, 1);
            break;
        }

        int n = recv(g_mp_socket, (char *)buf + len, 32768, 0);
        if (n == 0) {
            snprintf(g_mp_rx_error, sizeof(g_mp_rx_error), "Connection to server was lost.");
            InterlockedExchange(&g_mp_rx_lost, 1);
            break;
        }
        if (n < 0) {
            if (net_error_is_would_block()) {
                Sleep(1);
                continue;
            }
            snprintf(g_mp_rx_error, sizeof(g_mp_rx_error), "Connection to server was lost.");
            InterlockedExchange(&g_mp_rx_lost, 1);
            break;
        }
        len += (size_t)n;

        size_t off = 0;
        while (len - off >= sizeof(PexNetPacketHeader)) {
            PexNetPacketHeader h;
            memcpy(&h, buf + off, sizeof(h));
            if (h.size > 1024 * 1024) {
                snprintf(g_mp_rx_error, sizeof(g_mp_rx_error), "Server sent an oversized packet.");
                InterlockedExchange(&g_mp_rx_lost, 1);
                off = len;
                break;
            }
            size_t packet_size = sizeof(PexNetPacketHeader) + (size_t)h.size;
            if (len - off < packet_size) break;
            if (!pex_mp_rx_queue_push(h.type, buf + off + sizeof(PexNetPacketHeader), h.size)) {
                off = len;
                break;
            }
            off += packet_size;
        }
        if (off > 0 && off < len) memmove(buf, buf + off, len - off);
        if (off > 0) len -= off;
    }

    if (buf) free(buf);
    return 0;
}

static void pex_net_stop_rx_thread(void) {
    InterlockedExchange(&g_mp_rx_stop, 1);
    if (g_mp_rx_thread) {
        WaitForSingleObject(g_mp_rx_thread, 2000);
        CloseHandle(g_mp_rx_thread);
        g_mp_rx_thread = NULL;
    }
    pex_mp_rx_queue_clear();
}

static int pex_net_start_rx_thread(void) {
    pex_net_stop_rx_thread();
    pex_mp_rx_queue_init();
    pex_mp_rx_queue_clear();
    g_mp_rx_error[0] = 0;
    InterlockedExchange(&g_mp_rx_lost, 0);
    InterlockedExchange(&g_mp_rx_stop, 0);
    g_mp_rx_thread = CreateThread(NULL, 0, pex_net_rx_worker, NULL, 0, NULL);
    if (!g_mp_rx_thread) {
        snprintf(g_mp_rx_error, sizeof(g_mp_rx_error), "Could not start network thread.");
        return 0;
    }
    SetThreadPriority(g_mp_rx_thread, THREAD_PRIORITY_ABOVE_NORMAL);
    return 1;
}

static void pex_net_free_render_player(PexNetRenderPlayerState *r) {
    if (!r) return;
    if (r->has_skin || r->skin.id || r->skin.rgba) free_texture(&r->skin);
    memset(r, 0, sizeof(*r));
}

static void net_free_render_players(void) {
    for (int i = 0; i < PEX_NET_MAX_PLAYERS; i++) {
        pex_net_free_render_player(&g_mp_render_players[i]);
    }
}

static float pex_net_smooth_step(float dt, float sharpness) {
    if (dt <= 0.0f) return 0.0f;
    float a = 1.0f - expf(-sharpness * dt);
    if (a < 0.0f) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    return a;
}

static float pex_net_angle_delta(float from, float to) {
    float d = to - from;
    while (d < -180.0f) d += 360.0f;
    while (d >= 180.0f) d -= 360.0f;
    return d;
}

static PexNetRenderPlayerState *pex_net_find_render_player(int player_id) {
    if (player_id <= 0) return NULL;
    for (int i = 0; i < PEX_NET_MAX_PLAYERS; i++) {
        if (g_mp_render_players[i].active && g_mp_render_players[i].player_id == player_id) return &g_mp_render_players[i];
    }
    return NULL;
}

static const PexNetPlayerState *pex_net_find_player_state(int player_id) {
    if (player_id <= 0) return NULL;
    for (int i = 0; i < g_mp_player_count; i++) {
        if (g_mp_players[i].player_id == player_id) return &g_mp_players[i];
    }
    return NULL;
}

static const PexNetPlayerState *net_nearest_remote_player(void) {
    const PexNetPlayerState *best = NULL;
    float best_d2 = 999999.0f;
    for (int i = 0; i < g_mp_player_count; i++) {
        const PexNetPlayerState *p = &g_mp_players[i];
        if (p->player_id <= 0 || p->player_id == g_mp_player_id || p->health <= 0) continue;
        float dx = p->x - g_player_x;
        float dz = p->z - g_player_z;
        float d2 = dx * dx + dz * dz;
        if (d2 < best_d2) { best_d2 = d2; best = p; }
    }
    return best;
}

static void net_apply_local_knockback(float attacker_x, float attacker_z) {
    float dx = attacker_x - g_player_x;
    float dz = attacker_z - g_player_z;
    float len = sqrtf(dx * dx + dz * dz);
    if (len < 0.0001f) {
        dx = -sinf(g_player_yaw * (float)M_PI / 180.0f);
        dz =  cosf(g_player_yaw * (float)M_PI / 180.0f);
        len = 1.0f;
    }
    g_player_motion_x *= 0.5f;
    g_player_motion_y *= 0.5f;
    g_player_motion_z *= 0.5f;
    g_player_motion_x -= dx / len * 0.4f;
    g_player_motion_y += 0.4f;
    if (g_player_motion_y > 0.4f) g_player_motion_y = 0.4f;
    g_player_motion_z -= dz / len * 0.4f;
    g_player_hurt_time = g_player_max_hurt_time;
    g_player_attacked_at_yaw = atan2f(dz, dx) * 180.0f / (float)M_PI - g_player_yaw;
}

static PexNetRenderDropState *pex_net_find_render_drop(int net_id) {
    if (net_id <= 0) return NULL;
    for (int i = 0; i < PEX_NET_MAX_DROPS; i++) {
        if (g_mp_render_drops[i].active && g_mp_render_drops[i].net_id == net_id) return &g_mp_render_drops[i];
    }
    return NULL;
}

static float pex_net_interp_alpha(void) {
    if (!g_mp_connected || g_mp_interp_duration <= 0.0) return g_frame_partial;
    double a = (now_seconds() - g_mp_interp_start_time) / g_mp_interp_duration;
    if (a < 0.0) a = 0.0;
    if (a > 1.0) a = 1.0;
    return (float)a;
}

static int net_error_is_would_block(void) {
    int e = WSAGetLastError();
    return e == WSAEWOULDBLOCK || e == WSAEINPROGRESS || e == WSAEALREADY;
}

static void pex_net_set_status(const char *msg) {
    snprintf(g_multiplayer_status, sizeof(g_multiplayer_status), "%s", msg ? msg : "");
}

static int pex_net_wait_socket_ready(SOCKET s, int want_write, int timeout_ms) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(s, &set);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    int r = want_write ? select(0, NULL, &set, NULL, &tv) : select(0, &set, NULL, NULL, &tv);
    return r > 0;
}

static int pex_net_send_all(const void *data, int len) {
    const char *p = (const char *)data;
    while (len > 0) {
        int n = send(g_mp_socket, p, len, 0);
        if (n == SOCKET_ERROR) {
            /* Never block the render/UI thread during multiplayer.  If the OS
               send buffer is full, drop the connection instead of freezing. */
            return 0;
        }
        if (n == 0) return 0;
        p += n;
        len -= n;
    }
    return 1;
}

static int pex_net_send_packet(uint16_t type, const void *payload, uint32_t size) {
    if (!g_mp_connected || g_mp_socket == INVALID_SOCKET) return 0;
    PexNetPacketHeader h;
    h.type = type;
    h.reserved = 0;
    h.size = size;
    if (!pex_net_send_all(&h, (int)sizeof(h))) return 0;
    if (size && payload && !pex_net_send_all(payload, (int)size)) return 0;
    return 1;
}

static int pex_net_parse_host_port(const char *input, char *host, size_t host_cap, int *port) {
    if (!host || host_cap == 0 || !port) return 0;
    const char *src = input && input[0] ? input : "127.0.0.1";
    while (*src == ' ' || *src == '\t') src++;
    char tmp[128];
    snprintf(tmp, sizeof(tmp), "%s", src);
    char *end = tmp + strlen(tmp);
    while (end > tmp && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) *--end = 0;
    if (!tmp[0]) snprintf(tmp, sizeof(tmp), "127.0.0.1");

    *port = PEX_NET_DEFAULT_PORT;
    char *colon = strrchr(tmp, ':');
    if (colon && colon[1]) {
        int p = atoi(colon + 1);
        if (p > 0 && p < 65536) {
            *port = p;
            *colon = 0;
        }
    }
    snprintf(host, host_cap, "%s", tmp[0] ? tmp : "127.0.0.1");
    return 1;
}

static int pex_net_floor_chunk_coord(float v) {
    return (int)floorf(v / (float)PEX_NET_CHUNK_SIZE);
}

#define PEX_MP_CHUNK_CACHE_VERSION 1
#define PEX_MP_CHUNK_CACHE_MAGIC "PXCHV6\0\0"

typedef struct PexMpChunkCacheHeader {
    char magic[8];
    int32_t version;
    int32_t world_type;
    int32_t chunk_x;
    int32_t chunk_z;
    int64_t seed;
    uint32_t hash;
    uint32_t payload_size;
} PexMpChunkCacheHeader;

#define PEX_MP_CACHE_SAVE_QUEUE_MAX 64

typedef struct PexMpCacheSaveJob {
    char path[MAX_PATHBUF];
    PexMpChunkCacheHeader header;
    unsigned char *payload;
} PexMpCacheSaveJob;

static CRITICAL_SECTION g_mp_cache_save_cs;
static int g_mp_cache_save_initialized = 0;
static HANDLE g_mp_cache_save_event = NULL;
static HANDLE g_mp_cache_save_thread = NULL;
static volatile LONG g_mp_cache_save_stop = 0;
static PexMpCacheSaveJob g_mp_cache_save_jobs[PEX_MP_CACHE_SAVE_QUEUE_MAX];
static int g_mp_cache_save_head = 0;
static int g_mp_cache_save_tail = 0;
static int g_mp_cache_save_count = 0;

static void pex_mp_cache_save_job_free(PexMpCacheSaveJob *job) {
    if (!job) return;
    if (job->payload) free(job->payload);
    memset(job, 0, sizeof(*job));
}

static DWORD WINAPI pex_mp_cache_save_worker(LPVOID unused) {
    (void)unused;
    for (;;) {
        WaitForSingleObject(g_mp_cache_save_event, INFINITE);
        for (;;) {
            PexMpCacheSaveJob job;
            int have_job = 0;
            memset(&job, 0, sizeof(job));

            EnterCriticalSection(&g_mp_cache_save_cs);
            if (g_mp_cache_save_count > 0) {
                job = g_mp_cache_save_jobs[g_mp_cache_save_head];
                memset(&g_mp_cache_save_jobs[g_mp_cache_save_head], 0, sizeof(g_mp_cache_save_jobs[0]));
                g_mp_cache_save_head = (g_mp_cache_save_head + 1) % PEX_MP_CACHE_SAVE_QUEUE_MAX;
                g_mp_cache_save_count--;
                have_job = 1;
            }
            int should_stop = (g_mp_cache_save_count == 0 && InterlockedCompareExchange(&g_mp_cache_save_stop, 0, 0));
            LeaveCriticalSection(&g_mp_cache_save_cs);

            if (!have_job) {
                if (should_stop) return 0;
                break;
            }

            if (job.path[0] && job.payload) {
                FILE *f = fopen(job.path, "wb");
                if (f) {
                    fwrite(&job.header, 1, sizeof(job.header), f);
                    fwrite(job.payload, 1, PEX_NET_CHUNK_BLOCKS * 3, f);
                    fclose(f);
                }
            }
            pex_mp_cache_save_job_free(&job);
        }
    }
}

static int pex_mp_cache_save_async_init(void) {
    if (g_mp_cache_save_initialized) return g_mp_cache_save_event && g_mp_cache_save_thread;
    InitializeCriticalSection(&g_mp_cache_save_cs);
    g_mp_cache_save_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (g_mp_cache_save_event) {
        InterlockedExchange(&g_mp_cache_save_stop, 0);
        g_mp_cache_save_thread = CreateThread(NULL, 0, pex_mp_cache_save_worker, NULL, 0, NULL);
        if (g_mp_cache_save_thread) SetThreadPriority(g_mp_cache_save_thread, THREAD_PRIORITY_BELOW_NORMAL);
    }
    g_mp_cache_save_initialized = 1;
    return g_mp_cache_save_event && g_mp_cache_save_thread;
}

static int pex_mp_cache_enqueue_save(const char *path, const PexMpChunkCacheHeader *header, unsigned char *payload) {
    if (!path || !header || !payload) return 0;
    if (!pex_mp_cache_save_async_init()) return 0;
    EnterCriticalSection(&g_mp_cache_save_cs);
    if (g_mp_cache_save_count >= PEX_MP_CACHE_SAVE_QUEUE_MAX) {
        /* Cache saves are optional.  Drop the oldest queued write rather than
           ever blocking the render/network thread behind disk I/O. */
        pex_mp_cache_save_job_free(&g_mp_cache_save_jobs[g_mp_cache_save_head]);
        g_mp_cache_save_head = (g_mp_cache_save_head + 1) % PEX_MP_CACHE_SAVE_QUEUE_MAX;
        g_mp_cache_save_count--;
    }
    PexMpCacheSaveJob *job = &g_mp_cache_save_jobs[g_mp_cache_save_tail];
    pex_mp_cache_save_job_free(job);
    snprintf(job->path, sizeof(job->path), "%s", path);
    job->header = *header;
    job->payload = payload;
    g_mp_cache_save_tail = (g_mp_cache_save_tail + 1) % PEX_MP_CACHE_SAVE_QUEUE_MAX;
    g_mp_cache_save_count++;
    LeaveCriticalSection(&g_mp_cache_save_cs);
    SetEvent(g_mp_cache_save_event);
    return 1;
}


static uint32_t pex_mp_hash_byte(uint32_t h, unsigned char b) {
    h ^= (uint32_t)b;
    h *= 16777619u;
    return h;
}

static uint32_t pex_mp_hash_triplet(uint32_t h, unsigned char b, unsigned char level, unsigned char meta) {
    h = pex_mp_hash_byte(h, b);
    h = pex_mp_hash_byte(h, level);
    h = pex_mp_hash_byte(h, meta);
    return h;
}

static uint32_t pex_mp_hash_payload_chunk(const unsigned char *blocks, const unsigned char *levels, const unsigned char *meta) {
    uint32_t h = 2166136261u;
    if (!blocks || !levels || !meta) return 0;
    for (int i = 0; i < PEX_NET_CHUNK_BLOCKS; i++) h = pex_mp_hash_triplet(h, blocks[i], levels[i], meta[i]);
    return h ? h : 1u;
}

static uint32_t pex_mp_hash_local_chunk(int chunk_x, int chunk_z) {
    uint32_t h = 2166136261u;
    int base_x = chunk_x * PEX_NET_CHUNK_SIZE;
    int base_z = chunk_z * PEX_NET_CHUNK_SIZE;
    for (int y = 0; y < PEX_NET_WORLD_HEIGHT; y++) {
        for (int lz = 0; lz < PEX_NET_CHUNK_SIZE; lz++) {
            int wz = base_z + lz;
            for (int lx = 0; lx < PEX_NET_CHUNK_SIZE; lx++) {
                int wx = base_x + lx;
                unsigned char b = 0, level = 0, meta = 0;
                if (flat_in_bounds(wx, y, wz)) {
                    b = g_flat_blocks[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)];
                    level = g_flat_levels[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)];
                    meta = g_flat_meta[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)];
                }
                h = pex_mp_hash_triplet(h, b, level, meta);
            }
        }
    }
    return h ? h : 1u;
}

static void pex_mp_cache_server_key(char *out, size_t cap) {
    if (!out || cap == 0) return;
    char safe[96];
    size_t n = 0;
    const char *src = g_mp_server_name[0] ? g_mp_server_name : "server";
    for (size_t i = 0; src[i] && n + 1 < sizeof(safe); i++) {
        char c = src[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_') safe[n++] = c;
        else safe[n++] = '_';
    }
    safe[n] = 0;
    snprintf(out, cap, "%s_%d_%lld", safe[0] ? safe : "server", g_world_type, (long long)g_world_seed);
}

static int pex_mp_cache_dir(char *out, size_t cap) {
    if (!out || cap == 0 || !g_mc_dir[0]) return 0;
    char root[MAX_PATHBUF];
    char key[160];
    path_join(root, sizeof(root), g_mc_dir, "mpcache");
    ensure_dir(root);
    pex_mp_cache_server_key(key, sizeof(key));
    path_join(out, cap, root, key);
    ensure_dir(out);
    return dir_exists(out);
}

static int pex_mp_cache_chunk_path(int chunk_x, int chunk_z, char *out, size_t cap) {
    char dir[MAX_PATHBUF];
    if (!pex_mp_cache_dir(dir, sizeof(dir))) return 0;
    char name[96];
    snprintf(name, sizeof(name), "c_%d_%d.pxchunk", chunk_x, chunk_z);
    path_join(out, cap, dir, name);
    return 1;
}

static int pex_mp_cache_header_valid(const PexMpChunkCacheHeader *h, int chunk_x, int chunk_z) {
    if (!h) return 0;
    if (memcmp(h->magic, PEX_MP_CHUNK_CACHE_MAGIC, 8) != 0) return 0;
    if (h->version != PEX_MP_CHUNK_CACHE_VERSION) return 0;
    if (h->world_type != g_world_type || h->seed != (int64_t)g_world_seed) return 0;
    if (h->chunk_x != chunk_x || h->chunk_z != chunk_z) return 0;
    if (h->payload_size != (uint32_t)(PEX_NET_CHUNK_BLOCKS * 3)) return 0;
    if (h->hash == 0) return 0;
    return 1;
}

static int pex_mp_cache_peek_chunk_hash(int chunk_x, int chunk_z, uint32_t *out_hash) {
    char path[MAX_PATHBUF];
    if (!pex_mp_cache_chunk_path(chunk_x, chunk_z, path, sizeof(path))) return 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    PexMpChunkCacheHeader h;
    int ok = fread(&h, 1, sizeof(h), f) == sizeof(h) && pex_mp_cache_header_valid(&h, chunk_x, chunk_z);
    fclose(f);
    if (!ok) return 0;
    if (out_hash) *out_hash = h.hash;
    return 1;
}

static void mp_mark_chunk_loaded_from_cache(int chunk_x, int chunk_z, uint32_t hash) {
    int lcx = (chunk_x * PEX_NET_CHUNK_SIZE - g_flat_world_origin_x) / FLAT_RENDER_CHUNK;
    int lcz = (chunk_z * PEX_NET_CHUNK_SIZE - g_flat_world_origin_z) / FLAT_RENDER_CHUNK;
    if (!flat_local_chunk_valid(lcx, lcz)) return;
    g_mp_chunk_section_recv_mask[lcz][lcx] = (unsigned short)((1u << PEX_NET_SECTIONS_Y) - 1u);
    g_mp_chunk_hash_cache[lcz][lcx] = hash;
    g_mp_chunk_cache_loaded[lcz][lcx] = 1;
    stream_mark_chunk_generated(lcx, lcz);
    for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
        g_flat_section_dirty[sy][lcz][lcx] = 1;
        g_flat_section_valid[sy][lcz][lcx] = 0;
        flat_note_section_mesh_changed(sy, lcz, lcx);
    }
    g_flat_renderer_sort_dirty = 1;
}

static int pex_mp_cache_load_chunk(int chunk_x, int chunk_z, uint32_t expected_hash) {
    char path[MAX_PATHBUF];
    if (!pex_mp_cache_chunk_path(chunk_x, chunk_z, path, sizeof(path))) return 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    PexMpChunkCacheHeader h;
    if (fread(&h, 1, sizeof(h), f) != sizeof(h) || !pex_mp_cache_header_valid(&h, chunk_x, chunk_z) || h.hash != expected_hash) {
        fclose(f);
        return 0;
    }
    unsigned char *blocks = (unsigned char *)malloc(PEX_NET_CHUNK_BLOCKS * 3);
    if (!blocks) { fclose(f); return 0; }
    unsigned char *levels = blocks + PEX_NET_CHUNK_BLOCKS;
    unsigned char *meta = levels + PEX_NET_CHUNK_BLOCKS;
    int ok = fread(blocks, 1, PEX_NET_CHUNK_BLOCKS * 3, f) == PEX_NET_CHUNK_BLOCKS * 3;
    fclose(f);
    if (!ok) { free(blocks); return 0; }

    int base_x = chunk_x * PEX_NET_CHUNK_SIZE;
    int base_z = chunk_z * PEX_NET_CHUNK_SIZE;
    uint32_t verify = 2166136261u;
    for (int idx = 0; idx < PEX_NET_CHUNK_BLOCKS; idx++) {
        verify = pex_mp_hash_triplet(verify, blocks[idx], levels[idx], meta[idx]);
    }
    if (verify == 0) verify = 1u;
    if (verify != expected_hash) { free(blocks); return 0; }

    for (int y = 0; y < PEX_NET_WORLD_HEIGHT; y++) {
        for (int lz = 0; lz < PEX_NET_CHUNK_SIZE; lz++) {
            int wz = base_z + lz;
            for (int lx = 0; lx < PEX_NET_CHUNK_SIZE; lx++) {
                int wx = base_x + lx;
                int idx = y * PEX_NET_CHUNK_SIZE * PEX_NET_CHUNK_SIZE + lz * PEX_NET_CHUNK_SIZE + lx;
                if (!flat_in_bounds(wx, y, wz)) continue;
                unsigned char bid = blocks[idx];
                g_flat_blocks[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = bid;
                g_flat_levels[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = block_is_liquid(bid) ? (levels[idx] & 15) : 0;
                g_flat_meta[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = block_is_liquid(bid) ? (levels[idx] & 15) : meta[idx];
            }
        }
    }
    free(blocks);
    mp_mark_chunk_loaded_from_cache(chunk_x, chunk_z, expected_hash);
    return 1;
}

static void pex_mp_cache_save_chunk(int chunk_x, int chunk_z) {
    if (!g_mp_connected) return;
    int lcx = (chunk_x * PEX_NET_CHUNK_SIZE - g_flat_world_origin_x) / FLAT_RENDER_CHUNK;
    int lcz = (chunk_z * PEX_NET_CHUNK_SIZE - g_flat_world_origin_z) / FLAT_RENDER_CHUNK;
    if (!flat_local_chunk_valid(lcx, lcz) || !g_flat_world_chunk_generated[lcz][lcx]) return;

    unsigned char *payload = (unsigned char *)malloc(PEX_NET_CHUNK_BLOCKS * 3);
    if (!payload) return;
    unsigned char *blocks = payload;
    unsigned char *levels = blocks + PEX_NET_CHUNK_BLOCKS;
    unsigned char *meta = levels + PEX_NET_CHUNK_BLOCKS;
    int base_x = chunk_x * PEX_NET_CHUNK_SIZE;
    int base_z = chunk_z * PEX_NET_CHUNK_SIZE;
    for (int y = 0; y < PEX_NET_WORLD_HEIGHT; y++) {
        for (int lz = 0; lz < PEX_NET_CHUNK_SIZE; lz++) {
            int wz = base_z + lz;
            for (int lx = 0; lx < PEX_NET_CHUNK_SIZE; lx++) {
                int wx = base_x + lx;
                int idx = y * PEX_NET_CHUNK_SIZE * PEX_NET_CHUNK_SIZE + lz * PEX_NET_CHUNK_SIZE + lx;
                if (flat_in_bounds(wx, y, wz)) {
                    blocks[idx] = g_flat_blocks[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)];
                    levels[idx] = g_flat_levels[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)];
                    meta[idx] = g_flat_meta[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)];
                } else {
                    blocks[idx] = levels[idx] = meta[idx] = 0;
                }
            }
        }
    }

    uint32_t hash = pex_mp_hash_payload_chunk(blocks, levels, meta);
    char path[MAX_PATHBUF];
    if (!pex_mp_cache_chunk_path(chunk_x, chunk_z, path, sizeof(path))) { free(payload); return; }

    PexMpChunkCacheHeader h;
    memset(&h, 0, sizeof(h));
    memcpy(h.magic, PEX_MP_CHUNK_CACHE_MAGIC, 8);
    h.version = PEX_MP_CHUNK_CACHE_VERSION;
    h.world_type = g_world_type;
    h.chunk_x = chunk_x;
    h.chunk_z = chunk_z;
    h.seed = (int64_t)g_world_seed;
    h.hash = hash;
    h.payload_size = (uint32_t)(PEX_NET_CHUNK_BLOCKS * 3);

    if (pex_mp_cache_enqueue_save(path, &h, payload)) {
        g_mp_chunk_hash_cache[lcz][lcx] = hash;
        g_mp_chunk_cache_loaded[lcz][lcx] = 1;
    } else {
        free(payload);
    }
}

static void pex_mp_note_section_for_cache(int chunk_x, int chunk_z, int section_y) {
    int lcx = (chunk_x * PEX_NET_CHUNK_SIZE - g_flat_world_origin_x) / FLAT_RENDER_CHUNK;
    int lcz = (chunk_z * PEX_NET_CHUNK_SIZE - g_flat_world_origin_z) / FLAT_RENDER_CHUNK;
    if (!flat_local_chunk_valid(lcx, lcz) || !flat_section_index_valid(section_y)) return;
    g_mp_chunk_section_recv_mask[lcz][lcx] |= (unsigned short)(1u << section_y);
    if (g_mp_chunk_section_recv_mask[lcz][lcx] == (unsigned short)((1u << PEX_NET_SECTIONS_Y) - 1u)) {
        pex_mp_cache_save_chunk(chunk_x, chunk_z);
    }
}

static int mp_cache_hash_for_request(PexNetChunkRequest *req, int chunk_x, int chunk_z) {
    if (!req || req->hash_count >= PEX_NET_MAX_CHUNK_HASHES) return 0;
    uint32_t hash = 0;
    if (!pex_mp_cache_peek_chunk_hash(chunk_x, chunk_z, &hash)) return 0;
    req->hashes[req->hash_count].chunk_x = chunk_x;
    req->hashes[req->hash_count].chunk_z = chunk_z;
    req->hashes[req->hash_count].hash = hash;
    req->hash_count++;
    return 1;
}

static DWORD WINAPI pex_net_connect_worker(LPVOID unused) {
    (void)unused;
    char host[96];
    char port_text[16];
    snprintf(host, sizeof(host), "%s", g_mp_connect_thread_host[0] ? g_mp_connect_thread_host : "127.0.0.1");
    snprintf(port_text, sizeof(port_text), "%d", g_mp_connect_thread_port);
    g_mp_connect_thread_error[0] = 0;
    g_mp_connect_thread_socket = INVALID_SOCKET;
    InterlockedExchange(&g_mp_connect_thread_ok, 0);

    /* This used to happen when PEX_S2C_WELCOME was handled on the render
       thread.  Clearing the large block/meta/level arrays here makes the
       "Logging in" screen keep repainting while the async connect runs. */
    memset(g_flat_blocks, 0, sizeof(g_flat_blocks));
    memset(g_flat_meta, 0, sizeof(g_flat_meta));
    memset(g_flat_levels, 0, sizeof(g_flat_levels));

    struct addrinfo hints;
    struct addrinfo *result = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host, port_text, &hints, &result) != 0 || !result) {
        snprintf(g_mp_connect_thread_error, sizeof(g_mp_connect_thread_error), "Could not resolve server.");
        InterlockedExchange(&g_mp_connect_thread_done, 1);
        return 0;
    }

    SOCKET s = INVALID_SOCKET;
    for (struct addrinfo *rp = result; rp; rp = rp->ai_next) {
        s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (s == INVALID_SOCKET) continue;
        BOOL yes = TRUE;
        int sock_buf = 1024 * 1024;
        setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *)&yes, sizeof(yes));
        setsockopt(s, SOL_SOCKET, SO_SNDBUF, (const char *)&sock_buf, sizeof(sock_buf));
        setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char *)&sock_buf, sizeof(sock_buf));

        u_long nonblock = 1;
        ioctlsocket(s, FIONBIO, &nonblock);
        int cr = connect(s, rp->ai_addr, (int)rp->ai_addrlen);
        if (cr == 0) break;
        if (cr == SOCKET_ERROR) {
            int e = WSAGetLastError();
            if (e == WSAEWOULDBLOCK || e == WSAEINPROGRESS || e == WSAEALREADY) {
                fd_set wfds;
                FD_ZERO(&wfds);
                FD_SET(s, &wfds);
                struct timeval tv;
                tv.tv_sec = 10;
                tv.tv_usec = 0;
                int sr = select(0, NULL, &wfds, NULL, &tv);
                if (sr > 0) {
                    int err = 0;
#if defined(_WIN32) && !defined(PEX_PLATFORM_SDL2) && !defined(PEX_PLATFORM_ANDROID_TV)
                    int err_len = sizeof(err);
#else
                    socklen_t err_len = (socklen_t)sizeof(err);
#endif
                    getsockopt(s, SOL_SOCKET, SO_ERROR, (char *)&err, &err_len);
                    if (err == 0) break;
                }
            }
        }
        closesocket(s);
        s = INVALID_SOCKET;
    }
    freeaddrinfo(result);

    if (s == INVALID_SOCKET) {
        snprintf(g_mp_connect_thread_error, sizeof(g_mp_connect_thread_error), "Could not connect.");
        InterlockedExchange(&g_mp_connect_thread_done, 1);
        return 0;
    }

    g_mp_connect_thread_socket = s;
    InterlockedExchange(&g_mp_connect_thread_ok, 1);
    InterlockedExchange(&g_mp_connect_thread_done, 1);
    return 0;
}

static void net_request_chunks_around_player(int force) {
    if (!g_mp_connected || g_mp_socket == INVALID_SOCKET) return;
    double now = now_seconds();
    int cx = pex_net_floor_chunk_coord(g_player_x);
    int cz = pex_net_floor_chunk_coord(g_player_z);
    int radius = 0;
    if (g_mp_world_ready) {
        radius = stream_render_radius();
        if (radius < 1) radius = 1;
        if (radius > 8) radius = 8;
    }
    if (!force) {
        if ((now - g_mp_last_chunk_request_time) < 0.35 &&
            cx == g_mp_last_request_cx && cz == g_mp_last_request_cz) return;
    }
    PexNetChunkRequest req;
    memset(&req, 0, sizeof(req));
    req.center_chunk_x = cx;
    req.center_chunk_z = cz;
    req.radius = radius;

    if (g_mp_world_ready) {
        for (int ring = 0; ring <= radius && req.hash_count < PEX_NET_MAX_CHUNK_HASHES; ring++) {
            for (int dz = -ring; dz <= ring && req.hash_count < PEX_NET_MAX_CHUNK_HASHES; dz++) {
                for (int dx = -ring; dx <= ring && req.hash_count < PEX_NET_MAX_CHUNK_HASHES; dx++) {
                    if (ring != 0 && abs(dx) != ring && abs(dz) != ring) continue;
                    mp_cache_hash_for_request(&req, cx + dx, cz + dz);
                }
            }
        }
    }

    if (pex_net_send_packet(PEX_C2S_CHUNK_REQUEST, &req, sizeof(req))) {
        g_mp_last_chunk_request_time = now;
        g_mp_last_request_cx = cx;
        g_mp_last_request_cz = cz;
    }
}

static void pex_net_clear_remote_state(void) {
    net_free_render_players();
    memset(g_mp_players, 0, sizeof(g_mp_players));
    memset(g_mp_prev_players, 0, sizeof(g_mp_prev_players));
    memset(g_mp_render_drops, 0, sizeof(g_mp_render_drops));
    memset(g_falling_blocks, 0, sizeof(g_falling_blocks));
    g_mp_player_count = 0;
    g_mp_world_ready = 0;
    g_mp_expected_chunks = 0;
    g_mp_chunks_received = 0;
    g_mp_connect_progress = 0;
    g_mp_interp_start_time = now_seconds();
    g_mp_interp_duration = 0.10;
    g_mp_render_last_time = g_mp_interp_start_time;
    memset(g_mp_pending_pickups, 0, sizeof(g_mp_pending_pickups));
    g_mp_last_chunk_request_time = 0.0;
    g_mp_last_request_cx = 999999;
    g_mp_last_request_cz = 999999;
    memset(g_mp_chunk_section_recv_mask, 0, sizeof(g_mp_chunk_section_recv_mask));
    memset(g_mp_chunk_hash_cache, 0, sizeof(g_mp_chunk_hash_cache));
    memset(g_mp_chunk_cache_loaded, 0, sizeof(g_mp_chunk_cache_loaded));
    g_mp_pending_respawn_sync = 0;
}

static void pex_net_note_sections_received(int section_count) {
    if (section_count <= 0) section_count = 1;
    g_mp_chunks_received += section_count;
    if (g_mp_expected_chunks > 0 && !g_mp_world_ready) {
        int pct = (g_mp_chunks_received * 100) / g_mp_expected_chunks;
        if (pct < 0) pct = 0;
        if (pct > 100) pct = 100;
        g_mp_connect_progress = pct;
        snprintf(g_multiplayer_status, sizeof(g_multiplayer_status),
                 "Downloading terrain: %d/%d (%d%%)",
                 g_mp_chunks_received, g_mp_expected_chunks, pct);
    }
    if (g_mp_expected_chunks > 0 && g_mp_chunks_received >= g_mp_expected_chunks) {
        g_mp_world_ready = 1;
        g_mp_connect_progress = 100;
    }
}

static void pex_net_finish_world_download(void) {
    if (!g_mp_world_ready && g_mp_expected_chunks > 0 && g_mp_chunks_received < g_mp_expected_chunks) {
        snprintf(g_multiplayer_status, sizeof(g_multiplayer_status),
                 "Downloaded %d/%d sections; starting anyway.",
                 g_mp_chunks_received, g_mp_expected_chunks);
    }
    g_mp_world_ready = 1;
    g_mp_connect_progress = 100;
    if (g_screen != SCREEN_INGAME) {
        g_chat_count = 0;
        hud_add_chat("Connected to PEXCraft server.");
        set_screen(SCREEN_INGAME);
    }
}

static void pex_net_mark_chunk_ready(int chunk_x, int chunk_z) {
    int lcx = (chunk_x * PEX_NET_CHUNK_SIZE - g_flat_world_origin_x) / FLAT_RENDER_CHUNK;
    int lcz = (chunk_z * PEX_NET_CHUNK_SIZE - g_flat_world_origin_z) / FLAT_RENDER_CHUNK;
    if (lcx >= 0 && lcx < FLAT_RENDER_CHUNKS && lcz >= 0 && lcz < FLAT_RENDER_CHUNKS) {
        stream_mark_chunk_generated(lcx, lcz);
    }
}

static void pex_net_apply_chunk(const PexNetChunk *chunk) {
    if (!chunk) return;
    int base_x = chunk->chunk_x * PEX_NET_CHUNK_SIZE;
    int base_z = chunk->chunk_z * PEX_NET_CHUNK_SIZE;
    for (int y = 0; y < PEX_NET_WORLD_HEIGHT; y++) {
        if (y < FLAT_WORLD_Y_MIN || y > FLAT_WORLD_Y_MAX) continue;
        for (int lz = 0; lz < PEX_NET_CHUNK_SIZE; lz++) {
            int wz = base_z + lz;
            if (wz < g_flat_world_origin_z || wz >= g_flat_world_origin_z + FLAT_WORLD_SIZE) continue;
            for (int lx = 0; lx < PEX_NET_CHUNK_SIZE; lx++) {
                int wx = base_x + lx;
                if (wx < g_flat_world_origin_x || wx >= g_flat_world_origin_x + FLAT_WORLD_SIZE) continue;
                int idx = y * PEX_NET_CHUNK_SIZE * PEX_NET_CHUNK_SIZE + lz * PEX_NET_CHUNK_SIZE + lx;
                unsigned char bid = chunk->blocks[idx];
                unsigned char level = chunk->levels[idx] & 15;
                unsigned char meta = chunk->meta[idx];
                g_flat_blocks[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = bid;
                g_flat_levels[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = block_is_liquid(bid) ? level : 0;
                g_flat_meta[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = block_is_liquid(bid) ? level : meta;
            }
        }
    }
    pex_net_mark_chunk_ready(chunk->chunk_x, chunk->chunk_z);
    pex_mp_cache_save_chunk(chunk->chunk_x, chunk->chunk_z);
    pex_net_note_sections_received(16);
}

static void pex_net_apply_chunk_rle(const void *payload, uint32_t size) {
    if (!payload || size < sizeof(PexNetChunkRleHeader)) return;
    const unsigned char *bytes = (const unsigned char *)payload;
    PexNetChunkRleHeader rh;
    memcpy(&rh, bytes, sizeof(rh));
    if (rh.cell_count != PEX_NET_CHUNK_BLOCKS) return;
    if (rh.run_count == 0 || rh.run_count > PEX_NET_CHUNK_BLOCKS) return;
    size_t need = sizeof(PexNetChunkRleHeader) + (size_t)rh.run_count * sizeof(PexNetChunkRleRun);
    if ((size_t)size != need) return;

    int base_x = rh.chunk_x * PEX_NET_CHUNK_SIZE;
    int base_z = rh.chunk_z * PEX_NET_CHUNK_SIZE;
    const PexNetChunkRleRun *runs = (const PexNetChunkRleRun *)(bytes + sizeof(PexNetChunkRleHeader));
    uint32_t idx = 0;
    for (uint32_t r = 0; r < rh.run_count; r++) {
        uint32_t count = runs[r].count;
        if (count == 0 || idx + count > PEX_NET_CHUNK_BLOCKS) return;
        for (uint32_t k = 0; k < count; k++, idx++) {
            int y = (int)(idx / (PEX_NET_CHUNK_SIZE * PEX_NET_CHUNK_SIZE));
            int rem = (int)(idx % (PEX_NET_CHUNK_SIZE * PEX_NET_CHUNK_SIZE));
            int lz = rem / PEX_NET_CHUNK_SIZE;
            int lx = rem % PEX_NET_CHUNK_SIZE;
            int wx = base_x + lx;
            int wz = base_z + lz;
            if (y < FLAT_WORLD_Y_MIN || y > FLAT_WORLD_Y_MAX) continue;
            if (wx < g_flat_world_origin_x || wx >= g_flat_world_origin_x + FLAT_WORLD_SIZE) continue;
            if (wz < g_flat_world_origin_z || wz >= g_flat_world_origin_z + FLAT_WORLD_SIZE) continue;
            unsigned char bid = runs[r].block;
            unsigned char level = runs[r].level & 15;
            unsigned char meta = runs[r].meta;
            g_flat_blocks[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = bid;
            g_flat_levels[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = block_is_liquid(bid) ? level : 0;
            g_flat_meta[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = block_is_liquid(bid) ? level : meta;
        }
    }
    if (idx == PEX_NET_CHUNK_BLOCKS) {
        pex_net_mark_chunk_ready(rh.chunk_x, rh.chunk_z);
        pex_mp_cache_save_chunk(rh.chunk_x, rh.chunk_z);
        pex_net_note_sections_received(16);
    }
}

static void pex_net_apply_chunk_section_rle(const void *payload, uint32_t size) {
    if (!payload || size < sizeof(PexNetChunkSectionRleHeader)) return;
    const unsigned char *bytes = (const unsigned char *)payload;
    PexNetChunkSectionRleHeader rh;
    memcpy(&rh, bytes, sizeof(rh));
    if (rh.cell_count != PEX_NET_SECTION_BLOCKS) return;
    if (rh.run_count == 0 || rh.run_count > PEX_NET_SECTION_BLOCKS) return;
    if (rh.section_y < 0 || rh.section_y >= PEX_NET_SECTIONS_Y) return;
    size_t need = sizeof(PexNetChunkSectionRleHeader) + (size_t)rh.run_count * sizeof(PexNetChunkSectionRleRun);
    if ((size_t)size != need) return;

    int base_x = rh.chunk_x * PEX_NET_CHUNK_SIZE;
    int base_z = rh.chunk_z * PEX_NET_CHUNK_SIZE;
    int section_base_y = rh.section_y * PEX_NET_SECTION_HEIGHT;
    int lcx = (base_x - g_flat_world_origin_x) / FLAT_RENDER_CHUNK;
    int lcz = (base_z - g_flat_world_origin_z) / FLAT_RENDER_CHUNK;
    int non_empty = 0;
    const PexNetChunkSectionRleRun *runs = (const PexNetChunkSectionRleRun *)(bytes + sizeof(PexNetChunkSectionRleHeader));
    uint32_t idx = 0;
    for (uint32_t r = 0; r < rh.run_count; r++) {
        uint32_t count = runs[r].count;
        if (count == 0 || idx + count > PEX_NET_SECTION_BLOCKS) return;
        for (uint32_t k = 0; k < count; k++, idx++) {
            int yy = (int)(idx / (PEX_NET_CHUNK_SIZE * PEX_NET_CHUNK_SIZE));
            int rem = (int)(idx % (PEX_NET_CHUNK_SIZE * PEX_NET_CHUNK_SIZE));
            int lz = rem / PEX_NET_CHUNK_SIZE;
            int lx = rem % PEX_NET_CHUNK_SIZE;
            int y = section_base_y + yy;
            int wx = base_x + lx;
            int wz = base_z + lz;
            if (y < FLAT_WORLD_Y_MIN || y > FLAT_WORLD_Y_MAX) continue;
            if (wx < g_flat_world_origin_x || wx >= g_flat_world_origin_x + FLAT_WORLD_SIZE) continue;
            if (wz < g_flat_world_origin_z || wz >= g_flat_world_origin_z + FLAT_WORLD_SIZE) continue;
            unsigned char bid = runs[r].block;
            unsigned char level = runs[r].level & 15;
            unsigned char meta = runs[r].meta;
            if (bid) non_empty = 1;
            g_flat_blocks[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = bid;
            g_flat_levels[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = block_is_liquid(bid) ? level : 0;
            g_flat_meta[flat_y_index(y)][flat_z_index(wz)][flat_index(wx)] = block_is_liquid(bid) ? level : meta;
        }
    }
    if (idx != PEX_NET_SECTION_BLOCKS) return;
    if (flat_local_chunk_valid(lcx, lcz) && flat_section_index_valid(rh.section_y)) {
        g_flat_world_chunk_generated[lcz][lcx] = 1;
        if (non_empty) g_flat_chunk_section_non_empty_mask[lcz][lcx] |= (unsigned short)(1u << rh.section_y);
        else g_flat_chunk_section_non_empty_mask[lcz][lcx] &= (unsigned short)~(1u << rh.section_y);
        g_flat_world_chunk_dirty[lcz][lcx] = 1;
        g_flat_world_chunk_valid[lcz][lcx] = 0;
        g_flat_section_valid[rh.section_y][lcz][lcx] = 0;
        g_flat_section_dirty[rh.section_y][lcz][lcx] = 1;
        flat_note_section_mesh_changed(rh.section_y, lcz, lcx);
        g_flat_renderer_sort_dirty = 1;
    }
    pex_mp_note_section_for_cache(rh.chunk_x, rh.chunk_z, rh.section_y);
    pex_net_note_sections_received(1);
}

static void pex_net_apply_chunk_cache_hit(const PexNetChunkCacheHit *hit) {
    if (!hit || hit->hash == 0) return;
    if (!g_mp_world_ready) {
        /* Do not perform disk reads on the loading screen.  A cache hit is only an
           optimization; during initial join we ask for the real section stream so
           the render thread stays responsive and the Java-style progress screen
           keeps updating. */
        PexNetChunkRequest req;
        memset(&req, 0, sizeof(req));
        req.center_chunk_x = hit->chunk_x;
        req.center_chunk_z = hit->chunk_z;
        req.radius = 0;
        req.hash_count = -1;
        pex_net_send_packet(PEX_C2S_CHUNK_REQUEST, &req, sizeof(req));
        return;
    }
    if (pex_mp_cache_load_chunk(hit->chunk_x, hit->chunk_z, hit->hash)) {
        pex_net_note_sections_received(16);
    } else {
        /* The server says the hash matched, but the local file disappeared or
           failed verification. Ask for that exact column again without a hash so
           the server sends real section data next tick. */
        PexNetChunkRequest req;
        memset(&req, 0, sizeof(req));
        req.center_chunk_x = hit->chunk_x;
        req.center_chunk_z = hit->chunk_z;
        req.radius = 0;
        req.hash_count = -1; /* force resend; local cache was unusable */
        pex_net_send_packet(PEX_C2S_CHUNK_REQUEST, &req, sizeof(req));
    }
}

static void pex_net_apply_block_change(const PexNetBlockChange *ch) {
    if (!ch) return;
    if (!flat_in_bounds(ch->x, ch->y, ch->z)) return;
    unsigned char bid = (unsigned char)ch->block_id;
    unsigned char level = (unsigned char)(ch->level & 15);
    unsigned char meta = (unsigned char)(ch->meta & 255);
    int yi = flat_y_index(ch->y), zi = flat_z_index(ch->z), xi = flat_index(ch->x);
    g_flat_blocks[yi][zi][xi] = bid;
    g_flat_levels[yi][zi][xi] = block_is_liquid(bid) ? level : 0;
    g_flat_meta[yi][zi][xi] = block_is_liquid(bid) ? level : meta;
    int lcx = flat_local_chunk_x(ch->x);
    int lcz = flat_local_chunk_z(ch->z);
    if (flat_local_chunk_valid(lcx, lcz) && !g_flat_world_chunk_generated[lcz][lcx]) {
        g_flat_world_chunk_generated[lcz][lcx] = 1;
    }
    flat_update_section_after_block_change(ch->x, ch->y, ch->z, bid);
    flat_mark_sections_dirty_near_block(ch->x, ch->y, ch->z);
    flat_mark_chunks_dirty_near(ch->x, ch->z);
}



static void pex_net_apply_inventory_update(const PexNetInventoryUpdate *up) {
    if (!up) return;
    int count = up->slot_count;
    if (count < 0) count = 0;
    if (count > 36) count = 36;
    for (int i = 0; i < count; i++) {
        g_inventory[i].id = up->slots[i].id;
        g_inventory[i].count = up->slots[i].count;
        g_inventory[i].damage = up->slots[i].damage;
        g_inventory[i].pop_time = 0;
        if (g_inventory[i].id <= 0 || g_inventory[i].count <= 0) stack_clear(&g_inventory[i]);
        if (g_inventory[i].count > stack_limit_for_id(g_inventory[i].id)) g_inventory[i].count = stack_limit_for_id(g_inventory[i].id);
    }
    for (int i = count; i < 36; i++) stack_clear(&g_inventory[i]);
}

static void pex_net_apply_chest_update(const PexNetChestUpdate *up) {
    if (!up) return;
    int count = up->slot_count;
    if (count < 0) count = 0;
    if (count > PEX_NET_CHEST_SLOTS) count = PEX_NET_CHEST_SLOTS;
    for (int part = 0; part < up->chest_count && part < 2; part++) {
        int idx = chest_alloc_tile(up->x[part], up->y[part], up->z[part]);
        if (idx < 0) continue;
        for (int i = 0; i < 27; i++) {
            int src_i = part * 27 + i;
            if (src_i >= count) break;
            ItemStack *dst = &g_chest_tiles[idx].slots[i];
            dst->id = up->slots[src_i].id;
            dst->count = up->slots[src_i].count;
            dst->damage = up->slots[src_i].damage;
            dst->pop_time = 0;
            if (dst->id <= 0 || dst->count <= 0) stack_clear(dst);
        }
    }
}

static void pex_net_apply_player_action(const PexNetPlayerAction *a) {
    if (!a || a->player_id <= 0) return;
    if (a->action == PEX_ACTION_ATTACK) {
        PexNetRenderPlayerState *attacker_r = (a->player_id != g_mp_player_id) ? pex_net_find_render_player(a->player_id) : NULL;
        if (attacker_r) {
            attacker_r->swing = 0.0f;
            attacker_r->prev_swing = 0.0f;
            attacker_r->swing_time = 0.0f;
            attacker_r->swing_active = 1;
            attacker_r->swing_ticks = 0;
        }
        if (a->x != g_mp_player_id) {
            PexNetRenderPlayerState *victim_r = pex_net_find_render_player(a->x);
            if (victim_r) {
                victim_r->hurt_time = 0.50f;
                if (a->z <= 0 && victim_r->death_time <= 0.0f) victim_r->death_time = 0.05f;
            }
            return;
        }
        if (a->player_id == g_mp_player_id) return;
        const PexNetPlayerState *attacker = pex_net_find_player_state(a->player_id);
        float ax = attacker ? attacker->x : g_player_x;
        float az = attacker ? attacker->z : (g_player_z - 1.0f);
        if (attacker_r) { ax = attacker_r->x; az = attacker_r->z; }
        net_apply_local_knockback(ax, az);
        if (a->z <= 0 && !g_player_dead) {
            player_health_set_hearts(0);
            player_die("was slain");
        } else if (a->z > 0 && a->z < g_player_health) {
            player_health_set_hearts(a->z);
        }
        return;
    }
    if (a->player_id == g_mp_player_id) return;
    PexNetRenderPlayerState *r = pex_net_find_render_player(a->player_id);
    if (r) {
        int restart = (a->action != PEX_ACTION_MINE_HIT) || !r->swing_active;
        if (restart) {
            r->swing = 0.0f;
            r->prev_swing = 0.0f;
            r->swing_time = 0.0f;
            r->swing_active = 1;
            r->swing_ticks = 0;
        }
    }
    if (a->action == PEX_ACTION_MINE_HIT) {
        int id = a->block_id > 0 ? a->block_id : flat_get_block(a->x, a->y, a->z);
        if (id > 0) spawn_block_hit_particle(a->x, a->y, a->z, a->face, id);
        if (r) {
            r->mining_active = 1;
            r->mining_x = a->x;
            r->mining_y = a->y;
            r->mining_z = a->z;
            r->mining_stage = a->progress;
            if (r->mining_stage < 0) r->mining_stage = 0;
            if (r->mining_stage > 9) r->mining_stage = 9;
            r->mining_time = 1.25f;
        }
    } else if (a->action == PEX_ACTION_BREAK) {
        int id = a->block_id > 0 ? a->block_id : flat_get_block(a->x, a->y, a->z);
        if (id > 0) spawn_block_destroy_particles(a->x, a->y, a->z, id);
        if (r) r->mining_active = 0;
    }
}

static uint32_t pex_net_skin_packet_size_for(uint32_t byte_count) {
    if (byte_count > PEX_NET_SKIN_MAX_BYTES) return 0;
    return (uint32_t)(sizeof(PexNetSkin) - PEX_NET_SKIN_MAX_BYTES + byte_count);
}

static int pex_net_skin_payload_valid(const PexNetSkin *skin, uint32_t size) {
    if (!skin) return 0;
    if (size < sizeof(PexNetSkin) - PEX_NET_SKIN_MAX_BYTES) return 0;
    if (skin->width != 64 || (skin->height != 32 && skin->height != 64)) return 0;
    uint32_t bytes = (uint32_t)(skin->width * skin->height * 4);
    if (skin->byte_count != bytes || skin->byte_count > PEX_NET_SKIN_MAX_BYTES) return 0;
    return size == pex_net_skin_packet_size_for(skin->byte_count);
}

static void pex_net_send_skin(void) {
    if (!g_mp_connected || g_mp_player_id <= 0 || !tex_steve.rgba) return;
    if (tex_steve.w != 64 || (tex_steve.h != 32 && tex_steve.h != 64)) return;
    uint32_t bytes = (uint32_t)(tex_steve.w * tex_steve.h * 4);
    if (bytes == 0 || bytes > PEX_NET_SKIN_MAX_BYTES) return;
    PexNetSkin skin;
    memset(&skin, 0, sizeof(skin));
    skin.player_id = g_mp_player_id;
    skin.width = tex_steve.w;
    skin.height = tex_steve.h;
    skin.byte_count = bytes;
    memcpy(skin.rgba, tex_steve.rgba, bytes);
    pex_net_send_packet(PEX_C2S_SKIN, &skin, pex_net_skin_packet_size_for(bytes));
}

static void pex_net_apply_skin(const PexNetSkin *skin, uint32_t size) {
    if (!pex_net_skin_payload_valid(skin, size)) return;
    if (skin->player_id <= 0 || skin->player_id == g_mp_player_id) return;
    PexNetRenderPlayerState *r = pex_net_find_render_player(skin->player_id);
    if (!r) {
        for (int i = 0; i < PEX_NET_MAX_PLAYERS; i++) {
            if (!g_mp_render_players[i].active) {
                r = &g_mp_render_players[i];
                pex_net_free_render_player(r);
                r->active = 1;
                r->player_id = skin->player_id;
                r->skin_only = 1;
                r->health = 20;
                break;
            }
        }
    }
    if (!r) return;
    unsigned char *rgba = (unsigned char *)malloc(skin->byte_count);
    if (!rgba) return;
    memcpy(rgba, skin->rgba, skin->byte_count);
    if (upload_rgba_texture(&r->skin, skin->width, skin->height, rgba, 0)) {
        r->has_skin = 1;
    } else {
        free_texture(&r->skin);
    }
}

static void pex_net_apply_snapshot(const PexNetSnapshot *snap) {
    if (!snap) return;
    double now = now_seconds();
    double elapsed = now - g_mp_interp_start_time;
    float carry_alpha = pex_net_interp_alpha();
    if (elapsed > 0.03 && elapsed < 0.50) g_mp_interp_duration = elapsed;
    if (g_mp_interp_duration < 0.05) g_mp_interp_duration = 0.05;
    if (g_mp_interp_duration > 0.20) g_mp_interp_duration = 0.20;
    g_mp_interp_start_time = now;

    PexNetPlayerState old_players[PEX_NET_MAX_PLAYERS];
    PexNetPlayerState old_prev_players[PEX_NET_MAX_PLAYERS];
    int old_player_count = g_mp_player_count;
    memcpy(old_players, g_mp_players, sizeof(old_players));
    memcpy(old_prev_players, g_mp_prev_players, sizeof(old_prev_players));

    g_mp_player_count = snap->player_count;
    if (g_mp_player_count < 0) g_mp_player_count = 0;
    if (g_mp_player_count > PEX_NET_MAX_PLAYERS) g_mp_player_count = PEX_NET_MAX_PLAYERS;
    for (int i = 0; i < g_mp_player_count; i++) {
        g_mp_players[i] = snap->players[i];
        g_mp_prev_players[i] = g_mp_players[i];
        for (int j = 0; j < old_player_count; j++) {
            if (old_players[j].player_id == g_mp_players[i].player_id) {
                g_mp_prev_players[i] = old_players[j];
                g_mp_prev_players[i].x = old_prev_players[j].x + (old_players[j].x - old_prev_players[j].x) * carry_alpha;
                g_mp_prev_players[i].y = old_prev_players[j].y + (old_players[j].y - old_prev_players[j].y) * carry_alpha;
                g_mp_prev_players[i].z = old_prev_players[j].z + (old_players[j].z - old_prev_players[j].z) * carry_alpha;
                g_mp_prev_players[i].yaw = lerp_angle(old_prev_players[j].yaw, old_players[j].yaw, carry_alpha);
                g_mp_prev_players[i].pitch = old_prev_players[j].pitch + (old_players[j].pitch - old_prev_players[j].pitch) * carry_alpha;
                break;
            }
        }
        if (g_mp_players[i].player_id == g_mp_player_id) {
            int old_health = g_player_health;
            int server_health = g_mp_players[i].health;
            if (server_health < 0) server_health = 0;
            if (server_health > 20) server_health = 20;
            if (server_health < old_health && !g_player_dead) {
                const PexNetPlayerState *attacker = net_nearest_remote_player();
                if (attacker) {
                    float adx = attacker->x - g_player_x;
                    float adz = attacker->z - g_player_z;
                    if (adx * adx + adz * adz <= 64.0f) {
                        net_apply_local_knockback(attacker->x, attacker->z);
                    }
                }
            }
            g_mp_players[i].health = server_health;
            if (server_health != old_health) player_health_set_hearts(server_health);
            else g_player_health = server_health;
            g_player_armor = g_mp_players[i].armor;
            if (g_mp_pending_respawn_sync && server_health > 0) {
                g_player_x = g_player_prev_x = g_mp_players[i].x;
                g_player_y = g_player_prev_y = g_mp_players[i].y;
                g_player_z = g_player_prev_z = g_mp_players[i].z;
                g_player_motion_x = g_player_motion_y = g_player_motion_z = 0.0f;
                g_player_fall_distance = 0.0f;
                g_mp_pending_respawn_sync = 0;
                net_request_chunks_around_player(1);
            }
            if (g_player_health <= 0 && !g_player_dead) {
                player_die("was slain");
            } else if (g_player_health > 0 && g_player_dead) {
                g_player_dead = 0;
                g_player_death_time = 0;
                if (g_screen == SCREEN_DEATH) set_screen(SCREEN_INGAME);
            }
        }
    }

    FlatDroppedItem old_drops[MAX_DROP_ENTITIES];
    memcpy(old_drops, g_drops, sizeof(old_drops));
    FlatFallingBlock old_falling[MAX_FALLING_BLOCK_ENTITIES];
    memcpy(old_falling, g_falling_blocks, sizeof(old_falling));
    memset(g_drops, 0, sizeof(g_drops));
    memset(g_falling_blocks, 0, sizeof(g_falling_blocks));
    int drop_count = snap->drop_count;
    if (drop_count < 0) drop_count = 0;
    if (drop_count > MAX_DROP_ENTITIES) drop_count = MAX_DROP_ENTITIES;
    if (drop_count > PEX_NET_MAX_DROPS) drop_count = PEX_NET_MAX_DROPS;
    for (int i = 0; i < drop_count; i++) {
        const PexNetDropState *src = &snap->drops[i];
        FlatDroppedItem *dst = &g_drops[i];
        if (!src->active || src->id <= 0 || src->count <= 0) continue;
        dst->active = 1;
        dst->net_id = src->drop_id;
        dst->stack.id = src->id;
        dst->stack.count = src->count;
        dst->stack.damage = src->damage;
        dst->x = src->x;
        dst->y = src->y;
        dst->z = src->z;
        dst->prev_x = src->x;
        dst->prev_y = src->y;
        dst->prev_z = src->z;
        for (int j = 0; j < MAX_DROP_ENTITIES; j++) {
            if (old_drops[j].active && old_drops[j].net_id == src->drop_id) {
                dst->prev_x = old_drops[j].prev_x + (old_drops[j].x - old_drops[j].prev_x) * carry_alpha;
                dst->prev_y = old_drops[j].prev_y + (old_drops[j].y - old_drops[j].prev_y) * carry_alpha;
                dst->prev_z = old_drops[j].prev_z + (old_drops[j].z - old_drops[j].prev_z) * carry_alpha;
                break;
            }
        }
        dst->mx = src->mx;
        dst->my = src->my;
        dst->mz = src->mz;
        dst->rot = src->rot;
        dst->age = src->age;
        dst->pickup_delay = src->pickup_delay;
    }

    int falling_count = snap->falling_count;
    if (falling_count < 0) falling_count = 0;
    if (falling_count > MAX_FALLING_BLOCK_ENTITIES) falling_count = MAX_FALLING_BLOCK_ENTITIES;
    if (falling_count > PEX_NET_MAX_FALLING_BLOCKS) falling_count = PEX_NET_MAX_FALLING_BLOCKS;
    for (int i = 0; i < falling_count; i++) {
        const PexNetFallingBlockState *src = &snap->falling[i];
        FlatFallingBlock *dst = &g_falling_blocks[i];
        if (!src->active || src->block_id <= 0) continue;

        const FlatFallingBlock *old = NULL;
        for (int j = 0; j < MAX_FALLING_BLOCK_ENTITIES; j++) {
            if (old_falling[j].active && old_falling[j].net_id == src->entity_id) {
                old = &old_falling[j];
                break;
            }
        }

        dst->active = 1;
        dst->net_id = src->entity_id;
        dst->block_id = src->block_id;
        dst->start_x = src->start_x;
        dst->start_y = src->start_y;
        dst->start_z = src->start_z;
        dst->end_x = src->end_x;
        dst->end_y = src->end_y;
        dst->end_z = src->end_z;
        dst->path_id = src->path_id;
        dst->path_duration = src->path_duration > 0 ? src->path_duration : 1;

        if (old && old->path_id == src->path_id && old->path_duration > 0) {
            dst->path_start_time = old->path_start_time;
        } else {
            /* New path: start the visual clock now and slide from the server's
               start endpoint to its end endpoint.  No live server position is
               chased, so repeated snapshots cannot make the block snap/flicker. */
            dst->path_start_time = now;
        }
        double elapsed = now - dst->path_start_time;
        if (elapsed < 0.0) elapsed = 0.0;
        dst->path_age = (int)(elapsed * 20.0);
        if (dst->path_age < 0) dst->path_age = 0;
        if (dst->path_age > dst->path_duration) dst->path_age = dst->path_duration;
        float path_t = (float)dst->path_age / (float)dst->path_duration;
        if (path_t < 0.0f) path_t = 0.0f;
        if (path_t > 1.0f) path_t = 1.0f;
        dst->x = dst->start_x + (dst->end_x - dst->start_x) * path_t;
        dst->y = dst->start_y + (dst->end_y - dst->start_y) * path_t;
        dst->z = dst->start_z + (dst->end_z - dst->start_z) * path_t;
        dst->prev_x = dst->x;
        dst->prev_y = dst->y;
        dst->prev_z = dst->z;
        dst->mx = dst->my = dst->mz = 0.0f;
        dst->age = dst->path_age;
    }
}

static void pex_net_update_smoothing(void) {
    if (!g_mp_connected) return;
    double now = now_seconds();
    if (g_mp_render_last_time <= 0.0) g_mp_render_last_time = now;
    float dt = (float)(now - g_mp_render_last_time);
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.10f) dt = 0.10f;
    g_mp_render_last_time = now;

    float player_alpha = pex_net_smooth_step(dt, 18.0f);
    float drop_alpha = pex_net_smooth_step(dt, 14.0f);
    int player_seen[PEX_NET_MAX_PLAYERS];
    int drop_seen[PEX_NET_MAX_DROPS];
    memset(player_seen, 0, sizeof(player_seen));
    memset(drop_seen, 0, sizeof(drop_seen));

    for (int i = 0; i < g_mp_player_count; i++) {
        PexNetPlayerState *p = &g_mp_players[i];
        if (p->player_id <= 0 || p->player_id == g_mp_player_id) continue;
        PexNetRenderPlayerState *r = pex_net_find_render_player(p->player_id);
        if (!r) {
            for (int j = 0; j < PEX_NET_MAX_PLAYERS; j++) {
                if (!g_mp_render_players[j].active) {
                    r = &g_mp_render_players[j];
                    memset(r, 0, sizeof(*r));
                    r->active = 1;
                    r->player_id = p->player_id;
                    r->x = r->prev_x = p->x;
                    r->y = r->prev_y = p->y;
                    r->z = r->prev_z = p->z;
                    r->yaw = r->prev_yaw = p->yaw;
                    r->pitch = r->prev_pitch = p->pitch;
                    r->health = p->health > 0 ? p->health : 20;
                    break;
                }
            }
        }
        if (!r) continue;
        if (r->skin_only) {
            r->x = r->prev_x = p->x;
            r->y = r->prev_y = p->y;
            r->z = r->prev_z = p->z;
            r->yaw = r->prev_yaw = p->yaw;
            r->pitch = r->prev_pitch = p->pitch;
            r->skin_only = 0;
        }
        snprintf(r->name, sizeof(r->name), "%s",
                 p->name[0] ? p->name : "Player");
        r->prev_x = r->x;
        r->prev_y = r->y;
        r->prev_z = r->z;
        r->prev_yaw = r->yaw;
        r->prev_pitch = r->pitch;
        if (p->health < r->health) r->hurt_time = 0.50f;
        if (p->health <= 0 && r->health > 0 && r->death_time <= 0.0f) r->death_time = 0.05f;
        if (p->health > 0) r->death_time = 0.0f;
        r->health = p->health;
        r->flags = p->flags;

        float dx = p->x - r->x;
        float dy = p->y - r->y;
        float dz = p->z - r->z;
        float dist2 = dx * dx + dy * dy + dz * dz;
        float packet_dx = p->x - g_mp_prev_players[i].x;
        float packet_dz = p->z - g_mp_prev_players[i].z;
        float target_move = sqrtf(packet_dx * packet_dx + packet_dz * packet_dz) * 6.0f;
        if (target_move > 1.0f) target_move = 1.0f;
        if (dist2 > 16.0f) {
            r->x = p->x;
            r->y = p->y;
            r->z = p->z;
            r->move_amount = 0.0f;
        } else {
            r->x += dx * player_alpha;
            r->y += dy * player_alpha;
            r->z += dz * player_alpha;
            r->move_amount += (target_move - r->move_amount) * pex_net_smooth_step(dt, 10.0f);
        }
        r->yaw += pex_net_angle_delta(r->yaw, p->yaw) * player_alpha;
        r->pitch += (p->pitch - r->pitch) * player_alpha;
        r->limb_swing += r->move_amount * dt * 12.0f;
        if (r->hurt_time > 0.0f) {
            r->hurt_time -= dt;
            if (r->hurt_time < 0.0f) r->hurt_time = 0.0f;
        }
        if (r->death_time > 0.0f) {
            r->death_time += dt;
            if (r->death_time > 1.0f) r->death_time = 1.0f;
        }
        if (r->mining_time > 0.0f) {
            r->mining_time -= dt;
            if (r->mining_time <= 0.0f) {
                r->mining_time = 0.0f;
                r->mining_active = 0;
            }
        }
        r->prev_swing = r->swing;
        if (r->swing_active) {
            r->swing_time += dt;
            r->swing_ticks = (int)(r->swing_time * 20.0f);
            if (r->swing_time >= 0.40f) {
                r->swing = 0.0f;
                r->prev_swing = 0.0f;
                r->swing_time = 0.0f;
                r->swing_active = 0;
                r->swing_ticks = 0;
            } else {
                r->swing = r->swing_time / 0.40f;
            }
        } else {
            r->swing = 0.0f;
            r->prev_swing = 0.0f;
            r->swing_time = 0.0f;
            r->swing_ticks = 0;
        }

        for (int j = 0; j < PEX_NET_MAX_PLAYERS; j++) {
            if (&g_mp_render_players[j] == r) {
                player_seen[j] = 1;
                break;
            }
        }
    }

    for (int i = 0; i < PEX_NET_MAX_PLAYERS; i++) {
        if (g_mp_render_players[i].active && !player_seen[i] && !g_mp_render_players[i].skin_only) {
            pex_net_free_render_player(&g_mp_render_players[i]);
        }
    }

    for (int i = 0; i < MAX_DROP_ENTITIES; i++) {
        FlatDroppedItem *e = &g_drops[i];
        if (!e->active || e->net_id <= 0) continue;
        PexNetRenderDropState *r = pex_net_find_render_drop(e->net_id);
        if (!r) {
            for (int j = 0; j < PEX_NET_MAX_DROPS; j++) {
                if (!g_mp_render_drops[j].active) {
                    r = &g_mp_render_drops[j];
                    memset(r, 0, sizeof(*r));
                    r->active = 1;
                    r->net_id = e->net_id;
                    r->x = r->prev_x = e->x;
                    r->y = r->prev_y = e->y;
                    r->z = r->prev_z = e->z;
                    break;
                }
            }
        }
        if (!r) continue;
        r->prev_x = r->x;
        r->prev_y = r->y;
        r->prev_z = r->z;

        float dx = e->x - r->x;
        float dy = e->y - r->y;
        float dz = e->z - r->z;
        float dist2 = dx * dx + dy * dy + dz * dz;
        if (dist2 > 9.0f) {
            r->x = e->x;
            r->y = e->y;
            r->z = e->z;
        } else {
            r->x += dx * drop_alpha;
            r->y += dy * drop_alpha;
            r->z += dz * drop_alpha;
        }

        for (int j = 0; j < PEX_NET_MAX_DROPS; j++) {
            if (&g_mp_render_drops[j] == r) {
                drop_seen[j] = 1;
                break;
            }
        }
    }

    for (int i = 0; i < PEX_NET_MAX_DROPS; i++) {
        if (g_mp_render_drops[i].active && !drop_seen[i]) g_mp_render_drops[i].active = 0;
    }
}

static void pex_net_apply_knockback(const PexNetKnockback *kb) {
    if (!kb || kb->victim_id != g_mp_player_id) return;
    g_player_motion_x = kb->mx;
    g_player_motion_y = kb->my;
    g_player_motion_z = kb->mz;
    if (kb->health >= 0 && kb->health <= 20) {
        player_health_set_hearts(kb->health);
    }
    const PexNetPlayerState *attacker = pex_net_find_player_state(kb->attacker_id);
    if (attacker) {
        float dz = attacker->z - g_player_z;
        float dx = attacker->x - g_player_x;
        g_player_attacked_at_yaw = atan2f(dz, dx) * 180.0f / (float)M_PI - g_player_yaw;
    }
    g_player_hurt_time = g_player_max_hurt_time;
    if (g_player_health <= 0 && !g_player_dead) player_die("was slain");
}

static void pex_net_handle_packet(uint16_t type, const void *payload, uint32_t size) {
    if (type == PEX_S2C_WELCOME && size == sizeof(PexNetWelcome)) {
        const PexNetWelcome *w = (const PexNetWelcome *)payload;
        g_mp_player_id = w->player_id;
        g_world_type = w->world_type;
        g_world_seed = w->seed;
        g_flat_world_origin_x = w->origin_x;
        g_flat_world_origin_z = w->origin_z;
        /* g_flat_blocks/meta/levels were cleared by the connection worker before
           the server handshake completed.  Do not memset the 3D world arrays on
           the render thread here. */
        memset(g_flat_world_chunk_generated, 0, sizeof(g_flat_world_chunk_generated));
        memset(g_flat_chunk_section_non_empty_mask, 0, sizeof(g_flat_chunk_section_non_empty_mask));
        memset(g_mp_chunk_section_recv_mask, 0, sizeof(g_mp_chunk_section_recv_mask));
        memset(g_mp_chunk_hash_cache, 0, sizeof(g_mp_chunk_hash_cache));
        memset(g_mp_chunk_cache_loaded, 0, sizeof(g_mp_chunk_cache_loaded));
        stream_generation_queue_clear();
        memset(g_drops, 0, sizeof(g_drops));
        memset(g_falling_blocks, 0, sizeof(g_falling_blocks));
        inventory_reset();
        reset_flat_player_spawn();
        g_player_x = g_player_prev_x = w->spawn_x;
        g_player_y = g_player_prev_y = w->spawn_y;
        g_player_z = g_player_prev_z = w->spawn_z;
        player_health_set_silent(20);
        memset(g_armor_inventory, 0, sizeof(g_armor_inventory));
        g_player_armor = 0;
        g_player_damage_remainder = 0;
        g_player_dead = 0;
        g_player_death_time = 0;
        g_player_hurt_time = 0;
        g_player_attacked_at_yaw = 0.0f;
        net_free_render_players();
        memset(g_mp_render_drops, 0, sizeof(g_mp_render_drops));
        g_mp_render_last_time = now_seconds();
        snprintf(g_loaded_world_name, sizeof(g_loaded_world_name), "Multiplayer");
        g_loaded_world_dir[0] = 0;
        pex_net_set_status("Downloading terrain");
        pex_net_send_skin();
        net_request_chunks_around_player(1);
    } else if (type == PEX_S2C_WORLD_START && size == sizeof(PexNetWorldStart)) {
        const PexNetWorldStart *ws = (const PexNetWorldStart *)payload;
        if (ws->chunk_count > 0) g_mp_expected_chunks = ws->chunk_count;
        snprintf(g_multiplayer_status, sizeof(g_multiplayer_status), "Downloading terrain: 0/%d", g_mp_expected_chunks);
    } else if (type == PEX_S2C_CHUNK_SECTION_RLE) {
        pex_net_apply_chunk_section_rle(payload, size);
        if (g_mp_world_ready) pex_net_finish_world_download();
    } else if (type == PEX_S2C_CHUNK_CACHE_HIT && size == sizeof(PexNetChunkCacheHit)) {
        pex_net_apply_chunk_cache_hit((const PexNetChunkCacheHit *)payload);
        if (g_mp_world_ready) pex_net_finish_world_download();
    } else if (type == PEX_S2C_CHUNK && size == sizeof(PexNetChunk)) {
        pex_net_apply_chunk((const PexNetChunk *)payload);
        if (g_mp_world_ready) pex_net_finish_world_download();
    } else if (type == PEX_S2C_CHUNK_RLE) {
        pex_net_apply_chunk_rle(payload, size);
        if (g_mp_world_ready) pex_net_finish_world_download();
    } else if (type == PEX_S2C_WORLD_DONE && size == sizeof(PexNetWorldDone)) {
        const PexNetWorldDone *done = (const PexNetWorldDone *)payload;
        if (done->chunk_count > 0) g_mp_expected_chunks = done->chunk_count;
        pex_net_finish_world_download();
    } else if (type == PEX_S2C_BLOCK_CHANGE && size == sizeof(PexNetBlockChange)) {
        pex_net_apply_block_change((const PexNetBlockChange *)payload);
    } else if (type == PEX_S2C_CHEST_UPDATE && size == sizeof(PexNetChestUpdate)) {
        pex_net_apply_chest_update((const PexNetChestUpdate *)payload);
    } else if (type == PEX_S2C_INVENTORY_UPDATE && size == sizeof(PexNetInventoryUpdate)) {
        pex_net_apply_inventory_update((const PexNetInventoryUpdate *)payload);
    } else if (type == PEX_S2C_PLAYER_ACTION && size == sizeof(PexNetPlayerAction)) {
        pex_net_apply_player_action((const PexNetPlayerAction *)payload);
    } else if (type == PEX_S2C_PLAYER_KNOCKBACK && size == sizeof(PexNetKnockback)) {
        pex_net_apply_knockback((const PexNetKnockback *)payload);
    } else if (type == PEX_S2C_SKIN) {
        pex_net_apply_skin((const PexNetSkin *)payload, size);
    } else if (type == PEX_S2C_SNAPSHOT && size == sizeof(PexNetSnapshot)) {
        pex_net_apply_snapshot((const PexNetSnapshot *)payload);
    } else if (type == PEX_S2C_CHAT && size == sizeof(PexNetChat)) {
        const PexNetChat *chat = (const PexNetChat *)payload;
        if (chat->text[0]) hud_add_chat(chat->text);
    } else if (type == PEX_S2C_KICK && size == sizeof(PexNetChat)) {
        const PexNetChat *chat = (const PexNetChat *)payload;
        char msg[MAX_LABEL];
        snprintf(msg, sizeof(msg), "%s", chat->text[0] ? chat->text : "Disconnected by server.");
        pex_net_disconnect();
        open_notice("Disconnected", msg, "");
    } else if (type == PEX_S2C_PICKUP_APPROVED && size == sizeof(PexNetPickupApproved)) {
        const PexNetPickupApproved *approved = (const PexNetPickupApproved *)payload;
        ItemStack st = make_stack(approved->id, approved->count, approved->damage);
        inventory_add_stack(st);
        for (int i = 0; i < MAX_DROP_ENTITIES; i++) {
            if (g_drops[i].active && g_drops[i].net_id == approved->drop_id) {
                g_drops[i].active = 0;
                break;
            }
        }
        for (int i = 0; i < PEX_NET_MAX_DROPS; i++) {
            if (g_mp_pending_pickups[i] == approved->drop_id) {
                g_mp_pending_pickups[i] = 0;
                break;
            }
        }
    }
}

static int pex_net_reserve_recv(size_t need) {
    if (need <= g_mp_recv_cap) return 1;
    size_t cap = g_mp_recv_cap ? g_mp_recv_cap : 65536;
    while (cap < need) cap *= 2;
    unsigned char *buf = (unsigned char *)realloc(g_mp_recv_buf, cap);
    if (!buf) return 0;
    g_mp_recv_buf = buf;
    g_mp_recv_cap = cap;
    return 1;
}

static int pex_net_is_connecting(void) {
    return g_mp_connecting;
}

static void pex_net_connect_fail(const char *msg) {
    if (g_mp_connect_socket != INVALID_SOCKET) {
        closesocket(g_mp_connect_socket);
        g_mp_connect_socket = INVALID_SOCKET;
    }
    if (g_mp_connect_thread_socket != INVALID_SOCKET) {
        closesocket(g_mp_connect_thread_socket);
        g_mp_connect_thread_socket = INVALID_SOCKET;
    }
    if (g_mp_connect_thread && InterlockedCompareExchange(&g_mp_connect_thread_done, 0, 0)) {
        CloseHandle(g_mp_connect_thread);
        g_mp_connect_thread = NULL;
    }
    g_mp_connecting = 0;
    g_mp_connected = 0;
    pex_net_set_status(msg ? msg : "Could not connect.");
    if (g_screen == SCREEN_CONNECTING) {
        open_notice("Connection failed", g_multiplayer_status[0] ? g_multiplayer_status : "Could not connect to server.", "");
    }
}

static void pex_net_connect_tick(void) {
    if (!g_mp_connecting) return;
    double now = now_seconds();
    double elapsed = now - g_mp_connect_start_time;

    if (!InterlockedCompareExchange(&g_mp_connect_thread_done, 0, 0)) {
        int pct = (int)(elapsed * 100.0 / 10.0);
        if (pct < 1) pct = 1;
        if (pct > 99) pct = 99;
        g_mp_connect_progress = pct;
        snprintf(g_multiplayer_status, sizeof(g_multiplayer_status), "Resolving/connecting... %d%%", pct);
        return;
    }

    if (g_mp_connect_thread) {
        CloseHandle(g_mp_connect_thread);
        g_mp_connect_thread = NULL;
    }

    if (!InterlockedCompareExchange(&g_mp_connect_thread_ok, 0, 0) || g_mp_connect_thread_socket == INVALID_SOCKET) {
        pex_net_connect_fail(g_mp_connect_thread_error[0] ? g_mp_connect_thread_error : "Could not connect.");
        return;
    }

    g_mp_socket = g_mp_connect_thread_socket;
    g_mp_connect_thread_socket = INVALID_SOCKET;
    g_mp_connecting = 0;
    g_mp_connected = 1;
    g_mp_player_id = 0;

    PexNetHello hello;
    memset(&hello, 0, sizeof(hello));
    memcpy(hello.magic, PEX_NET_MAGIC, PEX_NET_MAGIC_LEN);
    hello.version = PEX_NET_PROTOCOL_VERSION;
    snprintf(hello.name, sizeof(hello.name), "%s", g_multiplayer_username[0] ? g_multiplayer_username : "Player");
    if (!pex_net_send_packet(PEX_C2S_HELLO, &hello, sizeof(hello))) {
        pex_net_disconnect();
        pex_net_set_status("Handshake failed.");
        if (g_screen == SCREEN_CONNECTING) open_notice("Connection failed", "Handshake failed.", "");
        return;
    }
    if (!pex_net_start_rx_thread()) {
        pex_net_disconnect();
        pex_net_set_status(g_mp_rx_error[0] ? g_mp_rx_error : "Network thread failed.");
        if (g_screen == SCREEN_CONNECTING) open_notice("Connection failed", g_multiplayer_status, "");
        return;
    }
    g_mp_connect_progress = 40;
    pex_net_set_status("Logging in...");
}

static void pex_net_poll(void) {
    if (g_mp_connecting) pex_net_connect_tick();
    if (!g_mp_connected || g_mp_socket == INVALID_SOCKET) return;

    if (InterlockedCompareExchange(&g_mp_rx_lost, 0, 0)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "%s", g_mp_rx_error[0] ? g_mp_rx_error : "Connection to server was lost.");
        pex_net_disconnect();
        open_notice("Disconnected", msg, "");
        return;
    }

    int packets_this_frame = 0;
    int chunk_packets_this_frame = 0;
    PexMpQueuedPacket qp;
    while (pex_mp_rx_queue_pop(&qp)) {
        int is_chunk_packet = (qp.type == PEX_S2C_CHUNK || qp.type == PEX_S2C_CHUNK_RLE ||
                               qp.type == PEX_S2C_CHUNK_SECTION_RLE || qp.type == PEX_S2C_CHUNK_CACHE_HIT);
        int chunk_budget = g_mp_world_ready ? 2 : 1;
        int packet_budget = g_mp_world_ready ? 96 : 32;
        if (is_chunk_packet && chunk_packets_this_frame >= chunk_budget) {
            /* Put it back at the head by handling it next frame.  The queue is FIFO;
               for simplicity, keep the packet payload and stop processing now. */
            EnterCriticalSection(&g_mp_rx_cs);
            if (g_mp_rx_count < PEX_MP_PACKET_QUEUE_MAX) {
                g_mp_rx_head = (g_mp_rx_head + PEX_MP_PACKET_QUEUE_MAX - 1) % PEX_MP_PACKET_QUEUE_MAX;
                g_mp_rx_packets[g_mp_rx_head] = qp;
                g_mp_rx_count++;
                memset(&qp, 0, sizeof(qp));
            }
            LeaveCriticalSection(&g_mp_rx_cs);
            break;
        }
        if (packets_this_frame >= packet_budget) {
            EnterCriticalSection(&g_mp_rx_cs);
            if (g_mp_rx_count < PEX_MP_PACKET_QUEUE_MAX) {
                g_mp_rx_head = (g_mp_rx_head + PEX_MP_PACKET_QUEUE_MAX - 1) % PEX_MP_PACKET_QUEUE_MAX;
                g_mp_rx_packets[g_mp_rx_head] = qp;
                g_mp_rx_count++;
                memset(&qp, 0, sizeof(qp));
            }
            LeaveCriticalSection(&g_mp_rx_cs);
            break;
        }

        pex_net_handle_packet(qp.type, qp.data, qp.size);
        if (is_chunk_packet) chunk_packets_this_frame++;
        packets_this_frame++;
        pex_mp_rx_packet_free(&qp);
        if (!g_mp_connected) return;
    }
    EnterCriticalSection(&g_mp_rx_cs);
    g_prof_rx_queue_last = g_mp_rx_count;
    LeaveCriticalSection(&g_mp_rx_cs);
    g_prof_packets_last = packets_this_frame;
    g_prof_chunks_last = chunk_packets_this_frame;
    net_request_chunks_around_player(0);
}

static int pex_net_connect_to_server(const char *server) {
    char host[96];
    int port = PEX_NET_DEFAULT_PORT;
    pex_net_parse_host_port(server, host, sizeof(host), &port);
    pex_net_disconnect();
    pex_net_clear_remote_state();
    g_mp_connect_progress = 1;
    pex_net_set_status("Resolving/connecting... 1%");

    if (!g_mp_winsock_started) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            pex_net_set_status("Winsock failed.");
            return 0;
        }
        g_mp_winsock_started = 1;
    }

    if (g_mp_connect_thread) {
        if (InterlockedCompareExchange(&g_mp_connect_thread_done, 0, 0)) {
            CloseHandle(g_mp_connect_thread);
        }
        g_mp_connect_thread = NULL;
    }
    if (g_mp_connect_thread_socket != INVALID_SOCKET) {
        closesocket(g_mp_connect_thread_socket);
        g_mp_connect_thread_socket = INVALID_SOCKET;
    }

    snprintf(g_mp_connect_thread_host, sizeof(g_mp_connect_thread_host), "%s", host[0] ? host : "127.0.0.1");
    g_mp_connect_thread_port = port;
    g_mp_connect_thread_error[0] = 0;
    InterlockedExchange(&g_mp_connect_thread_done, 0);
    InterlockedExchange(&g_mp_connect_thread_ok, 0);
    g_mp_connect_thread = CreateThread(NULL, 0, pex_net_connect_worker, NULL, 0, NULL);
    if (!g_mp_connect_thread) {
        pex_net_set_status("Could not start connection thread.");
        return 0;
    }

    g_mp_connecting = 1;
    g_mp_connect_start_time = now_seconds();
    snprintf(g_mp_server_name, sizeof(g_mp_server_name), "%s:%d", host, port);
    g_mp_connect_progress = 1;
    pex_net_set_status("Resolving/connecting... 1%");
    return 1;
}

static void pex_net_send_player_state(void) {
    if (!g_mp_connected || g_mp_player_id <= 0 || !g_mp_world_ready) return;
    PexNetPlayerState st;
    memset(&st, 0, sizeof(st));
    st.player_id = g_mp_player_id;
    st.x = g_player_x;
    st.y = g_player_y;
    st.z = g_player_z;
    st.yaw = g_player_yaw;
    st.pitch = g_player_pitch;
    st.health = g_player_health;
    st.armor = g_player_armor;
    if (g_screen == SCREEN_INGAME && key_down_vk(g_opts.keys[5])) {
        st.flags |= PEX_PLAYER_FLAG_SNEAKING;
    }
    st.selected_slot = g_selected_hotbar_slot;
    if (!pex_net_send_packet(PEX_C2S_PLAYER_STATE, &st, sizeof(st))) {
        pex_net_disconnect();
    }
}

static void pex_net_send_block_action(int action, int x, int y, int z, int face, int block_id) {
    if (!g_mp_connected || !g_mp_world_ready) return;
    PexNetBlockAction a;
    memset(&a, 0, sizeof(a));
    a.action = action;
    a.x = x;
    a.y = y;
    a.z = z;
    a.face = face;
    a.block_id = block_id;
    a.held_id = g_inventory[g_selected_hotbar_slot].id;
    a.held_damage = g_inventory[g_selected_hotbar_slot].damage;
    if (flat_in_bounds(x, y, z)) {
        a.level = flat_get_level(x, y, z) & 15;
        a.meta = flat_get_meta(x, y, z) & 255;
    }
    pex_net_send_packet(PEX_C2S_BLOCK_ACTION, &a, sizeof(a));
}

static void net_send_action_progress(int action, int x, int y, int z, int face, int block_id, int progress) {
    if (!g_mp_connected || !g_mp_world_ready || g_mp_player_id <= 0) return;
    PexNetPlayerAction a;
    memset(&a, 0, sizeof(a));
    a.player_id = g_mp_player_id;
    a.action = action;
    a.x = x;
    a.y = y;
    a.z = z;
    a.face = face;
    a.block_id = block_id;
    a.progress = progress;
    pex_net_send_packet(PEX_C2S_PLAYER_ACTION, &a, sizeof(a));
}

static void pex_net_send_player_action(int action, int x, int y, int z, int face, int block_id) {
    net_send_action_progress(action, x, y, z, face, block_id, 0);
}

static int pex_net_player_ray_distance(float max_dist, float *out_t) {
    if (out_t) *out_t = 9999.0f;
    if (!g_mp_connected || !g_mp_world_ready || g_player_dead) return 0;

    float lx, ly, lz;
    player_look_vector(&lx, &ly, &lz);
    int found = 0;
    float best_t = max_dist;
    if (best_t <= 0.0f) best_t = 5.0f;

    for (int i = 0; i < PEX_NET_MAX_PLAYERS; i++) {
        PexNetRenderPlayerState *r = &g_mp_render_players[i];
        if (!r->active || r->player_id <= 0 || r->player_id == g_mp_player_id) continue;
        const PexNetPlayerState *st = pex_net_find_player_state(r->player_id);
        if (st && st->health <= 0) continue;

        float feet_y = r->y - 1.62f;
        float min_x = r->x - 0.35f, max_x = r->x + 0.35f;
        float min_y = feet_y, max_y = feet_y + 1.85f;
        float min_z = r->z - 0.35f, max_z = r->z + 0.35f;

        float o[3] = {g_player_x, g_player_y, g_player_z};
        float d[3] = {lx, ly, lz};
        float mn[3] = {min_x, min_y, min_z};
        float mx[3] = {max_x, max_y, max_z};
        float tmin = 0.0f, tmax = best_t;
        const float eps = 0.00001f;
        int hit = 1;
        for (int axis = 0; axis < 3; axis++) {
            if (fabsf(d[axis]) < eps) {
                if (o[axis] < mn[axis] || o[axis] > mx[axis]) { hit = 0; break; }
            } else {
                float inv = 1.0f / d[axis];
                float t1 = (mn[axis] - o[axis]) * inv;
                float t2 = (mx[axis] - o[axis]) * inv;
                if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
                if (t1 > tmin) tmin = t1;
                if (t2 < tmax) tmax = t2;
                if (tmin > tmax) { hit = 0; break; }
            }
        }
        if (!hit || tmin < 0.0f) continue;
        if (tmin < best_t) {
            best_t = tmin;
            found = 1;
        }
    }

    if (found && out_t) *out_t = best_t;
    return found;
}

static int pex_net_try_attack_player(void) {
    if (!g_mp_connected || !g_mp_world_ready || g_mp_player_id <= 0 || g_player_dead) return 0;

    FlatRayHit block_hit = flat_raycast();
    float block_dist = 4.25f;
    if (block_hit.hit) {
        float bx = block_hit.hx - g_player_x;
        float by = block_hit.hy - g_player_y;
        float bz = block_hit.hz - g_player_z;
        block_dist = sqrtf(bx * bx + by * by + bz * bz);
    }

    float player_t = 9999.0f;
    if (!pex_net_player_ray_distance(block_dist, &player_t)) return 0;

    float lx, ly, lz;
    player_look_vector(&lx, &ly, &lz);
    int best_id = 0;
    float best_t = block_dist;
    for (int i = 0; i < PEX_NET_MAX_PLAYERS; i++) {
        PexNetRenderPlayerState *r = &g_mp_render_players[i];
        if (!r->active || r->player_id <= 0 || r->player_id == g_mp_player_id) continue;
        const PexNetPlayerState *st = pex_net_find_player_state(r->player_id);
        if (st && st->health <= 0) continue;

        float feet_y = r->y - 1.62f;
        float min_x = r->x - 0.35f, max_x = r->x + 0.35f;
        float min_y = feet_y, max_y = feet_y + 1.85f;
        float min_z = r->z - 0.35f, max_z = r->z + 0.35f;
        float o[3] = {g_player_x, g_player_y, g_player_z};
        float d[3] = {lx, ly, lz};
        float mn[3] = {min_x, min_y, min_z};
        float mx[3] = {max_x, max_y, max_z};
        float tmin = 0.0f, tmax = best_t;
        const float eps = 0.00001f;
        int hit = 1;
        for (int axis = 0; axis < 3; axis++) {
            if (fabsf(d[axis]) < eps) {
                if (o[axis] < mn[axis] || o[axis] > mx[axis]) { hit = 0; break; }
            } else {
                float inv = 1.0f / d[axis];
                float t1 = (mn[axis] - o[axis]) * inv;
                float t2 = (mx[axis] - o[axis]) * inv;
                if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
                if (t1 > tmin) tmin = t1;
                if (t2 < tmax) tmax = t2;
                if (tmin > tmax) { hit = 0; break; }
            }
        }
        if (!hit || tmin < 0.0f) continue;
        if (tmin < best_t) {
            best_t = tmin;
            best_id = r->player_id;
        }
    }

    if (best_id <= 0) return 0;
    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    int held_id = stack_empty(held) ? 0 : held->id;
    restart_hand_swing();
    pex_net_send_player_action(PEX_ACTION_ATTACK, best_id, 0, 0, 0, held_id);
    return 1;
}

static void pex_net_send_drop_item(ItemStack st) {
    if (!g_mp_connected || !g_mp_world_ready || stack_empty(&st)) return;
    PexNetDropItem d;
    memset(&d, 0, sizeof(d));
    d.id = st.id;
    d.count = st.count;
    d.damage = st.damage;
    d.x = g_player_x;
    d.y = g_player_y - 0.30f;
    d.z = g_player_z;
    d.yaw = g_player_yaw;
    d.pitch = g_player_pitch;
    pex_net_send_packet(PEX_C2S_DROP_ITEM, &d, sizeof(d));
}

static int pex_net_pickup_pending(int drop_id) {
    if (drop_id <= 0) return 1;
    for (int i = 0; i < PEX_NET_MAX_DROPS; i++) {
        if (g_mp_pending_pickups[i] == drop_id) return 1;
    }
    return 0;
}

static void pex_net_send_pickup_drop(int drop_id) {
    if (!g_mp_connected || !g_mp_world_ready || drop_id <= 0 || pex_net_pickup_pending(drop_id)) return;
    for (int i = 0; i < PEX_NET_MAX_DROPS; i++) {
        if (g_mp_pending_pickups[i] == 0) {
            g_mp_pending_pickups[i] = drop_id;
            break;
        }
    }
    PexNetPickupDrop pickup;
    memset(&pickup, 0, sizeof(pickup));
    pickup.drop_id = drop_id;
    pex_net_send_packet(PEX_C2S_PICKUP_DROP, &pickup, sizeof(pickup));
}

static void pex_net_fill_open_chest_header(PexNetChestOpen *open) {
    memset(open, 0, sizeof(*open));
    open->chest_count = g_open_chest_count;
    if (open->chest_count < 0) open->chest_count = 0;
    if (open->chest_count > 2) open->chest_count = 2;
    for (int part = 0; part < open->chest_count; part++) {
        int idx = g_open_chest_indices[part];
        if (idx < 0 || idx >= MAX_CHEST_TILES || !g_chest_tiles[idx].active) continue;
        open->x[part] = g_chest_tiles[idx].x;
        open->y[part] = g_chest_tiles[idx].y;
        open->z[part] = g_chest_tiles[idx].z;
    }
}


static void pex_net_send_craft_request(void) {
    if (!g_mp_connected || !g_mp_world_ready) return;
    PexNetCraftRequest req;
    memset(&req, 0, sizeof(req));
    if (g_screen == SCREEN_WORKBENCH) {
        req.grid_w = 3;
        req.grid_h = 3;
        req.slot_count = 9;
        for (int i = 0; i < 9; i++) {
            req.grid[i].id = g_workbench_grid[i].id;
            req.grid[i].count = g_workbench_grid[i].count > 0 ? 1 : 0;
            req.grid[i].damage = g_workbench_grid[i].damage;
        }
    } else {
        req.grid_w = 2;
        req.grid_h = 2;
        req.slot_count = 4;
        for (int i = 0; i < 4; i++) {
            req.grid[i].id = g_craft_grid[i].id;
            req.grid[i].count = g_craft_grid[i].count > 0 ? 1 : 0;
            req.grid[i].damage = g_craft_grid[i].damage;
        }
    }
    pex_net_send_packet(PEX_C2S_CRAFT_REQUEST, &req, sizeof(req));
}

static void pex_net_send_chest_open(void) {
    if (!g_mp_connected || !g_mp_world_ready || g_screen != SCREEN_CHEST) return;
    PexNetChestOpen open;
    pex_net_fill_open_chest_header(&open);
    if (open.chest_count <= 0) return;
    pex_net_send_packet(PEX_C2S_CHEST_OPEN, &open, sizeof(open));
}

static void pex_net_send_chest_update(void) {
    if (!g_mp_connected || !g_mp_world_ready || g_screen != SCREEN_CHEST) return;
    PexNetChestOpen open;
    pex_net_fill_open_chest_header(&open);
    if (open.chest_count <= 0) return;
    PexNetChestUpdate up;
    memset(&up, 0, sizeof(up));
    up.chest_count = open.chest_count;
    for (int part = 0; part < 2; part++) { up.x[part] = open.x[part]; up.y[part] = open.y[part]; up.z[part] = open.z[part]; }
    up.slot_count = chest_open_slot_count();
    if (up.slot_count > PEX_NET_CHEST_SLOTS) up.slot_count = PEX_NET_CHEST_SLOTS;
    for (int i = 0; i < up.slot_count; i++) {
        ItemStack *src = chest_get_open_slot_ptr(i);
        if (!src) continue;
        up.slots[i].id = src->id;
        up.slots[i].count = src->count;
        up.slots[i].damage = src->damage;
    }
    pex_net_send_packet(PEX_C2S_CHEST_UPDATE, &up, sizeof(up));
}

static void pex_net_send_chat(const char *text) {
    if (!g_mp_connected || !g_mp_world_ready || !text || !text[0]) return;
    PexNetChat chat;
    memset(&chat, 0, sizeof(chat));
    chat.player_id = g_mp_player_id;
    snprintf(chat.text, sizeof(chat.text), "%s", text);
    pex_net_send_packet(PEX_C2S_CHAT, &chat, sizeof(chat));
}

static void pex_net_disconnect(void) {
    pex_net_stop_rx_thread();
    if (g_mp_connect_socket != INVALID_SOCKET) {
        closesocket(g_mp_connect_socket);
        g_mp_connect_socket = INVALID_SOCKET;
    }
    if (g_mp_connect_thread_socket != INVALID_SOCKET) {
        closesocket(g_mp_connect_thread_socket);
        g_mp_connect_thread_socket = INVALID_SOCKET;
    }
    if (g_mp_connect_thread && InterlockedCompareExchange(&g_mp_connect_thread_done, 0, 0)) {
        CloseHandle(g_mp_connect_thread);
        g_mp_connect_thread = NULL;
    }
    g_mp_connecting = 0;
    if (g_mp_socket != INVALID_SOCKET) {
        if (g_mp_connected) {
            PexNetPacketHeader h;
            h.type = PEX_C2S_DISCONNECT;
            h.reserved = 0;
            h.size = 0;
            send(g_mp_socket, (const char *)&h, (int)sizeof(h), 0);
        }
        closesocket(g_mp_socket);
        g_mp_socket = INVALID_SOCKET;
    }
    g_mp_connected = 0;
    g_mp_world_ready = 0;
    g_mp_player_id = 0;
    g_mp_player_count = 0;
    memset(g_mp_players, 0, sizeof(g_mp_players));
    memset(g_mp_prev_players, 0, sizeof(g_mp_prev_players));
    net_free_render_players();
    memset(g_mp_render_drops, 0, sizeof(g_mp_render_drops));
    memset(g_falling_blocks, 0, sizeof(g_falling_blocks));
    g_mp_chunks_received = 0;
    g_mp_expected_chunks = 0;
    g_mp_connect_progress = 0;
    g_mp_recv_len = 0;
    g_mp_render_last_time = 0.0;
}
