#pragma once

#include <random>
#include <cmath>
#include <vector>
#include <set>

#include <stdint.h>

#include "CX_Utilities.h"

/*! \defgroup randomNumberGeneration Randomization 
This module provides a class that is used for random number generation.
*/

namespace CX {

	typedef int64_t CX_RandomInt_t;

	/*! This class is used for generating random values from a pseudo-random number generator. If uses
	a version of the Mersenne Twister algorithm, in particular std::mt19937_64 (see 
	http://en.cppreference.com/w/cpp/numeric/random/mersenne_twister_engine for the parameters used with
	this algorithm).

	The monolithic structure of CX_RandomNumberGenerator provides a certain important feature that a collection
	of loose function does not have, which is the ability to trivially track the random seed being used
	for the random number generator. The function CX_RandomNumberGenerator::setSeed() sets the seed for
	all random number generation tasks performed by this class. Likewise, CX_RandomNumberGenerator::getSeed()
	allows you to easily find the seed that is being used for random number generation. Due to this structure,
	you can easily save the seed that was used for each participant, which allows you to repeat the exact
	randomizations used for that participant (unless random number generation varies as a function of the 
	responses given by a participant).

	An instance of this class is preinstantiated for you. See CX::Instances::RNG for information about the instance
	with that name.

	Because the underlying C++ std library random number generators are not thread safe, CX_RandomNumberGenerator
	is not thread safe. If you want to use a CX_RandomNumberGenerator in a thread, that thread should have its
	own CX_RandomNumberGenerator. You may seed the thread's CX_RandomNumberGenerator with CX::Instances::RNG.

	\ingroup randomNumberGeneration
	*/
	class CX_RandomNumberGenerator {
	public:

		CX_RandomNumberGenerator (void);

		void setSeed (unsigned long seed);
		unsigned long getSeed (void);
	
		CX_RandomInt_t getMinimumRandomInt(void);
		CX_RandomInt_t getMaximumRandomInt(void);
	
		CX_RandomInt_t randomInt(void);
		CX_RandomInt_t randomInt(CX_RandomInt_t rangeLower, CX_RandomInt_t rangeUpper);

		double randomDouble (double lowerBound_closed, double upperBound_open);

		template <typename stdDist>	std::vector<typename stdDist::result_type> sampleRealizations(unsigned int count, stdDist dist);
		std::vector<double> sampleUniformRealizations (unsigned int count, double lowerBound_closed, double upperBound_open);
		std::vector<unsigned int> sampleBinomialRealizations (unsigned int count, unsigned int trials, double probSuccess);
		std::vector<double> sampleNormalRealizations(unsigned int count, double mean, double standardDeviation);

		template <typename T> void shuffleVector(std::vector<T> *v);
		template <typename T> std::vector<T> shuffleVector(std::vector<T> v);

		template <typename T> T sample(std::vector<T> values);
		template <typename T> std::vector<T> sample(unsigned int count, const std::vector<T> &source, bool withReplacement);
		std::vector<int> sample(unsigned int count, int lowerBound, int upperBound, bool withReplacement);


		template <typename T> std::vector<T> sampleBlocks(const std::vector<T>& valueSet, unsigned int blocksToSample);

		template <typename T> T randomExclusive(const std::vector<T> &values, const T& exclude);
		template <typename T> T randomExclusive(const std::vector<T> &values, const std::vector<T> &exclude);

		std::mt19937_64& getGenerator(void);

	private:
		unsigned long _seed;

		std::mt19937_64 _mersenneTwister;
	};

	namespace Instances {
		extern CX_RandomNumberGenerator RNG;
	}

	/*! Randomizes the order of the given vector.
	\param v A pointer to the vector to be shuffled. */
	template <typename T>
	void CX_RandomNumberGenerator::shuffleVector(std::vector<T> *v) {
		std::shuffle( v->begin(), v->end(), _mersenneTwister );
	}

	/*! Makes a copy of the given vector, randomizes the order of its elements, and returns the shuffled copy.
	\param v The vector to be operated on. 
	\return A shuffled copy of v. */
	template <typename T>
	std::vector<T> CX_RandomNumberGenerator::shuffleVector(std::vector<T> v) {
		std::shuffle( v.begin(), v.end(), _mersenneTwister );
		return v;
	}

	/*!	Returns a vector of count values drawn randomly from source, with or without replacement. The returned values are in a random order.
	\param count The number of samples to draw.
	\param source A vector to be sampled from.
	\param withReplacement Sample with or without replacement.
	\return A vector of the sampled values.
	\note If (count > source.size() && withReplacement == false), an empty vector is returned.
	*/
	template <typename T>
	std::vector<T> CX_RandomNumberGenerator::sample(unsigned int count, const std::vector<T> &source, bool withReplacement) {

		std::vector<T> samples;

		if (source.size() == 0) {
			Instances::Log.error("CX_RandomNumberGenerator") << "sample: Empty vector given to sample from.";
			return samples;
		}

		if (withReplacement) {
			for (typename std::vector<T>::size_type i = 0; i < count; i++) {
				samples.push_back(source.at((std::vector<CX_RandomInt_t>::size_type)randomInt(0, source.size() - 1)));
			}
		} else {
			//Without replacement. Make a vector of indices into the source vector, shuffle them, and select count of them from the vector.
			if (count > source.size()) {
				//Log a warning?
				return samples;
			}
			std::vector<typename std::vector<T>::size_type> indices = shuffleVector(CX::Util::intVector<typename std::vector<T>::size_type>(0, source.size() - 1));
			for (unsigned int i = 0; i < count; i++) {
				samples.push_back( source[ indices[i] ] );
			}
		}

		return samples;
	}

	/*! Returns a single value sampled randomly from values.
	\return The sampled value.
	\note If values.size() == 0, an error will be logged and T() will be returned. */
	template <typename T> T CX_RandomNumberGenerator::sample(std::vector<T> values) {
		if (values.size() == 0) {
			Instances::Log.error("CX_RandomNumberGenerator") << "sample: Empty vector given to sample from.";
			return T();
		}
		return values[ randomInt(0, values.size() - 1) ];
	}

	/*! Get a random value from a vector, without the possibility of getting the excluded value.
	\param values The vectors of values to sample from.
	\param exclude The value to exclude from sampling. 
	\return The sampled value. 
	\note If all of the values are excluded, an error will be logged and T() will be returned. */
	template <typename T> T CX_RandomNumberGenerator::randomExclusive(const std::vector<T> &values, const T &exclude) {
		std::vector<T> excludes(1, exclude);
		return randomExclusive(values, excludes);
	}

	/*! Get a random value from a vector without the possibility of getting any of the excluded values.
	\param values The vector of values to sample from.
	\param exclude The vector of values to exclude from sampling.
	\return The sampled value.
	\note If all of the values are excluded, an error will be logged and T() will be returned. */
	template <typename T> T CX_RandomNumberGenerator::randomExclusive(const std::vector<T> &values, const std::vector<T> &exclude) {
		std::set<T> s(exclude.begin(), exclude.end());

		if (values.size() == exclude.size()) {
			bool allExcluded = true;
			for (typename std::vector<T>::size_type i = 0; i < values.size(); i++) {
				if (s.find(values[i]) == s.end()) {
					allExcluded = allExcluded && false;
				}
			}

			if (allExcluded) {
				CX::Instances::Log.error("CX_RandomNumberGenerator") << "randomExclusive: All values are excluded.";
				return T();
			}
		}

		T attempt;
		do {
			attempt = sample(values);
		} while (s.find(attempt) != s.end());
		return attempt;
	}

	/*! Draws `count` samples from a distribution `dist` that is provided by the user.
	\param count The number of samples to take.
	\param dist A configured instance of a distribution class that has operator()(Generator& g),
	where Generator is a random number generator that has operator() that returns a random value.
	Basically, just look at this page: http://en.cppreference.com/w/cpp/numeric/random and
	pick one of the random number distributions.
	\return A vector of stdDist::result_type, where stdDist::result_type is the type of data
	that is returned by the distribution (e.g. int, double, etc.). You can usually set this
	when creating the distribution object.

	\code{.cpp}
	//Take 100 samples from a poisson distribution with lamda (mean result value) of 4.2.
	//stdDist::result_type is unsigned int in this example.
	vector<unsigned int> rpois = RNG.sampleFrom(100, std::poisson_distribution<unsigned int>(4.2));
	\endcode
	*/
	template <typename stdDist>
	std::vector<typename stdDist::result_type> CX_RandomNumberGenerator::sampleRealizations(unsigned int count, stdDist dist) {
		std::vector<typename stdDist::result_type> rval(count);
		for (unsigned int i = 0; i < count; i++) {
			rval[i] = dist(_mersenneTwister);
		}
		return rval;
	}

	/*!	This function helps with the case where a set of V values must be sampled randomly
	with the constraint that each block of V samples should have each value in the set.
	For example, if you want to	present a number of trials in four different conditions,
	where the conditions are intermixed, but you want to observe all four trial types
	in every block of four trials, you would use this function.
	\param valueSet The set of values to sample from.
	\param blocksToSample The number of blocks to sample.
	\return A vector with `valueSize.size() * blocksToSample` elements.
	*/
	template <typename T>
	std::vector<T> CX_RandomNumberGenerator::sampleBlocks(const std::vector<T>& valueSet, unsigned int blocksToSample) {
		std::vector<T> rval(valueSet.size() * blocksToSample);
		std::vector<unsigned int> indices = CX::Util::intVector<unsigned int>(0, valueSet.size() - 1);

		for (unsigned int b = 0; b < blocksToSample; b++) {
			this->shuffleVector(&indices);
			for (unsigned int i = 0; i < indices.size(); i++) {
				rval[(b * indices.size()) + i] = valueSet[indices[i]];
			}
		}

		return rval;
	}

}
