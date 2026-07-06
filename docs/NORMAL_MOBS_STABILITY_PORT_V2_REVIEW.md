# Normal mobs stability port v2

Scope: normal mobs only. Dragon/End systems were intentionally not changed in this pass.

## Ported in this pass

### Java-style held equipment state

Skeleton bows and zombie pigman golden swords are now stored as mob state (`held_item`) instead of being renderer-only hardcodes. This mirrors the Java 1.2.5 `getHeldItem` behavior in the C runtime, so AI, rendering, save/load, and future equipment drops use one shared source of truth.

Saved mob format moved to version 33 and appends:

- `recently_hit`
- `last_looting_level`
- `held_item`
- four armor/equipment ids reserved for normal mob equipment parity

Older saves still get default skeleton/pig-zombie held items on load.

### Tameable interaction stability

Owned wolves/cats now toggle sitting after item-specific interaction handling, not only when the player has an empty hand. This is closer to Java `EntityWolf` / `EntityOcelot` owner interaction: healing/breeding is handled first, otherwise owner interaction toggles sitting and clears navigation/attack target.

### Fireball impact behavior

Large fireballs now perform a Java 1.2.5-style explosion pass on impact instead of only playing particles. Direct large-fireball impact no longer double-applies direct projectile damage before the explosion. Small fireballs now attempt to place fire at/above the impact cell, matching the intended normal-mob Blaze fireball behavior more closely.

## Intentionally not touched

- Ender Dragon
- End crystals
- End portal/egg code
- End-only rendering/path logic

## Still dependent on engine systems

Normal mob parity still depends on future engine-level work for true ItemStack enchant/NBT, enchanted equipment metadata, and true tile-entity mob spawner data in chunk storage.
