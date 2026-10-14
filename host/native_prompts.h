// ClayTality 64 native-font controller prompts by djultra64.
#pragma once
#include <cstdint>
#include <string>
#include "recomp.h"
#include "input.h"
namespace clay::input {
// Main thread before guest boot; reads original licensed Kenney PNG assets.
bool load_native_prompt_assets(const std::string& config_path,
    const std::string& tiles_directory, std::string& error);
// Recompiler entry hooks. Original guest routines still measure and render
// original game text, using the ORIGINAL font and display-list sprite pipeline.
void prepare_native_prompt_measure(uint8_t* rdram, recomp_context* ctx); // 0x80071738
void prepare_native_prompt_draw(uint8_t* rdram, recomp_context* ctx); // 0x8007191C
// Explicit token resolved by the original text measure/draw hooks. Combat
// bindings and fixed menu bindings remain separate even outside combat.
std::string binding_marker(Action action);
std::string menu_marker(Action action);
// Original variant destructor frees owned entries that differ from the base.
// Restore borrowed icon aliases before that comparison and retire this lifetime.
void restore_native_prompt_font(uint8_t* rdram, uint64_t font);
// CPU-only caption normalization. Both controller buttons and keyboard keys
// retain native icon placeholders; renderer hooks resolve the genuine artwork.
std::string native_prompt_text(const std::string& source);
}
