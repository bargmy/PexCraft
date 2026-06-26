#!/usr/bin/env python3
"""Compare PexCraft worldgen_dump output against reference golden hashes.

Golden CSV columns:
  seed,dimension,chunkX,chunkZ,blocks_sha1,data_sha1,heightmap_sha1,biomes_sha1

Example:
  cc -std=c99 -O2 tools/worldgen_dump.c -lm -o worldgen_dump
  ./worldgen_dump 12345 0 0 0
  python3 tools/worldgen_compare_hashes.py golden.csv current_dump.txt
"""
import csv
import re
import sys
from pathlib import Path

LINE_RE = re.compile(
    r"seed=(?P<seed>-?\d+) dim=(?P<dim>-?\d+) chunk=(?P<cx>-?\d+),(?P<cz>-?\d+) "
    r"blocks_sha1=(?P<blocks>[0-9a-f]{40}) data_sha1=(?P<data>[0-9a-f]{40}) "
    r"heightmap_sha1=(?P<height>[0-9a-f]{40}) biomes_sha1=(?P<biomes>[0-9a-f]{40})"
)

def key(row):
    return (str(row["seed"]), str(row["dimension"]), str(row["chunkX"]), str(row["chunkZ"]))

def load_golden(path):
    rows = {}
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            rows[key(row)] = {
                "blocks": row["blocks_sha1"].lower(),
                "data": row["data_sha1"].lower(),
                "height": row["heightmap_sha1"].lower(),
                "biomes": row["biomes_sha1"].lower(),
            }
    return rows

def load_dump(path):
    rows = {}
    for line in Path(path).read_text(errors="replace").splitlines():
        m = LINE_RE.search(line)
        if not m:
            continue
        rows[(m.group("seed"), m.group("dim"), m.group("cx"), m.group("cz"))] = {
            "blocks": m.group("blocks"),
            "data": m.group("data"),
            "height": m.group("height"),
            "biomes": m.group("biomes"),
        }
    return rows

def main():
    if len(sys.argv) != 3:
        print("usage: worldgen_compare_hashes.py <golden.csv> <current_dump.txt>", file=sys.stderr)
        return 2
    golden = load_golden(sys.argv[1])
    current = load_dump(sys.argv[2])
    total = matched = 0
    for k, exp in sorted(golden.items(), key=lambda x: tuple(int(v) for v in x[0])):
        total += 1
        got = current.get(k)
        if got is None:
            print(f"MISSING seed={k[0]} dim={k[1]} chunk={k[2]},{k[3]}")
            continue
        bad = [name for name in ("blocks", "data", "height", "biomes") if got[name] != exp[name]]
        if bad:
            print(f"DIFF seed={k[0]} dim={k[1]} chunk={k[2]},{k[3]} fields={','.join(bad)}")
            for name in bad:
                print(f"  {name}: expected={exp[name]} actual={got[name]}")
        else:
            matched += 1
            print(f"OK seed={k[0]} dim={k[1]} chunk={k[2]},{k[3]}")
    print(f"summary: {matched}/{total} chunks fully matched")
    return 0 if matched == total else 1

if __name__ == "__main__":
    raise SystemExit(main())
