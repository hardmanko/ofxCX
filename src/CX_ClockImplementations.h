#pragma once

#include "ofConstants.h"

#include <chrono>
#include <string>
#include <sstream>
#include <type_traits>

namespace CX {

	/*! CX_Clock uses classes that are derived from this class for timing.

	nanos() should return the current time in nanoseconds. If the implementation does not
	have nanosecond precision, it should still return time in nanoseconds, which might just
	involve a multiplication (clock ticks are in microseconds, so multiply by 1000 to make
	each value equal to a nanosecond).

	It is assumed that the implementation has some way to subtract off a start time so that
	nanos() counts up from 0 and that resetStartTime can reset the start time.

	\ingroup timing
	*/
	class CX_BaseClock {
	public:
		virtual long long nanos(void) = 0;
		virtual void resetStartTime(void) = 0;
		virtual std::string getName(void) {
			return "CX_BaseClock";
		}
	};


	template <class stdClock>
	class CX_StdClockWrapper : public CX_BaseClock {
	public:

		CX_StdClockWrapper(void) {
			resetStartTime();
		}

		long long nanos(void) override {
			return std::chrono::duration_cast<std::chrono::nanoseconds>(stdClock::now() - _startTime).count();
		}

		void resetStartTime(void) override {
			_startTime = stdClock::now();
		}

		std::string getName(void) override {
			std::stringstream s;
			s << "CX_StdClockWrapper<" << typeid(stdClock).name() << ">";
			return s.str();
		}

	private:
		typename stdClock::time_point _startTime;
	};


#ifdef TARGET_WIN32

	class CX_WIN32_PerformanceCounterClock : public CX_BaseClock {
	public:
		CX_WIN32_PerformanceCounterClock(void);

		long long nanos(void) override;
		void resetStartTime(void) override;

		std::string getName(void) override {
			return "CX_WIN32_PerformanceCounterClock";
		}

	private:
		void _resetFrequency(void);

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
