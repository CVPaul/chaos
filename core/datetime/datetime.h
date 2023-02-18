#pragma once
#ifndef __DT_DATETIME_H_

#include <ctime>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>


namespace dt {
	class DateTime {
	public:
		int year, month, day, hour, minute, second, millisecond;
	public:
		DateTime(
			int year = 0, int month = 0, int day = 0, int hour = 0,
			int minute = 0, int second = 0, int millisecond = 0);

		DateTime(const std::tm* ptm);

		static DateTime now();

		std::tm to_tm();

		std::string to_string(const std::string& format = "%Y-%m-%d %H:%M:%S");

		static DateTime to_datetime(
			const std::string& str,
			const std::string& format = "%Y-%m-%d %H:%M:%S");

		// operator
		DateTime operator +(int ndays);
		DateTime operator -(int ndays);
	};

	DateTime now();

	DateTime to_datetime(
		const std::string& str,
		const std::string& format = "%Y-%m-%d %H:%M:%S");
}
#endif // !__DT_DATETIME_H_

