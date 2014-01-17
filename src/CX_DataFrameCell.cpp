#include "CX_DataFrameCell.h"

using namespace std;
using namespace CX;

ostream& operator<< (ostream& os, const CX_DataFrameCell& cell) {
	os << cell.toString();
	return os;
}

CX_DataFrameCell::CX_DataFrameCell (void) :
	_selfAllocated(true)
{
	_str = new string;
}

CX_DataFrameCell::CX_DataFrameCell (const CX_DataFrameCell& r) : 
	_selfAllocated(true)
{
	this->_str = new string(*r._str);
}

CX_DataFrameCell::CX_DataFrameCell (CX_DataFrameCell&& r) {
	this->_str = r._str;
	this->_selfAllocated = r._selfAllocated;

	r._str = nullptr;
	r._selfAllocated = false;
}

//Private
CX_DataFrameCell::CX_DataFrameCell(string *s) : 
	_selfAllocated(false), 
	_str(s) 
{}

CX_DataFrameCell::~CX_DataFrameCell (void) {
	if (_selfAllocated) {
		delete _str;
	}
}

string CX_DataFrameCell::toString (void) const {
	return *_str;
}

void CX_DataFrameCell::_setPointer (std::string *str) {
	if (_selfAllocated) {
		delete _str;
	}
	_str = str;
	_selfAllocated = false;
}