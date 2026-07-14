#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CACHE="$ROOT/third_party/cache"
SDL_SRC_DST="$ROOT/third_party/SDL2-src"
SDL_INCLUDE_DST="$ROOT/third_party/SDL2/include/SDL2"
APP_JAVA_DST="$ROOT/app/src/main/java"
mkdir -p "$CACHE" "$ROOT/third_party" "$SDL_INCLUDE_DST" "$APP_JAVA_DST"

find_latest_sdl2_source_url() {
  python3 - <<'PY'
import html.parser, re, sys, urllib.request
url = "https://www.libsdl.org/release/?C=M;O=D"
try:
    with urllib.request.urlopen(url, timeout=60) as r:
        html = r.read().decode("utf-8", "replace")
except Exception as exc:
    raise SystemExit(1)
versions = []
for name in re.findall(r'SDL2-(\d+\.\d+\.\d+)\.zip', html):
    try:
        parts = tuple(int(p) for p in name.split('.'))
    except ValueError:
        continue
    versions.append((parts, name))
if not versions:
    raise SystemExit(1)
versions.sort(reverse=True)
print("https://www.libsdl.org/release/SDL2-%s.zip" % versions[0][1])
PY
}

if [ -z "${SDL2_SOURCE_ZIP_URL:-}" ]; then
  SDL2_SOURCE_ZIP_URL="$(find_latest_sdl2_source_url || true)"
fi

# SDL2 no longer ships the Android AAR/devel package shape that SDL3 does.
# Keep a known-good SDL2 source release fallback so GitHub Actions reaches the
# native compiler instead of failing while searching for a prebuilt Android zip.
if [ -z "${SDL2_SOURCE_ZIP_URL:-}" ]; then
  SDL2_SOURCE_ZIP_URL="https://www.libsdl.org/release/SDL2-2.32.10.zip"
fi

ZIP="$CACHE/$(basename "$SDL2_SOURCE_ZIP_URL")"
if [ ! -s "$ZIP" ]; then
  echo "Downloading $SDL2_SOURCE_ZIP_URL"
  curl -L --retry 3 --fail "$SDL2_SOURCE_ZIP_URL" -o "$ZIP"
fi

rm -rf "$SDL_SRC_DST" "$CACHE/unpack"
mkdir -p "$CACHE/unpack"
unzip -q "$ZIP" -d "$CACHE/unpack"

SRC_ROOT="$(find "$CACHE/unpack" -maxdepth 4 -type f -name SDL.h -printf '%h\n' | sed 's#/include$##' | head -n 1)"
if [ -z "$SRC_ROOT" ] || [ ! -f "$SRC_ROOT/CMakeLists.txt" ]; then
  echo "Could not find SDL2 source root in $ZIP" >&2
  find "$CACHE/unpack" -maxdepth 3 -type f | sort >&2
  exit 1
fi

mv "$SRC_ROOT" "$SDL_SRC_DST"

rm -rf "$SDL_INCLUDE_DST"
mkdir -p "$SDL_INCLUDE_DST"
cp -R "$SDL_SRC_DST/include/"* "$SDL_INCLUDE_DST/"

SDL_JAVA_SRC="$SDL_SRC_DST/android-project/app/src/main/java/org"
if [ ! -d "$SDL_JAVA_SRC" ]; then
  echo "Could not find SDL2 Android Java sources under $SDL_SRC_DST" >&2
  exit 1
fi
rm -rf "$APP_JAVA_DST/org/libsdl"
mkdir -p "$APP_JAVA_DST/org"
cp -R "$SDL_JAVA_SRC/libsdl" "$APP_JAVA_DST/org/"

# Remove the older prebuilt-AAR/JNI layout if it exists from a previous run.
rm -rf "$ROOT/app/libs/SDL2.aar" "$ROOT/app/src/main/jniLibs"

cat > "$ROOT/third_party/SDL2-source.properties" <<EOF2
SDL2_SOURCE_ZIP_URL=$SDL2_SOURCE_ZIP_URL
SDL2_SOURCE_ROOT=$SDL_SRC_DST
EOF2

echo "Prepared SDL2 Android source dependency:"
echo "  Source:  $SDL_SRC_DST"
echo "  Headers: $SDL_INCLUDE_DST"
echo "  Java:    $APP_JAVA_DST/org/libsdl/app/SDLActivity.java"
