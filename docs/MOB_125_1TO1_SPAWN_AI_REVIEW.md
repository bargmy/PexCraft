# PexCraft Java 1.2.5 mob spawn/AI/model follow-up review

## Status

This patch is a stronger 1.2.5-style mob pass over the previous model-audit patch. It implements the specific mechanics requested in PexCraft's current architecture, but it is **not honestly certifiable as a byte-for-byte or behavior-perfect Java 1.2.5 clone** because PexCraft still does not contain several vanilla engine subsystems: `EntityAI` task scheduling, `PathNavigate`, `VillageCollection`, full render-pass/equipped-item layers, block-damaging explosions, complete projectile/NBT persistence, village door/reputation mechanics, and the Ender Dragon flight-history/boss logic.

The code now aims for 1.2.5-compatible behavior where the missing engine pieces can be represented locally.

## Implemented in this follow-up patch

### 1.2.5-style natural spawning

- Replaced the old fixed passive animal refill with a category-based `EnumCreatureType`-style system:
  - monsters
  - creatures
  - water creatures
  - utility/village mobs
- Uses one-player 17x17 eligible chunk scaling like `SpawnerAnimals`:
  - monster cap: `70 * 289 / 256`
  - creature cap: `15 * 289 / 256`
  - water cap: `5 * 289 / 256`
- Uses biome-weighted spawn entries matching the main Java 1.2.5 overworld lists:
  - sheep, pigs, chickens, cows
  - spiders, zombies, skeletons, creepers, slimes, endermen
  - squid water spawns
  - wolves in forest/taiga variants
  - ocelots in jungle variants
  - mooshrooms in mushroom island biomes
- Added 1.2.5-style spawn restrictions:
  - player distance greater than 24 blocks
  - world-spawn distance greater than 24 blocks
  - solid block below and two air blocks for land mobs
  - grass + light > 8 for normal passive creatures
  - dark light test for monsters
  - Peaceful disables monster spawns and despawns existing monsters
  - squid y-level/water checks
  - slime y < 40 + slime-chunk + random check
  - ocelot jungle + grass/leaves + y >= 63 + random check
  - mushroom cow mushroom biome + mycelium
- Disabled natural overworld spawning for mobs that Java 1.2.5 does not naturally spawn in normal overworld creature lists:
  - Giant
  - Ender Dragon
  - Blaze
  - Ghast
  - Zombie Pigman
  - Magma Cube
  - Cave Spider
  - Silverfish

### Difficulty support

- Peaceful prevents monster spawning and despawns hostile monster-category mobs.
- Mob damage now scales by difficulty:
  - Peaceful: 0
  - Easy: half-ish damage
  - Normal: base damage
  - Hard: 1.5x damage
- Projectile damage from mob-fired arrows/fireballs also uses the same difficulty scaling.

### Zombies, skeletons, spiders, and hostiles

- Zombies now use 1.2.5-style move speed `0.23`.
- Skeletons now use move speed `0.25`.
- Spiders and cave spiders now use speed `0.80`.
- Spiders climb when horizontally collided, matching the `EntitySpider.isOnLadder()` behavior concept.
- Spiders get a simple pounce behavior at mid range.
- Skeletons now shoot arrow projectiles at the player.
- Skeletons now play bow sounds on shooting.
- Skeletons now render with a visible bow sprite attached to the raised arm.
- Zombies and skeletons burn in daylight when skylight conditions are met. This is daylight burning, matching Minecraft behavior; they do not burn at night.

### Projectiles

- Added projectile types:
  - arrow
  - small fireball
  - large fireball
  - snowball
- Added mob-owned projectile fields:
  - owner type
  - owner mob type
  - damage
- Mob projectiles can hit the player.
- Player-thrown projectiles still use the existing player-owned path.
- Fireballs are rendered and fly mostly straight instead of using normal gravity.
- Snowballs and arrows have basic visual/impact handling.

### Villagers and villages

- Added simplified village-based villager spawning:
  - checks chunks where the existing worldgen says a village can spawn
  - spawns villagers around generated village centers near the player
  - tries to maintain a small local village population
- Added simplified iron golem village spawning:
  - if villagers are present and no golem is nearby, a golem may spawn

### Golems and mob-vs-mob combat

- Zombies target villagers.
- Iron golems target hostile mobs.
- Iron golems hit hostile mobs with heavy damage and vertical knock-up.
- Snow golems target hostile mobs and shoot snowballs.

### Save data

- Bumped passive mob save version to `24`.
- Added saved fields for:
  - attack cooldown
  - burn timer
  - target mob index
  - baby age placeholder
  - sitting placeholder
  - held block placeholder

## Remaining non-1:1 areas

These are still not exact Java 1.2.5:

1. **AI/task system**  
   Java 1.2.5 mobs use `EntityAI*` tasks and priorities. PexCraft still has a simpler per-tick state machine.

2. **Pathfinding**  
   Java uses `PathNavigate` and path entities. PexCraft still steers directly toward targets/wander points.

3. **Line of sight and ranged combat**  
   Skeletons shoot arrows, but the full Java bow aiming curve, visibility checks, enchantments, pickup rules, and arrow NBT are not complete.

4. **Explosions**  
   Creepers and fireballs use damage/sound/effect handling, but not a full Java 1.2.5 `Explosion` implementation with block destruction, fire, entity knockback, and drop chance.

5. **Villages**  
   Villager spawning is tied to the existing PexCraft village generator, not Java's `ComponentVillage` spawn counters, `VillageCollection`, valid wooden doors, reputation, mating, or house population rules.

6. **Iron golem exact behavior**  
   Golem targeting and attacks are present, but Java's village reputation/player anger/rose-holding details are still simplified.

7. **Models/rendering**  
   The previous patch copied many `Model*.java` cuboids and this patch adds a skeleton bow item, but PexCraft still does not have Java's full `ModelRenderer` hierarchy or all `RenderLiving` render passes. Exact overlays such as spider eyes, enderman eyes, slime translucent shell, dragon eyes, and held-item transforms are still not structurally identical.

8. **Ender Dragon**  
   Still not 1:1. Java 1.2.5's dragon depends on flight history samples and boss logic that PexCraft does not store.

9. **Nether/End-specific spawning**  
   The overworld flat-world spawn rules are implemented, but Nether fortress/end dimension spawning systems are not present in PexCraft.

10. **Mob persistence/NBT**  
   PexCraft has a custom flat save format. It does not serialize Java 1.2.5 entity NBT 1:1.

## Build attempt

I attempted the Linux SDL2 build in this container. It still cannot complete here because the environment lacks full SDL2/OpenGL development headers and the temporary stub headers are incomplete. The logged errors are SDL/OpenGL stub/environment errors, not new mob patch errors detected in `src/game/passive_mobs.c` or `src/game/inventory.c`.

See `PexCraft_125_1to1_build_attempt.log` for the full build output.
