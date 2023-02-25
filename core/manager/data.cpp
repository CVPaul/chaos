#include <iostream>
#include "manager/data.h"

namespace mgr {
	DataManager* DataManager::_instance = nullptr;

	void DataManager::reset() {
		for (auto &[_, val]: _data) {
			if (val) delete val;
		}
		_data.clear();
	}

	DataManager::~DataManager() {
		reset();
	}

	dat::TickData DataManager::update(const CThostFtdcDepthMarketDataField *  p) {
		WriteLock wlock(m_lock);
		dat::TickData* ptd = nullptr;
		auto it = _data.find(p->InstrumentID);
		if(it != _data.end()){
			ptd = it->second;
			ptd->reset(p);
		}
		else{
			ptd = new dat::TickData(p);
			_data[p->InstrumentID] = ptd;
		}
		return *ptd; 
	}

	float DataManager::get_price(const std::string& instrument_id) {
		ReadLock rlock(m_lock);
		return _data[instrument_id]->price;
	}

	DataManager* DataManager::Get() {
		if (_instance == nullptr) {
			static DataManager ins;
			_instance = &ins;
		}
		return _instance;
	}
}