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

## Android / Android TV low-end renderer path

Android terrain sections are uploaded to retained GLES vertex/index buffers when a
worker result is installed. Visible frames only bind and draw those buffers; they no
longer rescan section indices or stream the complete terrain mesh every frame. The
GLES compatibility layer also batches immediate-mode GUI, text, and entity geometry
into one retained dynamic ring buffer and caches unchanged pipeline state.

Terrain lighting uses the packed block/sky-light value already stored in each vertex
and a 16 by 16 shader lightmap, so daylight and gamma changes update one tiny texture
instead of copying and reuploading every visible section. Opaque, alpha-tested, and
two-sided section groups are submitted separately, allowing ordinary opaque terrain
to use early depth rejection and back-face culling without changing block models or
textures.

The Android active world window is 320 blocks wide. This preserves the normal
8-chunk view distance plus its streaming border while reducing the five dense world
arrays from about 405 MiB to about 125 MiB. Mesh worker counts and queues scale down
on low-core shared-memory devices.

## Android GLES title-screen regression repair (v10)

The Android and Android TV compatibility renderer now keeps menu, text, GUI,
entities, and other compatibility geometry on a lightweight one-texture GLES2
shader.  The terrain lightmap shader is used only while drawing retained terrain
meshes.  This avoids the severe title-screen slowdown seen on some low-end
GLES2 drivers after the v9 all-purpose shader change.

The renderer also no longer allocates/orphans a fixed 2 MiB streaming VBO at the
start of every frame.  Deferred compatibility batches perform one exact-size
stream upload when flushed, while retained terrain meshes continue to stay in
persistent GPU buffers.  Android V-Sync now follows the same V-Sync and FPS-limit
condition as the desktop SDL path.

## Android frame-pacing, UI and renderer-resolution controls (v12)

Android mesh generation is deliberately throttled behind the render thread: low
priority workers yield while scanning section layers, queue depths are small, and
only a bounded number of snapshots/results are submitted or installed per frame.
Cancellation is checked throughout block, face and visibility loops, allowing Save
& Quit to abandon active builders promptly instead of remaining on **Cancelling mesh
builders**.

Inventory and Creative screens group item icons by atlas and defer durability/count
text until after icon drawing. Container screens may reuse the low-resolution world
backdrop for a short interval, preventing the complete world from being redrawn for
every UI frame while preserving the normal native-resolution interface.

Android title panoramas are rendered into a 256x256 target at 30 Hz, blurred with a
separable five-tap filter, then aspect-cropped and scaled to fill the full display.
The panorama is never stretched and the menu remains native-resolution.

Video Settings now includes **Renderer Resolution** on Android and Android TV. It is
saved as a percentage in ten-percent increments from 10 to 100. The maximum is shown
as **Native** and the minimum as **VHS**. PexCraft queries the actual GLES drawable,
calculates both dimensions from the same percentage, renders the 3D world to that
size, and linearly scales it to fill the screen. GUI and text stay at native
resolution. Because only the percentage is stored, moving the same options file
between 720p, 1080p and 4K displays remains valid.

## Android TV remote, Java chunk retention and death-input repair (v13)

Android TV remotes are handled as ordinary D-pad/keyboard navigation rather than
being appended to the SDL gamepad list. Remote input no longer switches prompts,
virtual cursors, chat entry, or inventory layout into console mode. SDL devices
reported as remotes, CEC adapters, virtual devices, or `Generic Gamepad` are ignored
on Android TV, while real Xbox/XInput controllers remain on the controller path.
Mouse, stylus, touchscreen, gamepad, and joystick KeyEvents are excluded from the TV
remote JNI bridge so a mouse primary click cannot also become OK/jump.

Protocol-47 chunk snapshots that arrive outside the current 320-block Android ring
are retained until the multiplayer window reaches them. Visible snapshots are
processed first, newer complete snapshots replace every stale queued copy for the
same coordinate, and queue pressure can no longer silently discard a server chunk
and leave a permanent hole.

Entering the death screen now releases attack/use state and blocks mouse, trigger,
or remote attack controls until the physical buttons have been released once. This
prevents held or synthetic input from resuming as repeated interaction packets after
a server respawn.

## WASM validation and regular Android world-entry repair (v14)

The WASM structural validator now enforces the current Xbox/XInput-only D-pad
shortcuts instead of requiring the removed generic-controller expressions. Browser
builds therefore keep the remote-safe input behavior while passing the pre-build
check.

The regular Android GLES renderer is synchronized with the Android TV retained
terrain API. It now provides ranged terrain-mesh uploads, the seven-argument terrain
pass setup, split opaque/cutout/two-sided layers, and vertex-shader Java lighting.
This fixes the Android universal build errors and removes the stale phone-only
renderer path that failed when a world attempted to install or draw section meshes.
