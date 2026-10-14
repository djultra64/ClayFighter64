// Native Training stage panel lifecycle and portable last selection. djultra64.
#pragma once
#include "recomp.h"
#include <filesystem>
namespace clay::training {
void initialize_stage_storage(const std::filesystem::path& path);
// Call after 6DAE0's native 6CFBC initialization, before its update thread runs.
void stage_panel_started(uint8_t* rdram);
// Call before 6D77C reads its timer; the native introduction runs to readiness.
void stage_panel_tick(uint8_t* rdram);
// Wrap 6D77C's call to 6D544; restores the native stage-unlock option after input.
void stage_input(uint8_t* rdram,recomp_context* ctx,recomp_func_t* original);
// Insert immediately after 6D544 polls 800A0CA8, before aggregating navigation.
void stage_input_polled(uint8_t* rdram);
// Call after native 6DAE0 has returned and freed its stage resources.
void stage_panel_draw(uint8_t* rdram,recomp_context* ctx);
void stage_panel_finished(uint8_t* rdram);
// The outer native flow must return to the opponent selector on Back.
bool take_stage_back();
}
