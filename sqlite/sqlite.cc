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
	if (message != nullptr) {
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
Sqlite::Sqlite()
	: _buf_sz(0)
	,_conn(nullptr) {
}

Sqlite::~Sqlite() {
	if (_conn)
		close();
	if (_buffer) {
		delete[]_buffer;
		_buffer = nullptr;
		_buf_sz = 0;
	}
}

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
	if (_buffer) {
		delete[]_buffer;
		_buffer = nullptr;
		_buf_sz = 0;
	}
	return true;
}

bool Sqlite::reset(int buf_sz) {
	if (buf_sz == _buf_sz)
		return true;
	// release
	if (_buffer) {
		delete[]_buffer;
		_buffer = nullptr;
		_buf_sz = 0;
	}
	// allocate
	_buffer = new char[DEFAULT_SQL_BUFFER_SIZE];
	if (_buffer == nullptr) {
		std::cerr << "malloc buffer failed with size=1024!";
		return false;
	}
	_buf_sz = buf_sz;
	return true;
}

bool Sqlite::connect(const std::string& db, float timeout) {
	if (_conn) {
		std::cerr << "last connection is not closed, please close it first!" << std::endl;
		return false;
	}
	_buffer = new char[DEFAULT_SQL_BUFFER_SIZE];
	if (_buffer == nullptr) {
		std::cerr << "malloc buffer failed with size=1024!";
		return false;
	}
	_buf_sz = DEFAULT_SQL_BUFFER_SIZE;
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

SqliteRsp* Sqlite::execute(const char* sql, ...) {
	SqliteRsp* rsp = new SqliteRsp();
	va_list args;
	va_start(args, sql);
	vsprintf_s(_buffer, _buf_sz, sql, args);
	va_end(args);
	/* vsprintf ÎÛÈ¾ÁËhead£¬ÎÞ·¨delete
	if (sql_sz > _buf_sz) {
		int old_buf_sz = _buf_sz;
		std::cerr << "[warning]sqlite buffer size(" << old_buf_sz
			<< ") is smaller than sql query length(" << sql_sz << ")." 
			<< "sqlite will auto extand buffer with local scope to "
			<< sql_sz << " and shrink to origin size:" << old_buf_sz 
			<< " after successfully execute this command." << std::endl;
		reset(sql_sz + 1);
		delete rsp;
		rsp = execute(sql);
		reset(old_buf_sz);
		return rsp;
	}*/
	rsp->code = sqlite3_exec(
		_conn, _buffer, callback, rsp, &rsp->message);
	if (rsp->code != SQLITE_OK) {
		std::cerr << "execute with sql:" << sql
			<< " failed with message:"
			<< rsp->message << std::endl;
		sqlite3_free(rsp->message);
	}
	return rsp;
}

bool Sqlite::commit() {
	SqliteRsp rsp;
	rsp.code = sqlite3_exec(_conn, "COMMIT;", NULL, NULL, &rsp.message);
	if (rsp.code != SQLITE_OK) {
		std::cerr << "commit failed with message:"
			<< rsp.message << std::endl;
		sqlite3_free(rsp.message);
		return false;
	}
	return true;
}
