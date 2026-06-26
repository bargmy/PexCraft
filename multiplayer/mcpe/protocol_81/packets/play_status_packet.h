#ifndef PEXCRAFT_MCPE_PROTOCOL_81_PLAY_STATUS_PACKET_H
#define PEXCRAFT_MCPE_PROTOCOL_81_PLAY_STATUS_PACKET_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PexMcpePlayStatus {
    PEX_MCPE_PLAY_STATUS_LOGIN_SUCCESS = 0,
    PEX_MCPE_PLAY_STATUS_LOGIN_FAILED_CLIENT = 1,
    PEX_MCPE_PLAY_STATUS_LOGIN_FAILED_SERVER = 2,
    PEX_MCPE_PLAY_STATUS_PLAYER_SPAWN = 3,
    PEX_MCPE_PLAY_STATUS_UNKNOWN = 255
} PexMcpePlayStatus;

int pex_mcpe_decode_play_status_packet(const uint8_t *data, size_t size, PexMcpePlayStatus *out_status);

#ifdef __cplusplus
}
#endif

#endif /* PEXCRAFT_MCPE_PROTOCOL_81_PLAY_STATUS_PACKET_H */
