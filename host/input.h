// ClayTality 64 action input layer by djultra64.
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
struct _SDL_GameController;
union SDL_Event;
namespace clay::input {
enum class Family { Auto, Keyboard, Xbox, PlayStation, Switch };
enum class Action { Primary, Secondary, Start, Trigger, LeftShoulder, RightShoulder,
    CUp, CDown, CLeft, CRight, Up, Down, Left, Right, TrainingReset, TrainingPause, Count };
enum class Physical { South, East, West, North, Start, Back, LeftShoulder, RightShoulder,
    LeftTrigger, RightTrigger, DUp, DDown, DLeft, DRight, LeftStick, RightStick, Count };
constexpr size_t action_count = size_t(Action::Count), physical_count = size_t(Physical::Count);
struct Binding { Physical button; std::string key; };
struct Snapshot {
    std::array<bool, physical_count> buttons{};
    std::array<bool, action_count> keys{};
    float x = 0, y = 0;
    Family detected = Family::Xbox;
    bool keyboard_activity = false, controller_activity = false;
};
struct State { uint16_t buttons = 0; float x = 0, y = 0; Family active = Family::Keyboard;
    bool training_reset=false,training_pause=false; };
struct Prompt { Action action; std::string action_name, glyph, button_name; };
class Mapper {
    std::array<Binding, action_count> bindings_;
    Family manual_ = Family::Auto;
    Family active_ = Family::Keyboard;
    Family detected_ = Family::Xbox;
public:
    explicit Mapper(bool menu = false);
    void set_family(Family family);
    Family family() const { return active_ == Family::Keyboard ? active_ : (manual_ == Family::Auto ? detected_ : manual_); }
    Family manual_family() const { return manual_; }
    void remap(Action action, Physical button);
    void remap_key(Action action, const std::string& scancode_name);
    const Binding& binding(Action action) const;
    State update(const Snapshot& snapshot);
    Prompt prompt(Action action) const;
    bool load(const std::string& path, std::string& error);
    bool save(const std::string& path, std::string& error) const;
};
Family parse_family(const std::string& name);
const char* family_name(Family family);
const char* action_name(Action action);
// Call on the SDL event/polling thread. Initialize before SDL_Init.
void configure_sdl();
class SdlInput {
    Mapper mapper_;
    Mapper second_mapper_;
    Mapper menu_mapper_{true};
    _SDL_GameController* pad_ = nullptr;
    std::vector<_SDL_GameController*> pads_;
    int32_t instance_ = -1;
    bool keyboard_activity_ = false, pad_activity_ = false;
    bool wait_release_ = false,training_mode_=false,previous_combat_=false;
    struct Activity {float x=0,y=0;std::array<bool,2> trigger_down{};};
    std::unordered_map<int32_t,Activity> activity_;
    std::array<int32_t,2> preferred_{{-1,-1}}, assigned_{{-1,-1}};
    std::array<bool,2> connected_{{true,false}};
    void find_controller();
public:
    ~SdlInput();
    Mapper& mapper(unsigned player=0) { return player==0?mapper_:second_mapper_; }
    const Mapper& active_mapper(bool combat) const;
    void event(const SDL_Event& event);
    void set_training_mode(bool active){training_mode_=active;}
    void set_preferred_instances(int32_t player_one,int32_t player_two=-1);
    std::array<State,2> poll_players(bool combat=false);
    std::array<bool,2> connected_players() const { return connected_; }
    State poll(bool combat = false);
};
// Native Options queues requests; SDL remains on its main thread.
void begin_binding_edit();
void end_binding_edit();
unsigned binding_player();
void request_binding(Action action);
bool binding_capture_active();
std::string binding_label(Action action);
std::string binding_capture_status();
void reset_combat_bindings();
void set_controller_family(Family family);
Family controller_family();
void set_controls_storage(const std::string& path);
// Integrate these callbacks with the existing renderer's HUD pass. Coordinates
// are virtual 960x720 pixels. They must draw into the same game swapchain.
struct Hud {
    void* context = nullptr;
    void (*panel)(void*, float, float, float, float) = nullptr;
    void (*text)(void*, float, float, const char*) = nullptr;
};
void draw_prompts(const Mapper& mapper, const Hud& hud, const std::vector<Action>& actions);
}
