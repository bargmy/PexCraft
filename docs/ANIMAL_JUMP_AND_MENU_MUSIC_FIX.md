# Animal jump and title-menu music fix

## Animal navigation

The animal path follower was consuming a one-block-higher path node before the
mob had physically climbed or jumped to it. The old test measured horizontal
X/Z distance and separately allowed a Y difference below 1.25 blocks. At the
face of a full block, the higher node therefore looked "reached", leaving the
move helper with no upward target and no reason to request a jump.

The corrected implementation follows Minecraft Java Edition 1.2.5:

- `PathNavigate.pathFollow()` uses the pathable Y coordinate.
- Node reach uses full three-dimensional squared distance.
- The reach radius is exactly `entity.width * entity.width`.
- Proximity advancement happens before direct-path shortcutting.
- `EntityMoveHelper` compares the target Y to `floor(minY + 0.5)`.
- It requests a jump only for an upward node within one horizontal block.
- Normal living-entity jumps use `motionY = 0.42`, a 10-tick jump delay, jump
  potion height, and the sprint impulse only for a sprinting ocelot.

Affected file: `src/game/mobs.c`.

## Main-menu music

Entering `SCREEN_TITLE` used to clear `g_menu_music_started` unconditionally.
Returning from Options, World Select, Texture Packs, Language, or another title
submenu then called `pex_menu_music_start_once()` again. The sound backend
replaced the still-playing title track, making it audibly restart.

The title and its submenus now share one continuous menu-audio session. The
flag is cleared only by the existing true session boundaries, such as entering
or leaving a world through `pex_sound_stop_world_audio()` or
`pex_menu_music_stop()`.

Affected file: `src/ui/screen_state_input.c`.

## Validation

- Compiled and ran a focused C test for upward-node retention, same-level path
  advancement, and near/far jump requests.
- Ran source integration assertions for the Java 1.2.5 constants and title
  branch behavior.
- Checked the generated patch for whitespace errors.
- Tested the final ZIP with `unzip -t`.

A complete SDL2/OpenGL build was not possible in the execution environment
because SDL2 and SDL2_image development headers are not installed.
