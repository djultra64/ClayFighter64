// Original native sprite slice continuity regression by djultra64.
#include "recomp.h"
#include "sprite_seams.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>
#include <cstdlib>
extern "C" void func_800A87C4(uint8_t*,recomp_context*);
extern "C" void func_800A6520(uint8_t*,recomp_context*);
extern "C" void do_break(uint32_t pc){printf("Unexpected native break%x\n",pc);std::abort();}
extern "C" void switch_error(const char*,uint32_t pc,uint32_t){printf("Unexpected native switch%x\n",pc);std::abort();}
bool sprite_seam_fixture_limit=false;
int main(int argc,char**){
 const bool repair=argc==1;sprite_seam_fixture_limit=repair;std::vector<uint8_t> ram(8<<20);auto*rdram=ram.data();
 constexpr uint32_t sprite=0x80200000,dl=0x80300000,verts=0x80500000,sp=0x807fe000,basis=0x80160000;
 MEM_W(0,S32(0x800de400))=1;MEM_W(0,S32(0x800de404))=1;MEM_W(0,S32(0x800de408))=2;MEM_W(0,S32(0x800de40c))=4;
 MEM_W(0,S32(basis))=0xbf800000;MEM_W(0,S32(basis+20))=0x3f800000;
 MEM_W(0,S32(0x801108cc))=1;MEM_W(0,S32(0x80110880))=8192;MEM_W(0,S32(0x80110890))=4000;
 MEM_W(0,S32(0x801108dc))=S32(0x80150000);MEM_W(0,S32(0x801108e4))=S32(0x80150004);MEM_W(0,S32(0x80150000))=S32(dl);MEM_W(0,S32(0x80150004))=S32(verts);
 unsigned cases=0,all_edges=0,all_discontinuities=0;
 for(auto writer:{func_800A87C4,func_800A6520})for(unsigned format:{2u,3u,0x43u,0x11u})for(unsigned width:{64u,126u,146u})for(unsigned height:{64u,78u,177u})for(unsigned flags:{0u,1u,2u,4u,8u,16u,0xffffu}){
  unsigned row=(width*(1u<<(format&15))+1)/2;row=(row+7)&~7u;
  MEM_BU(0,S32(sprite))=format;const unsigned offset=(format&0x10)?528:16;MEM_H(0,S32(sprite+2))=offset;MEM_H(0,S32(sprite+6))=width;MEM_H(0,S32(sprite+8))=height;MEM_H(0,S32(sprite+14))=row;
  MEM_W(0,S32(0x80110898))=S32(dl);MEM_W(0,S32(0x801108a0))=S32(verts);MEM_W(0,S32(0x80118fc8))=0;
  recomp_context ctx{};ctx.f_odd=&ctx.f0.u32h;ctx.r29=S32(sp);ctx.r4=S32(0x80110898);ctx.r5=S32(sprite);ctx.r6=0;ctx.r7=0;
  MEM_W(0x10,ctx.r29)=0;MEM_W(0x14,ctx.r29)=S32(basis);MEM_W(0x18,ctx.r29)=S32(basis+16);MEM_W(0x1c,ctx.r29)=flags;
  clay::begin_sprite_slices(rdram,&ctx);writer(rdram,&ctx);
  const unsigned bytes=uint32_t(MEM_W(0,S32(0x80110898)))-dl,vertex_bytes=uint32_t(MEM_W(0,S32(0x801108a0)))-verts;
  std::vector<uint8_t> old_geometry(ram.begin()+(verts&0x1fffffffu),ram.begin()+(verts&0x1fffffffu)+vertex_bytes);
  struct Strip{unsigned first=0;uint32_t vertex=0;};std::vector<Strip> strips;unsigned first=0;
  for(unsigned k=0;k<bytes;k+=8){const auto w0=uint32_t(MEM_W(0,S32(dl+k))),w1=uint32_t(MEM_W(4,S32(dl+k)));
   if(w0>>24==0xfd&&(w1&0x1fffffffu)>=((sprite+offset)&0x1fffffffu)){assert((w1&0x1fffffffu)>=((sprite+offset)&0x1fffffffu));first=((w1&0x1fffffffu)-((sprite+offset)&0x1fffffffu))/row;}
   if(w0==0x0400103f)strips.push_back({first,0x80000000u|(w1&0x1fffffffu)});
  }
  unsigned fixed=repair?clay::end_sprite_slices(rdram):0;
  if(repair&&fixed+1!=strips.size()){fprintf(stderr,"helper coverage failure writer%u fmt%x %ux%u flags%x fixed%u strips%zu\n",writer==func_800A87C4?0:1,format,width,height,flags,fixed,strips.size());return 2;}
  for(unsigned i=0;i+1<strips.size();i++){
   const auto a=strips[i].vertex,b=strips[i+1].vertex;
   for(int dim=0;dim<6;dim+=2){assert(MEM_H(dim,S32(a+48))==MEM_H(dim,S32(b)));assert(MEM_H(dim,S32(a+32))==MEM_H(dim,S32(b+16)));}
   const int previous=strips[i].first*32+MEM_H(58,S32(a)),next=strips[i+1].first*32+MEM_H(10,S32(b));
   all_edges++;if(previous!=next)all_discontinuities++;
  }
  // No XYZ, vertex flags, S, colors, geometry count, or final-strip bytes change.
  for(unsigned k=0;k<vertex_bytes;k++)if(k%16!=8&&k%16!=9)assert(ram[(verts&0x1fffffffu)+k]==old_geometry[k]);
  if(vertex_bytes)assert(!memcmp(ram.data()+(verts&0x1fffffffu)+vertex_bytes-64,old_geometry.data()+vertex_bytes-64,64));
  assert(uint32_t(MEM_W(0,S32(0x80110898)))==dl+bytes);assert(uint32_t(MEM_W(0,S32(0x801108a0)))==verts+vertex_bytes);cases++;
 }
 printf("%s: %u original A87C4/A6520 cases, %u sharededges, %u texture discontinuities; postprocessor preserves XYZ/finalstrip/counts; bounded limiter may reslice\n",repair?"GREEN":"RED",cases,all_edges,all_discontinuities);
 return all_discontinuities?1:0;
}
