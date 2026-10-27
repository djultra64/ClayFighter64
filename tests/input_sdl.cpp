// ClayTality 64 SDL event ordering verification by djultra64.
#include "input.h"
#include <SDL.h>
#include <cassert>
#include <cstdio>
using namespace clay::input;
int main(){
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
    configure_sdl();assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER)==0);
    auto* window=SDL_CreateWindow("input test",0,0,16,16,SDL_WINDOW_HIDDEN);assert(window);
    SDL_VirtualJoystickDesc desc{};desc.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    desc.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER;desc.naxes=SDL_CONTROLLER_AXIS_MAX;
    desc.nbuttons=SDL_CONTROLLER_BUTTON_MAX;desc.name="Clay input test controller";
    desc.button_mask=(1u<<SDL_CONTROLLER_BUTTON_MAX)-1;desc.axis_mask=(1u<<SDL_CONTROLLER_AXIS_MAX)-1;
    int index=SDL_JoystickAttachVirtualEx(&desc);assert(index>=0);
    assert(SDL_IsGameController(index));
    {
        SdlInput input;input.set_preferred_instances(SDL_JoystickGetDeviceInstanceID(index));input.poll();
        SDL_Event e{};e.type=SDL_CONTROLLERBUTTONDOWN;e.cbutton.which=SDL_JoystickGetDeviceInstanceID(index);e.cbutton.button=SDL_CONTROLLER_BUTTON_A;
        input.event(e);assert(input.poll().active==Family::Xbox);
        e={};e.type=SDL_KEYDOWN;e.key.keysym.scancode=SDL_SCANCODE_Z;
        input.event(e);assert(input.poll().active==Family::Keyboard);assert(input.poll().active==Family::Keyboard);
        e.key.repeat=1;input.event(e);assert(input.poll().active==Family::Keyboard);
        e={};e.type=SDL_CONTROLLERAXISMOTION;e.caxis.which=SDL_JoystickGetDeviceInstanceID(index);e.caxis.axis=SDL_CONTROLLER_AXIS_LEFTX;e.caxis.value=100;
        input.event(e);assert(input.poll().active==Family::Keyboard);
        e.caxis.value=20000;input.event(e);assert(input.poll().active==Family::Xbox);
        input.mapper().set_family(Family::Switch);assert(input.mapper().prompt(Action::Primary).button_name=="B");
        e={};e.type=SDL_KEYDOWN;e.key.keysym.scancode=SDL_SCANCODE_RETURN;input.event(e);assert(input.poll().active==Family::Keyboard);
        e={};e.type=SDL_CONTROLLERAXISMOTION;e.caxis.which=SDL_JoystickGetDeviceInstanceID(index);e.caxis.axis=SDL_CONTROLLER_AXIS_TRIGGERLEFT;e.caxis.value=20000;
        input.event(e);assert(input.poll().active==Family::Switch);
        e={};e.type=SDL_KEYDOWN;e.key.keysym.scancode=SDL_SCANCODE_Z;input.event(e);assert(input.poll().active==Family::Keyboard);
        e={};e.type=SDL_CONTROLLERAXISMOTION;e.caxis.which=SDL_JoystickGetDeviceInstanceID(index);e.caxis.axis=SDL_CONTROLLER_AXIS_TRIGGERLEFT;e.caxis.value=20001;
        input.event(e);assert(input.poll().active==Family::Keyboard);
    }
    {
        SdlInput input;input.set_preferred_instances(SDL_JoystickGetDeviceInstanceID(index));input.poll();input.mapper().remap(Action::Primary,Physical::East);
        auto* pad=SDL_JoystickOpen(index);assert(pad);
        assert(SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_B,1)==0);SDL_PumpEvents();
        SDL_Event activity{};activity.type=SDL_CONTROLLERBUTTONDOWN;activity.cbutton.which=SDL_JoystickGetDeviceInstanceID(index);activity.cbutton.button=SDL_CONTROLLER_BUTTON_B;input.event(activity);
        assert((input.poll(false).buttons&0x8000)==0);
        assert(input.poll(false).buttons&0x4000);
        assert(input.active_mapper(false).prompt(Action::Secondary).button_name=="B");
        assert(input.active_mapper(true).binding(Action::Secondary).button==Physical::West);
        assert((input.poll(true).buttons&0x8000)!=0);
        assert(input.active_mapper(false).binding(Action::Primary).button==Physical::South);
        assert(input.active_mapper(true).binding(Action::Primary).button==Physical::East);
        SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_B,0);SDL_PumpEvents();
        SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_X,1);SDL_PumpEvents();
        assert(!(input.poll(false).buttons&0x4000));assert(input.poll(true).buttons&0x4000);
        SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_X,0);SDL_PumpEvents();
        SDL_Event confirm{};confirm.type=SDL_CONTROLLERBUTTONDOWN;confirm.cbutton.which=SDL_JoystickGetDeviceInstanceID(index);confirm.cbutton.button=SDL_CONTROLLER_BUTTON_A;input.event(confirm);begin_binding_edit();
        request_binding(Action::Primary);assert(binding_capture_active());
        SDL_Event capture{};capture.type=SDL_CONTROLLERBUTTONDOWN;capture.cbutton.which=SDL_JoystickGetDeviceInstanceID(index);capture.cbutton.button=SDL_CONTROLLER_BUTTON_Y;
        SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_Y,1);SDL_PumpEvents();input.event(capture);
        assert(!binding_capture_active()&&input.mapper().binding(Action::Primary).button==Physical::North);
        assert(input.poll(false).buttons==0&&input.poll(true).buttons==0); // binding press must not operate the menu
        SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_Y,0);SDL_PumpEvents();input.poll(false);
        SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_Y,1);SDL_PumpEvents();assert(input.poll(true).buttons&0x8000);
        SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_Y,0);end_binding_edit();SDL_JoystickClose(pad);
    }
    {
        SdlInput input;input.set_preferred_instances(SDL_JoystickGetDeviceInstanceID(index));input.poll();
        auto* pad=SDL_JoystickOpen(index);assert(pad);
        SDL_JoystickSetVirtualAxis(pad,SDL_CONTROLLER_AXIS_LEFTY,-24000);SDL_PumpEvents();
        assert(input.poll(false).buttons&0x0800); // Left stick navigates menus like Up.
        assert(!(input.poll(true).buttons&0x0800)); // Combat keeps the analog stick analog.
        SDL_JoystickSetVirtualAxis(pad,SDL_CONTROLLER_AXIS_LEFTY,0);SDL_PumpEvents();
        assert(!(input.poll(false).buttons&0x0800));
        SDL_JoystickClose(pad);
    }
    {
        SdlInput input;input.set_preferred_instances(SDL_JoystickGetDeviceInstanceID(index));input.poll();
        int second=SDL_JoystickAttachVirtualEx(&desc);assert(second>=0);input.set_preferred_instances(SDL_JoystickGetDeviceInstanceID(index),SDL_JoystickGetDeviceInstanceID(second));input.poll();
        auto* extra=SDL_JoystickOpen(second);assert(extra);
        SDL_JoystickSetVirtualButton(extra,SDL_CONTROLLER_BUTTON_A,1);SDL_PumpEvents();
        SDL_Event e;while(SDL_PollEvent(&e))input.event(e);
        auto players=input.poll_players(false);assert(players[0].buttons==0&&(players[1].buttons&0x8000)); // Newly attachedP2 owns its port while updating global prompts.
        SDL_JoystickSetVirtualButton(extra,SDL_CONTROLLER_BUTTON_A,0);SDL_JoystickClose(extra);SDL_JoystickDetachVirtual(second);
    }
    SDL_JoystickDetachVirtual(index);SDL_DestroyWindow(window);SDL_Quit();
    puts("PASS: SDL virtual pad, last event order, repeat/jitter rejection, manual family, held trigger persistence");
}
