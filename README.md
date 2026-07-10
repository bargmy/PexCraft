# PexCraft

PexCraft is an experimental custom client written in C. It is trying to work like **Minecraft JE r1.2.5**, with a focus on stability, predictable behavior, and cross-platform support.

> [!IMPORTANT]
> **PexCraft is not affiliated with, supported by, or approved by Mojang AB, Minecraft, or Microsoft in any way.**  
> **PexCraft is not trying to replicate modern Minecraft.**  
> **The target is Minecraft Java Edition r1.2.5-style behavior, while keeping the project stable, portable, and usable on many platforms.**

## Current platform state

- PSP State = barely playable.
- Windows State = playable.
- Linux SDL2 State = playable.
- Android TV State = build target added; uses SDL2/OpenGL ES and TV remote/gamepad input.

## Renderer states

- Direct3D 9 = unusable.
- Direct3D 11 = working.
- OpenGL = working.
- SDL2-OpenGL = working.

## Android TV

See `android_tv/README.md` for the APK build, signing, TV remote mapping, and Google TV channel integration.

## Responsive Save and Quit lifecycle

**Save and Quit to Title** now uses a live progress screen and a staged teardown.
Thread joins and final disk saving run on a below-normal-priority coordinator while
the main thread continues pumping the window and rendering progress. Renderer-owned
textures and world meshes are then released safely on the render thread in bounded
per-frame batches before title music starts.

See `docs/save_and_quit_progress_teardown.md` for the shutdown order, deadlock
protections, and validation notes. The earlier synchronous implementation is retained
only as historical context in `docs/save_and_quit_full_world_teardown.md`.

### Mesh-worker shutdown safety

The quit coordinator now cooperatively cancels active section builds, drops queued
mesh jobs, polls each worker handle independently, and reports per-worker state in
the log. Direct3D 11 buffer creation is no longer performed on a background worker
while the progress screen is presenting; CPU mesh results are uploaded by the
render thread through the existing bounded install path.

See `docs/save_and_quit_mesh_worker_fix.md` for the failure mechanism and changes.
## Linux/OpenGL mesh shutdown correction

This revision fixes the SDL pthread compatibility layer used by Linux/OpenGL.
Zero-time thread waits can now detect completed pthreads, and finite event waits
actually time out. This prevents **Cancelling mesh builders** from waiting forever.
See `docs/linux_opengl_mesh_shutdown_fix.md`.

