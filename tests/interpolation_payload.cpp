// Original private guest sprite writer payload proof by djultra64.
#include "recomp.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>
#include <cstdlib>
extern "C" void func_800A87C4(uint8_t*,recomp_context*);
extern "C" void do_break(uint32_t pc){printf("Unexpected native break%x\n",pc);std::abort();}
extern "C" void switch_error(const char*,uint32_t pc,uint32_t){printf("Unexpected native switch%x\n",pc);std::abort();}
int main(){std::vector<uint8_t> ram(9<<20);auto*rdram=ram.data();constexpr uint32_t sprite=0x80200000,dl=0x80300000,verts=0x80500000,sp=0x807fe000,basis=0x80160000;
 MEM_W(0,S32(0x800de400))=1;MEM_W(0,S32(0x800de404))=1;MEM_W(0,S32(0x800de408))=2;MEM_W(0,S32(0x800de40c))=4;
 MEM_W(0,S32(basis))=0xbf800000;MEM_W(0,S32(basis+16+4))=0x3f800000;
 unsigned cases=0,max_bytes=0;
 for(unsigned format:{2u,3u,0x30u,0x31u,0x40u,0x41u,0x42u,0x43u,0x50u,0x51u})for(unsigned width:{8u,15u,64u,126u,128u,144u,146u,256u,512u})for(unsigned height:{1u,15u,64u,78u,177u,256u})for(unsigned flags:{0u,1u,2u,4u,8u,16u,0xffffu}){
  unsigned row=(width*(1u<<(format&15))+1)/2;row=(row+7)&~7u;if(row>2048)continue;unsigned slices=(height+2048/row-1)/(2048/row),bound=(64+32*slices)*8;
  MEM_BU(0,S32(sprite))=format;MEM_H(0,S32(sprite+2))=(format>>4)==3?((format&15)?0x210:0x30):16;MEM_H(0,S32(sprite+6))=width;MEM_H(0,S32(sprite+8))=height;MEM_H(0,S32(sprite+14))=row;
  MEM_W(0,S32(0x80110898))=S32(dl);MEM_W(0,S32(0x801108a0))=S32(verts);MEM_W(0,S32(0x80118fc8))=0;
  recomp_context ctx{};ctx.f_odd=&ctx.f0.u32h;ctx.r29=S32(sp);ctx.r4=S32(0x80110898);ctx.r5=S32(sprite);ctx.r6=0;ctx.r7=0;MEM_W(0x10,ctx.r29)=0;MEM_W(0x14,ctx.r29)=S32(basis);MEM_W(0x18,ctx.r29)=S32(basis+16);MEM_W(0x1c,ctx.r29)=flags;
  func_800A87C4(rdram,&ctx);unsigned bytes=uint32_t(MEM_W(0,S32(0x80110898)))-dl,vertex_bytes=uint32_t(MEM_W(0,S32(0x801108a0)))-verts;
  if(bytes>bound||vertex_bytes>slices*64){printf("FAIL fmt%x %ux%u flags%x bytes%u bound%u vertices%u slices%u\n",format,width,height,flags,bytes,bound,vertex_bytes,slices);return 1;}max_bytes=bytes>max_bytes?bytes:max_bytes;cases++;
 }
 printf("PASS: %u original A87C4 format/dimension/state cases; maxDL%u bytes; setup64+32commands/slice and4vertices/slice bounds hold\n",cases,max_bytes);
}
