// Donor-only VS placement by djultra64; original cast and all poses remain native.
#include "vs_alignment.h"
#include "sc_assets.h"
#include <cstdio>
#include <cstdlib>
namespace clay::ui {
namespace {bool valid(uint32_t p,unsigned bytes){return p>=0x80000000u&&uint64_t(p)+bytes<=0x80800000ull;}}
bool align_vs_actor(uint8_t*rdram,recomp_context*ctx){
 if(!clay_sc::enabled()||int32_t(ctx->r7)!=-37)return false;
 uint32_t actor=uint32_t(ctx->r4),object=uint32_t(ctx->r5);
 if(!valid(actor,0x1d8)||!valid(object,16))return false;
 uint32_t definition=uint32_t(MEM_W(0x1d4,S32(actor)));
 if(uint32_t(MEM_W(12,S32(object)))!=definition||!clay_sc_definition_supported(rdram,definition))return false;
 unsigned id=MEM_W(0,S32(object));int offset;
 // OriginalVS table has unused zero entries for these four IDs. Offsets derive
 // from actual original5DB24/A6520 idle-foot bounds and grounded-cast medianY−64.
 // Piece coordinates, sprite hotspots, scaling, simulation and animation remain.
 switch(id){case 6:offset=-18;break;case 11:offset=-25;break;case 12:offset=-24;break;case 16:offset=-22;break;default:return false;}
 if(MEM_W(0,S32(0x800dacf4+id*4))!=0)return false;
 ctx->r7=ADD32(ctx->r7,offset);
 if(std::getenv("CLAY_TRACE_VS")){static thread_local unsigned prints=0;if(prints++<16)fprintf(stderr,"{\"event\":\"native_vs_alignment\",\"character_id\":%u,\"y_offset\":%d,\"base_y\":%d}\n",id,offset,int32_t(ctx->r7));}
 return true;
}
void draw_vs_actor(uint8_t*rdram,recomp_context*ctx,recomp_func_t*original){align_vs_actor(rdram,ctx);original(rdram,ctx);}
}
