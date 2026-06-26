#ifndef PEXCRAFT_MCPE_PROTOCOL_81_MOB_EQUIPMENT_PACKET_H
#define PEXCRAFT_MCPE_PROTOCOL_81_MOB_EQUIPMENT_PACKET_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int pex_mcpe_encode_mob_equipment_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size,
                                         uint64_t entity_id, int selected_slot,
                                         int item_id, int count, int damage);

#ifdef __cplusplus
}
#endif

#endif
