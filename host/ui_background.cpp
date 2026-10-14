// Native UI coverage by djultra64; RT64 extended GBI encoding follows its MIT header.
#include "ui_background.h"
#include "graphics_config.h"
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <set>
#include <array>
namespace clay::ui {
namespace {
bool valid(uint32_t p,size_t bytes){return p>=0x80000000u&&uint64_t(p)+bytes<=0x80800000ull;}
uint32_t word(uint8_t*rdram,uint32_t p){return uint32_t(MEM_W(0,S32(p)));}
void put(uint8_t*rdram,uint32_t p,uint32_t v){MEM_W(0,S32(p))=S32(v);}
struct Pool {uint32_t begin,end;};
bool pool(uint8_t*r,Pool&out){
 unsigned count=word(r,0x801108cc),index=word(r,0x800de174),commands=word(r,0x80110880);uint32_t table=word(r,0x801108dc);
 if(!count||count>8||index>=count||!commands||commands>65536||!valid(table,count*4))return false;
 uint32_t start=word(r,table+index*4);if(!valid(start,size_t(commands)*8))return false;
 out={start,start+commands*8};return true;
}
bool extent(uint8_t*r,uint32_t start,uint32_t end,unsigned extra){Pool p;return pool(r,p)&&start>=p.begin&&start<=end&&end<=p.end&&extra<=p.end-end&&!(start&7)&&!(end&7);}
void trace(const char* name,unsigned bytes){if(std::getenv("CLAY_TRACE_UI"))fprintf(stderr,"{\"event\":\"native_ui_coverage\",\"scope\":\"%s\",\"added_bytes\":%u}\n",name,bytes);}
bool decorate(uint8_t*r,uint32_t start,const std::vector<uint32_t>&before,const std::vector<uint32_t>&after){
 uint32_t end=display_list_cursor(r);unsigned extra=unsigned(before.size()+after.size())*4;
 if(!extent(r,start,end,extra))return false;
 // Never relocate a native branch/vertex/matrix target into the moved command span.
 for(uint32_t p=start;p<end;p+=8){uint32_t op=word(r,p)>>24,target=word(r,p+4)&0x1fffffffu;if((op==1||op==4||op==6)&&target>=(start&0x1fffffffu)&&target<(end&0x1fffffffu))return false;}
 std::vector<uint32_t> original((end-start)/4);for(size_t i=0;i<original.size();i++)original[i]=word(r,start+uint32_t(i)*4);
 uint32_t p=start;for(auto v:before){put(r,p,v);p+=4;}for(auto v:original){put(r,p,v);p+=4;}for(auto v:after){put(r,p,v);p+=4;}put(r,0x80110898,p);return true;
}
}
uint32_t display_list_cursor(uint8_t*r){return word(r,0x80110898);}
bool fix_scissor(uint8_t*r,uint32_t start){
 if(!graphics_options().widescreen)return false;uint32_t end=display_list_cursor(r),w=word(r,0x8010cfd0),h=word(r,0x8010cfd4);
 if(w!=320||h!=240||!extent(r,start,end,0))return false;
 const uint32_t expected=(w*4<<12)|((h-5)*4);
 for(uint32_t p=start;p<end;p+=8)if(word(r,p)==0xed000010u&&word(r,p+4)==expected){put(r,p,0xed000000);put(r,p+4,(w*4<<12)|(h*4));trace("full_scissor",0);return true;}
 return false;
}
void draw_fullscreen_fill(uint8_t*r,recomp_context*c,recomp_func_t*draw){
 uint32_t rect=uint32_t(c->r5),w=word(r,0x8010cfd0),h=word(r,0x8010cfd4),start=display_list_cursor(r);
 bool full=uint32_t(c->r4)==0x80110898&&w==320&&h==240&&valid(rect,16)&&word(r,rect)==0&&word(r,rect+4)==0&&word(r,rect+8)==w-1&&word(r,rect+12)==h-1;
 draw(r,c);if(!full||!graphics_options().widescreen)return;
 // Enable, push scissor, full viewport scissor, STRETCH; original F6 and RGBA stay exact.
 std::vector<uint32_t> prefix{0x00525464,0x10000064,0x64000017,0,0xed000000,(w*4<<12)|(h*4),0x64000033,1};
 std::vector<uint32_t> suffix{0x64000033,2,0x64000018,0};
 if(decorate(r,start,prefix,suffix))trace("fullscreen_fill",48);
}
uint32_t vertex_cursor(uint8_t*r){return word(r,0x801108a0);}
bool extend_circle_mask(uint8_t*r,uint32_t effect,uint32_t start){
 auto o=graphics_options();if(!o.widescreen||!valid(effect,0x18)||word(r,effect)==0||word(r,effect+4)!=2)return false;
 uint32_t end=vertex_cursor(r);constexpr unsigned bytes=192*16;
 if(end<start||end-start!=bytes||!valid(start,bytes))return false;
 auto*rdram=r;int edge=int(std::ceil(120*o.aspect()));if(edge<160||edge>480)return false;
 // Actual original396C4 emits64 triples: transparent inner circle, opaque rim,
 // opaque horizontal boundary anchor. Validate ALL anchors before modifying any.
 for(unsigned i=2;i<192;i+=3){uint32_t p=start+i*16;int x=MEM_H(0,S32(p));if((x!=160&&x!=-160)||MEM_BU(15,S32(p))!=255)return false;}
 for(unsigned i=2;i<192;i+=3){uint32_t p=start+i*16;int x=MEM_H(0,S32(p));MEM_H(0,S32(p))=x<0?-edge:edge;}
 trace("circle_outer_boundary",0);return true;
}
void draw_transition_band(uint8_t*r,recomp_context*c,recomp_func_t*draw){
 uint32_t rect=uint32_t(c->r5),start=display_list_cursor(r);
 bool full=uint32_t(c->r4)==0x80110898&&word(r,0x8010cfd0)==320&&word(r,0x8010cfd4)==240&&valid(rect,16)&&word(r,rect)==0&&word(r,rect+8)==319&&word(r,rect+4)<=word(r,rect+12)&&word(r,rect+12)<240;
 uint32_t y0=full?word(r,rect+4):0,y1=full?word(r,rect+12):0;
 draw(r,c);if(!full||!graphics_options().widescreen)return;
 // The original integer FILL edge leaves an upscale sample between an anchored
 // band and the fractional iris rim. Overlap only that inner edge by one RDP
 // effective quarter-pixel boundary; retain colors, outer edges and context.
 // RT64 fillRect quantizes lry with |=3, so +1 on an integer lower edge is
 // swallowed. Advancing top lry to the next integer is the minimum effective
 // change; the bottom band's upper edge accepts one quarter-pixel directly.
 bool top=y0==0&&y1<239,bottom=y1==239&&y0>0;
 uint32_t fill=0,end=display_list_cursor(r);unsigned matches=0;
 if((top||bottom)&&extent(r,start,end,0))for(uint32_t p=start;p<end;p+=8)
  if(word(r,p)==(0xf6000000u|(319u*4<<12)|(y1*4))&&word(r,p+4)==y0*4){fill=p;matches++;}
 if(decorate(r,start,{0x00525464,0x10000064,0x64000017,0,0xed000000,0x005003c0,0x64000033,1},{0x64000033,2,0x64000018,0})){
  if(matches==1){uint32_t p=fill+32+(bottom?4:0);put(r,p,top?word(r,p)+4:word(r,p)-1);}
  trace("transition_band",48);
 }
}
bool extend_selector_background_mesh(uint8_t*r,uint32_t part){
 constexpr unsigned bytes=0x6148;auto*rdram=r;
 if(!valid(part,bytes)||MEM_HU(0,S32(part))!=3||MEM_HU(0x18,S32(part))!=0xac)return false;
 struct Vertex {uint32_t p;int16_t x,current;std::array<uint8_t,14> other;};
 static thread_local uint32_t cached_part=0;static thread_local std::vector<Vertex> cached;
 std::set<uint32_t> vertices;bool ended=false;
 for(unsigned offset=0xe0;offset+8<=bytes;offset+=8){uint32_t command=word(r,part+offset),argument=word(r,part+offset+4);
  if(command==0xb8000000){ended=offset==0x2c60;break;}
  if(command>>24==4){unsigned n=(command>>10)&63;uint32_t p=0x80000000u|(argument&0x1fffffffu);if(n>32||p<part+0x2c68||uint64_t(p)+n*16>uint64_t(part)+bytes)return false;for(unsigned i=0;i<n;i++)vertices.insert(p+i*16);}
 }
 if(!ended||vertices.size()!=846)return false;
 bool same=part==cached_part&&cached.size()==846;
 if(same){unsigned i=0;for(auto p:vertices){auto&v=cached[i++];if(v.p!=p){same=false;break;}int16_t x=MEM_H(0,S32(p));if(x!=v.x&&x!=v.current){same=false;break;}for(unsigned j=0;j<14;j++)if(v.other[j]!=MEM_BU(0,S32(p+j+2))){same=false;break;}}}
 if(!same){std::vector<Vertex> fresh;unsigned outer=0;int left=0,right=0;for(auto p:vertices){int16_t x=MEM_H(0,S32(p));Vertex v{p,x,x,{}};for(unsigned j=0;j<14;j++)v.other[j]=MEM_BU(0,S32(p+j+2));fresh.push_back(v);outer+=std::abs(int(x))>160;left=std::min(left,int(x));right=std::max(right,int(x));}
  if(outer!=428||left!=-346||right!=346)return false;cached=std::move(fresh);cached_part=part;
 }
 auto o=graphics_options();double scale=o.widescreen?o.aspect()/(4.0/3.0):1.0;
 if(scale<1||scale>8.0/3.0)return false;unsigned changed=0;
 for(auto&v:cached){int x=v.x;if(std::abs(x)>160){int amount=std::abs(x);amount+=int(std::lround((amount-160)*(scale-1)*346.0/186.0));x=x<0?-amount:amount;}
  if(x!=MEM_H(0,S32(v.p))){MEM_H(0,S32(v.p))=x;changed++;}v.current=int16_t(x);
 }
 if(changed)trace("selector_industrial_mesh",0);return true;
}
}
