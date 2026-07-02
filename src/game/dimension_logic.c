/* PexCraft dimension logic: Nether/End dimension switching and portal teleportation.
   Included by src/main.c (unity build) after game/block_logic.c.
   References: WorldProvider.java, Teleporter.java (Minecraft JE 1.2.5). */

/* ----------------------------------------------------------------
   Build the chunk directory path for a given dimension.
   Follows Java's convention: overworld = <world>/chunks,
   Nether = <world>/DIM-1/chunks, End = <world>/DIM1/chunks.
   Creates the directory if it does not exist.
   ---------------------------------------------------------------- */
static void dimension_chunk_dir(int dimension, const char *world_dir, char *out, size_t cap) {
    if (dimension == -1)
        snprintf(out, cap, "%s\\DIM-1\\chunks", world_dir);
    else if (dimension == 1)
        snprintf(out, cap, "%s\\DIM1\\chunks", world_dir);
    else
        snprintf(out, cap, "%s\\chunks", world_dir);
    make_dir_recursive(out);
}

/* ----------------------------------------------------------------
   Search for an existing Nether portal within 128 blocks of (ox, oz).
   Returns 1 and sets *out_x/y/z if found, 0 otherwise.
   Port of Teleporter.placeInPortal() search phase.
   ---------------------------------------------------------------- */
static int dimension_find_nether_portal(int ox, int oz, int *out_x, int *out_y, int *out_z) {
    int best_dist2 = 0x7fffffff;
    int found = 0;
    for (int dx = -128; dx <= 128; dx += 2) {
        for (int dz = -128; dz <= 128; dz += 2) {
            int wx = ox + dx, wz = oz + dz;
            if (!flat_in_bounds(wx, 1, wz)) continue;
            for (int wy = 1; wy <= 120; wy++) {
                if (flat_get_block(wx, wy, wz) == BLOCK_PORTAL) {
                    int d2 = dx * dx + dz * dz;
                    if (d2 < best_dist2) {
                        best_dist2 = d2;
                        *out_x = wx;
                        *out_y = wy;
                        *out_z = wz;
                        found = 1;
                    }
                    break; /* only need the bottom-most portal block in this column */
                }
            }
        }
    }
    return found;
}

/* ----------------------------------------------------------------
   Build a new Nether portal frame at (px, py, pz).
   Creates a 4-wide x 5-tall obsidian frame (Z-axis orientation)
   and fills the 2x3 interior with BLOCK_PORTAL.
   ---------------------------------------------------------------- */
static void dimension_create_nether_portal(int px, int py, int pz) {
    if (!flat_in_bounds(px, py, pz)) {
        pex_logf("dimension: portal create out of bounds at %d,%d,%d", px, py, pz);
        return;
    }
    /* Clamp Y so we don't build into the void or above world height */
    if (py < 2)  py = 2;
    if (py > 118) py = 118;

    flat_begin_persistent_edit();
    /* Bottom sill */
    for (int i = 0; i < 4; i++)
        flat_set_block_raw(px + i, py - 1, pz, BLOCK_OBSIDIAN);
    /* Top sill */
    for (int i = 0; i < 4; i++)
        flat_set_block_raw(px + i, py + 3, pz, BLOCK_OBSIDIAN);
    /* Left wall */
    for (int dy = 0; dy < 3; dy++)
        flat_set_block_raw(px, py + dy, pz, BLOCK_OBSIDIAN);
    /* Right wall */
    for (int dy = 0; dy < 3; dy++)
        flat_set_block_raw(px + 3, py + dy, pz, BLOCK_OBSIDIAN);
    /* Interior portal blocks (2x3) */
    for (int di = 1; di <= 2; di++)
        for (int dy = 0; dy < 3; dy++)
            flat_set_block_raw(px + di, py + dy, pz, BLOCK_PORTAL);
    flat_end_persistent_edit();
    g_save_dirty = 1;
    pex_logf("dimension: created portal frame at %d,%d,%d", px, py, pz);
}

/* ----------------------------------------------------------------
   Main dimension-switching function.
   Saves the current world, moves the player to scaled destination
   coordinates, then finds or creates a portal there.
   ---------------------------------------------------------------- */
static void dimension_teleport_player(int target_dim) {
    pex_logf("dimension: teleport from dim=%d to dim=%d pos=%.1f,%.1f,%.1f",
             g_current_dimension, target_dim,
             (double)g_player_x, (double)g_player_y, (double)g_player_z);

    /* Save current world state before switching */
    save_world_state_sync();

    /* Calculate target coordinates */
    float nx = g_player_x, ny = g_player_y, nz = g_player_z;

    if (target_dim == PEX_DIM_NETHER) {
        /* Overworld -> Nether: divide X and Z by 8 (1:8 scale) */
        nx = g_player_x / 8.0f;
        nz = g_player_z / 8.0f;
        if (ny < 10.0f) ny = 10.0f;
        if (ny > 110.0f) ny = 110.0f;
    } else if (target_dim == PEX_DIM_OVERWORLD && g_current_dimension == PEX_DIM_NETHER) {
        /* Nether -> Overworld: multiply X and Z by 8 */
        nx = g_player_x * 8.0f;
        nz = g_player_z * 8.0f;
        if (ny < 10.0f) ny = 10.0f;
        if (ny > 200.0f) ny = 200.0f;
    } else if (target_dim == PEX_DIM_END) {
        /* -> End: fixed obsidian platform spawn (WorldProviderEnd.getEntrancePortalLocation) */
        nx = 0.5f;
        ny = 66.0f; /* one block above the platform at y=64 */
        nz = 0.5f;
    } else if (target_dim == PEX_DIM_OVERWORLD && g_current_dimension == PEX_DIM_END) {
        /* End -> Overworld: return to original overworld spawn */
        /* Keep current coords — they were saved */
        nx = g_player_x;
        ny = g_player_y;
        nz = g_player_z;
    }

    /* Switch dimension */
    g_current_dimension = target_dim;
    g_portal_timer = 0;

    /* Teleport player */
    g_player_x = nx; g_player_y = ny; g_player_z = nz;
    g_player_prev_x = nx; g_player_prev_y = ny; g_player_prev_z = nz;
    g_player_motion_x = 0.0f;
    g_player_motion_y = 0.0f;
    g_player_motion_z = 0.0f;
    g_player_fall_distance = 0.0f;

    /* Invalidate all chunk meshes to force full regeneration */
    flat_mark_all_chunks_dirty();

    /* For Nether/Overworld transitions: find or create the destination portal */
    if (target_dim == PEX_DIM_NETHER || target_dim == PEX_DIM_OVERWORLD) {
        int px = (int)nx, py = (int)ny, pz = (int)nz;
        if (dimension_find_nether_portal((int)nx, (int)nz, &px, &py, &pz)) {
            /* Place player just above the portal block */
            g_player_x = (float)px + 0.5f;
            g_player_y = (float)(py + 1);
            g_player_z = (float)pz + 0.5f;
            g_player_prev_x = g_player_x;
            g_player_prev_y = g_player_y;
            g_player_prev_z = g_player_z;
        } else {
            /* No portal found: create one and place player inside it */
            int bx = (int)nx - 1;
            int by = (int)ny;
            int bz = (int)nz;
            dimension_create_nether_portal(bx, by, bz);
            g_player_x = (float)bx + 1.5f;
            g_player_y = (float)by + 1.0f;
            g_player_z = (float)bz + 0.5f;
            g_player_prev_x = g_player_x;
            g_player_prev_y = g_player_y;
            g_player_prev_z = g_player_z;
        }
    }

    pex_logf("dimension: arrived in dim=%d at %.1f,%.1f,%.1f",
             g_current_dimension,
             (double)g_player_x, (double)g_player_y, (double)g_player_z);
}

/* ----------------------------------------------------------------
   Portal collision tick — called every game tick.
   Detects when the player stands in a BLOCK_PORTAL (Nether) or
   BLOCK_END_PORTAL (End) and triggers a dimension transition.
   Port of Entity.setInPortal() and EntityPlayerSP portal logic.
   ---------------------------------------------------------------- */
static void dimension_tick_portal_collision(void) {
    if (g_mp_connected) return;
    if (g_player_dead) return;

    int bx = (int)floorf(g_player_x);
    int by = (int)floorf(g_player_y + 0.5f); /* approximate player centre */
    int bz = (int)floorf(g_player_z);

    int in_nether_portal = 0;
    int in_end_portal = 0;

    /* Check a small vertical window that covers the player body */
    for (int dy = 0; dy <= 1 && !in_nether_portal && !in_end_portal; dy++) {
        if (!flat_in_bounds(bx, by + dy, bz)) continue;
        int bid = flat_get_block(bx, by + dy, bz);
        if (bid == BLOCK_PORTAL)     in_nether_portal = 1;
        if (bid == BLOCK_END_PORTAL) in_end_portal = 1;
    }

    if (in_end_portal) {
        /* End portal: instant teleport */
        if (g_current_dimension == PEX_DIM_OVERWORLD)
            dimension_teleport_player(PEX_DIM_END);
        else if (g_current_dimension == PEX_DIM_END)
            dimension_teleport_player(PEX_DIM_OVERWORLD);
        return;
    }

    if (in_nether_portal) {
        g_portal_timer++;
        if (g_portal_timer >= 80) { /* 4 seconds at 20 ticks/sec */
            if (g_current_dimension == PEX_DIM_OVERWORLD)
                dimension_teleport_player(PEX_DIM_NETHER);
            else
                dimension_teleport_player(PEX_DIM_OVERWORLD);
            g_portal_timer = 0;
        }
    } else {
        /* Gradually decrease timer while outside portal */
        if (g_portal_timer > 0) g_portal_timer--;
    }
}
