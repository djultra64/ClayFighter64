// Authentic original selector geometry extension checks; djultra64.
#define main sc_roster_portrait_test_main
#include "sc_roster.cpp"
#undef main
#include <set>
#include <array>
extern "C" void sc_roster_existing_constructor(uint8_t*,recomp_context*);
extern "C" void func_80004730(uint8_t*,recomp_context*);
int main(){
 rom=file("probe-output/base.z64");auto*rdram=static_cast<uint8_t*>(calloc(1,0x800000));
 for(uint32_t i=0x1000;i<0xF9980;i++)MEM_B(i-0xC00,gpr(S32(0x80000000)))=rom[i];
 recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=gpr(S32(0x807FF000));c.r4=0xB9B230;func_80004730(rdram,&c);
 uint32_t resource=c.r2,part=resource+MEM_W(12,gpr(S32(resource)));constexpr unsigned bytes=0x6148;
 std::vector<uint8_t> before(bytes);for(unsigned i=0;i<bytes;i++)before[i]=MEM_BU(i,gpr(S32(part)));
 if(clay_sc_roster_extend_grid(rdram,&c,gpr(S32(part)))!=0)return 1;
 clay_roster::configure_image(file("local/sc_roster/asset-image-roster.z64"));
 uint32_t copy=clay_sc_roster_extend_grid(rdram,&c,gpr(S32(part)));if(!copy)return 2;
 unsigned changed=0;for(unsigned i=0;i<bytes;i++)if(before[i]!=MEM_BU(i,gpr(S32(part))))changed++;
 if(!changed||changed>300)return 3;
 if(clay_sc_roster_extend_grid(rdram,&c,gpr(S32(part)))!=gpr(S32(copy)))return 4;
 std::set<int> panel_rows;unsigned panel_triangles=0,total_triangles=0;
 std::array<uint32_t,32> vertices{};unsigned count=0;
 for(unsigned o=0xE0;o+8<=bytes;o+=8){
  uint32_t word=MEM_W(o,gpr(S32(copy))),argument=MEM_W(o+4,gpr(S32(copy)));
  if(word==0xB8000000)break;
  if(word>>24==4){count=(word>>10)&63;uint32_t address=0x80000000|argument;if(address<copy||uint64_t(address)+count*16>uint64_t(copy)+bytes)return 5;for(unsigned i=0;i<count;i++)vertices[i]=address+i*16;}
  else if(word>>24==0xB1)for(unsigned triangle=0;triangle<2;triangle++){
   uint32_t encoded=(triangle?argument:word)&0xFFFFFF;if(!encoded)continue;total_triangles++;bool panel=true;int top=-1000;
   for(unsigned i=0;i<3;i++){unsigned index=(encoded>>(16-i*8)&255)/2;if(index>=count)return 6;uint32_t vertex=vertices[index];int x=MEM_H(0,gpr(S32(vertex))),y=MEM_H(2,gpr(S32(vertex))),z=MEM_H(4,gpr(S32(vertex)));if(x<73||x>158||y< -240||y>80||(z!=47&&z!=88))return 7;panel&=z==88;top=std::max(top,y);}
   if(panel){panel_triangles++;panel_rows.insert(top);}
  }
 }
 if(total_triangles!=28||panel_triangles!=8||panel_rows.size()!=4)return 8;
 std::vector<uint8_t> shifted(bytes);for(unsigned i=0;i<bytes;i++)shifted[i]=MEM_BU(i,gpr(S32(part)));
 // Native5DDCC is also called when the existing selector state is reused
 // after Back; its5DDE4 branch resets state but does not reload the source.
 const uint32_t state=allocate(rdram,0x118),bank=allocate(rdram,12);
 MEM_W(0,gpr(S32(0x800DDD30)))=gpr(S32(state));MEM_W(0x1c,gpr(S32(state)))=gpr(S32(resource));MEM_W(0x20,gpr(S32(state)))=gpr(S32(bank));MEM_W(4,gpr(S32(bank)))=147;
 const size_t releases_before_reset=released.size();
 for(unsigned reset=0;reset<16;reset++){
  c={};c.f_odd=&c.f0.u32h;c.r29=gpr(S32(0x807FF000));sc_roster_existing_constructor(rdram,&c);
  if(released.size()!=releases_before_reset){fprintf(stderr,"RED: native reused-state constructor released live selector column\n");return 14;}
  if(clay_sc_roster_extend_grid(rdram,&c,gpr(S32(part)))!=gpr(S32(copy)))return 15;
  for(unsigned i=0;i<bytes;i++)if(MEM_BU(i,gpr(S32(part)))!=shifted[i])return 16;
 }
 size_t releases= released.size();clay_sc_roster_release_grid(rdram,&c);if(released.size()!=releases+1||released.back()!=copy)return 10;
 // Real allocator can reuse source addresses after original selector destructor; restored source must be extended anew.
 for(unsigned i=0;i<bytes;i++)MEM_B(i,gpr(S32(part)))=before[i];uint32_t reentered=clay_sc_roster_extend_grid(rdram,&c,gpr(S32(part)));
 if(reentered==copy||!reentered)return 11;
 for(unsigned i=0;i<bytes;i++)if(MEM_BU(i,gpr(S32(part)))!=shifted[i])return 12;
 clay_sc_roster_release_grid(rdram,&c);if(released.back()!=reentered)return 13;
 for(auto[p,n]:allocations)for(unsigned i=0;i<32;i++)if(MEM_BU(n+i,gpr(S32(p)))!=0xA5)return 9;
 puts("Original native mesh loader/codecs: preserved architecture, four authentic added cell panels/walls,28 source triangles, relocated packed vertices and all guards plus16actual reused-state constructor resets passed");free(rdram);
}
