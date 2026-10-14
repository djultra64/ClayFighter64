// Native Training menu and portable settings, djultra64.
#pragma once
#include "training_session.h"
#include <filesystem>
#include <string>
namespace clay::training {
bool load_options(const std::filesystem::path& path,Options& options,std::string& error);
bool save_options(const std::filesystem::path& path,const Options& options,std::string& error);
void initialize_storage(const std::filesystem::path& path);
}
