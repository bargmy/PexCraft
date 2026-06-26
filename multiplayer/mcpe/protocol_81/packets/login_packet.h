#ifndef PEXCRAFT_MCPE_PROTOCOL_81_LOGIN_PACKET_H
#define PEXCRAFT_MCPE_PROTOCOL_81_LOGIN_PACKET_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Encodes a complete RakLib game packet: 0xfe, LoginPacket ID, then body. */
int pex_mcpe_encode_login_packet(uint8_t *out_data,
                                 size_t out_capacity,
                                 size_t *out_size,
                                 const char *username,
                                 int protocol_version,
                                 const char *server_address);
int pex_mcpe_encode_login_packet_with_skin(uint8_t *out_data,
                                           size_t out_capacity,
                                           size_t *out_size,
                                           const char *username,
                                           int protocol_version,
                                           const char *server_address,
                                           const uint8_t *skin_rgba,
                                           int skin_width,
                                           int skin_height);

#ifdef __cplusplus
}
#endif

#endif /* PEXCRAFT_MCPE_PROTOCOL_81_LOGIN_PACKET_H */
