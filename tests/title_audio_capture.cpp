// Installation-local optional capture fixture. djultra64.
#include "title_audio.h"
#include <array>
extern "C" void func_800C1520(uint8_t*,recomp_context*){}
int main(){
 clay::audio::title_audio_frequency(32000);
 std::array<int16_t,4> samples{1,-1,200,-200};
 clay::audio::mix_title_audio(samples);
}
