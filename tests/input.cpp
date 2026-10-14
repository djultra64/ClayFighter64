// ClayTality 64 input verification by djultra64.
#include "input.h"
#include <cassert>
#include <cstdio>
#include <fstream>
using namespace clay::input;
int main(){
    Mapper m;Snapshot s;s.detected=Family::Switch;s.controller_activity=true;
    s.buttons[size_t(Physical::South)]=true;
    assert(m.update(s).buttons==0x8000);assert(m.prompt(Action::Primary).button_name=="B");
    assert(m.prompt(Action::Secondary).button_name=="Y");
    assert(m.prompt(Action::CUp).button_name=="X");assert(m.prompt(Action::CRight).button_name=="A");
    s.controller_activity=false;s.keyboard_activity=true;s.keys[size_t(Action::Start)]=true;
    assert(m.update(s).active==Family::Keyboard);
    // Held pad button must not steal prompts back from the latest keyboard event.
    s.keyboard_activity=false;assert(m.update(s).active==Family::Keyboard);
    s.controller_activity=true;s.detected=Family::PlayStation;assert(m.update(s).active==Family::PlayStation);
    assert(m.prompt(Action::Primary).glyph=="×");assert(m.prompt(Action::Secondary).button_name=="Square");
    s.controller_activity=false;assert(m.update(s).active==Family::PlayStation);
    m.set_family(Family::Switch);assert(m.prompt(Action::Primary).button_name=="B");
    m.remap(Action::Primary,Physical::East);assert(m.prompt(Action::Primary).button_name=="A");
    s.keys.fill(false);s.buttons.fill(false);s.buttons[size_t(Physical::East)]=true;
    assert((m.update(s).buttons&0x8000)!=0);
    m.set_family(Family::Auto);s=Snapshot{};s.keyboard_activity=true;s.keys[size_t(Action::Up)]=true;
    auto out=m.update(s);assert(out.buttons==0x800&&out.y==1.f);
    s=Snapshot{};s.controller_activity=true;s.buttons[size_t(Physical::DLeft)]=true;
    assert(m.update(s).buttons==0x200);
    // SDL positive Y is inverted in adapter; core bounds extremes.
    s.x=-1.1f;s.y=1.1f;out=m.update(s);assert(out.x==-1.f&&out.y==1.f);
    std::string error; m.remap_key(Action::Primary,"Space");m.set_family(Family::Xbox);
    const std::string path="/tmp/clay-input-test-controls.cfg";
    assert(m.save(path,error));Mapper loaded;assert(loaded.load(path,error));
    assert(loaded.binding(Action::Primary).button==Physical::East);assert(loaded.binding(Action::Primary).key=="Space");
    loaded.update(s);assert(loaded.prompt(Action::Primary).button_name=="B");
    {std::ofstream bad(path);bad<<"bind 0 south Z\nbind 99 invalid X\n";}
    assert(!loaded.load(path,error));assert(loaded.binding(Action::Primary).button==Physical::East);
    std::vector<std::string> labels;
    Hud hud;hud.context=&labels;hud.text=[](void* context,float,float,const char* text){static_cast<std::vector<std::string>*>(context)->push_back(text);};
    draw_prompts(loaded,hud,{Action::Primary,Action::Secondary});
    assert(labels.size()==3&&labels[1]=="[B]  Primary action");
    std::remove(path.c_str());puts("PASS: physical layouts, last-device persistence, per-action remapping, keyboard/menu D-pad, analog bounds, config roundtrip and atomic invalid-config rejection");
}
