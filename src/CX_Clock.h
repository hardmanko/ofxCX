#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <istream>
#include <ostream>
#include <thread>

#include "Poco/DateTimeFormatter.h"

#include "CX_Utilities.h"
#include "CX_Logger.h"
#include "CX_ClockImplementations.h"
#include "CX_Time_t.h"

/*! \defgroup timing Timing 
This module provides methods for timestamping events in experiments.
*/

namespace CX {

	/*! This class is responsible for getting timestamps for anything requiring timestamps. The way to
	get timing information is the function now(). It returns the current time relative to the start
	of the experiment in microseconds (on most systems, see getTickPeriod() to check the actual precision).

	An instance of this class is preinstantiated for you. See CX::Instances::Clock.
	\ingroup timing
	*/
	class CX_Clock {
	public:
		CX_Clock (void);

		void setImplementation(CX::CX_BaseClock* impl);

		void precisionTest(unsigned int iterations);

		CX_Millis now(void);

		void sleep(CX_Millis t);

		void resetExperimentStartTime(void);

		std::string getExperimentStartDateTimeString(std::string format = "%Y-%b-%e %h-%M-%S %a");
		static std::string getDateTimeString (std::string format = "%Y-%b-%e %h-%M-%S %a");

	private:
		Poco::LocalDateTime _pocoExperimentStart;

		CX::CX_BaseClock* _impl;
	};

	namespace Instances {
		extern CX_Clock Clock;
	}


	namespace Util {

		/*! This class can be used for profiling event loops.

		\code{.cpp}
		//Set up collection:
		CX_LapTimer lt;
		lt.setup(&Clock, 1000); //Every 1000 samples, the results of those samples will be logged.

		//In the loop:
		while (whatever) {
		//other code...
		lt.takeSample();
		//other code...
		}
		Log.flush(); //Check the results of the profiling.

		\endcode

		\ingroup timing
		*/
		class CX_LapTimer {
		public:
			void setup(CX_Clock *clock, unsigned int samples);

			void reset(void);

			void takeSample(void);

			CX_Millis mean(void);
			CX_Millis min(void);
			CX_Millis max(void);
			CX_Millis stdDev(void);

			std::string getStatString(void);

		private:
			CX_Clock *_clock;
			std::vector<CX_Millis> _timePoints;
			std::vector<double> _durations;
			unsigned int _sampleIndex;

			void _calculateDurations(void);
			bool _durationRecalculationRequired;
		};

		class CX_SegmentProfiler {
		public:
			void setup(CX_Clock *clock);

			void t1(void);
			void t2(void);

			void reset(void);

			std::string getStatString(void);

		private:
			CX_Clock *_clock;

			CX_Millis _t1;
			std::vector<double> _durations;

		};
	}
}
