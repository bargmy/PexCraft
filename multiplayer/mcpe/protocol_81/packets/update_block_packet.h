#ifndef PEXCRAFT_MCPE_PROTOCOL_81_UPDATE_BLOCK_PACKET_H
#define PEXCRAFT_MCPE_PROTOCOL_81_UPDATE_BLOCK_PACKET_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct PexMcpeUpdateBlockRecord { int x; int y; int z; int id; int meta; int flags; } PexMcpeUpdateBlockRecord;
int pex_mcpe_decode_update_block_records(const uint8_t *data, size_t size,
                                         PexMcpeUpdateBlockRecord *records, size_t max_records, size_t *out_count);
#ifdef __cplusplus
}
#endif
#endif
