#ifndef PEXCRAFT_MCPE_PROTOCOL_81_CHUNK_CONVERT_H
#define PEXCRAFT_MCPE_PROTOCOL_81_CHUNK_CONVERT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int pex_mcpe_convert_chunk_to_pexcraft(const uint8_t *mcpe_chunk,
                                       size_t mcpe_chunk_size,
                                       int chunk_x,
                                       int chunk_z,
                                       int order);

#ifdef __cplusplus
}
#endif

#endif /* PEXCRAFT_MCPE_PROTOCOL_81_CHUNK_CONVERT_H */
