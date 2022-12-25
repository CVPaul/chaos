#ifndef __CTP_FRONT_DATA_TYPE_H__
#define __CTP_FRONT_DATA_TYPE_H__

#include <string>
#include <unordered_map>

namespace ctp{
    class BrokerInfo{
    public:
        std::string app_id;
        std::string td_front;
        std::string md_front;
		std::string password;
        std::string auth_code;
        std::string broker_id;
		std::string investor_id;
    public:
        BrokerInfo() {};
        BrokerInfo(std::unordered_map<std::string, std::string>&);
    };
}

namespace mgr {
	class BrokerManager {
	public:
		static BrokerManager* instance;
	private:
		BrokerManager() {}
	public:
		static BrokerManager* Get();
		~BrokerManager();
	public:
		ctp::BrokerInfo* add(const std::string& name);
		ctp::BrokerInfo* get(const std::string& name);
	private:
		std::unordered_map<std::string, ctp::BrokerInfo*> _brokers;
	};
}

#endif // __CTP_FRONT_DATA_TYPE_H__