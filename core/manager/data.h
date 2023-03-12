#pragma once

#include <map>
#include <mutex>
#include <string>
#include <atomic>
#include <shared_mutex>
#include <unordered_map>
#include "data/structs.h"
#include "io/writter.hpp"
#include "ctp/ThostFtdcMdApi.h"

typedef std::shared_mutex Lock;
typedef std::shared_lock<Lock> ReadLock;
typedef std::unique_lock<Lock> WriteLock;

namespace mgr {
	class DataManager {
	private:
		DataManager() {};
	public:
		void reset();
		float get_price(const std::string&);
		dat::TickData update(const CThostFtdcDepthMarketDataField * p);
		void add_writter(const std::string& symbol, const std::string& date);
	public:
		~DataManager();
		static DataManager* Get();
	private:
		Lock m_lock;
		static DataManager* _instance;
		std::unordered_map <std::string, dat::TickData*> _data;
		std::map<std::string, io::AsyncWritter<CThostFtdcDepthMarketDataField>*> _writter;
	};
}
