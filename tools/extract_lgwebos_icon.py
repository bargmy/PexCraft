#!/usr/bin/env python3
"""Download Minecraft Classic client.jar and extract pack.png as webOS icon."""
import sys
import urllib.request
import zipfile
from pathlib import Path

CLASSIC_PACK_URL = "https://piston-data.mojang.com/v1/objects/93faf3398ebf8008d59852dc3c2b22b909ca8a49/client.jar"


def main() -> int:
    out = Path(sys.argv[1] if len(sys.argv) > 1 else "webos/generated/icon.png")
    jar = Path(sys.argv[2] if len(sys.argv) > 2 else "build/lgwebos_work/client.jar")
    jar.parent.mkdir(parents=True, exist_ok=True)
    out.parent.mkdir(parents=True, exist_ok=True)
    if not jar.exists():
        print(f"downloading client.jar -> {jar}")
        urllib.request.urlretrieve(CLASSIC_PACK_URL, jar)
    with zipfile.ZipFile(jar, "r") as zf:
        if "pack.png" not in zf.namelist():
            raise SystemExit("client.jar does not contain pack.png")
        out.write_bytes(zf.read("pack.png"))
    print(f"wrote LG webOS icon: {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
