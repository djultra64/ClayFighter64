#include "graphics_config.h"
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <fstream>
static void require(bool pass,const char* reason){if(!pass)throw std::runtime_error(reason);}
int main(){try{
 auto first=clay::graphics_options();
 require(first.refresh==60,"First launch must default to 60 FPS");
 require(first.widescreen&&std::abs(first.aspect()-16.0/9.0)<1e-8,"First launch must use 16:9");
 auto uhd=clay::graphics_profile("4k120");
 require(uhd.width==3840&&uhd.height==2160&&uhd.refresh==120&&uhd.widescreen,"UHD output and presentation preset");
 require(uhd.resolution_scale()==9,"320x240 guest needs 9x vertical resolution for 2160");
 require(std::abs(320*uhd.resolution_scale()*(uhd.aspect()/(4.0/3.0))-3840)<1e-8,"Projection expansion and 9x scaling produce 3840 pixels");
 auto original=clay::graphics_profile("original");require(!original.widescreen&&original.refresh==0,"Original profile retains original aspect/rate");
 auto fhd=clay::graphics_profile("1080p120");require(fhd.resolution_scale()==4.5,"1080 profile uses exact fractional scaling");
 auto bad=uhd;bad.height=0;bool rejected=false;try{clay::validate_graphics(bad);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"Zero height rejected before division");
 bad=uhd;bad.refresh=1000;rejected=false;try{clay::validate_graphics(bad);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"Invalid presentation rate rejected");
 clay::set_graphics_options(uhd);require(clay::graphics_options().render_height==2160,"Explicit selection takes priority over environment");
 std::string error;const std::string path="/tmp/clay-graphics-test.cfg";
 uhd.filter=clay::TextureFilter::Linear;uhd.fullscreen=true;uhd.msaa=4;
 clay::set_graphics_options(uhd);require(clay::save_graphics_options(path,error),"Settings must persist");
 clay::set_graphics_options(original);require(clay::load_graphics_options(path,error)&&clay::graphics_options()==uhd,"Persisted display choices must survive reload");
 {std::ofstream f(path);f<<"width 3840\nheight 0\n";}
 require(!clay::load_graphics_options(path,error)&&clay::graphics_options()==uhd,"Invalid preferences must not partially replace live settings");
 std::remove(path.c_str());
 puts("Graphics profiles, exact UHD dimensions and validation passed");return 0;
}catch(const std::exception& e){fprintf(stderr,"%s\n",e.what());return 1;}}
