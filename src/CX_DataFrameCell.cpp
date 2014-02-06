#include "CX_DataFrameCell.h"

using namespace CX;

std::ostream& CX::operator<< (std::ostream& os, const CX_DataFrameCell& cell) {
	os << cell.toString();
	return os;
}

CX_DataFrameCell::CX_DataFrameCell (void) {
	_str = std::shared_ptr<std::string>(new std::string);
	_type = std::shared_ptr<std::string>(new std::string);
	*_type = "No type";
}

CX_DataFrameCell& CX_DataFrameCell::operator= (const char* c) {
	*_str = c;
	*_type = typeid(std::string).name();
	return *this;
}

/*
string CX_DataFrameCell::toString (void) const {
	return *_str;
}
*/

/*
bool CX_DataFrameCell::toBool (void) const {
	return this->to<bool>();
}

int CX_DataFrameCell::toInt (void) const {
	return this->to<int>();
}

double CX_DataFrameCell::toDouble (void) const {
	return this->to<double>();
}
*/