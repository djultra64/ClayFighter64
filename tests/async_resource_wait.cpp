// Actual guest framebuffer wait and VI actor; native SDK scheduling fixture by djultra64.
#include "recomp.h"
#include "ultramodern/ultramodern.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" void yield_self_1ms(uint8_t*);
extern "C" void func_80098A14(uint8_t*,recomp_context*);
extern "C" void func_800990E4(uint8_t*,recomp_context*);
extern "C" void original_wait(uint8_t*,recomp_context*);
extern "C" void yielding_wait(uint8_t*,recomp_context*);
static bool use_yield=false;
static std::atomic<uint32_t> current_fb{0x803da800},requested_fb{0x8038f800},vi_ticks{0};
std::atomic_bool exited{false};
extern "C" void osGetThreadId_recomp(uint8_t* rdram,recomp_context* c){c->r2=osGetThreadId(rdram,int32_t(c->r4));}
extern "C" void osRecvMesg_recomp(uint8_t* rdram,recomp_context* c){c->r2=osRecvMesg(rdram,int32_t(c->r4),int32_t(c->r5),int32_t(c->r6));}
extern "C" void osSendMesg_recomp(uint8_t* rdram,recomp_context* c){c->r2=osSendMesg(rdram,int32_t(c->r4),c->r5,int32_t(c->r6));}
// Hardware VI latch model only: guest status array is written by original guest VI code.
extern "C" void osViGetCurrentFramebuffer_recomp(uint8_t*,recomp_context* c){c->r2=S32(current_fb.load());}
extern "C" void osViSwapBuffer_recomp(uint8_t*,recomp_context* c){requested_fb=uint32_t(c->r4);}
extern "C" void osGetTime_recomp(uint8_t*,recomp_context* c){c->r2=0;c->r3=vi_ticks.load()*781250;}
void run_thread_function(uint8_t* rdram,uint64_t addr,uint64_t sp,uint64_t){
 recomp_context ctx{};ctx.r29=sp;ctx.f_odd=&ctx.f0.u32h;
 if(addr==0x100){
  osCreateThread(rdram,int32_t(0x80003000),20,0x300,0,int32_t(0x80410000),127);
  osStartThread(rdram,int32_t(0x80003000));
  if(use_yield)yielding_wait(rdram,&ctx);else original_wait(rdram,&ctx);
  func_80098A14(rdram,&ctx);
  fprintf(stderr,"wait_finished busy=%lld statuses=%u,%u,%u vi=%u\n",(long long)ctx.r2,uint32_t(MEM_W(0,S32(0x801448b0))),uint32_t(MEM_W(4,S32(0x801448b0))),uint32_t(MEM_W(8,S32(0x801448b0))),vi_ticks.load());
  std::_Exit(ctx.r2==0?0:1);
 }
 if(addr==0x200){osCreateThread(rdram,int32_t(0x80002000),6,0x100,0,int32_t(0x80408000),10);osStartThread(rdram,int32_t(0x80002000));for(;;)yield_self_1ms(rdram);}
 if(addr==0x300){func_800990E4(rdram,&ctx);std::_Exit(2);}
 std::_Exit(3);
}
int main(int argc,char** argv){
 use_yield=argc>1&&!strcmp(argv[1],"yield");std::vector<uint8_t> ram(0x800000);auto* rdram=ram.data();
 MEM_W(0,S32(0x8010d034))=3;MEM_W(0,S32(0x8010d038))=S32(0x80144890);MEM_W(0,S32(0x8010d03c))=S32(0x801448b0);
 for(int n=0;n<3;n++){MEM_W(n*4,S32(0x80144890))=S32(0x8038f800+n*0x25800);MEM_W(n*4,S32(0x801448b0))=n==2?3:2;}
 MEM_W(0,S32(0x800de034))=2;MEM_W(0,S32(0x800de038))=0;
 osCreateMesgQueue(rdram,int32_t(0x8010d010),int32_t(0x8010d028),1);
 osCreateMesgQueue(rdram,int32_t(0x8010d058),int32_t(0x8010d050),1);
 // Mutex token seeded before cooperative guest execution begins.
 TO_PTR(OSMesgQueue,int32_t(0x8010d010))->validCount=1;
 ultramodern::set_message_queue_control({});
 std::thread producer([]{for(;;){std::this_thread::sleep_for(std::chrono::milliseconds(10));current_fb=requested_fb.load();++vi_ticks;ultramodern::enqueue_external_message_src(int32_t(0x8010d058),0,false,ultramodern::EventMessageSource::Vi);}});producer.detach();
 ultramodern::set_entrypoint_thread();osCreateThread(rdram,int32_t(0x80001000),1,0x200,0,int32_t(0x80400000),0);osStartThread(rdram,int32_t(0x80001000));
 for(;;)std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
