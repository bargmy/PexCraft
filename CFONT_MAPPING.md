# PexCraft embedded cfont mapping

This patch adds `src/render/cfont.c`, a zlib-compressed embedded fallback font bundle for the non-default Minecraft-style fonts supplied in the conversation.

## Routing

| Language/script | Embedded source | Family in `cfont.c` |
|---|---|---|
| Arabic / Persian (`ar_SA`, `fa_IR`, Arabic-script codepoints) | Rooyin Bitmap | `CFONT_ROOYIN` |
| Chinese (`zh_CN`, `zh_TW`) | Cubic Pixel Fonts active `assets/minecraft/font/font.ttf` rasterized to Java-style glyph pages | `CFONT_CUBIC_ZH` |
| Japanese (`ja_JP`) | Japanese Font Better bitmap providers converted to Java-style glyph pages | `CFONT_JFB` |
| Korean (`ko_KR`) | Cubic Pixel Fonts `fontpack/混搭.ttf` rasterized to Java-style glyph pages | `CFONT_CUBIC_KO` |
| Cyrillic / Greek / Hebrew / Thai / Georgian / Armenian / Latin extended / symbols | MinecraftFont.zip unicode pages | `CFONT_MCFONT` |

## Runtime behavior

`cfont.c` stores each used 256x256 Unicode glyph page as compressed alpha data. Pages are decompressed and uploaded lazily the first time a glyph from that page is drawn. Width tables are also zlib-compressed and loaded per family on demand.

The normal Java 1.2.5 font path is still kept as fallback:

1. `font/default.png` + `font.txt` for default-font characters when Unicode mode is off.
2. Embedded `cfont.c` family for the language/script being rendered.
3. Extracted Java `/font/glyph_sizes.bin` and `/font/glyph_%02X.png` if no embedded font has the glyph.
4. Default font fallback when possible.

The language list now passes the row language code into the renderer, so `日本語`, Chinese names, Arabic names, Korean names, Cyrillic names, etc. route to the intended font even when the currently selected language is English.

## Arabic/Persian shaping

`gui_primitives.c` now performs a compact Arabic contextual shaping pass before the Bidi reorder. This maps Arabic/Persian letters into presentation forms so Rooyin can draw joined words instead of separated isolated letters. It also handles common lam-alef ligatures.

This is intentionally a small built-in shaper, not ICU/HarfBuzz. It is enough for Minecraft-style Arabic/Persian UI strings and language names, while keeping the port dependency-light.
