# ROM-free build (milestone 1)

A build of the engine with **no Korg data of any kind**: every table is generated from formulas, the sounds are a synthetic sample set, and the eight programs and four combinations are original. The wasm is about **1.4 MB** (the Korg-sample build is 5.7 MB) and contains no ROM-derived bytes. The engine is PicoFaceM1's public GPL-3 source, compiled from that source (not from prebuilt objects).

## What was written (all code in the engine's own C++ style, no STL, no heap)

| File | What | Check |
|---|---|---|
| `src/free/m1free_tables.h` | pitch step tables, 4096-phase 8-tap interpolation kernel, LFO rate, envelope rise/fall rates, velocity curve, pan words, balance curve, delay start, just-intonation scale, and the filter coefficient tables (4 x 4096), all from formulas | `tests/free/tables_test.cpp`: 36 checks; the kernel's DC sum is exactly 1226 at all 4096 phases |
| `src/free/m1free_samples.h` | 8 band-limited waveforms (sine, saw, square, triangle, organ, 25% pulse, soft saw, noise), 4 variants each so the playback step stays under the engine's cap, 100 multisounds, one drum | `tests/free/samples_test.cpp`: 7 groups, every cycle peaks at exactly 4096 |
| `src/free/m1free_patches.h` | 8 original programs and 4 combinations, written byte by byte from the offsets in `m1_params`, every program on "No Effect" | `tests/free/patches_test.cpp`: 84 checks |
| `src/free/m1free_fx.h` | effects tables, first batch (see Honest limits) | `tests/free/fx_tables_test.cpp`: 30 checks |
| `src/free/m1free_patches_fx.h` | six original effect programs (chorus, flanger, phaser, tremolo, overdrive, stereo delay) | `tests/free/patches_fx_test.cpp`: 55 checks; `tests/free/fx_render_test.cpp`: engine render checks |
| `src/free/m1free_fx_reverb.h` | reverb and early-reflection tables | `tests/free/fx_reverb_test.cpp`: 32 checks incl. the delay-RAM budget for every type |
| `src/free/m1free_patches_fx2.h` | five original reverb programs (14-18) | `tests/free/patches_fx2_test.cpp`: 40 checks; `tests/free/fx_render_test2.cpp`: 20 render checks incl. reverb tails |
| `src/free/m1free_all.h` | ties the generators to `m1::RomTables` | n/a |
| `src/free/hub_adapter_free.cpp` | the Hub adapter with every ROM reference removed | `grep m1rom` finds nothing |

## Measured results (`tools/pitch_test.mjs`, `tools/levels.mjs`)

* **Pitch:** measured with a windowed FFT and parabolic peak interpolation on the sine program (`tools/pitch_fft.mjs`): **every note from 36 to 108 is within 0.85 cents of equal temperament (most within 0.4)**, after calibrating the sample tuning (`tune128` 3 -> 2) once the constant -0.78 cent offset (2 periods in a 239-word loop = 261.51 Hz) was identified. An earlier autocorrelation test (`tools/pitch_test.mjs`) reported -2 to -3 cents at the top notes; that was a bias of the method at short periods, not an engine error. The 16' bass program is measured an octave lower, as designed.
* **Level:** a single note at velocity 0.8 peaks at -9.7 dBFS, matching the Korg-sample build (-9.6). Sustained 4- and 8-note chords peak near full scale and are limited (see `SOUND_ISSUES.md`).
* **Stress suite:** 33 of 33 pass (the Korg-sample builds had one known failure).
* **Imports:** none (`imports []`).

## Honest limits

* It does **not sound like an M1.** The sounds are plain synthetic waves; the filter, envelope, LFO and balance tables are modelled approximations (see the table in the conversation notes / `CLEAN_ROOM_SPEC.md` section 14 for what needs measuring on a real M1).
* **Effects: 25 of 33 types have tables.** Batch 1 (`m1free_fx.h`: LFO shapes and speed law, delay damping, EQ shelves, tremolo/exciter constants) enables stereo delay, cross delay, chorus 1/2, flanger 1, cross flanger, phaser 1/2, tremolo 1/2, equalizer, overdrive, distortion and exciter (14 types). Batch 2 (`m1free_fx_reverb.h`: four damped combs + all-passes + early-reflection taps, with reverb time from the textbook RT60 relation) enables Hall, Ensemble Hall, Concert Hall, Room, Large Room, Live Stage and Early Reflection 1-3 (9 types). Batch 3 (`m1free_fx_rotsym.h`: three-voice Symphonic Ensemble line offsets and LFO rates; Rotary Speaker crossover filters, horn/drum delays, microphone taps and speed ramps) enables Symphonic Ensemble and Rotary Speaker (2 types; `tests/free/rotsym_test.cpp`, 10 checks pass: stereo balance, differs from dry twin, amplitude modulation 1.28x slow / 1.36x fast). **Render-tested through the real engine against a dry twin (`tests/free/fx_render_test*.cpp`):** stereo delay (echo energy 2.5x), chorus 1, flanger 1, phaser 1, tremolo 1 (modulation ratio 2.8 vs 1.07), overdrive, Hall, Room, Live Stage, Concert Hall (tails ring after release; dry twin silent) and Early Reflection 1. The other enabled types are table-enabled but not yet render-tested. **Robustness:** `tests/free/fx_all_types_test.cpp` runs all 34 effect types with five parameter extremes (170 cases): every case is finite and bounded. **Behaviour of the remaining 8 types:** the five delay+modulation combinations (Dly/Dly, Dly/Chorus, Dly/Flanger, Dly/Phaser, Dly/Tremolo) already produce sound with the batch-1 tables (correctness not yet checked); the three reverb combinations (Dly/Hall, Dly/Room, Dly/E.Ref) are silent until their tables are designed. **Known engine quirk:** a modulated delay read goes negative when the LFO value dips below zero, the quadratic read then extrapolates and, with feedback above about 0.25, rings at Nyquist (peak 9.6 measured on the first Flange Saw). Flange Saw therefore uses the unipolar triangle wave and feedback 25 (peak 0.29); user edits with a sine wave and high feedback can still buzz.
* **No drum kits** (owner decision, 2026-11-07: not needed). One noise hit exists but no kit maps it to keys.
* **Playback range:** the engine caps the playback step at 3.0, so each waveform needs one variant per octave; this set covers keys up to 127 but has been pitch-tested only from note 36 to 84.
* **Offline checks only.** Nobody has listened to this build.

## Provenance of the code

Written by a local qwen3.8 model from short briefs (`docs/ai-briefs-free/`), with exact-match patch scripts and independent verification by the orchestrating assistant. One file (the sample generator) failed twice on qwen (it truncated samples before normalising, then ran away) and was written by Codex from the same spec (20,469 tokens), then re-verified independently. Two bugs found by the tests were in the briefs, not the model: a wrong kernel normalisation formula, and a test that expected an octave-low program to sound at its played pitch.
