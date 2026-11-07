# Using REA to confirm and extend the behaviour spec

[REA](https://github.com/morluto/rea) ("Reverse Engineer Anything", MIT, TypeScript, `npx rea-agents setup`) is an MCP server that lets an AI agent drive reverse-engineering tools and report conclusions *with the evidence and limitations behind them*. Per its README it analyses native binaries (through an existing **Hopper, Ghidra, IDA or Kuna** installation), JavaScript/Electron apps, .NET assemblies and websites; analysis runs locally and the same tools are usable from a terminal.

> Everything below is based on REA's README and repository metadata only. **I have not installed, run or audited REA.** Treat it like any third-party tool that can read your files and register itself with your agents (see §5).

## 1. What REA is good for here, and what it is not

| Target | Useful? | Why |
|---|---|---|
| **M1 OS ROM** (the Rev 19 OS image pair; NEC V50 = 16-bit x86 real mode, segmented code) | **Yes, the main use** | The voice driver, envelope engine, MG, allocator, drum-kit logic and the effect compiler are all firmware. A disassembler driven by an agent can answer "what does this routine compute?" with evidence |
| The **effect microcode** the OS uploads to the effects DSP (about 128 instruction words per program, built from ~22 templates) | **Yes, highest value** | The semantics were never decoded; this is why Symphonic Ensemble and others are "assumed". Decoding the instruction set from the OS's uploader and the templates would turn guesses into facts |
| The **sound chips** (tone generator, filter/amp chips, the effects DSP itself) | **No** | Their behaviour is silicon, not firmware. It can only be *measured* (spec §14) or read from datasheets/service docs |
| **Sample ROMs / coefficient ROMs** | **No** | They are data. REA cannot reconstruct the content (and the content must never be published) |
| Korg's M1 plugin binary, if you own it | Maybe | Earlier analysis used it as a cross-check only (it models the filter differently). Check its licence for reverse-engineering clauses first |
| The Firmware Hub JavaScript | Already done | Plain readable JS; the ABI is documented in `docs/FINDINGS.md` |
| Recordings of a real M1 | No | REA is not an audio analyser. Use the measurement plan in the spec |

## 2. The two-lane clean-room process

The aim is an implementation that provably owes nothing to Korg's code.

```
 Lane A: ANALYST                           Lane B: IMPLEMENTER
 (sees the ROM, uses REA)                  (never sees the ROM, REA output, or this repo's engine code)
        │                                          ▲
        ▼                                          │
  findings with evidence   ──►  REVIEW  ──►  sanitized behaviour spec (docs/CLEAN_ROOM_SPEC.md)
  (private notebook)            gate         ── behaviour, formulas, structure only ──
```

* **Lane A** runs REA against the OS image, produces notes *with evidence*, and answers the open questions in §4. Raw disassembly, decompiler output, addresses, and table contents **stay private** and are never committed.
* **Review gate:** a human (you) checks each sentence that moves from Lane A's notes into the spec: behaviour only, no code, no addresses, no data.
* **Lane B** implements in Rust or C from the spec alone, in a **separate** model session or by a person who has not seen Lane A's material. In this project that means the local qwen model gets only the spec as its brief. The orchestrating assistant that has seen the code **should not write the implementation**, which matches the project's existing rule that it orchestrates and reviews.
* Validate Lane B's output with the acceptance tests in spec §16 and against reference renders from **your own** local ROM build (never published).

## 3. Setup, carefully

1. **Provenance.** The README itself warns that lookalike repositories exist. Use only `github.com/morluto/rea`, `rea.tools` or the `rea-agents` npm package; check the publisher and download counts before running anything.
2. **Where.** Not on the shared AI box and not in this project's working directory. Use a throwaway VM or a separate user account. The ROM files are copyrighted, so keep them off any machine you do not control.
3. **`npx rea-agents setup`** registers an MCP server and workflow instructions in your agent's configuration (with backups) and can optionally install Hopper after approval. Read the proposed changes before approving. Run it from a trusted directory outside the target project.
4. **Disassembler.** Ghidra is free and has a 16-bit x86 real-mode processor module; REA can use an existing install. A large download on the shared box needs asking first.
5. **Dry run** on something harmless (a tiny open-source binary) to see what it records, where it writes, and whether results include the evidence trail the README promises.

## 4. First tasks for Lane A, in order of value

Each yields a sentence for the spec **with evidence**, or an explicit "could not determine".

1. **Locate the voice driver** from known landmarks (the tables the spec mentions: pitch tables, velocity curve, EG rate tables, MG tables) by following cross-references from the tables back to their readers. Confirm or correct the **pitch formula** (spec §4) including the +3 constant.
2. **Envelope engine:** confirm linear-rise / exponential-fall, the −48 dB and −60 dB end points, and the time→index mapping with its keyboard/velocity switch and polarity bits (spec §7).
3. **Cutoff register assembly** (spec §5): the built-in key tracking term, the split-trim and the clamp values.
4. **Allocator:** confirm queue ordering, Double-program handling and Mono/Hold behaviour (spec §10).
5. **MG:** step formula, waveform set, delay accumulator (spec §8).
6. **Effect microcode (the big one).** Find the routine that uploads instruction words to the effects DSP and the ~22 templates; work out the instruction word format and the operation set (multiply-accumulate on delay-RAM taps, LFO-driven offsets, shelf coefficients). Then re-derive Symphonic Ensemble, Rotary SP and the Delay/xxx combinations from the actual program. This is the item that replaces "assumed".
7. **Scale and tuning** (spec §4): the Equal-Temp-2 random detune generator, the Pure Major/Minor offsets.
8. **Hardware ramp rate:** the CPU writes ramp parameters to the filter/amp chips; find what it writes and how often (this bounds the "linear over 650 samples" assumption but cannot reveal the chip's internal curve: that needs measurement).

## 5. Risks and limits

* **Legality is jurisdiction-dependent.** Analysing a ROM you lawfully own for interoperability or education is treated differently from republishing its content, and EULAs may forbid reverse engineering. Not legal advice; if you intend to publish the spec, a short check with someone qualified is cheap.
* **AI-generated claims need verification.** REA reports evidence, but an agent can still mis-assign a routine. Require each spec sentence to cite a measurable check (spec §14/§16), not just "the disassembly says so".
* **Real-mode x86 is awkward** (segments, overlapping tables, self-modifying patterns). Expect manual guidance.
* **The contamination boundary is the whole point.** If the same agent session sees disassembly and then writes the engine, the result is a derivative work, not clean-room.
* **Supply chain.** An MCP server with filesystem and process access deserves the same care as any package you run.
