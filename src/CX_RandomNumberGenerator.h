#ifndef _CX_RANDOM_NUMBER_GENERATOR_H_
#define _CX_RANDOM_NUMBER_GENERATOR_H_

#include <random>
#include <vector>
#include <set>

#include <stdint.h>

#include "CX_Utilities.h"

namespace CX {

	typedef int64_t CX_RandomInt_t;

	class CX_RandomNumberGenerator {
	public:

		CX_RandomNumberGenerator (void);

		void setSeed (uint64_t seed);
		uint64_t getSeed (void) { return _seed; };
	
		CX_RandomInt_t getMinimumRandomInt(void);
		CX_RandomInt_t getMaximumRandomInt(void);
	
		CX_RandomInt_t randomInt(void);
		CX_RandomInt_t randomInt(CX_RandomInt_t rangeLower, CX_RandomInt_t rangeUpper);

		template <typename T> T randomExclusive (const vector<T> &values, const T& exclude);
		template <typename T> T randomExclusive (const vector<T> &values, const vector<T> &exclude);

		double uniformDouble(double lowerBound_closed, double upperBound_open);
		template <typename T> std::vector<T> binomialDeviates (unsigned int count, T trials, double probSuccess);
		std::vector<double> normalDeviates (unsigned int count, double mean, double standardDeviation);

		template <typename T> void shuffleVector (vector<T> *v);
		template <typename T> vector<T> shuffleVector (vector<T> v);
		template <typename T> T sample (vector<T> values);
		template <typename T> vector<T> sample (unsigned int count, const vector<T> &source, bool withReplacement);
		vector<int> sample (unsigned int count, int lowerBound, int upperBound, bool withReplacement);

	private:
		uint64_t _seed;

		std::mt19937_64 _mersenneTwister;
	};

	namespace Instances {
		extern CX_RandomNumberGenerator RNG;
	}

	template <typename T>
	void CX_RandomNumberGenerator::shuffleVector (vector<T> *v) {
		std::shuffle( v->begin(), v->end(), _mersenneTwister );
	}

	template <typename T>
	vector<T> CX_RandomNumberGenerator::shuffleVector (vector<T> v) {
		std::shuffle( v.begin(), v.end(), _mersenneTwister );
		return v;
	}

	/*!
	Returns a vector of count values drawn from source, with or without replacement. The returned values
	are in a random order.
	If (count > source.size() && withReplacement == false), an empty vector is returned.
	*/
	template <typename T>
	vector<T> CX_RandomNumberGenerator::sample (unsigned int count, const vector<T> &source, bool withReplacement) {
		vector<T> samples;

		if (withReplacement) {
			for (vector<T>::size_type i = 0; i < count; i++) {
				samples.push_back( source.at( (vector<CX_RandomInt_t>::size_type)randomInt(0, source.size() - 1) ) );
			}
		} else {
			//Without replacement. Make a vector of indices into the source vector, shuffle them, and select count of them from the vector.

			if (count > source.size()) {
				return samples;
			}

			vector<vector<T>::size_type> indices = shuffleVector( CX::intVector<vector<T>::size_type>(0, source.size() - 1) );

			for (unsigned int i = 0; i < count; i++) {
				samples.push_back( source[ indices[i] ] );
			}
		}

		return samples;
	}

	template <typename T> T CX_RandomNumberGenerator::sample (vector<T> values) {
		return values[ randomInt(0, values.size() - 1) ];
	}

	template <typename T> T CX_RandomNumberGenerator::randomExclusive (const vector<T> &values, const T &exclude) {
		std::vector<T> excludes(1, exclude);
		return randomExclusive(values, excludes);
	}

	template <typename T> T CX_RandomNumberGenerator::randomExclusive (const vector<T> &values, const vector<T> &exclude) {
		std::set<T> s(exclude.begin(), exclude.end());

		if (values.size() == exclude.size()) {
			bool allExcluded = true;
			for (std::vector<T>::size_type i = 0; i < values.size(); i++) {
				if (s.find(values[i]) == s.end()) {
					allExcluded = allExcluded && false;
				}
			}

			if (allExcluded) {
				Log.error("CX_RandomNumberGenerator") << "randomExclusive: All values are excluded.";
				return T();
			}
		}

		T attempt;
		do {
			attempt = sample(values);
		} while (s.find(attempt) != s.end());
		return attempt;
	}

	template <typename T> std::vector<T> CX_RandomNumberGenerator::binomialDeviates (unsigned int count, T trials, double probSuccess) {
		std::vector<T> samples(count);
		std::binomial_distribution<T> binDist(trials, probSuccess);
		for (uint64_t i = 0; i < count; i++) {
			samples[i] = binDist(_mersenneTwister);
		}
		return samples;
	}



}

#endif //_CX_RANDOM_NUMBER_GENERATOR_H_