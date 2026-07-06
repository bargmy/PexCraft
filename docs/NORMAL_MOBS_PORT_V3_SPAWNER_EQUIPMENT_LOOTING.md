# Normal Mobs Port v3: Spawner Tile Data, Equipment, Looting

Scope: normal mobs only.  Dragon/End systems were intentionally left untouched.

## Implemented

### TileEntityMobSpawner-style data

The spawner runtime now persists Java 1.2.5 TileEntityMobSpawner data instead of only re-deriving behavior from nearby blocks every tick:

- EntityId mapped to a PassiveMob type
- Delay
- SpawnCount
- MaxNearbyEntities
- RequiredPlayerRange
- SpawnRange

Older worlds still get a one-time context migration for spawners that were created before the runtime stored EntityId-like data.

### ItemStack enchantment-NBT bridge

ItemStack now has a compact C representation of Java's `ench` list as fixed id/lvl pairs.  World save version 29 preserves these fields for player inventory, armor inventory, chests, furnaces, and dropped item entities.

Looting uses Java's id:

```text
Enchantment.looting.effectId = 21
```

When the player hits a mob, the mob caches the player's held ItemStack Looting level for the Java `recentlyHit` death window.

### 1.2.5 equipment correction

Java 1.2.5 normal mobs do not have the later random armor/equipment spawn system.  The correct 1.2.5 port behavior is:

- Skeleton `getHeldItem()` = bow
- Zombie Pigman `getHeldItem()` = golden sword
- Zombie rare drops can include iron sword/helmet/ingot/shovel
- Skeleton rare bow can be enchanted on the enchanted branch
- Zombie Pigman rare golden sword can be enchanted on the enchanted branch

The enchanted rare-drop branch now produces ItemStacks with real enchant id/lvl storage.

## Still intentionally not in this pass

- Full enchantment table generation for every possible enchantment combination
- Visual enchantment glint
- Later-version random mob armor spawning, because that is not Java 1.2.5 behavior
