# Real root cause: world opened after only 49 chunks

The delay was not mainly the frame renderer and not mainly the per-chunk terrain worker.
The code was handing control to the player while the visible/render-distance world was still unfinished.

## Evidence from Loggy

The last useful log showed:

```text
SCREEN=29(CHAT)
gen_queue index=120 total=132 remaining=12 installed=49
async_workers=3 jobs=6 active=3 results=61
light_dirty=1
```

`installed=49` is the important number.

## Code path

World entry uses this sequence:

```c
worldgen_tick()
  -> flat_begin_initial_generation()
  -> wait until flat_initial_generation_active() is false
  -> flat_finish_initial_generation()
  -> finish_prepared_world_entry()
```

But `flat_begin_initial_generation()` was not loading the gameplay render radius.
It used `stream_initial_load_chunk_target()`, which returned `50` on desktop/Android.

The ring loader only allows complete rings:

- ring 0 = 1 chunk
- ring 1 = 8 more, total 9
- ring 2 = 16 more, total 25
- ring 3 = 24 more, total 49
- ring 4 = 32 more, total 81, but this would exceed 50, so it stops

So the loading screen actually preloaded **49 chunks**, not 50.

After entering gameplay, `update_infinite_world_streaming()` immediately ran the runtime stream-fill path, saw the real render-distance radius was not generated, and queued the rest of the visible area. That is why the log shows a new `total=132` queue and `results=61` after the world is already open.

## Why older fixes did not solve it

The previous patches tried to make the background queue less harmful. That reduced some symptoms, but it did not remove the design bug:

> gameplay starts before the visible world is loaded.

As long as world entry happens at 49 chunks and runtime render-distance needs 100+ chunks, the game will still feel delayed after entry.

## Fix

For desktop builds, initial world loading now preloads the same chunk radius as `stream_render_radius()`.

That moves the unavoidable terrain-generation cost back to the loading screen where it belongs, instead of hiding it behind chat/pause/ingame and making block/light/world behavior feel delayed.

Constrained platforms keep smaller caps:

- Wii: radius 1
- PSP-1000: radius 2
- PSP: radius 3
- Android/Android TV: max radius 4
- Desktop/Linux/Win32: full `stream_render_radius()`

Expected Loggy after world entry:

```text
gen_queue remaining=0 or very small
results=0 or draining quickly
installed should be far above 49 when render distance is above 3
```

If a delay remains after this, the next log needs to be taken after entry with the new `stream_service install=` line, but this patch removes the specific 49-chunk handoff bug shown in the provided log.
