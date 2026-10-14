// Original selector extensions using verified donor assets; djultra64.
#pragma once
#include <cstdint>
#include <vector>
#include "recomp.h"
namespace clay_roster {
void configure_image(const std::vector<uint8_t>& image,int demo_character=-1);
bool enabled();
}
extern "C" {
int clay_sc_roster_count();
int clay_sc_roster_columns();
int clay_sc_roster_state_bytes();
int clay_sc_roster_flag_offset();
int clay_sc_roster_row_stride(uint64_t row);
int clay_sc_roster_id_from_slot(uint64_t slot);
int clay_sc_roster_slot_from_id(uint64_t id);
int clay_sc_roster_extra_allowed(uint64_t id);
int clay_sc_roster_unlock_flags(uint64_t flags);
int clay_sc_roster_slot_unlocked(uint8_t* rdram,uint64_t slot);
int clay_sc_roster_secret_shift(uint64_t slot);
uint64_t clay_sc_roster_coordinate_address(uint8_t* rdram,uint64_t original_address);
uint64_t clay_sc_roster_coordinates(uint8_t* rdram);
void clay_sc_roster_augment_portraits(uint8_t* rdram,recomp_context* ctx,uint64_t source_rom);
uint64_t clay_sc_roster_extend_grid(uint8_t* rdram,recomp_context* ctx,uint64_t original_part);
void clay_sc_roster_release_grid(uint8_t* rdram,recomp_context* ctx);
void clay_sc_roster_draw_grid(uint8_t* rdram,recomp_context* ctx);
void clay_sc_roster_install(uint8_t* rdram,recomp_context* ctx);
}
