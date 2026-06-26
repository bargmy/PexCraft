#include "entity_packets.h"
#include "packet_codec.h"
#include "packet_ids.h"
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

int pex_mcpe_decode_player_list_packet(const uint8_t *data, size_t size, PexMcpePlayerListSkin *out_entries, size_t max_entries, size_t *out_count) {
    if (out_count) *out_count = 0;
    if (!data || !out_entries || max_entries == 0) return 0;
    PexMcpeReadBuffer b;
    pex_mcpe_read_buffer_init(&b, data, size);
    uint8_t type = 0;
    int32_t count = 0;
    if (!pex_mcpe_read_u8(&b, &type)) return 0;
    if (!pex_mcpe_read_i32_be(&b, &count) || count < 0) return 0;
    size_t written = 0;
    for (int32_t i = 0; i < count; ++i) {
        const uint8_t *uuid = NULL;
        if (!pex_mcpe_read_bytes(&b, &uuid, 16)) break;
        if (written >= max_entries) {
            if (type == 0) {
                int64_t eid_ignored = 0;
                uint8_t *skin_ignored = NULL;
                size_t skin_ignored_size = 0;
                char tmp[128];
                if (!pex_mcpe_read_i64_be(&b, &eid_ignored)) break;
                if (!pex_mcpe_read_string(&b, tmp, sizeof(tmp))) break;
                if (!pex_mcpe_read_string(&b, tmp, sizeof(tmp))) break;
                if (!pex_mcpe_read_string_bytes(&b, &skin_ignored, &skin_ignored_size)) break;
                pex_mcpe_free_string_bytes(skin_ignored);
            }
            continue;
        }
        PexMcpePlayerListSkin *e = &out_entries[written];
        memset(e, 0, sizeof(*e));
        e->type = type;
        memcpy(e->uuid, uuid, 16);
        if (type == 0) {
            int64_t eid = 0;
            if (!pex_mcpe_read_i64_be(&b, &eid)) break;
            e->eid = (uint64_t)eid;
            if (!pex_mcpe_read_string(&b, e->username, sizeof(e->username))) break;
            if (!pex_mcpe_read_string(&b, e->skin_id, sizeof(e->skin_id))) break;
            if (!pex_mcpe_read_string_bytes(&b, &e->skin_data, &e->skin_size)) break;
        }
        written++;
    }
    if (out_count) *out_count = written;
    return written > 0 || count == 0;
}

void pex_mcpe_free_player_list_entries(PexMcpePlayerListSkin *entries, size_t count) {
    if (!entries) return;
    for (size_t i = 0; i < count; ++i) {
        if (entries[i].skin_data) {
            pex_mcpe_free_string_bytes(entries[i].skin_data);
            entries[i].skin_data = NULL;
            entries[i].skin_size = 0;
        }
    }
}


static int pex_mcpe_write_slot_info(PexMcpeBuffer *b, int item_id, int count, int damage) {
    if (!b) return 0;
    if (item_id <= 0 || count <= 0) return pex_mcpe_write_i16_be(b, 0);
    if (count > 255) count = 255;
    return pex_mcpe_write_i16_be(b, (int16_t)item_id) &&
           pex_mcpe_write_u8(b, (uint8_t)count) &&
           pex_mcpe_write_i16_be(b, (int16_t)damage) &&
           pex_mcpe_write_u8(b, 0) && pex_mcpe_write_u8(b, 0);
}

int pex_mcpe_decode_mob_equipment_packet(const uint8_t *data, size_t size, PexMcpeMobEquipmentInfo *out_info) {
    if (!data || !out_info) return 0;
    memset(out_info, 0, sizeof(*out_info));
    PexMcpeReadBuffer b;
    int64_t eid = 0;
    uint8_t slot = 0, selected = 0;
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_i64_be(&b, &eid)) return 0;
    out_info->eid = (uint64_t)eid;
    if (!pex_mcpe_skip_slot(&b, &out_info->item.id, &out_info->item.count, &out_info->item.damage)) return 0;
    if (!pex_mcpe_read_u8(&b, &slot)) return 0;
    if (!pex_mcpe_read_u8(&b, &selected)) return 0;
    out_info->slot = (int)slot;
    out_info->selected_slot = (int)selected;
    return 1;
}

int pex_mcpe_decode_animate_packet(const uint8_t *data, size_t size, PexMcpeAnimateInfo *out_info) {
    if (!data || !out_info) return 0;
    memset(out_info, 0, sizeof(*out_info));
    PexMcpeReadBuffer b;
    uint8_t action = 0;
    int64_t eid = 0;
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_u8(&b, &action)) return 0;
    if (!pex_mcpe_read_i64_be(&b, &eid)) return 0;
    out_info->action = (int)action;
    out_info->eid = (uint64_t)eid;
    return 1;
}

int pex_mcpe_decode_add_item_entity_packet(const uint8_t *data, size_t size, PexMcpeDroppedItemInfo *out_info) {
    if (!data || !out_info) return 0;
    memset(out_info, 0, sizeof(*out_info));
    PexMcpeReadBuffer b;
    int64_t eid = 0;
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_i64_be(&b, &eid)) return 0;
    out_info->eid = (uint64_t)eid;
    if (!pex_mcpe_skip_slot(&b, &out_info->item.id, &out_info->item.count, &out_info->item.damage)) return 0;
    if (!pex_mcpe_read_f32_be(&b, &out_info->x)) return 0;
    if (!pex_mcpe_read_f32_be(&b, &out_info->y)) return 0;
    if (!pex_mcpe_read_f32_be(&b, &out_info->z)) return 0;
    if (!pex_mcpe_read_f32_be(&b, &out_info->mx)) return 0;
    if (!pex_mcpe_read_f32_be(&b, &out_info->my)) return 0;
    if (!pex_mcpe_read_f32_be(&b, &out_info->mz)) return 0;
    return out_info->eid != 0 && out_info->item.id > 0 && out_info->item.count > 0;
}

int pex_mcpe_decode_container_set_content_packet(const uint8_t *data, size_t size, PexMcpeContainerContentInfo *out_info) {
    if (!data || !out_info) return 0;
    memset(out_info, 0, sizeof(*out_info));
    PexMcpeReadBuffer b;
    uint8_t win = 0;
    int16_t count = 0;
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_u8(&b, &win)) return 0;
    out_info->window_id = (int)win;
    if (!pex_mcpe_read_i16_be(&b, &count) || count < 0) return 0;
    out_info->slot_count = count > PEX_MCPE_MAX_INVENTORY_SLOTS ? PEX_MCPE_MAX_INVENTORY_SLOTS : count;
    for (int i = 0; i < count; ++i) {
        int id = 0, cnt = 0, dmg = 0;
        if (!pex_mcpe_skip_slot(&b, &id, &cnt, &dmg)) return 0;
        if (i < PEX_MCPE_MAX_INVENTORY_SLOTS) {
            out_info->slots[i].id = id;
            out_info->slots[i].count = cnt;
            out_info->slots[i].damage = dmg;
        }
    }
    if (out_info->window_id == 0 && pex_mcpe_buffer_remaining(&b) >= 2) {
        int16_t hotbar_count = 0;
        if (!pex_mcpe_read_i16_be(&b, &hotbar_count) || hotbar_count < 0) return 0;
        if (hotbar_count > 16) hotbar_count = 16;
        out_info->hotbar_count = hotbar_count;
        for (int i = 0; i < hotbar_count; ++i) {
            int32_t hb = -1;
            if (!pex_mcpe_read_i32_be(&b, &hb)) return 0;
            out_info->hotbar[i] = hb;
        }
    }
    return 1;
}

int pex_mcpe_decode_container_set_slot_packet(const uint8_t *data, size_t size, PexMcpeContainerSlotInfo *out_info) {
    if (!data || !out_info) return 0;
    memset(out_info, 0, sizeof(*out_info));
    PexMcpeReadBuffer b;
    uint8_t win = 0;
    int16_t slot = 0, hotbar = 0;
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_u8(&b, &win)) return 0;
    if (!pex_mcpe_read_i16_be(&b, &slot)) return 0;
    if (!pex_mcpe_read_i16_be(&b, &hotbar)) return 0;
    out_info->window_id = (int)win;
    out_info->slot = (int)slot;
    out_info->hotbar_slot = (int)hotbar;
    if (!pex_mcpe_skip_slot(&b, &out_info->item.id, &out_info->item.count, &out_info->item.damage)) return 0;
    return 1;
}

int pex_mcpe_encode_animate_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size, uint64_t eid, int action) {
    if (out_size) *out_size = 0;
    PexMcpeBuffer b;
    pex_mcpe_buffer_init(&b, out_data, out_capacity);
    if (!pex_mcpe_write_u8(&b, PEX_MCPE_RAKLIB_GAME_PACKET) ||
        !pex_mcpe_write_u8(&b, PEX_MCPE_PACKET_ANIMATE) ||
        !pex_mcpe_write_u8(&b, (uint8_t)action) ||
        !pex_mcpe_write_i64_be(&b, (int64_t)eid)) return 0;
    if (out_size) *out_size = b.offset;
    return 1;
}

int pex_mcpe_encode_container_set_slot_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size,
                                             int window_id, int slot, int hotbar_slot,
                                             int item_id, int count, int damage) {
    if (out_size) *out_size = 0;
    PexMcpeBuffer b;
    pex_mcpe_buffer_init(&b, out_data, out_capacity);
    if (!pex_mcpe_write_u8(&b, PEX_MCPE_RAKLIB_GAME_PACKET) ||
        !pex_mcpe_write_u8(&b, PEX_MCPE_PACKET_CONTAINER_SET_SLOT) ||
        !pex_mcpe_write_u8(&b, (uint8_t)window_id) ||
        !pex_mcpe_write_i16_be(&b, (int16_t)slot) ||
        !pex_mcpe_write_i16_be(&b, (int16_t)hotbar_slot) ||
        !pex_mcpe_write_slot_info(&b, item_id, count, damage)) return 0;
    if (out_size) *out_size = b.offset;
    return 1;
}


int pex_mcpe_decode_take_item_entity_packet(const uint8_t *data, size_t size, PexMcpeTakeItemInfo *out_info) {
    if (!data || !out_info) return 0;
    memset(out_info, 0, sizeof(*out_info));
    PexMcpeReadBuffer b;
    int64_t target = 0, eid = 0;
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_i64_be(&b, &target)) return 0;
    if (!pex_mcpe_read_i64_be(&b, &eid)) return 0;
    out_info->target = (uint64_t)target;
    out_info->eid = (uint64_t)eid;
    return 1;
}

int pex_mcpe_decode_set_entity_motion_packet(const uint8_t *data, size_t size, PexMcpeEntityMotionInfo *out_entries, size_t max_entries, size_t *out_count) {
    if (out_count) *out_count = 0;
    if (!data || !out_entries || max_entries == 0) return 0;
    PexMcpeReadBuffer b;
    pex_mcpe_read_buffer_init(&b, data, size);
    size_t count = 0;
    while (pex_mcpe_buffer_remaining(&b) >= 20 && count < max_entries) {
        int64_t eid = 0;
        PexMcpeEntityMotionInfo *e = &out_entries[count];
        memset(e, 0, sizeof(*e));
        if (!pex_mcpe_read_i64_be(&b, &eid)) break;
        e->eid = (uint64_t)eid;
        if (!pex_mcpe_read_f32_be(&b, &e->mx)) break;
        if (!pex_mcpe_read_f32_be(&b, &e->my)) break;
        if (!pex_mcpe_read_f32_be(&b, &e->mz)) break;
        count++;
    }
    if (out_count) *out_count = count;
    return count > 0;
}

int pex_mcpe_decode_entity_event_packet(const uint8_t *data, size_t size, PexMcpeEntityEventInfo *out_info) {
    if (!data || !out_info) return 0;
    memset(out_info, 0, sizeof(*out_info));
    PexMcpeReadBuffer b;
    int64_t eid = 0;
    uint8_t ev = 0;
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_i64_be(&b, &eid)) return 0;
    if (!pex_mcpe_read_u8(&b, &ev)) return 0;
    out_info->eid = (uint64_t)eid;
    out_info->event = (int)ev;
    return 1;
}

int pex_mcpe_decode_level_event_packet(const uint8_t *data, size_t size, PexMcpeLevelEventInfo *out_info) {
    if (!data || !out_info) return 0;
    memset(out_info, 0, sizeof(*out_info));
    PexMcpeReadBuffer b;
    int16_t ev = 0;
    int32_t d = 0;
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_i16_be(&b, &ev)) return 0;
    if (!pex_mcpe_read_f32_be(&b, &out_info->x)) return 0;
    if (!pex_mcpe_read_f32_be(&b, &out_info->y)) return 0;
    if (!pex_mcpe_read_f32_be(&b, &out_info->z)) return 0;
    if (!pex_mcpe_read_i32_be(&b, &d)) return 0;
    out_info->event_id = (int)ev;
    out_info->data = (int)d;
    return 1;
}

int pex_mcpe_decode_update_attributes_packet(const uint8_t *data, size_t size, PexMcpeAttributesInfo *out_info) {
    if (!data || !out_info) return 0;
    memset(out_info, 0, sizeof(*out_info));
    PexMcpeReadBuffer b;
    int64_t eid = 0;
    int16_t count = 0;
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_i64_be(&b, &eid)) return 0;
    out_info->eid = (uint64_t)eid;
    if (!pex_mcpe_read_i16_be(&b, &count) || count < 0) return 0;
    for (int i = 0; i < count; ++i) {
        float minv = 0.0f, maxv = 0.0f, value = 0.0f;
        char name[128];
        if (!pex_mcpe_read_f32_be(&b, &minv)) return 0;
        if (!pex_mcpe_read_f32_be(&b, &maxv)) return 0;
        if (!pex_mcpe_read_f32_be(&b, &value)) return 0;
        if (!pex_mcpe_read_string(&b, name, sizeof(name))) return 0;
        (void)minv; (void)maxv;
        if (strstr(name, "health") || strstr(name, "minecraft:health")) {
            out_info->health = value;
            out_info->has_health = 1;
        } else if (strstr(name, "hunger") || strstr(name, "player.hunger") || strstr(name, "minecraft:player.hunger")) {
            out_info->hunger = value;
            out_info->has_hunger = 1;
        }
    }
    return 1;
}

int pex_mcpe_decode_mob_armor_equipment_packet(const uint8_t *data, size_t size, PexMcpeArmorInfo *out_info) {
    if (!data || !out_info) return 0;
    memset(out_info, 0, sizeof(*out_info));
    PexMcpeReadBuffer b;
    int64_t eid = 0;
    pex_mcpe_read_buffer_init(&b, data, size);
    if (!pex_mcpe_read_i64_be(&b, &eid)) return 0;
    out_info->eid = (uint64_t)eid;
    for (int i = 0; i < 4; ++i) {
        if (!pex_mcpe_skip_slot(&b, &out_info->slots[i].id, &out_info->slots[i].count, &out_info->slots[i].damage)) return 0;
    }
    return 1;
}

int pex_mcpe_encode_drop_item_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size,
                                     int item_id, int count, int damage) {
    if (out_size) *out_size = 0;
    PexMcpeBuffer b;
    pex_mcpe_buffer_init(&b, out_data, out_capacity);
    if (!pex_mcpe_write_u8(&b, PEX_MCPE_RAKLIB_GAME_PACKET) ||
        !pex_mcpe_write_u8(&b, PEX_MCPE_PACKET_DROP_ITEM) ||
        !pex_mcpe_write_u8(&b, 0) ||
        !pex_mcpe_write_slot_info(&b, item_id, count, damage)) return 0;
    if (out_size) *out_size = b.offset;
    return 1;
}

int pex_mcpe_encode_respawn_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size,
                                   float x, float y, float z) {
    if (out_size) *out_size = 0;
    PexMcpeBuffer b;
    pex_mcpe_buffer_init(&b, out_data, out_capacity);
    if (!pex_mcpe_write_u8(&b, PEX_MCPE_RAKLIB_GAME_PACKET) ||
        !pex_mcpe_write_u8(&b, PEX_MCPE_PACKET_RESPAWN) ||
        !pex_mcpe_write_f32_be(&b, x) ||
        !pex_mcpe_write_f32_be(&b, y) ||
        !pex_mcpe_write_f32_be(&b, z)) return 0;
    if (out_size) *out_size = b.offset;
    return 1;
}
