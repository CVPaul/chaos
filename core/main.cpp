// #include "stdafx.h"

#include <ctime>
#include <iostream>
#include <exception>

#include "util/config.h"
#include "util/logging.h"
#include "trade/trader.h"
#include "broker/front.h"
#include "manager/data.h"
#include "quote/marketdata.h"
#include "manager/strategy.h"
#include "datetime/datetime.h"

#include "backtest/engine.h"

// about switch on debug mode: https://blog.csdn.net/u010797208/article/details/40452797

int trade(){
	// init broker
	bool is_first_broker = true;
	auto brokers = util::split(mgr::Config::Get()->get("brokers"), ",");
	for (auto&& n : brokers) {
		ctp::BrokerInfo* pBroker = mgr::BrokerManager::Get()->add(n);
		// register trade api
		CThostFtdcTraderApi* pUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi("output");
		log_info << "api version of ctp:" << pUserApi->GetApiVersion();
		ctp::CTdSpi* pTdSpi = new ctp::CTdSpi(pUserApi, pBroker);
		pUserApi->RegisterSpi(pTdSpi);
		pUserApi->RegisterFront(const_cast<char*>(pBroker->td_front.c_str()));
		pUserApi->SubscribePrivateTopic(THOST_TERT_QUICK);
		pUserApi->SubscribePublicTopic(THOST_TERT_QUICK);
		if (is_first_broker) {
			// register market data pull api 
			CThostFtdcMdApi* pUserMdApi =
				CThostFtdcMdApi::CreateFtdcMdApi("output");
			ctp::CMdSpi* pMdSpi = new ctp::CMdSpi(pUserMdApi);
			pUserMdApi->RegisterSpi(pMdSpi);
			pUserMdApi->RegisterFront(const_cast<char*>(pBroker->md_front.c_str()));
			mgr::TraderManager::Get()->add(pMdSpi);
			is_first_broker = false;
			if (mgr::Config::Get()->get("download_data") == "true"){
				// create the asyn task to download data
				mgr::DataManager::Get()->add_writter(
					"futures", dt::now().to_string("%Y%m%d"));
			}
		}
		// add trader
		mgr::TraderManager::Get()->add(n, pTdSpi);
	}
	// register the strategy 
	for (auto&& n : util::split(mgr::Config::Get()->get("strategies"), ",")) {
		mgr::StrategyManager::Get()->add(
			n, mgr::Config::Get()->get(n + "::instrument"));
	}
	// main body
	try {
		// init
		bool is_new_session = true;
		auto pTdSpi = mgr::TraderManager::Get()->get(brokers[0]);
		auto pMdSpi = mgr::TraderManager::Get()->get_mdspi();
		while (1) {
			std::time_t t = std::time(0);
			std::tm* ptm = std::localtime(&t);
			int HMS = ptm->tm_hour * 10000 + ptm->tm_min * 100 + ptm->tm_sec;
#ifdef SIMULATE
			if(1){
#else
			if ((HMS >= 85000 && HMS < 153000) ||
				(HMS >= 205000 && HMS < 240000) || (HMS >= 0 && HMS < 30000)) {
#endif // Simulate
				if (is_new_session) {
					// login with trader
					if (!pTdSpi->had_connected) { 
						pTdSpi->Init();
						WaitForSingleObject(g_hEvent, INFINITE);
					}
					pTdSpi->ReqAuthenticate();
					WaitForSingleObject(g_hEvent, INFINITE);
					pTdSpi->ReqUserLogin();
					WaitForSingleObject(g_hEvent, INFINITE);
					// subscribe data
					if (!pMdSpi->had_connected) {
						pMdSpi->Init();
						WaitForSingleObject(xinhao, INFINITE);
					}
					pMdSpi->ReqUserLogin();
					WaitForSingleObject(xinhao, INFINITE);
					pTdSpi->ReqQryInstrument("", "");//????????????????
					WaitForSingleObject(xinhao, INFINITE);
					pMdSpi->SubscribeMarketData(mgr::Config::Get()->get("instruments"));//????????????????????????
					// set false
					is_new_session = false;
				}
				// mgr::DataManager::Get()->reset(); // ???????????????????????????????
				util::Logger::update_rotation_time(); // rolling log file
				std::this_thread::sleep_for(std::chrono::seconds(5));
			}
			else {
				if (HMS < 85000) {
					ptm->tm_hour = 8;
					ptm->tm_min = 50;
					ptm->tm_sec = 0;
					if (mgr::Config::Get()->get("download_data") == "true"){
						// create the async task to download data
						mgr::DataManager::Get()->add_writter(
							"futures", dt::now().to_string("%Y%m%d"));
					}
				}
				else{
					ptm->tm_hour = 20;
					ptm->tm_min = 50;
					ptm->tm_sec = 0;
				}
				std::this_thread::sleep_until(
					std::chrono::system_clock::from_time_t(std::mktime(ptm)));
				log_info << "wakeup!!!" << ", md_connected:" << pMdSpi->had_connected
					<< ", td_connected:" << pMdSpi->had_connected;
				is_new_session = true;
			}
			// set public and debug with this
			// pTdSpi->ReqOrderInsert("rb2205", 4394, 4, ord::Direction::BUY);
		}
	}
	catch (std::exception& e) {
		log_error << "front exited with:" << e.what();
	}
	// notify
	log_info << "processor exited after join!";
	return 0;
}

int backtest(){
	// register the strategy
	for (auto&& n : util::split(mgr::Config::Get()->get("strategies"), ",")) {
		std::string instrument = mgr::Config::Get()->get(n + "::instrument");
		mgr::StrategyManager::Get()->add(n, instrument);
	}
	// main body
	bt::BackTestEngine engine;
	engine.init();
	engine.run();
	// notify
	log_info << "backtest finished";
	return 0;
}

int main(int argc, char *argv[]) {
	util::mkdirs("orderbooks", true);
	// logger init
	if (!util::Logger::init("output/logs", "chaos", 4*3600)) {
		std::cerr << "init logger failed!" << std::endl;
		return -1;
	}else{
		log_info << "logger init succeeded!!!" << std::endl;
	}
	// config init
	if (!mgr::Config::Get()->init("config/chaos.ini")) {
		log_error << "load config/chaos.ini failed";
		return -2;
	}
	else {
		log_info << "load config/chaos.ini succeeded!";
	}
	std::string mode = mgr::Config::Get()->get("mode");
	if (mode == "trade")
		return trade();
	else if (mode == "backtest")
		return backtest();
	else
		log_fatal << "unknown mode:" << mode;
}

