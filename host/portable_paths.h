// Executable-relative portable storage, djultra64.
#pragma once
#include <filesystem>
namespace clay::portable {
std::filesystem::path installation_root(const std::filesystem::path& executable_directory);
// Overrides must stay inside the installation, including symlink resolution.
std::filesystem::path settings_directory(const std::filesystem::path& root,
                                        const std::filesystem::path& requested={});
}
