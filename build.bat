@echo off
setlocal EnableExtensions
if not defined MINGW32_BIN set "MINGW32_BIN=D:\msys64\mingw32\bin"
set "CC=%MINGW32_BIN%\i686-w64-mingw32-gcc.exe"
if not exist "%CC%" set "CC=i686-w64-mingw32-gcc"
%CC% -std=c99 -O2 -Wall -Wextra -mwindows main.c -o pexcraft.exe -static -static-libgcc -ld3d11 -ldxgi -ld3dcompiler -ld3d9 -lopengl32 -lglu32 -lgdi32 -luser32 -lshell32 -lole32 -lwindowscodecs -lcomdlg32 -lwinmm -lws2_32 -lz -lm
if errorlevel 1 exit /b 1
echo Built pexcraft.exe with static MinGW runtime where available.
endlocal
