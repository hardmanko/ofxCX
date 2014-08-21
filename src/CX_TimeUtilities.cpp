#include "CX_TimeUtilities.h"

namespace CX {
namespace Util {

	/////////////////
	// CX_LapTimer //
	/////////////////

	/*! Set up the CX_LapTimer with the selected clock source and the number of samples to log between each automatic logging of results.
	\param clock The instance of CX_Clock to use.
	\param logSamples If this is not 0, then every `logSamples` samples, a string containing information 
	about the last `logSamples` samples will be logged.
	*/
	void CX_LapTimer::setup(CX_Clock *clock, unsigned int logSamples) {
		_clock = clock;
		restart();

		_samplesBetweenLogging = logSamples;
	}

	void CX_LapTimer::restart(void) {
		_durationRecalculationRequired = true;
		_timePoints.clear();
	}

	void CX_LapTimer::takeSample(void) {
		_timePoints.push_back(_clock->now());
		_durationRecalculationRequired = true;

		if ((_samplesBetweenLogging != 0) && (_timePoints.size() == _samplesBetweenLogging)) {
			CX::Instances::Log.notice("CX_LapTimer") << "Data collected: " << getStatString();
			restart();
		}
	}

	unsigned int CX_LapTimer::collectedSamples(void) {
		if (_timePoints.size() == 0) {
			return 0;
		}
		return _timePoints.size() - 1;
	}

	std::string CX_LapTimer::getStatString(void) {
		std::stringstream s;

		s << "Min: " << this->min() << std::endl <<
			"Mean: " << this->mean() << std::endl <<
			"Max: " << this->max() << std::endl <<
			"Std. Dev.: " << this->stdDev() << std::endl;

		return s.str();
	}

	void CX_LapTimer::_calculateDurations(void) {
		if (_timePoints.size() < 2 || !_durationRecalculationRequired) {
			return;
		}

		_durations.resize(_timePoints.size() - 1);
		for (unsigned int i = 1; i < _timePoints.size(); i++) {
			_durations[i - 1] = (_timePoints[i] - _timePoints[i - 1]).value();
		}
		_durationRecalculationRequired = false;
	}

	CX_Millis CX_LapTimer::mean(void) {
		_calculateDurations();
		return Util::mean(_durations);
	}

	CX_Millis CX_LapTimer::max(void) {
		_calculateDurations();
		return Util::max(_durations);
	}

	CX_Millis CX_LapTimer::min(void) {
		_calculateDurations();
		return Util::min(_durations);
	}

	CX_Millis CX_LapTimer::stdDev(void) {
		_calculateDurations();
		return CX_Millis(sqrt(Util::var(_durations)));
	}




	CX_SegmentProfiler::CX_SegmentProfiler(void) :
		_clock(nullptr),
		_samplesBetweenLogging(0)
	{}

	/*! Set up the CX_SegmentProfiler with the selected clock source and the number of samples to log between each automatic logging of results.
	\param clock The instance of CX_Clock to use.
	\param logSamples If this is not 0, then every `logSamples` samples, a string containing information
	about the last `logSamples` samples will be logged.
	*/
	CX_SegmentProfiler::CX_SegmentProfiler(CX_Clock* clock, unsigned int logSamples) :
		_clock(clock),
		_samplesBetweenLogging(logSamples)
	{}

	/*! Set up the CX_SegmentProfiler with the selected clock source and the number of samples to log between each automatic logging of results.
	\param clock The instance of CX_Clock to use.
	\param logSamples If this is not 0, then every `logSamples` samples, a string containing information
	about the last `logSamples` samples will be logged.
	*/
	void CX_SegmentProfiler::setup(CX_Clock *clock, unsigned int logSamples) {
		_clock = clock;
		_samplesBetweenLogging = logSamples;
		this->restart();
	}

	/*! This function takes a timestamp at the current time and will be compared with the timestamp taken with t2(). */
	void CX_SegmentProfiler::t1(void) {
		_t1 = _clock->now();
	}

	/*! This function stores the difference between the current time and the time captured with t1().
	If enough samples have been collected, equal to the value of `logSamples` during setup(), a
	summary statistics string will be automatically logged. */
	void CX_SegmentProfiler::t2(void) {
		_durations.push_back((_clock->now() - _t1).value());
		if ((_samplesBetweenLogging != 0) && (_durations.size() == _samplesBetweenLogging)) {
			CX::Instances::Log.notice("CX_SegmentProfiler") << "Data collected: " << getStatString();
			restart();
		}
	}

	unsigned int CX_SegmentProfiler::collectedSamples(void) {
		return _durations.size();
	}

	void CX_SegmentProfiler::restart(void) {
		_durations.clear();
	}

	CX_Millis CX_SegmentProfiler::mean(void) {
		return Util::mean(_durations);
	}

	CX_Millis CX_SegmentProfiler::max(void) {
		return Util::max(_durations);
	}

	CX_Millis CX_SegmentProfiler::min(void) {
		return Util::min(_durations);
	}

	CX_Millis CX_SegmentProfiler::stdDev(void) {
		return CX_Millis(sqrt(Util::var(_durations)));
	}

	std::string CX_SegmentProfiler::getStatString(void) {
		std::stringstream s;

		s << "Min: " << this->min() << std::endl <<
			"Mean: " << this->mean() << std::endl <<
			"Max: " << this->max() << std::endl <<
			"Std. Dev.: " << this->stdDev() << std::endl;

		return s.str();
	}

}
}
