#ifndef __UTIL_COMMON_H__
#define __UTIL_COMMON_H__

#include <locale>
#include <string>
#include <memory>
#include <vector>
#include <cstring>
#include <codecvt>
#include <algorithm>
#include <unordered_map>

#define INSTRUMENT_MAX_LEN 20

namespace util{
    bool mkdirs(const std::string& dir, bool exist_ok);
    
    char** get_instruments(const std::string& instruments, int& count);

    inline char* strcpy_s(char* _Destination , const char* _Source){
        return strcpy(_Destination, _Source);
    }

    inline void lower(std::string& word) {
        std::transform(word.begin(), word.end(), word.begin(),
            [](unsigned char c) {return std::tolower(c); });
    }

    inline std::string lower_copy(const std::string& word) {
        std::string word_t(word.size(), '\0');
        std::transform(word.begin(), word.end(), word_t.begin(),
            [](unsigned char c) {return std::tolower(c); });
        return word_t;
    }

    inline void upper(std::string& word) {
        std::transform(word.begin(), word.end(), word.begin(),
            [](unsigned char c) {return std::toupper(c); });
    }

    inline std::string upper_copy(const std::string& word) {
        std::string word_t(word.size(), '\0');
        std::transform(word.begin(), word.end(), word_t.begin(),
            [](unsigned char c) {return std::toupper(c); });
        return word_t;
    }

    // std::wstring string2wstring(
    //     const std::string& str, const std::string& locale);

    // trim from start (in place)
    inline void ltrim(std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
    }

    // trim from end (in place)
    inline void rtrim(std::string& s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    // trim from both ends (in place)
    inline void trim(std::string& s) {
        ltrim(s);
        rtrim(s);
    }

    // trim from start (copying)
    inline std::string ltrim_copy(std::string s) {
        ltrim(s);
        return s;
    }

    // trim from end (copying)
    inline std::string rtrim_copy(std::string s) {
        rtrim(s);
        return s;
    }

    // trim from both ends (copying)
    inline std::string trim_copy(std::string s) {
        trim(s);
        return s;
    }
 
    // tick datatime to timestamp
    double to_timestamp(
        const std::string& trading_day,
        const std::string& update_time,
        int update_millisec = 0);

    // just like glob.glob in python
    int glob(const std::string& pattern,
        std::vector<std::string>& files);
    
    // just like split in python
    std::vector<std::string> split(const std::string& text, const std::string& delimiter);

    // parse argument
    /* skill display
    template<class Type>
    Type parse_argument(
        const std::unordered_map<std::string, std::string>& args,
        const std::string& key, Type _default) {
        auto iter = args.find(key);
        if (iter == args.end()) {
            return _default;
        }
        if (std::is_same<Type, int>::value)
            return std::stoi(iter->second);
        else if (std::is_same<Type, long>::value)
            return std::stol(iter->second);
        else if (std::is_same<Type, long long>::value)
            return std::stoll(iter->second);
        else if (std::is_same<Type, float>::value)
            return std::stof(iter->second);
        else if (std::is_same<Type, double>::value)
            return std::stod(iter->second);
        else if (std::is_same<Type, long double>::value)
            return std::stold(iter->second);
    }*/

    // parse argument for string
    inline std::string parse_argument(
        const std::unordered_map<std::string, std::string>& args,
        const std::string& key, std::string _default) {
        auto iter = args.find(key);
        if (iter == args.end()) {
            return _default;
        }
        return iter->second;
    }

    // parse argument for string
    inline double parse_argument(
        const std::unordered_map<std::string, std::string>& args,
        const std::string& key, double _default) {
        auto iter = args.find(key);
        if (iter == args.end()) {
            return _default;
        }
        return std::stod(iter->second);
    }
}

#endif // __UTIL_COMMON_H__