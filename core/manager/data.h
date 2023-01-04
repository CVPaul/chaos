#pragma once

#include <string>
#include <atomic>
#include <unordered_map>
#include "data/structs.h"
#include "ctp/ThostFtdcMdApi.h"


namespace mgr {
	class DataManager {
	private:
		DataManager() {};
	public:
		void reset();
		dat::TickData* get(const std::string&);
		dat::TickData* update(CThostFtdcDepthMarketDataField* p);
	public:
		~DataManager();
		static DataManager* Get();
	private:
		static DataManager* _instance;
		std::unordered_map <std::string, std::atomic<dat::TickData*>> _data;
	};
}