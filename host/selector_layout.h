// Original selector geometry adaptation by djultra64.
#pragma once
#include "recomp.h"
namespace clay::selector {
// Training keeps both interactive native cursor sprites visible while choosing fighters.
void draw_fighters(uint8_t*,recomp_context*,recomp_func_t*);
// Scoped to the four gears and four sliding panels in original5E37C.
void translate_mechanism(uint8_t*,recomp_context*,bool right,recomp_func_t*);
// Spread the original paired panels into the added width, keeping slide state.
void translate_panel(uint8_t*,recomp_context*,unsigned native_slot,recomp_func_t*);
// Retains the existing SC preview scale and all native animation coordinates.
void draw_preview(uint8_t*,recomp_context*,bool right,bool sprite,recomp_func_t*);
// Center the second player prompt using its current device-specific glyph width.
void draw_player2_prompt(uint8_t*,recomp_context*,recomp_func_t*);
// Original name bitmaps, centered in the widened native black name boxes.
void draw_name(uint8_t*,recomp_context*,bool right,recomp_func_t*);
}
