# PexCraft Android port

This is the regular Android phone/tablet APK target. It is separate from `android_tv/` and builds only `arm64-v8a` for now.

## Controls

- Lower-left PE-style D-pad: move; center D-pad button is Sneak.
- Swipe/drag anywhere in the world view: look around.
- Tap a block/item in the world view: place/use/open against the block under that finger.
- Hold a block in the world view: mine/break the block under that finger.
- Jump, Inv, Pause: PE-style on-screen game buttons.
- Hotbar: tap a slot to select it.
- Inventory/chest/furnace/crafting: touch slots directly; tap to pick/place and drag a carried stack from one slot to another.

## Classic resource pack download

The Android build uses Android `HttpURLConnection` to download Mojang `client.jar` at runtime, then the native ZIP extractor installs the Classic pack into the writable texturepacks folder. This is not a stub.

## Build output

GitHub Actions copies the APK as:

```text
pexcraft-android-arm64.apk
```

## Local build

```bash
cd android
./prepare_sdl2.sh
gradle assembleRelease
```

The release signing secrets are the same as the Android TV job: `sign`, `SIGNING_STORE_PASSWORD`, `SIGNING_KEY_PASSWORD`, and optional `SIGNING_KEY_ALIAS`.
