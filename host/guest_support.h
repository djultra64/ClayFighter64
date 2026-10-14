#pragma once
#include "recomp.h"
#include "bindings.h"
#include "sc_assets.h"
#include "native_prompts.h"
#include "input_mode.h"
#include "training_guest.h"
#include "training_stage.h"
#include "sc_preview.h"
#include "sc_roster.h"
#include "port_options.h"
#include "interpolation.h"
#include "sprite_seams.h"
#include "ui_background.h"
uint32_t clay_ai_length();
extern "C" void yield_self_1ms(uint8_t*);
// N64Recomp emits unsigned HI/LO storage for signed 64-bit division.
// Preserve the result's bits without aliasing signed and unsigned pointers.
static inline void DDIV(int64_t a,int64_t b,uint64_t* quotient,uint64_t* remainder){
 int64_t q=0,r=0;
 DDIV(a,b,&q,&r);
 *quotient=static_cast<uint64_t>(q);
 *remainder=static_cast<uint64_t>(r);
}

#include "vs_alignment.h"

#include "options_border.h"

#include "memory_budget.h"

#include "title_audio.h"
#include "selector_layout.h"
#include "logo64.h"

#include "sc_bios.h"

#include "main_menu.h"
