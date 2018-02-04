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

		CX_DataFrameCell(void);
		CX_DataFrameCell(const char* c);
		template <typename T> CX_DataFrameCell(const T& value); //!< Construct the cell, assigning the value to it.
		template <typename T> CX_DataFrameCell(const std::vector<T>& values); //!< Construct the cell, assigning the values to it.

		CX_DataFrameCell& operator=(const char* c);
		template <typename T> CX_DataFrameCell& operator=(const T& value); //!< Assigns a value to the cell.
		template <typename T> CX_DataFrameCell& operator=(const std::vector<T>& values); //!< Assigns a vector of values to the cell.

		template <typename T> operator T (void) const; //!< Attempts to convert the contents of the cell to T using to().
		template <typename T> operator std::vector<T>(void) const; //!< Attempts to convert the contents of the cell to vector<T> using toVector<T>().

		template <typename T> void store(const T& value);

		template <typename T> T to(bool log = true) const;

		std::string toString(void) const;

		//! Returns a copy of the stored data converted to bool. Equivalent to `to<bool>()`.
		bool toBool(void) const { return this->to<bool>(); };

		//! Returns a copy of the stored data converted to int. Equivalent to `to<int>()`.
		int toInt(void) const { return this->to<int>(); };

		//! Returns a copy of the stored data converted to double. Equivalent to `to<double>()`.
		double toDouble(void) const { return this->to<double>(); };

		template <typename T> std::vector<T> toVector(bool log = true) const;
		template <typename T> void storeVector(std::vector<T> values);
		bool isVector(void) const;
		unsigned int size(void) const;

		void copyCellTo(CX_DataFrameCell* targetCell) const;

		template <typename T> void setStoredType(void);
		std::string getStoredType(void) const;
		void deleteStoredType(void);

		void clear(void);

		static void setFloatingPointPrecision(unsigned int prec);
		static unsigned int getFloatingPointPrecision(void);

	private:

		static unsigned int _floatingPointPrecision;

		std::shared_ptr<std::vector<std::string>> _data;

		std::shared_ptr<std::string> _type;
		std::shared_ptr<bool> _ignoreStoredType;
		//std::shared_ptr<std::size_t> _typeHash; //Could make type comparison go much faster

		template <typename T> static std::string _toString(const T& value);
		template <typename T> static T _fromString(const std::string& str);

		void _allocatePointers(void);

	};

	template <typename T>
	CX_DataFrameCell::CX_DataFrameCell(const T& value) {
		_allocatePointers();
		this->store(value);
	}

	template <typename T>
	CX_DataFrameCell::CX_DataFrameCell(const std::vector<T>& values) {
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
	CX_DataFrameCell::operator std::vector<T>(void) const {
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
	T CX_DataFrameCell::to(bool log) const {

		if (_data->size() == 0) {
			if (log) {
				CX::Instances::Log.error("CX_DataFrameCell") << "to(): No data to extract from cell.";
			}
			return T();
			//TODO: It may be better to throw an exception here. 
			//Alternately, The fact that a default value is returned could
			//suggest that the returned value is stored in the cell, when in fact the cell is empty.
			//Maybe the default-constructed value should be stored.
		}

		if (log && (_data->size() > 1)) {
			CX::Instances::Log.warning("CX_DataFrameCell") << "to(): Attempt to extract a scalar when the stored data was a vector. "
				"Only the first value of the vector will be returned.";
		}

		std::string typeName = typeid(T).name();
		if (log && !(*_ignoreStoredType) && (*_type != typeName)) {
			CX::Instances::Log.warning("CX_DataFrameCell") << "to(): Extracting data of different type than was inserted:" <<
				" Inserted type was \"" << *_type << "\" and extracted type was \"" << typeName << "\".";
		}

		return _fromString<T>(_data->at(0));
	}

	/*! Returns a copy of the contents of the cell converted to a vector of the given type. If the type
	of data stored in the cell was not a vector of the given type or the type does match but it was a
	scalar that is stored, the logs a warning but attempts the conversion anyway.
	\tparam <T> The type of the elements of the returned vector.
	\return A vector containing the converted data.
	*/
	template <typename T>
	std::vector<T> CX_DataFrameCell::toVector(bool log) const {

		std::string extractedTypeName = typeid(T).name();
		if (log && !(*_ignoreStoredType) && (*_type != extractedTypeName)) {
			CX::Instances::Log.warning("CX_DataFrameCell") << "toVector(): Attempt to extract data of different type than was inserted:" <<
				" Inserted type was \"" << *_type << "\" and attempted extracted type was \"" << extractedTypeName << "\".";
		}

		std::vector<T> values;
		for (unsigned int i = 0; i < _data->size(); i++) {
			values.push_back(_fromString<T>((*_data)[i]));
		}
		return values;
	}

	/*! Stores a vector of data in the cell. The data is stored as a string with each element delimited by a semicolon.
	If the data to be stored are strings containing semicolons, the data will not be extracted properly.
	\param values A vector of values to store.
	*/
	template <typename T>
	void CX_DataFrameCell::storeVector(std::vector<T> values) {
		_data->clear();
		for (unsigned int i = 0; i < values.size(); i++) {
			_data->push_back(_toString<T>(values[i]));
		}
		this->setStoredType<T>();
		*_ignoreStoredType = false;
	}

	/*! Stores the given value with the given type. This function is a good way to explicitly
	state the type of the data you are storing into the cell if, for example, it is a literal.
	\tparam <T> The type to store the value as. If T is not specified, this function is essentially equivalent to using operator=.
	\param value The value to store.
	*/
	template <typename T> 
	void CX_DataFrameCell::store(const T& value) {
		_data->clear();
		_data->push_back(_toString<T>(value));

		this->setStoredType<T>();
		*_ignoreStoredType = false;
	}

	/*! \brief Convert from T to string. */
	template <typename T>
	static std::string CX_DataFrameCell::_toString(const T& value) {
		std::ostringstream os;
		os << std::fixed << std::setprecision(CX_DataFrameCell::getFloatingPointPrecision()) << value;
		return os.str();
	}

	/*! \brief Convert from string to T. */
	template <typename T>
	static T CX_DataFrameCell::_fromString(const std::string& str) {
		std::stringstream is;
		is << str;
		T val;
		is >> val;
		return val;
	}

	/*! \brief Sets the type of data stored by the cell to T. This doesn't convert the contents, it just sets metadata. */
	template <typename T> 
	void CX_DataFrameCell::setStoredType(void) {
		*_type = typeid(T).name();
		*_ignoreStoredType = false;
	}

	template<> std::string CX_DataFrameCell::to(bool log) const;

	template<> std::vector<std::string> CX_DataFrameCell::toVector(bool log) const;

	std::ostream& operator<< (std::ostream& os, const CX_DataFrameCell& cell);

} //namespace CX
