#include "strategy.h"
#include "util/config.h"
#include "strategy/renko_break.h"
#include "strategy/interval_trade.h"

namespace mgr {
// namespace for managers
StrategyManager* StrategyManager::instance = nullptr;

StrategyManager::~StrategyManager() {
	for (auto&& s : strategies) {
		for (auto&& v : s.second) {
			if (v) {
				delete v;
				v = nullptr;
			}
		}
	}
	strategies.clear();
}

StrategyManager* StrategyManager::Get() {
	if (instance == nullptr) {
		static StrategyManager ins;
		instance = &ins;
	}
	return instance;
}

int StrategyManager::add(
	const std::string& stg_name, const std::string& instrument) {
	// function body
	auto stg_name_ = util::split(stg_name, "\\\.");
	stg::StgBase* pStg = nullptr;
	if (stg_name_[0] == "renko_break"){
		pStg = new stg::RenkoBreak(stg_name, instrument);
		if (!pStg->init()) {
			delete pStg; pStg = nullptr;
		}
	}
	else if(stg_name_[0] == "interval_trade") {
		pStg = new stg::IntervalTrade(stg_name, instrument);
		if (!pStg->init()) {
			delete pStg; pStg = nullptr;
		}
	}
	if (pStg != nullptr) {
		strategies[instrument].push_back(pStg);
		return 0;
	}
	return -1;
}

int StrategyManager::update(const dat::TickData& td) {
	auto iter = strategies.find(td.instrument_id);
	if (iter == strategies.end())
		return 0; // nothing
	for (auto&& s : iter->second) {
		s->update(td);
	}
	return 0;
}
}