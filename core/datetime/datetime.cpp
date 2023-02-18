#include "datetime/datetime.h"

namespace dt {
	DateTime::DateTime(
		int year, int month, int day, int hour,
		int minute, int second, int millisecond) {
		// function body
		this->day = day;
		this->hour = hour;
		this->year = year;
		this->month = month;
		this->minute = minute;
		this->second = second;
		this->millisecond = millisecond;
	}

	DateTime::DateTime(const std::tm* ptm) {
		// function body
		this->day = ptm->tm_mday;
		this->hour = ptm->tm_hour;
		this->year = ptm->tm_year;
		this->month = ptm->tm_mon;
		this->minute = ptm->tm_min;
		this->second = ptm->tm_sec;
		this->millisecond = 0;
	}

	DateTime DateTime::now() {
		std::time_t t = std::time(0);
		std::tm* now = std::localtime(&t);
		return DateTime(now);
	}

	std::tm DateTime::to_tm() {
		std::tm ttm;
		ttm.tm_year = year;
		ttm.tm_mon = month;
		ttm.tm_mday = day;
		ttm.tm_hour = hour;
		ttm.tm_min = minute;
		ttm.tm_sec = second;
		return ttm;
	}

	std::string DateTime::to_string(const std::string& format) {
		char buffer[64];
		memset(buffer, 0, sizeof(buffer));
		std::tm ttm = to_tm();
		strftime(buffer, sizeof(buffer), format.c_str(), &ttm);
		return buffer;
	}

	DateTime DateTime::to_datetime(
		const std::string& str, const std::string& format) {
		// function body
		std::tm t = { 0 };
		std::istringstream ss(str);
		ss >> std::get_time(&t, format.c_str());
		return DateTime(&t);
	}

	DateTime DateTime::operator -(int ndays) {
		std::tm ttm = to_tm();
		ttm.tm_mday -= ndays;
		std::mktime(&ttm);
		return DateTime(&ttm);
	}

	DateTime DateTime::operator +(int ndays) {
		std::tm ttm = to_tm();
		ttm.tm_mday += ndays;
		std::mktime(&ttm);
		return DateTime(&ttm);
	}

	DateTime now() {
		return DateTime::now();
	}

	DateTime to_datetime(
		const std::string& str, const std::string& format) {
		return DateTime::to_datetime(str, format);
	}
}

