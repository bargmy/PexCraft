#include "entity_packets.h"
#include "packet_codec.h"
#include <string.h>

static int pex_mcpe_read_i16_le(PexMcpeReadBuffer *b, int16_t *out_value) {
    const uint8_t *p;
    if (!pex_mcpe_read_bytes(b, &p, 2)) return 0;
    if (out_value) *out_value = (int16_t)(((uint16_t)p[1] << 8) | (uint16_t)p[0]);
    return 1;
}

static int pex_mcpe_read_u16_le(PexMcpeReadBuffer *b, uint16_t *out_value) {
    int16_t s;
    if (!pex_mcpe_read_i16_le(b, &s)) return 0;
    if (out_value) *out_value = (uint16_t)s;
    return 1;
}

static int pex_mcpe_skip_slot(PexMcpeReadBuffer *b, int *out_id, int *out_count, int *out_damage) {
    int16_t id = 0;
    uint8_t count = 0;
    int16_t damage = 0;
    uint16_t nbt_len = 0;
    if (out_id) *out_id = 0;
    if (out_count) *out_count = 0;
    if (out_damage) *out_damage = 0;
    if (!pex_mcpe_read_i16_be(b, &id)) return 0;
    if (id <= 0) return 1;
    if (!pex_mcpe_read_u8(b, &count)) return 0;
    if (!pex_mcpe_read_i16_be(b, &damage)) return 0;
    if (!pex_mcpe_read_u16_le(b, &nbt_len)) return 0;
    if (nbt_len > 0) {
        const uint8_t *ignored;
        if (!pex_mcpe_read_bytes(b, &ignored, nbt_len)) return 0;
    }
    if (out_id) *out_id = id;
    if (out_count) *out_count = count;
    if (out_damage) *out_damage = damage;
    return 1;
}

static int pex_mcpe_skip_metadata(PexMcpeReadBuffer *b) {
    for (;;) {
        uint8_t header = 0;
        if (!pex_mcpe_read_u8(b, &header)) return 0;
        if (header == 0x7f) return 1;
        int type = header >> 5;
        const uint8_t *ignored;
        switch (type) {
            case 0: /* byte */
                if (!pex_mcpe_read_bytes(b, &ignored, 1)) return 0;
                break;
            case 1: /* short, little-endian */
                if (!pex_mcpe_read_bytes(b, &ignored, 2)) return 0;
                break;
            case 2: /* int, little-endian */
            case 3: /* float, little-endian */
                if (!pex_mcpe_read_bytes(b, &ignored, 4)) return 0;
                break;
            case 4: { /* string: little-endian short length + bytes */
                uint16_t len = 0;
                if (!pex_mcpe_read_u16_le(b, &len)) return 0;
                if (!pex_mcpe_read_bytes(b, &ignored, len)) return 0;
                break;
            }
            case 5: /* slot metadata: id short LE, count byte, damage short LE */
                if (!pex_mcpe_read_bytes(b, &ignored, 5)) return 0;
                break;
            case 6: /* block position: three little-endian ints */
                if (!pex_mcpe_read_bytes(b, &ignored, 12)) return 0;
                break;
            case 7: /* long, little-endian */
                if (!pex_mcpe_read_bytes(b, &ignored, 8)) return 0;
                break;
            default:
                return 0;
        }
    }
}

int pex_mcpe_decode_add_player_packet(const uint8_t *data, size_t size, PexMcpeRemotePlayerInfo *out_info) {
    if (!data || !out_info) return 0;
    memset(out_info, 0, sizeof(*out_info));
    PexMcpeReadBuffer b;
    pex_mcpe_read_buffer_init(&b, data, size);
    const uint8_t *uuid;
    int64_t eid = 0;
    if (!pex_mcpe_read_bytes(&b, &uuid, 16)) return 0;
    if (!pex_mcpe_read_string(&b, out_info->username, sizeof(out_info->username))) return 0;
    if (!pex_mcpe_read_i64_be(&b, &eid)) return 0;
    out_info->eid = (uint64_t)eid;
    if (!pex_mcpe_read_f32_be(&b, &out_info->x)) return 0;
    if (!pex_mcpe_read_f32_be(&b, &out_info->y)) return 0;
    if (!pex_mcpe_read_f32_be(&b, &out_info->z)) return 0;
    /* Speed vector: not used by PexCraft yet. */
    { float ignored_f; if (!pex_mcpe_read_f32_be(&b, &ignored_f)) return 0; if (!pex_mcpe_read_f32_be(&b, &ignored_f)) return 0; if (!pex_mcpe_read_f32_be(&b, &ignored_f)) return 0; }
    if (!pex_mcpe_read_f32_be(&b, &out_info->yaw)) return 0;
    { float head_yaw; if (!pex_mcpe_read_f32_be(&b, &head_yaw)) return 0; }
    if (!pex_mcpe_read_f32_be(&b, &out_info->pitch)) return 0;
    if (!pex_mcpe_skip_slot(&b, &out_info->held_item_id, &out_info->held_item_count, &out_info->held_item_damage)) return 0;
    /* Metadata is optional for our renderer. Decode enough to land on the 0x7f terminator. */
    pex_mcpe_skip_metadata(&b);
    return out_info->eid != 0;
}

int pex_mcpe_decode_move_player_packet(const uint8_t *data, size_t size, PexMcpeEntityMoveInfo *out_info) {
    if (!data || !out_info) return 0;
    memset(out_info, 0, sizeof(*out_info));
    PexMcpeReadBuffer b;
    int64_t eid = 0;
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_i64_be(&b, &eid)) return 0;
    out_info->eid = (uint64_t)eid;
    out_info->is_self = (eid == 0);
    if (!pex_mcpe_read_f32_be(&b, &out_info->x)) return 0;
    if (!pex_mcpe_read_f32_be(&b, &out_info->y)) return 0;
    if (!pex_mcpe_read_f32_be(&b, &out_info->z)) return 0;
    if (!pex_mcpe_read_f32_be(&b, &out_info->yaw)) return 0;
    { float body_yaw; if (!pex_mcpe_read_f32_be(&b, &body_yaw)) return 0; }
    if (!pex_mcpe_read_f32_be(&b, &out_info->pitch)) return 0;
    return 1;
}

static float pex_mcpe_byte_angle(uint8_t v) {
    return (float)v * (360.0f / 256.0f);
}

int pex_mcpe_decode_move_entity_packet(const uint8_t *data, size_t size, PexMcpeEntityMoveInfo *out_info) {
    if (!data || !out_info) return 0;
    memset(out_info, 0, sizeof(*out_info));
    PexMcpeReadBuffer b;
    int64_t eid = 0;
    uint8_t pitch_b = 0, yaw_b = 0, head_b = 0;
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_i64_be(&b, &eid)) return 0;
    out_info->eid = (uint64_t)eid;
    if (!pex_mcpe_read_f32_be(&b, &out_info->x)) return 0;
    if (!pex_mcpe_read_f32_be(&b, &out_info->y)) return 0;
    if (!pex_mcpe_read_f32_be(&b, &out_info->z)) return 0;
    if (!pex_mcpe_read_u8(&b, &pitch_b)) return 0;
    if (!pex_mcpe_read_u8(&b, &yaw_b)) return 0;
    if (!pex_mcpe_read_u8(&b, &head_b)) return 0;
    (void)head_b;
    out_info->pitch = pex_mcpe_byte_angle(pitch_b);
    out_info->yaw = pex_mcpe_byte_angle(yaw_b);
    return out_info->eid != 0;
}

int pex_mcpe_decode_remove_entity_packet(const uint8_t *data, size_t size, uint64_t *out_eid) {
    if (out_eid) *out_eid = 0;
    PexMcpeReadBuffer b;
    int64_t eid = 0;
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_i64_be(&b, &eid)) return 0;
    if (out_eid) *out_eid = (uint64_t)eid;
    return 1;
}
