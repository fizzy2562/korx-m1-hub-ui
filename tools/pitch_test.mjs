import fs from 'node:fs';

const path = process.argv[2];
const label = process.argv[3] || 'test';

if (!path) {
  console.error('Usage: node pitch_test.mjs <fw.wasm> [label]');
  process.exit(0);
}

const wasmBytes = fs.readFileSync(path);
const programs = [0, 1, 2, 3, 4, 5, 6];
const notes = [36, 48, 60, 72, 84];
const sampleRate = 48000;
const blockSize = 128;
const totalFrames = Math.round(sampleRate * 1.5);
const numBlocks = Math.ceil(totalFrames / blockSize);
const windowStart = 24000;
const windowEnd = 57600;
const windowLen = windowEnd - windowStart;
const maxAutocorrSamples = 20000;

console.log(`pitch ${label} ${path.split('/').pop()}`);

let cases = 0;
let passPitch = 0;
let silent = 0;
let maxAbsCents = 0;
let sumCents = 0;
let countCents = 0;

for (const p of programs) {
  for (const n of notes) {
    let instance;
    try {
      const result = await WebAssembly.instantiate(wasmBytes, {});
      instance = result.instance;
    } catch (e) {
      console.log(`prog=${p} note=${n} expected=0.00 measured=0.00 cents=0.0 conf=0.00 peak=0.000 rms=0.0000`);
      cases++;
      silent++;
      continue;
    }

    const exports = instance.exports;
    const memory = exports.memory;

    if (exports._initialize) {
      try { exports._initialize(); } catch (e) {}
    }

    if (exports.vibe_wasm_init) {
      try { exports.vibe_wasm_init(sampleRate); } catch (e) {}
    }

    const sendEvent = (type, id, down, value, midi) => {
      if (exports.vibe_wasm_event) {
        try { exports.vibe_wasm_event(type, id, down, value, midi); } catch (e) {}
      }
    };

    const renderBlock = () => {
      if (!exports.vibe_wasm_io || !exports.vibe_wasm_audio) return null;
      try {
        const io = exports.vibe_wasm_io();
        const buf = memory.buffer;
        const outLOffset = io + 1024;
        const outL = new Float32Array(buf, outLOffset, blockSize);
        exports.vibe_wasm_audio(blockSize, 0);
        return outL.slice();
      } catch (e) {
        return null;
      }
    };

    // Program change
    const pcMidi = 0xC0 | (p << 8) | (0 << 16);
    sendEvent(5, 0, 0, 0, pcMidi);

    // Render 16 blocks and discard
    for (let i = 0; i < 16; i++) {
      renderBlock();
    }

    // Note on
    const noteOnMidi = 0x90 | (n << 8) | (100 << 16);
    sendEvent(5, 0, 1, 0, noteOnMidi);

    // Render 1.5 seconds
    const allSamples = new Float32Array(totalFrames);
    let sampleIdx = 0;
    for (let b = 0; b < numBlocks; b++) {
      const block = renderBlock();
      if (block) {
        const len = Math.min(blockSize, totalFrames - sampleIdx);
        allSamples.set(block.subarray(0, len), sampleIdx);
        sampleIdx += len;
      }
    }

    // Analysis
    const octaveShift = (p === 2) ? -12 : 0;   // program 2 is a 16' bass: one octave below the played note
    const expected = 440 * Math.pow(2, (n + octaveShift - 69) / 12);
    const expectedPeriod = sampleRate / expected;

    // Peak over whole 1.5s
    let peak = 0;
    for (let i = 0; i < allSamples.length; i++) {
      const v = Math.abs(allSamples[i]);
      if (v > peak) peak = v;
    }

    // RMS of window
    let sumSq = 0;
    for (let i = windowStart; i < windowEnd; i++) {
      const v = allSamples[i];
      sumSq += v * v;
    }
    const rms = Math.sqrt(sumSq / windowLen);

    // Autocorrelation
    const lagMin = Math.floor(0.8 * expectedPeriod);
    const lagMax = Math.ceil(1.25 * expectedPeriod);
    const numSamples = Math.min(windowLen, maxAutocorrSamples);
    const x = allSamples.subarray(windowStart, windowStart + numSamples);

    let bestLag = -1;
    let bestR = -Infinity;
    const rValues = new Map();

    for (let lag = lagMin; lag <= lagMax; lag++) {
      let num = 0;
      let den1 = 0;
      let den2 = 0;
      for (let i = 0; i < numSamples - lag; i++) {
        num += x[i] * x[i + lag];
        den1 += x[i] * x[i];
        den2 += x[i + lag] * x[i + lag];
      }
      const den = Math.sqrt(den1 * den2);
      const r = den > 0 ? num / den : 0;
      rValues.set(lag, r);
      if (r > bestR) {
        bestR = r;
        bestLag = lag;
      }
    }

    if (bestLag === -1) {
      console.log(`prog=${p} note=${n} expected=${expected.toFixed(2)} measured=0.00 cents=0.0 conf=0.00 peak=${peak.toFixed(3)} rms=${rms.toFixed(4)}`);
      cases++;
      if (peak < 0.001) silent++;
      continue;
    }

    // Parabolic interpolation
    const rPrev = rValues.get(bestLag - 1) ?? bestR;
    const rCurr = bestR;
    const rNext = rValues.get(bestLag + 1) ?? bestR;
    const denom = rPrev - 2 * rCurr + rNext;
    let offset = 0;
    if (Math.abs(denom) > 1e-10) {
      offset = 0.5 * (rPrev - rNext) / denom;
    }
    const measuredPeriod = bestLag + offset;
    const measured = measuredPeriod > 0 ? sampleRate / measuredPeriod : 0;
    const cents = measured > 0 ? 1200 * Math.log2(measured / expected) : 0;
    const conf = bestR;

    console.log(`prog=${p} note=${n} expected=${expected.toFixed(2)} measured=${measured.toFixed(2)} cents=${cents >= 0 ? '+' : ''}${cents.toFixed(1)} conf=${conf.toFixed(2)} peak=${peak.toFixed(3)} rms=${rms.toFixed(4)}`);

    cases++;
    if (peak < 0.001) silent++;
    if (Math.abs(cents) <= 8 && conf >= 0.8) passPitch++;
    if (conf >= 0.8) {
      const absCents = Math.abs(cents);
      if (absCents > maxAbsCents) maxAbsCents = absCents;
      sumCents += cents;
      countCents++;
    }
  }
}

const meanCents = countCents > 0 ? sumCents / countCents : 0;
console.log(`summary cases=${cases} pass_pitch=${passPitch} silent=${silent} max_abs_cents=${maxAbsCents.toFixed(1)} mean_cents=${meanCents.toFixed(1)}`);

process.exit(0);
