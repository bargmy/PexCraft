#include "remove_block_packet.h"
#include "packet_codec.h"
#include "packet_ids.h"

int pex_mcpe_encode_remove_block_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size,
                                        uint64_t entity_id, int x, int y, int z) {
    if (out_size) *out_size = 0;
    if (y < 0) y = 0;
    if (y > 255) y = 255;
    PexMcpeBuffer b;
    pex_mcpe_buffer_init(&b, out_data, out_capacity);
    if (!pex_mcpe_write_u8(&b, PEX_MCPE_PACKET_REMOVE_BLOCK) ||
        !pex_mcpe_write_i64_be(&b, (int64_t)entity_id) ||
        !pex_mcpe_write_i32_be(&b, x) ||
        !pex_mcpe_write_i32_be(&b, z) ||
        !pex_mcpe_write_u8(&b, (uint8_t)y)) return 0;
    if (out_size) *out_size = b.offset;
    return 1;
}
