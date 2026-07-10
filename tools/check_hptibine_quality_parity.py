#!/usr/bin/env python3
"""Lightweight source-level HptiBine Quality parity checks.

This is not a renderer test. It verifies that the v8.3 source still exposes the
HptiBine-compatible Quality keys/defaults and that localization hooks are wired.
"""
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
options = (root / "src/settings/options.c").read_text(encoding="utf-8", errors="replace")
common = (root / "src/common/common.h").read_text(encoding="utf-8", errors="replace")
world = (root / "src/render/world_view.c").read_text(encoding="utf-8", errors="replace")
lang = (root / "src/i18n/language.c").read_text(encoding="utf-8", errors="replace")
inc_path = root / "src/i18n/hptibine_lang.inc"
inc = inc_path.read_text(encoding="utf-8", errors="replace") if inc_path.exists() else ""

checks = [
    ("Brightness option enabled", r'\{\s*HPTI_GAMMA,\s*"Brightness".*HPTI_KIND_SLIDER,\s*1\s*\}', options),
    ("Clear Water option enabled", r'\{\s*HPTI_CLEAR_WATER,\s*"Clear Water".*HPTI_KIND_BUTTON,\s*1\s*\}', options),
    ("Better Grass option enabled", r'\{\s*HPTI_BETTER_GRASS,\s*"Better Grass".*HPTI_KIND_BUTTON,\s*1\s*\}', options),
    ("Better Snow option enabled", r'\{\s*HPTI_BETTER_SNOW,\s*"Better Snow".*HPTI_KIND_BUTTON,\s*1\s*\}', options),
    ("Swamp Colors option enabled", r'\{\s*HPTI_SWAMP_COLORS,\s*"Swamp Colors".*HPTI_KIND_BUTTON,\s*1\s*\}', options),
    ("Smooth Biomes option enabled", r'\{\s*HPTI_SMOOTH_BIOMES,\s*"Smooth Biomes".*HPTI_KIND_BUTTON,\s*1\s*\}', options),
    ("Better Grass default OFF", r'g_opts\.hpti_grass\s*=\s*HPTI_OFF\s*;', options),
    ("Swamp Colors default ON", r'g_opts\.hpti_swamp_colors\s*=\s*1\s*;', options),
    ("Smooth Biomes default ON", r'g_opts\.hpti_smooth_biomes\s*=\s*1\s*;', options),
    ("Clear Water save/load key", r'hbClearWater', options),
    ("Better Snow save/load key", r'hbBetterSnow', options),
    ("Swamp Colors save/load key", r'hbSwampColors', options),
    ("Smooth Biomes save/load key", r'hbSmoothBiomes', options),
    ("HptiBine option name localization keys", r'hptibine_option_lang_key', options),
    ("HptiBine lang fallback include", r'hptibine_lang_supplements', lang + inc),
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
print("\nAll v8.3 source-level parity checks passed.")
