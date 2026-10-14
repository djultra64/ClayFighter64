// ClayTality 64 native prompt integration by djultra64.
#pragma once
#include "input.h"
namespace clay::input {
struct NativePromptSnapshot {
    Family family = Family::Keyboard;
    std::vector<Prompt> prompts;
    bool visible = true;
    bool expanded = false;
};
// Retained integration entry point. The rejected modern HUD is disabled; this
// installs no render hook. Native game text/glyph integration owns rendering.
void install_hud();
// Main input thread publishes actual mappings for native prompt rendering.
void publish_hud(const Mapper& mapper, bool visible = true, bool expanded = false);
// Native text renderer can consume this immutable copy without SDL/Mapper calls.
NativePromptSnapshot native_prompt_snapshot();
// Retained test/integration entry point; emits no generic overlay geometry.
void draw_hud_frame();
}
