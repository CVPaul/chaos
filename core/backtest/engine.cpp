#include <zlib.h>
#include <ranges>
#include <sstream>
#include <fstream>
#include <algorithm>

#include "util/config.h"
#include "util/common.h"
#include "util/logging.h"
#include "data/structs.h"
#include "backtest/engine.h"
#include "manager/strategy.h"
#include "datetime/datetime.h"

namespace bt {
// backtest namespace
BackTestEngine::BackTestEngine() {}

BackTestEngine::~BackTestEngine(){}

int BackTestEngine::init(const std::string& stg_name) {
	std::string start_date;
	auto sec = mgr::Config::Get()->get_section("backtest");
	if (stg_name.empty())
		start_date = sec["start_date"];
	else {
		start_date = (dt::now() - std::stoi(
			mgr::Config::Get()->get(stg_name + "::lookback_days").c_str())
		).to_string("%Y%m%d");
	}
	int count = util::glob(sec["history_data"], _source_files);
	std::sort(_source_files.begin(), _source_files.end());
	for (int i = 0; i < _source_files.size(); i++) {
		auto tokens = util::split(_source_files[i], "\\\.");
		if (tokens.size() < 2) {
			log_warning << "invalid data file:" << _source_files[i] << ", skip it!";
			continue;
		}
		if (tokens[1] >= start_date) {
			_source_files.erase(_source_files.begin(), _source_files.begin() + i);
			break;
		}
	}
	return count;
}

int BackTestEngine::run(stg::StgBase* pStg) {
	constexpr long chunk_size = 10 * 1024 *1024;
	std::unique_ptr<char> buffer(new char[chunk_size + 1]);
	for (auto&& file : _source_files) {
		gzFile gzfile = gzopen(file.c_str(), "r");
		if (gzfile == NULL) {
			throw "open data file:" + file + " failed!!!";
		}
		dat::TickData td;
		std::string content;
		while (!gzeof(gzfile)) {// load in once
			int count =	gzread(gzfile, buffer.get(), chunk_size);
			buffer.get()[count] = '\0'; // set the end mark
			bool tail_is_delim = buffer.get()[count - 1] == '\n';
			// 注意：strtok是会修改原来的字符串的
			char* first, * next;
			first = strtok(buffer.get(), "\n");
			content += first;
			bool is_updated = false;
			while (1) {
				next = strtok(NULL, "\n");
				if (next) {
					td.reset(content);
					content = next;
					is_updated = true;
				}
				else {
					if (tail_is_delim) {
						td.reset(content);
						content.clear();
						is_updated = true;
					}
					break;
				}
				if (!is_updated) // 无更新
					continue;
				if (pStg == nullptr) {
					mgr::StrategyManager::Get()->update(td);
				}
				else {
					pStg->update(td);
				}
			}
		}
	}
	return 0;
}

} // backtest namespace