// ClayTality 64 graphics configuration by djultra64.
#pragma once
#include <string_view>
#include <string>
namespace clay {
enum class TextureFilter { Original, Nearest, Linear };
struct GraphicsOptions {
 unsigned width=1280,height=720;
 unsigned render_height=720;
 bool widescreen=true;
 unsigned refresh=60; // Zero preserves the original presentation rate.
 TextureFilter filter=TextureFilter::Original;
 bool fullscreen=false;
 unsigned msaa=0;
 double aspect() const {return double(width)/height;}
 double resolution_scale() const {return double(render_height)/240.0;}
 bool operator==(const GraphicsOptions&) const = default;
};
GraphicsOptions graphics_profile(std::string_view name);
void validate_graphics(const GraphicsOptions&);
// Shared settings are read as immutable copies by SDL and renderer threads.
void set_graphics_options(const GraphicsOptions&);
GraphicsOptions graphics_options();
// An absent file preserves first-launch defaults. Invalid files leave state intact.
bool load_graphics_options(const std::string& path,std::string& error);
bool save_graphics_options(const std::string& path,std::string& error);
void set_graphics_storage(const std::string& path);
bool persist_graphics_options(std::string& error);
}
