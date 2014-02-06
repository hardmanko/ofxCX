#ifndef _CX_DATA_FRAME_CELL_H_
#define _CX_DATA_FRAME_CELL_H_

#include <vector>
#include <string>
#include <sstream>
#include <memory> //shared_pointer
#include <iostream>
#include <typeinfo>

#include "ofUtils.h"

#include "CX_Utilities.h"
#include "CX_Logger.h"

namespace CX {

/*! This class manages the contents of a single cell in a CX_DataFrame. It handles all of the type conversion nonsense
that goes on when data is inserted into or extracted from a data frame. It tracks the type of the data that is inserted
or extracted and logs warnings if the inserted type does not match the extracted type, with a few exceptions (see notes). 

\note There are a few exceptions to the type tracking. If the inserted type is const char*, it is treated as a string.
Additionally, you can extract anything as string without a warning. This is because the data is stored as a string 
internally so extracting the data as a string is a lossless operation.
\ingroup data
*/
class CX_DataFrameCell {
public:

	CX_DataFrameCell (void);
	template <typename T> CX_DataFrameCell (const T& value);
	template <typename T> CX_DataFrameCell (const std::vector<T>& values);

	CX_DataFrameCell& operator= (const char* c);
	template <typename T> CX_DataFrameCell& operator= (const T& value);
	template <typename T> CX_DataFrameCell& operator= (const std::vector<T>& values);

	template <typename T> operator T (void) const;
	template <typename T> operator std::vector<T> (void) const;

	template <typename T> T to(void) const;
	template<> std::string to<std::string>(void) const;
	std::string toString(void) const { return *_str; };
	bool toBool(void) const { return this->to<bool>(); };
	int toInt(void) const { return this->to<int>(); };
	double toDouble(void) const { return this->to<double>(); };


	template <typename T> std::vector<T> toVector (void) const;
	template <typename T> void storeVector (std::vector<T> values);

private:
	std::shared_ptr<std::string> _str;
	std::shared_ptr<std::string> _type;

	template <typename T>
	std::string _toString (const T& value) {
		return ofToString<T>(value, 16);
	}

	template <typename T>
	std::string _vectorToString (std::vector<T> values, std::string delimiter, int significantDigits) {
		std::stringstream s;
		s << std::fixed << std::setprecision(significantDigits);
		for (unsigned int i = 0; i < values.size(); i++) {
			s << _toString(values[i]);
			if (i != values.size() - 1) {
				s << delimiter;
			}
		}
		return s.str();
	}
};

template <typename T> 
CX_DataFrameCell::CX_DataFrameCell (const T& value) {
	_str = std::shared_ptr<std::string>(new std::string);
	_type = std::shared_ptr<std::string>(new std::string);
	*_str = _toString<T>(value);
	*_type = typeid(T).name();
}

template <typename T> 
CX_DataFrameCell::CX_DataFrameCell (const std::vector<T>& values) {
	_str = std::shared_ptr<std::string>(new std::string);
	_type = std::shared_ptr<std::string>(new std::string);
	storeVector<T>(values);
}

template <typename T>
CX_DataFrameCell& CX_DataFrameCell::operator= (const T& value) {
	*_str = _toString(value);
	*_type = typeid(T).name();
	return *this;
}

template <typename T>
CX_DataFrameCell& CX_DataFrameCell::operator= (const std::vector<T>& values) {
	storeVector<T>(values);
	return *this;
}

/*! Attempts to convert the contents of the cell to T using to(). */
template <typename T>
CX_DataFrameCell::operator T (void) const {
	return this->to<T>();
}

template <typename T>
CX_DataFrameCell::operator std::vector<T> (void) const {
	return toVector<T>();
}

/*! Attempts to convert the contents of the cell to type T. There are a variety of reasons
why this conversion can fail and they all center on the user inserting data of one type and
then attempting to extract data of a different type. Regardless of whether the conversion is
possible, if you try to extract a type that is different from the type that is stored in the
cell, a warning will be logged.
\tparam <T> The type to convert to.
\return The data in the cell converted to T.
*/
template <typename T> 
T CX_DataFrameCell::to (void) const {
	std::string typeName = typeid(T).name();
	if (*_type != typeName) {
		CX::Instances::Log.warning("CX_DataFrameCell") << "to: Attempt to extract data of different type than was inserted:" << 
			" Inserted type was \"" << *_type << "\" and attempted extracted type was \"" << typeName << "\".";
	}
	return ofFromString<T>(*_str);
}

template <typename T> 
std::vector<T> CX_DataFrameCell::toVector (void) const {
	std::string typeName = typeid(T).name();
	if (*_type != "vector<" + typeName + ">") {
		CX::Instances::Log.warning("CX_DataFrameCell") << "toVector: Attempt to extract data of different type than was inserted:" <<
			" Inserted type was \"" << *_type << "\" and attempted extracted type was \"vector<" + typeName + ">" << "\".";
	}

	std::string encodedVect = *_str;
	encodedVect = encodedVect.substr(1, encodedVect.size() - 2); //Strip off the quotes at either end. ofStringReplace(encodedVect, "\"", "");

	std::vector<std::string> parts = ofSplitString(encodedVect, ";");

	std::vector<T> values;
	for (unsigned int i = 0; i < parts.size(); i++) {
		values.push_back( ofFromString<T>( parts[i] ) );
	}
	return values;
}

template <typename T> 
void CX_DataFrameCell::storeVector (std::vector<T> values) {
	*_str = "\"" + CX::Util::vectorToString(values, ";", 16) + "\"";

	*_type = "vector<";
	*_type += typeid(T).name();
	*_type += ">";
}

template<>
std::string CX_DataFrameCell::to<std::string>(void) const {
	return *_str;
}

std::ostream& operator<< (std::ostream& os, const CX_DataFrameCell& cell);

}

#endif //_CX_DATA_FRAME_CELL_H_