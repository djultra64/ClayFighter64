// Original63-engine optional donor asset integration by djultra64.
#include "sc_assets.h"
#include "../tools/sc_sequence_adapter.h"
#include "recomp.h"
#include <array>
#include <atomic>
#include <cstdio>
#include <iomanip>
#include <sstream>
namespace {
struct Donor {unsigned id;uint32_t start,end,decoded;};
constexpr Donor donors[]{{6,12582912,13112272,116704},{11,13112272,13975376,190128},{12,13975376,14640768,186784},{16,14640768,15368752,183664}};
std::atomic_bool active{false};int demo=-1;clay_sc::SequenceFormats formats;
const Donor*find(unsigned id){for(const auto& d:donors)if(d.id==id)return &d;return nullptr;}
uint32_t be32(const std::vector<uint8_t>& d,size_t o){if(o+4>d.size())throw std::runtime_error("SC asset manifest bounds");return(uint32_t(d[o])<<24)|(uint32_t(d[o+1])<<16)|(uint32_t(d[o+2])<<8)|d[o+3];}
uint32_t rotr(uint32_t v,unsigned n){return(v>>n)|(v<<(32-n));}
bool ram_address(uint32_t a,uint32_t n){return a>=0x80000000u&&uint64_t(a)+n<=0x80800000ull;}
}
std::string clay_sc::sha256(const std::vector<uint8_t>& input){
 constexpr uint32_t k[]{0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
 std::array<uint32_t,8> h{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
 auto data=input;uint64_t bits=uint64_t(data.size())*8;data.push_back(0x80);while(data.size()%64!=56)data.push_back(0);for(int b=7;b>=0;--b)data.push_back(uint8_t(bits>>(8*b)));
 for(size_t o=0;o<data.size();o+=64){uint32_t w[64]{};for(unsigned i=0;i<16;i++)w[i]=be32(data,o+i*4);for(unsigned i=16;i<64;i++){uint32_t s0=rotr(w[i-15],7)^rotr(w[i-15],18)^(w[i-15]>>3),s1=rotr(w[i-2],17)^rotr(w[i-2],19)^(w[i-2]>>10);w[i]=w[i-16]+s0+w[i-7]+s1;}auto v=h;for(unsigned i=0;i<64;i++){uint32_t s1=rotr(v[4],6)^rotr(v[4],11)^rotr(v[4],25),ch=(v[4]&v[5])^(~v[4]&v[6]),t1=v[7]+s1+ch+k[i]+w[i],s0=rotr(v[0],2)^rotr(v[0],13)^rotr(v[0],22),maj=(v[0]&v[1])^(v[0]&v[2])^(v[1]&v[2]),t2=s0+maj;for(unsigned j=7;j>0;j--)v[j]=v[j-1];v[4]+=t1;v[0]=t1+t2;}for(unsigned i=0;i<8;i++)h[i]+=v[i];}
 std::ostringstream out;out<<std::hex<<std::setfill('0');for(auto v:h)out<<std::setw(8)<<v;return out.str();
}
void clay_sc::configure_asset_image(const std::vector<uint8_t>& image,int character){
 if(image.size()!=15368752||sha256(image)!="05f3d63f8c413520e4ee5fecd0ec400361a5dc11df4a651ef06ad22431c733da")throw std::runtime_error("Unverified optional SC asset image; build it with tools/sc_import_assets.py");
 if(character!=-1&&!find(unsigned(character)))throw std::invalid_argument("SC demo character must be6,11,12 or16");
 for(const auto& d:donors){if(be32(image,0xD9580+d.id*4)!=d.start||be32(image,0xD95C4+d.id*4)!=d.end)throw std::runtime_error("SC asset range mismatch");uint32_t packet=d.start+be32(image,d.start+0x20);if(be32(image,packet+8)!=d.decoded)throw std::runtime_error("SC decoded range mismatch");}
 demo=character;active.store(true,std::memory_order_release);
}
bool clay_sc::enabled(){return active.load(std::memory_order_acquire);}
extern "C" int clay_sc_definition_supported(uint8_t* rdram,uint64_t definition){
 uint32_t def=uint32_t(definition);
 return clay_sc::enabled()&&rdram&&ram_address(def,3)&&formats.registered_definition(def)&&MEM_HU(0,S32(def))==0x0521&&MEM_BU(2,S32(def))==1;
}
extern "C" int clay_sc_sequence_header_bytes(uint8_t*,uint64_t sequence){return clay_sc::enabled()?formats.header_bytes(uint32_t(sequence)):14;}
extern "C" void clay_sc_register_model(uint8_t* rdram,uint64_t model){
 if(!clay_sc::enabled())return;
 uint32_t obj=model;if(!ram_address(obj,64))throw std::runtime_error("Invalid optional SC model");const auto*d=find(unsigned(MEM_W(0,S32(obj))));if(!d)return;
 uint32_t def=MEM_W(0xC,S32(obj));if(!ram_address(def,d->decoded)||MEM_HU(0,S32(def))!=0x0521||MEM_BU(2,S32(def))!=1)throw std::runtime_error("Invalid optional SC CharDef5.21");formats.register_definition(d->id,def,d->decoded);
 fprintf(stderr,"{\"event\":\"experimental_sc_model\",\"character_id\":%u,\"definition\":\"%08X\",\"decoded_size\":%u}\n",d->id,def,d->decoded);
}
extern "C" void clay_sc_unregister_model(uint8_t* rdram,uint64_t model){if(!clay_sc::enabled())return;
 uint32_t obj=model;if(ram_address(obj,64))formats.unregister_definition(uint32_t(MEM_W(0xC,S32(obj))));}
extern "C" void clay_sc_demo_selection(uint8_t* rdram){
 if(!clay_sc::enabled()||demo<0)return;
 uint32_t state=MEM_W(0,S32(0x800DDD40));if(!ram_address(state,0x24))throw std::runtime_error("SC demo selection state unavailable");
 MEM_W(0xC,S32(state))=demo;MEM_W(0x18,S32(state))=0;
}

extern "C" void clay_sc_demo_combat_selection(uint8_t* rdram,uint64_t combat_config){
 if(!clay_sc::enabled()||demo<0)return;
 uint32_t config=uint32_t(combat_config);
 if(!ram_address(config,0x24))throw std::runtime_error("SC demo combat selection state unavailable");
 MEM_W(0,S32(config))=demo;MEM_W(4,S32(config))=0;
 fprintf(stderr,"{\"event\":\"experimental_sc_combat_selection\",\"character_id\":%d,\"config\":\"%08X\"}\n",demo,config);
}
