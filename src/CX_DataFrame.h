#ifndef _CX_DATA_FRAME_H_
#define _CX_DATA_FRAME_H_

#include <vector>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <iostream>

#include "ofUtils.h"

#include "CX_Utilities.h"
#include "CX_DeferredLogger.h"

#include "CX_DataFrameCell.h"

namespace CX {

//typedef std::map<std::string, CX_DataFrameCell> CX_DataFrameRow;
//typedef std::vector<CX_DataFrameCell> CX_DataFrameColumn;

class CX_DataFrameRow;
class CX_DataFrameColumn;

class CX_DataFrame {
public:

	typedef std::vector<CX_DataFrameCell>::size_type rowIndex_t;

	static struct DataFrameConfiguration {
		std::string vectorElementDelimiter;
		std::string vectorStart;
		std::string vectorEnd;
		std::string cellDelimiter;
		//bool printHeaders;
		//bool printRowNumbers;
	} Configuration;

	CX_DataFrame (void);

	CX_DataFrameCell& operator() (std::string column, rowIndex_t row);
	CX_DataFrameCell& operator() (rowIndex_t row, std::string column);
	CX_DataFrameColumn operator[] (std::string column);
	CX_DataFrameRow operator[] (rowIndex_t row);

	void appendRow (CX_DataFrameRow row);

	std::string print (std::string delimiter = "\t");
	std::string print (const std::set<std::string>& columns, std::string delimiter = "\t");
	std::string print (const std::vector<rowIndex_t>& rows, std::string delimiter = "\t");
	std::string print (const std::set<std::string>& columns, const std::vector<rowIndex_t>& rows, std::string delimiter = "\t", bool printRowNumbers = true);

	bool printToFile (std::string filename);
	bool printToFile (std::string filename, const std::set<std::string>& columns, const std::vector<rowIndex_t>& rows, std::string delimiter = "\t", bool printRowNumbers = true);

	std::vector<std::string> columnNames (void);
	rowIndex_t rowCount (void) { return _rowCount; };

	template <typename T> std::vector<T> copyColumn (std::string column) {
		_resizeToFit(column);
		vector<T> rval;
		for (unsigned int i = 0; i < _data[column].size(); i++) {
			rval.push_back( ofFromString<T>( _data[column][i] ) );
		}
		return rval;
	}

protected:
	friend class CX_DataFrameRow;
	friend class CX_DataFrameColumn;

	std::map <std::string, vector<CX_DataFrameCell>> _data;
	rowIndex_t _rowCount;

	void _resizeToFit (std::string column, rowIndex_t row);
	void _resizeToFit (rowIndex_t row);
	void _resizeToFit (std::string column);

	void _equalizeRowLengths (void);

	//std::vector<CX_DataFrameCell> _getColumn (std::string column);
	//std::map<std::string, CX_DataFrameCell> _getRow (rowIndex_t row);
};

class CX_DataFrameColumn {
public:
	CX_DataFrameColumn (void) :
		_df(nullptr),
		_columnName("")
	{}

	CX_DataFrameCell& operator[] (CX_DataFrame::rowIndex_t row) {
		if (_df) {
			return _df->operator()(row, _columnName);
		} else {
			return _data[row];
		}
	}

	CX_DataFrame::rowIndex_t size (void) {
		if (_df) {
			return _df->rowCount();
		} else {
			return _data.size();
		}
	}

private:
	friend class CX_DataFrame;
	CX_DataFrameColumn(CX_DataFrame *df, std::string column) : 
		_df(df), 
		_columnName(column) 
	{};

	CX_DataFrame *_df;
	vector<CX_DataFrameCell> _data;
	std::string _columnName;
};


class CX_DataFrameRow {
public:
	CX_DataFrameRow (void) :
		_df(nullptr),
		_rowNumber(-1)
	{}

	CX_DataFrameCell& operator[] (std::string column) {
		if (_df) {
			return _df->operator()(column, _rowNumber);
			//_df->_resizeToFit(column, _rowNumber);
			//return CX_DataFrameCell(&_df->_data[column][_rowNumber]);
		} else {
			return _data[column];
		}
	}

	vector<std::string> names (void) {
		if (_df) {
			return _df->columnNames();
		} else {
			vector<string> names;
			for (map<string, CX_DataFrameCell>::iterator it = _data.begin(); it != _data.end(); it++) {
				names.push_back( it->first );
			}
			return names;
		}
	}

	void clear (void) {
		if (_df) {
			//Something
		} else {
			_data.clear();
		}
	}

private:
	friend class CX_DataFrame;
	CX_DataFrameRow(CX_DataFrame *df, CX_DataFrame::rowIndex_t rowNumber) : 
		_df(df), 
		_rowNumber(rowNumber) 
	{};

	CX_DataFrame *_df;
	std::map<std::string, CX_DataFrameCell> _data;
	CX_DataFrame::rowIndex_t _rowNumber;
};


class CX_SafeDataFrame : private CX_DataFrame {
public:

	const CX_DataFrameCell operator() (std::string column, rowIndex_t row) {
		try {
			return _data.at(column).at(row);
			//return CX_DataFrameCell(&_data.at(column).at(row));
		} catch (...) {
			CX::Instances::Log.error("CX_SafeDataFrame") << "Out of bounds access with operator() on indices (\"" << column << "\", " << row << ")";
		}
		return CX_DataFrameCell();
	}

	/*
	const CX_DataFrameCell operator() (std::string column, rowIndex_t row) {
		try {
			return CX_DataFrameCell(&_data.at(column).at(row));
		} catch (...) {
			CX::Instances::Log.error("CX_SafeDataFrame") << "Out of bounds access with operator() on indices (\"" << column << "\", " << row << ")";
		}
		return CX_DataFrameCell();
	}
	*/

	const CX_DataFrameCell operator() (rowIndex_t row, std::string column) {
		return this->operator()(column, row);
	}

	using CX_DataFrame::appendRow;
	using CX_DataFrame::print;
	using CX_DataFrame::printToFile;
	using CX_DataFrame::copyColumn;
	using CX_DataFrame::columnNames;
	using CX_DataFrame::rowCount;
};

}

#endif //_CX_DATA_FRAME_H_