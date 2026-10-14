// User-provided private title announcement integration by djultra64.
#include "title_audio.h"
#include <fstream>
#include <algorithm>
#include <iterator>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <array>
#include <atomic>
#include <filesystem>
#include <stdexcept>
extern "C" void func_800C1520(uint8_t*,recomp_context*);
namespace clay::audio {
namespace {
constexpr uint32_t source_rate=32000;
TitleAudioMixer mixer;
std::atomic_bool replacement_cycle{false};
// Private optional diagnostic capture, never an audio output device.
class Capture {
 std::mutex mutex;std::fstream file;uint32_t hz=32000,opened_hz=0,bytes=0;bool attempted=false;
 void header(){
  std::array<uint8_t,44>h{};auto text=[&](unsigned p,const char*v){for(unsigned i=0;i<4;i++)h[p+i]=uint8_t(v[i]);};auto w=[&](unsigned p,uint32_t n){for(unsigned i=0;i<4;i++)h[p+i]=uint8_t(n>>(i*8));};
  text(0,"RIFF");w(4,36+bytes);text(8,"WAVE");text(12,"fmt ");w(16,16);h[20]=1;h[22]=2;w(24,opened_hz);w(28,opened_hz*4);h[32]=4;h[34]=16;text(36,"data");w(40,bytes);file.seekp(0);file.write(reinterpret_cast<const char*>(h.data()),h.size());file.seekp(0,std::ios::end);
 }
public:
 void frequency(uint32_t n){std::lock_guard lock(mutex);hz=n;}
 void record(std::span<int16_t>pcm){
  std::lock_guard lock(mutex);if(!attempted){attempted=true;const char*path=std::getenv("CLAY_CAPTURE_AUDIO");if(path&&*path){
   try {
    // main sets cwd to the installation before opening audio. Diagnostic writes
    // obey the same portability rule, including absolute paths and symlinks.
    const auto root=std::filesystem::canonical(std::filesystem::current_path());
    const auto requested=std::filesystem::path(path);
    const auto destination=std::filesystem::weakly_canonical(requested.is_absolute()?requested:root/requested);
    auto a=root.begin(),b=destination.begin();
    for(;a!=root.end();++a,++b)if(b==destination.end()||*a!=*b)
     throw std::invalid_argument("Audio capture must remain inside the game folder");
    file.open(destination,std::ios::binary|std::ios::in|std::ios::out|std::ios::trunc);
    if(file){opened_hz=hz;header();}else std::fprintf(stderr,"title_audio: cannot open private capture %s\n",path);
   }catch(const std::exception& error){std::fprintf(stderr,"title_audio: %s\n",error.what());}
  }}
  if(!file.is_open()||hz!=opened_hz||pcm.size()%2||uint64_t(bytes)+pcm.size()*2>128*1024*1024)return;
  std::vector<uint8_t>out(pcm.size()*2);for(size_t i=0;i<pcm.size();i++){out[i*2]=uint8_t(pcm[i]);out[i*2+1]=uint8_t(uint16_t(pcm[i])>>8);}file.write(reinterpret_cast<const char*>(out.data()),out.size());if(!file)return;bytes+=uint32_t(out.size());header();file.flush();
 }
} capture;
uint32_t word(uint8_t*rdram,uint32_t p){return uint32_t(MEM_W(0,S32(p)));}
bool valid(uint32_t p,size_t n){return p>=0x80000000u&&uint64_t(p)+n<=0x80800000ull;}
bool native_cue(uint8_t*rdram,recomp_context*c,uint32_t which){
 const uint32_t file=uint32_t(c->r4),sp=uint32_t(c->r29);
 if(c->r5||c->r6||uint32_t(c->r7)!=which||file!=word(rdram,0x800ff10c)||!valid(file,8)||!valid(sp,0x24))return false;
 if(MEM_BU(0,S32(file))!='B'||MEM_BU(1,S32(file))!='1'||MEM_HU(2,S32(file))!=1)return false;
 uint32_t bank=word(rdram,file+4);if(!valid(bank,16)||MEM_HU(0,S32(bank))!=1)return false;
 uint32_t inst=word(rdram,bank+12);if(!valid(inst,16+27*4)||MEM_HU(14,S32(inst))!=27)return false;
 uint32_t sound=word(rdram,inst+16+which*4);if(!valid(sound,16))return false;
 uint32_t wave=word(rdram,sound+8);return valid(wave,20)&&word(rdram,wave)==(which==0?0xb30620u:0xb39528u)&&word(rdram,wave+4)==(which==0?7750u:10890u)&&MEM_BU(8,S32(wave))==0;
}
}
bool TitleAudioMixer::load_pcm(const std::string&path,std::string&error){
 error.clear();std::ifstream in(path,std::ios::binary|std::ios::ate);
 auto fail=[&](const char*reason){error="Title announcement "+path+": "+reason;std::lock_guard lock(mutex);mono.clear();playing=false;phase=0;return false;};
 if(!in)return fail("cannot open PCM file; original announcement retained");
 const std::streamoff length=in.tellg();if(length<=0||length>source_rate*20*2||(length&1))return fail("expected nonempty mono signed16 littleendian 32000Hz PCM, at most20seconds");
 std::vector<uint8_t>bytes(size_t(length),0);in.seekg(0);if(!in.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size())))return fail("incomplete PCM read");
 std::vector<int16_t>samples(bytes.size()/2);for(size_t i=0;i<samples.size();i++)samples[i]=int16_t(uint16_t(bytes[i*2])|uint16_t(bytes[i*2+1])<<8);
 std::lock_guard lock(mutex);mono=std::move(samples);playing=false;phase=0;return true;
}
void TitleAudioMixer::set_frequency(uint32_t hz){if(hz<8000||hz>192000)return;std::lock_guard lock(mutex);frequency=hz;}
bool TitleAudioMixer::ready()const{std::lock_guard lock(mutex);return !mono.empty();}
bool TitleAudioMixer::active()const{std::lock_guard lock(mutex);return playing;}
bool TitleAudioMixer::trigger(uint16_t gain){std::lock_guard lock(mutex);if(mono.empty()||gain>32767)return false;if(!playing){phase=0;volume=gain;playing=true;}return true;}
void TitleAudioMixer::mix(std::span<int16_t>stereo){
 std::lock_guard lock(mutex);if(stereo.size()%2)return;const uint64_t start_frame=output_frames;output_frames+=stereo.size()/2;if(!playing||stereo.empty())return;
 if(!phase&&std::getenv("CLAY_TRACE_AUDIO"))std::fprintf(stderr,"{\"event\":\"title_audio_mix_start\",\"output_frame\":%llu,\"frequency\":%u,\"source_frames\":%zu}\n",static_cast<unsigned long long>(start_frame),frequency,mono.size());
 const uint64_t step=(uint64_t(source_rate)<<32)/frequency;
 for(size_t i=0;i<stereo.size();i+=2){
  size_t index=size_t(phase>>32);if(index>=mono.size()){playing=false;break;}
  int64_t sample=mono[index];if(index+1<mono.size())sample+=(int64_t(mono[index+1])-sample)*int64_t(uint32_t(phase))/int64_t(0x100000000ull);
  // Native cue is centered; equal-power center gain avoids doubling mono energy.
  const int32_t voice=int32_t(sample*volume*23170/(int64_t(32767)*32767));
  for(size_t channel=0;channel<2;channel++)stereo[i+channel]=int16_t(std::clamp(int32_t(stereo[i+channel])+voice,-32768,32767));
  phase+=step;
 }
 if((phase>>32)>=mono.size())playing=false;
}
bool load_title_audio(const std::string&path,std::string&error){replacement_cycle=false;return mixer.load_pcm(path,error);}
void title_audio_frequency(uint32_t hz){if(hz<8000||hz>192000)return;mixer.set_frequency(hz);capture.frequency(hz);}
void mix_title_audio(std::span<int16_t>stereo){mixer.mix(stereo);capture.record(stereo);}
void trace_native_sound(uint8_t*rdram,recomp_context*c){
 if(!std::getenv("CLAY_TRACE_AUDIO")||!rdram||!c)return;
 uint32_t file=uint32_t(c->r4),bank_index=uint32_t(c->r5),instrument_index=uint32_t(c->r6),sound_index=uint32_t(c->r7),wave=0;
 const char*name=file==word(rdram,0x800ff108)?"main":file==word(rdram,0x800ff10c)?"menu":file==word(rdram,0x801028c4)?"combat":"other";
 if(valid(file,8)&&bank_index<16&&MEM_HU(2,S32(file))>bank_index&&valid(file+4+bank_index*4,4)){
  uint32_t bank=word(rdram,file+4+bank_index*4),inst=0;
  if(valid(bank,12)){if(instrument_index==0xffffffff)inst=word(rdram,bank+8);else if(instrument_index<128&&MEM_HU(0,S32(bank))>instrument_index&&valid(bank+12+instrument_index*4,4))inst=word(rdram,bank+12+instrument_index*4);}
  if(valid(inst,16)&&sound_index<256&&MEM_HU(14,S32(inst))>sound_index&&valid(inst+16+sound_index*4,4)){uint32_t snd=word(rdram,inst+16+sound_index*4);if(valid(snd,16))wave=word(rdram,snd+8);}
 }
 uint32_t state=word(rdram,0x80118cf0);
 std::fprintf(stderr,"{\"event\":\"native_sound_request\",\"bank_family\":\"%s\",\"bank_file\":\"%08X\",\"bank\":%u,\"instrument\":%u,\"sound\":%u,\"wave_rom\":\"%08X\",\"wave_bytes\":%u,\"menu_state\":%u}\n",name,file,bank_index,instrument_index,sound_index,valid(wave,20)?word(rdram,wave):0,valid(wave,20)?word(rdram,wave+4):0,state);
}
void play_title_announcement(uint8_t*rdram,recomp_context*c,recomp_func_t*original){
 if(!rdram||!c)return;
 if(!mixer.ready()||!native_cue(rdram,c,0)){original(rdram,c);return;}
 const uint32_t master=word(rdram,0x800de164),requested=word(rdram,uint32_t(c->r29)+16);
 if(master>32767||requested>32767){original(rdram,c);return;}
 const auto gain=uint16_t(uint64_t(master)*requested/32767);
 if(!mixer.trigger(gain)){original(rdram,c);return;}
 replacement_cycle=true;
 // Original9BB34 consumes one native RNG value for this cue's pitch flag.
 // Keep the original game RNG progression while leaving the supplied voice intact.
 if(word(rdram,uint32_t(c->r29)+32)){recomp_context local=*c;local.f_odd=local.mips3_float_mode?&local.f1.u32l:&local.f0.u32h;func_800C1520(rdram,&local);}
 c->r2=0; // This exact caller ignores the native return value.
 if(std::getenv("CLAY_TRACE_AUDIO"))std::fprintf(stderr,"{\"event\":\"title_audio_replacement\",\"caller\":\"80065D74\",\"sound\":0,\"gain\":%u}\n",unsigned(gain));
}
void finish_title_announcement(uint8_t*rdram,recomp_context*c,recomp_func_t*original){
 if(!rdram||!c)return;
 if(!mixer.ready()||!native_cue(rdram,c,4)||!replacement_cycle.exchange(false)){original(rdram,c);return;}
 if(word(rdram,uint32_t(c->r29)+32)){recomp_context local=*c;local.f_odd=local.mips3_float_mode?&local.f1.u32l:&local.f0.u32h;func_800C1520(rdram,&local);}
 c->r2=0;
 if(std::getenv("CLAY_TRACE_AUDIO"))std::fprintf(stderr,"{\"event\":\"title_audio_original_tail_suppressed\",\"caller\":\"80066488\",\"sound\":4}\n");
}

}
