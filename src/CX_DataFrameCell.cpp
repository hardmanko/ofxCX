#include "CX_DataFrameCell.h"

using namespace CX;

CX_DataFrameCell::CX_DataFrameCell (void) {
	_allocatePointers();
	//*_str = "NULL"; //?
	*_type = "NULL";
}

/*! Constructs the cell with a string literal, treating it as a std::string. */
CX_DataFrameCell::CX_DataFrameCell(const char* c) {
	_allocatePointers();
	//*_str = c;
	//*_type = typeid(std::string).name();

	this->store<const char*>(c);
	*_type = typeid(std::string).name(); //override the type information with the type of std::string.
	*_dataIsVector = false;
}

void CX_DataFrameCell::_allocatePointers(void) {
	_str = std::shared_ptr<std::string>(new std::string);
	_type = std::shared_ptr<std::string>(new std::string);
	_dataIsVector = std::shared_ptr<bool>(new bool);
}

/*! Assigns a string literal to the cell, treating it as a std::string. */
CX_DataFrameCell& CX_DataFrameCell::operator= (const char* c) {
	//*_str = c;
	this->store<const char*>(c);
	*_type = typeid(std::string).name();
	return *this;
}


/*! Gets a string representing the type of data stored within the cell. This string is implementation-defined
(which is the C++ standards committee way of saying "It can be anything at all"). It is only guranteed to be 
the same for the same type, but not neccessarily be different for different types.
\return A string containing the name of the stored type as given by typeid(typename).name(). */
std::string CX_DataFrameCell::getStoredType(void) {
	if (*_dataIsVector) {
		return "vector<" + *_type + ">";
	}
	return *_type;
}


/*! Copies the contents of this cell to targetCell, including type information.
\param targetCell A pointer to the cell to copy data to.
*/
void CX_DataFrameCell::copyCellTo(CX_DataFrameCell* targetCell) {
	//Note that this breaks horribly if the data is a vector and the other cell is in a different data frame with a different configuration.
	*targetCell->_str = *this->_str; 
	*targetCell->_type = *this->_type;
	*targetCell->_dataIsVector = *this->_dataIsVector;
}

template<> std::string CX_DataFrameCell::to(void) const {
	return this->toString();
}

std::ostream& CX::operator<< (std::ostream& os, const CX_DataFrameCell& cell) {
	os << cell.toString();
	return os;
}