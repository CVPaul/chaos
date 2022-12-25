#pragma once

constexpr char ORDER_TABLE_NAME[] = "orders";
constexpr char ORDER_TABLE[] = \
"CREATE TABLE IF NOT EXISTS orders(" \
"id INTEGER PRIMARY KEY AUTOINCREMENT," \
"instrument CHAR(16), exchange_id CHAR(8), " \
"direction TINYINT DEFAULT 0, status TINYINT DEFAULT 0, " \
"order_type TINYINT DEFAULT 0, price FLOAT(9,5), origin_vol INT, " \
"traded_vol INT DEFAULT 0, create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP, " \
"update_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP);";
