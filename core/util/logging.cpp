// #include "stdafx.h"
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>

#include "util/common.h"
#include "util/logging.h"

// variables

namespace util{
	int Logger::m_nOffsetSec = 0;
	std::string Logger::m_strLogPath = "";
	std::string Logger::m_strLogName = "";
	std::atomic_bool Logger::m_bInit(false);
	// datetime
	std::string Logger::rotation_date = "";

	/* This function writes a prefix that matches glog's default format.
 	* (The third parameter can be used to receive user-supplied data, and is
 	* NULL by default.)
	*/
	void Logger::custom_prefix(
		std::ostream &sin, const google::LogMessageInfo &l, void*) {
		sin << std::setw(4) << 1900 + l.time.year() << '-'
			<< std::setw(2) << 1 + l.time.month() << '-'
			<< std::setw(2) << l.time.day() << ' '
			<< std::setw(2) << l.time.hour() << ':'
			<< std::setw(2) << (l.time.min)()  << ':'
			<< std::setw(2) << l.time.sec() << "."
			<< std::setw(6) << l.time.usec()
			<< " [" << l.severity << "]|thread_id="
			<< std::setfill(' ') << std::setw(5)
			<< l.thread_id << std::setfill('0') << "|locate="
			<< l.filename << ':' << l.line_number;
	}

	bool Logger::init(std::string strLogPath, std::string strLogName, int nOffsetSec) {
		if (m_bInit) return true;
		m_strLogPath = strLogPath;
		m_strLogName = strLogName;
		m_nOffsetSec = nOffsetSec;
		// Flags
		// FLAGS_log_prefix = false; // 原有的格式完全采用自己的格式
#ifdef _DEBUG
		FLAGS_logtostderr = 1;  //输出到控制台
#endif
		FLAGS_logtostderr = 0;
		FLAGS_alsologtostderr = 0;
		FLAGS_timestamp_in_logfile_name = 0; // 文件名里使用自己定义时间戳（废弃默认的）
		// 初始化日志
		google::InitGoogleLogging(strLogName.c_str(), &custom_prefix);
		google::SetStderrLogging(3);
		// FLAGS_log_dir = "./logs";
		google::InstallFailureSignalHandler(); // 安装信号处理程序，当程序出现崩溃时，会输出崩溃的位置等相关信息。
		update_rotation_time();	
		m_bInit = true;
		return m_bInit;
	}

	void Logger::update_rotation_time() {
		char buffer[30];
		std::time_t now = date::to_time_t(
			date::now() + std::chrono::seconds(m_nOffsetSec));
		std::strftime(buffer, sizeof(buffer), "%Y%m%d",
			std::localtime(&now));
		std::string cur_time(buffer);
		if (cur_time == rotation_date)
			return;
		rotation_date = cur_time;
		util::mkdirs(m_strLogPath, true);
		std::string filename = m_strLogPath + "/" + m_strLogName + "." + rotation_date + ".log";
#ifdef _WIN32
		google::SetLogDestination(google::GLOG_INFO, filename.c_str());
#else
		google::SetLogDestination(google::INFO, filename.c_str());
#endif	
	}

	void Logger::stop() {
		log_fatal << "logger stopped!";
		m_bInit = false;
	}
}