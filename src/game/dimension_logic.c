/* PexCraft dimension logic: Nether/End dimension switching and portal teleportation.
   Included by src/main.c (unity build) after game/block_logic.c.
   Behavior target: Minecraft JE 1.2.5-style dimension travel, portal search,
   portal creation, and post-teleport cooldown. */

#define PEX_PLAYER_EYE_HEIGHT 1.62f
#define PEX_NETHER_PORTAL_SEARCH_RADIUS 128
#define PEX_NETHER_PORTAL_CREATE_RADIUS 16
#define PEX_NETHER_PORTAL_COOLDOWN_TICKS 10

typedef struct PexPortalPlacement {
    int axis;      /* 0 = portal plane runs along X, 1 = portal plane runs along Z */
    int base_x;    /* frame corner / lower-left in the portal axis */
    int base_y;    /* bottom interior portal-block Y; frame sill is base_y - 1 */
    int base_z;
    float player_x;
    float player_y; /* eye Y */
    float player_z;
    double dist2;
} PexPortalPlacement;

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

static int dimension_portal_world_top(void) {
    int top = FLAT_WORLD_Y_MAX;
    /* 1.2.5 worlds are 128 high.  PC builds may keep a taller active buffer;
       keep Nether portal search/creation in the old 0..127 range when possible. */
    if (top > 127) top = 127;
    return top;
}

static int dimension_portal_replaceable(int id) {
    return id == 0 || id == BLOCK_PORTAL || id == BLOCK_FIRE || block_is_liquid(id) ||
           id == BLOCK_TALL_GRASS || id == BLOCK_DEAD_BUSH || id == BLOCK_SNOW_LAYER ||
           id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE ||
           id == BLOCK_BROWN_MUSHROOM || id == BLOCK_RED_MUSHROOM ||
           id == BLOCK_REDSTONE_WIRE || id == BLOCK_REEDS || id == BLOCK_NETHER_WART;
}

static int dimension_axis_from_yaw(void) {
    float yaw_rad = g_player_yaw * (float)M_PI / 180.0f;
    float sx = fabsf(sinf(yaw_rad));
    float cz = fabsf(cosf(yaw_rad));
    return (sx > cz) ? 1 : 0;
}

static int dimension_portal_axis_at(int x, int y, int z) {
    if ((flat_in_bounds(x - 1, y, z) && flat_get_block(x - 1, y, z) == BLOCK_PORTAL) ||
        (flat_in_bounds(x + 1, y, z) && flat_get_block(x + 1, y, z) == BLOCK_PORTAL)) return 0;
    if ((flat_in_bounds(x, y, z - 1) && flat_get_block(x, y, z - 1) == BLOCK_PORTAL) ||
        (flat_in_bounds(x, y, z + 1) && flat_get_block(x, y, z + 1) == BLOCK_PORTAL)) return 1;

    /* Single portal block / damaged frame fallback: infer from the obsidian rails,
       then from view direction. */
    if ((flat_in_bounds(x - 1, y, z) && flat_get_block(x - 1, y, z) == BLOCK_OBSIDIAN) ||
        (flat_in_bounds(x + 1, y, z) && flat_get_block(x + 1, y, z) == BLOCK_OBSIDIAN)) return 0;
    if ((flat_in_bounds(x, y, z - 1) && flat_get_block(x, y, z - 1) == BLOCK_OBSIDIAN) ||
        (flat_in_bounds(x, y, z + 1) && flat_get_block(x, y, z + 1) == BLOCK_OBSIDIAN)) return 1;
    return dimension_axis_from_yaw();
}

static int dimension_portal_bottom_y(int x, int y, int z) {
    while (y > FLAT_WORLD_Y_MIN && flat_in_bounds(x, y - 1, z) && flat_get_block(x, y - 1, z) == BLOCK_PORTAL)
        y--;
    return y;
}

static void dimension_portal_cluster_bounds(int x, int y, int z, int axis, int *out_min, int *out_max) {
    int mn, mx;
    if (axis == 0) {
        mn = mx = x;
        while (flat_in_bounds(mn - 1, y, z) && flat_get_block(mn - 1, y, z) == BLOCK_PORTAL) mn--;
        while (flat_in_bounds(mx + 1, y, z) && flat_get_block(mx + 1, y, z) == BLOCK_PORTAL) mx++;
    } else {
        mn = mx = z;
        while (flat_in_bounds(x, y, mn - 1) && flat_get_block(x, y, mn - 1) == BLOCK_PORTAL) mn--;
        while (flat_in_bounds(x, y, mx + 1) && flat_get_block(x, y, mx + 1) == BLOCK_PORTAL) mx++;
    }
    *out_min = mn;
    *out_max = mx;
}

static void dimension_portal_existing_place_from_block(int x, int y, int z, PexPortalPlacement *pl) {
    int axis = dimension_portal_axis_at(x, y, z);
    int bottom = dimension_portal_bottom_y(x, y, z);
    int mn, mx;
    dimension_portal_cluster_bounds(x, bottom, z, axis, &mn, &mx);

    pl->axis = axis;
    pl->base_y = bottom;
    if (axis == 0) {
        pl->base_x = mn - 1;
        pl->base_z = z;
        pl->player_x = ((float)mn + (float)mx + 1.0f) * 0.5f;
        pl->player_z = (float)z + 0.5f;
    } else {
        pl->base_x = x;
        pl->base_z = mn - 1;
        pl->player_x = (float)x + 0.5f;
        pl->player_z = ((float)mn + (float)mx + 1.0f) * 0.5f;
    }
    pl->player_y = (float)bottom + PEX_PLAYER_EYE_HEIGHT;
}

/* Search for the nearest existing Nether portal within the old Java 128-block
   radius.  Unlike the first PexCraft pass, this scans every X/Z column and uses
   the portal's actual center/feet Y for distance and placement. */
static int dimension_find_nether_portal(float ox, float oy, float oz, PexPortalPlacement *out) {
    int cx = (int)floorf(ox);
    int cz = (int)floorf(oz);
    int y0 = FLAT_WORLD_Y_MIN + 1;
    int y1 = dimension_portal_world_top();
    int found = 0;
    PexPortalPlacement best;
    memset(&best, 0, sizeof(best));
    best.dist2 = 1.0e100;

    for (int x = cx - PEX_NETHER_PORTAL_SEARCH_RADIUS; x <= cx + PEX_NETHER_PORTAL_SEARCH_RADIUS; x++) {
        for (int z = cz - PEX_NETHER_PORTAL_SEARCH_RADIUS; z <= cz + PEX_NETHER_PORTAL_SEARCH_RADIUS; z++) {
            if (!flat_in_bounds(x, y0, z)) continue;
            for (int y = y0; y <= y1; y++) {
                if (!flat_in_bounds(x, y, z) || flat_get_block(x, y, z) != BLOCK_PORTAL) continue;
                PexPortalPlacement cand;
                dimension_portal_existing_place_from_block(x, y, z, &cand);
                double dx = (double)cand.player_x - (double)ox;
                double dy = ((double)cand.player_y - (double)PEX_PLAYER_EYE_HEIGHT) - ((double)oy - (double)PEX_PLAYER_EYE_HEIGHT);
                double dz = (double)cand.player_z - (double)oz;
                cand.dist2 = dx * dx + dy * dy + dz * dz;
                if (!found || cand.dist2 < best.dist2) {
                    best = cand;
                    found = 1;
                }
            }
        }
    }

    if (found && out) *out = best;
    return found;
}

static void dimension_portal_player_pos_for_frame(int base_x, int base_y, int base_z, int axis,
                                                  float *px, float *py, float *pz) {
    if (axis == 0) {
        *px = (float)base_x + 2.0f;
        *pz = (float)base_z + 0.5f;
    } else {
        *px = (float)base_x + 0.5f;
        *pz = (float)base_z + 2.0f;
    }
    *py = (float)base_y + PEX_PLAYER_EYE_HEIGHT;
}

static int dimension_player_space_clear(float px, float feet_y, float pz) {
    float minx = px - 0.30f, maxx = px + 0.30f;
    float miny = feet_y + 0.001f, maxy = feet_y + 1.80f - 0.001f;
    float minz = pz - 0.30f, maxz = pz + 0.30f;
    int x0 = (int)floorf(minx), x1 = (int)floorf(maxx - 0.001f);
    int y0 = (int)floorf(miny), y1 = (int)floorf(maxy - 0.001f);
    int z0 = (int)floorf(minz), z1 = (int)floorf(maxz - 0.001f);
    for (int y = y0; y <= y1; y++) {
        for (int z = z0; z <= z1; z++) {
            for (int x = x0; x <= x1; x++) {
                if (!flat_in_bounds(x, y, z)) return 0;
                int id = flat_get_block(x, y, z);
                if (id == BLOCK_PORTAL || id == BLOCK_END_PORTAL) continue;
                if (flat_block_is_solid(id) || block_has_custom_collision(id)) return 0;
            }
        }
    }
    return 1;
}

static int dimension_portal_interior_block(int base_x, int base_y, int base_z, int axis,
                                           int a, int dy, int *x, int *y, int *z) {
    *y = base_y + dy;
    if (axis == 0) {
        *x = base_x + a;
        *z = base_z;
    } else {
        *x = base_x;
        *z = base_z + a;
    }
    return flat_in_bounds(*x, *y, *z);
}

static int dimension_portal_spot_ok(int base_x, int base_y, int base_z, int axis) {
    int top = dimension_portal_world_top();
    if (base_y < FLAT_WORLD_Y_MIN + 2 || base_y + 3 > top) return 0;

    /* The 2x3 portal interior must be clear/replaceable. */
    for (int a = 1; a <= 2; a++) {
        for (int dy = 0; dy < 3; dy++) {
            int x, y, z;
            if (!dimension_portal_interior_block(base_x, base_y, base_z, axis, a, dy, &x, &y, &z)) return 0;
            if (!dimension_portal_replaceable(flat_get_block(x, y, z))) return 0;
        }
    }

    /* Make sure the frame itself is in-bounds; the frame may replace blocks. */
    for (int a = 0; a <= 3; a++) {
        int x0 = base_x + (axis == 0 ? a : 0);
        int z0 = base_z + (axis == 1 ? a : 0);
        if (!flat_in_bounds(x0, base_y - 1, z0) || !flat_in_bounds(x0, base_y + 3, z0)) return 0;
    }
    for (int dy = 0; dy < 3; dy++) {
        int lx = base_x, lz = base_z;
        int rx = base_x + (axis == 0 ? 3 : 0);
        int rz = base_z + (axis == 1 ? 3 : 0);
        if (!flat_in_bounds(lx, base_y + dy, lz) || !flat_in_bounds(rx, base_y + dy, rz)) return 0;
    }

    float px, py, pz;
    dimension_portal_player_pos_for_frame(base_x, base_y, base_z, axis, &px, &py, &pz);
    if (!dimension_player_space_clear(px, py - PEX_PLAYER_EYE_HEIGHT, pz)) return 0;

    /* Leave one easy step-out space on either side of the portal plane when it
       is naturally available.  This is not strict vanilla logic, but it prevents
       the common PexCraft failure of creating a frame sealed inside rock. */
    if (axis == 0) {
        if (!flat_in_bounds(base_x + 1, base_y, base_z - 1) || !flat_in_bounds(base_x + 2, base_y + 1, base_z + 1)) return 0;
        for (int sx = base_x + 1; sx <= base_x + 2; sx++) {
            for (int side = -1; side <= 1; side += 2) {
                for (int dy = 0; dy < 2; dy++) {
                    int id = flat_get_block(sx, base_y + dy, base_z + side);
                    if (flat_block_is_solid(id) || block_has_custom_collision(id)) return 0;
                }
            }
        }
    } else {
        if (!flat_in_bounds(base_x - 1, base_y, base_z + 1) || !flat_in_bounds(base_x + 1, base_y + 1, base_z + 2)) return 0;
        for (int sz = base_z + 1; sz <= base_z + 2; sz++) {
            for (int side = -1; side <= 1; side += 2) {
                for (int dy = 0; dy < 2; dy++) {
                    int id = flat_get_block(base_x + side, base_y + dy, sz);
                    if (flat_block_is_solid(id) || block_has_custom_collision(id)) return 0;
                }
            }
        }
    }
    return 1;
}

static int dimension_find_portal_creation_spot(float tx, float ty, float tz, PexPortalPlacement *out) {
    int cx = (int)floorf(tx);
    int cz = (int)floorf(tz);
    int target_feet_y = (int)floorf(ty - PEX_PLAYER_EYE_HEIGHT);
    int top = dimension_portal_world_top() - 3;
    if (target_feet_y < FLAT_WORLD_Y_MIN + 2) target_feet_y = FLAT_WORLD_Y_MIN + 2;
    if (target_feet_y > top) target_feet_y = top;

    int preferred_axis = dimension_axis_from_yaw();
    int found = 0;
    PexPortalPlacement best;
    memset(&best, 0, sizeof(best));
    best.dist2 = 1.0e100;

    for (int dx = -PEX_NETHER_PORTAL_CREATE_RADIUS; dx <= PEX_NETHER_PORTAL_CREATE_RADIUS; dx++) {
        for (int dz = -PEX_NETHER_PORTAL_CREATE_RADIUS; dz <= PEX_NETHER_PORTAL_CREATE_RADIUS; dz++) {
            for (int pass = 0; pass < 2; pass++) {
                int axis = pass == 0 ? preferred_axis : 1 - preferred_axis;
                int base_x = cx + dx - (axis == 0 ? 2 : 0);
                int base_z = cz + dz - (axis == 1 ? 2 : 0);

                /* Search near the target height first, then the rest of the old world height. */
                for (int band = 0; band <= top; band++) {
                    int ys[2] = { target_feet_y - band, target_feet_y + band };
                    for (int yi = 0; yi < 2; yi++) {
                        int y = ys[yi];
                        if (yi == 1 && band == 0) continue;
                        if (y < FLAT_WORLD_Y_MIN + 2 || y > top) continue;
                        if (!dimension_portal_spot_ok(base_x, y, base_z, axis)) continue;

                        PexPortalPlacement cand;
                        memset(&cand, 0, sizeof(cand));
                        cand.axis = axis;
                        cand.base_x = base_x;
                        cand.base_y = y;
                        cand.base_z = base_z;
                        dimension_portal_player_pos_for_frame(base_x, y, base_z, axis,
                                                              &cand.player_x, &cand.player_y, &cand.player_z);
                        double ddx = (double)cand.player_x - (double)tx;
                        double ddy = ((double)cand.player_y - (double)PEX_PLAYER_EYE_HEIGHT) - ((double)ty - (double)PEX_PLAYER_EYE_HEIGHT);
                        double ddz = (double)cand.player_z - (double)tz;
                        cand.dist2 = ddx * ddx + ddy * ddy + ddz * ddz;
                        if (!found || cand.dist2 < best.dist2) {
                            best = cand;
                            found = 1;
                        }
                    }
                }
            }
        }
    }

    if (found && out) *out = best;
    return found;
}

static void dimension_clear_portal_volume(int base_x, int base_y, int base_z, int axis) {
    int x0 = base_x - 1, x1 = base_x + (axis == 0 ? 4 : 1);
    int z0 = base_z - 1, z1 = base_z + (axis == 1 ? 4 : 1);
    int y0 = base_y, y1 = base_y + 3;
    for (int y = y0; y <= y1; y++) {
        for (int z = z0; z <= z1; z++) {
            for (int x = x0; x <= x1; x++) {
                if (flat_in_bounds(x, y, z)) flat_set_block_raw(x, y, z, 0);
            }
        }
    }
}

/* ----------------------------------------------------------------
   Build a new Nether portal frame.  base_y is the bottom of the 2x3
   portal interior; the obsidian sill is base_y - 1.
   ---------------------------------------------------------------- */
static void dimension_create_nether_portal(int base_x, int base_y, int base_z, int axis) {
    int top = dimension_portal_world_top();
    if (base_y < FLAT_WORLD_Y_MIN + 2) base_y = FLAT_WORLD_Y_MIN + 2;
    if (base_y + 3 > top) base_y = top - 3;

    flat_begin_persistent_edit();

    dimension_clear_portal_volume(base_x, base_y, base_z, axis);

    /* Bottom/top sills. */
    for (int a = 0; a < 4; a++) {
        int x = base_x + (axis == 0 ? a : 0);
        int z = base_z + (axis == 1 ? a : 0);
        flat_set_block_raw(x, base_y - 1, z, BLOCK_OBSIDIAN);
        flat_set_block_raw(x, base_y + 3, z, BLOCK_OBSIDIAN);
    }

    /* Side rails. */
    for (int dy = 0; dy < 3; dy++) {
        flat_set_block_raw(base_x, base_y + dy, base_z, BLOCK_OBSIDIAN);
        flat_set_block_raw(base_x + (axis == 0 ? 3 : 0), base_y + dy,
                           base_z + (axis == 1 ? 3 : 0), BLOCK_OBSIDIAN);
    }

    /* Interior portal blocks. */
    for (int a = 1; a <= 2; a++) {
        for (int dy = 0; dy < 3; dy++) {
            int x = base_x + (axis == 0 ? a : 0);
            int z = base_z + (axis == 1 ? a : 0);
            flat_set_block_raw(x, base_y + dy, z, BLOCK_PORTAL);
        }
    }

    flat_end_persistent_edit();
    flat_mark_all_chunks_dirty();
    g_save_dirty = 1;
    pex_logf("dimension: created portal frame at %d,%d,%d axis=%d", base_x, base_y, base_z, axis);
}

static void dimension_apply_portal_destination(const PexPortalPlacement *pl) {
    g_player_x = pl->player_x;
    g_player_y = pl->player_y;
    g_player_z = pl->player_z;
    g_player_prev_x = g_player_x;
    g_player_prev_y = g_player_y;
    g_player_prev_z = g_player_z;
    g_player_motion_x = 0.0f;
    g_player_motion_y = 0.0f;
    g_player_motion_z = 0.0f;
    g_player_fall_distance = 0.0f;
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
        nx = g_player_x;
        ny = g_player_y;
        nz = g_player_z;
    }

    /* Switch dimension */
    g_current_dimension = target_dim;
    g_portal_timer = 0;
    g_time_in_portal = 0.0f;
    g_prev_time_in_portal = 0.0f;
    g_portal_cooldown = PEX_NETHER_PORTAL_COOLDOWN_TICKS;

    /* Teleport player to the scaled target before rebuilding/searching. */
    g_player_x = nx; g_player_y = ny; g_player_z = nz;
    g_player_prev_x = nx; g_player_prev_y = ny; g_player_prev_z = nz;
    g_player_motion_x = 0.0f;
    g_player_motion_y = 0.0f;
    g_player_motion_z = 0.0f;
    g_player_fall_distance = 0.0f;

    /* Rebuild only the target portal neighborhood before search/creation.
       Rebuilding and relighting the full 32x32 active window here could stall
       the gameplay thread for tens of seconds right after the portal effect. */
    flat_center_origin_near(g_player_x, g_player_z);
    if (target_dim == PEX_DIM_NETHER || target_dim == PEX_DIM_OVERWORLD) {
        flat_generate_portal_travel_blocks(g_player_x, g_player_z);
    } else {
        flat_generate_origin_blocks();
        flat_lighting_worker_wake();
    }

    /* For Nether/Overworld transitions: find or create the destination portal. */
    if (target_dim == PEX_DIM_NETHER || target_dim == PEX_DIM_OVERWORLD) {
        PexPortalPlacement dest;
        if (dimension_find_nether_portal(nx, ny, nz, &dest)) {
            dimension_apply_portal_destination(&dest);
        } else {
            if (!dimension_find_portal_creation_spot(nx, ny, nz, &dest)) {
                int top = dimension_portal_world_top() - 3;
                int base_y = (int)floorf(ny - PEX_PLAYER_EYE_HEIGHT);
                if (base_y < FLAT_WORLD_Y_MIN + 2) base_y = FLAT_WORLD_Y_MIN + 2;
                if (base_y > top) base_y = top;
                dest.axis = dimension_axis_from_yaw();
                dest.base_x = (int)floorf(nx) - (dest.axis == 0 ? 2 : 0);
                dest.base_y = base_y;
                dest.base_z = (int)floorf(nz) - (dest.axis == 1 ? 2 : 0);
                dimension_portal_player_pos_for_frame(dest.base_x, dest.base_y, dest.base_z, dest.axis,
                                                      &dest.player_x, &dest.player_y, &dest.player_z);
                dest.dist2 = 0.0;
            }
            dimension_create_nether_portal(dest.base_x, dest.base_y, dest.base_z, dest.axis);
            dimension_portal_player_pos_for_frame(dest.base_x, dest.base_y, dest.base_z, dest.axis,
                                                  &dest.player_x, &dest.player_y, &dest.player_z);
            dimension_apply_portal_destination(&dest);
        }
    }

    /* Light only the arrival neighborhood immediately; regular streaming/lighting
       can settle the rest after the player is already back in control. */
    if (target_dim == PEX_DIM_NETHER || target_dim == PEX_DIM_OVERWORLD) {
        flat_relight_chunks_near(g_player_x, g_player_z, 2);
        flat_lighting_worker_wake();
    }

    /* Java keeps a short portal cooldown after travel so a player standing in
       the destination portal cannot immediately fire the return trip. */
    g_portal_cooldown = PEX_NETHER_PORTAL_COOLDOWN_TICKS;

    pex_logf("dimension: arrived in dim=%d at %.1f,%.1f,%.1f",
             g_current_dimension,
             (double)g_player_x, (double)g_player_y, (double)g_player_z);
}

static int dimension_player_intersects_block_id(int wanted) {
    float feet = g_player_y - PEX_PLAYER_EYE_HEIGHT;
    float minx = g_player_x - 0.30f, maxx = g_player_x + 0.30f;
    float miny = feet + 0.001f, maxy = feet + 1.80f - 0.001f;
    float minz = g_player_z - 0.30f, maxz = g_player_z + 0.30f;
    int x0 = (int)floorf(minx), x1 = (int)floorf(maxx - 0.001f);
    int y0 = (int)floorf(miny), y1 = (int)floorf(maxy - 0.001f);
    int z0 = (int)floorf(minz), z1 = (int)floorf(maxz - 0.001f);

    for (int y = y0; y <= y1; y++) {
        for (int z = z0; z <= z1; z++) {
            for (int x = x0; x <= x1; x++) {
                if (flat_in_bounds(x, y, z) && flat_get_block(x, y, z) == wanted) return 1;
            }
        }
    }
    return 0;
}

/* ----------------------------------------------------------------
   Portal collision tick — called every game tick.
   Detects when the player intersects a BLOCK_PORTAL (Nether) or
   BLOCK_END_PORTAL (End) and triggers a dimension transition.
   ---------------------------------------------------------------- */
static void dimension_tick_portal_collision(void) {
    g_prev_time_in_portal = g_time_in_portal;
    if (g_mp_connected) return;
    if (g_player_dead) return;

    int in_nether_portal = dimension_player_intersects_block_id(BLOCK_PORTAL);
    int in_end_portal = dimension_player_intersects_block_id(BLOCK_END_PORTAL);

    if (in_end_portal) {
        /* End portal: instant teleport */
        g_portal_timer = 0;
        g_time_in_portal = 0.0f;
        g_prev_time_in_portal = 0.0f;
        if (g_current_dimension == PEX_DIM_OVERWORLD)
            dimension_teleport_player(PEX_DIM_END);
        else if (g_current_dimension == PEX_DIM_END)
            dimension_teleport_player(PEX_DIM_OVERWORLD);
        return;
    }

    if (g_portal_cooldown > 0) {
        if (in_nether_portal) {
            /* While the player remains in the destination frame, keep the
               cooldown fresh and do not build timeInPortal. */
            g_portal_cooldown = PEX_NETHER_PORTAL_COOLDOWN_TICKS;
            g_portal_timer = 0;
            if (g_time_in_portal > 0.0f) {
                g_time_in_portal -= 0.05f;
                if (g_time_in_portal < 0.0f) g_time_in_portal = 0.0f;
            }
            return;
        }
        g_portal_cooldown--;
    }

    if (in_nether_portal) {
        if (g_time_in_portal <= 0.0f) {
            pex_sound_play("portal.trigger", 1.0f, 0.8f + frand01() * 0.4f);
        }
        g_time_in_portal += 0.0125f; /* EntityPlayerSP-style 80 ticks from 0.0 to 1.0 */
        if (g_time_in_portal > 1.0f) g_time_in_portal = 1.0f;
        g_portal_timer++;
        if (g_portal_timer >= 80) { /* 4 seconds at 20 ticks/sec */
            pex_sound_play("portal.travel", 1.0f, 0.8f + frand01() * 0.4f);
            if (g_current_dimension == PEX_DIM_OVERWORLD)
                dimension_teleport_player(PEX_DIM_NETHER);
            else
                dimension_teleport_player(PEX_DIM_OVERWORLD);
            g_portal_timer = 0;
            g_time_in_portal = 0.0f;
            g_prev_time_in_portal = 0.0f;
        }
    } else {
        if (g_portal_timer > 0) g_portal_timer--;
        if (g_time_in_portal > 0.0f) {
            g_time_in_portal -= 0.05f;
            if (g_time_in_portal < 0.0f) g_time_in_portal = 0.0f;
        }
    }
}
