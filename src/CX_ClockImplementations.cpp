#include "CX_ClockImplementations.h"

#ifdef TARGET_WIN32

#include "Windows.h"

/*
Calculate the overflow characteristics of this implementation as follows:

uint64_t_max = 2^64 #although this uses long long, it still holds 2^64 values
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

	return time_point( duration((count.QuadPart - start) * static_cast<rep>(period::den) / freq) );
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