#ifndef PEXCRAFT_MCPE_PROTOCOL_81_MOVE_PLAYER_PACKET_H
#define PEXCRAFT_MCPE_PROTOCOL_81_MOVE_PLAYER_PACKET_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int pex_mcpe_encode_move_player_packet(uint8_t *out_data,
                                       size_t out_capacity,
                                       size_t *out_size,
                                       uint64_t entity_id,
                                       float x,
                                       float y,
                                       float z,
                                       float yaw,
                                       float pitch);

#ifdef __cplusplus
}
#endif

#endif /* PEXCRAFT_MCPE_PROTOCOL_81_MOVE_PLAYER_PACKET_H */
