#include "full_chunk_data_packet.h"
#include "packet_codec.h"
#include <string.h>

int pex_mcpe_decode_full_chunk_data_packet(const uint8_t *data, size_t size, PexMcpeFullChunkData *out_chunk) {
    if (out_chunk) memset(out_chunk, 0, sizeof(*out_chunk));
    if (!out_chunk) return 0;
    PexMcpeReadBuffer b;
    int32_t payload_len;
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_i32_be(&b, &out_chunk->chunk_x)) return 0;
    if (!pex_mcpe_read_i32_be(&b, &out_chunk->chunk_z)) return 0;
    if (!pex_mcpe_read_u8(&b, &out_chunk->order)) return 0;
    if (!pex_mcpe_read_i32_be(&b, &payload_len) || payload_len < 0) return 0;
    if ((size_t)payload_len > pex_mcpe_buffer_remaining(&b)) return 0;
    if (!pex_mcpe_read_bytes(&b, &out_chunk->payload, (size_t)payload_len)) return 0;
    out_chunk->payload_size = (size_t)payload_len;
    return 1;
}
