// Native options persistence fixtures by djultra64.
#include "native_preferences.h"
#include "recomp.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <vector>
static int music=-1,fx=-1;
extern "C" void func_8009C3D0(uint8_t*,recomp_context* c){music=int(c->r4);c->r2=123;}
extern "C" void func_8009C548(uint8_t* rdram,recomp_context* c){fx=int(c->r4);MEM_W(0,S32(0x800de164))=c->r4;}
int main(int argc,char**argv){
 assert(argc==2);const std::filesystem::path path=argv[1];std::filesystem::remove(path);
 std::vector<uint8_t> ram(0x900000);auto*rdram=ram.data();constexpr uint32_t p=0x80200000;
 MEM_W(0,S32(0x800ddd58))=p;
 auto defaults=[&]{auto o=clay::native_preferences::defaults();for(unsigned i=0;i<28;i++)MEM_H(0,S32(p+i*2))=o.values[i];};
 recomp_context c{};c.r2=789;defaults();
 clay::native_preferences::initialize_storage(path);clay::native_preferences::after_initialize(rdram,&c);
 assert(!std::filesystem::exists(path)); // Startup defaults do not overwrite absent or invalid files.
 for(unsigned i=0;i<28;i++)MEM_H(0,S32(p+i*2))=i>=4&&i<16?uint16_t((clay::native_preferences::defaults().values[i]+1)%6):0;
 MEM_H(0,S32(p+0x34))=16384;MEM_H(0,S32(p+2))=1;MEM_H(0,S32(p))=4;
 MEM_H(0,S32(p+0x36))=8192;MEM_H(0,S32(p+0x26))=0;MEM_H(0,S32(p+0x2c))=2;
 MEM_H(0,S32(p+8))=5;MEM_H(0,S32(p+0x14))=2;
 clay::native_preferences::options_changed(rdram,&c);assert(std::filesystem::exists(path));
 auto expected=clay::native_preferences::capture(rdram);defaults();
 clay::native_preferences::initialize_storage(path);clay::native_preferences::after_initialize(rdram,&c);
 assert(clay::native_preferences::capture(rdram)==expected);
 clay::native_preferences::audio_ready(rdram,&c);assert(music==16384&&fx==8192&&c.r2==789);
 defaults();clay::native_preferences::options_changed(rdram,&c); // Native Reset Defaults remains persistent.
 clay::native_preferences::initialize_storage(path);clay::native_preferences::after_initialize(rdram,&c);
 assert(clay::native_preferences::capture(rdram)==clay::native_preferences::defaults());
 {std::ofstream f(path);f<<"ClayFighter64NativeOptions 1\nmusic_volume 999999\nspillits 1\n";}
 defaults();clay::native_preferences::initialize_storage(path);clay::native_preferences::after_initialize(rdram,&c);
 assert(clay::native_preferences::capture(rdram)==clay::native_preferences::defaults());
 {std::ofstream f(path);f<<"ClayFighter64NativeOptions 999\n";}
 clay::native_preferences::Options loaded;std::string error;assert(!clay::native_preferences::load_options(path,loaded,error));
 auto invalid=clay::native_preferences::defaults();invalid.values[4]=6;assert(!clay::native_preferences::save_options(path,invalid,error));
 // A truncated or duplicated profile must be rejected as a whole.
 auto original=clay::native_preferences::defaults();assert(clay::native_preferences::save_options(path,original,error));
 {std::ofstream f(path,std::ios::app);f<<"spillits 1\n";}
 assert(!clay::native_preferences::load_options(path,loaded,error)&&loaded==original);
 {std::ofstream f(path);f<<"ClayFighter64NativeOptions 1\nspillits 1\n";}
 assert(!clay::native_preferences::load_options(path,loaded,error)&&loaded==original);
 assert(!clay::native_preferences::save_options(std::filesystem::current_path()/"../escaped-native-options.cfg",original,error));
 const auto second=std::filesystem::path(path.string()+".second");
 assert(clay::native_preferences::save_options(second,expected,error));
 assert(clay::native_preferences::save_options(path,original,error));
 assert(clay::native_preferences::load_options(second,loaded,error)&&loaded==expected);
 std::filesystem::remove(second);
 std::filesystem::remove(path);puts("Native options: round trip, music/FX application, reset defaults and invalid files passed");
}
