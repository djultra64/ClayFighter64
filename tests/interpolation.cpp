// Native fighter interpolation scope/command fixture by djultra64.
#include "interpolation.h"
#include "graphics_config.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>
int main(){std::vector<uint8_t> ram(9<<20);auto*rdram=ram.data();constexpr uint32_t dl=0x80200000,matrix=0x80300000,actor=0x80400000,sp=0x80700000,sprite=0x80500000;
 MEM_W(0,S32(0x80110898))=S32(dl);MEM_W(0,S32(0x8011089c))=S32(matrix+64);MEM_W(0,S32(0x801028b8))=S32(actor);MEM_W(0,S32(actor+0x1d4))=S32(0x80600000);for(unsigned i=0;i<4;i++)MEM_H(0,S32(matrix+i*10))=1;
 // Actual native pool tables/limits, with separately allocated DL/matrix/vertex pools.
 MEM_W(0,S32(0x801108cc))=2;MEM_W(0,S32(0x800de174))=0;MEM_W(0,S32(0x80110880))=0x2e00;
 MEM_W(0,S32(0x80110888))=0x80;MEM_W(0,S32(0x80110890))=0xfa0;
 MEM_W(0,S32(0x801108dc))=S32(0x80150000);MEM_W(0,S32(0x801108e0))=S32(0x80150100);MEM_W(0,S32(0x801108e4))=S32(0x80150200);
 MEM_W(0,S32(0x80150000))=S32(dl);MEM_W(0,S32(0x80150100))=S32(matrix);MEM_W(0,S32(0x80150200))=S32(0x80600000);MEM_W(0,S32(0x801108a0))=S32(0x80600000);
 auto descriptor=[&](uint32_t p){MEM_BU(0,S32(p))=0x31;MEM_H(0,S32(p+2))=0x210;MEM_H(0,S32(p+6))=15;MEM_H(0,S32(p+8))=15;MEM_H(0,S32(p+14))=16;};descriptor(sprite);descriptor(sprite+1024);
 recomp_context pool{};pool.r4=0x2800;pool.r5=0x80;pool.r6=0xfa0;clay::reserve_actor_interpolation_pool(&pool);assert(pool.r4==0x2e00);clay::reserve_actor_interpolation_pool(&pool);assert(pool.r4==0x2e00);pool.r4=0x1000;clay::reserve_actor_interpolation_pool(&pool);assert(pool.r4==0x1000);
 recomp_context ctx{};ctx.r29=S32(sp);ctx.r4=S32(0x80110898);ctx.r5=S32(sprite);MEM_W(0,S32(sp+0x100))=S32(actor);MEM_W(0,S32(sp+0xfc))=3;
 auto position=[&](float x){uint32_t b;std::memcpy(&b,&x,4);ctx.r6=b;};
 auto initial=clay::GraphicsOptions{};initial.refresh=120;clay::set_graphics_options(initial);clay::reset_actor_interpolation();clay::begin_actor_interpolation_frame(rdram);position(10);
 assert(clay::begin_actor_piece(rdram,&ctx));uint32_t first=MEM_W(0,S32(dl+12));assert(MEM_W(0,S32(dl+16))==0x2001);assert(MEM_W(0,S32(dl+24))==0x01020040);assert(MEM_W(0,S32(dl+28))==S32(matrix&0x1fffffff));assert(!clay::begin_actor_piece(rdram,&ctx));clay::end_actor_piece(rdram);assert(MEM_W(0,S32(dl+32))==0x6400000d);assert(MEM_W(0,S32(dl+40))==0x01020040);
 auto again=[&](){MEM_W(0,S32(0x80110898))=S32(dl);assert(clay::begin_actor_piece(rdram,&ctx));uint32_t id=MEM_W(0,S32(dl+12));clay::end_actor_piece(rdram);return id;};
 position(20);assert(again()==first);position(200);auto cut=again();assert(cut!=first);ctx.r5=S32(sprite+1024);assert(again()!=cut);auto image_id=again();MEM_BU(0,S32(sprite+1024+0x210))=7;assert(again()!=image_id);auto previous=again();clay::reset_actor_interpolation();clay::begin_actor_interpolation_frame(rdram);assert(again()!=previous);
 ctx.r4=S32(0x80110000);assert(!clay::begin_actor_piece(rdram,&ctx));ctx.r4=S32(0x80110898);MEM_W(0,S32(sp+0x100))=S32(actor+64);assert(!clay::begin_actor_piece(rdram,&ctx));MEM_W(0,S32(sp+0x100))=S32(actor);
 auto o=clay::graphics_options();o.refresh=60;clay::set_graphics_options(o);clay::begin_actor_interpolation_frame(rdram);assert(!clay::begin_actor_piece(rdram,&ctx));o.refresh=120;clay::set_graphics_options(o);MEM_H(0,S32(matrix))=2;clay::begin_actor_interpolation_frame(rdram);assert(!clay::begin_actor_piece(rdram,&ctx));
 // Native allocation boundaries reject tags BEFORE any original sprite write.
 MEM_H(0,S32(matrix))=1;clay::begin_actor_interpolation_frame(rdram);MEM_W(0,S32(0x80110898))=S32(dl+0x2e00*8-128);auto saved=MEM_W(0,S32(0x80110898));assert(!clay::begin_actor_piece(rdram,&ctx));assert(MEM_W(0,S32(0x80110898))==saved);
 MEM_W(0,S32(0x80110898))=S32(dl);MEM_W(0,S32(0x801108a0))=S32(0x80600000+0xfa0*16-32);assert(!clay::begin_actor_piece(rdram,&ctx));MEM_W(0,S32(0x801108a0))=S32(0x80600000);
 MEM_W(0,S32(0x80110880))=0x2800;clay::begin_actor_interpolation_frame(rdram);assert(!clay::begin_actor_piece(rdram,&ctx));MEM_W(0,S32(0x80110880))=0x2e00;
 clay::begin_actor_interpolation_frame(rdram);MEM_H(0,S32(sprite+1024+14))=1;assert(!clay::begin_actor_piece(rdram,&ctx));MEM_H(0,S32(sprite+1024+14))=16;
 // Actual hidden fighter descriptor: format43,126x78,stride504,pixeloffset16.
 MEM_BU(0,S32(sprite+1024))=0x43;MEM_H(0,S32(sprite+1024+2))=16;MEM_H(0,S32(sprite+1024+6))=126;MEM_H(0,S32(sprite+1024+8))=78;MEM_H(0,S32(sprite+1024+14))=504;
 clay::begin_actor_interpolation_frame(rdram);assert(clay::begin_actor_piece(rdram,&ctx));clay::end_actor_piece(rdram);
 auto rgba_id=again();MEM_BU(0,S32(sprite+1024+16+504*77+503))=9;assert(again()!=rgba_id);
 descriptor(sprite+1024);
 // Fixed native extra allocation caps totalmetadata across thewholeframe.
 clay::begin_actor_interpolation_frame(rdram);for(unsigned n=0;n<256;n++){MEM_W(0,S32(0x80110898))=S32(dl);assert(clay::begin_actor_piece(rdram,&ctx));clay::end_actor_piece(rdram);}MEM_W(0,S32(0x80110898))=S32(dl);assert(!clay::begin_actor_piece(rdram,&ctx));
 MEM_W(0,S32(0x800de174))=2;clay::begin_actor_interpolation_frame(rdram);assert(!clay::begin_actor_piece(rdram,&ctx));
 puts("PASS: fighter-only command scope, stable IDs, pose/teleport/new-fight cuts, exact matrix restore, original-rate bypass, pool/vertex bounds, full image reuse guard");}
