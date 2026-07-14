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


## WebAssembly / standalone HTML

An offline single-player browser target is available through `main_wasm.c`. It uses the existing GLES2 compatibility renderer, embeds the verified Minecraft 1.2.5 textures/fonts and the WebAssembly module into one `dist/PexCraft-WASM.html`, stores saves in browser IndexedDB when available, and disables multiplayer.

Build with `./build_wasm.sh` or `make -f Makefile.wasm`. See `docs/WASM_BUILD.md` for resource verification, offline/network guarantees, Telegram-only CI delivery, and browser validation.

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


## Gamepad and TV-remote UI controls

Menus now use spatial D-pad/left-stick focus, so two-column screens navigate by
position rather than button creation order. A/OK activates, B/Back returns, and
sliders use Left/Right (LB/RB for larger steps). Language and texture-pack lists,
Statistics, Achievements, Creative inventory, multiplayer lists, and all text-entry
screens have dedicated couch-friendly controls and on-screen hints. Controller-focused
text fields open the built-in virtual keyboard on every controller platform.

The Xbox UWP target polls physical controllers through `Windows.Gaming.Input` and
keeps CoreWindow Gamepad virtual keys as a media-remote fallback when no physical
controller is connected.

## Build commit information

The supported build scripts run `tools/generate_build_info.py` before compiling.
It reads the checked-out Git commit subject, full SHA, and `origin` repository URL,
then force-includes an ASCII-safe generated header. When Git metadata is unavailable,
the embedded name, SHA, repository, and commit URL remain empty.

On the title screen, click `PexCraft 1.2.5` to show or hide the embedded commit
subject and SHA. Clicking either visible line opens the exact commit URL in the
platform browser when that platform supports external URLs.
