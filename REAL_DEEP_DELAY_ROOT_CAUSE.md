# Real deep root cause: chunk commit/install, not generation

The useful proof was this Loggy contradiction:

```text
stream_worker last=689.717 avg=221.137 ... terrain=60.659 delta=0.048 light=0.243 pushwait=0.001
results=61
light_dirty=1
```

The label `stream_worker last` was misleading. It was timing the **world stream service loop**, not just the terrain worker. Terrain generation for the last chunk was about 60 ms, delta/light/push were basically free, but the service loop still took about 690 ms. That missing time was chunk **commit/install**.

## Bad path

```text
world_stream_service_proc
  -> update_infinite_world_streaming
     -> process_stream_generation_queue
        -> stream_async_install_ready(max_install)
           -> flat_copy_chunk_buffers
           -> flat_copy_light_buffers_to_world
           -> stream_mark_chunk_generated
```

When `results=61`, the service tried to install many completed chunks. One installed chunk is 16 x 16 x 256 = 65,536 cells for blocks/meta/levels and another 65,536 cells for sky/block light. The old copy path recalculated world indexes per cell, then scanned the entire chunk again for occupancy.

It also dirtied/bumped the chunk sections multiple times:

1. `flat_copy_light_buffers_to_world()` called `flat_publish_light_ready(..., dirty_sections=1)`.
2. `stream_mark_chunk_generated()` called `flat_publish_light_ready(..., dirty_sections=1)` again.
3. `stream_mark_chunk_generated()` then called `flat_mark_generated_section()` for every section.

So a completed chunk was not just copied; it was copied, rescanned, dirtied twice, then dirtied again for generated-section mesh rebuilds. With dozens of results waiting, this became the visible world delay.

## Fix

This patch changes the hot path, not the guessed generation path:

- Fast row-based chunk copy for full in-window streamed chunks.
- Computes the non-empty section mask while copying, so no second whole-chunk occupancy scan.
- Fast row-based light-buffer copy.
- Light copy no longer dirties every section; `stream_mark_chunk_generated()` owns the single mesh dirty pass.
- Runtime `stream_mark_chunk_generated()` sets light-ready without dirtying all sections twice.
- Stream install has a 10 ms runtime service budget so a backlog cannot turn one service wake into a 600 ms monopolizing commit.
- Loggy now separates:
  - `stream_service` = whole service loop / install time
  - `stream_chunk` = actual terrain/delta/light/push worker phases

Expected new Loggy shape:

```text
stream_service ... install=<small> n=<some> one=<small>
stream_chunk ... terrain=<actual terrain cost> delta=... light=... pushwait=...
results= drains instead of sitting around 60+
```

If `terrain` is high after this, generation is the next bottleneck. If `install` is still high, the commit path is still hot. This report makes those two paths visible separately.
