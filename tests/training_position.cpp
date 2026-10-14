// Native stage intersections/basis and transactional reset fixtures. djultra64.
#include "training_position.h"
#include "recomp.h"
#include <cassert>
#include <bit>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <limits>
#include <vector>
extern "C" {
void func_80096D10(uint8_t*,recomp_context*);
void func_80086CB4(uint8_t*,recomp_context*);
void func_80087018(uint8_t*,recomp_context*);
void func_80087054(uint8_t*,recomp_context*);
}
int main(int argc,char** argv){
    std::vector<uint8_t> memory(0x800000);auto* rdram=memory.data();
    constexpr uint32_t world=0x80100000,bank=0x80200000,p1=0x80300000,p2=0x80310000,ray=0x80400000,result=ray+32;
    auto w=[&](uint32_t p,uint32_t v){MEM_W(0,gpr(S32(p)))=S32(v);};
    auto h=[&](uint32_t p,int v){MEM_H(0,gpr(S32(p)))=v;};
    auto f=[&](uint32_t p,float v){w(p,std::bit_cast<uint32_t>(v));};
    auto get=[&](uint32_t p){return std::bit_cast<float>(uint32_t(MEM_W(0,gpr(S32(p)))));};
    auto call=[&](auto function,uint32_t a,uint32_t b=0,uint32_t c=0){
        recomp_context context{};context.f_odd=&context.f0.u32h;context.r29=gpr(S32(0x807ff000));
        context.r4=gpr(S32(a));context.r5=gpr(S32(b));context.r6=gpr(S32(c));function(rdram,&context);return uint32_t(context.r2);
    };
    w(world+0x12c,bank);w(world+0x134,bank+0x48);w(world+0x124,p1);w(world+0x128,p2);
    w(p1+0x2a8,world);w(p2+0x2a8,world);
    if(argc==2){
        std::ifstream input(argv[1],std::ios::binary);std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
        assert(bytes.size()>140&&bytes.size()<0x100000);
        for(unsigned i=0;i<bytes.size();i++)MEM_BU(i,gpr(S32(bank)))=bytes[i];
        const auto node=bank+uint32_t(MEM_W(0x3c,gpr(S32(bank))));w(world+0x134,node);
    }else{
        w(bank+4,0x200);w(bank+0x48+0x1c,4);w(bank+0x48+0x20,0x8c);
        const float coordinates[]{-500,-500,500,-500,500,-500,500,500,500,500,-500,500,-500,500,-500,-500};
        for(unsigned i=0;i<4;i++)for(unsigned n=0;n<4;n++)f(bank+0x8c+i*36+n*4,coordinates[i*4+n]);
        f(bank+0x48+0xc,-150);f(bank+0x48+0x10,0);f(bank+0x48+0x14,150);f(bank+0x48+0x18,0);
    }
    const uint32_t node=uint32_t(MEM_W(0x134,gpr(S32(world))));
    const float ax=get(node+0xc),az=get(node+0x10),bx=get(node+0x14),bz=get(node+0x18);
    const float length=std::hypot(bx-ax,bz-az),dx=(bx-ax)/length,dz=(bz-az)/length,ox=(ax+bx)/2,oz=(az+bz)/2;
    const auto restore=[&]{
        f(world,dx);f(world+8,dz);f(world+0xc,ox);f(world+0x14,oz);w(world+0x18,1);w(world+0x1c,0);
        for(uint32_t actor:{p1,p2}){w(actor+0x158,1);w(actor+0xfb0,7);f(actor+4,13);f(actor+0x10,13);f(actor+0x28,13);}
        h(p1+0x15c,-17);h(p1+0x160,23);h(p2+0x15c,-29);h(p2+0x160,19);
    };
    restore();
    // Obtain independent bounds using the original native segment intersection routine.
    float left=-std::numeric_limits<float>::infinity(),right=std::numeric_limits<float>::infinity();
    f(ray,ox-dx*10000);f(ray+4,oz-dz*10000);f(ray+8,ox+dx*10000);f(ray+12,oz+dz*10000);
    const uint32_t walls=bank+uint32_t(MEM_W(0x20,gpr(S32(node)))),count=uint32_t(MEM_W(0x1c,gpr(S32(node))));
    for(unsigned i=0;i<count;i++)if(call(&func_80096D10,ray,walls+i*36,result)==1){
        const float t=(get(result)-ox)*dx+(get(result+4)-oz)*dz;
        if(t<0)left=std::max(left,t);if(t>0)right=std::min(right,t);
    }
    assert(std::isfinite(left)&&std::isfinite(right));
    unsigned cases=0;
    for(bool side:{false,true})for(auto position:{clay::training::Position::Center,clay::training::Position::LeftCorner,clay::training::Position::RightCorner}){
        restore();clay::training::Options options;options.right_side=side;options.position=position;
        assert(clay::training::reset_position(rdram,world,{p1,p2},options,100));
        const auto l=side?p2:p1,r=side?p1:p2;const float lx=get(l+0x44),rx=get(r+0x44);
        assert(lx>=left+7.9&&rx<=right-7.9);
        if(position==clay::training::Position::LeftCorner)assert(lx<left+9.1);
        if(position==clay::training::Position::RightCorner)assert(rx>right-9.1);
        if(position==clay::training::Position::Center)assert(std::abs((lx+rx)/2-(left+right)/2)<0.6);
        assert(int(rx+MEM_H(0x15c,gpr(S32(r))))-int(lx+MEM_H(0x160,gpr(S32(l))))==100);
        for(uint32_t actor:{p1,p2}){
            assert(get(actor+4)==13&&get(actor+0x10)==13&&get(actor+0x28)==13&&MEM_W(0xfb0,gpr(S32(actor)))==7);
            assert(get(actor)==get(actor+0xc)&&get(actor+0x24)==get(actor+0xc));
            assert(get(actor+8)==get(actor+0x14)&&get(actor+0x2c)==get(actor+0x14));
            assert(get(actor+0x44)==get(actor+0x4c));
        }
        const float x1=get(p1+0xc),z1=get(p1+0x14),x2=get(p2+0xc),z2=get(p2+0x14);
        call(&func_80087054,world);assert(get(p1+0xc)==x1&&get(p2+0x14)==z2);
        // Native basis reconstruction must preserve physical positions and chosen side.
        call(&func_80086CB4,world);
        assert(std::abs(get(world)-dx)<1e-4&&std::abs(get(world+8)-dz)<1e-4);
        assert((get(p1+0x44)>get(p2+0x44))==side);
        call(&func_80087018,world);call(&func_80087054,world);
        assert(std::abs(get(p1+0xc)-x1)<1e-4&&std::abs(get(p1+0x14)-z1)<1e-4);
        assert(std::abs(get(p2+0xc)-x2)<1e-4&&std::abs(get(p2+0x14)-z2)<1e-4);
        cases++;
    }
    restore();clay::training::Options options;
    auto before=memory;
    assert(!clay::training::reset_position(rdram,world,{p1,p2},options,1e6)&&memory==before);
    assert(!clay::training::reset_position(rdram,world,{p1,p2},options,std::numeric_limits<float>::quiet_NaN())&&memory==before);
    assert(!clay::training::reset_position(rdram,world,{p1,p1},options)&&memory==before);
    w(node+0x20,0x7fffff);before=memory;
    assert(!clay::training::reset_position(rdram,world,{p1,p2},options)&&memory==before);
    printf("PASS %u side/corner cases against original96D10/86CB4/87018/87054; Y/animation preservation and invalid-geometry transaction\n",cases);
}
