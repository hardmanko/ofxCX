#include "CX_TimeUtilities.h"

namespace CX {

/////////////////
// CX_LapTimer //
/////////////////

/*! Set up the
\param clock The instance of CX_Clock to use.
\param logSamples If this is not 0, then every `logSamples` samples, the stats string will be logged.
*/
void Util::CX_LapTimer::setup(CX_Clock *clock, unsigned int logSamples) {
	_clock = clock;
	reset();

	_samplesBetweenLogging = logSamples;
}

void Util::CX_LapTimer::reset(void) {
	_durationRecalculationRequired = true;
}

void Util::CX_LapTimer::takeSample(void) {

	_timePoints.push_back(_clock->now());
	_durationRecalculationRequired = true;

	if ((_samplesBetweenLogging != 0) && (_timePoints.size() == _samplesBetweenLogging)) {
		CX::Instances::Log.notice("CX_LapTimer") << "Data collected: " << getStatString();
		reset();
	}

}

std::vector<double>::size_type Util::CX_LapTimer::collectedSamples(void) {
	if (_timePoints.size() == 0) {
		return 0;
	}
	return _timePoints.size() - 1;
}

std::string Util::CX_LapTimer::getStatString(void) {
	std::stringstream s;

	s << "Min: " << this->min() << std::endl <<
		"Mean: " << this->mean() << std::endl <<
		"Max: " << this->max() << std::endl <<
		"Std. Dev.: " << this->stdDev() << std::endl;

	return s.str();
}

void Util::CX_LapTimer::_calculateDurations(void) {
	if (_timePoints.size() < 2 || !_durationRecalculationRequired) {
		return;
	}

	_durations.resize(_timePoints.size() - 1);
	for (unsigned int i = 1; i < _timePoints.size(); i++) {
		_durations[i - 1] = (_timePoints[i] - _timePoints[i - 1]).value();
	}
	_durationRecalculationRequired = false;
}

CX_Millis Util::CX_LapTimer::mean(void) {
	_calculateDurations();
	return Util::mean(_durations);
}

CX_Millis Util::CX_LapTimer::max(void) {
	_calculateDurations();
	return Util::max(_durations);
}

CX_Millis Util::CX_LapTimer::min(void) {
	_calculateDurations();
	return Util::min(_durations);
}

CX_Millis Util::CX_LapTimer::stdDev(void) {
	_calculateDurations();
	return CX_Millis(sqrt(Util::var(_durations)));
}




Util::CX_SegmentProfiler::CX_SegmentProfiler(CX_Clock* clock) :
	_clock(clock)
{}

/*! Set up the CX_SegmentProfiler with the given CX_Clock as the source for timing measurements. */
void Util::CX_SegmentProfiler::setup(CX_Clock *clock) {
	_clock = clock;
	this->reset();
}

void Util::CX_SegmentProfiler::t1(void) {
	_t1 = _clock->now();
}
void Util::CX_SegmentProfiler::t2(void) {
	_durations.push_back((_clock->now() - _t1).value());
}

void Util::CX_SegmentProfiler::reset(void) {
	_durations.clear();
}

CX_Millis Util::CX_SegmentProfiler::mean(void) {
	return Util::mean(_durations);
}

CX_Millis Util::CX_SegmentProfiler::max(void) {
	return Util::max(_durations);
}

CX_Millis Util::CX_SegmentProfiler::min(void) {
	return Util::min(_durations);
}

CX_Millis Util::CX_SegmentProfiler::stdDev(void) {
	return CX_Millis(sqrt(Util::var(_durations)));
}

std::string Util::CX_SegmentProfiler::getStatString(void) {
	std::stringstream s;

	s << "Min: " << this->min() << std::endl <<
		"Mean: " << this->mean() << std::endl <<
		"Max: " << this->max() << std::endl <<
		"Std. Dev.: " << this->stdDev() << std::endl;

	return s.str();
}


}
