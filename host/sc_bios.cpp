// Native biography data extension; djultra64. Original game text is a private asset.
#include "sc_bios.h"
#include "sc_roster.h"
#include <array>
#include <vector>
#include <fstream>
#include <iterator>
#include <cstdio>
#include <cstdlib>
namespace clay::bios {
namespace {
struct Entry {uint32_t id=0,count=0;std::vector<uint8_t> text;};
std::array<Entry,4> entries;
constexpr uint32_t ids[]{6,11,12,16},table=0x800DDBDC,scratch=0x80830000;
bool loaded=false;
uint32_t le(const std::vector<uint8_t>& b,size_t p){return uint32_t(b[p])|(uint32_t(b[p+1])<<8)|(uint32_t(b[p+2])<<16)|(uint32_t(b[p+3])<<24);}
}
bool load(const std::string& path,std::string& error){
 loaded=false;entries={};error.clear();std::ifstream f(path,std::ios::binary);
 if(!f){error="Cannot open native biographies: "+path;return false;}
 std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});
 auto invalid=[&](){error="Invalid bounded native biography asset: "+path;return false;};
 if(b.size()<4||b.size()>0x4000||std::string(b.begin(),b.begin()+4)!="CFB1")return invalid();
 size_t pos=4,total=0;std::array<Entry,4> candidate;
 for(size_t i=0;i<4;i++){
  if(pos+12>b.size())return invalid();auto& e=candidate[i];e.id=le(b,pos);e.count=le(b,pos+4);uint32_t n=le(b,pos+8);pos+=12;
  if(e.id!=ids[i]||!e.count||e.count>128||!n||n>b.size()-pos||total+n>0x4000)return invalid();
  uint32_t lines=0,length=0;
  for(size_t j=0;j<n;j++){uint8_t c=b[pos+j];if(!c){lines++;length=0;}else if(c<32||c>126||++length>23)return invalid();}
  if(lines!=e.count||b[pos+n-1])return invalid();e.text.assign(b.begin()+pos,b.begin()+pos+n);pos+=n;total+=n;
 }
 if(pos!=b.size())return invalid();entries=std::move(candidate);loaded=true;return true;
}
void install(uint8_t* rdram,recomp_context* ctx){
 if(!loaded||!clay_roster::enabled()||uint32_t(ctx->r4)!=0)return;
 uint32_t address=scratch;
 // Validate every target before publishing any pointer. Existing endings and other fields stay intact.
 for(const auto&e:entries){uint32_t row=table+e.id*20;uint32_t ptr=MEM_W(gpr(S32(row)),0),count=MEM_W(gpr(S32(row)),8);
  if((ptr||count)&&(ptr!=address||count!=e.count))return;address+=uint32_t(e.text.size());}
 address=scratch;
 for(const auto&e:entries){for(size_t j=0;j<e.text.size();j++)MEM_B(j,gpr(S32(address)))=e.text[j];uint32_t row=table+e.id*20;
  MEM_W(0,gpr(S32(row)))=S32(address);MEM_W(8,gpr(S32(row)))=e.count;address+=uint32_t(e.text.size());}
 if(std::getenv("CLAY_TRACE_BIOS")){uint32_t config=MEM_W(gpr(S32(0x800DDD40)),0);if(config>=0x80000000&&config<0x807FFF00)std::fprintf(stderr,"{\"event\":\"native_bio_table\",\"character\":%u,\"mode\":%u,\"native_animation\":\"8006CC38/8006C6C0\"}\n",uint32_t(MEM_W(gpr(S32(config)),0x14)),uint32_t(MEM_W(gpr(S32(config)),0xD0)));}
}
}
