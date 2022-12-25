#ifndef __CONMMON_CONFIG_H__
#define __CONMMON_CONFIG_H__

#include <string>
#include <unordered_map>

namespace mgr{
    // singleton config class
    class Config{
    private:
        static Config* m_instance;
    public:
        std::unordered_map<
            std::string, std::unordered_map<
                std::string, std::string>> content;
    private:
        Config(){};
    public:
        static Config * Get();
        bool init(const std::string& init_file);
        std::string get(const std::string& key);
        std::unordered_map<std::string, std::string>
            get_section(const std::string& section = "global");
    };
}

#endif // __CONMMON_CONFIG_H__