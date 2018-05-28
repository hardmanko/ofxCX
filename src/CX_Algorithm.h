#pragma once

#include <exception>
#include <vector>
#include <map>
#include <utility> //pair
#include <string>
#include <functional>

#include "CX_Logger.h"
#include "CX_RandomNumberGenerator.h"
#include "CX_DataFrame.h"

namespace CX {

	/*! The `Algo` namespace contains a few complex algorithms that can be difficult to properly implement
	or are psychology-experiment-specific. */
	namespace Algo {

		template <typename dataT, typename distT>
		std::vector<dataT> generateSeparatedValues(int count, distT minDistance, std::function<distT(dataT, dataT)> distanceFunction,
												std::function<dataT(void)> randomDeviate, unsigned int maxSequentialFailures, int maxRestarts);

		template <typename T> 
		std::vector< std::vector<T> > fullyCross (std::vector< std::vector<T> > factors);

		template <typename T>
		CX_DataFrame fullyCross(std::map<std::string, std::vector<T>>& factors);

		/*! This class provides a way to work with Latin squares in a relatively easy way. 

		The constructed Latin squares use 0-indexed integers for the values, 
		meaning that a 3x3 square will have the values 0, 1, and 2 in various orders.

		Each row of the square is one condition of the design, so take use rows to determine condition order.

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
			void generateBalanced(unsigned int dimensions);

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
			size_t _columns;
		};


		/*! This class implements a simple linear regression model that 
		1. Collects samples of data over time using the store() function.
		2. Calculates new parameter values when updateModel() is called.
		3. The availability of valid parameter values can be checked with modelValid().
		4. With valid parameter values, calculates predicted x and y values with getX() and getY().
		5. Does all of this in a thread-safe way.

		This class is semi-internal to CX and is not well-documented, but is publicly available.

		\code{.cpp}
		Algo::RollingLinearModel rlm;

		rlm.setup(false, 10, 3);

		vector<double> x = { 0, 2, 4, 6, 9 };
		vector<double> y = { 15, 6, 8, 3, 0 };

		for (unsigned int i = 0; i < x.size(); i++) {
			rlm.store(x[i], y[i]);
		}

		if (rlm.modelReady()) {
			double predY = rlm.getY(5);
			double predX = rlm.getX(10);
		}
		\endcode
		*/
		class RollingLinearModel {
		public:

			RollingLinearModel(void);

			void setup(bool autoUpdate, unsigned int maxSamples, unsigned int minSamples = 3);

			void store(double x, double y);
			void storeMultiple(const std::vector<double>& x, const std::vector<double>& y);
			unsigned int storedSamples(void);
			void clear(void);

			bool updateModel(void);
			bool updateModelOnSubset(unsigned int start, unsigned int end);

			bool modelReady(void);

			double getY(double x);
			double getX(double y);

			double getSlope(void);
			double getIntercept(void);

			struct Datum {

				Datum(double x_, double y_) :
					x(x_),
					y(y_)
				{}

				double x;
				double y;
			};

			std::deque<Datum>& getData(void);

		private:

			std::recursive_mutex _mutex;

			bool _autoUpdate;
			bool _modelNeedsUpdate;

			unsigned int _minSamples;
			unsigned int _maxSamples;

			double _slope;
			double _intercept;

			std::deque<Datum> _data;

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

			/*! Set up the BlockSampler from a CX_DataFrame. This only works if the
			BlockSampler is templated to use CX_DataFrameRow as its type. Each row
			of the CX_DataFrame will be copied into the BlockSampler.
			
			\param rng A pointer to a CX_RandomNumberGenerator that will be used to randomize the sampled data.
			\param df A CX_DataFrame.
			*/
			void setup(CX_RandomNumberGenerator* rng, const CX_DataFrame& df) {
				std::vector<CX_DataFrameRow> allRows(df.getRowCount());
				for (CX_DataFrame::RowIndex i = 0; i < df.getRowCount(); i++) {
					allRows[i] = df.copyRow(i);
				}
				this->setup(rng, allRows);
			}

			/*! Get the next value sampled from the provided data.
			\return An element sampled from the provided values, or, if there were no values provided, 
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


		/*! This algorithm is designed to deal with the situation in which a number
		of random values must be generated that are each at least some distance from every other
		random value. This is a very generic implementation of this algorithm. It works by taking
		pointers to two functions that work on whatever type of data you are using. 

		The first function is a distance function: it returns the distance between two values of the type.
		You can define distance in whatever way you would like. Distance does not even need to be 
		unidimensional: note that the type of data used for distance is a template parameter. The 
		distance type must have operator<(distT,distT) defined.

		The second function generates random values of the type.


		\tparam <dataT> The type of data you are working with.
		\tparam <distT> The type of distance units used.

		\param count The number of values you want to be generated.
		\param minDistance The minimum distance between any two values. This will be compared to the result of distanceFunction.
		\param distanceFunction A function that computes the distance, in whatever units you want, between two values of type T.
		\param randomDeviate A function that generates random values of type T.
		\param maxSequentialFailures The maximum number of times in a row that a newly-generated value can be less than minDistance
		from at least one other value. If this number of failures is reached, the process will be restarted depending on the setting
		of maxRestarts. This is to help make sure that if the algorithm is having a hard time finding a value that works given the
		other values that have been selected, it doesn't just run forever, but tries over with new values.
		\param maxRestarts If non-negative, the number of times that the algorithm will restart before giving up. If negative,
		the algorithm will never give up. Note that this may result in an infinite loop if it is impossible to get enough samples.
		\return A vector of values. If the function terminated prematurely due to maxSequentialFailures being reached, the
		returned vector will have 0 elements.

		\code{.cpp}
		//This example function generates locCount points with both x and y values bounded by minimumValues and maximumValues that
		//are at least minDistance from each other.
		std::vector<ofPoint> getObjectLocations(int locCount, float minDistance, ofPoint minimumValues, ofPoint maximumValues) {
			//pointDistance is an anonymous function that takes two ofPoints as arguments and returns a float.
			auto pointDistance = [](ofPoint a, ofPoint b) -> float {
				return a.distance(b);
			};

			//randomPoint is an anonymous function that takes no arguments explicitly, but it captures by reference everything from
			//the enclosing environment (specifically minimumValues and maximumValues).
			auto randomPoint = [&]() -> ofPoint {
				ofPoint rval;
				rval.x = RNG.randomInt(minimumValues.x, maximumValues.x);
				rval.y = RNG.randomInt(minimumValues.y, maximumValues.y);
				return rval;
			};

			return CX::Algo::generateSeparatedValues<ofPoint, float>(locCount, minDistance, pointDistance, randomPoint, 1000, 100);
		}

		//Call of example function
		vector<ofPoint> v = getObjectLocations(5, 50, ofPoint(0, 0), ofPoint(400, 400));
		\endcode
		*/
		template <typename dataT, typename distT>
		std::vector<dataT> generateSeparatedValues(int count, distT minDistance, std::function<distT(dataT, dataT)> distanceFunction,
															 std::function<dataT(void)> randomDeviate, unsigned int maxSequentialFailures, int maxRestarts)
		{
			std::vector<dataT> samples;
			unsigned int sequentialFailures = 0;

			while (samples.size() < count) {
				bool sampleRejected = false;
				dataT sample = randomDeviate();

				for (dataT& s : samples) {
					if (distanceFunction(s, sample) < minDistance) {
						sampleRejected = true;
						break;
					}
				}

				if (sampleRejected) {
					if (++sequentialFailures >= maxSequentialFailures) {
						//If maxRestarts is greater than 0, restart. If it's less than 0, restart continuously.
						if (maxRestarts != 0) {
							if (maxRestarts < 0) {
								maxRestarts++;
							}
							return generateSeparatedValues(count, minDistance, distanceFunction, randomDeviate, maxSequentialFailures, maxRestarts - 1);
						} else {
							CX::Instances::Log.error("CX::Algo::generateSeparatedValues") << "Maximum number of restarts reached. Returning an empty vector.";
							return std::vector<dataT>();
						}
					}
				} else {
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
		std::vector< std::vector<T> > fullyCross (std::vector< std::vector<T> > factors) {

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

		/*! This function does the same thing as \ref CX::Algo::fullyCross(std::vector< std::vector<T> > factors),
		except that it returns a CX_DataFrame, which means that you can access factor values by the name of the
		factor, rather than an index. You can see this in the example.

		\code{.cpp}
		vector<string> shapes = { "square", "rectangle", "triangle" };
		vector<string> numbers = { "1.5", "3.7" };

		map<string, vector<string>> factors;
		factors["shapes"] = shapes;
		factors["numbers"] = numbers;

		CX_DataFrame crossed = Algo::fullyCross(factors);

		cout << crossed.print() << endl;

		double firstNumber = crossed(0, "numbers").toDouble();
		string secondShape = crossed(1, "shapes").toString();
		\endcode

		\tparam T The type of data to use. Typically, using `string`s works well, as you can stringify a number
		(or other type) and then extract that type from the CX_DataFrame, as can be seen in the example with the
		"numbers" factor.
		\param factors A map that uses the name of a factor as the key and a vector of factor levels as the value.
		*/
		template <typename T>
		CX_DataFrame fullyCross(std::map<std::string, std::vector<T>>& factors) {
	
			std::vector< std::string > factorNames;
			std::vector< std::vector<T> > vFactors;

			for (std::pair<std::string, std::vector<T>> f : factors) {
				factorNames.push_back(f.first);
				vFactors.push_back(f.second);
			}

			std::vector< std::vector<T> > crossed = fullyCross(vFactors);

			CX::CX_DataFrame rval;

			for (unsigned int i = 0; i < crossed.size(); i++) {
				for (unsigned int f = 0; f < crossed.at(i).size(); f++) {
					rval(i, factorNames.at(f)) = crossed.at(i).at(f);
				}
			}

			return rval;
		}


	} //namespace Algo
} //namespace CX
