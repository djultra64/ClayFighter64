// Deterministic training behavior independent of render rate, djultra64.
#include "training_session.h"
#include <cassert>
#include <iostream>
using namespace clay::training;
int main() {
    FrameState frame;frame.combat=true;frame.safe_rest=true;
    Session session;
    assert(!session.tick(frame).recover);
    session.begin_selection();session.begin_combat();
    session.damage(1,30,false);
    for(unsigned i=0;i<179;i++)assert(!session.tick(frame).recover);
    assert(session.tick(frame).recover);
    assert(session.damage_view().last==30&&session.damage_view().combo==30);

    session.damage(1,5,false);
    for(unsigned i=0;i<100;i++)assert(!session.tick(frame).recover);
    frame.paused=true;
    for(unsigned i=0;i<1000;i++)assert(!session.tick(frame).recover);
    frame.paused=false;frame.safe_rest=false;
    for(unsigned i=0;i<1000;i++)assert(!session.tick(frame).recover);
    frame.safe_rest=true;
    for(unsigned i=0;i<79;i++)assert(!session.tick(frame).recover);
    assert(session.tick(frame).recover);

    session.damage(1,11,false);
    for(unsigned i=0;i<170;i++)assert(!session.tick(frame).recover);
    session.damage(1,7,true);
    for(unsigned i=0;i<179;i++)assert(!session.tick(frame).recover);
    assert(session.tick(frame).recover);
    assert(session.damage_view().chip);
    session.combo_end();session.damage(1,300,false);
    assert(session.damage_view().combo==300); // Unclamped lethal damage.
    session.damage(0,12,false);
    assert(session.damage_view().combo==300); // Dummy's damage does not inflate P1 combo.

    Options options;options.recovery_ticks=0;session.set_options(options);
    session.damage(1,10,false);
    for(unsigned i=0;i<2000;i++)assert(!session.tick(frame).recover);
    session.request_reset();assert(session.tick(frame).reset);
    assert(!session.tick(frame).reset);

    assert(session.protected_hp(1,100,true)==1);
    assert(session.protected_hp(170,20,true)==150);
    assert(session.protected_hp(1,100,false)==0); // Preserve a non-final reserve's transition.
    session.leave();assert(session.protected_hp(1,100,true)==0);
    assert(!session.tick(frame).recover&&!session.active());

    options.preset=Preset::Claytalities;options.extended_window=true;
    session.set_options(options);session.begin_selection();session.begin_combat();
    assert(session.tick(frame).prepare);
    frame.finish_window=true;
    assert(session.tick(frame).extend_window);
    frame.finish_window=false;frame.finisher_playing=true;
    for(unsigned i=0;i<1000;i++) {
        const auto effect=session.tick(frame);
        assert(!effect.reset&&!effect.recover);
    }
    frame.finisher_playing=false;frame.attempt_ended=true;frame.success=true;
    assert(!session.tick(frame).reset);
    frame.attempt_ended=false;
    frame.paused=true;
    for(unsigned i=0;i<100;i++)assert(!session.tick(frame).reset);
    frame.paused=false;
    for(unsigned i=0;i<59;i++)assert(!session.tick(frame).reset);
    assert(session.tick(frame).reset);
    assert(session.attempts()==1&&session.successes()==1);
    assert(session.protected_hp(1,100,true)==0); // Finishers can defeat the dummy.
    session.begin_combat();assert(session.tick(frame).prepare);
    frame.attempt_ended=true;frame.success=false;session.tick(frame);
    frame.attempt_ended=false;
    for(unsigned i=0;i<60;i++)session.tick(frame);
    assert(session.attempts()==2&&session.successes()==1);
    session.leave();assert(!session.active());
    session.begin_selection();session.begin_combat();frame.attempt_ended=false;frame.finisher_playing=false;session.tick(frame);
    frame.finisher_playing=true;session.request_reset();
    for(unsigned i=0;i<300;i++)assert(!session.tick(frame).reset);
    frame.finisher_playing=false;assert(session.tick(frame).reset);
    session.leave();
    std::cout<<"PASS recovery ticks, pause/rest gating, damage events, KO protection and finisher retry lifecycle\n";
}
