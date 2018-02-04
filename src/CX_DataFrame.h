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

#include "CX_DataFrameCell.h"

namespace CX {

// Foward declarations
class CX_DataFrameRow;
class CX_DataFrameColumn;
class CX_RandomNumberGenerator;

/*! \defgroup dataManagement Data
This module is related to storing experimental data. CX_DataFrame is the most important class in this module.
*/

/*! This class provides and easy way to store data from an experiment and output that data to a file
at the end of the experiment. A CX_DataFrame is a square two-dimensional array of cells, but each cell
is capable of holding a vector of data. Each cell is indexed with a column name (a string) and a row
number. Cells can store many different kinds of data and the data can be inserted or extracted easily.
The standard method of storing data is to use CX_DataFrame::operator(), which dynamically resizes 
the data frame. When an experimental session is complete, the data can be written to a file using 
CX_DataFrame::printToFile().

See example-dataFrame for examples of how to use a CX_DataFrame.

Several of the member functions of this class could be blocking if the amount of data in the data frame
is large enough.

\ingroup dataManagement
*/
class CX_DataFrame {
public:

	typedef std::vector<CX_DataFrameCell>::size_type rowIndex_t; //!< An unsigned integer type used for indexing the rows of a CX_DataFrame.

	/*! \class IoOptions
	Options for the format of files that are output to or input from a CX_DataFrame.
	*/
	struct IoOptions {
		IoOptions(void) :
			cellDelimiter("\t"),
			vectorEncloser("\""),
			vectorElementDelimiter(";")
		{}
		std::string cellDelimiter; //!< The delimiter between cells of the data frame. Defaults to tab ("\t").
		std::string vectorEncloser; //!< The string which surrounds a vector of data (i.e. one cell of data, which happens to be a vector). Defaults to double quote ("\"").
		std::string vectorElementDelimiter; //!< The string which delimits elements of a vector. Defaults to semicolon (";").
	};

	/*! Options for the format of data that are output from a CX_DataFrame. */
	struct OutputOptions : public IoOptions {
		OutputOptions(void) :
			printRowNumbers(false)
		{}

		bool printRowNumbers; //!< If `true`, a column of row numbers will be printed. The column will be named "rowNumber". Defaults to `true`.
		std::vector<rowIndex_t> rowsToPrint; //!< The indices of the rows that should be printed. If the vector has size 0, all rows will be printed.
		std::vector<std::string> columnsToPrint; //!< The names of the columns that should be printed. If the vector has size 0, all columns will be printed.
	};

	/*! Options for the format of data that are input to a CX_DataFrame. */
	struct InputOptions : public IoOptions {};


	CX_DataFrame(void);
	CX_DataFrame(const CX_DataFrame& df);
	CX_DataFrame(CX_DataFrame&& df);

	CX_DataFrame& operator=(const CX_DataFrame& df);
	CX_DataFrame& operator=(CX_DataFrame&& df);

	void append(CX_DataFrame df);

	void clear(void);


	//Cell operations
	CX_DataFrameCell operator() (std::string column, rowIndex_t row);
	CX_DataFrameCell operator() (rowIndex_t row, std::string column);

	CX_DataFrameCell at(rowIndex_t row, std::string column);
	CX_DataFrameCell at(std::string column, rowIndex_t row);


	//Row operations
	void appendRow(CX_DataFrameRow row);
	void insertRow(CX_DataFrameRow row, rowIndex_t beforeIndex);
	bool deleteRow(rowIndex_t row);
	CX_DataFrameRow operator[] (rowIndex_t row);

	void setRowCount(rowIndex_t rowCount);
	rowIndex_t getRowCount(void) const;

	CX_DataFrameRow copyRow(rowIndex_t row) const;
	CX_DataFrame copyRows(std::vector<CX_DataFrame::rowIndex_t> rowOrder) const;

	bool reorderRows(const vector<CX_DataFrame::rowIndex_t>& newOrder);

	void shuffleRows(void);
	void shuffleRows(CX_RandomNumberGenerator &rng);


	//Column operations
	//template <typename T> bool addColumn(std::string columnName);

	bool addColumn(std::string columnName);
	bool deleteColumn(std::string columnName);

	std::vector<std::string> getColumnNames(void) const;
	bool columnExists(std::string columnName) const;

	//std::vector<CX_DataFrameCell>& getColumnReference(std::string columnName);

	CX_DataFrame copyColumns(std::vector<std::string> columns);
	
	CX_DataFrameColumn operator[] (std::string column);
	template <typename T> std::vector<T> copyColumn(std::string column) const;
	template <typename T> std::vector<std::vector<T>> copyVectorColumn(std::string column) const;

	bool columnContainsVectors(std::string columnName) const;
	std::vector<std::string> convertVectorColumnToColumns(std::string columnName, int startIndex, bool deleteOriginal, std::string newBaseName = "");
	void convertAllVectorColumnsToMultipleColumns(int startIndex, bool deleteOriginals);


	//Data IO
	std::string print(std::string delimiter = "\t", bool printRowNumbers = false) const;
	std::string print(const std::vector<std::string>& columns, std::string delimiter = "\t", bool printRowNumbers = false) const;
	std::string print(const std::vector<rowIndex_t>& rows, std::string delimiter = "\t", bool printRowNumbers = false) const;
	std::string print(const std::vector<std::string>& columns, const std::vector<rowIndex_t>& rows, std::string delimiter = "\t", bool printRowNumbers = false) const;
	std::string print(OutputOptions oOpt) const;

	bool printToFile(std::string filename, std::string delimiter = "\t", bool printRowNumbers = false) const;
	bool printToFile(std::string filename, const std::vector<std::string>& columns, std::string delimiter = "\t", bool printRowNumbers = false) const;
	bool printToFile(std::string filename, const std::vector<rowIndex_t>& rows, std::string delimiter = "\t", bool printRowNumbers = false) const;
	bool printToFile(std::string filename, const std::vector<std::string>& columns, const std::vector<rowIndex_t>& rows, std::string delimiter = "\t", bool printRowNumbers = false) const;
	bool printToFile(std::string filename, OutputOptions oOpt) const;

	bool readFromFile(std::string filename, InputOptions iOpt);
	bool readFromFile(std::string filename, std::string cellDelimiter = "\t", std::string vectorEncloser = "\"", std::string vectorElementDelimiter = ";");


private:
	friend class CX_DataFrameRow;
	friend class CX_DataFrameColumn;

	std::map<std::string, std::vector<CX_DataFrameCell>> _data;
	std::vector<std::string> _orderToName;

	rowIndex_t _rowCount;

	void _resizeToFit(std::string column, rowIndex_t row);
	void _resizeToFit(rowIndex_t row);
	void _resizeToFit(std::string column);

	void _equalizeRowLengths(void);

	bool _tryAddColumn(std::string column, bool setRowCount);

	void _duplicate(CX_DataFrame* target) const;


	static std::vector< std::vector<std::string> > _fileLineToVectors(std::string line, CX_DataFrame::InputOptions opt);
};

/*
template <typename T> 
bool CX_DataFrame::addColumn(std::string columnName) {
	if (!addColumn(columnName)) {
		return false;
	}

	for (rowIndex_t row = 0; row < getRowCount(); row++) {
		_data.at(columnName).at(row).setStoredType<T>();
	}
	
	return false;
}
*/

/*! Makes a copy of the data contained in the named column, converting it to the specified type
(such a conversion must be possible).

Note that if it is a vector column, you will only get the first element of each cell.
Use CX::CX_DataFrame::copyVectorColumn() to get all of the elements of each cell.

\tparam T The type of data to extract.
\param column The name of the column to copy data from.
\return A vector containing the copied data. */
template <typename T> 
std::vector<T> CX_DataFrame::copyColumn(std::string column) const {

	std::vector<T> rval;
	if (!columnExists(column)) {
		CX::Instances::Log.error("CX_DataFrame") << "copyColumn() given a nonexistent column name.";
		return rval;
	}

	for (unsigned int i = 0; i < _data.at(column).size(); i++) {
		const CX_DataFrameCell& c = _data.at(column).at(i);
		rval.push_back(c.to<T>());
	}
	return rval;
}

/*! Makes a copy of the data contained in the named column, converting it to 
vectors of the specified type (such a conversion must be possible).

\tparam T The type of data to extract.
\param column The name of the column to copy data from.
\return A vector of vectors containing the copied data. */
template <typename T> 
std::vector<std::vector<T>> CX_DataFrame::copyVectorColumn(std::string column) const {
	std::vector<std::vector<T>> rval;
	if (_data.find(column) == _data.end()) {
		CX::Instances::Log.error("CX_DataFrame") << "copyColumn() given a nonexistent column name.";
		return rval;
	}

	for (unsigned int i = 0; i < _data.at(column).size(); i++) {
		const CX_DataFrameCell& c = _data.at(column).at(i);
		rval.push_back(c.toVector<T>());
	}
	return rval;
}

/*! This class represents a column from a CX_DataFrame. It has special behavior that may not be obvious.
If it is extracted from a CX_DataFrame with the use of CX::CX_DataFrame::operator[](std::string),
then the extracted column is linked to the original column of data such that if either are modified, both
will see the effects.

\ingroup dataManagement */
class CX_DataFrameColumn {
public:
	CX_DataFrameColumn (void);
	CX_DataFrameCell operator[] (CX_DataFrame::rowIndex_t row);
	CX_DataFrame::rowIndex_t size (void);

private:
	friend class CX_DataFrame;
	CX_DataFrameColumn(CX_DataFrame *df, std::string column);

	CX_DataFrame *_df;
	std::vector<CX_DataFrameCell> _data;
	std::string _columnName;
};

/*! This class represents a row from a CX_DataFrame. It has special behavior that may not be obvious.
If it is extracted from a CX_DataFrame with the use of CX::CX_DataFrame::operator[](CX_DataFrame::rowIndex_t),
then the extracted row is linked to the original row of data such that if either are modified, both
will see the effects. See the code example.
If a CX_DataFrameRow is constructed normally (not extracted from a CX_DataFrame) it is not linked to any
data frame.

\code{.cpp}
//Create a CX_DataFrame and put some stuff in it.
CX_DataFrame df;
df(0, "a") = 2;
df(0, "b") = 5;

CX_DataFrameRow row0 = df[0]; //Extract row 0 from the data frame.
row0["a"] = 10; //Modify it.

cout << df.print() << endl; //See that the data frame has been modified.

df.appendRow(row0); //Append the row to the end of the data frame.

cout << df.print() << endl;

row0["a"] = 3; //Although row0 has been appended, it still only refers to row 0, not both rows,
//so this will only affect row 0 and not row 1.

cout << df.print() << endl;
\endcode

\ingroup dataManagement */
class CX_DataFrameRow {
public:
	CX_DataFrameRow (void);
	CX_DataFrameCell operator[] (std::string column);
	std::vector<std::string> names (void);
	void clear (void);

private:
	friend class CX_DataFrame;
	CX_DataFrameRow(CX_DataFrame *df, CX_DataFrame::rowIndex_t rowNumber);

	CX_DataFrame *_df;
	CX_DataFrame::rowIndex_t _rowNumber;

	std::map<std::string, CX_DataFrameCell> _data;
	std::vector<std::string> _orderToName;
	
};

}
