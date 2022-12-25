#include "dbmgr.h"
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
		Sqlite* p = new Sqlite();
		if (p == nullptr) {
			return false;
		}
		p->connect(path);
		_sqlites[name] = p;
		return true;
	}
}