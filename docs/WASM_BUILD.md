# Offline WebAssembly build

The WASM target builds PexCraft as one self-contained `dist/PexCraft-WASM.html` file. The HTML contains the JavaScript loader, WebAssembly module, and the verified Minecraft Java Edition 1.2.5 texture/font resources. It does not fetch assets when opened.

## Target behavior

- WebAssembly through Emscripten, SDL2, SDL_image, and WebGL/OpenGL ES 2.
- Browser-compatible frame callback instead of a blocking native loop.
- Responsive canvas sizing: CSS viewport size, WebGL drawing-buffer size, DPI, and GUI projection stay synchronized on resize/fullscreen.
- WebGL-safe full-screen panorama path with direct 8x8 accumulation blur; it does not use the flickering 256x256 default-framebuffer copy path.
- Single-player only. The Multiplayer button is visible but disabled.
- Single-threaded cooperative game, world-generation, meshing, and save fallbacks.
- Saves/options under `/persist`, backed by IndexedDB when the browser permits it.
- No runtime HTTP, WebSocket, or UDP/TCP connection path; the page CSP uses `connect-src 'none'`.
- Audio is disabled in this first browser target because the native backend dynamically loads desktop libraries.

## Build

Run:

```sh
./build_wasm.sh
```

or:

```sh
make -f Makefile.wasm
```

The script uses an installed `emcc` when available. Otherwise it installs Emscripten SDK 6.0.2 into `.cache/emsdk` by default. Set `EMSDK_VERSION` to override the pinned version. The build explicitly enables `GL_ENABLE_GET_PROC_ADDRESS` because Emscripten's SDL2 port requires `eglGetProcAddress` for its browser OpenGL ES context backend. The build also forces `SINGLE_FILE_BINARY_ENCODE=0`, so the embedded WASM uses base64 instead of the newer custom UTF-8 encoding. This avoids corrupted WASM bytes when the standalone HTML is opened through `file://` or transferred by file-sharing applications.

At build time, `tools/prepare_wasm_assets.py` downloads the official Minecraft 1.2.5 `client.jar`, verifies SHA-1 `4a2fac7504182a97dcbcd7560c6392d7c8139928`, safely extracts its non-code resources, validates the textures and fonts required by PexCraft, and stages them under `build/wasm_assets/Release`. Emscripten embeds that directory at `/bundle/Release` in the output HTML.

For an already-downloaded JAR, use:

```sh
python3 tools/prepare_wasm_assets.py --jar /path/to/client.jar --output build/wasm_assets
```

The same hash verification is still mandatory. Then run `./build_wasm.sh`; the cached verified JAR is reused.

## Output and network policy

The build directory must contain exactly one output:

```text
dist/PexCraft-WASM.html
```

The generated page has no external script, stylesheet, image, asset, telemetry, multiplayer, or update endpoint. The JAR download occurs only on the build machine. The GitHub Actions workflow does not publish an Actions artifact or GitHub release; after a successful push/manual build it sends only the standalone HTML to the configured Telegram bot/chat.

The Telegram workflow requires repository secret `TOKEN`. `CHAT_ID` is configured in `.github/workflows/build-wasm.yml`.

## Validation

Run the fast repository checks:

```sh
python3 tools/check_wasm_target.py
```

A release-grade validation still requires an Emscripten build and browser smoke test. Recommended checks are Chrome/Chromium and Firefox on desktop: open the title screen, resize the browser repeatedly, enter and leave fullscreen, confirm the panorama always fills the canvas without corner flicker and remains blurred, confirm Multiplayer is disabled, create/load/save a world, reload the page, and confirm no network requests appear in developer tools while the client runs.

## Notes

Opening the single file through `file://` is supported by the single-file packaging approach. The WASM payload is deliberately base64-encoded inside the HTML to avoid character-encoding corruption on local files. Some browsers still restrict IndexedDB on local-file origins; in that case the game runs but saves may be temporary. Serving the same one-file HTML from a static HTTPS origin gives persistent storage the most reliable origin.

Only distribute the bundled Minecraft resources where you have the rights and permission to do so.
