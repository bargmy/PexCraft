# Java 1.2.5 Worldgen Migration Status

This branch continues the deterministic Minecraft Java 1.2.5 world-generation migration. It now includes the GenLayer/ChunkProvider work from the previous pass plus a full Java 1.2.5 block/item ID and texture/icon audit pass. It still does **not** claim complete 1:1 worldgen parity because structures, exact `BiomeDecorator` random-call order, Nether/End providers, and Java golden chunk hashes are still not complete.

## What Changed

| File | Change |
|---|---|
| `src/common/common.h` | Verified Java 1.2.5 block IDs `1-124`; added missing item IDs `351-385`; added missing record item IDs `2258-2266`; added compatibility aliases for Java/PexCraft item naming differences. |
| `src/render/item_render.c` | Added `gui/items.png` icon coordinates for every Java 1.2.5 item ID `256-385` and record ID `2256-2266`; expanded block inventory tile mapping for every Java 1.2.5 block ID `1-124`; added display names for all block/item IDs; updated item stack limits for shears, potions, maps, ender pearls, XP bottles, and records. |
| `src/render/world_view.c` | Expanded metadata-aware `block_texture_125()` coverage for all added 1.2.5 block IDs; added texture handling for pistons, sandstone variants, stone brick variants, nether blocks, End blocks, panes, bars, vines, lily pads, redstone lamps, cauldrons, brewing stands, enchantment tables, and new stairs/fences. |
| `src/game/inventory.c` | Updated stack-limit rules for newly added Java 1.2.5 items. Existing material/solidity/collision helpers from the foundation pass remain in place. |
| `src/worldgen/level.c` | Continued the next decoration pass: tree generation now writes log/leaf metadata for spruce, birch, and jungle variants; added simplified swamp-tree vines, tall grass, dead bush, water lily, and jungle vine generation. |
| `docs/worldgen_125_migration_status.md` | Updated this report with the ID/texture audit and decoration status. |

## ID and Texture Coverage Audit

| Surface | Coverage |
|---|---|
| Block constants | Java 1.2.5 block numeric IDs `1-124` are present. |
| Item constants | Java 1.2.5 item numeric IDs `256-385` are present. |
| Music disc constants | Java 1.2.5 record IDs `2256-2266` are present. |
| Item GUI icons | Every numeric `ITEM_*` constant in the Java 1.2.5 range has an `item_icon_tile()` mapping. |
| Block inventory icons | Every numeric `BLOCK_*` constant `1-124` has a `block_item_tile_for_id()` mapping. |
| Added-block world textures | Every block added in the 1.2.5 ranges `21-36` and `92-124` has a `block_texture_125()` mapping. |
| Runtime behavior | Not complete for every block. Many blocks render and save, but interaction logic for redstone, brewing, enchanting, pistons, portals, mob spawners, and spawn eggs is intentionally not implemented in this worldgen pass. |

## Source B References Used

| Subsystem | Reference files |
|---|---|
| Block IDs and block textures | `Block.java`, `BlockLog.java`, `BlockLeaves.java`, `BlockSandStone.java`, `BlockStoneBrick.java`, `BlockTallGrass.java`, `BlockVine.java`, `BlockLilyPad.java`, `BlockEndPortalFrame.java`, `BlockRedstoneRepeater.java`, `BlockPistonBase.java`, `BlockCauldron.java`, `BlockBrewingStand.java`, `BlockEnchantmentTable.java` |
| Item IDs and item icons | `Item.java`, `ItemRecord.java`, `ItemArmor.java`, `ItemTool.java`, `ItemFood.java`, `ItemPotion.java`, `ItemMonsterPlacer.java` |
| GenLayer pipeline | `GenLayer.java`, `GenLayerIsland.java`, `GenLayerFuzzyZoom.java`, `GenLayerZoom.java`, `GenLayerAddIsland.java`, `GenLayerAddSnow.java`, `GenLayerBiome.java`, `GenLayerHills.java`, `GenLayerRiverInit.java`, `GenLayerRiver.java`, `GenLayerRiverMix.java`, `GenLayerShore.java`, `GenLayerSmooth.java`, `GenLayerSwampRivers.java`, `GenLayerVoronoiZoom.java`, `GenLayerAddMushroomIsland.java` |
| Biome data | `BiomeGenBase.java`, `BiomeGenBeach.java`, `BiomeGenDesert.java`, `BiomeGenForest.java`, `BiomeGenJungle.java`, `BiomeGenMushroomIsland.java`, `BiomeGenPlains.java`, `BiomeGenSwamp.java`, `BiomeGenTaiga.java` |
| WorldChunkManager behavior | `WorldChunkManager.java` |
| Overworld terrain | `ChunkProviderGenerate.java`, especially `generateTerrain`, `replaceBlocksForBiome`, `initializeNoiseField`, and `provideChunk` biome-array writing |
| Caves/ravines | `MapGenBase.java`, `MapGenCaves.java`, `MapGenRavine.java` |
| Decoration additions | `ChunkProviderGenerate.java`, `BiomeDecorator.java`, `WorldGenSand.java`, `WorldGenClay.java`, `WorldGenTrees.java`, `WorldGenForest.java`, `WorldGenTaiga1.java`, `WorldGenTaiga2.java`, `WorldGenShrub.java`, `WorldGenHugeTrees.java`, `WorldGenSwamp.java`, `WorldGenBigMushroom.java`, `WorldGenTallGrass.java`, `WorldGenDeadBush.java`, `WorldGenWaterlily.java`, `WorldGenVines.java`, `WorldGenPumpkin.java`, `WorldGenDesertWells.java` |

## Implemented So Far

| Area | Status |
|---|---|
| GenLayer chain construction | Implemented for default Java 1.2.5 world type, including jungle biomes. |
| GenLayer seeding | Implemented with Java long wraparound behavior and layer/chunk seed update order. |
| `WorldChunkManager.getBiomesForGeneration` equivalent | Implemented through `biome_manager_get_generation()` using the non-Voronoi layer. |
| `WorldChunkManager.loadBlockGeneratorData` equivalent | Implemented through `biome_manager_get()` using the Voronoi biome index layer. |
| Chunk `Biomes` array | Written as a 256-byte NBT array. |
| Terrain density | Rewired to 1.2.5 biome min/max height smoothing. |
| Base terrain | Rewired to 1.2.5 stone/still-water generation with sea level 63. |
| Surface replacement | Rewired to 1.2.5 biome top/filler, bedrock, sandstone-under-sand, and cold-water ice rules. |
| Cave mapgen seeding | Updated from old addition-style seed composition to 1.2.5 `MapGenBase` XOR composition. |
| Ravines | Added a `MapGenRavine`-style pass after caves. |
| Block/item ID surface | Full Java 1.2.5 numeric block and item ID surface is now present. |
| Texture/icon mapping surface | Full mapping coverage exists for block inventory icons, item GUI icons, and added 1.2.5 world block textures. |
| Decoration pass | Reordered toward Java 1.2.5 `ChunkProviderGenerate.populate` + `BiomeDecorator.decorate`: ores first, sand/clay patches, per-biome tree generator selection, big mushrooms, flowers/grass/bushes/lilies, mushroom passes, reeds, pumpkins, cacti, liquid springs, jungle vines, desert wells, and snow/ice. |

## Known Non-Parity

| Area | Expected Source B behavior | Current patch status | Fix needed |
|---|---|---|---|
| Chunk hashes | Blocks/Data/Biomes must match Java golden chunks | Not verified against Java golden output | Add/compile a Java reference dumper and a PexCraft chunk hash harness. |
| Decoration order | Full 1.2.5 `BiomeDecorator` call order and random-call order | Major order fixes are now implemented: ores, sand/clay patches, tree selection, flowers/grass, mushrooms, reeds, pumpkins, cacti, springs, jungle vines, and desert wells. Some feature internals still need golden hash verification. | Add Java golden chunk decorator hashes and finish exact `WorldGenHugeTrees`/swamp/big-tree parity. |
| Tree shapes | Exact oak, birch, spruce, jungle, and swamp tree algorithms | Oak, birch/forest, taiga1, taiga2, shrub, jungle small tree, and a visual 2x2 huge jungle path are now present with metadata. Huge jungle trees, big trees, and swamp trees still need hash-level verification. | Finish byte-for-byte ports of `WorldGenHugeTrees`, `WorldGenBigTree`, and `WorldGenSwamp`. |
| Structures | Mineshafts, villages, strongholds | Not implemented | Port `MapGenStructure` and structure component classes. |
| Cave block restoration | Caves should restore biome top block when carving under grass | Cave code is still mostly the previous implementation, with 1.2.5 mapgen seeding applied | Finish `MapGenCaves.java` exact port. |
| Metadata parity | All generated logs/leaves/stairs/vines/chests/spawners etc. write exact metadata | Metadata substrate exists and tree/vine/tall-grass metadata are now used, but many generated placements still write zero metadata | Port metadata-aware generators and structures. |
| Runtime item behavior | All items behave like Java 1.2.5 | IDs/icons/names exist; many gameplay behaviors are placeholders or unimplemented | Implement gameplay systems separately from worldgen. |
| Nether | `ChunkProviderHell`, Nether caves, Nether fortresses | Not implemented | Add debug/internal-only dimension provider. |
| End | `ChunkProviderEnd`, End terrain/spikes | Not implemented | Add debug/internal-only dimension provider. |

## Validation

A script audit was run over the C source and confirmed:

- no missing block numeric IDs from `1-124`;
- no missing item numeric IDs from `256-385`;
- no missing record numeric IDs from `2256-2266`;
- no missing item icon mappings for numeric Java 1.2.5 item constants;
- no missing block inventory tile mappings for numeric Java 1.2.5 block constants;
- no missing world texture mappings for the added 1.2.5 block ranges `21-36` and `92-124`;
- local worldgen water IDs were corrected to Java IDs: moving water `8`, still water `9`;
- a standalone syntax/runtime smoke harness generated and populated seed `12345`, chunk `0,0` without crashing.

A full SDL2 build still cannot be completed in this container because the container lacks SDL2 development headers (`SDL2/SDL.h`). No Java golden parity tests were run yet.

## Final Checklist

- [ ] Overworld terrain parity tested
- [x] Biome pipeline implemented
- [ ] Biome parity tested against Java golden output
- [x] Java 1.2.5 block ID surface added
- [x] Java 1.2.5 item ID surface added
- [x] Java 1.2.5 item icon mappings added
- [x] Java 1.2.5 block inventory texture mappings added
- [x] Added-block world texture mappings added
- [ ] Block metadata parity tested
- [ ] Cave parity tested
- [x] Ravine pass implemented
- [ ] Ravine parity tested
- [ ] Decoration parity tested against Java golden output
- [x] Decorator call order substantially ported
- [ ] Stronghold location parity tested
- [ ] Stronghold block placement tested
- [ ] Village generation tested
- [ ] Mineshaft generation tested
- [ ] Nether generator implemented but unreachable
- [ ] End generator implemented but unreachable
- [x] No new mobs added
- [x] No dimension travel added
- [ ] Old saves still load
- [ ] Edited chunks are not overwritten

## Additional Pass: Golden Harness, Trees, and Structure Location Scaffolding

| Area | Change |
|---|---|
| Golden chunk dump harness | Added `tools/worldgen_125_dump.c`, a standalone C harness that emits SHA-1 hashes for `Blocks`, `Data`, `HeightMap`, and `Biomes` for a seed/dimension/chunk. |
| Public chunk-array generator | Added `worldgen_125_generate_chunk_arrays()` so tests can generate raw arrays without writing NBT files. |
| Stronghold location API | Added `worldgen_125_get_stronghold_coords()`, porting the Java 1.2.5 three-stronghold ring placement and `WorldChunkManager.findBiomePosition()` reservoir sampling. |
| Village candidate API | Added `worldgen_125_village_can_spawn()`, porting Java 1.2.5 grid spacing/separation and plains/desert biome viability check. |
| Mineshaft candidate API | Added `worldgen_125_mineshaft_can_spawn()`, porting Java 1.2.5 mapgen seed setup and probability check. |
| Big trees | Fixed `WorldGenBigTree` leaf distance from `5` to Java default `4`; corrected line-check floor behavior for negative coordinates. |
| Swamp trees | Replaced the earlier visual placeholder with a closer port of `WorldGenSwamp`, including water descent, top-canopy radius, trunk-through-water behavior, and vine columns. |
| Huge jungle trees | Replaced the earlier visual 2x2 placeholder with a closer port of `WorldGenHugeTrees`, including Java crown blobs, side branches, trunk-vine random-call order, and jungle log/leaves metadata. |
| Leaf placement opacity | Corrected the worldgen-local `opaqueCubeLookup` equivalent so leaves are non-opaque for tree leaf placement, matching `BlockLeavesBase.isOpaqueCube() == false`. |

Build the dump harness from the repo root:

```sh
cc -std=c99 -O2 tools/worldgen_125_dump.c -lm -o worldgen_125_dump
./worldgen_125_dump 12345 0 0 0
```

Sample smoke-test output from this pass:

```txt
seed=12345 dim=0 chunk=0,0 blocks_sha1=5d65197647e14acd1c43a61c7100d731727d6ab0 data_sha1=b5a68646929b8bbcfb8d7c0f2ce30e2ab2c53b37 heightmap_sha1=8b24770a0c22e99017f9e38e16448bac5ffafe75 biomes_sha1=a394afc6ffb99202649564894ae9b48eb81f15b9
strongholds=-38,59;-22,-45;34,13 village_here=0 mineshaft_here=0
```

### Remaining Structure Limitation

This pass implements deterministic **structure location/candidate scaffolding**, not full structure block placement. Full parity still requires porting the large `StructureStrongholdPieces`, `StructureVillagePieces`, and `StructureMineshaftPieces` component graphs and their metadata-aware block placement helpers.

## Structure block-placement pass

This pass moves the structure work beyond candidate/location scaffolding.  The Overworld generator now mutates generated chunks with deterministic structure blocks before decoration/population output is extracted.

| Structure family | Implemented block placement | Java 1.2.5 reference being targeted | Parity status |
|---|---|---|---|
| Stronghold | Stone-brick stair/corridor rooms, randomized normal/mossy/cracked/silverfish stone brick selector, portal-room-style lava, iron bars, End portal frames with eye-bit probability, silverfish spawner block, library shelves/chest placeholder | `MapGenStronghold`, `StructureStrongholdStart`, `ComponentStrongholdPortalRoom`, `StructureStrongholdStones`, `ComponentStrongholdLibrary` | Deterministic and closer visually, but not full component-graph parity yet. |
| Village | Gravel roads, well, two houses, farm plots, doors, panes, torches, fences, desert sandstone material fallback, village lake suppression approximation | `MapGenVillage`, `StructureVillageStart`, `StructureVillagePieces`, `ComponentVillageWell`, `ComponentVillageHouse*`, `ComponentVillageField*` | Deterministic block placement exists; exact weighted piece selection and bounding-box graph are still TODO. |
| Mineshaft | Starting room, X/Z corridors, planks, fences, rails, webs, chest placeholder, cave-spider spawner placeholder | `MapGenMineshaft`, `StructureMineshaftStart`, `StructureMineshaftPieces`, `ComponentMineshaftRoom`, `ComponentMineshaftCorridor` | Deterministic block placement exists; exact recursive corridor graph/chest loot/spawner tile data are still TODO. |

### Structure validation smoke tests

The standalone dump harness still builds with:

```sh
cc -std=c99 -O2 tools/worldgen_125_dump.c -lm -o worldgen_125_dump
```

Additional smoke-test chunks used during this pass:

```txt
seed=12345 chunk=-38,61  stronghold-adjacent portal-frame/spawner chunk
seed=12345 chunk=-156,49 village candidate chunk
seed=12345 chunk=-160,-53 mineshaft candidate chunk
```

These tests confirmed the generator runs without crashing and produces different block/data hashes once structure blocks are present.  This is still **not** a Java golden-hash parity claim; the full Java structure component graph remains the next accuracy target.

## Traceplace / Metadata / Dimension Debug Pass

| Area | Change |
|---|---|
| Runtime generated metadata | The in-game streamed chunk path now preserves generated metadata from the worldgen canvas instead of flattening logs/leaves/vines/stairs/portal frames/chests back to metadata `0`. |
| Chunk extraction calls | Updated all runtime extraction callers to the metadata-aware `extract_canvas_chunk(..., data)` signature so the branch is compile-facing again. |
| `/traceplace` command | Added `/traceplace village`, `/traceplace stronghold`, `/traceplace stronghold 1-3`, and `/traceplace mineshaft`. The command recenters/generates the destination window and places the player at a collision-checked safe landing spot. |
| Tile entities in generated NBT chunks | Generated chunk files now emit `TileEntities` entries for generated chests and mob spawners instead of an always-empty list. Chests currently have empty item lists; spawners currently use a Silverfish entity placeholder. |
| Nether debug generator | Added internal dimension `-1` support to the dump harness and chunk-array API. It generates deterministic netherrack/lava/soul-sand/gravel/glowstone terrain and a debug fortress-cross scaffold. No portal travel or player dimension switching was added. |
| End debug generator | Added internal dimension `1` support to the dump harness and chunk-array API. It generates deterministic End-stone island terrain and debug obsidian spikes. No End portal activation or player dimension switching was added. |
| Dump harness | `tools/worldgen_125_dump.c` now accepts dimensions `0`, `-1`, and `1`. |

Example commands in-game:

```txt
/traceplace village
/traceplace stronghold
/traceplace stronghold 2
/traceplace mineshaft
```

Example harness checks:

```sh
cc -std=c99 -O2 tools/worldgen_125_dump.c -lm -o worldgen_125_dump
./worldgen_125_dump 12345 0 0 0
./worldgen_125_dump 12345 -1 0 0
./worldgen_125_dump 12345 1 0 0
```

Important: the Nether and End generators are internal/debug-only. This pass intentionally does **not** add Nether portals, End portal activation, or player dimension travel.

### Remaining parity limits after this pass

| Area | Remaining work |
|---|---|
| Full Java structure parity | The current structure block placement is still a compact deterministic implementation, not a line-by-line port of every `StructureStrongholdPieces`, `StructureVillagePieces`, and `StructureMineshaftPieces` component. |
| Chest loot | Generated chests have tile entities but empty inventories. Java loot tables/weighted random chest contents still need to be ported. |
| Spawner entities | Generated mob spawners now have tile entities, but the entity type is still a placeholder in generated NBT and runtime mob spawning is not implemented. |
| Nether parity | The debug Nether exists but is not yet an exact `ChunkProviderHell` + `MapGenNetherBridge` port. |
| End parity | The debug End exists but is not yet an exact `ChunkProviderEnd` + `WorldGenSpikes` port. |
| Java golden comparison | Local hashes are produced; exact Java 1.2.5 reference hashes still need to be generated and compared. |

## Structure, loot, Nether/End noise continuation pass

| Area | Change |
|---|---|
| Stronghold structure block graph | Expanded the deterministic structure graph with prison, crossing, fountain, stair, library and portal-room inspired pieces, plus connector corridors. This improves in-game look and testing coverage but is still not a byte-for-byte Java component graph. |
| Village structure block graph | Added blacksmith, church/tower, lamp posts, extra doors/windows, lava/furnace/chest blacksmith details, and preserved desert sandstone fallback. |
| Mineshaft structure block graph | Added crossing, side spider room, extra branch corridors, rails, web pockets, support posts, chest, and cave-spider spawner placement. |
| Chest tile entities | Replaced empty generated chest inventories with deterministic ItemStack lists. Loot is context-aware for stronghold/library, mineshaft, nether fortress, and village/blacksmith fallback chests. |
| Spawner tile entities | Spawner NBT now chooses `Silverfish`, `CaveSpider`, or `Blaze` based on nearby generated blocks instead of always using one placeholder. Runtime mob spawning is still not implemented. |
| Nether generator | Replaced the radial/noise-bit debug terrain with a `ChunkProviderHell`-shaped octave-density terrain pass, surface replacement for bedrock/netherrack/soul sand/gravel/lava, and chunk-local versions of Nether lava, fire, glowstone, mushroom and fortress-block side effects. |
| End generator | Replaced the radial End island approximation with a `ChunkProviderEnd`-shaped octave-density terrain pass and a `BiomeEndDecorator`/`WorldGenSpikes`-style block-only spike pass. Dragon/crystal entities remain intentionally unimplemented. |
| Golden comparison tooling | Added `tools/worldgen_125_compare_hashes.py` to compare `worldgen_125_dump` output against Java-generated golden CSV hashes. |

### Smoke tests for this pass

```sh
cc -std=c99 -O2 tools/worldgen_125_dump.c -lm -o worldgen_125_dump
./worldgen_125_dump 12345 0 -38 61
./worldgen_125_dump 12345 0 -156 49
./worldgen_125_dump 12345 0 -160 -53
./worldgen_125_dump 12345 -1 0 0
./worldgen_125_dump 12345 1 0 0
```

### Remaining parity limits after this continuation pass

| Area | Remaining work |
|---|---|
| Full Java structure parity | The new pieces are component-inspired and deterministic, but the full recursive Java `StructureStrongholdPieces`, `StructureVillagePieces`, `StructureMineshaftPieces`, and exact bounding-box collision/piece-weight selection still need a line-by-line port. |
| Exact Java loot tables | Chests now contain real deterministic items, but exact Java weighted loot table classes and RNG-call order are not fully ported. |
| Runtime spawner behavior | Correcter spawner NBT is emitted, but PexCraft still does not implement hostile mob spawning/AI for those generated spawners. |
| Nether fortress parity | Fortress blocks exist with useful testing coverage, but the full `MapGenNetherBridge` piece graph is not fully ported yet. |
| End entity side effects | End terrain/spike blocks exist; dragon and Ender Crystal entity side effects remain intentionally skipped. |
| Golden hashes | The compare script exists, but Java 1.2.5 golden CSV generation must still be run outside this container before any 1:1 claim. |
