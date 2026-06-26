#include "update_block_packet.h"
#include "packet_codec.h"
int pex_mcpe_decode_update_block_records(const uint8_t *data, size_t size,
                                         PexMcpeUpdateBlockRecord *records, size_t max_records, size_t *out_count) {
    if (out_count) *out_count = 0;
    if (!data || !records || max_records == 0) return 0;
    PexMcpeReadBuffer b; pex_mcpe_read_buffer_init(&b, data, size);
    size_t n = 0;
    while (pex_mcpe_buffer_remaining(&b) >= 11 && n < max_records) {
        int32_t x,z; uint8_t y,id,packed;
        if (!pex_mcpe_read_i32_be(&b, &x) || !pex_mcpe_read_i32_be(&b, &z) ||
            !pex_mcpe_read_u8(&b, &y) || !pex_mcpe_read_u8(&b, &id) || !pex_mcpe_read_u8(&b, &packed)) break;
        records[n].x = x; records[n].z = z; records[n].y = y; records[n].id = id;
        records[n].meta = packed & 15; records[n].flags = packed >> 4; n++;
    }
    if (out_count) *out_count = n;
    return n > 0;
}
