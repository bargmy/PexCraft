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
#include <zlib.h>

static void pex_mcpe_join_set_status(PexMcpeJoinSession *s, const char *msg) {
    if (!s) return;
    snprintf(s->status_text, sizeof(s->status_text), "%s", msg ? msg : "");
}


static int pex_mcpe_send_packet_batched(PexMcpeJoinSession *session, const uint8_t *data, size_t size) {
    if (!session || !session->raknet || !data || size == 0) return 0;
    if (size > 0x7fffffffUL) return 0;

    /* Batch payload format used by MCPE 0.15.x / protocol 81-82:
       [inner_packet_length:4 bytes BE] [inner_packet_bytes...]
       Then zlib-compress that payload and wrap it as:
       [0xfe RakLib game marker] [0x06 BatchPacket ID] [compressed_length:4 bytes BE] [zlib bytes]
    */
    size_t plain_size = 4 + size;
    uint8_t *plain = (uint8_t *)malloc(plain_size);
    if (!plain) return 0;
    plain[0] = (uint8_t)((size >> 24) & 0xff);
    plain[1] = (uint8_t)((size >> 16) & 0xff);
    plain[2] = (uint8_t)((size >> 8) & 0xff);
    plain[3] = (uint8_t)(size & 0xff);
    memcpy(plain + 4, data, size);

    uLongf compressed_size = compressBound((uLong)plain_size);
    uint8_t *compressed = (uint8_t *)malloc((size_t)compressed_size);
    if (!compressed) {
        free(plain);
        return 0;
    }
    int zr = compress2(compressed, &compressed_size, plain, (uLong)plain_size, Z_BEST_SPEED);
    free(plain);
    if (zr != Z_OK || compressed_size > 0x7fffffffUL) {
        free(compressed);
        return 0;
    }

    size_t final_size = 1 + 1 + 4 + (size_t)compressed_size;
    uint8_t *final_buf = (uint8_t *)malloc(final_size);
    if (!final_buf) {
        free(compressed);
        return 0;
    }
    final_buf[0] = PEX_MCPE_RAKLIB_GAME_PACKET;
    final_buf[1] = PEX_MCPE_PACKET_BATCH;
    final_buf[2] = (uint8_t)(((size_t)compressed_size >> 24) & 0xff);
    final_buf[3] = (uint8_t)(((size_t)compressed_size >> 16) & 0xff);
    final_buf[4] = (uint8_t)(((size_t)compressed_size >> 8) & 0xff);
    final_buf[5] = (uint8_t)((size_t)compressed_size & 0xff);
    memcpy(final_buf + 6, compressed, (size_t)compressed_size);
    free(compressed);

    int ok = pex_raknet_client_send(session->raknet, final_buf, final_size, 0);
    free(final_buf);
    return ok;
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
    pex_mcpe_join_set_status(session, "Locating server");
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
    if (!pex_mcpe_send_packet_batched(s, buf, n)) {
        pex_mcpe_join_set_status(s, pex_raknet_client_last_error(s->raknet));
        s->state = PEX_MCPE_JOIN_FAILED;
        return 0;
    }
    s->state = PEX_MCPE_JOIN_WAIT_PLAY_STATUS;
    pex_mcpe_join_set_status(s, "Locating server");
    return 1;
}

static int pex_mcpe_send_request_radius(PexMcpeJoinSession *s) {
    uint8_t buf[64];
    size_t n = 0;
    if (!pex_mcpe_encode_request_chunk_radius_packet(buf, sizeof(buf), &n, s->chunk_radius)) return 0;
    if (!pex_mcpe_send_packet_batched(s, buf, n)) return 0;
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
                    pex_mcpe_join_set_status(s, "Locating server");
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
        case PEX_MCPE_PACKET_MOB_EQUIPMENT: {
            PexMcpeMobEquipmentInfo eq;
            if (pex_mcpe_decode_mob_equipment_packet(body, body_size, &eq) && s->callbacks.on_mob_equipment) {
                s->callbacks.on_mob_equipment(s->callback_userdata, &eq);
            }
            break;
        }
        case PEX_MCPE_PACKET_ANIMATE: {
            PexMcpeAnimateInfo anim;
            if (pex_mcpe_decode_animate_packet(body, body_size, &anim) && s->callbacks.on_animate) {
                s->callbacks.on_animate(s->callback_userdata, &anim);
            }
            break;
        }
        case PEX_MCPE_PACKET_ADD_ITEM_ENTITY: {
            PexMcpeDroppedItemInfo item;
            if (pex_mcpe_decode_add_item_entity_packet(body, body_size, &item) && s->callbacks.on_add_item) {
                s->callbacks.on_add_item(s->callback_userdata, &item);
            }
            break;
        }
        case PEX_MCPE_PACKET_CONTAINER_SET_CONTENT: {
            PexMcpeContainerContentInfo content;
            if (pex_mcpe_decode_container_set_content_packet(body, body_size, &content) && s->callbacks.on_inventory_content) {
                s->callbacks.on_inventory_content(s->callback_userdata, &content);
            }
            break;
        }
        case PEX_MCPE_PACKET_CONTAINER_SET_SLOT: {
            PexMcpeContainerSlotInfo slot;
            if (pex_mcpe_decode_container_set_slot_packet(body, body_size, &slot) && s->callbacks.on_inventory_slot) {
                s->callbacks.on_inventory_slot(s->callback_userdata, &slot);
            }
            break;
        }
        case PEX_MCPE_PACKET_SET_HEALTH: {
            PexMcpeReadBuffer hb;
            int32_t health = 0;
            pex_mcpe_read_buffer_init(&hb, body, body_size);
            if (pex_mcpe_read_i32_be(&hb, &health) && s->callbacks.on_set_health) {
                s->callbacks.on_set_health(s->callback_userdata, (int)health);
            }
            break;
        }
        case PEX_MCPE_PACKET_ENTITY_EVENT: {
            PexMcpeEntityEventInfo ev;
            if (pex_mcpe_decode_entity_event_packet(body, body_size, &ev) && s->callbacks.on_entity_event) {
                s->callbacks.on_entity_event(s->callback_userdata, &ev);
            }
            break;
        }
        case PEX_MCPE_PACKET_TAKE_ITEM_ENTITY: {
            PexMcpeTakeItemInfo take;
            if (pex_mcpe_decode_take_item_entity_packet(body, body_size, &take) && s->callbacks.on_take_item) {
                s->callbacks.on_take_item(s->callback_userdata, &take);
            }
            break;
        }
        case PEX_MCPE_PACKET_SET_ENTITY_DATA: {
            PexMcpeEntityDataInfo data_info;
            if (pex_mcpe_decode_set_entity_data_packet(body, body_size, &data_info) && s->callbacks.on_entity_data) {
                s->callbacks.on_entity_data(s->callback_userdata, &data_info);
            }
            break;
        }
        case PEX_MCPE_PACKET_SET_ENTITY_MOTION: {
            PexMcpeEntityMotionInfo motions[32];
            size_t count = 0;
            if (pex_mcpe_decode_set_entity_motion_packet(body, body_size, motions, 32, &count) && s->callbacks.on_entity_motion) {
                for (size_t i = 0; i < count; ++i) s->callbacks.on_entity_motion(s->callback_userdata, &motions[i]);
            }
            break;
        }
        case PEX_MCPE_PACKET_LEVEL_EVENT: {
            PexMcpeLevelEventInfo ev;
            if (pex_mcpe_decode_level_event_packet(body, body_size, &ev) && s->callbacks.on_level_event) {
                s->callbacks.on_level_event(s->callback_userdata, &ev);
            }
            break;
        }
        case PEX_MCPE_PACKET_UPDATE_ATTRIBUTES: {
            PexMcpeAttributesInfo attrs;
            if (pex_mcpe_decode_update_attributes_packet(body, body_size, &attrs) && s->callbacks.on_attributes) {
                s->callbacks.on_attributes(s->callback_userdata, &attrs);
            }
            break;
        }
        case PEX_MCPE_PACKET_MOB_ARMOR_EQUIPMENT: {
            PexMcpeArmorInfo armor;
            if (pex_mcpe_decode_mob_armor_equipment_packet(body, body_size, &armor) && s->callbacks.on_armor) {
                s->callbacks.on_armor(s->callback_userdata, &armor);
            }
            break;
        }
        case PEX_MCPE_PACKET_SET_PLAYER_GAMETYPE: {
            PexMcpeReadBuffer gb;
            int32_t gm = 0;
            pex_mcpe_read_buffer_init(&gb, body, body_size);
            if (pex_mcpe_read_i32_be(&gb, &gm) && s->callbacks.on_gamemode) {
                s->callbacks.on_gamemode(s->callback_userdata, (int)gm);
            }
            break;
        }
        case PEX_MCPE_PACKET_RESPAWN: {
            PexMcpeReadBuffer rb;
            float x = 0, y = 0, z = 0;
            pex_mcpe_read_buffer_init(&rb, body, body_size);
            if (pex_mcpe_read_f32_be(&rb, &x) && pex_mcpe_read_f32_be(&rb, &y) && pex_mcpe_read_f32_be(&rb, &z)) {
                PexMcpeEntityMoveInfo move;
                memset(&move, 0, sizeof(move));
                move.eid = s->entity_id;
                move.is_self = 1;
                move.x = x; move.y = y; move.z = z;
                if (s->callbacks.on_move_entity) s->callbacks.on_move_entity(s->callback_userdata, &move);
            }
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
            pex_mcpe_join_set_status(session, "Locating server");
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
            pex_mcpe_join_set_status(session, "Locating server");
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
    return pex_mcpe_send_packet_batched(session, buf, n);
}

int pex_mcpe_join_session_send_chat(PexMcpeJoinSession *session, const char *message) {
    if (!session || !session->raknet || !message || !message[0] || !session->spawn_status_received) return 0;
    uint8_t buf[1024];
    size_t n;
    if (!pex_mcpe_encode_chat_packet(buf, sizeof(buf), &n, session->username, message)) return 0;
    return pex_mcpe_send_packet_batched(session, buf, n);
}


int pex_mcpe_join_session_send_equipment(PexMcpeJoinSession *session,
                                         int slot, int item_id, int count, int damage) {
    if (!session || !session->raknet || !session->entity_id_valid) return 0;
    uint8_t buf[128];
    size_t n = 0;
    if (!pex_mcpe_encode_mob_equipment_packet(buf, sizeof(buf), &n, session->entity_id,
                                             slot, item_id, count, damage)) return 0;
    return pex_mcpe_send_packet_batched(session, buf, n);
}

int pex_mcpe_join_session_send_break(PexMcpeJoinSession *session, int x, int y, int z, int face,
                                     int slot, int held_item_id, int held_count, int held_damage) {
    if (!session || !session->raknet || !session->entity_id_valid) return 0;
    uint8_t buf[128];
    size_t n = 0;
    pex_mcpe_join_session_send_equipment(session, slot, held_item_id, held_count, held_damage);
    if (pex_mcpe_encode_player_action_packet(buf, sizeof(buf), &n, session->entity_id, 0, x, y, z, face)) {
        pex_mcpe_send_packet_batched(session, buf, n);
    }
    if (pex_mcpe_encode_player_action_packet(buf, sizeof(buf), &n, session->entity_id, 2, x, y, z, face)) {
        pex_mcpe_send_packet_batched(session, buf, n);
    }
    if (pex_mcpe_encode_remove_block_packet(buf, sizeof(buf), &n, session->entity_id, x, y, z)) {
        return pex_mcpe_send_packet_batched(session, buf, n);
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
    return pex_mcpe_send_packet_batched(session, buf, n);
}

int pex_mcpe_join_session_send_animate(PexMcpeJoinSession *session, int action) {
    if (!session || !session->raknet || !session->entity_id_valid) return 0;
    uint8_t buf[64];
    size_t n = 0;
    if (!pex_mcpe_encode_animate_packet(buf, sizeof(buf), &n, session->entity_id, action)) return 0;
    return pex_mcpe_send_packet_batched(session, buf, n);
}

int pex_mcpe_join_session_send_interact(PexMcpeJoinSession *session, int action, uint64_t target_eid) {
    if (!session || !session->raknet || target_eid == 0) return 0;
    uint8_t buf[64];
    size_t n = 0;
    if (!pex_mcpe_encode_interact_packet(buf, sizeof(buf), &n, action, target_eid)) return 0;
    return pex_mcpe_send_packet_batched(session, buf, n);
}

int pex_mcpe_join_session_send_inventory_slot(PexMcpeJoinSession *session,
                                              int window_id, int slot, int hotbar_slot,
                                              int item_id, int count, int damage) {
    if (!session || !session->raknet) return 0;
    uint8_t buf[128];
    size_t n = 0;
    if (!pex_mcpe_encode_container_set_slot_packet(buf, sizeof(buf), &n, window_id, slot, hotbar_slot, item_id, count, damage)) return 0;
    return pex_mcpe_send_packet_batched(session, buf, n);
}

int pex_mcpe_join_session_send_player_action(PexMcpeJoinSession *session, int action, int x, int y, int z, int face) {
    if (!session || !session->raknet || !session->entity_id_valid) return 0;
    uint8_t buf[128];
    size_t n = 0;
    if (!pex_mcpe_encode_player_action_packet(buf, sizeof(buf), &n, session->entity_id, action, x, y, z, face)) return 0;
    return pex_mcpe_send_packet_batched(session, buf, n);
}

int pex_mcpe_join_session_send_drop_item(PexMcpeJoinSession *session, int item_id, int count, int damage) {
    if (!session || !session->raknet || item_id <= 0 || count <= 0) return 0;
    uint8_t buf[128];
    size_t n = 0;
    if (!pex_mcpe_encode_drop_item_packet(buf, sizeof(buf), &n, item_id, count, damage)) return 0;
    return pex_mcpe_send_packet_batched(session, buf, n);
}

int pex_mcpe_join_session_send_respawn(PexMcpeJoinSession *session, float x, float y, float z) {
    if (!session || !session->raknet) return 0;
    uint8_t buf[128];
    size_t n = 0;
    /* Genisys handles respawn from PlayerActionPacket ACTION_SPAWN_*; send the old RespawnPacket too for clients/servers that expect it. */
    if (pex_mcpe_encode_respawn_packet(buf, sizeof(buf), &n, x, y, z)) pex_mcpe_send_packet_batched(session, buf, n);
    return pex_mcpe_join_session_send_player_action(session, 13, 0, 0, 0, -1);
}

void pex_mcpe_join_session_disconnect(PexMcpeJoinSession *session) {
    if (!session) return;
    if (session->raknet) {
        uint8_t buf[64];
        size_t n = 0;
        PexMcpeBuffer b;
        pex_mcpe_buffer_init(&b, buf, sizeof(buf));
        if (pex_mcpe_write_u8(&b, PEX_MCPE_PACKET_DISCONNECT) &&
            pex_mcpe_write_string(&b, "disconnect")) {
            n = b.offset;
            pex_mcpe_send_packet_batched(session, buf, n);
        }
        pex_raknet_client_destroy(session->raknet);
        session->raknet = NULL;
    }
    session->state = PEX_MCPE_JOIN_IDLE;
}

const char *pex_mcpe_join_session_status(const PexMcpeJoinSession *session) {
    return session && session->status_text[0] ? session->status_text : "Locating server";
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
