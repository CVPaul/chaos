#pragma once

#include <chrono>
#include <iostream>
#include "sqlite.h"
#include "gtest/gtest.h"

constexpr char ORDER_TABLE[] = \
"CREATE TABLE IF NOT EXISTS orders(" \
"id INTEGER PRIMARY KEY AUTOINCREMENT," \
"instrument CHAR(16), exchange_id CHAR(8), " \
"direction TINYINT DEFAULT 0, status TINYINT DEFAULT 0, " \
"order_type TINYINT DEFAULT 0, price FLOAT(9,5), origin_vol INT, " \
"traded_vol INT DEFAULT 0, create_time INTEGER, update_time INTEGER)";


int64_t timestamp() {
	return std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
}


int sqlite_test() {
	Sqlite sqlite;
	std::string db = ":memory:";
	if (!sqlite.connect(db)){
		return -1;
	}/*
	std::shared_ptr<SqliteRsp> r1(sqlite.execute("DROP TABLE hello"));
	std::shared_ptr<SqliteRsp> r2(sqlite.execute("CREATE TABLE IF NOT EXISTS hello(greet CAHR(8), who CAHR(8));"));
	std::shared_ptr<SqliteRsp> r3(sqlite.execute("INSERT INTO hello(greet,who) VALUES('hello','world');"));
	std::shared_ptr<SqliteRsp> r4(sqlite.execute("INSERT INTO hello(greet,who) VALUES('hi', 'xqli');"));
	*/
	// std::cout << ORDER_TABLE << std::endl;
	std::shared_ptr<SqliteRsp> r1(sqlite.execute(ORDER_TABLE));
	std::shared_ptr<SqliteRsp> r2(sqlite.execute(
		"INSERT INTO orders(instrument, direction, price, origin_vol, create_time) "
		"VALUES('rb2212', 1, 3795, 10, %lld)", timestamp()
	));
	std::shared_ptr<SqliteRsp> r3(sqlite.execute(
		"INSERT INTO orders(instrument, direction, price, origin_vol, create_time) VALUES("
		"'%s', %d, %f, %d, %lld)", "IO2212", 2, 37995.282567, 7, timestamp()));
	std::shared_ptr<SqliteRsp> rsp(sqlite.execute("SELECT * FROM orders"));
	for (auto&& r: rsp->data) {
		for (auto&& c : r) {
			std::cout << c << ",";
		}
		std::cout << std::endl;
	}
	return 0;
}

TEST(sqlite_test, SqliteTest) {
	EXPECT_EQ(sqlite_test(), 0);
}