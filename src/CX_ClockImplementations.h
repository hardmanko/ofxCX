#pragma once

#include "ofConstants.h"

#ifdef TARGET_WIN32

#define CX_CLOCK_IMPLEMENTATION_COUNTS_FROM_ZERO

#include <chrono>
#include <string>

namespace CX {
	namespace Private{


		/* This implementation of a high resolution clock makes a number of assumptions that makes it well-
		suited for use in psychology experiments. Each tick is equal to one nanosecond, but it is targeted
		for microsecond precision. It uses QueryPerformanceCounter (a windows api call).

		Based on http://stackoverflow.com/questions/16299029/resolution-of-stdchronohigh-resolution-clock-doesnt-correspond-to-measureme/16299576#16299576
		with a number of significant improvements.

		*/
		struct CX_HighResClockImplementation {
			typedef long long                               rep;
			typedef std::nano                               period;
			typedef std::chrono::duration<rep, period>      duration;
			typedef std::chrono::time_point<CX_HighResClockImplementation>   time_point;
			static const bool is_steady = true;

			static time_point now();
		};
	}
}

#endif