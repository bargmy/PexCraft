#ifndef PEXCRAFT_MCPE_PROTOCOL_81_PLAYER_ACTION_PACKET_H
#define PEXCRAFT_MCPE_PROTOCOL_81_PLAYER_ACTION_PACKET_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int pex_mcpe_encode_player_action_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size,
                                         uint64_t entity_id, int action, int x, int y, int z, int face);
#ifdef __cplusplus
}
#endif
#endif
