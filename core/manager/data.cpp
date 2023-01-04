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

	dat::TickData* DataManager::update(CThostFtdcDepthMarketDataField* p) {
		dat::TickData* ptd(nullptr);
		auto it = _data.find(p->InstrumentID);
		if(it != _data.end()){
			ptd = it->second;
			ptd->reset(p);
		}
		else {
			ptd = new dat::TickData(p);
			_data[p->InstrumentID] = ptd;
		}
		return ptd; 
	}

	dat::TickData* DataManager::get(const std::string& instrument_id) {
		auto it = _data.find(instrument_id);
		if (it != _data.end()) {
			return it->second;
		}
		else {
			return nullptr;
		}
	}

	DataManager* DataManager::Get() {
		if (_instance == nullptr) {
			static DataManager ins;
			_instance = &ins;
		}
		return _instance;
	}
}