# transport

Placeholder RakNet wrapper integration layer.

This should adapt `raknet_c.h` / RakNetDLL into a small PexCraft-facing API.

## Normalized RakNetDLL names

The workflow downloads upstream assets such as `libRakNetDLL-linux-x64.so` and
`RakNetDLL-windows-x64.dll`, verifies SHA256, then stores them as:

```text
linux-*/libraknet.so
windows-*/raknet.dll
```

Game/runtime code should only look for those normalized names.
