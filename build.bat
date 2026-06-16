@echo off
set CC=i686-w64-mingw32-gcc
%CC% -std=c99 -O2 -Wall -Wextra -mwindows main.c -o pexcraft.exe -lopengl32 -lglu32 -lgdi32 -luser32 -lshell32 -lole32 -lwindowscodecs
if errorlevel 1 exit /b 1
echo Built pexcraft.exe
