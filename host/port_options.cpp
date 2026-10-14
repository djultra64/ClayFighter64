// Native port options by djultra64. No external font or overlay renderer.
#include "port_options.h"
#include "graphics_config.h"
#include "input.h"
#include "native_prompts.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <mutex>
#include <cmath>

extern "C" {
void func_800B0270(uint8_t*,recomp_context*);
void func_800A1B98(uint8_t*,recomp_context*);
void func_80071738(uint8_t*,recomp_context*);
void func_8007191C(uint8_t*,recomp_context*);
void func_80063EFC(uint8_t*,recomp_context*);
void func_8006464C(uint8_t*,recomp_context*);
void func_80064C20(uint8_t*,recomp_context*);
void func_80064180(uint8_t*,recomp_context*);
void func_80064870(uint8_t*,recomp_context*);
void func_8006493C(uint8_t*,recomp_context*);
}
namespace clay::port {
namespace {
std::mutex mutex;
std::array<unsigned,4> cursor{};
unsigned last_menu=0;
bool was_capture=false;
std::string status;
bool legacy_display=false;
OptionsView previous_page;
bool subpage_exit=false;
unsigned animation_page=0;
bool animation_busy=false;
struct LetterLine {
 std::string text;
 std::array<uint32_t,160> radius{};
 std::array<uint16_t,80> horizontal{},angle{};
 std::array<uint32_t,80> vertical{};
 bool seeded=false,exit_seeded=false;
};
std::array<LetterLine,20> letter_lines{};
bool valid(uint32_t p,size_t n){return (p&0xe0000000u)==0x80000000u&&(p&0x1fffffffu)+n<=0x800000;}
uint32_t word(uint8_t* rdram,uint32_t p){return uint32_t(MEM_W(0,S32(p)));}
uint32_t state(uint8_t* rdram){uint32_t p=word(rdram,0x800ddd3c);return valid(p,0x1a80)?p:0;}
unsigned menu_id(uint8_t* rdram){auto p=state(rdram);return p?word(rdram,p+12):0;}
bool stable(uint8_t* rdram,uint32_t p){return word(rdram,p+12)==word(rdram,p+16)&&word(rdram,p+0x1a64)==0&&word(rdram,p)==0;}
constexpr std::array<input::Action,14> actions={input::Action::Primary,input::Action::Secondary,input::Action::CUp,input::Action::CDown,input::Action::CLeft,input::Action::CRight,input::Action::Trigger,input::Action::LeftShoulder,input::Action::RightShoulder,input::Action::Start,input::Action::Up,input::Action::Down,input::Action::Left,input::Action::Right};
// N64 A/B/C buttons are punch/kick actions, retaining original logical slots.
constexpr std::array<const char*,6> native_moves={"Wobbly Punch","Fair Punch","Brutal Punch","Wobbly Kick","Fair Kick","Brutal Kick"};
// Original 5A2A0 iterates config halfword PAIRS and ROM800DDD64 masks:
// CUp8,CLeft2,B4000,CRight1,CDown4,A8000. Config is the current saved permutation.
constexpr std::array<unsigned,6> native_slot={5,2,0,4,1,3};
std::string action_label(uint8_t* rdram,size_t i){
 if(i>=6)return std::array<const char*,8>{"N64 Z","L shoulder","R shoulder","Pause","Up","Down","Left","Right"}[i-6];
 auto config=word(rdram,0x800ddd58);if(!valid(config,0x20))return "Unconfigured action";
 unsigned move=MEM_HU(0,S32(config+8+native_slot[i]*4));
 return move<native_moves.size()?native_moves[move]:"Invalid native action";
}
constexpr std::array<input::Family,4> families={input::Family::Auto,input::Family::Xbox,input::Family::PlayStation,input::Family::Switch};
constexpr std::array<unsigned,5> resolutions={480,720,1080,1440,2160};
constexpr std::array<unsigned,3> rates={0,60,120};
constexpr std::array<unsigned,4> samples={0,2,4,8};
template<class T,size_t N> T cycle(T v,const std::array<T,N>& a,int d){auto it=std::find(a.begin(),a.end(),v);size_t i=it==a.end()?0:size_t(it-a.begin());return a[(i+N+(d<0?-1:1))%N];}
OptionsView view(uint8_t* rdram,unsigned menu);
void change(uint8_t* rdram,unsigned menu,unsigned row,int direction){
 if(std::getenv("CLAY_TRACE_GAME"))fprintf(stderr,"{\"event\":\"native_options_change\",\"menu\":%u,\"row\":%u,\"direction\":%d}\n",menu,row,direction);
 if(menu==2){
  if(row<actions.size()){input::request_binding(actions[row]);status="Release, then press a key or button";}
  else if(row==actions.size()){input::set_controller_family(cycle(input::controller_family(),families,direction));status="Family saved";}
  else {input::reset_combat_bindings();status="Combat bindings reset";}
  return;
 }
 if(!legacy_display&&row==6){previous_page=view(rdram,3);legacy_display=true;cursor[3]=0;status.clear();subpage_exit=true;return;}
 if(legacy_display){uint32_t config=word(rdram,0x800ddd58);if(valid(config,0x28)){uint32_t p=config+0x20+row*2;MEM_H(0,S32(p))=MEM_HU(0,S32(p))^1;status="Setting changed";}return;}
 auto o=graphics_options();
 switch(row){case 0:o.widescreen=!o.widescreen;break;case 1:o.render_height=cycle(o.render_height,resolutions,direction);break;
 case 2:o.filter=cycle(o.filter,std::array{TextureFilter::Original,TextureFilter::Nearest,TextureFilter::Linear},direction);break;
 case 3:o.refresh=cycle(o.refresh,rates,direction);break;case 4:o.fullscreen=!o.fullscreen;break;case 5:o.msaa=cycle(o.msaa,samples,direction);break;}
 o.height=o.render_height;o.width=o.render_height*(o.widescreen?16:4)/(o.widescreen?9:3);
 set_graphics_options(o);std::string error;status=persist_graphics_options(error)?"Settings saved":"Save failed: "+error;
}
OptionsView view(uint8_t* rdram,unsigned menu){
 OptionsView v;v.selected=cursor[menu];v.status=status;
 if(menu==2){v.title="BUTTON REMAP";for(size_t i=0;i<actions.size();i++)v.rows.push_back(action_label(rdram,i)+": "+input::binding_marker(actions[i]));v.rows.push_back(std::string("Controller: ")+input::family_name(input::controller_family()));v.rows.push_back("Reset bindings");if(input::binding_capture_active())v.status=input::binding_capture_status();}
 else {v.title="DISPLAY";auto o=graphics_options();v.rows={std::string("Aspect: ")+(o.widescreen?"16:9":"4:3"),"Resolution: "+std::to_string(o.render_height)+"p",std::string("Filter: ")+(o.filter==TextureFilter::Original?"Original":o.filter==TextureFilter::Nearest?"Nearest":"Linear"),"FPS: "+(o.refresh?std::to_string(o.refresh):"Original"),std::string("Fullscreen: ")+(o.fullscreen?"On":"Off"),"MSAA: "+(o.msaa?std::to_string(o.msaa)+"x":"Off")};
 if(legacy_display){v.title="GAME DISPLAY";v.rows.clear();uint32_t config=word(rdram,0x800ddd58);constexpr const char* names[]={"Stun meter","Super meter","Health meter","Character picture"};for(unsigned i=0;i<4;i++)v.rows.push_back(std::string(names[i])+": "+(valid(config,0x28)&&MEM_HU(0,S32(config+0x20+i*2))?"On":"Off"));}
 else v.rows.push_back("Game display");}
 return v;
}
}
void prepare_native_options_labels(uint8_t* rdram){
 constexpr uint32_t entry=0x800db22c+10*4,text=0x8081f100;
 uint32_t prior=word(rdram,entry);if(prior!=0x800f1880&&prior!=text)return;
 constexpr char label[]="button remap";for(size_t i=0;i<sizeof(label);i++)MEM_BU(0,S32(text+uint32_t(i)))=uint8_t(label[i]);
 MEM_W(0,S32(entry))=S32(text);
}
void reset_options_cursor(){std::lock_guard lock(mutex);cursor={};last_menu=0;was_capture=false;status.clear();legacy_display=false;previous_page={};subpage_exit=false;animation_page=0;animation_busy=false;letter_lines={};}
OptionsView options_view(uint8_t* rdram,unsigned menu){std::lock_guard lock(mutex);if(menu!=2&&menu!=3)return {};return view(rdram,menu);}
bool update_native_options(uint8_t* rdram,recomp_context*){
 auto p=state(rdram);unsigned menu=menu_id(rdram);
 if(std::getenv("CLAY_TRACE_GAME")&&p){
  uint32_t target=word(rdram,p+16),row=word(rdram,p+0x18),buttons=word(rdram,0x80118cb4)|word(rdram,0x80118cc4);bool ready=stable(rdram,p);
  static std::array<uint32_t,6> prior{~0u,~0u,~0u,~0u,~0u,~0u};
  std::array<uint32_t,6> current{menu,target,row,uint32_t(ready),buttons,menu<cursor.size()?cursor[menu]:0};
  if(current!=prior){prior=current;fprintf(stderr,"{\"event\":\"native_options_state\",\"menu\":%u,\"target\":%u,\"native_row\":%u,\"port_row\":%u,\"stable\":%s,\"buttons\":%u,\"capture\":%s}\n",menu,target,row,current[5],ready?"true":"false",buttons,input::binding_capture_active()?"true":"false");}
 }
 if(!p||(menu!=2&&menu!=3)||!stable(rdram,p)){std::lock_guard lock(mutex);last_menu=0;return false;}
 std::lock_guard lock(mutex);if(last_menu!=menu){cursor[menu]=0;status.clear();last_menu=menu;legacy_display=false;}
 uint32_t buttons=word(rdram,0x80118cb4)|word(rdram,0x80118cc4);
 // Capture consumes the next physical press in SDL; menu navigation is fixed.
 if(input::binding_capture_active()){was_capture=true;return true;}
 if(was_capture){was_capture=false;status=input::binding_capture_status();return true;}
 if(subpage_exit||animation_busy)return true;
 if(buttons&0x5000){if(menu==3&&legacy_display){previous_page=view(rdram,3);legacy_display=false;cursor[3]=6;status.clear();subpage_exit=true;return true;}MEM_W(0,S32(p+16))=1;last_menu=0;return true;}
 unsigned count=menu==2?unsigned(actions.size()+2):(legacy_display?4:7);
 
 if(buttons&0x0800)cursor[menu]=cursor[menu]>=2?cursor[menu]-2:((count-1-((count-1)%2!=(cursor[menu]%2))));
 else if(buttons&0x0400)cursor[menu]=cursor[menu]+2<count?cursor[menu]+2:cursor[menu]%2;
 else if(buttons&0x0300){unsigned other=cursor[menu]^1u;if(other<count)cursor[menu]=other;}
 else if(buttons&0x8000)change(rdram,menu,cursor[menu],1);
 return true;
}
bool draw_native_options(uint8_t* rdram,recomp_context* ctx){
 auto p=state(rdram);unsigned menu=menu_id(rdram);if(!p||(menu!=2&&menu!=3)){std::lock_guard lock(mutex);animation_page=0;animation_busy=false;subpage_exit=false;letter_lines={};return false;}
 uint32_t game=word(rdram,0x800ddd34);if(!valid(game,0x6918))return false;uint32_t font=word(rdram,game+0x6914);if(!valid(font,0x28))return false;
 std::lock_guard lock(mutex);
 OptionsView v=subpage_exit?previous_page:view(rdram,menu);
 unsigned page=menu+(legacy_display?4:0);
 if(animation_page!=page&&!subpage_exit){animation_page=page;letter_lines={};}
 bool outgoing=subpage_exit||word(rdram,p+12)!=word(rdram,p+16);
 bool any_busy=false;

 // Each call keeps the original context, font placement and guest stack intact.
 // Root allocates 9MiB RAM; these CPU-only strings/vectors never go to RSP DMA.
 constexpr uint32_t work=0x80810000,matrix=work+0x100,orient=work+0x180,transform=work+0x190,text=work+0x200;
 recomp_context local=*ctx;local.f_odd=&local.f0.u32h;local.r29=S32(work+0xf000);std::array<uint16_t,6> placement{};
 for(unsigned i=0;i<6;i++)placement[i]=MEM_HU(0,S32(font+0x10+i*2));
 for(unsigned i=0;i<3;i++){MEM_W(0,S32(orient+i*4))=MEM_W(0,S32(0x800dde20+i*4));MEM_W(0,S32(transform+i*4))=MEM_W(0,S32(0x800dde2c+i*4));}
 local.r4=S32(matrix);func_800B0270(rdram,&local);local.r4=S32(0x80110898);local.r5=S32(matrix);func_800A1B98(rdram,&local);
 auto line=[&](unsigned slot,std::string s,int x,int y,unsigned variant,int max_width){
  // Original font has only ASCII; long capture descriptions get their own footer.
  s=input::native_prompt_text(s);if(s.size()>511){s.resize(511);auto token=s.rfind('\x1e');if(token!=std::string::npos&&token+2>=s.size())s.resize(token);}for(size_t i=0;i<=s.size();i++)MEM_BU(0,S32(text+uint32_t(i)))=i==s.size()?0:uint8_t(s[i]);
  local.r4=S32(font);local.r5=S32(text);input::prepare_native_prompt_measure(rdram,&local);
  // Resolve the WHOLE LINE before splitting: preserve its dynamic icon slots.
  s.clear();for(unsigned i=0;i<80;i++){char ch=MEM_BU(0,S32(uint32_t(local.r5)+i));if(!ch)break;s+=ch;}
  for(unsigned i=0;i<=s.size();i++)MEM_BU(0,S32(text+i))=i<s.size()?uint8_t(s[i]):0;
  local.r4=S32(font);local.r5=S32(text);func_80071738(rdram,&local);int width=int(local.r2);
  // Fit in the original viewport through native horizontal/vertical scale vectors.
  float scale=width*2.0f>max_width?float(max_width)/width:2.0f;uint32_t bits;std::memcpy(&bits,&scale,4);
  float neg=-scale;std::memcpy(&bits,&neg,4);MEM_W(0,S32(transform))=bits;
  // Original radial animation keeps independent radius/angle/velocities for
  // every byte, including the real native prompt glyphs on this one line.
  auto& letters=letter_lines.at(slot);
  constexpr uint32_t radius=work+0x2000,horizontal=work+0x2400,angle=work+0x2500,vertical=work+0x2600;
  auto load=[&](){for(unsigned i=0;i<160;i++)MEM_W(0,S32(radius+i*4))=letters.radius[i];for(unsigned i=0;i<80;i++){MEM_H(0,S32(horizontal+i*2))=letters.horizontal[i];MEM_H(0,S32(angle+i*2))=letters.angle[i];MEM_W(0,S32(vertical+i*4))=letters.vertical[i];}};
  auto save=[&](){for(unsigned i=0;i<160;i++)letters.radius[i]=word(rdram,radius+i*4);for(unsigned i=0;i<80;i++){letters.horizontal[i]=MEM_HU(0,S32(horizontal+i*2));letters.angle[i]=MEM_HU(0,S32(angle+i*2));letters.vertical[i]=word(rdram,vertical+i*4);}};
  if(s.empty())return;
  if(letters.text!=s&&letters.seeded){letters={};letters.seeded=true;} // live binding/status edits are stationary
  letters.text=s;load();
  uint16_t font_tick=MEM_HU(0,S32(font+0x24));
  if(!letters.seeded){
   local.r4=S32(radius);local.r5=S32(horizontal);local.r6=S32(angle);local.r7=S32(vertical);
   MEM_W(0x10,local.r29)=s.size();MEM_W(0x14,local.r29)=120;MEM_W(0x18,local.r29)=120;func_80063EFC(rdram,&local);letters.seeded=true;
  }
  if(outgoing){
   // Original Options exit deliberately reuses this storage as ONE float/byte.
   // Its64870 stagger,64C20 acceleration and6493C drawer form a distinct mode.
   if(!letters.exit_seeded){local.r4=S32(radius);local.r5=s.size()*2;func_80064870(rdram,&local);letters.exit_seeded=true;}
   local.r4=S32(radius);local.r5=y;local.r6=-300;local.r7=S32(text);func_80064C20(rdram,&local);
  }
  else {local.r4=S32(radius);local.r5=S32(horizontal);local.r6=S32(angle);local.r7=S32(vertical);MEM_W(0x10,local.r29)=0;MEM_W(0x14,local.r29)=S32(text);func_8006464C(rdram,&local);}
  any_busy|=local.r2!=0;save();MEM_H(0,S32(font+0x24))=font_tick;
  // Keep the established fitted typography while invoking the ORIGINAL radial
  // glyph drawer. Its copied scale vectors are scoped to this call and restored.
  uint32_t vscale=outgoing?0x800ddddc:0x800dddc4,hscale=outgoing?0x800ddde4:0x800dddcc;
  uint32_t prior_vertical=word(rdram,vscale),prior_horizontal=word(rdram,hscale);
  float two=2.f;std::memcpy(&bits,&two,4);MEM_W(0,S32(vscale))=bits;
  std::memcpy(&bits,&neg,4);MEM_W(0,S32(hscale))=bits;
  float pen=x-width*scale/2;
  for(unsigned i=0;i<s.size();i++){
   MEM_BU(0,S32(text))=uint8_t(s[i]);MEM_BU(0,S32(text+1))=0;
   local.r4=S32(font);local.r5=S32(text);func_80071738(rdram,&local);int advance=int(local.r2);
   if(outgoing){local.r4=S32(radius+i*4);local.r5=-1;local.r6=int(pen);local.r7=y;MEM_W(0x10,local.r29)=variant;MEM_W(0x14,local.r29)=0;MEM_W(0x18,local.r29)=S32(text);func_8006493C(rdram,&local);}
   else {local.r4=S32(radius+i*8);local.r5=S32(angle+i*2);local.r6=-1;local.r7=int(pen);
    MEM_W(0x10,local.r29)=y;MEM_W(0x14,local.r29)=variant;MEM_W(0x18,local.r29)=0;MEM_W(0x1c,local.r29)=S32(text);func_80064180(rdram,&local);}

   pen+=advance*scale;
  }
  MEM_W(0,S32(vscale))=prior_vertical;MEM_W(0,S32(hscale))=prior_horizontal;
 };
 // Two columns fit the complete controls list without shrinking its height.
 // This hook replaces ALL stock category/title/body/footer text after its border.
 std::vector<std::string> shown;
 const std::string footer=input::menu_marker(input::Action::Primary)+" select / "+input::menu_marker(input::Action::Secondary)+" back";
 uint32_t capacity=word(rdram,0x80110880),start=0;
 unsigned budget=480;
 uint32_t table=word(rdram,0x801108dc),buffer=word(rdram,0x800de174),writer=word(rdram,0x80110898);
 if(buffer<8&&capacity>=512&&capacity<=16384&&valid(table,(buffer+1)*4)){
  start=word(rdram,table+buffer*4);
  if(valid(start,capacity*8)&&writer>=start&&writer<=start+capacity*8){
   unsigned remaining=(start+capacity*8-writer)/8;
   budget=remaining>256?(remaining-256)/32:0;
  }
 }
 auto glyphs=[](const std::string& source){auto text=input::native_prompt_text(source);return unsigned(std::count_if(text.begin(),text.end(),[](unsigned char c){return c>32&&c<128;}));};
 unsigned used=glyphs(footer)+glyphs(v.title);
 if(used<=budget)line(0,v.title,0,104,0,400);
 for(unsigned i=0;i<v.rows.size();++i){
  std::string row=(i==v.selected?"- ":"  ")+v.rows[i];
  if(used+glyphs(row)>budget)continue;used+=glyphs(row);
  shown.push_back(row);
  line(i+1,row,i%2==0?-170:170,66-int(i/2)*32,0,290);
 }
 if(glyphs(footer)<=budget)line(17,footer,0,-204,0,600);
 if(!v.status.empty()&&used+glyphs(v.status)<=budget)line(18,v.status,0,-180,0,600);
 if(std::getenv("CLAY_TRACE_GAME")){
  static std::string previous;
  std::string current=std::to_string(menu)+":"+std::to_string(v.selected)+":"+v.status+":"+std::to_string(shown.size());
  if(current!=previous){previous=current;uint32_t end=word(rdram,0x80110898);fprintf(stderr,"{\"event\":\"native_options_draw\",\"menu\":%u,\"selected\":%u,\"rows\":%zu,\"glyph_budget\":%u,\"commands_used\":%u,\"command_capacity\":%u}\n",menu,v.selected,shown.size(),budget,start&&end>=start?(end-start)/8:0,capacity);}
 }
 animation_busy=any_busy;
 if(subpage_exit&&!any_busy){subpage_exit=false;animation_page=0;letter_lines={};}
 for(unsigned i=0;i<6;i++)MEM_H(0,S32(font+0x10+i*2))=placement[i];
 return true;
}
}
