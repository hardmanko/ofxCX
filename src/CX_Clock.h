#ifndef _CX_CLOCK_H_
#define _CX_CLOCK_H_

#include <string>
#include <vector>
#include <chrono>

#include "Poco/DateTimeFormatter.h"

#include "CX_Utilities.h"
#include "CX_Logger.h"

/*! \defgroup timing Timing 
This module provides methods for timestamping events in experiments.
*/

#include "CX_ClockImplementations.h"

namespace CX {

	typedef long long CX_Micros;

	struct CX_Millis {

		CX_Millis (int i);
		CX_Millis (double d) : millis(d)  {}
		CX_Millis (CX_Micros t);

		CX_Millis& operator= (int i);
		CX_Millis& operator= (double d);
		CX_Millis& operator= (CX_Micros t);

		operator CX_Micros (void);
		operator double (void);
	private:
		double millis;
	};

	
	/*! This class is responsible for getting timestamps for anything requiring timestamps. The way to
	get timing information is the function getTime(). It returns the current time relative to the start
	of the experiment in microseconds (on most systems, see getTickPeriod() to check the actual precision).

	An instance of this class is preinstantiated for you. See CX::Instances::Clock.
	\ingroup timing
	*/
	class CX_Clock {
	public:
#ifdef TARGET_WIN32
		typedef HighResClock CX_InternalClockType;
#else
		typedef std::chrono::steady_clock CX_InternalClockType;
#endif

		CX_Clock (void);

		void precisionTest(void);

		CX_Micros getTime(void);
		CX_Micros getSystemTime(void);

		CX_Micros getExperimentStartTime(void);
		std::string getExperimentStartDateTimeString(std::string format = "%Y-%b-%e %h-%M-%S %a");

		static std::string getDateTimeString (std::string format = "%Y-%b-%e %h-%M-%S %a");

	private:
		void _resetExperimentStartTime (void);

		CX_InternalClockType::time_point _experimentStart;

		Poco::LocalDateTime _pocoExperimentStart;

		double _getTheoreticalTickPeriod(void);
		
	};


	class CX_LapTimer {
	public:

		void setup(CX_Clock *clock, unsigned int samples);

		void reset(void);

		void collectData(void);

		std::string getStatString(void);

		CX_Micros getAverage(void);
		CX_Micros getMinimum(void);
		CX_Micros getMaximum(void);

	private:

		CX_Clock *_clock;
		vector<CX_Micros> _timePoints;
		unsigned int _sampleIndex;

	};



	namespace Instances {
		extern CX_Clock Clock;
	}
}

#endif //_CX_CLOCK_H_
