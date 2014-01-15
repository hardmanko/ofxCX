#pragma once

#include <vector>
#include <map>
#include <string>
#include <sstream>

#include <iostream>

#include "stdint.h"

#include "ofUtils.h"

using namespace std;

struct CX_DataFrameCell {
	CX_DataFrameCell (void) :
		//data(nullptr),
		type("null")
	{
		data = new int(0);
	}

	void *data;
	string type;

	template <typename T> void setDataType (void);
};

class CX_DataFrame {
public:

	CX_DataFrame (void);

	template <typename T> T getData (string column, unsigned int row);
	void* getDataPointer (string column, unsigned int row);

	template <typename T> void addData (string column, unsigned int row, T data);
	template <> void addData<const char*> (string column, unsigned int row, const char* data);

	string printData (void);

private:
	map< string, vector< CX_DataFrameCell > > _data;
	unsigned int _nrow;

	void _checkRowLengths (void);
	void _prepareToAddData (string column, unsigned int row);
};

template <typename T> void CX_DataFrame::addData (string column, unsigned int row, T data) {
	_prepareToAddData(column, row);

	_data[column][row].data = new T(data);
	_data[column][row].setDataType<T>();
}

template <> 
void CX_DataFrame::addData<const char*> (string column, unsigned int row, const char* data) {
	_prepareToAddData(column, row);

	_data[column][row].data = new string(data);
	_data[column][row].type = "string";

}

template <typename T> 
T CX_DataFrame::getData (string column, unsigned int row) {
	if (row >= _nrow) {
		return T();
	}
	return *(T*)_data[column][row].data;
}


template <typename T>
class CX_TypedDataFrame {
public:
	CX_TypedDataFrame (void) :
		_nrow(0)
	{}

	void set (string column, unsigned int row, T data) {
		_boundsCheck(column, row);
		_data[column][row] = data;
	}

	T get (string column, unsigned int row) {
		_boundsCheck(column, row);
		return _data[column][row];
	}

	string print (string delim = "\t") {
		stringstream output;

		output << "Row";
		for (map<string, vector<T>>::iterator it = _data.begin(); it != _data.end(); it++) {
			output << delim << it->first;
		}

		for (unsigned int r = 0; r < _nrow; r++) {
			output << endl << r;
			for (map<string, vector<T>>::iterator it = _data.begin(); it != _data.end(); it++) {
				output << delim << it->second[r];
			}
		}

		return output.str();
	}

	//Returns a column reference
	vector<T>& operator[] (string s) {
		if (_data[s].size() != _nrow) {
			_data[s].resize(_nrow);
		}
		return _data[s];
	}

	vector<T>& operator[] (const char* c) {
		return this->operator[](string(c));
	}

protected:
	void _boundsCheck (string column, unsigned int row) {
		if (_data[column].size() <= row) {
			_nrow = std::max(_nrow, row + 1);
			for (map<string, vector<T>>::iterator it = _data.begin(); it != _data.end(); it++) {
				string col = it->first;
				_data[col].resize(_nrow);
			}
		}
	}

	map<string, vector<T>> _data;
	unsigned int _nrow;

private:
	
};


class CX_StringDataFrame : public CX_TypedDataFrame<string> {
public:
	void set (string column, unsigned int row, string data);
	void set (string column, unsigned int row, const char* data);
	//void set (string column, unsigned int row, double data);
	//void set (string column, unsigned int row, long long data);

	template <typename T> void set (string column, unsigned int row, T data) {
		set( column, row, ofToString(data) );
	};

};



	class CX_BaseObject {
	public:
		~CX_BaseObject (void) {
			delete _data;
		}

	protected:
		void* _data;
	};

	class CX_Double : public CX_BaseObject {
	public:

		CX_Double (void) {
			this->_data = new double;
		}

		operator double (void) {
			return *(double*)(this->_data);
		}

		void operator= (double d) {
			*(double*)this->_data = d;
		}

		double get (void) {
			return *(double*)_data;
		}

	private:

	};

	class CX_String : public CX_BaseObject {
	public:
		CX_String (void) {
			this->_data = new string;
		}

		operator string (void) {
			return *(string*)(this->_data);
		}

		void operator= (string s) {
			*(string*)this->_data = s;
		}

		string get (void) {
			return *(string*)_data;
		}
	};
