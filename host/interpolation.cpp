// Scoped native fighter interpolation by djultra64.
// Extended command encoding follows RT64's rt64_extended_gbi.h (MIT).
#include "interpolation.h"
#include "graphics_config.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
namespace clay {
namespace {
struct Piece {uint32_t actor=0,definition=0,sprite=0,id=0;uint64_t image=0;float x=0,y=0;};
struct Scope {uint32_t matrix=0,generation=0,dl_start=0,dl_end=0,vertex_end=0,added=0,trace_frame=0,trace_rejects=0,piece_start=0,payload_bound=0;bool open=false;std::array<std::array<Piece,128>,2> pieces{};};
thread_local Scope scope;
std::atomic<uint32_t> generation{1},next_id{1};
bool valid(uint32_t p,unsigned n){return (p&0xe0000000u)==0x80000000u&&(p&0x1fffffffu)+n<=0x800000;}
uint32_t word(uint8_t* rdram,uint32_t p){return uint32_t(MEM_W(0,S32(p)));}
bool reject(uint8_t* rdram,recomp_context* ctx,const char* reason){
 if(std::getenv("CLAY_TRACE_INTERPOLATION")&&scope.trace_frame%60==1&&scope.trace_rejects++<8){
  uint32_t sp=uint32_t(ctx->r29),actor=valid(sp,0x108)?word(rdram,sp+0x100):0,index=valid(sp,0x108)?word(rdram,sp+0xfc):0,sprite=uint32_t(ctx->r5);
  unsigned format=0,width=0,height=0,stride=0,offset=0;if(valid(sprite,16)){format=MEM_BU(0,S32(sprite));width=MEM_HU(0,S32(sprite+6));height=MEM_HU(0,S32(sprite+8));stride=MEM_HU(0,S32(sprite+14));offset=MEM_HU(0,S32(sprite+2));}
  fprintf(stderr,"{\"event\":\"actor_interpolation_reject\",\"reason\":\"%s\",\"a0\":%u,\"sp\":%u,\"actor\":%u,\"p1\":%u,\"p2\":%u,\"piece\":%u,\"sprite\":%u,\"format\":%u,\"width\":%u,\"height\":%u,\"stride\":%u,\"offset\":%u,\"added\":%u}\n",reason,uint32_t(ctx->r4),sp,actor,word(rdram,0x801028b8),word(rdram,0x801028bc),index,sprite,format,width,height,stride,offset,scope.added);
 }
 return false;
}
bool append(uint8_t* rdram,const uint32_t* words,unsigned count){
 auto p=word(rdram,0x80110898);if(!valid(p,count*4)||(p&7)||p<scope.dl_start||p>scope.dl_end||count*4>scope.dl_end-p)return false;
 for(unsigned i=0;i<count;i++)MEM_W(0,S32(p+i*4))=words[i];
 MEM_W(0,S32(0x80110898))=S32(p+count*4);return true;
}
constexpr uint32_t extra_commands=1536,combat_commands=0x2800;
struct Image {uint64_t hash=1469598103934665603ull;unsigned slices=0;};
bool image_info(uint8_t* rdram,uint32_t sprite,Image& out){
 unsigned format=MEM_BU(0,S32(sprite)),size=format&15;
 if(size>3||!((format==2)||(format==3)||(format==0x30)||(format==0x31)||(format==0x40)||(format==0x41)||(format==0x42)||(format==0x43)||(format==0x50)||(format==0x51)))return false;
 unsigned width=MEM_HU(0,S32(sprite+6)),height=MEM_HU(0,S32(sprite+8)),stride=MEM_HU(0,S32(sprite+14)),offset=MEM_HU(0,S32(sprite+2));
 if(!width||!height||width>1024||height>1024||offset<16||offset>4096)return false;
 unsigned minimum=(width*(1u<<size)+1)/2;
 if(stride<minimum||stride>2048)return false;
 unsigned native_row=(minimum+7)&~7u;
 unsigned rows=2048/native_row;if(!rows)return false;
 out.slices=(height+rows-1)/rows;
 unsigned bytes=offset+stride*height;if(!valid(sprite,bytes))return false;
 // Full header, palette and pixels: reused addresses cannot disguise a changed pose.
 for(unsigned i=0;i<bytes;i++){out.hash^=MEM_BU(0,S32(sprite+i));out.hash*=1099511628211ull;}
 return true;
}
bool capture_pool(uint8_t* rdram){
 unsigned buffers=word(rdram,0x801108cc),index=word(rdram,0x800de174),commands=word(rdram,0x80110880);
 if(!buffers||buffers>8||index>=buffers||commands!=combat_commands+extra_commands)return false;
 auto table=word(rdram,0x801108dc),matrices=word(rdram,0x801108e0),vertices=word(rdram,0x801108e4);
 if(!valid(table,buffers*4)||!valid(matrices,buffers*4)||!valid(vertices,buffers*4))return false;
 uint32_t start=word(rdram,table+index*4),matrix=word(rdram,matrices+index*4),vertex=word(rdram,vertices+index*4);
 unsigned matrix_count=word(rdram,0x80110888),vertex_count=word(rdram,0x80110890);
 if(matrix_count!=0x80||vertex_count!=0xfa0||!valid(start,commands*8)||!valid(matrix,matrix_count*64)||!valid(vertex,vertex_count*16))return false;
 uint32_t current_matrix=word(rdram,0x8011089c),current_vertex=word(rdram,0x801108a0),current_dl=word(rdram,0x80110898);
 if(current_dl<start||current_dl>start+commands*8||current_matrix<matrix+64||current_matrix>matrix+matrix_count*64||current_vertex<vertex||current_vertex>vertex+vertex_count*16)return false;
 scope.dl_start=start;scope.dl_end=start+commands*8;scope.vertex_end=vertex+vertex_count*16;scope.added=0;return true;
}
float floating(uint64_t v){uint32_t bits=uint32_t(v);float out;std::memcpy(&out,&bits,4);return out;}
bool identity(uint8_t* rdram,uint32_t p){
 if(!valid(p,64))return false;
 // Native guMtxF2L layout: signed16 integer matrix, followed by16 fractional halves.
 for(unsigned i=0;i<32;i++)if(MEM_HU(0,S32(p+i*2))!=((i<16&&i%5==0)?1:0))return false;
 return true;
}
}
void reserve_actor_interpolation_pool(recomp_context* ctx){
 if(uint32_t(ctx->r4)==combat_commands&&uint32_t(ctx->r5)==0x80&&uint32_t(ctx->r6)==0xfa0)ctx->r4=S32(combat_commands+extra_commands);
}
void reset_actor_interpolation(){generation.fetch_add(1);scope.matrix=0;scope.open=false;for(auto& actor:scope.pieces)for(auto& p:actor)p={};}
void begin_actor_interpolation_frame(uint8_t* rdram){
 scope.matrix=0;uint32_t current=generation.load();if(scope.generation!=current){for(auto& actor:scope.pieces)for(auto& p:actor)p={};scope.generation=current;}if(graphics_options().refresh<=60)return;
 uint32_t matrix=word(rdram,0x8011089c)-64;
 bool pool=capture_pool(rdram),is_identity=pool&&identity(rdram,matrix);if(is_identity)scope.matrix=matrix;
 scope.trace_frame++;scope.trace_rejects=0;
 if(std::getenv("CLAY_TRACE_INTERPOLATION")&&scope.trace_frame%60==1)fprintf(stderr,"{\"event\":\"actor_interpolation_frame\",\"pool_valid\":%s,\"identity\":%s,\"dl_remaining\":%u}\n",pool?"true":"false",is_identity?"true":"false",pool?scope.dl_end-word(rdram,0x80110898):0);
}
bool begin_actor_piece(uint8_t* rdram,recomp_context* ctx){
 if(!scope.matrix||scope.open||uint32_t(ctx->r4)!=0x80110898)return reject(rdram,ctx,"scope_or_writer");
 uint32_t sp=uint32_t(ctx->r29);if(!valid(sp,0x108))return reject(rdram,ctx,"stack_extent");
 uint32_t actor=word(rdram,sp+0x100),index=word(rdram,sp+0xfc);
 unsigned player=actor==word(rdram,0x801028b8)?0:actor==word(rdram,0x801028bc)?1:2;
 if(player>=2)return reject(rdram,ctx,"actor_identity");
 if(index>=128)return reject(rdram,ctx,"piece_index");
 if(!valid(actor,0x1d8)||!valid(uint32_t(ctx->r5),16))return reject(rdram,ctx,"actor_or_sprite_extent");
 float x=floating(ctx->r6),y=floating(ctx->r7);if(!std::isfinite(x)||!std::isfinite(y))return reject(rdram,ctx,"position_nonfinite");
 auto& previous=scope.pieces[player][index];uint32_t definition=word(rdram,actor+0x1d4),sprite=uint32_t(ctx->r5);
 Image image;if(!image_info(rdram,sprite,image))return reject(rdram,ctx,"image_descriptor");
 // OriginalA87C4: setup<=64commands, loop<=32commands perTMEMslice.
 // Its loop creates four16-bytevertices per slice; our metadata creates none.
 unsigned payload=(64+32*image.slices)*8;auto dl=word(rdram,0x80110898),vtx=word(rdram,0x801108a0);
 if(scope.added+48>extra_commands*8||dl<scope.dl_start||dl>scope.dl_end||payload+48>scope.dl_end-dl||vtx>scope.vertex_end||image.slices*64>scope.vertex_end-vtx)return reject(rdram,ctx,"dl_or_vertex_capacity");
 // Texture/pose discontinuities and teleports stay at the original simulation boundary.
 if(previous.actor!=actor||previous.definition!=definition||previous.sprite!=sprite||previous.image!=image.hash||std::fabs(x-previous.x)>64||std::fabs(y-previous.y)>64){
  previous.id=0xCF000000u|((next_id.fetch_add(1)&0xFFFFFFu));if((previous.id&0xffffffu)==0)previous.id|=1;
 }
 // RT64 group's vertex interpolation is enabled only for the stable fighter piece.
 // Repeating the same LOAD matrix creates a distinct worldTransform for this group.
 constexpr uint32_t properties=1u|(1u<<13); // PUSH, vertex INTERPOLATE; other components SKIP.
 uint32_t words[]={0x00525464,0x10000064,0x6400000c,previous.id,properties,0,0x01020040,scope.matrix&0x1fffffffu};
 scope.piece_start=dl+32;scope.payload_bound=payload;
 if(!append(rdram,words,8))return false;
 if(std::getenv("CLAY_TRACE_INTERPOLATION")&&scope.trace_frame%60==1)fprintf(stderr,"{\"event\":\"actor_interpolation_piece\",\"actor\":%u,\"piece\":%u,\"group\":%u,\"x\":%.4f,\"y\":%.4f,\"sprite_payload_bound\":%u,\"frame_added_bytes\":%u}\n",actor,index,previous.id,x,y,payload,scope.added+48);
 previous.actor=actor;previous.definition=definition;previous.sprite=sprite;previous.image=image.hash;scope.added+=48;previous.x=x;previous.y=y;scope.open=true;return true;
}
void end_actor_piece(uint8_t* rdram){
 if(!scope.open)return;
 // Restore the prior default group and create a fresh untagged transform so HUD,
 // particles and other actor passes cannot inherit fighter vertex interpolation.
 uint32_t p=word(rdram,0x80110898);
 if(p<scope.piece_start||p-scope.piece_start>scope.payload_bound){fputs("Fighter sprite exceeded verified native DL payload bound\n",stderr);std::abort();}
 uint32_t words[]={0x6400000d,1,0x01020040,scope.matrix&0x1fffffffu};
 if(!append(rdram,words,4)){fputs("Fighter interpolation DL pool guard failed\n",stderr);std::abort();}scope.open=false;
}
}
