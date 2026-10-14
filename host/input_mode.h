// Original guest combat/pause lifecycle bridge by djultra64.
#pragma once
#include <cstdint>
namespace clay::input {
bool combat_input_active();
void begin_combat_input(uint8_t* rdram);
void update_combat_input(uint8_t* rdram);
void end_combat_input();
}
