# PexCraft Classic Achievements and Statistics Implementation

## Scope

This change ports the Minecraft 1.2.5 statistics and achievement model into PexCraft, including the persistent top-right **Taking Inventory** tutorial prompt and the normal **Achievement get!** notification.

The implementation uses the original 1.2.5 registry keys, achievement tree, parent relationships, coordinates, icons, texture regions, notification timing, and profile-wide behavior. It is integrated with PexCraft's existing localization, downloaded Release resources, selected texture packs, input bindings, world lifecycle, inventory, mobs, dimensions, and save directories.

## Main implementation files

### New core module

- `src/game/achievements.c`
  - 23 general-stat definitions with legacy IDs.
  - Per-block and per-item mined, crafted, used, depleted, and picked-up counters.
  - All 27 Minecraft 1.2.5 achievement definitions.
  - Parent/prerequisite enforcement.
  - Achievement unlock queue and notification state.
  - Profile-wide load/save logic.
  - Localized title and description lookup.
  - Dynamic configured-key insertion for the Taking Inventory prompt.

### New UI module

- `src/ui/achievements_ui.c`
  - Persistent Taking Inventory tutorial popup.
  - Three-second classic achievement notification animation.
  - Original `achievement/bg.png` texture regions.
  - Real item and block icons from the active atlases.
  - Achievement-tree terrain generated with the original Java RNG algorithm.
  - Original tree coordinates, parent connectors, special frames, pulse states, dragging, bounds, and tooltips.
  - General, Blocks, and Items statistics screens.
  - Sortable statistic columns, scrolling, touch dragging, real icons, and localized tooltips.

## Achievement registry

The registry contains all 27 classic achievements:

1. Taking Inventory
2. Getting Wood
3. Benchmarking
4. Time to Mine!
5. Hot Topic
6. Acquire Hardware
7. Time to Farm!
8. Bake Bread
9. The Lie
10. Getting an Upgrade
11. Delicious Fish
12. On A Rail
13. Time to Strike!
14. Monster Hunter
15. Cow Tipper
16. When Pigs Fly
17. Sniper Duel
18. DIAMONDS!
19. We Need to Go Deeper
20. Return to Sender
21. Into Fire
22. Local Brewery
23. The End?
24. The End.
25. Enchanter
26. Overkill
27. Librarian

Their original internal keys, parents, display coordinates, icon objects, and special-achievement flags are retained.

## Taking Inventory prompt

Before `openInventory` is unlocked, the upper-right corner continuously displays the classic information popup:

> Press 'E' to open your inventory.

The displayed key is not hardcoded. It is generated from `g_opts.keys[7]`, so rebinding Inventory changes the prompt automatically.

Opening either the normal inventory or creative inventory:

1. Unlocks `openInventory`.
2. Removes the persistent information prompt.
3. Queues the classic three-second notification:
   - `Achievement get!`
   - `Taking Inventory`
4. Persists the unlock for the active player profile across all worlds.

The popup uses:

- `achievement/bg.png`
- Source rectangle `(96, 202, 160, 32)`
- Upper-right positioning
- Real Book icon from `gui/items.png`
- Icon offset `(8, 8)`
- Text offset `(30, 7)`
- Original easing formula and duration

## Localization

Achievement and statistics strings use the existing active-language system and exact classic keys, including:

- `achievement.<key>`
- `achievement.<key>.desc`
- `achievement.get`
- `achievement.taken`
- `achievement.requires`
- `gui.achievements`
- `gui.stats`
- `stat.*`

The existing Release-resource language loader therefore supplies translations from the selected language's downloaded `.lang` file. English fallback text is included for missing keys.

Both Java positional `%1$s` and common texture-pack `%s` placeholders are accepted for the configured inventory key.

## Downloaded resources and selected texture packs

The default resource path now loads the original downloaded Minecraft 1.2.5 assets:

- `achievement/bg.png`
- `gui/slot.png`
- `terrain.png`
- `gui/items.png`

When a custom texture pack is selected, PexCraft attempts to replace each of those assets from that pack. Missing custom assets retain the downloaded Release-resource fallback instead of replacing the UI with hardcoded approximations.

Consequently:

- Achievement popup and tree frames use the selected pack's `achievement/bg.png` when provided.
- Statistics controls use the selected pack's `gui/slot.png` when provided.
- Achievement-tree terrain uses the selected pack's `terrain.png`.
- Achievement and statistic icons use the selected pack's item/block atlases.

The PSP and Wii asset-preparation scripts were also updated to convert and embed:

- `achievement/bg.png` as `achievement_bg.mcrw`
- `gui/slot.png` as `gui_slot.mcrw`

## Profile-wide persistence

Statistics are intentionally not stored in a world snapshot. They are stored by normalized player name under:

```text
<game directory>/stats/stats_<username>.dat
```

The writer uses a temporary file and retains the previous file as `.old` during replacement. Unknown or malformed records are ignored safely. Data is flushed periodically, when leaving a world, and during supported platform shutdown paths.

Tracked data includes:

- General counters
- Block mined/crafted/used/picked counters
- Item crafted/used/depleted/picked counters
- Achievement unlock states

## Gameplay hooks implemented

### General statistics

- Game/world entry
- New-world creation versus existing-world loading
- Multiplayer joins
- Leaving a game
- Play time
- Walking, swimming, diving, climbing, creative flight, and pig travel
- Completed fall distance
- Jumps
- Dropped items
- Damage dealt and received
- Deaths
- Mob kills

### Object statistics

- Successful block breaks
- Successful local crafting output collection
- Furnace output collection
- Item and block use
- Door placement
- Durability depletion
- Ground-item pickup

### Achievement triggers

The implemented gameplay paths cover:

- Opening inventory
- Picking up wood
- Crafting a workbench
- Crafting a wooden pickaxe
- Crafting a furnace
- Taking iron from a furnace
- Crafting a wooden hoe
- Crafting bread
- Crafting cake
- Crafting a stone pickaxe
- Taking cooked fish from a furnace
- Crafting a wooden sword
- Killing a hostile mob
- Picking up leather
- Falling more than five blocks while riding a pig
- Killing a skeleton with an arrow from at least 50 blocks away
- Picking up diamonds
- Entering the Nether
- Killing a Ghast with a reflected fireball
- Picking up a Blaze Rod
- Entering the End
- Killing the Ender Dragon
- Crafting an enchantment table
- Dealing at least nine hearts in one hit
- Crafting a bookshelf

## Menus and controls

The pause menu now follows the classic layout and includes localized:

- Achievements
- Statistics

The Achievements screen returns directly to gameplay through Done, Escape, or the configured Inventory key, matching the original behavior.

The Statistics screen includes:

- General, Blocks, and Items tabs
- Mouse wheel and Page Up/Page Down scrolling
- Pointer/touch drag scrolling
- Sort-state cycling on statistic headers
- Localized names and hover descriptions

## Platform integration

The new unity modules are included in all existing main variants:

- Win32
- SDL2/Linux
- Android
- Android TV
- LG webOS
- PSP
- Wii
- Xbox UWP

Pointer/touch handlers were connected for achievement-tree dragging and statistics scrolling where those platform paths expose them.

## Current engine limitations

The classic registry and UI include every achievement, but two original triggers cannot occur naturally until their underlying PexCraft mechanics exist:

- **On A Rail** — PexCraft exposes minecart items and rails but does not currently expose a player minecart-riding movement path suitable for the original one-kilometre start/end calculation.
- **Local Brewery** — PexCraft has potion items/effects and a brewing-stand block, but no brewing-container output path equivalent to `SlotBrewingStandPotion.onPickupFromSlot`.

They were not attached to unrelated surrogate actions because doing so would not be a 1:1 trigger.

Multiplayer statistics currently track local observable actions. They are not server-authoritative, and server-confirmed crafting/player-kill synchronization is outside this patch.

## Validation performed

- GCC unity-source syntax validation of `main_sdl2.c`: passed with return code 0.
- Python syntax validation of the PSP and Wii asset-preparation scripts: passed.
- No new warnings were identified in the two new achievement/statistics modules under an additional `-Wall -Wextra` syntax pass.
- Archive and patch integrity checks are performed during packaging.

A full linked runtime build was not possible in the execution environment because SDL2, SDL2_image, and OpenGL development packages are not installed. The existing warnings produced by the stubbed syntax build are unrelated pre-existing/stub warnings in texture, audio, RakNet, and legacy code.

## Suggested acceptance test

1. Delete or rename the active profile's `stats_<username>.dat`.
2. Select the downloaded Release pack and enter a world.
3. Confirm the top-right prompt uses the currently configured Inventory key.
4. Rebind Inventory and verify the prompt updates.
5. Open inventory and verify the prompt changes to `Achievement get! / Taking Inventory`.
6. Return to title, reopen another world, and verify Taking Inventory remains unlocked.
7. Change language and verify achievement titles, descriptions, buttons, and statistic labels update.
8. Select a custom texture pack containing `achievement/bg.png`, `gui/slot.png`, `terrain.png`, and `gui/items.png`; verify all achievement/statistics visuals use that pack.
9. Exercise crafting, furnace, mining, pickup, combat, portal, and End triggers.
10. Open Achievements and Statistics from the pause menu and verify dragging, scrolling, sorting, icons, prerequisites, and tooltips.
