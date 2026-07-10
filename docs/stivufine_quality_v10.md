# StivuFine v10 Quality completion

This version adds the four previously disabled Quality options without replacing
PexCraft's chunk/section mesh architecture.

## Connected Textures

- Values: Fast, Fancy, OFF. Default: Fancy.
- Supports the legacy `ctm/block<ID>[a-z].properties` and
  `ctm/terrain<TILE>[a-z].properties` layout.
- Supported methods: `ctm`, `horizontal`, `vertical`, `top`, `random`, and
  `repeat`, including metadata, faces, connect mode, weights, symmetry, width,
  and height.
- Includes `stivufine/ctm.png` for the default glass/bookshelf/sandstone rules.
- CTM source sheets are packed into a combined terrain atlas, preserving
  PexCraft's single-terrain-texture mesh batches.

## Natural Textures

- Values: ON/OFF. Default: OFF.
- Uses deterministic Java-style coordinate hashing.
- Supports `natural.properties` entries for `/terrain.png` and CTM source
  textures, with `2`, `4`, `F`, `2F`, and `4F` transforms.
- The built-in default mapping matches the supplied 1.2.5 template.

## Anisotropic Filtering

- Values: OFF, 2x, 4x, 8x, 16x. Default: OFF (`1`).
- Detects `GL_EXT_texture_filter_anisotropic`, clamps to the driver's maximum,
  and safely remains at 1x when unavailable.

## Antialiasing

- Values: OFF, 2x, 4x, 6x, 8x, 12x, 16x. Default: OFF.
- Desktop SDL2/OpenGL requests multisampling before window/context creation.
- It retries without multisampling if the requested mode is unsupported.
- Changing this option requires restart, as indicated in the Quality menu.

## Backend safety

Connected/Natural texture selection stays in the CPU mesh/UV layer and therefore
continues to feed the existing OpenGL and direct backend upload paths. AF is a
no-op when the GL extension is absent. AA currently applies to SDL2/OpenGL
context creation; other backends keep their existing framebuffer setup.
