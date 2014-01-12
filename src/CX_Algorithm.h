
#include <vector>
#include <functional>


template <typename T>
std::vector<T> getSepDist(int count, double minDistance, std::function<double(T, T)> distanceFunction,
	std::function<T(void)> randomDeviate, int maxSequentialFailures) {
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
				//Log the error condition...
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

/*
This function fully crosses the levels of the factors of a design. For example, for a 2X3 design,
it would give you all 6 combinations of the levels of the design.
\param factors A vector of factors, each factor being a vector containing all the levels of that factor.
\return A vector of factors, where each factor is a vector containing the levels of that factor, arranged
so that accessing the ith element of all of the factors will give you a unique combination of the levels of the factors.

E.g.:
std::vector< std::vector<int> > levels(2);
levels[0].push_back(1);
levels[0].push_back(2);
levels[1].push_back(3);
levels[1].push_back(4);
levels[1].push_back(5);
auto crossed = fullyCross(levels);
*/
template <typename T>
std::vector< std::vector<T> > fullyCross(std::vector< std::vector<T> > factors) {

	unsigned int crossedLevels = 1;
	for (int factor = 0; factor < factors.size(); factor++) {
		crossedLevels *= factors[factor].size();
	}

	std::vector< std::vector<T> > rval(factors.size());
	int lback = 1;
	for (int factor = 0; factor < factors.size(); factor++) {

		int lcurrent = factors[factor].size();
		int lforward = crossedLevels / (lback * lcurrent);

		for (int b = 0; b < lback; b++) {
			for (int c = 0; c < lcurrent; c++) {
				for (int f = 0; f < lforward; f++) {
					rval[factor].push_back(factors[factor][c]);
				}
			}
		}

		lback *= lcurrent;
	}
	return rval;
}