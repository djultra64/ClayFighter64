// Native port options by djultra64. Original game typography and sprite pipeline.
#pragma once
#include "recomp.h"
#include <cstdint>
#include <string>
#include <vector>
namespace clay::port {
// Entry hook8006B014: names the original Options category before native letter animation.
void prepare_native_options_labels(uint8_t*);
// Called after native controller poll in 80067980; true skips to its epilogue.
bool update_native_options(uint8_t*,recomp_context*);
// Called at80066E68 after the original animated category prefix.
// True skips the original Controls/Display body to the native epilogue.
bool draw_native_options(uint8_t*,recomp_context*);
struct OptionsView { std::string title; std::vector<std::string> rows; unsigned selected; std::string status; };
OptionsView options_view(uint8_t*,unsigned menu);
void reset_options_cursor();
}
