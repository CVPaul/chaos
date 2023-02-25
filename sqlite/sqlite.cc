#include "sqlite.h"
#include <stdarg.h>

SqliteRsp::~SqliteRsp() {
	reset();
}

SqliteRsp::SqliteRsp() 
	:code(SQLITE_OK)
	, message(nullptr) {
}

bool SqliteRsp::reset() {
	if (message) {
		sqlite3_free(message);
		message = nullptr;
	}
	code = 0;
	data.clear();
	return true;
}

void SqliteRsp::append(int sz) {
	data.push_back(std::vector<std::string>(sz));
}

void SqliteRsp::append(const std::string&& val) {
	int size = data.size() - 1;
	data[size].push_back(val);
}

// =====================================================================
Sqlite::Sqlite():_conn(nullptr) {}

Sqlite::~Sqlite() { if (_conn) close(); }

int Sqlite::last_insert_rowid() {
	return sqlite3_last_insert_rowid(_conn);
}

bool Sqlite::close() {
	if (_conn) {
		int code = sqlite3_close(_conn);
		if (SQLITE_OK != code){
			std::cerr << "close connection to " << db << " failed!";
			return false;
		}
		_conn = nullptr;
	}
	db = "";
	return true;
}

bool Sqlite::connect(const std::string& db, float timeout) {
	if (_conn) {
		std::cerr << "last connection is not closed, please close it first!" << std::endl;
		return false;
	}
	int code = sqlite3_open(db.c_str(), &_conn);
	if (code != SQLITE_OK) {
		std::cerr << "connect to " << db << " failed with code:"
			<< code << std::endl;
		return false;
	}
	// set time out
	// sqlite3_busy_handler(_conn, int(*)(void*, int), void*)
	// sqlite3_busy_timeout(_conn, timeou*100);
	this->db = db;
	return true;
}

int callback(void* para, int cnt, char** val, char** name) {
	SqliteRsp* rsp = (SqliteRsp*)para;
	rsp->append(cnt);
	int sz = rsp->data.size() - 1;
	for (int i = 0; i < cnt; i++) {
		if (val[i] != nullptr)
			rsp->data[sz][i] = val[i];
	}
	return 0;
}

std::shared_ptr<SqliteRsp> Sqlite::execute(const char* sql, ...) {
	SqliteRsp* rsp = new SqliteRsp();
	const size_t bufsz = 512;
	char buffer[bufsz];
	va_list args;
	va_start(args, sql);
	vsprintf_s(buffer, bufsz, sql, args);
	va_end(args);
	rsp->sql = buffer;
	rsp->code = sqlite3_exec(
		_conn, rsp->sql.c_str(), callback, rsp, &(rsp->message));
	if (rsp->code != SQLITE_OK) {
		std::cerr << "execute with sql:" << rsp->sql
			<< " failed with code: " << rsp->code << " and message: "
			<< rsp->message << std::endl;
	}
	return std::shared_ptr<SqliteRsp>(rsp);
}

bool Sqlite::commit() {
	SqliteRsp rsp;
	rsp.code = sqlite3_exec(_conn, "COMMIT;", NULL, NULL, &rsp.message);
	if (rsp.code != SQLITE_OK) {
		std::cerr << "commit failed with message:"
			<< rsp.message << std::endl;
		if (rsp.message) {
			sqlite3_free(rsp.message);
			rsp.message = nullptr;
		}
		return false;
	}
	return true;
}
