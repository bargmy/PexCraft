#include "start_game_packet.h"
#include "packet_codec.h"
#include <string.h>

int pex_mcpe_decode_start_game_packet(const uint8_t *data, size_t size, PexMcpeStartGameInfo *out_info) {
    if (!out_info) return 0;
    memset(out_info, 0, sizeof(*out_info));
    PexMcpeReadBuffer b;
    int32_t v;
    int64_t eid;
    uint8_t dim;
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_i32_be(&b, &out_info->seed)) return 0;
    if (!pex_mcpe_read_u8(&b, &dim)) return 0;
    out_info->dimension = (int)dim;
    if (!pex_mcpe_read_i32_be(&b, &out_info->generator)) return 0;
    if (!pex_mcpe_read_i32_be(&b, &out_info->game_mode)) return 0;
    if (!pex_mcpe_read_i64_be(&b, &eid)) return 0;
    out_info->entity_id = eid;
    out_info->runtime_id = (uint64_t)eid;
    if (!pex_mcpe_read_i32_be(&b, &v)) return 0;
    out_info->spawn_x = v;
    if (!pex_mcpe_read_i32_be(&b, &v)) return 0;
    out_info->spawn_y = v;
    if (!pex_mcpe_read_i32_be(&b, &v)) return 0;
    out_info->spawn_z = v;
    if (!pex_mcpe_read_f32_be(&b, &out_info->x)) return 0;
    if (!pex_mcpe_read_f32_be(&b, &out_info->y)) return 0;
    if (!pex_mcpe_read_f32_be(&b, &out_info->z)) return 0;
    return 1;
}
