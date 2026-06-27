// Xbox One / UWP Dev Mode native entry point.
// Uses C++/WinRT instead of C++/CX so the CI build does not need /ZW or platform.winmd.
// The full PexCraft engine can be attached behind this shell; this file owns the
// CoreWindow loop, D3D11 swapchain, app-local storage path, and basic controller-safe input hooks.

#include <windows.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <winrt/base.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Core.h>

#include <stdint.h>
#include <stdio.h>
#include <string>

using Microsoft::WRL::ComPtr;

namespace winrt_app = winrt::Windows::ApplicationModel;
namespace activation = winrt::Windows::ApplicationModel::Activation;
namespace coreapp = winrt::Windows::ApplicationModel::Core;
namespace foundation = winrt::Windows::Foundation;
namespace storage = winrt::Windows::Storage;
namespace system = winrt::Windows::System;
namespace coreui = winrt::Windows::UI::Core;

static std::wstring g_local_folder;

extern "C" const wchar_t *pex_xbox_uwp_get_local_folder_w(void) {
    if (g_local_folder.empty()) {
        auto folder = storage::ApplicationData::Current().LocalFolder();
        g_local_folder.assign(folder.Path().c_str());
        g_local_folder.append(L"\\PexCraft");
        CreateDirectoryW(g_local_folder.c_str(), nullptr);
    }
    return g_local_folder.c_str();
}

extern "C" const char *pex_xbox_uwp_get_local_folder(void) {
    static std::string cached;
    const wchar_t *wide = pex_xbox_uwp_get_local_folder_w();
    int need = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (need > 0) {
        cached.assign((size_t)need, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, &cached[0], need, nullptr, nullptr);
        if (!cached.empty() && cached.back() == '\0') cached.pop_back();
    }
    return cached.c_str();
}

namespace PexCraftUwp {

struct App : winrt::implements<App, coreapp::IFrameworkViewSource, coreapp::IFrameworkView> {
    App() = default;

    coreapp::IFrameworkView CreateView() {
        return *this;
    }

    void Initialize(coreapp::CoreApplicationView const& applicationView) {
        applicationView.Activated({ this, &App::OnActivated });
        coreapp::CoreApplication::Suspending({ this, &App::OnSuspending });
        coreapp::CoreApplication::Resuming({ this, &App::OnResuming });
        pex_xbox_uwp_get_local_folder_w();
    }

    void SetWindow(coreui::CoreWindow const& window) {
        m_window = window;
        m_window.Closed({ this, &App::OnWindowClosed });
        m_window.VisibilityChanged({ this, &App::OnVisibilityChanged });
        m_window.SizeChanged({ this, &App::OnSizeChanged });
        m_window.KeyDown({ this, &App::OnKeyDown });
        m_window.KeyUp({ this, &App::OnKeyUp });
        m_window.CharacterReceived({ this, &App::OnCharacterReceived });
        CreateDeviceResources();
        CreateWindowSizeDependentResources();
    }

    void Load(winrt::hstring const&) {}

    void Run() {
        while (!m_windowClosed) {
            if (m_windowVisible) {
                m_window.Dispatcher().ProcessEvents(coreui::CoreProcessEventsOption::ProcessAllIfPresent);
                if (m_needResize) CreateWindowSizeDependentResources();
                Render();
            } else {
                m_window.Dispatcher().ProcessEvents(coreui::CoreProcessEventsOption::ProcessOneAndAllPending);
            }
        }
    }

    void Uninitialize() {}

private:
    coreui::CoreWindow m_window{ nullptr };
    bool m_windowClosed = false;
    bool m_windowVisible = true;
    bool m_needResize = true;

    ComPtr<ID3D11Device1> m_device;
    ComPtr<ID3D11DeviceContext1> m_context;
    ComPtr<IDXGISwapChain1> m_swapChain;
    ComPtr<ID3D11RenderTargetView> m_renderTargetView;

    void OnActivated(coreapp::CoreApplicationView const&, activation::IActivatedEventArgs const&) {
        if (m_window) m_window.Activate();
    }

    void OnSuspending(winrt::Windows::Foundation::IInspectable const&, winrt_app::SuspendingEventArgs const&) {}
    void OnResuming(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::Foundation::IInspectable const&) {}
    void OnWindowClosed(coreui::CoreWindow const&, coreui::CoreWindowEventArgs const&) { m_windowClosed = true; }
    void OnVisibilityChanged(coreui::CoreWindow const&, coreui::VisibilityChangedEventArgs const& args) { m_windowVisible = args.Visible(); }
    void OnSizeChanged(coreui::CoreWindow const&, coreui::WindowSizeChangedEventArgs const&) { m_needResize = true; }

    void OnKeyDown(coreui::CoreWindow const&, coreui::KeyEventArgs const& args) {
        // Engine binding is intentionally small here; controller-friendly UI paths
        // consume mapped keys inside the shared PexCraft code once the full engine
        // is attached to the UWP shell.
        if (args.VirtualKey() == system::VirtualKey::Escape ||
            args.VirtualKey() == system::VirtualKey::GamepadB) {
            args.Handled(true);
        }
    }

    void OnKeyUp(coreui::CoreWindow const&, coreui::KeyEventArgs const&) {}
    void OnCharacterReceived(coreui::CoreWindow const&, coreui::CharacterReceivedEventArgs const&) {}

    void CreateDeviceResources() {
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
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;
        D3D_FEATURE_LEVEL selected = D3D_FEATURE_LEVEL_11_0;
        winrt::check_hresult(D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            flags,
            levels,
            ARRAYSIZE(levels),
            D3D11_SDK_VERSION,
            &device,
            &selected,
            &context));
        winrt::check_hresult(device.As(&m_device));
        winrt::check_hresult(context.As(&m_context));
    }

    void CreateWindowSizeDependentResources() {
        if (!m_window || !m_device) return;
        m_context->OMSetRenderTargets(0, nullptr, nullptr);
        m_renderTargetView.Reset();

        auto bounds = m_window.Bounds();
        UINT width = (UINT)(bounds.Width > 1.0f ? bounds.Width : 1280.0f);
        UINT height = (UINT)(bounds.Height > 1.0f ? bounds.Height : 720.0f);

        if (m_swapChain) {
            HRESULT hr = m_swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
                m_swapChain.Reset();
                CreateDeviceResources();
            } else {
                winrt::check_hresult(hr);
            }
        }

        if (!m_swapChain) {
            ComPtr<IDXGIDevice1> dxgiDevice;
            winrt::check_hresult(m_device.As(&dxgiDevice));
            ComPtr<IDXGIAdapter> adapter;
            winrt::check_hresult(dxgiDevice->GetAdapter(&adapter));
            ComPtr<IDXGIFactory2> factory;
            winrt::check_hresult(adapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(factory.GetAddressOf())));

            DXGI_SWAP_CHAIN_DESC1 desc = {};
            desc.Width = width;
            desc.Height = height;
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

            winrt::check_hresult(factory->CreateSwapChainForCoreWindow(
                m_device.Get(),
                winrt::get_unknown(m_window),
                &desc,
                nullptr,
                &m_swapChain));
        }

        ComPtr<ID3D11Texture2D> backBuffer;
        winrt::check_hresult(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf())));
        winrt::check_hresult(m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTargetView));
        m_needResize = false;
    }

    void Render() {
        if (!m_context || !m_renderTargetView) return;
        const float clear[4] = { 0.07f, 0.09f, 0.12f, 1.0f };
        m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
        m_context->ClearRenderTargetView(m_renderTargetView.Get(), clear);
        if (m_swapChain) m_swapChain->Present(1, 0);
    }
};

} // namespace PexCraftUwp

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    winrt::init_apartment();
    coreapp::CoreApplication::Run(winrt::make<PexCraftUwp::App>());
    return 0;
}
