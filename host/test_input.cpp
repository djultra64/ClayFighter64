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
 struct Event {unsigned at,duration;SDL_GameControllerButton button;unsigned player;int phase=-1;bool pressed=false,done=false;unsigned pressed_at=0,occurrence=0;bool wait_for_poll=false,acknowledged=false;};
 std::vector<Event> events;InputPhase phase=InputPhase::Startup;unsigned phase_at=0;std::array<unsigned,8> occurrences{{1,0,0,0,0,0,0,0}};std::array<SDL_Joystick*,2> pads{};unsigned players=1;
 bool started=false;std::chrono::steady_clock::time_point start;
 ~Impl(){for(int p=1;p>=0;p--)if(pads[p]){int id=SDL_JoystickInstanceID(pads[p]),index=-1;for(int i=0;i<SDL_NumJoysticks();i++)if(SDL_JoystickGetDeviceInstanceID(i)==id)index=i;SDL_JoystickClose(pads[p]);if(index>=0)SDL_JoystickDetachVirtual(index);}}
};
InputScript::InputScript(const std::string& path):impl_(std::make_unique<Impl>()){
 const std::map<std::string,SDL_GameControllerButton> buttons{{"ls",SDL_CONTROLLER_BUTTON_LEFTSTICK},{"rs",SDL_CONTROLLER_BUTTON_RIGHTSTICK},{"south",SDL_CONTROLLER_BUTTON_A},{"east",SDL_CONTROLLER_BUTTON_B},{"west",SDL_CONTROLLER_BUTTON_X},{"north",SDL_CONTROLLER_BUTTON_Y},{"start",SDL_CONTROLLER_BUTTON_START},{"back",SDL_CONTROLLER_BUTTON_BACK},{"lb",SDL_CONTROLLER_BUTTON_LEFTSHOULDER},{"rb",SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},{"up",SDL_CONTROLLER_BUTTON_DPAD_UP},{"down",SDL_CONTROLLER_BUTTON_DPAD_DOWN},{"left",SDL_CONTROLLER_BUTTON_DPAD_LEFT},{"right",SDL_CONTROLLER_BUTTON_DPAD_RIGHT}};
 std::ifstream stream(path);if(!stream)throw std::runtime_error("Cannot read explicit validation input script");auto data=nlohmann::json::parse(stream);
 for(const auto& step:data){auto name=step.at("button").get<std::string>();auto found=buttons.find(name);if(found==buttons.end())throw std::runtime_error("Unknown validation button");unsigned at=step.at("at_ms").get<unsigned>(),duration=step.value("duration_ms",150u);if(duration==0||duration>60000||at>3600000)throw std::runtime_error("Invalid validation input timing");unsigned player=step.value("player",1u);if(player<1||player>2)throw std::runtime_error("Validation player must be1 or2");impl_->players=std::max(impl_->players,player);int phase=-1;
  if(step.contains("phase")){
   const std::map<std::string,InputPhase> phases{{"startup",InputPhase::Startup},{"main_menu",InputPhase::MainMenu},{"fighter_select",InputPhase::FighterSelect},{"opponent_select",InputPhase::OpponentSelect},{"stage_select",InputPhase::StageSelect},{"loading",InputPhase::Loading},{"combat",InputPhase::Combat},{"pause",InputPhase::Pause}};
   auto it=phases.find(step.at("phase").get<std::string>());if(it==phases.end())throw std::runtime_error("Unknown validation input phase");phase=int(it->second);
  }
  Impl::Event event{at,duration,found->second,player-1,phase};event.occurrence=step.value("occurrence",0u);if(event.occurrence>4096||(event.occurrence&&phase<0))throw std::runtime_error("Invalid validation phase occurrence");event.wait_for_poll=step.value("wait_for_poll",false);impl_->events.push_back(event);}
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
 const auto phase=current_input_phase();
 if(phase!=impl_->phase){impl_->phase=phase;impl_->phase_at=unsigned(elapsed);++impl_->occurrences[size_t(phase)];}
 for(auto& e:impl_->events){
  if(e.done)continue;
  bool down=false,emit=false;
  if(!e.pressed){
   if(e.phase>=0&&(e.phase!=int(phase)||(e.occurrence&&e.occurrence!=impl_->occurrences[size_t(phase)])))continue;
   const unsigned origin=e.phase<0?0:impl_->phase_at;
   if(elapsed<origin+e.at)continue;
   if(e.wait_for_poll&&std::any_of(impl_->events.begin(),impl_->events.end(),[&](const auto& old){return old.phase==e.phase&&old.at<e.at&&old.pressed&&!old.done;}))continue;
   e.pressed=true;e.pressed_at=unsigned(elapsed);down=true;emit=true;
   if(e.wait_for_poll)script_seen_buttons[e.player].store(0,std::memory_order_release);
  }else{
   if(e.wait_for_poll&&!e.acknowledged){
    uint16_t mask=0;
    switch(e.button){case SDL_CONTROLLER_BUTTON_A:mask=0x8000;break;case SDL_CONTROLLER_BUTTON_B:mask=e.phase==int(InputPhase::Combat)?1:0x4000;break;case SDL_CONTROLLER_BUTTON_X:mask=e.phase==int(InputPhase::Combat)?0x4000:1;break;case SDL_CONTROLLER_BUTTON_Y:mask=8;break;case SDL_CONTROLLER_BUTTON_START:mask=0x1000;break;case SDL_CONTROLLER_BUTTON_BACK:mask=2;break;case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:mask=0x20;break;case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:mask=0x10;break;case SDL_CONTROLLER_BUTTON_DPAD_UP:mask=0x800;break;case SDL_CONTROLLER_BUTTON_DPAD_DOWN:mask=0x400;break;case SDL_CONTROLLER_BUTTON_DPAD_LEFT:mask=0x200;break;case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:mask=0x100;break;default:break;}
    if(mask&&(script_seen_buttons[e.player].load(std::memory_order_acquire)&mask)!=mask)continue;
    e.acknowledged=true;e.pressed_at=unsigned(elapsed);
   }
   if(elapsed>=e.pressed_at+e.duration){e.done=true;emit=true;}
  }
  if(!emit)continue;
  auto* pad=impl_->pads[e.player];
  if(SDL_JoystickSetVirtualButton(pad,e.button,down))throw std::runtime_error(SDL_GetError());
  SDL_JoystickUpdate();SDL_GameControllerUpdate();
  fprintf(stderr,"{\"event\":\"validation_physical_input\",\"player\":%u,\"at_ms\":%u,\"elapsed_ms\":%u,\"button\":%d,\"down\":%s,\"raw_button\":%u,\"attached\":%s}\n",e.player+1,e.at+(down?0:e.duration),unsigned(elapsed),int(e.button),down?"true":"false",unsigned(SDL_JoystickGetButton(pad,e.button)),SDL_JoystickGetAttached(pad)?"true":"false");
 }

}
}
