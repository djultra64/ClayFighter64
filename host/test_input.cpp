// Feed scripted physical controller events through SDL's real virtual-pad API. djultra64.
#include "test_input.h"
#include <SDL.h>
#include "json.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <map>
#include <stdexcept>
#include <vector>
namespace clay::testing {
struct InputScript::Impl {
 struct Event {unsigned at;SDL_GameControllerButton button;bool down;unsigned player;};
 std::vector<Event> events;size_t next=0;std::array<SDL_Joystick*,2> pads{};unsigned players=1;
 bool started=false;std::chrono::steady_clock::time_point start;
 ~Impl(){for(int p=1;p>=0;p--)if(pads[p]){int id=SDL_JoystickInstanceID(pads[p]),index=-1;for(int i=0;i<SDL_NumJoysticks();i++)if(SDL_JoystickGetDeviceInstanceID(i)==id)index=i;SDL_JoystickClose(pads[p]);if(index>=0)SDL_JoystickDetachVirtual(index);}}
};
InputScript::InputScript(const std::string& path):impl_(std::make_unique<Impl>()){
 const std::map<std::string,SDL_GameControllerButton> buttons{{"south",SDL_CONTROLLER_BUTTON_A},{"east",SDL_CONTROLLER_BUTTON_B},{"west",SDL_CONTROLLER_BUTTON_X},{"north",SDL_CONTROLLER_BUTTON_Y},{"start",SDL_CONTROLLER_BUTTON_START},{"back",SDL_CONTROLLER_BUTTON_BACK},{"lb",SDL_CONTROLLER_BUTTON_LEFTSHOULDER},{"rb",SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},{"up",SDL_CONTROLLER_BUTTON_DPAD_UP},{"down",SDL_CONTROLLER_BUTTON_DPAD_DOWN},{"left",SDL_CONTROLLER_BUTTON_DPAD_LEFT},{"right",SDL_CONTROLLER_BUTTON_DPAD_RIGHT}};
 std::ifstream stream(path);if(!stream)throw std::runtime_error("Cannot read explicit validation input script");auto data=nlohmann::json::parse(stream);
 for(const auto& step:data){auto name=step.at("button").get<std::string>();auto found=buttons.find(name);if(found==buttons.end())throw std::runtime_error("Unknown validation button");unsigned at=step.at("at_ms").get<unsigned>(),duration=step.value("duration_ms",150u);if(duration==0||duration>60000||at>3600000)throw std::runtime_error("Invalid validation input timing");unsigned player=step.value("player",1u);if(player<1||player>2)throw std::runtime_error("Validation player must be1 or2");impl_->players=std::max(impl_->players,player);impl_->events.push_back({at,found->second,true,player-1});impl_->events.push_back({at+duration,found->second,false,player-1});}
 std::stable_sort(impl_->events.begin(),impl_->events.end(),[](auto a,auto b){return a.at<b.at;});
 SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
 SDL_VirtualJoystickDesc desc{};desc.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;desc.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER;desc.naxes=SDL_CONTROLLER_AXIS_MAX;desc.nbuttons=SDL_CONTROLLER_BUTTON_MAX;desc.name="ClayTality64 hidden validation";desc.button_mask=(1u<<SDL_CONTROLLER_BUTTON_MAX)-1;desc.axis_mask=(1u<<SDL_CONTROLLER_AXIS_MAX)-1;
 for(unsigned player=0;player<impl_->players;player++){
 int index=SDL_JoystickAttachVirtualEx(&desc);if(index<0)throw std::runtime_error(SDL_GetError());impl_->pads[player]=SDL_JoystickOpen(index);if(!impl_->pads[player]){SDL_JoystickDetachVirtual(index);throw std::runtime_error(SDL_GetError());}
 if(!SDL_IsGameController(index))throw std::runtime_error("Virtual validation device is not a mapped SDL controller");
 fprintf(stderr,"{\"event\":\"validation_controller\",\"player\":%u,\"index\":%d,\"instance\":%d}\n",player+1,index,int(SDL_JoystickInstanceID(impl_->pads[player])));
 SDL_JoystickSetVirtualAxis(impl_->pads[player],SDL_CONTROLLER_AXIS_TRIGGERLEFT,-32768);SDL_JoystickSetVirtualAxis(impl_->pads[player],SDL_CONTROLLER_AXIS_TRIGGERRIGHT,-32768);
 }
}
InputScript::~InputScript()=default;
int32_t InputScript::instance_id(unsigned player) const{return player<impl_->pads.size()&&impl_->pads[player]?SDL_JoystickInstanceID(impl_->pads[player]):-1;}
void InputScript::tick(){
 if(!impl_->started){impl_->start=std::chrono::steady_clock::now();impl_->started=true;}
 auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-impl_->start).count();
 while(impl_->next<impl_->events.size()&&impl_->events[impl_->next].at<=elapsed){auto e=impl_->events[impl_->next++];auto* pad=impl_->pads[e.player];if(SDL_JoystickSetVirtualButton(pad,e.button,e.down))throw std::runtime_error(SDL_GetError());SDL_JoystickUpdate();SDL_GameControllerUpdate();fprintf(stderr,"{\"event\":\"validation_physical_input\",\"player\":%u,\"at_ms\":%u,\"button\":%d,\"down\":%s,\"raw_button\":%u,\"attached\":%s}\n",e.player+1,e.at,int(e.button),e.down?"true":"false",unsigned(SDL_JoystickGetButton(pad,e.button)),SDL_JoystickGetAttached(pad)?"true":"false");}
}
}
