// Authored descriptions of the integrated game's native commands. djultra64.
#pragma once
#include "input.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace clay::training {
enum class MoveCategory {Basic,Special,Super,Combo,Claytality};
struct Move {
    unsigned record=0,variant=0;
    MoveCategory category=MoveCategory::Basic;
    std::string name,requirements;
    std::vector<uint16_t> motion;
    uint16_t trigger=0,animation=0;
    uint16_t hold_mask=0,hold_ticks=0;
    float minimum_distance=0,maximum_distance=0;
};
std::vector<Move> read_moves(uint8_t* rdram,uint32_t actor);
std::string move_command(const Move& move,const std::array<input::Action,6>& attacks,bool facing_right);
const char* category_name(MoveCategory category);
const char* character_name(int id);
}
