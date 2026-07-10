# HptiBine v8.3 branding cleanup

This package keeps HptiBine naming for its own UI, settings, language files, sample color maps, and source identifiers.

Changed:
- HptiBine language exports live in `hptibine/langs/`.
- HptiBine translation keys use `hb.options.*`.
- HptiBine saved settings use `hb*` keys in `optionshptibine.txt`.
- HptiBine sample color maps live in `hptibine_custom_color_pngs/`.
- Source include name is `src/i18n/hptibine_lang.inc`.

Verification run:
- `tools/check_hptibine_quality_parity.py` passes.
- Source tree grep for external mod branding and old external key prefixes returns no matches.
