# Normalized RakNetDLL binaries

This folder is populated by:

```sh
python3 multiplayer/mcpe/protocol_81/workflow/fetch_raknetdll.py
```

Expected normalized runtime names:

```text
linux-*/libraknet.so
windows-*/raknet.dll
```

Do not load the upstream names directly from game code. The workflow renames
them so the transport loader has one predictable filename per OS.
