> **Superseded:** The synchronous button-callback teardown described here was replaced by the responsive staged design in `save_and_quit_progress_teardown.md`.

# Save and Quit: full world-session teardown

## Required behavior

`Save and Quit to Title` is a hard lifecycle boundary. When the button returns,
there must be no active world simulation, network receive, chunk generation,
lighting, mesh building, mob rendering, cache writing, jukebox stream, game music,
or world-owned renderer object left from the previous session.

For a local world, the final save must also be finished on disk before in-memory
world ownership is released.

## Why the old path was incomplete

The old `leave_world_to_title()` only disconnected networking, queued an
asynchronous save, cleared the loaded-world name, and switched screens. It did not
join the world worker pools or release world state. A jukebox record therefore kept
playing after the title screen appeared.

Title music had a separate lifecycle defect: it was started by the one-time boot
sequence only. Returning to `SCREEN_TITLE` reset `g_menu_music_started`, but did not
request another menu track.

## New teardown order

1. Stop every current world-audio job.
   - SDL2 halts all mixer channels and legacy music playback.
   - Native Windows increments a sound-session generation; detached `waveOut` jobs
     detect the generation change and reset themselves. A new title track captures
     the new generation and is not cancelled.
2. Stop and join the asynchronous simulation worker.
3. Disconnect multiplayer and join the receive/cache workers, or stop any dormant
   multiplayer cache writer for a local session.
4. Stop and join the world-stream service, terrain workers, and lighting workers.
5. Stop and join section-mesh/D3D11 upload workers and the passive-mob render worker.
6. Drain all older asynchronous save snapshots.
7. Write the final local-world save synchronously.
8. Release all world-owned renderer meshes, display-list handles, and CPU meshes.
9. Clear chunk ownership/validity, entities, block entities, effects, particles,
   projectiles, vehicles, maps, jukeboxes, and other world-session state.
10. Destroy shared world/light locks only after every possible user has exited.
11. Clear the loaded-world identity and enter the title screen.
12. Start title music after the title UI has been rebuilt.

The large static voxel ring is not pointlessly zero-filled during exit. Its ownership
and validity maps are cleared, making old bytes unreachable; the next world overwrites
them normally. This avoids turning Save and Quit into a long memory-bandwidth stall
while still leaving no active or addressable previous world.

## Restart behavior

Every stopped subsystem resets its initialized/stop/event/thread state. New-world
entry recreates the shared map/light locks before simulation or mesh workers start,
so opening another world in the same process is supported.

## Validation performed

- Checked teardown ordering with source assertions.
- Enumerated world-owned `CreateThread` sites and matched them to shutdown paths.
- Checked delimiter and preprocessor balance for all modified C/header files.
- Verified the complete patch applies cleanly to the uploaded v11 source tree.
- Attempted the Linux SDL2 build; this environment does not contain the SDL2,
  SDL2_image, OpenGL, or GLU development headers, so an executable runtime test was
  not possible here.
