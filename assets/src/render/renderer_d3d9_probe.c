/* Tiny Direct3D 9 runtime probe.
   This is intentionally not a renderer; it only checks whether d3d9.dll and a
   HAL device are present, mirroring the first part of the D3D9 setup in the
   uploaded O3D reference.  The actual game renderer still needs a separate
   D3D9 draw backend before RENDERER_D3D9 can be marked supported. */

#ifdef _WIN32
#include <d3d9.h>
#endif

#ifndef D3D_SDK_VERSION
#define D3D_SDK_VERSION 32
#endif

static int d3d9_runtime_available(void) {
#ifdef _WIN32
    HMODULE dll = LoadLibraryA("d3d9.dll");
    if (!dll) return 0;
    typedef IDirect3D9 *(WINAPI *Direct3DCreate9Fn)(UINT);
    Direct3DCreate9Fn create9 = (Direct3DCreate9Fn)GetProcAddress(dll, "Direct3DCreate9");
    if (!create9) { FreeLibrary(dll); return 0; }
    IDirect3D9 *d3d = create9(D3D_SDK_VERSION);
    if (!d3d) { FreeLibrary(dll); return 0; }
    D3DCAPS9 caps;
    HRESULT hr = IDirect3D9_GetDeviceCaps(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
    IDirect3D9_Release(d3d);
    FreeLibrary(dll);
    return SUCCEEDED(hr);
#else
    return 0;
#endif
}
