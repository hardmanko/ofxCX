#pragma once

#include <vector>
#include <map>
#include <set>
#include <string>
#include <sstream>

#include <iostream>

#include "stdint.h"

#include "ofUtils.h"

using namespace std;

class CX_DataFrameCell {
public:

	CX_DataFrameCell (void) :
		_selfAllocated(true)
	{
		_str = new string;
	}

	CX_DataFrameCell (const CX_DataFrameCell& r) : 
		_selfAllocated(true)
	{
		this->_str = new string(*r._str);
	}

	CX_DataFrameCell (CX_DataFrameCell&& r) {
		this->_str = r._str;
		this->_selfAllocated = r._selfAllocated;

		r._str = nullptr;
		r._selfAllocated = false;
	}

	~CX_DataFrameCell (void) {
		if (_selfAllocated) {
			delete _str;
		}
	}

	template <typename T>
	void operator= (T val) {
		*_str = ofToString<T>(val, 16);
	}

	template <typename T>
	operator T (void) {
		return ofFromString<T>(*_str);
	}

	template <typename T>
	void operator= (vector<T> val) {
		_setVector(val);
	}

	template <typename T>
	operator vector<T> (void) {
		return _getVector<T>();
	}

	string toString (void) const {
		return *_str;
	}

private:

	template <typename T> void _setVector (vector<T> values) {
		*_str = "\"" + CX::vectorToString(values, ",", 16) + "\"";
	}

	template <typename T> vector<T> _getVector (void) {
		string encodedVect = *_str;
		
		ofStringReplace(encodedVect, "\"", "");
		vector<string> parts = ofSplitString(encodedVect, ",");
		vector<T> values;
		for (unsigned int i = 0; i < parts.size(); i++) {
			values.push_back( ofFromString<T>( parts[i] ) );
		}
		return values;
	}

	friend class CX_DataFrame;
	CX_DataFrameCell(string *s) : _selfAllocated(false), _str(s) {}

	void setPointer (string *str) {
		if (_selfAllocated) {
			delete _str;
		}
		_str = str;
		_selfAllocated = false;
	}

	string *_str;
	bool _selfAllocated;
};


static ostream& operator<< (ostream& os, const CX_DataFrameCell& cell) {
	os << cell.toString();
	return os;
}

typedef map<string, CX_DataFrameCell> CX_DataFrameRow;

class CX_DataFrame {
public:

	CX_DataFrame (void) :
		_rowCount(0)
	{}

	CX_DataFrameCell operator() (string column, unsigned int row) {
		_resizeToFit(column, row);
		return CX_DataFrameCell(&_data[column][row]);
	}

	vector<CX_DataFrameCell> operator[] (string column) {
		return _getColumn(column);
	}

	CX_DataFrameRow operator[] (unsigned int row) {
		return _getRow(row);
	}

	void appendRow (CX_DataFrameRow row) {
		unsigned int nextRow = _rowCount;
		_rowCount++;

		for (map<string,CX_DataFrameCell>::iterator it = row.begin(); it != row.end(); it++) {
			_data[it->first].resize(_rowCount);
			_data[it->first][nextRow] = it->second.toString();
		}

		_equalizeRowLengths();
	}

	template <typename T> vector<T> copyColumn (string column) {
		_resizeToFit(column);
		vector<T> rval;
		for (unsigned int i = 0; i < _data[column].size(); i++) {
			rval.push_back( ofFromString<T>( _data[column][i] ) );
		}
		return rval;
	}

	string print (string delimiter = "\t") {
		return print(CX::uintVector(0, rowCount() - 1), delimiter);
	}

	string print (set<string> columns, vector<unsigned int> rows, string delimiter) {
		stringstream output;
		output << "Row";

		for (map<string, vector<string>>::iterator it = _data.begin(); it != _data.end(); it++) {
			if (columns.find(it->first) != columns.end()) {
				output << delimiter << it->first;
			}
		}

		for (unsigned int i = 0; i < rows.size(); i++) {
			output << endl << rows[i];
			for (map<string, vector<string> >::iterator it = _data.begin(); it != _data.end(); it++) {
				if (columns.find(it->first) != columns.end()) {
					output << delimiter << it->second[rows[i]];
				}
			}
		}
		return output.str();
	}

	string print (set<string> columns, string delimiter) {
		return print(columns, CX::uintVector(0, rowCount() - 1), delimiter);
	}

	string print (vector<unsigned int> rows, string delimiter) {
		set<string> columns;
		vector<string> names = columnNames();
		for (unsigned int i = 0; i < names.size(); i++) {
			columns.insert( names[i] );
		}
		return print(columns, rows, delimiter);
	}

	//void printToFile (string filename);

	vector<string> columnNames (void) {
		vector<string> names;
		for (map<string,vector<string>>::iterator it = _data.begin(); it != _data.end(); it++) {
			names.push_back( it->first );
		}
		return names;
	}

	unsigned int rowCount (void) { return _rowCount; };

	/*
	void set (string column, unsigned int row, string value) {
		_resizeToFit(column, row);
		_data[column][row] = value;
	}

	void set (string column, unsigned int row, const char* value) {
		this->set(column, row, string(value));
	}

	template <typename T> void set(string column, unsigned int row, T value) {
		this->set(column, row, ofToString(value, 16));
	}
	

	string get (string column, unsigned int row) {
		_resizeToFit(column, row);
		return _data[column][row];
	}

	template <typename T> T get (string column, unsigned int row) {
		//string s = this->get(column, row);
		//return ofFromString<T>(s);
		_resizeToFit(column, row);
		return this->operator()(column, row).operator T();
	}

	template <typename T> void setVector (string column, unsigned int row, vector<T> values) {
		this->operator()(column, row).setVector<T>(values);
	}

	template <typename T> vector<T> getVector (string column, unsigned int row) {
		return this->operator()(column, row).getVector<T>();
	}
	*/

private:
	map <string, vector<string>> _data;
	unsigned int _rowCount;

	void _resizeToFit (string column, unsigned int row) {
		if (_data[column].size() <= row) {
			_rowCount = std::max(_rowCount, row + 1);
			for (map<string, vector<string>>::iterator it = _data.begin(); it != _data.end(); it++) {
				_data[it->first].resize(_rowCount);
			}
		}
	}

	void _resizeToFit (unsigned int row) {
		if (row >= _rowCount) {
			_data.begin()->second.resize(row - 1);
			_equalizeRowLengths();
		}
	}

	void _resizeToFit (string column) {
		if (_data[column].size() != _rowCount) {
			_equalizeRowLengths();
		}
	}

	void _equalizeRowLengths (void) {
		unsigned int maxSize = 0;
		for (map<string, vector<string>>::iterator it = _data.begin(); it != _data.end(); it++) {
			maxSize = std::max( _data[it->first].size(), maxSize );
		}

		for (map<string, vector<string>>::iterator it = _data.begin(); it != _data.end(); it++) {
			_data[it->first].resize(maxSize);
		}
	}


	vector<CX_DataFrameCell> _getColumn (string column) {
		_resizeToFit(column);
		vector<CX_DataFrameCell> rval( rowCount() );
		for (unsigned int i = 0; i < rowCount(); i++) {
			rval[i] = CX_DataFrameCell( &_data[column][i] );
		}
		return rval;
	}

	map<string, CX_DataFrameCell> _getRow (unsigned int row) {
		_resizeToFit(row);

		map<string, CX_DataFrameCell> r;
		for (map<string, vector<string>>::iterator it = _data.begin(); it != _data.end(); it++) {
			r[it->first].setPointer( &it->second[row] );
		}
		return r;
	}
};