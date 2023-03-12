#include <iostream>
#include "manager/data.h"
#include "manager/instrument.h"

namespace mgr {
	DataManager* DataManager::_instance = nullptr;

	void DataManager::reset() {
		for (auto &[_, val]: _data) {
			if (val) delete val;
		}
		_data.clear();
		for (auto &[_, val]: _writter){
			if (val) delete val;
		}
		_writter.clear();
	}

	DataManager::~DataManager() {
		reset();
	}

	void DataManager::add_writter(const std::string& symbol, const std::string& date){
		std::string filename = "output/data/" + symbol + "." + date +".bin";
		auto iter = _writter.find(symbol);
		if (iter == _writter.end()){
			_writter[symbol] = new io::AsyncWritter<CThostFtdcDepthMarketDataField>(
				filename);
		}else if(iter->second->get_filename() != filename){
			delete iter->second;
			iter->second = new io::AsyncWritter<CThostFtdcDepthMarketDataField>(
				filename);
		}
	}

	dat::TickData DataManager::update(const CThostFtdcDepthMarketDataField* p) {
		auto iter = _writter.find("futures");
		if (iter != _writter.end()){
			iter->second->write(*p);
		}
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