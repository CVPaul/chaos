// lightio.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#ifndef __IO_LIGHTIO_H__
#define __IO_LIGHTIO_H__


#include <list>
#include <string>
#include <iostream>

#include "tick.pb.h"

namespace io {
	dat::TickData line2tick(const std::string& line);
	int log2bin(const std::string& flog, const std::string& fbin);
	int bin2tick(const std::string& filename, std::list<dat::TickData>&);
	int log2tick(const std::string& filename, std::list<dat::TickData>&);
	int tick2bin(const std::list<dat::TickData>&, const std::string& filename);
};


#endif // __IO_LIGHTIO_H__
// TODO: 在此处引用程序需要的其他标头。
