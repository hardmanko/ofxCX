#pragma once

#include "CX_Clock.h"

namespace CX {
	namespace Util {

		/*! This class can be used for profiling event loops. It measures the amount of time that has elapsed
		between subsequent calls to takeSample().

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
			void setup(CX_Clock *clock, unsigned int logSamples);

			void reset(void);

			void takeSample(void);
			std::vector<double>::size_type collectedSamples(void);

			CX_Millis mean(void);
			CX_Millis min(void);
			CX_Millis max(void);
			CX_Millis stdDev(void);

			std::string getStatString(void);

		private:
			CX_Clock *_clock;
			std::vector<CX_Millis> _timePoints;
			std::vector<double> _durations;

			unsigned int _samplesBetweenLogging;

			void _calculateDurations(void);
			bool _durationRecalculationRequired;
		};

		/*! This class is used for profiling small segments of code embedded within other code.

		\code{.cpp}
		//During setup
		CX::Util::CX_SegmentProfiler profiler(&CX::Instances::Clock);

		//In main code somewhere you have a process that is repeated some number
		//of times that has the code of interest embedded in it.
		for (int i = 0; i < 100; i++) {
		//Some code you aren't interested in profiling...

		profiler.t1();
		//Code you are interested in profiling.
		profiler.t2();

		//Other code you aren't interested in profiling...
		}

		//Once the process has been performed some number of times,
		//check out the statistics for the code segment that was profiled.
		std::cout << profiler.getStatString() << std::endl;

		\endcode
		\ingroup timing
		*/
		class CX_SegmentProfiler {
		public:

			CX_SegmentProfiler(CX_Clock* clock);

			void setup(CX_Clock* clock);

			void t1(void);
			void t2(void);

			std::vector<double>::size_type collectedSamples(void) {
				return _durations.size();
			}

			void reset(void);

			std::string getStatString(void);

			CX_Millis mean(void);
			CX_Millis min(void);
			CX_Millis max(void);
			CX_Millis stdDev(void);

		private:
			CX_Clock *_clock;

			CX_Millis _t1;
			std::vector<double> _durations;

		};
	}
}