#ifndef PEXCRAFT_JAVA_PROTOCOL_47JE_H
#define PEXCRAFT_JAVA_PROTOCOL_47JE_H

#include <stddef.h>
#include <stdint.h>

#define PEX_JAVA47_PROTOCOL_VERSION 47
#define PEX_JAVA47_DEFAULT_PORT 25565

typedef struct PexJava47Status {
    int ok;
    int protocol;
    int ping_ms;
    int online_players;
    int max_players;
    char version[64];
    char motd[384];
} PexJava47Status;

typedef enum PexJava47State {
    PEX_JAVA47_IDLE = 0,
    PEX_JAVA47_CONNECTING,
    PEX_JAVA47_LOGIN,
    PEX_JAVA47_PLAY,
    PEX_JAVA47_FAILED
} PexJava47State;

int pex_java47_probe_status(const char *host, int port, int timeout_ms, PexJava47Status *out_status);
int pex_java47_begin_join(const char *host, int port, const char *username);
int pex_java47_tick(void);
void pex_java47_disconnect(void);
int pex_java47_is_active(void);
int pex_java47_is_playing(void);
int pex_java47_has_join_game(void);
int pex_java47_world_ready(void);
int pex_java47_progress(void);
const char *pex_java47_status_text(void);
const char *pex_java47_disconnect_reason(void);
int pex_java47_server_protocol(void);
int pex_java47_local_entity_id(void);
float pex_java47_movement_speed_scale(void);

int pex_java47_send_player_state(double x, double feet_y, double z, float yaw, float pitch, int on_ground,
                                 int sneaking, int sprinting, int held_slot);
int pex_java47_send_chat(const char *text);
int pex_java47_send_swing(void);
int pex_java47_send_attack(int entity_id);
int pex_java47_send_interact(int entity_id, float hit_x, float hit_y, float hit_z);
int pex_java47_send_dig(int status, int x, int y, int z, int face);
int pex_java47_send_place(int x, int y, int z, int face, int item_id, int count, int damage,
                          float cursor_x, float cursor_y, float cursor_z);
int pex_java47_send_drop(int whole_stack);
int pex_java47_send_use_item(int item_id, int count, int damage);
int pex_java47_send_release_use_item(void);
int pex_java47_send_respawn(void);
int pex_java47_send_close_window(void);
int pex_java47_sync_inventory_from_game(void);
int pex_java47_send_window_click(int pex_slot, int button, int mode, const ItemStack *clicked_result);
int pex_java47_send_creative_slot(int pex_slot, const ItemStack *stack);

int pex_java47_get_equipment(int entity_id, int *item_id, int *count, int *damage, int *slot);
int pex_java47_entity_is_player(int entity_id);
int pex_java47_player_name_tag_visible(const char *player_name);
Texture *pex_java47_local_skin_texture(void);
int pex_java47_try_attack_mob(float max_dist);
int pex_java47_try_interact_entity(float max_dist);
ItemStack *pex_java47_get_open_container_slot(int local_slot);
int pex_java47_open_container_slot_count(void);
const char *pex_java47_slot_custom_name(int pex_slot);
int pex_java47_slot_lore(int pex_slot, const char **out_lines, int max_lines);
int pex_java47_translate_block_id(int java_block_id);
int pex_java47_translate_item_id(int java_item_id);
int pex_java47_translate_mob_type(int java_mob_type);

void pex_java47_draw_server_scoreboard(void);
void pex_java47_draw_tab_overlay(void);
void pex_java47_draw_special_entities(float partial);

#endif
