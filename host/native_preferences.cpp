// Persist only the verified original Options halfwords, never a RAM snapshot.
// Own work: djultra64.
#include "native_preferences.h"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif
extern "C" void func_8009C3D0(uint8_t*,recomp_context*);
extern "C" void func_8009C548(uint8_t*,recomp_context*);
namespace clay::native_preferences {
namespace {
// Order is the verified 0x38-byte allocation initialized by func_8005A918.
constexpr const char* keys[]={"difficulty","spillits","throws_and_holds","time_limit",
 "p1_wobbly_punch","p2_wobbly_punch","p1_fair_punch","p2_fair_punch",
 "p1_brutal_punch","p2_brutal_punch","p1_wobbly_kick","p2_wobbly_kick",
 "p1_fair_kick","p2_fair_kick","p1_brutal_kick","p2_brutal_kick",
 "stun_meter","super_meter","health_meter","character_picture",
 "choose_background","body_type","voice","camera_flights","claytality_timer",
 "fourth_pad_camera","music_volume","sfx_voice_volume"};
constexpr uint16_t maxima[]={4,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,1,1,1,1,1,2,2,1,1,1,32767,32767};
std::filesystem::path storage;
Options loaded,last_saved;
bool have_loaded=false,initialized=false,sequence_ready=false;
uint32_t pointer(uint8_t*rdram){
 if(!rdram)return 0;
 const auto p=uint32_t(MEM_W(0,S32(0x800ddd58)));
 return p>=0x80000400u&&uint64_t(p)+0x38<=0x80800000ull&&(p&3)==0?p:0;
}
bool valid(const Options&o){for(unsigned i=0;i<28;i++)if(o.values[i]>maxima[i])return false;return true;}
std::filesystem::path portable_file(const std::filesystem::path& path){
 if(path.filename().empty())throw std::invalid_argument("Missing native preferences filename");
 // Existing symlinks as well as ../ paths must remain within the installation.
 const auto destination=std::filesystem::weakly_canonical(path.is_absolute()?path:std::filesystem::current_path()/path);
 const auto root=std::filesystem::canonical(std::filesystem::current_path());
 auto a=root.begin(),b=destination.begin();
 for(;a!=root.end();++a,++b)if(b==destination.end()||*a!=*b)throw std::invalid_argument("Native Options must remain inside the game folder");
 return destination;
}
void apply_audio(uint8_t*rdram,recomp_context*c,const Options&o){
 if(!c)return;
 auto call=*c;call.f_odd=&call.f0.u32h; // Do not change the original function's register return state.
 if(sequence_ready){call.r4=o.values[26];func_8009C3D0(rdram,&call);}
 call=*c;call.f_odd=&call.f0.u32h;call.r4=o.values[27];func_8009C548(rdram,&call);
}
}
Options defaults(){return {{{1,0,1,1,2,2,1,1,0,0,5,5,4,4,3,3,1,1,1,1,0,1,1,1,1,0,32767,0x7333}}};}
Options capture(uint8_t*rdram){Options o=defaults();const auto p=pointer(rdram);if(p)for(unsigned i=0;i<28;i++)o.values[i]=MEM_HU(0,S32(p+i*2));return o;}
bool load_options(const std::filesystem::path&path,Options&o,std::string&error){
 error.clear();o=defaults();
 try{
  std::ifstream input(path);if(!input){if(std::filesystem::exists(path))error="Cannot read native Options";return false;}
  std::string magic;int version=0;if(!(input>>magic>>version)||magic!="ClayFighter64NativeOptions"||version!=1)throw std::runtime_error("Unsupported native Options version");
  std::string line;std::getline(input,line);Options candidate=defaults();std::array<bool,28> seen{};
  while(std::getline(input,line)){
   if(line.empty())continue;
   std::istringstream row(line);std::string key,extra;long value;
   if(!(row>>key>>value)||(row>>extra))throw std::runtime_error("Malformed native Options row");
   unsigned i=0;while(i<28&&key!=keys[i])++i;
   if(i==28||seen[i]||value<0||value>maxima[i])throw std::runtime_error("Invalid native Options value: "+key);
   candidate.values[i]=uint16_t(value);seen[i]=true;
  }
  if(!input.eof())throw std::runtime_error("Cannot read native Options");
  for(bool present:seen)if(!present)throw std::runtime_error("Incomplete native Options");
  o=candidate;return true;
 }catch(const std::exception&e){error=e.what();return false;}
}
bool save_options(const std::filesystem::path&path,const Options&o,std::string&error){
 error.clear();std::filesystem::path temp;
 try{
  if(!valid(o))throw std::invalid_argument("Invalid native Options");
  const auto destination=portable_file(path);temp=destination.string()+".tmp";
  // Validate the temporary path too: a pre-existing symlink must not escape.
  portable_file(temp);std::filesystem::create_directories(destination.parent_path());
  {std::ofstream output(temp);if(!output)throw std::runtime_error("Cannot write native Options");
   output<<"ClayFighter64NativeOptions 1\n";for(unsigned i=0;i<28;i++)output<<keys[i]<<' '<<o.values[i]<<'\n';
   output.flush();if(!output)throw std::runtime_error("Cannot write native Options");}
#ifdef _WIN32
  if(!MoveFileExW(temp.c_str(),destination.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot replace native Options");
#else
  std::filesystem::rename(temp,destination);
#endif
  return true;
 }catch(const std::exception&e){error=e.what();return false;}
}
void initialize_storage(const std::filesystem::path&path){
 storage.clear();initialized=sequence_ready=have_loaded=false;
 try{storage=portable_file(path);std::string error;have_loaded=load_options(storage,loaded,error);if(!error.empty())std::fprintf(stderr,"Native Options: %s; using original defaults\n",error.c_str());}
 catch(const std::exception&e){std::fprintf(stderr,"Native Options: %s\n",e.what());}
}
void after_initialize(uint8_t*rdram,recomp_context*c){
 const auto p=pointer(rdram);if(!p)return;
 if(initialized){options_changed(rdram,c);return;} // An explicit later reset must remain a reset.
 if(have_loaded)for(unsigned i=0;i<28;i++)MEM_H(0,S32(p+i*2))=loaded.values[i];
 last_saved=capture(rdram);initialized=true;apply_audio(rdram,c,last_saved);
 std::fprintf(stderr,"{\"event\":\"native_options_ready\",\"restored\":%s,\"music_volume\":%u,\"sfx_voice_volume\":%u,\"spillits\":%u}\n",have_loaded?"true":"false",last_saved.values[26],last_saved.values[27],last_saved.values[1]);
}
void options_changed(uint8_t*rdram,recomp_context*c){
 if(!initialized||!pointer(rdram))return;
 const auto current=capture(rdram);if(current==last_saved||!valid(current))return;
 if(current.values[26]!=last_saved.values[26]||current.values[27]!=last_saved.values[27])apply_audio(rdram,c,current);
 if(storage.empty())return;
 std::string error;if(save_options(storage,current,error)){
  last_saved=current;std::fprintf(stderr,"{\"event\":\"native_options_saved\",\"music_volume\":%u,\"sfx_voice_volume\":%u,\"spillits\":%u}\n",current.values[26],current.values[27],current.values[1]);
 }else std::fprintf(stderr,"Native Options: %s\n",error.c_str());
}
void audio_ready(uint8_t*rdram,recomp_context*c){sequence_ready=true;if(initialized&&pointer(rdram))apply_audio(rdram,c,capture(rdram));}
}
