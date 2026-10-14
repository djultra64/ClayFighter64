// Versioned portable preferences; no user-data fallback. djultra64.
#include "training_options.h"
#include <fstream>
#include <sstream>
#include <map>
#ifdef _WIN32
#include <windows.h>
#endif
namespace clay::training {
bool load_options(const std::filesystem::path& path,Options& options,std::string& error){
    options={};error.clear();std::ifstream input(path);
    if(!input)return false;
    std::string magic;int version=0;
    if(!(input>>magic>>version)||magic!="ClayFighter64Training"||version!=1){error="Unsupported Training settings";return false;}
    std::string line;std::getline(input,line);std::map<std::string,int> values;
    while(std::getline(input,line)){
        std::istringstream row(line);std::string key;int value;
        if(row>>key>>value)values[key]=value;
    }
    auto value=[&](const char* key,int fallback,int maximum){auto i=values.find(key);return i!=values.end()&&i->second>=0&&i->second<=maximum?i->second:fallback;};
    options.last_character=value("last_character",-1,16);options.last_opponent=value("last_opponent",-1,16);
    if(options.last_character==7)options.last_character=-1;if(options.last_opponent==7)options.last_opponent=-1;
    options.preset=Preset(value("preset",0,1));options.opponent=Opponent(value("opponent",0,4));
    options.guard=Guard(value("guard",0,2));options.super=Super(value("super",1,2));
    options.position=Position(value("position",0,2));
    const unsigned recovery=value("recovery_ticks",180,300);
    options.recovery_ticks=recovery==0||recovery==60||recovery==180||recovery==300?recovery:180;
    options.right_side=value("right_side",0,1);options.damage_display=value("damage_display",1,1);
    options.input_history=value("input_history",0,1);options.extended_window=value("extended_window",0,1);
    options.direct_finisher=value("direct_finisher",0,1);options.automatic_distance=value("automatic_distance",1,1);
    return true;
}
bool save_options(const std::filesystem::path& path,const Options& options,std::string& error){
    error.clear();const auto temp=std::filesystem::path(path.string()+".tmp");
    try{
        if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path());
        {std::ofstream output(temp);
            output<<"ClayFighter64Training 1\n"
                <<"last_character "<<options.last_character<<"\nlast_opponent "<<options.last_opponent<<"\n"
                <<"preset "<<int(options.preset)<<"\nopponent "<<int(options.opponent)
                <<"\nguard "<<int(options.guard)<<"\nsuper "<<int(options.super)
                <<"\nposition "<<int(options.position)<<"\nrecovery_ticks "<<options.recovery_ticks
                <<"\nright_side "<<options.right_side<<"\ndamage_display "<<options.damage_display
                <<"\ninput_history "<<options.input_history<<"\nextended_window "<<options.extended_window
                <<"\ndirect_finisher "<<options.direct_finisher<<"\nautomatic_distance "<<options.automatic_distance<<'\n';
            output.flush();if(!output)throw std::runtime_error("Cannot write Training settings");
        }
#ifdef _WIN32
        if(!MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Cannot replace Training settings");
#else
        std::filesystem::rename(temp,path);
#endif
        return true;
    }catch(const std::exception& exception){error=exception.what();std::error_code ignored;std::filesystem::remove(temp,ignored);return false;}
}
}
