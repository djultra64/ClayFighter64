// Validate optional import scope and runtime registry lifetimes; djultra64.
#include "../host/sc_assets.h"
#include "recomp.h"
#include <fstream>
#include <iterator>
#include <cstdio>
#include <cstdlib>
int main(int argc,char**argv){
 if(argc!=2)return 1;
 if(clay_sc::sha256({})!="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")return 2;
 if(clay_sc::sha256({'a','b','c'})!="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")return 3;
 std::ifstream f(argv[1],std::ios::binary);std::vector<uint8_t>image(std::istreambuf_iterator<char>(f),{});if(image.empty())return 4;
 auto bad=image;bad.back()^=1;try{clay_sc::configure_asset_image(bad);return 5;}catch(const std::exception&){}
 try{clay_sc::configure_asset_image(image,7);return 6;}catch(const std::exception&){}
 if(clay_sc::enabled())return 7;
 auto*rdram=static_cast<uint8_t*>(calloc(1,0x800000));const gpr model=S32(0x80300000),def=S32(0x80300100),state=S32(0x80200000);
 if(clay_sc_sequence_header_bytes(rdram,def+16)!=14||clay_sc_definition_supported(rdram,def))return 8;
 clay_sc::configure_asset_image(image,6);MEM_W(0,model)=6;MEM_W(0xC,model)=int32_t(def);MEM_H(0,def)=0x521;MEM_B(2,def)=1;clay_sc_register_model(rdram,model);
 if(clay_sc_sequence_header_bytes(rdram,def+16)!=16||clay_sc_sequence_header_bytes(rdram,def-16)!=14||clay_sc_sequence_header_bytes(rdram,S32(0x80400000))!=14)return 9;
 if(!clay_sc_definition_supported(rdram,def)||clay_sc_definition_supported(rdram,def+16)||clay_sc_definition_supported(nullptr,def))return 12;
 MEM_H(0,def)=0x522;if(clay_sc_definition_supported(rdram,def))return 13;MEM_H(0,def)=0x521;
 MEM_B(2,def)=2;if(clay_sc_definition_supported(rdram,def))return 14;MEM_B(2,def)=1;
 MEM_W(0,S32(0x800DDD40))=int32_t(state);MEM_W(0xC,state)=3;MEM_W(0x18,state)=1;clay_sc_demo_selection(rdram);if(MEM_W(0xC,state)!=6||MEM_W(0x18,state)!=0)return 10;
 clay_sc_unregister_model(rdram,model);if(clay_sc_sequence_header_bytes(rdram,def+16)!=14||clay_sc_definition_supported(rdram,def))return 11;
 const gpr combat=S32(0x80210000);for(int i=0;i<9;i++)MEM_W(i*4,combat)=0x100+i;
 clay_sc_demo_combat_selection(rdram,combat);if(MEM_W(0,combat)!=6||MEM_W(4,combat)!=0)return 15;for(int i=2;i<9;i++)if(MEM_W(i*4,combat)!=0x100+i)return 16;
 clay_sc::configure_asset_image(image);MEM_W(0,combat)=0x100;MEM_W(4,combat)=0x101;clay_sc_demo_combat_selection(rdram,combat);for(int i=0;i<9;i++)if(MEM_W(i*4,combat)!=0x100+i)return 17;
 free(rdram);puts("SC asset SHA256, mutation rejection, donor-only registry/unload and explicit demo selection passed");
}
