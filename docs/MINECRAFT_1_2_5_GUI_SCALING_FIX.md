# Minecraft 1.2.5 GUI scaling parity fix

## Problem

The previous renderer treated Minecraft GUI elements as generic stretchable or
nine-patch panels. It also rounded the scaled GUI size before building the
OpenGL projection and applied half-texel UV trimming. At large GUI scales those
differences made the one-pixel bevels in `gui.png` look too thin or uneven and
made resource-pack replacements sample incorrectly.

## Reference behavior

This implementation follows the supplied Minecraft 1.2.5 source:

- `ScaledResolution` selects the largest integer scale that leaves at least a
  320 x 240 logical GUI, subject to the GUI Scale setting.
- Layout dimensions use `ceil(display / scale)`.
- `EntityRenderer.setupOverlayRendering` projects with the unsnapped
  `scaledWidthD` and `scaledHeightD` values.
- `Gui.drawTexturedModalRect` uses exact 1/256 atlas coordinates.
- `GuiButton.drawButton` renders two halves from `gui.png`; it is not a
  nine-patch and its native logical height is 20 pixels.

## Changes

- Added localized GUI Scale values: Auto, Small, Normal and Large.
- Made Auto the default.
- Added exact double-precision scaled GUI dimensions for projection.
- Preserved SDL high-DPI drawable dimensions instead of replacing them with
  logical window dimensions every frame.
- Removed half-texel trimming from fixed 256-unit GUI atlas sampling.
- Replaced custom/tall button reconstruction with the exact two-half vanilla
  button draw.
- Normalized visible standard buttons to the vanilla 20 logical-pixel height.
- Kept GUI textures nearest-filtered and mipmap-free.
- Kept resource-pack `gui.png`, item, terrain, container, achievement and slot
  textures authoritative at every supported resolution.

## Validation

- C99 ScaledResolution parity harness passed for 357x250, 957x516, 1280x720,
  1920x1080 and 3840x2160, plus all explicit GUI Scale values.
- Fixed-atlas UV parity passed for 256, 512 and 1024 pixel GUI sheets.
- Source audit found no remaining custom button nine-patch path and no legacy
  tall literal standard buttons.

A full SDL2/OpenGL application link was not performed in the packaging
environment because the required development headers and libraries are not
installed there.
