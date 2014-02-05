#include "CX_RandomNumberGenerator.h"

#include <stdint.h>

using namespace CX;

/*! An instance of CX_RandomNumberGenerator that is (lightly) hooked into the CX backend.
\ingroup random
\ingroup entryPoint
*/
CX_RandomNumberGenerator CX::Instances::RNG;

/*! Constructs an instance of a CX_RandomNumberGenerator. Seeds the CX_RandomNumberGenerator
using a std::random_device. 

By the C++11 specification, std::random_device is supposed to be a non-deterministic 
(hardware) RNG. However, from http://en.cppreference.com/w/cpp/numeric/random/random_device:
"Note that std::random_device may be implemented in terms of a pseudo-random number engine if a 
non-deterministic source (e.g. a hardware device) is not available to the implementation."
According to a Stack Overflow comment, Microsoft's implementation of std::random_device is based
on a ton of stuff, which should result in a fairly random result to be used as a seed for our
Mersenne Twister. See the comment: http://stackoverflow.com/questions/9549357/the-implementation-of-random-device-in-vs2010/9575747#9575747
However complicated this data, it is not a hardware RNG. The random_device is only used 
to seed the Mersenne Twister, so as long as the initial value is random enough, it should be fine.
*/
CX_RandomNumberGenerator::CX_RandomNumberGenerator (void) {
	std::random_device rd;
	
	setSeed( rd() ); //Store the seed for reference and seed the Mersenne Twister.
}

/*! Set the seed for the random number generator. You can retrieve the seed with getSeed().
\param seed The new seed. */
void CX_RandomNumberGenerator::setSeed (unsigned long seed) {
	_seed = seed; //Store the seed for reference.

	_mersenneTwister.seed( _seed );
}

/*! Get the seed used to seed the random number generator. 
\return The seed. May have been set by the user with setSeed() or during construction of the CX_RandomNumberGenerator. */
unsigned long CX_RandomNumberGenerator::getSeed (void) { 
	return _seed; 
}

/*! Get a random integer in the range getMinimumRandomInt(), getMaximumRandomInt(), inclusive.
\return The int. */
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

/*! Get the minimum value that can be returned by randomInt(). 
\return The minimum value. */
CX_RandomInt_t CX_RandomNumberGenerator::getMinimumRandomInt (void) {
	return std::numeric_limits<CX_RandomInt_t>::min();
}

/*! Get the maximum possible value that can be returned by randomInt().
\return The maximum value. */
CX_RandomInt_t CX_RandomNumberGenerator::getMaximumRandomInt(void) {
	return std::numeric_limits<CX_RandomInt_t>::max();
}

/*! Samples a deviate from a uniform distribution with the range [lowerBound_closed, upperBound_open).
\param lowerBound_closed The lower bound of the distribution. This bound is closed, meaning that you can observe deviates with this value.
\param upperBound_open The upper bound of the distribution. This bound is open, meaning that you cannot observe deviates with this value.
\return The deviate. */
double CX_RandomNumberGenerator::uniformDeviate (double lowerBound_closed, double upperBound_open) {
	return std::uniform_real_distribution<double>(lowerBound_closed, upperBound_open)(_mersenneTwister);
}

/*! Returns a vector of count integers drawn randomly from the range [lowerBound, upperBound] with or without replacement.
\param count The number of samples to draw.
\param lowerBound The lower bound of the range to sample from. It is possible to sample this value.
\param upperBound The upper bound of the range to sample from. It is possible to sample this value.
\param withReplacement Sample with or without replacement.
\return A vector of the samples. */
vector<int> CX_RandomNumberGenerator::sample(unsigned int count, int lowerBound, int upperBound, bool withReplacement) {
	return sample(count, CX::intVector<int>(lowerBound, upperBound), withReplacement);
}

/*! Samples count deviates from a uniform distribution with the range [lowerBound_closed, upperBound_open).
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