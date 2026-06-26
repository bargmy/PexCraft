#ifndef PEXCRAFT_MCPE_PROTOCOL_81_ENTITY_PACKETS_H
#define PEXCRAFT_MCPE_PROTOCOL_81_ENTITY_PACKETS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PexMcpeItemStackInfo {
    int id;
    int count;
    int damage;
} PexMcpeItemStackInfo;

typedef struct PexMcpeRemotePlayerInfo {
    uint64_t eid;
    char username[64];
    float x, y, z;
    float yaw, pitch;
    int held_item_id;
    int held_item_count;
    int held_item_damage;
} PexMcpeRemotePlayerInfo;

typedef struct PexMcpeEntityMoveInfo {
    uint64_t eid;
    float x, y, z;
    float yaw, pitch;
    int is_self;
} PexMcpeEntityMoveInfo;

typedef struct PexMcpePlayerListSkin {
    int type;
    uint8_t uuid[16];
    uint64_t eid;
    char username[64];
    char skin_id[64];
    uint8_t *skin_data;
    size_t skin_size;
} PexMcpePlayerListSkin;

typedef struct PexMcpeMobEquipmentInfo {
    uint64_t eid;
    int slot;
    int selected_slot;
    PexMcpeItemStackInfo item;
} PexMcpeMobEquipmentInfo;

typedef struct PexMcpeAnimateInfo {
    uint64_t eid;
    int action;
} PexMcpeAnimateInfo;

typedef struct PexMcpeDroppedItemInfo {
    uint64_t eid;
    PexMcpeItemStackInfo item;
    float x, y, z;
    float mx, my, mz;
} PexMcpeDroppedItemInfo;

typedef struct PexMcpeTakeItemInfo {
    uint64_t target;
    uint64_t eid;
} PexMcpeTakeItemInfo;

typedef struct PexMcpeEntityMotionInfo {
    uint64_t eid;
    float mx, my, mz;
} PexMcpeEntityMotionInfo;

typedef struct PexMcpeEntityEventInfo {
    uint64_t eid;
    int event;
} PexMcpeEntityEventInfo;

typedef struct PexMcpeLevelEventInfo {
    int event_id;
    float x, y, z;
    int data;
} PexMcpeLevelEventInfo;

typedef struct PexMcpeAttributesInfo {
    uint64_t eid;
    float health;
    float hunger;
    int has_health;
    int has_hunger;
} PexMcpeAttributesInfo;

typedef struct PexMcpeArmorInfo {
    uint64_t eid;
    PexMcpeItemStackInfo slots[4];
} PexMcpeArmorInfo;

#define PEX_MCPE_MAX_INVENTORY_SLOTS 64

typedef struct PexMcpeContainerContentInfo {
    int window_id;
    int slot_count;
    PexMcpeItemStackInfo slots[PEX_MCPE_MAX_INVENTORY_SLOTS];
    int hotbar_count;
    int hotbar[16];
} PexMcpeContainerContentInfo;

typedef struct PexMcpeContainerSlotInfo {
    int window_id;
    int slot;
    int hotbar_slot;
    PexMcpeItemStackInfo item;
} PexMcpeContainerSlotInfo;

int pex_mcpe_decode_add_player_packet(const uint8_t *data, size_t size, PexMcpeRemotePlayerInfo *out_info);
int pex_mcpe_decode_move_player_packet(const uint8_t *data, size_t size, PexMcpeEntityMoveInfo *out_info);
int pex_mcpe_decode_move_entity_packet(const uint8_t *data, size_t size, PexMcpeEntityMoveInfo *out_info);
int pex_mcpe_decode_remove_entity_packet(const uint8_t *data, size_t size, uint64_t *out_eid);
int pex_mcpe_decode_player_list_packet(const uint8_t *data, size_t size, PexMcpePlayerListSkin *out_entries, size_t max_entries, size_t *out_count);
void pex_mcpe_free_player_list_entries(PexMcpePlayerListSkin *entries, size_t count);
int pex_mcpe_decode_mob_equipment_packet(const uint8_t *data, size_t size, PexMcpeMobEquipmentInfo *out_info);
int pex_mcpe_decode_animate_packet(const uint8_t *data, size_t size, PexMcpeAnimateInfo *out_info);
int pex_mcpe_decode_add_item_entity_packet(const uint8_t *data, size_t size, PexMcpeDroppedItemInfo *out_info);
int pex_mcpe_decode_take_item_entity_packet(const uint8_t *data, size_t size, PexMcpeTakeItemInfo *out_info);
int pex_mcpe_decode_set_entity_motion_packet(const uint8_t *data, size_t size, PexMcpeEntityMotionInfo *out_entries, size_t max_entries, size_t *out_count);
int pex_mcpe_decode_entity_event_packet(const uint8_t *data, size_t size, PexMcpeEntityEventInfo *out_info);
int pex_mcpe_decode_level_event_packet(const uint8_t *data, size_t size, PexMcpeLevelEventInfo *out_info);
int pex_mcpe_decode_update_attributes_packet(const uint8_t *data, size_t size, PexMcpeAttributesInfo *out_info);
int pex_mcpe_decode_mob_armor_equipment_packet(const uint8_t *data, size_t size, PexMcpeArmorInfo *out_info);
int pex_mcpe_decode_container_set_content_packet(const uint8_t *data, size_t size, PexMcpeContainerContentInfo *out_info);
int pex_mcpe_decode_container_set_slot_packet(const uint8_t *data, size_t size, PexMcpeContainerSlotInfo *out_info);
int pex_mcpe_encode_animate_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size, uint64_t eid, int action);
int pex_mcpe_encode_container_set_slot_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size,
                                             int window_id, int slot, int hotbar_slot,
                                             int item_id, int count, int damage);
int pex_mcpe_encode_drop_item_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size,
                                     int item_id, int count, int damage);
int pex_mcpe_encode_respawn_packet(uint8_t *out_data, size_t out_capacity, size_t *out_size,
                                   float x, float y, float z);

#ifdef __cplusplus
}
#endif

#endif /* PEXCRAFT_MCPE_PROTOCOL_81_ENTITY_PACKETS_H */
