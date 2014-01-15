#include "CX_DataFrame.h"

CX_DataFrame::CX_DataFrame (void) :
	_nrow(0)
{}

string CX_DataFrame::printData (void) {
	stringstream output;
	char delim = '\t';

	output << "Row";
	for (map<string, vector<CX_DataFrameCell>>::iterator it = _data.begin(); it != _data.end(); it++) {
		output << delim << it->first;
	}

	for (unsigned int r = 0; r < _nrow; r++) {
		output << endl << r;
		for (map<string, vector<CX_DataFrameCell>>::iterator it = _data.begin(); it != _data.end(); it++) {
			if (it->second[r].type == "null") {
				output << delim << "NULL";
			} else if (it->second[r].type == "string") {
				output << delim << *(string*)it->second[r].data;
			} else if (it->second[r].type == "double") {
				output << delim << *(double*)it->second[r].data;
			} else if (it->second[r].type == "int64_t") {
				output << delim << *(int64_t*)it->second[r].data;
			} else {
				output << delim << "NULL";
			}
		}
	}

	return output.str();
}

void CX_DataFrame::_prepareToAddData (string column, unsigned int row) {
	if (_data[column].size() <= row) {
		_data[column].resize(row + 1);
		_checkRowLengths();
	}
	delete _data[column][row].data;
}

void CX_DataFrame::_checkRowLengths (void) {
	unsigned int maxRowLength = 0;

	for (map<string, vector<CX_DataFrameCell>>::iterator it = _data.begin(); it != _data.end(); it++) {
		if (it->second.size() > maxRowLength) {
			maxRowLength = it->second.size();
		}
	}

	for (map<string, vector<CX_DataFrameCell>>::iterator it = _data.begin(); it != _data.end(); it++) {
		string col = it->first;
		_data[col].resize(maxRowLength);
	}

	_nrow = maxRowLength;

}

void* CX_DataFrame::getDataPointer (string column, unsigned int row) {
	return _data[column][row].data;
}

template <>
void CX_DataFrameCell::setDataType<string> (void) {
	this->type = "string";
}

template <>
void CX_DataFrameCell::setDataType<double> (void) {
	this->type = "double";
}

template <>
void CX_DataFrameCell::setDataType<int64_t> (void) {
	this->type = "int64_t";
}


template <>
void CX_DataFrameCell::setDataType<const char*> (void) {
	this->type = "string";
}








/*
template <>
void CX_TypedDataFrame<string>::set (string column, unsigned int row, string data) {
	set(column, row, data);
}
*/


void CX_StringDataFrame::set (string column, unsigned int row, string data) {
	_boundsCheck(column, row);

	_data[column][row] = data;
}

void CX_StringDataFrame::set (string column, unsigned int row, const char* data) {
	set( column, row, string(data) );
}








