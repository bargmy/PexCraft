# PexCraft streaming/ring/lighting fix v11

## Implemented

- Replaced the desktop 512-block sliding voxel volumes with a 576-block physical ring.
  Absolute X/Z coordinates map modulo the ring, so moving the active window no longer
  memmoves the five block/meta/fluid/sky/block-light volumes.
- A one-chunk recenter now remaps only small chunk/section metadata tables. The previous
  roughly 310 MiB payload copy (roughly 600+ MiB memory read/write traffic) is gone.
- Added absolute chunk identity stamps to logical slots. Collision, lighting, streaming,
  saves, multiplayer updates, and mesh snapshots verify that a slot still owns the
  expected world chunk.
- Added a publication lock around origin/metadata remaps. Collision and world rendering
  cannot observe a new origin with old logical metadata.
- Preserved useful absolute-coordinate terrain jobs across window slides instead of
  globally clearing/restarting the generation queue.
- Added movement-predicted critical streaming priority. The current 3x3 footprint and
  forward movement corridor outrank distant render-radius completion.
- Increased near-player mesh submission/install throughput with bounded time budgets.
- Mesh jobs/results now carry absolute chunk identity and can still install after a
  one-chunk ring slide when their terrain/light revisions remain current.
- Split provisional light readiness from validated neighbor-aware lighting.
- Newly installed chunks deterministically queue bounded chunk-sized validation for
  themselves and existing horizontal neighbors.
- Lighting dirty work remains in independent 16x16 jobs rather than merging adjacent
  chunks into a single huge full-height rectangle.
- Skylight validation checks every column instead of every other column.
- Save snapshots and multiplayer chunk/block paths now use physical ring addressing.

## Resulting architecture

Crossing a chunk boundary changes the logical origin and recycles only the entering
chunk slots. Existing voxel/light data stays at stable physical addresses. New terrain
is generated into worker buffers and copied only for the newly entering chunk; the
entire active world is never shifted.

## Deliberately not included in this patch

A full conversion from dense full-height chunk data to individually allocated sparse
section objects, nibble-packed lighting, and zero-copy worker-buffer ownership would be
a separate storage-format rewrite. This patch removes the pathological recenter copy
and correctness races without changing block storage formats or save format v12+.
