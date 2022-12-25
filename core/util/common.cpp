// #include "stdafx.h"

#include <io.h>
#include <list>
#include <regex>
#include <time.h>
#include <vector>
#include <cstring>
#include <iostream>
#include <direct.h>
#include <algorithm>
#include <sys/stat.h>
#include <filesystem>

#include "util/common.h"


namespace util {
// namespace utils
bool mkdirs(const std::string& dir, bool exist_ok) {
	struct stat st;
	const char* c_dir = dir.c_str();
	for (int i = 0; i < dir.length(); i++) {
		std::string folder(c_dir, c_dir + i);
		if (c_dir[i] == '/' && stat(folder.c_str(), &st) == -1) {
			if (exist_ok == false) {
				std::cerr << "folder is already exists, set exists_ok=true to skip check" << std::endl;
				return false;
			}
#ifdef _WIN32
			int _ = _mkdir(folder.c_str());
#else
			mkdir(folder.c_str(), 0644);
#endif
		}
	}
	if (stat(dir.c_str(), &st) == -1) {
		if (exist_ok == false) {
			std::cerr << "folder is already exists, set exists_ok=true to skip check" << std::endl;
			return false;
		}
#ifdef _WIN32
		int _ = _mkdir(dir.c_str());
#else
		mkdir(dir.c_str(), 0644);
#endif
	}
	return true;
}

char** get_instruments(const std::string& instruments, int& count) {
	if (instruments.empty())
		return nullptr;
	std::list<std::pair<int, int>> instrument_list;
	int last_pos = 0;
	const char* c_str = instruments.c_str();
	for (int i = 1; i < instruments.length(); i++) {
		if (c_str[i] == ',') {
			if ((last_pos - i) > INSTRUMENT_MAX_LEN) {
				std::cerr << "skip " << std::string(c_str + last_pos, c_str + i)
					<< ", whose size had exceeded INSTRUMENT_MAX_LEN("
					<< INSTRUMENT_MAX_LEN << ")" << std::endl;
			}
			else {
				instrument_list.push_back(std::make_pair(last_pos, i));
				last_pos = i + 1;
			}
		}
	}
	if (last_pos < instruments.length()) {
		instrument_list.push_back(
			std::make_pair(last_pos, instruments.length()));
	}
	count = 0;
	char** pp = new char* [instrument_list.size()];
	for (auto iter = instrument_list.begin(); iter != instrument_list.end(); iter++) {
		pp[count] = new char[INSTRUMENT_MAX_LEN];
		memcpy(pp[count], c_str + iter->first, iter->second - iter->first);
		count += 1;
	}
	return pp;
}

// std::wstring string2wstring(
//     const std::string& str, const std::string& locale){
//     typedef std::codecvt_byname<wchar_t, char, std::mbstate_t> F;
//     static std::wstring_convert<F> strCnv(new F(locale));
//     return strCnv.from_bytes(str);
// }


double to_timestamp(
	const std::string& trading_day,
	const std::string& update_time,
	int update_millisec) {
	// func body
	struct tm timeinfo;
	memset(&timeinfo, 0, sizeof(timeinfo));
	int day = atoi(trading_day.c_str());
	timeinfo.tm_mday = day % 100;
	day = day / 100;
	timeinfo.tm_mon = day % 100 - 1;
	day = day / 100;
	timeinfo.tm_year = day - 1900;

	timeinfo.tm_hour = atoi(update_time.substr(0, 2).c_str());
	timeinfo.tm_min = atoi(update_time.substr(3, 2).c_str());
	timeinfo.tm_sec = atoi(update_time.substr(6, 2).c_str());

	double timestamp = mktime(&timeinfo);
	timestamp += update_millisec / 1.0e6;
	return timestamp;
}

int glob(const std::string& pattern, std::vector<std::string>& files) {
	//function body
	long long hfile(0), count(0);
	struct _finddata_t fileInfo;
	size_t pos = pattern.find_last_of("/\\");
	std::string folder = pos == std::string::npos ? "" : pattern.substr(0, pos + 1);
	if ((hfile = _findfirst(pattern.c_str(), &fileInfo)) == -1) {
		return 0;
	}
	do {
		if (fileInfo.attrib & _A_SUBDIR) // directory
			continue;
		files.push_back(folder + fileInfo.name);
		count += 1;
	} while (_findnext(hfile, &fileInfo) == 0);
	_findclose(hfile);
	return count;
}

std::vector<std::string> split(const std::string& text, const std::string& delimiter) {
	std::regex sep(delimiter);
	return std::vector<std::string>(
		std::sregex_token_iterator(text.begin(), text.end(), sep, -1),
		std::sregex_token_iterator());
}

} // end of util namespace