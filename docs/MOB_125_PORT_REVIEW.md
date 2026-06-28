# Minecraft Java 1.2.5 mob port review for PexCraft

This patch expands PexCraft's old `passive_mobs.c` animal-only system into a registry-driven Java 1.2.5 living-mob layer. It adds every living mob ID present in Minecraft Java 1.2.5 and makes every corresponding spawn egg ID resolve to a spawnable entity.

## Important parity note

This is **not full 1:1 Java parity**. PexCraft does not currently contain Minecraft Java's entity inheritance tree, path navigator, AI task scheduler, projectile entities, terrain explosion system, dragon multipart boss logic, village/trade systems, taming ownership data, breeding/child age system, or biome/dimension-specific hostile spawn rules. Because of that, the Java methods cannot be matched function-for-function without first porting those missing engine subsystems.

What this patch does provide is a best-effort in the current PexCraft engine: all 1.2.5 mob IDs spawn, render with the correct texture family, use Java 1.2.5-like health/sizes/sounds/drop IDs, and have representative behavior/interactions where the existing engine can support them.

## Modified files

- `src/common/common.h`
  - Expanded `PassiveMobType` from 4 animals to 25 Java 1.2.5 living mob entries.
  - Declared new mob texture globals.
- `src/game/passive_mobs.c`
  - Added `PexMobInfo` registry mapping internal mob types to Java entity IDs, textures, sizes, health, sounds, drops, model family, hostile/passive flags, and attack values.
  - Replaced four-animal logic with registry-driven spawn, health, size, sound, drop, AI, and rendering paths.
  - Added basic hostile targeting/combat, creeper fuse damage, slime/magma hopping and sizing, ghast/dragon floating, enderman teleporting, squid water behavior, wolf/ocelot taming, mooshroom bowl/milk behavior, and iron golem/villager/snowman placeholder behavior.
- `src/game/inventory.c`
  - Added all 1.2.5 mob IDs to creative spawn eggs, including hidden/custom eggs for mobs that exist in Java 1.2.5 but did not have vanilla creative egg entries.
  - Updated passive mob health caps and death sounds.
- `src/render/item_render.c`
  - Fixed/expanded spawn egg colors and display names for all mob IDs.
  - Official Java 1.2.5 egg colors are used where Java had official egg metadata; non-egg mobs get custom colors.
- `src/assets/textures.c`
  - Added release-pack and texture-pack loading for Java 1.2.5 mob texture paths.

## Mob coverage table

| ID | Java 1.2.5 class | PexCraft type | Spawn egg | Texture | Basic behavior coverage | Not 1:1 / missing Java systems |
|---:|---|---|---|---|---|---|
| 50 | `EntityCreeper` | `PASSIVE_MOB_CREEPER` | Yes | `mob/creeper.png` | Hostile targeting, fuse sound, proximity damage, gunpowder drop | No terrain explosion, no powered creeper lightning state |
| 51 | `EntitySkeleton` | `PASSIVE_MOB_SKELETON` | Yes | `mob/skeleton.png` | Hostile targeting, ranged damage pulse, arrow/bone drops | No real arrow projectile, no sunlight burning |
| 52 | `EntitySpider` | `PASSIVE_MOB_SPIDER` | Yes | `mob/spider.png` | Fast hostile melee, spider model, string/spider-eye drops | No climbing, no light-level neutrality rules |
| 53 | `EntityGiantZombie` | `PASSIVE_MOB_GIANT` | Custom hidden egg | `mob/zombie.png` | Large scaled zombie, 100 HP, high melee damage | Java has no vanilla spawn egg; pathing/combat is simplified |
| 54 | `EntityZombie` | `PASSIVE_MOB_ZOMBIE` | Yes | `mob/zombie.png` | Hostile melee, rotten flesh drop | No sunlight burning, door breaking, reinforcement behavior |
| 55 | `EntitySlime` | `PASSIVE_MOB_SLIME` | Yes | `mob/slime.png` | Random size, hopping, melee, slimeball drop | No split-into-smaller-slimes on death, no chunk/slime-spawn logic |
| 56 | `EntityGhast` | `PASSIVE_MOB_GHAST` | Yes | `mob/ghast.png` | Floating hostile, ranged damage pulse, ghast tear/gunpowder drops | No fireball projectile or block damage |
| 57 | `EntityPigZombie` | `PASSIVE_MOB_PIG_ZOMBIE` | Yes | `mob/pigzombie.png` | Neutral until hit, then hostile melee, rotten flesh/gold nugget drops | No group anger propagation, no gold sword equipment |
| 58 | `EntityEnderman` | `PASSIVE_MOB_ENDERMAN` | Yes | `mob/enderman.png` | Hostile targeting, teleport near player, ender pearl drop | No block carrying/placing, no stare/aggression checks, no water damage |
| 59 | `EntityCaveSpider` | `PASSIVE_MOB_CAVE_SPIDER` | Yes | `mob/cavespider.png` | Small spider melee, string/spider-eye drops | No poison effect on hit |
| 60 | `EntitySilverfish` | `PASSIVE_MOB_SILVERFISH` | Yes | `mob/silverfish.png` | Small hostile melee | No block hiding/summoning other silverfish |
| 61 | `EntityBlaze` | `PASSIVE_MOB_BLAZE` | Yes | `mob/fire.png` | Hostile floating/ranged damage pulse, blaze rod drop | No fireball projectile, no fire immunity/state animation parity |
| 62 | `EntityMagmaCube` | `PASSIVE_MOB_MAGMA_CUBE` | Yes | `mob/lava.png` | Random size, hopping, melee, magma cream drop | No split-into-smaller-cubes on death |
| 63 | `EntityDragon` | `PASSIVE_MOB_ENDER_DRAGON` | Custom hidden egg | `mob/enderdragon/ender.png` | Large flying hostile placeholder, 200 HP | No multipart boss, crystals, End flight paths, death sequence, portal |
| 90 | `EntityPig` | `PASSIVE_MOB_PIG` | Yes | `mob/pig.png` | Saddle/mounting, raw pork drop, sounds/size/health | No lightning-to-pigman, no breeding/children |
| 91 | `EntitySheep` | `PASSIVE_MOB_SHEEP` | Yes | `mob/sheep.png`, `mob/sheep_fur.png` | Wool colors, shearing, wool drop, dye interaction | No grass-eating wool regrowth AI, no breeding/children |
| 92 | `EntityCow` | `PASSIVE_MOB_COW` | Yes | `mob/cow.png` | Milk bucket, leather/beef drops, sounds/size/health | No breeding/children |
| 93 | `EntityChicken` | `PASSIVE_MOB_CHICKEN` | Yes | `mob/chicken.png` | Egg timer, feather/chicken drops, falling wing behavior | No breeding/children |
| 94 | `EntitySquid` | `PASSIVE_MOB_SQUID` | Yes | `mob/squid.png` | Water mob flag, ink sac drop, squid-style render/animation | No exact tentacle physics/pathing |
| 95 | `EntityWolf` | `PASSIVE_MOB_WOLF` | Yes | `mob/wolf*.png` | Bone taming chance, angry-on-hit flag, wolf textures | No owner UUID/state, sitting/following, collar dye, pack AI |
| 96 | `EntityMooshroom` | `PASSIVE_MOB_MOOSHROOM` | Yes | `mob/redcow.png` | Milk bucket, bowl-to-stew interaction, cow-like drops | No shears-to-cow conversion, no breeding |
| 97 | `EntitySnowman` | `PASSIVE_MOB_SNOWMAN` | Custom hidden egg | `mob/snowman.png` | Snowman model and snowball drops | No snow trail, snowball projectile attack, biome heat/water damage |
| 98 | `EntityOcelot` | `PASSIVE_MOB_OCELOT` | Yes | `mob/ozelot.png`, cat variants | Raw fish taming chance, cat texture swap | No cautious approach AI, sitting, owner/following |
| 99 | `EntityIronGolem` | `PASSIVE_MOB_IRON_GOLEM` | Custom hidden egg | `mob/villager_golem.png` | Large golem model, 100 HP, iron/rose drops | No village defense AI, villager relationships, attack animation knock-up |
| 120 | `EntityVillager` | `PASSIVE_MOB_VILLAGER` | Yes | `mob/villager/villager.png` | Villager model/texture/sounds, 20 HP | No trades, professions, village door logic, mating AI |

## Function/system diff review

Java 1.2.5 implements each mob as a separate Java class with inherited behavior from `Entity`, `EntityLiving`, `EntityMob`, `EntityAnimal`, `EntityWaterMob`, `EntityGolem`, etc. PexCraft uses a compact C single-file entity array (`PassiveMob`) rather than separate class files. Because of that, the function diff is architectural rather than 1:1 by symbol name:

| Java responsibility | Java-side examples | PexCraft implementation in this patch | Match status |
|---|---|---|---|
| Entity ID/name registration | `EntityList.addMapping(...)` | `g_pex_mob_info[].entity_id`, `passive_mob_type_from_egg()` | Covered for all listed mobs |
| Per-mob dimensions | `setSize(width,height)` | `PexMobInfo.width/height`, slime-size adjustment | Mostly covered |
| Health | `getMaxHealth()` / constructor values | `PexMobInfo.health`, `pex_passive_health_cap_for_type()` | Covered for baseline values |
| Drops | `dropFewItems(...)` | `passive_mob_drop_on_death()` using registry drops and special cases | Partially covered; drop counts/chances simplified |
| Sounds | `getLivingSound`, `getHurtSound`, `getDeathSound` | Registry sound strings and existing PexCraft sound playback | Covered where sound keys/assets exist |
| Spawn egg color/name | `EntityList.entityEggs` | `spawn_egg_color()`, `spawn_egg_entity_name()` | Covered; custom colors for mobs with no official egg |
| AI task scheduling | `EntityAITasks` and specialized AI classes | `passive_mob_tick_ai()` procedural behavior | Approximate only |
| Navigation/pathfinding | `PathNavigate`, world collision/path cost | Existing random target plus collision jump recovery | Approximate only |
| Projectiles | arrows, fireballs, snowballs | Immediate ranged damage pulse with sound | Not 1:1 |
| Explosions | creeper/ghast fireball terrain effects | Creeper sound + player damage only | Not 1:1 |
| Boss/multipart entities | dragon parts/crystals/death sequence | Single large placeholder mob | Not 1:1 |
| Taming/ownership | wolf/ocelot owner/sit/follow | One-bit tamed flag in existing `sheared` field | Approximate only |
| Breeding/children | animal love mode and age | Not implemented | Missing |
| Trading/village systems | villagers/golems/villages | Not implemented | Missing |

## Build/verification

- A Linux SDL2 build was attempted with `make -f Makefile.linux_sdl2`.
- This environment does not have `libsdl2-dev` or `SDL2_image` installed, so the compile stopped before C syntax validation at `SDL2/SDL.h`.
- Generated build log: `PexCraft_mobs_125_build_attempt.log`.
- Static checks performed in the patching environment:
  - Modified C files have balanced braces.
  - All inserted `steve_part(...)` render calls were arity-checked against the 16-argument macro contract.
  - Diff generated against the original uploaded PexCraft zip.

## Diffstat

Initial patch diffstat before this review file:

- `src/assets/textures.c`: +87 / -1
- `src/common/common.h`: +28 / -1
- `src/game/inventory.c`: +49 / -7
- `src/game/passive_mobs.c`: +433 / -85
- `src/render/item_render.c`: +8 / -0

## Remaining work required for true 1:1 parity

To make this genuinely 1:1 with Java 1.2.5, the next step is not more switch cases. It requires porting or reimplementing these Minecraft Java subsystems in PexCraft:

1. `Entity`/`EntityLiving` inheritance-equivalent data and tick lifecycle.
2. Java AI task scheduler and all mob-specific AI tasks.
3. Path navigation with proper world path costs and door/water/lava rules.
4. Projectile entities: arrows, fireballs, snowballs, potion effects.
5. Explosion system with terrain damage and gamerule-style handling.
6. Breeding/age/owner/tame/sit/follow persistent data.
7. Village/trading/profession/door/golem logic.
8. Ender Dragon multipart boss and End crystal healing/death sequence.
9. Dimension/biome/light-level spawn rules for hostile, passive, water, and ambient mobs.
