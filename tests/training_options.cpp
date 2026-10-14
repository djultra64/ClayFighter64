// Portable Training option persistence and invalid input behavior, djultra64.
#include "training_options.h"
#include <cassert>
#include <fstream>
#include <filesystem>
#include <iostream>
int main(int argc,char** argv){
    assert(argc==2);const std::filesystem::path path=argv[1];
    clay::training::Options options;
    options.preset=clay::training::Preset::Claytalities;
    options.recovery_ticks=300;options.input_history=true;options.extended_window=true;
    options.last_character=16;options.last_opponent=6;options.pinned_display=false;
    options.opponent=clay::training::Opponent::Jumping;
    std::string error;
    assert(clay::training::save_options(path,options,error));
    clay::training::Options loaded;
    assert(clay::training::load_options(path,loaded,error));
    assert(loaded.preset==options.preset&&loaded.recovery_ticks==300&&loaded.input_history&&loaded.extended_window);
    assert(loaded.last_character==16&&loaded.last_opponent==6&&!loaded.pinned_display);
    assert(loaded.opponent==clay::training::Opponent::Jumping);
    {std::ofstream f(path);f<<"ClayFighter64Training 1\npreset 99\nrecovery_ticks -1\ninput_history 3\nfuture_option 7\n";}
    assert(clay::training::load_options(path,loaded,error));
    assert(loaded.preset==clay::training::Preset::Normal&&loaded.recovery_ticks==180&&!loaded.input_history);
    {std::ofstream f(path);f<<"ClayFighter64Training 99\n";}
    assert(!clay::training::load_options(path,loaded,error));
    assert(loaded.preset==clay::training::Preset::Normal);
    std::filesystem::remove(path);
    std::cout<<"PASS Training preferences roundtrip, invalid values/defaults and unknown schema rejection\n";
}
