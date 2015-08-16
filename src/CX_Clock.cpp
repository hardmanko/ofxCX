#include "CX_Clock.h"

#include "ofUtils.cpp" //for whatever reason, the function I need (ofGetMonotonicTime) is not in the header, only in the .cpp

#define CX_NANOS_PER_SECOND 1000000000LL

namespace CX {

CX_Clock CX::Instances::Clock;

CX_Clock::CX_Clock (void) {

#if defined(TARGET_WIN32) && OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 8
	//On windows for oF 0.8.x, use the custom clock. Elsewhere, the precision is good enough with std::chrono::high_resolution_clock.
	_impl = new CX::CX_WIN32_PerformanceCounterClock();
#else
	_impl = new CX::CX_StdClockWrapper<std::chrono::high_resolution_clock>();
#endif
	_implSelfAllocated = true;

	resetExperimentStartTime();
}

/*! This function tests the precision of the clock used by CX. The results are computer-specific. 
If the precision of the clock is worse than microsecond accuracy, a warning is logged including 
information about the actual precision of the clock.

Depending on the number of iterations, this function may be blocking. See \ref blockingCode.

\param iterations Number of time duration samples to take. More iterations should give a better
estimate. 
\return A CX_Clock::PrecisionTestResults struct containing some information about the precision of the clock.
*/
CX_Clock::PrecisionTestResults CX_Clock::precisionTest(unsigned int iterations) {
	std::vector<cxTick_t> durations(iterations);

	for (unsigned int i = 0; i < durations.size(); i++) {
		//Get two timestamps with as little code in between as possible.
		cxTick_t t1 = _impl->nanos();
		cxTick_t t2 = _impl->nanos();

		durations[i] = t2 - t1;
	}

	cxTick_t maxDuration = 0;
	cxTick_t minDuration = std::numeric_limits<cxTick_t>::max();
	cxTick_t minNonzeroDuration = std::numeric_limits<cxTick_t>::max();

	for (unsigned int i = 0; i < durations.size(); i++) {

		cxTick_t duration = durations[i];
		durations[i] = duration;

		if (duration > maxDuration) {
			maxDuration = duration;
		}

		if (duration < minDuration) {
			minDuration = duration;
		}

		if (duration != 0 && duration < minNonzeroDuration) {
			minNonzeroDuration = duration;
		}
	}

	PrecisionTestResults res;

	std::stringstream ss;
	ss << iterations << " iterations of a clock precision test gave the following results: \n" <<
		"Minimum nonzero time step: " << minNonzeroDuration << " ns" << endl <<
		"Smallest time step: " << minDuration << " ns" << endl <<
		"Largest time step: " << maxDuration << " ns" << endl;

	res.summaryString = ss.str();

	res.minNonzeroDuration = CX_Nanos(minNonzeroDuration);
	res.minDuration = CX_Nanos(minDuration);
	res.maxDuration = CX_Nanos(maxDuration);

	if (res.minNonzeroDuration > CX_Millis(1)) {
		CX::Instances::Log.warning("CX_Clock") << "The precision of the system clock used by CX_Clock appears to be worse than "
			"millisecond precision. Observed tick period of the system clock is " << res.minNonzeroDuration.millis() << " milliseconds. "
			"See CX_Clock::setImplementation() for information about using a different clock source than the default one." << std::endl << res.summaryString;
	} else {
		CX::Instances::Log.notice("CX_Clock") << "Clock precision test passed: Precision is better than 1 ms.";
	}

	return res;
}

/*! Set the underlying clock implementation used by this instance of CX_Clock. You would
use this function if the default clock implementation used by CX_Clock has insufficient 
precision on your system. You can use CX::CX_StdClockWrapper to wrap any of the clocks
from the std::chrono namespace or any clock that conforms to the standard of those clocks.
You can also write your own low level clock that implements CX_BaseClockInterface.
\param impl A pointer to an instance of a class implementing CX::CX_BaseClockInterface.
\note This function resets the experiment start time of `impl`, but does not
reset the experiment start time date/time string.
*/
void CX_Clock::setImplementation(CX::CX_BaseClockInterface* impl) {
	if (_implSelfAllocated) {
		_implSelfAllocated = false;
		delete _impl;
	}
	_impl = impl;
	_impl->resetStartTime();
}

/*! If for some reason you have a long setup period before the experiment proper
starts, you could call this function so that the values returned by CX_Clock::now()
will count up from 0 starting from when this function was called.
This function also resets the experiment start date/time (see getExperimentStartDateTimeString()).
*/
void CX_Clock::resetExperimentStartTime(void) {
	_pocoExperimentStart = Poco::LocalDateTime();
	if (_impl) {
		_impl->resetStartTime();
	}
}

/*! This functions sleeps for the requested period of time. This can be somewhat
imprecise because it requests a specific sleep duration from the operating system,
but the operating system may not provide the exact sleep time.
\param t The requested sleep duration. If 0, the thread yields rather than sleeping.
*/
void CX_Clock::sleep(CX_Millis t) {
	if (t.nanos() == 0) {
		std::this_thread::yield();
	} else {
		std::this_thread::sleep_for(std::chrono::nanoseconds(t.nanos()));
	}
}

/*! This functions blocks for the requested period of time. This is likely more
precise than CX_Clock::sleep() because it does not give up control to the operating
system, but it wastes resources because it just sits in a spinloop for the requested
duration. This is effectively a static function of the CX_Clock class. */
void CX_Clock::delay(CX_Millis t) {
	CX_Millis startTime = this->now();
	while ((this->now() - startTime) < t)
		;
}

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


#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 9 && OF_VERSION_PATCH >= 0
cxTick_t CX_ofMonotonicTimeClock::nanos(void) {
	uint64_t seconds;
	uint64_t nanos;
	ofGetMonotonicTime(seconds, nanos);
	seconds -= _startTime.seconds;
	nanos -= _startTime.nanos;

	return (cxTick_t)(seconds * CX_NANOS_PER_SECOND + nanos);
}

void CX_ofMonotonicTimeClock::resetStartTime(void) {
	ofGetMonotonicTime(_startTime.seconds, _startTime.nanos);
}
#endif


#ifdef TARGET_WIN32

#include "Windows.h"

CX_WIN32_PerformanceCounterClock::CX_WIN32_PerformanceCounterClock(void) {
	_resetFrequency();
	resetStartTime();
}

//This only has at best 1 microsecond precision.
cxTick_t CX_WIN32_PerformanceCounterClock::nanos(void) {
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);

	cxTick_t adjustedCount = count.QuadPart - _startTime;
	cxTick_t seconds = adjustedCount / _frequency;

	//Note that if _frequency is greater than or equal to 10 GHz (or just slightly lower), this multiplication will overflow
	//before reaching the division.
	cxTick_t nanos = ((adjustedCount % _frequency) * CX_NANOS_PER_SECOND) / _frequency;

	return (seconds * CX_NANOS_PER_SECOND) + nanos;
}

void CX_WIN32_PerformanceCounterClock::resetStartTime(void) {
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);
	_startTime = count.QuadPart;
}

void CX_WIN32_PerformanceCounterClock::setStartTime(cxTick_t ticks) {
	_startTime = ticks;
}

std::string CX_WIN32_PerformanceCounterClock::getName(void) {
	return "CX_WIN32_PerformanceCounterClock";
}

void CX_WIN32_PerformanceCounterClock::_resetFrequency(void) {
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	_frequency = li.QuadPart;
}

#endif //TARGET_WIN32

} //namespace CX