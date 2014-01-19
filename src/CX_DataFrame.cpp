#include "CX_DataFrame.h"

using namespace CX;

CX_DataFrame::DataFrameConfiguration CX_DataFrame::Configuration = { ",", "\"", "\"", "\t"};

CX_DataFrame::CX_DataFrame (void) :
	_rowCount(0)
{}

CX_DataFrameCell CX_DataFrame::operator() (string column, rowIndex_t row) {
	_resizeToFit(column, row);
	return _data[column][row];
}

CX_DataFrameCell CX_DataFrame::operator() (rowIndex_t row, string column) {
	return this->operator()(column, row);
}

CX_DataFrameColumn CX_DataFrame::operator[] (string column) {
	return CX_DataFrameColumn(this, column);
}

CX_DataFrameRow CX_DataFrame::operator[] (rowIndex_t row) {
	return CX_DataFrameRow(this, row);
}

string CX_DataFrame::print (string delimiter, bool printRowNumbers) {
	return print(CX::uintVector(0, getRowCount() - 1), delimiter, printRowNumbers);
}

string CX_DataFrame::print (const set<string>& columns, string delimiter, bool printRowNumbers) {
	return print(columns, CX::uintVector(0, getRowCount() - 1), delimiter, printRowNumbers);
}

string CX_DataFrame::print (const vector<rowIndex_t>& rows, string delimiter, bool printRowNumbers) {
	set<string> columns;
	vector<string> names = columnNames();
	for (rowIndex_t i = 0; i < names.size(); i++) {
		columns.insert( names[i] );
	}
	return print(columns, rows, delimiter, printRowNumbers);
}

string CX_DataFrame::print (const set<string>& columns, const vector<rowIndex_t>& rows, string delimiter, bool printRowNumbers) {
	stringstream output;

	if (printRowNumbers) {
		output << "row" << delimiter;
	}

	for (map<string, vector<CX_DataFrameCell>>::iterator it = _data.begin(); it != _data.end(); it++) {
		if (columns.find(it->first) != columns.end()) {
			if (it != _data.begin()) {
				output << delimiter;
			}
			output << it->first;
		}
	}

	for (rowIndex_t i = 0; i < rows.size(); i++) {
		//Skip invalid row numbers
		if (rows[i] >= _rowCount) {
			continue;
		}

		output << endl;
		if (printRowNumbers) {
			output << rows[i] << delimiter;
		}

		for (map<string, vector<CX_DataFrameCell> >::iterator it = _data.begin(); it != _data.end(); it++) {
			if (columns.find(it->first) != columns.end()) {
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

bool CX_DataFrame::printToFile (string filename, string delimiter, bool printRowNumbers) {
	return CX::writeToFile(filename, this->print(delimiter, printRowNumbers), false);
}

bool CX_DataFrame::printToFile (string filename, const set<string>& columns, string delimiter, bool printRowNumbers) {
	return CX::writeToFile(filename, this->print(columns, delimiter, printRowNumbers), false);
}

bool CX_DataFrame::printToFile (string filename, const vector<rowIndex_t>& rows, string delimiter, bool printRowNumbers) {
	return CX::writeToFile(filename, this->print(rows, delimiter, printRowNumbers), false);
}

bool CX_DataFrame::printToFile (string filename, const set<string>& columns, const vector<rowIndex_t>& rows, string delimiter, bool printRowNumbers) {
	return CX::writeToFile(filename, this->print(columns, rows, delimiter, printRowNumbers), false);
}

void CX_DataFrame::appendRow (CX_DataFrameRow row) {
	//This implementation looks weird, but don't change it: it deals with a number of edge cases.
	_rowCount++;
	vector<string> names = row.names();
	for (unsigned int i = 0; i < names.size(); i++) {
		_data[names[i]].resize(_rowCount);
		_data[names[i]].back() = row[names[i]].toString(); //Copy the string data to the cell in the data frame.
	}

	_equalizeRowLengths(); //This deals with the case when the row doesn't have the same columns as the rest of the data frame

}

vector<string> CX_DataFrame::columnNames (void) {
	vector<string> names;
	for (map<string,vector<CX_DataFrameCell>>::iterator it = _data.begin(); it != _data.end(); it++) {
		names.push_back( it->first );
	}
	return names;
}

void CX_DataFrame::_resizeToFit (string column, rowIndex_t row) {
	if (_data[column].size() <= row) {
		_rowCount = std::max(_rowCount, row + 1);
		for (map<string, vector<CX_DataFrameCell>>::iterator it = _data.begin(); it != _data.end(); it++) {
			_data[it->first].resize(_rowCount);
		}
		//CX::Instances::Log.verbose("CX_DataFrame") << "Data frame resized to fit (\"" << column << "\", " << row << ")";
	}
}

//This function fails if there are no columns in the data frame
void CX_DataFrame::_resizeToFit (rowIndex_t row) {
	if ((row >= _rowCount) && (_data.size() != 0)) {
		_data.begin()->second.resize(row + 1);
		_equalizeRowLengths();
		//CX::Instances::Log.verbose("CX_DataFrame") << "Data frame resized to fit row " << row;
	}
}

void CX_DataFrame::_resizeToFit (string column) {
	if (_data[column].size() != _rowCount) {
		_equalizeRowLengths();
		//CX::Instances::Log.verbose("CX_DataFrame") << "Data frame resized to fit column " << column;
	}
}

void CX_DataFrame::_equalizeRowLengths (void) {
	rowIndex_t maxSize = 0;
	for (map<string, vector<CX_DataFrameCell>>::iterator it = _data.begin(); it != _data.end(); it++) {
		maxSize = std::max( _data[it->first].size(), maxSize );
	}

	for (map<string, vector<CX_DataFrameCell>>::iterator it = _data.begin(); it != _data.end(); it++) {
		_data[it->first].resize(maxSize);
	}
	_rowCount = maxSize;
}


////////////////////////
// CX_DataFrameColumn //
////////////////////////

CX_DataFrameColumn::CX_DataFrameColumn (void) :
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

CX_DataFrame::rowIndex_t CX_DataFrameColumn::size (void) {
	if (_df) {
		return _df->getRowCount();
	} else {
		return _data.size();
	}
}


/////////////////////
// CX_DataFrameRow //
/////////////////////

CX_DataFrameRow::CX_DataFrameRow (void) :
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

vector<std::string> CX_DataFrameRow::names (void) {
	if (_df) {
		return _df->columnNames();
	} else {
		std::vector<std::string> names;
		for (map<std::string, CX_DataFrameCell>::iterator it = _data.begin(); it != _data.end(); it++) {
			names.push_back( it->first );
		}
		return names;
	}
}

void CX_DataFrameRow::clear (void) {
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
	} catch (...) {
		CX::Instances::Log.error("CX_SafeDataFrame") << "Out of bounds access with operator() on indices (\"" << column << "\", " << row << ")";
	}
	return CX_DataFrameCell();
}

CX_DataFrameCell CX_SafeDataFrame::operator() (rowIndex_t row, std::string column) {
	return this->operator()(column, row);
}

CX_DataFrameCell CX_SafeDataFrame::at (rowIndex_t row, std::string column) {
	try {
		return _data.at(column).at(row);
	} catch (std::exception& e) {
		std::stringstream s;
		s << "CX_SafeDataFrame: Out of bounds access with at() on indices (\"" << column << "\", " << row << ")";
		throw std::exception(s.str().c_str());
	}
}

CX_DataFrameCell CX_SafeDataFrame::at (std::string column, rowIndex_t row) {
	return at(row, column);
}

void CX_SafeDataFrame::setRowCount (rowIndex_t rowCount) {
	_resizeToFit(rowCount - 1);
}

void CX_SafeDataFrame::addColumn (std::string columnName) {
	_data[columnName].resize(_rowCount);
}