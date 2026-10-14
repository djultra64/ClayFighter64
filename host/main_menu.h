// Native main menu extension by djultra64.
#pragma once
#include "recomp.h"
namespace clay::main_menu {
void prepare(uint8_t* rdram);
uint32_t table(uint8_t* rdram, uint32_t original);
void draw_border(uint8_t*,recomp_context*,recomp_func_t* original);
// Called only at the original confirmed-action dispatch, never on hover.
bool confirm_exit(uint8_t* rdram);
bool exit_requested();
}
