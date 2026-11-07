@echo off
set ZIG=C:\build\korx-m1\tools\zigpkg\ziglang\zig.exe
set W=C:\build\korx-m1\work
set F=%W%\free
set P=C:\build\korx-m1\ref\PicoVintageSynthCollection\instruments\PicoFaceM1
set ZIG_GLOBAL_CACHE_DIR=%F%\.zigcache
set ZIG_LOCAL_CACHE_DIR=%F%\.zigcache
if exist %F%\out\korx-m1-free.wasm del %F%\out\korx-m1-free.wasm
cd /d %F%\out
%ZIG% c++ -target wasm32-wasi -mexec-model=reactor -O2 -fno-exceptions -fno-rtti -I %F%\src -I %P%\include %F%\src\hub_adapter_free.cpp %F%\src\m1_params.cpp %P%\src\m1_engine\m1_engine.cpp %P%\src\m1_engine\m1_fx.cpp -o %F%\out\korx-m1-free.wasm 2>&1 | findstr /C:"error" /C:"undefined" /C:"Error"
if not exist %F%\out\korx-m1-free.wasm (echo BUILD FAILED & exit /b 1)
for %%A in (%F%\out\korx-m1-free.wasm) do echo size %%~zA
node %W%\imp.mjs %F%\out\korx-m1-free.wasm | findstr /C:"imports" /C:"exports"
echo BUILD OK
