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
	_impl = new CX::CX_WIN32_PerformanceCounterClock(); //I realize this causes a memory leak... of about 8 bytes.
#else
	_impl = new CX::CX_StdClockWrapper<std::chrono::high_resolution_clock>();
#endif
	_implSelfAllocated = true;

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

/*! Set the underlying clock implementation used by this instance of CX_Clock. You would
use this function if the default clock implementation used by CX_Clock has insufficient 
precision on your system. You can use CX::CX_StdClockWrapper to wrap any of the clocks
from the std::chrono namespace or any clock that conforms to the standard of those clocks.
\param impl A pointer to an instance of a class derived from CX::CX_BaseClockImplementation. 
*/
void CX_Clock::setImplementation(CX::CX_BaseClockImplementation* impl) {

	if (_implSelfAllocated) {
		_implSelfAllocated = false;
		delete _impl;
	}
	_impl = impl;
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
but the operating system may not provide the exact sleep time. */
void CX_Clock::sleep(CX_Millis t) {
	std::this_thread::sleep_for(std::chrono::nanoseconds(t.nanos()));
}

/*! This functions blocks for the requested period of time. This is likely more
precise than CX_Clock::sleep() because it does not give up control to the operating
system, but it wastes resources because it just sits in a spinloop for the requested
duration. This is functionally a static function. */
void CX_Clock::delay(CX_Millis t) {
	CX_Millis startTime = this->now();
	while ((this->now() - startTime) < t)
		;
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



#ifdef TARGET_WIN32

#include "Windows.h"

/*
Calculate the overflow characteristics of this implementation as follows:

uint64_t_max = 2^64 #although this uses (signed) long long, it still holds 2^64 values
den = 1e9

secPerOvf = uint64_t_max/den

hoursPerOvf = secPerOvf/60/60
yearsPerOvf = hoursPerOvf / 24 / 365

*/
CX::CX_WIN32_HRC::time_point CX::CX_WIN32_HRC::now() {
	static long long freq = []() -> long long
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		return frequency.QuadPart;
	}();

	static long long start = []() -> long long
	{
		LARGE_INTEGER scount;
		QueryPerformanceCounter(&scount);
		return scount.QuadPart;
	}();

	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);

	return time_point(duration((count.QuadPart - start) * static_cast<rep>(period::den) / freq));
}



CX::CX_WIN32_PerformanceCounterClock::CX_WIN32_PerformanceCounterClock(void) {
	_resetFrequency();
	resetStartTime();
}

long long CX::CX_WIN32_PerformanceCounterClock::nanos(void) {
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);
	return ((count.QuadPart - _startTime) * 1000000000LL) / _frequency;
}

void CX::CX_WIN32_PerformanceCounterClock::resetStartTime(void) {
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);
	_startTime = count.QuadPart;
}

void CX::CX_WIN32_PerformanceCounterClock::_resetFrequency(void) {
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	_frequency = li.QuadPart;
}

#endif //TARGET_WIN32