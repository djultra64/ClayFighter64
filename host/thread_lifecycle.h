// ClayTality 64 integration by djultra64.
#pragma once
#include "ultramodern/ultramodern.hpp"
#include <stdexcept>
namespace clay {
inline void stop_other_thread(uint8_t* rdram,int32_t address) {
 auto* thread=TO_PTR(OSThread,address);
 if(thread->state==OSThreadState::STOPPED)return;
 const auto queue=thread->queue;
 // Original osStopThread keeps the previous queue for a subsequent restart.
 // Only detach its intrusive link; the native thread remains semaphore-paused.
 thread->state=OSThreadState::STOPPED;
 if(queue!=NULLPTR&&!ultramodern::thread_queue_remove(rdram,queue,address))
  throw std::runtime_error("stopped_thread_missing_from_queue");
 thread->next=NULLPTR;
 thread->queue=queue;
}
}
