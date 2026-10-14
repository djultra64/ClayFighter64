// Preserve the original stage/VS panel and its cleanup, by djultra64.
#include "training_stage.h"
#include "main_menu.h"
#include "test_input.h"
#include "training_menu_ui.h"
#include "native_prompts.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif
namespace clay::training {
namespace {
constexpr uint32_t game_slot=0x800ddd40,stage_slot=0x800ddd48,options_slot=0x800ddd58;
constexpr uint32_t primary_nav=0x80118d74,secondary_nav=0x80118d84,primary_held=0x80118cb0;
uint32_t panel=0;
bool waiting=false,release=true,confirmed=false,back=false;
int last_stage=-1;
std::filesystem::path storage;
bool valid(uint32_t p,unsigned bytes){return p>=0x80000000u&&(p&3)==0&&uint64_t(p)+bytes<=0x80800000ull;}
int32_t word(uint8_t* rdram,uint32_t p){return MEM_W(0,S32(p));}
void put(uint8_t* rdram,uint32_t p,int32_t value){MEM_W(0,S32(p))=value;}
uint32_t game(uint8_t* rdram){const uint32_t p=word(rdram,game_slot);return valid(p,0xf8)?p:0;}
bool active(uint8_t* rdram){return waiting&&main_menu::training_requested()&&valid(panel,0x14)&&uint32_t(word(rdram,stage_slot))==panel&&game(rdram);}
bool stage_valid(int value){return value>=0&&value<26;}
std::filesystem::path portable_file(const std::filesystem::path& requested){
 const auto root=std::filesystem::canonical(std::filesystem::current_path());
 const auto result=std::filesystem::weakly_canonical(requested.is_absolute()?requested:root/requested);
 auto a=root.begin(),b=result.begin();
 for(;a!=root.end();++a,++b)if(b==result.end()||*a!=*b)
  throw std::runtime_error("Training stage preferences must remain inside the game folder");
 return result;
}
void save(){
 if(storage.empty())return;
 std::filesystem::path temporary;
 try{
  const auto path=portable_file(storage);temporary=portable_file(path.string()+".tmp");
  std::filesystem::create_directories(path.parent_path());
  {std::ofstream output(temporary);output<<"ClayFighter64TrainingStage 1\nstage "<<last_stage<<'\n';
   output.flush();if(!output)throw std::runtime_error("Cannot write Training stage preferences");}
#ifdef _WIN32
  if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
   throw std::runtime_error("Cannot replace Training stage preferences");
#else
  std::filesystem::rename(temporary,path);
#endif
 }catch(const std::exception& error){std::fprintf(stderr,"Training stage preferences: %s\n",error.what());
  if(!temporary.empty()){std::error_code ignored;std::filesystem::remove(temporary,ignored);}}
}
}
void initialize_stage_storage(const std::filesystem::path& path){
 storage.clear();last_stage=-1;waiting=confirmed=back=false;release=true;panel=0;
 try{
  storage=portable_file(path);std::ifstream input(storage);if(!input)return;
  std::string magic;int version=0;
  if(!(input>>magic>>version)||magic!="ClayFighter64TrainingStage"||version!=1)return;
  std::string line;std::getline(input,line);
  while(std::getline(input,line)){std::istringstream row(line);std::string key;int value;
   if(row>>key>>value&&key=="stage"&&stage_valid(value))last_stage=value;}
 }catch(const std::exception& error){storage.clear();std::fprintf(stderr,"Training stage preferences: %s\n",error.what());}
}
void stage_panel_started(uint8_t* rdram){
 if(!main_menu::training_requested())return;
 const uint32_t state=word(rdram,stage_slot),configuration=game(rdram);
 if(!valid(state,0x14)||!configuration)return;
 panel=state;waiting=true;release=true;confirmed=back=false;
 if(stage_valid(last_stage))put(rdram,configuration+8,last_stage);
}
void stage_panel_tick(uint8_t* rdram){
 if(!active(rdram)||confirmed||back)return;
 // Original 6D544 accepts confirmation only at timer >=101. Holding101 keeps
 // its existing intro intact and prevents 6D77C's automatic deadline at300.
 if(word(rdram,panel)>=101){put(rdram,panel,101);testing::publish_input_phase(testing::InputPhase::StageSelect);}
}
void stage_input(uint8_t* rdram,recomp_context* ctx,recomp_func_t* original){
 if(!active(rdram)){original(rdram,ctx);return;}
 const uint32_t options=word(rdram,options_slot);
 if(!valid(options,0x2a)){original(rdram,ctx);return;}
 // Native 6D544 gates stage navigation on Options+28 if a CPU flag is set.
 // Temporarily open that path without changing saved settings or fighter flags.
 struct Restore {uint8_t* rdram;uint32_t options;uint16_t value;
  ~Restore(){MEM_H(0,S32(options+0x28))=value;}} restore{rdram,options,MEM_HU(0,S32(options+0x28))};
 MEM_H(0,S32(options+0x28))=1;
 original(rdram,ctx);
}
void stage_input_polled(uint8_t* rdram){
 if(!active(rdram)||confirmed||back)return;
 // The original handler aggregates these after the poll. Only P1 navigates
 // Training; preserve its real menu remapping and translate logical D-pad.
 uint32_t navigation=word(rdram,primary_nav);
 put(rdram,secondary_nav,0);
 if(release){if(word(rdram,primary_held))navigation=0;else release=false;}
 if(word(rdram,panel)<101)navigation=0;
 if(navigation&0x4000){
  back=true;navigation=0;
  // 6DAE0 exits when panel+8==1, then runs original 6D04C/preview cleanup.
  put(rdram,panel+8,1);
 }else if(navigation&0x9000){
  confirmed=true;navigation&=~0x30fu; // Confirm takes precedence over navigation.
 }else{
  const bool left=(navigation&0x200)!=0,right=(navigation&0x100)!=0;
  navigation&=~0x300u;
  if(left!=right)navigation|=left?2u:1u;
 }
 put(rdram,primary_nav,int32_t(navigation));
}
void stage_panel_draw(uint8_t* rdram,recomp_context* ctx){
 if(!active(rdram)||word(rdram,panel)<101)return;
 const auto font=uint32_t(word(rdram,game(rdram)+0x6914));if(!valid(font,0x28))return;
 std::vector<std::string> lines={input::menu_marker(input::Action::Left)+" "+input::menu_marker(input::Action::Right)+" change  "+input::menu_marker(input::Action::Primary)+" select  "+input::menu_marker(input::Action::Secondary)+" back"};
 draw_training_lines(rdram,ctx,lines,-214,20,620,0x80870000,font);
}
void stage_panel_finished(uint8_t* rdram){
 // The native cleanup has already cleared stage_slot at this point.
 if(waiting&&main_menu::training_requested()&&game(rdram)&&confirmed&&!back){const int selected=word(rdram,game(rdram)+8);
  if(stage_valid(selected)){last_stage=selected;save();}}
 waiting=false;panel=0;
}
bool take_stage_back(){const bool result=back;back=false;return result;}
}
