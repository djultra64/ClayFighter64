// Training text through the game's native font/display list. djultra64.
#include "training_menu_ui.h"
#include "native_prompts.h"
#include "ui_background.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cctype>

extern "C" {
void func_800B0270(uint8_t*,recomp_context*);
void func_800B0328(uint8_t*,recomp_context*);
void func_800B09DC(uint8_t*,recomp_context*);
void func_800A1B98(uint8_t*,recomp_context*);
void func_800A45BC(uint8_t*,recomp_context*);
void func_800A3F88(uint8_t*,recomp_context*);
void func_800A4D9C(uint8_t*,recomp_context*);
void func_800A561C(uint8_t*,recomp_context*);
void func_80071738(uint8_t*,recomp_context*);
void func_8007191C(uint8_t*,recomp_context*);
}
namespace clay::training {
namespace {
uint32_t word(uint8_t* rdram,uint32_t p){return uint32_t(MEM_W(0,S32(p)));}
bool valid(uint32_t p,uint32_t bytes){return p>=0x80000000u&&uint64_t(p)+bytes<=0x80900000ull;}
unsigned remaining(uint8_t* rdram){
    const auto capacity=word(rdram,0x80110880),table=word(rdram,0x801108dc),buffer=word(rdram,0x800de174),writer=word(rdram,0x80110898);
    if(buffer>=8||capacity<512||capacity>16384||!valid(table,(buffer+1)*4))return 0;
    auto start=word(rdram,table+buffer*4);
    if(!valid(start,capacity*8)||writer<start||writer>start+capacity*8)return 0;
    return (start+capacity*8-writer)/8;
}
unsigned remaining_matrices(uint8_t* rdram){
    const auto count=word(rdram,0x80110888),table=word(rdram,0x801108e0),buffer=word(rdram,0x800de174),writer=word(rdram,0x8011089c);
    if(buffer>=8||!count||count>4096||!valid(table,(buffer+1)*4))return 0;
    const auto start=word(rdram,table+buffer*4);
    if(!valid(start,count*64)||writer<start||writer>start+count*64)return 0;
    return (start+count*64-writer)/64;
}
std::string readable_literals(const std::string& source){
    std::string out;
    for(size_t i=0;i<source.size();++i){
        const unsigned char c=source[i];
        // Explicit prompt tokens resolve later, so icon aliases are never
        // mistaken for unsupported literal punctuation in the29px pause font.
        if(c==0x1e&&i+2<source.size()&&source[i+2]==0x1f){out.append(source,i,3);i+=2;}
        else if(std::isalnum(c)||c==' ')out+=char(c);
        else if(c=='+'||c=='&')out+=" and ";
        else if(c=='-'&&i+1<source.size()&&std::isdigit(static_cast<unsigned char>(source[i+1])))out+=" minus ";
        else if(c=='.'&&i&&i+1<source.size()&&std::isdigit(static_cast<unsigned char>(source[i-1]))&&std::isdigit(static_cast<unsigned char>(source[i+1])))out+=" point ";
        else out+=' ';
    }
    return out;
}
}
void draw_training_pause_background(uint8_t* rdram,recomp_context* ctx){
    const auto width=word(rdram,0x8010cfd0),height=word(rdram,0x8010cfd4),sp=uint32_t(ctx->r29);
    if(width<160||width>1280||height<120||height>960||remaining(rdram)<128||sp<0x80000100u||sp>0x80800000u||(sp&3))return;
    // The UI widescreen bridge validates its CPU rectangle inside8MiB. Borrow
    // four words BELOW the owner's SP temporarily and restore them immediately;
    // all native call frames use our separate CPU-only9MiB stack.
    const uint32_t rectangle=sp-0x80;
    std::array<uint32_t,4> prior{};for(unsigned i=0;i<4;i++)prior[i]=word(rdram,rectangle+i*4);
    constexpr uint32_t scratch=0x80850000;
    recomp_context local=*ctx;local.f_odd=&local.f0.u32h;local.r29=S32(scratch+0xf000);
    local.r4=S32(0x80110898);func_800A3F88(rdram,&local);
    MEM_W(0,S32(rectangle))=0;MEM_W(0,S32(rectangle+4))=0;MEM_W(0,S32(rectangle+8))=width-1;MEM_W(0,S32(rectangle+12))=height-1;
    // Exact verified native59CDC..59CF4 ABI: full-screen black RGBA alpha80.
    local.r4=S32(0x80110898);local.r5=S32(rectangle);local.r6=0;local.r7=0;
    MEM_W(0x10,local.r29)=0;MEM_W(0x14,local.r29)=80;
    clay::ui::draw_fullscreen_fill(rdram,&local,&func_800A4D9C);
    for(unsigned i=0;i<4;i++)MEM_W(0,S32(rectangle+i*4))=prior[i];
}
void draw_training_lines(uint8_t* rdram,recomp_context* ctx,const std::vector<std::string>& lines,int first_y,int step,unsigned max_width,uint32_t scratch,uint32_t font_override){
    const auto font=font_override?font_override:word(rdram,0x801028d0);
    if(!valid(font,0x28)||font>=0x80800000u||!valid(scratch,0x10000)||scratch<0x80800000u||remaining(rdram)<320||remaining_matrices(rdram)<4)return;
    const auto viewport_width=word(rdram,0x8010cfd0),viewport_height=word(rdram,0x8010cfd4);
    if(viewport_width<160||viewport_width>1280||viewport_height<120||viewport_height>960)return;
    // Callers lay out in 640x480 design coordinates; the game's orthographic
    // projection uses native framebuffer coordinates (normally 320x240).
    const float layout_scale=std::min(float(viewport_width)/640.f,float(viewport_height)/480.f);
    if(std::getenv("CLAY_TRACE_TRAINING")){
        static thread_local uint32_t previous_font=0;
        if(previous_font!=font){
            previous_font=font;const auto base_map=word(rdram,font+4),variants=word(rdram,font+8),count=word(rdram,font+12);
            const auto base=valid(base_map,94*4)?word(rdram,base_map+('A'-33)*4):0;
            const auto variant=count&&valid(variants,94*4)?word(rdram,variants+('A'-33)*4):0;
            fprintf(stderr,"{\"event\":\"training_font_probe\",\"font\":%u,\"viewport\":[%u,%u],\"base_map\":%u,\"variants\":%u,\"variant_count\":%u,\"base_A\":%u,\"base_size\":[%u,%u],\"variant_A\":%u,\"variant_size\":[%u,%u],\"layout_scale\":%.3f}\n",font,viewport_width,viewport_height,base_map,variants,count,base,valid(base,16)?MEM_HU(0,S32(base+6)):0,valid(base,16)?MEM_HU(0,S32(base+8)):0,variant,valid(variant,16)?MEM_HU(0,S32(variant+6)):0,valid(variant,16)?MEM_HU(0,S32(variant+8)):0,layout_scale);
        }
    }
    constexpr unsigned reserve=256,commands_per_glyph=32;
    const uint32_t matrix=scratch+0x100,orient=scratch+0x180,transform=scratch+0x190,text=scratch+0x200;
    recomp_context local=*ctx;local.f_odd=&local.f0.u32h;local.r29=S32(scratch+0xf000);
    std::array<uint16_t,6> placement{};for(unsigned i=0;i<6;i++)placement[i]=MEM_HU(0,S32(font+0x10+i*2));
    // Native spacing vectors stay unit length. Glyph size belongs in MODELVIEW,
    // as in original59B68; combat's font is32x29, not the15px menu test font.
    for(unsigned i=0;i<3;i++){MEM_W(0,S32(orient+i*4))=i?0:std::bit_cast<uint32_t>(-1.f);MEM_W(0,S32(transform+i*4))=i==1?std::bit_cast<uint32_t>(1.f):0;}
    // Verified native59B68 calls A45BC then A561C before its text matrices.
    // A1B98 loads only MODELVIEW; without A561C glyphs inherit the fight's
    // perspective projection and expand across the entire screen.
    local.r4=S32(0x80110898);func_800A45BC(rdram,&local);
    local.r4=S32(0x80110898);func_800A561C(rdram,&local);
    const auto map=word(rdram,font+4);if(!valid(map,94*4))return;
    const auto reference=word(rdram,map+('A'-33)*4);
    unsigned reference_height=valid(reference,16)?MEM_HU(0,S32(reference+8)):29;
    if(!reference_height||reference_height>256)reference_height=29;
    // The stage selector uses its15px menu font and its own CPU reservation.
    // Keep its single footer at8native pixels even if the caller's step is32.
    const float target_height=(scratch==0x80870000u?16.f:step>=32?24.f:16.f)*layout_scale;
    int y=first_y;
    for(const auto& source:lines){
        auto s=readable_literals(input::native_prompt_text(source));
        // Truncate at token boundary; the shared prompt hook has its own bounded
        // copy and resolves one whole line before measuring/drawing icons.
        if(s.size()>255){s.resize(255);auto token=s.rfind('\x1e');if(token!=std::string::npos&&token+2>=s.size())s.resize(token);}
        unsigned needed=unsigned(s.size())*commands_per_glyph+32;
        if(remaining(rdram)<reserve+needed){y-=step;continue;}
        for(unsigned i=0;i<=s.size();i++)MEM_BU(0,S32(text+i))=i<s.size()?uint8_t(s[i]):0;
        // Resolve the WHOLE caption before splitting, so all dynamic icon map
        // aliases remain attached to this line while individual glyphs draw.
        local.r4=S32(font);local.r5=S32(text);input::prepare_native_prompt_measure(rdram,&local);
        s.clear();for(unsigned i=0;i<255;i++){char ch=MEM_BU(0,S32(uint32_t(local.r5)+i));if(!ch)break;s+=ch;}
        if(remaining(rdram)<reserve+s.size()*commands_per_glyph+32||remaining_matrices(rdram)<3){y-=step;continue;}
        std::vector<float> advances,scales;float width=0;
        for(unsigned char ch:s){
            unsigned lookup=ch;
            if(MEM_HU(0,S32(font+0x26))==2&&lookup>='a'&&lookup<='z')lookup-=32;
            uint32_t glyph=lookup>=33&&lookup<=126?word(rdram,map+(lookup-33)*4):0;
            unsigned height=valid(glyph,16)?MEM_HU(0,S32(glyph+8)):reference_height;
            if(!height||height>256)height=29;
            float glyph_scale=target_height/float(height);
            MEM_BU(0,S32(text))=ch;MEM_BU(0,S32(text+1))=0;
            local.r4=S32(font);local.r5=S32(text);func_80071738(rdram,&local);
            float advance=std::max(0,int(local.r2))*glyph_scale;
            advances.push_back(advance);scales.push_back(glyph_scale);width+=advance;
        }
        if(width<=0){y-=step;continue;}
        float fit=std::min(1.f,float(max_width)*layout_scale/width),pen=-width*fit/2.f;
        const float model_scale=target_height/float(reference_height);
        // Native fight matrix pool has128 slots. One matrix per caption fits;
        // allocating one per byte would exhaust it when opening Move List.
        local.r4=S32(matrix);local.r5=std::bit_cast<uint32_t>(model_scale*fit);local.r6=std::bit_cast<uint32_t>(model_scale);local.r7=std::bit_cast<uint32_t>(1.f);func_800B0328(rdram,&local);
        // B09DC multiplies Translation * currentScale (B0A10), so its native
        // row-vector translation is scaled too. Compensate before composition.
        local.r4=S32(matrix);local.r5=0;local.r6=std::bit_cast<uint32_t>(float(y)*layout_scale/model_scale);local.r7=0;func_800B09DC(rdram,&local);
        local.r4=S32(0x80110898);local.r5=S32(matrix);func_800A1B98(rdram,&local);
        for(unsigned i=0;i<s.size();++i){
            if(s[i]!=' '){
                // Genuine15px icons and29px native letters share a readable
                // final height; neither inherits the other's asset dimensions.
                const float ratio=scales[i]/model_scale;
                MEM_W(0,S32(orient))=std::bit_cast<uint32_t>(-ratio);
                MEM_W(0,S32(transform+4))=std::bit_cast<uint32_t>(ratio);
                for(unsigned j=0;j<6;j++)MEM_H(0,S32(font+0x10+j*2))=0;
                MEM_H(0,S32(font+0x10))=int(pen/(model_scale*fit));
                MEM_H(0,S32(font+0x16))=MEM_HU(0,S32(font+0x10));
                MEM_BU(0,S32(text))=uint8_t(s[i]);MEM_BU(0,S32(text+1))=0;
                local.r4=S32(font);local.r5=0;local.r6=0;local.r7=S32(orient);
                MEM_W(0x10,local.r29)=S32(transform);MEM_W(0x14,local.r29)=0;MEM_W(0x18,local.r29)=S32(text);
                func_8007191C(rdram,&local);
            }
            pen+=advances[i]*fit;
        }
        y-=step;
    }
    for(unsigned i=0;i<6;i++)MEM_H(0,S32(font+0x10+i*2))=placement[i];
}
void draw_hud(uint8_t* rdram,recomp_context* ctx){
    auto s=snapshot();if(!s.active||word(rdram,0x8010849c))return;
    std::vector<std::string> lines={"TRAINING TIME INFINITE"};
    if(s.options.opponent==Opponent::Player2&&!s.second_player_connected)lines.push_back("Player 2 disconnected: Standing");
    if(s.options.damage_display)lines.push_back("Hit "+std::to_string(s.damage.last)+(s.damage.chip?" (guard)":"")+" / Combo "+std::to_string(s.damage.combo)+" / Hits "+std::to_string(s.damage.hits));
    if(s.options.preset==Preset::Claytalities)lines.push_back("Attempts "+std::to_string(s.attempts)+" / Completed "+std::to_string(s.successes));
    // Commands resolve native attack permutation, current host mapping, device
    // family and relative forward/back on every draw; no stale button caption.
    if(s.options.pinned_display&&s.moves&&s.pinned_move>=0&&unsigned(s.pinned_move)<s.moves->size()){
        const auto& m=s.moves->at(unsigned(s.pinned_move));
        lines.push_back(m.name+": "+move_command(m,s.attacks,s.facing_right));
        if(!m.requirements.empty())lines.push_back(m.requirements);
    }
    // History sits near the bottom, separately from bars and the pinned command.
    draw_training_lines(rdram,ctx,lines,108,24,560,0x80860000);
    if(s.options.input_history){
        std::vector<std::string> history;
        const size_t start=s.history.size()>12?s.history.size()-12:0;
        for(size_t i=start;i<s.history.size();i+=2){
            std::string row=s.history[i];if(i+1<s.history.size())row+="    |    "+s.history[i+1];history.push_back(row);
        }
        if(!history.empty())draw_training_lines(rdram,ctx,history,-62,22,560,0x80860000);
    }
}
}
