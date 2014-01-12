#include "CX_Clock.h"

#ifdef TARGET_WIN32
#include "Windows.h"
#else
#include "ofUtils.h" //For ofGetSystemTimeMicros()
#endif

using namespace CX;

CX_Clock Instances::Clock; //Single instance of this class.

CX_Clock::CX_Clock (void) {
#ifdef TARGET_WIN32
	LARGE_INTEGER performanceFrequency;
	if (!QueryPerformanceFrequency( &performanceFrequency )) {
		ofLogFatalError("CX_Clock") << "Querying of performance frequency failed. All CX functionality is broken without a high-resolution clock, so this is a fatal error.";
	}
	_performanceCounterFrequency = performanceFrequency.QuadPart;
#endif

	_resetExperimentStartTime();
}

/*!
This function returns the current system time in microseconds. 

On Windows, the performance counter is used. On Linux and OSx, the standard OF function is used.

This cannot be converted to time/day in any meaningful way. Use getDateTimeString() for that.
*/
uint64_t CX_Clock::getSystemTime (void) {
#ifdef TARGET_WIN32
	LARGE_INTEGER temp;
	QueryPerformanceCounter( &temp );
	uint64_t count = temp.QuadPart;
	return (count * 1000)/(_performanceCounterFrequency / 1000); //Assume at least 1 GHz.
#else
	//ofGetSystemTimeMicros() does not have very strong precision guarantees.
	//For Linux, look into: clock_gettime
	//For OSx: http://stackoverflow.com/questions/11680461/monotonic-clock-on-osx

	return ofGetSystemTimeMicros();
#endif

	//return (uint64_t)_stopwatch.getSystemTime(StopwatchTimeFormat::MICROSECONDS);
}

/*!
This function returns the current time relative to the start of the experiment in microseconds.
The start of the experiment is defined by default as when the CX_Clock instance named Clock
(instantiated in this file) is constructed (typically the beginning of program execution). The 
experiment start time can be reset at any time by calling resetExperimentStartTime().
*/
uint64_t CX_Clock::getTime (void) {
	return getSystemTime() - _experimentStartTime;
}

void CX_Clock::_resetExperimentStartTime (void) {
	_experimentStartTime = getSystemTime();
}


/*!
See http://pocoproject.org/docs/Poco.DateTimeFormatter.html#4684 for documentation of the format.
E.g. "%Y/%m/%d %H:%M:%S" gives "year/month/day 24-hour-clock:minute:second" with some zero-padding for most things.
The default "%Y/%b/%e %h:%M:%S %a" is "year-with-century/abbreviated-month-name/non-zero-padded-day 12-hour-clock:minute-zero-padded:second-zero-padded am/pm".
*/
std::string CX_Clock::getDateTimeString (std::string format) {
	Poco::LocalDateTime localTime;
	return Poco::DateTimeFormatter::format(localTime, format);
}


/*
namespace CX {
//Does not actually have microsecond precision on some systems.
void sleepMicros(uint64_t micros) {
	std::this_thread::sleep_for(std::chrono::microseconds(micros));
}


long long currentTime(void) {
	//std::chrono::high_resolution_clock::time_point experimentStart = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point t = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::microseconds>(t - experimentStart).count();
}


long long micros(void) {
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

double clockPeriod(void) {
	return (double)std::chrono::high_resolution_clock::period().num / std::chrono::high_resolution_clock::period().den;
}


}
*/