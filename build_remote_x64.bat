@echo off
setlocal EnableExtensions
if not defined MINGW64_BIN set "MINGW64_BIN=D:\msys64\mingw64\bin"
set "CC_X64=%MINGW64_BIN%\x86_64-w64-mingw32-gcc.exe"
if not exist "%CC_X64%" (
    echo ERROR: x64 compiler not found: %CC_X64%
    exit /b 1
)
set "PATH=%MINGW64_BIN%;%SystemRoot%\System32;%SystemRoot%;%SystemRoot%\System32\Wbem;%PATH%"
"%CC_X64%" -m64 -std=c99 -O2 -Wall -Wextra -DPEX_REMOTE_HTTP=1 -DPEX_REMOTE_FPS=30 -mwindows main.c -o pexcraft_remote_x64.exe -static -static-libgcc -ld3d11 -ldxgi -ld3dcompiler -ld3d9 -lopengl32 -lglu32 -lgdi32 -luser32 -lshell32 -lole32 -loleaut32 -lwindowscodecs -lcomdlg32 -lwinmm -lmfplat -lmfreadwrite -lmfuuid -lws2_32 -lz -lm
if errorlevel 1 exit /b 1
echo Built pexcraft_remote_x64.exe. Open http://SERVER_IP:8425/ in Opera 11.
endlocal
