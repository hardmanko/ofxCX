#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <istream>
#include <ostream>
#include <sstream>
#include <thread>
#include <type_traits>

#include "Poco/DateTimeFormatter.h"

#include "ofConstants.h"

#include "CX_Utilities.h"
#include "CX_Logger.h"
#include "CX_Time_t.h"

/*! \defgroup timing Timing
This module provides methods for timestamping events in experiments.
*/

namespace CX {

	class CX_BaseClockInterface;

	/*! This class is responsible for getting timestamps for anything requiring timestamps. The way to
	get timing information is the function now(). It returns the current time relative to the start
	of the experiment in microseconds (on most systems, see getTickPeriod() to check the actual precision).

	An instance of this class is preinstantiated for you. See CX::Instances::Clock.
	\ingroup timing
	*/
	class CX_Clock {
	public:

		struct PrecisionTestResults {
			std::string summaryString;
			CX_Millis minNonzeroDuration;
			CX_Millis minDuration;
			CX_Millis maxDuration;
		};

		CX_Clock(void);
		~CX_Clock(void);

		void setImplementation(CX_BaseClockInterface* impl);

		PrecisionTestResults precisionTest(unsigned int iterations);

		CX_Millis now(void);

		void sleep(CX_Millis t);
		void delay(CX_Millis t);

		void resetExperimentStartTime(void);

		std::string getExperimentStartDateTimeString(std::string format = "%Y-%b-%e %h-%M-%S %a");
		static std::string getDateTimeString(std::string format = "%Y-%b-%e %h-%M-%S %a");

	private:
		Poco::LocalDateTime* _pocoExperimentStart;

		CX_BaseClockInterface* _impl;
		bool _implSelfAllocated;
	};

	namespace Instances {
		/*! An instance of CX::CX_Clock that is hooked into the CX backend. Anything in CX
		that requires timing information uses this instance. You should use this instance in
		your code and not make your own instance of CX_Clock. You should never need another instance.
		You should never use another instance, as the experiment start times will not agree between
		instances.
		\ingroup timing */
		extern CX_Clock Clock;
	}

	/*! CX_Clock uses classes that are derived from this class for timing. See CX::CX_Clock::setImplementation().

	nanos() should return the current time in nanoseconds. If the implementation does not
	have nanosecond precision, it should still return time in nanoseconds, which might just
	involve a multiplication (e.g. clock ticks are in microseconds, so multiply by 1000 to make
	each value equal to a nanosecond).

	It is assumed that the implementation has some way to subtract off a start time so that
	nanos() counts up from 0 and that resetStartTime() can reset the start time so that the
	clock counts up from 0 after resetStartTime() is called.

	\ingroup timing
	*/
	class CX_BaseClockInterface {
	public:
	    virtual ~CX_BaseClockInterface(void) {}

		virtual cxTick_t nanos(void) = 0; //!< Returns the current time in nanoseconds.
		virtual void resetStartTime(void) = 0; //!< Resets the start time, so that an immediate call to nanos() would return 0.

		/*! \brief Returns a helpful name describing the clock implementation. */
		virtual std::string getName(void) {
			return "CX_BaseClockInterface";
		}
	};

	/* A generic wrapper for clocks meeting the standard library clock interface standards. 
	For example `std::chrono::high_resolution_clock`. */
	template <class stdClock>
	class CX_StdClockWrapper : public CX_BaseClockInterface {
	public:

		CX_StdClockWrapper(void) {
			resetStartTime();
		}

		cxTick_t nanos(void) override {
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


#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 9 && OF_VERSION_PATCH >= 0
	/* This clock implementation uses ofGetMonotonicTime() (in ofUtils.cpp).
	*/
	class CX_ofMonotonicTimeClock : public CX_BaseClockInterface {
	public:

		cxTick_t nanos(void) override;
		void resetStartTime(void) override;

		std::string getName(void) override {
			return "CX_ofMonotonicTimeClock";
		}

	private:
		struct {
			uint64_t seconds;
			uint64_t nanos;
		} _startTime;
	};
#endif

#ifdef TARGET_WIN32
	/* This clock uses the very precise win32 `QueryPerformanceCounter` interface. */
	class CX_WIN32_PerformanceCounterClock : public CX_BaseClockInterface {
	public:
		CX_WIN32_PerformanceCounterClock(void);

		cxTick_t nanos(void) override;
		void resetStartTime(void) override;
		void setStartTime(cxTick_t ticks);

		std::string getName(void) override;

	private:
		void _resetFrequency(void);

		cxTick_t _startTime;
		cxTick_t _frequency;
	};

#endif

}
