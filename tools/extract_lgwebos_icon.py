#!/usr/bin/env python3
"""Generate an original PexCraft webOS icon.

The old workflow downloaded client.jar and copied pack.png into the webOS IPK.
This script intentionally creates a simple original PNG so the webOS artifact does
not bundle Mojang-derived resources.
"""
import struct
import sys
import zlib
from pathlib import Path


def chunk(kind: bytes, data: bytes) -> bytes:
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)


def write_png(path: Path, size: int = 128) -> None:
    rows = []
    for y in range(size):
        row = bytearray([0])  # PNG filter type 0
        for x in range(size):
            # Original simple blocky grass/dirt icon; no external assets.
            border = x < 4 or y < 4 or x >= size - 4 or y >= size - 4
            if border:
                r, g, b, a = 24, 24, 24, 255
            elif y < size * 0.34:
                r, g, b, a = 76, 178, 72, 255
            elif y < size * 0.48:
                r, g, b, a = 52, 139, 48, 255
            else:
                checker = ((x // 16) + (y // 16)) & 1
                if checker:
                    r, g, b, a = 118, 80, 43, 255
                else:
                    r, g, b, a = 91, 61, 36, 255
            # Small P-shaped highlight made from rectangles.
            if 34 <= x < 48 and 35 <= y < 94:
                r, g, b, a = 238, 238, 238, 255
            if 48 <= x < 82 and 35 <= y < 49:
                r, g, b, a = 238, 238, 238, 255
            if 48 <= x < 82 and 58 <= y < 72:
                r, g, b, a = 238, 238, 238, 255
            if 72 <= x < 86 and 45 <= y < 62:
                r, g, b, a = 238, 238, 238, 255
            row.extend((r, g, b, a))
        rows.append(bytes(row))

    raw = b"".join(rows)
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9))
    png += chunk(b"IEND", b"")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(png)


def main() -> int:
    out = Path(sys.argv[1] if len(sys.argv) > 1 else "webos/generated/icon.png")
    write_png(out)
    print(f"wrote original LG webOS icon: {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
