# StivuFine v8.4 branding cleanup

This package keeps StivuFine naming for its own UI, settings, language files, sample color maps, and source identifiers.

Changed:
- StivuFine language exports live in `stivufine/langs/`.
- StivuFine translation keys use `sf.options.*`.
- StivuFine saved settings use `hb*` keys in `optionsstivufine.txt`.
- StivuFine sample color maps live in `stivufine/custom_color_pngs/`.
- Source include name is `src/i18n/stivufine_lang.inc`.

Verification run:
- `tools/check_stivufine_quality_parity.py` passes.
- Source tree grep for external mod branding and old external key prefixes returns no matches.
