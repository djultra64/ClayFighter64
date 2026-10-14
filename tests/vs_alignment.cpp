#define main unused_portrait_main
#include "sc_portrait_native.cpp"
#undef main
#include <fstream>
#include <iterator>
#include <bit>
#include <cassert>
#include "vs_alignment.h"
static float visible_low=10000;
extern "C" void vs_capture_sprite(uint8_t*rdram,recomp_context*c){uint32_t p=c->r5,basis=MEM_W(20,c->r29);unsigned fmt=MEM_BU(0,S32(p)),offset=MEM_HU(2,S32(p)),height=MEM_HU(8,S32(p)),width=MEM_HU(6,S32(p)),stride=MEM_HU(14,S32(p));int hy=MEM_H(12,S32(p));if(fmt!=0x43)return;int last=-1;for(unsigned y=0;y<height;y++)for(unsigned x=0;x<width;x++)if(MEM_BU(0,S32(p+offset+y*stride+x*4+3))>0)last=int(y);if(last<0)return;float cy=std::bit_cast<float>(uint32_t(c->r7)),by=std::bit_cast<float>(MEM_W(4,S32(basis)));visible_low=std::min(visible_low,cy+(hy-last-1)*by);}
extern "C" void func_8005DB24(uint8_t*,recomp_context*);
int main(int argc,char**argv){if(argc!=3)return 1;std::ifstream file(argv[1],std::ios::binary);rom.assign(std::istreambuf_iterator<char>(file),{});unsigned id=atoi(argv[2]);auto*rdram=static_cast<uint8_t*>(calloc(1,0x800000));for(uint32_t i=0x1000;i<0xF9980;i++)MEM_BU(i-0xC00,gpr(S32(0x80000000)))=rom[i];MEM_W(0,S32(0x800D92AC))=1;if(rom.size()!=0xc00000)clay_sc::configure_asset_image(rom,id);
 recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=S32(0x807FE000);c.r4=id;c.r5=0;func_80001008(rdram,&c);uint32_t object=c.r2;clay_sc_register_model(rdram,S32(object));uint32_t actor=alloc(rdram,0x3484);c.r4=S32(actor);func_80072D50(rdram,&c);c.r4=S32(actor);c.r5=MEM_W(12,S32(object));func_80072FE8(rdram,&c);uint32_t def=MEM_W(0x1D4,S32(actor)),table=MEM_W(0x1DC,S32(actor)),script=def+MEM_HU(0,S32(table));c.r4=S32(actor+0xFB4);c.r5=S32(script);c.r6=c.r7=S32(-1);func_80091A8C(rdram,&c);c.r4=S32(actor+0xFB4);func_8009215C(rdram,&c);
 constexpr uint32_t basis=0x80160000,dl=0x80500000,vtx=0x80600000;for(unsigned i=0;i<24;i++)MEM_BU(0,S32(basis+i))=0;MEM_W(4,S32(basis))=0x3f800000;MEM_W(16,S32(basis))=0xbf800000;MEM_W(0,S32(0x80110898))=S32(dl);MEM_W(0,S32(0x801108a0))=S32(vtx);
 int offsetX=MEM_H(0,S32(0x800DACF4+id*4)),offsetY=MEM_H(2,S32(0x800DACF4+id*4));unsigned checked=0;int target_offset=id==6?-18:id==11?-25:id==12?-24:id==16?-22:0;
 for(unsigned tick=0;tick<64;tick++){
  if(tick){c.r4=S32(actor+0xFB4);func_8009215C(rdram,&c);}
  MEM_W(0,S32(0x80110898))=S32(dl);MEM_W(0,S32(0x801108a0))=S32(vtx);visible_low=10000;
 c.r4=S32(actor);c.r5=S32(object);c.r6=S32(offsetX-90);c.r7=S32(offsetY-37);MEM_W(16,c.r29)=600;MEM_W(20,c.r29)=S32(basis);MEM_W(24,c.r29)=S32(basis+16);func_8005DB24(rdram,&c);unsigned vertices=(uint32_t(MEM_W(0,S32(0x801108a0)))-vtx)/16;int low=10000,high=-10000;for(unsigned i=0;i<vertices;i++){int y=MEM_H(2,S32(vtx+i*16));low=std::min(low,y);high=std::max(high,y);}float scale=std::bit_cast<float>(MEM_W(0x1D90,S32(def)));if(tick==0)printf("ID%u table(%d,%d) scale%.4f actual5DB24/A6520verts%u y(%d,%d) opaquefoot%.3f\n",id,offsetX,offsetY,scale,vertices,low,high,visible_low);
 std::vector<uint8_t>before(vertices*16);for(unsigned i=0;i<before.size();i++)before[i]=MEM_BU(0,S32(vtx+i));float before_low=visible_low;
 for(unsigned player=0;player<2;player++){
  MEM_W(0,S32(0x80110898))=S32(dl);MEM_W(0,S32(0x801108a0))=S32(vtx);visible_low=10000;c.r4=S32(actor);c.r5=S32(object);c.r6=S32(offsetX+(player?90:-90));c.r7=S32(offsetY-37);MEM_W(16,c.r29)=600;MEM_W(20,c.r29)=S32(basis);MEM_W(24,c.r29)=S32(basis+16);
  const bool changed=clay::ui::align_vs_actor(rdram,&c);assert(changed==(target_offset!=0));func_8005DB24(rdram,&c);assert(uint32_t(MEM_W(0,S32(0x801108a0)))-vtx==vertices*16);
  assert(std::abs(visible_low-(before_low+target_offset))<.01f);
  for(unsigned i=0;i<vertices;i++){
   int16_t original_y=int16_t(unsigned(before[i*16+2])<<8|before[i*16+3]);assert(MEM_H(2,S32(vtx+i*16))==original_y+target_offset);
   // P2 changes nativeX; every remaining position/Z/UV/color/alpha field is exact.
   for(unsigned j=4;j<16;j++)assert(MEM_BU(0,S32(vtx+i*16+j))==before[i*16+j]);
  }checked++;
 }
 if(target_offset&&tick==0)assert(std::abs(before_low+target_offset+64)<.01f);
 // Original/root changes to baseline and nonregistered actors remain untouched.
 c.r4=S32(actor);c.r5=S32(object);c.r7=S32(-38);assert(!clay::ui::align_vs_actor(rdram,&c)&&int32_t(c.r7)==-38);
 }
 printf("PASS: ID%u originalVS/nativeRGBA geometry64ticks×2players; constant donoroffset%d; allnativepose/Z/UV/colorfields retained\n",id,target_offset,checked);

}
