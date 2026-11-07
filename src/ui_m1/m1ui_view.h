#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "m1ui_model.h"
#include "m1ui_extras.h"

namespace m1ui {

#define KORX_DARK_MODE 0
static uint16_t fb[480 * 320];
static const uint16_t BG = KORX_DARK_MODE ? 0x1022 : 0xB6D5;
static const uint16_t INK = KORX_DARK_MODE ? 0xB6D5 : 0x08A2;
static const uint16_t GRID = KORX_DARK_MODE ? 0x1C44 : 0x9E33;
static const uint16_t INV_BG = INK;
static const uint16_t INV_FG = BG;
static const uint16_t BEZEL = KORX_DARK_MODE ? 0x18A3 : 0x2104;
static const uint16_t KEYTXT = INV_FG;

struct Meters { float l = 0, r = 0; int voices = 0; };
inline uint16_t* framebuffer() { return fb; }

inline void rect(int x,int y,int w,int h,uint16_t c) {
    if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;} if(x+w>480)w=480-x; if(y+h>320)h=320-y;
    if(w<=0||h<=0)return; for(int yy=y;yy<y+h;++yy)for(int xx=x;xx<x+w;++xx)fb[yy*480+xx]=c;
}
inline void line(int x0,int y0,int x1,int y1,uint16_t c) {
    int dx=x1>x0?x1-x0:x0-x1,sx=x0<x1?1:-1,dy=y1>y0?y1-y0:y0-y1,sy=y0<y1?1:-1,e=dx-dy;
    for(;;){if(x0>=0&&x0<480&&y0>=0&&y0<320)fb[y0*480+x0]=c;if(x0==x1&&y0==y1)break;int e2=e<<1;if(e2>-dy){e-=dy;x0+=sx;}if(e2<dx){e+=dx;y0+=sy;}}
}

// Direct ASCII table: entry 0 is space and entry 94 is '~'; each column has 8 row bits.
static const uint8_t font5x8[95][5] = {
{0,0,0,0,0},
{0,0,0x5f,0,0},
{0,7,0,7,0},
{0x14,0x7f,0x14,0x7f,0x14},
{0x24,0x2a,0x7f,0x2a,0x12},
{0x23,0x13,8,0x64,0x62},
{0x36,0x49,0x55,0x22,0x50},
{0,5,3,0,0},
{0,0x1c,0x22,0x41,0},
{0,0x41,0x22,0x1c,0},
{0x14,8,0x3e,8,0x14},
{8,8,0x3e,8,8},
{0,0x50,0x30,0,0},
{8,8,8,8,8},
{0,0x60,0x60,0,0},
{0x20,0x10,8,4,2},
{0x3e,0x51,0x49,0x45,0x3e},
{0,0x42,0x7f,0x40,0},
{0x62,0x51,0x49,0x49,0x46},
{0x22,0x41,0x49,0x49,0x36},
{0x18,0x14,0x12,0x7f,0x10},
{0x2f,0x49,0x49,0x49,0x31},
{0x3e,0x49,0x49,0x49,0x32},
{1,1,0x71,9,7},
{0x36,0x49,0x49,0x49,0x36},
{0x26,0x49,0x49,0x49,0x3e},
{0,0x36,0x36,0,0},
{0,0x56,0x36,0,0},
{8,0x14,0x22,0x41,0},
{0x14,0x14,0x14,0x14,0x14},
{0x41,0x22,0x14,8,0},
{2,1,0x51,9,6},
{0x3e,0x41,0x5d,0x59,0x4e},
{0x7e,9,9,9,0x7e},
{0x7f,0x49,0x49,0x49,0x36},
{0x3e,0x41,0x41,0x41,0x22},
{0x7f,0x41,0x41,0x22,0x1c},
{0x7f,0x49,0x49,0x49,0x41},
{0x7f,9,9,9,1},
{0x3e,0x41,0x49,0x49,0x7a},
{0x7f,8,8,8,0x7f},
{0,0x41,0x7f,0x41,0},
{0x20,0x40,0x41,0x3f,1},
{0x7f,8,0x14,0x22,0x41},
{0x7f,0x40,0x40,0x40,0x40},
{0x7f,2,0x0c,2,0x7f},
{0x7f,4,8,0x10,0x7f},
{0x3e,0x41,0x41,0x41,0x3e},
{0x7f,9,9,9,6},
{0x3e,0x41,0x51,0x21,0x5e},
{0x7f,9,0x19,0x29,0x46},
{0x46,0x49,0x49,0x49,0x31},
{1,1,0x7f,1,1},
{0x3f,0x40,0x40,0x40,0x3f},
{0x1f,0x20,0x40,0x20,0x1f},
{0x3f,0x40,0x38,0x40,0x3f},
{0x63,0x14,8,0x14,0x63},
{7,8,0x70,8,7},
{0x61,0x51,0x49,0x45,0x43},
{0,0x7f,0x41,0,0},
{2,4,8,0x10,0x20},
{0,0,0x41,0x7f,0},
{4,2,1,2,4},
{0x40,0x40,0x40,0x40,0x40},
{0,1,2,0,0},
{0x20,0x54,0x54,0x54,0x78},
{0x7f,0x48,0x44,0x44,0x38},
{0x38,0x44,0x44,0x44,0x20},
{0x38,0x44,0x44,0x48,0x7f},
{0x38,0x54,0x54,0x54,0x18},
{8,0x7e,9,1,2},
{0x18,0xa4,0xa4,0xa4,0x7c},
{0x7f,8,4,4,0x78},
{0,0x44,0x7d,0x40,0},
{0x40,0x80,0x84,0x7d,0},
{0x7f,0x10,0x28,0x44,0},
{0,0x41,0x7f,0x40,0},
{0x7c,4,0x18,4,0x78},
{0x7c,8,4,4,0x78},
{0x38,0x44,0x44,0x44,0x38},
{0xfc,0x24,0x24,0x24,0x18},
{0x18,0x24,0x24,0x18,0xfc},
{0x7c,8,4,4,8},
{0x48,0x54,0x54,0x54,0x20},
{4,0x3f,0x44,0x40,0x20},
{0x3c,0x40,0x40,0x20,0x7c},
{0x1c,0x20,0x40,0x20,0x1c},
{0x3c,0x40,0x30,0x40,0x3c},
{0x44,0x28,0x10,0x28,0x44},
{0x1c,0xa0,0xa0,0xa0,0x7c},
{0x44,0x64,0x54,0x4c,0x44},
{0,8,0x36,0x41,0},
{0,0x7f,0,0,0},
{0,0x41,0x36,8,0},
{2,1,2,4,2},
};
static_assert(sizeof(font5x8) / sizeof(font5x8[0]) == 95, "ASCII font must contain 95 entries");
inline uint8_t glyph(char ch,int col){return (ch>=32&&ch<=126&&col>=0&&col<5)?font5x8[(int)ch-32][col]:0;}

// Legacy glyph code retained only as a reference for this round's replacement.
inline uint8_t legacyGlyph(char ch,int col) {
    if(ch>='a'&&ch<='z')ch=(char)(ch-'a'+'A');
    static const uint8_t g[37][5]={
      {0,0,0,0,0},{0x3E,0x51,0x49,0x45,0x3E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},{0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,9,9,9,1},{0x3E,0x41,0x49,0x49,0x7A},{0x7F,8,8,8,0x7F},{0,0x41,0x7F,0x41,0},{0x20,0x40,0x41,0x3F,1},{0x7F,8,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},{0x7F,2,0x0C,2,0x7F},{0x7F,4,8,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},{0x7F,9,9,9,6},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,9,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},{1,1,0x7F,1,1},{0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,8,0x14,0x63},{7,8,0x70,8,7},{0x61,0x51,0x49,0x45,0x43},{0x3E,0x45,0x49,0x51,0x3E},{0x42,0x41,0x7F,0x40,0x40},{0x62,0x51,0x49,0x49,0x46},{0x22,0x41,0x49,0x49,0x36},{0x18,0x14,0x12,0x7F,0x10},{0x2F,0x49,0x49,0x49,0x31},{0x3E,0x49,0x49,0x49,0x32},{1,0x71,9,5,3},{0x36,0x49,0x49,0x49,0x36},{0x26,0x49,0x49,0x49,0x3E}};
    if(ch>='0'&&ch<='9')return g[1+ch-'0'][col]; if(ch>='A'&&ch<='Z')return g[11+ch-'A'][col];
    static const uint8_t dash[5]={8,8,8,8,8},colon[5]={0,0x36,0x36,0,0},dot[5]={0,0x40,0x60,0,0},star[5]={0x14,8,0x3E,8,0x14},slash[5]={0x20,0x10,8,4,2},plus[5]={8,8,0x3E,8,8};
    if(ch=='-')return dash[col];if(ch==':')return colon[col];if(ch=='.')return dot[col];if(ch=='*')return star[col];if(ch=='/')return slash[col];if(ch=='+')return plus[col];return 0;
}
inline int tWidth(const char*s,int sc=1){int n=0;while(*s++)++n;return n*6*sc;}
inline void text(int x,int y,const char*s,uint16_t c,int sc=1,bool inv=false){int w=tWidth(s,sc);if(inv)rect(x-2,y-2,w+3,8*sc+4,INV_BG);for(;*s;++s,x+=6*sc)for(int k=0;k<5;++k)for(int r=0;r<8;++r)if(glyph(*s,k)&(1<<r))rect(x+k*sc,y+r*sc,sc,sc,c);}
inline void textC(int cx,int y,const char*s,uint16_t c,int sc=1){text(cx-tWidth(s,sc)/2,y,s,c,sc);}
inline void upper(const char*in,char*out,int n){int i=0;for(;in[i]&&i<n-1;++i){char c=in[i];out[i]=(c>='a'&&c<='z')?(char)(c-'a'+'A'):c;}out[i]=0;}
inline int pnorm(const m1::ParamDef*p,const Model&m){if(!p||p->hi<=p->lo)return 0;int v=m1::paramGet(*p,block(m));return ((v-p->lo)*100)/(p->hi-p->lo);}
inline int meter100(float value){
    uint32_t u=0; memcpy(&u,&value,sizeof(u)); if(u&0x80000000u)return 0;
    int e=(int)((u>>23)&255)-127-23; uint32_t mant=(u&0x7FFFFFu)|0x800000u;
    uint64_t n=(uint64_t)mant*100u; if(e>=0){if(e>8)return 100;n<<=e;}else{int s=-e;if(s>63)return 0;n>>=s;}
    return n>100?100:(int)n;
}

inline void strip(const Model&m){rect(10,30,460,1,GRID);char title[16];if(m.nGroups)upper(m.groupName[m.group],title,16);else strcpy(title,"READY");text(12,8,"KORX",INK);text(70,8,title,INK);char p[12];snprintf(p,sizeof(p),"%02d/%02d",m.group+1,m.nGroups);text(425,8,p,INK);for(int i=0;i<8;++i)rect(12+i*57,34,48,3,(i==m.group%8)?INK:GRID);}
inline void params(const Model&m,bool env,bool filter){for(int i=0;i<4;++i){const m1::ParamDef*p=slotParam(m,i);if(!p)continue;char a[16],v[16];snprintf(a,sizeof(a),"%s",p->name);float ignored=0;m1::paramFormat(*p,block(m),v,sizeof(v),&ignored);int x=10+i*115,sel=i==0;rect(x,44,108,154,sel?INV_BG:BG);rect(x,44,108,1,GRID);rect(x,197,108,1,GRID);text(x+5,51,a,sel?INV_FG:INK);text(x+5,69,v,sel?INV_FG:INK,2);rect(x+5,101,98,7,GRID);rect(x+5,101,pnorm(p,m)*98/100,7,INK);}
    if(env){int x=16,y=211;rect(10,205,460,65,GRID);rect(12,207,456,61,BG);int px[5]={x,x+90,x+190,x+300,464},py[5];for(int i=0;i<5;++i){const m1::ParamDef*p=slotParam(m,i<4?i:3);int z=p?m1::paramGet(*p,block(m)):0;py[i]=y+43-(z+99)*43/198;if(py[i]<y)py[i]=y;if(py[i]>y+43)py[i]=y+43;if(i){line(px[i-1],py[i-1],px[i],py[i],INK);line(px[i-1],py[i-1]+1,px[i],py[i]+1,INK);}}}
    else if(filter){rect(10,205,460,65,GRID);rect(12,207,456,61,BG);for(int i=0;i<16;++i){int x=16+i*29;int z=pnorm(slotParam(m,0),m)*(i+1)/32;line(x,263,x,263-z,INK);line(x+1,263,x+1,263-z,INK);}}
}
inline void play(const Model&m,const Meters&mt){char n[11],num[8];getName(m,n);int cur=m.mode?m.combi:m.program;snprintf(num,sizeof(num),"A%02d",cur);text(14,48,m.mode?"COMBINATION":"PROGRAM",INK);text(14,72,num,INK,2);text(70,65,n,INK,3);if(m.edited)text(430,74,"*",INK,2);if(m.comparing)text(400,50,"CMP",INK);
    if(!m.mode){char value[16],summary[52];summary[0]=0;for(int i=0;i<3;++i){int idx=(m.nGroups>1?m.groupFirst[1]:0)+i;if(idx<m.nParams){m1::paramFormat(m.params[idx],block(m),value,sizeof(value),nullptr);if(i)strcat(summary," / ");strcat(summary,value);}}int sc=tWidth(summary,2)>430?1:2;text(14,100,summary,INK,sc);rect(14,115,204,8,GRID);rect(14,115,mt.voices*204/16,8,INK);text(14,140,"VOICE USE",INK);rect(100,140,350,6,GRID);rect(100,140,mt.voices*350/16,6,INK);text(14,154,"L / R",INK);rect(100,154,350,6,GRID);rect(100,154,meter100(mt.l)*350/100,6,INK);rect(100,164,meter100(mt.r)*350/100,6,INK);}
    else for(int i=0;i<8;++i){int y=102+i*18,pn=combiTimbreProgram(m,i),lv=combiTimbreLevel(m,i);char t[16],nm[11];snprintf(t,sizeof(t),"%c %02d",'A'+i,pn);programName(m,pn,nm);text(14,y,t,INK);text(62,y,nm,INK);rect(190,y+1,200,5,GRID);rect(190,y+1,lv*200/99,5,INK);text(405,y,combiTimbreOn(m,i)?"ON":"--",INK);}
}
inline bool fontTestMode=false;
inline void fontTest(){rect(0,0,480,320,BG);text(8,8,"ASCII 32..126 SCALE 1",INK);char row[33];for(int off=0;off<95;off+=32){int n=0;for(int j=0;j<32&&off+j<95;++j)row[n++]=(char)(32+off+j);row[n]=0;text(8,26+(off/32)*10,row,INK,1);}text(8,60,"PRG - PRG + COMBI COMPARE",INK);text(8,70,"PAGE - PAGE + KORX",INK);text(8,80,"Piano16' Brass1&2 E.Piano1",INK);text(8,96,"ASCII 32..126 SCALE 3",INK);for(int off=0;off<95;off+=10){int n=0;for(int j=0;j<10&&off+j<95;++j)row[n++]=(char)(32+off+j);row[n]=0;text(8,112+(off/10)*20,row,INK,3);}}
inline void draw(const Model&m,const Meters&mt){rect(0,0,480,320,BEZEL);rect(6,4,468,312,BG);for(int y=4;y<282;y+=8)for(int x=6;x<474;x+=8)rect(x,y,1,1,GRID);if(fontTestMode){fontTest();return;}strip(m);if(m.group==0)play(m,mt);else{bool e=strstr(m.groupName[m.group],"EG")!=nullptr;bool f=strstr(m.groupName[m.group],"VDF")!=nullptr&&!e;params(m,e,f);}char c[25][8];captions(m,c);for(int i=0;i<6;++i){int x=6+i*79,s=i==3&&m.comparing;rect(x,282,73,34,s?INV_BG:BEZEL);textC(x+36,294,c[i],s?INV_FG:KEYTXT);}}
inline bool fontCellSelfTest(int ch,int x,int y){for(int k=0;k<5;++k)for(int r=0;r<8;++r){bool got=fb[(y+r)*480+x+k]==INK;bool want=(font5x8[ch-32][k]&(1<<r))!=0;if(got!=want)return false;}return true;}
inline bool fontCharSelfTest(int ch){if(ch<32||ch>126)return false;rect(0,200,480,16,BG);char s[2]={(char)ch,0};text(8,202,s,INK);return fontCellSelfTest(ch,8,202);}
inline bool fontLiteralSelfTest(int which){static const char* ss[]={"Piano16'","Universe","Brass1&2","Octave"};if(which<0||which>=4)return false;const char*s=ss[which];rect(0,200,480,16,BG);text(8,202,s,INK);for(int i=0;s[i];++i)if(!fontCellSelfTest((unsigned char)s[i],8+i*6,202))return false;return true;}
inline int fontSelfTest(){fontTestMode=true;for(int ch=32;ch<=126;++ch)if(!fontCharSelfTest(ch))return 0;for(int i=0;i<4;++i)if(!fontLiteralSelfTest(i))return 0;return 1;}
inline void fillLeds(const Model&m,uint8_t*b){memset(b,0,384);uint32_t*u=(uint32_t*)b;float*f=(float*)b;for(int i=0;i<6&&i<m.nGroups;++i){u[42+i]=0x245A7A;f[67+i]=(i==m.group)?1.0f:0.3f;}if(m.comparing){u[35]=0x245A7A;f[60]=1.0f;}}
}
