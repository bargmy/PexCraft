# PexCraft 1.2.5 exact-systems pass — no Ender Dragon changes

This patch continues from `PexCraft-main-mobs-125-engine-layer-no-dragon-patch.zip` and targets the specific remaining gaps requested: Java 1.2.5 `PathFinder`, village wooden-door sky-side scoring, NBT-style village persistence, and `ModelRenderer` hierarchy support. The Ender Dragon was intentionally not changed in this pass.

## Implemented in this pass

### Java 1.2.5-style PathFinder

The previous compact A* was replaced with a C port following Java 1.2.5 `PathFinder`, `Path`, `PathPoint`, and `PathEntity` semantics:

- `PathPoint.makeHash`-style coordinate hashing.
- Binary heap open set matching `Path.addPoint`, `dequeue`, and `changeDistance` behavior.
- `PathFinder.addToPath` nearest-reachable fallback when the exact target is not found.
- `findPathOptions` with the vanilla neighbor order: south, west, east, north.
- `getSafePoint` with 1-block step-up and up-to-4-block controlled drop behavior.
- `getVerticalOffset` result codes matching 1.2.5: passable, water/trapdoor, water-blocked, lava-blocked, fence/fence-gate, trapdoor, blocked.
- Entity-size path volume uses `floor(width + 1)`, `floor(height + 1)`, `floor(width + 1)`, like Java 1.2.5.
- `PathEntity.getVectorFromIndex` offset now uses `(int)(width + 1) * 0.5`, not a hard-coded `0.5`.

Flying/swimming mobs still use direct fluid/air node selection because PexCraft does not expose a full Java flying navigator for Ghast/Blaze/Squid.

### Village door sky-side scoring

The village door code now follows Java 1.2.5 `VillageCollection.addDoorToNewListIfAppropriate`:

- Reads wooden door orientation from lower-door metadata.
- Scans five blocks on each side of the door.
- Uses `canBlockSeeTheSky`-style sky checks.
- Rejects doors with equal sky on both sides.
- Assigns `insideDirectionX` / `insideDirectionZ` as `±2`, matching Java `VillageDoorInfo`.
- Door targets use the inside position instead of the door center.

### Village runtime behavior

The runtime village structure now mirrors Java `Village` more closely:

- Maintains `centerHelper` sums.
- Recomputes center as `centerHelper / door_count`.
- Recomputes radius as `max(32, sqrt(maxDoorDistanceSq) + 1)`.
- Tracks `lastAddDoorTimestamp`, `tickCounter`, and per-door `lastActivityTimestamp`.
- Implements door removal when the block is gone or activity age exceeds 1200 ticks.
- Implements door opening restriction counters and `findNearestDoorUnrestricted` behavior for villager door selection.
- Iron golem spawning now uses the 1.2.5 conditions: `numIronGolems < numVillagers / 16`, more than 20 doors, and `rand.nextInt(7000) == 0`, with 10 spawn-location attempts.

### NBT-style village persistence

The patch adds village persistence in two forms:

- A `PVLG` binary block inside the existing `LEVELST1` world-state stream, so save/load consumes the village state in order.
- A Java-style `villages.dat` GZIP/NBT sidecar containing `Villages`, `Doors`, center/radius fields, tick counters, villager/golem counts, player reputation, and per-door inside direction/activity/restriction values.

The main world-state version was bumped from 23 to 27 and `LEVELST1` loading was updated to accept version 27.

### ModelRenderer hierarchy support

The mob renderer now routes 1.2.5 cuboids through a `Pex125ModelRendererNode` tree renderer:

- Supports parent `rotationPoint` + child `rotationPoint` nesting.
- Composes parent and child rotations before emitting the cube, matching the GL matrix nesting behavior of Java 1.2.5 `ModelRenderer.render`.
- Existing 1.2.5 cuboid renderers now use this ModelRenderer path as the leaf emission layer.

Ender Dragon model/animation was left unchanged.

## Files changed

- `src/game/passive_mobs.c`
- `src/game/world_session.c`
- `docs/MOB_125_EXACT_SYSTEMS_NO_DRAGON_REVIEW.md`

## Honest remaining limitations

This patch is much closer, but I still would not call the entire mob system mathematically proven 1:1. The main remaining non-dragon limitations are:

- PexCraft still does not have Java's complete `EntityAITasks` class hierarchy; the task mask is still C-side scheduling.
- The pathfinder follows Java 1.2.5 control flow, but block passability is mapped through PexCraft's block model, not Mojang's exact `Block.blocksList[].getBlocksMovement` objects.
- The NBT sidecar is written, and the active gameplay save/load uses the binary `PVLG` block for reliable loading. Full generic NBT readback is not implemented.
- The ModelRenderer tree backend exists and is used for cuboid emission, but some model functions still use already-flattened Java cuboid coordinates instead of being rewritten line-for-line as Java constructors with child arrays.
- Ender Dragon was intentionally not touched.

## Build validation

A normal `make -f Makefile.linux_sdl2` was attempted. The container still lacks SDL2 and SDL2_image development headers, so the build stops before compiling the project C sources. A separate syntax-only check using local stubs progressed past `passive_mobs.c` without passive-mob syntax errors, then stopped later in unrelated SDL/gamepad/log-window stubs.
