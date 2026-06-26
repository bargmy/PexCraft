#include "login_packet.h"
#include "packet_codec.h"
#include "packet_ids.h"
#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int pex_b64_encode(const uint8_t *data, size_t size, char *out, size_t out_cap) {
    size_t need = ((size + 2) / 3) * 4 + 1;
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

static int pex_make_jwt(const char *payload_json, char *out, size_t out_cap) {
    const char *header_json = "{\"alg\":\"none\",\"typ\":\"JWT\"}";
    char h[128];
    char p[4096];
    if (!pex_b64_encode((const uint8_t *)header_json, strlen(header_json), h, sizeof(h))) return 0;
    if (!pex_b64_encode((const uint8_t *)payload_json, strlen(payload_json), p, sizeof(p))) return 0;
    return snprintf(out, out_cap, "%s.%s.", h, p) > 0 && strlen(out) < out_cap;
}

int pex_mcpe_encode_login_packet(uint8_t *out_data,
                                 size_t out_capacity,
                                 size_t *out_size,
                                 const char *username,
                                 int protocol_version,
                                 const char *server_address) {
    if (out_size) *out_size = 0;
    if (!out_data || out_capacity < 64) return 0;
    if (!username || !username[0]) username = "PexPlayer";
    if (!server_address) server_address = "";
    if (protocol_version != 81 && protocol_version != 82) protocol_version = 82;

    char safe_name[64];
    char safe_server[256];
    pex_json_escape(username, safe_name, sizeof(safe_name));
    pex_json_escape(server_address, safe_server, sizeof(safe_server));

    char uuid[48];
    pex_uuid_v4_text(uuid, sizeof(uuid));

    char chain_payload[1024];
    snprintf(chain_payload, sizeof(chain_payload),
             "{\"extraData\":{\"displayName\":\"%s\",\"identity\":\"%s\"},\"identityPublicKey\":\"PEXDUMMYKEY\"}",
             safe_name, uuid);

    char chain_jwt[2048];
    if (!pex_make_jwt(chain_payload, chain_jwt, sizeof(chain_jwt))) return 0;

    char chain_json[2400];
    snprintf(chain_json, sizeof(chain_json), "{\"chain\":[\"%s\"]}", chain_jwt);

    uint8_t skin[64 * 64 * 4];
    for (size_t i = 0; i < sizeof(skin); i += 4) {
        skin[i + 0] = 0xb8;
        skin[i + 1] = 0x7a;
        skin[i + 2] = 0x4a;
        skin[i + 3] = 0xff;
    }
    char skin_b64[22000];
    if (!pex_b64_encode(skin, sizeof(skin), skin_b64, sizeof(skin_b64))) return 0;

    long long client_id = ((long long)time(NULL) << 20) ^ (long long)(rand() & 0xfffff);
    char skin_payload[24000];
    snprintf(skin_payload, sizeof(skin_payload),
             "{\"ClientRandomId\":%lld,\"ServerAddress\":\"%s\",\"SkinId\":\"Standard_Custom\",\"SkinData\":\"%s\"}",
             client_id, safe_server, skin_b64);

    char skin_jwt[33000];
    if (!pex_make_jwt(skin_payload, skin_jwt, sizeof(skin_jwt))) return 0;

    size_t chain_len = strlen(chain_json);
    size_t skin_len = strlen(skin_jwt);
    size_t plain_cap = 4 + chain_len + 4 + skin_len;
    uint8_t *plain = (uint8_t *)malloc(plain_cap);
    if (!plain) return 0;

    PexMcpeBuffer plain_buf;
    pex_mcpe_buffer_init(&plain_buf, plain, plain_cap);
    if (!pex_mcpe_write_i32_le(&plain_buf, (int32_t)chain_len) ||
        !pex_mcpe_write_bytes(&plain_buf, chain_json, chain_len) ||
        !pex_mcpe_write_i32_le(&plain_buf, (int32_t)skin_len) ||
        !pex_mcpe_write_bytes(&plain_buf, skin_jwt, skin_len)) {
        free(plain);
        return 0;
    }

    uLongf compressed_cap = compressBound((uLong)plain_buf.offset);
    uint8_t *compressed = (uint8_t *)malloc((size_t)compressed_cap);
    if (!compressed) { free(plain); return 0; }
    int zr = compress2(compressed, &compressed_cap, plain, (uLong)plain_buf.offset, Z_BEST_SPEED);
    free(plain);
    if (zr != Z_OK) { free(compressed); return 0; }

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
