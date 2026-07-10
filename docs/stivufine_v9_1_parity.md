# StivuFine v9.1 parity fixes

- Random Mobs selects variants with Java decimal-string `hashCode()` and scans sequential variants through texture number 1000.
- Mob persistent IDs use Java `Random.nextInt(Integer.MAX_VALUE)` semantics and remain stored in PexCraft saves.
- Mipmaps use the reference alpha-weighted two-stage blend.
- Transparent terrain pixels inherit their own tile's average opaque RGB before mip generation.
- Font atlases and Unicode glyph pages never receive mipmaps.
- Portal animation immediately regenerates terrain mip levels and restores the selected mipmap filter.
