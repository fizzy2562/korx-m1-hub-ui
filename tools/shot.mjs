import fs from 'node:fs';
import zlib from 'node:zlib';

function fail(msg) {
  console.error(msg);
  process.exit(1);
}

// ---------- CRC32 (table-based) ----------
const CRC_TABLE = (() => {
  const t = new Uint32Array(256);
  for (let n = 0; n < 256; n++) {
    let c = n;
    for (let k = 0; k < 8; k++) {
      c = (c & 1) ? (0xEDB88320 ^ (c >>> 1)) : (c >>> 1);
    }
    t[n] = c >>> 0;
  }
  return t;
})();

function crc32(buf) {
  let c = 0xFFFFFFFF;
  for (let i = 0; i < buf.length; i++) {
    c = CRC_TABLE[(c ^ buf[i]) & 0xFF] ^ (c >>> 8);
  }
  return (c ^ 0xFFFFFFFF) >>> 0;
}

// ---------- PNG writer ----------
function pngChunk(type, data) {
  const len = Buffer.alloc(4);
  len.writeUInt32BE(data.length, 0);
  const typeBuf = Buffer.from(type, 'ascii');
  const crcBuf = Buffer.alloc(4);
  crcBuf.writeUInt32BE(crc32(Buffer.concat([typeBuf, data])), 0);
  return Buffer.concat([len, typeBuf, data, crcBuf]);
}

function writePng(path, width, height, rgb) {
  // rgb: Uint8Array of width*height*3 (RGB888)
  const sig = Buffer.from([0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]);

  const ihdr = Buffer.alloc(13);
  ihdr.writeUInt32BE(width, 0);
  ihdr.writeUInt32BE(height, 4);
  ihdr.writeUInt8(8, 8);   // bit depth
  ihdr.writeUInt8(2, 9);   // color type: truecolor RGB
  ihdr.writeUInt8(0, 10);  // compression
  ihdr.writeUInt8(0, 11);  // filter
  ihdr.writeUInt8(0, 12);  // interlace

  // Build raw scanlines: each row prefixed with filter byte 0
  const stride = width * 3;
  const raw = Buffer.alloc((stride + 1) * height);
  for (let y = 0; y < height; y++) {
    raw[y * (stride + 1)] = 0; // filter none
    rgb.copy ? null : null;
    // copy row
    const srcOff = y * stride;
    const dstOff = y * (stride + 1) + 1;
    for (let x = 0; x < stride; x++) {
      raw[dstOff + x] = rgb[srcOff + x];
    }
  }

  const idat = zlib.deflateSync(raw, { level: 9 });

  const out = Buffer.concat([
    sig,
    pngChunk('IHDR', ihdr),
    pngChunk('IDAT', idat),
    pngChunk('IEND', Buffer.alloc(0)),
  ]);

  fs.writeFileSync(path, out);
}

// ---------- Main ----------
const args = process.argv.slice(2);
if (args.length < 2) {
  fail('usage: node shot.mjs <fw.wasm> <out.png> [step ...]');
}

const fwPath = args[0];
const outPath = args[1];
const steps = args.slice(2);

let bytes;
try {
  bytes = fs.readFileSync(fwPath);
} catch (e) {
  fail(`cannot read firmware: ${e.message}`);
}

let module;
try {
  module = new WebAssembly.Module(bytes);
} catch (e) {
  fail(`invalid wasm: ${e.message}`);
}

// Build import object
const imports = {};
for (const imp of WebAssembly.Module.imports(module)) {
  if (!imports[imp.module]) imports[imp.module] = {};
  if (imp.kind === 'function') {
    imports[imp.module][imp.name] = () => 0;
  } else if (imp.kind === 'memory') {
    imports[imp.module][imp.name] = new WebAssembly.Memory({ initial: 256, maximum: 65536 });
  } else if (imp.kind === 'table') {
    imports[imp.module][imp.name] = new WebAssembly.Table({ initial: 10, element: 'anyfunc' });
  } else if (imp.kind === 'global') {
    imports[imp.module][imp.name] = new WebAssembly.Global({ value: 'i32', mutable: true }, 0);
  }
}

let instance;
try {
  const result = await WebAssembly.instantiate(bytes, imports);
  instance = result.instance;
} catch (e) {
  fail(`instantiate failed: ${e.message}`);
}

const exports = instance.exports;

// Required exports
if (typeof exports.vibe_wasm_init !== 'function') fail('missing export vibe_wasm_init');
if (typeof exports.vibe_wasm_audio !== 'function') fail('missing export vibe_wasm_audio');
if (typeof exports.vibe_wasm_event !== 'function') fail('missing export vibe_wasm_event');
if (typeof exports.vibe_wasm_draw !== 'function') fail('missing export vibe_wasm_draw');
if (typeof exports.vibe_wasm_io !== 'function') fail('missing export vibe_wasm_io');

// Init
if (typeof exports._initialize === 'function') {
  exports._initialize();
}
exports.vibe_wasm_init(48000);

const memory = exports.memory;
const mem = memory.buffer;

const W = 480;
const H = 320;

let audioPeak = null; // track max abs of outL

function readFloat32(byteOffset) {
  return new Float32Array(mem, byteOffset, 1)[0];
}

function parseStep(step) {
  // returns an action descriptor
  if (step.startsWith('a:')) {
    const n = parseInt(step.slice(2), 10);
    if (!Number.isFinite(n) || n < 0) fail(`bad a: step: ${step}`);
    return { type: 'audio', n };
  }
  if (step.startsWith('e:')) {
    const parts = step.slice(2).split(',');
    if (parts.length < 4) fail(`bad e: step (need type,id,down,value[,midi]): ${step}`);
    const type = parseInt(parts[0], 10);
    const id = parseInt(parts[1], 10);
    const down = parseInt(parts[2], 10);
    const value = parseFloat(parts[3]);
    let midi = 0;
    if (parts.length >= 5 && parts[4] !== '') {
      midi = parseInt(parts[4], 10);
    }
    if (!Number.isFinite(type) || !Number.isFinite(id) || !Number.isFinite(down) || !Number.isFinite(value)) {
      fail(`bad e: step numbers: ${step}`);
    }
    return { type: 'event', etype: type, id, down, value, midi };
  }
  if (step.startsWith('m:')) {
    const parts = step.slice(2).split(',');
    if (parts.length < 3) fail(`bad m: step (need status,d1,d2): ${step}`);
    const status = parseInt(parts[0], 10);
    const d1 = parseInt(parts[1], 10);
    const d2 = parseInt(parts[2], 10);
    if (!Number.isFinite(status) || !Number.isFinite(d1) || !Number.isFinite(d2)) {
      fail(`bad m: step numbers: ${step}`);
    }
    const value = status | (d1 << 8) | (d2 << 16);
    return { type: 'event', etype: 5, id: 0, down: 0, value, midi: 0 };
  }
  if (step.startsWith('s:')) {
    const path = step.slice(2);
    if (!path) fail(`bad s: step (need path): ${step}`);
    return { type: 'save', path };
  }
  fail(`unknown step: ${step}`);
}

function takeScreenshot(path) {
  const fbOffset = exports.vibe_wasm_draw();
  if (typeof fbOffset !== 'number') fail('vibe_wasm_draw did not return a number');
  const fb = new Uint16Array(mem, fbOffset, W * H);

  // Upscale 2x nearest neighbor -> 960x640
  const outW = W * 2;
  const outH = H * 2;
  const rgb = new Uint8Array(outW * outH * 3);

  for (let y = 0; y < H; y++) {
    for (let x = 0; x < W; x++) {
      const p = fb[y * W + x];
      const r = (p >> 11) & 31;
      const g = (p >> 5) & 63;
      const b = p & 31;
      const r8 = (r << 3) | (r >> 2);
      const g8 = (g << 2) | (g >> 4);
      const b8 = (b << 3) | (b >> 2);

      // 2x2 block
      const baseY = y * 2;
      const baseX = x * 2;
      for (let dy = 0; dy < 2; dy++) {
        for (let dx = 0; dx < 2; dx++) {
          const idx = ((baseY + dy) * outW + (baseX + dx)) * 3;
          rgb[idx] = r8;
          rgb[idx + 1] = g8;
          rgb[idx + 2] = b8;
        }
      }
    }
  }

  writePng(path, outW, outH, rgb);
  console.log(`saved ${path}`);

  // Labels
  if (typeof exports.vibe_wasm_labels === 'function') {
    const labelsOffset = exports.vibe_wasm_labels();
    if (typeof labelsOffset === 'number') {
      const labels = new Uint8Array(mem, labelsOffset, 25 * 8);
      const names = [];
      for (let i = 0; i < 6; i++) {
        const slot = labels.subarray(i * 8, i * 8 + 8);
        let s = '';
        for (let j = 0; j < 8; j++) {
          if (slot[j] === 0) break;
          s += String.fromCharCode(slot[j]);
        }
        names.push(s);
      }
      console.log(`labels: F1..F6 = ${names.join('|')}`);
    }
  }
}

// Run steps
for (const step of steps) {
  const action = parseStep(step);
  if (action.type === 'audio') {
    for (let i = 0; i < action.n; i++) {
      exports.vibe_wasm_audio(128, 0);
    }
    // Read outL: 128 floats at float index 256 => byte offset io + 1024
    const io = exports.vibe_wasm_io();
    const outLOffset = io + 1024;
    const outL = new Float32Array(mem, outLOffset, 128);
    for (let i = 0; i < 128; i++) {
      const v = Math.abs(outL[i]);
      if (audioPeak === null || v > audioPeak) audioPeak = v;
    }
  } else if (action.type === 'event') {
    exports.vibe_wasm_event(action.etype, action.id, action.down, action.value, action.midi);
  } else if (action.type === 'save') {
    takeScreenshot(action.path);
  }
}

// Final screenshot
takeScreenshot(outPath);

if (audioPeak !== null) {
  console.log(`audio peak ${audioPeak}`);
}
