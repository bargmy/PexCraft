# PexCraft Xbox One UWP Dev Mode target

This target is for Xbox One Developer Mode testing. It uses the existing PexCraft
Direct3D 11 renderer path and keeps all mutable game data in the app's LocalState
folder so assets, options, skins, servers.txt, and worlds are writable on Xbox.

Manifest version policy:

- `TargetDeviceFamily Name="Windows.Universal"`
- `MinVersion="10.0.14393.0"`
- `MaxVersionTested="10.0.19041.0"`

`10.0.14393.0` is used because it is the oldest practical UWP/Xbox One OS line
for Dev Mode-era apps. Do not use new-only APIs in this target without a runtime
check.

## Virtual keyboard

The engine has an in-game controller keyboard enabled by `PEX_PLATFORM_XBOX_UWP`.
It is used instead of relying on a system IME. Press OK/A on a text field to open
it. D-pad moves, OK/A selects, Back/B erases, and the on-screen Done button
commits.

## Assets and saves

The UWP target must use `ApplicationData.Current.LocalFolder` for writable files.
The engine path helper `pex_xbox_uwp_get_local_folder()` feeds `init_dirs()` so:

- options.txt
- servers.txt
- skins
- texturepacks / downloaded Release resources
- saves

all live under LocalState/PexCraft.

## GitHub Actions

`.github/workflows/build.yml` has an `xbox_uwp` job. The job creates an Xbox UWP
Dev Mode package layout artifact and uploads it with the other successful builds;
when Visual Studio UWP packaging is available it also tries MSBuild packaging.
