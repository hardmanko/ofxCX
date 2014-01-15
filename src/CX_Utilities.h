#ifndef _CX_UTILITIES_H_
#define _CX_UTILITIES_H_

#include "ofConstants.h"

#include <vector>
#include <algorithm>

//#ifdef TARGET_WIN32
//#include "Windows.h" //Must include Windows.h before glfw3.h?
//#endif

struct GLFWwindow; //Forward declaration

//using namespace std;

namespace CX {
	namespace Private {
		extern GLFWwindow *glfwContext;
	}


	//void drawFboToBackBuffer (ofFbo &fbo);

	//void exit (void);

	//void pollEvents (void);

	//Add requireOFVersion(int versionMajor, versionMinor, etc) that complains if the version is not found.

	int getSampleCount (void);

	std::vector<int> intVector (int rangeBottom, int rangeTop);
	std::vector<int> intVectorByCount (std::vector<int> counts);
	std::vector<int> intVectorByCountAndValue (std::vector<int> counts, std::vector<int> values);

	template <typename T> std::vector<T> repeat (T value, unsigned int times);
	template <typename T> std::vector<T> repeat (std::vector<T> values, unsigned int times, unsigned int each = 1);
};


template <typename T> 
std::vector<T> CX::repeat (T value, unsigned int times) {
	return std::vector<T>( times, value );
}

template <typename T> 
std::vector<T> CX::repeat (std::vector<T> values, unsigned int times, unsigned int each) {
	std::vector<T> rval; //( values.size() * times * each );

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