// #include "stdafx.h"

#include "base.h"
#include "util/config.h"
#include "data/numcpp.hpp"
#include "backtest/engine.h"
#include "manager/instrument.h"

namespace stg {

StgBase::StgBase(const std::string& name, const std::string& contract)
	:timestamp(-1), marketposition(0) {
	this->stg_name = name;
	this->_initialized = false;
	this->instrument = contract;
	if (mgr::Config::Get()->get("mode") == "trade")
		this->spi_name = mgr::Config::Get()->get(stg_name + "::spi_name");
}

bool StgBase::init() {
	if (!parse_args()) {
		return false;
	}
	if (!spi_name.empty()) { // online init with history
		bt::BackTestEngine engine;
		engine.init();
		engine.run(this);
	}
	return true;
}

StgBase::~StgBase() {}

}
