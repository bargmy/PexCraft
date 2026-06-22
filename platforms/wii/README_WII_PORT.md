# PexCraft Wii port scaffold

This is a first-pass native Wii port scaffold.  It keeps the existing PC/PSP/Android paths intact and adds a Wii-specific path under `platforms/wii/`.

## Build target

```sh
make -f platforms/wii/Makefile.wii
```

Expected toolchain:

- devkitPro / devkitPPC
- libogc
- fatfs/libfat
- wiiuse/WPAD

The build output is intended to become `sd:/apps/pexcraft/boot.dol`.

## Files added

- `src/main_wii.c` - Wii unity entry
- `main_wii.c` - root wrapper
- `platforms/wii/wii_compat.h` - Win32/OpenGL compatibility shim for Wii
- `platforms/wii/wii_gx_renderer.c` - GX fixed-function renderer shim
- `platforms/wii/wii_filesystem.c` - SD-card filesystem/timing/profile layer
- `platforms/wii/wii_input.c` - Classic Controller, GameCube Controller, Wiimote+Nunchuk input
- `platforms/wii/wii_app.c` - Wii app bootstrap and main loop
- `platforms/wii/wii_classic_pack_stub.c` - disables runtime Classic pack download on Wii
- `platforms/wii/Makefile.wii` - first-pass devkitPPC build file

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

Place the app at:

```text
sd:/apps/pexcraft/boot.dol
sd:/apps/pexcraft/assets/
sd:/apps/pexcraft/saves/
sd:/apps/pexcraft/texturepacks/
sd:/apps/pexcraft/skins/
```

## Current limitations

- This was prepared without a local devkitPPC/libogc environment, so it is a scaffold, not a confirmed booting DOL.
- Networking/multiplayer is disabled for the first Wii pass.
- Runtime Classic pack downloading is disabled; copy texture packs to SD manually.
- The renderer is a GX fixed-function shim and will probably need real-hardware iteration for texture formats, EFB clearing, and matrix orientation.
- The Wii build uses the PSP-sized 128x128x128 active world window, not the PC-sized 512x256x512 window.
