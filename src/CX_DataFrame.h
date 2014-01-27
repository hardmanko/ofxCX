#ifndef _CX_DATA_FRAME_H_
#define _CX_DATA_FRAME_H_

#include <vector>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <iostream>
#include <exception>

#include "ofUtils.h"

#include "CX_Utilities.h"
#include "CX_Logger.h"
#include "CX_RandomNumberGenerator.h"

#include "CX_DataFrameCell.h"

namespace CX {

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

	CX_DataFrame& operator= (CX_DataFrame& df) {
		CX_DataFrame temp = df.copyRows(CX::intVector<CX_DataFrame::rowIndex_t>(0, df.getRowCount()));
		std::swap(this->_data, temp._data);
		std::swap(this->_rowCount, temp._rowCount);
		return *this;
	}

	CX_DataFrameCell operator() (std::string column, rowIndex_t row);
	CX_DataFrameCell operator() (rowIndex_t row, std::string column);
	CX_DataFrameColumn operator[] (std::string column);
	CX_DataFrameRow operator[] (rowIndex_t row);

	void appendRow (CX_DataFrameRow row);

	std::string print (std::string delimiter = "\t", bool printRowNumbers = true);
	std::string print (const std::set<std::string>& columns, std::string delimiter = "\t", bool printRowNumbers = true);
	std::string print (const std::vector<rowIndex_t>& rows, std::string delimiter = "\t", bool printRowNumbers = true);
	std::string print (const std::set<std::string>& columns, const std::vector<rowIndex_t>& rows, std::string delimiter = "\t", bool printRowNumbers = true);

	bool printToFile (std::string filename, std::string delimiter = "\t", bool printRowNumbers = true);
	bool printToFile (std::string filename, const std::set<std::string>& columns, std::string delimiter = "\t", bool printRowNumbers = true);
	bool printToFile (std::string filename, const std::vector<rowIndex_t>& rows, std::string delimiter = "\t", bool printRowNumbers = true);
	bool printToFile (std::string filename, const std::set<std::string>& columns, const std::vector<rowIndex_t>& rows, std::string delimiter = "\t", bool printRowNumbers = true);

	std::vector<std::string> columnNames (void);
	rowIndex_t getRowCount (void) { return _rowCount; };

	bool reorderRows (const vector<CX_DataFrame::rowIndex_t>& newOrder);
	CX_DataFrame copyRows (vector<CX_DataFrame::rowIndex_t> rowOrder);
	void shuffleRows (void);
	void shuffleRows (CX_RandomNumberGenerator &rng);


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
};

class CX_DataFrameColumn {
public:
	CX_DataFrameColumn (void);
	CX_DataFrameCell operator[] (CX_DataFrame::rowIndex_t row);
	CX_DataFrame::rowIndex_t size (void);

private:
	friend class CX_DataFrame;
	CX_DataFrameColumn(CX_DataFrame *df, std::string column);

	CX_DataFrame *_df;
	vector<CX_DataFrameCell> _data;
	std::string _columnName;
};


class CX_DataFrameRow {
public:
	CX_DataFrameRow (void);
	CX_DataFrameCell operator[] (std::string column);
	vector<std::string> names (void);
	void clear (void);

private:
	friend class CX_DataFrame;
	CX_DataFrameRow(CX_DataFrame *df, CX_DataFrame::rowIndex_t rowNumber);

	CX_DataFrame *_df;
	std::map<std::string, CX_DataFrameCell> _data;
	CX_DataFrame::rowIndex_t _rowNumber;
};

class CX_SafeDataFrame : protected CX_DataFrame {
public:

	CX_DataFrameCell operator() (std::string column, rowIndex_t row);
	CX_DataFrameCell operator() (rowIndex_t row, std::string column);

	CX_DataFrameCell at (rowIndex_t row, std::string column);
	CX_DataFrameCell at (std::string column, rowIndex_t row);

	void setRowCount (rowIndex_t rowCount);
	void addColumn (std::string columnName);

	using CX_DataFrame::appendRow;
	using CX_DataFrame::print;
	using CX_DataFrame::printToFile;
	using CX_DataFrame::copyColumn;
	using CX_DataFrame::columnNames;
	using CX_DataFrame::getRowCount;
	using CX_DataFrame::shuffleRows;

};


}

#endif //_CX_DATA_FRAME_H_