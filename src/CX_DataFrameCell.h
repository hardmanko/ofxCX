#ifndef _CX_DATA_FRAME_CELL_H_
#define _CX_DATA_FRAME_CELL_H_

#include <vector>
#include <string>
#include <sstream>
#include <memory> //shared_pointer
#include <iostream>

#include "ofUtils.h"

#include "CX_Utilities.h"
#include "CX_Logger.h"

namespace CX {

class CX_DataFrameCell {
public:

	CX_DataFrameCell (void);
	template <typename T> CX_DataFrameCell (const T& value);
	template <typename T> CX_DataFrameCell (const std::vector<T>& values);

	template <typename T> CX_DataFrameCell& operator= (const T& value);
	template <typename T> CX_DataFrameCell& operator= (const std::vector<T>& values);

	template <typename T> operator T (void) const;
	template <typename T> operator std::vector<T> (void) const;


	std::string toString (void) const;
	bool toBool (void) const;
	int toInt (void) const;
	double toDouble (void) const;
	template <typename T> T to (void) const;

	template <typename T> std::vector<T> toVector (void) const;
	template <typename T> void storeVector (std::vector<T> values);

private:
	std::shared_ptr<std::string> _str;

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
	*_str = _toString(value);
}

template <typename T> 
CX_DataFrameCell::CX_DataFrameCell (const std::vector<T>& values) {
	_str = std::shared_ptr<std::string>(new std::string);
	storeVector<T>(values);
}

template <typename T>
CX_DataFrameCell& CX_DataFrameCell::operator= (const T& value) {
	*_str = _toString(value);
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

template <typename T> 
T CX_DataFrameCell::to (void) const {
	return ofFromString<T>(*_str);
}

template <typename T> 
std::vector<T> CX_DataFrameCell::toVector (void) const {
	std::string encodedVect = *_str;
		
	ofStringReplace(encodedVect, "\"", "");

	std::vector<std::string> parts = ofSplitString(encodedVect, ";");

	std::vector<T> values;
	for (unsigned int i = 0; i < parts.size(); i++) {
		values.push_back( ofFromString<T>( parts[i] ) );
	}
	return values;
}

template <typename T> 
void CX_DataFrameCell::storeVector (std::vector<T> values) {
	*_str = "\"" + CX::vectorToString(values, ";", 16) + "\"";
}

std::ostream& operator<< (std::ostream& os, const CX_DataFrameCell& cell);

}

#endif //_CX_DATA_FRAME_CELL_H_