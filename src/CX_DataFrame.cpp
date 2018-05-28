#include "CX_DataFrame.h"

#include "CX_RandomNumberGenerator.h"

namespace CX {

CX_DataFrame::CX_DataFrame(void) :
	_rowCount(0)
{}

/*! \brief Copy constructor. */
CX_DataFrame::CX_DataFrame(const CX_DataFrame& df) {
	df._duplicate(this);
}

/*! \brief Move constructor. */
CX_DataFrame::CX_DataFrame(CX_DataFrame&& df) {
	this->operator=(std::move(df));
}



/*! Copy the contents of another CX_DataFrame to this data frame. Because this is a copy operation,
this may be \ref blockingCode if the copied data frame is large enough.
\param df The data frame to copy.
\return A reference to this data frame.
\note The contents of this data frame are deleted during the copy.
*/
CX_DataFrame& CX_DataFrame::operator= (const CX_DataFrame& df) {
	df._duplicate(this);
	return *this;
}

/*! \brief Move assignment. */
CX_DataFrame& CX_DataFrame::operator=(CX_DataFrame&& df) {

	this->_rowCount = df._rowCount;
	this->_data = std::move(df._data);
	this->_orderToName = std::move(df._orderToName);

	return *this;
}


/*! Access the cell at the given row and column. If the row or column is out of bounds,
the data frame will be resized in order to fit the new row(s) and/or column.
\param row The row number.
\param column The column name.
\return A CX_DataFrameCell that can be read from or written to.
*/
CX_DataFrameCell CX_DataFrame::operator() (std::string column, RowIndex row) {
	_resizeToFit(column, row);
	return _data.at(column).at(row);
}

/*! \brief Equivalent to CX_DataFrame::operator()(std::string, RowIndex). */
CX_DataFrameCell CX_DataFrame::operator() (RowIndex row, std::string column) {
	return this->operator()(column, row);
}

/*! Access the cell at the given row and column with bounds checking. Throws a `std::out_of_range`
exception and logs an error if either the row or column is out of bounds.
\param row The row number.
\param column The column name.
\return A CX_DataFrameCell that can be read from or written to.
*/
CX_DataFrameCell CX_DataFrame::at(RowIndex row, std::string column) {
	return at(column, row);
}

/*! Equivalent to `CX::CX_DataFrame::at(RowIndex, std::string)`. */
CX_DataFrameCell CX_DataFrame::at(std::string column, RowIndex row) {
	try {
		return _data.at(column).at(row);
	} catch (...) {
		//This just assumes that an exception here is out of bounds access...
		std::ostringstream e1;
		e1 << "at(): Out of bounds access at(" << column << ", " << row << ")";
		CX::Instances::Log.error("CX_DataFrame") << e1.str();

		std::ostringstream e2;
		e2 << "CX_DataFrame::" << e1;
		throw std::out_of_range(e2.str().c_str());
	}
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
parent data frame. If you want a copy of the row rather than a reference to 
the row, use copyRow().
\param row The index of the row to extract.
\return A CX_DataFrameRow.
*/
CX_DataFrameRow CX_DataFrame::operator[] (RowIndex row) {
	return CX_DataFrameRow(this, row);
}

/*! \brief Reduced argument version of print(). Prints all rows and columns. */
std::string CX_DataFrame::print(std::string delimiter, bool printRowNumbers) const {
	std::vector<CX_DataFrame::RowIndex> rowsToPrint;
	if (this->getRowCount() > 0) {
		rowsToPrint = CX::Util::intVector<CX_DataFrame::RowIndex>(0, getRowCount() - 1);
	}

	return print(rowsToPrint, delimiter, printRowNumbers);
}

/*! \brief Reduced argument version of print(). Prints all rows and the selected columns. */
std::string CX_DataFrame::print(const std::vector<std::string>& columns, std::string delimiter, bool printRowNumbers) const {
	std::vector<CX_DataFrame::RowIndex> rowsToPrint;
	if (this->getRowCount() > 0) {
		rowsToPrint = CX::Util::intVector<CX_DataFrame::RowIndex>(0, getRowCount() - 1);
	}

	return print(columns, rowsToPrint, delimiter, printRowNumbers);
}

/*! \brief Reduced argument version of print(). Prints all columns and the selected rows. */
std::string CX_DataFrame::print(const std::vector<RowIndex>& rows, std::string delimiter, bool printRowNumbers) const {
	return print(getColumnNames(), rows, delimiter, printRowNumbers);
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
std::string CX_DataFrame::print(const std::vector<std::string>& columns, const std::vector<RowIndex>& rows,
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

	// If no columns are to be printed, print all columns
	if (oOpt.columnsToPrint.empty()) {
		oOpt.columnsToPrint = getColumnNames();
	}

	//Get rid of invalid columns
	std::vector<std::string> validColumns = Util::intersectionV(oOpt.columnsToPrint, getColumnNames());
	validColumns = Util::reorder(validColumns, getColumnNames(), false);

	if (validColumns.size() < oOpt.columnsToPrint.size()) {
		std::vector<std::string> invalidColumns = Util::exclude(oOpt.columnsToPrint, validColumns);
		CX::Instances::Log.warning("CX_DataFrame") << "The following column names were requested for printing but were not found in the data frame: " << 
			Util::vectorToString(invalidColumns, ", ");
	}
	

	//No rows to print is not an error: Just the column headers are printed.
	if (oOpt.rowsToPrint.empty() && this->getRowCount() > 0) {
		oOpt.rowsToPrint = Util::intVector<RowIndex>(0, this->getRowCount() - 1);
	}

	std::ostringstream output;

	//Output the headers
	if (oOpt.printRowNumbers) {
		output << "rowNumber" << oOpt.cellDelimiter;
	}

	for (unsigned int j = 0; j < validColumns.size(); j++) {
		if (j > 0) {
			output << oOpt.cellDelimiter;
		}
		output << validColumns[j];
	}

	//Output the rows of data
	for (RowIndex i = 0; i < oOpt.rowsToPrint.size(); i++) {
		//Skip invalid row numbers
		if (oOpt.rowsToPrint[i] >= _rowCount) {
			Instances::Log.warning("CX_DataFrame") << "Invalid row index requested for printing: " << oOpt.rowsToPrint[i];
			continue;
		}

		output << std::endl; // Headers on first line
		if (oOpt.printRowNumbers) {
			output << oOpt.rowsToPrint[i] << oOpt.cellDelimiter;
		}

		for (unsigned int j = 0; j < validColumns.size(); j++) {
			if (j > 0) {
				output << oOpt.cellDelimiter;
			}

			std::map<std::string, std::vector<CX_DataFrameCell> >::const_iterator it = _data.find(validColumns[j]);

			//TODO: Update this to be more sensible/allow configuration.
			const CX_DataFrameCell& cellRef = it->second[oOpt.rowsToPrint[i]];
			if (cellRef.isVector()) {
				output << oOpt.vectorEncloser;
				output << Util::vectorToString(cellRef.toVector<std::string>(false), oOpt.vectorElementDelimiter);
				output << oOpt.vectorEncloser;
			} else {
				output << cellRef.to<std::string>(false);
			}
		}
	}
	output << std::endl;

	return output.str();
}

/*! \brief Reduced argument version of printToFile(). Prints all rows and columns. */
bool CX_DataFrame::printToFile(std::string filename, std::string delimiter, bool printRowNumbers) const {
	std::string dfStr = this->print(delimiter, printRowNumbers);
	return CX::Util::writeToFile(filename, dfStr, false);
}

/*! \brief Reduced argument version of printToFile(). Prints all rows and the selected columns. */
bool CX_DataFrame::printToFile(std::string filename, const std::vector<std::string>& columns, std::string delimiter, bool printRowNumbers) const {
	std::string dfStr = this->print(columns, delimiter, printRowNumbers);
	return CX::Util::writeToFile(filename, dfStr, false);
}

/*! \brief Reduced argument version of printToFile(). Prints all columns and the selected rows. */
bool CX_DataFrame::printToFile(std::string filename, const std::vector<RowIndex>& rows, std::string delimiter, bool printRowNumbers) const {
	std::string dfStr = this->print(rows, delimiter, printRowNumbers);
	return CX::Util::writeToFile(filename, dfStr, false);
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

\return `true` for success, `false` if there was a problem writing to the file. */
bool CX_DataFrame::printToFile(std::string filename, const std::vector<std::string>& columns, const std::vector<RowIndex>& rows,
							   std::string delimiter, bool printRowNumbers) const
{
	std::string dfStr = this->print(columns, rows, delimiter, printRowNumbers);
	return CX::Util::writeToFile(filename, dfStr, false);
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
	_orderToName.clear();
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
bool CX_DataFrame::readFromFile(std::string filename, std::string cellDelimiter, std::string vectorEncloser, std::string vectorElementDelimiter) {
	InputOptions iOpt;
	iOpt.cellDelimiter = cellDelimiter;
	iOpt.vectorEncloser = vectorEncloser;
	iOpt.vectorElementDelimiter = vectorElementDelimiter;

	return readFromFile(filename, iOpt);
}

/*! Equivalent to a call to readFromFile(string, string, string, string), except that the last three arguments are
taken from `iOpt`.
\param filename The name of the file to read data from. If it is a relative path, the file will be read relative to the data directory.
\param iOpt Input options, such as the delimiter between cells in the input file.
*/
bool CX_DataFrame::readFromFile(std::string filename, InputOptions iOpt) {
	filename = ofToDataPath(filename);

	if (!ofFile::doesFileExist(filename)) {
		Instances::Log.error("CX_DataFrame") << "Attempt to read from file " << filename << " failed: File not found.";
		return false;
	}

	this->clear();

	ofBuffer fileBuf = ofBufferFromFile(filename, false);

	bool loadSuccess = this->_readFromString(fileBuf.getText(), iOpt, "readFromFile(): ", filename);

	if (loadSuccess) {
		Instances::Log.notice("CX_DataFrame") << "readFromFile(): File " << filename << " loaded successfully.";
	}
	return loadSuccess;
}

bool CX_DataFrame::_readFromString(const std::string& dfStr, const CX_DataFrame::InputOptions& opt, std::string callingFunction, std::string filename) {

	std::ostringstream endlOss;
	endlOss << std::endl;
	std::vector<std::string> lines = ofSplitString(dfStr, endlOss.str(), false, false);

	std::vector<std::string> headers = ofSplitString(lines.front(), opt.cellDelimiter, true, true);

	RowIndex rowNumber = 0;

	for (unsigned int lineIndex = 1; lineIndex < lines.size(); lineIndex++) {

		const std::string& line = lines[lineIndex];

		if (line == "") {
			Instances::Log.warning("CX_DataFrame") << callingFunction << "Blank line skipped on line " << (lineIndex + 1) << ".";
			continue;
		}

		std::vector<std::vector<std::string>> cells = CX_DataFrame::_fileLineToVectors(line, opt);

		if (cells.size() != headers.size()) {
			Instances::Log.error("CX_DataFrame") << callingFunction << "Error while loading " << filename <<
				": The number of columns (" << cells.size() << ") on line " << (lineIndex + 1) << 
				" does not match the number of headers (" << headers.size() << ").";

			this->clear();
			return false;
		}

		for (unsigned int i = 0; i < cells.size(); i++) {
			this->operator()(rowNumber, headers[i]).storeVector(cells[i]);
			this->operator()(rowNumber, headers[i]).deleteStoredType();
		}

		rowNumber++;
	}

	return true;
}

std::vector< std::vector<std::string> > CX_DataFrame::_fileLineToVectors(std::string line, const CX_DataFrame::InputOptions& opt) {

	std::vector<std::vector<std::string>> lineParts;

	if (line == "") {
		return lineParts;
	}

	std::string nextPart = "";
	bool nextVector = false;

	bool inEncloser = false;

	auto storeNextPart = [&]() {
		std::vector<std::string> parts;
		if (nextVector) {
			parts = ofSplitString(nextPart, opt.vectorElementDelimiter, true, true); // ignore empty and do trim
		} else {
			parts.push_back(nextPart);
		}
		lineParts.push_back(parts);
		nextPart = "";
		nextVector = false;
	};

	auto isSymbolAtPosition = [](const std::string& line, size_t pos, const std::string& sym) -> bool {
		return sym == line.substr(pos, sym.size());
	};


	for (unsigned int i = 0; i < line.size(); i++) {

		if (isSymbolAtPosition(line, i, opt.cellDelimiter)) {

			if (!inEncloser) {
				storeNextPart();

				i += (opt.cellDelimiter.size() - 1);
				continue;
			}

		} else if (isSymbolAtPosition(line, i, opt.vectorEncloser)) {

			if (!inEncloser) {
				nextVector = true;
			}

			inEncloser = !inEncloser;

			i += (opt.vectorEncloser.size() - 1);
			continue;

		}

		nextPart += line[i];
	}

	if (inEncloser) {
		//warning: EOL reached in encloser.
	}

	storeNextPart();

	return lineParts;
}

/*! Deletes the given column of the data frame.
\param columnName The name of the column to delete. If the column is not in the data frame, a warning will be logged.
\return True if the column was found and deleted, false if it was not found.
*/
bool CX_DataFrame::deleteColumn(std::string columnName) {

	if (!columnExists(columnName)) {
		Instances::Log.warning("CX_DataFrame") << "Failed to delete column \"" << columnName << "\". It was not found in the data frame.";
		return false;
	}

	_data.erase(_data.find(columnName));

	std::vector<std::string>::iterator orderIt = std::find(_orderToName.begin(), _orderToName.end(), columnName);
	_orderToName.erase(orderIt);
	
	return true;
}

/*! Deletes the given row of the data frame.
\param row The row to delete (0 indexed). If row is greater than or equal to the number of rows in the data frame, a warning will be logged.
\return `true` if the row was in bounds and was deleted, `false` if the row was out of bounds.
*/
bool CX_DataFrame::deleteRow(RowIndex row) {
	if (row >= _rowCount) {
		Instances::Log.warning("CX_DataFrame") << "Failed to delete row " << row << ". It was out of bounds. Number of rows: " << this->getRowCount();
		return false;
	}

	for (std::map<std::string, std::vector<CX_DataFrameCell>>::iterator col = _data.begin(); col != _data.end(); col++) {
		col->second.erase(col->second.begin() + row);
	}
	_rowCount--;
	return true;
}

/*! Appends the row to the end of the data frame.
\param row The row of data to add. If `row` is empty, an empty row is appended to the CX_DataFrame.
\note If `row` has columns that do not exist in the data frame, those columns will be added to the data frame.
*/
void CX_DataFrame::appendRow(CX_DataFrameRow row) {
	//This implementation looks weird, but don't change it without care: it deals with a number of edge cases.

	_rowCount++; //Increment first so that resizing to _rowCount is the right size.

	for (const std::string& name : row.names()) {

		_tryAddColumn(name, false); // Don't size new columns
		_data.at(name).resize(_rowCount); // But resize all columns (that are in row)

		_data.at(name).back() = row[name].clone();
		//row[name].copyCellTo( &_data.at(name).back() ); //Copy the cell in the row into the data frame.
	}

	// Columns not in row must now be lengthened with empty cells.
	// This deals with the case when the row is missing some columns that the data frame has.
	_equalizeRowLengths();

	//or be about twice as slow with:
	//insertRow(row, _rowCount);
}


/*! Inserts a row into the data frame.
\param row The row of data to insert.
\param beforeIndex The index of the row before which `row` should be inserted. If >= the number of rows currently stored, `row`
will be appended to the end of the data frame.
\note If `row` has columns that do not exist in the data frame, those columns will be added to the data frame.
\note This may be a blocking operation, depending on the size of the data frame.
*/
void CX_DataFrame::insertRow(CX_DataFrameRow row, RowIndex beforeIndex) {

	//Cache row names
	std::vector<std::string> rowNames = row.names();

	//For each new column, add it in the order given by the row
	for (const std::string& name : rowNames) {
		this->_tryAddColumn(name, true);
	}

	//Set up the location at which the new data will be added.
	RowIndex insertIndex = std::min(beforeIndex, this->getRowCount());
	
	//For each existing column, insert one cell then assign new data to that cell
	for (auto existingColumn = this->_data.begin(); existingColumn != this->_data.end(); existingColumn++) {

		//By default, put new items at the end
		std::vector<CX_DataFrameCell>::iterator insertionIterator = existingColumn->second.end();

		//But if the insertion is supposed to come before the end, update the iterator.
		if (beforeIndex < this->getRowCount()) {
			insertionIterator = existingColumn->second.begin() + beforeIndex;
		}

		//For each column, make a new cell, regardless of if it is going to be filled right now.
		existingColumn->second.insert(insertionIterator, CX_DataFrameCell());

		//If this the row had data for this column, copy it over.
		if (Util::contains(rowNames, existingColumn->first)) {

			existingColumn->second[insertIndex] = row[existingColumn->first].clone();

			//row[existingColumn->first].copyCellTo(&(existingColumn->second[insertIndex]));
		}
	}

	//Note that a row has been added
	this->_rowCount++;
}

/*! Returns a vector containing the names of the columns in the data frame.
\return A vector of strings with the column names. */
std::vector<std::string> CX_DataFrame::getColumnNames(void) const {
	return _orderToName;
}

/*! \brief Returns the number of rows in the data frame. */
CX_DataFrame::RowIndex CX_DataFrame::getRowCount(void) const {
	return _rowCount;
}

/*! Re-orders the rows in the data frame.
\param newOrder Vector of row indices. newOrder.size() must equal this->getRowCount(). newOrder must not contain any out-of-range indices
(i.e. they must be < getRowCount()). Both of these error conditions are checked for in the function call and errors are logged.
\return true if all of the conditions of newOrder are met, false otherwise.
*/
bool CX_DataFrame::reorderRows(const std::vector<CX_DataFrame::RowIndex>& newOrder) {
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

	*this = copyRows(newOrder);
	return true;

	/*
	for (unsigned int i = 0; i < newOrder.size(); i++) {
		if (newOrder[i] >= _rowCount) {
			CX::Instances::Log.error("CX_DataFrame") << "reorderRows failed: newOrder contained out-of-range indices.";
			return false;
		}
	}

	std::vector<std::string> names = getColumnNames();

	for (std::vector<std::string>::iterator it = names.begin(); it != names.end(); it++) {

		std::vector<CX_DataFrameCell> columnCopy(_rowCount); //Data must first be copied into new columns,
			//otherwise moving data from one row to another could overwrite a row that hasn't been copied yet.
		for (RowIndex i = 0; i < newOrder.size(); i++) {
			_data[*it][newOrder[i]].copyCellTo( &columnCopy[i] );
		}

		for (RowIndex i = 0; i < newOrder.size(); i++) {
			columnCopy[i].copyCellTo( &_data[*it][newOrder[i]] );
		}

	}
	return true;
	*/
}

/*! Creates a CX_DataFrameRow that contains a copy of the given row of the CX_DataFrame.
This is different from the CX_DataFrameRow returned by operator[](RowIndex), 
which refers back to the original data frame.

\param row The row to copy.
\return A CX_DataFrameRow containing a copy of the row of the CX_DataFrame.
*/
CX_DataFrameRow CX_DataFrame::copyRow(RowIndex row) const {

	CX_DataFrameRow r;

	if (row >= _rowCount) {
		CX::Instances::Log.error("CX_DataFrame") << "copyRow(): row is out of range.";
		return r;
	}

	for (const std::string& col : getColumnNames()) {
		//this->_data.at(col).at(row).copyCellTo(&(r[col]));

		r[col] = _data.at(col).at(row).clone();
	}

	return r;
}

/*! Creates a `CX_DataFrame` containing a copy of the rows specified in `rowOrder`. The new data frame is not linked to the existing data frame.
\param rowOrder A vector of `CX_DataFrame::RowIndex` containing the rows from this data frame to be copied out.
The indices in `rowOrder` may be in any order: They don't need to be ascending. Additionally, the same row to be
copied may be specified multiple times, which will result in multiple copies of that row being created in the new data frame.
\return A `CX_DataFrame` containing the rows specified in `rowOrder`.

\note This function may be \ref blockingCode if the amount of copied data is large.
*/
CX_DataFrame CX_DataFrame::copyRows(std::vector<CX_DataFrame::RowIndex> rowOrder) const {
	unsigned int outOfRangeCount = 0;
	for (unsigned int i = 0; i < rowOrder.size(); i++) {
		if (rowOrder[i] >= _rowCount) {
			rowOrder.erase(rowOrder.begin() + i);
			i--;
			outOfRangeCount++;
		}
	}

	if (outOfRangeCount > 0) {
		CX::Instances::Log.warning("CX_DataFrame") << "copyRows(): rowOrder contained " << outOfRangeCount << " out-of-range indices. They will be ignored.";
	}

	CX_DataFrame copyDf;

	for (const std::string& col : getColumnNames()) {
        copyDf._resizeToFit(col, rowOrder.size() - 1);
        for (RowIndex row = 0; row < rowOrder.size(); row++) {
            //this->_data.at(col)[rowOrder[row]].copyCellTo( &copyDf._data.at(col)[row] );

			copyDf._data.at(col)[row] = _data.at(col)[rowOrder[row]].clone();
        }
	}

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

	/*
	std::vector<std::string> validColumns;
	for (const std::string& col : columns) {
		if (columnExists(col)) {
			validColumns.push_back(col);
		} else {
			Instances::Log.warning("CX_DataFrame") << "copyColumns(): Requested column not found in data frame: " << col;
		}
	}
	*/

	std::vector<std::string> validColumns = Util::intersectionV(columns, this->getColumnNames());
	std::vector<std::string> invalidColumns = Util::exclude(columns, validColumns);
	if (invalidColumns.size() > 0) {
		Instances::Log.warning("CX_DataFrame") << "copyColumns(): Requested columns not found in data frame: " << Util::vectorToString(invalidColumns, ", ");
	}



	CX_DataFrame copyDf;
	for (const std::string& col : validColumns) {

		copyDf._resizeToFit(col, this->getRowCount() - 1);

		for (RowIndex row = 0; row < this->getRowCount(); row++) {

			//this->operator()(col, row).copyCellTo( &copyDf._data.at(col)[row] );

			copyDf._data.at(col)[row] = _data.at(col)[row].clone();
		}
	}

	return copyDf;
}

/*! Randomly re-orders the rows of the data frame.
\param rng Reference to a CX_RandomNumberGenerator to be used for the shuffling. Uses CX::Instances::RNG by default.
\note This function may be \ref blockingCode if the data frame is large.
*/
void CX_DataFrame::shuffleRows(CX_RandomNumberGenerator &rng) {
	vector<CX_DataFrame::RowIndex> newOrder = CX::Util::intVector<CX_DataFrame::RowIndex>(0, _rowCount - 1);
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
\note If the row count is less than the number of rows already in the data frame, it will delete the extra rows.
*/
void CX_DataFrame::setRowCount(RowIndex rowCount) {
	_resizeToFit(rowCount - 1);
}

/*! Adds a column to the data frame.
\param columnName The name of the column to add. If a column with that name already exists in the data frame, a warning will be logged. 
\return `true` if the column was added, `false` otherwise.
*/
bool CX_DataFrame::addColumn(std::string columnName) {
	if (columnExists(columnName)) {
		Instances::Log.notice("CX_DataFrame") << "addColumn(): Column \"" << columnName << "\" already exists in data frame.";
		return false;
	}

	return _tryAddColumn(columnName, true);
}

/*! Appends a data frame to this data frame. 
Internally, CX_DataFrame::appendRow() is used to copy over the rows of `df` one at a time.
\param df The CX_DataFrame to append. */
void CX_DataFrame::append(CX_DataFrame df) {
	for (RowIndex i = 0; i < df.getRowCount(); i++) {
		this->appendRow(df[i]);
	}
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

	for (RowIndex i = 0; i < getRowCount(); i++) {
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
but its name will be changed by appending "_" (underscore). If this new name conflicts with an existing name, the process will
be repeated until the new name does not conflict.
*/
std::vector<std::string> CX_DataFrame::convertVectorColumnToColumns(std::string columnName, int startIndex, bool deleteOriginal, std::string newBaseName) {

	if (!columnExists(columnName) || !columnContainsVectors(columnName)) {
		return std::vector<std::string>();
	}

	if (newBaseName == "") {
		newBaseName = columnName;
	}

	//Copy the data to string vectors.
	std::vector<std::vector<std::string>> vectors(getRowCount()); //The original data; stored by row then column
	size_t maxVectorLength = 0;
	for (RowIndex i = 0; i < vectors.size(); i++) {
		vectors[i] = this->operator()(i, columnName).toVector<std::string>();
		maxVectorLength = std::max<size_t>(maxVectorLength, vectors[i].size());
	}

	//Create new column names and sort out conflicts.
	std::vector<std::string> columnNames(maxVectorLength);
	for (unsigned int i = 0; i < columnNames.size(); i++) {
		columnNames[i] = columnName + ofToString(startIndex + i);

		while (columnExists(columnNames[i])) {
			CX::Instances::Log.warning("CX_DataFrame") << "convertVectorColumnToColumns: New column name " << columnNames[i] << " conflicts with existing column name. "
				"The new column name will be changed to \"" << columnNames[i] << "_\".";
			columnNames[i] + "_";
		}
	}

	//Add the new columns and copy over the data
	for (unsigned int i = 0; i < columnNames.size(); i++) {
		this->addColumn(columnNames[i]);

		for (RowIndex j = 0; j < vectors.size(); j++) {
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





void CX_DataFrame::_resizeToFit(std::string column) {
	if (_tryAddColumn(column, true)) {
		CX::Instances::Log.verbose("CX_DataFrame") << "Data frame resized to fit column \"" << column << "\".";
	}
}

void CX_DataFrame::_resizeToFit(RowIndex row) {
	RowIndex newCount = row + 1;
	if (newCount > _rowCount && _data.size() > 0) {
		_rowCount = std::max(_rowCount, newCount);
		for (std::map<std::string, std::vector<CX_DataFrameCell>>::iterator it = _data.begin(); it != _data.end(); it++) {
			it->second.resize(_rowCount);
		}
		CX::Instances::Log.verbose("CX_DataFrame") << "Data frame resized to fit row " << row << ".";
	}
}

void CX_DataFrame::_resizeToFit(std::string column, RowIndex row) {
	_resizeToFit(column);
	_resizeToFit(row);
}

void CX_DataFrame::_equalizeRowLengths(void) {
	RowIndex maxSize = 0;
	for (std::map<std::string, std::vector<CX_DataFrameCell>>::iterator it = _data.begin(); it != _data.end(); it++) {
		maxSize = std::max(it->second.size(), maxSize);
	}

	for (std::map<std::string, std::vector<CX_DataFrameCell>>::iterator it = _data.begin(); it != _data.end(); it++) {
		it->second.resize(maxSize);
	}
	_rowCount = maxSize;
}

// Returns true if a new column was added
bool CX_DataFrame::_tryAddColumn(std::string column, bool setRowCount) {

	if (columnExists(column)) {
		return false;
	}

	_data.insert(std::pair<std::string, std::vector<CX_DataFrameCell>>(column, std::vector<CX_DataFrameCell>()));

	_orderToName.push_back(column);

	if (setRowCount) {
		_data.at(column).resize(_rowCount);
	}

	return true;
}

void CX_DataFrame::_duplicate(CX_DataFrame* target) const {

	target->clear();

	for (const std::string& col : this->getColumnNames()) {

		target->_resizeToFit(col, this->_rowCount - 1);

		for (RowIndex row = 0; row < this->_rowCount; row++) {
			//this->_data.at(col)[row].copyCellTo(&target->_data.at(col)[row]);

			target->_data.at(col)[row] = this->_data.at(col)[row].clone();
		}
	}
}


std::ostream& operator<< (std::ostream& os, const CX_DataFrame& df) {
	CX_DataFrame::OutputOptions opt;
	//os << opt.vectorEncloser;
	os << df.print(opt);
	//os << opt.vectorEncloser;
	return os;
}

std::istream& operator >> (std::istream& is, CX_DataFrame& df) {
	// See https://stackoverflow.com/a/3203502
	std::istreambuf_iterator<char> eoi; // default-constructed istreambuf_iterator is end-of-file (or end of input).
	std::string dfStr(std::istreambuf_iterator<char>(is), eoi);
	df._readFromString(dfStr, CX_DataFrame::InputOptions(), "operator>>(): ", "from input stream");
	return is;
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
CX_DataFrameCell CX_DataFrameColumn::operator[] (CX_DataFrame::RowIndex row) {
	if (_df) {
		return _df->operator()(row, _columnName);
	} else {
		return _data[row];
	}
}

/*! \brief Returns the number of rows in the column. */
CX_DataFrame::RowIndex CX_DataFrameColumn::size(void) {
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
CX_DataFrameRow::CX_DataFrameRow(CX_DataFrame *df, CX_DataFrame::RowIndex rowNumber) :
	_df(df),
	_rowNumber(rowNumber)
{}

/*! Accesses the element in the specified column of the row. */
CX_DataFrameCell CX_DataFrameRow::operator[] (std::string column) {
	if (_df) {
		return _df->operator()(column, _rowNumber);
	} else {
		if (_data.find(column) == _data.end()) {
			_data.insert(std::pair<std::string, CX_DataFrameCell>());
			_orderToName.push_back(column);
		}

		return _data[column];
	}
}

/*! \brief Returns a vector containing the names of the columns in this row. */
std::vector<std::string> CX_DataFrameRow::names(void) {
	if (_df) {
		return _df->getColumnNames();
	} else {
		return _orderToName;
	}
}

bool CX_DataFrameRow::columnExists(const std::string& column) {
	if (_df) {
		return _df->columnExists(column);
	} else {
		return _data.find(column) != _data.end();
	}
}

bool CX_DataFrameRow::deleteColumn(const std::string& column) {

	if (_df) {
		CX::Instances::Log.error("CX_DataFrameRow") << "deleteColumn(): Cannot delete a column of a CX_DataFrame through a CX_DataFrameRow. See CX_DataFrame::deleteColumn().";
		return false;
	}

	if (!this->columnExists(column)) {
		Instances::Log.warning("CX_DataFrameRow") << "deleteColumn(): Failed to delete column \"" << column << "\". It was not found in the CX_DataFrameRow.";
		return false;
	}

	_data.erase(_data.find(column));

	std::vector<std::string>::iterator orderIt = std::find(_orderToName.begin(), _orderToName.end(), column);
	_orderToName.erase(orderIt);

	return true;
}

/*! \brief Clears the contents of the row. Does not delete the row. */
void CX_DataFrameRow::clear(void) {
	if (_df) {
		std::vector<std::string> names = _df->getColumnNames();
		for (unsigned int i = 0; i < names.size(); i++) {
			_df->operator()(names[i], _rowNumber).clear();
		}
	} else {
		_data.clear();
		_orderToName.clear();
	}
}

} //namespace CX
