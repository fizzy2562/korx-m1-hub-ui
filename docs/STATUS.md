# Status: where we are (2026-11-07)

## Goal

Play the Korg M1 engine from PicoFaceM1 (a float re-implementation of the M1's voices, filters, envelopes and 33 effects) in two places: the **Firmware Hub web player** as a `.wasm` firmware, and natively on the **M-VAVE FM-1**. Then give it a front-panel UI that looks and feels like a native VIBE firmware.

## Summary table

| Piece | State | Evidence |
|---|---|---|
| Engine → WebAssembly (Hub ABI, full 4 MiB PCM embedded) | Built | Loads in the real Hub player (headless Edge): held note, release and combi audio pass. Native-vs-wasm renders compared on the earlier build: within ~14 LSB (before resampling to the host rate) |
| UI (header, page tabs, 4 arc knobs, status line, meters, 6 soft keys, K1–K6 page LEDs, touch) | **Works** | Rendered with scripted events and inspected as images; shown inside the real Hub. See `docs/images/` |
| Parameter editing (all M1 program/combination edit pages, via PicoFaceM1's tables) | Works | Encoder edits change values live; `*` edited marker; Compare |
| **Sound quality in the browser** | **Root cause fixed; tester: Lite build "much better"** | A resampler buffering bug ran the engine at 2× speed and corrupted the ring buffer; fixed and verified against the native engine's decay curve. See [SOUND_ISSUES.md](SOUND_ISSUES.md) |
| "Lite" sample bank for the FM-1 (flash ceiling) | Built, quality compromised | 3-bit ADPCM, 51 programs / 19 combis, median ~29 dB SNR vs original, six programs worse than 15 dB. Hearing it is believed to be gritty |
| FM-1 native firmware image | Built, **never flashed** | Passes the FM-1 build tool's checks (image 576,392 of 581,564 bytes). Plays a 4-note test pattern; no UI; no device run, no CPU measurement |
| Test suites | 32/33 stress, 32/40 contract | The failures are known: a small residue after All-Sound-Off, and contract tests that assume unrestricted program stepping |

## Timeline in brief

1. **Engine to wasm.** The Pico engine plus its generated ROM tables and the PCM, linked with an adapter that speaks the Hub's `vibe_wasm_*` ABI (no imports; exports `memory, vibe_sdk_version, vibe_wasm_init, vibe_wasm_io, vibe_wasm_audio, vibe_wasm_event, vibe_wasm_draw, vibe_wasm_leds`, later `vibe_wasm_labels`). The engine renders at the M1's own 31.25 kHz; the adapter resamples to the host rate with a cubic Hermite interpolator.
2. **Native FM-1 port.** The float engine cannot link on the FM-1's toolchain (no soft-float library), so an integer voice engine, integer effects and an integer polyphase resampler were written and checked against the float reference. The result links and passes the build tool's checks but has not run on hardware.
3. **Fitting the samples.** The FM-1 has roughly 350–600 KiB for PCM against an M1 ROM that references 98.5 % of 4 MiB. Samples were compressed with IMA-style ADPCM at 4, 3 and 2 bits and a planner picks which programs to keep. See [FINDINGS.md](FINDINGS.md#the-lite-bank-and-the-loop-lookback-bug) for the one real bug found there (looped waveforms read before their region start).
4. **UI.** Studied the Hub's "VIBE FM layout" standard and the OBERVAVE firmware, wrote a drawing library and font pipeline, ported PicoFaceM1's edit-page logic into a small model, wrote the view, wired it into the adapter, and checked it by rendering frames with scripted events.
5. **Sound complaints.** First "sounds clipped", which led to the velocity ABI bug, the missing output limiter and the Lite-codec-in-a-browser mistake being found and fixed. The tester still reports clipping and bad sound, so the real cause has not been found.

## What was verified, and what was not

Verified (with a command and an output you can re-run):

* the UI renders as designed and responds to buttons, encoders and program changes (`tools/shot.mjs`),
* output peaks and clipped-sample counts for four playing cases (`tools/levels.mjs`),
* the Hub-facing ABI matches the native firmwares (`tools/wasm_sig.mjs`: `vibe_wasm_event(i32,i32,i32,f32,i32)`, `vibe_wasm_init(f32)`).

**Not verified:** anything about how it *sounds*. No model can listen. The only listener is the human tester, and their verdict is negative.

## Open items, in priority order

1. **Listen to the post-fix full-quality build** (the tester has confirmed the post-fix *Lite* build sounds much better; all earlier builds had the resampler bug), then judge the limiter and headroom separately (see [SOUND_ISSUES.md](SOUND_ISSUES.md)).
2. Investigate the small All-Sound-Off residue (the original build had none; the UI build leaves ~-36 dB).
3. Port the UI to the FM-1 firmware: needs a 2- or 4-bit font (the 8-bit font data is ~42 KB; the image has ~5 KB spare).
4. Flash-test the FM-1 image (rescue: hold OCT− while powering on), then measure CPU and flash-read cost.
5. UI v2 ideas: a HOME overview page with envelope/filter curve previews, 16-pad quick program select with LEDs, edited-parameter highlighting, drawing the EG shapes, programme categories and colours.

## How to build

Not reproducible from this repository alone, on purpose:

1. Obtain your own M1 ROM set (the file names PicoFaceM1 expects) and run PicoFaceM1's `tools/m1_extract` to produce `m1_rom_tables.h` and `m1_pcm.bin` (4 MiB, 16-bit).
2. Build the engine objects (`m1_engine.o`, `m1_fx.o`) from PicoFaceM1's `src/m1_engine/` with Zig 0.17: `zig c++ -c -target wasm32-wasi -O2 -fno-exceptions -fno-rtti`.
3. Embed the PCM: a one-line Zig file `export const korx_pcm_data: [4194304]u8 align(4) = @embedFile("m1_pcm.bin").*;` compiled with `zig build-obj pcm.zig -target wasm32-freestanding -O ReleaseSmall`.
4. Link (see `build/build_hub_ui.bat` for the exact flags): the adapter, `src/ui/m1_params.cpp`, the engine objects and `pcm.o`, with `-I src/ui -I <dir with lite_sets.h> -I <PicoFaceM1 include> -I <dir with m1_rom_tables.h>`.
5. Check: `node tools/wasm_imports.mjs out.wasm` must print `imports []`.
