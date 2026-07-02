/* PexCraft block placement hooks: portal creation logic.
   Included by src/main.c (unity build) after game/inventory.c. */

/* ----------------------------------------------------------------
   Nether portal creation.
   Port of BlockPortal.tryToCreatePortal() from Minecraft JE 1.2.5.
   Called when BLOCK_FIRE is placed at (bx, by, bz).
   ---------------------------------------------------------------- */
static int try_create_nether_portal(int bx, int by, int bz) {
    /* Determine orientation from adjacent obsidian.
       dx/dz = which horizontal axis the portal frame runs along. */
    int dx = 0, dz = 0;
    if (flat_get_block(bx - 1, by, bz) == BLOCK_OBSIDIAN ||
        flat_get_block(bx + 1, by, bz) == BLOCK_OBSIDIAN) {
        dx = 1; dz = 0; /* portal runs E-W, obsidian on X sides */
    } else if (flat_get_block(bx, by, bz - 1) == BLOCK_OBSIDIAN ||
               flat_get_block(bx, by, bz + 1) == BLOCK_OBSIDIAN) {
        dx = 0; dz = 1; /* portal runs N-S, obsidian on Z sides */
    } else {
        return 0; /* no obsidian neighbours */
    }

    /* Scan downward to find the bottom obsidian sill */
    int cy = by;
    while (cy > 0 && flat_get_block(bx, cy - 1, bz) != BLOCK_OBSIDIAN) cy--;
    if (!flat_in_bounds(bx, cy - 1, bz) || flat_get_block(bx, cy - 1, bz) != BLOCK_OBSIDIAN)
        return 0;

    /* Scan left in portal axis to find the left obsidian wall */
    int cx = bx, cz = bz;
    while (flat_in_bounds(cx - dx, cy, cz - dz) &&
           flat_get_block(cx - dx, cy, cz - dz) != BLOCK_OBSIDIAN) {
        cx -= dx; cz -= dz;
    }
    if (!flat_in_bounds(cx - dx, cy, cz - dz) ||
        flat_get_block(cx - dx, cy, cz - dz) != BLOCK_OBSIDIAN)
        return 0;

    /* Validate the 4-wide x 5-tall frame.
       da in [-1..2]: -1 and 2 are the obsidian side walls (4 wide = 2 interior + 2 walls)
       db in [-1..3]: -1 is the floor sill, 3 is the top sill (5 tall = 3 interior + 2 caps)
       Corners (both da and db at extremes) are skipped per Java source. */
    for (int da = -1; da <= 2; da++) {
        for (int db = -1; db <= 3; db++) {
            /* Skip the four literal corners of the frame check */
            if ((da == -1 || da == 2) && (db == -1 || db == 3)) continue;
            int wx = cx + dx * da;
            int wy = cy + db;
            int wz = cz + dz * da;
            if (!flat_in_bounds(wx, wy, wz)) return 0;
            int is_frame = (da == -1 || da == 2 || db == -1 || db == 3);
            int bid = flat_get_block(wx, wy, wz);
            if (is_frame) {
                if (bid != BLOCK_OBSIDIAN) return 0;
            } else {
                if (bid != 0 && bid != BLOCK_FIRE) return 0;
            }
        }
    }

    /* All checks passed: fill the 2x3 interior with BLOCK_PORTAL */
    flat_begin_persistent_edit();
    for (int da = 0; da <= 1; da++) {
        for (int db = 0; db <= 2; db++) {
            flat_set_block_raw(cx + dx * da, cy + db, cz + dz * da, BLOCK_PORTAL);
        }
    }
    flat_end_persistent_edit();
    g_save_dirty = 1;
    pex_sound_play_at("portal.portal",
        (float)cx + 0.5f, (float)(cy + 1), (float)cz + 0.5f, 0.5f, 1.0f);
    pex_logf("block_logic: nether portal created at %d,%d,%d orientation dx=%d dz=%d",
             cx, cy, cz, dx, dz);
    return 1;
}

/* ----------------------------------------------------------------
   End portal activation.
   Port of BlockEndPortalFrame activation logic from JE 1.2.5.
   Called after an Eye of Ender is inserted into a frame at (fx, fy, fz).
   The 12-block ring surrounds a 3x3 center at the same Y level.
   Frame offsets (relative to NW outer corner of the 5x5 bounding box):
     North row: [1,0],[2,0],[3,0]
     South row: [1,4],[2,4],[3,4]
     West col:  [0,1],[0,2],[0,3]
     East col:  [4,1],[4,2],[4,3]
   ---------------------------------------------------------------- */
static int check_end_portal_complete(int fx, int fy, int fz) {
    static const int ring[12][2] = {
        {1,0},{2,0},{3,0},
        {4,1},{4,2},{4,3},
        {3,4},{2,4},{1,4},
        {0,3},{0,2},{0,1}
    };

    /* Try all possible NW corners where (fx, fz) could be one of the 12 ring slots */
    for (int ox = fx - 4; ox <= fx; ox++) {
        for (int oz = fz - 4; oz <= fz; oz++) {
            int all_ok = 1;
            for (int i = 0; i < 12; i++) {
                int tx = ox + ring[i][0];
                int tz = oz + ring[i][1];
                if (!flat_in_bounds(tx, fy, tz)) { all_ok = 0; break; }
                if (flat_get_block(tx, fy, tz) != BLOCK_END_PORTAL_FRAME) { all_ok = 0; break; }
                if (!(flat_get_meta(tx, fy, tz) & 0x4)) { all_ok = 0; break; }
            }
            if (!all_ok) continue;

            /* All 12 frames have eyes — fill the 3x3 center with END_PORTAL */
            flat_begin_persistent_edit();
            for (int dx = 1; dx <= 3; dx++) {
                for (int dz = 1; dz <= 3; dz++) {
                    int tx = ox + dx, tz = oz + dz;
                    if (flat_in_bounds(tx, fy, tz) &&
                        flat_get_block(tx, fy, tz) != BLOCK_END_PORTAL_FRAME) {
                        flat_set_block_raw(tx, fy, tz, BLOCK_END_PORTAL);
                    }
                }
            }
            flat_end_persistent_edit();
            g_save_dirty = 1;
            pex_sound_play_at("mob.endermen.portal",
                (float)(ox + 2) + 0.5f, (float)fy + 0.5f, (float)(oz + 2) + 0.5f,
                1.0f, 1.0f);
            pex_logf("block_logic: end portal activated at origin %d,%d,%d", ox, fy, oz);
            return 1;
        }
    }
    return 0;
}

/* ----------------------------------------------------------------
   Generic block-placed hook.
   Call this after any block is placed by player or game logic.
   ---------------------------------------------------------------- */
static void block_on_placed(int x, int y, int z, int id) {
    if (id == BLOCK_FIRE) {
        try_create_nether_portal(x, y, z);
    }
}
