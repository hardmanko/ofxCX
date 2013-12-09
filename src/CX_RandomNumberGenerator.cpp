#include "CX_RandomNumberGenerator.h"

#include <stdint.h>

using namespace CX;

CX_RandomNumberGenerator::CX_RandomNumberGenerator (void) {

	//Set the _unitInterval distribution to return results on [0,1).
	_unitInterval = std::uniform_real_distribution<double>(0, 1); //This it broken.

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

CX_RandomInt_t CX_RandomNumberGenerator::randomInt (void) {
	return _mersenneTwister();
}

int CX_RandomNumberGenerator::randomSignedInt (int rangeLower, int rangeUpper) {
	if (rangeUpper < rangeLower) {
		std::swap(rangeLower, rangeUpper);
		//return 0;
	}

	CX_RandomInt_t raw =  _mersenneTwister();

	raw = raw % (rangeUpper - rangeLower + 1);

	raw += rangeLower; //If rangeLower is negative, then raw is decreased (possibly into negative territory).

	return (int)raw;
}

/*
int64_t CX_RandomNumberGenerator::randomInt (int64_t rangeLower, int64_t rangeUpper) {
	if (rangeUpper < rangeLower) {
		return 0;
	}

	int64_t raw = randomInt();

	raw = raw % (rangeUpper - rangeLower);

	raw += rangeLower; //If rangeLower is negative, then raw is decreased (possibly into negative territory).

	return raw;
}
*/

/*!
This function returns a random unsigned 64-bit integer in the set {rangeLower, rangeLower + 1, ..., rangeUpper - 1, rangeUpper}.

This function returns an x such that x is an element of [rangeLower, rangeUpper] intersection Z.

If rangeLower > rangeUpper, the lower and upper ranges are swapped. If rangeLower == rangeUpper, it returns rangeLower.
*/
CX_RandomInt_t CX_RandomNumberGenerator::randomInt (CX_RandomInt_t rangeLower, CX_RandomInt_t rangeUpper) {
	if (rangeUpper < rangeLower) {
		std::swap(rangeLower, rangeUpper);
		//return 0;
	}

	if (rangeLower == rangeUpper) {
		return rangeLower;
	}

	CX_RandomInt_t raw = randomInt();

	raw = raw % (rangeUpper - rangeLower + 1);

	raw += rangeLower; //If rangeLower is negative, then raw is decreased (possibly into negative territory).

	return raw;
}

CX_RandomInt_t CX_RandomNumberGenerator::getMaximumRandomInt (void) {
	return _mersenneTwister.max();
}

CX_RandomInt_t CX_RandomNumberGenerator::getMinimumRandomInt (void) {
	return _mersenneTwister.min();
}

vector<int> CX_RandomNumberGenerator::sample (unsigned int count, int lowerBound, int upperBound, bool withReplacement) {
	return sample(count, CX::intVector(lowerBound, upperBound), withReplacement);
}


/*
This function returns a double drawn from a uniform distribution with a range of [0,1). Not implemented properly.

double CX_RandomNumberGenerator::uniformUnitInterval (void) {
	return _unitInterval( _mersenneTwister );
}

This function returns a double drawn from a uniform distribution with a range of [lowerBound_closed, upperBound_open).

double CX_RandomNumberGenerator::uniformDouble (double lowerBound_closed, double upperBound_open) {
	return std::uniform_real_distribution<double>(lowerBound_closed, upperBound_open)(_mersenneTwister); //Apparently this isn't implemented properly - it just returns ints.
}
*/