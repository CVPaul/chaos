#ifndef __STG_BASE_H__
#define __STG_BASE_H__

#include <string>
#include "util/common.h"
#include "data/structs.h"
#include "trade/trader.h"
#include "data/numcpp.hpp"

namespace stg {
class StgBase {
public:
	virtual ~StgBase();
	StgBase(const std::string& name, const std::string& contract);
private:
	std::atomic_bool _initialized;
public:
	int timestamp;
	int marketposition;
	std::string stg_name;
	std::string spi_name;
	std::string instrument;
	std::string trading_day;
public:
	bool init();
	virtual bool parse_args() { return true; };
	virtual int update(const dat::TickData&) = 0;
};
}

#endif // __STG_BASE_H__