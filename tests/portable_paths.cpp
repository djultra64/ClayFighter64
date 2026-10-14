// Portable installation paths and write boundaries, djultra64.
#include "portable_paths.h"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
int main() {
    const auto root=fs::temp_directory_path()/
        ("clay portable paths "+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(root/"config");fs::create_directories(root/"build/Release");
    std::ofstream(root/"config/controls.cfg")<<"fixture";
    assert(clay::portable::installation_root(root)==fs::weakly_canonical(root));
    assert(clay::portable::installation_root(root/"build")==fs::weakly_canonical(root));
    assert(clay::portable::installation_root(root/"build/Release")==fs::weakly_canonical(root));
    const auto settings=clay::portable::settings_directory(root);
    assert(settings==root/"local/settings"&&fs::is_directory(settings));
    const auto isolated=clay::portable::settings_directory(root,"local/tests/profile");
    assert(isolated==root/"local/tests/profile");
    bool rejected=false;
    try {clay::portable::settings_directory(root,"../escaped");}
    catch(const std::invalid_argument&) {rejected=true;}
    assert(rejected&&!fs::exists(root.parent_path()/"escaped"));
    rejected=false;
    try {clay::portable::settings_directory(root,fs::temp_directory_path());}
    catch(const std::invalid_argument&) {rejected=true;}
    assert(rejected);
    const auto elsewhere=fs::current_path();fs::current_path(root/"build");
    assert(clay::portable::settings_directory(root)==settings);
    fs::current_path(elsewhere);
    fs::remove_all(root);
    std::cout<<"PASS executable-relative portable settings, spaces, development layouts and escape rejection\n";
}
