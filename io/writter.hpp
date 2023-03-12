#pragma once

#include <queue>
#include <cstdio>
#include <string>

namespace io {
template <typename T>
class AsyncWritter{
    public: 
        // init a thread to auto call the write function every 500ms
        AsyncWritter(const std::string& filename):
            _filename(filename), _thread(nullptr), _stop(false){
            _handle = fopen(_filename.c_str(), "a");   // open file
            if(_handle == nullptr){
                throw std::runtime_error("open file failed!, filename:" + _filename);
            }
            _thread = new std::thread(&AsyncWritter::_write_loop, this);
            if (_thread == nullptr){
                throw std::runtime_error("create writter thread failed!");
            }
            _obj_size = sizeof(T);
        }
        ~AsyncWritter(){
            _stop = true;
            _thread->join();
            delete _thread;
            fclose(_handle);
        }

        // write data to file
        void write(const T p){
            _queue.push(p);  // push data to queue
        }
        
        // get filename
        std::string get_filename(){
            return _filename;
        }

    private:
        // write data to file
        void _write_loop(){
            while(!_stop){
                if(_queue.size() > 0){
                    fwrite(&_queue.front(), _obj_size, 1, _handle);
                    _queue.pop();
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    private:
        bool _stop;
        int _obj_size;
        FILE* _handle;
        std::queue<T> _queue;
        std::thread* _thread;
		std::string _filename;
};
}//namespace io
