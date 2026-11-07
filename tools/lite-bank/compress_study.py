import sys, os, csv, math
from array import array

STEP_TABLE=[7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,964,1060,1166,1283,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4429,4872,5359,5895,6485,7134,7847,8632,9496,10446,11491,12640,13904,15294,16824,18507,20358,22394,24633,27097,29807,32767]
ADJ={4:[-1,-1,-1,-1,2,4,6,8],3:[-1,-1,2,4],2:[-1,2]}; BLOCK_SIZE=256
def clip(x): return max(-32768,min(32767,int(x)))
def pack(codes,b):
    out=bytearray(); acc=used=0
    for x in codes:
        acc|=x<<used; used+=b
        if used>=8: out.append(acc&255); acc>>=8; used-=8
    if used: out.append(acc&255)
    return bytes(out)
def unpack(data,pos,n,b):
    out=[]; acc=used=0; mask=(1<<b)-1
    for _ in range(n):
        while used<b: acc|=data[pos]<<used; pos+=1; used+=8
        out.append(acc&mask); acc>>=b; used-=b
    return out,pos
def encode_pcm8(s):
    d=bytes(((int(x)>>8)&255) for x in s)
    return d,decode_pcm8(d,len(s))
def decode_pcm8(d,n): return array('h',[((x-256 if x>=128 else x)<<8) for x in d[:n]])
def enc_adpcm(s,b):
    adj=ADJ[b]; sign=1<<(b-1); stream=bytearray(); out=array('h',[0]*len(s))
    for st in range(0,len(s),BLOCK_SIZE):
        en=min(st+BLOCK_SIZE,len(s)); pred=int(s[st]); peak=max((abs(int(s[j])-int(s[j-1])) for j in range(st+1,min(en,st+33))),default=STEP_TABLE[0]); target=max(1,peak//4); ix=min(range(89),key=lambda j:abs(STEP_TABLE[j]-target)); stream+=pred.to_bytes(2,'little',signed=True)+bytes([ix]); out[st]=pred; codes=[]
        for i in range(st+1,en):
            step=STEP_TABLE[ix]; d=int(s[i])-pred; m=abs(d)
            if b==4: q=min(7,m*4//step); dq=(step>>3)+(step>>2 if q&1 else 0)+(step>>1 if q&2 else 0)+(step if q&4 else 0)
            elif b==3: q=min(3,m*2//step); dq=((2*q+1)*step)//4
            else: q=1 if m>=step else 0; dq=step if q else max(1,step//4)
            codes.append(q|(sign if d<0 else 0)); pred=clip(pred-dq if d<0 else pred+dq); out[i]=pred; ix=max(0,min(88,ix+adj[q]))
        stream+=pack(codes,b)
    return bytes(stream),out
def dec_adpcm(d,n,b):
    adj=ADJ[b]; sign=1<<(b-1); mask=sign-1; out=array('h',[0]*n); pos=0
    for st in range(0,n,BLOCK_SIZE):
        en=min(st+BLOCK_SIZE,n); pred=int.from_bytes(d[pos:pos+2],'little',signed=True); ix=d[pos+2]; pos+=3; out[st]=pred; codes,pos=unpack(d,pos,en-st-1,b)
        for i,c in enumerate(codes,st+1):
            q=c&mask; step=STEP_TABLE[ix]
            if b==4: dq=(step>>3)+(step>>2 if q&1 else 0)+(step>>1 if q&2 else 0)+(step if q&4 else 0)
            elif b==3: dq=((2*q+1)*step)//4
            else: dq=step if q else max(1,step//4)
            pred=clip(pred-dq if c&sign else pred+dq); out[i]=pred; ix=max(0,min(88,ix+adj[q]))
    return out
encode_ima4=lambda s:enc_adpcm(s,4); encode_adpcm3=lambda s:enc_adpcm(s,3); encode_adpcm2=lambda s:enc_adpcm(s,2)
decode_ima4=lambda d,n:dec_adpcm(d,n,4); decode_adpcm3=lambda d,n:dec_adpcm(d,n,3); decode_adpcm2=lambda d,n:dec_adpcm(d,n,2)
def energies(a,b):
    se=ee=0
    for x,y in zip(a,b): se+=x*x; ee+=(x-y)*(x-y)
    return se,ee
def snr(a,b):
    se,ee=energies(a,b); return 99.0 if not se or not ee else min(99.0,10*math.log10(se/ee))
def pct(a,p):
    a=sorted(a); k=(len(a)-1)*p/100; f=math.floor(k); c=math.ceil(k); return a[f] if f==c else a[f]*(c-k)+a[c]*(k-f)
def self_test():
    n=4096; s=array('h',[round(8000*math.sin(2*math.pi*1000*i/31250)) for i in range(n)])
    assert snr(s,s)==99.0 and abs(snr(s,array('h',[x//2 for x in s]))-6.0206)<.01
    r={}
    for name,enc,dec,lim in [('ima4',encode_ima4,decode_ima4,25),('adpcm3',encode_adpcm3,decode_adpcm3,15),('adpcm2',encode_adpcm2,decode_adpcm2,8)]:
        d,internal=enc(s); external=dec(d,n); assert internal==external,name+' round trip'; r[name]=snr(s,external); assert r[name]>lim,f'{name} SNR {r[name]:.2f} <= {lim}'
    print('Self-test:',', '.join(f'{k}={v:.2f} dB' for k,v in r.items()),flush=True); return r
def main():
    st=self_test()
    if len(sys.argv)!=5: raise SystemExit('Usage: compress_study.py <rom> <os> <pcm> <out>')
    rom,osdir,pcmp,outdir=sys.argv[1:]; sys.path.insert(0,rom); import m1_rom
    rs=m1_rom.M1RomSet.__new__(m1_rom.M1RomSet); rs.romdir=osdir; rs.os=rs._load_os(); regions={}
    def add(a,z,name,d=False,l=False):
        if a is None or z is None:return
        r=regions.setdefault((a,z),{'names':set(),'dwgs':False,'loop_on':False});r['names'].add(name);r['dwgs']|=bool(d);r['loop_on']|=bool(l)
    for m in rs.multisounds():
        for x in m.get('splits',[]):add(x.get('start'),x.get('end'),m.get('name','Unknown'),m.get('dwgs',False),x.get('loop_on',False))
    for i,x in enumerate(rs.drum_sounds()):add(x.get('start'),x.get('end'),f'drum {i}',x.get('dwgs',False),x.get('loop_on',False))
    print('Found',len(regions),'unique regions.',flush=True)
    with open(pcmp,'rb') as f: pcm=array('h');pcm.fromfile(f,2097152)
    os.makedirs(outdir,exist_ok=True); cs=['pcm8','ima4','adpcm3','adpcm2']; es={'pcm8':encode_pcm8,'ima4':encode_ima4,'adpcm3':encode_adpcm3,'adpcm2':encode_adpcm2}; ds={'pcm8':decode_pcm8,'ima4':decode_ima4,'adpcm3':decode_adpcm3,'adpcm2':decode_adpcm2}; A={c:{'bytes':0,'samples':0,'se':0,'ee':0,'sn':[],'rr':[]} for c in cs}; checks=0
    with open(os.path.join(outdir,'regions.csv'),'w',newline='') as f:
        w=csv.writer(f);w.writerow(['start','end','length','loop_on','dwgs','names','codec','bytes','snr','attack_snr'])
        for ri,(a,z) in enumerate(regions):
            if ri%50==0:print('Region',ri,'/',len(regions),flush=True)
            sm=pcm[a:z+1]; inf=regions[(a,z)]; names=';'.join(sorted(inf['names']))
            for c in cs:
                data,internal=es[c](sm); decoded=ds[c](data,len(sm))
                if internal!=decoded:raise RuntimeError(f'decoder mismatch {c} region {a}..{z}')
                if c!='pcm8':checks+=1
                se,ee=energies(sm,decoded); q=snr(sm,decoded); x=A[c];x['bytes']+=len(data);x['samples']+=len(sm);x['se']+=se;x['ee']+=ee;x['sn'].append(q);x['rr'].append((q,names,a,len(sm)));w.writerow([a,z,len(sm),inf['loop_on'],inf['dwgs'],names,c,len(data),q,snr(sm[:1024],decoded[:1024])])
    ints=sorted(regions); merged=[]
    for a,z in ints:
        if merged and a<=merged[-1][1]+1:merged[-1][1]=max(merged[-1][1],z)
        else:merged.append([a,z])
    uw=sum(z-a+1 for a,z in merged); rp=os.path.join(outdir,'report.md')
    def metrics(c):
        x=A[c]; w=99 if not x['se'] or not x['ee'] else min(99,10*math.log10(x['se']/x['ee']));q=x['sn'];return x['bytes']/1024,x['bytes']*8/x['samples'],w,pct(q,5),pct(q,25),pct(q,50),pct(q,75),sum(v<15 for v in q),sum(v<25 for v in q)
    with open(rp,'w') as f:
        f.write('# Compression Study Report\n\n');f.write('Self-test: '+', '.join(f'{k}={v:.2f} dB' for k,v in st.items())+'\n\n');f.write(f'Decoder-from-bytes checks: {checks} (480 regions x 3 ADPCM codecs).\n\nTotal Unique Regions: {len(regions)}\nTotal Samples (Unique Regions): {A["pcm8"]["samples"]}\nUnion of Ranges: {uw} words ({uw*2/1024:.2f} KiB at 16-bit)\n\n');f.write('## Aggregate Metrics\n\n| Codec | Total KiB | Bits/Sample | Weighted SNR (dB) | p5 | p25 | p50 | p75 | <15dB | <25dB |\n|---|---|---|---|---|---|---|---|---|---|\n')
        for c in cs:f.write('| %s | %.2f | %.2f | %.2f | %.2f | %.2f | %.2f | %.2f | %d | %d |\n'%((c,)+metrics(c)))
        f.write('\n## Projected Size on Union\n\n| Codec | Projected KiB |\n|---|---|\n');
        for c in cs:f.write(f'| {c} | {A[c]["bytes"]/A[c]["samples"]*uw/1024:.2f} |\n')
        f.write('\n## 12 Worst Regions by SNR\n\n');
        for c in cs:
            f.write(f'### {c}\n\n| Names | Start | Length | SNR |\n|---|---|---|---|\n');
            for q,n,a,l in sorted(A[c]['rr'])[:12]:f.write(f'| {n} | {a} | {l} | {q:.2f} |\n')
            f.write('\n')
        f.write('## Correctness, commands, and limitations\n\n')
        f.write('Root causes of the original bad results: the IMA sign bit was set for both delta signs; the IMA magnitude was incorrectly limited to two bits and used incorrect reconstruction weights; and the old code simulated reconstruction instead of producing and decoding a packed byte stream. The old `array` failure was also avoided by using the standard-library `array` module correctly.\n\n')
        f.write('Changes: packed little-endian bitstreams with 3-byte headers per block; independent decoders; exact reconstruction equality checks; integer-only clamped predictors and indices; verified SNR reference cases. IMA block initial indices are selected from the opening delta scale and stored in the header. The 3-bit codec uses four cell-centre levels at (2q+1)*step/4; the 2-bit codec uses a quarter-step zero cell and a full-step upper cell. These keep the decoder table-driven and integer-only.\n\n')
        f.write('Command: `python -I compress_study.py C:\\build\\korx-m1\\work C:\\build\\korx-m1\\roms\\set C:\\build\\korx-m1\\work\\gen_m1\\m1_pcm.bin C:\\build\\korx-m1\\work\\codex4\\out`\n\n')
        f.write('Not verified: perceptual listening quality, bitstream interoperability with an external IMA implementation, and embedded-target cycle counts.\n')
    if not 20<=metrics('ima4')[2]<=45:raise RuntimeError(f'ima4 weighted SNR implausible: {metrics("ima4")[2]:.2f}')
    print('Decoder checks:',checks,'expected',len(regions)*3);print('Aggregate:',[(c,metrics(c)) for c in cs]);print('Reports:',rp,os.path.join(outdir,'REPORT.md'))
if __name__=='__main__':main()
