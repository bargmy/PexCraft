# PSP-1000 Multiplayer-Only Target

This source tree contains a dedicated low-memory Sony PSP-1000 build profile.

## Target behavior

- Java Edition multiplayer only.
- Java protocol 47 only (Minecraft 1.8.x wire protocol).
- Singleplayer remains visible but disabled on the title screen.
- Singleplayer world entry points and ticking are disabled for this target.
- Local NBT world save/load is not compiled into the PSP unity entry.
- The 3D title panorama is preprocessor-disabled and replaced by the normal 2D background.
- Render distance defaults to 2 and the frame cap defaults to 30 FPS.
- Downloaded player skins and server favicons are disabled; the embedded default skin is used.
- Protocol queues, entities, server list entries, mesh queues, texture slots, GU list, and immediate vertices use PSP-1000-specific limits.

`worldgen/level.c` is still included by the legacy unity translation unit because inventory, biome-color, and multiplayer terrain code share its types and helper routines. The PSP multiplayer build cannot enter or initialize a local world, and unreferenced generator functions are placed in individual sections and discarded by `--gc-sections`.

## Texture bundle

The PSP CI builder first runs `tools/prepare_wasm_assets.py`. This downloads the same Minecraft 1.2.5 client JAR used by the HTML/WASM target and verifies the same SHA-1. The resulting verified `Release` tree is then passed to `tools/prepare_psp_assets.py`, which converts supported PNG files into MCRW textures and packs them into one `.incbin` payload for the EBOOT.

The PNG tree is not embedded a second time, which avoids wasting several MiB on PSP-1000.

## Build

Trigger the PSP GitHub Actions job using the repository's existing `(psp-only)` or `(console-only)` commit-title convention. The workflow prepares the verified asset bundle and builds with the `pspdev/pspdev` Docker image.

For a local PSPSDK build, prepare the same two asset stages shown in `.github/workflows/build.yml`, then run:

```sh
make -f Makefile.psp clean
make -f Makefile.psp
```

## Current server restrictions

- Online-mode login encryption/session authentication is not implemented by the existing protocol 47 client. Use an offline-mode server or a compatible proxy.
- The PSP must already have a working Infrastructure Wi-Fi profile.
- The active PSP terrain window stores the lower 128 world blocks and uses a 96x96 horizontal ring.
