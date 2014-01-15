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
class CX_TypedDataFrameRow {
public:

	unsigned int rowNumber;

	T& operator[] (string column) {
		return get(column);
	}

	void set (string column, const T& value) {
		_data[column] = value;
	}

	T& get (string column) {
		return _data[column];
	}

	string print (string delim = "\t") {
		stringstream output;

		output << "Row";
		for (map<string, T>::iterator it = _data.begin(); it != _data.end(); it++) {
			output << delim << it->first;
		}

		output << endl << rowNumber;
		for (map<string, T>::iterator it = _data.begin(); it != _data.end(); it++) {
			output << delim << it->second;
		}

		return output.str();
	}

	vector<string> names (void) {
		vector<string> n;
		for (map<string,T>::iterator it = _data.begin(); it != _data.end(); it++) {
			n.push_back( it->first );
		}
		return n;
	}

protected:
	//unsigned int rowNumber;
	map<string, T> _data;
};




class CX_StringDataFrameRow : public CX_TypedDataFrameRow<string> {
public:
	void set (string column, const string& value) {
		_data[column] = value;
	}
	void set (string column, const char* value) {
		_data[column] = string(value);
	}
	template <typename T> void set (string column, const T& value) {
		_data[column] = ofToString<T>(value);
	}
};



template <typename T>
class CX_TypedDataFrame {
public:
	CX_TypedDataFrame (void) :
		_nrow(0)
	{}

	void set (string column, unsigned int row, const T& data) {
		_boundsCheck(column, row);
		_data[column][row] = data;
	}

	T& get (string column, unsigned int row) {
		_boundsCheck(column, row);
		return _data[column][row];
	}

	void pushRow (CX_TypedDataFrameRow<T> &r) {
		_pushRow(r);
	}

	void pushColumn (string columnName, vector<T> columnData) {
		_data[columnName] = columnData;
		_equalizeRowLengths();
	}



	CX_TypedDataFrameRow<T> getRow (unsigned int row) {

		CX_TypedDataFrameRow<T> r;
		r.rowNumber = row;

		if (row >= _nrow) {
			return r;
		}

		for (map<string, vector<T>>::iterator it = _data.begin(); it != _data.end(); it++) {
			r[it->first] = it->second[row];
		}
		return r;
	}

	vector<T>& column (string column) {
		if (_data[column].size() != _nrow) {
			_data[column].resize(_nrow);
		}
		return _data[column];
	}

	string print (string delim = "\t") {
		stringstream output;

		output << "Row";
		for (map<string, vector<T>>::iterator it = _data.begin(); it != _data.end(); it++) {
			output << delim << it->first;
		}

		for (unsigned int r = 0; r < _nrow; r++) {
			output << endl << r;
			for (map<string, vector<T> >::iterator it = _data.begin(); it != _data.end(); it++) {
				output << delim << it->second[r];
			}
		}

		return output.str();
	}

	//Returns a column reference
	vector<T>& operator[] (string s) {
		return column(s);
	}

	vector<T>& operator[] (const char* c) {
		return this->column(string(c));
	}

	//unsigned int nrow (void) { return _nrow; };

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

	void _equalizeRowLengths (void) {
		unsigned int maxSize = 0;
		for (map<string, vector<T>>::iterator it = _data.begin(); it != _data.end(); it++) {
			if (_data[it->first].size() > maxSize) {
				maxSize = _data[it->first].size();
			}
		}

		for (map<string, vector<T>>::iterator it = _data.begin(); it != _data.end(); it++) {
			if (_data[it->first].size() != maxSize) {
				_data[it->first].resize(maxSize);
			}
		}
	}

	void _pushRow (CX_TypedDataFrameRow<T> &r) {
		unsigned int nextRow = _nrow;
		_nrow++;

		vector<string> names = r.names();
		
		for (vector<string>::iterator it = names.begin(); it != names.end(); it++) {
			_data[*it].resize(_nrow);
			_data[*it][nextRow] = r[*it];
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

	template <typename T> void set (string column, unsigned int row, T data) {
		set( column, row, ofToString(data) );
	};

	void pushRow (CX_StringDataFrameRow& r) {
		//CX_TypedDataFrameRow<string> rs = dynamic_cast<CX_TypedDataFrameRow<string>&>(r);
		this->_pushRow( r );
	}
};


