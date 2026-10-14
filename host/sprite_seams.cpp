// Native sprite slice continuity by djultra64.
#include "sprite_seams.h"
#include <vector>
#include <cstdlib>
#include <cstdio>
namespace clay {
namespace {
struct Scope {uint32_t sprite=0,container=0,dl=0,vertices=0;bool active=false;};
thread_local Scope scope;
bool valid(uint32_t p,unsigned n){return (p&0xe0000000u)==0x80000000u&&(p&0x1fffffffu)+n<=0x800000;}
uint32_t word(uint8_t* rdram,uint32_t p){return uint32_t(MEM_W(0,S32(p)));}
struct Strip {uint32_t image=0,load=0,tile_size=0,vertex=0;unsigned line=0,size=0,format=0;};
}
void begin_sprite_slices(uint8_t* rdram,recomp_context* ctx){
 scope={};scope.sprite=uint32_t(ctx->r5);scope.container=uint32_t(ctx->r4);
 if(!valid(scope.sprite,16)||!valid(scope.container,4))return;
 scope.dl=word(rdram,scope.container);scope.vertices=word(rdram,0x801108a0);
 scope.active=valid(scope.dl,8)&&valid(scope.vertices,16);
}
void limit_sprite_slice_rows(uint8_t* rdram,recomp_context* ctx,unsigned stack_offset){
 if(!scope.active||!valid(uint32_t(ctx->r29)+stack_offset,4))return;
 const unsigned format=MEM_BU(0,S32(scope.sprite)),size=format&15;
 const unsigned width=MEM_HU(0,S32(scope.sprite+6)),height=MEM_HU(0,S32(scope.sprite+8));
 // Four-bit DMA uses packed sixteen-bit load coordinates; the postprocessor
 // intentionally rejects that alias until its distinct footprint is verified.
 if(size==0||size>3||!width||width>1024||!height||height>1024)return;
 const unsigned tmem_row=size==3?2*((width*2+7)&~7u):(((width*(1u<<size)+1)/2+7)&~7u);
 const unsigned capacity=(format&0x10)?2048:4096;
 if(!tmem_row||capacity/tmem_row<2)return;
 const unsigned maximum=capacity/tmem_row-1,current=uint32_t(MEM_W(int32_t(stack_offset),ctx->r29));
 if(!current||current<=maximum)return;
 // Reducing the chunk size emits more geometry. Check the original active
 // display-list and vertex allocations before allowing those extra slices.
 const unsigned buffers=word(rdram,0x801108cc),index=word(rdram,0x800de174),commands=word(rdram,0x80110880),vertices=word(rdram,0x80110890);
 const auto dl_table=word(rdram,0x801108dc),vertex_table=word(rdram,0x801108e4);
 if(!buffers||buffers>8||index>=buffers||commands>65536||vertices>65536||!valid(dl_table,buffers*4)||!valid(vertex_table,buffers*4))return;
 const auto dl_start=word(rdram,dl_table+index*4),vertex_start=word(rdram,vertex_table+index*4);
 const unsigned slices=(height+maximum-1)/maximum,dl_required=(64+32*slices)*8,vertex_required=slices*64;
 if(!valid(dl_start,commands*8)||!valid(vertex_start,vertices*16)||scope.dl<dl_start||scope.vertices<vertex_start||scope.dl>dl_start+commands*8||scope.vertices>vertex_start+vertices*16||dl_required>dl_start+commands*8-scope.dl||vertex_required>vertex_start+vertices*16-scope.vertices)return;
 MEM_W(int32_t(stack_offset),ctx->r29)=maximum;
}
unsigned end_sprite_slices(uint8_t* rdram){
 if(!scope.active)return 0;scope.active=false;
 const auto end=word(rdram,scope.container),vertex_end=word(rdram,0x801108a0);
 if(end<scope.dl||end-scope.dl>65536||!valid(scope.dl,end-scope.dl)||(end-scope.dl)%8||vertex_end<scope.vertices||vertex_end-scope.vertices>65536||!valid(scope.vertices,vertex_end-scope.vertices))return 0;
 const unsigned width=MEM_HU(0,S32(scope.sprite+6)),height=MEM_HU(0,S32(scope.sprite+8)),stride=MEM_HU(0,S32(scope.sprite+14)),offset=MEM_HU(0,S32(scope.sprite+2));
 if(!width||width>1024||!height||height>1024||!stride||stride>4096||offset<16||offset>4096||!valid(scope.sprite,offset+stride*height))return 0;
 const auto pixels=(scope.sprite+offset)&0x1fffffffu;
 std::vector<Strip> strips;Strip current;
 for(uint32_t p=scope.dl;p<end;p+=8){const auto w0=word(rdram,p),w1=word(rdram,p+4);switch(w0>>24){
 case 0xfd:current={};current.image=w1&0x1fffffffu;current.size=(w0>>19)&3;current.format=(w0>>21)&7;break;
 case 0xf5:if(((w1>>24)&7)==0){current.line=(w0>>9)&511;current.size=(w0>>19)&3;current.format=(w0>>21)&7;}break;
 case 0xf4:if(((w1>>24)&7)==7)current.load=p;break;
 case 0xf2:if(((w1>>24)&7)==0)current.tile_size=p;break;
 case 0x04:if(w0==0x0400103f&&current.load&&current.tile_size){current.vertex=0x80000000u|(w1&0x1fffffffu);strips.push_back(current);}break;
 default:break;
 }}
 unsigned fixed=0;
 for(unsigned i=0;i+1<strips.size();i++){
  auto& s=strips[i];auto& next=strips[i+1];
  if(s.image<pixels||next.image<=s.image||(s.image-pixels)%stride||(next.image-s.image)%stride)continue;
  const unsigned first=(s.image-pixels)/stride,rows=(next.image-s.image)/stride;
  if(!rows||first+rows>=height||!s.line||s.size>3||s.format>4)continue;
  // RGBA32 uses both TMEM banks; CI reserves the upper half for its palette.
  const unsigned tmem_row=s.line*8*(s.size==3?2:1),capacity=s.format==2?2048:4096;
  if((rows+1)*tmem_row>capacity||!valid(s.vertex,64)||s.vertex<scope.vertices||s.vertex+64>vertex_end)continue;
  const auto load0=word(rdram,s.load),load1=word(rdram,s.load+4),size0=word(rdram,s.tile_size),size1=word(rdram,s.tile_size+4);
  const unsigned last_t=(rows-1)*4,last_s=(width-1)*4;
  if((load0&0xffffff)||(size0&0xffffff)||(load1&4095)!=last_t||(size1&4095)!=last_t||((load1>>12)&4095)!=last_s||((size1>>12)&4095)!=last_s)continue;
  if(MEM_H(10,S32(s.vertex))||MEM_H(26,S32(s.vertex))||MEM_H(42,S32(s.vertex))!=int((rows-1)*32)||MEM_H(58,S32(s.vertex))!=int((rows-1)*32))continue;
  // The geometry is unchanged. One overlapping source row supplies the shared
  // edge at T=rows, matching the next strip's source row and avoiding stale TMEM.
  MEM_W(4,S32(s.load))=S32((load1&~4095u)|(rows*4));
  MEM_W(4,S32(s.tile_size))=S32((size1&~4095u)|(rows*4));
  MEM_H(42,S32(s.vertex))=rows*32;MEM_H(58,S32(s.vertex))=rows*32;fixed++;
 }
 if(fixed&&std::getenv("CLAY_TRACE_SPRITE_SEAMS"))fprintf(stderr,"{\"event\":\"sprite_seams\",\"sprite\":%u,\"width\":%u,\"height\":%u,\"fixed\":%u}\n",scope.sprite,width,height,fixed);
 return fixed;
}
}
