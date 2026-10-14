// Guest thread publishes mode; SDL never reads guest RDRAM concurrently. djultra64.
#include "input_mode.h"
#include "recomp.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
namespace clay::input {
namespace {std::atomic_bool active{false},combat_loaded{false};}
bool combat_input_active(){return active.load(std::memory_order_acquire);}
void update_combat_input(uint8_t* rdram){
 bool next=combat_loaded.load(std::memory_order_acquire)&&MEM_W(0,S32(0x80106D58))==0&&MEM_W(0,S32(0x8010849C))==0;
 bool old=active.exchange(next,std::memory_order_acq_rel);
 if(old!=next&&std::getenv("CLAY_TRACE_GAME"))fprintf(stderr,"{\"event\":\"combat_input_mode\",\"active\":%s,\"paused\":%s}\n",next?"true":"false",MEM_W(0,S32(0x8010849C))?"true":"false");
 if(next&&std::getenv("CLAY_TRACE_GAME")){
  static thread_local unsigned sample=0;
  if(sample++%60==0){
   uint32_t p1=MEM_W(0,S32(0x801028B8)),p2=MEM_W(0,S32(0x801028BC));
   auto valid=[](uint32_t p){return p>=0x80000000u&&p<=0x807FFD3Cu;};
   if(valid(p1)&&valid(p2))fprintf(stderr,"{\"event\":\"combat_health\",\"p1_actor\":%u,\"p2_actor\":%u,\"p1_hp\":%d,\"p2_hp\":%d}\n",p1,p2,MEM_W(0,S32(p1+0x2C0)),MEM_W(0,S32(p2+0x2C0)));
  }
 }
}
void begin_combat_input(uint8_t* rdram){combat_loaded=true;update_combat_input(rdram);
 if(std::getenv("CLAY_TRACE_GAME"))fprintf(stderr,"{\"event\":\"combat_loaded\",\"p1\":%d,\"p2\":%d,\"attract\":%s}\n",MEM_W(0,S32(0x801028A8)),MEM_W(0,S32(0x801028B0)),MEM_W(0,S32(0x80106D58))?"true":"false");
}
void end_combat_input(){combat_loaded=false;active.store(false,std::memory_order_release);
 if(std::getenv("CLAY_TRACE_GAME"))fputs("{\"event\":\"combat_finished\"}\n",stderr);
}
}
