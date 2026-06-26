# PexCraft MCPE protocol_81 multiplayer

This folder contains the Genisys/RakLib join path for MCPE 0.15.4-era servers.
The folder name stays `protocol_81`, but the implementation accepts the server MOTD protocol and sends `82` when the MOTD reports protocol `82`.

Implemented pieces:

- RakNet unconnected MOTD detection in PexCraft's normal multiplayer connect path.
- Runtime RakNetDLL loading from normalized names:
  - Linux: `libraknet.so`
  - Windows: `raknet.dll`
- RakNet client wrapper using `raknet_c.h`.
- Genisys LoginPacket with offline JWT chain, compressed exactly how Genisys decodes it.
- PlayStatus, StartGame, Batch, RequestChunkRadius, ChunkRadiusUpdated, FullChunkData, Text, MovePlayer, PlayerAction, UseItem, and UpdateBlock handling.
- Conversion of old 128-high MCPE chunk payloads into PexCraft's 256-high flat chunk storage, with Y 128-255 filled as air.

Build/runtime note:

Run `python3 multiplayer/mcpe/protocol_81/workflow/fetch_raknetdll.py` before launching a build that needs to connect to Genisys. The script downloads the upstream architecture-specific libraries and stores them as the normalized runtime names under `transport/bin/<platform>/`.

Limits of this pass:

- The code is implemented against the uploaded Genisys packet formats, but the live RakNetDLL/Genisys handshake still needs real-server testing.
- Inventory/entity support is minimal. Joining, terrain, movement, chat, block break, block place/use, and block updates are the first target.
