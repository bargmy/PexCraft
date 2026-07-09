# PexCraft streaming/lighting/block-delay root cause report

This is based on the pasted Loggy diagnostics and the current C code paths.

## What the log actually says

`SCREEN=29` is not the loading screen. In `ScreenId`, `SCREEN_CHAT` is value 29. Earlier `SCREEN=22` is `SCREEN_PAUSE`. The diagnostics were captured while the game was under a GUI screen, not plain `SCREEN_INGAME`.

The important pattern was:

```text
gen_queue index=110 total=312 remaining=202
queue: jobs=0 busy=0 results=86
stream_worker last=484ms avg=122ms
light_dirty=0
FRAME=5.14ms RENDER=4.86ms
```

That means the renderer is not the root problem. Lighting is also not currently dirty in that sample. The stream workers already produced results, but those results are not being installed/drained while the screen is chat/pause/inventory-style UI.

## Root cause

`ingame_tick()` is allowed to run under chat/inventory/container screens, but `world_stream_service_proc()` was hard-coded to process only when:

```c
g_screen == SCREEN_INGAME
```

So opening chat/loggy/pause/inventory froze the streaming service even though the world still rendered behind the UI and sometimes still ticked. This left large queues and result rings stuck, which kept global `stream_generation_active()` true and made world updates look delayed even with good FPS.

A second bug was that the platform main loops omitted `SCREEN_CREATIVE` from the list of screens that drive `ingame_tick_async_queue()`, even though `ingame_screen_allows_tick()` includes creative. Creative inventory could therefore stall simulation/stream startup on SDL2/desktop/mobile loops.

## Patch behavior

- Stream service now runs under `INGAME`, `CHAT`, `INVENTORY`, `CREATIVE`, `WORKBENCH`, `FURNACE`, `CHEST`, `DEATH`, and `PAUSE`.
- `set_screen()` ensures the stream service is alive when entering those world-backed screens.
- Platform tick loops now include `SCREEN_CREATIVE`.
- Loggy now prints screen names like `SCREEN=29(CHAT)` so numeric screen IDs do not hide the real state.

## What to verify after this patch

The bad log shape should disappear. While chat/pause/inventory is open, `results=` should drain and `remaining=` should move instead of freezing. If a delay remains with `remaining=0`, `results=0`, `light_dirty=0`, then the next target is mesh job/result install timing, not world streaming or lighting.
