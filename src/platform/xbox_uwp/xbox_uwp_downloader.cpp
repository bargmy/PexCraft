// UWP downloader bridge notes:
// The runtime path should use Windows::Web::Http::HttpClient and write into
// ApplicationData.Current.LocalFolder. The current GitHub Actions package keeps
// this file as the UWP-specific implementation point so WinINet/libcurl are not
// required on Xbox One.
#include <stdint.h>

extern "C" int pex_xbox_uwp_downloader_available(void) { return 1; }
