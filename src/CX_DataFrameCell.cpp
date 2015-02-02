#include "CX_DataFrameCell.h"

namespace CX {

unsigned int CX_DataFrameCell::_floatingPointPrecision = std::numeric_limits<double>::max_digits10;

/*! Set the precision with which floating point numbers (`float`s and `double`s) are stored, in number of significant digits.
This value will be used for all `CX_DataFrameCell`s. Changing this value after storing data will
not change the precision of that data. Defaults to 20 significant digits.
\param prec The number of significant digits.
\note The fact that floating point values are (potentially) stored with less than full precision is one
of the reasons that `CX_DataFrame`s should not be used for numerical analysis, just storage.
*/
void CX_DataFrameCell::setFloatingPointPrecision(unsigned int prec) {
	_floatingPointPrecision = prec;
}

/*! Get the current floating point precision, set by CX_DataFrameCell::setFloatingPointPrecision(). */
unsigned int CX_DataFrameCell::getFloatingPointPrecision(void) {
	return _floatingPointPrecision;
}

CX_DataFrameCell::CX_DataFrameCell(void) {
	_allocatePointers();
	*_type = "NULL";
}

/*! Constructs the cell with a string literal, treating it as a std::string. */
CX_DataFrameCell::CX_DataFrameCell(const char* c) {
	_allocatePointers();

	this->store<const char*>(c);
	*_type = typeid(std::string).name(); //override the type information with the type of std::string.
}

void CX_DataFrameCell::_allocatePointers(void) {
	_data = std::shared_ptr<std::vector<std::string>>(new std::vector<std::string>);
	_type = std::shared_ptr<std::string>(new std::string);
	_ignoreStoredType = std::shared_ptr<bool>(new bool);
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
std::string CX_DataFrameCell::getStoredType(void) const {
	if (*_ignoreStoredType) {
		return "Data type ignored (type deleted or unknown).";
	}

	if (this->isVector()) {
		return "vector<" + *_type + ">";
	}
	return *_type;
}

/*! If for whatever reason the type of the data stored in the CX_DataFrameCell should
be ignored, you can delete it with this function. */
void CX_DataFrameCell::deleteStoredType(void) {
	*_ignoreStoredType = true;
}

/*! Copies the contents of this cell to targetCell, including type information.
\param targetCell A pointer to the cell to copy data to.
*/
void CX_DataFrameCell::copyCellTo(CX_DataFrameCell* targetCell) const {
	*targetCell->_data = *this->_data;
	*targetCell->_type = *this->_type;
}

/*! Equivalent to a call to toString(). This is specialized because it skips the type checks of to<T>.
\return A copy of the stored data encoded as a string. */
template<> std::string CX_DataFrameCell::to(void) const {
	if (_data->size() == 0) {
		CX::Instances::Log.error("CX_DataFrameCell") << "to(): No data to extract from cell.";
		return std::string();
	}

	if (_data->size() > 1) {
		CX::Instances::Log.warning("CX_DataFrameCell") << "to(): Attempt to extract a scalar when the stored data was a vector. Only the first value of the vector will be returned.";
	}

	return _data->at(0);
}

/*! \brief Equivalent to a call to to<string>(). */
std::string CX_DataFrameCell::toString(void) const {
	return this->to<std::string>();
}

/*! \brief Returns `true` if more than one element is stored in the CX_DataFrameCell. */
bool CX_DataFrameCell::isVector(void) const {
	return _data->size() > 1;
}

/*! \brief Delete the contents of the cell. */
void CX_DataFrameCell::clear(void) {
	_data->clear();
	this->deleteStoredType();
}

/*! Converts the contents of the CX_DataFrame cell to a vector of strings. */
template<> std::vector< std::string > CX_DataFrameCell::toVector(void) const {

	//But why is an empty vector an error? An empty scalar can be thought of as an error, but an empty vector should be fine.
	if (_data->size() == 0) {
		CX::Instances::Log.error("CX_DataFrameCell") << "toVector(): No data to extract from cell.";
	}

	return *_data;
}

/*! \brief Stream insertion operator for a CX_DataFrameCell. It simply prints the contents of the CX_DataFrameCell in a pretty way. */
std::ostream& operator<< (std::ostream& os, const CX_DataFrameCell& cell) {
	std::string s;
	if (cell.isVector()) {
		s = Util::vectorToString(cell.toVector<string>(), "; ");
	} else {
		s = cell.toString();
	}

	os << s;
	return os;
}

} //namespace CX
