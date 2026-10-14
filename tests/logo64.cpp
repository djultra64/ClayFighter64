// Actual original mesh plus native branding replacement checks; djultra64.
#include "logo64.h"
#include <cassert>
#include <fstream>
#include <vector>
#include <iterator>
#include <cstdio>
static uint32_t allocated=0,drawn=0,allocated_bytes=0;
static unsigned allocations=0;
static bool fail_allocation=false;
extern "C" void func_800B3C70(uint8_t*rdram,recomp_context*c){if(fail_allocation){c->r2=gpr(S32(0x80810000));return;}allocated=0x80500000;allocated_bytes=c->r5;allocations++;c->r2=gpr(S32(allocated));for(unsigned i=0;i<32;i++)MEM_BU(c->r5+i,gpr(S32(allocated)))=0xa5;}
extern "C" void func_800B4408(uint8_t*,recomp_context*){}
extern "C" void func_8005C234(uint8_t*,recomp_context*c){drawn=c->r5;}
int main(int argc,char**argv){assert(argc==2);std::ifstream f(argv[1],std::ios::binary);std::vector<uint8_t>b{std::istreambuf_iterator<char>(f),{}};assert(b.size()==11512);std::vector<uint8_t>ram(9<<20);auto*rdram=ram.data();constexpr uint32_t src=0x80233e00;
 for(unsigned i=0;i<b.size();i++)MEM_BU(i,gpr(S32(src)))=b[i];recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=S32(0x807ff000);c.r4=0x80110898;c.r5=gpr(S32(src));
 uint32_t replacement=clay_logo64_mesh(rdram,&c,gpr(S32(src)));assert(replacement!=src&&replacement>=0x80000000&&replacement<0x80800000);assert(allocations==1);assert(c.r4==0x80110898&&c.r29==gpr(S32(0x807ff000)));
 for(unsigned i=0;i<b.size();i++)assert(MEM_BU(i,gpr(S32(src)))==b[i]);
 assert(MEM_HU(0x24,gpr(S32(replacement)))==0xe0);assert(MEM_BU(0x14,gpr(S32(replacement)))==1);
 unsigned triangles=0,vertices=0;bool found_four=false;
 for(unsigned o=0xe0;o<0x10000;o+=8){uint32_t w=MEM_W(o,gpr(S32(replacement))),a=MEM_W(o+4,gpr(S32(replacement)));if(w>>24==0xb8)break;if(w>>24==4){unsigned n=(w>>10)&63;assert(n>0&&n<=30);assert(a>=uint32_t(replacement&0x1fffffff));assert(a+n*16<0x800000);vertices+=n;for(unsigned i=0;i<n;i++){int x=MEM_H(0,gpr(S32(0x80000000|a+i*16)));found_four|=x>0;}}if(w>>24==0xbf)triangles++;}
 assert(triangles>114&&vertices==triangles*3&&found_four);
 for(unsigned i=0;i<32;i++)assert(MEM_BU(allocated_bytes+i,gpr(S32(allocated)))==0xa5);
 // Compare EVERY original-six vertex attribute; only centering adds forty to X.
 unsigned vcount=0;uint32_t slots[32]{};uint32_t new_vertices=replacement+uint32_t(MEM_W(0xcc,gpr(S32(replacement))));
 for(unsigned o=0xe0;o<0x788;o+=8){uint32_t w=MEM_W(o,gpr(S32(src))),a=MEM_W(o+4,gpr(S32(src)));if(w>>24==0xb8)break;
  if(w>>24==4){for(unsigned i=0;i<((w>>10)&63);i++)slots[i]=0x80000000|a+i*16;}
  else if(w>>24==0xb1||w>>24==0xbf){for(unsigned t=0;t<(w>>24==0xb1?2u:1u);t++){uint32_t enc=w>>24==0xbf?a:(t?a:w&0xffffff);if(!enc)continue;uint32_t points[3];bool six=true;
   for(unsigned j=0;j<3;j++){points[j]=slots[(enc>>(16-j*8)&255)/2];six&=MEM_H(0,gpr(S32(points[j])))<-44;}
   if(six)for(auto point:points){uint32_t dest=new_vertices+vcount++*16;assert(MEM_H(0,gpr(S32(dest)))==MEM_H(0,gpr(S32(point)))+40);for(unsigned j=2;j<16;j++)assert(MEM_BU(j,gpr(S32(dest)))==MEM_BU(j,gpr(S32(point))));}
  }}
 }assert(vcount==114*3);
 assert(clay_logo64_mesh(rdram,&c,gpr(S32(src)))==gpr(gpr(S32(replacement)))&&allocations==1);
 // A later original mesh bank can occupy a different native allocation.
 // Rewrite its REAL physical Vtx addresses, then validate the immutable logo reuse.
 constexpr uint32_t relocated=0x80300000;
 for(unsigned i=0;i<b.size();i++)MEM_BU(i,gpr(S32(relocated)))=b[i];
 for(unsigned o=0xe0;o<0x788;o+=8){uint32_t w=MEM_W(o,gpr(S32(relocated)));if(w>>24==4)MEM_W(o+4,gpr(S32(relocated)))=MEM_W(o+4,gpr(S32(relocated)))+int32_t(relocated-src);if(w>>24==0xb8)break;}
 assert(clay_logo64_mesh(rdram,&c,gpr(S32(relocated)))==gpr(S32(replacement))&&allocations==1);
 // Original title->attract->title changes byte3 toB5; native5C234 readsmodebyte2only.
 for(unsigned tag:{0xb5u,0xffu,0x34u,0u}){MEM_BU(3,gpr(S32(relocated)))=tag;assert(clay_logo64_mesh(rdram,&c,gpr(S32(relocated)))==gpr(S32(replacement)));c.r5=gpr(S32(relocated));clay_logo64_draw(rdram,&c);assert(drawn==replacement);}
 for(unsigned padding:{0xa5u,0xffu,0x34u,0u}){MEM_BU(0x17,gpr(S32(relocated)))=padding;assert(clay_logo64_mesh(rdram,&c,gpr(S32(relocated)))==gpr(S32(replacement)));c.r5=gpr(S32(relocated));clay_logo64_draw(rdram,&c);assert(drawn==replacement);}
 MEM_W(0,gpr(S32(relocated)))=0x00030100;assert(clay_logo64_mesh(rdram,&c,gpr(S32(relocated)))==gpr(S32(relocated)));MEM_W(0,gpr(S32(relocated)))=0x00030000;
 MEM_W(0x2c,gpr(S32(relocated)))=16;assert(clay_logo64_mesh(rdram,&c,gpr(S32(relocated)))==gpr(S32(relocated)));MEM_W(0x2c,gpr(S32(relocated)))=0;
 c.r5=gpr(S32(src));clay_logo64_draw(rdram,&c);assert(drawn==replacement&&c.r5==gpr(gpr(S32(src))));
 assert(clay_logo64_mesh(rdram,&c,S32(0x80810000))==gpr(S32(0x80810000)));
 MEM_BU(0x14,gpr(S32(src)))=0;assert(clay_logo64_mesh(rdram,&c,gpr(S32(src)))==gpr(gpr(S32(src))));assert(allocations==1);
 std::vector<uint8_t>other(9<<20);rdram=other.data();for(unsigned i=0;i<b.size();i++)MEM_BU(i,gpr(S32(src)))=b[i];fail_allocation=true;
 assert(clay_logo64_mesh(rdram,&c,gpr(S32(src)))==gpr(S32(src)));c.r5=gpr(S32(src));clay_logo64_draw(rdram,&c);assert(drawn==src);
 std::vector<uint8_t>cold(9<<20);rdram=cold.data();for(unsigned i=0;i<b.size();i++)MEM_BU(i,gpr(S32(src)))=b[i];
 MEM_BU(3,gpr(S32(src)))=0xb5;MEM_BU(0x17,gpr(S32(src)))=0xa5;fail_allocation=false;
 assert(clay_logo64_mesh(rdram,&c,gpr(S32(src)))!=gpr(S32(src)));assert(allocations==2);
 puts("PASS: original six preserved, native 64 geometry, bounded physical vertex pointers, ABI, reuse and invalid-mesh/allocation fallback");
}
