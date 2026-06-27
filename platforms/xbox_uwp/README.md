# PexCraft Xbox One UWP Dev Mode

This target is no longer a fake clear-screen shell.  The UWP CoreWindow shell in
`platforms/xbox_uwp/Source/App.cpp` now calls the real C engine in
`src/main_xbox_uwp.c`.

Important pieces:

- `src/render/renderer_d3d11_xbox.c`
  - creates a UWP `CreateSwapChainForCoreWindow` Direct3D 11 swapchain
  - attaches it to PexCraft's existing D3D11 compatibility renderer
- `src/main_xbox_uwp.c`
  - includes the real PexCraft engine unity build
  - uses UWP LocalFolder storage
  - drives the real tick/render loop from CoreWindow
- `src/platform/xbox_uwp/xbox_uwp_input.c`
  - controller/key state bridge
  - no desktop mouse grab APIs
- `build_xbox_uwp.ps1`
  - builds with MSBuild
  - packages a real `.appx`
  - installs `zlib:x64-uwp` through vcpkg when available

The artifact should show the actual PexCraft title/resource-download UI, not a
blank D3D clear screen.
