#ifndef __OGRE_FEATURE_ENGINE_H__
#define __OGRE_FEATURE_ENGINE_H__

#include <map>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#include "operator.h"

namespace feature{

    class Engine{
    // lock free feature engine:
    // 为了实现`lock
    // free`这里要求先注册再使用，注册之后不可修改
    public:
        Engine(int n_fields);
        ~Engine();
        int add(
            size_t buffer_size,
            const std::string&,
            op::Operator*,
            const std::list<std::string>& pre = {});
        int compile();
        int update(op::Buffer& stream);
    private:
        bool m_bCompiled;
        int m_nFeatures;
        std::list<std::string> m_listEntryPoints;
        std::unordered_map<std::string, int> m_mapDegrees;
        std::unordered_map<std::string, op::Operator*> m_mapOps;
        std::unordered_map<std::string, op::Buffer*> m_mapData;
        std::unordered_map<std::string, std::list<std::string>> m_mapPre;
        std::unordered_map<std::string, std::list<std::string>> m_mapNext;
    };
}

#endif // __OGRE_FEATURE_ENGINE_H__
