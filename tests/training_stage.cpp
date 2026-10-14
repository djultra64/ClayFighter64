// Execute the original stage input handler with Training hooks. djultra64.
#include "training_stage.h"
#include "training_menu_ui.h"
namespace clay::training {void draw_training_lines(uint8_t*,recomp_context*,const std::vector<std::string>&,int,int,unsigned,uint32_t,uint32_t,int,int,bool){}}
namespace clay::input {std::string menu_marker(Action){return {};}}
#include "main_menu.h"
#include "test_input.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <vector>
namespace {
bool training_enabled=true;
unsigned polls=0;
uint32_t next_nav=0,next_held=0;
constexpr int32_t game=S32(0x80300000),stage=S32(0x80301000),options=S32(0x80302000);
}
namespace clay::main_menu {bool training_requested(){return ::training_enabled;}}
extern "C" void training_original_stage_input(uint8_t*,recomp_context*);
extern "C" void func_800A0CA8(uint8_t* rdram,recomp_context*){
 ++polls;
 MEM_W(0,S32(0x80118d74))=next_nav;MEM_W(0,S32(0x80118d84))=0;
 MEM_W(0,S32(0x80118cb0))=next_held;
}
int main(int argc,char** argv){
 assert(argc==2);const std::filesystem::path storage(argv[1]);
 std::vector<uint8_t> ram(0x900000);auto* rdram=ram.data();
 MEM_W(0,S32(0x800ddd40))=game;MEM_W(0,S32(0x800ddd48))=stage;MEM_W(0,S32(0x800ddd58))=options;
 MEM_W(0xc,game)=0;MEM_W(0x10,game)=0x80;MEM_H(0x28,options)=0;
 {std::ofstream f(storage);f<<"ClayFighter64TrainingStage 1\nstage 4\n";}
 clay::training::initialize_stage_storage(storage);
 MEM_W(8,game)=12;clay::training::stage_panel_started(rdram);assert(MEM_W(8,game)==4);
 auto input=[&](uint32_t nav,uint32_t held=0){next_nav=nav;next_held=held;recomp_context c{};c.r29=S32(0x807ff000);c.f_odd=&c.f0.u32h;clay::training::stage_input(rdram,&c,&training_original_stage_input);assert(MEM_HU(0x28,options)==0);};
 clay::testing::publish_input_phase(clay::testing::InputPhase::Loading);
 MEM_W(0,stage)=100;clay::training::stage_panel_tick(rdram);assert(clay::testing::observed_input_phase.load()==clay::testing::InputPhase::Loading);
 input(0x8000,0x8000);assert(MEM_W(0,stage)==100); // Intro/held selector confirm cannot accept.
 MEM_W(0,stage)=101;clay::training::stage_panel_tick(rdram);assert(clay::testing::observed_input_phase.load()==clay::testing::InputPhase::StageSelect);
 input(0x8000,0x8000);assert(MEM_W(0,stage)==101); // Require release after selection.
 input(0);
 input(0x100);assert(MEM_W(8,game)==5); // Logical right reaches native C-right branch despite CPU flag.
 input(0x200);assert(MEM_W(8,game)==4);
 MEM_W(8,game)=0;input(0x200);assert(MEM_W(8,game)==25);input(0x100);assert(MEM_W(8,game)==0);
 for(int i=0;i<2000;i++){MEM_W(0,stage)=299;clay::training::stage_panel_tick(rdram);assert(MEM_W(0,stage)==101);}
 input(0x8000|0x100);assert(MEM_W(0,stage)==299&&MEM_W(8,game)==0); // Confirm wins over simultaneous navigation.
 MEM_W(0,stage)=300;clay::training::stage_panel_tick(rdram);assert(MEM_W(0,stage)==300);
 MEM_W(0,S32(0x800ddd48))=0; // Original 6D04C cleanup clears this pointer.
 clay::training::stage_panel_finished(rdram);assert(!clay::training::take_stage_back());
 MEM_W(0,S32(0x800ddd48))=stage;
 MEM_W(8,game)=8;clay::training::initialize_stage_storage(storage);clay::training::stage_panel_started(rdram);assert(MEM_W(8,game)==0);
 MEM_W(0,stage)=101;MEM_W(8,stage)=0;input(0);input(0x4000);assert(MEM_W(8,stage)==1);
 clay::training::stage_panel_finished(rdram);assert(clay::training::take_stage_back());assert(!clay::training::take_stage_back());
 // Cancellation does not overwrite the last confirmed stage.
 MEM_W(8,game)=7;clay::training::initialize_stage_storage(storage);clay::training::stage_panel_started(rdram);assert(MEM_W(8,game)==0);
 clay::training::stage_panel_finished(rdram);
 {std::ofstream f(storage);f<<"ClayFighter64TrainingStage 1\nstage 999\n";}
 clay::training::initialize_stage_storage(storage);MEM_W(8,game)=9;clay::training::stage_panel_started(rdram);assert(MEM_W(8,game)==9);
 clay::training::stage_panel_finished(rdram);
 training_enabled=false;MEM_W(8,game)=8;MEM_W(0,stage)=250;const auto observed=clay::testing::observed_input_phase.load();
 clay::training::stage_panel_started(rdram);clay::training::stage_panel_tick(rdram);assert(MEM_W(0,stage)==250&&clay::testing::observed_input_phase.load()==observed);
 input(0x100);assert(MEM_W(8,game)==8); // Original non-Training D-pad remains untouched.
 std::filesystem::remove(storage);puts("PASS native Training stage readiness/release, D-pad wrapping, finite confirm, cancel, persistence and mode isolation");
}
