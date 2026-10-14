// Native stage polygon positioning for Training. djultra64.
#pragma once
#include "training_session.h"
#include <array>
#include <cstdint>
namespace clay::training {
// Simulation thread, after the native combat basis and active stage node are ready.
// Uses the native collision edge gap (rounded upward to an integer game unit).
// Invalid geometry or insufficient space leaves all guest memory untouched.
bool reset_position(uint8_t* rdram,uint32_t world,std::array<uint32_t,2> actors,
                    const Options& options,float edge_gap=100.0f);
}
