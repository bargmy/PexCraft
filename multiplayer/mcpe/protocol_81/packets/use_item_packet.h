#ifndef PEXCRAFT_MCPE_PROTOCOL_81_USE_ITEM_PACKET_H
#define PEXCRAFT_MCPE_PROTOCOL_81_USE_ITEM_PACKET_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int pex_mcpe_encode_use_item_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size,
                                    int x, int y, int z, int face,
                                    float pos_x, float pos_y, float pos_z,
                                    int slot, int item_id, int count, int damage);
#ifdef __cplusplus
}
#endif
#endif
