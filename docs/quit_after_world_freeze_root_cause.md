# Quit-after-world freeze: root cause and fix

## Root cause

The title-screen **Quit Game** button is not the fault. It posts the quit message and
enters the normal application shutdown path. The freeze only appears after a world or
server session because that session starts several persistent worker pools.

The v11 shutdown path had three lifecycle defects:

1. `g_stream_async_threads` (terrain generation) had initialization and worker loops,
   but no shutdown function. Workers could still be generating or blocked on a full
   result ring while world and renderer state was being torn down.
2. `world_stream_service_shutdown()` deleted `g_flat_world_map_cs` before the async
   simulation and section-mesh workers were joined. Both workers call
   `flat_world_map_enter()`, so this is a use-after-destroy race on a critical section.
3. The two lighting workers lazily initialized their shared commit critical section.
   Both could race through the initializer on their first commit.

All worker joins used infinite waits, so any one of these races presents as a permanent
application/fullscreen freeze at Quit Game.

## Fix

- Added `stream_async_shutdown()` and joined/freed every terrain worker/result.
- Stopped async simulation before world streaming, then stopped section mesh workers.
- Deferred destruction of world-map/light locks until every worker that can use them is
  joined.
- Initialized the shared lighting commit lock before launching the two lighting workers.

## Correct final shutdown order

1. async game/simulation tick
2. world stream service
3. terrain generation pool and lighting workers
4. section mesh workers / D3D11 prebuild worker
5. shared world/light critical sections
6. textures, audio, renderer, window
