#ifndef PEXCRAFT_MCPE_PROTOCOL_81_START_GAME_PACKET_H
#define PEXCRAFT_MCPE_PROTOCOL_81_START_GAME_PACKET_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PexMcpeStartGameInfo {
    int64_t entity_id;
    uint64_t runtime_id;
    int game_mode;
    float x;
    float y;
    float z;
    int seed;
    int dimension;
    int generator;
    int spawn_x;
    int spawn_y;
    int spawn_z;
} PexMcpeStartGameInfo;

int pex_mcpe_decode_start_game_packet(const uint8_t *data, size_t size, PexMcpeStartGameInfo *out_info);

#ifdef __cplusplus
}
#endif

#endif /* PEXCRAFT_MCPE_PROTOCOL_81_START_GAME_PACKET_H */
