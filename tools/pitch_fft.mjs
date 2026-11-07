import fs from 'node:fs';
const bytes = fs.readFileSync(process.argv[2]);
function fft(re, im){ const n=re.length; for(let i=1,j=0;i<n;i++){ let bit=n>>1; for(;j&bit;bit>>=1) j^=bit; j^=bit; if(i<j){[re[i],re[j]]=[re[j],re[i]];[im[i],im[j]]=[im[j],im[i]];} }
 for(let len=2;len<=n;len<<=1){ const ang=-2*Math.PI/len, wr=Math.cos(ang), wi=Math.sin(ang); for(let i=0;i<n;i+=len){ let cr=1,ci=0; for(let k=0;k<len/2;k++){ const ur=re[i+k],ui=im[i+k]; const vr=re[i+k+len/2]*cr-im[i+k+len/2]*ci, vi=re[i+k+len/2]*ci+im[i+k+len/2]*cr; re[i+k]=ur+vr; im[i+k]=ui+vi; re[i+k+len/2]=ur-vr; im[i+k+len/2]=ui-vi; const t=cr*wr-ci*wi; ci=cr*wi+ci*wr; cr=t; } } } }
const notes = [36,48,60,66,71,72,78,83,84,90,95,96,108];
console.log('FFT pitch of program 0 (sine): note expectedHz measuredHz cents');
for (const n of notes) {
  const { instance } = await WebAssembly.instantiate(bytes, {});
  const x = instance.exports; x._initialize && x._initialize(); x.vibe_wasm_init(48000);
  x.vibe_wasm_event(5,0,0,0,0xC0); for (let i=0;i<16;i++) x.vibe_wasm_audio(128,0);
  x.vibe_wasm_event(5,0,0,0,0x90|(n<<8)|(100<<16));
  const N=65536, skip=12000, total=Math.ceil((N+skip)/128)*128, buf=new Float32Array(total);
  for(let b=0;b<total/128;b++){ x.vibe_wasm_audio(128,0); const io=x.vibe_wasm_io(); buf.set(new Float32Array(x.memory.buffer,io+1024,128),b*128); }
  const re=new Float64Array(N), im=new Float64Array(N);
  for(let i=0;i<N;i++){ const w=0.5-0.5*Math.cos(2*Math.PI*i/(N-1)); re[i]=buf[skip+i]*w; }
  fft(re,im);
  const e=440*Math.pow(2,(n-69)/12); const k0=Math.round(e*N/48000);
  let kb=k0, mb=0; for(let k=Math.max(1,k0-40);k<k0+40;k++){ const m=Math.hypot(re[k],im[k]); if(m>mb){mb=m;kb=k;} }
  const a=Math.log(Math.hypot(re[kb-1],im[kb-1])), b=Math.log(mb), c=Math.log(Math.hypot(re[kb+1],im[kb+1]));
  const d=0.5*(a-c)/(a-2*b+c); const f=(kb+d)*48000/N;
  console.log(n, e.toFixed(2), f.toFixed(3), (1200*Math.log2(f/e)).toFixed(2));
}
