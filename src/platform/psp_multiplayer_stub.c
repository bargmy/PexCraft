static int g_mp_pending_respawn_sync = 0;
/* PSP network stubs.  Multiplayer is intentionally disabled in the first PSP port. */
static void pex_net_set_status(const char *msg) { snprintf(g_multiplayer_status, sizeof(g_multiplayer_status), "%s", msg ? msg : "Network disabled on PSP."); }
static int pex_net_connect_to_server(const char *server) { (void)server; pex_net_set_status("Network disabled on PSP."); return 0; }
static void pex_net_poll(void) { }
static int pex_net_is_connecting(void) { return 0; }
static void pex_net_update_smoothing(void) { }
static void pex_net_disconnect(void) { g_mp_connected = 0; g_mp_connecting = 0; g_mp_world_ready = 0; }
static void pex_mp_cache_save_shutdown(void) { }
static void pex_net_send_player_state(void) { }
static void pex_net_send_block_action(int action, int x, int y, int z, int face, int block_id) { (void)action; (void)x; (void)y; (void)z; (void)face; (void)block_id; }
static void net_send_action_progress(int action, int x, int y, int z, int face, int block_id, int progress) { (void)action; (void)x; (void)y; (void)z; (void)face; (void)block_id; (void)progress; }
static void pex_net_send_player_action(int action, int x, int y, int z, int face, int block_id) { net_send_action_progress(action, x, y, z, face, block_id, 0); }
static int pex_net_player_ray_distance(float max_dist, float *out_t) { (void)max_dist; if (out_t) *out_t = 0.0f; return 0; }
static int pex_net_try_attack_player(void) { return 0; }
static void pex_net_send_drop_item(ItemStack st) { (void)st; }
static void pex_net_send_pickup_drop(int drop_id) { (void)drop_id; }
static void pex_net_send_chest_open(void) { }
static void pex_net_send_chest_update(void) { }
static void pex_net_send_craft_request(void) { }
static void pex_net_send_chat(const char *text) { (void)text; }
static void pex_net_send_skin(void) { }
static void net_request_chunks_around_player(int force) { (void)force; }
static PexNetRenderDropState *pex_net_find_render_drop(int net_id) { (void)net_id; return NULL; }

/* Shared multiplayer server-list UI stubs for PSP/Wii builds.  The desktop
   implementation lives in platform/multiplayer_client.c; portable ports keep
   multiplayer disabled but still compile the Java-style server list screens. */
#ifndef PEX_MP_SERVER_LIST_STUBS
#define PEX_MP_SERVER_LIST_STUBS 1
#define PEX_MP_SERVER_LIST_MAX 32
#define PEX_MP_SERVER_ROW_BASE 6000

typedef struct PexMpServerEntry {
    char name[64];
    char host[96];
    char motd[128];
    char player_count[32];
    char version[24];
    int protocol;
    int ping_ms;
    int polled;
    int polling;
    int valid_mcpe;
} PexMpServerEntry;

static int g_mp_server_mode = 0;
static int g_mp_server_edit_field = 0;
static char g_mp_server_edit_name[64] = "";
static char g_mp_server_edit_address[96] = "";
static PexMpServerEntry g_mp_server_stub_entry;

static void pex_mp_server_list_ensure(void) { }
static int pex_mp_server_count_get(void) { return 0; }
static int pex_mp_server_selected_get(void) { return -1; }
static int pex_mp_server_scroll_get(void) { return 0; }
static int pex_mp_server_mode_get(void) { return g_mp_server_mode; }
static int pex_mp_server_edit_field_get(void) { return g_mp_server_edit_field; }
static const char *pex_mp_server_edit_name_get(void) { return g_mp_server_edit_name; }
static const char *pex_mp_server_edit_address_get(void) { return g_mp_server_edit_address; }
static const PexMpServerEntry *pex_mp_server_entry_get(int index) { (void)index; return NULL; }
static int pex_mp_server_visible_rows(void) {
    int top = 32;
    int bottom = g_gui_h - 64;
    int rows = (bottom - top - 8) / 36;
    return rows > 0 ? rows : 1;
}
static void pex_mp_server_clamp_scroll(void) { }
static void pex_mp_server_scroll_by(int rows) { (void)rows; }
static void pex_mp_server_select(int index) { (void)index; }
static void pex_mp_server_refresh_all(void) { }
static void pex_mp_server_delete_selected(void) { }
static void pex_mp_server_begin_direct(void) {
    g_mp_server_mode = 1;
    g_mp_server_edit_field = 0;
    snprintf(g_mp_server_edit_name, sizeof(g_mp_server_edit_name), "");
    snprintf(g_mp_server_edit_address, sizeof(g_mp_server_edit_address), "%s", g_multiplayer_ip);
}
static void pex_mp_server_begin_add(void) {
    g_mp_server_mode = 2;
    g_mp_server_edit_field = 1;
    snprintf(g_mp_server_edit_name, sizeof(g_mp_server_edit_name), "Minecraft Server");
    snprintf(g_mp_server_edit_address, sizeof(g_mp_server_edit_address), "");
}
static void pex_mp_server_begin_edit(void) { pex_mp_server_begin_add(); }
static void pex_mp_server_cancel_edit(void) { g_mp_server_mode = 0; }
static void pex_mp_server_commit_edit(void) { g_mp_server_mode = 0; }
static int pex_mp_server_connect_selected(void) { return 0; }
#endif
