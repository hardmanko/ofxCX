#ifndef _CX_UTILITIES_H_
#define _CX_UTILITIES_H_

#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>
#include <cmath>

#include "ofUtils.h"
#include "ofTrueTypeFont.h"

#include "CX_Logger.h"

//#ifdef TARGET_WIN32
//#include "Windows.h" //Must include Windows.h before glfw3.h?
//#endif

struct GLFWwindow; //Forward declaration

namespace CX {
	namespace Private {
		extern GLFWwindow *glfwContext;
	}


	bool checkOFVersion (int versionMajor, int versionMinor, int versionPatch);

	int getSampleCount (void);

	template <typename T> std::vector<T> arrayToVector (T arr[], unsigned int arraySize);

	template <typename T> std::vector<T> sequence (T start, T end, T stepSize);
	template <typename T> std::vector<T> sequenceSteps (T start, T stepSize, unsigned int steps);
	template <typename T> std::vector<T> sequenceAlong (T start, T end, unsigned int steps);

	template <typename T> std::vector<T> intVector (T start, T end);

	template <typename T> std::vector<T> repeat (T value, unsigned int times);
	template <typename T> std::vector<T> repeat (std::vector<T> values, unsigned int times, unsigned int each = 1);
	template <typename T> std::vector<T> repeat (std::vector<T> values, std::vector<unsigned int> each, unsigned int times = 1);

	template <typename T> std::string vectorToString (std::vector<T> values, std::string delimiter = ",", int significantDigits = 8);

	template <typename T> bool writeToFile (std::string filename, const T& data, bool append = true);
	bool writeToFile (std::string filename, string data, bool append = true);

	enum class CX_RoundingConfiguration {
		ROUND_TO_NEAREST,
		ROUND_UP,
		ROUND_DOWN,
		ROUND_TOWARD_ZERO
	};

	double round (double d, int roundingPower, CX_RoundingConfiguration c);
};

template <typename T> bool CX::writeToFile (std::string filename, const T& data, bool append) {
	return toFile(filename, ofToString<T>(data), append);
}

template <typename T> 
std::vector<T> CX::repeat (T value, unsigned int times) {
	return std::vector<T>( times, value );
}

template <typename T> 
std::vector<T> CX::repeat (std::vector<T> values, unsigned int times, unsigned int each) {
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

template <typename T> 
std::vector<T> CX::repeat (std::vector<T> values, vector<unsigned int> each, unsigned int times) {
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
std::string CX::vectorToString (std::vector<T> values, string delimiter, int significantDigits) {
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


template <typename T> 
std::vector<T> CX::sequence (T start, T end, T stepSize) {
	std::vector<T> rval;

	if (start < end) {
		if (stepSize < 0) {
			return rval;
		}
		do {
			rval.push_back(start);
			start += stepSize;
		} while (start <= end);
		return rval;
	}

	if (start >= end) {
		if (stepSize > 0) {
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

template <typename T> std::vector<T> CX::sequenceSteps (T start, T stepSize, unsigned int steps) {
	return CX::sequence<T>(start, start + (stepSize * steps), stepSize);
}

template <typename T> std::vector<T> CX::sequenceAlong (T start, T end, unsigned int outputLength) {
	T stepSize = (end - start)/(outputLength - 1);
	return CX::sequence<T>(start, end, stepSize);
}

template <typename T> std::vector<T> CX::intVector (T start, T end) {
	return CX::sequence<T>(start, end, 1);
}

template <typename T> std::vector<T> CX::arrayToVector (T arr[], unsigned int arraySize) {
	std::vector<T> rval(arraySize);
	for (std::vector<T>::size_type i = 0; i < arraySize; i++) {
		rval[i] = arr[i];
	}
	return rval;
}

#endif //_CX_UTILITIES_H_