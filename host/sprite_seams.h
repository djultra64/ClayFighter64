// Native sprite slice continuity by djultra64.
#pragma once
#include "recomp.h"
namespace clay {
void begin_sprite_slices(uint8_t* rdram, recomp_context* ctx);
void limit_sprite_slice_rows(uint8_t* rdram,recomp_context* ctx,unsigned stack_offset);
unsigned end_sprite_slices(uint8_t* rdram);
}
