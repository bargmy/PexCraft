# Normal mob stability Java 1.2.5 port pass

Scope: normal mobs only. Ender Dragon / End-crystal systems were left unchanged in this pass.

## Ported in this pass

### Tameable ownership stability

- Wolf and ocelot babies created by breeding now inherit the single-player owner state from their parents.
- Tamed wolf babies are initialized with Java-style tamed health cap behavior.
- Tamed wolf/cat babies no longer come out as ownerless tame-looking mobs.

### Tamed wolf target behavior

- Added Java `EntityAIOwnerHurtTarget` / `EntityAIOwnerHurtByTarget` style behavior for the current single-player engine:
  - when the player hits a mob, nearby owned wolves target that mob;
  - when a mob attacks the player, nearby owned wolves defend the player;
  - wolf attack target has priority over follow-owner, matching Java task ordering.
- Owned wolves will not target owned wolves or owned cats.
- Owned wolves keep the tame texture while attacking; the angry texture remains for wild angry wolves.

### Java drop formula cleanup

- Normal mob drop counts now use the Java 1.2.5 Looting-aware formulas where applicable.
- The current `ItemStack` has no enchant/NBT storage, so `last_looting_level` is still normally zero, but the mob drop code now matches Java's formula when a Looting level is later wired in.
- Rare-drop chance now follows `rand.nextInt(200) - looting < 5`.
- Spider eye, blaze rod, ghast, squid, cow, mooshroom, pig zombie, skeleton, slime, and magma-cube drop formulas were brought into the same formula layer.

## Not changed here

- Ender Dragon and End crystal code.
- Full item enchant/NBT serialization.
- Rendering-only model animation refinements.
