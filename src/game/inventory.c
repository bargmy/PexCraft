/* Split from original monolithic main.c. Included by src/main.c unity build. */

static int stack_empty(const ItemStack *s) { return !s || s->id <= 0 || s->count <= 0; }

static void stack_clear(ItemStack *s) { if (s) memset(s, 0, sizeof(*s)); }

static int stack_same_item(const ItemStack *a, const ItemStack *b) { return !stack_empty(a) && !stack_empty(b) && a->id == b->id && a->damage == b->damage; }

static int stack_limit_for_id(int id) {
    if (id == ITEM_WOODEN_SWORD || id == ITEM_WOODEN_SHOVEL || id == ITEM_WOODEN_PICKAXE || id == ITEM_WOODEN_AXE ||
        id == ITEM_STONE_SWORD || id == ITEM_STONE_SHOVEL || id == ITEM_STONE_PICKAXE || id == ITEM_STONE_AXE ||
        id == ITEM_IRON_SWORD || id == ITEM_IRON_SHOVEL || id == ITEM_IRON_PICKAXE || id == ITEM_IRON_AXE ||
        id == ITEM_DIAMOND_SWORD || id == ITEM_DIAMOND_SHOVEL || id == ITEM_DIAMOND_PICKAXE || id == ITEM_DIAMOND_AXE ||
        id == ITEM_GOLD_SWORD || id == ITEM_GOLD_SHOVEL || id == ITEM_GOLD_PICKAXE || id == ITEM_GOLD_AXE ||
        id == ITEM_WOODEN_HOE || id == ITEM_STONE_HOE || id == ITEM_IRON_HOE || id == ITEM_DIAMOND_HOE || id == ITEM_GOLD_HOE ||
        id == ITEM_BOW || id == ITEM_FLINT_AND_IRON ||
        id == ITEM_LEATHER_CAP || id == ITEM_LEATHER_CHESTPLATE || id == ITEM_LEATHER_LEGGINGS || id == ITEM_LEATHER_BOOTS ||
        id == ITEM_CHAIN_HELMET || id == ITEM_CHAIN_CHESTPLATE || id == ITEM_CHAIN_LEGGINGS || id == ITEM_CHAIN_BOOTS ||
        id == ITEM_IRON_HELMET || id == ITEM_IRON_CHESTPLATE || id == ITEM_IRON_LEGGINGS || id == ITEM_IRON_BOOTS ||
        id == ITEM_DIAMOND_HELMET || id == ITEM_DIAMOND_CHESTPLATE || id == ITEM_DIAMOND_LEGGINGS || id == ITEM_DIAMOND_BOOTS ||
        id == ITEM_GOLD_HELMET || id == ITEM_GOLD_CHESTPLATE || id == ITEM_GOLD_LEGGINGS || id == ITEM_GOLD_BOOTS ||
        id == ITEM_MINECART || id == ITEM_MINECART_CRATE || id == ITEM_MINECART_POWERED || id == ITEM_BOAT ||
        id == ITEM_SADDLE || id == ITEM_WOOD_DOOR || id == ITEM_IRON_DOOR || id == ITEM_SIGN ||
        id == ITEM_BUCKET_EMPTY || id == ITEM_WATER_BUCKET || id == ITEM_LAVA_BUCKET || id == ITEM_MILK ||
        id == ITEM_MUSHROOM_STEW || id == ITEM_CAKE || id == ITEM_BED || id == ITEM_MAP ||
        id == ITEM_SHEARS || id == ITEM_POTION || id == ITEM_EXP_BOTTLE ||
        id == ITEM_RECORD13 || id == ITEM_RECORD_CAT || id == ITEM_RECORD_BLOCKS || id == ITEM_RECORD_CHIRP ||
        id == ITEM_RECORD_FAR || id == ITEM_RECORD_MALL || id == ITEM_RECORD_MELLOHI || id == ITEM_RECORD_STAL ||
        id == ITEM_RECORD_STRAD || id == ITEM_RECORD_WARD || id == ITEM_RECORD_11) return 1;
    if (id == ITEM_SNOWBALL || id == ITEM_EGG || id == ITEM_ENDER_PEARL) return 16;
    (void)id;
    return ITEM_MAX_STACK;
}

static ItemStack make_stack(int id, int count, int damage) { ItemStack s; s.id = id; s.count = count; s.damage = damage; s.pop_time = 0; return s; }
static void spawn_item_stack(float x, float y, float z, ItemStack st, int random_spread);
static void unsupported_block_neighbor_cleanup(int x, int y, int z);
static void damage_held_item(ItemStack *held, int amount);
static int jukebox_eject_record(int x, int y, int z);
static int passive_mobs_spawn_from_egg_damage(int egg_damage, float x, float y, float z);
static int passive_mobs_apply_dye_to_target(int dye_damage);
static int spawn_flat_vehicle(int type, float x, float y, float z, float yaw);

typedef struct CreativeItemDef { int id; int damage; } CreativeItemDef;

/* Minecraft Java 1.2.5 ContainerCreative ordering.  The creative container is
   eight columns wide, nine rows tall, with a 72-slot temporary inventory and
   the player's hotbar at y=184.  Do not sort this table by PexCraft IDs: Java
   deliberately groups blocks before iterating Item.itemsList. */
static const CreativeItemDef g_creative_blocks_125[] = {
    {BLOCK_COBBLESTONE,0}, {BLOCK_STONE,0}, {BLOCK_DIAMOND_ORE,0}, {BLOCK_GOLD_ORE,0}, {BLOCK_IRON_ORE,0}, {BLOCK_COAL_ORE,0}, {BLOCK_LAPIS_ORE,0}, {BLOCK_REDSTONE_ORE,0},
    {BLOCK_STONE_BRICK,0}, {BLOCK_STONE_BRICK,1}, {BLOCK_STONE_BRICK,2}, {BLOCK_STONE_BRICK,3}, {BLOCK_CLAY,0}, {BLOCK_DIAMOND_BLOCK,0}, {BLOCK_GOLD_BLOCK,0}, {BLOCK_IRON_BLOCK,0},
    {BLOCK_BEDROCK,0}, {BLOCK_LAPIS_BLOCK,0}, {BLOCK_BRICK,0}, {BLOCK_MOSSY_COBBLESTONE,0}, {BLOCK_SLAB,0}, {BLOCK_SLAB,1}, {BLOCK_SLAB,2}, {BLOCK_SLAB,3},
    {BLOCK_SLAB,4}, {BLOCK_SLAB,5}, {BLOCK_OBSIDIAN,0}, {BLOCK_NETHERRACK,0}, {BLOCK_SOUL_SAND,0}, {BLOCK_GLOWSTONE,0}, {BLOCK_LOG,0}, {BLOCK_LOG,1},
    {BLOCK_LOG,2}, {BLOCK_LOG,3}, {BLOCK_LEAVES,0}, {BLOCK_LEAVES,1}, {BLOCK_LEAVES,2}, {BLOCK_LEAVES,3}, {BLOCK_DIRT,0}, {BLOCK_GRASS,0},
    {BLOCK_SAND,0}, {BLOCK_SANDSTONE,0}, {BLOCK_SANDSTONE,1}, {BLOCK_SANDSTONE,2}, {BLOCK_GRAVEL,0}, {BLOCK_WEB,0}, {BLOCK_PLANKS,0}, {BLOCK_PLANKS,1},
    {BLOCK_PLANKS,2}, {BLOCK_PLANKS,3}, {BLOCK_SAPLING,0}, {BLOCK_SAPLING,1}, {BLOCK_SAPLING,2}, {BLOCK_SAPLING,3}, {BLOCK_DEAD_BUSH,0}, {BLOCK_SPONGE,0},
    {BLOCK_ICE,0}, {BLOCK_SNOW_BLOCK,0}, {BLOCK_YELLOW_FLOWER,0}, {BLOCK_RED_ROSE,0}, {BLOCK_BROWN_MUSHROOM,0}, {BLOCK_RED_MUSHROOM,0}, {BLOCK_CACTUS,0}, {BLOCK_MELON,0},
    {BLOCK_PUMPKIN,0}, {BLOCK_JACK_O_LANTERN,0}, {BLOCK_VINE,0}, {BLOCK_IRON_BARS,0}, {BLOCK_GLASS_PANE,0}, {BLOCK_NETHER_BRICK,0}, {BLOCK_NETHER_BRICK_FENCE,0}, {BLOCK_NETHER_BRICK_STAIRS,0},
    {BLOCK_END_STONE,0}, {BLOCK_MYCELIUM,0}, {BLOCK_LILY_PAD,0}, {BLOCK_TALL_GRASS,1}, {BLOCK_TALL_GRASS,2}, {BLOCK_CHEST,0}, {BLOCK_CRAFTING_TABLE,0}, {BLOCK_GLASS,0},
    {BLOCK_TNT,0}, {BLOCK_BOOKSHELF,0}, {BLOCK_WOOL,0}, {BLOCK_WOOL,1}, {BLOCK_WOOL,2}, {BLOCK_WOOL,3}, {BLOCK_WOOL,4}, {BLOCK_WOOL,5},
    {BLOCK_WOOL,6}, {BLOCK_WOOL,7}, {BLOCK_WOOL,8}, {BLOCK_WOOL,9}, {BLOCK_WOOL,10}, {BLOCK_WOOL,11}, {BLOCK_WOOL,12}, {BLOCK_WOOL,13},
    {BLOCK_WOOL,14}, {BLOCK_WOOL,15}, {BLOCK_DISPENSER,0}, {BLOCK_FURNACE,0}, {BLOCK_NOTE_BLOCK,0}, {BLOCK_JUKEBOX,0}, {BLOCK_STICKY_PISTON,0}, {BLOCK_PISTON,0},
    {BLOCK_FENCE,0}, {BLOCK_FENCE_GATE,0}, {BLOCK_LADDER,0}, {BLOCK_RAILS,0}, {BLOCK_POWERED_RAIL,0}, {BLOCK_DETECTOR_RAIL,0}, {BLOCK_TORCH,0}, {BLOCK_WOOD_STAIRS,0},
    {BLOCK_COBBLE_STAIRS,0}, {BLOCK_BRICK_STAIRS,0}, {BLOCK_STONE_BRICK_STAIRS,0}, {BLOCK_LEVER,0}, {BLOCK_STONE_PRESSURE_PLATE,0}, {BLOCK_WOOD_PRESSURE_PLATE,0}, {BLOCK_REDSTONE_TORCH_ON,0}, {BLOCK_STONE_BUTTON,0},
    {BLOCK_TRAPDOOR,0}, {BLOCK_ENCHANTMENT_TABLE,0}, {BLOCK_REDSTONE_LAMP_OFF,0}
};

static const int g_creative_items_125[] = {
    ITEM_SHOVEL_IRON, ITEM_PICKAXE_IRON, ITEM_AXE_IRON, ITEM_FLINT_AND_IRON, ITEM_APPLE_RED, ITEM_BOW, ITEM_ARROW, ITEM_COAL,
    ITEM_DIAMOND, ITEM_INGOT_IRON, ITEM_INGOT_GOLD, ITEM_SWORD_IRON, ITEM_WOODEN_SWORD, ITEM_WOODEN_SHOVEL, ITEM_WOODEN_PICKAXE, ITEM_WOODEN_AXE,
    ITEM_STONE_SWORD, ITEM_STONE_SHOVEL, ITEM_STONE_PICKAXE, ITEM_STONE_AXE, ITEM_SWORD_DIAMOND, ITEM_SHOVEL_DIAMOND, ITEM_PICKAXE_DIAMOND, ITEM_AXE_DIAMOND,
    ITEM_STICK, ITEM_BOWL_EMPTY, ITEM_BOWL_SOUP, ITEM_SWORD_GOLD, ITEM_SHOVEL_GOLD, ITEM_PICKAXE_GOLD, ITEM_AXE_GOLD, ITEM_STRING,
    ITEM_FEATHER, ITEM_GUNPOWDER, ITEM_HOE_WOOD, ITEM_HOE_STONE, ITEM_HOE_IRON, ITEM_HOE_DIAMOND, ITEM_HOE_GOLD, ITEM_SEEDS,
    ITEM_WHEAT, ITEM_BREAD, ITEM_HELMET_LEATHER, ITEM_PLATE_LEATHER, ITEM_LEGS_LEATHER, ITEM_BOOTS_LEATHER, ITEM_HELMET_CHAIN, ITEM_PLATE_CHAIN,
    ITEM_LEGS_CHAIN, ITEM_BOOTS_CHAIN, ITEM_HELMET_IRON, ITEM_PLATE_IRON, ITEM_LEGS_IRON, ITEM_BOOTS_IRON, ITEM_HELMET_DIAMOND, ITEM_PLATE_DIAMOND,
    ITEM_LEGS_DIAMOND, ITEM_BOOTS_DIAMOND, ITEM_HELMET_GOLD, ITEM_PLATE_GOLD, ITEM_LEGS_GOLD, ITEM_BOOTS_GOLD, ITEM_FLINT, ITEM_PORK_RAW,
    ITEM_PORK_COOKED, ITEM_PAINTING, ITEM_APPLE_GOLD, ITEM_SIGN, ITEM_DOOR_WOOD, ITEM_BUCKET_EMPTY, ITEM_BUCKET_WATER, ITEM_BUCKET_LAVA,
    ITEM_MINECART_EMPTY, ITEM_SADDLE, ITEM_DOOR_IRON, ITEM_REDSTONE, ITEM_SNOWBALL, ITEM_BOAT, ITEM_LEATHER, ITEM_BUCKET_MILK,
    ITEM_BRICK, ITEM_CLAY, ITEM_REED, ITEM_PAPER, ITEM_BOOK, ITEM_SLIME_BALL, ITEM_MINECART_CRATE, ITEM_MINECART_POWERED,
    ITEM_EGG, ITEM_COMPASS, ITEM_FISHING_ROD, ITEM_CLOCK, ITEM_GLOWSTONE_DUST, ITEM_FISH_RAW, ITEM_FISH_COOKED, ITEM_DYE_POWDER,
    ITEM_BONE, ITEM_SUGAR, ITEM_CAKE, ITEM_BED, ITEM_REDSTONE_REPEATER, ITEM_COOKIE, ITEM_MAP, ITEM_SHEARS,
    ITEM_MELON, ITEM_PUMPKIN_SEEDS, ITEM_MELON_SEEDS, ITEM_BEEF_RAW, ITEM_BEEF_COOKED, ITEM_CHICKEN_RAW, ITEM_CHICKEN_COOKED, ITEM_ROTTEN_FLESH,
    ITEM_ENDER_PEARL, ITEM_BLAZE_ROD, ITEM_GHAST_TEAR, ITEM_GOLD_NUGGET, ITEM_NETHER_WART, ITEM_GLASS_BOTTLE, ITEM_SPIDER_EYE, ITEM_FERMENTED_SPIDER_EYE,
    ITEM_BLAZE_POWDER, ITEM_MAGMA_CREAM, ITEM_BREWING_STAND, ITEM_CAULDRON, ITEM_EYE_OF_ENDER, ITEM_SPECKLED_MELON, ITEM_EXP_BOTTLE, ITEM_FIREBALL_CHARGE,
    ITEM_RECORD13, ITEM_RECORD_CAT, ITEM_RECORD_BLOCKS, ITEM_RECORD_CHIRP, ITEM_RECORD_FAR, ITEM_RECORD_MALL, ITEM_RECORD_MELLOHI, ITEM_RECORD_STAL,
    ITEM_RECORD_STRAD, ITEM_RECORD_WARD, ITEM_RECORD_11
};

static const int g_creative_spawn_egg_damage_125[] = {
    /* Java 1.2.5 official eggs plus hidden IDs for mobs that exist but did not
       have a creative egg entry in vanilla (Giant, Dragon, Snow/Iron Golem). */
    50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 120
};

#define CREATIVE_COLS 8
#define CREATIVE_ROWS 9
#define CREATIVE_XSIZE 176
#define CREATIVE_YSIZE 208

static int creative_catalog_count(void) {
    return (int)(sizeof(g_creative_blocks_125) / sizeof(g_creative_blocks_125[0])) +
           (int)(sizeof(g_creative_items_125) / sizeof(g_creative_items_125[0])) +
           15 +
           (int)(sizeof(g_creative_spawn_egg_damage_125) / sizeof(g_creative_spawn_egg_damage_125[0]));
}

static int creative_total_rows(void) {
    return (creative_catalog_count() + CREATIVE_COLS - 1) / CREATIVE_COLS;
}

static int creative_max_row_scroll(void) {
    int max = creative_total_rows() - CREATIVE_ROWS;
    return max > 0 ? max : 0;
}

static void creative_scroll_to_row(int row) {
    int max = creative_max_row_scroll();
    if (row < 0) row = 0;
    if (row > max) row = max;
    g_creative_scroll_row = row;
    g_creative_scroll = max > 0 ? (float)row / (float)max : 0.0f;
}

static void creative_scroll_by(int rows) {
    creative_scroll_to_row(g_creative_scroll_row + rows);
}

static void creative_scroll_page(int delta) {
    creative_scroll_by(delta * CREATIVE_ROWS);
}

static void creative_set_scroll_fraction(float f) {
    if (f < 0.0f) f = 0.0f;
    if (f > 1.0f) f = 1.0f;
    int max = creative_max_row_scroll();
    g_creative_scroll = f;
    g_creative_scroll_row = (int)((double)(f * (float)max) + 0.5);
    if (g_creative_scroll_row < 0) g_creative_scroll_row = 0;
    if (g_creative_scroll_row > max) g_creative_scroll_row = max;
}

static void creative_mouse_drag(int my) {
    if (!g_creative_dragging_scroll) return;
    int x = (g_gui_w - CREATIVE_XSIZE) / 2;
    int y = (g_gui_h - CREATIVE_YSIZE) / 2;
    (void)x;
    int track_top = y + 17;
    int track_bottom = y + 17 + 160 + 2;
    float f = (float)(my - (track_top + 8)) / ((float)(track_bottom - track_top) - 16.0f);
    creative_set_scroll_fraction(f);
}

static ItemStack creative_stack_for_index(int idx) {
    if (idx < 0 || idx >= creative_catalog_count()) return make_stack(0, 0, 0);
    int nblocks = (int)(sizeof(g_creative_blocks_125) / sizeof(g_creative_blocks_125[0]));
    if (idx < nblocks) return make_stack(g_creative_blocks_125[idx].id, 1, g_creative_blocks_125[idx].damage);
    idx -= nblocks;
    int nitems = (int)(sizeof(g_creative_items_125) / sizeof(g_creative_items_125[0]));
    if (idx < nitems) return make_stack(g_creative_items_125[idx], 1, 0);
    idx -= nitems;
    if (idx < 15) return make_stack(ITEM_DYE_POWDER, 1, idx + 1);
    idx -= 15;
    int neggs = (int)(sizeof(g_creative_spawn_egg_damage_125) / sizeof(g_creative_spawn_egg_damage_125[0]));
    if (idx < neggs) return make_stack(ITEM_MONSTER_PLACER, 1, g_creative_spawn_egg_damage_125[idx]);
    return make_stack(0, 0, 0);
}

static int creative_item_index_at(int mx, int my) {
    int x = (g_gui_w - CREATIVE_XSIZE) / 2;
    int y = (g_gui_h - CREATIVE_YSIZE) / 2;
    int grid_x = x + 8;
    int grid_y = y + 18;
    if (mx < grid_x || my < grid_y || mx >= grid_x + CREATIVE_COLS * 18 || my >= grid_y + CREATIVE_ROWS * 18) return -1;
    int col = (mx - grid_x) / 18;
    int row = (my - grid_y) / 18;
    int idx = (g_creative_scroll_row + row) * CREATIVE_COLS + col;
    return (idx >= 0 && idx < creative_catalog_count()) ? idx : -1;
}

static int creative_hotbar_slot_at(int mx, int my) {
    int x = (g_gui_w - CREATIVE_XSIZE) / 2;
    int y = (g_gui_h - CREATIVE_YSIZE) / 2;
    int hx = x + 8;
    int hy = y + 184;
    if (mx < hx || my < hy || mx >= hx + 9 * 18 || my >= hy + 18) return -1;
    return (mx - hx) / 18;
}

static int creative_scrollbar_hit(int mx, int my) {
    int x = (g_gui_w - CREATIVE_XSIZE) / 2;
    int y = (g_gui_h - CREATIVE_YSIZE) / 2;
    int sx = x + 155;
    int sy = y + 17;
    return mx >= sx && mx < sx + 14 && my >= sy && my < sy + 162;
}

static void creative_click_hotbar_slot(int hotbar, int button) {
    if (hotbar < 0 || hotbar >= 9) return;
    ItemStack *s = &g_inventory[hotbar];
    if (button == 0) {
        if (stack_empty(&g_carried_stack)) {
            if (!stack_empty(s)) { g_carried_stack = *s; stack_clear(s); }
        } else if (stack_empty(s)) {
            *s = g_carried_stack; stack_clear(&g_carried_stack); s->pop_time = 5;
        } else if (stack_same_item(s, &g_carried_stack) && s->count < stack_limit_for_id(s->id)) {
            int room = stack_limit_for_id(s->id) - s->count;
            int move = g_carried_stack.count < room ? g_carried_stack.count : room;
            s->count += move;
            g_carried_stack.count -= move;
            s->pop_time = 5;
            if (g_carried_stack.count <= 0) stack_clear(&g_carried_stack);
        } else {
            ItemStack tmp = *s; *s = g_carried_stack; g_carried_stack = tmp; s->pop_time = 5;
        }
    } else {
        if (stack_empty(&g_carried_stack)) {
            if (!stack_empty(s)) {
                int take = (s->count + 1) / 2;
                g_carried_stack = make_stack(s->id, take, s->damage);
                s->count -= take;
                if (s->count <= 0) stack_clear(s);
            }
        } else if (stack_empty(s)) {
            *s = make_stack(g_carried_stack.id, 1, g_carried_stack.damage);
            s->pop_time = 5;
            if (--g_carried_stack.count <= 0) stack_clear(&g_carried_stack);
        } else if (stack_same_item(s, &g_carried_stack) && s->count < stack_limit_for_id(s->id)) {
            s->count++;
            s->pop_time = 5;
            if (--g_carried_stack.count <= 0) stack_clear(&g_carried_stack);
        } else {
            ItemStack tmp = *s; *s = g_carried_stack; g_carried_stack = tmp; s->pop_time = 5;
        }
    }
    g_save_dirty = 1;
}

static void creative_mouse_click(int mx, int my, int button) {
    if (button == 0 && creative_scrollbar_hit(mx, my)) {
        g_creative_dragging_scroll = 1;
        creative_mouse_drag(my);
        return;
    }

    int idx = creative_item_index_at(mx, my);
    if (idx >= 0) {
        ItemStack clicked = creative_stack_for_index(idx);
        int shift = key_down_vk(VK_SHIFT);
        if (!stack_empty(&g_carried_stack) && !stack_empty(&clicked) && g_carried_stack.id == clicked.id) {
            if (button == 0) {
                if (shift) g_carried_stack.count = stack_limit_for_id(g_carried_stack.id);
                else if (g_carried_stack.count < stack_limit_for_id(g_carried_stack.id)) g_carried_stack.count++;
            } else {
                if (g_carried_stack.count <= 1) stack_clear(&g_carried_stack);
                else g_carried_stack.count--;
            }
        } else if (!stack_empty(&g_carried_stack)) {
            stack_clear(&g_carried_stack);
        } else if (stack_empty(&clicked)) {
            stack_clear(&g_carried_stack);
        } else {
            g_carried_stack = clicked;
            if (shift) g_carried_stack.count = stack_limit_for_id(g_carried_stack.id);
        }
        return;
    }

    int hotbar = creative_hotbar_slot_at(mx, my);
    if (hotbar >= 0) {
        creative_click_hotbar_slot(hotbar, button);
        return;
    }

    int x = (g_gui_w - CREATIVE_XSIZE) / 2;
    int y = (g_gui_h - CREATIVE_YSIZE) / 2;
    if (mx < x || my < y || mx >= x + CREATIVE_XSIZE || my >= y + CREATIVE_YSIZE) {
        if (!stack_empty(&g_carried_stack)) {
            ItemStack drop = g_carried_stack;
            if (button != 0) drop.count = 1;
            if (g_mp_connected) pex_net_send_drop_item(drop);
            else spawn_item_stack(g_player_x, g_player_y - 0.30f, g_player_z, drop, 0);
            if (button == 0) stack_clear(&g_carried_stack);
            else if (--g_carried_stack.count <= 0) stack_clear(&g_carried_stack);
        }
    }
}

static int armor_stack_type(int id) {
    if (id == ITEM_HELMET_LEATHER || id == ITEM_HELMET_CHAIN || id == ITEM_HELMET_IRON || id == ITEM_HELMET_DIAMOND || id == ITEM_HELMET_GOLD) return 0;
    if (id == ITEM_PLATE_LEATHER || id == ITEM_PLATE_CHAIN || id == ITEM_PLATE_IRON || id == ITEM_PLATE_DIAMOND || id == ITEM_PLATE_GOLD) return 1;
    if (id == ITEM_LEGS_LEATHER || id == ITEM_LEGS_CHAIN || id == ITEM_LEGS_IRON || id == ITEM_LEGS_DIAMOND || id == ITEM_LEGS_GOLD) return 2;
    if (id == ITEM_BOOTS_LEATHER || id == ITEM_BOOTS_CHAIN || id == ITEM_BOOTS_IRON || id == ITEM_BOOTS_DIAMOND || id == ITEM_BOOTS_GOLD) return 3;
    return -1;
}

static int armor_slot_index_for_type(int armor_type) {
    /* Matches Java InventoryPlayer: armorInventory[0]=boots, [1]=legs, [2]=chest, [3]=helmet. */
    if (armor_type < 0 || armor_type > 3) return -1;
    return 3 - armor_type;
}

static int armor_stack_material_index(int id) {
    if (id == ITEM_HELMET_LEATHER || id == ITEM_PLATE_LEATHER || id == ITEM_LEGS_LEATHER || id == ITEM_BOOTS_LEATHER) return 0;
    if (id == ITEM_HELMET_CHAIN || id == ITEM_PLATE_CHAIN || id == ITEM_LEGS_CHAIN || id == ITEM_BOOTS_CHAIN) return 1;
    if (id == ITEM_HELMET_IRON || id == ITEM_PLATE_IRON || id == ITEM_LEGS_IRON || id == ITEM_BOOTS_IRON) return 2;
    if (id == ITEM_HELMET_DIAMOND || id == ITEM_PLATE_DIAMOND || id == ITEM_LEGS_DIAMOND || id == ITEM_BOOTS_DIAMOND) return 3;
    if (id == ITEM_HELMET_GOLD || id == ITEM_PLATE_GOLD || id == ITEM_LEGS_GOLD || id == ITEM_BOOTS_GOLD) return 4;
    return -1;
}

static int armor_stack_level(int id) {
    int mat = armor_stack_material_index(id);
    if (mat == 0) return 0;       /* leather */
    if (mat == 1 || mat == 4) return 1; /* chain/gold */
    if (mat == 2) return 2;       /* iron */
    if (mat == 3) return 3;       /* diamond */
    return 0;
}

static int armor_stack_max_damage(int id) {
    static const int base_by_type[4] = {11, 16, 15, 13};
    int type = armor_stack_type(id);
    if (type < 0) return 0;
    return (base_by_type[type] * 3) << armor_stack_level(id);
}

static int armor_stack_damage_reduce_amount(int id) {
    static const int reduce_by_type[4] = {3, 8, 6, 3};
    int type = armor_stack_type(id);
    return (type >= 0) ? reduce_by_type[type] : 0;
}

static int armor_slot_is_armor(int slot) { return slot >= 110 && slot < 114; }

static int armor_inventory_index_for_slot(int slot) {
    if (!armor_slot_is_armor(slot)) return -1;
    return 3 - (slot - 110);
}

static int armor_slot_type_for_slot(int slot) {
    if (!armor_slot_is_armor(slot)) return -1;
    return slot - 110;
}

static int armor_stack_valid_for_slot(const ItemStack *st, int slot) {
    int required_type = armor_slot_type_for_slot(slot);
    int type = st ? armor_stack_type(st->id) : -1;
    return required_type >= 0 && type == required_type && st->count == 1;
}

static int armor_total_value(void) {
    int total_reduce = 0;
    int total_remaining = 0;
    int total_max = 0;
    for (int i = 0; i < 4; i++) {
        ItemStack *s = &g_armor_inventory[i];
        if (stack_empty(s) || armor_stack_type(s->id) < 0) continue;
        int max_damage = armor_stack_max_damage(s->id);
        int remaining = max_damage - s->damage;
        if (remaining < 0) remaining = 0;
        total_reduce += armor_stack_damage_reduce_amount(s->id);
        total_remaining += remaining;
        total_max += max_damage;
    }
    if (total_max == 0) return 0;
    return (total_reduce - 1) * total_remaining / total_max + 1;
}

static void armor_sync_player_armor(void) {
    g_player_armor = armor_total_value();
    if (g_player_armor < 0) g_player_armor = 0;
    if (g_player_armor > 20) g_player_armor = 20;
}

static void armor_damage_equipped(int amount) {
    if (amount <= 0) return;
    for (int i = 0; i < 4; i++) {
        ItemStack *s = &g_armor_inventory[i];
        if (stack_empty(s) || armor_stack_type(s->id) < 0) continue;
        int max_damage = armor_stack_max_damage(s->id);
        s->damage += amount;
        if (s->damage > max_damage) {
            stack_clear(s);
        }
    }
    armor_sync_player_armor();
}

static int armor_apply_damage_reduction(int incoming) {
    if (incoming <= 0) return 0;
    int armor = armor_total_value();
    int scaled = incoming * (25 - armor) + g_player_damage_remainder;
    armor_damage_equipped(incoming);
    int out = scaled / 25;
    g_player_damage_remainder = scaled % 25;
    if (out < 0) out = 0;
    return out;
}

static int flat_index(int coord) { return coord - g_flat_world_origin_x; }
static int flat_z_index(int coord) { return coord - g_flat_world_origin_z; }
static int floor_div16(int v);
static int stream_world_chunk_in_window(int wcx, int wcz, int origin_x, int origin_z);
static int stream_local_chunk_x(int wcx);
static int stream_local_chunk_z(int wcz);
static void flat_mark_chunks_dirty_near(int x, int z);
static void flat_mark_all_sections_dirty(void);
static void flat_mark_sections_dirty_near_block(int x, int y, int z);
static void flat_mark_generated_section(int lcx, int lcz, int sy);
static void flat_mark_section_dirty(int cx, int cz, int sy);
static int flat_section_has_blocks(int lcx, int lcz, int sy);
static void flat_mark_light_dirty_region(int x0, int z0, int x1, int z1);
static void flat_bump_light_version(int lcx, int lcz);
static void flat_publish_light_ready(int lcx, int lcz, int ready, int dirty_sections);
static int flat_chunk_light_ready(int lcx, int lcz);
static void flat_reset_sky_light(int lcx, int lcz);
static void flat_apply_environment_light(int lcx, int lcz);
static void mark_flat_chunk_modified_at(int x, int z);
static int g_flat_persistent_edit_depth = 0;
static void flat_flush_pending_lighting(void);
static void flat_preview_pending_lighting(void);
static void flat_lighting_worker_ensure(void);
static void flat_lighting_worker_wake(void);
static void flat_lighting_worker_shutdown(void);
static int flat_lighting_pending_dirty(void);
static volatile int g_stream_remap_in_progress = 0;
static void flat_begin_persistent_edit(void) { g_flat_persistent_edit_depth++; }
static void flat_end_persistent_edit(void) {
    if (g_flat_persistent_edit_depth > 0) g_flat_persistent_edit_depth--;
    /* Do not run the full light repair from the gameplay tick.  A Java-sized
       torch/skylight refresh can touch tens or hundreds of thousands of cells,
       and doing that here showed up as 40ms+ in ingame/player logic when a block
       was broken.  Keep a tiny local preview so the edited block reacts right
       away, then let the stream service finish the full pending repair off the
       gameplay path. */
    if (g_flat_persistent_edit_depth == 0) flat_preview_pending_lighting();
}
static int flat_persistent_edit_active(void) { return g_flat_persistent_edit_depth > 0; }
static int flat_block_intersects_player(int bx, int by, int bz);
static void spawn_item_stack(float x, float y, float z, ItemStack st, int random_spread);
static int passive_mobs_spawn_from_egg_damage(int egg_damage, float x, float y, float z);
static int passive_mobs_apply_dye_to_target(int dye_damage);
static int stream_world_chunk_in_window(int wcx, int wcz, int origin_x, int origin_z);
static int stream_local_chunk_x(int wcx);
static int stream_local_chunk_z(int wcz);
static void stream_mark_chunk_dirty(int lcx, int lcz);
static void stream_mark_chunk_generated(int lcx, int lcz);
static void stream_queue_add_chunk(int wcx, int wcz);
static void stream_queue_chunk_safety(int base_cx, int base_cz, int pcx, int pcz, int radius);
static int stream_queue_visible_chunks(void);
static void process_stream_generation_queue(void);
static int stream_generation_active(void);
static int psp_player_terrain_ready_or_hold(void);
static void wake_neighbor_liquids(int x, int y, int z);
static void fluid_check_for_harden(int x, int y, int z);
static void flat_place_fluid_source(int x, int y, int z, int id);

static int flat_y_index(int y) { return y - FLAT_WORLD_Y_MIN; }

static int flat_in_bounds(int x, int y, int z) {
    return y >= FLAT_WORLD_Y_MIN && y <= FLAT_WORLD_Y_MAX &&
           x >= g_flat_world_origin_x && x < g_flat_world_origin_x + FLAT_WORLD_SIZE &&
           z >= g_flat_world_origin_z && z < g_flat_world_origin_z + FLAT_WORLD_SIZE;
}

static int flat_local_chunk_x(int x) { return flat_index(x) / FLAT_RENDER_CHUNK; }
static int flat_local_chunk_z(int z) { return flat_z_index(z) / FLAT_RENDER_CHUNK; }
static int flat_section_y_for_world(int y) { return (y - FLAT_WORLD_Y_MIN) / FLAT_RENDER_SECTION; }

static int flat_local_chunk_valid(int lcx, int lcz) {
    return lcx >= 0 && lcx < FLAT_RENDER_CHUNKS && lcz >= 0 && lcz < FLAT_RENDER_CHUNKS;
}

static int flat_chunk_generated_at_block(int x, int z) {
    if (x < g_flat_world_origin_x || x >= g_flat_world_origin_x + FLAT_WORLD_SIZE ||
        z < g_flat_world_origin_z || z >= g_flat_world_origin_z + FLAT_WORLD_SIZE) return 0;
    int lcx = flat_local_chunk_x(x);
    int lcz = flat_local_chunk_z(z);
    return flat_local_chunk_valid(lcx, lcz) && g_flat_world_chunk_generated[lcz][lcx];
}

static int flat_player_columns_generated_at(float px, float pz) {
    float xs[3] = { px, px - 0.30f, px + 0.30f };
    float zs[3] = { pz, pz - 0.30f, pz + 0.30f };
    for (int iz = 0; iz < 3; iz++) {
        for (int ix = 0; ix < 3; ix++) {
            int x = (int)floorf(xs[ix]);
            int z = (int)floorf(zs[iz]);
            if (!flat_chunk_generated_at_block(x, z)) return 0;
        }
    }
    return 1;
}

static int flat_section_index_valid(int sy) {
    return sy >= 0 && sy < FLAT_RENDER_SECTIONS_Y;
}

static int flat_scan_section_nonempty(int lcx, int lcz, int sy) {
    if (!flat_local_chunk_valid(lcx, lcz) || !flat_section_index_valid(sy)) return 0;
    int x0 = lcx * FLAT_RENDER_CHUNK;
    int z0 = lcz * FLAT_RENDER_CHUNK;
    int y0 = FLAT_WORLD_Y_MIN + sy * FLAT_RENDER_SECTION;
    int y1 = y0 + FLAT_RENDER_SECTION;
    if (y1 > FLAT_WORLD_Y_MAX + 1) y1 = FLAT_WORLD_Y_MAX + 1;
    for (int y = y0; y < y1; y++) {
        int yi = flat_y_index(y);
        for (int z = z0; z < z0 + FLAT_RENDER_CHUNK; z++) {
            for (int x = x0; x < x0 + FLAT_RENDER_CHUNK; x++) {
                if (g_flat_blocks[yi][z][x] != 0) return 1;
            }
        }
    }
    return 0;
}

static int flat_refresh_section_occupancy(int lcx, int lcz, int sy) {
    if (!flat_local_chunk_valid(lcx, lcz) || !flat_section_index_valid(sy)) return 0;
    unsigned short bit = (unsigned short)(1u << sy);
    int nonempty = flat_scan_section_nonempty(lcx, lcz, sy);
    if (nonempty) g_flat_chunk_section_non_empty_mask[lcz][lcx] |= bit;
    else g_flat_chunk_section_non_empty_mask[lcz][lcx] &= (unsigned short)~bit;
    return nonempty;
}

static unsigned short flat_refresh_chunk_occupancy(int lcx, int lcz) {
    if (!flat_local_chunk_valid(lcx, lcz)) return 0;
    unsigned short mask = 0;
    for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
        if (flat_scan_section_nonempty(lcx, lcz, sy)) mask |= (unsigned short)(1u << sy);
    }
    g_flat_chunk_section_non_empty_mask[lcz][lcx] = mask;
    return mask;
}

static void flat_update_section_after_block_change(int x, int y, int z, int new_id) {
    if (!flat_in_bounds(x, y, z)) return;
    int lcx = flat_local_chunk_x(x);
    int lcz = flat_local_chunk_z(z);
    int sy = flat_section_y_for_world(y);
    if (!flat_local_chunk_valid(lcx, lcz) || !flat_section_index_valid(sy)) return;
    unsigned short bit = (unsigned short)(1u << sy);
    if (new_id != 0) {
        g_flat_chunk_section_non_empty_mask[lcz][lcx] |= bit;
    } else if (g_flat_chunk_section_non_empty_mask[lcz][lcx] & bit) {
        flat_refresh_section_occupancy(lcx, lcz, sy);
    }
}

static int flat_get_block(int x, int y, int z) {
    if (g_async_mesh_blocks) {
        if (x < g_async_mesh_x0 || y < g_async_mesh_y0 || z < g_async_mesh_z0 ||
            x >= g_async_mesh_x0 + g_async_mesh_w ||
            y >= g_async_mesh_y0 + g_async_mesh_h ||
            z >= g_async_mesh_z0 + g_async_mesh_d) return 0;
        int ix = x - g_async_mesh_x0;
        int iy = y - g_async_mesh_y0;
        int iz = z - g_async_mesh_z0;
        return g_async_mesh_blocks[((iy * g_async_mesh_d) + iz) * g_async_mesh_w + ix];
    }
    if (!flat_in_bounds(x, y, z)) return 0;
    return g_flat_blocks[flat_y_index(y)][flat_z_index(z)][flat_index(x)];
}

static int flat_get_meta(int x, int y, int z) {
    if (g_async_mesh_meta) {
        if (x < g_async_mesh_x0 || y < g_async_mesh_y0 || z < g_async_mesh_z0 ||
            x >= g_async_mesh_x0 + g_async_mesh_w ||
            y >= g_async_mesh_y0 + g_async_mesh_h ||
            z >= g_async_mesh_z0 + g_async_mesh_d) return 0;
        int ix = x - g_async_mesh_x0;
        int iy = y - g_async_mesh_y0;
        int iz = z - g_async_mesh_z0;
        return g_async_mesh_meta[((iy * g_async_mesh_d) + iz) * g_async_mesh_w + ix];
    }
    if (!flat_in_bounds(x, y, z)) return 0;
    return g_flat_meta[flat_y_index(y)][flat_z_index(z)][flat_index(x)];
}

/* Fluid level metadata (Beta): 0 source, 1-7 flow decay, bit 0x8 = falling.
   g_flat_meta mirrors this only for liquid blocks so existing chunk-delta saves
   preserve fluid state without losing door/stairs/chest metadata. */
#define FLUID_LEVEL_MAX 7
#define FLUID_FALLING 0x8
static int block_is_liquid(int id);
static int block_is_water(int id);
static int block_is_lava(int id);
static int block_is_flowing_liquid(int id);
static int flat_get_level(int x, int y, int z) {
    if (g_async_mesh_levels) {
        if (x < g_async_mesh_x0 || y < g_async_mesh_y0 || z < g_async_mesh_z0 ||
            x >= g_async_mesh_x0 + g_async_mesh_w ||
            y >= g_async_mesh_y0 + g_async_mesh_h ||
            z >= g_async_mesh_z0 + g_async_mesh_d) return 0;
        int ix = x - g_async_mesh_x0;
        int iy = y - g_async_mesh_y0;
        int iz = z - g_async_mesh_z0;
        return g_async_mesh_levels[((iy * g_async_mesh_d) + iz) * g_async_mesh_w + ix];
    }
    if (!flat_in_bounds(x, y, z)) return 0;
    return g_flat_levels[flat_y_index(y)][flat_z_index(z)][flat_index(x)];
}


static int flat_get_sky_light(int x, int y, int z) {
    if (g_async_mesh_sky_light) {
        if (x < g_async_mesh_x0 || y < g_async_mesh_y0 || z < g_async_mesh_z0 ||
            x >= g_async_mesh_x0 + g_async_mesh_w ||
            y >= g_async_mesh_y0 + g_async_mesh_h ||
            z >= g_async_mesh_z0 + g_async_mesh_d) return 15;
        int ix = x - g_async_mesh_x0;
        int iy = y - g_async_mesh_y0;
        int iz = z - g_async_mesh_z0;
        return g_async_mesh_sky_light[((iy * g_async_mesh_d) + iz) * g_async_mesh_w + ix] & 15;
    }
    if (y < FLAT_WORLD_Y_MIN) return 0;
    if (y > FLAT_WORLD_Y_MAX) return 15;
    if (!flat_in_bounds(x, y, z)) return 15;
    return g_flat_sky_light[flat_y_index(y)][flat_z_index(z)][flat_index(x)] & 15;
}

static int flat_get_block_light(int x, int y, int z) {
    if (g_async_mesh_block_light) {
        if (x < g_async_mesh_x0 || y < g_async_mesh_y0 || z < g_async_mesh_z0 ||
            x >= g_async_mesh_x0 + g_async_mesh_w ||
            y >= g_async_mesh_y0 + g_async_mesh_h ||
            z >= g_async_mesh_z0 + g_async_mesh_d) return 0;
        int ix = x - g_async_mesh_x0;
        int iy = y - g_async_mesh_y0;
        int iz = z - g_async_mesh_z0;
        return g_async_mesh_block_light[((iy * g_async_mesh_d) + iz) * g_async_mesh_w + ix] & 15;
    }
    if (!flat_in_bounds(x, y, z)) return 0;
    return g_flat_block_light[flat_y_index(y)][flat_z_index(z)][flat_index(x)] & 15;
}

static int flat_light_opacity_for_id(int id) {
    /* Mirrors Java 1.2.5 Block.lightOpacity[] for the block ids this C port has.
       This is intentionally separate from collision/render occlusion: glass does
       not block light, leaves block one level, and water/ice block three levels. */
    if (id == 0) return 0;
    if (id == BLOCK_WATER || id == BLOCK_STILL_WATER || id == BLOCK_ICE) return 3;
    if (id == BLOCK_LEAVES || id == BLOCK_WEB) return 1;
    if (id == BLOCK_GLASS || id == BLOCK_GLASS_PANE || id == BLOCK_IRON_BARS ||
        id == BLOCK_SNOW_LAYER || id == BLOCK_END_PORTAL || id == BLOCK_END_PORTAL_FRAME) return 0;
    if (id == BLOCK_SAPLING || id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE ||
        id == BLOCK_BROWN_MUSHROOM || id == BLOCK_RED_MUSHROOM || id == BLOCK_TORCH ||
        id == BLOCK_FIRE || id == BLOCK_REDSTONE_WIRE || id == BLOCK_REDSTONE_TORCH_OFF ||
        id == BLOCK_REDSTONE_TORCH_ON || id == BLOCK_REEDS || id == BLOCK_LADDER ||
        id == BLOCK_RAILS || id == BLOCK_SIGN_POST || id == BLOCK_WALL_SIGN ||
        id == BLOCK_LEVER || id == BLOCK_STONE_BUTTON || id == BLOCK_WOOD_DOOR ||
        id == BLOCK_IRON_DOOR || id == BLOCK_PORTAL || id == BLOCK_CROPS ||
        id == BLOCK_TALL_GRASS || id == BLOCK_DEAD_BUSH || id == BLOCK_VINE ||
        id == BLOCK_LILY_PAD || id == BLOCK_NETHER_WART || id == BLOCK_PUMPKIN_STEM ||
        id == BLOCK_MELON_STEM || id == BLOCK_STONE_PRESSURE_PLATE ||
        id == BLOCK_WOOD_PRESSURE_PLATE || id == BLOCK_CHEST || id == BLOCK_FENCE ||
        id == BLOCK_NETHER_BRICK_FENCE || id == BLOCK_FENCE_GATE || id == BLOCK_CACTUS) return 0;
    /* Java slabs/stairs/farmland still have lightOpacity 255; they get smooth
       visible light from Block.useNeighborBrightness instead of opacity hacks. */
    return 255;
}

static int flat_block_light_value_for_id(int id) {
    /* Mirrors Java 1.2.5 Block.lightValue[] = (int)(15.0F * setLightValue()). */
    switch (id) {
        case BLOCK_LAVA:
        case BLOCK_STILL_LAVA:
        case BLOCK_FIRE:
        case BLOCK_GLOWSTONE:
        case BLOCK_JACK_O_LANTERN:
        case BLOCK_LOCKED_CHEST:
        case BLOCK_END_PORTAL:
        case BLOCK_REDSTONE_LAMP_ON:
            return 15;
        case BLOCK_TORCH:
            return 14; /* 15/16 */
        case BLOCK_FURNACE_LIT:
            return 13; /* 14/16 */
        case BLOCK_REDSTONE_TORCH_ON:
            return 7;  /* 1/2 */
        case BLOCK_REDSTONE_ORE_GLOWING:
        case BLOCK_REDSTONE_REPEATER_ON:
            return 9;  /* 10/16 */
        case BLOCK_PORTAL:
            return 11; /* 12/16 */
        case BLOCK_BROWN_MUSHROOM:
        case BLOCK_BREWING_STAND:
        case BLOCK_END_PORTAL_FRAME:
        case BLOCK_DRAGON_EGG:
            return 1;  /* 2/16 */
        default:
            return 0;
    }
}

static int flat_sky_light_after_block(int incoming, int id) {
    if (incoming <= 0) return 0;
    if (incoming > 15) incoming = 15;
    int opacity = flat_light_opacity_for_id(id);
    if (opacity <= 0) return incoming;
    if (opacity >= 255) return 0;
    incoming -= opacity;
    if (incoming < 0) incoming = 0;
    return incoming;
}

static float flat_light_level_brightness_base(int level, float base) {
    if (level < 0) level = 0;
    if (level > 15) level = 15;
    /* WorldProvider.generateLightBrightnessTable(); base=0.0 overworld, 0.1 nether. */
    float inv = 1.0f - (float)level / 15.0f;
    return (1.0f - inv) / (inv * 3.0f + 1.0f) * (1.0f - base) + base;
}

static float flat_light_table_base(void) {
    /* This client only has an overworld-like provider today.  Keeping the base
       as a function makes the Nether 0.1F table a one-line change if dimensions
       are added later. */
    return 0.0f;
}

static float flat_light_level_brightness(int level) {
    return flat_light_level_brightness_base(level, flat_light_table_base());
}
static float flat_light_sun_angle(void) {
    int t = (int)(g_world_time % 24000LL);
    if (t < 0) t += 24000;
    float a = ((float)t + 1.0f) / 24000.0f - 0.25f;
    if (a < 0.0f) a += 1.0f;
    if (a > 1.0f) a -= 1.0f;
    {
        float raw = a;
        a = 1.0f - (float)((cos((double)a * M_PI) + 1.0) / 2.0);
        a = raw + (a - raw) / 3.0f;
    }
    return a;
}

static int flat_skylight_subtracted(void) {
    /* Java 1.2.5 World.calculateSkylightSubtracted(), with rain/thunder = 0. */
    float a = flat_light_sun_angle();
    float v = 1.0f - (cosf(a * (float)M_PI * 2.0f) * 2.0f + 0.5f);
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    v = 1.0f - v;
    v = 1.0f - v;
    return (int)(v * 11.0f);
}


static int flat_block_can_block_grass_java(int id) {
    /* Java Block.canBlockGrass[] is true for blocks whose Material does NOT block
       grass (air/plants/circuits/snow/fire/portal), and false for normal solids,
       glass, water, leaves, cactus, doors, etc.  RenderBlocks uses it to decide
       whether diagonal AO corner samples are valid. */
    if (id == 0) return 1;
    if (id == BLOCK_SAPLING || id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE ||
        id == BLOCK_BROWN_MUSHROOM || id == BLOCK_RED_MUSHROOM || id == BLOCK_TORCH ||
        id == BLOCK_FIRE || id == BLOCK_REDSTONE_WIRE || id == BLOCK_REDSTONE_TORCH_OFF ||
        id == BLOCK_REDSTONE_TORCH_ON || id == BLOCK_REEDS || id == BLOCK_LADDER ||
        id == BLOCK_RAILS || id == BLOCK_SIGN_POST || id == BLOCK_WALL_SIGN ||
        id == BLOCK_LEVER || id == BLOCK_STONE_BUTTON || id == BLOCK_SNOW_LAYER ||
        id == BLOCK_PORTAL || id == BLOCK_CROPS || id == BLOCK_STONE_PRESSURE_PLATE ||
        id == BLOCK_WOOD_PRESSURE_PLATE) return 1;
    return 0;
}

static int flat_uses_neighbor_brightness(int id) {
    if (id == 0) return 0; /* Java Block.useNeighborBrightness[0] remains false. */
    if (id == BLOCK_SLAB || id == BLOCK_FARMLAND ||
        id == BLOCK_WOOD_STAIRS || id == BLOCK_COBBLE_STAIRS) return 1;
    if (flat_block_can_block_grass_java(id)) return 1;
    return 0;
}

static int flat_saved_sky_light(int x, int y, int z) {
    return flat_get_sky_light(x, y, z);
}

static int flat_saved_block_light(int x, int y, int z) {
    return flat_get_block_light(x, y, z);
}

static int flat_sample_sky_light(int x, int y, int z, int use_neighbor) {
    if (!use_neighbor || !flat_uses_neighbor_brightness(flat_get_block(x, y, z))) {
        return flat_saved_sky_light(x, y, z);
    }
    int best = flat_saved_sky_light(x, y + 1, z);
    int v = flat_saved_sky_light(x + 1, y, z); if (v > best) best = v;
    v = flat_saved_sky_light(x - 1, y, z); if (v > best) best = v;
    v = flat_saved_sky_light(x, y, z + 1); if (v > best) best = v;
    v = flat_saved_sky_light(x, y, z - 1); if (v > best) best = v;
    return best;
}

static int flat_sample_block_light(int x, int y, int z, int use_neighbor) {
    if (!use_neighbor || !flat_uses_neighbor_brightness(flat_get_block(x, y, z))) {
        return flat_saved_block_light(x, y, z);
    }
    int best = flat_saved_block_light(x, y + 1, z);
    int v = flat_saved_block_light(x + 1, y, z); if (v > best) best = v;
    v = flat_saved_block_light(x - 1, y, z); if (v > best) best = v;
    v = flat_saved_block_light(x, y, z + 1); if (v > best) best = v;
    v = flat_saved_block_light(x, y, z - 1); if (v > best) best = v;
    return best;
}

static int flat_pack_sky_block_light(int x, int y, int z, int min_block_light) {
    /* Java 1.2.5 World.getLightBrightnessForSkyBlocks(): pack saved sky and
       block light independently.  Day/night darkening belongs in the lightmap
       (`EntityRenderer.updateLightmap`), not by destructively subtracting from
       the packed sky channel. */
    int sky = flat_sample_sky_light(x, y, z, 1);
    int block = flat_sample_block_light(x, y, z, 1);
    if (block < min_block_light) block = min_block_light;
    if (sky < 0) sky = 0;
    if (sky > 15) sky = 15;
    if (block < 0) block = 0;
    if (block > 15) block = 15;
    return (sky << 20) | (block << 4);
}

static int flat_daylight_adjusted_light(int x, int y, int z, int min_block_light) {
    /* Java World/Chunk.getBlockLightValue(..., skylightSubtracted) path used for
       gameplay tests such as passive-mob spawning: sky is reduced by current
       skylightSubtracted, then compared against block light. */
    int sky = flat_sample_sky_light(x, y, z, 1) - flat_skylight_subtracted();
    int block = flat_sample_block_light(x, y, z, 1);
    if (block < min_block_light) block = min_block_light;
    if (sky < 0) sky = 0;
    if (sky > 15) sky = 15;
    if (block < 0) block = 0;
    if (block > 15) block = 15;
    return (sky > block) ? sky : block;
}

static int flat_block_mixed_brightness(int id, int x, int y, int z) {
    return flat_pack_sky_block_light(x, y, z, flat_block_light_value_for_id(id));
}

static int flat_combined_light(int x, int y, int z) {
    return flat_daylight_adjusted_light(x, y, z, 0);
}

static float flat_light_sun_brightness(float partial) {
    /* World.func_35464_b(float), with rain/thunder/lightning omitted because this
       port does not yet implement those weather systems. */
    float a = flat_light_sun_angle();
    (void)partial;
    float v = 1.0f - (cosf(a * (float)M_PI * 2.0f) * 2.0f + 0.2f);
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    v = 1.0f - v;
    return v * 0.8f + 0.2f;
}

static float world_dark_overlay_alpha(void) {
    /* Debug/overlay helper shared by GUI and platform unity builds.  It follows
       the same Java 1.2.5 sun-brightness curve used by the CPU lightmap, so the
       value is 0.0 in full daylight and rises smoothly toward night darkness.
       Because Java's overworld sun brightness bottoms out at 0.2F, the maximum
       dark overlay is 0.8 rather than full black. */
    float alpha = 1.0f - flat_light_sun_brightness(1.0f);
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    return alpha;
}

static void flat_lightmap_color(int packed, float *r, float *g, float *b) {
    /* CPU-side equivalent of EntityRenderer.updateLightmap() for the normal
       overworld, gamma=0, torchFlickerX=0, no lightning.  Java applies this as
       a 16x16 lightmap texture every frame.  This renderer still bakes the
       current lightmap result into section vertex colors, so time-light changes
       dirty section colors without invalidating the old drawable mesh. */
    int sky = (packed >> 20) & 15;
    int block = (packed >> 4) & 15;
    float sun = g_flat_bake_stable_mesh_light ? 1.0f : flat_light_sun_brightness(1.0f);
    float light_sky = flat_light_level_brightness(sky) * (sun * 0.95f + 0.05f);
    float light_block = flat_light_level_brightness(block) * 1.5f;
    float sky_rg = light_sky * (sun * 0.65f + 0.35f);
    float torch_g = light_block * ((light_block * 0.6f + 0.4f) * 0.6f + 0.4f);
    float torch_b = light_block * (light_block * light_block * 0.6f + 0.4f);
    float rr = sky_rg + light_block;
    float gg = sky_rg + torch_g;
    float bb = light_sky + torch_b;
    rr = rr * 0.96f + 0.03f;
    gg = gg * 0.96f + 0.03f;
    bb = bb * 0.96f + 0.03f;
    if (rr > 1.0f) rr = 1.0f; if (gg > 1.0f) gg = 1.0f; if (bb > 1.0f) bb = 1.0f;
    if (rr < 0.0f) rr = 0.0f; if (gg < 0.0f) gg = 0.0f; if (bb < 0.0f) bb = 0.0f;
    rr = rr * 0.96f + 0.03f;
    gg = gg * 0.96f + 0.03f;
    bb = bb * 0.96f + 0.03f;
    if (rr > 1.0f) rr = 1.0f; if (gg > 1.0f) gg = 1.0f; if (bb > 1.0f) bb = 1.0f;
    *r = rr; *g = gg; *b = bb;
}

static float flat_packed_light_brightness(int packed) {
    float r, g, b;
    flat_lightmap_color(packed, &r, &g, &b);
    /* Scalar fallback for AO/particles where this renderer still has a single
       brightness channel.  Weighted luminance keeps night closer to Java's blue
       lightmap than max(sky, block) did. */
    return r * 0.30f + g * 0.59f + b * 0.11f;
}

static void flat_light_color(int x, int y, int z, float *r, float *g, float *b) {
    flat_lightmap_color(flat_pack_sky_block_light(x, y, z, 0), r, g, b);
}

static float flat_light_brightness(int x, int y, int z) {
    return flat_packed_light_brightness(flat_pack_sky_block_light(x, y, z, 0));
}

typedef struct FlatLightQueueCell { short x, y, z; } FlatLightQueueCell;

static void flat_light_enqueue(FlatLightQueueCell *q, int *tail, int cap, int x, int y, int z,
                               int x0, int y0, int z0) {
    if (*tail >= cap) return;
    q[*tail].x = (short)(x - x0);
    q[*tail].y = (short)(y - y0);
    q[*tail].z = (short)(z - z0);
    (*tail)++;
}

static void flat_propagate_light_region(unsigned char *arr, int x0, int y0, int z0, int x1, int y1, int z1,
                                        FlatLightQueueCell *q, int head, int tail) {
    int w = x1 - x0 + 1;
    int h = y1 - y0 + 1;
    int d = z1 - z0 + 1;
    const int dx[6] = { 1, -1, 0, 0, 0, 0 };
    const int dy[6] = { 0, 0, 1, -1, 0, 0 };
    const int dz[6] = { 0, 0, 0, 0, 1, -1 };
    while (head < tail) {
        int lx = q[head].x, ly = q[head].y, lz = q[head].z;
        head++;
        int idx = ((ly * d) + lz) * w + lx;
        int src = arr[idx] & 15;
        if (src <= 1) continue;
        int wx = x0 + lx, wy = y0 + ly, wz = z0 + lz;
        for (int i = 0; i < 6; ++i) {
            int nx = wx + dx[i], ny = wy + dy[i], nz = wz + dz[i];
            if (nx < x0 || nx > x1 || ny < y0 || ny > y1 || nz < z0 || nz > z1) continue;
            int opacity = flat_light_opacity_for_id(flat_get_block(nx, ny, nz));
            if (opacity < 1) opacity = 1;
            int nv = src - opacity;
            if (nv <= 0) continue;
            int nlx = nx - x0, nly = ny - y0, nlz = nz - z0;
            int nidx = ((nly * d) + nlz) * w + nlx;
            if (nv > (arr[nidx] & 15)) {
                arr[nidx] = (unsigned char)nv;
                if (tail < w * h * d) {
                    q[tail].x = (short)nlx;
                    q[tail].y = (short)nly;
                    q[tail].z = (short)nlz;
                    tail++;
                }
            }
        }
    }
}

static void flat_relight_region(int rx0, int rz0, int rx1, int rz1, int margin, int propagate_sky) {
    if (rx1 < rx0) { int t = rx0; rx0 = rx1; rx1 = t; }
    if (rz1 < rz0) { int t = rz0; rz0 = rz1; rz1 = t; }
    if (margin < 0) margin = 0;
    if (margin > 16) margin = 16;
    int x0 = rx0 - margin, x1 = rx1 + margin;
    int z0 = rz0 - margin, z1 = rz1 + margin;
    if (x0 < g_flat_world_origin_x) x0 = g_flat_world_origin_x;
    if (z0 < g_flat_world_origin_z) z0 = g_flat_world_origin_z;
    if (x1 >= g_flat_world_origin_x + FLAT_WORLD_SIZE) x1 = g_flat_world_origin_x + FLAT_WORLD_SIZE - 1;
    if (z1 >= g_flat_world_origin_z + FLAT_WORLD_SIZE) z1 = g_flat_world_origin_z + FLAT_WORLD_SIZE - 1;
    int y0 = FLAT_WORLD_Y_MIN, y1 = FLAT_WORLD_Y_MAX;
    if (x1 < x0 || z1 < z0) return;
    int w = x1 - x0 + 1;
    int h = y1 - y0 + 1;
    int d = z1 - z0 + 1;
    int cap = w * h * d;
    unsigned char *sky = (unsigned char*)calloc((size_t)cap, 1);
    unsigned char *block = (unsigned char*)calloc((size_t)cap, 1);
    FlatLightQueueCell *q = (FlatLightQueueCell*)malloc(sizeof(FlatLightQueueCell) * (size_t)cap);
    if (!sky || !block || !q) {
        pex_logf("lighting region failed alloc x=%d..%d z=%d..%d cap=%d", x0, x1, z0, z1, cap);
        free(sky); free(block); free(q); return;
    }

    unsigned char changed_sections[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    unsigned char changed_chunks[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    memset(changed_sections, 0, sizeof(changed_sections));
    memset(changed_chunks, 0, sizeof(changed_chunks));
    int changed_section_count = 0;

    int tail = 0;
    for (int z = z0; z <= z1; ++z) {
        for (int x = x0; x <= x1; ++x) {
            int light = 15;
            for (int y = y1; y >= y0; --y) {
                int idx = (((y - y0) * d) + (z - z0)) * w + (x - x0);
                int bid = flat_get_block(x, y, z);
                light = flat_sky_light_after_block(light, bid);
                sky[idx] = (unsigned char)light;
            }
        }
    }
    if (propagate_sky) {
        tail = 0;
        for (int z = z0; z <= z1; ++z) {
            for (int x = x0; x <= x1; ++x) {
                for (int y = y0; y <= y1; ++y) {
                    int idx = (((y - y0) * d) + (z - z0)) * w + (x - x0);
                    int v = sky[idx] & 15;
                    if (v <= 1) continue;
                    int bid = flat_get_block(x, y, z);
                    if (flat_light_opacity_for_id(bid) >= 255) continue;
                    flat_light_enqueue(q, &tail, cap, x, y, z, x0, y0, z0);
                }
            }
        }
        if (tail > 0) flat_propagate_light_region(sky, x0, y0, z0, x1, y1, z1, q, 0, tail);
    }

    tail = 0;
    for (int z = z0; z <= z1; ++z) {
        for (int x = x0; x <= x1; ++x) {
            for (int y = y0; y <= y1; ++y) {
                int v = flat_block_light_value_for_id(flat_get_block(x, y, z));
                if (v <= 0) continue;
                int idx = (((y - y0) * d) + (z - z0)) * w + (x - x0);
                block[idx] = (unsigned char)v;
                flat_light_enqueue(q, &tail, cap, x, y, z, x0, y0, z0);
            }
        }
    }
    if (tail > 0) flat_propagate_light_region(block, x0, y0, z0, x1, y1, z1, q, 0, tail);

    for (int z = z0; z <= z1; ++z) {
        int fz = flat_z_index(z);
        int lcz = fz / FLAT_RENDER_CHUNK;
        for (int x = x0; x <= x1; ++x) {
            int fx = flat_index(x);
            int lcx = fx / FLAT_RENDER_CHUNK;
            for (int y = y0; y <= y1; ++y) {
                int yi = flat_y_index(y);
                int idx = (((y - y0) * d) + (z - z0)) * w + (x - x0);
                unsigned char new_sky = (unsigned char)(sky[idx] & 15);
                unsigned char new_block = (unsigned char)(block[idx] & 15);
                if ((g_flat_sky_light[yi][fz][fx] & 15) != new_sky ||
                    (g_flat_block_light[yi][fz][fx] & 15) != new_block) {
                    int lsy = yi / FLAT_RENDER_SECTION;
                    if (flat_section_index_valid(lsy) && flat_local_chunk_valid(lcx, lcz) &&
                        !changed_sections[lsy][lcz][lcx]) {
                        changed_sections[lsy][lcz][lcx] = 1;
                        changed_chunks[lcz][lcx] = 1;
                        changed_section_count++;
                    }
                    g_flat_sky_light[yi][fz][fx] = new_sky;
                    g_flat_block_light[yi][fz][fx] = new_block;
                }
            }
        }
    }
    for (int lcz = 0; lcz < FLAT_RENDER_CHUNKS; ++lcz) {
        for (int lcx = 0; lcx < FLAT_RENDER_CHUNKS; ++lcx) {
            if (changed_chunks[lcz][lcx]) {
                g_flat_chunk_light_ready[lcz][lcx] = g_flat_world_chunk_generated[lcz][lcx] ? 1 : 0;
                flat_bump_light_version(lcx, lcz);
            }
        }
    }
    for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; ++sy) {
        for (int lcz = 0; lcz < FLAT_RENDER_CHUNKS; ++lcz) {
            for (int lcx = 0; lcx < FLAT_RENDER_CHUNKS; ++lcx) {
                if (changed_sections[sy][lcz][lcx]) flat_mark_section_dirty(lcx, lcz, sy);
            }
        }
    }
    int cx0 = floor_div16(x0), cx1 = floor_div16(x1);
    int cz0 = floor_div16(z0), cz1 = floor_div16(z1);
    pex_logf_trace("lighting region recalculated x=%d..%d z=%d..%d chunks=%d..%d,%d..%d sky_spread=%d changed_sections=%d", x0, x1, z0, z1, cx0, cx1, cz0, cz1, propagate_sky, changed_section_count);
    (void)changed_section_count;
    free(sky); free(block); free(q);
}

static void flat_recalculate_lighting_region(int rx0, int rz0, int rx1, int rz1) {
    /* The caller already widens light-source/opacity edits to the Java 15-block
       influence radius.  Do not add another 16-block compute margin here: that
       was the main reason one block break expanded into a ~63x63xheight rebuild. */
    flat_relight_region(rx0, rz0, rx1, rz1, 0, 1);
}

#ifndef FLAT_CHUNK_BLOCK_COUNT
#define FLAT_CHUNK_BLOCK_COUNT (FLAT_RENDER_CHUNK * FLAT_RENDER_CHUNK * FLAT_WORLD_HEIGHT)
#endif
static int flat_chunk_buf_index(int lx, int y, int lz);
static void flat_mark_light_dirty_region(int x0, int z0, int x1, int z1);

static void flat_relight_fast_surface_chunk(int cx, int cz) {
    int x0 = cx * 16, z0 = cz * 16;
    int x1 = x0 + 15, z1 = z0 + 15;
    if (x0 < g_flat_world_origin_x) x0 = g_flat_world_origin_x;
    if (z0 < g_flat_world_origin_z) z0 = g_flat_world_origin_z;
    if (x1 >= g_flat_world_origin_x + FLAT_WORLD_SIZE) x1 = g_flat_world_origin_x + FLAT_WORLD_SIZE - 1;
    if (z1 >= g_flat_world_origin_z + FLAT_WORLD_SIZE) z1 = g_flat_world_origin_z + FLAT_WORLD_SIZE - 1;
    for (int z = z0; z <= z1; ++z) {
        int fz = flat_z_index(z);
        for (int x = x0; x <= x1; ++x) {
            int fx = flat_index(x);
            int light = 15;
            for (int y = FLAT_WORLD_Y_MAX; y >= FLAT_WORLD_Y_MIN; --y) {
                int yi = flat_y_index(y);
                int id = g_flat_blocks[yi][fz][fx];
                light = flat_sky_light_after_block(light, id);
                g_flat_sky_light[yi][fz][fx] = (unsigned char)(light & 15);
                g_flat_block_light[yi][fz][fx] = (unsigned char)(flat_block_light_value_for_id(id) & 15);
            }
        }
    }
    int lcx = stream_local_chunk_x(cx);
    int lcz = stream_local_chunk_z(cz);
    if (flat_local_chunk_valid(lcx, lcz)) {
        flat_publish_light_ready(lcx, lcz, 1, 1);
    }
    pex_logf("lighting fast surface chunk=%d,%d local=%d,%d", cx, cz, lcx, lcz);
}


static void flat_compute_chunk_light(const unsigned char *buf, unsigned char *sky, unsigned char *block) {
    if (!buf || !sky || !block) return;
    for (int lx = 0; lx < 16; lx++) {
        for (int lz = 0; lz < 16; lz++) {
            int light = 15;
            for (int y = FLAT_WORLD_Y_MAX; y >= FLAT_WORLD_Y_MIN; --y) {
                int bi = flat_chunk_buf_index(lx, y, lz);
                int id = buf[bi];
                light = flat_sky_light_after_block(light, id);
                sky[bi] = (unsigned char)(light & 15);
                block[bi] = (unsigned char)(flat_block_light_value_for_id(id) & 15);
            }
        }
    }
}

static int flat_id_is_light_source(int id) {
    return id == BLOCK_TORCH || id == BLOCK_FURNACE_LIT ||
           id == BLOCK_REDSTONE_TORCH_ON || id == BLOCK_FIRE ||
           id == BLOCK_GLOWSTONE || id == BLOCK_JACK_O_LANTERN ||
           id == BLOCK_PORTAL;
}

static int flat_chunk_buffer_has_light_source(const unsigned char *buf) {
    if (!buf) return 0;
    for (int lx = 0; lx < 16; lx++) {
        for (int lz = 0; lz < 16; lz++) {
            for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
                if (flat_id_is_light_source(buf[flat_chunk_buf_index(lx, y, lz)])) return 1;
            }
        }
    }
    return 0;
}

static void flat_mark_chunk_block_light_dirty(int cx, int cz) {
    int x0 = cx * 16;
    int z0 = cz * 16;
    flat_mark_light_dirty_region(x0 - 15, z0 - 15, x0 + 30, z0 + 30);
}

static void flat_copy_light_buffers_to_world(int cx, int cz, const unsigned char *sky, const unsigned char *block) {
    if (!sky || !block) return;
    for (int lx = 0; lx < 16; lx++) {
        int wx = cx * 16 + lx;
        if (wx < g_flat_world_origin_x || wx >= g_flat_world_origin_x + FLAT_WORLD_SIZE) continue;
        int fx = flat_index(wx);
        for (int lz = 0; lz < 16; lz++) {
            int wz = cz * 16 + lz;
            if (wz < g_flat_world_origin_z || wz >= g_flat_world_origin_z + FLAT_WORLD_SIZE) continue;
            int fz = flat_z_index(wz);
            for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
                int bi = flat_chunk_buf_index(lx, y, lz);
                int yi = flat_y_index(y);
                g_flat_sky_light[yi][fz][fx] = sky[bi] & 15;
                g_flat_block_light[yi][fz][fx] = block[bi] & 15;
            }
        }
    }
    int lcx = stream_local_chunk_x(cx);
    int lcz = stream_local_chunk_z(cz);
    if (flat_local_chunk_valid(lcx, lcz)) {
        flat_publish_light_ready(lcx, lcz, 0, 1);
    }
}

static int g_flat_chunk_environment_light_due[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];

static void flat_reset_sky_light(int lcx, int lcz) {
    if (!flat_local_chunk_valid(lcx, lcz)) return;
    flat_publish_light_ready(lcx, lcz, 0, 0);
    int x0 = lcx * FLAT_RENDER_CHUNK;
    int z0 = lcz * FLAT_RENDER_CHUNK;
    for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; ++y) {
        int yi = flat_y_index(y);
        for (int lz = 0; lz < FLAT_RENDER_CHUNK; ++lz) {
            int fz = z0 + lz;
            for (int lx = 0; lx < FLAT_RENDER_CHUNK; ++lx) {
                int fx = x0 + lx;
                int id = g_flat_blocks[yi][fz][fx];
                g_flat_sky_light[yi][fz][fx] = 15;
                g_flat_block_light[yi][fz][fx] = (unsigned char)(flat_block_light_value_for_id(id) & 15);
            }
        }
    }
}

static void flat_apply_environment_light(int lcx, int lcz) {
    if (!flat_local_chunk_valid(lcx, lcz)) return;
    if (!g_flat_world_chunk_generated[lcz][lcx]) return;
    int wcx = floor_div16(g_flat_world_origin_x) + lcx;
    int wcz = floor_div16(g_flat_world_origin_z) + lcz;
    pex_logf_trace("lighting immediate environment pass local=%d,%d world=%d,%d", lcx, lcz, wcx, wcz);
    flat_relight_region(wcx * 16, wcz * 16, wcx * 16 + 15, wcz * 16 + 15, 16, 1);
}

static int flat_sky_light_needs_rebuild(int lcx, int lcz) {
    if (!flat_local_chunk_valid(lcx, lcz)) return 0;
    if (!g_flat_world_chunk_generated[lcz][lcx]) return 0;
    int x0 = g_flat_world_origin_x + lcx * FLAT_RENDER_CHUNK;
    int z0 = g_flat_world_origin_z + lcz * FLAT_RENDER_CHUNK;

    /* A shifted active window used to move blocks without moving their light
       arrays.  That left whole generated chunks with zero skylight until the
       player edited a block.  Sample the chunk against the same cheap column
       skylight model used by the streamer and rebuild only chunks that are
       visibly impossible: open-sky cells carrying near-black skylight. */
    for (int lz = 0; lz < FLAT_RENDER_CHUNK; lz += 3) {
        int wz = z0 + lz;
        int fz = flat_z_index(wz);
        for (int lx = 0; lx < FLAT_RENDER_CHUNK; lx += 3) {
            int wx = x0 + lx;
            int fx = flat_index(wx);
            int expected = 15;
            for (int y = FLAT_WORLD_Y_MAX; y >= FLAT_WORLD_Y_MIN; --y) {
                int yi = flat_y_index(y);
                int id = g_flat_blocks[yi][fz][fx];
                int have = g_flat_sky_light[yi][fz][fx] & 15;
                if (expected >= 12 && have <= 1 && flat_light_opacity_for_id(id) < 255) return 1;
                int opacity = flat_light_opacity_for_id(id);
                if (opacity > 0 && expected > 0) {
                    if (id == BLOCK_LEAVES) {
                        if (expected > 8) expected -= 1;
                    } else if (opacity >= 255) {
                        expected = 0;
                    } else {
                        expected -= opacity;
                        if (expected < 0) expected = 0;
                    }
                }
            }
        }
    }
    return 0;
}

static int flat_repair_missing_light(int lcx, int lcz) {
    if (!flat_sky_light_needs_rebuild(lcx, lcz)) return 0;
    int wcx = floor_div16(g_flat_world_origin_x) + lcx;
    int wcz = floor_div16(g_flat_world_origin_z) + lcz;
    pex_logf("lighting repaired missing skylight chunk local=%d,%d world=%d,%d", lcx, lcz, wcx, wcz);
    flat_relight_fast_surface_chunk(wcx, wcz);
    return 1;
}

static void flat_relight_chunk(int cx, int cz) {
    flat_recalculate_lighting_region(cx * 16, cz * 16, cx * 16 + 15, cz * 16 + 15);
}


#define FLAT_JAVA_LIGHT_QUEUE_CAP 32768
#define FLAT_JAVA_LIGHT_KIND_SKY 0
#define FLAT_JAVA_LIGHT_KIND_BLOCK 1

static int flat_light_chunks_ready_near(int x, int y, int z, int radius) {
    (void)y;
    int cx0 = floor_div16(x - radius);
    int cx1 = floor_div16(x + radius);
    int cz0 = floor_div16(z - radius);
    int cz1 = floor_div16(z + radius);
    for (int cz = cz0; cz <= cz1; ++cz) {
        for (int cx = cx0; cx <= cx1; ++cx) {
            int lcx = stream_local_chunk_x(cx);
            int lcz = stream_local_chunk_z(cz);
            if (!flat_local_chunk_valid(lcx, lcz)) return 0;
            if (!g_flat_world_chunk_generated[lcz][lcx]) return 0;
        }
    }
    return 1;
}

static int flat_light_saved_value_kind(int kind, int x, int y, int z) {
    return (kind == FLAT_JAVA_LIGHT_KIND_SKY) ?
        flat_saved_sky_light(x, y, z) :
        flat_saved_block_light(x, y, z);
}

static void flat_mark_light_cell_changed(int x, int y, int z) {
    if (!flat_in_bounds(x, y, z)) return;
    int fx = flat_index(x);
    int fz = flat_z_index(z);
    int lcx = fx / FLAT_RENDER_CHUNK;
    int lcz = fz / FLAT_RENDER_CHUNK;
    int sy = (y - FLAT_WORLD_Y_MIN) / FLAT_RENDER_SECTION;
    if (flat_local_chunk_valid(lcx, lcz)) {
        g_flat_chunk_light_ready[lcz][lcx] = g_flat_world_chunk_generated[lcz][lcx] ? 1 : 0;
        flat_bump_light_version(lcx, lcz);
    }
    flat_mark_section_dirty(lcx, lcz, sy);
}

static void flat_set_light_value_kind(int kind, int x, int y, int z, int value) {
    if (value < 0) value = 0;
    if (value > 15) value = 15;
    if (!flat_in_bounds(x, y, z)) return;
    int yi = flat_y_index(y);
    int zi = flat_z_index(z);
    int xi = flat_index(x);
    unsigned char *dst = (kind == FLAT_JAVA_LIGHT_KIND_SKY) ?
        &g_flat_sky_light[yi][zi][xi] :
        &g_flat_block_light[yi][zi][xi];
    if ((*dst & 15) == value) return;
    *dst = (unsigned char)value;
    flat_mark_light_cell_changed(x, y, z);
}

static int flat_can_block_see_sky_slow(int x, int y, int z) {
    if (!flat_in_bounds(x, y, z)) return 1;
    for (int yy = y + 1; yy <= FLAT_WORLD_Y_MAX; ++yy) {
        if (flat_light_opacity_for_id(flat_get_block(x, yy, z)) >= 255) return 0;
    }
    return 1;
}

static int flat_compute_light_value_kind(int kind, int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    int opacity = flat_light_opacity_for_id(id);
    if (opacity == 0) opacity = 1;

    if (kind == FLAT_JAVA_LIGHT_KIND_SKY && flat_can_block_see_sky_slow(x, y, z)) {
        return 15;
    }

    int best = (kind == FLAT_JAVA_LIGHT_KIND_BLOCK) ? flat_block_light_value_for_id(id) : 0;
    int v;

    v = flat_light_saved_value_kind(kind, x - 1, y, z) - opacity; if (v > best) best = v;
    v = flat_light_saved_value_kind(kind, x + 1, y, z) - opacity; if (v > best) best = v;
    v = flat_light_saved_value_kind(kind, x, y - 1, z) - opacity; if (v > best) best = v;
    v = flat_light_saved_value_kind(kind, x, y + 1, z) - opacity; if (v > best) best = v;
    v = flat_light_saved_value_kind(kind, x, y, z - 1) - opacity; if (v > best) best = v;
    v = flat_light_saved_value_kind(kind, x, y, z + 1) - opacity; if (v > best) best = v;

    if (best < 0) best = 0;
    if (best > 15) best = 15;
    return best;
}

static void flat_update_light_now(int kind, int x, int y, int z) {
    if (y < FLAT_WORLD_Y_MIN || y > FLAT_WORLD_Y_MAX) return;
    if (!flat_light_chunks_ready_near(x, y, z, 17)) {
        flat_mark_light_dirty_region(x - 15, z - 15, x + 15, z + 15);
        return;
    }

    int *queue = (int*)malloc(sizeof(int) * FLAT_JAVA_LIGHT_QUEUE_CAP);
    if (!queue) {
        flat_mark_light_dirty_region(x - 15, z - 15, x + 15, z + 15);
        return;
    }

    int head = 0;
    int tail = 0;
    int old_value = flat_light_saved_value_kind(kind, x, y, z);
    int new_value = flat_compute_light_value_kind(kind, x, y, z);

    if (new_value > old_value) {
        queue[tail++] = 133152;
    } else if (new_value < old_value) {
        queue[tail++] = 133152 + (old_value << 18);
        while (head < tail) {
            int code = queue[head++];
            int wx = (code & 63) - 32 + x;
            int wy = ((code >> 6) & 63) - 32 + y;
            int wz = ((code >> 12) & 63) - 32 + z;
            int level = (code >> 18) & 15;
            int saved = flat_light_saved_value_kind(kind, wx, wy, wz);
            if (saved != level) continue;

            flat_set_light_value_kind(kind, wx, wy, wz, 0);
            if (level <= 0) continue;

            int dx = wx - x; if (dx < 0) dx = -dx;
            int dy = wy - y; if (dy < 0) dy = -dy;
            int dz = wz - z; if (dz < 0) dz = -dz;
            if (dx + dy + dz >= 17) continue;

            static const int ox[6] = {-1, 1, 0, 0, 0, 0};
            static const int oy[6] = {0, 0, -1, 1, 0, 0};
            static const int oz[6] = {0, 0, 0, 0, -1, 1};
            for (int i = 0; i < 6 && tail < FLAT_JAVA_LIGHT_QUEUE_CAP; ++i) {
                int nx = wx + ox[i], ny = wy + oy[i], nz = wz + oz[i];
                int n_saved = flat_light_saved_value_kind(kind, nx, ny, nz);
                int n_opacity = flat_light_opacity_for_id(flat_get_block(nx, ny, nz));
                if (n_opacity == 0) n_opacity = 1;
                if (n_saved == level - n_opacity) {
                    queue[tail++] = nx - x + 32 + ((ny - y + 32) << 6) +
                                    ((nz - z + 32) << 12) + ((level - n_opacity) << 18);
                }
            }
        }
        head = 0;
    }

    while (head < tail) {
        int code = queue[head++];
        int wx = (code & 63) - 32 + x;
        int wy = ((code >> 6) & 63) - 32 + y;
        int wz = ((code >> 12) & 63) - 32 + z;
        int saved = flat_light_saved_value_kind(kind, wx, wy, wz);
        int computed = flat_compute_light_value_kind(kind, wx, wy, wz);
        if (computed == saved) continue;

        flat_set_light_value_kind(kind, wx, wy, wz, computed);
        if (computed <= saved) continue;

        int dx = wx - x; if (dx < 0) dx = -dx;
        int dy = wy - y; if (dy < 0) dy = -dy;
        int dz = wz - z; if (dz < 0) dz = -dz;
        if (dx + dy + dz >= 17 || tail >= FLAT_JAVA_LIGHT_QUEUE_CAP - 6) continue;

        if (flat_light_saved_value_kind(kind, wx - 1, wy, wz) < computed) queue[tail++] = wx - 1 - x + 32 + ((wy - y + 32) << 6) + ((wz - z + 32) << 12);
        if (flat_light_saved_value_kind(kind, wx + 1, wy, wz) < computed) queue[tail++] = wx + 1 - x + 32 + ((wy - y + 32) << 6) + ((wz - z + 32) << 12);
        if (flat_light_saved_value_kind(kind, wx, wy - 1, wz) < computed) queue[tail++] = wx - x + 32 + ((wy - 1 - y + 32) << 6) + ((wz - z + 32) << 12);
        if (flat_light_saved_value_kind(kind, wx, wy + 1, wz) < computed) queue[tail++] = wx - x + 32 + ((wy + 1 - y + 32) << 6) + ((wz - z + 32) << 12);
        if (flat_light_saved_value_kind(kind, wx, wy, wz - 1) < computed) queue[tail++] = wx - x + 32 + ((wy - y + 32) << 6) + ((wz - 1 - z + 32) << 12);
        if (flat_light_saved_value_kind(kind, wx, wy, wz + 1) < computed) queue[tail++] = wx - x + 32 + ((wy - y + 32) << 6) + ((wz + 1 - z + 32) << 12);
    }

    free(queue);
}

static int flat_update_edit_light_now(int x, int y, int z, int old_id, int new_id) {
    if (g_stream_remap_in_progress) return 0;
    if (!flat_persistent_edit_active()) return 0;

    int old_light = flat_block_light_value_for_id(old_id);
    int new_light = flat_block_light_value_for_id(new_id);
    int old_opacity = flat_light_opacity_for_id(old_id);
    int new_opacity = flat_light_opacity_for_id(new_id);
    if (old_light == new_light && old_opacity == new_opacity) return 1;

    if (old_opacity != new_opacity) {
        flat_update_light_now(FLAT_JAVA_LIGHT_KIND_SKY, x, y, z);
    }
    flat_update_light_now(FLAT_JAVA_LIGHT_KIND_BLOCK, x, y, z);
    return 1;
}


static int g_flat_light_dirty = 0;
static int g_flat_light_x0 = 0, g_flat_light_z0 = 0, g_flat_light_x1 = 0, g_flat_light_z1 = 0;
static CRITICAL_SECTION g_flat_light_dirty_cs;
static int g_flat_light_dirty_cs_initialized = 0;

static void flat_light_dirty_lock(void) {
    if (g_flat_light_dirty_cs_initialized) return;
    InitializeCriticalSection(&g_flat_light_dirty_cs);
    g_flat_light_dirty_cs_initialized = 1;
}

static int flat_lighting_pending_dirty(void) {
    int pending = 0;
    flat_light_dirty_lock();
    EnterCriticalSection(&g_flat_light_dirty_cs);
    pending = g_flat_light_dirty ? 1 : 0;
    LeaveCriticalSection(&g_flat_light_dirty_cs);
    return pending;
}

static void flat_mark_light_dirty_region(int x0, int z0, int x1, int z1) {
    flat_light_dirty_lock();
    EnterCriticalSection(&g_flat_light_dirty_cs);
    if (!g_flat_light_dirty) {
        g_flat_light_dirty = 1;
        g_flat_light_x0 = x0; g_flat_light_z0 = z0; g_flat_light_x1 = x1; g_flat_light_z1 = z1;
    } else {
        if (x0 < g_flat_light_x0) g_flat_light_x0 = x0;
        if (z0 < g_flat_light_z0) g_flat_light_z0 = z0;
        if (x1 > g_flat_light_x1) g_flat_light_x1 = x1;
        if (z1 > g_flat_light_z1) g_flat_light_z1 = z1;
    }
    LeaveCriticalSection(&g_flat_light_dirty_cs);
    flat_lighting_worker_wake();
}

static void flat_mark_light_dirty_around_block(int x, int y, int z) {
    (void)y;
    flat_mark_light_dirty_region(x, z, x, z);
}

static void flat_mark_light_dirty_for_change(int x, int y, int z, int old_id, int new_id) {
    int old_light = flat_block_light_value_for_id(old_id);
    int new_light = flat_block_light_value_for_id(new_id);
    int old_opacity = flat_light_opacity_for_id(old_id);
    int new_opacity = flat_light_opacity_for_id(new_id);

    if (old_light > 0 || new_light > 0 || old_opacity != new_opacity) {
        /* Player edits must light immediately like Java World.setBlock() ->
           updateAllLightTypes().  Do not leave torch/block updates waiting for
           the background lighting worker; that made placed torches visibly lag. */
        if (flat_update_edit_light_now(x, y, z, old_id, new_id)) return;

        /* Non-player/batched updates still use the worker path so fluids/falling
           blocks and streaming do not stall the main/render thread. */
        flat_mark_light_dirty_region(x - 15, z - 15, x + 15, z + 15);
    } else {
        flat_mark_light_dirty_around_block(x, y, z);
    }
}

static void flat_preview_pending_lighting(void) {
    int dirty = 0, x0 = 0, z0 = 0, x1 = 0, z1 = 0;
    flat_light_dirty_lock();
    EnterCriticalSection(&g_flat_light_dirty_cs);
    dirty = g_flat_light_dirty;
    x0 = g_flat_light_x0; z0 = g_flat_light_z0; x1 = g_flat_light_x1; z1 = g_flat_light_z1;
    LeaveCriticalSection(&g_flat_light_dirty_cs);
    if (!dirty) return;
    int cx = (x0 + x1) / 2;
    int cz = (z0 + z1) / 2;
    /* Small, synchronous visual preview only.  This is intentionally much
       smaller than the full 15-block light update; it avoids gameplay hitches
       while making the edited area update before the async/full pass completes. */
    flat_relight_region(cx - 3, cz - 3, cx + 3, cz + 3, 0, 1);
}

static int flat_take_pending_light_region(int *x0, int *z0, int *x1, int *z1) {
    if (g_stream_remap_in_progress) return 0;
    flat_light_dirty_lock();
    EnterCriticalSection(&g_flat_light_dirty_cs);
    if (!g_flat_light_dirty || g_stream_remap_in_progress) {
        LeaveCriticalSection(&g_flat_light_dirty_cs);
        return 0;
    }
    *x0 = g_flat_light_x0; *z0 = g_flat_light_z0;
    *x1 = g_flat_light_x1; *z1 = g_flat_light_z1;
    g_flat_light_dirty = 0;
    LeaveCriticalSection(&g_flat_light_dirty_cs);
    return 1;
}

static void flat_flush_pending_lighting(void) {
    int x0 = 0, z0 = 0, x1 = 0, z1 = 0;
    if (!flat_take_pending_light_region(&x0, &z0, &x1, &z1)) return;
    pex_logf_trace("lighting flush pending x=%d..%d z=%d..%d", x0, x1, z0, z1);
    flat_recalculate_lighting_region(x0, z0, x1, z1);
}

static void flat_set_level_raw(int x, int y, int z, int level) {
    if (!flat_in_bounds(x, y, z)) return;
    int yi = flat_y_index(y), zi = flat_z_index(z), xi = flat_index(x);
    g_flat_levels[yi][zi][xi] = (unsigned char)(level & 15);
    if (block_is_liquid(g_flat_blocks[yi][zi][xi])) g_flat_meta[yi][zi][xi] = (unsigned char)(level & 15);
}

static float fluid_decay_height(int level) {
    if (level >= 8) level = 0;
    if (level < 0) level = 0;
    return (float)(level + 1) / 9.0f;
}

static void flat_set_meta_raw(int x, int y, int z, int meta) {
    if (!flat_in_bounds(x, y, z)) return;
    unsigned char *cell = &g_flat_meta[flat_y_index(y)][flat_z_index(z)][flat_index(x)];
    if (*cell != (unsigned char)(meta & 255)) {
        *cell = (unsigned char)(meta & 255);
        if (block_is_liquid(flat_get_block(x, y, z))) {
            g_flat_levels[flat_y_index(y)][flat_z_index(z)][flat_index(x)] = (unsigned char)(meta & 15);
        }
        flat_mark_sections_dirty_near_block(x, y, z);
        if (flat_persistent_edit_active()) {
            mark_flat_chunk_modified_at(x, z);
            g_save_dirty = 1;
        }
    }
}

static int block_is_door_id(int id) { return id == BLOCK_WOOD_DOOR || id == BLOCK_IRON_DOOR; }
static int door_meta_is_upper(int meta) { return (meta & 8) != 0; }
static int door_meta_is_open(int meta) { return (meta & 4) != 0; }
static int door_collision_state_from_meta(int meta) { return (meta & 4) == 0 ? ((meta - 1) & 3) : (meta & 3); }

static int door_lower_y_at(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (!block_is_door_id(id)) return y;
    return door_meta_is_upper(flat_get_meta(x, y, z)) ? y - 1 : y;
}


static int flat_chunk_buf_index(int lx, int y, int lz) {
    return flat_y_index(y) * (FLAT_RENDER_CHUNK * FLAT_RENDER_CHUNK) + lz * FLAT_RENDER_CHUNK + lx;
}

static void flat_build_chunk_delta_path(int cx, int cz, char *out, size_t cap, int create_dir) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    (void)cx; (void)cz; (void)create_dir; if (cap) out[0] = 0; return;
#endif
#if defined(PEX_PLATFORM_WII)
    if (!g_wii_fat_ready || strncmp(g_loaded_world_dir, "memory:", 7) == 0) { (void)cx; (void)cz; (void)create_dir; if (cap) out[0] = 0; return; }
#endif
    if (!g_loaded_world_dir[0]) {
        if (cap) out[0] = 0;
        return;
    }
    char bx[32], bz[32];
    base36_i32(cx, bx, sizeof(bx));
    base36_i32(cz, bz, sizeof(bz));
    char dir[MAX_PATHBUF];
    snprintf(dir, sizeof(dir), "%s\\chunks", g_loaded_world_dir);
    if (create_dir) make_dir_recursive(dir);
    snprintf(out, cap, "%s\\c.%s.%s.dat", dir, bx, bz);
}

static void flat_legacy_chunk_delta_path(int cx, int cz, char *out, size_t cap) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    (void)cx; (void)cz; if (cap) out[0] = 0; return;
#endif
#if defined(PEX_PLATFORM_WII)
    if (!g_wii_fat_ready || strncmp(g_loaded_world_dir, "memory:", 7) == 0) { (void)cx; (void)cz; if (cap) out[0] = 0; return; }
#endif
    if (!g_loaded_world_dir[0]) {
        if (cap) out[0] = 0;
        return;
    }
    char bx[32], bz[32];
    base36_i32(cx, bx, sizeof(bx));
    base36_i32(cz, bz, sizeof(bz));
    snprintf(out, cap, "%s\\pexcraft_modified_chunks\\c.%s.%s.pcd", g_loaded_world_dir, bx, bz);
}

static void flat_chunk_delta_path(int cx, int cz, char *out, size_t cap) {
    flat_build_chunk_delta_path(cx, cz, out, cap, 1);
}

static void mark_flat_chunk_modified_at(int x, int z) {
    if (x < g_flat_world_origin_x || x >= g_flat_world_origin_x + FLAT_WORLD_SIZE ||
        z < g_flat_world_origin_z || z >= g_flat_world_origin_z + FLAT_WORLD_SIZE) return;
    int lcx = flat_index(x) / FLAT_RENDER_CHUNK;
    int lcz = flat_z_index(z) / FLAT_RENDER_CHUNK;
    if (lcx >= 0 && lcx < FLAT_RENDER_CHUNKS && lcz >= 0 && lcz < FLAT_RENDER_CHUNKS) {
        g_flat_world_chunk_modified[lcz][lcx] = 1;
    }
}

static void flat_mark_all_chunks_modified(void) {
    for (int cz = 0; cz < FLAT_RENDER_CHUNKS; cz++) {
        for (int cx = 0; cx < FLAT_RENDER_CHUNKS; cx++) {
            g_flat_world_chunk_modified[cz][cx] = 1;
        }
    }
}

static TerrainProvider *beta_stream_provider(void);

#if (defined(PEX_PLATFORM_PSP) && !(defined(PEX_PSP_REAL_BETA_GEN) && PEX_PSP_REAL_BETA_GEN)) || defined(PEX_PLATFORM_WII)
/* PSP/Wii-safe "normal" terrain: the exact Beta generator allocates and populates a
   3x3 chunk canvas for every streamed chunk.  That is too heavy for PSP and was
   closing PPSSPP during normal-world generation.  This keeps the normal-world
   look but is deterministic, chunk-local, and cheap enough for cooperative
   streaming. */
static unsigned int psp_safe_terrain_hash(int x, int z, int salt, long long seed) {
    unsigned int h = (unsigned int)(x * 374761393u + z * 668265263u + salt * 1442695041u + (unsigned int)seed);
    h ^= (unsigned int)((unsigned long long)seed >> 32);
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static float psp_safe_noise(int x, int z, int scale, int salt, long long seed) {
    int gx = (x >= 0) ? x / scale : (x - scale + 1) / scale;
    int gz = (z >= 0) ? z / scale : (z - scale + 1) / scale;
    unsigned int h = psp_safe_terrain_hash(gx, gz, salt, seed);
    return ((float)(h & 65535u) / 65535.0f) * 2.0f - 1.0f;
}
static int psp_safe_height_at(int x, int z, long long seed) {
    float n1 = psp_safe_noise(x, z, 18, 1, seed) * 10.0f;
    float n2 = psp_safe_noise(x, z, 7, 2, seed) * 4.0f;
    float n3 = psp_safe_noise(x, z, 37, 3, seed) * 14.0f;
    int h = 32 + (int)(n1 + n2 + n3);
    if (h < 8) h = 8;
    int max_h = FLAT_WORLD_Y_MAX - 5;
    if (max_h < 12) max_h = 12;
    if (h > max_h) h = max_h;
    return h;
}
#endif

static void flat_generate_chunk_base_with_meta(int cx, int cz, unsigned char *out, unsigned char *out_meta, int world_type, long long seed, TerrainProvider *reuse_tp) {
    if (!out) return;
    memset(out, 0, FLAT_CHUNK_BLOCK_COUNT);
    if (out_meta) memset(out_meta, 0, FLAT_CHUNK_BLOCK_COUNT);

    if (world_type == 1) {
#if (defined(PEX_PLATFORM_PSP) && !(defined(PEX_PSP_REAL_BETA_GEN) && PEX_PSP_REAL_BETA_GEN)) || defined(PEX_PLATFORM_WII)
        for (int lx = 0; lx < 16; lx++) {
            int wx = cx * 16 + lx;
            for (int lz = 0; lz < 16; lz++) {
                int wz = cz * 16 + lz;
                int h = psp_safe_height_at(wx, wz, seed);
                for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
                    int id = 0;
                    if (y == 0) id = BLOCK_BEDROCK;
                    else if (y < h - 4) {
                        unsigned int oh = psp_safe_terrain_hash(wx, wz, y * 31 + 7, seed);
                        if (y < 18 && (oh % 900u) < 4u) id = BLOCK_DIAMOND_ORE;
                        else if (y < 32 && (oh % 360u) < 4u) id = BLOCK_GOLD_ORE;
                        else if (y < 45 && (oh % 180u) < 5u) id = BLOCK_IRON_ORE;
                        else if (y < 58 && (oh % 115u) < 6u) id = BLOCK_COAL_ORE;
                        else id = BLOCK_STONE;
                    } else if (y < h) id = BLOCK_DIRT;
                    else if (y == h) id = BLOCK_GRASS;
                    else if (y <= 30 && h < 30) id = (y == 30) ? BLOCK_STILL_WATER : BLOCK_WATER;
                    out[flat_chunk_buf_index(lx, y, lz)] = (unsigned char)id;
                }
            }
        }
#else
        /* Real Beta-style generator path.  Keep the extracted 16x128x16 chunk
           on the heap so PSP worker threads do not lose half their stack to a
           32KB local array. */
        unsigned char *beta_blocks = (unsigned char*)calloc(32768u, 1);
        unsigned char *beta_data = out_meta ? (unsigned char*)calloc(16384u, 1) : NULL;

        TerrainProvider *local_tp = NULL;
        TerrainProvider *tp = reuse_tp;
        if (!tp) {
            local_tp = (TerrainProvider*)calloc(1, sizeof(*local_tp));
            if (local_tp) {
                terrain_provider_init(local_tp, (int64_t)seed);
                tp = local_tp;
            }
        }

        GenCanvas cv;
        cv.minCx = cx - 1;
        cv.minCz = cz - 1;
        cv.chunks = 3;
        cv.blocks = (beta_blocks && tp) ? (unsigned char*)calloc((size_t)cv.chunks * (size_t)cv.chunks * 32768u, 1) : NULL;
        cv.meta = (cv.blocks && out_meta) ? (unsigned char*)calloc((size_t)cv.chunks * (size_t)cv.chunks * 32768u, 1) : NULL;
        if (cv.blocks) {
            for (int dz = 0; dz < cv.chunks; dz++) {
                for (int dx = 0; dx < cv.chunks; dx++) {
                    generate_canvas_chunk(tp, &cv, cv.minCx + dx, cv.minCz + dz);
                }
            }
            /* Structures are part of the chunk provider, not only the offline
               hash/dump path.  Missing this call made /traceplace locate a
               legal village/stronghold start while normal in-game chunks had
               only terrain and decoration. */
            worldgen_place_structure_blocks(tp, &cv, cx, cz);
            for (int pz = cz - 1; pz <= cz; pz++) {
                for (int px = cx - 1; px <= cx; px++) {
                    qm_populate_canvas(tp, &cv, px, pz);
                }
            }
            extract_canvas_chunk(&cv, cx, cz, beta_blocks, beta_data);
            free(cv.blocks);
            free(cv.meta);
        }
        if (local_tp) { terrain_provider_free(local_tp); free(local_tp); }

        for (int lx = 0; lx < 16; lx++) {
            for (int lz = 0; lz < 16; lz++) {
                for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
                    int id = 0;
                    if (beta_blocks && y < 128) {
                        id = get_block_local(beta_blocks, lx, y, lz);
                    }
                    int bi = flat_chunk_buf_index(lx, y, lz);
                    out[bi] = (unsigned char)id;
                    if (out_meta && beta_data && y < 128) {
                        int idx = chunk_index(lx, y, lz);
                        unsigned char packed = beta_data[idx >> 1];
                        out_meta[bi] = (unsigned char)((idx & 1) ? ((packed >> 4) & 15) : (packed & 15));
                    }
                }
                if (out[flat_chunk_buf_index(lx, 0, lz)] == 0) {
                    out[flat_chunk_buf_index(lx, 0, lz)] = BLOCK_BEDROCK;
                }
            }
        }
        free(beta_blocks);
        free(beta_data);
#endif
    } else {
        for (int lx = 0; lx < 16; lx++) {
            for (int lz = 0; lz < 16; lz++) {
                out[flat_chunk_buf_index(lx, 0, lz)] = BLOCK_BEDROCK;
                out[flat_chunk_buf_index(lx, 1, lz)] = BLOCK_DIRT;
                out[flat_chunk_buf_index(lx, 2, lz)] = BLOCK_DIRT;
                out[flat_chunk_buf_index(lx, 3, lz)] = BLOCK_GRASS;
            }
        }
    }
}

static void flat_generate_chunk_base(int cx, int cz, unsigned char *out, int world_type, long long seed, TerrainProvider *reuse_tp) {
    flat_generate_chunk_base_with_meta(cx, cz, out, NULL, world_type, seed, reuse_tp);
}

static void flat_generate_chunk_base_to_buffer(int cx, int cz, unsigned char *out) {
#if (defined(PEX_PLATFORM_PSP) && !(defined(PEX_PSP_REAL_BETA_GEN) && PEX_PSP_REAL_BETA_GEN)) || defined(PEX_PLATFORM_WII)
    TerrainProvider *reuse = NULL;
#else
    TerrainProvider *reuse = (g_world_type == 1) ? beta_stream_provider() : NULL;
#endif
    flat_generate_chunk_base(cx, cz, out, g_world_type, g_world_seed, reuse);
}

static void flat_copy_chunk_buffer(int cx, int cz, const unsigned char *buf) {
    if (!buf) return;
    for (int lx = 0; lx < 16; lx++) {
        int wx = cx * 16 + lx;
        if (wx < g_flat_world_origin_x || wx >= g_flat_world_origin_x + FLAT_WORLD_SIZE) continue;
        int fx = flat_index(wx);
        for (int lz = 0; lz < 16; lz++) {
            int wz = cz * 16 + lz;
            if (wz < g_flat_world_origin_z || wz >= g_flat_world_origin_z + FLAT_WORLD_SIZE) continue;
            int fz = flat_z_index(wz);
            for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
                g_flat_blocks[flat_y_index(y)][fz][fx] = buf[flat_chunk_buf_index(lx, y, lz)];
                g_flat_meta[flat_y_index(y)][fz][fx] = 0;
                g_flat_levels[flat_y_index(y)][fz][fx] = 0;
            }
        }
    }
    int lcx = stream_local_chunk_x(cx);
    int lcz = stream_local_chunk_z(cz);
    if (flat_local_chunk_valid(lcx, lcz)) flat_refresh_chunk_occupancy(lcx, lcz);
    flat_relight_fast_surface_chunk(cx, cz);
    if (flat_chunk_buffer_has_light_source(buf)) flat_mark_chunk_block_light_dirty(cx, cz);
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_FAST_WORLD) && PEX_PSP_FAST_WORLD
    psp_fast_surface_mark_dirty(cx * 16 + 8, cz * 16 + 8);
#endif
}

static void flat_load_chunk_delta(int cx, int cz) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    (void)cx; (void)cz; return;
#endif
#if defined(PEX_PLATFORM_WII)
    if (!g_wii_fat_ready || strncmp(g_loaded_world_dir, "memory:", 7) == 0) { (void)cx; (void)cz; return; }
#endif
    if (!g_loaded_world_dir[0]) return;
    char path[MAX_PATHBUF];
    flat_build_chunk_delta_path(cx, cz, path, sizeof(path), 0);
    FILE *f = fopen(path, "rb");
    if (!f) {
        flat_legacy_chunk_delta_path(cx, cz, path, sizeof(path));
        f = fopen(path, "rb");
    }
    if (!f) return;
    char magic[8];
    int rcx = 0, rcz = 0, count = 0;
    if (fread(magic, 1, 8, f) != 8 ||
        fread(&rcx, sizeof(rcx), 1, f) != 1 ||
        fread(&rcz, sizeof(rcz), 1, f) != 1 ||
        fread(&count, sizeof(count), 1, f) != 1) {
        fclose(f);
        return;
    }
    unsigned char *buf = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
    unsigned char *meta = (unsigned char*)calloc(FLAT_CHUNK_BLOCK_COUNT, 1);
    if (!buf || !meta) { free(buf); free(meta); fclose(f); return; }
    int ok = chunk_delta_read_payload(f, magic, rcx, rcz, count, cx, cz, buf, meta);
    fclose(f);
    if (ok) {
        flat_copy_chunk_buffer(cx, cz, buf);
        {
            for (int lx = 0; lx < 16; lx++) {
                int wx = cx * 16 + lx;
                if (wx < g_flat_world_origin_x || wx >= g_flat_world_origin_x + FLAT_WORLD_SIZE) continue;
                int fx = flat_index(wx);
                for (int lz = 0; lz < 16; lz++) {
                    int wz = cz * 16 + lz;
                    if (wz < g_flat_world_origin_z || wz >= g_flat_world_origin_z + FLAT_WORLD_SIZE) continue;
                    int fz = flat_z_index(wz);
                    for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
                        int bi = flat_chunk_buf_index(lx, y, lz);
                        int id = g_flat_blocks[flat_y_index(y)][fz][fx];
                        g_flat_meta[flat_y_index(y)][fz][fx] = meta[bi];
                        g_flat_levels[flat_y_index(y)][fz][fx] = block_is_liquid(id) ? (meta[bi] & 15) : 0;
                    }
                }
            }
        }
    }
    flat_relight_fast_surface_chunk(cx, cz);
    free(buf);
    free(meta);
}


static void flat_chunk_delta_path_for_dir(const char *world_dir, int cx, int cz, char *out, size_t cap, int create_dir) {
    if (!world_dir || !world_dir[0]) {
        if (cap) out[0] = 0;
        return;
    }
    char bx[32], bz[32];
    base36_i32(cx, bx, sizeof(bx));
    base36_i32(cz, bz, sizeof(bz));
    char dir[MAX_PATHBUF];
    snprintf(dir, sizeof(dir), "%s\\chunks", world_dir);
    if (create_dir) make_dir_recursive(dir);
    snprintf(out, cap, "%s\\c.%s.%s.dat", dir, bx, bz);
}

static void flat_legacy_chunk_delta_path_for_dir(const char *world_dir, int cx, int cz, char *out, size_t cap) {
    if (!world_dir || !world_dir[0]) {
        if (cap) out[0] = 0;
        return;
    }
    char bx[32], bz[32];
    base36_i32(cx, bx, sizeof(bx));
    base36_i32(cz, bz, sizeof(bz));
    snprintf(out, cap, "%s\\pexcraft_modified_chunks\\c.%s.%s.pcd", world_dir, bx, bz);
}

static void flat_load_chunk_delta_for_dir(const char *world_dir, int cx, int cz,
                                                                unsigned char *buf, unsigned char *meta) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    (void)world_dir; (void)cx; (void)cz; (void)buf; (void)meta; return;
#endif
    if (!world_dir || !world_dir[0] || !buf || !meta) return;
    char path[MAX_PATHBUF];
    flat_chunk_delta_path_for_dir(world_dir, cx, cz, path, sizeof(path), 0);
    FILE *f = fopen(path, "rb");
    if (!f) {
        flat_legacy_chunk_delta_path_for_dir(world_dir, cx, cz, path, sizeof(path));
        f = fopen(path, "rb");
    }
    if (!f) return;
    char magic[8];
    int rcx = 0, rcz = 0, count = 0;
    if (fread(magic, 1, 8, f) != 8 ||
        fread(&rcx, sizeof(rcx), 1, f) != 1 ||
        fread(&rcz, sizeof(rcz), 1, f) != 1 ||
        fread(&count, sizeof(count), 1, f) != 1) {
        fclose(f);
        return;
    }
    if (!chunk_delta_read_payload(f, magic, rcx, rcz, count, cx, cz, buf, meta)) {
        memset(meta, 0, FLAT_CHUNK_BLOCK_COUNT);
    }
    fclose(f);
}

static int g_copy_chunk_skip_main_light = 0;
static void flat_copy_chunk_buffers(int cx, int cz, const unsigned char *buf, const unsigned char *meta) {
    if (!buf) return;
    for (int lx = 0; lx < 16; lx++) {
        int wx = cx * 16 + lx;
        if (wx < g_flat_world_origin_x || wx >= g_flat_world_origin_x + FLAT_WORLD_SIZE) continue;
        int fx = flat_index(wx);
        for (int lz = 0; lz < 16; lz++) {
            int wz = cz * 16 + lz;
            if (wz < g_flat_world_origin_z || wz >= g_flat_world_origin_z + FLAT_WORLD_SIZE) continue;
            int fz = flat_z_index(wz);
            for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
                int bi = flat_chunk_buf_index(lx, y, lz);
                int id = buf[bi];
                unsigned char m = meta ? meta[bi] : 0;
                g_flat_blocks[flat_y_index(y)][fz][fx] = (unsigned char)id;
                g_flat_meta[flat_y_index(y)][fz][fx] = m;
                g_flat_levels[flat_y_index(y)][fz][fx] = block_is_liquid(id) ? (m & 15) : 0;
            }
        }
    }
    int lcx = stream_local_chunk_x(cx);
    int lcz = stream_local_chunk_z(cz);
    if (flat_local_chunk_valid(lcx, lcz)) flat_refresh_chunk_occupancy(lcx, lcz);
    if (!g_copy_chunk_skip_main_light) flat_relight_fast_surface_chunk(cx, cz);
    if (flat_chunk_buffer_has_light_source(buf)) flat_mark_chunk_block_light_dirty(cx, cz);
}

static void save_one_modified_flat_chunk(int lcx, int lcz) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    if (lcz >= 0 && lcz < FLAT_RENDER_CHUNKS && lcx >= 0 && lcx < FLAT_RENDER_CHUNKS) g_flat_world_chunk_modified[lcz][lcx] = 0;
    return;
#endif
    if (!g_loaded_world_dir[0]) return;
    if (lcx < 0 || lcx >= FLAT_RENDER_CHUNKS || lcz < 0 || lcz >= FLAT_RENDER_CHUNKS) return;
    if (!g_flat_world_chunk_modified[lcz][lcx]) return;

    int cx = floor_div16(g_flat_world_origin_x) + lcx;
    int cz = floor_div16(g_flat_world_origin_z) + lcz;
    unsigned char *base = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
    unsigned char *cur = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
    unsigned char *cur_meta = (unsigned char*)calloc(FLAT_CHUNK_BLOCK_COUNT, 1);
    if (!base || !cur || !cur_meta) { free(base); free(cur); free(cur_meta); return; }

    flat_generate_chunk_base_to_buffer(cx, cz, base);
    memset(cur, 0, FLAT_CHUNK_BLOCK_COUNT);
    for (int lx = 0; lx < 16; lx++) {
        int wx = cx * 16 + lx;
        if (wx < g_flat_world_origin_x || wx >= g_flat_world_origin_x + FLAT_WORLD_SIZE) continue;
        int fx = flat_index(wx);
        for (int lz = 0; lz < 16; lz++) {
            int wz = cz * 16 + lz;
            if (wz < g_flat_world_origin_z || wz >= g_flat_world_origin_z + FLAT_WORLD_SIZE) continue;
            int fz = flat_z_index(wz);
            for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
                int bi = flat_chunk_buf_index(lx, y, lz);
                cur[bi] = (unsigned char)g_flat_blocks[flat_y_index(y)][fz][fx];
                cur_meta[bi] = (unsigned char)g_flat_meta[flat_y_index(y)][fz][fx];
            }
        }
    }

    char path[MAX_PATHBUF];
    flat_chunk_delta_path(cx, cz, path, sizeof(path));
    int meta_nonzero = 0;
    for (int i = 0; i < FLAT_CHUNK_BLOCK_COUNT; i++) { if (cur_meta[i]) { meta_nonzero = 1; break; } }
    if (memcmp(base, cur, FLAT_CHUNK_BLOCK_COUNT) == 0 && !meta_nonzero) {
        DeleteFileA(path);
    } else {
        chunk_delta_write_payload(path, cx, cz, cur, cur_meta);
    }
    g_flat_world_chunk_modified[lcz][lcx] = 0;
    free(base);
    free(cur);
    free(cur_meta);
}

static void save_modified_flat_chunks_sync(void) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    memset(g_flat_world_chunk_modified, 0, sizeof(g_flat_world_chunk_modified));
    return;
#endif
    if (!g_loaded_world_dir[0]) return;
    for (int lcz = 0; lcz < FLAT_RENDER_CHUNKS; lcz++) {
        for (int lcx = 0; lcx < FLAT_RENDER_CHUNKS; lcx++) {
            save_one_modified_flat_chunk(lcx, lcz);
        }
    }
}

static void save_modified_flat_chunks(void) {
    request_chunk_save_async();
}


static void flat_note_section_mesh_changed(int sy, int cz, int cx) {
    if (sy < 0 || sy >= FLAT_RENDER_SECTIONS_Y || cz < 0 || cz >= FLAT_RENDER_CHUNKS || cx < 0 || cx >= FLAT_RENDER_CHUNKS) return;
    g_flat_section_mesh_version[sy][cz][cx]++;
    if (g_flat_section_mesh_version[sy][cz][cx] == 0) g_flat_section_mesh_version[sy][cz][cx] = 1;
    g_flat_section_mesh_building[sy][cz][cx] = 0;
}

static void flat_mark_all_chunks_dirty(void) {
    for (int cz = 0; cz < FLAT_RENDER_CHUNKS; cz++) {
        for (int cx = 0; cx < FLAT_RENDER_CHUNKS; cx++) {
            if (g_flat_world_chunk_generated[cz][cx]) flat_refresh_chunk_occupancy(cx, cz);
            g_flat_world_chunk_dirty[cz][cx] = 1;
            g_flat_world_chunk_valid[cz][cx] = 0;
            g_flat_world_chunk_has_liquid[cz][cx] = 0;
            for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
                flat_mark_generated_section(cx, cz, sy);
            }
        }
    }
    g_flat_renderer_sort_dirty = 1;
    g_flat_world_geometry_dirty = 0;
    g_flat_section_geometry_dirty = 0;
}

static void flat_mark_all_sections_dirty(void) {
    /* Java 1.2.5 changes day/night lighting through EntityRenderer.updateLightmap()
       without throwing away WorldRenderer display lists.  This port still bakes
       the lightmap into terrain vertex colors, so when skylightSubtracted changes
       we must rebuild section colors, but old meshes must remain drawable until
       their replacements are ready. */
    for (int cz = 0; cz < FLAT_RENDER_CHUNKS; cz++) {
        for (int cx = 0; cx < FLAT_RENDER_CHUNKS; cx++) {
            if (!g_flat_world_chunk_generated[cz][cx]) continue;
            flat_refresh_chunk_occupancy(cx, cz);
            g_flat_world_chunk_dirty[cz][cx] = 1;
            for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
                if (!flat_section_has_blocks(cx, cz, sy)) continue;
                flat_mark_section_dirty(cx, cz, sy);
            }
        }
    }
    g_flat_renderer_sort_dirty = 1;
}

static void flat_mark_chunks_dirty_near(int x, int z) {
    if (!flat_in_bounds(x, FLAT_WORLD_Y_MIN, z)) return;
    int fx = flat_index(x);
    int fz = flat_z_index(z);
    int cx0 = fx / FLAT_RENDER_CHUNK;
    int cz0 = fz / FLAT_RENDER_CHUNK;

    for (int dz = -1; dz <= 1; dz++) {
        for (int dx = -1; dx <= 1; dx++) {
            int cx = cx0 + dx;
            int cz = cz0 + dz;
            if (cx >= 0 && cx < FLAT_RENDER_CHUNKS && cz >= 0 && cz < FLAT_RENDER_CHUNKS) {
                g_flat_world_chunk_dirty[cz][cx] = 1;
                for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
                    g_flat_section_dirty[sy][cz][cx] = 1;
                    flat_note_section_mesh_changed(sy, cz, cx);
                }
            }
        }
    }
    g_flat_renderer_sort_dirty = 1;
}

static void flat_mark_section_dirty(int cx, int cz, int sy) {
    if (!flat_local_chunk_valid(cx, cz) || !flat_section_index_valid(sy)) return;
    g_flat_section_dirty[sy][cz][cx] = 1;
    flat_note_section_mesh_changed(sy, cz, cx);
    g_flat_world_chunk_dirty[cz][cx] = 1;
    g_flat_renderer_sort_dirty = 1;
}

static void flat_mark_sections_dirty_near_block(int x, int y, int z) {
    if (!flat_in_bounds(x, y, z)) return;
    /* A player/block edit should be visible quickly.  Java adds the touched
       WorldRenderer(s) to worldRenderersToUpdate immediately; keep the heavy
       mesh rebuild off-thread, but tell the render side to prioritize adopting
       nearby completed edit meshes over background streaming results. */
    g_flat_recent_block_mesh_dirty_tick = g_ingame_ticks;
    int fx = flat_index(x);
    int fz = flat_z_index(z);
    int cx0 = fx / FLAT_RENDER_CHUNK;
    int cz0 = fz / FLAT_RENDER_CHUNK;
    int sy0 = (y - FLAT_WORLD_Y_MIN) / FLAT_RENDER_SECTION;

    /* Java 1.5.2 marks the 16^3 WorldRenderer containing the changed block
       and only crosses into adjacent renderers when the block touches their
       shared boundary.  The previous implementation dirtied the full 3x3x3
       neighborhood for every edit/liquid/falling-block update, causing 27
       sections to be rebuilt for many single-block changes. */
    flat_mark_section_dirty(cx0, cz0, sy0);
    if ((fx & (FLAT_RENDER_CHUNK - 1)) == 0) flat_mark_section_dirty(cx0 - 1, cz0, sy0);
    if ((fx & (FLAT_RENDER_CHUNK - 1)) == FLAT_RENDER_CHUNK - 1) flat_mark_section_dirty(cx0 + 1, cz0, sy0);
    if ((fz & (FLAT_RENDER_CHUNK - 1)) == 0) flat_mark_section_dirty(cx0, cz0 - 1, sy0);
    if ((fz & (FLAT_RENDER_CHUNK - 1)) == FLAT_RENDER_CHUNK - 1) flat_mark_section_dirty(cx0, cz0 + 1, sy0);
    if (((y - FLAT_WORLD_Y_MIN) & (FLAT_RENDER_SECTION - 1)) == 0) flat_mark_section_dirty(cx0, cz0, sy0 - 1);
    if (((y - FLAT_WORLD_Y_MIN) & (FLAT_RENDER_SECTION - 1)) == FLAT_RENDER_SECTION - 1) flat_mark_section_dirty(cx0, cz0, sy0 + 1);
}

static int flat_section_has_blocks(int lcx, int lcz, int sy) {
    if (!flat_local_chunk_valid(lcx, lcz) || !flat_section_index_valid(sy)) return 0;
    return (g_flat_chunk_section_non_empty_mask[lcz][lcx] & (unsigned short)(1u << sy)) != 0;
}

static void flat_bump_light_version(int lcx, int lcz) {
    if (!flat_local_chunk_valid(lcx, lcz)) return;
    g_flat_chunk_light_version[lcz][lcx]++;
    if (g_flat_chunk_light_version[lcz][lcx] == 0) g_flat_chunk_light_version[lcz][lcx] = 1;
}

static int flat_chunk_light_ready(int lcx, int lcz) {
    if (!flat_local_chunk_valid(lcx, lcz)) return 0;
    return g_flat_world_chunk_generated[lcz][lcx] && g_flat_chunk_light_ready[lcz][lcx];
}

static void flat_publish_light_ready(int lcx, int lcz, int ready, int dirty_sections) {
    if (!flat_local_chunk_valid(lcx, lcz)) return;
    int old_ready = g_flat_chunk_light_ready[lcz][lcx];
    g_flat_chunk_light_ready[lcz][lcx] = ready ? 1 : 0;
    if (dirty_sections || old_ready != g_flat_chunk_light_ready[lcz][lcx]) {
        flat_bump_light_version(lcx, lcz);
    }
    if (!dirty_sections) return;
    for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; ++sy) {
        if (flat_section_has_blocks(lcx, lcz, sy)) {
            g_flat_section_dirty[sy][lcz][lcx] = 1;
            g_flat_section_valid[sy][lcz][lcx] = 0;
            flat_note_section_mesh_changed(sy, lcz, lcx);
            g_flat_world_chunk_dirty[lcz][lcx] = 1;
        } else {
            g_flat_section_mesh_building[sy][lcz][lcx] = 0;
            g_flat_section_mesh_light_version[sy][lcz][lcx] = g_flat_chunk_light_version[lcz][lcx];
        }
    }
    g_flat_renderer_sort_dirty = 1;
}

static void flat_mark_generated_section(int lcx, int lcz, int sy) {
    if (!flat_local_chunk_valid(lcx, lcz) || !flat_section_index_valid(sy)) return;
    if (!flat_chunk_light_ready(lcx, lcz) && g_flat_world_chunk_generated[lcz][lcx]) {
        g_flat_section_dirty[sy][lcz][lcx] = 1;
        g_flat_section_valid[sy][lcz][lcx] = 0;
        flat_note_section_mesh_changed(sy, lcz, lcx);
        g_flat_world_chunk_dirty[lcz][lcx] = 1;
        return;
    }
    if (flat_section_has_blocks(lcx, lcz, sy)) {
        g_flat_section_dirty[sy][lcz][lcx] = 1;
        flat_note_section_mesh_changed(sy, lcz, lcx);
        g_flat_section_valid[sy][lcz][lcx] = 0;
        g_flat_section_skip_pass[sy][lcz][lcx][0] = 1;
        g_flat_section_skip_pass[sy][lcz][lcx][1] = 1;
        g_flat_world_chunk_dirty[lcz][lcx] = 1;
        g_flat_world_chunk_valid[lcz][lcx] = 0;
    } else {
        /* Empty 16-high sections do not need a display-list/native-mesh rebuild.
           The section-existence mask makes this O(1), like Java's empty
           ExtendedBlockStorage sections, instead of a 4096-block scan. */
        g_flat_section_dirty[sy][lcz][lcx] = 0;
        g_flat_section_mesh_building[sy][lcz][lcx] = 0;
        g_flat_section_valid[sy][lcz][lcx] = 1;
        g_flat_section_skip_pass[sy][lcz][lcx][0] = 1;
        g_flat_section_skip_pass[sy][lcz][lcx][1] = 1;
        g_flat_section_mesh_light_version[sy][lcz][lcx] = g_flat_chunk_light_version[lcz][lcx];
    }
}

static int stream_generated_border_has_blocks(int cx, int cz, int sy, int dir) {
    if (!flat_local_chunk_valid(cx, cz) || !flat_section_index_valid(sy)) return 0;
    int x0 = cx * FLAT_RENDER_CHUNK;
    int z0 = cz * FLAT_RENDER_CHUNK;
    int y0 = sy * FLAT_RENDER_SECTION;
    int y1 = y0 + FLAT_RENDER_SECTION;
    if (y1 > FLAT_WORLD_HEIGHT) y1 = FLAT_WORLD_HEIGHT;

    int bx0 = x0, bx1 = x0 + FLAT_RENDER_CHUNK;
    int bz0 = z0, bz1 = z0 + FLAT_RENDER_CHUNK;
    if (dir == 0) { bx0 = x0; bx1 = x0 + 1; }
    else if (dir == 1) { bx0 = x0 + FLAT_RENDER_CHUNK - 1; bx1 = x0 + FLAT_RENDER_CHUNK; }
    else if (dir == 2) { bz0 = z0; bz1 = z0 + 1; }
    else { bz0 = z0 + FLAT_RENDER_CHUNK - 1; bz1 = z0 + FLAT_RENDER_CHUNK; }

    for (int y = y0; y < y1; ++y) {
        for (int z = bz0; z < bz1; ++z) {
            for (int x = bx0; x < bx1; ++x) {
                if (g_flat_blocks[y][z][x] != 0) return 1;
            }
        }
    }
    return 0;
}

static void stream_mark_neighbor_dirty_if_border_changed(int src_cx, int src_cz, int nx, int nz, int sy, int dir) {
    if (!flat_local_chunk_valid(nx, nz) || !flat_section_index_valid(sy)) return;
    if (!g_flat_world_chunk_generated[nz][nx]) return;
    if (!flat_section_has_blocks(nx, nz, sy)) return;
    if (!stream_generated_border_has_blocks(src_cx, src_cz, sy, dir)) return;
    flat_mark_section_dirty(nx, nz, sy);
}

static void stream_mark_chunk_generated(int lcx, int lcz) {
    if (!flat_local_chunk_valid(lcx, lcz)) return;
    pex_logf_trace("chunk generated mark local=%d,%d world=%d,%d", lcx, lcz, g_flat_world_origin_x / 16 + lcx, g_flat_world_origin_z / 16 + lcz);
    g_flat_world_chunk_generated[lcz][lcx] = 1;
    g_flat_chunk_initial_preload[lcz][lcx] = g_stream_generation_keep_completed ? 1 : 0;

    if (g_stream_generation_keep_completed) {
        /* Spawn preload must not run one full 48x48xheight light pass per chunk.
           Java 1.2.5 preloads the terrain area, drains lighting, then lets renderers
           rebuild from the settled world.  Running per-chunk light here was both
           slow and the source of mixed-brightness spawn chunks. */
        flat_publish_light_ready(lcx, lcz, 0, 1);
    } else {
        /* Runtime streaming may show a fast local light preview, but the expensive
           neighbor-aware pass is queued/batched for the lighting worker instead of
           blocking the stream commit thread for every chunk. */
        flat_publish_light_ready(lcx, lcz, 1, 1);
        {
            int wcx = floor_div16(g_flat_world_origin_x) + lcx;
            int wcz = floor_div16(g_flat_world_origin_z) + lcz;
            flat_mark_light_dirty_region(wcx * 16, wcz * 16, wcx * 16 + 15, wcz * 16 + 15);
        }
    }

    /* Recompute one chunk's 16-bit occupancy mask once, then use O(1) tests.
       This replaces the old 3x3x16 section scan performed for every installed
       streamed chunk.  Only the new chunk gets invalidated; existing neighbor
       meshes stay drawable and are dirtied only on shared borders. */
    unsigned short mask = flat_refresh_chunk_occupancy(lcx, lcz);
    g_flat_world_chunk_dirty[lcz][lcx] = 1;
    g_flat_world_chunk_valid[lcz][lcx] = 0;
    g_flat_renderer_sort_dirty = 1;
    for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
        flat_mark_generated_section(lcx, lcz, sy);
        if (mask & (unsigned short)(1u << sy)) {
            stream_mark_neighbor_dirty_if_border_changed(lcx, lcz, lcx - 1, lcz, sy, 0);
            stream_mark_neighbor_dirty_if_border_changed(lcx, lcz, lcx + 1, lcz, sy, 1);
            stream_mark_neighbor_dirty_if_border_changed(lcx, lcz, lcx, lcz - 1, sy, 2);
            stream_mark_neighbor_dirty_if_border_changed(lcx, lcz, lcx, lcz + 1, sy, 3);
        }
    }
}


static int flat_block_blocks_snow(int id) {
    if (id == 0 || id == BLOCK_WATER || id == BLOCK_STILL_WATER ||
        id == BLOCK_LAVA || id == BLOCK_STILL_LAVA) return 0;
    if (id == BLOCK_SNOW_LAYER || id == BLOCK_ICE || id == BLOCK_GLASS) return 0;
    if (id == BLOCK_SAPLING || id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE ||
        id == BLOCK_BROWN_MUSHROOM || id == BLOCK_RED_MUSHROOM ||
        id == BLOCK_TORCH || id == BLOCK_FIRE || id == BLOCK_REDSTONE_WIRE ||
        id == BLOCK_REDSTONE_TORCH_OFF || id == BLOCK_REDSTONE_TORCH_ON ||
        id == BLOCK_REEDS || id == BLOCK_LADDER || id == BLOCK_RAILS ||
        id == BLOCK_SIGN_POST || id == BLOCK_WALL_SIGN || id == BLOCK_LEVER ||
        id == BLOCK_STONE_BUTTON || id == BLOCK_WOOD_DOOR || id == BLOCK_IRON_DOOR ||
        id == BLOCK_PORTAL) return 0;
    return 1;
}

static int flat_snow_layer_can_stay_at(int x, int y, int z) {
    if (!flat_in_bounds(x, y, z) || y <= FLAT_WORLD_Y_MIN) return 0;
    return flat_block_blocks_snow(flat_get_block(x, y - 1, z));
}

static void flat_set_block_raw(int x, int y, int z, int id) {
    if (!flat_in_bounds(x, y, z)) return;
    int yi = flat_y_index(y), zi = flat_z_index(z), xi = flat_index(x);
    unsigned char *cell = &g_flat_blocks[yi][zi][xi];
    int old_id = *cell;
    /* Plain block writes clear both general metadata and liquid metadata.
       The fluid simulation uses flat_set_fluid() to update id + level together. */
    g_flat_levels[yi][zi][xi] = 0;
    if (*cell != id) {
        *cell = (unsigned char)id;
        g_flat_meta[yi][zi][xi] = 0;
        flat_update_section_after_block_change(x, y, z, id);
        flat_mark_sections_dirty_near_block(x, y, z);
        if (flat_light_opacity_for_id(old_id) != flat_light_opacity_for_id(id) ||
            flat_block_light_value_for_id(old_id) != flat_block_light_value_for_id(id)) {
            flat_mark_light_dirty_for_change(x, y, z, old_id, id);
        }
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_FAST_WORLD) && PEX_PSP_FAST_WORLD
        psp_fast_surface_mark_dirty(x, z);
        psp_fast_surface_note_edit_block(x, y, z);
#endif
        if (flat_persistent_edit_active()) {
            mark_flat_chunk_modified_at(x, z);
            g_save_dirty = 1;
        }
    }
    if (id == BLOCK_LAVA || id == BLOCK_STILL_LAVA) fluid_check_for_harden(x, y, z);
}

static void wake_falling_blocks_around(int x, int y, int z);

static void flat_set_fluid(int x, int y, int z, int id, int level) {
    if (!flat_in_bounds(x, y, z)) return;
    int yi = flat_y_index(y), zi = flat_z_index(z), xi = flat_index(x);
    unsigned char *cell = &g_flat_blocks[yi][zi][xi];
    int old_id = *cell;
    unsigned char *lvl = &g_flat_levels[yi][zi][xi];
    unsigned char want_level = (unsigned char)(level & 15);
    if (*cell != (unsigned char)id || *lvl != want_level || g_flat_meta[yi][zi][xi] != want_level) {
        *cell = (unsigned char)id;
        *lvl = want_level;
        g_flat_meta[yi][zi][xi] = want_level;
        flat_update_section_after_block_change(x, y, z, id);
        flat_mark_sections_dirty_near_block(x, y, z);
        if (flat_light_opacity_for_id(old_id) != flat_light_opacity_for_id(id) ||
            flat_block_light_value_for_id(old_id) != flat_block_light_value_for_id(id)) {
            flat_mark_light_dirty_for_change(x, y, z, old_id, id);
        }
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_FAST_WORLD) && PEX_PSP_FAST_WORLD
        psp_fast_surface_mark_dirty(x, z);
        psp_fast_surface_note_edit_block(x, y, z);
#endif
        if (flat_persistent_edit_active()) {
            mark_flat_chunk_modified_at(x, z);
            g_save_dirty = 1;
        }
    }
    fluid_check_for_harden(x, y, z);
    wake_falling_blocks_around(x, y, z);
}

static void flat_place_fluid_source(int x, int y, int z, int id) {
    flat_set_fluid(x, y, z, id, 0);
    wake_neighbor_liquids(x, y, z);
}

static void flat_set_block(int x, int y, int z, int id) {
    if (!flat_in_bounds(x, y, z)) return;

    flat_set_block_raw(x, y, z, id);

    /* Beta snow is a dependent layer.  If the supporting block is removed or
       replaced by something snow cannot sit on, neighbor update deletes the
       layer above.  This is not a player harvest, so no snowball is dropped. */
    if (y < FLAT_WORLD_Y_MAX && flat_get_block(x, y + 1, z) == BLOCK_SNOW_LAYER &&
        !flat_snow_layer_can_stay_at(x, y + 1, z)) {
        flat_set_block_raw(x, y + 1, z, 0);
    }

    wake_neighbor_liquids(x, y, z);
    wake_falling_blocks_around(x, y, z);
}




static int floor_div16(int v) {
    return v >= 0 ? v / 16 : -((15 - v) / 16);
}

static int stream_render_radius(void) {
    int chunks = g_opts.render_distance;
    if (chunks < 2) chunks = 2;
    int max_chunks = (FLAT_RENDER_CHUNKS / 2) - 2;
    if (max_chunks < 2) max_chunks = 2;
    if (chunks > max_chunks) chunks = max_chunks;
    return chunks;
}

static int stream_window_margin_blocks(void) {
    int margin = stream_render_radius() * FLAT_RENDER_CHUNK + FLAT_RENDER_CHUNK;
    int max_margin = FLAT_WORLD_SIZE / 2 - FLAT_RENDER_CHUNK;
    if (max_margin < FLAT_RENDER_CHUNK * 3) max_margin = FLAT_RENDER_CHUNK * 3;
    if (margin > max_margin) margin = max_margin;
    if (margin < FLAT_RENDER_CHUNK * 3) margin = FLAT_RENDER_CHUNK * 3;
    return margin;
}

/* Streaming terrain generation used to rebuild the full Beta TerrainProvider for
   every newly-exposed chunk.  That made walking into new chunks hammer the CPU.
   Keep one provider per seed and reuse its octave/noise buffers instead. */
static TerrainProvider g_beta_stream_tp;
static int g_beta_stream_tp_valid = 0;
static long long g_beta_stream_tp_seed = 0;

static TerrainProvider *beta_stream_provider(void) {
    if (!g_beta_stream_tp_valid || g_beta_stream_tp_seed != g_world_seed) {
        if (g_beta_stream_tp_valid) terrain_provider_free(&g_beta_stream_tp);
        terrain_provider_init(&g_beta_stream_tp, (int64_t)g_world_seed);
        g_beta_stream_tp_seed = g_world_seed;
        g_beta_stream_tp_valid = 1;
    }
    return &g_beta_stream_tp;
}

#if defined(PEX_PLATFORM_WII)
static int wii_copy_chunk_to_flat(int cx, int cz) {
    if (!stream_world_chunk_in_window(cx, cz, g_flat_world_origin_x, g_flat_world_origin_z)) return 0;
    int lcx = stream_local_chunk_x(cx);
    int lcz = stream_local_chunk_z(cz);
    if (!flat_local_chunk_valid(lcx, lcz)) return 0;
    wii_debug_logf("wii direct terrain begin wc=%d,%d local=%d,%d type=%d", cx, cz, lcx, lcz, g_world_type);

    for (int lx = 0; lx < FLAT_RENDER_CHUNK; ++lx) {
        int wx = cx * FLAT_RENDER_CHUNK + lx;
        if (wx < g_flat_world_origin_x || wx >= g_flat_world_origin_x + FLAT_WORLD_SIZE) continue;
        int fx = flat_index(wx);
        for (int lz = 0; lz < FLAT_RENDER_CHUNK; ++lz) {
            int wz = cz * FLAT_RENDER_CHUNK + lz;
            if (wz < g_flat_world_origin_z || wz >= g_flat_world_origin_z + FLAT_WORLD_SIZE) continue;
            int fz = flat_z_index(wz);
            int h = 3;
            if (g_world_type == 1) h = psp_safe_height_at(wx, wz, g_world_seed);
            if (h < 3) h = 3;
            if (h > FLAT_WORLD_Y_MAX - 2) h = FLAT_WORLD_Y_MAX - 2;

            for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; ++y) {
                int id = 0;
                if (y == FLAT_WORLD_Y_MIN) id = BLOCK_BEDROCK;
                else if (g_world_type == 1) {
                    if (y < h - 4) {
                        unsigned int oh = psp_safe_terrain_hash(wx, wz, y * 31 + 7, g_world_seed);
                        if (y < 18 && (oh % 900u) < 4u) id = BLOCK_DIAMOND_ORE;
                        else if (y < 32 && (oh % 360u) < 4u) id = BLOCK_GOLD_ORE;
                        else if (y < 45 && (oh % 180u) < 5u) id = BLOCK_IRON_ORE;
                        else if (y < 58 && (oh % 115u) < 6u) id = BLOCK_COAL_ORE;
                        else id = BLOCK_STONE;
                    } else if (y < h) id = BLOCK_DIRT;
                    else if (y == h) id = BLOCK_GRASS;
                    else if (y <= 30 && h < 30) id = (y == 30) ? BLOCK_STILL_WATER : BLOCK_WATER;
                } else {
                    if (y == 1 || y == 2) id = BLOCK_DIRT;
                    else if (y == 3) id = BLOCK_GRASS;
                }
                int yi = flat_y_index(y);
                g_flat_blocks[yi][fz][fx] = (unsigned char)id;
                g_flat_meta[yi][fz][fx] = 0;
                g_flat_levels[yi][fz][fx] = block_is_liquid(id) ? 0 : 0;
            }

            int light = 15;
            for (int y = FLAT_WORLD_Y_MAX; y >= FLAT_WORLD_Y_MIN; --y) {
                int yi = flat_y_index(y);
                int id = g_flat_blocks[yi][fz][fx];
                light = flat_sky_light_after_block(light, id);
                g_flat_sky_light[yi][fz][fx] = (unsigned char)(light & 15);
                g_flat_block_light[yi][fz][fx] = (unsigned char)(flat_block_light_value_for_id(id) & 15);
            }
        }
    }

    flat_refresh_chunk_occupancy(lcx, lcz);
    for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; ++sy) flat_mark_section_dirty(lcx, lcz, sy);
    g_flat_world_chunk_dirty[lcz][lcx] = 1;
    g_flat_world_chunk_valid[lcz][lcx] = 1;
    wii_debug_logf("wii direct terrain end wc=%d,%d", cx, cz);
    return 1;
}
#endif

static int beta_preview_copy_chunk_to_flat(int cx, int cz) {
#if defined(PEX_PLATFORM_WII)
    return wii_copy_chunk_to_flat(cx, cz);
#endif
    unsigned char *buf = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
    unsigned char *meta = (unsigned char*)calloc(FLAT_CHUNK_BLOCK_COUNT, 1);
    if (!buf || !meta) { free(buf); free(meta); return 0; }
    TerrainProvider *reuse = (g_world_type == 1) ? beta_stream_provider() : NULL;
    flat_generate_chunk_base_with_meta(cx, cz, buf, meta, g_world_type, g_world_seed, reuse);
    flat_copy_chunk_buffers(cx, cz, buf, meta);
    /* Overlay only gameplay edits.  Unchanged chunks live only as seed + coords. */
    flat_load_chunk_delta_for_dir(g_loaded_world_dir, cx, cz, buf, meta);
    flat_copy_chunk_buffers(cx, cz, buf, meta);
    free(buf);
    free(meta);
    return 1;
}


static void beta_preview_generate_world(void) {
    memset(g_flat_blocks, 0, sizeof(g_flat_blocks));
    memset(g_flat_meta, 0, sizeof(g_flat_meta));
    memset(g_flat_levels, 0, sizeof(g_flat_levels));
    memset(g_flat_chunk_section_non_empty_mask, 0, sizeof(g_flat_chunk_section_non_empty_mask));

    int min_cx = floor_div16(g_flat_world_origin_x);
    int max_cx = floor_div16(g_flat_world_origin_x + FLAT_WORLD_SIZE - 1);
    int min_cz = floor_div16(g_flat_world_origin_z);
    int max_cz = floor_div16(g_flat_world_origin_z + FLAT_WORLD_SIZE - 1);

#if defined(PEX_PLATFORM_WII) || (defined(PEX_PSP_1000_TARGET) && PEX_PSP_1000_TARGET && !(defined(PEX_PSP_REAL_BETA_GEN) && PEX_PSP_REAL_BETA_GEN))
    /* Wii/PSP safety: never allocate the full active-window Beta canvas.
       Generate one chunk at a time using the PSP-safe chunk-local generator. */
    for (int cz = min_cz; cz <= max_cz; cz++) {
        for (int cx = min_cx; cx <= max_cx; cx++) {
            beta_preview_copy_chunk_to_flat(cx, cz);
        }
    }
    return;
#endif

    /* Fast startup path: build one shared Beta canvas for the whole active
       in-memory window plus a 1-chunk population border.  The earlier per-chunk
       path recreated the terrain provider and a 3x3 canvas for every visible
       chunk, which is why opening a world paused for seconds.
       Keep TerrainProvider on the heap; it is far too large for PSP stacks. */
    TerrainProvider *tp = (TerrainProvider*)calloc(1, sizeof(*tp));
    if (!tp) {
        for (int cz = min_cz; cz <= max_cz; cz++) {
            for (int cx = min_cx; cx <= max_cx; cx++) {
                beta_preview_copy_chunk_to_flat(cx, cz);
            }
        }
        return;
    }
    terrain_provider_init(tp, (int64_t)g_world_seed);

    GenCanvas cv;
    cv.minCx = min_cx - 1;
    cv.minCz = min_cz - 1;
    int chunks_x = (max_cx - min_cx + 1) + 2;
    int chunks_z = (max_cz - min_cz + 1) + 2;
    cv.chunks = chunks_x; /* active window is square, so one dimension field is enough here */
    cv.blocks = NULL;
    cv.meta = NULL;

    if (chunks_x == chunks_z) {
        cv.blocks = (unsigned char*)calloc((size_t)cv.chunks * (size_t)cv.chunks * 32768u, 1);
        cv.meta = cv.blocks ? (unsigned char*)calloc((size_t)cv.chunks * (size_t)cv.chunks * 32768u, 1) : NULL;
    }

    if (cv.blocks) {
        for (int cz = cv.minCz; cz < cv.minCz + cv.chunks; cz++) {
            for (int cx = cv.minCx; cx < cv.minCx + cv.chunks; cx++) {
                generate_canvas_chunk(tp, &cv, cx, cz);
            }
        }

        for (int cz = min_cz - 1; cz <= max_cz; cz++) {
            for (int cx = min_cx - 1; cx <= max_cx; cx++) {
                qm_populate_canvas(tp, &cv, cx, cz);
            }
        }

        static unsigned char beta_blocks[32768];
        static unsigned char beta_data[16384];
        for (int cz = min_cz; cz <= max_cz; cz++) {
            for (int cx = min_cx; cx <= max_cx; cx++) {
                memset(beta_blocks, 0, sizeof(beta_blocks));
                memset(beta_data, 0, sizeof(beta_data));
                extract_canvas_chunk(&cv, cx, cz, beta_blocks, beta_data);
                for (int lx = 0; lx < 16; lx++) {
                    int wx = cx * 16 + lx;
                    if (wx < g_flat_world_origin_x || wx >= g_flat_world_origin_x + FLAT_WORLD_SIZE) continue;
                    int fx = flat_index(wx);
                    for (int lz = 0; lz < 16; lz++) {
                        int wz = cz * 16 + lz;
                        if (wz < g_flat_world_origin_z || wz >= g_flat_world_origin_z + FLAT_WORLD_SIZE) continue;
                        int fz = flat_z_index(wz);
                        for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; y++) {
                            int id = (y < 128) ? get_block_local(beta_blocks, lx, y, lz) : 0;
                            g_flat_blocks[flat_y_index(y)][fz][fx] = (unsigned char)id;
                            if (y < 128) {
                                int idx = chunk_index(lx, y, lz);
                                unsigned char packed = beta_data[idx >> 1];
                                g_flat_meta[flat_y_index(y)][fz][fx] = (unsigned char)((idx & 1) ? ((packed >> 4) & 15) : (packed & 15));
                                g_flat_levels[flat_y_index(y)][fz][fx] = block_is_liquid(id) ? (g_flat_meta[flat_y_index(y)][fz][fx] & 15) : 0;
                            }
                        }
                        if (g_flat_blocks[flat_y_index(0)][fz][fx] == 0) g_flat_blocks[flat_y_index(0)][fz][fx] = BLOCK_BEDROCK;
                    }
                }
                flat_load_chunk_delta(cx, cz);
                int lcx = stream_local_chunk_x(cx);
                int lcz = stream_local_chunk_z(cz);
                if (flat_local_chunk_valid(lcx, lcz)) flat_refresh_chunk_occupancy(lcx, lcz);
            }
        }
        free(cv.blocks);
        free(cv.meta);
    } else {
        /* Low-memory fallback keeps the old safe path. */
        for (int cz = min_cz; cz <= max_cz; cz++) {
            for (int cx = min_cx; cx <= max_cx; cx++) {
                beta_preview_copy_chunk_to_flat(cx, cz);
            }
        }
    }

    terrain_provider_free(tp);
    free(tp);
}


static unsigned int normal_world_hash(int x, int z, int salt) {
    unsigned int h = (unsigned int)(x * 374761393u + z * 668265263u + salt * 1442695041u + (unsigned int)g_world_seed);
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

static float normal_value_noise(int x, int z, int scale, int salt) {
    int gx = (x >= 0) ? x / scale : (x - scale + 1) / scale;
    int gz = (z >= 0) ? z / scale : (z - scale + 1) / scale;
    unsigned int h = normal_world_hash(gx, gz, salt);
    return ((float)(h & 65535) / 65535.0f) * 2.0f - 1.0f;
}

static int normal_ore_at(int x, int y, int z) {
    if (y < 5) return 0;
    unsigned int h = normal_world_hash(x, z, y * 31 + 7);
    if (y < 18 && (h % 900) < 4) return BLOCK_DIAMOND_ORE;
    if (y < 28 && (h % 420) < 5) return BLOCK_REDSTONE_ORE;
    if (y < 32 && (h % 360) < 4) return BLOCK_GOLD_ORE;
    if (y < 45 && (h % 180) < 5) return BLOCK_IRON_ORE;
    if (y < 58 && (h % 115) < 6) return BLOCK_COAL_ORE;
    return 0;
}

static void normal_place_tree(int x, int z, int ground_y) {
    int trunk_h = 4 + (int)(normal_world_hash(x, z, 99) % 3);
    if (ground_y + trunk_h + 2 > FLAT_WORLD_Y_MAX) return;
    for (int y = ground_y + 1; y <= ground_y + trunk_h; y++) {
        if (flat_get_block(x, y, z) != 0) return;
    }

    for (int y = ground_y + 1; y <= ground_y + trunk_h; y++) {
        g_flat_blocks[flat_y_index(y)][flat_z_index(z)][flat_index(x)] = BLOCK_LOG;
    }

    int top = ground_y + trunk_h;
    for (int yy = top - 2; yy <= top + 2; yy++) {
        int r = (yy >= top + 1) ? 1 : 2;
        for (int dz = -r; dz <= r; dz++) {
            for (int dx = -r; dx <= r; dx++) {
                if (abs(dx) == r && abs(dz) == r && normal_world_hash(x + dx, z + dz, yy) % 3 == 0) continue;
                int lx = x + dx, lz = z + dz;
                if (!flat_in_bounds(lx, yy, lz)) continue;
                if (flat_get_block(lx, yy, lz) == 0) {
                    g_flat_blocks[flat_y_index(yy)][flat_z_index(lz)][flat_index(lx)] = BLOCK_LEAVES;
                }
            }
        }
    }
}

static int normal_world_height_at(int x, int z) {
    /* Local b1.0-style preview: layered value noise, sea level 32, dirt/grass skin.
       Save folder still gets chunks from the existing Beta generator code. */
    float n1 = normal_value_noise(x, z, 18, 1) * 10.0f;
    float n2 = normal_value_noise(x, z, 7, 2) * 4.0f;
    float n3 = normal_value_noise(x, z, 37, 3) * 14.0f;
    int h = 32 + (int)(n1 + n2 + n3);
    if (h < 8) h = 8;
    if (h > 58) h = 58;
    return h;
}

static void flat_center_origin_near(float px, float pz) {
    int pcx = floor_div16((int)floorf(px));
    int pcz = floor_div16((int)floorf(pz));
    int base_cx = pcx - (FLAT_RENDER_CHUNKS / 2);
    int base_cz = pcz - (FLAT_RENDER_CHUNKS / 2);
    g_flat_world_origin_x = base_cx * FLAT_RENDER_CHUNK;
    g_flat_world_origin_z = base_cz * FLAT_RENDER_CHUNK;
    g_stream_last_center_chunk_x = 999999;
    g_stream_last_center_chunk_z = 999999;
}

static void flat_prepare_initial_generation(void) {
    memset(g_flat_blocks, 0, sizeof(g_flat_blocks));
    memset(g_flat_meta, 0, sizeof(g_flat_meta));
    memset(g_flat_levels, 0, sizeof(g_flat_levels));
    memset(g_flat_sky_light, 0, sizeof(g_flat_sky_light));
    memset(g_flat_block_light, 0, sizeof(g_flat_block_light));
    g_flat_light_dirty = 0;
    memset(g_flat_world_chunk_modified, 0, sizeof(g_flat_world_chunk_modified));
    memset(g_flat_world_chunk_generated, 0, sizeof(g_flat_world_chunk_generated));
    memset(g_flat_chunk_light_ready, 0, sizeof(g_flat_chunk_light_ready));
    memset(g_flat_chunk_light_version, 0, sizeof(g_flat_chunk_light_version));
    memset(g_flat_chunk_initial_preload, 0, sizeof(g_flat_chunk_initial_preload));
    memset(g_flat_section_mesh_light_version, 0, sizeof(g_flat_section_mesh_light_version));
    memset(g_flat_chunk_section_non_empty_mask, 0, sizeof(g_flat_chunk_section_non_empty_mask));
    stream_generation_queue_clear();
    for (int cz = 0; cz < FLAT_RENDER_CHUNKS; cz++) {
        for (int cx = 0; cx < FLAT_RENDER_CHUNKS; cx++) {
            g_flat_world_chunk_dirty[cz][cx] = 1;
            g_flat_world_chunk_valid[cz][cx] = 0;
            g_flat_world_chunk_has_liquid[cz][cx] = 0;
            for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
                g_flat_section_dirty[sy][cz][cx] = 0;
                g_flat_section_mesh_version[sy][cz][cx] = 1;
                g_flat_section_mesh_building[sy][cz][cx] = 0;
                g_flat_section_valid[sy][cz][cx] = 1;
                g_flat_section_skip_pass[sy][cz][cx][0] = 1;
                g_flat_section_skip_pass[sy][cz][cx][1] = 1;
            }
        }
    }
    g_flat_world_geometry_dirty = 0;
    g_flat_section_geometry_dirty = 0;
}

static int stream_initial_load_chunk_target(void) {
#if defined(PEX_PLATFORM_WII)
    return 9;
#elif defined(PEX_PSP_1000_TARGET) && PEX_PSP_1000_TARGET
    return 25;
#else
    /* Desktop/Android loading screen target requested by the user: publish up
       to 50 missing chunks before entering gameplay.  Only missing chunks are
       queued; already-loaded/generated chunks are skipped by stream_queue_add. */
    return 50;
#endif
}

static volatile int g_stream_initial_light_settle_requested = 0;
static volatile int g_stream_initial_light_settle_running = 0;
static volatile int g_stream_initial_light_settle_done = 0;
static volatile int g_stream_initial_light_settle_progress = 0;
/* Loading-screen terrain generation may run as one Beta canvas batch on the
   stream service thread.  Keep it visible to worldgen_tick/stream_generation_active
   so the progress screen does not advance to lighting while the batch is still
   extracting/publishing chunks. */
static volatile int g_stream_initial_batch_running = 0;
static volatile int g_stream_initial_batch_done_units = 0;
static volatile int g_stream_initial_batch_total_units = 0;

typedef struct StreamInitialBetaBatchState {
    int active;
    int phase;
    int min_cx, min_cz, max_cx, max_cz;
    int chunks_x, chunks_z, canvas_chunks;
    int gen_index;
    int populate_index;
    int extract_index;
    TerrainProvider *tp;
    unsigned char *beta_blocks;
    unsigned char *beta_data;
    unsigned char *buf;
    unsigned char *meta;
    GenCanvas cv;
} StreamInitialBetaBatchState;

static StreamInitialBetaBatchState g_stream_initial_beta_batch;

static void stream_initial_beta_batch_free(void) {
    StreamInitialBetaBatchState *s = &g_stream_initial_beta_batch;
    if (s->tp) {
        terrain_provider_free(s->tp);
        free(s->tp);
    }
    free(s->cv.blocks);
    free(s->cv.meta);
    free(s->beta_blocks);
    free(s->beta_data);
    free(s->buf);
    free(s->meta);
    memset(s, 0, sizeof(*s));
    g_stream_initial_batch_running = 0;
}

static void stream_reset_initial_batch(void) {
    stream_initial_beta_batch_free();
    g_stream_initial_batch_done_units = 0;
    g_stream_initial_batch_total_units = 0;
}

static void flat_begin_initial_generation(void) {
    stream_generation_queue_clear();
    g_stream_generation_keep_completed = 1;
    g_stream_initial_light_settle_requested = 0;
    g_stream_initial_light_settle_running = 0;
    g_stream_initial_light_settle_done = 0;
    g_stream_initial_light_settle_progress = 0;
    stream_reset_initial_batch();

    int base_cx = floor_div16(g_flat_world_origin_x);
    int base_cz = floor_div16(g_flat_world_origin_z);
    int player_wcx = floor_div16((int)floorf(g_player_x));
    int player_wcz = floor_div16((int)floorf(g_player_z));
    int pcx = player_wcx - base_cx;
    int pcz = player_wcz - base_cz;
    if (pcx < 0) pcx = 0;
    if (pcz < 0) pcz = 0;
    if (pcx >= FLAT_RENDER_CHUNKS) pcx = FLAT_RENDER_CHUNKS - 1;
    if (pcz >= FLAT_RENDER_CHUNKS) pcz = FLAT_RENDER_CHUNKS - 1;

    int target = stream_initial_load_chunk_target();
    for (int ring = 0; ring < FLAT_RENDER_CHUNKS && g_stream_gen_queue_count < target; ring++) {
        /* Release 1.2.5 preloads a square/ring around spawn.  Do not seed the
           loading-screen queue with the in-game directional corridor: that can
           create holes inside the preload bounds, and those holes make the final
           skylight settle treat missing chunks as air.  Keep only complete rings
           so the first visible world is a stable, contiguous spawn island. */
        int ring_slots = (ring == 0) ? 1 : ring * 8;
        if (ring > 0 && g_stream_gen_queue_count + ring_slots > target) break;
        for (int dz = -ring; dz <= ring && g_stream_gen_queue_count < target; dz++) {
            for (int dx = -ring; dx <= ring && g_stream_gen_queue_count < target; dx++) {
                if (ring != 0 && abs(dx) != ring && abs(dz) != ring) continue;
                int lcx = pcx + dx;
                int lcz = pcz + dz;
                if (lcx < 0 || lcx >= FLAT_RENDER_CHUNKS || lcz < 0 || lcz >= FLAT_RENDER_CHUNKS) continue;
                stream_queue_add_chunk(base_cx + lcx, base_cz + lcz);
            }
        }
    }

    /* Java 1.2.5 preloads chunks while repeatedly updating the loading screen.
       PexCraft keeps the same visible-progress behavior without moving terrain
       generation onto the main/render thread: the background stream service owns
       generation and commit while worldgen_tick only polls counters. */
    world_stream_service_ensure();
}

static void flat_continue_initial_generation(void) {
    world_stream_service_ensure();
}

static int flat_initial_generation_active(void) {
    return stream_generation_active();
}

static int flat_initial_generation_total(void) {
    if (g_stream_generation_keep_completed && g_world_type == 1 && g_stream_initial_batch_total_units > 0) {
        return g_stream_initial_batch_total_units;
    }
    return g_stream_gen_queue_count;
}

static int flat_initial_generation_done(void) {
    if (g_stream_generation_keep_completed && g_world_type == 1 && g_stream_initial_batch_total_units > 0) {
        int done = g_stream_initial_batch_done_units;
        int total = g_stream_initial_batch_total_units;
        if (done < 0) done = 0;
        if (done > total) done = total;
        return done;
    }
    return g_stream_gen_queue_installed_count;
}

static void flat_begin_initial_light_settle(void) {
    if (g_stream_initial_light_settle_done || g_stream_initial_light_settle_running) return;
    g_stream_initial_light_settle_requested = 1;
}

static int flat_initial_light_settle_done(void) {
    return g_stream_initial_light_settle_done;
}

static int flat_initial_light_settle_progress(void) {
    int p = g_stream_initial_light_settle_progress;
    if (p < 0) p = 0;
    if (p > 100) p = 100;
    return p;
}

static void flat_finish_initial_generation(void) {
    g_stream_generation_keep_completed = 0;
    g_stream_initial_light_settle_requested = 0;
    g_stream_initial_light_settle_running = 0;
    g_stream_initial_light_settle_done = 0;
    g_stream_initial_light_settle_progress = 0;
    stream_reset_initial_batch();
    stream_generation_queue_clear();
    g_flat_world_geometry_dirty = 0;
    g_flat_section_geometry_dirty = 0;
}

static void flat_generate_origin_blocks(void) {
    memset(g_flat_blocks, 0, sizeof(g_flat_blocks));
    memset(g_flat_meta, 0, sizeof(g_flat_meta));
    memset(g_flat_levels, 0, sizeof(g_flat_levels));
    memset(g_flat_sky_light, 0, sizeof(g_flat_sky_light));
    memset(g_flat_block_light, 0, sizeof(g_flat_block_light));
    g_flat_light_dirty = 0;
    memset(g_flat_world_chunk_modified, 0, sizeof(g_flat_world_chunk_modified));
    memset(g_flat_world_chunk_generated, 0, sizeof(g_flat_world_chunk_generated));
    memset(g_flat_chunk_light_ready, 0, sizeof(g_flat_chunk_light_ready));
    memset(g_flat_chunk_light_version, 0, sizeof(g_flat_chunk_light_version));
    memset(g_flat_chunk_initial_preload, 0, sizeof(g_flat_chunk_initial_preload));
    memset(g_flat_section_mesh_light_version, 0, sizeof(g_flat_section_mesh_light_version));
    memset(g_flat_chunk_section_non_empty_mask, 0, sizeof(g_flat_chunk_section_non_empty_mask));
    stream_generation_queue_clear();

    if (g_world_type == 1) {
        /* Exact same Beta 1.0 generator as before.  The difference is that the
           result is not saved unless gameplay edits the chunk. */
        beta_preview_generate_world();
        for (int lcz = 0; lcz < FLAT_RENDER_CHUNKS; lcz++)
            for (int lcx = 0; lcx < FLAT_RENDER_CHUNKS; lcx++) {
                stream_mark_chunk_generated(lcx, lcz);
            }
    } else {
        int min_cx = floor_div16(g_flat_world_origin_x);
        int max_cx = floor_div16(g_flat_world_origin_x + FLAT_WORLD_SIZE - 1);
        int min_cz = floor_div16(g_flat_world_origin_z);
        int max_cz = floor_div16(g_flat_world_origin_z + FLAT_WORLD_SIZE - 1);
        unsigned char *buf = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
        if (buf) {
            for (int cz = min_cz; cz <= max_cz; cz++) {
                for (int cx = min_cx; cx <= max_cx; cx++) {
                    flat_generate_chunk_base_to_buffer(cx, cz, buf);
                    flat_copy_chunk_buffer(cx, cz, buf);
                    flat_load_chunk_delta(cx, cz);
                    int lcx = stream_local_chunk_x(cx);
                    int lcz = stream_local_chunk_z(cz);
                    if (lcx >= 0 && lcx < FLAT_RENDER_CHUNKS && lcz >= 0 && lcz < FLAT_RENDER_CHUNKS) {
                        stream_mark_chunk_generated(lcx, lcz);
                    }
                }
            }
            free(buf);
        }
    }

    flat_mark_all_chunks_dirty();
}


/* Debug/QA teleport helper: build only the destination neighborhood instead of
   calling flat_generate_origin_blocks().  The normal world-type-1 preload path is
   intentionally queue based; forcing that path from /traceplace left the window
   marked as generated before any real terrain/structures were copied in, so the
   command could land the player in an empty/flat-looking area with no village. */
static void flat_generate_traceplace_area(int center_wcx, int center_wcz, int radius_chunks) {
    if (radius_chunks < 2) radius_chunks = 2;
    if (radius_chunks > 6) radius_chunks = 6;

    memset(g_flat_blocks, 0, sizeof(g_flat_blocks));
    memset(g_flat_meta, 0, sizeof(g_flat_meta));
    memset(g_flat_levels, 0, sizeof(g_flat_levels));
    memset(g_flat_sky_light, 0, sizeof(g_flat_sky_light));
    memset(g_flat_block_light, 0, sizeof(g_flat_block_light));
    g_flat_light_dirty = 0;
    memset(g_flat_world_chunk_modified, 0, sizeof(g_flat_world_chunk_modified));
    memset(g_flat_world_chunk_generated, 0, sizeof(g_flat_world_chunk_generated));
    memset(g_flat_chunk_light_ready, 0, sizeof(g_flat_chunk_light_ready));
    memset(g_flat_chunk_light_version, 0, sizeof(g_flat_chunk_light_version));
    memset(g_flat_chunk_initial_preload, 0, sizeof(g_flat_chunk_initial_preload));
    memset(g_flat_section_mesh_light_version, 0, sizeof(g_flat_section_mesh_light_version));
    memset(g_flat_chunk_section_non_empty_mask, 0, sizeof(g_flat_chunk_section_non_empty_mask));
    stream_reset_initial_batch();
    stream_generation_queue_clear();

    int min_cx = center_wcx - radius_chunks;
    int max_cx = center_wcx + radius_chunks;
    int min_cz = center_wcz - radius_chunks;
    int max_cz = center_wcz + radius_chunks;
    int base_cx = floor_div16(g_flat_world_origin_x);
    int base_cz = floor_div16(g_flat_world_origin_z);
    int window_max_cx = base_cx + FLAT_RENDER_CHUNKS - 1;
    int window_max_cz = base_cz + FLAT_RENDER_CHUNKS - 1;
    if (min_cx < base_cx) min_cx = base_cx;
    if (min_cz < base_cz) min_cz = base_cz;
    if (max_cx > window_max_cx) max_cx = window_max_cx;
    if (max_cz > window_max_cz) max_cz = window_max_cz;

    unsigned char *buf = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
    unsigned char *meta = (unsigned char*)calloc(FLAT_CHUNK_BLOCK_COUNT, 1);
    TerrainProvider *reuse = NULL;
#if !(defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII))
    reuse = (g_world_type == 1) ? beta_stream_provider() : NULL;
#endif
    if (buf && meta) {
        for (int cz = min_cz; cz <= max_cz; ++cz) {
            for (int cx = min_cx; cx <= max_cx; ++cx) {
                flat_generate_chunk_base_with_meta(cx, cz, buf, meta, g_world_type, g_world_seed, reuse);
                flat_load_chunk_delta_for_dir(g_loaded_world_dir, cx, cz, buf, meta);
                flat_copy_chunk_buffers(cx, cz, buf, meta);
                stream_mark_chunk_generated(stream_local_chunk_x(cx), stream_local_chunk_z(cz));
            }
        }
    }
    free(buf);
    free(meta);

    flat_mark_all_chunks_dirty();
    flat_lighting_worker_wake();
}

static void flat_world_reset_blocks(void) {
    flat_center_origin_near(g_player_x, g_player_z);
    g_stream_gen_queue_count = 0;
    g_stream_gen_queue_index = 0;

    flat_generate_origin_blocks();

    memset(g_flat_levels, 0, sizeof(g_flat_levels));
    memset(g_drops, 0, sizeof(g_drops));
    memset(g_vehicles, 0, sizeof(g_vehicles));
    memset(g_jukebox_tiles, 0, sizeof(g_jukebox_tiles));
    memset(g_player_potion_effects, 0, sizeof(g_player_potion_effects));
    memset(g_map_data, 0, sizeof(g_map_data));
    g_next_map_id = 0;
    memset(g_projectiles, 0, sizeof(g_projectiles));
    memset(g_xp_orbs, 0, sizeof(g_xp_orbs));
    memset(g_falling_blocks, 0, sizeof(g_falling_blocks));
    memset(g_button_timers, 0, sizeof(g_button_timers));
    memset(g_pressure_plate_timers, 0, sizeof(g_pressure_plate_timers));
    memset(g_dig_particles, 0, sizeof(g_dig_particles));
    g_next_dig_particle = 0;
}


static int block_is_liquid(int id) {
    return id == BLOCK_WATER || id == BLOCK_STILL_WATER ||
           id == BLOCK_LAVA || id == BLOCK_STILL_LAVA;
}

static int block_is_water(int id) {
    return id == BLOCK_WATER || id == BLOCK_STILL_WATER;
}

static int block_is_lava(int id) {
    return id == BLOCK_LAVA || id == BLOCK_STILL_LAVA;
}

static int block_is_source_liquid(int id) {
    return id == BLOCK_STILL_WATER || id == BLOCK_STILL_LAVA;
}

static int block_is_flowing_liquid(int id) {
    return id == BLOCK_WATER || id == BLOCK_LAVA;
}

static int fluid_flow_id(int is_water) {
    return is_water ? BLOCK_WATER : BLOCK_LAVA;
}

static int fluid_still_id(int is_water) {
    return is_water ? BLOCK_STILL_WATER : BLOCK_STILL_LAVA;
}

static int block_material_blocks_liquid(int id) {
    if (id == 0 || block_is_liquid(id)) return 0;
    if (id == BLOCK_WOOD_DOOR || id == BLOCK_IRON_DOOR || id == BLOCK_SIGN_POST ||
        id == BLOCK_WALL_SIGN || id == BLOCK_LADDER || id == BLOCK_REEDS) return 1;
    if (id == BLOCK_SAPLING || id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE ||
        id == BLOCK_BROWN_MUSHROOM || id == BLOCK_RED_MUSHROOM || id == BLOCK_TORCH ||
        id == BLOCK_FIRE || id == BLOCK_REDSTONE_WIRE || id == BLOCK_REDSTONE_TORCH_OFF ||
        id == BLOCK_REDSTONE_TORCH_ON || id == BLOCK_SNOW_LAYER || id == BLOCK_CROPS ||
        id == BLOCK_TALL_GRASS || id == BLOCK_DEAD_BUSH || id == BLOCK_VINE ||
        id == BLOCK_LILY_PAD || id == BLOCK_NETHER_WART || id == BLOCK_PUMPKIN_STEM ||
        id == BLOCK_MELON_STEM || id == BLOCK_RAILS || id == BLOCK_POWERED_RAIL ||
        id == BLOCK_DETECTOR_RAIL || id == BLOCK_LEVER || id == BLOCK_STONE_BUTTON ||
        id == BLOCK_STONE_PRESSURE_PLATE || id == BLOCK_WOOD_PRESSURE_PLATE) return 0;
    return 1;
}

static void fluid_wake_at(int x, int y, int z) {
    if (!flat_in_bounds(x, y, z)) return;
    int id = flat_get_block(x, y, z);
    if (id == BLOCK_STILL_WATER || id == BLOCK_STILL_LAVA) {
        int level = flat_get_level(x, y, z);
        int yi = flat_y_index(y), zi = flat_z_index(z), xi = flat_index(x);
        g_flat_blocks[yi][zi][xi] = (unsigned char)(id == BLOCK_STILL_WATER ? BLOCK_WATER : BLOCK_LAVA);
        g_flat_levels[yi][zi][xi] = (unsigned char)(level & 15);
        g_flat_meta[yi][zi][xi] = (unsigned char)(level & 15);
        flat_mark_sections_dirty_near_block(x, y, z);
        if (flat_persistent_edit_active()) {
            mark_flat_chunk_modified_at(x, z);
            g_save_dirty = 1;
        }
    }
}

static void wake_neighbor_liquids(int x, int y, int z) {
    fluid_wake_at(x - 1, y, z);
    fluid_wake_at(x + 1, y, z);
    fluid_wake_at(x, y, z - 1);
    fluid_wake_at(x, y, z + 1);
    fluid_wake_at(x, y - 1, z);
    fluid_wake_at(x, y + 1, z);
}

static void fluid_check_for_harden(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (!block_is_lava(id)) return;

    int touches_water =
        block_is_water(flat_get_block(x - 1, y, z)) ||
        block_is_water(flat_get_block(x + 1, y, z)) ||
        block_is_water(flat_get_block(x, y, z - 1)) ||
        block_is_water(flat_get_block(x, y, z + 1)) ||
        block_is_water(flat_get_block(x, y + 1, z));
    if (!touches_water) return;

    int level = flat_get_level(x, y, z);
    if (level == 0) flat_set_block_raw(x, y, z, BLOCK_OBSIDIAN);
    else if (level <= 4) flat_set_block_raw(x, y, z, BLOCK_COBBLESTONE);
}

static int flat_block_is_solid(int id) {
    return id != 0 && !block_is_liquid(id) &&
           id != BLOCK_SAPLING && id != BLOCK_YELLOW_FLOWER && id != BLOCK_RED_ROSE &&
           id != BLOCK_BROWN_MUSHROOM && id != BLOCK_RED_MUSHROOM &&
           id != BLOCK_TORCH && id != BLOCK_FIRE && id != BLOCK_REDSTONE_WIRE &&
           id != BLOCK_REDSTONE_TORCH_OFF && id != BLOCK_REDSTONE_TORCH_ON &&
           id != BLOCK_REEDS && id != BLOCK_SNOW_LAYER && id != BLOCK_LADDER &&
           id != BLOCK_TALL_GRASS && id != BLOCK_DEAD_BUSH && id != BLOCK_VINE &&
           id != BLOCK_LILY_PAD && id != BLOCK_NETHER_WART && id != BLOCK_PUMPKIN_STEM &&
           id != BLOCK_MELON_STEM && id != BLOCK_RAILS && id != BLOCK_POWERED_RAIL &&
           id != BLOCK_DETECTOR_RAIL && id != BLOCK_LEVER && id != BLOCK_STONE_BUTTON &&
           id != BLOCK_STONE_PRESSURE_PLATE && id != BLOCK_WOOD_PRESSURE_PLATE &&
           id != BLOCK_GLASS_PANE && id != BLOCK_IRON_BARS && id != BLOCK_END_PORTAL &&
           id != BLOCK_BREWING_STAND && id != BLOCK_CAULDRON && id != BLOCK_TRAPDOOR &&
           id != BLOCK_SIGN_POST && id != BLOCK_WALL_SIGN && !block_is_door_id(id);
}

static int door_is_open_at(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (!block_is_door_id(id)) return 0;
    int ly = door_lower_y_at(x, y, z);
    return door_meta_is_open(flat_get_meta(x, ly, z));
}

static int aabb_intersects_box(float minx, float maxx, float miny, float maxy, float minz, float maxz,
                                float x0, float y0, float z0, float x1, float y1, float z1) {
    return maxx > x0 && minx < x1 && maxy > y0 && miny < y1 && maxz > z0 && minz < z1;
}

static int door_thin_aabb_intersects(float minx, float maxx, float miny, float maxy, float minz, float maxz, int x, int y, int z) {
    int ly = door_lower_y_at(x, y, z);
    int dir = door_collision_state_from_meta(flat_get_meta(x, ly, z));
    float t = 3.0f / 16.0f;
    float x0 = (float)x, x1 = (float)(x + 1);
    float z0 = (float)z, z1 = (float)(z + 1);
    if (dir == 0) z1 = z0 + t;          /* north */
    else if (dir == 2) z0 = z1 - t;     /* south */
    else if (dir == 1) x0 = x1 - t;     /* east */
    else x1 = x0 + t;                   /* west */
    return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x0, (float)y, z0, x1, (float)(y + 1), z1);
}

static int ladder_thin_aabb_intersects(float minx, float maxx, float miny, float maxy, float minz, float maxz,
                                       int x, int y, int z, float inflate) {
    int meta = flat_get_meta(x, y, z);
    if (meta < 2 || meta > 5) meta = 3;
    float t = 2.0f / 16.0f;
    float x0 = (float)x, x1 = (float)(x + 1);
    float z0 = (float)z, z1 = (float)(z + 1);
    if (meta == 2) z0 = z1 - t;
    else if (meta == 3) z1 = z0 + t;
    else if (meta == 4) x0 = x1 - t;
    else if (meta == 5) x1 = x0 + t;
    x0 -= inflate; x1 += inflate; z0 -= inflate; z1 += inflate;
    return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x0, (float)y, z0, x1, (float)(y + 1), z1);
}

static int flat_block_occludes_for_support(int id);

static int block_custom_collision_intersects(int id, float minx, float maxx, float miny, float maxy, float minz, float maxz, int x, int y, int z) {
    if (id == BLOCK_LADDER) return ladder_thin_aabb_intersects(minx, maxx, miny, maxy, minz, maxz, x, y, z, 0.0f);
    if (id == BLOCK_SLAB) return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x, y, z, x + 1.0f, y + 0.5f, z + 1.0f);
    if (id == BLOCK_CHEST) return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x + 1.0f/16.0f, y, z + 1.0f/16.0f, x + 15.0f/16.0f, y + 14.0f/16.0f, z + 15.0f/16.0f);
    if (id == BLOCK_CACTUS) return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x + 1.0f/16.0f, y, z + 1.0f/16.0f, x + 15.0f/16.0f, y + 1.0f, z + 15.0f/16.0f);
    if (id == BLOCK_WOOD_STAIRS || id == BLOCK_COBBLE_STAIRS || id == BLOCK_BRICK_STAIRS ||
        id == BLOCK_STONE_BRICK_STAIRS || id == BLOCK_NETHER_BRICK_STAIRS) {
        if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x, y, z, x + 1.0f, y + 0.5f, z + 1.0f)) return 1;
        int dir = flat_get_meta(x, y, z) & 3;
        /* Beta BlockStairs metadata collision: 0=east half tall,
           1=west half tall, 2=south half tall, 3=north half tall. */
        if (dir == 0) return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x + 0.5f, y + 0.5f, z, x + 1.0f, y + 1.0f, z + 1.0f);
        if (dir == 1) return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x, y + 0.5f, z, x + 0.5f, y + 1.0f, z + 1.0f);
        if (dir == 2) return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x, y + 0.5f, z + 0.5f, x + 1.0f, y + 1.0f, z + 1.0f);
        return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x, y + 0.5f, z, x + 1.0f, y + 1.0f, z + 0.5f);
    }
    if (id == BLOCK_FENCE || id == BLOCK_NETHER_BRICK_FENCE) {
        if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x + 0.375f, y, z + 0.375f, x + 0.625f, y + 1.5f, z + 0.625f)) return 1;
        if (flat_block_is_solid(flat_get_block(x - 1, y, z)) || flat_get_block(x - 1, y, z) == BLOCK_FENCE || flat_get_block(x - 1, y, z) == BLOCK_NETHER_BRICK_FENCE)
            if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x, y, z + 0.375f, x + 0.5f, y + 1.5f, z + 0.625f)) return 1;
        if (flat_block_is_solid(flat_get_block(x + 1, y, z)) || flat_get_block(x + 1, y, z) == BLOCK_FENCE || flat_get_block(x + 1, y, z) == BLOCK_NETHER_BRICK_FENCE)
            if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x + 0.5f, y, z + 0.375f, x + 1.0f, y + 1.5f, z + 0.625f)) return 1;
        if (flat_block_is_solid(flat_get_block(x, y, z - 1)) || flat_get_block(x, y, z - 1) == BLOCK_FENCE || flat_get_block(x, y, z - 1) == BLOCK_NETHER_BRICK_FENCE)
            if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x + 0.375f, y, z, x + 0.625f, y + 1.5f, z + 0.5f)) return 1;
        if (flat_block_is_solid(flat_get_block(x, y, z + 1)) || flat_get_block(x, y, z + 1) == BLOCK_FENCE || flat_get_block(x, y, z + 1) == BLOCK_NETHER_BRICK_FENCE)
            if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x + 0.375f, y, z + 0.5f, x + 0.625f, y + 1.5f, z + 1.0f)) return 1;
        return 0;
    }
    if (id == BLOCK_WEB) return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x, y, z, x + 1.0f, y + 1.0f, z + 1.0f);
    if (id == BLOCK_LILY_PAD) return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x, y, z, x + 1.0f, y + 0.0625f, z + 1.0f);
    if (id == BLOCK_GLASS_PANE || id == BLOCK_IRON_BARS) {
        if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x + 0.4375f, y, z + 0.4375f, x + 0.5625f, y + 1.0f, z + 0.5625f)) return 1;
        if (flat_block_occludes_for_support(flat_get_block(x - 1, y, z)) || flat_get_block(x - 1, y, z) == id)
            if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x, y, z + 0.4375f, x + 0.5f, y + 1.0f, z + 0.5625f)) return 1;
        if (flat_block_occludes_for_support(flat_get_block(x + 1, y, z)) || flat_get_block(x + 1, y, z) == id)
            if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x + 0.5f, y, z + 0.4375f, x + 1.0f, y + 1.0f, z + 0.5625f)) return 1;
        if (flat_block_occludes_for_support(flat_get_block(x, y, z - 1)) || flat_get_block(x, y, z - 1) == id)
            if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x + 0.4375f, y, z, x + 0.5625f, y + 1.0f, z + 0.5f)) return 1;
        if (flat_block_occludes_for_support(flat_get_block(x, y, z + 1)) || flat_get_block(x, y, z + 1) == id)
            if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x + 0.4375f, y, z + 0.5f, x + 0.5625f, y + 1.0f, z + 1.0f)) return 1;
        return 0;
    }
    if (id == BLOCK_END_PORTAL_FRAME) return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x, y, z, x + 1.0f, y + 13.0f/16.0f, z + 1.0f);
    if (id == BLOCK_TRAPDOOR) return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz, x, y, z, x + 1.0f, y + 3.0f/16.0f, z + 1.0f);
    return 0;
}

static int block_has_custom_collision(int id) {
    return id == BLOCK_LADDER || id == BLOCK_SLAB || id == BLOCK_CHEST || id == BLOCK_CACTUS ||
           id == BLOCK_WOOD_STAIRS || id == BLOCK_COBBLE_STAIRS || id == BLOCK_BRICK_STAIRS ||
           id == BLOCK_STONE_BRICK_STAIRS || id == BLOCK_NETHER_BRICK_STAIRS ||
           id == BLOCK_FENCE || id == BLOCK_NETHER_BRICK_FENCE || id == BLOCK_GLASS_PANE ||
           id == BLOCK_IRON_BARS || id == BLOCK_LILY_PAD || id == BLOCK_END_PORTAL_FRAME ||
           id == BLOCK_WEB || id == BLOCK_TRAPDOOR;
}

static int flat_player_in_ladder(void) {
    float feet = g_player_y - 1.62f;
    float minx = g_player_x - 0.31f, maxx = g_player_x + 0.31f;
    float miny = feet + 0.05f, maxy = feet + 1.75f;
    float minz = g_player_z - 0.31f, maxz = g_player_z + 0.31f;
    int x0 = (int)floorf(minx), x1 = (int)floorf(maxx);
    int y0 = (int)floorf(miny), y1 = (int)floorf(maxy);
    int z0 = (int)floorf(minz), z1 = (int)floorf(maxz);
    for (int y = y0; y <= y1; y++) for (int z = z0; z <= z1; z++) for (int x = x0; x <= x1; x++) {
        if (flat_get_block(x, y, z) == BLOCK_LADDER && ladder_thin_aabb_intersects(minx, maxx, miny, maxy, minz, maxz, x, y, z, 0.06f)) return 1;
    }
    return 0;
}

static int flat_solid_for_spawn(int id) {
    return flat_block_is_solid(id);
}

#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
static int psp_spawn_surface_marked_at(int x, int y, int z) {
    if (!g_psp_spawn_surface_guard_active) return 0;
    if (y != g_psp_spawn_surface_ground_y) return 0;
    if (x < g_psp_spawn_surface_x0 || x > g_psp_spawn_surface_x1 ||
        z < g_psp_spawn_surface_z0 || z > g_psp_spawn_surface_z1) return 0;
    return flat_get_block(x, y, z) == BLOCK_GRASS &&
           (flat_get_meta(x, y, z) & 15) == PSP_SPAWN_SURFACE_META;
}

static int psp_spawn_surface_intersects(float minx, float maxx, float miny, float maxy,
                                        float minz, float maxz, int x, int y, int z) {
    if (!psp_spawn_surface_marked_at(x, y, z)) return 0;
    return aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz,
                               (float)x, (float)y, (float)z,
                               (float)(x + 1), (float)(y + 1), (float)(z + 1));
}

static void psp_set_spawn_surface_guard(int cx, int ground_y, int cz, int radius) {
    if (ground_y < FLAT_WORLD_Y_MIN + 1) ground_y = FLAT_WORLD_Y_MIN + 1;
    if (ground_y > FLAT_WORLD_Y_MAX - 4) ground_y = FLAT_WORLD_Y_MAX - 4;
    if (radius < 1) radius = 1;
    g_psp_spawn_surface_guard_active = 1;
    g_psp_spawn_surface_x0 = cx - radius;
    g_psp_spawn_surface_x1 = cx + radius;
    g_psp_spawn_surface_z0 = cz - radius;
    g_psp_spawn_surface_z1 = cz + radius;
    g_psp_spawn_surface_ground_y = ground_y;
}

static void psp_make_spawn_surface(int cx, int ground_y, int cz, int radius) {
    if (ground_y < FLAT_WORLD_Y_MIN + 1) ground_y = FLAT_WORLD_Y_MIN + 1;
    if (ground_y > FLAT_WORLD_Y_MAX - 4) ground_y = FLAT_WORLD_Y_MAX - 4;
    if (radius < 1) radius = 1;
    if (radius > 4) radius = 4;

    psp_set_spawn_surface_guard(cx, ground_y, cz, radius);

    for (int dz = -radius; dz <= radius; dz++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int x = cx + dx;
            int z = cz + dz;
            if (!flat_in_bounds(x, ground_y, z)) continue;

            /* Build a real, flat, solid top block and tag it with metadata so
               the PSP spawn/collision path knows this is the safety floor. */
            flat_set_block_raw(x, ground_y, z, BLOCK_GRASS);
            flat_set_meta_raw(x, ground_y, z, PSP_SPAWN_SURFACE_META);

            for (int y = ground_y - 1; y >= ground_y - 3 && y >= FLAT_WORLD_Y_MIN; y--) {
                int id = flat_get_block(x, y, z);
                if (id == 0 || block_is_liquid(id) || id == BLOCK_LEAVES) flat_set_block_raw(x, y, z, BLOCK_DIRT);
            }
            for (int y = ground_y + 1; y <= ground_y + 3 && y <= FLAT_WORLD_Y_MAX; y++) {
                int id = flat_get_block(x, y, z);
                if (id == BLOCK_LEAVES || block_is_liquid(id) || flat_block_is_solid(id)) {
                    flat_set_block_raw(x, y, z, 0);
                }
            }
        }
    }

    flat_mark_chunks_dirty_near(cx, cz);
    flat_mark_light_dirty_region(cx - radius - 1, cz - radius - 1, cx + radius + 1, cz + radius + 1);
}

static int psp_highest_surface_foot_y(int x, int z, int *out_foot_y) {
    if (!flat_chunk_generated_at_block(x, z)) return 0;
    for (int y = FLAT_WORLD_Y_MAX - 3; y >= FLAT_WORLD_Y_MIN; y--) {
        int id = flat_get_block(x, y, z);
        if (!flat_solid_for_spawn(id) || id == BLOCK_LEAVES || block_is_liquid(id)) continue;
        int ok = 1;
        for (int ay = y + 1; ay <= y + 3 && ay <= FLAT_WORLD_Y_MAX; ay++) {
            int above = flat_get_block(x, ay, z);
            if (above != 0 && above != BLOCK_SNOW_LAYER && !block_is_liquid(above)) { ok = 0; break; }
        }
        if (!ok) continue;
        *out_foot_y = y + 1;
        return 1;
    }
    return 0;
}

static void psp_ensure_spawn_surface(float *px, float *py, float *pz, int force_emergency) {
    int cx = (int)floorf(*px);
    int cz = (int)floorf(*pz);
    int foot_y = (int)floorf(*py - 1.62f + 0.001f);

    if (force_emergency || foot_y < FLAT_WORLD_Y_MIN + 2 || foot_y > FLAT_WORLD_Y_MAX - 3) {
        foot_y = 64;
        if (foot_y > FLAT_WORLD_Y_MAX - 3) foot_y = FLAT_WORLD_Y_MAX - 3;
        if (foot_y < FLAT_WORLD_Y_MIN + 2) foot_y = FLAT_WORLD_Y_MIN + 2;
    } else {
        int better_foot_y = 0;
        if (psp_highest_surface_foot_y(cx, cz, &better_foot_y)) foot_y = better_foot_y;
    }

    psp_make_spawn_surface(cx, foot_y - 1, cz, 3);
    *px = (float)cx + 0.5f;
    *py = (float)foot_y + 1.62f;
    *pz = (float)cz + 0.5f;
}
#endif

static int block_drop_id(int block_id) {
    /* ph.java grass drops dirt via of.v.a(0, random). */
    if (block_id == BLOCK_GRASS) return BLOCK_DIRT;
    if (block_id == BLOCK_SNOW_LAYER) return ITEM_SNOWBALL;
    if (block_id == BLOCK_LEAVES) return 0;
    if (block_is_liquid(block_id)) return 0;
    if (block_id == BLOCK_STONE) return BLOCK_COBBLESTONE;
    if (block_id == BLOCK_COAL_ORE) return ITEM_COAL;
    if (block_id == BLOCK_DIAMOND_ORE) return ITEM_DIAMOND;
    if (block_id == BLOCK_REDSTONE_ORE || block_id == BLOCK_REDSTONE_ORE_GLOWING) return ITEM_REDSTONE;
    if (block_id == BLOCK_CLAY) return ITEM_CLAY;
    if (block_id == BLOCK_GLOWSTONE) return ITEM_GLOWSTONE_DUST;
    if (block_id == BLOCK_REEDS) return ITEM_REED;
    if (block_id == BLOCK_SIGN_POST || block_id == BLOCK_WALL_SIGN) return ITEM_SIGN;
    if (block_id == BLOCK_FURNACE_LIT) return BLOCK_FURNACE;
    if (block_id == BLOCK_BEDROCK) return 0;
    return block_id;
}

static float block_hardness(int block_id) {
    /* Values mirror the deobfuscated Beta client Block.java setHardness(...). */
    switch (block_id) {
        case BLOCK_STONE: return 1.5f;
        case BLOCK_GRASS: return 0.6f;
        case BLOCK_DIRT: return 0.5f;
        case BLOCK_COBBLESTONE: return 2.0f;
        case BLOCK_PLANKS: return 2.0f;
        case BLOCK_SAPLING: return 0.0f;
        case BLOCK_BEDROCK: return -1.0f;
        case BLOCK_WATER: case BLOCK_STILL_WATER: return 100.0f;
        case BLOCK_LAVA: return 0.0f;
        case BLOCK_STILL_LAVA: return 100.0f;
        case BLOCK_SAND: return 0.5f;
        case BLOCK_GRAVEL: return 0.6f;
        case BLOCK_GOLD_ORE: case BLOCK_IRON_ORE: case BLOCK_COAL_ORE: return 3.0f;
        case BLOCK_LOG: return 2.0f;
        case BLOCK_LEAVES: return 0.2f;
        case BLOCK_SPONGE: return 0.6f;
        case BLOCK_GLASS: return 0.3f;
        case BLOCK_LAPIS_ORE: return 3.0f;
        case BLOCK_LAPIS_BLOCK: return 3.0f;
        case BLOCK_WOOL: return 0.8f;
        case BLOCK_YELLOW_FLOWER: case BLOCK_RED_ROSE:
        case BLOCK_BROWN_MUSHROOM: case BLOCK_RED_MUSHROOM: return 0.0f;
        case BLOCK_GOLD_BLOCK: return 3.0f;
        case BLOCK_IRON_BLOCK: return 5.0f;
        case BLOCK_DOUBLE_SLAB: case BLOCK_SLAB: return 2.0f;
        case BLOCK_BRICK: return 2.0f;
        case BLOCK_TNT: return 0.0f;
        case BLOCK_BOOKSHELF: return 1.5f;
        case BLOCK_MOSSY_COBBLESTONE: return 2.0f;
        case BLOCK_OBSIDIAN: return 10.0f;
        case BLOCK_TORCH: case BLOCK_FIRE: return 0.0f;
        case BLOCK_MOB_SPAWNER: return 5.0f;
        case BLOCK_WOOD_STAIRS: return 2.0f;       /* BlockStairs copies planks hardness. */
        case BLOCK_CHEST: return 2.5f;
        case BLOCK_REDSTONE_WIRE: return 0.0f;
        case BLOCK_DIAMOND_ORE: return 3.0f;
        case BLOCK_DIAMOND_BLOCK: return 5.0f;
        case BLOCK_CRAFTING_TABLE: return 2.5f;
        case BLOCK_CROPS: return 0.0f;
        case BLOCK_FARMLAND: return 0.6f;
        case BLOCK_FURNACE: case BLOCK_FURNACE_LIT: return 3.5f;
        case BLOCK_SIGN_POST: case BLOCK_WALL_SIGN: return 1.0f;
        case BLOCK_WOOD_DOOR: return 3.0f;
        case BLOCK_LADDER: return 0.4f;
        case BLOCK_RAILS: return 0.7f;
        case BLOCK_COBBLE_STAIRS: return 2.0f;     /* BlockStairs copies cobblestone hardness. */
        case BLOCK_LEVER: return 0.5f;
        case BLOCK_STONE_PRESSURE_PLATE: return 0.5f;
        case BLOCK_IRON_DOOR: return 5.0f;
        case BLOCK_WOOD_PRESSURE_PLATE: return 0.5f;
        case BLOCK_REDSTONE_ORE: case BLOCK_REDSTONE_ORE_GLOWING: return 3.0f;
        case BLOCK_REDSTONE_TORCH_OFF: case BLOCK_REDSTONE_TORCH_ON: return 0.0f;
        case BLOCK_STONE_BUTTON: return 0.5f;
        case BLOCK_SNOW_LAYER: return 0.1f;
        case BLOCK_ICE: return 0.5f;
        case BLOCK_SNOW_BLOCK: return 0.2f;
        case BLOCK_CACTUS: return 0.4f;
        case BLOCK_CLAY: return 0.6f;
        case BLOCK_REEDS: return 0.0f;
        case BLOCK_JUKEBOX: return 2.0f;
        case BLOCK_FENCE: return 2.0f;
        case BLOCK_PUMPKIN: return 1.0f;
        case BLOCK_NETHERRACK: return 0.4f;
        case BLOCK_SOUL_SAND: return 0.5f;
        case BLOCK_GLOWSTONE: return 0.3f;
        case BLOCK_PORTAL: return -1.0f;
        case BLOCK_JACK_O_LANTERN: return 1.0f;
        default: return 1.0f;
    }
}

static int held_tool_quality(int held_id) {
    /* Java Item.java constructor quality values: wood=0, stone=1, iron=2,
       diamond=3, and gold also uses quality 0 in this Beta source. */
    switch (held_id) {
        case ITEM_WOODEN_PICKAXE: case ITEM_WOODEN_SHOVEL: case ITEM_WOODEN_AXE:
        case ITEM_PICKAXE_GOLD: case ITEM_SHOVEL_GOLD: case ITEM_AXE_GOLD:
            return 0;
        case ITEM_STONE_PICKAXE: case ITEM_STONE_SHOVEL: case ITEM_STONE_AXE:
            return 1;
        case ITEM_PICKAXE_IRON: case ITEM_SHOVEL_IRON: case ITEM_AXE_IRON:
            return 2;
        case ITEM_PICKAXE_DIAMOND: case ITEM_SHOVEL_DIAMOND: case ITEM_AXE_DIAMOND:
            return 3;
        default:
            return -1;
    }
}

static int held_is_pickaxe(int held_id) {
    return held_id == ITEM_WOODEN_PICKAXE || held_id == ITEM_STONE_PICKAXE ||
           held_id == ITEM_PICKAXE_IRON || held_id == ITEM_PICKAXE_DIAMOND ||
           held_id == ITEM_PICKAXE_GOLD;
}

static int held_is_shovel(int held_id) {
    return held_id == ITEM_WOODEN_SHOVEL || held_id == ITEM_STONE_SHOVEL ||
           held_id == ITEM_SHOVEL_IRON || held_id == ITEM_SHOVEL_DIAMOND ||
           held_id == ITEM_SHOVEL_GOLD;
}

static int held_is_axe(int held_id) {
    return held_id == ITEM_WOODEN_AXE || held_id == ITEM_STONE_AXE ||
           held_id == ITEM_AXE_IRON || held_id == ITEM_AXE_DIAMOND ||
           held_id == ITEM_AXE_GOLD;
}

static int held_is_sword(int held_id) {
    return held_id == ITEM_WOODEN_SWORD || held_id == ITEM_STONE_SWORD ||
           held_id == ITEM_SWORD_IRON || held_id == ITEM_SWORD_DIAMOND ||
           held_id == ITEM_SWORD_GOLD;
}

static int block_is_pickaxe_fast_block(int block_id) {
    /* Pickaxes are fast on rock/metal/ore blocks and the small rock-derived
       utility blocks.  This keeps wooden stairs/plates/buttons on the axe path
       and stone buttons/plates/cobble stairs on the pickaxe path instead of
       letting swords beat the correct tool. */
    switch (block_id) {
        case BLOCK_STONE:
        case BLOCK_COBBLESTONE:
        case BLOCK_DOUBLE_SLAB:
        case BLOCK_SLAB:
        case BLOCK_BRICK:
        case BLOCK_MOSSY_COBBLESTONE:
        case BLOCK_OBSIDIAN:
        case BLOCK_MOB_SPAWNER:
        case BLOCK_FURNACE:
        case BLOCK_FURNACE_LIT:
        case BLOCK_COBBLE_STAIRS:
        case BLOCK_STONE_PRESSURE_PLATE:
        case BLOCK_STONE_BUTTON:
        case BLOCK_IRON_DOOR:
        case BLOCK_IRON_ORE:
        case BLOCK_IRON_BLOCK:
        case BLOCK_COAL_ORE:
        case BLOCK_GOLD_BLOCK:
        case BLOCK_GOLD_ORE:
        case BLOCK_DIAMOND_ORE:
        case BLOCK_DIAMOND_BLOCK:
        case BLOCK_REDSTONE_ORE:
        case BLOCK_REDSTONE_ORE_GLOWING:
        case BLOCK_ICE:
        case BLOCK_NETHERRACK:
        case BLOCK_LAPIS_ORE:
        case BLOCK_LAPIS_BLOCK:
            return 1;
        default:
            return 0;
    }
}

static int block_is_shovel_fast_block(int block_id) {
    return block_id == BLOCK_GRASS || block_id == BLOCK_DIRT || block_id == BLOCK_SAND ||
           block_id == BLOCK_GRAVEL || block_id == BLOCK_SNOW_LAYER ||
           block_id == BLOCK_SNOW_BLOCK || block_id == BLOCK_CLAY;
}

static int block_is_axe_fast_block(int block_id) {
    /* The old table only included the four Java Beta ItemAxe entries, which
       made later wooden block-forms like wood stairs/plates/fences slower than
       a sword.  Treat every wooden block currently implemented by this client
       as axe-effective. */
    switch (block_id) {
        case BLOCK_PLANKS:
        case BLOCK_BOOKSHELF:
        case BLOCK_LOG:
        case BLOCK_CHEST:
        case BLOCK_CRAFTING_TABLE:
        case BLOCK_WOOD_STAIRS:
        case BLOCK_WOOD_DOOR:
        case BLOCK_LADDER:
        case BLOCK_SIGN_POST:
        case BLOCK_WALL_SIGN:
        case BLOCK_WOOD_PRESSURE_PLATE:
        case BLOCK_FENCE:
        case BLOCK_JUKEBOX:
            return 1;
        default:
            return 0;
    }
}

static int block_requires_tool_to_harvest(int block_id) {
    /* InventoryPlayer.canHarvestBlock only requires a tool for rock, iron,
       builtSnow, and snow materials.  Everything else can drop by hand. */
    switch (block_id) {
        case BLOCK_STONE:
        case BLOCK_COBBLESTONE:
        case BLOCK_GOLD_ORE:
        case BLOCK_IRON_ORE:
        case BLOCK_COAL_ORE:
        case BLOCK_GOLD_BLOCK:
        case BLOCK_IRON_BLOCK:
        case BLOCK_DOUBLE_SLAB:
        case BLOCK_SLAB:
        case BLOCK_BRICK:
        case BLOCK_MOSSY_COBBLESTONE:
        case BLOCK_OBSIDIAN:
        case BLOCK_MOB_SPAWNER:
        case BLOCK_DIAMOND_ORE:
        case BLOCK_DIAMOND_BLOCK:
        case BLOCK_FURNACE:
        case BLOCK_FURNACE_LIT:
        case BLOCK_COBBLE_STAIRS:
        case BLOCK_IRON_DOOR:
        case BLOCK_REDSTONE_ORE:
        case BLOCK_REDSTONE_ORE_GLOWING:
        case BLOCK_SNOW_LAYER:
        case BLOCK_SNOW_BLOCK:
        case BLOCK_NETHERRACK:
        case BLOCK_LAPIS_ORE:
        case BLOCK_LAPIS_BLOCK:
            return 1;
        default:
            return 0;
    }
}

static int held_pickaxe_can_harvest(int held_id, int block_id) {
    int q = held_tool_quality(held_id);
    if (!held_is_pickaxe(held_id)) return 0;
    if (block_id == BLOCK_OBSIDIAN) return q == 3;
    if (block_id == BLOCK_DIAMOND_BLOCK || block_id == BLOCK_DIAMOND_ORE) return q >= 2;
    if (block_id == BLOCK_GOLD_BLOCK || block_id == BLOCK_GOLD_ORE) return q >= 2;
    if (block_id == BLOCK_IRON_BLOCK || block_id == BLOCK_IRON_ORE) return q >= 1;
    if (block_id == BLOCK_REDSTONE_ORE || block_id == BLOCK_REDSTONE_ORE_GLOWING) return q >= 2;
    if (block_id == BLOCK_LAPIS_ORE || block_id == BLOCK_LAPIS_BLOCK) return q >= 1;
    return block_requires_tool_to_harvest(block_id) &&
           block_id != BLOCK_SNOW_LAYER && block_id != BLOCK_SNOW_BLOCK;
}

static int held_shovel_can_harvest(int held_id, int block_id) {
    if (!held_is_shovel(held_id)) return 0;
    return block_id == BLOCK_SNOW_LAYER || block_id == BLOCK_SNOW_BLOCK;
}

static int held_item_can_harvest_block_id(int held_id, int block_id) {
    if (!block_requires_tool_to_harvest(block_id)) return 1;
    if (held_pickaxe_can_harvest(held_id, block_id)) return 1;
    if (held_shovel_can_harvest(held_id, block_id)) return 1;
    return 0;
}

static float held_item_strength_vs_block(int held_id, int block_id) {
    int q = held_tool_quality(held_id);
    if (held_is_pickaxe(held_id) && block_is_pickaxe_fast_block(block_id)) return (float)((q + 1) * 2);
    if (held_is_shovel(held_id) && block_is_shovel_fast_block(block_id)) return (float)((q + 1) * 2);
    if (held_is_axe(held_id) && block_is_axe_fast_block(block_id)) return (float)((q + 1) * 2);
    if (held_is_sword(held_id)) return 1.5f;
    return 1.0f;
}

static float block_relative_strength(int block_id) {
    if (player_is_creative()) return 1.0f;
    float h = block_hardness(block_id);
    if (h < 0.0f) return 0.0f;
    if (h <= 0.0f) return 1.0f;

    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    int held_id = stack_empty(held) ? 0 : held->id;

    if (!held_item_can_harvest_block_id(held_id, block_id)) {
        return 1.0f / h / 100.0f;
    }
    return held_item_strength_vs_block(held_id, block_id) / h / 30.0f;
}

static void restart_hand_swing(void) {
    /* A new click/use action should visibly restart the first-person arm
       animation even if the previous swing has not finished yet. */
    g_air_swing_playing = 0;
    g_air_swing_ticks = 0;
    g_hand_swing_active = 1;
    g_hand_swing_ticks = 0;
    g_hand_swing_progress = 0.0f;
    g_prev_hand_swing = 0.0f;
    g_hand_swing = 0.0f;
}

static void trigger_hand_swing(void) {
    /* Holding mine must not reset every tick, otherwise the hand freezes at
       the start of the swing.  For discrete re-click/place actions use
       restart_hand_swing(). */
    if (!g_hand_swing_active && g_hand_swing_ticks == 0) {
        restart_hand_swing();
    }
}

static int flat_player_aabb_collides(float px, float py, float pz) {
    float feet = py - 1.62f;
    float minx = px - 0.30f, maxx = px + 0.30f;
    float miny = feet + 0.001f, maxy = feet + 1.80f - 0.001f;
    float minz = pz - 0.30f, maxz = pz + 0.30f;
    int x0 = (int)floorf(minx), x1 = (int)floorf(maxx - 0.001f);
    int y0 = (int)floorf(miny), y1 = (int)floorf(maxy - 0.001f);
    int z0 = (int)floorf(minz), z1 = (int)floorf(maxz - 0.001f);
    for (int y = y0 - 1; y <= y1; y++) {
        for (int z = z0; z <= z1; z++) {
            for (int x = x0; x <= x1; x++) {
                /* Do not let the player step/fall into an async chunk that has
                   not been installed yet.  Treat only the column under/at the
                   feet as a temporary collision plane; head/body space stays
                   non-solid so this cannot trap the player inside invisible
                   chunk walls. */
                if (!flat_chunk_generated_at_block(x, z) && y <= (int)floorf(feet)) {
                    return 1;
                }
#if defined(PEX_PLATFORM_PSP)
                if (psp_spawn_surface_intersects(minx, maxx, miny, maxy, minz, maxz, x, y, z)) return 1;
#endif
                int bid = flat_get_block(x, y, z);
                if (block_is_door_id(bid)) {
                    if (door_thin_aabb_intersects(minx, maxx, miny, maxy, minz, maxz, x, y, z)) return 1;
                } else if (block_has_custom_collision(bid)) {
                    if (block_custom_collision_intersects(bid, minx, maxx, miny, maxy, minz, maxz, x, y, z)) return 1;
                } else if (flat_block_is_solid(bid)) {
                    if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz,
                                            (float)x, (float)y, (float)z,
                                            (float)(x + 1), (float)(y + 1), (float)(z + 1))) return 1;
                }
            }
        }
    }
    return 0;
}


static int flat_player_has_water_exit_ledge(float px, float py, float pz, float dx, float dz) {
    /* Only allow the strong water-exit jump when the player is actually
       pushing into a nearby bank/ledge.  Open water should keep normal slow
       swimming, not launch the player upward forever. */
    float len = sqrtf(dx * dx + dz * dz);
    if (len < 0.001f) return 0;
    dx /= len;
    dz /= len;

    /* Too deep underwater: swim upward normally first, then mantle only when
       the head is near the surface. */
    int bx = (int)floorf(px);
    int by = (int)floorf(py);
    int bz = (int)floorf(pz);
    if (block_is_water(flat_get_block(bx, by + 1, bz))) return 0;

    /* Check a few distances in the intended movement direction.  A real exit
       ledge blocks the current body, but has clearance slightly above it. */
    for (int i = 0; i < 4; i++) {
        float dist = 0.35f + (float)i * 0.15f;
        float tx = px + dx * dist;
        float tz = pz + dz * dist;
        if (!flat_player_aabb_collides(tx, py, tz)) continue;
        if (!flat_player_aabb_collides(tx, py + 0.62f, tz)) return 1;
    }

    return 0;
}

static int flat_player_has_sneak_support(float px, float py, float pz) {
    /* Source-style sneaking edge check from B1.0.0 mm.java:249-273.
       The source reduces X/Z motion in 0.05 steps while the player's bounding
       box offset downward by 1.0 has no collision. This checks for ANY floor
       under the current player footprint, so the invisible wall is at the real
       block edge, not too early. */
    float feet = py - 1.62f;
    int floor_y = (int)floorf(feet - 1.0f + 0.001f);

    float minx = px - 0.30f, maxx = px + 0.30f;
    float minz = pz - 0.30f, maxz = pz + 0.30f;
    int x0 = (int)floorf(minx + 0.001f);
    int x1 = (int)floorf(maxx - 0.001f);
    int z0 = (int)floorf(minz + 0.001f);
    int z1 = (int)floorf(maxz - 0.001f);

    for (int z = z0; z <= z1; z++) {
        for (int x = x0; x <= x1; x++) {
            if (flat_get_block(x, floor_y, z) != 0) return 1;
        }
    }
    return 0;
}

static int flat_block_intersects_player(int bx, int by, int bz) {
    float feet = g_player_y - 1.62f;
    float minx = g_player_x - 0.30f, maxx = g_player_x + 0.30f;
    float miny = feet + 0.001f, maxy = feet + 1.80f - 0.001f;
    float minz = g_player_z - 0.30f, maxz = g_player_z + 0.30f;
    return maxx > (float)bx && minx < (float)(bx + 1) &&
           maxy > (float)by && miny < (float)(by + 1) &&
           maxz > (float)bz && minz < (float)(bz + 1);
}

static int flat_item_aabb_collides(float px, float py, float pz) {
    float minx = px - 0.125f, maxx = px + 0.125f;
    float miny = py - 0.125f, maxy = py + 0.125f;
    float minz = pz - 0.125f, maxz = pz + 0.125f;
    int x0 = (int)floorf(minx), x1 = (int)floorf(maxx - 0.001f);
    int y0 = (int)floorf(miny), y1 = (int)floorf(maxy - 0.001f);
    int z0 = (int)floorf(minz), z1 = (int)floorf(maxz - 0.001f);
    for (int y = y0 - 1; y <= y1; y++) {
        for (int z = z0; z <= z1; z++) {
            for (int x = x0; x <= x1; x++) {
                int bid = flat_get_block(x, y, z);
                if (block_is_door_id(bid)) {
                    if (door_thin_aabb_intersects(minx, maxx, miny, maxy, minz, maxz, x, y, z)) return 1;
                } else if (block_has_custom_collision(bid)) {
                    if (block_custom_collision_intersects(bid, minx, maxx, miny, maxy, minz, maxz, x, y, z)) return 1;
                } else if (flat_block_is_solid(bid)) {
                    if (aabb_intersects_box(minx, maxx, miny, maxy, minz, maxz,
                                            (float)x, (float)y, (float)z,
                                            (float)(x + 1), (float)(y + 1), (float)(z + 1))) return 1;
                }
            }
        }
    }
    return 0;
}


typedef struct FlatAABB { float minx, miny, minz, maxx, maxy, maxz; } FlatAABB;

static int flat_aabb_intersects_aabb(const FlatAABB *a, const FlatAABB *b) {
    return a->maxx > b->minx && a->minx < b->maxx &&
           a->maxy > b->miny && a->miny < b->maxy &&
           a->maxz > b->minz && a->minz < b->maxz;
}

static void flat_add_collision_box(FlatAABB *boxes, int *count, int max_boxes, const FlatAABB *query,
                                   float minx, float miny, float minz, float maxx, float maxy, float maxz) {
    if (*count >= max_boxes) return;
    FlatAABB b = { minx, miny, minz, maxx, maxy, maxz };
    if (!flat_aabb_intersects_aabb(query, &b)) return;
    boxes[(*count)++] = b;
}

static void flat_block_add_collision_boxes(int id, int x, int y, int z, const FlatAABB *query, FlatAABB *boxes, int *count, int max_boxes) {
    if (id <= 0 || block_is_liquid(id)) return;
    if (block_is_door_id(id)) {
        int m = flat_get_meta(x, y, z);
        int lower_y = door_meta_is_upper(m) ? y - 1 : y;
        int lm = flat_get_meta(x, lower_y, z);
        int dir = door_collision_state_from_meta(lm);
        const float t = 3.0f / 16.0f;
        if (dir == 0) flat_add_collision_box(boxes, count, max_boxes, query, x, y, z, x + 1.0f, y + 1.0f, z + t);
        else if (dir == 1) flat_add_collision_box(boxes, count, max_boxes, query, x + 1.0f - t, y, z, x + 1.0f, y + 1.0f, z + 1.0f);
        else if (dir == 2) flat_add_collision_box(boxes, count, max_boxes, query, x, y, z + 1.0f - t, x + 1.0f, y + 1.0f, z + 1.0f);
        else flat_add_collision_box(boxes, count, max_boxes, query, x, y, z, x + t, y + 1.0f, z + 1.0f);
        return;
    }
    if (id == BLOCK_LADDER) return;
    if (id == BLOCK_SLAB) { flat_add_collision_box(boxes, count, max_boxes, query, x, y, z, x + 1.0f, y + 0.5f, z + 1.0f); return; }
    if (id == BLOCK_CHEST) { flat_add_collision_box(boxes, count, max_boxes, query, x + 1.0f/16.0f, y, z + 1.0f/16.0f, x + 15.0f/16.0f, y + 14.0f/16.0f, z + 15.0f/16.0f); return; }
    if (id == BLOCK_CACTUS) { flat_add_collision_box(boxes, count, max_boxes, query, x + 1.0f/16.0f, y, z + 1.0f/16.0f, x + 15.0f/16.0f, y + 1.0f, z + 15.0f/16.0f); return; }
    if (id == BLOCK_WOOD_STAIRS || id == BLOCK_COBBLE_STAIRS) {
        flat_add_collision_box(boxes, count, max_boxes, query, x, y, z, x + 1.0f, y + 0.5f, z + 1.0f);
        int dir = flat_get_meta(x, y, z) & 3;
        if (dir == 0) flat_add_collision_box(boxes, count, max_boxes, query, x + 0.5f, y + 0.5f, z, x + 1.0f, y + 1.0f, z + 1.0f);
        else if (dir == 1) flat_add_collision_box(boxes, count, max_boxes, query, x, y + 0.5f, z, x + 0.5f, y + 1.0f, z + 1.0f);
        else if (dir == 2) flat_add_collision_box(boxes, count, max_boxes, query, x, y + 0.5f, z + 0.5f, x + 1.0f, y + 1.0f, z + 1.0f);
        else flat_add_collision_box(boxes, count, max_boxes, query, x, y + 0.5f, z, x + 1.0f, y + 1.0f, z + 0.5f);
        return;
    }
    if (id == BLOCK_FENCE) {
        flat_add_collision_box(boxes, count, max_boxes, query, x + 0.375f, y, z + 0.375f, x + 0.625f, y + 1.5f, z + 0.625f);
        if (flat_block_is_solid(flat_get_block(x - 1, y, z)) || flat_get_block(x - 1, y, z) == BLOCK_FENCE)
            flat_add_collision_box(boxes, count, max_boxes, query, x, y, z + 0.375f, x + 0.5f, y + 1.5f, z + 0.625f);
        if (flat_block_is_solid(flat_get_block(x + 1, y, z)) || flat_get_block(x + 1, y, z) == BLOCK_FENCE)
            flat_add_collision_box(boxes, count, max_boxes, query, x + 0.5f, y, z + 0.375f, x + 1.0f, y + 1.5f, z + 0.625f);
        if (flat_block_is_solid(flat_get_block(x, y, z - 1)) || flat_get_block(x, y, z - 1) == BLOCK_FENCE)
            flat_add_collision_box(boxes, count, max_boxes, query, x + 0.375f, y, z, x + 0.625f, y + 1.5f, z + 0.5f);
        if (flat_block_is_solid(flat_get_block(x, y, z + 1)) || flat_get_block(x, y, z + 1) == BLOCK_FENCE)
            flat_add_collision_box(boxes, count, max_boxes, query, x + 0.375f, y, z + 0.5f, x + 0.625f, y + 1.5f, z + 1.0f);
        return;
    }
    if (flat_block_is_solid(id)) flat_add_collision_box(boxes, count, max_boxes, query, x, y, z, x + 1.0f, y + 1.0f, z + 1.0f);
}

static int flat_get_collision_boxes(const FlatAABB *query, FlatAABB *boxes, int max_boxes) {
    int count = 0;
    int x0 = (int)floorf(query->minx);
    int x1 = (int)floorf(query->maxx + 1.0f);
    int y0 = (int)floorf(query->miny);
    int y1 = (int)floorf(query->maxy + 1.0f);
    int z0 = (int)floorf(query->minz);
    int z1 = (int)floorf(query->maxz + 1.0f);
    for (int x = x0; x < x1; ++x) {
        for (int z = z0; z < z1; ++z) {
            for (int y = y0 - 1; y < y1; ++y) {
                flat_block_add_collision_boxes(flat_get_block(x, y, z), x, y, z, query, boxes, &count, max_boxes);
            }
        }
    }
    return count;
}

static float aabb_clip_x(const FlatAABB *b, const FlatAABB *a, float dx) {
    if (a->maxy <= b->miny || a->miny >= b->maxy || a->maxz <= b->minz || a->minz >= b->maxz) return dx;
    if (dx > 0.0f && a->maxx <= b->minx) { float v = b->minx - a->maxx; if (v < dx) dx = v; }
    if (dx < 0.0f && a->minx >= b->maxx) { float v = b->maxx - a->minx; if (v > dx) dx = v; }
    return dx;
}
static float aabb_clip_y(const FlatAABB *b, const FlatAABB *a, float dy) {
    if (a->maxx <= b->minx || a->minx >= b->maxx || a->maxz <= b->minz || a->minz >= b->maxz) return dy;
    if (dy > 0.0f && a->maxy <= b->miny) { float v = b->miny - a->maxy; if (v < dy) dy = v; }
    if (dy < 0.0f && a->miny >= b->maxy) { float v = b->maxy - a->miny; if (v > dy) dy = v; }
    return dy;
}
static float aabb_clip_z(const FlatAABB *b, const FlatAABB *a, float dz) {
    if (a->maxx <= b->minx || a->minx >= b->maxx || a->maxy <= b->miny || a->miny >= b->maxy) return dz;
    if (dz > 0.0f && a->maxz <= b->minz) { float v = b->minz - a->maxz; if (v < dz) dz = v; }
    if (dz < 0.0f && a->minz >= b->maxz) { float v = b->maxz - a->minz; if (v > dz) dz = v; }
    return dz;
}

static void aabb_offset(FlatAABB *a, float dx, float dy, float dz) {
    a->minx += dx; a->maxx += dx;
    a->miny += dy; a->maxy += dy;
    a->minz += dz; a->maxz += dz;
}

static void dropped_item_move_entity(FlatDroppedItem *e, float dx, float dy, float dz) {
    FlatAABB box = { e->x - 0.125f, e->y - 0.125f, e->z - 0.125f, e->x + 0.125f, e->y + 0.125f, e->z + 0.125f };
    FlatAABB sweep = box;
    if (dx < 0) sweep.minx += dx; else sweep.maxx += dx;
    if (dy < 0) sweep.miny += dy; else sweep.maxy += dy;
    if (dz < 0) sweep.minz += dz; else sweep.maxz += dz;
    FlatAABB boxes[128];
    int n = flat_get_collision_boxes(&sweep, boxes, 128);
    float odx = dx, ody = dy, odz = dz;
    for (int i = 0; i < n; ++i) dy = aabb_clip_y(&boxes[i], &box, dy);
    aabb_offset(&box, 0.0f, dy, 0.0f);
    for (int i = 0; i < n; ++i) dx = aabb_clip_x(&boxes[i], &box, dx);
    aabb_offset(&box, dx, 0.0f, 0.0f);
    for (int i = 0; i < n; ++i) dz = aabb_clip_z(&boxes[i], &box, dz);
    aabb_offset(&box, 0.0f, 0.0f, dz);
    e->x = (box.minx + box.maxx) * 0.5f;
    e->y = (box.miny + box.maxy) * 0.5f;
    e->z = (box.minz + box.maxz) * 0.5f;
    e->on_ground = (ody != dy && ody < 0.0f) ? 1 : 0;
    if (odx != dx) e->mx = 0.0f;
    if (ody != dy) e->my = 0.0f;
    if (odz != dz) e->mz = 0.0f;
}

static void inventory_reset(void) {
    memset(g_inventory, 0, sizeof(g_inventory));
    memset(g_armor_inventory, 0, sizeof(g_armor_inventory));
    armor_sync_player_armor();
    g_player_damage_remainder = 0;
    memset(g_craft_grid, 0, sizeof(g_craft_grid));
    memset(g_workbench_grid, 0, sizeof(g_workbench_grid));
    memset(g_chest_slots, 0, sizeof(g_chest_slots));
    g_legacy_global_chest_pending = 0;
    chest_clear_all_tiles();
    furnace_clear_all_tiles();
    stack_clear(&g_carried_stack);

    if (player_is_creative()) {
        g_inventory[0] = make_stack(BLOCK_STONE, 64, 0);
        g_inventory[1] = make_stack(BLOCK_GRASS, 64, 0);
        g_inventory[2] = make_stack(BLOCK_PLANKS, 64, 0);
        g_inventory[3] = make_stack(BLOCK_GLASS, 64, 0);
        g_inventory[4] = make_stack(BLOCK_TORCH, 64, 0);
        g_inventory[5] = make_stack(BLOCK_CHEST, 64, 0);
        g_inventory[6] = make_stack(ITEM_PICKAXE_DIAMOND, 1, 0);
        g_inventory[7] = make_stack(ITEM_BUCKET_WATER, 1, 0);
        g_inventory[8] = make_stack(ITEM_BUCKET_LAVA, 1, 0);
        g_equipped_item = g_inventory[g_selected_hotbar_slot];
        g_equipped_slot = g_selected_hotbar_slot;
        g_equipped_progress = g_prev_equipped_progress = 1.0f;
        return;
    }

    /* Start hotbar stone tool set. Only the stone shovel gets the dirt/grass
       mining boost; sword/pickaxe/axe are added as usable visible items only. */
    g_inventory[0] = make_stack(ITEM_STONE_SWORD, 1, 0);
    g_inventory[1] = make_stack(ITEM_STONE_PICKAXE, 1, 0);
    g_inventory[2] = make_stack(ITEM_STONE_AXE, 1, 0);
    g_inventory[3] = make_stack(ITEM_STONE_SHOVEL, 1, 0);
    g_inventory[4] = make_stack(BLOCK_GRASS, 64, 0);
    g_inventory[5] = make_stack(BLOCK_LOG, 64, 0);
    g_inventory[6] = make_stack(BLOCK_TORCH, 64, 0);

    /* Keep the earlier requested reserve of 10 stone shovels in the main inventory. */
    for (int i = 0; i < 10; i++) {
        g_inventory[9 + i] = make_stack(ITEM_STONE_SHOVEL, 1, 0);
    }

    /* One starter stack of stone in main inventory, not hotbar. Stone drops cobblestone. */
    g_inventory[19] = make_stack(BLOCK_STONE, 64, 0);
    g_inventory[20] = make_stack(BLOCK_FURNACE, 1, 0); /* test furnace for prepared furnace UI */
    g_inventory[21] = make_stack(BLOCK_SAND, 64, 0);
    g_inventory[22] = make_stack(BLOCK_GRAVEL, 64, 0);
    g_inventory[23] = make_stack(BLOCK_CHEST, 1, 0);
    g_inventory[24] = make_stack(ITEM_BUCKET_EMPTY, 1, 0); /* passive mob test: cow milking */
    g_inventory[25] = make_stack(ITEM_SADDLE, 1, 0);       /* passive mob test: pig saddle */
    g_equipped_item = g_inventory[g_selected_hotbar_slot];
    g_equipped_slot = g_selected_hotbar_slot;
    g_equipped_progress = g_prev_equipped_progress = 1.0f;
}

static int inventory_add_stack(ItemStack st) {
    if (stack_empty(&st)) return 1;
    for (int i = 0; i < 36 && st.count > 0; i++) {
        if (stack_same_item(&g_inventory[i], &st) && g_inventory[i].count < stack_limit_for_id(st.id)) {
            int room = stack_limit_for_id(st.id) - g_inventory[i].count;
            int move = st.count < room ? st.count : room;
            g_inventory[i].count += move;
            g_inventory[i].pop_time = 5;
            st.count -= move;
        }
    }
    for (int i = 0; i < 36 && st.count > 0; i++) {
        if (stack_empty(&g_inventory[i])) {
            int move = st.count < stack_limit_for_id(st.id) ? st.count : stack_limit_for_id(st.id);
            g_inventory[i] = make_stack(st.id, move, st.damage);
            g_inventory[i].pop_time = 5;
            st.count -= move;
        }
    }
    return st.count <= 0;
}

static void inventory_tick(void) {
    for (int i = 0; i < 36; i++) if (g_inventory[i].pop_time > 0) g_inventory[i].pop_time--;
}

static void player_look_vector(float *dx, float *dy, float *dz) {
    float yaw = g_player_yaw * (float)M_PI / 180.0f;
    float pitch = g_player_pitch * (float)M_PI / 180.0f;
    float cp = cosf(pitch);
    *dx = -sinf(yaw) * cp;
    *dy = -sinf(pitch);
    *dz =  cosf(yaw) * cp;
}

static void pex_touch_aware_look_vector(float *dx, float *dy, float *dz) {
#ifdef PEX_PLATFORM_ANDROID
    if (g_android_touch_pick_active) {
        *dx = g_android_touch_ray_dx;
        *dy = g_android_touch_ray_dy;
        *dz = g_android_touch_ray_dz;
        return;
    }
#endif
    player_look_vector(dx, dy, dz);
}

static void spawn_item_stack(float x, float y, float z, ItemStack st, int random_spread) {
    if (stack_empty(&st)) return;
    int slot = -1;
    for (int i = 0; i < MAX_DROP_ENTITIES; i++) if (!g_drops[i].active) { slot = i; break; }
    if (slot < 0) slot = 0;
    FlatDroppedItem *e = &g_drops[slot];
    memset(e, 0, sizeof(*e));
    e->active = 1;
    e->stack = st;
    e->x = e->prev_x = x;
    e->y = e->prev_y = y;
    e->z = e->prev_z = z;
    e->rot = (float)(rand() % 6283) / 1000.0f;
    e->pickup_delay = 10;
    g_save_dirty = 1;
    if (random_spread) {
        e->mx = ((float)rand() / (float)RAND_MAX) * 0.2f - 0.1f;
        e->my = 0.2f;
        e->mz = ((float)rand() / (float)RAND_MAX) * 0.2f - 0.1f;
    } else {
        float lx, ly, lz;
        pex_touch_aware_look_vector(&lx, &ly, &lz);
        e->mx = lx * 0.3f;
        e->my = 0.1f + ly * 0.1f;
        e->mz = lz * 0.3f;
    }
}


static int chest_find_tile(int x, int y, int z) {
    for (int i = 0; i < MAX_CHEST_TILES; i++) {
        if (g_chest_tiles[i].active && g_chest_tiles[i].x == x && g_chest_tiles[i].y == y && g_chest_tiles[i].z == z) return i;
    }
    return -1;
}

static int chest_slots_have_items(const ItemStack *slots, int n) {
    for (int i = 0; i < n; i++) if (!stack_empty(&slots[i])) return 1;
    return 0;
}

static int chest_alloc_tile(int x, int y, int z) {
    int existing = chest_find_tile(x, y, z);
    if (existing >= 0) return existing;

    int free_i = -1;
    for (int i = 0; i < MAX_CHEST_TILES; i++) {
        if (!g_chest_tiles[i].active) { free_i = i; break; }
    }
    if (free_i < 0) return -1;

    ChestTile *ct = &g_chest_tiles[free_i];
    memset(ct, 0, sizeof(*ct));
    ct->active = 1;
    ct->x = x; ct->y = y; ct->z = z;

    /* One-time rescue path for worlds saved by the old broken global chest
       implementation.  It had no coordinates, so the first real chest opened
       after upgrading receives that old inventory instead of silently losing it. */
    if (g_legacy_global_chest_pending && !chest_slots_have_items(ct->slots, 27)) {
        memcpy(ct->slots, g_chest_slots, sizeof(ct->slots));
        memset(g_chest_slots, 0, sizeof(g_chest_slots));
        g_legacy_global_chest_pending = 0;
        g_save_dirty = 1;
    }
    return free_i;
}

static void chest_clear_all_tiles(void) {
    memset(g_chest_tiles, 0, sizeof(g_chest_tiles));
    g_open_chest_indices[0] = g_open_chest_indices[1] = -1;
    g_open_chest_count = 0;
    g_open_chest_rows = 3;
    snprintf(g_open_chest_title, sizeof(g_open_chest_title), "Chest");
}

static void chest_close_open_inventory(void) {
    g_open_chest_indices[0] = g_open_chest_indices[1] = -1;
    g_open_chest_count = 0;
    g_open_chest_rows = 3;
    snprintf(g_open_chest_title, sizeof(g_open_chest_title), "Chest");
}

static void chest_remove_tile_index(int idx) {
    if (idx < 0 || idx >= MAX_CHEST_TILES) return;
    memset(&g_chest_tiles[idx], 0, sizeof(g_chest_tiles[idx]));
    for (int i = 0; i < 2; i++) {
        if (g_open_chest_indices[i] == idx) chest_close_open_inventory();
    }
    g_save_dirty = 1;
}

static int flat_block_blocks_chest(int id) {
    return flat_block_blocks_snow(id);
}

static int chest_neighbor_count_at(int x, int y, int z) {
    int n = 0;
    if (flat_get_block(x - 1, y, z) == BLOCK_CHEST) n++;
    if (flat_get_block(x + 1, y, z) == BLOCK_CHEST) n++;
    if (flat_get_block(x, y, z - 1) == BLOCK_CHEST) n++;
    if (flat_get_block(x, y, z + 1) == BLOCK_CHEST) n++;
    return n;
}

static int chest_has_neighbor_chest(int x, int y, int z) {
    if (flat_get_block(x, y, z) != BLOCK_CHEST) return 0;
    return chest_neighbor_count_at(x, y, z) > 0;
}

static int chest_can_place_at(int x, int y, int z) {
    int neighbors = chest_neighbor_count_at(x, y, z);
    if (neighbors > 1) return 0;
    if (chest_has_neighbor_chest(x - 1, y, z)) return 0;
    if (chest_has_neighbor_chest(x + 1, y, z)) return 0;
    if (chest_has_neighbor_chest(x, y, z - 1)) return 0;
    if (chest_has_neighbor_chest(x, y, z + 1)) return 0;
    return 1;
}

static void chest_on_block_placed(int x, int y, int z) {
    if (flat_get_block(x, y, z) == BLOCK_CHEST) {
        chest_alloc_tile(x, y, z);
        g_save_dirty = 1;
    }
}

static int chest_blocked_above(int x, int y, int z) {
    if (y >= FLAT_WORLD_Y_MAX) return 0;
    return flat_block_blocks_chest(flat_get_block(x, y + 1, z));
}

static int chest_open_at(int x, int y, int z) {
    if (flat_get_block(x, y, z) != BLOCK_CHEST) return 0;
    if (chest_blocked_above(x, y, z)) return 1;
    if (flat_get_block(x - 1, y, z) == BLOCK_CHEST && chest_blocked_above(x - 1, y, z)) return 1;
    if (flat_get_block(x + 1, y, z) == BLOCK_CHEST && chest_blocked_above(x + 1, y, z)) return 1;
    if (flat_get_block(x, y, z - 1) == BLOCK_CHEST && chest_blocked_above(x, y, z - 1)) return 1;
    if (flat_get_block(x, y, z + 1) == BLOCK_CHEST && chest_blocked_above(x, y, z + 1)) return 1;

    int self = chest_alloc_tile(x, y, z);
    if (self < 0) return 1;
    g_open_chest_indices[0] = self;
    g_open_chest_indices[1] = -1;
    g_open_chest_count = 1;
    g_open_chest_rows = 3;
    snprintf(g_open_chest_title, sizeof(g_open_chest_title), "Chest");

    int other = -1;
    if (flat_get_block(x - 1, y, z) == BLOCK_CHEST) {
        other = chest_alloc_tile(x - 1, y, z);
        g_open_chest_indices[0] = other;
        g_open_chest_indices[1] = self;
    } else if (flat_get_block(x + 1, y, z) == BLOCK_CHEST) {
        other = chest_alloc_tile(x + 1, y, z);
        g_open_chest_indices[0] = self;
        g_open_chest_indices[1] = other;
    } else if (flat_get_block(x, y, z - 1) == BLOCK_CHEST) {
        other = chest_alloc_tile(x, y, z - 1);
        g_open_chest_indices[0] = other;
        g_open_chest_indices[1] = self;
    } else if (flat_get_block(x, y, z + 1) == BLOCK_CHEST) {
        other = chest_alloc_tile(x, y, z + 1);
        g_open_chest_indices[0] = self;
        g_open_chest_indices[1] = other;
    }

    if (other >= 0) {
        g_open_chest_count = 2;
        g_open_chest_rows = 6;
        snprintf(g_open_chest_title, sizeof(g_open_chest_title), "Large chest");
    }

    set_screen(SCREEN_CHEST);
    if (g_mp_connected) pex_net_send_chest_open();
    return 1;
}

static int chest_open_slot_count(void) {
    return g_open_chest_count * 27;
}

static ItemStack *chest_get_open_slot_ptr(int local_slot) {
    if (local_slot < 0 || local_slot >= chest_open_slot_count()) return NULL;
    int chest_part = local_slot / 27;
    int slot = local_slot % 27;
    int idx = g_open_chest_indices[chest_part];
    if (idx < 0 || idx >= MAX_CHEST_TILES || !g_chest_tiles[idx].active) return NULL;
    return &g_chest_tiles[idx].slots[slot];
}

static void chest_spawn_stack_like_java(int x, int y, int z, ItemStack st) {
    if (stack_empty(&st)) return;
    float ox = ((float)rand() / (float)RAND_MAX) * 0.8f + 0.1f;
    float oy = ((float)rand() / (float)RAND_MAX) * 0.8f + 0.1f;
    float oz = ((float)rand() / (float)RAND_MAX) * 0.8f + 0.1f;
    int left = st.count;
    while (left > 0) {
        int take = (rand() % 21) + 10;
        if (take > left) take = left;
        left -= take;
        spawn_item_stack((float)x + ox, (float)y + oy, (float)z + oz, make_stack(st.id, take, st.damage), 1);
    }
}

static void chest_drop_contents(int x, int y, int z) {
    int idx = chest_find_tile(x, y, z);
    if (idx >= 0) {
        ChestTile *ct = &g_chest_tiles[idx];
        for (int i = 0; i < 27; i++) {
            if (!stack_empty(&ct->slots[i])) {
                chest_spawn_stack_like_java(x, y, z, ct->slots[i]);
                stack_clear(&ct->slots[i]);
            }
        }
        chest_remove_tile_index(idx);
    }
}


static int furnace_find_tile(int x, int y, int z) {
    for (int i = 0; i < MAX_FURNACE_TILES; i++) {
        if (g_furnace_tiles[i].active && g_furnace_tiles[i].x == x && g_furnace_tiles[i].y == y && g_furnace_tiles[i].z == z) return i;
    }
    return -1;
}

static int furnace_alloc_tile(int x, int y, int z) {
    int existing = furnace_find_tile(x, y, z);
    if (existing >= 0) return existing;

    int free_i = -1;
    for (int i = 0; i < MAX_FURNACE_TILES; i++) {
        if (!g_furnace_tiles[i].active) { free_i = i; break; }
    }
    if (free_i < 0) return -1;

    FurnaceTile *ft = &g_furnace_tiles[free_i];
    memset(ft, 0, sizeof(*ft));
    ft->active = 1;
    ft->x = x; ft->y = y; ft->z = z;
    return free_i;
}

static void furnace_clear_all_tiles(void) {
    memset(g_furnace_tiles, 0, sizeof(g_furnace_tiles));
    g_open_furnace_index = -1;
}

static void furnace_close_open_inventory(void) {
    g_open_furnace_index = -1;
}

static FurnaceTile *furnace_open_tile(void) {
    if (g_open_furnace_index < 0 || g_open_furnace_index >= MAX_FURNACE_TILES) return NULL;
    if (!g_furnace_tiles[g_open_furnace_index].active) return NULL;
    return &g_furnace_tiles[g_open_furnace_index];
}

static ItemStack *furnace_get_slot_ptr(int local_slot) {
    FurnaceTile *ft = furnace_open_tile();
    if (!ft || local_slot < 0 || local_slot >= 3) return NULL;
    return &ft->slots[local_slot];
}

static void furnace_remove_tile_index(int idx) {
    if (idx < 0 || idx >= MAX_FURNACE_TILES) return;
    memset(&g_furnace_tiles[idx], 0, sizeof(g_furnace_tiles[idx]));
    if (g_open_furnace_index == idx) furnace_close_open_inventory();
    g_save_dirty = 1;
}

static int furnace_smelting_result(int id) {
    if (id == BLOCK_IRON_ORE) return ITEM_INGOT_IRON;
    if (id == BLOCK_GOLD_ORE) return ITEM_INGOT_GOLD;
    if (id == BLOCK_DIAMOND_ORE) return ITEM_DIAMOND;
    if (id == BLOCK_SAND) return BLOCK_GLASS;
    if (id == ITEM_PORK_RAW) return ITEM_PORK_COOKED;
    if (id == ITEM_FISH_RAW) return ITEM_FISH_COOKED;
    if (id == BLOCK_COBBLESTONE) return BLOCK_STONE;
    if (id == ITEM_CLAY) return ITEM_BRICK;
    return -1;
}

static int furnace_is_wood_material_block(int id) {
    switch (id) {
        case BLOCK_PLANKS:
        case BLOCK_LOG:
        case BLOCK_BOOKSHELF:
        case BLOCK_CHEST:
        case BLOCK_CRAFTING_TABLE:
        case BLOCK_WOOD_STAIRS:
        case BLOCK_FENCE:
        case BLOCK_JUKEBOX:
        case BLOCK_SIGN_POST:
        case BLOCK_WALL_SIGN:
        case BLOCK_WOOD_DOOR:
            return 1;
        default:
            return 0;
    }
}

static int furnace_item_burn_time(const ItemStack *fuel) {
    if (stack_empty(fuel)) return 0;
    int id = fuel->id;
    if (id < 256 && furnace_is_wood_material_block(id)) return 300;
    if (id == ITEM_STICK) return 100;
    if (id == ITEM_COAL) return 1600;
    if (id == ITEM_LAVA_BUCKET) return 20000;
    return 0;
}

static int furnace_can_smelt(const FurnaceTile *ft) {
    if (!ft || stack_empty(&ft->slots[0])) return 0;
    int out_id = furnace_smelting_result(ft->slots[0].id);
    if (out_id < 0) return 0;
    const ItemStack *out = &ft->slots[2];
    if (stack_empty(out)) return 1;
    if (out->id != out_id || out->damage != 0) return 0;
    return out->count < 64 && out->count < stack_limit_for_id(out_id);
}

static void furnace_smelt_item(FurnaceTile *ft) {
    if (!furnace_can_smelt(ft)) return;
    int out_id = furnace_smelting_result(ft->slots[0].id);
    if (stack_empty(&ft->slots[2])) ft->slots[2] = make_stack(out_id, 1, 0);
    else ft->slots[2].count++;

    ft->slots[0].count--;
    if (ft->slots[0].count <= 0) stack_clear(&ft->slots[0]);
}

static void furnace_set_lit_at(FurnaceTile *ft, int lit) {
    if (!ft) return;
    int cur = flat_get_block(ft->x, ft->y, ft->z);
    int want = lit ? BLOCK_FURNACE_LIT : BLOCK_FURNACE;
    if ((cur == BLOCK_FURNACE || cur == BLOCK_FURNACE_LIT) && cur != want) {
        int meta = flat_get_meta(ft->x, ft->y, ft->z);
        flat_begin_persistent_edit();
        flat_set_block(ft->x, ft->y, ft->z, want);
        flat_set_meta_raw(ft->x, ft->y, ft->z, meta);
        flat_end_persistent_edit();
        /* flat_end_persistent_edit() already did the tiny responsive preview;
           the full light flood-fill stays pending for the stream service. */
    }
}

static void furnace_tick_tile(FurnaceTile *ft) {
    if (!ft || !ft->active) return;
    int block = flat_get_block(ft->x, ft->y, ft->z);
    if (block != BLOCK_FURNACE && block != BLOCK_FURNACE_LIT) return;

    int was_burning = ft->burn_time > 0;
    int changed = 0;

    if (ft->burn_time > 0) {
        ft->burn_time--;
        changed = 1;
    }

    if (ft->burn_time == 0 && furnace_can_smelt(ft)) {
        ft->current_item_burn_time = ft->burn_time = furnace_item_burn_time(&ft->slots[1]);
        if (ft->burn_time > 0) {
            changed = 1;
            if (!stack_empty(&ft->slots[1])) {
                ft->slots[1].count--;
                if (ft->slots[1].count <= 0) stack_clear(&ft->slots[1]);
            }
        }
    }

    if (ft->burn_time > 0 && furnace_can_smelt(ft)) {
        ft->cook_time++;
        changed = 1;
        if (ft->cook_time >= 200) {
            ft->cook_time = 0;
            furnace_smelt_item(ft);
        }
    } else if (ft->cook_time != 0) {
        ft->cook_time = 0;
        changed = 1;
    }

    if (was_burning != (ft->burn_time > 0)) {
        furnace_set_lit_at(ft, ft->burn_time > 0);
        changed = 1;
    }

    if (changed) g_save_dirty = 1;
}

static void furnace_tick_all(void) {
    for (int i = 0; i < MAX_FURNACE_TILES; i++) {
        if (g_furnace_tiles[i].active) furnace_tick_tile(&g_furnace_tiles[i]);
    }
}

static int furnace_burn_scaled(int pixels) {
    FurnaceTile *ft = furnace_open_tile();
    if (!ft) return 0;
    if (ft->current_item_burn_time == 0) ft->current_item_burn_time = 200;
    return ft->burn_time * pixels / ft->current_item_burn_time;
}

static int furnace_cook_scaled(int pixels) {
    FurnaceTile *ft = furnace_open_tile();
    if (!ft) return 0;
    return ft->cook_time * pixels / 200;
}

static void furnace_on_block_placed(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (id == BLOCK_FURNACE || id == BLOCK_FURNACE_LIT) {
        furnace_alloc_tile(x, y, z);
        g_save_dirty = 1;
    }
}

static int furnace_open_at(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (id != BLOCK_FURNACE && id != BLOCK_FURNACE_LIT) return 0;
    int idx = furnace_alloc_tile(x, y, z);
    if (idx < 0) return 1;
    g_open_furnace_index = idx;
    set_screen(SCREEN_FURNACE);
    return 1;
}

static void furnace_drop_contents(int x, int y, int z) {
    int idx = furnace_find_tile(x, y, z);
    if (idx >= 0) {
        FurnaceTile *ft = &g_furnace_tiles[idx];
        for (int i = 0; i < 3; i++) {
            if (!stack_empty(&ft->slots[i])) {
                chest_spawn_stack_like_java(x, y, z, ft->slots[i]);
                stack_clear(&ft->slots[i]);
            }
        }
        furnace_remove_tile_index(idx);
    }
}

static void inventory_drop_selected_one(void) {
    ItemStack *s = &g_inventory[g_selected_hotbar_slot];
    if (stack_empty(s)) return;
    ItemStack one = make_stack(s->id, 1, s->damage);
    if (!player_is_creative()) {
        s->count--;
        if (s->count <= 0) stack_clear(s);
    }
    if (g_mp_connected) pex_net_send_drop_item(one);
    else {
        spawn_item_stack(g_player_x, g_player_y - 0.30f, g_player_z, one, 0);
        pex_sound_play("random.pop", 0.20f, 0.6f);
        g_save_dirty = 1;
    }
    restart_hand_swing();
    hud_add_chat("Dropped item.");
}



static int craft_id_at(const ItemStack *grid, int w, int x, int y) {
    const ItemStack *s = &grid[y * w + x];
    return stack_empty(s) ? 0 : s->id;
}

static int craft_count_nonempty(const ItemStack *grid, int n) {
    int c = 0;
    for (int i = 0; i < n; i++) if (!stack_empty(&grid[i])) c++;
    return c;
}

static int crafting_pattern_width(const char **rows, int h) {
    int w = 0;
    for (int y = 0; y < h; y++) {
        int len = (int)strlen(rows[y]);
        if (len > w) w = len;
    }
    return w;
}

static int crafting_match_at(const ItemStack *grid, int gw, int gh,
                             const char **rows, int pw, int ph,
                             const int map[256], int ox, int oy, int mirror) {
    for (int y = 0; y < gh; y++) {
        for (int x = 0; x < gw; x++) {
            int expected = 0;
            int rx = x - ox;
            int ry = y - oy;
            if (rx >= 0 && ry >= 0 && rx < pw && ry < ph) {
                int sx = mirror ? (pw - 1 - rx) : rx;
                char ch = ' ';
                if (sx < (int)strlen(rows[ry])) ch = rows[ry][sx];
                if (ch != ' ') expected = map[(unsigned char)ch];
            }
            if (craft_id_at(grid, gw, x, y) != expected) return 0;
        }
    }
    return 1;
}

static int crafting_match_recipe(const ItemStack *grid, int gw, int gh,
                                 const char **rows, int ph,
                                 int a_ch, int a_id, int b_ch, int b_id, int c_ch, int c_id) {
    int map[256]; memset(map, 0, sizeof(map));
    if (a_ch) map[(unsigned char)a_ch] = a_id;
    if (b_ch) map[(unsigned char)b_ch] = b_id;
    if (c_ch) map[(unsigned char)c_ch] = c_id;
    int pw = crafting_pattern_width(rows, ph);
    if (pw <= 0 || ph <= 0 || pw > gw || ph > gh) return 0;
    for (int oy = 0; oy <= gh - ph; oy++) {
        for (int ox = 0; ox <= gw - pw; ox++) {
            if (crafting_match_at(grid, gw, gh, rows, pw, ph, map, ox, oy, 0)) return 1;
            if (crafting_match_at(grid, gw, gh, rows, pw, ph, map, ox, oy, 1)) return 1;
        }
    }
    return 0;
}

static ItemStack crafting_make_result_if_match(const ItemStack *grid, int gw, int gh,
                                               int out_id, int out_count,
                                               const char **rows, int ph,
                                               int a_ch, int a_id, int b_ch, int b_id, int c_ch, int c_id) {
    if (crafting_match_recipe(grid, gw, gh, rows, ph, a_ch, a_id, b_ch, b_id, c_ch, c_id)) {
        return make_stack(out_id, out_count, 0);
    }
    ItemStack empty; memset(&empty, 0, sizeof(empty)); return empty;
}

#define TRY_RECIPE(OUT,COUNT,ROWS,PH,A,AI,B,BI,C,CI) do { \
    ItemStack r = crafting_make_result_if_match(grid, gw, gh, (OUT), (COUNT), (ROWS), (PH), (A), (AI), (B), (BI), (C), (CI)); \
    if (!stack_empty(&r)) return r; \
} while(0)

static ItemStack crafting_output_for_grid(const ItemStack *grid, int gw, int gh) {
    ItemStack empty; memset(&empty, 0, sizeof(empty));

    /* Java Beta 1.0 recipe groups from the uploaded deobfuscated source:
       RecipesTools, RecipesWeapons, RecipesIngots, RecipesFood, RecipesCrafting,
       RecipesArmor, then CraftingManager's direct recipes.  This expands to all
       99 registered Java recipes.  Matching is shaped and mirrorable like
       CraftingRecipe.matchRecipe(). */
    const int mats[5] = {BLOCK_PLANKS, BLOCK_COBBLESTONE, ITEM_INGOT_IRON, ITEM_DIAMOND, ITEM_INGOT_GOLD};
    const int pick[5] = {ITEM_WOODEN_PICKAXE, ITEM_STONE_PICKAXE, ITEM_IRON_PICKAXE, ITEM_DIAMOND_PICKAXE, ITEM_GOLD_PICKAXE};
    const int shovel[5] = {ITEM_WOODEN_SHOVEL, ITEM_STONE_SHOVEL, ITEM_IRON_SHOVEL, ITEM_DIAMOND_SHOVEL, ITEM_GOLD_SHOVEL};
    const int axe[5] = {ITEM_WOODEN_AXE, ITEM_STONE_AXE, ITEM_IRON_AXE, ITEM_DIAMOND_AXE, ITEM_GOLD_AXE};
    const int hoe[5] = {ITEM_WOODEN_HOE, ITEM_STONE_HOE, ITEM_IRON_HOE, ITEM_DIAMOND_HOE, ITEM_GOLD_HOE};
    const int sword[5] = {ITEM_WOODEN_SWORD, ITEM_STONE_SWORD, ITEM_IRON_SWORD, ITEM_DIAMOND_SWORD, ITEM_GOLD_SWORD};
    const char *pick_pat[3] = {"XXX", " # ", " # "};
    const char *shovel_pat[3] = {"X", "#", "#"};
    const char *axe_pat[3] = {"XX", "X#", " #"};
    const char *hoe_pat[3] = {"XX", " #", " #"};
    const char *sword_pat[3] = {"X", "X", "#"};
    for (int i = 0; i < 5; i++) {
        TRY_RECIPE(pick[i], 1, pick_pat, 3, 'X', mats[i], '#', ITEM_STICK, 0, 0);
        TRY_RECIPE(shovel[i], 1, shovel_pat, 3, 'X', mats[i], '#', ITEM_STICK, 0, 0);
        TRY_RECIPE(axe[i], 1, axe_pat, 3, 'X', mats[i], '#', ITEM_STICK, 0, 0);
        TRY_RECIPE(hoe[i], 1, hoe_pat, 3, 'X', mats[i], '#', ITEM_STICK, 0, 0);
        TRY_RECIPE(sword[i], 1, sword_pat, 3, 'X', mats[i], '#', ITEM_STICK, 0, 0);
    }

    const char *bow_pat[3] = {" #X", "# X", " #X"};
    TRY_RECIPE(ITEM_BOW, 1, bow_pat, 3, 'X', ITEM_STRING, '#', ITEM_STICK, 0, 0);
    const char *arrow_pat[3] = {"X", "#", "Y"};
    TRY_RECIPE(ITEM_ARROW, 4, arrow_pat, 3, 'X', ITEM_FLINT, '#', ITEM_STICK, 'Y', ITEM_FEATHER);

    const int block_mats[3] = {BLOCK_GOLD_BLOCK, BLOCK_IRON_BLOCK, BLOCK_DIAMOND_BLOCK};
    const int ingots[3] = {ITEM_INGOT_GOLD, ITEM_INGOT_IRON, ITEM_DIAMOND};
    const char *block9_pat[3] = {"###", "###", "###"};
    const char *one_pat[1] = {"#"};
    for (int i = 0; i < 3; i++) {
        TRY_RECIPE(block_mats[i], 1, block9_pat, 3, '#', ingots[i], 0, 0, 0, 0);
        TRY_RECIPE(ingots[i], 9, one_pat, 1, '#', block_mats[i], 0, 0, 0, 0);
    }

    const char *soup1[3] = {"Y", "X", "#"};
    TRY_RECIPE(ITEM_MUSHROOM_STEW, 1, soup1, 3, 'X', BLOCK_BROWN_MUSHROOM, 'Y', BLOCK_RED_MUSHROOM, '#', ITEM_BOWL_EMPTY);
    TRY_RECIPE(ITEM_MUSHROOM_STEW, 1, soup1, 3, 'X', BLOCK_RED_MUSHROOM, 'Y', BLOCK_BROWN_MUSHROOM, '#', ITEM_BOWL_EMPTY);

    const char *chest_pat[3] = {"###", "# #", "###"};
    TRY_RECIPE(BLOCK_CHEST, 1, chest_pat, 3, '#', BLOCK_PLANKS, 0, 0, 0, 0);
    TRY_RECIPE(BLOCK_FURNACE, 1, chest_pat, 3, '#', BLOCK_COBBLESTONE, 0, 0, 0, 0);
    const char *workbench_pat[2] = {"##", "##"};
    TRY_RECIPE(BLOCK_CRAFTING_TABLE, 1, workbench_pat, 2, '#', BLOCK_PLANKS, 0, 0, 0, 0);

    const int armor_mats[5] = {ITEM_LEATHER, BLOCK_FIRE, ITEM_INGOT_IRON, ITEM_DIAMOND, ITEM_INGOT_GOLD};
    const int helmets[5] = {ITEM_HELMET_LEATHER, ITEM_HELMET_CHAIN, ITEM_HELMET_IRON, ITEM_HELMET_DIAMOND, ITEM_HELMET_GOLD};
    const int plates[5]  = {ITEM_PLATE_LEATHER, ITEM_PLATE_CHAIN, ITEM_PLATE_IRON, ITEM_PLATE_DIAMOND, ITEM_PLATE_GOLD};
    const int legs[5]    = {ITEM_LEGS_LEATHER, ITEM_LEGS_CHAIN, ITEM_LEGS_IRON, ITEM_LEGS_DIAMOND, ITEM_LEGS_GOLD};
    const int boots[5]   = {ITEM_BOOTS_LEATHER, ITEM_BOOTS_CHAIN, ITEM_BOOTS_IRON, ITEM_BOOTS_DIAMOND, ITEM_BOOTS_GOLD};
    const char *helmet_pat[2] = {"XXX", "X X"};
    const char *plate_pat[3] = {"X X", "XXX", "XXX"};
    const char *legs_pat[3] = {"XXX", "X X", "X X"};
    const char *boots_pat[2] = {"X X", "X X"};
    for (int i = 0; i < 5; i++) {
        TRY_RECIPE(helmets[i], 1, helmet_pat, 2, 'X', armor_mats[i], 0, 0, 0, 0);
        TRY_RECIPE(plates[i], 1, plate_pat, 3, 'X', armor_mats[i], 0, 0, 0, 0);
        TRY_RECIPE(legs[i], 1, legs_pat, 3, 'X', armor_mats[i], 0, 0, 0, 0);
        TRY_RECIPE(boots[i], 1, boots_pat, 2, 'X', armor_mats[i], 0, 0, 0, 0);
    }

    const char *paper_pat[1] = {"###"};
    TRY_RECIPE(ITEM_PAPER, 3, paper_pat, 1, '#', ITEM_REED, 0, 0, 0, 0);
    const char *book_pat[3] = {"#", "#", "#"};
    TRY_RECIPE(ITEM_BOOK, 1, book_pat, 3, '#', ITEM_PAPER, 0, 0, 0, 0);
    const char *fence_pat[2] = {"###", "###"};
    TRY_RECIPE(BLOCK_FENCE, 2, fence_pat, 2, '#', ITEM_STICK, 0, 0, 0, 0);
    const char *jukebox_pat[3] = {"###", "#X#", "###"};
    TRY_RECIPE(BLOCK_JUKEBOX, 1, jukebox_pat, 3, '#', BLOCK_PLANKS, 'X', ITEM_DIAMOND, 0, 0);
    const char *bookshelf_pat[3] = {"###", "XXX", "###"};
    TRY_RECIPE(BLOCK_BOOKSHELF, 1, bookshelf_pat, 3, '#', BLOCK_PLANKS, 'X', ITEM_BOOK, 0, 0);
    TRY_RECIPE(BLOCK_SNOW_BLOCK, 1, workbench_pat, 2, '#', ITEM_SNOWBALL, 0, 0, 0, 0);
    TRY_RECIPE(BLOCK_CLAY, 1, workbench_pat, 2, '#', ITEM_CLAY, 0, 0, 0, 0);
    TRY_RECIPE(BLOCK_BRICK, 1, workbench_pat, 2, '#', ITEM_BRICK, 0, 0, 0, 0);
    TRY_RECIPE(BLOCK_GLOWSTONE, 1, block9_pat, 3, '#', ITEM_GLOWSTONE_DUST, 0, 0, 0, 0);
    TRY_RECIPE(BLOCK_WOOL, 1, block9_pat, 3, '#', ITEM_STRING, 0, 0, 0, 0);
    const char *tnt_pat[3] = {"X#X", "#X#", "X#X"};
    TRY_RECIPE(BLOCK_TNT, 1, tnt_pat, 3, 'X', ITEM_GUNPOWDER, '#', BLOCK_SAND, 0, 0);
    TRY_RECIPE(BLOCK_SLAB, 3, paper_pat, 1, '#', BLOCK_COBBLESTONE, 0, 0, 0, 0);
    const char *ladder_pat[3] = {"# #", "###", "# #"};
    TRY_RECIPE(BLOCK_LADDER, 1, ladder_pat, 3, '#', ITEM_STICK, 0, 0, 0, 0);
    const char *door_pat[3] = {"##", "##", "##"};
    TRY_RECIPE(ITEM_DOOR_WOOD, 1, door_pat, 3, '#', BLOCK_PLANKS, 0, 0, 0, 0);
    TRY_RECIPE(ITEM_DOOR_IRON, 1, door_pat, 3, '#', ITEM_INGOT_IRON, 0, 0, 0, 0);
    const char *sign_pat[3] = {"###", "###", " X "};
    TRY_RECIPE(ITEM_SIGN, 1, sign_pat, 3, '#', BLOCK_PLANKS, 'X', ITEM_STICK, 0, 0);
    TRY_RECIPE(BLOCK_PLANKS, 4, one_pat, 1, '#', BLOCK_LOG, 0, 0, 0, 0);
    const char *stick_pat[2] = {"#", "#"};
    TRY_RECIPE(ITEM_STICK, 4, stick_pat, 2, '#', BLOCK_PLANKS, 0, 0, 0, 0);
    const char *torch_pat[2] = {"X", "#"};
    TRY_RECIPE(BLOCK_TORCH, 4, torch_pat, 2, 'X', ITEM_COAL, '#', ITEM_STICK, 0, 0);
    const char *bowl_pat[2] = {"# #", " # "};
    TRY_RECIPE(ITEM_BOWL_EMPTY, 4, bowl_pat, 2, '#', BLOCK_PLANKS, 0, 0, 0, 0);
    const char *rail_pat[3] = {"X X", "X#X", "X X"};
    TRY_RECIPE(BLOCK_RAILS, 16, rail_pat, 3, 'X', ITEM_INGOT_IRON, '#', ITEM_STICK, 0, 0);
    const char *minecart_pat[2] = {"# #", "###"};
    TRY_RECIPE(ITEM_MINECART, 1, minecart_pat, 2, '#', ITEM_INGOT_IRON, 0, 0, 0, 0);
    const char *stack2_pat[2] = {"A", "B"};
    TRY_RECIPE(BLOCK_JACK_O_LANTERN, 1, stack2_pat, 2, 'A', BLOCK_PUMPKIN, 'B', BLOCK_TORCH, 0, 0);
    TRY_RECIPE(ITEM_MINECART_CRATE, 1, stack2_pat, 2, 'A', BLOCK_CHEST, 'B', ITEM_MINECART, 0, 0);
    TRY_RECIPE(ITEM_MINECART_POWERED, 1, stack2_pat, 2, 'A', BLOCK_FURNACE, 'B', ITEM_MINECART, 0, 0);
    TRY_RECIPE(ITEM_BOAT, 1, minecart_pat, 2, '#', BLOCK_PLANKS, 0, 0, 0, 0);
    TRY_RECIPE(ITEM_BUCKET_EMPTY, 1, bowl_pat, 2, '#', ITEM_INGOT_IRON, 0, 0, 0, 0);
    const char *flintsteel_pat[2] = {"A ", " B"};
    TRY_RECIPE(ITEM_FLINT_AND_IRON, 1, flintsteel_pat, 2, 'A', ITEM_INGOT_IRON, 'B', ITEM_FLINT, 0, 0);
    TRY_RECIPE(ITEM_BREAD, 1, paper_pat, 1, '#', ITEM_WHEAT, 0, 0, 0, 0);
    const char *stairs_pat[3] = {"#  ", "## ", "###"};
    TRY_RECIPE(BLOCK_WOOD_STAIRS, 4, stairs_pat, 3, '#', BLOCK_PLANKS, 0, 0, 0, 0);
    const char *rod_pat[3] = {"  #", " #X", "# X"};
    TRY_RECIPE(ITEM_FISHING_ROD, 1, rod_pat, 3, '#', ITEM_STICK, 'X', ITEM_STRING, 0, 0);
    TRY_RECIPE(BLOCK_COBBLE_STAIRS, 4, stairs_pat, 3, '#', BLOCK_COBBLESTONE, 0, 0, 0, 0);
    const char *painting_pat[3] = {"###", "#X#", "###"};
    TRY_RECIPE(ITEM_PAINTING, 1, painting_pat, 3, '#', ITEM_STICK, 'X', BLOCK_WOOL, 0, 0);
    TRY_RECIPE(ITEM_APPLE_GOLD, 1, painting_pat, 3, '#', BLOCK_GOLD_BLOCK, 'X', ITEM_APPLE_RED, 0, 0);
    TRY_RECIPE(BLOCK_LEVER, 1, torch_pat, 2, 'X', ITEM_STICK, '#', BLOCK_COBBLESTONE, 0, 0);
    TRY_RECIPE(BLOCK_REDSTONE_TORCH_ON, 1, torch_pat, 2, 'X', ITEM_REDSTONE, '#', ITEM_STICK, 0, 0);
    const char *clock_pat[3] = {" # ", "#X#", " # "};
    TRY_RECIPE(ITEM_CLOCK, 1, clock_pat, 3, '#', ITEM_INGOT_GOLD, 'X', ITEM_REDSTONE, 0, 0);
    TRY_RECIPE(ITEM_COMPASS, 1, clock_pat, 3, '#', ITEM_INGOT_IRON, 'X', ITEM_REDSTONE, 0, 0);
    TRY_RECIPE(BLOCK_STONE_BUTTON, 1, stick_pat, 2, '#', BLOCK_STONE, 0, 0, 0, 0);
    TRY_RECIPE(BLOCK_STONE_PRESSURE_PLATE, 1, paper_pat, 1, '#', BLOCK_STONE, 0, 0, 0, 0);
    TRY_RECIPE(BLOCK_WOOD_PRESSURE_PLATE, 1, paper_pat, 1, '#', BLOCK_PLANKS, 0, 0, 0, 0);

    return empty;
}

#undef TRY_RECIPE

static ItemStack workbench_crafting_output(void) {
    return crafting_output_for_grid(g_workbench_grid, 3, 3);
}

static ItemStack inventory_crafting_output(void) {
    return crafting_output_for_grid(g_craft_grid, 2, 2);
}

static ItemStack current_crafting_output(void) {
    if (g_screen == SCREEN_WORKBENCH) return workbench_crafting_output();
    return inventory_crafting_output();
}

static int inventory_take_crafting_output(void) {
    ItemStack out = current_crafting_output();
    if (stack_empty(&out)) return 0;

    if (g_mp_connected) {
        if (!stack_empty(&g_carried_stack)) {
            hud_add_chat("Server crafting needs an empty cursor.");
            return 0;
        }
        pex_net_send_craft_request();
        if (g_screen == SCREEN_WORKBENCH) memset(g_workbench_grid, 0, sizeof(g_workbench_grid));
        else memset(g_craft_grid, 0, sizeof(g_craft_grid));
        return 1;
    }

    if (stack_empty(&g_carried_stack)) {
        g_carried_stack = out;
    } else if (stack_same_item(&g_carried_stack, &out) &&
               g_carried_stack.count + out.count <= stack_limit_for_id(out.id)) {
        g_carried_stack.count += out.count;
    } else {
        return 0;
    }

    if (g_screen == SCREEN_WORKBENCH) {
        for (int i = 0; i < 9; i++) {
            if (!stack_empty(&g_workbench_grid[i])) {
                g_workbench_grid[i].count--;
                if (g_workbench_grid[i].count <= 0) stack_clear(&g_workbench_grid[i]);
            }
        }
    } else {
        for (int i = 0; i < 4; i++) {
            if (!stack_empty(&g_craft_grid[i])) {
                g_craft_grid[i].count--;
                if (g_craft_grid[i].count <= 0) stack_clear(&g_craft_grid[i]);
            }
        }
    }
    g_save_dirty = 1;
    return 1;
}

static ItemStack *inventory_slot_ptr(int slot) {
    if (slot >= 0 && slot < 36) return &g_inventory[slot];
    if (armor_slot_is_armor(slot)) return &g_armor_inventory[armor_inventory_index_for_slot(slot)];
    if (slot >= 100 && slot < 104) return &g_craft_grid[slot - 100];
    if (slot >= 300 && slot < 309) return &g_workbench_grid[slot - 300];
    if (slot >= 200 && slot < 254) return chest_get_open_slot_ptr(slot - 200);
    if (slot >= 400 && slot < 403) return furnace_get_slot_ptr(slot - 400);
    return NULL;
}

static int inventory_slot_allows_place(int slot) {
    /* Java furnace output slot is take-only; players cannot place or swap
       arbitrary items into the smelt result slot. */
    return slot != 402;
}

static int inventory_slot_accepts_stack(int slot, const ItemStack *st) {
    if (!inventory_slot_allows_place(slot)) return 0;
    if (armor_slot_is_armor(slot)) return armor_stack_valid_for_slot(st, slot);
    return 1;
}

static int inventory_slot_at(int mx, int my) {
    int container_h = (g_screen == SCREEN_CHEST && g_open_chest_rows == 6) ? 222 : 166;
    int inv_x = (g_gui_w - 176) / 2;
    int inv_y = (g_gui_h - container_h) / 2;
    int rx = mx - inv_x;
    int ry = my - inv_y;

    if (g_screen == SCREEN_WORKBENCH) {
        for (int row = 0; row < 3; row++) for (int col = 0; col < 3; col++) {
            int sx = 30 + col * 18;
            int sy = 17 + row * 18;
            if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return 300 + col + row * 3;
        }
        if (rx >= 124 && rx < 142 && ry >= 35 && ry < 53) return 309;
    }

    if (g_screen == SCREEN_CHEST) {
        int rows = (g_open_chest_rows == 6) ? 6 : 3;
        int inv_slots_y = (rows == 6) ? 140 : 84;
        int hotbar_y = (rows == 6) ? 198 : 142;
        for (int row = 0; row < rows; row++) for (int col = 0; col < 9; col++) {
            int sx = 8 + col * 18;
            int sy = 18 + row * 18;
            if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return 200 + col + row * 9;
        }
        for (int row = 0; row < 3; row++) for (int col = 0; col < 9; col++) {
            int sx = 8 + col * 18;
            int sy = inv_slots_y + row * 18;
            if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return 9 + col + row * 9;
        }
        for (int col = 0; col < 9; col++) {
            int sx = 8 + col * 18;
            int sy = hotbar_y;
            if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return col;
        }
        return -1;
    }

    if (g_screen == SCREEN_FURNACE) {
        if (rx >= 56 - 1 && rx < 56 + 17 && ry >= 17 - 1 && ry < 17 + 17) return 400;
        if (rx >= 56 - 1 && rx < 56 + 17 && ry >= 53 - 1 && ry < 53 + 17) return 401;
        if (rx >= 116 - 1 && rx < 116 + 17 && ry >= 35 - 1 && ry < 35 + 17) return 402;
        for (int row = 0; row < 3; row++) for (int col = 0; col < 9; col++) {
            int sx = 8 + col * 18;
            int sy = 84 + row * 18;
            if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return 9 + col + row * 9;
        }
        for (int col = 0; col < 9; col++) {
            int sx = 8 + col * 18;
            int sy = 142;
            if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return col;
        }
        return -1;
    }

    for (int row = 0; row < 4; row++) {
        int sx = 8;
        int sy = 8 + row * 18;
        if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return 110 + row;
    }

    /* 2x2 crafting grid, matching gui_inventory.mcrw layout. */
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 2; col++) {
            int sx = 88 + col * 18;
            int sy = 26 + row * 18;
            if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return 100 + col + row * 2;
        }
    }

    /* Crafting output slot. */
    if (rx >= 144 - 1 && rx < 144 + 17 && ry >= 36 - 1 && ry < 36 + 17) return 104;

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 9; col++) {
            int sx = 8 + col * 18;
            int sy = 84 + row * 18;
            if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return 9 + col + row * 9;
        }
    }
    for (int col = 0; col < 9; col++) {
        int sx = 8 + col * 18;
        int sy = 142;
        if (rx >= sx - 1 && rx < sx + 17 && ry >= sy - 1 && ry < sy + 17) return col;
    }
    return -1;
}

static void inventory_mouse_click(int mx, int my, int button) {
    int sync_chest_after = (g_mp_connected && g_screen == SCREEN_CHEST);
    int container_h = (g_screen == SCREEN_CHEST && g_open_chest_rows == 6) ? 222 : 166;
    int inv_x = (g_gui_w - 176) / 2;
    int inv_y = (g_gui_h - container_h) / 2;
    int outside = (mx < inv_x || my < inv_y || mx >= inv_x + 176 || my >= inv_y + container_h);
    int slot = inventory_slot_at(mx, my);
    int armor_slot_touched = armor_slot_is_armor(slot);
    if (outside && !stack_empty(&g_carried_stack)) {
        if (button == 0) { spawn_item_stack(g_player_x, g_player_y - 0.30f, g_player_z, g_carried_stack, 0); stack_clear(&g_carried_stack); }
        else { ItemStack one = make_stack(g_carried_stack.id, 1, g_carried_stack.damage); spawn_item_stack(g_player_x, g_player_y - 0.30f, g_player_z, one, 0); if (--g_carried_stack.count <= 0) stack_clear(&g_carried_stack); }
        return;
    }
    if (slot < 0) return;

    if (slot == 104 || slot == 309) {
        inventory_take_crafting_output();
        return;
    }

    ItemStack *s = inventory_slot_ptr(slot);
    if (!s) return;

    if (button == 0) {
        if (stack_empty(&g_carried_stack)) {
            if (!stack_empty(s)) { g_carried_stack = *s; stack_clear(s); }
        } else if (stack_empty(s)) {
            if (!inventory_slot_accepts_stack(slot, &g_carried_stack)) return;
            *s = g_carried_stack; stack_clear(&g_carried_stack);
        } else if (stack_same_item(s, &g_carried_stack) && s->count < stack_limit_for_id(s->id)) {
            if (!inventory_slot_allows_place(slot)) {
                int room = stack_limit_for_id(g_carried_stack.id) - g_carried_stack.count;
                int move = s->count < room ? s->count : room;
                if (move <= 0) return;
                g_carried_stack.count += move;
                s->count -= move;
                if (s->count <= 0) stack_clear(s);
            } else {
                int room = stack_limit_for_id(s->id) - s->count;
                int move = g_carried_stack.count < room ? g_carried_stack.count : room;
                s->count += move;
                g_carried_stack.count -= move;
                s->pop_time = 5;
                if (g_carried_stack.count <= 0) stack_clear(&g_carried_stack);
            }
        } else {
            if (!inventory_slot_accepts_stack(slot, &g_carried_stack)) return;
            ItemStack tmp = *s; *s = g_carried_stack; g_carried_stack = tmp;
        }
    } else {
        if (stack_empty(&g_carried_stack)) {
            if (!stack_empty(s)) {
                int take = (s->count + 1) / 2;
                g_carried_stack = make_stack(s->id, take, s->damage);
                s->count -= take;
                if (s->count <= 0) stack_clear(s);
            }
        } else if (stack_empty(s)) {
            if (!inventory_slot_accepts_stack(slot, &g_carried_stack)) return;
            *s = make_stack(g_carried_stack.id, 1, g_carried_stack.damage);
            if (--g_carried_stack.count <= 0) stack_clear(&g_carried_stack);
        } else if (stack_same_item(s, &g_carried_stack) && s->count < stack_limit_for_id(s->id)) {
            if (!inventory_slot_allows_place(slot)) {
                if (g_carried_stack.count >= stack_limit_for_id(g_carried_stack.id)) return;
                g_carried_stack.count++;
                s->count--;
                if (s->count <= 0) stack_clear(s);
            } else {
                s->count++;
                s->pop_time = 5;
                if (--g_carried_stack.count <= 0) stack_clear(&g_carried_stack);
            }
        }
    }
    if (armor_slot_touched) armor_sync_player_armor();
    if (sync_chest_after) pex_net_send_chest_update();
    g_save_dirty = 1;
}

static FlatRayHit flat_raycast(void) {
    FlatRayHit h; memset(&h, 0, sizeof(h)); h.face = 1;
    float dx, dy, dz;
    pex_touch_aware_look_vector(&dx, &dy, &dz);
    float px = g_player_x;
    float py = g_player_y;
    float pz = g_player_z;
    int prev_bx = (int)floorf(px), prev_by = (int)floorf(py), prev_bz = (int)floorf(pz);
    for (float t = 0.0f; t <= 5.0f; t += 0.05f) {
        float x = px + dx * t;
        float y = py + dy * t;
        float z = pz + dz * t;
        int bx = (int)floorf(x);
        int by = (int)floorf(y);
        int bz = (int)floorf(z);
        int ray_id = flat_get_block(bx, by, bz);
        /* Water/lava should not be selectable like solid blocks.  Let the
           cursor pass through liquids so the outline/mining target can be the
           block behind or under the water. */
        if (ray_id != 0 && !block_is_liquid(ray_id)) {
            /* Snow is only a 2/16-high plate.  The old full-cell ray test made
               the cursor hit invisible snow volume beside/above the visible
               layer instead of the real block behind it. */
            if (ray_id == BLOCK_SNOW_LAYER) {
                float local_y = y - floorf(y);
                if (local_y > (2.0f / 16.0f)) {
                    prev_bx = bx; prev_by = by; prev_bz = bz;
                    continue;
                }
            } else if (ray_id == BLOCK_SLAB) {
                float local_y = y - floorf(y);
                if (local_y > 0.5f) { prev_bx = bx; prev_by = by; prev_bz = bz; continue; }
            } else if (ray_id == BLOCK_STONE_PRESSURE_PLATE || ray_id == BLOCK_WOOD_PRESSURE_PLATE || ray_id == BLOCK_RAILS || ray_id == BLOCK_REDSTONE_WIRE) {
                float local_y = y - floorf(y);
                if (local_y > 0.10f) { prev_bx = bx; prev_by = by; prev_bz = bz; continue; }
            } else if (ray_id == BLOCK_LADDER) {
                float local_x = x - floorf(x), local_z = z - floorf(z);
                int meta = flat_get_meta(bx, by, bz);
                int ok = 0;
                float lt = 2.0f / 16.0f;
                if (meta == 2) ok = local_z > 1.0f - lt;
                else if (meta == 3) ok = local_z < lt;
                else if (meta == 4) ok = local_x > 1.0f - lt;
                else if (meta == 5) ok = local_x < lt;
                if (!ok) { prev_bx = bx; prev_by = by; prev_bz = bz; continue; }
            } else if (ray_id == BLOCK_STONE_BUTTON || ray_id == BLOCK_LEVER) {
                float lx = x - floorf(x), ly = y - floorf(y), lz = z - floorf(z);
                int meta = flat_get_meta(bx, by, bz) & 7;
                int ok = 0;
                if (ray_id == BLOCK_STONE_BUTTON) {
                    float t = (flat_get_meta(bx, by, bz) & 8) ? (1.0f/16.0f) : (2.0f/16.0f);
                    if (ly >= 6.0f/16.0f && ly <= 10.0f/16.0f) {
                        if (meta == 1) ok = lx <= t && lz >= 0.5f - 3.0f/16.0f && lz <= 0.5f + 3.0f/16.0f;
                        else if (meta == 2) ok = lx >= 1.0f - t && lz >= 0.5f - 3.0f/16.0f && lz <= 0.5f + 3.0f/16.0f;
                        else if (meta == 3) ok = lz <= t && lx >= 0.5f - 3.0f/16.0f && lx <= 0.5f + 3.0f/16.0f;
                        else if (meta == 4) ok = lz >= 1.0f - t && lx >= 0.5f - 3.0f/16.0f && lx <= 0.5f + 3.0f/16.0f;
                    }
                } else {
                    ok = (ly <= 0.85f && lx >= 0.30f && lx <= 0.70f && lz >= 0.30f && lz <= 0.70f);
                }
                if (!ok) { prev_bx = bx; prev_by = by; prev_bz = bz; continue; }
            } else if (block_is_door_id(ray_id)) {
                int ly = door_lower_y_at(bx, by, bz);
                int meta = flat_get_meta(bx, ly, bz);
                int dir = door_collision_state_from_meta(meta);
                float local_x = x - floorf(x), local_z = z - floorf(z);
                int ok = 0;
                if (dir == 0) ok = local_z < 0.20f;
                else if (dir == 2) ok = local_z > 0.80f;
                else if (dir == 1) ok = local_x > 0.80f;
                else ok = local_x < 0.20f;
                if (!ok) {
                    prev_bx = bx; prev_by = by; prev_bz = bz;
                    continue;
                }
            }
            float block_dx = x - g_player_x;
            float block_dy = y - g_player_y;
            float block_dz = z - g_player_z;
            float block_dist = sqrtf(block_dx * block_dx + block_dy * block_dy + block_dz * block_dz);
            float player_t = 9999.0f;
            if (pex_net_player_ray_distance(block_dist, &player_t) && player_t < block_dist - 0.02f) {
                return h;
            }
            h.hit = 1; h.bx = bx; h.by = by; h.bz = bz; h.hx = x; h.hy = y; h.hz = z;
            if (prev_by < by) h.face = 0;
            else if (prev_by > by) h.face = 1;
            else if (prev_bz < bz) h.face = 2;
            else if (prev_bz > bz) h.face = 3;
            else if (prev_bx < bx) h.face = 4;
            else if (prev_bx > bx) h.face = 5;
            return h;
        }
        prev_bx = bx; prev_by = by; prev_bz = bz;
    }
    return h;
}

/* Ray that can land on a liquid SOURCE block (the normal raycast passes through
   liquids). Stops at the first solid block. Only level-0 sources are pickable. */
static int flat_raycast_liquid_source(int *ox, int *oy, int *oz) {
    float dx, dy, dz;
    pex_touch_aware_look_vector(&dx, &dy, &dz);
    for (float t = 0.0f; t <= 5.0f; t += 0.05f) {
        int bx = (int)floorf(g_player_x + dx * t);
        int by = (int)floorf(g_player_y + dy * t);
        int bz = (int)floorf(g_player_z + dz * t);
        int id = flat_get_block(bx, by, bz);
        if (id == 0) continue;
        if (block_is_liquid(id)) {
            if (flat_get_level(bx, by, bz) == 0) { *ox = bx; *oy = by; *oz = bz; return 1; }
            continue;
        }
        return 0;
    }
    return 0;
}

static void start_air_swing_once(void) {
    if (g_air_swing_consumed || g_air_swing_playing) return;
    g_air_swing_consumed = 1;
    g_air_swing_playing = 1;
    g_air_swing_ticks = 0;
    g_prev_hand_swing = 0.0f;
    g_hand_swing = 0.0f;
    g_hand_swing_ticks = 0;
}

static void reset_breaking_state(void) {
    if (g_mp_connected && g_breaking_block) {
        net_send_action_progress(PEX_ACTION_MINE_HIT, g_break_x, g_break_y, g_break_z, g_break_face, 0, -1);
    }
    g_breaking_block = 0;
    g_break_damage = 0.0f;
    g_prev_break_damage = 0.0f;
    g_break_sound_counter = 0.0f;
    g_last_sent_mine_stage = -1;
}

static int held_item_can_harvest_drop(int block_id) {
    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    int held_id = stack_empty(held) ? 0 : held->id;
    return held_item_can_harvest_block_id(held_id, block_id);
}

static int door_item_for_block(int id) {
    if (id == BLOCK_WOOD_DOOR) return ITEM_DOOR_WOOD;
    if (id == BLOCK_IRON_DOOR) return ITEM_DOOR_IRON;
    return id;
}

static int door_direction_from_yaw(void) {
    /* Java ItemDoor: floor((yaw + 180) * 4 / 360 - 0.5) & 3.
       The older +0.5 formula rotated placed doors by one quadrant, which also
       made the door texture flip rules look wrong. */
    int d = (int)floorf((g_player_yaw + 180.0f) * 4.0f / 360.0f - 0.5f) & 3;
    return d;
}

static int stair_direction_from_yaw(void) {
    /* Match Beta BlockStairs.onBlockPlacedBy instead of reusing door metadata. */
    int d = (int)floorf(g_player_yaw * 4.0f / 360.0f + 0.5f) & 3;
    if (d == 0) return 2;
    if (d == 1) return 1;
    if (d == 2) return 3;
    return 0;
}

static int furnace_facing_from_yaw(void) {
    int d = (int)floorf(g_player_yaw * 4.0f / 360.0f + 0.5f) & 3;
    if (d == 0) return 2;
    if (d == 1) return 5;
    if (d == 2) return 3;
    return 4;
}

static void door_toggle_at(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (!block_is_door_id(id)) return;
    if (id == BLOCK_IRON_DOOR) return; /* Java iron doors ignore player click; redstone toggles them. */
    int ly = door_lower_y_at(x, y, z);
    int meta = flat_get_meta(x, ly, z);
    meta ^= 4;
    if (!g_mp_connected) flat_begin_persistent_edit();
    flat_set_meta_raw(x, ly, z, meta);
    if (flat_get_block(x, ly + 1, z) == id) flat_set_meta_raw(x, ly + 1, z, meta + 8);
    if (!g_mp_connected) flat_end_persistent_edit();
    pex_sound_play_at((meta & 4) ? "random.door_open" : "random.door_close", (float)x + 0.5f, (float)ly + 0.5f, (float)z + 0.5f, 1.0f, 1.0f);
    if (g_mp_connected) pex_net_send_block_action(PEX_BLOCK_PLACE, ly == y ? x : x, ly, z, 0, id);
    restart_hand_swing();
}

static void door_break_at(int x, int y, int z, int drop_item) {
    int id = flat_get_block(x, y, z);
    if (!block_is_door_id(id)) return;
    int ly = door_lower_y_at(x, y, z);
    spawn_block_destroy_particles(x, ly, z, id);
    if (flat_get_block(x, ly + 1, z) == id) spawn_block_destroy_particles(x, ly + 1, z, id);
    flat_begin_persistent_edit();
    flat_set_block(x, ly + 1, z, 0);
    flat_set_block(x, ly, z, 0);
    flat_end_persistent_edit();
    if (drop_item) spawn_item_stack((float)x + 0.5f, (float)ly + 0.7f, (float)z + 0.5f, make_stack(door_item_for_block(id), 1, 0), 1);
}

static int place_door_from_item(int item_id, int x, int y, int z) {
    int block_id = (item_id == ITEM_DOOR_IRON) ? BLOCK_IRON_DOOR : BLOCK_WOOD_DOOR;
    if (y < FLAT_WORLD_Y_MIN || y + 1 > FLAT_WORLD_Y_MAX) return 0;
    if (flat_get_block(x, y - 1, z) == 0 || block_is_liquid(flat_get_block(x, y - 1, z))) return 0;
    if (flat_get_block(x, y, z) != 0 || flat_get_block(x, y + 1, z) != 0) return 0;
    if (flat_block_intersects_player(x, y, z) || flat_block_intersects_player(x, y + 1, z)) return 0;
    int dir = door_direction_from_yaw();

    int dx = 0, dz = 0;
    if (dir == 0) dz = 1;
    else if (dir == 1) dx = -1;
    else if (dir == 2) dz = -1;
    else dx = 1;

    int left_solid = (flat_block_blocks_snow(flat_get_block(x - dx, y, z - dz)) ? 1 : 0) +
                     (flat_block_blocks_snow(flat_get_block(x - dx, y + 1, z - dz)) ? 1 : 0);
    int right_solid = (flat_block_blocks_snow(flat_get_block(x + dx, y, z + dz)) ? 1 : 0) +
                      (flat_block_blocks_snow(flat_get_block(x + dx, y + 1, z + dz)) ? 1 : 0);
    int left_door = flat_get_block(x - dx, y, z - dz) == block_id || flat_get_block(x - dx, y + 1, z - dz) == block_id;
    int right_door = flat_get_block(x + dx, y, z + dz) == block_id || flat_get_block(x + dx, y + 1, z + dz) == block_id;
    if ((left_door && !right_door) || right_solid > left_solid) {
        dir = (dir - 1) & 3;
        dir += 4;
    }

    if (!g_mp_connected) flat_begin_persistent_edit();
    flat_set_block(x, y, z, block_id);
    flat_set_meta_raw(x, y, z, dir & 7);
    flat_set_block(x, y + 1, z, block_id);
    flat_set_meta_raw(x, y + 1, z, (dir & 7) + 8);
    if (!g_mp_connected) flat_end_persistent_edit();
    return 1;
}

static int flat_block_occludes_for_support(int id);

static int block_is_torch_id(int id) {
    return id == BLOCK_TORCH || id == BLOCK_REDSTONE_TORCH_OFF || id == BLOCK_REDSTONE_TORCH_ON;
}

static int torch_can_stay_with_meta(int x, int y, int z, int meta) {
    meta &= 7;
    if (meta == 1) return flat_block_occludes_for_support(flat_get_block(x - 1, y, z));
    if (meta == 2) return flat_block_occludes_for_support(flat_get_block(x + 1, y, z));
    if (meta == 3) return flat_block_occludes_for_support(flat_get_block(x, y, z - 1));
    if (meta == 4) return flat_block_occludes_for_support(flat_get_block(x, y, z + 1));
    return flat_block_occludes_for_support(flat_get_block(x, y - 1, z));
}

static int torch_can_place_at(int x, int y, int z) {
    return flat_block_occludes_for_support(flat_get_block(x - 1, y, z)) ||
           flat_block_occludes_for_support(flat_get_block(x + 1, y, z)) ||
           flat_block_occludes_for_support(flat_get_block(x, y, z - 1)) ||
           flat_block_occludes_for_support(flat_get_block(x, y, z + 1)) ||
           flat_block_occludes_for_support(flat_get_block(x, y - 1, z));
}

static int torch_meta_from_placement_face(int x, int y, int z, int face) {
    int meta = 0;
    if (face == 1 && flat_block_occludes_for_support(flat_get_block(x, y - 1, z))) meta = 5;
    if (face == 2 && flat_block_occludes_for_support(flat_get_block(x, y, z + 1))) meta = 4;
    if (face == 3 && flat_block_occludes_for_support(flat_get_block(x, y, z - 1))) meta = 3;
    if (face == 4 && flat_block_occludes_for_support(flat_get_block(x + 1, y, z))) meta = 2;
    if (face == 5 && flat_block_occludes_for_support(flat_get_block(x - 1, y, z))) meta = 1;
    if (meta != 0) return meta;
    if (flat_block_occludes_for_support(flat_get_block(x - 1, y, z))) return 1;
    if (flat_block_occludes_for_support(flat_get_block(x + 1, y, z))) return 2;
    if (flat_block_occludes_for_support(flat_get_block(x, y, z - 1))) return 3;
    if (flat_block_occludes_for_support(flat_get_block(x, y, z + 1))) return 4;
    if (flat_block_occludes_for_support(flat_get_block(x, y - 1, z))) return 5;
    return 0;
}

static int ladder_can_attach_to_face(int face) {
    return face >= 2 && face <= 5;
}

static int block_is_pressure_plate(int id) {
    return id == BLOCK_STONE_PRESSURE_PLATE || id == BLOCK_WOOD_PRESSURE_PLATE;
}

static int block_is_button_or_lever(int id) {
    return id == BLOCK_STONE_BUTTON || id == BLOCK_LEVER;
}

static int flat_block_occludes_for_support(int id) {
    if (id == 0 || block_is_liquid(id)) return 0;
    if (id == BLOCK_SLAB || id == BLOCK_WOOD_STAIRS || id == BLOCK_COBBLE_STAIRS ||
        id == BLOCK_BRICK_STAIRS || id == BLOCK_STONE_BRICK_STAIRS || id == BLOCK_NETHER_BRICK_STAIRS ||
        id == BLOCK_FENCE || id == BLOCK_NETHER_BRICK_FENCE || id == BLOCK_GLASS_PANE ||
        id == BLOCK_IRON_BARS || id == BLOCK_CACTUS || block_is_door_id(id) ||
        id == BLOCK_LADDER || id == BLOCK_RAILS || id == BLOCK_POWERED_RAIL ||
        id == BLOCK_DETECTOR_RAIL || block_is_pressure_plate(id) || id == BLOCK_STONE_BUTTON ||
        id == BLOCK_LEVER || id == BLOCK_TORCH || id == BLOCK_REDSTONE_TORCH_OFF ||
        id == BLOCK_REDSTONE_TORCH_ON || id == BLOCK_SNOW_LAYER || id == BLOCK_REDSTONE_WIRE ||
        id == BLOCK_TALL_GRASS || id == BLOCK_DEAD_BUSH || id == BLOCK_VINE ||
        id == BLOCK_LILY_PAD || id == BLOCK_NETHER_WART || id == BLOCK_END_PORTAL ||
        id == BLOCK_SIGN_POST || id == BLOCK_WALL_SIGN) return 0;
    return 1;
}

static int block_supports_attached_side(int x, int y, int z, int side_meta) {
    if (side_meta == 1) return flat_block_occludes_for_support(flat_get_block(x - 1, y, z));
    if (side_meta == 2) return flat_block_occludes_for_support(flat_get_block(x + 1, y, z));
    if (side_meta == 3) return flat_block_occludes_for_support(flat_get_block(x, y, z - 1));
    if (side_meta == 4) return flat_block_occludes_for_support(flat_get_block(x, y, z + 1));
    return 0;
}

static int side_meta_from_placement_face(int face) {
    if (face == 5) return 1; /* support is west of the new block */
    if (face == 4) return 2; /* support is east */
    if (face == 3) return 3; /* support is north */
    if (face == 2) return 4; /* support is south */
    return 0;
}

static int lever_meta_from_placement_face(int face) {
    if (face == 1) {
        int yaw4 = (int)floorf(g_player_yaw * 4.0f / 360.0f + 0.5f) & 3;
        return (yaw4 & 1) ? 5 : 6;
    }
    return side_meta_from_placement_face(face);
}

static int lever_can_stay_with_meta(int x, int y, int z, int meta) {
    int side = meta & 7;
    if (side == 5 || side == 6) return flat_block_occludes_for_support(flat_get_block(x, y - 1, z));
    return block_supports_attached_side(x, y, z, side);
}

static int redstone_active_source_at(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    int meta = flat_get_meta(x, y, z);
    if (id == BLOCK_STONE_BUTTON) return (meta & 8) ? 15 : 0;
    if (id == BLOCK_LEVER) return (meta & 8) ? 15 : 0;
    if (block_is_pressure_plate(id)) return (meta & 1) ? 15 : 0;
    if (id == BLOCK_REDSTONE_TORCH_ON) return 15;
    return 0;
}

static int attached_source_powers_block(int sx, int sy, int sz, int x, int y, int z) {
    int id = flat_get_block(sx, sy, sz);
    int meta = flat_get_meta(sx, sy, sz);
    if ((id != BLOCK_STONE_BUTTON && id != BLOCK_LEVER) || !(meta & 8)) return 0;
    int side = meta & 7;
    if (side == 1) return x == sx - 1 && y == sy && z == sz;
    if (side == 2) return x == sx + 1 && y == sy && z == sz;
    if (side == 3) return x == sx && y == sy && z == sz - 1;
    if (side == 4) return x == sx && y == sy && z == sz + 1;
    if (side == 5 || side == 6) return x == sx && y == sy - 1 && z == sz;
    return 0;
}

static int redstone_is_strongly_powered(int x, int y, int z) {
    if (!flat_in_bounds(x, y, z)) return 0;
    if (block_is_pressure_plate(flat_get_block(x, y + 1, z)) && (flat_get_meta(x, y + 1, z) & 1)) return 1;

    static const int d[6][3] = {{1,0,0},{-1,0,0},{0,0,1},{0,0,-1},{0,1,0},{0,-1,0}};
    for (int i = 0; i < 6; i++) {
        int sx = x + d[i][0], sy = y + d[i][1], sz = z + d[i][2];
        if (attached_source_powers_block(sx, sy, sz, x, y, z)) return 1;
        if (flat_get_block(sx, sy, sz) == BLOCK_REDSTONE_TORCH_ON) return 1;
    }
    return 0;
}

static int redstone_wire_strength_at(int x, int y, int z) {
    return flat_get_block(x, y, z) == BLOCK_REDSTONE_WIRE ? (flat_get_meta(x, y, z) & 15) : 0;
}

static int redstone_calc_wire_strength_at(int x, int y, int z) {
    int best = 0;
    static const int d[6][3] = {{1,0,0},{-1,0,0},{0,0,1},{0,0,-1},{0,1,0},{0,-1,0}};
    for (int i = 0; i < 6; i++) {
        int nx = x + d[i][0], ny = y + d[i][1], nz = z + d[i][2];
        int src = redstone_active_source_at(nx, ny, nz);
        if (src > best) best = src;
        if (redstone_is_strongly_powered(nx, ny, nz) && best < 15) best = 15;
        int w = redstone_wire_strength_at(nx, ny, nz);
        if (w > 0 && w - 1 > best) best = w - 1;

        /* Basic vertical wire stepping for one-block rises/drops. */
        w = redstone_wire_strength_at(nx, ny + 1, nz);
        if (w > 0 && w - 1 > best) best = w - 1;
        w = redstone_wire_strength_at(nx, ny - 1, nz);
        if (w > 0 && w - 1 > best) best = w - 1;
    }
    if (best < 0) best = 0;
    if (best > 15) best = 15;
    return best;
}

static int redstone_door_should_open(int x, int lower_y, int z) {
    static const int d[6][3] = {{1,0,0},{-1,0,0},{0,0,1},{0,0,-1},{0,1,0},{0,-1,0}};
    for (int half = 0; half < 2; half++) {
        int y = lower_y + half;
        if (redstone_is_strongly_powered(x, y, z)) return 1;
        for (int i = 0; i < 6; i++) {
            int nx = x + d[i][0], ny = y + d[i][1], nz = z + d[i][2];
            if (redstone_active_source_at(nx, ny, nz) > 0) return 1;
            if (redstone_wire_strength_at(nx, ny, nz) > 0) return 1;
            if (redstone_is_strongly_powered(nx, ny, nz)) return 1;
        }
    }
    return 0;
}

static void redstone_set_door_open_if_needed(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (!block_is_door_id(id)) return;
    int ly = door_lower_y_at(x, y, z);
    int lower_meta = flat_get_meta(x, ly, z);
    int should_open = redstone_door_should_open(x, ly, z);
    int is_open = (lower_meta & 4) ? 1 : 0;
    if (should_open == is_open) return;
    lower_meta = should_open ? (lower_meta | 4) : (lower_meta & ~4);
    flat_set_meta_raw(x, ly, z, lower_meta);
    if (flat_get_block(x, ly + 1, z) == id) flat_set_meta_raw(x, ly + 1, z, lower_meta + 8);
}

static void redstone_update_near(int cx, int cy, int cz) {
    int radius = 16;
    int y0 = cy - 3, y1 = cy + 3;
    if (y0 < FLAT_WORLD_Y_MIN) y0 = FLAT_WORLD_Y_MIN;
    if (y1 > FLAT_WORLD_Y_MAX) y1 = FLAT_WORLD_Y_MAX;

    flat_begin_persistent_edit();
    for (int pass = 0; pass < 16; pass++) {
        int changed = 0;
        for (int y = y0; y <= y1; y++) {
            for (int z = cz - radius; z <= cz + radius; z++) {
                for (int x = cx - radius; x <= cx + radius; x++) {
                    if (!flat_in_bounds(x, y, z) || flat_get_block(x, y, z) != BLOCK_REDSTONE_WIRE) continue;
                    int old = flat_get_meta(x, y, z) & 15;
                    int now = redstone_calc_wire_strength_at(x, y, z);
                    if (old != now) { flat_set_meta_raw(x, y, z, now); changed = 1; }
                }
            }
        }
        if (!changed) break;
    }

    for (int y = y0; y <= y1; y++) {
        for (int z = cz - radius; z <= cz + radius; z++) {
            for (int x = cx - radius; x <= cx + radius; x++) {
                int id = flat_get_block(x, y, z);
                if (block_is_door_id(id) && !door_meta_is_upper(flat_get_meta(x, y, z))) {
                    redstone_set_door_open_if_needed(x, y, z);
                }
            }
        }
    }
    flat_end_persistent_edit();
}

static void add_button_timer(int x, int y, int z) {
    int slot = -1;
    for (int i = 0; i < MAX_BUTTON_TIMERS; i++) {
        if (g_button_timers[i].active && g_button_timers[i].x == x && g_button_timers[i].y == y && g_button_timers[i].z == z) {
            slot = i;
            break;
        }
        if (slot < 0 && !g_button_timers[i].active) slot = i;
    }
    if (slot < 0) slot = 0;
    g_button_timers[slot].active = 1;
    g_button_timers[slot].x = x;
    g_button_timers[slot].y = y;
    g_button_timers[slot].z = z;
    g_button_timers[slot].ticks_left = 20;
}

static void press_button_at(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (id != BLOCK_STONE_BUTTON) return;
    int meta = flat_get_meta(x, y, z);
    if (meta & 8) return;
    flat_set_meta_raw(x, y, z, (meta & 7) | 8);
    pex_sound_play_at("random.click", (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, 0.30f, 0.6f);
    add_button_timer(x, y, z);
    redstone_update_near(x, y, z);
    if (g_mp_connected) pex_net_send_block_action(PEX_BLOCK_PLACE, x, y, z, 0, id);
    restart_hand_swing();
}

static void toggle_lever_at(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (id != BLOCK_LEVER) return;
    int new_meta = flat_get_meta(x, y, z) ^ 8;
    flat_set_meta_raw(x, y, z, new_meta);
    pex_sound_play_at("random.click", (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, 0.30f, (new_meta & 8) ? 0.6f : 0.5f);
    redstone_update_near(x, y, z);
    if (g_mp_connected) pex_net_send_block_action(PEX_BLOCK_PLACE, x, y, z, 0, id);
    restart_hand_swing();
}

static void add_pressure_plate_timer(int x, int y, int z) {
    int slot = -1;
    for (int i = 0; i < MAX_PRESSURE_PLATE_TIMERS; i++) {
        if (g_pressure_plate_timers[i].active && g_pressure_plate_timers[i].x == x &&
            g_pressure_plate_timers[i].y == y && g_pressure_plate_timers[i].z == z) {
            slot = i;
            break;
        }
        if (slot < 0 && !g_pressure_plate_timers[i].active) slot = i;
    }
    if (slot < 0) slot = 0;
    g_pressure_plate_timers[slot].active = 1;
    g_pressure_plate_timers[slot].x = x;
    g_pressure_plate_timers[slot].y = y;
    g_pressure_plate_timers[slot].z = z;
    g_pressure_plate_timers[slot].ticks_left = 20;
}

static int player_over_pressure_plate_at(int x, int y, int z) {
    float feet = g_player_y - 1.62f;
    float px0 = g_player_x - 0.30f, px1 = g_player_x + 0.30f;
    float pz0 = g_player_z - 0.30f, pz1 = g_player_z + 0.30f;
    float py0 = feet, py1 = feet + 0.25f;
    float e = 2.0f / 16.0f;
    float x0 = (float)x + e, x1 = (float)(x + 1) - e;
    float z0 = (float)z + e, z1 = (float)(z + 1) - e;
    return px1 > x0 && px0 < x1 && pz1 > z0 && pz0 < z1 && py1 >= (float)y && py0 <= (float)y + 0.25f;
}

static void update_buttons_and_plates(void) {
    for (int i = 0; i < MAX_BUTTON_TIMERS; i++) {
        ButtonTimer *bt = &g_button_timers[i];
        if (!bt->active) continue;
        if (flat_get_block(bt->x, bt->y, bt->z) != BLOCK_STONE_BUTTON) { bt->active = 0; continue; }
        if (--bt->ticks_left <= 0) {
            int meta = flat_get_meta(bt->x, bt->y, bt->z);
            if (meta & 8) {
                flat_set_meta_raw(bt->x, bt->y, bt->z, meta & 7);
                pex_sound_play_at("random.click", (float)bt->x + 0.5f, (float)bt->y + 0.5f, (float)bt->z + 0.5f, 0.30f, 0.5f);
                redstone_update_near(bt->x, bt->y, bt->z);
                if (g_mp_connected) pex_net_send_block_action(PEX_BLOCK_PLACE, bt->x, bt->y, bt->z, 0, BLOCK_STONE_BUTTON);
            }
            bt->active = 0;
        }
    }

    int range = 8;
    int px = (int)floorf(g_player_x);
    int pz = (int)floorf(g_player_z);
    int min_x = px - range, max_x = px + range;
    int min_z = pz - range, max_z = pz + range;
    if (min_x < g_flat_world_origin_x) min_x = g_flat_world_origin_x;
    if (max_x >= g_flat_world_origin_x + FLAT_WORLD_SIZE) max_x = g_flat_world_origin_x + FLAT_WORLD_SIZE - 1;
    if (min_z < g_flat_world_origin_z) min_z = g_flat_world_origin_z;
    if (max_z >= g_flat_world_origin_z + FLAT_WORLD_SIZE) max_z = g_flat_world_origin_z + FLAT_WORLD_SIZE - 1;

    int fy = (int)floorf(g_player_y - 1.62f);
    for (int y = fy - 1; y <= fy + 1; y++) {
        if (y < FLAT_WORLD_Y_MIN || y > FLAT_WORLD_Y_MAX) continue;
        for (int z = min_z; z <= max_z; z++) for (int x = min_x; x <= max_x; x++) {
            int id = flat_get_block(x, y, z);
            if (!block_is_pressure_plate(id)) continue;
            if (player_over_pressure_plate_at(x, y, z)) {
                if ((flat_get_meta(x, y, z) & 1) == 0) {
                    flat_set_meta_raw(x, y, z, 1);
                    pex_sound_play_at("random.click", (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, 0.30f, 0.6f);
                    redstone_update_near(x, y, z);
                    if (g_mp_connected) pex_net_send_block_action(PEX_BLOCK_PLACE, x, y, z, 0, id);
                }
                add_pressure_plate_timer(x, y, z);
            }
        }
    }

    for (int i = 0; i < MAX_PRESSURE_PLATE_TIMERS; i++) {
        PressurePlateTimer *pt = &g_pressure_plate_timers[i];
        if (!pt->active) continue;
        int id = flat_get_block(pt->x, pt->y, pt->z);
        if (!block_is_pressure_plate(id)) { pt->active = 0; continue; }
        if (player_over_pressure_plate_at(pt->x, pt->y, pt->z)) {
            pt->ticks_left = 20;
            if ((flat_get_meta(pt->x, pt->y, pt->z) & 1) == 0) {
                flat_set_meta_raw(pt->x, pt->y, pt->z, 1);
                pex_sound_play_at("random.click", (float)pt->x + 0.5f, (float)pt->y + 0.5f, (float)pt->z + 0.5f, 0.30f, 0.6f);
                redstone_update_near(pt->x, pt->y, pt->z);
                if (g_mp_connected) pex_net_send_block_action(PEX_BLOCK_PLACE, pt->x, pt->y, pt->z, 0, id);
            }
            continue;
        }
        if (--pt->ticks_left <= 0) {
            if (flat_get_meta(pt->x, pt->y, pt->z) & 1) {
                flat_set_meta_raw(pt->x, pt->y, pt->z, 0);
                pex_sound_play_at("random.click", (float)pt->x + 0.5f, (float)pt->y + 0.5f, (float)pt->z + 0.5f, 0.30f, 0.5f);
                redstone_update_near(pt->x, pt->y, pt->z);
                if (g_mp_connected) pex_net_send_block_action(PEX_BLOCK_PLACE, pt->x, pt->y, pt->z, 0, id);
            }
            pt->active = 0;
        }
    }
}

static int cactus_can_stay_at(int x, int y, int z) {
    int below = flat_get_block(x, y - 1, z);
    if (below != BLOCK_SAND && below != BLOCK_CACTUS) return 0;
    if (flat_get_block(x + 1, y, z) != 0) return 0;
    if (flat_get_block(x - 1, y, z) != 0) return 0;
    if (flat_get_block(x, y, z + 1) != 0) return 0;
    if (flat_get_block(x, y, z - 1) != 0) return 0;
    return 1;
}

static int reeds_can_stay_at(int x, int y, int z) {
    int below = flat_get_block(x, y - 1, z);
    if (below == BLOCK_REEDS) return 1;
    if (below != BLOCK_GRASS && below != BLOCK_DIRT && below != BLOCK_SAND) return 0;
    return block_is_water(flat_get_block(x + 1, y - 1, z)) || block_is_water(flat_get_block(x - 1, y - 1, z)) ||
           block_is_water(flat_get_block(x, y - 1, z + 1)) || block_is_water(flat_get_block(x, y - 1, z - 1));
}

static void drop_block_no_particles(int x, int y, int z, int drop_ok) {
    int id = flat_get_block(x, y, z);
    if (id == 0) return;
    flat_set_block(x, y, z, 0);
    int drop = block_drop_id(id);
    if (drop_ok && drop > 0) spawn_item_stack((float)x + 0.5f, (float)y + 0.7f, (float)z + 0.5f, make_stack(drop, 1, 0), 1);
}

static void unsupported_block_neighbor_cleanup(int x, int y, int z) {
    int above = flat_get_block(x, y + 1, z);
    if (block_is_door_id(above) && door_meta_is_upper(flat_get_meta(x, y + 1, z))) {
        door_break_at(x, y + 1, z, 1);
    }

    /* Vertical stacks that depend on their root/support. */
    for (int yy = y + 1; yy <= FLAT_WORLD_Y_MAX; yy++) {
        int id = flat_get_block(x, yy, z);
        if (id == BLOCK_CACTUS) {
            if (!cactus_can_stay_at(x, yy, z)) drop_block_no_particles(x, yy, z, 1);
            else break;
        } else if (id == BLOCK_REEDS) {
            if (!reeds_can_stay_at(x, yy, z)) drop_block_no_particles(x, yy, z, 1);
            else break;
        } else break;
    }

    const int dirs[6][3] = {{0,1,0},{0,-1,0},{1,0,0},{-1,0,0},{0,0,1},{0,0,-1}};
    for (int i = 0; i < 6; i++) {
        int nx = x + dirs[i][0], ny = y + dirs[i][1], nz = z + dirs[i][2];
        int id = flat_get_block(nx, ny, nz);
        if (id == 0) continue;

        if (id == BLOCK_SNOW_LAYER || id == BLOCK_CROPS || id == BLOCK_SAPLING ||
            id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE ||
            id == BLOCK_BROWN_MUSHROOM || id == BLOCK_RED_MUSHROOM ||
            block_is_pressure_plate(id) || id == BLOCK_RAILS || id == BLOCK_REDSTONE_WIRE) {
            int below = flat_get_block(nx, ny - 1, nz);
            int ok = below != 0 && !block_is_liquid(below);
            if (id == BLOCK_CROPS) ok = (below == BLOCK_FARMLAND);
            if (!ok) drop_block_no_particles(nx, ny, nz, 1);
            continue;
        }
        if (block_is_torch_id(id)) {
            int m = flat_get_meta(nx, ny, nz) & 7;
            if (m == 0) m = 5;
            if (!torch_can_stay_with_meta(nx, ny, nz, m)) drop_block_no_particles(nx, ny, nz, 1);
            continue;
        }
        if (id == BLOCK_LADDER || id == BLOCK_WALL_SIGN || block_is_button_or_lever(id)) {
            int m = flat_get_meta(nx, ny, nz) & 7;
            if (id == BLOCK_LADDER || id == BLOCK_WALL_SIGN) {
                int ok = 0;
                if (m == 2) ok = flat_block_occludes_for_support(flat_get_block(nx, ny, nz + 1));
                else if (m == 3) ok = flat_block_occludes_for_support(flat_get_block(nx, ny, nz - 1));
                else if (m == 4) ok = flat_block_occludes_for_support(flat_get_block(nx + 1, ny, nz));
                else if (m == 5) ok = flat_block_occludes_for_support(flat_get_block(nx - 1, ny, nz));
                if (!ok) drop_block_no_particles(nx, ny, nz, 1);
            } else if (id == BLOCK_LEVER) {
                if (!lever_can_stay_with_meta(nx, ny, nz, m)) drop_block_no_particles(nx, ny, nz, 1);
            } else if (!block_supports_attached_side(nx, ny, nz, m)) {
                drop_block_no_particles(nx, ny, nz, 1);
            }
            continue;
        }
    }
}
static void break_target_block(void) {
    int id = flat_get_block(g_break_x, g_break_y, g_break_z);
    int break_meta = flat_get_meta(g_break_x, g_break_y, g_break_z);
    if (id == 0 || (id == BLOCK_BEDROCK && !player_is_creative())) return;
    player_add_exhaustion(0.025f);
    if (id == BLOCK_GLASS) pex_sound_play_at("random.glass", (float)g_break_x + 0.5f, (float)g_break_y + 0.5f, (float)g_break_z + 0.5f, 1.0f, 1.0f);
    else pex_sound_play_at(pex_block_step_sound_key(id), (float)g_break_x + 0.5f, (float)g_break_y + 0.5f, (float)g_break_z + 0.5f, 1.0f, 0.8f);

    if (block_is_door_id(id)) {
        if (g_mp_connected) {
            pex_net_send_player_action(PEX_ACTION_BREAK, g_break_x, g_break_y, g_break_z, g_break_face, id);
            pex_net_send_block_action(PEX_BLOCK_BREAK, g_break_x, g_break_y, g_break_z, g_break_face, 0);
            flat_set_block(g_break_x, g_break_y, g_break_z, 0);
        } else {
            door_break_at(g_break_x, g_break_y, g_break_z, !player_is_creative() && held_item_can_harvest_drop(id));
            redstone_update_near(g_break_x, g_break_y, g_break_z);
        }
        restart_hand_swing();
        return;
    }

    /* Source EffectRenderer.func_1186_a: spawn destroy particles while the
       block still exists, then remove it. */
    spawn_block_destroy_particles(g_break_x, g_break_y, g_break_z, id);

    if (g_mp_connected) {
        pex_net_send_player_action(PEX_ACTION_BREAK, g_break_x, g_break_y, g_break_z, g_break_face, id);
        flat_set_block(g_break_x, g_break_y, g_break_z, 0);
        pex_net_send_block_action(PEX_BLOCK_BREAK, g_break_x, g_break_y, g_break_z, g_break_face, 0);
        restart_hand_swing();
        return;
    }

    flat_begin_persistent_edit();
    if (!player_is_creative() && id == BLOCK_JUKEBOX) {
        jukebox_eject_record(g_break_x, g_break_y, g_break_z);
    }
    if (!player_is_creative() && id == BLOCK_CHEST) {
        chest_drop_contents(g_break_x, g_break_y, g_break_z);
    } else if (!player_is_creative() && (id == BLOCK_FURNACE || id == BLOCK_FURNACE_LIT)) {
        furnace_drop_contents(g_break_x, g_break_y, g_break_z);
    }
    if (!player_is_creative() && id == BLOCK_ICE) {
        int below = flat_get_block(g_break_x, g_break_y - 1, g_break_z);
        if (flat_block_is_solid(below) || block_is_liquid(below)) {
            flat_place_fluid_source(g_break_x, g_break_y, g_break_z, BLOCK_WATER);
        } else {
            flat_set_block(g_break_x, g_break_y, g_break_z, 0);
        }
    } else {
        flat_set_block(g_break_x, g_break_y, g_break_z, 0);
    }
    unsupported_block_neighbor_cleanup(g_break_x, g_break_y, g_break_z);
    flat_end_persistent_edit();
    redstone_update_near(g_break_x, g_break_y, g_break_z);
    int drop = block_drop_id(id);
    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    if (!player_is_creative() && held && !stack_empty(held) && held->id == ITEM_SHEARS &&
        (id == BLOCK_WEB || id == BLOCK_LEAVES || id == BLOCK_TALL_GRASS || id == BLOCK_VINE)) {
        int drop_meta = 0;
        int drop_id = id;
        if (id == BLOCK_LEAVES) drop_meta = break_meta & 3;
        else if (id == BLOCK_TALL_GRASS) drop_meta = break_meta & 3;
        spawn_item_stack((float)g_break_x + 0.5f, (float)g_break_y + 0.7f, (float)g_break_z + 0.5f, make_stack(drop_id, 1, drop_meta), 1);
        damage_held_item(held, 1);
    } else if (!player_is_creative() && drop > 0 && held_item_can_harvest_drop(id)) {
        spawn_item_stack((float)g_break_x + 0.5f, (float)g_break_y + 0.7f, (float)g_break_z + 0.5f, make_stack(drop, 1, 0), 1);
        if (held && !stack_empty(held) && held->id == ITEM_SHEARS &&
            (id == BLOCK_WEB || id == BLOCK_LEAVES || id == BLOCK_TALL_GRASS || id == BLOCK_VINE)) damage_held_item(held, 1);
    }
    restart_hand_swing();
}

static void update_breaking(void) {
    g_prev_break_damage = g_break_damage;

    if (g_block_hit_delay > 0) {
        g_block_hit_delay--;
        return;
    }

    if (!key_down_vk(VK_LBUTTON)) {
        reset_breaking_state();
        return;
    }

    FlatRayHit hit = flat_raycast();
    if (!hit.hit) {
        reset_breaking_state();
        return;
    }

    int id = flat_get_block(hit.bx, hit.by, hit.bz);
    if (id == 0 || (id == BLOCK_BEDROCK && !player_is_creative()) || block_is_liquid(id)) {
        reset_breaking_state();
        return;
    }

    if (player_is_creative()) {
        /* Java PlayerControllerCreative: clickBlock destroys immediately and
           sets a 5-tick delay; holding mine repeats one destroy every 5 ticks.
           There is no crack-progress accumulation in creative. */
        g_breaking_block = 1;
        g_break_x = hit.bx; g_break_y = hit.by; g_break_z = hit.bz; g_break_face = hit.face;
        g_break_damage = 0.0f;
        g_prev_break_damage = 0.0f;
        g_break_sound_counter = 0.0f;
        g_last_sent_mine_stage = -1;
        break_target_block();
        g_block_hit_delay = 5;
        g_break_swing_holding = 1;
        return;
    }

    if (!g_breaking_block || hit.bx != g_break_x || hit.by != g_break_y || hit.bz != g_break_z) {
        /* PlayerControllerSP.onPlayerDamageBlock(): changing target only resets
           curBlockDamage/prevBlockDamage and records the new coordinates.  The
           first damage increment happens on the following tick, which is why the
           crack animation does not jump immediately in Java 1.2.5. */
        g_breaking_block = 1;
        g_break_x = hit.bx; g_break_y = hit.by; g_break_z = hit.bz; g_break_face = hit.face;
        g_break_damage = 0.0f;
        g_prev_break_damage = 0.0f;
        g_break_sound_counter = 0.0f;
        g_last_sent_mine_stage = -1;
        if (g_mp_connected) {
            net_send_action_progress(PEX_ACTION_MINE_HIT, g_break_x, g_break_y, g_break_z, g_break_face, id, 0);
            g_last_sent_mine_stage = 0;
        }
        return;
    }

    g_break_swing_holding = 1;

    if (g_ingame_ticks != g_last_hit_particle_tick ||
        g_break_x != g_last_hit_particle_x || g_break_y != g_last_hit_particle_y ||
        g_break_z != g_last_hit_particle_z || g_break_face != g_last_hit_particle_face) {
        spawn_block_hit_particle(g_break_x, g_break_y, g_break_z, g_break_face, id);
        g_last_hit_particle_x = g_break_x; g_last_hit_particle_y = g_break_y; g_last_hit_particle_z = g_break_z;
        g_last_hit_particle_face = g_break_face; g_last_hit_particle_tick = g_ingame_ticks;
    }

    float rel = block_relative_strength(id);
    if (player_has_potion(PEX_POTION_HASTE)) {
        rel *= 1.0f + 0.20f * (float)(player_potion_amplifier(PEX_POTION_HASTE) + 1);
    }
    if (player_has_potion(PEX_POTION_MINING_FATIGUE)) {
        int amp = player_potion_amplifier(PEX_POTION_MINING_FATIGUE);
        float mul = 0.30f;
        if (amp == 1) mul = 0.09f;
        else if (amp == 2) mul = 0.0027f;
        else if (amp >= 3) mul = 0.00081f;
        rel *= mul;
    }
    if (flat_player_head_in_water() && flat_block_is_underwater_target(g_break_x, g_break_y, g_break_z)) {
        rel *= 0.20f;
    }

    g_break_damage += rel;

    if (((int)g_break_sound_counter % 4) == 0) {
        /* Java PlayerControllerSP plays the step sound before incrementing
           blockDestroySoundCounter, so a newly-mined block gets feedback on the
           first damage tick and then every fourth tick. */
        pex_sound_play_at(pex_block_dig_sound_key(id), (float)g_break_x + 0.5f, (float)g_break_y + 0.5f, (float)g_break_z + 0.5f, 0.50f, 0.8f);
    }
    g_break_sound_counter += 1.0f;

    if (g_mp_connected) {
        int stage = (int)(g_break_damage * 10.0f);
        if (stage < 0) stage = 0;
        if (stage > 9) stage = 9;
        if (stage != g_last_sent_mine_stage) {
            net_send_action_progress(PEX_ACTION_MINE_HIT, g_break_x, g_break_y, g_break_z, g_break_face, id, stage);
            g_last_sent_mine_stage = stage;
        }
    }

    if (g_break_damage >= 1.0f) {
        break_target_block();
        reset_breaking_state();
        /* PlayerControllerSP.blockHitWait = 5 after a successful break. */
        g_block_hit_delay = 5;
    }
}

static void world_left_mouse_released(void) { reset_breaking_state(); g_block_hit_delay = 0; g_break_swing_consumed = 0; g_break_swing_holding = 0; }

static int item_is_placeable_block_id(int id) {
    if (id <= 0 || (id == BLOCK_BEDROCK && !player_is_creative())) return 0;
    if (id == ITEM_DOOR_WOOD || id == ITEM_DOOR_IRON || id == ITEM_REED || id == ITEM_SIGN ||
        id == ITEM_REDSTONE || id == ITEM_SEEDS) return 1;
    if (block_is_liquid(id)) return 0; /* water/lava need bucket logic, not block mining/placing */
    if (id >= 1 && id <= 91) return 1;
    return 0;
}

static int held_is_hoe_item(int id) {
    return id == ITEM_HOE_WOOD || id == ITEM_HOE_STONE || id == ITEM_HOE_IRON ||
           id == ITEM_HOE_DIAMOND || id == ITEM_HOE_GOLD;
}

static int item_food_heal_amount(int id) {
    if (id == ITEM_APPLE_RED) return 4;
    if (id == ITEM_BREAD) return 5;
    if (id == ITEM_BOWL_SOUP) return 8;
    if (id == ITEM_PORK_RAW) return 3;
    if (id == ITEM_PORK_COOKED) return 8;
    if (id == ITEM_FISH_RAW) return 2;
    if (id == ITEM_FISH_COOKED) return 5;
    if (id == ITEM_APPLE_GOLD) return 4;
    return 0;
}

static float item_food_saturation_modifier(int id) {
    if (id == ITEM_APPLE_RED) return 0.3f;
    if (id == ITEM_BREAD) return 0.6f;
    if (id == ITEM_BOWL_SOUP) return 0.6f;
    if (id == ITEM_PORK_RAW) return 0.3f;
    if (id == ITEM_PORK_COOKED) return 0.8f;
    if (id == ITEM_FISH_RAW) return 0.3f;
    if (id == ITEM_FISH_COOKED) return 0.6f;
    if (id == ITEM_APPLE_GOLD) return 1.2f;
    return 0.0f;
}

static int item_food_always_edible(int id) {
    return id == ITEM_APPLE_GOLD;
}

static void consume_held_stack_one(ItemStack *held, int replacement_id) {
    if (!held || stack_empty(held)) return;
    if (player_is_creative()) return;
    if (held->count <= 1) {
        if (replacement_id > 0) *held = make_stack(replacement_id, 1, 0);
        else stack_clear(held);
    } else {
        held->count--;
        if (replacement_id > 0) inventory_add_stack(make_stack(replacement_id, 1, 0));
    }
}

static int try_eat_held_food(ItemStack *held) {
    if (!held || stack_empty(held) || g_player_dead || g_player_health <= 0) return 0;
    int food = item_food_heal_amount(held->id);
    if (food <= 0) return 0;
    if (!player_can_eat(item_food_always_edible(held->id))) return 0;

    player_food_add_stats(food, item_food_saturation_modifier(held->id));
    consume_held_stack_one(held, held->id == ITEM_BOWL_SOUP ? ITEM_BOWL_EMPTY : 0);
    g_save_dirty = 1;
    restart_hand_swing();
    return 1;
}

static void hit_adjacent_cell(const FlatRayHit *hit, int *px, int *py, int *pz) {
    *px = hit->bx; *py = hit->by; *pz = hit->bz;
    if (hit->face == 0) (*py)--;
    else if (hit->face == 1) (*py)++;
    else if (hit->face == 2) (*pz)--;
    else if (hit->face == 3) (*pz)++;
    else if (hit->face == 4) (*px)--;
    else if (hit->face == 5) (*px)++;
}

static int item_max_damage_for_gameplay(int id) {
    if (id == ITEM_WOODEN_SHOVEL || id == ITEM_SHOVEL_GOLD) return 33;
    if (id == ITEM_STONE_SHOVEL) return 65;
    if (id == ITEM_SHOVEL_IRON) return 251;
    if (id == ITEM_SHOVEL_DIAMOND) return 1562;
    if (id == ITEM_WOODEN_PICKAXE || id == ITEM_WOODEN_AXE || id == ITEM_WOODEN_SWORD || id == ITEM_HOE_WOOD) return 60;
    if (id == ITEM_STONE_PICKAXE || id == ITEM_STONE_AXE || id == ITEM_STONE_SWORD || id == ITEM_HOE_STONE) return 132;
    if (id == ITEM_PICKAXE_IRON || id == ITEM_AXE_IRON || id == ITEM_SWORD_IRON || id == ITEM_HOE_IRON) return 251;
    if (id == ITEM_PICKAXE_DIAMOND || id == ITEM_AXE_DIAMOND || id == ITEM_SWORD_DIAMOND || id == ITEM_HOE_DIAMOND) return 1562;
    if (id == ITEM_PICKAXE_GOLD || id == ITEM_AXE_GOLD || id == ITEM_SWORD_GOLD || id == ITEM_HOE_GOLD) return 33;
    if (id == ITEM_BOW || id == ITEM_FISHING_ROD) return 385;
    if (id == ITEM_FLINT_AND_IRON) return 65;
    if (id == ITEM_SHEARS) return 238;
    return armor_stack_max_damage(id);
}

static void damage_held_item(ItemStack *held, int amount) {
    int maxd;
    if (!held || stack_empty(held) || amount <= 0 || player_is_creative()) return;
    maxd = item_max_damage_for_gameplay(held->id);
    if (maxd <= 0) return;
    held->damage += amount;
    if (held->damage > maxd) stack_clear(held);
}

static int dye_to_wool_meta(int dye_damage) {
    /* Java BlockCloth.getBlockFromDye(): dye metadata is inverted from cloth metadata. */
    return (~dye_damage) & 15;
}

typedef struct PexPotionEffectDef {
    int id;
    int duration;
    int amplifier;
} PexPotionEffectDef;

static int pex_potion_check_flag(int bits, int flag) { return (bits & (1 << flag)) != 0; }
static int pex_potion_count_flags(int bits) { int n = 0; while (bits > 0) { bits &= bits - 1; n++; } return n; }

static int pex_potion_cond_value(int not_flag, int has_comp, int invert, int comp_type, int flag, int mult, int bits) {
    int v = 0;
    if (not_flag) v = pex_potion_check_flag(bits, flag) ? 0 : 1;
    else if (comp_type != -1) {
        int c = pex_potion_count_flags(bits);
        if (comp_type == 0 && c == flag) v = 1;
        else if (comp_type == 1 && c > flag) v = 1;
        else if (comp_type == 2 && c < flag) v = 1;
    } else v = pex_potion_check_flag(bits, flag) ? 1 : 0;
    if (has_comp) v *= mult;
    if (invert) v *= -1;
    return v;
}

static int pex_potion_eval_expr_range(const char *expr, int a, int b, int bits) {
    if (!expr || a >= (int)strlen(expr) || b < 0 || a >= b) return 0;
    int i;
    for (i = a; i < b; ++i) if (expr[i] == '|') {
        int v = pex_potion_eval_expr_range(expr, a, i, bits);
        if (v > 0) return v;
        v = pex_potion_eval_expr_range(expr, i + 1, b, bits);
        return v > 0 ? v : 0;
    }
    for (i = a; i < b; ++i) if (expr[i] == '&') {
        int v1 = pex_potion_eval_expr_range(expr, a, i, bits);
        if (v1 <= 0) return 0;
        int v2 = pex_potion_eval_expr_range(expr, i + 1, b, bits);
        return v2 <= 0 ? 0 : (v1 > v2 ? v1 : v2);
    }
    int not_flag = 0, has_comp = 0, invert = 0, saw_digit = 0;
    int comp_type = -1, flag = 0, mult = 0, total = 0;
    for (i = a; i < b; ++i) {
        char ch = expr[i];
        if (ch >= '0' && ch <= '9') {
            if (has_comp) mult = ch - '0';
            else { flag = flag * 10 + (ch - '0'); saw_digit = 1; }
        } else if (ch == '*') has_comp = 1;
        else if (ch == '!') {
            if (saw_digit) {
                total += pex_potion_cond_value(not_flag, has_comp, invert, comp_type, flag, mult, bits);
                not_flag = has_comp = invert = saw_digit = 0; flag = mult = 0; comp_type = -1;
            }
            not_flag = 1;
        } else if (ch == '-') {
            if (saw_digit) {
                total += pex_potion_cond_value(not_flag, has_comp, invert, comp_type, flag, mult, bits);
                not_flag = has_comp = invert = saw_digit = 0; flag = mult = 0; comp_type = -1;
            }
            invert = 1;
        } else if (ch == '=') {
            if (saw_digit) { comp_type = 0; mult = flag; flag = 0; saw_digit = 0; }
        } else if (ch == '<') {
            if (saw_digit) { comp_type = 2; mult = flag; flag = 0; saw_digit = 0; }
        } else if (ch == '>') {
            if (saw_digit) { comp_type = 1; mult = flag; flag = 0; saw_digit = 0; }
        } else if (ch == '+') {
            if (saw_digit) {
                total += pex_potion_cond_value(not_flag, has_comp, invert, comp_type, flag, mult, bits);
                not_flag = has_comp = invert = saw_digit = 0; flag = mult = 0; comp_type = -1;
            }
        }
    }
    if (saw_digit) total += pex_potion_cond_value(not_flag, has_comp, invert, comp_type, flag, mult, bits);
    return total;
}

static int pex_potion_eval_expr(const char *expr, int bits) {
    return pex_potion_eval_expr_range(expr, 0, expr ? (int)strlen(expr) : 0, bits);
}

static const char *pex_potion_requirement(int id) {
    switch (id) {
        case PEX_POTION_REGENERATION: return "0 & !1 & !2 & !3 & 0+6";
        case PEX_POTION_SPEED: return "!0 & 1 & !2 & !3 & 1+6";
        case PEX_POTION_FIRE_RESISTANCE: return "0 & 1 & !2 & !3 & 0+6";
        case PEX_POTION_HEAL: return "0 & !1 & 2 & !3";
        case PEX_POTION_POISON: return "!0 & !1 & 2 & !3 & 2+6";
        case PEX_POTION_WEAKNESS: return "!0 & !1 & !2 & 3 & 3+6";
        case PEX_POTION_HARM: return "!0 & !1 & 2 & 3";
        case PEX_POTION_SLOWNESS: return "!0 & 1 & !2 & 3 & 3+6";
        case PEX_POTION_STRENGTH: return "0 & !1 & !2 & 3 & 3+6";
        default: return NULL;
    }
}

static const char *pex_potion_amp_requirement(int id) {
    switch (id) {
        case PEX_POTION_SPEED:
        case PEX_POTION_HASTE:
        case PEX_POTION_STRENGTH:
        case PEX_POTION_REGENERATION:
        case PEX_POTION_HARM:
        case PEX_POTION_HEAL:
        case PEX_POTION_RESISTANCE:
        case PEX_POTION_POISON:
            return "5";
        default: return NULL;
    }
}

static int pex_potion_is_instant_id(int id) { return id == PEX_POTION_HEAL || id == PEX_POTION_HARM; }
static float pex_potion_effectiveness(int id) {
    if (id == PEX_POTION_REGENERATION || id == PEX_POTION_POISON) return 0.25f;
    if (id == PEX_POTION_SLOWNESS || id == PEX_POTION_NAUSEA || id == PEX_POTION_BLINDNESS) return 0.5f;
    if (id == PEX_POTION_HASTE) return 1.5f;
    return 1.0f;
}

static int pex_potion_get_effects(int damage, PexPotionEffectDef *out, int max_out) {
    int count = 0;
    int ids[] = {PEX_POTION_REGENERATION, PEX_POTION_SPEED, PEX_POTION_FIRE_RESISTANCE, PEX_POTION_HEAL,
                 PEX_POTION_POISON, PEX_POTION_WEAKNESS, PEX_POTION_HARM, PEX_POTION_SLOWNESS, PEX_POTION_STRENGTH};
    for (int i = 0; i < (int)(sizeof(ids)/sizeof(ids[0])); ++i) {
        int id = ids[i];
        const char *req = pex_potion_requirement(id);
        int level = req ? pex_potion_eval_expr(req, damage) : 0;
        if (level <= 0) continue;
        int amp = 0;
        const char *amp_req = pex_potion_amp_requirement(id);
        if (amp_req) {
            amp = pex_potion_eval_expr(amp_req, damage);
            if (amp < 0) amp = 0;
        }
        int duration;
        if (pex_potion_is_instant_id(id)) duration = 1;
        else {
            duration = 1200 * (level * 3 + (level - 1) * 2);
            duration >>= amp;
            duration = (int)roundf((float)duration * pex_potion_effectiveness(id));
            if ((damage & 16384) != 0) duration = (int)roundf((float)duration * 0.75f + 0.5f);
        }
        if (out && count < max_out) {
            out[count].id = id;
            out[count].duration = duration;
            out[count].amplifier = amp;
        }
        count++;
    }
    return count;
}

static int pex_potion_liquid_color_for_id(int id) {
    switch (id) {
        case PEX_POTION_SPEED: return 8171462;
        case PEX_POTION_SLOWNESS: return 5926017;
        case PEX_POTION_HASTE: return 14270531;
        case PEX_POTION_MINING_FATIGUE: return 4866583;
        case PEX_POTION_STRENGTH: return 9643043;
        case PEX_POTION_HEAL: return 16262179;
        case PEX_POTION_HARM: return 4393481;
        case PEX_POTION_JUMP: return 7889559;
        case PEX_POTION_NAUSEA: return 5578058;
        case PEX_POTION_REGENERATION: return 13458603;
        case PEX_POTION_RESISTANCE: return 10044730;
        case PEX_POTION_FIRE_RESISTANCE: return 14981690;
        case PEX_POTION_WATER_BREATHING: return 3035801;
        case PEX_POTION_INVISIBILITY: return 8356754;
        case PEX_POTION_BLINDNESS: return 2039587;
        case PEX_POTION_NIGHT_VISION: return 2039713;
        case PEX_POTION_HUNGER: return 5797459;
        case PEX_POTION_WEAKNESS: return 4738376;
        case PEX_POTION_POISON: return 5149489;
        default: return 3694022;
    }
}

static int pex_potion_color_from_damage(int damage) {
    PexPotionEffectDef eff[8];
    int n = pex_potion_get_effects(damage, eff, 8);
    if (n <= 0) return 3694022;
    float r = 0.0f, g = 0.0f, b = 0.0f, weight = 0.0f;
    for (int i = 0; i < n; ++i) {
        int c = pex_potion_liquid_color_for_id(eff[i].id);
        for (int k = 0; k <= eff[i].amplifier; ++k) {
            r += (float)((c >> 16) & 255) / 255.0f;
            g += (float)((c >> 8) & 255) / 255.0f;
            b += (float)(c & 255) / 255.0f;
            weight += 1.0f;
        }
    }
    if (weight <= 0.0f) return 3694022;
    return ((int)(r / weight * 255.0f) << 16) | ((int)(g / weight * 255.0f) << 8) | (int)(b / weight * 255.0f);
}

static const char *pex_potion_effect_key(int id) {
    switch (id) {
        case PEX_POTION_SPEED: return "potion.moveSpeed";
        case PEX_POTION_SLOWNESS: return "potion.moveSlowdown";
        case PEX_POTION_HASTE: return "potion.digSpeed";
        case PEX_POTION_MINING_FATIGUE: return "potion.digSlowDown";
        case PEX_POTION_STRENGTH: return "potion.damageBoost";
        case PEX_POTION_HEAL: return "potion.heal";
        case PEX_POTION_HARM: return "potion.harm";
        case PEX_POTION_JUMP: return "potion.jump";
        case PEX_POTION_NAUSEA: return "potion.confusion";
        case PEX_POTION_REGENERATION: return "potion.regeneration";
        case PEX_POTION_RESISTANCE: return "potion.resistance";
        case PEX_POTION_FIRE_RESISTANCE: return "potion.fireResistance";
        case PEX_POTION_WATER_BREATHING: return "potion.waterBreathing";
        case PEX_POTION_INVISIBILITY: return "potion.invisibility";
        case PEX_POTION_BLINDNESS: return "potion.blindness";
        case PEX_POTION_NIGHT_VISION: return "potion.nightVision";
        case PEX_POTION_HUNGER: return "potion.hunger";
        case PEX_POTION_WEAKNESS: return "potion.weakness";
        case PEX_POTION_POISON: return "potion.poison";
        default: return "potion.empty";
    }
}

static const char *pex_potion_prefix_key(int damage) {
    static const char *keys[] = {
        "potion.prefix.mundane", "potion.prefix.uninteresting", "potion.prefix.bland", "potion.prefix.clear",
        "potion.prefix.milky", "potion.prefix.diffuse", "potion.prefix.artless", "potion.prefix.thin",
        "potion.prefix.awkward", "potion.prefix.flat", "potion.prefix.bulky", "potion.prefix.bungling",
        "potion.prefix.buttered", "potion.prefix.smooth", "potion.prefix.suave", "potion.prefix.debonair",
        "potion.prefix.thick", "potion.prefix.elegant", "potion.prefix.fancy", "potion.prefix.charming",
        "potion.prefix.dashing", "potion.prefix.refined", "potion.prefix.cordial", "potion.prefix.sparkling",
        "potion.prefix.potent", "potion.prefix.foul", "potion.prefix.odorless", "potion.prefix.rank",
        "potion.prefix.harsh", "potion.prefix.acrid", "potion.prefix.gross", "potion.prefix.stinky"
    };
    int idx = (pex_potion_check_flag(damage, 5) ? 16 : 0) | (pex_potion_check_flag(damage, 4) ? 8 : 0) |
              (pex_potion_check_flag(damage, 3) ? 4 : 0) | (pex_potion_check_flag(damage, 2) ? 2 : 0) |
              (pex_potion_check_flag(damage, 1) ? 1 : 0);
    if (idx < 0 || idx >= 32) idx = 0;
    return keys[idx];
}

static const char *pex_potion_display_name(const ItemStack *st) {
    static char buf[128];
    int dmg = st ? st->damage : 0;
    if (dmg == 0) return tr_key_default("item.emptyPotion.name", "Water Bottle");
    PexPotionEffectDef eff[8];
    int n = pex_potion_get_effects(dmg, eff, 8);
    const char *splash = ((dmg & 16384) != 0) ? tr_key_default("potion.prefix.grenade", "Splash") : "";
    if (n > 0) {
        char key[64];
        snprintf(key, sizeof(key), "%s.postfix", pex_potion_effect_key(eff[0].id));
        snprintf(buf, sizeof(buf), "%s%s%s", splash, splash[0] ? " " : "", tr_key_default(key, "Potion"));
        return buf;
    }
    snprintf(buf, sizeof(buf), "%s %s", tr_key_default(pex_potion_prefix_key(dmg), "Mundane"), tr_key_default("item.potion.name", "Potion"));
    return buf;
}

static const char *pex_potion_effect_default_name(int id) {
    switch (id) {
        case PEX_POTION_SPEED: return "Speed";
        case PEX_POTION_SLOWNESS: return "Slowness";
        case PEX_POTION_HASTE: return "Haste";
        case PEX_POTION_MINING_FATIGUE: return "Mining Fatigue";
        case PEX_POTION_STRENGTH: return "Strength";
        case PEX_POTION_HEAL: return "Instant Health";
        case PEX_POTION_HARM: return "Instant Damage";
        case PEX_POTION_JUMP: return "Jump Boost";
        case PEX_POTION_NAUSEA: return "Nausea";
        case PEX_POTION_REGENERATION: return "Regeneration";
        case PEX_POTION_RESISTANCE: return "Resistance";
        case PEX_POTION_FIRE_RESISTANCE: return "Fire Resistance";
        case PEX_POTION_WATER_BREATHING: return "Water Breathing";
        case PEX_POTION_INVISIBILITY: return "Invisibility";
        case PEX_POTION_BLINDNESS: return "Blindness";
        case PEX_POTION_NIGHT_VISION: return "Night Vision";
        case PEX_POTION_HUNGER: return "Hunger";
        case PEX_POTION_WEAKNESS: return "Weakness";
        case PEX_POTION_POISON: return "Poison";
        default: return "Effect";
    }
}

static const char *pex_potion_amplifier_roman(int amplifier) {
    switch (amplifier) {
        case 1: return " II";
        case 2: return " III";
        case 3: return " IV";
        case 4: return " V";
        default: return "";
    }
}

static const char *pex_potion_effect_display_name(int potion_id, int amplifier) {
    static char buf[96];
    const char *base = tr_key_default(pex_potion_effect_key(potion_id), pex_potion_effect_default_name(potion_id));
    snprintf(buf, sizeof(buf), "%s%s", base, pex_potion_amplifier_roman(amplifier));
    return buf;
}

static void pex_potion_duration_string(int ticks, char *out, size_t out_sz) {
    if (!out || out_sz == 0) return;
    if (ticks < 0) ticks = 0;
    int total = ticks / 20;
    snprintf(out, out_sz, "%d:%02d", total / 60, total % 60);
}

static int pex_potion_status_icon_index(int potion_id) {
    switch (potion_id) {
        case PEX_POTION_SPEED: return 0;
        case PEX_POTION_SLOWNESS: return 1;
        case PEX_POTION_HASTE: return 2;
        case PEX_POTION_MINING_FATIGUE: return 3;
        case PEX_POTION_STRENGTH: return 4;
        case PEX_POTION_HEAL: return 5;
        case PEX_POTION_HARM: return 6;
        case PEX_POTION_JUMP: return 7;
        case PEX_POTION_NAUSEA: return 8;
        case PEX_POTION_REGENERATION: return 9;
        case PEX_POTION_RESISTANCE: return 10;
        case PEX_POTION_FIRE_RESISTANCE: return 11;
        case PEX_POTION_WATER_BREATHING: return 12;
        case PEX_POTION_INVISIBILITY: return 13;
        case PEX_POTION_BLINDNESS: return 14;
        case PEX_POTION_NIGHT_VISION: return 15;
        case PEX_POTION_HUNGER: return 16;
        case PEX_POTION_WEAKNESS: return 17;
        case PEX_POTION_POISON: return 18;
        default: return -1;
    }
}

static int pex_active_potion_count(void) {
    int n = 0;
    for (int i = 1; i < PEX_POTION_MAX; ++i) if (g_player_potion_effects[i].duration > 0) ++n;
    return n;
}

static int pex_potion_tooltip_lines(const ItemStack *st, char out[][96], int max_lines) {
    if (!st || st->id != ITEM_POTION || !out || max_lines <= 0) return 0;
    PexPotionEffectDef eff[8];
    int n = pex_potion_get_effects(st->damage, eff, 8);
    int line = 0;
    if (n <= 0) {
        snprintf(out[line++], 96, "%s", tr_key_default("potion.empty", "No Effects"));
        return line;
    }
    for (int i = 0; i < n && line < max_lines; ++i) {
        char dur[32];
        if (pex_potion_is_instant_id(eff[i].id)) {
            snprintf(out[line++], 96, "%s", pex_potion_effect_display_name(eff[i].id, eff[i].amplifier));
        } else {
            pex_potion_duration_string(eff[i].duration, dur, sizeof(dur));
            snprintf(out[line++], 96, "%s (%s)", pex_potion_effect_display_name(eff[i].id, eff[i].amplifier), dur);
        }
    }
    return line;
}

static int player_has_potion(int potion_id) {
    return potion_id > 0 && potion_id < PEX_POTION_MAX && g_player_potion_effects[potion_id].duration > 0;
}

static int player_potion_amplifier(int potion_id) {
    if (!player_has_potion(potion_id)) return 0;
    return g_player_potion_effects[potion_id].amplifier;
}

static void player_apply_potion_effect(int id, int duration, int amplifier, float splash_scale) {
    if (id <= 0 || id >= PEX_POTION_MAX) return;
    if (splash_scale < 0.0f) splash_scale = 0.0f;
    if (splash_scale > 1.0f) splash_scale = 1.0f;
    if (pex_potion_is_instant_id(id)) {
        int amt = (int)((float)(6 << amplifier) * splash_scale + 0.5f);
        if (amt <= 0) return;
        if (id == PEX_POTION_HEAL) player_health_set_hearts(g_player_health + amt);
        else if (id == PEX_POTION_HARM) player_take_damage(amt, "magic");
        return;
    }
    duration = (int)((float)duration * splash_scale + 0.5f);
    if (duration <= 20) return;
    PexPotionEffectState *cur = &g_player_potion_effects[id];
    if (cur->duration <= 0 || amplifier > cur->amplifier || (amplifier == cur->amplifier && duration > cur->duration)) {
        cur->duration = duration;
        cur->amplifier = amplifier;
    }
}

static void player_apply_potion_damage(int damage, float splash_scale) {
    PexPotionEffectDef eff[8];
    int n = pex_potion_get_effects(damage, eff, 8);
    for (int i = 0; i < n && i < 8; ++i) player_apply_potion_effect(eff[i].id, eff[i].duration, eff[i].amplifier, splash_scale);
    g_save_dirty = 1;
}

static void player_potion_update_tick(void) {
    for (int i = 1; i < PEX_POTION_MAX; ++i) {
        PexPotionEffectState *e = &g_player_potion_effects[i];
        if (e->duration <= 0) continue;
        if (i == PEX_POTION_REGENERATION) {
            int interval = 25 >> e->amplifier;
            if (interval < 1) interval = 1;
            if ((e->duration % interval) == 0 && g_player_health < 20) player_health_set_hearts(g_player_health + 1);
        } else if (i == PEX_POTION_POISON) {
            int interval = 25 >> e->amplifier;
            if (interval < 1) interval = 1;
            if ((e->duration % interval) == 0 && g_player_health > 1) player_take_damage(1, "poison");
        } else if (i == PEX_POTION_HUNGER) {
            player_add_exhaustion(0.025f * (float)(e->amplifier + 1));
        }
        e->duration--;
        if (e->duration <= 0) { e->duration = 0; e->amplifier = 0; }
    }
}

static int pex_map_top_block_color_index(int id) {
    switch (id) {
        case 0: return 0;
        case BLOCK_GRASS: return 1;
        case BLOCK_SAND: case BLOCK_SANDSTONE: case BLOCK_GLOWSTONE: return 2;
        case BLOCK_WOOL: return 3;
        case BLOCK_FIRE: case BLOCK_TNT: case BLOCK_REDSTONE_LAMP_ON: return 4;
        case BLOCK_ICE: return 5;
        case BLOCK_IRON_BLOCK: case BLOCK_IRON_ORE: case BLOCK_STONE: case BLOCK_COBBLESTONE: case BLOCK_STONE_BRICK: return 6;
        case BLOCK_LEAVES: case BLOCK_TALL_GRASS: case BLOCK_VINE: case BLOCK_CACTUS: return 7;
        case BLOCK_SNOW_BLOCK: case BLOCK_SNOW_LAYER: return 8;
        case BLOCK_CLAY: return 9;
        case BLOCK_DIRT: case BLOCK_FARMLAND: return 10;
        case BLOCK_WATER: case BLOCK_STILL_WATER: return 12;
        case BLOCK_LOG: case BLOCK_PLANKS: case BLOCK_CHEST: case BLOCK_CRAFTING_TABLE: case BLOCK_FENCE: case BLOCK_FENCE_GATE: return 13;
                case BLOCK_LAPIS_BLOCK: case BLOCK_LAPIS_ORE: return 23;
        case BLOCK_BRICK: case BLOCK_NETHERRACK: case BLOCK_NETHER_BRICK: return 28;
        case BLOCK_GOLD_BLOCK: case BLOCK_GOLD_ORE: return 30;
        case BLOCK_DIAMOND_BLOCK: case BLOCK_DIAMOND_ORE: return 24;
        default:
            if (block_is_water(id)) return 12;
            if (id == BLOCK_LAVA || id == BLOCK_STILL_LAVA) return 4;
            return 6;
    }
}

static PexMapData *pex_map_find(int id, int create) {
    int free_i = -1;
    for (int i = 0; i < MAX_MAP_DATA; ++i) {
        if (g_map_data[i].active && g_map_data[i].id == id) return &g_map_data[i];
        if (!g_map_data[i].active && free_i < 0) free_i = i;
    }
    if (!create || free_i < 0) return NULL;
    PexMapData *m = &g_map_data[free_i];
    memset(m, 0, sizeof(*m));
    m->active = 1;
    m->id = id;
    m->x_center = (int)floorf(g_player_x);
    m->z_center = (int)floorf(g_player_z);
    m->scale = 3;
    m->dimension = 0;
    if (id >= g_next_map_id) g_next_map_id = id + 1;
    return m;
}

static void pex_map_ensure_item(ItemStack *st) {
    if (!st || stack_empty(st) || st->id != ITEM_MAP) return;
    if (st->damage < 0) st->damage = g_next_map_id++;
    if (!pex_map_find(st->damage, 0)) {
        pex_map_find(st->damage, 1);
        g_save_dirty = 1;
    }
}

static int pex_map_column_color(PexMapData *m, int mx, int mz, double *last_height) {
    int scale = 1 << (m->scale & 7);
    int wx0 = (m->x_center / scale + mx - 64) * scale;
    int wz0 = (m->z_center / scale + mz - 64) * scale;
    int counts[32]; memset(counts, 0, sizeof(counts));
    double avg_h = 0.0;
    int liquid_depth = 0;
    for (int dx = 0; dx < scale; ++dx) for (int dz = 0; dz < scale; ++dz) {
        int wx = wx0 + dx, wz = wz0 + dz;
        int top_y = FLAT_WORLD_Y_MIN;
        int top_id = 0;
        for (int y = FLAT_WORLD_Y_MAX; y >= FLAT_WORLD_Y_MIN; --y) {
            int id = flat_get_block(wx, y, wz);
            if (id != 0) { top_y = y; top_id = id; break; }
        }
        if (block_is_water(top_id)) {
            int y = top_y - 1;
            while (y >= FLAT_WORLD_Y_MIN && block_is_water(flat_get_block(wx, y, wz))) { liquid_depth++; y--; }
        }
        avg_h += (double)top_y / (double)(scale * scale);
        int c = pex_map_top_block_color_index(top_id);
        if (c < 0) c = 0;
        if (c > 31) c = 31;
        counts[c]++;
    }
    int best = 0, bestn = -1;
    for (int i = 0; i < 32; ++i) if (counts[i] > bestn) { bestn = counts[i]; best = i; }
    double slope = (avg_h - *last_height) * 4.0 / (double)(scale + 4) + (((mx + mz) & 1) ? 0.2 : -0.2);
    int shade = 1;
    if (slope > 0.6) shade = 2;
    if (slope < -0.6) shade = 0;
    if (best == 12) {
        double water = (double)liquid_depth * 0.1 + (double)((mx + mz) & 1) * 0.2;
        shade = 1;
        if (water < 0.5) shade = 2;
        if (water > 0.9) shade = 0;
    }
    *last_height = avg_h;
    return best * 4 + shade;
}

static void pex_map_update_data(PexMapData *m) {
    if (!m || !m->active || m->dimension != 0) return;
    int scale = 1 << (m->scale & 7);
    int px = ((int)floorf(g_player_x) - m->x_center) / scale + 64;
    int pz = ((int)floorf(g_player_z) - m->z_center) / scale + 64;
    int radius = 128 / scale;
    if (radius < 1) radius = 1;
    m->tick++;
    int mx = (m->tick & 15);
    for (; mx < 128; mx += 16) {
        if (mx < px - radius || mx > px + radius) continue;
        double last_h = 0.0;
        for (int mz = pz - radius - 1; mz < pz + radius; ++mz) {
            if (mz < 0 || mz >= 128) continue;
            int dx = mx - px, dz = mz - pz;
            if (dx * dx + dz * dz >= radius * radius) continue;
            unsigned char color = (unsigned char)pex_map_column_color(m, mx, mz, &last_h);
            m->colors[mx + mz * 128] = color;
        }
    }
}

static void update_held_map_item_tick(void) {
    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    if (stack_empty(held) || held->id != ITEM_MAP) return;
    pex_map_ensure_item(held);
    PexMapData *m = pex_map_find(held->damage, 1);
    pex_map_update_data(m);
}

static int try_use_map_item(ItemStack *held) {
    if (!held || stack_empty(held) || held->id != ITEM_MAP) return 0;
    pex_map_ensure_item(held);
    restart_hand_swing();
    return 1;
}

static int pex_passive_health_cap_for_type(int type) {
    switch (type) {
        case PASSIVE_MOB_PIG: return 10;
        case PASSIVE_MOB_SHEEP: return 8;
        case PASSIVE_MOB_COW: return 10;
        case PASSIVE_MOB_CHICKEN: return 4;
        case PASSIVE_MOB_CREEPER: return 20;
        case PASSIVE_MOB_SKELETON: return 20;
        case PASSIVE_MOB_SPIDER: return 16;
        case PASSIVE_MOB_GIANT: return 100;
        case PASSIVE_MOB_ZOMBIE: return 20;
        case PASSIVE_MOB_SLIME: return 16;
        case PASSIVE_MOB_GHAST: return 10;
        case PASSIVE_MOB_PIG_ZOMBIE: return 20;
        case PASSIVE_MOB_ENDERMAN: return 40;
        case PASSIVE_MOB_CAVE_SPIDER: return 12;
        case PASSIVE_MOB_SILVERFISH: return 8;
        case PASSIVE_MOB_BLAZE: return 20;
        case PASSIVE_MOB_MAGMA_CUBE: return 16;
        case PASSIVE_MOB_ENDER_DRAGON: return 200;
        case PASSIVE_MOB_SQUID: return 10;
        case PASSIVE_MOB_WOLF: return 8;
        case PASSIVE_MOB_MOOSHROOM: return 10;
        case PASSIVE_MOB_SNOWMAN: return 4;
        case PASSIVE_MOB_OCELOT: return 10;
        case PASSIVE_MOB_IRON_GOLEM: return 100;
        case PASSIVE_MOB_VILLAGER: return 20;
        default: return 1;
    }
}

static int pex_passive_has_potion(const PassiveMob *m, int potion_id) {
    return m && potion_id > 0 && potion_id < 32 && m->potion_duration[potion_id] > 0;
}

static int pex_passive_potion_amplifier(const PassiveMob *m, int potion_id) {
    if (!pex_passive_has_potion(m, potion_id)) return 0;
    return m->potion_amplifier[potion_id];
}

static void pex_passive_start_death_direct(PassiveMob *m) {
    if (!m || m->death_time > 0) return;
    const char *s = NULL;
    switch (m->type) {
        case PASSIVE_MOB_PIG: s = "mob.pigdeath"; break;
        case PASSIVE_MOB_COW: s = "mob.cowhurt"; break;
        case PASSIVE_MOB_SHEEP: s = "mob.sheep"; break;
        case PASSIVE_MOB_CHICKEN: s = "mob.chickenhurt"; break;
        case PASSIVE_MOB_CREEPER: s = "mob.creeperdeath"; break;
        case PASSIVE_MOB_SKELETON: s = "mob.skeletonhurt"; break;
        case PASSIVE_MOB_ZOMBIE: case PASSIVE_MOB_GIANT: s = "mob.zombiehurt"; break;
        case PASSIVE_MOB_SPIDER: case PASSIVE_MOB_CAVE_SPIDER: s = "mob.spiderdeath"; break;
        case PASSIVE_MOB_SLIME: case PASSIVE_MOB_MAGMA_CUBE: s = "mob.slime"; break;
        case PASSIVE_MOB_GHAST: s = "mob.ghast.death"; break;
        case PASSIVE_MOB_PIG_ZOMBIE: s = "mob.zombiepig.zpigdeath"; break;
        case PASSIVE_MOB_ENDERMAN: s = "mob.endermen.death"; break;
        case PASSIVE_MOB_SILVERFISH: s = "mob.silverfish.kill"; break;
        case PASSIVE_MOB_BLAZE: s = "mob.blaze.death"; break;
        case PASSIVE_MOB_SQUID: s = "mob.squid.death"; break;
        case PASSIVE_MOB_WOLF: s = "mob.wolf.death"; break;
        case PASSIVE_MOB_SNOWMAN: s = "mob.snowman"; break;
        case PASSIVE_MOB_OCELOT: s = "mob.cat.hitt"; break;
        case PASSIVE_MOB_IRON_GOLEM: s = "mob.irongolem.death"; break;
        case PASSIVE_MOB_VILLAGER: s = "mob.villager.death"; break;
        default: break;
    }
    if (s) pex_sound_play_at(s, m->x, m->y, m->z, 1.0f, 1.0f);
    m->health = 0;
    m->death_time = 1;
    m->has_path_target = 0;
}

static void pex_passive_apply_potion_effect(PassiveMob *m, int id, int duration, int amplifier, float splash_scale) {
    if (!m || !m->active || m->death_time > 0 || id <= 0 || id >= 32) return;
    if (splash_scale < 0.0f) splash_scale = 0.0f;
    if (splash_scale > 1.0f) splash_scale = 1.0f;
    if (id == PEX_POTION_HEAL || id == PEX_POTION_HARM) {
        int amt = (int)((float)(6 << amplifier) * splash_scale + 0.5f);
        if (amt < 1) amt = 1;
        if (id == PEX_POTION_HEAL) {
            int cap = pex_passive_health_cap_for_type(m->type);
            m->health += amt;
            if (m->health > cap) m->health = cap;
        } else {
            m->health -= amt;
            m->hurt_time = 10;
            m->damage_cooldown = 10;
            if (m->health <= 0) pex_passive_start_death_direct(m);
        }
        g_save_dirty = 1;
        return;
    }
    duration = (int)((float)duration * splash_scale + 0.5f);
    if (duration <= 20) return;
    if (m->potion_duration[id] <= 0 || amplifier > m->potion_amplifier[id] ||
        (amplifier == m->potion_amplifier[id] && duration > m->potion_duration[id])) {
        m->potion_duration[id] = duration;
        m->potion_amplifier[id] = amplifier;
        g_save_dirty = 1;
    }
}

static void pex_apply_potion_to_passive_mobs(float x, float y, float z, int damage) {
    PexPotionEffectDef eff[8];
    int n = pex_potion_get_effects(damage, eff, 8);
    if (n <= 0) return;
    for (int mi = 0; mi < MAX_PASSIVE_MOBS; ++mi) {
        PassiveMob *m = &g_passive_mobs[mi];
        if (!m->active || m->death_time > 0) continue;
        float dx = m->x - x;
        float dy = (m->y + m->height * 0.5f) - y;
        float dz = m->z - z;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);
        if (dist > 4.0f) continue;
        float scale = 1.0f - dist / 4.0f;
        if (scale <= 0.0f) continue;
        for (int i = 0; i < n && i < 8; ++i) {
            pex_passive_apply_potion_effect(m, eff[i].id, eff[i].duration, eff[i].amplifier, scale);
        }
    }
}

static void pex_spawn_xp_orb(float x, float y, float z, int value) {
    int slot = -1;
    if (value <= 0) value = 1;
    for (int i = 0; i < MAX_XP_ORBS; ++i) if (!g_xp_orbs[i].active) { slot = i; break; }
    if (slot < 0) slot = rand() % MAX_XP_ORBS;
    FlatXPOrb *o = &g_xp_orbs[slot];
    memset(o, 0, sizeof(*o));
    o->active = 1;
    o->value = value;
    o->x = o->prev_x = x;
    o->y = o->prev_y = y;
    o->z = o->prev_z = z;
    o->mx = (pex_rand_float01() - 0.5f) * 0.20f;
    o->my = 0.15f + pex_rand_float01() * 0.15f;
    o->mz = (pex_rand_float01() - 0.5f) * 0.20f;
    o->pickup_delay = 10;
}

static int pex_spawn_projectile(int type, int damage) {
    int slot = -1;
    for (int i = 0; i < MAX_PROJECTILE_ENTITIES; ++i) if (!g_projectiles[i].active) { slot = i; break; }
    if (slot < 0) slot = rand() % MAX_PROJECTILE_ENTITIES;
    float lx, ly, lz;
    pex_touch_aware_look_vector(&lx, &ly, &lz);
    float len = sqrtf(lx * lx + ly * ly + lz * lz);
    if (len < 0.0001f) { lx = 0.0f; ly = 0.0f; lz = 1.0f; len = 1.0f; }
    lx /= len; ly /= len; lz /= len;

    FlatProjectile *p = &g_projectiles[slot];
    memset(p, 0, sizeof(*p));
    p->active = 1;
    p->type = type;
    p->item_damage = damage;
    p->owner_type = 0;
    p->owner_mob_type = 0;
    p->damage = (type == FLAT_PROJECTILE_ARROW) ? 4 : 0;
    p->x = p->prev_x = g_player_x + lx * 0.35f;
    p->y = p->prev_y = g_player_y - 0.10f + ly * 0.35f;
    p->z = p->prev_z = g_player_z + lz * 0.35f;
    float speed = (type == FLAT_PROJECTILE_POTION) ? 0.50f : (type == FLAT_PROJECTILE_ARROW ? 1.50f : 0.70f);
    p->mx = lx * speed + g_player_motion_x;
    p->my = ly * speed + g_player_motion_y * 0.25f;
    p->mz = lz * speed + g_player_motion_z;
    p->yaw = g_player_yaw;
    p->pitch = g_player_pitch;
    return 1;
}


static int pex_spawn_projectile_from_entity(int type, int owner_mob_type, float sx, float sy, float sz,
                                            float tx, float ty, float tz, float speed, int damage) {
    int slot = -1;
    for (int i = 0; i < MAX_PROJECTILE_ENTITIES; ++i) if (!g_projectiles[i].active) { slot = i; break; }
    if (slot < 0) slot = rand() % MAX_PROJECTILE_ENTITIES;
    float lx = tx - sx, ly = ty - sy, lz = tz - sz;
    float len = sqrtf(lx * lx + ly * ly + lz * lz);
    if (len < 0.0001f) { lx = 0.0f; ly = 0.0f; lz = 1.0f; len = 1.0f; }
    lx /= len; ly /= len; lz /= len;
    FlatProjectile *p = &g_projectiles[slot];
    memset(p, 0, sizeof(*p));
    p->active = 1;
    p->type = type;
    p->owner_type = 1;
    p->owner_mob_type = owner_mob_type;
    p->damage = damage;
    p->x = p->prev_x = sx;
    p->y = p->prev_y = sy;
    p->z = p->prev_z = sz;
    p->mx = lx * speed;
    p->my = ly * speed;
    p->mz = lz * speed;
    p->yaw = atan2f(lx, lz) * 57.29578f;
    float clamped_ly = ly;
    if (clamped_ly < -1.0f) clamped_ly = -1.0f;
    if (clamped_ly > 1.0f) clamped_ly = 1.0f;
    p->pitch = -asinf(clamped_ly) * 57.29578f;
    return 1;
}

static int pex_projectile_block_collision(float x, float y, float z) {
    int bx = (int)floorf(x);
    int by = (int)floorf(y);
    int bz = (int)floorf(z);
    int id = flat_get_block(bx, by, bz);
    if (id == 0) return 0;
    if (block_is_liquid(id)) return 0;
    if (id == BLOCK_TALL_GRASS || id == BLOCK_VINE || id == BLOCK_REDSTONE_WIRE || id == BLOCK_SNOW_LAYER) return 0;
    return flat_block_is_solid(id);
}

static void pex_projectile_break_effect(float x, float y, float z, int potion_damage) {
    int color = pex_potion_color_from_damage(potion_damage);
    float r = (float)((color >> 16) & 255) / 255.0f;
    float g = (float)((color >> 8) & 255) / 255.0f;
    float b = (float)(color & 255) / 255.0f;
    (void)r; (void)g; (void)b;
    for (int i = 0; i < 16; ++i) {
        float mx = (pex_rand_float01() - 0.5f) * 0.20f;
        float my = pex_rand_float01() * 0.20f;
        float mz = (pex_rand_float01() - 0.5f) * 0.20f;
        add_splash_particle(x, y, z, mx, my, mz);
    }
}

static int pex_xp_orb_split_value(int value) {
    if (value >= 2477) return 2477;
    if (value >= 1237) return 1237;
    if (value >= 617) return 617;
    if (value >= 307) return 307;
    if (value >= 149) return 149;
    if (value >= 73) return 73;
    if (value >= 37) return 37;
    if (value >= 17) return 17;
    if (value >= 7) return 7;
    if (value >= 3) return 3;
    return 1;
}

static void pex_projectile_impact(FlatProjectile *p) {
    if (!p || !p->active) return;
    if (p->type == FLAT_PROJECTILE_POTION) {
        pex_sound_play_at("random.glass", p->x, p->y, p->z, 1.0f, 1.0f);
        pex_projectile_break_effect(p->x, p->y, p->z, p->item_damage);
        float dx = g_player_x - p->x;
        float dy = g_player_y - p->y;
        float dz = g_player_z - p->z;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);
        if (dist <= 4.0f) player_apply_potion_damage(p->item_damage, 1.0f - dist / 4.0f);
        pex_apply_potion_to_passive_mobs(p->x, p->y, p->z, p->item_damage);
    } else if (p->type == FLAT_PROJECTILE_XP_BOTTLE) {
        pex_sound_play_at("random.glass", p->x, p->y, p->z, 1.0f, 1.0f);
        for (int i = 0; i < 8; ++i) add_splash_particle(p->x, p->y, p->z, (pex_rand_float01()-0.5f)*0.25f, pex_rand_float01()*0.25f, (pex_rand_float01()-0.5f)*0.25f);
        int total = 3 + rand() % 5 + rand() % 5;
        while (total > 0) {
            int v = pex_xp_orb_split_value(total);
            pex_spawn_xp_orb(p->x, p->y, p->z, v);
            total -= v;
        }
    } else if (p->type == FLAT_PROJECTILE_LARGE_FIREBALL || p->type == FLAT_PROJECTILE_SMALL_FIREBALL) {
        pex_sound_play_at("random.explode", p->x, p->y, p->z, p->type == FLAT_PROJECTILE_LARGE_FIREBALL ? 4.0f : 1.0f, 1.0f);
        for (int i = 0; i < 24; ++i) add_splash_particle(p->x, p->y, p->z, (pex_rand_float01()-0.5f)*0.25f, pex_rand_float01()*0.25f, (pex_rand_float01()-0.5f)*0.25f);
    } else if (p->type == FLAT_PROJECTILE_ARROW) {
        pex_sound_play_at("random.bowhit", p->x, p->y, p->z, 1.0f, 1.2f);
    } else if (p->type == FLAT_PROJECTILE_SNOWBALL) {
        for (int i = 0; i < 8; ++i) add_splash_particle(p->x, p->y, p->z, (pex_rand_float01()-0.5f)*0.15f, pex_rand_float01()*0.15f, (pex_rand_float01()-0.5f)*0.15f);
    }
    p->active = 0;
}

static int pex_projectile_passive_hit(float x0, float y0, float z0, float x1, float y1, float z1, float *out_t, float *out_x, float *out_y, float *out_z, int *out_mob_index) {
    float dx = x1 - x0, dy = y1 - y0, dz = z1 - z0;
    float best_t = 2.0f;
    int found = 0;
    int best_idx = -1;
    for (int mi = 0; mi < MAX_PASSIVE_MOBS; ++mi) {
        PassiveMob *m = &g_passive_mobs[mi];
        if (!m->active || m->death_time > 0) continue;
        float expand = 0.30f;
        float minx = m->x - m->width * 0.5f - expand;
        float maxx = m->x + m->width * 0.5f + expand;
        float miny = m->y - expand;
        float maxy = m->y + m->height + expand;
        float minz = m->z - m->width * 0.5f - expand;
        float maxz = m->z + m->width * 0.5f + expand;
        float tmin = 0.0f, tmax = 1.0f;
#define PEX_PROJ_AXIS(o,d,minv,maxv) do { \
    if (fabsf(d) < 1e-6f) { if ((o) < (minv) || (o) > (maxv)) { tmin = 2.0f; break; } } \
    else { float inv = 1.0f / (d); float ta = ((minv) - (o)) * inv; float tb = ((maxv) - (o)) * inv; \
           if (ta > tb) { float tmp = ta; ta = tb; tb = tmp; } \
           if (ta > tmin) tmin = ta; if (tb < tmax) tmax = tb; if (tmin > tmax) { tmin = 2.0f; break; } } \
} while (0)
        PEX_PROJ_AXIS(x0, dx, minx, maxx);
        if (tmin <= 1.0f) PEX_PROJ_AXIS(y0, dy, miny, maxy);
        if (tmin <= 1.0f) PEX_PROJ_AXIS(z0, dz, minz, maxz);
#undef PEX_PROJ_AXIS
        if (tmin >= 0.0f && tmin <= 1.0f && tmin < best_t) { best_t = tmin; found = 1; best_idx = mi; }
    }
    if (!found) return 0;
    if (out_t) *out_t = best_t;
    if (out_x) *out_x = x0 + dx * best_t;
    if (out_y) *out_y = y0 + dy * best_t;
    if (out_z) *out_z = z0 + dz * best_t;
    if (out_mob_index) *out_mob_index = best_idx;
    return 1;
}

static void pex_projectile_damage_passive_mob(int mob_index, const FlatProjectile *p, float src_x, float src_z) {
    if (!p || mob_index < 0 || mob_index >= MAX_PASSIVE_MOBS) return;
    PassiveMob *m = &g_passive_mobs[mob_index];
    if (!m->active || m->death_time > 0) return;
    int damage = p->damage > 0 ? p->damage : (p->type == FLAT_PROJECTILE_ARROW ? 4 : 0);
    if (damage <= 0 && p->type == FLAT_PROJECTILE_SNOWBALL && m->type == PASSIVE_MOB_BLAZE) damage = 3;
    if (damage <= 0) return;
    m->health -= damage;
    m->hurt_time = 10;
    m->damage_cooldown = 10;
    float dx = m->x - src_x, dz = m->z - src_z;
    float len = sqrtf(dx * dx + dz * dz); if (len < 0.001f) len = 0.001f;
    m->mx += dx / len * 0.35f;
    m->mz += dz / len * 0.35f;
    m->my += 0.22f;
    if (m->health <= 0) pex_passive_start_death_direct(m);
    g_save_dirty = 1;
}

static int pex_projectile_player_hit(const FlatProjectile *p, float x0, float y0, float z0, float x1, float y1, float z1) {
    if (!p || p->owner_type != 1 || p->age < 2 || g_player_dead) return 0;
    float minx = g_player_x - 0.30f, maxx = g_player_x + 0.30f;
    float miny = g_player_y - 1.62f, maxy = g_player_y + 0.18f;
    float minz = g_player_z - 0.30f, maxz = g_player_z + 0.30f;
    float dx = x1 - x0, dy = y1 - y0, dz = z1 - z0;
    float tmin = 0.0f, tmax = 1.0f;
#define PEX_PLAYER_PROJ_AXIS(o,d,minv,maxv) do { \
    if (fabsf(d) < 1e-6f) { if ((o) < (minv) || (o) > (maxv)) return 0; } \
    else { float inv = 1.0f / (d); float ta = ((minv) - (o)) * inv; float tb = ((maxv) - (o)) * inv; \
           if (ta > tb) { float tmp = ta; ta = tb; tb = tmp; } \
           if (ta > tmin) tmin = ta; if (tb < tmax) tmax = tb; if (tmin > tmax) return 0; } \
} while (0)
    PEX_PLAYER_PROJ_AXIS(x0, dx, minx, maxx);
    PEX_PLAYER_PROJ_AXIS(y0, dy, miny, maxy);
    PEX_PLAYER_PROJ_AXIS(z0, dz, minz, maxz);
#undef PEX_PLAYER_PROJ_AXIS
    return tmin >= 0.0f && tmin <= 1.0f;
}

static int pex_difficulty_scaled_mob_damage(int base) {
    int d = g_opts.difficulty & 3;
    if (d == 0) return 0;
    if (d == 1) return (base + 1) / 2;
    if (d == 3) return (base * 3 + 1) / 2;
    return base;
}

static void update_projectiles(void) {
    for (int i = 0; i < MAX_PROJECTILE_ENTITIES; ++i) {
        FlatProjectile *p = &g_projectiles[i];
        if (!p->active) continue;
        p->prev_x = p->x; p->prev_y = p->y; p->prev_z = p->z;
        p->age++;
        float nx = p->x + p->mx;
        float ny = p->y + p->my;
        float nz = p->z + p->mz;
        int hit = 0;
        float hit_t = 2.0f, hx = 0.0f, hy = 0.0f, hz = 0.0f;
        float ehx = 0.0f, ehy = 0.0f, ehz = 0.0f, et = 2.0f;
        int passive_hit_idx = -1;
        if (pex_projectile_player_hit(p, p->x, p->y, p->z, nx, ny, nz)) {
            int dmg = pex_difficulty_scaled_mob_damage(p->damage > 0 ? p->damage : 2);
            if (dmg > 0) player_take_damage(dmg, p->type == FLAT_PROJECTILE_ARROW ? "was shot by Skeleton" : "was hit by projectile");
            p->x = nx; p->y = ny; p->z = nz; pex_projectile_impact(p); continue;
        }
        if ((p->owner_type == 0 || p->owner_type == 1) && p->age > 2 && pex_projectile_passive_hit(p->x, p->y, p->z, nx, ny, nz, &et, &ehx, &ehy, &ehz, &passive_hit_idx)) {
            hit = 1; hit_t = et; hx = ehx; hy = ehy; hz = ehz;
        }
        for (int step = 1; step <= 8; ++step) {
            float t = (float)step / 8.0f;
            float sx = p->x + (nx - p->x) * t;
            float sy = p->y + (ny - p->y) * t;
            float sz = p->z + (nz - p->z) * t;
            if (pex_projectile_block_collision(sx, sy, sz)) {
                if (!hit || t < hit_t) { hit = 1; hit_t = t; hx = sx; hy = sy; hz = sz; passive_hit_idx = -1; }
                break;
            }
        }
        if (hit) { if (passive_hit_idx >= 0) pex_projectile_damage_passive_mob(passive_hit_idx, p, p->x, p->z); p->x = hx; p->y = hy; p->z = hz; pex_projectile_impact(p); continue; }
        p->x = nx; p->y = ny; p->z = nz;
        int in_water = block_is_water(flat_get_block((int)floorf(p->x), (int)floorf(p->y), (int)floorf(p->z)));
        if (in_water) {
            p->mx *= 0.80f; p->my *= 0.80f; p->mz *= 0.80f;
            if ((p->age & 3) == 0) add_splash_particle(p->x, p->y, p->z, 0.0f, 0.02f, 0.0f);
        } else {
            p->mx *= 0.99f; p->my *= 0.99f; p->mz *= 0.99f;
        }
        if (p->type == FLAT_PROJECTILE_SMALL_FIREBALL || p->type == FLAT_PROJECTILE_LARGE_FIREBALL) {
            /* fireballs fly mostly straight in 1.2.5 */
        } else {
            p->my -= (p->type == FLAT_PROJECTILE_XP_BOTTLE) ? 0.07f : 0.05f;
        }
        if (p->y < -32.0f || p->age > 1200) p->active = 0;
    }
}

static void update_xp_orbs(void) {
    for (int i = 0; i < MAX_XP_ORBS; ++i) {
        FlatXPOrb *o = &g_xp_orbs[i];
        if (!o->active) continue;
        o->prev_x = o->x; o->prev_y = o->y; o->prev_z = o->z;
        o->age++;
        if (o->pickup_delay > 0) o->pickup_delay--;
        float dx = g_player_x - o->x;
        float dy = (g_player_y - 1.0f) - o->y;
        float dz = g_player_z - o->z;
        float d2 = dx*dx + dy*dy + dz*dz;
        if (d2 < 64.0f) {
            float d = sqrtf(d2);
            if (d < 0.001f) d = 0.001f;
            float pull = (1.0f - d / 8.0f);
            if (pull > 0.0f) {
                pull *= pull * 0.10f;
                o->mx += dx / d * pull;
                o->my += dy / d * pull;
                o->mz += dz / d * pull;
            }
        }
        o->my -= 0.03f;
        o->x += o->mx; o->y += o->my; o->z += o->mz;
        int bx = (int)floorf(o->x), by = (int)floorf(o->y), bz = (int)floorf(o->z);
        if (flat_block_is_solid(flat_get_block(bx, by, bz))) {
            o->y = (float)by + 1.01f;
            o->my *= -0.25f;
            o->mx *= 0.70f;
            o->mz *= 0.70f;
        } else {
            o->mx *= 0.98f; o->my *= 0.98f; o->mz *= 0.98f;
        }
        if (o->pickup_delay <= 0 && d2 < 1.2f) {
            player_add_experience(o->value);
            pex_sound_play("random.orb", 0.2f, 0.5f + pex_rand_float01() * 0.4f);
            o->active = 0;
            g_save_dirty = 1;
        }
        if (o->age > 6000 || o->y < -32.0f) o->active = 0;
    }
}

static int try_use_xp_bottle_item(ItemStack *held) {
    if (!held || stack_empty(held) || held->id != ITEM_EXP_BOTTLE) return 0;
    if (!pex_spawn_projectile(FLAT_PROJECTILE_XP_BOTTLE, 0)) return 0;
    pex_sound_play("random.bow", 0.5f, 0.4f / (pex_rand_float01() * 0.4f + 0.8f));
    consume_held_stack_one(held, 0);
    restart_hand_swing();
    return 1;
}

static int try_use_potion_item(ItemStack *held) {
    if (!held || stack_empty(held) || held->id != ITEM_POTION) return 0;
    int dmg = held->damage;
    PexPotionEffectDef eff[8];
    int n = pex_potion_get_effects(dmg, eff, 8);
    if ((dmg & 16384) != 0) {
        (void)eff; (void)n;
        if (!pex_spawn_projectile(FLAT_PROJECTILE_POTION, dmg)) return 0;
        pex_sound_play("random.bow", 0.5f, 0.4f / (pex_rand_float01() * 0.4f + 0.8f));
        consume_held_stack_one(held, 0);
    } else {
        pex_sound_play("random.drink", 0.5f, 1.0f);
        player_apply_potion_damage(dmg, 1.0f);
        consume_held_stack_one(held, ITEM_GLASS_BOTTLE);
    }
    g_save_dirty = 1;
    restart_hand_swing();
    return 1;
}

static int bone_meal_grow_tree_at(int x, int y, int z) {
    int below = flat_get_block(x, y - 1, z);
    if (below != BLOCK_GRASS && below != BLOCK_DIRT) return 0;
    if (flat_get_block(x, y, z) != BLOCK_SAPLING) return 0;
    int meta = flat_get_meta(x, y, z) & 3;
    flat_begin_persistent_edit();
    flat_set_block(x, y, z, 0);
    normal_place_tree(x, z, y - 1);
    if (flat_get_block(x, y, z) == 0) {
        flat_set_block(x, y, z, BLOCK_SAPLING);
        flat_set_meta_raw(x, y, z, meta);
        flat_end_persistent_edit();
        return 0;
    }
    flat_end_persistent_edit();
    unsupported_block_neighbor_cleanup(x, y, z);
    return 1;
}

static int bone_meal_on_grass_at(int x, int y, int z) {
    if (flat_get_block(x, y, z) != BLOCK_GRASS) return 0;
    flat_begin_persistent_edit();
    for (int i = 0; i < 128; ++i) {
        int tx = x;
        int ty = y + 1;
        int tz = z;
        int ok = 1;
        for (int j = 0; j < i / 16; ++j) {
            tx += (rand() % 3) - 1;
            ty += ((rand() % 3) - 1) * (rand() % 3) / 2;
            tz += (rand() % 3) - 1;
            if (flat_get_block(tx, ty - 1, tz) != BLOCK_GRASS || flat_block_is_solid(flat_get_block(tx, ty, tz))) { ok = 0; break; }
        }
        if (!ok || !flat_in_bounds(tx, ty, tz) || flat_get_block(tx, ty, tz) != 0) continue;
        if ((rand() % 10) != 0) { flat_set_block(tx, ty, tz, BLOCK_TALL_GRASS); flat_set_meta_raw(tx, ty, tz, 1); }
        else if ((rand() % 3) != 0) flat_set_block(tx, ty, tz, BLOCK_YELLOW_FLOWER);
        else flat_set_block(tx, ty, tz, BLOCK_RED_ROSE);
    }
    flat_end_persistent_edit();
    return 1;
}

static int try_use_dye_item(ItemStack *held, const FlatRayHit *hit, int target_id) {
    if (!held || stack_empty(held) || held->id != ITEM_DYE_POWDER || !hit || !hit->hit) return 0;
    int dmg = held->damage & 15;

    if (passive_mobs_apply_dye_to_target(dmg)) {
        consume_held_stack_one(held, 0);
        restart_hand_swing();
        g_save_dirty = 1;
        return 1;
    }

    if (dmg != 15) return 0;
    if (target_id == BLOCK_SAPLING) {
        if (!bone_meal_grow_tree_at(hit->bx, hit->by, hit->bz)) return 0;
    } else if (target_id == BLOCK_CROPS || target_id == BLOCK_PUMPKIN_STEM || target_id == BLOCK_MELON_STEM) {
        flat_begin_persistent_edit();
        flat_set_meta_raw(hit->bx, hit->by, hit->bz, 7);
        flat_end_persistent_edit();
    } else if (target_id == BLOCK_GRASS) {
        if (!bone_meal_on_grass_at(hit->bx, hit->by, hit->bz)) return 0;
    } else {
        return 0;
    }
    consume_held_stack_one(held, 0);
    pex_sound_play_at("step.grass", (float)hit->bx + 0.5f, (float)hit->by + 0.5f, (float)hit->bz + 0.5f, 1.0f, 1.0f);
    if (!g_mp_connected) g_save_dirty = 1;
    restart_hand_swing();
    return 1;
}

static int try_use_spawn_egg(ItemStack *held, const FlatRayHit *hit, int target_id) {
    if (!held || stack_empty(held) || held->id != ITEM_MONSTER_PLACER || !hit || !hit->hit) return 0;
    int px, py, pz;
    float sy;
    hit_adjacent_cell(hit, &px, &py, &pz);
    sy = (float)py;
    if (hit->face == 1 && (target_id == BLOCK_FENCE || target_id == BLOCK_NETHER_BRICK_FENCE)) sy += 0.5f;
    if (!flat_in_bounds(px, py, pz)) return 0;
    if (!passive_mobs_spawn_from_egg_damage(held->damage, (float)px + 0.5f, sy, (float)pz + 0.5f)) return 0;
    consume_held_stack_one(held, 0);
    if (!g_mp_connected) g_save_dirty = 1;
    restart_hand_swing();
    return 1;
}


static int block_is_rail_id(int id) {
    return id == BLOCK_RAILS || id == BLOCK_POWERED_RAIL || id == BLOCK_DETECTOR_RAIL;
}

static int item_is_record_id(int id) {
    return id >= ITEM_RECORD13 && id <= ITEM_RECORD_11;
}

static const char *record_sound_key(int id) {
    switch (id) {
        case ITEM_RECORD13: return "records.13";
        case ITEM_RECORD_CAT: return "records.cat";
        case ITEM_RECORD_BLOCKS: return "records.blocks";
        case ITEM_RECORD_CHIRP: return "records.chirp";
        case ITEM_RECORD_FAR: return "records.far";
        case ITEM_RECORD_MALL: return "records.mall";
        case ITEM_RECORD_MELLOHI: return "records.mellohi";
        case ITEM_RECORD_STAL: return "records.stal";
        case ITEM_RECORD_STRAD: return "records.strad";
        case ITEM_RECORD_WARD: return "records.ward";
        case ITEM_RECORD_11: return "records.11";
        default: return NULL;
    }
}

static int spawn_flat_vehicle(int type, float x, float y, float z, float yaw) {
    int slot = -1;
    if (type <= 0) return 0;
    for (int i = 0; i < MAX_VEHICLE_ENTITIES; ++i) if (!g_vehicles[i].active) { slot = i; break; }
    if (slot < 0) slot = 0;
    FlatVehicle *v = &g_vehicles[slot];
    memset(v, 0, sizeof(*v));
    v->active = 1;
    v->type = type;
    v->x = v->prev_x = x;
    v->y = v->prev_y = y;
    v->z = v->prev_z = z;
    v->yaw = v->prev_yaw = yaw;
    v->age = 0;
    return 1;
}

static void update_vehicles(void) {
    for (int i = 0; i < MAX_VEHICLE_ENTITIES; ++i) {
        FlatVehicle *v = &g_vehicles[i];
        if (!v->active) continue;
        v->prev_x = v->x; v->prev_y = v->y; v->prev_z = v->z; v->prev_yaw = v->yaw;
        v->age++;
        if (v->type == FLAT_VEHICLE_BOAT) {
            int bx = (int)floorf(v->x), by = (int)floorf(v->y - 0.15f), bz = (int)floorf(v->z);
            int id = flat_get_block(bx, by, bz);
            if (block_is_water(id)) {
                float target_y = (float)by + 0.62f;
                v->my += (target_y - v->y) * 0.08f;
                v->my *= 0.80f;
            } else {
                v->my -= 0.04f;
                v->my *= 0.98f;
            }
            v->x += v->mx; v->y += v->my; v->z += v->mz;
            v->mx *= 0.96f; v->mz *= 0.96f;
            if (v->y < -32.0f) v->active = 0;
        } else {
            int bx = (int)floorf(v->x), by = (int)floorf(v->y - 0.35f), bz = (int)floorf(v->z);
            if (block_is_rail_id(flat_get_block(bx, by, bz))) {
                v->y = (float)by + 0.50f;
                v->my = 0.0f;
            } else {
                v->my -= 0.04f;
                v->y += v->my;
                if (v->y < -32.0f) v->active = 0;
            }
            v->x += v->mx; v->z += v->mz;
            v->mx *= 0.95f; v->mz *= 0.95f;
        }
    }
}

static JukeboxTile *jukebox_find_tile(int x, int y, int z, int create) {
    int free_i = -1;
    for (int i = 0; i < MAX_JUKEBOX_TILES; ++i) {
        JukeboxTile *jt = &g_jukebox_tiles[i];
        if (jt->active && jt->x == x && jt->y == y && jt->z == z) return jt;
        if (!jt->active && free_i < 0) free_i = i;
    }
    if (!create || free_i < 0) return NULL;
    JukeboxTile *jt = &g_jukebox_tiles[free_i];
    memset(jt, 0, sizeof(*jt));
    jt->active = 1;
    jt->x = x; jt->y = y; jt->z = z;
    return jt;
}

static int jukebox_record_at(int x, int y, int z) {
    JukeboxTile *jt = jukebox_find_tile(x, y, z, 0);
    return jt ? jt->record_item : 0;
}

static int jukebox_insert_record(int x, int y, int z, int record_item) {
    if (!item_is_record_id(record_item)) return 0;
    if (flat_get_block(x, y, z) != BLOCK_JUKEBOX) return 0;
    if (flat_get_meta(x, y, z) != 0 || jukebox_record_at(x, y, z) != 0) return 0;
    JukeboxTile *jt = jukebox_find_tile(x, y, z, 1);
    if (!jt) return 0;
    jt->record_item = record_item;
    flat_set_meta_raw(x, y, z, 1); /* Java BlockJukeBox stores 1 in block metadata; record id is in its tile entity. */
    const char *snd = record_sound_key(record_item);
    if (snd) pex_sound_play_at(snd, (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, 4.0f, 1.0f);
    g_save_dirty = 1;
    return 1;
}

static int jukebox_eject_record(int x, int y, int z) {
    if (flat_get_block(x, y, z) != BLOCK_JUKEBOX) return 0;
    JukeboxTile *jt = jukebox_find_tile(x, y, z, 0);
    int rec = jt ? jt->record_item : 0;
    if (!item_is_record_id(rec)) return 0;
    const char *snd = record_sound_key(rec);
    if (snd) pex_sound_play_at(snd, (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, 0.0f, 1.0f);
    spawn_item_stack((float)x + 0.5f, (float)y + 1.0f, (float)z + 0.5f, make_stack(rec, 1, 0), 0);
    if (jt) memset(jt, 0, sizeof(*jt));
    flat_set_meta_raw(x, y, z, 0);
    g_save_dirty = 1;
    return 1;
}

static int flat_raycast_boat_target(FlatRayHit *out) {
    FlatRayHit h; memset(&h, 0, sizeof(h)); h.face = 1;
    float dx, dy, dz;
    pex_touch_aware_look_vector(&dx, &dy, &dz);
    int prev_bx = (int)floorf(g_player_x), prev_by = (int)floorf(g_player_y), prev_bz = (int)floorf(g_player_z);
    for (float t = 0.0f; t <= 5.0f; t += 0.05f) {
        float x = g_player_x + dx * t;
        float y = g_player_y + dy * t;
        float z = g_player_z + dz * t;
        int bx = (int)floorf(x), by = (int)floorf(y), bz = (int)floorf(z);
        int id = flat_get_block(bx, by, bz);
        if (id != 0) {
            h.hit = 1; h.bx = bx; h.by = by; h.bz = bz; h.hx = x; h.hy = y; h.hz = z;
            if (prev_by < by) h.face = 0;
            else if (prev_by > by) h.face = 1;
            else if (prev_bz < bz) h.face = 2;
            else if (prev_bz > bz) h.face = 3;
            else if (prev_bx < bx) h.face = 4;
            else if (prev_bx > bx) h.face = 5;
            if (out) *out = h;
            return 1;
        }
        prev_bx = bx; prev_by = by; prev_bz = bz;
    }
    return 0;
}

static int try_use_boat_item(ItemStack *held) {
    if (!held || stack_empty(held) || held->id != ITEM_BOAT) return 0;
    FlatRayHit hit;
    if (!flat_raycast_boat_target(&hit)) return 0;
    int by = hit.by;
    if (flat_get_block(hit.bx, hit.by, hit.bz) == BLOCK_SNOW_LAYER) by--;
    if (!spawn_flat_vehicle(FLAT_VEHICLE_BOAT, (float)hit.bx + 0.5f, (float)by + 1.0f, (float)hit.bz + 0.5f, g_player_yaw)) return 0;
    consume_held_stack_one(held, 0);
    if (!g_mp_connected) g_save_dirty = 1;
    restart_hand_swing();
    return 1;
}

static int minecart_vehicle_type_for_item(int id) {
    if (id == ITEM_MINECART_EMPTY || id == ITEM_MINECART) return FLAT_VEHICLE_MINECART_RIDEABLE;
    if (id == ITEM_MINECART_CRATE) return FLAT_VEHICLE_MINECART_CHEST;
    if (id == ITEM_MINECART_POWERED) return FLAT_VEHICLE_MINECART_FURNACE;
    return 0;
}

static int try_use_minecart_item(ItemStack *held, const FlatRayHit *hit, int target_id) {
    int vt = held ? minecart_vehicle_type_for_item(held->id) : 0;
    if (!vt || !hit || !hit->hit || !block_is_rail_id(target_id)) return 0;
    if (!spawn_flat_vehicle(vt, (float)hit->bx + 0.5f, (float)hit->by + 0.5f, (float)hit->bz + 0.5f, g_player_yaw)) return 0;
    consume_held_stack_one(held, 0);
    if (!g_mp_connected) g_save_dirty = 1;
    restart_hand_swing();
    return 1;
}

static int try_use_record_item(ItemStack *held, const FlatRayHit *hit, int target_id) {
    if (!held || stack_empty(held) || !item_is_record_id(held->id) || !hit || !hit->hit) return 0;
    if (target_id != BLOCK_JUKEBOX) return 0;
    if (!jukebox_insert_record(hit->bx, hit->by, hit->bz, held->id)) return 0;
    consume_held_stack_one(held, 0);
    restart_hand_swing();
    return 1;
}

static int try_use_nonblock_item(ItemStack *held, const FlatRayHit *hit, int target_id) {
    if (!held || stack_empty(held) || !hit || !hit->hit) return 0;

    int id = held->id;
    int px, py, pz;
    hit_adjacent_cell(hit, &px, &py, &pz);

    if (item_food_heal_amount(id) > 0) return try_eat_held_food(held);
    if (id == ITEM_POTION && try_use_potion_item(held)) return 1;
    if (id == ITEM_EXP_BOTTLE && try_use_xp_bottle_item(held)) return 1;
    if (id == ITEM_MAP && try_use_map_item(held)) return 1;
    if (id == ITEM_BOAT && try_use_boat_item(held)) return 1;
    if (minecart_vehicle_type_for_item(id) && try_use_minecart_item(held, hit, target_id)) return 1;
    if (item_is_record_id(id) && try_use_record_item(held, hit, target_id)) return 1;
    if (id == ITEM_MONSTER_PLACER && try_use_spawn_egg(held, hit, target_id)) return 1;
    if (id == ITEM_DYE_POWDER && try_use_dye_item(held, hit, target_id)) return 1;

    if (held_is_hoe_item(id)) {
        if (hit->face != 1) return 0;
        if ((target_id == BLOCK_GRASS || target_id == BLOCK_DIRT) && flat_get_block(hit->bx, hit->by + 1, hit->bz) == 0) {
            if (!g_mp_connected) flat_begin_persistent_edit();
            flat_set_block(hit->bx, hit->by, hit->bz, BLOCK_FARMLAND);
            if (!g_mp_connected) flat_end_persistent_edit();
            if (g_mp_connected) pex_net_send_block_action(PEX_BLOCK_PLACE, hit->bx, hit->by, hit->bz, hit->face, BLOCK_FARMLAND);
            else g_save_dirty = 1;
            pex_sound_play_at("step.gravel", (float)hit->bx + 0.5f, (float)hit->by + 0.5f, (float)hit->bz + 0.5f, 1.0f, 0.8f);
            restart_hand_swing();
            return 1;
        }
        return 0;
    }

    if (id == ITEM_FLINT_AND_IRON) {
        if (flat_in_bounds(px, py, pz) && flat_get_block(px, py, pz) == 0 && !flat_block_intersects_player(px, py, pz)) {
            if (!g_mp_connected) flat_begin_persistent_edit();
            flat_set_block(px, py, pz, BLOCK_FIRE);
            if (!g_mp_connected) flat_end_persistent_edit();
            if (g_mp_connected) pex_net_send_block_action(PEX_BLOCK_PLACE, px, py, pz, hit->face, BLOCK_FIRE);
            else g_save_dirty = 1;
            pex_sound_play_at("fire.ignite", (float)px + 0.5f, (float)py + 0.5f, (float)pz + 0.5f, 1.0f, 1.0f);
            restart_hand_swing();
            return 1;
        }
        return 0;
    }

    if (id == ITEM_BUCKET_EMPTY) {
        int sx, sy, sz;
        if (flat_raycast_liquid_source(&sx, &sy, &sz)) {
            int is_water = block_is_water(flat_get_block(sx, sy, sz));
            flat_begin_persistent_edit();
            flat_set_block(sx, sy, sz, 0);
            flat_end_persistent_edit();
            pex_sound_play_at(is_water ? "random.splash" : "random.fizz", (float)sx + 0.5f, (float)sy + 0.5f, (float)sz + 0.5f, 1.0f, 1.0f);
            consume_held_stack_one(held, is_water ? ITEM_BUCKET_WATER : ITEM_BUCKET_LAVA);
            if (g_mp_connected) {
                pex_net_send_block_action(PEX_BLOCK_BUCKET_PICKUP, sx, sy, sz, 0, is_water ? BLOCK_WATER : BLOCK_LAVA);
                pex_net_send_player_action(PEX_ACTION_SWING, sx, sy, sz, 0, is_water ? BLOCK_WATER : BLOCK_LAVA);
            } else {
                g_save_dirty = 1;
            }
            restart_hand_swing();
            return 1;
        }
        return 0;
    }

    if (id == ITEM_BUCKET_WATER || id == ITEM_BUCKET_LAVA) {
        if (!flat_in_bounds(px, py, pz)) return 0;
        int existing = flat_get_block(px, py, pz);
        if (existing == 0 || !flat_block_is_solid(existing) || block_is_liquid(existing)) {
            int fluid = id == ITEM_BUCKET_WATER ? BLOCK_WATER : BLOCK_LAVA;
            flat_begin_persistent_edit();
            flat_place_fluid_source(px, py, pz, fluid);
            flat_end_persistent_edit();
            pex_sound_play_at(fluid == BLOCK_WATER ? "random.splash" : "random.fizz", (float)px + 0.5f, (float)py + 0.5f, (float)pz + 0.5f, 1.0f, 1.0f);
            consume_held_stack_one(held, ITEM_BUCKET_EMPTY);
            if (g_mp_connected) {
                pex_net_send_block_action(PEX_BLOCK_BUCKET_PLACE, px, py, pz, hit->face, fluid);
                pex_net_send_player_action(PEX_ACTION_PLACE, px, py, pz, hit->face, fluid);
            } else {
                g_save_dirty = 1;
            }
            restart_hand_swing();
            return 1;
        }
        return 0;
    }

    return 0;
}

static void ingame_right_click(void) {
    if (g_screen != SCREEN_INGAME) return;
    if (g_right_click_delay_timer > 0) return;
    g_right_click_delay_timer = 4;

    FlatRayHit hit = flat_raycast();
    ItemStack *held = &g_inventory[g_selected_hotbar_slot];
    if (!hit.hit) {
        if (try_use_potion_item(held)) return;
        if (try_use_xp_bottle_item(held)) return;
        if (try_use_map_item(held)) return;
        if (try_use_boat_item(held)) return;
        try_eat_held_food(held);
        return;
    }

    int target_id = flat_get_block(hit.bx, hit.by, hit.bz);
    int sneaking = key_down_vk(g_opts.keys[5]);

    if (!sneaking) {
        if (target_id == BLOCK_CRAFTING_TABLE) {
            set_screen(SCREEN_WORKBENCH);
            return;
        }
        if (target_id == BLOCK_FURNACE || target_id == BLOCK_FURNACE_LIT) {
            furnace_open_at(hit.bx, hit.by, hit.bz);
            return;
        }
        if (target_id == BLOCK_CHEST) {
            chest_open_at(hit.bx, hit.by, hit.bz);
            return;
        }
        if (target_id == BLOCK_JUKEBOX && flat_get_meta(hit.bx, hit.by, hit.bz) != 0) {
            if (!g_mp_connected) flat_begin_persistent_edit();
            jukebox_eject_record(hit.bx, hit.by, hit.bz);
            if (!g_mp_connected) flat_end_persistent_edit();
            restart_hand_swing();
            return;
        }
        if (block_is_door_id(target_id)) {
            door_toggle_at(hit.bx, hit.by, hit.bz);
            return;
        }
        if (target_id == BLOCK_STONE_BUTTON) {
            press_button_at(hit.bx, hit.by, hit.bz);
            return;
        }
        if (target_id == BLOCK_LEVER) {
            toggle_lever_at(hit.bx, hit.by, hit.bz);
            return;
        }
    }

    if (stack_empty(held)) return;
    if (try_use_nonblock_item(held, &hit, target_id)) return;
    if (!item_is_placeable_block_id(held->id)) return;

    int px = hit.bx;
    int py = hit.by;
    int pz = hit.bz;

    /* Beta ItemBlock: clicking a snow layer replaces the layer instead of
       placing against a fake full cube. */
    if (target_id != BLOCK_SNOW_LAYER) {
        if (hit.face == 0) py--;
        else if (hit.face == 1) py++;
        else if (hit.face == 2) pz--;
        else if (hit.face == 3) pz++;
        else if (hit.face == 4) px--;
        else if (hit.face == 5) px++;
    }

    if (py > FLAT_WORLD_Y_MAX) {
        hud_add_chat("Height limit for building is 256 blocks.");
        return;
    }
    if (py < FLAT_WORLD_Y_MIN ||
        px < g_flat_world_origin_x || px >= g_flat_world_origin_x + FLAT_WORLD_SIZE ||
        pz < g_flat_world_origin_z || pz >= g_flat_world_origin_z + FLAT_WORLD_SIZE) return;

    if (held->id == ITEM_DOOR_WOOD || held->id == ITEM_DOOR_IRON) {
        if (!place_door_from_item(held->id, px, py, pz)) return;
        redstone_update_near(px, py, pz);
        if (g_mp_connected) {
            pex_net_send_player_action(PEX_ACTION_PLACE, px, py, pz, hit.face, held->id);
            pex_net_send_block_action(PEX_BLOCK_PLACE, px, py, pz, hit.face, held->id);
        }
        if (!player_is_creative() && --held->count <= 0) stack_clear(held);
        if (!g_mp_connected) g_save_dirty = 1;
        pex_sound_play_at(pex_block_step_sound_key(held->id == ITEM_DOOR_IRON ? BLOCK_IRON_DOOR : BLOCK_WOOD_DOOR), (float)px + 0.5f, (float)py + 0.5f, (float)pz + 0.5f, 1.0f, 0.8f);
        restart_hand_swing();
        return;
    }

    int place_id = held->id;
    if (place_id == ITEM_REED) place_id = BLOCK_REEDS;
    if (place_id == ITEM_REDSTONE) place_id = BLOCK_REDSTONE_WIRE;
    if (place_id == ITEM_SEEDS) place_id = BLOCK_CROPS;
    if (place_id == ITEM_SIGN) place_id = (hit.face == 1) ? BLOCK_SIGN_POST : BLOCK_WALL_SIGN;

    int place_existing = flat_get_block(px, py, pz);
    if (place_existing != 0 && !block_is_liquid(place_existing) && place_existing != BLOCK_SNOW_LAYER) return;
    if (place_id != BLOCK_LADDER && place_id != BLOCK_SIGN_POST && place_id != BLOCK_WALL_SIGN &&
        !block_is_torch_id(place_id) && flat_block_intersects_player(px, py, pz)) return;
    if (place_id == BLOCK_CHEST && !chest_can_place_at(px, py, pz)) return;
    if (place_id == BLOCK_LADDER && !ladder_can_attach_to_face(hit.face)) return;
    if (place_id == BLOCK_WALL_SIGN && !ladder_can_attach_to_face(hit.face)) return;
    if (block_is_torch_id(place_id) && !torch_can_place_at(px, py, pz)) return;
    if (place_id == BLOCK_STONE_BUTTON) {
        int bm = side_meta_from_placement_face(hit.face);
        if (bm == 0 || !block_supports_attached_side(px, py, pz, bm)) return;
    }
    if (place_id == BLOCK_LEVER) {
        int lm = lever_meta_from_placement_face(hit.face);
        if (lm == 0 || !lever_can_stay_with_meta(px, py, pz, lm)) return;
    }
    if (block_is_pressure_plate(place_id) && !flat_block_occludes_for_support(flat_get_block(px, py - 1, pz))) return;
    if (place_id == BLOCK_CACTUS && !cactus_can_stay_at(px, py, pz)) return;
    if (place_id == BLOCK_REEDS && !reeds_can_stay_at(px, py, pz)) return;
    if (place_id == BLOCK_CROPS && flat_get_block(px, py - 1, pz) != BLOCK_FARMLAND) return;

    if (!g_mp_connected) flat_begin_persistent_edit();
    flat_set_block(px, py, pz, place_id);
    if (place_id == BLOCK_CHEST) chest_on_block_placed(px, py, pz);
    if (place_id == BLOCK_FURNACE || place_id == BLOCK_FURNACE_LIT) {
        furnace_on_block_placed(px, py, pz);
        flat_set_meta_raw(px, py, pz, furnace_facing_from_yaw());
    }
    if (place_id == BLOCK_LADDER || place_id == BLOCK_WALL_SIGN) flat_set_meta_raw(px, py, pz, hit.face);
    if (block_is_torch_id(place_id)) flat_set_meta_raw(px, py, pz, torch_meta_from_placement_face(px, py, pz, hit.face));
    if (place_id == BLOCK_STONE_BUTTON) flat_set_meta_raw(px, py, pz, side_meta_from_placement_face(hit.face));
    if (place_id == BLOCK_LEVER) flat_set_meta_raw(px, py, pz, lever_meta_from_placement_face(hit.face));
    if (place_id == BLOCK_SIGN_POST) flat_set_meta_raw(px, py, pz, door_direction_from_yaw() & 3);
    if (place_id == BLOCK_WOOD_STAIRS || place_id == BLOCK_COBBLE_STAIRS) flat_set_meta_raw(px, py, pz, stair_direction_from_yaw());
    if (!g_mp_connected) flat_end_persistent_edit();
    if (place_id == BLOCK_REDSTONE_WIRE || place_id == BLOCK_STONE_BUTTON || place_id == BLOCK_LEVER ||
        place_id == BLOCK_REDSTONE_TORCH_OFF || place_id == BLOCK_REDSTONE_TORCH_ON ||
        block_is_pressure_plate(place_id) || block_is_door_id(place_id)) {
        redstone_update_near(px, py, pz);
    }
    if (g_mp_connected) {
        pex_net_send_player_action(PEX_ACTION_PLACE, px, py, pz, hit.face, place_id);
        pex_net_send_block_action(PEX_BLOCK_PLACE, px, py, pz, hit.face, place_id);
    }
    if (!player_is_creative() && --held->count <= 0) stack_clear(held);
    if (!g_mp_connected) g_save_dirty = 1;
    pex_sound_play_at(pex_block_step_sound_key(place_id), (float)px + 0.5f, (float)py + 0.5f, (float)pz + 0.5f, 1.0f, 0.8f);
    restart_hand_swing();
}



static int dropped_stack_can_merge(const FlatDroppedItem *a, const FlatDroppedItem *b) {
    if (!a || !b || !a->active || !b->active) return 0;
    if (!stack_same_item(&a->stack, &b->stack)) return 0;
    if (a->stack.count >= stack_limit_for_id(a->stack.id)) return 0;
    return 1;
}

static void merge_nearby_dropped_items(int idx) {
    FlatDroppedItem *a = &g_drops[idx];
    if (!a->active) return;
    for (int j = 0; j < MAX_DROP_ENTITIES; j++) {
        if (j == idx) continue;
        FlatDroppedItem *b = &g_drops[j];
        if (!dropped_stack_can_merge(a, b)) continue;

        float dx = a->x - b->x;
        float dy = a->y - b->y;
        float dz = a->z - b->z;
        if (dx*dx + dy*dy + dz*dz > 0.50f * 0.50f) continue;

        int room = stack_limit_for_id(a->stack.id) - a->stack.count;
        int move = b->stack.count < room ? b->stack.count : room;
        if (move <= 0) continue;

        a->stack.count += move;
        b->stack.count -= move;
        a->pickup_delay = (a->pickup_delay > b->pickup_delay) ? a->pickup_delay : b->pickup_delay;
        a->age = (a->age < b->age) ? a->age : b->age;

        if (b->stack.count <= 0) b->active = 0;
        g_save_dirty = 1;
        if (a->stack.count >= stack_limit_for_id(a->stack.id)) return;
    }
}


static void spawn_pickup_fx_from_drop(const FlatDroppedItem *drop) {
    if (!drop || stack_empty(&drop->stack)) return;
    int slot = -1;
    for (int i = 0; i < MAX_PICKUP_FX; ++i) if (!g_pickup_fx[i].active) { slot = i; break; }
    if (slot < 0) slot = 0;
    PickupFx *fx = &g_pickup_fx[slot];
    memset(fx, 0, sizeof(*fx));
    fx->active = 1;
    fx->stack = drop->stack;
    fx->start_x = drop->x;
    fx->start_y = drop->y;
    fx->start_z = drop->z;
    fx->prev_player_x = g_player_prev_x;
    fx->prev_player_y = g_player_prev_y;
    fx->prev_player_z = g_player_prev_z;
    fx->age = 0;
    fx->max_age = 3;
    fx->rot = drop->rot;
}

static float block_slipperiness_for_item(int id) {
    if (id == BLOCK_ICE) return 0.98f;
    return 0.60f;
}

static void inventory_drop_death_items(void) {
    for (int i = 0; i < 36; i++) {
        if (!stack_empty(&g_inventory[i])) {
            if (g_mp_connected) pex_net_send_drop_item(g_inventory[i]);
            else spawn_item_stack(g_player_x, g_player_y - 0.30f, g_player_z, g_inventory[i], 1);
            stack_clear(&g_inventory[i]);
        }
    }
    for (int i = 0; i < 4; i++) {
        if (!stack_empty(&g_armor_inventory[i])) {
            if (g_mp_connected) pex_net_send_drop_item(g_armor_inventory[i]);
            else spawn_item_stack(g_player_x, g_player_y - 0.30f, g_player_z, g_armor_inventory[i], 1);
            stack_clear(&g_armor_inventory[i]);
        }
    }
    armor_sync_player_armor();
    g_player_damage_remainder = 0;
    if (!stack_empty(&g_carried_stack)) {
        if (g_mp_connected) pex_net_send_drop_item(g_carried_stack);
        else spawn_item_stack(g_player_x, g_player_y - 0.30f, g_player_z, g_carried_stack, 1);
        stack_clear(&g_carried_stack);
    }
    memset(g_craft_grid, 0, sizeof(g_craft_grid));
    if (!g_mp_connected) g_save_dirty = 1;
}



static int dropped_item_touches_player(const FlatDroppedItem *e) {
    float dx = e->x - g_player_x;
    float dy = e->y - (g_player_y - 0.90f);
    float dz = e->z - g_player_z;
    return (dx * dx + dy * dy + dz * dz) <= (1.25f * 1.25f);
}

static void update_dropped_items(void) {
    for (int i = 0; i < MAX_PICKUP_FX; ++i) {
        if (!g_pickup_fx[i].active) continue;
        if (++g_pickup_fx[i].age >= g_pickup_fx[i].max_age) g_pickup_fx[i].active = 0;
    }
    for (int i = 0; i < MAX_DROP_ENTITIES; i++) {
        FlatDroppedItem *e = &g_drops[i];
        if (!e->active) continue;
        int multiplayer_drop = g_mp_connected && e->net_id > 0;
        e->prev_x = e->x; e->prev_y = e->y; e->prev_z = e->z;
        if (e->pickup_delay > 0) e->pickup_delay--;
        /* Deobf EntityItem has no magnetic pull; pickup happens on entity collision. */
        e->my -= 0.04f;
        int fluid_here = flat_get_block((int)floorf(e->x), (int)floorf(e->y), (int)floorf(e->z));
        if (fluid_here == BLOCK_LAVA || fluid_here == BLOCK_STILL_LAVA) {
            e->my = 0.20f;
            e->mx = ((float)rand() / (float)RAND_MAX - 0.5f) * 0.20f;
            e->mz = ((float)rand() / (float)RAND_MAX - 0.5f) * 0.20f;
        }

        /* Java Entity.moveEntity-style world collision: build all block AABBs
           around the swept item box, clip Y/X/Z offsets in that order, move the
           bounding box, then zero collided motion components.  This removes the
           old PexCraft axis rewind/bounce that made dropped items jitter. */
        dropped_item_move_entity(e, e->mx, e->my, e->mz);

        float friction = 0.98f;
        if (e->on_ground) {
            int below = flat_get_block((int)floorf(e->x), (int)floorf(e->y - 0.25f), (int)floorf(e->z));
            friction = block_slipperiness_for_item(below) * 0.98f;
        }
        e->mx *= friction;
        e->my *= 0.98f;
        e->mz *= friction;
        if (e->on_ground) e->my *= -0.5f;
        e->age++;
        if (e->age >= 6000 || e->y < -24.0f) { e->active = 0; continue; }

        if (!g_player_dead && g_screen == SCREEN_INGAME && e->pickup_delay <= 0 && dropped_item_touches_player(e)) {
            if (multiplayer_drop) {
                pex_net_send_pickup_drop(e->net_id);
            } else if (inventory_add_stack(e->stack)) {
                spawn_pickup_fx_from_drop(e);
                pex_sound_play("random.pop", 0.20f, (((float)rand() / (float)RAND_MAX) - ((float)rand() / (float)RAND_MAX)) * 0.7f + 1.0f);
                e->active = 0;
                g_save_dirty = 1;
            }
        }
        /* Java EntityItem in this source does not merge nearby drops. */
    }
}


static int flat_spawn_has_open_sky(int x, int z, int foot_y) {
    /* Reject cave/overhang spawn candidates.  The old test only required three
       air blocks above the ground, so a cave floor under a stone roof could win.
       For new/respawn placement, prefer a column with clear sky above the
       player's body. */
    for (int y = foot_y + 3; y <= FLAT_WORLD_Y_MAX; y++) {
        int id = flat_get_block(x, y, z);
        if (id == 0 || id == BLOCK_SNOW_LAYER || id == BLOCK_YELLOW_FLOWER || id == BLOCK_RED_ROSE ||
            id == BLOCK_BROWN_MUSHROOM || id == BLOCK_RED_MUSHROOM || id == BLOCK_TORCH ||
            id == BLOCK_REDSTONE_TORCH_OFF || id == BLOCK_REDSTONE_TORCH_ON) {
            continue;
        }
        return 0;
    }
    return 1;
}

static int flat_column_spawn_y_checked(int x, int z, int require_sky) {
    if (x < g_flat_world_origin_x || x >= g_flat_world_origin_x + FLAT_WORLD_SIZE || z < g_flat_world_origin_z || z >= g_flat_world_origin_z + FLAT_WORLD_SIZE) return -9999;
    if (!flat_chunk_generated_at_block(x, z)) return -9999;
    for (int y = FLAT_WORLD_Y_MAX - 3; y >= FLAT_WORLD_Y_MIN; y--) {
        int id = flat_get_block(x, y, z);
        if (!flat_solid_for_spawn(id)) continue;
        if (flat_get_block(x, y + 1, z) == 0 &&
            flat_get_block(x, y + 2, z) == 0 &&
            flat_get_block(x, y + 3, z) == 0) {
            int foot_y = y + 1;
            if (require_sky && !flat_spawn_has_open_sky(x, z, foot_y)) continue;
            return foot_y;
        }
    }
    return -9999;
}

static int flat_column_spawn_y(int x, int z) {
    return flat_column_spawn_y_checked(x, z, 1);
}

static int flat_find_safe_spawn_limited(float *out_x, float *out_y, float *out_z, int require_sky, int max_radius) {
    const int center_x = 0;
    const int center_z = 0;

    int best_x = 0, best_y = -9999, best_z = 0;
    int best_score = 0x7fffffff;
    int hard_limit = FLAT_WORLD_SIZE / 2 - 2;
    if (max_radius < 0 || max_radius > hard_limit) max_radius = hard_limit;

    for (int r = 0; r <= max_radius; r++) {
        for (int dz = -r; dz <= r; dz++) {
            for (int dx = -r; dx <= r; dx++) {
                if (abs(dx) != r && abs(dz) != r) continue;
                int x = center_x + dx;
                int z = center_z + dz;
                if (x < g_flat_world_origin_x + 2 || x >= g_flat_world_origin_x + FLAT_WORLD_SIZE - 2 ||
                    z < g_flat_world_origin_z + 2 || z >= g_flat_world_origin_z + FLAT_WORLD_SIZE - 2) continue;

                int sy = flat_column_spawn_y_checked(x, z, require_sky);
                if (sy == -9999) continue;

                int ground = flat_get_block(x, sy - 1, z);
                if (block_is_liquid(ground) || ground == BLOCK_LEAVES) continue;
                /* Prefer normal outdoor surface blocks; avoid ore/stone cliff ledges. */
                int surface_bonus = (ground == BLOCK_GRASS || ground == BLOCK_SAND || ground == BLOCK_DIRT) ? 0 : 2000;
                int score = dx*dx + dz*dz + surface_bonus + abs(sy - 34) * 2;

                float px = (float)x + 0.5f;
                float py = (float)sy + 1.62f;
                float pz = (float)z + 0.5f;
                if (!flat_player_aabb_collides(px, py, pz)) {
                    if (score < best_score) {
                        best_score = score;
                        best_x = x; best_y = sy; best_z = z;
                    }
                    if (r <= 8 && surface_bonus == 0) {
                        *out_x = px; *out_y = py; *out_z = pz;
                        return 1;
                    }
                }
            }
        }
        if (best_y != -9999 && r > 16) break;
    }

    if (best_y != -9999) {
        *out_x = (float)best_x + 0.5f;
        *out_y = (float)best_y + 1.62f;
        *out_z = (float)best_z + 0.5f;
        return 1;
    }
    return 0;
}

static int flat_find_safe_spawn_pass(float *out_x, float *out_y, float *out_z, int require_sky) {
    return flat_find_safe_spawn_limited(out_x, out_y, out_z, require_sky, -1);
}

static int flat_find_safe_spawn(float *out_x, float *out_y, float *out_z) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    /* PSP/Wii: never spawn above empty or not-yet-meshed terrain and hope collision catches
       up.  First try real generated terrain; then stamp a tiny metadata-tagged
       flat safety surface so first spawn/respawn has a guaranteed solid block
       under the player's feet while the real Beta chunks finish streaming. */
    if (flat_find_safe_spawn_limited(out_x, out_y, out_z, 1, 24)) {
        psp_ensure_spawn_surface(out_x, out_y, out_z, 0);
        return 1;
    }
    if (flat_find_safe_spawn_limited(out_x, out_y, out_z, 0, 40)) {
        psp_ensure_spawn_surface(out_x, out_y, out_z, 0);
        return 1;
    }

    *out_x = 0.5f;
    *out_y = 65.62f;
    *out_z = 0.5f;
    psp_ensure_spawn_surface(out_x, out_y, out_z, 1);
    g_psp_respawn_snap_pending = 1;
    return 1;
#else
    if (flat_find_safe_spawn_pass(out_x, out_y, out_z, 1)) return 1;

    /* Last-resort fallback keeps old saves/world types playable if the center
       area is completely roofed, but normal new worlds now choose open sky. */
    if (flat_find_safe_spawn_pass(out_x, out_y, out_z, 0)) return 1;

    *out_x = 0.5f;
    *out_y = (float)FLAT_WORLD_Y_MAX + 3.0f;
    *out_z = 0.5f;
    return 0;
#endif
}

static void psp_snap_respawn_to_ready_ground(void) {
#if defined(PEX_PLATFORM_PSP)
    if (!g_psp_respawn_snap_pending) return;
    if (!flat_player_columns_generated_at(g_player_x, g_player_z)) return;

    float sx, sy, sz;
    if (flat_find_safe_spawn_limited(&sx, &sy, &sz, 1, 24) ||
        flat_find_safe_spawn_limited(&sx, &sy, &sz, 0, 40)) {
        psp_ensure_spawn_surface(&sx, &sy, &sz, 0);
        g_player_x = g_player_prev_x = sx;
        g_player_y = g_player_prev_y = sy;
        g_player_z = g_player_prev_z = sz;
        g_player_motion_x = g_player_motion_y = g_player_motion_z = 0.0f;
        g_player_fall_distance = 0.0f;
        g_player_on_ground = 1;
        g_psp_respawn_snap_pending = 0;
        return;
    }

    /* If the center generated as ocean or a cliff with no clean sky column, at
       least avoid keeping the player in the void. */
    int x = (int)floorf(g_player_x);
    int z = (int)floorf(g_player_z);
    int syi = flat_column_spawn_y_checked(x, z, 0);
    if (syi != -9999) {
        float sx = (float)x + 0.5f;
        float sy = (float)syi + 1.62f;
        float sz = (float)z + 0.5f;
        psp_ensure_spawn_surface(&sx, &sy, &sz, 0);
        g_player_x = g_player_prev_x = sx;
        g_player_y = g_player_prev_y = sy;
        g_player_z = g_player_prev_z = sz;
        g_player_motion_x = g_player_motion_y = g_player_motion_z = 0.0f;
        g_player_fall_distance = 0.0f;
        g_player_on_ground = 1;
        g_psp_respawn_snap_pending = 0;
    }
#endif
}

static int psp_player_terrain_ready_or_hold(void) {
#if defined(PEX_PLATFORM_PSP)
    if (g_mp_connected) return 1;
    if (flat_player_columns_generated_at(g_player_x, g_player_z)) {
        psp_snap_respawn_to_ready_ground();
        return 1;
    }

    int pcx = floor_div16((int)floorf(g_player_x));
    int pcz = floor_div16((int)floorf(g_player_z));
    for (int dz = -1; dz <= 1; dz++) {
        for (int dx = -1; dx <= 1; dx++) {
            stream_queue_add_chunk(pcx + dx, pcz + dz);
        }
    }
    process_stream_generation_queue();

    g_player_prev_x = g_player_x;
    g_player_prev_y = g_player_y;
    g_player_prev_z = g_player_z;
    g_player_motion_x = g_player_motion_y = g_player_motion_z = 0.0f;
    g_player_fall_distance = 0.0f;
    g_player_on_ground = 1;
    if (g_ingame_ticks - g_psp_terrain_wait_chat_tick > 60) {
        hud_add_chat("Waiting for terrain...");
        g_psp_terrain_wait_chat_tick = g_ingame_ticks;
    }
    return 0;
#else
    return 1;
#endif
}

static int flat_player_suffocation_block(void) {
    /* Source does an "inside opaque block" test and renders that block texture over
       the screen.  Use the eye and upper-body samples for this flat renderer. */
    float samples[4][3] = {
        {g_player_x, g_player_y, g_player_z},
        {g_player_x - 0.22f, g_player_y - 0.30f, g_player_z},
        {g_player_x + 0.22f, g_player_y - 0.30f, g_player_z},
        {g_player_x, g_player_y - 0.80f, g_player_z}
    };

    for (int i = 0; i < 4; i++) {
        int bx = (int)floorf(samples[i][0]);
        int by = (int)floorf(samples[i][1]);
        int bz = (int)floorf(samples[i][2]);
        int id = flat_get_block(bx, by, bz);
        if (flat_solid_for_spawn(id) && id != BLOCK_GLASS && id != BLOCK_LEAVES) return id;
    }
    return 0;
}

static int g_falling_wakeups_accum = 0;
static int g_falling_spawns_accum = 0;

static int block_is_falling_type(int id) {
    return id == BLOCK_SAND || id == BLOCK_GRAVEL;
}

static int active_world_sim_radius(void) {
    /* Keep expensive falling/liquid simulation close to the player.  Rendering
       distance can be much larger than the area that needs block updates; tying
       these together was the main reason in-world FPS collapsed. */
    int range = g_opts.render_distance * 8 + 8;
    if (range < 24) range = 24;
    if (range > MAX_WORLD_SIM_RADIUS) range = MAX_WORLD_SIM_RADIUS;
    if (range > FLAT_WORLD_SIZE / 2) range = FLAT_WORLD_SIZE / 2;
    return range;
}

static int falling_block_can_fall_through(int id) {
    return id == 0 || block_is_liquid(id) || id == BLOCK_SNOW_LAYER;
}

static int falling_block_aabb_collides(float x, float y, float z) {
    float minx = x - 0.49f, maxx = x + 0.49f;
    float miny = y - 0.49f, maxy = y + 0.49f;
    float minz = z - 0.49f, maxz = z + 0.49f;
    int x0 = (int)floorf(minx), x1 = (int)floorf(maxx - 0.001f);
    int y0 = (int)floorf(miny), y1 = (int)floorf(maxy - 0.001f);
    int z0 = (int)floorf(minz), z1 = (int)floorf(maxz - 0.001f);
    for (int y = y0; y <= y1; y++) {
        for (int z = z0; z <= z1; z++) {
            for (int x = x0; x <= x1; x++) {
                if (flat_block_is_solid(flat_get_block(x, y, z))) return 1;
            }
        }
    }
    return 0;
}

static int spawn_falling_block_entity(int x, int y, int z, int block_id) {
    if (!flat_in_bounds(x, y, z) || !block_is_falling_type(block_id)) return 0;
    int slot = -1;
    for (int i = 0; i < MAX_FALLING_BLOCK_ENTITIES; i++) {
        if (!g_falling_blocks[i].active) { slot = i; break; }
    }
    if (slot < 0) return 0;
    FlatFallingBlock *fb = &g_falling_blocks[slot];
    memset(fb, 0, sizeof(*fb));
    fb->active = 1; /* reserve the slot before block removal wakes stacked sand */
    fb->net_id = slot + 1;
    fb->block_id = block_id;
    fb->x = fb->prev_x = (float)x + 0.5f;
    fb->y = fb->prev_y = (float)y + 0.5f;
    fb->z = fb->prev_z = (float)z + 0.5f;
    flat_set_block(x, y, z, 0);
    g_falling_spawns_accum++;
    return 1;
}

static void wake_falling_block_at(int x, int y, int z) {
    if (g_mp_connected) return;
    g_falling_wakeups_accum++;
    if (!flat_in_bounds(x, y, z) || y <= FLAT_WORLD_Y_MIN) return;
    int id = flat_get_block(x, y, z);
    if (!block_is_falling_type(id)) return;
    if (!falling_block_can_fall_through(flat_get_block(x, y - 1, z))) return;
    spawn_falling_block_entity(x, y, z, id);
}

static void wake_falling_blocks_around(int x, int y, int z) {
    if (g_mp_connected) return;
    /* Block updates wake gravity immediately.  There is no background scanner. */
    wake_falling_block_at(x, y, z);
    if (y < FLAT_WORLD_Y_MAX) wake_falling_block_at(x, y + 1, z);
}

static void finish_falling_block_entity(FlatFallingBlock *fb) {
    if (!fb || !fb->active) return;
    int bx = (int)floorf(fb->x);
    int by = (int)floorf(fb->y);
    int bz = (int)floorf(fb->z);
    int target = flat_get_block(bx, by, bz);
    if (flat_in_bounds(bx, by, bz) && falling_block_can_fall_through(target)) {
        flat_set_block(bx, by, bz, fb->block_id);
    } else {
        int drop = block_drop_id(fb->block_id);
        if (drop > 0) spawn_item_stack(fb->x, fb->y, fb->z, make_stack(drop, 1, 0), 1);
    }
    memset(fb, 0, sizeof(*fb));
}

static void update_falling_blocks(void) {
    /* Sand/gravel is event-driven only.  There is no background world scan here:
       placing sand/gravel or changing the block below it wakes gravity directly
       through flat_set_block()/flat_set_fluid(). */
    int active_count = 0;

    for (int i = 0; i < MAX_FALLING_BLOCK_ENTITIES; i++) {
        FlatFallingBlock *fb = &g_falling_blocks[i];
        if (!fb->active) continue;
        active_count++;
        fb->prev_x = fb->x;
        fb->prev_y = fb->y;
        fb->prev_z = fb->z;
        fb->my -= 0.04f;
        fb->x += fb->mx;
        fb->y += fb->my;
        fb->z += fb->mz;
        int on_ground = 0;
        if (falling_block_aabb_collides(fb->x, fb->y, fb->z)) {
            fb->x = fb->prev_x;
            fb->y = fb->prev_y;
            fb->z = fb->prev_z;
            if (fb->my < 0.0f) on_ground = 1;
            fb->mx *= 0.70f;
            fb->my *= -0.50f;
            fb->mz *= 0.70f;
        }
        fb->mx *= 0.98f;
        fb->my *= 0.98f;
        fb->mz *= 0.98f;
        fb->age++;
        if (on_ground) {
            finish_falling_block_entity(fb);
        } else if (fb->age > 100 || fb->y < -16.0f) {
            int drop = block_drop_id(fb->block_id);
            if (drop > 0) spawn_item_stack(fb->x, fb->y, fb->z, make_stack(drop, 1, 0), 1);
            memset(fb, 0, sizeof(*fb));
        }
    }

    g_prof_falling_active_last = active_count;
    g_prof_falling_cells_last = g_falling_wakeups_accum;
    g_prof_falling_spawns_last = g_falling_spawns_accum;
    g_falling_wakeups_accum = 0;
    g_falling_spawns_accum = 0;
}



static void reset_flat_player_spawn(void) {
    float sx, sy, sz;
    flat_find_safe_spawn(&sx, &sy, &sz);

    g_player_x = g_player_prev_x = sx;
    g_player_y = g_player_prev_y = sy;
    g_player_z = g_player_prev_z = sz;
    g_player_motion_x = g_player_motion_y = g_player_motion_z = 0.0f;
    g_player_fall_distance = 0.0f;
    g_player_dead = 0;
    g_player_death_time = 0;
    g_suffocation_damage_timer = 0;
    g_player_yaw = g_player_prev_yaw = 0.0f;
    g_player_pitch = g_player_prev_pitch = 0.0f;
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    psp_ensure_spawn_surface(&g_player_x, &g_player_y, &g_player_z, 0);
    g_player_prev_x = g_player_x;
    g_player_prev_y = g_player_y;
    g_player_prev_z = g_player_z;
    g_player_on_ground = 1;
#else
    g_player_on_ground = 0;
#endif
    g_distance_walked = g_prev_distance_walked = 0.0f;
    g_limb_swing = g_prev_limb_swing = 0.0f;
    g_limb_swing_amount = g_prev_limb_swing_amount = 0.0f;
    g_camera_yaw = g_prev_camera_yaw = 0.0f;
    g_camera_pitch = g_prev_camera_pitch = 0.0f;
    g_render_arm_yaw = g_prev_render_arm_yaw = 0.0f;
    g_render_arm_pitch = g_prev_render_arm_pitch = 0.0f;
    g_hand_swing = g_prev_hand_swing = 0.0f;
    g_hand_swing_ticks = 0;
    g_hand_swing_progress = 0.0f;
    g_hand_swing_active = 0;
    g_air_swing_playing = 0;
    g_air_swing_ticks = 0;
    g_air_swing_consumed = 0;
    g_break_swing_consumed = 0;
    g_break_swing_holding = 0;
}




/* ---- Beta-accurate fluid simulation (mirrors BlockFluids/BlockFlowing) ---- */
static const int FLUID_DX[4] = {-1, 1, 0, 0};
static const int FLUID_DZ[4] = {0, 0, -1, 1};
#if defined(PEX_PLATFORM_PSP)
enum { FLUID_SIM_RADIUS = 8 };
enum { FLUID_VERTICAL_RADIUS = 18 };
#else
enum { FLUID_SIM_RADIUS = 16 };
enum { FLUID_VERTICAL_RADIUS = FLAT_WORLD_HEIGHT };
#endif
enum { FLUID_TICK_MAX = (FLUID_SIM_RADIUS * 2 + 1) * (FLUID_SIM_RADIUS * 2 + 1) * ((FLUID_VERTICAL_RADIUS * 2) + 3) };

typedef struct FluidTickCell {
    int x, y, z;
    unsigned char is_water;
} FluidTickCell;

static int fluid_same_family(int id, int is_water) {
    if (!block_is_liquid(id)) return 0;
    return block_is_water(id) ? is_water : !is_water;
}

static int fluid_cell_replaceable(int id, int is_water) {
    if (id == 0) return 1;
    if (fluid_same_family(id, is_water)) return 0;
    if (block_is_lava(id)) return 0;
    return !block_material_blocks_liquid(id);
}

static int fluid_flow_cost(int x, int y, int z, int is_water, int dist, int skip_dir) {
    int best = 1000;
    for (int d = 0; d < 4; d++) {
        if ((d == 0 && skip_dir == 1) || (d == 1 && skip_dir == 0) ||
            (d == 2 && skip_dir == 3) || (d == 3 && skip_dir == 2)) continue;
        int nx = x + FLUID_DX[d], nz = z + FLUID_DZ[d];
        int n = flat_get_block(nx, y, nz);
        if (block_material_blocks_liquid(n)) continue;
        if (fluid_same_family(n, is_water) && flat_get_level(nx, y, nz) == 0) continue;
        int below = flat_get_block(nx, y - 1, nz);
        if (!block_material_blocks_liquid(below)) return dist;
        if (dist >= 4) continue;
        int c = fluid_flow_cost(nx, y, nz, is_water, dist + 1, d);
        if (c < best) best = c;
    }
    return best;
}

static void fluid_optimal_dirs(int x, int y, int z, int is_water, int out[4]) {
    int cost[4];
    int minc = 1000000;
    for (int d = 0; d < 4; d++) {
        out[d] = 0;
        int nx = x + FLUID_DX[d], nz = z + FLUID_DZ[d];
        int n = flat_get_block(nx, y, nz);
        if (block_material_blocks_liquid(n) ||
            (fluid_same_family(n, is_water) && flat_get_level(nx, y, nz) == 0)) {
            cost[d] = -1;
            continue;
        }
        int below = flat_get_block(nx, y - 1, nz);
        if (!block_material_blocks_liquid(below)) cost[d] = 0;
        else cost[d] = fluid_flow_cost(nx, y, nz, is_water, 1, d);
        if (cost[d] < minc) minc = cost[d];
    }
    for (int d = 0; d < 4; d++) if (cost[d] >= 0 && cost[d] == minc) out[d] = 1;
}

static int fluid_neighbor_min_level(int x, int y, int z, int is_water, int current_min, int *source_count) {
    int level = flat_get_level(x, y, z);
    if (!fluid_same_family(flat_get_block(x, y, z), is_water)) return current_min;
    if (level == 0 && source_count) (*source_count)++;
    if (level >= 8) level = 0;
    return (current_min >= 0 && level >= current_min) ? current_min : level;
}

static void fluid_spread_to(int x, int y, int z, int is_water, int level) {
    if (!fluid_cell_replaceable(flat_get_block(x, y, z), is_water)) return;
    int old = flat_get_block(x, y, z);
    if (old > 0 && !block_is_liquid(old)) {
        if (is_water) {
            int drop = block_drop_id(old);
            if (drop > 0) spawn_item_stack((float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, make_stack(drop, 1, 0), 1);
        }
    }
    flat_set_fluid(x, y, z, fluid_flow_id(is_water), level);
}

static void fluid_set_still_if_stable(int x, int y, int z, int is_water, int level) {
    flat_set_fluid(x, y, z, fluid_still_id(is_water), level);
}

static int fluid_effective_level_at(int x, int y, int z, int is_water) {
    if (!fluid_same_family(flat_get_block(x, y, z), is_water)) return -1;
    int level = flat_get_level(x, y, z);
    if (level >= 8) level = 0;
    return level;
}

static int fluid_side_exposed_for_flow(int x, int y, int z) {
    int id = flat_get_block(x, y, z);
    if (block_is_liquid(id)) return 0;
    if (id == BLOCK_ICE) return 0;
    return !block_material_blocks_liquid(id);
}

static void fluid_flow_vector_at(int x, int y, int z, int is_water, float *vx, float *vy, float *vz) {
    float ax = 0.0f, ay = 0.0f, az = 0.0f;
    int current = fluid_effective_level_at(x, y, z, is_water);
    if (current < 0) return;

    for (int d = 0; d < 4; d++) {
        int nx = x + FLUID_DX[d];
        int nz = z + FLUID_DZ[d];
        int nl = fluid_effective_level_at(nx, y, nz, is_water);
        if (nl < 0) {
            if (!block_material_blocks_liquid(flat_get_block(nx, y, nz))) {
                nl = fluid_effective_level_at(nx, y - 1, nz, is_water);
                if (nl >= 0) {
                    int diff = nl - (current - 8);
                    ax += (float)FLUID_DX[d] * (float)diff;
                    az += (float)FLUID_DZ[d] * (float)diff;
                }
            }
        } else {
            int diff = nl - current;
            ax += (float)FLUID_DX[d] * (float)diff;
            az += (float)FLUID_DZ[d] * (float)diff;
        }
    }

    if (flat_get_level(x, y, z) >= 8) {
        int edge =
            fluid_side_exposed_for_flow(x, y, z - 1) ||
            fluid_side_exposed_for_flow(x, y, z + 1) ||
            fluid_side_exposed_for_flow(x - 1, y, z) ||
            fluid_side_exposed_for_flow(x + 1, y, z) ||
            fluid_side_exposed_for_flow(x, y + 1, z - 1) ||
            fluid_side_exposed_for_flow(x, y + 1, z + 1) ||
            fluid_side_exposed_for_flow(x - 1, y + 1, z) ||
            fluid_side_exposed_for_flow(x + 1, y + 1, z);
        if (edge) {
            float len = sqrtf(ax * ax + ay * ay + az * az);
            if (len > 0.0001f) { ax /= len; ay /= len; az /= len; }
            ay -= 6.0f;
        }
    }

    float len = sqrtf(ax * ax + ay * ay + az * az);
    if (len > 0.0001f) {
        *vx += ax / len;
        *vy += ay / len;
        *vz += az / len;
    }
}

static void apply_player_fluid_velocity(int is_water) {
    float vx = 0.0f, vy = 0.0f, vz = 0.0f;
    float min_x = g_player_x - 0.30f;
    float max_x = g_player_x + 0.30f;
    float min_y = g_player_y - 1.62f - 0.40f;
    float max_y = g_player_y;
    float min_z = g_player_z - 0.30f;
    float max_z = g_player_z + 0.30f;

    int x0 = (int)floorf(min_x), x1 = (int)floorf(max_x + 1.0f);
    int y0 = (int)floorf(min_y), y1 = (int)floorf(max_y + 1.0f);
    int z0 = (int)floorf(min_z), z1 = (int)floorf(max_z + 1.0f);
    for (int y = y0; y < y1; y++) {
        for (int z = z0; z < z1; z++) {
            for (int x = x0; x < x1; x++) {
                if (!fluid_same_family(flat_get_block(x, y, z), is_water)) continue;
                float surface = (float)(y + 1) - fluid_decay_height(flat_get_level(x, y, z));
                if (max_y < surface) continue;
                fluid_flow_vector_at(x, y, z, is_water, &vx, &vy, &vz);
            }
        }
    }

    float len = sqrtf(vx * vx + vy * vy + vz * vz);
    if (len > 0.0001f) {
        const float push = 0.004f;
        g_player_motion_x += (vx / len) * push;
        g_player_motion_y += (vy / len) * push;
        g_player_motion_z += (vz / len) * push;
    }
}

static void update_liquids(void) {
    if (stream_generation_active()) return;

#if defined(PEX_PLATFORM_PSP)
    /* Real Beta terrain can contain many water/lava blocks.  A full 33x33x128
       liquid scan is deadly on PSP, so tick less often and only near the player
       vertically.  Fluid behavior is preserved locally; distant underground
       liquids sleep until the player gets near them. */
    int tick_water = (g_ingame_ticks % 10) == 0;
    int tick_lava = (g_ingame_ticks % 60) == 0;
#else
    int tick_water = (g_ingame_ticks % 5) == 0;
    int tick_lava = (g_ingame_ticks % 30) == 0;
#endif
    if (!tick_water && !tick_lava) return;

    int range = active_world_sim_radius();
    if (range > FLUID_SIM_RADIUS) range = FLUID_SIM_RADIUS;

    int px = (int)floorf(g_player_x);
    int pz = (int)floorf(g_player_z);
    int min_x = px - range, max_x = px + range;
    int min_z = pz - range, max_z = pz + range;
    if (min_x < g_flat_world_origin_x + 1) min_x = g_flat_world_origin_x + 1;
    if (max_x >= g_flat_world_origin_x + FLAT_WORLD_SIZE - 1) max_x = g_flat_world_origin_x + FLAT_WORLD_SIZE - 2;
    if (min_z < g_flat_world_origin_z + 1) min_z = g_flat_world_origin_z + 1;
    if (max_z >= g_flat_world_origin_z + FLAT_WORLD_SIZE - 1) max_z = g_flat_world_origin_z + FLAT_WORLD_SIZE - 2;

    int moved = 0;
#if defined(PEX_PLATFORM_PSP)
    int changes_left = 8;
    int center_y = (int)floorf(g_player_y) - 1;
    int scan_y_max = center_y + FLUID_VERTICAL_RADIUS / 2;
    int scan_y_min = center_y - FLUID_VERTICAL_RADIUS;
    if (scan_y_max > FLAT_WORLD_Y_MAX - 1) scan_y_max = FLAT_WORLD_Y_MAX - 1;
    if (scan_y_min < FLAT_WORLD_Y_MIN + 1) scan_y_min = FLAT_WORLD_Y_MIN + 1;
#else
    int changes_left = MAX_WORLD_SIM_CHANGES_PER_TICK;
    int scan_y_max = FLAT_WORLD_Y_MAX - 1;
    int scan_y_min = FLAT_WORLD_Y_MIN + 1;
#endif

    static FluidTickCell fluid_ticks[FLUID_TICK_MAX];
    int fluid_tick_count = 0;

    for (int y = scan_y_max; y >= scan_y_min; y--) {
        for (int z = min_z; z <= max_z; z++) {
            for (int x = min_x; x <= max_x; x++) {
                int id = flat_get_block(x, y, z);
                if (!block_is_flowing_liquid(id)) continue;
                int is_water = block_is_water(id);
                if ((is_water && !tick_water) || (!is_water && !tick_lava)) continue;
                if (fluid_tick_count < FLUID_TICK_MAX) {
                    fluid_ticks[fluid_tick_count].x = x;
                    fluid_ticks[fluid_tick_count].y = y;
                    fluid_ticks[fluid_tick_count].z = z;
                    fluid_ticks[fluid_tick_count].is_water = (unsigned char)is_water;
                    fluid_tick_count++;
                }
            }
        }
    }

    for (int i = 0; i < fluid_tick_count && changes_left > 0; i++) {
        int x = fluid_ticks[i].x;
        int y = fluid_ticks[i].y;
        int z = fluid_ticks[i].z;
        int is_water = fluid_ticks[i].is_water ? 1 : 0;
        int id = flat_get_block(x, y, z);
        if (!block_is_flowing_liquid(id) || !fluid_same_family(id, is_water)) continue;

        int decay = is_water ? 1 : 2;

        fluid_check_for_harden(x, y, z);
        id = flat_get_block(x, y, z);
        if (!block_is_flowing_liquid(id) || !fluid_same_family(id, is_water)) continue;

        int level = flat_get_level(x, y, z);
        if (level > 0) {
            int source_count = 0;
            int min_level = -100;
            min_level = fluid_neighbor_min_level(x - 1, y, z, is_water, min_level, &source_count);
            min_level = fluid_neighbor_min_level(x + 1, y, z, is_water, min_level, &source_count);
            min_level = fluid_neighbor_min_level(x, y, z - 1, is_water, min_level, &source_count);
            min_level = fluid_neighbor_min_level(x, y, z + 1, is_water, min_level, &source_count);

            int new_level = min_level + decay;
            if (new_level >= 8 || min_level < 0) new_level = -1;

            if (fluid_same_family(flat_get_block(x, y + 1, z), is_water)) {
                int above_level = flat_get_level(x, y + 1, z);
                new_level = (above_level >= 8) ? above_level : above_level + 8;
            }

            if (source_count >= 2 && is_water) {
                int source_below = flat_get_block(x, y - 1, z);
                if (block_material_blocks_liquid(source_below) ||
                    (block_is_water(source_below) && flat_get_level(x, y - 1, z) == 0)) {
                    new_level = 0;
                }
            }

            if (new_level != level) {
                if (new_level < 0) {
                    flat_set_block(x, y, z, 0);
                    moved = 1;
                    changes_left--;
                    continue;
                }
                flat_set_fluid(x, y, z, fluid_flow_id(is_water), new_level);
                wake_neighbor_liquids(x, y, z);
                level = new_level;
                moved = 1;
                changes_left--;
                if (changes_left <= 0) continue;
            } else {
                fluid_set_still_if_stable(x, y, z, is_water, level);
                moved = 1;
                changes_left--;
                if (changes_left <= 0) continue;
            }
        } else {
            fluid_set_still_if_stable(x, y, z, is_water, level);
            moved = 1;
            changes_left--;
            if (changes_left <= 0) continue;
        }

        int below = flat_get_block(x, y - 1, z);
        if (fluid_cell_replaceable(below, is_water)) {
            int fall_level = (level >= 8) ? level : level + 8;
            fluid_spread_to(x, y - 1, z, is_water, fall_level);
            moved = 1; changes_left--;
            continue;
        }

        int spread_level = level + decay;
        if (level >= 8) spread_level = 1;
        if (spread_level >= 8) continue;
        if (level != 0 && !block_material_blocks_liquid(below)) continue;

        int dirs[4];
        fluid_optimal_dirs(x, y, z, is_water, dirs);
        for (int d = 0; d < 4 && changes_left > 0; d++) {
            if (!dirs[d]) continue;
            int nx = x + FLUID_DX[d], nz = z + FLUID_DZ[d];
            if (!fluid_cell_replaceable(flat_get_block(nx, y, nz), is_water)) continue;
            fluid_spread_to(nx, y, nz, is_water, spread_level);
            moved = 1; changes_left--;
        }
    }

    (void)moved;
}


static void stream_generation_queue_clear(void) {
    g_stream_gen_queue_count = 0;
    g_stream_gen_queue_index = 0;
    g_stream_gen_queue_installed_count = 0;
    g_stream_generation_keep_completed = 0;
    g_stream_generation_epoch++;
    if (g_stream_generation_epoch <= 0) g_stream_generation_epoch = 1;
}

/* Dedicated Java-style light propagation service.  Java 1.2.5 keeps light
   propagation out of renderer rebuild logic (World.updateLightByType marks and
   fixes light, RenderGlobal/WorldRenderer rebuilds geometry afterward).  The C
   port used to run full pending propagation from the same streaming service that
   feeds chunk generation; that could block visible chunk installs and left the
   user seeing holes while a torch/skylight region was being repaired. */
static CRITICAL_SECTION g_flat_lighting_worker_cs;
static int g_flat_lighting_worker_initialized = 0;
static int g_flat_lighting_worker_stop = 0;
static int g_flat_lighting_worker_busy = 0;
static HANDLE g_flat_lighting_worker_event = NULL;
static HANDLE g_flat_lighting_worker_thread = NULL;

static DWORD WINAPI flat_lighting_worker_proc(LPVOID unused) {
    (void)unused;
    g_pex_profile_thread_role = PEX_PROFILE_ROLE_ASYNC_STREAM;
    for (;;) {
        WaitForSingleObject(g_flat_lighting_worker_event, INFINITE);
        for (;;) {
            int stop = 0;
            EnterCriticalSection(&g_flat_lighting_worker_cs);
            stop = g_flat_lighting_worker_stop;
            if (!stop) g_flat_lighting_worker_busy = 1;
            LeaveCriticalSection(&g_flat_lighting_worker_cs);
            if (stop) break;

            if (g_stream_remap_in_progress) {
                Sleep(1);
                if (!flat_lighting_pending_dirty()) break;
                continue;
            }

            int before_pending = flat_lighting_pending_dirty();
            double light_worker_start = now_seconds();
            flat_flush_pending_lighting();
            if (before_pending) {
                double light_ms = (now_seconds() - light_worker_start) * 1000.0;
                if (light_ms < 0.0) light_ms = 0.0;
                g_prof_light_worker_last_ms = light_ms;
                if (g_prof_light_worker_samples <= 0) g_prof_light_worker_avg_ms = light_ms;
                else g_prof_light_worker_avg_ms = g_prof_light_worker_avg_ms * 0.90 + light_ms * 0.10;
                g_prof_light_worker_samples++;
            }
            if (!before_pending || !flat_lighting_pending_dirty()) break;
        }
        EnterCriticalSection(&g_flat_lighting_worker_cs);
        if (!g_flat_lighting_worker_stop) g_flat_lighting_worker_busy = 0;
        LeaveCriticalSection(&g_flat_lighting_worker_cs);

        EnterCriticalSection(&g_flat_lighting_worker_cs);
        int stop = g_flat_lighting_worker_stop;
        LeaveCriticalSection(&g_flat_lighting_worker_cs);
        if (stop) break;
    }
    EnterCriticalSection(&g_flat_lighting_worker_cs);
    g_flat_lighting_worker_busy = 0;
    LeaveCriticalSection(&g_flat_lighting_worker_cs);
    return 0;
}

static void flat_lighting_worker_ensure(void) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    return;
#else
    if (g_flat_lighting_worker_initialized) return;
    flat_light_dirty_lock();
    InitializeCriticalSection(&g_flat_lighting_worker_cs);
    g_flat_lighting_worker_stop = 0;
    g_flat_lighting_worker_busy = 0;
    g_flat_lighting_worker_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (g_flat_lighting_worker_event) {
        g_flat_lighting_worker_thread = CreateThread(NULL, 0x200000, flat_lighting_worker_proc, NULL, 0, NULL);
        if (g_flat_lighting_worker_thread) {
            SetThreadPriority(g_flat_lighting_worker_thread, THREAD_PRIORITY_NORMAL);
            pex_logf("lighting worker started");
        }
    }
    g_flat_lighting_worker_initialized = 1;
#endif
}

static void flat_lighting_worker_wake(void) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    return;
#else
    flat_lighting_worker_ensure();
    if (g_flat_lighting_worker_event && g_flat_lighting_worker_thread) SetEvent(g_flat_lighting_worker_event);
#endif
}

static void flat_lighting_worker_shutdown(void) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    return;
#else
    if (!g_flat_lighting_worker_initialized) return;
    EnterCriticalSection(&g_flat_lighting_worker_cs);
    g_flat_lighting_worker_stop = 1;
    LeaveCriticalSection(&g_flat_lighting_worker_cs);
    if (g_flat_lighting_worker_event) SetEvent(g_flat_lighting_worker_event);
    if (g_flat_lighting_worker_thread) {
        WaitForSingleObject(g_flat_lighting_worker_thread, INFINITE);
        CloseHandle(g_flat_lighting_worker_thread);
        g_flat_lighting_worker_thread = NULL;
    }
    if (g_flat_lighting_worker_event) {
        CloseHandle(g_flat_lighting_worker_event);
        g_flat_lighting_worker_event = NULL;
    }
    DeleteCriticalSection(&g_flat_lighting_worker_cs);
    g_flat_lighting_worker_initialized = 0;
    g_flat_lighting_worker_stop = 0;
    g_flat_lighting_worker_busy = 0;
    pex_logf("lighting worker stopped");
#endif
}

/* Exact seed terrain generation runs on worker threads.  The main/game/render
   thread never performs the expensive Beta 3x3 canvas generation while walking;
   it only installs completed chunk buffers.  Desktop uses a small CPU-aware pool
   so the 49-chunk spawn preload does not crawl one chunk at a time, while PSP/Wii
   keep their conservative cooperative/single-worker paths. */
typedef struct StreamAsyncJob {
    int cx, cz;
    int type;
    int epoch;
    long long seed;
    char world_dir[MAX_PATHBUF];
} StreamAsyncJob;

typedef struct StreamAsyncResult {
    int cx, cz;
    int epoch;
    unsigned char *buf;
    unsigned char *meta;
    unsigned char *sky;
    unsigned char *blocklight;
} StreamAsyncResult;

#define STREAM_ASYNC_MAX_WORKERS 4
#define STREAM_ASYNC_JOB_RING 128
#define STREAM_ASYNC_RESULT_RING 64

static CRITICAL_SECTION g_stream_async_cs;
static int g_stream_async_initialized = 0;
static HANDLE g_stream_async_event = NULL;
static HANDLE g_stream_async_threads[STREAM_ASYNC_MAX_WORKERS];
static int g_stream_async_worker_count = 0;
static int g_stream_async_stop = 0;
static int g_stream_async_active_count = 0;
static StreamAsyncJob g_stream_async_jobs[STREAM_ASYNC_JOB_RING];
static int g_stream_async_job_head = 0, g_stream_async_job_tail = 0, g_stream_async_job_count = 0;
static StreamAsyncResult g_stream_async_results[STREAM_ASYNC_RESULT_RING];
static int g_stream_async_result_head = 0, g_stream_async_result_tail = 0, g_stream_async_result_count = 0;

static void stream_async_free_result_payload(StreamAsyncResult *r) {
    if (!r) return;
    free(r->buf);
    free(r->meta);
    free(r->sky);
    free(r->blocklight);
    memset(r, 0, sizeof(*r));
}

static void stream_async_reset_event(HANDLE h) {
#if defined(_WIN32)
    if (h) ResetEvent(h);
#elif defined(PEX_PLATFORM_PSP)
    if (h && h->kind == PEX_HANDLE_EVENT) {
        SceUInt t = 0;
        while (sceKernelWaitSema(h->uid, 1, &t) >= 0) { t = 0; }
        h->signaled = 0;
    }
#else
    if (h && h->kind == PEX_HANDLE_EVENT) {
        pthread_mutex_lock(&h->mutex);
        h->signaled = 0;
        pthread_mutex_unlock(&h->mutex);
    }
#endif
}

static int stream_async_cpu_count(void) {
#if defined(_WIN32)
    SYSTEM_INFO si;
    memset(&si, 0, sizeof(si));
    GetSystemInfo(&si);
    if (si.dwNumberOfProcessors > 0) return (int)si.dwNumberOfProcessors;
#endif
    return 2;
}

static int stream_async_choose_worker_count(void) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    return 1;
#else
    int cpu = stream_async_cpu_count();
    if (cpu <= 2) return 1;
    /* World generation is already heavy and the render thread still has to adopt
       meshes while the player walks.  Four generator workers could saturate a
       desktop CPU/GPU driver and show up as sudden frame freezes even though the
       work was "async".  Keep runtime generation conservative; initial preload
       still queues ahead and streams in the background. */
    return 2;
#endif
}

static int stream_async_pop_job(StreamAsyncJob *job) {
    int have = 0;
    EnterCriticalSection(&g_stream_async_cs);
    if (!g_stream_async_stop && g_stream_async_job_count > 0) {
        if (job) *job = g_stream_async_jobs[g_stream_async_job_head];
        memset(&g_stream_async_jobs[g_stream_async_job_head], 0, sizeof(g_stream_async_jobs[0]));
        g_stream_async_job_head = (g_stream_async_job_head + 1) % STREAM_ASYNC_JOB_RING;
        g_stream_async_job_count--;
        g_stream_async_active_count++;
        have = 1;
    } else if (!g_stream_async_stop) {
        stream_async_reset_event(g_stream_async_event);
    }
    LeaveCriticalSection(&g_stream_async_cs);
    return have;
}

static int stream_async_push_result(StreamAsyncResult *r) {
    for (;;) {
        int stop = 0;
        EnterCriticalSection(&g_stream_async_cs);
        stop = g_stream_async_stop;
        if (!stop && g_stream_async_result_count < STREAM_ASYNC_RESULT_RING) {
            g_stream_async_results[g_stream_async_result_tail] = *r;
            memset(r, 0, sizeof(*r));
            g_stream_async_result_tail = (g_stream_async_result_tail + 1) % STREAM_ASYNC_RESULT_RING;
            g_stream_async_result_count++;
            g_stream_async_active_count--;
            LeaveCriticalSection(&g_stream_async_cs);
            return 1;
        }
        if (stop) {
            if (g_stream_async_active_count > 0) g_stream_async_active_count--;
            LeaveCriticalSection(&g_stream_async_cs);
            return 0;
        }
        LeaveCriticalSection(&g_stream_async_cs);
        Sleep(1);
    }
}

static DWORD WINAPI stream_async_worker_proc(LPVOID unused) {
    (void)unused;
    TerrainProvider *worker_tp = NULL;
    int worker_tp_valid = 0;
    long long worker_tp_seed = 0;

    for (;;) {
        StreamAsyncJob job;
        memset(&job, 0, sizeof(job));
        if (!stream_async_pop_job(&job)) {
            EnterCriticalSection(&g_stream_async_cs);
            int stop = g_stream_async_stop;
            LeaveCriticalSection(&g_stream_async_cs);
            if (stop) break;
            WaitForSingleObject(g_stream_async_event, INFINITE);
            continue;
        }

        StreamAsyncResult r;
        memset(&r, 0, sizeof(r));
        r.cx = job.cx;
        r.cz = job.cz;
        r.epoch = job.epoch;
        r.buf = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
        r.meta = (unsigned char*)calloc(FLAT_CHUNK_BLOCK_COUNT, 1);
        r.sky = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
        r.blocklight = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);

        if (r.buf && r.meta && r.sky && r.blocklight) {
            TerrainProvider *reuse = NULL;
            if (job.type == 1) {
                if (!worker_tp_valid || worker_tp_seed != job.seed) {
                    if (worker_tp_valid && worker_tp) {
                        terrain_provider_free(worker_tp);
                        free(worker_tp);
                        worker_tp = NULL;
                    }
                    worker_tp = (TerrainProvider*)calloc(1, sizeof(*worker_tp));
                    if (worker_tp) {
                        terrain_provider_init(worker_tp, (int64_t)job.seed);
                        worker_tp_seed = job.seed;
                        worker_tp_valid = 1;
                    } else {
                        worker_tp_valid = 0;
                    }
                }
                reuse = worker_tp_valid ? worker_tp : NULL;
            }
            flat_generate_chunk_base_with_meta(job.cx, job.cz, r.buf, r.meta, job.type, job.seed, reuse);
            flat_load_chunk_delta_for_dir(job.world_dir, job.cx, job.cz, r.buf, r.meta);
            flat_compute_chunk_light(r.buf, r.sky, r.blocklight);
        } else {
            stream_async_free_result_payload(&r);
        }

        if (!r.buf) {
            EnterCriticalSection(&g_stream_async_cs);
            if (g_stream_async_active_count > 0) g_stream_async_active_count--;
            LeaveCriticalSection(&g_stream_async_cs);
            stream_async_free_result_payload(&r);
            continue;
        }
        if (!stream_async_push_result(&r)) {
            stream_async_free_result_payload(&r);
        }
    }

    if (worker_tp_valid && worker_tp) {
        terrain_provider_free(worker_tp);
        free(worker_tp);
    }
    return 0;
}

static void stream_async_init(void) {
    if (g_stream_async_initialized) return;
#if defined(PEX_PLATFORM_WII)
    /* Keep the first Wii port single-threaded.  The Win32-shaped pthread shim
       and libogc/Dolphin thread scheduling made world generation crash with
       unknown-pointer exceptions.  The cooperative fallback still generates one
       chunk per tick, just without a worker thread touching worldgen/heap data. */
    g_stream_async_event = NULL;
    g_stream_async_worker_count = 0;
    g_stream_async_initialized = 1;
    wii_debug_logf("stream async disabled on Wii; using cooperative generation");
    return;
#endif
#if defined(PEX_PLATFORM_PSP) && !(defined(PEX_PSP_REAL_BETA_GEN) && PEX_PSP_REAL_BETA_GEN)
    /* Safe PSP terrain uses cooperative generation.  Real Beta mode keeps the
       worker path so the 3x3 canvas generator does not run inside the frame. */
    g_stream_async_event = NULL;
    g_stream_async_worker_count = 0;
    g_stream_async_initialized = 1;
    return;
#endif

    InitializeCriticalSection(&g_stream_async_cs);
    g_stream_async_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    g_stream_async_stop = 0;
    g_stream_async_active_count = 0;
    g_stream_async_job_head = g_stream_async_job_tail = g_stream_async_job_count = 0;
    g_stream_async_result_head = g_stream_async_result_tail = g_stream_async_result_count = 0;
    memset(g_stream_async_threads, 0, sizeof(g_stream_async_threads));

    int desired = stream_async_choose_worker_count();
#if defined(PEX_PLATFORM_PSP)
    if (desired > 1) desired = 1;
#endif
    if (desired > STREAM_ASYNC_MAX_WORKERS) desired = STREAM_ASYNC_MAX_WORKERS;
    for (int i = 0; i < desired; ++i) {
#if defined(PEX_PLATFORM_PSP)
        g_stream_async_threads[i] = CreateThread(NULL, 0x40000, stream_async_worker_proc, NULL, 0, NULL);
#else
        g_stream_async_threads[i] = CreateThread(NULL, 0x400000, stream_async_worker_proc, NULL, 0, NULL);
#endif
        if (g_stream_async_threads[i]) {
#if defined(PEX_PLATFORM_PSP)
            SetThreadPriority(g_stream_async_threads[i], THREAD_PRIORITY_NORMAL);
#else
            SetThreadPriority(g_stream_async_threads[i], THREAD_PRIORITY_BELOW_NORMAL);
#endif
            g_stream_async_worker_count++;
        }
    }
    if (g_stream_async_worker_count > 0) {
        pex_logf("stream async worker pool started workers=%d", g_stream_async_worker_count);
    } else {
        CloseHandle(g_stream_async_event);
        g_stream_async_event = NULL;
        pex_logf("stream async worker pool failed to start; using cooperative streaming");
    }
    g_stream_async_initialized = 1;
}

static int stream_async_pending(void) {
#if defined(PEX_PLATFORM_WII)
    return 0;
#endif
    if (!g_stream_async_initialized) return 0;
#if defined(PEX_PLATFORM_PSP) && !(defined(PEX_PSP_REAL_BETA_GEN) && PEX_PSP_REAL_BETA_GEN)
    if (!g_stream_async_event || g_stream_async_worker_count <= 0) return 0;
#endif
    if (!g_stream_async_event || g_stream_async_worker_count <= 0) return 0;
    int pending = 0;
    EnterCriticalSection(&g_stream_async_cs);
    pending = g_stream_async_job_count || g_stream_async_active_count || g_stream_async_result_count;
    LeaveCriticalSection(&g_stream_async_cs);
    return pending;
}

static void stream_async_submit_next(void) {
    stream_async_init();
    if (!g_stream_async_event || g_stream_async_worker_count <= 0) return;

    int submitted = 0;
    int job_backlog_limit = g_stream_generation_keep_completed ? STREAM_ASYNC_JOB_RING : g_stream_async_worker_count * 2;
    if (job_backlog_limit < g_stream_async_worker_count) job_backlog_limit = g_stream_async_worker_count;
    if (job_backlog_limit > STREAM_ASYNC_JOB_RING) job_backlog_limit = STREAM_ASYNC_JOB_RING;
    EnterCriticalSection(&g_stream_async_cs);
    while (!g_stream_async_stop &&
           g_stream_gen_queue_index < g_stream_gen_queue_count &&
           g_stream_async_job_count < STREAM_ASYNC_JOB_RING &&
           g_stream_async_job_count < job_backlog_limit) {
        StreamAsyncJob *job = &g_stream_async_jobs[g_stream_async_job_tail];
        memset(job, 0, sizeof(*job));
        job->cx = g_stream_gen_queue_cx[g_stream_gen_queue_index];
        job->cz = g_stream_gen_queue_cz[g_stream_gen_queue_index];
        job->type = g_world_type;
        job->seed = g_world_seed;
        job->epoch = g_stream_generation_epoch;
        snprintf(job->world_dir, sizeof(job->world_dir), "%s", g_loaded_world_dir);
        g_stream_async_job_tail = (g_stream_async_job_tail + 1) % STREAM_ASYNC_JOB_RING;
        g_stream_async_job_count++;
        g_stream_gen_queue_index++;
        submitted++;
    }
    LeaveCriticalSection(&g_stream_async_cs);

    if (submitted > 0) SetEvent(g_stream_async_event);
}

static int stream_async_install_ready(int max_install) {
    int installed = 0;
    while (installed < max_install) {
        StreamAsyncResult r;
        memset(&r, 0, sizeof(r));

        EnterCriticalSection(&g_stream_async_cs);
        if (g_stream_async_result_count > 0) {
            r = g_stream_async_results[g_stream_async_result_head];
            memset(&g_stream_async_results[g_stream_async_result_head], 0, sizeof(g_stream_async_results[0]));
            g_stream_async_result_head = (g_stream_async_result_head + 1) % STREAM_ASYNC_RESULT_RING;
            g_stream_async_result_count--;
        }
        LeaveCriticalSection(&g_stream_async_cs);

        if (!r.buf) break;

        if (r.epoch == g_stream_generation_epoch &&
            stream_world_chunk_in_window(r.cx, r.cz, g_flat_world_origin_x, g_flat_world_origin_z)) {
            g_copy_chunk_skip_main_light = (r.sky && r.blocklight) ? 1 : 0;
            flat_copy_chunk_buffers(r.cx, r.cz, r.buf, r.meta);
            g_copy_chunk_skip_main_light = 0;
            if (r.sky && r.blocklight) flat_copy_light_buffers_to_world(r.cx, r.cz, r.sky, r.blocklight);
            stream_mark_chunk_generated(stream_local_chunk_x(r.cx), stream_local_chunk_z(r.cz));
            g_stream_gen_queue_installed_count++;
            installed++;
        }

        stream_async_free_result_payload(&r);
    }
    return installed;
}

static int stream_generation_active(void) {
    int queued = g_stream_gen_queue_count - g_stream_gen_queue_index;
    if (queued < 0) queued = 0;
    int async = stream_async_pending() ? 1 : 0;
    int batching = g_stream_initial_batch_running ? 1 : 0;
    g_prof_stream_pending_last = queued + async + batching;
    return queued > 0 || async || batching;
}

static int stream_world_chunk_in_window(int wcx, int wcz, int origin_x, int origin_z) {
    int wx0 = wcx * FLAT_RENDER_CHUNK;
    int wz0 = wcz * FLAT_RENDER_CHUNK;
    return wx0 >= origin_x && wz0 >= origin_z &&
           wx0 + FLAT_RENDER_CHUNK <= origin_x + FLAT_WORLD_SIZE &&
           wz0 + FLAT_RENDER_CHUNK <= origin_z + FLAT_WORLD_SIZE;
}

static PexRendererBackend *stream_direct_mesh_backend(void) {
#if defined(PEX_PLATFORM_WII)
    return wii_gx_get_backend();
#else
    if (g_runtime_renderer_backend == RENDERER_D3D9) return renderer_d3d9_get_backend();
    if (g_runtime_renderer_backend == RENDERER_D3D11) return renderer_d3d11_get_backend();
    return NULL;
#endif
}

static void stream_destroy_mesh_handle(unsigned int h) {
    (void)h;
    /* World streaming now runs off the game/render thread.  Do not destroy GPU
       mesh handles from the streaming service thread; the remap path reuses or
       invalidates the section slots and lets the normal render-owned rebuild path
       replace live handles safely. */
}

static void stream_remap_render_chunks(int old_origin_x, int old_origin_z,
                                                    int new_origin_x, int new_origin_z) {
    pex_logf_trace("chunk render remap origin old=%d,%d new=%d,%d", old_origin_x, old_origin_z, new_origin_x, new_origin_z);
    GLuint old_lists[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    GLuint old_liquid_lists[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_dirty[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_valid[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_has_liquid[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_generated[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_light_ready[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    unsigned int old_light_versions[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_initial_preload[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_modified[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_light_due[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    unsigned short old_section_masks[FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    GLuint old_section_lists[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS][2];
    int old_section_dirty[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    unsigned int old_section_versions[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    unsigned int old_section_light_versions[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_section_valid[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS];
    int old_section_skip[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS][2];
    unsigned int old_section_direct_mesh[FLAT_RENDER_SECTIONS_Y][FLAT_RENDER_CHUNKS][FLAT_RENDER_CHUNKS][2];
    GLuint reusable_section_lists[FLAT_RENDER_SECTIONS_Y * FLAT_RENDER_CHUNKS * FLAT_RENDER_CHUNKS][2];
    int reusable_section_count = 0;
    int reusable_section_index = 0;
    GLuint reusable_lists[STREAM_GEN_QUEUE_MAX];
    GLuint reusable_liquid_lists[STREAM_GEN_QUEUE_MAX];
    int reusable_count = 0;
    int reusable_index = 0;

    memcpy(old_lists, g_flat_world_chunk_lists, sizeof(old_lists));
    memcpy(old_liquid_lists, g_flat_world_chunk_liquid_lists, sizeof(old_liquid_lists));
    memcpy(old_dirty, g_flat_world_chunk_dirty, sizeof(old_dirty));
    memcpy(old_valid, g_flat_world_chunk_valid, sizeof(old_valid));
    memcpy(old_has_liquid, g_flat_world_chunk_has_liquid, sizeof(old_has_liquid));
    memcpy(old_generated, g_flat_world_chunk_generated, sizeof(old_generated));
    memcpy(old_light_ready, g_flat_chunk_light_ready, sizeof(old_light_ready));
    memcpy(old_light_versions, g_flat_chunk_light_version, sizeof(old_light_versions));
    memcpy(old_initial_preload, g_flat_chunk_initial_preload, sizeof(old_initial_preload));
    memcpy(old_modified, g_flat_world_chunk_modified, sizeof(old_modified));
    memcpy(old_light_due, g_flat_chunk_environment_light_due, sizeof(old_light_due));
    memcpy(old_section_masks, g_flat_chunk_section_non_empty_mask, sizeof(old_section_masks));
    memcpy(old_section_lists, g_flat_section_lists, sizeof(old_section_lists));
    memcpy(old_section_dirty, g_flat_section_dirty, sizeof(old_section_dirty));
    memcpy(old_section_versions, g_flat_section_mesh_version, sizeof(old_section_versions));
    memcpy(old_section_light_versions, g_flat_section_mesh_light_version, sizeof(old_section_light_versions));
    memcpy(old_section_valid, g_flat_section_valid, sizeof(old_section_valid));
    memcpy(old_section_skip, g_flat_section_skip_pass, sizeof(old_section_skip));
    memcpy(old_section_direct_mesh, g_flat_section_direct_mesh, sizeof(old_section_direct_mesh));

    int old_base_cx = floor_div16(old_origin_x);
    int old_base_cz = floor_div16(old_origin_z);
    int new_base_cx = floor_div16(new_origin_x);
    int new_base_cz = floor_div16(new_origin_z);

    for (int ocz = 0; ocz < FLAT_RENDER_CHUNKS; ocz++) {
        for (int ocx = 0; ocx < FLAT_RENDER_CHUNKS; ocx++) {
            int wcx = old_base_cx + ocx;
            int wcz = old_base_cz + ocz;
            if (!stream_world_chunk_in_window(wcx, wcz, new_origin_x, new_origin_z)) {
                reusable_lists[reusable_count] = old_lists[ocz][ocx];
                reusable_liquid_lists[reusable_count] = old_liquid_lists[ocz][ocx];
                reusable_count++;
                for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
                    if (old_section_lists[sy][ocz][ocx][0] != 0) {
                        reusable_section_lists[reusable_section_count][0] = old_section_lists[sy][ocz][ocx][0];
                        reusable_section_lists[reusable_section_count][1] = old_section_lists[sy][ocz][ocx][1];
                        reusable_section_count++;
                    }
                    stream_destroy_mesh_handle(old_section_direct_mesh[sy][ocz][ocx][0]);
                    stream_destroy_mesh_handle(old_section_direct_mesh[sy][ocz][ocx][1]);
                }
            }
        }
    }

    memset(g_flat_world_chunk_lists, 0, sizeof(g_flat_world_chunk_lists));
    memset(g_flat_world_chunk_liquid_lists, 0, sizeof(g_flat_world_chunk_liquid_lists));
    memset(g_flat_world_chunk_valid, 0, sizeof(g_flat_world_chunk_valid));
    memset(g_flat_world_chunk_has_liquid, 0, sizeof(g_flat_world_chunk_has_liquid));
    memset(g_flat_world_chunk_generated, 0, sizeof(g_flat_world_chunk_generated));
    memset(g_flat_chunk_light_ready, 0, sizeof(g_flat_chunk_light_ready));
    memset(g_flat_chunk_light_version, 0, sizeof(g_flat_chunk_light_version));
    memset(g_flat_chunk_initial_preload, 0, sizeof(g_flat_chunk_initial_preload));
    memset(g_flat_world_chunk_modified, 0, sizeof(g_flat_world_chunk_modified));
    memset(g_flat_chunk_environment_light_due, 0, sizeof(g_flat_chunk_environment_light_due));
    memset(g_flat_chunk_section_non_empty_mask, 0, sizeof(g_flat_chunk_section_non_empty_mask));
    memset(g_flat_section_lists, 0, sizeof(g_flat_section_lists));
    memset(g_flat_section_direct_mesh, 0, sizeof(g_flat_section_direct_mesh));
    memset(g_flat_section_dirty, 0, sizeof(g_flat_section_dirty));
    memset(g_flat_section_mesh_building, 0, sizeof(g_flat_section_mesh_building));
    memset(g_flat_section_mesh_light_version, 0, sizeof(g_flat_section_mesh_light_version));
    memset(g_flat_section_valid, 0, sizeof(g_flat_section_valid));
    for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
        for (int cz = 0; cz < FLAT_RENDER_CHUNKS; cz++) {
            for (int cx = 0; cx < FLAT_RENDER_CHUNKS; cx++) {
                g_flat_section_skip_pass[sy][cz][cx][0] = 1;
                g_flat_section_skip_pass[sy][cz][cx][1] = 1;
            }
        }
    }
    for (int ncz = 0; ncz < FLAT_RENDER_CHUNKS; ncz++) {
        for (int ncx = 0; ncx < FLAT_RENDER_CHUNKS; ncx++) {
            int wcx = new_base_cx + ncx;
            int wcz = new_base_cz + ncz;
            if (stream_world_chunk_in_window(wcx, wcz, old_origin_x, old_origin_z)) {
                int ocx = wcx - old_base_cx;
                int ocz = wcz - old_base_cz;
                g_flat_world_chunk_lists[ncz][ncx] = old_lists[ocz][ocx];
                g_flat_world_chunk_liquid_lists[ncz][ncx] = old_liquid_lists[ocz][ocx];
                g_flat_world_chunk_dirty[ncz][ncx] = old_dirty[ocz][ocx];
                g_flat_world_chunk_valid[ncz][ncx] = old_valid[ocz][ocx];
                g_flat_world_chunk_has_liquid[ncz][ncx] = old_has_liquid[ocz][ocx];
                g_flat_world_chunk_generated[ncz][ncx] = old_generated[ocz][ocx];
                g_flat_chunk_light_ready[ncz][ncx] = old_light_ready[ocz][ocx];
                g_flat_chunk_light_version[ncz][ncx] = old_light_versions[ocz][ocx];
                g_flat_chunk_initial_preload[ncz][ncx] = old_initial_preload[ocz][ocx];
                g_flat_world_chunk_modified[ncz][ncx] = old_modified[ocz][ocx];
                g_flat_chunk_environment_light_due[ncz][ncx] = old_light_due[ocz][ocx];
                g_flat_chunk_section_non_empty_mask[ncz][ncx] = old_section_masks[ocz][ocx];
            } else {
                if (reusable_index < reusable_count) {
                    g_flat_world_chunk_lists[ncz][ncx] = reusable_lists[reusable_index];
                    g_flat_world_chunk_liquid_lists[ncz][ncx] = reusable_liquid_lists[reusable_index];
                    reusable_index++;
                } else {
                    g_flat_world_chunk_lists[ncz][ncx] = 0;
                    g_flat_world_chunk_liquid_lists[ncz][ncx] = 0;
                }
                g_flat_world_chunk_dirty[ncz][ncx] = 1;
                g_flat_world_chunk_valid[ncz][ncx] = 0;
                g_flat_world_chunk_has_liquid[ncz][ncx] = 0;
                g_flat_world_chunk_generated[ncz][ncx] = 0;
                g_flat_chunk_light_ready[ncz][ncx] = 0;
                g_flat_chunk_light_version[ncz][ncx] = 0;
                g_flat_chunk_initial_preload[ncz][ncx] = 0;
                g_flat_world_chunk_modified[ncz][ncx] = 0;
                g_flat_chunk_environment_light_due[ncz][ncx] = 0;
                g_flat_chunk_section_non_empty_mask[ncz][ncx] = 0;
            }
        }
    }
    for (int ncz = 0; ncz < FLAT_RENDER_CHUNKS; ncz++) {
        for (int ncx = 0; ncx < FLAT_RENDER_CHUNKS; ncx++) {
            int wcx = new_base_cx + ncx;
            int wcz = new_base_cz + ncz;
            if (stream_world_chunk_in_window(wcx, wcz, old_origin_x, old_origin_z)) {
                int ocx = wcx - old_base_cx;
                int ocz = wcz - old_base_cz;
                for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
                    g_flat_section_lists[sy][ncz][ncx][0] = old_section_lists[sy][ocz][ocx][0];
                    g_flat_section_lists[sy][ncz][ncx][1] = old_section_lists[sy][ocz][ocx][1];
                    g_flat_section_direct_mesh[sy][ncz][ncx][0] = old_section_direct_mesh[sy][ocz][ocx][0];
                    g_flat_section_direct_mesh[sy][ncz][ncx][1] = old_section_direct_mesh[sy][ocz][ocx][1];
                    g_flat_section_dirty[sy][ncz][ncx] = old_section_dirty[sy][ocz][ocx];
                    g_flat_section_mesh_version[sy][ncz][ncx] = old_section_versions[sy][ocz][ocx];
                    g_flat_section_mesh_light_version[sy][ncz][ncx] = old_section_light_versions[sy][ocz][ocx];
                    g_flat_section_mesh_building[sy][ncz][ncx] = 0;
                    g_flat_section_valid[sy][ncz][ncx] = old_section_valid[sy][ocz][ocx];
                    g_flat_section_skip_pass[sy][ncz][ncx][0] = old_section_skip[sy][ocz][ocx][0];
                    g_flat_section_skip_pass[sy][ncz][ncx][1] = old_section_skip[sy][ocz][ocx][1];
                }
            } else {
                for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
                    if (reusable_section_index < reusable_section_count) {
                        g_flat_section_lists[sy][ncz][ncx][0] = reusable_section_lists[reusable_section_index][0];
                        g_flat_section_lists[sy][ncz][ncx][1] = reusable_section_lists[reusable_section_index][1];
                        reusable_section_index++;
                    }
                    g_flat_section_dirty[sy][ncz][ncx] = 1;
                    flat_note_section_mesh_changed(sy, ncz, ncx);
                    g_flat_section_valid[sy][ncz][ncx] = 0;
                    g_flat_section_skip_pass[sy][ncz][ncx][0] = 1;
                    g_flat_section_skip_pass[sy][ncz][ncx][1] = 1;
                }
            }
        }
    }
    /* Render-owned CPU mesh caches are not touched from the streaming thread.
       The next render call sees the origin mismatch and clears/rebuilds them on
       the GL/render thread, avoiding the old ghost-chunk/crash race. */
    g_flat_renderer_sort_dirty = 1;
    g_flat_world_geometry_dirty = 0;
    g_flat_section_geometry_dirty = 0;
}

static void stream_remap_block_storage(int old_origin_x, int old_origin_z,
                                                       int new_origin_x, int new_origin_z) {
    int ox0 = old_origin_x > new_origin_x ? old_origin_x : new_origin_x;
    int oz0 = old_origin_z > new_origin_z ? old_origin_z : new_origin_z;
    int ox1 = (old_origin_x + FLAT_WORLD_SIZE) < (new_origin_x + FLAT_WORLD_SIZE) ?
              (old_origin_x + FLAT_WORLD_SIZE) : (new_origin_x + FLAT_WORLD_SIZE);
    int oz1 = (old_origin_z + FLAT_WORLD_SIZE) < (new_origin_z + FLAT_WORLD_SIZE) ?
              (old_origin_z + FLAT_WORLD_SIZE) : (new_origin_z + FLAT_WORLD_SIZE);
    int row_len = ox1 - ox0;

    if (row_len <= 0 || oz1 <= oz0) {
        memset(g_flat_blocks, 0, sizeof(g_flat_blocks));
        memset(g_flat_meta, 0, sizeof(g_flat_meta));
        memset(g_flat_levels, 0, sizeof(g_flat_levels));
        memset(g_flat_sky_light, 0, sizeof(g_flat_sky_light));
        memset(g_flat_block_light, 0, sizeof(g_flat_block_light));
        memset(g_flat_chunk_environment_light_due, 0, sizeof(g_flat_chunk_environment_light_due));
        return;
    }

    int z_step = (new_origin_z < old_origin_z) ? -1 : 1;
    int z_begin = (z_step > 0) ? oz0 : (oz1 - 1);
    int z_end = (z_step > 0) ? oz1 : (oz0 - 1);
    int old_fx = ox0 - old_origin_x;
    int new_fx = ox0 - new_origin_x;

    for (int y = 0; y < FLAT_WORLD_HEIGHT; y++) {
        for (int z = z_begin; z != z_end; z += z_step) {
            int old_fz = z - old_origin_z;
            int new_fz = z - new_origin_z;
            memmove(&g_flat_blocks[y][new_fz][new_fx], &g_flat_blocks[y][old_fz][old_fx], (size_t)row_len);
            memmove(&g_flat_meta[y][new_fz][new_fx], &g_flat_meta[y][old_fz][old_fx], (size_t)row_len);
            memmove(&g_flat_levels[y][new_fz][new_fx], &g_flat_levels[y][old_fz][old_fx], (size_t)row_len);
            memmove(&g_flat_sky_light[y][new_fz][new_fx], &g_flat_sky_light[y][old_fz][old_fx], (size_t)row_len);
            memmove(&g_flat_block_light[y][new_fz][new_fx], &g_flat_block_light[y][old_fz][old_fx], (size_t)row_len);
        }

        int dz = new_origin_z - old_origin_z;
        if (dz > 0) {
            int n = dz;
            if (n > FLAT_WORLD_SIZE) n = FLAT_WORLD_SIZE;
            memset(&g_flat_blocks[y][FLAT_WORLD_SIZE - n][0], 0, (size_t)n * FLAT_WORLD_SIZE);
            memset(&g_flat_meta[y][FLAT_WORLD_SIZE - n][0], 0, (size_t)n * FLAT_WORLD_SIZE);
            memset(&g_flat_levels[y][FLAT_WORLD_SIZE - n][0], 0, (size_t)n * FLAT_WORLD_SIZE);
            memset(&g_flat_sky_light[y][FLAT_WORLD_SIZE - n][0], 0, (size_t)n * FLAT_WORLD_SIZE);
            memset(&g_flat_block_light[y][FLAT_WORLD_SIZE - n][0], 0, (size_t)n * FLAT_WORLD_SIZE);
        } else if (dz < 0) {
            int n = -dz;
            if (n > FLAT_WORLD_SIZE) n = FLAT_WORLD_SIZE;
            memset(&g_flat_blocks[y][0][0], 0, (size_t)n * FLAT_WORLD_SIZE);
            memset(&g_flat_meta[y][0][0], 0, (size_t)n * FLAT_WORLD_SIZE);
            memset(&g_flat_levels[y][0][0], 0, (size_t)n * FLAT_WORLD_SIZE);
            memset(&g_flat_sky_light[y][0][0], 0, (size_t)n * FLAT_WORLD_SIZE);
            memset(&g_flat_block_light[y][0][0], 0, (size_t)n * FLAT_WORLD_SIZE);
        }

        int dx = new_origin_x - old_origin_x;
        if (dx > 0) {
            int n = dx;
            if (n > FLAT_WORLD_SIZE) n = FLAT_WORLD_SIZE;
            int x0 = FLAT_WORLD_SIZE - n;
            for (int z = 0; z < FLAT_WORLD_SIZE; z++) {
                memset(&g_flat_blocks[y][z][x0], 0, (size_t)n);
                memset(&g_flat_meta[y][z][x0], 0, (size_t)n);
                memset(&g_flat_levels[y][z][x0], 0, (size_t)n);
                memset(&g_flat_sky_light[y][z][x0], 0, (size_t)n);
                memset(&g_flat_block_light[y][z][x0], 0, (size_t)n);
            }
        } else if (dx < 0) {
            int n = -dx;
            if (n > FLAT_WORLD_SIZE) n = FLAT_WORLD_SIZE;
            for (int z = 0; z < FLAT_WORLD_SIZE; z++) {
                memset(&g_flat_blocks[y][z][0], 0, (size_t)n);
                memset(&g_flat_meta[y][z][0], 0, (size_t)n);
                memset(&g_flat_levels[y][z][0], 0, (size_t)n);
                memset(&g_flat_sky_light[y][z][0], 0, (size_t)n);
                memset(&g_flat_block_light[y][z][0], 0, (size_t)n);
            }
        }
    }
}

static int stream_local_chunk_x(int wcx) {
    return (wcx * FLAT_RENDER_CHUNK - g_flat_world_origin_x) / FLAT_RENDER_CHUNK;
}

static int stream_local_chunk_z(int wcz) {
    return (wcz * FLAT_RENDER_CHUNK - g_flat_world_origin_z) / FLAT_RENDER_CHUNK;
}

static void stream_mark_chunk_dirty(int lcx, int lcz) {
    if (lcx < 0 || lcx >= FLAT_RENDER_CHUNKS || lcz < 0 || lcz >= FLAT_RENDER_CHUNKS) return;
    for (int dz = -1; dz <= 1; dz++) {
        for (int dx = -1; dx <= 1; dx++) {
            int cx = lcx + dx;
            int cz = lcz + dz;
            if (cx < 0 || cx >= FLAT_RENDER_CHUNKS || cz < 0 || cz >= FLAT_RENDER_CHUNKS) continue;
            g_flat_world_chunk_dirty[cz][cx] = 1;
            for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; sy++) {
                /* Do not invalidate an existing mesh here.  The rebuild path will
                   atomically replace it after the new section mesh has uploaded. */
                g_flat_section_dirty[sy][cz][cx] = 1;
                flat_note_section_mesh_changed(sy, cz, cx);
            }
        }
    }
}


static int stream_queue_contains_chunk(int wcx, int wcz) {
    for (int i = g_stream_gen_queue_index; i < g_stream_gen_queue_count; i++) {
        if (g_stream_gen_queue_cx[i] == wcx && g_stream_gen_queue_cz[i] == wcz) return 1;
    }
    return 0;
}

static void stream_queue_add_chunk(int wcx, int wcz) {
    if (g_stream_gen_queue_count >= STREAM_GEN_QUEUE_MAX) return;
    if (!stream_world_chunk_in_window(wcx, wcz, g_flat_world_origin_x, g_flat_world_origin_z)) return;
    int lcx = stream_local_chunk_x(wcx);
    int lcz = stream_local_chunk_z(wcz);
    if (lcx >= 0 && lcx < FLAT_RENDER_CHUNKS && lcz >= 0 && lcz < FLAT_RENDER_CHUNKS &&
        g_flat_world_chunk_generated[lcz][lcx]) return;
    if (stream_queue_contains_chunk(wcx, wcz)) return;
    g_stream_gen_queue_cx[g_stream_gen_queue_count] = wcx;
    g_stream_gen_queue_cz[g_stream_gen_queue_count] = wcz;
    g_stream_gen_queue_count++;
}

static int stream_round_nearest_int(float v) {
    return (int)(v >= 0.0f ? floorf(v + 0.5f) : ceilf(v - 0.5f));
}

static void stream_player_chunk_direction(float *out_dx, float *out_dz) {
    float dx = g_player_motion_x;
    float dz = g_player_motion_z;
    float len = sqrtf(dx * dx + dz * dz);
    if (len < 0.002f) {
        float yaw_rad = g_player_yaw * (float)M_PI / 180.0f;
        dx = -sinf(yaw_rad);
        dz = cosf(yaw_rad);
        len = sqrtf(dx * dx + dz * dz);
    }
    if (len < 0.0001f) {
        dx = 0.0f;
        dz = 1.0f;
        len = 1.0f;
    }
    *out_dx = dx / len;
    *out_dz = dz / len;
}

static void stream_queue_chunk_safety(int base_cx, int base_cz, int pcx, int pcz, int radius) {
    if (radius < 2) radius = 2;
    int ahead = radius;
    if (ahead > 8) ahead = 8;
    int width = 2;
    if (radius <= 2) width = 1;

    float dir_x = 0.0f, dir_z = 1.0f;
    stream_player_chunk_direction(&dir_x, &dir_z);
    float side_x = -dir_z;
    float side_z = dir_x;

    /* Queue a small forward corridor before the normal ring fill.  This does not
       block gameplay; it only changes async priority so walking forward consumes
       generated terrain instead of arriving at empty chunks. */
    for (int step = 0; step <= ahead; step++) {
        for (int side = -width; side <= width; side++) {
            int lcx = pcx + stream_round_nearest_int(dir_x * (float)step + side_x * (float)side);
            int lcz = pcz + stream_round_nearest_int(dir_z * (float)step + side_z * (float)side);
            if (lcx < 0 || lcx >= FLAT_RENDER_CHUNKS || lcz < 0 || lcz >= FLAT_RENDER_CHUNKS) continue;
            stream_queue_add_chunk(base_cx + lcx, base_cz + lcz);
        }
    }

    /* The immediate 3x3 footprint is always queued as a safety net even if the
       direction estimate changes suddenly due to keyboard input or knockback. */
    for (int dz = -1; dz <= 1; dz++) {
        for (int dx = -1; dx <= 1; dx++) {
            int lcx = pcx + dx;
            int lcz = pcz + dz;
            if (lcx < 0 || lcx >= FLAT_RENDER_CHUNKS || lcz < 0 || lcz >= FLAT_RENDER_CHUNKS) continue;
            stream_queue_add_chunk(base_cx + lcx, base_cz + lcz);
        }
    }
}

static int stream_chunk_priority_score(int wcx, int wcz) {
    int base_cx = floor_div16(g_flat_world_origin_x);
    int base_cz = floor_div16(g_flat_world_origin_z);
    int pcx = stream_local_chunk_x(floor_div16((int)floorf(g_player_x)));
    int pcz = stream_local_chunk_z(floor_div16((int)floorf(g_player_z)));
    if (pcx < 0) pcx = 0;
    if (pcz < 0) pcz = 0;
    if (pcx >= FLAT_RENDER_CHUNKS) pcx = FLAT_RENDER_CHUNKS - 1;
    if (pcz >= FLAT_RENDER_CHUNKS) pcz = FLAT_RENDER_CHUNKS - 1;

    float dir_x = 0.0f, dir_z = 1.0f;
    stream_player_chunk_direction(&dir_x, &dir_z);
    float side_x = -dir_z;
    float side_z = dir_x;

    float vx = (float)((wcx - base_cx) - pcx);
    float vz = (float)((wcz - base_cz) - pcz);
    float ahead = vx * dir_x + vz * dir_z;
    float side = fabsf(vx * side_x + vz * side_z);
    float dist2 = vx * vx + vz * vz;

    int score = (int)(dist2 * 32.0f + side * 10.0f - ahead * 24.0f);
    if (ahead < -0.25f) score += 512;
    return score;
}

static void stream_reprioritize_queue(void) {
    int begin = g_stream_gen_queue_index;
    if (begin < 0) begin = 0;
    if (begin + 1 >= g_stream_gen_queue_count) return;

    for (int i = begin + 1; i < g_stream_gen_queue_count; i++) {
        int cx = g_stream_gen_queue_cx[i];
        int cz = g_stream_gen_queue_cz[i];
        int score = stream_chunk_priority_score(cx, cz);
        int j = i - 1;
        while (j >= begin && stream_chunk_priority_score(g_stream_gen_queue_cx[j], g_stream_gen_queue_cz[j]) > score) {
            g_stream_gen_queue_cx[j + 1] = g_stream_gen_queue_cx[j];
            g_stream_gen_queue_cz[j + 1] = g_stream_gen_queue_cz[j];
            j--;
        }
        g_stream_gen_queue_cx[j + 1] = cx;
        g_stream_gen_queue_cz[j + 1] = cz;
    }
}

static int stream_queue_visible_chunks(void) {
    stream_generation_queue_clear();

    int base_cx = floor_div16(g_flat_world_origin_x);
    int base_cz = floor_div16(g_flat_world_origin_z);
    int pcx = stream_local_chunk_x(floor_div16((int)floorf(g_player_x)));
    int pcz = stream_local_chunk_z(floor_div16((int)floorf(g_player_z)));
    if (pcx < 0) pcx = 0;
    if (pcz < 0) pcz = 0;
    if (pcx >= FLAT_RENDER_CHUNKS) pcx = FLAT_RENDER_CHUNKS - 1;
    if (pcz >= FLAT_RENDER_CHUNKS) pcz = FLAT_RENDER_CHUNKS - 1;

    int radius = stream_render_radius();
    stream_queue_chunk_safety(base_cx, base_cz, pcx, pcz, radius);
    for (int ring = 0; ring <= radius; ring++) {
        int before = g_stream_gen_queue_count;
        for (int dz = -ring; dz <= ring; dz++) {
            for (int dx = -ring; dx <= ring; dx++) {
                if (ring != 0 && abs(dx) != ring && abs(dz) != ring) continue;
                int lcx = pcx + dx;
                int lcz = pcz + dz;
                if (lcx < 0 || lcx >= FLAT_RENDER_CHUNKS || lcz < 0 || lcz >= FLAT_RENDER_CHUNKS) continue;
                if (g_flat_world_chunk_generated[lcz][lcx]) continue;
                stream_queue_add_chunk(base_cx + lcx, base_cz + lcz);
            }
        }
        /* Queue every missing visible ring now that chunk generation, full
           light propagation, and section meshing have independent workers.
           The old one-ring queue left high render-distance worlds with visible
           holes until the whole previous ring completed. */
        (void)before;
    }
    return g_stream_gen_queue_count > 0;
}

static void stream_queue_missing_chunks(int old_origin_x, int old_origin_z) {
    stream_generation_queue_clear();

    int pcx = stream_local_chunk_x(floor_div16((int)floorf(g_player_x)));
    int pcz = stream_local_chunk_z(floor_div16((int)floorf(g_player_z)));
    if (pcx < 0) pcx = 0;
    if (pcz < 0) pcz = 0;
    if (pcx >= FLAT_RENDER_CHUNKS) pcx = FLAT_RENDER_CHUNKS - 1;
    if (pcz >= FLAT_RENDER_CHUNKS) pcz = FLAT_RENDER_CHUNKS - 1;

    int base_cx = floor_div16(g_flat_world_origin_x);
    int base_cz = floor_div16(g_flat_world_origin_z);
    stream_queue_chunk_safety(base_cx, base_cz, pcx, pcz, stream_render_radius());

    /* Queue only chunks that are newly exposed by the window shift.  The old
       version regenerated the entire 256x256 window synchronously here, which
       is exactly the hitch seen when the chat says "Generated new terrain
       chunks."  Ring order makes the nearest missing terrain appear first. */
    for (int ring = 0; ring < FLAT_RENDER_CHUNKS; ring++) {
        for (int dz = -ring; dz <= ring; dz++) {
            for (int dx = -ring; dx <= ring; dx++) {
                if (ring != 0 && abs(dx) != ring && abs(dz) != ring) continue;
                int lcx = pcx + dx;
                int lcz = pcz + dz;
                if (lcx < 0 || lcx >= FLAT_RENDER_CHUNKS || lcz < 0 || lcz >= FLAT_RENDER_CHUNKS) continue;
                int wcx = base_cx + lcx;
                int wcz = base_cz + lcz;
                if (stream_world_chunk_in_window(wcx, wcz, old_origin_x, old_origin_z)) continue;
                stream_queue_add_chunk(wcx, wcz);
            }
        }
    }
}

static int stream_generate_initial_batch(void) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    return 0;
#else
    StreamInitialBetaBatchState *s = &g_stream_initial_beta_batch;

    if (!g_stream_generation_keep_completed || g_world_type != 1 || g_stream_gen_queue_count <= 0) {
        if (s->active) stream_reset_initial_batch();
        return 0;
    }
    if (!s->active && (g_stream_gen_queue_index != 0 || g_stream_gen_queue_installed_count != 0)) return 0;

    if (!s->active) {
        int min_cx = g_stream_gen_queue_cx[0], max_cx = g_stream_gen_queue_cx[0];
        int min_cz = g_stream_gen_queue_cz[0], max_cz = g_stream_gen_queue_cz[0];
        for (int i = 1; i < g_stream_gen_queue_count; ++i) {
            if (g_stream_gen_queue_cx[i] < min_cx) min_cx = g_stream_gen_queue_cx[i];
            if (g_stream_gen_queue_cx[i] > max_cx) max_cx = g_stream_gen_queue_cx[i];
            if (g_stream_gen_queue_cz[i] < min_cz) min_cz = g_stream_gen_queue_cz[i];
            if (g_stream_gen_queue_cz[i] > max_cz) max_cz = g_stream_gen_queue_cz[i];
        }

        int chunks_x = max_cx - min_cx + 1;
        int chunks_z = max_cz - min_cz + 1;
        int canvas_chunks = (chunks_x > chunks_z ? chunks_x : chunks_z) + 2;
        if (canvas_chunks < 3 || canvas_chunks > FLAT_RENDER_CHUNKS + 2) return 0;

        memset(s, 0, sizeof(*s));
        s->min_cx = min_cx; s->min_cz = min_cz;
        s->max_cx = max_cx; s->max_cz = max_cz;
        s->chunks_x = chunks_x; s->chunks_z = chunks_z;
        s->canvas_chunks = canvas_chunks;
        s->tp = (TerrainProvider*)calloc(1, sizeof(*s->tp));
        s->beta_blocks = (unsigned char*)calloc(32768u, 1);
        s->beta_data = (unsigned char*)calloc(16384u, 1);
        s->buf = (unsigned char*)malloc(FLAT_CHUNK_BLOCK_COUNT);
        s->meta = (unsigned char*)calloc(FLAT_CHUNK_BLOCK_COUNT, 1);
        s->cv.minCx = min_cx - 1;
        s->cv.minCz = min_cz - 1;
        s->cv.chunks = canvas_chunks;
        s->cv.blocks = (unsigned char*)calloc((size_t)s->cv.chunks * (size_t)s->cv.chunks * 32768u, 1);
        s->cv.meta = s->cv.blocks ? (unsigned char*)calloc((size_t)s->cv.chunks * (size_t)s->cv.chunks * 32768u, 1) : NULL;

        if (!s->tp || !s->beta_blocks || !s->beta_data || !s->buf || !s->meta || !s->cv.blocks || !s->cv.meta) {
            stream_reset_initial_batch();
            return 0;
        }

        terrain_provider_init(s->tp, (int64_t)g_world_seed);
        s->active = 1;
        s->phase = 0;
        g_stream_initial_batch_running = 1;
        g_stream_initial_batch_done_units = 0;
        g_stream_initial_batch_total_units =
            s->canvas_chunks * s->canvas_chunks +
            (s->chunks_x + 1) * (s->chunks_z + 1) +
            g_stream_gen_queue_count;
        if (g_stream_initial_batch_total_units <= 0) g_stream_initial_batch_total_units = g_stream_gen_queue_count;
    }

    /* Keep this service thread cooperative.  Java 1.2.5 repaints the loading
       screen inside preloadWorld(); here the work is off the render thread, but a
       single unbroken Beta canvas build still made 0/N sit for seconds and heated
       small laptops.  Do a small time-sliced amount of canvas/populate/extract
       work per service wake. */
    const double deadline = now_seconds() + 0.0040;
    int did_work = 0;

    while (now_seconds() < deadline) {
        if (s->phase == 0) {
            int total = s->canvas_chunks * s->canvas_chunks;
            if (s->gen_index >= total) {
                s->phase = 1;
                continue;
            }
            int lx = s->gen_index % s->canvas_chunks;
            int lz = s->gen_index / s->canvas_chunks;
            generate_canvas_chunk(s->tp, &s->cv, s->cv.minCx + lx, s->cv.minCz + lz);
            s->gen_index++;
            g_stream_initial_batch_done_units++;
            did_work = 1;
        } else if (s->phase == 1) {
            int pop_w = s->chunks_x + 1;
            int pop_h = s->chunks_z + 1;
            int total = pop_w * pop_h;
            if (s->populate_index >= total) {
                s->phase = 2;
                continue;
            }
            int lx = s->populate_index % pop_w;
            int lz = s->populate_index / pop_w;
            qm_populate_canvas(s->tp, &s->cv, s->min_cx - 1 + lx, s->min_cz - 1 + lz);
            s->populate_index++;
            g_stream_initial_batch_done_units++;
            did_work = 1;
        } else if (s->phase == 2) {
            if (s->extract_index >= g_stream_gen_queue_count) {
                g_stream_gen_queue_index = g_stream_gen_queue_count;
                g_stream_gen_queue_installed_count = g_stream_gen_queue_count;
                g_stream_initial_batch_done_units = g_stream_initial_batch_total_units;
                stream_initial_beta_batch_free();
                return 1;
            }

            int i = s->extract_index++;
            int wcx = g_stream_gen_queue_cx[i];
            int wcz = g_stream_gen_queue_cz[i];
            if (!stream_world_chunk_in_window(wcx, wcz, g_flat_world_origin_x, g_flat_world_origin_z)) {
                g_stream_gen_queue_index = i + 1;
                g_stream_initial_batch_done_units++;
                did_work = 1;
                continue;
            }

            int lcx = stream_local_chunk_x(wcx);
            int lcz = stream_local_chunk_z(wcz);
            if (flat_local_chunk_valid(lcx, lcz) && g_flat_world_chunk_generated[lcz][lcx]) {
                g_stream_gen_queue_index = i + 1;
                g_stream_gen_queue_installed_count++;
                g_stream_initial_batch_done_units++;
                did_work = 1;
                continue;
            }

            memset(s->beta_blocks, 0, 32768u);
            memset(s->beta_data, 0, 16384u);
            memset(s->buf, 0, FLAT_CHUNK_BLOCK_COUNT);
            memset(s->meta, 0, FLAT_CHUNK_BLOCK_COUNT);
            extract_canvas_chunk(&s->cv, wcx, wcz, s->beta_blocks, s->beta_data);
            for (int lx = 0; lx < 16; ++lx) {
                for (int lz = 0; lz < 16; ++lz) {
                    for (int y = FLAT_WORLD_Y_MIN; y <= FLAT_WORLD_Y_MAX; ++y) {
                        int id = (y < 128) ? get_block_local(s->beta_blocks, lx, y, lz) : 0;
                        int bi = flat_chunk_buf_index(lx, y, lz);
                        s->buf[bi] = (unsigned char)id;
                        if (y < 128) {
                            int idx = chunk_index(lx, y, lz);
                            unsigned char packed = s->beta_data[idx >> 1];
                            s->meta[bi] = (unsigned char)((idx & 1) ? ((packed >> 4) & 15) : (packed & 15));
                        }
                    }
                    if (s->buf[flat_chunk_buf_index(lx, 0, lz)] == 0) {
                        s->buf[flat_chunk_buf_index(lx, 0, lz)] = BLOCK_BEDROCK;
                    }
                }
            }
            flat_load_chunk_delta_for_dir(g_loaded_world_dir, wcx, wcz, s->buf, s->meta);

            g_copy_chunk_skip_main_light = 1;
            flat_copy_chunk_buffers(wcx, wcz, s->buf, s->meta);
            g_copy_chunk_skip_main_light = 0;
            stream_mark_chunk_generated(lcx, lcz);

            g_stream_gen_queue_index = i + 1;
            g_stream_gen_queue_installed_count++;
            g_stream_initial_batch_done_units++;
            did_work = 1;
        } else {
            stream_reset_initial_batch();
            return 0;
        }

        /* Generation and population are the hot part.  Limit them to one unit per
           wake on light CPUs; extraction is cheaper and can continue until the
           deadline. */
        if (did_work && (s->phase == 0 || s->phase == 1)) break;
    }

    return did_work ? 1 : 0;
#endif
}

static void process_stream_generation_queue(void) {
    if (stream_generate_initial_batch()) return;

    stream_async_init();
    stream_reprioritize_queue();

    /* If thread creation fails/is disabled, keep the world functional with a
       small synchronous budget rather than leaving missing terrain forever. */
    if (!g_stream_async_event || g_stream_async_worker_count <= 0) {
        int budget = STREAM_CHUNKS_PER_TICK;
        while (budget-- > 0 && g_stream_gen_queue_index < g_stream_gen_queue_count) {
            int idx = g_stream_gen_queue_index++;
            int wcx = g_stream_gen_queue_cx[idx];
            int wcz = g_stream_gen_queue_cz[idx];
#if defined(PEX_PLATFORM_WII)
            wii_debug_logf("stream coop chunk %d/%d wc=%d,%d type=%d", idx + 1, g_stream_gen_queue_count, wcx, wcz, g_world_type);
#endif
            pex_logf_trace("chunk stream sync install index=%d/%d world=%d,%d", idx + 1, g_stream_gen_queue_count, wcx, wcz);
            beta_preview_copy_chunk_to_flat(wcx, wcz);
            stream_mark_chunk_generated(stream_local_chunk_x(wcx), stream_local_chunk_z(wcz));
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_FAST_WORLD) && PEX_PSP_FAST_WORLD
            psp_fast_surface_mark_dirty(wcx * 16 + 8, wcz * 16 + 8);
#endif
            g_stream_gen_queue_installed_count++;
        }
        if (!stream_generation_active() && !g_stream_generation_keep_completed) stream_generation_queue_clear();
        return;
    }

    static int s_stream_install_tick = 0;
    /* During the loading screen we care more about entering the world quickly than
       about hiding commit hitches, so install every ready chunk.  After gameplay
       starts, keep the old throttle to avoid frame spikes while walking. */
    int initial_load = g_stream_generation_keep_completed ? 1 : 0;
    int allow_install = 1;
    (void)s_stream_install_tick;
    if (allow_install) {
        int max_install = initial_load ? 16 : 1;
        pex_logf_trace("chunk stream async install tick queue=%d/%d budget=%d", g_stream_gen_queue_index, g_stream_gen_queue_count, max_install);
        stream_async_install_ready(max_install);
    }
    stream_async_submit_next();

    if (!stream_generation_active() && !g_stream_generation_keep_completed) {
        stream_generation_queue_clear();
    }
}




static void flat_run_initial_light_settle(void) {
    if (!g_stream_initial_light_settle_requested || g_stream_initial_light_settle_done) return;
    g_stream_initial_light_settle_requested = 0;
    g_stream_initial_light_settle_running = 1;
    g_stream_initial_light_settle_progress = 1;

    int min_lcx = FLAT_RENDER_CHUNKS, min_lcz = FLAT_RENDER_CHUNKS;
    int max_lcx = -1, max_lcz = -1;
    for (int lcz = 0; lcz < FLAT_RENDER_CHUNKS; ++lcz) {
        for (int lcx = 0; lcx < FLAT_RENDER_CHUNKS; ++lcx) {
            if (!g_flat_world_chunk_generated[lcz][lcx]) continue;
            if (lcx < min_lcx) min_lcx = lcx;
            if (lcz < min_lcz) min_lcz = lcz;
            if (lcx > max_lcx) max_lcx = lcx;
            if (lcz > max_lcz) max_lcz = lcz;
        }
    }

    if (max_lcx >= min_lcx && max_lcz >= min_lcz) {
        int base_cx = floor_div16(g_flat_world_origin_x);
        int base_cz = floor_div16(g_flat_world_origin_z);
        int rx0 = (base_cx + min_lcx) * FLAT_RENDER_CHUNK;
        int rz0 = (base_cz + min_lcz) * FLAT_RENDER_CHUNK;
        int rx1 = (base_cx + max_lcx) * FLAT_RENDER_CHUNK + FLAT_RENDER_CHUNK - 1;
        int rz1 = (base_cz + max_lcz) * FLAT_RENDER_CHUNK + FLAT_RENDER_CHUNK - 1;

        /* Match the important part of Minecraft 1.2.5 preloadWorld(): do not
           enter gameplay until the preload area has had its queued lighting
           settled.  This runs on the stream service, not the main thread, so the
           loading screen can keep repainting while the expensive region pass runs.
           The margin lets skylight/block-light cross the first generated chunk
           border and prevents the "one chunk has a different brightness until I
           edit a block" failure. */
        g_stream_initial_light_settle_progress = 15;
        flat_relight_region(rx0, rz0, rx1, rz1, 16, 1);
        g_stream_initial_light_settle_progress = 90;
        for (int lcz = min_lcz; lcz <= max_lcz; ++lcz) {
            for (int lcx = min_lcx; lcx <= max_lcx; ++lcx) {
                if (!g_flat_world_chunk_generated[lcz][lcx]) continue;
                flat_refresh_chunk_occupancy(lcx, lcz);
                flat_publish_light_ready(lcx, lcz, 1, 1);
                for (int sy = 0; sy < FLAT_RENDER_SECTIONS_Y; ++sy) {
                    flat_mark_generated_section(lcx, lcz, sy);
                }
            }
        }
    }

    g_stream_initial_light_settle_progress = 100;
    g_stream_initial_light_settle_done = 1;
    g_stream_initial_light_settle_running = 0;
}

static CRITICAL_SECTION g_world_stream_service_cs;
static int g_world_stream_service_initialized = 0;
static int g_world_stream_service_stop = 0;
static int g_world_stream_service_busy = 0;
static HANDLE g_world_stream_service_thread = NULL;

static DWORD WINAPI world_stream_service_proc(LPVOID unused) {
    (void)unused;
    g_pex_profile_thread_role = PEX_PROFILE_ROLE_ASYNC_STREAM;
    for (;;) {
        int stop = 0;
        EnterCriticalSection(&g_world_stream_service_cs);
        stop = g_world_stream_service_stop;
        if (!stop) g_world_stream_service_busy = 1;
        LeaveCriticalSection(&g_world_stream_service_cs);
        if (stop) break;

        double stream_worker_start = now_seconds();
        int stream_worker_sample = 0;
        if (!g_mp_connected && g_screen == SCREEN_GENERATING && g_worldgen.active && g_stream_generation_keep_completed) {
            /* Loading-screen preload uses the same worker-backed stream path as
               gameplay, but worldgen_tick only polls progress.  This keeps the
               dirt/progress screen responsive instead of blocking inside chunk
               generation like the failed main-thread preload path did. */
            stream_worker_sample = 1;
            if (stream_generation_active()) {
                process_stream_generation_queue();
            } else if (g_stream_initial_light_settle_requested && !g_stream_initial_light_settle_done) {
                flat_run_initial_light_settle();
            }
            flat_lighting_worker_wake();
            if (!g_flat_lighting_worker_thread) flat_flush_pending_lighting();
        } else if (g_screen == SCREEN_INGAME && !g_mp_connected) {
            /* Keep desktop gameplay streaming off the main tick. */
            stream_worker_sample = 1;
            update_infinite_world_streaming();
            flat_lighting_worker_wake();
            if (!g_flat_lighting_worker_thread) flat_flush_pending_lighting();
        }
        if (stream_worker_sample) {
            double stream_ms = (now_seconds() - stream_worker_start) * 1000.0;
            if (stream_ms < 0.0) stream_ms = 0.0;
            g_prof_stream_worker_last_ms = stream_ms;
            if (g_prof_stream_worker_samples <= 0) g_prof_stream_worker_avg_ms = stream_ms;
            else g_prof_stream_worker_avg_ms = g_prof_stream_worker_avg_ms * 0.90 + stream_ms * 0.10;
            g_prof_stream_worker_samples++;
        }

        EnterCriticalSection(&g_world_stream_service_cs);
        g_world_stream_service_busy = 0;
        LeaveCriticalSection(&g_world_stream_service_cs);

        /* Active streaming gets frequent service, idle mode backs off hard so
           uncapped rendering is not fighting a polling thread. */
        Sleep((stream_generation_active() || g_stream_initial_light_settle_requested ||
               g_stream_initial_light_settle_running || flat_lighting_pending_dirty()) ? 1 : 20);
    }
    EnterCriticalSection(&g_world_stream_service_cs);
    g_world_stream_service_busy = 0;
    LeaveCriticalSection(&g_world_stream_service_cs);
    return 0;
}

static void world_stream_service_ensure(void) {
#if defined(PEX_PLATFORM_WII)
    return;
#else
    if (g_world_stream_service_initialized) return;
    flat_lighting_worker_ensure();
    g_world_stream_service_initialized = 1;
    g_world_stream_service_stop = 0;
    g_world_stream_service_busy = 0;
    InitializeCriticalSection(&g_world_stream_service_cs);
    g_world_stream_service_thread = CreateThread(NULL, 0x400000, world_stream_service_proc, NULL, 0, NULL);
    if (g_world_stream_service_thread) {
        /* Terrain preload/generation is intentionally off the main/render thread.
           Keep the service below normal priority so a mobile/lightweight CPU stays
           responsive while the loading screen polls progress. */
        SetThreadPriority(g_world_stream_service_thread, THREAD_PRIORITY_BELOW_NORMAL);
        pex_logf("world stream service started");
    } else {
        pex_logf("world stream service failed to start; streaming will be serviced by fallbacks only");
    }
#endif
}

static void world_stream_service_shutdown(void) {
#if defined(PEX_PLATFORM_WII)
    return;
#else
    if (!g_world_stream_service_initialized) return;
    EnterCriticalSection(&g_world_stream_service_cs);
    g_world_stream_service_stop = 1;
    LeaveCriticalSection(&g_world_stream_service_cs);
    if (g_world_stream_service_thread) {
        WaitForSingleObject(g_world_stream_service_thread, INFINITE);
        CloseHandle(g_world_stream_service_thread);
        g_world_stream_service_thread = NULL;
    }
    DeleteCriticalSection(&g_world_stream_service_cs);
    g_world_stream_service_initialized = 0;
    g_world_stream_service_stop = 0;
    g_world_stream_service_busy = 0;
    flat_lighting_worker_shutdown();
    pex_logf("world stream service stopped");
#endif
}

static int world_stream_service_active(void) {
#if defined(PEX_PLATFORM_WII)
    return 0;
#endif
    if (!g_world_stream_service_initialized) return 0;
    int active = 0;
    EnterCriticalSection(&g_world_stream_service_cs);
    active = g_world_stream_service_busy;
    LeaveCriticalSection(&g_world_stream_service_cs);
    return active;
}

static void update_infinite_world_streaming(void) {
    /* Do a tiny amount of terrain generation per tick.  This is the important
       part: new chunks are no longer built all at once on the frame that the
       streaming message appears. */
    process_stream_generation_queue();
    if (stream_generation_active()) return;

    /* After the fast spawn-area load, fill the requested render-distance area
       in the background.  This keeps the loading screen short while still
       honoring high render distance as terrain becomes available. */
    if (stream_queue_visible_chunks()) {
        process_stream_generation_queue();
        return;
    }

#if defined(PEX_PLATFORM_WII)
    /* The Wii safety profile currently uses a 64x64 active world window
       (4x4 chunks).  The desktop streaming-margin logic assumes a much wider
       window and forced a 48-block margin; with a 64-block window that made the
       origin slide every single tick even while the player stood at spawn.  The
       result was an endless "Generating terrain chunks" loop and all generated
       terrain moving away from the player, so only sky/HUD/inventory rendered.
       Keep the Wii active window fixed for now.  The visible 4x4 chunk window is
       filled above by stream_queue_visible_chunks(). */
    return;
#endif

    int pcx = floor_div16((int)floorf(g_player_x));
    int pcz = floor_div16((int)floorf(g_player_z));

    int margin = stream_window_margin_blocks();
    int new_origin_x = g_flat_world_origin_x;
    int new_origin_z = g_flat_world_origin_z;

    /* Slide by one chunk at a time instead of recentring the whole 256x256
       window.  Recentering could expose five or more strips at once, which then
       required hundreds of chunks to be generated. */
    if (g_player_x > g_flat_world_origin_x + FLAT_WORLD_SIZE - margin) {
        new_origin_x += FLAT_RENDER_CHUNK;
    } else if (g_player_x < g_flat_world_origin_x + margin) {
        new_origin_x -= FLAT_RENDER_CHUNK;
    }

    if (g_player_z > g_flat_world_origin_z + FLAT_WORLD_SIZE - margin) {
        new_origin_z += FLAT_RENDER_CHUNK;
    } else if (g_player_z < g_flat_world_origin_z + margin) {
        new_origin_z -= FLAT_RENDER_CHUNK;
    }

    if (new_origin_x == g_flat_world_origin_x && new_origin_z == g_flat_world_origin_z) {
        g_stream_last_center_chunk_x = pcx;
        g_stream_last_center_chunk_z = pcz;
        return;
    }

    /* Save edited chunks before they leave the 256x256 active window.  Generated
       chunks are not written, only chunks with real gameplay changes. */
    save_modified_flat_chunks();

    int old_origin_x = g_flat_world_origin_x;
    int old_origin_z = g_flat_world_origin_z;

    g_stream_remap_in_progress = 1;
    g_flat_world_origin_x = new_origin_x;
    g_flat_world_origin_z = new_origin_z;
    stream_remap_render_chunks(old_origin_x, old_origin_z, new_origin_x, new_origin_z);

    /* Preserve edited/generated blocks in the overlap of the old and new window.
       This used to copy the entire 3D world into two huge scratch arrays and then
       copy it back byte-by-byte, which caused the hitch during terrain streaming.
       The row-copy remap keeps only a small 2D scratch plane and leaves exposed
       strips as air for the async generator to fill. */
    stream_remap_block_storage(old_origin_x, old_origin_z, new_origin_x, new_origin_z);
    g_stream_remap_in_progress = 0;
    flat_lighting_worker_wake();

    stream_queue_missing_chunks(old_origin_x, old_origin_z);
    process_stream_generation_queue();

    g_stream_last_center_chunk_x = pcx;
    g_stream_last_center_chunk_z = pcz;

#if defined(PEX_PLATFORM_WII)
    static int s_wii_stream_chat_active = 0;
    if (g_stream_gen_queue_count > 0) {
        if (!s_wii_stream_chat_active) {
            hud_add_chat("Generating terrain chunks...");
            s_wii_stream_chat_active = 1;
        }
    } else {
        if (s_wii_stream_chat_active) hud_add_chat("Generated new terrain chunks.");
        s_wii_stream_chat_active = 0;
    }
#else
    /* Streaming now runs on a service thread; do not mutate the chat HUD from
       that thread.  Detailed progress is available in log.txt when verbose
       chunk logging is enabled. */
#endif
}


static int flat_player_head_block(void) {
    /* g_player_y is the first-person camera / eye height.  Render-only water
       effects must use this sample, not the player's feet or body, otherwise
       shallow water paints the whole view blue while the head is still above it. */
    int bx = (int)floorf(g_player_x);
    int by = (int)floorf(g_player_y);
    int bz = (int)floorf(g_player_z);
    return flat_get_block(bx, by, bz);
}

static int flat_player_head_in_water(void) {
    return block_is_water(flat_player_head_block());
}

static int flat_player_head_in_lava(void) {
    return block_is_lava(flat_player_head_block());
}

static int flat_block_is_underwater_target(int bx, int by, int bz) {
    /* A mined block counts as underwater when water is directly above it or
       pressed against any side.  The target itself is normally solid, because
       raycast skips liquid blocks. */
    return block_is_water(flat_get_block(bx, by + 1, bz)) ||
           block_is_water(flat_get_block(bx + 1, by, bz)) ||
           block_is_water(flat_get_block(bx - 1, by, bz)) ||
           block_is_water(flat_get_block(bx, by, bz + 1)) ||
           block_is_water(flat_get_block(bx, by, bz - 1));
}

static int flat_player_in_water(void) {
    /* Physics/swimming intentionally still checks body/feet contact. */
    int bx = (int)floorf(g_player_x);
    int by = (int)floorf(g_player_y - 0.25f);
    int bz = (int)floorf(g_player_z);
    return block_is_water(flat_get_block(bx, by, bz)) || block_is_water(flat_get_block(bx, by - 1, bz));
}

static int flat_player_in_lava(void) {
    /* Physics/swimming intentionally still checks body/feet contact. */
    int bx = (int)floorf(g_player_x);
    int by = (int)floorf(g_player_y - 0.25f);
    int bz = (int)floorf(g_player_z);
    return block_is_lava(flat_get_block(bx, by, bz)) || block_is_lava(flat_get_block(bx, by - 1, bz));
}
