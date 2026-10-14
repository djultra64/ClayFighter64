// Scoped native fighter interpolation by djultra64.
#pragma once
#include "recomp.h"
#include <cstdint>
namespace clay {
// Entry8009DB18; only the verifiedcombat2800/80/FA0 pool gets12KiBextraDLspace.
void reserve_actor_interpolation_pool(recomp_context*);
// New combat invocation8003C740; prevents matching across new fights/cuts.
void reset_actor_interpolation();
// PC8003E130 immediately after original identity MODELVIEW load.
void begin_actor_interpolation_frame(uint8_t*);
// Around ONLY original main fighter sprite call at8004491C, after its delay slot.
bool begin_actor_piece(uint8_t*,recomp_context*);
void end_actor_piece(uint8_t*);
}
