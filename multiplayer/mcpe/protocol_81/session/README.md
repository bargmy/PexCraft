# session

Placeholder join state machine for connecting PexCraft to a Genisys server.

## MOTD detection route

For now, PexCraft probes the server MOTD before the old PXNet TCP connect. If the MOTD looks like `MCPE;...;82;0.15.4;...`, it selects this protocol_81 Bedrock/Genisys join path. The actual RakNet login is still the next step.
