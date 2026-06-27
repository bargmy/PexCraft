# PexCraft Xbox UWP real boot fix v9

This patch is intended to boot the real engine, not a placeholder shell.

Changes:
- Starts the real PexCraft C engine after the UWP Activated event and CoreWindow activation, matching Microsoft DirectX/UWP lifecycle order.
- Removes the earlier blank-shell fallback behavior: if the real engine init fails, it logs and exits instead of pretending boot succeeded.
- Removes manual `winrt::init_apartment()` from `wWinMain` to match Microsoft Core App C++/WinRT examples.
- Keeps manifest tokens so MSBuild generates the app entry point.
- Forces `/APPCONTAINER` and static CRT, avoiding a missing VCLibs dependency during sideload/dev-mode tests.
- Stops inheriting desktop Win32 default libraries in the linker line.
- Keeps zlib linked by discovering the actual vcpkg UWP .lib path.

After build, check the MSBuild link line contains `/APPCONTAINER` and does not contain `user32.lib`, `gdi32.lib`, `shell32.lib`, `comdlg32.lib`, `odbc32.lib`.
