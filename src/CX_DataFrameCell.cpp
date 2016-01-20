#include "CX_DataFrameCell.h"

namespace CX {

unsigned int CX_DataFrameCell::_floatingPointPrecision = std::numeric_limits<double>::max_digits10;

/*! Set the precision with which floating point numbers (`float`s and `double`s) are stored, in number of significant digits.
This value will be used for all `CX_DataFrameCell`s. Changing this value after storing data will
not change the precision of the stored data.

Defaults to `std::numeric_limits<double>::max_digits10` significant digits. To quote cppreference.com,
"The value of std::numeric_limits<T>::max_digits10 is the number of base-10 digits that are necessary to
uniquely represent all distinct values of the type T, such as necessary for serialization/deserialization to text."
That is to say, the default value is sufficient for lossless conversion between a double precision float
and the string representation of that value stored by the CX_DataFrameCell.

\param prec The number of significant digits.
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

/*! Constructs the cell with a string literal, treating it's type as the same as a `std::string`. */
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

/*! Assigns a string literal to the cell, treating it's type as the same as a `std::string`. */
CX_DataFrameCell& CX_DataFrameCell::operator= (const char* c) {
	//*_str = c;
	this->store<const char*>(c);
	*_type = typeid(std::string).name();
	return *this;
}


/*! Gets a string representing the type of data stored within the cell. This string is implementation-defined
(which is the C++ standards committee way of saying "It can be anything at all"). It is only guranteed to be
the same for the same type, but not neccessarily be different for different types.

You can test if the type of data stored in a CX_DataFrameCell is some type T with the following code snippet.
\code{.cpp}
CX_DataFrameCell cell; //Assume this actually has some data in it.
bool typesMatch = (cell.getStoredType() == typeid(T).name())
\endcode

If the stored data is a vector (i.e. has length > 1), the returned string is "vector<TID>", where
"TID" is replaced with the type string. In other words, you can test if the stored type is a vector<T>
by looking for "vector<" at the beginning of the return value of this function, getting the type name
between the surrounding angle brackets, and then using ` typeid(T).name()` as in the code snippet above.

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
template<> std::string CX_DataFrameCell::to(bool log) const {
	if (_data->size() == 0) {
		if (log) {
			CX::Instances::Log.error("CX_DataFrameCell") << "to(): No data to extract from cell.";
		}
		return std::string();
	}

	if (log && (_data->size() > 1)) {
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

/*! \brief Returns the number of elements stored in the cell. */
unsigned int CX_DataFrameCell::size(void) const {
	return _data->size();
}

/*! \brief Delete the contents of the cell. */
void CX_DataFrameCell::clear(void) {
	_data->clear();
	this->deleteStoredType();
}

/*! Converts the contents of the CX_DataFrame cell to a vector of strings. */
template<> std::vector< std::string > CX_DataFrameCell::toVector(bool log) const {

	//But why is an empty vector an error? An empty scalar can be thought of as an error, but an empty vector should be fine.
	if (log && (_data->size() == 0)) {
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
