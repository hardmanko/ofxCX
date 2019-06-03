#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <istream>
#include <ostream>
#include <thread>
#include <type_traits>
#include <memory>
#include <ctime>

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
	of the experiment in microseconds.

	An instance of this class is created for you to use. See CX::Instances::Clock.
	\ingroup timing
	*/
	class CX_Clock {
	public:

		CX_Clock(void);

		bool setup(std::shared_ptr<CX_BaseClockInterface> impl, bool resetStartTime = true, unsigned int samples = 100000);

		CX_Millis now(void) const;

		void sleep(CX_Millis t) const;
		void delay(CX_Millis t) const;

		// except if sleeping doesn't happen in such short units, interruptedSleep doesn't work (but interruptedDelay does)
		//void interruptedSleep(CX_Millis total, CX_Millis unit = CX_Micros(50), std::function<void(void)> fun = nullptr) const;
		//void interruptedDelay(CX_Millis total, CX_Millis unit = CX_Micros(50), std::function<void(void)> fun = nullptr) const;

		//CX_Millis estimateSleepPrecision(CX_Millis testLength);
		//void setSleepUnit(CX_Millis unit);
		//CX_Millis getSleepUnit(void);

		void resetExperimentStartTime(void);

		std::string getDateTimeString(const std::string& format = "%Y-%b-%d %H-%M-%S") const;
		std::string getExperimentStartDateTimeString(const std::string& format = "%Y-%b-%d %H-%M-%S") const;

		template <typename ImplType>
		void setImplementation(void) {
			std::shared_ptr<ImplType> impl = std::make_shared<ImplType>();
			setImplementation(impl);
		}

		void setImplementation(std::shared_ptr<CX_BaseClockInterface> impl);
		std::shared_ptr<CX_BaseClockInterface> getImplementation(void) const;

		/*! Struct containing results for a given clock implementation obtained from `testImplPrecision()`. */
		struct PrecisionTestResults {
			std::string summaryString; //<! A printable summary of the results.

			bool isMonotonic; //!< Whether the clock is monotonic/stable (only moves forward in time at a fixed rate).
			bool precisionWorseThanMs; //!< Whether the precision of the clock is worse than 1 millisecond.

			std::vector<double> percentiles; //<! The percentiles that were used to get the quantiles.

			struct {
				CX_Millis mean; //!< The mean intervals.
				std::vector<CX_Millis> quantiles; //<! Quantiles of the intervals, based on the `percentiles`.
			} withZeros; //!< Results when intervals of length 0 are included.

			struct {
				CX_Millis mean; 
				std::vector<CX_Millis> quantiles; //<! Quantiles of the intervals, based on the `percentiles`.
			} withoutZeros; //!< Results when intervals of length 0 are excluded.

		};

		static PrecisionTestResults testImplPrecision(std::shared_ptr<CX_BaseClockInterface> impl, 
			unsigned int samples = 100000, std::vector<double> percentiles = std::vector<double>());
		static std::pair<std::shared_ptr<CX_BaseClockInterface>, PrecisionTestResults> 
			chooseBestClockImplementation(unsigned int iterationsPerClock, bool excludeUnstable = true, bool excludeWorseThanMs = true, bool log = true);

		
		void enableRegularEvent(bool enable);
		bool isRegularEventEnabled(void);
		void setRegularEventPeriod(CX_Millis period);
		CX_Millis getRegularEventPeriod(void);
		ofEvent<void> regularEvent;

	private:
		std::time_t _experimentStartTime;

		std::shared_ptr<CX_BaseClockInterface> _impl;

		
		void _regularEventThreadFunction(void);

		struct {
			std::recursive_mutex mutex;
			std::thread thread;
			CX_Millis period;
			bool enabled;
		} _regularEvent;

		std::string _formatTime(const std::string& format, const std::time_t* time) const;
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

		virtual cxTick_t nanos(void) const = 0; //!< Returns the current time in nanoseconds.
		virtual void resetStartTime(void) = 0; //!< Resets the start time, so that an immediate call to nanos() would return 0 (on an infinitely fast computer).

		virtual std::string getName(void) const = 0; //!< Returns a helpful name describing the clock implementation.
		virtual bool isMonotonic(void) const = 0; //!< Returns `true` if the clock implementation is monotonic/stable (only moves forward at a fixed rate).
	};

	/* A generic wrapper for clocks meeting the standard library clock interface standards. 
	For example `std::chrono::high_resolution_clock`. */
	template <class stdClock>
	class CX_StdClockWrapper : public CX_BaseClockInterface {
	public:

		CX_StdClockWrapper(void) {
			resetStartTime();
		}

		cxTick_t nanos(void) const override {
			return std::chrono::duration_cast<std::chrono::nanoseconds>(stdClock::now() - _startTime).count();
		}

		void resetStartTime(void) override {
			_startTime = stdClock::now();
		}

		std::string getName(void) const override {
			return "CX_StdClockWrapper<" + std::string(typeid(stdClock).name()) + ">";
		}

		bool isMonotonic(void) const override {
			return stdClock::is_steady;
		}

	private:
		typename stdClock::time_point _startTime;
	};


#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 9 && OF_VERSION_PATCH >= 0
	/* This clock implementation uses ofGetMonotonicTime() (in ofUtils.cpp).
	*/
	class CX_ofMonotonicTimeClock : public CX_BaseClockInterface {
	public:

		CX_ofMonotonicTimeClock(void) {
			resetStartTime();
		}

		cxTick_t nanos(void) const override;
		void resetStartTime(void) override;
		
		std::string getName(void) const override {
			return "CX_ofMonotonicTimeClock";
		}

		bool isMonotonic(void) const override {
			return true;
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

		cxTick_t nanos(void) const override;
		void resetStartTime(void) override;
		void setStartTime(cxTick_t ticks); // Bonus function for this implementation

		std::string getName(void) const override {
			return "CX_WIN32_PerformanceCounterClock";
		}

		bool isMonotonic(void) const override {
			return true;
		}

	private:
		void _resetFrequency(void);

		cxTick_t _startTime;
		cxTick_t _frequency;
	};

#endif

}
