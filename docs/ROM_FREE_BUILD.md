# ROM-free build (milestone 1)

A build of the engine with **no Korg data of any kind**: every table is generated from formulas, the sounds are a synthetic sample set, and the eight programs and four combinations are original. The wasm is about **1.4 MB** (the Korg-sample build is 5.7 MB) and contains no ROM-derived bytes. The engine is PicoFaceM1's public GPL-3 source, compiled from that source (not from prebuilt objects).

## What was written (all code in the engine's own C++ style, no STL, no heap)

| File | What | Check |
|---|---|---|
| `src/free/m1free_tables.h` | pitch step tables, 4096-phase 8-tap interpolation kernel, LFO rate, envelope rise/fall rates, velocity curve, pan words, balance curve, delay start, just-intonation scale, and the filter coefficient tables (4 x 4096), all from formulas | `tests/free/tables_test.cpp`: 36 checks; the kernel's DC sum is exactly 1226 at all 4096 phases |
| `src/free/m1free_samples.h` | 8 band-limited waveforms (sine, saw, square, triangle, organ, 25% pulse, soft saw, noise), 4 variants each so the playback step stays under the engine's cap, 100 multisounds, one drum | `tests/free/samples_test.cpp`: 7 groups, every cycle peaks at exactly 4096 |
| `src/free/m1free_patches.h` | 8 original programs and 4 combinations, written byte by byte from the offsets in `m1_params`, every program on "No Effect" | `tests/free/patches_test.cpp`: 84 checks |
| `src/free/m1free_all.h` | ties the three generators to `m1::RomTables` | n/a |
| `src/free/hub_adapter_free.cpp` | the Hub adapter with every ROM reference removed | `grep m1rom` finds nothing |

## Measured results (`tools/pitch_test.mjs`, `tools/levels.mjs`)

* **Pitch:** 35 of 35 cases in tune: 7 programs x notes 36, 48, 60, 72, 84. **Max error 3.3 cents, mean -1.0 cents**; none silent. The 16' bass program is measured an octave lower, as designed. The error is under 1 cent at notes 36 to 60 and grows to about -2 to -3 cents at 72 and 84 (not yet calibrated; the spec's target is +/-1 cent).
* **Level:** a single note at velocity 0.8 peaks at -9.7 dBFS, matching the Korg-sample build (-9.6). Sustained 4- and 8-note chords peak near full scale and are limited (see `SOUND_ISSUES.md`).
* **Stress suite:** 33 of 33 pass (the Korg-sample builds had one known failure).
* **Imports:** none (`imports []`).

## Honest limits

* It does **not sound like an M1.** The sounds are plain synthetic waves; the filter, envelope, LFO and balance tables are modelled approximations (see the table in the conversation notes / `CLEAN_ROOM_SPEC.md` section 14 for what needs measuring on a real M1).
* **No effects yet.** The effect coefficient table is all zeros; every generated program uses "No Effect". Reverb, chorus and the rest need their own designs.
* **No drum kits.** One noise hit exists but no kit maps it to keys.
* **Playback range:** the engine caps the playback step at 3.0, so each waveform needs one variant per octave; this set covers keys up to 127 but has been pitch-tested only from note 36 to 84.
* **Offline checks only.** Nobody has listened to this build.

## Provenance of the code

Written by a local qwen3.8 model from short briefs (`docs/ai-briefs-free/`), with exact-match patch scripts and independent verification by the orchestrating assistant. One file (the sample generator) failed twice on qwen (it truncated samples before normalising, then ran away) and was written by Codex from the same spec (20,469 tokens), then re-verified independently. Two bugs found by the tests were in the briefs, not the model: a wrong kernel normalisation formula, and a test that expected an octave-low program to sound at its played pitch.
