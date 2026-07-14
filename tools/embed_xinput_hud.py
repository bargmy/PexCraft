#!/usr/bin/env python3
"""Convert src/assets/XINPUT.png into RGBA bytes compiled into the WASM module."""

from __future__ import annotations

import argparse
import hashlib
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src" / "assets" / "XINPUT.png"
OUTPUT = ROOT / "src" / "assets" / "xinput_hud_rgba.inc"
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def paeth(a: int, b: int, c: int) -> int:
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def decode_rgba_png(data: bytes) -> tuple[int, int, bytes]:
    if not data.startswith(PNG_SIGNATURE):
        raise ValueError("XINPUT.png is not a PNG")

    pos = len(PNG_SIGNATURE)
    width = height = 0
    idat = bytearray()
    saw_ihdr = False
    saw_iend = False

    while pos + 12 <= len(data):
        length = struct.unpack(">I", data[pos : pos + 4])[0]
        kind = data[pos + 4 : pos + 8]
        start = pos + 8
        end = start + length
        crc_end = end + 4
        if crc_end > len(data):
            raise ValueError("truncated PNG chunk")
        payload = data[start:end]
        expected_crc = struct.unpack(">I", data[end:crc_end])[0]
        actual_crc = zlib.crc32(kind)
        actual_crc = zlib.crc32(payload, actual_crc) & 0xFFFFFFFF
        if actual_crc != expected_crc:
            raise ValueError(f"bad PNG CRC in {kind!r}")

        if kind == b"IHDR":
            if length != 13:
                raise ValueError("invalid IHDR length")
            width, height, bit_depth, color_type, compression, filtering, interlace = struct.unpack(
                ">IIBBBBB", payload
            )
            if (bit_depth, color_type, compression, filtering, interlace) != (8, 6, 0, 0, 0):
                raise ValueError(
                    "XINPUT.png must be an 8-bit, non-interlaced RGBA PNG"
                )
            saw_ihdr = True
        elif kind == b"IDAT":
            idat.extend(payload)
        elif kind == b"IEND":
            saw_iend = True
            break

        pos = crc_end

    if not saw_ihdr or not saw_iend or width <= 0 or height <= 0 or not idat:
        raise ValueError("incomplete PNG")
    if (width, height) != (64, 64):
        raise ValueError(f"XINPUT.png must be 64x64, got {width}x{height}")

    packed = zlib.decompress(bytes(idat))
    bpp = 4
    stride = width * bpp
    expected = height * (stride + 1)
    if len(packed) != expected:
        raise ValueError(f"unexpected decoded PNG size: {len(packed)} != {expected}")

    rgba = bytearray(height * stride)
    src = 0
    for y in range(height):
        filter_type = packed[src]
        src += 1
        row = bytearray(packed[src : src + stride])
        src += stride
        prev_off = (y - 1) * stride
        for x in range(stride):
            left = row[x - bpp] if x >= bpp else 0
            up = rgba[prev_off + x] if y > 0 else 0
            up_left = rgba[prev_off + x - bpp] if y > 0 and x >= bpp else 0
            if filter_type == 0:
                value = row[x]
            elif filter_type == 1:
                value = (row[x] + left) & 0xFF
            elif filter_type == 2:
                value = (row[x] + up) & 0xFF
            elif filter_type == 3:
                value = (row[x] + ((left + up) >> 1)) & 0xFF
            elif filter_type == 4:
                value = (row[x] + paeth(left, up, up_left)) & 0xFF
            else:
                raise ValueError(f"unsupported PNG filter {filter_type}")
            row[x] = value
        rgba[y * stride : (y + 1) * stride] = row

    return width, height, bytes(rgba)


def render_include(source: bytes) -> str:
    width, height, rgba = decode_rgba_png(source)
    digest = hashlib.sha256(source).hexdigest()
    lines = [
        "/* Generated from XINPUT.png by tools/embed_xinput_hud.py.",
        f"   source-sha256: {digest}",
        f"   dimensions: {width}x{height} RGBA8; bytes: {len(rgba)} */",
    ]
    for offset in range(0, len(rgba), 16):
        chunk = rgba[offset : offset + 16]
        lines.append("    " + ", ".join(f"0x{value:02X}" for value in chunk) + ",")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--check",
        action="store_true",
        help="fail instead of rewriting when the generated include is stale",
    )
    args = parser.parse_args()

    if not SOURCE.is_file():
        print(f"missing controller sprite sheet: {SOURCE}", file=sys.stderr)
        return 1

    try:
        expected = render_include(SOURCE.read_bytes())
    except (OSError, ValueError, zlib.error) as exc:
        print(f"could not embed XINPUT.png: {exc}", file=sys.stderr)
        return 1

    current = OUTPUT.read_text(encoding="utf-8") if OUTPUT.is_file() else None
    if args.check:
        if current != expected:
            print(
                f"{OUTPUT.relative_to(ROOT)} is stale; run tools/embed_xinput_hud.py",
                file=sys.stderr,
            )
            return 1
        print("Embedded XInput HUD RGBA is up to date")
        return 0

    if current != expected:
        OUTPUT.write_text(expected, encoding="utf-8", newline="\n")
        print(f"Generated {OUTPUT.relative_to(ROOT)}")
    else:
        print(f"Up to date: {OUTPUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
