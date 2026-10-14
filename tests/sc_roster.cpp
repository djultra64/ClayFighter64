// Verified native portrait/selector buffer tests; djultra64.
#include "../host/sc_roster.h"
#include "../host/sc_assets.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
extern "C" void func_80002050(uint8_t*,recomp_context*);
extern "C" void func_80001E90(uint8_t*,recomp_context*);
extern "C" void func_8005AB2C(uint8_t*,recomp_context*);
extern "C" void func_8005AC38(uint8_t*,recomp_context*);
extern "C" void func_8005FC68(uint8_t*,recomp_context*);
extern "C" void func_8005D484(uint8_t*,recomp_context*);
extern "C" void sc_roster_original_lookup(uint8_t*,recomp_context*);
extern "C" void sc_roster_selector_tick(uint8_t*,recomp_context*);
extern "C" void sc_roster_portrait_lookup(uint8_t*,recomp_context*);
extern "C" void sc_roster_animation_tick(uint8_t*,recomp_context*);
static std::vector<uint8_t> rom;
static int hovered[2];
static uint32_t heap=0x80200000;
static std::map<uint32_t,uint32_t> handles;
static std::vector<uint32_t> released;
static std::map<uint32_t,uint32_t> live;static uint64_t live_bytes=0,peak_bytes=0;
static std::vector<std::pair<uint32_t,uint32_t>> allocations;
static uint32_t allocate(uint8_t*rdram,uint32_t n){uint32_t p=heap;heap=(heap+n+128)&~63u;if(heap>=0x807F0000)throw std::runtime_error("Fixture heap bounds");allocations.push_back({p,n});live[p]=n;live_bytes+=n;peak_bytes=std::max(peak_bytes,live_bytes);for(unsigned i=0;i<32;i++)MEM_B(n+i,gpr(S32(p)))=0xA5;return p;}
extern "C" {
void func_8005D334(uint8_t*rdram,recomp_context*){hovered[0]=MEM_W(0xC,gpr(S32(MEM_W(0,gpr(S32(0x800DDD40))))));}
void func_8005D3D4(uint8_t*rdram,recomp_context*){hovered[1]=MEM_W(0x10,gpr(S32(MEM_W(0,gpr(S32(0x800DDD40))))));}
void func_800A30C0(uint8_t*,recomp_context*){}
void func_8009B6B4(uint8_t*,recomp_context*){}
void func_800A0CA8(uint8_t*,recomp_context*){}
void sc_roster_unsupported(const char*s){throw std::runtime_error(s);}
void do_break(uint32_t){throw std::runtime_error("MIPS break");}
recomp_func_t*get_function(int32_t address){if(uint32_t(address)==0x80002050)return func_80002050;if(uint32_t(address)==0x80001E90)return func_80001E90;throw std::runtime_error("Unexpected native callback");}
void func_8009E36C(uint8_t*,recomp_context*){}
void func_800B9154(uint8_t*,recomp_context*){throw std::runtime_error("Actual guest fatal");}
void func_800B8FC8(uint8_t*,recomp_context*){throw std::runtime_error("Actual interpreter fatal");}
void switch_error(const char*,uint32_t,uint32_t){throw std::runtime_error("MIPS switch");}
void func_800B3C70(uint8_t*rdram,recomp_context*c){c->r2=S32(allocate(rdram,c->r5));}
void func_800B4408(uint8_t*,recomp_context*c){uint32_t p=c->r5;if(!p)return;released.push_back(p);auto it=live.find(p);if(it==live.end())throw std::runtime_error("Invalid free");live_bytes-=it->second;live.erase(it);}
void func_800B474C(uint8_t*rdram,recomp_context*c){uint32_t h=c->r5,p=handles.at(h);handles.erase(h);c->r5=gpr(S32(p));func_800B4408(rdram,c);c->r5=gpr(S32(h));func_800B4408(rdram,c);}
void func_800B59D0(uint8_t*,recomp_context*c){c->r2=60;}
void func_800B51F4(uint8_t*,recomp_context*){}
void func_800B4040(uint8_t*rdram,recomp_context*c){uint32_t p=allocate(rdram,c->r5),h=allocate(rdram,4);handles[h]=p;c->r2=gpr(S32(h));}
void func_800B6A00(uint8_t*,recomp_context*c){c->r2=S32(handles.at(uint32_t(c->r5)));}
void func_800B6E74(uint8_t*,recomp_context*){}
void func_80097E54(uint8_t*rdram,recomp_context*c){uint32_t dest=c->r4,src=c->r5,n=c->r6;if(uint64_t(src)+n>rom.size()||(dest&0x7fffff)+n>0x800000)throw std::runtime_error("DMA bounds");for(uint32_t i=0;i<n;i++)MEM_B(i,gpr(S32(dest)))=rom[src+i];}
}
static std::vector<uint8_t> file(const char*p){std::ifstream f(p,std::ios::binary);return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});}
static int navigation(uint8_t*rdram,uint32_t state,uint32_t profile,int columns,int state_bytes,const int*ids){
 recomp_context c{};
 constexpr unsigned directions[]{0x200,0x100,0x800,0x400};
 for(unsigned player=0;player<2;player++)for(unsigned slot=0;slot<unsigned(columns*4);slot++)for(unsigned direction=0;direction<4;direction++){
  for(unsigned i=0;i<unsigned(state_bytes);i++)MEM_B(i,gpr(S32(state)))=0;
  for(unsigned i=0;i<0x100;i++)MEM_B(i,gpr(S32(profile)))=0;
  for(unsigned i=0;i<0x100;i++)MEM_B(i,gpr(S32(0x80118CB0)))=0;
  MEM_H(0x20,gpr(S32(profile)))=7;
  MEM_W(0,gpr(S32(state)))=player?1:2;
  MEM_W(0x78+player*8,gpr(S32(state)))=slot%columns;MEM_W(0x7C+player*8,gpr(S32(state)))=slot/columns;
  MEM_W(4+player*24,gpr(S32(0x80118CB0)))=directions[direction];hovered[0]=hovered[1]=-1;
  c={};c.f_odd=&c.f0.u32h;c.r29=gpr(S32(0x807FF000));func_8005FC68(rdram,&c);
  int col=slot%columns,row=slot/columns;if(direction<2)col=(col+(direction==0?columns-1:1))%columns;else row=(row+(direction==2?3:1))%4;
  unsigned actualcol=MEM_W(0x78+player*8,gpr(S32(state))),actualrow=MEM_W(0x7C+player*8,gpr(S32(state)));
  if(actualcol!=unsigned(col)||actualrow!=unsigned(row)){fprintf(stderr,"nav mismatch P%u slot%u direction%u actual%u,%u expected%d,%d\n",player+1,slot,direction,actualcol,actualrow,col,row);return 23;}
  MEM_W(0x10,gpr(S32(state)))=1;MEM_W(4+player*24,gpr(S32(0x80118CB0)))=0x8000;
  c={};c.f_odd=&c.f0.u32h;c.r29=gpr(S32(0x807FF000));func_8005FC68(rdram,&c);
  if(hovered[player]!=ids[row*columns+col]){fprintf(stderr,"ID mismatch P%u slot%u got%d expected%d\n",player+1,slot,hovered[player],ids[row*columns+col]);return 24;}
 }
 return 0;
}
int main(int argc,char**argv){
 if(argc!=3)return 1;rom=file(argv[1]);auto base=file(argv[2]);auto*rdram=static_cast<uint8_t*>(calloc(1,0x800000));
 uint32_t profile=allocate(rdram,0x100);MEM_W(0,gpr(S32(0x800DDD40)))=gpr(S32(profile));MEM_H(0x20,gpr(S32(profile)))=7;
 recomp_context filter{};filter.f_odd=&filter.f0.u32h;filter.r29=gpr(S32(0x807FF000));
 for(int id:{6,11,16}){filter.r4=id;func_8005D484(rdram,&filter);if(filter.r2==unsigned(id))return 25;}
 if(clay_sc_roster_count()!=12||clay_sc_roster_columns()!=3||clay_sc_roster_state_bytes()!=0xB8||clay_sc_roster_flag_offset()!=0x2C)return 2;
 constexpr int old[]{0,1,2,10,15,8,14,5,9,13,4,3};for(unsigned i=0;i<12;i++)if(clay_sc_roster_id_from_slot(i)!=old[i])return 3;
 uint32_t original_state=allocate(rdram,0xB8);MEM_W(0,gpr(S32(0x800DDD30)))=gpr(S32(original_state));if(int result=navigation(rdram,original_state,profile,3,0xB8,old))return result;
 for(unsigned first=0;first<12;first++)for(unsigned second=0;second<12;second++){
  MEM_W(0xC,gpr(S32(profile)))=old[first];MEM_W(0x10,gpr(S32(profile)))=old[second];filter={};filter.f_odd=&filter.f0.u32h;filter.r29=gpr(S32(0x807FF000));sc_roster_original_lookup(rdram,&filter);
  if(MEM_W(0x78,gpr(S32(original_state)))!=int(first%3)||MEM_W(0x7C,gpr(S32(original_state)))!=int(first/3)||MEM_W(0x80,gpr(S32(original_state)))!=int(second%3)||MEM_W(0x84,gpr(S32(original_state)))!=int(second/3))return 29;
 }

 if(clay_sc_roster_coordinates(rdram)!=gpr(S32(0x800DABE8)))return 4;
 auto bad=rom;bad.back()^=1;try{clay_roster::configure_image(bad);return 5;}catch(const std::exception&){}
 if(clay_roster::enabled())return 6;clay_roster::configure_image(rom);
 for(int id:{6,11,12,16}){filter.r4=id;func_8005D484(rdram,&filter);if(filter.r2!=unsigned(id))return 26;}filter.r4=7;func_8005D484(rdram,&filter);if(filter.r2!=8)return 27;
 constexpr int ids[]{0,1,2,6,10,15,8,11,14,5,9,12,13,4,3,16};
 if(clay_sc_roster_count()!=16||clay_sc_roster_columns()!=4||clay_sc_roster_state_bytes()!=0x118||clay_sc_roster_flag_offset()!=0xB8)return 7;
 for(int i=0;i<16;i++){if(clay_sc_roster_id_from_slot(i)!=ids[i]||clay_sc_roster_slot_from_id(ids[i])!=i)return 8;for(int delta:{-1,1}){int col=i%4,row=i/4;int horizontal=row*4+(col+delta+4)%4,vertical=((row+delta+4)%4)*4+col;if(horizontal<0||horizontal>=16||vertical<0||vertical>=16||clay_sc_roster_row_stride(row)!=row*4)return 9;}}
 if(clay_sc_roster_slot_from_id(7)!=-1||clay_sc_roster_extra_allowed(7)||!clay_sc_roster_extra_allowed(6))return 10;
 for(uint32_t i=0x1000;i<0xF9980;i++)MEM_B(i-0xC00,gpr(S32(0x80000000)))=rom[i];
 MEM_W(0,gpr(S32(0x800D92AC)))=1;uint32_t state=allocate(rdram,0x118),bank=allocate(rdram,146400);for(uint32_t i=0;i<0xB8;i++)MEM_B(i,gpr(S32(state)))=uint8_t(0x30+i);MEM_W(0,gpr(S32(0x800DDD30)))=gpr(S32(state));MEM_W(0x20,gpr(S32(state)))=gpr(S32(bank));
 auto portrait_data=file("local/sc_roster/base-portraits.bin");if(portrait_data.size()!=146388)return 11;
 for(uint32_t i=0;i<12;i++)MEM_B(i,gpr(S32(bank)))=base[0xB6BA90+i];for(uint32_t i=0;i<portrait_data.size();i++)MEM_B(12+i,gpr(S32(bank)))=portrait_data[i];
 std::vector<uint8_t> before(0xB8);for(unsigned i=0;i<before.size();i++)before[i]=MEM_BU(i,gpr(S32(state)));
 recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=gpr(S32(0x807FF000));for(unsigned i=0;i<0x280;i++)MEM_B(i,gpr(S32(0x800FF140)))=uint8_t(0x91+i);uint64_t live_before=live_bytes;uint32_t rng_before=MEM_W(0,gpr(S32(0x800EE5B0)));std::vector<uint8_t> globals(rdram,rdram+0x200000);clay_sc_roster_install(rdram,&c);if(live_bytes!=live_before-146400+201120)return 31;if(uint32_t(MEM_W(0,gpr(S32(0x800EE5B0))))!=rng_before)return 32;for(unsigned a=0;a<0x200000;a++)if(rdram[a]!=globals[a]){unsigned p=a^3;if(p>=0xDABA0&&p<0xDABE4)continue;fprintf(stderr,"Changed global%08X\n",0x80000000+p);return 33;}
 uint32_t expanded=MEM_W(0x20,gpr(S32(state)));if(MEM_W(4,gpr(S32(expanded)))!=147)return 12;
 for(unsigned i=0;i<0xB8;i++)if(i<0x20||i>=0x24){if(MEM_BU(i,gpr(S32(state)))!=before[i])return 13;}
 for(unsigned i=0;i<107;i++)if(MEM_W(12+i*8,gpr(S32(expanded)))!=MEM_W(12+i*8,gpr(S32(bank)))+320||MEM_W(16+i*8,gpr(S32(expanded)))!=MEM_W(16+i*8,gpr(S32(bank))))return 14;
 for(unsigned i=868;i<146400;i++)if(MEM_BU(i+320,gpr(S32(expanded)))!=MEM_BU(i,gpr(S32(bank))))return 15;
 for(unsigned d=0;d<4;d++){std::set<std::vector<uint8_t>> different;int id=ids[d*4+3];int first=MEM_HU(id*4,gpr(S32(0x800DABA0)));if(first!=107+int(d)*10)return 16;for(unsigned f=0;f<8;f++){uint32_t sprite=expanded+uint32_t(MEM_W(12+(first+f)*8,gpr(S32(expanded))));if(MEM_HU(6,gpr(S32(sprite)))!=26||MEM_HU(8,gpr(S32(sprite)))!=26||MEM_HU(14,gpr(S32(sprite)))!=32||MEM_HU(10,gpr(S32(sprite)))||MEM_HU(12,gpr(S32(sprite)))||MEM_BU(0,gpr(S32(sprite)))!=0x11)return 17;std::vector<uint8_t> pixels;for(unsigned j=528;j<1360;j++)pixels.push_back(MEM_BU(j,gpr(S32(sprite))));different.insert(pixels);for(unsigned y=0;y<26;y++)if(MEM_BU(528+y*32,gpr(S32(sprite)))||MEM_BU(528+y*32+25,gpr(S32(sprite))))return 34;}if(different.size()<5)return 35;}
 for(unsigned d=0;d<4;d++)for(unsigned f=0;f<8;f++){unsigned i=107+d*10+f;uint32_t sprite=expanded+uint32_t(MEM_W(12+i*8,gpr(S32(expanded))));char path[256];snprintf(path,sizeof(path),"local/sc_portraits/head-%u-%u.bin",unsigned(ids[d*4+3]),f);FILE*out=fopen(path,"wb");for(unsigned j=0;j<1360;j++)fputc(MEM_BU(j,gpr(S32(sprite))),out);fclose(out);}
 // Regression: generated poses must actually advance and wrap in the original selector VM.
 for(unsigned slot:{3u,7u,11u,15u}){
  MEM_H(0xB8+slot*2,gpr(S32(state)))=0x4000;
  std::set<unsigned> visible;bool wrapped=false;unsigned previous=0;
  for(unsigned tick=0;tick<120;tick++){
   recomp_context step{};step.f_odd=&step.f0.u32h;step.r29=gpr(S32(0x807FF000));MEM_W(0x54,step.r29)=slot;
   sc_roster_animation_tick(rdram,&step);
   unsigned phase=MEM_HU(0xB8+slot*2,gpr(S32(state)))&255;
   if(phase<previous)wrapped=true;previous=phase;visible.insert(phase>>3);
   if(!(MEM_HU(0xB8+slot*2,gpr(S32(state)))&0x4000))return 37;
  }
  if(visible.size()<5||!wrapped){fprintf(stderr,"Portrait animation stuck: slot%u visible%zu wrap%d\n",slot,visible.size(),wrapped);return 36;}
 }
 // Regression: selected/hover/unhover must resolve TWO tail frames owned by the same fighter.
 for(unsigned d=0;d<4;d++){
  unsigned slot=d*4+3,id=ids[slot];uint32_t tail=MEM_HU(id*4+2,gpr(S32(0x800DABA0)));
  uint32_t owned_end=d<3?MEM_HU(ids[(d+1)*4+3]*4,gpr(S32(0x800DABA0))):MEM_W(4,gpr(S32(expanded)));
  for(unsigned mode:{0x100u,0x200u,0x300u})for(unsigned phase=0;phase<16;phase++){
   unsigned selected=tail+(phase>>3);
   if(selected>=owned_end){fprintf(stderr,"Selected portrait spills fighter: ID%u mode%X phase%u index%u end%u\n",id,mode,phase,selected,owned_end);return 38;}
   MEM_H(0xB8+slot*2,gpr(S32(state)))=0x4000|mode|phase;
   recomp_context draw{};draw.f_odd=&draw.f0.u32h;draw.r29=gpr(S32(0x807FF000));MEM_W(0x40,draw.r29)=slot;MEM_W(0x4C,draw.r29)=gpr(S32(state+0xD8));
   sc_roster_portrait_lookup(rdram,&draw);
   uint32_t expected=expanded+uint32_t(MEM_W(12+selected*8,gpr(S32(expanded))));
   if(uint32_t(draw.r2)!=expected||MEM_HU(6,gpr(S32(uint32_t(draw.r2))))!=26||MEM_HU(8,gpr(S32(uint32_t(draw.r2))))!=26)return 39;
  }
 }
 // Exercise actual hover, unhover, confirmation, recolor and rehover updates for both cursors.
 for(unsigned player=0;player<2;player++)for(unsigned d=0;d<4;d++)for(unsigned initial_mode:{0u,0x100u,0x200u,0x300u}){
  unsigned slot=d*4+3,id=ids[slot];unsigned first=MEM_HU(id*4,gpr(S32(0x800DABA0))),tail=MEM_HU(id*4+2,gpr(S32(0x800DABA0)));
  MEM_H(0xA0,gpr(S32(state)))=0;MEM_H(0xA4,gpr(S32(state)))=0;MEM_H(0xA8,gpr(S32(state)))=0;
  MEM_H(0xB8+slot*2,gpr(S32(state)))=0x4000|initial_mode|(initial_mode==0x200?15:0);
  for(unsigned tick=0;tick<96;tick++){
   recomp_context step{};step.f_odd=&step.f0.u32h;step.r29=gpr(S32(0x807FF000));MEM_W(0x54,step.r29)=slot;
   bool hovered=tick<32||tick>=64;MEM_W(0x50,step.r29)=player==0&&hovered?slot*2:0xFFFF;MEM_W(0x4C,step.r29)=player==1&&hovered?slot*2:0xFFFF;MEM_W(0x5C,step.r29)=clay_sc_roster_coordinates(rdram)+slot*4;
   sc_roster_selector_tick(rdram,&step);
   unsigned flags=MEM_HU(0xB8+slot*2,gpr(S32(state))),phase=flags&255,index=((flags&0x3F00)?tail:first)+(phase>>3);
   if(index<first||index>=first+10){fprintf(stderr,"Native selector update escaped fighter ID%u P%u mode%X tick%u index%u\n",id,player+1,initial_mode,tick,index);return 40;}
   recomp_context draw{};draw.f_odd=&draw.f0.u32h;draw.r29=gpr(S32(0x807FF000));MEM_W(0x40,draw.r29)=slot;MEM_W(0x4C,draw.r29)=clay_sc_roster_coordinates(rdram);
   sc_roster_portrait_lookup(rdram,&draw);uint32_t expected=expanded+uint32_t(MEM_W(12+index*8,gpr(S32(expanded))));if(uint32_t(draw.r2)!=expected)return 41;
  }
 }
 // Actual selector cache builder loads B6BA90 again; generic loader posthook must extend that fresh bank.
 c.r29=gpr(S32(0x807FF000));func_8005AB2C(rdram,&c);c.r4=6;c.r5=16;func_8005AC38(rdram,&c);
 for(unsigned player=0;player<2;player++){
  uint32_t descriptor=uint32_t(MEM_W(player*4,gpr(S32(0x800DDD5C))));if(MEM_W(0,gpr(S32(descriptor)))!=1)return 21;
  uint32_t handle=uint32_t(MEM_W(4,gpr(S32(descriptor)))),payload=handles.at(handle);unsigned frame=107+(player==0?0:30);uint32_t expected=expanded+uint32_t(MEM_W(12+frame*8,gpr(S32(expanded))));
  for(unsigned i=0;i<1360;i++)if(MEM_BU(i,gpr(S32(payload)))!=MEM_BU(i,gpr(S32(expected))))return 22;
 }
 if(clay_sc_roster_coordinates(rdram)!=gpr(S32(state+0xD8)))return 18;clay_sc_roster_install(rdram,&c);if(MEM_W(0x20,gpr(S32(state)))!=gpr(S32(expanded)))return 19;
 MEM_W(0,gpr(S32(0x800DDD40)))=gpr(S32(profile));
 if(int result=navigation(rdram,state,profile,4,0x118,ids))return result;
 for(unsigned first=0;first<16;first++)for(unsigned second=0;second<16;second++){
  MEM_W(0xC,gpr(S32(profile)))=ids[first];MEM_W(0x10,gpr(S32(profile)))=ids[second];
  c={};c.f_odd=&c.f0.u32h;c.r29=gpr(S32(0x807FF000));sc_roster_original_lookup(rdram,&c);
  if(MEM_W(0x78,gpr(S32(state)))!=int(first%4)||MEM_W(0x7C,gpr(S32(state)))!=int(first/4)||MEM_W(0x80,gpr(S32(state)))!=int(second%4)||MEM_W(0x84,gpr(S32(state)))!=int(second/4))return 28;
 }
 for(auto[p,n]:allocations)for(unsigned i=0;i<32;i++)if(MEM_BU(n+i,gpr(S32(p)))!=0xA5)return 20;
 puts("96 original base12 and128 expanded P1/P2 navigation and native confirmation cases;144 base and256 expanded original636E4 inverse lookup pairs; scoped original5D484 filter passed");
 puts("Authentic idle0 original01008/72FE8/9215C/02DE0 portraits, restored cache/global/RNG, zero temporary allocation leaks, black borders, exact original hotspot0/0 descriptors, real frame variation; all107 original frames unchanged,16slots4columns, sidecar/state preservation, eight valid donor animation indices, SHA mutation rejection and allocation guards passed");free(rdram);return 0;
}
