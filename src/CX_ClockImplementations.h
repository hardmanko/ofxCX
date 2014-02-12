#pragma once

#include "ofConstants.h"

#ifdef TARGET_WIN32

#include <chrono>
#include <string>

//From http://stackoverflow.com/questions/16299029/resolution-of-stdchronohigh-resolution-clock-doesnt-correspond-to-measureme/16299576#16299576
//with small modifications.
struct HighResClock {
	typedef long long                               rep;
	typedef std::nano                               period;
	typedef std::chrono::duration<rep, period>      duration;
	typedef std::chrono::time_point<HighResClock>   time_point;
	static const bool is_steady = true;

	static time_point now();
};


#endif