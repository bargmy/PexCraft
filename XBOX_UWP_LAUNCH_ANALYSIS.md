# Xbox One UWP launch analysis

## Most likely reasons the package installs/builds but immediately exits

1. **The manual MakeAppx fallback omits native runtime DLLs.** The build links a vcpkg zlib `.lib`, but the fallback originally copied only `PexCraft.UWP.exe` and `Assets`. If that `.lib` is an import library, Windows terminates the process in the loader before `wWinMain` because the corresponding zlib DLL is absent.
2. **The fallback manifest identifies native x64 code as architecture-neutral.** The source manifest had no `ProcessorArchitecture="x64"` even though the project produces only an x64 executable.
3. **The fallback package did not generate `resources.pri`.** Visual Studio normally generates this resource index. Omitting it makes the hand-built package materially different from the MSBuild package and can break resource/visual activation.
4. **Renderer initialization is fail-closed.** Any D3D11 device, CoreWindow swap-chain, render-target/depth-view, or shader failure returns false. `App::StartRealEngine` then sets `m_windowClosed = true`, so the app appears to “not launch.”
5. **Shaders are compiled at runtime with `D3DCompile`.** UWP deployment guidance expects precompiled `.cso` shader files. Runtime compilation is a deployment/certification risk and is a plausible Xbox-only renderer failure.
6. **The useful log was written to the package installation directory.** `fopen("log.txt", "a")` usually fails in an installed UWP package because that location is read-only, hiding the renderer error.
7. **The package finder could select a stale unrelated package.** It searched all of `build` and took the newest app package without validating identity or output path.

## Changes in this patched tree

- Adds x64 architecture to the manifest.
- Enables vcpkg app-local dependency deployment.
- Copies vcpkg runtime DLLs into the manual package fallback.
- Generates `resources.pri` before MakeAppx packaging.
- Deletes stale app packages before building, then restricts discovery to `build\xbox_uwp` and PexCraft-named packages.
- Adds a package validator that checks architecture, executable/entry point tokens, package contents, and missing zlib DLL imports.
- Writes engine logs to `ApplicationData.Current.LocalFolder\PexCraft\log.txt`.
- Writes lifecycle/fatal exception logs to `ApplicationData.Current.LocalFolder\PexCraft\uwp_boot.log`.
- Logs D3D11 HRESULTs and shader compiler errors instead of silently closing.

## Recommended test order

1. Build and deploy the Visual Studio/MSBuild-generated package, not the MakeAppx fallback.
2. Run `validate_xbox_uwp_package.ps1 -PackagePath <package.appx>` before deployment.
3. Launch once, then use Xbox Device Portal File Explorer to retrieve `LocalState\PexCraft\uwp_boot.log` and `LocalState\PexCraft\log.txt`.
4. If the boot log never contains `wWinMain`, inspect the package loader/dependencies and download the crash dump from Device Portal.
5. If it reaches `starting real PexCraft engine` but not `real engine init OK`, use the logged D3D HRESULT to fix the exact renderer stage.
6. For a production-quality fix, move the inline HLSL into `.hlsl` files, let Visual Studio build `.cso` files, package them, and create shaders from bytecode instead of calling `D3DCompile` at runtime.

## Alternative approaches

- **Static zlib:** use a custom UWP vcpkg triplet with `VCPKG_LIBRARY_LINKAGE static`, then link the resulting static library. This removes the zlib DLL loader failure entirely.
- **No manual package path:** delete the MakeAppx fallback and fail CI unless MSBuild emits the real AppPackages output. This is the least ambiguous deployment route.
- **Loose registration on Windows first:** register the generated package layout with `Add-AppxPackage -Register AppxManifest.xml` on a Windows test machine. This separates packaging/activation defects from Xbox renderer defects.
- **Minimal renderer canary:** temporarily skip texture loading and draw only a clear color after swap-chain creation. If that launches, re-enable pipeline creation, shaders, and assets one stage at a time.

## CI compile failure found after the launch patch

The July 12 GitHub Actions run stopped before packaging with 16 compiler errors.
The primary syntax cascade came from the Windows SDK RPC macro `small`, which
rewrote the Java protocol adapter's local declarations. Those locals are now
called `is_small`, and the Xbox compatibility header also undefines the SDK macro.

The same run exposed missing Xbox GL-compatibility tokens (`GL_EXP2` and
`GL_POLYGON_OFFSET_FILL`) plus undeclared calls for normals, polygon offset, and
texture sub-image updates. The constants and safe no-op compatibility functions
are now present. `glTexSubImage2D` is routed to a real D3D11
`UpdateSubresource` implementation, and D3D11 textures use `D3D11_USAGE_DEFAULT`
so animated atlas regions can be updated after creation.
