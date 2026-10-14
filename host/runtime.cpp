// ClayTality 64 native integration by djultra64.
#include "host.h"
#include "thread_lifecycle.h"
#include "funcs.h"
#include "bindings.h"
#include "ultramodern/ultramodern.hpp"
#include <cstdio>
#include <cstdlib>
std::vector<uint8_t> clay_rom;
std::atomic<uint64_t> clay_vis{0},clay_gfx{0},clay_polls{0};
std::atomic_bool exited{false};
moodycamel::LightweightSemaphore graphics_shutdown_ready;
uint32_t clay_ai_length(){return ultramodern::get_remaining_audio_bytes();}
[[noreturn]] void clay_fatal(const char* name,uint32_t address){fprintf(stderr,"{\"event\":\"fatal\",\"symbol\":\"%s\",\"address\":\"0x%08X\"}\n",name,address);fflush(stderr);std::_Exit(71);}
namespace ultramodern {bool is_game_started(){return true;}}
extern "C" recomp_func_t* get_function(int32_t address){switch(uint32_t(address)){
#include "dispatch.inc"
}clay_fatal("unresolved_indirect",address);}
void run_thread_function(uint8_t* rdram,uint64_t address,uint64_t stack,uint64_t argument){fprintf(stderr,"{\"event\":\"guest_thread\",\"entry\":\"0x%08X\"}\n",uint32_t(address));recomp_context ctx{};ctx.status_reg=0x2000FF01;ctx.r29=stack;ctx.r4=argument;ctx.f_odd=&ctx.f0.u32h;get_function(address)(rdram,&ctx);}
extern "C" {
void osViGetCurrentLine_recomp(uint8_t*,recomp_context* ctx){
 auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(ultramodern::time_since_start()).count();
 // NTSC VI_CURRENT counts half-lines through a 525-half-line field.
 ctx->r2=((uint64_t(ns)*60/1000)%1000000)*525/1000000;
}
void recomp_syscall_handler(uint8_t*,recomp_context*,int32_t address){clay_fatal("mips_syscall",address);}
void do_break(uint32_t address){clay_fatal("mips_break",address);}
void switch_error(const char* name,uint32_t address,uint32_t){clay_fatal(name,address);}
gpr cop0_status_read(recomp_context* ctx){return S32(ctx->status_reg);}
void cop0_status_write(recomp_context* ctx,gpr value){ctx->status_reg=value;ctx->mips3_float_mode=(value&0x4000000)!=0;ctx->f_odd=ctx->mips3_float_mode?&ctx->f1.u32l:&ctx->f0.u32h;}
void osCreatePiManager_recomp(uint8_t*,recomp_context*){/* DMA uses the host PI completion queue directly. */}
void osPiStartDma_recomp(uint8_t* rdram,recomp_context* ctx){uint32_t device=uint32_t(ctx->r7)&0x0fffffff,dest=MEM_W(0x10,ctx->r29),size=MEM_W(0x14,ctx->r29);int32_t queue=MEM_W(0x18,ctx->r29);if(ctx->r6!=0||device>clay_rom.size()||size>clay_rom.size()-device||(dest&0x7fffff)+size>0x800000)clay_fatal("dma_bounds",device);for(uint32_t i=0;i<size;i++)rdram[((dest&0x7fffff)+i)^3]=clay_rom[device+i];ultramodern::enqueue_external_message_src(queue,0,false,ultramodern::EventMessageSource::Pi);ctx->r2=0;}
void osPiRawReadIo_recomp(uint8_t* rdram,recomp_context* ctx){uint32_t offset=uint32_t(ctx->r4)&0x0fffffff;if(offset+4>clay_rom.size())clay_fatal("pio_bounds",offset);MEM_W(0,ctx->r5)=(uint32_t(clay_rom[offset])<<24)|(uint32_t(clay_rom[offset+1])<<16)|(uint32_t(clay_rom[offset+2])<<8)|clay_rom[offset+3];ctx->r2=0;}
void osSpTaskLoad_recomp(uint8_t*,recomp_context*){/* HLE consumes the OSTask directly on StartGo. */}
void osSpTaskStartGo_recomp(uint8_t* rdram,recomp_context* ctx){fprintf(stderr,"{\"event\":\"submit_task\",\"type\":%u,\"task\":\"%08X\"}\n",uint32_t(MEM_W(0,ctx->r4)),uint32_t(ctx->r4));ultramodern::submit_rsp_task(rdram,int32_t(ctx->r4));}
void osSpTaskYield_recomp(uint8_t*,recomp_context*){
 // HLE finishes each submitted list atomically. A yield request can race with
 // completion on hardware too; report not-yielded when that completion arrives.
 fprintf(stderr,"{\"event\":\"rsp_yield_requested\",\"mode\":\"finish_current_task\"}\n");
}
void osSpTaskYielded_recomp(uint8_t*,recomp_context* ctx){ctx->r2=0;}
void osAiSetFrequency_recomp(uint8_t*,recomp_context* ctx){ultramodern::set_audio_frequency(ctx->r4);ctx->r2=ctx->r4;}
void osAiGetLength_recomp(uint8_t*,recomp_context* ctx){ctx->r2=ultramodern::get_remaining_audio_bytes();}
void osAiSetNextBuffer_recomp(uint8_t* rdram,recomp_context* ctx){ultramodern::queue_audio_buffer(rdram,ctx->r4,ctx->r5);ctx->r2=0;}
void osEepromProbe_recomp(uint8_t*,recomp_context* ctx){ctx->r2=0;/* Device absent: truthful until save backend exists. */}
void osDpSetNextBuffer_recomp(uint8_t*,recomp_context*){clay_fatal("raw_rdp_buffer_not_implemented");}
void __osAiDeviceBusy_recomp(uint8_t*,recomp_context* ctx){ctx->r2=ultramodern::get_remaining_audio_bytes()!=0;}
void osEepromRead_recomp(uint8_t*,recomp_context* ctx){ctx->r2=-1;}
void osEepromWrite_recomp(uint8_t*,recomp_context* ctx){ctx->r2=-1;}
void osDpGetStatus_recomp(uint8_t*,recomp_context* ctx){ctx->r2=0;}
void osDpSetStatus_recomp(uint8_t*,recomp_context* ctx){if(ctx->r4&8)clay_fatal("dp_freeze_not_implemented");}
}

extern "C" void __real_osStartThread_recomp(uint8_t*,recomp_context*);
extern "C" void __wrap_osStartThread_recomp(uint8_t* rdram,recomp_context* ctx){
 auto* target=TO_PTR(OSThread,int32_t(ctx->r4));
 if(std::getenv("CLAY_TRACE_QUEUES")) fprintf(stderr,"{\"event\":\"start_thread\",\"pointer\":\"%08X\",\"id\":%d,\"state\":%u,\"queue\":\"%08X\",\"next\":\"%08X\",\"caller\":\"%08X\"}\n",uint32_t(ctx->r4),target->id,unsigned(target->state),uint32_t(target->queue),uint32_t(target->next),uint32_t(ctx->r31));
 __real_osStartThread_recomp(rdram,ctx);
}

extern "C" void __real_osStopThread_recomp(uint8_t*,recomp_context*);
extern "C" void __wrap_osStopThread_recomp(uint8_t* rdram,recomp_context* ctx){
 const auto self=ultramodern::this_thread();
 const int32_t address=ctx->r4?int32_t(ctx->r4):self;
 if(std::getenv("CLAY_TRACE_QUEUES")) {
  auto* target=TO_PTR(OSThread,address);
  fprintf(stderr,"{\"event\":\"stop_thread\",\"pointer\":\"%08X\",\"id\":%d,\"state\":%u,\"queue\":\"%08X\"}\n",uint32_t(address),target->id,unsigned(target->state),uint32_t(target->queue));
 }
 if(address==self){
  TO_PTR(OSThread,address)->state=OSThreadState::STOPPED;
  __real_osStopThread_recomp(rdram,ctx);
 }else clay::stop_other_thread(rdram,address);
}
