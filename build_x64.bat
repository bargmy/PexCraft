@echo off
setlocal EnableExtensions
call tools\generate_build_info.bat
if errorlevel 1 exit /b 1

rem Build the client as a real 64-bit Windows executable using MSYS2 mingw64.
rem Do not call cc1.exe directly; always call the GCC driver in mingw64\bin.
if not defined MINGW64_BIN set "MINGW64_BIN=D:\msys64\mingw64\bin"
set "CC_X64=%MINGW64_BIN%\x86_64-w64-mingw32-gcc.exe"
set "OBJDUMP_X64=%MINGW64_BIN%\objdump.exe"

if not exist "%CC_X64%" (
    echo ERROR: x64 compiler not found: %CC_X64%
    echo Set it like this if MSYS2 is elsewhere:
    echo   set MINGW64_BIN=D:\msys64\mingw64\bin
    exit /b 1
)

rem Keep mingw64 first so GCC and any runtime DLLs come from the 64-bit toolchain.
rem Also keep System32 early so Windows system DLLs are the native 64-bit ones.
set "PATH=%MINGW64_BIN%;%SystemRoot%\System32;%SystemRoot%;%SystemRoot%\System32\Wbem;%PATH%"

for /f "delims=" %%T in ('"%CC_X64%" -dumpmachine') do set "TARGET=%%T"
echo Using compiler: %CC_X64%
echo Compiler target: %TARGET%
echo %TARGET% | findstr /i "x86_64" >nul
if errorlevel 1 (
    echo ERROR: selected compiler is not an x86_64 compiler.
    exit /b 1
)

set "OUT=pexcraft_x64.exe"

rem -static and -static-libgcc avoid accidental dependency on 32-bit MinGW runtime DLLs in PATH.
"%CC_X64%" -include build/generated/pex_build_info.h -m64 -std=c99 -O2 -Wall -Wextra -mwindows main.c -o "%OUT%" -static -static-libgcc -ld3d11 -ldxgi -ld3dcompiler -ld3d9 -lopengl32 -lglu32 -lgdi32 -luser32 -lshell32 -lole32 -lwindowscodecs -lcomdlg32 -lvorbisfile -lvorbis -logg -lwinmm -lmfplat -lmfreadwrite -lmfuuid -lws2_32 -lz -lm
if errorlevel 1 exit /b 1

if exist "%OBJDUMP_X64%" (
    echo.
    echo PE header check:
    "%OBJDUMP_X64%" -f "%OUT%" | findstr /i "file format architecture"
    echo.
    echo Imported DLLs:
    "%OBJDUMP_X64%" -p "%OUT%" | findstr /i "DLL Name"
)

echo.
echo Built %OUT%
echo If Windows still shows 0xc000007b, check for 32-bit DLLs beside the exe or earlier in PATH.
echo Most common culprit: a 32-bit d3dcompiler_47.dll, libgcc_s_*.dll, or libwinpthread-1.dll.
endlocal
