#pragma once

#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cmath>

#include "ofUtils.h"
#include "ofTrueTypeFont.h"
#include "ofFbo.h"
#include "ofImage.h"

#include "CX_Logger.h"

/*! \defgroup utility Utility */

//More overbroadly-named macros to deal with...
#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

namespace CX {

	namespace Private {
		void setMsaaSampleCount(unsigned int count);
	}

	/*! This namespace contains a variety of utility functions.
	\ingroup utility*/
	namespace Util {

		bool checkOFVersion(int versionMajor, int versionMinor, int versionPatch, bool log = true);

		bool setProcessToHighPriority(void);

		unsigned int getMsaaSampleCount(void); //Move to Draw ns?

		template <typename T> std::vector<T> arrayToVector(T arr[], unsigned int arraySize);

		template <typename T> std::vector<T> sequence(T start, T end, T stepSize);
		template <typename T> std::vector<T> sequenceSteps(T start, unsigned int steps, T stepSize);
		template <typename T> std::vector<T> sequenceAlong(T start, T end, unsigned int steps);

		template <typename T> std::vector<T> intVector(T start, T end);

		template <typename T> std::vector<T> repeat(T value, unsigned int times);
		template <typename T> std::vector<T> repeat(std::vector<T> values, unsigned int times, unsigned int each = 1);
		template <typename T> std::vector<T> repeat(std::vector<T> values, std::vector<unsigned int> each, unsigned int times = 1);

		template <typename T> std::string vectorToString(std::vector<T> values, std::string delimiter = ",", int significantDigits = 8);
		template <typename T> std::vector<T> stringToVector(std::string s, std::string delimiter);

		bool writeToFile(std::string filename, std::string data, bool append = true);
		std::map<std::string, std::string> readKeyValueFile(std::string filename, std::string delimiter = "=", bool trimWhitespace = true, std::string commentString = "//");

		/*! The way in which numbers should be rounded with CX::Util::round(). */
		enum class CX_RoundingConfiguration {
			ROUND_TO_NEAREST, //!< Round to the nearest number.
			ROUND_UP, //!< Round to the number above the current number.
			ROUND_DOWN, //!< Round to the number below the current number.
			ROUND_TOWARD_ZERO //!< Round toward zero.
		};

		double round(double d, int roundingPower, CX_RoundingConfiguration c = CX_RoundingConfiguration::ROUND_TO_NEAREST);

		template <typename T> T clamp(T val, T minimum, T maximum);
		template <typename T> std::vector<T> clamp(std::vector<T> vals, T minimum, T maximum);

		template <typename T> std::vector<T> unique(std::vector<T> vals);
		template <typename T> std::vector<T> exclude(const std::vector<T>& vals, const std::vector<T>& exclude);

		template <typename T> std::vector<T> concatenate(const std::vector<T>& A, const std::vector<T>& B);
		template <typename T> std::vector<T> concatenate(T A, const std::vector<T>& B);

		template <typename T> T max(std::vector<T> vals);
		template <typename T> T min(std::vector<T> vals);
		template <typename T> T mean(std::vector<T> vals);
		template <typename T_OUT, typename T_IN> T_OUT mean(std::vector<T_IN> vals);
		template <typename T> T var(std::vector<T> vals);
		template <typename T_OUT, typename T_IN> T_OUT var(std::vector<T_IN> vals);

		float getAngleBetweenPoints(ofPoint p1, ofPoint p2);
		ofPoint getRelativePointFromDistanceAndAngle(ofPoint start, float distance, float angle);

	}
}

/*! Repeats value "times" times.
\param value The value to be repeated.
\param times The number of times to repeat the value.
\return A vector containing times copies of the repeated value.
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
		//CX::Instances::Log.error("CX::Util::repeat") << "values.size() != each.size()"; //For some reason GCC doesn't like this error printout.
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


/*! This function converts a vector of values to a string representation of the values.
\param values The vector of values to convert.
\param delimiter A string that is used to separate the elements of `value` in the final string.
\param significantDigits Only for floating point types. The number of significant digits in the value.
\return A string containing a representation of the vector of values.
*/
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

/*! This function takes a string, splits it on the delimiter, and converts each delimited part of
the string to T, returning a vector<T>.
\tparam T The type of the data encoded in the string.
\param s The string containing the encoded data.
\param delimiter The string that delimits the elements of the data.
\return A vector of the encoded data converted to T.
*/
template <typename T>
std::vector<T> CX::Util::stringToVector(std::string s, std::string delimiter) {
	std::vector< std::string > parts = ofSplitString(s, delimiter, true, true);
	std::vector<T> rval;
	rval.resize( parts.size() );
	for (unsigned int i = 0; i < parts.size(); i++) {
		rval[i] = ofFromString<T>( parts[i] );
	}
	return rval;
}


/*!
Creates a sequence of numbers from `start` to `end` by steps of size `stepSize`. `end` may be less than `start`, but only if
`stepSize` is less than 0. If `end` is greater than `start`, `stepSize` must be greater than 0.

Example call: `sequence<double>(1, 3.3, 2)` results in a vector containing {1, 3}

\param start The start of the sequence.
\param end The number past which the sequence should end. You are not guaranteed to get this value.
\param stepSize A nonzero number.
\return A vector containing the sequence. It may be empty.
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

/*! Make a sequence starting from that value given by `start` and taking `steps` steps of `stepSize`.

    sequenceSteps( 1.5, 4, 2.5 );

Creates the sequence {1.5, 4, 6.5, 9, 11.5}

\param start Value from which to start.
\param steps The number of steps to take.
\param stepSize The size of each step.
\return A vector containing the sequence.
*/
template <typename T> std::vector<T> CX::Util::sequenceSteps(T start, unsigned int steps, T stepSize) {
	return CX::Util::sequence<T>(start, start + (stepSize * (steps - 1)), stepSize);
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

	std::vector<T> seq = CX::Util::sequence<T>(start, start + stepSize * (outputLength - 1), stepSize);
	if (seq.size() == outputLength) {
		seq.back() = end;
	} else if (seq.size() < outputLength) {
		seq.push_back(end);
	}
	return seq;
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

/*! Clamps a value (i.e. forces the value to be between two bounds). If the value is
outside of the bounds, it is set to be equal to the nearest bound.
\param val The value to clamp.
\param minimum The lower bound. Must be less than or equal to maximum.
\param maximum The upper bound. Must be greater than or equal to minimum.
\return The clamped value. */
template <typename T>
T CX::Util::clamp(T val, T minimum, T maximum) {
	return std::min(std::max(val, minimum), maximum);
}

/*! Clamps a vector of values. See \ref CX::Util::clamp().
\param vals The values to clamp.
\param minimum The lower bound. Must be less than or equal to maximum.
\param maximum The upper bound. Must be greater than or equal to minimum.
\return The clamped values. */
template <typename T>
std::vector<T> CX::Util::clamp(std::vector<T> vals, T minimum, T maximum) {
	std::vector<T> rval(vals.size());
	for (unsigned int i = 0; i < vals.size(); i++) {
		rval[i] = clamp<T>(vals[i], minimum, maximum);
	}
	return rval;
}

/*! Finds the maximum value in a vector of values.
\tparam T The type of data to be operated on. This type must have operator> defined.
\param vals The vector of values.
\return The maximum value in the vector. */
template <typename T> T CX::Util::max(std::vector<T> vals) {
	if (vals.size() == 0) {
		return T();
	}
	T maximum = vals[0];
	for (unsigned int i = 1; i < vals.size(); i++) {
		if (vals[i] > maximum) {
			maximum = vals[i];
		}
	}
	return maximum;
}

/*! Finds the minimum value in a vector of values.
\tparam T The type of data to be operated on. This type must have operator< defined.
\param vals The vector of values.
\return The minimum value in the vector. */
template <typename T> T CX::Util::min(std::vector<T> vals) {
	if (vals.size() == 0) {
		return T();
	}
	T minimum = vals[0];
	for (unsigned int i = 1; i < vals.size(); i++) {
		if (vals[i] < minimum) {
			minimum = vals[i];
		}
	}
	return minimum;
}

/*! Calculates the mean value of a vector of values.
\tparam T The type of data to be operated on and returned. This type must have operator+(T) and operator/(unsigned int) defined.
\param vals The vector of values.
\return The mean of the vector. */
template <typename T> T CX::Util::mean(std::vector<T> vals) {
	return mean<T, T>(vals);
}

/*! Calculates the mean value of a vector of values.
\tparam T_OUT The type of data to be returned. This type must have operator+(T_IN) and operator/(unsigned int) defined.
\tparam T_IN The type of data to be operated on.
\param vals The vector of values.
\return The mean of the vector. */
template <typename T_OUT, typename T_IN> T_OUT CX::Util::mean(std::vector<T_IN> vals) {
	T_OUT sum = 0;
	for (unsigned int i = 0; i < vals.size(); i++) {
		sum = sum + vals[i];
	}
	return sum / vals.size();
}

/*! Calculates the sample variance of a vector of values. 
\tparam T The type of data.
\param vals The data.
\return The sample variance. */
template <typename T> T CX::Util::var(std::vector<T> vals) {
	return Util::var<T, T>(vals);
}

/*! Calculates the sample variance of a vector of values.
\tparam T_OUT The type of data to be returned.
\tparam T_IN The type of data to be operated on.
\param vals The vector of values.
\return The mean of the vector. */
template <typename T_OUT, typename T_IN> T_OUT CX::Util::var(std::vector<T_IN> vals) {
	T_OUT m = Util::mean(vals);
	T_OUT sum = 0;
	for (unsigned int i = 0; i < vals.size(); i++) {
		T_OUT dif = vals[i] - m;
		sum += dif * dif;
	}
	return sum / (vals.size() - 1); //Sample variance has n - 1 for denominator

	/*
	//Do single-pass var?
	T_OUT mean = 0;
	T_OUT M2 = 0;

	for (unsigned int i = 0; i < vals.size(); i++) {
		T_OUT delta = vals[i] - mean;
		mean = mean + delta / (i + 1);
		M2 = M2 + delta*(vals[i] - mean);
	}
	return M2 / (vals.size() - 1); //Sample variance has n - 1 for denominator
	*/
}

/*! Uses std::unique to find all of the unique values in `vals` and return copies of those values.
\param vals The vector of values to find unique values in.
\return A vector containing the unique values in `vals`.
*/
template <typename T> 
std::vector<T> CX::Util::unique(std::vector<T> vals) {
	 
	//Requires the sort, because std::unique needs the vector to be sorted.
	std::sort(vals.begin(), vals.end());
	auto pastEnd = std::unique(vals.begin(), vals.end());
	std::vector<T> uniqueVals;
	for (auto it = vals.begin(); it != pastEnd; it++) {
		uniqueVals.push_back(*it);
	}
	return uniqueVals;
	
}

/*! Concatenates together two vectors A and B.
\param A The first vector of values.
\param B The second vector of values.
\return The concatenation of A and B, being a vector containing {A1, A2, ... An, B1, B2, ... Bn}. */
template <typename T>
std::vector<T> CX::Util::concatenate(const std::vector<T>& A, const std::vector<T>& B) {
	std::vector<T> C(A.begin(), A.end());
	C.insert(C.end(), B.begin(), B.end());
	return C;
}

/*! Concatenates together the value A and the vector B. This is essentially push_front for vectors.
\param A The first value.
\param B The vector of values.
\return The concatenation of A and B, being a vector containing {A, B1, B2, ... Bn}. */
template <typename T>
std::vector<T> CX::Util::concatenate(T A, const std::vector<T>& B) {
	std::vector<T> C;
	C.push_back(A);
	C.insert(C.end(), B.begin(), B.end());
	return C;
}

/*! Gets the values from `values` that do not match the values in `exclude`.
\param values The starting set of values.
\param exclude The set of values to exclude from `values`.
\return A vector containing the values that were not excluded. This vector may be empty. */
template <typename T>
std::vector<T> CX::Util::exclude(const std::vector<T>& values, const std::vector<T>& exclude) {
	std::vector<T> kept;

	std::vector<T> uniqueExclude = unique(exclude);

	for (T val : values) {
		bool valueExcluded = false;
		for (T ex : uniqueExclude) {
			if (val == ex) {
				valueExcluded = true;
				break;
			}
		}

		if (!valueExcluded) {
			kept.push_back(val);
		}
	}
	return kept;
}
