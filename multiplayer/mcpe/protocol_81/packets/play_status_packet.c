#include "play_status_packet.h"
#include "packet_codec.h"

int pex_mcpe_decode_play_status_packet(const uint8_t *data, size_t size, PexMcpePlayStatus *out_status) {
    if (out_status) *out_status = PEX_MCPE_PLAY_STATUS_UNKNOWN;
    PexMcpeReadBuffer b;
    int32_t status;
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_i32_be(&b, &status)) return 0;
    if (out_status) {
        if (status >= 0 && status <= 3) *out_status = (PexMcpePlayStatus)status;
        else *out_status = PEX_MCPE_PLAY_STATUS_UNKNOWN;
    }
    return 1;
}
