#ifndef __CTP_TRADER_SPI_H__
#define __CTP_TRADER_SPI_H__

#include <map>
#include <mutex>
#define NOMINMAX
#include <thread>
#include <chrono>
#include <Windows.h>
#include <unordered_map>

#include "broker/front.h"
#include "util/logging.h"
#include "common/types.h"
#include "database/dbmgr.h"
#include "quote/marketdata.h"
#include "manager/instrument.h"
#include "ctp/ThostFtdcTraderApi.h"

extern HANDLE g_hEvent;

namespace ctp {
	//trader class
	class CTdSpi : public CThostFtdcTraderSpi{
	public:
		BrokerInfo* p_broker;
		std::atomic_bool had_logged_in;
		std::atomic_bool had_connected;
		std::atomic_uint64_t order_ref_id;
		int request_id, session_id, front_id;
		// std::map<std::string, ord::Order*> orderbook;
	public:
		CTdSpi(CThostFtdcTraderApi* pUserApi, BrokerInfo* pBroker);
		void Init() { m_pUserApi->Init(); }
		void Join() { m_pUserApi->Join(); }
		void Release() { m_pUserApi->Release(); }
		~CTdSpi();

		virtual void OnFrontConnected();

		// convert
		void ConvDirection(
			bool close_yd_pos,
			const ord::Direction direction,
			TThostFtdcDirectionType& ctp_direction,
			TThostFtdcOffsetFlagType& ctp_action);

		std::string GetDirection(
			ord::Direction& direction,
			const TThostFtdcDirectionType ctp_direction,
			const TThostFtdcOffsetFlagType ctp_action);

		ord::Direction ConvDirection(
			const TThostFtdcDirectionType ctp_direction,
			const TThostFtdcOffsetFlagType ctp_action);

		// OrderRef
		std::string GetOrderRef();
		
		// OrderId(self-define)
		std::string GetOrderId(const std::string& order_ref);

		// order helper
		void drop_order(const std::string& order_ref) {
			
		}

		// get trade condition
		char get_trade_condition(const ord::Direction direction) {
			if (direction == ord::Direction::BUY || direction == ord::Direction::BUYTOCOVER) {
				return THOST_FTDC_CC_LastPriceGreaterEqualStopPrice;
			}
			else if (direction == ord::Direction::SELLSHORT || direction == ord::Direction::SELL) {
				return THOST_FTDC_CC_LastPriceLesserEqualStopPrice;
			}
			else {
				return '0'; // invalid condition
			}
		}

		//客户端认证
		void ReqAuthenticate();

		///客户端认证响应
		virtual void OnRspAuthenticate(
			CThostFtdcRspAuthenticateField *pRspAuthenticateField,
			CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast);

		void RegisterFensUserInfo();

		virtual void OnFrontDisconnected(int nReason);

		void ReqUserLogin();

		virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
			CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast);

		void ReqUserLogout();

		///登出请求响应
		virtual void OnRspUserLogout(
			CThostFtdcUserLogoutField *pUserLogout, 
			CThostFtdcRspInfoField *pRspInfo,
			int request_id, bool bIsLast);

		///请求确认结算单
		void ReqSettlementInfoConfirm();
		

		///投资者结算结果确认响应
		virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
			CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast);
		

		///用户口令更新请求
		void ReqUserPasswordUpdate();
		

		///用户口令更新请求响应
		virtual void OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField *pUserPasswordUpdate, CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast);
		

		///资金账户口令更新请求
		void ReqTradingAccountPasswordUpdate();
		

		///资金账户口令更新请求响应
		virtual void OnRspTradingAccountPasswordUpdate(CThostFtdcTradingAccountPasswordUpdateField *pTradingAccountPasswordUpdate, CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast);
		

		///预埋单录入//限价单
		std::string ReqParkedOrderInsert(
			ord::Direction direction, bool close_yd_pos,
			double limit_price, int volume,
			const std::string& instrument_id,
			const std::string& exchange_id);

		///预埋撤单录入请求
		void ReqParkedOrderAction(
			const std::string& order_sys_id,
			const std::string& instrument_id, const std::string& exchange_id);

		///请求删除预埋单
		void ReqRemoveParkedOrder(const std::string& parked_order_id);
		

		///请求删除预埋撤单
		void ReqRemoveParkedOrderAction(const std::string& parked_order_action_id);
		
		///报单录入请求
		std::string ReqOrderInsert(
			const std::string& strategy,
			const std::string& instrument_id, bool close_yd_pos,
			float price, int volume, ord::Direction direction);

		///大商所止损单
		void ReqOrderInsert_Touch(
			const std::string& instrument_id, const std::string& exchange_id);

		///大商所止盈单
		void ReqOrderInsert_TouchProfit(
			const std::string& instrument_id, const std::string& exchange_id);

		//全成全撤
		void ReqOrderInsert_VC_CV(
			const std::string& instrument_id, const std::string& exchange_id);

		//部成部撤
		void ReqOrderInsert_VC_AV(
			const std::string& instrument_id, const std::string& exchange_id);

		//市价单
		void ReqOrderInsert_AnyPrice(
			const std::string& instrument_id, const std::string& exchange_id);

		//市价转限价单(中金所);
		void ReqOrderInsert_BestPrice(
			const std::string& instrument_id, const std::string& exchange_id);

		//套利指令
		void ReqOrderInsert_Arbitrage(
			const std::string& instrument_id, const std::string& exchange_id);

		//互换单
		void ReqOrderInsert_IsSwapOrder(
			const std::string& instrument_id, const std::string& exchange_id);

		///报单操作请求
		void ReqOrderAction_Ordinary(
			const std::string& order_sys_id, const std::string& order_ref,
			const std::string& instrument_id, const std::string& exchange_id);

		///执行宣告录入请求
		void ReqExecOrderInsert(
			int action,
			const std::string& instrument_id, const std::string& exchange_id);

		///执行宣告操作请求
		void ReqExecOrderAction(
			const std::string& order_sys_id, const std::string& order_ref,
			const std::string& instrument_id, const std::string& exchange_id);

		//批量报单操作请求
		void ReqBatchOrderAction();

		///请求查询报单
		void ReqQryOrder(const std::string& exchange_id);

		///报单录入请求
		std::string ReqOrderPreInsert(
			const std::string& instrument_id, bool close_yd_pos,
			float price, int volume, ord::Direction direction);

		///报单操作请求
		void ReqOrderAction(const std::string& order_ref);

		//撤销查询的报单
		void ReqOrderAction_forqry(int action_num);
		

		///请求查询成交
		void ReqQryTrade();
		

		///请求查询预埋单
		void ReqQryParkedOrder(const std::string& exchange_id);
		

		//请求查询服务器预埋撤单
		void ReqQryParkedOrderAction(
			const std::string& instrument_id, const std::string& exchange_id);

		//请求查询资金账户
		void ReqQryTradingAccount();
		

		//请求查询投资者持仓
		void ReqQryInvestorPosition();
		

		//请求查询投资者持仓明细
		void ReqQryInvestorPositionDetail();
		

		//请求查询交易所保证金率
		void ReqQryExchangeMarginRate(const std::string& instrument_id);
		

		//请求查询合约保证金率
		void ReqQryInstrumentMarginRate(const std::string& instrument_id);
		

		//请求查询合约手续费率
		void ReqQryInstrumentCommissionRate(const std::string& instrument_id);
		

		//请求查询做市商合约手续费率
		void ReqQryMMInstrumentCommissionRate(const std::string& instrument_id);
		

		//请求查询做市商期权合约手续费
		void ReqQryMMOptionInstrCommRate(const std::string& instrument_id);
		

		//请求查询报单手续费
		void ReqQryInstrumentOrderCommRate(const std::string& instrument_id);
		

		//请求查询期权合约手续费
		void ReqQryOptionInstrCommRate();
		

		//请求查询合约
		void ReqQryInstrument(
			const std::string& instrument_id, const std::string& exchange_id);

		///请求查询合约响应
		virtual void OnRspQryInstrument(
			CThostFtdcInstrumentField *pInstrument,
			CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast);

		//请求查询投资者结算结果
		void ReqQrySettlementInfo();
		

		//请求查询转帐流水
		void ReqQryTransferSerial(const std::string& exchange_id);
		

		//请求查询产品
		void ReqQryProduct(const std::string& exchange_id);
		

		//请求查询转帐银行
		void ReqQryTransferBank();
		

		//请求查询交易通知
		void ReqQryTradingNotice();
		

		//请求查询交易编码
		void ReqQryTradingCode(const std::string& exchange_id);
		

		//请求查询结算信息确认
		void ReqQrySettlementInfoConfirm();
		

		//请求查询产品组
		void ReqQryProductGroup();
		

		//请求查询投资者单元
		void ReqQryInvestUnit();
		

		//请求查询经纪公司交易参数
		void ReqQryBrokerTradingParams();
		

		//请求查询询价
		void ReqQryForQuote(
			const std::string& instrument_id, const std::string& exchange_id);

		//请求查询报价
		void ReqQryQuote(const std::string& instrument_id, const std::string& exchange_id);

		///询价录入请求
		void ReqForQuoteInsert(
			const std::string& instrument_id, const std::string& exchange_id);

		///做市商报价录入请求
		void ReqQuoteInsert(
			const std::string& instrument_id, const std::string& exchange_id);

		///报价通知
		virtual void OnRtnQuote(CThostFtdcQuoteField* pQuote);
		

		//报价撤销
		void ReqQuoteAction();
		

		//查询最大报单数量请求
		void ReqQueryMaxOrderVolume(const std::string& instrument_id);
		

		//请求查询监控中心用户令牌
		void ReqQueryCFMMCTradingAccountToken();
		

		

		///报单操作错误回报
		virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo);
		

		///报单录入请求响应
		virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo,
			int request_id, bool bIsLast);
		

		///报单录入错误回报
		virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo);
		
		/// 撤单通知

		virtual void OnRspOrderAction(
			CThostFtdcInputOrderActionField *pInputOrderAction,
			CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
		///报单通知
		virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);
		

		///删除预埋单响应
		virtual void OnRspRemoveParkedOrder(
			CThostFtdcRemoveParkedOrderField *pRemoveParkedOrder, 
			CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast);

		///删除预埋撤单响应
		virtual void OnRspRemoveParkedOrderAction(
			CThostFtdcRemoveParkedOrderActionField *pRemoveParkedOrderAction, 
			CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast);

		///预埋单录入请求响应
		virtual void OnRspParkedOrderInsert(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo,
			int request_id, bool bIsLast);
		

		///预埋撤单录入请求响应
		virtual void OnRspParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction, CThostFtdcRspInfoField *pRspInfo,
			int request_id, bool bIsLast);
		

		///请求查询预埋撤单响应
		virtual void OnRspQryParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast);
		

		///请求查询预埋单响应
		virtual void OnRspQryParkedOrder(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast);
		

		///请求查询报单响应
		virtual void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast);
		

		///执行宣告通知
		virtual void OnRtnExecOrder(CThostFtdcExecOrderField* pExecOrder);
		

		//期货发起查询银行余额请求
		void ReqQueryBankAccountMoneyByFuture();
		

		//期货发起银行资金转期货请求
		void ReqFromBankToFutureByFuture();
		

		//期货发起期货资金转银行请求
		void ReqFromFutureToBankByFuture();
		

		//期权自对冲录入请求
		void ReqOptionSelfCloseInsert(
			const std::string& instrument_id, const std::string& exchange_id);

		///期权自对冲通知
		virtual void OnRtnOptionSelfClose(
			CThostFtdcOptionSelfCloseField *pOptionSelfClose);

		//期权自对冲操作请求
		void ReqOptionSelfCloseAction();
		

		//请求查询期权自对冲
		void ReqQryOptionSelfClose(const std::string& instrument_id, const std::string& exchange_id);
		

		///请求查询期权自对冲响应
		virtual void OnRspQryOptionSelfClose(
			CThostFtdcOptionSelfCloseField *pOptionSelfClose,
			CThostFtdcRspInfoField *pRspInfo, int request_id, bool bIsLast);

		///请求查询执行宣告
		void ReqQryExecOrder(
			const std::string& instrument_id, const std::string exchange_id);

		///查询二代资金账户
		void ReqQrySecAgentTradingAccount();
		

		///请求查询二级代理商资金校验模式
		void ReqQrySecAgentCheckMode();
		

		///注册用户终端信息，用于中继服务器多连接模式
		///需要在终端认证成功后，用户登录前调用该接口
		void RegisterUserSystemInfo();
		

		///上报用户终端信息，用于中继服务器操作员登录模式
		///操作员登录后，可以多次调用该接口上报客户信息
		void SubmitUserSystemInfo();
		

		///查询用户当前支持的认证模式
		void ReqUserAuthMethod();
		

		///用户发出获取图形验证码请求
		void ReqGenUserCaptcha();
		

		///用户发出获取短信验证码请求
		void ReqGenUserText();
		

		///用户发出带有图片验证码的登陆请求
		void ReqUserLoginWithCaptcha();
		

		///用户发出带有短信验证码的登陆请求
		void ReqUserLoginWithText();
		

		///用户发出带有动态口令的登陆请求
		void ReqUserLoginWithOTP();
		

		///请求查询二级代理商信息
		void ReqQrySecAgentTradeInfo();

	private:
		CThostFtdcTraderApi *m_pUserApi;
		std::thread _worker;
	};
}

namespace mgr {
	class TraderManager {
	
	private:
		ctp::CMdSpi* _mdspi;
		std::unordered_map<std::string, ctp::CTdSpi*> _traders;
	public:
		static TraderManager* instance;
	private:
		TraderManager() {}
	public:
		static TraderManager* Get();
		~TraderManager();
	public:

		std::string trade(
			const std::string& strategy,
			const std::string& spi_name, const std::string& instrument,
			bool close_yd_pos, double price, int volume,
			ord::Direction direction, ord::TradeType);

		int add(ctp::CMdSpi* spi);
		int add(const std::string& name, ctp::CTdSpi* spi);

		ctp::CMdSpi* get_mdspi();
		ctp::CTdSpi* get(const std::string& name);
	};
}

#endif // __CTP_TRADER_SPI_H__