#pragma once

#include <string>
#include <vector>
#include <iostream>
#include "sqlite3.h"

constexpr int DEFAULT_SQL_BUFFER_SIZE = 512;

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
	std::vector<std::vector<std::string>> data;
};

class Sqlite {
public:
	Sqlite();
	~Sqlite();
public:
	SqliteRsp* execute(
		const char* sql, ...);
	int last_insert_rowid();
	bool reset(int buf_size);
	bool connect(const std::string& db, float timeout = 10.0);
	bool commit();
	bool close();
public:
	std::string db;
private:
	int _buf_sz;
	char* _buffer;
	sqlite3* _conn;
};
