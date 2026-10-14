// Donor-only original VS placement by djultra64.
#pragma once
#include "recomp.h"
namespace clay::ui {
// ONLY call at original VS6D208/6D27C, after their native delay slots.
bool align_vs_actor(uint8_t* rdram,recomp_context* ctx);
void draw_vs_actor(uint8_t* rdram,recomp_context* ctx,recomp_func_t* original);
}
