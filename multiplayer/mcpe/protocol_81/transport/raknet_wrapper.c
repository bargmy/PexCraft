#include "raknet_wrapper.h"
#include "raknet_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PexRakNetClient {
    PexRakNetLibrary *lib;
    RakPeerHandle *peer;
    char host[256];
    uint16_t port;
    int connected;
    char error[256];
};

PexRakNetClient *pex_raknet_client_create(void) {
    PexRakNetClient *c = (PexRakNetClient*)calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->lib = pex_raknet_library_open_for_current_platform();
    if (!c->lib || !c->lib->handle) {
        snprintf(c->error, sizeof(c->error), "%s", pex_raknet_library_last_error(c->lib));
        return c;
    }
    c->peer = c->lib->peer_create();
    if (!c->peer) snprintf(c->error, sizeof(c->error), "raknet_peer_create failed");
    return c;
}

int pex_raknet_client_connect(PexRakNetClient *client, const char *host, uint16_t port) {
    if (!client || !client->lib || !client->lib->handle || !client->peer) return 0;
    snprintf(client->host, sizeof(client->host), "%s", host && host[0] ? host : "127.0.0.1");
    client->port = port ? port : 19132;
    int sr = client->lib->peer_startup(client->peer, 1, 0);
    if (sr != RN_STARTED && sr != RN_ERR_ALREADY_STARTED) {
        snprintf(client->error, sizeof(client->error), "raknet_peer_startup failed: %d", sr);
        return 0;
    }
    client->lib->peer_set_max_incoming(client->peer, 0);
    int cr = client->lib->peer_connect(client->peer, client->host, client->port, NULL, 0);
    if (cr != RN_CONNECT_STARTED && cr != RN_ERR_ALREADY_CONNECTED && cr != RN_ERR_CONNECT_IN_PROGRESS) {
        snprintf(client->error, sizeof(client->error), "raknet_peer_connect failed: %d", cr);
        return 0;
    }
    return 1;
}

int pex_raknet_client_send(PexRakNetClient *client, const uint8_t *data, size_t size, int reliability) {
    if (!client || !client->lib || !client->peer || !data || size == 0) return 0;
    int rel = reliability ? reliability : RN_RELIABLE_ORDERED;
    int sent = client->lib->peer_send(client->peer, (const char*)data, (int)size,
                                      RN_PRIORITY_HIGH, rel, 0, client->host, client->port, 0);
    if (sent < 0) {
        snprintf(client->error, sizeof(client->error), "raknet_peer_send failed");
        return 0;
    }
    return 1;
}

int pex_raknet_client_poll(PexRakNetClient *client, uint8_t *buffer, size_t buffer_size, size_t *out_size, PexRakNetPollType *out_type) {
    if (out_size) *out_size = 0;
    if (out_type) *out_type = PEX_RAKNET_POLL_NONE;
    if (!client || !client->lib || !client->peer) return 0;

    RakPacket *pkt = client->lib->peer_receive(client->peer);
    if (!pkt) return 1;

    RakPacketInfo info;
    memset(&info, 0, sizeof(info));
    client->lib->packet_get_info(pkt, &info);
    if (info.length > 0 && info.data) {
        unsigned char id = info.data[0];
        switch (id) {
            case RN_MSG_CONNECTION_REQUEST_ACCEPTED:
            case RN_MSG_ALREADY_CONNECTED:
                client->connected = 1;
                if (out_type) *out_type = PEX_RAKNET_POLL_CONNECTED;
                break;
            case RN_MSG_CONNECTION_ATTEMPT_FAILED:
            case RN_MSG_NO_FREE_INCOMING_CONNECTIONS:
            case RN_MSG_INVALID_PASSWORD:
                snprintf(client->error, sizeof(client->error), "RakNet connection failed: %u", (unsigned)id);
                if (out_type) *out_type = PEX_RAKNET_POLL_FAILED;
                break;
            case RN_MSG_DISCONNECTION_NOTIFICATION:
            case RN_MSG_CONNECTION_LOST:
                client->connected = 0;
                snprintf(client->error, sizeof(client->error), "RakNet disconnected: %u", (unsigned)id);
                if (out_type) *out_type = PEX_RAKNET_POLL_DISCONNECTED;
                break;
            default:
                if (info.length <= buffer_size && buffer) {
                    memcpy(buffer, info.data, info.length);
                    if (out_size) *out_size = info.length;
                    if (out_type) *out_type = PEX_RAKNET_POLL_DATA;
                }
                break;
        }
    }
    client->lib->peer_deallocate_packet(client->peer, pkt);
    return 1;
}

int pex_raknet_client_receive(PexRakNetClient *client, uint8_t *buffer, size_t buffer_size, size_t *out_size) {
    PexRakNetPollType type;
    if (!pex_raknet_client_poll(client, buffer, buffer_size, out_size, &type)) return -1;
    return type == PEX_RAKNET_POLL_DATA ? 1 : 0;
}

int pex_raknet_client_is_connected(PexRakNetClient *client) {
    return client && client->connected;
}

const char *pex_raknet_client_last_error(PexRakNetClient *client) {
    return client && client->error[0] ? client->error : "RakNet client error";
}

void pex_raknet_client_destroy(PexRakNetClient *client) {
    if (!client) return;
    if (client->lib && client->peer) {
        client->lib->peer_shutdown(client->peer, 300);
        client->lib->peer_destroy(client->peer);
        client->peer = NULL;
    }
    if (client->lib) {
        pex_raknet_library_close(client->lib);
        client->lib = NULL;
    }
    free(client);
}
