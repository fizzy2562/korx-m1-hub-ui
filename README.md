# KORX-M1 Hub UI

Experiments in running the **PicoFaceM1** Korg M1 engine ([Michi71's PicoVintageSynthCollection](https://github.com/Michi71/PicoVintageSynthCollection)) as a **WebAssembly firmware for the VIBE FM Firmware Hub**, with a native-looking 480×320 touch UI, and as a firmware for the **M-VAVE FM-1**.

> ## Status: unfinished, and the sound is NOT good yet
> A tester reports that every sound clips and sounds bad in the browser build, even after a soft limiter and full-quality samples were added. That problem is **unresolved**; see [docs/SOUND_ISSUES.md](docs/SOUND_ISSUES.md) for what was measured, what was ruled out, and what to try next. The UI and the plumbing work; the audio quality does not meet the bar set by the PicoFace hardware. Treat this repository as a progress record and a code base to build on, not a finished product.

## What is in this repository

| Path | What |
|---|---|
| `src/ui/` | The UI: `gfx.h` (RGB565 drawing library: anti-aliased text, rounded rectangles, arcs), `m1ui_model.h` (pages, editing, program/combi stepping, button and encoder handling), `m1ui_view.h` (the screen), `font_data.h` (baked Ubuntu Medium, three sizes), `m1_params.{h,cpp}` (the M1 parameter tables, **from PicoFaceM1**, GPL-3.0) |
| `src/adapter/` | The Firmware Hub adapter (`hub_adapter_ui.cpp`): the Hub's `vibe_wasm_*` ABI around the Pico engine, 31.25 kHz → host-rate resampling, the soft limiter, the UI hooks. `hub_adapter_baseline.cpp` is the earlier UI-less version. `lite_sets_*.h` list which programs exist in each build |
| `tools/` | `shot.mjs` (render the wasm's screen to a PNG with scripted button/encoder events), `levels.mjs` (peak / clipping measurements), `bake_font.py` (font baker), `wasm_sig.mjs` / `wasm_imports.mjs` (ABI inspection), `lite-bank/` (the sample-compression and bank tools for the FM-1 build) |
| `build/` | The Windows `zig c++` build scripts used on the build box |
| `docs/` | [STATUS](docs/STATUS.md) (where we are), [FINDINGS](docs/FINDINGS.md) (the technical discoveries: Hub ABI, control IDs, layout standard, bugs), [SOUND_ISSUES](docs/SOUND_ISSUES.md) (the open audio problem), [CLEAN_ROOM_SPEC](docs/CLEAN_ROOM_SPEC.md) (a behaviour-only specification of the M1 engine, for an independent implementation), [REA_PLAN](docs/REA_PLAN.md) (how to use REA to confirm and extend it), [THIRD_PARTY](docs/THIRD_PARTY.md), `images/`, and `qwen-briefs/` (the prompts the code was generated from) |

## What is deliberately NOT in this repository

Everything that contains or is derived from **Korg's ROM data** is left out, and `.gitignore` blocks it:

* the `.wasm` builds (they embed the 4 MiB sample ROM),
* the FM-1 `.fwsc` firmware package and the compressed sample banks,
* the PCM / ROM images and the generated `m1_rom_tables.h`,
* the ROM archives themselves (`korgm1.zip`, `Korg M1 - Rev 19 Firmware.zip`, `M1_Sounds.zip`).

To build it you must supply **your own legally obtained M1 ROM set** and generate the tables with PicoFaceM1's `tools/m1_extract`. See [docs/STATUS.md](docs/STATUS.md#how-to-build).

## Screenshots

The old screen, then the new one rendered by the headless harness, then inside the real Firmware Hub:

| | |
|---|---|
| ![target](docs/images/reference_obervave_native.png) | ![osc page](docs/images/ui_osc1_page.png) |
| The target: a native VIBE firmware (OBERVAVE, GPL-3.0) | Program mode, OSC 1 page, edited program (`*`) |
| ![combi](docs/images/ui_combi_page.png) | ![meters](docs/images/ui_playing_meters.png) |
| Combination mode, Timbre 1 page | A held note: voice dot and L/R meters |

![in the hub](docs/images/ui_in_real_hub.png)

## How it was made

The code was written by a local LLM (qwen3.8 27B, run on a build box) from short written briefs (see `docs/qwen-briefs/`); an AI orchestrator reviewed every diff and verified with renders and measurements. Changes were applied with small exact-match patch scripts rather than whole-file rewrites, because a whole-file rewrite of a large file truncated. No audio was ever listened to by the models, so **sound quality has only been measured, never heard, except by the human tester**.

## Licence

GPL-3.0-or-later, because `src/ui/m1_params.*` come from PicoFaceM1 (GPL-3.0-or-later). The baked font is under the Ubuntu Font Licence. See [docs/THIRD_PARTY.md](docs/THIRD_PARTY.md).
