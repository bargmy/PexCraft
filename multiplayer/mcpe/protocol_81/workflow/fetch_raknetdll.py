#!/usr/bin/env python3
"""
Download RakNetDLL release assets and normalize their filenames for PexCraft.

Original release asset names are architecture-specific, for example:
  libRakNetDLL-linux-x64.so
  RakNetDLL-windows-x64.dll

PexCraft stores them under architecture folders with stable loader names:
  transport/bin/linux-x64/libraknet.so
  transport/bin/windows-x64/raknet.dll

This keeps runtime loading simple: choose the platform folder, then load one
predictable filename.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
from urllib.request import urlopen

ROOT = Path(__file__).resolve().parents[1]
BIN_ROOT = ROOT / "transport" / "bin"

ASSETS = [
    {
        "platform": "linux-x64",
        "url": "https://github.com/bargmy/RakNetDLL/releases/download/v1.0.0/libRakNetDLL-linux-x64.so",
        "sha256": "a29861e00f76cab2cfb2f9c673d529187d98e74602a58698b043175c38c5322e",
        "normalized_name": "libraknet.so",
    },
    {
        "platform": "linux-x86",
        "url": "https://github.com/bargmy/RakNetDLL/releases/download/v1.0.0/libRakNetDLL-linux-x86.so",
        "sha256": "6076549b90c644537d7e55238d8517105947b466f2888dac10d67786d0e2eefb",
        "normalized_name": "libraknet.so",
    },
    {
        "platform": "linux-arm64",
        "url": "https://github.com/bargmy/RakNetDLL/releases/download/v1.0.0/libRakNetDLL-linux-arm64.so",
        "sha256": "2a7c0ad2971849e71ac0af9309abc7ba66809c7447a11bcac975aeab922d406a",
        "normalized_name": "libraknet.so",
    },
    {
        "platform": "windows-arm64",
        "url": "https://github.com/bargmy/RakNetDLL/releases/download/v1.0.0/RakNetDLL-windows-arm64.dll",
        "sha256": "7b0a9d973d184a6f0386250fa6da69faa9fc9ff1401d69cfb0af518e2cd6b6a9",
        "normalized_name": "raknet.dll",
    },
    {
        "platform": "windows-x64",
        "url": "https://github.com/bargmy/RakNetDLL/releases/download/v1.0.0/RakNetDLL-windows-x64.dll",
        "sha256": "eb2e7a02882497b3d0d877e7072266ad5528cd6d55225e73ccfa4448ddce04b2",
        "normalized_name": "raknet.dll",
    },
    {
        "platform": "windows-x86",
        "url": "https://github.com/bargmy/RakNetDLL/releases/download/v1.0.0/RakNetDLL-windows-x86.dll",
        "sha256": "f60739f0a1020f120e82ac14938ff4c23b89387725a2185e7f964ad9aecfe316",
        "normalized_name": "raknet.dll",
    },
]


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def download(url: str, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    with urlopen(url) as r, dst.open("wb") as f:
        while True:
            chunk = r.read(1024 * 1024)
            if not chunk:
                break
            f.write(chunk)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--bin-root", type=Path, default=BIN_ROOT)
    parser.add_argument("--skip-existing", action="store_true")
    args = parser.parse_args()

    manifest = []
    for asset in ASSETS:
        platform = asset["platform"]
        dst = args.bin_root / platform / asset["normalized_name"]

        if not (args.skip_existing and dst.exists()):
            print(f"download {platform}: {asset['url']} -> {dst}")
            download(asset["url"], dst)
        else:
            print(f"keep existing {dst}")

        got = sha256_file(dst)
        if got.lower() != asset["sha256"].lower():
            raise SystemExit(
                f"SHA256 mismatch for {dst}\nexpected: {asset['sha256']}\nactual:   {got}"
            )

        if platform.startswith("linux"):
            os.chmod(dst, 0o755)

        manifest.append({
            "platform": platform,
            "path": str(dst.relative_to(ROOT)),
            "normalized_name": asset["normalized_name"],
            "source_url": asset["url"],
            "sha256": got,
        })

    manifest_path = args.bin_root / "raknetdll_manifest.json"
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {manifest_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
