# Xbox/UWP User32 keyboard name link fix v10

The v9 boot patch correctly removed inherited desktop Win32 libraries from the UWP link. That exposed two shared-engine calls in `src/settings/options.c`:

- `MapVirtualKeyA`
- `GetKeyNameTextA`

Those are desktop/user32 keyboard helper calls used only to format key names in options UI. Adding `user32.lib` back would reintroduce desktop dependencies into the UWP package. Instead this patch redirects those calls, for the Xbox/UWP unity build only, to local compatibility helpers in `src/platform/xbox_uwp/xbox_uwp_compat.h`.

The previous compat header tried to define static functions with the same names after including `windows.h`, but MSVC had already seen `dllimport` declarations. v10 uses `pex_uwp_*` helper names and preprocessor redirects so `options.c` calls the UWP-safe helpers and the linker no longer needs `user32.lib`.

This does not mock the game. It only removes desktop key-name formatting. The real engine still starts through `pex_xbox_uwp_engine_init()` and frames through `pex_xbox_uwp_engine_frame()`.
