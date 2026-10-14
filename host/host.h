// ClayTality 64 native integration by djultra64.
#pragma once
#include <atomic>
#include "recomp.h"
#include <vector>
#include "ultramodern/renderer_context.hpp"
extern std::vector<uint8_t> clay_rom;
extern std::atomic<uint64_t> clay_vis, clay_gfx, clay_polls;
[[noreturn]] void clay_fatal(const char*,uint32_t=0);
std::unique_ptr<ultramodern::renderer::RendererContext> clay_create_context(uint8_t*,ultramodern::renderer::WindowHandle,bool);
