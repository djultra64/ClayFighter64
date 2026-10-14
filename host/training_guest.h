// Verified guest Training bridge, djultra64.
#pragma once
#include <cstdint>
#include "recomp.h"
#include "training_session.h"
#include "input.h"
#include <array>
#include <memory>
#include <string>
#include <vector>
namespace clay::training {
struct Move;
struct Snapshot {
    bool active=false;
    Options options;
    DamageView damage;
    unsigned attempts=0,successes=0;
    int character=-1,opponent=-1;
    std::shared_ptr<const std::vector<Move>> moves;
    std::array<input::Action,6> attacks={input::Action::Secondary,input::Action::CLeft,
        input::Action::CUp,input::Action::Primary,input::Action::CDown,input::Action::CRight};
    bool facing_right=true;
    int pinned_move=-1;
    std::vector<std::string> history;
    bool second_player_connected=false;
};
Snapshot snapshot();
void configure(const Options& options);
void request_restart();
void request_exit();
void request_change_fighters();
void pin_move(int index);
void request_pause();
void set_second_player_connected(bool connected);
bool take_pause_request();
bool pause_tick(uint8_t* rdram,recomp_context* ctx);
bool draw_pause(uint8_t* rdram,recomp_context* ctx);
void draw_hud(uint8_t* rdram,recomp_context* ctx);
void select_fighters(uint8_t* rdram,recomp_context* ctx,recomp_func_t* original);
void selection_tick(uint8_t* rdram);
void return_to_opponent_selection(uint8_t* rdram);
bool selecting_opponent();
bool selecting_fighters();
bool selection_input_ready(uint16_t buttons);
void run_match(uint8_t* rdram,recomp_context* ctx,recomp_func_t* original);
void begin_combat(uint8_t* rdram);
int32_t damage_applied(uint8_t* rdram,uint32_t target,unsigned amount,int32_t result,bool chip);
void prepare_input(uint8_t* rdram,recomp_context* ctx);
void observe_world(uint8_t* rdram,uint32_t world,recomp_context* ctx=nullptr);
}
