// Original matrix and sprite layout regression by djultra64.
#include "selector_layout.h"
#include "graphics_config.h"
#include <cassert>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>
extern "C" void func_800B09DC(uint8_t*,recomp_context*);
extern "C" void func_800B0270(uint8_t*,recomp_context*);
extern "C" void selector_panel_slot(uint8_t*,recomp_context*);
extern "C" void func_800A6520(uint8_t*,recomp_context*);
extern "C" void do_break(uint32_t){std::abort();}
extern "C" void switch_error(const char*,uint32_t,uint32_t){std::abort();}
void clay_sc_draw_preview(uint8_t*r,recomp_context*c,bool,bool,recomp_func_t*f){f(r,c);}
namespace clay::training {bool selecting_fighters(){return false;}}
static float received_x;
static void actor(uint8_t*,recomp_context*c){received_x=float(int32_t(c->r6));}
static void sprite(uint8_t*,recomp_context*c){received_x=std::bit_cast<float>(int32_t(c->r6));}
static int32_t bits(float x){return std::bit_cast<int32_t>(x);}
int main(int argc,char**argv){
 fprintf(stderr,"selector_stage enter-main argc%d\n",argc);fflush(stderr);
 assert(argc==2||argc==3);const bool repair=argc==2;unsigned miscentered=0;std::ifstream file(argv[1],std::ios::binary);std::vector<uint8_t>rom{std::istreambuf_iterator<char>(file),{}};assert(rom.size()==0xc00000);
 fprintf(stderr,"selector_stage ROMbytes%zu\n",rom.size());fflush(stderr);
 std::vector<uint8_t>ram(8<<20);auto*rdram=ram.data();for(unsigned i=0x1000;i<0xf9980;i++)rdram[((i-0x1000)+0x400)^3]=rom[i];
 constexpr uint32_t sp=0x807fe000,matrix=0x80160000,image=0x80200000,basis=0x80170000,dl=0x80300000,verts=0x80500000;
 unsigned cases=0;
 setvbuf(stdout,nullptr,_IONBF,0);printf("selector_fixture boot ROM loaded\n");
 for(bool wide:{false,true})for(unsigned width:{1280u,1920u,2560u}){
  clay::GraphicsOptions o;o.width=width;o.height=720;o.widescreen=wide;clay::set_graphics_options(o);
  const double expansion=wide?o.aspect()/(4.0/3.0)-1:0;
  printf("selector_fixture aspect width%u wide%d matrices\n",width,wide);
  for(bool right:{false,true})for(int phase:{0,13,53,100,600}){
   recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=S32(sp);c.r4=S32(matrix);func_800B0270(rdram,&c);
   c.r4=S32(matrix);const float original=(right?92.f:-92.f)+(right?float(phase):-float(phase));c.r5=S32(bits(original));c.r6=S32(bits(-140));c.r7=S32(bits(-12));
   clay::selector::translate_mechanism(rdram,&c,right,func_800B09DC);
   const float result=std::bit_cast<float>(MEM_W(48,S32(matrix)));
   assert(std::abs(result-(original+(right?1:-1)*346*expansion))<.001);
   assert(std::bit_cast<float>(MEM_W(52,S32(matrix)))==-140);assert(std::bit_cast<float>(MEM_W(56,S32(matrix)))==-12);
   for(unsigned i=0;i<12;i++)assert(MEM_W(int(i*4),S32(matrix))==bits(i%5==0?1.f:0.f));
   c.r6=S32(right?234:-234);clay::selector::draw_preview(rdram,&c,right,false,actor);assert(received_x==(right?234:-234)+std::lround((right?1:-1)*160*expansion));
   c.r6=S32(bits(right?220:-220));clay::selector::draw_preview(rdram,&c,right,true,sprite);assert(std::abs(received_x-((right?220:-220)+(right?1:-1)*160*expansion))<.001);cases++;
  }
  printf("selector_fixture panels\n");
  for(unsigned slot=0;slot<4;slot++)for(int phase:{0,13,53,70,100}){
   recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=S32(sp);c.r4=S32(sp+0xa8);func_800B0270(rdram,&c);
   constexpr uint32_t state=0x80180000;MEM_W(0,S32(0x800ddd30))=S32(state);MEM_W(0x54,S32(state))=bits(float(phase));MEM_W(0x58,S32(state))=bits(float(phase));
   MEM_W(0x40,c.r29)=slot;MEM_W(0x44,c.r29)=S32(0x800dac70+slot*16);selector_panel_slot(rdram,&c);
   const float base=std::bit_cast<float>(MEM_W(0,S32(0x800dac70+slot*16)));const float direction=slot%2?-1.f:1.f;
   const float result=std::bit_cast<float>(MEM_W(0xd8,S32(sp)));assert(std::abs(result-(base+direction*phase+(slot>=2?1:-1)*346*expansion+(slot%2?1:-1)*186*expansion*std::max(0.0,1-double(phase)/70)))<.001);cases++;
  }
  printf("selector_fixture bitmaps\n");
  for(bool right:{false,true})for(unsigned image_width:{64u,128u,160u})for(int hotspot:{-8,0,7}){
   MEM_BU(0,S32(image))=0x11;MEM_BU(1,S32(image))=0xff;MEM_H(2,S32(image))=528;MEM_H(6,S32(image))=image_width;MEM_H(8,S32(image))=32;MEM_H(10,S32(image))=hotspot;MEM_H(14,S32(image))=image_width;
   for(unsigned i=0;i<28;i++)MEM_BU(int(i),S32(basis))=0;MEM_W(4,S32(basis))=bits(1.4f);MEM_W(16,S32(basis))=bits(-1.5f);
   MEM_W(0,S32(0x80110898))=S32(dl);MEM_W(0,S32(0x801108a0))=S32(verts);MEM_W(0,S32(0x80118fc8))=0;
   recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=S32(sp);c.r4=S32(0x80110898);c.r5=S32(image);c.r6=S32(bits(right?102:-295));c.r7=S32(bits(223));
   MEM_W(16,c.r29)=bits(90);MEM_W(20,c.r29)=S32(basis);MEM_W(24,c.r29)=S32(basis+16);MEM_W(28,c.r29)=4;
   if(repair)clay::selector::draw_name(rdram,&c,right,func_800A6520);else func_800A6520(rdram,&c);
   const unsigned count=(uint32_t(MEM_W(0,S32(0x801108a0)))-verts)/16;assert(count>0&&count<256);
   int low=32767,high=-32768;for(unsigned i=0;i<count;i++){int x=MEM_H(0,S32(verts+i*16));low=std::min(low,x);high=std::max(high,x);}
   const float center=(low+high)*.5f;
   if(wide){const float outer=303+std::lround(143*expansion*346/186);const float error=std::abs(center-(right?1:-1)*(outer+104)*.5f);if(error>1.1f)miscentered++;if(repair)assert(error<=1.1f);}
   else {const float expected=(right?102:-295)+float(image_width)*.5f*1.5f;if(std::abs(center-expected)>1.1f){fprintf(stderr,"wide%d right%d width%u hotspot%d native[%d,%d] center%f expected%f\n",wide,right,image_width,hotspot,low,high,center,expected);return 2;}}
   cases++;
  }
 }
 if(!repair){printf("RED: %u native wide bitmap cases retain misplaced original anchors\n",miscentered);return miscentered?1:0;}
 printf("PASS: %u actual B09DC matrix/A6520 bitmap cases; native slide/Y/Z/basis retained, 4:3 unchanged, wide names centered\n",cases);
}
