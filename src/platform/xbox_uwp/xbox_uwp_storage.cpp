// UWP storage bridge. This file is intentionally C++/CX so the C engine can ask
// for the writable LocalState path without using forbidden desktop paths.
#include <string>
#include <mutex>
#include <windows.h>

#if defined(PEX_PLATFORM_XBOX_UWP)
#include <collection.h>
#include <ppltasks.h>
using namespace Platform;
using namespace Windows::Storage;

extern "C" const char *pex_xbox_uwp_get_local_folder(void) {
    static std::string cached;
    static std::once_flag once;
    std::call_once(once, [](){
        auto folder = ApplicationData::Current->LocalFolder;
        std::wstring wide(folder->Path->Data());
        int need = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (need > 0) {
            std::string tmp((size_t)need, '\0');
            WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &tmp[0], need, nullptr, nullptr);
            if (!tmp.empty() && tmp.back() == '\0') tmp.pop_back();
            cached = tmp;
        }
        if (cached.empty()) cached = ".";
    });
    return cached.c_str();
}
#else
extern "C" const char *pex_xbox_uwp_get_local_folder(void) { return "."; }
#endif
