#include "dbmgr.h"
#include "tables.h"
#include "util/logging.h"

namespace mgr {
	DBManager* DBManager::DBManager::_instance = nullptr;
	DBManager* DBManager::Get() {
		if (_instance == nullptr) {
			static DBManager ins;
			_instance = &ins;
		}
		return _instance;
	}

	DBManager::~DBManager() {
		for (auto&& p : _sqlites) {
			if (p.second)
				delete p.second;
		}
		_sqlites.clear();
	}

	Sqlite* DBManager::GetSqlite(const std::string& name) {
		auto p = _sqlites.find(name);
		if (p == _sqlites.end()) {
			log_error << "can not get sqlite with name:" << name;
			return nullptr;
		}
		return p->second;
	}

	bool DBManager::RegisterSqlite(
		const std::string& name, const std::string& path) {
		// the function body
		if (_sqlites.find(name) != _sqlites.end())
			return true;
		Sqlite* p = new Sqlite();
		if (p == nullptr) {
			return false;
		}
		if(!p->connect(path)){
			log_fatal << "connect orderbook database for " << name;
			delete p;
			return false;
		}
		p->execute(ORDER_TABLE_CREATE);
		_sqlites[name] = p;
		return true;
	}
}