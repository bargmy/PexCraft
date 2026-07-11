# protocol_47je

Minecraft Java Edition 1.8.8 (`protocol 47`) adapter for PexCraft.

## Detection order

For every multiplayer address, PexCraft probes Java status first. If Java status does not answer, it sends the existing RakNet unconnected ping and treats a protocol 81/82 or `0.15.x` response as Bedrock 0.15. With no explicit port, Java uses `25565` and Bedrock uses `19132`; an explicit port is tested with both transports.

The server list displays the detected family/version, MOTD, player count, and measured ping.

## Java support

- Java status handshake, JSON response, and ping/pong latency
- Offline-mode login only
- Explicit rejection of online-mode encryption requests
- VarInt packet framing and fragmented/coalesced TCP packet handling
- Login and play compression with zlib
- Join Game, keep-alive, position correction, respawn, health, hunger, XP, time, and game-mode updates
- Chunk Data, Bulk Chunk Data, single and multi-block changes
- Movement, chat, digging, placement/use, swing, attacking, sprinting, sneaking, held-slot changes, item dropping, and respawn
- Remote players, living mobs, dropped items, movement, equipment, metadata, and removal
- Armor-stand spawn objects represented by targetable pig proxies, with mapped helmet/hand items shown as stable display items
- Right-click entity interaction for armor stands, villagers, living mobs, and fake-player NPCs
- Player inventory, server-opened generic containers, chest, crafting-table and furnace windows, clicks, creative slots, cursor updates, and transaction acknowledgements
- Multiplayer-only world-window remapping without local terrain generation

## Compatibility translation

The game remains a 1.2.5 gameplay client. Java 1.8.8 content that PexCraft does not implement is translated rather than added to gameplay:

- 1.8-only blocks are mapped to the closest 1.2.5 shape/material (new stairs to old stairs, new fences to old fences, stained clay to colored wool, ender chests to chests, and so on)
- 1.8-only items are mapped to the closest usable 1.2.5 icon (emerald to diamond, rabbit to chicken, banners/armor stands to signs, and so on)
- the original server item ID, damage, count, and bounded raw NBT are retained for clicks and use packets, so a substitute icon does not become the wrong network item
- unsupported living entity types -> pig
- armor stands -> pig proxy plus the closest mapped equipped-item display
- completely unknown blocks fall back to stone; completely unknown standalone items fall back to paper

Chat messages are wrapped to the existing HUD width and continue on additional chat lines rather than clipping off-screen.

Online authentication, encryption, skins/profile textures, resource-pack installation, and newer 1.8-only gameplay systems are intentionally not implemented. Optional presentation-only 1.8 packets that have no 1.2.5 equivalent are safely ignored.

DNS `_minecraft._tcp` SRV discovery is not implemented; use a directly resolvable host or an explicit `host:port`. The adapter targets ordinary vanilla-compatible offline-mode protocol-47 servers. A plugin that requires a custom client channel or mod handshake may reject the connection.
