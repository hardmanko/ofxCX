#pragma once

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
\ingroup dataManagement
*/
class CX_DataFrameCell {
public:

	CX_DataFrameCell (void);
	CX_DataFrameCell(const char* c);
	template <typename T> CX_DataFrameCell (const T& value); //!< Construct the cell, assigning the value to it.
	template <typename T> CX_DataFrameCell (const std::vector<T>& values); //!< Construct the cell, assigning the values to it.
	
	CX_DataFrameCell& operator= (const char* c);
	template <typename T> CX_DataFrameCell& operator= (const T& value); //!< Assigns a value to the cell.
	template <typename T> CX_DataFrameCell& operator= (const std::vector<T>& values); //!< Assigns a vector of values to the cell.

	template <typename T> operator T (void) const; //!< Attempts to convert the contents of the cell to T using to().
	template <typename T> operator std::vector<T> (void) const; //!< Attempts to convert the contents of the cell to vector<T> using toVector<T>().

	template <typename T> void store(const T& value);

	template <typename T> T to(void) const;

	std::string toString(void) const;

	//! Returns a copy of the stored data converted to bool. Equivalent to to<bool>().
	bool toBool(void) const { return this->to<bool>(); };

	//! Returns a copy of the stored data converted to int. Equivalent to to<int>().
	int toInt(void) const { return this->to<int>(); };

	//! Returns a copy of the stored data converted to double. Equivalent to to<double>().
	double toDouble(void) const { return this->to<double>(); };

	template <typename T> std::vector<T> toVector (void) const;
	template <typename T> void storeVector (std::vector<T> values);

	void copyCellTo(CX_DataFrameCell* targetCell);

	std::string getStoredType (void) const;
	void deleteStoredType(void);

	bool isVector(void) const {
		return _data->size() > 1;
	}
	
private:

	std::shared_ptr<std::vector<std::string>> _data;

	//std::shared_ptr<std::string> _str;
	std::shared_ptr<std::string> _type;
	//std::shared_ptr<bool> _dataIsVector;
	std::shared_ptr<bool> _ignoreStoredType;
	//std::shared_ptr<std::size_t> _typeHash; //Could make type comparison go much faster
	/*
	// For linking back to data frame
	friend class CX_DataFrame;
	void _setOwner(CX_DataFrame *df, std::string column, CX_DataFrame::rowIndex_t row) {
		//Because the cell is changing owners, the data storage format may be changing. If the stored data is a vector
		//it needs to be read out and then read back in with the new settings.
		vector<std::string> strings;
		std::string oldType = *_type;
		if (*_dataIsVector) {
			strings = toVector<std::string>(); //This will likely result in warnings. Maybe temporarily suppress warnings?
		}
		


		_df = df;
		_column = column;
		_row = row;

		if (*_dataIsVector) {
			storeVector<string>(strings);
			*_type = oldType;
		}

	}
	CX_DataFrame *_df;
	std::string _column;
	CX_DataFrame::rowIndex_t _row;
	*/

	unsigned int _getFloatingPointPrecision(void) const {
		return 16;
	}

	template <typename T>
	std::string _toString (const T& value) {
		return ofToString<T>(value, _getFloatingPointPrecision());
	}

	void _allocatePointers(void);

};

template <typename T> 
CX_DataFrameCell::CX_DataFrameCell (const T& value) {
	_allocatePointers();
	this->store(value);
}

template <typename T> 
CX_DataFrameCell::CX_DataFrameCell (const std::vector<T>& values) {
	_allocatePointers();
	storeVector<T>(values);
}

template <typename T>
CX_DataFrameCell& CX_DataFrameCell::operator= (const T& value) {
	this->store(value);
	return *this;
}

template <typename T>
CX_DataFrameCell& CX_DataFrameCell::operator= (const std::vector<T>& values) {
	storeVector<T>(values);
	return *this;
}

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

	if (_data->size() == 0) {
		CX::Instances::Log.error("CX_DataFrameCell") << "to(): No data to extract from cell.";
		return T();
	}

	if (_data->size() > 1) {
		CX::Instances::Log.warning("CX_DataFrameCell") << "to(): Attempt to extract a scalar when the stored data was a vector. Only the first value of the vector will be returned.";
	}

	std::string typeName = typeid(T).name();
	if (*_type != typeName) {
		CX::Instances::Log.warning("CX_DataFrameCell") << "to(): Attempt to extract data of different type than was inserted:" << 
			" Inserted type was \"" << *_type << "\" and attempted extracted type was \"" << typeName << "\".";
	}

	return ofFromString<T>(_data->at(0));
}

/*! Returns a copy of the contents of the cell converted to a vector of the given type. If the type
of data stored in the cell was not a vector of the given type or the type does match but it was a 
scalar that is stored, the logs a warning but attempts the conversion anyway.
\tparam <T> The type of the elements of the returned vector.
\return A vector containing the converted data.
*/
template <typename T> 
std::vector<T> CX_DataFrameCell::toVector (void) const {
	std::string extractedTypeName = typeid(T).name();

	if (_data->size() == 1) {
		CX::Instances::Log.notice("CX_DataFrameCell") << "toVector(): Attempt to extract a vector when the stored data was a scalar. The returned vector will be of length one.";
	}

	if (*_type != extractedTypeName) {
		CX::Instances::Log.warning("CX_DataFrameCell") << "toVector(): Attempt to extract data of different type than was inserted:" <<
			" Inserted type was \"" << *_type << "\" and attempted extracted type was \"" << extractedTypeName << "\".";
	}

	std::vector<T> values;
	for (unsigned int i = 0; i < _data->size(); i++) {
		values.push_back( ofFromString<T>( (*_data)[i] ) );
	}
	return values;
}

/*! Stores a vector of data in the cell. The data is stored as a string with each element delimited by a semicolon.
If the data to be stored are strings containing semicolons, the data will not be extracted properly.
\param values A vector of values to store.
*/
template <typename T> 
void CX_DataFrameCell::storeVector(std::vector<T> values) {
	//*_str = _getVectorStartString() + CX::Util::vectorToString(values, _getVectorElementDelimiter(), _getFloatingPointPrecision()) + _getVectorEndString();

	_data->clear();
	for (unsigned int i = 0; i < values.size(); i++) {
		_data->push_back( ofToString<T>( values[i] ) );
	}

	*_type = typeid(T).name();
	//*_dataIsVector = true;
	*_ignoreStoredType = false;
}

/*! Stores the given value with the given type. This function is a good way to explicitly
state the type of the data you are storing into the cell if, for example, it is a literal.
\tparam <T> The type to store the value as. If T is not specified, this function is essentially equivalent to using operator=.
\param value The value to store.
*/
template <typename T> void CX_DataFrameCell::store(const T& value) {
	_data->clear();
	_data->push_back( ofToString<T>(value) );

	//*_str = ofToString<T>(value);
	*_type = typeid(T).name();
	//*_dataIsVector = false;
	*_ignoreStoredType = false;
}

/*! Equivalent to a call to toString(). This is specialized because it skips the type checks of to<T>.
\return A copy of the stored data encoded as a string.
*/
template<> std::string CX_DataFrameCell::to(void) const;

template<> std::vector< std::string > CX_DataFrameCell::toVector(void) const;

std::ostream& operator<< (std::ostream& os, const CX_DataFrameCell& cell);

} //namespace CX