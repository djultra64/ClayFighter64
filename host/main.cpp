#include "native_preferences.h"
// ClayFighter 64 native integration by djultra64.
#include "host.h"
#include "funcs.h"
#include "input.h"
#include "title_audio.h"
#include "sc_bios.h"
#include "main_menu.h"
#include "input_hud.h"
#include "graphics_config.h"
#include "sc_assets.h"
#include "sc_roster.h"
#include "native_prompts.h"
#include "input_mode.h"
#include "test_input.h"
#include "portable_paths.h"
#include "training_guest.h"
#include "training_options.h"
#include "training_stage.h"
#include "ultramodern/ultramodern.hpp"
#include <SDL.h>
#ifdef _WIN32
#include <SDL_syswm.h>
#endif
#include "librecomp/rsp.hpp"
#include <fstream>
#include <filesystem>
#include <iterator>
#include <chrono>
#include <thread>
#include <mutex>
#include <algorithm>
#include <array>
#include <cstring>
static std::mutex input_mutex;
static std::array<clay::input::State,2> input_states{};
static std::array<bool,2> input_connected{true,false};
static std::string input_title;
static int sc_demo_character=-1;
static std::string manual_family="auto";
static void update_input(SDL_Window* window,clay::input::SdlInput& input){
 const bool combat=clay::input::combat_input_active();
 input.set_training_mode(clay::main_menu::training_requested());
 const auto states=input.poll_players(combat);
 clay::training::set_second_player_connected(input.connected_players()[1]);
 static bool reset_down=false,pause_down=false;
 if(states[0].training_reset&&!reset_down&&states[0].buttons==0)clay::training::request_restart();
 if(states[0].training_pause&&!pause_down)clay::training::request_pause();
 reset_down=states[0].training_reset;pause_down=states[0].training_pause;
 const auto& mapper=input.active_mapper(combat);
 clay::input::publish_hud(mapper);
 std::string title="ClayFighter 64 v2.1 — ";title+=clay::input::family_name(mapper.family());
 if(sc_demo_character>=0){title+=" — Prueba SC ";title+=std::to_string(sc_demo_character);}
 for(auto action:{clay::input::Action::Primary,clay::input::Action::Secondary,clay::input::Action::Start}){
  auto prompt=mapper.prompt(action);title+=" — ";title+=prompt.glyph;title+=" ";title+=prompt.action_name;
 }
 if(title!=input_title){input_title=title;SDL_SetWindowTitle(window,title.c_str());fprintf(stderr,"{\"event\":\"active_input\",\"family\":\"%s\"}\n",clay::input::family_name(mapper.family()));}
 std::lock_guard lock(input_mutex);input_states=states;input_connected=input.connected_players();
}
static bool get_input(int port,uint16_t* b,float* x,float* y){
 if(port<0||port>=2)return false;
 std::lock_guard lock(input_mutex);
 const bool rival=clay::training::selecting_opponent();
 const int source=rival&&port==1?0:port;
 if(!input_connected[source]&&!(port==1&&clay::main_menu::training_requested()))return false;
 const bool ready=clay::training::selection_input_ready(input_states[0].buttons);
 const auto state=(!ready||(rival&&port==0))?clay::input::State{}:input_states[source];
 *b=state.buttons;*x=state.x;*y=state.y;clay::testing::observe_script_input(unsigned(source),state.buttons);++clay_polls;
 if(std::getenv("CLAY_TRACE_INPUT")){
  static std::array<clay::input::State,2> previous{};auto& old=previous[port];
  if(*b!=old.buttons||*x!=old.x||*y!=old.y){
   fprintf(stderr,"{\"event\":\"guest_input\",\"port\":%d,\"poll\":%llu,\"buttons\":%u,\"x\":%.3f,\"y\":%.3f}\n",port,(unsigned long long)clay_polls.load(),unsigned(*b),*x,*y);
   old.buttons=*b;old.x=*x;old.y=*y;
  }
 }
 return true;
}
static ultramodern::input::connected_device_info_t connected_device(int port){
 std::lock_guard lock(input_mutex);bool connected=port>=0&&port<2&&(input_connected[port]||(port==1&&clay::main_menu::training_requested()));
 return {connected?ultramodern::input::Device::Controller:ultramodern::input::Device::None,ultramodern::input::Pak::None};
}
RspExitReason clayfighter_audio_rsp(uint8_t*,uint32_t);
static std::mutex audio_mutex;
static SDL_AudioDeviceID audio_device=0;
static void set_frequency(uint32_t hz){std::lock_guard lock(audio_mutex);if(audio_device)SDL_CloseAudioDevice(audio_device);SDL_AudioSpec spec{};spec.freq=hz;spec.format=AUDIO_S16SYS;spec.channels=2;spec.samples=1024;audio_device=SDL_OpenAudioDevice(nullptr,0,&spec,nullptr,0);if(!audio_device)clay_fatal(SDL_GetError());clay::audio::title_audio_frequency(hz);SDL_PauseAudioDevice(audio_device,0);}
static void queue_audio(int16_t* samples,size_t count){std::vector<int16_t> pcm(count);for(size_t i=0;i+1<count;i+=2){pcm[i]=samples[i+1];pcm[i+1]=samples[i];}std::lock_guard lock(audio_mutex);if(!audio_device)clay_fatal("audio_frequency_missing");clay::audio::mix_title_audio(pcm);if(SDL_QueueAudio(audio_device,pcm.data(),count*2))clay_fatal(SDL_GetError());}
static size_t remaining_audio(){std::lock_guard lock(audio_mutex);return audio_device?SDL_GetQueuedAudioSize(audio_device)/4:0;}
static RspUcodeFunc* select_ucode(const OSTask* task){if(task->t.type!=M_AUDTASK)clay_fatal("unsupported_rsp_task",task->t.type);return clayfighter_audio_rsp;}
static void rsp_init(){recomp::rsp::constants_init();recomp::rsp::set_callbacks({select_ucode});}
static bool run_rsp(uint8_t* rdram,const OSTask* task){fprintf(stderr,"{\"event\":\"audio_task\",\"ucode\":\"%08X\"}\n",task->t.ucode);return recomp::rsp::run_task(rdram,task);}
static void count_vi(){++clay_vis;}
static void error(const char* text){clay_fatal(text);}
int main(int argc,char** argv){
 std::string rom="probe-output/base.z64",controls="config/controls.cfg",sc_assets,test_input;double seconds=0;bool family_override=false,controls_override=false,hidden=false;
 std::string pref_dir;
 try{
  char* executable_path=SDL_GetBasePath();
  if(!executable_path)throw std::runtime_error("Cannot locate portable game folder");
  const std::filesystem::path executable_directory(executable_path);SDL_free(executable_path);
  const auto root=clay::portable::installation_root(executable_directory);
  std::filesystem::path requested;
  for(int i=1;i+1<argc;i++)if(std::string(argv[i])=="--data-dir"||std::string(argv[i])=="--user-data"){requested=argv[++i];break;}
  pref_dir=clay::portable::settings_directory(root,requested).string()+"/";
  std::filesystem::current_path(root);
#ifndef _WIN32
  const auto cache=clay::portable::settings_directory(root,"local/cache");
  setenv("XDG_CACHE_HOME",cache.string().c_str(),1);
  const auto mesa=clay::portable::settings_directory(root,"local/cache/mesa");
  setenv("MESA_SHADER_CACHE_DIR",mesa.string().c_str(),1);
  const auto nvidia=clay::portable::settings_directory(root,"local/cache/nvidia");
  setenv("__GL_SHADER_DISK_CACHE_PATH",nvidia.string().c_str(),1);
#endif
 }catch(const std::exception& e){clay_fatal(e.what());}
 std::string display_error;clay::set_graphics_storage(pref_dir+"display.cfg");
 clay::native_preferences::initialize_storage(pref_dir+"native-options.cfg");
 clay::training::initialize_storage(pref_dir+"training.cfg");
 clay::training::initialize_stage_storage(pref_dir+"training-stage.cfg");
 if(!clay::load_graphics_options(pref_dir+"display.cfg",display_error))fprintf(stderr,"Display preferences: %s\n",display_error.c_str());
 try{
  for(int i=1;i<argc;i++){std::string arg=argv[i];if(arg=="--rom"&&i+1<argc)rom=argv[++i];else if(arg=="--seconds"&&i+1<argc)seconds=std::stod(argv[++i]);else if(arg=="--controls"&&i+1<argc){controls=argv[++i];controls_override=true;}else if((arg=="--data-dir"||arg=="--user-data")&&i+1<argc)++i;else if(arg=="--hidden")hidden=true;else if(arg=="--test-input"&&i+1<argc)test_input=argv[++i];else if(arg=="--sc-assets"&&i+1<argc)sc_assets=argv[++i];else if(arg=="--sc-demo-character"&&i+1<argc){std::string value=argv[++i];size_t used=0;sc_demo_character=std::stoi(value,&used);if(used!=value.size())clay_fatal("invalid_sc_demo_character");}else if(arg=="--graphics-profile"&&i+1<argc)clay::set_graphics_options(clay::graphics_profile(argv[++i]));else if(arg=="--family"&&i+1<argc){manual_family=argv[++i];family_override=true;}else if(arg=="--help"){puts("ClayFighter 64 v2.1 by djultra64\n--rom PATH --seconds N --controls PATH --family auto|xbox|playstation|switch\n--graphics-profile default|original|1080p120|4k120 --hidden --data-dir RELATIVE_PATH\n--sc-assets PATH; test only: --sc-demo-character 6|11|12|16\nKeyboard defaults: arrows move; Z primary, X secondary, Enter Start, Shift Z trigger, Q/E L/R, I/J/K/L C buttons.\nPortable settings: local/settings; --data-dir must stay inside the game folder.\nBindings and fallback controller family: config/controls.cfg");return 0;}else clay_fatal("unknown_or_incomplete_argument");}
 }catch(const std::exception& e){clay_fatal(e.what());}
 std::ifstream file(rom,std::ios::binary);if(!file)clay_fatal("rom_open");clay_rom.assign(std::istreambuf_iterator<char>(file),{});if(clay_rom.size()!=0xc00000||clay_sc::sha256(clay_rom)!="357f26c6cd72559c5c05e8c2df46369624630dbcb1c02b3b6f3338f6ac79af07")clay_fatal("unsupported_original_rom");
 if(sc_assets.empty()&&sc_demo_character!=-1)clay_fatal("sc_demo_requires_assets");
 if(!sc_assets.empty()){
  std::ifstream donor(sc_assets,std::ios::binary);if(!donor)clay_fatal("sc_assets_open");
  std::vector<uint8_t> image(std::istreambuf_iterator<char>(donor),{});
  try{if(image.size()==15439696)clay_roster::configure_image(image,sc_demo_character);else clay_sc::configure_asset_image(image,sc_demo_character);}catch(const std::exception& e){clay_fatal(e.what());}
  clay_rom=std::move(image);
  fprintf(stderr,"{\"event\":\"experimental_sc_assets_enabled\",\"demo_character\":%d,\"base_cpu\":\"original_63\"}\n",sc_demo_character);
 }
 std::string bio_error; if(!clay::bios::load("local/sc_bios/bios.bin",bio_error))fprintf(stderr,"Biographies: %s\n",bio_error.c_str());
 std::string title_audio_error; if(!clay::audio::load_title_audio("local/audio/title64.s16le",title_audio_error))fprintf(stderr,"Title announcement: %s; using original audio\n",title_audio_error.c_str());
 std::vector<uint8_t> ram(0x900000);auto* rdram=ram.data();for(uint32_t i=0x1000;i<0xf9980;i++)rdram[((i-0x1000)+0x400)^3]=clay_rom[i];
 MEM_W(0,S32(0x80000300))=1;MEM_W(0,S32(0x80000304))=1;MEM_W(0,S32(0x80000308))=S32(0xb0000000);MEM_W(0,S32(0x80000318))=0x800000;
 clay::GraphicsOptions graphics_options;
 try{graphics_options=clay::graphics_options();}catch(const std::exception& e){clay_fatal(e.what());}
 clay::input::configure_sdl();if(!test_input.empty())SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_AUDIO))clay_fatal(SDL_GetError());
 std::unique_ptr<clay::testing::InputScript> scripted_input;
 if(!test_input.empty())try{scripted_input=std::make_unique<clay::testing::InputScript>(test_input);}catch(const std::exception& e){clay_fatal(e.what());}
 clay::input::SdlInput input;if(scripted_input&&!std::getenv("CLAY_TEST_UNASSIGNED_PADS"))input.set_preferred_instances(scripted_input->instance_id(),scripted_input->instance_id(1));std::string controls_error;
 if(!input.mapper().load(controls,controls_error))clay_fatal(controls_error.c_str());
 const auto stored_controls=pref_dir+"controls.cfg";
 if(!controls_override&&std::ifstream(stored_controls))if(!input.mapper().load(stored_controls,controls_error))clay_fatal(controls_error.c_str());
 // Preserve the existing profile for P1 and use it as the P2 starting point.
 input.mapper(1)=input.mapper(0);
 const auto second_controls=pref_dir+"controls-p2.cfg";
 if(!controls_override&&std::ifstream(second_controls))if(!input.mapper(1).load(second_controls,controls_error))clay_fatal(controls_error.c_str());
 // --controls imports a profile; subsequent edits always save portably.
 clay::input::set_controls_storage(stored_controls);
 for(size_t a=0;a<clay::input::action_count;a++)if(SDL_GetScancodeFromName(input.mapper().binding(clay::input::Action(a)).key.c_str())==SDL_SCANCODE_UNKNOWN)clay_fatal("unknown_controls_key");
 if(family_override)try{input.mapper().set_family(clay::input::parse_family(manual_family));input.mapper(1).set_family(clay::input::parse_family(manual_family));}catch(const std::exception& e){clay_fatal(e.what());}
 std::string glyph_error;
 if(!clay::input::load_native_prompt_assets("config/controller_glyphs.json","assets/kenney-input-prompts-pixel/Tiles",glyph_error))clay_fatal(glyph_error.c_str());
 int window_width=int(graphics_options.width),window_height=int(graphics_options.height);SDL_Rect usable{};
 if(SDL_GetDisplayUsableBounds(0,&usable)==0&&usable.w>64&&usable.h>64){
  const double fit=std::min({1.0,double(usable.w-24)/window_width,double(usable.h-48)/window_height});
  window_width=std::max(320,int(window_width*fit));window_height=std::max(240,int(window_height*fit));
 }
 // Fit the desktop window while retaining the selected internal render height.
 auto* window=SDL_CreateWindow("ClayFighter 64 v2.1",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,window_width,window_height,SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|(hidden?SDL_WINDOW_HIDDEN:0));if(!window)clay_fatal(SDL_GetError());
 fprintf(stderr,"{\"event\":\"window_ready\",\"width\":%d,\"height\":%d,\"render_height_requested\":%u}\n",window_width,window_height,graphics_options.render_height);
 ultramodern::renderer::GraphicsConfig graphics{};graphics.api_option=ultramodern::renderer::GraphicsApi::Vulkan;ultramodern::renderer::set_graphics_config(graphics);
 ultramodern::set_callbacks({rsp_init,run_rsp},{clay_create_context},{queue_audio,remaining_audio,set_frequency}, {[]{},get_input,[](int,bool){},connected_device}, {},{count_vi,nullptr},{error},{});
 ultramodern::set_message_queue_control({});

#ifdef _WIN32
 SDL_SysWMinfo wm{};SDL_VERSION(&wm.version);if(!SDL_GetWindowWMInfo(window,&wm))clay_fatal(SDL_GetError());
 ultramodern::renderer::WindowHandle native_window{wm.info.win.window,GetCurrentThreadId()};
#else
 auto native_window=window;
#endif
 if(!hidden&&graphics_options.fullscreen)SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN_DESKTOP);
 update_input(window,input);
 ultramodern::preinit(rdram,native_window);fprintf(stderr,"{\"event\":\"native_entry\",\"entry\":\"80000400\"}\n");std::thread([rdram]{ultramodern::set_entrypoint_thread();recomp_context ctx{};ctx.status_reg=0x2000ff01;ctx.r29=S32(0x800fad80);ctx.r8=S32(0x80000450);ctx.f_odd=&ctx.f0.u32h;get_function(S32(0x80000400))(rdram,&ctx);}).detach();
 auto start=std::chrono::steady_clock::now(),next=start,last_gfx_change=start,last_poll_change=start;uint64_t watched_gfx=0,watched_polls=0;int exit_code=72;
 for(;;){if(scripted_input)scripted_input->tick();SDL_Event event;bool done=clay::main_menu::exit_requested();while(SDL_PollEvent(&event)){input.event(event);if(event.type==SDL_QUIT)done=true;}update_input(window,input);auto selected_display=clay::graphics_options();
 if(selected_display.fullscreen!=graphics_options.fullscreen&&!hidden)SDL_SetWindowFullscreen(window,selected_display.fullscreen?SDL_WINDOW_FULLSCREEN_DESKTOP:0);
 graphics_options=selected_display;auto now=std::chrono::steady_clock::now();auto current_gfx=clay_gfx.load(),current_polls=clay_polls.load();if(current_gfx!=watched_gfx){watched_gfx=current_gfx;last_gfx_change=now;}if(current_polls!=watched_polls){watched_polls=current_polls;last_poll_change=now;}if(now>=next){fprintf(stderr,"{\"event\":\"progress\",\"vi\":%llu,\"gfx\":%llu,\"polls\":%llu}\n",(unsigned long long)clay_vis.load(),(unsigned long long)clay_gfx.load(),(unsigned long long)clay_polls.load());next=now+std::chrono::seconds(5);}if(done||(seconds>0&&std::chrono::duration<double>(now-start).count()>=seconds)){
  bool stalled=!done&&seconds>=10&&(now-last_gfx_change>std::chrono::seconds(2)||now-last_poll_change>std::chrono::seconds(2));
  exit_code=clay_gfx.load()?(stalled?73:0):72;
  fprintf(stderr,"{\"event\":\"validation_end\",\"exit_code\":%d,\"stalled\":%s,\"gfx\":%llu,\"polls\":%llu}\n",exit_code,stalled?"true":"false",(unsigned long long)clay_gfx.load(),(unsigned long long)clay_polls.load());break;
 }SDL_Delay(4);}
 std::_Exit(exit_code);
}
