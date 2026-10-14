#include "thread_lifecycle.h"
#include <array>
#include <cstdio>
#include <stdexcept>
#include <cstring>
// Queue fixtures have no executing guest; this supplies only diagnostic caller ID.
namespace ultramodern {PTR(OSThread) this_thread(){return NULLPTR;}}
static void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
int main(int argc,char** argv){
 try {
  alignas(OSThread) std::array<uint8_t,2048> ram{};auto* rdram=ram.data();
  constexpr int32_t mq=int32_t(0x80000020),idle=int32_t(0x80000080),a=int32_t(0x800000c0),b=int32_t(0x80000100),c=int32_t(0x80000140),missing=int32_t(0x80000180);
  auto* i=TO_PTR(OSThread,idle);auto* ta=TO_PTR(OSThread,a);auto* tb=TO_PTR(OSThread,b);auto* tc=TO_PTR(OSThread,c);
  i->id=1;i->priority=0;ta->id=27;ta->priority=30;tb->priority=20;tc->priority=10;
  i->state=ta->state=tb->state=tc->state=OSThreadState::QUEUED;
  ultramodern::thread_queue_insert(rdram,ultramodern::running_queue,idle);
  ultramodern::thread_queue_insert(rdram,mq,c);ultramodern::thread_queue_insert(rdram,mq,b);ultramodern::thread_queue_insert(rdram,mq,a);
  const std::string mode=argc>1?argv[1]:"stop";
  if(mode=="stop_running") {
   require(ultramodern::thread_queue_remove(rdram,mq,a),"detach running fixture from original MQ");
   ultramodern::thread_queue_insert(rdram,ultramodern::running_queue,a);
   clay::stop_other_thread(rdram,a);
   require(ta->state==OSThreadState::STOPPED&&ta->queue==ultramodern::running_queue,"Stop runnable worker state and remembered queue");
   require(ultramodern::thread_queue_peek(rdram,ultramodern::running_queue)==idle&&i->next==0,"Stop runnable worker must preserve idle");
  } else if(mode=="remove") {
   require(ultramodern::thread_queue_remove(rdram,mq,b),"remove nonhead must succeed");
   require(ta->next==c,"remove nonhead must preserve tail");
   require(!ultramodern::thread_queue_remove(rdram,mq,missing),"remove missing must terminate false");
   require(ultramodern::thread_queue_remove(rdram,mq,a),"remove head");
   require(ultramodern::thread_queue_remove(rdram,mq,c),"remove tail");
   require(!ultramodern::thread_queue_remove(rdram,mq,missing),"remove empty must terminate false");
  } else {
   const int32_t target=mode=="stop_nonhead"?b:a;auto* t=TO_PTR(OSThread,target);
   clay::stop_other_thread(rdram,target);
   require(t->state==OSThreadState::STOPPED,"nonself Stop must mark stopped");
   require(t->queue==mq,"Stop must remember old queue");
   require(t->next==0,"Stopped thread must not retain stale live link");
   require(ultramodern::thread_queue_peek(rdram,mq)==(target==a?b:a),"Stop must unlink target");
   if(target==b)require(ta->next==c,"Stop nonhead must preserve tail");
   clay::stop_other_thread(rdram,target);
   // SDK Start schedules the removed worker; simulates its next blocking receive.
   ultramodern::thread_queue_insert(rdram,ultramodern::running_queue,target);
   require(ultramodern::thread_queue_pop(rdram,ultramodern::running_queue)==target,"Restart must choose worker");
   ultramodern::thread_queue_insert(rdram,mq,target);
   require(ultramodern::thread_queue_peek(rdram,ultramodern::running_queue)==idle&&i->next==0,"Restart/reblock must preserve idle runnable");
  }
  puts("Thread queue removal and stopped-worker lifecycle passed");return 0;
 } catch(const std::exception& error){fprintf(stderr,"%s\n",error.what());return 1;}
}
