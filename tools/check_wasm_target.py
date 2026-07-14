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
    workflow = read(".github/workflows/build.yml")
    ui = read("src/ui/screen_state_input.c")
    assets = read("tools/prepare_wasm_assets.py")
    inventory = read("src/game/inventory.c")
    ingame = read("src/game/ingame_logic.c")
    java47 = read("multiplayer/java/protocol_47je/protocol_47je.c")
    gles2 = read("src/platform/lgwebos/lgwebos_gles2_renderer.c")
    title = read("src/ui/title_menus.c")
    textures = read("src/assets/textures.c")
    wasm_fs = read("src/platform/wasm/wasm_filesystem.c")
    gamepad = read("src/platform/gamepad.c")
    gui = read("src/ui/gui.c")
    xinput_embed = read("tools/embed_xinput_hud.py")
    xinput_rgba = read("src/assets/xinput_hud_rgba.inc")

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
    require(app, "pex_wasm_pointer_lock_lost", "wasm_app.c")
    require(app, "pex_wasm_open_github", "wasm_app.c")
    require(app, "frame_start_time - g_wasm_last_world_snapshot >= 3.0", "wasm_app.c")
    require(app, "save_world_state_sync();", "wasm_app.c")
    require(app, "Module.pexSyncPersistentStorage", "wasm_app.c")
    require(app, 'emscripten_get_element_css_size("#canvas"', "wasm_app.c")
    require(app, 'emscripten_set_canvas_element_size("#canvas"', "wasm_app.c")
    require(app, "emscripten_get_device_pixel_ratio", "wasm_app.c")
    require(app, "if (wasm_refresh_display_metrics())", "wasm_app.c")
    require(app, "pex_wasm_finish_skin_import", "wasm_app.c")
    require(app, "pex_wasm_finish_texture_pack_import", "wasm_app.c")
    require(app, "pxc_extract_zip_file(zip_path, dest", "wasm_app.c")
    require(app, "wasm_flatten_single_pack_directory", "wasm_app.c")
    require(app, "wasm_apply_first_run_graphics_defaults", "wasm_app.c")
    require(app, "g_opts.fancy_graphics = 0", "wasm_app.c")
    require(app, "g_opts.render_distance = 4", "wasm_app.c")
    require(app, "g_opts.sf_ao_level = 0.0f", "wasm_app.c")
    require(app, "g_opts.sf_gamma = 1.0f", "wasm_app.c")
    require(app, "int wasm_first_run = !file_exists(wasm_options_path)", "wasm_app.c")
    reject(app, r"if \(g_opts\.render_distance > 4\)", "wasm_app.c")
    reject(app, r"\bloggy_handle_event\s*\(", "wasm_app.c")
    reject(app, r"\bwhile\s*\(\s*g_running\s*\)", "wasm_app.c")

    require(ui, 'tr_key_default("menu.multiplayer", "Multiplayer")', "screen_state_input.c")
    require(ui, "multiplayer->enabled = 0", "screen_state_input.c")
    reject(ui, r"Multiplayer \(Unavailable\)", "screen_state_input.c")
    require(ui, "pex_wasm_choose_texture_pack_file();", "screen_state_input.c")
    require(ui, '"Github"', "screen_state_input.c")
    require(ui, "pex_wasm_open_github();", "screen_state_input.c")
    require(textures, "return pex_wasm_choose_skin_file();", "textures.c")
    require(textures, 'snprintf(out, cap, "/bundle/%s", CLASSIC_PACK_NAME)', "textures.c")
    require(textures, "static const unsigned char pex_xinput_hud_rgba[64 * 64 * 4]", "textures.c")
    require(textures, '#include "xinput_hud_rgba.inc"', "textures.c")
    require(textures, "memcpy(rgba, pex_xinput_hud_rgba, bytes);", "textures.c")
    require(textures, "Never depend on a loose PNG", "textures.c")
    reject(textures, r"xinput_hud_png\.inc|pex_xinput_hud_png", "textures.c")
    reject(textures, r"(?:try_release_texture|load_png_texture)\([^\n]*xinput", "textures.c")
    require(gamepad, "return pex_xinput_hud_active() ? 30 : 0;", "gamepad.c")
    # D-pad gameplay shortcuts are intentionally Xbox/XInput-only. Browser
    # remotes, keyboards, and generic navigation devices must not trigger the
    # legacy-console fly/chat/F5 mapping.
    require(gamepad, "if (p->a || (p->is_xbox && g_creative_flying && p->dpad_up))", "gamepad.c")
    require(gamepad, "if (g_gamepad_sneak_toggled || p->ls || (p->is_xbox && g_creative_flying && p->dpad_down))", "gamepad.c")
    reject(gamepad, r"p->l[xy][^\n]*\|\|\s*p->dpad_", "gamepad.c")
    require(gamepad, "SDL_GameControllerGetAttached", "gamepad.c")
    require(gamepad, "if (attached && total == g_sdl2_last_device_count) return;", "gamepad.c")
    require(gamepad, "PEX_TRIGGER_RELEASE_GRACE_FRAMES 3", "gamepad.c")
    require(gamepad, "pex_gamepad_update_trigger_buttons(&g_gamepads[i], &oldpads[i]);", "gamepad.c")
    require(gamepad, "if (p->rt_down) g_gamepad_vk_state[VK_LBUTTON] = 1;", "gamepad.c")
    require(gamepad, "if (p->lt_down) g_gamepad_vk_state[VK_RBUTTON] = 1;", "gamepad.c")
    reject(gamepad, r"p->is_xbox\s*&&\s*p->a\s*&&[^\n]*ingame_has_context_use_target", "gamepad.c")
    require(gui, "if (!pex_xinput_hud_active()) return;", "gui.c")
    require(gui, "int y = hotbar_y + 27;", "gui.c")
    require(gui, "int inventory_x = g_gui_w / 2 - 111;", "gui.c")
    require(gui, 'tr_key_default("key.inventory", "Inventory")', "gui.c")
    require(gui, 'tr_key_default("key.use", "Use Item")', "gui.c")
    require(gui, "int optional_x = inventory_x + 18 + text_width(inventory_label) + 16;", "gui.c")
    require(gui, "int show_optional = ingame_has_context_use_target();", "gui.c")
    require(gui, "draw_xinput_hud_tile(PEX_XINPUT_TILE_LT, optional_x, y);", "gui.c")
    require(gui, "if (show_optional)", "gui.c")
    require(gui, 'snprintf(field, sizeof(field), "%s%s", g_chat_input', "gui.c")
    reject(gui, r'snprintf\(field[^\n]*"> %s%s"', "gui.c")
    require(ui, "if (g_screen == SCREEN_INGAME && text[0] == '/')", "screen_state_input.c")
    require(ui, "if (g_screen == SCREEN_INGAME && ch == '/')", "screen_state_input.c")
    require(ui, "if (vk == '/')", "screen_state_input.c")
    require(app, "case SDLK_SLASH: case SDLK_KP_DIVIDE: return '/';", "wasm_app.c")
    common = read("src/common/common.h")
    options = read("src/settings/options.c")
    require(common, "VK_SHIFT,'Q','E','T','F'", "common.h")
    require(options, "case 18: return 'E';", "options.c")
    require(options, "case 'E': return 18;", "options.c")
    require(inventory, "dear_memories_destroy_if_burning", "inventory.c")
    require(inventory, "pex_achievement_on_dear_memories_destroyed();", "inventory.c")
    achievements = read("src/game/achievements.c")
    require(achievements, "#define PEX_ACHIEVEMENT_COUNT 28", "achievements.c")
    require(achievements, '{"cruelty", "Cruelty", "How could you?"', "achievements.c")
    require(wasm_fs, '"/persist/texturepacks"', "wasm_filesystem.c")
    require(inventory, "if (stream_generation_active()) process_stream_generation_queue();", "inventory.c")
    require(inventory, "stream async disabled on WASM", "inventory.c")
    require(ingame, "defined(PEX_PLATFORM_WASM)", "ingame_logic.c")
    require(java47, "defined(PEX_PLATFORM_WASM)", "protocol_47je.c")
    require(java47, "Browser builds are offline-only", "protocol_47je.c")
    require(title, "defined(PEX_PLATFORM_WASM)", "title_menus.c")
    require(title, "draw_release_panorama_wasm(partial)", "title_menus.c")
    require(title, "draw_release_panorama_cube_samples(partial, 1)", "title_menus.c")
    require(title, "pex_lgwebos_panorama_blur(2)", "title_menus.c")
    require(title, "float aspect = g_render_h > 0", "title_menus.c")
    require(title, "float half_v = 0.5f / aspect", "title_menus.c")
    require(title, "float half_u = 0.5f * aspect", "title_menus.c")
    require(gles2, "typedef struct LgWebOSPanoramaFx", "lgwebos_gles2_renderer.c")
    require(gles2, "static int pex_lgwebos_panorama_begin", "lgwebos_gles2_renderer.c")
    require(gles2, "static int pex_lgwebos_panorama_blur", "lgwebos_gles2_renderer.c")
    require(gles2, "static int pex_lgwebos_panorama_present", "lgwebos_gles2_renderer.c")
    require(gles2, "-1.0f, -1.0f, u0, v0", "lgwebos_gles2_renderer.c")
    require(gles2, "1.0f, -1.0f, u1, v0", "lgwebos_gles2_renderer.c")
    require(gles2, "-1.0f,  1.0f, u0, v1", "lgwebos_gles2_renderer.c")
    require(gles2, "u_step * 3.2307692308", "lgwebos_gles2_renderer.c")
    require(gles2, "glFramebufferTexture2D", "lgwebos_gles2_renderer.c")
    reject(title, r"WebAssembly uses all 8x8 samples", "title_menus.c")

    require(build, "-std=gnu99", "build_wasm.sh")
    require(build, "python3 tools/embed_xinput_hud.py", "build_wasm.sh")
    reject(build, r"-std=c(?:89|90|99|11|17|23)\b", "build_wasm.sh")
    require(build, 'EMSDK_VERSION="${EMSDK_VERSION:-6.0.2}"', "build_wasm.sh")
    require(build, "-sGL_ENABLE_GET_PROC_ADDRESS=1", "build_wasm.sh")
    reject(build, r"-sGL_ENABLE_GET_PROC_ADDRESS=0\b", "build_wasm.sh")
    for flag in ("-sWASM=1", "-sSINGLE_FILE=1", "-sSINGLE_FILE_BINARY_ENCODE=0", "--embed-file", "-sUSE_SDL=2", "-sUSE_SDL_IMAGE=2", "-lidbfs.js"):
        require(build, flag, "build_wasm.sh")
    reject(build, r"-sSINGLE_FILE_BINARY_ENCODE=(?:1|true)\b", "build_wasm.sh")
    require(build, "PexCraft-WASM.html", "build_wasm.sh")
    require(build, "_pex_wasm_finish_skin_import", "build_wasm.sh")
    require(build, "_pex_wasm_pointer_lock_lost", "build_wasm.sh")
    require(build, "_pex_wasm_finish_texture_pack_import", "build_wasm.sh")
    require(build, 'if "AGFzbQE" not in text:', "build_wasm.sh")
    require(build, "Embedded WASM encoding: base64", "build_wasm.sh")
    require(shell, "connect-src 'none'", "wasm_shell.html")
    require(shell, "FS.mount(IDBFS", "wasm_shell.html")
    require(shell, "autoPersist: true", "wasm_shell.html")
    require(shell, "pexSyncPersistentStorage", "wasm_shell.html")
    require(shell, "pointerlockchange", "wasm_shell.html")
    require(shell, 'draggable="false"', "wasm_shell.html")
    require(shell, "touch-action:none", "wasm_shell.html")
    require(shell, "user-select:none", "wasm_shell.html")
    require(shell, "pexPickSkin()", "wasm_shell.html")
    require(shell, "pexPickTexturePack()", "wasm_shell.html")
    require(shell, "_pex_wasm_finish_skin_import", "wasm_shell.html")
    require(shell, "_pex_wasm_finish_texture_pack_import", "wasm_shell.html")
    require(shell, "height:100dvh", "wasm_shell.html")
    require(shell, "image-rendering:auto", "wasm_shell.html")
    reject(shell, r"image-rendering\s*:\s*pixelated", "wasm_shell.html")
    reject(shell, r"https?://|wss?://", "wasm_shell.html")
    reject(shell, r'id=["\']status["\']|<progress|PexCraft stopped|PexCraft WebAssembly', "wasm_shell.html")

    require(assets, "4a2fac7504182a97dcbcd7560c6392d7c8139928", "prepare_wasm_assets.py")
    require(assets, "sha1_file", "prepare_wasm_assets.py")
    require(assets, "extract_resources", "prepare_wasm_assets.py")
    reject(assets, r"XINPUT\.png|gui/xinput\.png", "prepare_wasm_assets.py")
    require(xinput_embed, "decode_rgba_png", "embed_xinput_hud.py")
    require(xinput_embed, "xinput_hud_rgba.inc", "embed_xinput_hud.py")
    require(xinput_rgba, "dimensions: 64x64 RGBA8; bytes: 16384", "xinput_hud_rgba.inc")
    if not (ROOT / "src/assets/XINPUT.png").is_file():
        raise AssertionError("missing source controller sprite: src/assets/XINPUT.png")
    if (ROOT / "src/assets/xinput_hud_png.inc").exists():
        raise AssertionError("obsolete compressed PNG include must not exist")

    require(workflow, "  wasm:", "build.yml")
    require(workflow, "./build_wasm.sh", "build.yml")
    require(workflow, "dist/PexCraft-WASM.html", "build.yml")
    require(workflow, "telegram_upload_file.py", "build.yml")
    if (ROOT / ".github/workflows/build-wasm.yml").exists():
        raise AssertionError("separate .github/workflows/build-wasm.yml must not exist")
    ignored_generated_roots = {".git", ".cache", "build", "dist"}
    extra_md = []
    for path in ROOT.rglob("*.md"):
        rel = path.relative_to(ROOT)
        if rel.as_posix() == "README.md":
            continue
        if rel.parts and rel.parts[0] in ignored_generated_roots:
            continue
        extra_md.append(path)
    if extra_md:
        raise AssertionError(f"unexpected extra markdown files: {extra_md}")

    subprocess.run(["bash", "-n", str(ROOT / "build_wasm.sh")], check=True)
    subprocess.run([sys.executable, str(ROOT / "tools/embed_xinput_hud.py"), "--check"], check=True)
    py_compile.compile(str(ROOT / "tools/prepare_wasm_assets.py"), doraise=True)
    py_compile.compile(str(ROOT / "tools/embed_xinput_hud.py"), doraise=True)
    py_compile.compile(str(ROOT / ".github/scripts/telegram_upload_file.py"), doraise=True)

    print("WASM target structural checks passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, OSError, subprocess.CalledProcessError, py_compile.PyCompileError) as exc:
        print(f"check_wasm_target.py: {exc}", file=sys.stderr)
        raise SystemExit(1)
