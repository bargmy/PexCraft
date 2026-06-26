#include "login_packet.h"
#include "packet_codec.h"
#include "packet_ids.h"
#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static size_t pex_b64_encoded_size(size_t size) {
    return ((size + 2) / 3) * 4;
}

static int pex_b64_encode(const uint8_t *data, size_t size, char *out, size_t out_cap) {
    size_t need = pex_b64_encoded_size(size) + 1;
    if (!out || out_cap < need) return 0;
    size_t oi = 0;
    for (size_t i = 0; i < size; i += 3) {
        uint32_t v = (uint32_t)data[i] << 16;
        int have2 = (i + 1 < size);
        int have3 = (i + 2 < size);
        if (have2) v |= (uint32_t)data[i + 1] << 8;
        if (have3) v |= (uint32_t)data[i + 2];
        out[oi++] = b64_table[(v >> 18) & 0x3f];
        out[oi++] = b64_table[(v >> 12) & 0x3f];
        out[oi++] = have2 ? b64_table[(v >> 6) & 0x3f] : '=';
        out[oi++] = have3 ? b64_table[v & 0x3f] : '=';
    }
    out[oi] = 0;
    return 1;
}

static char *pex_b64_encode_alloc(const uint8_t *data, size_t size) {
    size_t cap = pex_b64_encoded_size(size) + 1;
    char *out = (char *)malloc(cap);
    if (!out) return NULL;
    if (!pex_b64_encode(data, size, out, cap)) {
        free(out);
        return NULL;
    }
    return out;
}

static void pex_uuid_v4_text(char *out, size_t out_cap) {
    unsigned char b[16];
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)(uintptr_t)out;
    srand(seed);
    for (int i = 0; i < 16; ++i) b[i] = (unsigned char)(rand() & 0xff);
    b[6] = (unsigned char)((b[6] & 0x0f) | 0x40);
    b[8] = (unsigned char)((b[8] & 0x3f) | 0x80);
    snprintf(out, out_cap,
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7],
             b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
}

static void pex_normalize_username(const char *src, char *dst, size_t dst_cap) {
    size_t j = 0;
    if (!dst || dst_cap == 0) return;
    if (!src || !src[0]) src = "PexPlayer";
    for (size_t i = 0; src[i] && j + 1 < dst_cap && j < 16; ++i) {
        unsigned char c = (unsigned char)src[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
            dst[j++] = (char)c;
        }
    }
    while (j < 3 && j + 1 < dst_cap) {
        char fill = (j == 0) ? 'P' : 'x';
        dst[j++] = fill;
    }
    dst[j] = 0;
    if (strcmp(dst, "rcon") == 0 || strcmp(dst, "console") == 0 || strcmp(dst, "RCON") == 0 || strcmp(dst, "CONSOLE") == 0) {
        snprintf(dst, dst_cap, "PexPlayer");
    }
}

static void pex_json_escape(const char *src, char *dst, size_t dst_cap) {
    size_t j = 0;
    if (!dst || dst_cap == 0) return;
    if (!src) src = "";
    for (size_t i = 0; src[i] && j + 2 < dst_cap; ++i) {
        unsigned char c = (unsigned char)src[i];
        if (c == '"' || c == '\\') {
            if (j + 2 >= dst_cap) break;
            dst[j++] = '\\';
            dst[j++] = (char)c;
        } else if (c >= 32 && c < 127) {
            dst[j++] = (char)c;
        }
    }
    dst[j] = 0;
}

static char *pex_make_jwt_alloc(const char *payload_json) {
    const char *header_json = "{\"alg\":\"none\",\"typ\":\"JWT\"}";
    char *h = pex_b64_encode_alloc((const uint8_t *)header_json, strlen(header_json));
    char *p = pex_b64_encode_alloc((const uint8_t *)payload_json, strlen(payload_json));
    if (!h || !p) {
        free(h);
        free(p);
        return NULL;
    }
    size_t need = strlen(h) + 1 + strlen(p) + 1 + 1;
    char *out = (char *)malloc(need);
    if (!out) {
        free(h);
        free(p);
        return NULL;
    }
    snprintf(out, need, "%s.%s.", h, p);
    free(h);
    free(p);
    return out;
}

int pex_mcpe_encode_login_packet_with_skin(uint8_t *out_data,
                                           size_t out_capacity,
                                           size_t *out_size,
                                           const char *username,
                                           int protocol_version,
                                           const char *server_address,
                                           const uint8_t *skin_rgba,
                                           int skin_width,
                                           int skin_height) {
    if (out_size) *out_size = 0;
    if (!out_data || out_capacity < 64) return 0;
    if (!server_address) server_address = "";
    if (protocol_version != 81 && protocol_version != 82) protocol_version = 82;

    char normalized_name[32];
    pex_normalize_username(username, normalized_name, sizeof(normalized_name));

    char safe_name[64];
    char safe_server[256];
    pex_json_escape(normalized_name, safe_name, sizeof(safe_name));
    pex_json_escape(server_address, safe_server, sizeof(safe_server));

    char uuid[48];
    pex_uuid_v4_text(uuid, sizeof(uuid));

    char chain_payload[1024];
    snprintf(chain_payload, sizeof(chain_payload),
             "{\"extraData\":{\"displayName\":\"%s\",\"identity\":\"%s\"},\"identityPublicKey\":\"PEXDUMMYKEY\"}",
             safe_name, uuid);

    char *chain_jwt = pex_make_jwt_alloc(chain_payload);
    if (!chain_jwt) return 0;

    size_t chain_json_cap = strlen(chain_jwt) + 32;
    char *chain_json = (char *)malloc(chain_json_cap);
    if (!chain_json) {
        free(chain_jwt);
        return 0;
    }
    snprintf(chain_json, chain_json_cap, "{\"chain\":[\"%s\"]}", chain_jwt);
    free(chain_jwt);

    /* Genisys accepts 64x32x4 or 64x64x4. Prefer PexCraft's selected Steve/custom skin. */
    uint8_t fallback_skin[64 * 32 * 4];
    for (size_t i = 0; i < sizeof(fallback_skin); i += 4) {
        fallback_skin[i + 0] = 0xb8;
        fallback_skin[i + 1] = 0x7a;
        fallback_skin[i + 2] = 0x4a;
        fallback_skin[i + 3] = 0xff;
    }
    const uint8_t *skin_src = fallback_skin;
    size_t skin_bytes = sizeof(fallback_skin);
    if (skin_rgba && skin_width == 64 && (skin_height == 32 || skin_height == 64)) {
        skin_src = skin_rgba;
        skin_bytes = (size_t)skin_width * (size_t)skin_height * 4u;
    }
    char *skin_b64 = pex_b64_encode_alloc(skin_src, skin_bytes);
    if (!skin_b64) {
        free(chain_json);
        return 0;
    }

    long long client_id = ((long long)time(NULL) << 20) ^ (long long)(rand() & 0xfffff);
    size_t skin_payload_cap = strlen(skin_b64) + strlen(safe_server) + 256;
    char *skin_payload = (char *)malloc(skin_payload_cap);
    if (!skin_payload) {
        free(chain_json);
        free(skin_b64);
        return 0;
    }
    snprintf(skin_payload, skin_payload_cap,
             "{\"ClientRandomId\":%lld,\"ServerAddress\":\"%s\",\"SkinId\":\"Standard_Custom\",\"SkinData\":\"%s\"}",
             client_id, safe_server, skin_b64);
    free(skin_b64);

    char *skin_jwt = pex_make_jwt_alloc(skin_payload);
    free(skin_payload);
    if (!skin_jwt) {
        free(chain_json);
        return 0;
    }

    size_t chain_len = strlen(chain_json);
    size_t skin_len = strlen(skin_jwt);
    if (chain_len > 0x7fffffffU || skin_len > 0x7fffffffU) {
        free(chain_json);
        free(skin_jwt);
        return 0;
    }
    size_t plain_cap = 4 + chain_len + 4 + skin_len;
    uint8_t *plain = (uint8_t *)malloc(plain_cap);
    if (!plain) {
        free(chain_json);
        free(skin_jwt);
        return 0;
    }

    PexMcpeBuffer plain_buf;
    pex_mcpe_buffer_init(&plain_buf, plain, plain_cap);
    if (!pex_mcpe_write_i32_le(&plain_buf, (int32_t)chain_len) ||
        !pex_mcpe_write_bytes(&plain_buf, chain_json, chain_len) ||
        !pex_mcpe_write_i32_le(&plain_buf, (int32_t)skin_len) ||
        !pex_mcpe_write_bytes(&plain_buf, skin_jwt, skin_len)) {
        free(chain_json);
        free(skin_jwt);
        free(plain);
        return 0;
    }
    free(chain_json);
    free(skin_jwt);

    uLongf compressed_cap = compressBound((uLong)plain_buf.offset);
    uint8_t *compressed = (uint8_t *)malloc((size_t)compressed_cap);
    if (!compressed) {
        free(plain);
        return 0;
    }
    int zr = compress2(compressed, &compressed_cap, plain, (uLong)plain_buf.offset, Z_BEST_SPEED);
    free(plain);
    if (zr != Z_OK || compressed_cap > 0x7fffffffUL) {
        free(compressed);
        return 0;
    }

    PexMcpeBuffer out;
    pex_mcpe_buffer_init(&out, out_data, out_capacity);
    int ok = pex_mcpe_write_u8(&out, PEX_MCPE_RAKLIB_GAME_PACKET) &&
             pex_mcpe_write_u8(&out, PEX_MCPE_PACKET_LOGIN) &&
             pex_mcpe_write_i32_be(&out, protocol_version) &&
             pex_mcpe_write_i32_be(&out, (int32_t)compressed_cap) &&
             pex_mcpe_write_bytes(&out, compressed, (size_t)compressed_cap);
    free(compressed);
    if (!ok) return 0;
    if (out_size) *out_size = out.offset;
    return 1;
}

int pex_mcpe_encode_login_packet(uint8_t *out_data,
                                 size_t out_capacity,
                                 size_t *out_size,
                                 const char *username,
                                 int protocol_version,
                                 const char *server_address) {
    return pex_mcpe_encode_login_packet_with_skin(out_data, out_capacity, out_size, username, protocol_version, server_address, NULL, 0, 0);
}
