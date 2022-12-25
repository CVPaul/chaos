#include "stdafx.h"

#include <set>

#include "util/logging.h"
#include "manager/instrument.h"

namespace mgr {
InstrumentManager* InstrumentManager::instance = nullptr;

InstrumentManager::~InstrumentManager() {
	for (auto&& v : instruments) {
		if (v.second) {
			delete v.second;
			v.second = nullptr;
		}
	}
	instruments.clear();
}

InstrumentManager* InstrumentManager::Get() {
	if (instance == nullptr) {
		static InstrumentManager ins;
		instance = &ins;
	}
	return instance;
}

std::string InstrumentManager::to_symbol(
	const std::string& instrument_id) {
	int i, j(0);
	const int buf_size = 8;
	char buf[buf_size];
	for (i = 0; i < instrument_id.length(); i++) {
		if (instrument_id[i] >= '0' && instrument_id[i] <= '9')
			continue;
		if (j >= buf_size) {
			log_error << "size exceeded limit(" << buf_size
				<< ") while convert " << instrument_id << " to symbol";
			return "";
		}
		buf[j++] = instrument_id[i];
	}
	buf[j] = '\0';
	return buf;
}


std::string InstrumentManager::get_exhcnage_id(const std::string& instrument) {
	auto iter = instruments.find(instrument);
	if (iter != instruments.end()) {
		return iter->second->ExchangeID;
	}
	return "";
}

	bool InstrumentManager::is_finance(const std::string& instrument_id) {
	auto iter = instruments.find(instrument_id);
	if (iter == instruments.end()) {
		return false;
	}
	if (strcmp(iter->second->ExchangeID, "CFFEX") == 0) {
		return true;
	}
	return false;
}

bool InstrumentManager::is_shfe(const std::string& instrument_id) {
	auto iter = instruments.find(instrument_id);
	if (iter == instruments.end()) {
		return false;
	}
	if (strcmp(iter->second->ExchangeID, "SHFE") == 0) {
		return true;
	}
	return false;
}

bool InstrumentManager::is_czce(const std::string& instrument_id) {
	auto iter = instruments.find(instrument_id);
	if (iter == instruments.end()) {
		return false;
	}
	if (strcmp(iter->second->ExchangeID, "CZCE") == 0) {
		return true;
	}
	return false;
}

bool InstrumentManager::is_dce(const std::string& instrument_id) {
	auto iter = instruments.find(instrument_id);
	if (iter == instruments.end()) {
		return false;
	}
	if (strcmp(iter->second->ExchangeID, "DCE") == 0) {
		return true;
	}
	return false;
}

bool InstrumentManager::is_option(const std::string& instrument_id) {
	if (instrument_id.length() > 6) {
		auto iter = instruments.find(instrument_id);
		if (iter == instruments.end()) {
			return false;
		}
		for (int i = 4; i < instrument_id.length(); i++) {
			// least symbol len is 1, least date len is 3, so start from 4
			if (instrument_id[i] == 'C' || instrument_id[i] == 'P')
				return true;
		}
	}
	return false;
}

bool InstrumentManager::is_arbitrage(const std::string& instrument_id) {
	return false;
}

void InstrumentManager::all(std::vector<std::string>& targets) {
	for (auto&& ins : instruments) {
		targets.push_back(ins.first);
	}
}

void InstrumentManager::option(std::vector<std::string>& targets) {
	for (auto&& ins : instruments) {
		if (is_option(ins.first)) {
			targets.push_back(ins.first);
		}
	}
}

void InstrumentManager::finance(std::vector<std::string>& targets) {
	for (auto&& ins : instruments) {
		if (is_finance(ins.first)) {
			targets.push_back(ins.first);
		}
	}
}

void InstrumentManager::commodity(std::vector<std::string>& targets) {
	for (auto&& ins : instruments) {
		if (is_option(ins.first) || is_finance(ins.first) || is_arbitrage(ins.first))
			continue;
		targets.push_back(ins.first);
	}
}

void InstrumentManager::arbitrage(std::vector<std::string>& targets) {
	for (auto&& ins : instruments) {
		if (is_arbitrage(ins.first)) {
			targets.push_back(ins.first);
		}
	}
}


void InstrumentManager::all_contracts(
	const std::string& symbol, std::vector<std::string>& targets) {
	// return all instruments of a symbol
	for (auto&& ins : instruments) {
		if (to_symbol(ins.first) == symbol) {
			targets.push_back(ins.first);
		}
	}
}

std::vector<std::string> InstrumentManager::all_contracts(
	const std::string& symbol) {
	// return all instruments of a symbol
	std::vector<std::string> res;
	all_contracts(symbol, res);
	return res;
}

}