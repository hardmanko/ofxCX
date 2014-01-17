#ifndef _CX_UTILITIES_H_
#define _CX_UTILITIES_H_

#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>

#include "ofUtils.h"

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

	template <typename T> std::vector<T> sequence (T start, T end, T stepSize);
	template <typename T> std::vector<T> sequenceSteps (T start, T stepSize, unsigned int steps);
	template <typename T> std::vector<T> sequenceAlong (T start, T end, unsigned int steps);

	std::vector<int> intVector (int start, int end);
	std::vector<unsigned int> uintVector (unsigned int start, unsigned int end);

	std::vector<int> intVectorByCount (std::vector<int> counts);
	std::vector<int> intVectorByCountAndValue (std::vector<int> counts, std::vector<int> values);

	template <typename T> std::vector<T> repeat (T value, unsigned int times);
	template <typename T> std::vector<T> repeat (std::vector<T> values, unsigned int times, unsigned int each = 1);

	template <typename T> std::string vectorToString (std::vector<T> values, std::string delimiter = ",", int significantDigits = 8);
	template <typename T> std::string vectorToString (std::vector<T> value, std::string elementStart = "{", std::string elementEnd = "}", int significantDigits = 8);

	template <typename T> bool writeToFile (std::string filename, const T& data, bool append = true);
	bool writeToFile (std::string filename, string data, bool append = true);

	//void drawFboToBackBuffer (ofFbo &fbo);
	//void exit (void);
	//void pollEvents (void);
};

template <typename T> bool writeToFile (std::string filename, const T& data, bool append) {
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

template <typename T> std::string vectorToString (std::vector<T> value, std::string elementStart, std::string elementEnd, int significantDigits) {
	std::stringstream s;
	s << std::fixed << std::setprecision(significantDigits);
	for (unsigned int i = 0; i < values.size(); i++) {
		s << elementStart << values[i] << elementEnd;
	}
	return s.str();
}


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

#endif //_CX_UTILITIES_H_