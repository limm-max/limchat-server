//读取配置的mgr，全局单例
#pragma once
#include <string>
#include <map>
#include <fstream>
#include "Singleton.h"
struct SectionInfo{
    std::map<std::string,std::string> _section_datas;

    //重载[]
    std::string operator[](const std::string& key){
        auto it=_section_datas.find(key);
        if(it==_section_datas.end()){
            return "";

        }
        return it->second;
    }
};

class ConfigMgr:public Singleton<ConfigMgr>{
    friend class Singleton<ConfigMgr>;
public:
    SectionInfo operator[](const std::string& section){
        auto it=_config_map.find(section);
        if(it==_config_map.end()){
            return SectionInfo{};
        }
        return it->second;
    }

private:
    ConfigMgr();
    std::map<std::string,SectionInfo> _config_map;
};