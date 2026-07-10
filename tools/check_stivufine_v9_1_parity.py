#!/usr/bin/env python3
from pathlib import Path
import sys
root = Path(__file__).resolve().parents[1]
files = {
    'textures': (root/'src/assets/textures.c').read_text(errors='replace'),
    'mobs': (root/'src/game/mobs.c').read_text(errors='replace'),
    'world': (root/'src/render/world_view.c').read_text(errors='replace'),
    'cfont': (root/'src/render/cfont.c').read_text(errors='replace'),
}
checks = [
    ('Java decimal-string hash', 'h = h * 31u' in files['textures']),
    ('Random Mobs length guard', 'strlen(skin_url) <= 1' in files['textures']),
    ('Variant range reaches 1000', 'SF_RANDOM_MOB_MAX_TEXTURE_NUMBER 1000' in files['textures']),
    ('Variants allocated dynamically', 'realloc(set->variants' in files['textures']),
    ('Old fixed 15 variant cap removed', 'SF_RANDOM_MOB_MAX_VARIANTS' not in files['textures']),
    ('Java bounded random ID generation', 'passive_random_mob_java_next_int_bound(0x7fffffff)' in files['mobs']),
    ('Alpha weighted pair blend', 'stivufine_alpha_blend_pair' in files['textures']),
    ('Four pixel reference blend order', 'stivufine_alpha_blend_four(p1, p2, p3, p4, dp)' in files['textures']),
    ('Terrain transparent RGB preparation', 'stivufine_prepare_terrain_transparent_rgb' in files['textures']),
    ('Font texture exclusion', 'stivufine_texture_is_font' in files['textures']),
    ('Embedded font suppression', 'stivufine_mipmaps_suppressed++' in files['cfont']),
    ('Portal regenerates mipmaps', 'stivufine_rebuild_terrain_mipmaps();' in files['world'][files['world'].find('static void update_portal_texture_animation'):files['world'].find('typedef struct PexLiquidTextureFX')]),
    ('No old branding', all(x not in '\n'.join(files.values()).lower() for x in ('opti'+'fine', 'hpti'+'bine'))),
]
failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(('PASS' if ok else 'FAIL') + ': ' + name)
if failed:
    sys.exit(1)
