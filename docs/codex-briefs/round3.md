# Round 3: last fixes on the KORX-M1 LCD UI

Same rules: work ONLY inside `C:\build\korx-m1\work\codex14` (owner-granted full access for this task only; stay in the folder; do NOT run ollama or any local model; do NOT touch other processes). Read `REPORT.md`, `src/m1ui_view.h`, `src/m1ui_model.h`, `render_shots.bat`, `verify_shots.mjs` first. A human reviewed your round-2 renders (you cannot see images). Uppercase text, digits, the brand, the soft-key captions, and page navigation are now correct. Keep all of that. Fix exactly these:

## 1. Font table is missing six glyphs, so every lowercase letter is wrong

The font test card shows that after `Z` the table continues directly with `a`: the glyphs for `[ \ ] ^ _` and the backtick (ASCII 91..96) are missing, so every character from `a` upward is drawn 6 positions off. Result: `Piano16'` renders as `Poatu16'`, `Universe` as `Utolkx4k`, `Brass1&2` as `Bxayy1&2`, and the parameter label `Octave` as `Oizalk`. Your self-test only checked `A..Z`, so it passed.

* The glyph table MUST have exactly **95 entries** (ASCII 32..126), indexed `ch - 32`, with a `static_assert` on the array size (`sizeof(table)/sizeof(table[0]) == 95`) so a missing glyph breaks the build.
* Draw `[ \ ] ^ _ \`` properly.
* Use an **8-row cell** (5 columns x 8 rows) so lowercase `g j p q y` have real descenders and `,` `;` sit low; pack as 8 bytes per glyph (760 bytes total, fine) or any equivalent. Keep the integer scale 1/2/3 and the dot gap.
* **Make the self-test exhaustive:** in `verify_shots.mjs` (or a native test), render ALL 95 characters through the framebuffer via the font test card and compare every character's cell against the table bits. It must print one PASS/FAIL per character and exit non-zero if any fails. It must ALSO render the literal strings `Piano16'`, `Universe`, `Brass1&2`, `Octave` and check each character.

## 2. Remove the brown, fix the palette

The soft keys, slider troughs, dividers, grid dots and tab strip are brown (about RGB 160,82,45) although the brief said no brown. Find every use of that colour and replace it. Use exactly these RGB values (convert to RGB565 correctly: r5 = r>>3, g6 = g>>2, b5 = b>>3, packed `(r5<<11)|(g6<<5)|b5`):

| Name | RGB | Use |
|---|---|---|
| `BG` | 176, 216, 172 | LCD backlight field |
| `GRID` | 156, 196, 154 | dot grid, slider troughs, inactive tab strip segments |
| `INK` | 14, 22, 20 | text, active slider fill, graph strokes, active tab segment |
| `INV_BG` | 14, 22, 20 | inverse-video cell background |
| `INV_FG` | 176, 216, 172 | inverse-video text |
| `BEZEL` | 34, 38, 36 | soft-key buttons and the frame around the LCD |
| `KEYTXT` | 176, 216, 172 | soft-key legends |

Soft keys: `BEZEL` buttons with `KEYTXT` legends, y 282..319 as now. Sliders: `INK` fill on a `GRID` trough. No other colours except in the dark theme alternate.

## 3. The Program/Combi play screen must be the landing screen

Currently the model lands on group 1 (an edit page) so the play screen (group 0) is never seen, and the "combination" shot is the Combi Type edit page. In YOUR copy of `src/m1ui_model.h` only, make `init`, `setMode` and the HOME button land on **group 0** (the play screen) instead of group 1. Then the shots must be:

* `01_program.png`: Program play screen (group 0).
* `02_combi.png`: press F3 (button id 2, down 1 then 0, then `a:3`) to enter Combi mode: **Combination play screen with the eight-timbre strip** (rows A..H: program number, program name, level slider, ON/--).
* `03_osc.png`: Program mode, K2 (id 11) -> OSC page with four slots.
* `04_vdf_eg.png`: a VDF EG page with the envelope graph.
* `05_effects.png`: an effects page.
* `06_filter.png`: the VDF page with the response curve.

Document in `render_shots.bat` exactly which events reach each shot, and in `verify_shots.mjs` assert that all six shots differ and print the page title text expected for each.

## 4. Real values on the Program play screen

The line currently printed is the literal caption `OSC1 MODE / ASSIGN / MULTISOUND`. Replace it with the actual values: for the current program, format `<OSC mode> / <assign> / <OSC 1 multisound name>` by calling `m1::paramFormat` on the first three parameters of the model's group 1 (use `m.groupFirst[1]`, `m.params[...]` and the edit block `m1ui::block(m)`), e.g. `Single / Poly / Piano`. Keep it on one line in INK; shorten with scale 1 if too long.

## Verification

* `build.bat` prints `imports []` and `BUILD OK`.
* Re-render all shots and `font_test.png`; run `verify_shots.mjs` (all PASS, non-zero exit on failure).
* Update `REPORT.md`: what changed, what is verified, what is not, font and view byte sizes. Stop when REPORT.md is updated.
