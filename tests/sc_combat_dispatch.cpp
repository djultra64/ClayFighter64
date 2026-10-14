// Original guest special-record dispatch fixture by djultra64.
#include "recomp.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>
extern "C" void func_80083A10(uint8_t*,recomp_context*);
extern "C" void func_8008307C(uint8_t*,recomp_context*);
extern "C" void do_break(uint32_t){std::abort();}
extern "C" void switch_error(const char*,uint32_t,uint32_t){std::abort();}
#ifdef CLAY_COMPARE_SC
extern "C" void sc_action_resolve(uint8_t*,recomp_context*);
extern "C" void sc_special_resolve(uint8_t*,recomp_context*);
#endif
static uint32_t be32(const std::vector<uint8_t>& b,size_t o){return(uint32_t(b.at(o))<<24)|(uint32_t(b.at(o+1))<<16)|(uint32_t(b.at(o+2))<<8)|b.at(o+3);}
int main(int argc,char** argv){
 if(argc!=2&&argc!=3)return 1;std::ifstream f(argv[1],std::ios::binary);std::vector<uint8_t> b(std::istreambuf_iterator<char>(f),{}),ram(0x800000);auto*rdram=ram.data();
 constexpr int32_t definition=S32(0x80100000),actor=S32(0x80010000),stack=S32(0x807ff000);
 for(size_t n=0;n<b.size();n++)MEM_BU(n,definition)=b[n];
 MEM_W(0x1d4,actor)=definition;
 uint32_t count=be32(b,0x18),offset=be32(b,0x1c);MEM_W(0x1e8,actor)=count;MEM_W(0x1ec,actor)=S32(uint32_t(definition)+offset);
 uint32_t expected=uint32_t(definition)+offset;
 for(uint32_t index=0;index<count;index++){
  recomp_context c{};c.r4=actor;c.r5=index;c.r29=stack;c.f_odd=&c.f0.u32h;func_80083A10(rdram,&c);
  #ifdef CLAY_COMPARE_SC
  recomp_context sc{};sc.r4=actor;sc.r5=index;sc.r29=stack;sc.f_odd=&sc.f0.u32h;sc_special_resolve(rdram,&sc);if(sc.r2!=c.r2)return 8;
#endif
  if(uint32_t(c.r2)!=expected||c.r29!=uint64_t(S32(stack)))return 2;
  uint32_t position=expected-uint32_t(definition),length=be32(b,position);
  if(length<48||uint64_t(position)+length>b.size())return 3;
  uint16_t pattern_count=(uint16_t(b[position+0x2c])<<8)|b[position+0x2d],pattern_offset=(uint16_t(b[position+0x2e])<<8)|b[position+0x2f];
  if(pattern_count&&uint64_t(pattern_offset)+uint64_t(pattern_count)*12>length)return 4;
  expected+=length;
 }
 if(argc==3){std::ifstream other(argv[2],std::ios::binary);std::vector<uint8_t> wrong(std::istreambuf_iterator<char>(other),{});constexpr int32_t wrong_definition=S32(0x80200000);for(size_t n=0;n<wrong.size();n++)MEM_BU(n,wrong_definition)=wrong[n];MEM_W(0x1d4,actor)=wrong_definition;}
 unsigned empty=0,populated=0;
 constexpr unsigned category_offsets[]{0x30,0x38,0x48,0x40,0x50,0x58,0x60,0x68,0x70,0x78,0x80,0x88,0x90,0x98};
 for(unsigned category=0;category<14;category++){
  int choices=int8_t(b.at(category_offsets[category]));if(choices<0||choices>7)return 5;
  for(unsigned sample=0;sample<128;sample++){
   MEM_W(0,S32(0x800ee5b0))=sample*7919;recomp_context c{};c.r4=actor;c.r5=9000+category;c.r29=stack;c.f_odd=&c.f0.u32h;func_8008307C(rdram,&c);
   #ifdef CLAY_COMPARE_SC
   MEM_W(0,S32(0x800c0f00))=sample*7919;recomp_context sc{};sc.r4=actor;sc.r5=9000+category;sc.r29=stack;sc.f_odd=&sc.f0.u32h;sc_action_resolve(rdram,&sc);if(sc.r2!=c.r2)return 9;
#endif
   if(!choices){if(int32_t(c.r2)!=-1){fprintf(stderr,"empty_category=%u result=%d choices=%d\n",category,int32_t(c.r2),choices);return 6;}}else{bool found=false;for(int i=0;i<choices;i++)if(int32_t(c.r2)==100+int8_t(b.at(category_offsets[category]+1+i)))found=true;if(!found)return 7;}
  }
  if(choices)populated++;else empty++;
 }
 printf("Original8307C+originalRNG resolved14 actual donor AI/action categories (populated%u empty%u) without defaultfighter substitution\n",populated,empty);
 printf("Original83A10 resolved all%u actual donor special records; definition-relative dispatch and input record bounds preserved\n",count);
}
