/* Split from original monolithic main.c. Included by src/main.c unity build. */

static int item_icon_tile(int id) {
    /* gui/items.png tile numbers match Java 1.2.5 Item.setIconCoord(x,y). */
    switch (id) {
        case ITEM_SHOVEL_IRON: return 82;
        case ITEM_PICKAXE_IRON: return 98;
        case ITEM_AXE_IRON: return 114;
        case ITEM_FLINT_AND_IRON: return 5;
        case ITEM_APPLE_RED: return 10;
        case ITEM_BOW: return 21;
        case ITEM_ARROW: return 37;
        case ITEM_COAL: return 7;
        case ITEM_DIAMOND: return 55;
        case ITEM_INGOT_IRON: return 23;
        case ITEM_INGOT_GOLD: return 39;
        case ITEM_SWORD_IRON: return 66;
        case ITEM_WOODEN_SWORD: return 64;
        case ITEM_WOODEN_SHOVEL: return 80;
        case ITEM_WOODEN_PICKAXE: return 96;
        case ITEM_WOODEN_AXE: return 112;
        case ITEM_STONE_SWORD: return 65;
        case ITEM_STONE_SHOVEL: return 81;
        case ITEM_STONE_PICKAXE: return 97;
        case ITEM_STONE_AXE: return 113;
        case ITEM_SWORD_DIAMOND: return 67;
        case ITEM_SHOVEL_DIAMOND: return 83;
        case ITEM_PICKAXE_DIAMOND: return 99;
        case ITEM_AXE_DIAMOND: return 115;
        case ITEM_STICK: return 53;
        case ITEM_BOWL_EMPTY: return 71;
        case ITEM_BOWL_SOUP: return 72;
        case ITEM_SWORD_GOLD: return 68;
        case ITEM_SHOVEL_GOLD: return 84;
        case ITEM_PICKAXE_GOLD: return 100;
        case ITEM_AXE_GOLD: return 116;
        case ITEM_STRING: return 8;
        case ITEM_FEATHER: return 24;
        case ITEM_GUNPOWDER: return 40;
        case ITEM_HOE_WOOD: return 128;
        case ITEM_HOE_STONE: return 129;
        case ITEM_HOE_IRON: return 130;
        case ITEM_HOE_DIAMOND: return 131;
        case ITEM_HOE_GOLD: return 132;
        case ITEM_SEEDS: return 9;
        case ITEM_WHEAT: return 25;
        case ITEM_BREAD: return 41;
        case ITEM_HELMET_LEATHER: return 0;
        case ITEM_PLATE_LEATHER: return 16;
        case ITEM_LEGS_LEATHER: return 32;
        case ITEM_BOOTS_LEATHER: return 48;
        case ITEM_HELMET_CHAIN: return 1;
        case ITEM_PLATE_CHAIN: return 17;
        case ITEM_LEGS_CHAIN: return 33;
        case ITEM_BOOTS_CHAIN: return 49;
        case ITEM_HELMET_IRON: return 2;
        case ITEM_PLATE_IRON: return 18;
        case ITEM_LEGS_IRON: return 34;
        case ITEM_BOOTS_IRON: return 50;
        case ITEM_HELMET_DIAMOND: return 3;
        case ITEM_PLATE_DIAMOND: return 19;
        case ITEM_LEGS_DIAMOND: return 35;
        case ITEM_BOOTS_DIAMOND: return 51;
        case ITEM_HELMET_GOLD: return 4;
        case ITEM_PLATE_GOLD: return 20;
        case ITEM_LEGS_GOLD: return 36;
        case ITEM_BOOTS_GOLD: return 52;
        case ITEM_FLINT: return 6;
        case ITEM_PORK_RAW: return 87;
        case ITEM_PORK_COOKED: return 88;
        case ITEM_PAINTING: return 26;
        case ITEM_APPLE_GOLD: return 11;
        case ITEM_SIGN: return 42;
        case ITEM_DOOR_WOOD: return 43;
        case ITEM_BUCKET_EMPTY: return 74;
        case ITEM_BUCKET_WATER: return 75;
        case ITEM_BUCKET_LAVA: return 76;
        case ITEM_MINECART_EMPTY: return 135;
        case ITEM_SADDLE: return 104;
        case ITEM_DOOR_IRON: return 44;
        case ITEM_REDSTONE: return 56;
        case ITEM_SNOWBALL: return 14;
        case ITEM_BOAT: return 136;
        case ITEM_LEATHER: return 103;
        case ITEM_BUCKET_MILK: return 77;
        case ITEM_BRICK: return 22;
        case ITEM_CLAY: return 57;
        case ITEM_REED: return 27;
        case ITEM_PAPER: return 58;
        case ITEM_BOOK: return 59;
        case ITEM_SLIME_BALL: return 30;
        case ITEM_MINECART_CRATE: return 151;
        case ITEM_MINECART_POWERED: return 167;
        case ITEM_EGG: return 12;
        case ITEM_COMPASS: return 54;
        case ITEM_FISHING_ROD: return 69;
        case ITEM_CLOCK: return 70;
        case ITEM_GLOWSTONE_DUST: return 73;
        case ITEM_FISH_RAW: return 89;
        case ITEM_FISH_COOKED: return 90;
        case ITEM_DYE_POWDER: return 78;
        case ITEM_BONE: return 28;
        case ITEM_SUGAR: return 13;
        case ITEM_CAKE: return 29;
        case ITEM_BED: return 45;
        case ITEM_REDSTONE_REPEATER: return 86;
        case ITEM_COOKIE: return 92;
        case ITEM_MAP: return 60;
        case ITEM_SHEARS: return 93;
        case ITEM_MELON: return 109;
        case ITEM_PUMPKIN_SEEDS: return 61;
        case ITEM_MELON_SEEDS: return 62;
        case ITEM_BEEF_RAW: return 105;
        case ITEM_BEEF_COOKED: return 106;
        case ITEM_CHICKEN_RAW: return 121;
        case ITEM_CHICKEN_COOKED: return 122;
        case ITEM_ROTTEN_FLESH: return 91;
        case ITEM_ENDER_PEARL: return 107;
        case ITEM_BLAZE_ROD: return 108;
        case ITEM_GHAST_TEAR: return 123;
        case ITEM_GOLD_NUGGET: return 124;
        case ITEM_NETHER_WART: return 125;
        case ITEM_POTION: return 141;
        case ITEM_GLASS_BOTTLE: return 140;
        case ITEM_SPIDER_EYE: return 139;
        case ITEM_FERMENTED_SPIDER_EYE: return 138;
        case ITEM_BLAZE_POWDER: return 157;
        case ITEM_MAGMA_CREAM: return 173;
        case ITEM_BREWING_STAND: return 172;
        case ITEM_CAULDRON: return 156;
        case ITEM_EYE_OF_ENDER: return 155;
        case ITEM_SPECKLED_MELON: return 137;
        case ITEM_MONSTER_PLACER: return 153;
        case ITEM_EXP_BOTTLE: return 171;
        case ITEM_FIREBALL_CHARGE: return 46;
        case ITEM_RECORD13: return 240;
        case ITEM_RECORD_CAT: return 241;
        case ITEM_RECORD_BLOCKS: return 242;
        case ITEM_RECORD_CHIRP: return 243;
        case ITEM_RECORD_FAR: return 244;
        case ITEM_RECORD_MALL: return 245;
        case ITEM_RECORD_MELLOHI: return 246;
        case ITEM_RECORD_STAL: return 247;
        case ITEM_RECORD_STRAD: return 248;
        case ITEM_RECORD_WARD: return 249;
        case ITEM_RECORD_11: return 250;
        default: return 0;
    }
}

static int item_is_block_id(int id) {
    return (id >= 1 && id <= BLOCK_REDSTONE_LAMP_ON);
}

static int item_max_damage(int id) {
    if (id == ITEM_WOODEN_SHOVEL || id == ITEM_SHOVEL_GOLD) return 33;
    if (id == ITEM_STONE_SHOVEL) return 65;
    if (id == ITEM_SHOVEL_IRON) return 251;
    if (id == ITEM_SHOVEL_DIAMOND) return 1562;

    if (id == ITEM_WOODEN_PICKAXE || id == ITEM_WOODEN_AXE ||
        id == ITEM_WOODEN_SWORD || id == ITEM_HOE_WOOD) return 60;
    if (id == ITEM_STONE_PICKAXE || id == ITEM_STONE_AXE ||
        id == ITEM_STONE_SWORD || id == ITEM_HOE_STONE) return 132;
    if (id == ITEM_PICKAXE_IRON || id == ITEM_AXE_IRON ||
        id == ITEM_SWORD_IRON || id == ITEM_HOE_IRON) return 251;
    if (id == ITEM_PICKAXE_DIAMOND || id == ITEM_AXE_DIAMOND ||
        id == ITEM_SWORD_DIAMOND || id == ITEM_HOE_DIAMOND) return 1562;
    if (id == ITEM_PICKAXE_GOLD || id == ITEM_AXE_GOLD ||
        id == ITEM_SWORD_GOLD || id == ITEM_HOE_GOLD) return 33;

    if (id == ITEM_BOW || id == ITEM_FISHING_ROD) return 385;
    if (id == ITEM_FLINT_AND_IRON) return 65;
    if (id == ITEM_SHEARS) return 238;

    int armor_max = armor_stack_max_damage(id);
    return armor_max > 0 ? armor_max : 0;
}

static int block_item_is_3d(int id) {
    if (!item_is_block_id(id)) return 0;
    /* Keep the Java 3-D inventory path for normal cube-like blocks, but also
       force chests down the model renderer so the inventory icon matches the
       in-world chest silhouette instead of falling back to a flat sprite. */
    if (block_is_liquid(id) ||
        id == BLOCK_SAPLING || id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE ||
        id == BLOCK_BROWN_MUSHROOM || id == BLOCK_RED_MUSHROOM || id == BLOCK_TORCH ||
        id == BLOCK_FIRE || id == BLOCK_REDSTONE_WIRE || id == BLOCK_CROPS ||
        id == BLOCK_SIGN_POST || id == BLOCK_WALL_SIGN || id == BLOCK_WOOD_DOOR ||
        id == BLOCK_IRON_DOOR || id == BLOCK_LADDER || id == BLOCK_RAILS ||
        id == BLOCK_POWERED_RAIL || id == BLOCK_DETECTOR_RAIL ||
        id == BLOCK_LEVER || id == BLOCK_REEDS || id == BLOCK_STONE_BUTTON ||
        id == BLOCK_STONE_PRESSURE_PLATE || id == BLOCK_WOOD_PRESSURE_PLATE ||
        id == BLOCK_TALL_GRASS || id == BLOCK_DEAD_BUSH || id == BLOCK_VINE ||
        id == BLOCK_LILY_PAD || id == BLOCK_NETHER_WART || id == BLOCK_PUMPKIN_STEM ||
        id == BLOCK_MELON_STEM || id == BLOCK_REDSTONE_REPEATER_OFF ||
        id == BLOCK_REDSTONE_REPEATER_ON || id == BLOCK_END_PORTAL) return 0;
    return 1;
}

static void draw_item_icon_gui_2d(const ItemStack *st, int x, int y) {
    if (stack_empty(st) || !tex_items.id) return;
    int tile = item_icon_tile(st->id);
    int sx = (tile & 15) * 16;
    int sy = (tile >> 4) * 16;
    glColorMask(1,1,1,1);
    glColor4f(1,1,1,1);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw_textured_rect_tex(&tex_items, x, y, sx, sy, 16, 16, 0xFFFFFF);
}


static void emit_inventory_cuboid_face(int id,
                                                 float bx, float by, float bz,
                                                 float x0, float y0, float z0,
                                                 float x1, float y1, float z1,
                                                 int face) {
    int tile = 2;
    float u0, v0, u1, v1;
    float ax0 = bx + x0, ay0 = by + y0, az0 = bz + z0;
    float ax1 = bx + x1, ay1 = by + y1, az1 = bz + z1;

    world_style_set_pos(0, 0, 0);
    world_face_style(id, face, &tile);

    if (face == 0 || face == 1) {
        terrain_tile_uv_subrect(tile, x0, z0, x1, z1, &u0, &v0, &u1, &v1);
    } else if (face == 2 || face == 3) {
        terrain_tile_uv_subrect(tile, x0, 1.0f - y1, x1, 1.0f - y0, &u0, &v0, &u1, &v1);
    } else {
        terrain_tile_uv_subrect(tile, z0, 1.0f - y1, z1, 1.0f - y0, &u0, &v0, &u1, &v1);
    }

    if (face == 0) {
        world_tex_vertex(ax0, ay0, az1, u0, v1);
        world_tex_vertex(ax0, ay0, az0, u0, v0);
        world_tex_vertex(ax1, ay0, az0, u1, v0);
        world_tex_vertex(ax1, ay0, az1, u1, v1);
    } else if (face == 1) {
        world_tex_vertex(ax1, ay1, az1, u1, v1);
        world_tex_vertex(ax1, ay1, az0, u1, v0);
        world_tex_vertex(ax0, ay1, az0, u0, v0);
        world_tex_vertex(ax0, ay1, az1, u0, v1);
    } else if (face == 2) {
        world_tex_vertex(ax0, ay1, az0, u1, v0);
        world_tex_vertex(ax1, ay1, az0, u0, v0);
        world_tex_vertex(ax1, ay0, az0, u0, v1);
        world_tex_vertex(ax0, ay0, az0, u1, v1);
    } else if (face == 3) {
        world_tex_vertex(ax0, ay1, az1, u0, v0);
        world_tex_vertex(ax0, ay0, az1, u0, v1);
        world_tex_vertex(ax1, ay0, az1, u1, v1);
        world_tex_vertex(ax1, ay1, az1, u1, v0);
    } else if (face == 4) {
        world_tex_vertex(ax0, ay1, az1, u1, v0);
        world_tex_vertex(ax0, ay1, az0, u0, v0);
        world_tex_vertex(ax0, ay0, az0, u0, v1);
        world_tex_vertex(ax0, ay0, az1, u1, v1);
    } else if (face == 5) {
        world_tex_vertex(ax1, ay0, az1, u0, v1);
        world_tex_vertex(ax1, ay0, az0, u1, v1);
        world_tex_vertex(ax1, ay1, az0, u1, v0);
        world_tex_vertex(ax1, ay1, az1, u0, v0);
    }
}

static void emit_inventory_cuboid_faces(int id,
                                                  float bx, float by, float bz,
                                                  float x0, float y0, float z0,
                                                  float x1, float y1, float z1) {
    for (int face = 0; face < 6; face++) {
        emit_inventory_cuboid_face(id, bx, by, bz, x0, y0, z0, x1, y1, z1, face);
    }
}

static void draw_inventory_cuboid_model(int id,
                                                   float bx, float by, float bz,
                                                   float x0, float y0, float z0,
                                                   float x1, float y1, float z1) {
    glBegin(GL_QUADS);
    emit_inventory_cuboid_faces(id, bx, by, bz, x0, y0, z0, x1, y1, z1);
    glEnd();
}

static void draw_inventory_block_model(int id) {
    /* Match Java RenderBlocks.func_1227_a exactly: after RenderItem applies
       its slot transform, RenderBlocks shifts the 0..1 block model by -0.5 on
       every axis before emitting faces.  Inventory cuboids use the same face
       winding/order as the deobfuscated Java source so only the correct faces
       are visible. */
    const float bx = -0.5f, by = -0.5f, bz = -0.5f;
    if (id == BLOCK_SLAB) { draw_inventory_cuboid_model(id, bx,by,bz, 0,0,0,1,0.5f,1); return; }
    if (id == BLOCK_SNOW_LAYER) { draw_inventory_cuboid_model(id, bx,by,bz, 0,0,0,1,0.125f,1); return; }
    if (id == BLOCK_STONE_PRESSURE_PLATE || id == BLOCK_WOOD_PRESSURE_PLATE) { draw_inventory_cuboid_model(id, bx,by,bz, 0.0625f,0,0.0625f,0.9375f,0.0625f,0.9375f); return; }
    if (id == BLOCK_STONE_BUTTON) { draw_inventory_cuboid_model(id, bx,by,bz, 0.3125f,0.375f,0.375f,0.6875f,0.625f,0.5000f); return; }
    if (id == BLOCK_CACTUS) { draw_inventory_cuboid_model(id, bx,by,bz, 0.0625f,0,0.0625f,0.9375f,1,0.9375f); return; }
    if (id == BLOCK_FENCE) {
        glBegin(GL_QUADS);
        emit_inventory_cuboid_faces(id, bx,by,bz, 0.375f,0.0f,0.375f,0.625f,1.0f,0.625f);
        emit_inventory_cuboid_faces(id, bx,by,bz, 0.0f,0.35f,0.4375f,1.0f,0.55f,0.5625f);
        emit_inventory_cuboid_faces(id, bx,by,bz, 0.0f,0.70f,0.4375f,1.0f,0.90f,0.5625f);
        glEnd();
        return;
    }
    if (id == BLOCK_WOOD_STAIRS || id == BLOCK_COBBLE_STAIRS) {
        glBegin(GL_QUADS);
        emit_inventory_cuboid_faces(id, bx,by,bz, 0,0,0,1,0.5f,1);
        emit_inventory_cuboid_faces(id, bx,by,bz, 0.5f,0.5f,0,1,1,1);
        glEnd();
        return;
    }
    if (id == BLOCK_CHEST) { draw_chest_block(bx,by,bz); return; }
    draw_inventory_cuboid_model(id, bx, by, bz, 0,0,0,1,1,1);
}

static void inventory_iso_vertex(int slot_x, int slot_y, float px, float py, float pz, float u, float v) {
    /* Java GUI block transform collapsed to 2-D screen coordinates:
       T(slot-2,+3) * S(10) * T(1,0.5,8) * Rx(210) * Ry(45).
       This keeps inventory block icons visually 1:1 with the deobfuscated Java
       path without letting old depth/cull state leak wrong cube faces into GUI. */
    const float c45 = 0.70710678118f;
    const float cx = -0.86602540378f;
    const float sx = -0.5f;
    float rx = c45 * px + c45 * pz;
    float rz = -c45 * px + c45 * pz;
    float ry = cx * py - sx * rz;
    float gx = (float)(slot_x - 2) + 10.0f * (1.0f + rx);
    float gy = (float)(slot_y + 3) + 10.0f * (0.5f + ry);
    glTexCoord2f(u, v);
    glVertex3f(gx, gy, 0.0f);
}

static void emit_inventory_iso_face(int id, int slot_x, int slot_y,
                                              float x0, float y0, float z0,
                                              float x1, float y1, float z1,
                                              int face) {
    int tile = 2;
    float u0, v0, u1, v1;
    world_style_set_pos(0, 0, 0);
    world_face_style(id, face, &tile);

    if (face == 0 || face == 1) {
        terrain_tile_uv_subrect(tile, x0, z0, x1, z1, &u0, &v0, &u1, &v1);
    } else if (face == 2 || face == 3) {
        terrain_tile_uv_subrect(tile, x0, 1.0f - y1, x1, 1.0f - y0, &u0, &v0, &u1, &v1);
    } else {
        terrain_tile_uv_subrect(tile, z0, 1.0f - y1, z1, 1.0f - y0, &u0, &v0, &u1, &v1);
    }

    float ax0 = x0 - 0.5f, ay0 = y0 - 0.5f, az0 = z0 - 0.5f;
    float ax1 = x1 - 0.5f, ay1 = y1 - 0.5f, az1 = z1 - 0.5f;

    if (face == 1) {
        inventory_iso_vertex(slot_x, slot_y, ax1, ay1, az1, u1, v1);
        inventory_iso_vertex(slot_x, slot_y, ax1, ay1, az0, u1, v0);
        inventory_iso_vertex(slot_x, slot_y, ax0, ay1, az0, u0, v0);
        inventory_iso_vertex(slot_x, slot_y, ax0, ay1, az1, u0, v1);
    } else if (face == 3) {
        inventory_iso_vertex(slot_x, slot_y, ax0, ay1, az1, u0, v0);
        inventory_iso_vertex(slot_x, slot_y, ax0, ay0, az1, u0, v1);
        inventory_iso_vertex(slot_x, slot_y, ax1, ay0, az1, u1, v1);
        inventory_iso_vertex(slot_x, slot_y, ax1, ay1, az1, u1, v0);
    } else if (face == 4) {
        inventory_iso_vertex(slot_x, slot_y, ax0, ay1, az1, u1, v0);
        inventory_iso_vertex(slot_x, slot_y, ax0, ay1, az0, u0, v0);
        inventory_iso_vertex(slot_x, slot_y, ax0, ay0, az0, u0, v1);
        inventory_iso_vertex(slot_x, slot_y, ax0, ay0, az1, u1, v1);
    }
}

static void draw_inventory_iso_cuboid(int id, int slot_x, int slot_y,
                                                float x0, float y0, float z0,
                                                float x1, float y1, float z1) {
    glBegin(GL_QUADS);
    /* At Java's inventory rotation the visible faces are x-min, z-max, and top.
       Emit only those faces, in painter order, so no back/inside face can bleed
       through on PSP/D3D compatibility paths. */
    emit_inventory_iso_face(id, slot_x, slot_y, x0, y0, z0, x1, y1, z1, 4);
    emit_inventory_iso_face(id, slot_x, slot_y, x0, y0, z0, x1, y1, z1, 3);
    emit_inventory_iso_face(id, slot_x, slot_y, x0, y0, z0, x1, y1, z1, 1);
    glEnd();
}

static void draw_inventory_iso_block_model(int id, int slot_x, int slot_y) {
    if (id == BLOCK_SLAB) { draw_inventory_iso_cuboid(id, slot_x, slot_y, 0,0,0,1,0.5f,1); return; }
    if (id == BLOCK_SNOW_LAYER) { draw_inventory_iso_cuboid(id, slot_x, slot_y, 0,0,0,1,0.125f,1); return; }
    if (id == BLOCK_CACTUS) { draw_inventory_iso_cuboid(id, slot_x, slot_y, 0.0625f,0,0.0625f,0.9375f,1,0.9375f); return; }
    if (id == BLOCK_CHEST) { draw_inventory_iso_cuboid(id, slot_x, slot_y, 0.0625f,0,0.0625f,0.9375f,14.0f/16.0f,0.9375f); return; }
    if (id == BLOCK_FENCE) {
        draw_inventory_iso_cuboid(id, slot_x, slot_y, 0.375f,0.0f,0.375f,0.625f,1.0f,0.625f);
        draw_inventory_iso_cuboid(id, slot_x, slot_y, 0.0f,0.35f,0.4375f,1.0f,0.55f,0.5625f);
        draw_inventory_iso_cuboid(id, slot_x, slot_y, 0.0f,0.70f,0.4375f,1.0f,0.90f,0.5625f);
        return;
    }
    if (id == BLOCK_WOOD_STAIRS || id == BLOCK_COBBLE_STAIRS) {
        draw_inventory_iso_cuboid(id, slot_x, slot_y, 0,0,0,1,0.5f,1);
        draw_inventory_iso_cuboid(id, slot_x, slot_y, 0.5f,0.5f,0,1,1,1);
        return;
    }
    draw_inventory_iso_cuboid(id, slot_x, slot_y, 0,0,0,1,1,1);
}

static void draw_block_item_gui_3d(const ItemStack *st, int x, int y) {
    if (stack_empty(st) || !tex_terrain.id) return;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glColorMask(1,1,1,1);
    glColor4f(1,1,1,1);

    int cutout_item = (st->id == BLOCK_GLASS || st->id == BLOCK_LEAVES || st->id == BLOCK_ICE);
    if (cutout_item) {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.5f);
        glDisable(GL_BLEND);
    } else {
        glDisable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    g_force_fullbright_item_model++;
    /* Use the explicit 2-D Java-projected inventory path for all GUI block
       models, including chests.  The matrix-rendered chest path clipped/overdrew
       its top and front faces in the GUI, leaving only the side faces visible. */
    draw_inventory_iso_block_model(st->id, x, y);
    g_force_fullbright_item_model--;

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1,1,1,1);
}



static int item_terrain_tile_has_pixels(int tile) {
    if (!tex_terrain.rgba || tex_terrain.w <= 0 || tex_terrain.h <= 0) return 0;
    int tx = (tile & 15) * 16;
    int ty = (tile >> 4) * 16;
    if (tx + 16 > tex_terrain.w || ty + 16 > tex_terrain.h) return 0;
    int count = 0;
    for (int yy = 0; yy < 16; yy++) {
        for (int xx = 0; xx < 16; xx++) {
            unsigned char *p = &tex_terrain.rgba[((ty + yy) * tex_terrain.w + (tx + xx)) * 4];
            if (p[3] > 8) count++;
        }
    }
    return count > 32;
}

static int item_chest_icon_tile(void) {
    if (strcmp(g_current_texpack, CLASSIC_PACK_NAME) == 0) return 27;
    return (item_terrain_tile_has_pixels(25) && item_terrain_tile_has_pixels(27)) ? 27 : 70;
}

static int item_water_tile(void) { return item_terrain_tile_has_pixels(205) ? 205 : 14; }
static int item_lava_tile(void) { return item_terrain_tile_has_pixels(237) ? 237 : 30; }

static int block_item_tile_for_id(int id) {
    /* Representative terrain.png tile for every Java 1.2.5 block ID.  Face- and
       metadata-specific rendering still happens in the 3-D item/world paths. */
    switch (id) {
        case BLOCK_STONE: return 1;
        case BLOCK_GRASS: return 0;
        case BLOCK_DIRT: return 2;
        case BLOCK_COBBLESTONE: return 16;
        case BLOCK_PLANKS: return 4;
        case BLOCK_SAPLING: return 15;
        case BLOCK_BEDROCK: return 17;
        case BLOCK_WATER:
        case BLOCK_STILL_WATER: return item_water_tile();
        case BLOCK_LAVA:
        case BLOCK_STILL_LAVA: return item_lava_tile();
        case BLOCK_SAND: return 18;
        case BLOCK_GRAVEL: return 19;
        case BLOCK_GOLD_ORE: return 32;
        case BLOCK_IRON_ORE: return 33;
        case BLOCK_COAL_ORE: return 34;
        case BLOCK_LOG: return 20;
        case BLOCK_LEAVES: return g_opts.fancy_graphics ? 52 : 53;
        case BLOCK_SPONGE: return 48;
        case BLOCK_GLASS: return 49;
        case BLOCK_LAPIS_ORE: return 160;
        case BLOCK_LAPIS_BLOCK: return 144;
        case BLOCK_DISPENSER: return 46;
        case BLOCK_SANDSTONE: return 192;
        case BLOCK_NOTE_BLOCK: return 75;
        case BLOCK_BED: return 134;
        case BLOCK_POWERED_RAIL: return 179;
        case BLOCK_DETECTOR_RAIL: return 195;
        case BLOCK_STICKY_PISTON: return 106;
        case BLOCK_WEB: return 11;
        case BLOCK_TALL_GRASS: return 39;
        case BLOCK_DEAD_BUSH: return 55;
        case BLOCK_PISTON: return 107;
        case BLOCK_PISTON_EXTENSION: return 107;
        case BLOCK_WOOL: return 113;
        case BLOCK_PISTON_MOVING: return 107;
        case BLOCK_YELLOW_FLOWER: return 13;
        case BLOCK_RED_ROSE: return 12;
        case BLOCK_BROWN_MUSHROOM: return 29;
        case BLOCK_RED_MUSHROOM: return 28;
        case BLOCK_GOLD_BLOCK: return 23;
        case BLOCK_IRON_BLOCK: return 22;
        case BLOCK_DOUBLE_SLAB:
        case BLOCK_SLAB: return 6;
        case BLOCK_BRICK: return 7;
        case BLOCK_TNT: return 8;
        case BLOCK_BOOKSHELF: return 35;
        case BLOCK_MOSSY_COBBLESTONE: return 36;
        case BLOCK_OBSIDIAN: return 37;
        case BLOCK_TORCH: return 80;
        case BLOCK_FIRE: return 31;
        case BLOCK_MOB_SPAWNER: return 65;
        case BLOCK_WOOD_STAIRS: return 4;
        case BLOCK_CHEST: return item_chest_icon_tile();
        case BLOCK_REDSTONE_WIRE: return 164;
        case BLOCK_DIAMOND_ORE: return 50;
        case BLOCK_DIAMOND_BLOCK: return 24;
        case BLOCK_CRAFTING_TABLE: return 59;
        case BLOCK_CROPS: return 88;
        case BLOCK_FARMLAND: return 87;
        case BLOCK_FURNACE: return furnace_front_tile();
        case BLOCK_FURNACE_LIT: return furnace_front_lit_tile();
        case BLOCK_SIGN_POST:
        case BLOCK_WALL_SIGN: return 4;
        case BLOCK_WOOD_DOOR: return 97;
        case BLOCK_LADDER: return 83;
        case BLOCK_RAILS: return 128;
        case BLOCK_COBBLE_STAIRS: return 16;
        case BLOCK_LEVER: return 96;
        case BLOCK_STONE_PRESSURE_PLATE: return 1;
        case BLOCK_IRON_DOOR: return 98;
        case BLOCK_WOOD_PRESSURE_PLATE: return 4;
        case BLOCK_REDSTONE_ORE:
        case BLOCK_REDSTONE_ORE_GLOWING: return 51;
        case BLOCK_REDSTONE_TORCH_OFF: return 115;
        case BLOCK_REDSTONE_TORCH_ON: return 99;
        case BLOCK_STONE_BUTTON: return 1;
        case BLOCK_SNOW_LAYER: return 66;
        case BLOCK_ICE: return 67;
        case BLOCK_SNOW_BLOCK: return 66;
        case BLOCK_CACTUS: return 70;
        case BLOCK_CLAY: return 72;
        case BLOCK_REEDS: return 73;
        case BLOCK_JUKEBOX: return 74;
        case BLOCK_FENCE: return 4;
        case BLOCK_PUMPKIN: return 118;
        case BLOCK_NETHERRACK: return 103;
        case BLOCK_SOUL_SAND: return 104;
        case BLOCK_GLOWSTONE: return 105;
        case BLOCK_PORTAL: return 14;
        case BLOCK_JACK_O_LANTERN: return 119;
        case BLOCK_CAKE: return 121;
        case BLOCK_REDSTONE_REPEATER_OFF: return 131;
        case BLOCK_REDSTONE_REPEATER_ON: return 147;
        case BLOCK_LOCKED_CHEST: return item_chest_icon_tile();
        case BLOCK_TRAPDOOR: return 84;
        case BLOCK_SILVERFISH: return 1;
        case BLOCK_STONE_BRICK: return 54;
        case BLOCK_BROWN_MUSHROOM_CAP:
        case BLOCK_RED_MUSHROOM_CAP: return 142;
        case BLOCK_IRON_BARS: return 85;
        case BLOCK_GLASS_PANE: return 49;
        case BLOCK_MELON: return 136;
        case BLOCK_PUMPKIN_STEM: return 19;
        case BLOCK_MELON_STEM: return 19;
        case BLOCK_VINE: return 143;
        case BLOCK_FENCE_GATE: return 4;
        case BLOCK_BRICK_STAIRS: return 7;
        case BLOCK_STONE_BRICK_STAIRS: return 54;
        case BLOCK_MYCELIUM: return 78;
        case BLOCK_LILY_PAD: return 76;
        case BLOCK_NETHER_BRICK:
        case BLOCK_NETHER_BRICK_FENCE:
        case BLOCK_NETHER_BRICK_STAIRS: return 224;
        case BLOCK_NETHER_WART: return 226;
        case BLOCK_ENCHANTMENT_TABLE: return 166;
        case BLOCK_BREWING_STAND: return 157;
        case BLOCK_CAULDRON: return 154;
        case BLOCK_END_PORTAL: return 15;
        case BLOCK_END_PORTAL_FRAME: return 158;
        case BLOCK_END_STONE: return 175;
        case BLOCK_DRAGON_EGG: return 167;
        case BLOCK_REDSTONE_LAMP_OFF: return 211;
        case BLOCK_REDSTONE_LAMP_ON: return 212;
        default: return 1;
    }
}


static void draw_block_item_icon_gui(int id, int x, int y) {
    if (!tex_terrain.id) return;
    int tile = block_item_tile_for_id(id);
    float u0, v0, u1, v1;
    terrain_tile_uv(tile, &u0, &v0, &u1, &v1);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f((float)x, (float)y);
    glTexCoord2f(u1, v0); glVertex2f((float)(x + 16), (float)y);
    glTexCoord2f(u1, v1); glVertex2f((float)(x + 16), (float)(y + 16));
    glTexCoord2f(u0, v1); glVertex2f((float)x, (float)(y + 16));
    glEnd();
    glDisable(GL_ALPHA_TEST);
}


static const char *item_display_name(int id) {
    if (id == BLOCK_STONE) return "Stone";
    if (id == BLOCK_GRASS) return "Grass";
    if (id == BLOCK_DIRT) return "Dirt";
    if (id == BLOCK_COBBLESTONE) return "Cobblestone";
    if (id == BLOCK_PLANKS) return "Wooden Planks";
    if (id == BLOCK_SAPLING) return "Sapling";
    if (id == BLOCK_BEDROCK) return "Bedrock";
    if (id == BLOCK_WATER) return "Water";
    if (id == BLOCK_STILL_WATER) return "Water";
    if (id == BLOCK_LAVA) return "Lava";
    if (id == BLOCK_STILL_LAVA) return "Lava";
    if (id == BLOCK_SAND) return "Sand";
    if (id == BLOCK_GRAVEL) return "Gravel";
    if (id == BLOCK_GOLD_ORE) return "Gold Ore";
    if (id == BLOCK_IRON_ORE) return "Iron Ore";
    if (id == BLOCK_COAL_ORE) return "Coal Ore";
    if (id == BLOCK_LOG) return "Wood";
    if (id == BLOCK_LEAVES) return "Leaves";
    if (id == BLOCK_SPONGE) return "Sponge";
    if (id == BLOCK_GLASS) return "Glass";
    if (id == BLOCK_WOOL) return "Wool";
    if (id == BLOCK_YELLOW_FLOWER) return "Flower";
    if (id == BLOCK_RED_ROSE) return "Rose";
    if (id == BLOCK_BROWN_MUSHROOM) return "Mushroom";
    if (id == BLOCK_RED_MUSHROOM) return "Mushroom";
    if (id == BLOCK_GOLD_BLOCK) return "Block of Gold";
    if (id == BLOCK_IRON_BLOCK) return "Block of Iron";
    if (id == BLOCK_DOUBLE_SLAB) return "Stone Slab";
    if (id == BLOCK_SLAB) return "Stone Slab";
    if (id == BLOCK_BRICK) return "Brick";
    if (id == BLOCK_TNT) return "TNT";
    if (id == BLOCK_BOOKSHELF) return "Bookshelf";
    if (id == BLOCK_MOSSY_COBBLESTONE) return "Moss Stone";
    if (id == BLOCK_OBSIDIAN) return "Obsidian";
    if (id == BLOCK_TORCH) return "Torch";
    if (id == BLOCK_FIRE) return "Fire";
    if (id == BLOCK_MOB_SPAWNER) return "Monster Spawner";
    if (id == BLOCK_WOOD_STAIRS) return "Wooden Stairs";
    if (id == BLOCK_CHEST) return "Chest";
    if (id == BLOCK_REDSTONE_WIRE) return "Redstone Dust";
    if (id == BLOCK_DIAMOND_ORE) return "Diamond Ore";
    if (id == BLOCK_DIAMOND_BLOCK) return "Block of Diamond";
    if (id == BLOCK_CRAFTING_TABLE) return "Workbench";
    if (id == BLOCK_CROPS) return "Crops";
    if (id == BLOCK_FARMLAND) return "Farmland";
    if (id == BLOCK_FURNACE) return "Furnace";
    if (id == BLOCK_FURNACE_LIT) return "Furnace";
    if (id == BLOCK_SIGN_POST) return "Sign";
    if (id == BLOCK_WOOD_DOOR) return "Wooden Door";
    if (id == BLOCK_LADDER) return "Ladder";
    if (id == BLOCK_RAILS) return "Rail";
    if (id == BLOCK_COBBLE_STAIRS) return "Stone Stairs";
    if (id == BLOCK_WALL_SIGN) return "Sign";
    if (id == BLOCK_LEVER) return "Lever";
    if (id == BLOCK_STONE_PRESSURE_PLATE) return "Pressure Plate";
    if (id == BLOCK_IRON_DOOR) return "Iron Door";
    if (id == BLOCK_WOOD_PRESSURE_PLATE) return "Pressure Plate";
    if (id == BLOCK_REDSTONE_ORE) return "Redstone Ore";
    if (id == BLOCK_REDSTONE_ORE_GLOWING) return "Redstone Ore";
    if (id == BLOCK_REDSTONE_TORCH_OFF) return "Redstone Torch";
    if (id == BLOCK_REDSTONE_TORCH_ON) return "Redstone Torch";
    if (id == BLOCK_STONE_BUTTON) return "Button";
    if (id == BLOCK_SNOW_LAYER) return "Snow";
    if (id == BLOCK_ICE) return "Ice";
    if (id == BLOCK_SNOW_BLOCK) return "Snow";
    if (id == BLOCK_CACTUS) return "Cactus";
    if (id == BLOCK_CLAY) return "Clay";
    if (id == BLOCK_REEDS) return "Reeds";
    if (id == BLOCK_JUKEBOX) return "Jukebox";
    if (id == BLOCK_FENCE) return "Fence";
    if (id == BLOCK_PUMPKIN) return "Pumpkin";
    if (id == BLOCK_NETHERRACK) return "Netherrack";
    if (id == BLOCK_SOUL_SAND) return "Soul Sand";
    if (id == BLOCK_GLOWSTONE) return "Glowstone";
    if (id == BLOCK_PORTAL) return "Portal";
    if (id == BLOCK_JACK_O_LANTERN) return "Jack o'Lantern";
    if (id == BLOCK_LAPIS_ORE) return "Lapis Lazuli Ore";
    if (id == BLOCK_LAPIS_BLOCK) return "Lapis Lazuli Block";
    if (id == BLOCK_DISPENSER) return "Dispenser";
    if (id == BLOCK_SANDSTONE) return "Sandstone";
    if (id == BLOCK_NOTE_BLOCK) return "Note Block";
    if (id == BLOCK_BED) return "Bed";
    if (id == BLOCK_POWERED_RAIL) return "Powered Rail";
    if (id == BLOCK_DETECTOR_RAIL) return "Detector Rail";
    if (id == BLOCK_STICKY_PISTON) return "Sticky Piston";
    if (id == BLOCK_WEB) return "Web";
    if (id == BLOCK_TALL_GRASS) return "Tall Grass";
    if (id == BLOCK_DEAD_BUSH) return "Dead Bush";
    if (id == BLOCK_PISTON) return "Piston";
    if (id == BLOCK_PISTON_EXTENSION) return "Piston Extension";
    if (id == BLOCK_PISTON_MOVING) return "Moving Piston";
    if (id == BLOCK_CAKE) return "Cake";
    if (id == BLOCK_REDSTONE_REPEATER_OFF || id == BLOCK_REDSTONE_REPEATER_ON) return "Redstone Repeater";
    if (id == BLOCK_LOCKED_CHEST) return "Locked Chest";
    if (id == BLOCK_TRAPDOOR) return "Trapdoor";
    if (id == BLOCK_SILVERFISH) return "Monster Egg";
    if (id == BLOCK_STONE_BRICK) return "Stone Bricks";
    if (id == BLOCK_BROWN_MUSHROOM_CAP || id == BLOCK_RED_MUSHROOM_CAP) return "Mushroom Cap";
    if (id == BLOCK_IRON_BARS) return "Iron Bars";
    if (id == BLOCK_GLASS_PANE) return "Glass Pane";
    if (id == BLOCK_MELON) return "Melon";
    if (id == BLOCK_PUMPKIN_STEM) return "Pumpkin Stem";
    if (id == BLOCK_MELON_STEM) return "Melon Stem";
    if (id == BLOCK_VINE) return "Vines";
    if (id == BLOCK_FENCE_GATE) return "Fence Gate";
    if (id == BLOCK_BRICK_STAIRS) return "Brick Stairs";
    if (id == BLOCK_STONE_BRICK_STAIRS) return "Stone Brick Stairs";
    if (id == BLOCK_MYCELIUM) return "Mycelium";
    if (id == BLOCK_LILY_PAD) return "Lily Pad";
    if (id == BLOCK_NETHER_BRICK) return "Nether Brick";
    if (id == BLOCK_NETHER_BRICK_FENCE) return "Nether Brick Fence";
    if (id == BLOCK_NETHER_BRICK_STAIRS) return "Nether Brick Stairs";
    if (id == BLOCK_NETHER_WART) return "Nether Wart";
    if (id == BLOCK_ENCHANTMENT_TABLE) return "Enchantment Table";
    if (id == BLOCK_BREWING_STAND) return "Brewing Stand";
    if (id == BLOCK_CAULDRON) return "Cauldron";
    if (id == BLOCK_END_PORTAL) return "End Portal";
    if (id == BLOCK_END_PORTAL_FRAME) return "End Portal Frame";
    if (id == BLOCK_END_STONE) return "End Stone";
    if (id == BLOCK_DRAGON_EGG) return "Dragon Egg";
    if (id == BLOCK_REDSTONE_LAMP_OFF || id == BLOCK_REDSTONE_LAMP_ON) return "Redstone Lamp";
    if (id == ITEM_SHOVEL_IRON) return "Iron Shovel";
    if (id == ITEM_PICKAXE_IRON) return "Iron Pickaxe";
    if (id == ITEM_AXE_IRON) return "Iron Axe";
    if (id == ITEM_FLINT_AND_IRON) return "Flint and Steel";
    if (id == ITEM_APPLE_RED) return "Apple";
    if (id == ITEM_BOW) return "Bow";
    if (id == ITEM_ARROW) return "Arrow";
    if (id == ITEM_COAL) return "Coal";
    if (id == ITEM_DIAMOND) return "Diamond";
    if (id == ITEM_INGOT_IRON) return "Iron Ingot";
    if (id == ITEM_INGOT_GOLD) return "Gold Ingot";
    if (id == ITEM_SWORD_IRON) return "Iron Sword";
    if (id == ITEM_WOODEN_SWORD) return "Wooden Sword";
    if (id == ITEM_WOODEN_SHOVEL) return "Wooden Shovel";
    if (id == ITEM_WOODEN_PICKAXE) return "Wooden Pickaxe";
    if (id == ITEM_WOODEN_AXE) return "Wooden Axe";
    if (id == ITEM_STONE_SWORD) return "Stone Sword";
    if (id == ITEM_STONE_SHOVEL) return "Stone Shovel";
    if (id == ITEM_STONE_PICKAXE) return "Stone Pickaxe";
    if (id == ITEM_STONE_AXE) return "Stone Axe";
    if (id == ITEM_SWORD_DIAMOND) return "Diamond Sword";
    if (id == ITEM_SHOVEL_DIAMOND) return "Diamond Shovel";
    if (id == ITEM_PICKAXE_DIAMOND) return "Diamond Pickaxe";
    if (id == ITEM_AXE_DIAMOND) return "Diamond Axe";
    if (id == ITEM_STICK) return "Stick";
    if (id == ITEM_BOWL_EMPTY) return "Bowl";
    if (id == ITEM_BOWL_SOUP) return "Mushroom Stew";
    if (id == ITEM_SWORD_GOLD) return "Golden Sword";
    if (id == ITEM_SHOVEL_GOLD) return "Golden Shovel";
    if (id == ITEM_PICKAXE_GOLD) return "Golden Pickaxe";
    if (id == ITEM_AXE_GOLD) return "Golden Axe";
    if (id == ITEM_STRING) return "String";
    if (id == ITEM_FEATHER) return "Feather";
    if (id == ITEM_GUNPOWDER) return "Sulphur";
    if (id == ITEM_HOE_WOOD) return "Wooden Hoe";
    if (id == ITEM_HOE_STONE) return "Stone Hoe";
    if (id == ITEM_HOE_IRON) return "Iron Hoe";
    if (id == ITEM_HOE_DIAMOND) return "Diamond Hoe";
    if (id == ITEM_HOE_GOLD) return "Golden Hoe";
    if (id == ITEM_SEEDS) return "Seeds";
    if (id == ITEM_WHEAT) return "Wheat";
    if (id == ITEM_BREAD) return "Bread";
    if (id == ITEM_HELMET_LEATHER) return "Leather Cap";
    if (id == ITEM_PLATE_LEATHER) return "Leather Tunic";
    if (id == ITEM_LEGS_LEATHER) return "Leather Pants";
    if (id == ITEM_BOOTS_LEATHER) return "Leather Boots";
    if (id == ITEM_HELMET_CHAIN) return "Chain Helmet";
    if (id == ITEM_PLATE_CHAIN) return "Chain Chestplate";
    if (id == ITEM_LEGS_CHAIN) return "Chain Leggings";
    if (id == ITEM_BOOTS_CHAIN) return "Chain Boots";
    if (id == ITEM_HELMET_IRON) return "Iron Helmet";
    if (id == ITEM_PLATE_IRON) return "Iron Chesplate";
    if (id == ITEM_LEGS_IRON) return "Iron Leggings";
    if (id == ITEM_BOOTS_IRON) return "Iron Boots";
    if (id == ITEM_HELMET_DIAMOND) return "Diamond Helmet";
    if (id == ITEM_PLATE_DIAMOND) return "Diamond Chestplate";
    if (id == ITEM_LEGS_DIAMOND) return "Diamond Leggings";
    if (id == ITEM_BOOTS_DIAMOND) return "Diamond Boots";
    if (id == ITEM_HELMET_GOLD) return "Golden Helmet";
    if (id == ITEM_PLATE_GOLD) return "Golden Chestplate";
    if (id == ITEM_LEGS_GOLD) return "Golden Leggings";
    if (id == ITEM_BOOTS_GOLD) return "Golden boots";
    if (id == ITEM_FLINT) return "Flint";
    if (id == ITEM_PORK_RAW) return "Raw Porkchop";
    if (id == ITEM_PORK_COOKED) return "Cooked Porkchop";
    if (id == ITEM_PAINTING) return "Painting";
    if (id == ITEM_APPLE_GOLD) return "Golden Apple";
    if (id == ITEM_SIGN) return "Sign";
    if (id == ITEM_DOOR_WOOD) return "Wooden Door";
    if (id == ITEM_BUCKET_EMPTY) return "Bucket";
    if (id == ITEM_BUCKET_WATER) return "Water Bucket";
    if (id == ITEM_BUCKET_LAVA) return "Lava bucket";
    if (id == ITEM_MINECART_EMPTY) return "Minecart";
    if (id == ITEM_SADDLE) return "Saddle";
    if (id == ITEM_DOOR_IRON) return "Iron Door";
    if (id == ITEM_REDSTONE) return "Redstone";
    if (id == ITEM_SNOWBALL) return "Snowball";
    if (id == ITEM_BOAT) return "Boat";
    if (id == ITEM_LEATHER) return "Leather";
    if (id == ITEM_BUCKET_MILK) return "Milk";
    if (id == ITEM_BRICK) return "Brick";
    if (id == ITEM_CLAY) return "Clay";
    if (id == ITEM_REED) return "Reeds";
    if (id == ITEM_PAPER) return "Paper";
    if (id == ITEM_BOOK) return "Book";
    if (id == ITEM_SLIME_BALL) return "Slimeball";
    if (id == ITEM_MINECART_CRATE) return "Minecart with Chest";
    if (id == ITEM_MINECART_POWERED) return "Minecart with Furnace";
    if (id == ITEM_EGG) return "Egg";
    if (id == ITEM_COMPASS) return "Compass";
    if (id == ITEM_FISHING_ROD) return "Fishing Rod";
    if (id == ITEM_CLOCK) return "Clock";
    if (id == ITEM_GLOWSTONE_DUST) return "Glowstone Dust";
    if (id == ITEM_FISH_RAW) return "Raw Fish";
    if (id == ITEM_FISH_COOKED) return "Cooked Fish";
    if (id == ITEM_DYE_POWDER) return "Dye";
    if (id == ITEM_BONE) return "Bone";
    if (id == ITEM_SUGAR) return "Sugar";
    if (id == ITEM_CAKE) return "Cake";
    if (id == ITEM_BED) return "Bed";
    if (id == ITEM_REDSTONE_REPEATER) return "Redstone Repeater";
    if (id == ITEM_COOKIE) return "Cookie";
    if (id == ITEM_MAP) return "Map";
    if (id == ITEM_SHEARS) return "Shears";
    if (id == ITEM_MELON) return "Melon";
    if (id == ITEM_PUMPKIN_SEEDS) return "Pumpkin Seeds";
    if (id == ITEM_MELON_SEEDS) return "Melon Seeds";
    if (id == ITEM_BEEF_RAW) return "Raw Beef";
    if (id == ITEM_BEEF_COOKED) return "Steak";
    if (id == ITEM_CHICKEN_RAW) return "Raw Chicken";
    if (id == ITEM_CHICKEN_COOKED) return "Cooked Chicken";
    if (id == ITEM_ROTTEN_FLESH) return "Rotten Flesh";
    if (id == ITEM_ENDER_PEARL) return "Ender Pearl";
    if (id == ITEM_BLAZE_ROD) return "Blaze Rod";
    if (id == ITEM_GHAST_TEAR) return "Ghast Tear";
    if (id == ITEM_GOLD_NUGGET) return "Gold Nugget";
    if (id == ITEM_NETHER_WART) return "Nether Wart";
    if (id == ITEM_POTION) return "Potion";
    if (id == ITEM_GLASS_BOTTLE) return "Glass Bottle";
    if (id == ITEM_SPIDER_EYE) return "Spider Eye";
    if (id == ITEM_FERMENTED_SPIDER_EYE) return "Fermented Spider Eye";
    if (id == ITEM_BLAZE_POWDER) return "Blaze Powder";
    if (id == ITEM_MAGMA_CREAM) return "Magma Cream";
    if (id == ITEM_BREWING_STAND) return "Brewing Stand";
    if (id == ITEM_CAULDRON) return "Cauldron";
    if (id == ITEM_EYE_OF_ENDER) return "Eye of Ender";
    if (id == ITEM_SPECKLED_MELON) return "Glistering Melon";
    if (id == ITEM_MONSTER_PLACER) return "Spawn Egg";
    if (id == ITEM_EXP_BOTTLE) return "Bottle o' Enchanting";
    if (id == ITEM_FIREBALL_CHARGE) return "Fire Charge";
    if (id == ITEM_RECORD13 || id == ITEM_RECORD_CAT || id == ITEM_RECORD_BLOCKS ||
        id == ITEM_RECORD_CHIRP || id == ITEM_RECORD_FAR || id == ITEM_RECORD_MALL ||
        id == ITEM_RECORD_MELLOHI || id == ITEM_RECORD_STAL || id == ITEM_RECORD_STRAD ||
        id == ITEM_RECORD_WARD || id == ITEM_RECORD_11) return "Music Disc";
    return "";
}

static void draw_item_stack_gui_base(const ItemStack *st, int x, int y, int animate_pop) {
    if (stack_empty(st)) return;
    int pushed = 0;
    float var6 = animate_pop ? ((float)st->pop_time - g_frame_partial) : 0.0f;
    if (var6 > 0.0f) {
        float var7 = 1.0f + var6 / 5.0f;
        glPushMatrix();
        glTranslatef((float)(x + 8), (float)(y + 12), 0.0f);
        glScalef(1.0f / var7, (var7 + 1.0f) / 2.0f, 1.0f);
        glTranslatef((float)-(x + 8), (float)-(y + 12), 0.0f);
        pushed = 1;
    }

    glColorMask(1,1,1,1);
    glColor4f(1,1,1,1);
    if (item_is_block_id(st->id)) {
        if (block_item_is_3d(st->id)) draw_block_item_gui_3d(st, x, y);
        else draw_block_item_icon_gui(st->id, x, y);
    } else draw_item_icon_gui_2d(st, x, y);

    if (pushed) glPopMatrix();

    /* Java renders the stack-size/damage overlay after popping the hotbar pickup
       scale matrix.  Scaling the count together with the item is what made the
       pickup UI look wrong. */
    if (st->count > 1) {
        char num[16];
        snprintf(num, sizeof(num), "%d", st->count);
        glDisable(GL_DEPTH_TEST);
        glColor4f(1,1,1,1);
        draw_text(num, x + 19 - 2 - text_width(num), y + 6 + 3, 16777215);
    }
    int max_damage = item_max_damage(st->id);
    if (max_damage > 0 && st->damage > 0) {
        int dmg = st->damage;
        if (dmg < 0) dmg = 0;
        if (dmg > max_damage) dmg = max_damage;
        int bar = (int)round(13.0 - (double)dmg * 13.0 / (double)max_damage);
        int green = (int)round(255.0 - (double)dmg * 255.0 / (double)max_damage);
        if (bar < 0) bar = 0;
        if (bar > 13) bar = 13;
        if (green < 0) green = 0;
        if (green > 255) green = 255;
        int fg = ((255 - green) << 16) | (green << 8);
        int bg = (((255 - green) / 4) << 16) | 0x3F00;
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_TEXTURE_2D);
        draw_rect(x + 2, y + 13, x + 15, y + 15, 0xFF000000);
        draw_rect(x + 2, y + 13, x + 14, y + 14, 0xFF000000 | bg);
        if (bar > 0) draw_rect(x + 2, y + 13, x + 2 + bar, y + 14, 0xFF000000 | fg);
        glEnable(GL_TEXTURE_2D);
        glColor4f(1,1,1,1);
    }
}

static void draw_item_stack_gui(const ItemStack *st, int x, int y) {
    draw_item_stack_gui_base(st, x, y, 0);
}

static void draw_item_stack_gui_animated(const ItemStack *st, int x, int y) {
    draw_item_stack_gui_base(st, x, y, 1);
}

static void draw_carried_stack(void) {
    if (!stack_empty(&g_carried_stack)) draw_item_stack_gui(&g_carried_stack, g_mouse_x - 8, g_mouse_y - 8);
}
