// Original event scoping and additive stereo DSP tests by djultra64.
#include "title_audio.h"
#include <cassert>
#include <array>
#include <fstream>
#include <vector>
#include <cstdio>
#include <filesystem>
static unsigned calls=0,rng_calls=0;
extern "C" void func_800C1520(uint8_t*,recomp_context*){rng_calls++;}
static void original(uint8_t*,recomp_context*){calls++;}
int main(int argc,char**argv){assert(argc==2);const auto path=std::filesystem::path(argv[1]);{std::ofstream f(path,std::ios::binary);for(int16_t s:{int16_t(1000),int16_t(-1000),int16_t(32767),int16_t(-32768)}){f.put(char(uint16_t(s)&255));f.put(char(uint16_t(s)>>8));}}
 clay::audio::TitleAudioMixer m;std::string error;assert(!m.load_pcm(path.string()+".missing",error));assert(!error.empty());assert(!m.ready());assert(m.load_pcm(path.string(),error));
 std::array<int16_t,8> base{100,200,300,400,30000,31000,-30000,-31000};auto untouched=base;m.mix(base);assert(base==untouched);assert(m.trigger(32767));assert(m.trigger(32767));m.mix(base);assert(!m.active());assert(base[0]-untouched[0]==base[1]-untouched[1]);assert(base[0]>untouched[0]);assert(base[2]<untouched[2]);assert(base[4]==32767&&base[5]==32767&&base[6]==-32768&&base[7]==-32768);auto ended=base;m.mix(base);assert(base==ended);
 m.set_frequency(64000);assert(m.trigger(0));std::array<int16_t,16> muted{};muted.fill(777);m.mix(muted);for(auto s:muted)assert(s==777);assert(!m.active());assert(m.trigger(32767));std::array<int16_t,16> split{},whole{};m.mix(std::span(split).first(6));m.mix(std::span(split).subspan(6));assert(m.trigger(32767));m.mix(whole);assert(split==whole);
 std::vector<uint8_t>ram(8<<20);auto*rdram=ram.data();recomp_context c{};c.r29=S32(0x80040000);c.r4=S32(0x80200000);c.r5=c.r6=0;c.r7=0;c.r2=55;MEM_W(0,S32(0x800ff10c))=c.r4;MEM_W(0,S32(0x800de164))=32767;MEM_W(16,c.r29)=32767;
 clay::audio::play_title_announcement(rdram,&c,original);assert(calls==1);assert(clay::audio::load_title_audio(path.string(),error));clay::audio::play_title_announcement(rdram,&c,original);assert(calls==2); // Malformed bank remains original.
 constexpr uint32_t file=0x80200000,bank=file+32,inst=file+64,snd=file+256,wave=file+288;MEM_BU(0,S32(file))='B';MEM_BU(1,S32(file))='1';MEM_H(2,S32(file))=1;MEM_W(4,S32(file))=bank;MEM_H(0,S32(bank))=1;MEM_W(12,S32(bank))=inst;MEM_H(14,S32(inst))=27;MEM_W(16,S32(inst))=snd;MEM_W(8,S32(snd))=wave;MEM_W(0,S32(wave))=0xb30620;MEM_W(4,S32(wave))=7750;MEM_BU(8,S32(wave))=0;
 clay::audio::play_title_announcement(rdram,&c,original);assert(calls==2&&c.r2==0&&rng_calls==0);MEM_W(32,c.r29)=1;clay::audio::play_title_announcement(rdram,&c,original);assert(calls==2&&rng_calls==1);
 for(unsigned other:{1u,5u,26u}){c.r7=other;clay::audio::play_title_announcement(rdram,&c,original);}assert(calls==5);c.r7=0;c.r6=1;clay::audio::play_title_announcement(rdram,&c,original);assert(calls==6);c.r6=0;MEM_W(0,S32(0x800de164))=40000;clay::audio::play_title_announcement(rdram,&c,original);assert(calls==7);
 c.r6=0;MEM_W(0,S32(0x800de164))=32767;MEM_W(16+4*4,S32(inst))=snd+64;MEM_W(8,S32(snd+64))=wave+64;MEM_W(0,S32(wave+64))=0xb39528;MEM_W(4,S32(wave+64))=10890;MEM_BU(8,S32(wave+64))=0;c.r7=4;clay::audio::finish_title_announcement(rdram,&c,original);assert(calls==7);clay::audio::finish_title_announcement(rdram,&c,original);assert(calls==8);
 std::filesystem::remove(path);printf("PASS additive stereo mix: inactivebyteexact, centerpan, saturating±boundaries, native mute, duplicatecue, fractionalrate/blockinvariance; missingasset/malformedbank/wrongIDs/instrument/gain forward original\n");
}
