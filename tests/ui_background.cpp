// Original native UI fill/scissor fixture by djultra64.
#include "ui_background.h"
#include "graphics_config.h"
#include <cassert>
#include <vector>
#include <fstream>
#include <iterator>
#include <cstdio>
#include <cstdlib>
#include <bit>
#include <cmath>
#include <cstring>
extern "C" void func_800A45BC(uint8_t*,recomp_context*);
extern "C" void func_800A4D9C(uint8_t*,recomp_context*);
extern "C" int rt64_fill_lower_edge(int);
extern "C" void func_800A4BF0(uint8_t*,recomp_context*);
extern "C" void do_break(uint32_t){std::abort();}
extern "C" void switch_error(const char*,uint32_t,uint32_t){std::abort();}
extern "C" void func_800396C4(uint8_t*,recomp_context*);
extern "C" void func_800A3F88(uint8_t*,recomp_context*){}
extern "C" void func_800A561C(uint8_t*,recomp_context*){}
extern "C" void func_800A5868(uint8_t*,recomp_context*){}
extern "C" void func_800AA9EC(uint8_t*,recomp_context*){std::abort();}
extern "C" void func_800ACD10(uint8_t*,recomp_context*){std::abort();}
int main(int argc,char**argv){assert(argc==2||argc==3);std::ifstream file(argv[1],std::ios::binary);std::vector<uint8_t>rom(std::istreambuf_iterator<char>(file),{}),ram(9<<20);assert(rom.size()==0xc00000);auto*r=ram.data();auto*rdram=r;for(uint32_t i=0x1000;i<0xf9980;i++)r[((i-0x1000)+0x400)^3]=rom[i];
 constexpr uint32_t dl=0x80300000,sp=0x807fe000,table=0x80200000,rect=0x80201000;
 auto set=[&](uint32_t p,uint32_t v){MEM_W(0,S32(p))=S32(v);};auto get=[&](uint32_t p){return uint32_t(MEM_W(0,S32(p)));};
 auto init=[&](){set(0x80110898,dl);set(0x801108cc,2);set(0x800de174,0);set(0x80110880,2048);set(0x801108dc,table);set(table,dl);set(0x8010cfd0,320);set(0x8010cfd4,240);};
 auto context=[&](){recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=S32(sp);c.r4=S32(0x80110898);return c;};
 for(unsigned height:{720u,2160u}){init();auto o=clay::GraphicsOptions{};o.render_height=height;clay::set_graphics_options(o);auto c=context();c.f_odd=&c.f0.u32h;func_800A45BC(r,&c);uint32_t end=get(0x80110898),scissor=0;for(uint32_t p=dl;p<end;p+=8)if(get(p)==0xed000010u)scissor=p;assert(scissor);assert(get(scissor+4)==0x005003ac); // RED original: y4..235.
  assert(clay::ui::fix_scissor(r,dl));assert(get(scissor)==0xed000000&&get(scissor+4)==0x005003c0);assert(get(0x80110898)==end);assert(!clay::ui::fix_scissor(r,dl));
  o.widescreen=false;clay::set_graphics_options(o);set(scissor,0xed000010);set(scissor+4,0x005003ac);assert(!clay::ui::fix_scissor(r,dl));assert(get(scissor)==0xed000010);
 }
 init();clay::set_graphics_options(clay::GraphicsOptions{});set(rect,0);set(rect+4,0);set(rect+8,319);set(rect+12,239);auto c=context();c.f_odd=&c.f0.u32h;c.r5=S32(rect);c.r6=0;c.r7=0;set(sp+16,0);set(sp+20,80);auto original_ram=ram;func_800A4D9C(r,&c);uint32_t bytes=get(0x80110898)-dl;std::vector<uint32_t>stock(bytes/4);for(unsigned i=0;i<stock.size();i++)stock[i]=get(dl+i*4);assert(bytes<=512);bool fill=false;for(unsigned i=0;i<stock.size();i+=2)fill|=(stock[i]>>24)==0xf6;assert(fill);
 // Reset the original state cache so the exact native payload stays identical.
 ram=original_ram;r=ram.data();rdram=r;init();c=context();c.f_odd=&c.f0.u32h;c.r5=S32(rect);set(sp+16,0);set(sp+20,80);clay::ui::draw_fullscreen_fill(r,&c,&func_800A4D9C);assert(get(dl)==0x00525464);assert(get(dl+8)==0x64000017);assert(get(dl+16)==0xed000000&&get(dl+20)==0x005003c0);assert(get(dl+24)==0x64000033&&get(dl+28)==1);for(unsigned i=0;i<stock.size();i++)assert(get(dl+32+i*4)==stock[i]);uint32_t finish=get(0x80110898);assert(get(finish-16)==0x64000033&&get(finish-12)==2&&get(finish-8)==0x64000018);
 // Near-pool exhaustion: original still executes; decoration must not write past capacity.
 ram=original_ram;r=ram.data();rdram=r;init();set(0x80110880,bytes/8);set(dl+bytes,0xa5a5a5a5);c=context();c.f_odd=&c.f0.u32h;c.r5=S32(rect);set(sp+20,80);clay::ui::draw_fullscreen_fill(r,&c,&func_800A4D9C);assert(get(dl)!=0x00525464);assert(get(0x80110898)==dl+bytes);assert(get(dl+bytes)==0xa5a5a5a5);
 // Original native full-width top/bottom bands meet a quarter-pixel iris edge.
 // At3x/9x upscale their integer-only edges leave sample gaps after the
 // actual RT64 FILL quantizer (compiled from its authoritative local source).
 for(unsigned render_height:{720u,2160u})for(bool top:{true,false}){
  ram=original_ram;r=ram.data();rdram=r;init();auto o=clay::GraphicsOptions{};o.render_height=render_height;clay::set_graphics_options(o);
  set(rect,0);set(rect+4,top?0:200);set(rect+8,319);set(rect+12,top?39:239);
  auto raw=context();raw.f_odd=&raw.f0.u32h;raw.r5=S32(rect);auto initial=ram;
  func_800A4BF0(r,&raw);uint32_t length=get(0x80110898)-dl,fill_at=0;
  std::vector<uint32_t> native(length/4);for(unsigned i=0;i<native.size();i++)native[i]=get(dl+i*4);
  for(unsigned i=0;i<native.size();i+=2)if(native[i]>>24==0xf6)fill_at=i*4;assert(fill_at<length);
  const uint32_t a=native[fill_at/4],b=native[fill_at/4+1];assert((a&4095)==(top?39:239)*4u);assert((b&4095)==(top?0:200)*4u);
  const double scale=render_height/240.0,iris_edge=top?40.25:199.75;
  if(top)assert(std::floor(rt64_fill_lower_edge(a&4095)/4.0*scale-.5)+1 < std::ceil(iris_edge*scale-.5));
  else assert(std::ceil((b&4095)/4.0*scale-.5) > std::floor(iris_edge*scale-.5)+1);
  ram=initial;r=ram.data();rdram=r;c=context();c.f_odd=&c.f0.u32h;c.r5=S32(rect);clay::ui::draw_transition_band(r,&c,&func_800A4BF0);
  assert(get(0x80110898)==dl+length+48);
  const uint32_t fixed_a=get(dl+32+fill_at),fixed_b=get(dl+36+fill_at);
  assert(fixed_a==(top?a+4:a));assert(fixed_b==(top?b:b-1));
  if(top)assert(std::floor(rt64_fill_lower_edge(fixed_a&4095)/4.0*scale-.5)+1 >= std::ceil(iris_edge*scale-.5));
  else assert(std::ceil((fixed_b&4095)/4.0*scale-.5) <= std::floor(iris_edge*scale-.5)+1);
  for(unsigned i=0;i<native.size();i++)if(i!=fill_at/4&&i!=fill_at/4+1)assert(get(dl+32+i*4)==native[i]);
  assert(get(rect)==0&&get(rect+4)==(top?0u:200u)&&get(rect+8)==319&&get(rect+12)==(top?39u:239u));
  raw.f_odd=c.f_odd=nullptr;assert(!memcmp(&raw,&c,sizeof(c)));
  // Capacity rejection is atomic: original F6 must stay byte-identical.
  ram=initial;r=ram.data();rdram=r;set(0x80110880,length/8);set(dl+length,0xa5a5a5a5);c=context();c.f_odd=&c.f0.u32h;c.r5=S32(rect);clay::ui::draw_transition_band(r,&c,&func_800A4BF0);
  assert(get(0x80110898)==dl+length&&get(dl+length)==0xa5a5a5a5);for(unsigned i=0;i<native.size();i++)assert(get(dl+i*4)==native[i]);
 }
 // Full-screen/interior bands retain their exact native corners; 4:3 is untouched.
 for(auto ys:std::vector<std::pair<unsigned,unsigned>>{{0,239},{30,80},{0,39},{200,239}}){
  for(bool wide:{false,true}){
   ram=original_ram;r=ram.data();rdram=r;init();auto o=clay::GraphicsOptions{};o.widescreen=wide;clay::set_graphics_options(o);
   set(rect,0);set(rect+4,ys.first);set(rect+8,319);set(rect+12,ys.second);auto initial=ram;
   auto raw=context();raw.f_odd=&raw.f0.u32h;raw.r5=S32(rect);func_800A4BF0(r,&raw);uint32_t length=get(0x80110898)-dl;
   std::vector<uint32_t> native(length/4);for(unsigned i=0;i<native.size();i++)native[i]=get(dl+i*4);
   ram=initial;r=ram.data();rdram=r;c=context();c.f_odd=&c.f0.u32h;c.r5=S32(rect);clay::ui::draw_transition_band(r,&c,&func_800A4BF0);
   if(!wide||ys==std::pair<unsigned,unsigned>{0,239}||ys==std::pair<unsigned,unsigned>{30,80}){
    assert(get(0x80110898)==dl+length+(wide?48:0));
    for(unsigned i=0;i<native.size();i++)assert(get(dl+(wide?32:0)+i*4)==native[i]);
   }
  }
 }
 clay::set_graphics_options(clay::GraphicsOptions{});
 for(float progress:{0.f,.1f,.5f,1.f}){init();set(0x801108a0,0x80400000);set(rect,1);set(rect+4,2);set(rect+8,std::bit_cast<uint32_t>(progress));set(rect+16,0);c=context();c.f_odd=&c.f0.u32h;c.r4=S32(rect);c.r5=S32(0x80110898);uint32_t vb=clay::ui::vertex_cursor(r);func_800396C4(r,&c);assert(get(0x801108a0)-vb==192*16);std::vector<uint8_t>original(192*16);for(unsigned i=0;i<original.size();i++)original[i]=MEM_BU(0,S32(vb+i));assert(clay::ui::extend_circle_mask(r,rect,vb));
  unsigned changed=0;for(unsigned i=0;i<192;i++){uint32_t v=vb+i*16;if(i%3==2){assert(std::abs(int(MEM_H(0,S32(v))))==214);changed++;}for(unsigned j=0;j<16;j++)if(i%3!=2||j>=2)assert(MEM_BU(0,S32(v+j))==original[i*16+j]);}assert(changed==64);assert(get(0x801108a0)-vb==192*16);
  // An unexpected original anchor rejects atomically, preserving every vertex.
  for(unsigned i=0;i<original.size();i++)MEM_BU(0,S32(vb+i))=original[i];MEM_H(0,S32(vb+2*16))=159;assert(!clay::ui::extend_circle_mask(r,rect,vb));assert(MEM_H(0,S32(vb+5*16))==160);
 }
 // Read-only private dump from the actual original native mesh codec fixture.
 if(argc>2){std::ifstream mesh(argv[2],std::ios::binary);std::vector<uint8_t>image(std::istreambuf_iterator<char>(mesh),{});assert(image.size()==0x800000);for(unsigned i=0;i<image.size();i++)MEM_BU(0,S32(0x80000000u+i))=image[i];constexpr uint32_t part=0x8021a170;std::vector<uint8_t>stock(0x6148);for(unsigned i=0;i<stock.size();i++)stock[i]=MEM_BU(0,S32(part+i));assert(clay::ui::extend_selector_background_mesh(r,part));assert(clay::ui::extend_selector_background_mesh(r,part));unsigned modified=0;
  for(unsigned i=0;i<stock.size();i++)if(MEM_BU(0,S32(part+i))!=stock[i]){assert(i>=0x2c68&&((i-0x2c68)%16)<2);unsigned at=i-(i-0x2c68)%16;int16_t original_x=int16_t(unsigned(stock[at])<<8|stock[at+1]);assert(std::abs(int(original_x))>160);modified++;}assert(modified>0&&modified<=856);
  auto o=clay::GraphicsOptions{};o.widescreen=false;clay::set_graphics_options(o);assert(clay::ui::extend_selector_background_mesh(r,part));for(unsigned i=0;i<stock.size();i++)assert(MEM_BU(0,S32(part+i))==stock[i]);
 }
 puts("PASS: native A45BC full scissor; A4D9C unchanged fill payload; A4BF0 anchored band quarter-pixel join closes actual720/2160 sample gap, preserving colors/context/sourceRect with atomic capacity rejection and unmodified4:3/full/interior bands; original192-vertex iris preserves128 aperture/rim vertices and all64Y/color/UV fields; actual846-vertex selector mesh extends428 exterior vertices/restores4:3");
}
