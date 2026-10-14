// Portable packages and development builds use only installation-local writes.
// djultra64.
#include "portable_paths.h"
#include <stdexcept>
namespace clay::portable {
namespace fs=std::filesystem;
fs::path installation_root(const fs::path& executable_directory) {
    auto base=fs::weakly_canonical(executable_directory);
    auto candidate=base;
    for(unsigned depth=0;depth<3;++depth) {
        if(fs::is_regular_file(candidate/"config/controls.cfg"))return candidate;
        candidate=candidate.parent_path();
    }
    return base;
}
fs::path settings_directory(const fs::path& root,const fs::path& requested) {
    const auto base=fs::weakly_canonical(root);
    const auto target=fs::weakly_canonical(requested.empty()?base/"local/settings":
                                        requested.is_absolute()?requested:base/requested);
    auto a=base.begin(),b=target.begin();
    for(;a!=base.end();++a,++b)
        if(b==target.end()||*a!=*b)
            throw std::invalid_argument("Portable settings must remain inside the game folder");
    fs::create_directories(target);
    return target;
}
}
