#ifndef __STG_INTERVAL_TRADE_H__
#define __STG_INDERVAL_TRADE_H__


#include "data/structs.h"
#include "trade/trader.h"
#include "strategy/base.h"


namespace stg {
// namespace of strategy
class IntervalTrade : public StgBase {
public:
	~IntervalTrade() {}
	IntervalTrade(const std::string& name, const std::string& instrument);
public:
	// @override
	virtual bool parse_args() override;
	virtual int update(const dat::TickData&) override;

	// @trade gateway
	bool trade_allowed();
private:
	float enpp;
	std::string header;
	std::vector<std::string> columns;
	int interval, count, marketposition;
};
}

#

#endif __STG_INDERVAL_TRADE_H__