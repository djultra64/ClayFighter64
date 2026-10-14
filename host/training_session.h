// Training rules independent of guest memory and presentation, djultra64.
#pragma once
#include <cstdint>
namespace clay::training {
enum class Preset { Normal, Claytalities };
enum class Phase { Inactive, Selecting, Preparing, Practicing, FinishWindow,
                   FinisherPlaying, RetryWait, ResetPending };
enum class Opponent { Standing, Crouching, Jumping, Cpu, Player2 };
enum class Guard { Off, Always, AfterFirstHit };
enum class Super { Normal, RefillOnReset, Infinite };
enum class Position { Center, LeftCorner, RightCorner };
struct Options {
    Preset preset=Preset::Normal;
    Opponent opponent=Opponent::Standing;
    Guard guard=Guard::Off;
    Super super=Super::RefillOnReset;
    Position position=Position::Center;
    unsigned recovery_ticks=180;
    int last_character=-1,last_opponent=-1;
    bool right_side=false,damage_display=true,input_history=false,
         extended_window=false,direct_finisher=false,automatic_distance=true;
};
struct FrameState {
    bool combat=false,paused=false,attract=false,safe_rest=false;
    bool finish_window=false,finisher_playing=false,attempt_ended=false,success=false;
};
struct FrameEffects {
    bool recover=false,reset=false,prepare=false,extend_window=false;
};
struct DamageView {
    unsigned last=0,combo=0,hits=0;
    bool chip=false;
};
class Session {
    Options options_{};
    Phase phase_=Phase::Inactive;
    unsigned idle_=0,retry_=0,attempts_=0,successes_=0;
    bool damaged_=false,reset_requested_=false,combo_finished_=true;
    DamageView damage_{};
public:
    void begin_selection();
    void begin_combat();
    void leave();
    void request_reset();
    void set_options(const Options& options);
    void damage(unsigned target,unsigned amount,bool chip);
    void combo_end();
    bool combo_active() const{return !combo_finished_;}
    FrameEffects tick(const FrameState& frame);
    // Called after measuring true damage, only for a verified final reserve.
    int protected_hp(int hp,unsigned damage,bool final_reserve) const;
    bool active() const {return phase_!=Phase::Inactive;}
    Phase phase() const {return phase_;}
    const Options& options() const {return options_;}
    const DamageView& damage_view() const {return damage_;}
    unsigned attempts() const {return attempts_;}
    unsigned successes() const {return successes_;}
};
}
