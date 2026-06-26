#ifndef PEXCRAFT_MCPE_PROTOCOL_81_TEXT_PACKET_H
#define PEXCRAFT_MCPE_PROTOCOL_81_TEXT_PACKET_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int pex_mcpe_decode_text_packet(const uint8_t *data, size_t size, char *out_text, size_t out_text_size);
int pex_mcpe_encode_chat_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size, const char *source, const char *message);
#ifdef __cplusplus
}
#endif
#endif
