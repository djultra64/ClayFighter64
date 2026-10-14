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
 assert(SDL_NumJoysticks()==before);std::remove(argv[1]);SDL_Quit();
}
