// UI/help/effects check for the ROM-free build (144 checks). Run from a work folder that holds src/ (flattened free build sources),
// out/korx-m1-free.wasm and shot.mjs (tools/shot.mjs). Exits non-zero on any failure. Writes shots/*.png.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
const src=fs.readFileSync('src/m1_params.cpp','utf8');
const names=[...src.match(/kFxNames\[34\] = \{([\s\S]*?)\};/)[1].matchAll(/"([^"]+)"/g)].map(x=>x[1]);
const tables={};
for(const m of src.matchAll(/const ParamDef (k\w+)\[8\] = \{([\s\S]*?)\};/g)) tables[m[1]]=[...m[2].replace('DLYH','E("Delay",0,Word,0,500,0), E("Feedback",2,S,-99,99,0), E("High Damp",3,U,0,99,0)').matchAll(/E\("([^"]+)"/g)].map(x=>x[1]);
const tableNames=['kRev','kRev','kRev','kRoom','kRev','kRev','kEr','kEr','kEr','kDly','kDly','kCho','kCho','kFlg','kFlg','kPha','kPha','kTrm','kTrm','kEq','kOd','kDst','kExc','kSym','kRot','kDHall','kDRoom','kDEr','kDDly','kDCho','kDFlg','kDPha','kDTrm'];
export const effects=names.slice(0,33).map((name,i)=>({name,params:tables[tableNames[i]]}));
const view=fs.readFileSync('src/m1ui_view.h','utf8');
const font=[...view.match(/font5x8\[95\]\[5\] = \{([\s\S]*?)\n\};/)[1].matchAll(/\{([^}]+)\}/g)].map(x=>x[1].split(',').map(Number));
const mod=new WebAssembly.Module(fs.readFileSync('out/korx-m1-free.wasm'));assert.deepEqual(WebAssembly.Module.imports(mod),[]);
function boot(){const e=new WebAssembly.Instance(mod,{}).exports;e._initialize?.();e.vibe_wasm_init(48000);return e;}
let e=boot();
const ev=(t,id,down=0,v=0)=>e.vibe_wasm_event(t,id,down,v,0);
const key=id=>{ev(3,id,1);ev(3,id,0);};
const enc=(id,v)=>ev(4,id,0,v);
const frame=()=>{const p=e.vibe_wasm_draw();return new Uint16Array(e.memory.buffer,p,480*320).slice();};
const labels=()=>{const p=e.vibe_wasm_labels();return Array.from({length:25},(_,i)=>Buffer.from(e.memory.buffer,p+i*8,8).toString('ascii').split('\0')[0]);};
function read(x,y,sc=1,len=16,ink=0xB6D5){const f=frame(),out=[];for(let n=0;n<len;n++){const cols=[];for(let k=0;k<5;k++){let b=0;for(let r=0;r<8;r++)if(f[(y+r*sc)*480+x+(n*6+k)*sc]===ink)b|=1<<r;cols.push(b);}const i=font.findIndex(g=>g.every((b,j)=>b===cols[j]));out.push(i<0?'?':String.fromCharCode(i+32));}return out.join('').trim();}
function value(slot){const x=15+slot*115,ink=slot===0?0xB6D5:0x08A2;
 const v=read(x,69,2,8,ink);return v.includes('?')?read(x,69,1,16,ink):v;
}
const region=(slot=0)=>{const f=frame();let b=[];for(let y=69;y<85;y++)for(let x=15+slot*115;x<113+slot*115;x++)b.push(f[y*480+x]);return b.join(',');};
const base=frame(),playLabels=labels();key(17);const hp=frame();assert.notDeepEqual(hp,base);key(17);assert.deepEqual(frame(),base);key(17);key(16);assert.deepEqual(frame(),base);
key(17);key(20);assert.notDeepEqual(frame(),hp);key(18);assert.deepEqual(frame(),hp);key(16);
key(2);key(17);assert.notDeepEqual(frame(),hp);key(16);key(2);key(11);const edit=frame();assert.notDeepEqual(labels(),playLabels);key(17);assert.notDeepEqual(frame(),hp);key(16);assert.deepEqual(frame(),edit);
key(17);enc(0,1);assert.notDeepEqual(frame(),edit);assert.equal(labels()[16],'PLAY');
key(12);key(17);assert.notDeepEqual(frame(),hp);assert.notDeepEqual(frame(),edit);key(16);
for(const cap of labels()) assert(cap.length<=7 && /^[ -~]*$/.test(cap));
console.log('PASS help toggle, restore, paging, live context, dismissal and captions');
for(let mode=0;mode<2;mode++) {
 e=boot();if(mode)key(2);key(12);
 const common=['FX1 Type','FX2 Type','Placement','FX1 L On','FX1 R On','FX2 L On','FX2 R On','FX1 Bal L','FX1 Bal R','FX2 Bal L','FX2 Bal R','Out3 Pan','Out4 Pan'];
 for(let p=0;p<common.length;p+=4){for(let slot=0;slot<4&&p+slot<common.length;slot++){
   assert.equal(read(15+slot*115,51,1,16,slot===0?0xB6D5:0x08A2),common[p+slot]);
   const before=region(slot);enc(slot,1);if(region(slot)===before)enc(slot,-1);assert.notEqual(region(slot),before);
 }if(p+4<common.length)key(20);}
 key(12);for(let i=0;i<34;i++)enc(0,1);assert.equal(value(0),'No Effect');assert.equal(labels()[13],'');
 console.log('PASS common effect routing, switches, balances, pans and No Effect',mode);
}
for(let mode=0;mode<2;mode++)for(let unit=0;unit<2;unit++)for(let t=0;t<33;t++){
 e=boot();if(mode)key(2); // force Double program to exercise the former capacity limit
 if(!mode){key(11);enc(0,-1);enc(0,-1);enc(0,1);}
 key(12);for(let n=0;n<34;n++)enc(unit,-1);for(let n=0;n<t;n++)enc(unit,1);
 // Type names fit their column; read FX1 directly, FX2 checked through its page header and params.
 assert.equal(value(unit),names[t],`type ${t} label`);
 key(13+unit);assert.notEqual(labels()[13+unit],'',`effect ${t} reachable`);
 const expected=effects[t].params;
 for(let p=0;p<expected.length;p+=4){
   for(let slot=0;slot<4&&p+slot<expected.length;slot++) {
     assert.equal(read(15+slot*115,51,1,16,slot===0?0xB6D5:0x08A2),expected[p+slot],`${mode}/${unit}/${t} parameter ${p+slot}`);
     const before=region(slot);enc(slot,1);if(region(slot)===before)enc(slot,-1);assert.notEqual(region(slot),before,`parameter ${p+slot} changes`);
     assert(!value(slot).includes('?'),`readable value ${t}/${p+slot}`);
   }
   if(p+4<expected.length)key(20);
 }
 console.log(`PASS ${mode?'Combi':'Program Double'} FX${unit+1} ${t} ${names[t]}: selected, parameters readable and editable`);
}
e=boot();ev(2,12,1,.8);let peak=0;for(let b=0;b<100;b++){e.vibe_wasm_audio(128,0);for(const off of [1024,2048])for(const v of new Float32Array(e.memory.buffer,e.vibe_wasm_io()+off,128)){assert(Number.isFinite(v));peak=Math.max(peak,Math.abs(v));}}assert(peak>.05);console.log('PASS audio peak',peak);
fs.mkdirSync('shots',{recursive:true});
const data8=Array(8).fill('e:4,6,0,1');
const shots={help_program:[...data8,'e:3,17,1,0'],help_combi:['e:3,2,1,0','e:3,17,1,0'],help_edit_osc:['e:3,11,1,0','e:3,17,1,0'],help_effects:[...data8,'e:3,12,1,0','e:3,17,1,0'],program_fx_line:data8,effects_page_1:[...data8,'e:3,13,1,0'],effects_page_2:[...data8,'e:3,13,1,0','e:3,20,1,0']};
for(const [name,steps] of Object.entries(shots)){const r=spawnSync(process.execPath,['shot.mjs','out/korx-m1-free.wasm',`shots/${name}.png`,...steps,'a:3'],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);const png=fs.readFileSync(`shots/${name}.png`);assert.equal(png.readUInt32BE(16),960);assert.equal(png.readUInt32BE(20),640);console.log('PASS screenshot',name,png.length);}
fs.writeFileSync('effects.json',JSON.stringify(effects,null,2));
console.log('ALL PASS');
