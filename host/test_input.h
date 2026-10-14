// Explicit hidden validation driver; djultra64.
#pragma once
#include <memory>
#include <string>
#include <cstdint>
#include <atomic>
#include <array>
namespace clay::testing {
enum class InputPhase {Startup,MainMenu,FighterSelect,OpponentSelect,StageSelect,Loading,Combat,Pause};
inline std::atomic<InputPhase> observed_input_phase{InputPhase::Startup};
inline void publish_input_phase(InputPhase phase){observed_input_phase.store(phase,std::memory_order_release);}
inline InputPhase current_input_phase(){return observed_input_phase.load(std::memory_order_acquire);}
inline std::array<std::atomic<uint16_t>,2> script_seen_buttons{};
inline void observe_script_input(unsigned player,uint16_t buttons){if(player<2)script_seen_buttons[player].store(buttons,std::memory_order_release);}
class InputScript {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 explicit InputScript(const std::string& path);
 ~InputScript();
 void tick();
 int32_t instance_id(unsigned player=0) const;
};
}
