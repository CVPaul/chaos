#ifndef __COMMON_TYPES_H__
#define __COMMON_TYPES_H__

#include <string>

#include "ctp/ThostFtdcUserApiDataType.h"

namespace ord {
    enum class Direction{
        _MIN,
        BUY,
        SELL,
        SELLSHORT,
        BUYTOCOVER,
        _MAX,
    };

    enum class Status{
        _MIN,
        INIT,
        PARKED,
        UNFILL,
        FILLING,
        CANCELLED,
        FINISHED,
        _MAX,
    };

    enum class Action{
        _MIN,
        CANCEL,
        MODIFY,
        _MAX,
    };
    
    enum class TradeType {
        _MIN,
        ORDINARY,
        CONDITION,
        PARKED,
        _MAX,
    };

    struct Order{
        double price;
        int volume, filled_vol;

        Status status;
        Direction direction;
        std::string order_id;
        std::string instrument_id;
        std::string exchange_id;
        /* 唯一确定一组报单的方案:
            1) 报单时用FrontID + SessionID + OrderRef维护本地报单，
            当第2个OnRtnOder回来后，找到原始报单，将OrderSysID填入到
            本地报单中，后面都用ExchangeID + OrderSysID维护订单
            2) 本地维护OrderRef字段，当前交易日内不管多少链接都一直单调递增，
            这样就可以不管FrontID 和 SessionID ，每次根据OrderRef就可
            以确定唯一报单。但这对多策略多链接交易开发可能存在问题，因为涉
            及到多个API链接间OrderRef字段的互相同步。
            注：OrderRef用来标识报单，OrderActionRef用来标识标撤单。CTP量
            化投资API要求报单的OrderRef/OrderActionRef字段在同一线程内
            必须是递增的，长度不超过13的数字字符串。
        */
        // order_ref
        Order(){
            status = Status::INIT;
            direction = Direction::BUY;
            exchange_id = "";

            price = 0;
            volume = filled_vol = 0;
        }

        std::string to_string(){
                char buffer[512];
                sprintf(buffer, 
                    "instrument_id=%s|volume=%d|direction=%d|price=%.3f"
                    "|filled_vol=%d|exchange_id=%s|status=%d|",
                    instrument_id.c_str(), volume, direction, price,
                    filled_vol, exchange_id.c_str(), status);
                return buffer;
        }
    };
}
#endif // __COMMON_TYPES_H__