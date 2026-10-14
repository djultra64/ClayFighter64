// Isolated native training menu contracts. djultra64.
#include "training_menu_ui.h"
#include "graphics_config.h"
#include "native_prompts.h"
#include "test_input.h"
#include "ui_background.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <bit>
#include <map>
using namespace clay::training;
namespace {
Snapshot live;
bool pause_request=false;
unsigned restarts=0,changes=0,exits=0,draws=0;
bool text_pipeline=false,orthographic=false;
unsigned backgrounds=0;
float glyph_scale_y=1,glyph_scale_x=1,model_y=0,model_x=0;
bool verify_left=false;unsigned left_draws=0;float expected_left=-144;
bool capture_rows=false;std::map<int,std::string> rendered_rows;
bool capture_geometry=false;std::vector<std::pair<float,float>> glyph_bounds;
clay::input::Action bound=clay::input::Action::Count;
std::string caption(uint8_t* rdram,uint32_t p){std::string s;for(unsigned i=0;i<512;i++){char c=MEM_BU(0,S32(p+i));if(!c)return s;s+=c;}assert(false);return {};}
}
namespace clay {GraphicsOptions graphics_options(){return {};}}
namespace clay::training {
Snapshot snapshot(){return live;}
void configure(const Options& o){live.options=o;}
void request_restart(){++restarts;}
void request_exit(){++exits;}
void request_change_fighters(){++changes;}
void pin_move(int i){live.pinned_move=i;}
bool take_pause_request(){bool p=pause_request;pause_request=false;return p;}

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
void func_800B0328(uint8_t*,recomp_context* c){glyph_scale_x=std::bit_cast<float>(uint32_t(c->r5));glyph_scale_y=std::bit_cast<float>(uint32_t(c->r6));}
void func_800B09DC(uint8_t*,recomp_context* c){model_x=std::bit_cast<float>(uint32_t(c->r5))*glyph_scale_x;model_y=std::bit_cast<float>(uint32_t(c->r6))*glyph_scale_y;}
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
    if(capture_rows)rendered_rows[int(std::lround(model_y*2))]+=s;
    for(unsigned char ch:s)assert(std::isalnum(ch)||ch=='%'); // Explicit resolvedicon or realfontletters/digits.
    // Conservative budget used by production renderer reserves 32 commands per
    // byte. Simulate that maximum and verify no overrun even at capacity edges.
    MEM_W(0,S32(0x80110898))+=int(s.size()*31*8);
    assert(uint32_t(MEM_W(0,S32(0x80110898)))<=0x80200000+4096*8);
    assert(MEM_W(0,S32(uint32_t(c->r7)))==int32_t(0xc0000000)||MEM_W(0,S32(uint32_t(c->r7)))<0);
    const auto font=uint32_t(c->r4);
    if(verify_left){assert(std::abs(model_x-expected_left)<0.01f);if(left_draws++==0)assert(MEM_H(0,S32(font+0x10))==0);}
    const auto vertical=uint32_t(MEM_W(0x10,c->r29));
    const auto ratio=std::bit_cast<float>(uint32_t(MEM_W(0,S32(vertical+4))));
    const auto glyph_map=uint32_t(MEM_W(0,S32(font+4)));
    const auto sprite=uint32_t(MEM_W(0,S32(glyph_map+(unsigned(s.front())-33)*4)));
    const unsigned actual_height=sprite?MEM_HU(0,S32(sprite+8)):font==0x80084000u?15:29;
    if(capture_geometry){
        assert(sprite);
        // Observable A6520 geometry: 6B48..6C38 adds the signed sprite bearing
        // along the vertical basis to the 7191C placement. Its quad extends one
        // scaled sprite height from that origin; average font advance is absent.
        const float origin=model_y+glyph_scale_y*(float(MEM_H(0,S32(font+0x12)))+
            float(MEM_H(0,S32(sprite+12)))*ratio);
        glyph_bounds.emplace_back(origin-glyph_scale_y*ratio*actual_height,origin);
    }
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
    const auto option_rows=menu.view(s).rows;
    assert(option_rows==std::vector<std::string>({"Practice Preset","Opponent","Resources","Display","Restart Setup","Claytality Practice","Shortcut Bindings"}));
    for(unsigned i=0;i<6;i++)menu.move(1,s);
    assert(menu.view(s).first==6&&menu.view(s).rows.size()==7);
    menu.activate(s);assert(menu.page()==MenuPage::Shortcuts);
    assert(menu.activate(s)==MenuIntent::BindReset);menu.move(1,s);assert(menu.activate(s)==MenuIntent::BindPause);
    menu.back();for(unsigned i=0;i<5;i++)menu.move(1,s);menu.activate(s);
    assert(menu.page()==MenuPage::Claytality&&menu.count(s)==3);
    const auto unprepared=s.options.direct_finisher;
    menu.activate(s);assert(s.pinned_move==-1&&s.options.direct_finisher==unprepared&&menu.view(s).status.find("verified")!=std::string::npos);
    menu.back();menu.activate(s); // preset
    menu.activate(s);assert(s.options.preset==Preset::Normal);
    auto moves=std::make_shared<std::vector<Move>>(3);
    moves->at(0).name="Hidden special";moves->at(0).category=MoveCategory::Special;
    moves->at(1).name="Hidden finisher";moves->at(1).category=MoveCategory::Claytality;
    moves->at(2).name="Hidden super";moves->at(2).category=MoveCategory::Super;s.moves=moves;
    assert(menu.activate(s)==MenuIntent::Pin&&s.options.preset==Preset::Claytalities&&s.pinned_move==1);
    menu.back();for(unsigned i=0;i<2;i++)menu.move(1,s);menu.activate(s); // resources
    s.options.preset=Preset::Normal;assert(s.options.recovery_ticks==180);
    menu.activate(s);assert(s.options.recovery_ticks==300);menu.activate(s);assert(s.options.recovery_ticks==0);
    menu.activate(s);assert(s.options.recovery_ticks==60);menu.activate(s,-1);assert(s.options.recovery_ticks==0);
    menu.back();for(unsigned i=0;i<3;i++)menu.move(1,s);menu.activate(s);
    assert(menu.page()==MenuPage::Display&&menu.count(s)==2);
    assert(menu.view(s).rows.size()==2);
    const auto damage_before=s.options.damage_display,history_before=s.options.input_history;
    menu.activate(s);assert(s.options.damage_display!=damage_before);
    menu.move(1,s);menu.activate(s);assert(s.options.input_history!=history_before);
    menu.move(1,s);assert(menu.row()==0); // no third pinned-command setting.
    TrainingMenu opponent_menu;opponent_menu.reset();opponent_menu.move(1,s);opponent_menu.activate(s);opponent_menu.move(1,s);opponent_menu.activate(s);
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
    // Claytality preparation survives removal of every move-selection control.
    TrainingMenu clay_menu;clay_menu.reset();clay_menu.move(1,s);clay_menu.activate(s);
    for(unsigned i=0;i<5;i++)clay_menu.move(1,s);
    clay_menu.activate(s);assert(clay_menu.page()==MenuPage::Claytality);
    const auto clay_rows=clay_menu.view(s).rows;
    assert(clay_rows.size()==3&&clay_rows[0].starts_with("Preparation:")&&clay_rows[1].starts_with("Window:")&&clay_rows[2].starts_with("Distance:"));
    const auto pinned_before=s.pinned_move;
    const auto direct_before=s.options.direct_finisher,window_before=s.options.extended_window,distance_before=s.options.automatic_distance;
    assert(clay_menu.activate(s)==MenuIntent::None&&s.options.direct_finisher!=direct_before);
    clay_menu.move(1,s);clay_menu.activate(s);assert(s.options.extended_window!=window_before);
    clay_menu.move(1,s);clay_menu.activate(s);assert(s.options.automatic_distance!=distance_before);
    clay_menu.move(1,s);assert(clay_menu.row()==0&&s.pinned_move==pinned_before);
    // Every reachable option page remains free of catalog names and commands.
    for(unsigned i=0;i<option_rows.size();++i){
        TrainingMenu page;page.reset();page.move(1,s);page.activate(s);
        for(unsigned j=0;j<i;++j)page.move(1,s);
        page.activate(s);const auto view=page.view(s);
        assert(view.title.find("MOVES")==std::string::npos&&view.status.find("Hidden")==std::string::npos);
        for(const auto& row:view.rows)assert(row.find("Hidden")==std::string::npos&&row.find("Pinned")==std::string::npos&&row.find("Finisher:")==std::string::npos&&row.find("Move List")==std::string::npos);
    }

    std::vector<uint8_t> memory(9*1024*1024);auto rdram=memory.data();recomp_context ctx{};ctx.r4=77;ctx.r29=S32(0x807ff000);ctx.f_odd=&ctx.f0.u32h;
    const auto original=ctx;
    MEM_W(0,S32(0x801028d0))=S32(0x80080000);MEM_W(0,S32(0x80110880))=4096;
    MEM_W(0,S32(0x8010cfd0))=320;MEM_W(0,S32(0x8010cfd4))=240;
    MEM_W(0,S32(0x80080004))=S32(0x80082000);MEM_W(0,S32(0x80082000+('A'-33)*4))=S32(0x80083000);
    MEM_H(0,S32(0x80083006))=32;MEM_H(0,S32(0x80083008))=29;
    MEM_H(0,S32(0x8008300c))=28; // Actual71070 loaded29pxletter bearing.
    MEM_W(0,S32(0x80082000+('%'-33)*4))=S32(0x80083100);MEM_H(0,S32(0x80083106))=15;MEM_H(0,S32(0x80083108))=15;
    MEM_W(0,S32(0x801108dc))=S32(0x80081000);MEM_W(0,S32(0x80081000))=S32(0x80200000);MEM_W(0,S32(0x80110898))=S32(0x80200000);
    MEM_W(0,S32(0x80110888))=128;MEM_W(0,S32(0x801108e0))=S32(0x80081010);MEM_W(0,S32(0x80081010))=S32(0x80300000);MEM_W(0,S32(0x8011089c))=S32(0x80300000);
    for(unsigned i=0;i<6;i++)MEM_H(0,S32(0x80080010+i*2))=i+20;
    draw_training_lines(rdram,&ctx,{"A short line","Next line"},100,24,560,0x80850000);
    assert(draws>2&&!std::memcmp(&ctx,&original,sizeof(ctx)));
    for(unsigned i=0;i<6;i++)assert(MEM_HU(0,S32(0x80080010+i*2))==i+20);
    draw_training_lines(rdram,&ctx,{std::string("A\x1e")+"A\x1f"},100,24,560,0x80850000);
    draw_training_lines(rdram,&ctx,{"Move: A+B / C & D (1-6)"},100,24,560,0x80850000);
    // Loaded digits vary in height/bearing; licensed15px icons have bearing0.
    for(unsigned n=0;n<2;++n){
        const uint32_t sprite=0x80083200+n*0x100;
        MEM_W(0,S32(0x80082000+('0'+n-33)*4))=S32(sprite);
        MEM_H(0,S32(sprite+6))=n?17:30;MEM_H(0,S32(sprite+8))=n?29:28;
        MEM_H(0,S32(sprite+12))=n?28:27;
    }
    auto aligned_bounds=[&](uint32_t font){
        for(int average:{1,22,80}){
            MEM_H(0,S32(font+0x1c))=average;
            glyph_bounds.clear();capture_geometry=true;
            draw_training_lines(rdram,&ctx,{std::string("01\x1e")+"A\x1f A"},100,24,560,0x80850000,font);
            capture_geometry=false;assert(glyph_bounds.size()==4);
            for(const auto& bounds:glyph_bounds){
                assert(std::abs(bounds.first-glyph_bounds.back().first)<0.15f);
                assert(std::abs(bounds.second-glyph_bounds.back().second)<0.15f);
            }
            assert(!std::memcmp(&ctx,&original,sizeof(ctx)));
        }
    };
    aligned_bounds(0x80080000);
    const uint32_t stage_font=0x80084000,stage_map=0x80084100,stage_glyph=0x80085000;
    MEM_W(0,S32(stage_font+4))=S32(stage_map);MEM_W(0,S32(stage_map+('A'-33)*4))=S32(stage_glyph);
    MEM_H(0,S32(stage_glyph+6))=9;MEM_H(0,S32(stage_glyph+8))=15;
    MEM_H(0,S32(stage_glyph+12))=14;
    for(char ch:std::string("01"))MEM_W(0,S32(stage_map+(unsigned(ch)-33)*4))=S32(stage_glyph);
    MEM_W(0,S32(stage_map+('%'-33)*4))=S32(0x80083100);
    aligned_bounds(stage_font);
    for(unsigned i=0;i<6;i++)MEM_H(0,S32(stage_font+0x10+i*2))=i+80;
    draw_training_lines(rdram,&ctx,{"A"},-214,32,560,0x80870000,stage_font);
    assert(std::abs(glyph_scale_y*15-8.f)<0.01f&&std::abs(model_y+107.f)<0.01f);
    assert(uint32_t(MEM_W(0,S32(0x801028d0)))==0x80080000u&&!std::memcmp(&ctx,&original,sizeof(ctx)));
    for(unsigned i=0;i<6;i++)assert(MEM_HU(0,S32(stage_font+0x10+i*2))==i+80);
    verify_left=true;left_draws=0;
    draw_training_lines(rdram,&ctx,{"A left aligned line"},100,28,560,0x80850000,0,-288,16,true);
    verify_left=false;assert(left_draws>0&&!std::memcmp(&ctx,&original,sizeof(ctx)));
    live=s;live.active=true;live.options.damage_display=true;live.options.input_history=true;
    live.options.opponent=Opponent::Standing;live.options.preset=Preset::Normal;
    live.damage={7,21,3,false};live.history={"A 12 ticks"};
    MEM_W(0,S32(0x8010849c))=0;
    MEM_W(0,S32(0x80110898))=S32(0x80200000);MEM_W(0,S32(0x8011089c))=S32(0x80300000);
    expected_left=float(training_left_edge())*.5f;
    capture_rows=verify_left=true;left_draws=0;rendered_rows.clear();
    draw_hud(rdram,&ctx);capture_rows=verify_left=false;
    assert(rendered_rows.size()==4&&left_draws>0);
    assert(rendered_rows.at(110)=="Damage7"&&rendered_rows.at(88)=="Combo21"&&rendered_rows.at(66)=="Hits3");
    assert(rendered_rows.at(32)=="12A");
    for(const auto& [y,text]:rendered_rows)assert(text.find("ticks")==std::string::npos&&text.find("Hidden")==std::string::npos);
    // Practice counters occupy their own rows and history follows beneath them.
    live.options.preset=Preset::Claytalities;live.attempts=5;live.successes=2;
    MEM_W(0,S32(0x80110898))=S32(0x80200000);MEM_W(0,S32(0x8011089c))=S32(0x80300000);
    expected_left=float(training_left_edge())*.5f;
    capture_rows=verify_left=true;left_draws=0;rendered_rows.clear();
    draw_hud(rdram,&ctx);capture_rows=verify_left=false;
    assert(rendered_rows.size()==6&&rendered_rows.at(44)=="Attempts5"&&rendered_rows.at(22)=="Completed2"&&rendered_rows.at(-12)=="12A");
    assert(!std::memcmp(&ctx,&original,sizeof(ctx)));
    live={};
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
    std::cout<<"PASS pause navigation, paging, no move-list controls, Claytality preparation, option cycles, shortcuts, release handling, native font/context restoration and display-list capacity\n";
}
