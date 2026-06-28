# PexCraft 1.2.5 mobs: closer no-dragon follow-up review

Base used: `PexCraft-main-mobs-125-1to1-spawn-ai-patch.zip`

Scope requested: make the 1.2.5 mob port closer to Java 1.2.5 while intentionally **not** working on the Ender Dragon yet.

## Honest status

This patch moves the mob system closer to Java 1.2.5, but it is still not a certified 1:1 clone. PexCraft still lacks several engine-level systems that Java 1.2.5 uses: full `EntityAI` task scheduling, A* pathfinding, village door/reputation tracking, full `ModelRenderer` parent-child rendering, exact mob ownership/NBT data, and some render-pass state. I did not touch the Ender Dragon behavior/model logic in this pass.

## Added or tightened in this pass

### Breeding, love mode, and babies

- Added `love_time` to the saved `PassiveMob` state and bumped passive mob save version to 25.
- Added breeding-item checks for the 1.2.5 animal set:
  - wheat for pig/sheep/cow/mooshroom/chicken,
  - meat for tamed wolves,
  - fish for tamed ocelots/cats.
- Added mate search, close-range breeding completion, child creation, and parent love cooldown.
- Added baby growth ticking.
- Added baby follow-parent behavior.
- Added adults following a nearby player holding the correct breeding item.
- Added render scaling and shadow scaling for baby mobs.

### Wolf and ocelot/cat state

- Tamed wolves now enter sitting state after taming.
- Tamed wolves can be healed/fed with meat.
- Tamed wolves can be toggled sitting/standing with empty-hand interaction.
- Tamed wolves use the tame texture, and angry wolves use the angry texture.
- Ocelots can tame into cat variants, stored in the existing color/state field.
- Tamed ocelots/cats can be toggled sitting/standing with empty-hand interaction.
- Cat variants now select black/red/Siamese textures when available.

Remaining non-1:1 wolf/cat gap: owner identity and full `EntityAITempt`/`EntityAISit`/teleport-to-owner logic are still approximated.

### Sheep and mooshroom behavior

- Added sheep grass-eating/regrowth approximation:
  - sheared sheep can regrow wool after eating grass,
  - baby sheep growth is accelerated by grass eating.
- Added mooshroom shearing:
  - shearing converts mooshroom to cow,
  - drops five red mushrooms,
  - damages shears.
- Added visible red mushroom render attachments on mooshrooms using terrain sprites.

Remaining non-1:1 gap: the mushroom render uses sprite attachments rather than Java's exact model-renderer block transform stack.

### Enderman excluding dragon

- Added enderman carried-block state use and persistence through existing save system.
- Added enderman block pickup/drop approximation using Java 1.2.5 carryable block categories.
- Added rendered held block sprite for endermen.
- Added enderman eye texture loading and additive eye render pass.
- Added water damage for endermen.

Remaining non-1:1 gap: exact stare/aggression state, carried block metadata, and teleport selection logic are still approximate.

### Hostile/projectile/utility mob behavior

- Added cave spider poison on melee hit for Normal/Hard-style difficulties.
- Added slime/magma-cube splitting on death.
- Added creeper explosion block breaking and nearby mob damage.
- Added projectile damage against mobs, not only players:
  - player arrows can hit mobs,
  - mob projectiles can hit mobs,
  - snowballs damage blazes.
- Added snow golem snow-trail placement in cold/snowy biomes and water damage.
- Added spider and cave-spider glowing eye render pass.
- Added ghast charging texture selection while attack timer is active.

Remaining non-1:1 gap: ghast/fireball area explosions, exact entity knockback math, and full block resistance calculation are still simplified.

### Rendering pass improvements

- Added texture loading/pack support for:
  - `mob/spider_eyes.png`,
  - `mob/enderman_eyes.png`.
- Added additive eye passes for spiders/cave spiders and endermen.
- Added held-block render pass for endermen.
- Added mooshroom mushroom render pass.
- Added ghast firing texture switch.

Remaining non-1:1 gap: Java 1.2.5 `RenderLiving`/`ModelRenderer` hierarchy is still not fully represented; this patch only closes some visible render-pass gaps.

## Files changed from previous spawn/AI patch

```text
src/assets/textures.c      +4 / -0
src/common/common.h        +3 / -2
src/game/inventory.c       +27 / -5
src/game/passive_mobs.c    +496 / -19
```

## Full diff impact from original uploaded PexCraft

```text
docs/MOB_125_1TO1_SPAWN_AI_REVIEW.md   +153 / -0
docs/MOB_125_PORT_REVIEW.md            +112 / -0
docs/MODEL_125_AUDIT_REVIEW.md         +129 / -0
src/assets/textures.c                   +91 / -1
src/common/common.h                     +48 / -7
src/game/inventory.c                    +156 / -14
src/game/passive_mobs.c                 +1860 / -189
src/render/item_render.c                +8 / -0
src/render/world_view.c                 +14 / -4
```

## Build attempt

I attempted the Linux SDL2 build with:

```sh
make -f Makefile.linux_sdl2
```

The environment still fails before compiling the game source because SDL2 and SDL2_image development headers are missing:

```text
fatal error: SDL2/SDL.h: No such file or directory
```

So the patched source is packaged, but the final C compile could not be validated in this container.

## Not touched in this pass

- Ender Dragon behavior/model/animation.
- Full Java village door scan and villager population manager.
- Full Java pathfinding and `EntityAI` task system.
- Exact owner UUID/name persistence for tamed mobs.
- Exact fireball/explosion resistance implementation.
- Full render-pass parity for every special mob state.
