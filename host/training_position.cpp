// Position along the original active-node collision polygon. djultra64.
#include "training_position.h"
#include "recomp.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
namespace clay::training {
namespace {
bool ram(uint32_t p,uint64_t n){return p>=0x80000000u&&uint64_t(p)+n<=0x80800000ull;}
uint32_t word(uint8_t* rdram,uint32_t p){return uint32_t(MEM_W(0,gpr(S32(p))));}
float scalar(uint8_t* rdram,uint32_t p){return std::bit_cast<float>(word(rdram,p));}
void put(uint8_t* rdram,uint32_t p,uint32_t v){MEM_W(0,gpr(S32(p)))=S32(v);}
void floating(uint8_t* rdram,uint32_t p,float v){put(rdram,p,std::bit_cast<uint32_t>(v));}
struct Point {double x,z;};
Point subtract(Point a,Point b){return {a.x-b.x,a.z-b.z};}
double cross(Point a,Point b){return a.x*b.z-a.z*b.x;}
bool finite(Point p){return std::isfinite(p.x)&&std::isfinite(p.z);}
}
bool reset_position(uint8_t* rdram,uint32_t world,std::array<uint32_t,2> actors,
                    const Options& options,float edge_gap){
    if(!rdram||!ram(world,0x2ca4)||world%4||actors[0]==actors[1]||
       !std::isfinite(edge_gap)||edge_gap<0)return false;
    for(unsigned i=0;i<2;i++)if(!ram(actors[i],0x2ac)||actors[i]%4||
        word(rdram,actors[i]+0x2a8)!=world||word(rdram,world+0x124+i*4)!=actors[i])return false;
    const uint32_t bank=word(rdram,world+0x12c),node=word(rdram,world+0x134);
    if(!ram(bank,8)||bank%4||!ram(node,68)||node%4)return false;
    const uint32_t bytes=word(rdram,bank+4),count=word(rdram,node+0x1c),offset=word(rdram,node+0x20);
    if(bytes<68||!ram(bank,bytes)||node<bank||uint64_t(node)+68>uint64_t(bank)+bytes||
       count<3||count>4096||uint64_t(offset)+uint64_t(count)*36>bytes)return false;
    // 86CB4 publishes the normalized combat direction and its midpoint origin.
    Point origin{scalar(rdram,world+0xc),scalar(rdram,world+0x14)};
    Point direction{scalar(rdram,world),scalar(rdram,world+8)};
    const double norm=std::hypot(direction.x,direction.z);
    if(!finite(origin)||!finite(direction)||norm<0.99||norm>1.01)return false;
    direction.x/=norm;direction.z/=norm;
    double left=-std::numeric_limits<double>::infinity(),right=std::numeric_limits<double>::infinity();
    bool inside=false;
    for(uint32_t i=0;i<count;i++){
        const uint32_t wall=bank+offset+i*36;
        // 90268/96D10 use 2D X/Z endpoint pairs, not mesh vertex bounds.
        const Point a{scalar(rdram,wall),scalar(rdram,wall+4)},b{scalar(rdram,wall+8),scalar(rdram,wall+12)};
        if(!finite(a)||!finite(b))return false;
        if((a.z>origin.z)!=(b.z>origin.z)){
            const double crossing=a.x+(b.x-a.x)*(origin.z-a.z)/(b.z-a.z);
            if(origin.x<crossing)inside=!inside;
        }
        const Point edge=subtract(b,a),delta=subtract(a,origin);
        const double denominator=cross(direction,edge);
        if(std::abs(denominator)<1e-8)continue;
        const double t=cross(delta,edge)/denominator,u=cross(delta,direction)/denominator;
        if(u<-1e-6||u>1.0+1e-6)continue;
        if(t<0)left=std::max(left,t);
        else if(t>0)right=std::min(right,t);
    }
    if(!inside||!std::isfinite(left)||!std::isfinite(right)||left>=right)return false;
    const unsigned left_index=options.right_side?1:0,right_index=1-left_index;
    auto edge=[&](unsigned index,unsigned offset){
        return word(rdram,actors[index]+0x158)?int(MEM_H(0,gpr(S32(actors[index]+offset)))):0;
    };
    const int left_radius=edge(left_index,0x160),right_radius=edge(right_index,0x15c);
    // Integer scalar positions make the two independently truncated native edges exact.
    const double gap=std::ceil(double(edge_gap)),separation=gap+left_radius-right_radius;
    if(!std::isfinite(separation)||separation<=0)return false;
    // 90268 extends movement by 3 units and nudges along normals. Keep an 8-unit
    // interior margin so the first simulation tick does not slide along a wall.
    const double lower=std::ceil(left+8),upper=std::floor(right-8-separation);
    if(lower>upper)return false;
    double first;
    switch(options.position){
        case Position::Center:first=std::clamp(std::round((left+right-separation)/2),lower,upper);break;
        case Position::LeftCorner:first=lower;break;
        case Position::RightCorner:first=upper;break;
        default:return false;
    }
    std::array<float,2> positions{};
    positions[left_index]=float(first);positions[right_index]=float(first+separation);
    std::array<Point,2> points;
    for(unsigned i=0;i<2;i++){
        points[i]={origin.x+direction.x*positions[i],origin.z+direction.z*positions[i]};
        if(!finite(points[i])||!std::isfinite(float(points[i].x))||!std::isfinite(float(points[i].z)))return false;
    }
    // Publish only after validating every field. Match current/desired/presentation
    // X/Z plus prior scalar coordinates; Y and native animation states remain intact.
    for(unsigned i=0;i<2;i++){
        for(unsigned field:{0u,0xcu,0x24u})floating(rdram,actors[i]+field,float(points[i].x));
        for(unsigned field:{8u,0x14u,0x2cu})floating(rdram,actors[i]+field,float(points[i].z));
        floating(rdram,actors[i]+0x44,positions[i]);floating(rdram,actors[i]+0x4c,positions[i]);
        floating(rdram,world+0x20+i*8,positions[i]);
    }
    floating(rdram,world,float(direction.x));floating(rdram,world+8,float(direction.z));
    put(rdram,world+0x18,options.right_side?0:1);
    put(rdram,world+0x1c,0); // No outstanding scalar-to-world displacement (87054).
    return true;
}
}
