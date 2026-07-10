#!/usr/bin/env python3
"""Lightweight source-level StivuFine Quality parity checks.

This is not a renderer test. It verifies that the v8.4 source still exposes the
StivuFine-compatible Quality keys/defaults and that the StivuFine-only keys/defaults are wired.
"""
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
options = (root / "src/settings/options.c").read_text(encoding="utf-8", errors="replace")
common = (root / "src/common/common.h").read_text(encoding="utf-8", errors="replace")
world = (root / "src/render/world_view.c").read_text(encoding="utf-8", errors="replace")

checks = [
    ("Brightness option enabled", r'\{\s*SF_GAMMA,\s*"Brightness".*SF_KIND_SLIDER,\s*1\s*\}', options),
    ("Clear Water option enabled", r'\{\s*SF_CLEAR_WATER,\s*"Clear Water".*SF_KIND_BUTTON,\s*1\s*\}', options),
    ("Better Grass option enabled", r'\{\s*SF_BETTER_GRASS,\s*"Better Grass".*SF_KIND_BUTTON,\s*1\s*\}', options),
    ("Better Snow option enabled", r'\{\s*SF_BETTER_SNOW,\s*"Better Snow".*SF_KIND_BUTTON,\s*1\s*\}', options),
    ("Swamp Colors option enabled", r'\{\s*SF_SWAMP_COLORS,\s*"Swamp Colors".*SF_KIND_BUTTON,\s*1\s*\}', options),
    ("Smooth Biomes option enabled", r'\{\s*SF_SMOOTH_BIOMES,\s*"Smooth Biomes".*SF_KIND_BUTTON,\s*1\s*\}', options),
    ("Better Grass default OFF", r'g_opts\.sf_grass\s*=\s*SF_OFF\s*;', options),
    ("Swamp Colors default ON", r'g_opts\.sf_swamp_colors\s*=\s*1\s*;', options),
    ("Smooth Biomes default ON", r'g_opts\.sf_smooth_biomes\s*=\s*1\s*;', options),
    ("Clear Water save/load key", r'sfClearWater', options),
    ("Better Snow save/load key", r'sfBetterSnow', options),
    ("Swamp Colors save/load key", r'sfSwampColors', options),
    ("Smooth Biomes save/load key", r'sfSmoothBiomes', options),
    ("StivuFine option name localization keys", r'stivufine_option_lang_key', options),
    ("Better Snow snow-layer neighbor rule", r'BLOCK_SNOW_LAYER', world),
    ("Swamp color Java tint constant", r'0x4E0E4E|5115470', world),
    ("Swamp water Java color", r'0xE0FFAE|14745518', world),
]

failed = []
for name, pattern, text in checks:
    ok = re.search(pattern, text, re.S) is not None
    print(f"{'PASS' if ok else 'FAIL'} - {name}")
    if not ok:
        failed.append(name)

if failed:
    print("\nFailed checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)

# v8.4 intentionally has no imported 1.7-era language fallback include.
if (root / "src/i18n/stivufine_lang.inc").exists() or (root / "stivufine/langs").exists():
    print("FAIL - no imported language files")
    sys.exit(1)
print("PASS - no imported language files")
print("\nAll v8.4 source-level parity checks passed.")
