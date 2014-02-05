#ifndef _CX_UTILITIES_H_
#define _CX_UTILITIES_H_

#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cmath>

#include "ofUtils.h"
#include "ofTrueTypeFont.h"

#include "CX_Logger.h"

/*! \defgroup utility Utility */

namespace CX {

	/*! This namespace contains a variety of utility functions.
	\ingroup utility*/
	namespace Util {

		bool checkOFVersion(int versionMajor, int versionMinor, int versionPatch);

		int getSampleCount(void); //Make private?

		template <typename T> std::vector<T> arrayToVector(T arr[], unsigned int arraySize);

		template <typename T> std::vector<T> sequence(T start, T end, T stepSize);
		template <typename T> std::vector<T> sequenceSteps(T start, T stepSize, unsigned int steps);
		template <typename T> std::vector<T> sequenceAlong(T start, T end, unsigned int steps);

		template <typename T> std::vector<T> intVector(T start, T end);

		template <typename T> std::vector<T> repeat(T value, unsigned int times);
		template <typename T> std::vector<T> repeat(std::vector<T> values, unsigned int times, unsigned int each = 1);
		template <typename T> std::vector<T> repeat(std::vector<T> values, std::vector<unsigned int> each, unsigned int times = 1);

		template <typename T> std::string vectorToString(std::vector<T> values, std::string delimiter = ",", int significantDigits = 8);

		bool writeToFile(std::string filename, std::string data, bool append = true);

		enum class CX_RoundingConfiguration {
			ROUND_TO_NEAREST,
			ROUND_UP,
			ROUND_DOWN,
			ROUND_TOWARD_ZERO
		};

		double round(double d, int roundingPower, CX_RoundingConfiguration c);

	}
};

/*!
Repeats value "times" times.
\return A vector containing the repeated values.
*/
template <typename T> 
std::vector<T> CX::Util::repeat(T value, unsigned int times) {
	return std::vector<T>( times, value );
}

/*!
Repeats the elements of values. Each element of values is repeated "each" times and then the process of repeating the elements is
repeated "times" times.
\param values Vector of values to be repeated.
\param times The number of times the process should be performed.
\param each Number of times each element of values should be repeated.
\return A vector of the repeated values.
*/
template <typename T> 
std::vector<T> CX::Util::repeat(std::vector<T> values, unsigned int times, unsigned int each) {
	std::vector<T> rval;

	for (int i = 0; i < times; i++) {
		for (int val = 0; val < values.size(); val++) {
			for (int j = 0; j < each; j++) {
				rval.push_back( values[val] );
			}
		}
	}

	return rval;
}

/*!
Repeats the elements of values. Each element of values is repeated "each" times and then the process of repeating the elements is
repeated "times" times.
\param values Vector of values to be repeated.
\param each Number of times each element of values should be repeated. Must be the same length as values. If not, an error
is logged and an empty vector is returned.
\param times The number of times the process should be performed.
\return A vector of the repeated values.
*/
template <typename T> 
std::vector<T> CX::Util::repeat(std::vector<T> values, std::vector<unsigned int> each, unsigned int times) {
	std::vector<T> rval;

	if (values.size() != each.size()) {
		Log.error("CX::Util::repeat") << "values.size() != each.size()";
		return rval;
	}

	for (int i = 0; i < times; i++) {
		for (int j = 0; j < values.size(); j++) {
			for (int k = 0; k < each[j]; k++) {
				rval.push_back( values[j] );
			}
		}
	}

	return rval;
}

template <typename T>
std::string CX::Util::vectorToString(std::vector<T> values, std::string delimiter, int significantDigits) {
	std::stringstream s;
	s << std::fixed << std::setprecision(significantDigits);
	for (unsigned int i = 0; i < values.size(); i++) {
		s << values[i];
		if (i != values.size() - 1) {
			s << delimiter;
		}
	}
	return s.str();
}

/*
template <typename T> std::string vectorToString (std::vector<T> value, std::string elementStart, std::string elementEnd, int significantDigits) {
	std::stringstream s;
	s << std::fixed << std::setprecision(significantDigits);
	for (unsigned int i = 0; i < values.size(); i++) {
		s << elementStart << values[i] << elementEnd;
	}
	return s.str();
}
*/

/*!
Creates a sequence of numbers from start to end by steps of size stepSize. start may be geater than end, but only if
stepSize is less than 0. If start is less than end, stepSize must be greater than 0.

Example call: sequence<double>(1, 3.3, 2) results in {1, 3}

\param start The start of the sequence. You are guaranteed to get this value in the sequence.
\param end The number past which the sequence should end. You are not guaranteed to get this value.
\param stepSize A nonzero number.
*/
template <typename T> 
std::vector<T> CX::Util::sequence(T start, T end, T stepSize) {
	std::vector<T> rval;

	if (start < end) {
		if (stepSize <= 0) {
			return rval;
		}
		do {
			rval.push_back(start);
			start += stepSize;
		} while (start <= end);
		return rval;
	}

	if (start >= end) {
		if (stepSize >= 0) {
			return rval;
		}
		do {
			rval.push_back(start);
			start += stepSize;
		} while (start >= end);
		return rval;
	}

	return rval;
}

template <typename T> std::vector<T> CX::Util::sequenceSteps(T start, T stepSize, unsigned int steps) {
	return CX::sequence<T>(start, start + (stepSize * steps), stepSize);
}

template <typename T> std::vector<T> CX::Util::sequenceAlong(T start, T end, unsigned int outputLength) {
	T stepSize = (end - start)/(outputLength - 1);
	return CX::sequence<T>(start, end, stepSize);
}

/*!
Creates a vector of integers going from start to end. start may be greater than end, in which case
the returned values will be in descending order. This is similar to using CX::sequence, but the step
size is fixed to 1 and it works properly when trying to create a descending sequence of unsigned integers.
\return A vector of the values int the sequence.
*/
template <typename T> std::vector<T> CX::Util::intVector(T start, T end) {
	std::vector<T> rval;
	int dir = (start > end) ? -1 : 1;
	rval.push_back(start);
	while (start != end) {
		start += dir;
		rval.push_back(start);
	}
	return rval;
}

template <typename T> std::vector<T> CX::Util::arrayToVector(T arr[], unsigned int arraySize) {
	std::vector<T> rval(arraySize);
	for (std::vector<T>::size_type i = 0; i < arraySize; i++) {
		rval[i] = arr[i];
	}
	return rval;
}

#endif //_CX_UTILITIES_H_