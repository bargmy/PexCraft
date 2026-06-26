#include "request_chunk_radius_packet.h"
#include "packet_codec.h"
#include "packet_ids.h"

int pex_mcpe_encode_request_chunk_radius_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size, int radius) {
    if (out_size) *out_size = 0;
    if (radius < 1) radius = 1;
    if (radius > 16) radius = 16;
    PexMcpeBuffer b;
    pex_mcpe_buffer_init(&b, out_data, out_capacity);
    if (!pex_mcpe_write_u8(&b, PEX_MCPE_PACKET_REQUEST_CHUNK_RADIUS) ||
        !pex_mcpe_write_i32_be(&b, radius)) return 0;
    if (out_size) *out_size = b.offset;
    return 1;
}

int pex_mcpe_decode_chunk_radius_updated_packet(const uint8_t *data, size_t size, int *out_radius) {
    PexMcpeReadBuffer b;
    int32_t r = 0;
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_i32_be(&b, &r)) return 0;
    if (out_radius) *out_radius = r;
    return 1;
}
