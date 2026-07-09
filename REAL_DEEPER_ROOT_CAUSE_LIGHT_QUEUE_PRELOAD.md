# Deeper root cause: the delay was still in lighting, but not the same lighting path

This pass ignored the previously-patched guesses and followed the remaining hidden delay paths in `src/game/inventory.c`.

## Finding 1: initial preload installed terrain with no light buffers

The initial Beta batch path (`stream_generate_initial_batch`) extracts chunks from one big Beta canvas and commits them like this:

```c
flat_copy_chunk_buffers(wcx, wcz, s->buf, s->meta);
stream_mark_chunk_generated(lcx, lcz);
```

but it did **not** compute/copy `g_flat_sky_light` or `g_flat_block_light` for those chunks. Runtime async chunks did compute light (`flat_compute_chunk_light` on the worker), but loading-screen preload chunks did not.

That meant the loading screen could say the chunk was generated while the world still had zero/stale light buffers for those chunks. Later edits/repairs then had to fix light after entry, which is the exact kind of hidden post-load delay the Loggy output kept showing.

### Fix

The initial batch now has `sky` and `blocklight` buffers, computes light per extracted chunk, copies those light buffers into the world, then publishes the chunk as light-ready.

## Finding 2: the light dirty system was one global rectangle

This was the worse root cause:

```c
if (!g_flat_light_dirty) {
    dirty = requested region;
} else {
    dirty = union(dirty, requested region);
}
```

So unrelated tiny light repairs got merged into one giant bounding rectangle.

Example:

- repair at chunk -6,6
- repair at chunk 6,6
- repair at chunk 0,0

became one huge rectangle covering everything between those chunks. That explains `light_worker last=1000ms+` even when each individual source looked small.

### Fix

Replaced the single global dirty rectangle with a small dirty-region queue:

- overlapping/nearby regions merge
- unrelated regions stay separate
- the lighting worker processes one region at a time
- if the queue overflows, it merges into the region with the smallest area growth, not a global mega-rectangle

## Finding 3: no-op block light changes still queued lighting

For block changes where both light value and opacity were unchanged, the old code still called:

```c
flat_mark_light_dirty_around_block(x, y, z);
```

That queued lighting work even when the edit could not affect sky/block light.

### Fix

If light value and opacity are unchanged, the lighting path now returns immediately.

## Expected Loggy change

Instead of seeing one huge light worker spike like:

```text
light_worker last=1006ms dirty=1
```

small independent repairs should stay small. If Loggy prints `light_dirty=3`, that now means three queued dirty regions, not one merged mega-region.

## Files changed

- `src/game/inventory.c`

