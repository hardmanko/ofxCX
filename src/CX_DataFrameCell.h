#ifndef _CX_DATA_FRAME_CELL_H_
#define _CX_DATA_FRAME_CELL_H_

//#include <vector>
//#include <map>
//#include <set>
#include <string>
#include <sstream>

#include <iostream>

//#include "stdint.h"

#include "ofUtils.h"

#include "CX_Utilities.h"
#include "CX_DeferredLogger.h"

namespace CX {

class CX_DataFrameCell {
public:

	CX_DataFrameCell (void);
	CX_DataFrameCell (const CX_DataFrameCell& r);
	CX_DataFrameCell (CX_DataFrameCell&& r);
	~CX_DataFrameCell (void);

	template <typename T> CX_DataFrameCell& operator= (T val);

	template <typename T> operator T (void) const;

	template <typename T> CX_DataFrameCell& operator= (std::vector<T> val);

	template <typename T> operator std::vector<T> (void) const;

	std::string toString (void) const;

private:

	friend class CX_DataFrame;
	friend class CX_SafeDataFrame;
	CX_DataFrameCell(string *s);
	
	template <typename T> void _setVector (std::vector<T> values);
	template <typename T> std::vector<T> _getVector (void) const;
	void _setPointer (std::string *str);

	std::string *_str;
	bool _selfAllocated;
};

template <typename T>
CX_DataFrameCell& CX_DataFrameCell::operator= (T val) {
	*_str = ofToString<T>(val, 16);
	return *this;
}

template <typename T>
CX_DataFrameCell::operator T (void) const {
	return ofFromString<T>(*_str);
}

template <typename T>
CX_DataFrameCell& CX_DataFrameCell::operator= (vector<T> val) {
	_setVector(val);
	return *this;
}

template <typename T>
CX_DataFrameCell::operator vector<T> (void) const {
	return _getVector<T>();
}

template <typename T> void CX_DataFrameCell::_setVector (std::vector<T> values) {
	*_str = "\"" + CX::vectorToString(values, ";", 16) + "\"";
}

template <typename T> std::vector<T> CX_DataFrameCell::_getVector (void) const {
	std::string encodedVect = *_str;
		
	ofStringReplace(encodedVect, "\"", "");

	std::vector<std::string> parts = ofSplitString(encodedVect, ";");

	std::vector<T> values;
	for (unsigned int i = 0; i < parts.size(); i++) {
		values.push_back( ofFromString<T>( parts[i] ) );
	}
	return values;
}

std::ostream& operator<< (std::ostream& os, const CX_DataFrameCell& cell);

}

#endif //_CX_DATA_FRAME_CELL_H_