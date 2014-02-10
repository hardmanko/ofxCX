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

CX_DataFrameCell::CX_DataFrameCell(const char* c) {
	_str = std::shared_ptr<std::string>(new std::string);
	_type = std::shared_ptr<std::string>(new std::string);
	*_str = c;
	*_type = typeid(std::string).name();
}

CX_DataFrameCell& CX_DataFrameCell::operator= (const char* c) {
	*_str = c;
	*_type = typeid(std::string).name();
	return *this;
}


/*! Gets a string representing the type of data stored within the cell. This string is implementation-defined
(which is the C++ standards committee way of saying "It can be anything at all"). It is only guranteed to be 
the same for the same type, but not neccessarily be different for different types.
\return A string containing the name of the stored type as given by typeid(typename).name(). */
std::string CX_DataFrameCell::getStoredType(void) {
	return *_type;
}


/*! Copies the contents of this cell to targetCell, including type information.
\param targetCell A pointer to the cell to copy data to.
*/
void CX_DataFrameCell::copyCellTo(CX_DataFrameCell* targetCell) {
	*targetCell->_str = *this->_str;
	*targetCell->_type = *this->_type;
}

template<> std::string CX_DataFrameCell::to(void) const {
	return this->toString();
}