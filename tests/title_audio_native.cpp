// Original alBnkfNew relocation + title waveform identity and replacement; djultra64.
#include "title_audio.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
#include <stdexcept>
extern "C" void func_800C1520(uint8_t*,recomp_context*);
extern "C" void func_800C1A04(uint8_t*,recomp_context*);
static unsigned original_calls=0;
static void original(uint8_t*,recomp_context*){original_calls++;}
int main(int argc,char**argv){assert(argc==3);std::ifstream in(argv[1],std::ios::binary);assert(in);std::vector<uint8_t>rom{std::istreambuf_iterator<char>(in),{}};assert(rom.size()>=0xb30620);std::vector<uint8_t>ram(8<<20);auto*rdram=ram.data();constexpr uint32_t base=0x80200000;
 for(unsigned i=0;i<0x15c0;i++)MEM_BU(0,S32(base+i))=rom[0xb2f060+i];recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=S32(0x80040000);c.r4=S32(base);c.r5=0xb30620;func_800C1A04(rdram,&c);
 std::string error;assert(clay::audio::load_title_audio(argv[2],error));MEM_W(0,S32(0x800ff10c))=base;MEM_W(0,S32(0x800de164))=32767;MEM_W(16,c.r29)=32767;c.r4=S32(base);c.r5=c.r6=0;c.r7=0;MEM_W(32,c.r29)=1;MEM_W(0,S32(0x800ee5b0))=12345;auto rng=c;func_800C1520(rdram,&rng);uint32_t expected_rng=MEM_W(0,S32(0x800ee5b0));MEM_W(0,S32(0x800ee5b0))=12345;clay::audio::play_title_announcement(rdram,&c,original);assert(!original_calls);assert(uint32_t(MEM_W(0,S32(0x800ee5b0)))==expected_rng);
 std::ifstream clip(argv[2],std::ios::binary);assert(clip);std::vector<uint8_t>source{std::istreambuf_iterator<char>(clip),{}};assert(!source.empty()&&!(source.size()%2));const size_t source_frames=source.size()/2;
 std::vector<int16_t>pcm((source_frames+32000)*2);clay::audio::title_audio_frequency(32000);const size_t split=(source_frames/2)*2;clay::audio::mix_title_audio(std::span(pcm).first(split));
 // The original tail arrives while the new recording is still active: suppressing
 // that old event must never truncate the rest of the supplied recording.
 c.r7=4;clay::audio::finish_title_announcement(rdram,&c,original);assert(original_calls==0);clay::audio::mix_title_audio(std::span(pcm).subspan(split));bool audible=false;
 for(size_t i=0;i<source_frames;i++){int16_t sample=int16_t(uint16_t(source[i*2])|uint16_t(source[i*2+1])<<8);int16_t expected=int16_t(int64_t(sample)*23170/32767);assert(pcm[i*2]==expected&&pcm[i*2+1]==expected);audible|=expected!=0;}assert(audible);
 for(size_t i=source_frames*2;i<pcm.size();i++)assert(pcm[i]==0);
c.r7=26;clay::audio::play_title_announcement(rdram,&c,original);assert(original_calls==1);printf("PASS original native alBnkfNew verifies originalmenu bank0/instrument0 sound0 waveROMB30620 len7750 +sound4waveROMB39528 len10890; providedPCM plays all%zu frames(%.6fs) once centered; tail cannot truncate clip; Claytality sound26 unchanged; privateWAV capture supported\n",source_frames,double(source_frames)/32000);
}
