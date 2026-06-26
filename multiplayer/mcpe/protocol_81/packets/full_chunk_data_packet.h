#ifndef PEXCRAFT_MCPE_PROTOCOL_81_FULL_CHUNK_DATA_PACKET_H
#define PEXCRAFT_MCPE_PROTOCOL_81_FULL_CHUNK_DATA_PACKET_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PexMcpeFullChunkData {
    int32_t chunk_x;
    int32_t chunk_z;
    uint8_t order;
    const uint8_t *payload;
    size_t payload_size;
} PexMcpeFullChunkData;

int pex_mcpe_decode_full_chunk_data_packet(const uint8_t *data, size_t size, PexMcpeFullChunkData *out_chunk);

#ifdef __cplusplus
}
#endif

#endif /* PEXCRAFT_MCPE_PROTOCOL_81_FULL_CHUNK_DATA_PACKET_H */
