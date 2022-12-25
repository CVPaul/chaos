// #include "stdafx.h"
#include <fstream>
#include <iostream>
#include <unordered_map>

#include "util/config.h"
#include "util/common.h"
#include "util/logging.h"

mgr::Config* mgr::Config::m_instance = nullptr;

mgr::Config* mgr::Config::Get(){
    if(m_instance == nullptr){
        static mgr::Config instance;
        m_instance = &instance;
    }
    return m_instance;
}

bool mgr::Config::init(const std::string& init_file){
	std::ifstream inifile(init_file);
	if (!inifile.is_open()){
		std::cerr << "could not open " << init_file << std::endl;
		inifile.clear();
        return false;
	}
    std::string section("global");
	std::string strtmp, strtitle, strcfgname, returnValue;
	while (getline(inifile, strtmp, '\n')){
        util::trim(strtmp);
        if (strtmp.empty())
            continue;
        else if(strtmp.substr(0, 1) == "#")
            continue; //过滤注释		
        if (strtmp.substr(0, 1) == "[") {
            size_t tail = strtmp.rfind(']');
            if (tail == std::string::npos)
                tail = strtmp.length();
            section = strtmp.substr(1, tail - 1);
            continue; // section
        }
        size_t pos = strtmp.find('=');
        if (pos == std::string::npos) {
            log_warning << "invalid config:" << strtmp;
            continue;
        }
        if (content.find(section) == content.end()) {
            content[section] = std::unordered_map<std::string, std::string>();
        }
        content[section][util::rtrim_copy(strtmp.substr(0, pos))] = \
            util::ltrim_copy(strtmp.substr(pos + 1));
	}
    return true;
}

std::string mgr::Config::get(const std::string& key){
    std::string section("global"), field(key);
    auto items = util::split(key, "::");
    if (items.size() > 1) {
        if (items.size() > 2) {
            log_warning << "key level > 2 is not support now, " \
                "using first two instead, key:" << key;
        }
        section = items[0];
        field = items[1];
    }
    auto sec = get_section(section);
    auto iter = sec.find(field);
    if (iter == sec.end()){
        log_error << key << " is not containted in the config with section:" << section;
        throw "unknow key:" + key;
        return "";
    }
    return iter->second;
}

std::unordered_map<std::string, std::string>
mgr::Config::get_section(const std::string& section) {
    if (content.find(section) == content.end()) {
        log_error << "section:" << section << "is not containted in this config";
        throw "unknow section:" + section;
    }
    return content[section];
}