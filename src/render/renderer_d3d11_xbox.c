/* Xbox One / UWP CoreWindow Direct3D 11 swapchain bridge.
   Included after backend_compat.c so it can initialize that file's immediate-mode
   D3D11 compatibility renderer (g_d3d11) with CreateSwapChainForCoreWindow. */

#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <unknwn.h>

static int pex_renderer_d3d11_xbox_init_corewindow(void *core_window_unknown, int width, int height) {
    IUnknown *core_window = (IUnknown *)core_window_unknown;
    if (!core_window) return 0;
    if (width < 1) width = 1280;
    if (height < 1) height = 720;

    memset(&g_d3d11, 0, sizeof(g_d3d11));
    g_d3d11.width = width;
    g_d3d11.height = height;
    g_d3d11.buffer_count = 2;
    g_d3d11.allow_tearing = 0;
    g_d3d11.swap_flags = 0;
    g_d3d11.frame_latency = 1;

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    D3D_DRIVER_TYPE drivers[] = { D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP };
    HRESULT hr = E_FAIL;
    D3D_FEATURE_LEVEL got = D3D_FEATURE_LEVEL_11_0;
    for (int d = 0; d < (int)ARRAY_COUNT(drivers); ++d) {
        hr = D3D11CreateDevice(NULL, drivers[d], NULL, flags, levels, (UINT)ARRAY_COUNT(levels),
                               D3D11_SDK_VERSION, &g_d3d11.dev, &got, &g_d3d11.ctx);
        if (SUCCEEDED(hr) && g_d3d11.dev && g_d3d11.ctx) break;
        compat_d3d11_release_failed_device(NULL, &g_d3d11.dev, &g_d3d11.ctx);
    }
    if (FAILED(hr) || !g_d3d11.dev || !g_d3d11.ctx) return 0;

    IDXGIDevice1 *dxgi_dev = NULL;
    IDXGIAdapter *adapter = NULL;
    IDXGIFactory2 *factory = NULL;
    IDXGISwapChain1 *swap1 = NULL;

    hr = ID3D11Device_QueryInterface(g_d3d11.dev, &IID_IDXGIDevice1, (void **)&dxgi_dev);
    if (SUCCEEDED(hr) && dxgi_dev) hr = IDXGIDevice1_GetAdapter(dxgi_dev, &adapter);
    if (SUCCEEDED(hr) && adapter) hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory2, (void **)&factory);
    if (SUCCEEDED(hr) && factory) {
        DXGI_SWAP_CHAIN_DESC1 desc;
        memset(&desc, 0, sizeof(desc));
        desc.Width = (UINT)width;
        desc.Height = (UINT)height;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.Stereo = FALSE;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        desc.Flags = 0;
        hr = IDXGIFactory2_CreateSwapChainForCoreWindow(factory,
                (IUnknown *)g_d3d11.dev, core_window, &desc, NULL, &swap1);
    }
    if (SUCCEEDED(hr) && swap1) {
        hr = IDXGISwapChain1_QueryInterface(swap1, &IID_IDXGISwapChain, (void **)&g_d3d11.swap);
    }
    if (swap1) IDXGISwapChain1_Release(swap1);
    if (factory) IDXGIFactory2_Release(factory);
    if (adapter) IDXGIAdapter_Release(adapter);
    if (dxgi_dev) IDXGIDevice1_Release(dxgi_dev);

    if (FAILED(hr) || !g_d3d11.swap) {
        compat_d3d11_release_failed_device(&g_d3d11.swap, &g_d3d11.dev, &g_d3d11.ctx);
        return 0;
    }

    compat_d3d11_set_latency_for_unlimited_fps();
    if (!pex_d3d11_create_views(width, height)) return 0;
    if (!pex_d3d11_create_pipeline()) return 0;
    ID3D11DeviceContext_IASetInputLayout(g_d3d11.ctx, g_d3d11.layout);
    ID3D11DeviceContext_VSSetShader(g_d3d11.ctx, g_d3d11.vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(g_d3d11.ctx, g_d3d11.ps, NULL, 0);
    ID3D11DeviceContext_RSSetState(g_d3d11.ctx, g_d3d11.rast_nocull);
    g_d3d11.active = 1;
    return 1;
}
