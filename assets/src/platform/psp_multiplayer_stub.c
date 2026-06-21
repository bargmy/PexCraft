static int g_mp_pending_respawn_sync = 0;
/* PSP network stubs.  Multiplayer is intentionally disabled in the first PSP port. */
static void pex_net_set_status(const char *msg) { snprintf(g_multiplayer_status, sizeof(g_multiplayer_status), "%s", msg ? msg : "Network disabled on PSP."); }
static int pex_net_connect_to_server(const char *server) { (void)server; pex_net_set_status("Network disabled on PSP."); return 0; }
static void pex_net_poll(void) { }
static int pex_net_is_connecting(void) { return 0; }
static void pex_net_update_smoothing(void) { }
static void pex_net_disconnect(void) { g_mp_connected = 0; g_mp_connecting = 0; g_mp_world_ready = 0; }
static void pex_net_send_player_state(void) { }
static void pex_net_send_block_action(int action, int x, int y, int z, int face, int block_id) { (void)action; (void)x; (void)y; (void)z; (void)face; (void)block_id; }
static void pex_net_send_player_action_progress(int action, int x, int y, int z, int face, int block_id, int progress) { (void)action; (void)x; (void)y; (void)z; (void)face; (void)block_id; (void)progress; }
static void pex_net_send_player_action(int action, int x, int y, int z, int face, int block_id) { pex_net_send_player_action_progress(action, x, y, z, face, block_id, 0); }
static int pex_net_player_ray_distance(float max_dist, float *out_t) { (void)max_dist; if (out_t) *out_t = 0.0f; return 0; }
static int pex_net_try_attack_player(void) { return 0; }
static void pex_net_send_drop_item(ItemStack st) { (void)st; }
static void pex_net_send_pickup_drop(int drop_id) { (void)drop_id; }
static void pex_net_send_chest_open(void) { }
static void pex_net_send_chest_update(void) { }
static void pex_net_send_craft_request(void) { }
static void pex_net_send_chat(const char *text) { (void)text; }
static void pex_net_send_skin(void) { }
static void pex_net_request_chunks_around_player(int force) { (void)force; }
static PexNetRenderDropState *pex_net_find_render_drop(int net_id) { (void)net_id; return NULL; }
