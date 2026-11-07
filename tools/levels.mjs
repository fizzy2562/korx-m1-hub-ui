import { readFileSync } from 'fs';
import { basename } from 'path';

const wasmBytes = readFileSync(process.argv[2]);
const label = process.argv[3] || basename(process.argv[2]);

async function runCase(name, setup, renderDurationSec) {
    const { instance } = await WebAssembly.instantiate(wasmBytes);
    const memory = instance.exports.memory;
    const f32 = new Float32Array(memory.buffer);

    if (instance.exports._initialize) {
        instance.exports._initialize();
    }

    instance.exports.vibe_wasm_init(48000);

    const ioOffset = instance.exports.vibe_wasm_io();
    const outLStart = ioOffset + 1024;
    const outREnd = ioOffset + 2048;

    const samplesL = [];
    const samplesR = [];

    const totalFrames = Math.floor(48000 * renderDurationSec);
    const blockFrames = 128;
    const numBlocks = Math.ceil(totalFrames / blockFrames);

    setup(instance);

    for (let i = 0; i < numBlocks; i++) {
        instance.exports.vibe_wasm_audio(blockFrames, 0);
        for (let j = 0; j < blockFrames; j++) {
            samplesL.push(f32[(outLStart + j * 4) / 4]);
            samplesR.push(f32[(outREnd + j * 4) / 4]);
        }
    }

    let peak = 0;
    let sumSq = 0;
    let over1Count = 0;
    let totalSamples = 0;

    for (let i = 0; i < samplesL.length; i++) {
        const l = samplesL[i];
        const r = samplesR[i];
        const al = Math.abs(l);
        const ar = Math.abs(r);

        if (al > peak) peak = al;
        if (ar > peak) peak = ar;

        sumSq += l * l + r * r;
        totalSamples += 2;

        if (al >= 0.999) over1Count++;
        if (ar >= 0.999) over1Count++;
    }

    const rms = Math.sqrt(sumSq / totalSamples);
    const peakdBFS = peak > 0 ? 20 * Math.log10(peak) : -Infinity;
    const rmsdBFS = rms > 0 ? 20 * Math.log10(rms) : -Infinity;
    const over1pct = (over1Count / totalSamples) * 100;
    const clipfree_gain = peak > 1 ? 1 / peak : 1;

    console.log(`${label} ${name}: peak=${peak.toFixed(3)} peakdBFS=${peakdBFS.toFixed(1)} rms=${rmsdBFS.toFixed(1)} over1=${over1Count} over1pct=${over1pct.toFixed(2)} clipfree_gain=${clipfree_gain.toFixed(3)}`);
}

async function main() {
    // Case 1
    await runCase("single_C3_vel0.8", (inst) => {
        inst.exports.vibe_wasm_event(2, 12, 1, 0.8, 0);
    }, 1.5);

    // Case 2
    await runCase("chord4_vel0.8", (inst) => {
        [12, 16, 19, 24].forEach(id => inst.exports.vibe_wasm_event(2, id, 1, 0.8, 0));
    }, 1.5);

    // Case 3
    await runCase("chord8_vel1.0", (inst) => {
        [0, 4, 7, 12, 16, 19, 24, 5].forEach(id => inst.exports.vibe_wasm_event(2, id, 1, 1.0, 0));
    }, 1.5);

    // Case 4
    await runCase("chord8_with_sustain", (inst) => {
        // Send Sustain On (CC64 = 127)
        inst.exports.vibe_wasm_event(5, 0, 0, 0, (0xB0) | (64 << 8) | (127 << 16));
        
        // Press keys
        const keys = [0, 4, 7, 12, 16, 19, 24, 5];
        keys.forEach(id => inst.exports.vibe_wasm_event(2, id, 1, 1.0, 0));
        
        // Render 0.5s
        const frames05 = Math.floor(48000 * 0.5);
        const blocks05 = Math.ceil(frames05 / 128);
        for (let i = 0; i < blocks05; i++) {
            inst.exports.vibe_wasm_audio(128, 0);
        }
        
        // Release keys
        keys.forEach(id => inst.exports.vibe_wasm_event(2, id, 0, 0, 0));
        
        // Render 1.0s
        const frames10 = Math.floor(48000 * 1.0);
        const blocks10 = Math.ceil(frames10 / 128);
        for (let i = 0; i < blocks10; i++) {
            inst.exports.vibe_wasm_audio(128, 0);
        }
    }, 1.5);
}

main().catch(console.error);
