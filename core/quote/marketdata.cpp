// #include "stdafx.h"

#include <set>
#include <regex>

#include "util/config.h"
#include "util/common.h"
#include "util/logging.h"
#include "data/structs.h"
#include "manager/data.h"
#include "quote/marketdata.h"
#include "manager/strategy.h"
#include "manager/instrument.h"
#include "datetime/datetime.h"

HANDLE xinhao = CreateEvent(NULL, false, false, NULL);

// 构造函数，需要一个有效的指向CThostFtdcMduserApi实例的指针
ctp::CMdSpi::CMdSpi(CThostFtdcMdApi *pUserApi) 
	: m_pUserMdApi(pUserApi) {
	had_connected = false;
	had_logged_in = false;
	need_download = mgr::Config::Get()->get("download_data") == "true";
}

ctp::CMdSpi::~CMdSpi() {
	m_pUserMdApi->Release();
}

// 当客户端与交易托管系统建立起通信连接，客户端需要进行登录
void ctp::CMdSpi::OnFrontConnected(){
	/*strcpy_s(broker.broker_id.c_str(), getConfig("config", "BrokerID").c_str());
	strcpy_s(investor_id.c_str(), getConfig("config", "UserID").c_str());
	strcpy_s(password.c_str(), getConfig("config", "Password").c_str());*/
	log_info << "md_front connnected";
	had_connected = true;
	SetEvent(xinhao);
}

void ctp::CMdSpi::ReqUserLogin(){
	CThostFtdcReqUserLoginField reqUserLogin;
	int num = m_pUserMdApi->ReqUserLogin(&reqUserLogin, 0);
}

// 当客户端与交易托管系统通信连接断开时，该方法被调用
void ctp::CMdSpi::OnFrontDisconnected(int nReason){
	// 当发生这个情况后，API会自动重新连接，客户端可不做处理
	had_connected = false;
	log_error << "md_front disconnected with reason:" << nReason;
}

// 当客户端发出登录请求之后，该方法会被调用，通知客户端登录是否成功
void ctp::CMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
	CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast){
	if (pRspInfo && pRspInfo->ErrorID != 0) {
		// 端登失败，客户端需进行错误处理
		log_error << "login failed with:" << pRspInfo->ErrorID
			<< pRspInfo->ErrorMsg << request_id << bIsLast;
	}
	else {
		had_logged_in = true;
		log_info << "login succeeded with code:" << pRspInfo->ErrorID
			<< ", msg:" << pRspInfo->ErrorMsg << ", req_id:"
			<< request_id << ", is_last:" << bIsLast;
	}
	SetEvent(xinhao);
	//SubscribeMarketData();//订阅行情
	//SubscribeForQuoteRsp();//询价请求
}

void ctp::CMdSpi::SubscribeMarketData(const std::string& instruments){//收行情
	int md_num = 0;
	char *ppInstrumentID[500];
	std::vector<std::string> instrument_ids;
	std::vector<std::string> tokens = util::split(instruments, ",");
	for (auto&& token : tokens) {
		std::string sub = util::trim_copy(token);
		if (sub == "all")
			mgr::InstrumentManager::Get()->all(instrument_ids);
		else if (sub == "finance")
			mgr::InstrumentManager::Get()->finance(instrument_ids);
		else if (sub == "commodity")
			mgr::InstrumentManager::Get()->commodity(instrument_ids);
		else if (sub == "arbitrage")
			mgr::InstrumentManager::Get()->arbitrage(instrument_ids);
		else if (sub.find("symbol:") != std::string::npos) {
			std::vector<std::string> symbols = util::split(sub.substr(7), "\\|");
			for (auto&& s : symbols) {
				mgr::InstrumentManager::Get()->all_contracts(s, instrument_ids);
			}
		}
		else {
			if (mgr::InstrumentManager::Get()->is_valid(sub)) {
				instrument_ids.push_back(sub);
			}
			else {
				log_error << "invalid instrument:" << sub << " to subscribe";
			}
		}
	}
	for (int count1 = 0; count1 <= instrument_ids.size() / 500; count1++){
		if (count1 < instrument_ids.size() / 500){
			int a = 0;
			for (a; a < 500; a++){
				ppInstrumentID[a] = const_cast<char *>(instrument_ids.at(md_num).c_str());
				md_num++;
			}
			int result = m_pUserMdApi->SubscribeMarketData(ppInstrumentID, a);
		}
		else if (count1 == instrument_ids.size() / 500){
			int count2 = 0;
			for (count2; count2 < instrument_ids.size() % 500; count2++){
				ppInstrumentID[count2] = const_cast<char *>(instrument_ids.at(md_num).c_str());
				md_num++;
			}
			if (count2 > 0) {
				int result = m_pUserMdApi->SubscribeMarketData(
					ppInstrumentID, count2);
			}
		}
	}
}

///订阅行情应答
void ctp::CMdSpi::OnRspSubMarketData(
	CThostFtdcSpecificInstrumentField *pSpecificInstrument,
	CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast){
	if (pRspInfo && pRspInfo->ErrorID){
		log_info << "resonse of subscribe instrument_id:" << pSpecificInstrument->InstrumentID
			<< ", code:" << pRspInfo->ErrorID << ", msg:" << pRspInfo->ErrorMsg;
	}
}

///深度行情通知
void ctp::CMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData){
	if (pDepthMarketData){
#ifdef SIMULATE
		if (pDepthMarketData->LastPrice < 0 or pDepthMarketData->LastPrice > 1e8){
			log_error << "origin data price is invalid:" << pDepthMarketData->InstrumentID;
		}
		if(pDepthMarketData->UpdateTime[0] == '\0'){
			log_warning << "Invalid data got update_time is '\\0', instrument_id:" << pDepthMarketData->InstrumentID;
			strcpy_s(pDepthMarketData->UpdateTime, dt::now().to_string("%H:%M:%S").c_str());
		}
#endif // simulate
		dat::TickData td = mgr::DataManager::Get()->update(pDepthMarketData);
		mgr::StrategyManager::Get()->update(td);
		if (need_download){
			log_info << "PUBLIC_MARKET_DATA|instrument_id=" << pDepthMarketData->InstrumentID
				<< "|ActionDay=" << pDepthMarketData->ActionDay
				<< "|AskPrice1=" << pDepthMarketData->AskPrice1
				<< "|AskPrice2=" << pDepthMarketData->AskPrice2
				<< "|AskPrice3=" << pDepthMarketData->AskPrice3
				<< "|AskPrice4=" << pDepthMarketData->AskPrice4
				<< "|AskPrice5=" << pDepthMarketData->AskPrice5
				<< "|AskVolume1=" << pDepthMarketData->AskVolume1
				<< "|AskVolume2=" << pDepthMarketData->AskVolume2
				<< "|AskVolume3=" << pDepthMarketData->AskVolume3
				<< "|AskVolume4=" << pDepthMarketData->AskVolume4
				<< "|AskVolume5=" << pDepthMarketData->AskVolume5
				<< "|AveragePrice=" << pDepthMarketData->AveragePrice
				<< "|BidPrice1=" << pDepthMarketData->BidPrice1
				<< "|BidPrice2=" << pDepthMarketData->BidPrice2
				<< "|BidPrice3=" << pDepthMarketData->BidPrice3
				<< "|BidPrice4=" << pDepthMarketData->BidPrice4
				<< "|BidPrice5=" << pDepthMarketData->BidPrice5
				<< "|BidVolume1=" << pDepthMarketData->BidVolume1
				<< "|BidVolume2=" << pDepthMarketData->BidVolume2
				<< "|BidVolume3=" << pDepthMarketData->BidVolume3
				<< "|BidVolume4=" << pDepthMarketData->BidVolume4
				<< "|BidVolume5=" << pDepthMarketData->BidVolume5
				<< "|ClosePrice=" << pDepthMarketData->ClosePrice
				<< "|CurrDelta=" << pDepthMarketData->CurrDelta
				<< "|ExchangeID=" << pDepthMarketData->ExchangeID
				<< "|ExchangeInstID=" << pDepthMarketData->ExchangeInstID
				<< "|HighestPrice=" << pDepthMarketData->HighestPrice
				<< "|LastPrice=" << pDepthMarketData->LastPrice
				<< "|LowerLimitPrice=" << pDepthMarketData->LowerLimitPrice
				<< "|LowestPrice=" << pDepthMarketData->LowestPrice
				<< "|OpenInterest=" << pDepthMarketData->OpenInterest
				<< "|OpenPrice=" << pDepthMarketData->OpenPrice
				<< "|PreClosePrice=" << pDepthMarketData->PreClosePrice
				<< "|PreDelta=" << pDepthMarketData->PreDelta
				<< "|PreOpenInterest=" << pDepthMarketData->PreOpenInterest
				<< "|PreSettlementPrice=" << pDepthMarketData->PreSettlementPrice
				<< "|SettlementPrice=" << pDepthMarketData->SettlementPrice
				<< "|TradingDay=" << pDepthMarketData->TradingDay
				<< "|Turnover=" << pDepthMarketData->Turnover
				<< "|UpdateMillisec=" << pDepthMarketData->UpdateMillisec
				<< "|UpdateTime=" << pDepthMarketData->UpdateTime
				<< "|UpperLimitPrice=" << pDepthMarketData->UpperLimitPrice
				<< "|Volume=" << pDepthMarketData->Volume;
		}
	}
}

///订阅询价请求
void ctp::CMdSpi::SubscribeForQuoteRsp(const std::string& instrument_id){
	char **ppInstrumentID = new char*[50];
	ppInstrumentID[0] = const_cast<char *>(instrument_id.c_str());
	int result = m_pUserMdApi->SubscribeForQuoteRsp(ppInstrumentID, 1);
}

///订阅询价应答
void ctp::CMdSpi::OnRspSubForQuoteRsp(
	CThostFtdcSpecificInstrumentField *pSpecificInstrument,
	CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast){
	if (pSpecificInstrument){
	}
	if (pRspInfo){
	}
	SetEvent(xinhao);
}


///询价通知
void ctp::CMdSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp){
	if (pForQuoteRsp){
	}
	SetEvent(xinhao);
}
