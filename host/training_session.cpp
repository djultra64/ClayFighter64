// Own work: djultra64. Only the guest bridge applies the returned effects.
#include "training_session.h"
#include <algorithm>
#include <limits>
namespace clay::training {
void Session::begin_selection() {
    phase_=Phase::Selecting;idle_=retry_=attempts_=successes_=0;
    damaged_=reset_requested_=false;combo_finished_=true;damage_={};
}
void Session::begin_combat() {
    if(!active())return;
    phase_=options_.preset==Preset::Claytalities?Phase::Preparing:Phase::Practicing;
    idle_=retry_=0;damaged_=reset_requested_=false;combo_finished_=true;damage_={};
}
void Session::leave() {
    phase_=Phase::Inactive;idle_=retry_=attempts_=successes_=0;
    damaged_=reset_requested_=false;combo_finished_=true;damage_={};
}
void Session::request_reset() {if(active())reset_requested_=true;}
void Session::set_options(const Options& options) {
    const bool change_preset=options.preset!=options_.preset;
    options_=options;
    if(options_.recovery_ticks!=0&&options_.recovery_ticks!=60&&
       options_.recovery_ticks!=180&&options_.recovery_ticks!=300)options_.recovery_ticks=180;
    idle_=0;
    if(change_preset&&active()&&phase_!=Phase::Selecting)request_reset();
}
void Session::damage(unsigned target,unsigned amount,bool chip) {
    if(!active()||phase_==Phase::Selecting||target>1||amount==0)return;
    damaged_=true;idle_=0;
    if(target!=1)return;
    if(combo_finished_){damage_.combo=damage_.hits=0;combo_finished_=false;}
    damage_.last=amount;damage_.chip=chip;
    damage_.combo+=std::min(amount,std::numeric_limits<unsigned>::max()-damage_.combo);
    if(damage_.hits<std::numeric_limits<unsigned>::max())++damage_.hits;
}
void Session::combo_end() {combo_finished_=true;}
int Session::protected_hp(int hp,unsigned amount,bool final_reserve) const {
    const auto result=std::max<int64_t>(0,int64_t(hp)-amount);
    if(active()&&phase_!=Phase::Selecting&&options_.preset==Preset::Normal&&final_reserve)
        return int(std::max<int64_t>(1,result));
    return int(result);
}
FrameEffects Session::tick(const FrameState& frame) {
    FrameEffects effect;
    if(!active()||phase_==Phase::Selecting||!frame.combat||frame.paused||frame.attract)return effect;
    if(reset_requested_&&!frame.finisher_playing) {
        reset_requested_=false;phase_=Phase::ResetPending;effect.reset=true;
        damaged_=false;idle_=retry_=0;damage_={};combo_finished_=true;
        return effect;
    }
    if(options_.preset==Preset::Normal) {
        if(frame.safe_rest&&damaged_&&options_.recovery_ticks&&++idle_>=options_.recovery_ticks) {
            effect.recover=true;damaged_=false;idle_=0;
        }
        return effect;
    }
    if(phase_==Phase::Preparing) {
        phase_=Phase::Practicing;effect.prepare=true;return effect;
    }
    if(frame.finisher_playing) {phase_=Phase::FinisherPlaying;return effect;}
    if(frame.attempt_ended&&phase_!=Phase::RetryWait&&phase_!=Phase::ResetPending) {
        ++attempts_;if(frame.success)++successes_;
        phase_=Phase::RetryWait;retry_=0;return effect;
    }
    if(phase_==Phase::RetryWait) {
        if(++retry_>=60){phase_=Phase::ResetPending;effect.reset=true;}
        return effect;
    }
    if(frame.finish_window) {
        effect.extend_window=options_.extended_window&&phase_!=Phase::FinishWindow;
        phase_=Phase::FinishWindow;
    }
    return effect;
}
}
