// Authentic donor text through the original biography animation; djultra64.
#pragma once
#include "recomp.h"
#include <string>
namespace clay::bios {
// Load once before guest threads. Missing/malformed data leaves original entries intact.
bool load(const std::string& path,std::string& error);
void install(uint8_t* rdram,recomp_context* ctx);
}
