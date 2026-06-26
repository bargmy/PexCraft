# RakNetDLL workflow assets

Run this from the repository root:

```sh
python3 multiplayer/mcpe/protocol_81/workflow/fetch_raknetdll.py
```

The script downloads the upstream RakNetDLL release assets, verifies SHA256, and
renames them into stable loader names:

```text
multiplayer/mcpe/protocol_81/transport/bin/linux-x64/libraknet.so
multiplayer/mcpe/protocol_81/transport/bin/linux-x86/libraknet.so
multiplayer/mcpe/protocol_81/transport/bin/linux-arm64/libraknet.so
multiplayer/mcpe/protocol_81/transport/bin/windows-x64/raknet.dll
multiplayer/mcpe/protocol_81/transport/bin/windows-x86/raknet.dll
multiplayer/mcpe/protocol_81/transport/bin/windows-arm64/raknet.dll
```

The original asset names stay only in the script/manifest. Runtime code should
load `libraknet.so` on Linux and `raknet.dll` on Windows from the selected
architecture folder.
