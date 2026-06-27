# Xbox UWP zlib link fix

The UWP project now adds the vcpkg UWP library directory to the linker search path:

```xml
<AdditionalLibraryDirectories>$(VcpkgInstalledDir)$(VcpkgTriplet)\lib;$(VcpkgRoot)\installed\$(VcpkgTriplet)\lib;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
```

This fixes the CI/MSBuild link failure:

```text
LINK : fatal error LNK1181: cannot open input file 'zlib.lib'
```

The project already installs `zlib:x64-uwp` through `platforms/xbox_uwp/build_xbox_uwp.ps1`; the missing part was passing the installed `lib` directory to the linker.
