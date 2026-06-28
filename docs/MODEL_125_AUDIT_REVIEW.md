# PexCraft Java 1.2.5 Mob Model Audit

## What I changed after the trust check

The previous `mobs-125-best-effort` patch did **not** implement the mob models 1:1. It used a set of generic approximations:

- Zombie, skeleton, pig zombie, enderman, creeper, and giant shared overly generic biped geometry.
- Wolf and ocelot were rendered as simplified quadrupeds instead of their real `ModelWolf` / `ModelOcelot` cuboids.
- Silverfish was rendered as a slime-like cube instead of segmented `ModelSilverfish` geometry.
- Magma cube was rendered as a slime instead of the eight-slice `ModelMagmaCube`.
- Ghast, Blaze, Squid, Spider, Iron Golem, Snowman, Villager, and Dragon were partial approximations.
- The renderer skipped model parts at distance through the old `detail` shortcut, which is not 1.2.5 behavior.

I replaced the placeholder emitter with model-specific Java 1.2.5 cuboid emitters in `src/game/passive_mobs.c`.

## Source of truth used

The patch was checked against these Java 1.2.5 deobfuscated classes from the uploaded `1.2.5_deobf(8).zip`:

- `ModelPig.java`
- `ModelQuadruped.java`
- `ModelSheep1.java`
- `ModelSheep2.java`
- `ModelCow.java`
- `ModelChicken.java`
- `ModelCreeper.java`
- `ModelBiped.java`
- `ModelZombie.java`
- `ModelSkeleton.java`
- `ModelEnderman.java`
- `ModelSpider.java`
- `ModelSlime.java`
- `ModelGhast.java`
- `ModelBlaze.java`
- `ModelSquid.java`
- `ModelWolf.java`
- `ModelOcelot.java`
- `ModelIronGolem.java`
- `ModelSnowMan.java`
- `ModelVillager.java`
- `ModelMagmaCube.java`
- `ModelSilverfish.java`
- `ModelDragon.java`

## Files changed by this model audit

Only one source file changed compared with the previous mob patch:

- `src/game/passive_mobs.c`

## Main implementation changes

- Added an `age` field to `PassiveMobRenderEntry` so models that use Java's `ageInTicks` parameter can animate.
- Replaced the old generic `passive_render_quad_model` / `passive_render_chicken` path with Java 1.2.5-specific emitters:
  - `passive_render_p125_quadruped`
  - `passive_render_p125_chicken`
  - `passive_render_p125_creeper`
  - `passive_render_p125_biped`
  - `passive_render_p125_spider`
  - `passive_render_p125_slime`
  - `passive_render_p125_ghast`
  - `passive_render_p125_blaze`
  - `passive_render_p125_squid`
  - `passive_render_p125_wolf`
  - `passive_render_p125_ocelot`
  - `passive_render_p125_iron_golem`
  - `passive_render_p125_snowman`
  - `passive_render_p125_villager`
  - `passive_render_p125_silverfish`
  - `passive_render_p125_dragon`
- Removed distance-based part omission from the model emitters. The `detail` parameter is now ignored for 1.2.5 model geometry.
- Corrected model pre-scales:
  - Giant: `6.0`, matching `RenderGiantZombie`.
  - Ghast: `4.5`, matching idle `RenderGhast` pre-scale.
  - Cave spider: `0.7`, matching `EntityCaveSpider.spiderScaleAmount()`.
  - Silverfish, squid, iron golem, dragon: now use normal model scale instead of previous approximated scales.
  - Slime and magma cube now use slime size as model scale instead of the previous entity-size scale.

## Per-mob model review

| Mob | Previous model state | New model state |
|---|---|---|
| Pig | Close, but not fully audited | Uses `ModelPig` / `ModelQuadruped(6)` cuboids, including snout |
| Sheep | Approximate base/fur | Uses `ModelSheep2` base and `ModelSheep1` fleece cuboids |
| Cow | Close but approximate | Uses `ModelCow`, including horns and udder |
| Chicken | Close but approximate | Uses `ModelChicken`, including bill, chin, wings, leg pivots, and negative head pitch |
| Creeper | Wrong generic biped | Uses `ModelCreeper` cuboids and four short legs |
| Skeleton | Generic biped limbs | Uses skinny `ModelSkeleton` arms/legs and zombie/bow-style pose approximation |
| Spider | Approximate | Uses `ModelSpider` head/neck/body/eight-leg cuboids and Java leg swing math |
| Giant | Generic biped | Uses biped model with `RenderGiantZombie` scale `6.0` |
| Zombie | Generic biped | Uses `ModelZombie` biped geometry and zombie arm pose |
| Slime | Approximate cube | Uses `ModelSlime(16)` inner face/body and `ModelSlime(0)` outer shell cuboids |
| Ghast | Approximate body/tentacles | Uses `ModelGhast` body, 9 tentacle pivots, and Java `Random(1660)` tentacle lengths |
| Zombie Pigman | Generic biped | Uses biped/zombie model shape; held sword render is not implemented |
| Enderman | Generic biped | Uses `ModelEnderman` tall body/limb geometry, headwear offset, and clamped limb swing |
| Cave Spider | Approximate spider scale | Uses `ModelSpider` with `0.7` cave-spider scale |
| Silverfish | Wrong slime-like cube | Uses segmented `ModelSilverfish` body and wing cuboids |
| Blaze | Approximate 8 rods | Uses `ModelBlaze` 12 rods and Java orbit formulas |
| Magma Cube | Wrong slime cube | Uses `ModelMagmaCube` eight slice cuboids plus core |
| Ender Dragon | Very approximate | Uses `ModelDragon` cuboids/UVs for head, jaw, neck, body, scales, wings, legs, feet, and tail, but see limitation below |
| Squid | Approximate | Uses `ModelSquid` body, 8 tentacle pivots, and radial yaw setup |
| Wolf | Wrong quadruped approximation | Uses `ModelWolf` head/muzzle/ears/body/mane/legs/tail cuboids |
| Mooshroom | Cow-ish approximation | Uses `ModelCow` body with mooshroom texture. Red mushroom block attachments are still a renderer/equipped-item feature, not completed here |
| Snow Golem | Approximate | Uses `ModelSnowMan` head/body/base/arm cuboids and arm yaw logic |
| Ocelot | Wrong quadruped approximation | Uses `ModelOcelot` head/nose/ears/body/tail/leg cuboids |
| Iron Golem | Approximate | Uses `ModelIronGolem` 128x128 texture cuboids, head nose, torso, arms, and legs |
| Villager | Approximate | Uses `ModelVillager` head/nose/body/robe/arms/legs cuboids |

## What is still not honestly 1:1

I cannot honestly claim full visual parity for everything, because PexCraft's renderer/entity system is not Minecraft Java's `RenderLiving` + `ModelRenderer` stack.

Remaining non-1:1 areas:

1. **Ender Dragon animation is not exact.** 1.2.5 `ModelDragon` depends on `EntityDragon.func_40160_a(...)`, a flight-history ring buffer. PexCraft has no equivalent dragon movement history, so the patch preserves the real cuboids/UVs but uses a local oscillator for neck/tail/wings.
2. **Hierarchical child transforms are approximated.** Java `ModelRenderer` supports child parts such as dragon jaw, wing tip, leg tips, and feet. PexCraft's current fast cuboid emitter is single-pivot. The patch places those cuboids with matching dimensions/UVs but does not exactly duplicate all parent-child transform composition.
3. **Render passes are incomplete.** Java 1.2.5 has extra passes for spider eyes, enderman eyes, dragon eyes, slime translucent shell, sheep fleece, saddle, mooshroom mushrooms, etc. Sheep fleece and pig saddle already exist; the others are not all fully rendered as Java passes.
4. **Equipped/held items are incomplete.** Skeleton bow, zombie pigman sword, enderman carried block, snow golem snowballs, and villager profession-specific texture behavior are not complete 1:1 renderer features.
5. **State-specific poses are limited.** Sitting wolf/ocelot, angry/tame wolf collar details, ocelot sneaking/sprinting/sitting, sheep eating-head animation, golem attack/rose pose, slime/magma squish, and squid tentacle state are approximated because the PexCraft entity state does not yet store the same Java fields.

## Build check

I attempted the SDL2 build again. It still cannot compile in this environment because the container is missing `SDL2/SDL.h` and `SDL2_image` development headers. This is the same environment/toolchain limitation as before, not a new model-code result.

See `PexCraft_models_125_build_attempt.log`.

## Bottom line

The previous version was not trustworthy for 1:1 model claims. This patch is a real model audit pass and copies the Java 1.2.5 model cuboid data into PexCraft's renderer for every mob. The geometry is now much closer and model-specific, but the whole result is still not a perfect 1:1 clone until PexCraft also gains Java's `ModelRenderer` hierarchy, render-pass system, and missing per-entity animation state.
