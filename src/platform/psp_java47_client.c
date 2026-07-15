/* PSP-1000 Java Edition protocol 47 client glue.
   Deliberately contains no PXNET, RakNet, MCPE, local world generation, skin
   downloader, server-icon cache, or disk-backed server list. */

typedef enum PexMpJoinBackend {
    PEX_MP_JOIN_BACKEND_PXNET = 0,
    PEX_MP_JOIN_BACKEND_BEDROCK_PROTOCOL_81 = 1,
    PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE = 2
} PexMpJoinBackend;
static PexMpJoinBackend g_mp_join_backend = PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE;

#define PEX_MP_SERVER_LIST_MAX 8
#define PEX_MP_SERVER_ROW_BASE 6000
#define PEX_MP_SERVER_ROW_HEIGHT 36

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
    int server_kind;
    int resource_mode;
    char icon_path[MAX_PATHBUF];
    int icon_available;
    int icon_generation;
    int icon_loaded_generation;
    Texture icon;
    HANDLE thread;
} PexMpServerEntry;

static PexMpServerEntry g_mp_server_entries[PEX_MP_SERVER_LIST_MAX];
static int g_mp_server_count = 0;
static int g_mp_server_selected = -1;
static int g_mp_server_scroll = 0;
static int g_mp_server_mode = 0; /* list/direct/add/edit/delete */
static int g_mp_server_edit_field = 0;
static char g_mp_server_edit_name[64] = "Minecraft Server";
static char g_mp_server_edit_address[96] = "";
static int g_mp_server_last_clicked = -1;
static int g_mp_server_last_click_tick = -1000;
static int g_mp_server_dragging_scrollbar = 0;
static int g_mp_server_drag_start_y = 0;
static int g_mp_server_drag_start_scroll = 0;

static void pex_net_set_status(const char *msg) {
    snprintf(g_multiplayer_status, sizeof(g_multiplayer_status), "%s", msg ? msg : "");
}

static void pex_psp_parse_host_port(const char *server, char *host, size_t cap, int *port) {
    if (host && cap) host[0] = 0;
    if (port) *port = PEX_JAVA47_DEFAULT_PORT;
    if (!server || !server[0] || !host || cap == 0) return;
    const char *colon = strrchr(server, ':');
    if (colon && colon != server && strchr(colon + 1, ':') == NULL) {
        char *end = NULL;
        long parsed = strtol(colon + 1, &end, 10);
        if (end && !*end && parsed > 0 && parsed <= 65535) {
            size_t n = (size_t)(colon - server);
            if (n >= cap) n = cap - 1;
            memcpy(host, server, n);
            host[n] = 0;
            if (port) *port = (int)parsed;
            return;
        }
    }
    snprintf(host, cap, "%s", server);
}

static PexNetRenderPlayerState *pex_net_find_render_player(int player_id) {
    for (int i = 0; i < PEX_NET_MAX_PLAYERS; ++i)
        if (g_mp_render_players[i].active && g_mp_render_players[i].player_id == player_id)
            return &g_mp_render_players[i];
    return NULL;
}

static PexNetRenderDropState *pex_net_find_render_drop(int net_id) {
    for (int i = 0; i < PEX_NET_MAX_DROPS; ++i)
        if (g_mp_render_drops[i].active && g_mp_render_drops[i].net_id == net_id)
            return &g_mp_render_drops[i];
    return NULL;
}

static void pex_psp_free_render_player(PexNetRenderPlayerState *r) {
    if (!r) return;
    if (r->has_skin) free_texture(&r->skin);
    memset(r, 0, sizeof(*r));
}

static void pex_net_release_render_resources(void) {
    for (int i = 0; i < PEX_NET_MAX_PLAYERS; ++i) pex_psp_free_render_player(&g_mp_render_players[i]);
    memset(g_mp_render_drops, 0, sizeof(g_mp_render_drops));
}

static void pex_psp_clear_remote_state(void) {
    memset(g_mp_players, 0, sizeof(g_mp_players));
    memset(g_mp_prev_players, 0, sizeof(g_mp_prev_players));
    g_mp_player_count = 0;
    pex_net_release_render_resources();
}

static void pex_net_java_server_closed_window(void) {
    if (g_screen == SCREEN_WORKBENCH || g_screen == SCREEN_FURNACE || g_screen == SCREEN_CHEST)
        set_screen(SCREEN_INGAME);
}

static void pex_net_mark_chunk_ready(int chunk_x, int chunk_z) {
    int lcx = (chunk_x * PEX_NET_CHUNK_SIZE - g_flat_world_origin_x) / FLAT_RENDER_CHUNK;
    int lcz = (chunk_z * PEX_NET_CHUNK_SIZE - g_flat_world_origin_z) / FLAT_RENDER_CHUNK;
    if (lcx >= 0 && lcx < FLAT_RENDER_CHUNKS && lcz >= 0 && lcz < FLAT_RENDER_CHUNKS)
        stream_mark_chunk_generated(lcx, lcz);
}

/* PSP-1000 initially uses the embedded Steve texture for every player. */
static void pex_net_apply_skin(const PexNetSkin *skin, uint32_t size) { (void)skin; (void)size; }

static void pex_mp_cache_save_request_stop(void) { }
static void pex_mp_cache_save_shutdown(void) { }

static void pex_net_disconnect_request_stop(void) {
    if (pex_java47_is_active()) pex_java47_disconnect();
}

static void pex_net_disconnect(void) {
    if (pex_java47_is_active()) pex_java47_disconnect();
    g_mp_socket = INVALID_SOCKET;
    g_mp_connected = 0;
    g_mp_connecting = 0;
    g_mp_world_ready = 0;
    g_mp_player_id = 0;
    g_mp_connect_progress = 0;
    g_mp_pending_respawn_sync = 0;
    pex_psp_clear_remote_state();
}

static int pex_net_connect_to_server(const char *server) {
    char host[96];
    int port = PEX_JAVA47_DEFAULT_PORT;
    pex_psp_parse_host_port(server, host, sizeof(host), &port);
    if (!host[0]) { pex_net_set_status("Enter a Java server address."); return 0; }

    pex_net_disconnect();
    if (!g_mp_winsock_started) {
        WSADATA wsa;
        pex_net_set_status("Connecting PSP Wi-Fi...");
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            pex_net_set_status("PSP Wi-Fi failed. Configure an Infrastructure connection first.");
            return 0;
        }
        g_mp_winsock_started = 1;
    }

    g_mp_join_backend = PEX_MP_JOIN_BACKEND_JAVA_PROTOCOL_47JE;
    g_mp_connect_progress = 5;
    g_mp_connecting = 1;
    g_mp_connected = 0;
    g_mp_world_ready = 0;
    snprintf(g_mp_server_name, sizeof(g_mp_server_name), "%s:%d", host, port);
    snprintf(g_multiplayer_ip, sizeof(g_multiplayer_ip), "%s:%d", host, port);
    snprintf(g_opts.last_server, sizeof(g_opts.last_server), "%s:%d", host, port);

    memset(g_flat_blocks, 0, sizeof(g_flat_blocks));
    memset(g_flat_meta, 0, sizeof(g_flat_meta));
    memset(g_flat_levels, 0, sizeof(g_flat_levels));
    memset(g_flat_block_light, 0, sizeof(g_flat_block_light));
    memset(g_flat_sky_light, 0, sizeof(g_flat_sky_light));
    memset(g_flat_world_chunk_generated, 0, sizeof(g_flat_world_chunk_generated));
    flat_center_origin_near(0.0f, 0.0f);

    pex_net_set_status("Connecting to Java 1.8.8 server...");
    if (!pex_java47_begin_join(host, port, g_multiplayer_username[0] ? g_multiplayer_username : "Player")) {
        g_mp_connecting = 0;
        pex_net_set_status(pex_java47_disconnect_reason());
        return 0;
    }
    return 1;
}

static int pex_net_is_connecting(void) { return g_mp_connecting || (pex_java47_is_active() && !g_mp_world_ready); }

static void pex_net_poll(void) {
    if (world_quit_is_active() || g_screen == SCREEN_SAVING_QUIT) return;
    if (!pex_java47_is_active()) return;
    if (!pex_java47_tick()) {
        char reason[256];
        snprintf(reason, sizeof(reason), "%s", pex_java47_disconnect_reason());
        pex_net_disconnect();
        open_notice("Disconnected", reason[0] ? reason : "Java connection lost.", "");
        return;
    }
    g_mp_connect_progress = pex_java47_progress();
    pex_net_set_status(pex_java47_status_text());
    if (pex_java47_has_join_game()) {
        g_mp_connected = 1;
        g_mp_connecting = 0;
        g_mp_player_id = pex_java47_local_entity_id();
    }
    if (!g_mp_world_ready && pex_java47_world_ready()) {
        g_mp_world_ready = 1;
        g_mp_connect_progress = 100;
        pex_net_set_status("Done!");
        g_chat_count = 0;
        hud_add_chat("Connected to Java 1.8.8 server.");
        set_screen(SCREEN_INGAME);
    }
}

static float pex_psp_smooth_step(float dt, float sharpness) {
    float a = dt * sharpness;
    if (a < 0.0f) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    return a;
}
static float pex_psp_angle_delta(float from, float to) {
    float d = fmodf(to - from, 360.0f);
    if (d > 180.0f) d -= 360.0f;
    if (d < -180.0f) d += 360.0f;
    return d;
}

static void pex_net_update_smoothing(void) {
    if (!g_mp_connected || world_quit_is_active()) return;
    double now = now_seconds();
    if (g_mp_render_last_time <= 0.0) g_mp_render_last_time = now;
    float dt = (float)(now - g_mp_render_last_time);
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.10f) dt = 0.10f;
    g_mp_render_last_time = now;
    float alpha = pex_psp_smooth_step(dt, 16.0f);
    unsigned char seen[PEX_NET_MAX_PLAYERS];
    memset(seen, 0, sizeof(seen));
    for (int i = 0; i < g_mp_player_count; ++i) {
        PexNetPlayerState *p = &g_mp_players[i];
        if (p->player_id <= 0 || p->player_id == g_mp_player_id) continue;
        PexNetRenderPlayerState *r = pex_net_find_render_player(p->player_id);
        if (!r) {
            for (int j = 0; j < PEX_NET_MAX_PLAYERS; ++j) if (!g_mp_render_players[j].active) {
                r = &g_mp_render_players[j]; memset(r, 0, sizeof(*r));
                r->active = 1; r->player_id = p->player_id;
                r->x = r->prev_x = p->x; r->y = r->prev_y = p->y; r->z = r->prev_z = p->z;
                r->yaw = r->prev_yaw = p->yaw; r->pitch = r->prev_pitch = p->pitch;
                break;
            }
        }
        if (!r) continue;
        r->prev_x = r->x; r->prev_y = r->y; r->prev_z = r->z;
        r->prev_yaw = r->yaw; r->prev_pitch = r->pitch;
        r->x += (p->x - r->x) * alpha; r->y += (p->y - r->y) * alpha; r->z += (p->z - r->z) * alpha;
        r->yaw += pex_psp_angle_delta(r->yaw, p->yaw) * alpha;
        r->pitch += (p->pitch - r->pitch) * alpha;
        r->health = p->health; r->flags = p->flags;
        snprintf(r->name, sizeof(r->name), "%s", p->name[0] ? p->name : "Player");
        pex_java47_get_equipment(p->player_id, &r->held_item_id, &r->held_item_count, &r->held_item_damage, &r->held_slot);
        pex_java47_get_armor(p->player_id, r->armor);
        for (int j = 0; j < PEX_NET_MAX_PLAYERS; ++j) if (&g_mp_render_players[j] == r) { seen[j] = 1; break; }
    }
    for (int i = 0; i < PEX_NET_MAX_PLAYERS; ++i)
        if (g_mp_render_players[i].active && !seen[i] && !g_mp_render_players[i].skin_only)
            pex_psp_free_render_player(&g_mp_render_players[i]);
}

static void pex_net_send_player_state(void) {
    if (!g_mp_connected || !g_mp_world_ready || !pex_java47_is_playing()) return;
    int sneaking = (g_screen == SCREEN_INGAME && key_down_vk(g_opts.keys[5])) ? 1 : 0;
    double x = 0.0, feet_y = 0.0, z = 0.0;
    java47_player_network_position(&x, &feet_y, &z);
    pex_java47_send_player_state(x, feet_y, z, g_player_yaw, g_player_pitch,
                                 g_player_on_ground, sneaking, g_player_sprinting,
                                 g_selected_hotbar_slot);
}

static int pex_net_send_block_interact(int x, int y, int z, int face) {
    if (!g_mp_connected || !g_mp_world_ready || !pex_java47_is_playing()) return 0;
    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    pex_java47_send_place(x, y, z, face, stack_empty(held) ? 0 : held->id,
                          stack_empty(held) ? 0 : held->count, stack_empty(held) ? 0 : held->damage,
                          0.5f, 0.5f, 0.5f);
    return 1;
}
static int pex_net_send_use_item_air(void) {
    if (!g_mp_connected || !g_mp_world_ready || !pex_java47_is_playing()) return 0;
    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    pex_java47_send_use_item(stack_empty(held) ? 0 : held->id, stack_empty(held) ? 0 : held->count,
                             stack_empty(held) ? 0 : held->damage);
    return 1;
}
static void pex_net_send_release_use_item(void) { if (g_mp_connected && g_mp_world_ready) pex_java47_send_release_use_item(); }
static int pex_net_send_inventory_click(int slot, int button, int mode, const ItemStack *result) {
    return g_mp_connected && g_mp_world_ready ? pex_java47_send_window_click(slot, button, mode, result) : 0;
}
static int pex_net_send_creative_slot(int slot, const ItemStack *stack) {
    return g_mp_connected && g_mp_world_ready ? pex_java47_send_creative_slot(slot, stack) : 0;
}
static void pex_net_send_container_close(void) { if (g_mp_connected) pex_java47_send_close_window(); }

static void pex_net_send_block_action(int action, int x, int y, int z, int face, int block_id) {
    if (!g_mp_connected || !g_mp_world_ready) return;
    if (action == PEX_BLOCK_BREAK) return;
    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    int tx = x, ty = y, tz = z;
    if (block_id != BLOCK_FARMLAND) {
        if (face == 0) ty += 1; else if (face == 1) ty -= 1;
        else if (face == 2) tz += 1; else if (face == 3) tz -= 1;
        else if (face == 4) tx += 1; else if (face == 5) tx -= 1;
    }
    pex_java47_send_place(tx, ty, tz, face, stack_empty(held) ? 0 : held->id,
                          stack_empty(held) ? 0 : held->count, stack_empty(held) ? 0 : held->damage,
                          0.5f, 0.5f, 0.5f);
}

static void net_send_action_progress(int action, int x, int y, int z, int face, int block_id, int progress) {
    (void)block_id;
    if (!g_mp_connected || !g_mp_world_ready) return;
    if (action == PEX_ACTION_SWING) pex_java47_send_swing();
    else if (action == PEX_ACTION_ATTACK) { pex_java47_send_swing(); pex_java47_send_attack(x); }
    else if (action == PEX_ACTION_MINE_HIT) {
        pex_java47_send_swing();
        if (progress < 0) pex_java47_send_dig(1, x, y, z, face);
        else if (progress == 0) pex_java47_send_dig(0, x, y, z, face);
    } else if (action == PEX_ACTION_BREAK) pex_java47_send_dig(player_is_creative() ? 0 : 2, x, y, z, face);
}
static void pex_net_send_player_action(int action, int x, int y, int z, int face, int block_id) {
    net_send_action_progress(action, x, y, z, face, block_id, 0);
}
static int pex_net_player_ray_distance(float max_dist, float *out_t) {
    (void)max_dist; if (out_t) *out_t = 9999.0f; return 0;
}
static int pex_net_try_attack_player(void) {
    if (!g_mp_connected || !g_mp_world_ready) return 0;
    return pex_java47_try_attack_mob(player_is_creative() ? 6.0f : 3.0f);
}
static void pex_net_send_drop_item(ItemStack st) { if (g_mp_connected && !stack_empty(&st)) pex_java47_send_drop(st.count > 1); }
static void pex_net_send_pickup_drop(int drop_id) { (void)drop_id; }
static void pex_net_send_chest_open(void) { }
static void pex_net_send_chest_update(void) { }
static void pex_net_send_craft_request(void) { if (g_mp_connected) pex_java47_sync_inventory_from_game(); }
static void pex_net_send_chat(const char *text) { if (g_mp_connected && g_mp_world_ready && text && text[0]) pex_java47_send_chat(text); }
static void pex_net_send_respawn(void) { if (g_mp_connected) pex_java47_send_respawn(); }
static void pex_net_send_skin(void) { }
static void net_request_chunks_around_player(int force) { (void)force; }

/* ------------------------ tiny in-memory server list --------------------- */
static int pex_mp_server_visible_rows(void) {
    int rows = (g_gui_h - 104) / PEX_MP_SERVER_ROW_HEIGHT;
    return rows > 0 ? rows : 1;
}
static void pex_mp_server_clamp_scroll(void) {
    int max = g_mp_server_count - pex_mp_server_visible_rows();
    if (max < 0) max = 0;
    if (g_mp_server_scroll < 0) g_mp_server_scroll = 0;
    if (g_mp_server_scroll > max) g_mp_server_scroll = max;
}
static void pex_mp_server_list_ensure(void) {
    if (g_mp_server_count || !g_opts.last_server[0]) return;
    PexMpServerEntry *e = &g_mp_server_entries[0];
    memset(e, 0, sizeof(*e));
    snprintf(e->name, sizeof(e->name), "Last Server");
    snprintf(e->host, sizeof(e->host), "%s", g_opts.last_server);
    snprintf(e->motd, sizeof(e->motd), "Java Edition protocol 47");
    e->server_kind = 1; e->protocol = 47; e->ping_ms = -1;
    g_mp_server_count = 1; g_mp_server_selected = 0;
}
static int pex_mp_server_count_get(void) { return g_mp_server_count; }
static int pex_mp_server_selected_get(void) { return g_mp_server_selected; }
static int pex_mp_server_scroll_get(void) { return g_mp_server_scroll; }
static int pex_mp_server_mode_get(void) { return g_mp_server_mode; }
static int pex_mp_server_edit_field_get(void) { return g_mp_server_edit_field; }
static const char *pex_mp_server_edit_name_get(void) { return g_mp_server_edit_name; }
static const char *pex_mp_server_edit_address_get(void) { return g_mp_server_edit_address; }
static const PexMpServerEntry *pex_mp_server_entry_get(int index) {
    return index >= 0 && index < g_mp_server_count ? &g_mp_server_entries[index] : NULL;
}
static Texture *pex_mp_server_icon_texture(int index) { (void)index; return NULL; }
static void pex_mp_server_scroll_by(int rows) { g_mp_server_scroll += rows; pex_mp_server_clamp_scroll(); }
static void pex_mp_server_select(int index) { if (index >= 0 && index < g_mp_server_count) g_mp_server_selected = index; }
static void pex_mp_server_refresh_one(int index) {
    if (index < 0 || index >= g_mp_server_count) return;
    PexMpServerEntry *e = &g_mp_server_entries[index];
    char host[96]; int port;
    pex_psp_parse_host_port(e->host, host, sizeof(host), &port);
    PexJava47Status st; memset(&st, 0, sizeof(st));
    e->polling = 1;
    if (WSAStartup(MAKEWORD(2,2), NULL) == 0 && pex_java47_probe_status(host, port, 1200, &st)) {
        e->polled = 1; e->protocol = st.protocol; e->ping_ms = st.ping_ms; e->server_kind = 1;
        snprintf(e->motd, sizeof(e->motd), "%s", st.motd[0] ? st.motd : "Java server");
        snprintf(e->version, sizeof(e->version), "%s", st.version);
        snprintf(e->player_count, sizeof(e->player_count), "%d/%d", st.online_players, st.max_players);
    } else {
        e->polled = 1; e->protocol = 0; e->ping_ms = -1; e->server_kind = 1;
        snprintf(e->motd, sizeof(e->motd), "Cannot reach server");
        e->player_count[0] = 0; e->version[0] = 0;
    }
    e->polling = 0;
}
static void pex_mp_server_refresh_all(void) { for (int i = 0; i < g_mp_server_count; ++i) pex_mp_server_refresh_one(i); }
static void pex_mp_server_begin_direct(void) {
    g_mp_server_mode = 1; g_mp_server_edit_field = 0;
    g_mp_server_edit_name[0] = 0;
    snprintf(g_mp_server_edit_address, sizeof(g_mp_server_edit_address), "%s", g_opts.last_server);
}
static void pex_mp_server_begin_add(void) {
    g_mp_server_mode = 2; g_mp_server_edit_field = 1;
    snprintf(g_mp_server_edit_name, sizeof(g_mp_server_edit_name), "Minecraft Server");
    g_mp_server_edit_address[0] = 0;
}
static void pex_mp_server_begin_edit(void) {
    if (g_mp_server_selected < 0 || g_mp_server_selected >= g_mp_server_count) return;
    PexMpServerEntry *e = &g_mp_server_entries[g_mp_server_selected];
    g_mp_server_mode = 3; g_mp_server_edit_field = 0;
    snprintf(g_mp_server_edit_name, sizeof(g_mp_server_edit_name), "%s", e->name);
    snprintf(g_mp_server_edit_address, sizeof(g_mp_server_edit_address), "%s", e->host);
}
static void pex_mp_server_begin_delete_confirm(void) { if (g_mp_server_selected >= 0) g_mp_server_mode = 4; }
static void pex_mp_server_cancel_edit(void) { g_mp_server_mode = 0; }
static void pex_mp_server_commit_edit(void) {
    if (!g_mp_server_edit_address[0]) return;
    if (g_mp_server_mode == 1) {
        snprintf(g_multiplayer_ip, sizeof(g_multiplayer_ip), "%s", g_mp_server_edit_address);
        g_mp_server_mode = 0;
        return;
    }
    int index = g_mp_server_mode == 3 ? g_mp_server_selected : g_mp_server_count;
    if (index < 0 || index >= PEX_MP_SERVER_LIST_MAX) return;
    PexMpServerEntry *e = &g_mp_server_entries[index];
    memset(e, 0, sizeof(*e));
    snprintf(e->name, sizeof(e->name), "%s", g_mp_server_edit_name[0] ? g_mp_server_edit_name : "Minecraft Server");
    snprintf(e->host, sizeof(e->host), "%s", g_mp_server_edit_address);
    snprintf(e->motd, sizeof(e->motd), "Java Edition protocol 47");
    e->server_kind = 1; e->protocol = 47; e->ping_ms = -1;
    if (index == g_mp_server_count) ++g_mp_server_count;
    g_mp_server_selected = index; g_mp_server_mode = 0; pex_mp_server_clamp_scroll();
}
static void pex_mp_server_delete_selected(void) {
    int i = g_mp_server_selected;
    if (i < 0 || i >= g_mp_server_count) { g_mp_server_mode = 0; return; }
    memmove(&g_mp_server_entries[i], &g_mp_server_entries[i+1], (size_t)(g_mp_server_count-i-1)*sizeof(g_mp_server_entries[0]));
    --g_mp_server_count;
    if (g_mp_server_selected >= g_mp_server_count) g_mp_server_selected = g_mp_server_count - 1;
    g_mp_server_mode = 0; pex_mp_server_clamp_scroll();
}
static int pex_mp_server_connect_selected(void) {
    if (g_mp_server_selected < 0 || g_mp_server_selected >= g_mp_server_count) return 0;
    snprintf(g_multiplayer_ip, sizeof(g_multiplayer_ip), "%s", g_mp_server_entries[g_mp_server_selected].host);
    return pex_net_connect_to_server(g_multiplayer_ip);
}
static int pex_mp_server_select_or_double_click(int index) {
    int dbl = index == g_mp_server_last_clicked && g_ticks - g_mp_server_last_click_tick <= 8;
    pex_mp_server_select(index); g_mp_server_last_clicked = index; g_mp_server_last_click_tick = g_ticks;
    return dbl ? pex_mp_server_connect_selected() : 0;
}
static int pex_mp_server_scrollbar_geometry(int *out_x, int *out_top, int *out_bottom,
                                             int *out_thumb_y, int *out_thumb_h) {
    int top = 32, bottom = g_gui_h - 64, view_h = bottom - top;
    int visible = pex_mp_server_visible_rows();
    int total = g_mp_server_count;
    int x = g_gui_w / 2 + 160;
    if (x > g_gui_w - 8) x = g_gui_w - 8;
    if (out_x) *out_x = x;
    if (out_top) *out_top = top;
    if (out_bottom) *out_bottom = bottom;
    if (total <= visible || view_h <= 0) {
        if (out_thumb_y) *out_thumb_y = top;
        if (out_thumb_h) *out_thumb_h = view_h;
        return 0;
    }
    int thumb_h = view_h * visible / total;
    if (thumb_h < 24) thumb_h = 24;
    if (thumb_h > view_h - 8) thumb_h = view_h - 8;
    int max_scroll = total - visible;
    int thumb_y = top + (view_h - thumb_h) * g_mp_server_scroll / max_scroll;
    if (out_thumb_y) *out_thumb_y = thumb_y;
    if (out_thumb_h) *out_thumb_h = thumb_h;
    return 1;
}
static void pex_mp_server_scrollbar_mouse_down(int mx, int my) {
    int x, top, bottom, thumb_y, thumb_h;
    g_mp_server_dragging_scrollbar = 0;
    if (g_mp_server_mode != 0 ||
        !pex_mp_server_scrollbar_geometry(&x, &top, &bottom, &thumb_y, &thumb_h)) return;
    if (mx < x - 3 || mx > x + 9 || my < top || my > bottom) return;
    if (my < thumb_y) pex_mp_server_scroll_by(-pex_mp_server_visible_rows());
    else if (my > thumb_y + thumb_h) pex_mp_server_scroll_by(pex_mp_server_visible_rows());
    else {
        g_mp_server_dragging_scrollbar = 1;
        g_mp_server_drag_start_y = my;
        g_mp_server_drag_start_scroll = g_mp_server_scroll;
    }
}
static void pex_mp_server_scrollbar_drag(int delta_y) {
    (void)delta_y;
    if (!g_mp_server_dragging_scrollbar || g_mp_server_mode != 0) return;
    int x, top, bottom, thumb_y, thumb_h;
    if (!pex_mp_server_scrollbar_geometry(&x, &top, &bottom, &thumb_y, &thumb_h)) return;
    int max_scroll = g_mp_server_count - pex_mp_server_visible_rows();
    int track = (bottom - top) - thumb_h;
    if (track > 0 && max_scroll > 0)
        g_mp_server_scroll = g_mp_server_drag_start_scroll +
            (g_mouse_y - g_mp_server_drag_start_y) * max_scroll / track;
    pex_mp_server_clamp_scroll();
}
static void pex_mp_server_scrollbar_mouse_up(void) { g_mp_server_dragging_scrollbar = 0; }
