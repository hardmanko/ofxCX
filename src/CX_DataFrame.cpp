#include "CX_DataFrame.h"

//CX_DataFrame::DataFrameConfiguration CX_DataFrame::Configuration = { ",", "\"", "\"", "\t" };

namespace CX {

CX_DataFrame::CX_DataFrame(void) :
	_rowCount(0)
{}

/*! Copy the contents of another CX_DataFrame to this data frame. Because this is a copy operation,
this may be \ref blockingCode if the copied data frame is large enough.
\param df The data frame to copy.
\return A reference to this data frame.
\note The contents of this data frame are deleted during the copy.
*/
CX_DataFrame& CX_DataFrame::operator= (const CX_DataFrame& df) {
	CX_DataFrame temp = df.copyRows(CX::Util::intVector<CX_DataFrame::rowIndex_t>(0, df.getRowCount() - 1));
	std::swap(this->_data, temp._data);
	std::swap(this->_rowCount, temp._rowCount);
	return *this;
}

/*! Access the cell at the given row and column. If the row or column is out of bounds,
the data frame will be resized in order to fit the new row(s) and/or column.
\param row The row number.
\param column The column name.
\return A CX_DataFrameCell that can be read from or written to.
*/
CX_DataFrameCell CX_DataFrame::operator() (std::string column, rowIndex_t row) {
	_resizeToFit(column, row);
	return _data[column][row];
}

/*! \brief Behaves just like CX_DataFrame::operator()(std::string, rowIndex_t). */
CX_DataFrameCell CX_DataFrame::operator() (rowIndex_t row, std::string column) {
	return this->operator()(column, row);
}

/*! Access the cell at the given row and column with bounds checking. Throws a `std::out_of_range`
exception and logs an error if either the row or column is out of bounds.
\param row The row number.
\param column The column name.
\return A CX_DataFrameCell that can be read from or written to.
*/
CX_DataFrameCell CX_DataFrame::at(rowIndex_t row, std::string column) {
	try {
		return _data.at(column).at(row);
	} catch (...) {
		//This just assumes that an exception here is out of bounds access...
		std::stringstream s;
		s << "CX_DataFrame: Out of bounds access with at() on indices (\"" << column << "\", " << row << ")";
		Instances::Log.error("CX_DataFrame") << s.str();
		throw std::out_of_range(s.str().c_str());
	}
}

/*! Equivalent to `CX::CX_DataFrame::at(rowIndex_t, std::string)`. */
CX_DataFrameCell CX_DataFrame::at(std::string column, rowIndex_t row) {
	return at(row, column);
}

/*! Extract a column from the data frame. Note that the returned value is not a 
copy of the original column. Rather, it represents the original column so that
if the returned column is modified, it will also modify the original data in the
parent data frame.
\param column The name of the column to extract.
\return A CX_DataFrameColumn.
\see See also copyColumn() for a way to copy out a column of data.
*/
CX_DataFrameColumn CX_DataFrame::operator[] (std::string column) {
	return CX_DataFrameColumn(this, column);
}

/*! Extract a row from the data frame. Note that the returned value is not a
copy of the original row. Rather, it represents the original row so that
if the returned row is modified, it will also modify the original data in the
parent data frame.
\param row The index of the row to extract.
\return A CX_DataFrameRow.
*/
CX_DataFrameRow CX_DataFrame::operator[] (rowIndex_t row) {
	return CX_DataFrameRow(this, row);
}

/*! Reduced argument version of CX_DataFrame::print(OutputOptions). Prints all rows and columns. */
std::string CX_DataFrame::print(std::string delimiter, bool printRowNumbers) const {
	std::vector<CX_DataFrame::rowIndex_t> rowsToPrint;
	if (this->getRowCount() > 0) {
		rowsToPrint = CX::Util::intVector<CX_DataFrame::rowIndex_t>(0, getRowCount() - 1);
	}

	return print(rowsToPrint, delimiter, printRowNumbers);
}

/*! Reduced argument version of print(). Prints all rows and the selected columns. */
std::string CX_DataFrame::print(const std::set<std::string>& columns, std::string delimiter, bool printRowNumbers) const {
	std::vector<CX_DataFrame::rowIndex_t> rowsToPrint;
	if (this->getRowCount() > 0) {
		rowsToPrint = CX::Util::intVector<CX_DataFrame::rowIndex_t>(0, getRowCount() - 1);
	}

	return print(columns, rowsToPrint, delimiter, printRowNumbers);
}

/*! Reduced argument version of print(). Prints all columns and the selected rows. */
std::string CX_DataFrame::print(const std::vector<rowIndex_t>& rows, std::string delimiter, bool printRowNumbers) const {
	set<string> columns;
	vector<string> names = getColumnNames();
	for (rowIndex_t i = 0; i < names.size(); i++) {
		columns.insert(names[i]);
	}
	return print(columns, rows, delimiter, printRowNumbers);
}

/*! Prints the selected rows and columns of the data frame to a string. Each cell of the data frame will be
separated with the selected delimiter. Each row of the data frame will be ended with a new line (whatever
std::endl evaluates to, typically "\n").

\param columns Columns to print. Column names not found in the data frame will be ignored with a warning.
\param rows Rows to print. Row indices not found in the data frame will be ignored with a warning.
\param delimiter Delimiter to be used between cells of the data frame. Using comma or semicolon for the
delimiter is not recommended because semicolons are used as element delimiters in the string-encoded vectors
stored in the data frame and commas are used for element delimiters within each element of the string-encoded
vectors.
\param printRowNumbers If true, a column will be printed with the header "rowNumber" with the contents of the column
being the selected row indices. If false, no row numbers will be printed.

\return A string containing the printed version of the data frame.

\note This function may be \ref blockingCode if the data frame is large enough.
*/
std::string CX_DataFrame::print(const std::set<std::string>& columns, const std::vector<rowIndex_t>& rows,
								std::string delimiter, bool printRowNumbers) const
{
	OutputOptions oOpt;
	oOpt.cellDelimiter = delimiter;
	oOpt.printRowNumbers = printRowNumbers;
	oOpt.columnsToPrint = columns;
	oOpt.rowsToPrint = rows;
	return print(oOpt);
}

/*! Prints the contents of the CX_DataFrame to a string with formatting options specified in oOpt.
\param oOpt Output formatting options. 
\return A string containing a formatted representation of the data frame contents. */
std::string CX_DataFrame::print(OutputOptions oOpt) const {

	if (oOpt.columnsToPrint.empty()) {
		std::vector<std::string> names = getColumnNames();
		for (rowIndex_t i = 0; i < names.size(); i++) {
			oOpt.columnsToPrint.insert(names[i]);
		}
	}

	//No rows to print is not an error: Just the column headers are printed.
	if (oOpt.rowsToPrint.empty() && this->getRowCount() > 0) {
		oOpt.rowsToPrint = Util::intVector<rowIndex_t>(0, this->getRowCount() - 1);
	}

	//Get rid of invalid columns
	std::set<std::string> validColumns = oOpt.columnsToPrint;
	for (std::set<std::string>::iterator it = oOpt.columnsToPrint.begin(); it != oOpt.columnsToPrint.end(); it++) {
		if (_data.find(*it) == _data.end()) {
			Instances::Log.warning("CX_DataFrame") << "Invalid column name requested for printing was ignored: " << *it;
			validColumns.erase(*it);
		}
	}

	std::stringstream output;

	//Output the headers
	if (oOpt.printRowNumbers) {
		output << "rowNumber" << oOpt.cellDelimiter;
	}

	for (std::map<std::string, std::vector<CX_DataFrameCell>>::const_iterator it = _data.begin(); it != _data.end(); it++) {
		if (validColumns.find(it->first) != validColumns.end()) {
			if (it != _data.begin()) {
				output << oOpt.cellDelimiter;
			}
			output << it->first;
		}
	}

	//Output the rows of data
	for (rowIndex_t i = 0; i < oOpt.rowsToPrint.size(); i++) {
		//Skip invalid row numbers
		if (oOpt.rowsToPrint[i] >= _rowCount) {
			Instances::Log.warning("CX_DataFrame") << "Invalid row index requested for printing: " << oOpt.rowsToPrint[i];
			continue;
		}

		output << endl;
		if (oOpt.printRowNumbers) {
			output << oOpt.rowsToPrint[i] << oOpt.cellDelimiter;
		}

		for (std::map<std::string, std::vector<CX_DataFrameCell> >::const_iterator it = _data.begin(); it != _data.end(); it++) {
			if (validColumns.find(it->first) != validColumns.end()) {
				if (it != _data.begin()) {
					output << oOpt.cellDelimiter;
				}

				//TODO: Update this to be more sensible/allow configuration.
				string s;
				if (it->second[oOpt.rowsToPrint[i]].isVector()) {
					std::vector<std::string> v = it->second[oOpt.rowsToPrint[i]].toVector<std::string>(false);
					s = oOpt.vectorEncloser + CX::Util::vectorToString(v, oOpt.vectorElementDelimiter) + oOpt.vectorEncloser;
				} else {
					s = it->second[oOpt.rowsToPrint[i]].to<std::string>(false); //Don't log errors/warnings
				}
				output << s;
			}
		}
	}
	output << endl;

	return output.str();
}

/*! Reduced argument version of printToFile(). Prints all rows and columns. */
bool CX_DataFrame::printToFile(std::string filename, std::string delimiter, bool printRowNumbers) const {
	return CX::Util::writeToFile(filename, this->print(delimiter, printRowNumbers), false);
}

/*! Reduced argument version of printToFile(). Prints all rows and the selected columns. */
bool CX_DataFrame::printToFile(std::string filename, const std::set<std::string>& columns, std::string delimiter, bool printRowNumbers) const {
	return CX::Util::writeToFile(filename, this->print(columns, delimiter, printRowNumbers), false);
}

/*! Reduced argument version of printToFile(). Prints all columns and the selected rows. */
bool CX_DataFrame::printToFile(std::string filename, const std::vector<rowIndex_t>& rows, std::string delimiter, bool printRowNumbers) const {
	return CX::Util::writeToFile(filename, this->print(rows, delimiter, printRowNumbers), false);
}

/*! This function is equivalent in behavior to CX::CX_DataFrame::print() except that instead of returning a string containing the
printed contents of the data frame, the string is printed directly to a file. If the file exists, it will be overwritten.
All paramters shared with print() are simply passed along to print(), so they have the same behavior.
\param filename Name of the file to print to. If it is an absolute path, the file will be put there. If it is
a local path, the file will be placed relative to the data directory of the project.

\param columns Columns to print. Column names not found in the data frame will be ignored with a warning.
\param rows Rows to print. Row indices not found in the data frame will be ignored with a warning.
\param delimiter Delimiter to be used between cells of the data frame. Using comma or semicolon for the
delimiter is not recommended because semicolons are used as element delimiters in the string-encoded vectors
stored in the data frame and commas are used for element delimiters within each element of the string-encoded
vectors.
\param printRowNumbers If true, a column will be printed with the header "rowNumber" with the contents of the column
being the selected row indices. If false, no row numbers will be printed.

\return `true` for success, `false` if there was some problem writing to the file (insufficient permissions, etc.) */
bool CX_DataFrame::printToFile(std::string filename, const std::set<std::string>& columns, const std::vector<rowIndex_t>& rows,
							   std::string delimiter, bool printRowNumbers) const
{
	return CX::Util::writeToFile(filename, this->print(columns, rows, delimiter, printRowNumbers), false);
}

/*! This function is equivalent in behavior to CX::CX_DataFrame::print() except that instead of returning a string containing the
printed contents of the data frame, the string is printed directly to a file. If the file exists, it will be overwritten.
All paramters shared with print() are simply passed along to print(), so they have the same behavior.
\param filename The name of the output file.
\param oOpt Output formatting options.
\param oOpt The output options.
\return `true` for success, `false` if there was some problem writing to the file (insufficient permissions, etc.) */
bool CX_DataFrame::printToFile(std::string filename, OutputOptions oOpt) const {
	return CX::Util::writeToFile(filename, this->print(oOpt), false);
}

/*! Deletes the contents of the data frame. Resizes the data frame to have no rows and no columns. */
void CX_DataFrame::clear (void) {
	_data.clear();
	_rowCount = 0;
}

/*! Reads data from the given file into the data frame. This function assumes that there will be a row of column names as the first row of the file.

\param filename The name of the file to read data from. If it is a relative path, the file will be read relative to the data directory.
\param cellDelimiter A string containing the delimiter between cells of data in the input file. Consecutive delimiters are not treated as a single delimiter.
\param vectorEncloser A string containing the character(s) that surround cells that contain a vector of data in the input file. By default,
vectors are enclosed in double quotes ("). This indicates to most software that it should treat the contents of the quotes "as-is", i.e.
if it finds a delimiter within the quotes, it should not split there, but wait until out of the quotes. If vectorEncloser is the empty
string, this function will not attempt to read in vectors: everything that looks like a vector will just be treated as a string.
\param vectorElementDelimiter The delimiter between the elements of the vector.
\return `false` if an error occurred, `true` otherwise.

\note The contents of the data frame will be deleted before attempting to read in the file.
\note If the data is read in from a file written with a row numbers column, that column will be read into the data frame. You can remove it using
deleteColumn("rowNumber").
\note This function may be \ref blockingCode if the read in data frame is large enough.
*/
bool CX_DataFrame::readFromFile (std::string filename, std::string cellDelimiter, std::string vectorEncloser, std::string vectorElementDelimiter) {
	InputOptions iOpt;
	iOpt.cellDelimiter = cellDelimiter;
	iOpt.vectorEncloser = vectorEncloser;
	iOpt.vectorElementDelimiter = vectorElementDelimiter;

	return readFromFile(filename, iOpt);
}

/*! Equivalent to a call to readFromFile(string, string, string, string), except that the last three arguments are
taken from iOpt.
\param filename The name of the file to read data from. If it is a relative path, the file will be read relative to the data directory.
\param iOpt Input options, such as the delimiter between cells in the input file.
*/
bool CX_DataFrame::readFromFile(std::string filename, InputOptions iOpt) {
	filename = ofToDataPath(filename);
	ofBuffer file = ofBufferFromFile(filename, false);

	if (!ofFile::doesFileExist(filename)) {
		Instances::Log.error("CX_DataFrame") << "Attempt to read from file " << filename << " failed: File not found.";
		return false;
	}

	this->clear();

	vector<string> headers = ofSplitString(file.getFirstLine(), iOpt.cellDelimiter, false, false);
	rowIndex_t rowNumber = 0;
	unsigned int fileRowNumber = 0;

	do {
		string line = file.getNextLine();
		

		vector<string> rowCells;
		vector<bool> isVector;
		unsigned int cellStart = 0;

		for (unsigned int i = 0; i < line.size(); i++) {

			bool enclosed = false;

			if ((iOpt.vectorEncloser != "") && (line.substr(i, iOpt.vectorEncloser.size()) == iOpt.vectorEncloser)) {
				i += iOpt.vectorEncloser.size();
				for (/* */; i < line.size(); i++) {
					if (line.substr(i, iOpt.vectorEncloser.size()) == iOpt.vectorEncloser) {
						enclosed = true;
						break;
					}
				}
			}

			if (line.substr(i, iOpt.cellDelimiter.size()) == iOpt.cellDelimiter) {
				rowCells.push_back(line.substr(cellStart, i - cellStart));
				isVector.push_back(enclosed);
				i += iOpt.cellDelimiter.size() - 1;
				cellStart = i + 1;
			}

			if (i == (line.size() - 1)) {
				rowCells.push_back(line.substr(cellStart, i + 1 - cellStart));
				isVector.push_back(enclosed);
			}
		}

		if (line == "") {
			Instances::Log.warning("CX_DataFrame") << "readFromFile(): Blank line skipped on line " << fileRowNumber << ".";
		} else if (rowCells.size() != headers.size()) {
			Instances::Log.error("CX_DataFrame") << "readFromFile(): Error while loading " << filename <<
				": The number of columns (" << headers.size() << ") on line " << fileRowNumber << " does not match the number of headers (" << headers.size() << ").";

			this->clear();
			return false;
		} else {
			for (unsigned int i = 0; i < headers.size(); i++) {

				if (isVector[i]) {
					std::string s = rowCells[i];
					std::string::size_type first = s.find_first_of(iOpt.vectorEncloser);
					std::string::size_type last = s.find_last_of(iOpt.vectorEncloser);

					std::vector<std::string> parts;
					if (first != std::string::npos && last != std::string::npos) {
						s = s.substr(first + iOpt.vectorEncloser.size(), last - first + iOpt.vectorEncloser.size() - 2);
						parts = ofSplitString(s, iOpt.vectorElementDelimiter, true, true);
					} else {
						//Error: Marked as vector but not within enclosers
					}

					this->operator()(headers[i], rowNumber) = parts;
				} else {
					this->operator()(headers[i], rowNumber) = rowCells[i];
				}
				this->operator()(headers[i], rowNumber).deleteStoredType();
			}
			rowNumber++;
		}

		fileRowNumber++;

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
	Instances::Log.warning("CX_DataFrame") << "Failed to delete column \"" << columnName << "\". It was not found in the data frame.";
	return false;
}

/*! Deletes the given row of the data frame.
\param row The row to delete (0 indexed). If row is greater than or equal to the number of rows in the data frame, a warning will be logged.
\return `true` if the row was in bounds and was deleted, `false` if the row was out of bounds.
*/
bool CX_DataFrame::deleteRow (rowIndex_t row) {
	if (row < _rowCount) {
		for (map<string, vector<CX_DataFrameCell>>::iterator col = _data.begin(); col != _data.end(); col++) {
			col->second.erase(col->second.begin() + row);
		}
		_rowCount--;
		return true;
	}
	Instances::Log.warning("CX_DataFrame") << "Failed to delete row " << row << ". It was out of bounds. Number of rows: " << this->getRowCount();
	return false;
}

/*! Appends the row to the end of the data frame.
\param row The row of data to add.
\note If `row` has columns that do not exist in the data frame, those columns will be added to the data frame.
*/
void CX_DataFrame::appendRow(CX_DataFrameRow row) {
	//This implementation looks weird, but don't change it without care: it deals with a number of edge cases.
	_rowCount++;
	vector<string> names = row.names();
	for (unsigned int i = 0; i < names.size(); i++) {
		_data[names[i]].resize(_rowCount);
		row[names[i]].copyCellTo( &_data[names[i]].back() ); //Copy the cell in the row into the data frame.
	}
	_equalizeRowLengths(); //This deals with the case when the row doesn't have the same columns as the rest of the data frame
}

/*! Inserts a row into the data frame.
\param row The row of data to insert.
\param beforeIndex The index of the row before which `row` should be inserted. If >= the number of rows currently stored, `row`
will be appended to the end of the data frame.
\note If `row` has columns that do not exist in the data frame, those columns will be added to the data frame.
*/
void CX_DataFrame::insertRow(CX_DataFrameRow row, rowIndex_t beforeIndex) {

	//For each new column, add it
	bool newColumnAdded = false;
	for (std::string& name : row.names()) {
		if (!this->columnExists(name)) {
			this->addColumn(name);
			newColumnAdded = true;
		}
	}

	if (newColumnAdded) {
		this->_equalizeRowLengths();
	}

	//Set up the location at which the new data will be added.
	rowIndex_t insertedElementIndex = std::min(beforeIndex, this->getRowCount());

	//For each existing column, insert one cell then assign new data to that cell
	std::vector<std::string> rowNames = row.names();
	for (auto existingColumn = this->_data.begin(); existingColumn != this->_data.end(); existingColumn++) {

		std::vector<CX_DataFrameCell>::iterator insertionIterator = existingColumn->second.end();

		if (beforeIndex < this->getRowCount()) {
			insertionIterator = existingColumn->second.begin() + beforeIndex;
		}

		existingColumn->second.insert(insertionIterator, CX_DataFrameCell());

		if (std::find(rowNames.begin(), rowNames.end(), existingColumn->first) != rowNames.end()) {
			row[existingColumn->first].copyCellTo(&existingColumn->second[insertedElementIndex]);
		}
	}

	//Note that a row has been added
	this->_rowCount++;
}

/*! Returns a vector containing the names of the columns in the data frame.
\return Vector of strings with the column names. */
std::vector<std::string> CX_DataFrame::getColumnNames(void) const {
	vector<string> names;
	for (map<string, vector<CX_DataFrameCell>>::const_iterator it = _data.begin(); it != _data.end(); it++) {
		names.push_back(it->first);
	}
	return names;
}

/*! Returns the number of rows in the data frame. */
CX_DataFrame::rowIndex_t CX_DataFrame::getRowCount(void) const {
	return _rowCount;
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

	vector<string> names = this->getColumnNames();

	for (vector<string>::iterator it = names.begin(); it != names.end(); it++) {

		vector<CX_DataFrameCell> columnCopy(_rowCount); //Data must first be copied into new columns,
			//otherwise moving data from one row to another could overwrite a row that hasn't been copied
			//yet.
		for (rowIndex_t i = 0; i < newOrder.size(); i++) {
			this->_data[*it][newOrder[i]].copyCellTo( &columnCopy[i] );
		}

		for (rowIndex_t i = 0; i < newOrder.size(); i++) {
			columnCopy[i].copyCellTo( &this->_data[*it][newOrder[i]] );
		}

		//this->_data[*it] = columnCopy;
	}
	return true;
}

/*! Creates CX_DataFrame containing a copy of the rows specified in rowOrder. The new data frame is not linked to the existing data frame.
\param rowOrder A vector of CX_DataFrame::rowIndex_t containing the rows from this data frame to be copied out.
The indices in rowOrder may be in any order: They don't need to be ascending. Additionally, the same row to be
copied may be specified multiple times.
\return A CX_DataFrame containing the rows specified in rowOrder.

\note This function may be \ref blockingCode if the amount of copied data is large.
*/
CX_DataFrame CX_DataFrame::copyRows(std::vector<CX_DataFrame::rowIndex_t> rowOrder) const {
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

	vector<string> columnNames = this->getColumnNames();

	for (string col : columnNames) {
        copyDf._resizeToFit(col, rowOrder.size() - 1);
        for (rowIndex_t row = 0; row < rowOrder.size(); row++) {
            this->_data.at(col)[rowOrder[row]].copyCellTo( &copyDf._data[col][row] );
        }
	}
	/*
	for (vector<string>::iterator col = columnNames.begin(); col != columnNames.end(); col++) {
		//copyDf._data[*col].resize( rowOrder.size() ); //This can be left out. For the first column, it will have to resize that vector repeatedly. For the
		//next columns, they will be resized to the proper size when they are created.

		copyDf._resizeToFit(*col, rowOrder.size() - 1);

		for (rowIndex_t row = 0; row < rowOrder.size(); row++) {
			this->operator()(*col, rowOrder[row]).copyCellTo( &copyDf._data[*col][row] );
		}
	}
    */
	return copyDf;
}

/*!
Copies the specified columns into a new data frame.
\param columns A vector of column names to copy out. If a requested column is not found, a warning will be logged,
but the function will otherwise complete successfully.
\return A CX_DataFrame containing the specified columns.
\note This function may be \ref blockingCode if the amount of copied data is large.
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
	for (std::set<string>::iterator col = columnSet.begin(); col != columnSet.end(); col++) {
		copyDf._resizeToFit(*col, this->getRowCount() - 1);
		for (rowIndex_t row = 0; row < this->getRowCount(); row++) {
			this->operator()(*col, row).copyCellTo( &copyDf._data[*col][row] );
		}
	}

	return copyDf;
}

/*! Randomly re-orders the rows of the data frame.
\param rng Reference to a CX_RandomNumberGenerator to be used for the shuffling.
\note This function may be \ref blockingCode if the data frame is large.
*/
void CX_DataFrame::shuffleRows(CX_RandomNumberGenerator &rng) {
	vector<CX_DataFrame::rowIndex_t> newOrder = CX::Util::intVector<CX_DataFrame::rowIndex_t>(0, _rowCount - 1);
	rng.shuffleVector(&newOrder);
	reorderRows(newOrder);
}

/*! Randomly re-orders the rows of the data frame using CX::Instances::RNG as the random number generator for the shuffling.
\note This function may be \ref blockingCode if the data frame is large. */
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

/*! Appends a data frame to this data frame. appendRow() is used to copy over the rows of `df`.
\param df The CX_DataFrame to append. */
void CX_DataFrame::append(CX_DataFrame df) {
	for (rowIndex_t i = 0; i < df.getRowCount(); i++) {
		this->appendRow(df[i]);
	}
}

void CX_DataFrame::_resizeToFit(std::string column, rowIndex_t row) {
	//Check the size of the column. If it is a new column, it will have size 0.
	//If it is an old column but too short, then it needs to be lengthened.
	if (_data[column].size() <= row) {
		_rowCount = std::max(_rowCount, row + 1);
		for (map<string, vector<CX_DataFrameCell>>::iterator it = _data.begin(); it != _data.end(); it++) {
			_data[it->first].resize(_rowCount);
		}
		CX::Instances::Log.verbose("CX_DataFrame") << "Data frame resized to fit (\"" << column << "\", " << row << ")";
	}
}

//This function fails if there are no columns in the data frame
//Resizes the data frame to fit a row with this index, NOT to fit this many rows.
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

/*! \brief Returns `true` if the named column exists in the `CX_DataFrame`. */
bool CX_DataFrame::columnExists(std::string columnName) const {
	return _data.find(columnName) != _data.end();
}

/*! \brief Returns `true` if the named column contains any cells which contain vectors (i.e.
have a length > 1). */
bool CX_DataFrame::columnContainsVectors(std::string columnName) const {
	if (!columnExists(columnName)) {
		return false;
	}

	for (rowIndex_t i = 0; i < getRowCount(); i++) {
		if (_data.at(columnName)[i].isVector()) {
			return true;
		}
	}
	return false;
}

/*! Converts a column which contains vectors of data into multiple columns which are given names
with an ascending integer suffix. Each new column will contain the data from one location in the
previous vectors of data. For example, if you have length 3 vectors in a column and use this function
on that column, you will end up with three columns, each of which contains one of the elements of those
vectors, with order maintained, of course.

If you have vectors with different lengths within the same column, this function still works, it
just fills empty cells of new columns with the string "NA".

\param columnName The name of the column to convert to multiple columns. If the named column does not exist or
it does not contain any vectors, this function has no effect.
\param startIndex The value at which to start giving suffix indices. For example, if it is 1, the first new column will
be named "newBaseName1", the second "newBaseName2", etc..
\param deleteOriginal If `true`, the original column, `columnName`, will be deleted once the data has been copied into the
new columns.
\param newBaseName If this is the empty string, `columnName` will be used as the base for the new column names. Otherwise,
`newBaseName` will be used.
\return A vector of strings containing the new names. If an error occurred or nothing needed to be done, this vector will
be of length 0.

\note If any of the names of the new columns conflicts with an existing column name, the new column will be created,
but its name will be changed by appending "_NEW". If this new name conflicts with an existing name, the process will
be repeated until the new name does not conflict.
*/
std::vector<std::string> CX_DataFrame::convertVectorColumnToColumns(std::string columnName, int startIndex, bool deleteOriginal, std::string newBaseName) {

	if (!columnExists(columnName) || !columnContainsVectors(columnName)) {
		return std::vector<std::string>();
	}

	if (newBaseName == "") {
		newBaseName = columnName;
	}

	rowIndex_t rowCount = this->getRowCount();

	//Copy the data to string vectors.
	std::vector<std::vector<std::string>> vectors(rowCount); //The original data; stored by row then column
	size_t maxVectorLength = 0;
	for (rowIndex_t i = 0; i < rowCount; i++) {
		vectors[i] = this->operator()(i, columnName).toVector<std::string>();
		maxVectorLength = std::max<size_t>(maxVectorLength, vectors[i].size());
	}

	//Create new column names and sort out conflicts.
	std::vector<std::string> columnNames(maxVectorLength);
	for (unsigned int i = 0; i < columnNames.size(); i++) {
		columnNames[i] = columnName + ofToString(startIndex + i);

		while (columnExists(columnNames[i])) {
			CX::Instances::Log.warning("CX_DataFrame") << "convertVectorColumnToColumns: New column name " << columnNames[i] << " conflicts with existing column name. "
				"The new column name will be changed to " << columnNames[i] << "_NEW.";
			columnNames[i] + "_NEW";
		}
	}

	//Add the new columns and copy over the data
	for (unsigned int i = 0; i < columnNames.size(); i++) {
		this->addColumn(columnNames[i]);

		for (rowIndex_t j = 0; j < rowCount; j++) {
			if (vectors[j].size() > i) {
				this->operator()(j, columnNames[i]) = vectors[j][i];
			} else {
				this->operator()(j, columnNames[i]) = "NA";
			}
		}
	}

	if (deleteOriginal) {
		this->deleteColumn(columnName);
	}

	return columnNames;
}

/*! For all columns with at least one cell that contains a vector, that column is converted into multiple columns
with CX_DataFrame::convertVectorColumnToColumns(). The name of the new columns will be the same as the name
of the original column, plus an index suffix.
\param startIndex The number at which to being suffixing the multiple columns derived from a vector column.
This value is used for each vector column (it's not cumuluative for all columns created with this function call, because
that would be bizarre).
\param deleteOriginals If `true`, the original vector columns will be deleted once they have been converted into
multiple columns.
*/
void CX_DataFrame::convertAllVectorColumnsToMultipleColumns(int startIndex, bool deleteOriginals) {
	std::vector<std::string> originalNames = this->getColumnNames();

	for (std::string& originalColumn : originalNames) {
		if (columnContainsVectors(originalColumn)) {
			convertVectorColumnToColumns(originalColumn, startIndex, deleteOriginals, originalColumn);
		}
	}
}

////////////////////////
// CX_DataFrameColumn //
////////////////////////

/*! Constructs a CX_DataFrameColumn without linking it to a CX_DataFrame. */
CX_DataFrameColumn::CX_DataFrameColumn(void) :
	_df(nullptr),
	_columnName("")
{}

CX_DataFrameColumn::CX_DataFrameColumn(CX_DataFrame *df, std::string column) :
	_df(df),
	_columnName(column)
{}

/*! Accesses the element in the specified row of the column. */
CX_DataFrameCell CX_DataFrameColumn::operator[] (CX_DataFrame::rowIndex_t row) {
	if (_df) {
		return _df->operator()(row, _columnName);
	} else {
		return _data[row];
	}
}

/*! \brief Returns the number of rows in the column. */
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

/*! Construct a CX_DataFrameRow without linking it to a CX_DataFrame. */
CX_DataFrameRow::CX_DataFrameRow(void) :
	_df(nullptr),
	_rowNumber(-1)
{}

//This is a private constructor
CX_DataFrameRow::CX_DataFrameRow(CX_DataFrame *df, CX_DataFrame::rowIndex_t rowNumber) :
	_df(df),
	_rowNumber(rowNumber)
{}

/*! Accesses the element in the specified column of the row. */
CX_DataFrameCell CX_DataFrameRow::operator[] (std::string column) {
	if (_df) {
		return _df->operator()(column, _rowNumber);
	} else {
		return _data[column];
	}
}

/*! \brief Returns a vector containing the names of the columns in this row. */
std::vector<std::string> CX_DataFrameRow::names(void) {
	if (_df) {
		return _df->getColumnNames();
	} else {
		std::vector<std::string> names;
		for (std::map<std::string, CX_DataFrameCell>::iterator it = _data.begin(); it != _data.end(); it++) {
			names.push_back(it->first);
		}
		return names;
	}
}

/*! \brief Clears the contents of the row. */
void CX_DataFrameRow::clear(void) {
	if (_df) {
		std::vector<std::string> names = _df->getColumnNames();
		for (unsigned int i = 0; i < names.size(); i++) {
			_df->operator()(names[i], _rowNumber).clear();
		}
	} else {
		_data.clear();
	}
}

} //namespace CX
