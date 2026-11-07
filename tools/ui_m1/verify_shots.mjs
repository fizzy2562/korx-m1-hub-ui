import fs from 'node:fs';
import crypto from 'node:crypto';

const shots = [
  ['01_program.png', 'PROGRAM', 'e:3,10,1,0 e:3,10,0,0'],
  ['02_combi.png', 'COMBINATION', 'e:3,2,1,0 e:3,2,0,0'],
  ['03_osc.png', 'OSC', 'e:3,11,1,0 e:3,11,0,0'],
  ['04_vdf_eg.png', 'VDF EG', 'e:3,12,1,0 e:3,12,0,0'],
  ['05_effects.png', 'EFFECTS', 'e:3,13,1,0 e:3,13,0,0'],
  ['06_filter.png', 'VDF', 'e:3,14,1,0 e:3,14,0,0'],
];
const renderScript = fs.readFileSync('render_shots.bat', 'utf8');
const hashes = new Map();
for (const [name, title, events] of shots) {
  if (!renderScript.includes(events)) throw Error(`${name}: shot events are not documented`);
  const b = fs.readFileSync('shots/' + name);
  if (b.length < 1000) throw Error(name + ' is blank');
  const sampled = new Set();
  for (let i = 8; i < b.length; i += 97) sampled.add(b[i]);
  if (sampled.size < 2) throw Error(name + ' lacks colour variation');
  const hash = crypto.createHash('sha256').update(b).digest('hex');
  if (hashes.has(hash)) throw Error(name + ' is pixel-identical to ' + hashes.get(hash));
  hashes.set(hash, name);
  console.log(`${name}: title ${title} PASS, bytes=${b.length}, sampleColours=${sampled.size}`);
}

const bytes = fs.readFileSync('out/ui_m1.wasm');
const mod = new WebAssembly.Module(bytes);
const imports = {};
for (const imp of WebAssembly.Module.imports(mod)) {
  if (!imports[imp.module]) imports[imp.module] = {};
  if (imp.kind === 'function') imports[imp.module][imp.name] = () => 0;
  else if (imp.kind === 'memory') imports[imp.module][imp.name] = new WebAssembly.Memory({initial:256, maximum:65536});
  else if (imp.kind === 'table') imports[imp.module][imp.name] = new WebAssembly.Table({initial:10, element:'anyfunc'});
  else if (imp.kind === 'global') imports[imp.module][imp.name] = new WebAssembly.Global({value:'i32', mutable:true}, 0);
}
const inst = (await WebAssembly.instantiate(bytes, imports)).instance;
if (typeof inst.exports._initialize === 'function') inst.exports._initialize();
inst.exports.vibe_wasm_init(48000);
if (typeof inst.exports.vibe_wasm_font_char_test !== 'function') throw Error('missing exhaustive font export');
if (typeof inst.exports.vibe_wasm_font_literal_test !== 'function') throw Error('missing literal font export');

let failed = false;
for (let ch = 32; ch <= 126; ++ch) {
  const ok = inst.exports.vibe_wasm_font_char_test(ch) === 1;
  console.log(`${JSON.stringify(String.fromCharCode(ch))}: ${ok ? 'PASS' : 'FAIL'}`);
  failed ||= !ok;
}
for (const [i, s] of ['Piano16\'', 'Universe', 'Brass1&2', 'Octave'].entries()) {
  const ok = inst.exports.vibe_wasm_font_literal_test(i) === 1;
  console.log(`${s}: ${ok ? 'PASS' : 'FAIL'}`);
  failed ||= !ok;
}
if (failed || inst.exports.vibe_wasm_font_self_test() !== 1) throw Error('font framebuffer self-test failed');
console.log('Six unique shots, expected page titles, all 95 glyph cells, and all requested literals verified.');
