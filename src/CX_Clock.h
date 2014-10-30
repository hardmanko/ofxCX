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
		CX_Clock (void);

		void setImplementation(CX_BaseClockInterface* impl);

		std::string precisionTest(unsigned int iterations);

		CX_Millis now(void);

		void sleep(CX_Millis t);
		void delay(CX_Millis t);

		void resetExperimentStartTime(void);

		std::string getExperimentStartDateTimeString(std::string format = "%Y-%b-%e %h-%M-%S %a");
		static std::string getDateTimeString (std::string format = "%Y-%b-%e %h-%M-%S %a");

	private:
		Poco::LocalDateTime _pocoExperimentStart;

		CX_BaseClockInterface* _impl;
		bool _implSelfAllocated;
	};

	namespace Instances {
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

		virtual long long nanos(void) = 0;
		virtual void resetStartTime(void) = 0;
		virtual std::string getName(void) {
			return "CX_BaseClock";
		}
	};


	template <class stdClock>
	class CX_StdClockWrapper : public CX_BaseClockInterface {
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

	class CX_WIN32_PerformanceCounterClock : public CX_BaseClockInterface {
	public:
		CX_WIN32_PerformanceCounterClock(void);

		long long nanos(void) override;
		void resetStartTime(void) override;
		void setStartTime(long long ticks) {
			_startTime = ticks;
		}

		std::string getName(void) override {
			return "CX_WIN32_PerformanceCounterClock";
		}

	private:
		void _resetFrequency(void);

		long long _startTime;
		long long _frequency;
	};

#endif

}
