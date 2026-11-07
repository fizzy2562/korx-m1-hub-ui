import fs from 'fs';
const b = fs.readFileSync(process.argv[2]); let p = 8;
const rd = () => { let r = 0, s = 0, x; do { x = b[p++]; r |= (x & 127) << s; s += 7; } while (x & 128); return r >>> 0; };
const types = [], funcs = [], imports = []; let exportsF = {}; let nImpF = 0;
while (p < b.length) {
  const id = b[p++]; const len = rd(); const end = p + len;
  if (id === 1) { const n = rd(); for (let i = 0; i < n; i++) { p++; const np = rd(); const ps = []; for (let j = 0; j < np; j++) ps.push(b[p++]); const nr = rd(); const rs = []; for (let j = 0; j < nr; j++) rs.push(b[p++]); types.push({ ps, rs }); } }
  else if (id === 2) { const n = rd(); for (let i = 0; i < n; i++) { const l1 = rd(); p += l1; const l2 = rd(); p += l2; const k = b[p++]; if (k === 0) { rd(); nImpF++; } else if (k === 1) { p++; const f = b[p++]; rd(); if (f & 1) rd(); } else if (k === 2) { const f = b[p++]; rd(); if (f & 1) rd(); } else if (k === 3) { p += 2; } } }
  else if (id === 3) { const n = rd(); for (let i = 0; i < n; i++) funcs.push(rd()); }
  else if (id === 7) { const n = rd(); for (let i = 0; i < n; i++) { const l = rd(); const nm = b.slice(p, p + l).toString(); p += l; const k = b[p++]; const ix = rd(); if (k === 0) exportsF[nm] = ix; } }
  p = end;
}
const nm = { 0x7f: 'i32', 0x7e: 'i64', 0x7d: 'f32', 0x7c: 'f64' };
for (const n of ['vibe_wasm_event', 'vibe_wasm_audio', 'vibe_wasm_init']) { const ix = exportsF[n]; if (ix === undefined) { console.log(n, 'missing'); continue; } const t = types[funcs[ix - nImpF]]; console.log(n, '(' + t.ps.map(x => nm[x]).join(',') + ')'); }
