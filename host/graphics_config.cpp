// ClayTality 64 graphics configuration by djultra64.
#include "graphics_config.h"
#include <cstdlib>
#include <stdexcept>
#include <optional>
#include <mutex>
#include <fstream>
#include <filesystem>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif
namespace clay {
namespace {std::optional<GraphicsOptions> selected;std::mutex config_mutex;std::string storage;}
GraphicsOptions graphics_profile(std::string_view name){
 if(name=="default")return {};
 if(name=="original")return {960,720,480,false,0};
 if(name=="1080p120")return {1920,1080,1080,true,120};
 if(name=="4k120")return {3840,2160,2160,true,120};
 throw std::invalid_argument("Graphics profile must be default, original, 1080p120 or 4k120");
}
void validate_graphics(const GraphicsOptions& options){
 if(options.width<320||options.width>7680||options.height<240||options.height>4320)
  throw std::invalid_argument("Output dimensions must be 320..7680 by 240..4320");
 if(options.render_height<240||options.render_height>7680)
  throw std::invalid_argument("Render height must be 240..7680");
 if(options.widescreen&&(options.aspect()<4.0/3.0||options.aspect()>32.0/9.0))
  throw std::invalid_argument("Widescreen aspect must be 4:3 through 32:9");
 if(options.refresh!=0&&(options.refresh<30||options.refresh>240))
  throw std::invalid_argument("Presentation rate must be original (0), or 30..240");
 if(unsigned(options.filter)>unsigned(TextureFilter::Linear))throw std::invalid_argument("Invalid texture filter");
 if(options.msaa!=0&&options.msaa!=2&&options.msaa!=4&&options.msaa!=8)throw std::invalid_argument("Invalid antialiasing sample count");
}
void set_graphics_options(const GraphicsOptions& options){validate_graphics(options);std::lock_guard lock(config_mutex);selected=options;}
GraphicsOptions graphics_options(){
 std::lock_guard lock(config_mutex);
 if(selected)return *selected;
 const auto* profile=std::getenv("CLAY_GRAPHICS_PROFILE");
 auto options=graphics_profile(profile?profile:"default");validate_graphics(options);return options;
}
void set_graphics_storage(const std::string& path){std::lock_guard lock(config_mutex);storage=path;}
bool persist_graphics_options(std::string& error){std::string path;{std::lock_guard lock(config_mutex);path=storage;}if(path.empty()){error="Display storage unavailable";return false;}return save_graphics_options(path,error);}
bool save_graphics_options(const std::string& path,std::string& error){
 try{
  auto o=graphics_options();const auto dest=std::filesystem::path(path),tmp=std::filesystem::path(path+".tmp");
  if(!dest.parent_path().empty())std::filesystem::create_directories(dest.parent_path());
  {std::ofstream f(tmp);if(!f)throw std::runtime_error("Cannot write display settings");
   f<<"# ClayTality64 display by djultra64\nwidth "<<o.width<<"\nheight "<<o.height<<"\nrender_height "<<o.render_height<<"\nwidescreen "<<o.widescreen<<"\nrefresh "<<o.refresh<<"\nfilter "<<unsigned(o.filter)<<"\nfullscreen "<<o.fullscreen<<"\nmsaa "<<o.msaa<<'\n';f.flush();if(!f)throw std::runtime_error("Display settings write failed");}
#ifdef _WIN32
  if(!MoveFileExW(tmp.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot replace display settings");
#else
  std::filesystem::rename(tmp,dest);
#endif
  return true;
 }catch(const std::exception& e){error=e.what();return false;}
}
bool load_graphics_options(const std::string& path,std::string& error){
 try{
  if(!std::filesystem::exists(path))return true;
  std::ifstream f(path);if(!f)throw std::runtime_error("Cannot read display settings");GraphicsOptions o;std::string line;
  while(std::getline(f,line)){if(line.empty()||line[0]=='#')continue;std::istringstream row(line);std::string key,extra;unsigned value;if(!(row>>key>>value)||row>>extra)throw std::runtime_error("Malformed display setting");
   if(key=="width")o.width=value;else if(key=="height")o.height=value;else if(key=="render_height")o.render_height=value;else if(key=="refresh")o.refresh=value;else if(key=="msaa")o.msaa=value;else if(key=="filter")o.filter=TextureFilter(value);
   else if(key=="widescreen"&&value<=1)o.widescreen=value;else if(key=="fullscreen"&&value<=1)o.fullscreen=value;else throw std::runtime_error("Unknown or invalid display setting");
  }
  if(!f.eof())throw std::runtime_error("Display settings read failed");set_graphics_options(o);return true;
 }catch(const std::exception& e){error=e.what();return false;}
}
}
