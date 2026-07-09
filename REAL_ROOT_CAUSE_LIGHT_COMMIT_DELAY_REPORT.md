# Real root cause: streamed chunks were generated, but commit/lighting kept them delayed

The new Loggy output proves the delay is not guessing anymore.

Important sample:

```text
SCREEN=29(CHAT)
gen_queue index=120 total=132 remaining=12 installed=49
async_workers=3 jobs=6 active=3 results=61
stream_worker last=689.717 avg=221.137 terrain=60.659 delta=0.048 light=0.243 pushwait=0.001
light_worker last=1006.885 avg=256.893 dirty=1 ready=1
```

Meaning:

- `terrain=60.659ms`: the last terrain generation phase was slow-ish, but it was not the 689ms service stall.
- `pushwait=0.001ms`: worker result publishing was not blocked.
- `results=61`: 61 chunks were already generated and waiting to be installed into the visible world.
- `light_worker last=1006.885ms` and `light_dirty=1`: the expensive delay was the lighting repair path.

## Actual code-level cause

Runtime chunk streaming already computes light in the worker:

```c
flat_compute_chunk_light(r.buf, r.sky, r.blocklight, job.dimension);
```

Then install copies those light buffers into the world:

```c
flat_copy_light_buffers_to_world(r.cx, r.cz, r.sky, r.blocklight);
```

But the old code still did two bad things:

1. `flat_copy_light_buffers_to_world()` marked the chunk as light-not-ready after copying valid light.
2. `stream_mark_chunk_generated()` called `flat_mark_light_dirty_region()` for every streamed chunk.

`flat_mark_light_dirty_region()` does not keep separate small jobs. It merges regions by expanding one bounding rectangle:

```c
if (x0 < g_flat_light_x0) g_flat_light_x0 = x0;
if (z0 < g_flat_light_z0) g_flat_light_z0 = z0;
if (x1 > g_flat_light_x1) g_flat_light_x1 = x1;
if (z1 > g_flat_light_z1) g_flat_light_z1 = z1;
```

So when 40-60 streamed chunks finish, they collapse into one huge relight rectangle. That is why the log showed a ~1 second light worker even after previous full-window-light fixes.

## Patch behavior

- Streamed chunks are marked light-ready after their generated light buffers are copied.
- Runtime streamed chunks no longer enqueue background full relight regions by default.
- Completed chunk-result install budget is increased because install is now cheap enough without huge light-repair spam.
- Worker submission gets back-pressure when result backlog is already high, so `results=61` cannot keep growing behind commit.

## Expected Loggy result

After this patch, while walking/opening chat around newly streamed chunks:

- `light_dirty` should usually stay `0` during normal terrain streaming.
- `light_worker last=1000ms+` should disappear unless a real block/light edit requires it.
- `results=` should drain faster instead of sitting around 50-60.
- `terrain=` may still show 40-100ms because Beta terrain generation is expensive, but generated chunks should not sit delayed behind a giant relight job.
