// Native main menu extension by djultra64. Original font/animation unchanged.
#include "main_menu.h"
#include "test_input.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
namespace clay::main_menu {
namespace {
std::atomic<bool> requested{false};
std::atomic<bool> training{false},reentry{false};
thread_local unsigned restore_row=0;
std::atomic<bool> input_release{true};
constexpr uint32_t placements=0x80840000, labels=0x80840080, auxiliary=0x808400a0, text=0x808400c0;
uint32_t word(uint8_t* rdram,uint32_t address){return uint32_t(MEM_W(0,gpr(S32(address))));}
uint32_t state(uint8_t* rdram){auto p=word(rdram,0x800ddd3c);return (p&0xff800000u)==0x80000000u&&((p&0x7fffffu)+0x1a80<=0x800000)?p:0;}
void copy(uint8_t* rdram,uint32_t dst,uint32_t src,unsigned count){for(unsigned i=0;i<count;i++)MEM_BU(0,gpr(S32(dst+i)))=MEM_BU(0,gpr(S32(src+i)));}
}
void prepare(uint8_t* rdram){
 // Menu0 is an inclusive range: blank/title0, GameStart1, Options2.
 // Other menus continue using their unchanged original overlapping tables.
 training.store(reentry.exchange(false,std::memory_order_acq_rel),std::memory_order_release);restore_row=0;input_release=true;
 if(clay::testing::current_input_phase()!=clay::testing::InputPhase::Startup)clay::testing::publish_input_phase(clay::testing::InputPhase::Loading);
 MEM_H(0,gpr(S32(0x800dadee)))=4;
 copy(rdram,placements,0x800dae04,14);
 for(unsigned row=1;row<=4;row++){
  copy(rdram,placements+row*14,0x800dae04+(row==1?1:2)*14,14);
  const int steps=row==1?2:4-int(row);
  MEM_H(0,gpr(S32(placements+row*14+2)))+=steps*50;
  MEM_H(0,gpr(S32(placements+row*14+8)))+=steps*42;
 }
 for(unsigned row=0;row<=4;row++){
  unsigned original=row==0?0:row==1?1:2;
  MEM_W(0,gpr(S32(labels+row*4)))=word(rdram,0x800db22c+original*4);
  MEM_W(0,gpr(S32(auxiliary+row*4)))=word(rdram,0x800db158+original*4);
 }
 MEM_W(0,gpr(S32(labels+8)))=S32(text);
 MEM_W(0,gpr(S32(labels+16)))=S32(text+32);
 MEM_W(0,gpr(S32(auxiliary+8)))=0;MEM_W(0,gpr(S32(auxiliary+16)))=0;
 constexpr char caption[]="Training Mode",exit_caption[]="Exit game";
 for(unsigned i=0;i<sizeof(caption);i++)MEM_BU(0,gpr(S32(text+i)))=uint8_t(caption[i]);
 for(unsigned i=0;i<sizeof(exit_caption);i++)MEM_BU(0,gpr(S32(text+32+i)))=uint8_t(exit_caption[i]);
}
uint32_t table(uint8_t* rdram,uint32_t original){
 auto p=state(rdram);if(!p||word(rdram,p+12)!=0)return original;
 switch(original){case 0x800dae04:return placements;case 0x800db22c:return labels;case 0x800db158:return auxiliary;default:return original;}
}
void draw_border(uint8_t* rdram,recomp_context* ctx,recomp_func_t* original){
 auto p=state(rdram);
 if(!p||word(rdram,p+12)!=0||uint32_t(ctx->r4)!=0x800dad80){original(rdram,ctx);return;}
 // Original rectangle's fill, alpha, edge renderer and animation survive.
 const uint32_t rectangle=uint32_t(ctx->r29)-0x80;
 constexpr uint32_t bounds[]={96,110,224,224};
 for(unsigned i=0;i<4;i++)MEM_W(0,gpr(S32(rectangle+i*4)))=bounds[i];
 const gpr sp=ctx->r29;ctx->r4=gpr(S32(rectangle));ctx->r29=ADD32(sp,-0x100);
 original(rdram,ctx);ctx->r29=sp;
}
void observe_input(uint8_t* rdram){
 auto p=state(rdram);if(!p||word(rdram,p+12)!=0||word(rdram,p+16)!=0||word(rdram,p)!=0||word(rdram,p+0x1a64)!=0)return;
 if(input_release.load(std::memory_order_acquire)){
  if(word(rdram,0x80118cb0)||word(rdram,0x80118cc0)||word(rdram,0x80118cc8)||word(rdram,0x80118cd8)){
   for(uint32_t address:{0x80118cb0u,0x80118cb4u,0x80118cc0u,0x80118cc4u,0x80118cc8u,0x80118cccu,0x80118cd8u,0x80118cdcu,0x80118d74u,0x80118d84u})MEM_W(0,gpr(S32(address)))=0;
   return;
  }
  input_release=false;
 }
 clay::testing::publish_input_phase(clay::testing::InputPhase::MainMenu);
}
bool confirm_exit(uint8_t* rdram){
 auto p=state(rdram);
 if(!p||word(rdram,p+12)!=0||word(rdram,p+16)!=0||word(rdram,p)!=0||word(rdram,p+0x1a64)!=0)return false;
 if(input_release.load(std::memory_order_acquire))return false;
 if((word(rdram,0x80118d74)&0x9000)==0)return false;
 const unsigned selected=word(rdram,p+24);
 if(selected==1){training=false;return false;}
 if(selected==2||selected==3){
  training.store(selected==2,std::memory_order_release);
  restore_row=selected;
  MEM_W(0,gpr(S32(p+24)))=selected==2?1:2;
  if(selected==2&&std::getenv("CLAY_TRACE_GAME"))fprintf(stderr,"{\"event\":\"training_menu_confirmed\"}\n");
  return false;
 }
 if(selected!=4)return false;
 requested.store(true,std::memory_order_release);
 if(std::getenv("CLAY_TRACE_GAME"))fprintf(stderr,"{\"event\":\"main_menu_exit_requested\"}\n");
 return true;
}
bool exit_requested(){return requested.load(std::memory_order_acquire);}
void restore_selection(uint8_t* rdram){
 if(!restore_row)return;
 if(auto p=state(rdram)){
  MEM_W(0,gpr(S32(p+24)))=restore_row;
  MEM_W(0,gpr(S32(p+0x1a68)))=restore_row;
 }
 restore_row=0;
}
void retain_training_request(){reentry=true;}
bool training_requested(){return training.load(std::memory_order_acquire);}
void clear_training_request(){training.store(false,std::memory_order_release);}
}
