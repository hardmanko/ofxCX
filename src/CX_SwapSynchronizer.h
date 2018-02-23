#pragma once

#include <deque>

#include "CX_Time_t.h"

namespace CX {
namespace Private {


class CX_SwapLinearModel {
public:

	struct Datum {
		uint64_t unit;
		CX_Millis time;
	};


	CX_SwapLinearModel(void);

	void setup(unsigned int sampleSize);

	void store(uint64_t unit, CX_Millis y);
	unsigned int storedSamples(void);
	void clear(void);

	bool updateModel(void);

	bool ready(void);

	CX_Millis getY(uint64_t unit);
	uint64_t getX(CX_Millis t);


	double getSlope(void);
	double getMillisecondsPerUnit(void) { return getSlope(); };
	CX_Millis getIntercept(void);

	std::deque<Datum>& getData(void);

private:

	unsigned int _sampleSize;
	std::deque<Datum> _data;

	bool _modelNeedsUpdate;

	double _slope; // milliseconds per unit
	CX_Millis _intercept;
};

class CX_SwapSynchronizer {
public:

	struct TestConfig {

		TestConfig(void) :
			swapIntervals(true),
			nextSwapPredictions(false),
			modelSlope(false),
			modelNextSwapPredictions(false)
		{}

		bool swapIntervals; // differences between successive swaps
		bool nextSwapPredictions; // Third argument of store()
		bool modelSlope; // Slope of model (ms per unit)
		bool modelNextSwapPredictions; // Internal model predictions

		void testAll(void) {
			swapIntervals = true;
			nextSwapPredictions = true;
			modelSlope = true;
			modelNextSwapPredictions = true;
		}

		void testNone(void) {
			swapIntervals = false;
			nextSwapPredictions = false;
			modelSlope = false;
			modelNextSwapPredictions = false;
		}

	};

	struct Configuration {

		Configuration(void) :
			requiredSwaps(5),
			nominalSwapPeriod(-1000),
			swapPeriodTolerance(0.10), // 10% tolerance
			swapAdvancesUnits(1) // swaps advance by some number of units (1 for display, some number of sample frames for sound stream)
		{}

		unsigned int requiredSwaps;

		CX_Millis nominalSwapPeriod;

		double swapPeriodTolerance; // proportion of nominalSwapPeriod

		TestConfig test;

		uint64_t swapAdvancesUnits;

	};


	CX_SwapSynchronizer(void);
	CX_SwapSynchronizer(const Configuration& config);

	bool setup(const Configuration& config);

	const Configuration& getConfiguration(void) const;

	void clear(void);

	void store(uint64_t swapNumber, CX_Millis swapTime);
	void store(uint64_t swapNumber, CX_Millis swapTime, CX_Millis nextSwapPrediction);

	bool ready(void) const;

	bool synchronized(void);
	bool synchronized(const TestConfig& test);

	bool testSwapIntervals(void);
	bool testModelNextSwapPredictions(void);
	bool testModelSlope(void);
	bool testNextSwapPredictions(void);

	// Times are stored as milliseconds
	CX_SwapLinearModel& getLM(void) {
		return _lm;
	}

private:

	Configuration _config;
	CX_Millis _calcTolerance;

	struct Datum {
		uint64_t swapNumber;
		CX_Millis swapTime;

		CX_Millis modelNextSwapEst;
		CX_Millis userNextSwapEst;
	};

	std::deque<Datum> _data;
	CX_SwapLinearModel _lm;

	bool _syncResultOnLastTest;
	bool _dataChangeSinceLastTest;

	bool _areTimesWithinTolerance(CX_Millis a, CX_Millis b, CX_Millis tolerance) const;

};

} // namespace Private
} // namespace CX