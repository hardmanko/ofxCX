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

typedef std::map<std::string, CX_DataFrameCell> CX_DataFrameRow;
typedef std::vector<CX_DataFrameCell> CX_DataFrameColumn;

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

	CX_DataFrameCell operator() (std::string column, rowIndex_t row);
	CX_DataFrameCell operator() (rowIndex_t row, std::string column);
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
	std::map <std::string, vector<std::string>> _data;
	rowIndex_t _rowCount;

	void _resizeToFit (std::string column, rowIndex_t row);
	void _resizeToFit (rowIndex_t row);
	void _resizeToFit (std::string column);

	void _equalizeRowLengths (void);

	std::vector<CX_DataFrameCell> _getColumn (std::string column);
	std::map<std::string, CX_DataFrameCell> _getRow (rowIndex_t row);
};


class CX_SafeDataFrame : private CX_DataFrame {
public:

	const CX_DataFrameCell operator() (std::string column, rowIndex_t row) {
		try {
			return CX_DataFrameCell(&_data.at(column).at(row));
		} catch (...) {
			CX::Instances::Log.error("CX_SafeDataFrame") << "Out of bounds access with operator() on indices (\"" << column << "\", " << row << ")";
		}
		return CX_DataFrameCell();
	}

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