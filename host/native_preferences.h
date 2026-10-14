// Original game Options, portable and versioned. djultra64.
#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include "recomp.h"
namespace clay::native_preferences {
struct Options {std::array<uint16_t,28> values{};bool operator==(const Options&)const=default;};
Options defaults();
Options capture(uint8_t* rdram);
bool load_options(const std::filesystem::path&,Options&,std::string& error);
bool save_options(const std::filesystem::path&,const Options&,std::string& error);
// Storage is configured before starting the guest. All subsequent RAM access and
// writes run on the original menu owner thread, never the SDL/audio threads.
void initialize_storage(const std::filesystem::path&);
void after_initialize(uint8_t*,recomp_context*);
void options_changed(uint8_t*,recomp_context*);
void audio_ready(uint8_t*,recomp_context*);
}
