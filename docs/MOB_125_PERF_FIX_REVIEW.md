# PexCraft 1.2.5 mob systems — tick performance fix

This patch starts from `PexCraft-main-mobs-125-exact-systems-no-dragon-patch.zip` and addresses the Loggy snapshot where FPS dropped to 1 while render stayed below 1 ms.

## Root cause from the provided Loggy snapshot

- Render was not the bottleneck: render total was about 0.6 ms while the frame was about 603 ms.
- The hot path was the simulation tick: `Tick total`, `Ingame enqueue`, and `Ingame tick` were about 577 ms average / 820 ms current.
- The existing profile buckets did not wrap `update_passive_mobs()`, so almost all of that cost appeared as unaccounted time inside `Ingame tick`.
- The exact-systems mob patch recalculated full Java-style A* paths too often. In particular, `passive_path_drive()` forced a new path whenever the target was more than 8 blocks away. Any hostile mob chasing the player from normal detection range could therefore run a full 1.2.5-style `PathFinder` search every tick.

## Implemented fixes

### PathFinder repath cooldown fixed

Changed `passive_path_drive()` so distance-to-target alone no longer forces a repath. Mobs now keep following the current path and only rebuild when:

- they have no path,
- the path is finished,
- the recalc cooldown expires,
- or they are stuck.

If a path solve fails, the mob now gets a 12–31 tick retry cooldown instead of trying again every single tick.

### Per-tick path solve budget

Added `PEX_PATHFIND_BUDGET_PER_TICK`, currently set to 3. This prevents many mobs from all doing full A* in the same game tick. Mobs that do not get a path solve still steer toward their current target or existing path.

### Faster PathPoint lookup

The previous port used a linear scan through every allocated path point in `passive_path_open_point_125()`. This made one A* solve trend toward O(n²) behavior. I added a hash bucket table using the existing 1.2.5-style path point hash, so duplicate/open point lookups are now bucketed instead of scanning the whole point list.

### Spawn scans do not fight chunk streaming

Natural spawn/village spawn scans are skipped while the world-stream service is busy. Existing mobs still tick. This avoids probing partially streamed terrain during the same frames where the chunk workers are still installing/generating chunks.

### Spawn Y scan fast reject

`passive_mob_column_spawn_y_for_category()` now rejects ungenerated chunks once before scanning Y levels, instead of checking the same ungenerated chunk repeatedly from every Y candidate.

### Profiling added around mob tick

`ingame_tick()` now wraps `update_passive_mobs()` in the profile bucket `PROF_ENTITY_PASSIVE_MOBS`, so future Loggy snapshots show whether mob tick is still hot instead of hiding it inside `Ingame tick`.

### Loggy diagnostic line added

The Win32 Loggy gameplay section now prints:

```text
mobs active=<n> path_budget_left=<n> spawn_scan_skipped_stream=<0/1> mob_tick_ms=<ms>
```

This should make the next snapshot much more actionable.

## What this does not change

- Ender Dragon remains untouched.
- Mob behavior/AI features from the previous exact-systems patch remain in place.
- This does not remove Java-style A*; it makes it behave like a cooldowned navigation system instead of solving constantly.
- This does not claim perfect 1:1 parity; it prioritizes preserving the 1.2.5-like decisions while making the game playable.

## Build status

The normal Linux SDL2 build still cannot complete in this container because SDL2/SDL2_image headers are missing. A syntax-only check with local minimal stubs still stops on unrelated incomplete SDL/OpenGL stub coverage before reaching the modified mob tick code.
