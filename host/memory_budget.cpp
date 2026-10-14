// Native Expansion Pak heap reservation by djultra64.
#include "memory_budget.h"
#include <cstdio>
extern "C" void func_800B3770(uint8_t*,recomp_context*);
namespace clay {
namespace {
constexpr uint32_t owner=0x800ee4b0,begin=0x80400000,end=0x80600000;
uint32_t word(uint8_t*rdram,uint32_t p){return uint32_t(MEM_W(0,S32(p)));}
}
bool extend_native_main_heap(uint8_t*rdram,recomp_context*ctx){
 if(!rdram||!ctx||word(rdram,0x80000318)!=0x800000||word(rdram,owner+0x54)!=1||word(rdram,owner+4)!=0x800b3c70||word(rdram,owner+12)!=0x800b4408||word(rdram,owner+0x58)!=16||word(rdram,owner+0x60)!=0xfffffff0)return false;
 uint32_t p=word(rdram,owner+0x68),prev=0;unsigned count=0;bool reserved=false;uint32_t reserved_end=begin;
 while(p){
  if(++count>4096||p<0x80100000||p>=0x80800000||(p&15)||(word(rdram,p)>>16!=0x5553&&word(rdram,p)>>16!=0x4652)||word(rdram,p+12)!=prev)return false;
  uint32_t bytes=word(rdram,p+4),next=word(rdram,p+8);
  if(bytes<16||(bytes&15)||bytes>0x80800000-p||(next&&next<p+bytes))return false;
  if(p<end&&p+bytes>begin){if(p!=reserved_end||p+bytes>end)return false;reserved=true;reserved_end=p+bytes;}
  prev=p;p=next;
 }
 if(!count||prev!=word(rdram,owner+0x6c))return false;
 if(reserved)return reserved_end==end; // Original allocator may have split the reserved region.
 for(uint32_t i=begin;i<end;i+=4)if(word(rdram,i))return false;
 uint32_t sp=uint32_t(ctx->r29);if(sp<0x80001000||sp>0x80400000||(sp&7))return false;
 recomp_context local=*ctx;local.f_odd=&local.f0.u32h;local.r29=S32(sp-0x100);local.r4=S32(owner);local.r5=S32(begin);local.r6=end-begin;
 func_800B3770(rdram,&local);
 std::fprintf(stderr,"{\"event\":\"native_heap_expansion\",\"begin\":%u,\"bytes\":%u,\"gpu_ram_limit\":8388608}\n",begin,end-begin);
 return true;
}
}
