// Native Expansion Pak heap reservation by djultra64.
#pragma once
#include "recomp.h"
namespace clay {
// Startup ONLY, immediately after original main-memory-manager initialization.
// Calls the original AddMemory implementation; caller registers remain unchanged.
bool extend_native_main_heap(uint8_t* rdram,recomp_context* ctx);
}
