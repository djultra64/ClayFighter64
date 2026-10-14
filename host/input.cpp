// ClayTality 64 action input layer by djultra64.
#include "input.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
namespace clay::input {
static constexpr uint16_t masks[] = {0x8000,0x4000,0x1000,0x2000,0x20,0x10,8,4,2,1,0x800,0x400,0x200,0x100};
static const char* actions[] = {"Primary action", "Secondary action", "Start", "Z trigger", "L shoulder", "R shoulder", "C up", "C down", "C left", "C right", "Move up", "Move down", "Move left", "Move right"};
static constexpr Physical defaults[] = {Physical::South,Physical::West,Physical::Start,Physical::LeftTrigger,Physical::LeftShoulder,Physical::RightShoulder,Physical::North,Physical::RightTrigger,Physical::Back,Physical::East,Physical::DUp,Physical::DDown,Physical::DLeft,Physical::DRight};
static const char* keys[] = {"Z","X","Return","Left Shift","Q","E","I","K","J","L","Up","Down","Left","Right"};
static const char* physical_names[] = {"south","east","west","north","start","back","left_shoulder","right_shoulder","left_trigger","right_trigger","dpad_up","dpad_down","dpad_left","dpad_right"};
const char* action_name(Action a) { return actions[size_t(a)]; }
const char* family_name(Family f) { static const char* n[]={"auto","keyboard","xbox","playstation","switch"}; return n[size_t(f)]; }
Family parse_family(const std::string& s) { for(int i=0;i<5;i++) if(s==family_name(Family(i))) return Family(i); throw std::invalid_argument("Unknown controller family: "+s); }
Mapper::Mapper(bool menu) {
    for(size_t i=0;i<action_count;i++) bindings_[i]={defaults[i],keys[i]};
    if(menu){bindings_[size_t(Action::Secondary)].button=Physical::East;bindings_[size_t(Action::CRight)].button=Physical::West;}
}
void Mapper::set_family(Family f) { if(f==Family::Keyboard) throw std::invalid_argument("Manual family must be auto/xbox/playstation/switch"); manual_=f; }
void Mapper::remap(Action a,Physical p) { if(size_t(a)>=action_count||size_t(p)>=physical_count) throw std::invalid_argument("Invalid binding"); bindings_[size_t(a)].button=p; }
void Mapper::remap_key(Action a,const std::string& key) { if(key.empty()||size_t(a)>=action_count) throw std::invalid_argument("Invalid keyboard binding"); bindings_[size_t(a)].key=key; }
const Binding& Mapper::binding(Action a) const { return bindings_.at(size_t(a)); }
State Mapper::update(const Snapshot& s) {
    if(s.detected!=Family::Auto&&s.detected!=Family::Keyboard) detected_=s.detected;
    // SDL events preserve activity order; callers set at most one activity flag.
    if(s.keyboard_activity) active_=Family::Keyboard;
    else if(s.controller_activity) active_=detected_;
    State out; out.active=family();
    for(size_t i=0;i<action_count;i++) if(s.keys[i]||s.buttons[size_t(bindings_[i].button)]) out.buttons|=masks[i];
    float kx=float(s.keys[size_t(Action::Right)])-float(s.keys[size_t(Action::Left)]);
    float ky=float(s.keys[size_t(Action::Up)])-float(s.keys[size_t(Action::Down)]);
    // Held keys don't erase a pad stick or steal the active-device prompts.
    out.x = active_==Family::Keyboard&&kx!=0 ? kx : s.x;
    out.y = active_==Family::Keyboard&&ky!=0 ? ky : s.y;
    if(out.x==0) out.x=kx;
    if(out.y==0) out.y=ky;
    out.x=std::clamp(out.x,-1.f,1.f);out.y=std::clamp(out.y,-1.f,1.f);
    return out;
}
Prompt Mapper::prompt(Action a) const {
    auto b=binding(a); auto f=family(); std::string glyph,name;
    if(f==Family::Keyboard) return {a,action_name(a),"["+b.key+"]",b.key};
    size_t p=size_t(b.button);
    const char* xbox[]={"A","B","X","Y","Menu","View","LB","RB","LT","RT","D-pad up","D-pad down","D-pad left","D-pad right"};
    const char* ps[]={"Cross","Circle","Square","Triangle","Options","Share","L1","R1","L2","R2","D-pad up","D-pad down","D-pad left","D-pad right"};
    const char* sw[]={"B","A","Y","X","+","-","L","R","ZL","ZR","D-pad up","D-pad down","D-pad left","D-pad right"};
    name=(f==Family::Switch?sw:f==Family::PlayStation?ps:xbox)[p];
    if(f==Family::PlayStation&&p<4) {const char* symbols[]={"×","○","□","△"};glyph=symbols[p];}
    else glyph="["+name+"]";
    return {a,action_name(a),glyph,name};
}
bool Mapper::save(const std::string& path,std::string& error) const {
    std::ofstream out(path);if(!out){error="Cannot write controls: "+path;return false;}
    out<<"# ClayTality 64 controls by djultra64\nfamily "<<family_name(manual_)<<'\n';
    for(size_t i=0;i<action_count;i++) out<<"bind "<<i<<' '<<physical_names[size_t(bindings_[i].button)]<<' '<<bindings_[i].key<<'\n';
    if(!out){error="Controls write failed";return false;}return true;
}
bool Mapper::load(const std::string& path,std::string& error) {
    std::ifstream in(path);if(!in){error="Cannot read controls: "+path;return false;}
    Mapper next=*this; std::string line;size_t line_no=0;
    try {while(std::getline(in,line)) {line_no++;if(line.empty()||line[0]=='#')continue;
        std::istringstream row(line);std::string op;row>>op;
        if(op=="family") {std::string f,extra;row>>f;if(f.empty()||row>>extra)throw std::invalid_argument("Invalid family record");next.set_family(parse_family(f));}
        else if(op=="bind") {size_t index;std::string physical,key;if(!(row>>index>>physical)||index>=action_count)throw std::invalid_argument("Invalid binding record");std::getline(row>>std::ws,key);size_t p=0;for(;p<physical_count;p++)if(physical==physical_names[p])break;if(p==physical_count||key.empty())throw std::invalid_argument("Invalid button/key");next.remap(Action(index),Physical(p));next.remap_key(Action(index),key);}
        else throw std::invalid_argument("Unknown controls record");
    }}catch(const std::exception& e){error="Controls line "+std::to_string(line_no)+": "+e.what();return false;}
    *this=next;return true;
}
void draw_prompts(const Mapper& mapper,const Hud& hud,const std::vector<Action>& actions) {
    if(!hud.text||actions.empty())return;
    if(hud.panel)hud.panel(hud.context,16,16,420,32+28*actions.size());
    hud.text(hud.context,28,26,family_name(mapper.family()));
    float y=54;for(auto a:actions){auto p=mapper.prompt(a);auto label=p.glyph+"  "+p.action_name;hud.text(hud.context,28,y,label.c_str());y+=28;}
}
}
