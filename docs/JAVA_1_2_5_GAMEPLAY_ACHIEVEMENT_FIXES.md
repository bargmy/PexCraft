# Java 1.2.5 parity fixes

This revision applies the five reported fixes on top of the Minecraft 1.2.5 GUI-scaling build.

## Tamed ocelot retaliation

Owned wolves and owned ocelots now reject their owner as a revenge/attack target. Owner damage clears stale target and navigation state. Player death also clears cached player targets and running attack tasks, including ocelot sneak/sprint attack state, so attacks and lunges cannot continue behind the death screen.

## Villager doors and first-person hand

Door changes now distinguish local player interaction from world/AI changes. Villager AI calls the non-player door path, which changes door metadata and plays the door sound without sending a player block-action packet or restarting the local hand swing.

## Player damage sound

The player hurt sound is now Java 1.2.5's `damage.hurtflesh`, with the original adult `EntityLiving` pitch formula `(randA - randB) * 0.2 + 1.0`. The old PexCraft classic-hurt alias is no longer used.

## Block step, breaking-progress, break, and placement sounds

Blocks use the Java 1.2.5 `Block.java` StepSound assignments. Grass, gravel, sand, cloth, wood, stone, metal, and glass are separated correctly. Break sounds use `StepSound.getBreakSound()`, including `random.glass` for glass-family blocks and `step.gravel` for sand. Volume and pitch formulas match `PlayerControllerSP`:

- breaking progress: `(volume + 1) / 8`, `pitch * 0.5`
- final break: `(volume + 1) / 2`, `pitch * 0.8`
- placement: `(volume + 1) / 2`, `pitch * 0.8`

This fixes grass blocks incorrectly using a generic walking/break sound combination.

## Achievement screen

The screen now follows `GuiAchievements` layering more closely:

- opaque selected-pack `gui/background.png` behind the pane
- exact seeded stone/dirt/ore/bedrock achievement map
- map content clipped to the 224x155 viewport before the 256x202 frame is drawn
- connector lines hidden outside the viewport
- locked icon brightness rather than a flat black overlay
- normal and special frames sampled from `achievement/bg.png`
- tooltip description and prerequisite text use non-shadowed split text
- title and `Taken!` retain Java's shadowed rendering and exact vertical spacing
- pane title is rendered as the final text layer, matching `func_27110_k()`

Selected resource-pack textures remain authoritative for `achievement/bg.png`, `terrain.png`, `gui/items.png`, and `gui/background.png`.
