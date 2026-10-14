// Private ROM fixture for authentic native biography timing/font rendering; djultra64.
#include "sc_bios.h"
#include <cassert>
#include <array>
#include <fstream>
#include <iterator>
#include <vector>
#include <string>
#include <cstdio>
#include <cstring>
extern "C" {void func_8006CC38(uint8_t*,recomp_context*);void func_8006C6C0(uint8_t*,recomp_context*);void func_80071738(uint8_t*,recomp_context*);}
static bool enabled=true;namespace clay_roster {bool enabled(){return ::enabled;}}
static unsigned glyphs=0;
extern "C" void func_800B3C70(uint8_t*,recomp_context*c){assert(c->r5==0x24);c->r2=S32(0x80260000);}
extern "C" void func_800B4408(uint8_t*,recomp_context*){}
extern "C" void func_8005B1E8(uint8_t*,recomp_context*){}
extern "C" void func_8005B528(uint8_t*,recomp_context*){}
extern "C" void func_800A561C(uint8_t*,recomp_context*){}
extern "C" void func_800A6520(uint8_t*,recomp_context*){glyphs++;}
extern "C" void func_800AA9EC(uint8_t*,recomp_context*){glyphs++;}
static std::vector<uint8_t> read(const char*p){std::ifstream f(p,std::ios::binary);assert(f);return{std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char**argv){assert(argc==4);auto rom=read(argv[1]),fontbin=read(argv[2]);std::vector<uint8_t> ram(9<<20);auto*rdram=ram.data();for(size_t i=0xC00;i<0xD3520;i++)rdram[(i-0xC00)^3]=rom[i];
 constexpr int32_t font=S32(0x80230000),map=font+0x100,bundle=S32(0x80200000),config=S32(0x80250000),game=S32(0x80240000),state=S32(0x80260000);
 for(size_t i=0;i<fontbin.size();i++)MEM_B(i,bundle+12)=fontbin[i];auto u32=[&](size_t o){return(uint32_t(fontbin[o])<<24)|(uint32_t(fontbin[o+1])<<16)|(uint32_t(fontbin[o+2])<<8)|fontbin[o+3];};size_t chars=u32(0)-12;int sum=0,count=0;
 for(int i=1;fontbin[chars+i-1]!='\\';i++){unsigned ch=fontbin[chars+i-1];uint32_t g=uint32_t(bundle)+u32(i*8);MEM_W(int32_t((ch-33)*4),map)=S32(g);sum+=MEM_H(gpr(S32(g)),6);count++;}
 MEM_W(4,font)=map;MEM_H(0x26,font)=2;MEM_H(0x1c,font)=sum/count;MEM_H(0x1e,font)=15;
 MEM_W(0,gpr(S32(0x800DDD40)))=config;MEM_W(0,gpr(S32(0x800DDD34)))=game;MEM_W(0x6914,game)=font;MEM_W(0,gpr(S32(0x8010CFD0)))=320;MEM_W(0,gpr(S32(0x8010CFD4)))=240;
 recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=S32(0x807FF000);std::string error;auto before=ram;assert(!clay::bios::load("/missing/private/bio",error));clay::bios::install(rdram,&c);assert(before==ram);assert(clay::bios::load(argv[3],error));enabled=false;clay::bios::install(rdram,&c);assert(before==ram);enabled=true;c.r4=1;clay::bios::install(rdram,&c);assert(before==ram);c.r4=0;clay::bios::install(rdram,&c);
 for(int id=0;id<17;id++)for(int field=0;field<20;field++){if((id==6||id==11||id==12||id==16)&&(field<4||(field>=8&&field<12)))continue;size_t offset=(0xDDBDC+id*20+field)^3;assert(ram[offset]==before[offset]);}
 auto installed=ram;clay::bios::install(rdram,&c);assert(installed==ram);
 // Compare donor tick state with the original Frosty biography for the same1200ticks.
 std::vector<std::array<int32_t,6>> original_ticks;
 MEM_W(0x14,config)=0;MEM_W(0xD0,config)=0;MEM_W(0,gpr(S32(0x800DDD44)))=0;c.r4=0;func_8006CC38(rdram,&c);
 for(int frame=0;frame<1200;frame++){c.r4=1;func_8006CC38(rdram,&c);original_ticks.push_back({MEM_W(state,0),MEM_W(state,4),MEM_W(state,8),MEM_W(state,12),MEM_W(state,20),MEM_W(state,24)});}
 // Actual native initializer/scroller/font draw, not a host recreation of its animation.
 for(int id:{6,11,12,16}){
  MEM_W(0x14,config)=id;MEM_W(0xD0,config)=0;MEM_W(0,gpr(S32(0x800DDD44)))=0;c.r4=0;func_8006CC38(rdram,&c);
  uint32_t row=0x800DDBDC+id*20,ptr=MEM_W(gpr(S32(row)),0),lines=MEM_W(gpr(S32(row)),8);assert(uint32_t(MEM_W(state,0x20))==ptr);assert(uint32_t(MEM_W(state,0x1c))==lines);assert(MEM_W(state,0x14)==-7);
  uint32_t pos=ptr;
  for(unsigned line=0;line<lines;line++){c.r4=font;c.r5=gpr(S32(pos));func_80071738(rdram,&c);assert(c.r2<=252);while(MEM_BU(gpr(S32(pos)),0))pos++;pos++;}assert(pos<=0x80834000);
  glyphs=0;for(int frame=0;frame<1200;frame++){c.r4=1;func_8006CC38(rdram,&c);assert((std::array<int32_t,6>{MEM_W(state,0),MEM_W(state,4),MEM_W(state,8),MEM_W(state,12),MEM_W(state,20),MEM_W(state,24)})==original_ticks[frame]);func_8006C6C0(rdram,&c);}assert(glyphs>0);assert(MEM_W(state,0)==1200);printf("PASS authentic bio ID%d: %u lines, %u native animated glyph draws\n",id,lines,glyphs);
 }
 // Foreign nonzero entry: atomic reject, no partial table or scratch writes.
 ram=before;rdram=ram.data();MEM_W(0,gpr(S32(0x800DDBDC+11*20)))=0x12345678;auto foreign=ram;c.r4=0;clay::bios::install(rdram,&c);assert(ram==foreign);
 puts("PASS missing/base/noninit/idempotent/foreign guards; base rows and endings byte exact");
}
