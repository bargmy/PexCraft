# Deep root cause: world/block delay

This is the code-level root cause, not a guess from the FPS number.

## 1. Runtime Beta chunk streaming is extremely expensive per queued chunk

The runtime worker path is `stream_async_worker_proc()` -> `flat_generate_chunk_base_with_meta()`.

For `world_type == 1` in the Overworld, one requested chunk does **not** generate one chunk. It allocates a 3x3 `GenCanvas`, runs `generate_canvas_chunk()` for all 9 chunks around the target, calls `worldgen_place_structure_blocks()`, then runs `qm_populate_canvas()` for 4 population chunks before extracting only the center chunk.

So a log line like:

```text
stream_worker last=484ms avg=122ms
```

matches the code: every runtime stream job is doing repeated neighbor generation and population work. The initial loading screen has a shared batch path, but runtime streaming falls back to this per-chunk 3x3 path.

The new loggy line now breaks worker time into:

```text
terrain=... delta=... light=... pushwait=...
```

If `terrain` is almost all of `stream_worker last`, the expensive 3x3 runtime generator is proven as the remaining stream delay.

## 2. Block placement visibility priority was not actually wired

The code already had a priority mesh submitter:

```c
async_mesh_submit_priority(int sy, int cx, int cz)
```

but it was never called anywhere.

A block edit only set:

```c
g_flat_recent_block_mesh_dirty_tick = g_ingame_ticks;
```

Then `rebuild_visible_flat_sections()` scanned the general visible section list and rebuilt the first dirty sections it found. Streaming chunk installs dirty lots of sections through `stream_mark_chunk_dirty()`, so the player's edited section could be behind background dirty sections even though it was the one that needed instant feedback.

## 3. What this patch changes

This patch adds an exact edit-priority section queue:

- `flat_mark_sections_dirty_near_block()` now records the exact dirty 16x16x16 sections caused by the edit.
- `rebuild_visible_flat_sections()` drains those edit sections before the normal streaming/background dirty-section scan.
- Very-near edit sections rebuild synchronously, matching Java's immediate `WorldRenderer` update feel.
- Farther edit sections use the existing priority async queue, which can drop a background job if the mesh queue is full.
- Loggy now prints:

```text
edit_priority queued=... drained=... sync=... async=... left=...
```

## 4. What to look for in the next log

If block placement still feels delayed, copy a log taken immediately after placing a block. The useful lines are:

```text
stream_worker ... terrain=... delta=... light=... pushwait=...
edit_priority queued=... drained=... sync=... async=... left=...
queue: jobs=... busy=... results=...
frame: submit_calls=... snapshot_cells=... installs=...
```

Interpretation:

- `edit_priority queued` increases but `drained/sync` does not: render path is not seeing/draining the edit queue.
- `sync` increases but visual still lags: mesh/list adoption or draw pass is stale.
- `terrain` dominates stream worker time: runtime chunk generation is the delay source.
- `pushwait` is high: result ring is full; install/drain is behind.
- mesh `results` is high while `installs` stays low: mesh result install budget is throttling visibility.
