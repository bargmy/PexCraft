#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

python3 tools/generate_build_info.py
python3 tools/embed_xinput_hud.py
python3 tools/check_wasm_target.py

EMSDK_VERSION="${EMSDK_VERSION:-6.0.2}"
EMSDK_DIR="${EMSDK_DIR:-$ROOT/.cache/emsdk}"
OUTPUT="$ROOT/dist/PexCraft-WASM.html"

find_emcc() {
  if command -v emcc >/dev/null 2>&1; then
    command -v emcc
    return 0
  fi
  if [[ -x "$EMSDK_DIR/upstream/emscripten/emcc" ]]; then
    printf '%s\n' "$EMSDK_DIR/upstream/emscripten/emcc"
    return 0
  fi
  return 1
}

if ! EMCC="$(find_emcc)"; then
  if [[ ! -d "$EMSDK_DIR/.git" ]]; then
    mkdir -p "$(dirname "$EMSDK_DIR")"
    git clone --depth 1 https://github.com/emscripten-core/emsdk.git "$EMSDK_DIR"
  fi
  "$EMSDK_DIR/emsdk" install "$EMSDK_VERSION"
  "$EMSDK_DIR/emsdk" activate "$EMSDK_VERSION"
  # shellcheck disable=SC1091
  source "$EMSDK_DIR/emsdk_env.sh" >/dev/null
  EMCC="$(find_emcc)"
fi

python3 tools/prepare_wasm_assets.py --output build/wasm_assets
rm -rf dist
mkdir -p dist

"$EMCC" -include build/generated/pex_build_info.h -std=gnu99 -O3 -DNDEBUG main_wasm.c -o "$OUTPUT" \
  -sUSE_SDL=2 \
  -sUSE_SDL_IMAGE=2 \
  -sSDL2_IMAGE_FORMATS='["png"]' \
  -sUSE_ZLIB=1 \
  -sWASM=1 \
  -sSINGLE_FILE=1 \
  -sSINGLE_FILE_BINARY_ENCODE=0 \
  -sENVIRONMENT=web \
  -sEXIT_RUNTIME=0 \
  -sALLOW_MEMORY_GROWTH=1 \
  -sINITIAL_MEMORY=536870912 \
  -sMAXIMUM_MEMORY=2147483648 \
  -sSTACK_SIZE=8388608 \
  -sMIN_WEBGL_VERSION=1 \
  -sMAX_WEBGL_VERSION=2 \
  -sGL_ENABLE_GET_PROC_ADDRESS=1 \
  -sFORCE_FILESYSTEM=1 \
  -sASSERTIONS=0 \
  -sEXPORTED_FUNCTIONS='["_main","_pex_wasm_visibility_flush","_pex_wasm_pointer_lock_lost","_pex_wasm_finish_skin_import","_pex_wasm_finish_texture_pack_import"]' \
  -sEXPORTED_RUNTIME_METHODS='["FS"]' \
  -lidbfs.js \
  --embed-file build/wasm_assets/Release@/bundle/Release \
  --shell-file web/wasm_shell.html \
  -lm

[[ -s "$OUTPUT" ]] || { echo "Build did not produce $OUTPUT" >&2; exit 1; }
[[ "$(find dist -maxdepth 1 -type f | wc -l | tr -d ' ')" == "1" ]] || {
  echo "WASM build must produce exactly one file in dist" >&2
  find dist -maxdepth 1 -type f -print >&2
  exit 1
}
grep -q "connect-src 'none'" "$OUTPUT" || { echo "Missing offline CSP" >&2; exit 1; }
if grep -Eqi 'piston-data\.mojang\.com|launcher\.mojang\.com|launchermeta\.mojang\.com|resources\.download\.minecraft\.net|api\.telegram\.org' "$OUTPUT"; then
  echo "Standalone HTML unexpectedly contains a runtime network endpoint" >&2
  exit 1
fi

python3 - "$OUTPUT" <<'PY'
import hashlib, pathlib, sys
path = pathlib.Path(sys.argv[1])
data = path.read_bytes()
try:
    text = data.decode("utf-8")
except UnicodeDecodeError as exc:
    raise SystemExit(f"Standalone HTML is not valid UTF-8: {exc}")
if "AGFzbQE" not in text:
    raise SystemExit("Embedded WASM is not using the required base64 encoding")
print(f"Built {path} ({len(data)} bytes)")
print("Embedded WASM encoding: base64")
print("SHA-256:", hashlib.sha256(data).hexdigest())
PY
