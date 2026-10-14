// Scale only the selector's two side previews; original game draw functions remain. djultra64.
#include "sc_preview.h"
#include "sc_roster.h"
#include <array>
#include <bit>
#include <stdexcept>
void clay_sc_draw_preview(uint8_t* rdram,recomp_context* ctx,bool right,bool sprite,recomp_func_t* draw){
 if(!clay_roster::enabled()){draw(rdram,ctx);return;}
 const uint32_t address=MEM_W(0x14,ctx->r29);
 if(address<0x80000000u||uint64_t(address)+12>0x80800000ull)throw std::runtime_error("Selector preview scale address invalid");
 struct Restore {uint8_t* rdram;gpr pointer;std::array<int32_t,3> words;~Restore(){for(unsigned i=0;i<3;i++)MEM_W(i*4,pointer)=words[i];}} saved{rdram,gpr(S32(address)),{}};
 for(unsigned i=0;i<3;i++){saved.words[i]=MEM_W(i*4,saved.pointer);float value=std::bit_cast<float>(saved.words[i])*.75f;MEM_W(i*4,saved.pointer)=std::bit_cast<int32_t>(value);}
 const int shift=right?24:-24;
 if(sprite){float x=std::bit_cast<float>(int32_t(ctx->r6));ctx->r6=S32(std::bit_cast<int32_t>(x+shift));}
 else ctx->r6=ADD32(ctx->r6,shift);
 draw(rdram,ctx);
}
