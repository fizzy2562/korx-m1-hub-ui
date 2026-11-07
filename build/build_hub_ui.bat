@echo off
set ZIG=C:\build\korx-m1\tools\zigpkg\ziglang\zig.exe
set W=C:\build\korx-m1\work
set INC=-I C:\build\korx-m1\ref\PicoVintageSynthCollection\instruments\PicoFaceM1\include -I %W%\gen_m1 -I %W%\ui
set SRC=%W%\gen74\hub_adapter_ui.cpp %W%\ui\m1_params.cpp %W%\wasm\m1_engine.o %W%\wasm\m1_fx.o
cd /d %W%\hubL
echo FULL
%ZIG% c++ -target wasm32-wasi -mexec-model=reactor -O2 -fno-exceptions -fno-rtti %INC% -I %W%\gen74full %SRC% %W%\hub\pcm.o -o korx-m1-ui-full.wasm 2>&1 | findstr /C:"error"
echo LITE
%ZIG% c++ -target wasm32-wasi -mexec-model=reactor -O2 -fno-exceptions -fno-rtti %INC% -I %W%\gen57 %SRC% pcm.o -o korx-m1-ui-lite.wasm 2>&1 | findstr /C:"error"
dir korx-m1-ui-full.wasm korx-m1-ui-lite.wasm | findstr wasm
node %W%\imp.mjs korx-m1-ui-full.wasm
