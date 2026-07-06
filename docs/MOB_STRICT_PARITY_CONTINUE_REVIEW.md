# Mobs Java 1.2.5 strict parity continuation pass

This pass continues from `PexCraft-mobs-full-continue-pass.zip` and keeps the scope to mobs.

## Added / changed

### Tameable mobs
- Wolf max health now follows Java 1.2.5 behavior: 8 health while wild/angry, 20 after taming.
- Taming a wolf now heals it to 20 health.
- Wolf attack damage now follows Java behavior: 2 while wild, 4 while tamed.
- Owned wolves and cats now try to follow the player and teleport near the player when far away, matching the practical behavior of `EntityAIFollowOwner`.
- Owned wolves/cats now sit after successful taming.
- Ocelot taming now requires the player to be close enough before the fish is consumed/taming is attempted.
- Wolf and cat living sounds are now state-dependent: growl/whine/pant/bark for wolves, and meow/purreow/purr for tamed cats.
- Wolf and ocelot sound volume is reduced to Java-like 0.4.

### Pig Zombies
- Pig zombies now use a Java-like anger timer instead of staying permanently angry.
- Player attacks alert nearby pig zombies in a 32 block range.
- Angry pig zombies now use the angry sound and faster movement.
- The old attack cooldown guard no longer blocks angry pig zombies from attacking while their anger timer is active.

### Zombies and wooden doors
- Zombies now look for nearby closed wooden doors.
- On hard difficulty, they can break the door after a Java-like 240 tick break timer.
- Door breaking is intentionally limited to wooden doors.

### AI priority cleanup
- Follow-owner behavior now has priority over normal breeding/tempt behavior for owned tameable mobs, closer to Java's AI task ordering.

### Health/save clamping
- Health clamping now uses dynamic wolf max health, so loaded wild wolves do not retain tamed health values and loaded tamed wolves can keep 20 health.

## Still not 1:1

This pass moves more behavior into Java 1.2.5 shape, but the whole mob system is not byte-for-byte or system-for-system 1:1 yet. The remaining differences require engine-level systems rather than simple table/function fixes:

- Full Java `EntityAI` task list scheduler and mutex bits.
- Full Java `PathNavigate`, `MoveHelper`, `LookHelper`, `Senses`, and `EntityLookHelper` behavior.
- Full NBT-compatible per-mob state saving.
- Exact village/door/reputation/golem systems.
- Exact mob spawner block logic.
- Nether fortress mob spawning for Blaze and Wither Skeleton-style fortress rules where applicable to the target version.
- Cave spider spawner and silverfish block behavior.
- Exact Looting/equipment/armor drop behavior.
- Exact projectile/explosion physics, line-of-sight edge cases, and collision edge cases.
- Ender Dragon boss system.
