// Execute the original 63⅓ codec96 MIPS recompilation on donor packets; djultra64.
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include "recomp.h"
extern "C" void func_800BE6C8(uint8_t*,recomp_context*);
extern "C" void func_800B3C70(uint8_t*,recomp_context*c){static uint32_t p=0x80500000;c->r2=S32(p);p+=(uint32_t(c->r5)+15)&~15u; if(p>=0x80700000)abort();}
extern "C" void func_800B4408(uint8_t*,recomp_context*){}
extern "C" void func_800B8FC8(uint8_t*,recomp_context*){abort();}
extern "C" void switch_error(const char*,uint32_t,uint32_t){abort();}
int main(int argc,char**argv){
 if(argc!=4)return 1;
 FILE*f=fopen(argv[2],"rb");if(!f)return 2;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);std::vector<uint8_t> packet(n);if(fread(packet.data(),1,n,f)!=size_t(n))return 3;fclose(f);
 if(n<12||n>0x100000)return 4;
 auto u32=[&](size_t o){return(uint32_t(packet[o])<<24)|(uint32_t(packet[o+1])<<16)|(uint32_t(packet[o+2])<<8)|packet[o+3];};
 uint32_t decoded=u32(8);if(packet[0]!=96||decoded>0x100000||n<12)return 4;
 auto*rdram=static_cast<uint8_t*>(calloc(1,8*1024*1024));if(!rdram)return 5;
 const uint32_t src=0x80200000,dst=0x80400000;
 FILE*rom=fopen(argv[1],"rb"); if(!rom)return 10; fseek(rom,0x1000,SEEK_SET); for(size_t i=0x400;i<0xf8d80;i++)rdram[i^3]=fgetc(rom);fclose(rom);
 for(size_t i=0;i<packet.size();i++)rdram[((src&0x7fffff)+i)^3]=packet[i];
 for(size_t i=decoded;i<decoded+64;i++)rdram[((dst&0x7fffff)+i)^3]=0xA5;
 recomp_context ctx{};ctx.r29=S32(0x807FF000);ctx.r4=S32(dst);ctx.r5=S32(src+12);ctx.r6=decoded;func_800BE6C8(rdram,&ctx);
 if(uint32_t(ctx.r2)!=decoded)return 6;
 for(size_t i=decoded;i<decoded+64;i++)if(rdram[((dst&0x7fffff)+i)^3]!=0xA5)return 7;
 std::vector<uint8_t> result(decoded);for(size_t i=0;i<decoded;i++)result[i]=rdram[((dst&0x7fffff)+i)^3];
 f=fopen(argv[3],"wb");if(!f)return 8;if(fwrite(result.data(),1,result.size(),f)!=result.size())return 9;fclose(f);free(rdram);printf("Original63 codec96 decoded %u bytes; output guard intact\n",decoded);
}
