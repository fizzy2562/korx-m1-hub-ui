import fs from 'node:fs';

const args = process.argv.slice(2);
if (args.length < 2) {
  process.exit(1);
}

const wasmPath = args[0];
const program = parseInt(args[1], 10);
const label = args[2] || 'default';

const { instance } = await WebAssembly.instantiate(fs.readFileSync(wasmPath), {});

const exports = instance.exports;

// Helper to get a fresh view of the memory buffer
function getMemoryView() {
  return new Float32Array(instance.exports.memory.buffer);
}

// Initialize
if (exports._initialize) {
  exports._initialize();
}
exports.vibe_wasm_init(48000.0);

// Send MIDI Program Change
// type 5 (MIDI), id 0, down 0, value 0, midi = status | d1<<8 | d2<<16
const status = 0xC0;
const d1 = program;
const d2 = 0;
const midi = status | (d1 << 8) | (d2 << 16);
exports.vibe_wasm_event(5, 0, 0, 0, midi);

// Render 64 blocks (discard)
for (let i = 0; i < 64; i++) {
  exports.vibe_wasm_audio(128, 0);
}

// Send Key Down
// type 2 (Key), id 12 (Middle C), down 1, value 0.8
exports.vibe_wasm_event(2, 12, 1, 0.8, 0);

// Render 6 seconds
const sampleRate = 48000;
const blockFrames = 128;
const totalSeconds = 6;
const totalBlocks = Math.floor((sampleRate * totalSeconds) / blockFrames);
const totalSamples = totalBlocks * blockFrames;

const audioBuffer = new Float32Array(totalSamples);
let writeIndex = 0;

for (let i = 0; i < totalBlocks; i++) {
  exports.vibe_wasm_audio(blockFrames, 0);
  
  // Read outL from memory
  // outL is at float index 256, which is byte offset 256 * 4 = 1024
  // The prompt says: "vibe_wasm_io() -> byte offset of a Float32 array (outL at float index 256 = byte io+1024)"
  // This implies the base of the audio buffer is at `io`, and outL starts at `io + 1024`.
  // Let's get the io offset.
  const ioOffset = exports.vibe_wasm_io();
  const outLByteOffset = ioOffset + 1024;
  
  // Copy 128 floats from outL
  const memView = new Float32Array(instance.exports.memory.buffer);
  for (let j = 0; j < blockFrames; j++) {
    audioBuffer[writeIndex + j] = memView[(outLByteOffset / 4) + j];
  }
  writeIndex += blockFrames;
}

// Analysis
const windowSize = 2400; // 50ms at 48kHz
const times = [0.00, 0.05, 0.10, 0.20, 0.40, 0.80, 1.5, 3.0];

// FFT Implementation (Radix-2 Cooley-Tukey)
function fft(re, im) {
  const n = re.length;
  if (n <= 1) return;
  
  // Bit-reversal permutation
  for (let i = 1, j = 0; i < n; i++) {
    let bit = n >> 1;
    for (; j & bit; bit >>= 1) {
      j ^= bit;
    }
    j ^= bit;
    if (i < j) {
      let temp = re[i];
      re[i] = re[j];
      re[j] = temp;
      temp = im[i];
      im[i] = im[j];
      im[j] = temp;
    }
  }
  
  // Iterative FFT
  for (let len = 2; len <= n; len <<= 1) {
    const ang = -2 * Math.PI / len;
    const wRe = Math.cos(ang);
    const wIm = Math.sin(ang);
    for (let i = 0; i < n; i += len) {
      let curRe = 1;
      let curIm = 0;
      for (let j = 0; j < len / 2; j++) {
        const uRe = re[i + j];
        const uIm = im[i + j];
        const vRe = re[i + j + len / 2] * curRe - im[i + j + len / 2] * curIm;
        const vIm = re[i + j + len / 2] * curIm + im[i + j + len / 2] * curRe;
        re[i + j] = uRe + vRe;
        im[i + j] = uIm + vIm;
        re[i + j + len / 2] = uRe - vRe;
        im[i + j + len / 2] = uIm - vIm;
        const nextRe = curRe * wRe - curIm * wIm;
        const nextIm = curRe * wIm + curIm * wRe;
        curRe = nextRe;
        curIm = nextIm;
      }
    }
  }
}

// Hann Window
function hannWindow(n) {
  const w = new Float32Array(n);
  for (let i = 0; i < n; i++) {
    w[i] = 0.5 * (1 - Math.cos(2 * Math.PI * i / (n - 1)));
  }
  return w;
}

const hann = hannWindow(2048);

function analyzeWindow(samples, startIdx) {
  // RMS
  let sumSq = 0;
  for (let i = 0; i < windowSize; i++) {
    const v = samples[startIdx + i];
    sumSq += v * v;
  }
  const rms = Math.sqrt(sumSq / windowSize);
  
  // Roughness
  let roughness = 0;
  if (rms >= 1e-6) {
    let sumDiffSq = 0;
    for (let i = 2; i < windowSize; i++) {
      const diff = samples[startIdx + i] - 2 * samples[startIdx + i - 1] + samples[startIdx + i - 2];
      sumDiffSq += diff * diff;
    }
    roughness = Math.sqrt(sumDiffSq / (windowSize - 2)) / rms;
  }
  
  // HF Ratio
  // Take 2048 samples from the start of the window
  const fftSize = 2048;
  const re = new Float32Array(fftSize);
  const im = new Float32Array(fftSize);
  
  for (let i = 0; i < fftSize; i++) {
    re[i] = samples[startIdx + i] * hann[i];
    im[i] = 0;
  }
  
  fft(re, im);
  
  let totalEnergy = 0;
  let hfEnergy = 0;
  
  // Frequency bin k corresponds to k * (sampleRate / fftSize)
  // We need bins where freq > 8000 Hz
  // k > 8000 / (48000 / 2048) = 8000 * 2048 / 48000 = 341.33...
  // So k >= 342
  // Also, since it's a real signal, we only need to look at the first half (0 to fftSize/2)
  // The prompt says "energy of bins with frequency > 8000 Hz divided by total energy"
  // Usually "total energy" in this context implies the total energy of the signal in the window.
  // However, FFT energy is distributed across all bins.
  // Let's sum |X[k]|^2 for all k from 0 to fftSize/2 (Nyquist) for total energy?
  // Or all bins? Parseval's theorem says sum |x[n]|^2 = (1/N) sum |X[k]|^2.
  // Let's use the sum of squared magnitudes of the first half of the FFT (0 to N/2) as the "total energy" representation,
  // and the subset of those bins > 8kHz as "hf energy".
  
  const nyquist = fftSize / 2;
  const hfStartBin = Math.floor(8000 / (sampleRate / fftSize)) + 1; // First bin strictly > 8000
  
  for (let k = 0; k <= nyquist; k++) {
    const magSq = re[k] * re[k] + im[k] * im[k];
    totalEnergy += magSq;
    if (k >= hfStartBin) {
      hfEnergy += magSq;
    }
  }
  
  let hfRatio = 0;
  if (totalEnergy > 0) {
    hfRatio = hfEnergy / totalEnergy;
  }
  
  return {
    rms: rms.toFixed(4),
    roughness: roughness.toFixed(4),
    hfRatio: hfRatio.toFixed(4)
  };
}

// Print Header
console.log(`drift ${label} program=${program}`);

// Process each time window
for (const t of times) {
  const startSample = Math.floor(t * sampleRate);
  
  // Check if window fits in buffer
  if (startSample + windowSize > audioBuffer.length) {
    // If it doesn't fit, we can't analyze it properly. 
    // The prompt implies we should be able to analyze these specific times.
    // 3.0s + 0.05s = 3.05s. Total is 6s. So it fits.
    // 0.00 to 3.00 all fit.
  }
  
  const { rms, roughness, hfRatio } = analyzeWindow(audioBuffer, startSample);
  console.log(`t=${t} rms=${rms} rough=${roughness} hf=${hfRatio}`);
}
