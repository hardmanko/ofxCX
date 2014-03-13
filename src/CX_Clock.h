#ifndef _CX_CLOCK_H_
#define _CX_CLOCK_H_

#include <string>
#include <vector>
#include <chrono>
#include <istream>
#include <ostream>

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

			CX_Millis getAverage(void);
			CX_Millis getMinimum(void);
			CX_Millis getMaximum(void);

			std::string getStatString(void);

		private:
			CX_Clock *_clock;
			vector<CX_Millis> _timePoints;
			unsigned int _sampleIndex;
		};
	}
}

#endif //_CX_CLOCK_H_
