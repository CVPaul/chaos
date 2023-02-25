#pragma once

#include <string>
#include <vector>
#include <iostream>
#include "sqlite3.h"

class SqliteRsp {
public:

	SqliteRsp();
	~SqliteRsp();
	void append(int row_sz=0);
	void append(const std::string&& val);
	bool reset();

public:
	int code;
	char* message;
	std::string sql;
	std::vector<std::vector<std::string>> data;
};

class Sqlite {
public:
	Sqlite();
	~Sqlite();
public:
	std::shared_ptr<SqliteRsp> execute(
		const char* sql, ...);
	int last_insert_rowid();
	bool connect(const std::string& db, float timeout = 10.0);
	bool commit();
	bool close();
public:
	std::string db;
private:
	sqlite3* _conn;
};
