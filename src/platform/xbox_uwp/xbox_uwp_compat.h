#ifndef PEXCRAFT_XBOX_UWP_COMPAT_H
#define PEXCRAFT_XBOX_UWP_COMPAT_H

/* Xbox One / UWP compatibility layer for the shared C engine files.
   The real UWP shell lives under platforms/xbox_uwp and provides the
   CoreWindow, D3D11 swapchain, app storage, and downloader glue. */

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <mmsystem.h>
#include <objbase.h>
#include <initguid.h>
#include <wincodec.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <stdint.h>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#endif /* PEXCRAFT_XBOX_UWP_COMPAT_H */
