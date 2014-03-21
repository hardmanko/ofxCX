#include "CX_Clock.h"

using namespace CX;

/*! An instance of CX::CX_Clock that is hooked into the CX backend. Anything in CX
that requires timing information uses this instance. You should use this instance in
your code and not make your own instance of CX_Clock. You should never need another instance. 
You should never use another instance, as the experiment start times will not agree between 
instances.
\ingroup timing */
CX_Clock CX::Instances::Clock;

CX_Clock::CX_Clock (void) {

#ifdef TARGET_WIN32
	_impl = new CX::CX_WIN32_PerformanceCounterClock();
#else
	_impl = new CX::CX_StdClockWrapper<std::chrono::high_resolution_clock>();
#endif

	resetExperimentStartTime();
}

/*! This function tests the precision of the clock used by CX. The results are computer-specific. 
If the precision of the clock is worse than microsecond accuracy, a warning is logged including 
information about the actual precision of the clock.

Depending on the number of iterations, this function may be considered blocking. See \ref blockingCode.

\param iterations Number of time duration samples to take. More iterations should give a better
estimate. */
void CX_Clock::precisionTest (unsigned int iterations) {
	std::vector<long long> durations(iterations);

	for (unsigned int i = 0; i < durations.size(); i++) {
		long long t1 = _impl->nanos();
		long long t2 = _impl->nanos();

		durations[i] = t2 - t1;
	}

	uint64_t differenceSum = 0;
	long long maxDifference = 0;
	long long minDifference = std::numeric_limits<long long>::max();
	long long minNonzeroDuration = std::numeric_limits<long long>::max();

	for (unsigned int i = 0; i < durations.size(); i++) {

		long long duration = durations[i];
		durations[i] = duration;

		differenceSum += duration;

		if (duration > maxDifference) {
			maxDifference = duration;
		}

		if (duration < minDifference) {
			minDifference = duration;
		}

		if (duration != 0 && duration < minNonzeroDuration) {
			minNonzeroDuration = duration;
		}
	}

	if (minNonzeroDuration > 1000) {
		CX::Instances::Log.warning("CX_Clock") << "The precision of the system clock used by CX_Clock appears to be worse than "
			"microsecond precision. Observed tick period of the system clock is " << minNonzeroDuration << " nanoseconds.";
	}
}

void CX_Clock::setImplementation(CX::CX_BaseClock* impl) {
	_impl = impl;
}

void CX_Clock::resetExperimentStartTime(void) {
	_pocoExperimentStart = Poco::LocalDateTime();
	if (_impl) {
		_impl->resetStartTime();
	}
}

void CX_Clock::sleep(CX_Millis t) {
	std::this_thread::sleep_for(std::chrono::nanoseconds(t.nanos()));
}

/* Get the start time of the experiment in system time. The returned value can be compared with the result of getSystemTime(). */
//CX_Micros CX_Clock::getExperimentStartTime(void) {
//	return std::chrono::duration_cast<std::chrono::microseconds>(_experimentStart.time_since_epoch()).count();
//}

/*! This function returns the current time relative to the start of the experiment in milliseconds.
The start of the experiment is defined by default as when the CX_Clock instance named Clock
(instantiated in this file) is constructed (typically the beginning of program execution).
\return A CX_Millis object containing the time.

\note This cannot be converted to current date/time in any meaningful way. Use getDateTimeString() for that.*/
CX_Millis CX_Clock::now(void) {
	return CX_Nanos(_impl->nanos());
}


/*! Get a string representing the date/time of the start of the experiment encoded according to a format.
\param format See getDateTimeString() for the definition of the format. */
std::string CX_Clock::getExperimentStartDateTimeString(std::string format) {
	return Poco::DateTimeFormatter::format(_pocoExperimentStart, format);
}


/*! This function returns a string containing the local time encoded according to some format.
\param format See http://pocoproject.org/docs/Poco.DateTimeFormatter.html#4684 for documentation of the format.
E.g. "%Y/%m/%d %H:%M:%S" gives "year/month/day 24HourClock:minute:second" with some zero-padding for most things.
The default "%Y-%b-%e %h-%M-%S %a" is "yearWithCentury-abbreviatedMonthName-nonZeroPaddedDay 12HourClock-minuteZeroPadded-secondZeroPadded am/pm".
*/
std::string CX_Clock::getDateTimeString (std::string format) {
	Poco::LocalDateTime localTime;
	return Poco::DateTimeFormatter::format(localTime, format);
}


void Util::CX_LapTimer::setup(CX_Clock *clock, unsigned int samples) {
	_clock = clock;
	_timePoints.resize(samples);
	reset();
}

void Util::CX_LapTimer::reset(void) {
	_sampleIndex = 0;
	_durationRecalculationRequired = true;
}

void Util::CX_LapTimer::takeSample(void) {
	_timePoints[_sampleIndex] = _clock->now();
	_durationRecalculationRequired = true;

	if (++_sampleIndex == _timePoints.size()) {
		CX::Instances::Log.notice("CX_LapTimer") << "Data collected: " << getStatString();
		reset();
	}
}

std::string Util::CX_LapTimer::getStatString(void) {
	std::stringstream s;
	s << "min, mean, max, stdDev: " << this->min() << ", " << this->mean() << ", " << this->max() << ", " << this->stdDev();
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
	return CX_Millis(Util::var(_durations));
}



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

std::string Util::CX_SegmentProfiler::getStatString(void) {
	std::stringstream s;
	s << "min, mean, max, stdDev: " << Util::min(_durations) << ", " << Util::mean(_durations) << ", " <<
		Util::max(_durations) << ", " << sqrt(Util::var(_durations));
	this->reset();
	return s.str();
}