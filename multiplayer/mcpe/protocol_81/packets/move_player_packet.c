#include "move_player_packet.h"
#include "packet_codec.h"
#include "packet_ids.h"

int pex_mcpe_encode_move_player_packet(uint8_t *out_data,
                                       size_t out_capacity,
                                       size_t *out_size,
                                       uint64_t entity_id,
                                       float x,
                                       float y,
                                       float z,
                                       float yaw,
                                       float pitch) {
    if (out_size) *out_size = 0;
    PexMcpeBuffer b;
    pex_mcpe_buffer_init(&b, out_data, out_capacity);
    if (!pex_mcpe_write_u8(&b, PEX_MCPE_PACKET_MOVE_PLAYER) ||
        !pex_mcpe_write_i64_be(&b, (int64_t)entity_id) ||
        !pex_mcpe_write_f32_be(&b, x) ||
        !pex_mcpe_write_f32_be(&b, y) ||
        !pex_mcpe_write_f32_be(&b, z) ||
        !pex_mcpe_write_f32_be(&b, yaw) ||
        !pex_mcpe_write_f32_be(&b, yaw) ||
        !pex_mcpe_write_f32_be(&b, pitch) ||
        !pex_mcpe_write_u8(&b, 0) ||
        !pex_mcpe_write_u8(&b, 1)) {
        return 0;
    }
    if (out_size) *out_size = b.offset;
    return 1;
}
