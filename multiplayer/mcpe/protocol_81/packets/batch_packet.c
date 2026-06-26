#include "batch_packet.h"
#include "packet_codec.h"
#include <zlib.h>
#include <stdlib.h>
#include <string.h>

int pex_mcpe_decode_batch_packet(const uint8_t *data, size_t size, uint8_t **out_payload, size_t *out_size) {
    if (out_payload) *out_payload = NULL;
    if (out_size) *out_size = 0;
    PexMcpeReadBuffer b;
    int32_t compressed_len;
    const uint8_t *compressed;
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_i32_be(&b, &compressed_len) || compressed_len < 0) return 0;
    if ((size_t)compressed_len > pex_mcpe_buffer_remaining(&b)) return 0;
    if (!pex_mcpe_read_bytes(&b, &compressed, (size_t)compressed_len)) return 0;

    uLongf cap = 1024 * 1024;
    uint8_t *buf = NULL;
    for (int attempt = 0; attempt < 6; ++attempt) {
        uint8_t *nbuf = (uint8_t *)realloc(buf, (size_t)cap);
        if (!nbuf) { free(buf); return 0; }
        buf = nbuf;
        uLongf actual = cap;
        int zr = uncompress(buf, &actual, compressed, (uLong)compressed_len);
        if (zr == Z_OK) {
            if (out_payload) *out_payload = buf; else free(buf);
            if (out_size) *out_size = (size_t)actual;
            return 1;
        }
        if (zr != Z_BUF_ERROR) break;
        cap *= 2;
    }
    free(buf);
    return 0;
}
