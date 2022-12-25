#ifndef __BT_ENGINE_H__
#define __BT_ENGINE_H__

#include <string>
#include <vector>
#include "strategy/base.h"

namespace bt {
// backtest namespace
class BackTestEngine {
public:
	BackTestEngine();
	~BackTestEngine();
public:
	int init(const std::string& std_name="");
	int run(stg::StgBase* pStg = nullptr);
private:
	std::vector<std::string> _source_files;
};

}

#endif // __BT_ENGINE_H__
