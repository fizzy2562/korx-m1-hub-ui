# Technical findings

Things that were not documented anywhere and cost time to find. Useful to anyone writing a Firmware Hub firmware or a UI for one.

## Firmware Hub ABI (read from the Hub's own `js/panel.js`, `host.js`, `worklet.js`)

* A `.wasm` firmware is instantiated in an AudioWorklet with only `env` imports (`vibe_host_log`, `vibe_host_request`, `vibe_host_store`). **WASI imports fail.** Link with `-mexec-model=reactor` and make sure the result has `imports []`.
* **Signatures matter.** Native firmwares export `vibe_wasm_event(i32 type, i32 id, i32 down, f32 value, i32 midi)` and `vibe_wasm_init(f32 sampleRate)`. Declaring `value` as an integer silently truncates velocity (0.2–1.0 becomes 0 or 1) and fractional encoder deltas from touch-dragging. Check yours with `tools/wasm_sig.mjs`.
* Required exports: `memory, vibe_sdk_version() (1..8), vibe_wasm_init, vibe_wasm_io() → float[1024] (in1@0, outL@256, outR@512, in2@768), vibe_wasm_audio(n≤128, flags), vibe_wasm_event, vibe_wasm_draw() → uint16[480×320] RGB565, vibe_wasm_leds()` (384 bytes).
* Optional: `vibe_wasm_labels()` (25 × 8 bytes of ASCII captions), `vibe_wasm_pad_leds`, `vibe_wasm_param_count/info/value/set` (mixer parameters), `vibe_wasm_sysex_in`, `vibe_wasm_stage_*` (a second, larger display), editor exports.

### Event types and control IDs

`EV`: PAD 1, KEY 2, BUTTON 3, ENCODER 4, MIDI 5, PEDAL 6, CLOCK 7, TRANSPORT 8, TOUCH 16.
`BTN`: F1–F6 = 0–5, SHIFT 6, OCT− 7, OCT+ 8, ENTER 9, K1–K6 = 10–15, RETURN 16, HOME 17, LEFT 18, MENU 19, RIGHT 20, REC 21, STOP 22, PLAY 23, DATA_PUSH 24.
`ENC`: E1–E4 = 0–3 (the four knobs above the screen), A 4, B 5, DATA 6 (the big jog). A button's `value` is always 0; pressed is `down`.
Keys send velocity 0.2–1.0 (a float; it depends on where on the key you press); pads send 0.25–1.0.

### LED table (`vibe_wasm_leds`, 384 bytes)

Read as `uint32 u[96]` and `float f[96]`: `u[0..15]` pad RGB (`0xRRGGBB`), `f[16..31]` pad brightness, `u[32..56]` button RGB (25 buttons), `f[57..81]` button brightness.

### The "VIBE FM layout" standard (what the Hub treats as native)

A firmware that returns **non-empty captions from `vibe_wasm_labels()`** is treated as native: the Hub then makes the 480×320 display **touch-clickable**. The soft keys sit at y 282–320 (six 80 px fields = F1–F6) and four knob columns (x 10–470, 115 px each) above them map to encoders 1–4; clicking a soft key presses F-n and dragging or scrolling a column turns that encoder. Captions also appear on the key tooltips and under the device.

Reference look (OBERVAVE, see `docs/images/reference_obervave_native.png`): header (page title violet left, preset centre, indicators right) to y≈34; a row of rounded page chips y 41–67; four arc knobs, centres x = 67 + 115·i, arc centre y≈127, outer radius 40, thickness 4, **track from 225° to 495°** (gap at the bottom; angles clockwise from 12 o'clock), name above at baseline ≈88, large value below at baseline ≈197; status line ≈222; a thin level line ≈255; six rounded soft-key buttons y 286–316 (w 73, gap 6, from x 6). Palette used here: background (11,17,25), chip (22,32,48), track (31,43,61), accent blue (77,163,232), violet (183,148,246), label grey (138,154,176).

## PicoFaceM1 building blocks that were reusable

* `m1_params.{h,cpp}`: *every* editable parameter described once (byte offset, range, kind) with display text and a 0..1 bar position: `programPages()`, `combiPages()`, `effectParams(type)`, `paramStep()`, `paramFormat()`, `setNameSources()`. It needs only `stdio`/`string.h` and compiles into the wasm unchanged.
* `M1_Controller::buildEdit` is the page-flattening rule set (ported to `m1ui_model.h::buildPages`): Double-only pages hidden unless `block[10]==1`; effect pages take their parameters from `effectParams(type)` at `fxBase+9+8k` (`fxBase` 38 for programs, 11 for combinations); timbre pages repeat eight times at `+11·t`.
* The engine plays from the controller's **persistent edit buffers** (`playProgram(ptr)`/`playCombi(ptr)`, 143 / 124 bytes); ordinary parameter edits need no call, only effect edits call `refreshEffects(buf+fxBase)`.
* The PicoFace core runs *at the engine's 31.25 kHz*; `M1_Bridge` applies `gain*2` then a soft clip (knee 0.9). A browser build runs at 44.1/48 kHz and therefore has to resample.
* Polyphony: 16 TG channels; a Double program takes two per note, so Single = 16 notes, Double = 8.

## The Lite bank and the loop-lookback bug

For the FM-1 the 4 MiB ROM is replaced by a compressed bank of the programs that fit. Looped waveforms (notably the octave-halved "DWGS" chain: 257, 129, 65, 33, 17, 9, 5, 3 words) have a loop address *below* the region start, so the engine reads samples before the region. If the bank stores only the nominal region those reads return zeros. `tools/lite-bank/bank_build.py` extends such regions backwards **only when `guard_words > 0`**; building with the default 0 produced seven programs at −18…+2 dB SNR. The fix is always build with a guard ≥ 1 (the guard width itself barely changes size). Diagnosed with a traced copy of the bank reader that logs every zero-filled gap.

## Drawing-library bugs worth knowing about

* `arc()`: an angle mapping that was mirrored top-to-bottom (`90 − atan2` instead of `90 + atan2` for a y-down screen).
* `mix(a, b, t)`: dividing by 256 while the weights sum to 255 darkens every anti-aliased edge at full alpha (a 5-bit channel value of 3 became 2). Use `(… + 127) / 255`.
* `fillRoundRect`: the centre rectangle was never filled.
* An object literal with a duplicate `type` key in the screenshot harness made every scripted event a silent no-op: a good reminder to check that a test tool can *fail*.

## Process notes

* qwen3.8 (27B, local) is good at writing code from a precise specification and poor at tests and numerics; it fails silently. Every change here was checked by running it and looking at the result.
* One call produces about 9,000 output tokens: a single large file per call, or it truncates mid-line. Small exact-match patch scripts (assert each target occurs exactly once; normalise CRLF) were far more reliable than rewriting files.
* The Windows build box writes CRLF; patch scripts must normalise line endings.
