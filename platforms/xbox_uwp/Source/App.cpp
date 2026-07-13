// Xbox One / UWP Dev Mode native shell for the real PexCraft engine.
// This file owns CoreWindow events and forwards them into src/main_xbox_uwp.c.

#include <windows.h>
#include <winrt/base.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Gaming.Input.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Core.h>
#include <stdint.h>
#include <string>
#include <cstdlib>
#include <exception>

namespace winrt_app = winrt::Windows::ApplicationModel;
namespace activation = winrt::Windows::ApplicationModel::Activation;
namespace coreapp = winrt::Windows::ApplicationModel::Core;
namespace storage = winrt::Windows::Storage;
namespace streams = winrt::Windows::Storage::Streams;
namespace webhttp = winrt::Windows::Web::Http;
namespace gaminginput = winrt::Windows::Gaming::Input;
namespace win_system = winrt::Windows::System;
namespace coreui = winrt::Windows::UI::Core;

extern "C" {
    const char *pex_xbox_uwp_get_local_folder(void);
    const wchar_t *pex_xbox_uwp_get_local_folder_w(void);
    int  pex_xbox_uwp_engine_init(void *core_window_unknown, int width, int height);
    void pex_xbox_uwp_engine_frame(void);
    void pex_xbox_uwp_engine_resize(int width, int height);
    void pex_xbox_uwp_engine_key_down(int vk);
    void pex_xbox_uwp_engine_key_up(int vk);
    void pex_xbox_uwp_engine_char(uint32_t ch);
    void pex_xbox_uwp_engine_remote_key(int raw_key, int down);
    void pex_xbox_uwp_engine_shutdown(void);
}

static std::wstring g_local_folder;


static void pex_uwp_append_boot_log(const wchar_t *msg) noexcept {
    if (!msg) return;
    try {
        std::wstring dir(storage::ApplicationData::Current().LocalFolder().Path().c_str());
        dir.append(L"\\PexCraft");
        CreateDirectoryW(dir.c_str(), nullptr);
        std::wstring path = dir + L"\\uwp_boot.log";
        HANDLE h = CreateFile2(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               OPEN_ALWAYS, nullptr);
        if (h == INVALID_HANDLE_VALUE) return;
        LARGE_INTEGER zero{};
        SetFilePointerEx(h, zero, nullptr, FILE_END);
        std::wstring line = L"[PexCraft UWP] ";
        line.append(msg);
        line.append(L"\r\n");
        int need = WideCharToMultiByte(CP_UTF8, 0, line.c_str(), (int)line.size(), nullptr, 0, nullptr, nullptr);
        if (need > 0) {
            std::string utf8((size_t)need, '\0');
            WideCharToMultiByte(CP_UTF8, 0, line.c_str(), (int)line.size(), &utf8[0], need, nullptr, nullptr);
            DWORD written = 0;
            WriteFile(h, utf8.data(), (DWORD)utf8.size(), &written, nullptr);
        }
        CloseHandle(h);
    } catch (...) {
        // OutputDebugString below remains available if app-data access itself failed.
    }
}

static void pex_uwp_output(const wchar_t *msg) {
    if (!msg) return;
    OutputDebugStringW(L"[PexCraft UWP] ");
    OutputDebugStringW(msg);
    OutputDebugStringW(L"\n");
    pex_uwp_append_boot_log(msg);
}


static std::wstring pex_wide_from_utf8(const char *text) {
    if (!text || !*text) return std::wstring();
    int need = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    if (need <= 0) return std::wstring();
    std::wstring out;
    out.resize((size_t)need);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, &out[0], need);
    if (!out.empty() && out.back() == L'\0') out.pop_back();
    return out;
}

extern "C" int pex_xbox_uwp_http_download_to_memory(const char *url, char **out_data, size_t *out_len, size_t max_len, volatile long *cancel_flag) {
    if (out_data) *out_data = nullptr;
    if (out_len) *out_len = 0;
    if (!url || !*url || !out_data || !out_len || max_len == 0) return 0;
    if (cancel_flag && *cancel_flag) return 0;
    try {
        webhttp::HttpClient client;
        std::wstring wurl = pex_wide_from_utf8(url);
        if (wurl.empty()) return 0;
        auto response = client.GetAsync(winrt::Windows::Foundation::Uri(winrt::hstring(wurl.c_str()))).get();
        if (cancel_flag && *cancel_flag) return 0;
        if (!response.IsSuccessStatusCode()) return 0;
        streams::IBuffer buffer = response.Content().ReadAsBufferAsync().get();
        uint32_t len = buffer.Length();
        if (len == 0 || (size_t)len > max_len) return 0;
        char *mem = (char *)std::malloc((size_t)len + 1);
        if (!mem) return 0;
        streams::DataReader reader = streams::DataReader::FromBuffer(buffer);
        reader.ReadBytes(winrt::array_view<uint8_t>((uint8_t *)mem, (uint8_t *)mem + len));
        mem[len] = 0;
        *out_data = mem;
        *out_len = (size_t)len;
        return 1;
    } catch (...) {
        return 0;
    }
}

extern "C" int pex_xbox_uwp_http_download_to_file(const char *url, const char *path, unsigned int expect_size, volatile long *cancel_flag, volatile long *progress_out, volatile long *total_out) {
    if (!url || !*url || !path || !*path) return 0;
    if (progress_out) *progress_out = 0;
    if (total_out) *total_out = 0;
    if (cancel_flag && *cancel_flag) return 0;
    try {
        webhttp::HttpClient client;
        std::wstring wurl = pex_wide_from_utf8(url);
        if (wurl.empty()) return 0;
        auto response = client.GetAsync(winrt::Windows::Foundation::Uri(winrt::hstring(wurl.c_str()))).get();
        if (cancel_flag && *cancel_flag) return 0;
        if (!response.IsSuccessStatusCode()) return 0;
        streams::IBuffer buffer = response.Content().ReadAsBufferAsync().get();
        uint32_t len = buffer.Length();
        if (len == 0) return 0;
        if (expect_size && len != expect_size) return 0;
        if (total_out) *total_out = (long)len;
        std::wstring wpath = pex_wide_from_utf8(path);
        if (wpath.empty()) return 0;
        HANDLE h = CreateFile2(wpath.c_str(), GENERIC_WRITE, 0, CREATE_ALWAYS, nullptr);
        if (h == INVALID_HANDLE_VALUE) return 0;
        char *mem = (char *)std::malloc((size_t)len);
        if (!mem) { CloseHandle(h); DeleteFileW(wpath.c_str()); return 0; }
        streams::DataReader reader = streams::DataReader::FromBuffer(buffer);
        reader.ReadBytes(winrt::array_view<uint8_t>((uint8_t *)mem, (uint8_t *)mem + len));
        DWORD written = 0;
        BOOL ok = WriteFile(h, mem, len, &written, nullptr);
        std::free(mem);
        CloseHandle(h);
        if (!ok || written != len) { DeleteFileW(wpath.c_str()); return 0; }
        if (progress_out) *progress_out = 100;
        return 1;
    } catch (...) {
        return 0;
    }
}

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
    if (cached.empty()) {
        const wchar_t *wide = pex_xbox_uwp_get_local_folder_w();
        int need = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
        if (need > 0) {
            cached.resize((size_t)need);
            WideCharToMultiByte(CP_UTF8, 0, wide, -1, &cached[0], need, nullptr, nullptr);
            if (!cached.empty() && cached.back() == '\0') cached.pop_back();
        }
    }
    return cached.c_str();
}

extern "C" int pex_xbox_uwp_gamepad_count(void) {
    try {
        auto pads = gaminginput::Gamepad::Gamepads();
        uint32_t count = pads.Size();
        return (int)(count > 8 ? 8 : count);
    } catch (...) {
        return 0;
    }
}

extern "C" int pex_xbox_uwp_gamepad_read(int index, float *lx, float *ly, float *rx, float *ry,
                                           float *lt, float *rt, unsigned int *buttons) {
    if (!lx || !ly || !rx || !ry || !lt || !rt || !buttons || index < 0) return 0;
    *lx = *ly = *rx = *ry = *lt = *rt = 0.0f;
    *buttons = 0;
    try {
        auto pads = gaminginput::Gamepad::Gamepads();
        if ((uint32_t)index >= pads.Size()) return 0;
        auto reading = pads.GetAt((uint32_t)index).GetCurrentReading();
        *lx = (float)reading.LeftThumbstickX;
        *ly = (float)-reading.LeftThumbstickY;
        *rx = (float)reading.RightThumbstickX;
        *ry = (float)-reading.RightThumbstickY;
        *lt = (float)reading.LeftTrigger;
        *rt = (float)reading.RightTrigger;

        uint32_t raw = (uint32_t)reading.Buttons;
        auto has = [raw](gaminginput::GamepadButtons button) {
            return (raw & (uint32_t)button) != 0;
        };
        unsigned int out = 0;
        if (has(gaminginput::GamepadButtons::A)) out |= 1u << 0;
        if (has(gaminginput::GamepadButtons::B)) out |= 1u << 1;
        if (has(gaminginput::GamepadButtons::X)) out |= 1u << 2;
        if (has(gaminginput::GamepadButtons::Y)) out |= 1u << 3;
        if (has(gaminginput::GamepadButtons::LeftShoulder)) out |= 1u << 4;
        if (has(gaminginput::GamepadButtons::RightShoulder)) out |= 1u << 5;
        if (has(gaminginput::GamepadButtons::View)) out |= 1u << 6;
        if (has(gaminginput::GamepadButtons::Menu)) out |= 1u << 7;
        if (has(gaminginput::GamepadButtons::LeftThumbstick)) out |= 1u << 8;
        if (has(gaminginput::GamepadButtons::RightThumbstick)) out |= 1u << 9;
        if (has(gaminginput::GamepadButtons::DPadUp)) out |= 1u << 10;
        if (has(gaminginput::GamepadButtons::DPadDown)) out |= 1u << 11;
        if (has(gaminginput::GamepadButtons::DPadLeft)) out |= 1u << 12;
        if (has(gaminginput::GamepadButtons::DPadRight)) out |= 1u << 13;
        *buttons = out;
        return 1;
    } catch (...) {
        return 0;
    }
}

static bool pex_is_gamepad_virtual_key(win_system::VirtualKey key) {
    uint32_t value = (uint32_t)key;
    /* Windows reserves 195..218 for the complete Gamepad virtual-key range.
       Numeric bounds avoid SDK-version enum gaps while preserving the exact ABI. */
    return value >= 195u && value <= 218u;
}

static int pex_vk_from_virtual_key(win_system::VirtualKey key) {
    using win_system::VirtualKey;
    uint32_t k = (uint32_t)key;
    if (k >= (uint32_t)VirtualKey::Number0 && k <= (uint32_t)VirtualKey::Number9) return (int)('0' + (k - (uint32_t)VirtualKey::Number0));
    if (k >= (uint32_t)VirtualKey::A && k <= (uint32_t)VirtualKey::Z) return (int)('A' + (k - (uint32_t)VirtualKey::A));
    switch (key) {
        case VirtualKey::Left: return 0x25;
        case VirtualKey::Up: return 0x26;
        case VirtualKey::Right: return 0x27;
        case VirtualKey::Down: return 0x28;
        case VirtualKey::Space: return 0x20;
        case VirtualKey::Enter: return 0x0D;
        case VirtualKey::Back: return 0x08;
        case VirtualKey::Escape: return 0x1B;
        case VirtualKey::Shift: return 0x10;
        case VirtualKey::Control: return 0x11;
        case VirtualKey::Menu: return 0x12;
        case VirtualKey::Tab: return 0x09;
        case VirtualKey::F3: return 0x72;
        case VirtualKey::F5: return 0x74;
        case VirtualKey::F11: return 0x7A;
        case VirtualKey::GamepadDPadLeft: return 0x25;
        case VirtualKey::GamepadDPadUp: return 0x26;
        case VirtualKey::GamepadDPadRight: return 0x27;
        case VirtualKey::GamepadDPadDown: return 0x28;
        case VirtualKey::GamepadA: return 0x0D;
        case VirtualKey::GamepadB: return 0x1B;
        case VirtualKey::GamepadView: return 0x09;
        case VirtualKey::GamepadMenu: return 0x1B;
        default: return 0;
    }
}

namespace PexCraftUWP {

struct App : winrt::implements<App, coreapp::IFrameworkViewSource, coreapp::IFrameworkView> {
    coreui::CoreWindow m_window{ nullptr };
    bool m_windowClosed = false;
    bool m_windowVisible = true;
    bool m_activated = false;
    bool m_engineStarted = false;
    bool m_engineReady = false;

    coreapp::IFrameworkView CreateView() { return *this; }

    void Initialize(coreapp::CoreApplicationView const& applicationView) {
        pex_uwp_output(L"IFrameworkView::Initialize");
        applicationView.Activated({ this, &App::OnActivated });
        coreapp::CoreApplication::Suspending({ this, &App::OnSuspending });
        coreapp::CoreApplication::Resuming({ this, &App::OnResuming });
    }

    void SetWindow(coreui::CoreWindow const& window) {
        pex_uwp_output(L"IFrameworkView::SetWindow");
        m_window = window;
        m_window.Closed({ this, &App::OnWindowClosed });
        m_window.VisibilityChanged({ this, &App::OnVisibilityChanged });
        m_window.SizeChanged({ this, &App::OnSizeChanged });
        m_window.KeyDown({ this, &App::OnKeyDown });
        m_window.KeyUp({ this, &App::OnKeyUp });
        m_window.CharacterReceived({ this, &App::OnCharacterReceived });
    }

    void Load(winrt::hstring const&) {
        pex_uwp_output(L"IFrameworkView::Load");
    }

    void StartRealEngine() {
        if (m_engineStarted) return;
        m_engineStarted = true;
        pex_uwp_output(L"starting real PexCraft engine");
        auto b = m_window.Bounds();
        int w = (int)(b.Width > 1.0f ? b.Width : 1280.0f);
        int h = (int)(b.Height > 1.0f ? b.Height : 720.0f);
        m_engineReady = pex_xbox_uwp_engine_init(winrt::get_unknown(m_window), w, h) != 0;
        if (!m_engineReady) {
            pex_uwp_output(L"real engine init returned failure");
            m_windowClosed = true;
        } else {
            pex_uwp_output(L"real engine init OK");
        }
    }

    void Run() {
        pex_uwp_output(L"IFrameworkView::Run entered");
        while (!m_windowClosed) {
            if (m_windowVisible) {
                m_window.Dispatcher().ProcessEvents(coreui::CoreProcessEventsOption::ProcessAllIfPresent);
                if (m_activated && !m_engineStarted) StartRealEngine();
                if (m_engineReady) pex_xbox_uwp_engine_frame();
            } else {
                m_window.Dispatcher().ProcessEvents(coreui::CoreProcessEventsOption::ProcessOneAndAllPending);
            }
        }
    }

    void Uninitialize() {
        pex_uwp_output(L"IFrameworkView::Uninitialize");
        if (m_engineReady) pex_xbox_uwp_engine_shutdown();
    }

    void OnActivated(coreapp::CoreApplicationView const&, activation::IActivatedEventArgs const&) {
        pex_uwp_output(L"OnActivated: Activate CoreWindow");
        m_activated = true;
        coreui::CoreWindow::GetForCurrentThread().Activate();
    }
    void OnSuspending(winrt::Windows::Foundation::IInspectable const&, winrt_app::SuspendingEventArgs const&) {}
    void OnResuming(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::Foundation::IInspectable const&) {}
    void OnWindowClosed(coreui::CoreWindow const&, coreui::CoreWindowEventArgs const&) { m_windowClosed = true; }
    void OnVisibilityChanged(coreui::CoreWindow const&, coreui::VisibilityChangedEventArgs const& args) { m_windowVisible = args.Visible(); }
    void OnSizeChanged(coreui::CoreWindow const&, coreui::WindowSizeChangedEventArgs const& args) {
        int w = (int)(args.Size().Width > 1.0f ? args.Size().Width : 1280.0f);
        int h = (int)(args.Size().Height > 1.0f ? args.Size().Height : 720.0f);
        if (m_engineReady) pex_xbox_uwp_engine_resize(w, h);
    }
    void OnKeyDown(coreui::CoreWindow const&, coreui::KeyEventArgs const& args) {
        auto key = args.VirtualKey();
        if (pex_is_gamepad_virtual_key(key)) {
            /* A physical Xbox controller is read through Windows.Gaming.Input.
               CoreWindow's duplicate Gamepad* key events are reserved as a
               digital media-remote fallback only when no gamepad is present. */
            if (m_engineReady && gaminginput::Gamepad::Gamepads().Size() == 0)
                pex_xbox_uwp_engine_remote_key((int)(uint32_t)key, 1);
            args.Handled(true);
            return;
        }
        int vk = pex_vk_from_virtual_key(key);
        if (vk && m_engineReady) { pex_xbox_uwp_engine_key_down(vk); args.Handled(true); }
    }
    void OnKeyUp(coreui::CoreWindow const&, coreui::KeyEventArgs const& args) {
        auto key = args.VirtualKey();
        if (pex_is_gamepad_virtual_key(key)) {
            if (m_engineReady && gaminginput::Gamepad::Gamepads().Size() == 0)
                pex_xbox_uwp_engine_remote_key((int)(uint32_t)key, 0);
            args.Handled(true);
            return;
        }
        int vk = pex_vk_from_virtual_key(key);
        if (vk && m_engineReady) { pex_xbox_uwp_engine_key_up(vk); args.Handled(true); }
    }
    void OnCharacterReceived(coreui::CoreWindow const&, coreui::CharacterReceivedEventArgs const& args) {
        uint32_t ch = args.KeyCode();
        if (ch >= 32 && ch < 127 && m_engineReady) { pex_xbox_uwp_engine_char(ch); args.Handled(true); }
    }
};
} // namespace PexCraftUWP

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    try {
        pex_uwp_output(L"wWinMain: CoreApplication::Run");
        coreapp::CoreApplication::Run(winrt::make<PexCraftUWP::App>());
        return 0;
    } catch (winrt::hresult_error const& e) {
        wchar_t msg[1024];
        swprintf_s(msg, L"fatal C++/WinRT exception: hr=0x%08X message=%s",
                   (unsigned int)e.code().value, e.message().c_str());
        pex_uwp_output(msg);
        return (int)e.code().value;
    } catch (std::exception const& e) {
        wchar_t msg[1024];
        std::wstring wide = pex_wide_from_utf8(e.what());
        swprintf_s(msg, L"fatal std::exception: %s", wide.c_str());
        pex_uwp_output(msg);
        return -1;
    } catch (...) {
        pex_uwp_output(L"fatal unknown exception escaped the app lifecycle");
        return -1;
    }
}
