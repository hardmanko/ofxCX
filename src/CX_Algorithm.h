
#include <vector>
#include <functional>

#include "CX_Logger.h"

namespace CX {
	namespace Algo {
		template <typename T> std::vector<T> getSepDist (int count, double minDistance, std::function<double(T, T)> distanceFunction, std::function<T(void)> randomDeviate, int maxSequentialFailures);
		template <typename T> std::vector< std::vector<T> > fullyCross (std::vector< std::vector<T> > factors);
	}
}

template <typename T>
std::vector<T> CX::Algo::getSepDist (int count, double minDistance, std::function<double(T, T)> distanceFunction, 
						  std::function<T(void)> randomDeviate, int maxSequentialFailures) 
{
	std::vector<T> samples;
	int sequentialFailures = 0;

	for (int i = 0; i < count; i++) {

		bool failed = false;
		T sample = randomDeviate();

		for (auto s : samples) {
			if (distanceFunction(s, sample) < minDistance) {
				failed = true;
				break;
			}
		}

		if (failed) {
			if (++sequentialFailures >= maxSequentialFailures) {
				CX::Instances::Log.error("CX::Algo::getSepDist") << "Maximum number of sequential failures reached. Returning an empty vector.";
				return std::vector<T>();
			}
			i--;
			continue;
		}
		else {
			sequentialFailures = 0;
			samples.push_back(sample);
		}
	}

	return samples;
}

/*!
This function fully crosses the levels of the factors of a design. For example, for a 2X3 design,
it would give you all 6 combinations of the levels of the design.
\param factors A vector of factors, each factor being a vector containing all the levels of that factor.
\return A vector of crossed factor levels. It's length is equal to the product of the levels of the factors.
The length of each "row" is equal to the number of factors.

Example call:
std::vector< std::vector<int> > levels(2);
levels[0].push_back(1);
levels[0].push_back(2);
levels[1].push_back(3);
levels[1].push_back(4);
levels[1].push_back(5);
auto crossed = fullyCross(levels);

The output would be: { {1,3}, {1,4}, {1,5}, {2,3}, {2,4}, {2,5} }
and:
crossed[3][0] == 2
crossed[3][1] == 3
crossed[0][1] == 3
etc.
*/
template <typename T>
std::vector< std::vector<T> > CX::Algo::fullyCross (std::vector< std::vector<T> > factors) {

	unsigned int crossedLevels = 1;
	for (unsigned int factor = 0; factor < factors.size(); factor++) {
		crossedLevels *= factors[factor].size();
	}

	std::vector< std::vector<T> > formatOne(factors.size());
	unsigned int lback = 1;
	for (unsigned int factor = 0; factor < factors.size(); factor++) {

		unsigned int lcurrent = factors[factor].size();
		unsigned int lforward = crossedLevels / (lback * lcurrent);

		for (unsigned int b = 0; b < lback; b++) {
			for (unsigned int c = 0; c < lcurrent; c++) {
				for (unsigned int f = 0; f < lforward; f++) {
					formatOne[factor].push_back(factors[factor][c]);
				}
			}
		}

		lback *= lcurrent;
	}

	//Now we have the data, but in wrong format.
	std::vector< std::vector<T> > rval(crossedLevels);
	for (unsigned int level = 0; level < crossedLevels; level++) {
		rval[level].resize( factors.size() );
		for (unsigned int factor = 0; factor < factors.size(); factor++) {
			rval[level][factor] = formatOne[factor][level];
		}
	}

	return rval;
}