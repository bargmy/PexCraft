/* Split from original monolithic main.c. Included by src/main.c unity build. */

static int item_icon_tile(int id) {
    if (id == BLOCK_GRASS) return 3;
    if (id == BLOCK_DIRT) return 2;
    if (id == ITEM_STONE_SHOVEL) return 81; /* source ed.java: stone shovel .a(1,5) */
    return 0;
}

static int item_is_block_id(int id) {
    return id == BLOCK_GRASS || id == BLOCK_DIRT || id == BLOCK_BEDROCK;
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

