#ifndef PEXCRAFT_NET_PROTOCOL_H
#define PEXCRAFT_NET_PROTOCOL_H

#include <stdint.h>

#define PEX_NET_DEFAULT_PORT 25566
#define PEX_NET_MAGIC "PXNET10"
#define PEX_NET_MAGIC_LEN 7
#define PEX_NET_PROTOCOL_VERSION 6
/* Client-side player tracking must be large enough for Java/Bedrock hubs.
   Keep the legacy PXNET snapshot wire format at 16 players below so this
   capacity increase does not change the custom protocol packet size. */
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MULTIPLAYER_ONLY) && PEX_PSP_MULTIPLAYER_ONLY
#define PEX_NET_MAX_PLAYERS 40
#define PEX_NET_SNAPSHOT_MAX_PLAYERS 16
#define PEX_NET_MAX_DROPS 24
#define PEX_NET_MAX_FALLING_BLOCKS 16
#define PEX_NET_MAX_CHAT 120
#else
#define PEX_NET_MAX_PLAYERS 128
#define PEX_NET_SNAPSHOT_MAX_PLAYERS 16
#define PEX_NET_MAX_DROPS 64
#define PEX_NET_MAX_FALLING_BLOCKS 64
#define PEX_NET_MAX_CHAT 180
#endif
#define PEX_NET_WORLD_SIZE 256
#define PEX_NET_WORLD_HEIGHT 256
#define PEX_NET_CHUNK_SIZE 16
#define PEX_NET_CHUNK_BLOCKS (PEX_NET_CHUNK_SIZE * PEX_NET_WORLD_HEIGHT * PEX_NET_CHUNK_SIZE)
#define PEX_NET_SECTION_HEIGHT 16
#define PEX_NET_SECTION_BLOCKS (PEX_NET_CHUNK_SIZE * PEX_NET_SECTION_HEIGHT * PEX_NET_CHUNK_SIZE)
#define PEX_NET_SECTIONS_Y (PEX_NET_WORLD_HEIGHT / PEX_NET_SECTION_HEIGHT)
#define PEX_NET_CHEST_SLOTS 54
#define PEX_NET_INVENTORY_SLOTS 36
#define PEX_NET_CRAFT_GRID_SLOTS 9
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MULTIPLAYER_ONLY) && PEX_PSP_MULTIPLAYER_ONLY
#define PEX_NET_MAX_CHUNK_HASHES 16
#else
#define PEX_NET_MAX_CHUNK_HASHES 128
#endif

enum {
    PEX_C2S_HELLO = 1,
    PEX_C2S_PLAYER_STATE = 2,
    PEX_C2S_BLOCK_ACTION = 3,
    PEX_C2S_DROP_ITEM = 4,
    PEX_C2S_CHAT = 5,
    PEX_C2S_DISCONNECT = 6,
    PEX_C2S_PICKUP_DROP = 7,
    PEX_C2S_CHEST_UPDATE = 8,
    PEX_C2S_PLAYER_ACTION = 9,
    PEX_C2S_SKIN = 10,
    PEX_C2S_CHEST_OPEN = 11,
    PEX_C2S_CRAFT_REQUEST = 12,
    PEX_C2S_INVENTORY_CLICK = 13,
    PEX_C2S_CHUNK_REQUEST = 14
};

enum {
    PEX_S2C_WELCOME = 101,
    PEX_S2C_CHUNK = 102,
    PEX_S2C_BLOCK_CHANGE = 103,
    PEX_S2C_SNAPSHOT = 104,
    PEX_S2C_CHAT = 105,
    PEX_S2C_KICK = 106,
    PEX_S2C_PICKUP_APPROVED = 107,
    PEX_S2C_CHUNK_RLE = 108,
    PEX_S2C_WORLD_DONE = 109,
    PEX_S2C_CHEST_UPDATE = 110,
    PEX_S2C_PLAYER_ACTION = 111,
    PEX_S2C_SKIN = 112,
    PEX_S2C_PLAYER_KNOCKBACK = 113,
    PEX_S2C_WORLD_START = 114,
    PEX_S2C_INVENTORY_UPDATE = 115,
    PEX_S2C_CHUNK_SECTION_RLE = 116,
    PEX_S2C_CHUNK_CACHE_HIT = 117
};

enum {
    PEX_ACTION_SWING = 0,
    PEX_ACTION_MINE_HIT = 1,
    PEX_ACTION_BREAK = 2,
    PEX_ACTION_PLACE = 3,
    PEX_ACTION_ATTACK = 4,
    PEX_ACTION_DIED = 5
};

enum {
    PEX_DEATH_GENERIC = 0,
    PEX_DEATH_FALL = 1,
    PEX_DEATH_LAVA = 2,
    PEX_DEATH_SUFFOCATION = 3,
    PEX_DEATH_VOID = 4
};

enum {
    PEX_BLOCK_BREAK = 0,
    PEX_BLOCK_PLACE = 1,
    PEX_BLOCK_BUCKET_PICKUP = 2,
    PEX_BLOCK_BUCKET_PLACE = 3
};

#define PEX_PLAYER_FLAG_SNEAKING 1
/* Render-only state used by protocol adapters.  These bits are intentionally
   outside the original PXNET flag set, so older private servers simply leave
   them clear. */
#define PEX_PLAYER_FLAG_INVISIBLE 2
#define PEX_PLAYER_FLAG_HIDE_NAMETAG 4
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MULTIPLAYER_ONLY) && PEX_PSP_MULTIPLAYER_ONLY
/* PSP uses the embedded Steve texture and never transports RGBA skins. */
#define PEX_NET_SKIN_MAX_BYTES 1
#else
#define PEX_NET_SKIN_MAX_BYTES (64 * 64 * 4)
#endif

typedef struct PexNetPacketHeader {
    uint16_t type;
    uint16_t reserved;
    uint32_t size;
} PexNetPacketHeader;

typedef struct PexNetHello {
    char magic[PEX_NET_MAGIC_LEN];
    uint8_t version;
    char name[32];
} PexNetHello;

typedef struct PexNetWelcome {
    int32_t player_id;
    int32_t world_type;
    int32_t origin_x;
    int32_t origin_z;
    int64_t seed;
    float spawn_x;
    float spawn_y;
    float spawn_z;
} PexNetWelcome;


typedef struct PexNetChunkHash {
    int32_t chunk_x;
    int32_t chunk_z;
    uint32_t hash;
} PexNetChunkHash;

typedef struct PexNetChunkCacheHit {
    int32_t chunk_x;
    int32_t chunk_z;
    uint32_t hash;
} PexNetChunkCacheHit;

typedef struct PexNetChunk {
    int32_t chunk_x;
    int32_t chunk_z;
    uint8_t blocks[PEX_NET_CHUNK_BLOCKS];
    uint8_t levels[PEX_NET_CHUNK_BLOCKS];
    uint8_t meta[PEX_NET_CHUNK_BLOCKS];
} PexNetChunk;

typedef struct PexNetChunkRleHeader {
    int32_t chunk_x;
    int32_t chunk_z;
    uint32_t cell_count;
    uint32_t run_count;
} PexNetChunkRleHeader;

typedef struct PexNetChunkRleRun {
    uint16_t count;
    uint8_t block;
    uint8_t level;
    uint8_t meta;
    uint8_t reserved;
} PexNetChunkRleRun;

typedef struct PexNetWorldDone {
    int32_t chunk_count;
} PexNetWorldDone;

typedef struct PexNetChunkSectionRleHeader {
    int32_t chunk_x;
    int32_t chunk_z;
    int32_t section_y;
    uint32_t cell_count;
    uint32_t run_count;
    uint8_t is_last_section;
    uint8_t reserved0;
    uint16_t reserved1;
} PexNetChunkSectionRleHeader;

typedef struct PexNetChunkSectionRleRun {
    uint16_t count;
    uint8_t block;
    uint8_t level;
    uint8_t meta;
    uint8_t reserved;
} PexNetChunkSectionRleRun;


typedef struct PexNetPlayerState {
    int32_t player_id;
    char name[32];
    float x;
    float y;
    float z;
    float yaw;
    float pitch;
    int32_t health;
    int32_t armor;
    int32_t flags;
    int32_t selected_slot;
} PexNetPlayerState;

typedef struct PexNetSkin {
    int32_t player_id;
    int32_t width;
    int32_t height;
    uint32_t byte_count;
    unsigned char rgba[PEX_NET_SKIN_MAX_BYTES];
} PexNetSkin;

typedef struct PexNetDropState {
    int32_t drop_id;
    int32_t active;
    int32_t id;
    int32_t count;
    int32_t damage;
    float x;
    float y;
    float z;
    float mx;
    float my;
    float mz;
    float rot;
    int32_t age;
    int32_t pickup_delay;
} PexNetDropState;

typedef struct PexNetFallingBlockState {
    int32_t entity_id;
    int32_t active;
    int32_t block_id;
    /* Protocol v5 async-delta falling blocks: the server sends only the
       visual path endpoints. Clients use a local timer to slide smoothly from
       start to end. Live position, previous position, velocity, and path age
       are intentionally not transmitted. */
    float start_x;
    float start_y;
    float start_z;
    float end_x;
    float end_y;
    float end_z;
    int32_t path_id;
    int32_t path_duration;
} PexNetFallingBlockState;

typedef struct PexNetPickupDrop {
    int32_t drop_id;
} PexNetPickupDrop;

typedef struct PexNetPickupApproved {
    int32_t drop_id;
    int32_t id;
    int32_t count;
    int32_t damage;
} PexNetPickupApproved;

typedef struct PexNetItemStackState {
    int32_t id;
    int32_t count;
    int32_t damage;
} PexNetItemStackState;

typedef struct PexNetChestOpen {
    int32_t chest_count;
    int32_t x[2];
    int32_t y[2];
    int32_t z[2];
} PexNetChestOpen;

typedef struct PexNetChestUpdate {
    int32_t chest_count;
    int32_t x[2];
    int32_t y[2];
    int32_t z[2];
    int32_t slot_count;
    PexNetItemStackState slots[PEX_NET_CHEST_SLOTS];
} PexNetChestUpdate;


typedef struct PexNetWorldStart {
    int32_t chunk_count;
} PexNetWorldStart;

typedef struct PexNetInventoryUpdate {
    int32_t player_id;
    int32_t slot_count;
    int32_t selected_slot;
    PexNetItemStackState slots[PEX_NET_INVENTORY_SLOTS];
} PexNetInventoryUpdate;

typedef struct PexNetCraftRequest {
    int32_t grid_w;
    int32_t grid_h;
    int32_t slot_count;
    PexNetItemStackState grid[PEX_NET_CRAFT_GRID_SLOTS];
    int32_t shift_take;
} PexNetCraftRequest;

typedef struct PexNetInventoryClick {
    int32_t slot;
    int32_t button;
    int32_t screen;
    PexNetItemStackState carried;
} PexNetInventoryClick;

typedef struct PexNetChunkRequest {
    int32_t center_chunk_x;
    int32_t center_chunk_z;
    int32_t radius;
    int32_t hash_count;
    PexNetChunkHash hashes[PEX_NET_MAX_CHUNK_HASHES];
} PexNetChunkRequest;

typedef struct PexNetSnapshot {
    uint32_t tick;
    int32_t player_count;
    PexNetPlayerState players[PEX_NET_SNAPSHOT_MAX_PLAYERS];
    int32_t drop_count;
    PexNetDropState drops[PEX_NET_MAX_DROPS];
    int32_t falling_count;
    PexNetFallingBlockState falling[PEX_NET_MAX_FALLING_BLOCKS];
} PexNetSnapshot;

typedef struct PexNetBlockAction {
    int32_t action;
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t face;
    int32_t block_id;
    int32_t held_id;
    int32_t held_damage;
    int32_t level;
    int32_t meta;
} PexNetBlockAction;

typedef struct PexNetPlayerAction {
    int32_t player_id;
    int32_t action;
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t face;
    int32_t block_id;
    int32_t progress;
} PexNetPlayerAction;

typedef struct PexNetBlockChange {
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t block_id;
    int32_t level;
    int32_t meta;
} PexNetBlockChange;

typedef struct PexNetKnockback {
    int32_t victim_id;
    int32_t attacker_id;
    float mx;
    float my;
    float mz;
    int32_t health;
} PexNetKnockback;

typedef struct PexNetDropItem {
    int32_t id;
    int32_t count;
    int32_t damage;
    float x;
    float y;
    float z;
    float yaw;
    float pitch;
} PexNetDropItem;

typedef struct PexNetChat {
    int32_t player_id;
    char text[PEX_NET_MAX_CHAT];
} PexNetChat;

#endif
