@echo off
setlocal EnableExtensions
set "SCRIPT=%~dp0generate_build_info.py"
set "ROOT=%~dp0.."

where python >nul 2>nul
if not errorlevel 1 (
    python "%SCRIPT%" --root "%ROOT%"
    if not errorlevel 1 exit /b 0
)

where py >nul 2>nul
if not errorlevel 1 (
    py -3 "%SCRIPT%" --root "%ROOT%"
    if not errorlevel 1 exit /b 0
)

echo ERROR: Python 3 is required to generate PexCraft build metadata. 1>&2
exit /b 1
