// Actual original allocator budget regression, by djultra64.
#include "memory_budget.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <vector>
#include <stdexcept>
#include <cstdio>
extern "C" {
void switch_error(const char*,uint32_t,uint32_t){throw std::runtime_error("native switch failed");}
void func_800B32D0(uint8_t*,recomp_context*);
void func_800B34C4(uint8_t*,recomp_context*);
void func_800B3C70(uint8_t*,recomp_context*);
void func_800B4408(uint8_t*,recomp_context*);
void func_800B9154(uint8_t*,recomp_context*){throw std::runtime_error("native fatal");}
void func_800BCE04(uint8_t*,recomp_context*){throw std::runtime_error("native memory fatal");}
void func_800B8FC8(uint8_t*,recomp_context*){throw std::runtime_error("native diagnostic fatal");}
void osCreateMesgQueue_recomp(uint8_t*,recomp_context*){}
void osGetThreadId_recomp(uint8_t*,recomp_context*c){c->r2=27;}
void osRecvMesg_recomp(uint8_t*,recomp_context*c){c->r2=0;}
void osSendMesg_recomp(uint8_t*,recomp_context*c){c->r2=0;}
}
int main(int argc,char**argv){assert(argc<=2);std::vector<uint8_t> pristine(8<<20);if(argc==2){std::ifstream f(argv[1],std::ios::binary);assert(f);pristine.assign(std::istreambuf_iterator<char>(f),{});assert(pristine.size()==8<<20);}auto ram=pristine;auto*rdram=ram.data();
 auto context=[](){recomp_context c{};c.r29=S32(0x80040000);c.f_odd=&c.f0.u32h;return c;};
 auto alloc=[&](uint32_t n){auto c=context();c.r4=S32(0x800ee4b0);c.r5=n;c.r6=0;func_800B3C70(rdram,&c);return uint32_t(c.r2);};
 auto free=[&](uint32_t p){auto c=context();c.r4=S32(0x800ee4b0);c.r5=S32(p);func_800B4408(rdram,&c);};
 if(argc==1){auto c=context();MEM_W(0,S32(0x80000318))=8<<20;c.r4=S32(0x800ee4b0);func_800B32D0(rdram,&c);c=context();c.r4=S32(0x800ee4b0);c.r5=S32(0x80143080);c.r6=0x24c780;c.r7=0x300;MEM_W(16,c.r29)=16;func_800B34C4(rdram,&c);assert(alloc(0x24af80-0x25df0-16));pristine=ram;}
 bool failed=false;try{alloc(190128);}catch(const std::runtime_error&){failed=true;}assert(failed);ram=pristine;rdram=ram.data();auto c=context();auto old=c;assert(clay::extend_native_main_heap(rdram,&c));assert(c.r29==old.r29&&c.r4==old.r4&&c.r5==old.r5);for(uint32_t at=uint32_t(MEM_W(0,S32(0x800ee518)));at<0x80400000&&at;at=uint32_t(MEM_W(8,S32(at)))){if(uint32_t(MEM_W(0,S32(at)))>>16==0x5553){uint32_t size=uint32_t(MEM_W(4,S32(at)));assert(std::equal(ram.begin()+(at&0x7fffff)+16,ram.begin()+(at&0x7fffff)+size,pristine.begin()+(at&0x7fffff)+16));}}std::vector<uint32_t> blocks;
 for(auto n:{190128u,310000u,450000u,21488u,24904u}){auto p=alloc(n);assert(p>=0x80100010&&uint64_t(p)+n<=0x80600000);if(n==190128)assert(p>=0x80400010);blocks.push_back(p);}
 for(auto p:blocks)free(p);assert(clay::extend_native_main_heap(rdram,&c));auto p=alloc(190128);free(p);
 assert(std::equal(ram.begin()+0x600000,ram.end(),pristine.begin()+0x600000));
 ram=pristine;rdram=ram.data();MEM_W(0,S32(0x80400100))=1;auto dirty=ram;assert(!clay::extend_native_main_heap(rdram,&c));assert(ram==dirty);
 ram=pristine;rdram=ram.data();MEM_W(0,S32(0x80000318))=4<<20;auto small=ram;assert(!clay::extend_native_main_heap(rdram,&c));assert(ram==small);
 ram=pristine;rdram=ram.data();uint32_t head=MEM_W(0,S32(0x800ee518));MEM_W(12,S32(head))=head;auto invalid=ram;assert(!clay::extend_native_main_heap(rdram,&c));assert(ram==invalid);
 ram=pristine;rdram=ram.data();MEM_W(0,S32(0x800ee504))=0;c=context();c.r4=S32(0x800ee4b0);c.r5=S32(0x80143080);c.r6=0x24c780;c.r7=0x300;MEM_W(16,c.r29)=16;func_800B34C4(rdram,&c);assert(clay::extend_native_main_heap(rdram,&c));assert(alloc(310000));
 printf("PASS original allocator RED OOM190128; GREEN disjoint2MiB reservation,5allocations/nativefrees/coalescing/reuse; dirty/4MiB/corruptchain unchanged;80600000..80800000 untouched; original startupInit+AddMemory GREEN\n");
}
