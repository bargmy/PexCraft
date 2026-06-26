#ifndef PEXCRAFT_MCPE_PROTOCOL_81_JOIN_SESSION_H
#define PEXCRAFT_MCPE_PROTOCOL_81_JOIN_SESSION_H

#include <stddef.h>
#include <stdint.h>
#include "../packets/play_status_packet.h"
#include "../packets/start_game_packet.h"
#include "../packets/full_chunk_data_packet.h"
#include "../packets/entity_packets.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PexMcpeJoinState {
    PEX_MCPE_JOIN_IDLE = 0,
    PEX_MCPE_JOIN_RAKNET_CONNECTING,
    PEX_MCPE_JOIN_RAKNET_CONNECTED,
    PEX_MCPE_JOIN_LOGIN_SENT,
    PEX_MCPE_JOIN_WAIT_PLAY_STATUS,
    PEX_MCPE_JOIN_WAIT_START_GAME,
    PEX_MCPE_JOIN_REQUEST_RADIUS_SENT,
    PEX_MCPE_JOIN_WORLD_READY,
    PEX_MCPE_JOIN_FAILED
} PexMcpeJoinState;

struct PexRakNetClient;

typedef struct PexMcpeJoinCallbacks {
    void (*on_status)(void *userdata, PexMcpePlayStatus status);
    void (*on_start_game)(void *userdata, const PexMcpeStartGameInfo *info);
    void (*on_chunk)(void *userdata, const PexMcpeFullChunkData *chunk);
    void (*on_text)(void *userdata, const char *text);
    void (*on_block_update)(void *userdata, int x, int y, int z, int id, int meta);
    void (*on_add_player)(void *userdata, const PexMcpeRemotePlayerInfo *player);
    void (*on_move_entity)(void *userdata, const PexMcpeEntityMoveInfo *move);
    void (*on_remove_entity)(void *userdata, uint64_t eid);
    void (*on_disconnect)(void *userdata, const char *message);
} PexMcpeJoinCallbacks;

typedef struct PexMcpeJoinSession {
    char host[256];
    uint16_t port;
    char username[64];
    int protocol_version;
    int chunk_radius;
    int server_chunk_radius;
    int chunks_received;
    int spawn_status_received;
    uint64_t entity_id;
    int entity_id_valid;
    float x, y, z, yaw, pitch;
    PexMcpeJoinState state;
    char status_text[256];
    struct PexRakNetClient *raknet;
    PexMcpeJoinCallbacks callbacks;
    void *callback_userdata;
} PexMcpeJoinSession;

void pex_mcpe_join_session_init(PexMcpeJoinSession *session,
                                const char *host,
                                uint16_t port,
                                const char *username);
void pex_mcpe_join_session_set_callbacks(PexMcpeJoinSession *session,
                                         const PexMcpeJoinCallbacks *callbacks,
                                         void *userdata);
int pex_mcpe_join_session_tick(PexMcpeJoinSession *session);
int pex_mcpe_join_session_send_move(PexMcpeJoinSession *session,
                                    float x, float y, float z,
                                    float yaw, float pitch);
int pex_mcpe_join_session_send_chat(PexMcpeJoinSession *session, const char *message);
int pex_mcpe_join_session_send_break(PexMcpeJoinSession *session, int x, int y, int z, int face);
int pex_mcpe_join_session_send_use_item(PexMcpeJoinSession *session, int x, int y, int z, int face,
                                        float pos_x, float pos_y, float pos_z,
                                        int slot, int item_id, int count, int damage);
void pex_mcpe_join_session_disconnect(PexMcpeJoinSession *session);
const char *pex_mcpe_join_session_status(const PexMcpeJoinSession *session);
int pex_mcpe_join_session_progress(const PexMcpeJoinSession *session);

#ifdef __cplusplus
}
#endif

#endif /* PEXCRAFT_MCPE_PROTOCOL_81_JOIN_SESSION_H */
