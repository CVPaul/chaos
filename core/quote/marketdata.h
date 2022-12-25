#ifndef __CTP_MARKET_DATA_SPI_H__
#define __CTP_MARKET_DATA_SPI_H__

#include <atomic>
#include <vector>
#include <windows.h>
#include "ctp/ThostFtdcMdApi.h"

extern HANDLE xinhao;

namespace ctp{
	//行情类
	class CMdSpi : public CThostFtdcMdSpi
	{
	public:
		std::atomic_bool had_logged_in;
		std::atomic_bool had_connected;
		std::atomic_bool need_download;
	public:
		// 构造函数，需要一个有效的指向CThostFtdcMduserApi实例的指针
		CMdSpi(CThostFtdcMdApi* pUserApi);
		void Init() { m_pUserMdApi->Init(); }
		void Join() { m_pUserMdApi->Join(); }
		void Release() { m_pUserMdApi->Release(); }
		~CMdSpi();
		// 当客户端与交易托管系统建立起通信连接，客户端需要进行登录
		virtual void OnFrontConnected();

		void ReqUserLogin();

		// 当客户端与交易托管系统通信连接断开时，该方法被调用
		virtual void OnFrontDisconnected(int nReason);

		// 当客户端发出登录请求之后，该方法会被调用，通知客户端登录是否成功
		virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin,
			CThostFtdcRspInfoField* pRspInfo, int request_id, bool bIsLast);

		void SubscribeMarketData(const std::string& instruments);//收行情

		///订阅行情应答
		virtual void OnRspSubMarketData(
			CThostFtdcSpecificInstrumentField* pSpecificInstrument,
			CThostFtdcRspInfoField* pRspInfo, int request_id, bool bIsLast);

		///深度行情通知
		virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData);

		///订阅询价请求
		void SubscribeForQuoteRsp(const std::string& instrument_id);

		///订阅询价应答
		virtual void OnRspSubForQuoteRsp(
			CThostFtdcSpecificInstrumentField* pSpecificInstrument,
			CThostFtdcRspInfoField* pRspInfo, int request_id, bool bIsLast);


		///询价通知
		virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField* pForQuoteRsp);

	private:
		// 指向CThostFtdcMduserApi实例的指针
		CThostFtdcMdApi *m_pUserMdApi;
	};
}
#endif // __TCP_MARKET_DATA_SPI_H__