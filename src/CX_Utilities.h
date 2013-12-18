#ifndef _CX_UTILITIES_H_
#define _CX_UTILITIES_H_

#include "ofConstants.h"

#include <vector>
#include <algorithm>

#ifdef TARGET_WIN32
#include "Windows.h"
#endif



struct GLFWwindow;

using namespace std;

namespace CX {
	namespace Private {
		extern GLFWwindow *glfwContext;
	}
	//void drawFboToBackBuffer (ofFbo &fbo);

	//void exit (void);

	//void pollEvents (void);

	//Add requireOFVersion(int versionMajor, versionMinor, etc) that complains to cout if the version is not found.

	int getSampleCount (void);

	vector<int> intVector (int rangeBottom, int rangeTop);
	vector<int> intVectorByCount (vector<int> counts);
	vector<int> intVectorByCountAndValue (vector<int> counts, vector<int> values);

	template <typename T> vector<T> repeat (T value, unsigned int times);
	template <typename T> vector<T> repeat (vector<T> values, unsigned int times, unsigned int each = 1);
};


template <typename T> 
vector<T> CX::repeat (T value, unsigned int times) {
	return vector<T>( times, value );
}

template <typename T> 
vector<T> CX::repeat (vector<T> values, unsigned int times, unsigned int each) {
	vector<T> rval; //( values.size() * times * each );

	for (int i = 0; i < times; i++) {
		for (int val = 0; val < values.size(); val++) {
			for (int j = 0; j < each; j++) {
				rval.push_back( values[val] );
			}
		}
	}

	return rval;
}

#endif //_CX_UTILITIES_H_