// Preserve the game's original six and material; model a beveled four. djultra64.
#include "logo64.h"
#include <array>
#include <vector>
#include <map>
#include <mutex>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
extern "C" void func_800B3C70(uint8_t*,recomp_context*);
extern "C" void func_800B4408(uint8_t*,recomp_context*);
extern "C" void func_8005C234(uint8_t*,recomp_context*);
namespace {
constexpr uint32_t source_bytes=11512;
struct Allocation{uint32_t pointer=0,bytes=0;bool ready=false;};
std::mutex mutex;
std::map<uint8_t*,Allocation> caches;
bool valid(uint32_t p,unsigned n){return(p&0xe0000000)==0x80000000&&uint64_t(p&0x1fffffff)+n<=0x800000;}
uint32_t word(uint8_t*rdram,uint32_t p){return MEM_W(0,gpr(S32(p)));}
uint16_t half(uint8_t*rdram,uint32_t p){return MEM_HU(0,gpr(S32(p)));}
void put32(std::vector<uint8_t>&b,unsigned p,uint32_t w){for(unsigned i=0;i<4;i++)b[p+i]=uint8_t(w>>(24-8*i));}
void put16(std::array<uint8_t,16>&b,unsigned p,int v){b[p]=uint16_t(v)>>8;b[p+1]=uint16_t(v);}
using Vertex=std::array<uint8_t,16>;using Triangle=std::array<Vertex,3>;
int coordinate(const Vertex&v,unsigned axis){return int16_t(uint16_t(v[axis*2])<<8|v[axis*2+1]);}
void trace(uint8_t*rdram,uint32_t mesh,const char* reason,uint32_t replacement=0){
 if(!std::getenv("CLAY_TRACE_LOGO"))return;
 bool readable=valid(mesh,source_bytes);uint32_t version=readable?word(rdram,mesh):0,segment=readable?word(rdram,mesh+0x2c):0,count=readable?word(rdram,mesh+0xc8):0,flags=readable?word(rdram,mesh+0x14):0,offset=readable?word(rdram,mesh+0xcc):0;
 std::string key=std::to_string(mesh)+reason+std::to_string(version)+std::to_string(segment)+std::to_string(flags)+std::to_string(replacement);
 static thread_local std::string last;static thread_local unsigned emitted=0;
 if(key==last||emitted>=64)return;last=key;emitted++;
 fprintf(stderr,"{\"event\":\"native_logo64\",\"reason\":\"%s\",\"source\":\"%08X\",\"replacement\":\"%08X\",\"prefix\":\"%08X\",\"segment\":\"%08X\",\"flags\":\"%08X\",\"count\":%u,\"vertex_offset\":%u,\"replacement_count\":%u}\n",reason,mesh,replacement,version,segment,flags,count,offset,valid(replacement,0xd0)?word(rdram,replacement+0xc8):0);
}
struct Point{double x,y,z;};
double cross(Point a,Point b,Point c){return(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);}
std::vector<std::array<unsigned,3>> triangulate(const std::vector<Point>&p){
 std::vector<unsigned> remaining;for(unsigned i=0;i<p.size();i++)remaining.push_back(i);std::vector<std::array<unsigned,3>>out;
 while(remaining.size()>3){bool cut=false;for(unsigned i=0;i<remaining.size();i++){
  unsigned a=remaining[(i+remaining.size()-1)%remaining.size()],b=remaining[i],c=remaining[(i+1)%remaining.size()];if(cross(p[a],p[b],p[c])<=0)continue;
  bool inside=false;for(unsigned q:remaining)if(q!=a&&q!=b&&q!=c&&cross(p[a],p[b],p[q])>=0&&cross(p[b],p[c],p[q])>=0&&cross(p[c],p[a],p[q])>=0){inside=true;break;}
  if(inside)continue;out.push_back({a,b,c});remaining.erase(remaining.begin()+i);cut=true;break;
 }if(!cut)return{};}
 if(remaining.size()==3)out.push_back({remaining[0],remaining[1],remaining[2]});return out;
}
void triangle(std::vector<Triangle>&out,Point a,Point b,Point c){
 double ux=b.x-a.x,uy=b.y-a.y,uz=b.z-a.z,vx=c.x-a.x,vy=c.y-a.y,vz=c.z-a.z;
 std::array<double,3>n{uy*vz-uz*vy,uz*vx-ux*vz,ux*vy-uy*vx};double len=std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);if(len<.1)return;
 Triangle t{};std::array<Point,3>p{a,b,c};for(unsigned i=0;i<3;i++){
  put16(t[i],0,std::lround(p[i].x));put16(t[i],2,std::lround(p[i].y));put16(t[i],4,std::lround(p[i].z));
  put16(t[i],8,std::lround((p[i].x+60)*10));put16(t[i],10,std::lround((p[i].y+43)*14));
  for(unsigned j=0;j<3;j++)t[i][12+j]=uint8_t(int8_t(std::lround(n[j]/len*127)));t[i][15]=255;
 }out.push_back(t);
}
bool original_six(uint8_t*rdram,uint32_t mesh,std::vector<Triangle>&out){
 std::array<Vertex,32>v{};std::array<bool,32>loaded{};unsigned triangles=0;bool ended=false;
 for(unsigned off=0xe0;off+8<=0x788;off+=8){uint32_t w=word(rdram,mesh+off),arg=word(rdram,mesh+off+4);unsigned op=w>>24;
  if(op==0xb8){ended=true;break;}
  if(op==4){unsigned count=(w>>10)&63,first=(w>>17)&127;uint32_t address=0x80000000|arg;
   if(count>32||first+count>32||address<mesh||uint64_t(address)+count*16>uint64_t(mesh)+source_bytes)return false;
   for(unsigned i=0;i<count;i++){loaded[first+i]=true;for(unsigned j=0;j<16;j++)v[first+i][j]=MEM_BU(j,gpr(S32(address+i*16)));}
  }else if(op==0xb1||op==0xbf){for(unsigned t=0;t<(op==0xb1?2u:1u);t++){
   uint32_t encoded=op==0xbf?arg:(t?arg:w&0xffffff);if(!encoded)continue;Triangle face{};bool six=true;
   for(unsigned i=0;i<3;i++){unsigned index=(encoded>>(16-i*8)&255)/2;if(index>=32||!loaded[index])return false;face[i]=v[index];six&=coordinate(face[i],0)<-44;}
   if(six){for(auto&vertex:face)put16(vertex,0,coordinate(vertex,0)+40);out.push_back(face);triangles++;}
  }}else if(op!=0xe7)return false;
 }
 return ended&&triangles==114;
}
std::vector<Triangle> geometry(uint8_t*rdram,uint32_t mesh){
 std::vector<Triangle> out;if(!original_six(rdram,mesh,out))return{};
 // Concave open four, matching the six's height and sixteen-unit depth.
 std::vector<Point>p{{23,31,0},{40,31,0},{15,-9,0},{39,-9,0},{39,31,0},{53,31,0},{53,-9,0},{63,-9,0},{63,-22,0},{53,-22,0},{53,-43,0},{39,-43,0},{39,-22,0},{-2,-22,0},{-2,-9,0}};
 double area=0;for(unsigned i=0;i<p.size();i++)area+=p[i].x*p[(i+1)%p.size()].y-p[(i+1)%p.size()].x*p[i].y;if(area<0)std::reverse(p.begin(),p.end());
 std::vector<Point>inner=p;
 for(unsigned i=0;i<p.size();i++){
  Point prior=p[(i+p.size()-1)%p.size()],here=p[i],next=p[(i+1)%p.size()];double ax=here.x-prior.x,ay=here.y-prior.y,bx=next.x-here.x,by=next.y-here.y,al=std::hypot(ax,ay),bl=std::hypot(bx,by);
  double nx=-ay/al-by/bl,ny=ax/al+bx/bl,den=1+(ax*bx+ay*by)/(al*bl);if(den<.1)den=.1;
  inner[i].x+=std::clamp(2*nx/den,-4.,4.);inner[i].y+=std::clamp(2*ny/den,-4.,4.);
 }
 auto faces=triangulate(inner);if(faces.size()!=p.size()-2)return{};
 for(auto f:faces){Point a=inner[f[0]],b=inner[f[1]],c=inner[f[2]];a.z=b.z=c.z=8;triangle(out,a,b,c);a.z=b.z=c.z=-8;triangle(out,c,b,a);}
 for(unsigned i=0;i<p.size();i++){unsigned j=(i+1)%p.size();Point a=p[i],b=p[j],ia=inner[i],ib=inner[j];a.z=b.z=6;ia.z=ib.z=8;triangle(out,a,b,ib);triangle(out,a,ib,ia);
  a.z=b.z=-6;ia.z=ib.z=-8;triangle(out,b,a,ia);triangle(out,b,ia,ib);
  Point lowa=a,lowb=b;Point higha=a,highb=b;higha.z=highb.z=6;triangle(out,lowa,lowb,highb);triangle(out,lowa,highb,higha);
 }return out;
}
}
extern "C" gpr clay_logo64_mesh(uint8_t*rdram,recomp_context*ctx,gpr original){
 uint32_t mesh=original;if(!valid(mesh,source_bytes)){trace(rdram,mesh,"range");return original;}
 // Reloaded original meshes have variablepaddingbytes3/17 (observedB5/A5).
 // Native5C234 consumesmodebyte2 at5C27C andflags14/15/16 at5C248..268;
 // class0003,mode0 andallthree enableflags remainstrict.
 // Padding is not referenced bythisrenderer or its queued logoDL, so keep the
 // retained geometry immutable instead of rewriting a live GPUallocation.
 if((word(rdram,mesh)&0xffffff00)!=0x00030000||word(rdram,mesh+0x2c)!=0||half(rdram,mesh+0x24)!=0xe0||word(rdram,mesh+0xc8)!=599||word(rdram,mesh+0xcc)!=0x788||(word(rdram,mesh+0x14)&0xffffff00)!=0x01010100){trace(rdram,mesh,"header");return original;}
 auto triangles=geometry(rdram,mesh);if(triangles.empty()){trace(rdram,mesh,"geometry");return original;}
 unsigned count=triangles.size(),groups=(count+9)/10,commands=groups+count+2,vertex_offset=(0xe0+commands*8+15)&~15u,bytes=vertex_offset+count*3*16;
 if(bytes>0x10000)return original;
 Allocation allocation;{std::lock_guard lock(mutex);allocation=caches[rdram];}
 if(!allocation.pointer){recomp_context local=*ctx;local.f_odd=&local.f0.u32h;local.r4=gpr(S32(0x800ee4b0));local.r5=bytes;local.r6=0;func_800B3C70(rdram,&local);
  allocation={uint32_t(local.r2),bytes};if(!valid(allocation.pointer,bytes))return original;
  uint32_t duplicate=0;{std::lock_guard lock(mutex);auto&entry=caches[rdram];if(entry.pointer){duplicate=allocation.pointer;allocation=entry;}else entry=allocation;}
  if(duplicate){local.r4=gpr(S32(0x800ee4b0));local.r5=gpr(S32(duplicate));func_800B4408(rdram,&local);}
 }
 if(allocation.bytes!=bytes)return original;
 // One retained logo buffer. Stable content remains alive for queued RSP work.
 {std::lock_guard lock(mutex);
  if(caches[rdram].ready){trace(rdram,mesh,"cache",allocation.pointer);return gpr(S32(allocation.pointer));}
  std::vector<uint8_t>b(bytes);for(unsigned i=0;i<0xe0;i++)b[i]=MEM_BU(i,gpr(S32(mesh)));
  put32(b,0xc4,0xe0);put32(b,0xc8,count*3);put32(b,0xcc,vertex_offset);
  unsigned offset=0xe0;
  for(unsigned first=0;first<count;first+=10){unsigned group=std::min(10u,count-first),n=group*3;
   put32(b,offset,0x04000000|(n<<10)|(n*16-1));put32(b,offset+4,(allocation.pointer&0x1fffffff)+vertex_offset+first*48);offset+=8;
   for(unsigned i=0;i<group;i++){put32(b,offset,0xbf000000);put32(b,offset+4,((i*3*2)<<16)|((i*3*2+2)<<8)|(i*3*2+4));offset+=8;}
  }
  put32(b,offset,0xe7000000);offset+=8;put32(b,offset,0xb8000000);
  for(unsigned i=0;i<count;i++)for(unsigned j=0;j<3;j++)std::copy(triangles[i][j].begin(),triangles[i][j].end(),b.begin()+vertex_offset+i*48+j*16);
  for(unsigned i=0;i<bytes;i++)MEM_BU(i,gpr(S32(allocation.pointer)))=b[i];
  caches[rdram].ready=true;
 }trace(rdram,mesh,"created",allocation.pointer);return gpr(S32(allocation.pointer));
}
extern "C" void clay_logo64_draw(uint8_t*rdram,recomp_context*ctx){gpr original=ctx->r5;ctx->r5=clay_logo64_mesh(rdram,ctx,original);func_8005C234(rdram,ctx);ctx->r5=original;}
