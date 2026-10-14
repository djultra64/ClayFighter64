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
extern "C" void sc_unsupported(const char*name){fprintf(stderr,"Unsupported actual guest dependency reached: %s\n",name);exit(71);}
extern "C" {
void switch_error(const char*n,uint32_t a,uint32_t){fprintf(stderr,"Switch failure%s %08x\n",n,a);exit(72);}
void do_break(uint32_t a){fprintf(stderr,"MIPS break%08x\n",a);exit(73);}
recomp_func_t*get_function(int32_t address){if(uint32_t(address)==0x80002050)return func_80002050;if(uint32_t(address)==0x80001E90)return func_80001E90;fprintf(stderr,"Unknown callback%08x\n",uint32_t(address));exit(74);}
static uint32_t alloc(uint8_t*rdram,uint32_t n){if(n>0x300000||heap+n+128>0x807E0000)exit(75);uint32_t p=heap;heap=(heap+n+127)&~63u;allocations.push_back({p,n});for(uint32_t j=0;j<32;j++)rdram[((p&0x7fffff)+n+j)^3]=0xA5;return p;}
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
void func_800B4408(uint8_t*,recomp_context*){}
void func_800B474C(uint8_t*,recomp_context*c){handles.erase(uint32_t(c->r5));}
void func_800B51F4(uint8_t*,recomp_context*){}
void func_8009E36C(uint8_t*,recomp_context*){}
void func_80001BE0(uint8_t*,recomp_context*){sc_unsupported("Unexpected asset-table init");}
void func_80097E54(uint8_t*rdram,recomp_context*c){uint32_t dest=c->r4,src=c->r5,size=c->r6;if(src>rom.size()||size>rom.size()-src||(dest&0x7fffff)+size>0x800000)exit(77);for(uint32_t j=0;j<size;j++)rdram[((dest&0x7fffff)+j)^3]=rom[src+j];}
}
int main(int argc,char**argv){if(argc!=3)return 1;FILE*f=fopen(argv[1],"rb");if(!f)return 2;fseek(f,0,SEEK_END);size_t size=ftell(f);rewind(f);rom.resize(size);if(fread(rom.data(),1,size,f)!=size)return 3;fclose(f);auto*rdram=static_cast<uint8_t*>(calloc(1,8*1024*1024));
 for(uint32_t o=0x1000;o<0xF9980;o++)rdram[((o-0x1000+0x400))^3]=rom[o];
 MEM_W(0,int32_t(0x800D92ACu))=1;
 if(atoi(argv[2])!=3)clay_sc::configure_asset_image(rom,atoi(argv[2]));
 recomp_context ctx{};ctx.r4=atoi(argv[2]);ctx.r5=0;ctx.r29=0x807FF000;ctx.f_odd=&ctx.f0.u32h;func_80001008(rdram,&ctx);
 uint32_t object_initial=ctx.r2;clay_sc_register_model(rdram,S32(object_initial));uint32_t state=alloc(rdram,0x3484);if(atoi(argv[2])!=3){
 uint32_t def=uint32_t(MEM_W(0xC,S32(object_initial)));
 auto reject=[&](const char*label,uint32_t pointer){recomp_context c{};c.f_odd=&c.f0.u32h;c.r29=S32(0x807FF000);c.r4=S32(state);c.r5=S32(pointer);try{func_80072FE8(rdram,&c);fprintf(stderr,"Invalid definition accepted: %s\n",label);exit(83);}catch(const GuestFatal&){printf("Original fatal path preserved: %s\n",label);}};
 if(!clay_sc_definition_supported(rdram,S32(def))||clay_sc_definition_supported(rdram,S32(def+16)))return 84;
 MEM_H(0,S32(def))=0x522;reject("registered5.22",def);MEM_H(0,S32(def))=0x621;reject("registered6.21",def);MEM_H(0,S32(def))=0x521;
 MEM_B(2,S32(def))=2;reject("registered5.21 wrong type",def);MEM_H(0,S32(def))=0x520;reject("original5.20 wrong type",def);MEM_H(0,S32(def))=0x521;MEM_B(2,S32(def))=1;
 uint32_t copy=alloc(rdram,116704);for(uint32_t j=0;j<32;j++)rdram[((copy&0x7fffff)+j)^3]=rdram[((def&0x7fffff)+j)^3];reject("unregistered5.21",copy);
 auto saved=MEM_H(16,S32(def));auto type=MEM_BU(18,S32(def));MEM_H(16,S32(def))=0x521;MEM_B(18,S32(def))=1;reject("registered range interior5.21",def+16);MEM_H(16,S32(def))=saved;MEM_B(18,S32(def))=type;
 clay_sc_unregister_model(rdram,S32(object_initial));reject("unloaded5.21",def);clay_sc_register_model(rdram,S32(object_initial));
 }unsigned expected_diagnostics=diagnostics;ctx.r4=S32(state);func_80072D50(rdram,&ctx);ctx.r4=S32(state);ctx.r5=MEM_W(0xC,S32(object_initial));try{func_80072FE8(rdram,&ctx);}catch(const GuestFatal&){fprintf(stderr,"Fatal diagnostic reached for intended valid character bootstrap\n");return 86;}if(diagnostics!=expected_diagnostics)return 85;
 
 uint32_t anim_count=MEM_W(0x1D8,S32(state)),anim_offsets=MEM_W(0x1DC,S32(state)),definition=MEM_W(0x1D4,S32(state));
 if(anim_count>1024||anim_offsets<0x80180000||anim_offsets>=heap)return 79;
 unsigned id=atoi(argv[2]);uint32_t decoded_size=0;

 if(clay_sc_sequence_header_bytes(rdram,definition-16)!=14||clay_sc_sequence_header_bytes(rdram,0x80001000)!=14)return 81;
unsigned animated=0;for(uint32_t a=0;a<anim_count;a++){uint32_t script_offset=MEM_HU(int32_t(a*2),S32(anim_offsets));if(!script_offset)continue;uint32_t script=definition+script_offset;if(script<definition||script>=heap)return 80;current_animation=a;ctx.r4=S32(state+0xFB4);ctx.r5=S32(script);ctx.r6=S32(-1);ctx.r7=S32(-1);func_80091A8C(rdram,&ctx);for(unsigned tick=0;tick<SC_PROBE_STEPS;tick++){ctx.r4=S32(state+0xFB4);func_8009215C(rdram,&ctx);}++animated;}
 printf("Original animation interpreter initialized and advanced%usteps for %u/%u defined animation slots; diagnostics%u\n",SC_PROBE_STEPS,animated,anim_count,diagnostics);
 if(id!=3){clay_sc_unregister_model(rdram,S32(object_initial));if(clay_sc_sequence_header_bytes(rdram,definition+16)!=14)return 82;}
 auto transfer_case=[&](recomp_func_t*wrapper,unsigned expected,const char*label){
 uint32_t combat_config=alloc(rdram,0x24);std::vector<uint8_t> config_before(0x24);
 for(uint32_t j=0;j<0x24;j++)rdram[((combat_config&0x7fffff)+j)^3]=uint8_t(0x30+j);
 MEM_W(0,S32(combat_config))=0;MEM_W(4,S32(combat_config))=1;MEM_W(0xC,S32(combat_config))=3;MEM_W(0x10,S32(combat_config))=1;
 for(uint32_t j=0;j<0x24;j++)config_before[j]=rdram[((combat_config&0x7fffff)+j)^3];
 recomp_context battle{};battle.f_odd=&battle.f0.u32h;battle.r29=S32(0x807FF000);battle.r4=S32(combat_config);battle.r5=0;battle.r6=0;wrapper(rdram,&battle);
 for(uint32_t j=(expected==0?0:8);j<0x24;j++)if(rdram[((combat_config&0x7fffff)+j)^3]!=config_before[j])exit(87);
 uint32_t p1model=uint32_t(MEM_W(0,S32(0x801028A0))),p2model=uint32_t(MEM_W(0,S32(0x801028A4))),p1state=uint32_t(MEM_W(0,S32(0x801028B8)));
 if(uint32_t(MEM_W(0,S32(p1model)))!=expected||MEM_W(0,S32(p2model))!=3||MEM_W(0,S32(0x801028AC))!=(expected==0?1:0))exit(88);
 if(MEM_W(0x1D4,S32(p1state))!=MEM_W(0xC,S32(p1model)))exit(89);
 printf("%s: actual P1ID%u P2ID3; P1 CharDef%08X; P2/stage/mode bytes preserved\n",label,uint32_t(MEM_W(0,S32(p1model))),uint32_t(MEM_W(0x1D4,S32(p1state))));
 };
 if(id!=3){apply_combat_selection=false;transfer_case(func_8003C6B4,0,"RED preview-only/wrong3C650 wrapper hook leaves actual3C6B4 P1 Frosty");apply_combat_selection=true;}
 transfer_case(func_8003C650,id==3?0:id,"GREEN shared3C740 via3C650");
 transfer_case(func_8003C6B4,id==3?0:id,"GREEN shared3C740 via3C6B4");
 transfer_case(func_8003C70C,id==3?0:id,"GREEN shared3C740 via3C70C");
 ctx.r2=S32(object_initial);
 for(auto [p,n]:allocations)for(uint32_t j=0;j<32;j++)if(rdram[((p&0x7fffff)+n+j)^3]!=0xA5){fprintf(stderr,"Allocation guard corrupted at%08x bytes%u\n",p,n);return 78;}
 uint32_t object=ctx.r2;if(object<0x80180000||object>=heap)return 4;
 uint32_t metadata=MEM_W(8,S32(object)),frames=MEM_W(0xc,S32(object)),palettes=MEM_W(0x18,S32(object));
 if(metadata<0x80180000||frames<metadata||frames>=heap||palettes<0x80180000||palettes>=heap)return 5;
 printf("Actual original63 loader returned ID%u object%08x metadata%08x frame-table%08x RGBA-palettes%08x; actual combat state parser executed; all allocation guards intact; heapbytes%u\n",uint32_t(MEM_W(0,S32(object))),object,metadata,frames,palettes,heap-0x80180000);
 free(rdram);return 0;}
