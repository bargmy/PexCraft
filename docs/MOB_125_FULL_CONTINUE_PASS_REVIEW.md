# PexCraft Java 1.2.5 mobs full-continue pass review

## Scope

This pass continues the mobs-only Java 1.2.5 parity work. It does not touch items, blocks, or block breaking logic except through existing mob drop/render calls. The focus is on practical mob behavior gaps that can be represented inside PexCraft's current C mob architecture.

## Implemented in this pass

### Enderman aggression and teleport behavior

- Endermen are now neutral by default and only become hostile after being stared at or attacked.
- Added a Java-style player-look check using the player's look vector and line-of-sight block tracing.
- Added the stare sound when an enderman becomes angry from eye contact.
- Endermen now become angry when hit by a living attacker.
- Endermen now try to teleport away from projectile damage instead of taking the hit.
- Endermen in water now attempt a random teleport and take periodic water/drown damage if still wet.
- Random chase teleport now uses a bounded Java-like teleport helper instead of simply shifting coordinates.

### Slime and magma cube size/drop behavior

- Slime-family mobs now use Java-legal sizes `1`, `2`, and `4` instead of `1`, `2`, `3`.
- Spawn eggs now use the same size family as natural spawns.
- Loaded old/invalid slime size values are normalized back to a valid size.
- Slime and magma cube width, height, health, attack damage, render scale, and split size now follow the saved size value.
- Slimeballs only drop from smallest slimes.
- Magma cream drops only from non-small magma cubes.
- XP is only awarded for player kills, closer to Java's `recentlyHit` behavior.

### Creeper behavior

- Creeper fuse logic is closer to `EntityAICreeperSwell`:
  - starts under 3 blocks,
  - keeps swelling while still within about 7 blocks,
  - counts up to a 30-tick fuse,
  - counts down if the player escapes.

### Spider and snow golem edge behavior

- Spider/cave-spider climbing and pounce behavior from previous work is preserved.
- Snow golems now take heat damage in desert/hell-style biomes and water damage periodically.

### Render/animation behavior

- Slimes and magma cubes now store a squish value and render with a non-uniform vertical/horizontal squash stretch.
- The renderer uses true slime size values instead of masking the value down to `0..3`.
- Existing eye overlays, sheep grazing animation, fleece/saddle/special layers, and skeleton/pig-zombie held items are preserved.

## Verification performed

- Ran a brace/parenthesis/bracket balance check over `src/game/mobs.c` after stripping comments and strings.
- Regenerated a patch from the clean pre-pass copy.
- Could not perform a real compiler build in this container because the project depends on platform headers/libraries not installed here, especially SDL2/SDL2_image and platform-specific Windows/GL headers.

## Still not exact Java 1.2.5

- PexCraft still does not have Java's exact `EntityAI` task scheduler, `PathNavigate`, `EntityMoveHelper`, `EntityLookHelper`, or full entity inheritance tree.
- Mob pathfinding is closer than the original but not byte-for-byte Java pathfinding.
- Villages/villagers/golems are still approximated compared with Java's full village-door collection and reputation system.
- Looting, equipment drops, full enchantment effects, and all NBT-like mob data are still incomplete.
- Wolf/cat owner identity, sitting/following/teleport-to-owner, collar render details, and cautious ocelot AI are still approximated.
- Ghast/blaze/skeleton ranged AI does not exactly match Java's projectile cooldown/visibility/strafe behavior.
- The Ender Dragon remains a separate, incomplete boss system and should be handled in its own pass.
