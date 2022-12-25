// #include "stdafx.h"
#include <iostream>
#include <algorithm>
#include <unordered_map>

#include "util/config.h"
#include "util/logging.h"
#include "broker/front.h"

namespace ctp {
	BrokerInfo::BrokerInfo(
		std::unordered_map<std::string, std::string>& sec) {
		// function body
		this->app_id = sec["app_id"];
		this->td_front = sec["td_front"];
		this->md_front = sec["md_front"];
		this->password = sec["password"];
		this->auth_code = sec["auth_code"];
		this->broker_id = sec["broker_id"];
		this->investor_id = sec["investor_id"];
	}
}

namespace mgr {
	BrokerManager* BrokerManager::instance = nullptr;
	BrokerManager* BrokerManager::Get() {
		if (instance == nullptr) {
			static BrokerManager ins;
			instance = &ins;
		}
		return instance;
	}
	BrokerManager::~BrokerManager() {
		for (auto&& b : _brokers) {
			if (b.second) {
				delete b.second;
				b.second = nullptr;
			}
		}
		_brokers.clear();
	}
	ctp::BrokerInfo* BrokerManager::add(const std::string& name) {
		if (_brokers.find(name) != _brokers.end()) {
			log_error << "broker with name:" << name << "is already exists!";
			return _brokers[name];
		}
		auto section = Config::Get()->get_section(name);
		if (section.empty())
			return nullptr;
		_brokers[name] = new ctp::BrokerInfo(section);
		return _brokers[name];
	}
	ctp::BrokerInfo* BrokerManager::get(const std::string& name) {
		if (_brokers.find(name) == _brokers.end()) {
			log_error << "broker with name:" << name << "does not exists!";
			return nullptr;
		}
		return _brokers[name];
	}
}
