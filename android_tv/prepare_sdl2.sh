#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CACHE="$ROOT/third_party/cache"
SDL_DST="$ROOT/third_party/SDL2"
JNI_DST="$ROOT/app/src/main/jniLibs"
AAR_DST="$ROOT/app/libs/SDL2.aar"
mkdir -p "$CACHE" "$SDL_DST" "$JNI_DST" "$ROOT/app/libs"

find_latest_sdl2_android_url() {
  python3 - <<'PY'
import json, sys, urllib.request
url = "https://api.github.com/repos/libsdl-org/SDL/releases?per_page=100"
with urllib.request.urlopen(url, timeout=60) as r:
    releases = json.load(r)
for rel in releases:
    for asset in rel.get("assets", []):
        name = asset.get("name", "")
        if name.startswith("SDL2-devel-") and name.endswith("-android.zip"):
            print(asset["browser_download_url"])
            raise SystemExit(0)
raise SystemExit(1)
PY
}

if [ -z "${SDL2_ANDROID_ZIP_URL:-}" ]; then
  SDL2_ANDROID_ZIP_URL="$(find_latest_sdl2_android_url || true)"
fi

if [ -z "${SDL2_ANDROID_ZIP_URL:-}" ]; then
  echo "Could not discover an SDL2 Android development zip. Set SDL2_ANDROID_ZIP_URL manually." >&2
  exit 1
fi

ZIP="$CACHE/$(basename "$SDL2_ANDROID_ZIP_URL")"
if [ ! -s "$ZIP" ]; then
  echo "Downloading $SDL2_ANDROID_ZIP_URL"
  curl -L --retry 3 --fail "$SDL2_ANDROID_ZIP_URL" -o "$ZIP"
fi

rm -rf "$SDL_DST" "$CACHE/unpack"
mkdir -p "$SDL_DST" "$CACHE/unpack"
unzip -q "$ZIP" -d "$CACHE/unpack"

AAR="$(find "$CACHE/unpack" -type f -name '*SDL2*.aar' -print -quit)"
if [ -z "$AAR" ]; then
  echo "No SDL2 AAR found in $ZIP" >&2
  find "$CACHE/unpack" -maxdepth 4 -type f | sort >&2
  exit 1
fi
cp -f "$AAR" "$AAR_DST"

INCLUDE_DIR="$(find "$CACHE/unpack" -type f -name SDL.h -printf '%h\n' | head -n 1)"
if [ -z "$INCLUDE_DIR" ]; then
  echo "No SDL.h found in $ZIP" >&2
  exit 1
fi
mkdir -p "$SDL_DST/include/SDL2"
if [ "$(basename "$INCLUDE_DIR")" = "SDL2" ]; then
  cp -R "$INCLUDE_DIR"/* "$SDL_DST/include/SDL2/"
else
  cp -R "$INCLUDE_DIR"/* "$SDL_DST/include/SDL2/"
fi

rm -rf "$JNI_DST"
mkdir -p "$JNI_DST"
AAR_UNPACK="$CACHE/aar"
rm -rf "$AAR_UNPACK" && mkdir -p "$AAR_UNPACK"
unzip -q "$AAR_DST" -d "$AAR_UNPACK"
for so in $(find "$AAR_UNPACK" "$CACHE/unpack" -type f -name libSDL2.so | sort -u); do
  abi="$(basename "$(dirname "$so")")"
  case "$abi" in
    arm64-v8a|armeabi-v7a|x86|x86_64)
      mkdir -p "$JNI_DST/$abi"
      cp -f "$so" "$JNI_DST/$abi/libSDL2.so"
      ;;
  esac
done

if [ -z "$(find "$JNI_DST" -type f -name libSDL2.so -print -quit)" ]; then
  echo "No libSDL2.so files found in SDL2 AAR/devel archive." >&2
  exit 1
fi

echo "Prepared SDL2 Android dependency:"
echo "  AAR:     $AAR_DST"
echo "  Headers: $SDL_DST/include/SDL2"
find "$JNI_DST" -type f -name libSDL2.so | sort | sed 's#^#  #'
