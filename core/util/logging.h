#ifndef __UTIL_LOGGING_H__
#define __UTIL_LOGGING_H__

// #define USING_ASYNC_LOG_MODE // using sync log sink for debug reason
#include <ctime>
#include <string>
#include <atomic>
#include <chrono>
#include <iomanip>
#include "glog/logging.h"

using date = std::chrono::system_clock;

namespace util{
	class Logger {
	private:
		Logger() {};
	public:
		static std::string rotation_date;
	private:
		static void custom_prefix(
			std::ostream &sin, const google::LogMessageInfo &l, void*);
	public:
		// functions
		static bool init(
			std::string strLogPath, std::string strLogName,
			int offset_sec=0/*transform date time in log filename with nOffsetSec*/);
		static void stop();
		static void update_rotation_time();
	private:
		static int m_nOffsetSec;
		static std::atomic_bool m_bInit;
		static std::string m_strLogPath;
		static std::string m_strLogName;
	};
}

#define log_trace	LOG(INFO) << "\b|"
#define log_debug	DLOG(INFO) << "\b|"
#define log_info	LOG(INFO) << "\b|"
#define log_warning	LOG(WARNING) << "\b|"
#define log_error	LOG(ERROR) << "\b|"
// 注意在调用完Fatal error之后程序会在打印完成后退出
#define log_fatal	LOG(FATAL) << "\b|"

#endif // __UTIL_LOGGING_H__