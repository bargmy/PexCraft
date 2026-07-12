#!/usr/bin/env python3
"""Fast structural checks for the offline single-file WebAssembly target."""
from __future__ import annotations

import pathlib
import py_compile
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        raise AssertionError(f"missing required file: {rel}")
    return path.read_text(encoding="utf-8")


def require(text: str, needle: str, where: str) -> None:
    if needle not in text:
        raise AssertionError(f"{where}: missing {needle!r}")


def reject(text: str, pattern: str, where: str) -> None:
    if re.search(pattern, text, re.IGNORECASE | re.MULTILINE):
        raise AssertionError(f"{where}: forbidden pattern {pattern!r}")


def main() -> int:
    root_entry = read("main_wasm.c")
    unity = read("src/main_wasm.c")
    app = read("src/platform/wasm/wasm_app.c")
    shell = read("web/wasm_shell.html")
    build = read("build_wasm.sh")
    workflow = read(".github/workflows/build-wasm.yml")
    ui = read("src/ui/screen_state_input.c")
    assets = read("tools/prepare_wasm_assets.py")
    inventory = read("src/game/inventory.c")
    ingame = read("src/game/ingame_logic.c")
    java47 = read("multiplayer/java/protocol_47je/protocol_47je.c")
    gles2 = read("src/platform/lgwebos/lgwebos_gles2_renderer.c")
    title = read("src/ui/title_menus.c")

    require(root_entry, '#include "src/main_wasm.c"', "main_wasm.c")
    for define in ("PEX_PLATFORM_WASM", "PEX_WASM_NO_MULTIPLAYER", "PEX_SINGLE_THREADED"):
        require(unity, define, "src/main_wasm.c")
    require(unity, 'platform/lgwebos/lgwebos_gles2_renderer.c', "src/main_wasm.c")
    require(unity, 'platform/wasm/wasm_app.c', "src/main_wasm.c")
    require(unity, "#define glColor4ub pex_lgwebos_glColor4ub", "src/main_wasm.c")
    require(unity, "#define glDeleteLists pex_lgwebos_glDeleteLists", "src/main_wasm.c")
    require(gles2, "static void pex_lgwebos_glColor4ub", "lgwebos_gles2_renderer.c")
    require(gles2, "static void pex_lgwebos_glDeleteLists", "lgwebos_gles2_renderer.c")

    require(app, "emscripten_set_main_loop", "wasm_app.c")
    require(app, "pex_wasm_visibility_flush", "wasm_app.c")
    require(app, 'emscripten_get_element_css_size("#canvas"', "wasm_app.c")
    require(app, 'emscripten_set_canvas_element_size("#canvas"', "wasm_app.c")
    require(app, "emscripten_get_device_pixel_ratio", "wasm_app.c")
    require(app, "if (wasm_refresh_display_metrics())", "wasm_app.c")
    reject(app, r"\bloggy_handle_event\s*\(", "wasm_app.c")
    reject(app, r"\bwhile\s*\(\s*g_running\s*\)", "wasm_app.c")

    require(ui, '"Multiplayer (Unavailable)"', "screen_state_input.c")
    require(ui, "multiplayer->enabled = 0", "screen_state_input.c")
    require(inventory, "if (stream_generation_active()) process_stream_generation_queue();", "inventory.c")
    require(inventory, "stream async disabled on WASM", "inventory.c")
    require(ingame, "defined(PEX_PLATFORM_WASM)", "ingame_logic.c")
    require(java47, "defined(PEX_PLATFORM_WASM)", "protocol_47je.c")
    require(java47, "Browser builds are offline-only", "protocol_47je.c")
    require(title, "defined(PEX_PLATFORM_WASM)", "title_menus.c")
    require(title, "draw_release_panorama_wasm(partial)", "title_menus.c")
    require(title, "draw_release_panorama_cube_samples(partial, 1)", "title_menus.c")
    require(title, "pex_lgwebos_panorama_blur(2)", "title_menus.c")
    require(gles2, "typedef struct LgWebOSPanoramaFx", "lgwebos_gles2_renderer.c")
    require(gles2, "static int pex_lgwebos_panorama_begin", "lgwebos_gles2_renderer.c")
    require(gles2, "static int pex_lgwebos_panorama_blur", "lgwebos_gles2_renderer.c")
    require(gles2, "static int pex_lgwebos_panorama_present", "lgwebos_gles2_renderer.c")
    require(gles2, "u_step * 3.2307692308", "lgwebos_gles2_renderer.c")
    require(gles2, "glFramebufferTexture2D", "lgwebos_gles2_renderer.c")
    reject(title, r"WebAssembly uses all 8x8 samples", "title_menus.c")

    require(build, "-std=gnu99", "build_wasm.sh")
    reject(build, r"-std=c(?:89|90|99|11|17|23)\b", "build_wasm.sh")
    require(build, 'EMSDK_VERSION="${EMSDK_VERSION:-6.0.2}"', "build_wasm.sh")
    require(build, "-sGL_ENABLE_GET_PROC_ADDRESS=1", "build_wasm.sh")
    reject(build, r"-sGL_ENABLE_GET_PROC_ADDRESS=0\b", "build_wasm.sh")
    for flag in ("-sWASM=1", "-sSINGLE_FILE=1", "-sSINGLE_FILE_BINARY_ENCODE=0", "--embed-file", "-sUSE_SDL=2", "-sUSE_SDL_IMAGE=2", "-lidbfs.js"):
        require(build, flag, "build_wasm.sh")
    reject(build, r"-sSINGLE_FILE_BINARY_ENCODE=(?:1|true)\b", "build_wasm.sh")
    require(build, "PexCraft-WASM.html", "build_wasm.sh")
    require(build, 'if "AGFzbQE" not in text:', "build_wasm.sh")
    require(build, "Embedded WASM encoding: base64", "build_wasm.sh")
    require(shell, "connect-src 'none'", "wasm_shell.html")
    require(shell, "FS.mount(IDBFS", "wasm_shell.html")
    require(shell, "height:100dvh", "wasm_shell.html")
    require(shell, "image-rendering:auto", "wasm_shell.html")
    reject(shell, r"image-rendering\s*:\s*pixelated", "wasm_shell.html")
    reject(shell, r"https?://|wss?://", "wasm_shell.html")

    require(assets, "4a2fac7504182a97dcbcd7560c6392d7c8139928", "prepare_wasm_assets.py")
    require(assets, "sha1_file", "prepare_wasm_assets.py")
    require(assets, "extract_resources", "prepare_wasm_assets.py")

    require(workflow, "telegram_upload_file.py", "build-wasm.yml")
    reject(workflow, r"actions/upload-artifact|softprops/action-gh-release|gh\s+release", "build-wasm.yml")

    subprocess.run(["bash", "-n", str(ROOT / "build_wasm.sh")], check=True)
    py_compile.compile(str(ROOT / "tools/prepare_wasm_assets.py"), doraise=True)
    py_compile.compile(str(ROOT / ".github/scripts/telegram_upload_file.py"), doraise=True)

    print("WASM target structural checks passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, OSError, subprocess.CalledProcessError, py_compile.PyCompileError) as exc:
        print(f"check_wasm_target.py: {exc}", file=sys.stderr)
        raise SystemExit(1)
