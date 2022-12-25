#include "util/config.h"
#include "trade/trader.h"
#include "strategy/interval_trade.h"

namespace stg {

IntervalTrade::IntervalTrade(const std::string& name, const std::string& instrument)
	:StgBase(name, instrument)
	,interval(1), count(0)
	,marketposition(0) ,enpp(0)
	,header("open,high,low,close,pos"){

	columns = util::split(header, ",");
	log_info << "header=" << header;
} 

bool IntervalTrade::trade_allowed() {
	return true;
}

bool IntervalTrade::parse_args() {
	try {
		auto args = mgr::Config::Get()->get_section(stg_name);
		interval = util::parse_argument(args, "interval", interval);
		log_info << stg_name << "update with: interval=" << interval;
	}
	catch (const std::exception& e) {
		log_error << stg_name << " parsing arguments failed, reason:" << e.what();
		return false;
	}
	return true;
}

int IntervalTrade::update(const dat::TickData& td) {
	if (trading_day != td.trading_day) { // new trading day
		trading_day = td.trading_day;
		count = 0;
	}
	count++;
	if (count % interval != 0)
		return 0;
	if (!spi_name.empty()) { // online
		// online trade
		ord::Direction direction = ord::Direction::_MIN;
		double minmove = mgr::InstrumentManager::Get()->get(instrument)->PriceTick;
		double price = int(td.price / minmove + 0.5) * minmove;
		if (marketposition == 0 ) { // buy
			direction = ord::Direction::BUY;
			mgr::TraderManager::Get()->trade(
				spi_name, instrument, false, price, 1, // marketposition - last_position,
				direction, ord::TradeType::ORDINARY);
			marketposition = 1;
		}
		else if (marketposition == 1) { // sell
			direction = ord::Direction::SELL;
			mgr::TraderManager::Get()->trade(
				spi_name, instrument, false, price, 1,
				direction, ord::TradeType::ORDINARY);
			marketposition = 2;
		}
		else if (marketposition == 2) { // sellshort
			direction = ord::Direction::SELLSHORT;
			mgr::TraderManager::Get()->trade(
				spi_name, instrument, false, price, 1,
				direction, ord::TradeType::ORDINARY);
			marketposition = 3;
		}
		else if (marketposition == 3) { // buytocover
			direction = ord::Direction::BUYTOCOVER;
			mgr::TraderManager::Get()->trade(
				spi_name, instrument, false, price, 1, 
				direction, ord::TradeType::ORDINARY);
			marketposition = 0;
		}
		else {
			log_error << "invalid marketposition got: " << marketposition;
		}
	}
	return 0;
}

}