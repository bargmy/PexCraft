#include "join_session.h"
#include "../transport/raknet_wrapper.h"
#include "../packets/packet_ids.h"
#include "../packets/packet_codec.h"
#include "../packets/login_packet.h"
#include "../packets/request_chunk_radius_packet.h"
#include "../packets/move_player_packet.h"
#include "../packets/batch_packet.h"
#include "../packets/text_packet.h"
#include "../packets/player_action_packet.h"
#include "../packets/remove_block_packet.h"
#include "../packets/mob_equipment_packet.h"
#include "../packets/use_item_packet.h"
#include "../packets/update_block_packet.h"
#include "../packets/entity_packets.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void pex_mcpe_join_set_status(PexMcpeJoinSession *s, const char *msg) {
    if (!s) return;
    snprintf(s->status_text, sizeof(s->status_text), "%s", msg ? msg : "");
}

void pex_mcpe_join_session_init(PexMcpeJoinSession *session,
                                const char *host,
                                uint16_t port,
                                const char *username) {
    if (!session) return;
    memset(session, 0, sizeof(*session));
    snprintf(session->host, sizeof(session->host), "%s", host && host[0] ? host : "127.0.0.1");
    session->port = port ? port : 19132;
    snprintf(session->username, sizeof(session->username), "%s", username && username[0] ? username : "PexPlayer");
    session->protocol_version = 82;
    session->chunk_radius = 4;
    session->state = PEX_MCPE_JOIN_IDLE;
    pex_mcpe_join_set_status(session, "Connecting");
}

void pex_mcpe_join_session_set_callbacks(PexMcpeJoinSession *session,
                                         const PexMcpeJoinCallbacks *callbacks,
                                         void *userdata) {
    if (!session) return;
    if (callbacks) session->callbacks = *callbacks;
    else memset(&session->callbacks, 0, sizeof(session->callbacks));
    session->callback_userdata = userdata;
}

void pex_mcpe_join_session_set_skin(PexMcpeJoinSession *session,
                                    const uint8_t *skin_rgba,
                                    int width,
                                    int height) {
    if (!session) return;
    if (skin_rgba && width == 64 && (height == 32 || height == 64)) {
        session->skin_rgba = skin_rgba;
        session->skin_width = width;
        session->skin_height = height;
    } else {
        session->skin_rgba = NULL;
        session->skin_width = 0;
        session->skin_height = 0;
    }
}

static int pex_mcpe_send_login(PexMcpeJoinSession *s) {
    uint8_t buf[65536];
    size_t n = 0;
    char address[320];
    snprintf(address, sizeof(address), "%s:%u", s->host, (unsigned)s->port);
    if (!pex_mcpe_encode_login_packet_with_skin(buf, sizeof(buf), &n, s->username, s->protocol_version, address,
                                                s->skin_rgba, s->skin_width, s->skin_height)) {
        pex_mcpe_join_set_status(s, "Could not encode LoginPacket.");
        s->state = PEX_MCPE_JOIN_FAILED;
        return 0;
    }
    if (!pex_raknet_client_send(s->raknet, buf, n, 0)) {
        pex_mcpe_join_set_status(s, pex_raknet_client_last_error(s->raknet));
        s->state = PEX_MCPE_JOIN_FAILED;
        return 0;
    }
    s->state = PEX_MCPE_JOIN_WAIT_PLAY_STATUS;
    pex_mcpe_join_set_status(s, "Connecting");
    return 1;
}

static int pex_mcpe_send_request_radius(PexMcpeJoinSession *s) {
    uint8_t buf[64];
    size_t n = 0;
    if (!pex_mcpe_encode_request_chunk_radius_packet(buf, sizeof(buf), &n, s->chunk_radius)) return 0;
    if (!pex_raknet_client_send(s->raknet, buf, n, 0)) return 0;
    s->state = PEX_MCPE_JOIN_REQUEST_RADIUS_SENT;
    pex_mcpe_join_set_status(s, "Building terrain");
    return 1;
}

static void pex_mcpe_process_one_packet(PexMcpeJoinSession *s, const uint8_t *packet, size_t size, int from_batch);

static void pex_mcpe_process_batch(PexMcpeJoinSession *s, const uint8_t *body, size_t body_size) {
    uint8_t *plain = NULL;
    size_t plain_size = 0;
    if (!pex_mcpe_decode_batch_packet(body, body_size, &plain, &plain_size)) return;
    size_t off = 0;
    while (off + 4 <= plain_size) {
        int32_t len = ((int32_t)plain[off] << 24) | ((int32_t)plain[off+1] << 16) | ((int32_t)plain[off+2] << 8) | (int32_t)plain[off+3];
        off += 4;
        if (len <= 0 || off + (size_t)len > plain_size) break;
        pex_mcpe_process_one_packet(s, plain + off, (size_t)len, 1);
        off += (size_t)len;
    }
    free(plain);
}

static void pex_mcpe_process_one_packet(PexMcpeJoinSession *s, const uint8_t *packet, size_t size, int from_batch) {
    if (!s || !packet || size == 0) return;
    (void)from_batch;
    if (packet[0] == PEX_MCPE_RAKLIB_GAME_PACKET) {
        packet++;
        size--;
        if (size == 0) return;
    }
    uint8_t id = packet[0];
    const uint8_t *body = packet + 1;
    size_t body_size = size - 1;
    switch (id) {
        case PEX_MCPE_PACKET_BATCH:
            pex_mcpe_process_batch(s, body, body_size);
            break;
        case PEX_MCPE_PACKET_PLAY_STATUS: {
            PexMcpePlayStatus st;
            if (pex_mcpe_decode_play_status_packet(body, body_size, &st)) {
                if (s->callbacks.on_status) s->callbacks.on_status(s->callback_userdata, st);
                if (st == PEX_MCPE_PLAY_STATUS_LOGIN_SUCCESS) {
                    s->state = PEX_MCPE_JOIN_WAIT_START_GAME;
                    pex_mcpe_join_set_status(s, "Connecting");
                } else if (st == PEX_MCPE_PLAY_STATUS_PLAYER_SPAWN) {
                    s->spawn_status_received = 1;
                    if (s->chunks_received > 0) {
                        s->state = PEX_MCPE_JOIN_WORLD_READY;
                        pex_mcpe_join_set_status(s, "Done!");
                    } else {
                        pex_mcpe_join_set_status(s, "Building terrain");
                    }
                } else if (st == PEX_MCPE_PLAY_STATUS_LOGIN_FAILED_CLIENT) {
                    s->state = PEX_MCPE_JOIN_FAILED;
                    pex_mcpe_join_set_status(s, "Login failed: client protocol rejected.");
                } else if (st == PEX_MCPE_PLAY_STATUS_LOGIN_FAILED_SERVER) {
                    s->state = PEX_MCPE_JOIN_FAILED;
                    pex_mcpe_join_set_status(s, "Login failed: server protocol rejected.");
                }
            }
            break;
        }
        case PEX_MCPE_PACKET_START_GAME: {
            PexMcpeStartGameInfo info;
            if (pex_mcpe_decode_start_game_packet(body, body_size, &info)) {
                s->entity_id = (uint64_t)info.entity_id;
                s->entity_id_valid = 1;
                s->x = info.x;
                s->y = info.y;
                s->z = info.z;
                if (s->callbacks.on_start_game) s->callbacks.on_start_game(s->callback_userdata, &info);
                pex_mcpe_send_request_radius(s);
            }
            break;
        }
        case PEX_MCPE_PACKET_CHUNK_RADIUS_UPDATED: {
            int r = 0;
            if (pex_mcpe_decode_chunk_radius_updated_packet(body, body_size, &r)) {
                s->server_chunk_radius = r;
                pex_mcpe_join_set_status(s, "Building terrain");
            }
            break;
        }
        case PEX_MCPE_PACKET_FULL_CHUNK_DATA: {
            PexMcpeFullChunkData chunk;
            if (pex_mcpe_decode_full_chunk_data_packet(body, body_size, &chunk)) {
                s->chunks_received++;
                if (s->callbacks.on_chunk) s->callbacks.on_chunk(s->callback_userdata, &chunk);
                if (s->state == PEX_MCPE_JOIN_REQUEST_RADIUS_SENT && s->chunks_received > 0) {
                    if (s->spawn_status_received) {
                        s->state = PEX_MCPE_JOIN_WORLD_READY;
                        pex_mcpe_join_set_status(s, "Done!");
                    } else {
                        pex_mcpe_join_set_status(s, "Building terrain");
                    }
                }
            }
            break;
        }
        case PEX_MCPE_PACKET_SET_TIME: {
            PexMcpeReadBuffer tb;
            int32_t t = 0;
            uint8_t started = 0;
            pex_mcpe_read_buffer_init(&tb, body, body_size);
            if (pex_mcpe_read_i32_be(&tb, &t)) {
                (void)pex_mcpe_read_u8(&tb, &started);
                if (s->callbacks.on_set_time) s->callbacks.on_set_time(s->callback_userdata, (int)t);
            }
            break;
        }
        case PEX_MCPE_PACKET_TEXT: {
            char text[512];
            if (pex_mcpe_decode_text_packet(body, body_size, text, sizeof(text)) && s->callbacks.on_text) {
                s->callbacks.on_text(s->callback_userdata, text);
            }
            break;
        }
        case PEX_MCPE_PACKET_UPDATE_BLOCK: {
            PexMcpeUpdateBlockRecord records[64];
            size_t count = 0;
            if (pex_mcpe_decode_update_block_records(body, body_size, records, 64, &count) && s->callbacks.on_block_update) {
                for (size_t i = 0; i < count; ++i) {
                    s->callbacks.on_block_update(s->callback_userdata, records[i].x, records[i].y, records[i].z, records[i].id, records[i].meta);
                }
            }
            break;
        }
        case PEX_MCPE_PACKET_ADD_PLAYER: {
            PexMcpeRemotePlayerInfo player;
            if (pex_mcpe_decode_add_player_packet(body, body_size, &player) && s->callbacks.on_add_player) {
                s->callbacks.on_add_player(s->callback_userdata, &player);
            }
            break;
        }
        case PEX_MCPE_PACKET_MOVE_PLAYER: {
            PexMcpeEntityMoveInfo move;
            if (pex_mcpe_decode_move_player_packet(body, body_size, &move)) {
                if (move.is_self) {
                    /* Server correction/teleport for this client. */
                    s->x = move.x;
                    s->y = move.y;
                    s->z = move.z;
                    s->yaw = move.yaw;
                    s->pitch = move.pitch;
                }
                if (s->callbacks.on_move_entity) s->callbacks.on_move_entity(s->callback_userdata, &move);
            }
            break;
        }
        case PEX_MCPE_PACKET_MOVE_ENTITY: {
            PexMcpeEntityMoveInfo move;
            if (pex_mcpe_decode_move_entity_packet(body, body_size, &move) && s->callbacks.on_move_entity) {
                s->callbacks.on_move_entity(s->callback_userdata, &move);
            }
            break;
        }
        case PEX_MCPE_PACKET_REMOVE_ENTITY: {
            uint64_t eid = 0;
            if (pex_mcpe_decode_remove_entity_packet(body, body_size, &eid) && s->callbacks.on_remove_entity) {
                s->callbacks.on_remove_entity(s->callback_userdata, eid);
            }
            break;
        }
        case PEX_MCPE_PACKET_PLAYER_LIST: {
            PexMcpePlayerListSkin entries[16];
            size_t count = 0;
            memset(entries, 0, sizeof(entries));
            if (pex_mcpe_decode_player_list_packet(body, body_size, entries, 16, &count) && s->callbacks.on_player_skin) {
                for (size_t i = 0; i < count; ++i) {
                    s->callbacks.on_player_skin(s->callback_userdata, &entries[i]);
                }
            }
            pex_mcpe_free_player_list_entries(entries, count);
            break;
        }
        case PEX_MCPE_PACKET_DISCONNECT: {
            char text[256];
            text[0] = 0;
            PexMcpeReadBuffer rb;
            pex_mcpe_read_buffer_init(&rb, body, body_size);
            pex_mcpe_read_string(&rb, text, sizeof(text));
            s->state = PEX_MCPE_JOIN_FAILED;
            pex_mcpe_join_set_status(s, text[0] ? text : "Disconnected by server.");
            if (s->callbacks.on_disconnect) s->callbacks.on_disconnect(s->callback_userdata, s->status_text);
            break;
        }
        default:
            break;
    }
}

int pex_mcpe_join_session_tick(PexMcpeJoinSession *session) {
    if (!session) return 0;
    if (session->state == PEX_MCPE_JOIN_IDLE || session->state == PEX_MCPE_JOIN_RAKNET_CONNECTING) {
        if (!session->raknet) {
            session->raknet = pex_raknet_client_create();
            if (!session->raknet) {
                session->state = PEX_MCPE_JOIN_FAILED;
                pex_mcpe_join_set_status(session, "Could not create RakNet client.");
                return 0;
            }
            if (!pex_raknet_client_connect(session->raknet, session->host, session->port)) {
                session->state = PEX_MCPE_JOIN_FAILED;
                pex_mcpe_join_set_status(session, pex_raknet_client_last_error(session->raknet));
                return 0;
            }
            session->state = PEX_MCPE_JOIN_RAKNET_CONNECTING;
            pex_mcpe_join_set_status(session, "Connecting");
        }
    }

    uint8_t buf[262144];
    for (int i = 0; i < 128; ++i) {
        size_t n = 0;
        PexRakNetPollType type = PEX_RAKNET_POLL_NONE;
        if (!pex_raknet_client_poll(session->raknet, buf, sizeof(buf), &n, &type)) {
            session->state = PEX_MCPE_JOIN_FAILED;
            pex_mcpe_join_set_status(session, pex_raknet_client_last_error(session->raknet));
            return 0;
        }
        if (type == PEX_RAKNET_POLL_NONE) break;
        if (type == PEX_RAKNET_POLL_CONNECTED) {
            session->state = PEX_MCPE_JOIN_RAKNET_CONNECTED;
            pex_mcpe_join_set_status(session, "Connecting");
            pex_mcpe_send_login(session);
        } else if (type == PEX_RAKNET_POLL_DATA && n > 0) {
            pex_mcpe_process_one_packet(session, buf, n, 0);
        } else if (type == PEX_RAKNET_POLL_FAILED || type == PEX_RAKNET_POLL_DISCONNECTED) {
            session->state = PEX_MCPE_JOIN_FAILED;
            pex_mcpe_join_set_status(session, pex_raknet_client_last_error(session->raknet));
            if (session->callbacks.on_disconnect) session->callbacks.on_disconnect(session->callback_userdata, session->status_text);
            return 0;
        }
    }
    return session->state != PEX_MCPE_JOIN_FAILED;
}

int pex_mcpe_join_session_send_move(PexMcpeJoinSession *session,
                                    float x, float y, float z,
                                    float yaw, float pitch) {
    if (!session || !session->raknet || session->state == PEX_MCPE_JOIN_FAILED || !session->entity_id_valid) return 0;
    uint8_t buf[128];
    size_t n;
    if (!pex_mcpe_encode_move_player_packet(buf, sizeof(buf), &n, session->entity_id, x, y, z, yaw, pitch)) return 0;
    session->x = x; session->y = y; session->z = z; session->yaw = yaw; session->pitch = pitch;
    return pex_raknet_client_send(session->raknet, buf, n, 0);
}

int pex_mcpe_join_session_send_chat(PexMcpeJoinSession *session, const char *message) {
    if (!session || !session->raknet || !message || !message[0] || !session->spawn_status_received) return 0;
    uint8_t buf[1024];
    size_t n;
    if (!pex_mcpe_encode_chat_packet(buf, sizeof(buf), &n, session->username, message)) return 0;
    return pex_raknet_client_send(session->raknet, buf, n, 0);
}


static int pex_mcpe_join_session_send_equipment(PexMcpeJoinSession *session,
                                                    int slot, int item_id, int count, int damage) {
    if (!session || !session->raknet || !session->entity_id_valid) return 0;
    uint8_t buf[128];
    size_t n = 0;
    if (!pex_mcpe_encode_mob_equipment_packet(buf, sizeof(buf), &n, session->entity_id,
                                             slot, item_id, count, damage)) return 0;
    return pex_raknet_client_send(session->raknet, buf, n, 0);
}

int pex_mcpe_join_session_send_break(PexMcpeJoinSession *session, int x, int y, int z, int face,
                                     int slot, int held_item_id, int held_count, int held_damage) {
    if (!session || !session->raknet || !session->entity_id_valid) return 0;
    uint8_t buf[128];
    size_t n = 0;
    pex_mcpe_join_session_send_equipment(session, slot, held_item_id, held_count, held_damage);
    if (pex_mcpe_encode_player_action_packet(buf, sizeof(buf), &n, session->entity_id, 0, x, y, z, face)) {
        pex_raknet_client_send(session->raknet, buf, n, 0);
    }
    if (pex_mcpe_encode_remove_block_packet(buf, sizeof(buf), &n, session->entity_id, x, y, z)) {
        return pex_raknet_client_send(session->raknet, buf, n, 0);
    }
    return 0;
}

int pex_mcpe_join_session_send_use_item(PexMcpeJoinSession *session, int x, int y, int z, int face,
                                        float pos_x, float pos_y, float pos_z,
                                        int slot, int item_id, int count, int damage) {
    if (!session || !session->raknet) return 0;
    uint8_t buf[256];
    size_t n;
    pex_mcpe_join_session_send_equipment(session, slot, item_id, count, damage);
    if (!pex_mcpe_encode_use_item_packet(buf, sizeof(buf), &n, x, y, z, face, pos_x, pos_y, pos_z, slot, item_id, count, damage)) return 0;
    return pex_raknet_client_send(session->raknet, buf, n, 0);
}

void pex_mcpe_join_session_disconnect(PexMcpeJoinSession *session) {
    if (!session) return;
    if (session->raknet) {
        pex_raknet_client_destroy(session->raknet);
        session->raknet = NULL;
    }
    session->state = PEX_MCPE_JOIN_IDLE;
}

const char *pex_mcpe_join_session_status(const PexMcpeJoinSession *session) {
    return session && session->status_text[0] ? session->status_text : "Connecting";
}

int pex_mcpe_join_session_progress(const PexMcpeJoinSession *session) {
    if (!session) return 0;
    switch (session->state) {
        case PEX_MCPE_JOIN_IDLE: return 5;
        case PEX_MCPE_JOIN_RAKNET_CONNECTING: return 18;
        case PEX_MCPE_JOIN_RAKNET_CONNECTED: return 30;
        case PEX_MCPE_JOIN_LOGIN_SENT:
        case PEX_MCPE_JOIN_WAIT_PLAY_STATUS: return 45;
        case PEX_MCPE_JOIN_WAIT_START_GAME: return 60;
        case PEX_MCPE_JOIN_REQUEST_RADIUS_SENT: return 75 + (session->chunks_received > 24 ? 20 : session->chunks_received);
        case PEX_MCPE_JOIN_WORLD_READY: return 100;
        case PEX_MCPE_JOIN_FAILED: return 0;
        default: return 0;
    }
}
