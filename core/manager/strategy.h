#ifndef __STRATEGY_MANAGER_H__
#define __STRATEGY_MANAGER_H__

#include <list>
#include <string>
#include <unordered_map>

#include "trade/trader.h"
#include "data/structs.h"
#include "strategy/base.h"
#include "quote/marketdata.h"

namespace mgr {
class StrategyManager {

private:
	std::unordered_map<std::string, std::list<stg::StgBase*>> strategies;
public:
	static StrategyManager* instance;
private:
	StrategyManager() {}
public:
	static StrategyManager* Get();
	~StrategyManager();
public:
	int update(const dat::TickData&);
	int add(const std::string& stg_name, const std::string& instrument);
};
}
#endif // __STRATEGY_MANAGER_H__namespace ctp {