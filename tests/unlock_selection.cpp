// Native execution of the original ROM's character-selection filter, by djultra64.
#include <cstdio>
#include <cstdlib>
#include <set>
#include "recomp.h"
extern "C" void func_8005D484(uint8_t*, recomp_context*);
int main() {
 auto* rdram=static_cast<uint8_t*>(calloc(1,8*1024*1024));
 if(!rdram)return 1;
 const int state=static_cast<int>(0x80300000u);
 MEM_W(0,static_cast<int>(0x800DDD40u))=state;
 const int roster[]={0,1,2,10,15,8,14,5,9,13,4,3};
 auto filter=[&](int id,int flags){MEM_H(0x20,state)=flags;recomp_context ctx{};ctx.r4=id;func_8005D484(rdram,&ctx);return static_cast<int>(ctx.r2);};
 for(int flags=0;flags<8;flags++) {
  const int secrets[]={13,4,3};
  const int bits[]={1,2,4};
  for(int i=0;i<3;i++)if((filter(secrets[i],flags)==secrets[i])!=bool(flags&bits[i]))return 2;
 }
 for(int flags:{0,7}) {
  std::set<int> accepted;
  for(int id:roster)if(filter(id,flags)==id)accepted.insert(id);
  if(accepted.size()!=static_cast<size_t>(flags?12:9))return 3;
  for(int absent:{6,7,11,12,16})if(accepted.count(absent))return 4;
  printf("Original native ROM filter flags=%d: %zu playable original roster IDs; Sculptor's Cut IDs absent\n",flags,accepted.size());
 }
 // Unrelated bits do not change whether the three requested secrets are unlocked.
 for(int id:roster)if(filter(id,7)!=filter(id,0x107))return 5;
 free(rdram);puts("All eight original unlock combinations and 9->12 roster checks passed");
}
