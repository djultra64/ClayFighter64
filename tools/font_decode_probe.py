#!/usr/bin/env python3
"""Decode private original ClayFighter font with its native MIPS codec; djultra64."""
import pathlib,re,subprocess
R=pathlib.Path(__file__).resolve().parents[1]; O=R/'analysis/font_private'; O.mkdir(exist_ok=True); (O/'.gitignore').write_text('*\n')
funcs={}
for p in (R/'generated/guest').glob('funcs_*.c'):
 for m in re.finditer(r'RECOMP_FUNC void (\w+)\([^\n]*\).*?;\}',p.read_text(),re.S):funcs[m[1]]=m[0]
names=['func_800BE590','func_800BE654','func_800BE6BC','func_800BE6C8']
(O/'guest.cpp').write_text('#include "recomp.h"\n#include "funcs.h"\nextern "C" {\n'+'\n'.join(funcs[n] for n in names)+'\n}\n')
h=(R/'tests/sc_decode.cpp').read_text().replace('func_800BD354','func_800BE6C8').replace('packet[0]!=16','packet[0]!=96').replace('ctx.r29=0x807FF000;ctx.r4=dst;ctx.r5=src+12','ctx.r29=S32(0x807FF000);ctx.r4=S32(dst);ctx.r5=S32(src+12)').replace('Original63 codec16','Original63 codec96').replace('codec16 MIPS','codec96 MIPS')
h=h.replace('int main(int argc,char**argv){','''extern "C" void func_800B3C70(uint8_t*,recomp_context*c){static uint32_t p=0x80500000;c->r2=S32(p);p+=(uint32_t(c->r5)+15)&~15u; if(p>=0x80700000)abort();}
extern "C" void func_800B4408(uint8_t*,recomp_context*){}
extern "C" void func_800B8FC8(uint8_t*,recomp_context*){abort();}
extern "C" void switch_error(const char*,uint32_t,uint32_t){abort();}
int main(int argc,char**argv){''')
h=h.replace('const uint32_t src=0x80200000,dst=0x80400000;','const uint32_t src=0x80200000,dst=0x80400000;\n FILE*rom=fopen("probe-output/base.z64","rb"); fseek(rom,0x1000,SEEK_SET); for(size_t i=0x400;i<0xf8d80;i++)rdram[i^3]=fgetc(rom);fclose(rom);')
(O/'decode.cpp').write_text(h)
rom=(R/'probe-output/base.z64').read_bytes(); packet=rom[0xB68EEC:0xB68EEC+0x2B9B]; (O/'packet.bin').write_bytes(packet)
sdk='/tmp/Blast Corps source review b48mc3gh/independent SDK/deps/N64ModernRuntime/N64Recomp/include'
subprocess.run(['g++','-O1','-I'+sdk,'-I'+str(R/'generated/guest'),str(O/'guest.cpp'),str(O/'decode.cpp'),'-o',str(O/'decode')],check=True)
subprocess.run([str(O/'decode'),str(O/'packet.bin'),str(O/'font.bin')],cwd=R,check=True,timeout=30)
print((O/'font.bin').read_bytes()[:128].hex())
