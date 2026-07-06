# Mob Java 1.2.5 Port Pass: Spawners + Silverfish

This pass continues the mobs-only Java 1.2.5 behavior port. It does not try to clone the Java class layout; it ports the gameplay behavior into PexCraft's existing C/flat-world systems.

## Ported in this pass

### TileEntityMobSpawner-style runtime

- Active range: player must be within 16 blocks.
- Initial delay: 20 ticks.
- Reset delay: 200 + random(600) ticks.
- Four spawn attempts per activation.
- Spawn position uses Java-like random horizontal spread and y offset around the spawner.
- Nearby cap: aborts when 6 same-type mobs already exist in the 8 x 4 x 8 box.
- Emits local particles/sound feedback when active/spawning.
- Runtime spawner delay/type cache is persisted in the passive mob save stream as save version 30.

Because this port currently lacks a full TileEntity/NBT runtime table for every placed spawner, spawner entity type is resolved from generated-structure context:

- Nether brick nearby -> Blaze
- Web/rails nearby -> Cave Spider
- End portal frame / stronghold stone brick nearby -> Silverfish
- Cobblestone/mossy cobblestone dungeon room nearby -> Skeleton/Zombie/Spider using Java dungeon distribution style
- Otherwise -> Pig, matching Java's default TileEntityMobSpawner entity id

### Silverfish block behavior

- Monster egg blocks now drop nothing.
- Breaking a monster egg block spawns a silverfish.
- Attacking a silverfish starts the Java-style 20 tick ally summon delay.
- On summon, nearby monster egg blocks break and release silverfish.
- Silverfish can hide in nearby stone/cobblestone/stone brick when idle and away from the player.
- Silverfish hidden block metadata maps to Java's disguise variants:
  - 0: stone
  - 1: cobblestone
  - 2: stone brick

## Validation done here

- Structural brace/parenthesis/bracket sanity check passed for `src/game/mobs.c` and `src/game/inventory.c`.
- Full compile could not be run in this environment because the available container is missing platform/build headers such as SDL2/SDL2_image and Windows compatibility headers.

## Still not exact yet

The runtime behavior is much closer, but exact generated spawner EntityId persistence still needs a real TileEntity/NBT table connected to generated structures. This pass uses deterministic/contextual mapping so the known Java structures spawn the correct mob types in PexCraft's current architecture.
