@echo off
if not exist shots mkdir shots
rem 01_program.png: init lands on Program group 0; no navigation event is needed.
node shot.mjs out\ui_m1.wasm shots\01_program.png e:3,10,1,0 e:3,10,0,0 a:3
rem 02_combi.png: F3 (button id 2) down/up toggles Combi mode; a:3 captures the combination play screen.
node shot.mjs out\ui_m1.wasm shots\02_combi.png e:3,2,1,0 e:3,2,0,0 a:3
rem 03_osc.png: K2 (button id 11) down/up selects Program group 1, the OSC page; a:3 captures it.
node shot.mjs out\ui_m1.wasm shots\03_osc.png e:3,11,1,0 e:3,11,0,0 a:3
rem 04_vdf_eg.png: K3 (button id 12) down/up selects Program group 2, the VDF EG page; a:3 captures it.
node shot.mjs out\ui_m1.wasm shots\04_vdf_eg.png e:3,12,1,0 e:3,12,0,0 a:3
rem 05_effects.png: K4 (button id 13) down/up selects Program group 3, the effects page; a:3 captures it.
node shot.mjs out\ui_m1.wasm shots\05_effects.png e:3,13,1,0 e:3,13,0,0 a:3
rem 06_filter.png: K5 (button id 14) down/up selects Program group 4, the VDF response page; a:3 captures it.
node shot.mjs out\ui_m1.wasm shots\06_filter.png e:3,14,1,0 e:3,14,0,0 a:3
rem font_test.png: event type 99/id 77 enables the printable-ASCII font card.
node shot.mjs out\ui_m1.wasm shots\font_test.png e:99,77,1,0
