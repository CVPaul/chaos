#ifndef __DAT_NUMCPP_H__
#define __DAT_NUMCPP_H__

#include <list>
#include <tuple>
#include <string>
#include <string>
#include <assert.h>
#include <iostream>

namespace dat {
// namespace for numcpp
template <class Dtype>
class Matrix {
// self define cpp
public:
	Matrix() {
		_buffer = nullptr;
		row = col = 0;
	}

	Matrix(int row) {
		assert(row > 0 && "row is required to larger than 0");
		// 解决引用计数的问题
		this->_buffer = std::shared_ptr<Dtype>(new Dtype[row]);
		this->row = row;
		this->col = 1;
	}
	
	Matrix(int row, int col) {
		assert(row > 0 && col > 0 && "row and col is required to larger than 0");
		_buffer =  std::shared_ptr<Dtype>(new Dtype[row * col]);
		this->row = row;
		this->col = col;
	}
	
	~Matrix() {
		row = col = 0;
	};
	
	Dtype& operator[](int idx) {
		return _buffer.get()[idx];
	}

	void reset(Dtype val) {
		/** 
		tips:
			如果val=0等价于：memset(_buffer, val, row * col * sizeof(Dtype));
			这里应该不会触发，因为在编译的时候根本不知道val的值
		*/
		int size = row > 0 && col > 0 ? row * col : 0;
		for (int i = 0; i < size; i++) {
			_buffer.get()[i] = val;
		}
	}

	void copy(Matrix& other) {
		assert(row == other.row && col == other.col && "shape of matrixs were different");
		int size = row > 0 && col > 0 ? row * col : 0;
		for (int i = 0; i < size; i++) {
			_buffer.get()[i] = other[i];
		}
	}

	Matrix clone() {
		Matrix obj(row, col);
		obj.copy(*this);
	}

	void print(std::ostream& os, int row=-1, int col=-1) {
		os << "[";
		if (row < 0 && col >= 0) {
			for (int i = 0; i < this->row; i++) {
				os << _buffer.get()[i * this->col + col]
					<< (i == this->row - 1 ? "" : ",");
			}
		}
		else if (row >= 0 && col < 0) {
			for (int i = 0; i < this->col; i++) {
				os << _buffer.get()[row * this->col + i]
					<< (i == this->col - 1 ? "" : ",");
			}
		}
		else if (row < 0 && col < 0) {
			for (int i = 0; i < this->row; i++) {
				os << "[";
				for (int j = 0; j < this->col; j++) {
					os << _buffer.get()[i * this->col + j]
						<< (j == this->col - 1 ? "" : ",");
				}
				os << "]," << std::endl;
			}
		}
		else {
			os << _buffer.get()[row * this->col + col];
		}
		os << "]" << std::endl;
	}

	std::string prints(int row = -1, int col = -1) {
		std::stringstream os;
		print(os, row, col);
		return os.str();
	}

private:
	std::shared_ptr<Dtype> _buffer;

public:
	int row, col;
	std::list<std::string> columns;
};

}

#endif // __DAT_NUMCPP_H__

