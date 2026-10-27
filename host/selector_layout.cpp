// Original selector geometry adaptation by djultra64.
#include "selector_layout.h"
#include "graphics_config.h"
#include "sc_preview.h"
#include "training_guest.h"
#include <bit>
#include <cmath>
extern "C" void func_80071738(uint8_t*,recomp_context*);
namespace clay::selector {
namespace {
double expansion(){const auto o=graphics_options();const double s=o.widescreen?o.aspect()/(4.0/3.0):1.0;return s>=1&&s<=8.0/3.0?s-1:0;}
float value(gpr v){return std::bit_cast<float>(int32_t(v));}
gpr bits(float v){return S32(std::bit_cast<int32_t>(v));}
bool valid(uint32_t p,unsigned n){return p>=0x80000000u&&uint64_t(p)+n<=0x80800000ull;}
// The existing native background extension anchors its interior at160 and its
// outer wall at346. Gears/panels retain their original rotations and sliding X.
float mechanism_offset(){return float(346*expansion());}
}
void draw_fighters(uint8_t*rdram,recomp_context*c,recomp_func_t*f){
 const uint32_t state=MEM_W(0,S32(0x800ddd30));
 if(!clay::training::selecting_fighters()||!valid(state,0xb8)){f(rdram,c);return;}
 // The versus selector's mode zero accepts input, but its draw routine hides
 // unconfirmed cursor sprites unless native mode 9E is two. Use that native
 // draw branch without changing input, confirmation, or animation state.
 const int16_t mode=MEM_H(0,S32(state+0x9e));
 struct Restore {uint8_t* rdram;uint32_t state;int16_t mode;~Restore(){MEM_H(0,S32(state+0x9e))=mode;}} restore{rdram,state,mode};
 MEM_H(0,S32(state+0x9e))=2;
 f(rdram,c);
}
void translate_mechanism(uint8_t*r,recomp_context*c,bool right,recomp_func_t*f){
 const float offset=mechanism_offset();if(offset)c->r5=bits(value(c->r5)+(right?offset:-offset));f(r,c);
}
void translate_panel(uint8_t*rdram,recomp_context*c,unsigned slot,recomp_func_t*f){
 const double grow=expansion();
 if(grow&&slot<4){
  // Native pair0/1 is left outer/inner; pair2/3 is right inner/outer.
  // Original panels have +/-58 local extents. Native phase0 opens136 units;
  // phase70 closes them with4 units of overlap (62C18/62C24). Preserve the
  // CLOSED endpoint and interpolate the extra aperture with the native slide,
  // without modifying state54/58, timing or the actor emergence animation.
  const float base=value(MEM_W(0,S32(0x800dac70+slot*16)));
  const float phase=std::abs(value(c->r5)-base);
  const float open=std::isfinite(phase)?std::fmax(0.f,1.f-phase/70.f):0.f;
  const float side=float(346*grow)*(slot>=2?1.f:-1.f);
  const float aperture=float(186*grow)*open*(slot&1?1.f:-1.f);
  c->r5=bits(value(c->r5)+side+aperture);
 }
 f(rdram,c);
}
void draw_preview(uint8_t*r,recomp_context*c,bool right,bool sprite,recomp_func_t*f){
 // The original orthographic display is240 high. Preserve preview size while
 // using half of the newly available width on each side of the centered grid.
 const float offset=float(160*expansion());
 if(offset){if(sprite)c->r6=bits(value(c->r6)+(right?offset:-offset));else c->r6=ADD32(c->r6,int(std::lround(right?offset:-offset)));}
 clay_sc_draw_preview(r,c,right,sprite,f);
}
void draw_player2_prompt(uint8_t*rdram,recomp_context*c,recomp_func_t*f){
 const uint32_t font=uint32_t(c->r4),sp=uint32_t(c->r29),scale=uint32_t(c->r7);
 if(!valid(font,0x28)||!valid(sp,0x1c)||!valid(scale,4)||!valid(sp-0x100,0x100)){f(rdram,c);return;}
 const uint32_t text=uint32_t(MEM_W(0x18,c->r29));
 if(!valid(text,1)){f(rdram,c);return;}
 const float sx=-value(MEM_W(0,S32(scale)));
 if(!std::isfinite(sx)||sx<=0||sx>8){f(rdram,c);return;}
 recomp_context measure=*c;measure.f_odd=&measure.f0.u32h;measure.r29=ADD32(c->r29,-0x80);measure.r4=S32(font);measure.r5=S32(text);
 func_80071738(rdram,&measure);
 const int width=int32_t(measure.r2);
 if(width<=0||width>1024){f(rdram,c);return;}
 const float outer=303+float(std::lround(143*expansion()*346.0/186.0));
 const int16_t original_x=MEM_H(0x10,S32(font)),original_origin=MEM_H(0x16,S32(font));
 const int16_t x=int16_t(std::lround((outer+104)*.5f-width*sx*.5f));
 MEM_H(0x10,S32(font))=x;MEM_H(0x16,S32(font))=x;
 f(rdram,c);
 MEM_H(0x10,S32(font))=original_x;MEM_H(0x16,S32(font))=original_origin;
}
void draw_name(uint8_t*rdram,recomp_context*c,bool right,recomp_func_t*f){
 if(expansion()){
  const uint32_t sprite=uint32_t(c->r5),scale=uint32_t(MEM_W(0x18,c->r29));
  if(valid(sprite,16)&&valid(scale,12)){
   const unsigned width=MEM_HU(6,S32(sprite));const float sx=-value(MEM_W(0,S32(scale)));
   // Black native box spans104..303. Its outer303 coordinate follows exactly
   // the same integer vertex warp as ui_background, including rounding.
   const float outer=303+float(std::lround(143*expansion()*346.0/186.0));
   const float center=(outer+104)*.5f*(right?1.f:-1.f);
   if(width&&width<=1024&&std::isfinite(sx)&&sx>0&&sx<=8)c->r6=bits(center-float(width)*.5f*sx);
  }
 }
 f(rdram,c);
}
}
