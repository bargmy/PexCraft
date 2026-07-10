#!/usr/bin/env python3
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
files = {
    'options': (ROOT/'src/settings/options.c').read_text(errors='replace'),
    'common': (ROOT/'src/common/common.h').read_text(errors='replace'),
    'textures': (ROOT/'src/assets/textures.c').read_text(errors='replace'),
    'world': (ROOT/'src/render/world_view.c').read_text(errors='replace'),
    'sdl': (ROOT/'src/platform/sdl2_app.c').read_text(errors='replace'),
}

def png_size(path: Path):
    b = path.read_bytes()
    assert b[:8] == b'\x89PNG\r\n\x1a\n'
    return struct.unpack('>II', b[16:24])

checks = []
def check(name, ok): checks.append((name, bool(ok)))

opt = files['options']; tex = files['textures']; world = files['world']; sdl = files['sdl']
for token in ('SF_CONNECTED_TEXTURES', 'SF_NATURAL_TEXTURES', 'SF_AA_LEVEL', 'SF_AF_LEVEL'):
    pattern = r'\{\s*' + re.escape(token) + r'\s*,[^\n]*,\s*1\s*\}'
    check(f'{token} enabled', re.search(pattern, opt) is not None)
for key in ('sfConnectedTextures','sfNaturalTextures','sfAaLevel','sfAfLevel'):
    check(f'{key} saved/loaded', opt.count(key) >= 3)
check('CTM methods', all(x in tex for x in ('"ctm"','"horizontal"','"vertical"','"top"','"random"','"repeat"')))
check('CTM fancy corners', 'index=46' in world and 'index=32' in world and 'index=33' in world)
check('CTM tile priority', 'static const int kinds[] = {2, 1}' in tex)
check('CTM C6 random face behavior', 'stivufine_coord_random(x,y,z,face)' in world)
check('Combined terrain atlas', 'stivufine_build_combined_terrain_atlas' in tex and 'stivufine_terrain_cells_per_row' in world)
check('Natural source textures', 'stivufine_natural_texture_slot' in tex and 'tile/256' in world)
check('Natural transforms', all(x in tex for x in ('"4F"','"2F"','"F"')))
check('Anisotropy extension', 'GL_EXT_texture_filter_anisotropic' in tex and 'GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT' in tex)
check('AA attributes', 'SDL_GL_MULTISAMPLEBUFFERS' in sdl and 'SDL_GL_MULTISAMPLESAMPLES' in sdl)
check('AA fallback', 'retrying without AA' in sdl and sdl.count('stivufine_configure_gl_attributes(0)') >= 1)
ctm = ROOT/'stivufine/ctm.png'
check('CTM PNG included', ctm.exists() and png_size(ctm) == (256,256))
check('Natural template included', (ROOT/'stivufine/template.natural.properties').exists())

bad = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(('PASS' if ok else 'FAIL') + ': ' + name)
if bad:
    raise SystemExit(f'{len(bad)} checks failed')
print(f'All {len(checks)} StivuFine v10 Quality checks passed.')
