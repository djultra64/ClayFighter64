// Actual original main-menu navigation fixture by djultra64.
#include "recomp.h"
#include "main_menu.h"
#include "test_input.h"
#include <vector>
#include <fstream>
#include <iterator>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <stdexcept>
bool main_menu_baseline=false;
static unsigned border_calls=0;
static void check_border(uint8_t*rdram,recomp_context*ctx){
 ++border_calls;const unsigned expected[]={96,110,224,224};
 for(unsigned i=0;i<4;i++)assert(MEM_W(i*4,ctx->r4)==expected[i]);
}
extern "C" {
void func_80067980(uint8_t*,recomp_context*);
void main_menu_idle_timeout(uint8_t*,recomp_context*);
void func_800A0CA8(uint8_t*,recomp_context*){}
void func_8009B6B4(uint8_t*,recomp_context*){}
void func_8009C3D0(uint8_t*,recomp_context*){}
void func_8009C548(uint8_t*,recomp_context*){}
void func_800640A0(uint8_t*,recomp_context*){}
void switch_error(const char*,uint32_t,uint32_t){throw std::runtime_error("unexpected original menu switch");}
}
int main(int argc,char**argv){
 assert(argc>=2);main_menu_baseline=argc>2&&std::string(argv[2])=="baseline";
 std::ifstream f(argv[1],std::ios::binary);std::vector<uint8_t>rom((std::istreambuf_iterator<char>(f)),{}),ram(0x900000);auto*rdram=ram.data();assert(rom.size()>0xf9980);
 for(unsigned i=0x1000;i<0xf9980;i++)rdram[((i-0x1000)+0x400)^3]=rom[i];
 auto w=[&](uint32_t p,uint32_t v){MEM_W(0,gpr(S32(p)))=v;};auto read=[&](uint32_t p){return uint32_t(MEM_W(0,gpr(S32(p))));};
 constexpr uint32_t state=0x80200000,game=0x80210000;
 w(0x800ddd3c,state);w(0x800ddd40,0x80230000);w(0x800ddd34,game);w(game+0x6914,0x80220000);
 clay::main_menu::prepare(rdram);if(main_menu_baseline)MEM_H(0,S32(0x800dadee))=2;
 recomp_context border{};border.r29=gpr(S32(0x807f0000));border.r4=gpr(S32(0x800dad80));clay::main_menu::draw_border(rdram,&border,&check_border);assert(border_calls==1&&uint32_t(border.r29)==0x807f0000);
 // Exact caption, original two labels and native placement stride.
 assert(read(0x800db230)==0x800f180c&&read(0x800db234)==0x800f1818);
 auto labels=clay::main_menu::table(rdram,0x800db22c);std::string caption;auto t=read(labels+16);for(unsigned i=0;MEM_BU(0,gpr(S32(t+i)));i++)caption+=char(MEM_BU(0,gpr(S32(t+i))));assert(caption=="Exit game");
 auto placement=clay::main_menu::table(rdram,0x800dae04);assert(MEM_H(0,gpr(S32(placement+44)))==MEM_H(0,gpr(S32(placement+30)))-50);
 assert(clay::testing::current_input_phase()==clay::testing::InputPhase::Startup); // Render-table lookup must not arm scripted input.
 auto native_tick=[&](){recomp_context ctx{};ctx.r29=S32(0x807f0000);ctx.f_odd=&ctx.f0.u32h;func_80067980(rdram,&ctx);};
 auto tick=[&](unsigned selected,unsigned buttons){
  for(unsigned a=0x80118cb0;a<=0x80118d88;a+=4)w(a,0);
  for(unsigned a=state;a<state+0x24;a+=4)w(a,0);
  w(state+24,selected);w(state+28,selected);
  w(0x80118cb0,buttons);w(0x80118cb4,buttons);w(0x80118d74,buttons);
  native_tick();
 };
 // The actual native tick must not consume a held Start/confirm that brought us
 // here. In particular confirm_exit cannot release a gate after observe_input
 // has just cleared its RAM words, and a duplicate observer must be rejected.
 clay::testing::publish_input_phase(clay::testing::InputPhase::Startup);
 clay::main_menu::prepare(rdram);if(main_menu_baseline)MEM_H(0,S32(0x800dadee))=2;
 for(unsigned offset:{0u,12u,16u,0x1a64u}){
  w(state+offset,1);clay::main_menu::observe_input(rdram);
  assert(clay::testing::current_input_phase()==clay::testing::InputPhase::Startup&&!clay::main_menu::confirm_exit(rdram));
  w(state+offset,0);
 }
 for(unsigned repeat=0;repeat<2;repeat++){
  tick(4,0x8000);
  assert(!clay::main_menu::exit_requested()&&read(state+16)==0);
  assert(clay::testing::current_input_phase()==clay::testing::InputPhase::Startup);
 }
 tick(4,0);assert(clay::testing::current_input_phase()==clay::testing::InputPhase::MainMenu);
 clay::main_menu::prepare(rdram);if(main_menu_baseline)MEM_H(0,S32(0x800dadee))=2;
 assert(clay::testing::current_input_phase()==clay::testing::InputPhase::Loading);
 constexpr uint32_t input_words[]={0x80118cb0,0x80118cb4,0x80118cc0,0x80118cc4,0x80118cc8,0x80118ccc,0x80118cd8,0x80118cdc,0x80118d74,0x80118d84};
 for(unsigned repeat=0;repeat<2;repeat++){
  for(auto p:input_words)w(p,0);
  // Native P2 held/edges and aggregated navigation must also be discarded.
  for(auto p:{0x80118cc8u,0x80118cccu,0x80118cd8u,0x80118cdcu,0x80118d84u})w(p,0x8000);
  w(state+24,4);w(0x80118d74,0x8000);native_tick();
  for(auto p:input_words)assert(read(p)==0);
  assert(!clay::main_menu::exit_requested()&&read(state+16)==0);
  assert(clay::testing::current_input_phase()==clay::testing::InputPhase::Loading);
 }
 native_tick();assert(clay::testing::current_input_phase()==clay::testing::InputPhase::MainMenu);
 tick(2,0x400);if(read(state+24)!=3){fprintf(stderr,"RED: original navigation wraps before Exit game\n");return 14;}
 tick(3,0x400);assert(read(state+24)==4);
 tick(4,0x400);assert(read(state+24)==1);
 tick(1,0x800);assert(read(state+24)==4);
 tick(1,0x8000);assert(read(state+16)==3&&!clay::main_menu::exit_requested());
 tick(3,0x8000);assert(read(state+16)==1&&!clay::main_menu::exit_requested());
 tick(2,0x8000);assert(read(state+16)==3&&!clay::main_menu::exit_requested());
 tick(4,0x4000);assert(!clay::main_menu::exit_requested());
 tick(4,0);assert(!clay::main_menu::exit_requested());
 w(state+4,500);recomp_context idle{};idle.f_odd=&idle.f0.u32h;main_menu_idle_timeout(rdram,&idle);
 assert(read(state+16)==3&&read(state)==3&&!clay::main_menu::exit_requested());
 tick(4,0);
 w(state+0x1a64,1);w(0x80118d74,0x8000);assert(!clay::main_menu::confirm_exit(rdram));w(state+0x1a64,0);
 w(state+12,1);assert(clay::main_menu::table(rdram,0x800db22c)==0x800db22c);assert(!clay::main_menu::confirm_exit(rdram));
 tick(4,argc>2&&std::string(argv[2])=="start"?0x1000:0x8000);assert(clay::main_menu::exit_requested());assert(read(state+16)==0);
 fprintf(stderr,"GREEN: single native observer, physical P1/P2 release gating, native fourth-row navigation/wrap, original actions, Back/idle/transition guards and exact Exit game caption\n");
}
