// Actual native definition format and canonical action notation. djultra64.
#include "move_list.h"
#include "native_prompts.h"
#include "recomp.h"
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
int main(int argc,char** argv){
    std::vector<uint8_t> ram(0x800000);auto* rdram=ram.data();
    constexpr uint32_t actor=0x80100000,table=0x80200000;
    auto half=[&](uint32_t p,uint16_t value){MEM_BU(0,S32(p))=value>>8;MEM_BU(0,S32(p+1))=value;};
    auto word=[&](uint32_t p,uint32_t value){half(p,value>>16);half(p+2,value);};
    word(actor+0x1e8,1);word(actor+0x1ec,table);
    word(table,128);MEM_BU(6,S32(table))=2;
    MEM_BU(0xb,S32(table))=255;MEM_BU(0xe,S32(table))=255;
    MEM_BU(0x12,S32(table))=1;MEM_BU(0x13,S32(table))=0;
    half(table+0x1a,5);half(table+0x1c,70);half(table+0x1e,30);
    half(table+0x28,3);half(table+0x2a,48);
    half(table+48,2);half(table+50,10);half(table+52,8);
    half(table+0x2c,2);half(table+0x2e,56);
    half(table+56,0x40);half(table+58,0x72);half(table+60,0xffff);
    half(table+92,0x8000|0x180);half(table+94,0x73);half(table+96,0xffff);
    auto list=clay::training::read_moves(rdram,actor);
    assert(list.size()==2&&list[0].category==clay::training::MoveCategory::Super);
    assert(list[0].motion==std::vector<uint16_t>({2,10,8})&&list[0].trigger==0x40);
    assert(list[1].variant==1&&list[1].animation==0x73&&list[1].trigger==0x8180);
    assert(list[0].minimum_distance==5&&list[0].maximum_distance==70);
    assert(list[0].requirements.find("Cooldown at least 30")!=std::string::npos);
    const std::array<clay::input::Action,6> actions={clay::input::Action::Secondary,
        clay::input::Action::CLeft,clay::input::Action::CUp,clay::input::Action::Primary,
        clay::input::Action::CDown,clay::input::Action::CRight};
    auto right=clay::training::move_command(list[0],actions,true);
    auto left=clay::training::move_command(list[0],actions,false);
    assert(right!=left&&right.find(clay::input::binding_marker(clay::input::Action::Secondary))!=std::string::npos);
    assert(right.find(clay::input::binding_marker(clay::input::Action::Left))!=std::string::npos);
    auto remapped=actions;remapped[0]=clay::input::Action::Primary;
    assert(clay::training::move_command(list[0],remapped,true).find(clay::input::binding_marker(clay::input::Action::Primary))!=std::string::npos);
    assert(clay::training::move_command(list[1],actions,true).find("Any of")!=std::string::npos);
    list[0].hold_mask=8;list[0].hold_ticks=45;
    assert(clay::training::move_command(list[0],actions,true).find("Charge Back")!=std::string::npos);
    half(table+92,0);assert(clay::training::read_moves(rdram,actor).size()==1);
    half(table+0x2c,3);assert(clay::training::read_moves(rdram,actor).empty());
    half(table+0x2c,1);half(table+0x28,0xffff);assert(clay::training::read_moves(rdram,actor).empty());
    half(table+0x28,3);half(table+0x2a,127);assert(clay::training::read_moves(rdram,actor).empty());
    half(table+0x2a,48);MEM_BU(6,S32(table))=3;
    half(table+0x1a,0xffff);half(table+0x1c,0xffff);
    list=clay::training::read_moves(rdram,actor);
    assert(list.size()==1&&list[0].category==clay::training::MoveCategory::Claytality);
    assert(list[0].minimum_distance==0&&std::isinf(list[0].maximum_distance));
    assert(list[0].requirements.find("finish window")!=std::string::npos);
    // A second record follows the first variable length, rather than a fixed stride.
    word(actor+0x1e8,2);word(table+128,88);MEM_BU(6,S32(table+128))=1;
    MEM_BU(0xb,S32(table+128))=255;MEM_BU(0xe,S32(table+128))=255;
    half(table+128+0x2c,1);half(table+128+0x2e,48);half(table+128+48,0x200);
    half(table+128+4,84);
    for(unsigned n=0;n<4;n++)MEM_BU(84+n,gpr(S32(table+128)))=uint8_t("Ice"[n]);
    list=clay::training::read_moves(rdram,actor);
    assert(list.size()==2&&list[1].record==1&&list[1].name=="Ice");
    assert(clay::training::read_moves(nullptr,actor).empty());
    assert(clay::training::read_moves(rdram,0).empty());
    word(actor+0x1ec,0x807ffff0);assert(clay::training::read_moves(rdram,actor).empty());
    word(actor+0x1ec,table);word(table,32);assert(clay::training::read_moves(rdram,actor).empty());
    if(argc==5){
        std::ifstream file(argv[1],std::ios::binary);
        const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)),{});
        const unsigned start=std::stoul(argv[2]),count=std::stoul(argv[3]),expected=std::stoul(argv[4]);
        assert(bytes.size()<0x600000&&start<bytes.size());
        for(unsigned n=start;n<bytes.size();n++)MEM_BU(n-start,gpr(S32(table)))=bytes[n];
        word(actor+0x1e8,count);word(actor+0x1ec,table);
        const auto actual=clay::training::read_moves(rdram,actor);
        assert(actual.size()==expected);
        for(const auto& move:actual){assert(!move.name.empty());assert(!clay::training::move_command(move,actions,true).empty());}
        std::cout<<"PASS private native table "<<count<<" records, "<<actual.size()<<" enabled variants\n";
    }
    std::cout<<"PASS variable records/36-byte variants, native names, bounds, finish distance/cooldown, charge and facing/remap notation\n";
}
