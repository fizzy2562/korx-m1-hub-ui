import struct,sys
b=open(sys.argv[1],'rb').read()
n=struct.unpack_from('<H',b,4)[0]
lo,hi=int(sys.argv[2]),int(sys.argv[3])
print("nregions",n)
for i in range(n):
    e=b[16+24*i:16+24*i+24]
    s,en=struct.unpack_from('<II',e,0)
    if en>=lo and s<=hi:
        print(i,"start",s,"end",en,"len",en-s+1,"bytes12-15",list(e[12:16]),"codec",e[13],"off",struct.unpack_from('<I',e,16)[0],"dlen",struct.unpack_from('<I',e,20)[0], "raw8-11",list(e[8:12]))
