# Xbox/UWP activation fix v5

This build changes the UWP packaging/runtime strategy after repeated `COM ActivateExtension` launch failures.

Changes:

- Links the Release UWP build with the static runtime (`RuntimeLibrary=MultiThreaded`) to avoid a missing/mismatched Microsoft.VCLibs runtime causing activation to fail before the game starts.
- Changes the manifest entry point string to `PexCraftUWP.App`, matching the project `RootNamespace` casing used by Visual Studio templates.
- Stops using the hand-built MakeAppx package as the primary artifact. The script now prefers the MSBuild-generated UWP package, because MSBuild generates the final `AppxManifest.xml` and UWP metadata expected by Windows activation.
- Keeps the controlled MakeAppx layout only as a fallback if MSBuild fails to emit a package.
- Still signs the selected final package with a temporary CI test certificate when possible.

Install notes:

1. Remove old installs first:
   `Get-AppxPackage "*PexCraft*" | Remove-AppxPackage`
2. Install the new appx from `dist`.
3. If it still fails, run the one-line crash-log command again after launching it.
