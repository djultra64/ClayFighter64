// Native training presentation and independently testable navigation. djultra64.
#pragma once
#include "training_guest.h"
#include "move_list.h"
namespace clay::training {
enum class MenuPage { Pause, Options, Moves, Preset, Opponent, Resources, Display, Restart, Claytality, Shortcuts };
enum class MenuIntent { None, Resume, Restart, ChangeFighters, Exit, Pin, BindReset, BindPause };
struct MenuView {
    std::string title,status;
    std::vector<std::string> rows;
    unsigned selected=0,first=0;
};
class TrainingMenu {
    MenuPage page_=MenuPage::Pause;
    unsigned row_=0;
    int character_=-1,finisher_=-1;
    std::string status_;
public:
    void reset();
    MenuPage page() const { return page_; }
    unsigned row() const { return row_; }
    unsigned count(const Snapshot& state) const;
    void move(int direction,const Snapshot& state);
    MenuIntent activate(Snapshot& state,int direction=1);
    MenuIntent back();
    int selected_move() const {return int(row_);}
    MenuView view(const Snapshot& state);
};
// Each renderer uses a separate CPU scratch reservation and keeps guest registers,
// font placement, and caller stack intact. The display-list writer is the output.
void draw_training_lines(uint8_t* rdram,recomp_context* ctx,
    const std::vector<std::string>& lines,int first_y,int step,unsigned max_width,uint32_t scratch,uint32_t font_override=0);
void draw_training_pause_background(uint8_t* rdram,recomp_context* ctx);
}
