#pragma once

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

struct CX_DataFrameConfiguration {
	std::string cellDelimiter;

	std::string vectorElementDelimiter;
	std::string vectorStart;
	std::string vectorEnd;

	unsigned int floatingPointPrecision;

	bool suppressTypeMismatchWarnings;
};

/*! \defgroup dataManagement Data
This module is related to storing experimental data. CX_DataFrame is the most important class in this module.
*/

/*! This class provides and easy way to store data from an experiment and output that data to a file
at the end of the experiment. A CX_DataFrame is a square two-dimensional array of cells, but each cell
is capable of holding a vector of data. Each cell is indexed with a column name (a string) and a row
number. Cells can store many different kinds of data and the data can be inserted or extracted easily.
The standard method of storing data is to use \ref operator(), which dynamically resizes the data frame.
When an experimental session is complete, the data can be written to a file using printToFile().

See the example dataFrame.cpp for thorough examples of how to use a CX_DataFrame.

Several of the member functions of this class could be blocking if the amount of data in the data frame
is large enough.

\ingroup dataManagement
*/
class CX_DataFrame {
public:

	typedef std::vector<CX_DataFrameCell>::size_type rowIndex_t;

	/*! \class IoOptions
	Options for the format of files that are output to or input from a CX_DataFrame.
	*/
	struct IoOptions {
		IoOptions(void) :
			cellDelimiter("\t"),
			vectorEncloser("\""),
			vectorElementDelimiter(";")
		{}
		std::string cellDelimiter; //!< The delimiter between cells of the data frame when in file format. Defaults to tab.
		std::string vectorEncloser;
		std::string vectorElementDelimiter;
	};

	struct OutputOptions : public IoOptions {
		OutputOptions(void) :
			printRowNumbers(true)
		{}

		bool printRowNumbers;
		std::vector<rowIndex_t> rowsToPrint;
		std::set<std::string> columnsToPrint;
	};

	struct InputOptions : public IoOptions {};



	CX_DataFrame (void);

	CX_DataFrame& operator= (const CX_DataFrame& df);

	CX_DataFrameCell operator() (std::string column, rowIndex_t row);
	CX_DataFrameCell operator() (rowIndex_t row, std::string column);

	CX_DataFrameCell at(rowIndex_t row, std::string column);
	CX_DataFrameCell at(std::string column, rowIndex_t row);

	CX_DataFrameColumn operator[] (std::string column);
	CX_DataFrameRow operator[] (rowIndex_t row);

	void appendRow (CX_DataFrameRow row);
	void setRowCount(rowIndex_t rowCount);
	void addColumn(std::string columnName);

	void append(CX_DataFrame df);

	std::string print(std::string delimiter = "\t", bool printRowNumbers = true) const;
	std::string print(const std::set<std::string>& columns, std::string delimiter = "\t", bool printRowNumbers = true) const;
	std::string print(const std::vector<rowIndex_t>& rows, std::string delimiter = "\t", bool printRowNumbers = true) const;
	std::string print(const std::set<std::string>& columns, const std::vector<rowIndex_t>& rows, std::string delimiter = "\t", bool printRowNumbers = true) const;
	std::string print(OutputOptions oOpt) const;

	bool printToFile(std::string filename, std::string delimiter = "\t", bool printRowNumbers = true) const;
	bool printToFile(std::string filename, const std::set<std::string>& columns, std::string delimiter = "\t", bool printRowNumbers = true) const;
	bool printToFile(std::string filename, const std::vector<rowIndex_t>& rows, std::string delimiter = "\t", bool printRowNumbers = true) const;
	bool printToFile(std::string filename, const std::set<std::string>& columns, const std::vector<rowIndex_t>& rows, std::string delimiter = "\t", bool printRowNumbers = true) const;
	bool printToFile(std::string filename, OutputOptions oOpt) const;

	bool readFromFile(std::string filename, InputOptions iOpt);
	bool readFromFile (std::string filename, std::string cellDelimiter = "\t", std::string vectorEncloser = "\"", std::string vectorElementDelimiter = ";");

	void clear (void);
	bool deleteColumn (std::string columnName);
	bool deleteRow (rowIndex_t row);

	std::vector<std::string> getColumnNames(void) const;
	bool columnExists(std::string columnName) const;
	bool columnContainsVectors(std::string columnName) const;

	rowIndex_t getRowCount(void) const;

	bool reorderRows (const vector<CX_DataFrame::rowIndex_t>& newOrder);
	CX_DataFrame copyRows (vector<CX_DataFrame::rowIndex_t> rowOrder) const;
	CX_DataFrame copyColumns (vector<std::string> columns);
	void shuffleRows (void);
	void shuffleRows (CX_RandomNumberGenerator &rng);

	template <typename T> std::vector<T> copyColumn(std::string column) const;
	std::vector<std::string> convertVectorColumnToColumns(std::string columnName, int startIndex, bool deleteOriginal, std::string newBaseName = "");
	void convertAllVectorColumnsToMultipleColumns(int startIndex, bool deleteOriginals);

protected:
	friend class CX_DataFrameRow;
	friend class CX_DataFrameColumn;

	std::map <std::string, vector<CX_DataFrameCell>> _data;
	rowIndex_t _rowCount;
	IoOptions _ioOptions;

	void _resizeToFit (std::string column, rowIndex_t row);
	void _resizeToFit (rowIndex_t row);
	void _resizeToFit (std::string column);

	void _equalizeRowLengths (void);
};

/*! Makes a copy of the data contained in the named column, converting it to the specified type
(such a conversion must be possible).
\tparam T The type of data to extract. Must not be std::vector<C>, where C is any type.
\param column The name of the column to copy data from.
\return A vector containing the copied data. */
template <typename T> std::vector<T> CX_DataFrame::copyColumn(std::string column) const {
	//_resizeToFit(column);
	vector<T> rval;
	if (_data.find(column) == _data.end()) {
		CX::Instances::Log.error("CX_DataFrame") << "copyColumn() given nonexistent column name.";
		return rval;
	}

	for (unsigned int i = 0; i < _data.at(column).size(); i++) {
		const CX_DataFrameCell& c = _data.at(column).at(i);
		rval.push_back(c.to<T>());
	}
	return rval;
}

/*! \ingroup dataManagement */
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

/*! \ingroup dataManagement */
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

}
