#include "CX_SwapSynchronizer.h"

#include "CX_Logger.h"


namespace CX {
namespace Private {


////////////////////////
// CX_SwapLinearModel //
////////////////////////

CX_SwapLinearModel::CX_SwapLinearModel(void) :
	_sampleSize(10),
	_modelNeedsUpdate(true),
	_slope(0),
	_intercept(0)
{}

void CX_SwapLinearModel::setup(unsigned int sampleSize) {
	_data.clear();

	_sampleSize = std::max(sampleSize, 2U);
}

void CX_SwapLinearModel::store(uint64_t unit, CX_Millis time) {
	_data.push_back(Datum{ unit, time });

	while (_data.size() > _sampleSize) {
		_data.pop_front();
	}

	_modelNeedsUpdate = true;
}

unsigned int CX_SwapLinearModel::storedSamples(void) {
	return _data.size();
}

void CX_SwapLinearModel::clear(void) {
	_data.clear();
	_modelNeedsUpdate = true;
}

bool CX_SwapLinearModel::updateModel(void) {

	if (!_modelNeedsUpdate) {
		return true;
	}

	if (_data.size() != _sampleSize) {
		return false;
	}

	double xBar = 0;
	double yBar = 0;
	for (unsigned int i = 0; i < _data.size(); i++) {
		xBar += (double)_data[i].unit;
		yBar += _data[i].time.millis();
	}
	xBar /= _sampleSize;
	yBar /= _sampleSize;


	double numSum = 0;
	double denSum = 0;

	for (unsigned int i = 0; i < _data.size(); i++) {

		double xDif = (_data[i].unit - xBar);

		numSum += xDif * (_data[i].time.millis() - yBar);

		denSum += xDif * xDif;
	}

	_slope = numSum / denSum;
	_intercept = CX_Millis(yBar - _slope * xBar);

	_modelNeedsUpdate = false;

	return true;

}

bool CX_SwapLinearModel::ready(void) {
	return updateModel();
}

CX_Millis CX_SwapLinearModel::getY(uint64_t unit) {
	if (!ready()) {
		return 0;
	}
	return CX_Millis(_slope * unit + _intercept.millis());
}

uint64_t CX_SwapLinearModel::getX(CX_Millis t) {
	if (!ready()) {
		return 0;
	}
	return (t - _intercept).millis() / _slope;
}

double CX_SwapLinearModel::getSlope(void) {
	if (!ready()) {
		return 0;
	}
	return _slope;
}

CX_Millis CX_SwapLinearModel::getIntercept(void) {
	if (!ready()) {
		return 0;
	}
	return _intercept;
}

std::deque<CX_SwapLinearModel::Datum>& CX_SwapLinearModel::getData(void) {
	return _data;
}


////////////////////////////////
// CX_SwapSynchronizer //
////////////////////////////////

CX_SwapSynchronizer::CX_SwapSynchronizer(void) :
	_syncResultOnLastTest(false),
	_dataChangeSinceLastTest(true),
	_calcTolerance(0)
{}

CX_SwapSynchronizer::CX_SwapSynchronizer(const Configuration& config) :
	CX_SwapSynchronizer()
{
	setup(config);
}

const CX_SwapSynchronizer::Configuration& CX_SwapSynchronizer::getConfiguration(void) const {
	return _config;
}

bool CX_SwapSynchronizer::setup(const Configuration& config) {

	bool anyTests = config.test.swapIntervals || config.test.nextSwapPredictions || config.test.modelSlope || config.test.modelNextSwapPredictions;
	if (!anyTests) {
		return false;
	}

	_config = config;

	if (_config.requiredSwaps < 2) {
		Instances::Log.notice("CX_SwapSynchronizer") << "setup(): The requiredSwaps were less than 2, but must be at least 2. requiredSwaps was set to 2.";
		_config.requiredSwaps = 2;
	}

	_calcTolerance = _config.nominalSwapPeriod * _config.swapPeriodTolerance;

	clear();

	_lm.setup(_config.requiredSwaps);

	return true;
}

void CX_SwapSynchronizer::clear(void) {
	_data.clear();
	_lm.clear();

	_dataChangeSinceLastTest = true;
}

void CX_SwapSynchronizer::store(uint64_t swapNumber, CX_Millis swapTime) {

	// Don't test predictions for next swap time if this version of store() is used.
	if (_config.test.nextSwapPredictions) {
		_config.test.nextSwapPredictions = false;
		Instances::Log.warning("CX_SwapSynchronizer") << "store(uint64_t, CX_Millis) was used while testing nextSwapPredictions. nextSwapPredictions will no longer be tested.";
	}
	
	store(swapNumber, swapTime, CX_Millis(-1));
}

void CX_SwapSynchronizer::store(uint64_t swapNumber, CX_Millis swapTime, CX_Millis nextSwapPrediction) {

	Datum dat;
	dat.swapNumber = swapNumber;
	dat.swapTime = swapTime;
	dat.userNextSwapEst = nextSwapPrediction;

	// Store this swap in the model and estimate the next swap time given that this swap is known
	_lm.store(swapNumber, swapTime);
	dat.modelNextSwapEst = _lm.getY(swapNumber + _config.swapAdvancesUnits);


	_data.push_back(dat);
	while (_data.size() > _config.requiredSwaps) {
		_data.pop_front();
	}

	_dataChangeSinceLastTest = true;
}

bool CX_SwapSynchronizer::ready(void) const {
	return _data.size() == _config.requiredSwaps; //_lm.ready();
}

bool CX_SwapSynchronizer::synchronized(void) {
	if (!_dataChangeSinceLastTest) {
		return _syncResultOnLastTest && ready();
	}

	_syncResultOnLastTest = synchronized(_config.test);
	_dataChangeSinceLastTest = false;

	return _syncResultOnLastTest;
}

bool CX_SwapSynchronizer::synchronized(const TestConfig& test) {

	if (!ready()) {
		return false;
	}

	bool sync = true;

	if (test.swapIntervals) {
		sync = sync && testSwapIntervals();
	}

	if (test.nextSwapPredictions) {
		sync = sync && testNextSwapPredictions();
	}

	if (test.modelSlope) {
		sync = sync && testModelSlope();
	}

	if (test.modelNextSwapPredictions) {
		sync = sync && testModelNextSwapPredictions();
	}

	return sync;
}

bool CX_SwapSynchronizer::testSwapIntervals(void) {

	if (!ready()) {
		return false;
	}
	
	bool allIntWithinTol = true;
	for (unsigned int i = 0; i < (_data.size() - 1); i++) {

		CX_Millis interval = _data[i + 1].swapTime - _data[i].swapTime;

		bool intWithinTol = _areTimesWithinTolerance(interval, _config.nominalSwapPeriod, _calcTolerance);

		allIntWithinTol = allIntWithinTol && intWithinTol;
	}

	return allIntWithinTol;
}

bool CX_SwapSynchronizer::testNextSwapPredictions(void) {

	if (!ready()) {
		return false;
	}

	bool allEstWithinTol = true;
	for (unsigned int i = 0; i < (_data.size() - 1); i++) {

		CX_Millis estNext = _data[i].userNextSwapEst;
		CX_Millis actNext = _data[i + 1].swapTime;

		bool estWithinTol = _areTimesWithinTolerance(estNext, actNext, _calcTolerance);
		allEstWithinTol = allEstWithinTol && estWithinTol;
	}

	return allEstWithinTol;

}

bool CX_SwapSynchronizer::testModelNextSwapPredictions(void) {
	if (!_lm.ready()) {
		return false;
	}

	bool allEstWithinTol = true;
	for (unsigned int i = 0; i < (_data.size() - 1); i++) {

		CX_Millis estNext = _data[i].modelNextSwapEst;
		CX_Millis actNext = _data[i + 1].swapTime;

		bool estWithinTol = _areTimesWithinTolerance(estNext, actNext, _calcTolerance);
		allEstWithinTol = allEstWithinTol && estWithinTol;
	}

	return allEstWithinTol;

}

bool CX_SwapSynchronizer::testModelSlope(void) {
	if (!_lm.ready()) {
		return false;
	}

	CX_Millis slope = _lm.getMillisecondsPerUnit();

	return _areTimesWithinTolerance(slope, _config.nominalSwapPeriod, _calcTolerance);
}



bool CX_SwapSynchronizer::_areTimesWithinTolerance(CX_Millis a, CX_Millis b, CX_Millis tolerance) const {
	cxTick_t absDif = abs(a.nanos() - b.nanos());
	return absDif < tolerance.nanos();
}



} // namespace Private
} // namespace CX