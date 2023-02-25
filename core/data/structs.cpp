// #include "stdafx.h"
#include "time.h"
#include "structs.h"
#include "util/common.h"
#include <iostream>

namespace dat {
// data struct name space
/// Tickdata

TickData::TickData() {
	price = 0;
	timestamp = -1;
	update_millisec = 0;
}

TickData::TickData(const std::string& content) {
	reset(content);
}

void TickData::reset(const std::string& content) {
	int i(1), j(1), count(0);
	for (; i < content.length(); i++) {
		if (content[i] == ',') {
			count += 1;
			switch (count) {
			case 1: // instrument_id=IC2108
				instrument_id = content.substr(j, i-j);
				break;
			case 2: //ActionDay=20210705
				break;
			case 3: //AskPrice1=6630
				break;
			case 4: //AskPrice2=1.79769e+308
				break;
			case 5: //AskPrice3=1.79769e+308
				break;
			case 6: //AskPrice4=1.79769e+308
				break;
			case 7: //AskPrice5=1.79769e+308
				break;
			case 8: //AskVolume1=1
				break;
			case 9: //AskVolume2=0
				break;
			case 10: //AskVolume3=0
				break;
			case 11: //AskVolume4=0
				break;
			case 12: //AskVolume5=0
				break;
			case 13: //AveragePrice=1.32391e+06
				break;
			case 14: //BidPrice1=6624
				break;
			case 15: //BidPrice2=1.79769e+308
				break;
			case 16: //BidPrice3=1.79769e+308
				break;
			case 17: //BidPrice4=1.79769e+308
				break;
			case 18: //BidPrice5=1.79769e+308
				break;
			case 19: //BidVolume1=1
				break;
			case 20: //BidVolume2=0
				break;
			case 21: //BidVolume3=0
				break;
			case 22: //BidVolume4=0
				break;
			case 23: //BidVolume5=0
				break;
			case 24: //ClosePrice=6627.4
				break;
			case 25: //CurrDelta=1.79769e+308
				break;
			case 26: //ExchangeID=
				break;
			case 27: //ExchangeInstID=
				break;
			case 28: //HighestPrice=6647.2
				break;
			case 29: //LastPrice=6627.4
				price = atof(content.substr(j, i - j).c_str());
				break;
			case 30: //LowerLimitPrice=5932
				break;
			case 31: //LowestPrice=6586
				break;
			case 32: //OpenInterest=7502
				break;
			case 33: //OpenPrice=6606
				break;
			case 34: //PreClosePrice=6591.4
				break;
			case 35: //PreDelta=0
				break;
			case 36: //PreOpenInterest=6902
				break;
			case 37: //PreSettlementPrice=6591
				break;
			case 38: //SettlementPrice=6625.2
				break;
			case 39: //TradingDay=20210705
				trading_day = content.substr(j, i - j);
				break;
			case 40: //Turnover=4.81374e+09
				break;
			case 41: //UpdateMillisec=500
				update_millisec = atoi(content.substr(j, i - j).c_str());
				break;
			case 42: //UpdateTime=16:02:36
				update_time = content.substr(j, i - j).c_str();
				break;
			case 43: //UpperLimitPrice=7250
				break;
			case 44: //Volume=3636
				break;
			}
			j = i + 1;
		}
	}
	timestamp = util::to_timestamp(trading_day, update_time, update_millisec);
}

TickData::TickData(const CThostFtdcDepthMarketDataField* p) {
	reset(p);
}

void TickData::reset(const CThostFtdcDepthMarketDataField* p){
	price = p->LastPrice;
	double eps = abs(p->LastPrice - price);
	/* ================================================================================
	������Ҫע��ģ�
		1��ActionDay��TradingDay�ĺ������ȯ���в��죺https://zhuanlan.zhihu.com/p/33553653
		2������TradingDay�ĺ��������ȷ��������ѡTradingDay
	================================================================================== */
	trading_day = p->TradingDay;
	update_millisec = p->UpdateMillisec;
	update_time = p->UpdateTime;
	timestamp = util::to_timestamp(p->TradingDay, p->UpdateTime, p->UpdateMillisec);
	instrument_id = p->InstrumentID;
}

/// Renko

Renko::Renko() {
	this->reset(0);
}

Renko::Renko(float scale){
	this->reset(scale);
}

Renko::~Renko() {}

int Renko::update(float price, int& direction) {
	int count = 0;
	if (m_bInit) {
		if (price >= high + span) {
			direction = 1;
			count = (price - high) / span;
			high = close = high + count * span;
			low = open = close - span;
		}
		if (price <= low - span) {
			direction = -1;
			count = (low - price) / span;
			low = close = low - count * span;
			high = open = close + span;
		}
	}
	else {
		open = close = high = low = price;
		span = price * scale;
		m_bInit = true;
	}
	return count;
}

void Renko::reset(float scale) noexcept{
	if (scale >= 0) // use new scale
		this->scale = scale;
	span = open = close = high = low = 0;
	m_bInit = false;
}

}
