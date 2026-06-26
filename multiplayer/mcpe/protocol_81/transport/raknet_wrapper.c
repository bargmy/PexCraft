#define _POSIX_C_SOURCE 200112L
#include "raknet_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef SOCKET PexRawSocket;
#define PEX_RAW_INVALID_SOCKET INVALID_SOCKET
#define pex_raw_close closesocket
#else
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
typedef int PexRawSocket;
#define PEX_RAW_INVALID_SOCKET (-1)
#define pex_raw_close close
#endif

/*
 * This file intentionally keeps the old public name "raknet_wrapper" so the
 * rest of PexCraft does not need to change, but the implementation below is a
 * real RakLib/RakNet protocol 6 UDP client. Genisys uses RakLib::PROTOCOL = 6;
 * stock RakNet DLLs often do not complete that old MCPE handshake, so this raw
 * transport is the default path for protocol_81/82 Genisys joining.
 */

#define PEX_RAKLIB_PROTOCOL 6
#define PEX_RAKLIB_MTU 548
#define PEX_RAKLIB_MAX_PACKET 262144
#define PEX_RAKLIB_QUEUE_MAX 64
#define PEX_RAKLIB_SPLIT_SLOTS 16
#define PEX_RAKLIB_MAX_SPLITS 128

static const unsigned char PEX_RAKLIB_MAGIC[16] = {
    0x00,0xff,0xff,0x00,0xfe,0xfe,0xfe,0xfe,0xfd,0xfd,0xfd,0xfd,0x12,0x34,0x56,0x78
};

typedef enum PexRawState {
    PEX_RAW_CLOSED = 0,
    PEX_RAW_WAIT_OPEN_REPLY_1,
    PEX_RAW_WAIT_OPEN_REPLY_2,
    PEX_RAW_WAIT_SERVER_HANDSHAKE,
    PEX_RAW_CONNECTED,
    PEX_RAW_FAILED
} PexRawState;

typedef struct PexQueuedPayload {
    size_t size;
    unsigned char *data;
} PexQueuedPayload;

typedef struct PexSplitSlot {
    int active;
    int split_id;
    int split_count;
    int received_count;
    unsigned char *parts[PEX_RAKLIB_MAX_SPLITS];
    size_t sizes[PEX_RAKLIB_MAX_SPLITS];
} PexSplitSlot;

struct PexRakNetClient {
    PexRawSocket sock;
    struct sockaddr_storage remote_addr;
    socklen_t remote_len;
    char host[256];
    char remote_ip[64];
    uint16_t port;
    uint64_t client_id;
    uint64_t server_id;
    int mtu;
    int connected;
    PexRawState state;
    int send_seq;
    int message_index;
    int order_index;
    int split_id;
    uint64_t connect_started_ms;
    uint64_t last_handshake_ms;
    int handshake_attempts;
    char error[512];
    PexQueuedPayload queue[PEX_RAKLIB_QUEUE_MAX];
    int q_head;
    int q_tail;
    int q_count;
    PexSplitSlot splits[PEX_RAKLIB_SPLIT_SLOTS];
};

static uint64_t pex_raw_now_ms(void) {
#if defined(_WIN32)
    return (uint64_t)GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)(ts.tv_nsec / 1000000ull);
#endif
}

static void pex_raw_put_u16_be(unsigned char *p, uint16_t v) { p[0] = (unsigned char)(v >> 8); p[1] = (unsigned char)v; }
static void pex_raw_put_u32_be(unsigned char *p, uint32_t v) { p[0]=(unsigned char)(v>>24); p[1]=(unsigned char)(v>>16); p[2]=(unsigned char)(v>>8); p[3]=(unsigned char)v; }
static void pex_raw_put_u64_be(unsigned char *p, uint64_t v) { for (int i = 7; i >= 0; --i) *p++ = (unsigned char)(v >> (i * 8)); }
static uint16_t pex_raw_get_u16_be(const unsigned char *p) { return (uint16_t)(((uint16_t)p[0] << 8) | p[1]); }
static uint64_t pex_raw_get_u64_be(const unsigned char *p) { uint64_t v = 0; for (int i = 0; i < 8; ++i) v = (v << 8) | p[i]; return v; }
static void pex_raw_put_ltriad(unsigned char *p, int v) { p[0]=(unsigned char)v; p[1]=(unsigned char)(v>>8); p[2]=(unsigned char)(v>>16); }
static int pex_raw_get_ltriad(const unsigned char *p) { return (int)p[0] | ((int)p[1] << 8) | ((int)p[2] << 16); }

static int pex_raw_set_nonblocking(PexRawSocket s) {
#if defined(_WIN32)
    u_long mode = 1;
    return ioctlsocket(s, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) return 0;
    return fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

static int pex_raw_would_block(void) {
#if defined(_WIN32)
    int e = WSAGetLastError();
    return e == WSAEWOULDBLOCK || e == WSAEINPROGRESS;
#else
    return errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS;
#endif
}

static void pex_raw_make_client_id(PexRakNetClient *c) {
    uint64_t a = (uint64_t)time(NULL);
    uint64_t b = (uint64_t)(uintptr_t)c;
    uint64_t r = ((uint64_t)(rand() & 0xffff) << 48) ^ ((uint64_t)(rand() & 0xffff) << 32) ^ ((uint64_t)(rand() & 0xffff) << 16) ^ (uint64_t)(rand() & 0xffff);
    c->client_id = (a << 32) ^ b ^ r;
    if (c->client_id == 0) c->client_id = 0x5045584352414654ull;
}

static int pex_raw_sendto(PexRakNetClient *c, const unsigned char *data, size_t size) {
    if (!c || c->sock == PEX_RAW_INVALID_SOCKET || !data || size == 0) return 0;
    int n = sendto(c->sock, (const char*)data, (int)size, 0, (struct sockaddr*)&c->remote_addr, c->remote_len);
    if (n < 0) {
#if defined(_WIN32)
        snprintf(c->error, sizeof(c->error), "UDP sendto failed: WSAGetLastError=%d", WSAGetLastError());
#else
        snprintf(c->error, sizeof(c->error), "UDP sendto failed: %s", strerror(errno));
#endif
        return 0;
    }
    return 1;
}

static void pex_raw_write_address_v4(unsigned char *buf, size_t *off, const char *ip, uint16_t port) {
    unsigned int b0 = 127, b1 = 0, b2 = 0, b3 = 1;
    if (ip && ip[0]) sscanf(ip, "%u.%u.%u.%u", &b0, &b1, &b2, &b3);
    buf[(*off)++] = 4;
    buf[(*off)++] = (unsigned char)((~b0) & 0xff);
    buf[(*off)++] = (unsigned char)((~b1) & 0xff);
    buf[(*off)++] = (unsigned char)((~b2) & 0xff);
    buf[(*off)++] = (unsigned char)((~b3) & 0xff);
    pex_raw_put_u16_be(buf + *off, port);
    *off += 2;
}

static int pex_raw_send_open_request_1(PexRakNetClient *c) {
    unsigned char buf[PEX_RAKLIB_MTU];
    size_t off = 0;
    memset(buf, 0, sizeof(buf));
    buf[off++] = 0x05;
    memcpy(buf + off, PEX_RAKLIB_MAGIC, 16); off += 16;
    buf[off++] = PEX_RAKLIB_PROTOCOL;
    while (off < (size_t)c->mtu) buf[off++] = 0;
    c->last_handshake_ms = pex_raw_now_ms();
    c->handshake_attempts++;
    return pex_raw_sendto(c, buf, off);
}

static int pex_raw_send_open_request_2(PexRakNetClient *c) {
    unsigned char buf[128];
    size_t off = 0;
    buf[off++] = 0x07;
    memcpy(buf + off, PEX_RAKLIB_MAGIC, 16); off += 16;
    pex_raw_write_address_v4(buf, &off, c->remote_ip, c->port);
    pex_raw_put_u16_be(buf + off, (uint16_t)c->mtu); off += 2;
    pex_raw_put_u64_be(buf + off, c->client_id); off += 8;
    c->last_handshake_ms = pex_raw_now_ms();
    c->handshake_attempts++;
    return pex_raw_sendto(c, buf, off);
}

static int pex_raw_send_ack(PexRakNetClient *c, int seq) {
    unsigned char buf[16];
    size_t off = 0;
    buf[off++] = 0xc0;
    pex_raw_put_u16_be(buf + off, 1); off += 2; /* one record */
    buf[off++] = 0x01; /* single packet */
    pex_raw_put_ltriad(buf + off, seq); off += 3;
    return pex_raw_sendto(c, buf, off);
}

static int pex_raw_queue_payload(PexRakNetClient *c, const unsigned char *data, size_t size) {
    if (!c || !data || size == 0) return 0;
    if (c->q_count >= PEX_RAKLIB_QUEUE_MAX) return 0;
    unsigned char *copy = (unsigned char*)malloc(size);
    if (!copy) return 0;
    memcpy(copy, data, size);
    c->queue[c->q_tail].data = copy;
    c->queue[c->q_tail].size = size;
    c->q_tail = (c->q_tail + 1) % PEX_RAKLIB_QUEUE_MAX;
    c->q_count++;
    return 1;
}

static int pex_raw_pop_payload(PexRakNetClient *c, uint8_t *buffer, size_t buffer_size, size_t *out_size) {
    if (out_size) *out_size = 0;
    if (!c || c->q_count <= 0) return 0;
    PexQueuedPayload *p = &c->queue[c->q_head];
    if (p->size > buffer_size) {
        snprintf(c->error, sizeof(c->error), "RakLib payload too large for receive buffer: %lu", (unsigned long)p->size);
        return -1;
    }
    if (buffer && p->size) memcpy(buffer, p->data, p->size);
    if (out_size) *out_size = p->size;
    free(p->data);
    p->data = NULL;
    p->size = 0;
    c->q_head = (c->q_head + 1) % PEX_RAKLIB_QUEUE_MAX;
    c->q_count--;
    return 1;
}

static int pex_raw_send_data_packet(PexRakNetClient *c, const unsigned char *encap, size_t encap_size, int packet_id) {
    unsigned char *buf = (unsigned char*)malloc(encap_size + 4);
    if (!buf) return 0;
    size_t off = 0;
    buf[off++] = (unsigned char)packet_id;
    pex_raw_put_ltriad(buf + off, c->send_seq++); off += 3;
    memcpy(buf + off, encap, encap_size); off += encap_size;
    int ok = pex_raw_sendto(c, buf, off);
    free(buf);
    return ok;
}

static int pex_raw_build_encapsulated(unsigned char *out, size_t out_cap, size_t *out_size,
                                      const unsigned char *payload, size_t payload_size,
                                      int reliability, int message_index, int order_index,
                                      int has_split, int split_count, int split_id, int split_index) {
    size_t off = 0;
    if (!out || !out_size || !payload || payload_size == 0) return 0;
    if (out_cap < payload_size + 32) return 0;
    out[off++] = (unsigned char)((reliability << 5) | (has_split ? 0x10 : 0));
    pex_raw_put_u16_be(out + off, (uint16_t)(payload_size << 3)); off += 2;
    if (reliability > 0) {
        if (reliability >= 2 && reliability != 5) { pex_raw_put_ltriad(out + off, message_index); off += 3; }
        if (reliability <= 4 && reliability != 2) {
            pex_raw_put_ltriad(out + off, order_index); off += 3;
            out[off++] = 0;
        }
    }
    if (has_split) {
        pex_raw_put_u32_be(out + off, (uint32_t)split_count); off += 4;
        pex_raw_put_u16_be(out + off, (uint16_t)split_id); off += 2;
        pex_raw_put_u32_be(out + off, (uint32_t)split_index); off += 4;
    }
    memcpy(out + off, payload, payload_size); off += payload_size;
    *out_size = off;
    return 1;
}

static int pex_raw_send_encapsulated(PexRakNetClient *c, const unsigned char *payload, size_t payload_size, int reliability, int immediate) {
    if (!c || !payload || payload_size == 0) return 0;
    if (reliability < 0) reliability = 3;
    int packet_id = immediate ? 0x80 : 0x84;
    int max_payload = c->mtu - 64;
    if (max_payload < 256) max_payload = 256;

    int order_index = (reliability == 3) ? c->order_index++ : 0;
    if ((int)payload_size <= max_payload) {
        unsigned char *enc = (unsigned char*)malloc(payload_size + 64);
        if (!enc) return 0;
        size_t enc_size = 0;
        int msg_index = (reliability >= 2 && reliability != 5) ? c->message_index++ : 0;
        int ok = pex_raw_build_encapsulated(enc, payload_size + 64, &enc_size, payload, payload_size,
                                            reliability, msg_index, order_index, 0, 0, 0, 0) &&
                 pex_raw_send_data_packet(c, enc, enc_size, packet_id);
        free(enc);
        return ok;
    }

    int split_count = ((int)payload_size + max_payload - 1) / max_payload;
    if (split_count <= 0 || split_count >= PEX_RAKLIB_MAX_SPLITS) {
        snprintf(c->error, sizeof(c->error), "RakLib payload too large to split: %lu", (unsigned long)payload_size);
        return 0;
    }
    int sid = (c->split_id++ & 0xffff);
    for (int i = 0; i < split_count; ++i) {
        size_t pos = (size_t)i * (size_t)max_payload;
        size_t part = payload_size - pos;
        if (part > (size_t)max_payload) part = (size_t)max_payload;
        unsigned char *enc = (unsigned char*)malloc(part + 96);
        if (!enc) return 0;
        size_t enc_size = 0;
        int msg_index = (reliability >= 2 && reliability != 5) ? c->message_index++ : 0;
        int ok = pex_raw_build_encapsulated(enc, part + 96, &enc_size, payload + pos, part,
                                            reliability, msg_index, order_index, 1, split_count, sid, i) &&
                 pex_raw_send_data_packet(c, enc, enc_size, 0x80);
        free(enc);
        if (!ok) return 0;
    }
    return 1;
}

static int pex_raw_send_client_connect(PexRakNetClient *c) {
    unsigned char payload[32];
    size_t off = 0;
    payload[off++] = 0x09;
    pex_raw_put_u64_be(payload + off, c->client_id); off += 8;
    pex_raw_put_u64_be(payload + off, pex_raw_now_ms()); off += 8;
    payload[off++] = 0; /* no security */
    c->last_handshake_ms = pex_raw_now_ms();
    c->handshake_attempts++;
    return pex_raw_send_encapsulated(c, payload, off, 0, 1);
}

static int pex_raw_send_client_handshake(PexRakNetClient *c) {
    unsigned char payload[256];
    size_t off = 0;
    payload[off++] = 0x13;
    pex_raw_write_address_v4(payload, &off, c->remote_ip, c->port);
    for (int i = 0; i < 10; ++i) {
        pex_raw_write_address_v4(payload, &off, i == 0 ? "127.0.0.1" : "0.0.0.0", 0);
    }
    pex_raw_put_u64_be(payload + off, pex_raw_now_ms()); off += 8;
    pex_raw_put_u64_be(payload + off, pex_raw_now_ms() + 1000); off += 8;
    return pex_raw_send_encapsulated(c, payload, off, 0, 1);
}

static void pex_raw_clear_split(PexSplitSlot *slot) {
    if (!slot) return;
    for (int i = 0; i < PEX_RAKLIB_MAX_SPLITS; ++i) {
        free(slot->parts[i]);
        slot->parts[i] = NULL;
        slot->sizes[i] = 0;
    }
    memset(slot, 0, sizeof(*slot));
}

static int pex_raw_handle_split(PexRakNetClient *c, const unsigned char *payload, size_t payload_size,
                                int split_count, int split_id, int split_index) {
    if (!c || !payload || payload_size == 0 || split_count <= 0 || split_count >= PEX_RAKLIB_MAX_SPLITS || split_index < 0 || split_index >= split_count) return 0;
    PexSplitSlot *slot = NULL;
    for (int i = 0; i < PEX_RAKLIB_SPLIT_SLOTS; ++i) {
        if (c->splits[i].active && c->splits[i].split_id == split_id) { slot = &c->splits[i]; break; }
    }
    if (!slot) {
        for (int i = 0; i < PEX_RAKLIB_SPLIT_SLOTS; ++i) {
            if (!c->splits[i].active) { slot = &c->splits[i]; break; }
        }
    }
    if (!slot) {
        slot = &c->splits[0];
        pex_raw_clear_split(slot);
    }
    if (!slot->active) {
        slot->active = 1;
        slot->split_id = split_id;
        slot->split_count = split_count;
    }
    if (slot->split_count != split_count || slot->parts[split_index]) return 0;
    unsigned char *copy = (unsigned char*)malloc(payload_size);
    if (!copy) return 0;
    memcpy(copy, payload, payload_size);
    slot->parts[split_index] = copy;
    slot->sizes[split_index] = payload_size;
    slot->received_count++;
    if (slot->received_count != slot->split_count) return 1;

    size_t total = 0;
    for (int i = 0; i < slot->split_count; ++i) total += slot->sizes[i];
    unsigned char *joined = (unsigned char*)malloc(total);
    if (!joined) { pex_raw_clear_split(slot); return 0; }
    size_t off = 0;
    for (int i = 0; i < slot->split_count; ++i) {
        memcpy(joined + off, slot->parts[i], slot->sizes[i]);
        off += slot->sizes[i];
    }
    pex_raw_queue_payload(c, joined, total);
    free(joined);
    pex_raw_clear_split(slot);
    return 1;
}

static int pex_raw_process_internal(PexRakNetClient *c, const unsigned char *payload, size_t payload_size, PexRakNetPollType *out_type) {
    if (!c || !payload || payload_size == 0) return 0;
    unsigned char id = payload[0];
    if (id == 0x10 && c->state == PEX_RAW_WAIT_SERVER_HANDSHAKE) { /* SERVER_HANDSHAKE */
        pex_raw_send_client_handshake(c);
        c->state = PEX_RAW_CONNECTED;
        c->connected = 1;
        if (out_type) *out_type = PEX_RAKNET_POLL_CONNECTED;
        c->error[0] = 0;
        return 1;
    }
    if (id == 0x00) { /* server ping */
        unsigned char pong[16];
        size_t off = 0;
        pong[off++] = 0x03;
        if (payload_size >= 9) memcpy(pong + off, payload + 1, 8); else pex_raw_put_u64_be(pong + off, pex_raw_now_ms());
        off += 8;
        pex_raw_send_encapsulated(c, pong, off, 0, 0);
        return 1;
    }
    return 0;
}

static int pex_raw_process_encapsulated(PexRakNetClient *c, const unsigned char *data, size_t size, PexRakNetPollType *out_type) {
    if (!c || !data || size < 3) return 0;
    size_t off = 0;
    unsigned char flags = data[off++];
    int reliability = (flags & 0xe0) >> 5;
    int has_split = (flags & 0x10) != 0;
    int bit_len = pex_raw_get_u16_be(data + off); off += 2;
    size_t payload_len = (size_t)((bit_len + 7) / 8);
    if (reliability > 0) {
        if (reliability >= 2 && reliability != 5) off += 3;
        if (reliability <= 4 && reliability != 2) off += 4;
    }
    int split_count = 0, split_id = 0, split_index = 0;
    if (has_split) {
        if (off + 10 > size) return 0;
        split_count = (int)(((uint32_t)data[off] << 24) | ((uint32_t)data[off+1] << 16) | ((uint32_t)data[off+2] << 8) | data[off+3]); off += 4;
        split_id = (int)pex_raw_get_u16_be(data + off); off += 2;
        split_index = (int)(((uint32_t)data[off] << 24) | ((uint32_t)data[off+1] << 16) | ((uint32_t)data[off+2] << 8) | data[off+3]); off += 4;
    }
    if (off + payload_len > size) return 0;
    const unsigned char *payload = data + off;
    if (has_split) return pex_raw_handle_split(c, payload, payload_len, split_count, split_id, split_index);
    if (payload_len > 0 && payload[0] < 0x80) return pex_raw_process_internal(c, payload, payload_len, out_type);
    return pex_raw_queue_payload(c, payload, payload_len);
}

static int pex_raw_process_data_packet(PexRakNetClient *c, const unsigned char *data, size_t size, PexRakNetPollType *out_type) {
    if (!c || !data || size < 4) return 0;
    int seq = pex_raw_get_ltriad(data + 1);
    pex_raw_send_ack(c, seq);
    size_t off = 4;
    while (off + 3 <= size) {
        size_t start = off;
        unsigned char flags = data[off];
        int reliability = (flags & 0xe0) >> 5;
        int has_split = (flags & 0x10) != 0;
        int bit_len = pex_raw_get_u16_be(data + off + 1);
        size_t payload_len = (size_t)((bit_len + 7) / 8);
        off += 3;
        if (reliability > 0) {
            if (reliability >= 2 && reliability != 5) off += 3;
            if (reliability <= 4 && reliability != 2) off += 4;
        }
        if (has_split) off += 10;
        if (off + payload_len > size) break;
        size_t enc_size = (off + payload_len) - start;
        pex_raw_process_encapsulated(c, data + start, enc_size, out_type);
        off += payload_len;
        if (out_type && *out_type == PEX_RAKNET_POLL_CONNECTED) return 1;
    }
    return 1;
}

static int pex_raw_recv_one(PexRakNetClient *c, unsigned char *buf, size_t cap, size_t *out_size) {
    if (out_size) *out_size = 0;
    if (!c || c->sock == PEX_RAW_INVALID_SOCKET) return -1;
    struct sockaddr_storage from;
    socklen_t from_len = sizeof(from);
    int n = recvfrom(c->sock, (char*)buf, (int)cap, 0, (struct sockaddr*)&from, &from_len);
    if (n < 0) {
        if (pex_raw_would_block()) return 0;
#if defined(_WIN32)
        snprintf(c->error, sizeof(c->error), "UDP recvfrom failed: WSAGetLastError=%d", WSAGetLastError());
#else
        snprintf(c->error, sizeof(c->error), "UDP recvfrom failed: %s", strerror(errno));
#endif
        return -1;
    }
    if (out_size) *out_size = (size_t)n;
    return 1;
}

static void pex_raw_drive_handshake(PexRakNetClient *c) {
    if (!c || c->state == PEX_RAW_CONNECTED || c->state == PEX_RAW_FAILED || c->state == PEX_RAW_CLOSED) return;
    uint64_t now = pex_raw_now_ms();
    if (now - c->connect_started_ms > 12000) {
        c->state = PEX_RAW_FAILED;
        snprintf(c->error, sizeof(c->error), "RakLib protocol 6 connection timed out before server handshake.");
        return;
    }
    if (now - c->last_handshake_ms < 700) return;
    switch (c->state) {
        case PEX_RAW_WAIT_OPEN_REPLY_1: pex_raw_send_open_request_1(c); break;
        case PEX_RAW_WAIT_OPEN_REPLY_2: pex_raw_send_open_request_2(c); break;
        case PEX_RAW_WAIT_SERVER_HANDSHAKE: pex_raw_send_client_connect(c); break;
        default: break;
    }
}

PexRakNetClient *pex_raknet_client_create(void) {
#if defined(_WIN32)
    static int wsa_started = 0;
    if (!wsa_started) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) == 0) wsa_started = 1;
    }
#endif
    PexRakNetClient *c = (PexRakNetClient*)calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->sock = PEX_RAW_INVALID_SOCKET;
    c->mtu = PEX_RAKLIB_MTU;
    c->state = PEX_RAW_CLOSED;
    c->split_id = 1;
    srand((unsigned int)time(NULL) ^ (unsigned int)(uintptr_t)c);
    pex_raw_make_client_id(c);
    snprintf(c->error, sizeof(c->error), "RakLib v6 client created.");
    return c;
}

int pex_raknet_client_connect(PexRakNetClient *client, const char *host, uint16_t port) {
    if (!client) return 0;
    snprintf(client->host, sizeof(client->host), "%s", host && host[0] ? host : "127.0.0.1");
    client->port = port ? port : 19132;

    char port_text[16];
    snprintf(port_text, sizeof(port_text), "%u", (unsigned)client->port);
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    int gai = getaddrinfo(client->host, port_text, &hints, &res);
    if (gai != 0 || !res) {
        snprintf(client->error, sizeof(client->error), "Could not resolve %s:%s for RakLib UDP.", client->host, port_text);
        return 0;
    }

    client->sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (client->sock == PEX_RAW_INVALID_SOCKET) {
        freeaddrinfo(res);
        snprintf(client->error, sizeof(client->error), "Could not create UDP socket for RakLib.");
        return 0;
    }
    memcpy(&client->remote_addr, res->ai_addr, res->ai_addrlen);
    client->remote_len = (socklen_t)res->ai_addrlen;
    struct sockaddr_in *sin = (struct sockaddr_in*)res->ai_addr;
    inet_ntop(AF_INET, &sin->sin_addr, client->remote_ip, sizeof(client->remote_ip));
    freeaddrinfo(res);

    if (!pex_raw_set_nonblocking(client->sock)) {
        snprintf(client->error, sizeof(client->error), "Could not set UDP socket non-blocking.");
        pex_raw_close(client->sock);
        client->sock = PEX_RAW_INVALID_SOCKET;
        return 0;
    }

    client->state = PEX_RAW_WAIT_OPEN_REPLY_1;
    client->connect_started_ms = pex_raw_now_ms();
    client->last_handshake_ms = 0;
    client->handshake_attempts = 0;
    snprintf(client->error, sizeof(client->error), "RakLib v6 opening connection to %s:%u.", client->remote_ip, (unsigned)client->port);
    return pex_raw_send_open_request_1(client);
}

int pex_raknet_client_send(PexRakNetClient *client, const uint8_t *data, size_t size, int reliability) {
    if (!client || !data || size == 0) return 0;
    if (client->state != PEX_RAW_CONNECTED) {
        snprintf(client->error, sizeof(client->error), "Tried to send MCPE packet before RakLib connected.");
        return 0;
    }
    int rel = reliability ? reliability : 3; /* RELIABLE_ORDERED */
    if (!pex_raw_send_encapsulated(client, data, size, rel, 0)) return 0;
    return 1;
}

int pex_raknet_client_poll(PexRakNetClient *client, uint8_t *buffer, size_t buffer_size, size_t *out_size, PexRakNetPollType *out_type) {
    if (out_size) *out_size = 0;
    if (out_type) *out_type = PEX_RAKNET_POLL_NONE;
    if (!client) return 0;

    int popped = pex_raw_pop_payload(client, buffer, buffer_size, out_size);
    if (popped < 0) { if (out_type) *out_type = PEX_RAKNET_POLL_FAILED; return 0; }
    if (popped > 0) { if (out_type) *out_type = PEX_RAKNET_POLL_DATA; return 1; }

    pex_raw_drive_handshake(client);
    if (client->state == PEX_RAW_FAILED) { if (out_type) *out_type = PEX_RAKNET_POLL_FAILED; return 1; }

    unsigned char pkt[PEX_RAKLIB_MAX_PACKET];
    size_t n = 0;
    int rr = pex_raw_recv_one(client, pkt, sizeof(pkt), &n);
    if (rr < 0) { if (out_type) *out_type = PEX_RAKNET_POLL_FAILED; return 0; }
    if (rr == 0 || n == 0) return 1;

    unsigned char id = pkt[0];
    if (id == 0x06 && client->state == PEX_RAW_WAIT_OPEN_REPLY_1 && n >= 28 && memcmp(pkt + 1, PEX_RAKLIB_MAGIC, 16) == 0) {
        client->server_id = pex_raw_get_u64_be(pkt + 17);
        client->mtu = (int)pex_raw_get_u16_be(pkt + 26);
        if (client->mtu <= 0 || client->mtu > 1432) client->mtu = PEX_RAKLIB_MTU;
        client->state = PEX_RAW_WAIT_OPEN_REPLY_2;
        snprintf(client->error, sizeof(client->error), "RakLib open reply 1 received; MTU=%d.", client->mtu);
        pex_raw_send_open_request_2(client);
        return 1;
    }
    if (id == 0x08 && client->state == PEX_RAW_WAIT_OPEN_REPLY_2 && n >= 35 && memcmp(pkt + 1, PEX_RAKLIB_MAGIC, 16) == 0) {
        client->server_id = pex_raw_get_u64_be(pkt + 17);
        size_t off = 25;
        if (off + 7 <= n) off += 7;
        if (off + 2 <= n) client->mtu = (int)pex_raw_get_u16_be(pkt + off);
        if (client->mtu <= 0 || client->mtu > 1432) client->mtu = PEX_RAKLIB_MTU;
        client->state = PEX_RAW_WAIT_SERVER_HANDSHAKE;
        snprintf(client->error, sizeof(client->error), "RakLib open reply 2 received; sending client connect.");
        pex_raw_send_client_connect(client);
        return 1;
    }
    if (id >= 0x80 && id <= 0x8f) {
        PexRakNetPollType t = PEX_RAKNET_POLL_NONE;
        pex_raw_process_data_packet(client, pkt, n, &t);
        if (t == PEX_RAKNET_POLL_CONNECTED) {
            if (out_type) *out_type = PEX_RAKNET_POLL_CONNECTED;
            return 1;
        }
        popped = pex_raw_pop_payload(client, buffer, buffer_size, out_size);
        if (popped < 0) { if (out_type) *out_type = PEX_RAKNET_POLL_FAILED; return 0; }
        if (popped > 0) { if (out_type) *out_type = PEX_RAKNET_POLL_DATA; return 1; }
        return 1;
    }
    if (id == 0xc0 || id == 0xa0) {
        /* ACK/NACK. We keep this MVP simple and do not maintain a resend queue yet. */
        return 1;
    }
    return 1;
}

int pex_raknet_client_receive(PexRakNetClient *client, uint8_t *buffer, size_t buffer_size, size_t *out_size) {
    PexRakNetPollType type;
    if (!pex_raknet_client_poll(client, buffer, buffer_size, out_size, &type)) return -1;
    return type == PEX_RAKNET_POLL_DATA ? 1 : 0;
}

int pex_raknet_client_is_connected(PexRakNetClient *client) {
    return client && client->connected;
}

const char *pex_raknet_client_last_error(PexRakNetClient *client) {
    return client && client->error[0] ? client->error : "RakLib client error";
}

void pex_raknet_client_destroy(PexRakNetClient *client) {
    if (!client) return;
    for (int i = 0; i < PEX_RAKLIB_QUEUE_MAX; ++i) free(client->queue[i].data);
    for (int i = 0; i < PEX_RAKLIB_SPLIT_SLOTS; ++i) pex_raw_clear_split(&client->splits[i]);
    if (client->sock != PEX_RAW_INVALID_SOCKET) {
        pex_raw_close(client->sock);
        client->sock = PEX_RAW_INVALID_SOCKET;
    }
    free(client);
}
