#ifndef PEXCRAFT_MCPE_PROTOCOL_81_REMOVE_BLOCK_PACKET_H
#define PEXCRAFT_MCPE_PROTOCOL_81_REMOVE_BLOCK_PACKET_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int pex_mcpe_encode_remove_block_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size,
                                        uint64_t entity_id, int x, int y, int z);

#ifdef __cplusplus
}
#endif

#endif
