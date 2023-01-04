// #include "stdafx.h"

#include <conio.h>
#include <iostream>

#include "util/common.h"
#include "trade/trader.h"
#include "data/structs.h"
#include "quote/marketdata.h"
#include "manager/data.h"
#include "manager/instrument.h"
#include "database/tables.h"

HANDLE g_hEvent = CreateEvent(NULL, false, false, NULL);

//交易类
ctp::CTdSpi::CTdSpi(CThostFtdcTraderApi* pUserApi, ctp::BrokerInfo* pBroker)
	: m_pUserApi(pUserApi)
	, session_id(0)
	, front_id(0)
	, request_id(0){
	p_broker = pBroker;
	had_logged_in = false;
	had_connected = false;
	// trade worker
	_worker = std::thread([this]() {
		auto sqlite = mgr::DBManager::Get()->GetSqlite("simnow");
	auto rsp = std::unique_ptr<SqliteRsp>(sqlite->execute(
		"SELECT id,instrument,exchange_id,(origin_vol - traded_vol) AS vol,"
		"direction,status,update_time FROME orders WHERE vol > 0"));
	for (int i = 0; i < rsp->data.size(); i++) { // re insert the failed orders
		if (rsp->data[i][5] == "1") {
			std::string instrument = rsp->data[i][1];
			double price = mgr::DataManager::Get()->get(instrument)->price;
			this->ReqOrderInsert(
				rsp->data[i][1], true, price,
				std::atoi(rsp->data[i][3].c_str()),
				static_cast<ord::Direction>(std::atoi(rsp->data[i][4].c_str())));
			std::unique_ptr<SqliteRsp>(
				sqlite->execute("UPATE orders SET status = %d WHERE id = %s",
					10086, rsp->data[i][1].c_str()));
		}
	}
	});
}

ctp::CTdSpi::~CTdSpi() {
	m_pUserApi->Release();
}

void ctp::CTdSpi::OnFrontConnected(){
	had_connected = true;
	log_info << "td_front connnected";
	SetEvent(g_hEvent);
}

//客户端认证
void ctp::CTdSpi::ReqAuthenticate(){
	//strcpy_s(g_chUserProductInfo, getConfig("config", "UserProductInfo").c_str());

	CThostFtdcReqAuthenticateField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	strcpy_s(a.AppID, p_broker->app_id.c_str());
	strcpy_s(a.AuthCode, p_broker->auth_code.c_str());
	//strcpy_s(a.UserProductInfo, "");
	int b = m_pUserApi->ReqAuthenticate(&a, 1);
}

///客户端认证响应
void ctp::CTdSpi::OnRspAuthenticate(
	CThostFtdcRspAuthenticateField *pRspAuthenticateField,
	CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast){
	//CTraderSpi::OnRspAuthenticate(pRspAuthenticateField, pRspInfo, request_id, bIsLast);
	if (pRspInfo && pRspInfo->ErrorID) {
		log_error << p_broker->investor_id << " auth failed with code:"
			<< pRspInfo->ErrorID << ", msg:" << pRspInfo->ErrorMsg;
	}
	else {
		log_info << p_broker->investor_id << " auth succeede with code:"
			<< pRspInfo->ErrorID << ", msg:" << pRspInfo->ErrorMsg;
	}
	SetEvent(g_hEvent);
}

void ctp::CTdSpi::RegisterFensUserInfo(){
	CThostFtdcFensUserInfoField pFensUserInfo = { 0 };
	strcpy_s(pFensUserInfo.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(pFensUserInfo.UserID, p_broker->investor_id.c_str());
	pFensUserInfo.LoginMode = THOST_FTDC_LM_Trade;
	m_pUserApi->RegisterFensUserInfo(&pFensUserInfo);
}

void ctp::CTdSpi::OnFrontDisconnected(int nReason){
	had_connected = false;
	log_error << "td_front disconnnected with reason:" << nReason;
}

void ctp::CTdSpi::ReqUserLogin(){
	CThostFtdcReqUserLoginField reqUserLogin = { 0 };
	strcpy_s(reqUserLogin.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(reqUserLogin.UserID, p_broker->investor_id.c_str());
	strcpy_s(reqUserLogin.Password, p_broker->password.c_str());
	//strcpy_s(reqUserLogin.ClientIPAddress, "::c0a8:0101");
	//strcpy_s(reqUserLogin.UserProductInfo, "123");
	// 发出登陆请求
	m_pUserApi->ReqUserLogin(&reqUserLogin, request_id++);
}

void ctp::CTdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
	CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast){
	front_id = pRspUserLogin->FrontID;
	session_id = pRspUserLogin->SessionID;
	//CTraderSpi::OnRspUserLogin(pRspUserLogin, pRspInfo, request_id, bIsLast);
	if (pRspInfo && pRspInfo->ErrorID) {
		log_error << "login failed with code:" << pRspInfo->ErrorID
			<< ", msg:" << pRspInfo->ErrorMsg;
	}
	else {
		had_logged_in = true;
		log_info << "login succeeded with code:" << pRspInfo->ErrorID
			<< ", msg:" << pRspInfo->ErrorMsg;
	}
	SetEvent(g_hEvent);
}

void ctp::CTdSpi::ReqUserLogout(){
	CThostFtdcUserLogoutField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	m_pUserApi->ReqUserLogout(&a, request_id++);
}

///登出请求响应
void ctp::CTdSpi::OnRspUserLogout(
	CThostFtdcUserLogoutField *pUserLogout, 
	CThostFtdcRspInfoField *pRspInfo,
	int request_id, bool bIsLast){
	if (pUserLogout){
	}
	if (pRspInfo && pRspInfo->ErrorID){
		log_error << p_broker->investor_id << " logout failed with code:" << pRspInfo->ErrorID
			<< ", msg:" << pRspInfo->ErrorMsg;
	}
	else {
		had_logged_in = false;
		log_info << p_broker->investor_id << " logout succeeded with code:" << pRspInfo->ErrorID
			<< ", msg:" << pRspInfo->ErrorMsg;
	}
	m_pUserApi->Release();
}

/// OrderRef
std::string ctp::CTdSpi::GetOrderRef() {
	order_ref_id++;
	time_t n_day = time(0) / 86400;
	char buffer[14];
	sprintf(
		buffer, "%02ld%010ld", n_day % 100,
		order_ref_id % 10000000000);
	return buffer;
}

/// get order_id
std::string ctp::CTdSpi::GetOrderId(const std::string& order_ref) {
	char buffer[40];
	strncpy_s(buffer, order_ref.c_str(), 13);
	sprintf(buffer + 13, "#%d#%d", front_id, session_id);
	return buffer;
}


///请求确认结算单
void ctp::CTdSpi::ReqSettlementInfoConfirm()
{
	CThostFtdcSettlementInfoConfirmField Confirm = { 0 };
	///经纪公司代码
	strcpy_s(Confirm.BrokerID, p_broker->broker_id.c_str());
	///投资者代码
	strcpy_s(Confirm.InvestorID, p_broker->investor_id.c_str());
	m_pUserApi->ReqSettlementInfoConfirm(&Confirm, request_id++);
}

///投资者结算结果确认响应
void ctp::CTdSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
	CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast){
	SetEvent(g_hEvent);
}

///用户口令更新请求
void ctp::CTdSpi::ReqUserPasswordUpdate()
{
	std::string newpassword;
	std::cin >> newpassword;
	CThostFtdcUserPasswordUpdateField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	strcpy_s(a.OldPassword, p_broker->password.c_str());
	strcpy_s(a.NewPassword, newpassword.c_str());
	int b = m_pUserApi->ReqUserPasswordUpdate(&a, request_id++);
}

///用户口令更新请求响应
void ctp::CTdSpi::OnRspUserPasswordUpdate(
	CThostFtdcUserPasswordUpdateField *pUserPasswordUpdate,
	CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast){
	//CTraderSpi::OnRspUserPasswordUpdate(pUserPasswordUpdate, pRspInfo, request_id, bIsLast);
	SetEvent(g_hEvent);
}

///资金账户口令更新请求
void ctp::CTdSpi::ReqTradingAccountPasswordUpdate(){
std::string newpassword;
std::cin >> newpassword;
CThostFtdcTradingAccountPasswordUpdateField a = { 0 };
strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
strcpy_s(a.AccountID, p_broker->investor_id.c_str());
strcpy_s(a.OldPassword, p_broker->password.c_str());
strcpy_s(a.NewPassword, newpassword.c_str());
strcpy_s(a.CurrencyID, "CNY");
int b = m_pUserApi->ReqTradingAccountPasswordUpdate(&a, request_id++);
}

///资金账户口令更新请求响应
void ctp::CTdSpi::OnRspTradingAccountPasswordUpdate(CThostFtdcTradingAccountPasswordUpdateField *pTradingAccountPasswordUpdate, CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast)
{
//CTraderSpi::OnRspTradingAccountPasswordUpdate(pTradingAccountPasswordUpdate, pRspInfo, request_id, bIsLast);
SetEvent(g_hEvent);
}

// 买卖方向转化的辅助函数
void ctp::CTdSpi::ConvDirection(
bool close_yd_pos,
const ord::Direction direction,
TThostFtdcDirectionType& ctp_direction,
TThostFtdcOffsetFlagType& ctp_action) {
if (direction == ord::Direction::BUY) {
	ctp_direction = THOST_FTDC_D_Buy;
	ctp_action = THOST_FTDC_OF_Open;
}
else if (direction == ord::Direction::SELL) {
	ctp_direction = THOST_FTDC_D_Sell;
	ctp_action = close_yd_pos ? THOST_FTDC_OF_Close : THOST_FTDC_OF_CloseToday;
}
else if (direction == ord::Direction::SELLSHORT) {
	ctp_direction = THOST_FTDC_D_Sell;
	ctp_action = THOST_FTDC_OF_Open;
}
else if (direction == ord::Direction::BUYTOCOVER) {
	ctp_direction = THOST_FTDC_D_Buy;
	ctp_action = close_yd_pos ? THOST_FTDC_OF_Close : THOST_FTDC_OF_CloseToday;
}
else {
	throw "unknow trade direction got:" + std::to_string(int(direction));
}
}

std::string ctp::CTdSpi::GetDirection(
	ord::Direction& direction,
	const TThostFtdcDirectionType ctp_direction,
	const TThostFtdcOffsetFlagType ctp_action) {
	if (ctp_direction == THOST_FTDC_D_Buy && ctp_action == THOST_FTDC_OF_Open){
		direction = ord::Direction::BUY;
		return "buy";
	}
	else if (ctp_direction == THOST_FTDC_D_Sell && (
		ctp_action == THOST_FTDC_OF_Close || ctp_action == THOST_FTDC_OF_CloseToday)){
		direction = ord::Direction::SELL;
		return "sell";
	}
	else if (ctp_direction == THOST_FTDC_D_Sell && ctp_action == THOST_FTDC_OF_Open){
		direction = ord::Direction::SELLSHORT;
		return "sellshort";
	}
	else if (ctp_direction == THOST_FTDC_D_Buy && (
		ctp_action == THOST_FTDC_OF_Close || ctp_action == THOST_FTDC_OF_CloseToday)){
		direction = ord::Direction::BUYTOCOVER;
		return "buytocover";
	}
	else {
		return "invalid-direction";
	}
}

ord::Direction ctp::CTdSpi::ConvDirection(
	TThostFtdcDirectionType ctp_direction,
	TThostFtdcOffsetFlagType ctp_action) {
	if (ctp_direction == THOST_FTDC_D_Buy &&
		ctp_action == THOST_FTDC_OF_Open){
		return ord::Direction::BUY;
	}
	else if (ctp_direction == THOST_FTDC_D_Sell &&
		ctp_action == THOST_FTDC_OF_Close){
		return ord::Direction::SELL;
	}
	else if (ctp_direction == THOST_FTDC_D_Sell &&
		ctp_action == THOST_FTDC_OF_Open){
		return ord::Direction::SELLSHORT;
	}
	else if (ctp_direction == THOST_FTDC_D_Buy &&
		ctp_action == THOST_FTDC_OF_Close){
		return ord::Direction::BUYTOCOVER;
	}
	else {
		return ord::Direction::_MAX;
	}
}


///预埋单录入//限价单
std::string ctp::CTdSpi::ReqParkedOrderInsert(
	ord::Direction direction, bool close_yd_pos,
	double limit_price, int volume,
	const std::string& instrument_id, 
	const std::string& exchange_id){
	// function body
	CThostFtdcParkedOrderField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	
	std::string order_ref = GetOrderRef();
	strncpy_s(a.OrderRef, order_ref.c_str(), 13);
	ConvDirection(close_yd_pos, direction, a.Direction, a.CombOffsetFlag[0]);
	a.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	a.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	a.LimitPrice = limit_price;
	a.VolumeTotalOriginal = volume;
	a.TimeCondition = THOST_FTDC_TC_GFD;
	a.VolumeCondition = THOST_FTDC_VC_AV;
	a.MinVolume = 1;
	a.ContingentCondition = THOST_FTDC_CC_Immediately;
	a.StopPrice = 0;
	a.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	a.IsAutoSuspend = 0;
	strcpy_s(a.ExchangeID, exchange_id.c_str());
	int b = m_pUserApi->ReqParkedOrderInsert(&a, request_id++);
	return b == 0 ? order_ref : "";
}

///预埋撤单录入请求
void ctp::CTdSpi::ReqParkedOrderAction(
	const std::string& order_sys_id,
	const std::string& instrument_id, const std::string& exchange_id){
	CThostFtdcParkedOrderActionField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	//strcpy_s(a.OrderRef, "          15");
	strcpy_s(a.ExchangeID, exchange_id.c_str());
	/*a.FrontID = 1;
	a.SessionID = -287506422;*/
	strcpy_s(a.OrderSysID, order_sys_id.c_str());
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	a.ActionFlag = THOST_FTDC_AF_Delete;
	int b = m_pUserApi->ReqParkedOrderAction(&a, request_id++);
}

///请求删除预埋单
void ctp::CTdSpi::ReqRemoveParkedOrder(const std::string& parked_order_id)
{
	CThostFtdcRemoveParkedOrderField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.ParkedOrderID, parked_order_id.c_str());
	int b = m_pUserApi->ReqRemoveParkedOrder(&a, request_id++);
}

///请求删除预埋撤单
void ctp::CTdSpi::ReqRemoveParkedOrderAction(const std::string& parked_order_action_id)
{
	CThostFtdcRemoveParkedOrderActionField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.ParkedOrderActionID, parked_order_action_id.c_str());
	int b = m_pUserApi->ReqRemoveParkedOrderAction(&a, request_id++);
}

///报单录入请求
std::string ctp::CTdSpi::ReqOrderInsert(
	const std::string& instrument_id, bool close_yd_pos,
	float price, int volume, ord::Direction direction){
	// put to data base first
	// 条件单由于是服务器代发，所以frontid和sessoinid都是0
	auto sqlite = mgr::DBManager::Get()->GetSqlite("simnow");
	char buffer[1024];
	std::string exchange_id = mgr::InstrumentManager::Get()->get_exhcnage_id(instrument_id);
	sprintf_s(
		buffer,
		"INSERT INTO %s(instrument, exchange_id, direction, "
		"price, origin_vol) VAUES('%s','%s',%d,%f,%d)",
		ORDER_TABLE_NAME, instrument_id.c_str(),
		exchange_id.c_str(), direction, price, volume);
	std::shared_ptr<SqliteRsp> rsp(sqlite->execute(buffer));
	int order_id = sqlite->last_insert_rowid();
	if (rsp->code||order_id == 0) {
		log_error << "insert order failed with sql:" << buffer
			<< ". code:" << rsp->code << ", order_id:" << order_id;
		return "";
	}
	// function body
	CThostFtdcInputOrderField ord = { 0 };
	sprintf_s(ord.OrderRef, "%d", order_id);
	strcpy_s(ord.ExchangeID, exchange_id.c_str());
	strcpy_s(ord.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(ord.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(ord.InstrumentID, instrument_id.c_str());
	strcpy_s(ord.UserID, p_broker->investor_id.c_str());
	ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	ConvDirection(
		close_yd_pos, direction, 
		ord.Direction, ord.CombOffsetFlag[0]);
	ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	ord.LimitPrice = price;
	ord.VolumeTotalOriginal = volume;
	ord.TimeCondition = THOST_FTDC_TC_GFD;///当日有效
	ord.VolumeCondition = THOST_FTDC_VC_AV;///全部数量
	ord.MinVolume = 1;
	ord.ContingentCondition = THOST_FTDC_CC_Immediately;
	ord.StopPrice = 0;
	ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	ord.IsAutoSuspend = 0;
	if (m_pUserApi->ReqOrderInsert(&ord, request_id++)) {
		log_warning << "insert order(" << order_id << ") failed!"
			" will re-send by trader-worker in next time.";
		if (sqlite->execute("UPDATE order SET status=1 WHERE id=%d", order_id)) {
			log_error << "retry to insert order(" << order_id << ") failed!";
		}
	}
	return ord.OrderRef;
}

///大商所止损单
void ctp::CTdSpi::ReqOrderInsert_Touch(
	const std::string& instrument_id, const std::string& exchange_id){
	int new_limitprice;
	std::cin >> new_limitprice;

	int new_StopPrice;
	std::cin >> new_StopPrice;

	CThostFtdcInputOrderField ord = { 0 };
	strcpy_s(ord.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(ord.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(ord.InstrumentID, instrument_id.c_str());
	strcpy_s(ord.UserID, p_broker->investor_id.c_str());
	//strcpy_s(ord.OrderRef, "");
	ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	ord.Direction = THOST_FTDC_D_Buy;//买
	ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	//ord.LimitPrice = atoi(getConfig("config", "LimitPrice").c_str());
	ord.LimitPrice = new_limitprice;
	ord.VolumeTotalOriginal = 1;
	ord.TimeCondition = THOST_FTDC_TC_GFD;///当日有效
	ord.VolumeCondition = THOST_FTDC_VC_AV;///任何数量
	ord.MinVolume = 1;
	ord.ContingentCondition = THOST_FTDC_CC_Touch;
	ord.StopPrice = new_StopPrice;
	ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	ord.IsAutoSuspend = 0;
	strcpy_s(ord.ExchangeID, exchange_id.c_str());
	int a = m_pUserApi->ReqOrderInsert(&ord, 1);
}

///大商所止盈单
void ctp::CTdSpi::ReqOrderInsert_TouchProfit(
	const std::string& instrument_id, const std::string& exchange_id){
	int new_limitprice;
	std::cin >> new_limitprice;

	int new_StopPrice;
	std::cin >> new_StopPrice;

	CThostFtdcInputOrderField ord = { 0 };
	strcpy_s(ord.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(ord.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(ord.InstrumentID, instrument_id.c_str());
	strcpy_s(ord.UserID, p_broker->investor_id.c_str());
	//strcpy_s(ord.OrderRef, "");
	ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	ord.Direction = THOST_FTDC_D_Buy;//买
	ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	//ord.LimitPrice = atoi(getConfig("config", "LimitPrice").c_str());
	ord.LimitPrice = new_limitprice;
	ord.VolumeTotalOriginal = 1;
	ord.TimeCondition = THOST_FTDC_TC_GFD;///当日有效
	ord.VolumeCondition = THOST_FTDC_VC_AV;///全部数量
	ord.MinVolume = 1;
	ord.ContingentCondition = THOST_FTDC_CC_TouchProfit;
	ord.StopPrice = new_StopPrice;
	ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	ord.IsAutoSuspend = 0;
	strcpy_s(ord.ExchangeID, exchange_id.c_str());
	int a = m_pUserApi->ReqOrderInsert(&ord, 1);
}

//全成全撤
void ctp::CTdSpi::ReqOrderInsert_VC_CV(
	const std::string& instrument_id, const std::string& exchange_id){
	int new_limitprice;
	std::cin >> new_limitprice;

	int insert_num;
	std::cin >> insert_num;

	CThostFtdcInputOrderField ord = { 0 };
	strcpy_s(ord.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(ord.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(ord.InstrumentID, instrument_id.c_str());
	strcpy_s(ord.UserID, p_broker->investor_id.c_str());
	//strcpy_s(ord.OrderRef, "");
	ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	ord.Direction = THOST_FTDC_D_Buy;//买
	ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	//ord.LimitPrice = atoi(getConfig("config", "LimitPrice").c_str());
	ord.LimitPrice = new_limitprice;
	ord.VolumeTotalOriginal = insert_num;
	ord.TimeCondition = THOST_FTDC_TC_GFD;///当日有效
	ord.VolumeCondition = THOST_FTDC_VC_CV;///全部数量
	ord.MinVolume = 1;
	ord.ContingentCondition = THOST_FTDC_CC_Immediately;
	ord.StopPrice = 0;
	ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	ord.IsAutoSuspend = 0;
	strcpy_s(ord.ExchangeID, exchange_id.c_str());
	int a = m_pUserApi->ReqOrderInsert(&ord, 1);
}

//部成部撤
void ctp::CTdSpi::ReqOrderInsert_VC_AV(
	const std::string& instrument_id, const std::string& exchange_id){
	int new_limitprice;
	std::cin >> new_limitprice;

	int insert_num;
	std::cin >> insert_num;

	CThostFtdcInputOrderField ord = { 0 };
	strcpy_s(ord.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(ord.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(ord.InstrumentID, instrument_id.c_str());
	strcpy_s(ord.UserID, p_broker->investor_id.c_str());
	//strcpy_s(ord.OrderRef, "");
	ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	ord.Direction = THOST_FTDC_D_Buy;//买
	ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	//ord.LimitPrice = atoi(getConfig("config", "LimitPrice").c_str());
	ord.LimitPrice = new_limitprice;
	ord.VolumeTotalOriginal = insert_num;
	ord.TimeCondition = THOST_FTDC_TC_GFD;///当日有效
	ord.VolumeCondition = THOST_FTDC_VC_AV;///任何数量
	ord.MinVolume = 1;
	ord.ContingentCondition = THOST_FTDC_CC_Immediately;
	ord.StopPrice = 0;
	ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	ord.IsAutoSuspend = 0;
	strcpy_s(ord.ExchangeID, exchange_id.c_str());
	int a = m_pUserApi->ReqOrderInsert(&ord, 1);
}

//市价单
void ctp::CTdSpi::ReqOrderInsert_AnyPrice(
	const std::string& instrument_id, const std::string& exchange_id){
	CThostFtdcInputOrderField ord = { 0 };
	strcpy_s(ord.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(ord.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(ord.InstrumentID, instrument_id.c_str());
	strcpy_s(ord.UserID, p_broker->investor_id.c_str());
	//strcpy_s(ord.OrderRef, "");
	ord.OrderPriceType = THOST_FTDC_OPT_AnyPrice;
	ord.Direction = THOST_FTDC_D_Buy;//买
	ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	//ord.LimitPrice = atoi(getConfig("config", "LimitPrice").c_str());
	//ord.LimitPrice = new_limitprice;
	ord.LimitPrice = 0;
	ord.VolumeTotalOriginal = 1;
	ord.TimeCondition = THOST_FTDC_TC_IOC;///立即完成，否则撤销
	ord.VolumeCondition = THOST_FTDC_VC_AV;///任何数量
	ord.MinVolume = 1;
	ord.ContingentCondition = THOST_FTDC_CC_Immediately;
	//ord.StopPrice = 0;
	ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	ord.IsAutoSuspend = 0;
	strcpy_s(ord.ExchangeID, exchange_id.c_str());
	int a = m_pUserApi->ReqOrderInsert(&ord, 1);
}

//市价转限价单(中金所)
void ctp::CTdSpi::ReqOrderInsert_BestPrice(
	const std::string& instrument_id, const std::string& exchange_id){
	CThostFtdcInputOrderField ord = { 0 };
	strcpy_s(ord.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(ord.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(ord.InstrumentID, instrument_id.c_str());
	strcpy_s(ord.UserID, p_broker->investor_id.c_str());
	//strcpy_s(ord.OrderRef, "");
	ord.OrderPriceType = THOST_FTDC_OPT_BestPrice;
	ord.Direction = THOST_FTDC_D_Buy;//买
	ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	//ord.LimitPrice = atoi(getConfig("config", "LimitPrice").c_str());
	//ord.LimitPrice = new_limitprice;
	ord.VolumeTotalOriginal = 1;
	ord.TimeCondition = THOST_FTDC_TC_GFD;///当日有效
	ord.VolumeCondition = THOST_FTDC_VC_AV;///任何数量
	ord.MinVolume = 1;
	ord.ContingentCondition = THOST_FTDC_CC_Immediately;
	ord.StopPrice = 0;
	ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	ord.IsAutoSuspend = 0;
	strcpy_s(ord.ExchangeID, exchange_id.c_str());
	int a = m_pUserApi->ReqOrderInsert(&ord, 1);
}

//套利指令
void ctp::CTdSpi::ReqOrderInsert_Arbitrage(
	const std::string& instrument_id, const std::string& exchange_id){
	int new_limitprice;
	std::cin >> new_limitprice;

	CThostFtdcInputOrderField ord = { 0 };
	strcpy_s(ord.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(ord.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(ord.InstrumentID, instrument_id.c_str());
	strcpy_s(ord.UserID, p_broker->investor_id.c_str());
	//strcpy_s(ord.OrderRef, "");
	ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	ord.Direction = THOST_FTDC_D_Buy;//买
	ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	//ord.LimitPrice = atoi(getConfig("config", "LimitPrice").c_str());
	ord.LimitPrice = new_limitprice;
	ord.VolumeTotalOriginal = 1;
	ord.TimeCondition = THOST_FTDC_TC_GFD;///当日有效
	ord.VolumeCondition = THOST_FTDC_VC_AV;///任何数量
	ord.MinVolume = 1;
	ord.ContingentCondition = THOST_FTDC_CC_Immediately;
	ord.StopPrice = 0;
	ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	ord.IsAutoSuspend = 0;
	strcpy_s(ord.ExchangeID, exchange_id.c_str());
	int a = m_pUserApi->ReqOrderInsert(&ord, 1);
}

//互换单
void ctp::CTdSpi::ReqOrderInsert_IsSwapOrder(
	const std::string& instrument_id, const std::string& exchange_id){
	int new_limitprice;
	std::cin >> new_limitprice;

	CThostFtdcInputOrderField ord = { 0 };
	strcpy_s(ord.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(ord.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(ord.InstrumentID, instrument_id.c_str());
	strcpy_s(ord.UserID, p_broker->investor_id.c_str());
	//strcpy_s(ord.OrderRef, "");
	ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	ord.Direction = THOST_FTDC_D_Buy;//买
	ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	//ord.LimitPrice = atoi(getConfig("config", "LimitPrice").c_str());
	ord.LimitPrice = new_limitprice;
	ord.VolumeTotalOriginal = 1;
	ord.TimeCondition = THOST_FTDC_TC_GFD;///当日有效
	ord.VolumeCondition = THOST_FTDC_VC_AV;///任何数量
	ord.MinVolume = 1;
	ord.ContingentCondition = THOST_FTDC_CC_Immediately;
	ord.StopPrice = 0;
	ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	ord.IsAutoSuspend = 0;
	ord.IsSwapOrder = 1;//互换单标志
	strcpy_s(ord.ExchangeID, exchange_id.c_str());
	int a = m_pUserApi->ReqOrderInsert(&ord, 1);
}

///报单操作请求
void ctp::CTdSpi::ReqOrderAction_Ordinary(
	const std::string& order_sys_id, const std::string& order_ref,
	const std::string& instrument_id, const std::string& exchange_id){
	CThostFtdcInputOrderActionField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	a.OrderActionRef = 1;
	strcpy_s(a.OrderRef, order_ref.c_str());
	//a.FrontID = front_id;
	//a.SessionID = session_id;
	strcpy_s(a.ExchangeID, exchange_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	strcpy_s(a.OrderSysID, order_sys_id.c_str());
	a.ActionFlag = THOST_FTDC_AF_Delete;
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	int ab = m_pUserApi->ReqOrderAction(&a, request_id++);
}

///执行宣告录入请求
void ctp::CTdSpi::ReqExecOrderInsert(
	int action,
	const std::string& instrument_id, const std::string& exchange_id){
	CThostFtdcInputExecOrderField OrderInsert = { 0 };
	strcpy_s(OrderInsert.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(OrderInsert.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(OrderInsert.InstrumentID, instrument_id.c_str());
	strcpy_s(OrderInsert.ExchangeID, exchange_id.c_str());
	//strcpy_s(OrderInsert.ExecOrderRef, "00001");
	strcpy_s(OrderInsert.UserID, p_broker->investor_id.c_str());
	OrderInsert.Volume = 1;
	OrderInsert.RequestID = 1;
	OrderInsert.OffsetFlag = THOST_FTDC_OF_Close;//开平标志
	OrderInsert.HedgeFlag = THOST_FTDC_HF_Speculation;//投机套保标志
	if (action == 0) {
		OrderInsert.ActionType = THOST_FTDC_ACTP_Exec;//执行类型类型
	}
	if (action == 1) {
		OrderInsert.ActionType = THOST_FTDC_ACTP_Abandon;//执行类型类型
	}
	OrderInsert.PosiDirection = THOST_FTDC_PD_Long;//持仓多空方向类型
	OrderInsert.ReservePositionFlag = THOST_FTDC_EOPF_Reserve;//期权行权后是否保留期货头寸的标记类型
	//OrderInsert.ReservePositionFlag = THOST_FTDC_EOPF_UnReserve;//不保留头寸
	OrderInsert.CloseFlag = THOST_FTDC_EOCF_NotToClose;//期权行权后生成的头寸是否自动平仓类型
	//OrderInsert.CloseFlag = THOST_FTDC_EOCF_AutoClose;//自动平仓
	//strcpy_s(OrderInsert.InvestUnitID, "");AccountID
	//strcpy_s(OrderInsert.AccountID, "");
	//strcpy_s(OrderInsert.CurrencyID, "CNY");
	//strcpy_s(OrderInsert.ClientID, "");
	int b = m_pUserApi->ReqExecOrderInsert(&OrderInsert, 1);
}

///执行宣告操作请求
void ctp::CTdSpi::ReqExecOrderAction(
	const std::string& order_sys_id, const std::string& order_ref,
	const std::string& instrument_id, const std::string& exchange_id){
	CThostFtdcInputExecOrderActionField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	a.ExecOrderActionRef = 1;
	strcpy_s(a.ExecOrderRef, order_ref.c_str());
	a.FrontID = front_id;
	a.SessionID = session_id;
	strcpy_s(a.ExchangeID, exchange_id.c_str());
	strcpy_s(a.ExecOrderSysID, order_sys_id.c_str());
	a.ActionFlag = THOST_FTDC_AF_Delete;//删除
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	//strcpy_s(a.InvestUnitID, "");
	//strcpy_s(a.IPAddress, "");
	//strcpy_s(a.MacAddress, "");
	int b = m_pUserApi->ReqExecOrderAction(&a, 1);
}

//批量报单操作请求
void ctp::CTdSpi::ReqBatchOrderAction(){
	CThostFtdcInputBatchOrderActionField a = { 0 };

}

///请求查询报单
void ctp::CTdSpi::ReqQryOrder(const std::string& exchange_id){
	CThostFtdcQryOrderField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	//strcpy_s(a.InstrumentID, instrument_id.c_str());
	strcpy_s(a.ExchangeID, exchange_id.c_str());
	int ab = m_pUserApi->ReqQryOrder(&a, request_id++);
}

///报单录入请求
std::string ctp::CTdSpi::ReqOrderPreInsert(
	const std::string& instrument_id, bool close_yd_pos,
	float price, int volume, ord::Direction direction){
	// function body
	auto sqlite = mgr::DBManager::Get()->GetSqlite("simnow");
	char buffer[1024];
	std::string exchange_id = mgr::InstrumentManager::Get()->get_exhcnage_id(instrument_id);
	sprintf_s(
		buffer,
		"INSERT INTO %s(instrument, echange_id, direction, "
		"order_type, price, origin_vol) VAUES('%s','%s',%d,%d,%f,%d)",
		ORDER_TABLE_NAME, instrument_id.c_str(),
		exchange_id.c_str(), 1, direction, price, volume);
	std::shared_ptr<SqliteRsp> rsp(sqlite->execute(buffer));
	int order_id = sqlite->last_insert_rowid();
	if (rsp->code || order_id == 0) {
		log_error << "insert order failed with sql:" << buffer
			<< ". code:" << rsp->code << ", order_id:" << order_id;
		return 0;
	}
	CThostFtdcInputOrderField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	a.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	ConvDirection(close_yd_pos, direction, a.Direction, a.CombOffsetFlag[0]);
	sprintf_s(a.OrderRef, "%d", order_id);
	a.ContingentCondition = get_trade_condition(direction);
	a.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	// a.LimitPrice = atoi(limit_price.c_str());
	a.VolumeTotalOriginal = volume;
	a.TimeCondition = THOST_FTDC_TC_GFD;
	a.VolumeCondition = THOST_FTDC_VC_AV;
	a.MinVolume = 0;
	a.StopPrice = price;
	a.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	a.IsAutoSuspend = 0;
	strcpy_s(a.ExchangeID, exchange_id.c_str());
	if (m_pUserApi->ReqOrderInsert(&a, request_id++)) {
		log_warning << "insert order(" << order_id << ") failed!"
			" will re-send by trader-worker in next time.";
		if (sqlite->execute("UPDATE order SET status=1 WHERE id=%d", order_id)) {
			log_error << "retry to insert order(" << order_id << ") failed!";
		}
	}
	return a.OrderRef;
}

///报单操作请求
void ctp::CTdSpi::ReqOrderAction(const std::string& order_ref){
	// function body
	auto sqlite = mgr::DBManager::Get()->GetSqlite("simnow");
	std::shared_ptr<SqliteRsp> rsp(sqlite->execute(
		"SELECT instrument, exchange_id FROM %s WHERE id=%s",
		ORDER_TABLE_NAME, order_ref.c_str()));
	if (rsp->code || rsp->data.size() < 1){
		log_error << "FATAL ERROR: Can not found order with ref:" << order_ref
			<< ". sql return code:" << rsp->code << " and data size:" << rsp->data.size();
		return;
	}
	CThostFtdcInputOrderActionField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, rsp->data[0][0].c_str());
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	strcpy_s(a.OrderRef, order_ref.c_str());
	// strcpy_s(a.OrderSysID, order_sys_id.c_str());
	strcpy_s(a.ExchangeID, rsp->data[0][1].c_str());
	a.ActionFlag = THOST_FTDC_AF_Delete;
	int code = m_pUserApi->ReqOrderAction(&a, request_id++);
	if (code) {
		log_error << "Cancel Failed with OrderId:" << order_ref;
	}
}

//撤销查询的报单
void ctp::CTdSpi::ReqOrderAction_forqry(int action_num){
	CThostFtdcInputOrderActionField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.UserID, p_broker->investor_id.c_str());

	/*
	strcpy_s(a.OrderSysID, vector_OrderSysID.at(action_num - 1).c_str());
	strcpy_s(a.ExchangeID, vector_ExchangeID.at(action_num - 1).c_str());
	strcpy_s(a.InstrumentID, vector_InstrumentID.at(action_num - 1).c_str());
	*/

	a.ActionFlag = THOST_FTDC_AF_Delete;
	int ab = m_pUserApi->ReqOrderAction(&a, request_id++);
}

///请求查询成交
void ctp::CTdSpi::ReqQryTrade()
{
	CThostFtdcQryTradeField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	std::string instr;
	instr.clear();
	std::cin.ignore();
	getline(std::cin, instr);
	strcpy_s(a.InstrumentID, instr.c_str());

	std::string Exch;
	Exch.clear();
	//std::cin.ignore();
	getline(std::cin, Exch);
	strcpy_s(a.ExchangeID, Exch.c_str());
	/*strcpy_s(a.TradeID, "");
	strcpy_s(a.TradeTimeStart, "");
	strcpy_s(a.TradeTimeEnd, "");*/
	int b = m_pUserApi->ReqQryTrade(&a, request_id++);
}

///请求查询预埋单
void ctp::CTdSpi::ReqQryParkedOrder(const std::string& exchange_id)
{
	CThostFtdcQryParkedOrderField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	//strcpy_s(a.InstrumentID, instrument_id.c_str());
	strcpy_s(a.ExchangeID, exchange_id.c_str());
	int ab = m_pUserApi->ReqQryParkedOrder(&a, request_id++);
}

//请求查询服务器预埋撤单
void ctp::CTdSpi::ReqQryParkedOrderAction(
	const std::string& instrument_id, const std::string& exchange_id){
	CThostFtdcQryParkedOrderActionField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	strcpy_s(a.ExchangeID, exchange_id.c_str());
	int ab = m_pUserApi->ReqQryParkedOrderAction(&a, request_id++);
}

//请求查询资金账户
void ctp::CTdSpi::ReqQryTradingAccount()
{
	CThostFtdcQryTradingAccountField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.CurrencyID, "CNY");
	int ab = m_pUserApi->ReqQryTradingAccount(&a, request_id++);
}

//请求查询投资者持仓
void ctp::CTdSpi::ReqQryInvestorPosition()
{
	CThostFtdcQryInvestorPositionField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	std::string instr;
	instr.clear();
	std::cin.ignore();
	getline(std::cin, instr);
	strcpy_s(a.InstrumentID, instr.c_str());

	std::string exch;
	exch.clear();
	std::cin.ignore();
	getline(std::cin, exch);
	strcpy_s(a.ExchangeID, exch.c_str());
	//strcpy_s(a.InstrumentID, "SPD");
	int b = m_pUserApi->ReqQryInvestorPosition(&a, request_id++);
}

//请求查询投资者持仓明细
void ctp::CTdSpi::ReqQryInvestorPositionDetail()
{
	CThostFtdcQryInvestorPositionDetailField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	std::string instr;
	instr.clear();
	std::cin.ignore();
	getline(std::cin, instr);
	strcpy_s(a.InstrumentID, instr.c_str());
	std::string exch;
	exch.clear();
	std::cin.ignore();
	getline(std::cin, exch);
	strcpy_s(a.ExchangeID, exch.c_str());
	//strcpy_s(a.InstrumentID, instrument_id.c_str());
	int b = m_pUserApi->ReqQryInvestorPositionDetail(&a, request_id++);
}

//请求查询交易所保证金率
void ctp::CTdSpi::ReqQryExchangeMarginRate(const std::string& instrument_id)
{
	CThostFtdcQryExchangeMarginRateField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	a.HedgeFlag = THOST_FTDC_HF_Speculation;//投机
	int b = m_pUserApi->ReqQryExchangeMarginRate(&a, request_id++);
}

//请求查询合约保证金率
void ctp::CTdSpi::ReqQryInstrumentMarginRate(const std::string& instrument_id)
{
	CThostFtdcQryInstrumentMarginRateField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	a.HedgeFlag = THOST_FTDC_HF_Speculation;//投机
	int b = m_pUserApi->ReqQryInstrumentMarginRate(&a, request_id++);
}

//请求查询合约手续费率
void ctp::CTdSpi::ReqQryInstrumentCommissionRate(const std::string& instrument_id)
{
	CThostFtdcQryInstrumentCommissionRateField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	int b = m_pUserApi->ReqQryInstrumentCommissionRate(&a, request_id++);
}

//请求查询做市商合约手续费率
void ctp::CTdSpi::ReqQryMMInstrumentCommissionRate(const std::string& instrument_id)
{
	CThostFtdcQryMMInstrumentCommissionRateField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	int b = m_pUserApi->ReqQryMMInstrumentCommissionRate(&a, request_id++);
}

//请求查询做市商期权合约手续费
void ctp::CTdSpi::ReqQryMMOptionInstrCommRate(const std::string& instrument_id)
{
	CThostFtdcQryMMOptionInstrCommRateField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	int b = m_pUserApi->ReqQryMMOptionInstrCommRate(&a, request_id++);
}

//请求查询报单手续费
void ctp::CTdSpi::ReqQryInstrumentOrderCommRate(const std::string& instrument_id)
{
	CThostFtdcQryInstrumentOrderCommRateField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	int b = m_pUserApi->ReqQryInstrumentOrderCommRate(&a, request_id++);
}

//请求查询期权合约手续费
void ctp::CTdSpi::ReqQryOptionInstrCommRate()
{
	CThostFtdcQryOptionInstrCommRateField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	std::string Inst;
	std::string Exch;
	std::string InvestUnit;
	std::cin >> Inst;
	std::cin >> Exch;
	std::cin >> InvestUnit;
	strcpy_s(a.InstrumentID, Inst.c_str());
	strcpy_s(a.ExchangeID, Exch.c_str());
	strcpy_s(a.InvestUnitID, InvestUnit.c_str());
	int b = m_pUserApi->ReqQryOptionInstrCommRate(&a, request_id++);
}

//请求查询合约
void ctp::CTdSpi::ReqQryInstrument(
	const std::string& instrument_id, const std::string& exchange_id){
	CThostFtdcQryInstrumentField a = { 0 };
	strcpy_s(a.ExchangeID, exchange_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	int b = m_pUserApi->ReqQryInstrument(&a, request_id++);
}

///请求查询合约响应
void ctp::CTdSpi::OnRspQryInstrument(
	CThostFtdcInstrumentField *pInstrument,
	CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast){
	if (pInstrument){
		// log_info << "PUBLIC_INSTRUMENT_INFO|instrument_id=" << pInstrument->InstrumentID
		// 	<< "|exchange_id=" << pInstrument->ExchangeID << "|symbol=" 
		// 	<< ctp::InstrumentManager::Get()->to_symbol(pInstrument->InstrumentID);
		mgr::InstrumentManager::Get()->append(new CThostFtdcInstrumentField(*pInstrument));
	}
	if (pRspInfo){
		log_error << "OnRspQryInstrument Failed, code:" << pRspInfo->ErrorID
			<< ", msg:" << pRspInfo->ErrorMsg;
	} 
	if (bIsLast){
		SetEvent(xinhao);
	}
}

//请求查询投资者结算结果
void ctp::CTdSpi::ReqQrySettlementInfo()
{
	CThostFtdcQrySettlementInfoField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	std::string Traday;
	std::cin >> Traday;
	strcpy_s(a.TradingDay, Traday.c_str());
	int b = m_pUserApi->ReqQrySettlementInfo(&a, request_id++);
}

//请求查询转帐流水
void ctp::CTdSpi::ReqQryTransferSerial(const std::string& exchange_id)
{
	CThostFtdcQryTransferSerialField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.AccountID, p_broker->investor_id.c_str());
cir1:int bankid;
	std::cin >> bankid;
	if (bankid == 1 | 2 | 3 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16){
		//strcpy_s(a.BankID, itoa(bankid, a.BankID, 10));///银行代码
		itoa(bankid, a.BankID, 10);
	}
	else
	{
		goto cir1;
	}
curr:int choos;
	std::cin >> choos;
	switch (choos)
	{
	case 1:
		strcpy_s(a.CurrencyID, "CNY");
		break;
	case 2:
		strcpy_s(a.CurrencyID, "USD");
		break;
	default:
		_getch();
		goto curr;
	}
	int b = m_pUserApi->ReqQryTransferSerial(&a, request_id++);
}

//请求查询产品
void ctp::CTdSpi::ReqQryProduct(const std::string& exchange_id)
{
	CThostFtdcQryProductField a = { 0 };
	strcpy_s(a.ProductID, "sc");
	a.ProductClass = THOST_FTDC_PC_Futures;
	strcpy_s(a.ExchangeID, exchange_id.c_str());
	m_pUserApi->ReqQryProduct(&a, request_id++);
}

//请求查询转帐银行
void ctp::CTdSpi::ReqQryTransferBank()
{
	CThostFtdcQryTransferBankField a = { 0 };
	strcpy_s(a.BankID,"3");
	int b = m_pUserApi->ReqQryTransferBank(&a, request_id++);
}

//请求查询交易通知
void ctp::CTdSpi::ReqQryTradingNotice()
{
	CThostFtdcQryTradingNoticeField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	int b = m_pUserApi->ReqQryTradingNotice(&a, request_id++);
}

//请求查询交易编码
void ctp::CTdSpi::ReqQryTradingCode(const std::string& exchange_id)
{
	CThostFtdcQryTradingCodeField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.ExchangeID, exchange_id.c_str());
	a.ClientIDType = THOST_FTDC_CIDT_Speculation;
	int b = m_pUserApi->ReqQryTradingCode(&a, request_id++);
}

//请求查询结算信息确认
void ctp::CTdSpi::ReqQrySettlementInfoConfirm()
{
	CThostFtdcQrySettlementInfoConfirmField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	//strcpy_s(a.AccountID, p_broker->investor_id.c_str());
	strcpy_s(a.CurrencyID, "CNY");
	int b = m_pUserApi->ReqQrySettlementInfoConfirm(&a, request_id++);
}

//请求查询产品组
void ctp::CTdSpi::ReqQryProductGroup()
{
	CThostFtdcQryProductGroupField a = { 0 };

}

//请求查询投资者单元
void ctp::CTdSpi::ReqQryInvestUnit()
{
	CThostFtdcQryInvestUnitField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	//strcpy_s(a.InvestorID, "00402");
	//strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	int b = m_pUserApi->ReqQryInvestUnit(&a, request_id++);
}

//请求查询经纪公司交易参数
void ctp::CTdSpi::ReqQryBrokerTradingParams()
{
	CThostFtdcQryBrokerTradingParamsField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.CurrencyID, "CNY");
	int b = m_pUserApi->ReqQryBrokerTradingParams(&a, request_id++);
}

//请求查询询价
void ctp::CTdSpi::ReqQryForQuote(
	const std::string& instrument_id, const std::string& exchange_id){
	CThostFtdcQryForQuoteField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	strcpy_s(a.ExchangeID, exchange_id.c_str());
	strcpy_s(a.InsertTimeStart, "");
	strcpy_s(a.InsertTimeEnd, "");
	strcpy_s(a.InvestUnitID, "");
	int b = m_pUserApi->ReqQryForQuote(&a, request_id++);
}

//请求查询报价
void ctp::CTdSpi::ReqQryQuote(const std::string& instrument_id, const std::string& exchange_id){
	CThostFtdcQryQuoteField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	strcpy_s(a.ExchangeID, exchange_id.c_str());
	strcpy_s(a.QuoteSysID, "");
	strcpy_s(a.InsertTimeStart, "");
	strcpy_s(a.InsertTimeEnd, "");
	strcpy_s(a.InvestUnitID, "");
	int b = m_pUserApi->ReqQryQuote(&a, request_id++);
}

///询价录入请求
void ctp::CTdSpi::ReqForQuoteInsert(
	const std::string& instrument_id, const std::string& exchange_id){
	CThostFtdcInputForQuoteField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	//strcpy_s(a.ForQuoteRef, "");
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	strcpy_s(a.ExchangeID, exchange_id.c_str());
	//strcpy_s(a.InvestUnitID, "");
	//strcpy_s(a.IPAddress, "");
	//strcpy_s(a.MacAddress, "");
	int b = m_pUserApi->ReqForQuoteInsert(&a, request_id++);
}

///做市商报价录入请求
void ctp::CTdSpi::ReqQuoteInsert(
	const std::string& instrument_id, const std::string& exchange_id){
choose:int choose_Flag;
	std::cin >> choose_Flag;

	if (choose_Flag != 1 && choose_Flag!=2){
		_getch();
		choose_Flag = NULL;
		goto choose;
	}

	int price_bid;
	std::cin >> price_bid;

	int price_ask;
	std::cin >> price_ask;
	std::string quoteref;
	std::cin >> quoteref;
	std::string AskOrderRef;
	std::string BidOrderRef;
	std::cin >> AskOrderRef;
	std::cin >> BidOrderRef;
	_getch();
	CThostFtdcInputQuoteField t = { 0 };
	strcpy_s(t.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(t.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(t.InstrumentID, instrument_id.c_str());
	strcpy_s(t.ExchangeID, exchange_id.c_str());
	
	strcpy_s(t.QuoteRef, quoteref.c_str());
	strcpy_s(t.UserID, p_broker->investor_id.c_str());
	t.AskPrice = price_ask;
	t.BidPrice = price_bid;
	t.AskVolume = 1;
	t.BidVolume = 1;
	if (choose_Flag ==1)
	{
		t.AskOffsetFlag = THOST_FTDC_OF_Open;///卖开平标志
		t.BidOffsetFlag = THOST_FTDC_OF_Open;///买开平标志
	}
	else if (choose_Flag ==2)
	{
		t.AskOffsetFlag = THOST_FTDC_OF_Close;///卖开平标志
		t.BidOffsetFlag = THOST_FTDC_OF_Close;///买开平标志
	}
	t.AskHedgeFlag = THOST_FTDC_HF_Speculation;///卖投机套保标志
	t.BidHedgeFlag = THOST_FTDC_HF_Speculation;///买投机套保标志

	strcpy_s(t.AskOrderRef, AskOrderRef.c_str());///衍生卖报单引用
	strcpy_s(t.BidOrderRef, BidOrderRef.c_str());///衍生买报单引用
	//strcpy_s(t.ForQuoteSysID, "");///应价编号
	//strcpy_s(t.InvestUnitID, "1");///投资单元代码
	int a = m_pUserApi->ReqQuoteInsert(&t, 1);
}

///报价通知
void ctp::CTdSpi::OnRtnQuote(CThostFtdcQuoteField *pQuote) 
{
	if (pQuote && strcmp(pQuote->InvestorID, p_broker->investor_id.c_str()) != 0)
	{
		return;
	}
	else
	{
		//CTraderSpi::OnRtnQuote(pQuote);
		//SetEvent(g_hEvent);
	}
}

//报价撤销
void ctp::CTdSpi::ReqQuoteAction()
{
	CThostFtdcInputQuoteActionField t = { 0 };
	strcpy_s(t.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(t.InvestorID, "00404");
	//strcpy_s(t.UserID, p_broker->investor_id.c_str());
	strcpy_s(t.ExchangeID, "SHFE");
	strcpy_s(t.QuoteRef, "           8");
	t.FrontID = 7;
	t.SessionID = 1879781311;
	t.ActionFlag = THOST_FTDC_AF_Delete;
	strcpy_s(t.InstrumentID, "cu1905C55000");
	int a = m_pUserApi->ReqQuoteAction(&t, 1);
	printf("m_pUserApi->ReqQuoteAction = [%d]", a);
}

//查询最大报单数量请求
void ctp::CTdSpi::ReqQueryMaxOrderVolume(const std::string& instrument_id)
{
	CThostFtdcQueryMaxOrderVolumeField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	a.Direction = THOST_FTDC_D_Buy;
	a.OffsetFlag = THOST_FTDC_OF_Open;
	a.HedgeFlag = THOST_FTDC_HF_Speculation;
	a.MaxVolume = 1;
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	int b = m_pUserApi->ReqQueryMaxOrderVolume(&a, request_id++);
}

//请求查询监控中心用户令牌
void ctp::CTdSpi::ReqQueryCFMMCTradingAccountToken()
{
	CThostFtdcQueryCFMMCTradingAccountTokenField a = { 0 };

}


///报单操作错误回报
void ctp::CTdSpi::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo)
{
	if (pOrderAction && strcmp(pOrderAction->InvestorID, p_broker->investor_id.c_str()) != 0)
	{
		return;
	}
	else
	{
		//CTraderSpi::OnErrRtnOrderAction(pOrderAction,pRspInfo);
		SetEvent(g_hEvent);
	}
}

///报单录入请求响应
void ctp::CTdSpi::OnRspOrderInsert(
	CThostFtdcInputOrderField *pInputOrder,
	CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast){
	if (pInputOrder && strcmp(pInputOrder->InvestorID, p_broker->investor_id.c_str()) != 0){
		return;
	}
	else{
		//CTraderSpi::OnRspOrderInsert(pInputOrder,pRspInfo,request_id,bIsLast);
	}
}

///报单录入错误回报
void ctp::CTdSpi::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{
	if (pInputOrder && strcmp(pInputOrder->InvestorID, p_broker->investor_id.c_str()) != 0)
	{
		return;
	}
	else
	{
		//CTraderSpi::OnErrRtnOrderInsert(pInputOrder, pRspInfo);
		SetEvent(g_hEvent);
	}
}

///报单通知
void ctp::CTdSpi::OnRtnOrder(CThostFtdcOrderField *pOrder){
	if (pOrder && strcmp(pOrder->InvestorID, p_broker->investor_id.c_str()) != 0){
		return;
	}
	else{
		//CTraderSpi::OnRtnOrder(pOrder);
		std::string status = ":not_set";
		if (pOrder->OrderStatus == THOST_FTDC_OST_AllTraded){ ///全部成交
			status = "2:all_traded";
			//SetEvent(g_hEvent);
		}
		else if (pOrder->OrderStatus == THOST_FTDC_OST_PartTradedQueueing) {///部分成交还在队列中
			status = "3:part_in_queue";
		}
		else if (pOrder->OrderStatus == THOST_FTDC_OST_PartTradedNotQueueing){///部分成交并撤单
			status = "1.4:part_trade_and_cancel";
		}
		else if (pOrder->OrderStatus == THOST_FTDC_OST_NoTradeQueueing){/// waiting
			status = "4:waiting";
		}
		else if (pOrder->OrderStatus == THOST_FTDC_OST_NoTradeNotQueueing){/// canceling
			status = "5:canceling";
		}
		else if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled){///撤单
			status = "1.5:canceld";
		}
		else if (pOrder->OrderStatus == THOST_FTDC_OST_Unknown){///未知
			status = "6:unknown";
		}
		else if (pOrder->OrderStatus == THOST_FTDC_OST_NotTouched){///尚未触发
			status = "7:not_touched";
		}
		else if (pOrder->OrderStatus == THOST_FTDC_OST_Touched){///已触发
			status = "8:touched";
		}
		auto sqlite = mgr::DBManager::Get()->GetSqlite("simnow");
		std::shared_ptr<SqliteRsp> rsp(sqlite->execute(
			"UPDATE order SET traded_vol=%d, status=%c WHERE id=%s",
			pOrder->VolumeTraded, status[0], pOrder->OrderRef));
		if (rsp->code) {
			log_error << "update order failed with code:" << rsp->code
				<< ", msg:" << rsp->message << ", order_id:" << pOrder->OrderRef;
		}
		ord::Direction dir;
		log_info << "PUBLIC_ORDER_TRADED|investor_id=" << p_broker->investor_id
			<< "|order_status=" << pOrder->OrderStatus << status
			<< "|instrument_id=" << pOrder->InstrumentID << "|instrument_id=" << pOrder->VolumeTraded
			<< "|account_id=" << pOrder->AccountID << "|order_ref=" << pOrder->OrderRef
			<< "|direction=" << GetDirection(dir, pOrder->Direction, pOrder->CombOffsetFlag[0]);
	}
}

///删除预埋单响应
void ctp::CTdSpi::OnRspRemoveParkedOrder(
	CThostFtdcRemoveParkedOrderField *pRemoveParkedOrder, 
	CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast){
	if (pRemoveParkedOrder && strcmp(pRemoveParkedOrder->InvestorID, p_broker->investor_id.c_str()) != 0){
		return;
	}
	else{
		//CTraderSpi::OnRspRemoveParkedOrder(pRemoveParkedOrder, pRspInfo, request_id, bIsLast);
		SetEvent(g_hEvent);
	}
}

///删除预埋撤单响应
void ctp::CTdSpi::OnRspRemoveParkedOrderAction(
	CThostFtdcRemoveParkedOrderActionField *pRemoveParkedOrderAction, 
	CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast){
	if (pRemoveParkedOrderAction && strcmp(pRemoveParkedOrderAction->InvestorID, p_broker->investor_id.c_str()) != 0)
	{
		return;
	}
	else {
		//CTraderSpi::OnRspRemoveParkedOrderAction(pRemoveParkedOrderAction, pRspInfo, request_id, bIsLast);
		SetEvent(g_hEvent);
	}
}

///预埋单录入请求响应
void ctp::CTdSpi::OnRspParkedOrderInsert(
	CThostFtdcParkedOrderField *pParkedOrder,
	CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast){
	if (pParkedOrder && strcmp(pParkedOrder->InvestorID, p_broker->investor_id.c_str()) != 0){
		return;
	}
	else{
		// strcpy_s(parked_order_id.c_str()1, pParkedOrder->ParkedOrderID);
		//CTraderSpi::OnRspParkedOrderInsert(pParkedOrder, pRspInfo, request_id, bIsLast);
		SetEvent(g_hEvent);
	}
}

///预埋撤单录入请求响应
void ctp::CTdSpi::OnRspParkedOrderAction(
	CThostFtdcParkedOrderActionField *pParkedOrderAction,
	CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast){
	if (pParkedOrderAction && strcmp(pParkedOrderAction->InvestorID, p_broker->investor_id.c_str()) != 0){
		return;
	}
	else{
		// strcpy_s(parked_order_action_id.c_str(), pParkedOrderAction->ParkedOrderActionID);
		//CTraderSpi::OnRspParkedOrderAction(pParkedOrderAction,pRspInfo,request_id,bIsLast);
		drop_order(pParkedOrderAction->OrderRef);
		SetEvent(g_hEvent);
	}
}

///请求查询预埋撤单响应
void ctp::CTdSpi::OnRspQryParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast)
{
	//CTraderSpi::OnRspQryParkedOrderAction(pParkedOrderAction, pRspInfo, request_id, bIsLast);
}

///请求查询预埋单响应
void ctp::CTdSpi::OnRspQryParkedOrder(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast)
{
	//CTraderSpi::OnRspQryParkedOrder(pParkedOrder,pRspInfo,request_id,bIsLast);
}

///请求查询报单响应
void ctp::CTdSpi::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast)
{
	/*if (pOrder) {
		vector_OrderSysID.push_back(pOrder->OrderSysID);
		vector_ExchangeID.push_back(pOrder->ExchangeID);
		vector_InstrumentID.push_back(pOrder->InstrumentID);
	}*/
	//CTraderSpi::OnRspQryOrder(pOrder,pRspInfo,request_id,bIsLast);
	// action_number++;
}

///执行宣告通知
void ctp::CTdSpi::OnRtnExecOrder(CThostFtdcExecOrderField *pExecOrder) 
{
	/*
	if (pExecOrder) {
		strcpy_s(order_ref.c_str(), pExecOrder->ExecOrderRef);
		strcpy_s(order_sys_id.c_str(), pExecOrder->ExecOrderSysID);
		g_NewFrontID = pExecOrder->FrontID;
		g_NewSessionID = pExecOrder->SessionID;
	}*/
	//CTraderSpi::OnRtnExecOrder(pExecOrder);
}

//期货发起查询银行余额请求
void ctp::CTdSpi::ReqQueryBankAccountMoneyByFuture()
{
	CThostFtdcReqQueryAccountField a = { 0 };
	int b = m_pUserApi->ReqQueryBankAccountMoneyByFuture(&a, request_id++);
}

//期货发起银行资金转期货请求
void ctp::CTdSpi::ReqFromBankToFutureByFuture()
{
	int output_num;
	std::cin >> output_num;

	CThostFtdcReqTransferField a = { 0 };
	strcpy_s(a.TradeCode, "202001");///业务功能码
int bankid = 0;
	while (bankid != 1 & 2 & 3 & 5 & 6 & 7 & 8 & 9 & 10 & 11 & 12 & 13 & 14 & 15 & 16) {
		std::cin >> bankid;
		if (bankid == 1 | 2 | 3 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16)
		{
			//strcpy_s(a.BankID, itoa(bankid, a.BankID, 10));///银行代码
			itoa(bankid, a.BankID, 10);
		}
		else
		{
			_getch();
		}
	}
	
	
	strcpy_s(a.BankBranchID, "0000");///期商代码
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.TradeDate, "20170829");///交易日期
	strcpy_s(a.TradeTime, "09:00:00");
	strcpy_s(a.BankSerial, "6889");///银行流水号
	strcpy_s(a.TradingDay, "20170829");///交易系统日期 
	a.PlateSerial = 5;///银期平台消息流水号
	a.LastFragment = THOST_FTDC_LF_Yes;///最后分片标志 '0'=是最后分片
	a.SessionID = session_id;
	//strcpy_s(a.CustomerName, "");///客户姓名
	a.IdCardType = THOST_FTDC_ICT_IDCard;///证件类型
	a.CustType = THOST_FTDC_CUSTT_Person;///客户类型
	//strcpy_s(a.IdentifiedCardNo, "310115198706241914");///证件号码
	/*strcpy_s(a.BankAccount, "123456789");
	strcpy_s(a.BankPassWord, "123456");///银行密码*/
	strcpy_s(a.BankAccount, "621485212110187");
	//strcpy_s(a.BankPassWord, "092812");///银行密码--不需要银行卡密码
	strcpy_s(a.AccountID, p_broker->investor_id.c_str());///投资者帐号
	//strcpy_s(a.Password, "092812");///期货密码--资金密码
	strcpy_s(a.Password, "123456");///期货密码--资金密码
	a.InstallID = 1;///安装编号
	a.FutureSerial = 0;///期货公司流水号
	a.VerifyCertNoFlag = THOST_FTDC_YNI_No;///验证客户证件号码标志
	strcpy_s(a.CurrencyID, "CNY");///币种代码
	a.TradeAmount = output_num;///转帐金额
	a.FutureFetchAmount = 0;///期货可取金额
	a.CustFee = 0;///应收客户费用
	a.BrokerFee = 0;///应收期货公司费用
	a.SecuPwdFlag = THOST_FTDC_BPWDF_BlankCheck;///期货资金密码核对标志
	a.RequestID = 0;///请求编号
	a.TID = 0;///交易ID
	int b = m_pUserApi->ReqFromBankToFutureByFuture(&a, 1);
}

//期货发起期货资金转银行请求
void ctp::CTdSpi::ReqFromFutureToBankByFuture()
{
	int output_num;
	std::cin >> output_num;

	CThostFtdcReqTransferField a = { 0 };
	strcpy_s(a.TradeCode, "202002");///业务功能码
	bankid_new:int bankid = 0;
	std::cin >> bankid;
	if (bankid == 1 | 2 | 3 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16)
	{
		//strcpy_s(a.BankID, itoa(bankid, a.BankID, 10));///银行代码
		itoa(bankid, a.BankID, 10);
	}
	else {
		_getch();
		goto bankid_new;
	}
	strcpy_s(a.BankBranchID, "0000");///期商代码
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	//strcpy_s(a.BankBranchID, "0000");///银行分支机构代码
	//strcpy_s(a.TradeDate, "20170829");///交易日期
	//strcpy_s(a.TradeTime, "09:00:00");
	//strcpy_s(a.BankSerial, "");///银行流水号
	//strcpy_s(a.TradingDay, "20170829");///交易系统日期 
	//a.PlateSerial= 0;///银期平台消息流水号
	a.LastFragment = THOST_FTDC_LF_Yes;///最后分片标志 '0'=是最后分片
	a.SessionID = session_id;
	//strcpy_s(a.CustomerName, "");///客户姓名
	a.IdCardType = THOST_FTDC_ICT_IDCard;///证件类型
	strcpy_s(a.IdentifiedCardNo, "310115198706241914");///证件号码
	strcpy_s(a.BankAccount, "123456789");///银行帐号
	//strcpy_s(a.BankPassWord, "123456");///银行密码
	strcpy_s(a.AccountID, p_broker->investor_id.c_str());///投资者帐号
	strcpy_s(a.Password, "123456");///期货密码
	a.InstallID = 1;///安装编号
	a.CustType = THOST_FTDC_CUSTT_Person;
	//a.FutureSerial = 0;///期货公司流水号
	a.VerifyCertNoFlag = THOST_FTDC_YNI_No;///验证客户证件号码标志
	strcpy_s(a.CurrencyID, "CNY");///币种代码
	a.TradeAmount = output_num;///转帐金额
	a.FutureFetchAmount = 0;///期货可取金额
	a.CustFee = 0;///应收客户费用
	a.BrokerFee = 0;///应收期货公司费用
	//a.SecuPwdFlag = THOST_FTDC_BPWDF_BlankCheck;///期货资金密码核对标志
	a.RequestID = 0;///请求编号
	a.TID = 0;///交易ID
	int b = m_pUserApi->ReqFromFutureToBankByFuture(&a, 1);
}

//期权自对冲录入请求
void ctp::CTdSpi::ReqOptionSelfCloseInsert(
	const std::string& instrument_id, const std::string& exchange_id){
	CThostFtdcInputOptionSelfCloseField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	strcpy_s(a.OptionSelfCloseRef, "1");
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	a.Volume = 1;
	
	int choose_1 = 0;
	while (choose_1 != 1 && choose_1 != 2 && choose_1 != 3 && choose_1 != 4) {
		std::cin >> choose_1;
		if (choose_1 == 1) { a.HedgeFlag = THOST_FTDC_HF_Speculation; }
		else if (choose_1 == 2) { a.HedgeFlag = THOST_FTDC_HF_Arbitrage; }
		else if (choose_1 == 3) { a.HedgeFlag = THOST_FTDC_HF_Hedge; }
		else if (choose_1 == 4) { a.HedgeFlag = THOST_FTDC_HF_MarketMaker; }
		else {
			_getch();
		}
	}
	
	int choose_2 = 0;
	while (choose_2 != 1 && choose_2 != 2 && choose_2 != 3) {
		std::cin >> choose_2;
		if (choose_2 == 1) { a.OptSelfCloseFlag = THOST_FTDC_OSCF_CloseSelfOptionPosition; }
		else if (choose_2 == 2) { a.OptSelfCloseFlag = THOST_FTDC_OSCF_ReserveOptionPosition; }
		else if (choose_2 == 3) { a.OptSelfCloseFlag = THOST_FTDC_OSCF_SellCloseSelfFuturePosition; }
		else {
			_getch();
			continue;
		}
	}

	strcpy_s(a.ExchangeID, exchange_id.c_str());
	std::string accountid_new;
	std::cin >> accountid_new;
	strcpy_s(a.AccountID, accountid_new.c_str());
	strcpy_s(a.CurrencyID, "CNY");
	int b = m_pUserApi->ReqOptionSelfCloseInsert(&a, 1);
}

///期权自对冲通知
void ctp::CTdSpi::OnRtnOptionSelfClose(
	CThostFtdcOptionSelfCloseField *pOptionSelfClose){
	if (pOptionSelfClose) {
		front_id = pOptionSelfClose->FrontID;
		session_id = pOptionSelfClose->SessionID;
		/* TODO
		strcpy_s(g_chOptionSelfCloseSysID, pOptionSelfClose->OptionSelfCloseSysID);//期权自对冲编号
		strcpy_s(g_chOptionSelfCloseRef, pOptionSelfClose->OptionSelfCloseRef);//期权自对冲引用
		*/
	}
	//CTraderSpi::OnRtnOptionSelfClose(pOptionSelfClose);
}

//期权自对冲操作请求
void ctp::CTdSpi::ReqOptionSelfCloseAction()
{
	CThostFtdcInputOptionSelfCloseActionField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	/* TODO
	//strcpy_s(a.OptionSelfCloseSysID, g_chOptionSelfCloseSysID);//期权自对冲编号
	strcpy_s(a.OptionSelfCloseRef, g_chOptionSelfCloseRef);//期权自对冲引用
	//a.FrontID = front_id;
	//a.SessionID = session_id;
	strcpy_s(a.ExchangeID, exchange_id.c_str());
	a.ActionFlag = THOST_FTDC_AF_Delete;
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	int b = m_pUserApi->ReqOptionSelfCloseAction(&a, 1);
	*/
}

//请求查询期权自对冲
void ctp::CTdSpi::ReqQryOptionSelfClose(const std::string& instrument_id, const std::string& exchange_id)
{
	CThostFtdcQryOptionSelfCloseField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	strcpy_s(a.ExchangeID, exchange_id.c_str());
	int b = m_pUserApi->ReqQryOptionSelfClose(&a, 1);
}

///请求查询期权自对冲响应
void ctp::CTdSpi::OnRspQryOptionSelfClose(
	CThostFtdcOptionSelfCloseField *pOptionSelfClose,
	CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast){
	if (pOptionSelfClose) {
		front_id = pOptionSelfClose->FrontID;
		session_id = pOptionSelfClose->SessionID;
		/* TODO
		strcpy_s(g_chOptionSelfCloseSysID, pOptionSelfClose->OptionSelfCloseSysID);//期权自对冲编号
		strcpy_s(g_chOptionSelfCloseRef, pOptionSelfClose->OptionSelfCloseRef);//期权自对冲引用
		*/
	}
	//CTraderSpi::OnRspQryOptionSelfClose(pOptionSelfClose, pRspInfo, request_id, bIsLast);
}

///请求查询执行宣告
void ctp::CTdSpi::ReqQryExecOrder(
	const std::string& instrument_id, const std::string exchange_id){
	CThostFtdcQryExecOrderField a = { 0 }; 
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.InstrumentID, instrument_id.c_str());
	strcpy_s(a.ExchangeID, exchange_id.c_str());
	strcpy_s(a.ExecOrderSysID, "");
	strcpy_s(a.InsertTimeStart, "");
	strcpy_s(a.InsertTimeEnd, "");
	int b = m_pUserApi->ReqQryExecOrder(&a, 1);
}

///查询二代资金账户
void ctp::CTdSpi::ReqQrySecAgentTradingAccount()
{
	CThostFtdcQryTradingAccountField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	strcpy_s(a.CurrencyID, "CNY");
	a.BizType = THOST_FTDC_BZTP_Future;
	strcpy_s(a.AccountID, p_broker->investor_id.c_str());
	int b = m_pUserApi->ReqQrySecAgentTradingAccount(&a, 1);
}

///请求查询二级代理商资金校验模式
void ctp::CTdSpi::ReqQrySecAgentCheckMode()
{
	CThostFtdcQrySecAgentCheckModeField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.InvestorID, p_broker->investor_id.c_str());
	int b = m_pUserApi->ReqQrySecAgentCheckMode(&a, 1);
}

///注册用户终端信息，用于中继服务器多连接模式
///需要在终端认证成功后，用户登录前调用该接口
void ctp::CTdSpi::RegisterUserSystemInfo()
{
	char pSystemInfo[344];
	int len;
	// CTP_GetSystemInfo(pSystemInfo, len);

	CThostFtdcUserSystemInfoField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	memcpy(a.ClientSystemInfo, pSystemInfo, len);
	a.ClientSystemInfoLen = len;

	/*string ip;
	ip.clear();
	std::cin.ignore();
	getline(std::cin, ip);
	strcpy_s(a.ClientPublicIP, ip.c_str());*/
	strcpy_s(a.ClientPublicIP, "192.168.0.1");//ip地址

	//int Port;
	//Port = 0;
	//std::cin.ignore();
	//std::cin >> Port;
	//a.ClientIPPort = Port;//端口号
	a.ClientIPPort = 51305;//端口号

	/*string LoginTime;
	LoginTime.clear();
	std::cin.ignore();
	getline(std::cin, LoginTime);
	strcpy_s(a.ClientPublicIP, LoginTime.c_str());*/
	strcpy_s(a.ClientLoginTime, "20190121");
	strcpy_s(a.ClientAppID, p_broker->app_id.c_str());
	int b = m_pUserApi->RegisterUserSystemInfo(&a);
}

///上报用户终端信息，用于中继服务器操作员登录模式
///操作员登录后，可以多次调用该接口上报客户信息
void ctp::CTdSpi::SubmitUserSystemInfo()
{
	char pSystemInfo[344];
	int len;
	// ??? CTP_GetSystemInfo(pSystemInfo, len);

	CThostFtdcUserSystemInfoField a = { 0 };
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	memcpy(a.ClientSystemInfo, pSystemInfo, len);
	a.ClientSystemInfoLen = len;

	/*string ip;
	ip.clear();
	std::cin.ignore();
	getline(std::cin, ip);
	strcpy_s(a.ClientPublicIP, ip.c_str());*/
	strcpy_s(a.ClientPublicIP, "192.168.0.1");//ip地址

	//int Port;
	//Port = 0;
	//std::cin.ignore();
	//std::cin >> Port;
	//a.ClientIPPort = Port;//端口号
	a.ClientIPPort = 51305;//端口号

	/*string LoginTime;
	LoginTime.clear();
	std::cin.ignore();
	getline(std::cin, LoginTime);
	strcpy_s(a.ClientPublicIP, LoginTime.c_str());*/
	strcpy_s(a.ClientLoginTime, "20190121");
	strcpy_s(a.ClientAppID, p_broker->app_id.c_str());
	int b = m_pUserApi->SubmitUserSystemInfo(&a);
}

///查询用户当前支持的认证模式
void ctp::CTdSpi::ReqUserAuthMethod()
{
	CThostFtdcReqUserAuthMethodField a = { 0 };
	strcpy_s(a.TradingDay, "20190308");
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	int b = m_pUserApi->ReqUserAuthMethod(&a, request_id++);
}

///用户发出获取图形验证码请求
void ctp::CTdSpi::ReqGenUserCaptcha()
{
	CThostFtdcReqGenUserCaptchaField a = { 0 };
	strcpy_s(a.TradingDay, "");
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	int b = m_pUserApi->ReqGenUserCaptcha(&a, request_id++);
}

///用户发出获取短信验证码请求
void ctp::CTdSpi::ReqGenUserText()
{
	CThostFtdcReqGenUserTextField a = { 0 };
	strcpy_s(a.TradingDay, "");
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	int b = m_pUserApi->ReqGenUserText(&a, request_id++);
}

///用户发出带有图片验证码的登陆请求
void ctp::CTdSpi::ReqUserLoginWithCaptcha()
{
	CThostFtdcReqUserLoginWithCaptchaField a = { 0 };
	strcpy_s(a.TradingDay, "");
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	strcpy_s(a.Password, p_broker->password.c_str());
	strcpy_s(a.UserProductInfo, "");
	strcpy_s(a.InterfaceProductInfo, "");
	strcpy_s(a.ProtocolInfo, "");//协议信息
	strcpy_s(a.MacAddress, "");//Mac地址
	strcpy_s(a.ClientIPAddress, "");//终端IP地址
	strcpy_s(a.LoginRemark, "");//登录主备
	strcpy_s(a.Captcha, "");//图形验证码的文字内容
	a.ClientIPPort = 10203;
	int b = m_pUserApi->ReqUserLoginWithCaptcha(&a, request_id++);
}

///用户发出带有短信验证码的登陆请求
void ctp::CTdSpi::ReqUserLoginWithText()
{
	CThostFtdcReqUserLoginWithTextField a = { 0 };
	strcpy_s(a.TradingDay, "");
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	strcpy_s(a.Password, p_broker->password.c_str());
	strcpy_s(a.UserProductInfo, "");
	strcpy_s(a.InterfaceProductInfo, "");
	strcpy_s(a.MacAddress, "");
	strcpy_s(a.ClientIPAddress, "");
	strcpy_s(a.LoginRemark, "");
	strcpy_s(a.Text, "");
	a.ClientIPPort = 10000;
	int b = m_pUserApi->ReqUserLoginWithText(&a, request_id++);
}

///用户发出带有动态口令的登陆请求
void ctp::CTdSpi::ReqUserLoginWithOTP()
{
	CThostFtdcReqUserLoginWithOTPField a = { 0 };
	strcpy_s(a.TradingDay, "");
	strcpy_s(a.BrokerID, p_broker->broker_id.c_str());
	strcpy_s(a.UserID, p_broker->investor_id.c_str());
	strcpy_s(a.Password, p_broker->password.c_str());
	strcpy_s(a.UserProductInfo, "");
	strcpy_s(a.InterfaceProductInfo, "");
	strcpy_s(a.MacAddress, "");
	strcpy_s(a.ClientIPAddress, "");
	strcpy_s(a.LoginRemark, "");
	strcpy_s(a.OTPPassword, "");
	a.ClientIPPort = 10000;
	int b = m_pUserApi->ReqUserLoginWithOTP(&a, request_id++);
}

///请求查询二级代理商信息
void ctp::CTdSpi::ReqQrySecAgentTradeInfo()
{
	CThostFtdcQrySecAgentTradeInfoField a = { 0 };
	strcpy_s(a.BrokerID, "");
	strcpy_s(a.BrokerSecAgentID, "");
	int b = m_pUserApi->ReqQrySecAgentTradeInfo(&a, request_id++);
}

namespace mgr {

	TraderManager* TraderManager::instance = nullptr;

	TraderManager* TraderManager::Get() {
		if (instance == nullptr) {
			static TraderManager ins;
			instance = &ins;
		}
		return instance;
	}

	TraderManager::~TraderManager() {
		// TdSpi clear
		for (auto&& o : _traders) {
			log_info << "release spi:" << o.first;
			if (o.second) {
				o.second->Release();
				o.second->Join();
				delete o.second;
				o.second = nullptr;
			}
		}
		_traders.clear();
		// MdSpi clear
		if (_mdspi) {
			_mdspi->Release();
			_mdspi->Join();
			delete _mdspi;
			_mdspi = nullptr;
		}
	}

	std::string TraderManager::trade(
		const std::string& spi_name, const std::string& instrument,
		bool close_yd_pos, double price, int volume, ord::Direction direction,
		ord::TradeType trade_type) {
		switch (trade_type) {
			// ordinary order
		case ord::TradeType::ORDINARY:
			return _traders[spi_name]->ReqOrderInsert(
				instrument, close_yd_pos, price, volume, direction);
			// pre-insert order
		case ord::TradeType::CONDITION:
			return _traders[spi_name]->ReqOrderPreInsert(
				instrument, close_yd_pos, price, volume, direction);
		default:
			log_error << "Unknown trade_type:" << int(trade_type);
		}
	}

	int TraderManager::add(ctp::CMdSpi* spi) {
		_mdspi = spi;
		return 0;
	}

	int TraderManager::add(const std::string& name, ctp::CTdSpi* spi) {
		_traders[name] = spi;
		return 0;
	}

	ctp::CMdSpi* TraderManager::get_mdspi() {
		return _mdspi;
	}

	ctp::CTdSpi* TraderManager::get(const std::string& name) {
		if (_traders.find(name) == _traders.end()) {
			return nullptr;
		}
		return _traders[name];
	}

};
