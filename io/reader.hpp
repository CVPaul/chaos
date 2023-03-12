#pragma once

#include <string>


// implement a class to read data in a file with multi threads
namespace io {
template <typename T>
class ParallelReader {
public:
    ParallelReader(const std::string& filename, int num_threads){
        this->filename = filename;
        this->num_threads = num_threads;
        this->obj_size = sizeof(T);
    }
    ~ParallelReader(){
        for(auto& t : _threads){
            t.join();
        }
    }

    void read(std::list<T>& data){
        size_t total_size = fseek(file->_file, 0, SEEK_END);
        fseek(file->_file, 0, SEEK_SET);
        int n_obj = total_size / obj_size;
        int n_obj_per_thread = n_obj / num_threads;
        if (n_obj_per_thread * num_threads < n_obj) {
            n_obj_per_thread++;
        }
        data.reserze(n_obj);
        for (int i = 0; i < num_threads; ++i) {
            int start = i * n_obj_per_thread;
            int end = (i + 1) * n_obj_per_thread;
            if (i == num_threads - 1) {
                end = n_obj;
            }
            _threads.push_back(std::thread([this, start, end]() {
                int len(end - start) * obj_size; 
                FILE* fi = fopen(filename.c_str(), "rb");
                fseek(fi, start * obj_size, SEEK_SET);
                T* obj = new T[len];
                fread(obj, len, obj_size, fi);
                fclose(fi);
                for (int i = start; i < end; ++i) {
                    data[i] = *(obj + i - start);
                }
                delete[] obj;
            }));
        }
        for (auto& t : _threads) {
            t.join();
        }
    }
private:
    std::string filename;
    int num_threads;
    int obj_size;
    std::list<std::thread> _threads;
};
} // namespace io