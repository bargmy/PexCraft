# PexCraft Android TV port

This folder is the Android TV APK target. It builds the native C game through SDL2 and the OpenGL ES 2.0 compatibility renderer copied from the webOS path.

## What is included

- Android TV manifest with `LEANBACK_LAUNCHER`, TV banner, game category, non-touchscreen support, and gamepad/remote compatibility.
- Native `libmain.so` build from `src/main_android_tv.c`.
- `src/platform/android_tv/` platform layer for filesystem, lifecycle, GLES2, and TV remote input.
- TV remote mapping:
  - D-pad: menu/game movement
  - OK: select / jump
  - Back: pause / back
  - Red: break / attack
  - Green: place / use
  - Yellow: inventory
  - Blue: sneak / back
  - 1-9: hotbar slots
  - 0: camera toggle
- Google TV / Android TV home channel publisher with a `TYPE_GAME` preview program card.
- GitHub Actions release signing hook using the repository secret named `sign` as the base64 keystore.

## Local build

```bash
cd android_tv
./prepare_sdl2.sh
./gradlew assembleRelease 2>/dev/null || gradle assembleRelease
```

`prepare_sdl2.sh` downloads the current SDL2 Android development archive from the official libsdl-org GitHub releases, copies the SDL2 AAR, headers, and `libSDL2.so` files into the Android project, then CMake links the native game against them.

## GitHub signing secrets

Required by this workflow if you want a signed release APK:

- `sign`: base64 of the Android keystore/JKS file.

Optional, depending on how the keystore was created:

- `SIGNING_STORE_PASSWORD` or `SIGN_STORE_PASSWORD`
- `SIGNING_KEY_ALIAS` or `SIGN_KEY_ALIAS`
- `SIGNING_KEY_PASSWORD` or `SIGN_KEY_PASSWORD`

If the password/alias secrets are omitted, the Gradle config defaults to `pexcraft` for local/dev keystores.
