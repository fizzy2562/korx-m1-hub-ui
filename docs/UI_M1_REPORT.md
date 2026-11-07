# KORX-M1 UI round 3 report

## Changed

- Replaced the incomplete font with an exhaustive 95-entry ASCII 32..126 table. It includes `[`, `\\`, `]`, `^`, `_`, and `` ` ``; rendering uses an 8-row, 5-column cell with integer scales and the existing dot gap. Added a compile-time array-size assertion.
- Added framebuffer-backed self-tests for every printable character and for `Piano16'`, `Universe`, `Brass1&2`, and `Octave`.
- Corrected the light RGB565 palette: BG `0xB6D5`, GRID `0x9E33`, INK/INV_BG `0x08A2`, INV_FG/KEYTXT `0xB6D5`, and BEZEL `0x2104`. Soft keys now use BEZEL buttons with KEYTXT legends.
- Made init, mode changes, and HOME land on group 0. Updated the shot script comments with the exact event sequence and expected page for all six renders.
- Program play now formats the first three group-1 parameters as the live OSC mode, assign, and multisound values.

## Verified

- `build.bat`: `imports []` and `BUILD OK`.
- `render_shots.bat` regenerated all six shots and `font_test.png`.
- `verify_shots.mjs`: all six PNGs are non-empty, color-varied, and unique; expected titles are asserted/documented; every ASCII cell and all four requested literals print PASS.

## Sizes

- `src/m1ui_view.h`: 11,121 bytes.
- Font table: 95 × 5 packed column bytes = 475 bytes, with 8 row bits per column (equivalent 5×8 cells).
- `src/m1ui_model.h`: 14,356 bytes.
- `out/ui_m1.wasm`: 5,714,616 bytes.

## Not verified

Human visual inspection is not available in this environment, so final subjective spacing/clipping review remains outstanding. No unrelated processes or local models were run.

## Follow-up fixes (2026-11-07, qwen patch, verified by rendering)

- Timbre ON/-- flag was inverted: bit 4 of the timbre byte set means **off** (m1_params `Toggle` convention). Fixed.
- A stray voice-meter rectangle overlapped the program info line. Removed.
- Program screen now shows the two effect names (FX 1 / FX 2) from the program block.
- Still open: unused timbres (E-H) draw full-width level bars; EG graph stroke is thin; not yet measured on the FM-1.
