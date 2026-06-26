#ifndef PEXCRAFT_MCPE_PROTOCOL_81_ENTITY_PACKETS_H
#define PEXCRAFT_MCPE_PROTOCOL_81_ENTITY_PACKETS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PexMcpeRemotePlayerInfo {
    uint64_t eid;
    char username[64];
    float x, y, z;
    float yaw, pitch;
    int held_item_id;
    int held_item_count;
    int held_item_damage;
} PexMcpeRemotePlayerInfo;

typedef struct PexMcpeEntityMoveInfo {
    uint64_t eid;
    float x, y, z;
    float yaw, pitch;
    int is_self;
} PexMcpeEntityMoveInfo;

int pex_mcpe_decode_add_player_packet(const uint8_t *data, size_t size, PexMcpeRemotePlayerInfo *out_info);
int pex_mcpe_decode_move_player_packet(const uint8_t *data, size_t size, PexMcpeEntityMoveInfo *out_info);
int pex_mcpe_decode_move_entity_packet(const uint8_t *data, size_t size, PexMcpeEntityMoveInfo *out_info);
int pex_mcpe_decode_remove_entity_packet(const uint8_t *data, size_t size, uint64_t *out_eid);

#ifdef __cplusplus
}
#endif

#endif /* PEXCRAFT_MCPE_PROTOCOL_81_ENTITY_PACKETS_H */
