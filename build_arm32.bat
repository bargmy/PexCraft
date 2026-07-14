@echo off
setlocal EnableExtensions
call tools\generate_build_info.bat
if errorlevel 1 exit /b 1

rem Windows ARM32 cross-build helper.
rem This is intended for llvm-mingw/clang toolchains that provide an armv7-w64-mingw32 target.
rem Set WIN_ARM32_BIN or WIN_ARM32_CC if your toolchain is installed somewhere else.

if not defined WIN_ARM32_BIN set "WIN_ARM32_BIN=D:\llvm-mingw\bin"
if not defined WIN_ARM32_CC set "WIN_ARM32_CC=%WIN_ARM32_BIN%\armv7-w64-mingw32-clang.exe"
if not exist "%WIN_ARM32_CC%" set "WIN_ARM32_CC=armv7-w64-mingw32-clang"

set "OUT=pexcraft_win_arm32.exe"
set "COMMON_FLAGS=-include build/generated/pex_build_info.h -std=c99 -O2 -Wall -Wextra -DPEX_TARGET_WINDOWS_ARM32=1 -mwindows"
set "LIBS=-ld3d11 -ldxgi -ld3dcompiler -ld3d9 -lopengl32 -lglu32 -lgdi32 -luser32 -lshell32 -lole32 -lwindowscodecs -lcomdlg32 -lvorbisfile -lvorbis -logg -lwinmm -lmfplat -lmfreadwrite -lmfuuid -lws2_32 -lz -lm"

echo Using Windows ARM32 compiler: %WIN_ARM32_CC%
"%WIN_ARM32_CC%" %COMMON_FLAGS% main.c -o "%OUT%" -static -static-libgcc %LIBS%
if errorlevel 1 exit /b 1

echo Built %OUT%
echo Runtime RakNet DLL path: multiplayer\mcpe\protocol_81\transport\bin\windows-arm32\raknet.dll
endlocal
