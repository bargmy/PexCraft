# Release sound downloader pass

Scope requested: download everything from Mojang legacy.json except in-game background music.

Included asset classes:
- `sound/**`
- `sounds/**`, deduped into `resources/sound/**`
- `sounds/music/menu/**`, mapped to `resources/music/menu/**`
- `records/**` and `sounds/records/**`, deduped into `resources/streaming/**`

Excluded asset classes:
- `music/**` root background tracks
- `sounds/music/game/**` background in-game songs

The filtered/deduped set from the provided legacy.json is 498 files, 30,088,008 bytes.

User flow:
- If Release sounds are missing, the startup resource prompt shows the sound download size.
- The user can toggle Sounds ON/OFF or Ignore to play without downloaded sounds.
- The install screen now has a Cancel button. Cancel disables the optional sound download for that options file.

Runtime mapping:
- Mojang's duplicated `sounds/` root is normalized to the existing PexCraft `resources/sound/` tree.
- Records are normalized to `resources/streaming/` so jukebox playback uses the existing record backend.
- Menu music now chooses any installed `menu1.ogg` through `menu4.ogg` instead of only `menu2.ogg`.
- Villager Java 1.2.5 sound keys are aliased to the available legacy filenames.
