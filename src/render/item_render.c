/* Split from original monolithic main.c. Included by src/main.c unity build. */

static int item_icon_tile(int id) {
    if (id == BLOCK_GRASS) return 3;
    if (id == BLOCK_DIRT) return 2;

    /* Source ed.java:
       stone sword  .a(1,4) => tile 65,  item id 272
       stone shovel .a(1,5) => tile 81,  item id 273
       stone pick   .a(1,6) => tile 97,  item id 274
       stone axe    .a(1,7) => tile 113, item id 275 */
    if (id == ITEM_WOODEN_SWORD) return 64;
    if (id == ITEM_WOODEN_SHOVEL) return 80;
    if (id == ITEM_WOODEN_PICKAXE) return 96;
    if (id == ITEM_WOODEN_AXE) return 112;
    if (id == ITEM_STONE_SWORD) return 65;
    if (id == ITEM_STONE_SHOVEL) return 81;
    if (id == ITEM_STONE_PICKAXE) return 97;
    if (id == ITEM_STONE_AXE) return 113;
    if (id == ITEM_STICK) return 53;
    return 0;
}

static int item_is_block_id(int id) {
    return (id >= 1 && id <= 89);
}

static void draw_item_icon_2d_gui(const ItemStack *st, int x, int y) {
    if (stack_empty(st) || !tex_items.id) return;
    int tile = item_icon_tile(st->id);
    int sx = (tile & 15) * 16;
    int sy = (tile >> 4) * 16;
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw_textured_rect_tex(&tex_items, x, y, sx, sy, 16, 16, 0xFFFFFF);
}

static void draw_block_item_3d_gui(const ItemStack *st, int x, int y) {
    if (stack_empty(st) || !tex_terrain.id) return;
    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_terrain.id);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    /* Beta inventory block items are rendered as small 3D isometric blocks, not
       flat terrain icons. This keeps the GUI orthographic projection but uses
       the same grass/dirt cube renderer as the world. */
    glTranslatef((float)x + 8.0f, (float)y + 8.5f, 80.0f);
    glScalef(10.0f, 10.0f, -10.0f);
    glRotatef(210.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
    draw_world_block_id(st->id, -0.5f, -0.5f, -0.5f);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1,1,1,1);
    glPopMatrix();
}


static int block_item_tile_for_id(int id) {
    if (id == BLOCK_WATER || id == BLOCK_STILL_WATER) return 14;
    if (id == BLOCK_LAVA || id == BLOCK_STILL_LAVA) return 30;
    if (id == BLOCK_GRASS) return 0;
    if (id == BLOCK_STONE) return 1;
    if (id == BLOCK_DIRT) return 2;
    if (id == BLOCK_COBBLESTONE) return 16;
    if (id == BLOCK_PLANKS) return 4;
    if (id == BLOCK_LOG) return 20;
    if (id == BLOCK_BEDROCK) return 17;
    if (id == BLOCK_SAND) return 18;
    if (id == BLOCK_GRAVEL) return 19;
    if (id == BLOCK_CRAFTING_TABLE) return 59;
    if (id == BLOCK_FURNACE) return 45;
    if (id == BLOCK_CHEST) return 70;
    if (id == BLOCK_GOLD_ORE) return 32;
    if (id == BLOCK_IRON_ORE) return 33;
    if (id == BLOCK_COAL_ORE) return 34;
    if (id == BLOCK_OBSIDIAN) return 35;
    if (id == BLOCK_DIAMOND_ORE) return 36;
    if (id == BLOCK_REDSTONE_ORE || id == BLOCK_REDSTONE_ORE_GLOWING) return 37;
    if (id == BLOCK_LAPIS_ORE) return 38;
    if (id == BLOCK_LAPIS_BLOCK) return 39;
    if (id == BLOCK_GOLD_BLOCK) return 40;
    if (id == BLOCK_IRON_BLOCK) return 41;
    if (id == BLOCK_DIAMOND_BLOCK) return 42;
    if (id == BLOCK_BRICK) return 47;
    if (id == BLOCK_TNT) return 48;
    if (id == BLOCK_GLASS) return 49;
    if (id == BLOCK_BOOKSHELF) return 50;
    if (id == BLOCK_MOSSY_COBBLESTONE) return 51;
    if (id == BLOCK_LEAVES) return 52;
    if (id == BLOCK_SPONGE) return 54;
    if (id == BLOCK_WOOL) return 55;
    if (id == BLOCK_SNOW_BLOCK) return 56;
    if (id == BLOCK_ICE) return 57;
    if (id == BLOCK_CLAY) return 62;
    if (id == BLOCK_NETHERRACK) return 65;
    if (id == BLOCK_SOUL_SAND) return 66;
    if (id == BLOCK_GLOWSTONE) return 67;
    return 1;
}

static void draw_block_item_icon_gui(int id, int x, int y) {
    if (!tex_terrain.id) return;
    int tile = block_item_tile_for_id(id);
    float u0, v0, u1, v1;
    terrain_tile_uv(tile, &u0, &v0, &u1, &v1);
    glEnable(GL_TEXTURE_2D);
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

static void draw_item_stack_gui(const ItemStack *st, int x, int y) {
    if (stack_empty(st)) return;
    if (item_is_block_id(st->id)) draw_block_item_3d_gui(st, x, y);
    else draw_item_icon_2d_gui(st, x, y);
    if (st->count > 1) {
        char num[16];
        snprintf(num, sizeof(num), "%d", st->count);
        draw_text(num, x + 19 - 2 - text_width(num), y + 6 + 3, 16777215);
    }
}

static void draw_carried_stack(void) {
    if (!stack_empty(&g_carried_stack)) draw_item_stack_gui(&g_carried_stack, g_mouse_x - 8, g_mouse_y - 8);
}

