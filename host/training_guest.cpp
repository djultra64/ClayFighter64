// Observe the actual native simulation boundary, independent of presentation.
// Own work: djultra64.
#include "training_guest.h"
#include "recomp.h"
#include "main_menu.h"
#include "test_input.h"
#include "training_options.h"
#include "move_list.h"
#include "training_position.h"
#include "input_hud.h"
#include "native_prompts.h"
#include <algorithm>
#include <cmath>
#include <array>
#include <atomic>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <mutex>
extern "C" void func_80080880(uint8_t*,recomp_context*);
extern "C" void func_800636E4(uint8_t*,recomp_context*);
namespace clay::training {
namespace {
std::mutex mutex;
Session session;
std::atomic<bool> selection{false},rival_selection{false},selection_release{false};
std::array<uint32_t,2> actors{};
std::array<int,2> maximum_hp{};
uint32_t live_world=0;
bool repeat_match=false,exit_match=false,change_fighters=false,opponent_first=false,unlock_first=false;
bool repeated_scene=false;
std::atomic<bool> pause_requested{false},second_connected{false};
std::shared_ptr<const std::vector<Move>> catalog;
std::array<input::Action,6> mapped_attacks{};
bool facing=true,played_finisher=false;
int pinned=-1;
bool position_pending=true,spacing_pending=false,finish_window_seen=false;
struct HistoryEntry {uint16_t buttons;unsigned ticks;};
std::vector<HistoryEntry> input_history;
uint16_t previous_input=0;
int character=-1,opponent=-1;
std::filesystem::path storage;
bool valid(uint32_t p,uint32_t bytes) {
    return p>=0x80000000u&&uint64_t(p)+bytes<=0x80800000ull&&(p&3)==0;
}
int32_t word(uint8_t* rdram,uint32_t p) {return MEM_W(0,S32(p));}
void put(uint8_t* rdram,uint32_t p,int32_t value){MEM_W(0,S32(p))=value;}
float scalar(uint8_t* rdram,uint32_t p){return std::bit_cast<float>(word(rdram,p));}
}
Snapshot snapshot(){
    std::lock_guard lock(mutex);std::vector<std::string> history;
    constexpr uint16_t masks[]={0x8000,0x4000,0x1000,0x2000,0x20,0x10,8,4,2,1,0x800,0x400,0x200,0x100};
    for(const auto& entry:input_history){
        std::string line;
        for(unsigned i=0;i<14;i++)if(entry.buttons&masks[i]){if(!line.empty())line+=" and ";line+=input::binding_marker(input::Action(i));}
        if(line.empty())line="Neutral";
        history.push_back(line+" "+std::to_string(entry.ticks)+" ticks");
    }
    return {session.active(),session.options(),session.damage_view(),session.attempts(),session.successes(),character,opponent,catalog,mapped_attacks,facing,pinned,history,second_connected.load()};
}
void initialize_storage(const std::filesystem::path& path){
    std::lock_guard lock(mutex);storage=path;Options loaded;std::string error;
    load_options(path,loaded,error);session.set_options(loaded);
    if(!error.empty())fprintf(stderr,"Training preferences: %s\n",error.c_str());
}
void configure(const Options& options){
    std::lock_guard lock(mutex);const auto before=session.options();session.set_options(options);
    if(options.preset==Preset::Claytalities&&(options.direct_finisher!=before.direct_finisher||options.automatic_distance!=before.automatic_distance||options.extended_window!=before.extended_window))session.request_reset();
    if(!storage.empty()){std::string error;if(!save_options(storage,session.options(),error))fprintf(stderr,"Training preferences: %s\n",error.c_str());}
}
void request_restart(){std::lock_guard lock(mutex);session.request_reset();}
void request_exit(){std::lock_guard lock(mutex);exit_match=true;change_fighters=false;}
void request_change_fighters(){std::lock_guard lock(mutex);change_fighters=exit_match=true;}
void pin_move(int index){std::lock_guard lock(mutex);
    const int selected=catalog&&index>=0&&size_t(index)<catalog->size()?index:-1;
    if(selected!=pinned&&selected>=0&&session.options().preset==Preset::Claytalities&&catalog->at(selected).category==MoveCategory::Claytality)session.request_reset();
    pinned=selected;
}
void set_second_player_connected(bool connected){second_connected=connected;}
void request_pause(){std::lock_guard lock(mutex);if(session.active()&&session.phase()!=Phase::Selecting&&live_world)pause_requested=true;}
bool take_pause_request(){return pause_requested.exchange(false);}
void return_to_opponent_selection(uint8_t* rdram){opponent_first=true;put(rdram,0x80108cf0,2);const uint32_t game=word(rdram,0x800ddd40);if(valid(game,0x150))put(rdram,game,1);clay::testing::publish_input_phase(clay::testing::InputPhase::Loading); }
bool selecting_opponent(){return rival_selection.load(std::memory_order_acquire);}
bool selecting_fighters(){return selection.load(std::memory_order_acquire);}
bool selection_input_ready(uint16_t buttons){
    if(!selection_release.load(std::memory_order_acquire))return true;
    if(buttons)return false;
    selection_release.store(false,std::memory_order_release);return true;
}
void select_fighters(uint8_t* rdram,recomp_context* ctx,recomp_func_t* original){
    if(!clay::main_menu::training_requested()){original(rdram,ctx);return;}
    {std::lock_guard lock(mutex);session.begin_selection();exit_match=repeat_match=change_fighters=false;pause_requested=false;live_world=0;}
    {
        std::lock_guard lock(mutex);const uint32_t game=word(rdram,0x800ddd40);
        if(valid(game,0x150)){
            if(session.options().last_character>=0)put(rdram,game+0xc,session.options().last_character);
            if(session.options().last_opponent>=0)put(rdram,game+0x10,session.options().last_opponent);
        }
    }
    selection=true;rival_selection=false;selection_release=false;
    MEM_W(0x64,ctx->r29)=0;MEM_W(0x34,ctx->r29)=0; // Verified 6FBD0 parent mode/continuation slots.
    ctx->r4=0;ctx->r5=opponent_first?1:0;ctx->r6=0;unlock_first=opponent_first;opponent_first=false; // Native versus selector: zero means interactive, one preselects.
    original(rdram,ctx);
    selection=false;rival_selection=false;selection_release=false;
    if(ctx->r2!=0){std::lock_guard lock(mutex);session.leave();clay::main_menu::clear_training_request();}
    else{
        std::lock_guard lock(mutex);const uint32_t game=word(rdram,0x800ddd40);
        if(valid(game,0x150)){auto remembered=session.options();remembered.last_character=word(rdram,game+0xc)&0x7f;remembered.last_opponent=word(rdram,game+0x10)&0x7f;session.set_options(remembered);if(!storage.empty()){std::string error;save_options(storage,remembered,error);}}
    }
    if(std::getenv("CLAY_TRACE_GAME")){const uint32_t game=word(rdram,0x800ddd40);fprintf(stderr,"{\"event\":\"training_selection_complete\",\"result\":%d,\"p1\":%d,\"p2\":%d}\n",int32_t(ctx->r2),valid(game,0x150)?word(rdram,game+0xc)&0x7f:-1,valid(game,0x150)?word(rdram,game+0x10)&0x7f:-1);}
}
void selection_tick(uint8_t* rdram){
    if(!selecting_fighters())return;
    const uint32_t selector=word(rdram,0x800ddd30);
    if(!valid(selector,0xb8))return;
    if(unlock_first&&word(rdram,selector+0x10)==1&&word(rdram,selector+0x14)==0){put(rdram,selector,word(rdram,selector)&~1);unlock_first=false;}
    const bool first=word(rdram,selector+0x6c)!=0;
    const bool second=word(rdram,selector+0x70)!=0;
    const bool rival=first&&!second;
    if(rival&&(word(rdram,0x80118ccc)&0x4000)){
        // Deliver Back to the original P1 unselect handler instead of bypassing
        // its portrait/preview lifetime and native animation cleanup.
        put(rdram,0x80118cb4,word(rdram,0x80118cb4)|0x4000);
        put(rdram,0x80118ccc,word(rdram,0x80118ccc)&~0x4000);
    }
    if(!first||!second)MEM_H(0,S32(selector+0x9c))=0x4eb; // Original reset value; no auto-pick deadline.
    const bool old=rival_selection.exchange(rival,std::memory_order_acq_rel);
    if(old!=rival)selection_release.store(true,std::memory_order_release);
    const bool both=first&&second;
    if(std::getenv("CLAY_TRACE_TRAINING")){
        static thread_local unsigned n=0;
        if(n++%120==0||(word(rdram,0x80118cb4)&0x9000))fprintf(stderr,"{\"event\":\"training_selector_probe\",\"state\":%d,\"transition\":%d,\"first\":%d,\"second\":%d,\"timer\":%d,\"edge\":%d}\n",word(rdram,selector+0x10),word(rdram,selector+0x14),word(rdram,selector+0x6c),word(rdram,selector+0x70),int(MEM_HU(0,S32(selector+0x9c))),word(rdram,0x80118cb4));
    }
    if(word(rdram,selector+0x10)==1&&word(rdram,selector+0x14)==0)clay::testing::publish_input_phase(both?clay::testing::InputPhase::Loading:
        rival?clay::testing::InputPhase::OpponentSelect:clay::testing::InputPhase::FighterSelect);
    if(old!=rival&&std::getenv("CLAY_TRACE_GAME"))fprintf(stderr,"{\"event\":\"training_selection_stage\",\"opponent\":%s}\n",rival?"true":"false");
}
void run_match(uint8_t* rdram,recomp_context* ctx,recomp_func_t* original){
    if(!clay::main_menu::training_requested()){original(rdram,ctx);return;}
    const auto config=ctx->r4;
    unsigned scene_index=0;
    for(;;){
        {std::lock_guard lock(mutex);repeat_match=false;repeated_scene=scene_index++!=0;}
        if(std::getenv("CLAY_TRACE_GAME"))fprintf(stderr,"{\"event\":\"training_match_enter\",\"worker_exists\":%d}\n",word(rdram,0x800dab68));
        ctx->r4=config;original(rdram,ctx);
        std::lock_guard lock(mutex);
        if(std::getenv("CLAY_TRACE_GAME"))fprintf(stderr,"{\"event\":\"training_match_return\",\"repeat\":%s,\"exit\":%s}\n",repeat_match?"true":"false",exit_match?"true":"false");
        live_world=0;actors={};pause_requested=false;
        if(!repeat_match||exit_match){session.leave();break;}
    }
    if(change_fighters){clay::main_menu::retain_training_request();ctx->r2=2;return;}
    clay::main_menu::clear_training_request();ctx->r2=1;
}
void begin_combat(uint8_t* rdram){
    std::lock_guard lock(mutex);
    if(!session.active()||!clay::main_menu::training_requested())return;
    live_world=word(rdram,0x801028c0);
    actors={uint32_t(word(rdram,0x801028b8)),uint32_t(word(rdram,0x801028bc))};
    if(!valid(live_world,0x2cd0)||!valid(actors[0],0x1100)||!valid(actors[1],0x1100))return;
    maximum_hp={word(rdram,actors[0]+0x2c0),word(rdram,actors[1]+0x2c0)};
    character=word(rdram,0x801028a8);opponent=word(rdram,0x801028b0);
    auto remembered=session.options();remembered.last_character=character;remembered.last_opponent=opponent;session.set_options(remembered);
    if(!storage.empty()){std::string error;save_options(storage,remembered,error);}
    session.begin_combat();
    if(std::getenv("CLAY_TRACE_TRAINING"))fprintf(stderr,"{\"event\":\"training_stage_bounds\",\"left\":%d,\"right\":%d,\"p1x\":%.2f,\"p2x\":%.2f}\n",word(rdram,live_world+0x2c9c),word(rdram,live_world+0x2ca0),scalar(rdram,actors[0]+0x44),scalar(rdram,actors[1]+0x44));
    catalog=std::make_shared<const std::vector<Move>>(read_moves(rdram,actors[0]));
    constexpr uint16_t masks[]={0x8000,0x4000,0x1000,0x2000,0x20,0x10,8,4,2,1,0x800,0x400,0x200,0x100};
    for(unsigned i=0;i<6;i++){
        const auto mask=MEM_HU(0,S32(actors[0]+0x24c+2*i));
        auto found=std::find(std::begin(masks),std::end(masks),mask);
        mapped_attacks[i]=found==std::end(masks)?input::Action::Primary:input::Action(found-std::begin(masks));
    }
    if(pinned>=int(catalog->size()))pinned=-1;
    input_history.clear();previous_input=0;played_finisher=false;position_pending=true;spacing_pending=finish_window_seen=false;pause_requested=false;
    if(session.options().preset==Preset::Claytalities&&std::none_of(catalog->begin(),catalog->end(),[](const Move& move){return move.category==MoveCategory::Claytality;})){
        auto supported=session.options();supported.preset=Preset::Normal;session.set_options(supported);session.begin_combat();
    }
    for(auto actor:actors)if(session.options().super!=Super::Normal)put(rdram,actor+0x2c8,300);
    put(rdram,actors[0]+0x260,0);
    put(rdram,actors[1]+0x260,session.options().preset==Preset::Normal&&session.options().opponent==Opponent::Cpu?1:0);
    // Repeated scenes already own fully loaded actors and stage resources. Enter
    // the native ready phase, which resets both actors' inputs and performs the
    // normal fight announcement before phase 3. The ordinary simulation also
    // initializes the camera through 89708 -> 8EB24 on this first ready tick.
    const int native_phase=word(rdram,live_world+0x50);
    if(repeated_scene&&(native_phase==0||native_phase==1)&&
       word(rdram,actors[0]+0xfb0)==0&&word(rdram,actors[1]+0xfb0)==0&&
       reset_position(rdram,live_world,actors,session.options())){
        put(rdram,live_world+0x64,1); // Native 85704 no-stage-intro readiness flag.
        put(rdram,live_world+0x4c,1); // Native phase 2 must execute its entry work.
        put(rdram,live_world+0x50,2);
        position_pending=false;
        if(std::getenv("CLAY_TRACE_GAME"))fprintf(stderr,"{\"event\":\"training_repeat_ready\"}\n");
    }
    if(std::getenv("CLAY_TRACE_GAME"))fprintf(stderr,"{\"event\":\"training_combat_ready\",\"p1\":%d,\"p2\":%d}\n",character,opponent);
}
int32_t damage_applied(uint8_t* rdram,uint32_t target,unsigned amount,int32_t result,bool chip){
    std::lock_guard lock(mutex);
    if(!session.active()||!live_world||!valid(target,0x1100)||!amount)return result;
    const int index=target==actors[0]?0:target==actors[1]?1:-1;
    if(index<0)return result;
    session.damage(unsigned(index),amount,chip);
    const bool final=word(rdram,target+0x2c4)+1>=word(rdram,live_world+0x30);
    if(final&&session.options().preset==Preset::Normal)result=std::max(1,result);
    if(std::getenv("CLAY_TRACE_GAME"))fprintf(stderr,"{\"event\":\"training_damage\",\"target\":%d,\"damage\":%u,\"protected_hp\":%d}\n",index,amount,result);
    return result;
}
void prepare_input(uint8_t* rdram,recomp_context* ctx){
    std::lock_guard lock(mutex);
    if(!session.active()||uint32_t(ctx->r4)!=live_world||!valid(actors[1],0x1100))return;
    const auto& options=session.options();
    const uint16_t p1=uint16_t(ctx->r5);
    put(rdram,actors[0]+0x260,0);
    if(word(rdram,live_world+0x50)>=3){
        if(input_history.empty()||p1!=previous_input){input_history.insert(input_history.begin(),HistoryEntry{p1,1});if(input_history.size()>12)input_history.pop_back();}
        else if(input_history.front().ticks<0xffffffffu)++input_history.front().ticks;
        if(p1!=previous_input&&std::getenv("CLAY_TRACE_TRAINING"))fprintf(stderr,"{\"event\":\"training_native_input\",\"buttons\":%u,\"gap\":%d,\"p1_state\":%d,\"p2_state\":%d,\"window\":%.2f}\n",unsigned(p1),word(rdram,live_world+0x2cb8),word(rdram,actors[0]+0xfb0),word(rdram,actors[1]+0xfb0),scalar(rdram,actors[1]+0x2b4));
        previous_input=p1;
    }
    if(options.preset==Preset::Claytalities){put(rdram,actors[1]+0x260,0);ctx->r6=0;return;}
    put(rdram,actors[1]+0x260,options.opponent==Opponent::Cpu?1:0);
    if(options.opponent==Opponent::Cpu)return;
    if(options.opponent==Opponent::Player2){if(!second_connected.load())ctx->r6=0;return;}
    uint16_t dummy=0;
    if(options.opponent==Opponent::Crouching)dummy|=0x400;
    if(options.opponent==Opponent::Jumping)dummy|=0x800;
    if(options.guard==Guard::Always||(options.guard==Guard::AfterFirstHit&&session.combo_active()))
        dummy|=scalar(rdram,actors[1]+0x44)>=scalar(rdram,actors[0]+0x44)?0x100:0x200;
    ctx->r6=dummy;
}
void observe_world(uint8_t* rdram,uint32_t world,recomp_context* ctx) {
    if(!valid(world,0x2cd0))return;
    const uint32_t a=word(rdram,world+0x124),b=word(rdram,world+0x128);
    if(!valid(a,0x1100)||!valid(b,0x1100))return;
    {
        std::unique_lock lock(mutex);
        if(session.active()&&world==live_world){
            const int native_phase=word(rdram,world+0x50);
            if(position_pending&&native_phase==3){
                position_pending=!reset_position(rdram,world,actors,session.options());
                if(!position_pending&&std::getenv("CLAY_TRACE_GAME"))fprintf(stderr,"{\"event\":\"training_position_reset\"}\n");
            }
            facing=scalar(rdram,a+0x44)<=scalar(rdram,b+0x44);
            if(session.options().preset==Preset::Normal&&native_phase==3&&session.options().super==Super::Infinite)for(auto actor:actors)put(rdram,actor+0x2c8,300);
            if(native_phase==8)played_finisher=true;
            FrameState frame;
            frame.combat=native_phase>=3;
            frame.paused=word(rdram,0x8010849c)!=0;frame.attract=word(rdram,0x80106d58)!=0;
            clay::testing::publish_input_phase(frame.paused?clay::testing::InputPhase::Pause:
                frame.combat?clay::testing::InputPhase::Combat:clay::testing::InputPhase::Loading);
            auto resting=[&](uint32_t actor){const int action=word(rdram,actor+0xfb0);return action>=0&&action<21&&word(rdram,0x800f247c+action*4)!=0&&scalar(rdram,actor+0x40c)<=0;};
            frame.safe_rest=native_phase==3&&resting(a)&&resting(b)&&
                scalar(rdram,a+0x2b0)<=0&&scalar(rdram,b+0x2b0)<=0&&scalar(rdram,world+0x2cbc)<=0;
            if(frame.safe_rest)session.combo_end();
            frame.finisher_playing=native_phase==8;
            frame.finish_window=word(rdram,b+0xfb0)==0x28&&word(rdram,b+0x2c0)<=0;
            frame.attempt_ended=native_phase==5||native_phase==6||native_phase==7;
            frame.success=played_finisher&&native_phase==7;
            const auto effect=session.tick(frame);
            if(effect.prepare){
                put(rdram,b+0x2c0,1);put(rdram,b+0x2c4,std::max(0,word(rdram,world+0x30)-1));put(rdram,b+0x438,0);
                if(ctx&&session.options().direct_finisher){
                    const auto saved=*ctx;const int32_t old_stack=MEM_W(0x10,ctx->r29);
                    ctx->r4=S32(b);ctx->r5=0x28;ctx->r6=-1;ctx->r7=1;MEM_W(0x10,ctx->r29)=-1;
                    lock.unlock();func_80080880(rdram,ctx);lock.lock();*ctx=saved;MEM_W(0x10,ctx->r29)=old_stack;
                    put(rdram,b+0x2c0,0);put(rdram,b+0x2c4,word(rdram,world+0x30));put(rdram,b+0x438,1);
                    put(rdram,b+0x318,1);put(rdram,b+0x2b0,std::bit_cast<int32_t>(9999.0f));
                    put(rdram,b+0x2b4,std::bit_cast<int32_t>(180.0f));
                }
                spacing_pending=true;
            }
            if(frame.finish_window&&!finish_window_seen){finish_window_seen=true;spacing_pending=true;}
            if(spacing_pending&&session.options().preset==Preset::Claytalities&&session.options().automatic_distance&&catalog&&word(rdram,a+0xfb0)<=1&&(frame.finish_window||effect.prepare)){
                const Move* chosen=nullptr;
                if(pinned>=0&&size_t(pinned)<catalog->size()&&catalog->at(pinned).category==MoveCategory::Claytality)chosen=&catalog->at(pinned);
                if(!chosen)for(const auto& move:*catalog)if(move.category==MoveCategory::Claytality){chosen=&move;break;}
                if(chosen){
                    const float gap=std::isfinite(chosen->maximum_distance)&&chosen->maximum_distance>=chosen->minimum_distance?(chosen->minimum_distance+chosen->maximum_distance)/2:chosen->minimum_distance+20;
                    spacing_pending=!reset_position(rdram,world,actors,session.options(),gap);
                    if(!spacing_pending&&std::getenv("CLAY_TRACE_GAME"))fprintf(stderr,"{\"event\":\"training_finisher_spacing\"}\n");
                }
            }
            if(effect.extend_window)put(rdram,b+0x2b4,std::bit_cast<int32_t>(1800.0f));
            if(session.options().preset==Preset::Claytalities&&(native_phase==6||native_phase==7))
                put(rdram,world+0x2cc8,std::bit_cast<int32_t>(120.0f));
            if(effect.recover){
                for(unsigned i=0;i<2;i++){
                    put(rdram,actors[i]+0x2c0,maximum_hp[i]);
                    put(rdram,actors[i]+0x2c4,0);put(rdram,actors[i]+0x438,0);
                }
                if(std::getenv("CLAY_TRACE_GAME"))fprintf(stderr,"{\"event\":\"training_recovered\"}\n");
            }
            if(effect.reset||exit_match){
                if(!word(rdram,world+0x54)&&std::getenv("CLAY_TRACE_GAME"))fprintf(stderr,"{\"event\":\"training_scene_shutdown\",\"repeat\":%s,\"phase\":%d}\n",exit_match?"false":"true",native_phase);
                repeat_match=!exit_match;put(rdram,world+0x54,1);
            }
            if(native_phase==3&&!frame.paused){
                // Freeze ordinary time before its verified native subtraction.
                put(rdram,world+0x34,std::bit_cast<int32_t>(99.0f));
                if(session.options().preset==Preset::Claytalities||session.options().opponent!=Opponent::Cpu)put(rdram,b+0x260,0);
            }
        }
    }
    if(!std::getenv("CLAY_TRACE_TRAINING"))return;
    const std::array<int32_t,9> state={word(rdram,world+0x50),
        word(rdram,a+0x2c0),word(rdram,a+0x2c4),word(rdram,a+0xfb0),word(rdram,a+0x318),
        word(rdram,b+0x2c0),word(rdram,b+0x2c4),word(rdram,b+0xfb0),word(rdram,b+0x318)};
    static thread_local std::array<int32_t,9> previous{};
    static thread_local uint64_t tick=0;
    ++tick;
    if(previous==state&&tick%60)return;
    previous=state;
    fprintf(stderr,"{\"event\":\"training_state_probe\",\"tick\":%llu,\"world\":%u,\"phase\":%d,\"timer\":%.4f,\"dt\":%.4f,\"final_reserve\":%d,\"pause\":%d,\"attract\":%d,\"p1\":%u,\"p2\":%u,\"p1_hp\":%d,\"p1_depleted\":%d,\"p1_state\":%d,\"p1_finish\":%d,\"p2_hp\":%d,\"p2_depleted\":%d,\"p2_state\":%d,\"p2_finish\":%d,\"p1_control\":%d,\"p2_control\":%d}\n",
        (unsigned long long)tick,world,state[0],std::bit_cast<float>(word(rdram,world+0x34)),
        std::bit_cast<float>(word(rdram,world+0x2c8c)),word(rdram,world+0x30),
        word(rdram,0x8010849c),word(rdram,0x80106d58),a,b,
        state[1],state[2],state[3],state[4],state[5],state[6],state[7],state[8],
        word(rdram,a+0x260),word(rdram,b+0x260));
}
}
