# The browser sound problem: root cause found (resampler buffering bug)

**Status (2026-11-07): the main cause was found and fixed.** It is a source-level indexing bug in the 31.25 kHz → host-rate resampler, found by an outside reviewer reading this repository and then **verified here against the native engine**. Earlier builds (every `.wasm` produced before this fix, including `korx-m1-hub` and `korx-m1-lite`) have it. Remaining quality questions (limiter character, the Lite codec) should be judged only on builds made *after* the fix.

## The bug

```cpp
void ensureFrames(int needed) {
    int have = g_framesRendered - (int)g_readPos;   // frames buffered AHEAD of the read position
    while (have < needed) { /* render 64 frames, have += 64 */ }
}
...
ensureFrames(i0 + 4);                                // needed = an ABSOLUTE source index
```

`have` is a *relative* lookahead; `needed` is an *absolute* index that grows forever. So the requirement "lookahead ≥ i0 + 4" got ever larger and the adapter rendered the engine further and further ahead, about **twice as fast as it was consumed**. Two consequences:

1. **Every envelope, LFO and effect ran at double speed.** Notes decayed in half the time.
2. The 2,048-frame ring can only hold the newest 2,048 frames; once `i0` passed ≈ 2,040 source frames (≈ 65 ms), the interpolator was reading ring slots that had already been overwritten by later audio.

The first ~65 ms (the attack) was right, then the sound deteriorated. Peak/RMS tests never noticed because levels stayed plausible. The bug was present from the first Hub build, so no earlier build was a fair test of the engine.

**Fix** (`src/adapter/*.cpp`): compare absolute positions with absolute positions:

```cpp
void ensureFrames(int needed) {
    while (g_framesRendered < needed) { /* render 64 frames into the ring */ }
}
```

## Evidence (`tools/drift.mjs`, RMS of a 50 ms window of a held middle-C, Piano programme 1)

| t (s) | native engine (reference, 31.25 kHz) | **before fix** | **after fix** |
|---|---|---|---|
| 0.4 | 0.102 | 0.070 | 0.107 |
| 0.8 | 0.069 | 0.028 | 0.072 |
| 1.5 | 0.030 | 0.013 | 0.031 |

Before the fix the curve was the correct one *compressed in time by exactly 2×*: `before(t) = after(2t)` at every sampled point (for example before(0.1 s) = 0.120 = after(0.2 s); before(1.5 s) = 0.0130 = after(3.0 s) = 0.0134). After the fix it follows the native engine within a few percent. (The native reference note ends near 3 s, so the 3 s row is omitted.) Peak/clipping measurements are essentially unchanged by the fix (`tools/levels.mjs`).

## One more observation, not a bug

The cubic interpolator outputs the value between `ring[i0+1]` and `ring[i0+2]` rather than between `ring[i0]` and `ring[i0+1]`: a constant one-sample (32 µs) advance. It does not distort anything and is left as is.

## Still worth doing

* **Listen to a post-fix build before touching anything else.** The soft limiter, the velocity-driven loudness and the Lite codec are all separate, smaller questions that were mixed into the complaint. The earlier measurements below remain correct but were taken on builds that had the bug.
* The remaining checks proposed earlier (resampler isolation test against an offline high-quality resample, calibration against a native firmware's levels, headroom before the limiter) are still reasonable A/B experiments, in that order.

---

# Earlier investigation (kept for the record; measured on pre-fix builds)


**Report (human tester, 2026-11-07):** "everything clips, the sounds are terrible", and the PicoFace hardware "sounds so much better". This was reported on `korx-m1-ui.wasm` before the fixes below, and again after the full-quality / limiter build was supplied. **The cause has not been found.**

Nobody on the project (the orchestrating AI, the local model, the Codex fallback) can listen to audio. Everything below is measurement; the tester's ears are the only ground truth.

## What was measured (`tools/levels.mjs`)

Four cases per build, 1.5 s each, 48 kHz, keys sent as Hub `EV.KEY` events with velocity as a float 0..1.

| Build | single note v0.8 | 4-note chord v0.8 | 8-note chord v1.0 | notes |
|---|---|---|---|---|
| Original full-PCM wasm (`hub3`), **old integer ABI** | peak 0.315 (−10.0 dBFS) | 0.660 (−3.6) | 0.027 (−31) | velocity was truncated by the ABI (see below): every press was 100 or 1, so these numbers are not representative of real play |
| UI build, full PCM, **no limiter** | 0.381 (−8.4) | 0.919 (−0.7) | **2.029 (+6.1)**, 2.35 % of samples ≥ full scale | real velocities; hard clipping on a loud chord |
| UI build, Lite 3-bit PCM, no limiter | 0.375 (−8.5) | 0.940 (−0.5) | **2.019 (+6.1)**, 2.32 % over | same clipping, plus codec noise |
| UI build, full PCM, **with Pico soft limiter** | 0.329 (−9.6) | 0.689 (−3.2) | 0.986 (−0.1), 0 over | no sample reaches full scale, but the loud chord is heavily limited (RMS −10 dBFS) |

So a *single* note is not hot (≈ −9 dBFS), but the Hub's on-screen keys send velocity up to 1.0 on the lower half of a key, and sixteen voices sum to well over full scale. With the limiter nothing hard-clips any more. **The tester still hears clipping and bad sound**, so either something else is wrong, or the limiting itself sounds bad at those levels.

## Already found and fixed (each was a real defect, none was the whole story)

1. **Velocity ABI.** The Hub's native firmwares export `vibe_wasm_event(i32 type, i32 id, i32 down, f32 value, i32 midi)` and `vibe_wasm_init(f32 sampleRate)`. The first builds declared `value` as an integer, so key velocity (a float 0.2–1.0) was truncated to 0 → "default 100", or to 1 at full strength. Fixed in the UI build. (This also silently dropped touch-drag encoder deltas.)
2. **No output limiter.** PicoFace's `M1_Bridge::fillBufferI32` multiplies by `gain*2` and soft-clips above 0.9 (knee). The browser build sent the raw float sum out. Fixed: the same soft clip is applied.
3. **Wrong sample bank for a browser.** One tested build used the FM-1 "Lite" 3-bit ADPCM bank (≈ 22–29 dB SNR; gritty by construction). The browser has no flash limit, so the main build now embeds the full 4 MiB PCM and all 100 programs / combinations.
4. **Lite-bank loop-lookback bug** (FM-1 only): looped waveforms read samples *before* their region start; the bank builder only stored those when `guard_words > 0`.

## What has NOT been ruled out (ranked guesses, with a way to test each)

1. **The 31.25 kHz → host-rate resampler.** The PicoFace core runs *at* 31.25 kHz, so the hardware never resamples. This adapter resamples with a stateful cubic Hermite interpolator and a 2048-sample ring, filled in 64-frame engine blocks and read in 128-frame host blocks. A discontinuity at block boundaries, a read-position drift, or imaging above 15.6 kHz would all sound like crackle/"clipping" on every patch. **Test:** render a steady sine-ish note at 31.25 kHz directly (no resampling), then through the adapter, and compare spectra; scan the adapter output for sample-to-sample jumps larger than the source allows.
2. **Calibrating against a known-good native firmware.** Run the same level measurement on OBERVAVE and S1VAVE (they are in the Hub's firmware list) to see what peak level the Hub expects, and whether the Hub applies its own gain or clips differently from a plain ±1.0 expectation. **Test:** extend `tools/levels.mjs` with a native-firmware case.
3. **The limiter's character.** A static soft clip on a sum that peaks at +6 dBFS is a very strong waveshaper; it may simply sound distorted on chords even though no sample exceeds 1.0. **Test:** add headroom (−6 to −12 dB before the limiter), or replace it with a look-ahead limiter, and have the tester compare.
4. **Effects at 31.25 kHz.** Reverb/chorus/rotary run in the engine on 16-bit delay RAM; if the tester is on an effects-heavy program the wet path may be the harsh part. **Test:** render the same note with effects bypassed (`setBypassFx(true)` exists in the engine).
5. **Float behaviour in wasm vs the RP2350.** Unlikely; earlier native-vs-wasm renders matched to ~14 LSB.
6. **Hub-side:** sample rate (44.1 vs 48 kHz), the Hub's own volume knob, and buffer size (`buffer_ms`) are untested variables.

## What the tester can tell us that would narrow this fastest

* Which file, and the programme number and name that sounds worst.
* A single key at medium velocity vs a chord at full velocity: does a *single quiet note* already sound wrong?
* The character: **crackle/clicks** (suggests the resampler or block boundaries), **fuzz/saturation that grows with loudness** (suggests gain/limiter), **constant hiss or graininess** (suggests the sample data/codec), **thin/harsh top end** (suggests imaging from the resampler).
* Whether turning the Hub's volume knob down changes it.

## Suggested next experiment (≈ 1 hour)

Build three variants of the same program and level: (a) engine output at 31.25 kHz written straight to a WAV, (b) the adapter output at 48 kHz without the limiter, (c) with the limiter. Compare (b) against (a) upsampled with a high-quality offline resampler. If (b) differs from that reference by more than rounding noise, the resampler is the culprit; if (b) is clean, the problem is level/limiter/effects.
