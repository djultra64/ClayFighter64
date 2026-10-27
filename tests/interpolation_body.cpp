// Original mainbody draw ABI fixture by djultra64; cache/math helpers isolated.
#include "recomp.h"
#include "interpolation.h"
#include "graphics_config.h"
#include <cstdio>
#include <vector>
#include <cassert>
extern "C" void func_800438D8(uint8_t*,recomp_context*);
extern "C" void func_800B0AC4(uint8_t*,recomp_context*){}
extern "C" void func_80041DC8(uint8_t*,recomp_context*){}
extern "C" void func_80002DE0(uint8_t*,recomp_context*c){c->r2=S32(0x80500000);}
static unsigned calls=0;
extern "C" void func_800A87C4(uint8_t*rdram,recomp_context*c){calls++;printf("nativebody a0%x actor%x piece%u sprite%x xbits%x ybits%x\n",unsigned(c->r4),unsigned(MEM_W(0x100,c->r29)),unsigned(MEM_W(0xfc,c->r29)),unsigned(c->r5),unsigned(c->r6),unsigned(c->r7));assert(uint32_t(MEM_W(0,S32(0x80110898)))==0x80200020);assert(uint32_t(MEM_W(0,S32(0x80200008)))==0x6400000c);}
int main(){std::vector<uint8_t>ram(9<<20);auto*rdram=ram.data();constexpr uint32_t actor=0x80400000;
 MEM_W(0,S32(0x801028b8))=S32(actor);MEM_W(0,S32(actor+0x1054))=1;MEM_W(0,S32(actor+0x1d4))=S32(0x80600000);
 MEM_W(0,S32(0x801028c0))=S32(0x80120000);MEM_W(0,S32(0x801028c8))=S32(0x80130000);MEM_W(0,S32(0x80130044))=0x3f800000;MEM_W(0,S32(0x80601d90))=0x3f800000;MEM_W(0,S32(0x80601d94))=0x3f800000;MEM_W(0,S32(0x80601d98))=0x3f800000;for(unsigned p=0x4b0;p<=0x4cc;p+=4)MEM_W(0,S32(actor+p))=0x3f800000;
 constexpr uint32_t matrix=0x80300000,dl=0x80200000,sprite=0x80500000;
 MEM_W(0,S32(0x80110898))=S32(dl);MEM_W(0,S32(0x8011089c))=S32(matrix+64);for(unsigned i=0;i<4;i++)MEM_H(0,S32(matrix+i*10))=1;
 MEM_W(0,S32(0x801108cc))=2;MEM_W(0,S32(0x800de174))=0;MEM_W(0,S32(0x80110880))=0x2e00;MEM_W(0,S32(0x80110888))=0x80;MEM_W(0,S32(0x80110890))=0xfa0;
 MEM_W(0,S32(0x801108dc))=S32(0x80150000);MEM_W(0,S32(0x801108e0))=S32(0x80150100);MEM_W(0,S32(0x801108e4))=S32(0x80150200);MEM_W(0,S32(0x80150000))=S32(dl);MEM_W(0,S32(0x80150100))=S32(matrix);MEM_W(0,S32(0x80150200))=S32(0x80600000);MEM_W(0,S32(0x801108a0))=S32(0x80600000);
 MEM_BU(0,S32(sprite))=0x31;MEM_H(0,S32(sprite+2))=0x210;MEM_H(0,S32(sprite+6))=15;MEM_H(0,S32(sprite+8))=15;MEM_H(0,S32(sprite+14))=16;
 auto graphics=clay::GraphicsOptions{};graphics.refresh=120;clay::set_graphics_options(graphics);clay::reset_actor_interpolation();clay::begin_actor_interpolation_frame(rdram);
recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=S32(0x80770000);c.r4=S32(actor);c.r5=S32(0x80120000);func_800438D8(rdram,&c);assert(calls==1);assert(uint32_t(MEM_W(0,S32(0x80110898)))==dl+48);puts("PASS: original438D8 actor/index/sprite/stack ABI reaches scoped matrixgroup hook");}
