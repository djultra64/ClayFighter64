// Isolated original game loader harness; deterministic allocation and ROM DMA fixtures by djultra64.
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <map>
#include <stdexcept>
#include "recomp.h"
#include "funcs.h"
#include "../host/sc_assets.h"
static std::vector<uint8_t> rom;
struct GuestFatal:std::runtime_error{using std::runtime_error::runtime_error;};
static unsigned diagnostics=0;
static unsigned current_animation=0;
static bool apply_combat_selection=true;
#ifndef SC_PROBE_STEPS
#define SC_PROBE_STEPS 32
#endif
extern "C" void sc_actual_combat_load(uint8_t*,recomp_context*);
extern "C" int sc_header_bytes(unsigned long sequence){return clay_sc_sequence_header_bytes(nullptr,sequence);}
static uint32_t heap=0x80180000;
static std::map<uint32_t,uint32_t> handles;
static std::vector<std::pair<uint32_t,uint32_t>> allocations;
static std::map<uint32_t,uint32_t> live;static uint64_t live_bytes=0,peak_bytes=0;
extern "C" void sc_unsupported(const char*name){fprintf(stderr,"Unsupported actual guest dependency reached: %s\n",name);exit(71);}
extern "C" {
void switch_error(const char*n,uint32_t a,uint32_t){fprintf(stderr,"Switch failure%s %08x\n",n,a);exit(72);}
void do_break(uint32_t a){fprintf(stderr,"MIPS break%08x\n",a);exit(73);}
recomp_func_t*get_function(int32_t address){if(uint32_t(address)==0x80002050)return func_80002050;if(uint32_t(address)==0x80001E90)return func_80001E90;fprintf(stderr,"Unknown callback%08x\n",uint32_t(address));exit(74);}
static uint32_t alloc(uint8_t*rdram,uint32_t n){if(n>0x300000||heap+n+128>0x807E0000)exit(75);uint32_t p=heap;heap=(heap+n+127)&~63u;allocations.push_back({p,n});live[p]=n;live_bytes+=n;peak_bytes=std::max(peak_bytes,live_bytes);for(uint32_t j=0;j<32;j++)rdram[((p&0x7fffff)+n+j)^3]=0xA5;return p;}
void func_8003C740(uint8_t*rdram,recomp_context*c){
 if(apply_combat_selection)clay_sc_demo_combat_selection(rdram,c->r4);
 uint32_t config=uint32_t(c->r4);MEM_W(0,S32(0x801028C8))=S32(config);
 uint32_t p1=alloc(rdram,0x3484),p2=alloc(rdram,0x3484);c->r4=S32(p1);func_80072D50(rdram,c);c->r4=S32(p2);func_80072D50(rdram,c);
 MEM_W(0,S32(0x801028B8))=S32(p1);MEM_W(0,S32(0x801028BC))=S32(p2);sc_actual_combat_load(rdram,c);
}
void func_800B8FC8(uint8_t*rdram,recomp_context*c){char message[256]{};uint32_t p=c->r4;for(unsigned j=0;j<255;j++){message[j]=rdram[((p&0x7fffff)+j)^3];if(!message[j])break;}fprintf(stderr,"Animation decoder diagnostic: %s opcode%02x command_pointer%08x\n",message,uint32_t(MEM_BU(0x43,c->r29)),uint32_t(MEM_W(0x44,c->r29)));fprintf(stderr,"Current animation slot%u\n",current_animation);sc_unsupported("Original animation decoder diagnostic");}
void func_800B9154(uint8_t*rdram,recomp_context*c){++diagnostics;char message[256]{};uint32_t p=c->r4;for(unsigned j=0;j<255;j++){message[j]=rdram[((p&0x7fffff)+j)^3];if(!message[j])break;}fprintf(stderr,"Guest diagnostic: %s (args%llx,%llx)\n",message,(unsigned long long)c->r5,(unsigned long long)c->r6);throw GuestFatal(message);}
void func_800B3C70(uint8_t*rdram,recomp_context*c){c->r2=S32(alloc(rdram,uint32_t(c->r5)));}
void func_800B4040(uint8_t*rdram,recomp_context*c){uint32_t p=alloc(rdram,uint32_t(c->r5));uint32_t h=alloc(rdram,4);handles[h]=p;c->r2=S32(h);}
void func_800B6A00(uint8_t*,recomp_context*c){auto it=handles.find(uint32_t(c->r5));if(it==handles.end())exit(76);c->r2=S32(it->second);}
void func_800B6E74(uint8_t*,recomp_context*){}
void func_800B4408(uint8_t*,recomp_context*c){uint32_t p=c->r5;if(!p)return;auto i=live.find(p);if(i==live.end()){fprintf(stderr,"Invalid free%08X\n",p);exit(81);}live_bytes-=i->second;live.erase(i);}
void func_800B474C(uint8_t*rdram,recomp_context*c){auto i=handles.find(uint32_t(c->r5));if(i==handles.end())exit(82);uint32_t handle=c->r5;c->r5=gpr(S32(i->second));func_800B4408(rdram,c);handles.erase(i);c->r5=gpr(S32(handle));func_800B4408(rdram,c);}
void func_800B51F4(uint8_t*,recomp_context*){}
void func_8009E36C(uint8_t*,recomp_context*){}
void func_80001BE0(uint8_t*,recomp_context*){sc_unsupported("Unexpected asset-table init");}
void func_80097E54(uint8_t*rdram,recomp_context*c){uint32_t dest=c->r4,src=c->r5,size=c->r6;if(src>rom.size()||size>rom.size()-src||(dest&0x7fffff)+size>0x800000)exit(77);for(uint32_t j=0;j<size;j++)rdram[((dest&0x7fffff)+j)^3]=rom[src+j];}
}
static void dump(uint8_t*rdram,uint32_t p,uint32_t n,const char*path){FILE*f=fopen(path,"wb");if(!f)exit(90);for(uint32_t i=0;i<n;i++)fputc(MEM_BU(i,gpr(S32(p))),f);fclose(f);}
int main(int argc,char**argv){if(argc!=4)return 1;FILE*f=fopen(argv[1],"rb");fseek(f,0,SEEK_END);rom.resize(ftell(f));rewind(f);fread(rom.data(),1,rom.size(),f);fclose(f);auto*rdram=static_cast<uint8_t*>(calloc(1,0x800000));
 for(uint32_t i=0x1000;i<0xF9980;i++)MEM_BU(i-0xC00,gpr(S32(0x80000000)))=rom[i];MEM_W(0,gpr(S32(0x800D92AC)))=1;
 std::vector<uint8_t> before(rdram,rdram+0x180000);unsigned id=atoi(argv[2]);clay_sc::configure_asset_image(rom,id);recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=gpr(S32(0x807FF000));c.r4=id;c.r5=0;func_80001008(rdram,&c);uint32_t object=c.r2;clay_sc_register_model(rdram,gpr(S32(object)));
 uint32_t actor=alloc(rdram,0x3484);c.r4=gpr(S32(actor));func_80072D50(rdram,&c);c.r4=gpr(S32(actor));c.r5=MEM_W(12,gpr(S32(object)));func_80072FE8(rdram,&c);
 uint32_t definition=MEM_W(0x1D4,gpr(S32(actor))),table=MEM_W(0x1DC,gpr(S32(actor)));uint32_t script=definition+MEM_HU(0,gpr(S32(table)));
 c.r4=gpr(S32(actor+0xFB4));c.r5=gpr(S32(script));c.r6=gpr(S32(-1));c.r7=gpr(S32(-1));func_80091A8C(rdram,&c);
 char path[512];snprintf(path,sizeof(path),"%s/%u-poses.jsonl",argv[3],id);FILE*out=fopen(path,"w");if(!out)return 2;
 for(unsigned tick=0;tick<64;tick++){
  c.r4=gpr(S32(actor+0xFB4));func_8009215C(rdram,&c);unsigned pieces=MEM_W(0x1054,gpr(S32(actor)));if(!pieces||pieces>64)return 3;
  fprintf(out,"{\"character_id\":%u,\"tick\":%u,\"animation_slot\":0,\"pieces\":[",id,tick);
  for(unsigned piece=0;piece<pieces;piece++){
   uint32_t p=actor+0x1058+piece*8;int sprite_id=MEM_H(0,gpr(S32(p))),x=MEM_H(2,gpr(S32(p))),y=MEM_H(4,gpr(S32(p))),flags=MEM_BU(6,gpr(S32(p)));
   c.r4=gpr(S32(object));c.r5=gpr(S32(sprite_id));func_80002DE0(rdram,&c);uint32_t sprite=c.r2;unsigned offset=MEM_HU(2,gpr(S32(sprite))),height=MEM_HU(8,gpr(S32(sprite))),stride=MEM_HU(14,gpr(S32(sprite)));
   if(offset>4096||height>512||stride>2048||!height||!stride)return 4;snprintf(path,sizeof(path),"%s/%u-sprite-%d.bin",argv[3],id,sprite_id);dump(rdram,sprite,offset+height*stride,path);
   fprintf(out,"%s{\"sprite_id\":%d,\"x\":%d,\"y\":%d,\"flags\":%d,\"format\":%u}",piece?",":"",sprite_id,x,y,flags,unsigned(MEM_BU(0,gpr(S32(sprite)))));
  }fprintf(out,"]}\n");
 }
 fclose(out);c.r4=gpr(S32(object));func_80001AA8(rdram,&c);func_80003AA8(rdram,&c);c.r5=gpr(S32(actor));func_800B4408(rdram,&c);if(!live.empty()||!handles.empty()){fprintf(stderr,"Leaks%zu handles%zu\n",live.size(),handles.size());for(auto[p,n]:live)fprintf(stderr,"%08X %u\n",p,n);return 6;}for(uint32_t a=0;a<before.size();a++)if(before[a]!=rdram[a])fprintf(stderr,"GLOBAL_DIFF%08X %02X->%02X\n",0x80000000+(a^3),before[a],rdram[a]);fprintf(stderr,"PEAK_BYTES%llu\n",(unsigned long long)peak_bytes);for(auto[p,n]:allocations)for(unsigned i=0;i<32;i++)if(MEM_BU(n+i,gpr(S32(p)))!=0xA5)return 5;printf("Original idle slot0 VM64ticks, native sprite parser and guards passed: ID%u\n",id);free(rdram);
}
