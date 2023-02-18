#ifndef __DAT_TYPES_H__
#define __DAT_TYPES_H__

#include <string>
#include "quote/marketdata.h"

namespace dat {

class TickData {
public:
	float price;
	double timestamp;
	int update_millisec;
	std::string trading_day;
	std::string update_time;
	std::string instrument_id;
public:
	TickData();
	TickData(const std::string& content);
	TickData(CThostFtdcDepthMarketDataField* p);

	void reset(const std::string& content);
	void reset(CThostFtdcDepthMarketDataField* p);

};

class Renko {
public:
	bool m_bInit;
	float open, close, high, low, span, scale;
public:
	Renko();
	Renko(float scale);
	~Renko();
public:
	int update(float price, int &);
	void reset(float scale = -1) noexcept;
};

}



#endif // __DAT_TYPES_H__