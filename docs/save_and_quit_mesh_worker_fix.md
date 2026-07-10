# Save and Quit: mesh-worker freeze fix

## Observed failure

The responsive teardown could remain at **Stopping mesh workers**. That stage
called `async_section_mesh_shutdown()` and waited indefinitely for every section
mesh handle. Two unsafe behaviors made that wait unbounded:

1. A busy CPU worker completed an entire dense section build before checking the
   stop flag. Queued jobs were not discarded at the stop boundary.
2. Direct3D 11 used a second background worker that called
   `ID3D11Device_CreateBuffer` while the main thread continued presenting the
   progress screen. A vendor driver is allowed to serialize those operations, so
   the quit coordinator could wait on a thread blocked inside the graphics driver.

The stage also included the passive entity-render worker, making the displayed
status ambiguous.

## New shutdown behavior

- The stop request atomically blocks submissions and clears jobs that have not
  started.
- Active section builders check cancellation at every section row and through the
  vertex/index reserve path. A cancelled build frees its private CPU buffers and
  does not publish a result or re-enter world locks.
- Direct3D 11 workers now publish CPU mesh builders exactly like the other
  backends. GPU buffers are created only by the render thread through the existing
  bounded result-install path.
- Shutdown polls all worker handles instead of performing an opaque infinite wait
  on the first handle. The progress percentage advances as workers close.
- Every worker exposes a diagnostic state: unused, waiting, building, publishing,
  or exited. A log line is emitted every two seconds only while a worker remains.
- Passive entity-render shutdown has its own progress label after mesh cancellation.
- The pool still destroys its event, critical section, queues, and state only after
  every mesh handle has exited, preserving restart safety for the next world.

## Performance tradeoff

D3D11 terrain buffer creation is moved back to the render thread. The existing
per-frame upload count and time deadline bound this work, so normal chunk loading
may show a small increase in upload-frame cost, but quitting no longer competes
with a background graphics-driver call. CPU section construction remains parallel.

## Validation performed

- Checked C delimiter and preprocessor balance in both modified source files.
- Verified the stop flag is observed from section iteration and buffer-reserve paths.
- Verified cancelled workers skip result publication and world-map retry locking.
- Verified no D3D11 upload event or upload thread is created by the active mesh path.
- Verified shutdown closes all four possible CPU worker handles before deleting the
  queue critical section.
- Verified teardown progress separates mesh builders from the entity-render worker.
- Dry-applied the generated patch to a clean copy of the responsive-save-and-quit
  source tree.

A Windows/D3D11 executable could not be run in this Linux environment, so the
final runtime check should cover quitting during heavy chunk generation and rapidly
re-entering another world.
