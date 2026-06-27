# Xbox UWP zlib link fix v2

The uploaded CI log still failed at the link step with:

```text
LINK : fatal error LNK1181: cannot open input file 'zlib.lib'
```

This version makes zlib resolution explicit instead of relying only on inherited MSBuild/vcpkg integration properties:

- `platforms/xbox_uwp/build_xbox_uwp.ps1` now verifies `C:\vcpkg\installed\x64-uwp\lib\zlib.lib` exists after `vcpkg install zlib:x64-uwp`.
- It prepends the zlib lib/include directories to `LIB` and `INCLUDE`.
- It passes `VcpkgRoot`, `VcpkgInstalledDir`, `ZlibLibraryDir`, and `ZlibLibrary` to MSBuild with `/p:` arguments.
- `platforms/xbox_uwp/PexCraft.UWP.vcxproj` now links `$(ZlibLibrary)` directly, so the link line no longer depends on finding a bare `zlib.lib`.
