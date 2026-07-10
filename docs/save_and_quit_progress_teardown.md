# Save and Quit: responsive staged teardown

## Problem fixed

The previous full-teardown patch performed thread joins, cache draining, final disk
saving, and renderer cleanup directly from the **Save and Quit** button callback.
That callback runs on the same thread that pumps the window and presents frames.
Any slow worker, socket, save, or GPU-resource release therefore stopped Windows
message processing and made the machine appear frozen.

There was also a shutdown wakeup hazard: lighting and section-mesh worker pools use
shared auto-reset events. A single event signal can wake only one waiter, leaving a
second worker asleep forever while shutdown waits on its handle.

## New lifecycle

Clicking **Save and Quit to Title** now immediately:

1. Stops all world audio, including jukebox records.
2. Changes to `SCREEN_SAVING_QUIT` and rebuilds a dedicated progress UI.
3. Blocks normal gameplay, network polling, world rendering, and input.
4. Sends non-blocking stop requests to every world-owned worker.
5. Starts a below-normal-priority teardown coordinator.

The coordinator performs the operations that may block without blocking the render
or OS-message thread:

1. Join the simulation worker.
2. Disconnect and join network/connect/cache workers.
3. Join stream, terrain-generation, and lighting workers.
4. Join section-mesh/upload and passive-mob rendering workers.
5. Drain older asynchronous save snapshots.
6. Write the final local save synchronously.
7. Destroy shared world/light synchronization objects after all users have exited.

When the coordinator reports completion, the main/render thread safely finishes
renderer-owned work:

1. Release multiplayer skin textures.
2. Release world meshes, display lists, and CPU mesh buffers in bounded batches per
   frame, updating the progress bar from 93 to 97 percent.
3. Clear world-owned memory and identity state.
4. Enter the title screen and start menu music.

## Responsiveness and deadlock protections

- The button callback contains no thread join and no disk save.
- The main loop continues to pump events, render, and present the progress screen.
- Stop requests are sent to all producers before waiting for any single producer,
  preventing lock-order dependencies between simulation, streaming, lighting, and
  mesh workers.
- Multi-worker lighting and mesh waits periodically recheck their stop flags, so an
  auto-reset event cannot strand a sleeping worker during shutdown.
- Worker priorities are lowered during teardown so the progress UI keeps CPU time.
- Diagnostics and frame snapshots do not enter synchronization objects while they
  are being destroyed.
- OpenGL/D3D renderer-owned textures and world meshes are released only on the
  render thread.
- Closing the application while the progress screen is active completes the same
  coordinator once instead of running a second competing save/shutdown path.

## Progress stages

The UI reports these stages:

- Stopping world activity
- Stopping simulation
- Disconnecting from server / stopping world cache
- Stopping terrain and lighting
- Stopping mesh workers
- Finishing pending saves
- Saving world to disk
- Closing world services
- Releasing world graphics
- Clearing world memory
- Done

The percentage is stage-based. It intentionally waits at the current stage when a
worker or disk operation is still completing rather than claiming false progress.

## Restart behavior

All stopped worker pools reset their handles, events, critical sections, queues, and
stop flags. Opening another world recreates the map/light locks and workers, so the
sequence can be repeated without restarting the client.

## Validation performed

- Verified that `leave_world_to_title()` performs only immediate non-blocking stop
  requests and coordinator creation.
- Checked delimiter and preprocessor balance for every modified C/header file.
- Verified request-stop functions exist for simulation, network/cache, stream,
  terrain, lighting, mesh/upload, and passive-render workers.
- Verified renderer-owned multiplayer textures and world meshes are finalized on
  the main/render thread.
- Verified the incremental mesh index visits every section/chunk/pass exactly once.
- Generated the patch from the uploaded full-teardown source and dry-applied it to a
  clean extraction.

A full executable runtime test was not possible in this environment because the
SDL2, SDL2_image, OpenGL, and GLU development headers are not installed. Windows
runtime testing should cover local worlds, multiplayer, an active jukebox record,
repeated enter/exit cycles, and closing the window during the progress screen.
