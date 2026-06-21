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

`prepare_sdl2.sh` downloads SDL2 source from libsdl-org, copies the Android Java shim into the project, and lets CMake build/link SDL2 with the native game.

## GitHub signing secrets

Required by this workflow if you want a signed release APK:

- `sign`: base64 of the Android keystore/JKS file.

Also required for signing:

- `SIGNING_STORE_PASSWORD` or `SIGN_STORE_PASSWORD`: the keystore password.
- `SIGNING_KEY_PASSWORD` or `SIGN_KEY_PASSWORD`: the key password. If you pressed Enter at the key password prompt, use the same value as the store password.

Optional:

- `SIGNING_KEY_ALIAS` or `SIGN_KEY_ALIAS`: defaults to `pexcraft_android_tv`.

The workflow validates the decoded keystore, alias, and key password before Gradle packages the APK. Telegram uploads all successful build artifacts, including this APK when the Android TV job succeeds.
