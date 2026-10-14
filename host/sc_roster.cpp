// Native 4x4 selector preparation, preserving the original three columns; djultra64.
#include "sc_roster.h"
#include "sc_assets.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <set>
#include <map>
extern "C" void func_800B3C70(uint8_t*,recomp_context*);
extern "C" void func_800B4408(uint8_t*,recomp_context*);
extern "C" void func_800A30C0(uint8_t*,recomp_context*);
extern "C" void func_800AFFE0(uint8_t*,recomp_context*);
extern "C" void func_80001008(uint8_t*,recomp_context*);
extern "C" void func_80001AA8(uint8_t*,recomp_context*);
extern "C" void func_80002DE0(uint8_t*,recomp_context*);
extern "C" void func_80003AA8(uint8_t*,recomp_context*);
extern "C" void func_80072D50(uint8_t*,recomp_context*);
extern "C" void func_80072FE8(uint8_t*,recomp_context*);
extern "C" void func_80091A8C(uint8_t*,recomp_context*);
extern "C" void func_8009215C(uint8_t*,recomp_context*);
namespace {
constexpr std::array<int,12> original{0,1,2,10,15,8,14,5,9,13,4,3};
constexpr std::array<int,16> expanded{0,1,2,6,10,15,8,11,14,5,9,12,13,4,3,16};
constexpr std::array<unsigned,4> donors{6,11,12,16};
constexpr std::array<uint32_t,4> portrait_rom{15368752,15380528,15400240,15416480};
constexpr uint32_t original_state_bytes=0xB8,flags_offset=0xB8,coordinates_offset=0xD8;
constexpr uint32_t original_bundle_bytes=146400,original_frames=107,idle_frames=8,portraits_per_donor=10,added_frames=4*portraits_per_donor;
constexpr uint32_t thumbnail_bytes=1360,expanded_bundle_bytes=original_bundle_bytes+added_frames*8+added_frames*thumbnail_bytes;
std::atomic_bool active{false};
uint32_t grid_source=0,grid_copy=0;
bool ram(uint32_t p,uint32_t n){return p>=0x80000000u&&uint64_t(p)+n<=0x80800000ull;}
uint32_t read32(const std::vector<uint8_t>& b,size_t o){if(o+4>b.size())throw std::runtime_error("Portrait package bounds");return(uint32_t(b[o])<<24)|(uint32_t(b[o+1])<<16)|(uint32_t(b[o+2])<<8)|b[o+3];}
uint8_t get(uint8_t*rdram,uint32_t p){return uint8_t(MEM_BU(0,gpr(S32(p))));}
void put(uint8_t*rdram,uint32_t p,uint8_t v){MEM_B(0,gpr(S32(p)))=v;}
uint32_t allocate(uint8_t*rdram,recomp_context& c,uint32_t bytes){c.r4=gpr(S32(0x800EE4B0));c.r5=bytes;c.r6=0;func_800B3C70(rdram,&c);uint32_t p=c.r2;if(!ram(p,bytes))throw std::runtime_error("Native selector allocation failed");return p;}
void release(uint8_t*rdram,recomp_context& c,uint32_t p){c.r4=gpr(S32(0x800EE4B0));c.r5=gpr(S32(p));func_800B4408(rdram,&c);}
using Portrait=std::array<uint8_t,thumbnail_bytes>;
std::array<Portrait,32> idle_portraits{};
bool idle_ready=false;
// Fixed anatomical windows in native idle world coordinates. These do not follow
// each frame's bounds: genuine pose movement stays visible within the crop.
struct Crop {int x,y,size;};
constexpr Crop head_crops[]{{-98,-100,102},{-15,-120,60},{17,-107,60},{-8,-133,46}};
void compose_head(uint8_t*rdram,uint32_t actor,uint32_t object,unsigned donor,Portrait& out,recomp_context& c){
 std::array<std::array<unsigned,3>,676> rgb{};
 const Crop crop=head_crops[donor];unsigned pieces=MEM_W(0x1054,gpr(S32(actor)));
 if(!pieces||pieces>64)throw std::runtime_error("Invalid native idle piece count");
 for(unsigned piece=0;piece<pieces;piece++){
  uint32_t p=actor+0x1058+piece*8;int id=MEM_H(0,gpr(S32(p))),x=MEM_H(2,gpr(S32(p))),y=MEM_H(4,gpr(S32(p)));unsigned flags=MEM_BU(6,gpr(S32(p)));
  c.r4=gpr(S32(object));c.r5=id;func_80002DE0(rdram,&c);uint32_t sprite=c.r2;
  if(!ram(sprite,16)||get(rdram,sprite)!=0x43||MEM_HU(2,gpr(S32(sprite)))!=16)throw std::runtime_error("Unsupported native idle sprite pixels");
  int width=MEM_HU(6,gpr(S32(sprite))),height=MEM_HU(8,gpr(S32(sprite))),hx=MEM_H(10,gpr(S32(sprite))),hy=MEM_H(12,gpr(S32(sprite))),stride=MEM_HU(14,gpr(S32(sprite)));
  if(width<1||height<1||width>512||height>512||stride<width*4||!ram(sprite,16+height*stride)||flags>1)throw std::runtime_error("Invalid native idle sprite bounds");
  int left=x-(flags&1?width-hx:hx),top=-y-hy;
  for(unsigned py=1;py<25;py++)for(unsigned px=1;px<25;px++){
   std::array<unsigned,3> sum{};unsigned alpha=0;
   // Supersample the original pixels; no synthetic artwork or motion.
   for(unsigned sy=0;sy<4;sy++)for(unsigned sx=0;sx<4;sx++){
    int ix=crop.x+int((px*4+sx)*crop.size/104)-left,iy=crop.y+int((py*4+sy)*crop.size/104)-top;
    if(ix<0||ix>=width||iy<0||iy>=height)continue;if(flags&1)ix=width-1-ix;
    uint32_t q=sprite+16+iy*stride+ix*4;unsigned a=get(rdram,q+3);alpha+=a;
    for(unsigned k=0;k<3;k++)sum[k]+=get(rdram,q+k)*a;
   }
   auto& dest=rgb[py*26+px];for(unsigned k=0;k<3;k++)dest[k]=(sum[k]+dest[k]*(4080-alpha))/4080;
  }
 }
 out.fill(0);constexpr uint8_t header[]{0x11,0xFF,2,0x10,0,0,0,26,0,26,0,0,0,0,0,32};std::copy(std::begin(header),std::end(header),out.begin());
 std::map<uint16_t,unsigned> histogram;std::array<uint16_t,676> colors{};
 for(unsigned i=0;i<rgb.size();i++){auto p=rgb[i];colors[i]=uint16_t((p[0]>>3)<<11|(p[1]>>3)<<6|(p[2]>>3)<<1|1);if(colors[i]!=1)histogram[colors[i]]++;}
 std::vector<std::pair<unsigned,uint16_t>> ranked;for(auto[col,n]:histogram)ranked.push_back({n,col});std::sort(ranked.rbegin(),ranked.rend());
 std::vector<uint16_t> palette{1};for(auto[n,col]:ranked){if(palette.size()==256)break;palette.push_back(col);}
 for(unsigned i=0;i<256;i++){uint16_t col=i<palette.size()?palette[i]:1;out[16+i*2]=col>>8;out[17+i*2]=col;}
 for(unsigned y=0;y<26;y++)for(unsigned x=0;x<26;x++){
  uint16_t color=colors[y*26+x];unsigned best=0,error=~0u;
  for(unsigned i=0;i<palette.size();i++){int r=int(color>>11)-int(palette[i]>>11),g=int(color>>6&31)-int(palette[i]>>6&31),b=int(color>>1&31)-int(palette[i]>>1&31);unsigned e=r*r+g*g+b*b;if(e<error){error=e;best=i;}}
  out[528+y*32+x]=best;
 }
}
void generate_idle_portraits(uint8_t*rdram,recomp_context& c){
 if(idle_ready)return;
 std::array<uint8_t,0x280> cache{};for(unsigned i=0;i<cache.size();i++){cache[i]=get(rdram,0x800FF140+i);put(rdram,0x800FF140+i,0);}
 uint32_t object=0,actor=0;const uint32_t rng=MEM_W(0,gpr(S32(0x800EE5B0)));
 auto cleanup=[&]{if(object){c.r4=gpr(S32(object));func_80001AA8(rdram,&c);object=0;}func_80003AA8(rdram,&c);if(actor){release(rdram,c,actor);actor=0;}};
 try{
  for(unsigned d=0;d<4;d++){
   c.r4=donors[d];c.r5=0;func_80001008(rdram,&c);object=c.r2;
   if(!ram(object,64))throw std::runtime_error("Native idle character loader failed");
   actor=allocate(rdram,c,0x3484);for(unsigned i=0;i<0x3484;i++)put(rdram,actor+i,0);c.r4=gpr(S32(actor));func_80072D50(rdram,&c);c.r4=gpr(S32(actor));c.r5=MEM_W(12,gpr(S32(object)));func_80072FE8(rdram,&c);
   uint32_t definition=MEM_W(0x1D4,gpr(S32(actor))),table=MEM_W(0x1DC,gpr(S32(actor)));
   c.r4=gpr(S32(actor+0xFB4));c.r5=gpr(S32(definition+MEM_HU(0,gpr(S32(table)))));c.r6=gpr(S32(-1));c.r7=gpr(S32(-1));func_80091A8C(rdram,&c);
   constexpr unsigned normal_ticks[]{0,3,6,9,12,15,18,21},zappa_ticks[]{0,3,6,9,13,16,19,23};unsigned frame=0;
   for(unsigned tick=0;frame<8;tick++){c.r4=gpr(S32(actor+0xFB4));func_8009215C(rdram,&c);if(tick==(d==3?zappa_ticks[frame]:normal_ticks[frame]))compose_head(rdram,actor,object,d,idle_portraits[d*8+frame++],c);}
   cleanup();
  }
 }catch(...){cleanup();for(unsigned i=0;i<cache.size();i++)put(rdram,0x800FF140+i,cache[i]);throw;}
 for(unsigned i=0;i<cache.size();i++)put(rdram,0x800FF140+i,cache[i]);
 if(uint32_t(MEM_W(0,gpr(S32(0x800EE5B0))))!=rng)throw std::runtime_error("Native idle portrait generation changed RNG");
 idle_ready=true;
}

}
void clay_roster::configure_image(const std::vector<uint8_t>& image,int demo_character){
 if(image.size()!=15439696||clay_sc::sha256(image)!="26ccae86b452d4bacadf429e0b241751aeef58a41d676e7d6173974414dafd7d")throw std::runtime_error("Unverified optional roster portrait package");
 for(auto p:portrait_rom)if(read32(image,p)!=0x01000100||read32(image,p+4)!=2)throw std::runtime_error("Invalid verified portrait manifest");
 // The original model importer verifies the unchanged original experimental prefix independently.
 std::vector<uint8_t> prefix(image.begin(),image.begin()+15368752);clay_sc::configure_asset_image(prefix,demo_character);
 idle_ready=false;active.store(true,std::memory_order_release);
}
bool clay_roster::enabled(){return active.load(std::memory_order_acquire);}
extern "C" int clay_sc_roster_count(){return clay_roster::enabled()?16:12;}
extern "C" int clay_sc_roster_columns(){return clay_roster::enabled()?4:3;}
extern "C" int clay_sc_roster_state_bytes(){return clay_roster::enabled()?0x118:original_state_bytes;}
extern "C" int clay_sc_roster_flag_offset(){return clay_roster::enabled()?flags_offset:0x2C;}
extern "C" int clay_sc_roster_row_stride(uint64_t row){return int32_t(row)*clay_sc_roster_columns();}
extern "C" int clay_sc_roster_id_from_slot(uint64_t slot){uint32_t i=uint32_t(slot);if(i>=unsigned(clay_sc_roster_count()))throw std::out_of_range("Native selector slot outside roster");return clay_roster::enabled()?expanded[i]:original[i];}
extern "C" int clay_sc_roster_slot_from_id(uint64_t id){for(unsigned i=0;i<unsigned(clay_sc_roster_count());i++)if(clay_sc_roster_id_from_slot(i)==int32_t(id))return i;return -1;}
extern "C" int clay_sc_roster_extra_allowed(uint64_t id){if(!clay_roster::enabled())return 0;for(auto d:donors)if(d==uint32_t(id))return 1;return 0;}
extern "C" int clay_sc_roster_unlock_flags(uint64_t flags){return int32_t(flags)|(clay_roster::enabled()?8:0);}
extern "C" int clay_sc_roster_secret_shift(uint64_t slot){
 if(!clay_roster::enabled())return int32_t(slot)+23;
 int id=clay_sc_roster_id_from_slot(slot);return id==13?0:id==4?1:id==3?2:3;
}
extern "C" int clay_sc_roster_slot_unlocked(uint8_t*rdram,uint64_t slot){
 if(!clay_roster::enabled())return int32_t(slot)<9;
 int id=clay_sc_roster_id_from_slot(slot);if(id!=13&&id!=4&&id!=3)return 1;
 uint32_t state=MEM_W(0,gpr(S32(0x800DDD40)));if(!ram(state,0x22))throw std::runtime_error("Native selector unlock state unavailable");return(MEM_HU(0x20,gpr(S32(state)))&(1u<<clay_sc_roster_secret_shift(slot)))!=0;
}
extern "C" uint64_t clay_sc_roster_coordinate_address(uint8_t*rdram,uint64_t original_address){
 if(!clay_roster::enabled())return original_address;
 uint32_t offset=uint32_t(original_address)-0x800DABE8u;if(offset>=64)throw std::runtime_error("Native selector coordinate outside grid");return ADD32(clay_sc_roster_coordinates(rdram),offset);
}
extern "C" uint64_t clay_sc_roster_coordinates(uint8_t*rdram){if(!clay_roster::enabled())return gpr(S32(0x800DABE8));uint32_t state=MEM_W(0,gpr(S32(0x800DDD30)));if(!ram(state,0x118))throw std::runtime_error("Expanded selector state unavailable");return gpr(S32(state+coordinates_offset));}
extern "C" void clay_sc_roster_install(uint8_t*rdram,recomp_context*ctx){
 // Original5DDCC also resets an existing selector (branch5DDE4), retaining
 // its loaded mesh. Keep its already shifted source and added column paired
 // until original5E2CC destroys the scene; recreating from the shifted source
 // would apply the grid offset twice and fail the native topology contract.
 if(!clay_roster::enabled())return;
 uint32_t state=MEM_W(0,gpr(S32(0x800DDD30)));if(!ram(state,0x118))throw std::runtime_error("Expanded selector allocation unavailable");
 constexpr int16_t xs[]{-146,-70,7,83},ys[]{70,-7,-83,-160};
 for(unsigned row=0;row<4;row++)for(unsigned col=0;col<4;col++){unsigned slot=row*4+col;MEM_H(flags_offset+slot*2,gpr(S32(state)))=col==3?0x4000:0;MEM_H(coordinates_offset+slot*4,gpr(S32(state)))=xs[col];MEM_H(coordinates_offset+slot*4+2,gpr(S32(state)))=ys[row];}
 uint32_t old=MEM_W(0x20,gpr(S32(state)));
 recomp_context c=*ctx;c.f_odd=&c.f0.u32h;c.r2=gpr(S32(old));clay_sc_roster_augment_portraits(rdram,&c,0xB6BA90);MEM_W(0x20,gpr(S32(state)))=c.r2;
}
extern "C" void clay_sc_roster_augment_portraits(uint8_t*rdram,recomp_context*ctx,uint64_t source_rom){
 if(!clay_roster::enabled()||uint32_t(source_rom)!=0xB6BA90)return;
 uint32_t old=uint32_t(ctx->r2);
if(!ram(old,12))throw std::runtime_error("Original selector portrait bank unavailable");
 unsigned count=MEM_W(4,gpr(S32(old)));if(count==original_frames+added_frames)return;
 if(count!=original_frames||!ram(old,original_bundle_bytes))throw std::runtime_error("Unexpected original selector portrait capacity");
 recomp_context c=*ctx;c.f_odd=&c.f0.u32h;c.r29=ADD32(ctx->r29,-0x80);uint32_t bank=allocate(rdram,c,expanded_bundle_bytes);
 for(uint32_t i=0;i<12;i++){put(rdram,bank+i,get(rdram,old+i));}
 MEM_W(4,gpr(S32(bank)))=original_frames+added_frames;
 for(unsigned i=0;i<original_frames;i++){MEM_W(12+i*8,gpr(S32(bank)))=MEM_W(12+i*8,gpr(S32(old)))+added_frames*8;MEM_W(16+i*8,gpr(S32(bank)))=MEM_W(16+i*8,gpr(S32(old)));}
 uint32_t old_payload=12+original_frames*8,new_payload=old_payload+added_frames*8;
 for(uint32_t i=old_payload;i<original_bundle_bytes;i++)put(rdram,bank+new_payload+i-old_payload,get(rdram,old+i));
 generate_idle_portraits(rdram,c);
 for(unsigned d=0;d<4;d++){
  uint32_t offset=original_bundle_bytes+added_frames*8+d*portraits_per_donor*thumbnail_bytes;
  for(unsigned frame=0;frame<portraits_per_donor;frame++){unsigned i=original_frames+d*portraits_per_donor+frame;uint32_t frame_offset=offset+frame*thumbnail_bytes;
   for(uint32_t j=0;j<thumbnail_bytes;j++)put(rdram,bank+frame_offset+j,idle_portraits[d*idle_frames+(frame<idle_frames?frame:frame-2)][j]);
   MEM_W(12+i*8,gpr(S32(bank)))=frame_offset;MEM_W(16+i*8,gpr(S32(bank)))=thumbnail_bytes;}
  // Native selector plays [idle,selected), then selected and selected+1 for hover/confirmation.
  MEM_H(donors[d]*4,gpr(S32(0x800DABA0)))=original_frames+d*portraits_per_donor;
  MEM_H(donors[d]*4+2,gpr(S32(0x800DABA0)))=original_frames+d*portraits_per_donor+idle_frames;
 }
 ctx->r2=gpr(S32(bank));release(rdram,c,old);
 fprintf(stderr,"{\"event\":\"experimental_sc_roster_portraits\",\"frames\":147,\"slots\":16,\"bank\":\"%08X\"}\n",bank);
}

extern "C" uint64_t clay_sc_roster_extend_grid(uint8_t*rdram,recomp_context*ctx,uint64_t original_part){
 if(!clay_roster::enabled())return 0;
 const uint32_t part=original_part,bytes=0x6148;
 if(part==grid_source&&grid_copy)return gpr(S32(grid_copy));
 if(!ram(part,bytes)||MEM_HU(0,gpr(S32(part)))!=3||MEM_HU(0x18,gpr(S32(part)))!=0xAC)throw std::runtime_error("Unsupported original selector mesh");
 std::array<uint32_t,32> vertices{};unsigned count=0,grid_triangles=0,column_triangles=0;std::set<uint32_t> shifted;
 recomp_context local=*ctx;local.f_odd=&local.f0.u32h;local.r29=ADD32(ctx->r29,-0x80);
 if(grid_copy)release(rdram,local,grid_copy);
 uint32_t copy=allocate(rdram,local,bytes);for(uint32_t i=0;i<bytes;i++)put(rdram,copy+i,get(rdram,part+i));
 bool ended=false;
 for(uint32_t offset=0xE0;offset+8<=bytes;offset+=8){
  uint32_t word=MEM_W(offset,gpr(S32(part))),argument=MEM_W(offset+4,gpr(S32(part)));
  if(word==0xB8000000){ended=true;break;}
  if(word>>24==4){
   count=(word>>10)&63;if(count>32)throw std::runtime_error("Selector vertex count");
   uint32_t address=0x80000000u|argument;
   if(address<part||uint64_t(address)+count*16>uint64_t(part)+bytes)throw std::runtime_error("Selector vertex relocation bounds");
   for(unsigned i=0;i<count;i++)vertices[i]=address+i*16;
   MEM_W(offset+4,gpr(S32(copy)))=(copy+address-part)&0x7FFFFFFF;
  }else if(word>>24==0xB1){
   uint32_t filtered[2]{0,0};
   for(unsigned triangle=0;triangle<2;triangle++){
    uint32_t encoded=(triangle?argument:word)&0xFFFFFF;bool grid=true,column=true;std::array<uint32_t,3> triangle_vertices{};
    for(unsigned i=0;i<3;i++){
     unsigned index=(encoded>>(16-i*8)&255)/2;if(index>=count)throw std::runtime_error("Selector triangle index");
     uint32_t vertex=vertices[index];triangle_vertices[i]=vertex;
     int x=MEM_H(0,gpr(S32(vertex))),y=MEM_H(2,gpr(S32(vertex))),z=MEM_H(4,gpr(S32(vertex)));
     grid &= x>=-120&&x<=120&&y>=-240&&y<=80&&(z==47||z==88);column &= x>=35;
    }
    if(grid){grid_triangles++;shifted.insert(triangle_vertices.begin(),triangle_vertices.end());if(column){filtered[triangle]=encoded;column_triangles++;}}
   }
   MEM_W(offset,gpr(S32(copy)))=0xB1000000|filtered[0];MEM_W(offset+4,gpr(S32(copy)))=filtered[1];
  }
 }
 if(!ended||grid_triangles!=74||column_triangles!=28)throw std::runtime_error("Original selector grid topology mismatch");
 for(uint32_t vertex:shifted){int x=MEM_H(0,gpr(S32(vertex)));MEM_H(0,gpr(S32(vertex)))=x-38;MEM_H(0,gpr(S32(copy+vertex-part)))=x+38;}
 grid_source=part;grid_copy=copy;return gpr(S32(copy));
}
extern "C" void clay_sc_roster_draw_grid(uint8_t*rdram,recomp_context*ctx){
 recomp_context second=*ctx;second.f_odd=&second.f0.u32h;uint64_t extension=clay_sc_roster_extend_grid(rdram,ctx,ctx->r5);
 func_800A30C0(rdram,ctx);
 if(extension){second.r5=extension;func_800A30C0(rdram,&second);}
}

extern "C" void clay_sc_roster_release_grid(uint8_t*rdram,recomp_context*ctx){
 if(grid_copy){recomp_context local=*ctx;local.f_odd=&local.f0.u32h;local.r29=ADD32(ctx->r29,-0x80);release(rdram,local,grid_copy);}grid_source=grid_copy=0;
}
