#include "CX_RandomNumberGenerator.h"

#include <stdint.h>

using namespace CX;

CX_RandomNumberGenerator Instances::RNG;

CX_RandomNumberGenerator::CX_RandomNumberGenerator (void) {

	std::random_device rd; //By the C++11 specification, std::random_device is supposed to be a non-deterministic 
	//(e.g. hardware) RNG. However, from http://en.cppreference.com/w/cpp/numeric/random/random_device:
	//"Note that std::random_device may be implemented in terms of a pseudo-random number engine if a 
	//non-deterministic source (e.g. a hardware device) is not available to the implementation."
	//According to a Stack Overflow comment, Microsoft's implementation of std::random_device is based
	//on a ton of stuff, which should result in a fairly random result to be used as a seed for our
	//Mersenne Twister. See the comment: http://stackoverflow.com/questions/9549357/the-implementation-of-random-device-in-vs2010/9575747#9575747
	//However complicated this data, it is not a hardware RNG. The random_device is only used 
	//to seed the Mersenne Twister, so as long as the initial value is random enough, it should be fine.
	
	setSeed( rd() ); //Store the seed for reference and seed the Mersenne Twister.
}

void CX_RandomNumberGenerator::setSeed (uint64_t seed) {
	_seed = seed; //Store the seed for reference.

	_mersenneTwister.seed( (unsigned long)_seed );
}

CX_RandomInt_t CX_RandomNumberGenerator::randomInt(void) {
	return std::uniform_int_distribution<CX_RandomInt_t>(std::numeric_limits<CX_RandomInt_t>::min(), std::numeric_limits<CX_RandomInt_t>::max())(_mersenneTwister);
}

/*! This function returns an integer from the range [rangeLower, rangeUpper]. The minimum and maximum values for the
int returned from this function are given by getMinimumRandomInt() and getMaximumRandomInt().

If rangeLower > rangeUpper, the lower and upper ranges are swapped. If rangeLower == rangeUpper, it returns rangeLower.
*/
CX_RandomInt_t CX_RandomNumberGenerator::randomInt(CX_RandomInt_t min, CX_RandomInt_t max) {
	if (min > max) {
		//Emit an error/warning...
		std::swap(min, max);
	}

	return std::uniform_int_distribution<CX_RandomInt_t>(min, max)(_mersenneTwister);
}

CX_RandomInt_t CX_RandomNumberGenerator::getMinimumRandomInt (void) {
	return std::numeric_limits<CX_RandomInt_t>::min();
}

CX_RandomInt_t CX_RandomNumberGenerator::getMaximumRandomInt(void) {
	return std::numeric_limits<CX_RandomInt_t>::max();
}

//This function returns a double drawn from a uniform distribution with a range of [lowerBound_closed, upperBound_open).
//double CX_RandomNumberGenerator::uniformDouble (double lowerBound_closed, double upperBound_open) {
//	return std::uniform_real_distribution<double>(lowerBound_closed, upperBound_open)(_mersenneTwister);
//}

/*!
Returns a vector of count integers from the range [lowerBound, upperBound] with or without replacement.
*/
vector<int> CX_RandomNumberGenerator::sample(unsigned int count, int lowerBound, int upperBound, bool withReplacement) {
	return sample(count, CX::intVector(lowerBound, upperBound), withReplacement);
}

/*! Samples count deviates from a uniform distribution with the given lower bound and upper bound.
\param count The number of deviates to generate.
\param lowerBound_closed The lower bound of the distribution. This bound is closed, meaning that you can observe deviates with this value.
\param upperBound_open The upper bound of the distribution. This bound is open, meaning that you cannot observe deviates with this value.
\return A vector of the deviates. */
std::vector<double> CX_RandomNumberGenerator::uniformDeviates (unsigned int count, double lowerBound_closed, double upperBound_open) {
	std::vector<double> samples(count);
	std::uniform_real_distribution<double> unifDist(lowerBound_closed, upperBound_open);
	for (unsigned int i = 0; i < count; i++) {
		samples[i] = unifDist(_mersenneTwister);
	}
	return samples;
}

/*! Samples count deviates from a normal distribution with the given mean and standard deviation.
\param count The number of deviates to generate.
\param mean The mean of the distribution.
\param standardDeviation The standard deviation of the distribution.
\return A vector of the deviates. */
std::vector<double> CX_RandomNumberGenerator::normalDeviates (unsigned int count, double mean, double standardDeviation) {
	std::vector<double> samples(count);
	std::normal_distribution<double> normDist(mean, standardDeviation);
	for (unsigned int i = 0; i < count; i++) {
		samples[i] = normDist(_mersenneTwister);
	}
	return samples;
}