#ifndef PEXCRAFT_MCPE_PROTOCOL_81_REQUEST_CHUNK_RADIUS_PACKET_H
#define PEXCRAFT_MCPE_PROTOCOL_81_REQUEST_CHUNK_RADIUS_PACKET_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int pex_mcpe_encode_request_chunk_radius_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size, int radius);
int pex_mcpe_decode_chunk_radius_updated_packet(const uint8_t *data, size_t size, int *out_radius);
#ifdef __cplusplus
}
#endif
#endif
