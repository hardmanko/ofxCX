#include "CX_DataFrameCell.h"

using namespace CX;

std::ostream& CX::operator<< (std::ostream& os, const CX_DataFrameCell& cell) {
	os << cell.toString();
	return os;
}

CX_DataFrameCell::CX_DataFrameCell (void) {
	_str = std::shared_ptr<std::string>(new std::string);
	_type = std::shared_ptr<std::string>(new std::string);
	//*_str = "NULL"; //?
	*_type = "NULL";
}

CX_DataFrameCell& CX_DataFrameCell::operator= (const char* c) {
	*_str = c;
	*_type = typeid(std::string).name();
	return *this;
}

/* Copies the cell. Includes type information with the copy. Assigning to another
CX_DataFrameCell using operator= does not perform a copy operation.
\return A copy of the cell. */
/*
CX_DataFrameCell CX_DataFrameCell::copyCell (void) {
	CX_DataFrameCell newCell;
	newCell._str = std::shared_ptr<std::string>(new std::string);
	newCell._type = std::shared_ptr<std::string>(new std::string);
	*newCell._str = this->toString();
	*newCell._type = this->getStoredType();
	return newCell;
}
*/

/*! Gets a string representing the type of data stored within the cell. This string is implementation-defined
(which is the C++ standards committee way of saying "It can be anything at all"). It is only guranteed to be 
the same for the same type, but not neccessarily be different for different types.
\return A string containing the name of the stored type as given by typeid(typename).name(). */
std::string CX_DataFrameCell::getStoredType(void) {
	return *_type;
}

/*
CX_DataFrameCell& CX_DataFrameCell::operator= (const CX_DataFrameCell& cell) {
	*this->_str = *cell._str;
	*this->_type = *cell._type;
	return *this;
}
*/
/*
CX_DataFrameCell::CX_DataFrameCell (const CX_DataFrameCell& cell) {
	*this->_str = *cell._str;
	*this->_type = *cell._type;
}
*/

void CX_DataFrameCell::copyCellTo(CX_DataFrameCell& targetCell) {
	*targetCell._str = *this->_str;
	*targetCell._type = *this->_type;
}