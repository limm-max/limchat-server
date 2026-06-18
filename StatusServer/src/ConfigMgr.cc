#include "ConfigMgr.h"
#include<iostream>
#include "Logger.h"
#include "const.h"
ConfigMgr::ConfigMgr(){
    std::ifstream ifs("config.json");
    if(!ifs.is_open()){
        LOG_ERROR<<"config.json 文件打开失败！路径失败";
        return;
    }

    json root=json::parse(ifs,nullptr,false);
    if(root.is_discarded()){
        LOG_ERROR<<"config.json 解析失败（格式错误）！";
        return;
    }

    for(auto& [section_name,section_value]:root.items()){
        SectionInfo info;
        for(auto& [key,value]:section_value.items()){
            info._section_datas[key]=
                value.is_string()? value.get<std::string>():value.dump();
        }
        _config_map[section_name]=info;
    }

    LOG_INFO<<"config.json 加载成功";
}