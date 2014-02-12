#include "CX_ClockImplementations.h"

#include "Windows.h"

//From http://stackoverflow.com/questions/16299029/resolution-of-stdchronohigh-resolution-clock-doesnt-correspond-to-measureme/16299576#16299576
//with small modifications.
HighResClock::time_point HighResClock::now() {
	static long long freq = []() -> long long
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		return frequency.QuadPart;
	}();

	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);
	return time_point(duration(count.QuadPart * static_cast<rep>(period::den) / freq));
}