#pragma once

#include "ofConstants.h"

#include <chrono>
#include <string>

namespace CX {

	class CX_BaseClock {
	public:
		virtual long long micros(void) = 0;
		virtual long long nanos(void) {
			return micros() * 1000;
		}

		virtual std::string getName(void) {
			return "CX_BaseClock";
		}
	};


	template <class stdClock>
	class CX_StdClockWrapper : public CX_BaseClock {
	public:

		CX_StdClockWrapper(void) {
			_startTime = stdClock::now();
		}

		long long micros(void) override {
			return std::chrono::duration_cast<std::chrono::microseconds>(stdClock::now() - _startTime).count();
		}

		long long nanos(void) override {
			return std::chrono::duration_cast<std::chrono::nanoseconds>(stdClock::now() - _startTime).count();
		}

	private:
		typename stdClock::time_point _startTime;
	};


#ifdef TARGET_WIN32

	class CX_WIN32_PerformanceCounterClock : public CX_BaseClock {
	public:
		CX_WIN32_PerformanceCounterClock(void);
		long long micros(void) override;
		long long nanos(void) override;

	private:
		long long _startTime;
		long long _frequency;
	};

	/* This implementation of a high resolution clock uses QueryPerformanceCounter (a Windows API call).

	Based on http://stackoverflow.com/questions/16299029/resolution-of-stdchronohigh-resolution-clock-doesnt-correspond-to-measureme/16299576#16299576
	with a few changes.
	*/
	struct CX_WIN32_HRC {
		typedef long long                               rep;
		typedef std::nano                               period;
		typedef std::chrono::duration<rep, period>      duration;
		typedef std::chrono::time_point<CX_WIN32_HRC>   time_point;
		static const bool is_steady = true;

		static time_point now();
	};

#endif

}
