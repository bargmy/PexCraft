#include "mob_equipment_packet.h"
#include "packet_codec.h"
#include "packet_ids.h"

static int pex_mcpe_write_slot(PexMcpeBuffer *b, int item_id, int count, int damage) {
    if (count <= 0) count = 1;
    if (item_id <= 0) return pex_mcpe_write_i16_be(b, 0);
    return pex_mcpe_write_i16_be(b, (int16_t)item_id) &&
           pex_mcpe_write_u8(b, (uint8_t)count) &&
           pex_mcpe_write_i16_be(b, (int16_t)damage) &&
           pex_mcpe_write_u8(b, 0) && pex_mcpe_write_u8(b, 0); /* little-endian NBT length 0 */
}

int pex_mcpe_encode_mob_equipment_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size,
                                         uint64_t entity_id, int selected_slot,
                                         int item_id, int count, int damage) {
    if (out_size) *out_size = 0;
    if (selected_slot < 0) selected_slot = 0;
    if (selected_slot > 8) selected_slot = 8;
    PexMcpeBuffer b;
    pex_mcpe_buffer_init(&b, out_data, out_capacity);
    int wire_slot = item_id <= 0 ? 0x28 : selected_slot + 9;
    if (!pex_mcpe_write_u8(&b, PEX_MCPE_PACKET_MOB_EQUIPMENT) ||
        !pex_mcpe_write_i64_be(&b, (int64_t)entity_id) ||
        !pex_mcpe_write_slot(&b, item_id, count, damage) ||
        !pex_mcpe_write_u8(&b, (uint8_t)wire_slot) ||
        !pex_mcpe_write_u8(&b, (uint8_t)selected_slot)) return 0;
    if (out_size) *out_size = b.offset;
    return 1;
}
