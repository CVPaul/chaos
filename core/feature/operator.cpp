#include <cstring>
#include <assert.h>
#include <iostream>
#include "operator.h"

namespace op{
Buffer::Buffer(int window){
    assert(window > 0);
    this->m_nPos = 0;
    this->m_nWindow = window;
    this->m_bFull = false;
    // for the convenience of updating, one more sentinel is added
    //     update: new_value = old_value - old_dat + new_data
    this->m_pBuffer = new ValueType[this->m_nWindow];
}

Buffer::Buffer(int window, ValueType* buffer, int pos, bool is_full){
    // point to a block of allocated memory
    assert(window > 0);
    assert(buffer != nullptr);
    assert(pos >= 0 && pos < window);
    this->m_nWindow = window;
    this->m_pBuffer = buffer;
    this->m_nPos = pos; 
    this->m_bFull = is_full;
}

Buffer::Buffer(const Buffer& o){
    // deep copy
    this->m_nPos = o.m_nPos;
    this->m_bFull = o.m_bFull;
    this->m_nWindow = o.m_nWindow;
    this->m_pBuffer = new ValueType[this->m_nWindow];
    memcpy(this->m_pBuffer, o.m_pBuffer, o.m_nWindow);
}

Buffer::~Buffer(){
    if (m_pBuffer){
        delete []m_pBuffer;
        m_pBuffer = nullptr;
    }
}

int Buffer::length(){
    if (m_bFull)
        return m_nWindow;
    return m_nPos;
}

bool Buffer::empty(){
    return m_nPos == 0 && !m_bFull;
}

ValueType& Buffer::operator[](int i){ 
    i = i < 0 ? length() + i : i;
    assert(i < length() && i >= 0);
    if (m_bFull){ 
        // trick: `m_nPos` is the start
        return m_pBuffer[(m_nPos + i) % m_nWindow];
    }else{
        return m_pBuffer[i];
    }
}

int Buffer::reset(const ValueType& value) {
    for (int i = 0; i < m_nWindow; i++) {
        m_pBuffer[i] = value;
    }
    m_nPos = 0;
    m_bFull = true;
    return 0;
}

int Buffer::update(const ValueType& value){
    if (m_bFull){
        m_pBuffer[m_nPos++] = value;
        m_nPos = m_nPos % m_nWindow;
    }
    else{
        m_pBuffer[m_nPos++] = value;
        if (m_nPos == m_nWindow){
            m_nPos = 0;
            m_bFull = true;
        }
    }
    return 0;
}

Operator::Operator(const Param& para){
    assert(para.size() > 0);
    this->value = 0;
    this->length = 0;
    auto iter = para.find("nWindow");
    if (iter == para.end()){
        throw "nWindow is required for operator";
    }
    this->m_nWindow = iter->second;
}

int Mean::call(const Input& input){
    ValueType res(0);
    length = std::min(input[0]->length(), m_nWindow);
    for(int i = 0; i < length; i++){
        res += input[0][0][input[0]->length() - i - 1];
    }
    value = res / length;
    return 0;
}

int Mean::update(const Input& input, Output* output){
    if (input[0]->empty() || output->m_nWindow == 0){
        return 1;
    }
    // `<=` : m_nWindow of buffer-in is required at least one bigger than op.m_nWindow
    if (input[0]->m_nWindow <= m_nWindow){
        return 2;
    }
    if (length == 0){ // init
        value = input[0][0][-1];
        length = 1;
    }
    else{
        if (input[0]->length() <= m_nWindow){
            length += 1;
            value += (input[0][0][-1] - value) / length;
        }else{
            value += (input[0][0][-1] - input[0][0][-length-1]) / length;
        }
    }
    if (output)
        output->update(value);
    return 0;
}

int Max::call(const Input& input){
    length = std::min(input[0]->length(), m_nWindow);
    ValueType res = input[0][0][-1];
    for(int i = 1; i < length; i++){
        res = std::max(res, input[0][0][input[0]->length() - i - 1]);
    }
    value = res;
    return 0;
}

int Max::update(const Input& input, Output* output){
    if (input[0]->empty() || output->m_nWindow == 0){
        return 1;
    }
    // `<=` : m_nWindow of buffer-in is required at least one bigger than op.m_nWindow
    if (input[0]->m_nWindow <= m_nWindow){
        return 2;
    }
    if (length == 0){ // init
        value = input[0][0][-1];
        length = 1;
    }
    else{
        if (input[0]->length() <= m_nWindow){
            length += 1;
            value = std::max(value, input[0][0][-1]);
        }else{
            if (input[0][0][-length-1] >= value){ // `value` is no longer the maximum, re-call
                // TODO: if necessary, efficiency can be further optimized here
                call(input);
            }
            else{
                value = std::max(value, input[0][0][-1]);      
            }
        }
    }
    if (output)
        output->update(value);
    return 0; // `value` is still the maximum
}

int Min::call(const Input& input){
    length = std::min(input[0]->length(), m_nWindow);
    ValueType res = input[0][0][-1];
    for(int i = 1; i < length; i++){
        res = std::min(res, input[0][0][input[0]->length() - i - 1]);
    }
    value = res;
    return 0;
}

int Min::update(const Input& input, Output* output){
    if (input[0]->empty() || output->m_nWindow == 0){
        return 1;
    }
    // `<=` : m_nWindow of buffer-in is required at least one bigger than op.m_nWindow
    if (input[0]->m_nWindow <= m_nWindow){
        return 2;
    }
    if (length == 0){ // init
        value = input[0][0][-1];
        length = 1;
    }
    else{
        if (input[0]->length() <= m_nWindow){
            length += 1;
            value = std::max(value, input[0][0][-1]);
        }
        else{
            if (input[0][0][-length-1] <= value){ // `value` is no longer the maximum, re-call
                // TODO: if necessary, efficiency can be further optimized here
                call(input);
            }
            else{
                value = std::min(value, input[0][0][-1]);
            }
        }
    }
    if (output)
        output->update(value);
    return 0;
}

int Open::update(const Input& input, Output* output){
    long t_stamp = input[0][0][0] / m_fPeriod;
    if (m_nTimestamp != t_stamp){ // new line
        if (m_bLatest) {
            value = input[0][0][1];
            if (output)
                output->update(value); // save the last to output
        }
        else {
            if (output)
                output->update(value); // save the last to output
            value = input[0][0][1];
        }
        m_nTimestamp = t_stamp;
        return 0;
    }
    return 1;
}

int High::update(const Input& input, Output* output){
    long t_stamp = input[0][0][0] / m_fPeriod;
    if (m_nTimestamp != t_stamp){ // new line
        if (output){
            output->update(value); // save the last to output
        }
        value = input[0][0][1];
        m_nTimestamp = t_stamp;
        return 0;
    }
    else{
        value = std::max(value, input[0][0][1]);
    }
    return 1;
}

int Low::update(const Input& input, Output* output){
    long t_stamp = input[0][0][0] / m_fPeriod;
    if (m_nTimestamp != t_stamp){ // new line
        if (output){
            output->update(value); // save the last to output
        }
        value = input[0][0][1];
        m_nTimestamp = t_stamp;
        return 0;
    }
    else{
        value = std::min(value, input[0][0][1]);
    }
    return 1;
}

int Close::update(const Input& input, Output* output){
    long t_stamp = input[0][0][0] / m_fPeriod;
    if (m_nTimestamp != t_stamp){ // new line
        if (output){
            output->update(value); // save the last to output
        }
        value = input[0][0][1];
        m_nTimestamp = t_stamp;
        return 0;
    }
    else{
        value = input[0][0][1];
    }
    return 1;
}

int TR::update(const Input& input, Output* output){
    ValueType h(input[0][0][-1]), l(input[1][0][-1]), c1(input[2][0][-2]);
    value = std::max(h-l, std::max(std::abs(c1 - h), std::abs(c1 - l)));
    if (output)
        output->update(value);
    return 0;
}
} // namespace op
