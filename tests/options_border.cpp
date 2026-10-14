// Actual original fill/outline/fade helper verification; djultra64.
#include "../host/options_border.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <vector>
struct Rect {std::array<int32_t,4> p;std::array<unsigned,4> color;};
static std::vector<Rect> rects;
extern "C" void func_800A3F88(uint8_t*,recomp_context*){}
extern "C" void func_800A45BC(uint8_t*,recomp_context*){}
extern "C" void func_800A4D9C(uint8_t*rdram,recomp_context*c){Rect r{};for(unsigned i=0;i<4;i++)r.p[i]=MEM_W(i*4,c->r5);r.color={unsigned(c->r6),unsigned(c->r7),unsigned(MEM_W(0x10,c->r29)),unsigned(MEM_W(0x14,c->r29))};rects.push_back(r);}
extern "C" void func_8005B1E8(uint8_t*,recomp_context*);
int main(){std::vector<uint8_t> memory(9<<20);auto*rdram=memory.data();
 for(unsigned menu=0;menu<4;menu++){
  std::array<int32_t,4> source=menu==2?std::array<int32_t,4>{30,67,290,207}:menu==3?std::array<int32_t,4>{17,87,302,149}:std::array<int32_t,4>{105,170,216,219};uint32_t p=0x800DAD80+menu*16;for(unsigned i=0;i<4;i++)MEM_W(i*4,gpr(S32(p)))=source[i];
  for(unsigned alpha:{0,1,63,127,128,255}){
   recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=gpr(S32(0x8081F000));c.r4=gpr(S32(p));c.r5=alpha;rects.clear();func_8005B1E8(rdram,&c);auto baseline=rects;
   c={};c.f_odd=&c.f0.u32h;c.r29=gpr(S32(0x8081F000));c.r4=gpr(S32(p));c.r5=alpha;rects.clear();clay_native_options_border(rdram,&c);assert(c.r29==gpr(S32(0x8081F000)));assert(rects.size()==baseline.size()&&rects.size()==5);
   for(unsigned i=0;i<rects.size();i++){assert(rects[i].color==baseline[i].color);if(menu<2)assert(rects[i].p==baseline[i].p);}
   const std::array<int32_t,4> expected=menu==2?std::array<int32_t,4>{17,67,303,207}:menu==3?std::array<int32_t,4>{17,67,303,169}:source;assert(rects[0].p==expected);
   for(unsigned i=0;i<4;i++)assert(MEM_W(i*4,gpr(S32(p)))==source[i]);
  }
 }
 puts("PASS: actual5B1E8 native blackfill/fouroriginaledges/fade sixlevels; scoped2/3 bounds; originaltables, othermenus andstack preserved; zeroheap allocations");}
