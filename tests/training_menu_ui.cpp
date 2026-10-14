// Isolated native training menu contracts. djultra64.
#include "training_menu_ui.h"
#include "native_prompts.h"
#include "test_input.h"
#include "ui_background.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <bit>
using namespace clay::training;
namespace {
Snapshot live;
bool pause_request=false;
unsigned restarts=0,changes=0,exits=0,draws=0;
bool text_pipeline=false,orthographic=false;
unsigned backgrounds=0;
float glyph_scale_y=1,model_y=0;
clay::input::Action bound=clay::input::Action::Count;
std::string caption(uint8_t* rdram,uint32_t p){std::string s;for(unsigned i=0;i<512;i++){char c=MEM_BU(0,S32(p+i));if(!c)return s;s+=c;}assert(false);return {};}
}
namespace clay::training {
Snapshot snapshot(){return live;}
void configure(const Options& o){live.options=o;}
void request_restart(){++restarts;}
void request_exit(){++exits;}
void request_change_fighters(){++changes;}
void pin_move(int i){live.pinned_move=i;}
bool take_pause_request(){bool p=pause_request;pause_request=false;return p;}
const char* character_name(int){return "Test Fighter";}
const char* category_name(MoveCategory c){return c==MoveCategory::Claytality?"Claytality":"Basic";}
std::string move_command(const Move&,const std::array<clay::input::Action,6>& a,bool right){return std::string(right?"forward":"back")+" attack="+std::to_string(unsigned(a[0]));}
}
namespace clay::input {
bool binding_capture_active(){return false;}
std::string binding_capture_status(){return "Done";}
void request_binding(Action a){bound=a;}
std::string binding_marker(Action a){return "action="+std::to_string(unsigned(a));}
std::string menu_marker(Action a){return "menu="+std::to_string(unsigned(a));}
std::string native_prompt_text(const std::string& s){return s;}
void prepare_native_prompt_measure(uint8_t* rdram,recomp_context* c){
    const auto p=uint32_t(c->r5);auto s=caption(rdram,p);std::string result;
    for(unsigned i=0;i<s.size();i++){if(s[i]=='\x1e'&&i+2<s.size()&&s[i+2]=='\x1f'){result+='%';i+=2;}else result+=s[i];}
    for(unsigned i=0;i<=result.size();i++)MEM_BU(0,S32(p+i))=i<result.size()?uint8_t(result[i]):0;
}
void update_combat_input(uint8_t*){}
}
namespace clay::ui {void draw_fullscreen_fill(uint8_t* rdram,recomp_context* c,recomp_func_t* original){original(rdram,c);}}
extern "C" {
void func_800B0270(uint8_t*,recomp_context*){}
void func_800B0328(uint8_t*,recomp_context* c){glyph_scale_y=std::bit_cast<float>(uint32_t(c->r6));}
void func_800B09DC(uint8_t*,recomp_context* c){model_y=std::bit_cast<float>(uint32_t(c->r6))*glyph_scale_y;}
void func_800A45BC(uint8_t*,recomp_context*){text_pipeline=true;}
void func_800A3F88(uint8_t* rdram,recomp_context*){MEM_W(0,S32(0x80110898))+=32*8;}
void func_800A4D9C(uint8_t* rdram,recomp_context* c){
    const auto rectangle=uint32_t(c->r5);assert(rectangle<0x80800000u);
    assert(MEM_W(0,S32(rectangle))==0&&MEM_W(0,S32(rectangle+4))==0&&MEM_W(0,S32(rectangle+8))==319&&MEM_W(0,S32(rectangle+12))==239);
    assert(c->r6==0&&c->r7==0&&MEM_W(0x10,c->r29)==0&&MEM_W(0x14,c->r29)==80);++backgrounds;
    MEM_W(0,S32(0x80110898))+=32*8;c->r4=101;
}
void func_800A561C(uint8_t* rdram,recomp_context*){assert(text_pipeline);orthographic=true;MEM_W(0,S32(0x8011089c))+=128;}
void func_800A1B98(uint8_t* rdram,recomp_context*){assert(orthographic);MEM_W(0,S32(0x80110898))+=8;MEM_W(0,S32(0x8011089c))+=64;assert(uint32_t(MEM_W(0,S32(0x8011089c)))<=0x80300000+128*64);}
void func_80071738(uint8_t* rdram,recomp_context* c){c->r2=caption(rdram,uint32_t(c->r5)).size()*8;}
void func_8007191C(uint8_t* rdram,recomp_context* c){
    auto p=uint32_t(MEM_W(0x18,c->r29));auto s=caption(rdram,p);++draws;
    for(unsigned char ch:s)assert(std::isalnum(ch)||ch=='%'); // Explicit resolvedicon or realfontletters/digits.
    // Conservative budget used by production renderer reserves 32 commands per
    // byte. Simulate that maximum and verify no overrun even at capacity edges.
    MEM_W(0,S32(0x80110898))+=int(s.size()*31*8);
    assert(uint32_t(MEM_W(0,S32(0x80110898)))<=0x80200000+4096*8);
    assert(MEM_W(0,S32(uint32_t(c->r7)))==int32_t(0xc0000000)||MEM_W(0,S32(uint32_t(c->r7)))<0);
    const auto font=uint32_t(c->r4);
    const auto vertical=uint32_t(MEM_W(0x10,c->r29));
    const auto ratio=std::bit_cast<float>(uint32_t(MEM_W(0,S32(vertical+4))));
    const unsigned actual_height=s=="%"||font==0x80084000u?15:29;
    assert(glyph_scale_y*ratio*actual_height<=12.01f); // Actual glyph fits its row.
    if(s=="%")assert(std::abs(glyph_scale_y*ratio*15-8.f)<0.01f); //15pxicon matches8pxHUD lettering.
    assert(std::abs(model_y)<=120);
    if(draws==1)assert(std::abs(model_y-50.f)<0.01f); // Native B09DC composes translation BEFOREscale.
    assert(std::abs(int(MEM_H(0,S32(font+0x10))))<=160);
    assert(std::abs(int(MEM_H(0,S32(font+0x12))))<=120);
    for(unsigned i=0;i<6;i++)MEM_H(0,S32(font+0x10+i*2))=123;
    c->r4=99;c->r29-=100; // caller must isolate all register mutations
}
}
int main(){
    TrainingMenu menu;menu.reset();Snapshot s;s.character=0;
    assert(menu.view(s).rows.size()==5);
    menu.move(-1,s);assert(menu.row()==4);menu.move(1,s);assert(menu.row()==0);
    menu.move(1,s);assert(menu.activate(s)==MenuIntent::None);assert(menu.page()==MenuPage::Options);
    for(unsigned i=0;i<6;i++)menu.move(1,s);
    assert(menu.view(s).first==6&&menu.view(s).rows.size()==8);
    menu.activate(s);assert(menu.page()==MenuPage::Claytality);
    menu.activate(s);assert(s.pinned_move==-1&&menu.view(s).status.find("verified")!=std::string::npos);
    menu.back();menu.move(1,s);menu.activate(s); // preset
    menu.activate(s);assert(s.options.preset==Preset::Normal);
    auto moves=std::make_shared<std::vector<Move>>(19);
    for(unsigned i=0;i<moves->size();i++)moves->at(i).name="Move "+std::to_string(i);
    moves->back().category=MoveCategory::Claytality;s.moves=moves;
    menu.activate(s);assert(s.options.preset==Preset::Claytalities);
    menu.back();menu.activate(s);assert(menu.page()==MenuPage::Moves);
    for(unsigned i=0;i<18;i++)menu.move(1,s);
    assert(menu.view(s).first==18&&menu.activate(s)==MenuIntent::Pin&&s.pinned_move==18);
    s.character=1;assert(menu.view(s).selected==0); // no cursor leakage across fighters
    menu.back();for(unsigned i=0;i<3;i++)menu.move(1,s);menu.activate(s); // resources
    s.options.preset=Preset::Normal;assert(s.options.recovery_ticks==180);
    menu.activate(s);assert(s.options.recovery_ticks==300);menu.activate(s);assert(s.options.recovery_ticks==0);
    menu.activate(s);assert(s.options.recovery_ticks==60);menu.activate(s,-1);assert(s.options.recovery_ticks==0);
    menu.back();for(unsigned i=0;i<7;i++)menu.move(1,s);menu.activate(s);
    assert(menu.activate(s)==MenuIntent::BindReset);menu.move(1,s);assert(menu.activate(s)==MenuIntent::BindPause);
    TrainingMenu opponent_menu;opponent_menu.reset();opponent_menu.move(1,s);opponent_menu.activate(s);opponent_menu.move(1,s);opponent_menu.move(1,s);opponent_menu.activate(s);
    s.options.opponent=Opponent::Player2;s.second_player_connected=false;
    assert(opponent_menu.view(s).status.find("disconnected")!=std::string::npos);
    s.second_player_connected=true;assert(opponent_menu.view(s).status.empty()&&s.options.opponent==Opponent::Player2);
    opponent_menu.move(1,s); // Guard row.
    for(auto behavior:{Opponent::Cpu,Opponent::Player2}){
        s.options.opponent=behavior;s.options.guard=Guard::Always;
        const auto guard_before=s.options.guard;opponent_menu.activate(s);
        assert(s.options.guard==guard_before);
        assert(opponent_menu.view(s).rows[1].find("controlled by")!=std::string::npos);
        assert(opponent_menu.view(s).status.find("controls guard")!=std::string::npos);
    }
    s.options.opponent=Opponent::Cpu;s.options.guard=Guard::AfterFirstHit;s.options.preset=Preset::Claytalities;
    auto forced=opponent_menu.view(s);assert(forced.rows[0].find("Standing")!=std::string::npos&&forced.rows[1].find("Off")!=std::string::npos);
    opponent_menu.activate(s);assert(s.options.guard==Guard::AfterFirstHit);
    assert(opponent_menu.view(s).status.find("Normal preset")!=std::string::npos);
    opponent_menu.move(-1,s);opponent_menu.activate(s);
    assert(s.options.opponent==Opponent::Cpu&&s.options.guard==Guard::AfterFirstHit);
    s.options.preset=Preset::Normal;
    assert(opponent_menu.view(s).rows[0].find("CPU")!=std::string::npos);
    // Pause resets presentation state, but the selected verified finisher lives
    // in the session's pinned move. Reopening must show and cycle from that one.
    auto finishers=std::make_shared<std::vector<Move>>(4);
    finishers->at(0).name="Basic";finishers->at(1).name="First finisher";finishers->at(2).name="Another basic";finishers->at(3).name="Second finisher";
    finishers->at(1).category=finishers->at(3).category=MoveCategory::Claytality;
    Snapshot pinned=s;pinned.moves=finishers;pinned.pinned_move=3;
    TrainingMenu reopened;reopened.reset();reopened.move(1,pinned);reopened.activate(pinned);
    for(unsigned i=0;i<6;i++)reopened.move(1,pinned);
    reopened.activate(pinned);
    assert(reopened.view(pinned).rows[0].find("Second finisher")!=std::string::npos);
    assert(reopened.activate(pinned)==MenuIntent::Pin&&pinned.pinned_move==1);
    pinned.pinned_move=3;reopened.reset();reopened.move(1,pinned);reopened.activate(pinned);
    for(unsigned i=0;i<6;i++)reopened.move(1,pinned);
    reopened.activate(pinned);
    // No intervening render/view: input still starts from the persisted second.
    assert(reopened.activate(pinned,-1)==MenuIntent::Pin&&pinned.pinned_move==1);

    std::vector<uint8_t> memory(9*1024*1024);auto rdram=memory.data();recomp_context ctx{};ctx.r4=77;ctx.r29=S32(0x807ff000);ctx.f_odd=&ctx.f0.u32h;
    const auto original=ctx;
    MEM_W(0,S32(0x801028d0))=S32(0x80080000);MEM_W(0,S32(0x80110880))=4096;
    MEM_W(0,S32(0x8010cfd0))=320;MEM_W(0,S32(0x8010cfd4))=240;
    MEM_W(0,S32(0x80080004))=S32(0x80082000);MEM_W(0,S32(0x80082000+('A'-33)*4))=S32(0x80083000);
    MEM_H(0,S32(0x80083006))=32;MEM_H(0,S32(0x80083008))=29;
    MEM_W(0,S32(0x80082000+('%'-33)*4))=S32(0x80083100);MEM_H(0,S32(0x80083106))=15;MEM_H(0,S32(0x80083108))=15;
    MEM_W(0,S32(0x801108dc))=S32(0x80081000);MEM_W(0,S32(0x80081000))=S32(0x80200000);MEM_W(0,S32(0x80110898))=S32(0x80200000);
    MEM_W(0,S32(0x80110888))=128;MEM_W(0,S32(0x801108e0))=S32(0x80081010);MEM_W(0,S32(0x80081010))=S32(0x80300000);MEM_W(0,S32(0x8011089c))=S32(0x80300000);
    for(unsigned i=0;i<6;i++)MEM_H(0,S32(0x80080010+i*2))=i+20;
    draw_training_lines(rdram,&ctx,{"A short line","Next line"},100,24,560,0x80850000);
    assert(draws>2&&!std::memcmp(&ctx,&original,sizeof(ctx)));
    for(unsigned i=0;i<6;i++)assert(MEM_HU(0,S32(0x80080010+i*2))==i+20);
    draw_training_lines(rdram,&ctx,{std::string("A\x1e")+"A\x1f"},100,24,560,0x80850000);
    draw_training_lines(rdram,&ctx,{"Move: A+B / C & D (1-6)"},100,24,560,0x80850000);
    const uint32_t stage_font=0x80084000,stage_map=0x80084100,stage_glyph=0x80085000;
    MEM_W(0,S32(stage_font+4))=S32(stage_map);MEM_W(0,S32(stage_map+('A'-33)*4))=S32(stage_glyph);
    MEM_H(0,S32(stage_glyph+6))=9;MEM_H(0,S32(stage_glyph+8))=15;
    for(unsigned i=0;i<6;i++)MEM_H(0,S32(stage_font+0x10+i*2))=i+80;
    draw_training_lines(rdram,&ctx,{"A"},-214,32,560,0x80870000,stage_font);
    assert(std::abs(glyph_scale_y*15-8.f)<0.01f&&std::abs(model_y+107.f)<0.01f);
    assert(uint32_t(MEM_W(0,S32(0x801028d0)))==0x80080000u&&!std::memcmp(&ctx,&original,sizeof(ctx)));
    for(unsigned i=0;i<6;i++)assert(MEM_HU(0,S32(stage_font+0x10+i*2))==i+80);
    const auto below_stack=uint32_t(ctx.r29)-0x80;
    for(unsigned i=0;i<4;i++)MEM_W(0,S32(below_stack+i*4))=i+100;
    draw_training_pause_background(rdram,&ctx);assert(backgrounds==1&&!std::memcmp(&ctx,&original,sizeof(ctx)));
    for(unsigned i=0;i<4;i++)assert(MEM_W(0,S32(below_stack+i*4))==int(i+100));
    MEM_W(0,S32(0x80110898))=S32(0x80200000+4096*8-200*8);
    const auto prior_draws=draws;
    draw_training_lines(rdram,&ctx,{"No capacity"},100,24,560,0x80850000);assert(draws==prior_draws);
    MEM_W(0,S32(0x80110898))=S32(0x80200000+4096*8-64*8);draw_training_pause_background(rdram,&ctx);assert(backgrounds==1);
    MEM_W(0,S32(0x80110898))=S32(0x80200000);MEM_W(0,S32(0x8011089c))=S32(0x80300000+128*64-64);
    draw_training_lines(rdram,&ctx,{"No matrix capacity"},100,24,560,0x80850000);assert(draws==prior_draws);
    assert(!pause_tick(rdram,&ctx));assert(!draw_pause(rdram,&ctx));
    live=s;live.active=true;
    assert(!pause_tick(rdram,&ctx)); // Preserve native ordinary controller logic.
    MEM_W(0,S32(0x80118cb4))=0x1000;MEM_W(0,S32(0x80118cb0))=0x1000;
    assert(pause_tick(rdram,&ctx)&&MEM_W(0,S32(0x8010849c))==1);
    MEM_W(0,S32(0x80118cb4))=0;MEM_W(0,S32(0x80118cb0))=0;pause_tick(rdram,&ctx);
    MEM_W(0,S32(0x80118d74))=0x0400;pause_tick(rdram,&ctx);pause_tick(rdram,&ctx); // restart row
    MEM_W(0,S32(0x80118d74))=0x8000;MEM_W(0,S32(0x80118cb0))=0x8000;pause_tick(rdram,&ctx);
    assert(restarts==1&&MEM_W(0,S32(0x8010849c))==0);
    assert(MEM_W(0,S32(0x80118cb0))==0&&MEM_W(0,S32(0x80118cb4))==0);
    std::cout<<"PASS pause navigation, paging, catalog gating, mapped command, option cycles, shortcuts, release handling, native font/context restoration and display-list capacity\n";
}
