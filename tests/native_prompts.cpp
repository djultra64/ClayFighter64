// Original font renderer + genuine Kenney keycap/controller verification; djultra64.
#include "native_prompts.h"
#include "input_hud.h"
#include "stb/stb_image.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstring>
#include <algorithm>
extern "C" void func_80072C4C(uint8_t*,recomp_context*);
extern "C" void func_8007145C(uint8_t*,recomp_context*);
extern "C" void func_80071738(uint8_t*,recomp_context*);
extern "C" void func_8007191C(uint8_t*,recomp_context*);
static std::vector<uint32_t> dispatched;
static uint32_t heap=0x80400000;static unsigned allocations=0;
static std::vector<uint32_t> freed;
extern "C" void func_800B4408(uint8_t*,recomp_context*c){freed.push_back(uint32_t(c->r5));}
extern "C" void func_800B3C70(uint8_t*,recomp_context*c){assert(c->r5<=0x20000);c->r2=gpr(S32(heap));heap+=c->r5;allocations++;}
extern "C" void func_800AA9EC(uint8_t*,recomp_context*c){dispatched.push_back(uint32_t(c->r5));}
extern "C" void func_800A6520(uint8_t*,recomp_context*c){dispatched.push_back(uint32_t(c->r5));}
using namespace clay::input;
static Mapper combat;
namespace clay::input {std::string binding_label(Action a){return combat.prompt(a).button_name;}}
int main(){
 std::string error;assert(load_native_prompt_assets("config/controller_glyphs.json","assets/kenney-input-prompts-pixel/Tiles",error));std::vector<uint8_t> mem(9<<20);auto*rdram=mem.data();
 std::ifstream in("analysis/font_private/font.bin",std::ios::binary);std::vector<uint8_t>b((std::istreambuf_iterator<char>(in)),{});assert(b.size()==15148);
 const auto bundle=gpr(S32(0x80200000)),font=gpr(S32(0x80300000)),map=font+0x100,text=gpr(S32(0x80820000));
 for(size_t i=0;i<b.size();i++)MEM_BU(12+i,bundle)=b[i];auto u32=[&](size_t o){return(uint32_t(b[o])<<24)|(uint32_t(b[o+1])<<16)|(uint32_t(b[o+2])<<8)|b[o+3];};
 size_t chars=u32(0)-12;int sum=0,count=0;std::vector<std::pair<unsigned,uint32_t>> originals;
 for(int i=1;b[chars+i-1]!='\\';i++){unsigned c=b[chars+i-1];uint32_t p=uint32_t(bundle)+u32(i*8);MEM_W((c-33)*4,map)=gpr(S32(p));sum+=MEM_H(6,gpr(S32(p)));count++;originals.push_back({c,p});}
 MEM_W(0,font)=int32_t(bundle);MEM_W(4,font)=int32_t(map);MEM_H(0x26,font)=2;MEM_H(0x1c,font)=sum/count;auto original=mem;
 auto write=[&](std::string s){for(unsigned i=0;i<=s.size();i++)MEM_BU(i,text)=i==s.size()?0:uint8_t(s[i]);};
 auto measure=[&](std::string s){write(s);recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=gpr(S32(0x8081F000));c.r4=font;c.r5=text;prepare_native_prompt_measure(rdram,&c);func_80071738(rdram,&c);return uint32_t(c.r2);};
 const auto matrix=font+0x500;MEM_W(0,matrix)=0x3f800000;
 auto draw=[&](std::string s,unsigned variant=0){write(s);recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=gpr(S32(0x8081F000));c.r4=font;c.r7=matrix;MEM_W(0x10,c.r29)=int32_t(matrix);MEM_W(0x14,c.r29)=variant;MEM_W(0x18,c.r29)=int32_t(text);prepare_native_prompt_draw(rdram,&c);dispatched.clear();func_8007191C(rdram,&c);assert(!dispatched.empty());return dispatched;};
 auto family=[&](Family f){Mapper menu(true);Snapshot s;s.detected=f;s.keyboard_activity=f==Family::Keyboard;s.controller_activity=f!=Family::Keyboard;menu.update(s);combat.set_family(f==Family::Keyboard?Family::Auto:f);combat.update(s);publish_hud(menu);};
 auto glyph=[&](char c){return uint32_t(MEM_W((unsigned(c)-33)*4,map));};
 auto identical_tile=[&](uint32_t sprite,int tile){char path[256];snprintf(path,sizeof(path),"assets/kenney-input-prompts-pixel/Tiles/tile_%04d.png",tile);int w,h,n;auto*p=stbi_load(path,&w,&h,&n,4);assert(p&&w==16&&h==16);assert(MEM_HU(6,gpr(S32(sprite)))==15&&MEM_HU(14,gpr(S32(sprite)))==8);for(unsigned y=0;y<15;y++)for(unsigned x=0;x<15;x++){unsigned i=MEM_BU(48+y*8+x/2,gpr(S32(sprite)));i=(i>>(x%2?0:4))&15;unsigned actual=MEM_HU(16+i*2,gpr(S32(sprite)));auto*q=p+(y*16+x)*4;unsigned expected=q[3]<128?0:((q[0]>>3)<<11|(q[1]>>3)<<6|(q[2]>>3)<<1|1);assert(actual==expected);}stbi_image_free(p);};
 family(Family::Switch);assert(measure("press { to select / } to exit")==289);auto south=glyph('{');identical_tile(south,14);identical_tile(glyph('}'),13);draw("PRESS { TO SELECT / } TO EXIT");
 combat.remap(Action::Primary,Physical::North);measure(binding_marker(Action::Primary));auto north=glyph('%');identical_tile(north,15);assert(north!=south);draw(binding_marker(Action::Primary));assert(dispatched.back()==north);draw(menu_marker(Action::Primary));assert(dispatched.back()==south);
 // Later rows change map entries, while earlier GPU references remain distinct.
 auto first=draw(binding_marker(Action::Primary));auto second=draw(binding_marker(Action::Secondary));assert(first.back()!=second.back());identical_tile(first.back(),15);identical_tile(second.back(),16);
 family(Family::Xbox);draw(menu_marker(Action::Secondary));identical_tile(dispatched.back(),5);
 family(Family::Switch);draw(menu_marker(Action::Secondary));identical_tile(dispatched.back(),13);
 family(Family::Keyboard);assert(measure("PRESS { TO SELECT / } TO EXIT")==289);identical_tile(glyph('{'),155);identical_tile(glyph('}'),156);draw("PRESS { TO SELECT / } TO EXIT");assert(measure("press start")>0);assert(MEM_HU(6,gpr(S32(glyph('*'))))==31);
 combat.remap_key(Action::Primary,"Q");measure(binding_marker(Action::Primary));identical_tile(glyph('%'),85);draw(binding_marker(Action::Primary));identical_tile(dispatched.back(),85);draw(menu_marker(Action::Primary));identical_tile(dispatched.back(),155);
 for(auto[c,p]:originals){if(std::strchr("{}*^|<>",c)==nullptr)assert(glyph(char(c))==p);unsigned begin=p&0x7fffff;unsigned bytes=(MEM_BU(0,gpr(S32(p)))&1)?768:168;assert(!memcmp(mem.data()+begin,original.data()+begin,bytes));}
 for(Family f:{Family::Xbox,Family::PlayStation,Family::Switch,Family::Keyboard}){family(f);for(unsigned i=0;i<action_count;i++){auto sprites=draw(binding_marker(Action(i)));assert(sprites.back()>=0x80400000&&sprites.back()<heap);assert(MEM_BU(0,gpr(S32(sprites.back())))==0x30);}}
 family(Family::Switch);std::string six;for(unsigned i=0;i<6;i++)six+=menu_marker(Action(i));assert(measure(six)==90);assert(draw(six).size()==6);
 family(Family::Keyboard);
 // Existing native punctuation in another font must never be replaced.
 MEM_W(('%'-33)*4,map)=int32_t(originals.front().second);measure(binding_marker(Action::Primary));assert(glyph('%')==originals.front().second);identical_tile(glyph('&'),85);
 assert(!memcmp(mem.data()+(uint32_t(bundle)&0x7fffff)+12,original.data()+(uint32_t(bundle)&0x7fffff)+12,b.size()));assert(allocations==1);assert(heap<0x80420000);assert(native_prompt_text("PRESS START")=="press *");
 // Guard actual sprite allocations within8MiB; CPU-only Options stack/text9MiB.
 recomp_context bad{};bad.r29=gpr(S32(0x80900000));auto before=mem;prepare_native_prompt_draw(rdram,&bad);assert(mem==before);
 // Actual variant destructor: untouched variants alias the ORIGINAL base
 // bitmap, while the renderer borrows a new bank sprite in the base map.
 // RED proves the former code frees an original raw-bank interior pointer.
 const uint32_t variants=0x80302000;MEM_W(8,font)=int32_t(variants);MEM_W(12,font)=4;
 for(unsigned variant=0;variant<4;variant++)for(unsigned ch=0;ch<94;ch++)MEM_W((variant*94+ch)*4,gpr(S32(variants)))=0;
 for(auto[ch,p]:originals)for(unsigned variant=0;variant<4;variant++)MEM_W((variant*94+ch-33)*4,gpr(S32(variants)))=int32_t(p);
 // One original owned variant glyph must still be freed by the native code.
 const uint32_t owned_variant=0x80304000;MEM_W(('A'-33)*4,gpr(S32(variants)))=int32_t(owned_variant);
 draw(binding_marker(Action::Primary),1);auto raw_before=mem;recomp_context variant_ctx{};variant_ctx.r29=gpr(S32(0x807FF000));variant_ctx.r4=font;freed.clear();func_80072C4C(rdram,&variant_ctx);
 assert(std::any_of(freed.begin(),freed.end(),[&](uint32_t p){return p>=uint32_t(bundle)&&p<uint32_t(bundle)+b.size();}));
 mem=raw_before;rdram=mem.data();freed.clear();restore_native_prompt_font(rdram,font);
 // BOTHbase and all modifiedvariant aliases restored before comparison.
 variant_ctx={};variant_ctx.r29=gpr(S32(0x807FF000));variant_ctx.r4=font;func_80072C4C(rdram,&variant_ctx);assert((freed==std::vector<uint32_t>{owned_variant,variants}));
 freed.clear();recomp_context destroy{};destroy.r29=gpr(S32(0x807FF000));destroy.r4=font;func_8007145C(rdram,&destroy);assert((freed==std::vector<uint32_t>{uint32_t(bundle),uint32_t(map),uint32_t(font)}));
 // Reusing the same font address starts a new ownership lifetime.
 MEM_W(('{'-33)*4,map)=0x80305000;measure("press { to select");restore_native_prompt_font(rdram,font);assert(glyph('{')==0x80305000);assert(allocations==1);
 printf("Native icon bank bytes=%u\n",heap-0x80400000);puts("PASS: real native width/draw on9MiB Options stack; original62glyphs intact; genuine Kenney keyboard/controller pixels; explicit combat North vs menu South; immutable distinct GPU sprites; actual72C4C RED/GREEN variant cleanup andsame-address reuse; single bounded guestbank");
}
