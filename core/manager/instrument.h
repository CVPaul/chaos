#ifndef __CTP_INSTRUMENT_MANAGER_H__
#define __CTP_INSTRUMENT_MANAGER_H__

#include <map>
#include <string>
#include <unordered_map>

#include "manager/instrument.h"
#include "ctp/ThostFtdcUserApiStruct.h"
#include "ctp/ThostFtdcUserApiDataType.h"

namespace mgr {
class InstrumentManager {
private:
	std::unordered_map<std::string, std::string> dominants;
	std::unordered_map<std::string, CThostFtdcInstrumentField*> instruments;
public:
	static InstrumentManager* instance;
private:
	InstrumentManager() {}
public:
	static InstrumentManager* Get();
	~InstrumentManager();
	inline void append(CThostFtdcInstrumentField* pInstrument) {
		// no lock needed now
		instruments[pInstrument->InstrumentID] = pInstrument;
		instruments[to_symbol(pInstrument->InstrumentID)] = pInstrument;
	}
	inline CThostFtdcInstrumentField* get(const std::string& instrument_id) {
		// no lock needed now
		auto iter = instruments.find(instrument_id);
		if (iter == instruments.end()) {
			auto symbol = to_symbol(instrument_id);
			auto it2 = instruments.find(symbol);
			if (it2 == instruments.end()){
				return nullptr;
			}
			return it2->second;
		}
		return iter->second;
	}
	// instrument to symbol
	std::string to_symbol(const std::string& instrument_id);
	// instrument/symbol to exchange_id
	std::string get_exhcnage_id(const std::string& instrument);

	// contract operators
	bool is_dce(const std::string& instrument_id);
	bool is_czce(const std::string& instrument_id);
	bool is_shfe(const std::string& instrument_id);
	bool is_finance(const std::string& instrument_id);
	bool is_option(const std::string& instrument_id);
	bool is_arbitrage(const std::string& instrument_id);

	inline bool is_valid(const std::string& instrument_id) {
		return instruments.find(instrument_id) != instruments.end();
	}

	void all(std::vector<std::string>& instruments); // return all instruments
	void option(std::vector<std::string>& instruments); // return all option instruments
	void finance(std::vector<std::string>& instruments); // return all finance instruments
	void commodity(std::vector<std::string>& instruments); // return all commodity instruments
	void arbitrage(std::vector<std::string>& instruments); // return all arbitrage  instruments

	void all_contracts(
		const std::string& symbol,
		std::vector<std::string>& instruments); // return all instruments of a symbol
	std::vector<std::string> all_contracts(const std::string& symbol);
};
}

#endif // __CTP_MANAGER_INSTRUMENT_H__