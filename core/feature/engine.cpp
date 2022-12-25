#include <queue>
#include "engine.h"

namespace feature{

Engine::Engine(int n_fields){
    m_bCompiled = false;
    m_nFeatures = 0;
}

Engine::~Engine(){
    for (auto iter = m_mapOps.begin(); iter != m_mapOps.end(); iter ++){
        if (iter->second){
            delete iter->second;
            iter->second = nullptr;
        }
    }
    m_mapOps.clear();

    for (auto&& v: m_mapData){
        if (v.second){
            delete v.second;
            v.second = nullptr;
        }
    }
    m_mapData.clear();
}

int Engine::add(
    size_t buffer_size,
    const std::string& name,
    op::Operator* p,
    const std::list<std::string>& pre_requireds){
    if (m_bCompiled){
        // add is not allowed after m_bCompiled!"
        return 1;
    }
    if (m_mapPre.find(name) != m_mapPre.end()) {
        return 2; // already in
    }
    if (p == nullptr){
        return 3; // null
    }
    p->name = static_cast<std::string>(name);
    m_mapOps[name] = p;
    buffer_size = buffer_size == 0 ? p->m_nWindow : buffer_size;
    m_mapData[name] = new op::Buffer(buffer_size + 1);
    for (auto&& n : pre_requireds) {
        m_mapNext[n].push_back(name);
    }
    if (pre_requireds.empty()) {
        m_listEntryPoints.push_back(name);
    }
    else {
        m_mapPre[name] = pre_requireds;
        m_mapDegrees[name] = pre_requireds.size();
    }
    return 0;
}

int Engine::compile() {
    // core start
    if (m_listEntryPoints.empty()) {
        return 1; // no entry-point found
    }
    // check if the dag have ring
    std::unordered_map<std::string, int> degrees = m_mapDegrees;
    std::list<std::string> next_level, cur_level = m_listEntryPoints;
    while (!cur_level.empty()) {
        for (auto&& v : cur_level) {
            for (auto&& t : m_mapNext[v]) {
                degrees[t] -= 1;
                if (degrees[v] == 0) {
                    next_level.push_back(t);
                }
            }
        }
        cur_level.clear();
        for (auto&& v : next_level) {
            for (auto&& t : m_mapNext[v]) {
                degrees[t] -= 1;
                if (degrees[t] == 0) {
                    cur_level.push_back(t);
                }
            }
        }
        next_level.clear();
    }
    for (auto&& n : degrees) {
        if (0 != n.second) {
            return 2; // ring
        }
    }
    return 0;
}

int Engine::update(op::Buffer& stream){
    std::unordered_map<std::string, int> degrees = m_mapDegrees;
    std::list<std::string> next_level, cur_level = m_listEntryPoints;
    while(!cur_level.empty()){
        for(auto&& l : cur_level){
            op::Input in;
            for(auto&& k: m_mapPre[l]){
                in.push_back(m_mapData[k]);
            }
            if (in.empty()) {
                in.push_back(&stream);
            }
            // 所有前序依赖成功更新了才会更新后续的， 需要结合degrees == 1一起看
            if (0 != m_mapOps[l]->update(in, m_mapData[l]))
                continue;
            for(auto&& n : m_mapNext[l]){
                if (degrees[n] == 1){
                    next_level.push_back(n);
                }
                degrees[n] -= 1;
            }
        }
        cur_level.clear();
		for(auto&& l : next_level){
			op::Input in;
			for(auto&& k: m_mapPre[l]){
				in.push_back(m_mapData[k]);
			}
			if (in.empty()) {
				in.push_back(&stream);
			}
            // 所有前序依赖成功更新了才会更新后续的， 需要结合degrees == 1一起看
			if (0 != m_mapOps[l]->update(in, m_mapData[l]))
				continue;
			for(auto&& n : m_mapNext[l]){
				if (degrees[n] == 1){
					cur_level.push_back(n);
				}
				degrees[n] -= 1;
			}
		}
		next_level.clear();
    }
    return 0;
}
} // namespace feature
