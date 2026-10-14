// Original sound bank relocation/dispatch bounds fixture by djultra64.
#include "recomp.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>
extern "C" void func_800C1A04(uint8_t*,recomp_context*);
extern "C" void clay_sound_selector(uint8_t*,recomp_context*);
extern "C" void func_800B8FC8(uint8_t*,recomp_context*){throw std::runtime_error("Original dispatch rejected bank/instrument index");}
int main(int argc,char**argv){assert(argc==2);std::ifstream file(argv[1],std::ios::binary);assert(file);std::vector<uint8_t>rom{std::istreambuf_iterator<char>(file),{}};std::vector<uint8_t>ram(9<<20);auto*rdram=ram.data();unsigned calls=0;
 for(auto source: {std::pair{0xa33c70u,0xa38ac0u},std::pair{0xb141b0u,0xb156b0u},std::pair{0xb2f060u,0xb30620u}}){
  constexpr uint32_t address=0x80200000;for(unsigned i=0;i<source.second-source.first;i++)MEM_BU(0,S32(address+i))=rom[source.first+i];
  recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=S32(0x807fe000);c.r4=S32(address);c.r5=source.second;func_800C1A04(rdram,&c);
  uint32_t bank=MEM_W(4,S32(address));unsigned instruments=MEM_HU(0,S32(bank));
  for(unsigned i=0;i<instruments;i++){uint32_t instrument=MEM_W(int32_t(12+i*4),S32(bank));assert(instrument>=address);unsigned sounds=MEM_HU(14,S32(instrument));
   for(unsigned j=0;j<sounds;j++){c.r4=S32(address);c.r5=0;c.r6=i;c.r7=j;clay_sound_selector(rdram,&c);uint32_t expected=MEM_W(int32_t(16+j*4),S32(instrument));assert(uint32_t(c.r2)==expected);assert(expected>=address&&expected<address+source.second-source.first);calls++;}
   c.r4=S32(address);c.r5=0;c.r6=i;c.r7=sounds;clay_sound_selector(rdram,&c);assert(c.r2==0);c.r4=S32(address);c.r5=0;c.r6=i;c.r7=S32(-1);clay_sound_selector(rdram,&c);assert(c.r2==0);
  }
  for(auto invalid: {std::pair{0u,instruments},std::pair{1u,0u},std::pair{0xffffffffu,0u},std::pair{0u,0xffffffffu}}){c.r4=S32(address);c.r5=invalid.first;c.r6=invalid.second;c.r7=0;bool rejected=false;try{clay_sound_selector(rdram,&c);}catch(const std::runtime_error&){rejected=true;}assert(rejected);c.r29=S32(0x807fe000);}
 }
 printf("PASS: original alBnkfNew relocation + original9B6B4 selector resolves%u voice/combat/menu sound IDs; allbank/instrument/sound boundarynegativecasesreject\n",calls);
}
