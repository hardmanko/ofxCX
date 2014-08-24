#include "CX_RandomNumberGenerator.h"

using namespace CX;

/*! An instance of CX_RandomNumberGenerator that is (lightly) hooked into the CX backend.
\ingroup entryPoint
*/
CX::CX_RandomNumberGenerator CX::Instances::RNG;

/*! Constructs an instance of a CX_RandomNumberGenerator. Seeds the CX_RandomNumberGenerator
using a std::random_device. 

By the C++11 specification, std::random_device is supposed to be a non-deterministic 
(hardware) RNG. However, from http://en.cppreference.com/w/cpp/numeric/random/random_device:
"Note that std::random_device may be implemented in terms of a pseudo-random number engine if a 
non-deterministic source (e.g. a hardware device) is not available to the implementation."
According to a Stack Overflow comment, Microsoft's implementation of std::random_device is based
on a ton of stuff, which should result in a fairly random result to be used as a seed for our
Mersenne Twister. See the comment: http://stackoverflow.com/questions/9549357/the-implementation-of-random-device-in-vs2010/9575747#9575747
Although this data should have high entropy, it is not a hardware RNG. The random_device is only used 
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

/*! This function provides a method of setting the seed using an arbitrary string
(e.g. date-time and participant number) as the seed. A CRC32 checksum is used 
to convert the string into an unsinged long, which is then used as the seed for 
the CX_RandomNumberGenerator. You can retrieve the seed with getSeed().
\param seedString The string from which the new seed will be calculated. 
*/
void CX_RandomNumberGenerator::setSeed(const std::string& seedString) {
	Poco::Checksum cs(Poco::Checksum::TYPE_CRC32);
	cs.update(seedString);
	this->setSeed(cs.checksum());
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

/*! Samples a realization from a uniform distribution with the range [lowerBound_closed, upperBound_open).
\param lowerBound_closed The lower bound of the distribution. This bound is closed, meaning that you can observe this value.
\param upperBound_open The upper bound of the distribution. This bound is open, meaning that you cannot observe this value.
\return The realization. */
double CX_RandomNumberGenerator::randomDouble(double lowerBound_closed, double upperBound_open) {
	if (lowerBound_closed > upperBound_open) {
		Instances::Log.error("CX_RandomNumberGenerator") << "randomDouble: The lower bound is greater than the upper bound, returning 0.";
		return 0;
	}
	return std::uniform_real_distribution<double>(lowerBound_closed, upperBound_open)(_mersenneTwister);
}

/*! Returns a vector of count integers drawn randomly from the range [lowerBound, upperBound] with or without replacement.
\param count The number of samples to draw.
\param lowerBound The lower bound of the range to sample from. It is possible to sample this value.
\param upperBound The upper bound of the range to sample from. It is possible to sample this value.
\param withReplacement Sample with or without replacement.
\return A vector of the samples. */
std::vector<int> CX_RandomNumberGenerator::sample(unsigned int count, int lowerBound, int upperBound, bool withReplacement) {
	return sample(count, CX::Util::intVector<int>(lowerBound, upperBound), withReplacement);
}

/*!	Samples count realizations from a binomial distribution with the given number of trials and probability of success on each trial.
\param count The number of deviates to generate.
\param trials The number of trials. Must be a non-negative integer.
\param probSuccess The probability of a success on a given trial, where a success is the value 1.
\return A vector of the realizations. */
std::vector<unsigned int> CX_RandomNumberGenerator::sampleBinomialRealizations(unsigned int count, unsigned int trials, double probSuccess) {
	return this->sampleRealizations(count, std::binomial_distribution<unsigned int>(trials, probSuccess));
}

/*! This function returns a reference to the standard library PRNG used by the CX_RandomNumberGenerator.
This can be used for various things, including sampling from some of the other distributions
provided by the standard library: http://en.cppreference.com/w/cpp/numeric/random
\code{.cpp}
std::poisson_distribution<int> pois(4);
int deviate = pois(RNG.getGenerator());
\endcode */
std::mt19937_64& CX_RandomNumberGenerator::getGenerator(void) { 
	return _mersenneTwister; 
}