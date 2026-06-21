@echo off
setlocal EnableExtensions
if not defined MINGW64_BIN set "MINGW64_BIN=D:\msys64\mingw64\bin"
set "OBJDUMP_X64=%MINGW64_BIN%\objdump.exe"
set "OUT=pexcraft_x64.exe"

if not exist "%OUT%" (
    echo ERROR: %OUT% not found. Build it first with build_x64.bat.
    exit /b 1
)

if exist "%OBJDUMP_X64%" (
    echo PE header:
    "%OBJDUMP_X64%" -f "%OUT%"
    echo.
    echo Imported DLLs:
    "%OBJDUMP_X64%" -p "%OUT%" | findstr /i "DLL Name"
) else (
    echo objdump not found at %OBJDUMP_X64%
)

echo.
echo DLL search hits that can cause 0xc000007b if the first one is 32-bit:
where d3dcompiler_47.dll 2>nul
where libgcc_s_seh-1.dll 2>nul
where libgcc_s_sjlj-1.dll 2>nul
where libwinpthread-1.dll 2>nul
where zlib1.dll 2>nul

echo.
echo Also check this folder for copied 32-bit DLLs:
dir /b *.dll 2>nul
endlocal
