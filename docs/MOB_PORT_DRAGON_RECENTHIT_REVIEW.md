# Mob Java 1.2.5 C-port continuation: Ender Dragon + recentlyHit

This pass continues the mob-only Java 1.2.5 port work from `PexCraft-mobs-port-spawners-silverfish`.

## Ported in this pass

### Ender Dragon lifecycle

- Adds an End-dimension dragon runtime gate.
- Automatically creates the dragon in the End dimension when no live dragon exists and the saved dragon-killed flag is false.
- Adds a persistent dragon-killed flag to the mob save stream (`PDRG`, save version 31) so the dragon does not respawn after death.
- Adds C-native dragon target picking around the End center/player region.
- Adds flying motion independent of normal ground mob AI/physics.
- Adds broad contact damage using dragon-scale distance instead of zombie-style reach.
- Adds a 200-tick death sequence.
- Creates a functional End exit portal and dragon egg when the death sequence finishes.

### EntityLiving.recentlyHit behavior

- Adds `PassiveMob.recently_hit` and decrements it every tick.
- Player-caused damage sets `recently_hit = 60`, matching Java `EntityLiving.recentlyHit` timing.
- XP, rare drops, spider eyes, and blaze rods now use the recent-player-hit rule rather than only checking the killing blow source.

### Drop-table correction

- Zombie Pigman rare drop now matches Java 1.2.5 normal branch:
  - gold ingot
  - gold sword
  - gold helmet
- The enchanted rare sword branch is still blocked by the current C `ItemStack` format lacking enchant/NBT data.

## Validation performed

- Bracket/brace/parenthesis balance check for:
  - `src/game/mobs.c`
  - `src/game/inventory.c`
  - `src/common/common.h`
- Generated patch against the previous pass.

## Not full-compile-tested here

This environment does not have the project platform headers/libs installed, especially SDL2/SDL2_image and Windows SDK headers, so a full build could not be completed in this container.

## Still not exact Java internally

The dragon now has lifecycle/flight/death/exit-portal behavior, but true internal 1:1 still requires the missing Java engine objects:

- multipart `EntityDragonPart` hitboxes
- end-crystal entity type and crystal healing beam
- dragon path history ring buffer used by Java rendering/body segments
- Java XP orb counts from the dragon death sequence
- full NBT/enchantment support for rare enchanted drops and Looting
