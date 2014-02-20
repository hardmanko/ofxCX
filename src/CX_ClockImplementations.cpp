#include "CX_ClockImplementations.h"

#ifdef TARGET_WIN32

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
freq = 4236547897 #Assume near 4 GHz tick rate. It could be lower, not likely to be faster
uint64_t_max = 2^64
tickPeriodDen = 1e9
adjust = 1e6

ticksPerOvf = uint64_t_max/((tickPeriodDen * adjust)/freq)
secPerOvf = ticksPerOvf/freq

hoursPerOvf = secPerOvf/60/60 #These settings result in slightly over 5 hours before an overflow occurs


The error characteristics of the clock can be calculated using this R code using the same variables from above:
multiplier = (tickPeriodDen * adjust)/freq
multiplierRound = round(multiplier, 0)
ticks = 1 * freq
nonRoundedValue = ticks * multiplier / adjust
roundedValue = ticks * multiplierRound / adjust

print(roundedValue/nonRoundedValue, digits=12)

if (abs(roundedValue/nonRoundedValue - 1) < 1e-6) {
print("Microsecond accuracy")
} else {
print("Worse than microseconds accuracy: ")
print(abs(roundedValue/nonRoundedValue - 1), digits=12)
}

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
	uint64_t multiplier = ((uint64_t)period::den * 1000000) / freq;
	return time_point(duration((relative * multiplier) / 1000000));

	//return time_point(duration(((count.QuadPart - start) * (static_cast<rep>(period::den * 100000) / freq)) / 100000));
}

#endif //TARGET_WIN32