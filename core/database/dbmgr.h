#pragma once

#include <map>
#include "sqlite/sqlite.h"

namespace mgr {
	
	class DBManager {
	private:
		DBManager() {}
	public:
		~DBManager();
		static DBManager* Get();
		Sqlite* GetSqlite(const std::string& name);
		bool RegisterSqlite(const std::string& name, const std::string& path);
	private:
		static DBManager* _instance;
		std::map<std::string, Sqlite*> _sqlites;
	};
}