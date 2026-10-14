// Native main menu extension by djultra64. Original font/animation unchanged.
#include "main_menu.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
namespace clay::main_menu {
namespace {
std::atomic<bool> requested{false};
constexpr uint32_t placements=0x80840000, labels=0x80840040, auxiliary=0x80840050, text=0x80840060;
uint32_t word(uint8_t* rdram,uint32_t address){return uint32_t(MEM_W(0,gpr(S32(address))));}
uint32_t state(uint8_t* rdram){auto p=word(rdram,0x800ddd3c);return (p&0xff800000u)==0x80000000u&&((p&0x7fffffu)+0x1a80<=0x800000)?p:0;}
void copy(uint8_t* rdram,uint32_t dst,uint32_t src,unsigned count){for(unsigned i=0;i<count;i++)MEM_BU(0,gpr(S32(dst+i)))=MEM_BU(0,gpr(S32(src+i)));}
}
void prepare(uint8_t* rdram){
 // Menu0 is an inclusive range: blank/title0, GameStart1, Options2.
 // Other menus continue using their unchanged original overlapping tables.
 MEM_H(0,gpr(S32(0x800dadee)))=3;
 copy(rdram,placements,0x800dae04,3*14);
 copy(rdram,placements+3*14,0x800dae04+2*14,14);
 // Reflow upwards by one existing native row step; the new last item
 // occupies the old Options row, keeping all three inside the viewport.
 for(unsigned row=1;row<=2;row++){
  MEM_H(0,gpr(S32(placements+row*14+2)))=MEM_H(0,gpr(S32(placements+row*14+2)))+50;
  MEM_H(0,gpr(S32(placements+row*14+8)))=MEM_H(0,gpr(S32(placements+row*14+8)))+42;
 }
 copy(rdram,labels,0x800db22c,3*4);
 MEM_W(0,gpr(S32(labels+12)))=S32(text);
 copy(rdram,auxiliary,0x800db158,3*4);
 MEM_W(0,gpr(S32(auxiliary+12)))=0;
 constexpr char caption[]="Exit game";
 for(unsigned i=0;i<sizeof(caption);i++)MEM_BU(0,gpr(S32(text+i)))=uint8_t(caption[i]);
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
 constexpr uint32_t bounds[]={105,144,216,219};
 for(unsigned i=0;i<4;i++)MEM_W(0,gpr(S32(rectangle+i*4)))=bounds[i];
 const gpr sp=ctx->r29;ctx->r4=gpr(S32(rectangle));ctx->r29=ADD32(sp,-0x100);
 original(rdram,ctx);ctx->r29=sp;
}
bool confirm_exit(uint8_t* rdram){
 auto p=state(rdram);
 if(!p||word(rdram,p+12)!=0||word(rdram,p+16)!=0||word(rdram,p+24)!=3||word(rdram,p)!=0||word(rdram,p+0x1a64)!=0||(word(rdram,0x80118d74)&0x9000)==0)return false;
 requested.store(true,std::memory_order_release);
 if(std::getenv("CLAY_TRACE_GAME"))fprintf(stderr,"{\"event\":\"main_menu_exit_requested\"}\n");
 return true;
}
bool exit_requested(){return requested.load(std::memory_order_acquire);}
}
