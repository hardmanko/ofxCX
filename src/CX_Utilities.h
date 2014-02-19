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
		template <typename T> std::vector<T> sequenceSteps(T start, unsigned int steps, T stepSize);
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

		float degreesToPixels(float degrees, float pixelsPerUnit, float viewingDistance);
		float pixelsToDegrees(float pixels, float pixelsPerUnit, float viewingDistance);

		class CX_BaseUnitConverter {
		public:
			virtual float operator() (float inputUnits) {
				return inputUnits;
			};
		};

		/* This simple utility class is used for converting degrees of visual angle to pixels on a monitor.
		This class uses CX::Util::degreesToPixels() internally.

		\code{.cpp}
		CX_degreeToPixelConverter d2p(34, 60); //34 pixels per (e.g.) cm, user is 60 cm from monitor.
		ofLine( 200, 100, 200 + d2p(1), 100 + d2p(2) ); //Draw a line from (200, 100) in pixel coordinates to 1 degree
		//horizontally and 2 degrees vertically from that point.

		\endcode
		*/
		class CX_DegreeToPixelConverter : public CX_BaseUnitConverter {
		public:
			CX_DegreeToPixelConverter(float pixelsPerUnit, float viewingDistance, bool roundResult = false);
			float operator() (float degrees);

		private:
			float _pixelsPerUnit;
			float _viewingDistance;
			bool _roundResult;
		};

		class CX_LengthToPixelConverter : public CX_BaseUnitConverter {
		public:
			CX_LengthToPixelConverter(float pixelsPerUnit, bool roundResult = false);
			float operator() (float length);

		private:
			float _pixelsPerUnit;
			bool _roundResult;
		};



		/* This helper class is used for converting the standard computer monitor coordinate system
		into more familiar coordinate systems.

		\code{.cpp}
		CX_CoordinateConverter cc(Display.getCenterOfDisplay(), false, true); //Make the center of the display the origin and invert
			//the Y-axis. This makes positive x values go to the right and positive y values go up from the center of the display.
		ofSetColor(255, 0, 0); //Draw a red circle in the center of the display.
		ofCircle(cc(0, 0), 20);
		ofSetColor(0, 255, 0); //Draw a green circle 100 pixels to the right of the center.
		ofCircle(cc(100, 0), 20);
		ofSetColor(0, 0, 255); //Draw a blue circle 100 pixels above the center (inverted y-axis).
		ofCircle(cc(0, 100), 20);
		\endcode

		*/
		class CX_CoordinateConverter {
		public:

			CX_CoordinateConverter(ofPoint origin, bool invertX, bool invertY, bool invertZ = false);

			ofPoint operator() (ofPoint p);
			ofPoint operator() (float x, float y, float z = 0);

			void setAxisInversion(bool invertX, bool invertY, bool invertZ = false);
			void setOrigin(ofPoint newOrigin);

			void setUnitConverter(CX_BaseUnitConverter *converter);

		private:
			ofPoint _origin;
			bool _invertX;
			bool _invertY;
			bool _invertZ;

			//ofVec3f _orientations; to do input * orientations

			CX_BaseUnitConverter *_conv;
		};

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
		//CX::Instances::Log.error("CX::Util::repeat") << "values.size() != each.size()"; //For some reason GCC doesn't like this.
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

/*! Make a sequence starting from start and taking steps steps of stepSize.

    sequenceSteps( 1.5, 4, 2.5 );

Creates the sequence {1.5, 4, 6.5, 9, 11.5}

\param start Value from which to start.
\param steps The number of steps to take.
\param stepSize The size of each step.
\return A vector containing the sequence.
*/
template <typename T> std::vector<T> CX::Util::sequenceSteps(T start, unsigned int steps, T stepSize) {
	return CX::Util::sequence<T>(start, start + (stepSize * steps), stepSize);
}

/*! Creates a sequence from start to end, where the size of each step is chosen so that the length
of the sequence if equal to outputLength.
\param start The value at which to start the sequence.
\param end The value to which to end the sequence.
\param outputLength The number of elements in the returned sequence.
\return A vector containing the sequence.
*/
template <typename T> std::vector<T> CX::Util::sequenceAlong(T start, T end, unsigned int outputLength) {
	T stepSize = (end - start)/(outputLength - 1);
	return CX::Util::sequence<T>(start, end, stepSize);
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

/*! Copies arraySize elements of an array of T to a vector<T>.
\tparam <T> The type of the array. Is often inferred by the compiler.
\param arr The array of data to put into the vector.
\param arraySize The length of the array, or the number of elements to copy from the array
if not all of the elements are wanted.
\return The elements in a vector.
*/
template <typename T> std::vector<T> CX::Util::arrayToVector(T arr[], unsigned int arraySize) {
	std::vector<T> rval(arraySize);
	for (unsigned int i = 0; i < arraySize; i++) {
		rval[i] = arr[i];
	}
	return rval;
}

#endif //_CX_UTILITIES_H_