// Actual original arithmetic/state boundary fixtures, djultra64.
#include "recomp.h"
#include "training_guest.h"
#include <cassert>
#include "main_menu.h"
#include "move_list.h"
#include "input_hud.h"
#include "native_prompts.h"
namespace clay::training {std::vector<Move> read_moves(uint8_t*,uint32_t){return {};}}
namespace clay::input {std::string binding_marker(Action action){return "{action:"+std::to_string(int(action))+"}";}NativePromptSnapshot native_prompt_snapshot(){return {};}const char* action_name(Action){return "fixture";}}
extern "C" void func_80080880(uint8_t*,recomp_context*){assert(false);}
extern "C" void func_800636E4(uint8_t*,recomp_context*){assert(false);}
extern "C" void func_800790F0(uint8_t*,recomp_context*){}
#include <cassert>
#include <bit>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
extern "C" {
void training_original_damage(uint8_t*, recomp_context*);
void training_original_timer(uint8_t*, recomp_context*);
void training_original_reserve(uint8_t*, recomp_context*);
void training_original_depletion(uint8_t*, recomp_context*);
void training_original_ready(uint8_t*, recomp_context*);
}
static void selection_fixture(uint8_t*,recomp_context* ctx){
    assert(ctx->r4==0&&ctx->r5==0&&ctx->r6==0);
    assert(clay::training::selecting_fighters());
    assert(!clay::training::take_pause_request()); // Selection discards pending combat pause.
    clay::training::request_pause();
    assert(!clay::training::take_pause_request()); // Logical pause is unavailable in selection.
    ctx->r2=0;
}
int main(int argc,char** argv) {
    assert(argc>=2);
    std::vector<uint8_t> ram(0x900000);
    auto* rdram=ram.data();
    std::ifstream f(argv[1],std::ios::binary);
    std::vector<uint8_t> rom((std::istreambuf_iterator<char>(f)),{});
    assert(rom.size()==0xc00000);
    for(unsigned i=0x1000;i<0xf9980;++i)rdram[((i-0x1000)+0x400)^3]=rom[i];
    constexpr int32_t actor=S32(0x80100000), world=S32(0x80200000),
                      other=S32(0x80110000), stack=S32(0x807ff000);
    auto context=[&] {recomp_context c{}; c.r29=stack; c.f_odd=&c.f0.u32h;return c;};
    assert(!clay::training::snapshot().active);
    clay::training::request_pause();assert(!clay::training::take_pause_request());
    auto damage=[&](int hp,int value,bool lethal) {
        MEM_W(0x2c0,actor)=hp;
        MEM_W(0x88,stack)=actor;
        MEM_W(0x98,stack)=value;
        MEM_W(0x8c,stack)=lethal;
        auto c=context();c.f_odd=&c.f0.u32h;training_original_damage(rdram,&c);
        return MEM_W(0x2c0,actor);
    };
    assert(damage(170,17,true)==153);
    assert(damage(5,20,true)==0);
    assert(damage(5,20,false)==1); // Native nonlethal damage floor, before KO logic.
    auto timer=[&](int phase,int hp,float value,float dt) {
        MEM_W(0x50,world)=phase;MEM_W(0x38,world)=0;
        MEM_W(0x68,stack)=world;
        MEM_W(0x124,world)=actor;MEM_W(0x128,world)=other;
        MEM_W(0x2c0,actor)=hp;MEM_W(0x2c0,other)=170;
        MEM_W(0x34,world)=std::bit_cast<uint32_t>(value);
        MEM_W(0x2c8c,world)=std::bit_cast<uint32_t>(dt);
        auto c=context();c.f_odd=&c.f0.u32h;c.r10=world;
        training_original_timer(rdram,&c);
        return std::bit_cast<float>(uint32_t(MEM_W(0x34,world)));
    };
    assert(timer(2,170,99,1)==99); // Intro is not active combat.
    assert(timer(3,0,99,1)==99);   // Defeat freezes ordinary round timer.
    const float decremented=timer(3,170,99,1);
    assert(decremented<99&&decremented>98); // Native seconds conversion.
    auto reserve=[&](int depleted) {
        MEM_W(0x2c0,actor)=0;MEM_W(0x2c4,actor)=depleted;
        MEM_W(0x2a8,actor)=world;MEM_W(0x30,world)=2;
        MEM_W(0x2b8,actor)=123;MEM_W(0x40,stack)=actor;
        auto c=context();c.f_odd=&c.f0.u32h;c.r13=actor;
        training_original_reserve(rdram,&c);
        return MEM_W(0x2c0,actor);
    };
    assert(reserve(1)==170&&MEM_W(0x2b8,actor)==0);
    assert(reserve(2)==0); // Exhausted final reserve must not revive in original modes.
    MEM_W(0x88,stack)=actor;MEM_W(0x438,actor)=0;
    {auto c=context();c.f_odd=&c.f0.u32h;training_original_depletion(rdram,&c);}
    assert(MEM_W(0x2c4,actor)==3&&MEM_W(0x438,actor)==1);
    {auto c=context();c.f_odd=&c.f0.u32h;training_original_depletion(rdram,&c);}
    assert(MEM_W(0x2c4,actor)==3); // A single KO cannot count twice.
    if(argc>2){
        constexpr int32_t menu=S32(0x80300000);
        MEM_W(0,S32(0x800ddd3c))=menu;
        clay::main_menu::prepare(rdram);
        clay::main_menu::observe_input(rdram); // Native idle/release observation precedes confirmed dispatch.
        MEM_W(0x18,menu)=2;MEM_W(0,S32(0x80118d74))=0x8000;
        assert(!clay::main_menu::confirm_exit(rdram)&&clay::main_menu::training_requested());
        auto selected=context();selected.f_odd=&selected.f0.u32h;
        clay::training::select_fighters(rdram,&selected,&selection_fixture);
        MEM_W(0,S32(0x801028b8))=other;MEM_W(0,S32(0x801028bc))=actor;
        MEM_W(0,S32(0x801028c0))=world;
        MEM_W(0x2c0,actor)=170;MEM_W(0x2c0,other)=170;
        MEM_W(0x2c4,actor)=1;MEM_W(0x438,actor)=0;MEM_W(0x30,world)=2;
        clay::training::begin_combat(rdram);
        if(damage(1,100,true)!=1){puts("RED original damage ends the final training reserve");return 14;}
        auto hit=context();hit.f_odd=&hit.f0.u32h;
        training_original_depletion(rdram,&hit);
        assert(MEM_W(0x2c4,actor)==1&&MEM_W(0x438,actor)==0);
        const auto view=clay::training::snapshot();
        assert(view.damage.last==100&&view.damage.combo==100&&view.damage.hits==1);
        puts("PASS native Training final-reserve KO protection and unclamped damage telemetry");

        // Input history consumes delivered simulation inputs, including held duration,
        // release/neutral and simultaneous actions, independently of render snapshots.
        auto feed=[&](uint16_t buttons,unsigned ticks=1,uint32_t input_world=0,uint16_t rival=0x8000){
            if(!input_world)input_world=uint32_t(world);
            uint16_t delivered=0;
            for(unsigned n=0;n<ticks;n++){
                auto input=context();input.f_odd=&input.f0.u32h;
                input.r4=gpr(S32(input_world));input.r5=buttons;input.r6=rival;
                clay::training::prepare_input(rdram,&input);
                assert(input.r5==buttons);delivered=uint16_t(input.r6);
            }
            return delivered;
        };
        assert(clay::training::snapshot().history.empty());
        MEM_W(0x50,world)=2;feed(0x8800,5);
        assert(clay::training::snapshot().history.empty()); // Intro is not combat input.
        MEM_W(0x50,world)=3;feed(0,3);
        assert(clay::training::snapshot().history==std::vector<std::string>{"Neutral 3 ticks"});
        feed(0x8800,4);
        auto history=clay::training::snapshot().history;
        assert(history.size()==2&&history[0]=="{action:0} and {action:10} 4 ticks"&&history[1]=="Neutral 3 ticks");
        feed(0,2);history=clay::training::snapshot().history;
        assert(history.size()==3&&history[0]=="Neutral 2 ticks"&&history[2]=="Neutral 3 ticks");
        const auto observed=history;
        assert(feed(0x4000,9,uint32_t(world)+4,0x1000)==0x1000);
        assert(clay::training::snapshot().history==observed); // A different world's input has no effect.

        // Distinct user inputs cover all fourteen N64 actions; only twelve entries fit.
        constexpr uint16_t inputs[]{0x8000,0x4000,0x1000,0x2000,0x20,0x10,8,4,2,1,0x800,0x400,0x200,0x100};
        for(uint16_t buttons:inputs)feed(buttons);
        history=clay::training::snapshot().history;
        assert(history.size()==12&&history.front()=="{action:13} 1 ticks"&&history.back()=="{action:2} 1 ticks");
        assert(history[6]=="{action:7} 1 ticks"&&history[11]=="{action:2} 1 ticks");

        auto options=clay::training::snapshot().options;options.preset=clay::training::Preset::Claytalities;
        clay::training::configure(options);
        assert(feed(0x10,6,uint32_t(world),0xffff)==0); // Clay dummy receives no physical P2 input.
        history=clay::training::snapshot().history;
        assert(history.size()==12&&history.front()=="{action:5} 6 ticks"); // Recorded before Clay's early return.
        feed(0,2);assert(clay::training::snapshot().history.front()=="Neutral 2 ticks");

        // Pause is a one-shot request during combat and cannot escape into selection.
        clay::training::request_pause();assert(clay::training::take_pause_request());
        assert(!clay::training::take_pause_request());
        clay::training::request_pause(); // Deliberately pending when selection begins.
        options.preset=clay::training::Preset::Normal;clay::training::configure(options);
        selected=context();selected.f_odd=&selected.f0.u32h;
        clay::training::select_fighters(rdram,&selected,&selection_fixture);
        assert(!clay::training::selecting_fighters()&&!clay::training::selecting_opponent());
        clay::training::request_pause();assert(!clay::training::take_pause_request()); // No live combat world yet.
        clay::training::begin_combat(rdram);
        assert(clay::training::snapshot().history.empty()); // A new combat clears prior commands/durations.
        feed(0,1);assert(clay::training::snapshot().history==std::vector<std::string>{"Neutral 1 ticks"});

        clay::training::request_pause(); // Pending request must also be retired on leaving Training.
        auto match=context();match.f_odd=&match.f0.u32h;
        // Execute the actual repeated-scene entry against a valid native wall
        // polygon, then execute phase 2's original actor readiness routine.
        auto completed=[](uint8_t* rdram,recomp_context* native){
            constexpr int32_t w=S32(0x80200000),p1=S32(0x80110000),p2=S32(0x80100000),bank=S32(0x80400000);
            static unsigned cycle=0;
            MEM_W(0x124,w)=p1;MEM_W(0x128,w)=p2;
            MEM_W(0x2a8,p1)=w;MEM_W(0x2a8,p2)=w;
            MEM_W(0xfb0,p1)=0;MEM_W(0xfb0,p2)=0;
            MEM_W(0x50,w)=0;MEM_W(0x4c,w)=-1;MEM_W(0x64,w)=0;
            MEM_W(0,w)=std::bit_cast<int32_t>(1.0f);MEM_W(8,w)=0;MEM_W(0xc,w)=0;MEM_W(0x14,w)=0;
            MEM_W(0x12c,w)=bank;MEM_W(0x134,w)=bank+0x48;
            MEM_W(4,bank)=0x200;MEM_W(0x64,bank)=4;MEM_W(0x68,bank)=0x8c;
            const float walls[]{-500,-500,500,-500,500,-500,500,500,500,500,-500,500,-500,500,-500,-500};
            for(unsigned i=0;i<4;i++)for(unsigned n=0;n<4;n++)MEM_W(0,gpr(S32(uint32_t(bank)+0x8c+i*36+n*4)))=std::bit_cast<int32_t>(walls[i*4+n]);
            clay::training::begin_combat(rdram);
            if(cycle++==0){
                assert(MEM_W(0x50,w)==0&&MEM_W(0x4c,w)==-1&&MEM_W(0x64,w)==0); // First load retains intro.
                MEM_W(0x50,w)=3;clay::training::request_restart();
                clay::training::observe_world(rdram,uint32_t(w),native);
                assert(MEM_W(0x54,w)==1); // Original scene exits through native cleanup.
            }else{
                assert(cycle==2&&MEM_W(0x50,w)==2&&MEM_W(0x4c,w)==1&&MEM_W(0x64,w)==1);
                assert(MEM_W(0xfb0,p1)==0&&MEM_W(0xfb0,p2)==0);
                assert(std::bit_cast<float>(uint32_t(MEM_W(0x44,p2)))-std::bit_cast<float>(uint32_t(MEM_W(0x44,p1)))==100);
                MEM_W(0x34,p1)=MEM_W(0x38,p1)=123;MEM_W(0x34,p2)=MEM_W(0x38,p2)=456;
                MEM_W(0x68,native->r29)=w;
                training_original_ready(rdram,native);
                assert(MEM_W(0x34,p1)==0&&MEM_W(0x38,p1)==0&&MEM_W(0x34,p2)==0&&MEM_W(0x38,p2)==0);
                assert(MEM_W(0x50,w)==2); // Native worker owns announcement and transition to combat.
            }
            native->r2=1;
        };
        clay::training::run_match(rdram,&match,completed);
        assert(!clay::training::snapshot().active&&!clay::main_menu::training_requested());
        assert(!clay::training::take_pause_request());
        clay::training::request_pause();assert(!clay::training::take_pause_request());
        auto ordinary=context();ordinary.f_odd=&ordinary.f0.u32h;ordinary.r4=77;ordinary.r5=88;ordinary.r6=99;
        auto passthrough=[](uint8_t*,recomp_context* native){
            assert(!clay::training::selecting_fighters());native->r2=23;
        };
        clay::training::select_fighters(rdram,&ordinary,passthrough);
        assert(ordinary.r2==23&&ordinary.r4==77&&ordinary.r5==88&&ordinary.r6==99);
        assert(!clay::training::snapshot().active);
        puts("PASS delivered input history: 12 entries, simultaneous actions, neutral/held ticks, Clay inputs and combat reset");
        puts("PASS logical pause: inactive gating, one-shot consumption, selection/exit cleanup and ordinary selector isolation");
        puts("PASS repeated Training scene skips entrance with native phase-2 input readiness; first-load intro retained");
    }
    puts("PASS native damage/floor, active-phase timer and depleted-reserve boundary");
}
