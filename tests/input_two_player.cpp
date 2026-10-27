// Real SDL virtual-controller routing regression; djultra64.
#include "input.h"
#include <SDL.h>
#include <cassert>
#include <cstdio>
#include <filesystem>
struct Pad{
 int index=-1;SDL_Joystick* joy=nullptr;
 Pad(const char* name){SDL_VirtualJoystickDesc d{};d.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;d.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER;d.naxes=SDL_CONTROLLER_AXIS_MAX;d.nbuttons=SDL_CONTROLLER_BUTTON_MAX;d.button_mask=(1u<<SDL_CONTROLLER_BUTTON_MAX)-1;d.axis_mask=(1u<<SDL_CONTROLLER_AXIS_MAX)-1;d.name=name;index=SDL_JoystickAttachVirtualEx(&d);assert(index>=0);joy=SDL_JoystickOpen(index);assert(joy);SDL_JoystickSetVirtualAxis(joy,SDL_CONTROLLER_AXIS_TRIGGERLEFT,-32768);SDL_JoystickSetVirtualAxis(joy,SDL_CONTROLLER_AXIS_TRIGGERRIGHT,-32768);}
 int32_t id(){return SDL_JoystickInstanceID(joy);}
 void button(int b,bool down){assert(SDL_JoystickSetVirtualButton(joy,b,down)==0);SDL_JoystickUpdate();SDL_GameControllerUpdate();}
 void detach(){if(joy){SDL_JoystickClose(joy);joy=nullptr;}if(index>=0){assert(SDL_JoystickDetachVirtual(index)==0);index=-1;}}
 ~Pad(){detach();}
};
static void events(clay::input::SdlInput& input){SDL_Event e;SDL_PumpEvents();while(SDL_PollEvent(&e))input.event(e);}
int main(){
 clay::input::configure_sdl();SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");assert(SDL_Init(SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS)==0);
 {
  Pad idle("Unassigned physical-pad stand-in"),p1("Preferred player one"),p2("Preferred player two");
  clay::input::SdlInput input;input.set_preferred_instances(p1.id(),p2.id());events(input);auto both=input.poll_players(false);assert(input.connected_players()[0]&&input.connected_players()[1]);assert(both[0].buttons==0&&both[1].buttons==0);
  assert(SDL_JoystickSetVirtualAxis(p2.joy,SDL_CONTROLLER_AXIS_LEFTY,-24000)==0);events(input);
  both=input.poll_players(false);assert(both[0].buttons==0&&(both[1].buttons&0x0800));
  both=input.poll_players(true);assert(!(both[1].buttons&0x0800)&&both[1].y>.5f);
  assert(SDL_JoystickSetVirtualAxis(p2.joy,SDL_CONTROLLER_AXIS_LEFTY,0)==0);events(input);
  p1.button(SDL_CONTROLLER_BUTTON_A,true);events(input);both=input.poll_players(false);assert(both[0].buttons==0x8000&&both[1].buttons==0);
  p2.button(SDL_CONTROLLER_BUTTON_B,true);events(input);both=input.poll_players(false);assert(both[0].buttons==0x8000&&both[1].buttons==0x4000);
  idle.button(SDL_CONTROLLER_BUTTON_Y,true);events(input);both=input.poll_players(false);assert(both[0].buttons==0x8000&&both[1].buttons==0x4000);
  p1.button(SDL_CONTROLLER_BUTTON_A,false);p2.button(SDL_CONTROLLER_BUTTON_B,false);events(input);both=input.poll_players(false);assert(!both[0].buttons&&!both[1].buttons);
  input.mapper().remap(clay::input::Action::Primary,clay::input::Physical::North);
  p2.button(SDL_CONTROLLER_BUTTON_A,true);events(input);both=input.poll_players(false);assert(both[0].buttons==0&&both[1].buttons==0x8000); // Menus retain South after combat remap.
  both=input.poll_players(true);assert(both[0].buttons==0&&(both[1].buttons&0x8000));
  input.mapper(1).remap(clay::input::Action::Primary,clay::input::Physical::North);
  p2.button(SDL_CONTROLLER_BUTTON_A,false);p2.button(SDL_CONTROLLER_BUTTON_Y,true);events(input);both=input.poll_players(true);assert(both[0].buttons==0&&(both[1].buttons&0x8000));
  p1.button(SDL_CONTROLLER_BUTTON_X,true);events(input);both=input.poll_players(true);assert(both[0].buttons==0x4000&&(both[1].buttons&0x8000));
  auto keyboard=[&]{SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.scancode=SDL_SCANCODE_Z;input.event(e);input.poll_players(false);assert(input.active_mapper(false).family()==clay::input::Family::Keyboard);};
  assert(SDL_JoystickSetVirtualAxis(p1.joy,SDL_CONTROLLER_AXIS_LEFTX,20000)==0);events(input);input.poll_players(false);keyboard();
  assert(SDL_JoystickSetVirtualAxis(p2.joy,SDL_CONTROLLER_AXIS_LEFTX,20000)==0);events(input);input.poll_players(false);assert(input.active_mapper(false).family()==clay::input::Family::Xbox);
  assert(SDL_JoystickSetVirtualAxis(p1.joy,SDL_CONTROLLER_AXIS_TRIGGERLEFT,20000)==0);events(input);input.poll_players(false);keyboard();
  assert(SDL_JoystickSetVirtualAxis(p2.joy,SDL_CONTROLLER_AXIS_TRIGGERLEFT,20000)==0);events(input);input.poll_players(false);assert(input.active_mapper(false).family()==clay::input::Family::Xbox);
  assert(SDL_JoystickSetVirtualAxis(p2.joy,SDL_CONTROLLER_AXIS_LEFTX,20000)==0);events(input);both=input.poll_players(false);assert(both[0].x>.5f&&both[1].x>.5f);
  p2.detach();events(input);both=input.poll_players(true);assert(input.connected_players()[0]&&!input.connected_players()[1]);assert((both[0].buttons&0x4000)&&both[1].buttons==0&&both[1].x==0);
 }
 {
  Pad first("Connected first but idle"),second("Pressed first");
  SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  clay::input::SdlInput input;input.poll_players(false);
  assert(input.connected_players()[1]); // The game sees P2 before either pad is used.
  second.button(SDL_CONTROLLER_BUTTON_A,true);events(input);
  auto both=input.poll_players(false);
  assert((both[0].buttons&0x8000)&&both[1].buttons==0);
  second.button(SDL_CONTROLLER_BUTTON_A,false);first.button(SDL_CONTROLLER_BUTTON_B,true);events(input);
  both=input.poll_players(false);assert(both[0].buttons==0&&(both[1].buttons&0x4000));
  first.button(SDL_CONTROLLER_BUTTON_B,false);first.button(SDL_CONTROLLER_BUTTON_A,true);events(input);input.poll_players(false);
  clay::input::begin_binding_edit();assert(clay::input::binding_player()==1);
  auto temp=std::filesystem::temp_directory_path()/"clay-player-remap-test";std::filesystem::create_directories(temp);
  clay::input::set_controls_storage((temp/"controls.cfg").string());
  clay::input::request_binding(clay::input::Action::Primary);
  second.button(SDL_CONTROLLER_BUTTON_Y,true);events(input);assert(clay::input::binding_capture_active());
  first.button(SDL_CONTROLLER_BUTTON_X,true);events(input);assert(!clay::input::binding_capture_active());
  assert(input.mapper(1).binding(clay::input::Action::Primary).button==clay::input::Physical::West);
  assert(input.mapper(0).binding(clay::input::Action::Primary).button==clay::input::Physical::South);
  clay::input::Mapper saved;std::string error;
  assert(saved.load((temp/"controls-p2.cfg").string(),error));
  assert(saved.binding(clay::input::Action::Primary).button==clay::input::Physical::West);
  assert(!std::filesystem::exists(temp/"controls.cfg"));
  clay::input::set_controls_storage("");std::filesystem::remove_all(temp);
  clay::input::end_binding_edit();
 }
 {
  Pad idle("Connected idle"),active("Stick moved first");
  SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  clay::input::SdlInput input;input.poll_players(false);
  assert(SDL_JoystickSetVirtualAxis(active.joy,SDL_CONTROLLER_AXIS_LEFTX,25000)==0);events(input);
  auto both=input.poll_players(false);
  assert((both[0].buttons&0x0100)&&both[1].buttons==0);
  idle.button(SDL_CONTROLLER_BUTTON_A,true);events(input);both=input.poll_players(false);
  assert((both[0].buttons&0x0100)&&(both[1].buttons&0x8000));
 }
 SDL_Quit();puts("PASS: two real SDL virtual pads, explicit preferred IDs, no last-device player stealing, independent buttons/sticks and combat-only remapping");
}
