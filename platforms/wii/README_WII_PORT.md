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

The Wii build links the generated client.jar/MCRW texture pak into a dedicated `.wii_assets` DOL section at `0x90010000` so the texture blob lives in MEM2, not in the 24MB MEM1 region used by the main executable/data/BSS.  `wii_reserve_embedded_asset_mem2()` bumps the MEM2 arena past the embedded pak before normal runtime allocations can use MEM2.

The GX renderer also splits indexed terrain draws into chunks of at most 65,535 vertices, because `GX_Begin()` takes a 16-bit vertex count.  Writing more vertices than the count passed to GX corrupts the FIFO and can show up in Dolphin as invalid MEM1/MEM2 pointer warnings.
