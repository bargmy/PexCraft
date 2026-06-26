#ifndef PEXCRAFT_MCPE_PROTOCOL_81_RAKNET_WRAPPER_H
#define PEXCRAFT_MCPE_PROTOCOL_81_RAKNET_WRAPPER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PexRakNetClient PexRakNetClient;

typedef enum PexRakNetPollType {
    PEX_RAKNET_POLL_NONE = 0,
    PEX_RAKNET_POLL_CONNECTED,
    PEX_RAKNET_POLL_DATA,
    PEX_RAKNET_POLL_DISCONNECTED,
    PEX_RAKNET_POLL_FAILED
} PexRakNetPollType;

PexRakNetClient *pex_raknet_client_create(void);
int pex_raknet_client_connect(PexRakNetClient *client, const char *host, uint16_t port);
int pex_raknet_client_send(PexRakNetClient *client, const uint8_t *data, size_t size, int reliability);
int pex_raknet_client_poll(PexRakNetClient *client, uint8_t *buffer, size_t buffer_size, size_t *out_size, PexRakNetPollType *out_type);
int pex_raknet_client_receive(PexRakNetClient *client, uint8_t *buffer, size_t buffer_size, size_t *out_size);
int pex_raknet_client_is_connected(PexRakNetClient *client);
const char *pex_raknet_client_last_error(PexRakNetClient *client);
void pex_raknet_client_destroy(PexRakNetClient *client);

#ifdef __cplusplus
}
#endif

#endif /* PEXCRAFT_MCPE_PROTOCOL_81_RAKNET_WRAPPER_H */
