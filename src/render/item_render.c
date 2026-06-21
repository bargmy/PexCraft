/* Split from original monolithic main.c. Included by src/main.c unity build. */

static int item_icon_tile(int id) {
    if (id == ITEM_SHOVEL_IRON) return 82;
    if (id == ITEM_PICKAXE_IRON) return 98;
    if (id == ITEM_AXE_IRON) return 114;
    if (id == ITEM_FLINT_AND_IRON) return 5;
    if (id == ITEM_APPLE_RED) return 10;
    if (id == ITEM_BOW) return 21;
    if (id == ITEM_ARROW) return 37;
    if (id == ITEM_COAL) return 7;
    if (id == ITEM_DIAMOND) return 55;
    if (id == ITEM_INGOT_IRON) return 23;
    if (id == ITEM_INGOT_GOLD) return 39;
    if (id == ITEM_SWORD_IRON) return 66;
    if (id == ITEM_WOODEN_SWORD) return 64;
    if (id == ITEM_WOODEN_SHOVEL) return 80;
    if (id == ITEM_WOODEN_PICKAXE) return 96;
    if (id == ITEM_WOODEN_AXE) return 112;
    if (id == ITEM_STONE_SWORD) return 65;
    if (id == ITEM_STONE_SHOVEL) return 81;
    if (id == ITEM_STONE_PICKAXE) return 97;
    if (id == ITEM_STONE_AXE) return 113;
    if (id == ITEM_SWORD_DIAMOND) return 67;
    if (id == ITEM_SHOVEL_DIAMOND) return 83;
    if (id == ITEM_PICKAXE_DIAMOND) return 99;
    if (id == ITEM_AXE_DIAMOND) return 115;
    if (id == ITEM_STICK) return 53;
    if (id == ITEM_BOWL_EMPTY) return 71;
    if (id == ITEM_BOWL_SOUP) return 72;
    if (id == ITEM_SWORD_GOLD) return 68;
    if (id == ITEM_SHOVEL_GOLD) return 84;
    if (id == ITEM_PICKAXE_GOLD) return 100;
    if (id == ITEM_AXE_GOLD) return 116;
    if (id == ITEM_STRING) return 8;
    if (id == ITEM_FEATHER) return 24;
    if (id == ITEM_GUNPOWDER) return 40;
    if (id == ITEM_HOE_WOOD) return 128;
    if (id == ITEM_HOE_STONE) return 129;
    if (id == ITEM_HOE_IRON) return 130;
    if (id == ITEM_HOE_DIAMOND) return 131;
    if (id == ITEM_HOE_GOLD) return 132;
    if (id == ITEM_SEEDS) return 9;
    if (id == ITEM_WHEAT) return 25;
    if (id == ITEM_BREAD) return 41;
    if (id == ITEM_HELMET_LEATHER) return 0;
    if (id == ITEM_PLATE_LEATHER) return 16;
    if (id == ITEM_LEGS_LEATHER) return 32;
    if (id == ITEM_BOOTS_LEATHER) return 48;
    if (id == ITEM_HELMET_CHAIN) return 1;
    if (id == ITEM_PLATE_CHAIN) return 17;
    if (id == ITEM_LEGS_CHAIN) return 33;
    if (id == ITEM_BOOTS_CHAIN) return 49;
    if (id == ITEM_HELMET_IRON) return 2;
    if (id == ITEM_PLATE_IRON) return 18;
    if (id == ITEM_LEGS_IRON) return 34;
    if (id == ITEM_BOOTS_IRON) return 50;
    if (id == ITEM_HELMET_DIAMOND) return 3;
    if (id == ITEM_PLATE_DIAMOND) return 19;
    if (id == ITEM_LEGS_DIAMOND) return 35;
    if (id == ITEM_BOOTS_DIAMOND) return 51;
    if (id == ITEM_HELMET_GOLD) return 4;
    if (id == ITEM_PLATE_GOLD) return 20;
    if (id == ITEM_LEGS_GOLD) return 36;
    if (id == ITEM_BOOTS_GOLD) return 52;
    if (id == ITEM_FLINT) return 6;
    if (id == ITEM_PORK_RAW) return 87;
    if (id == ITEM_PORK_COOKED) return 88;
    if (id == ITEM_PAINTING) return 26;
    if (id == ITEM_APPLE_GOLD) return 11;
    if (id == ITEM_SIGN) return 42;
    if (id == ITEM_DOOR_WOOD) return 43;
    if (id == ITEM_BUCKET_EMPTY) return 74;
    if (id == ITEM_BUCKET_WATER) return 75;
    if (id == ITEM_BUCKET_LAVA) return 76;
    if (id == ITEM_MINECART_EMPTY) return 135;
    if (id == ITEM_SADDLE) return 104;
    if (id == ITEM_DOOR_IRON) return 44;
    if (id == ITEM_REDSTONE) return 56;
    if (id == ITEM_SNOWBALL) return 14;
    if (id == ITEM_BOAT) return 136;
    if (id == ITEM_LEATHER) return 103;
    if (id == ITEM_BUCKET_MILK) return 77;
    if (id == ITEM_BRICK) return 22;
    if (id == ITEM_CLAY) return 57;
    if (id == ITEM_REED) return 27;
    if (id == ITEM_PAPER) return 58;
    if (id == ITEM_BOOK) return 59;
    if (id == ITEM_SLIME_BALL) return 30;
    if (id == ITEM_MINECART_CRATE) return 151;
    if (id == ITEM_MINECART_POWERED) return 167;
    if (id == ITEM_EGG) return 12;
    if (id == ITEM_COMPASS) return 54;
    if (id == ITEM_FISHING_ROD) return 69;
    if (id == ITEM_CLOCK) return 70;
    if (id == ITEM_GLOWSTONE_DUST) return 73;
    if (id == ITEM_FISH_RAW) return 89;
    if (id == ITEM_FISH_COOKED) return 90;
    if (id == ITEM_RECORD13) return 240;
    if (id == ITEM_RECORD_CAT) return 241;
    return 0;
}

static int item_is_block_id(int id) {
    return (id >= 1 && id <= 91);
}

static int block_item_should_render_3d(int id) {
    if (!item_is_block_id(id)) return 0;
    if (id == BLOCK_SAPLING || id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE ||
        id == BLOCK_BROWN_MUSHROOM || id == BLOCK_RED_MUSHROOM || id == BLOCK_TORCH ||
        id == BLOCK_FIRE || id == BLOCK_REDSTONE_WIRE || id == BLOCK_CROPS ||
        id == BLOCK_SIGN_POST || id == BLOCK_WALL_SIGN || id == BLOCK_WOOD_DOOR ||
        id == BLOCK_IRON_DOOR || id == BLOCK_LADDER || id == BLOCK_RAILS ||
        id == BLOCK_LEVER || id == BLOCK_REEDS) return 0;
    return 1;
}

static void draw_item_icon_2d_gui(const ItemStack *st, int x, int y) {
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


static void draw_inventory_block_model(int id) {
    if (id == BLOCK_SLAB) { draw_cuboid_model_for_block(id, -0.5f,-0.5f,-0.5f, 0,0,0,1,0.5f,1); return; }
    if (id == BLOCK_SNOW_LAYER) { draw_cuboid_model_for_block(id, -0.5f,-0.5f,-0.5f, 0,0,0,1,0.125f,1); return; }
    if (id == BLOCK_STONE_PRESSURE_PLATE || id == BLOCK_WOOD_PRESSURE_PLATE) { draw_cuboid_model_for_block(id, -0.5f,-0.5f,-0.5f, 0.0625f,0,0.0625f,0.9375f,0.0625f,0.9375f); return; }
    if (id == BLOCK_STONE_BUTTON) { draw_cuboid_model_for_block(id, -0.5f,-0.5f,-0.5f, 0.3125f,0.375f,0.375f,0.6875f,0.625f,0.5000f); return; }
    if (id == BLOCK_CACTUS) { draw_cuboid_model_for_block(id, -0.5f,-0.5f,-0.5f, 0.0625f,0,0.0625f,0.9375f,1,0.9375f); return; }
    if (id == BLOCK_FENCE) {
        glBegin(GL_QUADS);
        emit_cuboid_model_faces_for_block(id, -0.5f,-0.5f,-0.5f, 0.375f,0.0f,0.375f,0.625f,1.0f,0.625f);
        emit_cuboid_model_faces_for_block(id, -0.5f,-0.5f,-0.5f, 0.0f,0.35f,0.4375f,1.0f,0.55f,0.5625f);
        emit_cuboid_model_faces_for_block(id, -0.5f,-0.5f,-0.5f, 0.0f,0.70f,0.4375f,1.0f,0.90f,0.5625f);
        glEnd();
        return;
    }
    if (id == BLOCK_WOOD_STAIRS || id == BLOCK_COBBLE_STAIRS) {
        draw_cuboid_model_for_block(id, -0.5f,-0.5f,-0.5f, 0,0,0,1,0.5f,1);
        draw_cuboid_model_for_block(id, -0.5f,-0.5f,-0.5f, 0.5f,0.5f,0,1,1,1);
        return;
    }
    if (id == BLOCK_CHEST) { draw_chest_block(-0.5f,-0.5f,-0.5f); return; }
    draw_world_block_id(id, -0.5f, -0.5f, -0.5f);
}

static void draw_block_item_3d_gui(const ItemStack *st, int x, int y) {
    if (stack_empty(st) || !tex_terrain.id) return;
    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    /* Java RenderItem does not clear the whole depth buffer for every slot.
       Clearing it made 3D blocks interact badly with the GUI and caused odd
       facing/depth artifacts. */
    glDisable(GL_CULL_FACE);
    glColorMask(1,1,1,1);
    glColor4f(1,1,1,1);
    int cutout_item = (st->id == BLOCK_GLASS || st->id == BLOCK_LEAVES || st->id == BLOCK_ICE);
    if (cutout_item) {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.5f);
        glDisable(GL_BLEND);
    } else {
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_BLEND);
    }

    /* Match Java RenderItem.renderItemIntoGUI: translate to slot-2/+3, scale,
       translate to the block center, then rotate. */
    glTranslatef((float)(x - 2), (float)(y + 3), 0.0f);
    glScalef(10.0f, 10.0f, 10.0f);
    glTranslatef(1.0f, 0.5f, 8.0f);
    glRotatef(210.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
    g_force_fullbright_item_model++;
    draw_inventory_block_model(st->id);
    g_force_fullbright_item_model--;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1,1,1,1);
    glPopMatrix();
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
    if (id == BLOCK_STONE) return 1;
    if (id == BLOCK_GRASS) return 0;
    if (id == BLOCK_DIRT) return 2;
    if (id == BLOCK_COBBLESTONE) return 16;
    if (id == BLOCK_PLANKS) return 4;
    if (id == BLOCK_SAPLING) return 15;
    if (id == BLOCK_BEDROCK) return 17;
    if (id == BLOCK_WATER) return item_water_tile();
    if (id == BLOCK_STILL_WATER) return item_water_tile();
    if (id == BLOCK_LAVA) return item_lava_tile();
    if (id == BLOCK_STILL_LAVA) return item_lava_tile();
    if (id == BLOCK_SAND) return 18;
    if (id == BLOCK_GRAVEL) return 19;
    if (id == BLOCK_GOLD_ORE) return 32;
    if (id == BLOCK_IRON_ORE) return 33;
    if (id == BLOCK_COAL_ORE) return 34;
    if (id == BLOCK_LOG) return 20;
    if (id == BLOCK_LEAVES) return g_opts.fancy_graphics ? 52 : 53;
    if (id == BLOCK_SPONGE) return 48;
    if (id == BLOCK_GLASS) return 49;
    if (id == BLOCK_WOOL) return 64;
    if (id == BLOCK_YELLOW_FLOWER) return 13;
    if (id == BLOCK_RED_ROSE) return 12;
    if (id == BLOCK_BROWN_MUSHROOM) return 29;
    if (id == BLOCK_RED_MUSHROOM) return 28;
    if (id == BLOCK_GOLD_BLOCK) return 39;
    if (id == BLOCK_IRON_BLOCK) return 38;
    if (id == BLOCK_DOUBLE_SLAB) return 6;
    if (id == BLOCK_SLAB) return 6;
    if (id == BLOCK_BRICK) return 7;
    if (id == BLOCK_TNT) return 8;
    if (id == BLOCK_BOOKSHELF) return 35;
    if (id == BLOCK_MOSSY_COBBLESTONE) return 36;
    if (id == BLOCK_OBSIDIAN) return 37;
    if (id == BLOCK_TORCH) return 80;
    if (id == BLOCK_FIRE) return 31;
    if (id == BLOCK_MOB_SPAWNER) return 65;
    if (id == BLOCK_WOOD_STAIRS) return 4;
    if (id == BLOCK_CHEST) return item_chest_icon_tile();
    if (id == BLOCK_REDSTONE_WIRE) return 84;
    if (id == BLOCK_DIAMOND_ORE) return 50;
    if (id == BLOCK_DIAMOND_BLOCK) return 40;
    if (id == BLOCK_CRAFTING_TABLE) return 59;
    if (id == BLOCK_CROPS) return 88;
    if (id == BLOCK_FARMLAND) return 87;
    if (id == BLOCK_FURNACE) return furnace_front_tile();
    if (id == BLOCK_FURNACE_LIT) return furnace_front_lit_tile();
    if (id == BLOCK_SIGN_POST) return 4;
    if (id == BLOCK_WOOD_DOOR) return 97;
    if (id == BLOCK_LADDER) return 83;
    if (id == BLOCK_RAILS) return 128;
    if (id == BLOCK_COBBLE_STAIRS) return 16;
    if (id == BLOCK_WALL_SIGN) return 4;
    if (id == BLOCK_LEVER) return 96;
    if (id == BLOCK_STONE_PRESSURE_PLATE) return 1;
    if (id == BLOCK_IRON_DOOR) return 98;
    if (id == BLOCK_WOOD_PRESSURE_PLATE) return 4;
    if (id == BLOCK_REDSTONE_ORE) return 51;
    if (id == BLOCK_REDSTONE_ORE_GLOWING) return 51;
    if (id == BLOCK_REDSTONE_TORCH_OFF) return 115;
    if (id == BLOCK_REDSTONE_TORCH_ON) return 99;
    if (id == BLOCK_STONE_BUTTON) return 1;
    if (id == BLOCK_SNOW_LAYER) return 66;
    if (id == BLOCK_ICE) return 67;
    if (id == BLOCK_SNOW_BLOCK) return 66;
    if (id == BLOCK_CACTUS) return 70;
    if (id == BLOCK_CLAY) return 72;
    if (id == BLOCK_REEDS) return 73;
    if (id == BLOCK_JUKEBOX) return 74;
    if (id == BLOCK_FENCE) return 4;
    if (id == BLOCK_PUMPKIN) return 102;
    if (id == BLOCK_NETHERRACK) return 103;
    if (id == BLOCK_SOUL_SAND) return 104;
    if (id == BLOCK_GLOWSTONE) return 105;
    if (id == BLOCK_PORTAL) return 14;
    if (id == BLOCK_JACK_O_LANTERN) return 102;
    if (id == BLOCK_LAPIS_ORE) return 38;
    if (id == BLOCK_LAPIS_BLOCK) return 39;
    return 1;
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
    if (id == BLOCK_JACK_O_LANTERN) return "Pumpkin";
    if (id == BLOCK_LAPIS_ORE) return "Lapis Lazuli Ore";
    if (id == BLOCK_LAPIS_BLOCK) return "Lapis Lazuli Block";
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
    if (id == ITEM_RECORD13) return "Music Disc";
    if (id == ITEM_RECORD_CAT) return "Music Disc";
    return "";
}

static void draw_item_stack_gui_ex(const ItemStack *st, int x, int y, int animate_pop) {
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
        if (block_item_should_render_3d(st->id)) draw_block_item_3d_gui(st, x, y);
        else draw_block_item_icon_gui(st->id, x, y);
    } else draw_item_icon_2d_gui(st, x, y);

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
}

static void draw_item_stack_gui(const ItemStack *st, int x, int y) {
    draw_item_stack_gui_ex(st, x, y, 0);
}

static void draw_item_stack_gui_animated(const ItemStack *st, int x, int y) {
    draw_item_stack_gui_ex(st, x, y, 1);
}

static void draw_carried_stack(void) {
    if (!stack_empty(&g_carried_stack)) draw_item_stack_gui(&g_carried_stack, g_mouse_x - 8, g_mouse_y - 8);
}

