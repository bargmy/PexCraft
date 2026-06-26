# PexCraft Xbox One UWP Dev Mode

This target is for Xbox One Developer Mode UWP testing.

The CI job now invokes MSBuild and requires a real `.appx`/`.msix` package. It no longer uploads a zipped source layout as a fake build.

Compatibility target:

- Target family: Windows.Universal
- MinVersion: 10.0.14393.0
- Architecture: x64
- Storage: `ApplicationData.Current.LocalFolder\\PexCraft`

The native shell owns CoreWindow and a D3D11 swapchain. The engine-side virtual keyboard remains the text-input path for Xbox builds because Xbox One UWP Dev Mode should not rely on a desktop IME.
