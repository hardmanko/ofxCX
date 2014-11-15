#pragma once

#include <exception>
#include <vector>
#include <functional>

#include "CX_Logger.h"
#include "CX_RandomNumberGenerator.h"

namespace CX {
	/*! This namespace contains a few complex algorithms that can be difficult to properly implement
	or are very experiment-specific. */
	namespace Algo {
		template <typename T> std::vector<T> generateSeparatedValues(int count, double minDistance, std::function<double(T, T)> distanceFunction, std::function<T(void)> randomDeviate, int maxSequentialFailures = 200);
		template <typename T> std::vector< std::vector<T> > fullyCross (std::vector< std::vector<T> > factors);

		/*! This class provides a way to work with Latin squares in a relatively easy way. 
		\code{.cpp}
		Algo::LatinSquare ls(4); //Construct a standard 4x4 LatinSquare.
		cout << "This latin square has " << ls.rows() << " rows and " << ls.columns() << " columns." << endl;
		cout << ls.print() << endl;

		ls.reverseColumns();
		cout << "Reverse the columns: " << endl << ls.print() << endl;

		ls.swapRows(0, 2);
		cout << "Swap rows 0 and 2: " << endl << ls.print() << endl;

		if (ls.validate()) {
			cout << "The latin square is still a valid latin square." << endl;
		}

		cout << "Let's copy, reverse, and append a latin square." << endl;
		Algo::LatinSquare sq = ls;
		sq.reverseColumns();
		ls.appendBelow(sq);

		cout << ls.print() << endl;
		if (!ls.validate()) {
			cout << "The latin square is no longer valid, but it is still useful (8 counterbalancing conditions, both forward and backward ordering)." << endl;
		}
		\endcode
		*/
		class LatinSquare {
		public:
			LatinSquare(void);
			LatinSquare(unsigned int dimensions);

			void generate(unsigned int dimensions);

			void reorderRight(void);
			void reorderLeft(void);
			void reorderUp(void);
			void reorderDown(void);

			void reverseColumns(void);
			void reverseRows(void);

			void swapColumns(unsigned int c1, unsigned int c2);
			void swapRows(unsigned int r1, unsigned int r2);

			bool appendRight(const LatinSquare& ls);
			bool appendBelow(const LatinSquare& ls);

			LatinSquare& operator+=(unsigned int value);

			std::string print(std::string delim = ",");

			bool validate(void) const;

			unsigned int columns(void) const;
			unsigned int rows(void) const;

			std::vector<unsigned int> getColumn(unsigned int col) const;
			std::vector<unsigned int> getRow(unsigned int row) const;

			std::vector< std::vector<unsigned int> > square; //!< The Latin square.
			
		private:
			unsigned int _columns;
		};


		/*! This class helps with the case where a set of V values must be sampled randomly
		with the constraint that each block of V samples should have each value in the set.
		For example, if you want to	present a number of trials in four different conditions, 
		where the conditions are intermixed, but you want to observe all four trial types 
		every four trials, you could use this class.

		\code{.cpp}
		#include "CX.h"

		void runExperiment(void) {
			//Construct a BlockSampler using RNG as the random number generator
			//and integer values 1 to 4 as the data to sample from.
			Algo::BlockSampler<int> bs(&RNG, Util::intVector(1, 4)); 
				
			//Generate 4 blocks of values and print those values along with information about the block and position
			cout << "Block, Position: Value" << endl;
			while (bs.getBlockNumber() < 4) {
				cout << bs.getBlockNumber() << ", " << bs.getBlockPosition() << ": ";
				cout << bs.getNextValue() << endl;
			}
		}
		\endcode

		\note Another way of getting blocked random samples is to use CX::CX_RandomNumberGenerator::sampleBlocks().
		*/
		template <typename T>
		class BlockSampler {
		public:

			BlockSampler(void) :
				_rng(nullptr)
			{}

			/*! Constructs a BlockSampler with the given settings. See setup() for the meaning of the parameters. */
			BlockSampler(CX_RandomNumberGenerator* rng, const std::vector<T>& values) {
				setup(rng, values);
			}

			/*! Set up the BlockSampler.
			\param rng A pointer to a CX_RandomNumberGenerator that will be used to randomize the sampled data.
			\param values A vector of values from which to sample.
			*/
			void setup(CX_RandomNumberGenerator* rng, const std::vector<T>& values) {
				_rng = rng;
				_values = values;
				_blockIndices = Util::intVector<unsigned int>(0, _values.size() - 1);
				restartSampling();
			}

			/*! Get the next value sampled from the provided data.
			\return An element sampled from the provided values, or if there were no values provided, 
			a warning will be logged and a default-constructed instance of T will be returned. */
			T getNextValue(void) {
				if (_values.size() == 0) {
					CX::Instances::Log.warning("BlockSampler") << "getNextValue: Value requested but no values avaiable to sample from."
						"Did you provide a vector of values to the BlockSampler?";
					return T();
				}

				T rval = _values[_blockIndices[_blockPosition]];
				if (++_blockPosition >= _blockIndices.size()) {
					_blockPosition = 0;
					_rng->shuffleVector(&_blockIndices);
					_blockNumber++;
				}
				return rval;
			}

			/*! Restarts sampling to be at the beginning of a block of samples. Also resets the block number (  */
			void restartSampling(void) {
				_blockPosition = 0;
				_blockNumber = 0;
				_rng->shuffleVector(&_blockIndices);
			}

			/*! Returns the index of the block that is currently being sampled. Because it is zero-indexed, 
			you can alternately think of the value as the number of completed blocks. */
			unsigned int getBlockNumber(void) const {
				return _blockNumber;
			}

			/*! Returns the index of the sample that will be taken the next time getNextValue() is called. If 0,
			it means that a block of samples was just finished. 
			If within the current block 4 samples had already been taken, this will return 4 */
			unsigned int getBlockPosition(void) const {
				return _blockPosition;
			}

		private:
			CX_RandomNumberGenerator* _rng;

			std::vector<T> _values;

			std::vector<unsigned int> _blockIndices;
			unsigned int _blockPosition;
			unsigned int _blockNumber;
		};
	}
}

/*! This algorithm is designed to deal with the situation in which a number
of random values must be generated that are each at least some distance from every other
random value. This is a very generic implementation of this algorithm. It works by taking
pointers to two functions that work on whatever type of data you are using. The first
function is a distance function: it returns the distance between two values of the type.
You can define distance in whatever way you would like. The second function generates 
random values of the type.
\tparam <T> The type of data you are working with.
\param count The number of values you want to be generated.
\param minDistance The minimum distance between any two values. This will be compared to the result of distanceFunction.
\param distanceFunction A function that computes the distance, in whatever units you want, between two values of type T.
\param randomDeviate A function that generates random values of type T.
\param maxSequentialFailures The maximum number of times in a row that a newly-generated value is less than minDistance
from at least one other value. This essentially makes sure that if it is not possible to generate a random value that
is at least some distance from the others, the algorithm will terminate.
\return A vector of values. If the function terminated prematurely due to maxSequentialFailures being reached, the
returned vector will have 0 elements.

\code{.cpp}
//This example function generates locCount points with both x and y values bounded by minimumValues and maximumValues that 
//are at least minDistance pixels from each other.
std::vector<ofPoint> getObjectLocations(int locCount, double minDistance, ofPoint minimumValues, ofPoint maximumValues) {
	auto pointDistance = [](ofPoint a, ofPoint b) {
		return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
	};

	auto randomPoint = [&]() {
		ofPoint rval;
		rval.x = RNG.randomInt(minimumValues.x, maximumValues.x);
		rval.y = RNG.randomInt(minimumValues.y, maximumValues.y);
		return rval;
	};

	return CX::Algo::generateSeparatedValues<ofPoint>(locCount, minDistance, pointDistance, randomPoint, 1000);
}

//Call of example function
vector<ofPoint> v = getObjectLocations(5, 50, ofPoint(0, 0), ofPoint(400, 400));
\endcode
*/
template <typename T>
std::vector<T> CX::Algo::generateSeparatedValues (int count, double minDistance, std::function<double(T, T)> distanceFunction, 
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
				CX::Instances::Log.error("CX::Algo::generateSeparatedValues") << "Maximum number of sequential failures reached. Returning an empty vector.";
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

Example use:
\code{.cpp}
std::vector< std::vector<int> > levels(2); //Two factors
levels[0].push_back(1); //The first factor has two levels (1 and 2)
levels[0].push_back(2);
levels[1].push_back(3); //The second factor has three levels (3, 4, and 5)
levels[1].push_back(4);
levels[1].push_back(5);
auto crossed = fullyCross(levels);
\endcode
crossed should contain a vector with six subvectors with the contents: 
\code
{ {1,3}, {1,4}, {1,5}, {2,3}, {2,4}, {2,5} }
\endcode
where
\code
crossed[3][0] == 2
crossed[3][1] == 3
crossed[0][1] == 3
\endcode

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

	//Now we have the data, but in wrong format, so transpose it.
	std::vector< std::vector<T> > rval(crossedLevels);
	for (unsigned int level = 0; level < crossedLevels; level++) {
		rval[level].resize( factors.size() );
		for (unsigned int factor = 0; factor < factors.size(); factor++) {
			rval[level][factor] = formatOne[factor][level];
		}
	}

	return rval;
}

