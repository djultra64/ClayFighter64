// ClayTality 64 SDL input adapter by djultra64.
#include "input.h"
#include <SDL.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <algorithm>
#include <stdexcept>
namespace clay::input {
namespace {
std::mutex settings_mutex;
std::optional<Action> capture_action;
std::optional<Family> requested_family;
Family configured_family=Family::Auto;
bool reset_requested=false;
std::string capture_status,controls_storage;
std::array<std::string,action_count> binding_labels;
void persist(const Mapper& mapper){if(controls_storage.empty())return;std::string error;if(!mapper.save(controls_storage,error)){capture_status="COULD NOT SAVE BINDING";fprintf(stderr,"Controls preferences: %s\n",error.c_str());}}
std::optional<Physical> physical(SDL_GameControllerButton b){
 switch(b){case SDL_CONTROLLER_BUTTON_LEFTSTICK:return Physical::LeftStick;case SDL_CONTROLLER_BUTTON_RIGHTSTICK:return Physical::RightStick;case SDL_CONTROLLER_BUTTON_A:return Physical::South;case SDL_CONTROLLER_BUTTON_B:return Physical::East;case SDL_CONTROLLER_BUTTON_X:return Physical::West;case SDL_CONTROLLER_BUTTON_Y:return Physical::North;case SDL_CONTROLLER_BUTTON_START:return Physical::Start;case SDL_CONTROLLER_BUTTON_BACK:return Physical::Back;case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:return Physical::LeftShoulder;case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:return Physical::RightShoulder;case SDL_CONTROLLER_BUTTON_DPAD_UP:return Physical::DUp;case SDL_CONTROLLER_BUTTON_DPAD_DOWN:return Physical::DDown;case SDL_CONTROLLER_BUTTON_DPAD_LEFT:return Physical::DLeft;case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:return Physical::DRight;default:return {};}
}
}
void request_binding(Action action){if(size_t(action)>=action_count)return;std::lock_guard lock(settings_mutex);capture_action=action;capture_status="PRESS NEW BUTTON";}
bool binding_capture_active(){std::lock_guard lock(settings_mutex);return capture_action.has_value();}
std::string binding_label(Action action){std::lock_guard lock(settings_mutex);return binding_labels.at(size_t(action));}
std::string binding_capture_status(){std::lock_guard lock(settings_mutex);return capture_status;}
void reset_combat_bindings(){std::lock_guard lock(settings_mutex);reset_requested=true;capture_action.reset();capture_status="DEFAULTS RESTORED";}
void set_controller_family(Family family){if(family==Family::Keyboard)return;std::lock_guard lock(settings_mutex);requested_family=family;}
Family controller_family(){std::lock_guard lock(settings_mutex);return requested_family.value_or(configured_family);}
void set_controls_storage(const std::string& path){std::lock_guard lock(settings_mutex);controls_storage=path;}
void configure_sdl() {
    // SDL button A/B/X/Y become SOUTH/EAST/WEST/NORTH positions, including Switch.
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS,"0");
}
static Family detect(SDL_GameController* pad) {
    auto t=SDL_GameControllerGetType(pad);
    if(t==SDL_CONTROLLER_TYPE_PS3||t==SDL_CONTROLLER_TYPE_PS4||t==SDL_CONTROLLER_TYPE_PS5)return Family::PlayStation;
    if(t==SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO||t==SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT||t==SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT||t==SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR)return Family::Switch;
    return Family::Xbox; // Ambiguous devices can be overridden in Mapper/config.
}
static float axis(Sint16 v) {if(std::abs(int(v))<=6000)return 0;return v<0?float(v)/32768.f:float(v)/32767.f;}
SdlInput::~SdlInput() {for(auto* pad:pads_)SDL_GameControllerClose(pad);}
void SdlInput::find_controller() {
    for(auto it=pads_.begin();it!=pads_.end();){
        if(!SDL_GameControllerGetAttached(*it)){activity_.erase(SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(*it)));if(pad_==*it){pad_=nullptr;instance_=-1;}SDL_GameControllerClose(*it);it=pads_.erase(it);}else ++it;
    }
    // SDL emits controller activity only for opened devices. Keep every mapped
    // pad open so a second/newly attached device can become the active one.
    for(int i=0;i<SDL_NumJoysticks();i++)if(SDL_IsGameController(i)){
        auto id=SDL_JoystickGetDeviceInstanceID(i);bool opened=false;
        for(auto* pad:pads_)if(SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad))==id){opened=true;break;}
        if(!opened)if(auto* pad=SDL_GameControllerOpen(i))pads_.push_back(pad);
    }
    if(!pad_&&!pads_.empty()){pad_=pads_.front();instance_=SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad_));}
}
void SdlInput::set_preferred_instances(int32_t first,int32_t second){
 if(first>=0&&first==second)throw std::invalid_argument("Player controller IDs must differ");
 preferred_={first,second};assigned_={-1,-1};
}
void SdlInput::event(const SDL_Event& e) {
    {std::lock_guard lock(settings_mutex);if(capture_action){
        bool captured=false;
        try{
        if(e.type==SDL_KEYDOWN&&!e.key.repeat){
            if(e.key.keysym.scancode==SDL_SCANCODE_ESCAPE){capture_action.reset();capture_status="CANCELLED";return;}
            const char* name=SDL_GetScancodeName(e.key.keysym.scancode);
            if(name&&*name){mapper_.remap_key(*capture_action,name);captured=true;}
        } else if(e.type==SDL_CONTROLLERBUTTONDOWN){
            auto button=physical(SDL_GameControllerButton(e.cbutton.button));if(button){mapper_.remap(*capture_action,*button);captured=true;}
        } else if(e.type==SDL_CONTROLLERAXISMOTION&&e.caxis.value>16384){
            if(e.caxis.axis==SDL_CONTROLLER_AXIS_TRIGGERLEFT){mapper_.remap(*capture_action,Physical::LeftTrigger);captured=true;}
            if(e.caxis.axis==SDL_CONTROLLER_AXIS_TRIGGERRIGHT){mapper_.remap(*capture_action,Physical::RightTrigger);captured=true;}
        }
        }catch(const std::invalid_argument&){capture_status="BUTTON ALREADY USED BY ANOTHER ACTION";}
        if(captured){capture_action.reset();capture_status="BINDING SAVED";wait_release_=true;persist(mapper_);}
    }}
    if(std::getenv("CLAY_TRACE_INPUT_SDL") && (e.type==SDL_KEYDOWN||e.type==SDL_KEYUP)) {
        auto* focus=SDL_GetKeyboardFocus();
        fprintf(stderr,"{\"event\":\"sdl_key\",\"down\":%s,\"scan\":%d,\"repeat\":%u,\"window\":%u,\"focus_window\":%u}\n",e.type==SDL_KEYDOWN?"true":"false",int(e.key.keysym.scancode),unsigned(e.key.repeat),e.key.windowID,focus?SDL_GetWindowID(focus):0);
    }
    if(e.type==SDL_KEYDOWN&&!e.key.repeat){
        for(size_t i=0;i<action_count;i++)if(e.key.keysym.scancode==SDL_GetScancodeFromName(mapper_.binding(Action(i)).key.c_str())||e.key.keysym.scancode==SDL_GetScancodeFromName(menu_mapper_.binding(Action(i)).key.c_str())){keyboard_activity_=true;pad_activity_=false;break;}
    }
    bool meaningful=e.type==SDL_CONTROLLERBUTTONDOWN;
    if(e.type==SDL_CONTROLLERAXISMOTION){
        auto& history=activity_[e.caxis.which];
        if(e.caxis.axis==SDL_CONTROLLER_AXIS_LEFTX||e.caxis.axis==SDL_CONTROLLER_AXIS_LEFTY){
            float v=axis(e.caxis.value);float& previous=e.caxis.axis==SDL_CONTROLLER_AXIS_LEFTX?history.x:history.y;
            meaningful=std::abs(v)>0.2f&&std::abs(v-previous)>0.1f;
            if(meaningful||v==0)previous=v;
        } else if(e.caxis.axis==SDL_CONTROLLER_AXIS_TRIGGERLEFT||e.caxis.axis==SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
            size_t trigger=e.caxis.axis==SDL_CONTROLLER_AXIS_TRIGGERLEFT?0:1;
            bool down=e.caxis.value>16384;
            meaningful=down&&!history.trigger_down[trigger];history.trigger_down[trigger]=down;
        }
    }
    if(meaningful){
        Sint32 id=e.type==SDL_CONTROLLERBUTTONDOWN?e.cbutton.which:e.caxis.which;
        if(id!=instance_){
            find_controller();
            for(auto* next:pads_)if(SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(next))==id){pad_=next;instance_=id;break;}
        }
        if(id==instance_){pad_activity_=true;keyboard_activity_=false;}
    }
}
std::array<State,2> SdlInput::poll_players(bool combat) {
    {std::lock_guard lock(settings_mutex);
     if(reset_requested){auto family=mapper_.manual_family();mapper_=Mapper{};mapper_.set_family(family);reset_requested=false;persist(mapper_);}
     if(requested_family){mapper_.set_family(*requested_family);requested_family.reset();persist(mapper_);}
    }
    menu_mapper_.set_family(mapper_.manual_family());
    find_controller();Snapshot s;s.keyboard_activity=keyboard_activity_;s.controller_activity=pad_activity_;
    keyboard_activity_=pad_activity_=false;
    int num_keys=0;const Uint8* keys=SDL_GetKeyboardState(&num_keys);
    auto* focus=SDL_GetKeyboardFocus();
    bool focused=focus!=nullptr;
    uint16_t raw_actions=0;
    for(size_t i=0;i<action_count;i++){
        auto key=SDL_GetScancodeFromName(mapper_.binding(Action(i)).key.c_str());
        bool down=key!=SDL_SCANCODE_UNKNOWN&&int(key)<num_keys&&keys[key];
        if(down)raw_actions|=uint16_t(1u<<i);
        s.keys[i]=focused&&down;
    }
    if(std::getenv("CLAY_TRACE_INPUT_SDL")) {
        static uint16_t previous_actions=0xFFFF;static Uint32 previous_focus=~0u;
        auto id=focus?SDL_GetWindowID(focus):0;
        if(raw_actions!=previous_actions||id!=previous_focus) {
            fprintf(stderr,"{\"event\":\"sdl_key_state\",\"raw_action_mask\":%u,\"focus_window\":%u,\"focus_flags\":%u,\"key_count\":%d,\"gate_allows\":%s}\n",unsigned(raw_actions),id,focus?unsigned(SDL_GetWindowFlags(focus)):0,num_keys,focused?"true":"false");
            previous_actions=raw_actions;previous_focus=id;
        }
    }
    auto controller=[&](int32_t id)->SDL_GameController*{for(auto* p:pads_)if(SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(p))==id)return p;return nullptr;};
    for(unsigned i=0;i<2;i++)if(assigned_[i]>=0&&!controller(assigned_[i]))assigned_[i]=-1;
    for(unsigned i=0;i<2;i++)if(preferred_[i]>=0&&controller(preferred_[i]))assigned_[i]=preferred_[i];
    for(auto* pad:pads_){int32_t id=SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad));if(id==assigned_[0]||id==assigned_[1])continue;for(unsigned i=0;i<2;i++)if(assigned_[i]<0&&preferred_[i]<0){assigned_[i]=id;break;}}
    connected_={true,controller(assigned_[1])!=nullptr};
    std::array<Snapshot,2> snapshots{};snapshots[0]=s;
    for(unsigned player=0;player<2;player++)if(auto* pad=controller(assigned_[player])){
        auto& snapshot=snapshots[player];snapshot.detected=detect(pad);
        snapshot.controller_activity=s.controller_activity&&assigned_[player]==instance_;
        const SDL_GameControllerButton physical[]={SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_B,SDL_CONTROLLER_BUTTON_X,SDL_CONTROLLER_BUTTON_Y,SDL_CONTROLLER_BUTTON_START,SDL_CONTROLLER_BUTTON_BACK,SDL_CONTROLLER_BUTTON_LEFTSHOULDER,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER};
        for(size_t i=0;i<8;i++)snapshot.buttons[i]=SDL_GameControllerGetButton(pad,physical[i]);
        snapshot.buttons[size_t(Physical::LeftTrigger)]=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_TRIGGERLEFT)>16384;
        snapshot.buttons[size_t(Physical::RightTrigger)]=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_TRIGGERRIGHT)>16384;
        const SDL_GameControllerButton directions[]={SDL_CONTROLLER_BUTTON_DPAD_UP,SDL_CONTROLLER_BUTTON_DPAD_DOWN,SDL_CONTROLLER_BUTTON_DPAD_LEFT,SDL_CONTROLLER_BUTTON_DPAD_RIGHT};
        for(size_t i=0;i<4;i++)snapshot.buttons[size_t(Physical::DUp)+i]=SDL_GameControllerGetButton(pad,directions[i]);
        snapshot.buttons[size_t(Physical::LeftStick)]=SDL_GameControllerGetButton(pad,SDL_CONTROLLER_BUTTON_LEFTSTICK);
        snapshot.buttons[size_t(Physical::RightStick)]=SDL_GameControllerGetButton(pad,SDL_CONTROLLER_BUTTON_RIGHTSTICK);
        snapshot.x=axis(SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTX));snapshot.y=-axis(SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTY));
    }
    unsigned recent=0;if(!s.keyboard_activity&&assigned_[1]>=0&&assigned_[1]==instance_)recent=1;
    Snapshot recent_snapshot=snapshots[recent];recent_snapshot.keyboard_activity=s.keyboard_activity;recent_snapshot.controller_activity=s.controller_activity;
    mapper_.update(recent_snapshot);
    std::array<State,2> combat_states{},menu_states{};
    for(unsigned player=0;player<2;player++){
        auto local=mapper_;combat_states[player]=local.update(snapshots[player]);
        auto menu=snapshots[player];
        for(size_t i=0;i<action_count;i++){
            auto key=SDL_GetScancodeFromName(menu_mapper_.binding(Action(i)).key.c_str());
            menu.keys[i]=player==0&&focused&&key!=SDL_SCANCODE_UNKNOWN&&int(key)<num_keys&&keys[key];
        }
        if(player==recent)menu_mapper_.update(menu);
        auto local_menu=menu_mapper_;menu_states[player]=local_menu.update(menu);
    }
    {std::lock_guard lock(settings_mutex);configured_family=mapper_.manual_family();for(size_t i=0;i<action_count;i++)binding_labels[i]=mapper_.prompt(Action(i)).button_name;}
    if(training_mode_&&previous_combat_!=combat)wait_release_=true;
    previous_combat_=combat;
    bool released=true;for(unsigned i=0;i<2;i++)released &= menu_states[i].buttons==0&&combat_states[i].buttons==0&&
        !combat_states[i].training_reset&&!combat_states[i].training_pause&&combat_states[i].x==0&&combat_states[i].y==0;
    if(wait_release_&&released)wait_release_=false;
    if(binding_capture_active()||wait_release_){for(auto& state:menu_states){state={};}return menu_states;}
    for(unsigned player=0;player<2;player++){
        menu_states[player].training_reset=combat_states[player].training_reset;
        menu_states[player].training_pause=combat_states[player].training_pause;
    }
    return combat?combat_states:menu_states;
}
State SdlInput::poll(bool combat){return poll_players(combat)[0];}
}
