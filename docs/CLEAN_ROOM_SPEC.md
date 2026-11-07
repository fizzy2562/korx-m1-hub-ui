# Korg M1 engine: behaviour specification (draft v0.1)

**Purpose.** Describe *what an M1-style synth engine does*, with the aim that someone who has never seen Korg's code or ROMs can implement it in Rust or C and then check it against measurements. **Status: an incomplete draft.** It is not yet precise enough to implement from alone; §15.1 lists what is still missing. It contains behaviour, formulas and structure only: **no ROM addresses, no table contents, no sample data, no code.**

**Education only.** Written to learn how a classic sample-based (ROMpler) synthesiser is built. Nothing here distributes Korg's data.

## 0. Provenance and an honest contamination statement

Each statement carries a tag:

| Tag | Meaning |
|---|---|
| **[OS]** | Observed in the M1's firmware by earlier analysis, as documented in PicoFaceM1's `doc/ENGINE.md` (GPL-3.0). Re-verify before relying on it |
| **[MEAS]** | Measured against recordings of a real M1 (very limited: two MP3s) |
| **[MAN]** | From the owner's manual / MIDI implementation chart |
| **[ASSUMED]** | An inference with no measurement. Highest risk |
| **[MEASURE]** | A value this spec does *not* give: you must obtain it yourself (see §14) |

**Contamination.** This draft was produced by an AI assistant that had also read PicoFaceM1's engine source and `ENGINE.md`. It is therefore **not** a clean-room document by itself. To make a clean implementation, give *only this file* to an implementer who has seen none of that code (see `docs/REA_PLAN.md` for the two-lane process), and treat every **[OS]** statement as a hypothesis to confirm by measurement.

Numbers in this spec are *functional constants* (rates, scales, ranges) that determine behaviour. If you prefer, re-derive each by the measurement in §14 instead of trusting it.

## 1. System model

* Output sample rate **31,250 Hz** (stereo). All time constants below are in these samples. **[OS]**
* **16 voice channels.** A *Double* program occupies **two** channels per note. **[OS]**
* Control-rate: a voice's parameters (cutoff, amplitude, pitch, envelope state) are updated once per **650 samples (20.8 ms)**; between updates, cutoff and amplitude **ramp linearly** from the old target to the new one. Pitch changes in steps. **[OS]**; the ramp shape is **[ASSUMED]** (the hardware ramps at a CPU-set rate).
* One program = one MIDI channel of 16 voices; a **combination** layers up to 8 *timbres*, each pointing at a program, sharing the 16 channels.

## 2. Data model

Parameter layout is defined by PicoFaceM1's `m1_params` (GPL): *program* 143 bytes, *combination* 124 bytes, *global* 21 bytes, *drum kit* 4 × 210 bytes, with names in the first 10 bytes of a program/combination. Treat that file as the interface definition for patch data; it contains no Korg audio.

### 2.1 Sample-set interface (the part you replace)

The engine reads a **sample set** through this abstract interface. Supplying your own set is how you avoid any Korg data.

* **Multisound table:** 100 entries. Entry *i* has a name, a kind (*PCM* or *DWGS*, see below) and an ordered list of **splits**.
* **Split:** `{ top_key, pan, cents, root_key (+ "NT" flag = no transposition, play at root), level_trim, cutoff_trim, decay_trim, sample }`. A split covers keys from just above the previous split's `top_key` to its own. **[OS]**
* **Sample:** `{ tune (1/128 semitone), start, loop_start, end, loop_flag }`, addresses are 20-bit word addresses into a flat array of **16-bit linear PCM**. **[OS]**
* **DWGS multisounds** (single-cycle digital waveforms): a *single* split list shared by 23 multisounds; multisound *m* is selected by adding a per-wave word offset (a constant stride per wave) to every address. **[OS]** Define your own stride.
* **Drum sounds:** 44 samples with the same record. **[OS]**

## 3. Oscillator (sample playback)

* **Phase** is a fixed-point address, 20.12: integer word address plus 12 fractional bits. **Step** is 4.12 (a 16-bit value: 0x1000 = 1.0 words per output sample), clamped to **0x2FFF**. **[OS]**
* **Output sample** = weighted sum of **8 consecutive source samples** around the read point, with weights chosen by the 12-bit fractional phase from a **4096-phase × 8-tap** table (a polyphase interpolator). **[OS]** Any good 8-tap windowed-sinc polyphase table with unity DC gain is acceptable; **[MEASURE]** the original's passband flatness and stopband rejection (§14.1) if fidelity matters.
* **Looping.** A looped sample repeats `[loop_start, end)`: for read addresses ≥ `end`, taps wrap by `(end − loop_start)`. The word *at* `end` is **not** part of the loop. **[OS]**, checked against all looped splits. For the 8 taps that straddle the loop point, taps *before* `loop_start` read the data linearly (it normally continues the loop anyway). **[ASSUMED]** for the hardware.
* **One-shot** samples stop when the phase passes `end`; the filter then rings out until the next control update. **[OS]**

## 4. Pitch

All pitch arithmetic is in **1/128 semitone**:

```
d = (n_e << 7) + osc2_detune + mg_pitch + (bend + aftertouch + detune + master_tune) + pitch_eg + 3 − root
step(4.12) = S[d >> 7] + (S[d >> 7] * F[d & 127] >> 16)
```

* `n_e` = played key + 12 × octave + combination transpose (+ interval for OSC 2), folded into 0..127. **[OS]**
* `S[]` is the step for each semitone, `F[]` a fractional multiplier for the 128 sub-steps. **They are just equal-temperament maths**: derive them from `2^(n/12)` in 4.12 fixed point; no table is needed. **[ASSUMED]** that the original is ideal 12-TET to within a fraction of a cent (**[MEASURE]** §14.4).
* The constant **+3** (about +2.3 cents) is in the original arithmetic. **[OS]**
* `root = root_key·128 − cents·1.285 + sample_tune`; for *NT* splits root = played key (no transposition). **[OS]**
* **Global scale** adds a per-note offset: *Equal Temp 1/2*, *Pure Major*, *Pure Minor*, *User*. Equal Temp 2 draws a random detune in about −16..+15 (1/128 semitone) per note, biased toward zero. Pure scales offset by the interval of `(key − pure_key) mod 12`. **[OS]**; the exact offsets **[MEASURE]**/**[MAN]**.

## 5. Filter (VDF) and cutoff

* **Cutoff register** (13-bit, 0..0x1FFF):
  `cutoff = program_cutoff × 82.75 + max(0, n_e × 0x47 − 0x900) + keyboard_tracking × (key − centre) + split_trim × 0x52 + MG + joystick/aftertouch + EG_level × intensity`, clamped to 0..0x1FFF (VDF 1 additionally to 0x1FD2). The `max(0, …)` term is a built-in key tracking every program has. **[OS]** Both filter chips use the same register value.
* **Topology (assumed):** 4 poles total, **24 dB/octave, no resonance**. **[ASSUMED]**
  * *Stage type A:* plain one-pole low-pass `y += k·(x − y)`.
  * *Stage type B:* bilinear one-pole with a zero at Nyquist, DC gain 1: `y = −T·y₁ + (1+T)/2·(x + x₁)` with `T = 2k − 1`.
  * Each of two filter units contains one A and one B stage; unit 1 scales `k` by 0.707.
  * `k` as a function of the cutoff register is a **12-bit lookup**: **[MEASURE]** (§14.3). A one-pole with `k = 1 − exp(−2π·fc/fs)` and a monotone register→`fc` mapping measured from the real filter is the right way to rebuild it.
* **Oversampling [ASSUMED]:** the filter runs at **twice the output rate**: every output sample passes the four stages twice with the input held. This doubles all corner frequencies relative to evaluating `k` at 31.25 kHz. It is the best fit to two recordings (which carry hall, chorus and MP3 coding), **not** a measurement of the hardware, and nothing here distinguishes it from other fits (see §14.3 and §15).

## 6. Amplitude (VDA) and velocity

* `gain = osc_level × split_level_trim` (+ keyboard tracking + aftertouch) `× velocity_curve × timbre_level × VDA_EG`, all multiplied. Full scale 0x1FFF; the real machine sits about 0.5 dB below it. **[OS]** / **[ASSUMED]**
* **Velocity curve:** 256 entries mapping a velocity-derived index to a linear gain; index **252 = unity**. **[OS]** **[MEASURE]** the curve (§14.5). Hub keys deliver velocity as a float 0..1; map `round(v·127)`.

## 7. Envelopes

Three independent EGs per voice: **pitch EG, VDF EG, VDA EG**, each with attack/decay/break/slope/sustain/release style segments (see `m1_params` for the parameter list).

* **Levels** are parameter value × 330. **[OS]**
* **Rising segments** are **linear**: `phase += RISE[idx]` until it reaches 0xFFDC. **[OS]**
* **Falling VDF and VDA segments are exponential**: each update `phase −= (RISE[idx] × phase) >> 16`, ending at **−48 dB** of the segment's range, or **−60 dB** when the VDA falls to zero. **[OS]** (an earlier model with all-linear segments was wrong).
* **Pitch EG is linear in both directions**, using the rise rates. **[OS]**
* **Time → rate index:** `idx = (t × 330 × F) >> 22` where `t` is the 0..99 time parameter and `F` a time factor from the **EG time keyboard/velocity** parameters and their *switch/polarity* bytes (bits 0–3 enable for attack/decay/slope/release; bits 4–7 flip the sign). **[OS]** `RISE[idx]` is a 256-entry rate table: **[MEASURE]** (§14.6).
* A drum's *Decay* scales the VDA decay segment.

## 8. Modulation generators (MGs)

* 16-bit phase; **step per update** from a 100-entry frequency table (≈ 1.64·v² → about 11.8 Hz at v = 99). **[OS]** **[MEASURE]** (§14.7).
* Waveforms: **triangle, saw up, saw down, unipolar square**. **[OS]**
* **Delay:** an accumulator that holds depth at 0 for the first half of the delay and then fades it in. **[OS]**
* **Depths:** pitch depth 99 ≈ ±12 semitones; VDF depth 99 ≈ ±97 cutoff steps. **[OS]**
* **Key sync:** without it, a new voice adopts the timbre's running MG phase. **[OS]**

## 9. Controllers

Joystick X = pitch bend (range from the program's *JS pitch bend* in semitones) and also *JS VDF sweep*; +Y (CC1) adds pitch-MG depth and speed; −Y (CC2) adds VDF-MG depth and speed; **aftertouch** adds pitch, pitch-MG, cutoff, VDF-MG and amplitude. **[OS]**

## 10. Voice allocation

* A single queue of 16 channels, ordered `[free | releasing (oldest first) | held (oldest first)]`. A new note takes the head of the queue. A channel that is released or damper-held moves in front of the held ones; a channel whose sound has finished moves to the head. **[OS]**
* **Double** programs take two channels per note. **Mono** = retrigger with last-note priority. **Hold** skips the release. A delayed OSC 2 whose key is released before its delay elapses never starts. **[OS]**

## 11. Drum programs

Program byte 12 selects one of 4 global kits. Each instrument plays on its own key **and on the free keys below it**, transposed. Tune is in 10-cent steps; Level and Decay are trims of 330 per step relative to 1.0. Drum programs always use each instrument's own pan. **[OS]**

## 12. Effects

Two effect units (effect 1, effect 2), fed by output buses A/B and C/D. In **parallel** placement effect 2 reads C/D and is added to outputs 1/2 through the Out-3/4 panpots; in **serial** placement C/D are added to effect 2's input and effect 2 follows effect 1. Each unit has a *dry : effect* balance from an S-curve table (**[MEASURE]**); a switched-off effect is balance 0. For delay, chorus, flanger, tremolo and exciter the EQ stays in the dry path. **[MAN]** **[OS]**

**33 types:** Hall, Ens Hall, ConcHall, Room, LargeRoom, LiveStage, E.Ref 1/2/3, St Delay, CrossDly, Chorus 1/2, Flanger 1, CrossFlng, Phaser 1/2, Tremolo 1/2, Equalizer, OverDrive, Distortion, Exciter, Symph Ens, Rotary SP, Dly/Hall, Dly/Room, Dly/E.Ref, Dly/Dly, Dly/Chorus, Dly/Flngr, Dly/Phasr, Dly/Treml, No Effect.

**Delay RAM:** 2 × 32 K words of 16-bit, one ring buffer per unit, a pointer advancing once per sample. Every delay line is an offset range inside its half. **[OS]**

**Structural families** (build each as a standard DSP block; the original's *exact* gains and delay lengths must be measured, §14.8):

| Family | Types | Structure |
|---|---|---|
| Reverbs | Hall, Ens Hall, ConcHall, Room, LargeRoom, LiveStage | parallel comb filters (feedback + damping low-pass) into series all-passes, plus pre-delay and early-reflection taps |
| Early reflections | E.Ref 1/2/3 | a tapped delay with per-tap gains (E.Ref 1/2) and a chain of three all-passes for E.Ref 3 |
| Delays | St Delay, CrossDly | stereo delay lines with feedback and damping; cross-feedback for CrossDly |
| Modulation | Chorus 1/2, Flanger 1, CrossFlng, Phaser 1/2, Symph Ens | short modulated delays / all-pass stages driven by an LFO (raised-cosine sine; triangle) |
| Amplitude | Tremolo 1/2 | LFO gain modulation |
| Rotary | Rotary SP | split drum/horn, per-band AM and Doppler, two virtual microphones |
| Tone | Equalizer, Exciter, OverDrive, Distortion | shelves (250/500 Hz and 1/2/4 kHz corners, 0 to −12 dB), harmonic exciter, waveshapers |
| Combinations | Dly/xxx | a delay followed by a second effect |

**Known gap.** The original runs a programmable DSP whose microcode is uploaded per program; the effect structures above come from analysis, not from emulating that chip. The least-certain unit is **Symphonic Ensemble** (its LFO semantics and offsets are assumed). Chorus appears 115 times in the factory banks, so LFO shape matters. **[ASSUMED]**

## 13. Output

Engine output is a float or Q15 stereo sum. **This section describes the PicoFaceM1 port's output stage, not behaviour of the M1 itself.** The port multiplies by `master × 2` and soft-clips above 0.9 (a smooth knee that never exceeds 1.0), and its authors note that one voice peaks around −12 dBFS and 16 voices sum below full scale. Any implementation needs equivalent headroom and a limiter; the exact curve is a design choice. **[ASSUMED]**

## 14. What to MEASURE (replaces trusting the original's tables)

For each, the method works on a real M1 **or** on any reference implementation you own locally; never publish the reference data.

| # | Quantity | Method |
|---|---|---|
| 14.1 | Interpolator response | Play a sine at several pitches through a one-shot looped sample; fit passband ripple and stopband attenuation |
| 14.2 | Sample-set structure | n/a: your own sample set defines this |
| 14.3 | VDF `k` vs cutoff register (4096 values) | Program a flat-spectrum looped sample (noise); for each register step capture the output spectrum and fit `fc`; check the 24 dB/oct slope |
| 14.4 | Pitch accuracy | Sweep notes; measure cents error against 12-TET (confirms `S`/`F` are ideal) |
| 14.5 | Velocity curve (256 values) | Fixed program, sweep velocity 0..127, record RMS of the sustained note |
| 14.6 | EG rate tables | For each time value 0..99 (and key/velocity time-scaling), measure segment duration; derive `RISE[idx]` and the exponential law |
| 14.7 | MG frequency, delay, depth | Record MG-modulated pitch/cutoff, extract rate and depth for each 0..99 |
| 14.8 | Effects | For each type and parameter set, drive with an impulse, a noise burst and a sine sweep; identify delay lengths (impulse), feedback/damping (decay), LFO rate/depth (sweep) |
| 14.9 | Balance curve and panpots | Sweep the dry:effect parameter and the Out-3/4 pan, record levels |
| 14.10 | Ramp shape of VDF/VDA updates | Step a parameter mid-note and capture the transition |

## 15. Open questions (carried from the earlier analysis)

1. Hardware ramp shape of the filter/amplifier chips ( **[ASSUMED]** linear over 650 samples ).
2. The filter's exact topology (four poles from two tables per chip is a fit, not a proof).
3. Interpolator taps before the loop start.
4. The 1.3 ms voice tick rests on a 2 MHz timer clock inferred from the MIDI baud divisor.
5. Only two MP3 recordings (with hall, chorus and unknown velocity) exist as ground truth. Agreement between the earlier Python reference and the engine checks the *port*, not the *analysis*.
6. Symphonic Ensemble and Rotary depth parameters.

### 15.1 Under-specified in this draft (found by independent review, 2026-11-07)

This draft is **not yet implementable from alone**; the following must be defined (by measurement or by the analyst lane) before it can be called a specification:

* **Units, ranges, rounding and boundaries** for the pitch formula (valid indices for negative or out-of-range `d`; quantisation of `F`), the cutoff sum (scale and sign of keyboard tracking, MG, joystick/aftertouch, EG intensity) and `n_e` folding at the ends of the keyboard.
* **Filter state and ordering:** per-stage state, the exact input/hold/update order of the two passes, and the stage coefficients.
* **Amplitude:** which terms add and which multiply, their scales, and where clipping and pan gains apply; "full scale 0x1FFF" is not an operational definition.
* **Envelopes:** phase initialisation, segment transitions (break, sustain, release), zero-time behaviour, parameter ranges, and exact keyboard/velocity polarity arithmetic. "Ends at −48 dB" lacks the stopping condition.
* **Loops:** the source index of every tap when the 8-tap window straddles `loop_start` or `end`, and equality and out-of-range cases for one-shots.
* **Effects:** per-type routing, parameter ranges, coefficients and state initialisation; §16's ±1 sample / ±1 dB effect test cannot be run without them.
* **Provenance:** several statements in §§3-9 carry no tag; tags should be added to every normative sentence.
* **Reference implementations** can only validate a *port*, never the M1; real-M1 measurements (§14) are required for any claim of fidelity.

## 16. Acceptance tests (implementation-independent)

* **Pitch:** every note within ±1 cent of 12-TET after the global scale offset.
* **Envelope:** each EG segment's duration matches the measured curve within 5 %.
* **Filter:** −3 dB frequency within ±5 % of the measured `fc` at 16 register points; slope 24 ± 3 dB/octave.
* **Voices:** on a Single program, hold 16 notes, release the two oldest, then play two more: the new notes take the released channels (oldest-released first). With all 16 held and none released, the 17th note takes the oldest *held* channel. A Double program holds 8 notes. (A damper-held channel counts as releasing.)
* **Output:** no sample above ±1.0 for any 16-voice full-velocity chord.
* **Effects:** impulse response of each type matches the measured delay taps within ±1 sample and decay within ±1 dB.
