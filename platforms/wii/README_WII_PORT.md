# PexCraft Wii port

This is the native Wii build path.  Wii-specific code lives under `platforms/wii/`, while the source entrypoint is `src/main_wii.c` with a root `main_wii.c` wrapper for the Makefile/CI job.

## Build target

```sh
make -f platforms/wii/Makefile.wii
```

Expected toolchain:

- devkitPro / devkitPPC
- libogc
- fatfs/libfat
- wiiuse/WPAD
- Python 3 + Pillow for generating the embedded texture bundle

The build output is:

```text
build/wii/boot.dol
```

## Embedded texture bundle

The Wii build must not depend on external texture files for gameplay.  It follows the PSP-style flow:

1. `tools/prepare_wii_assets.py` downloads `client.jar` during CI.
2. The needed PNGs are converted to `.mcrw`.
3. The `.mcrw` files are packed into `build/wii_generated/wii_mcrw_assets.pak`.
4. `build/wii_generated/wii_mcrw_assets.S` embeds that pak with `.incbin`.
5. `platforms/wii/Makefile.wii` links the generated object into `boot.dol`.

So the Wii artifact only needs:

```text
sd:/apps/pexcraft/boot.dol
```

The app may still create saves/options on SD, but textures are loaded from the DOL, not from `sd:/apps/pexcraft/assets/` or `sd:/apps/pexcraft/texturepacks/`.

## Files added

- `src/main_wii.c` - Wii unity entry
- `main_wii.c` - root wrapper
- `platforms/wii/wii_compat.h` - Win32/OpenGL compatibility shim for Wii
- `platforms/wii/wii_gx_renderer.c` - GX fixed-function renderer shim
- `platforms/wii/wii_filesystem.c` - SD-card filesystem/timing/profile layer
- `platforms/wii/wii_input.c` - Classic Controller, GameCube Controller, Wiimote+Nunchuk input
- `platforms/wii/wii_app.c` - Wii app bootstrap and main loop
- `platforms/wii/wii_classic_pack_embedded.c` - embedded-DOL Classic pack handler
- `platforms/wii/Makefile.wii` - devkitPPC build file
- `tools/prepare_wii_assets.py` - client.jar -> MCRW -> embedded pak generator

## Controller priority

1. Classic Controller / Classic Controller Pro
2. GameCube Controller
3. Wiimote + Nunchuk

Classic Controller mapping:

- Left stick: move
- Right stick: look
- A: jump / select
- B: sneak / back
- ZR: break / attack
- ZL: place / use
- L/R: hotbar previous/next
- Y: inventory
- X: drop
- +: pause
- HOME: exit

## Runtime layout

Minimum app layout:

```text
sd:/apps/pexcraft/boot.dol
sd:/apps/pexcraft/meta.xml
```

Writable runtime data:

```text
sd:/apps/pexcraft/saves/
sd:/apps/pexcraft/skins/       optional/custom only
```

## Notes

- Networking/multiplayer is disabled for the Wii build.
- Runtime Classic pack downloading/extraction is disabled; the Classic textures are embedded at build time.
- The Wii build uses the PSP-sized 128x128x128 active world window, not the PC-sized 512x256x512 window.

## Embedded texture memory

The Wii build links the generated client.jar/MCRW texture pak into the normal DOL read-only data area. Do not force this blob to a high address such as `0x90010000`: that introduces a 256MB address hole and can produce a massive DOL in Dolphin/elf2dol. The runtime texture bundle is still inside `boot.dol`; it is not loaded from SD.

The GX renderer also splits indexed terrain draws into chunks of at most 65,535 vertices, because `GX_Begin()` takes a 16-bit vertex count.  Writing more vertices than the count passed to GX corrupts the FIFO and can show up in Dolphin as invalid MEM1/MEM2 pointer warnings.

## DOL size sanity check

Do not trust the `.zip` size. A broken DOL can compress to under 1 MB while
still being hundreds of MB when extracted, because address holes/zero padding
compress extremely well. The Wii Makefile and GitHub Actions workflow now check
the **uncompressed** `boot.dol` and fail if it exceeds 16 MB.

A normal embedded-texture Wii build should be a few MB, not hundreds of MB.

## Dolphin diagnostics

The Wii build prints boot stages through `OSReport()` and also brings up an early libogc text console before GX starts. In Dolphin, enable OSReport logging or check the log window for lines beginning with:

```text
[PexCraft/Wii]
```

Important stages now logged:

- `main entered`
- `init_dirs / fatInitDefault`
- `fatInitDefault -> ok` or `FAILED; continuing without SD saves/options`
- `renderer init / GX`
- GX video mode, XFB and FIFO allocation
- embedded texture pack lookups and uploads
- `set title screen`
- `present #1/#2/#3`
- periodic `heartbeat` lines every 60 frames

The game should continue booting even if Dolphin cannot create/open `sd.raw`; in that case saves/options are disabled for the session but textures still come from the embedded DOL bundle.
