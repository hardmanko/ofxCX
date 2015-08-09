#include "CX_TimeUtilities.h"

namespace CX {
namespace Util {

	/////////////////
	// CX_LapTimer //
	/////////////////

	CX_LapTimer::CX_LapTimer(void) :
		_clock(nullptr),
		_samplesBetweenLogging(0),
		name("")
	{}

	/*! Construct and set up a CX_LapTimer. See CX_LapTimer::setup() for a description of the parameters. */
	CX_LapTimer::CX_LapTimer(CX_Clock *clock, unsigned int logSamples) :
		name("")
	{
		setup(clock, logSamples);
	}

	/*! Set up the CX_LapTimer with the selected clock source and the number of samples to log between each automatic logging of results.
	\param clock The instance of CX_Clock to use.
	\param logSamples If this is not 0, then every `logSamples` samples, a string containing information 
	about the last `logSamples` samples will be logged and then those samples will be cleared.
	*/
	void CX_LapTimer::setup(CX_Clock *clock, unsigned int logSamples) {
		_clock = clock;
		restart();

		_samplesBetweenLogging = logSamples;
	}

	/*! Restart data collection. All collected samples are cleared. */
	void CX_LapTimer::restart(void) {
		_durationRecalculationRequired = true;
		_timePoints.clear();
	}

	/*! Take a single sample of time. If at least one previous sample has been taken, the difference 
	between the current time and the previous time is stored as the duration of that "lap" through the code. */
	void CX_LapTimer::takeSample(void) {
		_timePoints.push_back(_clock->now());
		_durationRecalculationRequired = true;

		if ((_samplesBetweenLogging != 0) && (_timePoints.size() == _samplesBetweenLogging)) {
			CX::Instances::Log.notice("CX_LapTimer") << "Stats for last " << _samplesBetweenLogging << " samples." << getStatString();
			restart();
		}
	}

	/*! Returns the number of lap durations that have been collected. */
	unsigned int CX_LapTimer::collectedSamples(void) {
		if (_timePoints.size() == 0) {
			return 0;
		}
		return _timePoints.size() - 1;
	}

	/*! Get a string summarizing some basic descriptive statistics for the currently stored lap durations. 
	\return A string containing the minimum, mean, maximum, and standard deviation, in ms, of the collected samples.
	*/
	std::string CX_LapTimer::getStatString(void) {
		std::stringstream s;

		if (this->name != "") {
			s << " Name: " << this->name << std::endl;
		}

		s << "Range: " << this->min() << ", " << this->max() << " ms" << std::endl <<
			"Mean (SD): " << this->mean() << " (" << this->stdDev() << ") ms" << std::endl;

		return s.str();
	}


	/*! \brief Get the mean value of the stored lap times. */
	CX_Millis CX_LapTimer::mean(void) {
		_calculateDurations();
		return Util::mean(_durations);
	}

	/*! \brief Get the longest stored lap time. */
	CX_Millis CX_LapTimer::max(void) {
		_calculateDurations();
		return Util::max(_durations);
	}

	/*! \brief Get the shortest stored lap time. */
	CX_Millis CX_LapTimer::min(void) {
		_calculateDurations();
		return Util::min(_durations);
	}

	/*! \brief Get the standard deviation of the stored lap times. */
	CX_Millis CX_LapTimer::stdDev(void) {
		_calculateDurations();
		return CX_Millis(sqrt(Util::var(_durations)));
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

	///////////////////////
	//CX_SegmentProfiler //
	///////////////////////

	CX_SegmentProfiler::CX_SegmentProfiler(void) :
		_clock(nullptr),
		_samplesBetweenLogging(0),
		name("")
	{}

	/*! Set up the CX_SegmentProfiler with the selected clock source and the number of samples to log between each automatic logging of results.
	\param clock The instance of CX_Clock to use.
	\param logSamples If this is not 0, then every `logSamples` samples, a string containing information
	about the last `logSamples` samples will be logged and then those samples will be cleared.
	*/
	CX_SegmentProfiler::CX_SegmentProfiler(CX_Clock* clock, unsigned int logSamples) :
		_clock(clock),
		_samplesBetweenLogging(logSamples),
		name("")
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
			CX::Instances::Log.notice("CX_SegmentProfiler") << "Stats for last " << _samplesBetweenLogging << " samples." << getStatString();
			restart();
		}
	}

	/*! \return The number of collected samples. */
	unsigned int CX_SegmentProfiler::collectedSamples(void) {
		return _durations.size();
	}

	/*! Restart data collection. All collected samples are cleared. */
	void CX_SegmentProfiler::restart(void) {
		_durations.clear();
	}

	/*! \brief Get the mean of the stored segment durations. */
	CX_Millis CX_SegmentProfiler::mean(void) {
		return Util::mean(_durations);
	}

	/*! \brief Get the longest of the stored segment durations. */
	CX_Millis CX_SegmentProfiler::max(void) {
		return Util::max(_durations);
	}

	/*! \brief Get the shortest of the stored segment durations. */
	CX_Millis CX_SegmentProfiler::min(void) {
		return Util::min(_durations);
	}

	/*! \brief Get the standard deviation of the stored segment durations. */
	CX_Millis CX_SegmentProfiler::stdDev(void) {
		return CX_Millis(sqrt(Util::var(_durations)));
	}

	/*! Get a string summarizing some basic descriptive statistics for the currently stored data.
	\return A string containing the minimum, mean, maximum, and standard deviation, in ms, of the stored data.
	*/
	std::string CX_SegmentProfiler::getStatString(void) {
		std::stringstream s;

		if (this->name != "") {
			s << " Name: " << this->name << std::endl;
		}

		s << "Range: " << this->min() << ", " << this->max() << " ms" << std::endl <<
			"Mean (SD): " << this->mean() << " (" << this->stdDev() << ") ms" << std::endl;

		return s.str();
	}

}
}
