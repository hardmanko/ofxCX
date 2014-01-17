#include "CX_DataFrame.h"

using namespace CX;

CX_DataFrame::DataFrameConfiguration CX_DataFrame::Configuration = { ",", "\"", "\"", "\t"};

CX_DataFrame::CX_DataFrame (void) :
	_rowCount(0)
{}

CX_DataFrameCell CX_DataFrame::operator() (string column, rowIndex_t row) {
	_resizeToFit(column, row);
	return CX_DataFrameCell(&_data[column][row]);
}

CX_DataFrameCell CX_DataFrame::operator() (rowIndex_t row, string column) {
	return this->operator()(column, row);
}

CX_DataFrameColumn CX_DataFrame::operator[] (string column) {
	return _getColumn(column);
}

CX_DataFrameRow CX_DataFrame::operator[] (rowIndex_t row) {
	return _getRow(row);
}

string CX_DataFrame::print (const set<string>& columns, const vector<rowIndex_t>& rows, string delimiter, bool printRowNumbers) {
	stringstream output;

	if (printRowNumbers) {
		output << "row" << delimiter;
	}

	for (map<string, vector<string>>::iterator it = _data.begin(); it != _data.end(); it++) {
		if (columns.find(it->first) != columns.end()) {
			if (it != _data.begin()) {
				output << delimiter;
			}
			output << it->first;
		}
	}

	for (rowIndex_t i = 0; i < rows.size(); i++) {
		output << endl;
		if (printRowNumbers) {
			output << rows[i] << delimiter;
		}

		for (map<string, vector<string> >::iterator it = _data.begin(); it != _data.end(); it++) {
			if (columns.find(it->first) != columns.end()) {
				if (it != _data.begin()) {
					output << delimiter;
				}
				output << it->second[rows[i]];
			}
		}
	}
	output << endl;

	return output.str();
}

string CX_DataFrame::print (string delimiter) {
	return print(CX::uintVector(0, rowCount() - 1), delimiter);
}

string CX_DataFrame::print (const set<string>& columns, string delimiter) {
	return print(columns, CX::uintVector(0, rowCount() - 1), delimiter);
}

string CX_DataFrame::print (const vector<rowIndex_t>& rows, string delimiter) {
	set<string> columns;
	vector<string> names = columnNames();
	for (rowIndex_t i = 0; i < names.size(); i++) {
		columns.insert( names[i] );
	}
	return print(columns, rows, delimiter);
}

bool CX_DataFrame::printToFile (string filename) {
	return CX::writeToFile(filename, this->print("\t"), false);
}

bool CX_DataFrame::printToFile (string filename, const set<string>& columns, const vector<rowIndex_t>& rows, string delimiter, bool printRowNumbers) {
	return CX::writeToFile(filename, this->print(columns, rows, delimiter, printRowNumbers), false);
}

void CX_DataFrame::appendRow (CX_DataFrameRow row) {
	//This implementation looks really weird, but don't change it: it deals with a number of edge cases.
	_rowCount++;

	for (map<string,CX_DataFrameCell>::iterator it = row.begin(); it != row.end(); it++) {
		_data[it->first].resize(_rowCount);
		_data[it->first].back() = it->second.toString();
	}

	_equalizeRowLengths(); //This deals with the case when the row doesn't have the same columns as the rest of the data frame
}

vector<string> CX_DataFrame::columnNames (void) {
	vector<string> names;
	for (map<string,vector<string>>::iterator it = _data.begin(); it != _data.end(); it++) {
		names.push_back( it->first );
	}
	return names;
}

void CX_DataFrame::_resizeToFit (string column, rowIndex_t row) {
	if (_data[column].size() <= row) {
		_rowCount = std::max(_rowCount, row + 1);
		for (map<string, vector<string>>::iterator it = _data.begin(); it != _data.end(); it++) {
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
	for (map<string, vector<string>>::iterator it = _data.begin(); it != _data.end(); it++) {
		maxSize = std::max( _data[it->first].size(), maxSize );
	}

	for (map<string, vector<string>>::iterator it = _data.begin(); it != _data.end(); it++) {
		_data[it->first].resize(maxSize);
	}
	_rowCount = maxSize;
}

vector<CX_DataFrameCell> CX_DataFrame::_getColumn (string column) {
	_resizeToFit(column);
	vector<CX_DataFrameCell> rval( rowCount() );
	for (rowIndex_t i = 0; i < rowCount(); i++) {
		rval[i] = CX_DataFrameCell( &_data[column][i] );
	}
	return rval;
}

map<string, CX_DataFrameCell> CX_DataFrame::_getRow (unsigned int row) {
	_resizeToFit(row);

	map<string, CX_DataFrameCell> r;
	for (map<string, vector<string>>::iterator it = _data.begin(); it != _data.end(); it++) {
		r[it->first]._setPointer( &it->second[row] );
	}
	return r;
}



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