#include "CX_DataFrame.h"

using namespace std;
using namespace CX;

//CX_DataFrame::DataFrameConfiguration CX_DataFrame::Configuration = { ",", "\"", "\"", "\t" };

CX_DataFrame::CX_DataFrame(void) :
	_rowCount(0)
{}

/*! Copy the contents of another CX_DataFrame.
\param df The data frame to copy.
\return A reference to this data frame.
\note The contents of this data frame are deleted during the copy.
*/
CX_DataFrame& CX_DataFrame::operator= (CX_DataFrame& df) {
	CX_DataFrame temp = df.copyRows(CX::Util::intVector<CX_DataFrame::rowIndex_t>(0, df.getRowCount()));
	std::swap(this->_data, temp._data);
	std::swap(this->_rowCount, temp._rowCount);
	return *this;
}

/*! Access the cell at the given row and column. If the row or column is out of bounds,
the data frame will be dynamically resized in order to fit the row or column.
\param row The row number.
\param column The column name.
\return A CX_DataFrameCell that can be read from or written to.
*/
CX_DataFrameCell CX_DataFrame::operator() (std::string column, rowIndex_t row) {
	_resizeToFit(column, row);
	return _data[column][row];
}

CX_DataFrameCell CX_DataFrame::operator() (rowIndex_t row, std::string column) {
	return this->operator()(column, row);
}

/*! Access the cell at the given row and column with bounds checking. Throws a std::exception
and logs an error if either the row or column is out of bounds.
\param row The row number.
\param column The column name.
\return A CX_DataFrameCell that can be read from or written to.
*/
CX_DataFrameCell CX_DataFrame::at(rowIndex_t row, std::string column) {
	try {
		return _data.at(column).at(row);
	} catch (std::exception& e) {
		std::stringstream s;
		s << "CX_SafeDataFrame: Out of bounds access with at() on indices (\"" << column << "\", " << row << ")";
		Instances::Log.error("CX_DataFrame") << s.str();
		throw std::exception(s.str().c_str());
	}
}

CX_DataFrameCell CX_DataFrame::at(std::string column, rowIndex_t row) {
	return at(row, column);
}

CX_DataFrameColumn CX_DataFrame::operator[] (std::string column) {
	return CX_DataFrameColumn(this, column);
}

CX_DataFrameRow CX_DataFrame::operator[] (rowIndex_t row) {
	return CX_DataFrameRow(this, row);
}

/*! Reduced argument version of print(). Prints all rows and columns. */
std::string CX_DataFrame::print(std::string delimiter, bool printRowNumbers) {
	if (getRowCount() == 0) {
		return "No rows to print.";
	}
	return print(CX::Util::intVector<CX_DataFrame::rowIndex_t>(0, getRowCount() - 1), delimiter, printRowNumbers);
}

/*! Reduced argument version of print(). Prints all rows and the selected columns. */
std::string CX_DataFrame::print(const std::set<std::string>& columns, std::string delimiter, bool printRowNumbers) {
	if (getRowCount() == 0) {
		return "No rows to print.";
	}
	return print(columns, CX::Util::intVector<CX_DataFrame::rowIndex_t>(0, getRowCount() - 1), delimiter, printRowNumbers);
}

/*! Reduced argument version of print(). Prints all columns and the selected rows. */
std::string CX_DataFrame::print(const std::vector<rowIndex_t>& rows, std::string delimiter, bool printRowNumbers) {
	set<string> columns;
	vector<string> names = columnNames();
	for (rowIndex_t i = 0; i < names.size(); i++) {
		columns.insert(names[i]);
	}
	return print(columns, rows, delimiter, printRowNumbers);
}

/*!
Prints the selected rows and columns of the data frame to a string. Each cell of the data frame will be separated with the selected
delimiter.

Each row of the data frame will be ended with a new line (whatever std::endl evaluates to, typically "\r\n").

\param columns Columns to print. Column names not found in the data frame will be ignored with a warning.
\param rows Rows to print. Row indices not found in the data frame will be ignored with a warning.
\param delimiter Delimiter to be used between cells of the data frame. Using comma or semicolon for the
delimiter is not recommended because semicolons are used as element delimiters in the string-encoded vectors
stored in the data frame and commas are used for element delimiters within each element of the string-encoded
vectors.
\param printRowNumbers If true, a column will be printed with the header "rowNumber" with the contents of the column
being the selected row indices. If false, no row numbers will be printed.
\return A string containing the printed version of the data frame.
*/
std::string CX_DataFrame::print(const std::set<std::string>& columns, const std::vector<rowIndex_t>& rows, std::string delimiter, bool printRowNumbers) {
	if (getRowCount() == 0) {
		return "No rows to print.";
	}

	//Get rid of invalid columns
	set<string> validColumns = columns;
	for (set<string>::iterator it = columns.begin(); it != columns.end(); it++) {
		if (_data.find(*it) == _data.end()) {
			Instances::Log.warning("CX_DataFrame") << "Invalid column name requested for printing: " << *it;
			validColumns.erase(*it);
		}
	}

	stringstream output;

	if (printRowNumbers) {
		output << "rowNumber" << delimiter;
	}

	for (map<string, vector<CX_DataFrameCell>>::iterator it = _data.begin(); it != _data.end(); it++) {
		if (validColumns.find(it->first) != validColumns.end()) {
			if (it != _data.begin()) {
				output << delimiter;
			}
			output << it->first;
		}
	}

	for (rowIndex_t i = 0; i < rows.size(); i++) {
		//Skip invalid row numbers
		if (rows[i] >= _rowCount) {
			Instances::Log.warning("CX_DataFrame") << "Invalid row index requested for printing: " << rows[i];
			continue;
		}

		output << endl;
		if (printRowNumbers) {
			output << rows[i] << delimiter;
		}

		for (map<string, vector<CX_DataFrameCell> >::iterator it = _data.begin(); it != _data.end(); it++) {
			if (validColumns.find(it->first) != validColumns.end()) {
				if (it != _data.begin()) {
					output << delimiter;
				}
				output << it->second[rows[i]].toString();
			}
		}
	}
	output << endl;

	return output.str();
}

/*! Reduced argument version of printToFile(). Prints all rows and columns. */
bool CX_DataFrame::printToFile(std::string filename, std::string delimiter, bool printRowNumbers) {
	return CX::Util::writeToFile(filename, this->print(delimiter, printRowNumbers), false);
}

/*! Reduced argument version of printToFile(). Prints all rows and the selected columns. */
bool CX_DataFrame::printToFile(std::string filename, const std::set<std::string>& columns, std::string delimiter, bool printRowNumbers) {
	return CX::Util::writeToFile(filename, this->print(columns, delimiter, printRowNumbers), false);
}

/*! Reduced argument version of printToFile(). Prints all columns and the selected rows. */
bool CX_DataFrame::printToFile(std::string filename, const std::vector<rowIndex_t>& rows, std::string delimiter, bool printRowNumbers) {
	return CX::Util::writeToFile(filename, this->print(rows, delimiter, printRowNumbers), false);
}

/*!
This function is equivalent in behavior to print() except that instead of returning a string containing the
printed contents of the data frame, the string is printed to a file. If the file exists, it will be overwritten.
\param filename Name of the file to print to. If it is an absolute path, the file will be put there. If it is
a local path, the file will be placed relative to the data directory of the project.
\return True for success, false if there was some problem writing to the file (insufficient permissions, etc.)
*/
bool CX_DataFrame::printToFile(std::string filename, const std::set<std::string>& columns, const std::vector<rowIndex_t>& rows, std::string delimiter, bool printRowNumbers) {
	return CX::Util::writeToFile(filename, this->print(columns, rows, delimiter, printRowNumbers), false);
}

/*! Deletes the contents of the data frame. Resizes the data frame to have no rows and no columns. */
void CX_DataFrame::clear (void) {
	_data.clear();
	_rowCount = 0;
}

/*! Reads data from the given file into the data frame. This function assumes that there will be a row of column names as the first row of the file.
It does not treat consecutive delimiters as a single delimiter.
\param filename The name of the file to read data from. If it is a relative path, the file will be read relative to the data directory.
\param cellDelimiter A string containing the delimiter between cells of the data frame.
\param vectorEncloser A string containing the characters that surround cell that contain a vector of data. By default,
vectors are enclosed in double quotes. This indicates to most software that it should treat the contents of the quotes "as-is", i.e.
if it finds a delimiter within the quotes, it should not split there, but wait until out of the quotes.
\return False if an error occurred, true otherwise.

\note The contents of the data frame will be deleted before attempting to read in the file.
\note If the data is read in from a file written with a row numbers column, that column will be read into the data frame. You can remove it using
deleteColumn("rowNumber").
*/
bool CX_DataFrame::readFromFile (std::string filename, std::string cellDelimiter, std::string vectorEncloser) {

	filename = ofToDataPath(filename);
	ofBuffer file = ofBufferFromFile(filename, false);

	if (!ofFile::doesFileExist(filename)) {
		Instances::Log.error("CX_DataFrame") << "Attempt to read from file " << filename << " failed: File not found.";
		return false;
	}

	this->clear();
	
	vector<string> headers = ofSplitString(file.getFirstLine(), cellDelimiter, false, false);
	rowIndex_t rowNumber = 0;

	do {
		string line = file.getNextLine();

		vector<string> parts;
		unsigned int cellStart = 0;

		for (unsigned int i = 0; i < line.size(); i++) {

			if (line.substr(i, vectorEncloser.size()) == vectorEncloser) {
				i += vectorEncloser.size();
				int vectorStart = i;
				for (; i < line.size(); i++) {
					if (line.substr(i, vectorEncloser.size()) == vectorEncloser) {
						break;
					}
				}
			}

			if (line.substr(i, cellDelimiter.size()) == cellDelimiter) {
				parts.push_back( line.substr(cellStart, i - cellStart) );
				i += cellDelimiter.size() - 1;
				cellStart = i + 1;
			} 
				
			if (i == (line.size() - 1)) {
				parts.push_back( line.substr(cellStart, i + 1 - cellStart) );
			}
		}

		if (parts.size() != headers.size()) {
			this->clear();
			Instances::Log.error("CX_DataFrame") << "Error while loading file " << filename << 
				": Row column count (" << parts.size() << ") does not match header column count (" << headers.size() << "). On row " << rowNumber;
			return false;
		} else {
			for (unsigned int i = 0; i < headers.size(); i++) {
				this->operator()(headers[i], rowNumber) = parts[i];
			}
			rowNumber++;
		}

	} while (!file.isLastLine());

	Instances::Log.notice("CX_DataFrame") << "File " << filename << " loaded successfully.";
	return true;
}

/*! Deletes the given column of the data frame.
\param columnName The name of the column to delete. If the column is not in the data frame, a warning will be logged.
\return True if the column was found and deleted, false if it was not found.
*/
bool CX_DataFrame::deleteColumn (std::string columnName) {
	map<string, vector<CX_DataFrameCell>>::iterator it = _data.find(columnName);
	if (it != _data.end()) {
		_data.erase(it);
		return true;
	}
	Instances::Log.warning("CX_DataFrame") << "Failed to delete column: " << columnName << " not found in the data frame.";
	return false;
}

/*! Deletes the given row of the data frame. 
\param row The row to delete (0 indexed). If row is greater than or equal to the number of rows in the data frame, a warning will be logged.
\return True if the row was in bounds and was deleted, false if the row was out of bounds.
*/
bool CX_DataFrame::deleteRow (rowIndex_t row) {
	if (row < _rowCount) {
		for (map<string, vector<CX_DataFrameCell>>::iterator col = _data.begin(); col != _data.end(); col++) {
			col->second.erase(col->second.begin() + row);
		}
		_rowCount--;
		return true;
	}
	Instances::Log.warning("CX_DataFrame") << "Failed to delete row: Index " << row << " was out of bounds. Number of rows " << this->getRowCount();
	return false;
}

/*! Appends the row to the end of the data frame.
\param row The row to add.
\note If the row has columns that do not exist in the data frame, those columns will be added to the data frame.
*/
void CX_DataFrame::appendRow(CX_DataFrameRow row) {
	//This implementation looks weird, but don't change it: it deals with a number of edge cases.
	_rowCount++;
	vector<string> names = row.names();
	for (unsigned int i = 0; i < names.size(); i++) {
		_data[names[i]].resize(_rowCount);
		_data[names[i]].back() = row[names[i]].toString(); //Copy the string data to the cell in the data frame.
	}

	_equalizeRowLengths(); //This deals with the case when the row doesn't have the same columns as the rest of the data frame
}

/*! Returns a vector containing the names of the columns in the data frame.
\return Vector of strings with the column names. */
std::vector<std::string> CX_DataFrame::columnNames(void) {
	vector<string> names;
	for (map<string, vector<CX_DataFrameCell>>::iterator it = _data.begin(); it != _data.end(); it++) {
		names.push_back(it->first);
	}
	return names;
}

/*! Re-orders the rows in the data frame.
\param newOrder Vector of row indices. newOrder.size() must equal this->getRowCount(). newOrder must not contain any out-of-range indices
(i.e. they must be < getRowCount()). Both of these error conditions are checked for in the function call and errors are logged.
\return true if all of the conditions of newOrder are met, false otherwise.
*/
bool CX_DataFrame::reorderRows(const std::vector<CX_DataFrame::rowIndex_t>& newOrder) {
	if (newOrder.size() != _rowCount) {
		CX::Instances::Log.error("CX_DataFrame") << "reorderRows failed: The number of indices in newOrder did not equal the number of rows in the data frame.";
		return false;
	}

	for (unsigned int i = 0; i < newOrder.size(); i++) {
		if (newOrder[i] >= _rowCount) {
			CX::Instances::Log.error("CX_DataFrame") << "reorderRows failed: newOrder contained out-of-range indices.";
			return false;
		}
	}

	vector<string> names = this->columnNames();

	for (vector<string>::iterator it = names.begin(); it != names.end(); it++) {
		vector<string> columnStrings = this->copyColumn<string>(*it);
		for (rowIndex_t i = 0; i < newOrder.size(); i++) {
			_data.at(*it).at(i) = columnStrings.at(newOrder[i]);
		}
	}
	return true;
}

/*! Creates CX_DataFrame containing a copy of the rows specified in rowOrder. The new data frame is not linked to the existing data frame.
\param rowOrder A vector of CX_DataFrame::rowIndex_t containing the rows from this data frame to be copied out.
The indices in rowOrder may be in any order: They don't need to be ascending. Additionally, the same row to be
copied may be specified multiple times.
\return A CX_DataFrame containing the rows specified in rowOrder.
*/
CX_DataFrame CX_DataFrame::copyRows(std::vector<CX_DataFrame::rowIndex_t> rowOrder) {
	unsigned int outOfRangeCount = 0;
	for (unsigned int i = 0; i < rowOrder.size(); i++) {
		if (rowOrder[i] >= _rowCount) {
			rowOrder.erase(rowOrder.begin() + i);
			i--;
			outOfRangeCount++;
		}
	}
	if (outOfRangeCount) {
		CX::Instances::Log.warning("CX_DataFrame") << "copyRows: rowOrder contained " << outOfRangeCount << " out-of-range indices. They will be ignored.";
	}

	CX_DataFrame copyDf;

	vector<string> columnNames = this->columnNames();
	for (vector<string>::iterator it = columnNames.begin(); it != columnNames.end(); it++) {
		//copyDf._data[*it].resize( rowOrder.size() ); //This can be left out. For the first column, it will have to resize that vector repeatedly. For the
		//next columns, they will be resized to the proper size when they are created.

		vector<string> columnStrings = this->copyColumn<string>(*it);
		for (rowIndex_t i = 0; i < rowOrder.size(); i++) {
			copyDf(*it, i) = columnStrings[rowOrder[i]];
		}
	}

	return copyDf;
}

/*!
Copies the specified columns into a new data frame.
\param columns A vector of column names to copy out. If a requested column is not found, a warning will be logged,
but the function will otherwise complete successfully.
\return A CX_DataFrame containing the specified columns.
*/
CX_DataFrame CX_DataFrame::copyColumns(std::vector<std::string> columns) {
	std::set<std::string> columnSet;

	for (unsigned int i = 0; i < columns.size(); i++) {
		if (_data.find(columns[i]) != _data.end()) {
			columnSet.insert(columns[i]);
		} else {
			Instances::Log.warning("CX_DataFrame") << "copyColumns: Requested column not found in data frame: " << columns[i];
		}
	}

	CX_DataFrame copyDf;
	for (std::set<string>::iterator it = columnSet.begin(); it != columnSet.end(); it++) {
		vector<string> s = this->copyColumn<string>(*it);
		for (rowIndex_t i = 0; i < this->getRowCount(); i++) {
			copyDf(*it, i) = s[i];
		}
	}

	return copyDf;
}

/*! Randomly re-orders the rows of the data frame.
\param rng Reference to a CX_RandomNumberGenerator to be used for the shuffling. */
void CX_DataFrame::shuffleRows(CX_RandomNumberGenerator &rng) {
	vector<CX_DataFrame::rowIndex_t> newOrder = CX::Util::intVector<CX_DataFrame::rowIndex_t>(0, _rowCount - 1);
	rng.shuffleVector(&newOrder);
	reorderRows(newOrder);
}

/*! Randomly re-orders the rows of the data frame using CX::Instances::RNG as the random number generator for the shuffling. */
void CX_DataFrame::shuffleRows(void) {
	shuffleRows(CX::Instances::RNG);
}

/*! Sets the number of rows in the data frame.
\param rowCount The new number of rows in the data frame.
\note If the row count is less than the number of rows already in the data frame, it will delete those rows with a warning.
*/
void CX_DataFrame::setRowCount(rowIndex_t rowCount) {
	if (rowCount < _rowCount) {
		Instances::Log.warning("CX_DataFrame") << "setRowCount: New row count less than current number of rows in the data frame. Extra rows deleted.";
	}
	_resizeToFit(rowCount - 1);
}

/*! Adds a column to the data frame.
\param columnName The name of the column to add. If a column with that name already exists in the data frame, a warning will be logged. */
void CX_DataFrame::addColumn(std::string columnName) {
	if (_data.find(columnName) != _data.end()) {
		Instances::Log.warning("CX_DataFrame") << "addColumn: Column already exists in data frame.";
		return;
	}
	_data[columnName].resize(_rowCount);
}

void CX_DataFrame::_resizeToFit(std::string column, rowIndex_t row) {
	if (_data[column].size() <= row) {
		_rowCount = std::max(_rowCount, row + 1);
		for (map<string, vector<CX_DataFrameCell>>::iterator it = _data.begin(); it != _data.end(); it++) {
			_data[it->first].resize(_rowCount);
		}
		CX::Instances::Log.verbose("CX_DataFrame") << "Data frame resized to fit (\"" << column << "\", " << row << ")";
	}
}

//This function fails if there are no columns in the data frame
void CX_DataFrame::_resizeToFit(rowIndex_t row) {
	if ((row >= _rowCount) && (_data.size() != 0)) {
		_data.begin()->second.resize(row + 1);
		_equalizeRowLengths();
		CX::Instances::Log.verbose("CX_DataFrame") << "Data frame resized to fit row " << row;
	}
}

void CX_DataFrame::_resizeToFit(std::string column) {
	if (_data[column].size() != _rowCount) {
		_equalizeRowLengths();
		CX::Instances::Log.verbose("CX_DataFrame") << "Data frame resized to fit column " << column;
	}
}

void CX_DataFrame::_equalizeRowLengths(void) {
	rowIndex_t maxSize = 0;
	for (map<string, vector<CX_DataFrameCell>>::iterator it = _data.begin(); it != _data.end(); it++) {
		maxSize = std::max(_data[it->first].size(), maxSize);
	}

	for (map<string, vector<CX_DataFrameCell>>::iterator it = _data.begin(); it != _data.end(); it++) {
		_data[it->first].resize(maxSize);
	}
	_rowCount = maxSize;
}


////////////////////////
// CX_DataFrameColumn //
////////////////////////

CX_DataFrameColumn::CX_DataFrameColumn(void) :
	_df(nullptr),
	_columnName("")
{}

CX_DataFrameColumn::CX_DataFrameColumn(CX_DataFrame *df, std::string column) :
	_df(df),
	_columnName(column)
{}

CX_DataFrameCell CX_DataFrameColumn::operator[] (CX_DataFrame::rowIndex_t row) {
	if (_df) {
		return _df->operator()(row, _columnName);
	} else {
		return _data[row];
	}
}

CX_DataFrame::rowIndex_t CX_DataFrameColumn::size(void) {
	if (_df) {
		return _df->getRowCount();
	} else {
		return _data.size();
	}
}


/////////////////////
// CX_DataFrameRow //
/////////////////////

CX_DataFrameRow::CX_DataFrameRow(void) :
	_df(nullptr),
	_rowNumber(-1)
{}

CX_DataFrameRow::CX_DataFrameRow(CX_DataFrame *df, CX_DataFrame::rowIndex_t rowNumber) :
	_df(df),
	_rowNumber(rowNumber)
{}

CX_DataFrameCell CX_DataFrameRow::operator[] (std::string column) {
	if (_df) {
		return _df->operator()(column, _rowNumber);
	} else {
		return _data[column];
	}
}

vector<std::string> CX_DataFrameRow::names(void) {
	if (_df) {
		return _df->columnNames();
	} else {
		std::vector<std::string> names;
		for (map<std::string, CX_DataFrameCell>::iterator it = _data.begin(); it != _data.end(); it++) {
			names.push_back(it->first);
		}
		return names;
	}
}

void CX_DataFrameRow::clear(void) {
	if (_df) {
		std::vector<std::string> names = _df->columnNames();
		for (unsigned int i = 0; i < names.size(); i++) {
			_df->operator()(names[i], _rowNumber) = "";
		}
	} else {
		_data.clear();
	}
}


//////////////////////
// CX_SafeDataFrame //
//////////////////////

CX_DataFrameCell CX_SafeDataFrame::operator() (std::string column, rowIndex_t row) {
	try {
		return _data.at(column).at(row);
	}
	catch (...) {
		CX::Instances::Log.error("CX_SafeDataFrame") << "Out of bounds access with operator() on indices (\"" << column << "\", " << row << ")";
	}
	return CX_DataFrameCell();
}

CX_DataFrameCell CX_SafeDataFrame::operator() (rowIndex_t row, std::string column) {
	return this->operator()(column, row);
}


