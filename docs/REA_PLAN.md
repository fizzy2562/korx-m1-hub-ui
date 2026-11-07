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

The aim is an implementation whose dependence on Korg's code is **minimised and documented**. A two-lane process reduces contamination but **cannot prove** independent creation, especially because the analyst lane works from copyrighted firmware and the spec already contains prior source-informed analysis.

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

1. **Provenance.** The README itself warns that lookalike repositories exist. Use only `github.com/morluto/rea`, `rea.tools` or the `rea-agents` npm package. Publisher and download counts are *not* evidence of safety: **pin an exact version, record and verify its integrity hash, and read what the package does** (install scripts, network access, what it registers) before running it.
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

## 6. Controls and gaps added after an independent review (2026-11-07)

An independent review found the plan above too thin on controls. Before any ROM analysis is attempted:

1. **Isolation:** a throwaway VM or separate account on a machine that does not hold other people's work; least-privilege user; an allowlist of readable folders and runnable processes; no access to unrelated projects, credentials or `.env` files; a snapshot to roll back to.
2. **Supply chain:** pinned REA version with a recorded hash; review the package; network egress restricted to what the disassembler and REA need; no auto-update.
3. **ROM custody:** record how each image was lawfully obtained; hash it; keep it read-only; keep it out of backups, logs, transcripts and any model context except Lane A's; delete work copies when finished.
4. **Evidence format and redaction:** each Lane A finding is a short record (question, behavioural conclusion, confidence, how to re-verify by measurement) with **no addresses, code, tables or disassembly**. A reviewer signs each record before it enters the spec.
5. **Lane B isolation, enforced:** a fresh session with no tool or file access except the spec; no ROM-derived files; its prompts and outputs logged; any question it asks goes through the reviewer. Prompt-injection is possible, so treat the spec as untrusted input.
6. **Deliverables and stop criteria:** Lane A has a written question list, a time budget, and a defined "could not determine" outcome; it stops when the list is done, not when it runs out of ideas.
7. **Measurements REA cannot do:** the silicon behaviour (filter chips, tone generator, effects DSP internals) needs its own plan from spec §14, with its own equipment and owner.
8. **Legal gate:** a go/no-go decision, from someone qualified, on analysing the firmware, retaining notes, and publishing the resulting spec, before Lane A starts.
