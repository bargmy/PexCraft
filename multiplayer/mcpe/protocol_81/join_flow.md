# Join Flow Placeholder

Planned join path for PexCraft -> Genisys:

1. Probe the RakNet MOTD. If it says `MCPE` with protocol `82` or version `0.15.4`, select this Bedrock/Genisys join path.
2. Open RakNet client connection to the Genisys UDP server.
3. Complete RakNet handshake through the DLL wrapper.
4. Send MCPE protocol 82 login packet from the `protocol_81` folder.
5. Read play status / login result.
6. Read start game data.
7. Begin chunk request / terrain receive path.
8. Start sending movement packets.

Implementation files can be added here later without changing the folder name.
