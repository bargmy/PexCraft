#!/usr/bin/env python3
"""Download and verify Minecraft 1.2.5 client.jar, then stage browser assets.

The browser never performs this download. The staged Release directory is
embedded by Emscripten into the single output HTML.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import sys
import tempfile
import urllib.request
import zipfile
from pathlib import Path, PurePosixPath

PROJECT_ROOT = Path(__file__).resolve().parents[1]

JAR_SHA1 = "4a2fac7504182a97dcbcd7560c6392d7c8139928"
JAR_URL = f"https://piston-data.mojang.com/v1/objects/{JAR_SHA1}/client.jar"
JAR_NAME = "minecraft-1.2.5-client.jar"

REQUIRED = (
    "terrain.png",
    "gui/gui.png",
    "gui/items.png",
    "gui/icons.png",
    "achievement/bg.png",
    "gui/slot.png",
    "font/default.png",
    "font.txt",
    "font/glyph_sizes.bin",
    "font/glyph_04.png",
    "font/glyph_06.png",
    "title/mclogo.png",
    "title/mojang.png",
    "title/bg/panorama0.png",
    "title/bg/panorama1.png",
    "title/bg/panorama2.png",
    "title/bg/panorama3.png",
    "title/bg/panorama4.png",
    "title/bg/panorama5.png",
    "mob/pig.png",
    "mob/sheep.png",
    "mob/sheep_fur.png",
    "mob/cow.png",
    "mob/chicken.png",
    "mob/saddle.png",
    "mob/creeper.png",
    "mob/skeleton.png",
    "mob/spider.png",
    "mob/zombie.png",
    "mob/slime.png",
    "mob/ghast.png",
    "mob/ghast_fire.png",
    "mob/pigzombie.png",
    "mob/enderman.png",
    "mob/cavespider.png",
    "mob/silverfish.png",
    "mob/fire.png",
    "mob/lava.png",
    "mob/enderdragon/ender.png",
    "mob/squid.png",
    "mob/wolf.png",
    "mob/wolf_tame.png",
    "mob/wolf_angry.png",
    "mob/redcow.png",
    "mob/snowman.png",
    "mob/ozelot.png",
    "mob/cat_red.png",
    "mob/villager_golem.png",
    "mob/villager/villager.png",
)


def sha1_file(path: Path) -> str:
    digest = hashlib.sha1()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def download_verified_jar(cache_path: Path, source_jar: Path | None) -> Path:
    cache_path.parent.mkdir(parents=True, exist_ok=True)
    if source_jar is not None:
        source_jar = source_jar.resolve()
        if not source_jar.is_file():
            raise FileNotFoundError(f"JAR does not exist: {source_jar}")
        if source_jar != cache_path.resolve():
            shutil.copy2(source_jar, cache_path)
    elif not cache_path.exists() or sha1_file(cache_path) != JAR_SHA1:
        request = urllib.request.Request(
            JAR_URL,
            headers={"User-Agent": "PexCraft-WASM-builder/1.0"},
        )
        fd, temp_name = tempfile.mkstemp(prefix="pexcraft-jar-", suffix=".tmp", dir=cache_path.parent)
        os.close(fd)
        temp_path = Path(temp_name)
        try:
            print(f"Downloading verified Minecraft 1.2.5 client.jar from {JAR_URL}")
            with urllib.request.urlopen(request, timeout=120) as response, temp_path.open("wb") as output:
                shutil.copyfileobj(response, output, 1024 * 1024)
            temp_path.replace(cache_path)
        finally:
            temp_path.unlink(missing_ok=True)

    actual = sha1_file(cache_path)
    if actual != JAR_SHA1:
        cache_path.unlink(missing_ok=True)
        raise RuntimeError(f"client.jar SHA-1 mismatch: expected {JAR_SHA1}, got {actual}")
    return cache_path


def safe_member(name: str) -> PurePosixPath | None:
    normalized = name.replace("\\", "/")
    path = PurePosixPath(normalized)
    if not normalized or normalized.endswith("/"):
        return None
    if path.is_absolute() or ".." in path.parts:
        raise RuntimeError(f"Unsafe path in client.jar: {name!r}")
    if path.parts and path.parts[0].upper() == "META-INF":
        return None
    if path.suffix.lower() in {".class", ".dll", ".so", ".dylib", ".exe"}:
        return None
    return path


def extract_resources(jar_path: Path, output_root: Path) -> list[dict[str, object]]:
    release = output_root / "Release"
    if output_root.exists():
        shutil.rmtree(output_root)
    release.mkdir(parents=True, exist_ok=True)
    manifest: list[dict[str, object]] = []

    with zipfile.ZipFile(jar_path) as archive:
        for info in archive.infolist():
            rel = safe_member(info.filename)
            if rel is None:
                continue
            target = release.joinpath(*rel.parts)
            target.parent.mkdir(parents=True, exist_ok=True)
            with archive.open(info, "r") as source, target.open("wb") as destination:
                shutil.copyfileobj(source, destination, 1024 * 1024)
            manifest.append({"path": rel.as_posix(), "bytes": target.stat().st_size})

    pack_text = release / "pack.txt"
    if not pack_text.exists():
        pack_text.write_text("Minecraft 1.2.5 Release\nEmbedded for PexCraft WASM\n", encoding="utf-8")

    controller_source = PROJECT_ROOT / "src" / "assets" / "XINPUT.png"
    controller_target = release / "gui" / "xinput.png"
    if not controller_source.is_file():
        raise RuntimeError(f"Missing controller HUD sprite sheet: {controller_source}")
    controller_target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(controller_source, controller_target)
    manifest.append({"path": "gui/xinput.png", "bytes": controller_target.stat().st_size})

    missing = [name for name in REQUIRED if not (release / name).is_file()]
    if missing:
        raise RuntimeError("Verified client.jar is missing required assets:\n  " + "\n  ".join(missing))

    manifest.sort(key=lambda entry: str(entry["path"]))
    metadata = {
        "source": "Minecraft 1.2.5 client.jar",
        "url": JAR_URL,
        "sha1": JAR_SHA1,
        "files": manifest,
    }
    (output_root / "wasm-assets-manifest.json").write_text(
        json.dumps(metadata, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    return manifest


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=Path("build/wasm_assets"))
    parser.add_argument("--cache", type=Path, default=Path(".cache/pexcraft") / JAR_NAME)
    parser.add_argument("--jar", type=Path, help="Use a local client.jar instead of downloading")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        jar = download_verified_jar(args.cache, args.jar)
        files = extract_resources(jar, args.output)
    except (OSError, RuntimeError, zipfile.BadZipFile) as exc:
        print(f"prepare_wasm_assets.py: {exc}", file=sys.stderr)
        return 1
    total = sum(int(entry["bytes"]) for entry in files)
    print(f"Staged {len(files)} resource files ({total} bytes) in {args.output / 'Release'}")
    print(f"Verified client.jar SHA-1: {JAR_SHA1}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
