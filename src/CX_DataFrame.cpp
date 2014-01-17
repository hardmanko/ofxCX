#include "CX_DataFrame.h"

CX_DataFrame::DataFrameConfiguration CX_DataFrame::Configuration = { ",", "\"", "\"", "\t"};
bool CX_DataFrame::tf = true;


ostream& operator<< (ostream& os, const CX_DataFrameCell& cell) {
	os << cell.toString();
	return os;
}