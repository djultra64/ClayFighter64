// Native UI background coverage by djultra64.
#pragma once
#include "recomp.h"
namespace clay::ui {
// Capture the writer at original A45BC entry; call fix_scissor at its return.
uint32_t display_list_cursor(uint8_t* rdram);
bool fix_scissor(uint8_t* rdram,uint32_t original_begin);
// Replace ONLY verified full-screen pause/fade calls, preserving native colors.
void draw_fullscreen_fill(uint8_t* rdram,recomp_context* ctx,recomp_func_t* original);
// Native full-width transition bands retain their original vertical boundaries.
void draw_transition_band(uint8_t* rdram,recomp_context* ctx,recomp_func_t* original);
// At396C4 entry save a0/effect and current801108A0, then call at its return.
uint32_t vertex_cursor(uint8_t* rdram);
bool extend_circle_mask(uint8_t* rdram,uint32_t effect,uint32_t original_vertex_begin);
// Before selector grid draw5E688, widen ONLY original industrial frame vertices.
bool extend_selector_background_mesh(uint8_t* rdram,uint32_t original_part);
}
