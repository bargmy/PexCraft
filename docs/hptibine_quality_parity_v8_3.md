# HptiBine v8.3 Quality parity audit

Target references:

- Minecraft Java Edition 1.2.5 source (`src(2).zip`)
- the reference 1.2.5 mod Java source (`hptibine_1.2.5_HD_C6(1).zip`)
- User-supplied HptiBine 1.7.2 jar language files, used only as fallback translations for shared HptiBine option keys because the reference 1.2.5 mod does not bundle language files.

## Implemented Quality options

| Option | v8.3 status | Notes |
|---|---|---|
| Brightness | Behavior/math parity | Uses the Minecraft/HptiBine gamma curve and labels Moody / +N% / Bright. PexCraft still applies it through its renderer/light path, so screenshot-level identity is not guaranteed. |
| Clear Water | Behavior parity | Water opacity switches 3 -> 1, then relights/rebuilds active chunks, matching HptiBine's loaded-world effect. |
| Better Grass | Close parity | Uses HptiBine's Fast / Fancy / OFF cycle and terrain side-tile rules for grass, snow grass, and mycelium. |
| Better Snow | Logic parity | Uses HptiBine's snow-layer-neighbor + opaque-block-below trigger and visual fake snow heights. |
| Custom Fonts | Safe partial | Gates texture-pack font override. Full HptiBine font renderer behavior is not complete. |
| Custom Colors | Partial HptiBine hook support | Supports grasscolor, foliagecolor, watercolorX, pinecolor, birchcolor, swampgrasscolor, and swampfoliagecolor PNG hooks. Full color.properties/custom-lightmap system is not complete. |
| Swamp Colors | Vanilla/HptiBine behavior parity for current renderer | Adds Java swampland tint, Java swamp water color, ON/OFF toggle, and custom swamp maps when provided. |
| Smooth Biomes | Core behavior parity | Averages nearby biome color samples for grass/foliage paths; water keeps the vanilla 3x3 behavior and custom-water smoothing path. |

## Language/localization audit

- the reference 1.2.5 mod source/compiled zips: no bundled `.lang` files found.
- the later reference jar jar: 22 `assets/minecraft/hptibine/lang/*.lang` files found.
- v8.3 embeds shared `hb.options.*` translations from that 1.7.2 jar as fallback strings only; it does not claim they are 1.2.5 language files.
- HptiBine menu/category/option buttons now use `tr_key_default()` with HptiBine-compatible keys like `hb.options.quality`, `hb.options.CLEAR_WATER`, `hb.options.BETTER_GRASS`, etc.

## Still not full Quality-page parity

These remain not implemented or partial:

- Random Mobs
- Mipmap Level
- Mipmap Type
- Connected Textures
- Natural Textures
- Antialiasing
- Anisotropic Filtering
- Full HptiBine Custom Colors: `color.properties`, custom block palettes, sky/fog/underwater maps, redstone/stem/mycelium particle colors, and `environment/lightmap*.png`
- Full HptiBine Custom Fonts behavior beyond gating texture-pack font overrides
