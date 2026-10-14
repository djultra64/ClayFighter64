// Native Training pause and short option pages. Own work: djultra64.
#include "training_menu_ui.h"
#include "native_prompts.h"
#include "input_mode.h"
#include "test_input.h"
#include <algorithm>
#include <array>
#include <mutex>

namespace clay::training {
namespace {
std::mutex menu_mutex;
TrainingMenu menu;
bool was_paused=false,release_gate=false,was_capture=false;
constexpr unsigned visible_rows=6;
unsigned finishers(const Snapshot& s){return s.moves?unsigned(std::count_if(s.moves->begin(),s.moves->end(),[](const Move& m){return m.category==MoveCategory::Claytality;})):0;}
const Move* finisher(const Snapshot& s,int index,int* move_index=nullptr){
    if(!s.moves)return nullptr;
    for(unsigned i=0;i<s.moves->size();++i)if(s.moves->at(i).category==MoveCategory::Claytality&&index--==0){if(move_index)*move_index=int(i);return &s.moves->at(i);}
    return nullptr;
}
int finisher_ordinal(const Snapshot& s){
    if(!s.moves||s.pinned_move<0)return 0;
    int ordinal=0;
    for(unsigned i=0;i<s.moves->size();++i)if(s.moves->at(i).category==MoveCategory::Claytality){
        if(int(i)==s.pinned_move)return ordinal;
        ++ordinal;
    }
    return 0;
}
template<class T> void cycle(T& value,unsigned size,int direction){value=T((int(value)+int(size)+(direction<0?-1:1))%int(size));}
std::string on(bool value){return value?"On":"Off";}
uint32_t word(uint8_t* rdram,uint32_t p){return uint32_t(MEM_W(0,S32(p)));}
void neutralize(uint8_t* rdram){
    // Only the owner's tick touches RDRAM. Do not leak a fixed-menu confirm into
    // combat, where that physical button can resolve to a different attack.
    for(uint32_t p:{0x80118cb0u,0x80118cb4u,0x80118cccu,0x80118d74u})MEM_W(0,S32(p))=0;
}
}
void TrainingMenu::reset(){page_=MenuPage::Pause;row_=0;character_=-1;finisher_=0;status_.clear();}
unsigned TrainingMenu::count(const Snapshot& s) const {
    switch(page_){
    case MenuPage::Pause:return 5;
    case MenuPage::Options:return 8;
    case MenuPage::Moves:return s.moves?unsigned(s.moves->size()):0;
    case MenuPage::Preset:return 1;
    case MenuPage::Opponent:case MenuPage::Resources:case MenuPage::Shortcuts:return 2;
    case MenuPage::Display:case MenuPage::Restart:return 3;
    case MenuPage::Claytality:return 4;
    }return 0;
}
void TrainingMenu::move(int direction,const Snapshot& s){auto n=count(s);if(n)row_=(row_+n+(direction<0?-1:1))%n;status_.clear();}
MenuIntent TrainingMenu::back(){
    if(page_==MenuPage::Pause)return MenuIntent::Resume;
    page_=page_==MenuPage::Options?MenuPage::Pause:MenuPage::Options;row_=0;status_.clear();return MenuIntent::None;
}
MenuIntent TrainingMenu::activate(Snapshot& s,int direction){
    auto& o=s.options;status_.clear();if(row_>=count(s))return MenuIntent::None;
    switch(page_){
    case MenuPage::Pause:
        if(row_==0)return MenuIntent::Resume;
        if(row_==1){page_=MenuPage::Options;row_=0;break;}
        return row_==2?MenuIntent::Restart:row_==3?MenuIntent::ChangeFighters:MenuIntent::Exit;
    case MenuPage::Options:
        page_=std::array{MenuPage::Moves,MenuPage::Preset,MenuPage::Opponent,MenuPage::Resources,MenuPage::Display,MenuPage::Restart,MenuPage::Claytality,MenuPage::Shortcuts}.at(row_);row_=0;break;
    case MenuPage::Moves:s.pinned_move=int(row_);return MenuIntent::Pin;
    case MenuPage::Preset:
        if(!finishers(s)){status_="No verified Claytality for this fighter";break;}
        o.preset=o.preset==Preset::Normal?Preset::Claytalities:Preset::Normal;
        if(o.preset==Preset::Claytalities&&(s.pinned_move<0||!s.moves||unsigned(s.pinned_move)>=s.moves->size()||s.moves->at(unsigned(s.pinned_move)).category!=MoveCategory::Claytality)){
            finisher(s,0,&s.pinned_move);finisher_=0;return MenuIntent::Pin;
        }
        break;
    case MenuPage::Opponent:
        if(o.preset==Preset::Claytalities){status_="Standing and Guard Off fixed; use Normal preset";break;}
        if(row_==1&&(o.opponent==Opponent::Cpu||o.opponent==Opponent::Player2)){
            status_=std::string(o.opponent==Opponent::Cpu?"CPU":"Player 2")+" controls guard; use a dummy behavior";break;
        }
        if(row_==0)cycle(o.opponent,5,direction);else cycle(o.guard,3,direction);break;
    case MenuPage::Resources:
        if(o.preset==Preset::Claytalities){status_="Resources suspended for Claytalities";break;}
        if(row_==1)cycle(o.super,3,direction);
        else {constexpr std::array<unsigned,4> values={60,180,300,0};auto it=std::find(values.begin(),values.end(),o.recovery_ticks);unsigned i=it==values.end()?1:unsigned(it-values.begin());o.recovery_ticks=values[(i+4+(direction<0?-1:1))%4];}break;
    case MenuPage::Display:
        if(row_==0)o.damage_display=!o.damage_display;else if(row_==1)o.input_history=!o.input_history;else o.pinned_display=!o.pinned_display;break;
    case MenuPage::Restart:
        if(row_==0)cycle(o.position,3,direction);else if(row_==1)o.right_side=!o.right_side;else return MenuIntent::Restart;break;
    case MenuPage::Claytality:
        if(!finishers(s)){status_="No verified Claytality for this fighter";break;}
        if(row_==0){finisher_=finisher_ordinal(s);cycle(finisher_,finishers(s),direction);int index=-1;finisher(s,finisher_,&index);s.pinned_move=index;return MenuIntent::Pin;}
        if(row_==1)o.direct_finisher=!o.direct_finisher;else if(row_==2)o.extended_window=!o.extended_window;else o.automatic_distance=!o.automatic_distance;break;
    case MenuPage::Shortcuts:return row_==0?MenuIntent::BindReset:MenuIntent::BindPause;
    }return MenuIntent::None;
}
MenuView TrainingMenu::view(const Snapshot& s){
    if(character_!=s.character){character_=s.character;finisher_=0;if(page_==MenuPage::Moves)row_=0;}
    // The session owns the selected catalog index. Reopening pause resets only
    // presentation state; derive its Claytality ordinal from that same index.
    finisher_=finisher_ordinal(s);
    auto n=count(s);if(n&&row_>=n)row_=n-1;
    MenuView v;v.selected=row_;v.first=row_/visible_rows*visible_rows;v.status=status_;
    const auto& o=s.options;
    switch(page_){
    case MenuPage::Pause:v.title="TRAINING PAUSED";v.rows={"Resume","Training Options","Restart Training","Change Fighters & Stage","Main Menu"};break;
    case MenuPage::Options:v.title="TRAINING OPTIONS";v.rows={"Move List","Practice Preset","Opponent","Resources","Display","Restart Setup","Claytality Practice","Shortcut Bindings"};break;
    case MenuPage::Moves:
        v.title=std::string("MOVES FOR ")+character_name(s.character);
        if(s.moves)for(const auto& m:*s.moves)v.rows.push_back(std::string(category_name(m.category))+": "+m.name);
        if(!n)v.status="Move catalog unavailable";
        else {const auto& m=s.moves->at(row_);v.status=move_command(m,s.attacks,s.facing_right);if(!m.requirements.empty())v.status+=" / "+m.requirements;}
        break;
    case MenuPage::Preset:
        v.title="PRACTICE PRESET";v.rows={std::string("Preset: ")+(o.preset==Preset::Normal?"Normal":"Claytalities")};
        if(!finishers(s))v.status="Claytalities disabled: no verified finisher";
        break;
    case MenuPage::Opponent:
        v.title="OPPONENT";v.rows={std::string("Behavior: ")+std::array{"Standing","Crouching","Repeated Jump","CPU","Player 2"}[unsigned(o.opponent)],std::string("Guard: ")+std::array{"Off","Always","After first hit"}[unsigned(o.guard)]};
        if(o.preset==Preset::Claytalities){
            // Show effective preset rules while retaining the user's ordinary
            // behavior/guard selections for switching back to Normal.
            v.rows={"Behavior Standing fixed","Guard Off fixed"};v.status="Standing and Guard Off fixed; use Normal preset";
        }else {
            if(o.opponent==Opponent::Cpu||o.opponent==Opponent::Player2)v.rows[1]=std::string("Guard controlled by ")+(o.opponent==Opponent::Cpu?"CPU":"Player 2");
            if(o.opponent==Opponent::Player2&&!s.second_player_connected)v.status="Player 2 disconnected: opponent stands still";
        }
        break;
    case MenuPage::Resources:
        v.title="RESOURCES";v.rows={"Recovery: "+(o.recovery_ticks?std::to_string(o.recovery_ticks/60)+" seconds":"Manual"),std::string("Super: ")+std::array{"Normal","Full on restart","Infinite"}[unsigned(o.super)]};break;
    case MenuPage::Display:v.title="DISPLAY";v.rows={"Damage & combo: "+on(o.damage_display),"Input history: "+on(o.input_history),"Pinned move: "+on(o.pinned_display)};break;
    case MenuPage::Restart:v.title="RESTART SETUP";v.rows={std::string("Position: ")+std::array{"Center","Left corner","Right corner"}[unsigned(o.position)],std::string("P1 side: ")+(o.right_side?"Right":"Left"),"Restart Training"};break;
    case MenuPage::Claytality:{
        v.title="CLAYTALITY PRACTICE";auto m=finisher(s,finisher_);
        v.rows={std::string("Finisher: ")+(m?m->name:"Unavailable"),std::string("Preparation: ")+(o.direct_finisher?"Ready to execute":"Final hit"),std::string("Window: ")+(o.extended_window?"Extended (30s)":"Original"),std::string("Distance: ")+(o.automatic_distance?"Automatic":"Manual")};
        if(!m)v.status="Disabled: no verified Claytality";
        else if(v.status.empty())v.status=move_command(*m,s.attacks,s.facing_right)+" / "+m->requirements;
        break;
    }
    case MenuPage::Shortcuts:v.title="SHORTCUT BINDINGS";v.rows={"Restart: "+input::binding_marker(input::Action::TrainingReset),"Pause: "+input::binding_marker(input::Action::TrainingPause)};v.status="Optional; conflicting combat buttons are rejected";break;
    }return v;
}
bool pause_tick(uint8_t* rdram,recomp_context*){
    auto s=snapshot();std::lock_guard lock(menu_mutex);
    if(!s.active){was_paused=release_gate=was_capture=false;menu.reset();return false;}
    const uint32_t held=word(rdram,0x80118cb0);
    const uint32_t edge=word(rdram,0x80118cb4),nav=word(rdram,0x80118d74);
    bool paused=word(rdram,0x8010849c)!=0;
    bool requested=take_pause_request();
    if(release_gate){if(!held)release_gate=false;else {neutralize(rdram);return true;}}
    if(!paused&&((edge&0x1000)||requested)){
        MEM_W(0,S32(0x8010849c))=1;paused=true;menu.reset();was_paused=true;release_gate=true;
        clay::testing::publish_input_phase(clay::testing::InputPhase::Pause);input::update_combat_input(rdram);neutralize(rdram);return true;
    }
    // The original unpaused3F1F4..3F6A8 path includes controller ownership and
    // disconnect/start handling. Substitute only our paused menu or an explicit
    // pause/resume release transition, leaving ordinary native ticks intact.
    if(!paused){was_paused=false;return false;}
    if(!was_paused){menu.reset();was_paused=true;}
    clay::testing::publish_input_phase(clay::testing::InputPhase::Pause);
    if(input::binding_capture_active()){was_capture=true;return true;}
    if(was_capture){was_capture=false;release_gate=true;neutralize(rdram);return true;}
    MenuIntent intent=MenuIntent::None;
    if((edge&0x1000)||requested)intent=MenuIntent::Resume;
    else if(nav&0x4000)intent=menu.back();
    else if(nav&0x0800)menu.move(-1,s);
    else if(nav&0x0400)menu.move(1,s);
    else if(nav&0x8300){
        // Horizontal changes values on a setting page; it does not accidentally
        // activate actions on the pause/options/move-list pages.
        const bool action_page=menu.page()==MenuPage::Pause||menu.page()==MenuPage::Options||menu.page()==MenuPage::Moves||menu.page()==MenuPage::Shortcuts;
        if((nav&0x8000)||!action_page){intent=menu.activate(s,(nav&0x0200)?-1:1);configure(s.options);}
    }
    if(intent==MenuIntent::Pin)pin_move(s.pinned_move);
    if(intent==MenuIntent::BindReset)input::request_binding(input::Action::TrainingReset);
    if(intent==MenuIntent::BindPause)input::request_binding(input::Action::TrainingPause);
    if(intent==MenuIntent::Restart)request_restart();
    if(intent==MenuIntent::ChangeFighters)request_change_fighters();
    if(intent==MenuIntent::Exit)request_exit();
    if(intent==MenuIntent::Resume||intent==MenuIntent::Restart||intent==MenuIntent::ChangeFighters||intent==MenuIntent::Exit){
        MEM_W(0,S32(0x8010849c))=0;was_paused=false;release_gate=true;neutralize(rdram);input::update_combat_input(rdram);
        clay::testing::publish_input_phase(clay::testing::InputPhase::Combat);
    }
    return true;
}
bool draw_pause(uint8_t* rdram,recomp_context* ctx){
    auto s=snapshot();if(!s.active||!word(rdram,0x8010849c))return false;
    MenuView v;{std::lock_guard lock(menu_mutex);v=menu.view(s);}
    std::vector<std::string> lines={v.title};
    unsigned end=std::min(unsigned(v.rows.size()),v.first+visible_rows);
    for(unsigned i=v.first;i<end;++i)lines.push_back(std::string(i==v.selected?"X ":"  ")+v.rows[i]);
    if(v.rows.size()>visible_rows)lines.push_back("Rows "+std::to_string(v.first+1)+" TO "+std::to_string(end)+" OF "+std::to_string(v.rows.size())+"  UP DOWN");
    if(!v.status.empty())lines.push_back(v.status);
    if(input::binding_capture_active())lines.push_back(input::binding_capture_status());
    lines.push_back(input::menu_marker(input::Action::Primary)+" select  "+input::menu_marker(input::Action::Secondary)+" back");
    draw_training_pause_background(rdram,ctx);
    draw_training_lines(rdram,ctx,lines,160,32,560,0x80850000);return true;
}
}
