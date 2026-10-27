// Native cursor sprite submission regression. djultra64.
#include "selector_layout.h"
#include <cassert>
#include <bit>
#include <cstdio>
#include <vector>
extern "C" void func_80071738(uint8_t*,recomp_context*){}
extern "C" void native_cursor_draw(uint8_t*,recomp_context*);
namespace {bool training_active=false;struct Draw{uint32_t sprite;float x,y;};std::vector<Draw> draws;constexpr uint32_t state=0x80200000,bank=0x80300000,stack=0x807fe000;}
namespace clay::training {bool selecting_fighters(){return training_active;}}
void clay_sc_draw_preview(uint8_t*r,recomp_context*c,bool,bool,recomp_func_t*f){f(r,c);}
extern "C" uint64_t clay_sc_roster_coordinates(uint8_t*){return S32(state+0xd8);}
extern "C" void func_800A6520(uint8_t*,recomp_context*c){draws.push_back({uint32_t(c->r5),std::bit_cast<float>(uint32_t(c->r6)),std::bit_cast<float>(uint32_t(c->r7))});}
extern "C" void func_800A87C4(uint8_t*rdram,recomp_context*c){assert(MEM_BU(0,S32(0x80118fee))!=0);func_800A6520(rdram,c);}
int main(){
 std::vector<uint8_t>memory(8<<20);auto*rdram=memory.data();
 MEM_W(0,S32(0x800ddd30))=S32(state);MEM_W(0,S32(state+0x20))=S32(bank);
 MEM_W(0,S32(bank+0x344))=0x800;MEM_W(0,S32(bank+0x34c))=0x900;MEM_W(0,S32(bank+0x354))=0xa00;MEM_W(0,S32(bank+0x35c))=0xa00;
 MEM_B(0,S32(state+0x88))=240;MEM_B(0,S32(state+0x89))=200;
 for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col){MEM_H(0,S32(state+0xd8+(row*4+col)*4))=-146+int(col)*76;MEM_H(0,S32(state+0xda+(row*4+col)*4))=70-int(row)*77;}
 for(unsigned p1=0;p1<16;++p1)for(unsigned p2=0;p2<16;++p2)for(bool confirmed:{false,true}){
  recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=S32(stack);MEM_W(0x38,c.r29)=p1*2;MEM_W(0x3c,c.r29)=p2*2;
  MEM_W(0,S32(state+0x6c))=confirmed?128:0;MEM_W(0,S32(state+0x70))=0;
  MEM_H(0,S32(state+0x9e))=0;MEM_H(0,S32(state+0xa0))=0;draws.clear();training_active=false;
  clay::selector::draw_fighters(rdram,&c,native_cursor_draw);
  if(!confirmed)assert(draws.empty()); // Reproduces the original missing cursor.
  draws.clear();training_active=true;clay::selector::draw_fighters(rdram,&c,native_cursor_draw);
  assert(draws.size()==2);assert(MEM_H(0,S32(state+0x9e))==0);
  assert(MEM_W(0,S32(state+0x6c))==(confirmed?128:0)&&MEM_W(0,S32(state+0x70))==0);
  unsigned matched=0;
  for(const auto&draw:draws){assert(draw.sprite>=bank+0x800&&draw.sprite<=bank+0xa00);for(unsigned slot:{p1,p2})if(draw.x==float(MEM_H(0,S32(state+0xd8+slot*4))-11)&&draw.y==float(MEM_H(0,S32(state+0xda+slot*4))+9)){++matched;break;}}
  assert(matched==2);
 }
 puts("PASS 512 native cursor draw cases: P1/opponent, coincident/separate slots, four columns, confirmation and scoped mode restoration");
}
