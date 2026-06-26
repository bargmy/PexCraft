# Java 1.2.5 item/block parity status after registry/render patch

## Done in this patch

- Fixed the Linux build failure from the runtime language patch:
  - added a forward declaration for `rebuild_screen()` before `language.c` calls it;
  - removed the `/*` sequence inside the top comment caused by `/lang/*.lang`;
  - removed duplicated dead `return 0` in the language downloader.
- Replaced PexCraft's creative/inventory 3D decision with Java 1.2.5 `RenderBlocks.renderItemIn3d(getRenderType())` rules.
- Added a Java 1.2.5 block render-type switch for block IDs 1..124.
- Split inventory item rendering metadata from placed-world metadata.
- Added Java piston item special case: piston inventory rendering uses metadata `1` like Java `renderBlockAsItem()`.
- Replaced the fake 2D inventory block projection with the actual Java `RenderItem.drawItemIntoGui()` transform:
  - `translate(x - 2, y + 3, -3)`
  - `scale(10,10,10)`
  - `translate(1,0.5,1)`
  - `scale(1,1,-1)`
  - `rotateX(210)`
  - `rotateY(45)`
  - `rotateY(-90)`
- Added Java-style inventory models for:
  - normal cubes, including furnace/dispenser/workbench through face texture resolver;
  - slabs;
  - pressure plates;
  - stone button;
  - cactus;
  - stairs: wood, cobble, brick, stone brick, nether brick;
  - fence and nether brick fence;
  - fence gate;
  - chest/locked chest path;
  - piston/sticky piston;
  - dragon egg stacked cuboids.
- Fixed Java flat/3D decisions for these important mismatches:
  - bed/web/iron bars/glass pane/brewing stand/cauldron/end portal frame are no longer forced through the fake cube renderer;
  - pressure plates and stone button now go through the 3D cuboid path.
- Fixed wool texture metadata conversion to Java `BlockCloth` behavior.
- Fixed flat block item icon side selection to use Java-style side 2 / metadata item-class behavior instead of the top face.
- Added Java item tint handling for lily pad, vine, tall grass, and leaves in GUI icons.

## Still left from the full plan

### A. Texture resolver parity still needs a full table audit

The patch centralizes more behavior, but PexCraft still has older duplicated texture paths:

- `block_texture_resolve()`
- `world_face_style()`
- `block_item_tile_for_id()`
- digging/particle texture logic

These must be merged into one Java 1.2.5-style resolver. Until then, a block can be correct in creative inventory but still wrong in world rendering or particles.

### B. Render types not fully ported yet

The patch implements the inventory item models needed for the main creative visible bugs, but these Java render types are still not fully 1:1 in world rendering:

- `1` crossed squares: flowers, web, saplings, reeds, tall grass/dead bush variants
- `2` torch/redstone torch leaning placement
- `5` redstone wire overlays
- `6` crops/nether wart age quads
- `8` ladder exact placement and bounds
- `9` rail slope and powered/detector variants
- `12` lever exact model and powered rotation
- `15` repeater orientation/delay torches
- `18` panes/bars connection geometry
- `20` vine side-bit geometry
- `23` lily pad water-only placement and thin Y bounds
- `24` cauldron model and water fill levels
- `25` brewing stand model
- `26` end portal frame model and eye state

### C. Placed-block facing metadata still needs a Java class-by-class pass

Inventory facing is now separated from placed-world facing. The following placed metadata rules still need exact Java verification/fixing:

- furnace/dispenser `onBlockPlacedBy` and default orientation after placement;
- pumpkin/jack-o-lantern front metadata;
- stairs orientation and upside-down behavior where applicable;
- piston/sticky piston direction and extension metadata;
- ladder face metadata;
- torch/redstone torch attached side metadata;
- lever/button wall/floor metadata and powered bit;
- trapdoor hinge/top bit;
- door upper/lower half metadata;
- rails/detector/powered rail slope metadata;
- repeater direction and delay bits;
- bed head/foot and occupied bit;
- signs rotation/wall face;
- vines side-bit metadata;
- lily pad water placement.

### D. Item functions are still incomplete

The creative icon/name path is improved, but actual item behavior still needs Java 1.2.5 parity for:

- spawn eggs actually spawning the correct entity;
- dyes/bone meal behavior on saplings, mushrooms, crops, grass, sheep;
- shears on leaves/vines/tall grass/sheep;
- maps;
- potions/brewing;
- ender pearls/eyes of ender;
- bows/arrows durability and use animation;
- records/jukebox behavior;
- minecarts/boats placement and entity behavior.

### E. Language/font parity still needs final FontRenderer work

Runtime `.lang` loading from `client.jar` is present. Still left:

- key every remaining hardcoded UI string through Java language keys;
- replace fallback English item names with exact `.lang` key lookups;
- implement Java Unicode glyph pages `glyph_XX.png` and `glyph_sizes.bin`;
- implement bidi shaping/order for Arabic/Hebrew like Java's language settings path.

### F. Testing required before calling it complete

Run these after CI builds:

1. Open Java 1.2.5 creative inventory and PexCraft creative inventory at the same scale.
2. Compare the first 12 rows: order, texture, metadata, model, tint, stack count position.
3. Specifically verify furnace, dispenser, workbench, chest, fence, nether fence, fence gate, stairs, pressure plates, button, lily pad, vine, panes, bars, wool, dyes, spawn eggs.
4. Place each directional block in world from all four horizontal directions and compare metadata/facing to Java.
5. Run Linux SDL2 CI to confirm the language build error is gone.
