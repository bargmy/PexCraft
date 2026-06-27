# Xbox/UWP activation fix v4

The uploaded Windows logs did not show a normal crash inside `PexCraft.UWP.exe`.
`Application` crash logs were empty and `Microsoft-Windows-TWinUI/Operational`
reported:

- `Activation ... !App failed`
- `The app didn't start`
- `Activation phase: COM ActivateExtension`

The installed package was also reported as a loose development-mode package with
`Architecture : Neutral`, `SignatureKind : None`, and an install path under the
user's Downloads folder. That points to activation/package-layout failure before
normal game code runs.

This fix changes the UWP packaging path so the appx is made from a controlled
layout:

- Source manifest now uses literal `Executable="PexCraft.UWP.exe"` and
  `EntryPoint="PexCraftUwp.App"` instead of relying on source-layout manifest
  placeholders.
- Source manifest now declares `ProcessorArchitecture="x64"`.
- `build_xbox_uwp.ps1` now always creates its own MakeAppx layout after MSBuild.
- The layout copies app-local UWP VC runtime DLLs when found on the builder
  (`vcruntime140_app.dll`, `vcruntime140_1_app.dll`, `msvcp140_app.dll`,
  `concrt140_app.dll`).
- The app now writes an early activation diagnostic log named
  `pexcraft-uwp-activation.log` in the app LocalFolder if the process starts and
  throws before the engine is ready.

If reinstall fails with `0x80073D02`, close/terminate the existing PexCraft app
first, then install again.
