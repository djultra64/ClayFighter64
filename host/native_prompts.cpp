// Authentic native-font prompt integration by djultra64.
// Controller/keycap pixels: Kenney Input Prompts Pixel (CC0), notices retained.
#include "native_prompts.h"
#include "input_hud.h"
#include "json.hpp"
#include "stb/stb_image.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <mutex>
#include <stdexcept>
#include <vector>
extern "C" void func_800B3C70(uint8_t*,recomp_context*);
extern "C" void func_800B4408(uint8_t*,recomp_context*);
namespace clay::input {
namespace {
struct Icon {unsigned width=15,stride=8;std::vector<uint8_t> data;};
struct Bank {uint32_t begin=0;std::map<std::string,uint32_t> sprites;};
std::map<std::string,Icon> icons;
std::map<uint8_t*,Bank> banks;
std::map<std::pair<uint8_t*,uint32_t>,std::map<uint32_t,uint32_t>> borrowed_maps;
std::mutex icon_mutex;
std::atomic<unsigned> next_bucket=0;
constexpr char standard[]={'{','}','*','^','|','<','>'};
constexpr Action actions[]={Action::Primary,Action::Secondary,Action::Start,Action::CUp,Action::CDown,Action::CLeft,Action::CRight};
// Original62-glyph uppercase font has no entries at these six positions.
// Each text call binds these map slots to immutable guest sprites. GPU texture
// commands keep their own sprite addresses when the next row changes the map.
constexpr char extra[]={'%','&','+','=','\\','`'};
bool range(uint32_t p,size_t n,size_t limit){return(p&0xE0000000u)==0x80000000u&&uint64_t(p&0x1FFFFFFFu)+n<=limit;}
bool guest(uint32_t p,size_t n){return range(p,n,0x800000);}
bool cpu(uint32_t p,size_t n){return range(p,n,0x900000);}
uint32_t word(uint8_t*rdram,uint32_t p){return uint32_t(MEM_W(0,gpr(S32(p))));}
uint16_t half(uint8_t*rdram,uint32_t p){return MEM_HU(0,gpr(S32(p)));}
uint8_t byte(uint8_t*rdram,uint32_t p){return MEM_BU(0,gpr(S32(p)));}
void put(uint8_t*rdram,uint32_t p,uint8_t v){MEM_BU(0,gpr(S32(p)))=v;}
std::string read_text(uint8_t*rdram,uint32_t p){if(!cpu(p,1))return{};std::string out;for(unsigned i=0;i<512;i++){if(!cpu(p+i,1))return{};auto c=byte(rdram,p+i);if(!c)return out;out.push_back(char(c));}return{};}
std::string upper(std::string s){for(auto&c:s)c=char(std::toupper(static_cast<unsigned char>(c)));return s;}
uint32_t scratch(uint8_t*rdram,const std::string& text){
 thread_local unsigned bucket=next_bucket.fetch_add(1),slot=0;
 if(bucket>=8||text.size()>=512)throw std::runtime_error("Native prompt scratch capacity exceeded");
 uint32_t p=0x80800000+bucket*0x2000+(slot++%16)*512;for(unsigned i=0;i<=text.size();i++)put(rdram,p+i,i==text.size()?0:uint8_t(text[i]));return p;
}
Icon encode(const std::vector<std::array<uint8_t,4>>& rgba,unsigned width){
 Icon icon;icon.width=width;icon.stride=((width+1)/2+7)&~7u;icon.data.resize(48+15*icon.stride);
 const uint8_t header[]{0x30,0x0F,0,48,0,0,0,0,0,15,0,0,0,0,0,0};std::copy(std::begin(header),std::end(header),icon.data.begin());icon.data[7]=width;icon.data[15]=icon.stride;
 std::vector<uint16_t> palette{0};
 for(unsigned y=0;y<15;y++)for(unsigned x=0;x<width;x++){
  auto c=rgba[y*(width+1)+x];uint16_t col=c[3]<128?0:uint16_t((c[0]>>3)<<11|(c[1]>>3)<<6|(c[2]>>3)<<1|1);auto it=std::find(palette.begin(),palette.end(),col);unsigned index;
  if(it==palette.end()){index=palette.size();if(index>=16)throw std::runtime_error("Kenney icon exceeds native CI4 palette");palette.push_back(col);}else index=unsigned(it-palette.begin());
  icon.data[48+y*icon.stride+x/2]|=uint8_t(index<<(x%2?0:4));
 }
 for(unsigned i=0;i<palette.size();i++){icon.data[16+i*2]=palette[i]>>8;icon.data[17+i*2]=palette[i];}return icon;
}
Bank bank(uint8_t*rdram,recomp_context*ctx){
 uint32_t bytes=0;
 {std::lock_guard lock(icon_mutex);auto it=banks.find(rdram);if(it!=banks.end())return it->second;
  for(auto&[name,icon]:icons){(void)name;bytes+=(icon.data.size()+15)&~15u;}}
 if(!bytes||bytes>0x20000)throw std::runtime_error("Native prompt icon bank exceeds bounded capacity");
 // Native heap calls may yield. Never hold a host mutex while invoking them.
 recomp_context local=*ctx;local.f_odd=&local.f0.u32h;local.r29=ADD32(ctx->r29,-0x80);local.r4=gpr(S32(0x800EE4B0));local.r5=bytes;local.r6=0;func_800B3C70(rdram,&local);uint32_t p=local.r2;
 if(!guest(p,bytes))throw std::runtime_error("Native prompt icon bank allocation failed");Bank result;result.begin=p;
 for(auto&[name,icon]:icons){result.sprites[name]=p;for(unsigned i=0;i<icon.data.size();i++)put(rdram,p+i,icon.data[i]);p+=(icon.data.size()+15)&~15u;}
 uint32_t duplicate=0;
 {std::lock_guard lock(icon_mutex);auto[it,inserted]=banks.try_emplace(rdram,result);if(!inserted){duplicate=result.begin;result=it->second;}}
 if(duplicate){local.r4=gpr(S32(0x800EE4B0));local.r5=gpr(S32(duplicate));func_800B4408(rdram,&local);}return result;
}
Prompt fixed_prompt(Action action,Family family){Mapper mapper(true);mapper.set_family(family==Family::Keyboard?Family::Auto:family);Snapshot s;s.detected=family;s.keyboard_activity=family==Family::Keyboard;s.controller_activity=family!=Family::Keyboard;mapper.update(s);return mapper.prompt(action);}
std::string icon_name(Family family,std::string name){
 // SDL keyboard aliases retain the correct physical key; all standard defaults
 // have genuine keycaps. Unknown remapped keys keep their exact textual name.
 if(family==Family::Keyboard){if(name=="ENTER")name="Return";if(name=="ESC")name="Escape";for(auto&[key,icon]:icons){(void)icon;if(key.starts_with("keyboard/")&&upper(key.substr(9))==upper(name))return key;}}
 return std::string(family_name(family))+"/"+name;
}
void map_sprite(uint8_t*rdram,uint32_t font,uint32_t map,char marker,uint32_t sprite){
 if(!guest(map,94*4))return;uint32_t entry=map+(unsigned(marker)-33)*4;
 borrowed_maps[{rdram,font}].try_emplace(entry,word(rdram,entry));MEM_W(0,gpr(S32(entry)))=gpr(S32(sprite));
}
void map_font(uint8_t*rdram,uint32_t font,unsigned variant,char marker,uint32_t sprite){
 if(!guest(font,40))return;map_sprite(rdram,font,word(rdram,font+4),marker,sprite);uint32_t variants=word(rdram,font+8),count=word(rdram,font+12);
 if(variant&&variant<=count&&guest(variants,size_t(variant)*94*4))map_sprite(rdram,font,variants+(variant-1)*94*4,marker,sprite);
}
bool marker_available(uint8_t*rdram,uint32_t font,unsigned variant,char marker,const Bank& atlas){
 if(!guest(font,40))return false;
 auto available=[&](uint32_t map){if(!guest(map,94*4))return false;uint32_t p=word(rdram,map+(unsigned(marker)-33)*4);if(!p)return true;return std::any_of(atlas.sprites.begin(),atlas.sprites.end(),[&](const auto& pair){return pair.second==p;});};
 if(!available(word(rdram,font+4)))return false;uint32_t variants=word(rdram,font+8),count=word(rdram,font+12);
 return !variant||variant>count||!variants||available(variants+(variant-1)*94*4);
}
std::string transform(const std::string& source){if(upper(source)=="PRESS START")return "press *";return source;}
uint32_t prepare(uint8_t*rdram,recomp_context*ctx,uint32_t font,uint32_t text,unsigned variant){
 auto source=read_text(rdram,text);if(source.empty())return text;source=transform(source);
 if(source.find_first_of("{}*^|<>")==std::string::npos&&source.find('\x1e')==std::string::npos)return source==read_text(rdram,text)?text:scratch(rdram,source);
 auto view=native_prompt_snapshot();if(icons.empty())return text;auto atlas=bank(rdram,ctx);std::lock_guard lock(icon_mutex);std::string result;unsigned extra_slot=0;
 for(char marker:standard){size_t i=std::find(std::begin(standard),std::end(standard),marker)-std::begin(standard);auto p=view.prompts.size()==action_count?view.prompts[size_t(actions[i])]:fixed_prompt(actions[i],view.family);auto it=atlas.sprites.find(icon_name(view.family,p.button_name));if(it!=atlas.sprites.end())map_font(rdram,font,variant,marker,it->second);}
 for(unsigned i=0;i<source.size();i++){
  if(source[i]=='\x1e'&&i+2<source.size()&&source[i+2]=='\x1f'){
   unsigned code=uint8_t(source[i+1]);bool menu=code>='a'&&code<'a'+action_count;unsigned index=code-(menu?'a':'A');
   if(index>=action_count)throw std::runtime_error("Invalid native binding icon token");
   std::string name=menu?fixed_prompt(Action(index),view.family).button_name:binding_label(Action(index));auto it=atlas.sprites.find(icon_name(view.family,name));
   while(extra_slot<std::size(extra)&&!marker_available(rdram,font,variant,extra[extra_slot],atlas))extra_slot++;
   if(it!=atlas.sprites.end()&&extra_slot<std::size(extra)){char marker=extra[extra_slot++];map_font(rdram,font,variant,marker,it->second);result+=marker;}else result+=upper(name);
   i+=2;
  }else result+=source[i];
 }
 return result==read_text(rdram,text)?text:scratch(rdram,result);
}
}
bool load_native_prompt_assets(const std::string& config_path,const std::string& directory,std::string& error){
 try{
  std::ifstream input(config_path);if(!input)throw std::runtime_error("Cannot read controller glyph config");nlohmann::json config;input>>config;std::map<std::string,Icon> loaded;
  for(auto&[family,entries]:config.at("families").items())for(auto&[position,entry]:entries.items()){
   (void)position;bool horizontal=entry.value("layout",std::string{})=="horizontal";unsigned tiles=entry.at("tiles").size(),columns=horizontal?tiles:1,width=columns*16;std::vector<std::array<uint8_t,4>> image(width*16);unsigned column=0;
   for(auto tile:entry.at("tiles")){char name[32];snprintf(name,sizeof(name),"/tile_%04d.png",tile.get<int>());int w=0,h=0,n=0;auto*p=stbi_load((directory+name).c_str(),&w,&h,&n,4);if(!p||w!=16||h!=16){if(p)stbi_image_free(p);throw std::runtime_error("Invalid Kenney tile" );}
    for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++){unsigned src=(y*16+x)*4;if(p[src+3])std::copy_n(p+src,4,image[y*width+(horizontal?column*16:0)+x].begin());}stbi_image_free(p);column++;
   }
   loaded.emplace(family+"/"+entry.at("name").get<std::string>(),encode(image,width-1));
  }
  std::lock_guard lock(icon_mutex);if(!banks.empty())throw std::runtime_error("Native icon assets already resident");icons=std::move(loaded);return true;
 }catch(const std::exception&e){error=e.what();return false;}
}
void restore_native_prompt_font(uint8_t*rdram,uint64_t font){
 std::lock_guard lock(icon_mutex);auto found=borrowed_maps.find({rdram,uint32_t(font)});if(found==borrowed_maps.end())return;
 for(auto[entry,original]:found->second)if(guest(entry,4))MEM_W(0,gpr(S32(entry)))=gpr(S32(original));borrowed_maps.erase(found);
}
std::string binding_marker(Action action){if(size_t(action)>=action_count)throw std::out_of_range("Invalid binding action");return std::string{"\x1e"}+char('A'+size_t(action))+"\x1f";}
std::string menu_marker(Action action){if(size_t(action)>=action_count)throw std::out_of_range("Invalid menu action");return std::string{"\x1e"}+char('a'+size_t(action))+"\x1f";}
std::string native_prompt_text(const std::string& source){return transform(source);}
void prepare_native_prompt_measure(uint8_t*rdram,recomp_context*ctx){ctx->r5=gpr(S32(prepare(rdram,ctx,uint32_t(ctx->r4),uint32_t(ctx->r5),0)));}
void prepare_native_prompt_draw(uint8_t*rdram,recomp_context*ctx){uint32_t sp=ctx->r29;if(!cpu(sp,0x1C))return;uint32_t text=word(rdram,sp+0x18),variant=word(rdram,sp+0x14);MEM_W(0,gpr(S32(sp+0x18)))=gpr(S32(prepare(rdram,ctx,uint32_t(ctx->r4),text,variant)));}
}
