# Task: an M1-inspired, modern UI theme for the KORX-M1 front panel (Hub wasm now, FM-1 hardware later)

Work ONLY inside `C:\build\korx-m1\work\codex14` (you have full access to this machine, granted by the owner for this task only; limit yourself to this folder anyway; the compiler caches already point inside it). Do not touch any other directory. This is a shared machine and another person's job is running on it: do NOT run ollama or any local model, do NOT kill or inspect other processes, do NOT delete anything outside your folder. No network is needed.

## Goal

Make the screen look and feel like the original **Korg M1** (1988): its dot-matrix LCD workstation interface, with its modes, page structure, cursor and parameter-editing feel, but reinterpreted as a **modern, crisp design** on a 480x320 colour screen. This will later be ported to the **M-VAVE FM-1** firmware, so every choice must be device-friendly (see "Device constraints"). The design must be **original**: do not copy Korg bitmaps, ROM fonts, logos or trademarks; the brand on screen is "KORX". Use your own knowledge of the original M1's user interface (Program / Combination play modes, the edit pages for OSC, pitch EG, VDF, VDF EG, VDA, MGs, effects; inverse-video cursor on the selected parameter; Compare/Write; bank/number naming such as A00 or INT-A00). Where unsure, follow the page structure the code already has (below).

## What exists (read these first)

* `src/m1ui_model.h` : the UI model (state, pages, editing, events). Do not change its behaviour. You may add read-only helper functions in a NEW header `src/m1ui_extras.h` (for example to read timbre data from the combination block).
* `src/m1ui_view.h` : the CURRENT view (anti-aliased "native VIBE" look). `original\` holds pristine copies. **You will replace `src/m1ui_view.h`** (keep the same two public functions: `m1ui::draw(const Model&, const Meters&)` and `m1ui::fillLeds(const Model&, uint8_t* buf384)`, same `Meters` struct) so the adapter builds unchanged.
* `src/gfx.h`, `src/font_data.h` : the current drawing library and 8-bit anti-aliased fonts (float maths, big data). Fine for the Hub, **not acceptable for the device**; see constraints.
* `src/hub_adapter_ui.cpp` : the Hub adapter. It returns `gfx::fb` from `vibe_wasm_draw`. You may edit this copy minimally (for example if your view owns its own framebuffer) but keep every export and its signature.
* `src/m1_params.{h,cpp}` : every M1 parameter (names, ranges, formatting): `m1::programPages`, `combiPages`, `paramFormat`, `effectParams`, `kFxNames` etc.
* `docs/FINDINGS.md`, `docs/reference_obervave_native.png` (a native VIBE firmware screen), `docs/ui_osc1_page.png`, `docs/ui_combi_page.png` (the current look).
* `build.bat` : builds `out\ui_m1.wasm` from `src\` (check it prints `imports []` and `BUILD OK`).
* `shot.mjs` : renders the wasm's 480x320 screen to a PNG (shown at 2x). Usage:
  `node shot.mjs out\ui_m1.wasm shots\NAME.png a:30 [steps...]`. Steps: `a:N` run N audio blocks; `e:type,id,down,value` send an event; `s:path.png` save a frame mid-run. Events: type 3 = button (down 1 then 0), type 4 = encoder (value +1/-1). Button ids: F1..F6 = 0..5 (F1 prev program, F2 next program, F3 toggles Program/Combi, F4 Compare, F5/F6 page -/+), K1..K6 = 10..15 jump to page 0..5, HOME 17, LEFT 18, RIGHT 20, ENTER 9. Encoders E1..E4 = ids 0..3 edit the four visible parameters; id 6 (DATA) steps programs. Key (type 2) `e:2,12,1,0.8` plays middle C. After a key press add `a:60` so the meters move.

## Hub touch geometry (keep it)

The Hub makes the screen touch-clickable because the firmware names its soft keys. Keep: the **six soft-key fields along y 282-320** (80 px each, the captions come from the model and must line up with F1..F6) and **four parameter columns, 115 px wide from x 10 to 470**, one per edit slot (slot i is encoder i). Your LCD design can look different, but the four edit slots must sit in those four columns.

## Device constraints (important)

* The FM-1 build tool has **no soft-float library**. The new view and any new drawing code must use **integer maths only**: no `float`, `double`, `sqrt`, `sin`, `atan2`, `fmod`. Use integer algorithms, fixed-point and small lookup tables (for curves, arcs, circles, rounded corners).
* **No 8-bit anti-aliased font data.** Design the text as a **1-bit dot-matrix bitmap font of your own design** (5x7 or 5x8 cell, ASCII 32-126, stored compactly, about 760 bytes for 95 glyphs), drawn scaled by an integer factor (x2, x3) with an LCD-style dot gap. At most two sizes (scale factors) of the same font.
* Flash is scarce (the device image has only a few KB spare today): the whole new view including font data should target **<= 16 KB of code+data**, and the font itself <= ~2 KB. State the real numbers you measure in `REPORT.md` (for example the size of the compiled objects or the wasm code section difference against `original\`).
* No heap, no STL, no exceptions/RTTI. Keep the existing `-fno-exceptions -fno-rtti` build.
* The view may keep its own `uint16_t` RGB565 framebuffer; make the framebuffer pointer what the adapter returns.

## Design brief

Visual language (your decisions, document them in `docs/UI_M1_DESIGN.md`): a backlit LCD panel inside a dark bezel with a faint dot grid; an authentic-feeling LCD palette (dark pixels on a pale blue-green backlight is the classic look; offer a dark-mode alternate as a theme switch constant); inverse-video (light text on dark cell) for the selected item/cursor; slider-style value bars; modern touches welcome (crisp 1-pixel dividers, subtle bezel gradient, clean icons) but keep it pixel-honest, not photoreal. Branding "KORX" only.

Screens to implement and render (one PNG each, plus anything extra you think helps):

1. **Program play**: bank/number (for example `A00`), the program name very large in the dot-matrix font, mode label, OSC mode / assign / multisound names for OSC 1, level meters, voice-use indicator, `*` edited marker, `CMP` when comparing.
2. **Combination play**: name and number, and an **eight-timbre strip**: for each timbre its program number/name (short), level, and whether it is on/muted. Read the timbre bytes from the combination block (offset layout is in `m1_params.cpp`'s combination pages: timbre t's parameters are at `offset + 11*t`). Put the helpers in `src/m1ui_extras.h`.
3. **Edit page, four parameters**: page title, four columns each with the parameter name, value text and a slider bar, with the "current" column drawn in inverse video.
4. **Envelope page** (VDF 1 EG or VDA EG): draw the **actual envelope shape** from the segment time/level parameters above the four values (a modern step beyond the original), integer maths only.
5. **Effects page**: effect type name and its parameters.
6. **Filter (VDF) page**: if feasible, a small response-curve preview from cutoff/intensity.
7. A page title strip / page navigation indicator (the model's `groupName`, `group`, `nGroups`) and the six soft-key captions (from `m1ui::captions`).

Not required now (list as future work in the design doc): Write dialog, Global page layout, Disk, Sequencer, numeric keypad entry.

## Process and verification

1. Read the files, then design, then implement. Prefer one solid pass over many tiny ones.
2. Build with `build.bat`; render every screen with `shot.mjs` into `shots\` using commands you put in `render_shots.bat` (the events needed to reach each page, using the ids above; discover valid pages with the model).
3. You cannot look at images, so verify numerically in a script (for example with Node: decode your own PNGs or read the framebuffer through `vibe_wasm_draw`): not blank, text inside its cell (no overlap or clipping beyond the screen), at least two distinct colours per screen, values changing after an encoder event. Report honestly what you could and could not verify.
4. Write `REPORT.md` in `C:\build\korx-m1\work\codex14` containing: files created, how to rebuild and render, what is verified, what is not, flash and size numbers, any deviations, and open questions. A human will review the renders and send feedback, so keep the design doc and code readable.

Stop when `REPORT.md` is written.
