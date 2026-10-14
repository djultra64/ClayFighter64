// Native command definitions and action notation. djultra64.
#include "move_list.h"
#include "native_prompts.h"
#include "recomp.h"
#include <limits>
#include <utility>

namespace clay::training {
namespace {
bool ram(uint32_t p,uint64_t size) {
    return p>=0x80000000u && uint64_t(p)+size<=0x80800000ull;
}
uint8_t byte(uint8_t* rdram,uint32_t p) {return MEM_BU(0,S32(p));}
uint16_t half(uint8_t* rdram,uint32_t p) {
    return uint16_t(uint16_t(byte(rdram,p))<<8|byte(rdram,p+1));
}
uint32_t word(uint8_t* rdram,uint32_t p) {
    return uint32_t(half(rdram,p))<<16|half(rdram,p+2);
}
void append(std::string& result,const std::string& part,const char* separator="; ") {
    if(part.empty())return;
    if(!result.empty())result+=separator;
    result+=part;
}
std::string posture(uint8_t value) {
    switch(value) {case 0:return "standing";case 1:return "crouching";case 2:return "airborne";}
    return "posture "+std::to_string(value);
}
std::string mask_command(uint16_t mask,const std::array<input::Action,6>& attacks,bool right) {
    std::string result;
    const bool alternatives=(mask&0x8000)!=0;
    auto add=[&](const std::string& name,input::Action action) {
        append(result,name+" "+input::binding_marker(action),alternatives?" or ":" + ");
    };
    if(mask&1)add("Up",input::Action::Up);
    if(mask&2)add("Down",input::Action::Down);
    if(mask&4)add("Forward",right?input::Action::Right:input::Action::Left);
    if(mask&8)add("Back",right?input::Action::Left:input::Action::Right);
    if(mask&0x10)add("L",input::Action::LeftShoulder);
    if(mask&0x20)add("R",input::Action::RightShoulder);
    static constexpr const char* names[]{"Weak punch","Medium punch","Strong punch",
        "Weak kick","Medium kick","Strong kick"};
    for(unsigned i=0;i<6;i++)if(mask&(0x40u<<i))add(names[i],attacks[i]);
    if(result.empty())result="Neutral";
    // 8008480C treats bit 15 as ANY matching bit, not a duration/charge flag.
    if((mask&0x8000) && ((mask&0x7fff)&((mask&0x7fff)-1)))result="Any of ("+result+")";
    return result;
}

}

const char* category_name(MoveCategory category) {
    switch(category) {
        case MoveCategory::Basic:return "Basic";case MoveCategory::Special:return "Special";
        case MoveCategory::Super:return "Super";case MoveCategory::Combo:return "Combo";
        case MoveCategory::Claytality:return "Claytality";
    }
    return "Move";
}
const char* character_name(int id) {
    switch(id) {
        case 0:return "Bad Mr. Frosty";case 1:return "Blob";case 2:return "Bonker";
        case 3:return "Boogerman";case 4:return "Dr. Kiln";case 5:return "Earthworm Jim";
        case 6:return "High Five";case 8:return "Houngan";case 9:return "Ickybod Clay";
        case 10:return "Kung Pow";case 13:return "Sumo Santa";case 14:return "Taffy";
        case 15:return "T-Hoppy";
        case 11:return "Lady Liberty";case 12:return "Lockjaw";case 16:return "Zappa";
        default:return "Fighter";
    }
}

std::vector<Move> read_moves(uint8_t* rdram,uint32_t actor) {
    std::vector<Move> result;
    if(!rdram || !ram(actor,0x1f0))return result;
    const int32_t count=int32_t(word(rdram,actor+0x1e8));
    uint32_t record=word(rdram,actor+0x1ec);
    if(count<=0 || count>1024)return result;
    for(int32_t i=0;i<count;i++) {
        if(!ram(record,48))break;
        const uint32_t length=word(rdram,record);
        if(length<48 || !ram(record,length))break;
        const unsigned type=byte(rdram,record+6);
        const int motion_count=int16_t(half(rdram,record+0x28));
        const uint32_t motion_offset=half(rdram,record+0x2a);
        const int variants=int16_t(half(rdram,record+0x2c));
        const uint32_t pattern_offset=half(rdram,record+0x2e);
        // Original recognizer advances each variant by 0x24 at 80079D40.
        constexpr unsigned stride=36;
        const bool motions_valid=motion_count>=0 && (motion_count==0 ||
            (motion_offset>=48 && uint64_t(motion_offset)+uint64_t(motion_count)*2<=length));
        const bool patterns_valid=variants>=0 && (variants==0 ||
            (pattern_offset>=48 && uint64_t(pattern_offset)+uint64_t(variants)*stride<=length));
        const unsigned states=byte(rdram,record+0x12),drifts=byte(rdram,record+0x16);
        if(type<=3 && motions_valid && patterns_valid && states<=3 && drifts<=3) {
            std::vector<uint16_t> motion;
            for(int n=0;n<motion_count;n++)motion.push_back(half(rdram,record+motion_offset+n*2));
            for(int v=0;v<variants;v++) {
                const uint32_t pattern=record+pattern_offset+v*stride;
                const uint16_t trigger=half(rdram,pattern);
                if(!trigger)continue; // Native recognizer explicitly skips disabled patterns.
                Move move;
                move.record=unsigned(i);move.variant=unsigned(v);
                move.category=type==1?MoveCategory::Special:type==2?MoveCategory::Super:
                    type==3?MoveCategory::Claytality:MoveCategory::Basic;
                const int combo=int8_t(byte(rdram,record+0xb));
                const int breaker=int8_t(byte(rdram,record+0xe));
                if(type==0 && (combo>=0 || breaker>=0 || byte(rdram,record+9)))move.category=MoveCategory::Combo;
                move.motion=motion;move.trigger=trigger;move.animation=half(rdram,pattern+2);
                move.hold_mask=half(rdram,record+0x24);
                const int hold=int16_t(half(rdram,record+0x26));
                move.hold_ticks=hold>0?uint16_t(hold):0;
                // 8008A68C computes the gap between facing collision edges; world+2CB8
                // is its integer result. -1 removes the respective bound.
                const int minimum=int16_t(half(rdram,record+0x1a));
                const int maximum=int16_t(half(rdram,record+0x1c));
                move.minimum_distance=minimum==-1?0.0f:float(minimum);
                move.maximum_distance=maximum==-1?std::numeric_limits<float>::infinity():float(maximum);
                std::string state;
                for(unsigned n=0;n<states;n++)append(state,posture(byte(rdram,record+0x13+n))," or ");
                append(move.requirements,state);
                if(type==2)append(move.requirements,"Super meter at least 100");
                if(type==3)append(move.requirements,"Opponent in native finish window");
                if(byte(rdram,record+0xf))append(move.requirements,"Opponent throwable and grounded");
                if(combo>=0)append(move.requirements,"Active combo stage "+std::to_string(combo));
                if(breaker>=0)append(move.requirements,"Opponent combo stage "+std::to_string(breaker)+"; breaker opportunity");
                if(byte(rdram,record+9))append(move.requirements,"Combo continuation opportunity; fewer than 3 continuations");
                if(byte(rdram,record+0xa))append(move.requirements,"Opponent final reserve below 41 HP; native combo finish opportunity");
                if(byte(rdram,record+0xc))append(move.requirements,"Both fighters on stage floor");
                const int cooldown=int16_t(half(rdram,record+0x1e));
                if(cooldown>0)append(move.requirements,"Cooldown at least "+std::to_string(cooldown)+" native time units");
                if(minimum!=-1 || maximum!=-1) {
                    std::string range="Collision edge gap ";
                    if(minimum!=-1 && maximum!=-1)range+=std::to_string(minimum)+" to "+std::to_string(maximum);
                    else if(minimum!=-1)range+="at least "+std::to_string(minimum);
                    else range+="at most "+std::to_string(maximum);
                    append(move.requirements,range);
                }
                if(drifts && states && byte(rdram,record+0x13)==2) {
                    std::string drift;
                    for(unsigned n=0;n<drifts;n++) {
                        const unsigned d=byte(rdram,record+0x17+n);
                        append(drift,d==0?"neutral jump":d==1?"forward jump":d==2?"backward jump":"jump direction "+std::to_string(d)," or ");
                    }
                    append(move.requirements,drift);
                }
                move.name=std::string(category_name(move.category))+" "+std::to_string(i+1);
                if(byte(rdram,record+0xf))move.name="Throw "+std::to_string(i+1);
                if(breaker>=0)move.name="Combo breaker "+std::to_string(i+1);
                // Names reside in the caller's privately loaded definitions. No extracted
                // move strings or command tables are shipped in the executable.
                const unsigned name_offset=half(rdram,record+4);
                if(name_offset>=48 && name_offset<length) {
                    std::string native_name;
                    bool terminated=false;
                    for(unsigned n=name_offset;n<length && native_name.size()<96;n++) {
                        const auto c=byte(rdram,record+n);
                        if(!c) {terminated=true;break;}
                        if(c<32 || c>126)break;
                        native_name+=char(c);
                    }
                    if(terminated && !native_name.empty())move.name=std::move(native_name);
                }
                if(variants>1)move.name+=" / "+std::to_string(v+1);
                result.push_back(std::move(move));
            }
        }
        record+=length;
    }
    return result;
}

std::string move_command(const Move& move,const std::array<input::Action,6>& attacks,bool facing_right) {
    std::string command;
    if(move.hold_mask && move.hold_ticks)append(command,"Charge "+mask_command(move.hold_mask,attacks,facing_right)+
        " for "+std::to_string(move.hold_ticks)+" ticks",", then ");
    for(uint16_t direction:move.motion)append(command,mask_command(direction,attacks,facing_right),", then ");
    append(command,mask_command(move.trigger,attacks,facing_right),", then ");
    return command;
}
}
