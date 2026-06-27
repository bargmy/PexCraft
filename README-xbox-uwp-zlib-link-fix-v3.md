# Xbox UWP zlib link fix v3

The previous fix assumed vcpkg would always place `zlib.lib` at `C:\vcpkg\installed\x64-uwp\lib\zlib.lib`.
The CI log showed vcpkg reported success, but that exact file was not present.

This version changes `platforms/xbox_uwp/build_xbox_uwp.ps1` to discover the actual UWP zlib library after installation.
It searches both:

- `C:\vcpkg\installed\x64-uwp\lib`
- `C:\vcpkg\packages\zlib_x64-uwp\lib`

It then passes the exact discovered `.lib` path and include directory into MSBuild as `ZlibLibrary`, `ZlibLibraryDir`, and `ZlibIncludeDir`.

This avoids linking against a hardcoded bare `zlib.lib` when vcpkg stores the usable library in the package staging directory.
