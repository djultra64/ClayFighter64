// Hidden native options integration fixture by djultra64.
#include "port_options.h"
#include "graphics_config.h"
#include "input.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
extern "C" void do_break(uint32_t){std::abort();}
static bool capturing=false;
static clay::input::Action requested=clay::input::Action::Count;
static unsigned draws=0,matrices=0;
static unsigned native_letter_draws=0,native_exit_draws=0,native_seeds=0;
extern "C" void native_letter_draw_called(){native_letter_draws++;}
extern "C" void native_exit_draw_called(){native_exit_draws++;}
extern "C" void native_animation_seeded(uint8_t*rdram){
 native_seeds++;float radius;uint32_t bits=MEM_W(0,S32(0x80812000));memcpy(&radius,&bits,4);assert(radius>=480&&radius<=544);assert(MEM_HU(0,S32(0x80812500))<360);
}
static float first_glyph_x=0;
namespace clay::input {
void request_binding(Action a){requested=a;capturing=true;}
bool binding_capture_active(){return capturing;}
std::string binding_label(Action){return "Z / Cross";}
std::string binding_marker(Action){return "+";}
std::string menu_marker(Action){return "+";}
std::string binding_capture_status(){return "Press a key or button";}
void reset_combat_bindings(){}
void set_controller_family(Family){}
Family controller_family(){return Family::Auto;}
const char* family_name(Family f){return f==Family::Auto?"Auto":"Xbox";}
std::string native_prompt_text(const std::string&s){return s;}
void prepare_native_prompt_measure(uint8_t*,recomp_context*){}
}
extern "C" void func_800B0270(uint8_t*,recomp_context*){matrices++;}
extern "C" void func_800A1B98(uint8_t*,recomp_context*){}
extern "C" void func_800A6520(uint8_t*,recomp_context*c){if(!draws){uint32_t bits=uint32_t(c->r6);std::memcpy(&first_glyph_x,&bits,4);}draws++;}
extern "C" void func_800AA9EC(uint8_t*,recomp_context*){draws++;}
int main(int argc,char**argv){
 assert(argc==3);std::vector<uint8_t> ram(9<<20);auto*rdram=ram.data();constexpr int32_t st=S32(0x80310000),config=S32(0x80320000),game=S32(0x80330000),font=S32(0x80340000),map=font+0x100,bundle=S32(0x80200000);
 FILE*rom=fopen(argv[2],"rb");assert(rom);std::vector<uint8_t>image(0xc00000);assert(fread(image.data(),1,image.size(),rom)==image.size());fclose(rom);for(uint32_t i=0x1000;i<0xf9980;i++)rdram[((i-0x1000)+0x400)^3]=image[i];
 MEM_W(0,S32(0x800ddd3c))=st;MEM_W(0,S32(0x800ddd58))=config;MEM_W(0,S32(0x800ddd34))=game;MEM_W(0x6914,game)=font;MEM_W(12,st)=3;MEM_W(16,st)=3;
 const unsigned defaults[]={2,1,0,5,4,3};for(unsigned i=0;i<6;i++)MEM_H(0,S32(uint32_t(config)+8+i*4))=defaults[i];
 MEM_W(0,S32(0x800db254))=S32(0x800f1880);clay::port::prepare_native_options_labels(rdram);assert(MEM_W(0,S32(0x800db254))==S32(0x8081f100));
 clay::set_graphics_storage("/tmp/clay-options-fixture.cfg");clay::set_graphics_options(clay::GraphicsOptions{});clay::port::reset_options_cursor();recomp_context ctx{};ctx.f_odd=&ctx.f0.u32h;
 FILE*f=fopen(argv[1],"rb");assert(f);std::vector<uint8_t>b(15148);assert(fread(b.data(),1,b.size(),f)==b.size());fclose(f);auto u32=[&](size_t o){return(uint32_t(b[o])<<24)|(uint32_t(b[o+1])<<16)|(uint32_t(b[o+2])<<8)|b[o+3];};
 for(size_t i=0;i<b.size();i++)MEM_BU(0,S32(uint32_t(bundle)+12+uint32_t(i)))=b[i];size_t chars=u32(0)-12;int sum=0,count=0;
 for(int i=1;b[chars+i-1]!='\\';i++){unsigned c=b[chars+i-1];uint32_t p=uint32_t(bundle)+u32(i*8);MEM_W(int32_t((c-33)*4),map)=S32(p);sum+=MEM_H(S32(p),6);count++;}
 MEM_W(4,font)=map;MEM_H(0x26,font)=2;MEM_H(0x1c,font)=sum/count;MEM_H(0x10,font)=42;
 MEM_W(0,S32(0x800dde20))=0;MEM_W(0,S32(0x800dde24))=0x3f800000;MEM_W(0,S32(0x800dde2c))=0xbf800000;
 auto settle=[&](){for(unsigned frame=0;frame<400;frame++)assert(clay::port::draw_native_options(rdram,&ctx));};
 settle();
 auto press=[&](unsigned bits){MEM_W(0,S32(0x80118cb4))=bits;assert(clay::port::update_native_options(rdram,&ctx));MEM_W(0,S32(0x80118cb4))=0;};
 auto v=clay::port::options_view(rdram,3);assert(v.rows.size()==7);assert(v.rows.front()=="Aspect: 16:9");assert(v.rows[6]=="Game display");
 press(0x0100);assert(clay::port::options_view(rdram,3).selected==1);assert(clay::graphics_options().widescreen);press(0x0200);assert(clay::port::options_view(rdram,3).selected==0);press(0x8000);assert(!clay::graphics_options().widescreen);press(0x0100);press(0x8000);assert(clay::graphics_options().render_height==1080);assert(clay::graphics_options().width==1440);
 press(0x0200);for(unsigned i=0;i<3;i++)press(0x400);press(0x8000);auto game_display=clay::port::options_view(rdram,3);assert(game_display.title=="GAME DISPLAY"&&game_display.rows.size()==4);assert(game_display.rows[0].starts_with("Stun meter"));settle();press(0x8000);assert(MEM_HU(0x20,config)==1);assert(MEM_HU(0x22,config)==0);press(0x4000);assert(MEM_W(16,st)==3);assert(clay::port::options_view(rdram,3).rows.size()==7);assert(clay::port::options_view(rdram,3).selected==6);settle();
 press(0x4000);assert(MEM_W(16,st)==1);assert(!clay::port::update_native_options(rdram,&ctx));
 MEM_W(12,st)=2;MEM_W(16,st)=2;settle();auto controls=clay::port::options_view(rdram,2);assert(controls.rows[0].starts_with("Wobbly Kick"));assert(controls.rows[1].starts_with("Wobbly Punch"));assert(controls.rows[2].starts_with("Brutal Punch"));assert(controls.rows[3].starts_with("Fair Kick"));assert(controls.rows[4].starts_with("Fair Punch"));assert(controls.rows[5].starts_with("Brutal Kick"));MEM_H(0x1c,config)=1;assert(clay::port::options_view(rdram,2).rows[0].starts_with("Fair Punch"));press(0x8000);assert(capturing&&requested==clay::input::Action::Primary);press(0x4000);assert(MEM_W(16,st)==2);capturing=false;press(0x4000);assert(MEM_W(16,st)==2);assert(clay::port::options_view(rdram,2).status=="Press a key or button");press(0x4000);assert(MEM_W(16,st)==1);
 MEM_W(12,st)=3;MEM_W(16,st)=3;ctx.r4=123;ctx.r29=S32(0x807fe000);settle();matrices=0;draws=0;assert(clay::port::draw_native_options(rdram,&ctx));assert(native_letter_draws>100&&native_exit_draws>100&&native_seeds>10);assert(matrices==1);assert(first_glyph_x<0);assert(ctx.r4==123&&ctx.r29==S32(0x807fe000));assert(MEM_HU(0x10,font)==42);
 MEM_W(0,st)=1;MEM_W(16,st)=1;assert(clay::port::draw_native_options(rdram,&ctx));MEM_W(12,st)=1;assert(!clay::port::draw_native_options(rdram,&ctx));
 unsigned previous_seeds=native_seeds;MEM_W(12,st)=3;MEM_W(16,st)=3;MEM_W(0,st)=0;assert(clay::port::draw_native_options(rdram,&ctx));assert(native_seeds>previous_seeds);settle();
 // CPU-only native animation writes stay inside their original per-line capacities.
 MEM_W(0,S32(0x80812280))=0x5a5a5a5a;MEM_W(0,S32(0x808124a0))=0x5a5a5a5a;MEM_W(0,S32(0x808125a0))=0x5a5a5a5a;MEM_W(0,S32(0x80812740))=0x5a5a5a5a;
 assert(clay::port::draw_native_options(rdram,&ctx));for(uint32_t guard:{0x80812280u,0x808124a0u,0x808125a0u,0x80812740u})assert(uint32_t(MEM_W(0,S32(guard)))==0x5a5a5a5a);
std::remove("/tmp/clay-options-fixture.cfg");puts("PASS: native Display legacy toggles, graphics persistence, combat capture isolation, actual native radial seed/update/draw plus separately reseeded native exit, footer/subpage/reentry/animation bounds and ABI preservation");
}
