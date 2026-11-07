@echo off
set ZIG=C:\build\korx-m1\tools\zigpkg\ziglang\zig.exe
set W=C:\build\korx-m1\work
copy /Y %W%\gen61\gfx.h %W%\ui\gfx.h >nul
copy /Y %W%\gen60b\gfx_test.cpp %W%\ui\gfx_test.cpp >nul
cd /d %W%\ui
%ZIG% c++ -target wasm32-wasi -mexec-model=reactor -O2 -fno-exceptions -fno-rtti -I . gfx_test.cpp -o gfx_test.wasm 2>&1
dir gfx_test.wasm | findstr wasm
node ..\gen58\shot.mjs gfx_test.wasm gfx_test.png a:2
