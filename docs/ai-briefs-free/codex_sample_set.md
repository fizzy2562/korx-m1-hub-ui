# Task: write and verify a ROM-free synthetic sample-set generator (C++17)

Work ONLY inside `C:\build\korx-m1\work\codex16` (the owner granted full access on this machine for this project; stay in this folder anyway; do NOT run ollama or any local model; do NOT touch or kill other processes). This is a small, exactly-specified job that a local model failed twice (first it truncated samples to int16 before normalising them, so some waves came out silent; the retry ran away with 40 KB of junk). You may use WSL (`wsl`) for g++.

## What to produce in `C:\build\korx-m1\work\codex16`
1. `m1free_samples.h` and 2. `samples_test.cpp`, exactly as specified in `SPEC.md` (the same folder: read it fully; it is the original brief). Follow it literally, including the layout constants, the formulas, the names, and the CRITICAL IMPLEMENTATION RULE at its end (compute each cycle into a `double buf[kCycle]` first, normalise to a peak of exactly 8192, write `(int16_t)lround(...)`; the noise wave uses ONE running LCG per region).
3. `REPORT.md`: what you built, the commands you ran, the results.

## How to compile and test (use WSL)
```
wsl bash -c "cd /mnt/c/build/korx-m1/work/codex16 && g++ -std=c++17 -O1 -Wall -Wextra -I. -I/mnt/c/build/korx-m1/ref/PicoVintageSynthCollection/instruments/PicoFaceM1/include samples_test.cpp -o st && ./st"
```
The header includes `m1_engine/m1_engine.h` (found through the second `-I`) for the structs `m1::Split`, `m1::Multisound`, `m1::DrumSound`; do not redefine them. It must compile with ZERO warnings under `-Wall -Wextra`, every variable declared once per scope, no unused variables, no sign-compare warnings.

## Acceptance
* `./st` prints `PASS ...` lines only (no `FAIL`), exit code 0.
* Additionally verify yourself, and print in REPORT.md, the peak absolute value of every wave/variant cycle (8 waves x 4 variants; ALL must be exactly 8192 except none may be 0), and that the sine variants are accurate to within 2 LSB of `8192*sin(2*pi*cycles*i/239)`.
* Do not edit anything outside `codex16`. Stop when REPORT.md is written.
