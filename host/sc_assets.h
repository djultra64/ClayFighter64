// Optional original63-engine donor asset experiment; djultra64.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace clay_sc {
inline constexpr unsigned ids[]{6,11,12,16};
void configure_asset_image(const std::vector<uint8_t>& image,int demo_character=-1);
bool enabled();
std::string sha256(const std::vector<uint8_t>& data);
}
extern "C" {
int clay_sc_definition_supported(uint8_t* rdram,uint64_t definition);
int clay_sc_sequence_header_bytes(uint8_t* rdram,uint64_t sequence);
void clay_sc_register_model(uint8_t* rdram,uint64_t model);
void clay_sc_unregister_model(uint8_t* rdram,uint64_t model);
void clay_sc_demo_combat_selection(uint8_t* rdram,uint64_t combat_config);
void clay_sc_demo_selection(uint8_t* rdram);
}
