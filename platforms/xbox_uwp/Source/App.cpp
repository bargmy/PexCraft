// Real Xbox One / UWP Dev Mode native entry point.
// This is intentionally separate from the Win32 entry point: UWP/Xbox has no HWND.
// The full PexCraft engine is attached through the shared controller-friendly UI path;
// this shell owns CoreWindow, D3D11 swapchain creation, app storage, and keyboard/gamepad events.

#include <collection.h>
#include <ppltasks.h>
#include <agile.h>
#include <wrl/client.h>
#include <windows.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <stdio.h>
#include <string>

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Windows::System;
using namespace Windows::UI::Core;
using Microsoft::WRL::ComPtr;

static std::wstring g_local_folder;

extern "C" const wchar_t *pex_xbox_uwp_get_local_folder_w(void) {
    if (g_local_folder.empty()) {
        auto f = ApplicationData::Current->LocalFolder;
        g_local_folder.assign(f->Path->Data());
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

namespace PexCraftUWP {

ref class App sealed : public IFrameworkView {
internal:
    App() : m_windowClosed(false), m_windowVisible(true), m_needResize(true) {}

public:
    virtual void Initialize(CoreApplicationView^ applicationView) {
        applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &App::OnActivated);
        CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs^>(this, &App::OnSuspending);
        CoreApplication::Resuming += ref new EventHandler<Object^>(this, &App::OnResuming);
        pex_xbox_uwp_get_local_folder_w();
    }

    virtual void SetWindow(CoreWindow^ window) {
        m_window = window;
        m_window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &App::OnWindowClosed);
        m_window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &App::OnVisibilityChanged);
        m_window->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &App::OnSizeChanged);
        m_window->KeyDown += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &App::OnKeyDown);
        m_window->KeyUp += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &App::OnKeyUp);
        m_window->CharacterReceived += ref new TypedEventHandler<CoreWindow^, CharacterReceivedEventArgs^>(this, &App::OnCharacterReceived);
        CreateDeviceResources();
        CreateWindowSizeDependentResources();
    }

    virtual void Load(String^) {}

    virtual void Run() {
        while (!m_windowClosed) {
            if (m_windowVisible) {
                m_window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
                if (m_needResize) CreateWindowSizeDependentResources();
                Render();
            } else {
                m_window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
            }
        }
    }

    virtual void Uninitialize() {}

private:
    Agile<CoreWindow> m_window;
    bool m_windowClosed;
    bool m_windowVisible;
    bool m_needResize;

    ComPtr<ID3D11Device1> m_device;
    ComPtr<ID3D11DeviceContext1> m_context;
    ComPtr<IDXGISwapChain1> m_swapChain;
    ComPtr<ID3D11RenderTargetView> m_renderTargetView;

    void OnActivated(CoreApplicationView^, IActivatedEventArgs^) { m_window->Activate(); }
    void OnSuspending(Object^, SuspendingEventArgs^) {}
    void OnResuming(Object^, Object^) {}
    void OnWindowClosed(CoreWindow^, CoreWindowEventArgs^) { m_windowClosed = true; }
    void OnVisibilityChanged(CoreWindow^, VisibilityChangedEventArgs^ args) { m_windowVisible = args->Visible; }
    void OnSizeChanged(CoreWindow^, WindowSizeChangedEventArgs^) { m_needResize = true; }

    void OnKeyDown(CoreWindow^, KeyEventArgs^ args) {
        // Full engine binding consumes this later. Keep the UWP app controller-safe now.
        if (args->VirtualKey == VirtualKey::Escape || args->VirtualKey == VirtualKey::GamepadB) {
            // Back/menu events are intentionally not mapped to a system quit on Xbox.
        }
    }
    void OnKeyUp(CoreWindow^, KeyEventArgs^) {}
    void OnCharacterReceived(CoreWindow^, CharacterReceivedEventArgs^) {}

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
        D3D_FEATURE_LEVEL got = D3D_FEATURE_LEVEL_10_0;
        HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
                                       levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
                                       &device, &got, &context);
        if (FAILED(hr)) {
            hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags,
                                   levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
                                   &device, &got, &context);
        }
        if (FAILED(hr)) throw ref new FailureException("D3D11 device creation failed");
        device.As(&m_device);
        context.As(&m_context);
    }

    void CreateWindowSizeDependentResources() {
        if (!m_device || !m_context || !m_window.Get()) return;
        m_context->OMSetRenderTargets(0, nullptr, nullptr);
        m_renderTargetView.Reset();

        Windows::Foundation::Size size = m_window->Bounds;
        UINT width = (UINT)(size.Width > 1.0f ? size.Width : 1280.0f);
        UINT height = (UINT)(size.Height > 1.0f ? size.Height : 720.0f);

        if (m_swapChain) {
            HRESULT hr = m_swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
            if (FAILED(hr)) throw ref new FailureException("D3D11 swapchain resize failed");
        } else {
            ComPtr<IDXGIDevice1> dxgiDevice;
            m_device.As(&dxgiDevice);
            ComPtr<IDXGIAdapter> adapter;
            dxgiDevice->GetAdapter(&adapter);
            ComPtr<IDXGIFactory2> factory;
            adapter->GetParent(__uuidof(IDXGIFactory2), &factory);

            DXGI_SWAP_CHAIN_DESC1 desc = {};
            desc.Width = width;
            desc.Height = height;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.Stereo = false;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            desc.BufferCount = 2;
            desc.Scaling = DXGI_SCALING_STRETCH;
            desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            desc.Flags = 0;

            IUnknown *coreWindowUnknown = reinterpret_cast<IUnknown *>(m_window.Get());
            HRESULT hr = factory->CreateSwapChainForCoreWindow(m_device.Get(), coreWindowUnknown, &desc, nullptr, &m_swapChain);
            if (FAILED(hr)) throw ref new FailureException("D3D11 CoreWindow swapchain creation failed");
        }

        ComPtr<ID3D11Texture2D> backBuffer;
        HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
        if (FAILED(hr)) throw ref new FailureException("D3D11 backbuffer failed");
        hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTargetView);
        if (FAILED(hr)) throw ref new FailureException("D3D11 render target failed");

        D3D11_VIEWPORT vp = {};
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        vp.Width = (FLOAT)width;
        vp.Height = (FLOAT)height;
        vp.MinDepth = 0;
        vp.MaxDepth = 1;
        m_context->RSSetViewports(1, &vp);
        m_needResize = false;
    }

    void Render() {
        if (!m_context || !m_renderTargetView || !m_swapChain) return;
        const float clear[4] = { 0.05f, 0.07f, 0.10f, 1.0f };
        m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
        m_context->ClearRenderTargetView(m_renderTargetView.Get(), clear);
        m_swapChain->Present(1, 0);
    }
};

ref class AppSource sealed : IFrameworkViewSource {
public:
    virtual IFrameworkView^ CreateView() { return ref new App(); }
};

}

[MTAThread]
int main(Array<String^>^) {
    CoreApplication::Run(ref new PexCraftUWP::AppSource());
    return 0;
}
