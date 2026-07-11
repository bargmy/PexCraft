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
- Player inventory, chest, crafting-table and furnace windows, clicks, creative slots, cursor updates, and transaction acknowledgements
- Multiplayer-only world-window remapping without local terrain generation

## Compatibility translation

The game remains a 1.2.5 gameplay client. Java 1.8.8 content that PexCraft does not implement is translated rather than added to gameplay:

- unsupported block IDs -> stone
- unsupported item IDs -> stone
- unsupported living entity types -> pig
- item NBT is validated/skipped; supported base item ID, count, and damage are retained

Online authentication, encryption, skins/profile textures, resource-pack installation, and newer 1.8-only gameplay systems are intentionally not implemented. Optional presentation-only 1.8 packets that have no 1.2.5 equivalent are safely ignored.

DNS `_minecraft._tcp` SRV discovery is not implemented; use a directly resolvable host or an explicit `host:port`. The adapter targets ordinary vanilla-compatible offline-mode protocol-47 servers. A plugin that requires a custom client channel or mod handshake may reject the connection.
