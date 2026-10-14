// Actual SDL validation driver regression by djultra64.
#include "test_input.h"
#include <SDL.h>
#include <cassert>
#include <fstream>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==2);SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");assert(SDL_Init(SDL_INIT_GAMECONTROLLER)==0);
 {std::ofstream out(argv[1]);out<<R"([{"at_ms":0,"button":"south","player":1},{"at_ms":0,"button":"west","player":2}])";}
 int before=SDL_NumJoysticks();
 {clay::testing::InputScript script(argv[1]);assert(SDL_NumJoysticks()==before+2);assert(script.instance_id()!=script.instance_id(1));script.tick();
  auto* p1=SDL_JoystickFromInstanceID(script.instance_id());auto* p2=SDL_JoystickFromInstanceID(script.instance_id(1));assert(p1&&p2);
  assert(SDL_JoystickGetButton(p1,SDL_CONTROLLER_BUTTON_A));assert(!SDL_JoystickGetButton(p1,SDL_CONTROLLER_BUTTON_X));
  assert(SDL_JoystickGetButton(p2,SDL_CONTROLLER_BUTTON_X));assert(!SDL_JoystickGetButton(p2,SDL_CONTROLLER_BUTTON_A));
 }
 assert(SDL_NumJoysticks()==before);
 {std::ofstream out(argv[1]);out<<R"([{"at_ms":0,"phase":"main_menu","button":"right","duration_ms":10}])";}
 clay::testing::publish_input_phase(clay::testing::InputPhase::Startup);
 {clay::testing::InputScript script(argv[1]);auto* pad=SDL_JoystickFromInstanceID(script.instance_id());
  script.tick();assert(!SDL_JoystickGetButton(pad,SDL_CONTROLLER_BUTTON_DPAD_RIGHT));
  clay::testing::publish_input_phase(clay::testing::InputPhase::MainMenu);
  script.tick();assert(SDL_JoystickGetButton(pad,SDL_CONTROLLER_BUTTON_DPAD_RIGHT));
  clay::testing::publish_input_phase(clay::testing::InputPhase::Combat);
  SDL_Delay(15);script.tick();assert(!SDL_JoystickGetButton(pad,SDL_CONTROLLER_BUTTON_DPAD_RIGHT));
 }
 assert(SDL_NumJoysticks()==before);
 {std::ofstream out(argv[1]);out<<R"([{"at_ms":0,"phase":"combat","occurrence":1,"button":"south","duration_ms":10},{"at_ms":0,"phase":"combat","occurrence":2,"button":"west","duration_ms":10}])";}
 clay::testing::publish_input_phase(clay::testing::InputPhase::Startup);
 {clay::testing::InputScript script(argv[1]);auto* pad=SDL_JoystickFromInstanceID(script.instance_id());script.tick();
  clay::testing::publish_input_phase(clay::testing::InputPhase::Combat);script.tick();
  assert(SDL_JoystickGetButton(pad,SDL_CONTROLLER_BUTTON_A)&&!SDL_JoystickGetButton(pad,SDL_CONTROLLER_BUTTON_X));
  clay::testing::publish_input_phase(clay::testing::InputPhase::Loading);SDL_Delay(15);script.tick();assert(!SDL_JoystickGetButton(pad,SDL_CONTROLLER_BUTTON_A));
  clay::testing::publish_input_phase(clay::testing::InputPhase::Combat);script.tick();assert(!SDL_JoystickGetButton(pad,SDL_CONTROLLER_BUTTON_A)&&SDL_JoystickGetButton(pad,SDL_CONTROLLER_BUTTON_X));
 }
 // Slow rendering can delay the actual guest controller poll beyond an entire
 // scripted press. wait_for_poll must retain the real physical press until the
 // correct player/action is observed, then begin its full requested duration.
 {std::ofstream out(argv[1]);out<<R"([{"at_ms":0,"phase":"main_menu","button":"south","player":2,"duration_ms":10,"wait_for_poll":true},{"at_ms":1,"phase":"main_menu","button":"down","player":2,"duration_ms":10,"wait_for_poll":true}])";}
 clay::testing::publish_input_phase(clay::testing::InputPhase::Startup);
 {clay::testing::InputScript script(argv[1]);auto* p1=SDL_JoystickFromInstanceID(script.instance_id());auto* p2=SDL_JoystickFromInstanceID(script.instance_id(1));assert(p1&&p2);
  script.tick();assert(!SDL_JoystickGetButton(p2,SDL_CONTROLLER_BUTTON_A));
  clay::testing::publish_input_phase(clay::testing::InputPhase::MainMenu);script.tick();
  SDL_Delay(25);script.tick();assert(SDL_JoystickGetButton(p2,SDL_CONTROLLER_BUTTON_A));assert(!SDL_JoystickGetButton(p2,SDL_CONTROLLER_BUTTON_DPAD_DOWN));
  clay::testing::observe_script_input(0,0x8000);script.tick(); // Correct action, wrong player.
  assert(SDL_JoystickGetButton(p2,SDL_CONTROLLER_BUTTON_A));
  clay::testing::observe_script_input(1,0x4000);script.tick(); // Correct player, wrong action.
  assert(SDL_JoystickGetButton(p2,SDL_CONTROLLER_BUTTON_A));
  clay::testing::observe_script_input(1,0x8000);script.tick(); // Guest finally polled it.
  assert(SDL_JoystickGetButton(p2,SDL_CONTROLLER_BUTTON_A)); // Duration starts at acknowledgement.
  SDL_Delay(15);script.tick();assert(!SDL_JoystickGetButton(p2,SDL_CONTROLLER_BUTTON_A));assert(SDL_JoystickGetButton(p2,SDL_CONTROLLER_BUTTON_DPAD_DOWN));
  // The next ordered press has its own acknowledgement, not the prior poll.
  SDL_Delay(25);script.tick();assert(SDL_JoystickGetButton(p2,SDL_CONTROLLER_BUTTON_DPAD_DOWN));
  clay::testing::observe_script_input(1,0x400);script.tick();assert(SDL_JoystickGetButton(p2,SDL_CONTROLLER_BUTTON_DPAD_DOWN));
  SDL_Delay(15);script.tick();assert(!SDL_JoystickGetButton(p2,SDL_CONTROLLER_BUTTON_DPAD_DOWN));
  assert(!SDL_JoystickGetButton(p1,SDL_CONTROLLER_BUTTON_A)&&!SDL_JoystickGetButton(p1,SDL_CONTROLLER_BUTTON_DPAD_DOWN));
 }
 assert(SDL_NumJoysticks()==before);std::remove(argv[1]);SDL_Quit();
}
