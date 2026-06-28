# PexCraft 1.2.5 mob engine-layer patch review (no Ender Dragon changes)

## Scope

This patch starts from `PexCraft-main-mobs-125-closer-no-dragon-patch.zip` and implements another non-dragon pass over the missing engine-level systems that were still called out as incomplete:

- Java 1.2.5-style `EntityAI` task selection layer.
- A compact A* path navigation layer for ground/water/flying mobs.
- Runtime village door collection, villager/golem population rules, and simple reputation.
- Persistent owner/tame metadata for wolves and cats/ocelots.
- Additional render-pass parity: slime translucent shell, pig zombie held sword, golem rose, and saved render/tame state support.

The Ender Dragon model/AI/flight-history logic is intentionally not changed in this pass.

## Files changed in this pass

- `src/common/common.h`
- `src/game/passive_mobs.c`

## Engine systems added

### 1. EntityAI-style task layer

Added task bits that map to the major Java 1.2.5 `EntityAI*` classes used by these mobs:

- `PEX_AI_SWIM`
- `PEX_AI_FLEE_SUN`
- `PEX_AI_ATTACK`
- `PEX_AI_ARROW_ATTACK`
- `PEX_AI_MATE`
- `PEX_AI_TEMPT`
- `PEX_AI_FOLLOW_PARENT`
- `PEX_AI_MOVE_VILLAGE`
- `PEX_AI_WANDER`
- `PEX_AI_SIT`
- `PEX_AI_AVOID_PLAYER`

`passive_mob_tick_ai()` now assigns task bits before movement. Mating, baby-parent following, food temptation, attacking, sitting, village movement, and wandering are no longer indistinguishable straight-line behavior internally.

### 2. A* path navigation

Added a compact `PathNavigate` equivalent:

- `passive_path_find()` searches walkable nodes around the mob and target.
- `passive_path_drive()` moves the mob through waypoints.
- Ground mobs use `passive_mob_ground_target_ok()`.
- Squid use water nodes.
- Ghast/blaze can path through air nodes.
- Paths are stored per mob with `PEX_MOB_PATH_MAX` waypoints.
- Path state is runtime-only; it is rebuilt after loading instead of being saved.

This is much closer than the previous straight-line steering, especially around village buildings, fences/walls, ledges, and obstacles.

### 3. Village, door, villager, golem, and reputation layer

Added runtime village tracking:

- `PexVillageRuntime`
- `PexVillageDoorRuntime`
- `passive_villages_refresh()`
- `passive_village_scan_doors()`
- `passive_villager_pick_door()`
- `passive_mobs_try_village_spawns_exact()`

Behavior now uses village doors instead of only fixed offsets:

- Villages are discovered from `worldgen_village_can_spawn()` around the player.
- Wooden doors are scanned in the generated village area.
- If actual doors are unavailable, a fallback virtual door set is created from the simplified PexCraft village layout.
- Villager target count is derived from door count using the Java-style `door_count * 0.35` rule.
- Iron golems spawn from villager/door thresholds instead of the old hardcoded “5 villagers” rule.
- Villagers choose doors at night / periodically and path toward them.
- Attacking villagers or golems lowers the single-player village reputation value.
- Iron golems can become hostile to the player when village reputation is low.

This is still not a full Java `VillageCollection` port because PexCraft does not have chunk-saved village NBT, per-player reputation maps, siege state, or exact door-side sky exposure scoring.

### 4. Owner persistence for tamed wolves/cats

Added to `PassiveMob` and save version 26:

- `owner_id`
- `tame_state`
- `collar_color`
- `village_id`
- door/path/task runtime state fields

Wolf and ocelot/cat taming now assign `PEX_MOB_OWNER_SINGLEPLAYER` instead of only overloading `sheared`. Older saves are migrated: existing tamed wolves/cats from the previous patch get owner id `PEX_MOB_OWNER_SINGLEPLAYER` when loaded.

### 5. Render-pass improvements

Added/changed:

- Slime now has a translucent outer shell pass closer to Java `RenderSlime`.
- Slime opaque model is split from outer-shell model.
- Pig zombies visibly hold a golden sword.
- Iron golems can render the rose item when not attacking.
- Render tint now supports alpha so translucent passes can work.

Existing render passes from previous patches remain: sheep fleece, pig saddle, spider eyes, enderman eyes, enderman carried block, mooshroom mushrooms, skeleton bow, ghast firing texture, villager professions.

## What this makes closer to 1.2.5

- Mobs navigate around obstacles instead of always walking directly into them.
- Villagers are tied to village doors and population counts.
- Village golems are tied to door/villager thresholds.
- Tamed wolf/cat ownership survives saving/loading.
- More Java render-pass/equipped-item behavior is visible in-game.
- AI decision categories are now represented explicitly, making future exact ports much easier.

## Still not perfectly 1:1

The patch is closer, but not a byte-for-byte or behavior-complete Minecraft 1.2.5 clone yet. Remaining gaps:

- The A* layer is compact and local; Java `PathFinder`/`PathEntity` does more detailed collision and entity-size accounting.
- Village reputation is single-player only and runtime-rebuilt, not a full saved `VillageCollection` NBT implementation.
- Door validation does not fully match Java’s inside/outside sky-count test.
- Exact `EntityAITasks` mutex/prioritization is approximated with task bits plus the existing tick order.
- Exact `ModelRenderer` parent-child transform hierarchy is still not globally reconstructed for every model part; this pass focuses on engine behavior and render passes.
- Ender Dragon remains intentionally untouched.

## Build validation

A real Linux SDL2 build was attempted with `make -f Makefile.linux_sdl2`, but this container is missing SDL2 and SDL2_image development headers, so compilation stops before source validation:

```text
fatal error: SDL2/SDL.h: No such file or directory
```

A local stub-header syntax check was also run far enough to parse `src/game/passive_mobs.c`; no passive-mob syntax errors appeared before the check later stopped in SDL platform-window stubs.
