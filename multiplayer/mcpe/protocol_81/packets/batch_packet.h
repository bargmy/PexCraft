#ifndef PEXCRAFT_MCPE_PROTOCOL_81_BATCH_PACKET_H
#define PEXCRAFT_MCPE_PROTOCOL_81_BATCH_PACKET_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int pex_mcpe_decode_batch_packet(const uint8_t *data, size_t size, uint8_t **out_payload, size_t *out_size);
#ifdef __cplusplus
}
#endif
#endif
