#include "CX_ClockImplementations.h"

#include "Windows.h"

/*

The overflow characteristics of this clock implementation can be calculated as follows:

Get the frequency of the underlying clock (as reported by QueryPerformanceFrequency).

Calculate multiplier from the denominator of the tick period times some adjustment that allows the division by 
frequency to result in sub-microsecond error. multiplier = ((1e9 * 1e5)/frequency)

Divide the largest value that can be held by a 64 bit unsigned integer by multiplier (2^64/multiplier). This results
in the number of underlying clock ticks (as reported by QueryPerformanceCounter) that will be observed before an overflow
occurs.

Take this value and divide it by frequency in order to get the number of seconds between overflows.

See this R code:
freq = 4e6 #Assume 4 GHz
uint64_t_max = 2^64
tickPeriodDen = 1e9
adjust = 1e5

ticksPerOvf = uint64_t_max/((tickPeriodDen * adjust)/freq)
secPerOvf = ticksPerOvf/freq

hoursPerOvf = secPerOvf/60/60

*/
CX::Private::CX_HighResClockImplementation::time_point CX::Private::CX_HighResClockImplementation::now() {
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

	uint64_t relative = count.QuadPart - start;
	uint64_t multiplier = ((uint64_t)period::den * 100000) / freq;
	return time_point(duration((relative * multiplier) / 100000));

	//return time_point(duration(((count.QuadPart - start) * (static_cast<rep>(period::den * 100000) / freq)) / 100000));
}