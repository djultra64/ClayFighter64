// Native selector branch fixtures; djultra64. No resource/renderer execution claim.
#include "recomp.h"
#include <cassert>
#include <cstdio>
#include <vector>
extern "C" {
void selector_native_init(uint8_t*,recomp_context*);
void selector_training_init(uint8_t*,recomp_context*);
void selector_native_p2_gate(uint8_t*,recomp_context*);
void selector_native_p1_confirm(uint8_t*,recomp_context*);
void selector_native_p2_confirm(uint8_t*,recomp_context*);
void osStartThread_recomp(uint8_t*,recomp_context*){}
void func_8005D334(uint8_t*,recomp_context*){}
void func_8005D3D4(uint8_t*,recomp_context*){}
void func_8009B6B4(uint8_t*,recomp_context*){}
int clay_sc_roster_columns(){return 3;}
}
int main(int argc,char**){
    std::vector<uint8_t> memory(0x900000);auto* rdram=memory.data();
    constexpr int32_t selector=S32(0x80200000),config=S32(0x80300000),stack=S32(0x807ff000);
    MEM_W(0,S32(0x800ddd30))=selector;MEM_W(0,S32(0x800ddd40))=config;
    auto context=[&]{recomp_context c{};c.r29=gpr(stack);return c;};
    auto init=[&](int mode,int first,int second){
        MEM_W(0x6c,selector)=99;MEM_W(0x70,selector)=99;
        MEM_W(0,selector)=0;
        auto c=context();c.f_odd=&c.f0.u32h;c.r4=mode;c.r5=first;c.r6=second;
        selector_native_init(rdram,&c);
    };
    auto gate=[&]{auto c=context();c.f_odd=&c.f0.u32h;selector_native_p2_gate(rdram,&c);return int(c.r2);};
    // Baseline reproduces the original mode gate that ignores interactive P2.
    init(1,0,0);assert(MEM_H(0xa0,selector)==1);assert(gate()==2);
    if(argc>1){puts("RED original single-player selector routes P2 to the noninteractive branch");return 14;}
    // A1/A2=1 preselects fighters, independently of versus mode.
    init(0,1,1);assert(MEM_W(0x6c,selector)==0x80&&MEM_W(0x70,selector)==0x80);
    assert((MEM_W(0,selector)&3)==3&&gate()==0);
    init(0,0,0);
    assert(MEM_W(0x6c,selector)==0&&MEM_W(0x70,selector)==0&&MEM_H(0xa0,selector)==0);
    // Actual Training bridge argument instructions, followed by original initialization.
    auto c=context();c.f_odd=&c.f0.u32h;c.r4=1;c.r5=1;c.r6=1;
    selector_training_init(rdram,&c);
    assert(MEM_H(0xa0,selector)==0&&MEM_W(0x6c,selector)==0&&MEM_W(0x70,selector)==0);
    assert(gate()==1);
    MEM_W(0x10,selector)=1;MEM_W(0x14,selector)=0;
    MEM_W(0,S32(0x80118cb4))=0x8000;
    c=context();c.f_odd=&c.f0.u32h;selector_native_p1_confirm(rdram,&c);
    assert(MEM_W(0x6c,selector)==1&&MEM_W(0x70,selector)==0);
    assert(gate()==1);
    // P1 confirm never accidentally confirms P2; the native P2 edge is required.
    MEM_W(0,S32(0x80118ccc))=0;
    c=context();c.f_odd=&c.f0.u32h;selector_native_p2_confirm(rdram,&c);
    assert(MEM_W(0x70,selector)==0);
    MEM_W(0,S32(0x80118ccc))=0x8000;
    c=context();c.f_odd=&c.f0.u32h;selector_native_p2_confirm(rdram,&c);
    assert(MEM_W(0x6c,selector)==1&&MEM_W(0x70,selector)==1&&gate()==3);
    // Native locks and transitions suppress confirmations.
    MEM_W(0x70,selector)=0;MEM_W(0x14,selector)=1;
    c=context();c.f_odd=&c.f0.u32h;selector_native_p2_confirm(rdram,&c);
    assert(MEM_W(0x70,selector)==0);
    MEM_W(0,selector)=2;assert(gate()==0);
    puts("PASS original selector mode/initialization branches and sequential native confirmations");
}
