#ifndef __OGRE_OPERATOR_H__
#define __OGRE_OPERATOR_H__

#include <map>
#include <cmath>
#include <vector>
#include "NumCpp.hpp"

namespace op{
    
    typedef float ValueType;

    typedef std::map<std::string, float> Param;

    class Buffer{
    public:
        int m_nPos;
        int m_nWindow;
        bool m_bFull;

        ValueType m_vtLast;
        ValueType *m_pBuffer;
    public:
        Buffer(int window);
        Buffer(
            int window, ValueType* buffer,
            int pos, bool is_full);
        Buffer(const Buffer&);
        ~Buffer();
        
        int reset(const ValueType&);
        int update(const ValueType&);
        ValueType& operator[](int i);
        
        int length();
        bool empty();

    };

    typedef Buffer Output;

    typedef std::vector<Buffer*> Input;

    class Operator{
    public:
        Operator(const Param&);
        ~Operator(){};
        virtual int call(const Input& input){return 0;};
        virtual int update(const Input& input, Output* output) = 0;
    public:
        ValueType value;
        std::string name;
        int m_nWindow, length;
    };

    class Open: public Operator{
    public:
        Open(const Param& para) :Operator(para) {
            auto iter = para.find("fPeriod");
            if (iter == para.end()) {
                throw "fPeriod is required!";
            }
            this->m_fPeriod = iter->second;

            iter = para.find("bLatest");
            m_bLatest = iter != para.end() && iter->second != 0;
            this->m_nTimestamp = -1;
        }
        Open(const Param&& para) :Operator(para) {
            auto iter = para.find("fPeriod");
            if (iter == para.end()){
                throw "fPeriod is required!";
            }
            this->m_fPeriod = iter->second;

            iter = para.find("bLatest");
            m_bLatest = iter != para.end() && iter->second != 0;
            this->m_nTimestamp = -1;
        }
        ~Open(){};
        virtual int update(const Input& input, Output* output);
    public:
        float m_fPeriod;
        bool m_bLatest;
        long m_nTimestamp;
    }; 

    class High: public Operator{
    public:
        High(const Param& para) :Operator(para) {
            auto iter = para.find("fPeriod");
            if (iter == para.end()) {
                throw "fPeriod is required!";
            }
            this->m_fPeriod = iter->second;
            this->m_nTimestamp = -1;
        }
        High(const Param&& para) :Operator(para) {
            auto iter = para.find("fPeriod");
            if (iter == para.end()) {
                throw "fPeriod is required!";
            }
            this->m_fPeriod = iter->second;
            this->m_nTimestamp = -1;
        }
        ~High(){};
        virtual int update(const Input& input, Output* output);
    public:
        float m_fPeriod;
        long m_nTimestamp;
    }; 

    class Low: public Operator{
    public:
        Low(const Param& para) :Operator(para) {
            auto iter = para.find("fPeriod");
            if (iter == para.end()) {
                throw "fPeriod is required!";
            }
            this->m_fPeriod = iter->second;
            this->m_nTimestamp = -1;
        }
        Low(const Param&& para) :Operator(para) {
            auto iter = para.find("fPeriod");
            if (iter == para.end()) {
                throw "fPeriod is required!";
            }
            this->m_fPeriod = iter->second;
            this->m_nTimestamp = -1;
        }
        ~Low(){};
        virtual int update(const Input& input, Output* output);
    public:
        float m_fPeriod;
        long m_nTimestamp;
    };

    class Close: public Operator{
    public:
        Close(const Param& para) :Operator(para) {
            auto iter = para.find("fPeriod");
            if (iter == para.end()) {
                throw "fPeriod is required!";
            }
            this->m_fPeriod = iter->second;
            this->m_nTimestamp = -1;
        }
        Close(const Param&& para) :Operator(para) {
            auto iter = para.find("fPeriod");
            if (iter == para.end()) {
                throw "fPeriod is required!";
            }
            this->m_fPeriod = iter->second;
            this->m_nTimestamp = -1;
        }
        ~Close(){};
        virtual int update(const Input& input, Output* output);
    public:
        float m_fPeriod;
        long m_nTimestamp;
    };

    class TR: public Operator{
    public:
        TR(const Param& para):Operator(para){};
        TR(const Param&& para):Operator(para){};
        ~TR(){}
        virtual int call(const Input& input);
        virtual int update(const Input& input, Output* output);
    };

    class Mean: public Operator{
    public:
        Mean(const Param& para):Operator(para){};
        Mean(const Param&& para):Operator(para){};
        ~Mean(){};
        virtual int call(const Input& input);
        virtual int update(const Input& input, Output* output);
    };

    class Max: public Operator{
    public:
        Max(const Param& para):Operator(para){};
        Max(const Param&& para):Operator(para){};
        ~Max(){};
        virtual int call(const Input& input);
        virtual int update(const Input& input, Output* output);
    };

    class Min: public Operator{
    public:
        Min(const Param& para):Operator(para){};
        Min(const Param&& para):Operator(para){};
        ~Min(){};
        virtual int call(const Input& input);
        virtual int update(const Input& input, Output* output);
    };
}

#endif // __OGRE_OPERATOR_H__
