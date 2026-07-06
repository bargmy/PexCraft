# Mob Java 1.2.5 C-port continuation: dragon crystals / dragon parts

This pass continues from `PexCraft-mobs-port-dragon-recenthit.zip` and keeps the scope to mobs plus the projectile hook needed for Ender Crystal interaction.

## Ported in this pass

- Added a C runtime for Java 1.2.5 `EntityEnderCrystal` behavior:
  - seeded End crystals in the End dimension,
  - persistent crystal save data,
  - fire block at the crystal position,
  - crystal health/destruction path,
  - crystal explosion damage and block breaking,
  - player melee hit detection,
  - projectile hit detection.

- Added dragon healing from crystals:
  - nearest active crystal within 32 blocks is selected,
  - dragon heals 1 HP every 10 ticks while linked and below max health.

- Added a C-native multipart dragon hit approximation:
  - separate head/body/wing/tail boxes are ray-tested for player melee,
  - non-head parts apply Java `damage / 4 + 1`,
  - head hits apply full damage.

- Reworked Ender Dragon death behavior closer to Java 1.2.5:
  - XP orbs are released in Java-style chunks: 1000 XP every 5 ticks after death tick 150, plus 10000 XP at tick 200,
  - normal 1-3 mob XP is skipped for the dragon,
  - exit portal generation now uses the Java fixed y=64 disk/pillar/torch/egg layout centered on the dragon's death X/Z.

- Save format bumped to passive mob save version 32 with `PDCR` crystal state appended after the existing dragon state.

## Notes

This is a port, not a class-for-class clone. The Java source has real `EntityEnderCrystal` and `EntityDragonPart` entities; this engine now ports those gameplay effects into compact C runtime tables to fit PexCraft's current mob architecture.

## Validation

A structural check confirmed balanced braces, parentheses, and brackets for:

- `src/game/mobs.c`
- `src/game/inventory.c`
- `src/common/common.h`

Full compile was not possible in this environment because SDL2/SDL2_image/platform SDK headers are unavailable.
