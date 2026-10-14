// User-provided private title announcement integration by djultra64.
#pragma once
#include "recomp.h"
#include <cstdint>
#include <cstddef>
#include <span>
#include <string>
#include <vector>
#include <mutex>
namespace clay::audio {
class TitleAudioMixer {
public:
 bool load_pcm(const std::string&path,std::string&error);
 void set_frequency(uint32_t hz);
 bool ready()const;
 bool active()const;
 bool trigger(uint16_t gain); // Returns handled also for a duplicate active event.
 void mix(std::span<int16_t> stereo);
private:
 mutable std::mutex mutex;
 std::vector<int16_t> mono;
 uint32_t frequency=32000;uint64_t phase=0,output_frames=0;uint16_t volume=0;bool playing=false;
};
bool load_title_audio(const std::string&path,std::string&error);
void title_audio_frequency(uint32_t hz);
void mix_title_audio(std::span<int16_t> stereo);
// Read-only diagnostic at original sound-dispatch entry.
void trace_native_sound(uint8_t*rdram,recomp_context*ctx);
// ONLY intro calls PC80065D74 (start) and PC80066488 (original tail).
void play_title_announcement(uint8_t*rdram,recomp_context*ctx,recomp_func_t*original);
void finish_title_announcement(uint8_t*rdram,recomp_context*ctx,recomp_func_t*original);
}
