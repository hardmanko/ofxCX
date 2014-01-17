#pragma once

#include <vector>
#include <map>
#include <set>
#include <string>
#include <sstream>

#include <iostream>

#include "stdint.h"

#include "ofUtils.h"

#include "CX_Utilities.h"
#include "CX_DeferredLogger.h"

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
	CX_DataFrameCell& operator= (T val) {
		*_str = ofToString<T>(val, 16);
		return *this;
	}

	template <typename T>
	operator T (void) const {
		return ofFromString<T>(*_str);
	}

	template <typename T>
	CX_DataFrameCell& operator= (vector<T> val) {
		_setVector(val);
		return *this;
	}

	template <typename T>
	operator vector<T> (void) const {
		return _getVector<T>();
	}

	string toString (void) const {
		return *_str;
	}


private:

	friend class CX_DataFrame;
	friend class CX_SafeDataFrame;
	CX_DataFrameCell(string *s) : 
		_selfAllocated(false), 
		_str(s) 
	{}
	
	template <typename T> void _setVector (vector<T> values) {
		*_str = "\"" + CX::vectorToString(values, ";", 16) + "\"";
		//*_str = CX::vectorToString(values, ",", 16);
		//*_str = CX::vectorToString(values, "{", 16);
	}

	template <typename T> vector<T> _getVector (void) const {
		string encodedVect = *_str;

		//ofStringReplace( encodedVect, CX_DataFrame::Configuration.vectorStart );
		//CX_DataFrame::Configuration.vectorEnd;
		
		ofStringReplace(encodedVect, "\"", "");

		vector<string> parts = ofSplitString(encodedVect, ";");

		vector<T> values;
		for (unsigned int i = 0; i < parts.size(); i++) {
			values.push_back( ofFromString<T>( parts[i] ) );
		}
		return values;
	}

	void _setPointer (string *str) {
		if (_selfAllocated) {
			delete _str;
		}
		_str = str;
		_selfAllocated = false;
	}

	string *_str;
	bool _selfAllocated;
};


ostream& operator<< (ostream& os, const CX_DataFrameCell& cell);

typedef map<string, CX_DataFrameCell> CX_DataFrameRow;
typedef vector<CX_DataFrameCell> CX_DataFrameColumn;

class CX_DataFrame {
public:

	typedef vector<CX_DataFrameCell>::size_type rowIndex_t;

	static struct DataFrameConfiguration {
		string vectorElementDelimiter;
		string vectorStart;
		string vectorEnd;
		string cellDelimiter;
		//bool printHeaders;
		//bool printRowNumbers;
	} Configuration;

	static bool tf;

	CX_DataFrame (void) :
		_rowCount(0)
	{}

	CX_DataFrameCell operator() (string column, rowIndex_t row) {
		_resizeToFit(column, row);
		return CX_DataFrameCell(&_data[column][row]);
	}

	CX_DataFrameCell operator() (rowIndex_t row, string column) {
		return this->operator()(column, row);
		//_resizeToFit(column, row);
		//return CX_DataFrameCell(&_data[column][row]);
	}

	CX_DataFrameColumn operator[] (string column) {
		return _getColumn(column);
	}

	CX_DataFrameRow operator[] (rowIndex_t row) {
		return _getRow(row);
	}

	void appendRow (CX_DataFrameRow row) {
		rowIndex_t nextRow = _rowCount;
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

	string print (set<string> columns, vector<rowIndex_t> rows, string delimiter) {
		stringstream output;
		output << "Row";

		for (map<string, vector<string>>::iterator it = _data.begin(); it != _data.end(); it++) {
			if (columns.find(it->first) != columns.end()) {
				output << delimiter << it->first;
			}
		}

		for (rowIndex_t i = 0; i < rows.size(); i++) {
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

	string print (vector<rowIndex_t> rows, string delimiter) {
		set<string> columns;
		vector<string> names = columnNames();
		for (rowIndex_t i = 0; i < names.size(); i++) {
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

	rowIndex_t rowCount (void) { return _rowCount; };

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

protected:
	map <string, vector<string>> _data;
	rowIndex_t _rowCount;

	void _resizeToFit (string column, rowIndex_t row) {
		if (_data[column].size() <= row) {
			_rowCount = std::max(_rowCount, row + 1);
			for (map<string, vector<string>>::iterator it = _data.begin(); it != _data.end(); it++) {
				_data[it->first].resize(_rowCount);
			}
			//CX::Instances::Log.verbose("CX_DataFrame") << "Data frame resized to fit (\"" << column << "\", " << row << ")";
		}
	}

	void _resizeToFit (rowIndex_t row) {
		if (row >= _rowCount) {
			_data.begin()->second.resize(row - 1);
			_equalizeRowLengths();
			//CX::Instances::Log.verbose("CX_DataFrame") << "Data frame resized to fit row " << row;
		}
	}

	void _resizeToFit (string column) {
		if (_data[column].size() != _rowCount) {
			_equalizeRowLengths();
			//CX::Instances::Log.verbose("CX_DataFrame") << "Data frame resized to fit column " << column;
		}
	}

	void _equalizeRowLengths (void) {
		rowIndex_t maxSize = 0;
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
		for (rowIndex_t i = 0; i < rowCount(); i++) {
			rval[i] = CX_DataFrameCell( &_data[column][i] );
		}
		return rval;
	}

	map<string, CX_DataFrameCell> _getRow (unsigned int row) {
		_resizeToFit(row);

		map<string, CX_DataFrameCell> r;
		for (map<string, vector<string>>::iterator it = _data.begin(); it != _data.end(); it++) {
			r[it->first]._setPointer( &it->second[row] );
		}
		return r;
	}
};

class CX_SafeDataFrame : private CX_DataFrame {
public:

	const CX_DataFrameCell operator() (string column, rowIndex_t row) {
		try {
			return CX_DataFrameCell(&_data.at(column).at(row));
		} catch (...) {
			//log error?
			CX::Instances::Log.error("CX_SafeDataFrame") << "Out of bounds access with operator() on indices (\"" << column << "\", " << row << ")";
		}
		return CX_DataFrameCell();
	}

	const CX_DataFrameCell operator() (rowIndex_t row, string column) {
		return this->operator()(column, row);
	}

	using CX_DataFrame::appendRow;
	using CX_DataFrame::print;
	using CX_DataFrame::copyColumn;
	using CX_DataFrame::columnNames;
	using CX_DataFrame::rowCount;

};