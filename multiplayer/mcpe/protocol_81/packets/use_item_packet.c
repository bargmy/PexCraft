#include "use_item_packet.h"
#include "packet_codec.h"
#include "packet_ids.h"
int pex_mcpe_encode_use_item_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size,
                                    int x, int y, int z, int face,
                                    float pos_x, float pos_y, float pos_z,
                                    int slot, int item_id, int count, int damage) {
    if (out_size) *out_size = 0;
    if (slot < 0) slot = 0;
    if (count <= 0) count = 1;
    PexMcpeBuffer b; pex_mcpe_buffer_init(&b, out_data, out_capacity);
    if (!pex_mcpe_write_u8(&b, PEX_MCPE_PACKET_USE_ITEM) ||
        !pex_mcpe_write_i32_be(&b, x) ||
        !pex_mcpe_write_i32_be(&b, y) ||
        !pex_mcpe_write_i32_be(&b, z) ||
        !pex_mcpe_write_u8(&b, (uint8_t)face) ||
        !pex_mcpe_write_f32_be(&b, 0.5f) ||
        !pex_mcpe_write_f32_be(&b, 0.5f) ||
        !pex_mcpe_write_f32_be(&b, 0.5f) ||
        !pex_mcpe_write_f32_be(&b, pos_x) ||
        !pex_mcpe_write_f32_be(&b, pos_y) ||
        !pex_mcpe_write_f32_be(&b, pos_z) ||
        !pex_mcpe_write_i32_be(&b, slot)) return 0;
    if (item_id <= 0) {
        if (!pex_mcpe_write_i16_be(&b, 0)) return 0;
    } else {
        if (!pex_mcpe_write_i16_be(&b, (int16_t)item_id) ||
            !pex_mcpe_write_u8(&b, (uint8_t)count) ||
            !pex_mcpe_write_i16_be(&b, (int16_t)damage) ||
            !pex_mcpe_write_u8(&b, 0) || !pex_mcpe_write_u8(&b, 0)) return 0; /* little-endian short NBT length 0 */
    }
    if (out_size) *out_size = b.offset;
    return 1;
}
