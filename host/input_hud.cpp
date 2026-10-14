// ClayTality 64 native prompt integration by djultra64.
// The modern utility bar was rejected. Keep its integration entry points inert
// until the original game's text renderer and bitmap glyphs are connected.
#include "input_hud.h"
#include <mutex>
namespace clay::input {
namespace {
std::mutex snapshot_mutex;
NativePromptSnapshot published;
}
void install_hud() {}
void publish_hud(const Mapper& mapper, bool visible, bool expanded) {
    NativePromptSnapshot next;
    next.family = mapper.family();
    next.visible = visible;
    next.expanded = expanded;
    for (size_t i = 0; i < action_count; i++)
        next.prompts.push_back(mapper.prompt(Action(i)));
    std::lock_guard lock(snapshot_mutex);
    published = std::move(next);
}
NativePromptSnapshot native_prompt_snapshot() {
    std::lock_guard lock(snapshot_mutex);
    return published;
}
void draw_hud_frame() {}
}
